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

#include "foundation-private.h"

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
		MCUnicodeCharsMapToNative(p_chars, p_char_count, self -> chars, self -> char_count, '?');

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
		MCMemoryCopy(self -> chars, p_chars, p_char_count);
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

bool MCStringFormatV(MCStringRef& r_string, const char *p_format, va_list p_args)
{
	char_t *t_string;
	uindex_t t_size;
	if (!MCNativeCharsFormatV(t_string, t_size, p_format, p_args))
		return false;

	if (MCStringCreateWithNativeCharsAndRelease(t_string, t_size, r_string))
		return true;
	MCMemoryDeallocate(t_string);
	return false;
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
	return MCStringCreateWithNativeChars(self -> chars, self -> char_count, r_new_string);
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
	t_success = MCStringCreateWithNativeChars(self -> chars, self -> char_count, r_new_string);
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
	MCMemoryCopy(t_new_string -> chars, self -> chars, self -> char_count + 1);
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

	return MCStringCreateWithNativeChars(self -> chars + p_range . offset, p_range . length, r_substring);
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
	MCMemoryCopy(t_new_string -> chars, self -> chars + p_range . offset, p_range . length);
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
	return nil;
}

const char_t *MCStringGetNativeCharPtr(MCStringRef self)
{
	return (const char_t *)self -> chars;
}

const uint8_t *MCStringGetBytePtr(MCStringRef self)
{
	return (const uint8_t *)self -> chars;
}

unichar_t MCStringGetCharAtIndex(MCStringRef self, uindex_t p_index)
{
	return MCUnicodeCharMapFromNative(self -> chars[p_index]);
}

char_t MCStringGetNativeCharAtIndex(MCStringRef self, uindex_t p_index)
{
	return self -> chars[p_index];
}

uindex_t MCStringGetChars(MCStringRef self, MCRange p_range, unichar_t *p_chars)
{
	uindex_t t_count;
	t_count = 0;

	for(uindex_t i = p_range . offset; i < p_range . offset + p_range . length; i++)
	{
		if (i >= self -> char_count)
			break;

		p_chars[i - p_range . offset] = MCUnicodeCharMapFromNative(self -> chars[i]);

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

		p_chars[i - p_range . offset] = self -> chars[i];

		t_count += 1;
	}

	return t_count;
}

////////////////////////////////////////////////////////////////////////////////

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
	// Allocate an array of chars one byte bigger than needed. As the allocated array
	// is filled with zeros, this will naturally NUL terminate the string.
    uindex_t t_length;
    uindex_t t_byte_count;
    unichar_t* t_unichars;
	t_length = MCStringGetLength(p_string);
    
    if (!MCMemoryNewArray(t_length + 1, t_unichars))
        return false;
    
    uindex_t t_char_count = MCStringGetChars(p_string, MCRangeMake(0, t_length), t_unichars);
    
    t_byte_count = MCUnicodeCharsMapToUTF8(t_unichars, t_char_count, nil);
    
    if (!MCMemoryNewArray(t_byte_count, r_utf8string))
        return false;
    
    MCUnicodeCharsMapToUTF8(t_unichars, t_char_count, (byte_t*)r_utf8string);
    
    // Delete temporary unichar_t array
    MCMemoryDeleteArray(t_unichars);
    
    return true;
}

#if defined(__MAC__) || defined (__IOS__)
bool MCStringConvertToCFStringRef(MCStringRef p_string, CFStringRef& r_cfstring)
{
    uindex_t t_length;
    char_t* t_chars;
    
    t_length = MCStringGetLength(p_string);
    if (!MCMemoryNewArray(t_length + 1, t_chars))
        return false;
    
    MCStringGetNativeChars(p_string, MCRangeMake(0, t_length), t_chars);
    r_cfstring = CFStringCreateWithCharacters(nil, (UniChar *)t_chars, t_length);
    
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
		return MCNativeCharsHashCaseless(self -> chars, self -> char_count);
	return MCNativeCharsHashExact(self -> chars, self -> char_count);
}

