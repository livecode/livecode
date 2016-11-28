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

extern "C" MC_DLLEXPORT_DEF
void *MCSDylibLoadModule(const char *p_path)
{
	void *t_module;
	t_module = _MCSDylibLoadModule(MCSTR(p_path));
	
	if (t_module == nil)
	{
		MCErrorCreateAndThrow(kMCSDylibModuleNotFoundErrorTypeInfo,
		                      "path", p_path, nil);
	}
	
	return t_module;
}

extern "C" MC_DLLEXPORT_DEF
void MCSDylibUnloadModule(void *p_module)
{
	_MCSDylibUnloadModule(p_module);
}

extern "C" MC_DLLEXPORT_DEF
void *MCSDylibResolveModuleSymbol(void* p_module, const char *p_symbol)
{
	void *t_symbol;
	t_symbol = _MCSDylibResolveModuleSymbol(p_module, MCSTR(p_symbol));
	
	if (t_symbol == nil)
	{
		MCErrorCreateAndThrow(kMCSDylibSymbolNotFoundErrorTypeInfo,
		                      "name", p_symbol, nil);
	}
	
	return t_symbol;
}

/* ================================================================
 * Initialization
 * ================================================================ */

MC_DLLEXPORT_DEF MCTypeInfoRef kMCSDylibModuleNotFoundErrorTypeInfo;
MC_DLLEXPORT_DEF MCTypeInfoRef kMCSDylibSymbolNotFoundErrorTypeInfo;

bool
__MCSDylibInitialize (void)
{
	/* Create error types */
	if (!MCNamedErrorTypeInfoCreate(
	        MCNAME("livecode.lang.DylibModuleNotFoundError"),
			MCNAME("dylib"),
	        MCSTR("Unable to load module '%{path}': not found"),
	        kMCSDylibModuleNotFoundErrorTypeInfo))
		return false;

	if (!MCNamedErrorTypeInfoCreate(
	        MCNAME("livecode.lang.DylibSymbolNotFoundError"),
			MCNAME("dylib"),
	        MCSTR("Unable to resolve symbol '%{name}'"),
	        kMCSDylibSymbolNotFoundErrorTypeInfo))
		return false;

	return true;
}

void
__MCSDylibFinalize (void)
{
	MCValueRelease (kMCSDylibModuleNotFoundErrorTypeInfo);
	MCValueRelease (kMCSDylibSymbolNotFoundErrorTypeInfo);
}
