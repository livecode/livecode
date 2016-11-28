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
#include <dlfcn.h>

void *_MCSDylibLoadModule(MCStringRef p_path)
{
	MCAutoStringRefAsSysString t_filename_sys;
	if (!t_filename_sys.Lock(p_path))
		return nil;
	
	return dlopen(*t_filename_sys, (RTLD_NOW | RTLD_LOCAL));
}

void _MCSDylibUnloadModule(void *p_module)
{
	dlclose(p_module);
}

void *_MCSDylibResolveModuleSymbol(void* p_module, MCStringRef p_symbol)
{
	MCAutoStringRefAsSysString t_symbol_sys;
	if (!t_symbol_sys.Lock(p_symbol))
		return nil;

	return dlsym(p_module, *t_symbol_sys);
}
