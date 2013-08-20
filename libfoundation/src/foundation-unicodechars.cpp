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

#include "foundation-private.h"

////////////////////////////////////////////////////////////////////////////////

bool MCUnicodeCharsMapToNative(const unichar_t *p_uchars, uindex_t p_uchar_count, char_t *p_nchars, uindex_t& r_nchar_count, char_t p_invalid)
{
	bool t_lossy;
	t_lossy = false;

	r_nchar_count = 0;

	for(uindex_t i = 0; i < p_uchar_count; i++)
	{
		uint8_t t_native_char;
		if (!MCUnicodeCharMapToNative(p_uchars[i], t_native_char))
		{
			t_native_char = p_invalid;
			t_lossy = true;	
		}
		p_nchars[r_nchar_count++] = t_native_char;
	}

	return !t_lossy;
}

void MCUnicodeCharsMapFromNative(const char_t *p_chars, uindex_t p_char_count, unichar_t *p_uchars)
{
	for(uindex_t i = 0; i < p_char_count; i++)
		p_uchars[i] = MCUnicodeCharMapFromNative(p_chars[i]);
}

////////////////////////////////////////////////////////////////////////////////

// If utf8bytes is nil, returns the number of bytes needed to convert the chars
// If utf8bytes is not nil, does the conversion
uindex_t MCUnicodeCharsMapToUTF8(const unichar_t *wchars, uindex_t wchar_count, byte_t *utf8bytes)
{
    uindex_t t_bytes_needed;
    t_bytes_needed = 0;
    
    uindex_t t_size_in_bytes;
    
    for (uindex_t i = 0; i < wchar_count; ++i)
    {
        // According to the first byte value, we can determine the number of
        // bytes a conversion would need
        if (wchars[i] < 0x80)
            t_size_in_bytes = 1;
        
        else if (wchars[i] < 0xE0)
            t_size_in_bytes = 2;
        
        else if (wchars[i] < 0xF0)
            t_size_in_bytes = 3;
        
        else
            t_size_in_bytes = 4;
        
        // Performs the conversion from the Unicode character to the byte_t array
        if (utf8bytes != nil)
            MCMemoryCopy(utf8bytes + t_bytes_needed, wchars + i, t_size_in_bytes);
        
        t_bytes_needed += t_size_in_bytes;
    }
    
    return t_bytes_needed;
}

// If wchars is nil, returns the size of the buffer (in wchars needed)
// If wchars is not nil, does the conversion into wchars
uindex_t MCUnicodeCharsMapFromUTF8(const byte_t *utf8bytes, uindex_t utf8byte_count, unichar_t *wchars)
{
    uindex_t t_wchars_needed;
    t_wchars_needed = 0;
    
    uindex_t t_size_in_bytes;
    
    for (index_t i = 0; i < utf8byte_count;)
    {
        // According to the first byte value, we can determine the number of
        // bytes a conversion would need
        if (utf8bytes[i] < 0x80)
            t_size_in_bytes = 1;
        
        else if (wchars[i] < 0xE0)
            t_size_in_bytes = 2;
        
        else if (wchars[i] < 0xF0)
            t_size_in_bytes = 3;
        
        else
            t_size_in_bytes = 4;
        
        if (wchars != nil)
            // Copy the bytes into the unichar_t array
            MCMemoryCopy(wchars + t_wchars_needed, utf8bytes + i, t_size_in_bytes);

        t_wchars_needed ++;
        i += t_size_in_bytes;
    }
    
    return t_wchars_needed;  
}

////////////////////////////////////////////////////////////////////////////////

