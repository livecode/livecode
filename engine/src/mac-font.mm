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

// TD-2013-07-01 [[ DynamicFonts ]]
// MM-2014-06-12: [[ CoreText ]] Updated to use core text routinees.
bool MCPlatformLoadFont(MCStringRef p_utf8_path, bool p_globally, MCPlatformLoadedFontRef& r_loaded_font)
{
    if (coretext_font_load_from_path(p_utf8_path, p_globally))
    {
        r_loaded_font = NULL;
        return true;
    } else
        return false;
}

// MM-2014-06-12: [[ CoreText ]] Updated to use core text routinees.
bool MCPlatformUnloadFont(MCStringRef utf8path, bool globally, MCPlatformLoadedFontRef p_loaded_font)
{
    return coretext_font_unload(utf8path, globally);
}

////////////////////////////////////////////////////////////////////////////////
