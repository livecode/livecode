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

////////////////////////////////////////////////////////////////////////////////

// TD-2013-07-01 [[ DynamicFonts ]]
bool MCPlatformLoadFont(const char *p_utf8_path, bool p_globally, MCPlatformLoadedFontRef& r_loaded_font)
{
	FSRef t_ref;	
	if (FSPathMakeRef((const UInt8 *)p_utf8_path, &t_ref, NULL) != noErr)
		return false;
    
    ATSFontContext t_context = kATSFontContextLocal;
    if (p_globally)
        t_context = kATSFontContextGlobal;
    
    ATSFontContainerRef t_container = NULL;
    if (ATSFontActivateFromFileReference(&t_ref, t_context, kATSFontFormatUnspecified, NULL, kATSOptionFlagsDefault, &t_container) != noErr)
		return false;
    
    r_loaded_font = (MCPlatformLoadedFontRef)t_container;
    
    return true;
}

bool MCPlatformUnloadFont(const char *utf8path, bool globally, MCPlatformLoadedFontRef p_loaded_font)
{
	return ATSFontDeactivate((ATSFontContainerRef)p_loaded_font, NULL, kATSOptionFlagsDefault) == noErr;
}

////////////////////////////////////////////////////////////////////////////////
