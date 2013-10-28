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

#include "sysdefs.h"
#include "system.h"
#include "parsedef.h"
#include "filedefs.h"
#include "osspec.h"

#include "util.h"
#include "mcssl.h"

#include "variable.h"
#include "execpt.h"

#include "srvmain.h"
#include "srvsession.h"

typedef struct
{
	char *					save_path;
	MCSystemFileHandle *	file;
	
	uint32_t				session_count;
	MCSession **			session;
} MCSessionIndex, *MCSessionIndexRef;

////////////////////////////////////////////////////////////////////////////////

bool MCSystemLockFile(MCSystemFileHandle *p_file, bool p_shared, bool p_wait);
bool MCServerSetCookie(const MCString &p_name, const MCString &p_value, uint32_t p_expires, const MCString &p_path, const MCString &p_domain, bool p_secure, bool p_http_only);

////////////////////////////////////////////////////////////////////////////////

bool MCSessionGenerateID(char *&r_id);
void MCSessionRefreshExpireTime(MCSession *p_session);

bool MCSessionOpenIndex(MCSessionIndexRef &r_index);
bool MCSessionCloseIndex(MCSessionIndexRef p_index, bool p_update);
void MCSessionDisposeIndex(MCSessionIndexRef p_index);
bool MCSessionIndexAddSession(MCSessionIndexRef p_index, MCSession *p_session);

bool MCSessionWriteIndex(MCSessionIndexRef p_index);
bool MCSessionReadIndex(MCSessionIndexRef p_index);

bool MCSessionOpenSession(MCSessionIndexRef p_index, MCSession *p_session);
bool MCSessionCreateSession(MCSessionIndexRef p_index, const char *p_session_id, MCSession *&r_session);
bool MCSessionCloseSession(MCSession *p_session, bool p_update);
void MCSessionDisposeSession(MCSession *p_session);
bool MCSessionCopySession(MCSession *p_src, MCSession *&r_dst);
bool MCSessionWriteSession(MCSession *p_session);
bool MCSessionReadSession(MCSession *p_session);

////////////////////////////////////////////////////////////////////////////////

bool MCSessionOpenIndex(MCSessionIndexRef &r_index)
{
	bool t_success = true;
	
	MCSessionIndexRef t_index = nil;
	
	char *t_path = nil;

	t_success = MCMemoryNew(t_index);
	
	MCAutoStringRef t_save_path;
	if (t_success)
		t_success = MCS_get_session_save_path(&t_save_path);
	
	if (t_success)
		t_success = MCCStringClone(MCStringGetCString(*t_save_path), t_index->save_path);
	
	MCAutoStringRef t_path_string;
	if (t_success)
		t_success = MCStringFormat(&t_path_string, "%s/lcsessions.idx", t_index->save_path);
	
	// open file
	if (t_success)
		t_success = NULL != (t_index->file = MCsystem->OpenFile(*t_path_string, kMCSystemFileModeUpdate, false));

	// lock file
	if (t_success)
		t_success = MCSystemLockFile(t_index->file, false, true);
	
	// read index
	if (t_success && t_index->file->GetFileSize() > 0)
		t_success = MCSessionReadIndex(t_index);
	
	if (t_success)
		r_index = t_index;
	else
		MCSessionCloseIndex(t_index, false);
	
	return t_success;
}

void MCSessionDisposeIndex(MCSessionIndexRef p_index)
{
	if (p_index == NULL)
		return;
	
	MCCStringFree(p_index->save_path);
	
	if (p_index->session != NULL)
	{
		for (uint32_t i = 0; i < p_index->session_count; i++)
		{
			MCSessionDisposeSession(p_index->session[i]);
		}
	}
	MCMemoryDelete(p_index);
}

bool MCSessionCloseIndex(MCSessionIndexRef p_index, bool p_update)
{
	bool t_success = true;
	
	if (p_index == NULL)
		return true;
	
	if (p_index->file != NULL)
	{
		if (p_update)
		{
			// write data to file
			t_success = p_index->file->Seek(0, 1);
			if (t_success)
				t_success = MCSessionWriteIndex(p_index);
			if (t_success)
				t_success = p_index->file->Flush();
			if (t_success)
				t_success = p_index->file->Truncate();
		}
		p_index->file->Close();
	}
	
	MCSessionDisposeIndex(p_index);
	
	return t_success;
}

