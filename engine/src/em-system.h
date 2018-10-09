/*                                                                     -*-c++-*-

Copyright (C) 2003-2015 LiveCode Ltd.

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

#ifndef __MC_EMSCRIPTEN_SYSTEM_H__
#define __MC_EMSCRIPTEN_SYSTEM_H__

#include <foundation.h>

#include "globdefs.h"
#include "sysdefs.h"
#include "system.h"

/* ================================================================
 * System abstraction layer
 * ================================================================ */

class MCEmscriptenSystem: public MCSystemInterface
{
public:
	MCEmscriptenSystem(void);
	virtual ~MCEmscriptenSystem(void);

	virtual MCServiceInterface *QueryService(MCServiceType type);

	/* ---------- Start up and tear down */
	virtual bool Initialize(void);
	virtual void Finalize(void);

	/* ---------- System error handling */
	virtual uint32_t GetSystemError(void);
	virtual int GetErrno(void);
	virtual void SetErrno(int p_errno);

	/* ---------- Debugging */
	virtual void Debug(MCStringRef p_string);

	/* ---------- Date & time */
	virtual real64_t GetCurrentTime(void);

	/* ---------- System information */
	virtual bool GetVersion(MCStringRef & r_string);
	virtual bool GetMachine(MCStringRef & r_string);
	virtual bool GetAddress(MCStringRef & r_address);

	/* ---------- Engine process information */
	virtual uint32_t GetProcessId(void);

	/* ---------- Scheduling */
	virtual void Alarm(real64_t p_when);
	virtual void Sleep(real64_t p_when);

	/* ---------- Environment variables */
	virtual void SetEnv(MCStringRef p_name, MCStringRef p_value);
	virtual bool GetEnv(MCStringRef p_name, MCStringRef & r_value);

	/* ---------- Filesystem interface */
	virtual Boolean CreateFolder(MCStringRef p_path);
	virtual Boolean DeleteFolder(MCStringRef p_path);

	virtual Boolean DeleteFile(MCStringRef p_path);

	virtual Boolean RenameFileOrFolder(MCStringRef p_old_name,
	                                MCStringRef p_new_name);

	virtual Boolean BackupFile(MCStringRef p_old_name, MCStringRef p_new_name);
	virtual Boolean UnbackupFile(MCStringRef p_old_name, MCStringRef p_new_name);

	virtual Boolean CreateAlias(MCStringRef p_target, MCStringRef p_alias);
	virtual Boolean ResolveAlias(MCStringRef p_target, MCStringRef & r_dest);

	virtual bool GetCurrentFolder(MCStringRef & r_path);
	virtual Boolean SetCurrentFolder(MCStringRef p_path);
	virtual bool ListFolderEntries(MCStringRef p_folder,
	                               MCSystemListFolderEntriesCallback p_callback,
	                               void *x_context);
	virtual Boolean GetStandardFolder(MCNameRef p_type, MCStringRef & r_folder);

	virtual real64_t GetFreeDiskSpace();

	virtual Boolean GetDevices(MCStringRef & r_devices);
	virtual Boolean GetDrives(MCStringRef & r_drives);

	virtual Boolean FileExists(MCStringRef p_path);
	virtual Boolean FolderExists(MCStringRef p_path);
	virtual Boolean FileNotAccessible(MCStringRef p_path);

	virtual Boolean ChangePermissions(MCStringRef p_path, uint16_t p_mask);
	virtual uint16_t UMask(uint16_t p_mask);

	virtual IO_handle OpenFile(MCStringRef p_path,
	                           intenum_t p_mode,
	                           Boolean p_map);
	virtual IO_handle OpenFd(uint32_t p_fd, intenum_t p_mode);
	virtual IO_handle OpenDevice(MCStringRef p_path, intenum_t p_mode);

	virtual bool GetTemporaryFileName(MCStringRef & r_tmp_name);

	virtual bool PathToNative(MCStringRef p_path, MCStringRef & r_path);
	virtual bool PathFromNative(MCStringRef p_native, MCStringRef & r_path);
	virtual bool ResolvePath(MCStringRef p_path, MCStringRef & r_resolved_path);

	virtual bool LongFilePath(MCStringRef p_path, MCStringRef & r_long_path);
	virtual bool ShortFilePath(MCStringRef p_path, MCStringRef & r_short_path);

	/* ---------- Text codecs */
	virtual uint32_t TextConvert(const void *p_string,
	                             uint32_t p_string_length,
	                             void *r_buffer,
	                             uint32_t p_buffer_length,
	                             uint32_t p_from_charset,
	                             uint32_t p_to_charset);
	virtual bool TextConvertToUnicode(uint32_t p_input_encoding,
	                                  const void *p_input,
	                                  uint32_t p_input_length,
	                                  void *p_output,
	                                  uint32_t & p_output_length,
	                                  uint32_t & r_used);

	/* ---------- Subprocesses */
	virtual bool Shell(MCStringRef p_filename,
	                   MCDataRef & r_data,
	                   int & r_retcode);

	virtual void CheckProcesses(void);
	virtual bool StartProcess(MCNameRef p_name,
	                          MCStringRef p_doc,
	                          intenum_t p_mode,
	                          Boolean p_elevated);
	virtual void CloseProcess(uint16_t p_index);
	virtual void Kill(int32_t p_pid, int32_t p_signal);
	virtual void KillAll(void);
	virtual Boolean Poll(real64_t p_delay, int p_fd);

	virtual Boolean IsInteractiveConsole(int p_fd);

	/* ---------- Miscellanous */
	virtual void LaunchDocument(MCStringRef p_document);
	virtual void LaunchUrl(MCStringRef p_document);

	virtual void DoAlternateLanguage(MCStringRef p_script,
	                                 MCStringRef p_language);
	virtual bool AlternateLanguages(MCListRef & r_list);

	virtual bool GetDNSservers(MCListRef & r_list);
    
    virtual void ShowMessageDialog(MCStringRef title,
                                   MCStringRef message);
};

MCSystemInterface * MCDesktopCreateEmscriptenSystem(void);

#endif /* ! __MC_EMSCRIPTEN_SYSTEM_H__ */
