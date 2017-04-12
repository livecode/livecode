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

#include "prefix.h"

#include "sysdefs.h"
#include "filedefs.h"
#include "mcio.h"
#include "osspec.h"
#include "parsedef.h"


#include "system.h"
#include "srvmultipart.h"


////////////////////////////////////////////////////////////////////////////////

// utility class to read from a stream up to (but not including) a specified
// boundary.  useful for parsing multipart mime messages, where we want to read
// individual parts without loading the whole thing into memory

class MCBoundaryReader
{
public:
	IO_handle m_stream;
	MCStringRef m_boundary;
	int32_t *m_table;
	
	uint32_t m_match_index;
	uint32_t m_match_frontier;
	
	char m_match_char;
	bool m_have_char;
	
	MCBoundaryReader(IO_handle p_stream, MCStringRef p_boundary)
	{
		m_stream = p_stream;
		m_table = NULL;
        // SN-2015-02-09: [[ Bug 14477 ]] Initialise to NULL,
        //  otherwise setBoundary calls MCValueRelease on some
        //  random position in the memory.
        m_boundary = NULL;
		
		setBoundary(p_boundary);
		
		m_have_char = false;
	}
	
	~MCBoundaryReader()
	{
		MCMemoryDeleteArray(m_table);
        // SN-2015-02-09: [[ Bug 14477 ]] Release what we retained.
        MCValueRelease(m_boundary);
	}
	
	void setBoundary(MCStringRef p_boundary)
	{
		MCValueAssign(m_boundary, p_boundary);
		
		m_match_index = 0;
		m_match_frontier = 0;
		
		if (m_table != NULL)
		{
			MCMemoryDeleteArray(m_table);
			m_table = NULL;
		}
		create_table();
	}
	
	IO_stat read(char *r_buffer, uint32_t p_buffer_size, uint32_t &r_bytes_read, uint32_t &r_bytes_consumed, bool &r_boundary_reached)
	{
		IO_stat t_status = IO_NORMAL;
		
		r_boundary_reached = false;
		r_bytes_read = 0;
		r_bytes_consumed = 0;
		
		while ((!r_boundary_reached) && r_bytes_read < p_buffer_size)
		{
			if (!m_have_char)
			{
				t_status = MCS_readfixed(&m_match_char, 1, m_stream);
				if (t_status != IO_NORMAL)
					break;
				
				r_bytes_consumed++;
				m_have_char = true;
			}
			if (MCStringGetNativeCharAtIndex(m_boundary, m_match_index) == m_match_char)
			{
				m_match_index++;
				m_have_char = false;
				if (m_match_index == MCStringGetLength(m_boundary))
				{
					m_match_index = 0;
					r_boundary_reached = true;
				}
			}
			else
			{
				if (m_match_index == 0)
				{
					r_buffer[r_bytes_read++] = m_match_char;
					m_have_char = false;
				}
				else
				{
					// MDW-2013-04-15: [[ x64 ]] m_match_frontier is unsigned
					// so making t_diff unsigned to avoid compiler warning.
					uint32_t t_diff = m_match_index - m_table[m_match_index];
					if (m_match_frontier != t_diff)
					{
						uint32_t t_out = MCMin(t_diff - m_match_frontier, p_buffer_size - r_bytes_read);
                        
						MCMemoryCopy(r_buffer + r_bytes_read, MCStringGetCString(m_boundary) + m_match_frontier, t_out);
						m_match_frontier += t_out;
						r_bytes_read += t_out;
					}
					if (m_match_frontier == t_diff)
					{
						m_match_frontier = 0;
						m_match_index = m_table[m_match_index];
					}
				}
			}
		}
		
		return t_status;
	}
	
private:
	// constructs a fallback table, indicating the latest possible candidate match in the boundary,
	// should the current comparison fail
	// based on KMP algorithm (see http://en.wikipedia.org/wiki/Knuth–Morris–Pratt_algorithm)
	void create_table()
	{
		MCMemoryNewArray(MCStringGetLength(m_boundary), m_table);
		m_table[0] = -1;
		if (MCStringGetLength(m_boundary) > 1)
			m_table[1] = 0;
		
		uint32_t t_candidate = 0;
		uint32_t pos = 2;
		while (pos < MCStringGetLength(m_boundary))
		{
			if (MCStringGetNativeCharAtIndex(m_boundary, pos - 1) == MCStringGetNativeCharAtIndex(m_boundary, t_candidate))
				m_table[pos++] = ++t_candidate;
			else if (t_candidate > 0)
				t_candidate = m_table[t_candidate];
			else
				m_table[pos++] = 0;
		}
	}
};

