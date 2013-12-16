/* Copyright (C) 2003-2013 Runtime Revolution Ltd

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

#include "foundation-private.h"

#ifdef __LINUX__
#include <errno.h>
#include <iconv.h>
#include <langinfo.h>
#include <locale.h>
#endif

////////////////////////////////////////////////////////////////////////////////

// CAVEAT!
//
// The currently implementation of the string value is native-only. Although
// the main (Unicode) methods will work, the input will get coerced to native
// encoding.
//
// When all the work is done to change the engine to use MCValueRef exclusively,
// gaining full transparent Unicode *should* just be a case of reimplementing
// the string value!

// NOTE!
//
// As it stands the underlying (native) char buffer always has an implicit NUL
// terminator added on. This is merely an aid to integration until all the uses
// of the legacy 'NativeCString' method is eliminated from use in the engine
// (which is pre-requisite for unicodification).

////////////////////////////////////////////////////////////////////////////////

// This method ensures there is 'count' chars of empty space starting at 'at'
// in 'string'. It returns false if allocation fails.
static bool __MCStringExpandAt(MCStringRef string, uindex_t at, uindex_t count);

// This method removes 'count' chars from string starting at 'at'.
static void __MCStringShrinkAt(MCStringRef string, uindex_t at, uindex_t count);

// This method clamps the given range to the valid limits for the string.
static void __MCStringClampRange(MCStringRef string, MCRange& x_range);

// This method ensures there is a native string ptr.
static void __MCStringNativize(MCStringRef string);

// This method marks the string as changed.
static void __MCStringChanged(MCStringRef string);

// Converts two surrogate pair code units into a codepoint
static codepoint_t __MCStringSurrogatesToCodepoint(unichar_t p_lead, unichar_t p_trail);

// Converts a codepoint to UTF-16 code units and returns the number of units
static unsigned int __MCStringCodepointToSurrogates(codepoint_t, unichar_t (&r_units)[2]);

// Returns true if the code unit at the given index and the next code unit form
// a valid surrogate pair. Lone lead or trail code units are not valid pairs.
static bool __MCStringIsValidSurrogatePair(MCStringRef, uindex_t);

// Creates a string


////////////////////////////////////////////////////////////////////////////////

// This method creates a 'constant' MCStringRef from the given c-string. At some
// point we'll make it work 'magically' at compile/build time. For now, uniquing
// and returning that has a similar effect (if slightly slower).
MCStringRef MCSTR(const char *p_cstring)
{
	MCStringRef t_string;
	/* UNCHECKED */ MCStringCreateWithNativeChars((const char_t *)p_cstring, strlen(p_cstring), t_string);
	
	MCValueRef t_unique_string;
	/* UNCHECKED */ MCValueInter(t_string, t_unique_string);
	
	MCValueRelease(t_string);
	
	return (MCStringRef)t_unique_string;
}

////////////////////////////////////////////////////////////////////////////////

bool MCStringCreateWithCString(const char* p_cstring, MCStringRef& r_string)
{
	return MCStringCreateWithNativeChars((const char_t*)p_cstring, p_cstring == nil ? 0 : strlen(p_cstring), r_string);
}

bool MCStringCreateWithCStringAndRelease(char_t* p_cstring, MCStringRef& r_string)
{
	return MCStringCreateWithNativeCharsAndRelease(p_cstring, p_cstring == nil ? 0 : strlen((const char*)p_cstring), r_string);
}

const char *MCStringGetCString(MCStringRef p_string)
{
    if (p_string == nil)
        return nil;
    
	const char *t_cstring;
	t_cstring = (const char *)MCStringGetNativeCharPtr(p_string);
	
	MCAssert(t_cstring != nil);
    
	return t_cstring;
}

bool MCStringIsEqualToCString(MCStringRef p_string, const char *p_cstring, MCStringOptions p_options)
{
	return MCStringIsEqualToNativeChars(p_string, (const char_t *)p_cstring, strlen(p_cstring), p_options);
}

// Create an immutable string from the given bytes, interpreting them using
// the specified encoding.
bool MCStringCreateWithBytes(const byte_t *p_bytes, uindex_t p_byte_count, MCStringEncoding p_encoding, bool p_is_external_rep, MCStringRef& r_string)
{
    MCAssert(!p_is_external_rep);
    
    switch (p_encoding)
    {
        case kMCStringEncodingASCII:
        case kMCStringEncodingNative:
            return MCStringCreateWithNativeChars(p_bytes, p_byte_count, r_string);
        case kMCStringEncodingUTF16:
            return MCStringCreateWithChars((unichar_t *)p_bytes, p_byte_count / 2, r_string);
        case kMCStringEncodingUTF16BE:
        {
            unichar_t *t_buffer;
            uindex_t t_length = p_byte_count / 2;
            MCMemoryAllocate(t_length * sizeof(unichar_t), t_buffer);

            for (uindex_t i = 0; i < t_length; i++)
                t_buffer[i] = (unichar_t)MCSwapInt16HostToBig(((unichar_t *)p_bytes)[i]);
            return MCStringCreateWithCharsAndRelease(t_buffer, t_length, r_string);
        }
            
        case kMCStringEncodingUTF8:
        {
            unichar_t *t_chars;
            uindex_t t_char_count;
            t_char_count = MCUnicodeCharsMapFromUTF8(p_bytes, p_byte_count, nil, 0);
            if (!MCMemoryNewArray(t_char_count, t_chars))
                return false;
            MCUnicodeCharsMapFromUTF8(p_bytes, p_byte_count, t_chars, t_char_count);
            if (!MCStringCreateWithCharsAndRelease(t_chars, t_char_count, r_string))
            {
                MCMemoryDeleteArray(t_chars);
                return false;
            }
            return true;
        }
        break;
        case kMCStringEncodingUTF32:
		{
			// Round the byte count to a multiple of UTF-32 units
			p_byte_count = ((p_byte_count + sizeof(uint32_t) - 1) & ~(sizeof(uint32_t) - 1));
			
			// Convert the string to UTF-16 first. The UTF-16 string cannot be longer than the UTF-32 string
			MCAutoArray<unichar_t> t_buffer;
			t_buffer.Extend(p_byte_count / sizeof(uint32_t));
			uindex_t t_in_offset;
			uindex_t t_out_offset = 0;
			for (t_in_offset = 0; t_in_offset < p_byte_count; t_in_offset += sizeof(uint32_t))
			{
				// BMP characters are output unchanged, non-BMP requires surrogate pairs
				codepoint_t t_codepoint;
				t_codepoint = *(uint32_t*)&p_bytes[t_in_offset];
				if (t_codepoint < 0x10000)
				{
					t_buffer[t_out_offset] = unichar_t(t_codepoint);
					t_out_offset += 1;
				}
				else
				{
					// Split to surrogate pairs
					unichar_t t_lead, t_trail;
					t_lead =  unichar_t((t_codepoint - 0x10000) >> 10) + 0xD800;
					t_trail = unichar_t((t_codepoint - 0x10000) & 0x3FF) + 0xDC00;
					t_buffer[t_out_offset] = t_lead;
					t_buffer[t_out_offset + 1] = t_trail;
					t_out_offset += 2;
				}
			}
			
			return MCStringCreateWithChars(t_buffer.Ptr(), t_out_offset, r_string);
		}
            break;
#if !defined(__LINUX__) && !defined(__ANDROID__)
        case kMCStringEncodingISO8859_1:
            break;
#endif
#ifndef __WINDOWS__
        case kMCStringEncodingWindows1252:
            break;
#endif
#if !defined(__MAC__) && !defined(__IOS__)
        case kMCStringEncodingMacRoman:
            break;
#endif
    }
    
    return false;
}

bool MCStringCreateWithBytesAndRelease(byte_t *p_bytes, uindex_t p_byte_count, MCStringEncoding p_encoding, bool p_is_external_rep, MCStringRef& r_string)
{
    MCStringRef t_string;
    
    if (!MCStringCreateWithBytes(p_bytes, p_byte_count, p_encoding, p_is_external_rep, t_string))
        return false;
    
    r_string = t_string;
    free(p_bytes);
    
    return true;
}

////////////////////////////////////////////////////////////////////////////////

bool MCStringCreateWithChars(const unichar_t *p_chars, uindex_t p_char_count, MCStringRef& r_string)
{
	bool t_success;
	t_success = true;

	__MCString *self;
	self = nil;
	if (t_success)
		t_success = __MCValueCreate(kMCValueTypeCodeString, self);

	if (t_success)
		t_success = MCMemoryNewArray(p_char_count + 1, self -> chars);

	if (t_success)
	{
		MCStrCharsMapFromUnicode(p_chars, p_char_count, self -> chars, self -> char_count);

		r_string = self;
	}
	else
	{
		if (self != nil)
			MCMemoryDeleteArray(self -> chars);
		MCMemoryDelete(self);
	}

	return t_success;
}

bool MCStringCreateWithCharsAndRelease(unichar_t *p_chars, uindex_t p_char_count, MCStringRef& r_string)
{
    if (MCStringCreateWithChars(p_chars, p_char_count, r_string))
    {
        free(p_chars);
        return true;
    }
    
    return false;
}

bool MCStringCreateWithWString(const unichar_t *p_wstring, MCStringRef& r_string)
{
	uindex_t t_length;
	for(t_length = 0; p_wstring[t_length] != 0; t_length++)
		;

	return MCStringCreateWithChars(p_wstring, t_length, r_string);
}

bool MCStringCreateWithNativeChars(const char_t *p_chars, uindex_t p_char_count, MCStringRef& r_string)
{
	bool t_success;
	t_success = true;

	if (p_char_count == 0 && kMCEmptyString != nil)
	{
		r_string = MCValueRetain(kMCEmptyString);
		return true;
	}

	__MCString *self;
	self = nil;
	if (t_success)
		t_success = __MCValueCreate(kMCValueTypeCodeString, self);

	if (t_success)
		t_success = MCMemoryNewArray(p_char_count + 1, self -> chars);

	if (t_success)
	{
		MCStrCharsMapFromNative(self -> chars, p_chars, p_char_count);
		self -> char_count = p_char_count;

		r_string = self;
	}
	else
	{
		if (self != nil)
			MCMemoryDeleteArray(self -> chars);
		MCMemoryDelete(self);
	}

	return t_success;
}

bool MCStringCreateWithNativeCharsAndRelease(char_t *p_chars, uindex_t p_char_count, MCStringRef& r_string)
{
	if (MCStringCreateWithNativeChars(p_chars, p_char_count, r_string))
	{
		MCMemoryDeallocate(p_chars);
		return true;
	}
	return false;
}

bool MCStringCreateMutable(uindex_t p_initial_capacity, MCStringRef& r_string)
{
	bool t_success;
	t_success = true;

	__MCString *self;
	self = nil;
	if (t_success)
		t_success = __MCValueCreate(kMCValueTypeCodeString, self);

	if (t_success)
		t_success = __MCStringExpandAt(self, 0, p_initial_capacity);

	if (t_success)
	{
		self -> flags |= kMCStringFlagIsMutable;
		r_string = self;
	}

	return t_success;
}

////////////////////////////////////////////////////////////////////////////////

bool MCStringEncode(MCStringRef p_string, MCStringEncoding p_encoding, bool p_is_external_rep, MCDataRef& r_data)
{
    byte_t *t_bytes;
    uindex_t t_byte_count;
    if (!MCStringConvertToBytes(p_string, p_encoding, p_is_external_rep, t_bytes, t_byte_count))
        return false;
    
    if (!MCDataCreateWithBytesAndRelease(t_bytes, t_byte_count, r_data))
    {
        free(t_bytes);
        return false;
    }

    return true;
}

bool MCStringEncodeAndRelease(MCStringRef p_string, MCStringEncoding p_encoding, bool p_is_external_rep, MCDataRef& r_data)
{    
    MCDataRef t_data;
    
    if (!MCStringEncode(p_string, p_encoding, p_is_external_rep, t_data))
        return false;
    
    MCValueRelease(p_string);
    r_data = t_data;
    
    return true;
}

bool MCStringDecode(MCDataRef p_data, MCStringEncoding p_encoding, bool p_is_external_rep, MCStringRef& r_string)
{
    return MCStringCreateWithBytes(MCDataGetBytePtr(p_data), MCDataGetLength(p_data), p_encoding, p_is_external_rep, r_string);
}

