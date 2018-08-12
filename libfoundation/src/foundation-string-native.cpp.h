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

#include <foundation.h>
#include <foundation-auto.h>
#include <foundation-unicode.h>
#include <foundation-string-hash.h>

#include "foundation-private.h"

////////////////////////////////////////////////////////////////////////////////

// This file contains low-level native string operations and is designed to be
// included in foundation-string.cpp.

// Although the current operations are based around 'char_t' the properties
// of the strings they assume are that all their characters are 1 unit long,
// are composed and folding preserves length.

////////////////////////////////////////////////////////////////////////////////

// Identity function used by templates for cases where no case folding is
// required.
inline
char_t __MCNativeChar_NoFold(char_t p_char)
{
    return p_char;
}

// Return the folded version of 'char'.
inline
char_t __MCNativeChar_Fold(char_t p_char)
{
#if defined(__WINDOWS_1252__) || defined(__ISO_8859_1__)
	static const char_t kMCStringFoldWindows1252_Mapping[256] =
	{
		0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
		0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
		0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
		0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f,
		0x40, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f,
		0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f,
		0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f,
		0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a, 0x7b, 0x7c, 0x7d, 0x7e, 0x7f,
		0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x9a, 0x8b, 0x9c, 0x8d, 0x9e, 0x8f,
		0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0xff,
		0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf,
		0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xbb, 0xbc, 0xbd, 0xbe, 0xbf,
		0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef,
		0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xd7, 0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xdf,
		0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef,
		0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff,
	};
	return kMCStringFoldWindows1252_Mapping[p_char];
#elif defined(__MACROMAN__)
	static const char_t kMCStringFoldMacRoman_Mapping[256] =
	{
		0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
		0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
		0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
		0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f,
		0x40, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f,
		0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f,
		0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f,
		0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 0x7a, 0x7b, 0x7c, 0x7d, 0x7e, 0x7f,
		0x8a, 0x8c, 0x8d, 0x8e, 0x96, 0x9a, 0x9f, 0x87, 0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f,
		0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f,
		0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xbe, 0xbf,
		0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xbb, 0xbc, 0xbd, 0xbe, 0xbf,
		0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0x88, 0x8b, 0x9b, 0xcf, 0xcf,
		0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd8, 0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xdf,
		0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0x89, 0x90, 0x87, 0x91, 0x8f, 0x92, 0x94, 0x95, 0x93, 0x97, 0x99,
		0xf0, 0x98, 0x9c, 0x9e, 0x9d, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff,
	};
	return kMCStringFoldMacRoman_Mapping[p_char];
#else
#error Unknown native encoding
#endif
}

// Return the folded version of 'char' into 'folded_char', and return true if
// the char is cased (i.e. there exists c' != c s.t. fold(c') == c).
static inline bool
__MCNativeChar_CheckedFold(char_t p_char,
                           char_t& r_folded_char)
{
	/* These bit-tables can be automatically generated from the tables
	 in __MCNativeChar__Fold() and __MCNativeChar_Uppercase() using a
	 script like:

	 function MakeCheckMap pLowerMap, pUpperMap
	    local tResult, tRow, tBit, tChar, tBitMap
	    put empty into tResult
	    repeat with tRow = 1 to 8
	       put 0 into tBitMap
	       repeat with tBit = 1 to 32
	          put (tRow - 1) * 32 + tBit into tChar

	          if (item tChar of pLowerMap) is not \
	                (item tChar of pUpperMap) then
	             add 1 * (2 ^ (tBit - 1)) to tBitMap
	          end if
	       end repeat
	       put baseConvert(tBitMap, 10, 16) into tBitMap
	       repeat while the number of chars in tBitMap < 8
	          put "0" before tBitMap
	       end repeat
	       put "0x" & tBitMap & "U" after tResult
	       put comma & return after tResult
	    end repeat
	    return tResult
	 end MakeCheckMap
	*/
	uint8_t t_hi = p_char >> 5;
	uint8_t t_lo = p_char & 0x1F;
#if defined(__WINDOWS_1252__) || defined(__ISO_8859_1__)
	static const uint32_t kMCStringCheckFoldWindows1252_Mapping[8] =
		{
			0x00000000U,
			0x00000000U,
			0x07FFFFFEU,
			0x07FFFFFEU,
			0xD4005400U,
			0x00000000U,
			0x7F7FFFFFU,
			0xFF7FFFFFU,
		};
	if (!(kMCStringCheckFoldWindows1252_Mapping[t_hi] & (1 << t_lo)))
	{
		r_folded_char = p_char;
		return false;
	}
#elif defined(__MACROMAN__)
	static const uint32_t kMCStringCheckFoldMacRoman_Mapping[8] =
		{
			0x00000000U,
			0x00000000U,
			0x07FFFFFEU,
			0x07FFFFFEU,
			0xFFFFFFFFU,
			0xC000C000U,
			0x0300F800U,
			0x003EFFE0U,
		};
	if (!(kMCStringCheckFoldMacRoman_Mapping[t_hi] & (1 << t_lo)))
	{
		r_folded_char = p_char;
		return false;
	}
#else
	#error Unknown native encoding
#endif
    r_folded_char = __MCNativeChar_Fold(p_char);
    return true;
}

