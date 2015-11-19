/* Copyright (C) 2015 LiveCode Ltd.
 
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
#include "libbrowser_cef.h"

////////////////////////////////////////////////////////////////////////////////

extern bool MCCefCreateApp(CefRefPtr<CefApp> &r_app);

////////////////////////////////////////////////////////////////////////////////

extern "C" int initialise_weak_link_cef(void);
extern "C" int initialise_weak_link_cef_with_path(const char *p_path);

int main(int argc, char *argv[])
{
	const char *t_lib_path;
	t_lib_path = MCCefPlatformGetCefLibraryPath();
	
	// IM-2014-03-18: [[ revBrowserCEF ]] Initialise dynamically loaded cef library
	if (t_lib_path != nil)
	{
		MCLog("lib path: %s", t_lib_path);
		if (!initialise_weak_link_cef_with_path(t_lib_path))
			return -1;
	}
	else
	{
		if (!initialise_weak_link_cef())
			return -1;
	}

	MCLog("constructing args", nil);
	CefMainArgs t_args(argc, argv);
	
	MCLog("creating app", nil);
	CefRefPtr<CefApp> t_app;
	if (!MCCefCreateApp(t_app))
		return -1;
	
	MCLog("executing", nil);
	return CefExecuteProcess(t_args, t_app, NULL);
}

////////////////////////////////////////////////////////////////////////////////

#define CEF_PATH_PREFIX "Externals/CEF"

const char *MCCefPlatformGetExecutableFolder(void);
bool MCCefLinuxAppendPath(const char *p_base, const char *p_path, char *&r_path);

const char *MCCefPlatformGetCefFolder(void)
{
	static char *s_cef_path = nil;

	if (s_cef_path == nil)
		/* UNCHECKED */ MCCefLinuxAppendPath(MCCefPlatformGetExecutableFolder(), CEF_PATH_PREFIX, s_cef_path);
	
	MCLog("libbrowser-cefprocess cef folder: %s", s_cef_path);
	
	return s_cef_path;
}

////////////////////////////////////////////////////////////////////////////////

// SN-2015-02-25: [[ Merge 7.0.4-rc-1 ]] Added MCU_*module* functions for Linux
//  since CEF browser is implemented in LC 8

// AL-2015-02-17: [[ SB Inclusions ]] Work around problems linking to MCU_ functions from CEF
#include <stdlib.h>
#include <stdio.h>
#include <cstring>
#include <dlfcn.h>

void *MCU_loadmodule(const char *p_source)
{
    return dlopen(p_source, (RTLD_NOW | RTLD_LOCAL));
}

void MCU_unloadmodule(void *p_module)
{

}

void *MCU_resolvemodulesymbol(void *p_module, const char *p_name)
{
    return dlsym(p_module, p_name);
}

