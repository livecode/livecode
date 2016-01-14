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


#include "CString.h"

#ifdef __MAC__
#include <CoreFoundation/CoreFoundation.h>
#endif

#ifdef __WINDOWS__
#include <windows.h>
#endif

////////////////////////////////////////////////////////////////////////////////

bool MCCStringClone(const char *p_string, char *& r_new_string)
{
	if (p_string == nil)
	{
		r_new_string = nil;
		return true;
	}

	if (!MCMemoryAllocate(strlen(p_string) + 1, r_new_string))
		return false;
	strcpy(r_new_string, p_string);
	return true;
}

bool MCCStringAppend(char*& x_string, const char *p_extra_string)
{
	uint32_t t_old_length, t_new_length;
	t_old_length = MCCStringLength(x_string);
	t_new_length = MCCStringLength(p_extra_string);
	
	if (!MCMemoryReallocate(x_string, t_old_length + t_new_length + 1, x_string))
		return false;
	
	MCMemoryCopy(x_string + t_old_length, p_extra_string, t_new_length + 1);

	return true;
}

bool MCCStringCloneSubstring(const char *p_string, uint32_t p_length, char*& r_new_string)
{
	if (!MCMemoryAllocate(p_length + 1, r_new_string))
		return false;
	MCMemoryCopy(r_new_string, p_string, p_length);
	r_new_string[p_length] = '\0';
	return true;
}

bool MCCStringFormat(char*& r_string, const char *p_format, ...)
{
	va_list t_args;
	int t_count;
#if defined(_WINDOWS) || defined(_WINDOWS_SERVER)
	va_start(t_args, p_format);
	t_count = _vscprintf(p_format, t_args);
	va_end(t_args);
#elif defined(_MACOSX) || defined(_LINUX) || defined(TARGET_SUBPLATFORM_IPHONE) || defined(TARGET_SUBPLATFORM_ANDROID) || defined(_MAC_SERVER)
	va_start(t_args, p_format);
	t_count = vsnprintf(nil, 0, p_format, t_args);
	va_end(t_args);
#else
#error "Implement MCCStringFormat"
#endif

	char *t_new_string;
	if (!MCMemoryAllocate(t_count + 1, t_new_string))
		return false;

	va_start(t_args, p_format);
	vsprintf(t_new_string, p_format, t_args);
	va_end(t_args);

	r_string = t_new_string;

	return true;
}

bool MCCStringFormatV(char*& r_string, const char *p_format, va_list p_args)
{
	int t_count;
#if defined(_WINDOWS) || defined(_WINDOWS_SERVER)
	t_count = _vscprintf(p_format, p_args);
#elif defined(_MACOSX) || defined(_LINUX) || defined(TARGET_SUBPLATFORM_IPHONE) || defined(TARGET_SUBPLATFORM_ANDROID) || defined(_MAC_SERVER)
	t_count = vsnprintf(nil, 0, p_format, p_args);
#else
#error "Implement MCCStringFormat"
#endif

	char *t_new_string;
	if (!MCMemoryAllocate(t_count + 1, t_new_string))
		return false;

	vsprintf(t_new_string, p_format, p_args);

	r_string = t_new_string;

	return true;
}

bool MCCStringAppendFormat(char*& x_string, const char *p_format, ...)
{
	va_list t_args;
	int t_count;
#if defined(_WINDOWS) || defined(_WINDOWS_SERVER)
	va_start(t_args, p_format);
	t_count = _vscprintf(p_format, t_args);
	va_end(t_args);
#elif defined(_MACOSX) || defined(_LINUX) || defined(TARGET_SUBPLATFORM_IPHONE) || defined(TARGET_SUBPLATFORM_ANDROID) || defined(_MAC_SERVER)
	va_start(t_args, p_format);
	t_count = vsnprintf(nil, 0, p_format, t_args);
	va_end(t_args);
#else
#error "Implement MCCStringAppendFormat"
#endif

	uint32_t t_old_length;
	t_old_length = MCCStringLength(x_string);
	
	if (!MCMemoryReallocate(x_string, t_old_length + t_count + 1, x_string))
		return false;
	
	va_start(t_args, p_format);
	vsprintf(x_string + t_old_length, p_format, t_args);
	va_end(t_args);
	
	return true;
}

