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

#include "prefix.h"

#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"
#include "mcio.h"

#include "param.h"
#include "mcerror.h"
#include "execpt.h"
#include "util.h"
#include "object.h"
#include "socket.h"
#include "osspec.h"

#include "globals.h"
#include "text.h"
#include "stacksecurity.h"

#include "core.h"
#include "system.h"

////////////////////////////////////////////////////////////////////////////////

extern bool MCSystemLaunchUrl(const char *p_document);
extern char *MCSystemGetVersion(void);
extern char *MCSystemGetMachine(void);
extern char *MCSystemGetProcessor(void);
extern char *MCSystemGetAddress(void);
extern uint32_t MCSystemPerformTextConversion(const char *string, uint32_t string_length, char *buffer, uint32_t buffer_length, uint1 from_charset, uint1 to_charset);

////////////////////////////////////////////////////////////////////////////////

MCSystemInterface *MCsystem;

#ifdef TARGET_SUBPLATFORM_ANDROID
static volatile int *s_mainthread_errno;
#else
static int *s_mainthread_errno;
#endif


uint1 *MClowercasingtable = NULL;
uint1 *MCuppercasingtable = NULL;

////////////////////////////////////////////////////////////////////////////////

void MCS_common_init(void)
{
	s_mainthread_errno = &errno;
	
	MCsystem -> Initialize();
	
	IO_stdin = new IO_header(MCsystem -> OpenStdFile(0), 0);
	IO_stdout = new IO_header(MCsystem -> OpenStdFile(1), 1);
	IO_stderr = new IO_header(MCsystem -> OpenStdFile(2), 2);
	
	MCinfinity = HUGE_VAL;

	MCuppercasingtable = new uint1[256];
	for(uint4 i = 0; i < 256; ++i)
		MCuppercasingtable[i] = (uint1)toupper((uint1)i);
	
	MClowercasingtable = new uint1[256];
	for(uint4 i = 0; i < 256; ++i)
		MClowercasingtable[i] = (uint1)tolower((uint1)i);
	
	MCStackSecurityInit();
}

void MCS_shutdown(void)
{
	MCsystem -> Finalize();
}

////////////////////////////////////////////////////////////////////////////////

real8 MCS_time(void)
{
	return MCsystem -> GetCurrentTime();
}

void MCS_setenv(const char *p_name, const char *p_value)
{
	MCsystem -> SetEnv(p_name, p_value);
}

void MCS_unsetenv(const char *p_name)
{
	MCsystem -> SetEnv(p_name, NULL);
}

char *MCS_getenv(const char *p_name)
{
	return MCsystem -> GetEnv(p_name);
}

real8 MCS_getfreediskspace(void)
{
	return 0.0;
}

void MCS_launch_document(char *p_document)
{
	MCresult -> sets("not supported");
	delete p_document;
}

void MCS_launch_url(char *p_document)
{
	MCresult -> clear();
	if (!MCSystemLaunchUrl(p_document))
		MCresult -> sets("no association");
	delete p_document;
}

void MCS_getspecialfolder(MCExecPoint& ep)
{
	char *t_path;
	t_path = MCsystem -> GetStandardFolder(ep . getcstring());
	if (t_path != nil)
	{
		ep . copysvalue(t_path);
		delete t_path;
	}
	else
	{
		ep . clear();
		MCresult -> sets("folder not found");
	}
}

void MCS_doalternatelanguage(MCString& p_code, const char *p_language)
{
	MCresult -> sets("not supported");
}

void MCS_alternatelanguages(MCExecPoint& ep)
{
	ep . clear();
}

void MCS_seterrno(int value)
{
	*s_mainthread_errno = value;
}

int MCS_geterrno(void)
{
	return *s_mainthread_errno;
}

void MCS_sleep(real8 p_delay)
{
	MCsystem -> Sleep(p_delay);
}

void MCS_alarm(real8 p_delay)
{
	MCsystem -> Alarm(p_delay);
}

uint32_t MCS_getpid(void)
{
	return MCsystem -> GetProcessId();
}

////////////////////////////////////////////////////////////////////////////////

// As there is no clear way (at present) to see how to indirect these through
// the system interface we just implement them as is in srvwindows.cpp, providing
// the default 'dummy' functions here.

