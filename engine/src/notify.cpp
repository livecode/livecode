/* Copyright (C) 2003-2015 LiveCode Ltd.

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

#include "notify.h"

#if !defined(FEATURE_NOTIFY)
#	error MCNotify API not supported on this platform
#endif

#if defined(_MAC_DESKTOP)
#include "osxprefix.h"
#include <pthread.h>
#define USE_PTHREADS
#define USE_PINGORPIPE
#define PING_FUNC MCMacBreakWait
#elif defined(_MAC_SERVER)
#include <unistd.h>
#include <pthread.h>
#define USE_PTHREADS
#define USE_PIPE
#elif defined(_LINUX_DESKTOP) || defined(_LINUX_SERVER)
#include <pthread.h>
#include <unistd.h>
#define USE_PTHREADS
#define USE_PIPE
#elif defined(_WINDOWS_DESKTOP) || defined(_WINDOWS_SERVER)
#define USE_WINTHREADS
#elif defined(_IOS_MOBILE)
#include <pthread.h>
#define USE_PTHREADS
#define USE_PING
#define PING_FUNC MCIPhoneBreakWait
#elif defined(_ANDROID_MOBILE)
#include <pthread.h>
#define USE_PTHREADS
#define USE_PING
#define PING_FUNC MCAndroidBreakWait
#endif

extern Boolean MCnoui;

struct MCNotifySyncEvent
{
	MCNotifySyncEvent *next;
#if defined(USE_WINTHREADS)
	HANDLE object;
#elif defined(USE_PTHREADS)
    pthread_mutex_t mutex;
	pthread_cond_t cond;
	bool triggered;
#else
#error Threading API not specified
#endif
};

struct MCNotification
{
	MCNotification *next;
    // MW-2014-10-23: [[ Bug 13721 ]] Required field indicates signature of callback.
    bool required;
    union
    {
        void (*callback)(void *);
        void (*required_callback)(void *, int);
    };
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
#if defined(USE_WINTHREADS)
HANDLE g_notify_wakeup = NULL;
static CRITICAL_SECTION s_notify_lock;
static DWORD s_main_thread_id = 0;
#elif defined(USE_PTHREADS)
static bool s_notify_sent = false;
static pthread_mutex_t s_notify_lock;
static pthread_t s_main_thread;
#if defined(USE_PIPE) || defined(USE_PINGORPIPE)
int g_notify_pipe[2] = {-1, -1};
#endif
#else
#error Threading API not specified
#endif

////////////////////////////////////////////////////////////////////////////////

static MCNotifySyncEvent *MCNotifySyncEventCreate(void)
{
	if (s_sync_events != NULL)
		return MCListPopFront(s_sync_events);

	MCNotifySyncEvent *t_event;
	t_event = new (nothrow) MCNotifySyncEvent;
	t_event -> next = NULL;

#if defined(USE_WINTHREADS)
	t_event -> object = CreateEvent(NULL, FALSE, FALSE, NULL);
#elif defined(USE_PTHREADS)
	pthread_mutex_init(&t_event -> mutex, 0);
	pthread_cond_init(&t_event -> cond, 0);
	t_event -> triggered = false;
#else
#error Threading API not specified
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
    
#if defined(USE_WINTHREADS)
	CloseHandle(self -> object);
#elif defined(USE_PTHREADS)
	pthread_cond_destroy(&self -> cond);
	pthread_mutex_destroy(&self -> mutex);
#else
#error Threading API not specified
#endif

	delete self;
}

static void MCNotifySyncEventTrigger(MCNotifySyncEvent *self)
{
#if defined(USE_WINTHREADS)
	SetEvent(self -> object);
#elif defined(USE_PTHREADS)
	pthread_mutex_lock(&self -> mutex);
	self -> triggered = true;
	pthread_cond_signal(&self -> cond);
	pthread_mutex_unlock(&self -> mutex);
#else
#error Threading API not specified
#endif
}

static void MCNotifySyncEventReset(MCNotifySyncEvent *self)
{
#if defined(USE_WINTHREADS)
	ResetEvent(self -> object);
#elif defined(USE_PTHREADS)
	pthread_mutex_lock(&self -> mutex);
	self -> triggered = false;
	pthread_mutex_unlock(&self -> mutex);
#else
#error Threading API not specified
#endif
}

static void MCNotifySyncEventWait(MCNotifySyncEvent *self)
{
#if defined(USE_WINTHREADS)
	WaitForSingleObject(self -> object, INFINITE);
#elif defined(USE_PTHREADS)
	pthread_mutex_lock(&self -> mutex);
	while(!self -> triggered)
		pthread_cond_wait(&self -> cond, &self -> mutex);
	pthread_mutex_unlock(&self -> mutex);
#else
#error Threading API not specified
#endif
}

////////////////////////////////////////////////////////////////////////////////

static void MCNotifyLock(void)
{
#if defined(USE_WINTHREADS)
	EnterCriticalSection(&s_notify_lock);
#elif defined(USE_PTHREADS)
	pthread_mutex_lock(&s_notify_lock);
#else
#error Threading API not specified
#endif
}

static void MCNotifyUnlock(void)
{
#if defined(USE_WINTHREADS)
	LeaveCriticalSection(&s_notify_lock);
#elif defined(USE_PTHREADS)
	pthread_mutex_unlock(&s_notify_lock);
#else
#error Threading API not specified
#endif
}

////////////////////////////////////////////////////////////////////////////////

// MW-2013-06-25: [[ DesktopPingWait ]] Checks to see if the current thread is
//   the main thread, returning 'true' if it so.
static bool MCNotifyIsMainThread(void)
{
#if defined(USE_WINTHREADS)
	return GetCurrentThreadId() == s_main_thread_id;
#elif defined(USE_PTHREADS)
	return pthread_equal(pthread_self(), s_main_thread);
#else
#error Threading API not specified
#endif
}

////////////////////////////////////////////////////////////////////////////////

bool MCNotifyInitialize(void)
{
	if (s_initialized)
		return true;

	s_initialized = true;
	
	// MW-2013-10-01: [[ Bug 11166 ]] Make sure we reset the shutdown flag (Android
	//   reinitialization).
	s_shutting_down = false;

	// MW-2013-06-25: [[ DesktopPingWait ]] Initialize the main thread references
	//   needed by 'MCNotifyIsMainThread()'.
#if defined(USE_WINTHREADS)
	g_notify_wakeup = CreateEvent(NULL, FALSE, FALSE, NULL);
	InitializeCriticalSection(&s_notify_lock);
	s_main_thread_id = GetCurrentThreadId();
#elif defined(USE_PTHREADS)
	pthread_mutex_init(&s_notify_lock, NULL);
	s_main_thread = pthread_self();
#if defined(USE_PIPE)
	pipe(g_notify_pipe);
#elif defined(USE_PINGORPIPE)
    if (MCnoui)
        pipe(g_notify_pipe);
#endif
#else
#error Threading API not specified
#endif
	
	return true;
}

static void MCNotifyFinalizeList(MCNotification*& p_list)
{
	while(p_list != NULL)
	{
		MCNotification *t_notify;
		t_notify = MCListPopFront(p_list);
        
        // MW-2014-10-23: [[ Bug 13721 ]] If the callback is required, then invoke it with 1 as the
        //   flags. This tells it not to take action, but just to free up state.
        if (t_notify -> required)
            t_notify -> required_callback(t_notify -> state, 1);
        
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
	
	// MW-2013-10-01: [[ Bug 11166 ]] Make sure we reset the initialized flag (Android
	//   reinitialization).
	s_initialized = false;

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
    
#if defined(USE_WINTHREADS)
	DeleteCriticalSection(&s_notify_lock);
	CloseHandle(g_notify_wakeup);
#elif defined(USE_PTHREADS)
#if defined(USE_PIPE)
	close(g_notify_pipe[0]);
	close(g_notify_pipe[1]);
#elif defined(USE_PINGORPIPE)
    if (MCnoui)
    {
        close(g_notify_pipe[0]);
        close(g_notify_pipe[1]);
    }
#endif
	pthread_mutex_destroy(&s_notify_lock);
#else
#error Threading API not specified
#endif
}

// MW-2014-10-23: [[ Bug 13721 ]] Added 'required' parameter. Indicates whether the callback should always
//   be invoked. If in action context, then with 0, otherwise in finalize context with 1 (should free up state
//  but not do anything else.
bool MCNotifyPush(void (*p_callback)(void *), void *p_state, bool p_block, bool p_safe, bool p_required)
{
	bool t_success;
	t_success = true;

	// MW-2013-06-25: [[ DesktopPingWait ]] The request for a blocking, non-safe notification
	// on the main thread should just invoke the callback.
	if (p_block && !p_safe && MCNotifyIsMainThread())
	{
        // MW-2014-10-23: [[ Bug 13721 ]] If the callback is required, then it expects a 0 second
        //   argument if it should take action (otherwise 1 to free up, but not take action).
        if (p_required)
            ((void(*)(void *, int))p_callback)(p_state, 0);
        else
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
		t_notification = new (nothrow) MCNotification;

	// Fill it in.
	if (t_success)
	{
        // MW-2014-10-23: [[ Bug 13721 ]] Add the required field (indicates callback sig).
		t_notification -> next = NULL;
        t_notification -> required = p_required;
		t_notification -> callback = p_callback;
		t_notification -> state = p_state;
		t_notification -> notify = NULL;

		// If we are shutting down, then we have failed.
		if (s_shutting_down)
			t_success = false;
		else
		{
			// Assign a sync event if we want to block
			if (p_block)
            {
                MCNotifyLock();
				t_notification -> notify = MCNotifySyncEventCreate();
                MCNotifyUnlock();
            }

			if (!p_block || t_notification -> notify != NULL)
			{
                // Enter the critical region
                MCNotifyLock();
                
				// Add the notification to the queue
				if (p_safe)
					MCListPushBack(s_safe_notifications, t_notification);
				else
					MCListPushBack(s_notifications, t_notification);

                // MM-2015-06-12: [[ Bug ]] Make sure we unlock before calling ping. Not doing so can cause deadlock.
                MCNotifyUnlock();
                
				// MW-2013-06-25: [[ DesktopPingWait ]] Moved to MCNotifyPing().
				// Ping the main thread to make sure it knows to check for a shiny new
				// thing.
				MCNotifyPing(p_block);
			}
			else
				t_success = false;
		}
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
#if defined(USE_WINTHREADS)
			ResetEvent(g_notify_wakeup);
#elif defined(USE_PTHREADS)
			s_notify_sent = false;
#else
#error Threading API not specified
#endif
			t_notify = MCListPopFront(p_list);
			MCNotifyUnlock();
            
			// MW-2014-10-23: [[ Bug 13721 ]] If the callback is required then use a different
            //   signature.
            if (t_notify -> required)
                t_notify -> required_callback(t_notify -> state, 0);
            else
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
// MM-2015-06-12: [[ Bug ]] Make sure we lock around s_notify_sent.
void MCNotifyPing(bool p_high_priority)
{
#if defined(USE_WINTHREADS)
	SetEvent(g_notify_wakeup);
#elif defined(USE_PTHREADS)
    if (!s_notify_sent)
    {
        MCNotifyLock();
        if (!s_notify_sent)
        {
            s_notify_sent = true;
            MCNotifyUnlock();

#if defined(USE_PING)
            extern void PING_FUNC(void);
            PING_FUNC();
#elif defined(USE_PIPE)
            char t_notify_char = 1;
            write(g_notify_pipe[1], &t_notify_char, 1);
#elif defined(USE_PINGORPIPE)
            if (!MCnoui)
            {
                extern void PING_FUNC(void);
                PING_FUNC();
            }
            else
            {
                char t_notify_char = 1;
                write(g_notify_pipe[1], &t_notify_char, 1);
            }
#endif
        }
        else
            MCNotifyUnlock();
    }
#else
#error Threading API not specified
#endif
}

bool MCNotifyPending(void)
{
	return s_notifications != nil || s_safe_notifications != nil;
}
