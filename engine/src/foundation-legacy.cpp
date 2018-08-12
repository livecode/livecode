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
#include "objectstream.h"
#include "util.h"

#include "stacksecurity.h"

#ifdef __MAC__
#include <CoreFoundation/CoreFoundation.h>
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
#elif defined(_MACOSX) || defined(_LINUX) || defined(TARGET_SUBPLATFORM_IPHONE) || defined(TARGET_SUBPLATFORM_ANDROID) || defined(_MAC_SERVER) || defined(_LINUX_SERVER) || defined(__EMSCRIPTEN__)
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
#elif defined(_MACOSX) || defined(_LINUX) || defined(TARGET_SUBPLATFORM_IPHONE) || defined(TARGET_SUBPLATFORM_ANDROID) || defined(_MAC_SERVER) || defined(_LINUX_SERVER) || defined(__EMSCRIPTEN__)
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
#elif defined(_MACOSX) || defined(_LINUX) || defined(TARGET_SUBPLATFORM_IPHONE) || defined(TARGET_SUBPLATFORM_ANDROID) || defined(_MAC_SERVER) || defined(_LINUX_SERVER) || defined(__EMSCRIPTEN__)
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

////////////////////////////////////////////////////////////////////////////////

struct MCList
{
	MCList *next;
};

void MCListPushBack(void*& x_list, void *p_element)
{
	if (x_list == nil)
	{
		x_list = p_element;
		return;
	}

	MCList *t_list, *t_element, *t_previous;
	t_list = static_cast<MCList *>(x_list);
	t_element = static_cast<MCList *>(p_element);
	for(t_previous = t_list; t_previous -> next != nil; t_previous = t_previous -> next)
		;
	t_previous -> next = t_element;
}

void *MCListPopFront(void*& x_list)
{
	void *t_element;
	t_element = x_list;
	x_list = static_cast<void **>(x_list)[0];
	return t_element;
}

void MCListPushFront(void*& x_list, void *p_element)
{
	if (x_list == nil)
	{
		x_list = p_element;
		return;
	}

	MCList *t_list, *t_element;
	t_list = static_cast<MCList *>(x_list);
	t_element = static_cast<MCList *>(p_element);
	t_element -> next = t_list;

	x_list = t_element;
}

void MCListRemove(void*& x_list, void *p_element)
{
	MCList *t_list, *t_element;
	t_list = static_cast<MCList *>(x_list);
	t_element = static_cast<MCList *>(p_element);

	if (t_list == t_element)
	{
		x_list = t_element -> next;
		return;
	}

	MCList *t_previous;
	for(t_previous = t_list; t_previous != nil && t_previous -> next != t_element; t_previous = t_previous -> next)
		;
	if (t_previous != nil)
		t_previous -> next = t_element -> next;
}

////////////////////////////////////////////////////////////////////////////////

bool MCValueConvertToStringForSave(MCValueRef self, MCStringRef& r_string)
{
	bool t_success;
	t_success = true;

	switch(MCValueGetTypeCode(self))
	{
	case kMCValueTypeCodeNull:
		r_string = MCValueRetain(kMCEmptyString);
		break;
	case kMCValueTypeCodeBoolean:
		r_string = MCValueRetain(self == kMCTrue ? kMCTrueString : kMCFalseString);
		break;
	case kMCValueTypeCodeName:
		r_string = MCValueRetain(MCNameGetString((MCNameRef)self));
		break;
	case kMCValueTypeCodeNumber:
		t_success = MCU_r8tos(MCNumberFetchAsReal(static_cast<MCNumberRef>(self)),
		                      8, 6, 0, r_string);
		break;
	case kMCValueTypeCodeString:
		t_success = MCStringCopy((MCStringRef)self, r_string);
		break;
    case kMCValueTypeCodeData:
        t_success = MCStringDecode((MCDataRef)self, kMCStringEncodingNative, false, r_string);
        break;
	case kMCValueTypeCodeArray:
		r_string = MCValueRetain(kMCEmptyString);
		break;
	default:
		MCAssert(false);
		break;
	}

	return t_success;
}

bool MCValueIsEmpty(MCValueRef p_value)
{
    // AL-2014-10-31: [[ Bug 13890 ]] An empty array is not necessarily equal to kMCEmptyArray,
    //  so we need to use MCArrayIsEmpty instead.
    return p_value == kMCNull ||
            p_value == kMCEmptyName ||
            (MCValueGetTypeCode(p_value) == kMCValueTypeCodeArray && MCArrayIsEmpty((MCArrayRef)p_value)) ||
            (MCValueGetTypeCode(p_value) == kMCValueTypeCodeString && MCStringIsEmpty((MCStringRef)p_value)) ||
            (MCValueGetTypeCode(p_value) == kMCValueTypeCodeName && MCNameIsEmpty((MCNameRef)p_value)) ||
            (MCValueGetTypeCode(p_value) == kMCValueTypeCodeData && MCDataIsEmpty((MCDataRef)p_value)) ||
            (MCValueGetTypeCode(p_value) == kMCValueTypeCodeList && MCListIsEmpty((MCListRef)p_value));
}

