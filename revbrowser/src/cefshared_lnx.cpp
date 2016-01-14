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

#include "core.h"

#include "cefshared.h"

#include <sys/stat.h>
#include <unistd.h>

const char *MCCefPlatformGetCefFolder();
const char *MCCefPlatformGetExecutableFolder(void);

bool MCCefLinuxAppendPath(const char *p_base, const char *p_path, char *&r_path)
{
	if (p_base == nil)
		return MCCStringClone(p_path, r_path);
	else if (MCCStringEndsWith(p_base, "/"))
		return MCCStringFormat(r_path, "%s%s", p_base, p_path);
	else
		return MCCStringFormat(r_path, "%s/%s", p_base, p_path);
}

const char *MCCefPlatformGetResourcesDirPath(void)
{
    return MCCefPlatformGetCefFolder();
}

// IM-2014-10-03: [[ revBrowserCEF ]] locales located in CEF subfolder relative to revbrowser external
const char *MCCefPlatformGetLocalePath(void)
{
	static char *s_locale_path = nil;

	if (s_locale_path == nil)
		/* UNCHECKED */ MCCefLinuxAppendPath(MCCefPlatformGetCefFolder(), "locales", s_locale_path);

	return s_locale_path;
}

// IM-2015-10-16: [[ BrowserWidget ]] relocate subprocess exe next to application exe so hardcoded "icudtl.dat" file can be shared.
const char *MCCefPlatformGetSubProcessName(void)
{
	static char *s_exe_path = nil;

	if (s_exe_path == nil)
		/* UNCHECKED */ MCCefLinuxAppendPath(MCCefPlatformGetExecutableFolder(), "revbrowser-cefprocess", s_exe_path);

	return s_exe_path;
}

// IM-2014-10-03: [[ revBrowserCEF ]] libcef library located in CEF subfolder relative to revbrowser external
const char *MCCefPlatformGetCefLibraryPath(void)
{
	static char *s_lib_path = nil;

	if (s_lib_path == nil)
		/* UNCHECKED */ MCCefLinuxAppendPath(MCCefPlatformGetCefFolder(), "libcef.so", s_lib_path);

	return s_lib_path;
}

////////////////////////////////////////////////////////////////////////////////

static bool get_link_size(const char *p_path, uint32_t &r_size)
{
	if (p_path == nil)
		return false;
		
	struct stat t_stat;
	if (lstat(p_path, &t_stat) == -1)
		return false;
	
	r_size = t_stat.st_size;
	return true;
}

static bool get_link_path(const char *p_link, char *&r_path)
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

static bool get_exe_path_from_proc_fs(char *&r_path)
{
	return get_link_path("/proc/self/exe", r_path);
}

//////////

const char *MCCefPlatformGetExecutableFolder(void)
{
    static char *s_exe_path = nil;

	if (s_exe_path == nil)
	{
		bool t_success;
		t_success = get_exe_path_from_proc_fs(s_exe_path);
		if (t_success)
		{
			// remove library component from path
			uint32_t t_index;
			if (MCCStringLastIndexOf(s_exe_path, '/', t_index))
				s_exe_path[t_index] = '\0';
		}
	}
	
	return s_exe_path;
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
