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

#include "em-system.h"
#include "em-filehandle.h"
#include "em-util.h"

#include <fcntl.h>
#include <sys/time.h>
#include <emscripten.h>

/* ================================================================
 * System abstraction layer
 * ================================================================ */

/* ----------------------------------------------------------------
 * Utility functions
 * ---------------------------------------------------------------- */

/* Return a new Emscripten system interface instance */
MCSystemInterface *
MCDesktopCreateEmscriptenSystem(void)
{
	return new MCEmscriptenSystem;
}

static bool
IsRootFolder(MCStringRef p_path)
{
	return MCStringIsEqualToCString(p_path, "/", kMCStringOptionCompareExact);
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
	MCEmscriptenNotImplemented();
}

bool
MCEmscriptenSystem::GetMachine(MCStringRef & r_string)
{
	MCEmscriptenNotImplemented();
}

MCNameRef
MCEmscriptenSystem::GetProcessor()
{
	MCEmscriptenNotImplemented();
}

bool
MCEmscriptenSystem::GetAddress(MCStringRef & r_address)
{
	MCEmscriptenNotImplemented();
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
	/* FIXME Force test stack loading */
	if (MCStringIsEqualToCString(p_name,
	                             "TEST_STACK",
	                             kMCStringOptionCompareExact))
	{
		return MCStringCreateWithCString("/boot/boot.livecode", r_value);
	}

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
	/* FIXME Implement CreateFolder() -- using mkdir(2) */
	/* The filesystem containing p_path does not support the creation
	 * of directories. */
	SetErrno(EPERM);
	return false;
}

Boolean
MCEmscriptenSystem::DeleteFolder(MCStringRef p_path)
{
	/* FIXME Implement Deletefolder() -- using rmdir(2) */
	/* p_path does not exist */
	SetErrno(ENOENT);
	return false;
}

Boolean
MCEmscriptenSystem::RenameFileOrFolder(MCStringRef p_old_name,
                                       MCStringRef p_new_name)
{
	/* FIXME Implement RenameFileOrFolder() -- using rename(2) */
	/* p_old_name does not exist */
	SetErrno(ENOENT);
	return false;
}

Boolean
MCEmscriptenSystem::BackupFile(MCStringRef p_old_name,
                               MCStringRef p_new_name)
{
	/* FIXME Implement BackupFile */
	/* p_old_name does not exist */
	SetErrno(ENOENT);
	return false;
}

Boolean
MCEmscriptenSystem::UnbackupFile(MCStringRef p_old_name,
                                 MCStringRef p_new_name)
{
	/* FIXME Implement UnbackupFile() */
	/* p_old_name does not exist */
	SetErrno(ENOENT);
	return false;
}

Boolean
MCEmscriptenSystem::CreateAlias(MCStringRef p_target,
                                MCStringRef p_alias)
{
	/* FIXME Implement CreateAlias() using symlink(2) */
	/* The filesystem containing p_alias does not support the creation
	 * of aliases */
	SetErrno(EPERM);
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
	/* FIXME Implement GetCurrentFolder() */
	/* For now, always treat the root directory as current */
	return MCStringCreateWithCString("/", r_path);
}

Boolean
MCEmscriptenSystem::SetCurrentFolder(MCStringRef p_path)
{
	/* FIXME Implement SetCurrentFolder() */
	/* For now, can only chdir to the root directory */
	if (IsRootFolder(p_path))
		return true;

	SetErrno(ENOTDIR);
	return false;
}

bool
MCEmscriptenSystem::ListFolderEntries(MCSystemListFolderEntriesCallback p_callback,
                                      void *x_context)
{
	/* FIXME Implement ListFolderEntries() */
	return false;
}

Boolean
MCEmscriptenSystem::GetStandardFolder(MCNameRef p_type,
                                      MCStringRef & r_folder)
{
	/* FIXME Implement GetStandardFolder() */
	return false;
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
	/* FIXME Implement FileExists() -- using stat(2) */
	MCEmscriptenNotImplemented();
}

Boolean
MCEmscriptenSystem::FolderExists(MCStringRef p_path)
{
	/* FIXME Implement FolderExists() -- using stat(2) */
	MCEmscriptenNotImplemented();
}

Boolean
MCEmscriptenSystem::FileNotAccessible(MCStringRef p_path)
{
	/* FIXME Implement FileNotAccessible -- maybe using access(2) */
	MCEmscriptenNotImplemented();
}

Boolean
MCEmscriptenSystem::ChangePermissions(MCStringRef p_path, uint16_t p_mask)
{
	/* FIXME Implement ChangePermissions -- using chmod(2) */
	MCEmscriptenNotImplemented();
}

uint16_t
MCEmscriptenSystem::UMask(uint16_t p_mask)
{
	/* FIXME Implement UMask() -- using umask(2) */
	MCEmscriptenNotImplemented();
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
	case kMCOpenFileModeUpdate:
	case kMCOpenFileModeAppend:
		MCEmscriptenNotImplemented();
		return NULL;
	default:
		MCUnreachable();
	}

	errno = 0;
	t_fd = open(*t_path_sys, t_open_flags);
	if (-1 == t_fd)
	{
		return NULL;
	}

	return new MCEmscriptenFileHandle(t_fd);
}

IO_handle
MCEmscriptenSystem::OpenFd(uint32_t p_fd,
                           intenum_t p_mode)
{
	/* FIXME Implement OpenFd() */
	MCEmscriptenNotImplemented();
}

IO_handle
MCEmscriptenSystem::OpenDevice(MCStringRef p_path,
                               intenum_t p_mode)
{
	/* FIXME Implement OpenDevice() */
	MCEmscriptenNotImplemented();
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

	/* At the moment, no user exists other than the current user,
	 * whose home directory is "/" */

	if (MCStringIsEqualToCString(p_path, "~", kMCStringOptionCompareExact))
	{
		return MCStringCreateWithCString("/", r_resolved_path);
	}
	else if (MCStringBeginsWithCString(p_path, (const char_t *) "~/",
	                                   kMCStringOptionCompareExact))
	{
		if (!MCStringCopySubstring(p_path, MCRangeMake(1, UINDEX_MAX), &t_user_expand))
		{
			return false;
		}
	}
	else
	{
		if (!MCStringCopy(p_path, &t_user_expand))
		{
			return false;
		}
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
			return false;
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
	/* No symlinks! */

	return MCStringCopy(*t_absolute, r_resolved_path);
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
}

/* ----------------------------------------------------------------
 * Loadable modules
 * ---------------------------------------------------------------- */

/* Emscripten doesn't support dynamic linking */

MCSysModuleHandle
MCEmscriptenSystem::LoadModule(MCStringRef p_path)
{
	MCEmscriptenNotImplemented();
}

MCSysModuleHandle
MCEmscriptenSystem::ResolveModuleSymbol(MCSysModuleHandle p_module,
                                        MCStringRef p_symbol)
{
	MCEmscriptenNotImplemented();
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
LaunchDocument(MCStringRef p_document)
{
	/* Successfully do nothing */
}

void
LaunchUrl(MCStringRef p_document)
{
	/* Successfully do nothing */
}

void
DoAlternateLanguage(MCStringRef p_script,
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