bool MCValueIsArray(MCValueRef p_value)
{
	return MCValueGetTypeCode(p_value) == kMCValueTypeCodeArray;
}

////////////////////////////////////////////////////////////////////////////////

bool MCStringCreateWithOldString(const MCString& p_old_string, MCStringRef& r_string)
{
	return MCStringCreateWithNativeChars((const char_t *)p_old_string . getstring(), p_old_string . getlength(), r_string);
}

bool MCStringIsEqualToOldString(MCStringRef p_string, const MCString& p_oldstring, MCCompareOptions p_options)
{
	return MCStringIsEqualToNativeChars(p_string, (const char_t *)p_oldstring . getstring(), p_oldstring . getlength(), p_options);
}

bool MCStringToInteger(MCStringRef p_string, integer_t& r_integer)
{
	char *t_end;
	t_end = nil;
	
	integer_t t_value;
	t_value = strtol(MCStringGetCString(p_string), &t_end, 10);
	
	if (t_end != MCStringGetCString(p_string) + strlen(MCStringGetCString(p_string)))
		return false;
	
	r_integer = t_value;
	return true;
}

bool MCStringToDouble(MCStringRef p_string, double& r_real)
{
	char *t_end;
	t_end = nil;
	
    MCAutoStringRefAsCString t_string;
    t_string . Lock(p_string);
    
	double t_value;
	t_value = strtod(*t_string, &t_end);
	
	if (t_end != *t_string + strlen(*t_string))
		return false;
	
	r_real = t_value;
	return true;
}

MCString MCDataGetOldString(MCDataRef p_data)
{
	return MCString((const char *)MCDataGetBytePtr(p_data), MCDataGetLength(p_data));
}

#if defined(_MACOSX) || defined(TARGET_SUBPLATFORM_IPHONE)
bool MCCStringToCFString(const char *p_cstring, CFStringRef& r_cfstring)
{
	CFStringRef t_cfstring;
	t_cfstring = CFStringCreateWithBytes(kCFAllocatorDefault, (const UInt8 *)p_cstring, MCCStringLength(p_cstring), kCFStringEncodingUTF8, false);
	if (t_cfstring == nil)
		return false;
	
	r_cfstring = t_cfstring;
	return true;
}

bool MCCStringFromCFString(CFStringRef p_cfstring, char *&r_cstring)
{
	uint32_t t_buffer_size = CFStringGetMaximumSizeForEncoding(CFStringGetLength(p_cfstring), kCFStringEncodingUTF8) + 1;
	
	char *t_cstring = nil;
	bool t_success = MCMemoryAllocate(t_buffer_size, t_cstring);
	
	if (t_success)
		t_success = CFStringGetCString(p_cfstring, t_cstring, t_buffer_size, kCFStringEncodingUTF8);
	
	if (t_success)
		r_cstring = t_cstring;
	else
		MCMemoryDeallocate(t_cstring);
	
	return t_success;
}

bool MCCStringToNative(const char *p_string, char*& r_native_string)
{
	CFStringRef t_cfstring;
	if (!MCCStringToCFString(p_string, t_cfstring))
		return false;
	
	CFDataRef t_data;
	t_data = CFStringCreateExternalRepresentation(kCFAllocatorDefault, t_cfstring, kCFStringEncodingMacRoman, '?');
	
	uint32_t t_length;
	t_length = CFDataGetLength(t_data);
	
	const UInt8 *t_bytes;
	t_bytes = CFDataGetBytePtr(t_data);
	if (t_bytes[0] == 0xEF && t_bytes[1] == 0xBB && t_bytes[2] == 0xBF)
		t_bytes += 3, t_length -= 3;
	
	bool t_success;
	t_success = MCCStringCloneSubstring((const char *)t_bytes, t_length, r_native_string);
	CFRelease(t_data);
	CFRelease(t_cfstring);
	return t_success;
}