// Fold 'length' characters in 'chars', placing them into 'out_chars'.
static inline void
__MCNativeStr_Fold(const char_t *p_chars,
                   size_t p_length,
                   char_t *r_out_chars)
{
    for(; p_length > 0; p_length -= 1)
        *r_out_chars++ = __MCNativeChar_Fold(*p_chars++);
}

// Fold 'length' characters in 'chars', placing them into 'out_chars'.
// The function returns true if at least one of the chars is cased.
static inline bool
__MCNativeStr_CheckedFold(const char_t *p_chars,
                          size_t p_length,
                          char_t *r_out_char)
{
    bool t_needs_fold;
    t_needs_fold = false;
    
    for(; p_length > 0; p_length -= 1)
    {
        if (__MCNativeChar_CheckedFold(*p_chars++, *r_out_char++))
        {
            t_needs_fold = true;
            break;
        }
    }
    
    for(; p_length > 0; p_length -= 1)
        *r_out_char++ = __MCNativeChar_Fold(*p_chars++);

    return t_needs_fold;
}

////////////////////////////////////////////////////////////////////////////////

// Return the lower-case version of 'char'.
static inline
char_t __MCNativeChar_Lowercase(char_t p_char)
{
    return __MCNativeChar_Fold(p_char);
}

// Return the upper-case version of 'char'.
static inline
char_t __MCNativeChar_Uppercase(char_t p_char)
{
#if defined(__WINDOWS_1252__) || defined(__ISO_8859_1__)
	static char_t kMCStringUppercaseWindows1252_Mapping[256] =
	{
		0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
		0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
		0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
		0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f,
		0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f,
		0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f,
		0x60, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f,
		0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a, 0x7b, 0x7c, 0x7d, 0x7e, 0x7f,
		0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f,
		0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x8a, 0x9b, 0x8c, 0x9d, 0x8e, 0x9f,
		0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf,
		0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xbb, 0xbc, 0xbd, 0xbe, 0xbf,
		0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf,
		0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8, 0xd9, 0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xdf,
		0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf,
		0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xf7, 0xd8, 0xd9, 0xda, 0xdb, 0xdc, 0xdd, 0xde, 0x9f,
	};
	return kMCStringUppercaseWindows1252_Mapping[p_char];
#elif defined(__MACROMAN__)
	static char_t kMCStringUppercaseMacRoman_Mapping[256] =
	{
		0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
		0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
		0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
		0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f,
		0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f,
		0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f,
		0x60, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f,
		0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a, 0x7b, 0x7c, 0x7d, 0x7e, 0x7f,
		0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0xe7, 0xcb, 0xe5, 0x80, 0xcc, 0x81, 0x82, 0x83, 0xe9,
		0xe6, 0xe8, 0xea, 0xed, 0xeb, 0xec, 0x84, 0xee, 0xf1, 0xef, 0x85, 0xcd, 0xf2, 0xf4, 0xf3, 0x86,
		0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf,
		0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xbb, 0xbc, 0xbd, 0xae, 0xaf,
		0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xce,
		0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd9, 0xd9, 0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xdf,
		0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef,
		0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0x49, 0xf6, 0xf7, 0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff,
	};
	return kMCStringUppercaseMacRoman_Mapping[p_char];
#else
#error Unknown native encoding
#endif
}

////////////////////////////////////////////////////////////////////////////////

// Compare two uncased or prefolded chars for caseless equality.
inline bool
__MCNativeChar_Equal_Unfolded(char_t p_left,
                              char_t p_right)
{
    return p_left == p_right;
}

