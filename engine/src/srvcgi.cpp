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

#include "core.h"
#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"
#include "mcio.h"

#include "execpt.h"
#include "scriptpt.h"
#include "globals.h"
#include "param.h"
#include "external.h"
#include "handler.h"
#include "unicode.h"
#include "date.h"

#include "system.h"
#include "srvmain.h"
#include "srvdebug.h"
#include "srvscript.h"
#include "srvcgi.h"

#include "srvmultipart.h"
#include "srvsession.h"

#ifndef _WINDOWS_SERVER
#include <unistd.h>
#endif

#ifdef _WINDOWS_SERVER
#define strncasecmp _strnicmp
#define strcasecmp _stricmp
#endif

////////////////////////////////////////////////////////////////////////////////

static bool MCservercgiheaders_sent = false;

// The CGI related synthetic environment variables.
static MCVariable *s_cgi_server;
static MCVariable *s_cgi_post;
static MCVariable *s_cgi_post_raw;
static MCVariable *s_cgi_post_binary;
static MCVariable *s_cgi_files;
static MCVariable *s_cgi_get;
static MCVariable *s_cgi_get_raw;
static MCVariable *s_cgi_get_binary;
static MCVariable *s_cgi_cookie;

static bool s_cgi_processed_post = false;

// rather than making stdin / $_POST_RAW / $_POST, $_POST_BINARY, $_FILES
// exclusive, we store the stream contents in this cache object and create a
// cache reader handle around it when reading from stdin
class MCStreamCache;
static MCStreamCache *s_cgi_stdin_cache;

////////////////////////////////////////////////////////////////////////////////

static bool cgi_send_cookies(void);
static bool cgi_send_headers(void);

#ifndef _LINUX_SERVER
static char *strndup(const char *s, uint32_t n)
{
	char *r;
	r = (char *)malloc(n + 1);
	strncpy(r, s, n);
	r[n] = '\0';
	return r;
}
#endif

////////////////////////////////////////////////////////////////////////////////

static char *s_cgi_upload_temp_dir = NULL;
static char *s_cgi_temp_dir = NULL;

bool MCS_get_temporary_folder(char *&r_temp_folder);

static const char *cgi_get_upload_temp_dir()
{
	if (s_cgi_upload_temp_dir != NULL)
		return s_cgi_upload_temp_dir;
	
	if (s_cgi_temp_dir != NULL)
		return s_cgi_temp_dir;
	
	char *t_temp_folder = NULL;
	if (MCS_get_temporary_folder(t_temp_folder))
		s_cgi_temp_dir = t_temp_folder;
	
	return s_cgi_temp_dir;
}

////////////////////////////////////////////////////////////////////////////////

class MCDelegateFileHandle: public MCSystemFileHandle
{
public:
	MCDelegateFileHandle(IO_handle p_delegate)
	{
		m_delegate = p_delegate;
	}
	
	void Close(void)
	{
		delete this;
	}
	
	bool Read(void *p_buffer, uint32_t p_length, uint32_t& r_read)
	{
		return m_delegate -> handle -> Read(p_buffer, p_length, r_read);
	}
	
	bool Write(const void *p_buffer, uint32_t p_length, uint32_t& r_written)
	{
		return m_delegate -> handle -> Write(p_buffer, p_length, r_written);
	}
	
	bool Seek(int64_t p_offset, int p_dir)
	{
		return m_delegate -> handle -> Seek(p_offset, p_dir);
	}
	
	bool Truncate(void)
	{
		return m_delegate -> handle -> Truncate();
	}
	
	bool Sync(void)
	{
		return m_delegate -> handle -> Sync();
	}
	
	bool Flush(void)
	{
		return m_delegate -> handle -> Flush();
	}
	
	bool PutBack(char p_char)
	{
		return m_delegate -> handle -> PutBack(p_char);
	}
	
	int64_t Tell(void)
	{
		return m_delegate -> handle -> Tell();
	}
	
	void *GetFilePointer(void)
	{
		return m_delegate -> handle -> GetFilePointer();
	}
	
	int64_t GetFileSize(void)
	{
		return m_delegate -> handle -> GetFileSize();
	}
	
protected:
	IO_handle m_delegate;
};

class cgi_stdout: public MCDelegateFileHandle
{
public:
	cgi_stdout(void)
		: MCDelegateFileHandle(IO_stdout)
	{
	}
	
	void Close(void)
	{
		IO_stdout = m_delegate;
		MCDelegateFileHandle::Close();
	}
	
	bool Write(const void *p_buffer, uint32_t p_length, uint32_t& r_written)
	{
		Close();

		if (!(cgi_send_cookies() && cgi_send_headers()))
			return false;
		
		return IO_stdout -> handle -> Write(p_buffer, p_length, r_written);
	}
};

////////////////////////////////////////////////////////////////////////////////

// caching object, stores stream data in memory unless larger than 64k, in which
// case, the cached stream is stored in a temporary file
class MCStreamCache
{
public:
	MCStreamCache(MCSystemFileHandle *p_source_stream);
	~MCStreamCache();
	
	bool Read(void *p_buffer, uint32_t p_offset, uint32_t p_length, uint32_t &r_read);
	bool Ensure(uint32_t p_offset);
	
private:
	bool ReadFromCache(void *p_buffer, uint32_t p_offset, uint32_t p_length, uint32_t &r_read);
	bool ReadFromStream(void *p_buffer, uint32_t p_length, uint32_t &r_read);
	bool AppendToCache(void *p_buffer, uint32_t p_length, uint32_t &r_written);
	
	static const uint32_t m_buffer_limit = 64 * 1024;
	static const uint32_t m_min_read = 1024;
	
	MCSystemFileHandle *m_source_stream;
	uint32_t m_cache_length;
	void *m_cache_buffer;
	IO_handle m_cache_file;
	const char *m_cache_filename;
};

MCStreamCache::MCStreamCache(MCSystemFileHandle *p_source_stream)
{
	m_source_stream = p_source_stream;
	m_cache_length = 0;
	m_cache_buffer = NULL;
	m_cache_file = NULL;
	m_cache_filename = NULL;
}

MCStreamCache::~MCStreamCache()
{
	if (m_cache_file)
		MCS_close(m_cache_file);
	if (m_cache_buffer)
		MCMemoryDeallocate(m_cache_buffer);
}

bool MCStreamCache::Read(void *p_buffer, uint32_t p_offset, uint32_t p_length, uint32_t &r_read)
{
	bool t_success = true;
	
	uint32_t t_to_read;
	t_to_read = 0;
	
	uint32_t t_read;
	t_read = 0;
	
	if (p_offset < m_cache_length)
	{
		t_to_read = MCMin(m_cache_length - p_offset, p_length);
		t_success = ReadFromCache(p_buffer, p_offset, t_to_read, t_read);
	}
	
	r_read = t_read;
	
	if (t_success && t_read != t_to_read)
		return true;
	
	if (t_success)
	{
		t_to_read = p_length - t_read;
		t_read = 0;
		if (t_to_read > 0)
			t_success = ReadFromStream((uint8_t*)p_buffer + r_read, t_to_read, t_read);
		
		r_read += t_read;
	}
	
	return t_success;
}