bool MCStringDecodeAndRelease(MCDataRef p_data, MCStringEncoding p_encoding, bool p_is_external_rep, MCStringRef& r_string)
{
    MCStringRef t_string;
    
    if (!MCStringDecode(p_data, p_encoding, p_is_external_rep, t_string))
        return false;
    
    MCValueRelease(p_data);
    r_string = t_string;
    
    return true;
}

////////////////////////////////////////////////////////////////////////////////

static bool __MCStringFormatSupportedForUnicode(const char *p_format)
{
	while(*p_format != '\0')
	{
		if (*p_format == '%' &&
			(p_format[1] != 's' && p_format[1] != 'd' && p_format[1] != '@'))
			return false;
		
		if (*p_format == '\\' &&
			(p_format[1] != 'n' && p_format[1] != '"'))
			return false;
		
		p_format++;
	}
	
	return true;
}

#if defined(__32_BIT__)
#define FORMAT_ARG_32_BIT 1
#define FORMAT_ARG_64_BIT 2
#elif defined(__64_BIT__)
#define FORMAT_ARG_32_BIT 2
#define FORMAT_ARG_64_BIT 2
#endif

bool MCStringFormatV(MCStringRef& r_string, const char *p_format, va_list p_args)
{
	MCStringRef t_buffer;
	if (!MCStringCreateMutable(0, t_buffer))
		return false;
	
	bool t_success;
	t_success = true;
	
	const char *t_format_ptr;
	t_format_ptr = p_format;
	while(t_success && *t_format_ptr != '\0')
	{
		const char *t_format_start_ptr;
		t_format_start_ptr = t_format_ptr;
		
		bool t_has_range;
		t_has_range = false;
		
		int t_arg_count;
		t_arg_count = 0;
		while(*t_format_ptr != '\0')
		{
			if (*t_format_ptr == '%')
			{
				t_format_ptr++;
				
				if (*t_format_ptr == '@')
					break;
				
				if (*t_format_ptr == '*')
				{
					t_arg_count += FORMAT_ARG_32_BIT;
					t_format_ptr++;
					
					if (*t_format_ptr == '@')
					{
						t_format_start_ptr = t_format_ptr;
                        t_has_range = true;
						break;
					}
				}
				else
				{
					while(*t_format_ptr != '\0' && isdigit(*t_format_ptr))
						t_format_ptr++;
				}
				
				if (*t_format_ptr == '.')
				{
					t_format_ptr++;
					if (*t_format_ptr == '*')
					{
						t_arg_count += FORMAT_ARG_32_BIT;
						t_format_ptr++;
					}
					else
					{
						while(*t_format_ptr != '\0' && isdigit(*t_format_ptr))
							t_format_ptr++;
					}
				}
				
				if (strncmp(t_format_ptr, "lld", 3) == 0 ||
					strncmp(t_format_ptr, "llu", 3) == 0 ||
					strncmp(t_format_ptr, "lf", 2) == 0 ||
					strncmp(t_format_ptr, "f", 1) == 0)
					t_arg_count += FORMAT_ARG_64_BIT;
				else
					t_arg_count += FORMAT_ARG_32_BIT;
			}
			
			t_format_ptr += 1;
		}
		
        if (t_format_start_ptr != t_format_ptr)
        {
            char *t_format;
            uint32_t t_format_size;

            // [[ vsnprintf ]] On Linux, the trailing '%' from '%@' placeholder causes vsprintf to fail
            // and return -1 in MCNativeCharsFormat (and thus creates a 0-byte sized array).
            if (*t_format_ptr == '@' && *(t_format_ptr - 1) == '%')
                t_format_size = t_format_ptr - t_format_start_ptr - 1;
            else
                t_format_size = t_format_ptr - t_format_start_ptr;

            /* UNCHECKED */ t_format = (char *)malloc(t_format_size + 1);
            if (t_format == nil)
                t_success = false;

            char_t *t_string;
            uindex_t t_size;
            t_string = nil;
			if (t_success)
            {
                memcpy(t_format, t_format_start_ptr, t_format_size);
                t_format[t_format_size] = '\0';
				t_success = MCNativeCharsFormatV(t_string, t_size, t_format, p_args);
			}
			
			if (t_success)
				t_success = MCStringAppendNativeChars(t_buffer, t_string, t_size);

			if (t_success)
				while(t_arg_count > 0)
				{
					va_arg(p_args, int);
					t_arg_count -= 1;
				}
					
            MCMemoryDeallocate(t_string);
			free(t_format);
		}
		
		if (t_success && *t_format_ptr == '@')
		{
			t_format_ptr += 1;
		
			const MCRange *t_range;
			if (t_has_range)
				t_range = va_arg(p_args, const MCRange *);
			else
				t_range = nil;
				
			MCValueRef t_value;
			t_value = va_arg(p_args, MCValueRef);
			
			MCAutoStringRef t_string;
			if (MCValueGetTypeCode(t_value) == kMCValueTypeCodeString)
				t_string = (MCStringRef)t_value;
			else if (MCValueGetTypeCode(t_value) == kMCValueTypeCodeName)
				t_string = MCNameGetString((MCNameRef)t_value);
			else if (MCValueGetTypeCode(t_value) == kMCValueTypeCodeNumber)
				if (MCNumberIsInteger((MCNumberRef)t_value))
					/* UNCHECKED */ MCStringFormat(&t_string, "%d", MCNumberFetchAsInteger((MCNumberRef)t_value));
				else
					/* UNCHECKED */ MCStringFormat(&t_string, "%f", MCNumberFetchAsReal((MCNumberRef)t_value));
			else
				MCAssert(false);

			if (t_range == nil)
				t_success = MCStringAppend(t_buffer, *t_string);
			else
				t_success = MCStringAppendSubstring(t_buffer, *t_string, *t_range);
		}
	}

	if (t_success)
		t_success = MCStringCopyAndRelease(t_buffer, r_string);
	
	return t_success;
}

bool MCStringFormat(MCStringRef& r_string, const char *p_format, ...)
{
	bool t_success;

	va_list t_args;
	va_start(t_args, p_format);
	t_success = MCStringFormatV(r_string, p_format, t_args);
	va_end(t_args);

	return t_success;
}

////////////////////////////////////////////////////////////////////////////////

static bool __MCStringClone(MCStringRef self, MCStringRef& r_new_string)
{
	return MCStringCreateWithChars(self -> chars, self -> char_count, r_new_string);
}

bool MCStringCopy(MCStringRef self, MCStringRef& r_new_string)
{
	// If the string is immutable we can just bump the reference count.
	if (!MCStringIsMutable(self))
	{
		r_new_string = self;
		MCValueRetain(self);
		return true;
	}

	// Otherwise make a copy of the string.
	return __MCStringClone(self, r_new_string);
}

bool MCStringCopyAndRelease(MCStringRef self, MCStringRef& r_new_string)
{
	// If the string is immutable we just pass it through (as we are releasing the string).
	if (!MCStringIsMutable(self))
	{
		r_new_string = self;
		return true;
	}

	// If the reference count is one, then convert it to an immutable string.
	if (self -> references == 1)
	{
		// Shrink the char buffer to be just long enough for the characters plus
		// an implicit NUL.
		MCMemoryResizeArray(self -> char_count + 1, self -> chars, self -> char_count);
		self -> flags &= ~kMCStringFlagIsMutable;
		self -> char_count -= 1;
		r_new_string = self;
		return true;
	}

	// Otherwise, make a copy of the string *then* release the original. We have
	// to do this in this order because the other order would scupper any sort of
	// eventual thread safety (a thread must not mutate a value for which it hasnt
	// contributed to the retain count).
	bool t_success;
	t_success = __MCStringClone(self, r_new_string);
	MCValueRelease(self);
	return t_success;
}

bool MCStringMutableCopy(MCStringRef self, MCStringRef& r_new_string)
{
	// Simply create a mutable string with enough initial capacity and then copy
	// in the other strings chars.
	// Notice the slight amount of anal-retentiveness here in the use of a
	// temporary - any 'r_' (out) parameter should never be updated until the end
	// of the method and then only if the method is succeeding.
	MCStringRef t_new_string;
	if (!MCStringCreateMutable(self -> char_count + 1, t_new_string))
		return false;

	// Now copy across the chars (not we copy the implicit NUL too, just to be
	// on the safe-side!).
	MCMemoryCopy(t_new_string -> chars, self -> chars, (self -> char_count + 1) * sizeof(strchar_t));
	t_new_string -> char_count = self -> char_count;

	// Return the new string and success.
	r_new_string = t_new_string;
	return true;
}

bool MCStringMutableCopyAndRelease(MCStringRef self, MCStringRef& r_new_string)
{
	if (MCStringMutableCopy(self, r_new_string))
	{
		MCValueRelease(self);
		return true;
	}

	return false;
}

////////////////////////////////////////////////////////////////////////////////

bool MCStringCopySubstring(MCStringRef self, MCRange p_range, MCStringRef& r_substring)
{
	__MCStringClampRange(self, p_range);
	
	return MCStringCreateWithChars(self -> chars + p_range . offset, p_range . length, r_substring);
}

bool MCStringCopySubstringAndRelease(MCStringRef self, MCRange p_range, MCStringRef& r_substring)
{
	if (MCStringCopySubstring(self, p_range, r_substring))
	{
		MCValueRelease(self);
		return true;
	}

	return false;
}

bool MCStringMutableCopySubstring(MCStringRef self, MCRange p_range, MCStringRef& r_new_string)
{
    __MCStringClampRange(self, p_range);
    
	// Simply create a mutable string with enough initial capacity and then copy
	// in the other strings chars.
	// Notice the slight amount of anal-retentiveness here in the use of a
	// temporary - any 'r_' (out) parameter should never be updated until the end
	// of the method and then only if the method is succeeding.
	MCStringRef t_new_string;
	if (!MCStringCreateMutable(p_range . length + 1, t_new_string))
		return false;
    
	// Now copy across the chars (note we set the implicit NUL too, just to be
	// on the safe-side!).
	MCMemoryCopy(t_new_string -> chars, self -> chars + p_range . offset, p_range . length * sizeof(strchar_t));
    t_new_string -> chars[p_range . length] = '\0';

	t_new_string -> char_count = p_range . length;
    
	// Return the new string and success.
	r_new_string = t_new_string;
	return true;
}

bool MCStringMutableCopySubstringAndRelease(MCStringRef self, MCRange p_range, MCStringRef& r_new_string)
{
	if (MCStringMutableCopySubstring(self, p_range, r_new_string))
	{
		MCValueRelease(self);
		return true;
	}
    
	return false;
}

////////////////////////////////////////////////////////////////////////////////

bool MCStringIsMutable(const MCStringRef self)
{
	return (self -> flags & kMCStringFlagIsMutable) != 0;
}

////////////////////////////////////////////////////////////////////////////////

uindex_t MCStringGetLength(MCStringRef self)
{
	return self -> char_count;
}

const unichar_t *MCStringGetCharPtr(MCStringRef self)
{
	return self -> chars;
}

const char_t *MCStringGetNativeCharPtr(MCStringRef self)
{
	__MCStringNativize(self);
	return self -> native_chars;
}

unichar_t MCStringGetCharAtIndex(MCStringRef self, uindex_t p_index)
{
	return self -> chars[p_index];
}

char_t MCStringGetNativeCharAtIndex(MCStringRef self, uindex_t p_index)
{
	char_t t_native_char;
	if (MCUnicodeCharMapToNative(self -> chars[p_index], t_native_char))
		return t_native_char;
	return '?';
}

codepoint_t MCStringGetCodepointAtIndex(MCStringRef self, uindex_t p_index)
{
	// Calculate the code unit index for the given codepoint
	MCRange t_codepoint_idx, t_codeunit_idx;
    t_codepoint_idx = MCRangeMake(p_index, 1);
    if (!MCStringMapCodepointIndices(self, t_codepoint_idx, t_codeunit_idx))
        return 0;
    
    // Get the codepoint at this index
    unichar_t t_lead, t_trail;
    t_lead = self -> chars[t_codeunit_idx.offset];
    if (t_codeunit_idx.length == 1)
        return t_lead;
    
    // We have a surrogate pair
    t_trail = self -> chars[t_codeunit_idx.offset + 1];
    return __MCStringSurrogatesToCodepoint(t_lead, t_trail);
}

