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
	if (!t_filename_sys.Lock(p_path))
		return nil;

#ifdef _SERVER
	return dlopen(*t_filename_sys, RTLD_LAZY);
#else
	return (void *)NSAddImage(*t_filename_sys,
	                          NSADDIMAGE_OPTION_RETURN_ON_ERROR |
	                          NSADDIMAGE_OPTION_WITH_SEARCHING);
#endif
}

void _MCSDylibUnloadModule(void *p_module)
{
#ifdef _SERVER
	dlclose(p_module);
#endif
	//  Module unloading is not required after NSAddImage
}

void *_MCSDylibResolveModuleSymbol(void* p_module, MCStringRef p_symbol)
{
	MCAutoStringRefAsSysString t_symbol_sys;
	if (!t_symbol_sys.Lock(p_symbol))
		return nil;

#ifdef _SERVER
	return dlsym(p_module, *t_symbol_sys);
#else
	NSSymbol t_symbol;
	t_symbol = NSLookupSymbolInImage((mach_header *)p_module,
									 *t_symbol_sys,
									 NSLOOKUPSYMBOLINIMAGE_OPTION_BIND_NOW);
	if (t_symbol != NULL)
		return NSAddressOfSymbol(t_symbol);
#endif
	
	return nil;
}