bool MCStreamCache::ReadFromCache(void *p_buffer, uint32_t p_offset, uint32_t p_length, uint32_t &r_read)
{
	uint32_t t_to_read;
	if (m_cache_buffer != NULL)
	{
		r_read = MCMin(p_length, m_cache_length - p_offset);
		MCMemoryCopy(p_buffer, (uint8_t*)m_cache_buffer + p_offset, r_read);
		return true;
	}
	else if (m_cache_file != NULL)
	{
		bool t_success = true;
		
		IO_stat t_status;
		
		t_success = (IO_NORMAL == MCS_seek_set(m_cache_file, p_offset));
		if (t_success)
			t_success = (IO_ERROR != MCS_read(p_buffer, 1, p_length, m_cache_file));
		
		if (t_success)
		{
			r_read = p_length;
		}
		
		return t_success;
	}
	
	r_read = 0;
	return true;
}

bool MCStreamCache::ReadFromStream(void *p_buffer, uint32_t p_length, uint32_t &r_read)
{
	bool t_success = true;
	
	t_success = m_source_stream->Read(p_buffer, p_length, r_read);
	
	uint32_t t_written = 0;
	
	if (t_success)
		t_success = AppendToCache(p_buffer, r_read, t_written);
	
	if (t_success)
		t_success = t_written == r_read;
	
	return t_success;
}

bool MCStreamCache::AppendToCache(void *p_buffer, uint32_t p_length, uint32_t &r_written)
{
	bool t_success = true;
	
	if (m_cache_length + p_length > m_buffer_limit)
	{
		if (m_cache_file == NULL)
		{
			t_success = MCMultiPartCreateTempFile(cgi_get_upload_temp_dir(), m_cache_file, m_cache_filename);
			if (t_success && m_cache_buffer != NULL)
				t_success = (IO_NORMAL == MCS_write(m_cache_buffer, 1, m_cache_length, m_cache_file));
			
			MCMemoryDeallocate(m_cache_buffer);
			m_cache_buffer = NULL;
		}
		
		if (t_success)
			t_success = (IO_NORMAL == MCS_write(p_buffer, 1, p_length, m_cache_file));
		
		m_cache_length += p_length;
		r_written = p_length;
	}
	else
	{
		if (m_cache_buffer == NULL)
			t_success = MCMemoryAllocate(m_buffer_limit, m_cache_buffer);
		if (t_success)
		{
			MCMemoryCopy((uint8_t*)m_cache_buffer + m_cache_length, p_buffer, p_length);
			m_cache_length += p_length;
			
			r_written = p_length;
		}
	}
		
	return t_success;
}

bool MCStreamCache::Ensure(uint32_t p_offset)
{
	if (p_offset <= m_cache_length)
		return true;
	
	bool t_success = true;
	
	void *t_buffer;
	
	t_success = MCMemoryAllocate(m_buffer_limit, t_buffer);
	
	while (t_success && p_offset > m_cache_length)
	{
		uint32_t t_to_read;
		uint32_t t_read;
		
		t_to_read = MCMin(p_offset - m_cache_length, m_buffer_limit);
		t_success = Read(t_buffer, m_cache_length, t_to_read, t_read) && (t_read == t_to_read);
	}
	
	MCMemoryDeallocate(t_buffer);
	
	return t_success;
}

////////

// file handle class which reads from a cache
class MCCacheHandle: public MCSystemFileHandle
{
public:
	MCCacheHandle(MCStreamCache *p_cache)
	{
		m_cache = p_cache;
		m_offset = 0;
	}
	
	bool Read(void *p_buffer, uint32_t p_length, uint32_t &r_read)
	{
		bool t_success = true;
		
		t_success = m_cache->Read(p_buffer, m_offset, p_length, r_read);
		m_offset += r_read;
		
		return t_success;
	}
	
	bool Write(const void *p_buffer, uint32_t p_length, uint32_t &r_written)
	{
		return false;
	}
	
	bool Seek(int64_t p_offset, int p_direction)
	{
		bool t_success = true;
		
		// don't allow seek from end
		if (p_direction < 0)
			t_success = false;
		
		int64_t t_offset = 0;
		
		if (t_success)
			t_success = m_cache->Ensure(t_offset);
		
		if (t_success)
			m_offset = t_offset;
		
		return t_success;
	}
	
	void Close(void)
	{
		delete this;
	}
	
	
	bool Truncate()
	{
		return false;
	}
	
	bool Sync()
	{
		return true;
	}
	
	bool Flush()
	{
		return true;
	}
	
	bool PutBack(char)
	{
		if (m_offset > 1)
		{
			m_offset -= 1;
			return true;
		}
		else
			return false;
	}
	
	int64_t Tell()
	{
		return m_offset;
	}
	
	void* GetFilePointer()
	{
		return false;
	}
	
	int64_t GetFileSize()
	{
		return 0;
	}

private:
	
	MCStreamCache *m_cache;
	uint32_t m_offset;
};

////////////////////////////////////////////////////////////////////////////////

const char *strchr_limit(const char *s, const char *l, char c)
{
	while(s < l && *s != c)
		s++;
	return s;
}

static int convxdigit(char c)
{
	c = MCS_tolower(c);
	if (isdigit(c))
		return c - '0';
	return 10 + (c - 'a');
}

static void cgi_unescape_url(const char *s, const char *l, char *&r_s, char*& r_l)
{
	char *rs, *rl;
	rs = new char[l - s + 1];
	rl = rs;
	
	while(s < l)
	{
		if (*s == '+')
		{
			*rl++ = ' ';
			s += 1;
		}
		else if (*s == '%')
		{
			if (l - s < 3)
				break;
			
			if (isxdigit(s[1]) && isxdigit(s[2]))
				*rl++ = (convxdigit(s[1]) << 4) | (convxdigit(s[2]));
			
			s += 3;
			
			if (rl - rs >= 2 && rl[-1] == 10 && rl[-2] == 13)
			{
				rl[-2] = 10;
				rl -= 1;
			}
		}
		else
			*rl++ = *s++;
	}
	
	r_s = rs;
	r_l = rl;
}

static void cgi_fetch_variable_value_for_key(MCVariable *p_variable, const char *p_key, uint32_t p_key_length, MCExecPoint &ep, MCVariableValue *&r_var_value)
{
	MCVariableValue *t_value;
	t_value = &p_variable -> getvalue();
	
	// Look for the initial sub-key and if it has no further subkeys, then just store straight away.
	const char *t_key;
	t_key = p_key;
	
	const char *t_key_end;
	t_key_end = strchr_limit(p_key, p_key + p_key_length, '[');
	if (t_key_end == p_key + p_key_length)
	{
		t_value -> lookup_element(ep, MCString(p_key, p_key_length), r_var_value);
		return;
	}
	
	// Fetch the initial sub-key and start iterating through subsequent ones.
	t_value -> lookup_element(ep, MCString(t_key, t_key_end - t_key), t_value);
	
	t_key = t_key_end + 1;
	while(t_key < p_key + p_key_length)
	{
		t_key_end = strchr_limit(t_key, p_key + p_key_length, ']');
		if (t_key_end == p_key + p_key_length)
			break;
		
		if (t_key_end == t_key)
		{
			// Its a numeric key we need
			uint32_t t_index;
			if (!t_value -> is_array())
				t_index = 1;
			else if (t_value -> get_array() -> issequence())
			{
				t_index = t_value -> get_array() -> getnfilled() + 1;
			}
			else
			{
				for(t_index = 1; t_value -> get_array() -> lookupindex(t_index, False) != NULL; t_index += 1)
					;
			}
			
			char t_buffer[U4L];
			sprintf(t_buffer, "%u", t_index);
			t_value -> lookup_element(ep, t_buffer, t_value);
		}
		else
		{
			// Its a named key
			t_value -> lookup_element(ep, MCString(t_key, t_key_end - t_key), t_value);
		}
		
		t_key = t_key_end + 1;
	}
	
	r_var_value = t_value;
}

