/*                                                                     -*-c++-*-
Copyright (C) 2015 Runtime Revolution Ltd.

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

#include <foundation.h>
#include <foundation-auto.h>

#include "foundation-file.h"

////////////////////////////////////////////////////////////////////////////////

extern "C" MC_DLLEXPORT void
MCFileExecGetContents (MCStringRef p_path, MCDataRef & r_data)
{
	/* UNCHECKED */ MCFileGetContents (p_path, r_data);
}

extern "C" MC_DLLEXPORT void
MCFileExecSetContents (MCDataRef p_contents, MCStringRef p_path)
{
	/* UNCHECKED */ MCFileSetContents (p_path, p_contents);
}

////////////////////////////////////////////////////////////////////////////////

extern "C" MC_DLLEXPORT void
MCFileExecDeleteFile (MCStringRef p_path)
{
	/* UNCHECKED */ MCFileDelete (p_path);
}

extern "C" MC_DLLEXPORT void
MCFileExecCreateDirectory (MCStringRef p_path)
{
	/* UNCHECKED */ MCFileCreateDirectory (p_path);
}

extern "C" MC_DLLEXPORT void
MCFileExecDeleteDirectory (MCStringRef p_path)
{
	/* UNCHECKED */ MCFileDeleteDirectory (p_path);
}

extern "C" MC_DLLEXPORT void
MCFileExecGetDirectoryEntries (MCStringRef p_path,
                               MCProperListRef & r_entries)
{
	/* UNCHECKED */ MCFileGetDirectoryEntries (p_path, r_entries);
}