void MCCStringFree(char *p_string)
{
	MCMemoryDeallocate(p_string);
}

uint32_t MCCStringLength(const char *p_string)
{
	if (p_string == nil)
		return 0;
	return strlen(p_string);
}

bool MCCStringEqual(const char *p_left, const char *p_right)
{
	return strcmp(p_left, p_right) == 0;
}

bool MCCStringEqualCaseless(const char *p_left, const char *p_right)
{
#if defined(_WINDOWS) || defined(_WINDOWS_SERVER)
	return _stricmp(p_left, p_right) == 0;
#else
	return strcasecmp(p_left, p_right) == 0;
#endif
}

bool MCCStringEqualSubstring(const char *p_left, const char *p_right, index_t p_length)
{
	return strncmp(p_left, p_right, p_length) == 0;
}

bool MCCStringEqualSubstringCaseless(const char *p_left, const char *p_right, index_t p_length)
{
#if defined(_WINDOWS) || defined(_WINDOWS_SERVER)
	return _strnicmp(p_left, p_right, p_length) == 0;
#else
	return strncasecmp(p_left, p_right, p_length) == 0;
#endif
}

bool MCCStringBeginsWith(const char *p_string, const char *p_prefix)
{
	return strlen(p_string) >= strlen(p_prefix) && strncmp(p_string, p_prefix, strlen(p_prefix)) == 0;
}

bool MCCStringBeginsWithCaseless(const char *p_string, const char *p_prefix)
{
	return strlen(p_string) >= strlen(p_prefix) && MCCStringEqualSubstringCaseless(p_string, p_prefix, strlen(p_prefix));
}

bool MCCStringEndsWith(const char *p_string, const char *p_suffix)
{
	return strlen(p_string) >= strlen(p_suffix) && strcmp(p_string + strlen(p_string) - strlen(p_suffix), p_suffix) == 0;
}

bool MCCStringEndsWithCaseless(const char *p_string, const char *p_suffix)
{
	return strlen(p_string) >= strlen(p_suffix) && MCCStringEqualCaseless(p_string + strlen(p_string) - strlen(p_suffix), p_suffix);
}

bool MCCStringSplit(const char *p_string, char p_split_char, char**& r_elements, uint32_t& r_element_count)
{
	bool t_success;
	t_success = true;

	char **t_elements;
	t_elements = nil;
	
	uint32_t t_element_count;
	t_element_count = 0;
	
	if (p_string != nil)
	{
		const char *t_ptr;
		t_ptr = p_string;
		
		const char *t_next;
		t_next = p_string;
		
		while(t_success && (t_next != nil))
		{
			t_next = strchr(t_ptr, p_split_char);
			
			// Add a new element to the array, and clone the substring
			t_success = MCMemoryResizeArray(t_element_count + 1, t_elements, t_element_count);
			if (t_success)
				t_success = MCCStringCloneSubstring(t_ptr, (t_next == nil) ? MCCStringLength(t_ptr) : t_next - t_ptr, t_elements[t_element_count - 1]);
			t_ptr = t_next + 1;
		}
	}
	
	if (t_success)
	{
		r_elements = t_elements;
		r_element_count = t_element_count;
	}
	else
	{
		for(uint32_t i = 0; i < t_element_count; i++)
			MCCStringFree(t_elements[i]);
		MCMemoryDeleteArray(t_elements);
	}
	
	return t_success;
}

void MCCStringArrayFree(char **p_strings, uint32_t p_count)
{
	if (p_strings == nil)
		return;

	for(uint32_t i = 0; i < p_count; i++)
		MCCStringFree(p_strings[i]);
	MCMemoryDeleteArray(p_strings);
}


