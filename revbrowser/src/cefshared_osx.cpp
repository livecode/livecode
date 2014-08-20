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

#include "core.h"

#include "cefshared.h"

const char *MCCefPlatformGetCefFrameworkFolder();

// IM-2014-08-12: [[ LibCef ]] libcef library file located in the CEF framework folder
const char *MCCefPlatformGetCefLibraryPath(void)
{
	static char *s_lib_path = nil;
	
	if (s_lib_path == nil)
	{
		const char *t_cef_folder;
		t_cef_folder = MCCefPlatformGetCefFrameworkFolder();
		
		if (t_cef_folder != nil)
		/* UNCHECKED */ MCCStringFormat(s_lib_path, "%s/%s", t_cef_folder, "Chromium Embedded Framework");
	}
	
	return s_lib_path;
}

// IM-2014-03-25: [[ revBrowserCEF ]] Can't change the locale path on OSX so return nil
const char *MCCefPlatformGetLocalePath(void)
{
	return nil;
}

// IM-2014-08-12: [[ LibCef ]] Using standard resource path so return nil here
const char *MCCefPlatformGetResourcesDirPath(void)
{
	return nil;
}
