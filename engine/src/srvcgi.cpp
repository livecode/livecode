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

//#include "execpt.h"
#include "exec.h"
#include "scriptpt.h"
#include "globals.h"
#include "param.h"
#include "external.h"
#include "handler.h"
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
static MCVariable *s_cgi_post;       // StringRef
static MCVariable *s_cgi_post_raw;   // DataRef
static MCVariable *s_cgi_post_binary;// DataRef
static MCVariable *s_cgi_files;      // ArrayRef
static MCVariable *s_cgi_get;        // nativised StringRef
static MCVariable *s_cgi_get_raw;    // StringRef
static MCVariable *s_cgi_get_binary; // DataRef
static MCVariable *s_cgi_cookie;     // StringRef

static bool s_cgi_processed_post = false;

// rather than making stdin / $_POST_RAW / $_POST, $_POST_BINARY, $_FILES
// exclusive, we store the stream contents in this cache object and create a
// cache reader handle around it when reading from stdin
class MCStreamCache;
static MCStreamCache *s_cgi_stdin_cache;

////////////////////////////////////////////////////////////////////////////////

static bool cgi_send_cookies(void);
static bool cgi_send_headers(void);

#if !defined(_LINUX_SERVER) && !defined(_MAC_SERVER)
static char *strndup(const char *s, size_t n)
{
	char *r;
	r = (char *)malloc(n + 1);
	strncpy(r, s, n);
	r[n] = '\0';
	return r;
}
#endif

////////////////////////////////////////////////////////////////////////////////

static MCStringRef s_cgi_upload_temp_dir;
static MCStringRef s_cgi_temp_dir;

bool MCS_get_temporary_folder(MCStringRef &r_temp_folder);

static MCStringRef cgi_get_upload_temp_dir()
{
	if (!MCStringIsEmpty(s_cgi_upload_temp_dir))
		return s_cgi_upload_temp_dir;
	
	if (!MCStringIsEmpty(s_cgi_temp_dir))
		return s_cgi_temp_dir;

	MCAutoStringRef t_temp_folder;
	if (MCS_get_temporary_folder(&t_temp_folder))
		MCValueAssign(s_cgi_temp_dir, *t_temp_folder);
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
		return m_delegate -> Read(p_buffer, p_length, r_read);
	}
	
	bool Write(const void *p_buffer, uint32_t p_length)
	{
		return m_delegate -> Write(p_buffer, p_length);
	}
	
	bool Seek(int64_t p_offset, int p_dir)
	{
		return m_delegate -> Seek(p_offset, p_dir);
	}
	
	bool Truncate(void)
	{
		return m_delegate -> Truncate();
	}
	
	bool Sync(void)
	{
		return m_delegate -> Sync();
	}
	
	bool Flush(void)
	{
		return m_delegate -> Flush();
	}
	
	bool PutBack(char p_char)
	{
		return m_delegate -> PutBack(p_char);
	}
	
	int64_t Tell(void)
	{
		return m_delegate -> Tell();
	}
	
	void *GetFilePointer(void)
	{
		return m_delegate -> GetFilePointer();
	}
	
	int64_t GetFileSize(void)
	{
		return m_delegate -> GetFileSize();
	}

    bool TakeBuffer(void*& r_buffer, size_t& r_length)
    {
        return m_delegate -> TakeBuffer(r_buffer, r_length);
    }

    bool IsExhausted(void)
    {
        return m_delegate -> IsExhausted();
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
	
	bool Write(const void *p_buffer, uint32_t p_length)
	{
		Close();

		if (!(cgi_send_cookies() && cgi_send_headers()))
			return false;
		
		return IO_stdout -> Write(p_buffer, p_length);
	}
};

////////////////////////////////////////////////////////////////////////////////

// caching object, stores stream data in memory unless larger than 64k, in which
// case, the cached stream is stored in a temporary file
class MCStreamCache
{
public:
    MCStreamCache(IO_handle p_source_stream);
	~MCStreamCache();
	
    bool Read(void *p_buffer, uint32_t p_offset, uint32_t p_length, uint32_t &r_read);
    bool Ensure(uint32_t p_offset);
	
private:
	bool ReadFromCache(void *p_buffer, uint32_t p_offset, uint32_t p_length, uint32_t &r_read);
	bool ReadFromStream(void *p_buffer, uint32_t p_length, uint32_t &r_read);
	bool AppendToCache(void *p_buffer, uint32_t p_length, uint32_t &r_written);
	
	static const uint32_t m_buffer_limit = 64 * 1024;
	static const uint32_t m_min_read = 1024;
	
    IO_handle m_source_stream;
	uint32_t m_cache_length;
	void *m_cache_buffer;
	IO_handle m_cache_file;
	MCStringRef m_cache_filename;
};

