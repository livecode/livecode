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

////////////////////////////////////////////////////////////////////////////////

#include "systhreads.h"

#include "prefix.h"

#include "core.h"
#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"

#include <pthread.h>

////////////////////////////////////////////////////////////////////////////////

struct __MCThreadPoolTask;
struct __MCThreadPoolThread;

typedef __MCThreadPoolTask *MCThreadPoolTaskRef;
typedef __MCThreadPoolThread *MCThreadPoolThreadRef;

struct __MCThreadPoolTask
{
    void (*task)(void*);
    void* context;
    MCThreadPoolTaskRef next;
};

struct __MCThreadPoolThread
{
    pthread_t               thread;
    MCThreadPoolThreadRef   next;
};

static MCThreadPoolThreadRef s_thread_pool = NULL;
static MCThreadPoolTaskRef s_task_list_start = NULL;
static MCThreadPoolTaskRef s_task_list_end = NULL;

static bool s_thread_pool_running = false;

static MCThreadMutexRef s_task_mutex = NULL;
static MCThreadConditionRef s_task_condition = NULL;

#if defined(_MAC_DESKTOP) || defined(_MAC_SERVER) || defined(_DARWIN_SERVER) || defined(_IOS_MOBILE)
extern void *MCMacPlatfromCreateAutoReleasePool();
extern void MCMacPlatformReleaseAutoReleasePool(void *pool);
#endif

void *MCThreadPoolThreadExecute(void *)
{
#if defined(_MAC_DESKTOP) || defined(_MAC_SERVER) || defined(_DARWIN_SERVER) || defined(_IOS_MOBILE)
    void *t_pool;
    t_pool = MCMacPlatfromCreateAutoReleasePool();
#endif
    
    pthread_setname_np("Thread Pool Worker");
    
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
            pthread_exit(NULL);
            return NULL;
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
    
#if defined(_MAC_DESKTOP) || defined(_MAC_SERVER) || defined(_DARWIN_SERVER) || defined(_IOS_MOBILE)
    MCMacPlatformReleaseAutoReleasePool(t_pool);
#endif
    return NULL;
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
        s_thread_pool_running = true;
        for (uint32_t i = 0; i < kMCThreadPoolSize && t_success; i++)
        {
            __MCThreadPoolThread *t_thread;
            t_thread = NULL;
            if (t_success)
                t_success = MCMemoryNew(t_thread);
            
            if (t_success)
                t_success = pthread_create(&t_thread -> thread, NULL, MCThreadPoolThreadExecute, NULL) == 0;
            
            if (t_success)
            {
                t_thread -> next = s_thread_pool;;
                s_thread_pool = t_thread;
            }
            else
            {
                //if (t_thread != NULL)
                //    pthread_destroy(&t_thread -> thread);
                MCMemoryDelete(t_thread);
            }
        }
    }
    
    if (!t_success)
    {
        s_thread_pool_running = true;
        MCThreadPoolFinalize();
    }
    
    return t_success;
}

bool MCThreadPoolFinalize()
{
    
    MCThreadMutexRelese(s_task_mutex);
    MCThreadConditionRelese(s_task_condition);
    
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
    pthread_mutex_t mutex;
    uint32_t        references;
};

static void MCThreadMutexDestroy(MCThreadMutexRef self)
{
	if (self != NULL)
        pthread_mutex_destroy(&self -> mutex);
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
        t_success = pthread_mutex_init(&t_mutex -> mutex, NULL) == 0;
    
    if (t_success)
    {
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

void MCThreadMutexRelese(MCThreadMutexRef self)
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
        pthread_mutex_lock(&self -> mutex);   
}

void MCThreadMutexUnlock(MCThreadMutexRef self)
{
    if (self != NULL)
        pthread_mutex_unlock(&self -> mutex);
}

////////////////////////////////////////////////////////////////////////////////

struct __MCThreadCondition
{
    pthread_cond_t  condition;
    uint32_t        references;
};

static void MCThreadConditionDestroy(MCThreadConditionRef self)
{
	if (self != NULL)
        pthread_cond_destroy(&self -> condition);
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
        t_success = pthread_cond_init(&t_condition -> condition, NULL) == 0;
    
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

void MCThreadConditionRelese(MCThreadConditionRef self)
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
        pthread_cond_wait(&self -> condition, &p_mutex -> mutex);
}

void MCThreadConditionSignal(MCThreadConditionRef self)
{
    if (self != NULL)
        pthread_cond_signal(&self -> condition);
}

////////////////////////////////////////////////////////////////////////////////

static pthread_mutex_t s_main_thread_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t s_main_thread_condition = PTHREAD_COND_INITIALIZER;

void MCThreadMainThreadMutexLock()
{
    pthread_mutex_lock(&s_main_thread_mutex);
}

void MCThreadMainThreadMutexUnlock()
{
    pthread_mutex_unlock(&s_main_thread_mutex);
}

void MCThreadMainThreadConditionSignal()
{
    pthread_cond_signal(&s_main_thread_condition);
}

void MCThreadMainThreadConditionWait()
{
    pthread_cond_wait(&s_main_thread_condition, &s_main_thread_mutex);
}

////////////////////////////////////////////////////////////////////////////////