uindex_t MCStringGetChars(MCStringRef self, MCRange p_range, unichar_t *p_chars)
{
	uindex_t t_count;
	t_count = 0;

	for(uindex_t i = p_range . offset; i < p_range . offset + p_range . length; i++)
	{
		if (i >= self -> char_count)
			break;

		p_chars[i - p_range . offset] = MCStrCharMapToUnicode(self -> chars[i]);

		t_count += 1;
	}

	return t_count;
}

uindex_t MCStringGetNativeChars(MCStringRef self, MCRange p_range, char_t *p_chars)
{
	uindex_t t_count;
	t_count = 0;

	for(uindex_t i = p_range . offset; i < p_range . offset + p_range . length; i++)
	{
		if (i >= self -> char_count)
			break;

		p_chars[i - p_range . offset] = MCStrCharMapToNative(self -> chars[i]);

		t_count += 1;
	}

	return t_count;
}

bool MCStringIsNative(MCStringRef self)
{
	__MCStringNativize(self);
	return (self -> flags & kMCStringFlagIsNative) != 0;
}

bool MCStringIsSimple(MCStringRef self)
{
    return (self -> flags & kMCStringFlagIsSimple) != 0;
}

bool MCStringMapCodepointIndices(MCStringRef self, MCRange p_in_range, MCRange &r_out_range)
{
    MCAssert(self != nil);
    
    // Shortcut for strings containing only BMP characters
    if (MCStringIsSimple(self))
    {
        r_out_range = p_in_range;
        return true;
    }
    
    // Scan through the string, counting the number of codepoints
    bool t_is_simple = true;
    uindex_t t_counter = 0;
    MCRange t_units = MCRangeMake(0, 0);
    while (t_counter < p_in_range.offset + p_in_range.length)
    {
        // Is this a single code unit or a valid surrogate pair?
        uindex_t t_length;
        if (__MCStringIsValidSurrogatePair(self, t_units.offset + t_units.length))
            t_length = 2, t_is_simple = false;
        else
            t_length = 1;
        
        // Update the appropriate field of the output
        if (t_counter < p_in_range.offset)
            t_units.offset += t_length;
        else
            t_units.length += t_length;
        
        // Make sure we haven't exceeded the length of the string
        if (t_units.offset > self -> char_count)
        {
            t_units = MCRangeMake(self -> char_count, 0);
            break;
        }
        if ((t_units.offset + t_units.length) > self -> char_count)
        {
            t_units.length = self -> char_count - t_units.offset;
            break;
        }
        
        t_counter++;
    }
    
    // If no surrogates were found, mark the string as simple
    if (t_is_simple && t_units.offset + t_units.length == self -> char_count)
        self -> flags |= kMCStringFlagIsSimple;
    
    // All done
    r_out_range = t_units;
    return true;
}

bool MCStringUnmapCodepointIndices(MCStringRef self, MCRange p_in_range, MCRange &r_out_range)
{
    MCAssert(self != nil);
    
    // Shortcut for strings containing only BMP characters
    if (MCStringIsSimple(self))
    {
        r_out_range = p_in_range;
        return true;
    }
    
    // Check that the input indices are valid
    if (p_in_range.offset + p_in_range.length > self -> char_count)
        return false;
    
    // Scan through the string, counting the number of code points
    bool t_is_simple = true;
    uindex_t t_counter = 0;
    MCRange t_codepoints = MCRangeMake(0, 0);
    while (t_counter < p_in_range.offset + p_in_range.length)
    {
        // Is this a single code unit or a valid surrogate pair?
        uindex_t t_length;
        if (__MCStringIsValidSurrogatePair(self, t_counter))
            t_length = 2, t_is_simple = false;
        else
            t_length = 1;
        
        // Increment the counters
        if (t_counter < p_in_range.offset)
            t_codepoints.offset++;
        else
            t_codepoints.length++;
        t_counter += t_length;
    }
    
    // If no surrogates were found, mark the string as simple
    if (t_is_simple && p_in_range.offset + p_in_range.length >= self -> char_count)
        self -> flags |= kMCStringFlagIsSimple;
    
    // All done
    r_out_range = t_codepoints;
    return true;
}

bool MCStringMapGraphemeIndices(MCStringRef self, MCLocaleRef p_locale, MCRange p_in_range, MCRange &r_out_range)
{
    MCAssert(self != nil);
    MCAssert(p_locale != nil);
    
    // Create a grapheme break iterator
    MCBreakIteratorRef t_iter;
    if (!MCLocaleBreakIteratorCreate(p_locale, kMCBreakIteratorTypeCharacter, t_iter))
        return false;
    
    // Set the iterator's text
    if (!MCLocaleBreakIteratorSetText(t_iter, self))
    {
        MCLocaleBreakIteratorRelease(t_iter);
        return false;
    }
    
    // Move through the string until the indices have been resolved
    uindex_t t_counter = 0;
    MCRange t_units = MCRangeMake(0, 0);
    bool t_done = false;
    uindex_t t_boundary = 0;
    while (!t_done)
    {
        // Have we found the start of the requested grapheme range?
        if (t_counter == p_in_range.offset)
            t_units.offset = t_boundary;
        
        // Find the next boundary
        t_boundary = MCLocaleBreakIteratorAdvance(t_iter);
        if (t_boundary == kMCLocaleBreakIteratorDone)
        {
            // Ran out of string to process
            MCLocaleBreakIteratorRelease(t_iter);
            if (t_counter < p_in_range.offset)
                t_units = MCRangeMake(self -> char_count, 0);
            else
                t_units.length = self -> char_count - t_units.offset;
            break;
        }
        
        // Have we found the end of the requested grapheme range?
        if (t_counter == p_in_range.offset + p_in_range.length)
        {
            t_units.length = t_boundary;
            t_done = true;
        }
        
        t_counter++;
    }
    
    // All done
    MCLocaleBreakIteratorRelease(t_iter);
    r_out_range = t_units;
    return true;
}

bool MCStringUnmapGraphemeIndices(MCStringRef self, MCLocaleRef p_locale, MCRange p_in_range, MCRange &r_out_range)
{
    MCAssert(self != nil);
    MCAssert(p_locale != nil);
    
    // Check that the input range is valid
    if (p_in_range.offset + p_in_range.length > self -> char_count)
        return false;
    
    // Create a grapheme break iterator
    MCBreakIteratorRef t_iter;
    if (!MCLocaleBreakIteratorCreate(p_locale, kMCBreakIteratorTypeCharacter, t_iter))
        return false;
    
    // Set the iterator's text
    if (!MCLocaleBreakIteratorSetText(t_iter, self))
    {
        MCLocaleBreakIteratorRelease(t_iter);
        return false;
    }
    
    // Move through the string until the indices have been resolved
    MCRange t_graphemes = MCRangeMake(0, 0);
    bool t_done = false;
    uindex_t t_boundaries, t_units;
    t_boundaries = 0;
    t_units = 0;
    while (!t_done)
    {
        // Have we found the start of the requested code unit range?
        if (t_units == p_in_range.offset)
            t_graphemes.offset = t_boundaries;
        
        // Have we found the end of the requested code unit range?
        if (t_units == p_in_range.offset + p_in_range.length)
        {
            t_graphemes.length = t_units - t_graphemes.offset;
            t_done = true;
        }
        
        // If this is a boundary, increment the boundary counter
        if (MCLocaleBreakIteratorIsBoundary(t_iter, t_units))
            t_boundaries++;
        
        t_units++;
    }
    
    // All done
    MCLocaleBreakIteratorRelease(t_iter);
    r_out_range = t_graphemes;
    return true;
}

////////////////////////////////////////////////////////////////////////////////

bool MCStringConvertToBytes(MCStringRef self, MCStringEncoding p_encoding, bool p_is_external_rep, byte_t*& r_bytes, uindex_t& r_byte_count)
{
    MCAssert(!p_is_external_rep);
    
    switch(p_encoding)
    {
    case kMCStringEncodingASCII:
    case kMCStringEncodingNative:
        return MCStringConvertToNative(self, (char_t*&)r_bytes, r_byte_count);
    case kMCStringEncodingUTF16:
        {
            uindex_t t_char_count;
            if (MCStringConvertToUnicode(self, (unichar_t*&)r_bytes, t_char_count))
            {
                r_byte_count = t_char_count * sizeof(unichar_t);
                return true;
            }
            return false;
        }
    case kMCStringEncodingUTF8:
        return MCStringConvertToUTF8(self, (char*&)r_bytes, r_byte_count);
    case kMCStringEncodingUTF32:
        break;
#if !defined(__LINUX__) && !defined(__ANDROID__)
    case kMCStringEncodingISO8859_1:
        break;
#endif
#ifndef __WINDOWS__
    case kMCStringEncodingWindows1252:
        break;
#endif
#if !defined(__MAC__) && !defined(__IOS__)
    case kMCStringEncodingMacRoman:
        break;
#endif
    }
    
    return false;
}

bool MCStringConvertToUnicode(MCStringRef self, unichar_t*& r_chars, uindex_t& r_char_count)
{
	// Allocate an array of chars one bigger than needed. As the allocated array
	// is filled with zeros, this will naturally NUL terminate the string.
	unichar_t *t_chars;
	if (!MCMemoryNewArray(self -> char_count + 1, t_chars))
		return false;

	r_char_count = MCStringGetChars(self, MCRangeMake(0, self -> char_count), t_chars);
	r_chars = t_chars;
	return true;
}

bool MCStringConvertToNative(MCStringRef self, char_t*& r_chars, uindex_t& r_char_count)
{
	// Allocate an array of chars one byte bigger than needed. As the allocated array
	// is filled with zeros, this will naturally NUL terminate the string.
	char_t *t_chars;
	if (!MCMemoryNewArray(self -> char_count + 1, t_chars))
		return false;

	r_char_count = MCStringGetNativeChars(self, MCRangeMake(0, self -> char_count), t_chars);
	r_chars = t_chars;
	return true;
}


bool MCStringConvertToCString(MCStringRef p_string, char*& r_cstring)
{
    uindex_t t_length;
    t_length = MCStringGetLength(p_string);
    if (!MCMemoryNewArray(t_length + 1, r_cstring))
        return false;
    
    MCStringGetNativeChars(p_string, MCRangeMake(0, t_length), (char_t*)r_cstring);
    r_cstring[t_length] = '\0';
    
    return true;
}

bool MCStringConvertToWString(MCStringRef p_string, unichar_t*& r_wstring)
{
    uindex_t t_length;
    t_length = MCStringGetLength(p_string);
    if (!MCMemoryNewArray(t_length + 1, r_wstring))
        return false;
    
    MCStringGetChars(p_string, MCRangeMake(0, t_length), r_wstring);
    r_wstring[t_length] = '\0';
    
    return true;
}

bool MCStringConvertToUTF8String(MCStringRef p_string, char*& r_utf8string)
{
	uindex_t length_is_ignored;
	return MCStringConvertToUTF8(p_string, r_utf8string, length_is_ignored);
}

bool MCStringConvertToUTF8(MCStringRef p_string, char*& r_utf8string, uindex_t& r_utf8_chars)
{
	// Allocate an array of chars one byte bigger than needed. As the allocated array
	// is filled with zeros, this will naturally NUL terminate the string.
    uindex_t t_length;
    uindex_t t_byte_count;
    unichar_t* t_unichars;
	t_length = MCStringGetLength(p_string);
    
    if (!MCMemoryNewArray(t_length + 1, t_unichars))
        return false;
    
    uindex_t t_char_count = MCStringGetChars(p_string, MCRangeMake(0, t_length), t_unichars);
    
    t_byte_count = MCUnicodeCharsMapToUTF8(t_unichars, t_char_count, nil, 0);
    
    if (!MCMemoryNewArray(t_byte_count + 1, r_utf8string))
        return false;
    
    MCUnicodeCharsMapToUTF8(t_unichars, t_char_count, (byte_t*)r_utf8string, t_byte_count);
	r_utf8_chars = t_byte_count;
    
    // Delete temporary unichar_t array
    MCMemoryDeleteArray(t_unichars);
    
    return true;
}