static void cgi_store_control_value(MCVariable *p_variable, const char *p_key, uint32_t p_key_length, MCExecPoint& ep)
{
	MCVariableValue *t_value;
	cgi_fetch_variable_value_for_key(p_variable, p_key, p_key_length, ep, t_value);
	t_value -> store(ep);
}

static bool MCConvertNativeFromUTF16(const uint16_t *p_chars, uint32_t p_char_count, uint8_t*& r_output, uint32_t& r_output_length);
static bool MCConvertNativeFromWindows1252(const uint8_t *p_chars, uint32_t p_char_count, uint8_t*& r_output, uint32_t& r_output_length);
static bool MCConvertNativeFromMacRoman(const uint8_t *p_chars, uint32_t p_char_count, uint8_t*& r_output, uint32_t& r_output_length);
static bool MCConvertNativeFromISO8859_1(const uint8_t *p_chars, uint32_t p_char_count, uint8_t*& r_output, uint32_t& r_output_length);

extern int UTF8ToUnicode(const char *p_source_str, int p_source, uint2 *p_dest_str, int p_dest);

static bool cgi_native_from_encoding(MCSOutputTextEncoding p_encoding, const char *p_text, uint32_t p_text_length, char *&r_native, uint32_t &r_native_length)
{
	bool t_success = true;

	uint8_t *t_native = NULL;
	uint32_t t_native_length = 0;

	if (p_encoding == kMCSOutputTextEncodingUTF8)
	{
		int32_t t_unicode_length;
		t_unicode_length = UTF8ToUnicode(p_text, p_text_length, NULL, 0);
		
		uint16_t *t_unicode = NULL;
		t_success = MCMemoryAllocate(t_unicode_length, t_unicode);
		if (t_success)
		{
			UTF8ToUnicode(p_text, p_text_length, t_unicode, t_unicode_length);
			t_success = MCConvertNativeFromUTF16(t_unicode, t_unicode_length / 2, t_native, t_native_length);
		}
		MCMemoryDeallocate(t_unicode);
	}
	else if (p_encoding == kMCSOutputTextEncodingWindows1252)
		t_success = MCConvertNativeFromWindows1252((uint8_t*)p_text, p_text_length, t_native, t_native_length);
	else if (p_encoding == kMCSOutputTextEncodingMacRoman)
		t_success = MCConvertNativeFromMacRoman((uint8_t*)p_text, p_text_length, t_native, t_native_length);
	else if (p_encoding == kMCSOutputTextEncodingISO8859_1)
		t_success = MCConvertNativeFromISO8859_1((uint8_t*)p_text, p_text_length, t_native, t_native_length);

	if (t_success)
	{
		r_native = (char*)t_native;
		r_native_length = t_native_length;
	}

	return t_success;
}

static void cgi_store_data_urlencoded(MCExecPoint& ep, MCVariable *p_variable, const char *p_data_start, const char *p_data_end, bool p_native_encoding, char p_delimiter, bool p_remove_whitespace)
{
	const char *t_data;
	t_data = p_data_start;
	while(t_data < p_data_end)
	{
		const char *t_encoded_key;
		t_encoded_key = t_data;
		
		if (p_remove_whitespace)
			while (t_encoded_key[0] == ' ')
				t_encoded_key++;
		
		const char *t_end;
		t_end = strchr_limit(t_encoded_key, p_data_end, p_delimiter);
		
		const char *t_encoded_key_end;
		t_encoded_key_end = strchr_limit(t_encoded_key, t_end, '=');
		
		const char *t_encoded_value;
		if (t_encoded_key_end != t_end)
			t_encoded_value = t_encoded_key_end + 1;
		else
			t_encoded_value = t_encoded_key_end;
		
		char *t_key_start, *t_key_finish;
		cgi_unescape_url(t_encoded_key, t_encoded_key_end, t_key_start, t_key_finish);
		
		const char *t_encoded_value_end;
		t_encoded_value_end = t_end;
		
		if (p_remove_whitespace)
			while (t_encoded_value_end > t_encoded_value && *(t_encoded_value - 1) == ' ')
				t_encoded_value_end--;
		
		char *t_value_start, *t_value_finish;
		cgi_unescape_url(t_encoded_value, t_encoded_value_end, t_value_start, t_value_finish);
		
		// MM-2011-07-13: Added p_native_encoding flag that specifies if the text should 
		//   be converted from the outputTextEncoding to the native character set.
		// IM-2011-07-13 convert from MCserveroutputtextencoding to native
		if (!p_native_encoding || MCserveroutputtextencoding == kMCSOutputTextEncodingNative)
			ep . grabbuffer(t_value_start, t_value_finish - t_value_start);
		else
		{
			uint32_t t_native_length;
			char *t_native = NULL;
			if (cgi_native_from_encoding(MCserveroutputtextencoding, t_value_start, t_value_finish - t_value_start, t_native, t_native_length))
				ep . grabbuffer(t_native, t_native_length);
			MCCStringFree(t_value_start);
		}
		
		cgi_store_control_value(p_variable, t_key_start, t_key_finish - t_key_start, ep);
		delete t_key_start;
		
		if (t_end != p_data_end)
			t_end += 1;
		
		t_data = t_end;
	}	
}

static void cgi_store_cookie_urlencoded(MCExecPoint &ep, MCVariable *p_variable, const char *p_data_start, const char *p_data_end, bool p_native_encoding)
{
	return cgi_store_data_urlencoded(ep, p_variable, p_data_start, p_data_end, p_native_encoding, ';', true);
}

static void cgi_store_form_urlencoded(MCExecPoint& ep, MCVariable *p_variable, const char *p_data_start, const char *p_data_end, bool p_native_encoding)
{
	return cgi_store_data_urlencoded(ep, p_variable, p_data_start, p_data_end, p_native_encoding, '&', false);
}

