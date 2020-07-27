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

#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"
#include "mcio.h"

#include "globals.h"
#include "util.h"
#include "md5.h"
#include "sha1.h"

#include "exec.h"

#include "foundation-filters.h"

////////////////////////////////////////////////////////////////////////////////

bool MCFiltersIsoToMac(MCDataRef p_source, MCDataRef &r_result)
{
	MCAutoByteArray t_buffer;
	const char_t *t_srcptr;
	uindex_t t_srclen;
	t_srcptr = MCDataGetBytePtr(p_source);
	t_srclen = MCDataGetLength(p_source);

	if (!t_buffer.New(t_srclen))
		return false;

	uint1 *sptr = (uint1 *)t_buffer.Bytes();
	uindex_t len = t_srclen;
	while (len--)
		*sptr++ = MCisotranslations[*t_srcptr++];

	return t_buffer.CreateDataAndRelease(r_result);
}

bool MCFiltersMacToIso(MCDataRef p_source, MCDataRef &r_result)
{
	MCAutoByteArray t_buffer;
	const char_t *t_srcptr;
	uindex_t t_srclen;
	t_srcptr = MCDataGetBytePtr(p_source);
	t_srclen = MCDataGetLength(p_source);

	if (!t_buffer.New(t_srclen))
		return false;

	uint1 *sptr = (uint1 *)t_buffer.Bytes();
	uindex_t len = t_srclen;
	while (len--)
		*sptr++ = MCmactranslations[*t_srcptr++];

	return t_buffer.CreateDataAndRelease(r_result);
}

////////////////////////////////////////////////////////////////////////////////

static const char * const url_table[256] =
    {
        "%00", "%01", "%02", "%03", "%04", "%05", "%06", "%07", "%08", "%09",
        "%0D%0A", "%0B", "%0C", "%0D", "%0E", "%0F", "%10", "%11", "%12",
        "%13", "%14", "%15", "%16", "%17", "%18", "%19", "%1A", "%1B", "%1C",
        "%1D", "%1E", "%1F", "+", "%21", "%22", "%23", "%24", "%25", "%26",
        "%27", "%28", "%29", "*", "%2B", "%2C", "-", ".", "%2F", "0", "1",
        "2", "3", "4", "5", "6", "7", "8", "9", "%3A", "%3B", "%3C", "%3D",
        "%3E", "%3F", "%40", "A", "B", "C", "D", "E", "F", "G", "H", "I", "J",
        "K", "L", "M", "N", "O", "P", "Q", "R", "S", "T", "U", "V", "W", "X",
        "Y", "Z", "%5B", "%5C", "%5D", "%5E", "_", "%60", "a", "b", "c", "d",
        "e", "f", "g", "h", "i", "j", "k", "l", "m", "n", "o", "p", "q", "r",
        "s", "t", "u", "v", "w", "x", "y", "z", "%7B", "%7C", "%7D", "%7E",
        "%7F", "%80", "%81", "%82", "%83", "%84", "%85", "%86", "%87", "%88",
        "%89", "%8A", "%8B", "%8C", "%8D", "%8E", "%8F", "%90", "%91", "%92",
        "%93", "%94", "%95", "%96", "%97", "%98", "%99", "%9A", "%9B", "%9C",
        "%9D", "%9E", "%9F", "%A0", "%A1", "%A2", "%A3", "%A4", "%A5", "%A6",
        "%A7", "%A8", "%A9", "%AA", "%AB", "%AC", "%AD", "%AE", "%AF", "%B0",
        "%B1", "%B2", "%B3", "%B4", "%B5", "%B6", "%B7", "%B8", "%B9", "%BA",
        "%BB", "%BC", "%BD", "%BE", "%BF", "%C0", "%C1", "%C2", "%C3", "%C4",
        "%C5", "%C6", "%C7", "%C8", "%C9", "%CA", "%CB", "%CC", "%CD", "%CE",
        "%CF", "%D0", "%D1", "%D2", "%D3", "%D4", "%D5", "%D6", "%D7", "%D8",
        "%D9", "%DA", "%DB", "%DC", "%DD", "%DE", "%DF", "%E0", "%E1", "%E2",
        "%E3", "%E4", "%E5", "%E6", "%E7", "%E8", "%E9", "%EA", "%EB", "%EC",
        "%ED", "%EE", "%EF", "%F0", "%F1", "%F2", "%F3", "%F4", "%F5", "%F6",
        "%F7", "%F8", "%F9", "%FA", "%FB", "%FC", "%FD", "%FE", "%FF"
    };

