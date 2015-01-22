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

MC_DLLEXPORT extern MCTypeInfoRef kMCFileIOErrorTypeInfo;
MC_DLLEXPORT extern MCTypeInfoRef kMCFileEndOfFileErrorTypeInfo;
MC_DLLEXPORT extern MCTypeInfoRef kMCFileInvalidPathErrorTypeInfo;

////////////////////////////////////////////////////////////////////////////////

/* Read an entire file into allocated memory, with good error checking. */
MC_DLLEXPORT bool MCFileGetContents(MCStringRef p_filename, MCDataRef & r_data);

/* Write all of p_data to a file called p_filename, with good error
 * checking.  If a file called p_filename already exists it will be
 * overwritten.
 *
 * The data is first written to a temporary file which is then renamed
 * to the final name.  On some systems, the write will therefore be
 * atomic in some sense.  Note that:
 *
 * 1) On POSIX systems, if p_filename already exists hard links to
 *    p_filename will break.  Also, existing permissions, access
 *    control lists, metadata etc. may be lost.  If p_filename is a
 *    symbolic link, the link itself will be replaced, not the linked
 *    file.
 *
 * 2) On Windows it isn't possible to rename over an existing file.
 *    There will therefore be a race condition between the existing
 *    file being removed and the temporary file being moved into
 *    place.
 *
 * 3) On Windows, this function will fail if p_filename already exists
 *    and is open.
 */
MC_DLLEXPORT bool MCFileSetContents(MCStringRef p_filename, MCDataRef p_data);

////////////////////////////////////////////////////////////////////////////////

enum MCOpenFileMode
{
	kMCOpenFileModeRead = (1 << 0),
	kMCOpenFileModeWrite = (1 << 1),
	kMCOpenFileModeAppend = (1 << 2),
	kMCOpenFileModeCreate = (1 << 3),  /* Force creation of file */

	kMCOpenFileModeUpdate = (kMCOpenFileModeRead | kMCOpenFileModeWrite),
};

MC_DLLEXPORT bool MCFileCreateStream(MCStringRef p_filename, intenum_t p_mode, MCStreamRef& r_stream);

////////////////////////////////////////////////////////////////////////////////

#endif