static void cgi_fix_path_variables()
{
	char *t_path, *t_path_end;
    
    // SN-2014-07-29: [[ Bug 12865 ]] When a LiveCode CGI script has a .cgi extension and has
    //  the appropriate shebang pointing to the LiveCode server, PATH_TRANSLATED is not set by Apache.
    //  The current file (stored in SCRIPT_FILENAME) is the one containing the script.
    if (MCS_getenv("PATH_TRANSLATED") == NULL)
        t_path = strdup(MCS_getenv("SCRIPT_FILENAME"));
    else
        t_path = strdup(MCS_getenv("PATH_TRANSLATED"));
    
	t_path_end = t_path + strlen(t_path);

#ifdef _WINDOWS_SERVER
	for(uint32_t i = 0; t_path[i] != '\0'; i++)
		if (t_path[i] == '\\')
			t_path[i] = '/';
#endif

	char t_sep;
	t_sep = '\0';

	while (!MCS_exists(t_path, True))
	{
		char *t_new_end;
		t_new_end = strrchr(t_path, '/');
		*t_path_end = t_sep;
		if (t_new_end == NULL)
		{
			t_sep = '\0';
			break;
		}
		t_path_end = t_new_end;
		t_sep = *t_path_end;
		*t_path_end = '\0';
	}

	MCS_setenv("PATH_TRANSLATED", t_path);
	*t_path_end = t_sep;
	MCS_setenv("PATH_INFO", t_path_end);

	free(t_path);
}

static Exec_stat cgi_compute_get_var(void *p_context, MCVariable *p_var)
{
	MCExecPoint ep;

	const char *t_query_string;
	t_query_string = MCS_getenv("QUERY_STRING");
	if (t_query_string != NULL)
		cgi_store_form_urlencoded(ep, s_cgi_get, t_query_string, t_query_string + strlen(t_query_string), true);

	return ES_NORMAL;
}

// MM-2011-07-13: Added new deferred variable $_GET_RAW.
//   $_GET_RAW is just a copy of the QUERY_STRING.
static Exec_stat cgi_compute_get_raw_var(void *p_context, MCVariable *p_var)
{
	MCExecPoint ep;
	
	const char *t_query_string;
	t_query_string = MCS_getenv("QUERY_STRING");
	if (t_query_string != NULL)
		s_cgi_get_raw -> copysvalue(MCString(t_query_string, strlen(t_query_string)));
	
	return ES_NORMAL;
}

// MM-2011-07-13: Added new deferred variable $_GET_BINARY.
//   $_GET_BINARY is just the binary get data not encoded in the native charset.
static Exec_stat cgi_compute_get_binary_var(void *p_context, MCVariable *p_var)
{
	MCExecPoint ep;
	
	const char *t_query_string;
	t_query_string = MCS_getenv("QUERY_STRING");
	if (t_query_string != NULL)
		cgi_store_form_urlencoded(ep, s_cgi_get_binary, t_query_string, t_query_string + strlen(t_query_string), false);
	
	return ES_NORMAL;
}

// $_POST_RAW contains the entire post message and is read from stdin on access
static Exec_stat cgi_compute_post_raw_var(void *p_context, MCVariable *p_var)
{
	MCCacheHandle *t_stdin = new MCCacheHandle(s_cgi_stdin_cache);
	
	bool t_success = true;
	
	char *t_content_length;
	t_content_length = MCS_getenv("CONTENT_LENGTH");
	if (t_content_length != NULL)
	{
		uint32_t t_length;
		t_length = atoi(t_content_length);
		
		uint32_t t_read = 0;
		
		char *t_data;
		t_data = new char[t_length];
		t_success = t_stdin->Read(t_data, t_length, t_read) && t_length == t_read;

		// Store the raw POST data
		if (t_success)
			s_cgi_post_raw -> copysvalue(MCString(t_data, t_length));
		
		delete t_data;
	}
	
	delete t_stdin;

	return t_success ? ES_NORMAL : ES_ERROR;
}

static bool cgi_store_form_multipart(MCExecPoint& ep, IO_handle p_stream);

// IM-2011-08-05: Reorganization of post variables, now generating all at the same time
static Exec_stat cgi_compute_post_variables()
{
	Exec_stat t_stat = ES_NORMAL;
	
	if (s_cgi_processed_post)
		return ES_NORMAL;
	
	s_cgi_processed_post = true;
	
	char *t_content_type;
	t_content_type = MCS_getenv("CONTENT_TYPE");
	
    // SN-2015-06-01: [[ Bug 9820 ]] We should discard the erroneous parameters
    //  that may have been added after the content type (such as "charset", see
    //  the green box about MIME parameters for x-www-form-urlencoded at
    //  http://www.w3.org/TR/html5/forms.html#application/x-www-form-urlencoded-encoding-algorithm)
    if (t_content_type != NULL
            && (MCCStringEqualCaseless(t_content_type, "application/x-www-form-urlencoded")
                || MCCStringBeginsWithCaseless(t_content_type, "application/x-www-form-urlencoded;")))
	{
		// TODO: currently we assume that urlencoded form data is small enough to fit into memory,
		// so we fetch the contents from $_POST_RAW (which automatically reads the data from stdin).
		// in the future we should read from stdin to avoid duplicating large amounts of data
		MCExecPoint raw_ep, ep;
		MCVarref *t_raw_ref;
		t_raw_ref = s_cgi_post_raw->newvarref();

		t_stat = t_raw_ref->eval(raw_ep);
		if (t_stat == ES_NORMAL)
		{
			MCString t_raw_string;
			t_raw_string = raw_ep.getsvalue();
			cgi_store_form_urlencoded(ep, s_cgi_post_binary, t_raw_string.getstring(), t_raw_string.getstring() + t_raw_string.getlength(), false);
			cgi_store_form_urlencoded(ep, s_cgi_post, t_raw_string.getstring(), t_raw_string.getstring() + t_raw_string.getlength(), true);
		}
		delete t_raw_ref;
	}
	else if (t_content_type != NULL && MCCStringBeginsWithCaseless(t_content_type, "multipart/form-data;"))
	{
		// read post-data from stdin, via the stream cache
		MCExecPoint ep;

		MCCacheHandle *t_stdin = new MCCacheHandle(s_cgi_stdin_cache);
		IO_handle t_stdin_handle = new IO_header(t_stdin, 0);
		
		cgi_store_form_multipart(ep, t_stdin_handle);
		MCS_close(t_stdin_handle);
	}
	return t_stat;
}

static Exec_stat cgi_compute_post_binary_var(void *p_context, MCVariable *p_var)
{
	return cgi_compute_post_variables();
}

static Exec_stat cgi_compute_post_var(void *p_context, MCVariable *p_var)
{
	return cgi_compute_post_variables();
}

static Exec_stat cgi_compute_files_var(void *p_context, MCVariable *p_var)
{
	return cgi_compute_post_variables();
}

////////////////////////////////////////////////////////////////////////////////

// processing of multipart/form-data type post data

// from srvmultipart.cpp:
bool MCMultiPartParseHeaderParams(const char *p_params, char **&r_names, char **&r_values, uint32_t &r_param_count);

static bool cgi_multipart_get_boundary(char *&r_boundary)
{
	bool t_success = true;
	char *t_content_type;
	t_content_type = MCS_getenv("CONTENT_TYPE");

	char *t_params = NULL;
	uint32_t t_index = 0;
	
	char **t_names = NULL;
	char **t_values = NULL;
	uint32_t t_param_count = 0;
	
	t_success = MCCStringFirstIndexOf(t_content_type, ';', t_index);

	if (t_success)
		t_success = MCMultiPartParseHeaderParams(t_content_type + t_index + 1, t_names, t_values, t_param_count);

	r_boundary = NULL;
	
	if (t_success)
	{
		for (uint32_t i = 0; i < t_param_count; i++)
		{
			if (MCCStringEqualCaseless(t_names[i], "boundary") && MCCStringLength(t_values[i]) > 0)
			{
				r_boundary = t_values[i];
				t_values[i] = NULL;
				break;
			}
		}
	}
	
	if (t_success)
		t_success = r_boundary != NULL;
	
	for (uint32_t i = 0; i < t_param_count; i++)
	{
		MCCStringFree(t_names[i]);
		MCCStringFree(t_values[i]);
	}
	MCMemoryDeleteArray(t_names);
	MCMemoryDeleteArray(t_values);
	
	return t_success;
}

