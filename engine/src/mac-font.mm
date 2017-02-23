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
#include "platform-internal.h"

#include "mac-internal.h"

bool coretext_font_load_from_path(MCStringRef p_path, bool p_globally);
bool coretext_font_unload(MCStringRef p_path, bool p_globally);

////////////////////////////////////////////////////////////////////////////////

MCMacPlatformLoadedFont::MCMacPlatformLoadedFont(MCStringRef p_path, bool p_globally)
: MCPlatformLoadedFont(p_path,p_globally)
{
    coretext_font_load_from_path(p_path, p_globally);
}

MCMacPlatformLoadedFont::~MCMacPlatformLoadedFont(void)
{
    coretext_font_unload(m_path, m_globally);
}

////////////////////////////////////////////////////////////////////////////////

MCPlatformLoadedFontRef MCMacPlatformCreateLoadedFont(MCStringRef p_path, bool p_globally)
{
    return new (nothrow) MCMacPlatformLoadedFont(p_path, p_globally);
}
