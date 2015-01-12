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
#include <foundation-file.h>

#include "foundation-private.h"
#include "foundation-file-private"

/* ================================================================
 * Windows file errors
 * ================================================================ */

static bool
__MCFileErrorCodeGetDescription (DWORD p_error_code,
                                 MCStringRef &r_description)
{
	if (p_error_code == ERROR_SUCCESS)
	{
		r_description = MCSTR("Unknown error");
		return true;
	}

	unichar_t *t_temp = NULL;
	DWORD t_temp_size;

	t_temp_size = FormatMessageW ((FORMAT_MESSAGE_ALLOCATE_BUFFER |
	                               FORMAT_MESSAGE_FROM_SYSTEM |
	                               FORMAT_MESSAGE_IGNORE_INSERTS),
	                              NULL,
	                              p_error_code,
	                              MAKELANGID (LANG_NEUTRAL, SUBLANG_DEFAULT),
	                              (LPWSTR) &t_temp,
	                              0, NULL);

	if (0 == t_temp_size || NULL == t_temp)
		t_description = MCSTR("Unknown error");
	else
		return MCStringCreateWithWString (t_temp, &t_description);

	return true;
}

static bool
__MCFileThrowIOErrorWithErrorCode (MCStringRef p_native_path,
                                   MCStringRef p_message,
                                   DWORD p_error_code)
{
	MCAutoStringRef t_description;
	MCAutoNumberRef t_error_code;

	/* UNCHECKED */ __MCFileErrorCodeGetDescription (p_error_code,
	                                                 &t_description);
	/* UNCHECKED */ MCNumberCreateWithInteger (p_error_code, &t_error_code);

	MCAutoStringRef t_path;
	/* UNCHECKED */ __MCFilePathFromNative (p_native_path, &t_path);

	if (p_message)
		return MCErrorCreateAndThrowWithMessage (kMCFileIOErrorTypeInfo,
		                                         p_message,
		                                         "path", *t_path,
		                                         "description", *t_description,
		                                         "error_code", *t_error_code,
		                                         NULL);
	else
		return MCErrorCreateAndThrow (kMCFileIOErrorTypeInfo,
		                              "path", *t_path,
		                              "description", *t_description,
		                              "error_code", *t_error_code,
		                              NULL);
}

static bool
__MCFileThrowReadErrorWithErrorCode (MCStringRef p_native_path,
                                     DWORD p_error_code)
{
	return __MCFileThrowIOErrorWithErrorCode (p_native_path, MCSTR("Failed to read from file '%{path}': %{description"), p_error_code);
}

static bool
__MCFileThrowWriteErrorWithErrorCode (MCStringRef p_native_path,
                                      DWORD p_error_code)
{
	return __MCFileThrowIOErrorWithErrorCode (p_native_path, MCSTR("Failed to write to file '%{path}': %{description"), p_error_code);
}

static bool
__MCFileThrowOpenErrorWithErrorCode (MCStringRef p_native_path,
                                     DWORD p_error_code)
{
	return __MCFileThrowIOErrorWithErrorCode (p_native_path, MCSTR("Failed to open file '%{path}': %{description"), p_error_code);
}

/* ================================================================
 * Windows whole-file IO
 * ================================================================ */

/* Read the full contents of the file located at p_native_path into r_data.
 *
 * Performs the read in several steps:
 *
 * 1) Open a read-only handle to the file
 * 2) Get the size of the file, using the handle
 * 3) Allocate a new, sufficiently-large buffer
 * 4) Read the file into the buffer in a single operation.
 */
