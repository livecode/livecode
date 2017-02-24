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

////////////////////////////////////////////////////////////////////////////////

MCPlatformLoadedFont::LoadedFont(MCStringRef p_path, bool p_globally)
: m_path(p_path)
, m_globally(p_globally)
{
}

MCPlatformLoadedFont::~LoadedFont(void)
{
}

bool MCPlatformLoadFont(MCStringRef p_utf8_path, bool p_globally, MCPlatformLoadedFontRef& r_loaded_font)
{
    MCPlatform::LoadedFontRef t_font = MCMacPlatformCreateLoadedFont(p_utf8_path, p_globally);
    if (!t_font)
        return false;
    r_loaded_font = t_font.unsafeTake();
    return true;
}

bool MCPlatformUnloadFont(MCPlatformLoadedFontRef p_loaded_font)
{
    p_loaded_font->Release();
    
    return true;
}

////////////////////////////////////////////////////////////////////////////////