bool MCCStringFromNative(const char *p_native, char*& r_cstring)
{
	bool t_success;
	t_success = true;
	
	CFStringRef t_cfstring;
	t_cfstring = nil;
	if (t_success)
	{
		t_cfstring = CFStringCreateWithCStringNoCopy(kCFAllocatorDefault, p_native, kCFStringEncodingMacRoman, kCFAllocatorNull);
		if (t_cfstring == nil)
			t_success = false;
	}
	
	CFIndex t_cstring_length;
	char *t_buffer;
	t_buffer = nil;
	if (t_success)
	{
		CFStringGetBytes(t_cfstring, CFRangeMake(0, CFStringGetLength(t_cfstring)), kCFStringEncodingUTF8, 0, false, nil, 0, &t_cstring_length);
		t_success = MCMemoryNewArray(t_cstring_length + 1, t_buffer);
	}
	
	if (t_success)
	{
		CFStringGetBytes(t_cfstring, CFRangeMake(0, CFStringGetLength(t_cfstring)), kCFStringEncodingUTF8, 0, false, (UInt8 *)t_buffer, t_cstring_length, nil);
		t_buffer[t_cstring_length] = '\0';
		r_cstring = t_buffer;
	}
	
	if (t_cfstring != nil)
		CFRelease(t_cfstring);
	
	return t_success;
}

#elif defined(_WINDOWS)

bool MCCStringToNative(const char *p_string, char *&r_native)
{
	bool t_success = true;

	unichar_t *t_unistring = nil;
	char *t_native = nil;

	uint32_t t_string_len = MCCStringLength(p_string);

	if (t_success)
		t_success = MCCStringToUnicode(p_string, t_unistring);
	if (t_success)
		t_success = MCMemoryNewArray(t_string_len + 1, t_native);
	if (t_success)
		WideCharToMultiByte(1252, WC_NO_BEST_FIT_CHARS, t_unistring, -1, t_native, t_string_len + 1, "?", NULL);

	MCMemoryDeleteArray(t_unistring);

	if (t_success)
		r_native = t_native;

	return t_success;
}

bool MCCStringFromNative(const char *p_native, char*& r_cstring)
{
	bool t_success;
	t_success = true;

	// For us 'native' means Windows-1252 which has a 1-1 mapping to UTF-16
	// so we know the length we need...
	unichar_t *t_wstring;
	t_wstring = nil;
	if (t_success)
		t_success = MCMemoryNewArray(MCCStringLength(p_native) + 1, t_wstring);

	if (t_success)
	{
		MultiByteToWideChar(1252, 0, p_native, -1, t_wstring, MCCStringLength(p_native) + 1);
		t_success = MCCStringFromUnicode(t_wstring, r_cstring);
	}

	MCMemoryDeleteArray(t_wstring);

	return t_success;
}
#elif defined(_LINUX) || defined (_LINUX_SERVER)
bool MCCStringFromNativeSubstring(const char *p_string, uint32_t p_length, char*& r_cstring)
{
	char *t_native;
	if (!MCMemoryNewArray(p_length * 2 + 1, t_native))
		return false;

	const uint8_t *t_string;
	t_string = (const uint8_t *)p_string;

	uint32_t j;
	j = 0;
	for(uint32_t i = 0; i < p_length; i++)
		if (t_string[i] < 128)
			t_native[j++] = t_string[i];
		else
		{
			t_native[j++] = 0xc0 | (t_string[i] >> 6);
			t_native[j++] = 0x80 | (t_string[i] & 0x3f);
		}

	t_native[j] = '\0';

	r_cstring = t_native;

	return true;
}

bool MCCStringFromNative(const char *p_string, char*& r_cstring)
{
	return MCCStringFromNativeSubstring(p_string, MCCStringLength(p_string), r_cstring);
}
#endif

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

struct get_array_extent_context_t
{
	index_t minimum;
	index_t maximum;
};

static bool get_name_as_index(MCNameRef p_name, index_t& r_index)
{
    MCStringRef t_key;
    t_key = MCNameGetString(p_name);
    
    // AL-2015-01-05: [[ Bug 14303 ]] Don't treat keys of the form "01" as indices,
    //  since for example "01" and "1" are distinct array keys.
    if (MCStringGetLength(t_key) != 1 && MCStringGetCodepointAtIndex(t_key, 0) == '0')
        return false;
    
    // SN-2015-05-15: [[ Bug 15457 ]] Store the string-to-number conversion.
    double t_double_index;
    if (MCStringGetNumericValue(t_key, t_double_index))
    {
        r_index = (index_t)t_double_index;
        return true;
    }
    
	char *t_end;
	index_t t_index;
    
    // AL-2014-05-15: [[ Bug 12203 ]] Don't nativize array name when checking
    //  for a sequential array.
    MCAutoStringRefAsCString t_cstring;
    t_cstring . Lock(t_key);
    
	t_index = strtol(*t_cstring, &t_end, 10);
	if (*t_end == '\0')
	{
        // SN-2015-06-15: [[ Bug 15457 ]] Store the converted value - improve
        //  speed if repeating several times over the elements of a array.
        MCStringSetNumericValue(t_key, t_index);
		r_index = t_index;
		return true;
	}
	return false;
}