#if defined(__MAC__) || defined (__IOS__)
bool MCStringConvertToCFStringRef(MCStringRef p_string, CFStringRef& r_cfstring)
{
    uindex_t t_length;
    unichar_t* t_chars;
    
    t_length = MCStringGetLength(p_string);
    if (!MCMemoryNewArray(t_length + 1, t_chars))
        return false;
    
    MCStringGetChars(p_string, MCRangeMake(0, t_length), t_chars);
	r_cfstring = CFStringCreateWithCharacters(nil, t_chars, t_length);
    
    MCMemoryDeleteArray(t_chars);
    return r_cfstring != nil;
}
#endif

#if 0
#ifdef __WINDOWS__
bool MCStringConvertToBSTR(MCStringRef p_string, BSTR& r_bstr)
{
    uindex_t t_length;
    unichar_t* t_chars;
    t_length = MCStringGetLength(p_string);
    if (!MCMemoryNewArray(t_length + 1, t_chars))
        return false;
    
    MCStringGetChars(p_string, MCRangeMake(0, t_length), t_chars);
    
    r_bstr = SysAllocString((OLECHAR*)t_chars);
    
    MCMemoryDeleteArray(t_chars);
    
    if (r_bstr == nil)
        return false;
    
    return true;
}
#endif
#endif

////////////////////////////////////////////////////////////////////////////////

#if 0

// At some point the CF string hashing function will probably be better - but
// shall use that when we move to Unicode.

/* String hashing: Should give the same results whatever the encoding; so we hash UniChars.
If the length is less than or equal to 96, then the hash function is simply the 
following (n is the nth UniChar character, starting from 0):
   
  hash(-1) = length
  hash(n) = hash(n-1) * 257 + unichar(n);
  Hash = hash(length-1) * ((length & 31) + 1)

If the length is greater than 96, then the above algorithm applies to 
characters 0..31, (length/2)-16..(length/2)+15, and length-32..length-1, inclusive;
thus the first, middle, and last 32 characters.

Note that the loops below are unrolled; and: 257^2 = 66049; 257^3 = 16974593; 257^4 = 4362470401;  67503105 is 257^4 - 256^4
If hashcode is changed from UInt32 to something else, this last piece needs to be readjusted.  
!!! We haven't updated for LP64 yet

NOTE: The hash algorithm used to be duplicated in CF and Foundation; but now it should only be in the four functions below.

Hash function was changed between Panther and Tiger, and Tiger and Leopard.
*/
#define HashEverythingLimit 96

#define HashNextFourUniChars(accessStart, accessEnd, pointer) \
    {result = result * 67503105 + (accessStart 0 accessEnd) * 16974593  + (accessStart 1 accessEnd) * 66049  + (accessStart 2 accessEnd) * 257 + (accessStart 3 accessEnd); pointer += 4;}

#define HashNextUniChar(accessStart, accessEnd, pointer) \
    {result = result * 257 + (accessStart 0 accessEnd); pointer++;}


/* In this function, actualLen is the length of the original string; but len is the number of characters in buffer. The buffer is expected to contain the parts of the string relevant to hashing.
*/
CF_INLINE CFHashCode __CFStrHashCharacters(const UniChar *uContents, CFIndex len, CFIndex actualLen) {
    CFHashCode result = actualLen;
    if (len <= HashEverythingLimit) {
        const UniChar *end4 = uContents + (len & ~3);
        const UniChar *end = uContents + len;
        while (uContents < end4) HashNextFourUniChars(uContents[, ], uContents); 	// First count in fours
        while (uContents < end) HashNextUniChar(uContents[, ], uContents);		// Then for the last <4 chars, count in ones...
    } else {
        const UniChar *contents, *end;
	contents = uContents;
        end = contents + 32;
        while (contents < end) HashNextFourUniChars(contents[, ], contents);
	contents = uContents + (len >> 1) - 16;
        end = contents + 32;
        while (contents < end) HashNextFourUniChars(contents[, ], contents);
	end = uContents + len;
        contents = end - 32;
        while (contents < end) HashNextFourUniChars(contents[, ], contents);
    }
    return result + (result << (actualLen & 31));
}

#endif

hash_t MCStringHash(MCStringRef self, MCStringOptions p_options)
{
	if (p_options != kMCStringOptionCompareExact)
		return MCStrCharsHashCaseless(self -> chars, self -> char_count);
	return MCStrCharsHashExact(self -> chars, self -> char_count);
}

bool MCStringIsEqualTo(MCStringRef self, MCStringRef p_other, MCStringOptions p_options)
{
	if (p_options != kMCStringOptionCompareExact)
		return MCStrCharsEqualCaseless(self -> chars, self -> char_count, p_other -> chars, p_other -> char_count);
	return MCStrCharsEqualExact(self -> chars, self -> char_count, p_other -> chars, p_other -> char_count);
}

bool MCStringIsEmpty(MCStringRef string)
{
	return MCStringIsEqualTo(string, kMCEmptyString, kMCStringOptionCompareExact);
}

bool MCStringSubstringIsEqualTo(MCStringRef self, MCRange p_sub, MCStringRef p_other, MCStringOptions p_options)
{
	__MCStringClampRange(self, p_sub);

	if (p_options != kMCStringOptionCompareExact)
		return MCStrCharsEqualCaseless(self -> chars + p_sub . offset, p_sub . length, p_other -> chars, p_other -> char_count);
	return MCStrCharsEqualExact(self -> chars + p_sub . offset, p_sub . length, p_other -> chars, p_other -> char_count);
}

bool MCStringIsEqualToNativeChars(MCStringRef self, const char_t *p_chars, uindex_t p_char_count, MCStringOptions p_options)
{
	MCAutoStringRef t_string;
	MCStringCreateWithNativeChars(p_chars, p_char_count, &t_string);
	return MCStringIsEqualTo(self, *t_string, p_options);
}

compare_t MCStringCompareTo(MCStringRef self, MCStringRef p_other, MCStringOptions p_options)
{
	if (p_options != kMCStringOptionCompareExact)
		return MCStrCharsCompareCaseless(self -> chars, self -> char_count, p_other -> chars, p_other -> char_count);
	return MCStrCharsCompareExact(self -> chars, self -> char_count, p_other -> chars, p_other -> char_count);
}

bool MCStringBeginsWith(MCStringRef self, MCStringRef p_prefix, MCStringOptions p_options)
{
	if (p_options == kMCStringOptionCompareExact)
        return MCStrCharsBeginsWithExact(self -> chars, self -> char_count, p_prefix -> chars, p_prefix -> char_count);
    else if (p_options == kMCStringOptionCompareNonliteral)
        return MCStrCharsBeginsWithNonliteral(self -> chars, self -> char_count, p_prefix -> chars, p_prefix -> char_count);
    else
        return MCStrCharsBeginsWithCaseless(self -> chars, self -> char_count, p_prefix -> chars, p_prefix -> char_count);
}

bool MCStringBeginsWithCString(MCStringRef self, const char_t *p_suffix_cstring, MCStringOptions p_options)
{
	MCAutoStringRef t_string;
	MCStringCreateWithNativeChars(p_suffix_cstring, strlen((const char *)p_suffix_cstring), &t_string);
	return MCStringBeginsWith(self, *t_string, p_options);
}

bool MCStringEndsWith(MCStringRef self, MCStringRef p_suffix, MCStringOptions p_options)
{
    if (p_options == kMCStringOptionCompareExact)
        return MCStrCharsEndsWithExact(self -> chars, self -> char_count, p_suffix -> chars, p_suffix -> char_count);
    else if (p_options == kMCStringOptionCompareNonliteral)
        return MCStrCharsEndsWithNonliteral(self -> chars, self -> char_count, p_suffix -> chars, p_suffix -> char_count);
    else
        return MCStrCharsEndsWithCaseless(self -> chars, self -> char_count, p_suffix -> chars, p_suffix -> char_count);
}

bool MCStringEndsWithCString(MCStringRef self, const char_t *p_suffix_cstring, MCStringOptions p_options)
{
	MCAutoStringRef t_string;
	MCStringCreateWithNativeChars(p_suffix_cstring, strlen((const char *)p_suffix_cstring), &t_string);
	return MCStringEndsWith(self, *t_string, p_options);
}

bool MCStringContains(MCStringRef self, MCStringRef p_needle, MCStringOptions p_options)
{
	if (p_options == kMCStringOptionCompareExact)
        return MCStrCharsContainsExact(self -> chars, self -> char_count, p_needle -> chars, p_needle -> char_count);
    else if (p_options == kMCStringOptionCompareNonliteral)
        return MCStrCharsContainsNonliteral(self -> chars, self -> char_count, p_needle -> chars, p_needle -> char_count);
    else
        return MCStrCharsContainsCaseless(self -> chars, self -> char_count, p_needle -> chars, p_needle -> char_count);
}

bool MCStringSubstringContains(MCStringRef self, MCRange p_range, MCStringRef p_needle, MCStringOptions p_options)
{
	__MCStringClampRange(self, p_range);

    if (p_options == kMCStringOptionCompareExact)
        return MCStrCharsContainsExact(self -> chars + p_range . offset, p_range . length, p_needle -> chars, p_needle -> char_count);
    else if (p_options == kMCStringOptionCompareNonliteral)
        return MCStrCharsContainsNonliteral(self -> chars + p_range . offset, p_range . length, p_needle -> chars, p_needle -> char_count);
    else
        return MCStrCharsContainsCaseless(self -> chars + p_range . offset, p_range . length, p_needle -> chars, p_needle -> char_count);
}

////////////////////////////////////////////////////////////////////////////////

bool MCStringFirstIndexOf(MCStringRef self, MCStringRef p_needle, uindex_t p_after, MCStringOptions p_options, uindex_t& r_offset)
{
	// Make sure the after index is in range.
	p_after = MCMin(p_after, self -> char_count);

    bool t_result;
    if (p_options == kMCStringOptionCompareExact)
        t_result = MCStrCharsFirstIndexOfExact(self -> chars + p_after, self -> char_count - p_after, p_needle -> chars, p_needle -> char_count, r_offset);
    else if (p_options == kMCStringOptionCompareNonliteral)
        t_result = MCStrCharsFirstIndexOfNonliteral(self -> chars + p_after, self -> char_count - p_after, p_needle -> chars, p_needle -> char_count, r_offset);
    else
        t_result =  MCStrCharsFirstIndexOfCaseless(self -> chars + p_after, self -> char_count - p_after, p_needle -> chars, p_needle -> char_count, r_offset);
   
    // Correct the output index
    if (t_result == true)
        r_offset += p_after;
    
    return t_result;
}

bool MCStringFirstIndexOfChar(MCStringRef self, codepoint_t p_needle, uindex_t p_after, MCStringOptions p_options, uindex_t& r_offset)
{
	// Make sure the after index is in range.
	p_after = MCMin(p_after, self -> char_count);

    bool t_result;
    if (p_options == kMCStringOptionCompareExact)
        t_result = MCStrCharsFirstIndexOfCharExact(self -> chars + p_after, self -> char_count - p_after, p_needle, r_offset);
    else if (p_options == kMCStringOptionCompareNonliteral)
        t_result = MCStrCharsFirstIndexOfCharNonliteral(self -> chars + p_after, self -> char_count - p_after, p_needle, r_offset);
    else
        t_result = MCStrCharsFirstIndexOfCharCaseless(self -> chars + p_after, self -> char_count - p_after, p_needle, r_offset);
    
    // Correct the output index
    if (t_result == true)
        r_offset += p_after;
    
    return t_result;
}

bool MCStringLastIndexOf(MCStringRef self, MCStringRef p_needle, uindex_t p_before, MCStringOptions p_options, uindex_t& r_offset)
{
	// Make sure the before index is in range.
	p_before = MCMin(p_before, self -> char_count);

    bool t_result;
    if (p_options == kMCStringOptionCompareExact)
        t_result = MCStrCharsLastIndexOfExact(self -> chars, p_before, p_needle -> chars, p_needle -> char_count, r_offset);
    else if (p_options == kMCStringOptionCompareNonliteral)
        t_result = MCStrCharsLastIndexOfNonliteral(self -> chars, p_before, p_needle -> chars, p_needle -> char_count, r_offset);
    else
        t_result = MCStrCharsLastIndexOfCaseless(self -> chars, p_before, p_needle -> chars, p_needle -> char_count, r_offset);
    
    return t_result;
}

