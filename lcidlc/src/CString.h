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

#ifndef __MC_FOUNDATION_LEGACY__
#define __MC_FOUNDATION_LEGACY__

#ifndef __MC_FOUNDATION__
#include "foundation.h"
#endif

////////////////////////////////////////////////////////////////////////////////

class MCString;

typedef MCStringOptions     MCCompareOptions;
#define kMCCompareExact     kMCStringOptionCompareExact
#define kMCCompareCaseless  kMCStringOptionCompareCaseless

////////////////////////////////////////////////////////////////////////////////

int UTF8ToUnicode(const char * lpSrcStr, int cchSrc, uint16_t * lpDestStr, int cchDest);
int UnicodeToUTF8(const uint16_t *lpSrcStr, int cchSrc, char *lpDestStr, int cchDest);

////////////////////////////////////////////////////////////////////////////////

bool MCCStringClone(const char *s, char*& r_s);
bool MCCStringCloneSubstring(const char *p_string, uint32_t p_length, char*& r_new_string);
bool MCCStringAppend(char *& x_string, const char *p_suffix);
bool MCCStringFormat(char*& r_string, const char *format, ...);
bool MCCStringFormatV(char*& r_string, const char *format, va_list args);
bool MCCStringAppendFormat(char*& x_string, const char *format, ...);

void MCCStringFree(char *s);

bool MCCStringEqual(const char *x, const char *y);
bool MCCStringEqualCaseless(const char *x, const char *y);
bool MCCStringEqualSubstring(const char *x, const char *y, index_t length);
bool MCCStringEqualSubstringCaseless(const char *x, const char *y, index_t length);

bool MCCStringBeginsWith(const char *string, const char *prefix);
bool MCCStringEndsWith(const char *string, const char *suffix);
bool MCCStringBeginsWithCaseless(const char *string, const char *prefix);
bool MCCStringEndsWithCaseless(const char *string, const char *suffix);

uint32_t MCCStringLength(const char *s);

bool MCCStringSplit(const char *string, char p_separator, char**& r_elements, uint32_t& r_element_count);
bool MCCStringCombine(const char * const *p_elements, uint32_t p_element_count, char p_separator, char*& r_string);

void MCCStringArrayFree(char **cstrings, uint32_t count);

bool MCCStringToNative(const char *string, char*& r_new_string);
bool MCCStringFromNative(const char *string, char*& r_new_string);

bool MCCStringToUnicode(const char *string, unichar_t*& r_unicode_string);
bool MCCStringFromUnicode(const unichar_t* unicode_string, char*& r_string);

bool MCCStringFromUnicodeSubstring(const unichar_t *p_chars, uint32_t p_char_count, char*& r_string);
bool MCCStringFromNativeSubstring(const char *p_string, uint32_t p_length, char*& r_cstring);

bool MCCStringTokenize(const char *string, char**& r_elements, uint32_t& r_element_count);

bool MCCStringFirstIndexOf(const char *p_string, char p_search, uint32_t &r_index);
bool MCCStringFirstIndexOf(const char *p_string, const char *p_search, uint32_t &r_index);
bool MCCStringLastIndexOf(const char *p_string, char p_search, uint32_t &r_index);
bool MCCStringLastIndexOf(const char *p_string, const char *p_search, uint32_t &r_index);
bool MCCStringContains(const char *p_haystack, const char *p_needle);

bool MCCStringIsEmpty(const char *);
bool MCCStringIsInteger(const char *);

#endif