static bool get_array_extent(void *context, MCArrayRef p_array, MCNameRef p_key, MCValueRef p_value)
{
	get_array_extent_context_t *ctxt;
	ctxt = (get_array_extent_context_t *)context;

	index_t t_index;
	if (!get_name_as_index(p_key, t_index))
		return false;

	ctxt -> minimum = MCMin(ctxt -> minimum, t_index);
	ctxt -> maximum = MCMax(ctxt -> maximum, t_index);

	return true;
}

bool MCArrayIsSequence(MCArrayRef self)
{
    int32_t t_start_index;
    
    // IsSequence returns true if the sequence starts with 1 only
    return MCArrayIsNumericSequence(self, t_start_index) && t_start_index == 1;
}

bool MCArrayIsNumericSequence(MCArrayRef self, int32_t &r_start_index)
{
    get_array_extent_context_t ctxt;
    ctxt . minimum = INDEX_MAX;
    ctxt . maximum = INDEX_MIN;
    
    if (MCArrayApply(self, get_array_extent, &ctxt) &&
            (ctxt . maximum - ctxt . minimum + 1) == MCArrayGetCount(self))
    {
        r_start_index = ctxt . minimum;
        return true;
    }
    
    return false;
}

static bool list_keys(void *p_context, MCArrayRef p_array, MCNameRef p_key, MCValueRef p_value)
{
	MCListRef t_list = (MCListRef)p_context;

	return MCListAppend(t_list, p_key);
}

bool MCArrayListKeys(MCArrayRef self, char p_delimiter, MCStringRef& r_list)
{
	MCAutoListRef t_list;
	if (!MCListCreateMutable(p_delimiter, &t_list))
		return false;

	return MCArrayApply(self, list_keys, *t_list) &&
		MCListCopyAsString(*t_list, r_list);
}

struct measure_array_context_t
{
	uint32_t size;
	bool nested_only;
};

static uint32_t measure_array_entry(MCNameRef p_key, MCValueRef p_value)
{
	// Size of record is:
	//   1 byte - identifier
	//   4 bytes - length not including identifier byte
	//   * bytes - C string of key

	uint32_t t_size;
	t_size = 1 + 4 + MCCStringLength(MCStringGetCString(MCNameGetString(p_key))) + 1;
	switch(MCValueGetTypeCode(p_value))
	{
	case kMCValueTypeCodeNull:
		break;
	case kMCValueTypeCodeBoolean:
		t_size += 4 + (p_value == kMCTrue ? 4 : 5);
		break;
	case kMCValueTypeCodeString:
		t_size += 4 + MCStringGetLength((MCStringRef)p_value);
		break;
	case kMCValueTypeCodeName:
		t_size += 4 + MCStringGetLength(MCNameGetString((MCNameRef)p_value));
        break;
	case kMCValueTypeCodeNumber:
		t_size += 8;
		break;
	case kMCValueTypeCodeArray:
		t_size += MCArrayMeasureForStreamLegacy((MCArrayRef)p_value, false);
		break;
    case kMCValueTypeCodeData:
        t_size += 4 + MCDataGetLength((MCDataRef)p_value);
        break;
	default:
		MCAssert(false);
		break;
	}

	return t_size;
}

static bool measure_array(void *p_context, MCArrayRef p_array, MCNameRef p_key, MCValueRef p_value)
{
	measure_array_context_t *ctxt;
	ctxt = (measure_array_context_t *)p_context;

	if (ctxt -> nested_only &&
		MCValueGetTypeCode(p_value) != kMCValueTypeCodeArray)
		return true;

	ctxt -> size += measure_array_entry(p_key, p_value);

	return true;
}

uint32_t MCArrayMeasureForStreamLegacy(MCArrayRef self, bool p_nested_only)
{
	measure_array_context_t ctxt;
	ctxt . size = 0;
	ctxt . nested_only = p_nested_only;
	
	MCArrayApply(self, measure_array, &ctxt);

	// If we are measuring nested elements only, and there are none, we measure
	// as nothing (otherwise there is a header byte plus count).
	if (!ctxt . nested_only || ctxt . size != 0)
		ctxt . size += 5;

	return ctxt . size;
}

static bool is_array_nested(void *p_context, MCArrayRef p_array, MCNameRef p_key, MCValueRef p_value)
{
	if (MCValueGetTypeCode(p_value) == kMCValueTypeCodeArray)
		return false;

	return true;
}

bool MCArrayIsNestedLegacy(MCArrayRef self)
{
	// The 'is_array_nested' method terminates when it encounters an array value;
	// therefore if the array is nested, it will return false.
	return !MCArrayApply(self, is_array_nested, nil);
}

