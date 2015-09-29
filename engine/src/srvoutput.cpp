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

#include "core.h"
#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"
#include "mcio.h"

#include "execpt.h"
#include "util.h"

#include "unicode.h"
#include "srvmain.h"

////////////////////////////////////////////////////////////////////////////////

#define kMCServerOutputBufferSize 64

// Do EOL conversion on the given char, placing the result in the output
// buffer.
static inline void MCServerOutputLineEnding(char *p_output, uint32_t& x_output_count)
{
	switch(MCserveroutputlineendings)
	{
		case kMCSOutputLineEndingsLF:
			p_output[x_output_count++] = 10;
			break;
		case kMCSOutputLineEndingsCR:
			p_output[x_output_count++] = 13;
			break;
		case kMCSOutputLineEndingsCRLF:
			p_output[x_output_count++] = 13;
			p_output[x_output_count++] = 10;
			break;
		default:
			break;
	}
}

// Output a native char in UTF-8 encoding to the given output buffer.
static inline void MCServerOutputNativeCharAsUTF8(uint8_t p_native_char, char *p_output, uint32_t& x_output_count)
{
	uint32_t t_codepoint;
	t_codepoint = MCUnicodeMapFromNative(p_native_char);

	if (t_codepoint < 0x80)
		p_output[x_output_count++] = t_codepoint;
	else if (t_codepoint < 0x0800)
	{
		p_output[x_output_count++] = 0xc0 | (t_codepoint >> 6);
		p_output[x_output_count++] = 0x80 | (t_codepoint & 0x3f);
	}
}

// Output a unicode character in UTF-8 encoding to the given output buffer.
static inline void MCServerOutputUnicodeCharAsUTF8(uint32_t p_codepoint, char *p_output, uint32_t& x_output_count)
{
	if (p_codepoint < 0x80)
		p_output[x_output_count++] = p_codepoint;
	else if (p_codepoint < 0x0800)
	{
		p_output[x_output_count++] = 0xc0 | (p_codepoint >> 6);
		p_output[x_output_count++] = 0x80 | (p_codepoint & 0x3f);
	}
	else if (p_codepoint < 0x010000)
	{
		p_output[x_output_count++] = 0xe0 | (p_codepoint >> 12);
		p_output[x_output_count++] = 0x80 | ((p_codepoint >> 6) & 0x3f);
		p_output[x_output_count++] = 0x80 | (p_codepoint & 0x3f);
	}
	else
	{
		p_output[x_output_count++] = 0xf0 | (p_codepoint >> 18);
		p_output[x_output_count++] = 0x80 | ((p_codepoint >> 12) & 0x3f);
		p_output[x_output_count++] = 0x80 | ((p_codepoint >> 6) & 0x3f);
		p_output[x_output_count++] = 0x80 | (p_codepoint & 0x3f);
	}
}

// Attempt to map a full unicode cluster to a native char, returning
// false if this can't be done.
static inline bool MCServerOutputMapUnicodeCluster(const unichar_t *p_chars, uint32_t p_char_count, uint8_t& r_char)
{
	switch(MCserveroutputtextencoding)
	{
		case kMCSOutputTextEncodingWindows1252:
			if (MCUnicodeMapToNative_Windows1252((const uint2 *)p_chars, p_char_count, r_char))
				return true;
			break;
			
		case kMCSOutputTextEncodingMacRoman:
			if (MCUnicodeMapToNative_MacRoman((const uint2 *)p_chars, p_char_count, r_char))
				return true;
			break;
			
		case kMCSOutputTextEncodingISO8859_1:
			if (!MCUnicodeMapToNative_ISO8859_1((const uint2 *)p_chars, p_char_count, r_char))
				return true;
			break;
		default:
			break;
	}
	
	return false;
}

