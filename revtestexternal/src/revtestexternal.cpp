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

#include <cstdio>
#include <cstdarg>
#include <stdint.h>
#include <cstring>
#include <cstdlib>

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

void revTestExternalTestArrays(void)
{
}

#ifndef __ANDROID__
void write_utf8_clef(char* t_string)
{
    t_string[0] = 0xF0;
    t_string[1] = 0x9D;
    t_string[2] = 0x84;
    t_string[3] = 0x9E;
}

void write_utf16_clef(uint16_t* t_string)
{
    t_string[0] = 0xD834;
    t_string[1] = 0xDD1E;
}

int measure_utf16(uint16_t* string)
{
    uint32_t t_size  = 0;
    
    while (*string != 0)
        string++, t_size++;
    
    return t_size;
}

char* revTestExternalTestUTF8String(const char* string)
{
    uint32_t t_length;    
    t_length = strlen(string);
    
    char* t_out = (char*)malloc(t_length + 9);
    
    write_utf8_clef(t_out);
    memcpy(t_out + 4, string, t_length);
    write_utf8_clef(t_out + 4 + t_length);
    t_out[t_length + 8] = '\0';
    
    return t_out;
}

char* revTestExternalTestUTF16String(const char* string)
{
    uint32_t t_length;
    
    t_length = measure_utf16((uint16_t*)string);
    
    uint16_t* t_out = (uint16_t*)malloc(2 * (t_length + 5));
    
    write_utf16_clef(t_out);
    memcpy(t_out + 2, string, 2 * t_length);
    write_utf16_clef(t_out + 2 + t_length);
    t_out[t_length + 4] = '\0';
    
    return (char*)t_out;
}

char *revTestExternalTestNativeString(const char* string)
{
    uint32_t t_length;
    t_length = strlen(string);
    
    char* t_out = (char*)malloc(t_length + 3);
    
    t_out[0] = '?';
    memcpy(t_out + 1, string, t_length);
    t_out[t_length + 1] = '?';
    t_out[t_length + 2] = '\0';
    
    return t_out;
}

LCBytes revTestExternalTestUTF8Data(LCBytes data)
{
    LCBytes t_out;
    t_out.buffer = malloc(data.length + 9);
    
    write_utf8_clef((char*)t_out.buffer);
    memcpy(((char*)t_out.buffer) + 4, data.buffer, data.length);
    write_utf8_clef(((char*)t_out.buffer) + 4 + data.length);
    ((char*)t_out.buffer)[data.length + 8] = '\0';
    t_out.length = data.length + 8;
    
    return t_out;
}

LCBytes revTestExternalTestUTF16Data(LCBytes data)
{    
    LCBytes t_out;
    t_out.buffer = malloc(2 * (data.length + 5));
    
    write_utf16_clef((uint16_t*)t_out.buffer);
    memcpy(((uint16_t*)t_out.buffer) + 2, data.buffer, 2 * data.length);
    write_utf16_clef(((uint16_t*)t_out.buffer) + 2 + data.length);
    ((uint16_t*)t_out.buffer)[data.length + 4] = '\0';
    t_out.length = data.length + 4;
    
    return t_out;
}

LCBytes revTestExternalTestNativeData(LCBytes data)
{
    LCBytes t_out;
    t_out.buffer = malloc(data.length + 3);
    
    ((char*)t_out.buffer)[0] = '?';
    memcpy(((char*)t_out.buffer) + 1, data.buffer, data.length);
    ((char*)t_out.buffer)[data.length + 1] = '?';
    ((char*)t_out.buffer)[data.length + 2] = '\0';
    t_out.length = data.length + 2;
    
    return t_out;
}
#endif

/*
command revTestExternalTestObjcArrays
in array as objc-array
return objc-array

command revTestExternalTestObjcDictionaries
in dict as objc-dictionary
return objc-dictionary

command revtestExternalTestObjcNumber
in number as objc-number
return objc-number

command revTestExternalTestObbjcData
in data as objc-data
return objc-data
*/

#ifndef __ANDROID__
void revTestExternalTestPostAndSend(void)
{
	LCObjectRef t_target;
	LCContextMe(&t_target);
	
	LCObjectPost(t_target, "handlePost", "");
	LCObjectSend(t_target, "handleSend", "");
	
	LCObjectRelease(t_target);
}

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