// Compare an unfolded char with an uncased or prefolded char for caseless
// equality.
inline bool
__MCNativeChar_Equal_Prefolded(char_t p_left,
                               char_t p_folded_right)
{
    if (p_left != p_folded_right)
        return __MCNativeChar_Fold(p_left) ==
                p_folded_right;
    
    return true;
}

// Compare two unfolded chars for caseless equality.
inline bool
__MCNativeChar_Equal_Folded(char_t p_left,
                            char_t p_right)
{
    if (p_left != p_right)
        return __MCNativeChar_Fold(p_left) ==
                __MCNativeChar_Fold(p_right);
    
    return true;
}

// Compare two uncased or prefolded chars.
inline ssize_t
__MCNativeChar_Compare_Unfolded(char_t p_left,
                                char_t p_right)
{
    return p_left - p_right;
}

// Compare an unfolded char with an uncased or prefolded char.
inline ssize_t
__MCNativeChar_Compare_Prefolded(char_t p_left,
                                 char_t p_folded_right)
{
    if (p_left != p_folded_right)
        return __MCNativeChar_Fold(p_left) -
                p_folded_right;
    
    return 0;
}

// Compare two unfolded chars.
inline ssize_t
__MCNativeChar_Compare_Folded(char_t p_left,
                              char_t p_right)
{
    if (p_left != p_right)
        return __MCNativeChar_Fold(p_left) -
                __MCNativeChar_Fold(p_right);
    
    return 0;
}

////////////////////////////////////////////////////////////////////////////////

// Check the two strings for equality, using the given char comparison method.
template<bool (*CharEqual)(char_t left, char_t right)>
static inline bool
__MCNativeStr_Equal(const char_t *p_left_chars,
                    size_t p_left_length,
                    const char_t *p_right_chars,
                    size_t p_right_length)
{
    if (p_left_length != p_right_length)
        return false;
    
    if (p_left_chars == p_right_chars)
        return true;
    
    while(p_left_length > 0)
    {
        if (!CharEqual(*p_left_chars++, *p_right_chars++))
            return false;
        
        p_left_length -= 1;
    }
    
    return true;
}

// Compare two strings, using the given char comparison method.
template<ssize_t (*CharCompare)(char_t left, char_t right)>
static inline ssize_t
__MCNativeStr_Compare(const char_t *p_left_chars,
                      size_t p_left_length,
                      const char_t *p_right_chars,
                      size_t p_right_length)
{
    for(;;)
    {
        if (p_left_length == 0 || p_right_length == 0)
            break;
        
        ssize_t t_diff;
        t_diff = CharCompare(*p_left_chars++, *p_right_chars++);
        if (t_diff != 0)
            return t_diff;

        p_left_length -= 1;
        p_right_length -= 1;
    }
    
    return (signed)p_left_length - (signed)p_right_length;
}

// Find the maximum equal prefix of the two strings, using the given char
// comparison method. The length of the shared prefix is returned.
template<bool (*CharEqual)(char_t left, char_t right)>
static inline size_t
__MCNativeStr_Prefix(const char_t *p_left_chars,
                     size_t p_left_length,
                     const char_t *p_right_chars,
                     size_t p_right_length)
{
    if (p_right_length < p_left_length)
        p_left_length = p_right_length;
    
    size_t t_prefix;
    t_prefix = 0;
    for(;;)
    {
        if (p_left_length == 0)
            break;
        
        if (!CharEqual(*p_left_chars++, *p_right_chars++))
            break;
        
        t_prefix += 1;
        
        p_left_length -= 1;
    }
    
    return t_prefix;
}

// Find the maximum equal suffix of the two strings, using the given char
// comparison method. The length of the shared suffix is returned.
template<bool (*CharEqual)(char_t left, char_t right)>
static inline size_t
__MCNativeStr_Suffix(const char_t *p_left_chars,
                     size_t p_left_length,
                     const char_t *p_right_chars,
                     size_t p_right_length)
{
    p_left_chars += p_left_length;
    p_right_chars += p_right_length;
    
    if (p_right_length < p_left_length)
        p_left_length = p_right_length;
    
    size_t t_suffix;
    t_suffix = 0;
    for(;;)
    {
        if (p_left_length == 0)
            break;
        
        if (!CharEqual(*--p_left_chars, *--p_right_chars))
            break;
        
        t_suffix += 1;
        
        p_left_length -= 1;
    }
    
    return t_suffix;
}

