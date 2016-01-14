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

#include "em-system.h"
#include "em-filehandle.h"
#include "em-util.h"

#include "filedefs.h"
#include "mcstring.h"
#include "osspec.h"

#include <fcntl.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <emscripten.h>

/* ================================================================
 * System abstraction layer
 * ================================================================ */

/* Directories understood by MCEmscriptenSystem::GetStandardFolder() */
#define kMCEmscriptenStandardFolderEngine "/boot"
#define kMCEmscriptenStandardFolderResources "/boot/standalone"
#define kMCEmscriptenStandardFolderFonts "/boot/fonts"
#define kMCEmscriptenStandardFolderTemporary "/tmp"
#define kMCEmscriptenStandardFolderHome "/livecode"

/* ----------------------------------------------------------------
 * Utility functions
 * ---------------------------------------------------------------- */

/* Return a new Emscripten system interface instance */
MCSystemInterface *
MCDesktopCreateEmscriptenSystem(void)
{
	return new MCEmscriptenSystem;
}

/* Helper for resolving a path and converting to a system string in
 * one go, since this is needed very often. */
class MCEmscriptenAutoStringRefAsSysPath
	: public MCAutoStringRefAsSysString
{
public:
	bool LockPath(MCStringRef p_path)
	{
		MCAutoStringRef t_resolved;
		if (!MCS_resolvepath(p_path, &t_resolved)) {
			return false;
		}

		return Lock(*t_resolved);
	}
};

/* Helper for stat()-ing a file */
static inline bool
MCEmscriptenStatPath(MCStringRef p_path, struct stat & x_stat_buf)
{
	MCEmscriptenAutoStringRefAsSysPath t_path_sys;
	if (!t_path_sys.LockPath(p_path))
	{
		return false;
	}

	return (0 == stat(*t_path_sys, &x_stat_buf));
}

/* ----------------------------------------------------------------
 * Start up and tear down
 * ---------------------------------------------------------------- */

MCEmscriptenSystem::MCEmscriptenSystem()
{
}

MCEmscriptenSystem::~MCEmscriptenSystem()
{
}

bool
MCEmscriptenSystem::Initialize()
{
	IO_stdout = OpenFd(1, kMCOpenFileModeWrite);
	IO_stderr = OpenFd(2, kMCOpenFileModeWrite);

	return true;
}

void
MCEmscriptenSystem::Finalize()
{
}

/* ----------------------------------------------------------------
 * System error handling
 * ---------------------------------------------------------------- */

uint32_t
MCEmscriptenSystem::GetSystemError()
{
	return GetErrno();
}

int
MCEmscriptenSystem::GetErrno()
{
	return errno;
}

void
MCEmscriptenSystem::SetErrno(int p_errno)
{
	errno = p_errno;
}

/* ----------------------------------------------------------------
 * Debugging
 * ---------------------------------------------------------------- */

void
MCEmscriptenSystem::Debug(MCStringRef p_string)
{
	MCAutoStringRefAsUTF8String t_utf8_string;

	if (!t_utf8_string.Lock(p_string))
	{
		return;
	}

	emscripten_log(EM_LOG_CONSOLE, "%s", *t_utf8_string);
}

/* ----------------------------------------------------------------
 * Date & time
 * ---------------------------------------------------------------- */

real64_t
MCEmscriptenSystem::GetCurrentTime()
{
	struct timeval t_time;

	gettimeofday(&t_time, NULL);

	return t_time.tv_sec + (t_time.tv_usec / 1000000.0);
}

/* ----------------------------------------------------------------
 * System information
 * ---------------------------------------------------------------- */

bool
MCEmscriptenSystem::GetVersion(MCStringRef & r_string)
{
	return MCStringCopy(kMCEmptyString, r_string);
}

bool
MCEmscriptenSystem::GetMachine(MCStringRef & r_string)
{
	return MCStringCopy(kMCEmptyString, r_string);
}

MCNameRef
MCEmscriptenSystem::GetProcessor()
{
	return kMCEmptyName;
}

bool
MCEmscriptenSystem::GetAddress(MCStringRef & r_address)
{
	MCEmscriptenNotImplemented();
	return false;
}

MCServiceInterface *
MCEmscriptenSystem::QueryService(MCServiceType type)
{
	/* No Emscripten-specific services */
	return NULL;
}

