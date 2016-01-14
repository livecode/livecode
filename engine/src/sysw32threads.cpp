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

#include "w32prefix.h"
#include "prefix.h"

#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"

#include "systhreads.h"
#include <windows.h>

////////////////////////////////////////////////////////////////////////////////

struct __MCThreadPoolTask;
struct __MCThreadPoolThread;

typedef __MCThreadPoolTask *MCThreadPoolTaskRef;
typedef __MCThreadPoolThread *MCThreadPoolThreadRef;

struct __MCThreadPoolTask
{
    void				(*task)(void *);
    void				*context;
    MCThreadPoolTaskRef	next;
};

struct __MCThreadPoolThread
{
    HANDLE                  thread;
    DWORD                   thread_id;
    MCThreadPoolThreadRef	next;
};

static bool s_thread_pool_running = false;

static MCThreadPoolThreadRef s_thread_pool = NULL;
static MCThreadPoolTaskRef s_task_list_start = NULL;
static MCThreadPoolTaskRef s_task_list_end = NULL;

static MCThreadMutexRef s_task_mutex = NULL;
static MCThreadConditionRef s_task_condition = NULL;

////////////////////////////////////////////////////////////////////////////////

DWORD WINAPI MCThreadPoolThreadExecute(LPVOID p_arg)
{
    MCThreadPoolTaskRef t_task;
    t_task = NULL;
    
    while (true)
    {
        MCThreadMutexLock(s_task_mutex);
        
        while (s_thread_pool_running && s_task_list_start == NULL)
            MCThreadConditionWait(s_task_condition, s_task_mutex);
        
        if (!s_thread_pool_running)
        {
            MCThreadMutexUnlock(s_task_mutex);
            return 0;
        }
        
        if (s_task_list_start != NULL)
        {
            t_task = s_task_list_start;
            s_task_list_start = t_task -> next;            
            MCThreadMutexUnlock(s_task_mutex);

            t_task -> task(t_task -> context);
            MCMemoryDelete(t_task);
        }
    }
    
    return 0;
}

bool MCThreadPoolInitialize()
{
    bool t_success;
    t_success = true;
    
    s_task_mutex = NULL;
    s_task_condition = NULL;
    s_task_list_start = NULL;
    s_task_list_end = NULL;
    s_thread_pool_running = false;
    
    if (t_success)
        t_success = MCThreadMutexCreate(s_task_mutex);
    
    if (t_success)
        t_success = MCThreadConditionCreate(s_task_condition);
    
    if (t_success)
    {
        uint32_t t_thread_pool_size;
        t_thread_pool_size = MCMin(MCThreadGetNumberOfCores(), (uint32_t) kMCThreadPoolSize);
        
        s_thread_pool_running = true;
        for (uint32_t i = 0; i < t_thread_pool_size && t_success; i++)
        {
            __MCThreadPoolThread *t_thread;
            t_thread = NULL;
            if (t_success)
                t_success = MCMemoryNew(t_thread);
            
            if (t_success)
            {
                t_thread -> thread = CreateThread(NULL, 0, MCThreadPoolThreadExecute, NULL, 0, &t_thread -> thread_id);
                t_success = t_thread -> thread != NULL;
            }
            
            if (t_success)
            {
                t_thread -> next = s_thread_pool;
                s_thread_pool = t_thread;
            }
            else
                MCMemoryDelete(t_thread);
        }
    }
    
    if (!t_success)
    {
        s_thread_pool_running = false;
        MCThreadPoolFinalize();
    }
    
    return t_success;
}

void MCThreadPoolFinalize()
{
	MCThreadMutexRelease(s_task_mutex);
    MCThreadConditionRelease(s_task_condition);
    
    s_task_mutex = NULL;
    s_task_condition = NULL;
    s_task_list_start = NULL;
    s_task_list_end = NULL;
    s_thread_pool_running = false;
}