typedef enum
{
	kMCDispositionUnknown,
	kMCDispositionFormData,
	kMCDispositionFile,
} cgi_multipart_disposition_t;

typedef struct
{
	cgi_multipart_disposition_t disposition;
	char *name;
	char *type;

	char *file_name;
	const char *temp_name;
	IO_handle file_handle;
	uint32_t file_size;
	MCMultiPartFileStatus file_status;
	MCVariableValue *file_variable;

	char *boundary;
	
	MCVariableValue *post_variable;
	MCVariableValue *post_binary_variable;
} cgi_multipart_context_t;

static void cgi_dispose_multipart_context(cgi_multipart_context_t *p_context)
{
	MCCStringFree(p_context->name);
	MCCStringFree(p_context->file_name);
	MCCStringFree(p_context->type);
	MCCStringFree(p_context->boundary);
	if (p_context->file_handle != NULL)
		MCS_close(p_context->file_handle);
	
	MCMemoryClear(p_context, sizeof(cgi_multipart_context_t));
}

static bool cgi_context_is_form_data(cgi_multipart_context_t *p_context)
{
	return p_context->disposition == kMCDispositionFormData && p_context->file_name == NULL;
}

static bool cgi_context_is_file(cgi_multipart_context_t *p_context)
{
	return (p_context->disposition == kMCDispositionFile || p_context->disposition == kMCDispositionFormData) && p_context->file_name != NULL;
}

static void inline grab_string(char *&x_dest, char *&x_src)
{
	if (x_dest != NULL)
		MCCStringFree(x_dest);
	x_dest = x_src;
	x_src = NULL;
}

static bool cgi_multipart_header_callback(void *p_context, MCMultiPartHeader *p_header)
{
	bool t_success = true;
	cgi_multipart_context_t *t_context = (cgi_multipart_context_t*)p_context;
	
	if (p_header != NULL)
	{
		if (MCCStringEqualCaseless(p_header->name, "Content-Disposition"))
		{
			if (MCCStringEqualCaseless(p_header->value, "form-data"))
				t_context->disposition = kMCDispositionFormData;
			else if (MCCStringEqualCaseless(p_header->value, "file"))
				t_context->disposition = kMCDispositionFile;
			else
				t_context->disposition = kMCDispositionUnknown;
			
			for (uint32_t i = 0; i < p_header->param_count; i++)
			{
				if (MCCStringEqualCaseless(p_header->param_name[i], "name"))
					grab_string(t_context->name, p_header->param_value[i]);
				else if (MCCStringEqualCaseless(p_header->param_name[i], "filename"))
					grab_string(t_context->file_name, p_header->param_value[i]);
			}
		}
		else if (MCCStringEqualCaseless(p_header->name, "Content-Type"))
		{
			grab_string(t_context->type, p_header->value);
			
			for (uint32_t i = 0; i < p_header->param_count; i++)
			{
				if (MCCStringEqualCaseless(p_header->param_name[i], "boundary"))
					grab_string(t_context->boundary, p_header->param_value[i]);
			}
		}
	}
	else
	{
		if (cgi_context_is_form_data(t_context))
		{
			MCExecPoint ep;
			t_success = t_context->name != NULL;
			if (t_success)
			{
				cgi_fetch_variable_value_for_key(s_cgi_post, t_context->name, MCCStringLength(t_context->name), ep, t_context->post_variable);
				t_context->post_variable->assign_empty();
				cgi_fetch_variable_value_for_key(s_cgi_post_binary, t_context->name, MCCStringLength(t_context->name), ep, t_context->post_binary_variable);
				t_context->post_binary_variable->assign_empty();
			}
		}
		else if (cgi_context_is_file(t_context))
		{
			const char *t_temp_dir = cgi_get_upload_temp_dir();
			const char *t_error = NULL;
			if (t_temp_dir == NULL || !MCS_exists(t_temp_dir, False))
			{
				t_context->file_status = kMCFileStatusNoUploadFolder;
			}
			else if (!MCMultiPartCreateTempFile(cgi_get_upload_temp_dir(), t_context->file_handle, t_context->temp_name))
			{
				t_context->file_status = kMCFileStatusIOError;
			}
		}
	}
	
	return t_success;
}

static bool cgi_multipart_body_callback(void *p_context, const char *p_data, uint32_t p_data_length, bool p_finished, bool p_truncated)
{
	cgi_multipart_context_t *t_context = (cgi_multipart_context_t*)p_context;
	bool t_success = true;

	if (cgi_context_is_form_data(t_context))
	{
		if (t_context->post_binary_variable != NULL)
		{
			t_success = t_context->post_binary_variable->append_string(MCString(p_data, p_data_length));

			if (t_success && p_finished)
			{
				uint32_t t_native_length;
				char *t_native = NULL;
				MCString t_value;
				t_value = t_context->post_binary_variable->get_string();
				if (cgi_native_from_encoding(MCserveroutputtextencoding, t_value.getstring(), t_value.getlength(), t_native, t_native_length))
					t_context->post_variable -> assign_buffer(t_native, t_native_length);
			}
		}
	}
	else if (cgi_context_is_file(t_context))
	{
		if (t_context->file_status == kMCFileStatusOK)
		{
			if (IO_NORMAL == MCS_write(p_data, 1, p_data_length, t_context->file_handle))
				t_context->file_size += p_data_length;
			else
				t_context->file_status = kMCFileStatusIOError;
		}
		
		if (t_success && (p_finished || p_truncated))
		{
			MCExecPoint ep;
			MCVariableValue *t_file_varvalue = NULL;
			cgi_fetch_variable_value_for_key(s_cgi_files, t_context->name, MCCStringLength(t_context->name), ep, t_context->file_variable);
			
			if (t_context->file_status == kMCFileStatusOK && t_context->file_size == 0)
				t_context->file_status = kMCFileStatusFailed;
			
			if (p_truncated)
				t_context->file_status = kMCFileStatusStopped;
			
			ep.setsvalue(MCString(t_context->file_name));
			t_context->file_variable->store_element(ep, MCString("name"));
			ep.setsvalue(MCString(t_context->type));
			t_context->file_variable->store_element(ep, MCString("type"));
			ep.setsvalue(MCString(t_context->temp_name));
			t_context->file_variable->store_element(ep, MCString("filename"));
			ep.setuint(t_context->file_size);
			t_context->file_variable->store_element(ep, MCString("size"));

			if (t_context->file_status != kMCFileStatusOK)
			{
				ep.setsvalue(MCMultiPartGetErrorMessage(t_context->file_status));
				t_context->file_variable->store_element(ep, MCString("error"));
			}
		}
	}
	
	if (t_success && p_finished)
	{
		// clear context for next part
		cgi_dispose_multipart_context(t_context);
	}
	
	return t_success;
}

