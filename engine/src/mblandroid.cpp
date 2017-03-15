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

#include <android/log.h>
#include <dlfcn.h>

#include "prefix.h"

#include "system.h"
#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"
#include "globals.h"
#include "uidc.h"

#include "mblandroid.h"
#include "variable.h"

#include "socket.h"

////////////////////////////////////////////////////////////////////////////////

extern bool MCSystemLaunchUrl(MCStringRef p_url);
extern bool MCAndroidResolveLibraryPath(MCStringRef p_library, MCStringRef &r_path);

////////////////////////////////////////////////////////////////////////////////

uint1 *MClowercasingtable;
uint1 *MCuppercasingtable;

////////////////////////////////////////////////////////////////////////////////

bool MCAndroidSystem::Initialize(void)
{
    IO_stdin = MCsystem -> OpenFd(0, kMCOpenFileModeRead);
    IO_stdout = MCsystem -> OpenFd(1, kMCOpenFileModeWrite);
    IO_stderr = MCsystem -> OpenFd(2, kMCOpenFileModeWrite);
    
    // Initialize our case mapping tables
    
    MCuppercasingtable = new (nothrow) uint1[256];
    for(uint4 i = 0; i < 256; ++i)
        MCuppercasingtable[i] = (uint1)toupper((uint1)i);
    
    MClowercasingtable = new (nothrow) uint1[256];
    for(uint4 i = 0; i < 256; ++i)
        MClowercasingtable[i] = (uint1)tolower((uint1)i);
    
	return true;
}

void MCAndroidSystem::Finalize(void)
{
}

////////////////////////////////////////////////////////////////////////////////

MCServiceInterface *MCAndroidSystem::QueryService(MCServiceType type)
{
    return nil;
}

////////////////////////////////////////////////////////////////////////////////

void MCAndroidSystem::Debug(MCStringRef p_string)
{
    MCAutoStringRefAsUTF8String t_utf8_string;
    /* UNCHECKED */ t_utf8_string . Lock(p_string);
	__android_log_print(ANDROID_LOG_INFO, "LiveCode", "%s", *t_utf8_string);
}

////////////////////////////////////////////////////////////////////////////////

MCSystemInterface *MCMobileCreateAndroidSystem(void)
{
	return new MCAndroidSystem;
}

////////////////////////////////////////////////////////////////////////////////

Boolean MCAndroidSystem::GetDevices(MCStringRef& r_devices)
{
    return False;
}

Boolean MCAndroidSystem::GetDrives(MCStringRef& r_drives)
{
    return False;
}

void MCAndroidSystem::CheckProcesses(void)
{
    return;
}

bool MCAndroidSystem::StartProcess(MCNameRef p_name, MCStringRef p_doc, intenum_t p_mode, Boolean p_elevated)
{
    return false;
}

void MCAndroidSystem::CloseProcess(uint2 p_index)
{
    return;
}

void MCAndroidSystem::Kill(int4 p_pid, int4 p_sig)
{
    return;
}

void MCAndroidSystem::KillAll(void)
{
    return;
}

Boolean MCAndroidSystem::Poll(real8 p_delay, int p_fd)
{
    return False;
}

Boolean MCAndroidSystem::IsInteractiveConsole(int p_fd)
{
    return False;
}

void MCAndroidSystem::LaunchDocument(MCStringRef p_document)
{
    return;
}

void MCAndroidSystem::LaunchUrl(MCStringRef p_url)
{
    // AL-2014-06-26: [[ Bug 12700 ]] Implement launch url
	if (!MCSystemLaunchUrl(p_url))
        MCresult -> sets("no association");
}

void MCAndroidSystem::DoAlternateLanguage(MCStringRef p_script, MCStringRef p_language)
{
    return;
}

bool MCAndroidSystem::AlternateLanguages(MCListRef& r_list)
{
    return false;
}

bool MCAndroidSystem::GetDNSservers(MCListRef& r_list)
{
    return false;
}

void MCAndroidSystem::ShowMessageDialog(MCStringRef p_title,
                                        MCStringRef p_message)
{
    if (MCscreen == nil)
        return;
    
    MCscreen -> popupanswerdialog(nil, 0, 0, p_title, p_message, true);
}

////////////////////////////////////////////////////////////////////////////////
