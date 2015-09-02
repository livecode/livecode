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

#include "foundation-private.h"
#include "foundation-bidi.h"

#ifdef __LINUX__
#include <errno.h>
#include <iconv.h>
#include <langinfo.h>
#include <locale.h>
#endif

#ifdef __WINDOWS__
#include <Windows.h>
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

// This method forces a nativization of a string even if there is already a native char ptr.
static uindex_t __MCStringNativize(MCStringRef string);

// This method ensures there is a unichar ptr.
static void __MCStringUnnativize(MCStringRef self);

// This method marks the string as changed.
static void __MCStringChanged(MCStringRef string, uindex_t simple = kMCStringFlagNoChange, uindex_t combined = kMCStringFlagNoChange, uindex_t native = kMCStringFlagNoChange);

// Creates an indirect mutable string with contents.
static bool __MCStringCreateIndirect(__MCString *contents, __MCString*& r_string);

// Returns true if the string is indirect.
static bool __MCStringIsIndirect(__MCString *self);

static bool __MCStringMakeImmutable(__MCString *self);

// Creates an immutable string from this one, changing 'self' to indirect.
static bool __MCStringMakeIndirect(__MCString *self);

// Ensures the given mutable but indirect string is direct.
static bool __MCStringResolveIndirect(__MCString *self);

// Makes direct mutable string indirect, referencing r_new_string.
static bool __MCStringCopyMutable(__MCString *self, __MCString*& r_new_string);

// Copy the given unicode chars into the target unicode buffer and return true
// if all the chars being copied in could be native.
static bool __MCStringCopyChars(unichar_t *target, const unichar_t *source, uindex_t count, bool target_can_be_native);

////////////////////////////////////////////////////////////////////////////////

// AL-2015-02-06: [[ Bug 14504 ]] Add wrappers for string flag and length checking,
// for internal use when a string is known to be direct.
static bool __MCStringIsNative(MCStringRef self)
{
    MCAssert(!__MCStringIsIndirect(self));
    
    return (self -> flags & kMCStringFlagIsNotNative) == 0;
}

static bool __MCStringIsChecked(MCStringRef self)
{
    MCAssert(!__MCStringIsIndirect(self));
    
    return (self -> flags & kMCStringFlagIsChecked) != 0;
}

static bool __MCStringIsUncombined(MCStringRef self)
{
    MCAssert(!__MCStringIsIndirect(self));
    
    return (self -> flags & kMCStringFlagIsUncombined) != 0;
}

static bool __MCStringIsSimple(MCStringRef self)
{
    MCAssert(!__MCStringIsIndirect(self));
    
    return (self -> flags & kMCStringFlagIsSimple) != 0;
}

static bool __MCStringCanBeNative(MCStringRef self)
{
    MCAssert(!__MCStringIsIndirect(self));
    
    return (self -> flags & kMCStringFlagIsNotNative) == 0 || (self -> flags & kMCStringFlagCanBeNative) != 0;
}

static bool __MCStringCantBeEqualToNative(MCStringRef self, MCStringOptions p_options)
{
    // If self can't be native, then we check the comparison options to see if
    // it could still be native after normalization.
    if (!__MCStringCanBeNative(self))
    {
        // At this point self must contain unicode characters which don't directly
        // map to native. Thus the only way we could be equal to a native string is
        // if we contain combining sequences which compose to a native char.
        switch(p_options)
        {
            case kMCStringOptionCompareExact:
            case kMCStringOptionCompareFolded:
                // If no normalization is taking place, then no composition can occur
                // so we can't be equal to native.
                return true;
                
            case kMCStringOptionCompareNonliteral:
            case kMCStringOptionCompareCaseless:
                // If the string has been checked then we have more information.
                if (__MCStringIsChecked(self))
                {
                    // If there are no combining chars, then normalization is not
                    // going to make a difference - there's no way this string
                    // can be native.
                    if (__MCStringIsUncombined(self))
                        return true;
                
                    // If the string is not simple, then even though it contains
                    // combining chars it can't be native.
                    if (!__MCStringIsSimple(self))
                        return true;
                }
                break;
            
            default:
                MCUnreachable();
                break;
        }
    }
    
    return false;
}

static bool __MCStringCopyChars(unichar_t *p_dst, const unichar_t *p_src, uindex_t p_count, bool p_dst_can_be_native)
{
    // If the dst cannot be native, then there's no point checking if the src can be.
    if (!p_dst_can_be_native)
    {
        MCMemoryCopy(p_dst, p_src, p_count * sizeof(unichar_t));
        return false;
    }
    
    // Copy the unicode chars to our dst checking if we can nativize as we go.
    for(uindex_t i = 0; i < p_count; i++)
    {
        // If we fail to convert the char to native, then we can't be native so
        // finish up with a direct copy.
        char_t t_nchar;
        if (!MCUnicodeCharMapToNative(p_src[i], t_nchar))
        {
            MCMemoryCopy(p_dst + i, p_src + i, (p_count - i) * sizeof(unichar_t));
            return false;
        }

        // We still copy across unicode char.
        p_dst[i] = p_src[i];
    }
    
    // If we get here, then both src and dst can be native.
    return true;
}

static uindex_t __MCStringGetLength(MCStringRef self)
{
    MCAssert(!__MCStringIsIndirect(self));
    
    return self -> char_count;
}

static bool __MCStringIsEmpty(MCStringRef string)
{
    return string == nil || __MCStringGetLength(string) == 0;
}

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

bool MCStringCreateWithCStringAndRelease(char* p_cstring, MCStringRef& r_string)
{
	if (MCStringCreateWithNativeChars((const char_t *)p_cstring, p_cstring == nil ? 0 : strlen((const char*)p_cstring), r_string))
    {
        delete p_cstring;
        return true;
    }
    
    return false;
}