bool MCStringIsEqualTo(MCStringRef self, MCStringRef p_other, MCStringOptions p_options)
{
	if (p_options != kMCStringOptionCompareExact)
		return MCNativeCharsEqualCaseless(self -> chars, self -> char_count, p_other -> chars, p_other -> char_count);
	return MCNativeCharsEqualExact(self -> chars, self -> char_count, p_other -> chars, p_other -> char_count);
}

bool MCStringIsEmpty(MCStringRef string)
{
	return MCStringIsEqualTo(string, kMCEmptyString, kMCStringOptionCompareExact);
}

bool MCStringSubstringIsEqualTo(MCStringRef self, MCRange p_sub, MCStringRef p_other, MCStringOptions p_options)
{
	__MCStringClampRange(self, p_sub);

	if (p_options != kMCStringOptionCompareExact)
		return MCNativeCharsEqualCaseless(self -> chars + p_sub . offset, p_sub . length, p_other -> chars, p_other -> char_count);
	return MCNativeCharsEqualExact(self -> chars + p_sub . offset, p_sub . length, p_other -> chars, p_other -> char_count);
}

bool MCStringIsEqualToNativeChars(MCStringRef self, const char_t *p_chars, uindex_t p_char_count, MCStringOptions p_options)
{
	if (p_options != kMCStringOptionCompareExact)
		return MCNativeCharsEqualCaseless(self -> chars, self -> char_count, p_chars, p_char_count);
	return MCNativeCharsEqualExact(self -> chars, self -> char_count, p_chars, p_char_count);
}

compare_t MCStringCompareTo(MCStringRef self, MCStringRef p_other, MCStringOptions p_options)
{
	if (p_options != kMCStringOptionCompareExact)
		return MCNativeCharsCompareCaseless(self -> chars, self -> char_count, p_other -> chars, p_other -> char_count);
	return MCNativeCharsCompareExact(self -> chars, self -> char_count, p_other -> chars, p_other -> char_count);
}

bool MCStringBeginsWith(MCStringRef self, MCStringRef p_prefix, MCStringOptions p_options)
{
	// At first the following approach might seem slightly unusual since we are measuring
	// the length of the common prefix and *then* checking to see if it is the same length
	// as prefix. However, it is important to remember that the length of two equal strings
	// might be different when compared caselessly or non-literally. In particular, the
	// length of the prefix might exceed that of self, but still be equal. e.g.
	//   prefix = e,<acute>
	//   self = e-acute

	// Measure the common prefix of the two strings. Note that these methods return the
	// number of chars in prefix that match those in self (which is what we want).
	uindex_t t_prefix_length;
	if (p_options != kMCStringOptionCompareExact)
		t_prefix_length = MCNativeCharsSharedPrefixCaseless(self -> chars, self -> char_count, p_prefix -> chars, p_prefix -> char_count);
	else
		t_prefix_length = MCNativeCharsSharedPrefixExact(self -> chars, self -> char_count, p_prefix -> chars, p_prefix -> char_count);

	// self begins with prefix iff t_prefix_length == length(prefix).
	return t_prefix_length == p_prefix -> char_count;
}

bool MCStringBeginsWithCString(MCStringRef self, const char_t *p_prefix, MCStringOptions p_options)
{
	uindex_t t_prefix_length;
	if (p_options != kMCStringOptionCompareExact)
		t_prefix_length = MCNativeCharsSharedPrefixCaseless(self -> chars, self -> char_count, p_prefix, strlen((const char *)p_prefix));
	else
		t_prefix_length = MCNativeCharsSharedPrefixExact(self -> chars, self -> char_count, p_prefix, strlen((const char *)p_prefix));

	// self begins with prefix iff t_prefix_length == length(prefix).
	return t_prefix_length == strlen((const char *)p_prefix);
}

bool MCStringEndsWith(MCStringRef self, MCStringRef p_suffix, MCStringOptions p_options)
{
	// Measure the common suffix of the two strings.
	uindex_t t_suffix_length;
	if (p_options != kMCStringOptionCompareExact)
		t_suffix_length = MCNativeCharsSharedSuffixCaseless(self -> chars, self -> char_count, p_suffix -> chars, p_suffix -> char_count);
	else
		t_suffix_length = MCNativeCharsSharedSuffixExact(self -> chars, self -> char_count, p_suffix -> chars, p_suffix -> char_count);

	// self ends with suffix iff t_suffix_length = length(suffix).
	return t_suffix_length == p_suffix -> char_count;
}

