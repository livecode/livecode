/*
Copyright (C) 2016 LiveCode Ltd.

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

#include "system-private.h"

#include <foundation-auto.h>
#ifdef _SERVER
#include <dlfcn.h>
#else
#include <mach-o/dyld.h>
#endif

void *_MCSDylibLoadModule(MCStringRef p_path)
{
	MCAutoStringRefAsSysString t_filename_sys;
	if (t_filename_sys.Lock(p_path))
		return nil;

#ifdef _SERVER
	return dlopen(*t_utf_path, RTLD_LAZY);
#else
	CFURLRef t_url;
	t_url = CFURLCreateFromFileSystemRepresentation(NULL, (const UInt8 *)*t_filename_sys, strlen(*t_filename_sys), false);
	
	if (t_url == NULL)
		return NULL;
	
	void *t_result = CFBundleCreate(NULL, t_url);
	
	CFRelease(t_url);

	return t_result;
#endif
}

void _MCSDylibUnloadModule(void *p_module)
{
#ifdef _SERVER
	dlclose(p_module);
#else
	CFRelease((CFBundleRef)p_module);
#endif
}

void *_MCSDylibResolveModuleSymbol(void* p_module, MCStringRef p_symbol)
{
#ifdef _SERVER
	MCAutoStringRefAsSysString t_symbol_sys;
	if (!t_symbol_sys.Lock(p_symbol))
		return nil;
	
	return dlsym(p_module, *t_symbol_sys);
#else
	
	CFStringRef t_cf_symbol;
	
	MCStringConvertToCFStringRef(p_symbol, t_cf_symbol);
	if (t_cf_symbol == NULL)
		return NULL;

	void *t_sym;	
	t_sym = CFBundleGetFunctionPointerForName((CFBundleRef)p_module, t_cf_symbol);
	
	CFRelease(t_cf_symbol);

	return t_sym;
#endif
}
