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

#ifndef __MC_SYSTEM__
#define __MC_SYSTEM__

#include "mcio.h"
#include "osspec.h"

#if !defined(_WINDOWS_SERVER) && !defined(_WINDOWS_DESKTOP)
#include <sys/mman.h>
#include <unistd.h>

#if defined(_MAC_SERVER) || defined(_MAC_DESKTOP)
#define ftello64(a) ftello(a)
#define fseeko64(a, b, c) fseeko(a, b, c)

#define opendir64(a) opendir(a)
#define readdir64(a) readdir(a)
#define closedir64(a) closedir(a)
#define DIR64 DIR
#endif

#endif // Mac and Linux-specific includes for file mapping

enum
{
    kMCSystemFileSeekSet = 1,
    kMCSystemFileSeekCurrent = 0,
    kMCSystemFileSeekEnd = -1,
};

struct MCDateTime;
struct MCDateTimeLocale;

struct MCSystemFolderEntry
{
	MCStringRef name;
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
	char* file_type;
	bool is_folder;
};

typedef bool (*MCSystemListFolderEntriesCallback)(void *p_context, const MCSystemFolderEntry *p_entry);
typedef bool (*MCSystemHostResolveCallback)(void *p_context, MCStringRef p_host);

struct MCSystemFileHandle
{
    virtual void Close(void) = 0;

    // Returns true if an attempt has been made to read past the end of the
    // stream.
    virtual bool IsExhausted(void) = 0;

	/* Attempt to read p_length bytes from the file handle.  Returns
	 * true iff p_length bytes are successfully read.  Partial reads
	 * may modify p_buffer, r_read and the file handle state, but
	 * should still return false. */
    virtual bool Read(void *p_buffer, uint32_t p_length, uint32_t& r_read) = 0;
    
	virtual bool Write(const void *p_buffer, uint32_t p_length) = 0;
    
	virtual bool Seek(int64_t offset, int p_dir) = 0;
	
	virtual bool Truncate(void) = 0;
	virtual bool Sync(void) = 0;
	virtual bool Flush(void) = 0;
	
	virtual bool PutBack(char p_char) = 0;
	
	virtual int64_t Tell(void) = 0;
	
	virtual void *GetFilePointer(void) = 0;
	virtual uint64_t GetFileSize(void) = 0;
    
    virtual bool TakeBuffer(void*& r_buffer, size_t& r_length) = 0;

    // Polymorphic - needs virtual destructor
    virtual ~MCSystemFileHandle()
    {
        ;
    }
};

////////////////////////////////////////////////////////////////////////////////

class MCMemoryFileHandle: public MCSystemFileHandle
{
public:
	MCMemoryFileHandle(void)
	{
		m_buffer = NULL;
		m_pointer = 0;
		m_length = 0;
		m_capacity = 0;
        // SN-2015-02-11: [[ Bug 14531 ]] Initially not EOF
        m_is_eof = false;
	}
	
	MCMemoryFileHandle(const void *p_data, size_t p_length)
	{
		m_buffer = (char *)p_data;
		m_pointer = 0;
		m_length = p_length;
		m_capacity = 0;
        // SN-2015-02-11: [[ Bug 14531 ]] Initially not EOF
        m_is_eof = false;
	}
	
	bool TakeBuffer(void*& r_buffer, size_t& r_length)
	{
		r_buffer = (char *)realloc(m_buffer, m_length);
		r_length = (size_t)m_length;
        
		m_buffer = NULL;
		m_length = 0;
		m_capacity = 0;
		m_pointer = 0;
        // SN-2015-02-11: [[ Bug 14531 ]] Not EOF anymore
        m_is_eof = false;
        
        return (r_buffer != nil);
	}
	
	void Close(void)
	{
		if (m_capacity != 0)
			free(m_buffer);
		delete this;
	}
    
    virtual bool IsExhausted(void)
    {
        // SN-2015-02-11: [[ Bug 14531 ]] Updated to a bool
        return m_is_eof;
    }
    
