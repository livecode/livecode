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
#include "em-platform.h"
#include "globals.h"
#include "em-clipboard.h"

bool MCEmscriptenPlatformCore::WaitForEvent(double p_duration, bool p_blocking)
{
    return SystemWaitForEvent(p_duration, p_blocking);
}

void MCEmscriptenPlatformCore::BreakWait(void)
{
    SystemBreakWait();
}

// Platform extensions
bool MCEmscriptenPlatformCore::QueryInterface(const char * p_interface_id, MCPlatform::Base *&r_interface)
{
    return false;
}

////////////////////////////////////////////////////////////////////////////////

MCRawClipboard* MCEmscriptenPlatformCore::CreateSystemClipboard()
{
    return new MCEmscriptenRawClipboard;
}

MCRawClipboard* MCEmscriptenPlatformCore::CreateSystemSelectionClipboard()
{
    return new MCEmscriptenRawClipboard;
}

MCRawClipboard* MCEmscriptenPlatformCore::CreateSystemDragboard()
{
    return new MCEmscriptenRawClipboard;
}

////////////////////////////////////////////////////////////////////////////////

MCPlatform::Ref<MCPlatformCore> MCPlatformCreateCore(MCPlatformCallbackRef p_callback)
{
    return MCPlatform::makeRef<MCEmscriptenPlatformCore>(p_callback);
}
////////////////////////////////////////////////////////////////////////////////