/* ----------------------------------------------------------------
 * Engine process information
 * ---------------------------------------------------------------- */

uint32_t
MCEmscriptenSystem::GetProcessId()
{
	/* Emscripten usually returns 42.  The PID is a fairly meaningless
	 * value. */
	return getpid();
}

bool
MCEmscriptenSystem::GetExecutablePath(MCStringRef & r_path)
{
	MCEmscriptenNotImplemented();
	return false;
}

/* ----------------------------------------------------------------
 * Scheduling
 * ---------------------------------------------------------------- */

void
MCEmscriptenSystem::Alarm(real64_t p_when)
{
	/* According to Mark Waddingham, this is supposed to be a no-op.
	 * It's used for <Ctrl+.> processing, which isn't relevant on
	 * Emscripten. */
}

void
MCEmscriptenSystem::Sleep(real64_t p_when)
{
	MCEmscriptenNotImplemented();
}

/* ----------------------------------------------------------------
 * Environment variables
 * ---------------------------------------------------------------- */

/* Environment variables are meaningless when running on emscripten */

void
MCEmscriptenSystem::SetEnv(MCStringRef p_name,
                           MCStringRef p_value)
{
	/* No environment variables */
}

bool
MCEmscriptenSystem::GetEnv(MCStringRef p_name,
                           MCStringRef & r_value)
{
	/* No environment variables */
	r_value = MCValueRetain(kMCEmptyString);
	return true;
}

/* ----------------------------------------------------------------
 * Filesystem interface
 * ---------------------------------------------------------------- */

Boolean
MCEmscriptenSystem::CreateFolder(MCStringRef p_path)
{
	MCEmscriptenAutoStringRefAsSysPath t_path_sys;
	if (!t_path_sys.LockPath(p_path))
	{
		return false;
	}

	errno = 0;
	return (0 == mkdir(*t_path_sys, 0777));
}

Boolean
MCEmscriptenSystem::DeleteFolder(MCStringRef p_path)
{
	MCEmscriptenAutoStringRefAsSysPath t_path_sys;
	if (!t_path_sys.LockPath(p_path))
	{
		return false;
	}

	errno = 0;
	return (0 == rmdir(*t_path_sys));
}

Boolean
MCEmscriptenSystem::DeleteFile(MCStringRef p_path)
{
	MCEmscriptenAutoStringRefAsSysPath t_path_sys;
	if (!t_path_sys.LockPath(p_path))
	{
		return false;
	}

	errno = 0;
	return (0 == unlink(*t_path_sys));
}

Boolean
MCEmscriptenSystem::RenameFileOrFolder(MCStringRef p_old_name,
                                       MCStringRef p_new_name)
{
	MCEmscriptenAutoStringRefAsSysPath t_path_old, t_path_new;
	if (!t_path_old.LockPath(p_old_name))
	{
		return false;
	}
	if (!t_path_new.LockPath(p_new_name))
	{
		return false;
	}

	errno = 0;
	return (0 == rename(*t_path_old, *t_path_new));
}

Boolean
MCEmscriptenSystem::BackupFile(MCStringRef p_old_name,
                               MCStringRef p_new_name)
{
	/* FIXME Implement BackupFile */
	/* function not implemented */
	SetErrno(ENOSYS);
	return false;
}

Boolean
MCEmscriptenSystem::UnbackupFile(MCStringRef p_old_name,
                                 MCStringRef p_new_name)
{
	/* FIXME Implement UnbackupFile() */
	/* function not implemented */
	SetErrno(ENOSYS);
	return false;
}

Boolean
MCEmscriptenSystem::CreateAlias(MCStringRef p_target,
                                MCStringRef p_alias)
{
	/* FIXME Implement CreateAlias() using symlink(2) */
	/* The filesystem containing p_alias does not support the creation
	 * of aliases */
	SetErrno(ENOSYS);
	return false;
}

Boolean
MCEmscriptenSystem::ResolveAlias(MCStringRef p_target,
                                 MCStringRef & r_dest)
{
	/* FIXME Implement ResolveAlias() using readlink(2) */
	/* N.b. returns a standard path (i.e. not a native path) */
	return MCS_resolvepath(p_target, r_dest);
}