bool MCSessionIndexAddSession(MCSessionIndexRef p_index, MCSession *p_session)
{
	bool t_success = true;
	t_success = MCMemoryResizeArray(p_index->session_count + 1, p_index->session, p_index->session_count);
	if (t_success)
		t_success = MCSessionCopySession(p_session, p_index->session[p_index->session_count - 1]);
	
	return t_success;
}

bool MCSessionIndexRemoveSession(MCSessionIndexRef p_index, MCSession *p_session)
{
	for (uint32_t i = 0; i < p_index->session_count; i++)
	{
		if (MCCStringEqual(p_session->id, p_index->session[i]->id) && MCCStringEqual(p_session->ip, p_index->session[i]->ip))
		{
			MCSessionDisposeSession(p_index->session[i]);
			MCMemoryMove(p_index->session + i, p_index->session + i + 1, (p_index->session_count - i - 1) * sizeof(MCSession*));
			p_index->session_count -= 1;
			
			return true;
		}
	}
	
	return false;
}

////////

bool write_uint32(MCSystemFileHandle *p_file, uint32_t p_val)
{
	uint32_t t_written;
	return p_file->Write(&p_val, sizeof(p_val), t_written) && t_written == sizeof(p_val);
}

bool write_real64(MCSystemFileHandle *p_file, real64_t p_val)
{
	uint32_t t_written;
	return p_file->Write(&p_val, sizeof(p_val), t_written) && t_written == sizeof(p_val);
}

bool write_cstring(MCSystemFileHandle *p_file, const char *p_string)
{
	uint32_t t_written;
	
	uint32_t t_strlen = MCCStringLength(p_string);
	if (!write_uint32(p_file, t_strlen))
		return false;

	return p_file->Write(p_string, t_strlen, t_written) && t_written == t_strlen;
}

bool write_binary(MCSystemFileHandle *p_file, void *p_data, uint32_t p_length)
{
	uint32_t t_written;
	
	if (!write_uint32(p_file, p_length))
		return false;
	
	return p_file->Write(p_data, p_length, t_written) && t_written == p_length;
}

bool read_uint32(MCSystemFileHandle *p_file, uint32_t &r_val)
{
	uint32_t t_read;
	return p_file->Read(&r_val, sizeof(r_val), t_read) && t_read == sizeof(r_val);
}

bool read_real64(MCSystemFileHandle *p_file, real64_t &r_val)
{
	uint32_t t_read;
	return p_file->Read(&r_val, sizeof(r_val), t_read) && t_read == sizeof(r_val);
}

bool read_cstring(MCSystemFileHandle *p_file, char *&r_string)
{
	uint32_t t_strlen;
	if (!read_uint32(p_file, t_strlen))
		return false;
	
	uint32_t t_read;
	char *t_str = NULL;
	
	if (!MCMemoryAllocate(t_strlen + 1, t_str))
		return false;
	
	if (p_file->Read(t_str, t_strlen, t_read) && t_read == t_strlen)
	{
		t_str[t_strlen] = '\0';
		r_string = t_str;
		return true;
	}
	else
	{
		MCMemoryDeallocate(t_str);
		return false;
	}
}

bool read_binary(MCSystemFileHandle *p_file, void *&r_data, uint32_t &r_length)
{
	if (!read_uint32(p_file, r_length))
		return false;
	
	uint32_t t_read;
	void *t_data = NULL;
	
	if (!MCMemoryAllocate(r_length, t_data))
		return false;
	
	if (p_file->Read(t_data, r_length, t_read) && t_read == r_length)
	{
		r_data = t_data;
		return true;
	}
	else
	{
		MCMemoryDeallocate(t_data);
		return false;
	}
}

// session index file format:
// entry count (uint32_t)
// followed by entries of format:
// session id (string) + originating ip (string) + session filename (string) + expires (uint32_t seconds)

bool MCSessionWriteIndex(MCSessionIndexRef p_index)
{
	bool t_success = true;
	
	t_success = write_uint32(p_index->file, p_index->session_count);

	for (uint32_t i = 0; t_success && i < p_index->session_count; i++)
	{
		t_success = write_cstring(p_index->file, p_index->session[i]->id) &&
					write_cstring(p_index->file, p_index->session[i]->ip) &&
					write_cstring(p_index->file, p_index->session[i]->filename) &&
					write_real64(p_index->file, p_index->session[i]->expires);
	}
	
	return t_success;
}

