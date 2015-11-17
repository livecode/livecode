/*                                                                     -*-c++-*-
Copyright (C) 2015 LiveCode Ltd.

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
#include <foundation-system.h>
#include <foundation-auto.h>

////////////////////////////////////////////////////////////////////////////////

extern "C" MC_DLLEXPORT_DEF void
MCFileExecGetContents (MCStringRef p_path, MCDataRef & r_data)
{
	/* UNCHECKED */ MCSFileGetContents (p_path, r_data);
}

extern "C" MC_DLLEXPORT_DEF void
MCFileExecSetContents (MCDataRef p_contents, MCStringRef p_path)
{
	/* UNCHECKED */ MCSFileSetContents (p_path, p_contents);
}

////////////////////////////////////////////////////////////////////////////////

extern "C" MC_DLLEXPORT_DEF void
MCFileExecDeleteFile (MCStringRef p_path)
{
	/* UNCHECKED */ MCSFileDelete (p_path);
}

extern "C" MC_DLLEXPORT_DEF void
MCFileExecCreateDirectory (MCStringRef p_path)
{
	/* UNCHECKED */ MCSFileCreateDirectory (p_path);
}

extern "C" MC_DLLEXPORT_DEF void
MCFileExecDeleteDirectory (MCStringRef p_path)
{
	/* UNCHECKED */ MCSFileDeleteDirectory (p_path);
}

extern "C" MC_DLLEXPORT_DEF void
MCFileExecGetDirectoryEntries (MCStringRef p_path,
                               MCProperListRef & r_entries)
{
	/* UNCHECKED */ MCSFileGetDirectoryEntries (p_path, r_entries);
}

////////////////////////////////////////////////////////////////

extern "C" bool com_livecode_file_Initialize (void)
{
	return true;
}

extern "C" void com_livecode_file_Finalize (void)
{
}

////////////////////////////////////////////////////////////////
