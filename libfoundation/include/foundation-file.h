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

#ifndef __MC_FOUNDATION_FILE__
#define __MC_FOUNDATION_FILE__

#ifndef __MC_FOUNDATION__
#include <foundation.h>
#endif

////////////////////////////////////////////////////////////////////////////////

enum MCOpenFileMode
{
    kMCOpenFileModeRead,
    kMCOpenFileModeWrite,
    kMCOpenFileModeUpdate,
    kMCOpenFileModeAppend,
    kMCOpenFileModeCreate,
};

////////////////////////////////////////////////////////////////////////////////

bool MCFileCreateStreamForFile(MCStringRef p_filename, MCOpenFileMode p_mode, MCStreamRef& r_stream);

bool MCFilePathToNative(MCStringRef p_path, MCStringRef& r_native);
bool MCFilePathFromNative(MCStringRef p_path, MCStringRef& r_livecode_path);
bool MCFileGetCurrentFolder(MCStringRef& r_path);
bool MCFileResolveNativePath(MCStringRef p_path, MCStringRef& r_resolved_path);
bool MCFileResolvePath(MCStringRef p_path, MCStringRef& r_resolved_path);

////////////////////////////////////////////////////////////////////////////////

#endif