// Return the hash of the given string, using the given char folding method.
template<char_t (*CharFold)(char_t chr)>
static inline hash_t
__MCNativeStr_Hash(const char_t *p_chars,
                   size_t p_char_count)
{
    MCHashCharsContext t_context;
    while (p_char_count--)
        t_context.consume(MCUnicodeCharMapFromNative(CharFold(*p_chars++)));
    return t_context;
}

////////////////////////////////////////////////////////////////////////////////

// Most delimiter based string operations depend on a single 'scanning'
// operation:
//
//   Scan(haystack, needle, max_count -> found_count, last_found_offset)
//
// If max_count is 0, then the number of occurrences of needle in haystack is
// returned.
//
// If max_count is not 0, then the haystack will be scanned for at most
// max_count occurences with the number found returned.
//
// In both cases, if at least 1 occurrence was found then the offset of the last
// found occurrence is also returned.

template<bool (*CharEqual)(char_t left, char_t right)>
struct __MCNativeStr_Forward
{
    static inline size_t CharScan(const char_t *p_haystack_chars,
                                  size_t p_haystack_length,
                                  char_t p_needle_char,
                                  size_t p_max_count,
                                  size_t *r_offset)
    {
        size_t t_count;
        t_count = 0;
        
        size_t t_char_offset;
        t_char_offset = 0;
        
        size_t t_offset = 0;
        while(t_char_offset < p_haystack_length)
        {
            if (CharEqual(p_haystack_chars[t_char_offset], p_needle_char))
            {
                t_offset = t_char_offset;
                t_count += 1;
                
                if (t_count == p_max_count)
                    break;
            }
            
            t_char_offset += 1;
        }
        
        if (t_count > 0 &&
            r_offset != nil)
            *r_offset = t_offset;
        
        return t_count;
    }

    static inline size_t Scan(const char_t *p_haystack_chars,
                              size_t p_haystack_length,
                              const char_t *p_needle_chars,
                              size_t p_needle_length,
                              size_t p_max_count,
                              size_t *r_offset)
    {
        if (p_needle_length == 0)
            return 0;
        
        p_haystack_length -= p_needle_length;
        
        size_t t_count;
        t_count = 0;
        
        size_t t_char_offset;
        t_char_offset = 0;
        
        size_t t_offset = 0;
        while(t_char_offset <= p_haystack_length)
        {
            if (__MCNativeStr_Equal<CharEqual>(p_haystack_chars + t_char_offset,
                                               p_needle_length,
                                               p_needle_chars,
                                               p_needle_length))
            {
                t_offset = t_char_offset;
                t_count += 1;
                
                if (t_count == p_max_count)
                    break;
                
                t_char_offset += p_needle_length;
            }
            else
                t_char_offset += 1;
        }
        
        if (t_count > 0 &&
            r_offset != nil)
            *r_offset = t_offset;
        
        return t_count;
    }
};

template<bool (*CharEqual)(char_t left, char_t right)>
struct __MCNativeStr_Reverse
{
    static inline size_t CharScan(const char_t *p_haystack_chars,
                                  size_t p_haystack_length,
                                  char_t p_needle_char,
                                  size_t p_max_count,
                                  size_t *r_offset)
    {
        size_t t_count;
        t_count = 0;
        
        size_t t_char_offset;
        t_char_offset = p_haystack_length;
        
        size_t t_offset = 0;
        while(t_char_offset > 0)
        {
            if (CharEqual(p_haystack_chars[t_char_offset - 1], p_needle_char))
            {
                t_offset = t_char_offset;
                t_count += 1;
                
                if (t_count == p_max_count)
                    break;
            }
            
            t_char_offset -= 1;
        }
        
        if (t_count > 0 &&
            r_offset != nil)
            *r_offset = t_offset - 1;
        
        return t_count;
    }
    