bool MCSessionReadIndex(MCSessionIndexRef p_index)
{
	bool t_success = true;
	
	t_success = read_uint32(p_index->file, p_index->session_count);
	if (t_success)
		t_success = MCMemoryNewArray(p_index->session_count, p_index->session);

	for (uint32_t i = 0; t_success && i < p_index->session_count; i++)
	{
		t_success = MCMemoryNew(p_index->session[i]);
		if (t_success)
			t_success = read_cstring(p_index->file, p_index->session[i]->id) &&
						read_cstring(p_index->file, p_index->session[i]->ip) &&
						read_cstring(p_index->file, p_index->session[i]->filename) &&
						read_real64(p_index->file, p_index->session[i]->expires);
	}

	return t_success;
}

// session file format:
// entry count (uint32_t)
// followed by entries of format:
// entry key (binary data) + entry value (binary data)

bool MCSessionWriteSession(MCSession *p_session)
{
	bool t_success = true;
	
	t_success = write_binary(p_session->filehandle, p_session->data, p_session->data_length);

	return t_success;
}

bool MCSessionReadSession(MCSession *p_session)
{
	bool t_success = true;
	
	t_success = read_binary(p_session->filehandle, (void*&)p_session->data, p_session->data_length);
	
	return t_success;
}

////////////////////////////////////////////////////////////////////////////////

bool MCSessionFindMatchingSession(MCSessionIndexRef p_index, const char *p_session_id, MCSession *&r_session)
{
	MCAutoStringRef t_remote_addr_str;
	
	const char *t_remote_addr = NULL;

	if (!MCS_getenv(MCSTR("REMOTE_ADDR"), &t_remote_addr_str))
		t_remote_addr = "";
	else
		t_remote_addr = MCStringGetCString(*t_remote_addr_str);

	for (uint32_t i = 0; i < p_index->session_count; i++)
	{
		if (MCCStringEqual(p_session_id, p_index->session[i]->id) && MCCStringEqual(p_index->session[i]->ip, t_remote_addr))
		{
			r_session = p_index->session[i];
			return true;
		}
	}
	
	return false;
}

bool MCSessionOpenSession(MCSessionIndexRef p_index, MCSession *p_session)
{
	bool t_success = true;
	
	MCAutoStringRef t_path_string;
	t_success = MCStringFormat(&t_path_string, "%s/%s", p_index->save_path, p_session->filename);
	
	if (t_success)
		t_success = NULL != (p_session->filehandle = MCsystem->OpenFile(*t_path_string, kMCSystemFileModeUpdate, false));
	
	if (t_success)
		t_success = MCSystemLockFile(p_session->filehandle, false, true);
	
	if (t_success && p_session->filehandle->GetFileSize() > 0 && p_session->expires > MCS_time())
		t_success = MCSessionReadSession(p_session);
	
	return t_success;
}

bool MCSessionCreateSession(MCSessionIndexRef p_index, const char *p_session_id, MCSession *&r_session)
{
	bool t_success = true;
	
	MCSession *t_session = NULL;

	MCAutoStringRef t_remote_addr_string;
	char *t_remote_addr;

	if (MCS_getenv(MCSTR("REMOTE_ADDR"), &t_remote_addr_string))
		MCCStringClone(MCStringGetCString(*t_remote_addr_string), t_remote_addr);
	
	t_success = MCMemoryNew(t_session);
	
	if (t_success)
		t_success = MCCStringClone(t_remote_addr ? t_remote_addr : "", t_session->ip);

	if (t_success)
	{
		if (p_session_id != NULL && p_session_id[0] != '\0')
			t_success = MCCStringClone(p_session_id, t_session->id);
		else
			t_success = MCSessionGenerateID(t_session->id);
	}
	
	if (t_success)
		t_success = MCCStringFormat(t_session->filename, "%s_%s", t_remote_addr ? t_remote_addr : "", t_session->id);
	
	if (t_success)
		t_success = MCSessionIndexAddSession(p_index, t_session);
	
	if (t_success)
		r_session = t_session;
	else
	{
		if (t_session != NULL)
			MCSessionCloseSession(t_session, false);
	}
	
	return t_success;
}