bool
__MCFileGetContents (MCStringRef p_native_path,
                     MCDataRef & r_data)
{
	/* ---------- 1) Open file handle */

	/* Get a system path */
	MCAutoStringRefAsWString t_path_w32;
	if (!t_path_w32.Lock(p_native_path))
		return false;

	HANDLE t_handle;
	t_handle = CreateFileW (*t_path_w32,            /* filename */
	                        GENERIC_READ,           /* desired access */
	                        0,                      /* share mode */
	                        NULL,                   /* security attr. */
	                        OPEN_EXISTING,          /* creation disp. */
	                        FILE_ATTRIBUTE_NORMAL,  /* flags & attrs. */
	                        NULL);                  /* template file */

	if (h == INVALID_HANDLE_VALUE)
	{
		return __MCFileThrowOpenErrorWithErrorCode (p_native_path, GetLastError());
	}

	/* ---------- 2) Get the file size */

	LARGE_INTEGER t_file_size;
	if (!GetFileSizeEx (t_handle, &t_file_size))
	{
		__MCFileThrowIOErrorWithErrorCode (p_native_path, MCSTR("Failed to read from '%{path}'; GetFileSizeEx() failed: %{description}"), GetLastError());
		goto error_cleanup;
	}

	/* ---------- 3) Allocate a sufficiently-large buffer */
	byte *t_buffer = NULL;
	size_t t_file_size;

	/* Check that the file isn't too large (i.e., size > 2^31 on
	 * 32-bit systems) */
	if (t_file_size.QuadPart < 0 || t_file_size.QuadPart > SIZE_MAX)
	{
		__MCFileThrowIOErrorWithErrorCode (p_native_path, MCSTR("File '%{path}' is too large"), 0);
		goto error_cleanup;
	}

	t_file_size = (size_t) t_file_size.QuadPart;

	if (!MCMemoryAllocate (t_buffer, t_file_size))
		goto error_cleanup;

	/* ---------- 4) Read the contents of the file */

	/* If the file size is bigger than will fit into a DWORD (i.e. 32
	 * bits), then it may be necessary to perform multiple reads. */
	size_t t_total_read = 0;

	while (t_total_read < t_file_size)
	{
		uint32_t t_bytes_request;
		uint32_t t_bytes_read;

		t_bytes_request = MCMax ((t_file_size - t_total_read), UINT32_MAX);

		if (!ReadFile (t_handle, t_buffer,
		               t_bytes_request, &t_bytes_read, NULL))
		{
			__MCFileThrowReadErrorWithErrorCode (p_native_path, GetLastError());
			goto error_cleanup;
		}

		/* Handle reaching EOF sooner than expected (hopefully this
		 * should never happen because called CreateFileW() without
		 * allowing any sharing, so the file should be locked. */
		if (0 == t_bytes_read) break;

		t_total_read += t_bytes_read;
	}

	/* ---------- 5) Create & return MCDataRef */

	/* UNCHECKED */ CloseHandle (t_handle);

	return MCDataCreateWithBytesAndRelease (t_buffer, t_total_read, r_data);

 error_cleanup:
	/* UNCHECKED */ CloseHandle (t_handle);
	MCMemoryDeallocate (t_buffer);
	return false;
}

/* Creates (or replaces) the file at p_native_path with a new file
 * containing p_data. */
