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
#include "platform-internal.h"
#include "mac-extern.h"
#include "globals.h"

////////////////////////////////////////////////////////////////////////////////

bool MCPlatformLoadFont(MCStringRef p_utf8_path, bool p_globally, MCPlatformLoadedFontRef& r_loaded_font)
{
    MCPlatformLoadedFontRef t_font = MCplatform -> CreateLoadedFont();
    if (!t_font)
        return false;
    if (!t_font->CreateWithPath(p_utf8_path, p_globally))
    {
        t_font -> Release();
        return false;
    }
    r_loaded_font = t_font;
    return true;
}

bool MCPlatformUnloadFont(MCPlatformLoadedFontRef p_loaded_font)
{
    p_loaded_font->Release();
    
    return true;
}

////////////////////////////////////////////////////////////////////////////////
