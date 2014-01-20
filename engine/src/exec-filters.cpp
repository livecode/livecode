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

#include "globals.h"
#include "util.h"
#include "md5.h"
#include "sha1.h"

#include "exec.h"

////////////////////////////////////////////////////////////////////////////////

MC_EXEC_DEFINE_EVAL_METHOD(Filters, Base64Encode, 2)
MC_EXEC_DEFINE_EVAL_METHOD(Filters, Base64Decode, 2)
MC_EXEC_DEFINE_EVAL_METHOD(Filters, BinaryEncode, 3)
MC_EXEC_DEFINE_EVAL_METHOD(Filters, BinaryDecode, 4)
MC_EXEC_DEFINE_EVAL_METHOD(Filters, Compress, 2)
MC_EXEC_DEFINE_EVAL_METHOD(Filters, Decompress, 2)
MC_EXEC_DEFINE_EVAL_METHOD(Filters, IsoToMac, 2)
MC_EXEC_DEFINE_EVAL_METHOD(Filters, MacToIso, 2)
MC_EXEC_DEFINE_EVAL_METHOD(Filters, UrlEncode, 2)
MC_EXEC_DEFINE_EVAL_METHOD(Filters, UrlDecode, 2)
MC_EXEC_DEFINE_EVAL_METHOD(Filters, UniEncode, 3)
MC_EXEC_DEFINE_EVAL_METHOD(Filters, UniDecode, 3)
MC_EXEC_DEFINE_EVAL_METHOD(Filters, MD5Digest, 2)
MC_EXEC_DEFINE_EVAL_METHOD(Filters, SHA1Digest, 2)

////////////////////////////////////////////////////////////////////////////////

inline uint1 isvalid(uint1 x)
{
	return ((x >= 'A' && x <= 'Z') || (x >= 'a' && x <= 'z') ||
	        (x >= '0' && x <= '9') || x == '+' || x == '/');
}

inline uint1 val(uint1 x)
{
	return (x >= 'A' && x <= 'Z' ? x - 'A' :
	        (x >= 'a' && x <= 'z' ? x - 'a' + 26 :
	         (x >= '0' && x <= '9' ? x - '0' + 52 :
	          (x == '+' ? 62 : (x == '/' ? 63 : 0)))));
}

bool MCFiltersBase64Decode(MCStringRef p_src, MCDataRef& r_dst)
{
	MCAutoByteArray buffer;

    uint32_t l;
    MCAutoStringRefAsNativeChars t_native;
    char_t *s = nil;
    byte_t *p = nil;

    if (!t_native . Lock(p_src, s, l))
        return false;

	if (!buffer . New(l))
		return false;

	p = buffer . Bytes();

	uint1 c[5];
	c[4] = '\0';

	while (l)
	{
		uint2 i = 0;
		int2 pad = -1;
		uint4 d;

		while (i < 4)
		{
			if (l--)
				c[i] = *s++;
			else
				c[i] = '=';
			if (c[i] && isvalid(c[i]))
			{
				i++;
				continue;
			}
			if (c[i] && c[i] != '=')
				continue;
			while ((!c[i] || c[i] == '=') && i < 4)
			{
				pad++;
				c[i++] = 0;
				c[i] = '=';
			}
		}

		d = (val(c[0]) << 18) | (val(c[1]) << 12) | (val(c[2]) << 6) | val(c[3]);

		if (pad < 2)
			*p++ = (d & 0xff0000) >> 16;
		if (pad < 1)
			*p++ = (d & 0xff00) >> 8;
		if (pad < 0)
			*p++ = d & 0xff;

		if (c[4] == '=')
		{
			*p = 0;
			break;
		}
	}


	buffer . Shrink(p - buffer . Bytes());

	return buffer . CreateDataAndRelease(r_dst);
}

//////////

inline uint1 lav(uint1 x)
{
	return (x <= 25 ? x + 'A' : (x >= 26 && x <= 51 ? x + 'a' - 26 :
	                             (x >= 52 && x <= 61 ? x + '0' - 52 :
	                              (x == 62 ? '+' : (x == 63 ? '/' : '?')))));
}