bool MCUnicodeCharMapToNative(unichar_t p_uchar, char_t& r_nchar)
{
#if defined(__WINDOWS_1252__)
	static const uint32_t s_singletons[] =
	{
		0x8c000152, 0x9c000153, 0x8a000160, 0x9a000161, 0x9f000178, 0x8e00017d,
		0x9e00017e, 0x83000192, 0x880002c6, 0x980002dc, 0x96002013, 0x97002014,
		0x91002018, 0x92002019, 0x8200201a, 0x9300201c, 0x9400201d, 0x8400201e,
		0x86002020, 0x87002021, 0x95002022, 0x85002026, 0x89002030, 0x8b002039,
		0x9b00203a, 0x800020ac, 0x99002122
	};
	if (p_uchar < 0x80 ||
		p_uchar >= 0xA0 && p_uchar <= 0xFF)
	{
		r_nchar = (char_t)p_uchar;
		return true;
	}
#elif defined(__MACROMAN__)
	static const uint32_t s_singletons[] =
	{
		0xca0000a0, 0xc10000a1, 0xa20000a2, 0xa30000a3, 0xb40000a5, 0xa40000a7, 
		0xac0000a8, 0xa90000a9, 0xbb0000aa, 0xc70000ab, 0xc20000ac, 0xa80000ae, 
		0xf80000af, 0xa10000b0, 0xb10000b1, 0xab0000b4, 0xb50000b5, 0xa60000b6, 
		0xe10000b7, 0xfc0000b8, 0xbc0000ba, 0xc80000bb, 0xc00000bf, 0xcb0000c0, 
		0xe70000c1, 0xe50000c2, 0xcc0000c3, 0x800000c4, 0x810000c5, 0xae0000c6, 
		0x820000c7, 0xe90000c8, 0x830000c9, 0xe60000ca, 0xe80000cb, 0xed0000cc, 
		0xea0000cd, 0xeb0000ce, 0xec0000cf, 0x840000d1, 0xf10000d2, 0xee0000d3, 
		0xef0000d4, 0xcd0000d5, 0x850000d6, 0xaf0000d8, 0xf40000d9, 0xf20000da, 
		0xf30000db, 0x860000dc, 0xa70000df, 0x880000e0, 0x870000e1, 0x890000e2, 
		0x8b0000e3, 0x8a0000e4, 0x8c0000e5, 0xbe0000e6, 0x8d0000e7, 0x8f0000e8, 
		0x8e0000e9, 0x900000ea, 0x910000eb, 0x930000ec, 0x920000ed, 0x940000ee, 
		0x950000ef, 0x960000f1, 0x980000f2, 0x970000f3, 0x990000f4, 0x9b0000f5, 
		0x9a0000f6, 0xd60000f7, 0xbf0000f8, 0x9d0000f9, 0x9c0000fa, 0x9e0000fb, 
		0x9f0000fc, 0xd80000ff, 0xf5000131, 0xce000152, 0xcf000153, 0xd9000178, 
		0xc4000192, 0xf60002c6, 0xff0002c7, 0xf90002d8, 0xfa0002d9, 0xfb0002da, 
		0xfe0002db, 0xf70002dc, 0xfd0002dd, 0xbd0003a9, 0xb90003c0, 0xd0002013, 
		0xd1002014, 0xd4002018, 0xd5002019, 0xe200201a, 0xd200201c, 0xd300201d, 
		0xe300201e, 0xa0002020, 0xe0002021, 0xa5002022, 0xc9002026, 0xe4002030, 
		0xdc002039, 0xdd00203a, 0xda002044, 0xdb0020ac, 0xaa002122, 0xb6002202, 
		0xc6002206, 0xb800220f, 0xb7002211, 0xc300221a, 0xb000221e, 0xba00222b, 
		0xc5002248, 0xad002260, 0xb2002264, 0xb3002265, 0xd70025ca, 0xf000f8ff, 
		0xde00fb01, 0xdf00fb02
	};
	if (p_uchar < 0x80)
	{
		r_nchar = (char_t)p_uchar;
		return true;
	}
#elif defined(__ISO_8859_1__)
	static const uint32_t s_singletons[] =
	{
	};
	if (p_uchar <= 0xff)
	{
		r_nchar = (char_t)p_uchar;
		return true;
	}
#else
#error no charset defined
#endif
	uint32_t t_low, t_high;
	t_low = 0;
	t_high = sizeof(s_singletons) / sizeof(s_singletons[0]);
	while(t_low < t_high)
	{
		uint32_t t_mid;
		t_mid = t_low + (t_high - t_low) / 2;
		
		uint32_t t_singleton;
		t_singleton = s_singletons[t_mid] & 0xffffff;
		
		if (p_uchar < t_singleton)
			t_high = t_mid;
		else if (p_uchar > t_singleton)
			t_low = t_mid + 1;
		else
		{
			r_nchar = (char_t)(s_singletons[t_mid] >> 24);
			return true;
		}
	}
	return false;
}

