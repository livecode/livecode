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

#if !defined(__MCS_SYSTEM_H_INSIDE__)
#	error "Only <foundation-system.h> can be included directly"
#endif

/* ================================================================
 * Errors
 * ================================================================ */

MC_DLLEXPORT extern MCTypeInfoRef kMCSFileIOErrorTypeInfo;
MC_DLLEXPORT extern MCTypeInfoRef kMCSFileEndOfFileErrorTypeInfo;
MC_DLLEXPORT extern MCTypeInfoRef kMCSFileInvalidPathErrorTypeInfo;

#ifdef __MCS_INTERNAL_API__

bool __MCSFileThrowIOErrorWithErrno (MCStringRef p_native_path, MCStringRef p_message, int p_errno);
bool __MCSFileThrowReadErrorWithErrno (MCStringRef p_native_path, int p_errno);
bool __MCSFileThrowWriteErrorWithErrno (MCStringRef p_native_path, int p_errno);
bool __MCSFileThrowOpenErrorWithErrno (MCStringRef p_native_path, int p_errno);
bool __MCSFileThrowInvalidPathError (MCStringRef p_path);

#endif

/* ================================================================
 * Path manipulation
 * ================================================================ */

/* Get the current working directory */
MC_DLLEXPORT bool MCSFileGetCurrentDirectory (MCStringRef & r_path);

/* Test whether a path is absolute. */
MC_DLLEXPORT bool MCSFilePathIsAbsolute (MCStringRef p_path);

#ifdef __MCS_INTERNAL_API__

bool __MCSFileGetCurrentDirectory (MCStringRef & r_native_path);
bool __MCSFilePathIsAbsolute (MCStringRef p_path);

bool __MCSFilePathToNative (MCStringRef p_path, MCStringRef & r_native_path);
bool __MCSFilePathFromNative (MCStringRef p_native_path, MCStringRef & r_path);

#endif

/* ================================================================
 * Whole-file IO
 * ================================================================ */

/* Read an entire file into allocated memory, with good error checking. */
MC_DLLEXPORT bool MCSFileGetContents(MCStringRef p_filename, MCDataRef & r_data);

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
MC_DLLEXPORT bool MCSFileSetContents(MCStringRef p_filename, MCDataRef p_data);

#ifdef __MCS_INTERNAL_API__

bool __MCSFileGetContents (MCStringRef p_native_path, MCDataRef & r_data);
bool __MCSFileSetContents (MCStringRef p_native_path, MCDataRef p_data);

#endif

/* ================================================================
 * File streams
 * ================================================================ */

enum MCSFileOpenMode
{
	kMCSFileOpenModeRead = (1 << 0),
	kMCSFileOpenModeWrite = (1 << 1),
	kMCSFileOpenModeAppend = (1 << 2),
	kMCSFileOpenModeCreate = (1 << 3),  /* Force creation of file */

	kMCSFileOpenModeUpdate = (kMCSFileOpenModeRead | kMCSFileOpenModeWrite),
};

MC_DLLEXPORT bool MCSFileCreateStream(MCStringRef p_filename, intenum_t p_mode, MCStreamRef& r_stream);

#ifdef __MCS_INTERNAL_API__

bool __MCSFileCreateStream (MCStringRef p_native_path, intenum_t p_mode, MCStreamRef & r_stream);

#endif

/* ================================================================
 * Filesystem operations
 * ================================================================ */

enum MCSFileType
{
	kMCSFileTypeRegular = (1 << 0),
	kMCSFileTypeDirectory = (1 << 1),
	kMCSFileTypeSymbolicLink = (1 << 5),

	/* File type known, but cannot be expressed with MCSFileType */
	kMCSFileTypeUnsupported = (1 << 31),
};

/* Delete the file at path. */
MC_DLLEXPORT bool MCSFileDelete (MCStringRef p_path);

/* Create a directory at p_path.  Does not recursively create
 * directories. */
MC_DLLEXPORT bool MCSFileCreateDirectory (MCStringRef p_path);

/* Delete a directory at p_path.  The directory must be empty. */
MC_DLLEXPORT bool MCSFileDeleteDirectory (MCStringRef p_path);

/* Return a list of the entries in the directory at p_path.  The
 * returned list never includes "." and "..". */
MC_DLLEXPORT bool MCSFileGetDirectoryEntries (MCStringRef p_path, MCProperListRef & r_entries);

/* Get the type of the file located at p_path.  If p_follow_links is
 * true, dereferences symbolic links in p_path. */
MC_DLLEXPORT bool MCSFileGetType (MCStringRef p_path, bool p_follow_links, MCSFileType & r_type);

#ifdef __MCS_INTERNAL_API__

bool __MCSFileDelete (MCStringRef p_native_path);
bool __MCSFileCreateDirectory (MCStringRef p_native_path);
bool __MCSFileDeleteDirectory (MCStringRef p_native_path);
bool __MCSFileGetDirectoryEntries (MCStringRef p_native_path, MCProperListRef & r_entries);
bool __MCSFileGetType (MCStringRef p_native_path, bool p_follow_links, MCSFileType & r_type);

#endif

/* ================================================================
 * File API initialization
 * ================================================================ */

#ifdef __MCS_INTERNAL_API__

bool __MCSFileInitialize (void);
void __MCSFileFinalize (void);

#endif