////////////////////////////////////////////////////////////////////////////////

// rfc 822
static inline bool is_whitespace(char p_char)
{
	return p_char == ' ' || p_char == '\t';
}

static inline bool is_ctl(char p_char)
{
	return p_char >= 0 && p_char <= 31;
}

static inline bool is_crlf(const char *p_str)
{
	return p_str[0] == '\r' && p_str[1] == '\n';
}

static inline bool is_folded_lwsp(const char *p_str)
{
	return is_crlf(p_str) && is_whitespace(p_str[2]);
}

static inline bool is_field_name_char(char p_char)
{
	return (!is_ctl(p_char)) && (p_char != ' ') && (p_char != ':');
}

// rfc 2045
static char s_tspecials[] = {'(', ')', '<', '>', '@', ',', ';', ':', '\\', '"', '/', '[', ']', '?', '='};

bool is_tspecial(char p_char)
{
	for (uint32_t i = 0; i < sizeof(s_tspecials); i++)
		if (s_tspecials[i] == p_char)
			return true;
	return false;
}

bool is_token_char(char p_char)
{
	return (p_char != ' ') && (!is_ctl(p_char)) && (!is_tspecial(p_char));
}

////////////////////////////////////////////////////////////////////////////////

static bool skip_whitespace(const char *&x_header)
{
	if (is_whitespace(*x_header))
	{
		x_header++;
		return true;
	}
	else if (is_folded_lwsp(x_header))
	{
		x_header += 3;
		return true;
	}
	
	return false;
}

void consume_whitespace(const char *&x_header)
{
	while (skip_whitespace(x_header))
		;
}

bool read_name(const char *p_header, const char * &r_name_end, char *&r_name)
{
	bool t_success = true;
	
	char *t_name = NULL;
	uint32_t t_index = 0;
	
	while (t_success && is_field_name_char(p_header[t_index]))
	{
		t_index++;
	}
	if (t_success)
		t_success = MCCStringCloneSubstring(p_header, t_index, t_name);
	if (t_success)
	{
		r_name = t_name;
		r_name_end = p_header + t_index;
	}
	else
		MCCStringFree(t_name);
	
	return t_success;
}

bool read_token(const char *p_header, const char *&r_token_end, char *&r_token)
{
	bool t_success = true;
	
	char *t_token = NULL;
	uint32_t t_index = 0;
	
	while (t_success && is_token_char(p_header[t_index]))
		t_index++;
	t_success = t_index > 0;
	if (t_success)
		t_success = MCCStringCloneSubstring(p_header, t_index, t_token);
	if (t_success)
	{
		r_token_end = p_header + t_index;
		r_token = t_token;
	}
	return t_success;
}

static inline bool is_qtext(char p_char)
{
	return (p_char != '"') && (p_char != '\\') && (p_char != '\r');
}

bool read_quoted_string(const char *p_header, const char *&r_string_end, char *&r_string)
{
	bool t_success = true;
	char *t_string = NULL;
	uint32_t t_strlen = 0;
	
	if (p_header[0] != '"')
		return false;
	
	p_header++;
	while (t_success && p_header[0] != '"')
	{
		char t_char;
		if (is_folded_lwsp(p_header))
		{
			t_char = p_header[2];
			p_header += 3;
		}
		else if (p_header[0] == '\\')
		{
			t_char = p_header[1];
			p_header += 2;
		}
		else if (is_qtext(p_header[0]))
		{
			t_char = p_header[0];
			p_header += 1;
		}
		else
			t_success = false;
		if (t_success)
			t_success = MCMemoryReallocate(t_string, t_strlen + 2, t_string);
		if (t_success)
		{
			t_string[t_strlen++] = t_char;
		}
	}
	
	if (t_success)
	{
		if (t_string != NULL)
			t_string[t_strlen] = '\0';
		r_string = t_string;
		r_string_end = p_header + 1;
	}
	else
		MCCStringFree(t_string);
	
	return t_success;
}

