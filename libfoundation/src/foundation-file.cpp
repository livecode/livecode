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

#include <foundation.h>
#include <foundation-auto.h>
#include <foundation-file.h>

#include "foundation-file-private.h"

/* ================================================================
 * Error handling
 * ================================================================ */

bool
__MCFileThrowIOErrorWithErrno (MCStringRef p_native_path,
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
	/* UNCHECKED */ __MCFilePathFromNative (p_native_path, &t_path);

	if (p_message)
	{
		return MCErrorCreateAndThrowWithMessage (kMCFileIOErrorTypeInfo,
		                                         p_message,
		                                         "path", *t_path,
		                                         "description", *t_description,
		                                         "error_code", *t_error_code,
		                                         NULL);
	}
	else
	{
		return MCErrorCreateAndThrow (kMCFileIOErrorTypeInfo,
		                              "path", *t_path,
		                              "description", *t_description,
		                              "error_code", *t_error_code,
		                              NULL);
	}
}

bool
__MCFileThrowReadErrorWithErrno (MCStringRef p_native_path,
                                 int p_errno)
{
	return __MCFileThrowIOErrorWithErrno (p_native_path, MCSTR("Failed to read from file '%{path}': %{description"), p_errno);
}

bool
__MCFileThrowWriteErrorWithErrno (MCStringRef p_native_path,
                                  int p_errno)
{
	return __MCFileThrowIOErrorWithErrno (p_native_path, MCSTR("Failed to write to file '%{path}': %{description"), p_errno);
}

bool
__MCFileThrowOpenErrorWithErrno (MCStringRef p_native_path,
                                 int p_errno)
{
	return __MCFileThrowIOErrorWithErrno (p_native_path, MCSTR("Failed to open file '%{path}': %{description"), p_errno);
}

bool
__MCFileThrowInvalidPathError (MCStringRef p_path)
{
	return MCErrorCreateAndThrow (kMCFileInvalidPathErrorTypeInfo,
	                              "path", p_path,
	                              NULL);
}

/* ================================================================
 * Whole-file IO
 * ================================================================ */

bool
MCFileGetContents (MCStringRef p_path,
                   MCDataRef & r_data)
{
	MCAutoStringRef t_native_path;
	if (!__MCFilePathToNative (p_path, &t_native_path))
		return false;

	return __MCFileGetContents (*t_native_path, r_data);
}

bool
MCFileSetContents (MCStringRef p_path,
                   MCDataRef p_data)
{
	MCAutoStringRef t_native_path;
	if (!__MCFilePathToNative (p_path, &t_native_path))
		return false;

	return __MCFileSetContents (*t_native_path, p_data);
}

/* ================================================================
 * File streams
 * ================================================================ */

bool
MCFileCreateStream (MCStringRef p_path,
                    intenum_t p_mode,
                    MCStreamRef & r_stream)
{
	MCAutoStringRef t_native_path;
	if (!__MCFilePathToNative (p_path, &t_native_path))
		return false;

	return __MCFileCreateStream (*t_native_path, p_mode, r_stream);
}

/* ================================================================
 * Initialization
 * ================================================================ */

MCTypeInfoRef kMCFileIOErrorTypeInfo;
MCTypeInfoRef kMCFileEndOfFileErrorTypeInfo;
MCTypeInfoRef kMCFileInvalidPathErrorTypeInfo;

static bool
__MCFileCreateNamedErrorType (MCNameRef p_name,
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
__MCFileInitialize (void)
{
	/* Create error types */
	if (!__MCFileCreateNamedErrorType (
	          MCNAME("livecode.lang.FileIOError"),
	          MCSTR("File input/output error for '%{path}': %{description}"),
	          kMCFileIOErrorTypeInfo))
		return false;

	if (!__MCFileCreateNamedErrorType (
	          MCNAME("livecode.lang.EndOfFileError"),
	          MCSTR("End of file '%{path}'"),
	          kMCFileEndOfFileErrorTypeInfo))
		return false;

	if (!__MCFileCreateNamedErrorType (
	         MCNAME("livecode.lang.InvalidFilenameError"),
	         MCSTR("No valid native path representation for path '%{path}'"),
	         kMCFileInvalidPathErrorTypeInfo))
		return false;

	return true;
}

void
__MCFileFinalize (void)
{
	MCValueRelease (kMCFileInvalidPathErrorTypeInfo);
	MCValueRelease (kMCFileEndOfFileErrorTypeInfo);
	MCValueRelease (kMCFileIOErrorTypeInfo);
}
