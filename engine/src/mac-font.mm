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

#include "mac-platform.h"

#import <ApplicationServices/ApplicationServices.h>

////////////////////////////////////////////////////////////////////////////////

MCMacPlatformLoadedFont::~MCMacPlatformLoadedFont(void)
{
    bool t_success;
    t_success = true;
    
    MCPlatformAutoStringRefAsCFString t_cf_path(m_callback);
    t_success = t_cf_path . Lock(*m_path);
    
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
        if (m_globally)
            t_scope = kCTFontManagerScopeUser;
        else
            t_scope = kCTFontManagerScopeProcess;
        t_success = CTFontManagerUnregisterFontsForURL(t_font_url, t_scope, NULL);
    }
    
    if (t_font_url != NULL)
        CFRelease(t_font_url);
}


bool
MCMacPlatformLoadedFont::CreateWithPath(MCStringRef p_path, bool p_globally)
{
    m_path.SetCallback(m_callback);
    m_path.Reset(p_path);
    m_globally = p_globally;
    
    bool t_success;
    t_success = true;
    
    MCPlatformAutoStringRefAsCFString t_cf_path(m_callback);
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
////////////////////////////////////////////////////////////////////////////////

MCPlatformLoadedFontRef MCMacPlatformCore::CreateLoadedFont()
{
    MCPlatform::Ref<MCPlatformLoadedFont> t_ref = MCPlatform::makeRef<MCMacPlatformLoadedFont>(this);
    
    return t_ref.unsafeTake();
}