unichar_t MCUnicodeCharMapFromNative(char_t p_nchar)
{
#if defined(__WINDOWS_1252__)
	static const unichar_t kMCStringUnicodeFromWindows1252_Mapping[] =
	{
		0x20ac, 0x0081, 0x201a, 0x0192, 0x201e, 0x2026, 0x2020, 0x2021, 0x02c6, 0x2030, 0x0160, 0x2039, 
		0x0152, 0x008d, 0x017d, 0x008f, 0x0090, 0x2018, 0x2019, 0x201c, 0x201d, 0x2022, 0x2013, 0x2014, 
		0x02dc, 0x2122, 0x0161, 0x203a, 0x0153, 0x009d, 0x017e, 0x0178
	};
	return p_nchar < 0x80 || p_nchar >= 0xA0 ? (unichar_t)p_nchar :  kMCStringUnicodeFromWindows1252_Mapping[p_nchar - 0x80];
#elif defined(__MACROMAN__)
	static const unichar_t kMCStringUnicodeFromMacRoman_Mapping[] =
	{
		0x00c4, 0x00c5, 0x00c7, 0x00c9, 0x00d1, 0x00d6, 0x00dc, 0x00e1, 0x00e0, 0x00e2, 0x00e4, 0x00e3, 
		0x00e5, 0x00e7, 0x00e9, 0x00e8, 0x00ea, 0x00eb, 0x00ed, 0x00ec, 0x00ee, 0x00ef, 0x00f1, 0x00f3, 
		0x00f2, 0x00f4, 0x00f6, 0x00f5, 0x00fa, 0x00f9, 0x00fb, 0x00fc, 0x2020, 0x00b0, 0x00a2, 0x00a3, 
		0x00a7, 0x2022, 0x00b6, 0x00df, 0x00ae, 0x00a9, 0x2122, 0x00b4, 0x00a8, 0x2260, 0x00c6, 0x00d8, 
		0x221e, 0x00b1, 0x2264, 0x2265, 0x00a5, 0x00b5, 0x2202, 0x2211, 0x220f, 0x03c0, 0x222b, 0x00aa, 
		0x00ba, 0x03a9, 0x00e6, 0x00f8, 0x00bf, 0x00a1, 0x00ac, 0x221a, 0x0192, 0x2248, 0x2206, 0x00ab, 
		0x00bb, 0x2026, 0x00a0, 0x00c0, 0x00c3, 0x00d5, 0x0152, 0x0153, 0x2013, 0x2014, 0x201c, 0x201d, 
		0x2018, 0x2019, 0x00f7, 0x25ca, 0x00ff, 0x0178, 0x2044, 0x20ac, 0x2039, 0x203a, 0xfb01, 0xfb02, 
		0x2021, 0x00b7, 0x201a, 0x201e, 0x2030, 0x00c2, 0x00ca, 0x00c1, 0x00cb, 0x00c8, 0x00cd, 0x00ce, 
		0x00cf, 0x00cc, 0x00d3, 0x00d4, 0xf8ff, 0x00d2, 0x00da, 0x00db, 0x00d9, 0x0131, 0x02c6, 0x02dc, 
		0x00af, 0x02d8, 0x02d9, 0x02da, 0x00b8, 0x02dd, 0x02db, 0x02c7 
	};
	return p_nchar < 0x80 ? (unichar_t)p_nchar : kMCStringUnicodeFromMacRoman_Mapping[p_nchar - 0x80];
#elif defined(__ISO_8859_1__)
	return (unichar_t)p_nchar;
#else
#error no charset defined
#endif
}

////////////////////////////////////////////////////////////////////////////////