bool MCCStringToUnicode(const char *p_string, unichar_t*& r_unicode_string)
{
	uint32_t t_cstring_length;
	t_cstring_length = MCCStringLength(p_string);
	
	uint32_t t_length;
	t_length = UTF8ToUnicode(p_string, t_cstring_length, nil, 0) / 2;
	
	if (!MCMemoryNewArray(t_length + 1, r_unicode_string))
		return false;
	
	t_length = UTF8ToUnicode(p_string, t_cstring_length, (uint16_t *)r_unicode_string, t_length * 2) / 2;
	r_unicode_string[t_length] = '\0';
	
	return true;
}

bool MCCStringFromUnicodeSubstring(const unichar_t *p_chars, uint32_t p_char_count, char*& r_string)
{
	uint32_t t_length;
	t_length = UnicodeToUTF8((const uint16_t *)p_chars, p_char_count * 2, nil, 0);
	
	if (!MCMemoryNewArray(t_length + 1, r_string))
		return false;

	t_length = UnicodeToUTF8((const uint16_t *)p_chars, p_char_count * 2, r_string, t_length);
	r_string[t_length] = '\0';
	
	return true;
}

bool MCCStringFromUnicode(const unichar_t *p_unicode_string, char*& r_string)
{
	if (NULL == p_unicode_string)
		return false;

	uint32_t t_wstring_length;
	t_wstring_length = 0;
	while(p_unicode_string[t_wstring_length] != 0)
		t_wstring_length++;
	
	return MCCStringFromUnicodeSubstring(p_unicode_string, t_wstring_length, r_string);
}
	
////////////////////////////////////////////////////////////////////////////////

// Convert the given UTF-8 string to Unicode. Both counts are in bytes.
// Returns the number of bytes used.
int32_t UTF8ToUnicode(const char *p_src, int32_t p_src_count, uint16_t *p_dst, int32_t p_dst_count)
{
	int32_t t_made;
	t_made = 0;
	
	for(;;)
	{
		if (p_src_count == 0)
			break;
		
		uint32_t t_consumed;
		t_consumed = 0;
		
		uint32_t t_codepoint;
		if ((p_src[0] & 0x80) == 0)
		{
			t_codepoint = p_src[0];
			t_consumed = 1;
		}
		else if ((p_src[0] & 0x40) == 0)
		{
			// This is an error
		}
		else if ((p_src[0] & 0x20) == 0)
		{
			if (p_src_count >= 2)
			{
				t_codepoint = (p_src[0] & 0x1f) << 6;
				if ((p_src[1] & 0xc0) == 0x80)
				{
					t_codepoint |= (p_src[1] & 0x3f);
					t_consumed = 2;
				}
			}
		}
		else if ((p_src[0] & 0x10) == 0)
		{
			if (p_src_count >= 3)
			{
				t_codepoint = (p_src[0] & 0x0f) << 12;
				if ((p_src[1] & 0xc0) == 0x80)
				{
					t_codepoint |= (p_src[1] & 0x3f) << 6;
					if ((p_src[2] & 0xc0) == 0x80)
					{
						t_codepoint |= (p_src[2] & 0x3f);
						t_consumed = 3;
					}
				}
			}
		}
		else if ((p_src[0] & 0x08) == 0)
		{
			if (p_src_count >= 4)
			{
				t_codepoint = (p_src[0] & 0x07) << 18;
				if ((p_src[1] & 0xc0) == 0x80)
				{
					t_codepoint |= (p_src[1] & 0x3f) << 12;
					if ((p_src[2] & 0xc0) == 0x80)
					{
						t_codepoint |= (p_src[2] & 0x3f) << 6;
						if ((p_src[3] & 0xc0) == 0x80)
						{
							t_codepoint |= p_src[3] & 0x3f;
							t_consumed = 4;
						}
					}
				}
			}	
		}
		
		if (t_consumed != 0)
		{
			if (t_codepoint < 65536)
			{
				if (p_dst_count != 0)
				{
					if ((t_made + 1) * 2 > p_dst_count)
						break;
					
					p_dst[t_made] = t_codepoint;
					
				}
				t_made += 1;
			}
			else
			{
				if (p_dst_count != 0)
				{
					if ((t_made + 2) * 2 > p_dst_count)
						break;
					
					t_codepoint -= 0x10000;
					
					p_dst[t_made + 0] = 0xD800 + (t_codepoint >> 10);
					p_dst[t_made + 1] = 0xDC00 + (t_codepoint & 0x03ff);
				}
				
				t_made += 2;
			}
		}
		else
			t_consumed = 1;
		
		p_src += t_consumed;
		p_src_count -= t_consumed;
	}
	
	return t_made * 2;
}