bool MCFiltersUrlEncode(MCStringRef p_source, bool p_use_utf8, MCStringRef& r_result)
{
    char *t_chars;
    uint4 l;
    int4 size;

    // SN-2014-11-13: [[ Bug 14015 ]] If specified, we don't want to nativise the string,
    // but rather to encode it in UTF-8 and write the bytes (a '%' will be added).
    if (p_use_utf8)
    {
        if (!MCStringConvertToUTF8(p_source, t_chars, l))
            return false;

        size = l + 1;
    }
    else
    {
        if (!MCStringConvertToNative(p_source, (char_t*&)t_chars, l))
            return false;
        size = strlen(t_chars);
    }

    // AL-2015-02-13: [[ Bug 14602 ]] Use copy of t_chars ptr for iteration so it can be freed properly.
    const char *t_chars_iter = t_chars;
    size = l + 1;
    size += size / 4;

    bool t_success;
    t_success = true;
    
	MCAutoNativeCharArray buffer;
    t_success = buffer . New(size);

    if (t_success)
    {
        char_t *dptr;
        dptr = buffer . Chars();
        while (l--)
        {
            if (dptr - buffer . Chars() + 7 > size)
            {
                uint4 newsize = size + size / 4 + 7;
                uint4 offset = dptr - buffer . Chars();
                if (!buffer . Extend(newsize))
                {
                    t_success = false;
                    break;
                }
                
                dptr = buffer . Chars() + offset;
                size = newsize;
            }
            const char_t *sptr = (const char_t *)url_table[(uint1)*t_chars_iter++];
            do
            {
                *dptr++ = *sptr++;
            }
            while (*sptr);
        }
        buffer . Shrink(dptr - buffer . Chars());
    }

    MCMemoryDeleteArray(t_chars);
    if (!t_success)
        return false;
    
	return buffer . CreateStringAndRelease(r_result);
}

bool MCFiltersUrlDecode(MCStringRef p_source, bool p_use_utf8, MCStringRef& r_result)
{
    // SN-2014-11-13: [[ Bug 14015 ]] If specified, we don't want to use a nativised string, but
    // bytes, as we can get UTF-8 characters (now usable in 7.0)
    MCAutoByteArray t_buffer;
    char_t *t_srcptr;
    uindex_t t_srclen;

    if (!MCStringConvertToNative(p_source, t_srcptr, t_srclen))
        return false;

    if (!t_buffer . New(t_srclen))
    {
        MCMemoryDeleteArray(t_srcptr);
        return false;
    }

	const uint1 *sptr = (uint1 *)t_srcptr;
    const uint1 *eptr = sptr + t_srclen;
    uint1 *dptr = (uint1*)t_buffer . Bytes();
	while (sptr < eptr)
	{
		if (*sptr == '%')
		{
            uint1 source = MCS_toupper(*++sptr);
			uint1 value = 0;
			if (isdigit(source))
				value = (source - '0') << 4;
			else
				if (source >= 'A' && source <= 'F')
					value = (source - 'A' + 10) << 4;
			source = MCS_toupper(*++sptr);
			if (isdigit(source))
				value += source - '0';
			else
				if (source >= 'A' && source <= 'F')
					value += source - 'A' + 10;
			if (value != 13)
				*dptr++ = value;
		}
		else
			if (*sptr == '+')
				*dptr++ = ' ';
			else
				if (*sptr == '\r')
				{
					if (*(sptr + 1) == '\n')
						sptr++;
					*dptr++ = '\n';
				}
				else
					*dptr++ = *sptr;
		sptr++;
	}
    t_buffer.Shrink(dptr - t_buffer.Bytes());

    MCMemoryDeleteArray(t_srcptr);

    // SN-2014-11-13: [[ Bug 14015 ]] The string might be explicitely UTF-8 encoded.
    if (p_use_utf8)
        return MCStringCreateWithBytes(t_buffer . Bytes(), t_buffer.ByteCount(), kMCStringEncodingUTF8, false, r_result);
    else
        return MCStringCreateWithBytes(t_buffer . Bytes(), t_buffer.ByteCount(), kMCStringEncodingNative, false, r_result);
}

////////////////////////////////////////////////////////////////////////////////

void MCFiltersEvalCompress(MCExecContext& ctxt, MCDataRef p_source, MCDataRef& r_result)
{
	if (!MCFiltersCompress(p_source, r_result))
		ctxt.LegacyThrow(EE_COMPRESS_ERROR);
}

void MCFiltersEvalDecompress(MCExecContext& ctxt, MCDataRef p_source, MCDataRef& r_result)
{
	if (!MCFiltersIsCompressed(p_source))
		ctxt.LegacyThrow(EE_DECOMPRESS_NOTCOMPRESSED);
	else if (!MCFiltersDecompress(p_source, r_result))
		ctxt.LegacyThrow(EE_DECOMPRESS_ERROR);
}

