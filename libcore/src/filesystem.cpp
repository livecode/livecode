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

#if defined _WINDOWS || defined _WINDOWS_SERVER

#include <windows.h>

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
			t_entry . filename = t_entry_filename;

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

#elif defined(_MACOSX) || defined(_LINUX) || defined(TARGET_SUBPLATFORM_ANDROID) || defined(TARGET_SUBPLATFORM_IPHONE)

#if defined(_MACOSX) || defined(TARGET_SUBPLATFORM_IPHONE)
#include <sys/syslimits.h>
#endif

#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

#ifdef _LINUX
#define PATH_MAX 4096
#endif

bool MCFileSystemPathResolve(const char *p_path, char*& r_resolved_path)
{
	char t_path[PATH_MAX];
	ssize_t t_size;
	t_size = readlink(p_path, t_path, PATH_MAX);
	if (t_size == -1 || t_size == PATH_MAX)
	{
		if (errno == EINVAL)
			return MCCStringClone(p_path, r_resolved_path);
		return false;
	}
	t_path[t_size] = '\0';
	return MCCStringClone(t_path, r_resolved_path);
}

bool MCFileSystemPathExists(const char *p_path, bool p_folder, bool& r_exists)
{
	char *t_resolved_path;
	if (!MCFileSystemPathResolve(p_path, t_resolved_path))
		return false;

	struct stat t_stat;
	if (stat(t_resolved_path, &t_stat) == 0)
	{
		r_exists =
			(p_folder && (t_stat . st_mode & S_IFDIR) != 0) ||
			(!p_folder && (t_stat . st_mode & S_IFDIR) == 0);
	}
	else
		r_exists = false;

	MCCStringFree(t_resolved_path);

	return true;
}

#include <sys/types.h>
#include <dirent.h>

bool MCFileSystemListEntries(const char *p_folder, uint32_t p_options, MCFileSystemListCallback p_callback, void *p_context)
{
	bool t_success = true;
	
	char *t_resolved_path = nil;
	DIR *t_dir = nil;
	t_success = MCFileSystemPathResolve(p_folder, t_resolved_path);
	
	if (t_success)
	{
		t_dir = opendir(t_resolved_path);
		t_success = t_dir != nil;
	}
	
	if (t_success)
	{
		struct dirent *t_entry = nil;
		struct stat t_entry_stat;
		MCFileSystemEntry t_fs_entry;
		while (t_success && nil != (t_entry = readdir(t_dir)))
		{
			if (!MCCStringEqual(t_entry->d_name, ".") && !MCCStringEqual(t_entry->d_name, ".."))
			{
				char *t_child_path = nil;
				t_success = MCCStringFormat(t_child_path, "%s/%s", t_resolved_path, t_entry->d_name);
				if (t_success)
					t_success = -1 != lstat(t_child_path, &t_entry_stat);
				MCCStringFree(t_child_path);
				if (t_success)
				{
					t_fs_entry.filename = t_entry->d_name;
					if (S_ISLNK(t_entry_stat.st_mode))
						t_fs_entry.type = kMCFileSystemEntryLink;
					else if (S_ISDIR(t_entry_stat.st_mode))
						t_fs_entry.type = kMCFileSystemEntryFolder;
					else
						t_fs_entry.type = kMCFileSystemEntryFile;
					
					t_success = p_callback(p_context, t_fs_entry);
				}
			}
		}
		closedir(t_dir);
	}
	MCCStringFree(t_resolved_path);
	
	return t_success;
}

#endif