    bool Read(void *p_buffer, uint32_t p_length, uint32_t& r_read)
    {
        // SN-2015-02-11: [[ Bug 14531 ]] We are only EOF if we
        //  are asked more than what we have.
        if (p_length > m_length - m_pointer)
        {
            r_read = (uint32_t)(m_length - m_pointer);
            m_is_eof = true;
        }
        else
        {
            m_is_eof = false;
            r_read = p_length;
        }
        
        memcpy(p_buffer, m_buffer + m_pointer, r_read);
        m_pointer += r_read;

        return true;
    }
	
	bool Write(const void *p_buffer, uint32_t p_length)
	{
		// If we aren't writable then its an error (writable buffers start off with
		// nil buffer pointer, and 0 capacity).
		if (m_buffer != NULL && m_capacity == 0)
			return false;
		
		// If there isn't enough room, extend
		if (m_pointer + p_length > m_capacity)
		{
			size_t t_new_capacity;
			t_new_capacity = (m_pointer + p_length + 4096) & ~4095;
			
			void *t_new_buffer;
			t_new_buffer = realloc(m_buffer, t_new_capacity);
			if (t_new_buffer == NULL)
				return false;
			
			m_buffer = static_cast<char *>(t_new_buffer);
			m_capacity = t_new_capacity;
		}

		memcpy(m_buffer + m_pointer, p_buffer, p_length);
		m_pointer += p_length;
		m_length = MCMax(m_pointer, m_length);
        // SN-2015-02-11: [[ Bug 14531 ]] We are no longer at
        //  the EOF position once we have written.
        m_is_eof = false;
        
		return true;
	}
	
	bool Seek(int64_t p_offset, int p_dir)
	{
		int64_t t_base;
		if (p_dir == 0)
			t_base = m_pointer;
		else if (p_dir < 0)
			t_base = m_length;
		else
			t_base = 0;
		
		int64_t t_new_offset;
		t_new_offset = p_offset + t_base;
		if (t_new_offset < 0 || size_t(t_new_offset) > m_length)
			return false;
		
        // SN-2015-02-11: [[ Bug 14531 ]] We are no longer at
        //  the EOF position, since we have moved.
        m_is_eof = false;

		m_pointer = (uint32_t)t_new_offset;
		return true;
	}
	
	bool PutBack(char c)
	{
		if (m_pointer == 0)
			return false;
		
		m_pointer -= 1;
        // SN-2015-02-11: [[ Bug 14531 ]] We are no longer at
        //  the EOF position,
		return true;
	}
	
	int64_t Tell(void)
	{
		return m_pointer;
	}
	
	uint64_t GetFileSize(void)
	{
		return m_length;
	}
	
	void *GetFilePointer(void)
	{
		return m_buffer;
	}
	
	bool Truncate(void)
	{
		if (m_capacity != 0)
		{
			m_length = m_pointer;
			return true;
		}
        
		return false;
	}
	
	bool Sync(void)
	{
		return true;
	}
	
	bool Flush(void)
	{
		return true;
	}
	
protected:
	char *m_buffer;
	size_t m_pointer;
	size_t m_length;
	size_t m_capacity;
    // SN-2015-02-11: [[ Bug 14531 ]] Added a bool.
    //  We don't want m_pointer to go over m_length,
    //  but we need to know when we tried to read
    //  *more* than we had - which is the EOF 'state'.
    bool m_is_eof;
};

////////////////////////////////////////////////////////////////////////////////

#if !defined(_WINDOWS_SERVER) && !defined(_WINDOWS_DESKTOP)
class MCMemoryMappedFileHandle: public MCMemoryFileHandle
{
public:
    MCMemoryMappedFileHandle(int p_fd, void *p_buffer, size_t p_length)
    : MCMemoryFileHandle(p_buffer, p_length)
    {
        m_fd = p_fd;
        m_buffer = p_buffer;
        m_length = p_length;
    }
    
    void Close(void)
    {
        munmap((char *)m_buffer, m_length);
        close(m_fd);
        MCMemoryFileHandle::Close();
    }
    
private:
    int m_fd;
    void *m_buffer;
    size_t m_length;
};
#endif

