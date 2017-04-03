/* Copyright (C) 2003-2017 LiveCode Ltd.
 
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

#include "platform.h"
#include "windows-platform.h"
#include "globals.h"

bool MCWindowsPlatformCore::WaitForEvent(double p_duration, bool p_blocking)
{
    return MCscreen->wait(p_duration, p_blocking ? False : True, True);
}

void MCWindowsPlatformCore::BreakWait(void)
{
    MCscreen->pingwait();
}

// Platform extensions
bool MCWindowsPlatformCore::QueryInterface(const char * p_interface_id, MCPlatform::Base *&r_interface)
{
    return false;
}

////////////////////////////////////////////////////////////////////////////////

MCRawClipboard* MCWindowsPlatformCore::CreateSystemClipboard()
{
    return new MCWin32RawClipboard;
}

MCRawClipboard* MCWindowsPlatformCore::CreateSystemSelectionClipboard()
{
    return new MCWin32RawClipboardNull;
}

MCRawClipboard* MCWindowsPlatformCore::CreateSystemDragboard()
{
    return new MCWin32RawClipboardNull;
}

////////////////////////////////////////////////////////////////////////////////