IO_stat MCArrayLoadFromHandleLegacy(MCArrayRef self, IO_handle p_stream)
{
	IO_stat t_stat;
	t_stat = IO_NORMAL;

	uint32_t t_nfilled;
	if (t_stat == IO_NORMAL)
		t_stat = IO_read_uint4(&t_nfilled, p_stream);

	bool t_decrypt, t_large, t_encrypted;
	if (t_stat == IO_NORMAL)
	{
		t_decrypt = (t_nfilled & SAVE_ENCRYPTED) != 0;
		t_large = (t_nfilled & SAVE_LARGE) != 0;
		t_nfilled &= ~(SAVE_ENCRYPTED | SAVE_LARGE);
		
		t_encrypted = MCStackSecurityIsIOEncryptionEnabled();
		MCStackSecuritySetIOEncryptionEnabled(t_decrypt);
	}

	if (t_stat == IO_NORMAL)
	{
		uint32_t t_size;
		t_size = t_large ? 4 : 2;
		
		uint32_t t_nprops;
		t_nprops = t_nfilled;
		while(t_nprops-- && t_stat == IO_NORMAL)
		{
			MCNewAutoNameRef t_name;
			MCAutoValueRef t_value;

			char *t_key = nil;
			uint32_t t_length = 0;
			
			// IM-2013-04-04 loadkeys() would previously translate the key string,
			// so we pass p_translate = true.
			t_stat = IO_read_string_legacy_full(t_key, t_length, p_stream, 1, true, true);
			if (t_stat == IO_NORMAL)
				if (!MCNameCreateWithNativeChars((const char_t*)t_key, t_length, &t_name))
					t_stat = IO_ERROR;
			
			MCMemoryDeallocate(t_key);
			
			char *t_buffer = nil;
			if (t_stat == IO_NORMAL)
				t_stat = IO_read_string_legacy_full(t_buffer, t_length, p_stream, t_size, false, false);
			
			if (t_stat == IO_NORMAL)
				if (!MCStringCreateWithNativeChars((const char_t *)t_buffer, t_length, (MCStringRef&)&t_value))
					t_stat = IO_ERROR;
			
			MCMemoryDeallocate(t_buffer);

			if (t_stat == IO_NORMAL)
				if (!MCArrayStoreValue(self, true, *t_name, *t_value))
					t_stat = IO_ERROR;
		}
		
		MCStackSecuritySetIOEncryptionEnabled(t_encrypted);
	}

	return t_stat;
}

IO_stat MCArrayLoadFromStreamLegacy(MCArrayRef self, MCObjectInputStream& p_stream)
{
	IO_stat t_stat;
	t_stat = IO_NORMAL;

	uint32_t t_nfilled;
	t_nfilled = 0;
	if (t_stat == IO_NORMAL)
		t_stat = p_stream . ReadU32(t_nfilled);

	//if (t_stat == IO_NORMAL && t_nfilled == 0)
	//	return t_stat;

	while(t_stat == IO_NORMAL)
	{
		uint8_t t_type;
		if (t_stat == IO_NORMAL)
			t_stat = p_stream . ReadU8(t_type);
		if (t_stat == IO_NORMAL && t_type == 0)
			break;

		uint32_t t_length;
		if (t_stat == IO_NORMAL)
			t_stat = p_stream . ReadU32(t_length);

		MCAutoStringRef t_key;
		if (t_stat == IO_NORMAL)
			t_stat = p_stream . ReadStringRefNew(&t_key, false);
        
		MCNewAutoNameRef t_key_name;
		if (t_stat == IO_NORMAL)
		{
			if (!MCStringIsEmpty(*t_key))
			{
				if (!MCNameCreate(*t_key, &t_key_name))
					t_stat = IO_ERROR;
			}
			else
				t_key_name = kMCEmptyName;
		}

		MCValueRef t_value;
		t_value = nil;
		if (t_stat == IO_NORMAL)
		{
			switch((Value_format)(t_type - 1))
			{
			case VF_UNDEFINED:
				t_value = kMCEmptyString;
				break;
			case VF_STRING:
				{
					uint32_t t_string_length;
					t_stat = p_stream . ReadU32(t_string_length);

					char *t_string;
					t_string = nil;
					if (t_stat == IO_NORMAL)
						if (!MCMemoryNewArray(t_string_length, t_string))
							t_stat = IO_ERROR;

					if (t_stat == IO_NORMAL)
						t_stat = p_stream . Read(t_string, t_string_length);

					if (t_stat == IO_NORMAL)
					{
                        // AL-2014-04-14: [[ Bug 11989 ]] Prevent memory leak in array loading.
						MCStringRef t_string_val;
                        t_string_val = nil;
						if (MCStringCreateWithNativeChars((const char_t *)t_string, t_string_length, t_string_val))
							t_value = t_string_val;
						else
                        {
							t_stat = IO_ERROR;
                            MCValueRelease(t_string_val);
                        }
					}

					MCMemoryDeleteArray(t_string);
				}
				break;
			case VF_BOTH:
			case VF_NUMBER:
				{
					double t_real;
					t_stat = p_stream . ReadFloat64(t_real);
					if (t_stat == IO_NORMAL)
					{
                        // AL-2014-04-14: [[ Bug 11989 ]] Prevent memory leak in array loading.
						MCNumberRef t_number;
                        t_number = nil;
						if (MCNumberCreateWithReal(t_real, t_number))
							t_value = t_number;
						else
                        {
							t_stat = IO_ERROR;
                            MCValueRelease(t_number);
                        }
					}
				}
				break;
			case VF_ARRAY:
				{
					MCArrayRef t_array;
					t_array = nil;
					if (!MCArrayCreateMutable(t_array))
						t_stat = IO_ERROR;
					if (t_stat == IO_NORMAL)
						t_stat = MCArrayLoadFromStreamLegacy(t_array, p_stream);
					if (t_stat == IO_NORMAL)
						t_value = t_array;
					else
						MCValueRelease(t_array);
				}
				break;
            // AL-2014-05-23: [[ Bug 12493 ]] Prevent crash cause by bad type data while decoding array
            default:
                t_stat = IO_ERROR;
			}
		}

		if (t_stat == IO_NORMAL)
			if (!MCArrayStoreValue(self, true, *t_key_name, t_value))
				t_stat = IO_ERROR;

		MCValueRelease(t_value);
	}

	return t_stat;
}