bool
MCEmscriptenSystem::GetCurrentFolder(MCStringRef & r_path)
{
	/* FIXME use get_current_dir_name() once it's available in Emscripten's
	 * libc. */

	/* We have to call getcwd() in a loop, expanding the output buffer
	 * if necessary.  Starting off with a PATH_MAX-sized buffer means
	 * that expanding is unlikely. */
	MCAutoArray<char> t_cwd_sys;

	/* Allocate initial buffer */
	if (!t_cwd_sys.New(PATH_MAX))
	{
		return false;
	}

	bool t_got_cwd = false;
	while (!t_got_cwd)
	{
		errno = 0;
		if (t_cwd_sys.Ptr() == getcwd(t_cwd_sys.Ptr(), t_cwd_sys.Size()))
		{
			t_got_cwd = true;
		}
		else if (errno == ERANGE)
		{
			/* The buffer wasn't large enough! */

			uindex_t t_new_size;
			/* Check that the buffer can actually be expanded */
			if (t_cwd_sys.Size() == UINDEX_MAX)
			{
				/* Buffer is already maximum size */
				return false;
			}
			else if (UINDEX_MAX / 2 < t_cwd_sys.Size())
			{
				t_new_size = UINDEX_MAX;
			}
			else
			{
				t_new_size = t_cwd_sys.Size() * 2;
			}

			/* Resize the buffer */
			if (!t_cwd_sys.Resize(t_new_size))
			{
				return false;
			}

			errno = 0;
		}
		else
		{
			return false;
		}
	}

	/* Convert to engine string */
	return MCStringCreateWithSysString(t_cwd_sys.Ptr(), r_path);
}

Boolean
MCEmscriptenSystem::SetCurrentFolder(MCStringRef p_path)
{
	MCEmscriptenAutoStringRefAsSysPath t_path_sys;
	if (!t_path_sys.LockPath(p_path))
	{
		return false;
	}

	errno = 0;
	return (0 == chdir(*t_path_sys));
}

bool
MCEmscriptenSystem::ListFolderEntries(MCStringRef p_folder,
                                      MCSystemListFolderEntriesCallback p_callback,
                                      void *x_context)
{
	/* Cope with legacy usage of ListFolderEntries() without a
	 * specific target directory path. */
	if (NULL == p_folder)
	{
		return ListFolderEntries(MCSTR("."), p_callback, x_context);
	}

	MCEmscriptenAutoStringRefAsSysPath t_path_sys;
	if (!t_path_sys.LockPath(p_folder))
	{
		return false;
	}

	bool t_success = true;

	/* Get a list of all the directory entries.  Using scandir() means
	 * we get reentrancy without having to fuss around figuring out
	 * how much memory to allocate. */
	/* FIXME use scandirat() once it's available in emscripten's libc */
	struct dirent **t_entries = NULL;
	int t_num_entries = -1;
	if (t_success)
	{
		errno = 0;
		t_num_entries = scandir(*t_path_sys, &t_entries, NULL, alphasort);
		t_success = (t_num_entries >= 0);
	}

	/* Create a buffer in which to build the full path to each entry in the
	 * directory.  It'll be expanded as we go. */
	size_t t_folder_len = strlen(*t_path_sys);

	/* May need an extra separator to ensure that the folder's path ends with
	 * a separator. */
	bool t_need_sep = ((*t_path_sys)[t_folder_len - 1] != '/');
	size_t t_base_len = t_folder_len + (t_need_sep ? 1 : 0);

	MCAutoArray<char> t_entry_path_sys;
	if (t_success)
	{
		/* Overflow check (allowing space for trailing null) */
		t_success = (t_base_len < UINDEX_MAX - 1);
	}
	if (t_success)
	{
		/* Allocate buffer */
		t_success = t_entry_path_sys.New(t_base_len + 1);
	}
	if (t_success)
	{
		/* Copy in base path */
		MCMemoryCopy(t_entry_path_sys.Ptr(), *t_path_sys, t_folder_len);
		/* Ensure trailing separator */
		t_entry_path_sys[t_folder_len] = '/';
		/* Ensure nul termination */
		t_entry_path_sys[t_base_len] = '\0';
	}


	/* Check the properties of each entry in the directory */
	for (int i = 0; i < t_num_entries && t_success; ++i)
	{
		const char *t_entry_name = t_entries[i]->d_name;

		/* Skip directory entries with null names */
		if (NULL == t_entry_name)
		{
			continue;
		}

		/* Skip the current directory itself */
		if (MCCStringEqual(t_entry_name, "."))
		{
			continue;
		}

		/* Build the full path to each entry.  We may need to expand the buffer. */
		size_t t_entry_name_len = strlen(t_entry_name);
		if (t_entry_name_len > t_entry_path_sys.Size() - t_base_len - 1)
		{
			/* Overflow check */
			if (t_entry_name_len > UINDEX_MAX - t_base_len - 1)
			{
				t_success = false;
				continue;
			}

			/* Expand buffer */
			if (!t_entry_path_sys.Extend(t_base_len + t_entry_name_len + 1))
			{
				t_success = false;
				continue;
			}
		}
		/* Copy in entry name after base path */
		MCMemoryCopy(t_entry_path_sys.Ptr() + t_base_len,
		             t_entry_name, t_entry_name_len);
		/* Ensure nul termination */
		t_entry_path_sys[t_base_len + t_entry_name_len] = '\0';

		struct stat t_stat_buf;

		/* This is sort of racy because the directory being scanned
		 * could move between scandir() and fstat(). */
		errno = 0;
		if (0 != stat(t_entry_path_sys.Ptr(), &t_stat_buf))
		{
			/* If the directory entry has gone away (e.g. because it's
			 * been deleted) just skip it. */
			t_success = (errno == ENOENT);
			continue;
		}

		MCSystemFolderEntry t_engine_entry;
		if (!MCStringCreateWithSysString(t_entry_name, t_engine_entry.name))
		{
			t_success = false;
			continue;
		}

		t_engine_entry.data_size = t_stat_buf.st_size;
		t_engine_entry.modification_time = uint32_t(t_stat_buf.st_mtime);
		t_engine_entry.access_time = uint32_t(t_stat_buf.st_atime);
		t_engine_entry.group_id = t_stat_buf.st_gid;
		t_engine_entry.user_id = t_stat_buf.st_uid;
		t_engine_entry.permissions = t_stat_buf.st_mode & 0777;
		t_engine_entry.is_folder = S_ISDIR(t_stat_buf.st_mode);

		/* Run callback */
		t_success = p_callback(x_context, &t_engine_entry);

		MCValueRelease (t_engine_entry.name);
	}

	/* Clean up the buffer allocated by scandir() */
	if (NULL != t_entries)
	{
		free(t_entries);
	}

	return t_success;
}