// Output a (decimal) markup character entity.
static void MCServerOutputCharEntity(uint32_t p_codepoint, char *p_output, uint32_t& x_output_count)
{
	char t_entity[16];
	sprintf(t_entity, "&#%u;", p_codepoint);
	memcpy(p_output + x_output_count, t_entity, strlen(t_entity));
	x_output_count += strlen(t_entity);
}

// Output a markup character, producing entities for special characters
// if 'is_content' is true.
static void MCServerOutputMarkupChar(char p_char, bool p_is_content, char *p_output, uint32_t& x_output_count)
{
	if (!p_is_content)
	{
		if (p_char != 10)
			p_output[x_output_count++] = p_char;
		else
			MCServerOutputLineEnding(p_output, x_output_count);
		
		return;
	}
	
	switch(p_char)
	{
		case 10:
			MCServerOutputLineEnding(p_output, x_output_count);
			break;
		case '&':
			p_output[x_output_count++] = '&';
			p_output[x_output_count++] = 'a';
			p_output[x_output_count++] = 'm';
			p_output[x_output_count++] = 'p';
			p_output[x_output_count++] = ';';
			break;
		case '<':
			p_output[x_output_count++] = '&';
			p_output[x_output_count++] = 'l';
			p_output[x_output_count++] = 't';
			p_output[x_output_count++] = ';';
			break;
		case '>':
			p_output[x_output_count++] = '&';
			p_output[x_output_count++] = 'g';
			p_output[x_output_count++] = 't';
			p_output[x_output_count++] = ';';
			break;
		case '"':
			p_output[x_output_count++] = '&';
			p_output[x_output_count++] = 'q';
			p_output[x_output_count++] = 'u';
			p_output[x_output_count++] = 'o';
			p_output[x_output_count++] = 't';
			p_output[x_output_count++] = ';';
			break;
		default:
			p_output[x_output_count++] = p_char;
			break;
	}
}

// Take the chars in native encoding and map through to the output encoding,
// performing any end of line conversion as it goes.
static void MCServerOutputNativeChars(const char *p_chars, uint32_t p_char_count)
{
	// We ensure there is one char of 'slop' in the output buffer. This
	// is because LF can map to CR LF, and UTF-8 needs 2 bytes for high bit
	// chars.
	char t_output[kMCServerOutputBufferSize + 1];
	uint32_t t_output_count;
	t_output_count = 0;

	while(p_char_count > 0)
	{
		uint8_t t_char;
		t_char = (uint8_t)*p_chars;
		p_chars += 1;
		p_char_count -= 1;
		
		if (t_char < 128 || MCserveroutputtextencoding == kMCSOutputTextEncodingNative)
		{
			if (t_char != 10)
				t_output[t_output_count++] = t_char;
			else
				MCServerOutputLineEnding(t_output, t_output_count);
		}
		else if (MCserveroutputtextencoding == kMCSOutputTextEncodingUTF8)
			MCServerOutputNativeCharAsUTF8(t_char, t_output, t_output_count);
		else
		{
			unichar_t t_codepoint;
			t_codepoint = MCUnicodeMapFromNative(t_char);
			if (!MCServerOutputMapUnicodeCluster(&t_codepoint, 1, t_char))
				t_char = '?';
			
			t_output[t_output_count++] = t_char;
		}
		
		if (t_output_count >= kMCServerOutputBufferSize)
		{
			MCS_write(t_output, 1, t_output_count, IO_stdout);
			t_output_count = 0;
		}
	}
	
	MCS_write(t_output, 1, t_output_count, IO_stdout);
}

