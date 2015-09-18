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
#include "filesystem.h"
#include "module.h"

#if defined(_WINDOWS)
#include <windows.h>

static bool throw_win32(void)
{
	return false;
}

bool MCModuleLoad(const char *p_filename, MCModuleRef& r_module)
{
	bool t_success;
	t_success = true;

	void *t_native_filename;
	t_native_filename = nil;
	if (t_success)
		t_success = MCFileSystemPathToNative(p_filename, t_native_filename);

	HMODULE t_module;
	t_module = nil;
	if (t_success)
	{
		t_module = LoadLibraryW((LPCWSTR)t_native_filename);
		if (t_module == nil)
			t_success = throw_win32();
	}

	if (t_success)
		r_module = (MCModuleRef)t_module;

	MCMemoryDeallocate(t_native_filename);

	return t_success;
}

void MCModuleUnload(MCModuleRef self)
{
	if (self == nil)
		return;

	FreeLibrary((HMODULE)self);
}

bool MCModuleLookupSymbol(MCModuleRef self, const char *p_symbol, void **r_address)
{
	void *t_address;
	t_address = GetProcAddress((HMODULE)self, p_symbol);
	*r_address = t_address;
	return true;
}

bool MCModuleGetFilename(MCModuleRef p_module, char*& r_path)
{
	bool t_success;
	t_success = true;

	// For some unfathomable reason, it is not possible find out how big a
	// buffer you might need for a module file name. Instead we loop until
	// we are sure we have the whole thing.
	unichar_t *t_wpath;
	uint32_t t_wpath_length;
	t_wpath_length = 0;
	t_wpath = nil;
	while(t_success)
	{
		t_wpath_length += 256;
		MCMemoryDeleteArray(t_wpath);

		if (t_success)
			t_success = MCMemoryNewArray(t_wpath_length, t_wpath);

		DWORD t_result;
		t_result = 0;
		if (t_success)
		{
			// If the buffer is too small, the result will equal the input
			// buffer size.
			t_result = GetModuleFileNameW((HMODULE)p_module, t_wpath, t_wpath_length);
			if (t_result == 0)
				t_success = false;
			else if (t_result == t_wpath_length)
				continue;
		}

		if (t_success)
			break;
	}

	// Convert the unicode string to UTF-8.
	if (t_success)
		t_success = MCFileSystemPathFromNative(t_wpath, r_path);

	MCMemoryDeleteArray(t_wpath);

	return t_success;
}

#elif defined(_LINUX)
#include <dlfcn.h>

bool MCModuleLoad(const char *p_filename, MCModuleRef& r_module)
{
	MCModuleRef t_module;
	t_module = (void *)dlopen(p_filename, (RTLD_NOW | RTLD_LOCAL));
	if (t_module == nil)
		return false;

	r_module = t_module;

	return true;
}

void MCModuleUnload(MCModuleRef self)
{
	if (self != nil)
		dlclose(self);
}

bool MCModuleLookupSymbol(MCModuleRef self, const char *p_symbol, void **r_address)
{
	void *t_address;
	t_address = dlsym(self, p_symbol);
	if (t_address == nil)
		return false;

	*r_address = t_address;

	return true;
}

bool MCModuleGetFilename(MCModuleRef p_module, char*& r_path)
{
	Dl_info t_info;
	if (p_module == nil && dladdr((void *)MCModuleGetFilename, &t_info) != 0)
		return MCCStringClone(t_info . dli_fname, r_path);

	return false;
}
#elif defined(_MACOSX)
#include <dlfcn.h>
#include <CoreFoundation/CoreFoundation.h>

bool MCModuleLoad(const char *p_filename, MCModuleRef& r_module)
{
	MCModuleRef t_module;
	if (MCCStringEndsWith(p_filename, ".dylib"))
	{
		t_module = (void *)dlopen(p_filename, (RTLD_NOW | RTLD_LOCAL));
		if (t_module == nil)
			return false;
	}
	else
	{
		CFURLRef t_url;
		t_url = CFURLCreateFromFileSystemRepresentation(kCFAllocatorDefault, (const UInt8 *)p_filename, MCCStringLength(p_filename), TRUE);
		if (t_url != nil)
		{
			t_module = (void *)CFBundleCreate(kCFAllocatorDefault, t_url);
			if (t_module != nil)
				t_module = (void *)((uintptr_t)t_module | 1);
			CFRelease(t_url);
		}
		
		if (t_module == nil)
			return false;
	}
	
	r_module = t_module;
	
	return true;
}

void MCModuleUnload(MCModuleRef self)
{
	if (self == nil)
		return;
	
	if (((uintptr_t)self & 1) == 0)
		dlclose(self);
	else
		CFRelease((CFBundleRef)((uintptr_t)self & ~1));
}

bool MCModuleLookupSymbol(MCModuleRef self, const char *p_symbol, void **r_address)
{
	if (self == nil)
		return false;
	
	void *t_address;
	if (((uintptr_t)self & 1) == 0)
	{
		t_address = dlsym(self, p_symbol);
		if (t_address == nil)
			return false;
	}
	else
	{
		CFStringRef t_symbol;
		if (!MCCStringToCFString(p_symbol, t_symbol))
			return false;
		t_address = CFBundleGetFunctionPointerForName((CFBundleRef)((uintptr_t)self & ~1), t_symbol);
		CFRelease(t_symbol);
		
		if (t_address == nil)
			return false;
	}
	
	*r_address = t_address;
	
	return true;
}

bool MCModuleGetFilename(MCModuleRef p_module, char*& r_path)
{
	Dl_info t_info;
	if (p_module == nil && dladdr((void *)MCModuleGetFilename, &t_info) != 0)
		return MCCStringClone(t_info . dli_fname, r_path);
	
	return false;
}
#endif
