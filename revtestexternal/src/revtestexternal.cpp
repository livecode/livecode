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
#include <windows.h>
#include <process.h>

typedef void *(*windows_thread_function)(void *p_context);
struct windows_thread_info
{
	windows_thread_function m_function;
	void * m_context;
};

static unsigned int __stdcall windows_thread_callback(void *p_context)
{
	windows_thread_info *t_info = (windows_thread_info*)p_context;

	unsigned int t_result;
	t_result = (unsigned int)t_info->m_function(t_info->m_context);

	free(t_info);
	return t_result;
}

void thread_sleep(unsigned int p_seconds)
{
	Sleep(p_seconds * 1000);
}

void *thread_begin(void *(*p_callback)(void *), void *p_context)
{
	windows_thread_info *t_info = NULL;
	t_info = (windows_thread_info *)malloc(sizeof(windows_thread_info));

	t_info -> m_function = p_callback;
	t_info -> m_context = p_context;

	HANDLE t_thread;
	t_thread = (HANDLE)_beginthreadex(NULL, 0, windows_thread_callback, t_info, 0, NULL);

	return (void *)t_thread;
}

void *thread_finish(void *p_thread)
{
	WaitForSingleObject((HANDLE)p_thread, INFINITE);
	CloseHandle((HANDLE)p_thread);
	return NULL;
}
#else
#include <pthread.h>
#include <unistd.h>

void thread_sleep(unsigned int p_seconds)
{
	usleep(p_seconds * 1000 * 1000);
}

void *thread_begin(void *(*p_callback)(void *), void *p_context)
{
	pthread_t t_handle;
	if (pthread_create(&t_handle, NULL, p_callback, p_context) != 0)
		return NULL;
	return (void *)t_handle;
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
	thread_sleep(10);
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

#ifndef __ANDROID__
void revTestExternalAndroidButtonCreate(void)
{
}

void revTestExternalAndroidButtonDestroy(void)
{
}
#endif

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
