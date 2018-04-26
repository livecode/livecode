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

#include "system-private.h"

#include <foundation-auto.h>

#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <dirent.h>

/* ================================================================
 * POSIX whole-file IO
 * ================================================================ */

/* If the file is a regular file, use the read(2) system call to read
 * the whole file into a preallocated buffer of the correct size.
 * This is the preferred and most efficient option. */
static bool
__MCSFileGetContentsRegular (MCStringRef p_native_path,
                             struct stat *p_stat_buf,
                             int x_fd,
                             byte_t *& r_contents,
                             size_t & r_length)
{
	MCAssert (p_stat_buf);

	bool t_success = true;
	size_t t_size = p_stat_buf->st_size;

	/* Allocate the read buffer */
	size_t t_alloc_size = t_size;
	byte_t *t_buffer;

	if (!MCMemoryAllocate (t_alloc_size, t_buffer))
		return false;

	/* Read in a loop */
	size_t t_total_read = 0;
	while (t_success && t_total_read < t_size)
	{
		ssize_t t_read_len;

		t_read_len = read (x_fd, t_buffer + t_total_read, t_size - t_total_read);
		t_total_read += t_read_len;

		if (t_read_len == 0)
			break;
		else if (t_read_len < 0)
		{
			int t_save_errno = errno;
			t_success = __MCSFileThrowReadErrorWithErrno (p_native_path, t_save_errno);
		}
	}

	if (t_success)
	{
		r_contents = t_buffer;
		r_length = t_total_read;
	}
	else
	{
		MCMemoryDeallocate (t_buffer);
	}

	/* UNCHECKED */ close (x_fd);

	return t_success;
}

/* If the file is not a regular file, use the cstdio API to read the
 * file in incremental chunks into a buffer that grows as necessary
 * until the end of the the file has been found.  This is much less
 * efficient than __MCSFileGetContentsRegular(); it will usually
 * require multiple memory reallocations and memory copies. */
static bool
__MCSFileGetContentsStream (MCStringRef p_native_path,
                            FILE *p_cstream,
                            byte_t *& r_contents,
                            size_t & r_length)
{
	MCAssert (p_cstream);

	bool t_success = true;

	byte_t t_read_buffer[4096];
	size_t t_read_len;

	size_t t_alloc_size = 0;
	byte_t *t_buffer = NULL;

	size_t t_total_read = 0;
	while (t_success && !feof (p_cstream))
	{
		/* Read the next chunk of data from the stream */
		t_read_len = fread (t_read_buffer,
		                    1,
		                    sizeof (t_read_buffer),
		                    p_cstream);
		int t_save_errno = errno;

		/* Overflow protection.  Note this test must be in this form
		 * because t_total_read + t_read_len may be too large to fit
		 * in a size_t */
		if (t_total_read > SIZE_MAX - t_read_len)
			goto file_too_large;

		/* Expand the output buffer */
		while (t_success && t_total_read + t_read_len > t_alloc_size)
		{
			/* Check how much to expand by */
			if (t_buffer)
			{
				if (t_alloc_size > SIZE_MAX / 2)
					goto file_too_large;
				t_alloc_size *= 2;
			}
			else
			{
				t_alloc_size = MCMin (t_read_len, sizeof (t_read_buffer));
			}

			if (t_success)
				t_success = MCMemoryReallocate (t_buffer, t_alloc_size,
				                                t_buffer);
		}

		if (t_success && ferror (p_cstream))
		{
			t_success = __MCSFileThrowReadErrorWithErrno (p_native_path, t_save_errno);
		}

		if (t_success && 0 == t_read_len)
			break;

		/* Copy read bytes onto the end of the result buffer */
		MCAssert (t_buffer);
		MCMemoryCopy (t_buffer + t_total_read, t_read_buffer, t_read_len);
		t_total_read += t_read_len;
	}

	/* UNCHECKED */ fclose (p_cstream);

	if (t_success)
	{
		r_contents = t_buffer;
		r_length = t_total_read;
	}
	else
	{
		MCMemoryDeallocate (t_buffer);
	}

	return t_success;

 file_too_large:
	MCMemoryDeallocate (t_buffer);
	/* UNCHECKED */ fclose (p_cstream);
	return __MCSFileThrowIOErrorWithErrno (p_native_path, MCSTR("File '%{path}' is too large"), 0);
}

