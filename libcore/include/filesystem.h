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

#ifndef __MC_FILESYSTEM__
#define __MC_FILESYSTEM__

#ifndef __MC_CORE__
#include "core.h"
#endif

////////////////////////////////////////////////////////////////////////////////

enum MCFileSystemEntryType
{
	kMCFileSystemEntryFile,
	kMCFileSystemEntryFolder,
	kMCFileSystemEntryLink,
	kMCFileSystemEntryPackage
};

struct MCFileSystemEntry
{
	MCFileSystemEntryType type;
	const char *filename;
};

typedef bool (*MCFileSystemListCallback)(void *context, const MCFileSystemEntry& entry);

bool MCFileSystemListEntries(const char *folder, uint32_t options, MCFileSystemListCallback callback, void *p_context);

////////////////////////////////////////////////////////////////////////////////

bool MCFileSystemPathExists(const char *path, bool folder, bool& exists);

////////////////////////////////////////////////////////////////////////////////

bool MCFileSystemPathToNative(const char *path, void*& r_native_path);
bool MCFileSystemPathFromNative(const void *native_path, char*& r_path);

bool MCFileSystemPathResolve(const char *path, char*& r_resolved_path);

////////////////////////////////////////////////////////////////////////////////

#endif
