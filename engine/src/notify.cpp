/* Copyright (C) 2003-2013 Runtime Revolution Ltd.

This file is part of LiveCode.

LiveCode is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License v3 as published by the Free
Software Foundation.

LiveCode is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with LiveCode.  If not see <http://www.gnu.org/licenses/>.  */

#include "prefix.h"

#include "core.h"
#include "notify.h"

#if defined(_MAC_DESKTOP)
#include "osxprefix.h"
#include <pthread.h>
#elif defined(_LINUX_DESKTOP)
#include <pthread.h>
#include <unistd.h>
#elif defined(_WINDOWS_DESKTOP)
#include "w32prefix.h"
#elif defined(_IOS_MOBILE) || defined(_ANDROID_MOBILE)
#include <pthread.h>
#endif

struct MCNotifySyncEvent
{
	MCNotifySyncEvent *next;
#if defined(_WINDOWS)
	HANDLE object;
#elif defined(_MACOSX) || defined(_LINUX) || defined(_IOS_MOBILE) || defined(_ANDROID_MOBILE)
	pthread_mutex_t mutex;
	pthread_cond_t cond;
	bool triggered;
#endif
};

struct MCNotification
{
	MCNotification *next;
	void (*callback)(void *);
	void *state;
	MCNotifySyncEvent *notify;
};

// We keep a list of allocated, but currently unused synchronization event objects
static MCNotifySyncEvent *s_sync_events = nil;

// This is a list of pending notifications.
static MCNotification *s_notifications = nil;
static MCNotification *s_safe_notifications = nil;

// The notification system has been initialized.
static bool s_initialized = false;

// This is true if the notification system is being shutdown
static bool s_shutting_down = false;

// MW-2013-06-25: [[ DesktopPingWait ]] Added main thread ids for all platforms
//   so that MCNotifyPush can work correctly regardless of thread.
#if defined(_WINDOWS)
HANDLE g_notify_wakeup = NULL;
static CRITICAL_SECTION s_notify_lock;
static DWORD s_main_thread_id = 0;
#elif defined(_MACOSX)
static bool s_notify_sent = false;
static pthread_mutex_t s_notify_lock;
static pthread_t s_main_thread;
#elif defined(_LINUX)
static bool s_notify_sent = false;
int g_notify_pipe[2] = {-1, -1};
static pthread_mutex_t s_notify_lock;
static pthread_t s_main_thread;
#elif defined(_IOS_MOBILE) || defined(_ANDROID_MOBILE)
static bool s_notify_sent = false;
static pthread_mutex_t s_notify_lock;
static pthread_t s_main_thread;
#endif

////////////////////////////////////////////////////////////////////////////////

static MCNotifySyncEvent *MCNotifySyncEventCreate(void)
{
	if (s_sync_events != NULL)
		return MCListPopFront(s_sync_events);

	MCNotifySyncEvent *t_event;
	t_event = new MCNotifySyncEvent;
	t_event -> next = NULL;

#if defined(_WINDOWS)
	t_event -> object = CreateEvent(NULL, FALSE, FALSE, NULL);
#elif defined(_MACOSX) || defined(_LINUX) || defined(_IOS_MOBILE) || defined(_ANDROID_MOBILE)
	pthread_mutex_init(&t_event -> mutex, 0);
	pthread_cond_init(&t_event -> cond, 0);
	t_event -> triggered = false;
#endif

	return t_event;
}

static void MCNotifySyncEventDestroy(MCNotifySyncEvent *self, bool p_force)
{
	if (!p_force)
	{
		self -> next = s_sync_events;
		s_sync_events = self;
		return;
	}

#if defined(_WINDOWS)
	CloseHandle(self -> object);
#elif defined(_MACOSX) || defined(_LINUX) || defined(_IOS_MOBILE) || defined(_ANDROID_MOBILE)
	pthread_cond_destroy(&self -> cond);
	pthread_mutex_destroy(&self -> mutex);
#endif

	delete self;
}

static void MCNotifySyncEventTrigger(MCNotifySyncEvent *self)
{
#if defined(_WINDOWS)
	SetEvent(self -> object);
#elif defined(_MACOSX) || defined(_LINUX) || defined(_IOS_MOBILE) || defined(_ANDROID_MOBILE)
	pthread_mutex_lock(&self -> mutex);
	self -> triggered = true;
	pthread_cond_signal(&self -> cond);
	pthread_mutex_unlock(&self -> mutex);
#endif
}

static void MCNotifySyncEventReset(MCNotifySyncEvent *self)
{
#if defined(_WINDOWS)
	ResetEvent(self -> object);
#elif defined(_MACOSX) || defined(_LINUX) || defined(_IOS_MOBILE) || defined(_ANDROID_MOBILE)
	pthread_mutex_lock(&self -> mutex);
	self -> triggered = false;
	pthread_mutex_unlock(&self -> mutex);
#endif
}

