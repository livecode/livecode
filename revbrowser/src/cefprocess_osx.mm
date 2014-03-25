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

////////////////////////////////////////////////////////////////////////////////

extern bool MCCefCreateApp(CefRefPtr<CefApp> &r_app);

////////////////////////////////////////////////////////////////////////////////

extern "C" int initialise_weak_link_cef(void);
extern "C" int initialise_weak_link_cef_with_path(const char *p_path);

int main(int argc, char *argv[])
{
	// IM-2014-03-21: [[ revBrowserCEF ]] Look for libcef.dylib library in containing bundle
	// IM-2014-03-25: [[ revBrowserCEF ]] cef library located in same folder as this app bundle
	NSAutoreleasePool *t_pool;
	t_pool = [[NSAutoreleasePool alloc] init];
	
	NSString *t_bundle_path;
	t_bundle_path = [[NSBundle mainBundle] bundlePath];
	
	// Split path into components
	NSMutableArray *t_components;
	t_components = [[t_bundle_path pathComponents] mutableCopy];

	// Remove "revbrowser-cefprocess.app" path component
	[t_components removeLastObject];
	
	// Add library name path component
	[t_components addObject: @"libcef.dylib"];
	
	// Rebuild path
	NSString *t_lib_path;
	t_lib_path = [NSString pathWithComponents:t_components];
	
	const char *t_c_path;
	t_c_path = [t_lib_path cStringUsingEncoding:NSUTF8StringEncoding];
	
	// IM-2014-03-19: [[ revBrowserCEF ]] Initialise dynamically loaded cef library
	if (!initialise_weak_link_cef_with_path(t_c_path) && !initialise_weak_link_cef())
	{
		printf("failed to load libcef library: %s", t_c_path);
		return -1;
	}
	[t_pool release];
	
	CefMainArgs t_args(argc, argv);
	
	CefRefPtr<CefApp> t_app;
	if (!MCCefCreateApp(t_app))
		return -1;
	
	return CefExecuteProcess(t_args, t_app);
}

////////////////////////////////////////////////////////////////////////////////
