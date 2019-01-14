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
#include <foundation-stdlib.h>

#include <errno.h>

#include "foundation-private.h"

////////////////////////////////////////////////////////////////////////////////

// Forward declarations of internal utility functions

static bool __MCNumberParseNativeString(const char *p_string, uindex_t p_length, bool p_full_string, bool p_integer_only, uindex_t &r_length_used, MCNumberRef &r_number);
static bool __MCNumberParseOffset(MCStringRef p_string, uindex_t offset, uindex_t char_count, bool p_integer_only, MCNumberRef &r_number);
static bool __MCNumberParseUnicodeChars(const unichar_t *p_chars, uindex_t p_char_count, bool p_integer_only, MCNumberRef& r_number);

////////////////////////////////////////////////////////////////////////////////

MC_DLLEXPORT_DEF
bool MCNumberCreateWithInteger(integer_t p_value, MCNumberRef& r_number)
{
	__MCNumber *self;
	if (!__MCValueCreate(kMCValueTypeCodeNumber, self))
		return false;

	self -> integer = p_value;

	r_number = self;

	return true;
}

MC_DLLEXPORT_DEF
bool MCNumberCreateWithReal(real64_t p_value, MCNumberRef& r_number)
{
	__MCNumber *self;
	if (!__MCValueCreate(kMCValueTypeCodeNumber, self))
		return false;

	self -> real = p_value;
	self -> flags |= (1 << 0);

	r_number = self;

	return true;
}

MC_DLLEXPORT_DEF
bool MCNumberCreateWithUnsignedInteger(uinteger_t p_value, MCNumberRef& r_number)
{
    if (p_value <= INTEGER_MAX)
        return MCNumberCreateWithInteger((integer_t)p_value, r_number);
    
    return MCNumberCreateWithReal((real64_t)p_value, r_number);
}

MC_DLLEXPORT_DEF
bool MCNumberIsInteger(MCNumberRef self)
{
	__MCAssertIsNumber(self);
	return (self -> flags & kMCNumberFlagIsReal) == 0;
}

MC_DLLEXPORT_DEF
bool MCNumberIsReal(MCNumberRef self)
{
	__MCAssertIsNumber(self);
	return (self -> flags & kMCNumberFlagIsReal) != 0;
}

MC_DLLEXPORT_DEF
real64_t MCNumberFetchAsReal(MCNumberRef self)
{
	if (MCNumberIsReal(self))
		return self -> real;
	return (real64_t)self -> integer;
}

MC_DLLEXPORT_DEF
integer_t MCNumberFetchAsInteger(MCNumberRef self)
{
	if (MCNumberIsInteger(self))
		return self -> integer;
	return self -> real < 0.0 ? (integer_t)(self -> real - 0.5) : (integer_t)(self -> real + 0.5);
}

MC_DLLEXPORT_DEF
uinteger_t MCNumberFetchAsUnsignedInteger(MCNumberRef self)
{
	if (MCNumberIsInteger(self))
		return self -> integer >= 0 ? self -> integer : 0;
	return self -> real >= 0.0 ? (uinteger_t)(self -> real + 0.5) : (uinteger_t)0.0;
}

MC_DLLEXPORT_DEF
bool MCNumberStrictFetchAsIndex(MCNumberRef self,
                                index_t& r_index)
{
    static_assert(sizeof(index_t) == sizeof(integer_t), "index_t is not the same size as integer_t");
    
    if (MCNumberIsInteger(self))
    {
        r_index = self->integer;
        return true;
    }
    
    index_t t_as_int = (index_t)(self -> real);
    if (self->real - t_as_int != 0.0)
    {
        return false;
    }
    
    r_index = t_as_int;

    return true;
}

