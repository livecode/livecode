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

#ifndef __MC_MOBILE_ANDROID__
#define __MC_MOBILE_ANDROID__

#ifndef __MC_SYSTEM__
#include "system.h"
#endif

////////////////////////////////////////////////////////////////////////////////

struct MCAndroidSystem: public MCSystemInterface
{
public:
	bool Initialize(void);
	void Finalize(void);
	
	void Debug(const char *p_string);

	real64_t GetCurrentTime(void);
	
	uint32_t GetProcessId(void);
	
	bool GetVersion(MCStringRef& r_string);
	bool GetMachine(MCStringRef& r_string);
	MCNameRef GetProcessor(void);
	char *GetAddress(void);
	
	void Alarm(real64_t p_when);
	void Sleep(real64_t p_when);
	
	void SetEnv(MCStringRef name, MCStringRef value);
	void GetEnv(MCStringRef name, MCStringRef& r_env);
	
	bool CreateFolder(MCStringRef p_path);
	bool DeleteFolder(MCStringRef p_path);
	
	bool DeleteFile(const char *p_path);
	
	bool RenameFileOrFolder(MCStringRef p_old_name, MCStringRef p_new_name);
	
	bool BackupFile(MCStringRef p_old_name, MCStringRef p_new_name);
	bool UnbackupFile(MCStringRef p_old_name, MCStringRef p_new_name);
	
	bool CreateAlias(MCStringRef p_target, MCStringRef p_alias);
	char *ResolveAlias(const char *p_target);
	
	bool GetCurrentFolder(MCStringRef& r_path);
	bool SetCurrentFolder(MCStringRef p_path);
	
	bool FileExists(const char *p_path);
	bool FolderExists(const char *p_path);
	bool FileNotAccessible(MCStringRef p_path);
	
	bool ChangePermissions(MCStringRef p_path, uint2 p_mask);
	uint2 UMask(uint2 p_mask);
	
	MCSystemFileHandle *OpenFile(MCStringRef p_path, uint32_t p_mode, bool p_map);
	MCSystemFileHandle *OpenStdFile(uint32_t i);
	MCSystemFileHandle *OpenDevice(MCStringRef p_path, uint32_t p_mode, MCStringRef p_control_string);
	
	char *GetTemporaryFileName(void);
	char *GetStandardFolder(const char *folder);
	
	void *LoadModule(MCStringRef p_path);
	void *ResolveModuleSymbol(void *p_module, MCStringRef p_symbol);
	void UnloadModule(void *p_module);
	
	bool LongFilePath(MCStringRef p_path, MCStringRef& r_long_path);
	bool ShortFilePath(MCStringRef p_path, MCStringRef& r_short_path);
	
	bool PathToNative(MCStringRef p_path, MCStringRef& r_native);
	bool PathFromNative(MCStringRef p_native, MCStringRef& r_path);
	bool ResolvePath(MCStringRef p_path, MCStringRef& r_resolved);
	bool ResolveNativePath(MCStringRef p_path, MCStringRef& r_resolved);
	
	bool ListFolderEntries(MCSystemListFolderEntriesCallback p_callback, void *p_context);
	
	bool Shell(MCStringRef p_cmd, MCDataRef& r_data, int& r_retcode);
	
	char *GetHostName(void);
	bool HostNameToAddress(MCStringRef p_hostname, MCSystemHostResolveCallback p_callback, void *p_context);
	bool AddressToHostName(MCStringRef p_address, MCSystemHostResolveCallback p_callback, void *p_context);

	uint32_t TextConvert(const void *string, uint32_t string_length, void *buffer, uint32_t buffer_length, uint32_t from_charset, uint32_t to_charset);
	bool TextConvertToUnicode(uint32_t p_input_encoding, const void *p_input, uint4 p_input_length, void *p_output, uint4 p_output_length, uint4& r_used);
};

////////////////////////////////////////////////////////////////////////////////

#endif
