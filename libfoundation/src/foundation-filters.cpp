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

#include <foundation.h>

#include "foundation-auto.h"
#include "foundation-private.h"

////////////////////////////////////////////////////////////////////////////////

inline uint8_t isvalid(uint8_t x)
{
	return ((x >= 'A' && x <= 'Z') || (x >= 'a' && x <= 'z') ||
	        (x >= '0' && x <= '9') || x == '+' || x == '/');
}

inline uint8_t val(uint8_t x)
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
    
	uint8_t c[5];
	c[4] = '\0';
    
	while (l)
	{
		uint16_t i = 0;
		int16_t pad = -1;
		uint32_t d;
        
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

inline uint8_t lav(uint8_t x)
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
		uint8_t c[3];
		uint8_t d;
		int16_t i = 0;
		int16_t pad = -1;
        
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
    char *t_utf8_string;
    char *s;
    uint32_t l;
    int32_t size;
    
    // SN-2014-11-13: [[ Bug 14015 ]] We don't want to nativise the string,
    // but rather to encode it in UTF-8 and write the bytes (a '%' will be added).
    if (!MCStringConvertToUTF8(p_source, t_utf8_string, l))
        return false;
    
    s = t_utf8_string;
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
            uint32_t newsize = size + size / 4 + 7;
            uint32_t offset = dptr - buffer . Chars();
            if (!buffer . Extend(newsize))
                return false;
            dptr = buffer . Chars() + offset;
            size = newsize;
        }
        const char_t *sptr = (const char_t *)url_table[(uint8_t)*s++];
        do
        {
            *dptr++ = *sptr++;
        }
        while (*sptr);
    }
    
    buffer . Shrink(dptr - buffer . Chars());
    
    MCMemoryDeleteArray(t_utf8_string);
    return buffer . CreateStringAndRelease(r_result);
}

bool MCFiltersUrlDecode(MCStringRef p_source, MCStringRef& r_result)
{
    // SN-2014-11-13: [[ Bug 14015 ]] We don't want to use a nativised string, but
    // bytes, as we can get UTF-8 characters (now usable in 7.0)
    MCAutoByteArray t_buffer;
    char_t *t_srcptr;
    uindex_t t_srclen;
    
    if (!MCStringConvertToNative(p_source, t_srcptr, t_srclen))
        return false;
    
    if (!t_buffer . New(t_srclen))
        return false;
    
    const uint8_t *sptr = (uint8_t *)t_srcptr;
    const uint8_t *eptr = sptr + t_srclen;
    uint8_t *dptr = (uint8_t*)t_buffer . Bytes();
    while (sptr < eptr)
    {
        if (*sptr == '%')
        {
            uint8_t source = MCNativeCharUppercase(*++sptr);
            uint8_t value = 0;
            if (isdigit(source))
                value = (source - '0') << 4;
            else
                if (source >= 'A' && source <= 'F')
                    value = (source - 'A' + 10) << 4;
            source = MCNativeCharUppercase(*++sptr);
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
    
    // SN-2014-11-13: [[ Bug 14015 ]] The string is UTF-8 encoded, not native.
    return MCStringCreateWithBytes(t_buffer . Bytes(), t_buffer.ByteCount(), kMCStringEncodingUTF8, false, r_result);
}

////////////////////////////////////////////////////////////////////////////////