static bool cgi_store_form_multipart(MCExecPoint& ep, IO_handle p_stream)
{
	bool t_success = true;
	char *t_boundary = NULL;
	
	cgi_multipart_context_t t_context;
	MCMemoryClear(&t_context, sizeof(t_context));
	
	uint32_t t_bytes_read = 0;
	
	if (t_success)
		t_success = cgi_multipart_get_boundary(t_boundary);
	if (t_success)
		t_success = MCMultiPartReadMessageFromStream(p_stream, t_boundary, t_bytes_read,
													 cgi_multipart_header_callback, cgi_multipart_body_callback, &t_context);

	// clean up in case of errors;
	if (!t_success)
		cgi_dispose_multipart_context(&t_context);

	MCCStringFree(t_boundary);

	return t_success;
}

////////////////////////////////////////////////////////////////////////////////

static Exec_stat cgi_compute_cookie_var(void *p_context, MCVariable *p_var)
{
	bool t_success = true;
	char *t_cookie_string;
	t_cookie_string = MCS_getenv("HTTP_COOKIE");
	
	MCExecPoint ep;
	
	if (t_cookie_string == NULL)
		return ES_NORMAL;
	
	cgi_store_cookie_urlencoded(ep, s_cgi_cookie, t_cookie_string, t_cookie_string + MCCStringLength(t_cookie_string), true);

	return ES_NORMAL;
}

////////////////////////////////////////////////////////////////////////////////

#if defined (_WINDOWS_SERVER)
_CRTIMP extern char **_environ;
#define environ _environ
#else
extern char **environ;
#endif

bool cgi_initialize()
{
	// need to ensure PATH_TRANSLATED points to the script and PATH_INFO contains everything that follows
	cgi_fix_path_variables();

	// Resolve the main script that has been requested by the CGI interface.
	MCserverinitialscript = MCsystem -> ResolvePath(MCS_getenv("PATH_TRANSLATED"));
	
	// Set the current folder to be that containing the CGI file.
	char *t_server_script_folder;
	t_server_script_folder = strdup(MCserverinitialscript);
#ifdef _WINDOWS_SERVER
	strrchr(t_server_script_folder, '\\')[0] = '\0';
#else
	strrchr(t_server_script_folder, '/')[0] = '\0';
#endif
	MCsystem -> SetCurrentFolder(t_server_script_folder);
	delete t_server_script_folder;
	
	// Initialize the headers.
	MCservercgiheaders = NULL;
	MCservercgiheaders_sent = false;
	
	// Get the document root
	MCservercgidocumentroot = getenv("DOCUMENT_ROOT");
	
	// Initialize the input wrapper.  This creates a cache of the input stream
	// which is filled as the stream is read from.  this allows stdin to be used
	// to populate the post data arrays, and also to be read from by the script
	// without conflicting
	s_cgi_stdin_cache = new MCStreamCache(IO_stdin->handle);
	IO_stdin = new IO_header(new MCCacheHandle(s_cgi_stdin_cache), 0);
		
	// Initialize the output wrapper, this simply ensures we output headers
	// before any content.
	IO_stdout = new IO_header(new cgi_stdout, 0);
	
	// Need an exec-point for variable creation.
	MCExecPoint ep;
	
	// Construct the _SERVER variable
	/* UNCHECKED */ MCVariable::createwithname_cstring("$_SERVER", s_cgi_server);
	s_cgi_server -> setnext(MCglobals);
	MCglobals = s_cgi_server;
	for(uint32_t i = 0; environ[i] != NULL; i++)
	{
		
		static const char *s_cgi_vars[] =
		{
			"GATEWAY_INTERFACE=",
			"SERVER_ADDR=",
			"SERVER_NAME=",
			"SERVER_SOFTWARE=",
			"SERVER_PROTOCOL=",
			"REQUEST_METHOD=",
			"REQUEST_TIME=",
			"QUERY_STRING=",
			"DOCUMENT_ROOT=",
			"HTTPS=",
			
			// MW-2009-08-12: For some reason I missed this first time around :o)
			"REMOTE_USER=",
			"REMOTE_ADDR=",
			"REMOTE_HOST=",
			"REMOTE_PORT=",
			
			// MM-2011-09-08: Added as part of customer support request for REMOTE_USER.
			//   The standard Apache LiveCode server config redirects the script to the server engine.
			//   This appears to mean that REMOTE_USER is never set 
			//   (I guess it will only ever be set if the server engine itself is behind authentication)
			//   but REDIRECT_REMOTE_USER is.
			"REDIRECT_REMOTE_USER=",
			
			"SERVER_ADMIN=",
			"SERVER_PORT=",
			"SERVER_SIGNATURE=",
			
			"PATH_TRANSLATED=",
			
			"REQUEST_URI=",

			"PATH_INFO=",
			"SCRIPT_NAME=",
			"SCRIPT_FILENAME=",

			"CONTENT_TYPE=",
			"CONTENT_LENGTH=",

			NULL
		};
		
		bool t_found;
		t_found = false;
		
		if (strncasecmp(environ[i], "HTTP_", 5) == 0)
			t_found = true;
		else
			for(uint32_t j = 0; s_cgi_vars[j] != NULL && !t_found; j++)
				if (strncasecmp(environ[i], s_cgi_vars[j], strlen(s_cgi_vars[j])) == 0)
					t_found = true;

		if (t_found)
		{
			const char *t_value;
			t_value = strchr(environ[i], '=');
			if (t_value == NULL)
			{
				ep . clear();
				t_value = environ[i] + strlen(environ[i]);
			}
			else
				ep . setsvalue(t_value + 1);
			
			s_cgi_server -> getvalue() . store_element(ep, MCString(environ[i], t_value - environ[i]));
		}
	}
	
	// Construct the GET variables by parsing the QUERY_STRING
	
	/* UNCHECKED */ MCDeferredVariable::createwithname_cstring("$_GET_RAW", cgi_compute_get_raw_var, nil, s_cgi_get_raw);
	s_cgi_get_raw -> setnext(MCglobals);
	MCglobals = s_cgi_get_raw;	
	/* UNCHECKED */ MCDeferredVariable::createwithname_cstring("$_GET", cgi_compute_get_var, nil, s_cgi_get);
	s_cgi_get -> setnext(MCglobals);
	MCglobals = s_cgi_get;
	/* UNCHECKED */ MCDeferredVariable::createwithname_cstring("$_GET_BINARY", cgi_compute_get_binary_var, nil, s_cgi_get_binary);
	s_cgi_get_binary -> setnext(MCglobals);
	MCglobals = s_cgi_get_binary;	
	
	// Construct the _POST variables by reading stdin.
	
	/* UNCHECKED */ MCDeferredVariable::createwithname_cstring("$_POST_RAW", cgi_compute_post_raw_var, nil, s_cgi_post_raw);
	s_cgi_post_raw -> setnext(MCglobals);
	MCglobals = s_cgi_post_raw;
	/* UNCHECKED */ MCDeferredVariable::createwithname_cstring("$_POST", cgi_compute_post_var, nil, s_cgi_post);
	s_cgi_post -> setnext(MCglobals);
	MCglobals = s_cgi_post;
	/* UNCHECKED */ MCDeferredVariable::createwithname_cstring("$_POST_BINARY", cgi_compute_post_binary_var, nil, s_cgi_post_binary);
	s_cgi_post_binary -> setnext(MCglobals);
	MCglobals = s_cgi_post_binary;	
	
	// Construct the FILES variable by reading stdin

	/* UNCHECKED */ MCDeferredVariable::createwithname_cstring("$_FILES", cgi_compute_files_var, nil, s_cgi_files);
	s_cgi_files -> setnext(MCglobals);
	MCglobals = s_cgi_files;
	
	// Construct the COOKIES variable by parsing HTTP_COOKIE
	/* UNCHECKED */ MCDeferredVariable::createwithname_cstring("$_COOKIE", cgi_compute_cookie_var, nil, s_cgi_cookie);
	s_cgi_cookie -> setnext(MCglobals);
	MCglobals = s_cgi_cookie;
	
	// Create the $_SESSION variable explicitly, to be populated upon calls to "start session"
	// required as implicit references to "$_SESSION" will result in its creation as an env var
	MCVariable *t_session_var = NULL;
	/* UNCHECKED */ MCVariable::createwithname_cstring("$_SESSION", t_session_var);
	t_session_var->setnext(MCglobals);
	MCglobals = t_session_var;

	return true;
}