////////////////////////////////////////////////////////////////////////////////

class MCCustomFileHandle: public MCSystemFileHandle
{
public:
	MCCustomFileHandle(MCFakeOpenCallbacks *p_callbacks, void *p_state)
	{
		m_state = p_state;
		m_callbacks = p_callbacks;
	}
	
	void Close(void)
	{
		// MW-2011-06-12: Fix memory leak - Close() should delete the handle.
		delete this;
	}
    
    virtual bool IsExhausted(void)
    {
        return m_is_eof;
    }
	
	bool Read(void *p_buffer, uint32_t p_blocksize, uint32_t& r_read)
	{
		if (m_callbacks -> read == nil)
			return false;
        
        IO_stat t_stat;
        
        t_stat = m_callbacks -> read(m_state, p_buffer, p_blocksize, r_read);
        
        if (t_stat == IO_EOF)
        {
            m_is_eof = true;
            return false;
        }
        
        m_is_eof = false;
        
		if (t_stat != IO_NORMAL)
            return false;
        
        return true;
	}
	
	bool Write(const void *p_buffer, uint32_t p_length)
	{
		return false;
	}
	
	bool Seek(int64_t p_offset, int p_dir)
	{
		if (p_dir == 0)
			return m_callbacks -> seek_cur(m_state, p_offset) == IO_NORMAL;
		else if (p_dir > 0)
			return m_callbacks -> seek_set(m_state, p_offset) == IO_NORMAL;
		return false;
	}
    
    bool TakeBuffer(void*& r_buffer, size_t& r_length)
    {
        return false;
    }
	
	bool PutBack(char c)
	{
		return false;
	}
	
	int64_t Tell(void)
	{
		return m_callbacks -> tell(m_state);
	}
	
	uint64_t GetFileSize(void)
	{
		return 0;
	}
	
	void *GetFilePointer(void)
	{
		return nil;
	}
	
	bool Truncate(void)
	{
		return false;
	}
	
	bool Sync(void)
	{
		return true;
	}
	
	bool Flush(void)
	{
		return true;
	}
	
private:
	void *m_state;
	MCFakeOpenCallbacks *m_callbacks;
    bool m_is_eof;
};

enum MCServiceType
{
    kMCServiceTypeMacSystem,
    kMCServiceTypeWindowsSystem,
    kMCServiceTypeLinuxSystem,
};

struct MCServiceInterface
{
};

struct MCMacSystemServiceInterface: public MCServiceInterface
{
    virtual bool SetResource(MCStringRef p_source, MCStringRef p_type, MCStringRef p_id, MCStringRef p_name, MCStringRef p_flags, MCStringRef p_value, MCStringRef& r_error) = 0;    
    virtual bool GetResource(MCStringRef p_source, MCStringRef p_type, MCStringRef p_name, MCStringRef& r_value, MCStringRef& r_error) = 0;    
    virtual bool GetResources(MCStringRef p_source, MCStringRef p_type, MCListRef& r_list, MCStringRef& r_error) = 0;    
    virtual bool CopyResource(MCStringRef p_source, MCStringRef p_dest, MCStringRef p_type, MCStringRef p_name, MCStringRef p_newid, MCStringRef& r_error) = 0;
    virtual bool DeleteResource(MCStringRef p_source, MCStringRef p_type, MCStringRef p_name, MCStringRef& r_error) = 0;
    virtual void CopyResourceFork(MCStringRef p_source, MCStringRef p_destination) = 0;
    virtual void LoadResFile(MCStringRef p_filename, MCStringRef& r_data) = 0;
    virtual void SaveResFile(MCStringRef p_path, MCDataRef p_data) = 0;
    
    virtual bool ProcessTypeIsForeground(void) = 0;
    virtual bool ChangeProcessType(bool p_to_foreground) = 0;
    