#ifndef _WINDOWS_SERVER
void MCS_query_registry(MCExecPoint &dest, const char** type)
{
	MCresult->sets("not supported");
	dest.clear();
}

void MCS_set_registry(const char *key, MCExecPoint &dest, char *type)
{
	MCresult->sets("not supported");
	dest.setsvalue(MCU_btos(False));
}

void MCS_delete_registry(const char *key, MCExecPoint &dest)
{
	MCresult->sets("not supported");
	dest.setsvalue(MCU_btos(False));
}

void MCS_list_registry(MCExecPoint& p_dest)
{
	MCresult -> sets("not supported");
	p_dest . setsvalue(MCU_btos(False));
}
#endif

////////////////////////////////////////////////////////////////////////////////

const char *MCS_getsystemversion(void)
{
	static char *t_version = nil;
	if (t_version == nil)
		t_version = MCsystem -> GetVersion();
	return t_version;
}

const char *MCS_getprocessor(void)
{
	static char *t_processor = nil;
	if (t_processor == nil)
		t_processor = MCsystem -> GetProcessor();
	return t_processor;
}

const char *MCS_getmachine(void)
{
	static char *t_machine = nil;
	if (t_machine == nil)
		t_machine = MCsystem -> GetMachine();
	return t_machine;
}

const char *MCS_getaddress(void)
{
	static char *t_address = nil;
	if (t_address == nil)
		t_address = MCsystem -> GetAddress();
	return t_address;
}

////////////////////////////////////////////////////////////////////////////////

Boolean MCS_mkdir(const char *p_path)
{
	char *t_resolved_path;
	t_resolved_path = MCS_resolvepath(p_path);
	
	bool t_result;
	t_result = MCsystem -> CreateFolder(t_resolved_path);
	
	delete t_resolved_path;
	
	return t_result;
}

Boolean MCS_rmdir(const char *p_path)
{
	char *t_resolved_path;
	t_resolved_path = MCS_resolvepath(p_path);
	
	bool t_result;
	t_result = MCsystem -> DeleteFolder(t_resolved_path);
	
	delete t_resolved_path;
	
	return t_result;
}

Boolean MCS_rename(const char *p_old_name, const char *p_new_name)
{
	char *t_old_resolved_path, *t_new_resolved_path;
	t_old_resolved_path = MCS_resolvepath(p_old_name);
	t_new_resolved_path = MCS_resolvepath(p_new_name);
	
	bool t_result;
	t_result = MCsystem -> RenameFileOrFolder(t_old_resolved_path, t_new_resolved_path);
	
	delete t_old_resolved_path;
	delete t_new_resolved_path;
	
	return t_result;
}

Boolean MCS_unlink(const char *p_path)
{
	char *t_resolved_path;
	t_resolved_path = MCS_resolvepath(p_path);
	
	bool t_result;
	t_result = MCsystem -> DeleteFile(t_resolved_path);
	
	delete t_resolved_path;
	
	return t_result;
}

Boolean MCS_backup(const char *p_old_name, const char *p_new_name)
{
	return MCsystem -> BackupFile(p_old_name, p_new_name);
}

Boolean MCS_unbackup(const char *p_old_name, const char *p_new_name)
{
	return MCsystem -> UnbackupFile(p_old_name, p_new_name);
}
	
Boolean MCS_createalias(char *p_target, char *p_alias)
{
	return MCsystem -> CreateAlias(p_target, p_alias);
}

void MCS_resolvealias(MCExecPoint& ep)
{
	char *t_alias;
	t_alias = ep . getsvalue() . clone();
	
	char *t_resolved_alias;
	t_resolved_alias = MCsystem -> ResolveAlias(t_alias);
	if (t_resolved_alias == NULL)
	{
		ep . clear();
		MCresult -> sets("can't get");
	}
	else
	{
		ep . copysvalue(t_resolved_alias, strlen(t_resolved_alias));
		MCresult -> clear();
	}
	
	delete t_alias;
	delete t_resolved_alias;
}

void MCS_setresource(const char *p_source, const char *p_type, const char *p_name, const char *p_id, const char *p_flags, const MCString& p_data)
{
	MCresult -> sets("not supported");
}

void MCS_getresource(const char *p_src, const char *p_type, const char *p_name, MCExecPoint& ep)
{
	MCresult -> sets("not supported");
	ep . clear();
}

