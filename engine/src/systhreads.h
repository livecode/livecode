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

#ifndef __MC_SYSTHREADS__
#define __MC_SYSTHREADS__

#define kMCThreadPoolSize 4

typedef struct __MCThreadCondition *MCThreadConditionRef;
typedef struct __MCThreadMutex *MCThreadMutexRef;

bool MCThreadPoolInitialize();
void MCThreadPoolFinalize();
bool MCThreadPoolPushTask(void (*task)(void*), void* context);

bool MCThreadMutexCreate(MCThreadMutexRef &r_mutex);
MCThreadMutexRef MCThreadMutexRetain(MCThreadMutexRef mutex);
void MCThreadMutexRelese(MCThreadMutexRef mutex);
void MCThreadMutexLock(MCThreadMutexRef mutex);
void MCThreadMutexUnlock(MCThreadMutexRef mutex);

bool MCThreadConditionCreate(MCThreadConditionRef &r_condition);
MCThreadConditionRef MCThreadConditionRetain(MCThreadConditionRef condition);
void MCThreadConditionRelese(MCThreadConditionRef condition);
void MCThreadConditionWait(MCThreadConditionRef condition, MCThreadMutexRef mutex);
void MCThreadConditionSignal(MCThreadConditionRef condition);

void MCThreadGlobalMutexLock();
void MCThreadGlobalMutexUnlock();
MCThreadMutexRef MCThreadGlobalMutexFetch();

#endif