bool MCThreadPoolPushTask(void (*p_task)(void*), void* p_context)
{
    bool t_success;
    t_success = true;
    
    if (t_success)
        t_success = s_thread_pool_running;
    
    __MCThreadPoolTask *t_task;
    t_task = NULL;
    if (t_success)
        t_success = MCMemoryNew(t_task);
    
    if (t_success)
    {
        t_task -> task = p_task;
        t_task -> context = p_context;
        t_task -> next = NULL;
        
        MCThreadMutexLock(s_task_mutex);        
        if (s_task_list_start == NULL)
        {
            s_task_list_start = t_task;
            s_task_list_end = t_task;
        }
        else
        {
            s_task_list_end -> next = t_task;
            s_task_list_end = t_task;
        }        
        MCThreadConditionSignal(s_task_condition);
        MCThreadMutexUnlock(s_task_mutex);
    }
    
    if (!t_success)
        MCMemoryDelete(t_task);
    
    return t_success;
}

////////////////////////////////////////////////////////////////////////////////

struct __MCThreadMutex
{
    CRITICAL_SECTION	mutex;
    uint32_t			references;
};

static void MCThreadMutexDestroy(MCThreadMutexRef self)
{
	if (self != NULL)
        DeleteCriticalSection(&self -> mutex);
	MCMemoryDelete(self);
}

bool MCThreadMutexCreate(MCThreadMutexRef &r_mutex)
{
    bool t_success;
    t_success = true;
    
    __MCThreadMutex *t_mutex;
    t_mutex = NULL;
    if (t_success)
        t_success = MCMemoryNew(t_mutex);
    
    if (t_success)
	{
		InitializeCriticalSection(&t_mutex -> mutex);
        t_mutex -> references = 1;
        r_mutex = t_mutex;
    }
    else
        MCThreadMutexDestroy(t_mutex);
    
    return t_success;
}

MCThreadMutexRef MCThreadMutexRetain(MCThreadMutexRef self)
{
    if (self != NULL)
        self -> references++;
    return self;
}

void MCThreadMutexRelease(MCThreadMutexRef self)
{
 	if (self != NULL)
	{
		self -> references--;
		if (self -> references <= 0)
			MCThreadMutexDestroy(self);
	}
}

void MCThreadMutexLock(MCThreadMutexRef self)
{
    if (self != NULL)
        EnterCriticalSection(&self -> mutex);
}

void MCThreadMutexUnlock(MCThreadMutexRef self)
{
    if (self != NULL)
        LeaveCriticalSection(&self -> mutex);
}

////////////////////////////////////////////////////////////////////////////////

struct __MCThreadCondition
{
    HANDLE		condition;
    uint32_t	references;
};

static void MCThreadConditionDestroy(MCThreadConditionRef self)
{
	if (self != NULL && self -> condition != NULL)
		CloseHandle(self -> condition);
	MCMemoryDelete(self);
}

bool MCThreadConditionCreate(MCThreadConditionRef &r_condition)
{
    bool t_success;
    t_success = true;
    
    __MCThreadCondition *t_condition;
    t_condition = NULL;
    if (t_success)
        t_success = MCMemoryNew(t_condition);
    
    if (t_success)
	{
		t_condition -> condition = CreateSemaphore(NULL, 0, 10, NULL);
		t_success = t_condition -> condition != NULL;
	}
    
	if (t_success)
	{
        t_condition -> references = 1;
        r_condition = t_condition;
    }
    else
        MCThreadConditionDestroy(t_condition);
    
    return t_success;
}

MCThreadConditionRef MCThreadConditionRetain(MCThreadConditionRef self)
{
    if (self != NULL)
        self -> references++;
    return self;
}

void MCThreadConditionRelease(MCThreadConditionRef self)
{
  	if (self != NULL)
	{
		self -> references--;
		if (self -> references <= 0)
			MCThreadConditionDestroy(self);
	}
}

void MCThreadConditionWait(MCThreadConditionRef self, MCThreadMutexRef p_mutex)
{
    if (self != NULL && p_mutex != NULL)
	{
		MCThreadMutexUnlock(p_mutex);
		WaitForSingleObject(self -> condition, INFINITE);
		MCThreadMutexLock(p_mutex);
	}
}

void MCThreadConditionSignal(MCThreadConditionRef self)
{
    if (self != NULL)
		ReleaseSemaphore(self -> condition, 1, NULL);
}

////////////////////////////////////////////////////////////////////////////////

uint32_t MCThreadGetNumberOfCores()
{
    /*SYSTEM_INFO t_sysinfo;
    GetSystemInfo(&t_sysinfo);
    return (uint32_t)t_sysinfo . dwNumberOfProcessors;*/
    return 1;
}

////////////////////////////////////////////////////////////////////////////////