bool MCMultiPartParseHeaderParams(const char *p_params, char **&r_names, char **&r_values, uint32_t &r_param_count)
{
	bool t_success = true;
	
	const char *t_next_param = NULL;
	char *t_name = NULL;
	char *t_value = NULL;
	uint32_t t_index = 0;
	
	t_next_param = p_params;
	
	r_names = NULL;
	r_values = NULL;
	r_param_count = 0;
	
	while (t_success && t_next_param != NULL)
	{
		p_params = t_next_param;
		consume_whitespace(p_params);
		
		bool t_has_value = false;
		
		t_success = read_token(p_params, p_params, t_name);
		if (t_success)
		{
			consume_whitespace(p_params);
			t_has_value = p_params[0] == '=';
			p_params++;
		}
		if (t_success && t_has_value)
		{
			t_success = read_token(p_params, p_params, t_value) || read_quoted_string(p_params, p_params, t_value);
		}
		if (t_success)
		{
			consume_whitespace(p_params);
			if (MCCStringFirstIndexOf(p_params, ';', t_index))
			{
				t_next_param = p_params + t_index + 1;
			}
			else
			{
				t_next_param = NULL;
			}
			
			t_success = MCMemoryResizeArray(r_param_count + 1, r_names, r_param_count) &&
			MCMemoryResizeArray(r_param_count, r_values, r_param_count);
		}
		if (t_success)
		{
			r_names[r_param_count - 1] = t_name;
			r_values[r_param_count - 1] = t_value;
		}
		else
		{
			MCCStringFree(t_name);
			MCCStringFree(t_value);
		}
		t_name = t_value = NULL;
	}

	return t_success;
}

bool MCMultiPartParseHeader(const char *p_header, MCMultiPartHeaderCallback p_callback, void *p_context)
{
	MCMultiPartHeader t_header = {NULL, NULL, 0, NULL, NULL};
	
	bool t_success = true;
	
	uint32_t t_index = 0;
	const char *t_next_param = NULL;
	
	t_success = read_name(p_header, p_header, t_header.name);
	if (t_success)
	{
		consume_whitespace(p_header);
		t_success = p_header[0] == ':';
		p_header++;
	}

	if (t_success)
	{
		consume_whitespace(p_header);

		if (MCCStringFirstIndexOf(p_header, ';', t_index))
		{
			t_next_param = p_header + t_index + 1;
		}
		else
			t_index = MCCStringLength(p_header);

		while (t_index > 0 && p_header[t_index - 1] == ' ')
			t_index--;
		
		t_success = MCCStringCloneSubstring(p_header, t_index, t_header.value);
		
		if (t_success && t_next_param != NULL)
			t_success = MCMultiPartParseHeaderParams(t_next_param, t_header.param_name, t_header.param_value, t_header.param_count);
	}
	
	if (t_success)
		t_success = p_callback(p_context, &t_header);
	
	for (uint32_t i = 0; i < t_header.param_count; i++)
	{
		MCCStringFree(t_header.param_name[i]);
		MCCStringFree(t_header.param_value[i]);
	}
	
	MCMemoryDeleteArray(t_header.param_name);
	MCMemoryDeleteArray(t_header.param_value);
	
	MCCStringFree(t_header.name);
	MCCStringFree(t_header.value);
	
	return t_success;
}