    static inline size_t Scan(const char_t *p_haystack_chars,
                              size_t p_haystack_length,
                              const char_t *p_needle_chars,
                              size_t p_needle_length,
                              size_t p_max_count,
                              size_t *r_offset)
    {
        if (p_needle_length == 0)
            return 0;
        
        p_haystack_length -= p_needle_length;
        
        size_t t_count;
        t_count = 0;
        
        size_t t_char_offset;
        t_char_offset = p_haystack_length;
        
        size_t t_offset = 0;
        while(t_char_offset > 0)
        {
            if (__MCNativeStr_Equal<CharEqual>(p_haystack_chars + t_char_offset - 1,
                                               p_needle_length,
                                               p_needle_chars,
                                               p_needle_length))
            {
                t_offset = t_char_offset;
                t_count += 1;
                
                if (t_count == p_max_count)
                    break;
                
                t_char_offset -= p_needle_length;
            }
            else
                t_char_offset -= 1;
        }
        
        if (t_count > 0 &&
            r_offset != nil)
            *r_offset = t_offset - 1;
        
        return t_count;
    }
};

////////////////////////////////////////////////////////////////////////////////

// This is the maximum length of 'needle' which will be prefolded in a caseless
// operation.
#define __NATIVEOP_PREFOLD_LIMIT 64

// Returns true if the given string options require folded comparison.
static inline bool __MCNativeOp_IsFolded(MCStringOptions p_options)
{
    return p_options >= kMCStringOptionCompareFolded;
}

// Compare the two strings for equality, taking into account the given options.
static bool __MCNativeOp_IsEqualTo(const char_t *p_left_chars,
                                   size_t p_left_length,
                                   const char_t *p_right_chars,
                                   size_t p_right_length,
                                   MCStringOptions p_options)
{
    if (__MCNativeOp_IsFolded(p_options))
        return __MCNativeStr_Equal<__MCNativeChar_Equal_Folded>(p_left_chars,
                                                                p_left_length,
                                                                p_right_chars,
                                                                p_right_length);
    
    return __MCNativeStr_Equal<__MCNativeChar_Equal_Unfolded>(p_left_chars,
                                                              p_left_length,
                                                              p_right_chars,
                                                              p_right_length);
}

// Compare the two strings, taking into account the given options.
static ssize_t __MCNativeOp_CompareTo(const char_t *p_left_chars,
                                      size_t p_left_length,
                                      const char_t *p_right_chars,
                                      size_t p_right_length,
                                      MCStringOptions p_options)
{
    if (__MCNativeOp_IsFolded(p_options))
        return __MCNativeStr_Compare<__MCNativeChar_Compare_Folded>(p_left_chars,
                                                                    p_left_length,
                                                                    p_right_chars,
                                                                    p_right_length);
    
    return __MCNativeStr_Compare<__MCNativeChar_Compare_Unfolded>(p_left_chars,
                                                                  p_left_length,
                                                                  p_right_chars,
                                                                  p_right_length);
}

// Hash the given string, taking into account the given options.
static hash_t __MCNativeOp_Hash(const char_t *p_chars,
                                size_t p_char_count,
                                MCStringOptions p_options)
{
    if (__MCNativeOp_IsFolded(p_options))
        return __MCNativeStr_Hash<__MCNativeChar_Fold>(p_chars,
                                                       p_char_count);

    return __MCNativeStr_Hash<__MCNativeChar_NoFold>(p_chars,
                                                     p_char_count);
}

// Return the length of maximum shared prefix of the two strings, taking into
// account the given options.
static size_t __MCNativeOp_SharedPrefix(const char_t *p_left_chars,
                                        size_t p_left_length,
                                        const char_t *p_right_chars,
                                        size_t p_right_length,
                                        MCStringOptions p_options)
{
    if (__MCNativeOp_IsFolded(p_options))
        return __MCNativeStr_Prefix<__MCNativeChar_Equal_Folded>(p_left_chars,
                                                                 p_left_length,
                                                                 p_right_chars,
                                                                 p_right_length);
    
    return __MCNativeStr_Prefix<__MCNativeChar_Equal_Unfolded>(p_left_chars,
                                                               p_left_length,
                                                               p_right_chars,
                                                               p_right_length);
}

// Return the length of maximum shared suffix of the two strings, taking into
// account the given options.
static size_t __MCNativeOp_SharedSuffix(const char_t *p_left_chars,
                                        size_t p_left_length,
                                        const char_t *p_right_chars,
                                        size_t p_right_length,
                                        MCStringOptions p_options)
{
    if (__MCNativeOp_IsFolded(p_options))
        return __MCNativeStr_Suffix<__MCNativeChar_Equal_Folded>(p_left_chars,
                                                                 p_left_length,
                                                                 p_right_chars,
                                                                 p_right_length);
    
    return __MCNativeStr_Suffix<__MCNativeChar_Equal_Unfolded>(p_left_chars,
                                                               p_left_length,
                                                               p_right_chars,
                                                               p_right_length);
}