bool MCFiltersBase64Encode(MCDataRef p_src, MCStringRef& r_dst)
{
	MCAutoNativeCharArray buffer;
	uint32_t size;

	const char_t *s = nil;
	char_t *p = nil;

	size = MCDataGetLength(p_src);
	s = (char_t *)MCDataGetBytePtr(p_src);

	uint32_t newsize = size * 4 / 3 + size / 54 + 5;
	if (!buffer.New(newsize))
		return false;

	p = buffer.Chars();
	char_t *linestart = p;

	while (size)
	{
		uint1 c[3];
		uint1 d;
		int2 i = 0;
		int2 pad = -1;

		while (i < 3)
		{
			if (size)
			{
				c[i++] = *s++;
				size--;
			}
			else
			{
				c[i++] = 0;
				pad++;
			}
		}

		d = c[0] >> 2;
		*p++ = lav(d);

		if (pad < 2)
		{
			d = ((c[0] & 0x3) << 4) | (c[1] >> 4);
			*p++ = lav(d);
		}
		else
			*p++ = '=';

		if (pad < 1)
		{
			d = ((c[1] & 0xf) << 2) | (c[2] >> 6);
			*p++ = lav(d);
		}
		else
			*p++ = '=';

		if (pad < 0)
		{
			d = c[2] & 0x3f;
			*p++ = lav(d);
		}
		else
			*p++ = '=';
		if (p - linestart == 72)
		{
			*p++ = '\n';
			linestart = p;
		}
	}

	buffer.Shrink(p - buffer.Chars());
	return buffer.CreateStringAndRelease(r_dst);
}

////////////////////////////////////////////////////////////////////////////////

#include "zlib.h"

#define GZIP_HEAD_CRC     0x02 /* bit 1 set: header CRC present */
#define GZIP_EXTRA_FIELD  0x04 /* bit 2 set: extra field present */
#define GZIP_ORIG_NAME    0x08 /* bit 3 set: original file name present */
#define GZIP_COMMENT      0x10 /* bit 4 set: file comment present */
#define GZIP_RESERVED     0xE0
#define GZIP_HEADER_SIZE 10
static char_t gzip_header[GZIP_HEADER_SIZE] = { (char_t)0x1f, (char_t)0x8b,
        Z_DEFLATED, 0, 0, 0, 0, 0, 0, 3 };

bool MCFiltersCompress(MCDataRef p_source, MCDataRef& r_result)
{
	const byte_t *t_src_ptr = MCDataGetBytePtr(p_source);
	uindex_t t_src_len = MCDataGetLength(p_source);

	uint4 size = t_src_len + 12 + GZIP_HEADER_SIZE + 8;
	size += size / 999;  //dest must be "0.1% larger than (source) plus 12 bytes"
	MCAutoByteArray t_buffer;
	if (!t_buffer.New(size))
		return false;

	memcpy(t_buffer.Bytes(), gzip_header, GZIP_HEADER_SIZE);
	z_stream zstrm;
	memset((char *)&zstrm, 0, sizeof(z_stream));
	zstrm.next_in = (unsigned char *)t_src_ptr;
	zstrm.avail_in = t_src_len;
	zstrm.next_out = (unsigned char *)t_buffer.Bytes() + GZIP_HEADER_SIZE;
	zstrm.avail_out = size - GZIP_HEADER_SIZE - 8;
	if (deflateInit2(&zstrm, Z_DEFAULT_COMPRESSION, Z_DEFLATED, -MAX_WBITS, 8, 0) != Z_OK
	        || deflate(&zstrm, Z_FINISH) != Z_STREAM_END
	        || deflateEnd(&zstrm) != Z_OK)
	{
		return false;
	}
	uint4 osize = zstrm.total_out + GZIP_HEADER_SIZE;
	uint4 check = crc32(0L, Z_NULL, 0);
	check = crc32(check, (unsigned char *)t_src_ptr, t_src_len);
	MCswapbytes = !MCswapbytes;
	swap_uint4(&check);
	memcpy(t_buffer.Bytes() + osize, &check, 4);
	check = t_src_len;
	swap_uint4(&check);
	memcpy(t_buffer.Bytes() + osize + 4, &check, 4);
	MCswapbytes = !MCswapbytes;

	t_buffer.Shrink(osize + 8);
	return t_buffer.CreateDataAndRelease(r_result);
}