bool MCMultiPartReadHeaders(IO_handle p_stream, uint32_t &r_bytes_read, MCMultiPartHeaderCallback p_callback, void *p_context)
{
	bool t_success = true;
	
	MCBoundaryReader *t_reader;
	t_reader = new (nothrow) MCBoundaryReader(p_stream, MCSTR("\r\n"));
	
	r_bytes_read = 0;
	
	char *t_line_buffer = NULL;
	uint32_t t_line_buffer_size = 1024;
	if (t_success)
		t_success = MCMemoryAllocate(1024, t_line_buffer);
	
	if (t_success)
	{
		while (t_success)
		{
			bool t_have_line = false;
			uint32_t t_frontier = 0;
			while (t_success && !t_have_line)
			{
				uint32_t t_line_size;
				uint32_t t_line_bytes_read;
				uint32_t t_max_bytes = t_line_buffer_size - t_frontier - 1;
				t_success = t_max_bytes > 0;
				if (t_success)
				{
					IO_stat t_status = t_reader->read(t_line_buffer + t_frontier, t_max_bytes, t_line_size, t_line_bytes_read, t_have_line);
					r_bytes_read += t_line_bytes_read;
					t_frontier += t_line_size;
					t_success = (t_status == IO_NORMAL);
				}
				if (t_success && !t_have_line)
				{
					t_line_buffer_size += 1024;
					t_success = MCMemoryReallocate(t_line_buffer, t_line_buffer_size, t_line_buffer);
				}
			}
			if (t_success)
			{
				t_line_buffer[t_frontier] = '\0';
				
				if (t_frontier == 0)
				{
					t_success = p_callback(p_context, NULL);
					break;
				}
				else
					t_success = MCMultiPartParseHeader(t_line_buffer, p_callback, p_context);
			}
		}
	}
	
	if (t_line_buffer != NULL)
		MCMemoryDeallocate(t_line_buffer);
	
	if (t_reader != NULL)
		delete t_reader;
	
	return t_success;
}

bool MCMultiPartReadMessageFromStream(IO_handle p_stream, MCStringRef p_boundary, uint32_t &r_total_bytes_read,
									  MCMultiPartHeaderCallback p_header_callback, MCMultiPartBodyCallback p_body_callback, void *p_context)
{
	bool t_success = true;
	MCStringRef t_boundary;
	uint32_t t_boundary_length;
	uint32_t t_bytes_read = 0;
	uint32_t t_bytes_consumed = 0;

	r_total_bytes_read = 0;

	char *t_buffer = NULL;
	uint32_t t_buffer_size = 4096;
	
	char t_crlf[2] = {'\0', '\0'};
	
	MCBoundaryReader *t_reader = NULL;

	t_boundary_length = MCStringGetLength(p_boundary);
	// typical boundary == CRLF & "--" & p_boundary
	if (t_success)
		t_success = MCStringFormat(t_boundary, "\r\n--%@", p_boundary);
	
	if (t_success)
		t_success = MCMemoryAllocate(t_buffer_size, t_buffer);
	
	// the first boundary should either be the first thing we read, or should occur immediately
	// after a CRLF
	if (t_success)
	{
        // SN-2015-02-26: [[ CID 37861 ]] Make sure that we never try to create
        //  a MCBoundaryReader with a NULL p_boundary.
        MCAutoStringRef t_boundary_head, t_boundary_tail;
        if (!MCStringDivideAtIndex(t_boundary, 2, &t_boundary_head, &t_boundary_tail))
            t_success = false;
        else
        {
            t_reader = new (nothrow) MCBoundaryReader(p_stream, *t_boundary_tail);
            t_success = t_reader != NULL;
        }
	}
	
	bool t_boundary_reached = false;
	
	while (t_success && !t_boundary_reached)
	{
		t_success = IO_NORMAL == t_reader->read(t_buffer, t_buffer_size, t_bytes_read, t_bytes_consumed, t_boundary_reached);
		r_total_bytes_read += t_bytes_consumed;
		if (t_success)
		{
			if (t_bytes_read > 1)
			{
				// remember last two characters read
				t_crlf[0] = t_buffer[t_bytes_read - 2];
				t_crlf[1] = t_buffer[t_bytes_read - 1];
			}
			else if (t_bytes_read == 1)
			{
				t_crlf[0] = t_crlf[1];
				t_crlf[1] = t_buffer[0];
			}
		}
	}
	
	if (t_success && t_boundary_reached)
	{
		t_success = (r_total_bytes_read == (t_boundary_length + 2)) || MCCStringEqualSubstring(t_crlf, "\r\n", 2);
	}
	
	if (t_success)
	{
		t_reader->setBoundary(t_boundary);
		bool t_message_ended = false;
		while (t_success && (!t_message_ended))
		{
			if (t_boundary_reached)
			{
				// consume preceding CRLF
				// if this if the last part, the boundary will be followed by '--'
				uint32_t t_count = 1;
				char t_char;
				t_crlf[0] = t_crlf[1] = ' ';
				
				// check for spaces at end of boundary line.
				while (t_success && t_crlf[0] == ' ')
				{
					t_success = IO_NORMAL == MCS_readall(&t_char, t_count, p_stream, t_count); // ?? readall ??
					t_crlf[0] = t_crlf[1];
					t_crlf[1] = t_char;
					r_total_bytes_read += t_count;
				}
				if (t_success)
				{
					if (MCCStringEqualSubstring(t_crlf, "--", 2))
					{
						t_message_ended = true;
					}
					else if (MCCStringEqualSubstring(t_crlf, "\r\n", 2))
					{
						t_success = MCMultiPartReadHeaders(p_stream, t_bytes_consumed, p_header_callback, p_context);
						r_total_bytes_read += t_bytes_consumed;
						t_boundary_reached = false;
					}
					else
					{
						t_success = false;
					}
				}
			}
			
			while (t_success && !t_boundary_reached)
			{
				IO_stat t_state;
				t_state = t_reader->read(t_buffer, t_buffer_size, t_bytes_read, t_bytes_consumed, t_boundary_reached);
				r_total_bytes_read += t_bytes_consumed;

				t_success = p_body_callback(p_context, t_buffer, t_bytes_read, t_boundary_reached, t_state == IO_EOF);

				t_success &= t_state == IO_NORMAL;
			}
		}
	}
	
	if (t_buffer != NULL)
		MCMemoryDeallocate(t_buffer);
	if (t_reader != NULL)
		delete t_reader;
	if (t_boundary != nil)
		MCValueRelease(t_boundary);

	return t_success;
}