bool MCSessionCopySession(MCSession *p_src, MCSession *&r_dst)
{
	bool t_success = true;
	
	MCSession *t_session;
	t_success = MCMemoryNew(t_session);
	
	if (t_success)
		t_success = MCCStringClone(p_src->id, t_session->id);
	if (t_success)
		t_success = MCCStringClone(p_src->ip, t_session->ip);
	if (t_success)
		t_success = MCCStringClone(p_src->filename, t_session->filename);
	if (t_success)
		t_session->expires = p_src->expires;
	
	if (t_success)
		r_dst = t_session;
	else
		MCSessionDisposeSession(t_session);
	
	return t_success;
}

void MCSessionDisposeSession(MCSession *p_session)
{
	if (p_session == NULL)
		return;
	
	MCCStringFree(p_session->id);
	MCCStringFree(p_session->ip);
	MCCStringFree(p_session->filename);

	MCMemoryDeallocate(p_session->data);
	
	MCMemoryDelete(p_session);
}

bool MCSessionCloseSession(MCSession *p_session, bool p_update)
{
	bool t_success = true;
	
	if (p_session == NULL)
		return true;
	
	if (p_update)
	{
		MCSessionIndexRef t_index = NULL;
		MCSession *t_index_session = NULL;
		
		t_success = MCSessionOpenIndex(t_index);
		if (t_success)
		{
			if (!MCSessionFindMatchingSession(t_index, p_session->id, t_index_session))
			{
				t_success = MCSessionIndexAddSession(t_index, p_session);
				if (t_success)
					t_success = MCSessionFindMatchingSession(t_index, p_session->id, t_index_session);
			}
		}
		if (t_success)
			MCSessionRefreshExpireTime(t_index_session);

		if (t_success)
			t_success = MCSessionCloseIndex(t_index, true);
	}
	if (p_session->filehandle != NULL)
	{
		if (p_update)
		{
			if (t_success)
				t_success = p_session->filehandle->Seek(0, 1);
			if (t_success)
				t_success = MCSessionWriteSession(p_session);
			if (t_success)
				t_success = p_session->filehandle->Flush();
			if (t_success)
				t_success = p_session->filehandle->Truncate();
		}
		p_session->filehandle->Close();
	}
	
	MCSessionDisposeSession(p_session);
	
	return t_success;
}

////////////////////////////////////////////////////////////////////////////////

bool MCSessionStart(const char *p_session_id, MCSessionRef &r_session)
{
	bool t_success = true;
	
	MCSessionIndexRef t_index = NULL;
	MCSession *t_session = NULL;
	MCSession *t_index_session = NULL;
	
	t_success = MCSessionOpenIndex(t_index);
	
	if (t_success)
	{
		if (p_session_id != NULL  && MCSessionFindMatchingSession(t_index, p_session_id, t_index_session))
		{
			t_success = MCSessionCopySession(t_index_session, t_session);
		}
		else
		{
			t_success = MCSessionCreateSession(t_index, p_session_id, t_session);
		}
		
		MCAutoStringRef t_session_name;
		if (t_success)
			t_success = MCS_get_session_name(&t_session_name);
			
		if (t_success)
			t_success = MCServerSetCookie(MCStringGetOldString(*t_session_name), t_session->id, 0, NULL, NULL, false, true);
		
		if (t_success)
			t_success = MCSessionOpenSession(t_index, t_session);
		
		if (t_success)
			MCSessionRefreshExpireTime(t_session);
	}

	if (t_index != NULL)
		t_success &= MCSessionCloseIndex(t_index, t_success);
	
	if (t_success)
		r_session = t_session;
	else
	{
		if (t_session != NULL)
			MCSessionCloseSession(t_session, false);
	}
	
	return t_success;
}

bool MCSessionCommit(MCSessionRef p_session)
{
	return MCSessionCloseSession(p_session, true);
}

void MCSessionDiscard(MCSessionRef p_session)
{
	MCSessionCloseSession(p_session, false);
}
bool MCSessionExpireSession(const char *p_id)
{
	bool t_success = true;
	
	MCSession *t_session = NULL;
	MCSessionIndexRef t_index = NULL;
	
	t_success = MCSessionOpenIndex(t_index);
	if (t_success)
	{
		if (MCSessionFindMatchingSession(t_index, p_id, t_session))
			t_session->expires = MCS_time() - 60 * 60 * 24;
	}
	if (t_index)
		t_success &= MCSessionCloseIndex(t_index, t_success);
	
	return t_success;
}