void MCS_copyresource(const char *p_src, const char *p_dst, const char *p_type, const char *p_name, const char *p_new_id)
{
	MCresult -> sets("not supported");
}

void MCS_copyresourcefork(const char *source, const char *dst)
{
}

void MCS_deleteresource(const char *p_src, const char *p_type, const char *p_name)
{
	MCresult -> sets("not supported");
}

char *MCS_getresources(const char *p_src, const char *p_type)
{
	MCresult -> sets("not supported");
	return NULL;
}

void MCS_longfilepath(MCExecPoint& ep)
{
	char *t_long_path;
	t_long_path = MCsystem -> LongFilePath(ep . getcstring());
	if (t_long_path != nil)
	{
		MCresult -> clear();
		ep . copysvalue(t_long_path);
		delete t_long_path;
	}
	else
	{
		MCresult -> sets("can't get");
		ep . clear();
	}
}

void MCS_shortfilepath(MCExecPoint& ep)
{
	char *t_short_path;
	t_short_path = MCsystem -> ShortFilePath(ep . getcstring());
	if (t_short_path != nil)
	{
		MCresult -> clear();
		ep . copysvalue(t_short_path);
		delete t_short_path;
	}
	else
	{
		MCresult -> sets("can't get");
		ep . clear();
	}
}

Boolean MCS_setcurdir(const char *p_folder)
{
	char *t_resolved_folder;
	t_resolved_folder = MCS_resolvepath(p_folder);
	
	bool t_success;
	t_success = MCsystem -> SetCurrentFolder(t_resolved_folder);
	delete t_resolved_folder;
	return t_success;
}

char *MCS_getcurdir(void)
{
	char *t_native_folder;
	t_native_folder = MCsystem -> GetCurrentFolder();
	
	char *t_folder;
	t_folder = MCsystem -> PathFromNative(t_native_folder);
	
	free(t_native_folder);
	
	return t_folder;
}

struct MCS_getentries_state
{
	bool first;
	bool files;
	bool details;
	MCExecPoint *ep;
};

static bool MCS_getentries_callback(void *p_context, const MCSystemFolderEntry *p_entry)
{
	MCS_getentries_state *t_state;
	t_state = static_cast<MCS_getentries_state *>(p_context);
	
	if (!t_state -> files != p_entry -> is_folder)
		return true;
	
	// MW-2011-01-23: [[ Bug 9315 ]] Only urlEncode the filenames/folders if returning
	//   detailed info.
	MCExecPoint t_encoded_name;
	t_encoded_name . copysvalue(p_entry -> name);
	if (t_state -> details)
		MCU_urlencode(t_encoded_name);
	t_state -> ep -> concatmcstring(t_encoded_name . getsvalue(), EC_RETURN, t_state -> first);
	
	if (t_state -> details)
	{
		char t_buffer[U4L * 5 + 21];
#ifdef _WIN32
		sprintf(t_buffer, "%I64d,,%ld,%ld,%ld,,,,%03o",
				p_entry -> data_size,
				p_entry -> creation_time,
				p_entry -> modification_time,
				p_entry -> access_time,
				p_entry -> permissions);
#else
		sprintf(t_buffer, "%lld,,,%u,%u,,%d,%d,%03o,",
				p_entry -> data_size,
				p_entry -> modification_time, p_entry -> access_time,
				p_entry -> user_id, p_entry -> group_id,
				p_entry -> permissions);
#endif
		t_state -> ep -> concatcstring(t_buffer, EC_COMMA, False);
	}
	
	t_state -> first = False;
	
	return true;
}

void MCS_getentries(MCExecPoint& ep, bool p_files, bool p_detailed)
{
	MCS_getentries_state t_state;
	t_state . first = true;
	t_state . files = p_files;
	t_state . details = p_detailed;
	t_state . ep = &ep;
	
	ep . clear();
	if (!MCsystem -> ListFolderEntries(MCS_getentries_callback, &t_state))
		ep . clear();
}

IO_stat MCS_chmod(const char *p_path, uint2 p_mask)
{
	if (!MCsystem -> ChangePermissions(p_path, p_mask))
		return IO_ERROR;
	
	return IO_NORMAL;
}

int4 MCS_getumask(void)
{
	int4 t_old_mask;
	t_old_mask = MCsystem -> UMask(0);
	MCsystem -> UMask(t_old_mask);
	return t_old_mask;
}