bool
__MCSFileGetContents (MCStringRef p_native_path,
                      MCDataRef & r_data)
{
	errno = 0;

	/* Get a system path */
	MCAutoStringRefAsSysString t_path_sys;
	if (!t_path_sys.Lock(p_native_path))
		return false;

	byte_t *t_buffer = NULL;
	size_t t_length;

	/* Open a fd for reading */
	int t_fd;
	t_fd = open (*t_path_sys, O_RDONLY);
	if (t_fd < 0)
	{
		int t_save_errno = errno;
		return __MCSFileThrowOpenErrorWithErrno (p_native_path, t_save_errno);
	}

	/* If the file is a regular file of finite size, read it in one
	 * block using low-level fd calls.  Otherwise, use a cstdio stream
	 * to read the data incrementally. */
	struct stat t_stat_buf;
	if (fstat (t_fd, &t_stat_buf) < 0)
	{
		int t_save_errno = errno;
		close (t_fd);
		return __MCSFileThrowIOErrorWithErrno (p_native_path, MCSTR("Failed to get the attributes of file '%{path}': %{description}"), t_save_errno);
	}

	if (t_stat_buf.st_size > 0 && S_ISREG (t_stat_buf.st_mode))
	{
		if (!__MCSFileGetContentsRegular (p_native_path, &t_stat_buf, t_fd,
		                                  t_buffer, t_length))
			return false; /* Error should already be set */
	}
	else
	{
		FILE *t_cstream;

		t_cstream = fdopen (t_fd, "r");
		if (NULL == t_cstream)
		{
			int t_save_errno = errno;
			return __MCSFileThrowIOErrorWithErrno (p_native_path, MCSTR ("Failed to open file '%{path}': fdopen() failed: %{description}"), t_save_errno);
		}

		if (!__MCSFileGetContentsStream (p_native_path, t_cstream,
		                                 t_buffer, t_length))
			return false; /* Error should already be set */
	}

	/* Store the data in a dataref */
	bool t_success;
	if (0 == t_length || NULL == t_buffer)
	{
		r_data = MCValueRetain (kMCEmptyData);
		t_success = true;
	}
	else
	{
		t_success = MCDataCreateWithBytesAndRelease (t_buffer, t_length,
		                                             r_data);
		t_buffer = NULL;
	}

	MCMemoryDeallocate (t_buffer);
	return t_success;
}

bool
__MCSFileSetContents (MCStringRef p_native_path,
                      MCDataRef p_data)
{
	bool t_success = true;

	/* Set the contents of a file in two steps:
	 *
	 * 1) Write a temporary file
	 * 2) Rename the temporary file into place
	 */

	errno = 0;

	/* Securely create a temporary file */
	MCAutoStringRef t_temp_native_path;
	if (!MCStringFormat (&t_temp_native_path, "%@.XXXXXX", p_native_path))
		return false;

	MCAutoStringRefAsSysString t_temp_path_sys;
	if (!t_temp_path_sys.Lock (*t_temp_native_path))
		return false;

	/* Save the umask, and set it to forbid group/other access to new
	 * files.  We'll restore it after calling mkstemp. */
	mode_t t_umask = ~(S_IRUSR | S_IWUSR);
	t_umask = umask (t_umask);

	/* mkstemp() modifies its argument.  The char array belongs to the
	 * MCAutoStringRefAsSysString instance, so this is a bit of a hack
	 * that depends on the char array not being shared. */
	int t_temp_fd;
	t_temp_fd = mkstemp ((char *) *t_temp_path_sys);
	int t_save_errno = errno;

	/* Restore the umask to its original value */
	umask (t_umask);

	if (0 > t_temp_fd)
	{
		return __MCSFileThrowIOErrorWithErrno (*t_temp_native_path, MCSTR("Failed to create temporary file '%{path}': %s"), t_save_errno);
	}

	/* Write the data into the temporary file */
	size_t t_total_written = 0;
	size_t t_amount = MCDataGetLength (p_data);
	const byte_t *t_buffer = MCDataGetBytePtr (p_data);

	while (t_success && t_total_written < t_amount)
	{
		ssize_t t_written_len;

		t_written_len = write (t_temp_fd, t_buffer + t_total_written,
		                       t_amount - t_total_written);
		if (t_written_len < 0)
		{
			t_success = __MCSFileThrowWriteErrorWithErrno (*t_temp_native_path, errno);
		}
		t_total_written += t_written_len;
	}

	/* Set the mode.  The default permissions are the same as if the
	 * file had been created by opening a stream for writing,
	 * i.e. 0666 modified by the effective umask. */
	mode_t t_mode;

	t_mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;
	t_mode &= ~t_umask;

	if (t_success)
	{
		if (0 != fchmod (t_temp_fd, t_mode))
		{
			t_success = __MCSFileThrowIOErrorWithErrno (*t_temp_native_path, MCSTR("Failed to set permissions of file '%{path}': %s"), errno);
		}
	}

	if (!t_success)
	{
		/* UNCHECKED */ close (t_temp_fd);
		/* UNCHECKED */ unlink (*t_temp_path_sys);
		return false;
	}

	/* If the target file already exists and is > 0 bytes, sync the
	 * newly-written file to ensure that the data is on disk when we
	 * rename over the destination.  Otherwise, if a system crash
	 * occurs we can lose both the old and new files on some
	 * filesystems. */

	/* Get a system path */
	MCAutoStringRefAsSysString t_path_sys;
	if (!t_path_sys.Lock(p_native_path))
		return false;

	struct stat t_target_statbuf;
	if (0 == lstat (*t_path_sys, &t_target_statbuf) &&
	    t_target_statbuf.st_size > 0)
	{
		if (0 != fsync (t_temp_fd))
		{
			return __MCSFileThrowIOErrorWithErrno (*t_temp_native_path, MCSTR("Failed to write to file %{path}; fsync() failed: %{description}"), errno);
		}
	}

	if (0 != close (t_temp_fd))
	{
		/* UNCHECKED */ unlink (*t_temp_path_sys);
		return false; /* FIXME Should we throw an error here? */
	}

	/* Rename the new file into place */

	if (0 != rename (*t_temp_path_sys, *t_path_sys))
	{
		t_save_errno = errno;
        
        /* UNCHECKED */ unlink (*t_temp_path_sys);
        errno = t_save_errno;
        
		/* Report rename error */
		MCAutoStringRef t_description, t_path, t_temp_path;
		MCAutoNumberRef t_error_code;
		/* UNCHECKED */ MCStringCreateWithCString (strerror (t_save_errno),
		                                           &t_description);
		/* UNCHECKED */ MCNumberCreateWithInteger (t_save_errno,
		                                           &t_error_code);
		/* UNCHECKED */ __MCSFilePathFromNative (p_native_path, &t_path);
		/* UNCHECKED */ __MCSFilePathFromNative (*t_temp_native_path,
		                                         &t_temp_path);

		return MCErrorCreateAndThrowWithMessage (
		           kMCSFileIOErrorTypeInfo,
		           MCSTR("Failed to rename file '%{temp_path}' to '%{path}': %{description}"),
		           "path", *t_path,
		           "temp_path", *t_temp_path,
		           "description", *t_description,
		           "error_code", *t_error_code,
		           NULL);
	}

	return true;
}