// Take the chars in native encoding and map through to the output encoding,
// mapping undefined characters to entities, and treating some characters
// specially if 'p_is_content' is true.
static void MCServerOutputNativeMarkup(const char *p_chars, uint32_t p_char_count, bool p_is_content)
{
	// Our buffer has a certain amout of 'slop' to account for one-to-many char
	// mappings. LF can map to CR LF, UTF-8 requires 2 chars for chars 128-255 and
	// char entities can be up to 10 chars (&#xxxxxxx;)
	char t_output[kMCServerOutputBufferSize + 9];
	uint32_t t_output_count;
	t_output_count = 0;
	while(p_char_count > 0)
	{
		uint8_t t_char;
		t_char = (uint8_t)*p_chars;
		p_chars += 1;
		p_char_count -= 1;
		
		if (t_char < 128 || MCserveroutputtextencoding == kMCSOutputTextEncodingNative)
			MCServerOutputMarkupChar(t_char, p_is_content, t_output, t_output_count);
		else if (MCserveroutputtextencoding == kMCSOutputTextEncodingUTF8)
			MCServerOutputNativeCharAsUTF8(t_char, t_output, t_output_count);
		else
		{
			unichar_t t_codepoint;
			t_codepoint = MCUnicodeMapFromNative(t_char);
			if (MCServerOutputMapUnicodeCluster(&t_codepoint, 1, t_char))
				t_output[t_output_count++] = t_char;
			else
				MCServerOutputCharEntity(t_codepoint, t_output, t_output_count);
		}
		
		if (t_output_count >= kMCServerOutputBufferSize)
		{
			MCS_write(t_output, 1, t_output_count, IO_stdout);
			t_output_count = 0;
		}
	}
	
	MCS_write(t_output, 1, t_output_count, IO_stdout);
	
}

static void MCServerOutputAdvanceUnicodeCluster(const unichar_t *p_chars, uint32_t p_char_count, uint32_t& x_index)
{
	uint32_t t_codepoint;
	t_codepoint = MCUnicodeCodepointAdvance((const uint2 *)p_chars, p_char_count, x_index);
	
	while(x_index < p_char_count)
	{
		uint4 t_old_index;
		t_old_index = x_index;
		
		t_codepoint = MCUnicodeCodepointAdvance((const uint2 *)p_chars, p_char_count, x_index);
		
		if (MCUnicodeCodepointIsBase(t_codepoint))
		{
			x_index = t_old_index;
			break;
		}
	}
}

static void MCServerOutputUnicodeChars(const unichar_t *p_chars, uint32_t p_char_count)
{
	// We ensure there is one char of 'slop' in the output buffer. This
	// is because LF can map to CR LF, and UTF-8 needs up to 4 bytes for
	// any single input character.
	char t_output[kMCServerOutputBufferSize + 3];
	uint32_t t_output_count;
	t_output_count = 0;
	
	uint32_t t_index;
	t_index = 0;
	while(t_index < p_char_count)
	{
		if (p_chars[t_index] == 10 ||
			p_chars[t_index] < 128 && (t_index == p_char_count - 1 || p_chars[t_index + 1] < 128))
		{
			if (p_chars[t_index] != 10)
				t_output[t_output_count++] = (char)p_chars[t_index];
			else
				MCServerOutputLineEnding(t_output, t_output_count);
			
			t_index += 1;
		}
		else if (MCserveroutputtextencoding == kMCSOutputTextEncodingUTF8)
		{
			uint32_t t_codepoint;
			t_codepoint = MCUnicodeCodepointAdvance((const uint2 *)p_chars, p_char_count, t_index);
			MCServerOutputUnicodeCharAsUTF8(t_codepoint, t_output, t_output_count);
		}
		else
		{
			uint32_t t_start;
			t_start = t_index;
			
			MCServerOutputAdvanceUnicodeCluster(p_chars, p_char_count, t_index);

			uint8_t t_char;
			if (!MCServerOutputMapUnicodeCluster(p_chars + t_start, t_index - t_start, t_char))
				t_char = '?';
			
			t_output[t_output_count++] = t_char;
		}
		
		if (t_output_count >= kMCServerOutputBufferSize)
		{
			MCS_write(t_output, 1, t_output_count, IO_stdout);
			t_output_count = 0;
		}
	}

	MCS_write(t_output, 1, t_output_count, IO_stdout);
}