bool
__MCFileSetContents (MCStringRef p_native_path,
                     MCDataRef p_data)
{
	/* Get a system path */
	MCAutoStringRefAsWString t_path_w32;
	if (!t_path_w32.Lock (p_native_path))
		return false;

	/* ---------- 1) Create a temporary file */

	/* FIXME Bad/insecure implementation:
	 *
	 * 1) The generated filename isn't guaranteed to be on the same
	 *    filesystem as the target file, which means that MoveFileEx()
	 *    is less likely to be atomic.
	 *
	 * 2) GetTempFileName() suffers from a temporary file race
	 *    condition, in that it's possible for an attacker to mess
	 *    with the file it generates in between GetTempFileName()
	 *    returning and our call to CreateFileW().
	 *
	 * 3) Path length limitations, fixed size buffers, etc.
	 */
	uinteger_t t_temp_result;

	unichar_t t_tempdir_path_w32[MAX_PATH];
	DWORD t_tempdir_path_w32_len;
	t_tempdir_path_w32_len = GetTempPathW (MAX_PATH,
	                                       t_tempdir_path_w32);
	if (0 == t_tempdir_path_w32_len ||
	    (MAX_PATH - 14) < t_tempdir_path_w32_len)
		return __MCFileThrowIOErrorWithErrorCode (kMCEmptyString, MCSTR("Failed to create temporary file; GetTempPath() failed: %{description}"), GetLastError());

	unichar_t t_temp_path_w32[MAX_PATH];
	UINT t_temp_result;
	t_temp_result = GetTempFileNameW (t_tempdir_path_w32, /* path name */
	                                  NULL,               /* prefix */
	                                  0,                  /* unique */
	                                  t_temp_path_w32);   /* temp file name */
	if (0 == t_temp_result)
		return __MCFileThrowIOErrorWithErrorCode (kMCEmptyString, MCSTR("Failed to create temporary file; GetTempFileNameW() failed: %{description}"), GetLastError());

	MCAutoStringRef t_temp_native_path;
	if (!MCStringCreateWithWString (t_temp_path_w32, &t_temp_native_path))
	{
		goto error_cleanup;
	}

	/* ---------- 2) Populate the temporary file */

	/* FIXME Possibly inefficient */
	MCStreamRef t_stream = NULL;
	if (!__MCFileCreateStream (t_temp_native_path, kMCOpenFileModeRead,
	                           t_stream))
		goto error_cleanup;

	if (!__MCStreamWrite (t_stream, MCDataGetBytePtr (p_data),
	                      MCDataGetLength (p_data)))
		goto error_cleanup;

	/* FIXME explicitly finish stream */
	MCValueRelease (t_stream);
	t_stream = NULL;

	/* ---------- 3) Move the temporary file into place */

	if (!MoveFileEx (t_temp_path_w32, t_path_w32, MOVEFILE_REPLACE_EXISTING))
	{
		/* Report rename error */
		DWORD t_error = GetLastError();
		MCAutoStringRef t_description, t_path, t_temp_path;
		MCAutoNumberRef t_error_code;
		/* UNCHECKED */ __MCFileErrorCodeGetDescription (t_error,
		                                                 &t_description);
		/* UNCHECKED */ MCNumberCreateWithInteger (t_error, &t_error_code);
		/* UNCHECKED */ __MCFilePathFromNative (p_native_path, &t_path);
		/* UNCHECKED */ __MCFilePathFromNative (*t_temp_native_path,
		                                        &t_temp_path);

		MCErrorCreateAndThrowWithMessage (
		    kMCFileIOErrorTypeInfo,
		    MCSTR("Failed to rename file '%{temp_path}' to '%{path}': %{description}"),
		    "path", *t_path,
		    "temp_path", *t_temp_path,
		    "description", *t_description,
		    "error_code", *t_error_code,
		    NULL);

		goto error_cleanup;
	}
	return true;

 error_cleanup:
	/* FIXME explicitly finish stream */
	MCValueRelease (t_stream);
	/* UNCHECKED */ DeleteFileW (t_temp_path_w32);
	return false;
}

/* ================================================================
 * Path manipulation
 * ================================================================ */

/* Convert a path in LiveCode representation (with '/' as the file
 * separator) to a Windows path (using '\').  This function also
 * performs some (very) limited validation, ensuring that the input
 * path does not contain any nul bytes or '\' characters.
 *
 * FIXME Currently, this function -- and thus the files API on Windows
 * -- does not support the translation of long paths (i.e. longer than
 * 260 characters including the leading disk specifier and the
 * trailing nul).  There are a number of issues here:
 *
 * 1) Using __MCFilePathToNative() followed by
 *    __MCFilePathFromNative() is supposed to be a lossless operation,
 *    but supporting long file names means losing the relative
 *    filename information (because "\\?\" format supports absolute
 *    filenames only).
 *
 * 2) In some cases, users may be intentionally trying to open devices
 *    (to obtain an input stream for a serial port, for example).  It
 *    would be necessary to provide an alternative API for these
 *    users.
 *
 * 3) Since we may want to use the "\\?\" format for the native
 *    representation in the future, we need to forbid "/" in native
 *    paths returned by this function (because "\\?\" namespace treats
 *    them as normal characters rather than as path separators).
 *
 * It may be better to resolve these issues (and similar ones on other
 * platforms, e.g. Linux) by defining a 'filename' type and adding API
 * for obtaining the filename in various different forms.
 *
 * Initially, there's support for translating file namespace ("\\?\")
 * paths, but only if they're presented to the API already in that
 * format.
 */
