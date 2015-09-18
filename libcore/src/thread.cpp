/* Copyright (C) 2009-2015 LiveCode Ltd.

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

#include "core.h"
#include "thread.h"

////////////////////////////////////////////////////////////////////////////////

#if defined(_WINDOWS)

#include <windows.h>

bool MCThreadEventCreate(MCThreadEventRef& r_event)
{
	HANDLE t_event;
	t_event = CreateEvent(NULL, FALSE, FALSE, NULL);
	if (t_event == nil)
		return false;

	r_event = (MCThreadEventRef)t_event;

	return true;
}

void MCThreadEventDestroy(MCThreadEventRef self)
{
	if (self == nil)
		return;

	CloseHandle((HANDLE)self);
}

void MCThreadEventTrigger(MCThreadEventRef self)
{
	SetEvent((HANDLE)self);
}

void MCThreadEventReset(MCThreadEventRef self)
{
	ResetEvent((HANDLE)self);
}

void MCThreadEventWait(MCThreadEventRef self)
{
	WaitForSingleObject((HANDLE)self, INFINITE);
}

#elif defined(_MACOSX) || defined(_LINUX)

#include <pthread.h>

struct MCThreadEvent
{
	pthread_mutex_t mutex;
	pthread_cond_t cond;
	bool triggered;
};

bool MCThreadEventCreate(MCThreadEventRef& r_event)
{
	MCThreadEvent *self;
	if (!MCMemoryNew(self))
		return false;

	pthread_mutex_init(&self -> mutex, 0);
	pthread_cond_init(&self -> cond, 0);
	self -> triggered = false;

	r_event = self;
	return true;
}

void MCThreadEventDestroy(MCThreadEventRef self)
{
	if (self == nil)
		return;

	pthread_cond_destroy(&self -> cond);
	pthread_mutex_destroy(&self -> mutex);

	MCMemoryDelete(self);
}

void MCThreadEventTrigger(MCThreadEventRef self)
{
	// First lock the mutex - this ensures triggering is atomic from one thread
	pthread_mutex_lock(&self -> mutex);
	self -> triggered = true;

	// This call will unblock one of the threads waiting in 'pthread_cond_wait'.
	pthread_cond_signal(&self -> cond);

	// Now unlock the mutex, causing the chosen thread to continue.
	pthread_mutex_unlock(&self -> mutex);
}

void MCThreadEventReset(MCThreadEventRef self)
{
	pthread_mutex_lock(&self -> mutex);
	self -> triggered = false;
	pthread_mutex_unlock(&self -> mutex);
}

void MCThreadEventWait(MCThreadEventRef self)
{
	// Lock the mutex so wait can atomically unlock and wait
	pthread_mutex_lock(&self -> mutex);

	// We loop until the triggered var is true, this is because being woken up
	// is no guarantee that we were actually signalled to do so.
	while(!self -> triggered)
		pthread_cond_wait(&self -> cond, &self -> mutex);

	// At this point, we have a lock on the mutex so we release it as its
	// has served its purpose.
	pthread_mutex_unlock(&self -> mutex);
}

#endif

////////////////////////////////////////////////////////////////////////////////