static void MCServerOutputUnicodeMarkup(const unichar_t *p_chars, uint32_t p_char_count, bool p_is_content)
{
	// Our buffer has a certain amout of 'slop' to account for one-to-many char
	// mappings. LF can map to CR LF, UTF-8 requires up to 4 chars and char
	// entities can be up to 10 chars (&#xxxxxxx;)
	char t_output[kMCServerOutputBufferSize + 9];
	uint32_t t_output_count;
	t_output_count = 0;
	
	uint32_t t_index;
	t_index = 0;
	while(t_index < p_char_count)
	{
		if (p_chars[t_index] == 10 ||
			(p_is_content && (p_chars[t_index] == '&' || p_chars[t_index] == '<' || p_chars[t_index] == '>' || p_chars[t_index] == '"')) ||
			p_chars[t_index] < 128 && (t_index == p_char_count - 1 || p_chars[t_index + 1] < 128))
		{
			MCServerOutputMarkupChar((char)p_chars[t_index], p_is_content, t_output, t_output_count);
			t_index += 1;
		}
		else if (MCserveroutputtextencoding == kMCSOutputTextEncodingUTF8)
		{
			uint32_t t_codepoint;
			t_codepoint = MCUnicodeCodepointAdvance((const uint2 *)p_chars, p_char_count, t_index);
			MCServerOutputUnicodeCharAsUTF8(t_codepoint, t_output, t_output_count);
		}
		else
		{
			uint32_t t_start;
			t_start = t_index;
			
			MCServerOutputAdvanceUnicodeCluster(p_chars, p_char_count, t_index);
			
			uint8_t t_char;
			if (MCServerOutputMapUnicodeCluster(p_chars + t_start, t_index - t_start, t_char))
				t_output[t_output_count++] = t_char;
			else
			{
				for(uint32_t i = t_start; i < t_index; i++)
					MCServerOutputCharEntity(p_chars[i], t_output, t_output_count);
			}
		}
		
		if (t_output_count >= kMCServerOutputBufferSize)
		{
			MCS_write(t_output, 1, t_output_count, IO_stdout);
			t_output_count = 0;
		}
	}
	
	MCS_write(t_output, 1, t_output_count, IO_stdout);
}

////////////////////////////////////////////////////////////////////////////////

void MCServerPutBinaryOutput(const MCString& s)
{
	// Binary data so output verbatim.
	MCS_write(s . getstring(), 1, s . getlength(), IO_stdout);
}

// TODO: This should throw an error if output encoding is binary.
void MCServerPutUnicodeOutput(const MCString& s)
{
	// UTF-16 encoded data, so convert to output encoding.
	MCServerOutputUnicodeChars((unichar_t *)s . getstring(), s . getlength() / 2);
}

void MCServerPutOutput(const MCString& s)
{	
	if (MCserveroutputtextencoding == kMCSOutputTextEncodingNative && MCserveroutputlineendings == kMCSOutputLineEndingsLF)
	{
		MCS_write(s . getstring(), 1, s . getlength(), IO_stdout);
		return;
	}
	
	MCServerOutputNativeChars(s . getstring(), s . getlength());
}

void MCServerPutHeader(const MCString& s, bool p_new)
{
	// Find where the ':' is
	const char *t_loc;
	t_loc = s . getstring();
	uint4 t_loc_len;
	t_loc_len = s . getlength();
	if (!MCU_strchr(t_loc, t_loc_len, ':', False))
		return;
	
	// Loop through to see if we are replacing one (we replace the last)
	uint32_t i;
	if (p_new)
		i = 0;
	else
		for(i = MCservercgiheadercount; i > 0; i--)
		{
			if (MCU_strncasecmp(s . getstring(), MCservercgiheaders[i - 1], t_loc - s . getstring()) == 0)
				break;
		}
	
	if (i == 0)
	{
		MCservercgiheaders = (char **)realloc(MCservercgiheaders, sizeof(char *) * (MCservercgiheadercount + 1));
		MCservercgiheadercount += 1;
		i = MCservercgiheadercount;
	}
	else
		free(MCservercgiheaders[i - 1]);
	
	MCCStringCloneSubstring(s . getstring(), s . getlength(), MCservercgiheaders[i - 1]);
}