bool MCStringEndsWithCString(MCStringRef self, const char_t *p_suffix_cstring, MCStringOptions p_options)
{
	// Measure the common suffix of the two strings.
	uindex_t t_suffix_length;
	if (p_options != kMCStringOptionCompareExact)
		t_suffix_length = MCNativeCharsSharedSuffixCaseless(self -> chars, self -> char_count, p_suffix_cstring, strlen((const char *)p_suffix_cstring));
	else
		t_suffix_length = MCNativeCharsSharedSuffixExact(self -> chars, self -> char_count, p_suffix_cstring, strlen((const char *)p_suffix_cstring));

	// self ends with suffix iff t_suffix_length = length(suffix).
	return t_suffix_length == strlen((const char *)p_suffix_cstring);
}

bool MCStringContains(MCStringRef self, MCStringRef p_needle, MCStringOptions p_options)
{
	// Loop through self starting at each char in turn until we find a common prefix of
	// sufficient length.
	for(uindex_t t_offset = 0; t_offset < self -> char_count; t_offset += 1)
	{
		uindex_t t_prefix_length;
		if (p_options != kMCStringOptionCompareExact)
			t_prefix_length = MCNativeCharsSharedPrefixCaseless(self -> chars + t_offset, self -> char_count - t_offset, p_needle -> chars, p_needle -> char_count);
		else
			t_prefix_length = MCNativeCharsSharedPrefixExact(self -> chars + t_offset, self -> char_count - t_offset, p_needle -> chars, p_needle -> char_count);

		// If the prefix length is the same as needle, we are done.
		if (t_prefix_length == p_needle -> char_count)
			return true;
	}

	// If we get here we've exhausted the string so are done.
	return false;
}

bool MCStringSubstringContains(MCStringRef self, MCRange p_range, MCStringRef p_needle, MCStringOptions p_options)
{
	__MCStringClampRange(self, p_range);

	if (p_needle->char_count > p_range . length)
		return false;

	// Loop through self starting at each char in turn until we find a common prefix of
	// sufficient length.
	uindex_t t_end = p_range . offset + p_range . length;
	uindex_t t_max = t_end - p_needle->char_count;
	for(uindex_t t_offset = p_range . offset; t_offset <= t_max; t_offset += 1)
	{
		uindex_t t_prefix_length;
		if (p_options != kMCStringOptionCompareExact)
			t_prefix_length = MCNativeCharsSharedPrefixCaseless(self -> chars + t_offset, t_end - t_offset, p_needle -> chars, p_needle -> char_count);
		else
			t_prefix_length = MCNativeCharsSharedPrefixExact(self -> chars + t_offset, t_end - t_offset, p_needle -> chars, p_needle -> char_count);

		// If the prefix length is the same as needle, we are done.
		if (t_prefix_length == p_needle -> char_count)
			return true;
	}

	// If we get here we've exhausted the string so are done.
	return false;
}
////////////////////////////////////////////////////////////////////////////////

bool MCStringFirstIndexOf(MCStringRef self, MCStringRef p_needle, uindex_t p_after, MCStringOptions p_options, uindex_t& r_offset)
{
	// Make sure the after index is in range.
	p_after = MCMin(p_after, self -> char_count);

	// Similar to contains, we loop through checking for a shared prefix of the
	// length of needle - notice that we start at 'after', though.
	for(uindex_t t_offset = p_after; t_offset < self -> char_count; t_offset += 1)
	{
		uindex_t t_prefix_length;
		if (p_options != kMCStringOptionCompareExact)
			t_prefix_length = MCNativeCharsSharedPrefixCaseless(self -> chars + t_offset, self -> char_count - t_offset, p_needle -> chars, p_needle -> char_count);
		else
			t_prefix_length = MCNativeCharsSharedPrefixExact(self -> chars + t_offset, self -> char_count - t_offset, p_needle -> chars, p_needle -> char_count);

		// If the prefix length is the same as needle, we are done.
		if (t_prefix_length == p_needle -> char_count)
		{
			r_offset = t_offset;
			return true;
		}
	}

	// If we get here we've exhausted the string so we are done.
	return false;
}