bool
__MCFilePathToNative (MCStringRef p_path,
                      MCStringRef & r_native_path)
{
	uindex_t t_len;
	t_len = MCStringGetLength (p_path);

	if (0 == t_len)
		return __MCFileThrowInvalidPathError (p_path);

	/* Check for '//?/' prefix.  If present, we handle some path
	 * translations differently. */
	bool t_file_namespace;
	t_file_namespace = MCStringBeginsWithCString (p_path, "//?/",
	                                              kMCStringOptionCompareExact);

	MCAutoArray<unichar_t> t_native_chars;
	if (!t_native_chars.New (t_len))
		return false;

	for (uindex i = 0; i < t_len; ++i)
	{
		unichar_t t_char = MCStringGetCharAtIndex (p_path, i);

		switch (t_char)
		{
		case 0:
			/* Path may not incorporate a nul */
			return __MCFileThrowInvalidPathError (p_path);
		case '\\':
			/* If the input path is a Win32 file namespace path,
			 * translate '\\' to '/', since they'll be treated as
			 * "normal" path characters.  Otherwise, fail (because we
			 * may need to do translation to file namespace in future
			 * versions of this API). */
			if (t_file_namespace)
				t_native_chars[i] = '/';
			else
				return __MCFileThrowInvalidPathError (p_path);
			break;
		case '/':
			t_native_chars[i] = '\\';
			break;
		default:
			t_native_chars[i] = t_char;
			break;
		}
	}

	return MCStringCreateWithChars (t_native_chars.Ptr(),
	                                t_native_chars.Size(),
	                                r_native_path);
}

bool
__MCFilePathFromNative (MCStringRef p_native_path,
                        MCStringRef & r_path)
{
	uindex_t t_len;
	t_len = MCStringGetLength (p_native_path);

	if (0 == t_len)
		return MCStringCopy (p_native_path, r_path);

	/* Check for '\\?\' prefix.  If present, we handle some path
	 * translations differently. */
	bool t_file_namespace;
	t_file_namespace = MCStringBeginsWithCString (p_native_path, "\\\\?\\",
	                                              kMCStringOptionCompareExact);

	MCAutoArray<unichar_t> t_internal_chars;
	if (!t_internal_chars.New (t_len))
		return false;

	for (uindex i = 0; i < t_len; ++i)
	{
		unichar_t t_char = MCStringGetCharAtIndex (p_native_path, i);

		switch (t_char)
		{
		case 0:
			/* If a nul is encountered, truncate the output path to
			 * the number of preceding characters visited */
			t_internal_chars.Shrink (i);
			break;
		case '/':
			/* If the input path is a Win32 file namespace path,
			 * translate '/' to '\\', since they're "normal" path
			 * characters in this context.  Otherwise, fail (beacuse
			 * we may need to do translation to/from file namespace in
			 * future versions of this API). */
			if (t_file_namespace)
				t_native_chars[i] = '\\';
			else
				return __MCFileThrowInvalidPathError (p_native_path);
		case '\\':
			t_internal_chars[i] = '/';
			break;
		default:
			t_internal_chars[i] = t_char;
			break;
		}
	}

	return MCStringCreateWithChars (t_internal_chars.Ptr(),
	                                t_internal_chars.Size(),
	                                r_path);
}

/* ================================================================
 * File stream creation
 * ================================================================ */