void cgi_finalize_session();

void cgi_finalize()
{
	// clean up any temporary uploaded files
	MCMultiPartRemoveTempFiles();
	
	// clean up session data
	cgi_finalize_session();
}

////////////////////////////////////////////////////////////////////////////////

static bool cgi_send_cookies(void)
{
	bool t_success = true;
	
	char *t_cookie_header = NULL;
	MCExecPoint ep;
	
	for (uint32_t i = 0; t_success && i < MCservercgicookiecount; i++)
	{
		t_success = MCCStringFormat(t_cookie_header, "Set-Cookie: %s=%s", MCservercgicookies[i].name, MCservercgicookies[i].value);
		
		if (t_success && MCservercgicookies[i].expires != 0)
		{
			ep.setuint(MCservercgicookies[i].expires);
			t_success = MCD_convert(ep, CF_SECONDS, CF_UNDEFINED, CF_INTERNET_DATE, CF_UNDEFINED);
			if (t_success)
			{
				MCString t_date;
				t_date = ep.getsvalue();
				t_success = MCCStringAppendFormat(t_cookie_header, "; Expires=%.*s", t_date.getlength(), t_date.getstring());
			}
		}
		
		if (t_success && MCservercgicookies[i].path != NULL)
			t_success = MCCStringAppendFormat(t_cookie_header, "; Path=%s", MCservercgicookies[i].path);
		
		if (t_success && MCservercgicookies[i].domain != NULL)
			t_success = MCCStringAppendFormat(t_cookie_header, "; Domain=%s", MCservercgicookies[i].domain);

		if (t_success && MCservercgicookies[i].secure)
			t_success = MCCStringAppend(t_cookie_header, "; Secure");
		
		if (t_success && MCservercgicookies[i].http_only)
			t_success = MCCStringAppend(t_cookie_header, "; HttpOnly");
		
		if (t_success)
			t_success = MCCStringAppend(t_cookie_header, "\n");
		
		if (t_success)
			t_success = IO_NORMAL == MCS_write(t_cookie_header, 1, MCCStringLength(t_cookie_header), IO_stdout);
		MCCStringFree(t_cookie_header);
		t_cookie_header = NULL;
	}
	return t_success;
}

static bool cgi_send_headers(void)
{
	bool t_sent_content;
	t_sent_content = false;
	
	for(uint32_t i = 0; i < MCservercgiheadercount; i++)
	{
		if (strncasecmp("Content-Type:", MCservercgiheaders[i], 13) == 0)
			t_sent_content = true;
		if (MCS_write(MCservercgiheaders[i], 1, strlen(MCservercgiheaders[i]), IO_stdout) != IO_NORMAL)
			return false;
		if (MCS_write("\n", 1, 1, IO_stdout) != IO_NORMAL)
			return false;
	}
	
	if (!t_sent_content)
	{
		char t_content_header[128];
		switch(MCserveroutputtextencoding)
		{
			case kMCSOutputTextEncodingWindows1252:
				sprintf(t_content_header, "Content-Type: text/html; charset=windows-1252\n");
				break;
			case kMCSOutputTextEncodingMacRoman:
				sprintf(t_content_header, "Content-Type: text/html; charset=macintosh\n");
				break;
			case kMCSOutputTextEncodingISO8859_1:
				sprintf(t_content_header, "Content-Type: text/html; charset=iso-8859-1\n");
				break;
			case kMCSOutputTextEncodingUTF8:
				sprintf(t_content_header, "Content-Type: text/html; charset=utf-8\n");
				break;
		}
		
		if (MCS_write(t_content_header, 1, strlen(t_content_header), IO_stdout) != IO_NORMAL)
			return false;
	}
	
	if (MCS_write("\n", 1, 1, IO_stdout) != IO_NORMAL)
		return false;
	
	return true;
}

////////////////////////////////////////////////////////////////////////////////

bool MCServerGetSessionIdFromCookie(char *&r_id);

MCSession *s_current_session = NULL;

bool MCServerStartSession()
{
	// start session:
	// if MCsessionid is empty - get id from cookie
	// if MCsessionid not empty - use MCsessionid - clear MCsessionid once opened
	bool t_success = true;
	
	if (s_current_session != NULL)
		return true;

	const char *t_session_id = NULL;
	char *t_cookie_id = NULL;
	
	t_session_id = MCsessionid;
	
	if (t_session_id == NULL)
	{
		t_success = MCServerGetSessionIdFromCookie(t_cookie_id);
		t_session_id = t_cookie_id;
	}
	
	if (t_success)
		t_success = MCSessionStart(t_session_id, s_current_session);
	
	MCVariable *t_session_var = NULL;
	
	if (t_success)
	{
		t_session_var = MCVariable::lookupglobal_cstring("$_SESSION");
		if (t_session_var == NULL)
		{
			t_success = MCVariable::createwithname_cstring("$_SESSION", t_session_var);
			if (t_success)
			{
				t_session_var->setnext(MCglobals);
				MCglobals = t_session_var;
			}
		}
	}
	if (t_success)
		t_session_var->getvalue().assign_empty();
	
	if (t_success)
	{
		if (s_current_session->data_length > 0)
			t_success = t_session_var->getvalue().decode(MCString(s_current_session->data, s_current_session->data_length));
	}
	
	if (t_success)
	{
		// clear MCsessionid - now associated with open session
		MCCStringFree(MCsessionid);
		MCsessionid = NULL;
	}
	else
	{
		MCSessionDiscard(s_current_session);
		s_current_session = NULL;
	}
	
	MCCStringFree(t_cookie_id);
	
	return t_success;
}

bool MCServerStopSession()
{
	bool t_success = true;
	
	if (s_current_session == NULL)
		return true;
	
	char *t_data = NULL;
	uint32_t t_data_length = 0;
	
	MCVariable *t_session_var;
	t_session_var = MCVariable::lookupglobal_cstring("$_SESSION");

	if (t_session_var != NULL)
		t_success = t_session_var->getvalue().encode((void*&)t_data, t_data_length);

	if (t_success)
	{
		MCMemoryDeallocate(s_current_session->data);
		s_current_session->data = t_data;
		s_current_session->data_length = t_data_length;
		
		t_success = MCSessionCommit(s_current_session);
		s_current_session = NULL;
	}

	return t_success;
}