//////////

void MCFiltersEvalBase64Decode(MCExecContext& ctxt, MCStringRef p_source, MCDataRef& r_result)
{
	if (!MCFiltersBase64Decode(p_source, r_result))
		ctxt.Throw();
}

void MCFiltersEvalBase64Encode(MCExecContext& ctxt, MCDataRef p_source, MCStringRef& r_result)
{
	if (!MCFiltersBase64Encode(p_source, r_result))
		ctxt.Throw();
}

//////////

void MCFiltersEvalIsoToMac(MCExecContext& ctxt, MCDataRef p_source, MCDataRef& r_result)
{
	if (!MCFiltersIsoToMac(p_source, r_result))
		ctxt.Throw();
}

void MCFiltersEvalMacToIso(MCExecContext& ctxt, MCDataRef p_source, MCDataRef& r_result)
{
	if (!MCFiltersMacToIso(p_source, r_result))
		ctxt.Throw();
}

//////////

void MCFiltersEvalUrlEncode(MCExecContext& ctxt, MCStringRef p_source, MCStringRef& r_result)
{
    // SN-2014-12-02: [[ Bug 14015 ]] Need a further checking for allowing the UTF-8 encoding for URLs
    //  at the script level
	if (!MCFiltersUrlEncode(p_source, false, r_result))
		ctxt.Throw();
}

void MCFiltersEvalUrlDecode(MCExecContext& ctxt, MCStringRef p_source, MCStringRef& r_result)
{
    // SN-2014-12-02: [[ Bug 14015 ]] Need a further checking for allowing the UTF-8 encoding for URLs
    //  at the script level
	if (!MCFiltersUrlDecode(p_source, false, r_result))
		ctxt.Throw();
}

////////////////////////////////////////////////////////////////////////////////

#define BINARY_NOCOUNT (UINDEX_MAX - 1)
#define BINARY_ALL (UINDEX_MAX)

// From exec-strings.cpp
extern bool MCStringsEvalTextEncoding(MCStringRef p_encoding, MCStringEncoding &r_encoding);

static bool MCU_gettemplate(MCStringRef format, uindex_t &x_offset, unichar_t &cmd, uindex_t &count, MCStringEncoding &r_encoding)
{
	cmd = MCStringGetCharAtIndex(format, x_offset++);    
    
    // Check the encoding specified by the user
    if (MCStringGetCharAtIndex(format, x_offset) == '{')
    {
        uindex_t t_brace_end;
        MCAutoStringRef t_encoding_string;
        if (!MCStringFirstIndexOfChar(format, '}', x_offset, kMCStringOptionCompareExact, t_brace_end))
            // No ending curly brace: error
            return false;
        
        /* UNCHECKED */ MCStringCopySubstring(format, MCRangeMakeMinMax(x_offset, t_brace_end), &t_encoding_string);
        
        x_offset = t_brace_end + 1;
        
        if (!MCStringsEvalTextEncoding(*t_encoding_string, r_encoding))
            // Encoding not recognised
            return false;    
    }
	if (isdigit(MCStringGetCharAtIndex(format, x_offset)))
    {
        MCAutoNumberRef t_number;
        uindex_t t_number_size;
        t_number_size = 1;

        while (isdigit(MCStringGetCharAtIndex(format, x_offset + t_number_size)))
            t_number_size++;

        if (MCNumberParseOffset(format, x_offset, t_number_size, &t_number))
        {        
            count = MCNumberFetchAsUnsignedInteger(*t_number);
            x_offset += t_number_size;
        }
        else
            count = 0;
    }
	else
    {
		if (MCStringGetCharAtIndex(format, x_offset) == '*')
		{
			count = BINARY_ALL;
			x_offset++;
		}
		else
			count = 1;
    }
    
    return true;
}

