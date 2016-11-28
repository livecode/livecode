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
#include <mach-o/dyld.h>

void *_MCSDylibLoadModule(MCStringRef p_path)
{
	MCAutoStringRefAsSysString t_filename_sys;
	/* UNCHECKED */ t_filename_sys.Lock(p_path);
	
	void* t_result;
	t_result = (void *)NSAddImage(*t_filename_sys, NSADDIMAGE_OPTION_RETURN_ON_ERROR | NSADDIMAGE_OPTION_WITH_SEARCHING);
	if (t_result != nil)
		return t_result;
	
	CFURLRef t_url;
	t_url = CFURLCreateFromFileSystemRepresentation(NULL, (const UInt8 *)*t_filename_sys, strlen(*t_filename_sys), false);
	
	if (t_url == NULL)
		return NULL;
	
	t_result = CFBundleCreate(NULL, t_url);
	
	CFRelease(t_url);
	
	return t_result;
}

void _MCSDylibUnloadModule(void *p_module)
{

}

void *_MCSDylibResolveModuleSymbol(void* p_module, MCStringRef p_symbol)
{
	MCAutoStringRefAsSysString t_symbol_sys;
	/* UNCHECKED */ t_symbol_sys.Lock(p_symbol);
	
	NSSymbol t_symbol;
	t_symbol = NSLookupSymbolInImage((mach_header *)p_module, *t_symbol_sys, NSLOOKUPSYMBOLINIMAGE_OPTION_BIND_NOW);
	if (t_symbol != NULL)
		return NSAddressOfSymbol(t_symbol);
	
	CFStringRef t_cf_symbol;
	
	MCStringConvertToCFStringRef(p_symbol, t_cf_symbol);
	if (t_cf_symbol == NULL)
		return NULL;

	void *t_sym;	
	t_sym = CFBundleGetFunctionPointerForName((CFBundleRef)p_module, t_cf_symbol);
	
	CFRelease(t_cf_symbol);

	return t_sym;
}