static void MCNotifySyncEventWait(MCNotifySyncEvent *self)
{
#if defined(_WINDOWS)
	WaitForSingleObject(self -> object, INFINITE);
#elif defined(_MACOSX) || defined(_LINUX) || defined(_IOS_MOBILE) || defined(_ANDROID_MOBILE)
	pthread_mutex_lock(&self -> mutex);
	while(!self -> triggered)
		pthread_cond_wait(&self -> cond, &self -> mutex);
	pthread_mutex_unlock(&self -> mutex);
#endif
}

////////////////////////////////////////////////////////////////////////////////

static void MCNotifyLock(void)
{
#if defined(_WINDOWS)
	EnterCriticalSection(&s_notify_lock);
#elif defined(_MACOSX) || defined(_LINUX) || defined(_IOS_MOBILE) || defined(_ANDROID_MOBILE)
	pthread_mutex_lock(&s_notify_lock);
#endif
}

static void MCNotifyUnlock(void)
{
#if defined(_WINDOWS)
	LeaveCriticalSection(&s_notify_lock);
#elif defined(_MACOSX) || defined(_LINUX) || defined(_IOS_MOBILE) || defined(_ANDROID_MOBILE)
	pthread_mutex_unlock(&s_notify_lock);
#endif
}

////////////////////////////////////////////////////////////////////////////////

// MW-2013-06-25: [[ DesktopPingWait ]] Checks to see if the current thread is
//   the main thread, returning 'true' if it so.
static bool MCNotifyIsMainThread(void)
{
#if defined(_WINDOWS)
	return GetCurrentThreadId() == s_main_thread_id;
#elif defined(_MACOSX) || defined(_LINUX) || defined(_IOS_MOBILE) || defined(_ANDROID_MOBILE)
	return pthread_self() == s_main_thread;
#else
	// TODO: Implement for other platforms (inc. _WINDOWS_SERVER)
	return true;
#endif
}

////////////////////////////////////////////////////////////////////////////////

#if defined(_MACOSX)
extern bool g_osx_dispatch_event;
static OSStatus MCNotifyEventHandler(EventHandlerCallRef p_ref, EventRef p_event, void *p_data)
{
	MCNotifyDispatch(g_osx_dispatch_event);
}
#endif

bool MCNotifyInitialize(void)
{
	if (s_initialized)
		return true;

	s_initialized = true;

	// MW-2013-06-25: [[ DesktopPingWait ]] Initialize the main thread references
	//   needed by 'MCNotifyIsMainThread()'.
#if defined(_WINDOWS)
	g_notify_wakeup = CreateEvent(NULL, FALSE, FALSE, NULL);
	InitializeCriticalSection(&s_notify_lock);
	s_main_thread_id = GetCurrentThreadId();
#elif defined(_MACOSX)
	static EventTypeSpec t_events[] = { 'revo', 'wkup' };
	::InstallApplicationEventHandler(MCNotifyEventHandler, 1, t_events, NULL, NULL);
	pthread_mutex_init(&s_notify_lock, NULL);
	s_main_thread = pthread_self();
#elif defined(_LINUX)
	pthread_mutex_init(&s_notify_lock, NULL);
	pipe(g_notify_pipe);
	s_main_thread = pthread_self();
#elif defined(_IOS_MOBILE) || defined(_ANDROID_MOBILE)
	pthread_mutex_init(&s_notify_lock, NULL);
	s_main_thread = pthread_self();
#endif
	return true;
}

static void MCNotifyFinalizeList(MCNotification*& p_list)
{
	while(p_list != NULL)
	{
		MCNotification *t_notify;
		t_notify = MCListPopFront(p_list);

		// Make sure we release any pending threads.
		if (t_notify -> notify != NULL)
		{
			// Set the notify handle to NIL to stop the other thread destroying
			// it.
			MCNotifySyncEvent *t_event;
			t_event = t_notify -> notify;

			t_notify -> notify = NULL;
			MCNotifySyncEventTrigger(t_event);

			MCNotifySyncEventDestroy(t_event, true);
		}
		else
			delete t_notify;
	}
}

void MCNotifyFinalize(void)
{
	// Mark the notification system as shutting down.
	s_shutting_down = true;

	// Make sure all pending notifications are eradicated.
	MCNotifyLock();
	MCNotifyFinalizeList(s_notifications);
	MCNotifyFinalizeList(s_safe_notifications);
	MCNotifyUnlock();

	// Destroy all the sync events we created.
	while(s_sync_events != nil)
	{
		MCNotifySyncEvent *t_event;
		t_event = MCListPopFront(s_sync_events);
		
		MCNotifySyncEventDestroy(t_event, true);
	}

#if defined(_WINDOWS)
	DeleteCriticalSection(&s_notify_lock);
	CloseHandle(g_notify_wakeup);
#elif defined(_MACOSX) || defined(_IOS_MOBILE) || defined(_ANDROID_MOBILE)
	pthread_mutex_destroy(&s_notify_lock);
#elif defined(_LINUX)
	pthread_mutex_destroy(&s_notify_lock);
	close(g_notify_pipe[0]);
	close(g_notify_pipe[1]);
#endif
}

