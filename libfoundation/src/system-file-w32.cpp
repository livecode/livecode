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

#define __MCS_INTERNAL_API__
#include <foundation.h>
#include <foundation-system.h>
#include <foundation-auto.h>

#include <windows.h>
#include <io.h>
#include <fcntl.h>

/* ================================================================
 * Windows file errors
 * ================================================================ */

static bool
__MCSFileErrorCodeGetDescription (DWORD p_error_code,
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
		r_description = MCSTR("Unknown error");
	else
		return MCStringCreateWithWString (t_temp, r_description);

	return true;
}

static bool
__MCSFileThrowIOErrorWithErrorCode (MCStringRef p_native_path,
                                    MCStringRef p_message,
                                    DWORD p_error_code)
{
	MCAutoStringRef t_description;
	MCAutoNumberRef t_error_code;

	/* UNCHECKED */ __MCSFileErrorCodeGetDescription (p_error_code,
	                                                  &t_description);
	/* UNCHECKED */ MCNumberCreateWithInteger (p_error_code, &t_error_code);

	MCAutoStringRef t_path;
	/* UNCHECKED */ __MCSFilePathFromNative (p_native_path, &t_path);

	if (p_message)
		return MCErrorCreateAndThrowWithMessage (kMCSFileIOErrorTypeInfo,
		                                         p_message,
		                                         "path", *t_path,
		                                         "description", *t_description,
		                                         "error_code", *t_error_code,
		                                         NULL);
	else
		return MCErrorCreateAndThrow (kMCSFileIOErrorTypeInfo,
		                              "path", *t_path,
		                              "description", *t_description,
		                              "error_code", *t_error_code,
		                              NULL);
}

static bool
__MCSFileThrowReadErrorWithErrorCode (MCStringRef p_native_path,
                                      DWORD p_error_code)
{
	return __MCSFileThrowIOErrorWithErrorCode (p_native_path, MCSTR("Failed to read from file '%{path}': %{description}"), p_error_code);
}

static bool
__MCSFileThrowWriteErrorWithErrorCode (MCStringRef p_native_path,
                                       DWORD p_error_code)
{
	return __MCSFileThrowIOErrorWithErrorCode (p_native_path, MCSTR("Failed to write to file '%{path}': %{description}"), p_error_code);
}

