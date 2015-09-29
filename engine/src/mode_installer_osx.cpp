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

#include "osxprefix.h"

#include "core.h"

#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"
#include "osspec.h"

#include <sys/stat.h>

////////////////////////////////////////////////////////////////////////////////

typedef bool (*MCSystemListProcessesCallback)(void *context, uint32_t id, const char *path, const char *description);
typedef bool (*MCSystemListProcessModulesCallback)(void *context, const char *path);

////////////////////////////////////////////////////////////////////////////////

bool MCSystemListProcesses(MCSystemListProcessesCallback p_callback, void* p_context)
{
	return false;
}

////////////////////////////////////////////////////////////////////////////////

bool MCSystemListProcessModules(uint32_t p_process_id, MCSystemListProcessModulesCallback p_callback, void *p_context)
{
	return false;
}

////////////////////////////////////////////////////////////////////////////////

// There is no registry on linux, so this is irrelevant.
bool MCSystemCanDeleteKey(const char *p_key)
{
	return false;
}

// This call simply checks to see if this user has 'write' privilege on the
// folder containing the file as this is what determines whether a file can
// be deleted.
bool MCSystemCanDeleteFile(const char *p_file)
{
	char *t_resolved_file;
	t_resolved_file = MCS_resolvepath(p_file);
	
	// Now get the folder.
	if (strrchr(t_resolved_file, '/') == nil)
		return false;
	
	strrchr(t_resolved_file, '/')[0] = '\0';
	
	struct stat t_stat;
	if (stat(t_resolved_file, &t_stat) != 0)
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
	
	delete t_resolved_file;
	
	return false;
}

////////////////////////////////////////////////////////////////////////////////

// MM-2011-03-16: Make the installer doc icon bounce.

static NMRecPtr s_bounce_nmr = nil;

void MCSystemCancelRequestUserAttention(void)
{
	if (s_bounce_nmr != nil)
	{
		NMRemove(s_bounce_nmr);
		MCMemoryDelete(s_bounce_nmr);
		s_bounce_nmr = nil;
	}
}

void MCSystemRequestUserAttention(void)
{
	if (s_bounce_nmr != nil)
		MCSystemCancelRequestUserAttention();
	
	bool t_success;
	t_success = true;
	
	if (t_success)
		t_success = MCMemoryNew(s_bounce_nmr);
	
	if (t_success)
	{
		s_bounce_nmr -> nmMark = 1;
		s_bounce_nmr -> qType = nmType;
		if (NMInstall(s_bounce_nmr) != noErr)
			t_success = false;
	}
	
	if (!t_success)
	{
		MCMemoryDelete(s_bounce_nmr);
		s_bounce_nmr = nil;
	}
}

// MM-2011-04-04: Added prototype.
void MCSystemBalloonNotification(const char *p_title, const char *p_message)
{
}

////////////////////////////////////////////////////////////////////////////////