// Perform the appropriate direction 'scan' operation with the given parameters.
// The direction is determined by the classed used for 'Actions'.
template<template<bool (*CharEqual)(char_t left, char_t right)> class Actions>
static size_t __MCNativeOp_Scan(const char_t *p_haystack_chars,
                                size_t p_haystack_length,
                                const char_t *p_needle_chars,
                                size_t p_needle_length,
                                size_t p_max_count,
                                MCStringOptions p_options,
                                size_t *r_offset)
{
	char_t t_folded_needle_char;
	char_t t_folded_needle[__NATIVEOP_PREFOLD_LIMIT];

	if (p_needle_length > p_haystack_length)
		return 0;

	if (!__MCNativeOp_IsFolded(p_options))
	{
		if (p_needle_length == 1)
		{
			goto unfolded_char;
		}
		else
		{
			goto unfolded;
		}
	}
	else
	{
		if (p_needle_length == 1)
		{
			if (__MCNativeChar_CheckedFold(p_needle_chars[0], t_folded_needle_char))
				goto prefolded_char;
			else
				goto unfolded_char;
		}
		else if (p_needle_length < __NATIVEOP_PREFOLD_LIMIT)
		{
			if (__MCNativeStr_CheckedFold(p_needle_chars, p_needle_length, t_folded_needle))
				goto prefolded;
			else
				goto unfolded;
		}
		else
		{
			goto folded;
		}
	}

 unfolded_char:
	return Actions<__MCNativeChar_Equal_Unfolded>::CharScan
		(p_haystack_chars,
		 p_haystack_length,
		 p_needle_chars[0],
		 p_max_count,
		 r_offset);
 unfolded:
	return Actions<__MCNativeChar_Equal_Unfolded>::Scan
		(p_haystack_chars,
		 p_haystack_length,
		 p_needle_chars,
		 p_needle_length,
		 p_max_count,
		 r_offset);
 prefolded_char:
	return Actions<__MCNativeChar_Equal_Prefolded>::CharScan
		(p_haystack_chars,
		 p_haystack_length,
		 t_folded_needle_char,
		 p_max_count,
		 r_offset);
 prefolded:
	return Actions<__MCNativeChar_Equal_Prefolded>::Scan
		(p_haystack_chars,
		 p_haystack_length,
		 t_folded_needle,
		 p_needle_length,
		 p_max_count,
		 r_offset);
 folded:
	return Actions<__MCNativeChar_Equal_Folded>::Scan
		(p_haystack_chars,
		 p_haystack_length,
		 p_needle_chars,
		 p_needle_length,
		 p_max_count,
		 r_offset);
}

#define __MCNativeOp_ForwardScan __MCNativeOp_Scan<__MCNativeStr_Forward>
#define __MCNativeOp_ReverseScan __MCNativeOp_Scan<__MCNativeStr_Reverse>

// Search forward for needle in haystack, using the given options.
// If needle is found, then its offset is returned and the result is true.
static bool __MCNativeOp_FirstIndexOf(const char_t *p_haystack_chars,
                                      size_t p_haystack_length,
                                      const char_t *p_needle_chars,
                                      size_t p_needle_length,
                                      MCStringOptions p_options,
                                      size_t& r_offset)
{
    return __MCNativeOp_ForwardScan(p_haystack_chars,
                                    p_haystack_length,
                                    p_needle_chars,
                                    p_needle_length,
                                    1,
                                    p_options,
                                    &r_offset) == 1;
}

// Search backward for needle in haystack, using the given options.
// If needle is found, then its offset is returned and the result is true.
static bool __MCNativeOp_LastIndexOf(const char_t *p_haystack_chars,
                                     size_t p_haystack_length,
                                     const char_t *p_needle_chars,
                                     size_t p_needle_length,
                                     MCStringOptions p_options,
                                     size_t& r_offset)
{
    return __MCNativeOp_ReverseScan(p_haystack_chars,
                                    p_haystack_length,
                                    p_needle_chars,
                                    p_needle_length,
                                    1,
                                    p_options,
                                    &r_offset) == 1;
}

