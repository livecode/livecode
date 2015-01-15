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

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

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

bool get_link_size(const char *p_path, uint32_t &r_size)
{
	if (p_path == nil)
		return false;
		
	struct stat t_stat;
	if (lstat(p_path, &t_stat) == -1)
		return false;
	
	r_size = t_stat.st_size;
	return true;
}

bool get_link_path(const char *p_link, char *&r_path)
{
	bool t_success;
	t_success = true;
	
	char *t_buffer;
	t_buffer = nil;
	uint32_t t_buffer_size;
	t_buffer_size = 0;
	
	uint32_t t_link_size;
	t_success = get_link_size(p_link, t_link_size);
	
	while (t_success && t_link_size + 1 > t_buffer_size)
	{
		t_buffer_size = t_link_size + 1;
		t_success = MCMemoryReallocate(t_buffer, t_buffer_size, t_buffer);
		
		if (t_success)
		{
			int32_t t_read;
			t_read = readlink(p_link, t_buffer, t_buffer_size);
			
			t_success = t_read >= 0;
			t_link_size = t_read;
		}
	}
	
	if (t_success)
	{
		t_buffer[t_link_size] = '\0';
		r_path = t_buffer;
	}
	else
		MCMemoryDeallocate(t_buffer);
	
	return t_success;
}

bool get_exe_path_from_proc_fs(char *&r_path)
{
	return get_link_path("/proc/self/exe", r_path);
}

const char *MCCefPlatformGetCefFolder(void)
{
	static char *s_cef_path = nil;

	if (s_cef_path == nil)
	{
		bool t_success;
		t_success = get_exe_path_from_proc_fs(s_cef_path);
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