void MCFiltersEvalBinaryDecode(MCExecContext& ctxt, MCStringRef p_format, MCDataRef p_data, MCValueRef *r_results, uindex_t p_result_count, integer_t& r_done)
{
	if (p_data == nil)
	{
		ctxt . LegacyThrow(EE_BINARYD_BADPARAM);
		return;
	}

    uindex_t t_format_length;
    uindex_t t_format_index;
    t_format_length = MCStringGetLength(p_format);
    t_format_index = 0;

	const byte_t *t_data_ptr = MCDataGetBytePtr(p_data);
	uindex_t length = MCDataGetLength(p_data);

	uindex_t t_index = 0;
	uindex_t offset = 0;
	uindex_t done = 0;

	bool t_success = true;

	while (t_success && t_format_index < t_format_length && offset < length)
	{
		unichar_t cmd;
		uindex_t count;
        MCStringEncoding t_encoding = kMCStringEncodingNative;

		if (!MCU_gettemplate(p_format, t_format_index, cmd, count, t_encoding))
        {
            // Invalid format specified
            ctxt . Throw();
            return;
        }
            
		if (count == 0 && cmd != '@')
			continue;

		if (cmd != 'x' && t_index >= p_result_count)
		{
			ctxt.LegacyThrow(EE_BINARYD_BADDEST);
			return;
		}

		switch (cmd)
		{
		case 'a':
		case 'A':
			{
				if (count == BINARY_ALL)
					count = length - offset;
				else
					if (count == BINARY_NOCOUNT)
						count = 1;
				if (count > length - offset)
					break;
				const byte_t *src = t_data_ptr + offset;
				uint4 size = count;
				if (cmd == 'A')
				{
					while (size > 0)
					{
						if (src[size - 1] != '\0' && src[size - 1] != ' ')
							break;
						size--;
					}
				}

				t_success = MCStringCreateWithBytes(t_data_ptr + offset, size, kMCStringEncodingNative, false, (MCStringRef&)r_results[t_index]);

				done++;
				offset += count;
				break;
			}
		case 'b':
		case 'B':
			{
				if (count == BINARY_ALL)
					count = (length - offset) * 8;
				else
					if (count == BINARY_NOCOUNT)
						count = 1;
				if (count > (length - offset) * 8)
					break;
				uint1 *src = (uint1 *)t_data_ptr + offset;
				uint1 value = 0;

				MCAutoNativeCharArray t_buffer;
				t_success = t_buffer.New(count);
				if (!t_success)
					break;

				char_t *dest = t_buffer.Chars();
				if (cmd == 'b')
				{
					for (uindex_t i = 0 ; i < count ; i++)
					{
						if (i % 8)
							value >>= 1;
						else
							value = *src++;
						*dest++ = (value & 1) ? '1' : '0';
					}
				}
				else
				{
					for (uindex_t i = 0 ; i < count ; i++)
					{
						if (i % 8)
							value <<= 1;
						else
							value = *src++;
						*dest++ = (value & 0x80) ? '1' : '0';
					}
				}
				t_success = t_buffer.CreateStringAndRelease((MCStringRef&)r_results[t_index]);
				
				done++;
				offset += (count + 7 ) / 8;
				break;
			}
		case 'h':
		case 'H':
			{
				if (count == BINARY_ALL)
					count = (length - offset) * 2;
				else
					if (count == BINARY_NOCOUNT)
						count = 1;
				if (count > (length - offset) * 2)
					break;
				uint1 *src = (uint1 *)t_data_ptr + offset;
				uint1 value = 0;
				MCAutoNativeCharArray t_buffer;
				t_success = t_buffer.New(count);
				if (!t_success)
					break;

				char_t *dest;
				dest = t_buffer.Chars();
				static char hexdigit[] = "0123456789abcdef";
				if (cmd == 'h')
					for (uindex_t i = 0 ; i < count ; i++)
					{
						if (i % 2)
							value >>= 4;
						else
							value = *src++;
						*dest++ = hexdigit[value & 0xf];
					}
				else
					for (uindex_t i = 0 ; i < count ; i++)
					{
						if (i % 2)
							value <<= 4;
						else
							value = *src++;
						*dest++ = hexdigit[(value >> 4) & 0xf];
					}
				
				t_success = t_buffer.CreateStringAndRelease((MCStringRef&)r_results[t_index]);

				done++;
				offset += (count + 1) / 2;
				break;
			}
		case 'c':
		case 'C':
		case 's':
		case 'S':
		case 'i':
		case 'I':
		case 'm':
		case 'M':
		case 'n':
		case 'N':
		case 'f':
		case 'd':
			if (count == BINARY_ALL || count == BINARY_NOCOUNT)
				count = 1;
			while (t_success && count--)
			{
				uindex_t oldoffset = offset;
				real64_t t_number;
				switch (cmd)
				{
				case 'c':
					if (offset + (int4)sizeof(int1) <= length)
					{
						int1 c;
						memcpy(&c, t_data_ptr + offset, sizeof(int1));
						t_number = c;
						offset += sizeof(int1);
					}
					break;
				case 'C':
					if (offset + (int4)sizeof(uint1) <= length)
					{
						uint1 c;
						memcpy(&c, t_data_ptr + offset, sizeof(uint1));
						t_number = c;
						offset += sizeof(uint1);
					}
					break;
				case 's':
					if (offset + (int4)sizeof(int2) <= length)
					{
						int2 c;
						memcpy(&c, t_data_ptr + offset, sizeof(int2));
						t_number = c;
						offset += sizeof(int2);
					}
					break;
				case 'S':
					if (offset + (int4)sizeof(uint2) <= length)
					{
						uint2 c;
						memcpy(&c, t_data_ptr + offset, sizeof(uint2));
						t_number = c;
						offset += sizeof(uint2);
					}
					break;
				case 'i':
					if (offset + (int4)sizeof(int4) <= length)
					{
						int4 c;
						memcpy(&c, t_data_ptr + offset, sizeof(int4));
						t_number = c;
						offset += sizeof(int4);
					}
					break;
				case 'I':
					if (offset + (int4)sizeof(uint4) <= length)
					{
						uint4 c;
						memcpy(&c, t_data_ptr + offset, sizeof(uint4));
						t_number = c;
						offset += sizeof(uint4);
					}
					break;
				case 'm':
					if (offset + (int4)sizeof(uint2) <= length)
					{
						uint2 c;
						memcpy(&c, t_data_ptr + offset, sizeof(uint2));
						t_number = MCSwapInt16HostToNetwork(c);
						offset += sizeof(int2);
					}
					break;
				case 'M':
					if (offset + (int4)sizeof(uint4) <= length)
					{
						uint4 c;
						memcpy(&c, t_data_ptr + offset, sizeof(uint4));
						t_number = MCSwapInt32HostToNetwork(c);
						offset += sizeof(uint4);
					}
					break;

				// MW-2007-09-11: [[ Bug 5315 ]] Make sure we coerce to signed integers
				//   before we convert to doubles - failing to do this results in getting
				//   unsigned results on little endian machines.
				case 'n':
					if (offset + (int4)sizeof(int2) <= length)
					{
						int2 c;
						memcpy(&c, t_data_ptr + offset, sizeof(int2));
						t_number = (int2)MCSwapInt16HostToNetwork(c);
						offset += sizeof(int2);
					}
					break;
				case 'N':
					if (offset + (int4)sizeof(int4) <= length)
					{
						int4 c;
						memcpy(&c, t_data_ptr + offset, sizeof(int4));
						t_number = (int4)MCSwapInt32HostToNetwork(c);
						offset += sizeof(int4);
					}
					break;

				case 'f':
					if (offset + (int4)sizeof(float) <= length)
					{
						float f;
						memcpy(&f, t_data_ptr + offset, sizeof(float));
						t_number = f;
						offset += sizeof(float);
					}
					break;
				case 'd':
					if (offset + (int4)sizeof(double) <= length)
					{
						double d;
						memcpy(&d, t_data_ptr + offset, sizeof(double));
						t_number = d;
						offset += sizeof(double);
					}
					break;
				}
				if (offset == oldoffset)
				{
					offset = length;
					break;
				}
				t_success = MCNumberCreateWithReal(t_number, (MCNumberRef&)r_results[t_index]);
				if (!t_success)
					break;

				done++;
				if (count)
				{
					t_index++;
					if (t_index >= p_result_count)
					{
						ctxt.LegacyThrow(EE_BINARYD_BADPARAM);
						return;
					}
				}
			}
			break;
        case 'u':
        case 'U':
            {
                
                if (count == BINARY_ALL)
                    count = length - offset;
                
                uindex_t t_size = count;
                
                if (cmd == 'U')
                {
                    // We need to skip all the spaces
                    MCAutoDataRef t_encoded_space;
                    MCStringEncode(MCSTR(" "), t_encoding, false, &t_encoded_space);
                    
                    const byte_t* t_space_ptr = MCDataGetBytePtr(*t_encoded_space);
                    uindex_t t_space_length = MCDataGetLength(*t_encoded_space);
                    
                    bool t_space_skipped = false;
                    uindex_t t_temp_size = t_size;
                    
                    while (!t_space_skipped)
                    {
                        // stop looking for spaces when an encoded space char won't fit within the remaining data
                        if (t_space_length > t_temp_size)
                        {
                            // No char remaining
                            t_space_skipped = true;
                        }
                        else
                        {
                            bool t_is_space = true;
                            
                            // Compare the encoded spaces
                            for (uindex_t i = 0; i < t_space_length && t_is_space; ++i)
                                t_is_space = t_data_ptr[offset + t_temp_size - 1 - (t_space_length - 1) + i] == t_space_ptr[i];
                            
                            if (t_is_space)
                            {
                                t_size = t_temp_size - t_space_length;
                                t_temp_size = t_size;
                            }
                            else
                            {
                                // The byte might be offset, we need to try to offset at most the size of the encoded space
                                if (t_size - t_temp_size == t_space_length)
                                    t_space_skipped = true;
                                else
                                    --t_temp_size;
                            }
                            
                        }
                    }
                }
                
                MCStringCreateWithBytes(t_data_ptr + offset, t_size, t_encoding, false, (MCStringRef&)r_results[t_index]);
                                
                done++;
                offset += count;
                break;
            }
		case 'x':
			if (count == BINARY_NOCOUNT)
				count = 1;
			if (count == BINARY_ALL || (count > (length - offset)))
				offset = length;
			else
				offset += count;
                
            r_results[t_index] = nil;
            done++;
            break;
		default:
			ctxt.LegacyThrow(EE_BINARYD_BADFORMAT);
			return;
		}
		t_index++;
	}

	if (t_success)
	{
		r_done = done;
		return;
	}

	ctxt.Throw();
}