uint2 MCS_umask(uint2 p_mask)
{
	return MCsystem -> UMask(p_mask);
}

Boolean MCS_exists(const char *p_path, Boolean p_file)
{
	char *t_resolved_path;
	t_resolved_path = MCS_resolvepath(p_path);
	
	Bool t_found;
	if (p_file)
		t_found = MCsystem -> FileExists(t_resolved_path);
	else
		t_found = MCsystem -> FolderExists(t_resolved_path);
	
	delete t_resolved_path;
	
	return t_found;
}

Boolean MCS_noperm(const char *p_path)
{
	return MCsystem -> FileNotAccessible(p_path);
}

Boolean MCS_getdrives(MCExecPoint& ep)
{
	ep . clear();
	return True;
}

Boolean MCS_getdevices(MCExecPoint& ep)
{
	ep . clear();
	return True;
}

char *MCS_resolvepath(const char *p_path)
{
	return MCsystem -> ResolvePath(p_path);
}

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
	}
	
	MCMemoryFileHandle(const MCString& p_data)
	{
		m_buffer = (char *)p_data . getstring();
		m_pointer = 0;
		m_length = p_data . getlength();
		m_capacity = 0;
	}
	
	void TakeBuffer(char*& r_buffer, uint32_t& r_length)
	{
		r_buffer = (char *)realloc(m_buffer, m_length);
		r_length = m_length;
	
		m_buffer = NULL;
		m_length = 0;
		m_capacity = 0;
		m_pointer = 0;
	}
	
	void WriteAt(uint32_t p_pos, const void *p_buffer, uint32_t p_length)
	{
		memcpy(m_buffer + p_pos, p_buffer, p_length);
	}
	
	void Close(void)
	{
		if (m_capacity != 0)
			free(m_buffer);
		delete this;
	}
	
	bool Read(void *p_buffer, uint32_t p_length, uint32_t& r_read)
	{
		r_read = MCU_min(p_length, m_length - m_pointer);
		memcpy(p_buffer, m_buffer + m_pointer, r_read);
		m_pointer += r_read;
		return true;
	}
	
	bool Write(const void *p_buffer, uint32_t p_length, uint32_t& r_written)
	{
		// If we aren't writable then its an error (writable buffers start off with
		// nil buffer pointer, and 0 capacity).
		if (m_buffer != NULL && m_capacity == 0)
			return false;
		
		// If there isn't enough room, extend
		if (m_pointer + p_length > m_capacity)
		{
			uint32_t t_new_capacity;
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
		m_length = MCU_max(m_pointer, m_length);
		r_written = p_length;
		
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
		if (t_new_offset < 0 || t_new_offset > m_length)
			return false;
		
		m_pointer = (uint32_t)t_new_offset;
		return true;
	}
	
	bool PutBack(char c)
	{
		if (m_pointer == 0)
			return false;
		
		m_pointer -= 1;
		return true;
	}
	
	int64_t Tell(void)
	{
		return m_pointer;
	}
	
	int64_t GetFileSize(void)
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
	
private:
	char *m_buffer;
	uint32_t m_pointer;
	uint32_t m_length;
	uint32_t m_capacity;
};

IO_handle MCS_fakeopen(const MCString& p_data)
{
	MCMemoryFileHandle *t_handle;
	t_handle = new MCMemoryFileHandle(p_data);
	return new IO_header(t_handle, IO_FAKE);
}

IO_handle MCS_fakeopenwrite(void)
{
	MCMemoryFileHandle *t_handle;
	t_handle = new MCMemoryFileHandle();
	return new IO_header(t_handle, IO_FAKEWRITE);
}

IO_stat MCS_fakeclosewrite(IO_handle& p_stream, char*& r_buffer, uint4& r_length)
{
	if ((p_stream -> flags & IO_FAKEWRITE) != IO_FAKEWRITE)
	{
		r_buffer = NULL;
		r_length = 0;
		MCS_close(p_stream);
		return IO_ERROR;
	}
	
	MCMemoryFileHandle *t_handle;
	t_handle = static_cast<MCMemoryFileHandle *>(p_stream -> handle);
	t_handle -> TakeBuffer(r_buffer, r_length);
	
	MCS_close(p_stream);
	
	return IO_NORMAL;
}