bool MCStringLastIndexOfChar(MCStringRef self, codepoint_t p_needle, uindex_t p_before, MCStringOptions p_options, uindex_t& r_offset)
{
	// Make sure the after index is in range.
	p_before = MCMin(p_before, self -> char_count);
    
    bool t_result;
    if (p_options == kMCStringOptionCompareExact)
        t_result = MCStrCharsLastIndexOfCharExact(self -> chars, p_before, p_needle, r_offset);
    else if (p_options == kMCStringOptionCompareNonliteral)
        t_result = MCStrCharsLastIndexOfCharNonliteral(self -> chars, p_before, p_needle, r_offset);
    else
        t_result = MCStrCharsLastIndexOfCharCaseless(self -> chars, p_before, p_needle, r_offset);

    return t_result;
}

bool MCStringFind(MCStringRef self, MCRange p_range, MCStringRef p_needle, MCStringOptions p_options, MCRange *r_result)
{
    // TODO: use ICU
    // Unfortunately, this is less than trivial as ICU doesn't provide a
    // mechanism for returning the length of the range that was matched.
    
    // Similar to contains, this searches for needle but only with range of self.
	// It also returns the the range in self that needle occupies (but only if
	// r_result is non-nil).

	// Compute the char ptr and length based on range.
	const strchar_t *t_chars;
	uindex_t t_char_count;
	t_chars = self -> chars + MCMin(p_range . offset, self -> char_count);
	t_char_count = MCMin(p_range . length, self -> char_count - (t_chars - self -> chars));

	// Loop through the char range until we find a common prefix of sufficient
	// length.
	for(uindex_t t_offset = 0; t_offset < t_char_count; t_offset += 1)
	{
		// Compute the length of the shared prefix at the current offset.
		uindex_t t_prefix_length;
		if (p_options != kMCStringOptionCompareExact)
			t_prefix_length = MCStrCharsSharedPrefixCaseless(t_chars + t_offset, t_char_count - t_offset, p_needle -> chars, p_needle -> char_count);
		else
			t_prefix_length = MCStrCharsSharedPrefixExact(t_chars + t_offset, t_char_count - t_offset, p_needle -> chars, p_needle -> char_count);

		// If the prefix length is the same as needle, we are done.
		if (t_prefix_length == p_needle -> char_count)
		{
			// If requested, then compute the resulting range.
			if (r_result != nil)
			{
				// As the length of the prefix is counted relative to needle
				// we must recompute with things 'the other way around' as
				// range is relative to self. [ This will not be necessary when
				// we have a better low-level comparison function that returns
				// equal char counts for both parties! ].
				if (p_options != kMCStringOptionCompareExact)
					t_prefix_length = MCStrCharsSharedPrefixCaseless(p_needle -> chars, p_needle -> char_count, t_chars + t_offset, t_char_count - t_offset);
				else
					t_prefix_length = MCStrCharsSharedPrefixExact(p_needle -> chars, p_needle -> char_count, t_chars + t_offset, t_char_count - t_offset);

				// Build the range.
				r_result -> offset = p_range . offset + t_offset;
				r_result -> length = t_prefix_length;
			}

			return true;
		}
	}

	// If we get here then we didn't find the string we were looking for.
	return false;
}

static uindex_t MCStringCountStrChars(MCStringRef self, MCRange p_range, const strchar_t *p_needle_chars, uindex_t p_needle_char_count, MCStringOptions p_options)
{
	// Keep track of how many occurances have been found.
	uindex_t t_count;
	t_count = 0;

	// Compute the char ptr and length based on range.
	const strchar_t *t_chars;
	uindex_t t_char_count;
	t_chars = self -> chars + MCMin(p_range . offset, self -> char_count);
	t_char_count = MCMin(p_range . length, self -> char_count - (t_chars - self -> chars));
	
	// Loop through the char range checking for occurances of needle.
	uindex_t t_offset;
	t_offset = 0;
	while(t_offset < t_char_count)
	{
		// Compute the length of the shared prefix at the current offset.
		uindex_t t_prefix_length;
		if (p_options != kMCStringOptionCompareExact)
			t_prefix_length = MCStrCharsSharedPrefixCaseless(t_chars + t_offset, t_char_count - t_offset, p_needle_chars, p_needle_char_count);
		else
			t_prefix_length = MCStrCharsSharedPrefixExact(t_chars + t_offset, t_char_count - t_offset, p_needle_chars, p_needle_char_count);

		// If we find a match, increase the count and move past it, otherwise
		// just bump.
		if (t_prefix_length == p_needle_char_count)
		{
			t_offset += t_prefix_length;
			t_count += 1;
		}
		else
			t_offset += 1;
	}

	// Return the number of occurances.
	return t_count;
}

uindex_t MCStringCount(MCStringRef self, MCRange p_range, MCStringRef p_needle, MCStringOptions p_options)
{
	return MCStringCountStrChars(self, p_range, p_needle -> chars, p_needle -> char_count, p_options);
}

uindex_t MCStringCountChar(MCStringRef self, MCRange p_range, codepoint_t p_needle, MCStringOptions p_options)
{
	// We only support ASCII for now.
	MCAssert(p_needle < 128);
	
	strchar_t t_native_needle;
	t_native_needle = (strchar_t)p_needle;
	
	return MCStringCountStrChars(self, p_range, &t_native_needle, 1, p_options);
}

////////////////////////////////////////////////////////////////////////////////

bool MCStringDivideAtChar(MCStringRef self, codepoint_t p_separator, MCStringOptions p_options, MCStringRef& r_head, MCStringRef& r_tail)
{
	uindex_t t_offset;
	if (!MCStringFirstIndexOfChar(self, p_separator, 0, p_options, t_offset))
	{
		if (!MCStringCopy(self, r_head))
			return false;
		
		r_tail = MCValueRetain(kMCEmptyString);
		
		return true;
	}
	
	return MCStringDivideAtIndex(self, t_offset, r_head, r_tail);
}

bool MCStringDivideAtIndex(MCStringRef self, uindex_t p_offset, MCStringRef& r_head, MCStringRef& r_tail)
{
	MCStringRef t_head;
	if (!MCStringCopySubstring(self, MCRangeMake(0, p_offset), t_head))
		return false;
	
	MCStringRef t_tail;
	if (!MCStringCopySubstring(self, MCRangeMake(p_offset + 1, MCStringGetLength(self) - p_offset - 1), t_tail))
	{
		MCValueRelease(t_head);
		return false;
	}
	
	r_head = t_head;
	r_tail = t_tail;
	
	return true;
}

////////////////////////////////////////////////////////////////////////////////

bool MCStringBreakIntoChunks(MCStringRef self, codepoint_t p_separator, MCStringOptions p_options, MCRange*& r_ranges, uindex_t& r_range_count)
{
	MCAssert(p_separator < 128);
	
	uindex_t t_length;
	t_length = MCStringGetLength(self);
	
	// Count the number of chunks, adjusting for an empty trailing chunk.
	uindex_t t_range_count;
	t_range_count = MCStringCountChar(self, MCRangeMake(0, MCStringGetLength(self)), p_separator, p_options);
	if (t_length > 0 && MCStringGetNativeCharAtIndex(self, t_length - 1) == p_separator)
		t_range_count -= 1;
	
	// Allocate the range array.
	MCRange *t_ranges;
	if (!MCMemoryNewArray(t_range_count, t_ranges))
		return false;
	
	// Now compute the ranges.
	uindex_t t_prev_offset, t_offset, t_index;
	t_prev_offset = 0;
	t_offset = 0;
	t_index = 0;
	for(;;)
	{
		if (!MCStringFirstIndexOfChar(self, p_separator, t_prev_offset, p_options, t_offset))
		{
			t_ranges[t_index] . offset = t_prev_offset;
			t_ranges[t_index] . length = t_length - t_prev_offset;
			break;
		}
		
		t_ranges[t_index] . offset = t_prev_offset;
		t_ranges[t_index] . length = t_offset - t_prev_offset;
		
		t_prev_offset = t_offset + 1;
	}
	
	r_ranges = t_ranges;
	r_range_count = t_range_count;
	
	return true;
}

////////////////////////////////////////////////////////////////////////////////

bool MCStringFold(MCStringRef self, MCStringOptions p_options)
{
	MCAssert(MCStringIsMutable(self));

	// If we are looking for exact comparison then folding has no effect.
	if (p_options == kMCStringOptionCompareExact)
		return true;

    // Case-fold the string
    unichar_t *t_folded;
    uindex_t t_folded_length;
    if (!MCUnicodeCaseFold(self -> chars, self -> char_count, t_folded, t_folded_length))
        return false;
    
    // Update the string
    MCMemoryDelete(self -> chars);
    self -> chars = t_folded;
    self -> char_count = t_folded_length;

	__MCStringChanged(self);
	
	// We always succeed (at the moment)
	return true;
}

bool MCStringLowercase(MCStringRef self, MCLocaleRef p_locale)
{
	MCAssert(MCStringIsMutable(self));

	// Case transformations can change string lengths
    unichar_t *t_lowered;
    uindex_t t_lowered_length;
    if (!MCUnicodeLowercase(p_locale, self -> chars, self -> char_count, t_lowered, t_lowered_length))
        return false;
    
    MCMemoryDelete(self -> chars);
    self -> chars = t_lowered;
    self -> char_count = t_lowered_length;
	
	__MCStringChanged(self);
	
	// We always succeed (at the moment)
	return true;
}

bool MCStringUppercase(MCStringRef self, MCLocaleRef p_locale)
{
	MCAssert(MCStringIsMutable(self));
    
	// Case transformations can change string lengths
    unichar_t *t_lowered;
    uindex_t t_lowered_length;
    if (!MCUnicodeUppercase(p_locale, self -> chars, self -> char_count, t_lowered, t_lowered_length))
        return false;
    
    MCMemoryDelete(self -> chars);
    self -> chars = t_lowered;
    self -> char_count = t_lowered_length;
	
	__MCStringChanged(self);
	
	// We always succeed (at the moment)
	return true;
}

////////////////////////////////////////////////////////////////////////////////

bool MCStringAppend(MCStringRef self, MCStringRef p_suffix)
{
	MCAssert(MCStringIsMutable(self));

	// Only do the append now if self != suffix.
	if (self != p_suffix)
	{
		// Ensure we have enough room in self - with the gap at the end.
		if (!__MCStringExpandAt(self, self -> char_count, p_suffix -> char_count))
			return false;

		// Now copy the chars across (including the NUL).
		MCMemoryCopy(self -> chars + self -> char_count - p_suffix -> char_count, p_suffix -> chars, (p_suffix -> char_count + 1) * sizeof(strchar_t));
		
		__MCStringChanged(self);
		
		// We succeeded.
		return true;
	}

	// Otherwise copy and recurse.
	MCAutoStringRef t_suffix_copy;
	MCStringCopy(p_suffix, &t_suffix_copy);
	return MCStringAppend(self, *t_suffix_copy);
}

bool MCStringAppendSubstring(MCStringRef self, MCStringRef p_suffix, MCRange p_range)
{
	MCAssert(MCStringIsMutable(self));

	// Only do the append now if self != suffix.
	if (self != p_suffix)
	{
		__MCStringClampRange(p_suffix, p_range);

		// Ensure we have enough room in self - with the gap at the end.
		if (!__MCStringExpandAt(self, self -> char_count, p_range . length))
			return false;

		// Now copy the chars across.
		MCMemoryCopy(self -> chars + self -> char_count - p_range . length, p_suffix -> chars + p_range . offset, p_range . length * sizeof(strchar_t));

		// Set the NULL
		self -> chars[ self -> char_count ] = '\0';
		
		__MCStringChanged(self);
		
		// We succeeded.
		return true;
	}

	// Otherwise copy substring and append.
	MCAutoStringRef t_suffix_substring;
	return MCStringCopySubstring(p_suffix, p_range, &t_suffix_substring) &&
		MCStringAppend(self, *t_suffix_substring);
}

bool MCStringAppendNativeChars(MCStringRef self, const char_t *p_chars, uindex_t p_char_count)
{
	MCAssert(MCStringIsMutable(self));
	
	// Ensure we have enough room in self - with the gap at the end.
	if (!__MCStringExpandAt(self, self -> char_count, p_char_count))
		return false;
	
	// Now copy the chars across.
	for(uindex_t i = 0; i < p_char_count; i++)
		self -> chars[i + self -> char_count - p_char_count] = MCUnicodeCharMapFromNative(p_chars[i]);
	
	// Set the NULL
	self -> chars[self -> char_count] = '\0';
	
	__MCStringChanged(self);
	
	// We succeeded.
	return true;
}