    virtual void Send(MCStringRef p_message, MCStringRef p_program, MCStringRef p_eventtype, Boolean p_reply) = 0;
    virtual void Reply(MCStringRef p_message, MCStringRef p_keyword, Boolean p_error) = 0;
    virtual void RequestAE(MCStringRef p_message, uint2 p_ae, MCStringRef& r_value) = 0;
    virtual bool RequestProgram(MCStringRef p_message, MCStringRef p_program, MCStringRef& r_result) = 0;
};

struct MCWindowsSystemServiceInterface: public MCServiceInterface
{
    virtual bool MCISendString(MCStringRef p_command, MCStringRef& r_result, bool& r_error) = 0;
    
    virtual bool QueryRegistry(MCStringRef p_key, MCValueRef& r_value, MCStringRef& r_type, MCStringRef& r_error) = 0;
    virtual bool SetRegistry(MCStringRef p_key, MCValueRef p_value, MCSRegistryValueType p_type, MCStringRef& r_error) = 0;
    virtual bool DeleteRegistry(MCStringRef p_key, MCStringRef& r_error) = 0;
    virtual bool ListRegistry(MCStringRef p_path, MCListRef& r_list, MCStringRef& r_error) = 0;
    
    virtual void ResetTime() = 0;
};

struct MCSystemInterface
{
    virtual MCServiceInterface *QueryService(MCServiceType type) = 0;
    
	virtual bool Initialize(void) = 0;
	virtual void Finalize(void) = 0;
	
	virtual void Debug(MCStringRef p_string) = 0;

	virtual real64_t GetCurrentTime(void) = 0;

	virtual bool GetVersion(MCStringRef& r_string) = 0;
	virtual bool GetMachine(MCStringRef& r_string) = 0;
	virtual bool GetAddress(MCStringRef& r_address) = 0;

	virtual uint32_t GetProcessId(void) = 0;
	
	virtual void Alarm(real64_t p_when) = 0;
	virtual void Sleep(real64_t p_when) = 0;
	
	virtual void SetEnv(MCStringRef p_name, MCStringRef p_value) = 0;
	virtual bool GetEnv(MCStringRef p_name, MCStringRef& r_value) = 0;
	
	virtual Boolean CreateFolder(MCStringRef p_path) = 0;
	virtual Boolean DeleteFolder(MCStringRef p_path) = 0;
	
	virtual Boolean DeleteFile(MCStringRef p_path) = 0;
	
	virtual Boolean RenameFileOrFolder(MCStringRef p_old_name, MCStringRef p_new_name) = 0;
	
	virtual Boolean BackupFile(MCStringRef p_old_name, MCStringRef p_new_name) = 0;
	virtual Boolean UnbackupFile(MCStringRef p_old_name, MCStringRef p_new_name) = 0;
	
	virtual Boolean CreateAlias(MCStringRef p_target, MCStringRef p_alias) = 0;
	// NOTE: 'ResolveAlias' returns a standard (not native) path.
	virtual Boolean ResolveAlias(MCStringRef p_target, MCStringRef& r_dest) = 0;
	
	virtual bool GetCurrentFolder(MCStringRef& r_path) = 0;
	///* LEGACY */ char *GetCurrentFolder(void);
	virtual Boolean SetCurrentFolder(MCStringRef p_path) = 0;
	
	// NOTE: 'GetStandardFolder' returns a standard (not native) path.
	virtual Boolean GetStandardFolder(MCNameRef p_type, MCStringRef& r_folder) = 0;
	
    virtual real8 GetFreeDiskSpace() = 0;    
    virtual Boolean GetDevices(MCStringRef& r_devices) = 0;
    virtual Boolean GetDrives(MCStringRef& r_drives) = 0;

	virtual Boolean FileExists(MCStringRef p_path) = 0;
	virtual Boolean FolderExists(MCStringRef p_path) = 0;
	virtual Boolean FileNotAccessible(MCStringRef p_path) = 0;
	
	virtual Boolean ChangePermissions(MCStringRef p_path, uint2 p_mask) = 0;
	virtual uint2 UMask(uint2 p_mask) = 0;
	
    virtual IO_handle DeployOpen(MCStringRef p_path, intenum_t p_mode)
    {
        return OpenFile(p_path, p_mode, False);
    }
    
