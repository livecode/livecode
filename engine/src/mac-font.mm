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

#include <Cocoa/Cocoa.h>

#include "globdefs.h"

#include "platform.h"

#include "mac-internal.h"

bool coretext_font_load_from_path(MCStringRef p_path, bool p_globally);
bool coretext_font_unload(MCStringRef p_path, bool p_globally);

////////////////////////////////////////////////////////////////////////////////

MCMacPlatformLoadedFont::~MCMacPlatformLoadedFont(void)
{
    coretext_font_unload(*m_path, m_globally);
}


bool
MCMacPlatformLoadedFont::CreateWithPath(MCStringRef p_path, bool p_globally)
{
    m_path.Reset(p_path);
    m_globally = p_globally;
    coretext_font_load_from_path(*m_path, m_globally);
    return true;
}
////////////////////////////////////////////////////////////////////////////////

MCPlatformLoadedFontRef MCMacPlatformCore::CreateLoadedFont()
{
    MCPlatform::Ref<MCPlatformLoadedFont> t_ref = MCPlatform::makeRef<MCMacPlatformLoadedFont>();
    t_ref -> SetPlatform(this);
    
    return t_ref.unsafeTake();
}