// Skips count occurrences of needle in haystack, using the given options.
// If the specified number of occurrences are found then true is returned and
// the offset of the last one is passed back.
static bool __MCNativeOp_Skip(const char_t *p_haystack_chars,
                              size_t p_haystack_length,
                              const char_t *p_needle_chars,
                              size_t p_needle_length,
                              size_t p_count,
                              MCStringOptions p_options,
                              size_t *r_last_offset)
{
    return __MCNativeOp_ForwardScan(p_haystack_chars,
                                    p_haystack_length,
                                    p_needle_chars,
                                    p_needle_length,
                                    p_count,
                                    p_options,
                                    r_last_offset) == p_count;
}

// Return the number of occurrences of needle in haystack, using the given
// options. The offset of the last found occurrence is returned, if any.
static size_t __MCNativeOp_Count(const char_t *p_haystack_chars,
                                 size_t p_haystack_length,
                                 const char_t *p_needle_chars,
                                 size_t p_needle_length,
                                 MCStringOptions p_options,
                                 size_t *r_last_offset)
{
    return __MCNativeOp_ForwardScan(p_haystack_chars,
                                    p_haystack_length,
                                    p_needle_chars,
                                    p_needle_length,
                                    0,
                                    p_options,
                                    r_last_offset);
}

// Return true if haystack contains needle, using the given options.
static bool __MCNativeOp_Contains(const char_t *p_haystack_chars,
                                  size_t p_haystack_length,
                                  const char_t *p_needle_chars,
                                  size_t p_needle_length,
                                  MCStringOptions p_options)
{
    return __MCNativeOp_ForwardScan(p_haystack_chars,
                                    p_haystack_length,
                                    p_needle_chars,
                                    p_needle_length,
                                    1,
                                    p_options,
                                    nil) == 1;
}

// Return true if haystack begins with needle, using the given options.
static bool __MCNativeOp_BeginsWith(const char_t *p_haystack_chars,
                                    size_t p_haystack_length,
                                    const char_t *p_needle_chars,
                                    size_t p_needle_length,
                                    MCStringOptions p_options)
{
    if (p_needle_length > p_haystack_length)
        return false;
    
    return __MCNativeOp_IsEqualTo(p_haystack_chars,
                                  p_needle_length,
                                  p_needle_chars,
                                  p_needle_length,
                                  p_options);
}

// Return true if haystack ends with needle, using the given options.
static bool __MCNativeOp_EndsWith(const char_t *p_haystack_chars,
                                  size_t p_haystack_length,
                                  const char_t *p_needle_chars,
                                  size_t p_needle_length,
                                  MCStringOptions p_options)
{
    if (p_needle_length > p_haystack_length)
        return false;
    
    return __MCNativeOp_IsEqualTo(p_haystack_chars + p_haystack_length - p_needle_length,
                                  p_needle_length,
                                  p_needle_chars,
                                  p_needle_length,
                                  p_options);
}

////////////////////////////////////////////////////////////////////////////////

// The Core template (parameterized by char equality comparator) for the
// optimized DelimiterOffset function when delimiter is a single char.
template<bool (*DelimiterCharEqual)(char_t left, char_t right)>
static bool __MCNativeOp_ForwardCharDelimitedOffset_Core(const char_t *p_haystack_chars,
                                                         size_t p_haystack_length,
                                                         const char_t *p_needle_chars,
                                                         size_t p_needle_length,
                                                         char_t p_delimiter_char,
                                                         size_t p_skip,
                                                         MCStringOptions p_options,
                                                         size_t& r_index,
                                                         size_t *r_found_offset,
                                                         size_t *r_before_offset,
                                                         size_t *r_after_offset)
{
    if (p_needle_length == 0)
        return false;
    
    size_t t_index;
    t_index = 0;
    
    size_t t_offset = 0;
    t_offset = 0;
    
    size_t t_end_before;
    t_end_before = 0;
    for(; p_skip > 0 && t_offset < p_haystack_length; t_offset++)
    {
        if (DelimiterCharEqual(p_haystack_chars[t_offset], p_delimiter_char))
        {
            p_skip -= 1;
            t_index += 1;
            t_end_before = t_offset;
        }
    }
    
    size_t t_start_found;
    if (!__MCNativeOp_FirstIndexOf(p_haystack_chars + t_offset,
                                   p_haystack_length - t_offset,
                                   p_needle_chars,
                                   p_needle_length,
                                   p_options,
                                   t_start_found))
        return false;
    
    t_start_found += t_offset;
    
    for(; t_offset < t_start_found; t_offset++)
    {
        if (DelimiterCharEqual(p_haystack_chars[t_offset], p_delimiter_char))
        {
            t_index += 1;
            t_end_before = t_offset;
        }
    }
    
    r_index = t_index;
    
    if (r_found_offset != nil)
        *r_found_offset = t_start_found;
    
    if (r_before_offset != nil)
        *r_before_offset = t_end_before;
    
    if (r_after_offset != nil)
    {
        for(t_offset = t_start_found + p_needle_length; t_offset < p_haystack_length; t_offset++)
            if (DelimiterCharEqual(p_haystack_chars[t_offset], p_delimiter_char))
                break;
        
        *r_after_offset = t_offset;
    }
    
    return true;
}