	virtual IO_handle OpenFile(MCStringRef p_path, intenum_t p_mode, Boolean p_map) = 0;
	virtual IO_handle OpenFd(uint32_t fd, intenum_t p_mode) = 0;
    virtual IO_handle OpenDevice(MCStringRef p_path, intenum_t p_mode) = 0;
	
	// NOTE: 'GetTemporaryFileName' returns a standard (not native) path.
	virtual bool GetTemporaryFileName(MCStringRef& r_tmp_name) = 0;
	
	virtual bool ListFolderEntries(MCStringRef p_folder, MCSystemListFolderEntriesCallback p_callback, void *x_context) = 0;
    
    // ST-2014-12-18: [[ Bug 14259 ]] Returns the executable from the system tools, not from argv[0]
	virtual bool PathToNative(MCStringRef p_path, MCStringRef& r_native) = 0;
	virtual bool PathFromNative(MCStringRef p_native, MCStringRef& r_path) = 0;
	virtual bool ResolvePath(MCStringRef p_path, MCStringRef& r_resolved_path) = 0;
	
	virtual bool LongFilePath(MCStringRef p_path, MCStringRef& r_long_path) = 0;
	virtual bool ShortFilePath(MCStringRef p_path, MCStringRef& r_short_path) = 0;

	virtual bool Shell(MCStringRef filename, MCDataRef& r_data, int& r_retcode) = 0;

	virtual uint32_t TextConvert(const void *p_string, uint32_t p_string_length, void *r_buffer, uint32_t p_buffer_length, uint32_t p_from_charset, uint32_t p_to_charset) = 0;
	virtual bool TextConvertToUnicode(uint32_t p_input_encoding, const void *p_input, uint4 p_input_length, void *p_output, uint4& p_output_length, uint4& r_used) = 0;
    
    virtual void CheckProcesses(void) = 0;
    
    virtual uint32_t GetSystemError(void) = 0;
    
    virtual bool StartProcess(MCNameRef p_name, MCStringRef p_doc, intenum_t p_mode, Boolean p_elevated) = 0;
    virtual void CloseProcess(uint2 p_index) = 0;
    virtual void Kill(int4 p_pid, int4 p_sig) = 0;
    virtual void KillAll(void) = 0;
    virtual Boolean Poll(real8 p_delay, int p_fd) = 0;
    
    virtual Boolean IsInteractiveConsole(int p_fd) = 0;
    
    virtual int GetErrno(void) = 0;
    virtual void SetErrno(int p_errno) = 0;
    
    virtual void LaunchDocument(MCStringRef p_document) = 0;
    virtual void LaunchUrl(MCStringRef p_document) = 0;
    
    virtual void DoAlternateLanguage(MCStringRef p_script, MCStringRef p_language) = 0;
    virtual bool AlternateLanguages(MCListRef& r_list) = 0;
    
    virtual bool GetDNSservers(MCListRef& r_list) = 0;
    
    virtual void ShowMessageDialog(MCStringRef p_title, MCStringRef p_message) = 0;
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
	kMCSystemUrlStatusFinished,
	kMCSystemUrlStatusLoadingProgress,
};

typedef bool (*MCSystemUrlCallback)(void *context, MCSystemUrlStatus status, const void *param);

enum MCSystemUrlOperation
{
	kMCSystemUrlOperationStrip = (1 << 0), // remove whitespace from the beginning / end of url string
};

bool MCSystemProcessUrl(MCStringRef p_url, MCSystemUrlOperation p_operations, MCStringRef &r_processed_url);
bool MCSystemLoadUrl(MCStringRef p_url, MCSystemUrlCallback p_callback, void *p_context);
bool MCSystemPostUrl(MCStringRef p_url, MCDataRef p_data, uint32_t p_length, MCSystemUrlCallback p_callback, void *p_context);
bool MCSystemPutUrl(MCStringRef p_url, MCDataRef p_data, uint32_t p_length, MCSystemUrlCallback p_callback, void *p_context);
void MCSystemSetUrlSSLVerification(bool enabled);

//////////

#endif