static bool
__MCSFileThrowOpenErrorWithErrorCode (MCStringRef p_native_path,
                                      DWORD p_error_code)
{
	return __MCSFileThrowIOErrorWithErrorCode (p_native_path, MCSTR("Failed to open file '%{path}': %{description}"), p_error_code);
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
__MCSFileGetContents (MCStringRef p_native_path,
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

	if (t_handle == INVALID_HANDLE_VALUE)
	{
		return __MCSFileThrowOpenErrorWithErrorCode (p_native_path, GetLastError());
	}

	/* ---------- 2) Get the file size */

	LARGE_INTEGER t_file_size_struct;
	if (!GetFileSizeEx (t_handle, &t_file_size_struct))
	{
		__MCSFileThrowIOErrorWithErrorCode (p_native_path, MCSTR("Failed to read from '%{path}'; GetFileSizeEx() failed: %{description}"), GetLastError());
		goto error_cleanup;
	}

	/* ---------- 3) Allocate a sufficiently-large buffer */
	byte *t_buffer = NULL;

	/* Check that the file isn't too large (i.e., size > 2^31 on
	 * 32-bit systems) */
	if (t_file_size_struct.QuadPart < 0 || t_file_size_struct.QuadPart > SIZE_MAX)
	{
		__MCSFileThrowIOErrorWithErrorCode (p_native_path, MCSTR("File '%{path}' is too large"), 0);
		goto error_cleanup;
	}

	size_t t_file_size;
	t_file_size = t_file_size_struct.QuadPart;

	if (!MCMemoryAllocate (t_file_size, t_buffer))
		goto error_cleanup;

	/* ---------- 4) Read the contents of the file */

	/* If the file size is bigger than will fit into a DWORD (i.e. 32
	 * bits), then it may be necessary to perform multiple reads. */
	size_t t_total_read = 0;

	while (t_total_read < t_file_size)
	{
		uint32_t t_bytes_request;
		DWORD t_bytes_read;

		t_bytes_request = MCMin ((t_file_size - t_total_read), UINT32_MAX);

		if (!ReadFile (t_handle, t_buffer,
		               t_bytes_request, &t_bytes_read, NULL))
		{
			__MCSFileThrowReadErrorWithErrorCode (p_native_path, GetLastError());
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
__MCSFileSetContents (MCStringRef p_native_path,
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
		return __MCSFileThrowIOErrorWithErrorCode (kMCEmptyString, MCSTR("Failed to create temporary file; GetTempPath() failed: %{description}"), GetLastError());

	unichar_t t_temp_path_w32[MAX_PATH];
	t_temp_result = GetTempFileNameW (t_tempdir_path_w32, /* path name */
	                                  NULL,               /* prefix */
	                                  0,                  /* unique */
	                                  t_temp_path_w32);   /* temp file name */
	if (0 == t_temp_result)
		return __MCSFileThrowIOErrorWithErrorCode (kMCEmptyString, MCSTR("Failed to create temporary file; GetTempFileNameW() failed: %{description}"), GetLastError());

	MCAutoStringRef t_temp_native_path;
	if (!MCStringCreateWithWString (t_temp_path_w32, &t_temp_native_path))
	{
		goto error_cleanup;
	}

	/* ---------- 2) Populate the temporary file */

	/* FIXME Possibly inefficient */
	MCStreamRef t_stream = NULL;
	if (!__MCSFileCreateStream (*t_temp_native_path, kMCSFileOpenModeWrite,
	                            t_stream))
		goto error_cleanup;

	if (!MCStreamWrite (t_stream, MCDataGetBytePtr (p_data),
	                    MCDataGetLength (p_data)))
		goto error_cleanup;

	/* FIXME explicitly finish stream */
	MCValueRelease (t_stream);
	t_stream = NULL;

	/* ---------- 3) Move the temporary file into place */

	if (!MoveFileExW(t_temp_path_w32, *t_path_w32, MOVEFILE_REPLACE_EXISTING))
	{
		/* Report rename error */
		DWORD t_error = GetLastError();
		MCAutoStringRef t_description, t_path, t_temp_path;
		MCAutoNumberRef t_error_code;
		/* UNCHECKED */ __MCSFileErrorCodeGetDescription (t_error,
		                                                  &t_description);
		/* UNCHECKED */ MCNumberCreateWithInteger (t_error, &t_error_code);
		/* UNCHECKED */ __MCSFilePathFromNative (p_native_path, &t_path);
		/* UNCHECKED */ __MCSFilePathFromNative (*t_temp_native_path,
		                                         &t_temp_path);

		MCErrorCreateAndThrowWithMessage (
		    kMCSFileIOErrorTypeInfo,
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

/* The essential reference material for working with these functions
 * is the "Naming Files, Paths and Namespaces" page in the Windows API
 * documentation on MSDN. */

/* For use only when operating on *native* paths. */
static inline bool
__MCSFileCharIsSeparator (codepoint_t p_char)
{
	return (p_char == '/' || p_char == '\\');
}

/* For use only when operating on *native* paths. */
static inline bool
__MCSFileCharIsDriveLetter (codepoint_t p_char)
{
	/* Windows drive letters must be A-Z */
	return ((p_char >= 'A' && p_char <= 'Z') ||
	        (p_char >= 'a' && p_char <= 'z'));
}

/* For use only when operating on *native* paths.
 *
 * Tests whether p_path is a UNC path. */
static inline bool
__MCSFilePathIsUnc (MCStringRef p_native_path)
{
	if (2 > MCStringGetLength (p_native_path))
	{
		return false;
	}

	return (__MCSFileCharIsSeparator (MCStringGetCharAtIndex (p_native_path, 0)) &&
	        __MCSFileCharIsSeparator (MCStringGetCharAtIndex (p_native_path, 1)));
}

/* For use only when operating on *native* paths.
 *
 * Tests whether p_path begins with "\\?\" or "\\.\" (or equivalent).
 * N.b. that "\\?" and "\\." are treated as UNC paths rather than NT
 * namespace paths. */
static bool
__MCSFilePathIsNamespace (MCStringRef p_native_path)
{
	if (4 > MCStringGetLength (p_native_path))
	{
		return false;
	}

	if (!__MCSFilePathIsUnc (p_native_path))
	{
		return false;
	}

	codepoint_t t_char = MCStringGetCharAtIndex (p_native_path, 2);
	if ('.' != t_char && '?' != t_char)
	{
		return false;
	}

	if (!__MCSFileCharIsSeparator (MCStringGetCharAtIndex (p_native_path, 3)))
	{
		return false;
	}

	return true;
}

/* For use only when operating on *native* paths.
 *
 * Tests whether p_path is a special Windows "device" path. */
static inline bool
__MCSFilePathIsDevice (MCStringRef p_native_path)
{
	static const char* const kMCSDeviceBaseNames[] = {
		"CON",
		"PRN",
		"AUX",
		"NUL",
		"COM1", "COM2", "COM3", "COM4", "COM5", "COM6", "COM7", "COM8", "COM9",
		"LPT1", "LPT2", "LPT3", "LPT4", "LPT5", "LPT6", "LPT7", "LPT8", "LPT9",
	};

	/* UNC paths are safe! */
	if (__MCSFilePathIsUnc (p_native_path))
		return false;

	uindex_t t_len = MCStringGetLength (p_native_path);

	/* Find the last separator in the path */
	/* FIXME[2017-04-20] there should be a "basename" function */
	uindex_t t_basename_offset = t_len;
	while (t_basename_offset > 0)
	{
		codepoint_t t_char = MCStringGetCharAtIndex (p_native_path,
		                                             t_basename_offset - 1);
		if (__MCSFileCharIsSeparator (t_char))
			break;
		--t_basename_offset;
	}

	/* Find the first '.' in the separator */
	uindex_t t_basename_len = 0;
	while (t_basename_len + t_basename_offset < t_len)
	{
		if ('.' == MCStringGetCharAtIndex (p_native_path,
		                                   t_basename_offset + t_basename_len))
		{
			break;
		}
		++t_basename_len;
	}

	/* No basename -> not a device */
	if (0 == t_basename_len)
	{
		return false;
	}

	MCAutoStringRef t_basename;
	if (!MCStringCopySubstring (p_native_path,
	                            MCRangeMake (t_basename_offset, t_basename_len),
	                            &t_basename))
	{
		return false;
	}

	/* Check against static table of reserved device names */
	for (const char* t_device_base : kMCSDeviceBaseNames)
	{
		if (MCStringIsEqualToCString (*t_basename, t_device_base,
		                              kMCStringOptionCompareCaseless))
		{
			return true;
		}
	}

	return false;
}

/* Convert a path in LiveCode representation (with '/' as the file
 * separator) to a Windows path (using '\').  This function also
 * performs some (very) limited validation, ensuring that the input
 * path does not contain any nul bytes or '\' characters, and is not a
 * "special" device path (e.g. "NUL" or "COM1").
 *
 * FIXME Currently, this function -- and thus the files API on Windows
 * -- does not support the translation of long paths (i.e. longer than
 * 260 characters including the leading disk specifier and the
 * trailing nul).  Excessively long paths are passed through to the
 * underlying operating system APIs for them to error on.
 */
bool
__MCSFilePathToNative (MCStringRef p_path,
                       MCStringRef & r_native_path)
{
	uindex_t t_len;
	t_len = MCStringGetLength (p_path);

	if (0 == t_len)
		return __MCSFileThrowInvalidPathError (p_path);

	MCAutoArray<unichar_t> t_native_chars;
	if (!t_native_chars.New (t_len))
		return false;

	for (uindex_t i = 0; i < t_len; ++i)
	{
		unichar_t t_char = MCStringGetCharAtIndex (p_path, i);

		/* Explicitly reserved characters */
		if (t_char < 32)
		{
			return __MCSFileThrowInvalidPathError (p_path);
		}

		switch (t_char)
		{
		case '\\':
			/* Backslashes in LiveCode internal paths are always
			 * invalid on Windows, because they are ambiguous.  In
			 * LiveCode internal paths, '\' is just another character,
			 * but Windows doesn't distinguish.  It's better to
			 * enforce that the only path separator used in internal
			 * paths is '/'. */
			return __MCSFileThrowInvalidPathError (p_path);
		case '/':
			t_native_chars[i] = '\\';
			break;
		default:
			t_native_chars[i] = t_char;
			break;
		}
	}

	MCAutoStringRef t_native_path;
	if (!MCStringCreateWithChars (t_native_chars.Ptr(),
	                              t_native_chars.Size(),
	                              &t_native_path))
	{
		return false;
	}

	/* Verify that the file is not a device path.  Assume that COM1 is a valid
	 * filename when a UNC path is used. */
	if (__MCSFilePathIsDevice (*t_native_path))
	{
		return __MCSFileThrowInvalidPathError (p_path);
	}

	return MCStringCopy (*t_native_path, r_native_path);
}

/* Convert a Windows path (using '\' as a separator) to LiveCode
 * representation (using '/').  This function also performs some
 * validation by truncating at nul bytes and ensuring that the path is
 * not a device path. */
bool
__MCSFilePathFromNative (MCStringRef p_native_path,
                         MCStringRef & r_path)
{
	uindex_t t_len;
	t_len = MCStringGetLength (p_native_path);

	if (0 == t_len)
		return MCStringCopy (p_native_path, r_path);

	if (__MCSFilePathIsDevice (p_native_path))
	{
		return __MCSFileThrowInvalidPathError (p_native_path);
	}

	MCAutoArray<unichar_t> t_internal_chars;
	if (!t_internal_chars.New (t_len))
		return false;

	for (uindex_t i = 0; i < t_len; ++i)
	{
		unichar_t t_char = MCStringGetCharAtIndex (p_native_path, i);

		switch (t_char)
		{
		case 0:
			/* If a nul is encountered, truncate the output path to
			 * the number of preceding characters visited. */
			t_len = i;
			t_internal_chars.Shrink (i);
			break;
		case '/':
		case '\\':
			/* Always translate both '/' and '\' separators to '/'. */
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

bool
__MCSFileGetCurrentDirectory (MCStringRef & r_native_path)
{

	DWORD t_length = 0;
	MCAutoArray<unichar_t> t_native_chars;
	while (true)
	{
		t_length = GetCurrentDirectoryW (t_native_chars.Size(), t_native_chars.Ptr());

		/* On error, GetCurrentDirectoryW() returns 0 */
		if (0 == t_length)
		{
			return __MCSFileThrowIOErrorWithErrorCode (kMCEmptyString, MCSTR("Failed to get current working directory: %{description}"), GetLastError());
		}

		/* On success, it returns the number of characters read, not
		 * including the trailing nul */
		if (t_length + 1 < t_native_chars.Size())
		{
			break;
		}

		/* If the buffer isn't large enough, it returns the length of
		 * buffer required. */
		if (!t_native_chars.Resize (t_length))
		{
			return false;
		}
	}

	return MCStringCreateWithWString (t_native_chars.Ptr(), r_native_path);
}

bool
__MCSFilePathIsAbsolute (MCStringRef p_native_path)
{
	/* This case covers UNC paths and absolute paths on the current drive. */
	if (MCStringIsEmpty (p_native_path))
	{
		return false;
	}

	if (__MCSFileCharIsSeparator(MCStringGetCharAtIndex (p_native_path, 0)))
	{
		return true;
	}

	/* This case covers "C:\" absolute Windows paths.  Note that "C:foo\bar"
	 * is actually a relative path. */
	if (3 > MCStringGetLength (p_native_path))
	{
		return false;
	}

	if (__MCSFileCharIsDriveLetter (MCStringGetCharAtIndex (p_native_path, 0)) &&
	    ':' == MCStringGetCharAtIndex (p_native_path, 1) &&
	    __MCSFileCharIsSeparator (MCStringGetCharAtIndex (p_native_path, 2)))
	{
		return true;
	}

	return false;
}

/* ================================================================
 * File stream creation
 * ================================================================ */

bool
__MCSFileCreateStream (MCStringRef p_native_path,
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

	if ((p_mode & kMCSFileOpenModeRead) && (p_mode & kMCSFileOpenModeWrite))
	{
		t_desired_access |= GENERIC_READ | GENERIC_WRITE;

		if (p_mode & kMCSFileOpenModeCreate)
			t_creation_disposition = CREATE_NEW;
		else
			t_creation_disposition = OPEN_ALWAYS;
	}
	else if (p_mode & kMCSFileOpenModeRead)
	{
		t_desired_access |= GENERIC_READ;
		t_creation_disposition = OPEN_EXISTING;
	}
	else if (p_mode & kMCSFileOpenModeWrite)
	{
		t_desired_access |= GENERIC_WRITE;

		if (p_mode & kMCSFileOpenModeCreate)
			t_creation_disposition = CREATE_NEW;
		else if (p_mode & kMCSFileOpenModeAppend)
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

	if (t_handle == INVALID_HANDLE_VALUE)
	{
		return __MCSFileThrowOpenErrorWithErrorCode (p_native_path,
		                                             GetLastError());
	}

	/* ---------- 2. Call _open_osfhandle() */
	errno = 0;
	int t_save_errno;

	int t_oflags = 0;

	/* Compute flags */
	if ((p_mode & kMCSFileOpenModeRead) && !(p_mode & kMCSFileOpenModeWrite))
		t_oflags |= _O_RDONLY;
	else if (p_mode & kMCSFileOpenModeAppend)
		t_oflags |= _O_APPEND;

	/* Create file descriptor */
	int t_fd = -1;

	t_fd = _open_osfhandle ((intptr_t) t_handle, t_oflags);
	t_save_errno = errno;

	if (-1 == t_fd)
	{
		CloseHandle (t_handle);

		return __MCSFileThrowOpenErrorWithErrno (p_native_path, t_save_errno);
	}

	/* ---------- 3. Call _fdopen() */

	/* Compute flags (yes, again) */
	const char *t_stream_mode;
	if ((p_mode & kMCSFileOpenModeRead) && (p_mode & kMCSFileOpenModeWrite))
	{
		t_stream_mode = (p_mode & kMCSFileOpenModeAppend) ? "ab+" : "rb+";
	}
	else if (p_mode & kMCSFileOpenModeRead)
	{
		t_stream_mode = "r";
	}
	else if (p_mode & kMCSFileOpenModeWrite)
	{
		t_stream_mode = (p_mode & kMCSFileOpenModeAppend) ? "ab" : "wb";
	}

	/* Create stream */
	FILE *t_cstream = NULL;
	t_cstream = _fdopen (t_fd, t_stream_mode);
	t_save_errno = errno;

	if (NULL == t_cstream)
	{
		/* UNCHECKED */ _close (t_fd); /* Also closes underlying handle */

		return __MCSFileThrowOpenErrorWithErrno (p_native_path, t_save_errno);
	}

	/* Store the newly-created cstdio stream in a new MCStream instance. */
	MCStreamRef t_stream;
	if (!__MCSStreamCreateWithStdio (t_cstream, t_stream))
	{
		/* UNCHECKED */ fclose (t_cstream); /* Also closes underlying handle */
	}

	r_stream = t_stream;
	return true;
}

/* ================================================================
 * Filesystem operations
 * ================================================================ */

bool
__MCSFileDelete (MCStringRef p_native_path)
{
	/* Get a system path */
	MCAutoStringRefAsWString t_path_w32;
	if (!t_path_w32.Lock(p_native_path))
		return false;

	if (!DeleteFileW (*t_path_w32))
		return __MCSFileThrowIOErrorWithErrorCode (p_native_path, MCSTR("Failed to delete file %{path}: %{description}"), GetLastError());

	return true;
}

bool
__MCSFileCreateDirectory (MCStringRef p_native_path)
{
	/* Get a system path */
	MCAutoStringRefAsWString t_path_w32;
	if (!t_path_w32.Lock(p_native_path))
		return false;

	if (!CreateDirectoryW (*t_path_w32, NULL))
		return __MCSFileThrowIOErrorWithErrorCode (p_native_path, MCSTR("Failed to create directory %{path}: %{description}"), GetLastError());

	return true;
}

bool
__MCSFileDeleteDirectory (MCStringRef p_native_path)
{
	/* Get a system path */
	MCAutoStringRefAsWString t_path_w32;
	if (!t_path_w32.Lock(p_native_path))
		return false;

	if (!RemoveDirectoryW (*t_path_w32))
		return __MCSFileThrowIOErrorWithErrorCode (p_native_path, MCSTR("Failed to delete directory %{path}: %{description}"), GetLastError());

	return true;
}

bool
__MCSFileGetDirectoryEntries (MCStringRef p_native_path,
                              MCProperListRef & r_native_entries)
{
	/* Windows needs the search string to be in a glob format.  To
	 * match all directories, the glob string needs to end in "\\*".
	 * However, the input path might already end in a directory
	 * separator.  If so, remove it. */
	MCAutoStringRef t_native_path;
	if (MCStringEndsWithCString (p_native_path, (const char_t*)"\\",
	                             kMCStringOptionCompareExact))
		/* Trim last character */
		MCStringCopySubstring (p_native_path,
		    MCRangeMake (0, MCStringGetLength (p_native_path) - 1),
		    &t_native_path);
	else
		MCStringCopy (p_native_path, &t_native_path);

	/* Add the glob suffix */
	MCAutoStringRef t_native_search_path;
	if (!MCStringFormat (&t_native_search_path, "%@\\*",
	                     *t_native_path))
		return false;

	/* Get a system path */
	MCAutoStringRefAsWString t_search_path_w32;
	if (!t_search_path_w32.Lock(*t_native_search_path))
		return false;

	/* Open a directory search handle */
	WIN32_FIND_DATAW t_find_data;
	HANDLE t_find_handle;
	DWORD t_error_code;

	t_find_handle = FindFirstFileW (*t_search_path_w32, &t_find_data);
	if (INVALID_HANDLE_VALUE == t_find_handle)
	{
		t_error_code = GetLastError();

		/* If the error *isn't* "file not found", throw an error */
		if (ERROR_FILE_NOT_FOUND != t_error_code)
			return __MCSFileThrowIOErrorWithErrorCode (p_native_path, MCSTR("Failed to get entries of directory %{path}: %{description}"), t_error_code);

		/* If the error *is* "file not found", then there can be two
		 * reasons.  First, the p_native_path isn't a directory, in
		 * which case we should throw an error.  If the p_native_path
		 * is a directory, on the other hand, then we should return
		 * the empty list. */
		MCAutoStringRefAsWString t_path_w32;
		if (!t_path_w32.Lock (*t_native_path))
			return false;

		t_find_handle = FindFirstFileW (*t_path_w32, &t_find_data);

		if (INVALID_HANDLE_VALUE == t_find_handle ||
		    !(FILE_ATTRIBUTE_DIRECTORY & t_find_data.dwFileAttributes))
		{
			return __MCSFileThrowIOErrorWithErrorCode (p_native_path, MCSTR("Failed to get entries of directory %{path}: %{path} is not a directory"), t_error_code);
		}
		else
		{
			return MCProperListCopy (kMCEmptyProperList, r_native_entries);
		}
	}

	/* Build a list of directory entries, skipping '.' and '..' */
	MCAutoStringRefArray t_entries;
	uindex_t t_num_entries = 0;

	MCStringRef t_curdir = MCSTR(".");
	MCStringRef t_parentdir = MCSTR("..");

	if (!t_entries.New(1))
		goto error_cleanup;

	do
	{
		MCStringRef t_entry_name;
		if (!t_entries.Extend(++t_num_entries))
			goto error_cleanup;

		if (!MCStringCreateWithWString (t_find_data.cFileName, t_entry_name))
			goto error_cleanup;

		/* Skip "." and ".." */
		if (MCStringIsEqualTo (t_entry_name, t_curdir, kMCStringOptionCompareExact) ||
		    MCStringIsEqualTo (t_entry_name, t_parentdir, kMCStringOptionCompareExact))
		{
			MCValueRelease (t_entry_name);
			continue;
		}

		t_entries[t_num_entries - 1] = t_entry_name;
	}
	while (0 != FindNextFileW(t_find_handle, &t_find_data));

	/* Clean up and return the list of directory entries */
	FindClose (t_find_handle);
	return t_entries.TakeAsProperList (r_native_entries);

 error_cleanup:
	FindClose (t_find_handle);
	return false;
}

bool
__MCSFileGetType (MCStringRef p_native_path,
                  bool p_follow_links,
                  MCSFileType & r_type)
{
	/* Get a system path */
	MCAutoStringRefAsWString t_path_w32;
	if (!t_path_w32.Lock (p_native_path))
		return false;

	DWORD t_attributes = GetFileAttributesW(*t_path_w32);

	if (INVALID_FILE_ATTRIBUTES == t_attributes)
	{
		return __MCSFileThrowIOErrorWithErrorCode (p_native_path, MCSTR("Failed to get attributes from file '%{path}': %{description}"), GetLastError());
	}

	/* Symbolic links are indicated by the reparse point flag.  But the other
	 * relevant attributes are still present. */
	if ((t_attributes & FILE_ATTRIBUTE_REPARSE_POINT) && !p_follow_links)
	{
		r_type = kMCSFileTypeSymbolicLink;
	}
	else if (t_attributes & FILE_ATTRIBUTE_DIRECTORY)
	{
		r_type = kMCSFileTypeDirectory;
	}
	else
	{
		r_type = kMCSFileTypeRegular;
	}

	return true;
}
