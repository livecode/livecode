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

#ifndef __MC_LINUX_PLATFORM__
#define __MC_LINUX_PLATFORM__

#include "platform.h"

////////////////////////////////////////////////////////////////////////////////

class MCLinuxPlatformCore: public MCPlatformCore
{
public:
    constexpr MCLinuxPlatformCore(void) = default;
    virtual ~MCLinuxPlatformCore(void) {}
    
    // Wait
    virtual bool WaitForEvent(double p_duration, bool p_blocking);
    virtual void BreakWait(void);
    
    // Player
    virtual MCPlatformPlayerRef CreatePlayer(void) { return nil;}
    
    // Theme
    virtual bool GetControlThemePropBool(MCPlatformControlType p_type, MCPlatformControlPart p_part, MCPlatformControlState p_state, MCPlatformThemeProperty p_which, bool& r_bool);
    virtual bool GetControlThemePropInteger(MCPlatformControlType p_type, MCPlatformControlPart p_part, MCPlatformControlState p_state, MCPlatformThemeProperty p_which, int& r_int);
    virtual bool GetControlThemePropColor(MCPlatformControlType p_type, MCPlatformControlPart p_part, MCPlatformControlState p_state, MCPlatformThemeProperty p_which, MCColor& r_color);
    virtual bool GetControlThemePropFont(MCPlatformControlType p_type, MCPlatformControlPart p_part, MCPlatformControlState p_state, MCPlatformThemeProperty p_which, MCFontRef& r_font);
    virtual bool GetControlThemePropString(MCPlatformControlType p_type, MCPlatformControlPart p_part, MCPlatformControlState p_state, MCPlatformThemeProperty p_which, MCStringRef& r_string);
    
    // Platform extensions
    virtual bool QueryInterface(const char * p_interface_id, MCPlatform::Base *&r_interface);
};

#endif