/* Resolve the path of a standard system directory, by name.
 *
 * Normally, sets r_folder to the native filesystem path of the
 * standard system directory named by p_type, and returns true.
 *
 * If p_type is not recognised, or its corresponding filesystem path
 * does not exist on the current system, sets r_folder to the empty
 * string and returns true.
 *
 * If an unexpected error occurs, returns false.
 */
Boolean
MCEmscriptenSystem::GetStandardFolder(MCNameRef p_type,
                                      MCStringRef & r_folder)
{
	const char *t_path_chars;
	MCAutoStringRef t_path;

	/* Decode the requested standard folder name */
	if (MCNameIsEqualTo(p_type, MCN_engine))
	{
		t_path_chars = kMCEmscriptenStandardFolderEngine;
	}
	else if (MCNameIsEqualTo(p_type, MCN_resources))
	{
		t_path_chars = kMCEmscriptenStandardFolderResources;
	}
	else if (MCNameIsEqualTo(p_type, MCN_temporary))
	{
		t_path_chars = kMCEmscriptenStandardFolderTemporary;
	}
	else if (MCNameIsEqualTo(p_type, MCN_fonts))
	{
		t_path_chars = kMCEmscriptenStandardFolderFonts;
	}
	else if (MCNameIsEqualTo(p_type, MCN_home))
	{
		t_path_chars = kMCEmscriptenStandardFolderHome;
	}
	else
	{
		/* We don't know anything about the requested folder */
		return MCStringCopy(kMCEmptyString, r_folder);
	}

	if (!MCStringCreateWithCString(t_path_chars, &t_path))
	{
		return false;
	}

	/* Check that the directory actually exists */
	if (!FolderExists(*t_path))
	{
		return MCStringCopy(kMCEmptyString, r_folder);
	}

	return MCStringCopy(*t_path, r_folder);
}