bool MCFiltersIsCompressed(MCDataRef p_source)
{
	const char_t *sptr = MCDataGetBytePtr(p_source);
	return (MCDataGetLength(p_source) >= 10) &&
		(sptr[0] == gzip_header[0]) &&
		(sptr[1] == gzip_header[1]) &&
		(sptr[2] == gzip_header[2]) &&
		((sptr[3] & GZIP_RESERVED) == 0);
}

bool MCFiltersDecompress(MCDataRef p_source, MCDataRef& r_result)
{
	const char_t *t_src_ptr = MCDataGetBytePtr(p_source);
	uindex_t t_src_len = MCDataGetLength(p_source);

	const char_t *sptr = t_src_ptr;

	MCswapbytes = !MCswapbytes;
	uint4 startindex = 10;
	if (sptr[3] & GZIP_EXTRA_FIELD)
	{ /* skip the extra field */
		uint2 len;
		memcpy(&len, &sptr[startindex], 2);
		swap_uint2(&len);
		startindex += len;
	}
	if (sptr[3] & GZIP_ORIG_NAME) /* skip the original file name */
		while (startindex < t_src_len && sptr[startindex++])
			;
	if (sptr[3] & GZIP_COMMENT)   /* skip the .gz file comment */
		while (startindex < t_src_len && sptr[startindex++])
			;
	if (sptr[3] & GZIP_HEAD_CRC) /* skip the header crc */
		startindex += 2;
	uint4 size;
	memcpy(&size, &sptr[t_src_len - 4], 4);
	swap_uint4(&size);
	MCswapbytes = !MCswapbytes;
	if (size == 0)
	{
		r_result = MCValueRetain(kMCEmptyData);
		return true;
	}
	if (t_src_len < (startindex + 8))
	{
		return false;
	}
	MCAutoByteArray t_buffer;
	if (!t_buffer.New(size))
	{
		// SMR 1935 no need to abort rest of script...
		MCabortscript = False;
		return false;
	}

	z_stream zstrm;
	memset((char *)&zstrm, 0, sizeof(z_stream));
	zstrm.next_in = (unsigned char *)sptr + startindex;
	zstrm.avail_in = t_src_len - startindex - 8;
	zstrm.next_out = (unsigned char *)t_buffer.Bytes();
	zstrm.avail_out = size;
	int err;
	if (inflateInit2(&zstrm, -MAX_WBITS) != Z_OK
	        || (err = inflate(&zstrm, Z_FINISH)) != Z_STREAM_END
	        && err != Z_OK && err != Z_BUF_ERROR // bug on OS X returns this error
	        || inflateEnd(&zstrm) != Z_OK)
		return false;

	return t_buffer.CreateDataAndRelease(r_result);
}

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

static const char *url_table[256] =
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

bool MCFiltersUrlEncode(MCStringRef p_source, MCStringRef& r_result)
{
    MCAutoStringRefAsNativeChars t_native;
    char_t *s;
    uint4 l;
    int4 size;

    if (!t_native . Lock(p_source, s, l))
        return false;

    size = l + 1;
    size += size / 4;

	MCAutoNativeCharArray buffer;
	if (!buffer . New(size))
		return false;

	char_t *dptr = buffer . Chars();
	while (l--)
	{
		if (dptr - buffer . Chars() + 7 > size)
		{
			uint4 newsize = size + size / 4 + 7;
			uint4 offset = dptr - buffer . Chars();
			if (!buffer . Extend(newsize))
				return false;
			dptr = buffer . Chars() + offset;
			size = newsize;
		}
		const char_t *sptr = (const char_t *)url_table[(uint1)*s++];
		do
		{
			*dptr++ = *sptr++;
		}
		while (*sptr);
	}
	
	buffer . Shrink(dptr - buffer . Chars());

	return buffer . CreateStringAndRelease(r_result);
}