// Converts the given UTF-16 string to UTF-8. Both counts are in bytes.
// Returns the number of bytes generated.
int32_t UnicodeToUTF8(const uint16_t *p_src, int32_t p_src_count, char *p_dst, int32_t p_dst_count)
{
	int32_t t_made;
	t_made = 0;
	
	for(;;)
	{
		if (p_src_count < 2)
			break;
		
		uint32_t t_codepoint;
		t_codepoint = p_src[0];
		if (t_codepoint < 0xD800 ||
			t_codepoint >= 0xDC00 ||
			p_src_count < 4 ||
			p_src[1] < 0xDC00 ||
			p_src[1] >= 0xE000)
		{
			p_src_count -= 2;
			p_src += 1;
		}
		else
		{
			t_codepoint = 0x10000 + ((t_codepoint - 0xD800) << 10) + (p_src[1] - 0xDC00);
			p_src_count -= 4;
			p_src += 2;
		}
		
		if (t_codepoint < 128)
		{
			if (p_dst_count != 0)
			{
				if (t_made + 1 > p_dst_count)
					break;
				
				p_dst[t_made] = t_codepoint;
			}
			
			t_made += 1;
		}
		else if (t_codepoint < 0x0800)
		{
			if (p_dst_count != 0)
			{
				if (t_made + 2 > p_dst_count)
					break;
				
				p_dst[t_made + 0] = 0xc0 | (t_codepoint >> 6);
				p_dst[t_made + 1] = 0x80 | (t_codepoint & 0x3f);
			}
			
			t_made += 2;
		}
		else if (t_codepoint < 0x10000)
		{
			if (p_dst_count != 0)
			{
				if (t_made + 3 > p_dst_count)
					break;
				
				p_dst[t_made + 0] = 0xe0 | (t_codepoint >> 12);
				p_dst[t_made + 1] = 0x80 | ((t_codepoint >> 6) & 0x3f);
				p_dst[t_made + 2] = 0x80 | (t_codepoint & 0x3f);
			}
			
			t_made += 3;
		}
		else
		{
			if (p_dst_count != 0)
			{
				if (t_made + 4 > p_dst_count)
					break;
				
				p_dst[t_made + 0] = 0xf0 | (t_codepoint >> 18);
				p_dst[t_made + 1] = 0x80 | ((t_codepoint >> 12) & 0x3f);
				p_dst[t_made + 2] = 0x80 | ((t_codepoint >> 6) & 0x3f);
				p_dst[t_made + 3] = 0x80 | (t_codepoint & 0x3f);
			}
			
			t_made += 4;
		}
	}
	
	return t_made;
}