real64_t
MCEmscriptenSystem::GetFreeDiskSpace()
{
	/* FIXME Implement GetFreeDiskSpace() */
	/* The filesystem does not support this call */
	SetErrno(ENOSYS);
	return 0.0;
}

Boolean
MCEmscriptenSystem::GetDevices(MCStringRef & r_devices)
{
	/* FIXME Implement GetDevices() */
	/* No devices */
	r_devices = MCValueRetain(kMCEmptyString);
	return true;
}

Boolean
MCEmscriptenSystem::GetDrives(MCStringRef & r_drives)
{
	/* FIXME Implement GetDrives() */
	/* No drives */
	SetErrno(0);
	r_drives = MCValueRetain(kMCEmptyString);
	return true;
}

Boolean
MCEmscriptenSystem::FileExists(MCStringRef p_path)
{
	struct stat t_stat_buf;
	errno = 0;
	if (!MCEmscriptenStatPath(p_path, t_stat_buf))
	{
		/* If the error is ENOENT, then consume the error; we've
		 * successfully detected that the file does not exist. */
		if (errno == ENOENT)
		{
			errno = 0;
		}
		return false;
	}

	return S_ISREG(t_stat_buf.st_mode);
}

Boolean
MCEmscriptenSystem::FolderExists(MCStringRef p_path)
{
	struct stat t_stat_buf;
	errno = 0;
	if (!MCEmscriptenStatPath(p_path, t_stat_buf))
	{
		/* If the error is ENOENT, then consume the error; we've
		 * successfully detected that the file does not exist. */
		if (errno == ENOENT)
		{
			errno = 0;
		}
		return false;
	}

	return S_ISDIR(t_stat_buf.st_mode);
}

Boolean
MCEmscriptenSystem::FileNotAccessible(MCStringRef p_path)
{
	/* This is only ever used to introduce TOCTTOU errors, so just
	 * pretend that files are always present/accessible. */
	return false;
}

Boolean
MCEmscriptenSystem::ChangePermissions(MCStringRef p_path, uint16_t p_mask)
{
	MCEmscriptenAutoStringRefAsSysPath t_path_sys;
	if (!t_path_sys.LockPath(p_path))
	{
		return false;
	}

	errno = 0;
	return (0 == chmod(*t_path_sys, p_mask));
}

uint16_t
MCEmscriptenSystem::UMask(uint16_t p_mask)
{
	return umask(p_mask);
}

IO_handle
MCEmscriptenSystem::OpenFile(MCStringRef p_path,
                             intenum_t p_mode,
                             Boolean p_map)
{
	MCAutoStringRefAsUTF8String t_path_sys;
	if (!t_path_sys.Lock(p_path))
	{
		return NULL;
	}

	int t_fd = -1;
	int t_open_flags = 0;

	switch (p_mode)
	{
	case kMCOpenFileModeRead:
		t_open_flags = O_RDONLY;
		break;
	case kMCOpenFileModeWrite:
		t_open_flags = O_WRONLY | O_TRUNC | O_CREAT;
		break;
	case kMCOpenFileModeUpdate:
		t_open_flags = O_RDWR | O_CREAT;
		break;
	case kMCOpenFileModeAppend:
		t_open_flags = O_RDWR | O_APPEND | O_CREAT;
		break;
	default:
		MCUnreachable();
		return NULL;
	}

	errno = 0;
	t_fd = open(*t_path_sys, t_open_flags, 0666);
	if (0 > t_fd)
	{
		return NULL;
	}

	return new MCEmscriptenFileHandle(t_fd);
}

IO_handle
MCEmscriptenSystem::OpenFd(uint32_t p_fd,
                           intenum_t p_mode)
{
	return new MCEmscriptenFileHandle(p_fd);
}

IO_handle
MCEmscriptenSystem::OpenDevice(MCStringRef p_path,
                               intenum_t p_mode)
{
	/* FIXME Implement OpenDevice() */
	MCEmscriptenNotImplemented();
	return NULL;
}

bool
MCEmscriptenSystem::GetTemporaryFileName(MCStringRef & r_tmp_name)
{
	/* Never implement GetTemporaryFileName(). No-one should use it
	 * anyway due to inherent race conditions, and it should be
	 * removed from the language eventually. */
	return false;
}

bool
MCEmscriptenSystem::PathToNative(MCStringRef p_path,
                                 MCStringRef & r_path)
{
	/* Emscripten paths are internal paths */
	return MCStringCopy(p_path, r_path);
}

