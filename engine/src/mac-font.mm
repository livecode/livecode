/* Copyright (C) 2003-2013 Runtime Revolution Ltd.
 
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

#include "core.h"
#include "globdefs.h"

#include "platform.h"
#include "platform-internal.h"

#include "mac-internal.h"

bool coretext_font_load_from_path(const char *p_path, bool p_globally, void *&r_loaded_font);
bool coretext_font_unload(const char *p_path, bool p_globally, void *p_loaded_font);


////////////////////////////////////////////////////////////////////////////////

// TD-2013-07-01 [[ DynamicFonts ]]
// MM-2014-06-12: [[ CoreText ]] Updated to use core text routinees.
bool MCPlatformLoadFont(const char *p_utf8_path, bool p_globally, MCPlatformLoadedFontRef& r_loaded_font)
{
	void *t_loaded_font;
    if (coretext_font_load_from_path(p_utf8_path, p_globally, t_loaded_font))
    {
        r_loaded_font = (MCPlatformLoadedFontRef)t_loaded_font;
        return true;
    } else
        return false;
}

// MM-2014-06-12: [[ CoreText ]] Updated to use core text routinees.
bool MCPlatformUnloadFont(const char *utf8path, bool globally, MCPlatformLoadedFontRef p_loaded_font)
{
    return coretext_font_unload(utf8path, globally, (void *)p_loaded_font);
}

////////////////////////////////////////////////////////////////////////////////
