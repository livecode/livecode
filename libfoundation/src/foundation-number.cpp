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
#include <foundation-stdlib.h>

#include <errno.h>

#include "foundation-private.h"

////////////////////////////////////////////////////////////////////////////////

bool MCNumberCreateWithInteger(integer_t p_value, MCNumberRef& r_number)
{
	__MCNumber *self;
	if (!__MCValueCreate(kMCValueTypeCodeNumber, self))
		return false;

	self -> integer = p_value;

	r_number = self;

	return true;
}

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


bool MCNumberCreateWithUnsignedInteger(uinteger_t p_value, MCNumberRef& r_number)
{
    if (p_value <= INTEGER_MAX)
        return MCNumberCreateWithInteger((integer_t)p_value, r_number);
    
    return MCNumberCreateWithReal((real64_t)p_value, r_number);
}

bool MCNumberIsInteger(MCNumberRef self)
{
	return (self -> flags & kMCNumberFlagIsReal) == 0;
}

bool MCNumberIsReal(MCNumberRef self)
{
	return (self -> flags & kMCNumberFlagIsReal) != 0;
}

real64_t MCNumberFetchAsReal(MCNumberRef self)
{
	if (MCNumberIsReal(self))
		return self -> real;
	return (real64_t)self -> integer;
}

integer_t MCNumberFetchAsInteger(MCNumberRef self)
{
	if (MCNumberIsInteger(self))
		return self -> integer;
	return self -> real < 0.0 ? (integer_t)(self -> real - 0.5) : (integer_t)(self -> real + 0.5);
}

uinteger_t MCNumberFetchAsUnsignedInteger(MCNumberRef self)
{
	if (MCNumberIsInteger(self))
		return self -> integer >= 0 ? self -> integer : 0;
	return self -> real >= 0.0 ? (uinteger_t)(self -> real + 0.5) : (uinteger_t)0.0;
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
	y = t_self_is_integer ? (double)p_other_self -> integer : p_other_self -> real;

	// TODO: Handle nan / infinity / etc.
		
	if (x < y)
		return -1;
		
	if (x > y)
		return 1;
		
	return 0;
}

bool MCNumberParseOffset(MCStringRef p_string, uindex_t offset, uindex_t char_count, MCNumberRef &r_number)
{
    uindex_t length = MCStringGetLength(p_string);
    if (offset > length)
        offset = length;
    
    if (char_count > length - offset)
        char_count = length - offset;
    
    if (!MCStringIsNative(p_string))
        return MCNumberParseUnicodeChars(MCStringGetCharPtr(p_string) + offset, MCStringGetLength(p_string), r_number);

    bool t_success;
    t_success = false;

    const char* t_chars = (const char*)MCStringGetNativeCharPtr(p_string) + offset;
    
    if (char_count > 2 &&
            t_chars[0] == '0' &&
            (t_chars[1] == 'x' || t_chars[1] == 'X'))
        t_success = MCNumberCreateWithInteger(strtoul(t_chars + 2, nil, 16), r_number);
    else
    {
        char *t_end;
        // SN-2014-10-06: [[ Bug 13594 ]] We want an unsigned integer if possible
        uinteger_t t_uinteger;
        t_uinteger = strtoul(t_chars, &t_end, 10);
        
        // AL-2014-07-31: [[ Bug 12936 ]] Check the right number of chars has been consumed
        // SN-2014-10-06: [[ Bug 13594 ]] Also check that no error was encountered
        if (errno != ERANGE && t_end - t_chars == char_count)
            t_success = MCNumberCreateWithUnsignedInteger(t_uinteger, r_number);
        else
        {
            real64_t t_real;
            t_real = strtod(t_chars, &t_end);
            
            // AL-2014-07-31: [[ Bug 12936 ]] Check the right number of chars has been consumed
            if (t_end - t_chars == char_count)
                t_success = MCNumberCreateWithReal(t_real, r_number);
        }
    }

    return t_success;
}

bool MCNumberParse(MCStringRef p_string, MCNumberRef &r_number)
{
    return MCNumberParseOffset(p_string, 0, MCStringGetLength(p_string), r_number);
}

bool MCNumberParseUnicodeChars(const unichar_t *p_chars, uindex_t p_char_count, MCNumberRef& r_number)
{
	char *t_native_chars;
	if (!MCMemoryNewArray(p_char_count + 1, t_native_chars))
		return false;

	uindex_t t_native_char_count;
	MCUnicodeCharsMapToNative(p_chars, p_char_count, (char_t *)t_native_chars, t_native_char_count, '?');

	bool t_success;
	t_success = false;
	if (p_char_count >= 2 && t_native_chars[0] == '0' && (t_native_chars[1] == 'x' || t_native_chars[1] == 'X'))
		t_success = MCNumberCreateWithInteger(strtoul(t_native_chars + 2, nil, 16), r_number);
	else
	{
		char *t_end;
		integer_t t_integer;
		t_integer = strtoul(t_native_chars, &t_end, 10);

        // SN-2014-10-07: [[ Bug 13594 ]] Check that strtoul did not fail.
		if (errno != ERANGE && *t_end == '\0')
			t_success = MCNumberCreateWithInteger(t_integer, r_number);
		else
		{
			real64_t t_real;
			t_real = strtod(t_native_chars, &t_end);

			if (*t_end == '\0')
				t_success = MCNumberCreateWithReal(t_real, r_number);
		}
	}

	MCMemoryDeleteArray(t_native_chars);

	return t_success;
}

////////////////////////////////////////////////////////////////////////////////

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

MCNumberRef kMCZero;
MCNumberRef kMCOne;
MCNumberRef kMCMinusOne;

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
