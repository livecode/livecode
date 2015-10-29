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

#ifndef __MC_SYSTHREADS__
#define __MC_SYSTHREADS__

#define kMCThreadPoolSize 1

bool MCThreadPoolInitialize();
void MCThreadPoolFinalize();
bool MCThreadPoolPushTask(void (*task)(void*), void* context);

bool MCThreadMutexCreate(MCThreadMutexRef &r_mutex);
MCThreadMutexRef MCThreadMutexRetain(MCThreadMutexRef mutex);
void MCThreadMutexRelease(MCThreadMutexRef mutex);
void MCThreadMutexLock(MCThreadMutexRef mutex);
void MCThreadMutexUnlock(MCThreadMutexRef mutex);

bool MCThreadConditionCreate(MCThreadConditionRef &r_condition);
MCThreadConditionRef MCThreadConditionRetain(MCThreadConditionRef condition);
void MCThreadConditionRelease(MCThreadConditionRef condition);
void MCThreadConditionWait(MCThreadConditionRef condition, MCThreadMutexRef mutex);
void MCThreadConditionSignal(MCThreadConditionRef condition);

uint32_t MCThreadGetNumberOfCores(void);

#ifndef WIN32

static inline __attribute__((always_inline)) int32_t MCThreadAtomicInc(int32_t* addr) {
    return __sync_fetch_and_add(addr, 1);
}

static inline __attribute__((always_inline)) int32_t MCThreadAtomicDec(int32_t* addr) {
    return __sync_fetch_and_add(addr, -1);
}

#else

#pragma push_macro("_interlockedbittestandset")
#pragma push_macro("_interlockedbittestandreset")
#pragma push_macro("_interlockedbittestandset64")
#pragma push_macro("_interlockedbittestandreset64")
#define _interlockedbittestandset _local_interlockedbittestandset
#define _interlockedbittestandreset _local_interlockedbittestandreset
#define _interlockedbittestandset64 _local_interlockedbittestandset64
#define _interlockedbittestandreset64 _local_interlockedbittestandreset64
#include <intrin.h>
#pragma pop_macro("_interlockedbittestandreset64")
#pragma pop_macro("_interlockedbittestandset64")
#pragma pop_macro("_interlockedbittestandreset")
#pragma pop_macro("_interlockedbittestandset")
#pragma intrinsic(_ReadWriteBarrier)

#pragma intrinsic(_InterlockedIncrement, _InterlockedExchangeAdd, _InterlockedDecrement)
#pragma intrinsic(_InterlockedCompareExchange)

static inline int32_t MCThreadAtomicInc(int32_t* addr) {
    // InterlockedIncrement returns the new value, we want to return the old.
    return _InterlockedIncrement(reinterpret_cast<long*>(addr)) - 1;
}

static inline int32_t MCThreadAtomicDec(int32_t* addr) {
    // InterlockedDecrement returns the new value, we want to return the old.
    return _InterlockedDecrement(reinterpret_cast<long*>(addr)) + 1;
}

#endif

#endif
