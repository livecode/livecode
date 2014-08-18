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

#include <include/cef_app.h>

#include "core.h"

#include <AppKit/AppKit.h>

#include "cefshared.h"

////////////////////////////////////////////////////////////////////////////////

extern bool MCCefCreateApp(CefRefPtr<CefApp> &r_app);

////////////////////////////////////////////////////////////////////////////////

extern "C" int initialise_weak_link_cef(void);
extern "C" int initialise_weak_link_cef_with_path(const char *p_path);

// IM-2014-08-12: [[ LibCef ]] cef Framework folder located in same folder as helper app.
const char *MCCefPlatformGetCefFrameworkFolder()
{
	static char *s_path = nil;
	
	if (s_path == nil)
	{
		NSBundle *t_bundle;
		t_bundle = [NSBundle mainBundle];
		
		NSString *t_cef_path;
		t_cef_path = [[[ t_bundle bundlePath] stringByDeletingLastPathComponent] stringByAppendingPathComponent:@"Chromium Embedded Framework.framework"];
		
		/* UNCHECKED */ MCCStringClone([t_cef_path cStringUsingEncoding:NSUTF8StringEncoding], s_path);
	}
	
	return s_path;
}

int main(int argc, char *argv[])
{
	const char *t_lib_path;
	t_lib_path = MCCefPlatformGetCefLibraryPath();
	
	// IM-2014-03-19: [[ revBrowserCEF ]] Initialise dynamically loaded cef library
	if (!initialise_weak_link_cef_with_path(t_lib_path) && !initialise_weak_link_cef())
	{
		printf("failed to load libcef library: %s", t_lib_path);
		return -1;
	}
	
	CefMainArgs t_args(argc, argv);
	
	CefRefPtr<CefApp> t_app;
	if (!MCCefCreateApp(t_app))
		return -1;
	
	return CefExecuteProcess(t_args, t_app, nil);
}

////////////////////////////////////////////////////////////////////////////////
