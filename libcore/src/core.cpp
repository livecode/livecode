/* Copyright (C) 2009-2015 LiveCode Ltd.

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

////////////////////////////////////////////////////////////////////////////////
//
//  Private Source File:
//    core.cpp
//
//  Description:
//    This file contains core methods abstracting away from the C runtime-
//    library.
//
//  Changes:
//    2009-07-04 MW Created.
//    2009-07-29 MW Moved from kernel to libcore.
//
////////////////////////////////////////////////////////////////////////////////

#include "core.h"

#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <cctype>

#if defined(_MACOSX) || defined(TARGET_SUBPLATFORM_IPHONE)
#include <CoreFoundation/CoreFoundation.h>
#endif

#if defined(_WINDOWS) || defined(TARGET_SUBPLATFORM_WINDOWS) || defined(_WINDOWS_SERVER)
#include <windows.h>
#endif

////////////////////////////////////////////////////////////////////////////////

int UTF8ToUnicode(const char * lpSrcStr, int cchSrc, uint16_t * lpDestStr, int cchDest);
int UnicodeToUTF8(const uint16_t *lpSrcStr, int cchSrc, char *lpDestStr, int cchDest);

////////////////////////////////////////////////////////////////////////////////

bool MCThrow(uint32_t p_error)
{
	return false;
}

////////////////////////////////////////////////////////////////////////////////

#if defined(_DEBUG)

#if defined(_WINDOWS) || defined(_WINDOWS_SERVER)

#include <windows.h>
#include <crtdbg.h>
#include <dbghelp.h>

void __MCAssert(const char *p_file, uint32_t p_line, const char *p_message)
{
	_CrtDbgReport(_CRT_ASSERT, p_file, p_line, NULL, "%s", p_message);
}

void __MCLog(const char *p_file, uint32_t p_line, const char *p_format, ...)
{
	char *t_string;
	t_string = nil;

	va_list t_args;
	va_start(t_args, p_format);
	MCCStringFormatV(t_string, p_format, t_args);
	va_end(t_args);

	_CrtDbgReport(_CRT_WARN, p_file, p_line, NULL, "[%u] %s\n", GetCurrentProcessId(), t_string);

	MCCStringFree(t_string);
}

void __MCLogWithTrace(const char *p_file, uint32_t p_line, const char *p_format, ...)
{
	typedef BOOL (WINAPI *SymFromAddrPtr)(HANDLE, DWORD64, PDWORD64, PSYMBOL_INFO);
	typedef BOOL (WINAPI *SymGetLineFromAddr64Ptr)(HANDLE, DWORD64, PDWORD64, PIMAGEHLP_LINE64);
	typedef BOOL (WINAPI *SymInitializePtr)(HANDLE, PCSTR, BOOL);
	typedef DWORD (WINAPI *SymSetOptionsPtr)(DWORD);
	static SymInitializePtr s_sym_initialize = nil;
	static SymSetOptionsPtr s_sym_setoptions = nil;
	static SymGetLineFromAddr64Ptr s_sym_get_line_from_addr_64 = nil;
	static SymFromAddrPtr s_sym_from_addr = nil;
	if (s_sym_from_addr == nil)
	{
		HMODULE t_dbg_help;
		t_dbg_help = LoadLibraryA("dbghelp.dll");
		if (t_dbg_help != nil)
		{
			s_sym_setoptions = (SymSetOptionsPtr)GetProcAddress(t_dbg_help, "SymSetOptions");
			s_sym_initialize = (SymInitializePtr)GetProcAddress(t_dbg_help, "SymInitialize");
			s_sym_from_addr = (SymFromAddrPtr)GetProcAddress(t_dbg_help, "SymFromAddr");
			s_sym_get_line_from_addr_64 = (SymGetLineFromAddr64Ptr)GetProcAddress(t_dbg_help, "SymGetLineFromAddr64");
			s_sym_initialize(GetCurrentProcess(), NULL, TRUE);
			s_sym_setoptions(SYMOPT_LOAD_LINES);
		}
	}

	char *t_string;
	t_string = nil;

	va_list t_args;
	va_start(t_args, p_format);
	MCCStringFormatV(t_string, p_format, t_args);
	va_end(t_args);

	_CrtDbgReport(_CRT_WARN, p_file, p_line, NULL, "[%u] %s\n", GetCurrentProcessId(), t_string);

	MCCStringFree(t_string);

	if (s_sym_from_addr != nil)
	{
		char t_buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(CHAR)];
		PSYMBOL_INFO t_symbol = (PSYMBOL_INFO)t_buffer;

		t_symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
		t_symbol->MaxNameLen = MAX_SYM_NAME - 1;

		void *t_backtrace[64];
		USHORT t_count;
		t_count = CaptureStackBackTrace(1, 62, t_backtrace, nil);
		for(uint32_t i = 0; i < t_count; i++)
		{
			if (s_sym_from_addr(GetCurrentProcess(), (DWORD64)t_backtrace[i], nil, t_symbol))
			{
				IMAGEHLP_LINE64 t_line;
				DWORD64 t_displacement;
				t_line . SizeOfStruct = sizeof(IMAGEHLP_LINE64);
				s_sym_get_line_from_addr_64(GetCurrentProcess(), (DWORD64)t_backtrace[i], &t_displacement, &t_line);
				_CrtDbgReport(_CRT_WARN, p_file, p_line, NULL, "[%u]  %s@0x%p, line %d\n", GetCurrentProcessId(), t_symbol -> Name, t_backtrace[i], t_line . LineNumber);
			}
		}
	}
}

#elif defined(_MACOSX) || defined(_LINUX) || defined(TARGET_SUBPLATFORM_IPHONE)

#include <unistd.h>

void __MCAssert(const char *p_file, uint32_t p_line, const char *p_message)
{
}

void __MCLog(const char *p_file, uint32_t p_line, const char *p_format, ...)
{
	fprintf(stderr, "[%d] ", getpid());
	va_list t_args;
	va_start(t_args, p_format);
	vfprintf(stderr, p_format, t_args);
	va_end(t_args);
	fprintf(stderr, "\n");
}

#elif defined(TARGET_SUBPLATFORM_ANDROID)

#include <android/log.h>

void __MCAssert(const char *p_file, uint32_t p_line, const char *p_message)
{
}

void __MCLog(const char *p_file, uint32_t p_line, const char *p_format, ...)
{
	va_list args;
	va_start(args, p_format);
	__android_log_vprint(ANDROID_LOG_INFO, "revandroid", p_format, args);
	va_end(args);
}

#endif

#endif

////////////////////////////////////////////////////////////////////////////////

bool MCMemoryAllocate(uindex_t p_size, void*& r_block)
{
	void *t_block;
	t_block = malloc(p_size);
	if (t_block != nil)
	{
		r_block = t_block;
		return true;
	}
	return false;
}

bool MCMemoryAllocateCopy(const void *p_block, uindex_t p_block_size, void*& r_block)
{
	if (MCMemoryAllocate(p_block_size, r_block))
	{
		MCMemoryCopy(r_block, p_block, p_block_size);
		return true;
	}
	return false;
}

bool MCMemoryReallocate(void *p_block, uindex_t p_new_size, void*& r_new_block)
{
	void *t_new_block;
	t_new_block = realloc(p_block, p_new_size);
	if (t_new_block != nil)
	{
		r_new_block = t_new_block;
		return true;
	}
	return false;
}

void MCMemoryDeallocate(void *p_block)
{
	free(p_block);
}

bool MCMemoryNew(uindex_t p_size, void*& r_record)
{
	if (MCMemoryAllocate(p_size, r_record))
	{
		MCMemoryClear(r_record, p_size);
		return true;
	}
	return false;
}

void MCMemoryDelete(void *p_record)
{
	MCMemoryDeallocate(p_record);
}

bool MCMemoryNewArray(uindex_t p_count, uindex_t p_size, void*& r_array)
{
	if (MCMemoryAllocate(p_count * p_size, r_array))
	{
		MCMemoryClear(r_array, p_count * p_size);
		return true;
	}
	return false;
}

bool MCMemoryResizeArray(uindex_t p_new_count, uindex_t p_size, void*& x_array, uindex_t& x_count)
{
	if (MCMemoryReallocate(x_array, p_new_count * p_size, x_array))
	{
		if (p_new_count > x_count)
			MCMemoryClear(static_cast<char *>(x_array) + x_count * p_size, (p_new_count - x_count) * p_size);
		x_count = p_new_count;
		return true;
	}
	return false;
}

void MCMemoryDeleteArray(void *p_array)
{
	MCMemoryDeallocate(p_array);
}

void MCMemoryClear(void *p_bytes, uindex_t p_size)
{
	memset(p_bytes, 0, p_size);
}

void MCMemoryCopy(void *p_dst, const void *p_src, uindex_t p_size)
{
	memcpy(p_dst, p_src, p_size);
}

void MCMemoryMove(void *p_dst, const void *p_src, uindex_t p_size)
{
	memmove(p_dst, p_src, p_size);
}

bool MCMemoryEqual(const void *p_left, const void *p_right, uindex_t p_size)
{
	return memcmp(p_left, p_right, p_size) == 0;
}

compare_t MCMemoryCompare(const void *p_left, const void *p_right, uindex_t p_size)
{
	return memcmp(p_left, p_right, p_size);
}

////////////////////////////////////////////////////////////////////////////////

uint32_t MCCStringLength(const char *p_string)
{
	if (p_string == nil)
		return 0;
	return strlen(p_string);
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

bool MCCStringFormat(char*& r_string, const char *p_format, ...)
{
	va_list t_args;
	int t_count;
#if defined(_WINDOWS) || defined(_WINDOWS_SERVER)
	va_start(t_args, p_format);
	t_count = _vscprintf(p_format, t_args);
	va_end(t_args);
#elif defined(_MACOSX) || defined(_LINUX) || defined(TARGET_SUBPLATFORM_IPHONE) || defined(TARGET_SUBPLATFORM_ANDROID) || defined(__EMSCRIPTEN__)
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
#elif defined(_MACOSX) || defined(_LINUX) || defined(TARGET_SUBPLATFORM_IPHONE) || defined(TARGET_SUBPLATFORM_ANDROID) || defined(__EMSCRIPTEN__)
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
#elif defined(_MACOSX) || defined(_LINUX) || defined(TARGET_SUBPLATFORM_IPHONE) || defined(TARGET_SUBPLATFORM_ANDROID) || defined(__EMSCRIPTEN__)
	va_start(t_args, p_format);
	t_count = vsnprintf(nil, 0, p_format, t_args);
	va_end(t_args);
#else
#error "Implement MCCStringFormat"
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

bool MCCStringCloneSubstring(const char *p_string, uint32_t p_length, char*& r_new_string)
{
	if (!MCMemoryAllocate(p_length + 1, r_new_string))
		return false;
	MCMemoryCopy(r_new_string, p_string, p_length);
	r_new_string[p_length] = '\0';
	return true;
}

void MCCStringFree(char *p_string)
{
	MCMemoryDeallocate(p_string);
}

bool MCCStringArrayClone(const char *const *p_strings, uint32_t p_count, char**& r_new_strings)
{
	bool t_success;
	t_success = true;

	char **t_array;
	t_array = nil;
	if (t_success)
		t_success = MCMemoryNewArray(p_count, t_array);

	for(uint32_t i = 0; i < p_count && t_success; i++)
		t_success = MCCStringClone(p_strings[i], t_array[i]);

	if (t_success)
		r_new_strings = t_array;
	else
		MCCStringArrayFree(t_array, p_count);

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

#if defined(_MACOSX) || defined(TARGET_SUBPLATFORM_IPHONE)
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
#elif defined(_LINUX)
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

compare_t MCCStringCompare(const char *p_left, const char *p_right)
{
	return strcmp(p_left, p_right);
}

compare_t MCCStringCompareCaseless(const char *p_left, const char *p_right)
{
#if defined(_WINDOWS) || defined(_WINDOWS_SERVER)
	return _stricmp(p_left, p_right);
#else
	return strcasecmp(p_left, p_right);
#endif
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

bool MCCStringToCardinal(const char *p_string, uint32_t& r_value)
{
	if (*p_string == 0)
		return false;

	uint32_t t_value;
	t_value = 0;
	while(*p_string != 0)
	{
		if (!isdigit(*p_string))
			return false;

		uint32_t t_new_value;
		t_new_value = t_value * 10 + (*p_string - '0');
		if (t_new_value < t_value)
			return false;

		t_value = t_new_value;
		p_string++;
	}

	r_value = t_value;

	return true;
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

#endif

#ifdef _WINDOWS
bool MCCStringToBSTR(const char *p_cstring, BSTR& r_bstr)
{
	if (p_cstring == nil)
		p_cstring = "";

	uint32_t t_length;
	t_length = MCCStringLength(p_cstring) + 1;

	BSTR t_result;
	t_result = SysAllocStringLen(nil, t_length);
	if (t_result == nil)
		return false;

	for(uint32_t i = 0; i < t_length; i++)
		t_result[i] = p_cstring[i];

	r_bstr = t_result;

	return true;
}

bool MCCStringFromBSTR(BSTR p_bstr, char*& r_cstring)
{
	if (p_bstr == nil)
		return MCCStringClone("", r_cstring);

	uint32_t t_length;
	t_length = SysStringLen(p_bstr) + 1;
	
	char *t_result;
	if (!MCMemoryNewArray(t_length, t_result))
		return false;

	for(uint32_t i = 0; i < t_length; i++)
		t_result[i] = (char)p_bstr[i];

	r_cstring = t_result;

	return true;
}
#endif

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