uint32_t MCS_faketell(IO_handle stream)
{
	return (uint32_t)static_cast<MCSystemFileHandle *>(stream -> handle) -> Tell();
}

bool MCS_isfake(IO_handle p_stream)
{
	return (p_stream -> flags & IO_FAKEWRITE) != 0;
}

void MCS_fakewriteat(IO_handle p_stream, uint4 p_pos, const void *p_buffer, uint4 p_size)
{
	MCMemoryFileHandle *t_handle;
	t_handle = static_cast<MCMemoryFileHandle *>(p_stream -> handle);
	t_handle -> WriteAt(p_pos, p_buffer, p_size);
}

const char *MCS_tmpnam(void)
{
	static char *t_file = NULL;
	
	delete t_file;
	t_file = MCsystem -> GetTemporaryFileName();
	
	return t_file;
}

////////////////////////////////////////////////////////////////////////////////

class MCCustomFileHandle: public MCSystemFileHandle
{
public:
	MCCustomFileHandle(MCFakeOpenCallbacks *p_callbacks, void *p_state)
	{
		m_state = p_state;
		m_callbacks = p_callbacks;
	}
	
	void WriteAt(uint32_t p_pos, const void *p_buffer, uint32_t p_length)
	{
	}
	
	void Close(void)
	{
		// MW-2011-06-12: Fix memory leak - Close() should delete the handle.
		delete this;
	}
	
	bool Read(void *p_buffer, uint32_t p_length, uint32_t& r_read)
	{
		if (m_callbacks -> read == nil)
			return false;
		return m_callbacks -> read(m_state, p_buffer, p_length, r_read) == IO_NORMAL;
	}
	
	bool Write(const void *p_buffer, uint32_t p_length, uint32_t& r_written)
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
	
	bool PutBack(char c)
	{
		return false;
	}
	
	int64_t Tell(void)
	{
		return m_callbacks -> tell(m_state);
	}
	
	int64_t GetFileSize(void)
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
};

IO_handle MCS_fakeopencustom(MCFakeOpenCallbacks *p_callbacks, void *p_state)
{
	MCSystemFileHandle *t_handle;
	t_handle = new MCCustomFileHandle(p_callbacks, p_state);
	return new IO_header(t_handle, 0);
}

IO_handle MCS_open(const char *p_path, const char *p_mode, Boolean p_map, Boolean p_driver, uint4 p_offset)
{
	char *t_resolved_path;
	t_resolved_path = MCS_resolvepath(p_path);
	
	uint32_t t_mode;
	if (strequal(p_mode, IO_READ_MODE))
		t_mode = kMCSystemFileModeRead;
	else if (strequal(p_mode, IO_WRITE_MODE))
		t_mode = kMCSystemFileModeWrite;
	else if (strequal(p_mode, IO_UPDATE_MODE))
		t_mode = kMCSystemFileModeUpdate;
	else if (strequal(p_mode, IO_APPEND_MODE))
		t_mode = kMCSystemFileModeAppend;
	
	MCSystemFileHandle *t_handle;
	if (!p_driver)
		t_handle = MCsystem -> OpenFile(t_resolved_path, t_mode, p_map && MCmmap);
	else
		t_handle = MCsystem -> OpenDevice(t_resolved_path, t_mode, MCserialcontrolsettings);
	
	// MW-2011-06-12: Fix memory leak - make sure we delete the resolved path.
	delete t_resolved_path;
	
	if (t_handle == NULL)
		return NULL;
	
	if (p_offset != 0)
		t_handle -> Seek(p_offset, 1);
	
	return new IO_header(t_handle, 0);;
}

IO_stat MCS_close(IO_handle& p_stream)
{
	p_stream -> handle -> Close();
	delete p_stream;
	p_stream = NULL;
	return IO_NORMAL;
}

IO_stat MCS_putback(char p_char, IO_handle p_stream)
{
	if (!p_stream -> handle -> PutBack(p_char))
		return IO_ERROR;
	return IO_NORMAL;
}

IO_stat MCS_seek_cur(IO_handle p_stream, int64_t p_offset)
{
	if (!p_stream -> handle -> Seek(p_offset, 0))
		return IO_ERROR;
	return IO_NORMAL;
}