bool
MCEmscriptenSystem::PathFromNative(MCStringRef p_native_path,
                                   MCStringRef & r_path)
{
	/* Emscripten paths are internal paths */
	return MCStringCopy(p_native_path, r_path);
}

bool
MCEmscriptenSystem::ResolvePath(MCStringRef p_path,
                                MCStringRef & r_resolved_path)
{
	/* FIXME Implement ResolvePath() */
	/* This should do the following:
	 * 1) Expand user prefix:
	 *    a) If the prefix is ~/, expand to current user home dir
	 *    b) If the prefix is ~foo/, and foo is a user, expand to its home dir
	 *    c) If the prefix is ~foo/, and foo isn't a user, leave unchanged
	 * 2) Make absolute, by prepending CWD if necessary
	 * 3) Follow all symlinks
	 *
	 * If a system error occurs (e.g. due to a dangling symlink),
	 * return the "best" expansion found so far.
	 */

	/* ---------- 1. Expand user prefix */
	MCAutoStringRef t_user_expand;

	/* Check for a user prefix */
	if (MCStringBeginsWithCString(p_path, (const char_t *) "~",
	                              kMCStringOptionCompareExact))
	{
		/* Split into user part and rest of path */
		MCAutoStringRef t_user_part;
		MCAutoStringRef t_rel_part;
		uindex_t t_sep_offset;
		bool t_have_rel_part;

		if (MCStringFirstIndexOfChar(p_path, '/', 0,
		                             kMCStringOptionCompareExact,
		                             t_sep_offset))
		{
			t_have_rel_part = true;

			if (!MCStringDivideAtIndex(p_path, t_sep_offset,
			                           &t_user_part, &t_rel_part))
			{
				return false;
			}
		}
		else
		{
			t_have_rel_part = false;
			t_user_part = p_path;
			t_rel_part = kMCEmptyString;
		}

		/* At the moment, no user exists other than the current user,
		 * whose home directory is "/livecode" */
		if (MCStringIsEqualToCString(*t_user_part, "~",
		                             kMCStringOptionCompareExact) ||
		    MCStringIsEqualToCString(*t_user_part, "~livecode",
		                             kMCStringOptionCompareExact))
		{
			if (t_have_rel_part)
			{
				if (!MCStringFormat(&t_user_expand, "%s/%@",
				                    kMCEmscriptenStandardFolderHome,
				                    *t_rel_part))
				{
					return false;
				}
			}
			else
			{
				t_user_expand = MCSTR(kMCEmscriptenStandardFolderHome);
			}
		}
		else
		{
			/* User prefix doesn't correspond to an actual username */
			t_user_expand = p_path;
		}
	}
	else
	{
		/* No user prefix */
		t_user_expand = p_path;
	}

	/* ---------- 2. Make absolute */
	MCAutoStringRef t_absolute;

	if (MCStringBeginsWithCString(*t_user_expand, (const char_t *) "/",
	                              kMCStringOptionCompareExact))
	{
		/* Absolute paths always begin with "/" */
		if (!MCStringCopy(*t_user_expand, &t_absolute))
		{
			return false;
		}
	}
	else
	{
		/* Make absolute by prepending the CWD, adding a separator if
		 * necessary */
		MCAutoStringRef t_cwd;
		if (!GetCurrentFolder(&t_cwd))
		{
			/* System error -> return "best" expansion */
			return MCStringCopy(*t_user_expand, r_resolved_path);
		}

		const char *t_fmt;
		if (MCStringEndsWithCString(*t_cwd, (const char_t *) "/",
		                            kMCStringOptionCompareExact))
		{
			t_fmt = "%@%@";
		}
		else
		{
			t_fmt = "%@/%@";
		}

		if (!MCStringFormat(&t_absolute, t_fmt, *t_cwd, *t_user_expand))
		{
			return false;
		}
	}

	/* ---------- 3. Follow all symlinks */

	MCAutoStringRefAsSysString t_absolute_sys;
	if (!t_absolute_sys.Lock(*t_absolute))
	{
		return false;
	}

	/* Use realpath(3) to recursively resolve symlinks */
	errno = 0;
	char *t_realpath_buf = realpath(*t_absolute_sys, NULL);
	if (NULL == t_realpath_buf)
	{
		/* System error -> return "best" expansion */
		return MCStringCopy(*t_absolute, r_resolved_path);
	}

	MCAutoStringRef t_realpath;
	bool t_realpath_ok = MCStringCreateWithSysString(t_realpath_buf,
	                                                 &t_realpath);

	free(t_realpath_buf);

	if (!t_realpath_ok)
	{
		return false;
	}


	return MCStringCopy(*t_realpath, r_resolved_path);
}