struct array_info_context_t
{
	bool is_large;
	uindex_t non_array_count;
};

static bool get_array_info(void *p_context, MCArrayRef p_array, MCNameRef p_key, MCValueRef p_value)
{
	array_info_context_t *ctxt;
	ctxt = (array_info_context_t *)p_context;

	if (MCValueGetTypeCode(p_value) == kMCValueTypeCodeString &&
		MCStringGetLength((MCStringRef)p_value) > MAXUINT2)
		ctxt -> is_large = true;
    
    // MW-2014-10-21: [[ Bug 13732 ]] Make sure we check the length of dataref's too to
    //   ensure we mark the array as large if necessary.
    if (MCValueGetTypeCode(p_value) == kMCValueTypeCodeData &&
        MCDataGetLength((MCDataRef)p_value) > MAXUINT2)
        ctxt -> is_large = true;

	if (MCValueGetTypeCode(p_value) != kMCValueTypeCodeArray)
		ctxt -> non_array_count += 1;

	return true;
}

struct save_array_to_handle_context_t
{
	bool is_large;
	IO_handle stream;
};

static bool save_array_to_handle(void *p_context, MCArrayRef p_array, MCNameRef p_key, MCValueRef p_value)
{
	save_array_to_handle_context_t *ctxt;
	ctxt = (save_array_to_handle_context_t *)p_context;

	IO_handle t_stream;
	t_stream = ctxt -> stream;

	if (MCValueGetTypeCode(p_value) == kMCValueTypeCodeArray)
		return true;

	IO_stat t_stat;
	t_stat = IO_NORMAL;

	if (t_stat == IO_NORMAL)
	{
		MCAutoPointer<char> t_key_string;
        
        // SN-2015-04-23: [[ Bug 15258 ]] We don't want to nativise the string,
        //  but rather to get a C-String copy of it.
        if (!MCStringConvertToCString(MCNameGetString(p_key), &t_key_string))
            return false;
        
		// IM-2013-04-04: [[ BZ 10811 ]] pre 6.0 versions of loadkeys() expect
		// a null-terminated string of non-zero length (including null),
		// but IO_write_string() writes a single zero byte for an empty string
		// so we need a special case here.
		if (*t_key_string == nil || (*t_key_string)[0] == '\0')
		{
			// write length + null
			t_stat = IO_write_uint1(1, t_stream);
			// write null
			if (t_stat == IO_NORMAL)
				t_stat = IO_write_uint1(0, t_stream);
		}
		else
			t_stat = IO_write_cstring_legacy(*t_key_string, t_stream, 1);
	}

	MCAutoStringRef t_string;
	if (t_stat == IO_NORMAL)
		if (!MCValueConvertToStringForSave(p_value, &t_string))
			t_stat = IO_ERROR;

	if (t_stat == IO_NORMAL)
	{
		uint32_t t_size;
		if (ctxt -> is_large)
			t_size = 4;
		else
			t_size = 2;
		
        // SN-2015-04-23: [[ Bug 15258 ]] We don't want to nativise the string,
        //  but rather to get a C-String copy of it.
        MCAutoPointer<char_t> t_c_string;
        uindex_t t_length;
        
        // SN-2015-06-03: [[ Bug 15455 ]] The length can be different from a
        //  C-string length, as image for instance can be stored as custom props
        if (!MCStringConvertToNative(*t_string, &t_c_string, t_length))
            return false;
        
		t_stat = IO_write_string_legacy_full(MCString((const char*)*t_c_string, (uint4)t_length), t_stream, t_size, false);
	}
	
	return t_stat == IO_NORMAL;
}