bool append_to_array(MCAutoByteArray& p_chars, const void* p_bytes, size_t p_byte_count)
{
	size_t t_size = p_chars.ByteCount();
	if (!p_chars.Extend(t_size + p_byte_count))
		return false;

	MCMemoryCopy(p_chars.Bytes() + t_size, p_bytes, p_byte_count);
	return true;
}

bool pad_array(MCAutoByteArray& p_chars, char_t p_pad, size_t p_count)
{
	size_t t_size = p_chars.ByteCount();
	if (!p_chars.Extend(t_size + p_count))
		return false;

	for (uinteger_t i = 0; i < p_count; i++)
		p_chars.Bytes()[t_size + i] = p_pad;

	return true;
}

void MCFiltersEvalBinaryEncode(MCExecContext& ctxt, MCStringRef p_format, MCValueRef *p_params, uindex_t p_param_count, MCDataRef& r_string)
{
	MCAutoByteArray t_buffer;
	uindex_t t_index = 0;
    
    uindex_t t_format_index;
    uindex_t t_format_length;
    t_format_length = MCStringGetLength(p_format);
    t_format_index = 0;

	bool t_success = true;

	while (t_success && t_format_index < t_format_length)
	{
		unichar_t cmd;
		uindex_t count;
        MCStringEncoding t_encoding;
        
		if (!MCU_gettemplate(p_format, t_format_index, cmd, count, t_encoding))
        {
            ctxt . Throw();
            return;
        }
        
		if (count == 0 && cmd != '@')
		{
			t_index++;
			continue;
		}
		if (t_index >= p_param_count)
		{
			ctxt.LegacyThrow(EE_BINARYE_BADPARAM);
			return;
		}

		MCValueRef t_value = p_params[t_index++];

		switch (cmd)
		{
		case 'a':
		case 'A':
		case 'b':
		case 'B':
		case 'h':
		case 'H':
		case 'x':
			{
				MCAutoStringRef t_string;
				t_success = ctxt.ForceToString(t_value, &t_string);
				if (!t_success)
					break;
                                
                MCAutoStringRefAsNativeChars t_auto_native;
                const char_t* t_native;
                uindex_t t_length;
                t_success = t_auto_native . Lock(*t_string, t_native, t_length);
								if (!t_success)
									break;
									
				switch (cmd)
				{
				case 'a':
				case 'A':
					{
						char_t pad = cmd == 'a' ? '\0' : ' ';
						if (count == BINARY_ALL)
                            count = t_length;
						else
							if (count == BINARY_NOCOUNT)
								count = 1;
                        if (t_length >= count)
                            t_success = append_to_array(t_buffer, t_native, count);
						else
						{
                            t_success = append_to_array(t_buffer, t_native, t_length) &&
                                pad_array(t_buffer, pad, count - t_length);
						}
						break;
					}
				case 'b':
				case 'B':
					{
						if (count == BINARY_ALL)
                            count = t_length;
						else
							if (count == BINARY_NOCOUNT)
								count = 1;
						size_t t_char_count = t_buffer.ByteCount();
						size_t t_output_count = (count + 7) / 8;
						t_success = t_buffer.Extend(t_char_count + t_output_count);
						if (!t_success)
							break;
						char_t *cursor = t_buffer.Bytes() + t_char_count;
						char_t *buffer_end = cursor + t_output_count;
						uint1 value = 0;
                        if (count > t_length)
                            count = t_length;
						uindex_t offset;
						bool t_bigendian = cmd == 'B';
						for (offset = 0 ; offset < count ; offset++)
						{
							value = t_bigendian ? (value << 1) : (value >> 1);
                            if (t_native[offset] == '1')
								value |= t_bigendian ? 1 : 0x80;
							else
                                if (t_native[offset] != '0')
								{
                                    ctxt.LegacyThrow(EE_BINARYE_BADFORMAT, t_value);
                                    return;
								}
							if ((offset + 1) % 8 == 0)
							{
								*cursor++ = value;
								value = 0;
							}
						}
						if ((offset % 8) != 0)
						{
							if (cmd == 'B')
								value <<= 8 - (offset % 8);
							else
								value >>= 8 - (offset % 8);
							*cursor++ = value;
						}
						while (cursor < buffer_end)
							*cursor++ = '\0';
						break;
					}
				case 'h':
				case 'H':
					{
						int c;
						if (count == BINARY_ALL)
                            count = t_length;
						else
							if (count == BINARY_NOCOUNT)
								count = 1;
						size_t t_char_count = t_buffer.ByteCount();
						size_t t_output_count = (count + 1) / 2;
						t_success = t_buffer.Extend(t_char_count + t_output_count);
						if (!t_success)
							break;
						char_t *cursor = t_buffer.Bytes() + t_char_count;
						char_t *buffer_end = cursor + t_output_count;
						uint1 value = 0;
                        if (count > t_length)
                            count = t_length;
						uindex_t offset;
						bool t_bigendian = cmd == 'H';
						for (offset = 0 ; offset < count ; offset++)
						{
							value = t_bigendian ? (value << 4) : (value >> 4);
                            if (!isxdigit(t_native[offset]))
							{
                                ctxt.LegacyThrow(EE_BINARYE_BADFORMAT, t_value);
								return;
							}
                            c = t_native[offset] - '0';
							if (c > 9)
								c += ('0' - 'A') + 10;
							if (c > 16)
								c += ('A' - 'a');
							value |= t_bigendian ? (c & 0xf) : ((c << 4) & 0xf0);
							if (offset % 2)
							{
								*cursor++ = value;
								value = 0;
							}
						}
						if (offset % 2)
						{
							if (cmd == 'H')
								value <<= 4;
							else
								value >>= 4;
							*cursor++ = (unsigned char) value;
						}
						while (cursor < buffer_end)
							*cursor++ = '\0';
						break;
					}
				case 'x':
					{
						if (count == BINARY_ALL)
                            count = t_length;
						else
							if (count == BINARY_NOCOUNT)
								count = 1;
						t_success = pad_array(t_buffer, '\0', count);
					}
                }
			}
			break;
		case 'c':
		case 'C':
		case 's':
		case 'S':
		case 'i':
		case 'I':
		case 'm':
		case 'M':
		case 'n':
		case 'N':
		case 'f':
		case 'd':
			{
				if (count == BINARY_ALL || count == BINARY_NOCOUNT)
					count = 1;
				while (count--)
				{
					real64_t t_number;
					if (!ctxt.ConvertToReal(t_value, t_number))
					{
						ctxt.LegacyThrow(EE_BINARYE_BADFORMAT, t_value);
						return;
					}
					switch (cmd)
					{
					case 'c':
						{
							int1 c = (int1)t_number;
							t_success = append_to_array(t_buffer, &c, sizeof(int1));
							break;
						}
					case 'C':
						{
							uint1 c = (uint1)t_number;
							t_success = append_to_array(t_buffer, &c, sizeof(uint1));
							break;
						}
					case 's':
						{
							int2 c = (int2)t_number;
							t_success = append_to_array(t_buffer, &c, sizeof(int2));
							break;
						}
					case 'S':
						{
							uint2 c = (uint2)t_number;
							t_success = append_to_array(t_buffer, &c, sizeof(uint2));
							break;
						}
					case 'i':
						{
							int4 c = (int4)t_number;
							t_success = append_to_array(t_buffer, &c, sizeof(int4));
							break;
						}
					case 'I':
						{
							uint4 c = (uint4)t_number;
							t_success = append_to_array(t_buffer, &c, sizeof(uint4));
							break;
						}
					case 'm':
						{
							int2 c = MCSwapInt16HostToNetwork((uint2)t_number);
							t_success = append_to_array(t_buffer, &c, sizeof(int2));
							break;
						}
					case 'M':
						{
							uint4 c = MCSwapInt32HostToNetwork((uint4)t_number);
							t_success = append_to_array(t_buffer, &c, sizeof(uint4));
							break;
						}
					case 'n':
						{
							int2 c = MCSwapInt16HostToNetwork((int2)t_number);
							t_success = append_to_array(t_buffer, &c, sizeof(int2));
							break;
						}
					case 'N':
						{
							int4 c = MCSwapInt32HostToNetwork((int4)t_number);
							t_success = append_to_array(t_buffer, &c, sizeof(int4));
							break;
						}
					case 'f':
						{
							float f = (float)t_number;
							t_success = append_to_array(t_buffer, &f, sizeof(float));
							break;
						}
					case 'd':
						{
							double d = t_number;
							t_success = append_to_array(t_buffer, &d, sizeof(double));
							break;
						}
					}
					if (count)
					{
						if (t_index >= p_param_count)
						{
							ctxt.LegacyThrow(EE_BINARYE_BADPARAM);
							return;
						}
						t_value = p_params[t_index++];
					}
				}
				break;
			}
        case 'u':
        case 'U':
            {
                MCAutoStringRef t_param;
                
                ctxt . ForceToString(t_value, &t_param);
                
                if (count == BINARY_ALL)
                {
                    // Get all the bytes of the encoded string
                    MCAutoDataRef t_encoded_string;
                    MCStringEncode(*t_param, t_encoding, false, &t_encoded_string);
                    
                    append_to_array(t_buffer, MCDataGetBytePtr(*t_encoded_string), MCDataGetLength(*t_encoded_string));
                }
                else
                {
                    uindex_t t_byte_pos = 0;
                    uindex_t t_cu_pos = 0;
                    uindex_t t_char_pos = 0;
                    
                    bool t_char_offsets = false;
                    
                    // Loop until we have reached either the output byte-amount specified by the user
                    // or the length of the string given
                    while (t_byte_pos < count && t_cu_pos < MCStringGetLength(*t_param)
                            && !t_char_offsets)
                    {
                        MCStringRef t_char_substring;
                        MCAutoDataRef t_encoded_char;
                        MCRange t_cu_range;
                        MCStringMapIndices(*t_param, kMCCharChunkTypeGrapheme, MCRangeMake(t_char_pos, 1), t_cu_range);
                        
                        MCStringCopySubstring(*t_param, t_cu_range, t_char_substring);
                        
                        MCStringEncodeAndRelease(t_char_substring, t_encoding, false, &t_encoded_char);
                        
                        // Tests whether the encoded char fits in the amount given
                        if (t_byte_pos + MCDataGetLength(*t_encoded_char) <= count)
                        {
                            append_to_array(t_buffer, MCDataGetBytePtr(*t_encoded_char), MCDataGetLength(*t_encoded_char));
                            t_byte_pos += MCDataGetLength(*t_encoded_char);
                            t_cu_pos +=  t_cu_range . length;
                            ++t_char_pos;
                        }
                        else
                            t_char_offsets = true;
                            
                    }
                    
                    // Check whether padding is needed
                    if (t_byte_pos != count)
                    {
                        if (cmd == 'U')
                        {
                            // We pad with as many spaces as possible
                            MCAutoDataRef t_encoded_space;
                            MCStringEncode(MCSTR(" "), t_encoding, false, &t_encoded_space);
                            
                            const byte_t *t_space_bytes = MCDataGetBytePtr(*t_encoded_space);
                            uindex_t t_space_length = MCDataGetLength(*t_encoded_space);
                            
                            while (t_byte_pos + t_space_length <= count)
                            {
                                append_to_array(t_buffer, t_space_bytes, t_space_length);
                                t_byte_pos += t_space_length;
                            }
                            
                            // In case the last space would have overfitted the byte amount
                            if (t_byte_pos != count)
                                t_buffer . Extend(t_buffer . ByteCount() + count);
                        }
                        else
                            // Pad the remaining bytes needed with NULL
                            t_buffer.Extend(t_buffer . ByteCount() + count);
                    }
                }
                break;
            }
		default:
			ctxt.LegacyThrow(EE_BINARYE_BADFORMAT);
			return;
		}
	}

	if (t_success && t_buffer.CreateDataAndRelease(r_string))
		return;

	ctxt.Throw();
}