bool MCServerDeleteSession()
{
	bool t_success = true;
	
	t_success = MCSessionExpire(MCS_get_session_id());
	
	if (s_current_session != NULL)
	{
		MCSessionDiscard(s_current_session);
		s_current_session = NULL;
	}
	
	return t_success;
}

void cgi_finalize_session()
{
	// close current session if open
	if (s_current_session != NULL)
	{
		MCSessionCommit(s_current_session);
		s_current_session = NULL;
	}

	// clean up session files
	MCSessionCleanup();
}

////////////////////////////////////////////////////////////////////////////////

// Session properties

static char *s_session_temp_dir = NULL;
bool MCS_get_temporary_folder(char *&r_folder);

bool MCS_set_session_save_path(const char *p_path)
{
	char *t_save_path = nil;
	
	if (!MCCStringClone(p_path, t_save_path))
		return false;
	
	MCCStringFree(MCsessionsavepath);
	MCsessionsavepath = t_save_path;
	
	return true;
}

const char *MCS_get_session_save_path(void)
{
	if (MCsessionsavepath != NULL && MCCStringLength(MCsessionsavepath) > 0)
		return MCsessionsavepath;
	
	if (s_session_temp_dir != NULL)
		return s_session_temp_dir;
	
	if (MCS_get_temporary_folder(s_session_temp_dir))
		return s_session_temp_dir;
	
	return NULL;
}

bool MCS_set_session_lifetime(uint32_t p_lifetime)
{
	MCsessionlifetime = p_lifetime;
	return true;
}

uint32_t MCS_get_session_lifetime(void)
{
	return MCsessionlifetime;
}

bool MCS_set_session_name(const char *p_name)
{
	char *t_name = nil;
	
	if (!MCCStringClone(p_name, t_name))
		return false;
	
	MCCStringFree(MCsessionname);
	MCsessionname = t_name;
	
	return true;
}

const char *MCS_get_session_name(void)
{
	if (MCsessionname != NULL && MCCStringLength(MCsessionname) > 0)
		return MCsessionname;
	
	return "LCSESSION";
}

bool MCS_set_session_id(const char *p_id)
{
	char *t_id = nil;
	
	if (s_current_session != NULL)
		return false;
	
	if (!MCCStringClone(p_id, t_id))
		return false;
	
	MCCStringFree(MCsessionid);
	MCsessionid = t_id;
	
	return true;
}

const char *MCS_get_session_id(void)
{
	if (s_current_session != NULL)
		return s_current_session->id;
	
	return MCsessionid;
}

bool MCServerGetSessionIdFromCookie(char *&r_id)
{
	MCVariable *t_cookie_array;
	t_cookie_array = MCVariable::lookupglobal_cstring("$_COOKIE");
	
	if (t_cookie_array == NULL)
	{
		r_id = NULL;
		return true;
	}
	
	// ensure cookie array is evaluated
	if (t_cookie_array->isdeferred() && ES_NORMAL != ((MCDeferredVariable*)t_cookie_array)->compute())
		return false;
	
	MCExecPoint ep;
	if (ES_NORMAL != t_cookie_array->fetch_element(ep, MCS_get_session_name()))
		return false;
	
	// retrieve ID from cookie value
	if (ep.isempty())
		r_id = NULL;
	else
		r_id = ep.getsvalue().clone();
	
	return true;
}

////////////////////////////////////////////////////////////////////////////////

static bool MCConvertNativeFromWindows1252(const uint8_t *p_input, uint32_t p_input_length, uint8_t*& r_output, uint32_t& r_output_length)
{
	uint8_t *t_output;
	if (!MCMemoryAllocate(p_input_length, t_output))
		return false;

	for(uint32_t i = 0; i < p_input_length; i++)
	{
		uint2 t_input_char;
		t_input_char = MCUnicodeMapFromNative_Windows1252(p_input[i]);
		if (!MCUnicodeMapToNative(&t_input_char, 1, t_output[i]))
			t_output[i] = '?';
	}

	r_output = t_output;
	r_output_length = p_input_length;

	return true;
}

static bool MCConvertNativeFromMacRoman(const uint8_t *p_input, uint32_t p_input_length, uint8_t*& r_output, uint32_t& r_output_length)
{
	uint8_t *t_output;
	if (!MCMemoryAllocate(p_input_length, t_output))
		return false;

	for(uint32_t i = 0; i < p_input_length; i++)
	{
		uint2 t_input_char;
		t_input_char = MCUnicodeMapFromNative_MacRoman(p_input[i]);
		if (!MCUnicodeMapToNative(&t_input_char, 1, t_output[i]))
			t_output[i] = '?';
	}

	r_output = t_output;
	r_output_length = p_input_length;

	return true;
}

static bool MCConvertNativeFromISO8859_1(const uint8_t *p_input, uint32_t p_input_length, uint8_t*& r_output, uint32_t& r_output_length)
{
	uint8_t *t_output;
	if (!MCMemoryAllocate(p_input_length, t_output))
		return false;

	for(uint32_t i = 0; i < p_input_length; i++)
	{
		uint2 t_input_char;
		t_input_char = MCUnicodeMapFromNative_MacRoman(p_input[i]);
		if (!MCUnicodeMapToNative(&t_input_char, 1, t_output[i]))
			t_output[i] = '?';
	}

	r_output = t_output;
	r_output_length = p_input_length;

	return true;
}

static bool MCConvertNativeFromUTF16(const uint16_t *p_chars, uint32_t p_char_count, uint8_t*& r_output, uint32_t& r_output_length)
{
	uint8_t *t_output;
	uint32_t t_output_length;
	if (!MCMemoryAllocate(p_char_count, t_output))
		return false;

	uint32_t t_index;
	t_index = 0;
	t_output_length = 0;
	while(t_index < p_char_count)
	{
		if (p_chars[t_index] < 128 && (t_index == p_char_count - 1 || p_chars[t_index + 1] < 128))
		{
			t_output[t_output_length++] = (char)p_chars[t_index];
			t_index += 1;
		}
		else
		{
			uint32_t t_start;
			t_start = t_index;
			
			uint32_t t_codepoint;
			t_codepoint = MCUnicodeCodepointAdvance((const uint2 *)p_chars, p_char_count, t_index);
			
			while(t_index < p_char_count)
			{
				uint4 t_old_index;
				t_old_index = t_index;
				
				t_codepoint = MCUnicodeCodepointAdvance((const uint2 *)p_chars, p_char_count, t_index);
				
				if (MCUnicodeCodepointIsBase(t_codepoint))
				{
					t_index = t_old_index;
					break;
				}
			}

			uint8_t t_char;
			if (!MCUnicodeMapToNative(p_chars + t_start, t_index - t_start, t_char))
				t_char = '?';
			
			t_output[t_output_length++] = t_char;
		}
	}

	MCMemoryReallocate(t_output, t_output_length, t_output);

	r_output = t_output;
	r_output_length = t_output_length;

	return true;
}