IO_stat MCS_seek_set(IO_handle p_stream, int64_t p_offset)
{
	if (!p_stream -> handle -> Seek(p_offset, 1))
		return IO_ERROR;
	return IO_NORMAL;
}

IO_stat MCS_seek_end(IO_handle p_stream, int64_t p_offset)
{
	if (!p_stream -> handle -> Seek(p_offset, -1))
		return IO_ERROR;
	return IO_NORMAL;
}

void MCS_loadfile(MCExecPoint& ep, Boolean p_binary)
{
	const char *t_filename;
	t_filename = ep . getcstring();
	
	char *t_resolved_path;
	t_resolved_path = MCS_resolvepath(t_filename);
	
	MCSystemFileHandle *t_file;
	t_file = MCsystem -> OpenFile(t_resolved_path, kMCSystemFileModeRead, false);
	delete t_resolved_path;
	if (t_file == NULL)
	{
		// MW-2011-05-23: [[ Bug 9549 ]] Make sure we empty the result if opening the file
		//   failed.
		ep . clear();
		MCresult -> sets("can't open file");
		return;
	}
	
	uint32_t t_size;
	t_size = (uint32_t)t_file -> GetFileSize();
	
	char *t_buffer;
	t_buffer = ep . getbuffer(t_size);
	
	uint32_t t_read;
	if (t_buffer != NULL &&
		t_file -> Read(t_buffer, t_size, t_read) &&
		t_read == t_size)
	{
		ep . setlength(t_size);
		if (!p_binary)
			ep . texttobinary();
		MCresult -> clear(False);
	}
	else
	{
		ep . clear();
		MCresult -> sets("error reading file");
	}

	t_file -> Close();
}

void MCS_savefile(const MCString& p_filename, MCExecPoint& p_data, Boolean p_binary)
{
	char *t_filename;
	t_filename = p_filename . clone();
	
	char *t_resolved_path;
	t_resolved_path = MCS_resolvepath(t_filename);
	delete t_filename;
	
	MCSystemFileHandle *t_file;
	t_file = MCsystem -> OpenFile(t_resolved_path, kMCSystemFileModeWrite, false);
	delete t_resolved_path;
	if (t_file == NULL)
	{
		MCresult -> sets("can't open file");
		return;
	}
	
	if (!p_binary)
		p_data . binarytotext();
	
	uint32_t t_written;
	if (!t_file -> Write(p_data . getsvalue() . getstring(), p_data . getsvalue() . getlength(), t_written) ||
		p_data . getsvalue() . getlength() != t_written)
		MCresult -> sets("error writing file");
	else
		MCresult -> clear();
	
	t_file -> Close();
}

void MCS_loadresfile(MCExecPoint& ep)
{
	ep . clear();
	MCresult -> sets("not supported");
}

void MCS_saveresfile(const MCString& p_filename, const MCString p_data)
{
	MCresult -> sets("not supported");
}

int64_t MCS_fsize(IO_handle p_stream)
{
	return p_stream -> handle -> GetFileSize();
}

IO_stat MCS_read(void *p_ptr, uint4 p_size, uint4& p_count, IO_handle p_stream)
{
	if (MCabortscript || p_ptr == NULL || p_stream == NULL)
		return IO_ERROR;
	
	if ((p_stream -> flags & IO_FAKEWRITE) == IO_FAKEWRITE)
		return IO_ERROR;
	
	uint32_t t_to_read;
	t_to_read = p_size * p_count;
	
	uint32_t t_read;
	if (!p_stream -> handle -> Read(p_ptr, t_to_read, t_read))
	{
		p_count = t_read / p_size;
		return IO_ERROR;
	}
	
	p_count = t_read / p_size;
	
	if (t_read < t_to_read)
	{
		p_stream -> flags |= IO_ATEOF;
		p_count = t_read / p_size;
		return IO_EOF;
	}
	
	p_stream -> flags &= ~IO_ATEOF;
	
	return IO_NORMAL;
}

IO_stat MCS_write(const void *p_ptr, uint32_t p_size, uint32_t p_count, IO_handle p_stream)
{
	if (p_stream == IO_stdin)
		return IO_NORMAL;
	
	if (p_stream == NULL)
		return IO_ERROR;
	
	uint32_t t_to_write;
	t_to_write = p_size * p_count;
	
	uint32_t t_written;
	if (!p_stream -> handle -> Write(p_ptr, t_to_write, t_written) ||
		t_to_write != t_written)
		return IO_ERROR;
	
	return IO_NORMAL;
}

