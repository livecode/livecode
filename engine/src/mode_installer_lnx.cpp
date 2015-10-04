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

#include "foundation-legacy.h"

#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"
#include "osspec.h"
#include "util.h"

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>

////////////////////////////////////////////////////////////////////////////////

typedef bool (*MCSystemListProcessesCallback)(void *context, uint32_t id, MCStringRef path, MCStringRef description);
typedef bool (*MCSystemListProcessModulesCallback)(void *context, MCStringRef path);

////////////////////////////////////////////////////////////////////////////////

// This call is primarily designed to be used by the plugin installer. As this
// is generally installed 'per-user', we simply scan '/proc' for processes owned
// by the calling user.
bool MCSystemListProcesses(MCSystemListProcessesCallback p_callback, void* p_context)
{
	bool t_success;
	t_success = true;

	DIR *t_dir;
	t_dir = nil;
	if (t_success)
	{
		t_dir = opendir("/proc");
		if (t_dir == nil)
			t_success = false;
	}

	if (t_success)
	{
		dirent *t_entry;
		while(t_success)
		{
			// Fetch the next entry
			t_entry = readdir(t_dir);
			if (t_entry == nil)
				break;

			// Work out if the entry is a process id
			int32_t t_pid;
            MCAutoStringRef t_dname;
            /* UNCHECKED */ MCStringCreateWithCString(t_entry -> d_name, &t_dname);
			if (!MCU_strtol(*t_dname, t_pid))
				continue;

			// Work out the full path ("/proc/<int>") and stat so we can
			// check ownership.
			char t_path[6 + I4L + 1];
			struct stat64 t_stat;
			sprintf(t_path, "/proc/%u", t_pid);
			stat64(t_path, &t_stat);
			if (t_stat . st_uid != getuid())
				continue;

			// We have a viable process to report. First fetch its path
            MCAutoStringRef t_exe_link, t_exe_path;
            /* UNCHECKED */ MCStringFormat(&t_exe_link, "/proc/%u/exe", t_pid);
			if (!MCS_resolvepath(*t_exe_link, &t_exe_path))
			{
				t_success = false;
				break;
			}

			// Next fetch its 'description' from the first line of the status
			// file.
			char t_status_file[6 + I4L + 7 + 1];
			char t_status[256];
			FILE *t_stream;
			sprintf(t_status_file, "/proc/%u/status", t_pid);
			t_stream = fopen(t_status_file, "r");
			if (t_stream != nil)
			{
				if (fgets(t_status, 256, t_stream) != nil)
				{
					char *t_tab;
					t_tab = strchr(t_status, '\t');
					if (t_tab != nil)
						MCMemoryMove(t_status, t_tab + 1, MCCStringLength(t_tab + 1));
				}
				else
					t_status[0] = '\0';
				fclose(t_stream);
			}
			else
				t_status[0] = '\0';

            MCAutoStringRef t_status_str;
            /* UNCHECKED */ MCStringCreateWithSysString(t_status, &t_status_str);
			t_success = p_callback(p_context, t_pid, *t_exe_path, *t_status_str);
		}
	}

	if (t_dir != nil)
		closedir(t_dir);

	return t_success;
}

////////////////////////////////////////////////////////////////////////////////

// We extract the list of open modules for a process by parsing out the
//   /proc/<pid>/maps
// File. This lists all the files mapped into the processes memory space.
bool MCSystemListProcessModules(uint32_t p_process_id, MCSystemListProcessModulesCallback p_callback, void *p_context)
{
	char t_maps_file[6 + I4L + 5 + 1];
	sprintf(t_maps_file, "/proc/%u/maps", p_process_id);

	FILE *t_stream;
	t_stream = fopen(t_maps_file, "r");
	if (t_stream == nil)
		return false;

	bool t_success;
	t_success = true;

	// Each line is of a fixed format, with any module path appearing at '/'.
	// Modules are repeated in the map for each segment that is mapped in. Thus
	// we only report the first appearance, and assume that they are always
	// consecutive.
	char t_last_module[4096];
	t_last_module[0] = '\0';
	while(t_success)
	{
		char t_line[4096];
		if (fgets(t_line, 4096, t_stream) == nil)
			break;

		// See if there is a module path
		char *t_path;
		t_path = strchr(t_line, '/');
		if (t_path == nil)
			continue;

		// See if it is a shared library (terminated by .so, or containing .so.)
		if (!MCCStringEndsWith(t_path, ".so") ||
			!MCCStringContains(t_path, ".so.") ||
			MCCStringEqual(t_path, t_last_module))
			continue;

		MCAutoStringRef t_path_str;
        /* UNCHECKED */ MCStringCreateWithSysString(t_path, &t_path_str);

		t_success = p_callback(p_context, *t_path_str);
	}

	fclose(t_stream);

	return t_success;
}
	
////////////////////////////////////////////////////////////////////////////////

// There is no registry on linux, so this is irrelevant.
bool MCSystemCanDeleteKey(MCStringRef p_key)
{
	return false;
}


// This call simply checks to see if this user has 'write' privilege on the
// folder containing the file as this is what determines whether a file can
// be deleted.
bool MCSystemCanDeleteFile(MCStringRef p_file)
{
    MCAutoStringRef t_resolved_file_str;

    if (!MCS_resolvepath(p_file, &t_resolved_file_str))
        return false;

    uindex_t t_last_delimiter;

    if (!MCStringLastIndexOfChar(*t_resolved_file_str, '/', UINDEX_MAX, kMCStringOptionCompareExact, t_last_delimiter))
        return false;

    // Now get the folder.
    MCAutoStringRef t_folder_path;
    MCAutoStringRefAsSysString t_folder_sys_str;

    if (!MCStringCopySubstring(*t_resolved_file_str, MCRangeMake(0, t_last_delimiter), &t_folder_path)
            || !t_folder_sys_str . Lock(*t_folder_path))
        return false;
	
	struct stat64 t_stat;
    if (stat64(*t_folder_sys_str, &t_stat) != 0)
		return false;
	
	// Check for user 'write' bit.
	if (t_stat . st_uid == getuid() && (t_stat . st_mode & S_IWUSR) != 0)
		return true;
	
	// Check for same group and group 'write' bit
	if (t_stat . st_gid == getgid() && (t_stat . st_mode & S_IWGRP) != 0)
		return true;
	
	// Check for other
	if ((t_stat . st_mode & S_IWOTH) != 0)
		return true;

	return false;
}

////////////////////////////////////////////////////////////////////////////////

// MM-2011-03-16: Added prototype but not yet implemented.
void MCSystemRequestUserAttention(void)
{
}

void MCSystemCancelRequestUserAttention(void)
{
}

// MM-2011-03-25: Added prototype.
void MCSystemBalloonNotification(MCStringRef p_title, MCStringRef p_message)
{
}

////////////////////////////////////////////////////////////////////////////////
