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

const char *MCCefPlatformGetCefFolder();

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

// IM-2014-10-03: [[ revBrowserCEF ]] subprocess executable located in CEF subfolder relative to revbrowser external
const char *MCCefPlatformGetSubProcessName(void)
{
	static char *s_exe_path = nil;

	if (s_exe_path == nil)
		/* UNCHECKED */ MCCefLinuxAppendPath(MCCefPlatformGetCefFolder(), "revbrowser-cefprocess", s_exe_path);

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
