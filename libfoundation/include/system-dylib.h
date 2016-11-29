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

#if !defined(__MCS_SYSTEM_H_INSIDE__)
#	error "Only <foundation-system.h> can be included directly"
#endif

/* ================================================================
 * Errors
 * ================================================================ */

MC_DLLEXPORT extern MCTypeInfoRef kMCSDylibModuleNotFoundErrorTypeInfo;
MC_DLLEXPORT extern MCTypeInfoRef kMCSDylibSymbolNotFoundErrorTypeInfo;

/* ================================================================
 * Dynamic library operations
 * ================================================================ */

/* Load the module at path. */
MC_DLLEXPORT extern "C" void *MCSDylibLoadModuleCString(const char *p_path);
MC_DLLEXPORT void *MCSDylibLoadModule(MCStringRef p_path);

/* Unload the module at path. */
MC_DLLEXPORT extern "C" void MCSDylibUnloadModule(void *p_module);

/* Resolve the symbol with given name in module. */
MC_DLLEXPORT extern "C" void *MCSDylibResolveModuleSymbolCString(void* p_module, const char *p_symbol);
MC_DLLEXPORT void *MCSDylibResolveModuleSymbol(void* p_module, MCStringRef p_symbol);

#ifdef __MCS_INTERNAL_API__

void *_MCSDylibLoadModule(MCStringRef p_path);
void _MCSDylibUnloadModule(void *p_module);
void *_MCSDylibResolveModuleSymbol(void* p_module, MCStringRef p_symbol);

#endif

/* ================================================================
 * Dylib API initialization
 * ================================================================ */

#ifdef __MCS_INTERNAL_API__

bool __MCSDylibInitialize (void);
void __MCSDylibFinalize (void);

#endif
