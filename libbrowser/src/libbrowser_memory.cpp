/* Copyright (C) 2015 LiveCode Ltd.
 
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
// Basic memory allocation util functions

#include <core.h>

#include <libbrowser.h>

#include <stdlib.h>
#include <string.h>

static bool MCBrowserMallocWrapper(size_t p_size, void *&r_mem)
{
	void *t_mem;
	t_mem = malloc(p_size);
	if (t_mem == nil)
		return false;
	r_mem = t_mem;
	return true;
}

static bool MCBrowserReallocWrapper(void *p_mem, size_t p_new_size, void *&r_new_mem)
{
	void *t_mem;
	t_mem = realloc(p_mem, p_new_size);
	if (t_mem == nil)
		return false;
	
	r_new_mem = t_mem;
	return true;
}

static MCBrowserAllocator s_mem_allocate = MCBrowserMallocWrapper;
static MCBrowserReallocator s_mem_reallocate = MCBrowserReallocWrapper;
static MCBrowserDeallocator s_mem_deallocate = free;

//////////

bool MCBrowserMemoryAllocate(size_t p_size, void *&r_mem)
{
	return s_mem_allocate(p_size, r_mem);
}

bool MCBrowserMemoryReallocate(void *p_mem, size_t p_new_size, void *&r_new_mem)
{
	return s_mem_reallocate(p_mem, p_new_size, r_new_mem);
}

void MCBrowserMemoryDeallocate(void *p_mem)
{
	s_mem_deallocate(p_mem);
}

void MCBrowserMemoryClear(void *p_mem, size_t p_size)
{
	memset(p_mem, 0, p_size);
}

////////////////////////////////////////////////////////////////////////////////

MC_BROWSER_DLLEXPORT_DEF
void MCBrowserLibrarySetAllocator(MCBrowserAllocator p_alloc)
{
	if (p_alloc != nil)
		s_mem_allocate = p_alloc;
	else
		s_mem_allocate = MCBrowserMallocWrapper; // default allocator (malloc-based)
}

MC_BROWSER_DLLEXPORT_DEF
void MCBrowserLibrarySetDeallocator(MCBrowserDeallocator p_dealloc)
{
	if (p_dealloc != nil)
		s_mem_deallocate = p_dealloc;
	else
		s_mem_deallocate = free; // default deallocator
}

MC_BROWSER_DLLEXPORT_DEF
void MCBrowserLibrarySetReallocator(MCBrowserReallocator p_realloc)
{
	if (p_realloc != nil)
		s_mem_reallocate = p_realloc;
	else
		s_mem_reallocate = MCBrowserReallocWrapper; // default reallocator (realloc-based)
}

////////////////////////////////////////////////////////////////////////////////
