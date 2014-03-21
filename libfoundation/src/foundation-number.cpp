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

#include "foundation-private.h"

////////////////////////////////////////////////////////////////////////////////

#define TAGGED_INTEGER_MIN (-(1 << 30))
#define TAGGED_INTEGER_MAX ((1 << 30) - 1)

bool MCNumberCreateWithInteger(integer_t p_value, MCNumberRef& r_number)
{
    if (p_value >= TAGGED_INTEGER_MIN && p_value < TAGGED_INTEGER_MAX)
    {
        r_number = (MCNumberRef)(uintptr_t)((p_value << 2) | 0x01);
        return true;
    }
    
	__MCNumber *self;
	if (!__MCValueCreate(kMCValueTypeCodeNumber, self))
		return false;

	self -> integer = p_value;

	r_number = self;

	return true;
}

bool MCNumberCreateWithReal(real64_t p_value, MCNumberRef& r_number)
{
    double t_frac, t_int;
    t_frac = modf(p_value, &t_int);
    if (t_frac == 0.0 && t_int >= INTEGER_MIN && t_int < INTEGER_MAX)
        return MCNumberCreateWithInteger((integer_t)t_int, r_number);
    
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
    if (__MCValueIsTagged(self))
        return true;
	return (self -> flags & kMCNumberFlagIsReal) == 0;
}

bool MCNumberIsReal(MCNumberRef self)
{
    if (__MCValueIsTagged(self))
        return false;
	return (self -> flags & kMCNumberFlagIsReal) != 0;
}

real64_t MCNumberFetchAsReal(MCNumberRef self)
{
    if (__MCValueIsTagged(self))
        return (real64_t)(((intptr_t)self) >> 2);
	if (MCNumberIsReal(self))
		return self -> real;
	return (real64_t)self -> integer;
}

integer_t MCNumberFetchAsInteger(MCNumberRef self)
{
    if (__MCValueIsTagged(self))
        return (((intptr_t)self) >> 2);
	if (MCNumberIsInteger(self))
		return self -> integer;
	return self -> real < 0.0 ? (integer_t)(self -> real - 0.5) : (integer_t)(self -> real + 0.5);
}

uinteger_t MCNumberFetchAsUnsignedInteger(MCNumberRef self)
{
    if (__MCValueIsTagged(self))
    {
        integer_t t_integer;
        t_integer = (((intptr_t)self) >> 2);
        if (t_integer >= 0)
            return t_integer;
        return 0;
    }
	if (MCNumberIsInteger(self))
		return self -> integer >= 0 ? self -> integer : 0;
	return self -> real >= 0.0 ? (uinteger_t)(self -> real + 0.5) : (uinteger_t)0.0;
}

compare_t MCNumberCompareTo(MCNumberRef self, MCNumberRef p_other_self)
{
    bool t_self_is_integer;
    integer_t t_self_value;
    if (__MCValueIsTagged(self))
    {
        t_self_is_integer = true;
        t_self_value = (((intptr_t)self) >> 2);
    }
    else if ((self -> flags & kMCNumberFlagIsReal) == 0)
    {
        t_self_is_integer = true;
        t_self_value = self -> integer;
    }
    
    bool t_other_self_is_integer;
    integer_t t_other_self_value;
    if (__MCValueIsTagged(p_other_self))
    {
        t_other_self_is_integer = true;
        t_other_self_value = (((intptr_t)p_other_self) >> 2);
    }
    else if ((self -> flags & kMCNumberFlagIsReal) == 0)
    {
        t_other_self_is_integer = true;
        t_other_self_value = p_other_self -> integer;
    }

	// If both are stored as integers then compare.
	if (t_self_is_integer && t_other_self_is_integer)
        return t_self_value - t_other_self_value;
    
	// Otherwise fetch both as reals.
	double x, y;
	x = t_self_is_integer ? (double)t_self_value : self -> real;
	y = t_self_is_integer ? (double)t_other_self_value : p_other_self -> real;

	// TODO: Handle nan / infinity / etc.
		
	if (x < y)
		return -1;
		
	if (x > y)
		return 1;
		
	return 0;
}

bool MCNumberParse(MCStringRef p_string, MCNumberRef &r_number)
{
    if (!MCStringIsNative(p_string))
        return MCNumberParseUnicodeChars(MCStringGetCharPtr(p_string), MCStringGetLength(p_string), r_number);

    bool t_success;
    t_success = false;

    const char* t_chars = (const char*)MCStringGetNativeCharPtr(p_string);

    if (MCStringGetLength(p_string) > 2 &&
            t_chars[0] == '0' &&
            (t_chars[1] == 'x' || t_chars[1] == 'X'))
        t_success = MCNumberCreateWithInteger(strtoul(t_chars + 2, nil, 16), r_number);
    else
    {
        char *t_end;
        integer_t t_integer;
        t_integer = strtoul(t_chars, &t_end, 10);

        if (*t_end == '\0')
            t_success = MCNumberCreateWithInteger(t_integer, r_number);
        else
        {
            real64_t t_real;
            t_real = strtod(t_chars, &t_end);
            
            if (*t_end == '\0')
                t_success = MCNumberCreateWithReal(t_real, r_number);
        }
    }

    return t_success;
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

		if (*t_end == '\0')
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
		return MCStringFormat(r_string, "%d", MCNumberFetchAsInteger(self));
	return MCStringFormat(r_string, "%lf", self -> real);
}

hash_t __MCNumberHash(__MCNumber *self)
{
	if (MCNumberIsInteger(self))
		return MCHashInteger(MCNumberFetchAsInteger(self));
	return MCHashDouble(self -> real);
}

bool __MCNumberIsEqualTo(__MCNumber *self, __MCNumber *p_other_self)
{
	return MCNumberCompareTo(self, p_other_self) == 0;
}

////////////////////////////////////////////////////////////////////////////////