const char *MCStringGetCString(MCStringRef p_string)
{
    if (p_string == nil)
        return nil;
    
    MCStringNativize(p_string);
    
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
        // AL-2014-31-03: [[ Bug 12067 ]] Fix conversion from little endian bytes.    
        case kMCStringEncodingUTF16LE:
        case kMCStringEncodingUTF16BE:
        {
            unichar_t *t_buffer;
            uindex_t t_length = p_byte_count / 2;
            MCMemoryAllocate(t_length * sizeof(unichar_t), t_buffer);

            for (uindex_t i = 0; i < t_length; i++)
            {
                if (p_encoding == kMCStringEncodingUTF16BE)
                    t_buffer[i] = (unichar_t)MCSwapInt16BigToHost(((unichar_t *)p_bytes)[i]);
                else
                    t_buffer[i] = (unichar_t)MCSwapInt16LittleToHost(((unichar_t *)p_bytes)[i]);
            }
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
        case kMCStringEncodingUTF32LE:
        case kMCStringEncodingUTF32BE:
		{
			// Round the byte count to a multiple of UTF-32 units
			p_byte_count = ((p_byte_count + sizeof(uint32_t) - 1) & ~(sizeof(uint32_t) - 1));
			
            // Convert the string to UTF-16 first.
			MCAutoArray<unichar_t> t_buffer;
			if (!t_buffer.Extend(p_byte_count / sizeof(uint32_t)))
                return false;
            
			uindex_t t_in_offset;
			uindex_t t_out_offset = 0;
			for (t_in_offset = 0; t_in_offset < p_byte_count; t_in_offset += sizeof(uint32_t))
			{
				// BMP characters are output unchanged, non-BMP requires surrogate pairs
				codepoint_t t_codepoint;
				t_codepoint = *(uint32_t*)&p_bytes[t_in_offset];
                
                if (p_encoding == kMCStringEncodingUTF32BE)
                    t_codepoint = MCSwapInt32BigToHost(t_codepoint);
                else if (p_encoding == kMCStringEncodingUTF32LE)
                    t_codepoint = MCSwapInt32LittleToHost(t_codepoint);
                
				if (t_codepoint < 0x10000)
				{
					t_buffer[t_out_offset] = unichar_t(t_codepoint);
					t_out_offset += 1;
				}
				else
				{
                    // Split to surrogate pairs
                    // SN-2015-07-03: [[ Bug 15571 ]] Creating a surrogate pair
                    //  makes the UTF-16 string longer.
                    if (!t_buffer . Extend(t_buffer . Size() + 1))
                        return false;
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
#if !defined(__ISO_8859_1__)
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
    t_string = nil;
    
    switch (p_encoding)
    {
        case kMCStringEncodingASCII:
        case kMCStringEncodingNative:
            break;
        default:
            if (!MCStringCreateWithBytes(p_bytes, p_byte_count, p_encoding, p_is_external_rep, t_string))
                return false;
            
            r_string = t_string;
            free(p_bytes);
            return true;
    }
    
    bool t_success;
    t_success = true;
    
    if (p_byte_count == 0 && kMCEmptyString != nil)
    {
        r_string = MCValueRetain(kMCEmptyString);
        free(p_bytes);
        return true;
    }
    
    if (t_success)
        t_success = __MCValueCreate(kMCValueTypeCodeString, t_string);
    
    if (t_success)
        t_success = MCMemoryReallocate(p_bytes, p_byte_count + 1, p_bytes);
    
    if (t_success)
    {
        p_bytes[p_byte_count] = '\0';
        t_string -> native_chars = p_bytes;
        t_string -> char_count = p_byte_count;
        r_string = t_string;
    }
    else
        MCMemoryDelete(t_string);
    
    return t_success;
}

////////////////////////////////////////////////////////////////////////////////

bool MCStringCreateWithChars(const unichar_t *p_chars, uindex_t p_char_count, MCStringRef& r_string)
{
    if (p_char_count == 0 && kMCEmptyString != nil)
	{
		r_string = MCValueRetain(kMCEmptyString);
		return true;
	}
    
	bool t_success;
	t_success = true;

	__MCString *self;
	self = nil;
	if (t_success)
		t_success = __MCValueCreate(kMCValueTypeCodeString, self);

    bool t_not_native;
    t_not_native = false;
    
    if (t_success)
        t_success = MCMemoryNewArray(p_char_count + 1, self -> native_chars);
    
    if (t_success)
    {
        uindex_t i;
        for(i = 0; i < p_char_count; i++)
            if (!MCUnicodeCharMapToNative(p_chars[i], self -> native_chars[i]))
            {
                t_not_native = true;
                break;
            }
        
        if (t_not_native)
        {
            MCMemoryDeleteArray(self -> native_chars);
            t_success = MCMemoryNewArray(p_char_count + 1, self -> chars);
        }
    }

	if (t_success)
    {
        if (t_not_native)
        {
            MCStrCharsMapFromUnicode(p_chars, p_char_count, self -> chars, self -> char_count);
            self -> flags |= kMCStringFlagIsNotNative;
        }
        else
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

bool MCStringCreateWithWStringAndRelease(unichar_t* p_wstring, MCStringRef& r_string)
{
	if (MCStringCreateWithWString(p_wstring, r_string))
	{
		free(p_wstring);
		return true;
	}

	return false;
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
		t_success = MCMemoryNewArray(p_char_count + 1, self -> native_chars);

	if (t_success)
        MCMemoryCopy(self -> native_chars, p_chars, p_char_count);
	else
	{
		if (self != nil)
			MCMemoryDeleteArray(self -> native_chars);
		MCMemoryDelete(self);
	}

    if (t_success)
    {
		self -> char_count = p_char_count;
        r_string = self;
    }
    
	return t_success;
}

bool MCStringCreateWithNativeCharsAndRelease(char_t *p_chars, uindex_t p_char_count, MCStringRef& r_string)
{
    bool t_success;
    t_success = true;
    
    if (p_char_count == 0 && kMCEmptyString != nil)
    {
        r_string = MCValueRetain(kMCEmptyString);
        MCMemoryDeallocate(p_chars);
        return true;
    }
    
    __MCString *self;
    self = nil;
    if (t_success)
        t_success = __MCValueCreate(kMCValueTypeCodeString, self);
    
    if (t_success)
        t_success = MCMemoryReallocate(p_chars, p_char_count + 1, p_chars);
    
    if (t_success)
    {
        p_chars[p_char_count] = '\0';
        self -> native_chars = p_chars;
        self -> char_count = p_char_count;
        r_string = self;
    }
    else
        MCMemoryDelete(self);
    
    return t_success;
}

static bool MCStringCreateMutableUnicode(uindex_t p_initial_capacity, MCStringRef& r_string)
{
	bool t_success;
	t_success = true;
    
	__MCString *self;
	self = nil;
	if (t_success)
		t_success = __MCValueCreate(kMCValueTypeCodeString, self);
    
	if (t_success)
    {
        self -> flags |= kMCStringFlagIsNotNative;
		t_success = __MCStringExpandAt(self, 0, p_initial_capacity);
    }
    
	if (t_success)
	{
		self -> flags |= kMCStringFlagIsMutable;
		self->char_count = 0;
		r_string = self;
	}
    
	return t_success;
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
		self->char_count = 0;
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

// SN-2015-07-27: [[ Bug 15379 ]] This function is only used for internal
//  purposes, when a LiveCode function parameter can be data - in which case
//  no char translation must occur between the bytes in the DataRef and the
//  Unicode string that the engine function takes (in MCR_exec for instance)
bool MCStringCreateUnicodeStringFromData(MCDataRef p_data, bool p_is_external_rep, MCStringRef& r_string)
{
    MCAssert(!p_is_external_rep);
    
    if (MCDataIsEmpty(p_data))
    {
        r_string = MCValueRetain(kMCEmptyString);
        return true;
    }
    
    bool t_success;
    t_success = true;
    
    __MCString *self;
    self = nil;
    if (t_success)
        t_success = __MCValueCreate(kMCValueTypeCodeString, self);
    
    uint32_t t_byte_count;
    t_byte_count = MCDataGetLength(p_data);
    
    const byte_t* t_bytes = MCDataGetBytePtr(p_data);
    
    if (t_success)
        t_success = MCMemoryNewArray(t_byte_count + 1, self -> chars);
    
    if (t_success)
    {
        uindex_t i;
        for(i = 0; i < t_byte_count; i++)
            self -> chars[i] = t_bytes[i];
    }
    
    if (t_success)
    {
        self -> flags |= kMCStringFlagIsNotNative;
        self -> char_count = t_byte_count;
        
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
#define FORMAT_ARG_32_BIT 1
#define FORMAT_ARG_64_BIT 1
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
                // AL-2014-09-19: Flush chars between format strings
                if (t_format_ptr != t_format_start_ptr)
                    break;
                
				t_format_ptr++;
				
				if (*t_format_ptr == '@')
					break;
				
                // AL-2014-11-19: [[ Bug 14059 ]] Add support for variable length zero padding
                if (*t_format_ptr == '0')
                    t_format_ptr++;
                
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
				
				// MW-2014-10-23: [[ Bug 13757 ]] Make sure we process the VS specific 'I64d' format
				//   as 64-bit.
				if (strncmp(t_format_ptr, "lld", 3) == 0 ||
					strncmp(t_format_ptr, "llu", 3) == 0 ||
					strncmp(t_format_ptr, "lf", 2) == 0 ||
					strncmp(t_format_ptr, "f", 1) == 0 ||
					strncmp(t_format_ptr, "I64d", 4) == 0)
					t_arg_count += FORMAT_ARG_64_BIT;
                // SN-2015-01-05: [[ Bug 14304 ]] There is no argument to be popped from the list
                //   if we are considering a "%%" sequence
				else if (*t_format_ptr != '%')
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
#if defined(_WINDOWS) || defined(_WINDOWS_SERVER)
				t_success = MCNativeCharsFormatV(t_string, t_size, t_format, p_args);
#else
                va_list t_args;
                va_copy(t_args, p_args);
				t_success = MCNativeCharsFormatV(t_string, t_size, t_format, t_args);
                va_end(t_args);
#endif
			}
			
			if (t_success)
				t_success = MCStringAppendNativeChars(t_buffer, t_string, t_size);

			if (t_success)
				while(t_arg_count > 0)
				{
					va_arg(p_args, uintptr_t);
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
    
    if (!t_success)
		MCValueRelease (t_buffer);
	
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

static bool __MCStringCloneBuffer(MCStringRef self, unichar_t*& chars, uindex_t& char_count)
{
    MCAssert(!__MCStringIsIndirect(self));
    
	if (MCMemoryNewArray(self -> char_count + 1, chars))
	{
		MCStrCharsMapFromUnicode(self -> chars, self -> char_count, chars, char_count);
        return true;
    }
    
    return false;
}

static bool __MCStringCloneNativeBuffer(MCStringRef self, char_t*& chars, uindex_t& char_count)
{
    MCAssert(!__MCStringIsIndirect(self));
    
	if (MCMemoryNewArray(self -> char_count + 1, chars))
	{
		MCMemoryCopy(chars, self -> native_chars, self -> char_count);
        char_count = self -> char_count;
        return true;
    }
    
    return false;
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
    
    // If it is mutable and indirect then retain the referenced string
    if (__MCStringIsIndirect(self))
    {
        r_new_string = MCValueRetain(self -> string);
        return true;
    }

    // Otherwise make the mutable direct string immutable,
    // assign the new string as that, and make self indirect
    // referencing it.
    return __MCStringCopyMutable(self, r_new_string);
}

bool MCStringCopyAndRelease(MCStringRef self, MCStringRef& r_new_string)
{
	// If the string is immutable we just pass it through (as we are releasing the string).
	if (!MCStringIsMutable(self))
	{
		r_new_string = self;
		return true;
	}

    // If the string is indirect then retain its reference, and release
    if (__MCStringIsIndirect(self))
    {
        r_new_string = MCValueRetain(self -> string);
        MCValueRelease(self);
        return true;
    }
    
	// If the reference count is one, then shrink the buffer and mark as immutable.
	if (self -> references == 1)
	{
        __MCStringMakeImmutable(self);
        self -> flags &= ~kMCStringFlagIsMutable;
        self -> capacity = 0;
		r_new_string = self;
		return true;
	}

	// Otherwise, make a new indirect string
    if (!__MCStringMakeIndirect(self))
        return false;
    
    // Reduce reference count
    self -> references -= 1;
    
    // And return a copy of the string
    r_new_string = MCValueRetain(self -> string);
    return true;
}

bool MCStringMutableCopy(MCStringRef self, MCStringRef& r_new_string)
{
	// If self is immutable, then the new mutable string will be indirect
	// referencing it.
    if (!MCStringIsMutable(self))
        return __MCStringCreateIndirect(self, r_new_string);

    // If the string is already indirect, we just create a new reference to its string
	if (__MCStringIsIndirect(self))
		return __MCStringCreateIndirect(self -> string, r_new_string);
    
    // If the string is mutable, we make it indirect and share
	// the indirect copy.
    if (!__MCStringMakeIndirect(self))
        return false;
    
    return __MCStringCreateIndirect(self -> string, r_new_string);
}

bool MCStringMutableCopyAndRelease(MCStringRef self, MCStringRef& r_new_string)
{
	if (self -> references == 1)
	{
		if (!MCStringIsMutable(self))
        {
			self -> flags |= kMCStringFlagIsMutable;
            //self -> capacity = self -> char_count;
        }
        
		r_new_string = self;
		return true;
	}
    
	if (!MCStringMutableCopy(self, r_new_string))
		return false;
    
	self -> references -= 1;
	return true;
}

////////////////////////////////////////////////////////////////////////////////

bool MCStringCopySubstring(MCStringRef self, MCRange p_range, MCStringRef& r_substring)
{
    if (__MCStringIsIndirect(self))
        self = self -> string;
    
    // Avoid copying in case the substring is actually the whole string
    if (p_range . offset == 0 && self -> char_count < p_range . length)
        return MCStringCopy(self, r_substring);

	__MCStringClampRange(self, p_range);
	
    if (__MCStringIsNative(self))
        return MCStringCreateWithNativeChars(self -> native_chars + p_range . offset, p_range . length, r_substring);
    
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
    if (__MCStringIsIndirect(self))
        self = self -> string;
    
    __MCStringClampRange(self, p_range);
    
	// Simply create a mutable string with enough initial capacity and then copy
	// in the other strings chars.
	// Notice the slight amount of anal-retentiveness here in the use of a
	// temporary - any 'r_' (out) parameter should never be updated until the end
	// of the method and then only if the method is succeeding.
	MCStringRef t_new_string;
    
    if (__MCStringIsNative(self))
    {
        if (!MCStringCreateMutable(p_range . length + 1, t_new_string))
            return false;
        
        // Now copy across the chars (note we set the implicit NUL too, just to be
        // on the safe-side!).
        MCMemoryCopy(t_new_string -> native_chars, self -> native_chars + p_range . offset, p_range . length);
        t_new_string -> native_chars[p_range . length] = '\0';
    }
    else
    {
        if (!MCStringCreateMutableUnicode(p_range . length + 1, t_new_string))
            return false;
        
        MCMemoryCopy(t_new_string -> chars, self -> chars + p_range . offset, p_range . length * sizeof(strchar_t));
        t_new_string -> chars[p_range . length] = '\0';
    }

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
    if (__MCStringIsIndirect(self))
        self = self -> string;
    
    return __MCStringGetLength(self);
}

const unichar_t *MCStringGetCharPtr(MCStringRef self)
{
    if (__MCStringIsIndirect(self))
        __MCStringResolveIndirect(self);
    
    __MCStringUnnativize(self);
	return self -> chars;
}

const char_t *MCStringGetNativeCharPtr(MCStringRef self)
{
    if (MCStringIsNative(self))
    {
        // AL-2014-07-25: [[ Bug 12672 ]] Ensure possibly indirect string is resolved before returning char ptr
        if (__MCStringIsIndirect(self))
            __MCStringResolveIndirect(self);
        
        return self -> native_chars;
    }
    
    return nil;
}

const char_t *MCStringGetNativeCharPtrAndLength(MCStringRef self, uindex_t& r_char_count)
{
    r_char_count = __MCStringNativize(self);
	return self -> native_chars;
}

unichar_t MCStringGetCharAtIndex(MCStringRef self, uindex_t p_index)
{
    if (__MCStringIsIndirect(self))
        self = self -> string;
    
    if (__MCStringIsNative(self))
        return MCUnicodeCharMapFromNative(self -> native_chars[p_index]);
    
	return self -> chars[p_index];
}

char_t MCStringGetNativeCharAtIndex(MCStringRef self, uindex_t p_index)
{
    if (__MCStringIsIndirect(self))
        self = self -> string;
    
    if (__MCStringIsNative(self))
        return self -> native_chars[p_index];
    
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
 
    if (__MCStringIsIndirect(self))
        self = self -> string;
    
    if (__MCStringIsNative(self))
    {
        char_t native_char = self -> native_chars[p_index];
        return MCUnicodeCharMapFromNative(native_char);
    }
    
    // Get the codepoint at this index
    unichar_t t_lead, t_trail;
    t_lead = self -> chars[t_codeunit_idx.offset];
    if (t_codeunit_idx.length == 1)
        return t_lead;
    
    // We have a surrogate pair
    t_trail = self -> chars[t_codeunit_idx.offset + 1];
    return MCStringSurrogatesToCodepoint(t_lead, t_trail);
}

uindex_t MCStringGetChars(MCStringRef self, MCRange p_range, unichar_t *p_chars)
{
    if (__MCStringIsIndirect(self))
        self = self -> string;
    
	uindex_t t_count;
	t_count = 0;

	for(uindex_t i = p_range . offset; i < p_range . offset + p_range . length; i++)
	{
		if (i >= self -> char_count)
			break;
        if (__MCStringIsNative(self))
            p_chars[i - p_range . offset] = MCUnicodeCharMapFromNative(self -> native_chars[i]);
        else
            p_chars[i - p_range . offset] = MCStrCharMapToUnicode(self -> chars[i]);

		t_count += 1;
	}

	return t_count;
}

uindex_t MCStringGetNativeChars(MCStringRef self, MCRange p_range, char_t *p_chars)
{
    if (__MCStringIsIndirect(self))
        self = self -> string;
    
	uindex_t t_count;
	t_count = 0;

	for(uindex_t i = p_range . offset; i < p_range . offset + p_range . length; i++)
	{
		if (i >= self -> char_count)
			break;
        
        if (__MCStringIsNative(self))
            p_chars[i - p_range . offset] = self -> native_chars[i];
        else
            p_chars[i - p_range . offset] = MCStrCharMapToNative(self -> chars[i]);

		t_count += 1;
	}

	return t_count;
}

void MCStringNativize(MCStringRef self)
{
    __MCStringNativize(self);
}

bool MCStringNativeCopy(MCStringRef p_string, MCStringRef& r_copy)
{
    // AL-2014-12-12: [[ Bug 14208 ]] Implement a native copy function to aid conversion to data
    if (MCStringIsNative(p_string))
        return MCStringCopy(p_string, r_copy);
    
    MCStringRef t_string;
    t_string = nil;
    
    if (!MCStringMutableCopy(p_string, t_string))
        return false;
    
    __MCStringNativize(t_string);
    
    __MCStringMakeImmutable(t_string);
    t_string -> flags &= ~kMCStringFlagIsMutable;
    
    r_copy = t_string;
    return true;
}

bool MCStringIsNative(MCStringRef self)
{
    if (__MCStringIsIndirect(self))
        self = self -> string;
    
    return __MCStringIsNative(self);
}

bool MCStringCantBeEqualToNative(MCStringRef self, MCStringOptions p_options)
{
    if (__MCStringIsIndirect(self))
        self = self -> string;
    
    return __MCStringCantBeEqualToNative(self, p_options);
}

bool MCStringCanBeNative(MCStringRef self)
{
    if (__MCStringIsIndirect(self))
        self = self -> string;
    
    return __MCStringCanBeNative(self);
}

// AL-2015-02-06: [[ Bug 14504 ]] Ensure 'simple' flag is checked against the direct string.
bool MCStringIsSimple(MCStringRef self)
{
    if (__MCStringIsIndirect(self))
        self = self -> string;
    
    return __MCStringIsSimple(self);
}

// AL-2015-02-06: [[ Bug 14504 ]] Ensure 'uncombined' flag is checked against the direct string.
bool MCStringIsUncombined(MCStringRef self)
{
    if (__MCStringIsIndirect(self))
        self = self -> string;
    
    return __MCStringIsUncombined(self);
}

bool MCStringMapCodepointIndices(MCStringRef self, MCRange p_in_range, MCRange &r_out_range)
{
    if (__MCStringIsIndirect(self))
        self = self -> string;
    
    MCAssert(self != nil);
    
    // Shortcut for strings containing only BMP characters
    if (__MCStringIsNative(self) || (__MCStringIsSimple(self) && __MCStringIsUncombined(self)))
    {
        __MCStringClampRange(self, p_in_range);
        r_out_range = p_in_range;
        return true;
    }
    
    uindex_t char_count = __MCStringGetLength(self);
    
    // If the string has not yet been scanned for simplicity, scan the whole
    // thing (assuming multiple mapping requests will be made)
    uindex_t t_scan_end;
    if (__MCStringIsChecked(self))
        t_scan_end = p_in_range.offset + p_in_range.length;
    else
    {
        t_scan_end = char_count;
        // The whole string is going to be checked for simplicity
        self -> flags |= kMCStringFlagIsChecked;
    }
    
    // Scan through the string, counting the number of codepoints
    bool t_is_simple = true;
    bool t_is_uncombined = true;
    uindex_t t_cp_counter = 0;
    uindex_t t_codeunit_pos = 0;
    MCRange t_units = MCRangeMake(0, 0);
    // If we are scanning the whole string, the end comes when all the codeunits have been processed
    // Otherwise, we have an amount of codepoints to read, not codeunits
    while (t_codeunit_pos < t_scan_end)
    {
        // Is this a single code unit or a valid surrogate pair?
        uindex_t t_length;
        if (MCStringIsValidSurrogatePair(self, t_codeunit_pos))
            t_length = 2, t_is_simple = false;
        else
            t_length = 1;
        
        if (MCUnicodeGetIntegerProperty(MCStringGetCharAtIndex(self, t_codeunit_pos), kMCUnicodePropertyCanonicalCombiningClass))
            t_is_uncombined = false;
            
        // Update the appropriate field of the output
        if (t_codeunit_pos < p_in_range.offset)
            t_units.offset += t_length;
        else if (t_cp_counter < p_in_range.offset + p_in_range.length)
            t_units.length += t_length;
        
        // Make sure we haven't exceeded the length of the string
        if (t_units.offset > char_count)
        {
            t_units = MCRangeMake(char_count, 0);
            break;
        }
        if ((t_units.offset + t_units.length) > char_count)
        {
            t_units.length = char_count - t_units.offset;
            break;
        }
        
        ++t_cp_counter;
        t_codeunit_pos += t_length;
    }
    
    // If no surrogates were found, mark the string as simple
    if (t_is_simple && t_scan_end == char_count)
        self -> flags |= kMCStringFlagIsSimple;
    
    if (t_is_uncombined)
        self -> flags |= kMCStringFlagIsUncombined;
    else
        self -> flags &= ~kMCStringFlagIsUncombined;
    
    // All done
    r_out_range = t_units;
    return true;
}

bool MCStringUnmapCodepointIndices(MCStringRef self, MCRange p_in_range, MCRange &r_out_range)
{    
    MCAssert(self != nil);
    
    // AL-2015-02-06: [[ Bug 14504 ]] Use direct string for checks here, as the flags are not set on the indirect string.
    if (__MCStringIsIndirect(self))
        self = self -> string;
    
    // Shortcut for strings containing only BMP characters
    if (__MCStringIsNative(self) || (__MCStringIsSimple(self) && __MCStringIsUncombined(self)))
    {
        __MCStringClampRange(self, p_in_range);
        r_out_range = p_in_range;
        return true;
    }
    
    uindex_t char_count = __MCStringGetLength(self);
    
    // Check that the input indices are valid
    if (p_in_range.offset + p_in_range.length > char_count)
        return false;
    
    // Scan through the string, counting the number of code points
    bool t_is_simple = true;
    bool t_is_uncombined = true;
    
    uindex_t t_counter = 0;
    MCRange t_codepoints = MCRangeMake(0, 0);
    while (t_counter < p_in_range.offset + p_in_range.length)
    {
        // Is this a single code unit or a valid surrogate pair?
        uindex_t t_length;
        if (MCStringIsValidSurrogatePair(self, t_counter))
            t_length = 2, t_is_simple = false;
        else
            t_length = 1;
        
        if (MCUnicodeGetIntegerProperty(MCStringGetCharAtIndex(self, t_counter), kMCUnicodePropertyCanonicalCombiningClass))
            t_is_uncombined = false;
        
        // Increment the counters
        if (t_counter < p_in_range.offset)
            t_codepoints.offset++;
        else
            t_codepoints.length++;
        t_counter += t_length;
    }
    
    // If no surrogates were found, mark the string as simple
    if (t_is_simple && p_in_range.offset + p_in_range.length >= char_count)
        self -> flags |= kMCStringFlagIsSimple;
            
    if (t_is_uncombined)
        self -> flags |= kMCStringFlagIsUncombined;
    else
        self -> flags &= ~kMCStringFlagIsUncombined;
    
    // The string has been checked
    self -> flags |= kMCStringFlagIsChecked;
    // All done
    r_out_range = t_codepoints;
    return true;
}

bool MCStringMapIndices(MCStringRef self, MCBreakIteratorType p_type, MCLocaleRef p_locale, MCRange p_in_range, MCRange &r_out_range)
{
    MCAssert(self != nil);
    MCAssert(p_locale != nil);
    
    // Create the appropriate break iterator
    MCBreakIteratorRef t_iter;
    if (!MCLocaleBreakIteratorCreate(p_locale, p_type, t_iter))
        return false;
    
    // Set the iterator's text
    if (!MCLocaleBreakIteratorSetText(t_iter, self))
    {
        MCLocaleBreakIteratorRelease(t_iter);
        return false;
    }
    
    // Advance to the beginning of the specified range
    uindex_t t_start;
    t_start = MCLocaleBreakIteratorNext(t_iter, p_in_range.offset);
    
    if (t_start == kMCLocaleBreakIteratorDone)
    {
        r_out_range = MCRangeMake(MCStringGetLength(self), 0);
        return true;
    }
    
    // Advance to the end of the specified range
    uindex_t t_end;
    t_end = MCLocaleBreakIteratorNext(t_iter, p_in_range.length);
    if (t_end == kMCLocaleBreakIteratorDone)
        t_end = MCStringGetLength(self);
    
    MCRange t_units;
    t_units = MCRangeMake(t_start, t_end - t_start);
    
    // All done
    MCLocaleBreakIteratorRelease(t_iter);
    r_out_range = t_units;
    return true;
}

bool MCStringMapGraphemeIndices(MCStringRef self, MCLocaleRef p_locale, MCRange p_in_range, MCRange &r_out_range)
{
    if (__MCStringIsIndirect(self))
        self = self -> string;
    
    // SN-2014-04-11 [[ FasterStrings ]] Process a checking of the string - in case we can ensure it is
    // combining chars/surrogate pairs-free
    if (!__MCStringIsChecked(self))
    {        
        MCRange t_input, t_out;
        t_input . offset = 0;
        t_input . length = self -> char_count;
        MCStringMapCodepointIndices(self, t_input, t_out);
    }
    
    // Quick-n-dirty workaround
    if (__MCStringIsNative(self) || (__MCStringIsUncombined(self) && __MCStringIsSimple(self)))
    {
        __MCStringClampRange(self, p_in_range);
        r_out_range = p_in_range;
        return true;
    }

    return MCStringMapIndices(self, kMCBreakIteratorTypeCharacter, p_locale, p_in_range, r_out_range);
}

bool MCStringCodepointIsWordPart(codepoint_t p_codepoint)
{
    return MCUnicodeIsAlphabetic(p_codepoint) || MCUnicodeIsDigit(p_codepoint);
}

bool MCStringMapTrueWordIndices(MCStringRef self, MCLocaleRef p_locale, MCRange p_in_range, MCRange &r_out_range)
{    
    MCAssert(self != nil);
    MCAssert(p_locale != nil);
    
    // Create the appropriate break iterator
    MCBreakIteratorRef t_iter;
    if (!MCLocaleBreakIteratorCreate(p_locale, kMCBreakIteratorTypeWord, t_iter))
        return false;
    
    // Set the iterator's text
    if (!MCLocaleBreakIteratorSetText(t_iter, self))
    {
        MCLocaleBreakIteratorRelease(t_iter);
        return false;
    }

    p_in_range . offset++;
    
    MCRange t_word_range = MCRangeMake(0, 0);
    bool t_found;
    // Advance to the beginning of the specified range
    while (p_in_range . offset-- && (t_found = MCLocaleWordBreakIteratorAdvance(self, t_iter, t_word_range)))
        ;

    if (!t_found)
    {
        r_out_range = MCRangeMake(MCStringGetLength(self), 0);
        return true;
    }
    
    // Advance to the end of the current word
    uindex_t t_start = t_word_range . offset;
    p_in_range . length--;
    
    // While more words are requested, find the end of the next word.
    while (p_in_range . length-- && MCLocaleWordBreakIteratorAdvance(self, t_iter, t_word_range))
        ;
    
    MCRange t_units;
    t_units = MCRangeMake(t_start, t_word_range . offset + t_word_range . length - t_start);
    
    // All done
    MCLocaleBreakIteratorRelease(t_iter);
    r_out_range = t_units;
    return true;
}

bool MCStringMapSentenceIndices(MCStringRef self, MCLocaleRef p_locale, MCRange p_in_range, MCRange &r_out_range)
{
    return MCStringMapIndices(self, kMCBreakIteratorTypeSentence, p_locale, p_in_range, r_out_range);
}

bool MCStringUnmapIndices(MCStringRef self, MCBreakIteratorType p_type, MCLocaleRef p_locale, MCRange p_in_range, MCRange &r_out_range)
{
    MCAssert(self != nil);
    MCAssert(p_locale != nil);
    
    // Check that the input range is valid
    if (p_in_range.offset + p_in_range.length > MCStringGetLength(self))
        return false;
    
    // Create a break iterator of the appropriate type
    MCBreakIteratorRef t_iter;
    if (!MCLocaleBreakIteratorCreate(p_locale, p_type, t_iter))
        return false;
    
    // Set the iterator's text
    if (!MCLocaleBreakIteratorSetText(t_iter, self))
    {
        MCLocaleBreakIteratorRelease(t_iter);
        return false;
    }
    
    // Count how many of the given unit it takes to reach or exceed the offset
    uindex_t t_start, t_offset;
    t_start = 0;
    t_offset = 0;
    while (t_offset < p_in_range.offset)
    {
        if (MCLocaleBreakIteratorIsBoundary(t_iter, t_offset++))
            t_start++;
        
        if (t_offset >= MCStringGetLength(self))
        {
            r_out_range = MCRangeMake(t_offset, 0);
            return true;
        }
    }
    
    // Count how many more it takes to accomodate all the code units
    uindex_t t_end;
    t_end = 0;
    while (t_offset < p_in_range.offset + p_in_range.length)
    {
        if (MCLocaleBreakIteratorIsBoundary(t_iter, t_offset++))
            t_end++;
        
        if (t_offset >= MCStringGetLength(self))
            break;
    }
    
    MCRange t_units;
    t_units = MCRangeMake(t_start, t_end);
    
    // All done
    MCLocaleBreakIteratorRelease(t_iter);
    r_out_range = t_units;
    return true;
}

bool MCStringUnmapGraphemeIndices(MCStringRef self, MCLocaleRef p_locale, MCRange p_in_range, MCRange &r_out_range)
{    
    if (__MCStringIsIndirect(self))
        self = self -> string;
    
    // SN-2014-04-11 [[ FasterStrings ]] Process a checking of the string - in case we can ensure it is
    // combining chars/surrogate pairs-free
    if (!__MCStringIsChecked(self))
    {
        MCRange t_input, t_out;
        t_input . offset = 0;
        t_input . length = self -> char_count;
        MCStringUnmapCodepointIndices(self, t_input, t_out);
    }
    
    // Quick-n-dirty workaround
    if (__MCStringIsNative(self) || (__MCStringIsUncombined(self) && __MCStringIsSimple(self)))
    {
        __MCStringClampRange(self, p_in_range);
        r_out_range = p_in_range;
        return true;
    }
    
    return MCStringUnmapIndices(self, kMCBreakIteratorTypeCharacter, p_locale, p_in_range, r_out_range);
}

bool MCStringUnmapTrueWordIndices(MCStringRef self, MCLocaleRef p_locale, MCRange p_in_range, MCRange &r_out_range)
{
    MCAssert(self != nil);
    MCAssert(p_locale != nil);
    
    // Check that the input range is valid
    if (p_in_range.offset + p_in_range.length > self -> char_count)
        return false;
    
    // Create a break iterator of the appropriate type
    MCBreakIteratorRef t_iter;
    if (!MCLocaleBreakIteratorCreate(p_locale, kMCBreakIteratorTypeWord, t_iter))
        return false;
    
    // Set the iterator's text
    if (!MCLocaleBreakIteratorSetText(t_iter, self))
    {
        MCLocaleBreakIteratorRelease(t_iter);
        return false;
    }
    
    // Count how many words it takes to reach or exceed the offset
    uindex_t t_start, t_left_break, t_right_break;
    t_start = 0;
    t_right_break = 0;
    t_left_break = 0;
    while (t_right_break < p_in_range.offset)
    {
        t_right_break++;
        if (MCLocaleBreakIteratorIsBoundary(t_iter, t_right_break))
        {
            // if the intervening chars contain a letter or number then it was a valid 'word'
            while (t_left_break < t_right_break)
            {
                if (MCStringCodepointIsWordPart(MCStringGetCodepointAtIndex(self, t_left_break)))
                    break;
                if (MCStringIsValidSurrogatePair(self, t_left_break++))
                    t_left_break++;
            }
            
            if (t_left_break < t_right_break)
                t_start++;
            t_left_break = t_right_break;
        }

        if (t_right_break >= MCStringGetLength(self))
        {
            r_out_range = MCRangeMake(t_right_break, 0);
            return true;
        }
    }
    
    // Count how many more it takes to accomodate all the code units
    uindex_t t_end;
    t_end = 0;
    while (t_right_break < p_in_range.offset + p_in_range.length)
    {
        t_right_break++;
        if (MCLocaleBreakIteratorIsBoundary(t_iter, t_right_break))
        {
            // if the intervening chars contain a letter or number then it was a valid 'word'
            while (t_left_break < t_right_break)
            {
                if (MCStringCodepointIsWordPart(MCStringGetCodepointAtIndex(self, t_left_break)))
                    break;
                if (MCStringIsValidSurrogatePair(self, t_left_break++))
                    t_left_break++;
            }
            
            if (t_left_break < t_right_break)
                t_end++;
            t_left_break = t_right_break;
        }
        
        if (t_right_break >= MCStringGetLength(self))
            break;
    }
    
    MCRange t_units;
    t_units = MCRangeMake(t_start, t_end);
    
    // All done
    MCLocaleBreakIteratorRelease(t_iter);
    r_out_range = t_units;
    return true;
}

bool MCStringUnmapSentenceIndices(MCStringRef self, MCLocaleRef p_locale, MCRange p_in_range, MCRange &r_out_range)
{
    return MCStringUnmapIndices(self, kMCBreakIteratorTypeSentence, p_locale, p_in_range, r_out_range);
}

extern MCLocaleRef kMCBasicLocale;
bool MCStringMapIndices(MCStringRef self, MCCharChunkType p_type, MCRange p_char_range, MCRange &r_cu_range)
{
    switch (p_type)
    {
        case kMCCharChunkTypeCodeunit:
            r_cu_range = p_char_range;
            return true;
            
        case kMCCharChunkTypeCodepoint:
            return MCStringMapCodepointIndices(self, p_char_range, r_cu_range);
            
        case kMCCharChunkTypeGrapheme:
            return MCStringMapGraphemeIndices(self, kMCBasicLocale, p_char_range, r_cu_range);
    }
    
    MCAssert(false);
    return false;
}

bool MCStringUnmapIndices(MCStringRef self, MCCharChunkType p_type, MCRange p_cu_range, MCRange &r_char_range)
{
    switch (p_type)
    {
        case kMCCharChunkTypeCodeunit:
            r_char_range = p_cu_range;
            return true;
            
        case kMCCharChunkTypeCodepoint:
            return MCStringUnmapCodepointIndices(self, p_cu_range, r_char_range);
            
        case kMCCharChunkTypeGrapheme:
            return MCStringUnmapGraphemeIndices(self, kMCBasicLocale, p_cu_range, r_char_range);
    }
    
    MCAssert(false);
    return false;
}

////////////////////////////////////////////////////////////////////////////////

bool MCStringConvertToBytes(MCStringRef self, MCStringEncoding p_encoding, bool p_is_external_rep, byte_t*& r_bytes, uindex_t& r_byte_count)
{
    MCAssert(!p_is_external_rep);
    
    switch(p_encoding)
    {
    // [[ Bug 12204 ]] textEncode ASCII support is actually native
    case kMCStringEncodingASCII:
        return MCStringConvertToAscii(self, (char_t*&)r_bytes, r_byte_count);
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
    // AL-2014-31-03: [[ Bug 12067 ]] Implement conversion to big endian bytes.
    case kMCStringEncodingUTF16LE:
    case kMCStringEncodingUTF16BE:
        {
            uindex_t t_char_count;
            unichar_t *t_bytes;
            if (MCStringConvertToUnicode(self, t_bytes, t_char_count))
            {
                unichar_t *t_buffer;
                MCMemoryAllocate((t_char_count + 1) * sizeof(unichar_t), t_buffer);
                
                for (uindex_t i = 0; i < t_char_count; i++)
                {
                    if (p_encoding == kMCStringEncodingUTF16BE)
                        t_buffer[i] = (unichar_t)MCSwapInt16HostToBig((t_bytes)[i]);   
                    else
                        t_buffer[i] = (unichar_t)MCSwapInt16HostToLittle((t_bytes)[i]);
                }
                r_bytes = (byte_t*&)t_buffer;
                r_byte_count = t_char_count * sizeof(unichar_t);
                return true;
            }
            return false;
        }
    case kMCStringEncodingUTF8:
        return MCStringConvertToUTF8(self, (char*&)r_bytes, r_byte_count);
    case kMCStringEncodingUTF32:
        {
            uindex_t t_char_count;
            uint32_t *t_codepoints;
            if (MCStringConvertToUTF32(self, t_codepoints, t_char_count))
            {
                r_bytes = (byte_t*) t_codepoints;
                r_byte_count = t_char_count * sizeof(uint32_t);
                return true;
            }
        }
        break;            
    case kMCStringEncodingUTF32BE:
    case kMCStringEncodingUTF32LE:
        {
            uindex_t t_char_count;
            uint32_t* t_codepoints;
            
            if (MCStringConvertToUTF32(self, t_codepoints, t_char_count))
            {
                for (uinteger_t i = 0 ; i < t_char_count ; ++i)
                {
                    if (p_encoding == kMCStringEncodingUTF32BE)
                        t_codepoints[i] = MCSwapInt32HostToBig(t_codepoints[i]);
                    else
                        t_codepoints[i] = MCSwapInt32HostToLittle(t_codepoints[i]);
                }
                
                r_bytes = (byte_t*)t_codepoints;
                r_byte_count = t_char_count * sizeof(uint32_t);
                return true;
            }
        }
        break;
#if !defined(__ISO_8859_1__)
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

bool MCStringConvertToAscii(MCStringRef self, char_t *&r_chars, uindex_t& r_char_count)
{
    // Get the native chars, but excludes any char belonging to the extended part of the ASCII -
    char_t *t_chars;
    uindex_t t_char_count = MCStringGetLength(self);
    if (!MCMemoryNewArray(t_char_count + 1, t_chars))
        return false;
    
    t_char_count = MCStringGetNativeChars(self, MCRangeMake(0, t_char_count), t_chars);
    
    for (uindex_t i = 0; i < t_char_count; ++i)
    {
        if (t_chars[i] > 127)
            t_chars[i] = '?';
    }
    
    r_chars = t_chars;
    r_char_count = t_char_count;
    return true;
}

bool MCStringConvertToUnicode(MCStringRef self, unichar_t*& r_chars, uindex_t& r_char_count)
{
	// Allocate an array of chars one bigger than needed. As the allocated array
	// is filled with zeros, this will naturally NUL terminate the string.
	unichar_t *t_chars;
	if (!MCMemoryNewArray(MCStringGetLength(self) + 1, t_chars))
		return false;

	r_char_count = MCStringGetChars(self, MCRangeMake(0, MCStringGetLength(self)), t_chars);
	r_chars = t_chars;
	return true;
}

bool MCStringNormalizeAndConvertToNative(MCStringRef string, char_t*& r_chars, uindex_t& r_char_count)
{
    MCAutoStringRef t_normalized;
    if (!MCStringNormalizedCopyNFC(string, &t_normalized))
        return false;
    
    return MCStringConvertToNative(*t_normalized, r_chars, r_char_count);
}

bool MCStringConvertToNative(MCStringRef self, char_t*& r_chars, uindex_t& r_char_count)
{
	// Allocate an array of chars one byte bigger than needed. As the allocated array
	// is filled with zeros, this will naturally NUL terminate the string.
	char_t *t_chars;
	if (!MCMemoryNewArray(MCStringGetLength(self) + 1, t_chars))
		return false;

	r_char_count = MCStringGetNativeChars(self, MCRangeMake(0, MCStringGetLength(self)), t_chars);
	r_chars = t_chars;
	return true;
}

bool MCStringNormalizeAndConvertToCString(MCStringRef string, char*& r_cstring)
{
    MCAutoStringRef t_normalized;
    if (!MCStringNormalizedCopyNFC(string, &t_normalized))
        return false;
    
    return MCStringConvertToCString(*t_normalized, r_cstring);
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

bool MCStringConvertToUTF32(MCStringRef self, uint32_t *&r_codepoints, uinteger_t &r_char_count)
{
    if (MCStringIsNative(self))
    {
        // Shortcut for native string - no surrogate pair checking
        uindex_t t_char_count;
        const char_t* t_chars;
        uint32_t *t_codepoints;
        
        t_chars = MCStringGetNativeCharPtrAndLength(self, t_char_count);
        
        // SN-2015-07-03: [[ Bug 15571 ]] Allocate the right size
        if (!MCMemoryAllocate((t_char_count + 1) * sizeof(uint32_t), t_codepoints))
            return false;
        
        for (uindex_t i = 0 ; i < t_char_count; ++i)
            t_codepoints[i] = (uint32_t)t_chars[i];
        
        r_codepoints = t_codepoints;
        r_char_count = t_char_count;
        return true;
    }
    else
    {
        uindex_t t_char_count;
        uindex_t t_codepoint_count;
        const unichar_t *t_unichars;
        MCAutoArray<uint32_t> t_codepoints;
        bool t_invalid_char;
        
        t_unichars = MCStringGetCharPtr(self);
        t_char_count = MCStringGetLength(self);
        t_codepoint_count = 0;
        t_invalid_char = false; 
        
        if (!t_codepoints . New(t_char_count + 1))
            return false;
        
        // Loop up to the penultimate char, to avoid the checking of the index each
        // time a trail surrogate must be found - a pair may be broken if imported for instance
        uindex_t i = 0;
        for (; i < t_char_count - 1 && !t_invalid_char; ++i)
        {
            if (t_unichars[i] > 0xD7FF && t_unichars[i] < 0xDC00)
            {
                // Surrogate lead found
                if (t_unichars[i+1] > 0xDBFF && t_unichars[i+1] < 0xE000)
                {
                    // SN-2015-07-03: [[ Bug 15571 ]] Codepoint and UTF-16 index will not
                    //  remain the same if any surrogate pair appears.
                    // Surogate trail found: valid surrogate pair
                    t_codepoints[t_codepoint_count] = MCUnicodeSurrogatesToCodepoint(t_unichars[i], t_unichars[i+1]);
                    ++i;
                }
                else
                    t_invalid_char = true;
            }
            else
            {
                // SN-2015-07-03: [[ Bug 15571 ]] Codepoint and UTF-16 index will not
                //  remain the same if any surrogate pair appears.
                t_codepoints[t_codepoint_count] = (uint32_t)t_unichars[i];
            }
            
            ++t_codepoint_count;
        }
        
        if (t_invalid_char)
            return false;
        
        // Add the last codeunit
        if (i < t_char_count)
        {
            // SN-2015-07-03: [[ Bug 15571 ]] Codepoint and UTF-16 index will not
            //  remain the same if any surrogate pair appears.
            t_codepoints[t_codepoint_count] = (uint32_t)t_unichars[i];
            ++t_codepoint_count;
        }
        
        t_codepoints . Shrink(t_codepoint_count + 1);
        t_codepoints . Take(r_codepoints, r_char_count);
        // SN-2015-07-03: [[ Bug 15571 ]] The NULL codepoint is not amongst the
        //  char count.
        r_char_count = t_codepoint_count;
        return true;
    }
    
    return false;
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

hash_t MCStringHash(MCStringRef self, MCStringOptions p_options)
{
    if (__MCStringIsIndirect(self))
        self = self -> string;
    
    if (__MCStringIsNative(self))
        return MCNativeCharsHash(self -> native_chars, self -> char_count, p_options);
    
	return MCUnicodeHash(self -> chars, self -> char_count, (MCUnicodeCompareOption)p_options);
}

bool MCStringIsEqualTo(MCStringRef self, MCStringRef p_other, MCStringOptions p_options)
{
    if (__MCStringIsIndirect(self))
        self = self -> string;
    
    if (__MCStringIsIndirect(p_other))
        p_other = p_other -> string;
    
	if (self == p_other)
        return true;
    
    if (__MCStringIsEmpty(self) != __MCStringIsEmpty(p_other))
        return false;

    bool self_native, other_native;
    self_native = __MCStringIsNative(self);
    other_native = __MCStringIsNative(p_other);
    
    if ((self_native && __MCStringCantBeEqualToNative(p_other, p_options)) || (other_native && __MCStringCantBeEqualToNative(self, p_options)))
        return false;
    
    if (self_native && other_native)
    {
        if (__MCStringGetLength(self) != __MCStringGetLength(p_other))
            return false;
        
        if (p_options == kMCStringOptionCompareExact || p_options == kMCStringOptionCompareNonliteral)
            return MCNativeCharsEqualExact(self -> native_chars, self -> char_count, p_other -> native_chars, p_other -> char_count);
        else
            return MCNativeCharsEqualCaseless(self -> native_chars, self -> char_count, p_other -> native_chars, p_other -> char_count);
    }

    return MCUnicodeCompare(self -> chars, self -> char_count, self_native, p_other -> chars, p_other -> char_count, other_native, (MCUnicodeCompareOption)p_options) == 0;
}

bool MCStringIsEmpty(MCStringRef string)
{
	return string == nil || MCStringGetLength(string) == 0;
}

bool MCStringSubstringIsEqualTo(MCStringRef self, MCRange p_sub, MCStringRef p_other, MCStringOptions p_options)
{
    if (__MCStringIsIndirect(self))
        self = self -> string;
    
    if (__MCStringIsIndirect(p_other))
        p_other = p_other -> string;
    
	__MCStringClampRange(self, p_sub);
    
    bool self_native = __MCStringIsNative(self);
    if (self_native)
    {
        if (__MCStringIsNative(p_other))
        {
            if (p_options == kMCStringOptionCompareExact || p_options == kMCStringOptionCompareNonliteral)
                return MCNativeCharsEqualExact(self -> native_chars + p_sub . offset, p_sub . length, p_other -> native_chars, p_other -> char_count);
            else
                return MCNativeCharsEqualCaseless(self -> native_chars + p_sub . offset, p_sub . length, p_other -> native_chars, p_other -> char_count);
        }
        
        if (__MCStringCantBeEqualToNative(p_other, p_options))
            return false;
    }
    const void *self_chars;
    if (self_native)
        self_chars = self -> native_chars + p_sub . offset;
    else
        self_chars = self -> chars + p_sub . offset;
    return MCUnicodeCompare(self_chars, p_sub . length, self_native, p_other -> chars, p_other -> char_count, __MCStringIsNative(p_other), (MCUnicodeCompareOption)p_options) == 0;
}

bool MCStringSubstringIsEqualToSubstring(MCStringRef self, MCRange p_sub, MCStringRef p_other, MCRange p_other_sub, MCStringOptions p_options)
{
    if (__MCStringIsIndirect(self))
        self = self -> string;
    
    if (__MCStringIsIndirect(p_other))
        p_other = p_other -> string;
    
	__MCStringClampRange(self, p_sub);
    __MCStringClampRange(p_other, p_other_sub);
    
    bool self_native = __MCStringIsNative(self);
    bool other_native = __MCStringIsNative(p_other);
    if (self_native && other_native)
    {
        if (p_options == kMCStringOptionCompareExact || p_options == kMCStringOptionCompareNonliteral)
            return MCNativeCharsEqualExact(self -> native_chars + p_sub . offset, p_sub . length, p_other -> native_chars + p_other_sub . offset, p_other_sub . length);
        else
            return MCNativeCharsEqualCaseless(self -> native_chars + p_sub . offset, p_sub . length, p_other -> native_chars + p_other_sub . offset, p_other_sub . length);
    }
    
    const void *self_chars, *other_chars;
    if (self_native)
        self_chars = self -> native_chars + p_sub . offset;
    else
        self_chars = self -> chars + p_sub . offset;
    
    if (other_native)
        other_chars = p_other-> native_chars + p_other_sub . offset;
    else
        other_chars = p_other -> chars + p_other_sub . offset;
    
    return MCUnicodeCompare(self_chars, p_sub . length, self_native, other_chars, p_other_sub . length, other_native, (MCUnicodeCompareOption)p_options) == 0;
}

bool MCStringIsEqualToNativeChars(MCStringRef self, const char_t *p_chars, uindex_t p_char_count, MCStringOptions p_options)
{
    if (MCStringIsNative(self))
    {
        if (__MCStringIsIndirect(self))
            self = self -> string;
        
        if (p_options == kMCStringOptionCompareExact || p_options == kMCStringOptionCompareNonliteral)
            return MCNativeCharsEqualExact(self -> native_chars, self -> char_count, p_chars, p_char_count);
        
        return MCNativeCharsEqualCaseless(self -> native_chars, self -> char_count, p_chars, p_char_count);
    }
    
    if (MCStringCantBeEqualToNative(self, p_options))
        return false;
    
	MCAutoStringRef t_string;
	MCStringCreateWithNativeChars(p_chars, p_char_count, &t_string);
	return MCStringIsEqualTo(self, *t_string, p_options);
}

compare_t MCStringCompareTo(MCStringRef self, MCStringRef p_other, MCStringOptions p_options)
{
    if (__MCStringIsIndirect(self))
        self = self -> string;
    
    if (__MCStringIsIndirect(p_other))
        p_other = p_other -> string;

    return MCUnicodeCompare(self -> chars, self -> char_count, __MCStringIsNative(self), p_other -> chars, p_other -> char_count, __MCStringIsNative(p_other), (MCUnicodeCompareOption)p_options);
}

bool MCStringBeginsWith(MCStringRef self, MCStringRef p_prefix, MCStringOptions p_options)
{
    if (__MCStringIsIndirect(self))
        self = self -> string;
    
    if (__MCStringIsIndirect(p_prefix))
        p_prefix = p_prefix -> string;
    
    if (__MCStringIsNative(self))
    {
        if (__MCStringIsNative(p_prefix))
        {
            uindex_t t_prefix_length;
            if (p_options == kMCStringOptionCompareCaseless || p_options == kMCStringOptionCompareFolded)
                t_prefix_length = MCNativeCharsSharedPrefixCaseless(self -> native_chars, self -> char_count, p_prefix -> native_chars, p_prefix -> char_count);
            else
                t_prefix_length = MCNativeCharsSharedPrefixExact(self -> native_chars, self -> char_count, p_prefix -> native_chars, p_prefix -> char_count);
            
            return t_prefix_length == p_prefix -> char_count;
        }
        
        if (__MCStringCantBeEqualToNative(p_prefix, p_options))
            return false;
    }

    return MCUnicodeBeginsWith(self -> chars, self -> char_count, __MCStringIsNative(self), p_prefix -> chars, p_prefix -> char_count, __MCStringIsNative(p_prefix), (MCUnicodeCompareOption)p_options);
}

bool MCStringSharedPrefix(MCStringRef self, MCRange p_range, MCStringRef p_prefix, MCStringOptions p_options, uindex_t& r_self_match_length)
{
    if (__MCStringIsIndirect(self))
        self = self -> string;
    
    if (__MCStringIsIndirect(p_prefix))
        p_prefix = p_prefix -> string;
    
    __MCStringClampRange(self, p_range);
    
    bool self_native = __MCStringIsNative(self);
    if (self_native)
    {
        if (__MCStringIsNative(p_prefix))
        {
            if (p_options == kMCStringOptionCompareCaseless || p_options == kMCStringOptionCompareFolded)
                r_self_match_length = MCNativeCharsSharedPrefixCaseless(self -> native_chars + p_range . offset, p_range . length, p_prefix -> native_chars, p_prefix -> char_count);
            else
                r_self_match_length = MCNativeCharsSharedPrefixExact(self -> native_chars + p_range . offset, p_range . length, p_prefix -> native_chars, p_prefix -> char_count);
            
            return r_self_match_length == p_prefix -> char_count;
        }
        
        if (__MCStringCantBeEqualToNative(p_prefix, p_options))
            return false;
    }

    uindex_t t_prefix_share;
    
    const void *self_chars;
    if (self_native)
        self_chars = self -> native_chars + p_range . offset;
    else
        self_chars = self -> chars + p_range . offset;
    MCUnicodeSharedPrefix(self_chars, p_range . length, self_native, p_prefix -> chars, p_prefix -> char_count, MCStringIsNative(p_prefix), (MCUnicodeCompareOption)p_options, r_self_match_length, t_prefix_share);

    return t_prefix_share == __MCStringGetLength(p_prefix);
}

bool MCStringBeginsWithCString(MCStringRef self, const char_t *p_prefix_cstring, MCStringOptions p_options)
{
    if (__MCStringIsIndirect(self))
        self = self -> string;
    
    if (__MCStringIsNative(self))
    {
        uindex_t t_prefix_length;
        if (p_options == kMCStringOptionCompareCaseless || p_options == kMCStringOptionCompareFolded)
            t_prefix_length = MCNativeCharsSharedPrefixCaseless(self -> native_chars, self -> char_count, p_prefix_cstring, strlen((const char *)p_prefix_cstring));
        else
            t_prefix_length = MCNativeCharsSharedPrefixExact(self -> native_chars, self -> char_count, p_prefix_cstring, strlen((const char *)p_prefix_cstring));
        
        return t_prefix_length == strlen((const char *)p_prefix_cstring);
    }
    
	MCAutoStringRef t_string;
	MCStringCreateWithNativeChars(p_prefix_cstring, strlen((const char *)p_prefix_cstring), &t_string);
	return MCStringBeginsWith(self, *t_string, p_options);
}

bool MCStringEndsWith(MCStringRef self, MCStringRef p_suffix, MCStringOptions p_options)
{
    if (__MCStringIsIndirect(self))
        self = self -> string;
    
    if (__MCStringIsIndirect(p_suffix))
        p_suffix = p_suffix -> string;
    
    if (__MCStringIsNative(self))
    {
        if (__MCStringIsNative(p_suffix))
        {
            uindex_t t_prefix_length;
            if (p_options == kMCStringOptionCompareCaseless || p_options == kMCStringOptionCompareFolded)
                t_prefix_length = MCNativeCharsSharedSuffixCaseless(self -> native_chars, self -> char_count, p_suffix -> native_chars, p_suffix -> char_count);
            else
                t_prefix_length = MCNativeCharsSharedSuffixExact(self -> native_chars, self -> char_count, p_suffix -> native_chars, p_suffix -> char_count);
            
            // self begins with prefix iff t_prefix_length == length(prefix).
            return t_prefix_length == p_suffix -> char_count;
        }
        
        if (__MCStringCantBeEqualToNative(p_suffix, p_options))
            return false;
    }

    return MCUnicodeEndsWith(self -> chars, self -> char_count, __MCStringIsNative(self), p_suffix -> chars, p_suffix -> char_count, __MCStringIsNative(p_suffix), (MCUnicodeCompareOption)p_options);
}

bool MCStringSharedSuffix(MCStringRef self, MCRange p_range, MCStringRef p_suffix, MCStringOptions p_options, uindex_t& r_self_match_length)
{
    if (__MCStringIsIndirect(self))
        self = self -> string;
    
    if (__MCStringIsIndirect(p_suffix))
        p_suffix = p_suffix -> string;
    
    __MCStringClampRange(self, p_range);
    
    bool self_native = __MCStringIsNative(self);
    if (self_native)
    {
        if (__MCStringIsNative(p_suffix))
        {
            if (p_options == kMCStringOptionCompareCaseless || p_options == kMCStringOptionCompareFolded)
                r_self_match_length = MCNativeCharsSharedSuffixCaseless(self -> native_chars + p_range . offset, p_range . length, p_suffix -> native_chars, p_suffix -> char_count);
            else
                r_self_match_length = MCNativeCharsSharedSuffixExact(self -> native_chars + p_range . offset, p_range . length, p_suffix -> native_chars, p_suffix -> char_count);
            
            return r_self_match_length == p_suffix -> char_count;
        }
        
        if (__MCStringCantBeEqualToNative(p_suffix, p_options))
            return false;
    }

    uindex_t t_suffix_share;
    
    const void *self_chars;
    if (self_native)
        self_chars = self -> native_chars + p_range . offset;
    else
        self_chars = self -> chars + p_range . offset;
    MCUnicodeSharedSuffix(self_chars, p_range . length, self_native, p_suffix -> chars, p_suffix -> char_count, __MCStringIsNative(p_suffix), (MCUnicodeCompareOption)p_options, r_self_match_length, t_suffix_share);
    
    return t_suffix_share == MCStringGetLength(p_suffix);
}

bool MCStringEndsWithCString(MCStringRef self, const char_t *p_suffix_cstring, MCStringOptions p_options)
{
    if (__MCStringIsIndirect(self))
        self = self -> string;
    
    if (__MCStringIsNative(self))
    {
        uindex_t t_prefix_length;
        if (p_options == kMCStringOptionCompareCaseless || p_options == kMCStringOptionCompareFolded)
            t_prefix_length = MCNativeCharsSharedPrefixCaseless(self -> native_chars, self -> char_count, p_suffix_cstring, strlen((const char *)p_suffix_cstring));
        else
            t_prefix_length = MCNativeCharsSharedPrefixExact(self -> native_chars, self -> char_count, p_suffix_cstring, strlen((const char *)p_suffix_cstring));
        
        return t_prefix_length == strlen((const char *)p_suffix_cstring);
    }
    
	MCAutoStringRef t_string;
	MCStringCreateWithNativeChars(p_suffix_cstring, strlen((const char *)p_suffix_cstring), &t_string);
	return MCStringEndsWith(self, *t_string, p_options);
}

bool MCStringContains(MCStringRef self, MCStringRef p_needle, MCStringOptions p_options)
{
    if (MCStringIsEmpty(p_needle))
        return false;
    
    if (__MCStringIsIndirect(self))
        self = self -> string;
    
    if (__MCStringIsIndirect(p_needle))
        p_needle = p_needle -> string;

    if (__MCStringIsNative(self))
    {
        if (__MCStringIsNative(p_needle))
        {
            // Loop through self starting at each char in turn until we find a common prefix of
            // sufficient length.
            for(uindex_t t_offset = 0; t_offset < self -> char_count; t_offset += 1)
            {
                uindex_t t_prefix_length;
                if (p_options == kMCStringOptionCompareExact || p_options == kMCStringOptionCompareNonliteral)
                    t_prefix_length = MCNativeCharsSharedPrefixExact(self -> native_chars + t_offset, self -> char_count - t_offset, p_needle -> native_chars, p_needle -> char_count);
                else
                    t_prefix_length = MCNativeCharsSharedPrefixCaseless(self -> native_chars + t_offset, self -> char_count - t_offset, p_needle -> native_chars, p_needle -> char_count);
                
                // If the prefix length is the same as needle, we are done.
                if (t_prefix_length == p_needle -> char_count)
                    return true;
            }
            return false;
        }
        
        if (__MCStringCantBeEqualToNative(p_needle, p_options))
            return false;
    }

    return MCUnicodeContains(self -> chars, self -> char_count, __MCStringIsNative(self), p_needle -> chars, p_needle -> char_count, __MCStringIsNative(p_needle), (MCUnicodeCompareOption)p_options);
}

bool MCStringSubstringContains(MCStringRef self, MCRange p_range, MCStringRef p_needle, MCStringOptions p_options)
{
    if (__MCStringIsIndirect(p_needle))
        p_needle = p_needle -> string;
    
    // SN-2014-09-05: [[ Bug 13346 ]] Empty is *never* contained in a string. In the loop, a commong string of length 0
    // will be found, which unfortunaly matches the length of the empty needle.
    if (__MCStringIsEmpty(p_needle))
        return false;
    
    if (__MCStringIsIndirect(self))
        self = self -> string;
    
	__MCStringClampRange(self, p_range);

    bool self_native = __MCStringIsNative(self);
    if (self_native)
    {
        if (__MCStringIsNative(p_needle))
        {
            // Loop through self starting at each char in turn until we find a common prefix of
            // sufficient length.
            for(uindex_t t_offset = p_range . offset; t_offset < p_range . offset + p_range . length; t_offset += 1)
            {
                uindex_t t_prefix_length;
                if (p_options == kMCStringOptionCompareExact || p_options == kMCStringOptionCompareNonliteral)
                    t_prefix_length = MCNativeCharsSharedPrefixExact(self -> native_chars + t_offset, self -> char_count - t_offset, p_needle -> native_chars, p_needle -> char_count);
                else
                    t_prefix_length = MCNativeCharsSharedPrefixCaseless(self -> native_chars + t_offset, self -> char_count - t_offset, p_needle -> native_chars, p_needle -> char_count);
                
                // If the prefix length is the same as needle, we are done.
                if (t_prefix_length == p_needle -> char_count)
                    return true;
            }
            return false;
        }

        if (__MCStringCantBeEqualToNative(p_needle, p_options))
            return false;
    }
    
    const void *self_chars;
    if (self_native)
        self_chars = self -> native_chars + p_range . offset;
    else
        self_chars = self -> chars + p_range . offset;
    return MCUnicodeContains(self_chars, p_range . length, self_native, p_needle -> chars, p_needle -> char_count, __MCStringIsNative(p_needle), (MCUnicodeCompareOption)p_options);
}

////////////////////////////////////////////////////////////////////////////////

bool MCStringFirstIndexOf(MCStringRef self, MCStringRef p_needle, uindex_t p_after, MCStringOptions p_options, uindex_t& r_offset)
{
    if (__MCStringIsIndirect(self))
        self = self -> string;
    
    if (__MCStringIsIndirect(p_needle))
        p_needle = p_needle -> string;
    
	// Make sure we are not looking after the string length
	if (p_after > self -> char_count)
        return false;
        
    
    bool self_native = __MCStringIsNative(self);
    if (self_native)
    {
        if (__MCStringIsNative(p_needle))
        {
            for(uindex_t t_offset = p_after; t_offset < self -> char_count; t_offset += 1)
            {
                uindex_t t_prefix_length;
                if (p_options == kMCStringOptionCompareExact || p_options == kMCStringOptionCompareNonliteral)
                    t_prefix_length = MCNativeCharsSharedPrefixExact(self -> native_chars + t_offset, self -> char_count - t_offset, p_needle -> native_chars, p_needle -> char_count);
                else
                    t_prefix_length = MCNativeCharsSharedPrefixCaseless(self -> native_chars + t_offset, self -> char_count - t_offset, p_needle -> native_chars, p_needle -> char_count);
                
                // If the prefix length is the same as needle, we are done.
                if (t_prefix_length == p_needle -> char_count)
                {
                    r_offset = t_offset;
                    return true;
                }
            }
            return false;
        }
        
        if (__MCStringCantBeEqualToNative(p_needle, p_options))
            return false;
    }

    bool t_result;
    const void *self_chars;
    if (self_native)
        self_chars = self -> native_chars + p_after;
    else
        self_chars = self -> chars + p_after;
    
    // AL-2014-09-05: [[ Bug 13352 ]] Crash due to not taking into account p_after by adjusting length of string.
    t_result = MCUnicodeFirstIndexOf(self_chars, self -> char_count - p_after, self_native, p_needle -> chars, p_needle -> char_count, __MCStringIsNative(p_needle), (MCUnicodeCompareOption)p_options, r_offset);
    
    // Correct the output index
    if (t_result == true)
        r_offset += p_after;

    return t_result;
}

bool MCStringFirstIndexOfChar(MCStringRef self, codepoint_t p_needle, uindex_t p_after, MCStringOptions p_options, uindex_t& r_offset)
{
    return MCStringFirstIndexOfCharInRange(self, p_needle, MCRangeMake(p_after, self -> char_count - p_after), p_options, r_offset);
}

bool MCStringFirstIndexOfCharInRange(MCStringRef self, codepoint_t p_needle, MCRange p_range, MCStringOptions p_options, uindex_t& r_offset)
{
    if (__MCStringIsIndirect(self))
        self = self -> string;
    
    __MCStringClampRange(self, p_range);
    
    if (__MCStringIsNative(self))
    {
        if (p_needle >= 0xFF)
            return false;
        
        char_t t_char;
        t_char = (char_t)p_needle;
        if (p_options == kMCStringOptionCompareCaseless || p_options == kMCStringOptionCompareFolded)
            t_char = MCNativeCharFold(t_char);
        
        for(uindex_t t_offset = p_range . offset; t_offset < p_range . offset + p_range . length; t_offset += 1)
        {
            char_t t_other_char;
            t_other_char = self -> native_chars[t_offset];
            if (p_options == kMCStringOptionCompareCaseless || p_options == kMCStringOptionCompareFolded)
                t_other_char = MCNativeCharFold(t_other_char);
            
            if (t_other_char == t_char)
            {
                r_offset = t_offset;
                return true;
            }
        }
        return false;
    }
    
    bool t_result;
    t_result = MCUnicodeFirstIndexOfChar(self -> chars + p_range . offset, p_range . length, p_needle, (MCUnicodeCompareOption)p_options, r_offset);
    
    // Correct the output index
    if (t_result == true)
        r_offset += p_range . offset;
    
    return t_result;
}

bool MCStringLastIndexOf(MCStringRef self, MCStringRef p_needle, uindex_t p_before, MCStringOptions p_options, uindex_t& r_offset)
{
    if (__MCStringIsIndirect(self))
        self = self -> string;
    
    if (__MCStringIsIndirect(p_needle))
        p_needle = p_needle -> string;
    
	// Make sure the before index is in range.
	p_before = MCMin(p_before, self -> char_count);
    
    if (__MCStringIsNative(self))
    {
        if (__MCStringIsNative(p_needle))
        {
            for(uindex_t t_offset = p_before; t_offset > 0; t_offset -= 1)
            {
                // Compute the length of the shared prefix *before* offset - this means
                // we adjust offset down by one before comparing.
                uindex_t t_prefix_length;
                if (p_options == kMCStringOptionCompareCaseless || p_options == kMCStringOptionCompareFolded)
                    t_prefix_length = MCNativeCharsSharedPrefixCaseless(self -> native_chars + (t_offset - 1), self -> char_count - (t_offset - 1), p_needle -> native_chars, p_needle -> char_count);
                else
                    t_prefix_length = MCNativeCharsSharedPrefixExact(self -> native_chars + (t_offset - 1), self -> char_count - (t_offset - 1), p_needle -> native_chars, p_needle -> char_count);
                
                // If the prefix length is the same as the needle then we are done.
                if (t_prefix_length == p_needle -> char_count)
                {
                    r_offset = t_offset - 1;
                    return true;
                }
            }
            return false;
        }
        
        if (__MCStringCantBeEqualToNative(p_needle, p_options))
            return false;
    }

    return MCUnicodeLastIndexOf(self -> chars, p_before, __MCStringIsNative(self), p_needle -> chars, p_needle -> char_count, __MCStringIsNative(p_needle), (MCUnicodeCompareOption)p_options, r_offset);
}

bool MCStringLastIndexOfChar(MCStringRef self, codepoint_t p_needle, uindex_t p_before, MCStringOptions p_options, uindex_t& r_offset)
{
    if (__MCStringIsIndirect(self))
        self = self -> string;
    
	// Make sure the after index is in range.
	p_before = MCMin(p_before, self -> char_count);
    
    if (__MCStringIsNative(self))
    {
        if (p_needle >= 0xFF)
            return false;
        
        char_t t_char;
        t_char = (char_t)p_needle;
        if (p_options == kMCStringOptionCompareCaseless || p_options == kMCStringOptionCompareFolded)
            t_char = MCNativeCharFold(t_char);
        
        for(uindex_t t_offset = p_before; t_offset > 0; t_offset -= 1)
        {
            char_t t_other_char;
            t_other_char = self -> native_chars[t_offset - 1];
        if (p_options == kMCStringOptionCompareCaseless || p_options == kMCStringOptionCompareFolded)
                t_other_char = MCNativeCharFold(t_other_char);
            
            if (t_other_char == t_char)
            {
                r_offset = t_offset - 1;
                return true;
            }
        }
        return false;
    }
    
    return MCUnicodeLastIndexOfChar(self -> chars, p_before, p_needle, (MCUnicodeCompareOption)p_options, r_offset);
}

// Find where both needle and self are native.
static bool MCStringFindNative(MCStringRef self, MCRange p_range, MCStringRef p_needle, MCStringOptions p_options, MCRange *r_result)
{
	// Similar to contains, this searches for needle but only with range of self.
	// It also returns the the range in self that needle occupies (but only if
	// r_result is non-nil).
    
	// Compute the char ptr and length based on range.
	const char_t *t_chars;
	uindex_t t_char_count;
	t_chars = self -> native_chars + MCMin(p_range . offset, self -> char_count);
	t_char_count = MCMin(p_range . length, self -> char_count - (t_chars - self -> native_chars));
    
	// Loop through the char range until we find a common prefix of sufficient
	// length.
	for(uindex_t t_offset = 0; t_offset < t_char_count; t_offset += 1)
	{
		// Compute the length of the shared prefix at the current offset.
		uindex_t t_prefix_length;
        if (p_options == kMCStringOptionCompareCaseless || p_options == kMCStringOptionCompareFolded)
			t_prefix_length = MCNativeCharsSharedPrefixCaseless(t_chars + t_offset, t_char_count - t_offset, p_needle -> native_chars, p_needle -> char_count);
		else
			t_prefix_length = MCNativeCharsSharedPrefixExact(t_chars + t_offset, t_char_count - t_offset, p_needle -> native_chars, p_needle -> char_count);
        
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
                if (p_options == kMCStringOptionCompareCaseless || p_options == kMCStringOptionCompareFolded)
					t_prefix_length = MCNativeCharsSharedPrefixCaseless(p_needle -> native_chars, p_needle -> char_count, t_chars + t_offset, t_char_count - t_offset);
				else
					t_prefix_length = MCNativeCharsSharedPrefixExact(p_needle -> native_chars, p_needle -> char_count, t_chars + t_offset, t_char_count - t_offset);
                
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

bool MCStringFind(MCStringRef self, MCRange p_range, MCStringRef p_needle, MCStringOptions p_options, MCRange *r_result)
{
    if (__MCStringIsIndirect(self))
        self = self -> string;
    
    if (__MCStringIsIndirect(p_needle))
        p_needle = p_needle -> string;
    
    __MCStringClampRange(self, p_range);
    
    // Circumvent performance hit due to possibility of case / form sensitivity affecting delimiter search.
    // TODO: Implement properly, based on properties of the needle string.
    if (__MCStringGetLength(p_needle) == 1)
    {
        codepoint_t t_codepoint =  MCStringGetCodepointAtIndex(p_needle, 0);
        // if codepoint is among first 64 ASCII characters then do case and form sensitive comparison.
        if (t_codepoint < 0x41)
            p_options = kMCStringOptionCompareExact;
    }
    bool self_native = __MCStringIsNative(self);
    if (self_native)
    {
        if (__MCStringIsNative(p_needle))
            return MCStringFindNative(self, p_range, p_needle, p_options, r_result);
        
        if (MCStringCantBeEqualToNative(p_needle, p_options))
            return false;
    }

    // Similar to contains, this searches for needle but only with range of self.
	// It also returns the the range in self that needle occupies (but only if
	// r_result is non-nil).

    bool t_result;
    MCRange t_range;
    t_result = MCUnicodeFind(self->native_chars + (self_native ? p_range . offset : 2 * p_range . offset), p_range . length, __MCStringIsNative(self), p_needle -> chars, p_needle -> char_count, __MCStringIsNative(p_needle), (MCUnicodeCompareOption)p_options, t_range);
    
    // Correct the range
    t_range.offset += p_range.offset;
    
    if (r_result != nil)
        *r_result = t_range;
    return t_result;
}

static uindex_t MCStringCountNativeChars(MCStringRef self, MCRange p_range, const char_t *p_needle_chars, uindex_t p_needle_char_count, MCStringOptions p_options)
{
    if (__MCStringIsIndirect(self))
        self = self -> string;
    
	// Keep track of how many occurances have been found.
	uindex_t t_count;
	t_count = 0;
    
	// Compute the char ptr and length based on range.
	const char_t *t_chars;
	uindex_t t_char_count;
	t_chars = self -> native_chars + MCMin(p_range . offset, self -> char_count);
	t_char_count = MCMin(p_range . length, self -> char_count - (t_chars - self -> native_chars));
    
	// Loop through the char range checking for occurances of needle.
	uindex_t t_offset;
	t_offset = 0;
	while(t_offset < t_char_count)
	{
		// Compute the length of the shared prefix at the current offset.
		uindex_t t_prefix_length;
        if (p_options == kMCStringOptionCompareCaseless || p_options == kMCStringOptionCompareFolded)
			t_prefix_length = MCNativeCharsSharedPrefixCaseless(t_chars + t_offset, t_char_count - t_offset, p_needle_chars, p_needle_char_count);
		else
			t_prefix_length = MCNativeCharsSharedPrefixExact(t_chars + t_offset, t_char_count - t_offset, p_needle_chars, p_needle_char_count);
        
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
    
	// Return the number of occurrences.
	return t_count;
}

static uindex_t MCStringCountStrChars(MCStringRef self, MCRange p_range, const void *p_needle_chars, uindex_t p_needle_char_count, bool p_needle_native, MCStringOptions p_options)
{
    if (__MCStringIsIndirect(self))
        self = self -> string;
    
	// Keep track of how many occurrences have been found.
	uindex_t t_count;
	t_count = 0;

    __MCStringClampRange(self, p_range);
	
    bool self_native = __MCStringIsNative(self);
    const void *self_chars;
    if (self_native)
        self_chars = self -> native_chars + p_range . offset;
    else
        self_chars = self -> chars + p_range . offset;
    
	// Loop through the char range checking for occurrences of needle.
	uindex_t t_offset;
	t_offset = p_range . offset;
	while(t_offset < p_range . offset + p_range . length)
	{
		// Compute the length of the shared prefix at the current offset.
		uindex_t t_prefix_length;
        uindex_t t_ignored;
        MCUnicodeSharedPrefix((const char *)self_chars + (self_native ? t_offset : (t_offset * 2)), p_range . offset + p_range . length - t_offset, self_native, p_needle_chars, p_needle_char_count, p_needle_native, (MCUnicodeCompareOption)p_options, t_ignored, t_prefix_length);
        
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

	// Return the number of occurrences.
	return t_count;
}

uindex_t MCStringCount(MCStringRef self, MCRange p_range, MCStringRef p_needle, MCStringOptions p_options)
{
    if (__MCStringIsIndirect(p_needle))
        p_needle = p_needle -> string;
    
    if (MCStringIsNative(self))
    {
        if (__MCStringIsNative(p_needle))
            return MCStringCountNativeChars(self, p_range, p_needle -> native_chars, p_needle -> char_count, p_options);
        
        if (__MCStringCantBeEqualToNative(p_needle, p_options))
            return 0;
    }
    
	uindex_t t_count = MCStringCountStrChars(self, p_range, p_needle -> chars, p_needle -> char_count, __MCStringIsNative(p_needle), p_options);
    
    return t_count;
}

uindex_t MCStringCountChar(MCStringRef self, MCRange p_range, codepoint_t p_needle, MCStringOptions p_options)
{
	// We only support ASCII for now.
	//MCAssert(p_needle < 128);
	
	strchar_t t_native_needle;
	t_native_needle = (strchar_t)p_needle;
	
    if (MCStringIsNative(self))
    {
        if (p_needle >= 0xFF)
            return 0;
        
        return MCStringCountNativeChars(self, p_range, (const char_t *)&p_needle, 1, p_options);
    }
    
	return MCStringCountStrChars(self, p_range, &t_native_needle, 1, false, p_options);
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
	// SN-2014-11-13: [[ Bug 13993 ]] No delimiter found means 1 range, 1 delimiter means 2 ranges, etc.
	t_range_count = MCStringCountChar(self, MCRangeMake(0, MCStringGetLength(self)), p_separator, p_options) + 1;
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
		// SN-2014-11-13: [[ Bug 13993 ]] The offset might be after the last char, if the previous delimiter
		// was the last char of the string. We are done in that case.
		if (t_prev_offset == MCStringGetLength(self))
			break;

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
	if (p_options == kMCStringOptionCompareExact || p_options == kMCStringOptionCompareNonliteral)
		return true;

    // Ensure the string is not indirect.
    if (__MCStringIsIndirect(self))
        if (!__MCStringResolveIndirect(self))
            return false;
    
    if (__MCStringIsNative(self))
    {
        MCNativeCharsLowercase(self -> native_chars, self -> char_count);
        __MCStringChanged(self, true, true, true);
        return true;
    }
    
    // Case-fold the string
    unichar_t *t_folded;
    uindex_t t_folded_length;
    if (!MCUnicodeCaseFold(self -> chars, self -> char_count, t_folded, t_folded_length))
        return false;
    
    // Update the string
    MCMemoryDeleteArray(self -> chars);
    self -> chars = t_folded;
    self -> char_count = t_folded_length;

	__MCStringChanged(self);
	
	// We always succeed (at the moment)
	return true;
}

bool MCStringLowercase(MCStringRef self, MCLocaleRef p_locale)
{
	MCAssert(MCStringIsMutable(self));

    // Ensure the string is not indirect.
    if (__MCStringIsIndirect(self))
        if (!__MCStringResolveIndirect(self))
            return false;
    
    if (__MCStringIsNative(self))
    {
        MCNativeCharsLowercase(self -> native_chars, self -> char_count);
        __MCStringChanged(self, true, true, true);
        return true;
    }
    
	// Case transformations can change string lengths
    unichar_t *t_lowered;
    uindex_t t_lowered_length;
    if (!MCUnicodeLowercase(p_locale, self -> chars, self -> char_count, t_lowered, t_lowered_length))
        return false;
    
    MCMemoryDeleteArray(self -> chars);
    self -> chars = t_lowered;
    self -> char_count = t_lowered_length;
	
	__MCStringChanged(self);
	
	// We always succeed (at the moment)
	return true;
}

bool MCStringUppercase(MCStringRef self, MCLocaleRef p_locale)
{
	MCAssert(MCStringIsMutable(self));
    
    // Ensure the string is not indirect.
    if (__MCStringIsIndirect(self))
        if (!__MCStringResolveIndirect(self))
            return false;
    
    if (__MCStringIsNative(self))
    {
        MCNativeCharsUppercase(self -> native_chars, self -> char_count);
        __MCStringChanged(self, true, true, true);
        return true;
    }
    
	// Case transformations can change string lengths
    unichar_t *t_lowered;
    uindex_t t_lowered_length;
    if (!MCUnicodeUppercase(p_locale, self -> chars, self -> char_count, t_lowered, t_lowered_length))
        return false;
    
    MCMemoryDeleteArray(self -> chars);
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

    if (__MCStringIsIndirect(p_suffix))
        p_suffix = p_suffix -> string;
    
    // Only do the append now if self != suffix.
	if (self != p_suffix)
	{
        if (__MCStringIsNative(p_suffix))
            return MCStringAppendNativeChars(self, p_suffix -> native_chars, p_suffix -> char_count);
    
        return MCStringAppendChars(self, p_suffix -> chars, p_suffix -> char_count);
    }
    
    // Otherwise copy and recurse.
	MCAutoStringRef t_suffix_copy;
	MCStringCopy(p_suffix, &t_suffix_copy);
	return MCStringAppend(self, *t_suffix_copy);
}

bool MCStringAppendSubstring(MCStringRef self, MCStringRef p_suffix, MCRange p_range)
{
	MCAssert(MCStringIsMutable(self));
  
    if (__MCStringIsIndirect(p_suffix))
        p_suffix = p_suffix -> string;

	// Only do the append now if self != suffix.
	if (self != p_suffix)
	{
        __MCStringClampRange(p_suffix, p_range);
        
        if (__MCStringIsNative(p_suffix))
            return MCStringAppendNativeChars(self, p_suffix -> native_chars + p_range . offset, p_range . length);
        
        return MCStringAppendChars(self, p_suffix -> chars + p_range . offset, p_range . length);
    }
    
    // Otherwise copy substring and append.
	MCAutoStringRef t_suffix_substring;
	return MCStringCopySubstring(p_suffix, p_range, &t_suffix_substring) &&
    MCStringAppend(self, *t_suffix_substring);
}

bool MCStringAppendNativeChars(MCStringRef self, const char_t *p_chars, uindex_t p_char_count)
{
	MCAssert(MCStringIsMutable(self));
	
    // Ensure the string is not indirect.
    if (__MCStringIsIndirect(self))
        if (!__MCStringResolveIndirect(self))
            return false;
    
	// Ensure we have enough room in self - with the gap at the end.
	if (!__MCStringExpandAt(self, self -> char_count, p_char_count))
		return false;

	if (__MCStringIsNative(self))
    {
        MCMemoryCopy(self -> native_chars + self -> char_count - p_char_count, p_chars, p_char_count);
        self -> native_chars[self -> char_count] = '\0';
        __MCStringChanged(self, true, true, true);
        return true;
    }
    
	// Now copy the chars across.
	for(uindex_t i = 0; i < p_char_count; i++)
        self -> chars[i + self -> char_count - p_char_count] = MCUnicodeCharMapFromNative(p_chars[i]);

	// Set the NULL
    self -> chars[self -> char_count] = '\0';
	
    // Appending native chars cannot change the simple status
	__MCStringChanged(self);
    
	// We succeeded.
	return true;
}

bool MCStringAppendChars(MCStringRef self, const unichar_t *p_chars, uindex_t p_char_count)
{
	MCAssert(MCStringIsMutable(self));
    
    // Ensure the string is not indirect.
    if (__MCStringIsIndirect(self))
        if (!__MCStringResolveIndirect(self))
            return false;
	
    // Ensure we have enough room in self - with the gap at the end.
    if (!__MCStringExpandAt(self, self -> char_count, p_char_count))
        return false;
    
    // If we are native, attempt a native copy of the input chars.
    if (__MCStringIsNative(self))
    {
        bool t_not_native;
        t_not_native = false;
        for(uindex_t i = 0; i < p_char_count; i++)
            if (!MCUnicodeCharMapToNative(p_chars[i], self -> native_chars[i + self -> char_count - p_char_count]))
            {
                t_not_native = true;
                break;
            }
        
        if (!t_not_native)
        {
            self -> native_chars[self -> char_count] = '\0';
            __MCStringChanged(self, true, true, true);
            return true;
        }
        
        // At least one of the additional chars was not native.
        __MCStringShrinkAt(self, self -> char_count - p_char_count, p_char_count);
        __MCStringUnnativize(self);
        return MCStringAppendChars(self, p_chars, p_char_count);
    }
    
    // Copy the chars across recomputing whether the string can be native at
    // the same time.
    bool t_can_be_native;
    t_can_be_native = __MCStringCopyChars(self -> chars + self -> char_count - p_char_count, p_chars, p_char_count, __MCStringCanBeNative(self));
	
	// Set the NULL
	self -> chars[self -> char_count] = '\0';
	
	__MCStringChanged(self, false, false, t_can_be_native);
	
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
    
    if (__MCStringIsIndirect(p_prefix))
        p_prefix = p_prefix -> string;
    
 	// Only do the prepend now if self != prefix.
	if (self != p_prefix)
	{
        if (__MCStringIsNative(p_prefix))
            return MCStringPrependNativeChars(self, p_prefix -> native_chars, p_prefix -> char_count);
        
        return MCStringPrependChars(self, p_prefix -> chars, p_prefix -> char_count);
    }
    
    // Otherwise copy and recurse.
	MCAutoStringRef t_prefix_copy;
	MCStringCopy(p_prefix, &t_prefix_copy);
	return MCStringPrepend(self, *t_prefix_copy);
}

bool MCStringPrependSubstring(MCStringRef self, MCStringRef p_prefix, MCRange p_range)
{
	MCAssert(MCStringIsMutable(self));

    if (__MCStringIsIndirect(p_prefix))
        p_prefix = p_prefix -> string;
    
    // Only do the prepend now if self != prefix.
	if (self != p_prefix)
	{
        __MCStringClampRange(p_prefix, p_range);
        
        if (__MCStringIsNative(p_prefix))
            return MCStringAppendNativeChars(self, p_prefix -> native_chars + p_range . offset, p_range . length);
        
        return MCStringAppendChars(self, p_prefix -> chars + p_range . offset, p_range . length);
    }
    
    // Otherwise copy substring and prepend.
	MCAutoStringRef t_prefix_substring;
	return MCStringCopySubstring(p_prefix, p_range, &t_prefix_substring) &&
    MCStringPrepend(self, *t_prefix_substring);
}

bool MCStringPrependNativeChars(MCStringRef self, const char_t *p_chars, uindex_t p_char_count)
{
	MCAssert(MCStringIsMutable(self));
	
    // Ensure the string is not indirect.
    if (__MCStringIsIndirect(self))
        if (!__MCStringResolveIndirect(self))
            return false;
    
	// Ensure we have enough room in self - with the gap at the beginning.
	if (!__MCStringExpandAt(self, 0, p_char_count))
		return false;
	
    if (__MCStringIsNative(self))
    {
        MCMemoryCopy(self -> native_chars, p_chars, p_char_count);
        __MCStringChanged(self, true, true, true);
        return true;
    }
    
	// Now copy the chars across.
	for(uindex_t i = 0; i < p_char_count; i++)
		self -> chars[i] = MCUnicodeCharMapFromNative(p_chars[i]);
	
    // Prepending native chars cannot change the simple status
	__MCStringChanged(self);
	
	// We succeeded.
	return true;
}

bool MCStringPrependChars(MCStringRef self, const unichar_t *p_chars, uindex_t p_char_count)
{
	MCAssert(MCStringIsMutable(self));
	
    // Ensure the string is not indirect.
    if (__MCStringIsIndirect(self))
        if (!__MCStringResolveIndirect(self))
            return false;
	
    // Ensure we have enough room in self - with the gap at the end.
    if (!__MCStringExpandAt(self, 0, p_char_count))
        return false;
    
    // If we are native, attempt a native copy of the input chars.
    if (__MCStringIsNative(self))
    {
        bool t_not_native;
        t_not_native = false;
        for(uindex_t i = 0; i < p_char_count; i++)
            // SN-2014-05-20 [[ Bug 12344 ]] [[ Bug 12345 ]]
            // Prepending chars was appending them
            if (!MCUnicodeCharMapToNative(p_chars[i], self -> native_chars[i]))
            {
                t_not_native = true;
                break;
            }
        
        if (!t_not_native)
        {
            self -> native_chars[self -> char_count] = '\0';
            __MCStringChanged(self, true, true, true);
            return true;
        }
        
        // At least one of the additional chars was not native.
        __MCStringShrinkAt(self, 0, p_char_count);
        __MCStringUnnativize(self);
        return MCStringPrependChars(self, p_chars, p_char_count);
    }
    
    // Copy the chars across recomputing whether the string can be native at
    // the same time.
    bool t_can_be_native;
    t_can_be_native = __MCStringCopyChars(self -> chars, p_chars, p_char_count, __MCStringCanBeNative(self));
	
	__MCStringChanged(self, false, false, t_can_be_native);
	
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

    if (__MCStringIsIndirect(p_substring))
        p_substring = p_substring -> string;
    
	// Only do the insert now if self != substring.
	if (self != p_substring)
	{
        if (__MCStringIsNative(p_substring))
            return MCStringInsertNativeChars(self, p_at, p_substring -> native_chars, p_substring -> char_count);
        
        return MCStringInsertChars(self, p_at, p_substring -> chars, p_substring -> char_count);
    }
    
    // Otherwise copy and recurse.
	MCAutoStringRef t_substring_copy;
	MCStringCopy(p_substring, &t_substring_copy);
	return MCStringInsert(self, p_at, *t_substring_copy);
}

bool MCStringInsertSubstring(MCStringRef self, uindex_t p_at, MCStringRef p_substring, MCRange p_range)
{
	MCAssert(MCStringIsMutable(self));

    if (__MCStringIsIndirect(p_substring))
        p_substring = p_substring -> string;
    
	// Only do the insert now if self != substring.
	if (self != p_substring)
	{
        if (__MCStringIsNative(p_substring))
            return MCStringInsertNativeChars(self, p_at, p_substring -> native_chars + p_range . offset, p_range . length);
        
        return MCStringInsertChars(self, p_at, p_substring -> chars + p_range . offset, p_range . length);
    }
    
	// Otherwise copy substring and insert.
	MCAutoStringRef t_substring_substring;
	return MCStringCopySubstring(p_substring, p_range, &t_substring_substring) &&
    MCStringInsert(self, p_at, *t_substring_substring);
}

bool MCStringInsertNativeChars(MCStringRef self, uindex_t p_at, const char_t *p_chars, uindex_t p_char_count)
{
	MCAssert(MCStringIsMutable(self));
	
    // Ensure the string is not indirect.
    if (__MCStringIsIndirect(self))
        if (!__MCStringResolveIndirect(self))
            return false;
    
	p_at = MCMin(p_at, self -> char_count);

	// Ensure we have enough room in self - with the gap at p_at.
	if (!__MCStringExpandAt(self, p_at, p_char_count))
		return false;
	
    if (__MCStringIsNative(self))
    {
        MCMemoryCopy(self -> native_chars + p_at, p_chars, p_char_count);
        __MCStringChanged(self, true, true, true);
        return true;
    }
    
	// Now copy the chars across.
	for(uindex_t i = 0; i < p_char_count; i++)
		self -> chars[p_at + i] = MCUnicodeCharMapFromNative(p_chars[i]);
	
    // Inserting native chars cannot change simple status    
	__MCStringChanged(self);
	
	// We succeeded.
	return true;
}

bool MCStringInsertChars(MCStringRef self, uindex_t p_at, const unichar_t *p_chars, uindex_t p_char_count)
{
	MCAssert(MCStringIsMutable(self));

    // Ensure the string is not indirect.
    if (__MCStringIsIndirect(self))
        if (!__MCStringResolveIndirect(self))
            return false;
    
	p_at = MCMin(p_at, self -> char_count);
	
	// Ensure we have enough room in self - with the gap at the p_at.
	if (!__MCStringExpandAt(self, p_at, p_char_count))
		return false;
	
    if (__MCStringIsNative(self))
    {
        bool t_not_native;
        t_not_native = false;
        for(uindex_t i = 0; i < p_char_count; i++)
            if (!MCUnicodeCharMapToNative(p_chars[i], self -> native_chars[i + p_at]))
            {
                t_not_native = true;
                break;
            }
        
        if (!t_not_native)
        {
            self -> native_chars[self -> char_count] = '\0';
            __MCStringChanged(self, true, true, true);
            return true;
        }
        
        __MCStringShrinkAt(self, p_at, p_char_count);
        __MCStringUnnativize(self);
        return MCStringInsertChars(self, p_at, p_chars, p_char_count);
    }
    
    // Need to clamp p_at again in case number of codeunits has decreased.
    p_at = MCMin(p_at, self -> char_count);
    
    // Copy the chars across recomputing whether the string can be native at
    // the same time.
	bool t_can_be_native;
    t_can_be_native = __MCStringCopyChars(self -> chars + p_at, p_chars, p_char_count, __MCStringCanBeNative(self));
	
	__MCStringChanged(self, false, false, t_can_be_native);
	
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

    // Ensure the string is not indirect.
    if (__MCStringIsIndirect(self))
        if (!__MCStringResolveIndirect(self))
            return false;
    
	__MCStringClampRange(self, p_range);

	// Copy down the chars above the string taking into account the implicit
	// NUL.
	__MCStringShrinkAt(self, p_range . offset, p_range . length);
	
    if (!__MCStringIsNative(self))
        __MCStringChanged(self, false, false);
    else
        __MCStringChanged(self, true, true, true);
	
	// We succeeded.
	return true;
}

bool MCStringSubstring(MCStringRef self, MCRange p_range)
{
	MCAssert(MCStringIsMutable(self));

    // Ensure the string is not indirect.
    if (__MCStringIsIndirect(self))
        if (!__MCStringResolveIndirect(self))
            return false;
    
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

bool MCStringReplaceNativeChars(MCStringRef self, MCRange p_range, const char_t *p_chars, uindex_t p_char_count)
{
    MCAssert(MCStringIsMutable(self));
    
    // Ensure the string is not indirect.
    if (__MCStringIsIndirect(self))
        if (!__MCStringResolveIndirect(self))
            return false;
    
    __MCStringClampRange(self, p_range);
    
    // Work out the new size of the string.
    uindex_t t_new_char_count;
    t_new_char_count = self -> char_count - p_range . length + p_char_count;
    
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
    
    if (__MCStringIsNative(self))
    {
        // Copy across the replacement chars.
        MCMemoryCopy(self -> native_chars + p_range . offset, p_chars, p_char_count);
        __MCStringChanged(self, true, true, true);
        return true;
    }
    
	// Now copy the chars across.
	for(uindex_t i = 0; i < p_char_count; i++)
		self -> chars[i + p_range . offset] = MCUnicodeCharMapFromNative(p_chars[i]);
    
    __MCStringChanged(self);
    
    // We succeeded.
    return true;
}

bool MCStringReplaceChars(MCStringRef self, MCRange p_range, const unichar_t *p_chars, uindex_t p_char_count)
{
    MCAssert(MCStringIsMutable(self));
    
    // Ensure the string is not indirect.
    if (__MCStringIsIndirect(self))
        if (!__MCStringResolveIndirect(self))
            return false;
    
    __MCStringClampRange(self, p_range);
    
    // Work out the new size of the string.
    uindex_t t_new_char_count;
    t_new_char_count = self -> char_count - p_range . length + p_char_count;
    
    index_t t_change;
    t_change = t_new_char_count - self -> char_count;
    if (t_change > 0)
    {
        // Expand the string at the end of the range by the amount extra we
        // need.
        if (!__MCStringExpandAt(self, p_range . offset + p_range . length, t_change))
            return false;
    }
    else if (t_change < 0)
    {
        // Shrink the last part of the range by the amount less we need.
        __MCStringShrinkAt(self, p_range . offset + (p_range . length + t_change), -t_change);
    }
    
    if (__MCStringIsNative(self))
    {
        bool t_not_native;
        t_not_native = false;
        for(uindex_t i = 0; i < p_char_count; i++)
            if (!MCUnicodeCharMapToNative(p_chars[i], self -> native_chars[i + p_range . offset]))
            {
                t_not_native = true;
                break;
            }
        
        if (!t_not_native)
        {
            self -> native_chars[self -> char_count] = '\0';
            __MCStringChanged(self, true, true, true);
            return true;
        }
        
        if (t_change > 0)
            __MCStringShrinkAt(self, p_range . offset + p_range . length, t_change);
        else if (t_change < 0)
        {
            if (!__MCStringExpandAt(self, p_range . offset + (p_range . length + t_change), -t_change))
                return false;
        }
        
        __MCStringUnnativize(self);
        return MCStringReplaceChars(self, p_range, p_chars, p_char_count);
    }
    
    // Need to clamp range again in case the number of codeunits has decreased.
    __MCStringClampRange(self, p_range);
    
    // Copy the chars across recomputing whether the string can be native at
    // the same time.
    bool t_can_be_native;
    t_can_be_native = __MCStringCopyChars(self -> chars + p_range . offset, p_chars, p_char_count, __MCStringCanBeNative(self));
    
    __MCStringChanged(self, false, false, t_can_be_native);
    
    // We succeeded.
    return true;
}

bool MCStringReplace(MCStringRef self, MCRange p_range, MCStringRef p_replacement)
{
    if (__MCStringIsIndirect(p_replacement))
        p_replacement = p_replacement -> string;
    
	// Only do the replace now if self != substring.
	if (self != p_replacement)
	{
        if (__MCStringIsNative(p_replacement))
            return MCStringReplaceNativeChars(self, p_range, p_replacement -> native_chars, p_replacement -> char_count);
        
        return MCStringReplaceChars(self, p_range, p_replacement -> chars, p_replacement -> char_count);
    }
    
    // Otherwise copy and recurse.
	MCAutoStringRef t_replacement_copy;
	MCStringCopy(p_replacement, &t_replacement_copy);
	return MCStringReplace(self, p_range, *t_replacement_copy);
}

bool MCStringPad(MCStringRef self, uindex_t p_at, uindex_t p_count, MCStringRef p_value)
{
    // Ensure the string is not indirect.
    if (__MCStringIsIndirect(self))
        if (!__MCStringResolveIndirect(self))
            return false;
    
    if (__MCStringIsIndirect(p_value))
        p_value = p_value -> string;
    
	if (!__MCStringExpandAt(self, p_at, p_count * (p_value != nil ? p_value -> char_count : 1)))
		return false;

	if (p_value != nil)
		for(uindex_t i = 0; i < p_count; i++)
			MCMemoryCopy(self -> chars + p_at + i * p_value -> char_count, p_value -> chars, p_value -> char_count * sizeof(strchar_t));
	
	__MCStringChanged(self);
	
	return true;
}

bool MCStringResolvesLeftToRight(MCStringRef self)
{
    if (MCStringIsNative(self) || MCStringCanBeNative(self))
        return true;
    
    return MCBidiFirstStrongIsolate(self, 0) == 0;
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

static void split_find_end_of_element_native(const char_t *sptr, const char_t *eptr, const char_t *del, uindex_t p_del_length, const char_t*& r_end_ptr, MCStringOptions p_options)
{
	while(sptr < eptr - p_del_length + 1)
	{
        // Compute the length of the shared prefix at the current offset.
		uindex_t t_prefix_length;
        if (p_options == kMCStringOptionCompareCaseless || p_options == kMCStringOptionCompareFolded)
			t_prefix_length = MCNativeCharsSharedPrefixCaseless(sptr, eptr - sptr, del, p_del_length);
		else
			t_prefix_length = MCNativeCharsSharedPrefixExact(sptr, eptr - sptr, del, p_del_length);
		if (t_prefix_length == p_del_length)
		{
			r_end_ptr = sptr;
			return;
		}
        
		sptr += 1;
	}
	r_end_ptr = eptr;
}

static void split_find_end_of_element_and_key_native(const char_t *sptr, const char_t *eptr, const char_t *del, uindex_t p_del_length, const char_t *key, uindex_t p_key_length, const char_t*& r_key_ptr, const char_t *& r_end_ptr, MCStringOptions p_options)
{
    while(sptr < eptr - p_key_length + 1)
    {
        // Compute the length of the shared prefix at the current offset.
        uindex_t t_prefix_length;
        if (p_options == kMCStringOptionCompareCaseless || p_options == kMCStringOptionCompareFolded)
            t_prefix_length = MCNativeCharsSharedPrefixCaseless(sptr, eptr - sptr, key, p_key_length);
        else
            t_prefix_length = MCNativeCharsSharedPrefixExact(sptr, eptr - sptr, key, p_key_length);
        if (t_prefix_length == p_key_length)
        {
			r_key_ptr = sptr;
			break;
        }
        
        if (sptr < eptr - p_del_length + 1)
        {
            if (p_options == kMCStringOptionCompareCaseless || p_options == kMCStringOptionCompareFolded)
                t_prefix_length = MCNativeCharsSharedPrefixCaseless(sptr, eptr - sptr, del, p_del_length);
            else
                t_prefix_length = MCNativeCharsSharedPrefixExact(sptr, eptr - sptr, del, p_del_length);
            if (t_prefix_length == p_del_length)
            {
                r_key_ptr = r_end_ptr = sptr;
                return;
            }
        }
		sptr += 1;
	}
    
    // key not found
    if (sptr == eptr - p_key_length + 1)
        r_key_ptr = sptr;
    
	split_find_end_of_element_native(sptr, eptr, del, p_del_length, r_end_ptr, p_options);
}

bool MCStringSplitNative(MCStringRef self, MCStringRef p_elem_del, MCStringRef p_key_del, MCStringOptions p_options, MCArrayRef& r_array)
{
	MCAutoArrayRef t_array;
	if (!MCArrayCreateMutable(&t_array))
		return false;

    if (__MCStringIsIndirect(self))
        self = self -> string;
    
    if (__MCStringIsIndirect(p_elem_del))
        p_elem_del = p_elem_del -> string;
    
    
    const char_t *t_sptr;
    const char_t *t_eptr;
    t_sptr = self -> native_chars;
    t_eptr = self -> native_chars + self -> char_count;
    
	if (p_key_del == nil)
	{
        uindex_t t_index;
		t_index = 1;
        
        for(;;)
        {
            const char_t *t_element_end;
            split_find_end_of_element_native(t_sptr, t_eptr, p_elem_del -> native_chars, p_elem_del -> char_count, t_element_end, p_options);
            
            MCAutoStringRef t_string;
            if (!MCStringCreateWithNativeChars(t_sptr, t_element_end - t_sptr, &t_string))
                return false;
            
            if (!MCArrayStoreValueAtIndex(*t_array, t_index, *t_string))
                return false;
            
            if (t_element_end + p_elem_del -> char_count >= t_eptr)
                break;
            
            t_index += 1;
            
            t_sptr = t_element_end + p_elem_del -> char_count;
        }
	}
	else
	{
        if (__MCStringIsIndirect(p_key_del))
            p_elem_del = p_key_del -> string;
        
		for(;;)
		{
			const char_t *t_element_end;
			const char_t *t_key_end;

            split_find_end_of_element_and_key_native(t_sptr, t_eptr, p_elem_del -> native_chars, p_elem_del -> char_count, p_key_del -> native_chars, p_key_del -> char_count, t_key_end, t_element_end, p_options);
            
			MCNewAutoNameRef t_name;
			if (!MCNameCreateWithNativeChars(t_sptr, t_key_end - t_sptr, &t_name))
				return false;
            
			if (t_key_end != t_element_end)
				t_key_end += p_key_del -> char_count;
            
			MCAutoStringRef t_string;
			if (!MCStringCreateWithNativeChars(t_key_end, t_element_end - t_key_end, &t_string))
				return false;
            
			if (!MCArrayStoreValue(*t_array, true, *t_name, *t_string))
				return false;
            
			if (t_element_end + p_elem_del -> char_count >= t_eptr)
				break;
            
			t_sptr = t_element_end + p_elem_del -> char_count;
		}
	}
    
	if (!MCArrayCopy(*t_array, r_array))
		return false;
    
	return true;
}

bool MCStringFindAndReplaceChar(MCStringRef self, char_t p_pattern, char_t p_replacement, MCStringOptions p_options)
{
	if (p_options == kMCStringOptionCompareExact || p_options == kMCStringOptionCompareNonliteral)
	{
		// Simplest case, just substitute pattern for replacement.
		for(uindex_t i = 0; i < self -> char_count; i++)
			if (self -> native_chars[i] == p_pattern)
				self -> native_chars[i] = p_replacement;
	}
	else
	{
		char_t t_from;
		t_from = MCNativeCharFold(p_pattern);
        
		// Now substitute pattern for replacement, taking making sure its a caseless compare.
		for(uindex_t i = 0; i < self -> char_count; i++)
			if (MCNativeCharFold(self -> native_chars[i]) == t_from)
				self -> native_chars[i] = p_replacement;
	}
    
	return true;
}

bool MCStringFindAndReplaceNative(MCStringRef self, MCStringRef p_pattern, MCStringRef p_replacement, MCStringOptions p_options)
{
    // Ensure the string is not indirect.
    if (__MCStringIsIndirect(self))
        if (!__MCStringResolveIndirect(self))
            return false;
    
    if (__MCStringIsIndirect(p_replacement))
        p_replacement = p_replacement -> string;
    
    if (__MCStringIsIndirect(p_pattern))
        p_pattern = p_pattern -> string;
    
	if (p_pattern -> char_count == 1 && p_replacement -> char_count == 1)
		return MCStringFindAndReplaceChar(self, p_pattern -> native_chars[0], p_replacement -> native_chars[0], p_options);
    
	if (self -> char_count != 0)
	{
		char_t *t_output;
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
            // AL-2014-05-23: [[ Bug 12482 ]] Pass through string options themselves, rather than a bool.
			t_found = MCStringFirstIndexOf(self, p_pattern, t_offset, p_options, t_next_offset);
            
			// If we found an instance of from, then we need space for to; otherwise,
			// we update the offset, and need just room up to it.
			uindex_t t_space_needed;
			if (t_found)
				t_space_needed = (t_next_offset - t_offset) + p_replacement -> char_count;
			else
			{
				t_next_offset = self -> char_count;
				t_space_needed = t_next_offset - t_offset;
			}
            
			// Expand the buffer as necessary.
            // MW-2015-05-26: [[ Bug 15352 ]] Allocate more memory
			if (t_output_length + t_space_needed + 1 > t_output_capacity)
			{
				if (t_output_capacity == 0)
					t_output_capacity = 4096;
                
				while(t_output_length + t_space_needed + 1 > t_output_capacity)
					t_output_capacity *= 2;
                
				if (!MCMemoryReallocate(t_output, t_output_capacity, t_output))
				{
					MCMemoryDeallocate(t_output);
					return false;
				}
			}
			// Copy in self, up to the offset.
			memcpy(t_output + t_output_length, self -> native_chars + t_offset, t_next_offset - t_offset);
			t_output_length += t_next_offset - t_offset;
            
			// No more occurences were found, so we are done.
			if (!t_found)
				break;
            
			// Now copy in replacement.
			memcpy(t_output + t_output_length, p_replacement -> native_chars, p_replacement -> char_count);
			t_output_length += p_replacement -> char_count;
            
			// Update offset
			t_offset = t_next_offset + p_pattern -> char_count;	
		}
        
		// Add the implicit NUL
		t_output[t_output_length] = '\0';
        
		MCMemoryDeleteArray(self -> native_chars);
        
		self -> native_chars = t_output;
		self -> char_count = t_output_length;
		self -> capacity = t_output_capacity;
	}
    
    __MCStringChanged(self, true, true, true);
	return true;
}

////////////////////////////////////////////////////////////////////////////////

static void split_find_end_of_element(const void *sptr, uindex_t length, bool native, const void* p_del, uindex_t p_del_length, bool p_del_native, MCStringOptions p_options, uindex_t& r_end_offset, uindex_t& r_found_length)
{
	bool t_found;
    MCRange t_found_range;
    t_found = MCUnicodeFind(sptr, length, native, p_del, p_del_length, p_del_native, (MCUnicodeCompareOption)p_options, t_found_range);

    if (!t_found)
    {
        r_end_offset = length;
        return;
    }
    
    r_end_offset = t_found_range . offset;
    r_found_length = t_found_range . length;
}

static void split_find_end_of_element_and_key(const void *sptr, uindex_t length, bool native, const void *p_del, uindex_t p_del_length, bool p_del_native, const void *p_key, uindex_t p_key_length, bool p_key_native, MCStringOptions p_options, uindex_t& r_key_end, uindex_t& r_element_end, uindex_t& r_del_found_length, uindex_t& r_key_found_length)
{
	// Not as fast as it could be...
	bool t_key_found, t_del_found;
    MCRange t_key_found_range, t_del_found_range;
    
    t_del_found = MCUnicodeFind(sptr, length, native, p_del, p_del_length, p_del_native, (MCUnicodeCompareOption)p_options, t_del_found_range);
    // SN-2014-07-29: [[ Bug 13018 ]] Use t_key_found_range for the key, not t_del_found_range
    t_key_found = MCUnicodeFind(sptr, length, native, p_key, p_key_length, p_key_native, (MCUnicodeCompareOption)p_options, t_key_found_range);
    
    if (!t_key_found)
        r_key_end = length;
    
    if (!t_del_found)
        r_element_end = length;
    
    if (t_key_found_range . offset > t_del_found_range . offset)
    {
        // Delimiter came before the key
        r_key_end = r_element_end = length;
        return;
    }
    
    r_key_end = t_key_found_range . offset;
    r_key_found_length = t_key_found_range . length;
    split_find_end_of_element(sptr, length, native, p_del, p_del_length, p_del_native, p_options, r_element_end, r_del_found_length);
}

bool MCStringSplit(MCStringRef self, MCStringRef p_elem_del, MCStringRef p_key_del, MCStringOptions p_options, MCArrayRef& r_array)
{
    if (__MCStringIsIndirect(self))
        self = self -> string;
    
    // SN-2014-03-24: [[ SplitWithStrings ]] No longer checks whether the delimiter is actually 1-char long.
	if (self -> char_count == 0)
	{
		r_array = MCValueRetain(kMCEmptyArray);
		return true;
	}
    
    if (__MCStringIsNative(self))
    {
        if (MCStringIsNative(p_elem_del) && (p_key_del == nil || MCStringIsNative(p_key_del)))
            return MCStringSplitNative(self, p_elem_del, p_key_del, p_options, r_array);
    }

    MCAutoArrayRef t_array;
	if (!MCArrayCreateMutable(&t_array))
		return false;
    
    if (__MCStringIsIndirect(p_elem_del))
        p_elem_del = p_elem_del -> string;
    
	const void *t_echar, *t_kchar;
    bool del_native, key_native;
    del_native = __MCStringIsNative(p_elem_del);
	t_echar = p_elem_del -> chars;

	if (p_key_del != nil)
    {
        if (__MCStringIsIndirect(p_key_del))
            p_key_del = p_key_del -> string;
        
        key_native = __MCStringIsNative(p_key_del);
		t_kchar = p_key_del -> chars;
    }

	const void *t_sptr;
    bool self_native = __MCStringIsNative(self);

    uindex_t t_del_length = __MCStringGetLength(p_elem_del);
    
    if (self_native)
        t_sptr = self -> native_chars;
    else
        t_sptr = self -> chars;
    
    uindex_t t_offset, t_to_end;
    t_to_end = self -> char_count;
    t_offset = 0;
	if (p_key_del == nil)
	{
		uindex_t t_index;
		t_index = 1;
		for(;;)
		{
            uindex_t t_found_del_length, t_end_offset;
            
            split_find_end_of_element(t_sptr, t_to_end, self_native, t_echar, t_del_length, del_native, p_options, t_end_offset, t_found_del_length);
			
			MCAutoStringRef t_string;
			if (!MCStringCopySubstring(self, MCRangeMake(t_offset, t_end_offset), &t_string))
				return false;

			if (!MCArrayStoreValueAtIndex(*t_array, t_index, *t_string))
				return false; 

			if (t_end_offset + t_found_del_length >= t_to_end)
				break;

			t_index += 1;
            
            t_offset += t_end_offset + t_found_del_length;
			t_sptr = (const char *)t_sptr + (self_native ? t_end_offset + t_found_del_length : 2 * (t_end_offset + t_found_del_length));
            t_to_end -= (t_end_offset + t_found_del_length);
		}
	}
	else
	{
        uindex_t t_key_length = __MCStringGetLength(p_key_del);
		for(;;)
		{
            uindex_t t_found_del_length, t_found_key_length, t_key_end, t_element_end;

            split_find_end_of_element_and_key(t_sptr, t_to_end, self_native, t_echar, t_del_length, del_native, t_kchar, t_key_length, key_native, p_options, t_key_end, t_element_end, t_found_del_length, t_found_key_length);
			
			MCAutoStringRef t_key_string;
			if (!MCStringCopySubstring(self, MCRangeMake(t_offset, t_key_end), &t_key_string))
				return false;
            
            MCNewAutoNameRef t_key_name;
            if (!MCNameCreate(*t_key_string, &t_key_name))
                return false;

			if (t_key_end != t_element_end)
				t_key_end += t_found_key_length;

			MCAutoStringRef t_string;
			if (!MCStringCopySubstring(self, MCRangeMake(t_offset + t_key_end, t_element_end - t_key_end), &t_string))
				return false;

			if (!MCArrayStoreValue(*t_array, true, *t_key_name, *t_string))
				return false;

			if (t_element_end + t_found_del_length >= t_to_end)
				break;

            t_offset += t_element_end + t_found_del_length;
			t_sptr = (const char *)t_sptr + (self_native ? t_element_end + t_found_del_length : 2 * (t_element_end + t_found_del_length));
            t_to_end -= (t_element_end + t_found_del_length);
		}
	}

	if (!MCArrayCopy(*t_array, r_array))
		return false;

	return true;
}

bool MCStringFindAndReplaceChar(MCStringRef self, codepoint_t p_pattern, codepoint_t p_replacement, MCStringOptions p_options)
{
    // Ensure the string is not indirect.
    if (__MCStringIsIndirect(self))
        if (!__MCStringResolveIndirect(self))
            return false;
    
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
        // Either character is outside the BMP
        (p_pattern > 0xFFFF || p_replacement > 0xFFFF)
     
        // Normalisation or case-folding has been requested
        || (p_options != kMCStringOptionCompareExact)
    )
    {
        // Do it via the slow-path full string replacement
        MCAutoStringRef t_pattern, t_replacement;
        unichar_t t_buffer[2];
        /* UNCHECKED */ MCStringCreateWithChars(t_buffer, MCStringCodepointToSurrogates(p_pattern, t_buffer), &t_pattern);
        /* UNCHECKED */ MCStringCreateWithChars(t_buffer, MCStringCodepointToSurrogates(p_replacement, t_buffer), &t_replacement);
        return MCStringFindAndReplace(self, *t_pattern, *t_replacement, p_options);
    }
    
    bool t_native = __MCStringIsNative(self);
    // The options must be kMCStringOptionCompareExact
    for (uindex_t i = 0; i < self -> char_count; i++)
    {
        if (t_native)
        {
            if (self -> native_chars[i] == p_pattern)
                self -> native_chars[i] = p_replacement;

        }
        else
        {
            if (self -> chars[i] == p_pattern)
                self -> chars[i] = p_replacement;
        }
    }
    
      return true;
}

bool MCStringFindAndReplace(MCStringRef self, MCStringRef p_pattern, MCStringRef p_replacement, MCStringOptions p_options)
{
    // Ensure the string is not indirect.
    if (__MCStringIsIndirect(self))
        if (!__MCStringResolveIndirect(self))
            return false;
    
    if (__MCStringIsNative(self))
    {
        if (MCStringIsNative(p_pattern))
        {
            if (!MCStringIsNative(p_replacement))
            {
                MCRange t_dummy;
                if (!MCStringFindNative(self, MCRangeMake(0, __MCStringGetLength(self)), p_pattern, p_options, &t_dummy))
                    return true;
            }
            else
                return MCStringFindAndReplaceNative(self, p_pattern, p_replacement, p_options);
        }
        else if (MCStringCantBeEqualToNative(p_pattern, p_options))
            return false;
    }
    
    __MCStringUnnativize(self);

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
			t_found = MCStringFirstIndexOf(self, p_pattern, t_offset, p_options, t_next_offset);
			
			// If we found an instance of from, then we need space for to; otherwise,
			// we update the offset, and need just room up to it.
			uindex_t t_space_needed;
			if (t_found)
				t_space_needed = (t_next_offset - t_offset) + p_replacement -> char_count;
			else
			{
				t_next_offset = self -> char_count;
				t_space_needed = t_next_offset - t_offset;
			}

			// Expand the buffer as necessary.
            // MW-2015-05-26: [[ Bug 15352 ]] Allocate more memory
			if (t_output_length + t_space_needed + 1 > t_output_capacity)
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
            if (MCStringIsNative(p_replacement))
                MCUnicodeCharsMapFromNative(p_replacement -> native_chars, p_replacement -> char_count, t_output + t_output_length);
            else
                memcpy(t_output + t_output_length, p_replacement -> chars, p_replacement -> char_count * sizeof(strchar_t));
			t_output_length += p_replacement -> char_count;

			// Update offset
			t_offset = t_next_offset + MCStringGetLength(p_pattern);
            if (t_offset >= self -> char_count)
                break;
		}
	
		// Add the implicit NUL
		t_output[t_output_length] = '\0';

		MCMemoryDeleteArray(self -> chars);

		self -> chars = t_output;
		self -> char_count = t_output_length;
		self -> capacity = t_output_capacity;
		
		__MCStringChanged(self, false, false, __MCStringCanBeNative(self) && MCStringCanBeNative(p_replacement));
	}
	return true;
}

bool MCStringWildcardMatch(MCStringRef source, MCRange source_range, MCStringRef pattern, MCStringOptions p_options)
{
    bool source_native = MCStringIsNative(source);
    
    const void *source_chars;
    if (source_native)
        source_chars = source -> native_chars + source_range . offset;
    else
        source_chars = source -> chars + source_range . offset;

    return MCUnicodeWildcardMatch(source_chars, source_range . length, source_native, pattern -> chars, pattern -> char_count, MCStringIsNative(pattern), (MCUnicodeCompareOption)p_options);

}

////////////////////////////////////////////////////////////////////////////////

void __MCStringDestroy(__MCString *self)
{
    if (__MCStringIsIndirect(self))
    {
        MCValueRelease(self -> string);
    }
    else
    {
        if (__MCStringIsNative(self))
            MCMemoryDeleteArray(self -> native_chars);
        else
            MCMemoryDeleteArray(self -> chars);
    }
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
    MCAssert(!__MCStringIsIndirect(self));
    
	// Fetch the capacity.
	uindex_t t_capacity;
	t_capacity = __MCStringGetCapacity(self);
    
	// The capacity field stores the total number of chars that could fit not
	// including the implicit NUL, so if we fit, we can fast-track.
	if (t_capacity != 0 && self -> char_count + p_count <= t_capacity)
	{
		// Shift up the chars above - including the implicit NUL.
        if (__MCStringIsNative(self))
            MCMemoryMove(self -> native_chars + p_at + p_count, self -> native_chars + p_at, ((self -> char_count + 1) - p_at));
        else
            MCMemoryMove(self -> chars + p_at + p_count, self -> chars + p_at, ((self -> char_count + 1) - p_at) * sizeof(unichar_t));

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

	// Reallocate and shift up the chars above - including the implicit NUL.
    if (__MCStringIsNative(self))
    {
        if (!MCMemoryReallocate(self -> native_chars, t_new_capacity, self -> native_chars))
            return false;
        
    	MCMemoryMove(self -> native_chars + p_at + p_count, self -> native_chars + p_at, ((self -> char_count + 1) - p_at));
    }
    else
    {
        if (!MCMemoryReallocate(self -> chars, t_new_capacity * sizeof(unichar_t), self -> chars))
            return false;
        
        MCMemoryMove(self -> chars + p_at + p_count, self -> chars + p_at, ((self -> char_count + 1) - p_at) * sizeof(unichar_t));
    }

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
    MCAssert(!__MCStringIsIndirect(self));
    
	// Shift the chars above 'at' down to remove 'count', remembering to include
	// the implicit NUL.
    if (__MCStringIsNative(self))
        MCMemoryMove(self -> native_chars + p_at, self -> native_chars + (p_at + p_count), (self -> char_count - (p_at + p_count) + 1));
    else
        MCMemoryMove(self -> chars + p_at, self -> chars + (p_at + p_count), (self -> char_count - (p_at + p_count) + 1) * sizeof(strchar_t));

	// Now adjust the length of the string.
	self -> char_count -= p_count;

	// TODO: Shrink the buffer if its too big.
}

static uindex_t __MCStringNativize(MCStringRef self)
{
    if (MCStringIsNative(self))
        return self -> char_count;
    
    if (__MCStringIsIndirect(self))
        __MCStringResolveIndirect(self);
    
    bool t_not_native;
    t_not_native = false;
    
    char_t *chars;
    /* UNCHECKED */ MCMemoryNewArray(self -> char_count + 1, chars);
    for(uindex_t i = 0; i < self -> char_count; i++)
        if (!MCUnicodeCharMapToNative(self -> chars[i], chars[i]))
        {
            t_not_native = true;
            break;
        }
    
    if (!t_not_native)
    {
        MCMemoryDeleteArray(self -> chars);
        self -> native_chars = chars;
        __MCStringChanged(self, true, true, true);
        self -> flags &= ~kMCStringFlagIsNotNative;
        self -> native_chars[self -> char_count] = '\0';
        return self -> char_count;
    }
    
    // The string needs to be normalised before conversion to native characters.
    // All the native character sets we support use pre-composed characters.
    MCAutoStringRef t_norm;
    /* UNCHECKED */ MCStringNormalizedCopyNFC(self, &t_norm);
    
    // Nativisation is done on a char (grapheme) basis so we need to know the
    // number of graphemes in the string
    MCRange t_cu_range, t_char_range;
    t_cu_range = MCRangeMake(0, (*t_norm) -> char_count);
    /* UNCHECKED */ MCStringUnmapIndices(*t_norm, kMCCharChunkTypeGrapheme, t_cu_range, t_char_range);
    
    // Create a character break iterator and go through the string
    MCBreakIteratorRef t_breaker;
    /* UNCHECKED */ MCLocaleBreakIteratorCreate(kMCBasicLocale, kMCBreakIteratorTypeCharacter, t_breaker);
    /* UNCHECKED */ MCLocaleBreakIteratorSetText(t_breaker, *t_norm);
    
    uindex_t t_current = 0, t_next;
    bool t_is_native = true;
    for (uindex_t i = 0; i < t_char_range . length; i++)
    {
        // If we've reached the end, set the next boundary manually
        t_next = MCLocaleBreakIteratorAdvance(t_breaker);
        if (t_next == 0)
            t_next = self -> char_count;
        
        // All nativisable characters are 1 codeunit in length. We do, however,
        // need a special case for CRLF sequences (Unicode defines them as being
        // a single grapheme but we want them to produce 2 bytes).
        //
        // This should go away when we teach ICU about our own breaking rules.
        if (t_next != t_current + 1
            || !MCUnicodeCharMapToNative((*t_norm) -> chars[t_current], chars[i]))
        {
            if (t_next == t_current + 2
                && (*t_norm) -> chars[t_current] == '\r'
                && (*t_norm) -> chars[t_current + 1] == '\n')
            {
                // Need to resize the output array :-(
                /* UNCHECKED */ MCMemoryReallocate(chars, ++t_char_range . length + 1, chars);
                chars[i] = '\r';
                chars[++i] = '\n';
            }
            else
            {
                t_is_native = false;
                chars[i] = '?';
            }
        }
        
        // Advance
        t_current = t_next;
    }

    MCLocaleBreakIteratorRelease(t_breaker);
    
    MCMemoryDeleteArray(self -> chars);
    self -> native_chars = chars;
	self -> native_chars[t_char_range.length] = '\0';
    
    __MCStringChanged(self, true, true, true);
    self -> flags &= ~kMCStringFlagIsNotNative;
    self -> char_count = t_char_range . length;

    return t_char_range . length;
}

static void __MCStringUnnativize(MCStringRef self)
{    
    if (!MCStringIsNative(self))
        return;

    if (__MCStringIsIndirect(self))
        __MCStringResolveIndirect(self);
    
    bool t_success;
    t_success = true;
    
    uindex_t t_capacity = self -> capacity != 0 ? self -> capacity : self -> char_count;
    uindex_t t_char_count = self -> char_count;

    unichar_t *chars;
    
    if (t_success)
        t_success = MCMemoryNewArray(t_capacity + 1, chars);
    
	if (t_success)
	{
		MCStrCharsMapFromNative(chars, self -> native_chars, t_char_count);
        MCMemoryDeleteArray(self -> native_chars);
        self -> chars = chars;
		self -> char_count = t_char_count;
        // Set the NUL char.
        self -> chars[t_char_count] = '\0';
    }

    __MCStringChanged(self, true, true);
    self -> flags |= kMCStringFlagIsNotNative;
    self -> flags |= kMCStringFlagCanBeNative;
}

static void __MCStringChanged(MCStringRef self, uindex_t simple, uindex_t uncombined, uindex_t native)
{
    MCAssert(!__MCStringIsIndirect(self));
    
	// String changed to assume that it is no longer simple
    if (simple == kMCStringFlagSetTrue)
        self -> flags |=  kMCStringFlagIsSimple;
    else if (simple == kMCStringFlagSetFalse)
        self -> flags &= ~kMCStringFlagIsSimple;
    
    if (uncombined == kMCStringFlagSetTrue)
        self -> flags |= kMCStringFlagIsUncombined;
    else if (uncombined == kMCStringFlagSetFalse)
        self -> flags &= ~kMCStringFlagIsUncombined;

    self -> flags &= ~kMCStringFlagIsChecked;
    self -> flags &= ~kMCStringFlagHasNumber;
    
    if (native == kMCStringFlagSetFalse)
    {
        self -> flags &= ~kMCStringFlagCanBeNative;
        self -> flags |= kMCStringFlagIsNotNative;
    }
    else if (native == kMCStringFlagSetTrue)
        self -> flags |= kMCStringFlagCanBeNative;
}

codepoint_t MCStringSurrogatesToCodepoint(unichar_t p_lead, unichar_t p_trail)
{
    return 0x10000 + ((p_lead & 0x3FF) << 10) + (p_trail & 0x3FF);
}

unsigned int MCStringCodepointToSurrogates(codepoint_t p_codepoint, unichar_t (&r_units)[2])
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

bool MCStringIsValidSurrogatePair(MCStringRef self, uindex_t p_index)
{
    if (__MCStringIsIndirect(self))
        self = self -> string;
    
    if (__MCStringIsNative(self))
        return false;
    
    // Check that the string is long enough
    // (Double-checking here is due to possible unsigned wrapping)
    if (p_index >= self -> char_count || (p_index + 1) >= self -> char_count)
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
MCStringRef kMCCommaString;
MCStringRef kMCLineEndString;
MCStringRef kMCTabString;

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
    
    if (!MCStringCreateWithNativeChars((const char_t *)",", 1, kMCCommaString))
		return false;
    
	if (!MCStringCreateWithNativeChars((const char_t *)"\n", 1, kMCLineEndString))
		return false;
    
    if (!MCStringCreateWithNativeChars((const char_t *)"\t", 1, kMCTabString))
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
    
    MCValueRelease(kMCCommaString);
    kMCCommaString = nil;
    MCValueRelease(kMCLineEndString);
    kMCLineEndString = nil;
    MCValueRelease(kMCTabString);
    kMCTabString = nil;
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
	size_t t_alloc_remain = 0;
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

bool MCStringConvertToSysString(MCStringRef p_string, char *& r_system_string, size_t& r_byte_count)
{
    // Create the pseudo-FD that iconv uses for character conversion. For
    // efficiency, convert straight from the internal format.
	iconv_t t_fd;
	const char *t_mc_string;
	size_t t_mc_len;

    // What is the system character encoding?
    //
    // Doing this here is unpleasant but the MCString*SysString functions are
    // needed before the libfoundation initialise call is made
    if (__MCSysCharset == nil)
    {
        setlocale(LC_CTYPE, "");
        __MCSysCharset = nl_langinfo(CODESET);
    }

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
    r_byte_count = t_sys_len;
    
	return true;
}
#elif defined(__WINDOWS__)
bool MCStringConvertToSysString(MCStringRef p_string, char *& r_system_string, size_t& r_byte_count)
{
    UINT t_codepage;
    t_codepage = GetConsoleOutputCP();
    
    int t_needed;
    t_needed = WideCharToMultiByte(t_codepage,
                                   WC_COMPOSITECHECK | WC_NO_BEST_FIT_CHARS,
                                   MCStringGetCharPtr(p_string),
                                   -1,
                                   NULL,
                                   0,
                                   NULL,
                                   NULL);
    
    MCAutoArray<char> t_bytes;
    if (!t_bytes . New(t_needed))
        return false;
    
    if (WideCharToMultiByte(t_codepage,
                            WC_COMPOSITECHECK | WC_NO_BEST_FIT_CHARS,
                            MCStringGetCharPtr(p_string),
                            -1,
                            t_bytes . Ptr(),
                            t_needed,
                            NULL,
                            NULL) == 0)
        return false;
    
    uindex_t t_size;
    char *t_ptr;
    t_bytes . Take(t_ptr, t_size);
    
    r_system_string = t_ptr;
    
    // Account for the fact that array size includes the NUL byte.
    r_byte_count = t_size - 1;
    
    return true;
}
#else
bool MCStringConvertToSysString(MCStringRef p_string, char *& r_system_string, size_t& r_byte_count)
{
    bool t_success;
    uindex_t t_byte_count;
    if (!MCStringConvertToUTF8(p_string, r_system_string, t_byte_count))
        return false;
    r_byte_count = t_byte_count;
    return true;
}

bool
MCStringCreateWithSysString(const char *p_sys_string,
                            MCStringRef & r_string)
{
	/* Count the number of chars */
	size_t p_byte_count;
	for (p_byte_count = 0; p_sys_string[p_byte_count] != '\0'; ++p_byte_count);

	if (0 == p_byte_count)
	{
		r_string = MCValueRetain(kMCEmptyString);
		return true;
	}

	return MCStringCreateWithBytes((const byte_t *) p_sys_string,
	                               p_byte_count,
	                               kMCStringEncodingUTF8,
	                               false, /* is_external_rep */
	                               r_string);
}
#endif

bool MCStringNormalizedCopyNFC(MCStringRef self, MCStringRef &r_string)
{
    if (MCStringIsNative(self))
        return MCStringCopy(self, r_string);
    
    // Normalise
    unichar_t *t_norm = nil;
    uindex_t t_norm_length;
    if (MCUnicodeNormaliseNFC(self -> chars, self -> char_count, t_norm, t_norm_length)
        && MCStringCreateWithCharsAndRelease(t_norm, t_norm_length, r_string))
        return true;
    MCMemoryDelete(t_norm);
    return false;
}

bool MCStringNormalizedCopyNFD(MCStringRef self, MCStringRef &r_string)
{
    // AL-2014-06-24: [[ Bug 12656 ]] Native strings can be decomposed into non-native ones.
    unichar_t *t_norm = nil;
    uindex_t t_norm_length;
    if (MCUnicodeNormaliseNFD(MCStringGetCharPtr(self), self -> char_count, t_norm, t_norm_length)
        && MCStringCreateWithCharsAndRelease(t_norm, t_norm_length, r_string))
        return true;
    MCMemoryDelete(t_norm);
    return false;
}

bool MCStringNormalizedCopyNFKC(MCStringRef self, MCStringRef &r_string)
{
    // Native strings are already normalized
    if (MCStringIsNative(self))
        return MCStringCopy(self, r_string);
    
    // Normalise
    unichar_t *t_norm = nil;
    uindex_t t_norm_length;
    if (MCUnicodeNormaliseNFKC(self -> chars, self -> char_count, t_norm, t_norm_length)
        && MCStringCreateWithCharsAndRelease(t_norm, t_norm_length, r_string))
        return true;
    MCMemoryDelete(t_norm);
    return false;
}

bool MCStringNormalizedCopyNFKD(MCStringRef self, MCStringRef &r_string)
{
    // AL-2014-06-24: [[ Bug 12656 ]] Native strings can be decomposed into non-native ones.
    // Normalise
    unichar_t *t_norm = nil;
    uindex_t t_norm_length;
    if (MCUnicodeNormaliseNFKD(MCStringGetCharPtr(self), self -> char_count, t_norm, t_norm_length)
        && MCStringCreateWithCharsAndRelease(t_norm, t_norm_length, r_string))
        return true;
    MCMemoryDelete(t_norm);
    return false;
}

/////////

bool MCStringSetNumericValue(MCStringRef self, double p_value)
{
    if (__MCStringIsIndirect(self))
        self = self -> string;
    
    if (MCStringIsMutable(self))
        return false;
    
    // Compute the number of bytes used by the string - including 1 for the
    // implicit NUL.
    uindex_t t_byte_count;
    if (__MCStringIsNative(self))
        t_byte_count = self -> char_count + 1;
    else
        t_byte_count = self -> char_count * 2 + 1;
    
    // Round up the byte count to the nearest 8 bytes.
    t_byte_count = (t_byte_count + 7) & ~7;
    
    if (!MCMemoryReallocate(self -> native_chars, t_byte_count + 8, self -> native_chars))
        return false;
    
    *(double *)(&(self -> native_chars[t_byte_count])) = p_value;
    
    self -> flags |= kMCStringFlagHasNumber;
    
    return true;
}

bool MCStringGetNumericValue(MCStringRef self, double &r_value)
{
    if (__MCStringIsIndirect(self))
        self = self -> string;
    
    if ((self -> flags & kMCStringFlagHasNumber) != 0)
    {
        // Compute the number of bytes used by the string - including 1 for the
        // implicit NUL.
        uindex_t t_byte_count;
        if (__MCStringIsNative(self))
            t_byte_count = self -> char_count + 1;
        else
            t_byte_count = self -> char_count * 2 + 1;
        
        // Round up the byte count to the nearest 8 bytes.
        t_byte_count = (t_byte_count + 7) & ~7;
        
        r_value = *(double *)(&(self -> native_chars[t_byte_count]));
        
        return true;
    }
    else
        return false;
}

////////////////////////////////////////////////////////////////////////////////

static bool __MCStringCreateIndirect(__MCString *string, __MCString*& r_string)
{
    MCStringRef self;
    if (!__MCValueCreate(kMCValueTypeCodeString, self))
        return false;
    
    self -> string = MCValueRetain(string);
    self -> flags |= kMCStringFlagIsIndirect | kMCStringFlagIsMutable;
    
    r_string = self;
    return true;
}

// Returns true if the string is indirect.
static bool __MCStringIsIndirect(__MCString *self)
{
    return (self -> flags & kMCStringFlagIsIndirect) != 0;
}

static bool __MCStringMakeImmutable(__MCString *self)
{
    // Shrink the char buffer to be just long enough for the characters plus
    // an implicit NUL.
    if (MCStringIsNative(self))
    {
        if (!MCMemoryResizeArray(self -> char_count + 1, self -> native_chars, self -> char_count))
            return false;
    }
    else
    {
        if (!MCMemoryResizeArray(self -> char_count + 1, self -> chars, self -> char_count))
            return false;
    }
    
    self -> char_count -= 1;
    return true;
}

// Creates an immutable string from this one, changing 'self' to indirect.
static bool __MCStringMakeIndirect(__MCString *self)
{
    // If we are already indirect, there's nothing to do.
	if (__MCStringIsIndirect(self))
		return true;
    
    if (!__MCStringMakeImmutable(self))
        return false;
    
	// Create a new direct string for self to reference
	MCStringRef t_string;
	if (!__MCValueCreate(kMCValueTypeCodeString, t_string))
		return false;
    
	// Share the buffer and assign flags & count
	t_string -> flags |= self -> flags;
    t_string -> flags &= ~kMCStringFlagIsMutable;
	t_string -> char_count = self -> char_count;
    
    if (__MCStringIsNative(self))
        t_string -> native_chars = self -> native_chars;
    else
    {
        t_string -> chars = self -> chars;
        t_string -> flags |= kMCStringFlagIsNotNative;
        // AL-2015-02-05: [[ Bug 14504 ]] Ensure 'CanBeNative' flag is preserved when making a string indirect
        if (__MCStringCanBeNative(self))
            t_string -> flags |= kMCStringFlagCanBeNative;
    }

	// 'self' now becomes indirect with a reference to the new string.
	self -> flags |= kMCStringFlagIsIndirect;
	self -> string = t_string;
	return true;
}

// Ensures the given mutable but indirect string is direct.
static bool __MCStringResolveIndirect(__MCString *self)
{
    // Make sure we are indirect.
	MCAssert(__MCStringIsIndirect(self));
    
	// Fetch the reference.
	MCStringRef t_string;
	t_string = self -> string;
    
	// If the string only has a single reference, then re-absorb; otherwise
	// copy.
	if (self -> string -> references == 1)
	{
        self -> char_count = t_string -> char_count;
        self -> capacity = t_string -> capacity;
        self -> flags |= t_string -> flags;
        
        if (__MCStringIsNative(t_string))
            self -> native_chars = t_string -> native_chars;
        else
        {
            self -> chars = t_string -> chars;
            self -> flags |= kMCStringFlagIsNotNative;
            // AL-2015-02-05: [[ Bug 14504 ]] Ensure 'CanBeNative' flag is preserved when making resolving an indirect string.
            if (__MCStringCanBeNative(t_string))
                self -> flags |= kMCStringFlagCanBeNative;
        }

		t_string -> char_count = 0;
		t_string -> chars = nil;
        t_string -> native_chars = nil;
        MCValueRelease(t_string);
	}
	else
	{
        // SN-2015-01-13: [[ Bug 14354 ]] We don't want to release the string we reference
        // before being sure that we can clone its buffer.
        unichar_t *t_chars;
        uint32_t t_char_count;
        
        if (__MCStringIsNative(t_string))
        {
            char_t *t_native_chars;
            if (!__MCStringCloneNativeBuffer(t_string, t_native_chars, t_char_count))
                return false;
            
            t_chars = (unichar_t*)t_native_chars;
        }
        else
        {
            unichar_t *t_uni_chars;
            if (!__MCStringCloneBuffer(t_string, t_chars, t_char_count))
                return false;
            
            self -> flags |= kMCStringFlagIsNotNative;
            
            // AL-2015-02-05: [[ Bug 14504 ]] Ensure 'CanBeNative' flag is preserved when making resolving an indirect string.
            if (__MCStringCanBeNative(t_string))
                self -> flags |= kMCStringFlagCanBeNative;
        }
        
        // SN-2015-01-13: [[ Bug 14354 ]] We can release now release the string,
        // and then change the value of the string attributes
        MCValueRelease(self -> string);
        
        self -> chars = t_chars;
        self -> char_count = t_char_count;

        self -> capacity = t_string -> char_count;
	}
    
	self -> flags &= ~kMCStringFlagIsIndirect;
    
	return true;
}

static bool __MCStringCopyMutable(__MCString *self, __MCString*& r_new_string)
{
    if (!__MCStringMakeImmutable(self))
        return false;
    
    __MCString *t_string;
	t_string = nil;
	
    if (self -> char_count == 0)
    {
        t_string = MCValueRetain(kMCEmptyString);
        MCMemoryDeleteArray(self -> native_chars);
    }
    else
    {
        if (!__MCValueCreate(kMCValueTypeCodeString, t_string))
            return false;
        
        t_string -> char_count = self -> char_count;
        if (__MCStringIsNative(self))
            t_string -> native_chars = self -> native_chars;
        else
        {
            t_string -> chars = self -> chars;
            t_string -> flags |= kMCStringFlagIsNotNative;
            // AL-2015-02-05: [[ Bug 14504 ]] Ensure 'CanBeNative' flag is preserved when making a new direct string.
            if (__MCStringCanBeNative(self))
                t_string -> flags |= kMCStringFlagCanBeNative;
        }
        t_string -> capacity = 0;
    }
    
    self -> char_count = 0;
    self -> chars = nil;
    self -> native_chars = nil;
    self -> string = MCValueRetain(t_string);
    self -> flags |= kMCStringFlagIsIndirect;
    
    r_new_string = t_string;
    return true;
}