bool MCNotifyPush(void (*p_callback)(void *), void *p_state, bool p_block, bool p_safe)
{
	bool t_success;
	t_success = true;

	// MW-2013-06-25: [[ DesktopPingWait ]] The request for a blocking, non-safe notification
	// on the main thread should just invoke the callback.
	if (p_block && !p_safe && MCNotifyIsMainThread())
	{
		p_callback(p_state);
		return true;
	}
	
	// MW-2013-06-25: [[ DesktopPingWait ]] The request for a blocking, safe notification on the
	//   main thread is an error.
	if (p_block && p_safe && MCNotifyIsMainThread())
		return false;
	
	// Create a new notification
	MCNotification *t_notification;
	t_notification = NULL;
	if (t_success)
		t_notification = new MCNotification;

	// Fill it in.
	if (t_success)
	{
		t_notification -> next = NULL;
		t_notification -> callback = p_callback;
		t_notification -> state = p_state;
		t_notification -> notify = NULL;

		// Enter the critical region
		MCNotifyLock();

		// If we are shutting down, then we have failed.
		if (s_shutting_down)
			t_success = false;
		else
		{
			// Assign a sync event if we want to block
			if (p_block)
				t_notification -> notify = MCNotifySyncEventCreate();

			if (!p_block || t_notification -> notify != NULL)
			{
				// Add the notification to the queue
				if (p_safe)
					MCListPushBack(s_safe_notifications, t_notification);
				else
					MCListPushBack(s_notifications, t_notification);

				// MW-2013-06-25: [[ DesktopPingWait ]] Moved to MCNotifyPing().
				// Ping the main thread to make sure it knows to check for a shiny new
				// thing.
				MCNotifyPing(p_block);
			}
			else
				t_success = false;
		}

		// Unlock
		MCNotifyUnlock();
	}

	if (t_success)
	{
		if (p_block)
		{
			// Wait for the event to fire
			MCNotifySyncEventWait(t_notification -> notify);

			// Reset the sync event and destroy it, but only if its still there
			if (t_notification -> notify != NULL)
			{
				MCNotifySyncEventReset(t_notification -> notify);

				// Take the lock and destroy the sync event.
				MCNotifyLock();
				MCNotifySyncEventDestroy(t_notification -> notify, false);
				MCNotifyUnlock();
			}

			delete t_notification;
		}
	}
	else
		delete t_notification;

	return t_success;
}

static bool MCNotifyDispatchList(MCNotification*& p_list)
{
	bool t_dispatched = false;
	if (p_list != NULL)
	{
		t_dispatched = true;
		while(p_list != NULL)
		{
			MCNotification *t_notify;

			MCNotifyLock();
#ifdef _WINDOWS
			ResetEvent(g_notify_wakeup);
#elif defined(_MACOSX) || defined(_LINUX) || defined(_IOS_MOBILE) || defined(_ANDROID_MOBILE)
			s_notify_sent = false;
#endif
			t_notify = MCListPopFront(p_list);
			MCNotifyUnlock();

			// Invoke the callback
			t_notify -> callback(t_notify -> state);

			// Notify the blocking thread which will destroy the event
			if (t_notify -> notify != NULL)
				MCNotifySyncEventTrigger(t_notify -> notify);
			else
			{
				// Delete the notification
				delete t_notify;
			}
		}
	}

	return t_dispatched;
}

bool MCNotifyDispatch(bool p_safe)
{
	bool t_dispatched;
	t_dispatched = MCNotifyDispatchList(s_notifications);

	if (p_safe)
		if (MCNotifyDispatchList(s_safe_notifications))
			t_dispatched = true;

	return t_dispatched;
}

// MW-2013-06-25: [[ DesktopPingWait ]] Wake up the event loop.
void MCNotifyPing(bool p_high_priority)
{
#if defined(_WINDOWS)
	SetEvent(g_notify_wakeup);
#elif defined(_MACOSX)
	if (!s_notify_sent)
	{
		s_notify_sent = true;
		EventRef t_event;
		::CreateEvent(NULL, 'revo', 'wkup', 0, kEventAttributeNone, &t_event);
		::PostEventToQueue(::GetMainEventQueue(), t_event, p_high_priority ? kEventPriorityHigh : kEventPriorityStandard);
		::ReleaseEvent(t_event);
	}
#elif defined(_LINUX)
	if (!s_notify_sent)
	{
		s_notify_sent = true;
		char t_notify_char = 1;
		write(g_notify_pipe[1], &t_notify_char, 1);
	}
#elif defined(_IOS_MOBILE)
	extern void MCIPhoneBreakWait(void);
	MCIPhoneBreakWait();
#elif defined(_ANDROID_MOBILE)
	extern void MCAndroidBreakWait(void);
	MCAndroidBreakWait();
#endif
}
