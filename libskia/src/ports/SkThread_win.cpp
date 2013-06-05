/* libs/graphics/ports/SkThread_none.cpp
**
** Copyright 2008, The Android Open Source Project
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

#include <windows.h>
#include "SkThread.h"

namespace {

template <bool>
struct CompileAssert {
};

}  // namespace

#define COMPILE_ASSERT(expr, msg) \
  typedef CompileAssert<(bool(expr))> msg[bool(expr) ? 1 : -1]

int32_t sk_atomic_inc(int32_t* addr)
{
    // InterlockedIncrement returns the new value, we want to return the old.
    return InterlockedIncrement(reinterpret_cast<LONG*>(addr)) - 1;
}

int32_t sk_atomic_dec(int32_t* addr)
{
    return InterlockedDecrement(reinterpret_cast<LONG*>(addr)) + 1;
}

SkMutex::SkMutex(bool /* isGlobal */)
{
    COMPILE_ASSERT(sizeof(fStorage) > sizeof(CRITICAL_SECTION),
                   NotEnoughSizeForCriticalSection);
    InitializeCriticalSection(reinterpret_cast<CRITICAL_SECTION*>(&fStorage));
}

SkMutex::~SkMutex()
{
    DeleteCriticalSection(reinterpret_cast<CRITICAL_SECTION*>(&fStorage));
}

void SkMutex::acquire()
{
    EnterCriticalSection(reinterpret_cast<CRITICAL_SECTION*>(&fStorage));
}

void SkMutex::release()
{
    LeaveCriticalSection(reinterpret_cast<CRITICAL_SECTION*>(&fStorage));
}