bool MCStringAppendChars(MCStringRef self, const unichar_t *p_chars, uindex_t p_char_count)
{
	MCAssert(MCStringIsMutable(self));
	
	// Ensure we have enough room in self - with the gap at the end.
	if (!__MCStringExpandAt(self, self -> char_count, p_char_count))
		return false;
	
	// Now copy the chars across.
	MCMemoryCopy(self -> chars + self -> char_count - p_char_count, p_chars, p_char_count * sizeof(unichar_t));
	
	// Set the NULL
	self -> chars[self -> char_count] = '\0';
	
	__MCStringChanged(self);
	
	// We succeeded.
	return true;
}

bool MCStringAppendNativeChar(MCStringRef self, char_t p_char)
{
	return MCStringAppendNativeChars(self, &p_char, 1);
}

bool MCStringAppendChar(MCStringRef self, unichar_t p_char)
{
	return MCStringAppendChars(self, &p_char, 1);
}

bool MCStringPrepend(MCStringRef self, MCStringRef p_prefix)
{
	MCAssert(MCStringIsMutable(self));

	// Only do the prepend now if self != prefix.
	if (self != p_prefix)
	{
		// Ensure we have enough room in self - with the gap at the beginning.
		if (!__MCStringExpandAt(self, 0, p_prefix -> char_count))
			return false;

		// Now copy the chars across.
		MCMemoryCopy(self -> chars, p_prefix -> chars, p_prefix -> char_count * sizeof(strchar_t));
		
		__MCStringChanged(self);

		// We succeeded.
		return true;
	}

	// Otherwise copy and recurse.
	MCAutoStringRef t_prefix_copy;
	MCStringCopy(p_prefix, &t_prefix_copy);
	return MCStringPrepend(self, *t_prefix_copy);
}

bool MCStringPrependSubstring(MCStringRef self, MCStringRef p_prefix, MCRange p_range)
{
	MCAssert(MCStringIsMutable(self));

	// Only do the prepend now if self != prefix.
	if (self != p_prefix)
	{
		__MCStringClampRange(p_prefix, p_range);

		// Ensure we have enough room in self - with the gap at the beginning.
		if (!__MCStringExpandAt(self, 0, p_range . length))
			return false;

		// Now copy the chars across.
		MCMemoryCopy(self -> chars, p_prefix -> chars + p_range . offset, p_range . length * sizeof(strchar_t));
		
		__MCStringChanged(self);

		// We succeeded.
		return true;
	}

	// Otherwise copy substring and prepend.
	MCAutoStringRef t_prefix_substring;
	return MCStringCopySubstring(p_prefix, p_range, &t_prefix_substring) &&
		MCStringPrepend(self, *t_prefix_substring);
}

bool MCStringPrependNativeChars(MCStringRef self, const char_t *p_chars, uindex_t p_char_count)
{
	MCAssert(MCStringIsMutable(self));
	
	// Ensure we have enough room in self - with the gap at the beginning.
	if (!__MCStringExpandAt(self, 0, p_char_count))
		return false;
	
	// Now copy the chars across.
	for(uindex_t i = 0; i < p_char_count; i++)
		self -> chars[i] = MCUnicodeCharMapFromNative(p_chars[i]);
	
	__MCStringChanged(self);
	
	// We succeeded.
	return true;
}

bool MCStringPrependChars(MCStringRef self, const unichar_t *p_chars, uindex_t p_char_count)
{
	MCAssert(MCStringIsMutable(self));
	
	// Ensure we have enough room in self - with the gap at the end.
	if (!__MCStringExpandAt(self, 0, p_char_count))
		return false;
	
	// Now copy the chars across.
	MCMemoryCopy(self -> chars, p_chars, p_char_count * sizeof(unichar_t));
	
	__MCStringChanged(self);
	
	// We succeeded.
	return true;
}

bool MCStringPrependNativeChar(MCStringRef self, char_t p_char)
{
	return MCStringPrependNativeChars(self, &p_char, 1);
}

bool MCStringPrependChar(MCStringRef self, unichar_t p_char)
{
	return MCStringPrependChars(self, &p_char, 1);
}

bool MCStringInsert(MCStringRef self, uindex_t p_at, MCStringRef p_substring)
{
	MCAssert(MCStringIsMutable(self));

	if (self != p_substring)
	{
		p_at = MCMin(p_at, self -> char_count);

		// Ensure we have enough room in self - with the gap at 'at'.
		if (!__MCStringExpandAt(self, p_at, p_substring -> char_count))
			return false;

		// Now copy the chars across.
		MCMemoryCopy(self -> chars + p_at, p_substring -> chars, p_substring -> char_count * sizeof(strchar_t));
		
		__MCStringChanged(self);
		
		// We succeeded.
		return true;
	}

	// Otherwise copy and recurse.
	MCAutoStringRef t_substring_copy;
	MCStringCopy(p_substring, &t_substring_copy);
	return MCStringInsert(self, p_at, *t_substring_copy);
}

bool MCStringInsertSubstring(MCStringRef self, uindex_t p_at, MCStringRef p_substring, MCRange p_range)
{
	MCAssert(MCStringIsMutable(self));
	
	// Only do the prepend now if self != prefix.
	if (self != p_substring)
	{
		p_at = MCMin(p_at, self -> char_count);
		
		// Ensure we have enough room in self - with the gap at the beginning.
		if (!__MCStringExpandAt(self, p_at, p_range . length))
			return false;
		
		// Now copy the chars across.
		MCMemoryCopy(self -> chars + p_at, p_substring -> chars + p_range . offset, p_range . length * sizeof(strchar_t));
		
		__MCStringChanged(self);
		
		// We succeeded.
		return true;
	}
	
	// Otherwise copy substring and prepend.
	MCAutoStringRef t_substring_substring;
	return MCStringCopySubstring(p_substring, p_range, &t_substring_substring) &&
		MCStringInsert(self, p_at, *t_substring_substring);
}

bool MCStringInsertNativeChars(MCStringRef self, uindex_t p_at, const char_t *p_chars, uindex_t p_char_count)
{
	MCAssert(MCStringIsMutable(self));
	
	p_at = MCMin(p_at, self -> char_count);

	// Ensure we have enough room in self - with the gap at p_at.
	if (!__MCStringExpandAt(self, p_at, p_char_count))
		return false;
	
	// Now copy the chars across.
	for(uindex_t i = 0; i < p_char_count; i++)
		self -> chars[p_at + i] = MCUnicodeCharMapFromNative(p_chars[i]);
	
	__MCStringChanged(self);
	
	// We succeeded.
	return true;
}

bool MCStringInsertChars(MCStringRef self, uindex_t p_at, const unichar_t *p_chars, uindex_t p_char_count)
{
	MCAssert(MCStringIsMutable(self));
	
	p_at = MCMin(p_at, self -> char_count);
	
	// Ensure we have enough room in self - with the gap at the p_at.
	if (!__MCStringExpandAt(self, p_at, p_char_count))
		return false;
	
	// Now copy the chars across.
	MCMemoryCopy(self -> chars + p_at, p_chars, p_char_count * sizeof(unichar_t));
	
	__MCStringChanged(self);
	
	// We succeeded.
	return true;
}

bool MCStringInsertNativeChar(MCStringRef self, uindex_t p_at, char_t p_char)
{
	return MCStringInsertNativeChars(self, p_at, &p_char, 1);
}

bool MCStringInsertChar(MCStringRef self, uindex_t p_at, unichar_t p_char)
{
	return MCStringInsertChars(self, p_at, &p_char, 1);
}

bool MCStringRemove(MCStringRef self, MCRange p_range)
{
	MCAssert(MCStringIsMutable(self));

	__MCStringClampRange(self, p_range);

	// Copy down the chars above the string taking into account the implicit
	// NUL.
	__MCStringShrinkAt(self, p_range . offset, p_range . length);
	
	__MCStringChanged(self);
	
	// We succeeded.
	return true;
}

bool MCStringSubstring(MCStringRef self, MCRange p_range)
{
	MCAssert(MCStringIsMutable(self));
    
	__MCStringClampRange(self, p_range);
    
	// Remove the surrounding chars.
    // On the left if necessary
    if (p_range . offset != 0)
    {
        __MCStringShrinkAt(self, 0, p_range . offset);
        p_range . offset = 0;
    }
    
    // And on the right if necessary
    if (p_range . offset + p_range . length != self -> char_count)
        __MCStringShrinkAt(self, p_range . length, self -> char_count - p_range . length);
    
	// We succeeded.
	return true;
}

bool MCStringReplace(MCStringRef self, MCRange p_range, MCStringRef p_replacement)
{
	MCAssert(MCStringIsMutable(self));

	if (self != p_replacement)
	{
		__MCStringClampRange(self, p_range);

		// Work out the new size of the string.
		uindex_t t_new_char_count;
		t_new_char_count = self -> char_count - p_range . length + p_replacement -> char_count;

		if (t_new_char_count > self -> char_count)
		{
			// Expand the string at the end of the range by the amount extra we
			// need.
			if (!__MCStringExpandAt(self, p_range . offset + p_range . length, t_new_char_count - self -> char_count))
				return false;
		}
		else if (t_new_char_count < self -> char_count)
		{
			// Shrink the last part of the range by the amount less we need.
			__MCStringShrinkAt(self, p_range . offset + (p_range . length - (self -> char_count - t_new_char_count)), (self -> char_count - t_new_char_count));
		}

		// Copy across the replacement chars.
		MCMemoryCopy(self -> chars + p_range . offset, p_replacement -> chars, p_replacement -> char_count * sizeof(strchar_t));
		
		__MCStringChanged(self);
		
		// We succeeded.
		return true;
	}

	MCAutoStringRef t_replacement_copy;
	MCStringCopy(p_replacement, &t_replacement_copy);
	return MCStringReplace(self, p_range, *t_replacement_copy);
}

bool MCStringPad(MCStringRef self, uindex_t p_at, uindex_t p_count, MCStringRef p_value)
{
	if (!__MCStringExpandAt(self, p_at, p_count * (p_value != nil ? p_value -> char_count : 1)))
		return false;

	if (p_value != nil)
		for(uindex_t i = 0; i < p_count; i++)
			MCMemoryCopy(self -> chars + p_at + i * p_value -> char_count, p_value -> chars, p_value -> char_count * sizeof(strchar_t));
	
	__MCStringChanged(self);
	
	return true;
}

////////////////////////////////////////////////////////////////////////////////

bool MCStringAppendFormat(MCStringRef self, const char *p_format, ...)
{
	bool t_success;
	va_list t_args;
	va_start(t_args, p_format);
	t_success = MCStringAppendFormatV(self, p_format, t_args);
	va_end(t_args);
	return t_success;
}

bool MCStringAppendFormatV(MCStringRef self, const char *p_format, va_list p_args)
{
	MCAutoStringRef t_formatted_string;
	if (!MCStringFormatV(&t_formatted_string, p_format, p_args))
		return false;

	return MCStringAppend(self, *t_formatted_string);
}

////////////////////////////////////////////////////////////////////////////////

static void split_find_end_of_element_exact(const strchar_t *sptr, const strchar_t *eptr, codepoint_t del, const strchar_t*& r_end_ptr)
{
	uindex_t t_index;
    if (!MCUnicodeFirstIndexOfChar(sptr, eptr-sptr, del, kMCUnicodeCompareOptionExact, t_index))
        t_index = eptr-sptr;
    
    r_end_ptr = sptr + t_index;
}

static void split_find_end_of_element_caseless(const strchar_t *sptr, const strchar_t *eptr, codepoint_t del, const strchar_t*& r_end_ptr)
{
    uindex_t t_index;
    if (!MCUnicodeFirstIndexOfChar(sptr, eptr-sptr, del, kMCUnicodeCompareOptionCaseless, t_index))
        t_index = eptr-sptr;
    
    r_end_ptr = sptr + t_index;
}

