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

#ifndef __MCCEF_SHARED_H__
#define __MCCEF_SHARED_H__

const char *MCCefPlatformGetCefLibraryPath(void);
const char *MCCefPlatformGetResourcesDirPath(void);
const char *MCCefPlatformGetLocalePath(void);

// AL-2015-02-17: [[ SB Inclusions ]] Work around problems linking to MCU_ functions from CEF
extern "C" void *MCU_loadmodule(const char *p_source);
extern "C" void MCU_unloadmodule(void *p_module);
extern "C" void *MCU_resolvemodulesymbol(void *p_module, const char *p_symbol);

#endif // __MCCEF_SHARED_H__
