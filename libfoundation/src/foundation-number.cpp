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

bool MCNumberParseUnicodeChars(const unichar_t *p_chars, uindex_t p_char_count, MCNumberRef& r_number)
{
	char *t_native_chars;
	if (!MCMemoryNewArray(p_char_count + 1, t_native_chars))
		return false;

	uindex_t t_native_char_count;
	MCUnicodeCharsMapToNative(p_chars, p_char_count, (char_t *)t_native_chars, t_native_char_count, '?');

	bool t_success;
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
			t_success = MCNumberCreateWithReal(strtod(t_native_chars, &t_end), r_number);
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