static void split_find_end_of_element_and_key_exact(const strchar_t *sptr, const strchar_t *eptr, strchar_t del, strchar_t key, const strchar_t*& r_key_ptr, const strchar_t *& r_end_ptr)
{
	// Not as fast as it could be...
    uindex_t t_key_idx, t_del_idx;
    if (!MCUnicodeFirstIndexOfChar(sptr, eptr-sptr, key, kMCUnicodeCompareOptionExact, t_key_idx))
        t_key_idx = eptr-sptr;
    if (!MCUnicodeFirstIndexOfChar(sptr, eptr-sptr, del, kMCUnicodeCompareOptionExact, t_del_idx))
        t_del_idx = eptr-sptr;
    
    if (t_key_idx > t_del_idx)
    {
        // Delimiter came before the key
        r_key_ptr = r_end_ptr = sptr + t_del_idx;
        return;
    }
    
    r_key_ptr = sptr + t_key_idx;
    split_find_end_of_element_exact(sptr, eptr, del, r_end_ptr);
}

static void split_find_end_of_element_and_key_caseless(const strchar_t *sptr, const strchar_t *eptr, strchar_t del, strchar_t key, const strchar_t*& r_key_ptr, const strchar_t *& r_end_ptr)
{
	// Not as fast as it could be...
    uindex_t t_key_idx, t_del_idx;
    if (!MCUnicodeFirstIndexOfChar(sptr, eptr-sptr, key, kMCUnicodeCompareOptionCaseless, t_key_idx))
        t_key_idx = eptr-sptr;
    if (!MCUnicodeFirstIndexOfChar(sptr, eptr-sptr, del, kMCUnicodeCompareOptionCaseless, t_del_idx))
        t_del_idx = eptr-sptr;
    
    if (t_key_idx > t_del_idx)
    {
        // Delimiter came before the key
        r_key_ptr = r_end_ptr = sptr + t_del_idx;
        return;
    }
    
    r_key_ptr = sptr + t_key_idx;
    split_find_end_of_element_exact(sptr, eptr, del, r_end_ptr);
}

bool MCStringSplit(MCStringRef self, MCStringRef p_elem_del, MCStringRef p_key_del, MCStringOptions p_options, MCArrayRef& r_array)
{
	if (MCStringGetLength(p_elem_del) != 1 ||
		(p_key_del != nil && MCStringGetLength(p_key_del) != 1))
		return false;

	if (self -> char_count == 0)
	{
		r_array = MCValueRetain(kMCEmptyArray);
		return true;
	}

	MCAutoArrayRef t_array;
	if (!MCArrayCreateMutable(&t_array))
		return false;

	strchar_t t_echar, t_kchar;
	t_echar = p_elem_del -> chars[0];
	if (p_key_del != nil)
		t_kchar = p_key_del -> chars[0];

	const strchar_t *t_sptr;
	const strchar_t *t_eptr;
	t_sptr = self -> chars;
	t_eptr = self -> chars + self -> char_count;
	if (p_key_del == nil)
	{
		uindex_t t_index;
		t_index = 1;
		for(;;)
		{
			const strchar_t *t_element_end;
			if (p_options == kMCStringOptionCompareExact)
				split_find_end_of_element_exact(t_sptr, t_eptr, t_echar, t_element_end);
			else
				split_find_end_of_element_caseless(t_sptr, t_eptr, t_echar, t_element_end);
			
			MCAutoStringRef t_string;
			if (!MCStringCreateWithChars(t_sptr, t_element_end - t_sptr, &t_string))
				return false;

			if (!MCArrayStoreValueAtIndex(*t_array, t_index, *t_string))
				return false;

			if (t_element_end + 1 >= t_eptr)
				break;

			t_index += 1;

			t_sptr = t_element_end + 1;
		}
	}
	else
	{
		for(;;)
		{
			const strchar_t *t_element_end;
			const strchar_t *t_key_end;
			if (p_options == kMCStringOptionCompareExact)
				split_find_end_of_element_and_key_exact(t_sptr, t_eptr, t_echar, t_kchar, t_key_end, t_element_end);
			else
				split_find_end_of_element_and_key_caseless(t_sptr, t_eptr, t_echar, t_kchar, t_key_end, t_element_end);
			
			MCNewAutoNameRef t_name;
			if (!MCNameCreateWithChars(t_sptr, t_key_end - t_sptr, &t_name))
				return false;	

			if (t_key_end != t_element_end)
				t_key_end += 1;

			MCAutoStringRef t_string;
			if (!MCStringCreateWithChars(t_key_end, t_element_end - t_key_end, &t_string))
				return false;

			if (!MCArrayStoreValue(*t_array, true, *t_name, *t_string))
				return false;

			if (t_element_end + 1 >= t_eptr)
				break;

			t_sptr = t_element_end + 1;
		}
	}

	if (!MCArrayCopy(*t_array, r_array))
		return false;

	return true;
}

bool MCStringFindAndReplaceChar(MCStringRef self, codepoint_t p_pattern, codepoint_t p_replacement, MCStringOptions p_options)
{
	// Can the replacement be done in-place? Reasons it might not be possible:
    //
    //  (x) The UTF-16 encoding of the codepoints are different lengths
    //  (x) Normalisation is required and one or both codepoints are composed
    //  (x) Case folding is required and the case mapping is not simple
    //
    //This can only be true if both
    // codepoints have the same length when encoded. It also isn't possible if
    // normalisation is required and one or both codepoints are composed. Another
    if
    (
        /*// One is BMP and one is non-BMP
        ((!(p_pattern & 0x1F0000)) ^ (!(p_replacement & 0x1F0000)))
     
        // Normalisation has been requested and either the pattern or replacement
        // will potentially change under normalisation
        || ((p_options == kMCStringOptionCompareCaseless || p_options == kMCStringOptionCompareNonliteral)
            && (MCUnicodeIsComposed(p_pattern) || MCUnicodeIsComposed(p_replacement)))
     
        // Case folding changes the length of either the pattern or replacement
        || ((p_options == kMCStringOptionCompareCaseless)
            && (MCUnicodeGetBinaryProperty(p_pattern, kMCUni)
                || !MCUnicodeGetBinaryProperty(p_replacement, kMCUnicodePropertyChangesWhenCaseFolded)))
        */
     
        // Don't bother doing it the slow way yet; do it fast + wrong instead...
        false
    )
    {
        // Do it via the slow-path full string replacement
        MCAutoStringRef t_pattern, t_replacement;
        unichar_t t_buffer[2];
        /* UNCHECKED */ MCStringCreateWithChars(t_buffer, __MCStringCodepointToSurrogates(p_pattern, t_buffer), &t_pattern);
        /* UNCHECKED */ MCStringCreateWithChars(t_buffer, __MCStringCodepointToSurrogates(p_replacement, t_buffer), &t_replacement);
        return MCStringFindAndReplace(self, *t_pattern, *t_replacement, p_options);
    }
    
    // Are we dealing with a surrogate pair?
    unichar_t t_pattern_units[2];
    unichar_t t_replacement_units[2];
    bool t_pair;
    t_pair = __MCStringCodepointToSurrogates(p_pattern, t_pattern_units) == 2;
    __MCStringCodepointToSurrogates(p_replacement, t_replacement_units);
    
    // Which type of comparison are we using?
    if (p_options == kMCStringOptionCompareExact)
    {
        for (uindex_t i = 0; i < self -> char_count; i++)
        {
            if (self -> chars[i] == t_pattern_units[0]
                && (!t_pair ||
                    (i + 1 < self -> char_count && self -> chars[i + 1] == t_pattern_units[1])))
            {
                // Found a match
                self -> chars[i] = t_replacement_units[0];
                if (t_pair)
                    self -> chars[++i] = t_replacement_units[1];
            }
        }
    }
    else if (p_options == kMCStringOptionCompareCaseless)
    {
        for (uindex_t i = 0; i < self -> char_count; i++)
        {
            codepoint_t t_char;
            if (__MCStringIsValidSurrogatePair(self, i))
                t_char = __MCStringSurrogatesToCodepoint(self -> chars[i], self -> chars[i + 1]);
            else
                t_char = self -> chars[i];
            
            if (MCUnicodeGetCharacterProperty(t_char, kMCUnicodePropertySimpleCaseFolding)
                == MCUnicodeGetCharacterProperty(p_pattern, kMCUnicodePropertySimpleCaseFolding))
            {
                // Found a match
                self -> chars[i] = t_replacement_units[0];
                if (t_pair)
                    self -> chars[++i] = t_replacement_units[1];
            }
        }
    }
    else if (p_options == kMCStringOptionCompareNonliteral)
    {
        MCAssert(false);
        return false;
    }
    else
    {
        MCAssert(false);
        return false;
    }
    
    return true;
}

bool MCStringFindAndReplace(MCStringRef self, MCStringRef p_pattern, MCStringRef p_replacement, MCStringOptions p_options)
{
	if (p_pattern -> char_count == 1 && p_replacement -> char_count == 1)
		return MCStringFindAndReplaceChar(self, p_pattern -> chars[0], p_replacement -> chars[0], p_options);
	
	if (self -> char_count != 0)
	{
		strchar_t *t_output;
		uindex_t t_output_length;
		uindex_t t_output_capacity;
		uindex_t t_offset;

		t_output = nil;
		t_output_length = 0;
		t_output_capacity = 0;
		t_offset = 0;

		for(;;)
		{
			// Search for the next occurence of from in whole.
			uindex_t t_next_offset;
			bool t_found;
			t_found = MCStringFirstIndexOf(self, p_pattern, t_offset, p_options == kMCStringOptionCompareCaseless, t_next_offset);
			
			// If we found an instance of from, then we need space for to; otherwise,
			// we update the offset, and need just room up to it.
			uindex_t t_space_needed;
			if (t_found)
				t_space_needed = t_next_offset + p_replacement -> char_count;
			else
			{
				t_next_offset = self -> char_count;
				t_space_needed = t_next_offset;
			}

			// Expand the buffer as necessary.
			if (t_output_length + t_space_needed > t_output_capacity)
			{
				if (t_output_capacity == 0)
					t_output_capacity = 4096;
					
				while(t_output_length + t_space_needed + 1 > t_output_capacity)
					t_output_capacity *= 2;
				
				if (!MCMemoryReallocate(t_output, t_output_capacity * sizeof(strchar_t), t_output))
				{
					MCMemoryDeallocate(t_output);
					return false;
				}
			}
			// Copy in self, up to the offset.
			memcpy(t_output + t_output_length, self -> chars + t_offset, (t_next_offset - t_offset) * sizeof(strchar_t));
			t_output_length += t_next_offset - t_offset;

			// No more occurences were found, so we are done.
			if (!t_found)
				break;
				
			// Now copy in replacement.
			memcpy(t_output + t_output_length, p_replacement -> chars, p_replacement -> char_count * sizeof(strchar_t));
			t_output_length += p_replacement -> char_count;

			// Update offset
			t_offset = t_next_offset + p_pattern -> char_count;	
		}
	
		// Add the implicit NUL
		t_output[t_output_length] = '\0';

		MCMemoryDeallocate(self -> chars);

		self -> chars = t_output;
		self -> char_count = t_output_length;
		self -> capacity = t_output_capacity;
		
		__MCStringChanged(self);
	}

	return true;
}

////////////////////////////////////////////////////////////////////////////////

void __MCStringDestroy(__MCString *self)
{
	MCMemoryDeleteArray(self -> chars);
	MCMemoryDeleteArray(self -> native_chars);
}

bool __MCStringCopyDescription(__MCString *self, MCStringRef& r_desc)
{
	return MCStringFormat(r_desc, "\"%@\"", self);
}

hash_t __MCStringHash(__MCString *self)
{
	return MCStringHash(self, kMCStringOptionCompareExact);
}

bool __MCStringIsEqualTo(__MCString *self, __MCString *p_other_self)
{
	return MCStringIsEqualTo(self, p_other_self, kMCStringOptionCompareExact);
}

bool __MCStringImmutableCopy(__MCString* self, bool p_release, __MCString*& r_immutable_value)
{
	if (!p_release)
		return MCStringCopy(self, r_immutable_value);

	return MCStringCopyAndRelease(self, r_immutable_value);
}

////////////////////////////////////////////////////////////////////////////////

static void __MCStringClampRange(MCStringRef self, MCRange& x_range)
{
	uindex_t t_left, t_right;
	t_left = MCMin(x_range . offset, self -> char_count);
	t_right = MCMin(x_range . offset + MCMin(x_range . length, UINDEX_MAX - x_range . offset), self -> char_count);
	x_range . offset = t_left;
	x_range . length = t_right - t_left;
}