IO_stat MCArraySaveToHandleLegacy(MCArrayRef self, IO_handle p_stream)
{
	array_info_context_t t_info;
	t_info . is_large = false;
	t_info . non_array_count = 0;
	MCArrayApply(self, get_array_info, &t_info);

	uint32_t t_writable_nfilled;
	t_writable_nfilled = t_info . non_array_count;
	if (MCStackSecurityIsIOEncrypted())
		t_writable_nfilled |= SAVE_ENCRYPTED;
	if (t_info . is_large)
		t_writable_nfilled |= SAVE_LARGE;
	
	save_array_to_handle_context_t t_save;
	t_save . is_large = t_info . is_large;
	t_save . stream = p_stream;

	IO_stat t_stat;
	t_stat = IO_NORMAL;
	if (t_stat == IO_NORMAL)
		t_stat = IO_write_uint4(t_writable_nfilled, p_stream);

	if (!MCArrayApply(self, save_array_to_handle, &t_save))
		t_stat = IO_ERROR;

	return t_stat;
}

struct save_array_to_stream_context_t
{
	bool nested_only;
	MCObjectOutputStream *stream;
};

static bool save_array_to_stream(void *p_context, MCArrayRef p_array, MCNameRef p_key, MCValueRef p_value)
{
	save_array_to_stream_context_t *ctxt;
	ctxt = (save_array_to_stream_context_t *)p_context;

	if (ctxt -> nested_only && MCValueGetTypeCode(p_value) != kMCValueTypeCodeArray)
		return true;

	MCStringRef t_str_value;
    MCAutoStringRef t_string_buffer;
	unsigned int t_type;
	switch(MCValueGetTypeCode(p_value))
	{
	case kMCValueTypeCodeNull:
		t_type = VF_UNDEFINED;
		t_str_value = kMCEmptyString;
		break;
	case kMCValueTypeCodeBoolean:
		t_type = VF_STRING;
		t_str_value = p_value == kMCTrue ? kMCTrueString : kMCFalseString;
		break;
	case kMCValueTypeCodeName:
		t_type = VF_STRING;
		t_str_value = MCNameGetString((MCNameRef)p_value);
		break;
	case kMCValueTypeCodeString:
		t_type = VF_STRING;
		t_str_value = (MCStringRef)p_value;
		break;
    case kMCValueTypeCodeData:
        t_type = VF_STRING;
        MCStringDecode((MCDataRef)p_value, kMCStringEncodingNative, false, &t_string_buffer);
        t_str_value = *t_string_buffer;
        break;
	case kMCValueTypeCodeNumber:
		t_type = VF_NUMBER;
		t_str_value = nil;
		break;
	case kMCValueTypeCodeArray:
        if (MCArrayGetCount((MCArrayRef)p_value) != 0)
        {
            t_type = VF_ARRAY;
            t_str_value = nil;
        }
        else
        {
            t_type = VF_STRING;
            t_str_value = kMCEmptyString;
        }
		break;
    default:
        MCUnreachableReturn(false);
	}

	IO_stat t_stat;
	t_stat = ctxt -> stream -> WriteU8(t_type + 1);
	if (t_stat == IO_NORMAL)
		t_stat = ctxt -> stream -> WriteU32(measure_array_entry(p_key, p_value) - 1);
	if (t_stat == IO_NORMAL)
		t_stat = ctxt -> stream -> WriteStringRefNew(MCNameGetString(p_key), false);
	if (t_stat == IO_NORMAL)
		switch((Value_format)t_type)
		{
		case VF_UNDEFINED:
			break;
        case VF_STRING:
        {
            // SN-2014-08-05: [[ Bug 13050 ]] VF_STRING is used for binary as well, so that
            //  we can't use strlen() to get the size of the string.
            uindex_t t_length;
            char_t *t_cstring;
            /* UNCHECKED */ MCStringConvertToNative(t_str_value, t_cstring, t_length);
			t_stat = ctxt -> stream -> WriteU32(t_length);
			if (t_stat == IO_NORMAL)
            {
				t_stat = ctxt -> stream -> Write(t_cstring, t_length);
            }
        }
			break;
		case VF_NUMBER:
			t_stat = ctxt -> stream -> WriteFloat64(MCNumberFetchAsReal((MCNumberRef)p_value));
			break;
		case VF_ARRAY:
			t_stat = MCArraySaveToStreamLegacy((MCArrayRef)p_value, false, *(ctxt -> stream));
			break;
		default:
			MCAssert(false);
			break;
		}

	return t_stat == IO_NORMAL;
}

