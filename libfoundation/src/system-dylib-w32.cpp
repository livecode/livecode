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

#define __MCS_INTERNAL_API__
#include <foundation.h>
#include <foundation-system.h>
#include <foundation-auto.h>

#include <windows.h>

void *_MCSFileLoadModule(MCStringRef p_path)
{
	MCAutoStringRefAsWString t_path_wstr;
	if (!t_path_wstr.Lock(p_path))
		return NULL;
	
	// MW-2011-02-28: [[ Bug 9410 ]] Use the Ex form of LoadLibrary and ask it to try
	//   to resolve dependent DLLs from the folder containing the DLL first.
	HMODULE t_handle;
	t_handle = LoadLibraryExW(*t_path_wstr, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
}

void _MCSFileUnloadModule(void *p_module)
{
	FreeLibrary((HMODULE)p_module);
}

void *_MCSFileResolveModuleSymbol(void* p_module, MCStringRef p_symbol)
{
	// NOTE: symbol addresses are never Unicode and only an ANSI call exists
	return GetProcAddress((HMODULE)p_module, MCStringGetCString(p_symbol));
}