void MCServerPutContent(const MCString& s)
{
	MCServerOutputNativeMarkup(s . getstring(), s . getlength(), true);
}

void MCServerPutUnicodeContent(const MCString& s)
{
	MCServerOutputUnicodeMarkup((const unichar_t *)s . getstring(), s . getlength() / 2, true);
}

void MCServerPutMarkup(const MCString& s)
{
	MCServerOutputNativeMarkup(s . getstring(), s . getlength(), false);
}

void MCServerPutUnicodeMarkup(const MCString& s)
{
	MCServerOutputUnicodeMarkup((const unichar_t *)s . getstring(), s . getlength() / 2, false);
}

////////////////////////////////////////////////////////////////////////////////
bool MCSafeCStringEqual(const char *a, const char *b)
{
	if (a == NULL && b == NULL)
		return true;
	else if (a == NULL || b == NULL)
		return false;
	else
		return MCCStringEqual(a, b);
}

bool MCServerSetCookie(const MCString &p_name, const MCString &p_value, uint32_t p_expires, const MCString &p_path, const MCString &p_domain, bool p_secure, bool p_http_only)
{
	bool t_success = true;
	char *t_name = NULL;
	char *t_value = NULL;
	char *t_path = NULL;
	char *t_domain = NULL;
	
	uint32_t t_index = 0;
	
	if (t_success && p_name.getlength() != 0)
		t_success = MCCStringCloneSubstring(p_name.getstring(), p_name.getlength(), t_name);
		
	if (t_success && p_value.getlength() != 0)
	{
		// urlencode cookie value
		MCExecPoint ep;
		ep.setsvalue(p_value);
		MCU_urlencode(ep);
		MCString t_encoded = ep.getsvalue();
		t_success = MCCStringCloneSubstring(t_encoded.getstring(), t_encoded.getlength(), t_value);
	}	
	if (t_success && p_path.getlength() != 0)
		t_success = MCCStringCloneSubstring(p_path.getstring(), p_path.getlength(), t_path);

	if (t_success && p_domain.getlength() != 0)
		t_success = MCCStringCloneSubstring(p_domain.getstring(), p_domain.getlength(), t_domain);
	
	if (t_success)
	{
		// try to find matching cookie (name, path, domain)
		for (; t_index < MCservercgicookiecount; t_index++)
		{
			if (MCSafeCStringEqual(t_name, MCservercgicookies[t_index].name) &&
				MCSafeCStringEqual(t_path, MCservercgicookies[t_index].path) &&
				MCSafeCStringEqual(t_domain, MCservercgicookies[t_index].domain))
				break;
		}
		if (t_index == MCservercgicookiecount)
			t_success = MCMemoryResizeArray(MCservercgicookiecount + 1, MCservercgicookies, MCservercgicookiecount);
	}
	
	if (t_success)
	{
		MCservercgicookies[t_index].name = t_name;
		MCservercgicookies[t_index].value = t_value;
		MCservercgicookies[t_index].path = t_path;
		MCservercgicookies[t_index].domain = t_domain;

		MCservercgicookies[t_index].expires = p_expires;
		MCservercgicookies[t_index].secure = p_secure;
		MCservercgicookies[t_index].http_only = p_http_only;
	}
	else
	{
		MCCStringFree(t_name);
		MCCStringFree(t_value);
		MCCStringFree(t_path);
		MCCStringFree(t_domain);
	}
	
	return t_success;
}

////////////////////////////////////////////////////////////////////////////////

