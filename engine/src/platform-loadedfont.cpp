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

extern MCPlatformLoadedFontRef MCMacPlatformCreateLoadedFont(MCStringRef p_utf8_path, bool p_globally);

////////////////////////////////////////////////////////////////////////////////

MCPlatformLoadedFont::MCPlatformLoadedFont(MCStringRef p_path, bool p_globally)
: m_path(p_path)
, m_globally(p_globally)
, m_references(1)
{
}

MCPlatformLoadedFont::~MCPlatformLoadedFont(void)
{
}

void MCPlatformLoadedFont::Retain(void)
{
    m_references += 1;
}

void MCPlatformLoadedFont::Release(void)
{
    m_references -= 1;
    if (m_references == 0)
    {
        delete this;
    }
}

bool MCPlatformLoadFont(MCStringRef p_utf8_path, bool p_globally, MCPlatformLoadedFontRef& r_loaded_font)
{
    MCPlatformLoadedFontRef t_font =
        MCMacPlatformCreateLoadedFont(p_utf8_path, p_globally);
    
    r_loaded_font = t_font;
    
    return t_font != nullptr;
}

bool MCPlatformUnloadFont(MCPlatformLoadedFontRef p_loaded_font)
{
    p_loaded_font->Release();
    
    return true;
}

////////////////////////////////////////////////////////////////////////////////
