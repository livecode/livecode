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
#include "thunk.h"

#if defined(_WINDOWS)

#include <windows.h>
#include <cstdlib>

// This holds the handle to a private heap that will allocate executable memory.
// We can't use normal memory for this as 'Data Execution Prevention' would
// cause faults when we try and run our dynamically built code.
//
static HANDLE s_thunk_heap = nil;

struct Trampoline
{
#pragma pack(push, 1)
	unsigned short lea_ecx;
	unsigned long this_ptr;
	unsigned char mov_eax;
	unsigned long jump_ptr;
	unsigned short jmp_eax;
#pragma pack(pop)
};

struct TrampolineStdCall
{
#pragma pack(push, 1)
	unsigned char pop_eax;
	unsigned char push_ptr;
	unsigned long this_ptr;
	unsigned char push_eax;
	unsigned char mov_eax;
	unsigned long jump_ptr;
	unsigned short jmp_eax;
#pragma pack(pop)
};

bool MCThunkInitialize(void)
{
	s_thunk_heap = HeapCreate(HEAP_CREATE_ENABLE_EXECUTE, 0, 0);
	if (s_thunk_heap == nil)
		return false;
	return true;
}

void MCThunkFinalize(void)
{
	if (s_thunk_heap != nil)
	{
		HeapDestroy(s_thunk_heap);
		s_thunk_heap = nil;
	}
}

bool MCThunkNew(void *p_object, void *p_method, void*& r_closure)
{
	// If the heap isn't initialized yet, then initialize it.
	if (s_thunk_heap == nil)
		return false;

	// Allocate the trampoline structure in our exectuable heap.
	Trampoline *t_trampoline;
	t_trampoline = static_cast<Trampoline *>(HeapAlloc(s_thunk_heap, 0, sizeof(Trampoline)));
	if (t_trampoline == nil)
		return false;

	// Set up the trampoline code. This sequence of bytes is the following asm:
	//   lea ecx, ds:[p_object]
	//   mov eax, p_method
	//   jmp eax
	//
	t_trampoline -> lea_ecx = 0x0D8D;
	t_trampoline -> this_ptr = (long)p_object;
	t_trampoline -> mov_eax = 0xB8;
	t_trampoline -> jump_ptr = (long)p_method;
	t_trampoline -> jmp_eax = 0xE0FF;

	// On some architectures, explicit flushing needs to be performed on the
	// instruction cache to make sure stale code is not executed.
	FlushInstructionCache(GetCurrentProcess(), t_trampoline, sizeof(Trampoline));

	r_closure = t_trampoline;

	return true;
}

bool MCThunkNewStdCall(void *p_object, void *p_method, void*& r_closure)
{
	// If the heap isn't initialized yet, then initialize it.
	if (s_thunk_heap == nil)
		return false;

	// Allocate the trampoline structure in our exectuable heap.
	TrampolineStdCall *t_trampoline;
	t_trampoline = static_cast<TrampolineStdCall *>(HeapAlloc(s_thunk_heap, 0, sizeof(TrampolineStdCall)));
	if (t_trampoline == nil)
		return false;

	// Set up the trampoline code. This sequence of bytes is the following asm:
	//   pop eax
	//   push <p_object>
	//   push eax
	//   mov eax, <method>
	//   jmp eax
	//
	t_trampoline -> pop_eax = 0x58;
	t_trampoline -> push_ptr = 0x68;
	t_trampoline -> this_ptr = (long)p_object;
	t_trampoline -> push_eax = 0x50;
	t_trampoline -> mov_eax = 0xB8;
	t_trampoline -> jump_ptr = (long)p_method;
	t_trampoline -> jmp_eax = 0xE0FF;

	// On some architectures, explicit flushing needs to be performed on the
	// instruction cache to make sure stale code is not executed.
	FlushInstructionCache(GetCurrentProcess(), t_trampoline, sizeof(Trampoline));

	r_closure = t_trampoline;

	return true;
}

void MCThunkDelete(void *p_closure)
{
	if (p_closure == nil)
		return;

	HeapFree(s_thunk_heap, 0, p_closure);
}

#else

bool MCThunkNew(void *p_object, void *p_method, void*& r_closure)
{
	return false;
}

void MCThunKDelete(void *p_closure)
{
}

#endif
