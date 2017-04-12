/* Copyright (C) 2003-2015 LiveCode Ltd.

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

#include "prefix.h"

#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"
#include "mcio.h"



bool MCFileSystemPathToNative(const char *p_path, void*& r_native_path)
{
	unichar_t *t_w_path;
	t_w_path = nil;
	if (!MCCStringToUnicode(p_path, t_w_path))
		return false;

	for(uint32_t i = 0; t_w_path[i] != 0; i++)
		if (t_w_path[i] == '/')
			t_w_path[i] = '\\';

	r_native_path = t_w_path;

	return true;
}

bool MCFileSystemPathFromNative(const void *p_native_path, char*& r_path)
{
	char *t_path;
	t_path = nil;
	if (!MCCStringFromUnicode((const unichar_t *)p_native_path, t_path))
		return false;

	for(uint32_t i = 0; t_path[i] != 0; i++)
		if (t_path[i] == '\\')
			t_path[i] = '/';

	r_path = t_path;

	return true;
}

bool MCFileSystemListEntries(const char *p_folder, uint32_t p_options, MCFileSystemListCallback p_callback, void *p_context)
{
	bool t_success;
	t_success = true;

	char *t_pattern;
	t_pattern = nil;
	if (t_success)
		t_success = MCCStringFormat(t_pattern, "%s%s", p_folder, MCCStringEndsWith(p_folder, "/") ? "*" : "/*");

	void *t_native_pattern;
	t_native_pattern = nil;
	if (t_success)
		t_success = MCFileSystemPathToNative(t_pattern, t_native_pattern);

	HANDLE t_find_handle;
	WIN32_FIND_DATAW t_find_data;
	t_find_handle = INVALID_HANDLE_VALUE;
	if (t_success)
	{
		t_find_handle = FindFirstFileW((LPCWSTR)t_native_pattern, &t_find_data);
		if (t_find_handle == INVALID_HANDLE_VALUE)
			t_success = false;
	}

	while(t_success)
	{
		char *t_entry_filename;
		if (t_success)
			t_success = MCFileSystemPathFromNative(t_find_data . cFileName, t_entry_filename);

		MCFileSystemEntry t_entry;
		if (t_success)
		{
			t_entry . type = (t_find_data . dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0 ? kMCFileSystemEntryFolder : kMCFileSystemEntryFile;
			MCStringCreateWithCString(t_entry_filename, t_entry.filename);
			//t_entry . filename = t_entry_filename;

			t_success = p_callback(p_context, t_entry);
		}

		MCCStringFree(t_entry_filename);

		////

		if (!FindNextFileW(t_find_handle, &t_find_data))
		{
			if (GetLastError() == ERROR_NO_MORE_FILES)
				break;

			t_success = false;
		}
	}

	if (t_find_handle != INVALID_HANDLE_VALUE)
		FindClose(t_find_handle);

	MCMemoryDeallocate(t_native_pattern);
	MCCStringFree(t_pattern);

	return t_success;
}

bool MCFileSystemPathResolve(const char *p_path, char*& r_resolved_path)
{
	return MCCStringClone(p_path, r_resolved_path);
}

bool MCFileSystemPathExists(const char *p_path, bool p_folder, bool& r_exists)
{
	bool t_success;
	t_success = true;

	void *t_native_path;
	t_native_path = nil;
	if (t_success)
		t_success = MCFileSystemPathToNative(p_path, t_native_path);

	if (t_success)
	{
		DWORD t_result;
		t_result = GetFileAttributesW((LPCWSTR)t_native_path);
		if (t_result != INVALID_FILE_ATTRIBUTES)
		{
			r_exists =
				((t_result & (FILE_ATTRIBUTE_DIRECTORY)) == 0 && !p_folder) ||
				((t_result & (FILE_ATTRIBUTE_DIRECTORY)) != 0 && p_folder);
		}
		else
		{
			if (GetLastError() == ERROR_FILE_NOT_FOUND)
				r_exists = false;
			else
				t_success = false;
		}
	}

	MCMemoryDeleteArray(t_native_path);

	return t_success;
}