////////////////////////////////////////////////////////////////////////////////

// maintain list of opened temp files
// on exit, remove lock, and delete (if not moved)
// on startup, look for temp lock files (.lc_lock_$tempname)
// and delete any orphan files

#ifdef MULTIPART_USE_LOCKS

#define LOCK_PREFIX ".lock_"
#define TEMP_PREFIX "lc_post_"

static bool lock_file(IO_handle p_handle)
{
	bool t_success = true;
	
	if (p_handle->handle == NULL)
		return false;

	t_success = p_handle->handle->Lock(false, false);
	
	return t_success;
}

static bool temp_file_name(const char *p_temp_folder, const char *p_prefix, char *&r_temp_name)
{
	bool t_success = true;
	char *t_temp_name = NULL;
	
#if defined _WINDOWS_SERVER
	t_success = MCMemoryAllocate(MAX_PATH, t_temp_name);
	if (t_success)
	{
		t_success = 0 != GetTempFileName(p_temp_folder, p_prefix, 0, t_temp_name);
	}
#elif defined _MAC_SERVER || defined _LINUX_SERVER
	t_success = NULL != (t_temp_name = tempnam(p_temp_folder, p_prefix));
#endif
	if (t_success)
		r_temp_name = t_temp_name;
	else
		MCCStringFree(t_temp_name);
	
	return t_success;
}

static bool open_file_locked(const char *p_filename, const char *p_mode, IO_handle &r_file_handle, bool &r_locked)
{
	bool t_success = true;

	IO_handle t_file_handle;

	r_locked = false;
	t_success = NULL != (t_file_handle = MCS_open(p_filename, p_mode, false, false, 0));
	
	if (t_success)
	{
		t_success = lock_file(t_file_handle);
		if (!t_success)
			r_locked = true;
	}
	
	if (t_success)
		r_file_handle = t_file_handle;
	else
		MCS_close(t_file_handle);
	
	return t_success;
}

