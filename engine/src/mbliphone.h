/* Copyright (C) 2003-2015 LiveCode Ltd.

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

#ifndef __MBL_IPHONE__
#define __MBL_IPHONE__

#ifndef __MC_SYSTEM__
#include "system.h"
#endif

////////////////////////////////////////////////////////////////////////////////

struct MCIPhoneSystem: public MCSystemInterface
{
public:
	bool Initialize(void);
	void Finalize(void);
	
	void Debug(const char *p_string);

	real64_t GetCurrentTime(void);
	
	uint32_t GetProcessId(void);
	
	char *GetVersion(void);
	char *GetMachine(void);
	char *GetProcessor(void);
	char *GetAddress(void);
	
	void Alarm(real64_t p_when);
	void Sleep(real64_t p_when);
	
	void SetEnv(const char *name, const char *value);
	char *GetEnv(const char *name);
	
	bool CreateFolder(const char *p_path);
	bool DeleteFolder(const char *p_path);
	
	bool DeleteFile(const char *p_path);
	
	bool RenameFileOrFolder(const char *p_old_name, const char *p_new_name);
	
	bool BackupFile(const char *p_old_name, const char *p_new_name);
	bool UnbackupFile(const char *p_old_name, const char *p_new_name);
	
	bool CreateAlias(const char *p_target, const char *p_alias);
	char *ResolveAlias(const char *p_target);
	
	char *GetCurrentFolder(void);
	bool SetCurrentFolder(const char *p_path);
	
	bool FileExists(const char *p_path);
	bool FolderExists(const char *p_path);
	bool FileNotAccessible(const char *p_path);
	
	bool ChangePermissions(const char *p_path, uint2 p_mask);
	uint2 UMask(uint2 p_mask);
	
	MCSystemFileHandle *OpenFile(const char *p_path, uint32_t p_mode, bool p_map);
	MCSystemFileHandle *OpenStdFile(uint32_t i);
	MCSystemFileHandle *OpenDevice(const char *p_path, uint32_t p_mode, const char *p_control_string);
	
	char *GetTemporaryFileName(void);
	char *GetStandardFolder(const char *folder);
	
	void *LoadModule(const char *p_path);
	void *ResolveModuleSymbol(void *p_module, const char *p_symbol);
	void UnloadModule(void *p_module);
	
	char *LongFilePath(const char *p_path);
	char *ShortFilePath(const char *p_path);
	
	char *PathToNative(const char *p_rev_path);
	char *PathFromNative(const char *p_rev_path);
	char *ResolvePath(const char *p_rev_path);
	char *ResolveNativePath(const char *p_rev_path);
	
	bool ListFolderEntries(MCSystemListFolderEntriesCallback p_callback, void *p_context);
	
	bool Shell(const char *p_cmd, uint32_t p_cmd_length, void*& r_data, uint32_t& r_data_length, int& r_retcode);
	
	char *GetHostName(void);
	bool HostNameToAddress(const char *p_hostname, MCSystemHostResolveCallback p_callback, void *p_context);
	bool AddressToHostName(const char *p_address, MCSystemHostResolveCallback p_callback, void *p_context);

	uint32_t TextConvert(const void *string, uint32_t string_length, void *buffer, uint32_t buffer_length, uint32_t from_charset, uint32_t to_charset);
	bool TextConvertToUnicode(uint32_t p_input_encoding, const void *p_input, uint4 p_input_length, void *p_output, uint4 p_output_length, uint4& r_used);
};

////////////////////////////////////////////////////////////////////////////////

#import <Foundation/NSString.h>

@interface NSString (com_runrev_livecode_NSStringAdditions)
	- (const char *)nativeCString;
@end

////////////////////////////////////////////////////////////////////////////////

#endif