bool MCStringFirstIndexOfChar(MCStringRef self, codepoint_t p_needle, uindex_t p_after, MCStringOptions p_options, uindex_t& r_offset)
{
	// We only support ASCII for now.
	MCAssert(p_needle < 128);

	// Make sure the after index is in range.
	p_after = MCMin(p_after, self -> char_count);

	char_t t_char;
	t_char = (char_t)p_needle;
	if (p_options != kMCStringOptionCompareExact)
		t_char = MCNativeCharFold(t_char);

	for(uindex_t t_offset = p_after; t_offset < self -> char_count; t_offset += 1)
	{
		char_t t_other_char;
		t_other_char = self -> chars[t_offset];
		if (p_options != kMCStringOptionCompareExact)
			t_other_char = MCNativeCharFold(t_other_char);
	
		if (t_other_char == t_char)
		{
			r_offset = t_offset;
			return true;
		}
	}

	return false;
}

bool MCStringLastIndexOf(MCStringRef self, MCStringRef p_needle, uindex_t p_before, MCStringOptions p_options, uindex_t& r_offset)
{
	// We loop backwards checking for shared prefix of the length of needle as
	// we go. Notice that t_offset here tracks the end of char (rather than the
	// start).

	// Make sure the before index is in range.
	p_before = MCMin(p_before, self -> char_count);

	for(uindex_t t_offset = p_before; t_offset > 0; t_offset -= 1)
	{
		// Compute the length of the shared prefix *before* offset - this means
		// we adjust offset down by one before comparing.
		uindex_t t_prefix_length;
		if (p_options != kMCStringOptionCompareExact)
			t_prefix_length = MCNativeCharsSharedPrefixCaseless(self -> chars + (t_offset - 1), self -> char_count - (t_offset - 1), p_needle -> chars, p_needle -> char_count);
		else
			t_prefix_length = MCNativeCharsSharedPrefixExact(self -> chars + (t_offset - 1), self -> char_count - (t_offset - 1), p_needle -> chars, p_needle -> char_count);

		// If the prefix length is the same as the needle then we are done.
		if (t_prefix_length == p_needle -> char_count)
		{
			r_offset = t_offset - 1;
			return true;
		}
	}

	// If we get here we've exhausted the string so we are done.
	return false;
}

bool MCStringLastIndexOfChar(MCStringRef self, codepoint_t p_needle, uindex_t p_before, MCStringOptions p_options, uindex_t& r_offset)
{
	// We only support ASCII for now.
	MCAssert(p_needle < 128);

	// Make sure the before index is in range.
	p_before = MCMin(p_before, self -> char_count);

	char_t t_char;
	t_char = (char_t)p_needle;
	if (p_options != kMCStringOptionCompareExact)
		t_char = MCNativeCharFold(t_char);

	for(uindex_t t_offset = p_before; t_offset > 0; t_offset -= 1)
	{
		char_t t_other_char;
		t_other_char = self -> chars[t_offset - 1];
		if (p_options != kMCStringOptionCompareExact)
			t_other_char = MCNativeCharFold(t_other_char);
	
		if (t_other_char == t_char)
		{
			r_offset = t_offset - 1;
			return true;
		}
	}

	return false;
}