static bool create_temp_file_with_lock(const char *p_temp_folder, IO_handle &r_temp_file, IO_handle &r_lock_file, char *&r_temp_name, char *&r_lock_name, bool &r_blocked)
{
	bool t_success = true;
		
	IO_handle t_lock_file = NULL;
	IO_handle t_temp_file = NULL;
	
	int t_lock_fd = -1;
	
	char *t_lock_name = NULL;
	char *t_temp_name = NULL;
		
	uint32_t t_index = 0;
	r_blocked = false;

	// get lock on lock file
	t_success = temp_file_name(p_temp_folder, LOCK_PREFIX, t_lock_name);

	if (t_success)
		t_success = MCCStringLastIndexOf(t_lock_name, LOCK_PREFIX, t_index);

	if (t_success)
	{
		t_success = MCCStringFormat(t_temp_name, "%s/"TEMP_PREFIX"%s", p_temp_folder, t_lock_name + t_index + strlen(LOCK_PREFIX));
	}

	if (t_success)
		t_success = NULL != (t_lock_file = MCS_open(t_lock_name, IO_WRITE_MODE, false, false, 0));
	
	if (t_success)
	{
		t_success = lock_file(t_lock_file);
		r_blocked = !t_success;
	}
	
	if (t_success)
		t_success = NULL != (t_temp_file = MCS_open(t_temp_name, IO_WRITE_MODE, false, false, 0));

	if (t_success)
	{
		r_temp_file = t_temp_file;
		r_lock_file = t_lock_file;
		r_temp_name = t_temp_name;
		r_lock_name = t_lock_name;
	}
	else
	{
		MCCStringFree(t_temp_name);
		if (t_lock_file != NULL)
		{
			if (!r_blocked)
				MCS_unlink(t_lock_name);
			MCS_close(t_lock_file);
		}
	}
	
	MCCStringFree(t_lock_name);
	
	return t_success;
}

////////////////////////////////////////////////////////////////////////////////

typedef struct _mcmultiparttempfilelist
{
	IO_handle file;
	IO_handle lock;
	char *file_name;
	char *lock_name;

	struct _mcmultiparttempfilelist *next;
} MCMultiPartTempFileList;

static MCMultiPartTempFileList *s_temp_files = NULL;

bool MCMultiPartCreateTempFile(MCStringRef p_temp_folder, IO_handle &r_file_handle, MCStringRef &r_temp_name)
{
	bool t_success = true;
	bool t_blocked = true;
	
	IO_handle t_file_handle = NULL;
	
	MCMultiPartTempFileList *t_list_item = NULL;
	
	if (t_success)
		t_success = MCMemoryNew(t_list_item);

	if (t_success)
	{
		while (t_blocked)
			t_success = create_temp_file_with_lock(MCStringGetCString(p_temp_folder), t_list_item->file, t_list_item->lock, t_list_item->file_name, t_list_item->lock_name, t_blocked);
	}
	if (t_success)
	{
		t_list_item->next = s_temp_files;
		s_temp_files = t_list_item;
		
		r_file_handle = t_list_item->file;
        /* UNCHECKED */ MCStringCreateWithCString(t_list_item->file_name, r_temp_name);
	}
	else
	{
		MCS_close(t_file_handle);
	}
	
	return t_success;
}

void MCMultiPartRemoveTempFiles()
{
	MCMultiPartTempFileList *t_item;
	while (s_temp_files != NULL)
	{
		MCS_unlink(s_temp_files->file_name);
		MCCStringFree(s_temp_files->file_name);

		MCS_unlink(s_temp_files->lock_name);
		MCCStringFree(s_temp_files->lock_name);
		MCS_close(s_temp_files->lock);
		
		t_item = s_temp_files;
		s_temp_files = s_temp_files->next;
		MCMemoryDelete(t_item);
	}
}