IO_stat MCS_flush(IO_handle p_stream)
{
	if (!p_stream -> handle -> Flush())
		return IO_ERROR;
	return IO_NORMAL;
}

IO_stat MCS_trunc(IO_handle p_stream)
{
	if (!p_stream -> handle -> Truncate())
		return IO_ERROR;
	return IO_NORMAL;
}

int64_t MCS_tell(IO_handle p_stream)
{
	return p_stream -> handle -> Tell();
}

IO_stat MCS_sync(IO_handle p_stream)
{
	if (!p_stream -> handle -> Sync())
		return IO_ERROR;
	return IO_NORMAL;
}

Boolean MCS_eof(IO_handle p_stream)
{
	return (p_stream -> flags & IO_ATEOF) != 0;
}

char *MCS_get_canonical_path(const char *p_path)
{
	char *t_path = NULL;
	
	t_path = MCS_resolvepath(p_path);
	MCU_fix_path(t_path);
	
	return t_path;
}

////////////////////////////////////////////////////////////////////////////////

void MCS_send(const MCString &message, const char *program, const char *eventtype, Boolean reply)
{
	MCresult->sets("not supported");
}

void MCS_reply(const MCString &message, const char *keyword, Boolean error)
{
	MCresult->sets("not supported");
}

char *MCS_request_ae(const MCString &message, uint2 ae)
{
	MCresult->sets("not supported");
	return NULL;
}

char *MCS_request_program(const MCString &message, const char *program)
{
	MCresult->sets("not supported");
	return NULL;
}

////////////////////////////////////////////////////////////////////////////////

IO_stat MCS_runcmd(MCExecPoint& ep)
{
	void *t_data;
	uint32_t t_data_length;
	int t_return_code;

	if (!MCsystem -> Shell(ep . getsvalue() . getstring(), ep . getsvalue() . getlength(), t_data, t_data_length, t_return_code))
	{
		MCresult -> clear(False);
		MCeerror -> add(EE_SHELL_BADCOMMAND, 0, 0, ep.getsvalue());
		return IO_ERROR;
	}
	ep . grabbuffer((char *)t_data, t_data_length);
	
	// MW-2013-08-07: [[ Bug 11089 ]] The MCSystem::Shell() call returns binary data,
	//   so since uses of MCS_runcmd() expect text, we need to do EOL conversion.
	ep . texttobinary();
	
	MCresult -> setnvalue(t_return_code);
	
	return IO_NORMAL;
}

void MCS_startprocess(char *name, char *doc, Open_mode mode, Boolean elevated)
{
	delete doc;
	delete name;
	MCresult -> sets("not opened");
}

void MCS_closeprocess(uint2 p_index)
{
}

void MCS_checkprocesses(void)
{
}

void MCS_kill(int4 p_pid, int4 p_signal)
{
}

void MCS_killall(void)
{
}

////////////////////////////////////////////////////////////////////////////////

void MCS_multibytetounicode(const char *p_mb_string, uint4 p_mb_length, char *p_buffer, uint4 p_available, uint4& r_used, uint1 p_charset)
{	
	r_used = MCsystem -> TextConvert(p_mb_string, p_mb_length, p_buffer, p_available, p_charset, LCH_UNICODE);
}

void MCS_unicodetomultibyte(const char *p_u_string, uint4 p_u_length, char *p_buffer, uint4 p_available, uint4& r_used, uint1 p_charset)
{
	r_used = MCsystem -> TextConvert(p_u_string, p_u_length, p_buffer, p_available, LCH_UNICODE, p_charset);
}

bool MCSTextConvertToUnicode(MCTextEncoding p_encoding, const void *p_input, uint4 p_input_length, void *p_output, uint4 p_output_length, uint4& r_used)
{
	return MCsystem -> TextConvertToUnicode(p_encoding, p_input, p_input_length, p_output, p_output_length, r_used);
}

////////////////////////////////////////////////////////////////////////////////

MCSocket::~MCSocket(void)
{
}

MCSocket::MCSocket(char *n, MCObject *o, MCNameRef m, Boolean d, MCSocketHandle sock,Boolean a, Boolean s, Boolean issecure)
{
}