bool MCFiltersUrlDecode(MCStringRef p_source, MCStringRef& r_result)
{
    MCAutoStringRefAsNativeChars t_native;
    MCAutoNativeCharArray t_buffer;
    char_t *t_srcptr;
	uindex_t t_srclen;

    if (!t_native . Lock(p_source, t_srcptr, t_srclen))
        return false;

	if (!t_buffer.New(t_srclen))
		return false;

	const uint1 *sptr = (uint1 *)t_srcptr;
	const uint1 *eptr = sptr + t_srclen;
	uint1 *dptr = (uint1*)t_buffer.Chars();
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
	t_buffer.Shrink(dptr - t_buffer.Chars());
	return t_buffer.CreateStringAndRelease(r_result);
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
	if (!MCFiltersUrlEncode(p_source, r_result))
		ctxt.Throw();
}

void MCFiltersEvalUrlDecode(MCExecContext& ctxt, MCStringRef p_source, MCStringRef& r_result)
{
	if (!MCFiltersUrlDecode(p_source, r_result))
		ctxt.Throw();
}

////////////////////////////////////////////////////////////////////////////////

#define BINARY_NOCOUNT -2
#define BINARY_ALL -1
static void MCU_gettemplate(const char_t *&format, char_t &cmd, uindex_t &count)
{
	cmd = *format++;
	if (isdigit(*format))
		count = strtoul((char*)format, (char **)&format, 10);
	else
    {
		if (*format == '*')
		{
			count = BINARY_ALL;
			format++;
		}
		else
			count = 1;
    }
}

static void MCU_gettemplate(MCStringRef format, uindex_t &p_offset, unichar_t &cmd, uindex_t &count)
{
	cmd = MCStringGetCharAtIndex(format, p_offset++);
	if (isdigit(MCStringGetCharAtIndex(format, p_offset)))
    {
        MCAutoNumberRef t_number;
        uindex_t t_number_size;
        t_number_size = 1;

        while (isdigit(MCStringGetCharAtIndex(format, p_offset + t_number_size)))
            t_number_size++;

        if (MCNumberParseUnicodeChars(MCStringGetCharPtr(format) + p_offset, t_number_size, &t_number))
        {        
            count = MCNumberFetchAsUnsignedInteger(*t_number);
            p_offset += t_number_size;
        }
        else
            count = 0;
    }
	else
    {
		if (MCStringGetCharAtIndex(format, p_offset) == '*')
		{
			count = BINARY_ALL;
			p_offset++;
		}
		else
			count = 1;
    }
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

		MCU_gettemplate(p_format, t_format_index, cmd, count);
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
				int4 oldoffset = offset;
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
		case 'x':
			if (count == BINARY_NOCOUNT)
				count = 1;
			if (count == BINARY_ALL || (count > (length - offset)))
				offset = length;
			else
				offset += count;
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
        
		MCU_gettemplate(p_format, t_format_index, cmd, count);
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
                char_t* t_native;
                uindex_t t_length;
                /* UNCHECKED */ t_auto_native . Lock(*t_string, t_native, t_length);

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

bool MCFiltersMD5Digest(MCDataRef p_src, MCDataRef& r_digest)
{
	md5_state_t state;
	md5_byte_t digest[16];
	md5_init(&state);
	md5_append(&state, MCDataGetBytePtr(p_src), MCDataGetLength(p_src));
	md5_finish(&state, digest);

	return MCDataCreateWithBytes(digest, 16, r_digest);
}

bool MCFiltersSHA1Digest(MCDataRef p_src, MCDataRef& r_digest)
{
	sha1_state_t state;
	uint8_t digest[20];
	sha1_init(&state);
	sha1_append(&state, MCDataGetBytePtr(p_src), MCDataGetLength(p_src));
	sha1_finish(&state, digest);

	return MCDataCreateWithBytes(digest, 20, r_digest);
}

void MCFiltersEvalMD5Digest(MCExecContext& ctxt, MCDataRef p_src, MCDataRef& r_digest)
{
	if (MCFiltersMD5Digest(p_src, r_digest))
		return;

	ctxt.Throw();
}

void MCFiltersEvalSHA1Digest(MCExecContext& ctxt, MCDataRef p_src, MCDataRef& r_digest)
{
	if (MCFiltersSHA1Digest(p_src, r_digest))
		return;

	ctxt.Throw();
}

////////////////////////////////////////////////////////////////////////////////