bool MCSessionExpireCookie()
{
	MCAutoStringRef t_session_name;
	/* UNCHECKED */ MCS_get_session_name(&t_session_name);
	
	MCString t_value("EXPIRE");
	MCString t_path(NULL, 0);
	MCString t_domain(NULL, 0);
	return MCServerSetCookie(MCStringGetOldString(*t_session_name), t_value, MCS_time() - 60 * 60 * 24, t_path, t_domain, false, true);
}

bool MCSessionExpire(const char *p_id)
{
	return MCSessionExpireSession(p_id) && MCSessionExpireCookie();
}

bool MCSessionCleanup(void)
{
	bool t_success = true;
	
	MCSessionIndexRef t_index = NULL;
	t_success = MCSessionOpenIndex(t_index);
	
	real8 t_time;
	t_time = MCS_time();
	
	for (uint32_t i = 0; t_success && i < t_index->session_count; i++)
	{
		if (t_index->session[i]->expires <= t_time)
		{
			bool t_deleted = false;
			// check file not locked
			MCSystemFileHandle *t_file;
			MCAutoStringRef t_full_path_string;
			if (MCStringFormat(&t_full_path_string, "%s/%s", t_index->save_path, t_index->session[i]->filename)  && MCS_exists(*t_full_path_string, True))
			{
				t_file = MCsystem->OpenFile(*t_full_path_string, kMCSystemFileModeRead, false);
				if (t_file != NULL)
				{
					bool t_locked = false;
					t_locked = MCSystemLockFile(t_file, false, false);
					t_file->Close();
					
					if (t_locked)
						t_deleted = MCsystem->DeleteFile(*t_full_path_string);
				}
			}
			else
				t_deleted = true;

			if (t_deleted)
				MCSessionIndexRemoveSession(t_index, t_index->session[i]);
		}
	}
	
	if (t_index != NULL)
		t_success &= MCSessionCloseIndex(t_index, t_success);
	
	return t_success;
}

////////////////////////////////////////////////////////////////////////////////

#include "md5.h"

static const char *s_hex_char = "0123456789ABCDEF";
bool byte_to_hex(uint8_t *p_src, uint32_t p_len, char *&r_hex)
{
	if (!MCMemoryNewArray(p_len * 2 + 1, r_hex))
		return false;
	
	char *t_dst = r_hex;
	for (uint32_t i = 0; i < p_len; i++)
	{
		*t_dst++ = s_hex_char[(p_src[i] >> 4)];
		*t_dst++ = s_hex_char[(p_src[i] & 0xF)];
	}
	// append terminating null
	*t_dst = '\0';
	
	return true;
}
				 
bool MCSessionGenerateID(char *&r_id)
{
	// php calculates session ids by hashing a string composed of REMOTE_ADDR, time in seconds & milliseconds, and a random value

	MCAutoStringRef t_remote_addr_string;
	char *t_remote_addr;

	if (MCS_getenv(MCSTR("REMOTE_ADDR"), &t_remote_addr_string))
		MCCStringClone(MCStringGetCString(*t_remote_addr_string), t_remote_addr);
		
	time_t t_time;
	time(&t_time);
	
	MCAutoDataRef t_randombytes;
    
    // MW-2013-05-21; [[ RandomBytes ]] Use system primitive rather than SSL
	//   directly.
	/* UNCHECKED */ MCU_random_bytes(64, &t_randombytes);
	
	md5_state_t t_state;
	md5_byte_t t_digest[16];
	md5_init(&t_state);
	if (t_remote_addr != NULL)
		md5_append(&t_state, (md5_byte_t *)t_remote_addr, MCCStringLength(t_remote_addr));
	md5_append(&t_state, (md5_byte_t *)&t_time, sizeof(t_time));
	md5_append(&t_state, (md5_byte_t *)MCDataGetBytePtr(*t_randombytes), 64);
	md5_finish(&t_state, t_digest);
	
	return byte_to_hex((uint8_t*)t_digest, 16, r_id);
}

void MCSessionRefreshExpireTime(MCSession *p_session)
{
	p_session->expires = MCS_time() + MCS_get_session_lifetime();
}