/*static uindex_t *__MCStringGetCapacityPtr(MCStringRef self)
{
	// In mutable strings, the capacity is stored after the chars at the
	// first aligned location.
	uindex_t *t_capacity_ptr;
	t_capacity_ptr = (uindex_t *)(self -> chars + ((self -> char_count + sizeof(uindex_t) - 1) & ~(sizeof(uindex_t) - 1)));
	return t_capacity_ptr;
}*/

static uindex_t __MCStringGetCapacity(MCStringRef self)
{
	return self -> capacity;

	/*if (self -> chars != nil)
		return *(__MCStringGetCapacityPtr(self));

	// An uninitialized string (or one that has had its buffer deleted) will
	// have no chars ptr, so just return 0.
	return 0;*/
}

static bool __MCStringExpandAt(MCStringRef self, uindex_t p_at, uindex_t p_count)
{
	// Fetch the capacity.
	uindex_t t_capacity;
	t_capacity = __MCStringGetCapacity(self);

	// The capacity field stores the total number of chars that could fit not
	// including the implicit NUL, so if we fit, we can fast-track.
	if (t_capacity != 0 && self -> char_count + p_count <= t_capacity)
	{
		// Shift up the chars above - including the implicit NUL.
		MCMemoryMove(self -> chars + p_at + p_count, self -> chars + p_at, ((self -> char_count + 1) - p_at) * sizeof(strchar_t));

		// Increase the char_count.
		self -> char_count += p_count;
		
		// Rewrite the capacity.
		//*(__MCStringGetCapacityPtr(self)) = t_capacity;

		// We succeeded.
		return true;
	}

	// If we get here then we need to reallocate first.

	// Base capacity - current length + inserted length + implicit NUL.
	uindex_t t_new_capacity;
	t_new_capacity = self -> char_count + p_count + 1;

	// Capacity rounded up to nearest aligned boundary and then space for
	// the capacity field.
	//t_new_capacity = ((t_new_capacity + sizeof(uindex_t) - 1) & (~(sizeof(uindex_t) - 1))) + sizeof(uindex_t);

	// Capacity rounded up to a suitable boundary (at some point this should
	// be a function of the string's size).
	t_new_capacity = (t_new_capacity + 63) & ~63;

	// Reallocate.
	if (!MCMemoryReallocate(self -> chars, t_new_capacity * sizeof(strchar_t), self -> chars))
		return false;

	// Shift up the chars above - including the implicit NUL.
	MCMemoryMove(self -> chars + p_at + p_count, self -> chars + p_at, ((self -> char_count + 1) - p_at) * sizeof(strchar_t));

	// Increase the char_count.
	self -> char_count += p_count;

	// Update the capacity - notice that we shave off the capacity field itself
	// since it represents the char capacity rather than the size of the alloc'd
	// block. We also shave off room for the implicit NUL.
	self -> capacity = t_new_capacity - 1;
	//*(__MCStringGetCapacityPtr(self)) = t_new_capacity - 1 - sizeof(uindex_t);

	// We succeeded.
	return true;
}

static void __MCStringShrinkAt(MCStringRef self, uindex_t p_at, uindex_t p_count)
{
	// Shift the chars above 'at' down to remove 'count', remembering to include
	// the implicit NUL.
	MCMemoryMove(self -> chars + p_at, self -> chars + (p_at + p_count), (self -> char_count - (p_at + p_count) + 1) * sizeof(strchar_t));

	// Now adjust the length of the string.
	self -> char_count -= p_count;

	// TODO: Shrink the buffer if its too big.
}

static void __MCStringNativize(MCStringRef self)
{
	if (self -> native_chars != nil)
		return;
	
	/* UNCHECKED */ MCMemoryNewArray(self -> char_count + 1, self -> native_chars);
	
	bool t_is_native;
	t_is_native = true;
	for(uindex_t i = 0; i < self -> char_count; i++)
	{
		if (MCUnicodeCharMapToNative(self -> chars[i], self -> native_chars[i]))
			continue;
		
		self -> native_chars[i] = '?';
		t_is_native = false;
	}
	
	if (t_is_native)
		self -> flags |= kMCStringFlagIsNative;
	else
		self -> flags &= ~kMCStringFlagIsNative;
}

static void __MCStringChanged(MCStringRef self)
{
	// String changed to assume that it is no longer simple
    self -> flags &= ~kMCStringFlagIsSimple;
    
    MCMemoryDeleteArray(self -> native_chars);
	self -> native_chars = nil;
}

static codepoint_t __MCStringSurrogatesToCodepoint(unichar_t p_lead, unichar_t p_trail)
{
    return 0x10000 + ((p_lead & 0x3FF) << 10) + (p_trail & 0x3FF);
}

static unsigned int __MCStringCodepointToSurrogates(codepoint_t p_codepoint, unichar_t (&r_units)[2])
{
    if (p_codepoint > 0xFFFF)
    {
        p_codepoint -= 0x10000;
        r_units[0] = 0xD800 + (p_codepoint >> 10);
        r_units[1] = 0xDC00 + (p_codepoint & 0x3FF);
        return 2;
    }
    else
    {
        r_units[0] = p_codepoint;
        return 1;
    }
}

static bool __MCStringIsValidSurrogatePair(MCStringRef self, uindex_t p_index)
{
    // Check that the string is long enough
    if ((p_index + 1) >= self -> char_count)
        return false;
    
    // Check for a valid leading surrogate
    unichar_t t_char;
    t_char = self -> chars[p_index];
    if (t_char < 0xD800 || t_char > 0xDBFF)
        return false;
    
    // Check for a valid trailFing surrogate
    t_char = self -> chars[p_index + 1];
    if (t_char < 0xDC00 || t_char > 0xDFFF)
        return false;
    
    // All the checks passed
    return true;
}
	
////////////////////////////////////////////////////////////////////////////////

MCStringRef kMCEmptyString;
MCStringRef kMCTrueString;
MCStringRef kMCFalseString;
MCStringRef kMCMixedString;

bool __MCStringInitialize(void)
{
	if (!MCStringCreateWithNativeChars((const char_t *)"", 0, kMCEmptyString))
		return false;

	if (!MCStringCreateWithNativeChars((const char_t *)"true", 4, kMCTrueString))
		return false;

	if (!MCStringCreateWithNativeChars((const char_t *)"false", 5, kMCFalseString))
		return false;
    
    if (!MCStringCreateWithNativeChars((const char_t *)"mixed", 5, kMCMixedString))
		return false;

	return true;
}

void __MCStringFinalize(void)
{
	MCValueRelease(kMCFalseString);
	kMCFalseString = nil;
	MCValueRelease(kMCTrueString);
	kMCTrueString = nil;
	MCValueRelease(kMCEmptyString);
	kMCEmptyString = nil;
    MCValueRelease(kMCMixedString);
    kMCMixedString = nil;
}

////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////

#ifdef __LINUX__

#define ALLOC_PAD 64
static bool do_iconv(iconv_t fd, const char *in, size_t in_len, char * &out, size_t &out_len)
{
	// Begin conversion. As a start, assume both encodings take the same
	// space. This is probably wrong but the array is grown as needed.
	size_t t_status = 0;
	uindex_t t_alloc_remain = 0;
    char * t_out;
	char * t_out_cursor;

    t_out = (char*)malloc(in_len);
    if (t_out == nil)
        return false;

    t_alloc_remain = in_len;
    t_out_cursor = t_out;

	while (in_len > 0)
	{
		// Resize the destination array if it has been exhausted
		t_status = iconv(fd, (char**)&in, &in_len, &t_out_cursor, &t_alloc_remain);
		
		// Did the conversion fail?
		if (t_status == (size_t)-1)
		{
			// Insufficient output space errors can be fixed and retried
			if (errno == E2BIG)
			{
				// Increase the size of the output array
				uindex_t t_offset;
                t_offset = t_out_cursor - t_out;

                char *t_new_out = (char*)realloc(t_out, t_offset + t_alloc_remain + ALLOC_PAD);
                if (t_new_out == nil)
                {
                    free(t_out);
                    return false;
                }

                t_out = t_new_out;
				
				// Adjust the pointers because the output buffer may have moved
                t_out_cursor = t_out + t_offset;
                t_alloc_remain += ALLOC_PAD;		// Remaining size, not total size
				
				// Try the conversion again
				continue;
			}
			else
			{
				// The error is one of the following:
				//	EILSEQ	-	input byte invalid for input encoding
				//	EINVAL	-	incomplete multibyte character at end of input
				//	EBADF	-	invalid conversion file descriptor
				// None of these are recoverable so abort
                free(t_out);
				return false;
			}
		}
		else
		{
			// No error, conversion should be complete.
			MCAssert(in_len == 0);
		}
	}
    
	// Conversion has been completed
    out_len = t_out_cursor - t_out;
    out = t_out;
	return true;
}

bool MCStringCreateWithSysString(const char *p_system_string, MCStringRef &r_string)
{
    // Is the string empty?
    if (p_system_string == nil)
    {
        r_string = MCValueRetain(kMCEmptyString);
        return true;
    }

    // What is the system character encoding?
    //
    // Doing this here is unpleasant but the MCString*SysString functions are
    // needed before the libfoundation initialise call is made
    if (__MCSysCharset == nil)
    {
        setlocale(LC_CTYPE, "");
        __MCSysCharset = nl_langinfo(CODESET);
    }

    // Create the pseudo-FD that iconv uses for character conversion. The most
	// convenient form is UTF-16 as StringRefs can be constructed directly from that.
#ifdef __LITTLE_ENDIAN__
    iconv_t t_fd = iconv_open("UTF-16LE", __MCSysCharset);
#else
    iconv_t t_fd = iconv_open("UTF-16BE", __MCSysCharset);
#endif
	
    // Was creation of the iconv FD successful?
    if (t_fd == (iconv_t)-1)
        return false;

    // Measure the string
    size_t t_len;
    t_len = strlen(p_system_string);

	// Convert the string
	char *t_utf16_bytes;
	size_t t_utf16_byte_len;
	bool t_success;
    t_success = do_iconv(t_fd, p_system_string, t_len, t_utf16_bytes, t_utf16_byte_len);
	iconv_close(t_fd);
	
	if (!t_success)
		return false;
	
	// Create the StringRef
	MCStringRef t_string;
	t_success = MCStringCreateWithBytes((const byte_t*)t_utf16_bytes, t_utf16_byte_len, kMCStringEncodingUTF16, false, t_string);
	MCMemoryDeleteArray(t_utf16_bytes);
	
	if (!t_success)
		return false;
	
	r_string = t_string;
	return true;
}

bool MCStringConvertToSysString(MCStringRef p_string, const char * &r_system_string)
{
    // Create the pseudo-FD that iconv uses for character conversion. For
	// efficiency, convert straight from the internal format.
	iconv_t t_fd;
	const char *t_mc_string;
	size_t t_mc_len;
	if (MCStringIsNative(p_string) && MCStringGetNativeCharPtr(p_string) != nil)
	{
        t_fd = iconv_open(__MCSysCharset, "ISO-8859-1");
		t_mc_string = (const char *)MCStringGetNativeCharPtr(p_string);
		t_mc_len = MCStringGetLength(p_string);
	}
	else
	{
#ifdef __LITTLE_ENDIAN__
        t_fd = iconv_open(__MCSysCharset, "UTF-16LE");
#else
        t_fd = iconv_open(__MCSysCharset, "UTF-16BE");
#endif
		t_mc_string = (const char *)MCStringGetCharPtr(p_string);
		t_mc_len = MCStringGetLength(p_string) * sizeof(unichar_t);
	}

    // Was creation of the iconv FD successful?
    if (t_fd == (iconv_t)-1)
        return false;
	
	// Perform the conversion
	bool t_success;
	char *t_sys_string;
	size_t t_sys_len;
	t_success = do_iconv(t_fd, t_mc_string, t_mc_len, t_sys_string, t_sys_len);
	iconv_close(t_fd);
	
	if (!t_success)
		return false;
	
    // iconv doesn't append a null character
    char *t_term = (char*)realloc(t_sys_string, t_sys_len + 1);
    if (t_term == nil)
    {
        free(t_sys_string);
        return false;
    }
    t_sys_string = t_term;
    t_sys_string[t_sys_len] = '\0';

	r_system_string = t_sys_string;
	return true;
}

#endif