bool MCStringFind(MCStringRef self, MCRange p_range, MCStringRef p_needle, MCStringOptions p_options, MCRange *r_result)
{
	// Similar to contains, this searches for needle but only with range of self.
	// It also returns the the range in self that needle occupies (but only if
	// r_result is non-nil).

	// Compute the char ptr and length based on range.
	const char_t *t_chars;
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
			t_prefix_length = MCNativeCharsSharedPrefixCaseless(t_chars + t_offset, t_char_count - t_offset, p_needle -> chars, p_needle -> char_count);
		else
			t_prefix_length = MCNativeCharsSharedPrefixExact(t_chars + t_offset, t_char_count - t_offset, p_needle -> chars, p_needle -> char_count);

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
					t_prefix_length = MCNativeCharsSharedPrefixCaseless(p_needle -> chars, p_needle -> char_count, t_chars + t_offset, t_char_count - t_offset);
				else
					t_prefix_length = MCNativeCharsSharedPrefixExact(p_needle -> chars, p_needle -> char_count, t_chars + t_offset, t_char_count - t_offset);

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

static uindex_t MCStringCountNativeChars(MCStringRef self, MCRange p_range, const char_t *p_needle_chars, uindex_t p_needle_char_count, MCStringOptions p_options)
{
	// Keep track of how many occurances have been found.
	uindex_t t_count;
	t_count = 0;

	// Compute the char ptr and length based on range.
	const char_t *t_chars;
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

	// Return the number of occurances.
	return t_count;
}

uindex_t MCStringCount(MCStringRef self, MCRange p_range, MCStringRef p_needle, MCStringOptions p_options)
{
	return MCStringCountNativeChars(self, p_range, p_needle -> chars, p_needle -> char_count, p_options);
}

uindex_t MCStringCountChar(MCStringRef self, MCRange p_range, codepoint_t p_needle, MCStringOptions p_options)
{
	// We only support ASCII for now.
	MCAssert(p_needle < 128);
	
	char_t t_native_needle;
	t_native_needle = (char_t)p_needle;
	
	return MCStringCountNativeChars(self, p_range, &t_native_needle, 1, p_options);
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

bool MCStringFold(MCStringRef self, MCStringOptions p_options)
{
	MCAssert(MCStringIsMutable(self));

	// If we are looking for exact comparison then folding has no effect.
	if (p_options == kMCStringOptionCompareExact)
		return true;

	// Otherwise, we just lowercase all the chars in the string (when we move to
	// unicode this gets significantly more complicated!).
	MCNativeCharsLowercase(self -> chars, self -> char_count);

	// We always succeed (at the moment)
	return true;
}

bool MCStringLowercase(MCStringRef self)
{
	MCAssert(MCStringIsMutable(self));

	// Otherwise, we just lowercase all the chars in the string (when we move to
	// unicode this gets significantly more complicated!).
	MCNativeCharsLowercase(self -> chars, self -> char_count);

	// We always succeed (at the moment)
	return true;
}

bool MCStringUppercase(MCStringRef self)
{
	MCAssert(MCStringIsMutable(self));

	// Otherwise, we just lowercase all the chars in the string (when we move to
	// unicode this gets significantly more complicated!).
	MCNativeCharsUppercase(self -> chars, self -> char_count);

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
		MCMemoryCopy(self -> chars + self -> char_count - p_suffix -> char_count, p_suffix -> chars, p_suffix -> char_count + 1);

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
		MCMemoryCopy(self -> chars + self -> char_count - p_range . length, p_suffix -> chars + p_range . offset, p_range . length);

		// Set the NULL
		self -> chars[ self -> char_count ] = '\0';

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
	MCMemoryCopy(self -> chars + self -> char_count - p_char_count, p_chars, p_char_count);

	// Set the NULL
	self -> chars[ self -> char_count ] = '\0';

	// We succeeded.
	return true;
}

bool MCStringAppendChars(MCStringRef self, const unichar_t *p_chars, uindex_t p_char_count)
{
	MCAssert(MCStringIsMutable(self));

	// Ensure we have enough room in self - with the gap at the end.
	if (!__MCStringExpandAt(self, self -> char_count, p_char_count))
		return false;

	// Now copy the chars across (including the NUL).
	uindex_t t_nchar_count;
	/* FRAGILE */ MCUnicodeCharsMapToNative(p_chars, p_char_count, self -> chars + self -> char_count - p_char_count, t_nchar_count, '?');

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
		MCMemoryCopy(self -> chars, p_prefix -> chars, p_prefix -> char_count);

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

	// Only do the append now if self != prefix.
	if (self != p_prefix)
	{
		__MCStringClampRange(p_prefix, p_range);

		// Ensure we have enough room in self - with the gap at the beginning.
		if (!__MCStringExpandAt(self, 0, p_range . length))
			return false;

		// Now copy the chars across.
		MCMemoryCopy(self -> chars, p_prefix -> chars + p_range . offset, p_range . length);

		// We succeeded.
		return true;
	}

	// Otherwise copy substring and append.
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
	MCMemoryCopy(self -> chars, p_chars, p_char_count);

	// We succeeded.
	return true;
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
		MCMemoryCopy(self -> chars + p_at, p_substring -> chars, p_substring -> char_count);

		// We succeeded.
		return true;
	}

	// Otherwise copy and recurse.
	MCAutoStringRef t_substring_copy;
	MCStringCopy(p_substring, &t_substring_copy);
	return MCStringInsert(self, p_at, *t_substring_copy);
}

bool MCStringRemove(MCStringRef self, MCRange p_range)
{
	MCAssert(MCStringIsMutable(self));

	__MCStringClampRange(self, p_range);

	// Copy down the chars above the string taking into account the implicit
	// NUL.
	__MCStringShrinkAt(self, p_range . offset, p_range . length);

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
		MCMemoryCopy(self -> chars + p_range . offset, p_replacement -> chars, p_replacement -> char_count);

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
			MCMemoryCopy(self -> chars + p_at + i * p_value -> char_count, p_value -> chars, p_value -> char_count);

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

static void split_find_end_of_element_exact(const char_t *sptr, const char_t *eptr, char_t del, const char_t*& r_end_ptr)
{
	while(sptr < eptr)
	{
		if (*sptr == del)
		{
			r_end_ptr = sptr;
			return;
		}

		sptr += 1;
	}
	r_end_ptr = eptr;
}

static void split_find_end_of_element_caseless(const char_t *sptr, const char_t *eptr, char_t del, const char_t*& r_end_ptr)
{
	while(sptr < eptr)
	{
		if (MCNativeCharFold(*sptr) == MCNativeCharFold(del))
		{
			r_end_ptr = sptr;
			return;
		}

		sptr += 1;
	}
	r_end_ptr = eptr;
}

static void split_find_end_of_element_and_key_exact(const char_t *sptr, const char_t *eptr, char_t del, char_t key, const char_t*& r_key_ptr, const char_t *& r_end_ptr)
{
	while(sptr < eptr)
	{
		if (*sptr == key)
		{
			r_key_ptr = sptr;
			break;
		}
		if (*sptr == del)
		{
			r_key_ptr = r_end_ptr = sptr;
			return;
		}

		sptr += 1;
	}

	return split_find_end_of_element_exact(sptr, eptr, del, r_end_ptr);
}

static void split_find_end_of_element_and_key_caseless(const char_t *sptr, const char_t *eptr, char_t del, char_t key, const char_t*& r_key_ptr, const char_t *& r_end_ptr)
{
	while(sptr < eptr)
	{
		if (MCNativeCharFold(*sptr) == MCNativeCharFold(key))
		{
			r_key_ptr = sptr;
			break;
		}
		if (MCNativeCharFold(*sptr) == MCNativeCharFold(del))
		{
			r_key_ptr = r_end_ptr = sptr;
			return;
		}

		sptr += 1;
	}

	return split_find_end_of_element_caseless(sptr, eptr, del, r_end_ptr);
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

	char_t t_echar, t_kchar;
	t_echar = p_elem_del -> chars[0];
	if (p_key_del != nil)
		t_kchar = p_key_del -> chars[0];

	const char_t *t_sptr;
	const char_t *t_eptr;
	t_sptr = self -> chars;
	t_eptr = self -> chars + self -> char_count;
	if (p_key_del == nil)
	{
		uindex_t t_index;
		t_index = 1;
		for(;;)
		{
			const char_t *t_element_end;
			if (p_options == kMCStringOptionCompareExact)
				split_find_end_of_element_exact(t_sptr, t_eptr, t_echar, t_element_end);
			else
				split_find_end_of_element_caseless(t_sptr, t_eptr, t_echar, t_element_end);
			
			MCAutoStringRef t_string;
			if (!MCStringCreateWithNativeChars(t_sptr, t_element_end - t_sptr, &t_string))
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
			const char_t *t_element_end;
			const char_t *t_key_end;
			if (p_options == kMCStringOptionCompareExact)
				split_find_end_of_element_and_key_exact(t_sptr, t_eptr, t_echar, t_kchar, t_key_end, t_element_end);
			else
				split_find_end_of_element_and_key_caseless(t_sptr, t_eptr, t_echar, t_kchar, t_key_end, t_element_end);
			
			MCNewAutoNameRef t_name;
			if (!MCNameCreateWithNativeChars(t_sptr, t_key_end - t_sptr, &t_name))
				return false;

			if (t_key_end != t_element_end)
				t_key_end += 1;

			MCAutoStringRef t_string;
			if (!MCStringCreateWithNativeChars(t_key_end, t_element_end - t_key_end, &t_string))
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

bool MCStringFindAndReplaceChar(MCStringRef self, char_t p_pattern, char_t p_replacement, MCStringOptions p_options)
{
	if (p_options == kMCStringOptionCompareExact)
	{
		// Simplest case, just substitute pattern for replacement.
		for(uindex_t i = 0; i < self -> char_count; i++)
			if (self -> chars[i] == p_pattern)
				self -> chars[i] = p_replacement;
	}
	else if (p_options == kMCStringOptionCompareCaseless)
	{
		char_t t_from;
		t_from = MCNativeCharFold(p_pattern);

		// Now substitute pattern for replacement, taking making sure its a caseless compare.
		for(uindex_t i = 0; i < self -> char_count; i++)
			if (MCNativeCharFold(self -> chars[i]) == t_from)
				self -> chars[i] = p_replacement;
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
				
				if (!MCMemoryReallocate(t_output, t_output_capacity, t_output))
				{
					MCMemoryDeallocate(t_output);
					return false;
				}
			}
			// Copy in self, up to the offset.
			memcpy(t_output + t_output_length, self -> chars + t_offset, t_next_offset - t_offset);
			t_output_length += t_next_offset - t_offset;

			// No more occurences were found, so we are done.
			if (!t_found)
				break;
				
			// Now copy in replacement.
			memcpy(t_output + t_output_length, p_replacement -> chars, p_replacement -> char_count);
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
	}

	return true;
}

////////////////////////////////////////////////////////////////////////////////

void __MCStringDestroy(__MCString *self)
{
	MCMemoryDeleteArray(self -> chars);
}

bool __MCStringCopyDescription(__MCString *self, MCStringRef& r_desc)
{
	return MCStringFormat(r_desc, "\"%s\"", self -> chars);
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

static uindex_t *__MCStringGetCapacityPtr(MCStringRef self)
{
	// In mutable strings, the capacity is stored after the chars at the
	// first aligned location.
	uindex_t *t_capacity_ptr;
	t_capacity_ptr = (uindex_t *)(self -> chars + ((self -> char_count + sizeof(uindex_t) - 1) & ~(sizeof(uindex_t) - 1)));
	return t_capacity_ptr;
}

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
		MCMemoryMove(self -> chars + p_at + p_count, self -> chars + p_at, (self -> char_count + 1) - p_at);

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
	if (!MCMemoryReallocate(self -> chars, t_new_capacity, self -> chars))
		return false;

	// Shift up the chars above - including the implicit NUL.
	MCMemoryMove(self -> chars + p_at + p_count, self -> chars + p_at, (self -> char_count + 1) - p_at);

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
	MCMemoryMove(self -> chars + p_at, self -> chars + (p_at + p_count), self -> char_count - (p_at + p_count) + 1);

	// Now adjust the length of the string.
	self -> char_count -= p_count;

	// TODO: Shrink the buffer if its too big.
}

////////////////////////////////////////////////////////////////////////////////

MCStringRef kMCEmptyString;
MCStringRef kMCTrueString;
MCStringRef kMCFalseString;

bool __MCStringInitialize(void)
{
	if (!MCStringCreateWithNativeChars((const char_t *)"", 0, kMCEmptyString))
		return false;

	if (!MCStringCreateWithNativeChars((const char_t *)"true", 4, kMCTrueString))
		return false;

	if (!MCStringCreateWithNativeChars((const char_t *)"false", 5, kMCFalseString))
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
}

////////////////////////////////////////////////////////////////////////////////
