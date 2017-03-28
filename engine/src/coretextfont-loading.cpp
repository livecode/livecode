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

#include "globdefs.h"

#ifdef TARGET_SUBPLATFORM_IPHONE
#import <CoreText/CoreText.h>
#else
#import <ApplicationServices/ApplicationServices.h>
#endif


bool coretext_font_load_from_path(MCStringRef p_path, bool p_globally)
{
    bool t_success;
    t_success = true;
    
    MCAutoStringRefAsCFString t_cf_path;
    t_success = t_cf_path . Lock(p_path);
    
    CFURLRef t_font_url;
    t_font_url = NULL;
    if (t_success)
    {
        t_font_url = CFURLCreateWithFileSystemPath(NULL, *t_cf_path, kCFURLPOSIXPathStyle, false);
        t_success = t_font_url != NULL;
    }
    
    if (t_success)
    {
        CTFontManagerScope t_scope;
        if (p_globally)
            t_scope = kCTFontManagerScopeUser;
        else
            t_scope = kCTFontManagerScopeProcess;
        t_success = CTFontManagerRegisterFontsForURL(t_font_url, t_scope, NULL);
    }
    
    if (t_font_url != NULL)
        CFRelease(t_font_url);
    
    return t_success;
}

bool coretext_font_unload(MCStringRef p_path, bool p_globally)
{
    bool t_success;
    t_success = true;
    
    MCAutoStringRefAsCFString t_cf_path;
    t_success = t_cf_path . Lock(p_path);
    
    CFURLRef t_font_url;
    t_font_url = NULL;
    if (t_success)
    {
        t_font_url = CFURLCreateWithFileSystemPath(NULL, *t_cf_path, kCFURLPOSIXPathStyle, false);
        t_success = t_font_url != NULL;
    }
    
    if (t_success)
    {
        CTFontManagerScope t_scope;
        if (p_globally)
            t_scope = kCTFontManagerScopeUser;
        else
            t_scope = kCTFontManagerScopeProcess;
        t_success = CTFontManagerUnregisterFontsForURL(t_font_url, t_scope, NULL);
    }
    
    if (t_font_url != NULL)
        CFRelease(t_font_url);
    
    return t_success;
}
