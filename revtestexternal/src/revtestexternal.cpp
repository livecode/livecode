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

#include <cstdio>
#include <cstdarg>

#include <LiveCode.h>

////////////////////////////////////////////////////////////////////////////////

#if defined(__WINDOWS__)
#else
#include <pthread.h>
#include <unistd.h>

void thread_sleep(unsigned int p_seconds)
{
	usleep(p_seconds * 1000);
}

void *thread_begin(void *(*p_callback)(void *), void *p_context)
{
	pthread_t t_handle;
	if (pthread_create(&t_handle, NULL, p_callback, p_context) != 0)
		return NULL;
	return t_handle;
}

void *thread_finish(void *p_thread)
{
	void *t_result;
	pthread_join((pthread_t)p_thread, &t_result);
	return t_result;
}

#endif

////////////////////////////////////////////////////////////////////////////////

static void *test_wait_callback(void *p_handle)
{
	usleep(10 * 1000 * 1000);
	LCWaitBreak((LCWaitRef)p_handle);
	return NULL;
}

void revTestExternalTestWait(void)
{
	LCWaitRef t_wait;
	LCWaitCreate(kLCWaitOptionDispatching, &t_wait);
	
	void *t_thread;
	t_thread = thread_begin(test_wait_callback, t_wait);
	if (t_thread == NULL)
		return;
	
	LCWaitRun(t_wait);
	
	thread_finish(t_thread);
	
	LCWaitRelease(t_wait);
	
	LCContextExecute("answer \"Done!\"", 0);
}

void revTestExternalTestPost(void)
{
}

void revTestExternalTestArrays(void)
{
}

////////////////////////////////////////////////////////////////////////////////

// This handler is called when the external is loaded. We use it to set all our
// static locals to default values.
bool revTestExternalStartup(void)
{
	return true;
}

// Should we have anything to clean up, we should do it here.
void revTestExternalShutdown(void)
{
}

////////////////////////////////////////////////////////////////////////////////