MCStreamCache::MCStreamCache(IO_handle p_source_stream)
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
	if (m_cache_buffer != NULL)
	{
		r_read = MCMin(p_length, m_cache_length - p_offset);
		MCMemoryCopy(p_buffer, (uint8_t*)m_cache_buffer + p_offset, r_read);
		return true;
	}
	else if (m_cache_file != NULL)
	{
		bool t_success = true;
		
		
		t_success = (IO_NORMAL == MCS_seek_set(m_cache_file, p_offset));
		if (t_success)
			t_success = (IO_ERROR != MCS_readall(p_buffer, p_length, m_cache_file, p_length));
		
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
	
	bool Write(const void *p_buffer, uint32_t p_length)
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
		return NULL;
	}
	
	int64_t GetFileSize()
	{
		return 0;
    }

    // Returns true if an attempt has been made to read past the end of the
    // stream.
    virtual bool IsExhausted(void)
    {
        return false;
    }

    virtual bool TakeBuffer(void*& r_buffer, size_t& r_length)
    {
        return false;
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

static uindex_t strchr_limit(MCDataRef p_data, char_t p_char, uindex_t p_index)
{
    uindex_t t_found_index = p_index;
    while (t_found_index < MCDataGetLength(p_data) && MCDataGetByteAtIndex(p_data, t_found_index) != p_char)
        ++t_found_index;

    return t_found_index;
}

static void cgi_unescape_url(MCDataRef p_url, MCRange p_url_range, MCDataRef &r_unescaped_url)
{
    uindex_t t_index;
    uindex_t t_last_index;
    uindex_t t_unescaped_length;
    t_last_index = p_url_range . offset + p_url_range . length;
    t_index = p_url_range . offset;
    t_unescaped_length = 0;

    MCAutoDataRef t_unescaped_url;
    const byte_t *t_url_string;

    t_url_string = MCDataGetBytePtr(p_url);
    
    // SN-2014-09-04: [[ Bug 13340 ]] Creating a mutable data with an initial capacity increases the byte_count
    //  and any appending goes after those unitialised, original bytes.
    /* UNCHECKED */ MCDataCreateMutable(0, &t_unescaped_url);

    while (t_index < t_last_index)
    {
        if (t_url_string[t_index] == '+')
        {
            MCDataAppendByte(*t_unescaped_url, ' ');
            t_unescaped_length++;
            t_index++;
        }
        else if (t_url_string[t_index] == '%')
        {
            if (t_last_index - t_index < 3)
                break;

            if (MCUnicodeIsHexDigit(t_url_string[t_index + 1]) && MCUnicodeIsHexDigit(t_url_string[t_index + 2]))
            {
                MCDataAppendByte(*t_unescaped_url, (convxdigit(t_url_string[t_index + 1]) << 4) | (convxdigit(t_url_string[t_index + 2])));
                t_unescaped_length++;
            }

            t_index += 3;

            if (MCDataGetLength(*t_unescaped_url) >= 2
                    && MCDataGetByteAtIndex(*t_unescaped_url, t_unescaped_length - 1) == 10
                    && MCDataGetByteAtIndex(*t_unescaped_url, t_unescaped_length - 2) == 13)
                MCDataRemove(*t_unescaped_url, MCRangeMake(t_unescaped_length - 2, 1));

        }
        else
            MCDataAppendByte(*t_unescaped_url, t_url_string[t_index++]);
    }

    /* UNCHECKED */ MCDataCopy(*t_unescaped_url, r_unescaped_url);
}

static void cgi_fetch_valueref_for_key(MCVariable *p_variable, MCNameRef p_key, MCValueRef &r_var_value)
{
    r_var_value = p_variable->getvalueref(&p_key, 1, false);
}

static bool cgi_store_control_value(MCVariable *p_variable, MCNameRef p_key, MCValueRef p_value)
{
    return p_variable->setvalueref(&p_key, 1, false, p_value);
}

static bool MCConvertNativeFromUTF16(const uint16_t *p_chars, uint32_t p_char_count, uint8_t*& r_output, uint32_t& r_output_length);
static bool MCConvertNativeFromWindows1252(const uint8_t *p_chars, uint32_t p_char_count, uint8_t*& r_output, uint32_t& r_output_length);
static bool MCConvertNativeFromMacRoman(const uint8_t *p_chars, uint32_t p_char_count, uint8_t*& r_output, uint32_t& r_output_length);
static bool MCConvertNativeFromISO8859_1(const uint8_t *p_chars, uint32_t p_char_count, uint8_t*& r_output, uint32_t& r_output_length);

static bool cgi_native_from_encoding(MCSOutputTextEncoding p_encoding, MCDataRef p_text, MCStringRef &r_native_text)
{
    MCStringEncoding t_encoding;

    if (p_encoding == kMCSOutputTextEncodingUTF8)
        t_encoding = kMCStringEncodingUTF8;
    else if (p_encoding == kMCSOutputTextEncodingWindows1252)
        t_encoding = kMCStringEncodingWindows1252;
    else if (p_encoding == kMCSOutputTextEncodingMacRoman)
        t_encoding = kMCStringEncodingMacRoman;
    else if (p_encoding == kMCSOutputTextEncodingISO8859_1)
        t_encoding = kMCStringEncodingISO8859_1;
    else if (p_encoding == kMCSOutputTextEncodingNative)
        t_encoding = kMCStringEncodingNative;

    if (MCStringDecode(p_text, t_encoding, false, r_native_text))
    {
        MCStringNativize(r_native_text);
        return true;
    }
    else
        return false;
}

static void cgi_store_data_urlencoded(MCVariable *p_variable, MCDataRef p_data, bool p_native_encoding, char p_delimiter, bool p_remove_whitespace)
{
    uindex_t t_length;
    uindex_t t_encoded_index;
    t_encoded_index = 0;
    t_length = MCDataGetLength(p_data);

    while(t_encoded_index < t_length)
    {
        uindex_t t_delimiter_index;

        if (p_remove_whitespace)
            while (MCDataGetByteAtIndex(p_data, t_encoded_index) == ' ')
                t_encoded_index++;

        t_delimiter_index = strchr_limit(p_data, p_delimiter, t_encoded_index);
		
        uindex_t t_encoded_key_end;
        t_encoded_key_end = strchr_limit(p_data, '=', t_encoded_index);

        uindex_t t_encoded_value_index;
        if (t_encoded_key_end != t_delimiter_index)
            t_encoded_value_index = t_encoded_key_end + 1;
		else
            t_encoded_value_index = t_encoded_key_end;

        MCAutoDataRef t_key;
        MCNewAutoNameRef t_key_as_name;
        MCAutoStringRef t_key_as_string;
        cgi_unescape_url(p_data, MCRangeMake(t_encoded_index, t_encoded_key_end - t_encoded_index), &t_key);

        // The key should be native
        /* UNCHECKED */ MCStringCreateWithNativeChars((char_t*)MCDataGetBytePtr(*t_key), MCDataGetLength(*t_key), &t_key_as_string);
        /* UNCHECKED */ MCNameCreate(*t_key_as_string, &t_key_as_name);

        uindex_t t_encoded_value_end;
        t_encoded_value_end = t_delimiter_index;
		
		if (p_remove_whitespace)
            while (t_encoded_value_end > t_encoded_value_index && MCDataGetByteAtIndex(p_data, t_encoded_value_end) == ' ')
				t_encoded_value_end--;
		
        MCAutoDataRef t_value;
        cgi_unescape_url(p_data, MCRangeMake(t_encoded_value_index, t_encoded_value_end - t_encoded_value_index), &t_value);

		// MM-2011-07-13: Added p_native_encoding flag that specifies if the text should 
		//   be converted from the outputTextEncoding to the native character set.
        // IM-2011-07-13 convert from MCserveroutputtextencoding to native

        if (!p_native_encoding)
        {
            // We need to store the value as it comes: DataRef
            cgi_store_control_value(p_variable, *t_key_as_name, (MCValueRef)*t_value);
        }
		else
        {
            MCAutoStringRef t_native_text;
            if (cgi_native_from_encoding(MCserveroutputtextencoding, *t_value, &t_native_text))
                cgi_store_control_value(p_variable, *t_key_as_name, (MCValueRef)*t_native_text);
        }

        t_encoded_index = t_delimiter_index + 1;
    }
}

static void cgi_store_cookie_urlencoded(MCVariable *p_variable, MCDataRef p_data, bool p_native)
{
    cgi_store_data_urlencoded(p_variable, p_data, p_native, ';', true);
}

static void cgi_store_form_urlencoded(MCVariable *p_variable, MCDataRef p_data, bool p_native)
{
    cgi_store_data_urlencoded(p_variable, p_data, p_native, '&', false);
}


static void cgi_fix_path_variables()
{
	char *t_path, *t_path_end;

	MCStringRef env;
    env = nil;
	t_path = nil;
    t_path_end = nil;
    
    // SN-2014-07-29: [[ Bug 12865 ]] When a LiveCode CGI script has a .cgi extension and has
    //  the appropriate shebang pointing to the LiveCode server, PATH_TRANSLATED is not set by Apache.
    //  The current file (stored in SCRIPT_FILENAME) is the one containing the script.
	if (MCS_getenv(MCSTR("PATH_TRANSLATED"), env))
		t_path = strdup(MCStringGetCString(env));
    else if (MCS_getenv(MCSTR("SCRIPT_FILENAME"), env))
        t_path = strdup(MCStringGetCString(env));

    MCAutoStringRef t_path_string;
    /* UNCHECKED */ MCStringCreateWithCString(t_path, &t_path_string);
             
    if (t_path != nil)
    {
		t_path_end = t_path + strlen(t_path);
        
#ifdef _WINDOWS_SERVER
		for(uint32_t i = 0; t_path[i] != '\0'; i++)
			if (t_path[i] == '\\')
				t_path[i] = '/';
#endif
             
        char t_sep;
        t_sep = '\0';
        
        while (!MCS_exists(*t_path_string, True))
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
        
        *t_path_end = t_sep;
    }

    MCS_setenv(MCSTR("PATH_TRANSLATED"), *t_path_string);
             
	MCAutoStringRef t_path_end_string;
	/* UNCHECKED */ MCStringCreateWithCString(t_path_end, &t_path_end_string);
	MCS_setenv(MCSTR("PATH_INFO"), *t_path_end_string);

	free(t_path);
}

static bool cgi_compute_get_var(void *p_context, MCVariable *p_var)
{
	MCAutoStringRef t_query_string;

	if (MCS_getenv(MCSTR("QUERY_STRING"), &t_query_string))
    {
        // Need to get the appropriate character pointer from the stringref
        MCAutoDataRef t_query_data;
        if (MCStringIsNative(*t_query_string))
            /* UNCHECKED */ MCDataCreateWithBytes((byte_t*)MCStringGetNativeCharPtr(*t_query_string), MCStringGetLength(*t_query_string), &t_query_data);
        else
            /* UNCHECKED */ MCDataCreateWithBytes((byte_t*)MCStringGetCharPtr(*t_query_string), 2 * MCStringGetLength(*t_query_string), &t_query_data);

        cgi_store_form_urlencoded(s_cgi_get, *t_query_data, true);
	}

    return true;
}

// MM-2011-07-13: Added new deferred variable $_GET_RAW.
//   $_GET_RAW is just a copy of the QUERY_STRING.
static bool cgi_compute_get_raw_var(void *p_context, MCVariable *p_var)
{
	MCAutoStringRef t_query_string;
	
	if (MCS_getenv(MCSTR("QUERY_STRING"), &t_query_string))
	{
        s_cgi_get_raw -> setvalueref(*t_query_string);
    }
    return true;
}

// MM-2011-07-13: Added new deferred variable $_GET_BINARY.
//   $_GET_BINARY is just the binary get data not encoded in the native charset.
static bool cgi_compute_get_binary_var(void *p_context, MCVariable *p_var)
{
	MCAutoStringRef t_query_string;	
	
	if (MCS_getenv(MCSTR("QUERY_STRING"), &t_query_string))
    {
        // Need to get the appropriate character pointer from the stringref
        MCAutoDataRef t_query_data;
        if (MCStringIsNative(*t_query_string))
            /* UNCHECKED */ MCDataCreateWithBytes((byte_t*)MCStringGetNativeCharPtr(*t_query_string), MCStringGetLength(*t_query_string), &t_query_data);
        else
            /* UNCHECKED */ MCDataCreateWithBytes((byte_t*)MCStringGetCharPtr(*t_query_string), 2 * MCStringGetLength(*t_query_string), &t_query_data);

        cgi_store_form_urlencoded(s_cgi_get_binary, *t_query_data, true);
	}
    return true;
}

// $_POST_RAW contains the entire post message and is read from stdin on access
static bool cgi_compute_post_raw_var(void *p_context, MCVariable *p_var)
{
	MCCacheHandle *t_stdin = new MCCacheHandle(s_cgi_stdin_cache);
	
	bool t_success = true;

	MCAutoStringRef t_content_length;
	
	if (MCS_getenv(MCSTR("CONTENT_LENGTH"), &t_content_length))
	{
		uint32_t t_length;
		t_length = atoi(MCStringGetCString(*t_content_length));
		
		uint32_t t_read = 0;
		
		char *t_data;
		t_data = new char[t_length];
		t_success = t_stdin->Read(t_data, t_length, t_read) && t_length == t_read;

		// Store the raw POST data
		if (t_success)
        {
            MCAutoDataRef t_valueref_data;
            MCDataCreateWithBytes((byte_t*)t_data, t_length, &t_valueref_data);
            s_cgi_post_raw -> setvalueref(*t_valueref_data);
        }
		
		delete t_data;
	}
	
	delete t_stdin;

    return t_success;
}

static bool cgi_store_form_multipart(IO_handle p_stream);

// IM-2011-08-05: Reorganization of post variables, now generating all at the same time
static bool cgi_compute_post_variables()
{
    bool t_success = true;
	
	if (s_cgi_processed_post)
        return true;
	
	s_cgi_processed_post = true;
	
	MCAutoStringRef t_content_type;

	bool gotenv = MCS_getenv(MCSTR("CONTENT_TYPE"), &t_content_type);
	if (gotenv && MCStringIsEqualToCString(*t_content_type, "application/x-www-form-urlencoded", kMCCompareCaseless))
	{
		// TODO: currently we assume that urlencoded form data is small enough to fit into memory,
		// so we fetch the contents from $_POST_RAW (which automatically reads the data from stdin).
		// in the future we should read from stdin to avoid duplicating large amounts of data
        MCExecContext ctxt;
        MCExecValue t_value;
        MCVarref *t_raw_ref;
        t_raw_ref = s_cgi_post_raw->newvarref();

        t_raw_ref->eval_ctxt(ctxt, t_value);

        if (!ctxt . HasError())
        {
            MCAutoValueRef t_value_ref;
            MCAutoDataRef t_data;

            MCExecTypeConvertToValueRefAndReleaseAlways(ctxt, t_value . type, &t_value, &t_value_ref);

            t_success = ctxt . ConvertToData(*t_value_ref, &t_data);

            if (t_success)
            {
                cgi_store_form_urlencoded(s_cgi_post_binary, *t_data, false);
                cgi_store_form_urlencoded(s_cgi_post, *t_data, true);
            }
		}
        else
            t_success = false;

		delete t_raw_ref;
	}
	else if (gotenv && MCStringBeginsWithCString(*t_content_type, (const char_t *)"multipart/form-data;", kMCStringOptionCompareCaseless))
    {
		MCCacheHandle *t_stdin = new MCCacheHandle(s_cgi_stdin_cache);
        IO_handle t_stdin_handle = t_stdin;
		
        cgi_store_form_multipart(t_stdin_handle);
		MCS_close(t_stdin_handle);
	}
    return t_success;
}

static bool cgi_compute_post_binary_var(void *p_context, MCVariable *p_var)
{
	return cgi_compute_post_variables();
}

static bool cgi_compute_post_var(void *p_context, MCVariable *p_var)
{
	return cgi_compute_post_variables();
}

static bool cgi_compute_files_var(void *p_context, MCVariable *p_var)
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

	MCAutoStringRef t_content_type;
	
	MCS_getenv(MCSTR("CONTENT_TYPE"), &t_content_type);
	
	char *t_params = NULL;
	uint32_t t_index = 0;
	
	char **t_names = NULL;
	char **t_values = NULL;
	uint32_t t_param_count = 0;
	
	t_success = MCStringFirstIndexOfChar(*t_content_type, ';', 0, kMCStringOptionCompareExact, t_index);

	if (t_success)
		t_success = MCMultiPartParseHeaderParams(MCStringGetCString(*t_content_type) + t_index + 1, t_names, t_values, t_param_count);

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
    MCNameRef name;
    MCStringRef type;

    MCStringRef file_name;
    MCStringRef temp_name;
	IO_handle file_handle;
	uint32_t file_size;
	MCMultiPartFileStatus file_status;

    MCArrayRef file_variable;

    MCStringRef boundary;
	
    MCStringRef post_variable;
    MCDataRef post_binary_variable;

} cgi_multipart_context_t;