bool
__MCFileCreateStream (MCStringRef p_native_path,
                      intenum_t p_mode,
                      MCStreamRef & r_stream)
{
	bool t_success = true;

	/* Get a system path */
	MCAutoStringRefAsWString t_path_w32;
	if (!t_path_w32.Lock(p_native_path))
		return false;

	/* Three step process:
	 *
	 * 1) Open (and possibly create) a file using CreateFileW().  This
	 *    is to allow the feature whereby a new file is forced to be
	 *    created.
	 *
	 * 2) Convert the file handle to a file descriptor using
	 *    _open_osfhandle().
	 *
	 * 3) Turn the file descriptor into a cstdio stream using
	 *    _fdopen().
	 */

	/* ---------- 1. Call CreateFileW() */

	/* Compute flags */
	DWORD t_desired_access = 0;
	DWORD t_creation_disposition = 0;

	if ((p_mode & kMCOpenFileModeRead) && (p_mode & kMCOpenFileModeWrite))
	{
		t_desired_access |= GENERIC_READ | GENERIC_WRITE;

		if (p_mode & kMCOpenFileModeCreate)
			t_creation_disposition = CREATE_NEW;
		else
			t_creation_disposition = OPEN_ALWAYS;
	}
	else if (p_mode & kMCOpenFileModeRead)
	{
		t_desired_access |= GENERIC_READ;
		t_creation_disposition = OPEN_EXISTING;
	}
	else if (p_mode & kMCOpenFileModeWrite)
	{
		t_desired_access |= GENERIC_WRITE;

		if (p_mode & kMCOpenFileModeCreate)
			t_creation_disposition = CREATE_NEW;
		else if (p_mode & kMCOpenFileModeAppend)
			t_creation_disposition = OPEN_ALWAYS;
		else
			t_creation_disposition = CREATE_ALWAYS;
	}
	else
	{
		return false;
	}

	/* Open/create file */
	HANDLE t_handle;
	t_handle = CreateFileW (*t_path_w32,            /* filename */
	                        t_desired_access,       /* desired access */
	                        0,                      /* share mode */
	                        NULL,                   /* security attr. */
	                        t_creation_disposition, /* creation disp. */
	                        FILE_ATTRIBUTE_NORMAL,  /* flags & attrs. */
	                        NULL);                  /* template file */

	if (h == INVALID_HANDLE_VALUE)
	{
		return __MCFileThrowOpenErrorWithErrorCode (p_native_path,
		                                            GetLastError());
	}

	/* ---------- 2. Call _open_osfhandle() */
	errno = 0;
	int t_save_errno;

	int t_oflags = 0;

	/* Compute flags */
	if ((p_mode & kMCOpenFileModeRead) && !(p_mode & kMCOpenFileModeWrite))
		t_oflags |= _O_RDONLY;
	else if (p_mode & kMCOpenFileModeAppend)
	    t_oflags |= _O_APPEND;

	/* Create file descriptor */
	int t_fd = -1;

	t_fd = _open_osfhandle ((intptr_t) t_handle, t_oflags);
	t_save_errno = errno;

	if (-1 == t_fd)
	{
		CloseHandle (t_handle);

		return __MCFileThrowOpenErrorWithErrno (p_native_path, t_save_errno);
	}

	/* ---------- 3. Call _fdopen() */

	/* Compute flags (yes, again) */
	const char *t_stream_mode;
	if ((p_mode & kMCOpenFileModeRead) && (p_mode & kMCOpenFileModeWrite))
	{
		t_stream_mode = (p_mode & kMCOpenFileModeAppend) ? "ab+" : "rb+";
	}
	else if (p_mode & kMCOpenFileModeRead)
	{
		t_stream_mode = "r";
	}
	else if (p_mode & kMCOpenFileModeWrite)
	{
		t_stream_mode = (p_mode & kMCOpenFileModeAppend) ? "ab" : "wb"
	}

	/* Create stream */
	FILE *t_cstream = NULL;
	t_cstream = _fdopen (t_fd, t_stream_mode);
	t_save_errno = errno;

	if (NULL == t_cstream)
	{
		/* UNCHECKED */ _close (t_fd); /* Also closes underlying handle */

		return __MCFileThrowOpenErrorWithErrno (p_native_path, t_save_errno);
	}

	/* Store the newly-created cstdio stream in a new MCStream instance. */
	MCStreamRef t_stream;
	if (!__MCStdioStreamCreate (t_cstream, t_stream))
	{
		/* UNCHECKED */ fclose (t_cstream); /* Also closes underlying handle */
	}

	r_stream = t_stream;
	return true;
}