/* ================================================================
 * Path manipulation
 * ================================================================ */

/* We require filenames to be able to be round-tripped to a system
 * string and back again in order to be validly converted to a native
 * filename.  We also require the system string encoding to contain no
 * null bytes, because these null bytes are used as terminal
 * characters by POSIX APIs. */

static bool
__MCSFilePathValidate (MCStringRef p_path)
{
	if (MCStringIsEmpty (p_path))
		return __MCSFileThrowInvalidPathError (p_path);

#if !defined(__LINUX__)

	/* On non-Linux platforms, assume UTF-8 filename encoding.  We
	 * don't need to do a full roundtrip encoding because all
	 * MCStringRefs should be able to be perfectly represented in
	 * UTF-8. */
	MCAutoDataRef t_encoded;

	/* Assume that if this fails it's because of a memory error. */
	if (!MCStringEncode (p_path, kMCStringEncodingUTF8,
	                     false, &t_encoded))
		return false;

	/* Verify that the encoded representation contains no null bytes */
	uindex_t t_encoded_length;
	t_encoded_length = MCDataGetLength (*t_encoded);
	for (uindex_t i = 0; i < t_encoded_length; ++i)
	{
		byte_t t_byte = MCDataGetByteAtIndex (*t_encoded, i);
		if (t_byte == 0)
			return __MCSFileThrowInvalidPathError (p_path);
	}

#else /* __LINUX__ */

	/* On Linux, things are more complicated.  Currently, we merely
	 * check that we can round-trip to the current locale's encoding.
	 * However, arguably we should step through the elements of the
	 * canonicalized path and check that each element cleanly encodes
	 * to the corresponding filesystem's encoding. */
	MCAutoStringRefAsSysString t_path_sys;
	MCAutoStringRef t_roundtrip;

	/* Assume that if this fails it's because of a memory error. */
	if (!t_path_sys.Lock (p_path))
		return false;
	if (!MCStringCreateWithSysString (*t_path_sys, &t_roundtrip))
		return false;

	/* Verify the roundtrip conversion.  Note that if the original
	 * string contained a null somewhere, the roundtripped string will
	 * be shorter than the input string. */
	if (!MCStringIsEqualTo (p_path, *t_roundtrip, kMCStringOptionCompareExact))
	{
		return __MCSFileThrowInvalidPathError (p_path);
	}

#endif /* __LINUX __ */

	return true;
}