void MCSocket::close(void)
{
}

void MCSocket::doclose(void)
{
}

Boolean MCSocket::init(MCSocketHandle newfd)
{
	return False;
}

MCSocket *MCS_open_socket(char *p_name, Boolean p_datagram, MCObject *p_object, MCNameRef p_message, Boolean p_secure, Boolean p_ssl_verify, char *p_ssl_cert_file)
{
	return NULL;
}

void MCS_close_socket(MCSocket *p_socket)
{
}

void MCS_read_socket(MCSocket *p_socket, MCExecPoint& ep, uint4 p_length, char *p_until, MCNameRef p_message)
{
}

void MCS_write_socket(const MCString& p_data, MCSocket *p_socket, MCObject *p_object, MCNameRef p_message)
{
}

MCSocket *MCS_accept(uint2 p_port, MCObject* p_object, MCNameRef p_message, Boolean p_datagram, Boolean p_secure, Boolean p_ssl_verify, char *p_ssl_cert_file)
{
	return NULL;
}

void MCS_ha(MCExecPoint& ep, MCSocket *p_socket)
{
}

void MCS_pa(MCExecPoint& ep, MCSocket *p_socket)
{
}

void MCS_hn(MCExecPoint& ep)
{
	char *t_host_name;
	t_host_name = MCsystem -> GetHostName();
	if (t_host_name == NULL)
		ep . clear();
	else
		ep . copysvalue(t_host_name, strlen(t_host_name));
	delete t_host_name;
}

static bool MCS_resolve_callback(void *p_context, const char *p_host)
{
	MCExecPoint *ep;
	ep = static_cast<MCExecPoint *>(p_context);
	ep -> concatcstring(p_host, EC_RETURN, ep -> getsvalue() . getlength() == 0);
	return true;
}

void MCS_aton(MCExecPoint& ep)
{
	char *t_address;
	t_address = ep . getsvalue() . clone();
	strtok(t_address, "|:");
	
	ep . clear();
	if (!MCsystem -> AddressToHostName(t_address, MCS_resolve_callback, &ep))
		MCresult -> sets("invalid host address");
	else
		MCresult -> clear();
	
	delete t_address;
}

void MCS_ntoa(MCExecPoint& ep, MCExecPoint& ep2)
{
	if (!ep2.isempty())
	{
		MCresult -> sets("not supported");
		return;
	}
	
	char *t_hostname;
	t_hostname = ep . getsvalue() . clone();
	strtok(t_hostname, "|:");
	
	ep . clear();
	if (!MCsystem -> HostNameToAddress(t_hostname, MCS_resolve_callback, &ep))
		MCresult -> sets("invalid host name");
	else
		MCresult -> clear();
	
	delete t_hostname;
}

void MCS_getDNSservers(MCExecPoint& ep)
{
}

char *MCS_dnsresolve(const char *p_hostname)
{
	return NULL;
}

char *MCS_hostaddress(void)
{
	return NULL;
}

////////////////////////////////////////////////////////////////////////////////

Boolean MCS_poll(real8 p_delay, int fd)
{
	MCsystem -> Sleep(p_delay);
	return False;
}

////////////////////////////////////////////////////////////////////////////////

MCSysModuleHandle MCS_loadmodule(const char *p_filename)
{
	return (MCSysModuleHandle)MCsystem -> LoadModule(p_filename);
}

void *MCS_resolvemodulesymbol(MCSysModuleHandle p_module, const char *p_symbol)
{
	return MCsystem -> ResolveModuleSymbol(p_module, p_symbol);
}

void MCS_unloadmodule(MCSysModuleHandle p_module)
{
	MCsystem -> UnloadModule(p_module);
}

////////////////////////////////////////////////////////////////////////////////

bool MCS_changeprocesstype(bool x)
{
	return x;
}

bool MCS_processtypeisforeground(void)
{
	return true;
}

////////////////////////////////////////////////////////////////////////////////

bool MCS_isatty(int fd)
{
	return false;
}

////////////////////////////////////////////////////////////////////////////////

void MCS_system_alert(const char *p_title, const char *p_message)
{
}

uint32_t MCS_getsyserror(void)
{
	return errno;
}

bool MCS_mcisendstring(const char *p_command, char p_buffer[])
{
	strcpy(p_buffer, "not supported");
	return true;
}