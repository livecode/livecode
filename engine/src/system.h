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

#ifndef __MC_SYSTEM__
#define __MC_SYSTEM__

enum
{
	kMCSystemFileModeRead = 0,
	kMCSystemFileModeWrite = 1,
	kMCSystemFileModeUpdate = 2,
	kMCSystemFileModeAppend = 3,
	
	kMCSystemFileModeNulTerminate = 1 << 16
};

struct MCDateTime;
struct MCDateTimeLocale;

struct MCSystemFolderEntry
{
	const char *name;
	int64_t data_size;
	int64_t resource_size;
	uint32_t creation_time;
	uint32_t modification_time;
	uint32_t access_time;
	uint32_t backup_time;
	uint32_t user_id;
	uint32_t group_id;
	uint32_t permissions;
	uint32_t file_creator;
	uint32_t file_type;
	bool is_folder;
};

typedef bool (*MCSystemListFolderEntriesCallback)(void *p_context, const MCSystemFolderEntry *p_entry);
typedef bool (*MCSystemHostResolveCallback)(void *p_context, const char *p_host);

struct MCSystemFileHandle
{
	virtual void Close(void) = 0;
	
	virtual bool Read(void *p_buffer, uint32_t p_length, uint32_t& r_read) = 0;
	virtual bool Write(const void *p_buffer, uint32_t p_length, uint32_t& r_written) = 0;
	virtual bool Seek(int64_t offset, int p_dir) = 0;
	
	virtual bool Truncate(void) = 0;
	virtual bool Sync(void) = 0;
	virtual bool Flush(void) = 0;
	
	virtual bool PutBack(char p_char) = 0;
	
	virtual int64_t Tell(void) = 0;
	
	virtual void *GetFilePointer(void) = 0;
	virtual int64_t GetFileSize(void) = 0;
};

struct MCSystemInterface
{
	virtual bool Initialize(void) = 0;
	virtual void Finalize(void) = 0;
	
	virtual void Debug(const char *p_string) = 0;

	virtual real64_t GetCurrentTime(void) = 0;

	virtual char *GetVersion(void) = 0;
	virtual char *GetMachine(void) = 0;
	virtual char *GetProcessor(void) = 0;
	virtual char *GetAddress(void) = 0;

	virtual uint32_t GetProcessId(void) = 0;
	
	virtual void Alarm(real64_t p_when) = 0;
	virtual void Sleep(real64_t p_when) = 0;
	
	virtual void SetEnv(const char *name, const char *value) = 0;
	virtual char *GetEnv(const char *name) = 0;
	
	virtual bool CreateFolder(const char *p_path) = 0;
	virtual bool DeleteFolder(const char *p_path) = 0;
	
	virtual bool DeleteFile(const char *p_path) = 0;
	
	virtual bool RenameFileOrFolder(const char *p_old_name, const char *p_new_name) = 0;
	
	virtual bool BackupFile(const char *p_old_name, const char *p_new_name) = 0;
	virtual bool UnbackupFile(const char *p_old_name, const char *p_new_name) = 0;
	
	virtual bool CreateAlias(const char *p_target, const char *p_alias) = 0;
	// NOTE: 'ResolveAlias' returns a standard (not native) path.
	virtual char *ResolveAlias(const char *p_target) = 0;
	
	virtual char *GetCurrentFolder(void) = 0;
	virtual bool SetCurrentFolder(const char *p_path) = 0;
	
	// NOTE: 'GetStandardFolder' returns a standard (not native) path.
	virtual char *GetStandardFolder(const char *p_folder) = 0;
	
	virtual bool FileExists(const char *p_path) = 0;
	virtual bool FolderExists(const char *p_path) = 0;
	virtual bool FileNotAccessible(const char *p_path) = 0;
	
	virtual bool ChangePermissions(const char *p_path, uint2 p_mask) = 0;
	virtual uint2 UMask(uint2 p_mask) = 0;
	
	virtual MCSystemFileHandle *OpenFile(const char *p_path, uint32_t p_mode, bool p_map) = 0;
	virtual MCSystemFileHandle *OpenStdFile(uint32_t i) = 0;
	virtual MCSystemFileHandle *OpenDevice(const char *p_path, uint32_t p_mode, const char *p_control_string) = 0;
	
	// NOTE: 'GetTemporaryFileName' returns a standard (not native) path.
	virtual char *GetTemporaryFileName(void) = 0;
	
	virtual void *LoadModule(const char *p_path) = 0;
	virtual void *ResolveModuleSymbol(void *p_module, const char *p_symbol) = 0;
	virtual void UnloadModule(void *p_module) = 0;
	
	virtual char *LongFilePath(const char *p_path) = 0;
	virtual char *ShortFilePath(const char *p_path) = 0;

	virtual char *PathToNative(const char *p_rev_path) = 0;
	virtual char *PathFromNative(const char *p_rev_path) = 0;
	virtual char *ResolvePath(const char *p_rev_path) = 0;
	virtual char *ResolveNativePath(const char *p_rev_path) = 0;
	
	virtual bool ListFolderEntries(MCSystemListFolderEntriesCallback p_callback, void *p_context) = 0;
	
	virtual bool Shell(const char *p_cmd, uint32_t p_cmd_length, void*& r_data, uint32_t& r_data_length, int& r_retcode) = 0;

	virtual char *GetHostName(void) = 0;
	virtual bool HostNameToAddress(const char *p_hostname, MCSystemHostResolveCallback p_callback, void *p_context) = 0;
	virtual bool AddressToHostName(const char *p_address, MCSystemHostResolveCallback p_callback, void *p_context) = 0;

	virtual uint32_t TextConvert(const void *string, uint32_t string_length, void *buffer, uint32_t buffer_length, uint32_t from_charset, uint32_t to_charset) = 0;
	virtual bool TextConvertToUnicode(uint32_t p_input_encoding, const void *p_input, uint4 p_input_length, void *p_output, uint4 p_output_length, uint4& r_used) = 0;
};

extern MCSystemInterface *MCsystem;

//////////

enum MCSystemUrlStatus
{
	kMCSystemUrlStatusNone,
	kMCSystemUrlStatusError,
	kMCSystemUrlStatusStarted,
	kMCSystemUrlStatusNegotiated,
	kMCSystemUrlStatusUploading,
	kMCSystemUrlStatusUploaded,
	kMCSystemUrlStatusLoading,
	kMCSystemUrlStatusFinished
};

typedef bool (*MCSystemUrlCallback)(void *context, MCSystemUrlStatus status, const void *param);

enum MCSystemUrlOperation
{
	kMCSystemUrlOperationStrip = (1 << 0), // remove whitespace from the beginning / end of url string
};

bool MCSystemProcessUrl(const char *p_url, MCSystemUrlOperation p_operations, char *&r_processed_url);
bool MCSystemLoadUrl(const char *p_url, MCSystemUrlCallback p_callback, void *p_context);
bool MCSystemPostUrl(const char *p_url, const void *p_data, uint32_t p_length, MCSystemUrlCallback p_callback, void *p_context);
bool MCSystemPutUrl(const char *p_url, const void *p_data, uint32_t p_length, MCSystemUrlCallback p_callback, void *p_context);
void MCSystemSetUrlSSLVerification(bool enabled);

//////////

#endif