/* On POSIX platforms, the native path representation should match the
 * LiveCode internal representation. */

bool
__MCSFilePathToNative (MCStringRef p_path,
                       MCStringRef & r_native_path)
{
	return
		__MCSFilePathValidate (p_path) &&
		MCStringCopy (p_path, r_native_path);
}

bool
__MCSFilePathFromNative (MCStringRef p_native_path,
                         MCStringRef & r_path)
{
	return MCStringCopy (p_native_path, r_path);
}

bool
__MCSFileGetCurrentDirectory (MCStringRef & r_native_path)
{
	/* Assume that we have a C library that will allocate a buffer when getcwd(3)
	 * is called with a NULL string buffer. */
	char *t_cwd_sys;
	errno = 0;
	t_cwd_sys = getcwd (NULL, 0);

	if (NULL == t_cwd_sys)
	{
		return __MCSFileThrowIOErrorWithErrno (kMCEmptyString, MCSTR("Failed to get current working directory: %{description}"), errno);
	}

	bool t_success = MCStringCreateWithSysString (t_cwd_sys, r_native_path);
	free (t_cwd_sys);
	return t_success;
}

bool
__MCSFilePathIsAbsolute (MCStringRef p_path)
{
	return MCStringBeginsWithCString(p_path, (const char_t *) "/",
	                                 kMCStringOptionCompareExact);
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
	MCAutoStringRefAsSysString t_path_sys;
	if (!t_path_sys.Lock(p_native_path))
		return false;

	/* Compute the flags to be passed to open() and fdopen() */
	int t_oflag = 0;
	const char *t_stream_mode;

	if (p_mode & kMCSFileOpenModeRead && p_mode & kMCSFileOpenModeWrite)
	{
		t_oflag |= O_RDWR | O_CREAT;
		t_stream_mode = (p_mode & kMCSFileOpenModeAppend) ? "a+" : "r+";
	}
	else if (p_mode & kMCSFileOpenModeRead)
	{
		t_oflag |= O_RDONLY;
		t_stream_mode = "r";
	}
	else if (p_mode & kMCSFileOpenModeWrite)
	{
		t_oflag |= O_WRONLY | O_CREAT;
		if (p_mode & kMCSFileOpenModeAppend)
		{
			t_stream_mode = "a";
		}
		else
		{
			t_stream_mode = "w";
			t_oflag |= O_TRUNC;
		}
	}
	else
	{
		/* We'll use fdopen(3) later; throw the same error that it
		 * would throw with an invalid open mode argument. */
		return __MCSFileThrowOpenErrorWithErrno (p_native_path, EINVAL);
	}

	if (p_mode & kMCSFileOpenModeWrite && p_mode & kMCSFileOpenModeCreate)
		t_oflag |= O_EXCL;

	/* Compute the default access poermissions. */
	mode_t t_mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;

	/* First open a file descriptor using open(2), and then convert it
	 * into a cstdio stream using fdopen(3).  This is necessary because
	 * there's no way to cause an error if the file already exists when
	 * creating it using fopen(3). */
	int t_fd = -1;
	FILE *t_cstream = NULL;

	if (t_success)
	{
		t_fd = open (*t_path_sys, t_oflag, t_mode);
		t_success = (t_fd >= 0);
	}

	if (t_success)
	{
		t_cstream = fdopen (t_fd, t_stream_mode);
		t_success = (NULL != t_cstream);
	}

	if (!t_success)
	{
		if (t_fd >= 0)
			close (t_fd);
		return false; /* FIXME proper error */
	}

	/* Store the newly created cstdio stream in a new MCStream instance. */
	MCStreamRef t_stream;
	if (!__MCSStreamCreateWithStdio (t_cstream, t_stream))
	{
		/* UNCHECKED */ fclose (t_cstream);
		return false;
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
	MCAutoStringRefAsSysString t_path_sys;
	if (!t_path_sys.Lock(p_native_path))
		return false;

	errno = 0;
	if (-1 == unlink (*t_path_sys))
		return __MCSFileThrowIOErrorWithErrno (p_native_path, MCSTR("Failed to delete file %{path}: %{description}"), errno);

	return true;
}

bool
__MCSFileCreateDirectory (MCStringRef p_native_path)
{
	/* Get a system path */
	MCAutoStringRefAsSysString t_path_sys;
	if (!t_path_sys.Lock(p_native_path))
		return false;

	/* The default permissions are full, but they're modified by the effective
	 * umask in the mkdir() call.*/
	mode_t t_mode = (S_IRUSR | S_IWUSR | S_IXUSR |
	                 S_IRGRP | S_IWGRP | S_IXGRP |
	                 S_IROTH | S_IWOTH | S_IXOTH);

	errno = 0;
	if (0 != mkdir (*t_path_sys, t_mode))
		return __MCSFileThrowIOErrorWithErrno (p_native_path, MCSTR("Failed to create directory %{path}: %{description}"), errno);

	return true;
}

bool
__MCSFileDeleteDirectory (MCStringRef p_native_path)
{
	/* Get a system path */
	MCAutoStringRefAsSysString t_path_sys;
	if (!t_path_sys.Lock(p_native_path))
		return false;

	if (0 != rmdir (*t_path_sys))
		return __MCSFileThrowIOErrorWithErrno (p_native_path, MCSTR("Failed to delete directory %{path}: %{description}"), errno);

	return true;
}

/* This function is used by __MCSFileGetDirectoryEntries() with
 * scandir(3) in order to filter out '.' and '..' from directory
 * listings. */
static int
__MCSFileGetDirectoryEntries_Filter (const struct dirent *p_entry)
{
	const char *p_name;

	MCAssert (p_entry);
	MCAssert (p_entry->d_name);

	p_name = p_entry->d_name;

	if (0 == strcmp (p_name, ".") ||
	    0 == strcmp (p_name, ".."))
		return 0;
	else
		return 1;
}

bool
__MCSFileGetDirectoryEntries (MCStringRef p_native_path,
                              MCProperListRef & r_native_entries)
{
	/* Get a system path */
	MCAutoStringRefAsSysString t_path_sys;
	if (!t_path_sys.Lock(p_native_path))
		return false;

	/* Use scandir(3) to retrieve a list of all directory entries. We
	 * don't use readdir(3) because it's not reentrant.  We don't use
	 * readdir_r(3) because it's complicated to calculate the amount
	 * of memory to allocate and if you use it wrong you get buffer
	 * overflows and other Bad Things. */
	int t_num_entries;
	struct dirent **t_raw_entries;

	errno = 0;
	t_num_entries = scandir (*t_path_sys, &t_raw_entries,
	                         __MCSFileGetDirectoryEntries_Filter,
	                         NULL);
	if (-1 == t_num_entries)
		return __MCSFileThrowIOErrorWithErrno (p_native_path, MCSTR("Failed get entries of directory %{path}: %{description}"), errno);

	/* Convert all directory entries' names to strings. N.b. the
	 * MCSFileGetDirectoryEntries() wrapper will convert them from
	 * native representation. */
	MCAutoStringRefArray t_entries;

	if (!t_entries.New (t_num_entries))
		goto error_cleanup;

	for (int i = 0; i < t_num_entries; ++i)
	{
#ifdef __LINUX__
		if (!MCStringCreateWithSysString (t_raw_entries[i]->d_name,
		                                  t_entries[i]))
#else
        if (!MCStringCreateWithBytes((const byte_t *)t_raw_entries[i]->d_name, strlen(t_raw_entries[i]->d_name), kMCStringEncodingUTF8, false, t_entries[i]))
#endif
			goto error_cleanup;
	}

	/* Finally return the constructed list. */
	free (t_raw_entries);
	return t_entries.TakeAsProperList (r_native_entries);

 error_cleanup:
	free (t_raw_entries);
	return false;
}

bool
__MCSFileGetType (MCStringRef p_native_path,
                  bool p_follow_links,
                  MCSFileType & r_type)
{
	/* Get a system path */
	MCAutoStringRefAsSysString t_path_sys;
	if (!t_path_sys.Lock (p_native_path))
		return false;

	struct stat t_stat_buf;
	int t_stat_result;

	if (p_follow_links)
	{
		t_stat_result = stat (*t_path_sys, &t_stat_buf);
	}
	else
	{
		t_stat_result = lstat (*t_path_sys, &t_stat_buf);
	}

	if (0 != t_stat_result)
	{
		return __MCSFileThrowIOErrorWithErrno (p_native_path, MCSTR("Failed to stat %{path}: %{description}"), errno);
	}

	switch (t_stat_buf.st_mode & S_IFMT)
	{
	case S_IFREG: r_type = kMCSFileTypeRegular;      break;
	case S_IFDIR: r_type = kMCSFileTypeDirectory;    break;
	case S_IFLNK: r_type = kMCSFileTypeSymbolicLink; break;
	default:      r_type = kMCSFileTypeUnsupported;  break;
	}

	return true;
}