compare_t MCNumberCompareTo(MCNumberRef self, MCNumberRef p_other_self)
{
	// First determine the storage types of both numbers.
	bool t_self_is_integer, t_other_self_is_integer;
	t_self_is_integer = MCNumberIsInteger(self);
	t_other_self_is_integer = MCNumberIsInteger(p_other_self);
	
	// If both are stored as integers then compare.
	if (t_self_is_integer && t_other_self_is_integer)
		return self -> integer - p_other_self -> integer;

	// Otherwise fetch both as reals.
	double x, y;
	x = t_self_is_integer ? (double)self -> integer : self -> real;
	y = t_other_self_is_integer ? (double)p_other_self -> integer : p_other_self -> real;

	// TODO: Handle nan / infinity / etc.
		
	if (x < y)
		return -1;
		
	if (x > y)
		return 1;
		
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////

MC_DLLEXPORT_DEF
bool MCNumberParseOffset(MCStringRef p_string, uindex_t offset, uindex_t char_count, MCNumberRef &r_number)
{
    return __MCNumberParseOffset(p_string, offset, char_count, false, r_number);
}

MC_DLLEXPORT_DEF
bool MCNumberParse(MCStringRef p_string, MCNumberRef &r_number)
{
    return __MCNumberParseOffset(p_string, 0, MCStringGetLength(p_string), false, r_number);
}

MC_DLLEXPORT_DEF
bool MCNumberParseInteger(MCStringRef p_string, MCNumberRef &r_number)
{
    return __MCNumberParseOffset(p_string, 0, MCStringGetLength(p_string), true, r_number);
}

MC_DLLEXPORT_DEF
bool MCNumberParseUnicodeChars(const unichar_t *p_chars, uindex_t p_char_count, MCNumberRef& r_number)
{
    return __MCNumberParseUnicodeChars(p_chars, p_char_count, false, r_number);
}

MC_DLLEXPORT_DEF
bool MCNumberParseOffsetPartial(MCStringRef p_string, uindex_t offset, uindex_t &r_chars_used, MCNumberRef &r_number)
{
	bool t_success;
	t_success = true;
	
	char *t_buffer;
	t_buffer = nil;
	
	const char *t_native_string;
	t_native_string = nil;
	
	uindex_t t_length;
	t_length = MCStringGetLength(p_string);
	
	if (offset > t_length)
		offset = t_length;
	
	if (MCStringIsNative(p_string))
		t_native_string = (const char*)MCStringGetNativeCharPtr(p_string) + offset;
	else
	{
		t_success = MCMemoryNewArray(t_length - offset + 1, t_buffer);
		
		uindex_t t_native_char_count;
		if (t_success)
			t_success = MCUnicodeCharsMapToNative(MCStringGetCharPtr(p_string) + offset, t_length - offset, (char_t*)t_buffer, t_native_char_count, '?');

		t_native_string = t_buffer;
	}
	
	if (t_success)
		t_success = __MCNumberParseNativeString(t_native_string, t_length - offset, false, false, r_chars_used, r_number);
	
	MCMemoryDeleteArray(t_buffer);
	
	return t_success;
}

////////////////////////////////////////////////////////////////////////////////

static bool __MCNumberParseNativeString(const char *p_string, uindex_t p_length, bool p_full_string, bool p_integer_only, uindex_t &r_length_used, MCNumberRef &r_number)
{
    bool t_success;
    t_success = true;
    
    MCNumberRef t_number;
    t_number = nil;
    
    uinteger_t t_base;
    t_base = 10;
    
    const char *t_string;
    t_string = p_string;
    
    if (!p_integer_only)
    {
        if (p_length > 2 &&
            p_string[0] == '0' &&
            (p_string[1] == 'x' || p_string[1] == 'X'))
        {
            // If the string begins with 0x then parse as hex, and discard first two chars
            t_base = 16;
            t_string += 2;
        }
    }
    
    errno = 0;
    
    char *t_end;
    t_end  = nil;
    
    integer_t t_integer;
#if defined(__LP64__)
    long t_long;
    t_long = strtol(t_string, &t_end, t_base);
    if (t_long > UINTEGER_MAX)
        errno = ERANGE;
    t_integer = (integer_t)t_long;
#elif defined(__LP32__) || defined(__LLP64__)
    t_integer = strtol(t_string, &t_end, t_base);
#endif
    
    t_success = (errno != ERANGE) && (p_full_string ? (t_end - p_string == (ptrdiff_t)p_length) : (t_end != t_string));
    
    if (!t_success && p_integer_only)
        return false;
    
    if (t_success)
        t_success = MCNumberCreateWithInteger(t_integer, t_number);
    // If parsing as base 10 unsigned integer failed, try to parse as real.
    else if (t_base == 10)
    {
        errno = 0;
        
        real64_t t_real;
        t_real = strtod(p_string, &t_end);
        
        // SN-2014-10-06: [[ Bug 13594 ]] check that no error was encountered
        t_success = (errno != ERANGE) && (p_full_string ? (t_end - p_string == (ptrdiff_t)p_length) : (t_end != t_string));
        if (t_success)
            t_success = MCNumberCreateWithReal(t_real, t_number);
    }
    
    if (t_success)
    {
        r_number = t_number;
        r_length_used = t_end - p_string;
    }
    
    return t_success;
}

static bool __MCNumberParseOffset(MCStringRef p_string, uindex_t offset, uindex_t char_count, bool p_integer_only, MCNumberRef &r_number)
{
    uindex_t length = MCStringGetLength(p_string);
    if (offset > length)
        offset = length;
    
    if (char_count > length - offset)
        char_count = length - offset;
    
    if (!MCStringIsNative(p_string))
        return __MCNumberParseUnicodeChars(MCStringGetCharPtr(p_string) + offset, char_count, p_integer_only, r_number);
    
    bool t_success;
    t_success = false;
    
    uindex_t t_length_used;
    t_length_used = 0;
    
    t_success = __MCNumberParseNativeString((const char*)MCStringGetNativeCharPtr(p_string) + offset, char_count, true, p_integer_only, t_length_used, r_number);
    
    return t_success;
}

static bool __MCNumberParseUnicodeChars(const unichar_t *p_chars, uindex_t p_char_count, bool p_integer_only, MCNumberRef& r_number)
{
    char *t_native_chars;
    if (!MCMemoryNewArray(p_char_count + 1, t_native_chars))
        return false;
    
    uindex_t t_native_char_count;
    MCUnicodeCharsMapToNative(p_chars, p_char_count, (char_t *)t_native_chars, t_native_char_count, '?');
    
    bool t_success;
    t_success = false;
    
    uindex_t t_length_used;
    t_length_used = 0;
    
    t_success = __MCNumberParseNativeString(t_native_chars, p_char_count, true, p_integer_only, t_length_used, r_number);
    
    MCMemoryDeleteArray(t_native_chars);
    
    return t_success;
}

////////////////////////////////////////////////////////////////////////////////////////////////

bool __MCNumberCopyDescription(__MCNumber *self, MCStringRef& r_string)
{
	if (MCNumberIsInteger(self))
		return MCStringFormat(r_string, "%d", self -> integer);
	return MCStringFormat(r_string, "%lf", self -> real);
}

hash_t __MCNumberHash(__MCNumber *self)
{
	if (MCNumberIsInteger(self))
		return MCHashInteger(self -> integer);
	return MCHashDouble(self -> real);
}

bool __MCNumberIsEqualTo(__MCNumber *self, __MCNumber *p_other_self)
{
	return MCNumberCompareTo(self, p_other_self) == 0;
}

////////////////////////////////////////////////////////////////////////////////

MC_DLLEXPORT_DEF MCNumberRef kMCZero;
MC_DLLEXPORT_DEF MCNumberRef kMCOne;
MC_DLLEXPORT_DEF MCNumberRef kMCMinusOne;

bool __MCNumberInitialize(void)
{
    if (!MCNumberCreateWithInteger(0, kMCZero))
        return false;
    
    if (!MCNumberCreateWithInteger(1, kMCOne))
        return false;
		
    if (!MCNumberCreateWithInteger(-1, kMCMinusOne))
        return false;
		
    return true;
}

void __MCNumberFinalize(void)
{
    MCValueRelease(kMCZero);
    MCValueRelease(kMCOne);
    MCValueRelease(kMCMinusOne);
}

////////////////////////////////////////////////////////////////////////////////
