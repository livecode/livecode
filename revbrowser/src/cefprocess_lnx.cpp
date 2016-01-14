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

#include <include/cef_app.h>

#include <sys/types.h>

#include "core.h"
#include "cefshared.h"

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

const char *MCCefPlatformGetExecutableFolder(void);
bool MCCefLinuxAppendPath(const char *p_base, const char *p_path, char *&r_path);
const char *MCCefPlatformGetCefFolder(void)
{
	static char *s_cef_path = nil;

	if (s_cef_path == nil)
		/* UNCHECKED */ MCCefLinuxAppendPath(MCCefPlatformGetExecutableFolder(), "Externals/CEF", s_cef_path);

	return s_cef_path;
}

#if 0
const char *MCCefPlatformGetCefFolder(void)
{
	static char *s_cef_path = nil;

	if (s_cef_path == nil)
	{
		bool t_success;
		t_success = true;

		Dl_info t_info;
		if (t_success)
			t_success = 0 != dladdr((const void*)MCCefPlatformGetCefFolder, &t_info);
			
		if (t_success)
			t_success = MCCStringClone(t_info.dli_fname, s_cef_path);

		if (t_success)
		{
			// remove library component from path
			uint32_t t_index;
			if (MCCStringLastIndexOf(s_cef_path, '/', t_index))
				s_cef_path[t_index] = '\0';
		}
	}

	return s_cef_path;
}
#endif

////////////////////////////////////////////////////////////////////////////////
