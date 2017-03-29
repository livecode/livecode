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
#include "linux-platform.h"
#include "globals.h"

bool MCLinuxPlatformCore::WaitForEvent(double p_duration, bool p_blocking)
{
    return MCscreen->wait(p_duration, p_blocking ? False : True, True);
}

void MCLinuxPlatformCore::BreakWait(void)
{
    MCscreen->pingwait();
}

// Platform extensions
bool MCLinuxPlatformCore::QueryInterface(const char * p_interface_id, MCPlatform::Base *&r_interface)
{
    return false;
}

////////////////////////////////////////////////////////////////////////////////

MCPlatform::Ref<MCPlatformCore> MCPlatformCreateCore(MCPlatformCallbackRef p_callback)
{
    return MCPlatform::makeRef<MCLinuxPlatformCore>(p_callback);
}

////////////////////////////////////////////////////////////////////////////////