////////////////////////////////////////////////////////////////////////////////

void MCFiltersEvalUniDecodeToEncoding(MCExecContext& ctxt, MCDataRef p_src, MCNameRef p_lang, MCDataRef& r_dest)
{
	uinteger_t t_dest_charset = MCU_languagetocharset(MCNameGetString(p_lang));

	if (MCU_unicodetomultibyte(p_src, t_dest_charset, r_dest))
		return;

	ctxt.Throw();
}

void MCFiltersEvalUniDecodeToNative(MCExecContext& ctxt, MCDataRef p_input, MCStringRef &r_output)
{
	if (MCStringDecode(p_input, kMCStringEncodingUTF16, false, r_output))
		return;
	
	ctxt.Throw();
}

void MCFiltersEvalUniEncodeFromEncoding(MCExecContext& ctxt, MCDataRef p_src, MCNameRef p_lang, MCDataRef& r_dest)
{
	uinteger_t t_dest_charset = MCU_languagetocharset(MCNameGetString(p_lang));

	if (MCU_multibytetounicode(p_src, t_dest_charset, r_dest))
		return;

	ctxt.Throw();
}

void MCFiltersEvalUniEncodeFromNative(MCExecContext& ctxt, MCStringRef p_input, MCDataRef &r_output)
{
	if (MCStringEncode(p_input, kMCStringEncodingUTF16, false, r_output))
		return;
	
	ctxt.Throw();
}

////////////////////////////////////////////////////////////////////////////////
