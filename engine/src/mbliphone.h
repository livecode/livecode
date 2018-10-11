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
    virtual MCServiceInterface *QueryService(MCServiceType type);
    
	virtual bool Initialize(void);
	virtual void Finalize(void);
	
	virtual void Debug(MCStringRef p_string);
    
	virtual real64_t GetCurrentTime(void);
    
	virtual bool GetVersion(MCStringRef& r_string);
	virtual bool GetMachine(MCStringRef& r_string);
	virtual bool GetAddress(MCStringRef& r_address);
    
	virtual uint32_t GetProcessId(void);
	
	virtual void Alarm(real64_t p_when);
	virtual void Sleep(real64_t p_when);
	
	virtual void SetEnv(MCStringRef p_name, MCStringRef p_value);
	virtual bool GetEnv(MCStringRef p_name, MCStringRef& r_value);
	
	virtual Boolean CreateFolder(MCStringRef p_path);
	virtual Boolean DeleteFolder(MCStringRef p_path);
	
	virtual Boolean DeleteFile(MCStringRef p_path);
	
	virtual Boolean RenameFileOrFolder(MCStringRef p_old_name, MCStringRef p_new_name);
	
	virtual Boolean BackupFile(MCStringRef p_old_name, MCStringRef p_new_name);
	virtual Boolean UnbackupFile(MCStringRef p_old_name, MCStringRef p_new_name);
	
	virtual Boolean CreateAlias(MCStringRef p_target, MCStringRef p_alias);
	// NOTE: 'ResolveAlias' returns a standard (not native) path.
	virtual Boolean ResolveAlias(MCStringRef p_target, MCStringRef& r_dest);
	
	virtual bool GetCurrentFolder(MCStringRef& r_path);
	///* LEGACY */ char *GetCurrentFolder(void);
	virtual Boolean SetCurrentFolder(MCStringRef p_path);
	
	// NOTE: 'GetStandardFolder' returns a standard (not native) path.
	virtual Boolean GetStandardFolder(MCNameRef p_type, MCStringRef& r_folder);
	
    virtual real8 GetFreeDiskSpace();
    virtual Boolean GetDevices(MCStringRef& r_devices);
    virtual Boolean GetDrives(MCStringRef& r_drives);
    
	virtual Boolean FileExists(MCStringRef p_path);
	virtual Boolean FolderExists(MCStringRef p_path);
	virtual Boolean FileNotAccessible(MCStringRef p_path);
	
	virtual Boolean ChangePermissions(MCStringRef p_path, uint2 p_mask);
	virtual uint2 UMask(uint2 p_mask);
	
	virtual IO_handle OpenFile(MCStringRef p_path, intenum_t p_mode, Boolean p_map);
	virtual IO_handle OpenFd(uint32_t fd, intenum_t p_mode);
    virtual IO_handle OpenDevice(MCStringRef p_path, intenum_t p_mode);
	
	// NOTE: 'GetTemporaryFileName' returns a standard (not native) path.
	virtual bool GetTemporaryFileName(MCStringRef& r_tmp_name);
	
	virtual bool ListFolderEntries(MCStringRef p_folder, MCSystemListFolderEntriesCallback p_callback, void *x_context);
    
    // ST-2014-12-18: [[ Bug 14259 ]] Returns the executable from the system tools, not from argv[0]
	virtual bool PathToNative(MCStringRef p_path, MCStringRef& r_native);
	virtual bool PathFromNative(MCStringRef p_native, MCStringRef& r_path);
	virtual bool ResolvePath(MCStringRef p_path, MCStringRef& r_resolved_path);
	
	virtual bool LongFilePath(MCStringRef p_path, MCStringRef& r_long_path);
	virtual bool ShortFilePath(MCStringRef p_path, MCStringRef& r_short_path);
    
	virtual bool Shell(MCStringRef filename, MCDataRef& r_data, int& r_retcode);
    
	virtual uint32_t TextConvert(const void *p_string, uint32_t p_string_length, void *r_buffer, uint32_t p_buffer_length, uint32_t p_from_charset, uint32_t p_to_charset);
	virtual bool TextConvertToUnicode(uint32_t p_input_encoding, const void *p_input, uint4 p_input_length, void *p_output, uint4& p_output_length, uint4& r_used);
    
    virtual void CheckProcesses(void);
    
    virtual uint32_t GetSystemError(void);
    
    virtual bool StartProcess(MCNameRef p_name, MCStringRef p_doc, intenum_t p_mode, Boolean p_elevated);
    virtual void CloseProcess(uint2 p_index);
    virtual void Kill(int4 p_pid, int4 p_sig);
    virtual void KillAll(void);
    virtual Boolean Poll(real8 p_delay, int p_fd);
    
    virtual Boolean IsInteractiveConsole(int p_fd);
    
    virtual int GetErrno(void);
    virtual void SetErrno(int p_errno);
    
    virtual void LaunchDocument(MCStringRef p_document);
    virtual void LaunchUrl(MCStringRef p_document);
    
    virtual void DoAlternateLanguage(MCStringRef p_script, MCStringRef p_language);
    virtual bool AlternateLanguages(MCListRef& r_list);
    
    virtual bool GetDNSservers(MCListRef& r_list);
    
    virtual void ShowMessageDialog(MCStringRef title,
                                   MCStringRef message);
};

////////////////////////////////////////////////////////////////////////////////

#import <Foundation/NSString.h>

@interface NSString (com_runrev_livecode_NSStringAdditions)
	- (const char *)nativeCString;
@end

NSString *MCStringRefToNSString(MCStringRef p_string, bool p_unicode);
bool NSStringToMCStringRef(NSString *p_string, MCStringRef& r_string);

////////////////////////////////////////////////////////////////////////////////

#endif