static void cgi_dispose_multipart_context(cgi_multipart_context_t *p_context)
{
    MCValueRelease(p_context->name);
    MCValueRelease(p_context->file_name);
    MCValueRelease(p_context->type);
    MCValueRelease(p_context->boundary);
    MCValueRelease(p_context->temp_name);

	if (p_context->file_handle != NULL)
		MCS_close(p_context->file_handle);

    MCValueRelease(p_context->file_variable);
    MCValueRelease(p_context->post_binary_variable);
    MCValueRelease(p_context->post_variable);
	
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
                {
                    MCAutoStringRef t_name;
                    MCStringCreateWithCStringAndRelease(p_header->param_value[i], &t_name);
                    MCNameCreate(*t_name, t_context->name);
                }
				else if (MCCStringEqualCaseless(p_header->param_name[i], "filename"))
                    MCStringCreateWithCStringAndRelease(p_header->param_value[i], t_context->file_name);
			}
		}
		else if (MCCStringEqualCaseless(p_header->name, "Content-Type"))
		{
            MCStringCreateWithCStringAndRelease(p_header->value, t_context->type);
			
			for (uint32_t i = 0; i < p_header->param_count; i++)
			{
				if (MCCStringEqualCaseless(p_header->param_name[i], "boundary"))
                    MCStringCreateWithCStringAndRelease(p_header->param_value[i], t_context->boundary);
			}
		}
	}
	else
	{
		if (cgi_context_is_form_data(t_context))
        {
            t_success = t_context->name != NULL;
			if (t_success)
			{
                // We need to reset the binary data fetched from the global variable
                // and create a mutable DataRef
                cgi_fetch_valueref_for_key(s_cgi_post, t_context->name, (MCValueRef&)t_context->post_variable);

                cgi_fetch_valueref_for_key(s_cgi_post_binary, t_context -> name, (MCValueRef&)t_context->post_binary_variable);
                MCValueRelease(t_context->post_binary_variable);
                MCDataCreateMutable(0, t_context->post_binary_variable);
			}
		}
		else if (cgi_context_is_file(t_context))
		{
			MCStringRef t_temp_dir;
            MCAutoStringRef t_temp_name;

            t_temp_dir = cgi_get_upload_temp_dir();

            if (t_temp_dir == NULL || !MCS_exists(t_temp_dir, False))
			{
				t_context->file_status = kMCFileStatusNoUploadFolder;
			}
            else
			{
                if (MCMultiPartCreateTempFile(t_temp_dir, t_context->file_handle, &t_temp_name))
                    MCValueAssign(t_context -> temp_name, *t_temp_name);
                else
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
            t_success = MCDataAppendBytes(t_context->post_binary_variable, (const byte_t*)p_data, p_data_length);

			if (t_success && p_finished)
            {
                MCAutoStringRef t_native_string;
                if (cgi_native_from_encoding(MCserveroutputtextencoding, t_context->post_binary_variable, &t_native_string))
                    MCValueAssign(t_context->post_variable, *t_native_string);
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
            cgi_fetch_valueref_for_key(s_cgi_files, t_context->name, (MCValueRef &)t_context->file_variable);
			
			if (t_context->file_status == kMCFileStatusOK && t_context->file_size == 0)
				t_context->file_status = kMCFileStatusFailed;
			
			if (p_truncated)
                t_context->file_status = kMCFileStatusStopped;

            MCAutoNumberRef t_size;
            MCAutoStringRef t_error;

            MCNumberCreateWithUnsignedInteger(t_context->file_size, &t_size);

            MCArrayStoreValue(t_context->file_variable, false, MCNAME("name"), t_context->file_name);
            MCArrayStoreValue(t_context->file_variable, false, MCNAME("type"), t_context->type);
            MCArrayStoreValue(t_context->file_variable, false, MCNAME("filename"), t_context->temp_name);
            MCArrayStoreValue(t_context->file_variable, false, MCNAME("size"), *t_size);

            if (t_context->file_status != kMCFileStatusOK
                    && MCMultiPartGetErrorMessage(t_context->file_status, &t_error))
                MCArrayStoreValue(t_context->file_variable, false, MCNAME("error"), *t_error);
		}
	}
	
	if (t_success && p_finished)
	{
		// clear context for next part
		cgi_dispose_multipart_context(t_context);
    }
	return t_success;
}

static bool cgi_store_form_multipart(IO_handle p_stream)
{
    bool t_success = true;
	char *t_boundary = NULL;
	
	cgi_multipart_context_t t_context;
	MCMemoryClear(&t_context, sizeof(t_context));
	
	uint32_t t_bytes_read = 0;
	
	if (t_success)
		t_success = cgi_multipart_get_boundary(t_boundary);
	if (t_success)
    {
        MCAutoStringRef t_boundary_str;
        /* UNCHECKED */ MCStringCreateWithCString(t_boundary, &t_boundary_str);
		t_success = MCMultiPartReadMessageFromStream(p_stream, *t_boundary_str, t_bytes_read,
													 cgi_multipart_header_callback, cgi_multipart_body_callback, &t_context);
    }

	// clean up in case of errors;
	if (!t_success)
		cgi_dispose_multipart_context(&t_context);

	MCCStringFree(t_boundary);

	return t_success;
}

////////////////////////////////////////////////////////////////////////////////

static bool cgi_compute_cookie_var(void *p_context, MCVariable *p_var)
{
    bool t_success = true;
	MCAutoStringRef t_cookie;
	
    if (MCS_getenv(MCSTR("HTTP_COOKIE"), &t_cookie) && !MCStringIsEmpty(*t_cookie))
    {
        // Need to get the appropriate character pointer from the stringref
        MCAutoDataRef t_query_data;
        if (MCStringIsNative(*t_cookie))
            t_success = MCDataCreateWithBytes((byte_t*)MCStringGetNativeCharPtr(*t_cookie), MCStringGetLength(*t_cookie), &t_query_data);
        else
            t_success = MCDataCreateWithBytes((byte_t*)MCStringGetCharPtr(*t_cookie), 2 * MCStringGetLength(*t_cookie), &t_query_data);

        if (t_success)
            cgi_store_cookie_urlencoded(s_cgi_cookie, *t_query_data, true);
    }
    else
        // returns true if nothing had to be computed
        return true;

    return t_success;
}

////////////////////////////////////////////////////////////////////////////////

#if defined (_WINDOWS_SERVER)
_CRTIMP extern wchar_t **_wenviron;
#define environ_var _wenviron
#else
extern char **environ;
#define environ_var environ
#endif

bool cgi_initialize()
{
	s_cgi_upload_temp_dir = MCValueRetain(kMCEmptyString);
	s_cgi_temp_dir = MCValueRetain(kMCEmptyString);
	// need to ensure PATH_TRANSLATED points to the script and PATH_INFO contains everything that follows
	cgi_fix_path_variables();

	// Resolve the main script that has been requested by the CGI interface.
	MCAutoStringRef t_env;

	if (MCS_getenv(MCSTR("PATH_TRANSLATED"), &t_env))
        MCsystem -> ResolvePath(*t_env, MCserverinitialscript);

	
	// Set the current folder to be that containing the CGI file.
	char *t_server_script_folder;
    t_server_script_folder = strdup(MCStringGetCString(MCserverinitialscript));

	// Windows paths have been fixed - no backslashes in the environment variables
	strrchr(t_server_script_folder, '/')[0] = '\0';

    MCAutoStringRef t_server_script_folder_string;
    /* UNCHECKED */ MCStringCreateWithCString(t_server_script_folder, &t_server_script_folder_string);

	MCsystem -> SetCurrentFolder(*t_server_script_folder_string);
	delete t_server_script_folder;
	
	// Initialize the headers.
	MCservercgiheaders = NULL;
	MCservercgiheaders_sent = false;
	
    // Get the document root
    /* UNCHECKED */ MCStringCreateWithCString(getenv("DOCUMENT_ROOT"), MCservercgidocumentroot);
	
	// Initialize the input wrapper.  This creates a cache of the input stream
	// which is filled as the stream is read from.  this allows stdin to be used
	// to populate the post data arrays, and also to be read from by the script
	// without conflicting
    s_cgi_stdin_cache = new MCStreamCache(IO_stdin);
    IO_stdin = new MCCacheHandle(s_cgi_stdin_cache);
		
	// Initialize the output wrapper, this simply ensures we output headers
	// before any content.
    IO_stdout = new cgi_stdout;
	
	// Construct the _SERVER variable
    /* UNCHECKED */ MCVariable::createwithname(MCNAME("$_SERVER"), s_cgi_server);
	s_cgi_server -> setnext(MCglobals);
	MCglobals = s_cgi_server;
	
	MCAutoArrayRef t_vars;
    /* UNCHECKED */ MCArrayCreateMutable(&t_vars);
	for(uint32_t i = 0; environ_var[i] != NULL; i++)
	{
        MCAutoStringRef t_environ;
#ifdef _LINUX_SERVER
        MCStringCreateWithSysString(environ_var[i], &t_environ);
#elif defined (_DARWIN_SERVER) || defined(_MAC_SERVER)
        MCStringCreateWithBytes((byte_t*)environ_var[i], strlen(environ_var[i]), kMCStringEncodingUTF8, false, &t_environ);
#elif defined (_WINDOWS_SERVER)
        MCStringCreateWithWString(environ_var[i], &t_environ);
#endif
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
		
        if (MCStringBeginsWithCString(*t_environ, (const char_t*)"HTTP_", kMCStringOptionCompareCaseless))
            t_found = true;
		else
			for(uint32_t j = 0; s_cgi_vars[j] != NULL && !t_found; j++)
                if (MCStringBeginsWithCString(*t_environ, (const char_t*)s_cgi_vars[j], kMCStringOptionCompareCaseless))
					t_found = true;

		if (t_found)
        {
            MCAutoStringRef t_value;
            MCAutoStringRef t_key;
            MCNameRef t_key_name;

            if (!MCStringDivideAtChar(*t_environ, '=', kMCStringOptionCompareExact, &t_key, &t_value))
            {
                t_key = *t_environ;
                t_value = kMCEmptyString;
            }

            /* UNCHECKED */ MCNameCreate(*t_key, t_key_name);
            /* UNCHECKED */ s_cgi_server->setvalueref(&t_key_name, 1, false, *t_value);
            MCNameDelete(t_key_name);
		}
	}
	
	MCresult -> setvalueref(*t_vars);
	
	// Construct the GET variables by parsing the QUERY_STRING
	
	/* UNCHECKED */ MCDeferredVariable::createwithname(MCNAME("$_GET_RAW"), cgi_compute_get_raw_var, nil, s_cgi_get_raw);
	s_cgi_get_raw -> setnext(MCglobals);
	MCglobals = s_cgi_get_raw;	
	/* UNCHECKED */ MCDeferredVariable::createwithname(MCNAME("$_GET"), cgi_compute_get_var, nil, s_cgi_get);
	s_cgi_get -> setnext(MCglobals);
	MCglobals = s_cgi_get;
	/* UNCHECKED */ MCDeferredVariable::createwithname(MCNAME("$_GET_BINARY"), cgi_compute_get_binary_var, nil, s_cgi_get_binary);
	s_cgi_get_binary -> setnext(MCglobals);
	MCglobals = s_cgi_get_binary;	
	
	// Construct the _POST variables by reading stdin.
	
	/* UNCHECKED */ MCDeferredVariable::createwithname(MCNAME("$_POST_RAW"), cgi_compute_post_raw_var, nil, s_cgi_post_raw);
	s_cgi_post_raw -> setnext(MCglobals);
	MCglobals = s_cgi_post_raw;
	/* UNCHECKED */ MCDeferredVariable::createwithname(MCNAME("$_POST"), cgi_compute_post_var, nil, s_cgi_post);
	s_cgi_post -> setnext(MCglobals);
	MCglobals = s_cgi_post;
	/* UNCHECKED */ MCDeferredVariable::createwithname(MCNAME("$_POST_BINARY"), cgi_compute_post_binary_var, nil, s_cgi_post_binary);
	s_cgi_post_binary -> setnext(MCglobals);
	MCglobals = s_cgi_post_binary;	
	
	// Construct the FILES variable by reading stdin

	/* UNCHECKED */ MCDeferredVariable::createwithname(MCNAME("$_FILES"), cgi_compute_files_var, nil, s_cgi_files);
	s_cgi_files -> setnext(MCglobals);
	MCglobals = s_cgi_files;
	
	// Construct the COOKIES variable by parsing HTTP_COOKIE
	/* UNCHECKED */ MCDeferredVariable::createwithname(MCNAME("$_COOKIE"), cgi_compute_cookie_var, nil, s_cgi_cookie);
	s_cgi_cookie -> setnext(MCglobals);
	MCglobals = s_cgi_cookie;
	
	// Create the $_SESSION variable explicitly, to be populated upon calls to "start session"
	// required as implicit references to "$_SESSION" will result in its creation as an env var
	MCVariable *t_session_var = NULL;
	/* UNCHECKED */ MCVariable::createwithname(MCNAME("$_SESSION"), t_session_var);
	t_session_var->setnext(MCglobals);
	MCglobals = t_session_var;

	return true;
}

void cgi_finalize_session();

void cgi_finalize()
{
	MCValueRelease(s_cgi_upload_temp_dir);
    MCValueRelease(s_cgi_temp_dir);
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
    MCExecContext ctxt;
	
	for (uint32_t i = 0; t_success && i < MCservercgicookiecount; i++)
	{
		t_success = MCCStringFormat(t_cookie_header, "Set-Cookie: %s=%s", MCservercgicookies[i].name, MCservercgicookies[i].value);
		
		if (t_success && MCservercgicookies[i].expires != 0)
		{
			MCAutoNumberRef t_num;
			MCAutoStringRef t_string;
			/* UNCHECKED */ MCNumberCreateWithInteger(MCservercgicookies[i].expires, &t_num);
			t_success = MCD_convert(ctxt, *t_num, CF_SECONDS, CF_UNDEFINED, CF_INTERNET_DATE, CF_UNDEFINED, &t_string);
			if (t_success)
			{
				t_success = MCCStringAppendFormat(t_cookie_header, "; Expires=%s", MCStringGetCString(*t_string));
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

bool MCServerGetSessionIdFromCookie(MCExecContext &ctxt, MCStringRef &r_id);

MCSession *s_current_session = NULL;

bool MCServerStartSession(MCExecContext &ctxt)
{
	// start session:
	// if MCsessionid is empty - get id from cookie
	// if MCsessionid not empty - use MCsessionid - clear MCsessionid once opened
	bool t_success = true;
	
	if (s_current_session != NULL)
		return true;

	MCAutoStringRef t_session_id;
	MCAutoStringRef t_cookie_id;

	if (MCsessionid == nil)
	{
        t_success = MCServerGetSessionIdFromCookie(ctxt, &t_cookie_id);
		t_session_id = *t_cookie_id;
	}
	else
        /* UNCHECKED */ MCStringCopy(MCsessionid, &t_session_id);
	
	if (t_success)
	{
		t_success = MCSessionStart(*t_session_id, s_current_session);
	}
	
	MCVariable *t_session_var = NULL;
	
	if (t_success)
	{
		t_session_var = MCVariable::lookupglobal_cstring("$_SESSION");
		if (t_session_var == NULL)
		{
			t_success = MCVariable::createwithname(MCNAME("$_SESSION"), t_session_var);
			if (t_success)
			{
				t_session_var->setnext(MCglobals);
				MCglobals = t_session_var;
			}
		}
	}
	if (t_success)
		t_session_var->clear();
	
	if (t_success)
    {
        if (s_current_session->data_length > 0)
            t_success = t_session_var->decode(s_current_session->data, s_current_session->data_length);
	}
	
	if (t_success)
	{
		// clear MCsessionid - now associated with open session
        MCValueRelease(MCsessionid);
		MCsessionid = NULL;
	}
	else
	{
		MCSessionDiscard(s_current_session);
		s_current_session = NULL;
	}
	
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
        t_success = t_session_var->encode((void*&)t_data, t_data_length);

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
	
	MCAutoStringRef t_id;
	t_success = MCS_get_session_id(&t_id);

	if (t_success)
		t_success = MCSessionExpire(*t_id);
	
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

static MCStringRef s_session_temp_dir = NULL;
bool MCS_get_temporary_folder(MCStringRef &r_folder);

bool MCS_set_session_save_path(MCStringRef p_path)
{
    MCValueAssign(MCsessionsavepath, p_path);
	return true;
}

bool MCS_get_session_save_path(MCStringRef& r_path)
{
    if (!MCStringIsEmpty(MCsessionsavepath))
        return MCStringCopy(MCsessionsavepath, r_path);
	
	if (s_session_temp_dir != NULL)
	{
		r_path = MCValueRetain(s_session_temp_dir);
		return true;
    }
	
	if (MCS_get_temporary_folder(r_path))
		return true;
	
	r_path = MCValueRetain(kMCEmptyString);
	return true;
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

bool MCS_set_session_name(MCStringRef p_name)
{
    MCValueAssign(MCsessionname, p_name);
	return true;
}

bool MCS_get_session_name(MCStringRef &r_name)
{
    if (!MCStringIsEmpty(MCsessionname))
        return MCStringCopy(MCsessionname, r_name);
	
	return MCStringCreateWithCString("LCSESSION", r_name);
}

bool MCS_set_session_id(MCStringRef p_id)
{
	if (s_current_session != NULL)
        return false;

    MCValueAssign(MCsessionid, p_id);
	return true;
}

bool MCS_get_session_id(MCStringRef& r_id)
{
	if (s_current_session != NULL)
		return MCStringCreateWithCString(s_current_session->id, r_id);
    // SN-2014-06-13 [[ RefactorServer ]]
    // Having a nil pointer causes a silent crash on the server prior to 7.0
    // but a more violent one with the StringRefs. Fixed
    else if (MCsessionid != NULL)
        return MCStringCopy(MCsessionid, r_id);
    else
        return MCStringCopy(kMCEmptyString, r_id);
}

bool MCServerGetSessionIdFromCookie(MCExecContext &ctxt, MCStringRef &r_id)
{
	MCVariable *t_cookie_array;
    t_cookie_array = MCVariable::lookupglobal(MCNAME("$_COOKIE"));
	
	if (t_cookie_array == NULL)
	{
		r_id = MCValueRetain(kMCEmptyString);
		return true;
	}
	
	// ensure cookie array is evaluated
    if (t_cookie_array->isdeferred() && !((MCDeferredVariable*)t_cookie_array)->compute())
		return false;

	MCAutoStringRef t_name;
	if (!MCS_get_session_name(&t_name))
		return false;
    MCNewAutoNameRef t_key;
	/* UNCHECKED */ MCNameCreate(*t_name, &t_key);
		
    if (!ctxt.CopyOptElementAsString((MCArrayRef)t_cookie_array -> getvalueref(), *t_key, false, r_id))
		return false;
	
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
		unichar_t t_input_char;
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
		unichar_t t_input_char;
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
		unichar_t t_input_char;
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
			t_codepoint = MCUnicodeCodepointAdvance((const unichar_t *)p_chars, p_char_count, t_index);
			
			while(t_index < p_char_count)
			{
				uint4 t_old_index;
				t_old_index = t_index;
				
				t_codepoint = MCUnicodeCodepointAdvance((const unichar_t *)p_chars, p_char_count, t_index);
				
				if (MCUnicodeCodepointIsBase(t_codepoint))
				{
					t_index = t_old_index;
					break;
				}
			}

			uint8_t t_char;
			if (!MCUnicodeMapToNative((const unichar_t*)p_chars + t_start, t_index - t_start, t_char))
				t_char = '?';
			
			t_output[t_output_length++] = t_char;
		}
	}

	MCMemoryReallocate(t_output, t_output_length, t_output);

	r_output = t_output;
	r_output_length = t_output_length;

	return true;
}