// Returns true if needle is found in haystack after skip occurrences of
// delimiter.
// If skip delimiters are not found, or needle is not found then false is
// returned.
// Otherwise, r_index will contain the total number of delimiters in haystack
// before the found needle, found_offset will contain the offset of needle in
// haystack, before_offset the offset of the char after the previous found
// delimiter and after_offset the offset of the delimiter after the found
// string.
static bool __MCNativeOp_ForwardCharDelimitedOffset(const char_t *p_haystack_chars,
                                                    size_t p_haystack_length,
                                                    const char_t *p_needle_chars,
                                                    size_t p_needle_length,
                                                    char_t p_delimiter_char,
                                                    size_t p_skip,
                                                    MCStringOptions p_options,
                                                    size_t& r_index,
                                                    size_t *r_found_offset,
                                                    size_t *r_before_offset,
                                                    size_t *r_after_offset)
{
    if (__MCNativeOp_IsFolded(p_options))
    {
        if (__MCNativeChar_CheckedFold(p_delimiter_char, p_delimiter_char))
            return __MCNativeOp_ForwardCharDelimitedOffset_Core<__MCNativeChar_Equal_Prefolded>
                        (p_haystack_chars,
                         p_haystack_length,
                         p_needle_chars,
                         p_needle_length,
                         p_delimiter_char,
                         p_skip,
                         p_options,
                         r_index,
                         r_found_offset,
                         r_before_offset,
                         r_after_offset);
    }
    
    return __MCNativeOp_ForwardCharDelimitedOffset_Core<__MCNativeChar_Equal_Unfolded>
                (p_haystack_chars,
                 p_haystack_length,
                 p_needle_chars,
                 p_needle_length,
                 p_delimiter_char,
                 p_skip,
                 p_options,
                 r_index,
                 r_found_offset,
                 r_before_offset,
                 r_after_offset);
}

////////////////////////////////////////////////////////////////////////////////

char_t MCNativeCharFold(char_t p_char)
{
    return __MCNativeChar_Fold(p_char);
}

char_t MCNativeCharUppercase(char_t p_char)
{
    return __MCNativeChar_Uppercase(p_char);
}

void MCNativeCharsLowercase(char_t *x_chars, uindex_t p_char_count)
{
	for(uindex_t i = 0; i < p_char_count; i++)
		x_chars[i] = __MCNativeChar_Lowercase(x_chars[i]);
}

void MCNativeCharsUppercase(char_t *x_chars, uindex_t p_char_count)
{
	for(uindex_t i = 0; i < p_char_count; i++)
		x_chars[i] = __MCNativeChar_Uppercase(x_chars[i]);
}

bool MCNativeCharsFormatV(char_t*& r_string, uindex_t& r_size, const char *p_format, va_list p_args)
{
    int t_count;
#if defined(_WINDOWS) || defined(_WINDOWS_SERVER)
	t_count = _vscprintf(p_format, p_args);
#elif defined(_MACOSX) || defined(_LINUX) || defined(TARGET_SUBPLATFORM_IPHONE) || defined(TARGET_SUBPLATFORM_ANDROID) || defined(__EMSCRIPTEN__)
    va_list t_args;
    va_copy(t_args, p_args);
	t_count = vsnprintf(nil, 0, p_format, t_args);
    va_end(t_args);
#else
#error "Implement MCCStringFormat"
#endif
    
	char_t *t_new_string;
	if (!MCMemoryAllocate((size_t)(t_count + 1), t_new_string))
		return false;
    
	vsprintf((char *)t_new_string, p_format, p_args);
    
	r_size = (uindex_t)t_count;
	r_string = t_new_string;
    
	return true;
}

////////////////////////////////////////////////////////////////////////////////