bool
MCEmscriptenSystem::LongFilePath(MCStringRef p_path,
                                 MCStringRef & r_long_path)
{
	return MCStringCopy(p_path, r_long_path);
}

bool
MCEmscriptenSystem::ShortFilePath(MCStringRef p_path,
                                  MCStringRef & r_short_path)
{
	return MCStringCopy(p_path, r_short_path);
}

/* ----------------------------------------------------------------
 * Text codecs
 * ---------------------------------------------------------------- */

uint32_t
MCEmscriptenSystem::TextConvert(const void *p_string,
                                uint32_t p_string_length,
                                void *r_buffer,
                                uint32_t p_buffer_length,
                                uint32_t p_from_charset,
                                uint32_t p_to_charset)
{
	MCEmscriptenNotImplemented();
	return 0;
}

bool
MCEmscriptenSystem::TextConvertToUnicode(uint32_t p_input_encoding,
                                         const void *p_input,
                                         uint32_t p_input_length,
                                         void *p_output,
                                         uint32_t & p_output_length,
                                         uint32_t & r_used)
{
	MCEmscriptenNotImplemented();
	return false;
}

/* ----------------------------------------------------------------
 * Loadable modules
 * ---------------------------------------------------------------- */

/* Emscripten doesn't support dynamic linking */

MCSysModuleHandle
MCEmscriptenSystem::LoadModule(MCStringRef p_path)
{
	MCEmscriptenNotImplemented();
	return NULL;
}

MCSysModuleHandle
MCEmscriptenSystem::ResolveModuleSymbol(MCSysModuleHandle p_module,
                                        MCStringRef p_symbol)
{
	MCEmscriptenNotImplemented();
	return NULL;
}

void
MCEmscriptenSystem::UnloadModule(MCSysModuleHandle p_module)
{
	/* Successfully do nothing */
}

/* ----------------------------------------------------------------
 * Subprocesses
 * ---------------------------------------------------------------- */

/* Emscripten doesn't support subprocesses */

bool
MCEmscriptenSystem::Shell(MCStringRef p_filename,
                          MCDataRef & r_data,
                          int & r_retcode)
{
	return false;
}

void
MCEmscriptenSystem::CheckProcesses()
{
	/* Successfully do nothing */
}

bool
MCEmscriptenSystem::StartProcess(MCNameRef p_name,
                                 MCStringRef p_doc,
                                 intenum_t p_mode,
                                 Boolean p_elevated)
{
	return false;
}

void
MCEmscriptenSystem::CloseProcess(uint16_t p_index)
{
	/* Successfully do nothing */
}

void
MCEmscriptenSystem::Kill(int32_t p_pid,
                         int32_t p_signal)
{
	/* Successfully do nothing */
}

void
MCEmscriptenSystem::KillAll()
{
	/* Successfully do nothing */
}

Boolean
MCEmscriptenSystem::Poll(real64_t p_delay,
                         int p_fd)
{
	return false;
}

Boolean
MCEmscriptenSystem::IsInteractiveConsole(int p_fd)
{
	return false;
}

/* ----------------------------------------------------------------
 * Miscellanous
 * ---------------------------------------------------------------- */

void
MCEmscriptenSystem::LaunchDocument(MCStringRef p_document)
{
	/* Successfully do nothing */
}

void
MCEmscriptenSystem::LaunchUrl(MCStringRef p_document)
{
	/* Successfully do nothing */
}

void
MCEmscriptenSystem::DoAlternateLanguage(MCStringRef p_script,
                    MCStringRef p_language)
{
	/* Successfully do nothing */
}

bool
MCEmscriptenSystem::AlternateLanguages(MCListRef & r_list)
{
	/* No alternate languages available */
	return false;
}

bool
MCEmscriptenSystem::GetDNSservers(MCListRef & r_list)
{
	/* DNS servers aren't available */
	return false;
}
