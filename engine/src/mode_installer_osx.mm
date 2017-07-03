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

#include "foundation.h"
#include "osxprefix.h"

#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"
#include "osspec.h"

#include <sys/stat.h>

#import <AppKit/AppKit.h>

////////////////////////////////////////////////////////////////////////////////

typedef bool (*MCSystemListProcessesCallback)(void *context, uint32_t id, MCStringRef path, MCStringRef description);
typedef bool (*MCSystemListProcessModulesCallback)(void *context, MCStringRef path);

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

// There is no registry on OSX, so this is irrelevant.
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
	MCS_resolvepath(p_file, &t_resolved_file_str);
    
    uindex_t t_last_slash;
    if (!MCStringLastIndexOfChar(*t_resolved_file_str, '/',
                                    UINDEX_MAX,
                                    kMCStringOptionCompareExact,
                                    t_last_slash))
        return false;
    
    MCAutoStringRef t_folder;
    if (!MCStringCopySubstring(*t_resolved_file_str,
                               MCRangeMake(0, t_last_slash), &t_folder))
        return false;
    
    MCAutoStringRefAsUTF8String t_folder_cstring;
    /* UNCHECKED */ t_folder_cstring.Lock(*t_folder);
    
	struct stat t_stat;
	if (stat(*t_folder_cstring, &t_stat) != 0)
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

// MM-2011-03-16: Make the installer doc icon bounce.
static NSInteger s_attention_request_id;

void MCSystemCancelRequestUserAttention(void)
{
    if (s_attention_request_id != 0)
        [NSApp cancelUserAttentionRequest: s_attention_request_id];
    s_attention_request_id = 0;
}

void MCSystemRequestUserAttention(void)
{
    if (s_attention_request_id != 0)
		MCSystemCancelRequestUserAttention();
	
    // We request attention as "critical" rather than "informational" so that
    // the dock icon will bounce until the application is activated.
    s_attention_request_id = [NSApp requestUserAttention: NSCriticalRequest];
}

// MM-2011-04-04: Added prototype.
void MCSystemBalloonNotification(MCStringRef p_title, MCStringRef p_message)
{
}

////////////////////////////////////////////////////////////////////////////////