void MCMultiPartCleanTempFolder(const char *p_temp_folder)
{
	bool t_success = true;
	MCExecPoint ep;
	
	char **t_files = NULL;
	uint32_t t_file_count = 0;
	
	MCS_getentries(ep, true, false);
	
	MCString t_filelist = ep.getsvalue0();
	
	t_success = MCCStringSplit(t_filelist.getstring(), '\n', t_files, t_file_count);
	
	if (t_success)
	{
		for (uint32_t i = 0; i < t_file_count; i++)
		{
			if (MCCStringBeginsWith(t_files[i], LOCK_PREFIX))
			{
				char *t_lockpath = NULL;
				char *t_filepath = NULL;
				bool t_blocked = false;
				
				IO_handle t_lock_file = NULL;
				t_success = MCCStringFormat(t_lockpath, "%s/%s", p_temp_folder, t_files[i]);
				if (t_success)
					t_success = open_file_locked(t_lockpath, IO_READ_MODE, t_lock_file, t_blocked);
				
				if (t_success)
				{
					t_success = MCCStringFormat(t_filepath, "%s/"TEMP_PREFIX"%s", p_temp_folder, t_files + strlen(LOCK_PREFIX));
				}
				
				if (t_success)
					t_success = (!MCS_exists(t_filepath, True)) || MCS_unlink(t_filepath);

				if (t_success)
					MCS_unlink(t_lockpath);
				
				if (t_lock_file)
					MCS_close(t_lock_file);
				
				MCCStringFree(t_filepath);
				MCCStringFree(t_lockpath);
			}
		}
	}
	
	if (t_files != NULL)
	{
		for (uint32_t i = 0; i < t_file_count; i++)
			MCCStringFree(t_files[i]);
		MCMemoryDeleteArray(t_files);
	}
}
#endif

////////////////////////////////////////////////////////////////////////////////

#define TEMP_PREFIX "livecode_"
bool MCS_create_temporary_file(MCStringRef p_path, MCStringRef p_prefix, IO_handle &r_file, MCStringRef &r_name);

typedef struct _mcmultiparttempfilelist
{
	char *file_name;
	
	struct _mcmultiparttempfilelist *next;
} MCMultiPartTempFileList;

static MCMultiPartTempFileList *s_temp_files = NULL;

bool MCMultiPartCreateTempFile(MCStringRef p_temp_folder, IO_handle &r_file_handle, MCStringRef &r_temp_name)
{
	bool t_success = true;
	MCAutoStringRef t_temp_name_string;
	
	IO_handle t_file_handle = NULL;
	
	MCMultiPartTempFileList *t_list_item = NULL;
	
	if (t_success)
		t_success = MCMemoryNew(t_list_item);
	
	if (t_success)
	{
		t_success = MCS_create_temporary_file(p_temp_folder, MCSTR(TEMP_PREFIX), t_file_handle, &t_temp_name_string);
	}

	if (t_success)
	{
		t_list_item->file_name = strdup(MCStringGetCString(*t_temp_name_string));
		t_list_item->next = s_temp_files;
		s_temp_files = t_list_item;
		
		r_file_handle = t_file_handle;
		r_temp_name = MCValueRetain(*t_temp_name_string);
	}
	else
	{
		MCMemoryDelete (t_list_item);
		MCS_close(t_file_handle);
	}
	
	return t_success;
}

void MCMultiPartRemoveTempFiles()
{
	while (s_temp_files != NULL)
	{
		MCAutoStringRef file_name_string;
		/* UNCHECKED */ MCStringCreateWithCString(s_temp_files->file_name, &file_name_string);

		MCS_unlink(*file_name_string);
		MCCStringFree(s_temp_files->file_name);

		MCMultiPartTempFileList *t_item;
		t_item = s_temp_files;
		s_temp_files = s_temp_files->next;
		MCMemoryDelete(t_item);
	}
}

////////////////////////////////////////////////////////////////////////////////

bool MCMultiPartGetErrorMessage(MCMultiPartFileStatus p_status, MCStringRef &r_message)
{
	switch (p_status)
	{
    case kMCFileStatusOK:
        r_message = MCValueRetain(kMCEmptyString);
        return true;
    case kMCFileStatusStopped:
        return MCStringCreateWithCString("upload stopped", r_message);
    case kMCFileStatusFailed:
        return MCStringCreateWithCString("upload failed", r_message);
    case kMCFileStatusNoUploadFolder:
        return MCStringCreateWithCString("no upload folder", r_message);
    case kMCFileStatusIOError:
        return MCStringCreateWithCString("i/o error", r_message);
	}
	
    return false;
}
