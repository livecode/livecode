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

#include "system-private.h"

#include <foundation-auto.h>

#define MCS_FILE_CONVERT_PATH(path, native_path)	  \
	MCAutoStringRef native_path##__auto; \
	MCStringRef native_path; \
	do { if (!__MCSFilePathToNative (path, & native_path##__auto)) \
			return false; \
		native_path = * native_path##__auto; \
	} while (0)

/* ================================================================
 * Error handling
 * ================================================================ */

bool
__MCSFileThrowIOErrorWithErrno (MCStringRef p_native_path,
                                MCStringRef p_message,
                                int p_errno)
{
	MCAutoStringRef t_description;
	MCAutoNumberRef t_error_code;

	if (p_errno != 0)
	{
		/* UNCHECKED */ MCStringCreateWithCString (strerror (p_errno),
		                                           &t_description);
		/* UNCHECKED */ MCNumberCreateWithInteger (p_errno,
		                                           &t_error_code);
	}
	else
	{
		t_description = MCSTR("Unknown error");
		t_error_code = kMCZero;
	}

	MCAutoStringRef t_path;
	/* UNCHECKED */ __MCSFilePathFromNative (p_native_path, &t_path);

	if (p_message)
	{
		return MCErrorCreateAndThrowWithMessage (kMCSFileIOErrorTypeInfo,
		                                         p_message,
		                                         "path", *t_path,
		                                         "description", *t_description,
		                                         "error_code", *t_error_code,
		                                         NULL);
	}
	else
	{
		return MCErrorCreateAndThrow (kMCSFileIOErrorTypeInfo,
		                              "path", *t_path,
		                              "description", *t_description,
		                              "error_code", *t_error_code,
		                              NULL);
	}
}

bool
__MCSFileThrowReadErrorWithErrno (MCStringRef p_native_path,
                                  int p_errno)
{
	return __MCSFileThrowIOErrorWithErrno (p_native_path, MCSTR("Failed to read from file '%{path}': %{description"), p_errno);
}

bool
__MCSFileThrowWriteErrorWithErrno (MCStringRef p_native_path,
                                   int p_errno)
{
	return __MCSFileThrowIOErrorWithErrno (p_native_path, MCSTR("Failed to write to file '%{path}': %{description"), p_errno);
}

bool
__MCSFileThrowOpenErrorWithErrno (MCStringRef p_native_path,
                                  int p_errno)
{
	return __MCSFileThrowIOErrorWithErrno (p_native_path, MCSTR("Failed to open file '%{path}': %{description"), p_errno);
}

bool
__MCSFileThrowInvalidPathError (MCStringRef p_path)
{
	return MCErrorCreateAndThrow (kMCSFileInvalidPathErrorTypeInfo,
	                              "path", p_path,
	                              NULL);
}

/* ================================================================
 * Whole-file IO
 * ================================================================ */

bool
MCSFileGetContents (MCStringRef p_path,
                    MCDataRef & r_data)
{
	MCS_FILE_CONVERT_PATH(p_path, t_native_path);
	return __MCSFileGetContents (t_native_path, r_data);
}

bool
MCSFileSetContents (MCStringRef p_path,
                    MCDataRef p_data)
{
	MCS_FILE_CONVERT_PATH(p_path, t_native_path);
	return __MCSFileSetContents (t_native_path, p_data);
}

/* ================================================================
 * File streams
 * ================================================================ */

bool
MCSFileCreateStream (MCStringRef p_path,
                     intenum_t p_mode,
                     MCStreamRef & r_stream)
{
	MCS_FILE_CONVERT_PATH(p_path, t_native_path);
	return __MCSFileCreateStream (t_native_path, p_mode, r_stream);
}

/* ================================================================
 * File system operations
 * ================================================================ */

bool
MCSFileDelete (MCStringRef p_path)
{
	MCS_FILE_CONVERT_PATH(p_path, t_native_path);
	return __MCSFileDelete (p_path);
}

bool
MCSFileCreateDirectory (MCStringRef p_path)
{
	MCS_FILE_CONVERT_PATH(p_path, t_native_path);
	return __MCSFileCreateDirectory (t_native_path);
}

bool
MCSFileDeleteDirectory (MCStringRef p_path)
{
	MCS_FILE_CONVERT_PATH(p_path, t_native_path);
	return __MCSFileDeleteDirectory (t_native_path);
}

/* This callback function is used by MCSFileGetDirectoryEntries() to
 * convert directory entries from system filename representation to
 * LiveCode's internal representation. */
static bool
MCSFileGetDirectoryEntries_MapCallback (void *p_context,
                                        MCValueRef p_native_path,
                                        MCValueRef & r_path)
{
	MCAssert (kMCStringTypeInfo == MCValueGetTypeInfo (p_native_path));
	MCStringRef t_path;
	if (!__MCSFilePathFromNative (static_cast<MCStringRef>(p_native_path),
	                              t_path))
		return false;

	r_path = t_path;
	return true;
}

bool
MCSFileGetDirectoryEntries (MCStringRef p_path,
                            MCProperListRef & r_entries)
{
	MCS_FILE_CONVERT_PATH(p_path, t_native_path);

	MCAutoProperListRef t_native_entries;
	if (!__MCSFileGetDirectoryEntries (t_native_path, &t_native_entries))
		return false;

	/* Convert the returned directory entries to LiveCode path
	 * representation. */
	return MCProperListMap (*t_native_entries,
	                        MCSFileGetDirectoryEntries_MapCallback,
	                        r_entries,
	                        NULL);
}

/* ================================================================
 * Initialization
 * ================================================================ */

MCTypeInfoRef kMCSFileIOErrorTypeInfo;
MCTypeInfoRef kMCSFileEndOfFileErrorTypeInfo;
MCTypeInfoRef kMCSFileInvalidPathErrorTypeInfo;

static bool
__MCSFileCreateNamedErrorType (MCNameRef p_name,
                               MCStringRef p_message,
                               MCTypeInfoRef & r_error_type)
{
	MCAutoTypeInfoRef t_raw_typeinfo, t_typeinfo;

	if (!MCErrorTypeInfoCreate (MCNAME("file"), p_message, &t_raw_typeinfo))
		return false;

	if (!MCNamedTypeInfoCreate (p_name, &t_typeinfo))
		return false;

	if (!MCNamedTypeInfoBind (*t_typeinfo, *t_raw_typeinfo))
		return false;

	r_error_type = MCValueRetain (*t_typeinfo);
	return true;
}

bool
__MCSFileInitialize (void)
{
	/* Create error types */
	if (!__MCSFileCreateNamedErrorType (
	        MCNAME("livecode.lang.FileIOError"),
	        MCSTR("File input/output error for '%{path}': %{description}"),
	        kMCSFileIOErrorTypeInfo))
		return false;

	if (!__MCSFileCreateNamedErrorType (
	        MCNAME("livecode.lang.EndOfFileError"),
	        MCSTR("End of file '%{path}'"),
	        kMCSFileEndOfFileErrorTypeInfo))
		return false;

	if (!__MCSFileCreateNamedErrorType (
	        MCNAME("livecode.lang.InvalidFilenameError"),
	        MCSTR("No valid native path representation for path '%{path}'"),
	        kMCSFileInvalidPathErrorTypeInfo))
		return false;

	return true;
}

void
__MCSFileFinalize (void)
{
	MCValueRelease (kMCSFileInvalidPathErrorTypeInfo);
	MCValueRelease (kMCSFileEndOfFileErrorTypeInfo);
	MCValueRelease (kMCSFileIOErrorTypeInfo);
}