bool MCCStringTokenize(const char *p_string, char**& r_elements, uint32_t& r_element_count)
{
	bool t_success;
	t_success = true;

	char **t_elements;
	t_elements = nil;
	
	uint32_t t_element_count;
	t_element_count = 0;
	
	if (p_string != nil)
	{
		const char *t_ptr;
		t_ptr = p_string;
		
		const char *t_token;
		t_token = nil;
		
		while(t_success)
		{
			// Skip spaces
			while(*t_ptr == ' ')
				t_ptr += 1;
			
			t_token = t_ptr;
			
			// If we find a quote, start a quoted token, else loop until whitespace
			if (*t_ptr == '"')
			{
				t_ptr += 1;
				while(*t_ptr != '\0' && *t_ptr != '"')
					t_ptr += 1;
				
				if (*t_ptr == '"')
					t_ptr += 1;
			}
			else
			{
				while(*t_ptr != '\0' && *t_ptr != ' ')
					t_ptr += 1;
			}
			
			// If ptr and token are the same, we have exhausted the string
			if (t_ptr == t_token)
				break;
			
			// Add a new element to the array, and clone the substring
			t_success = MCMemoryResizeArray(t_element_count + 1, t_elements, t_element_count);
			if (t_success)
				t_success = MCCStringCloneSubstring(t_token, t_ptr - t_token, t_elements[t_element_count - 1]);
		}
	}
	
	if (t_success)
	{
		r_elements = t_elements;
		r_element_count = t_element_count;
	}
	else
	{
		for(uint32_t i = 0; i < t_element_count; i++)
			MCCStringFree(t_elements[i]);
		MCMemoryDeleteArray(t_elements);
	}
	
	return t_success;
}

bool MCCStringFirstIndexOf(const char *p_string, char p_search, uint32_t &r_index)
{
	if (p_string == nil)
		return false;
	
	const char *t_position;
	t_position = strchr(p_string, p_search);
	if (t_position != nil)
	{
		r_index  = t_position - p_string;
		return true;
	}
	else
		return false;
}

bool MCCStringFirstIndexOf(const char *p_string, const char *p_search, uint32_t &r_index)
{
	if (p_string == nil)
		return false;
	
	const char *t_position;
	t_position = strstr(p_string, p_search);
	if (t_position != nil)
	{
		r_index  = t_position - p_string;
		return true;
	}
	else
		return false;
}

bool MCCStringLastIndexOf(const char *p_string, char p_search, uint32_t &r_index)
{
	if (p_string == nil)
		return false;
	
	const char *t_position;
	t_position = strrchr(p_string, p_search);
	if (t_position != nil)
	{
		r_index  = t_position - p_string;
		return true;
	}
	else
		return false;
}

bool MCCStringLastIndexOf(const char *p_string, const char *p_search, uint32_t &r_index)
{
	if (p_string == nil)
		return false;
	
	const char *t_position;
	t_position = strstr(p_string, p_search);
	if (t_position != nil)
	{
		const char *t_next;
		while ((t_next = strstr(t_position + 1, p_search)) != nil)
			t_position = t_next;
	}
	if (t_position != nil)
	{
		r_index  = t_position - p_string;
		return true;
	}
	else
		return false;
}

bool MCCStringContains(const char *p_haystack, const char *p_needle)
{
	return strstr(p_haystack, p_needle) != nil;
}

bool MCCStringCombine(const char * const *p_elements, uint32_t p_element_count, char p_separator, char*& r_string)
{
	uint32_t t_length;
	t_length = 0;
	for(uint32_t i = 0; i < p_element_count; i++)
		t_length += MCCStringLength(p_elements[i]) + 1;
	
	char *t_string;
	if (!MCMemoryNewArray(t_length, t_string))
		return false;
	
	char *t_ptr;
	t_ptr = t_string;
	for(uint32_t i = 0; i < p_element_count; i++)
	{
		if (i > 0)
			*t_ptr++ = p_separator;
		
		uint32_t t_element_length;
		t_element_length = MCCStringLength(p_elements[i]);
		MCMemoryCopy(t_ptr, p_elements[i], t_element_length);
		t_ptr += t_element_length;
	}
	*t_ptr = '\0';
	
	r_string = t_string;
    
	return true;
}

bool MCCStringIsEmpty(const char *p_string)
{
	return p_string == nil || *p_string == '\0';
}

bool MCCStringIsInteger(const char *p_string)
{
	if (p_string == nil)
		return false;
	
	while(isdigit(*p_string++))
		;
	
	if (*p_string == '\0')
		return true;
	
	return false;
}

////////////////////////////////////////////////////////////////////////////////
