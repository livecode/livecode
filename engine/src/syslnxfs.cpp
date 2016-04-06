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

#include "foundation-legacy.h"

//#include <sys/syslimits.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <dirent.h>

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
			(p_folder && S_ISDIR(t_stat . st_mode)) ||
			(!p_folder && !S_ISDIR(t_stat . st_mode));
	}
	else
		r_exists = false;

	MCCStringFree(t_resolved_path);

	return true;
}

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
					MCStringCreateWithSysString(t_entry->d_name, t_fs_entry.filename);
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