IO_stat MCArraySaveToStreamLegacy(MCArrayRef self, bool p_nested_only, MCObjectOutputStream& p_stream)
{
	IO_stat t_stat;
	t_stat = IO_NORMAL;

	if (t_stat == IO_NORMAL)
		t_stat = p_stream . WriteU32(MCArrayGetCount(self));

	if (t_stat == IO_NORMAL)
	{
		save_array_to_stream_context_t t_save;
		t_save . nested_only = p_nested_only;
		t_save . stream = &p_stream;
		if (!MCArrayApply(self, save_array_to_stream, &t_save))
			t_stat = IO_ERROR;
	}

	if (t_stat == IO_NORMAL)
		t_stat = p_stream . WriteU8(0);

	return t_stat;
}

////////////////////////////////////////////////////////////////////////////////

bool MCListAppendCString(MCListRef self, const char *p_cstring)
{
	return MCListAppendNativeChars(self, (const char_t *)p_cstring, MCCStringLength(p_cstring));
}

bool MCListAppendOldString(MCListRef self, const MCString& p_oldstring)
{
	return MCListAppendNativeChars(self, (const char_t *)p_oldstring . getstring(), p_oldstring . getlength());
}

bool MCListAppendInteger(MCListRef self, integer_t p_value)
{
	char_t t_buffer[16];
	sprintf((char *)t_buffer, "%d", p_value);
	return MCListAppendNativeChars(self, t_buffer, strlen((char *)t_buffer));
}

bool MCListAppendUnsignedInteger(MCListRef self, uinteger_t p_value)
{
	char_t t_buffer[16];
	sprintf((char *)t_buffer, "%u", p_value);
	return MCListAppendNativeChars(self, t_buffer, strlen((char *)t_buffer));
}

////////////////////////////////////////////////////////////////////////////////

bool serialize_bytes(char *&r_stream, uint32_t &r_stream_size, uint32_t &r_offset, const void *p_data, uint32_t p_data_size)
{
	if (p_data_size == 0)
		return true;
	if (r_offset + p_data_size > r_stream_size)
	{
		char *t_stream;
		uint32_t t_size;
		t_size = r_offset + p_data_size;
		if (!MCMemoryReallocate(r_stream, t_size, t_stream))
			return false;
		r_stream = t_stream;
		r_stream_size = t_size;
	}
	MCMemoryCopy(r_stream + r_offset, p_data, p_data_size);
	r_offset += p_data_size;

	return true;
}

bool deserialize_bytes(const char *p_stream, uint32_t p_stream_size, uint32_t &r_offset, void *p_dest, uint32_t p_size)
{
	if (r_offset + p_size > p_stream_size)
		return false;

	MCMemoryCopy(p_dest, p_stream + r_offset, p_size);
	r_offset += p_size;
	return true;
}

bool serialize_uint32(char *&r_stream, uint32_t &r_stream_size, uint32_t &r_offset, uint32_t p_val)
{
	return serialize_bytes(r_stream, r_stream_size, r_offset, &p_val, sizeof(uint32_t));
}

bool deserialize_uint32(const char *p_stream, uint32_t p_stream_size, uint32_t &r_offset, uint32_t &r_val)
{
	return deserialize_bytes(p_stream, p_stream_size, r_offset, &r_val, sizeof(uint32_t));
}

bool serialize_data(char *&r_stream, uint32_t &r_stream_size, uint32_t &r_offset, const void *p_data, uint32_t p_data_size)
{
	if (serialize_uint32(r_stream, r_stream_size, r_offset, p_data_size) &&
		serialize_bytes(r_stream, r_stream_size, r_offset, p_data, p_data_size))
		return true;
	return false;
}

bool deserialize_data(const char *p_stream, uint32_t p_stream_size, uint32_t &r_offset, void *&r_data, uint32_t &r_size)
{
	uint32_t t_size = 0, t_data_size = 0;
	bool t_success = true;
	void *t_data = nil;
	t_success = deserialize_uint32(p_stream, p_stream_size, r_offset, t_data_size);
	if (t_success)
	{
		if (t_data_size == 0)
		{
			r_size = 0;
			return true;
		}
		if (r_data == nil)
		{
			t_size = t_data_size;
			if (!MCMemoryAllocate(t_size, t_data))
				return false;
		}
		else
		{
			t_size = r_size;
			t_data = r_data;
		}
		t_success = (t_data != nil && t_data_size <= t_size);
	}
	if (t_success)
	{
		MCMemoryCopy(t_data, p_stream + r_offset, t_data_size);
		r_data = t_data;
		r_size = t_size;
		r_offset += t_data_size;
	}
	return t_success;
}

////////////////////////////////////////////////////////////////////////////////
