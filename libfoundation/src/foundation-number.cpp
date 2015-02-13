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

enum __MCNumberRepType
{
    __kMCNumberRepSignedInteger = kMCNumberFlagSignedIntegerRep,
    __kMCNumberRepUnsignedInteger = kMCNumberFlagUnsignedIntegerRep,
    __kMCNumberRepDouble = kMCNumberFlagDoubleRep,
};

static inline __MCNumberRepType __MCNumberGetRepType(MCNumberRef self)
{
    return (__MCNumberRepType)(self -> flags & kMCNumberFlagRepMask);
}

static inline bool __MCNumberIsSignedIntegerRep(MCNumberRef self)
{
    return __MCNumberGetRepType(self) == __kMCNumberRepSignedInteger;
}

static inline bool __MCNumberIsUnsignedIntegerRep(MCNumberRef self)
{
    return __MCNumberGetRepType(self) == __kMCNumberRepUnsignedInteger;
}

static inline bool __MCNumberIsDoubleRep(MCNumberRef self)
{
    return __MCNumberGetRepType(self) == __kMCNumberRepDouble;
}

static bool __MCNumberThrowOverflowError(void)
{
    return MCErrorCreateAndThrow(kMCNumberOverflowErrorTypeInfo, nil);
}

static bool __MCNumberThrowDivisionByZeroError(void)
{
    return MCErrorCreateAndThrow(kMCNumberDivisionByZeroErrorTypeInfo, nil);
}

////////////////////////////////////////////////////////////////////////////////

bool MCNumberCreateWithInteger(integer_t p_value, MCNumberRef& r_number)
{
	__MCNumber *self;
	if (!__MCValueCreate(kMCValueTypeCodeNumber, self))
		return false;

    self -> flags |= kMCNumberFlagSignedIntegerRep;
	self -> integer = p_value;

	r_number = self;

	return true;
}

bool MCNumberCreateWithReal(real64_t p_value, MCNumberRef& r_number)
{
	__MCNumber *self;
	if (!__MCValueCreate(kMCValueTypeCodeNumber, self))
		return false;
    
	self -> flags |= kMCNumberFlagDoubleRep;
	self -> real = p_value;

	r_number = self;

	return true;
}

bool MCNumberCreateWithUnsignedInteger(uinteger_t p_value, MCNumberRef& r_number)
{
    if (p_value <= INTEGER_MAX)
        return MCNumberCreateWithInteger((integer_t)p_value, r_number);
    
	__MCNumber *self;
	if (!__MCValueCreate(kMCValueTypeCodeNumber, self))
		return false;
    
    self -> flags |= kMCNumberFlagUnsignedIntegerRep;
	self -> unsigned_integer = p_value;
    
	r_number = self;
    
	return true;
}

bool MCNumberIsInteger(MCNumberRef self)
{
    return !__MCNumberIsDoubleRep(self);
}

bool MCNumberIsReal(MCNumberRef self)
{
    return __MCNumberIsDoubleRep(self);
}

bool MCNumberIsFinite(MCNumberRef self)
{
    if (!__MCNumberIsDoubleRep(self))
        return true;
    return isfinite(self -> real);
}

bool MCNumberIsFetchableAsInteger(MCNumberRef self)
{
    switch(__MCNumberGetRepType(self))
    {
        case __kMCNumberRepSignedInteger:
            return true;
        case __kMCNumberRepUnsignedInteger:
            // Unsigned integer rep is only used for values outside the range
            // of integer rep.
            return false;
        case __kMCNumberRepDouble:
            return false;
        default:
            MCUnreachable();
            break;
    }
    
    return false;
}

bool MCNumberIsFetchableAsUnsignedInteger(MCNumberRef self)
{
    switch(__MCNumberGetRepType(self))
    {
        case __kMCNumberRepSignedInteger:
            return self -> integer >= 0;
        case __kMCNumberRepUnsignedInteger:
            return true;
        case __kMCNumberRepDouble:
            return false;
        default:
            MCUnreachable();
            break;
    }
    
    return false;
}

integer_t MCNumberFetchAsInteger(MCNumberRef self)
{
    switch(__MCNumberGetRepType(self))
    {
        case __kMCNumberRepSignedInteger:
            return self -> integer;
        case __kMCNumberRepUnsignedInteger:
            return self -> unsigned_integer < INT32_MAX ? (integer_t)self -> unsigned_integer : INT32_MAX;
        case __kMCNumberRepDouble:
            return self -> real < 0.0 ? (integer_t)(self -> real - 0.5) : (integer_t)(self -> real + 0.5);
        default:
            MCUnreachable();
            break;
    }
    
    return 0;
}

uinteger_t MCNumberFetchAsUnsignedInteger(MCNumberRef self)
{
    switch(__MCNumberGetRepType(self))
    {
        case __kMCNumberRepSignedInteger:
            return self -> integer >= 0 ? (uinteger_t)self -> integer : 0;
        case __kMCNumberRepUnsignedInteger:
            return self -> unsigned_integer;
        case __kMCNumberRepDouble:
            return self -> real < 0.0 ? 0.0 : (uinteger_t)(self -> real + 0.5);
        default:
            MCUnreachable();
            break;
    }
    
    return 0;
}

real64_t MCNumberFetchAsReal(MCNumberRef self)
{
    switch(__MCNumberGetRepType(self))
    {
        case __kMCNumberRepSignedInteger:
            return self -> integer;
        case __kMCNumberRepUnsignedInteger:
            return self -> unsigned_integer;
        case __kMCNumberRepDouble:
            return self -> real;
        default:
            MCUnreachable();
            break;
    }
    
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

bool MCNumberFormat(MCNumberRef self, MCStringRef& r_string)
{
    switch(__MCNumberGetRepType(self))
    {
        case __kMCNumberRepSignedInteger:
            return MCStringFormat(r_string, "%d", self -> integer);
        case __kMCNumberRepUnsignedInteger:
            return MCStringFormat(r_string, "%u", self -> unsigned_integer);
        case __kMCNumberRepDouble:
            return MCStringFormat(r_string, "%lf", self -> real);
        default:
            MCUnreachable();
            break;
    }

    return false;
}

bool MCNumberTryToParse(MCStringRef p_string, MCNumberRef& r_number)
{
    // If the string is not native, then it cannot be a number since
    // numbers are composed of ASCII characters.
    if (!MCStringIsNative(p_string))
        goto error_exit;
    
    // As we know the string is native, this will never return nil.
    // (The returned string is always guaranteed to be a C-string -
    // at the moment at least!)
    const char_t *t_chars;
    t_chars = MCStringGetNativeCharPtr(p_string);
    
    // The C stdlib number parsing functions will skip whitespace at
    // the start. Whitespace is not considered to be a valid prefix to
    // a number in this case, so we check for it and exit if it is
    // there.
    if (isspace(t_chars[0]))
        goto error_exit;
    
    char *t_end;
    integer_t t_integer;
    t_integer = strtol((const char *)t_chars, &t_end, 10);
    if (errno != ERANGE && *t_end == '\0')
        return MCNumberCreateWithInteger(t_integer, r_number);
    
    uinteger_t t_uinteger;
    t_uinteger = strtoul((const char *)t_chars, &t_end, 10);
    if (errno != ERANGE && *t_end == '\0')
        return MCNumberCreateWithUnsignedInteger(t_uinteger, r_number);
    
    // strtod will return HUGE_VAL or 0 if overflow or underflow happens.
    // both of these are fine since we happily propagate such things through
    // arithmetic at the MCNumber level.
    double t_real;
    t_real = strtod((const char *)t_chars, &t_end);
    if (*t_end == '\0')
        return MCNumberCreateWithReal(t_real, r_number);
    
error_exit:
    // If we get here then the string isn't in a format we recognise so return
    // (the C version of) 'undefined'.
    r_number = nil;
    
    return true;
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
    switch(__MCNumberGetRepType(self))
    {
        case __kMCNumberRepSignedInteger:
            return MCHashInteger(self -> integer);
        case __kMCNumberRepUnsignedInteger:
            return MCHashInteger((integer_t)self -> unsigned_integer);
        case __kMCNumberRepDouble:
            return MCHashDouble(self -> real);
        default:
            MCUnreachable();
            break;
    }
    
    return 0;
}

bool __MCNumberIsEqualTo(__MCNumber *self, __MCNumber *p_other_self)
{
    if ((self -> flags & kMCNumberFlagRepMask) != (p_other_self -> flags & kMCNumberFlagRepMask))
        return false;
    
    switch(__MCNumberGetRepType(self))
    {
        case __kMCNumberRepSignedInteger:
            return self -> integer == p_other_self -> integer;
        case __kMCNumberRepUnsignedInteger:
            return self -> unsigned_integer == p_other_self -> unsigned_integer;
        case __kMCNumberRepDouble:
            return self -> real == p_other_self -> real;
        default:
            MCUnreachable();
            break;
    }
    
    return false;
}

////////////////////////////////////////////////////////////////////////////////

static inline bool __checked_signed_add(int32_t x, int32_t y, int32_t *z)
{
    if (((y > 0) && (x > (INT32_MAX - y))) ||
       ((y < 0) && (x < (INT32_MIN - y))))
        return false;
    
    *z = x + y;

    return true;
}

static inline bool __checked_signed_subtract(int32_t x, int32_t y, int32_t *z)
{
    if ((y > 0 && x < INT32_MIN + y) ||
        (y < 0 && x > INT32_MAX + y))
        return false;
    
    *z = x - y;
    
    return true;
}

static inline bool __checked_signed_multiply(int32_t x, int32_t y, int32_t *z)
{
    int64_t w;
    w = (int64_t)x * (int64_t)y;
    if ((w > INT32_MAX) || (w < INT32_MIN))
        return false;
    
    *z = (int32_t)w;
    
    return true;
}

static inline bool __checked_unsigned_add(uint32_t x, uint32_t y, uint32_t *z)
{
    if (UINT32_MAX - x < y)
        return false;
    
    *z = x + y;
    
    return true;
}

static inline bool __checked_unsigned_subtract(uint32_t x, uint32_t y, uint32_t *z)
{
    if (x < y)
        return false;
    
    *z = x - y;
    
    return true;
}

static inline bool __checked_unsigned_multiply(uint32_t x, uint32_t y, uint32_t *z)
{
    uint64_t w;
    w = (uint64_t)x * (uint64_t)y;
    if (w > UINT32_MAX)
        return false;
    
    *z = (uint32_t)w;
    
    return true;
}

// Returns the unsigned value which is the negation of the signed value x.
// Note that x is known to be strictly negative.
static inline uinteger_t __negate_negative_signed(integer_t x)
{
    return (uinteger_t)(-((int64_t)x));
}

////////////////////////////////////////////////////////////////////////////////

// The MCNumber operation implementation is optimized for working with the same
// data type. Other codepaths will be slower.

enum __MCNumberBinaryType
{
    __kMCNumberBinaryTypeSignedIntegerBySignedInteger = (kMCNumberFlagSignedIntegerRep << kMCNumberFlagRepBitCount) | kMCNumberFlagSignedIntegerRep,
    __kMCNumberBinaryTypeSignedIntegerByUnsignedInteger = (kMCNumberFlagSignedIntegerRep << kMCNumberFlagRepBitCount) | kMCNumberFlagUnsignedIntegerRep,
    __kMCNumberBinaryTypeSignedIntegerByDouble = (kMCNumberFlagSignedIntegerRep << kMCNumberFlagRepBitCount) | kMCNumberFlagDoubleRep,
    __kMCNumberBinaryTypeUnsignedIntegerBySignedInteger = (kMCNumberFlagUnsignedIntegerRep << kMCNumberFlagRepBitCount) | kMCNumberFlagSignedIntegerRep,
    __kMCNumberBinaryTypeUnsignedIntegerByUnsignedInteger = (kMCNumberFlagUnsignedIntegerRep << kMCNumberFlagRepBitCount) | kMCNumberFlagUnsignedIntegerRep,
    __kMCNumberBinaryTypeUnsignedIntegerByDouble = (kMCNumberFlagUnsignedIntegerRep << kMCNumberFlagRepBitCount) | kMCNumberFlagDoubleRep,
    __kMCNumberBinaryTypeDoubleBySignedInteger = (kMCNumberFlagDoubleRep << kMCNumberFlagRepBitCount) | kMCNumberFlagSignedIntegerRep,
    __kMCNumberBinaryTypeDoubleByUnsignedInteger = (kMCNumberFlagDoubleRep << kMCNumberFlagRepBitCount) | kMCNumberFlagUnsignedIntegerRep,
    __kMCNumberBinaryTypeDoubleByDouble = (kMCNumberFlagDoubleRep << kMCNumberFlagRepBitCount) | kMCNumberFlagDoubleRep,
};

static inline __MCNumberBinaryType __MCNumberComputeBinaryType(MCNumberRef x, MCNumberRef y)
{
    return (__MCNumberBinaryType)(((x -> flags & kMCNumberFlagRepMask) << kMCNumberFlagRepBitCount) | (y -> flags & kMCNumberFlagRepMask));
}

//////////

// This type must be big enough to be able to hold the result of any arithmeticå
// binary operation performed on a combination of integer_t and uinteger_t.
typedef int64_t bigint_t;

struct __MCNumberOperationAdd
{
    static inline double double_by_double(double x, double y)
    {
        return x + y;
    }
 
    static inline bigint_t bigint_by_bigint(bigint_t x, bigint_t y)
    {
        return x + y;
    }
    
    static inline bool signed_by_signed(integer_t x, integer_t y, integer_t& r_value)
    {
        return __checked_signed_add(x, y, &r_value);
    }
    
    static inline bool unsigned_by_unsigned(uinteger_t x, uinteger_t y, uinteger_t& r_value)
    {
        return __checked_unsigned_add(x, y, &r_value);
    }
    
    static inline bool signed_by_unsigned(integer_t x, uinteger_t y, integer_t& r_value)
    {
        // As we are guaranteed that y will be in the range (INT_MAX, UINT_MAX]
        // if x is positive, we cannot produce an integer_t.
        if (x > 0)
            return false;
        
        // if x <= 0 then
        //   x + y === -(-x) + y === y - (-x)
        uinteger_t z;
        if (!__checked_unsigned_subtract(y, (uinteger_t)-x, &z))
            return false;
        
        if (z > INTEGER_MAX)
            return false;
        
        r_value = (integer_t)z;
        
        return true;
    }
    
    static inline bool unsigned_by_signed(uinteger_t x, integer_t y, integer_t& r_value)
    {
        // As we are guaranteed that x will be in the range (INT_MAX, UINT_MAX]
        // if y is positive, we cannot produce an integer_t.
        if (y > 0)
            return false;
        
        // if y <= 0 then
        //   x + y === x + -(-y) === x - (-y)
        uinteger_t z;
        if (!__checked_unsigned_subtract(x, __negate_negative_signed(y), &z))
            return false;
        
        if (z > INTEGER_MAX)
            return false;
        
        r_value = (integer_t)z;
        
        return true;
    }
};

struct __MCNumberOperationSubtract
{
    static inline double double_by_double(double x, double y)
    {
        return x - y;
    }
    
    static inline bigint_t bigint_by_bigint(bigint_t x, bigint_t y)
    {
        return x - y;
    }
    
    static inline bool signed_by_signed(integer_t x, integer_t y, integer_t& r_value)
    {
        return __checked_signed_subtract(x, y, &r_value);
    }
    
    static inline bool unsigned_by_unsigned(uinteger_t x, uinteger_t y, uinteger_t& r_value)
    {
        return __checked_unsigned_subtract(x, y, &r_value);
    }
    
    static inline bool signed_by_unsigned(integer_t x, uinteger_t y, integer_t& r_value)
    {
        // As we are guaranteed that y will be in the range (INT_MAX, UINT_MAX]
        // if x is negative, we cannot produce an integer_t
        if (x < 0)
            return false;
        
        // x - y === -(y - x)
        uinteger_t z;
        if (!__checked_unsigned_subtract(y, x, &z))
            return false;
        
        if (z > __negate_negative_signed(INTEGER_MIN))
            return false;
        
        r_value = -z;
        
        return true;
    }
    
    static inline bool unsigned_by_signed(uinteger_t x, integer_t y, integer_t& r_value)
    {
        // As we are guaranteed that x will be in the range (INT_MAX, UINT_MAX]
        // if y is negative, we cannot produce an integer_t
        if (y < 0)
            return false;
        
        // y is now known to be non-negative.
        uinteger_t z;
        if (!__checked_unsigned_subtract(x, (uinteger_t)y, &z))
            return false;
        
        if (z > INTEGER_MAX)
            return false;
        
        r_value = (integer_t)z;
        
        return true;
    }
};

struct __MCNumberOperationMultiply
{
    static inline double double_by_double(double x, double y)
    {
        return x * y;
    }
    
    static inline bigint_t bigint_by_bigint(bigint_t x, bigint_t y)
    {
        return x * y;
    }
    
    static inline bool signed_by_signed(integer_t x, integer_t y, integer_t& r_value)
    {
        return __checked_signed_multiply(x, y, &r_value);
    }
    
    static inline bool unsigned_by_unsigned(uinteger_t x, uinteger_t y, uinteger_t& r_value)
    {
        return __checked_unsigned_multiply(x, y, &r_value);
    }
    
    static inline bool signed_by_unsigned(integer_t x, uinteger_t y, integer_t& r_value)
    {
        // We are guaranteed that y is in the range (INT_MAX, UINT_MAX] so
        // x * y will only fit into an integer_t if x is 0.
        if (x == 0)
        {
            r_value = 0;
            return true;
        }
        
        return false;
    }
    
    static inline bool unsigned_by_signed(uinteger_t x, integer_t y, integer_t& r_value)
    {
        // We are guaranteed that y is in the range (INT_MAX, UINT_MAX] so
        // x * y will only fit into an integer_t if y is 0.
        if (y == 0)
        {
            r_value = 0;
            return true;
        }
        
        return false;
    }
};

// The '/' operator always produces reals (use div or mod for integer ops).
struct __MCNumberOperationDivide
{
    static inline double double_by_double(double x, double y)
    {
        return x / y;
    }
    
    static inline bool signed_by_signed(integer_t x, integer_t y, integer_t& r_value)
    {
        return false;
    }
    
    static inline bool unsigned_by_unsigned(uinteger_t x, uinteger_t y, uinteger_t& r_value)
    {
        return false;
    }
    
    static inline bool signed_by_unsigned(integer_t x, uinteger_t y, integer_t& r_value)
    {
        return false;
    }
    
    static inline bool unsigned_by_signed(uinteger_t x, integer_t y, integer_t& r_value)
    {
        return false;
    }
};

struct __MCNumberOperationDiv
{
    static inline double double_by_double(double x, double y)
    {
        return floor(x / y);
    }
    
    static inline bigint_t bigint_by_bigint(bigint_t x, bigint_t y)
    {
        // Note: div by zero is handled in the caller (integer code path).

        return x / y;
    }
    
    static inline bool signed_by_signed(integer_t x, integer_t y, integer_t& r_value)
    {
        // If we are dividing by zero, integers cannot represent the result.
        if (y == 0)
            return false;
        
        // The only case which we can't represent is INT_MIN/-1.
        if (x == INTEGER_MIN && y == -1)
            return false;
        
        r_value = x / y;
        
        return true;
    }
    
    static inline bool unsigned_by_unsigned(uinteger_t x, uinteger_t y, uinteger_t& r_value)
    {
        // We know y is in the range (INT_MAX, UINT_MAX] so there is no need to check for y == 0.
        
        r_value = x / y;
        
        return true;
    }
    
    static inline bool signed_by_unsigned(integer_t x, uinteger_t y, integer_t& r_value)
    {
        // We know y is in the range (INT_MAX, UINT_MAX] - so this is always
        // zero (as a side-effect there is no need to check for y == 0).
        
        r_value = 0;
        
        return true;
    }
    
    static inline bool unsigned_by_signed(uinteger_t x, integer_t y, integer_t& r_value)
    {
        // If we are dividing by zero, integers cannot represent the result.
        if (y == 0)
            return false;
        
        // If y is negative we try to do an unsigned operation, adjusting appropriately.
        if (y < 0)
        {
            uinteger_t t_value;
            t_value = x / __negate_negative_signed(y);
            if (t_value > __negate_negative_signed(INTEGER_MIN))
                return false;
            
            r_value = -t_value;
            
            return true;
        }
        
        r_value = x / (uinteger_t)y;
        
        return true;
    }
};

// C's % operator isn't quite what we want as the sign of the remainder
// is undefined if either x or y are negative.
template<typename T> inline T __flooring_integral_mod(T x, T y)
{
    if (x < y || y < 0)
        return x - (x / y) * y;
    return x % y;
}

struct __MCNumberOperationMod
{
    static inline double double_by_double(double x, double y)
    {
        // fmod nor remainder can be used here as they use the 'integer nearest
        // the exact value of x/y' and not floor(x/y).
        return x - floor(x / y) * y;
    }
    
    static inline bigint_t bigint_by_bigint(bigint_t x, bigint_t y)
    {
        // Note: div by zero is handled in the caller (integer code path).
        return __flooring_integral_mod(x, y);
    }
    
    static inline bool signed_by_signed(integer_t x, integer_t y, integer_t& r_value)
    {
        // If we are dividing by zero, integers cannot represent the result.
        if (y == 0)
            return false;
        
        // -y < x mod y < y so we can represent all results (no -INT_MIN not representable
        // edge case).
        
        r_value = __flooring_integral_mod(x, y);
        
        return true;
    }
    
    static inline bool unsigned_by_unsigned(uinteger_t x, uinteger_t y, uinteger_t& r_value)
    {
        // We know y is in the range (INT_MAX, UINT_MAX] so there is no need to check for y == 0.
        
        // We are dealing with unsigned numbers so there are no edge cases to deal
        // with (even the plain C operator will do!).
        r_value = x % y;
        
        return true;
    }
    
    static inline bool signed_by_unsigned(integer_t x, uinteger_t y, integer_t& r_value)
    {
        // We know y is in the range (INT_MAX, UINT_MAX] - y is always greater than
        // x so the result is always abs(x).
        
        if (x < 0)
        {
            r_value = x;
            return true;
        }
        
        r_value = -x;
        
        return true;
    }
    
    static inline bool unsigned_by_signed(uinteger_t x, integer_t y, integer_t& r_value)
    {
        // If we are dividing by zero, integers cannot represent the result.
        if (y == 0)
            return false;
        
        // The result is always in the range -y < result < y so there is no
        // representation problem.
        
        // If y is negative, then we do an unsigned operation.
        if (y < 0)
        {
            r_value = (integer_t)(-(x % __negate_negative_signed(y)));
            return true;
        }
        
        r_value = (integer_t)(x % (uinteger_t)y);
        
        return true;
    }
};

struct __MCNumberOperationIsEqualTo
{
    static bool double_by_double(double x, double y)
    {
        return x == y;
    }
    
    static bool signed_by_signed(integer_t x, integer_t y)
    {
        return x == y;
    }
    
    static bool unsigned_by_unsigned(integer_t x, integer_t y)
    {
        return x == y;
    }
    
    static bool signed_by_unsigned(integer_t x, uinteger_t y)
    {
        // y is in the range (INT_MAX, UINT_MAX].
        return false;
    }
    
    static bool unsigned_by_signed(uinteger_t x, integer_t y)
    {
        // x is in the range (INT_MAX, UINT_MAX].
        return false;
    }
};

struct __MCNumberOperationIsNotEqualTo
{
    static bool double_by_double(double x, double y)
    {
        return x != y;
    }
    
    static bool signed_by_signed(integer_t x, integer_t y)
    {
        return x != y;
    }
    
    static bool unsigned_by_unsigned(integer_t x, integer_t y)
    {
        return x != y;
    }
    
    static bool signed_by_unsigned(integer_t x, uinteger_t y)
    {
        // y is in the range (INT_MAX, UINT_MAX].
        return true;
    }
    
    static bool unsigned_by_signed(uinteger_t x, integer_t y)
    {
        // x is in the range (INT_MAX, UINT_MAX].
        return true;
    }
};

struct __MCNumberOperationIsLessThan
{
    static bool double_by_double(double x, double y)
    {
        return x < y;
    }
    
    static bool signed_by_signed(integer_t x, integer_t y)
    {
        return x < y;
    }
    
    static bool unsigned_by_unsigned(integer_t x, integer_t y)
    {
        return x < y;
    }
    
    static bool signed_by_unsigned(integer_t x, uinteger_t y)
    {
        // y is in the range (INT_MAX, UINT_MAX].
        //   so x is always less than y
        return true;
    }
    
    static bool unsigned_by_signed(uinteger_t x, integer_t y)
    {
        // x is in the range (INT_MAX, UINT_MAX].
        //   so x is never less than y
        return false;
    }
};

struct __MCNumberOperationIsLessThanOrEqualTo
{
    static bool double_by_double(double x, double y)
    {
        return x <= y;
    }
    
    static bool signed_by_signed(integer_t x, integer_t y)
    {
        return x <= y;
    }
    
    static bool unsigned_by_unsigned(integer_t x, integer_t y)
    {
        return x <= y;
    }
    
    static bool signed_by_unsigned(integer_t x, uinteger_t y)
    {
        // y is in the range (INT_MAX, UINT_MAX].
        //   so x is always less than (or equal to) y
        return true;
    }
    
    static bool unsigned_by_signed(uinteger_t x, integer_t y)
    {
        // x is in the range (INT_MAX, UINT_MAX].
        //   so x is never less than or equal to y
        return false;
    }
};

struct __MCNumberOperationIsGreaterThan
{
    static bool double_by_double(double x, double y)
    {
        return x > y;
    }
    
    static bool signed_by_signed(integer_t x, integer_t y)
    {
        return x > y;
    }
    
    static bool unsigned_by_unsigned(integer_t x, integer_t y)
    {
        return x > y;
    }
    
    static bool signed_by_unsigned(integer_t x, uinteger_t y)
    {
        // y is in the range (INT_MAX, UINT_MAX].
        //   so x is never greater than y
        return false;
    }
    
    static bool unsigned_by_signed(uinteger_t x, integer_t y)
    {
        // x is in the range (INT_MAX, UINT_MAX].
        //   so x is always greater than y
        return true;
    }
};

struct __MCNumberOperationIsGreaterThanOrEqualTo
{
    static bool double_by_double(double x, double y)
    {
        return x >= y;
    }
    
    static bool signed_by_signed(integer_t x, integer_t y)
    {
        return x >= y;
    }
    
    static bool unsigned_by_unsigned(integer_t x, integer_t y)
    {
        return x >= y;
    }
    
    static bool signed_by_unsigned(integer_t x, uinteger_t y)
    {
        // y is in the range (INT_MAX, UINT_MAX].
        //   so x is never greater than or equal to y
        return false;
    }
    
    static bool unsigned_by_signed(uinteger_t x, integer_t y)
    {
        // x is in the range (INT_MAX, UINT_MAX].
        //   so x is always greater than or equal to y
        return true;
    }
};

//////////

template<typename T> inline bool __MCNumberOverflowingIntegerBinaryOperation(double x, double y, MCNumberRef& r_z)
{
    double t_value;
    t_value = T::double_by_double(x, y);
    
    if (t_value < INTEGER_MIN || t_value > UINTEGER_MAX)
        return MCNumberCreateWithReal(t_value, r_z);
    
    if (t_value > INTEGER_MAX)
        return MCNumberCreateWithUnsignedInteger((uint32_t)llrint(t_value), r_z);
    
    return MCNumberCreateWithInteger(lrint(t_value), r_z);
}

template<typename T> inline bool __MCNumberDoubleBinaryOperation(double x, double y, MCNumberRef& r_z)
{
    return MCNumberCreateWithReal(T::double_by_double(x, y), r_z);
}

template<typename T> inline bool __MCNumberBinaryOperation(MCNumberRef x, MCNumberRef y, MCNumberRef& r_z)
{
    switch(__MCNumberComputeBinaryType(x, y))
    {
        case __kMCNumberBinaryTypeSignedIntegerBySignedInteger:
        {
            integer_t t_signed_value;
            if (!T::signed_by_signed(x -> integer, y -> integer,t_signed_value))
                return __MCNumberOverflowingIntegerBinaryOperation<T>(x -> integer, y -> integer, r_z);
            return MCNumberCreateWithInteger(t_signed_value, r_z);
        }
        break;
        case __kMCNumberBinaryTypeSignedIntegerByUnsignedInteger:
        {
            integer_t t_signed_value;
            if (!T::signed_by_unsigned(x -> integer, y -> unsigned_integer, t_signed_value))
                return __MCNumberOverflowingIntegerBinaryOperation<T>(x -> integer, y -> unsigned_integer, r_z);
            return MCNumberCreateWithInteger(t_signed_value, r_z);
        }
        break;
        case __kMCNumberBinaryTypeSignedIntegerByDouble:
            return __MCNumberDoubleBinaryOperation<T>(x -> integer, y -> real, r_z);
        case __kMCNumberBinaryTypeUnsignedIntegerBySignedInteger:
        {
            integer_t t_signed_value;
            if (!T::unsigned_by_signed(x -> unsigned_integer, y -> integer, t_signed_value))
                return __MCNumberOverflowingIntegerBinaryOperation<T>(x -> unsigned_integer, y -> integer, r_z);
            return MCNumberCreateWithInteger(t_signed_value, r_z);
        }
        break;
        case __kMCNumberBinaryTypeUnsignedIntegerByUnsignedInteger:
        {
            uinteger_t t_unsigned_value;
            if (!T::unsigned_by_unsigned(x -> unsigned_integer, y -> unsigned_integer, t_unsigned_value))
                return __MCNumberOverflowingIntegerBinaryOperation<T>(x -> unsigned_integer, y -> unsigned_integer, r_z);
            return MCNumberCreateWithUnsignedInteger(t_unsigned_value, r_z);
        }
        break;
        case __kMCNumberBinaryTypeUnsignedIntegerByDouble:
            return __MCNumberDoubleBinaryOperation<T>(x -> unsigned_integer, y -> real, r_z);
        case __kMCNumberBinaryTypeDoubleBySignedInteger:
            return __MCNumberDoubleBinaryOperation<T>(x -> real, y -> integer, r_z);
        case __kMCNumberBinaryTypeDoubleByUnsignedInteger:
            return __MCNumberDoubleBinaryOperation<T>(x -> real, y -> unsigned_integer, r_z);
        case __kMCNumberBinaryTypeDoubleByDouble:
            return __MCNumberDoubleBinaryOperation<T>(x -> real, y -> real, r_z);
        default:
            MCUnreachable();
            break;
    }
    
    return false;
}

template<typename T> inline bool __MCNumberComparisonOperation(MCNumberRef x, MCNumberRef y)
{
    switch(__MCNumberComputeBinaryType(x, y))
    {
        case __kMCNumberBinaryTypeSignedIntegerBySignedInteger:
            return T::signed_by_signed(x -> integer, y -> integer);
        case __kMCNumberBinaryTypeSignedIntegerByUnsignedInteger:
            return T::signed_by_unsigned(x -> integer, y -> unsigned_integer);
        case __kMCNumberBinaryTypeSignedIntegerByDouble:
            return T::double_by_double(x -> integer, y -> real);
        case __kMCNumberBinaryTypeUnsignedIntegerBySignedInteger:
            return T::unsigned_by_signed(x -> unsigned_integer, y -> integer);
        case __kMCNumberBinaryTypeUnsignedIntegerByUnsignedInteger:
            return T::unsigned_by_unsigned(x -> unsigned_integer, y -> unsigned_integer);
        case __kMCNumberBinaryTypeUnsignedIntegerByDouble:
            return T::double_by_double(x -> unsigned_integer, y -> real);
        case __kMCNumberBinaryTypeDoubleBySignedInteger:
            return T::double_by_double(x -> real, y -> integer);
        case __kMCNumberBinaryTypeDoubleByUnsignedInteger:
            return T::double_by_double(x -> real, y -> unsigned_integer);
        case __kMCNumberBinaryTypeDoubleByDouble:
            return T::double_by_double(x -> real, y -> real);
        default:
            MCUnreachable();
            break;
    }

    return false;
}

//////////

bool MCNumberNegate(MCNumberRef x, MCNumberRef& r_y)
{
    switch(__MCNumberGetRepType(x))
    {
        case __kMCNumberRepSignedInteger:
            if (x -> integer == INTEGER_MIN)
                return MCNumberCreateWithUnsignedInteger(__negate_negative_signed(INTEGER_MIN), r_y);
            return MCNumberCreateWithInteger(-x -> integer, r_y);
            
        case __kMCNumberRepUnsignedInteger:
            if (x -> unsigned_integer > __negate_negative_signed(INTEGER_MIN))
                return MCNumberCreateWithReal(-(double)x -> unsigned_integer, r_y);
            return MCNumberCreateWithInteger(-x -> unsigned_integer, r_y);
            
        case __kMCNumberRepDouble:
            return MCNumberCreateWithReal(-x -> real, r_y);
            
        default:
            MCUnreachable();
            break;
    }
    
    return false;
}

bool MCNumberAdd(MCNumberRef x, MCNumberRef y, MCNumberRef& r_z)
{
    return __MCNumberBinaryOperation<__MCNumberOperationAdd>(x, y, r_z);
}

bool MCNumberSubtract(MCNumberRef x, MCNumberRef y, MCNumberRef& r_z)
{
    return __MCNumberBinaryOperation<__MCNumberOperationSubtract>(x, y, r_z);
}

bool MCNumberMultiply(MCNumberRef x, MCNumberRef y, MCNumberRef& r_z)
{
    return __MCNumberBinaryOperation<__MCNumberOperationMultiply>(x, y, r_z);
}

bool MCNumberDivide(MCNumberRef x, MCNumberRef y, MCNumberRef& r_z)
{
    // We special case '/' as it always produces a real result.
    switch(__MCNumberComputeBinaryType(x, y))
    {
        case __kMCNumberBinaryTypeSignedIntegerBySignedInteger:
            return __MCNumberDoubleBinaryOperation<__MCNumberOperationDivide>(x -> integer, y -> integer, r_z);
        case __kMCNumberBinaryTypeSignedIntegerByUnsignedInteger:
            return __MCNumberDoubleBinaryOperation<__MCNumberOperationDivide>(x -> integer, y -> unsigned_integer, r_z);
        case __kMCNumberBinaryTypeSignedIntegerByDouble:
            return __MCNumberDoubleBinaryOperation<__MCNumberOperationDivide>(x -> integer, y -> real, r_z);
        case __kMCNumberBinaryTypeUnsignedIntegerBySignedInteger:
            return __MCNumberDoubleBinaryOperation<__MCNumberOperationDivide>(x -> unsigned_integer, y -> integer, r_z);
        case __kMCNumberBinaryTypeUnsignedIntegerByUnsignedInteger:
            return __MCNumberDoubleBinaryOperation<__MCNumberOperationDivide>(x -> unsigned_integer, y -> unsigned_integer, r_z);
        case __kMCNumberBinaryTypeUnsignedIntegerByDouble:
            return __MCNumberDoubleBinaryOperation<__MCNumberOperationDivide>(x -> unsigned_integer, y -> real, r_z);
        case __kMCNumberBinaryTypeDoubleBySignedInteger:
            return __MCNumberDoubleBinaryOperation<__MCNumberOperationDivide>(x -> real, y -> integer, r_z);
        case __kMCNumberBinaryTypeDoubleByUnsignedInteger:
            return __MCNumberDoubleBinaryOperation<__MCNumberOperationDivide>(x -> real, y -> unsigned_integer, r_z);
        case __kMCNumberBinaryTypeDoubleByDouble:
            return __MCNumberDoubleBinaryOperation<__MCNumberOperationDivide>(x -> real, y -> real, r_z);
        default:
            MCUnreachable();
            break;
    }
    
    return false;
}

bool MCNumberDiv(MCNumberRef x, MCNumberRef y, MCNumberRef& r_z)
{
    return __MCNumberBinaryOperation<__MCNumberOperationDiv>(x, y, r_z);
}

bool MCNumberMod(MCNumberRef x, MCNumberRef y, MCNumberRef& r_z)
{
    return __MCNumberBinaryOperation<__MCNumberOperationMod>(x, y, r_z);
}

bool MCNumberIsEqualTo(MCNumberRef x, MCNumberRef y)
{
    return __MCNumberComparisonOperation<__MCNumberOperationIsEqualTo>(x, y);
}

bool MCNumberIsNotEqualTo(MCNumberRef x, MCNumberRef y)
{
    return __MCNumberComparisonOperation<__MCNumberOperationIsNotEqualTo>(x, y);
}

bool MCNumberIsLessThan(MCNumberRef x, MCNumberRef y)
{
    return __MCNumberComparisonOperation<__MCNumberOperationIsLessThan>(x, y);
}

bool MCNumberIsLessThanOrEqualTo(MCNumberRef x, MCNumberRef y)
{
    return __MCNumberComparisonOperation<__MCNumberOperationIsLessThanOrEqualTo>(x, y);
}

bool MCNumberIsGreaterThan(MCNumberRef x, MCNumberRef y)
{
    return __MCNumberComparisonOperation<__MCNumberOperationIsGreaterThan>(x, y);
}

bool MCNumberIsGreaterThanOrEqualTo(MCNumberRef x, MCNumberRef y)
{
    return __MCNumberComparisonOperation<__MCNumberOperationIsGreaterThanOrEqualTo>(x, y);
}

////////////////////////////////////////////////////////////////////////////////

bool MCNumberFiniteNegate(MCNumberRef x, MCNumberRef& r_y)
{
    MCNumberRef y;
    if (!MCNumberNegate(x, y))
        return false;
    
    if (!MCNumberIsFinite(y))
        return __MCNumberThrowOverflowError();

    r_y = y;
    
    return true;
}

bool MCNumberFiniteAdd(MCNumberRef x, MCNumberRef y, MCNumberRef& r_z)
{
    MCNumberRef z;
    if (!MCNumberAdd(x, y, z))
        return false;
    
    if (!MCNumberIsFinite(z))
        return __MCNumberThrowOverflowError();
    
    r_z = z;
    
    return true;
    
}

bool MCNumberFiniteSubtract(MCNumberRef x, MCNumberRef y, MCNumberRef& r_z)
{
    MCNumberRef z;
    if (!MCNumberSubtract(x, y, z))
        return false;
    
    if (!MCNumberIsFinite(z))
        return __MCNumberThrowOverflowError();
    
    r_z = z;
    
    return true;
}

bool MCNumberFiniteMultiply(MCNumberRef x, MCNumberRef y, MCNumberRef& r_z)
{
    MCNumberRef z;
    if (!MCNumberMultiply(x, y, z))
        return false;
    
    if (!MCNumberIsFinite(z))
        return __MCNumberThrowOverflowError();
    
    r_z = z;
    
    return true;
}

bool MCNumberFiniteDivide(MCNumberRef x, MCNumberRef y, MCNumberRef& r_z)
{
    MCNumberRef z;
    if (!MCNumberDivide(x, y, z))
        return false;
    
    if (!MCNumberIsFinite(z))
    {
        if (y -> real != 0.0)
            return __MCNumberThrowOverflowError();
        
        return __MCNumberThrowDivisionByZeroError();
    }
    
    r_z = z;
    
    return true;
}

bool MCNumberFiniteDiv(MCNumberRef x, MCNumberRef y, MCNumberRef& r_z)
{
    MCNumberRef z;
    if (!MCNumberDiv(x, y, z))
        return false;
    
    if (!MCNumberIsFinite(z))
    {
        if (y -> real != 0.0)
            return __MCNumberThrowOverflowError();
        
        return __MCNumberThrowDivisionByZeroError();
    }
    
    r_z = z;
    
    return true;
}

bool MCNumberFiniteMod(MCNumberRef x, MCNumberRef y, MCNumberRef& r_z)
{
    MCNumberRef z;
    if (!MCNumberMod(x, y, z))
        return false;
    
    if (!MCNumberIsFinite(z))
        return __MCNumberThrowOverflowError();

    r_z = z;
    
    return true;
}

////////////////////////////////////////////////////////////////////////////////

template<typename T> inline bool __MCNumberOverflowingIntegralBinaryOperation(int64_t x, int64_t y, MCNumberRef& r_z)
{
    int64_t t_value;
    t_value = T::bigint_by_bigint(x, y);
    
    // If we can't fit in an unsigned or signed int then
    if (t_value < INTEGER_MIN || t_value > UINTEGER_MAX)
        return __MCNumberThrowOverflowError();
    
    if (t_value > INTEGER_MAX)
        return MCNumberCreateWithUnsignedInteger((uinteger_t)t_value, r_z);
    
    return MCNumberCreateWithInteger((integer_t)t_value, r_z);
}

template<typename T> inline bool __MCNumberIntegralBinaryOperation(MCNumberRef x, MCNumberRef y, MCNumberRef& r_z)
{
    switch(__MCNumberComputeBinaryType(x, y))
    {
        case __kMCNumberBinaryTypeSignedIntegerBySignedInteger:
        {
            integer_t t_signed_value;
            if (!T::signed_by_signed(x -> integer, y -> integer,t_signed_value))
                return __MCNumberOverflowingIntegralBinaryOperation<T>(x -> integer, y -> integer, r_z);
            return MCNumberCreateWithInteger(t_signed_value, r_z);
        }
        break;
        case __kMCNumberBinaryTypeSignedIntegerByUnsignedInteger:
        {
            integer_t t_signed_value;
            if (!T::signed_by_unsigned(x -> integer, y -> unsigned_integer, t_signed_value))
                return __MCNumberOverflowingIntegralBinaryOperation<T>(x -> integer, y -> unsigned_integer, r_z);
            return MCNumberCreateWithInteger(t_signed_value, r_z);
        }
        break;
        case __kMCNumberBinaryTypeUnsignedIntegerBySignedInteger:
        {
            integer_t t_signed_value;
            if (!T::unsigned_by_signed(x -> unsigned_integer, y -> integer, t_signed_value))
                return __MCNumberOverflowingIntegralBinaryOperation<T>(x -> unsigned_integer, y -> integer, r_z);
            return MCNumberCreateWithInteger(t_signed_value, r_z);
        }
        break;
        case __kMCNumberBinaryTypeUnsignedIntegerByUnsignedInteger:
        {
            uinteger_t t_unsigned_value;
            if (!T::unsigned_by_unsigned(x -> unsigned_integer, y -> unsigned_integer, t_unsigned_value))
                return __MCNumberOverflowingIntegralBinaryOperation<T>(x -> unsigned_integer, y -> unsigned_integer, r_z);
            return MCNumberCreateWithUnsignedInteger(t_unsigned_value, r_z);
        }
        break;
        default:
            MCUnreachable();
            break;
    }
    
    return false;
}

template<typename T> inline bool __MCNumberIntegralComparisonOperation(MCNumberRef x, MCNumberRef y)
{
    switch(__MCNumberComputeBinaryType(x, y))
    {
        case __kMCNumberBinaryTypeSignedIntegerBySignedInteger:
            return T::signed_by_signed(x -> integer, y -> integer);
        case __kMCNumberBinaryTypeSignedIntegerByUnsignedInteger:
            return T::signed_by_unsigned(x -> integer, y -> unsigned_integer);
        case __kMCNumberBinaryTypeUnsignedIntegerBySignedInteger:
            return T::unsigned_by_signed(x -> unsigned_integer, y -> integer);
        case __kMCNumberBinaryTypeUnsignedIntegerByUnsignedInteger:
            return T::unsigned_by_unsigned(x -> unsigned_integer, y -> unsigned_integer);
        default:
            MCUnreachable();
            break;
    }
    
    return false;
}

bool MCNumberIntegralNegate(MCNumberRef x, MCNumberRef& r_y)
{
    switch(__MCNumberGetRepType(x))
    {
        case __kMCNumberRepSignedInteger:
            if (x -> integer == INTEGER_MIN)
                return MCNumberCreateWithUnsignedInteger(__negate_negative_signed(INTEGER_MIN), r_y);
            return MCNumberCreateWithInteger(-x -> integer, r_y);
            
        case __kMCNumberRepUnsignedInteger:
            if (x -> unsigned_integer > __negate_negative_signed(INTEGER_MIN))
                return __MCNumberThrowOverflowError();
            return MCNumberCreateWithInteger(-x -> unsigned_integer, r_y);
            
        default:
            MCUnreachable();
            break;
    }
    
    return false;
}

bool MCNumberIntegralAdd(MCNumberRef x, MCNumberRef y, MCNumberRef& r_z)
{
    return __MCNumberIntegralBinaryOperation<__MCNumberOperationAdd>(x, y, r_z);
}

bool MCNumberIntegralSubtract(MCNumberRef x, MCNumberRef y, MCNumberRef& r_z)
{
    return __MCNumberIntegralBinaryOperation<__MCNumberOperationSubtract>(x, y, r_z);
}

bool MCNumberIntegralMultiply(MCNumberRef x, MCNumberRef y, MCNumberRef& r_z)
{
    return __MCNumberIntegralBinaryOperation<__MCNumberOperationMultiply>(x, y, r_z);
}

bool MCNumberIntegralDiv(MCNumberRef x, MCNumberRef y, MCNumberRef& r_z)
{
    // The representation of 0 for signed and unsigned ints is identical.
    if (y -> integer == 0)
        return __MCNumberThrowDivisionByZeroError();
    
    return __MCNumberIntegralBinaryOperation<__MCNumberOperationDiv>(x, y, r_z);
}

bool MCNumberIntegralMod(MCNumberRef x, MCNumberRef y, MCNumberRef& r_z)
{
    // The representation of 0 for signed and unsigned ints is identical.
    if (y -> integer == 0)
        return __MCNumberThrowDivisionByZeroError();
    
    return __MCNumberIntegralBinaryOperation<__MCNumberOperationMod>(x, y, r_z);
}

bool MCNumberIntegralIsEqualTo(MCNumberRef x, MCNumberRef y)
{
    return __MCNumberIntegralComparisonOperation<__MCNumberOperationIsEqualTo>(x, y);
}

bool MCNumberIntegralIsNotEqualTo(MCNumberRef x, MCNumberRef y)
{
    return __MCNumberIntegralComparisonOperation<__MCNumberOperationIsNotEqualTo>(x, y);
}

bool MCNumberIntegralIsLessThan(MCNumberRef x, MCNumberRef y)
{
    return __MCNumberIntegralComparisonOperation<__MCNumberOperationIsLessThan>(x, y);
}

bool MCNumberIntegralIsLessThanOrEqualTo(MCNumberRef x, MCNumberRef y)
{
    return __MCNumberIntegralComparisonOperation<__MCNumberOperationIsLessThanOrEqualTo>(x, y);
}

bool MCNumberIntegralIsGreaterThan(MCNumberRef x, MCNumberRef y)
{
    return __MCNumberIntegralComparisonOperation<__MCNumberOperationIsGreaterThan>(x, y);
}

bool MCNumberIntegralIsGreaterThanOrEqualTo(MCNumberRef x, MCNumberRef y)
{
    return __MCNumberIntegralComparisonOperation<__MCNumberOperationIsGreaterThanOrEqualTo>(x, y);
}

////////////////////////////////////////////////////////////////////////////////

MCNumberRef kMCIntegerZero;
MCNumberRef kMCIntegerOne;
MCNumberRef kMCIntegerMinusOne;

MCNumberRef kMCRealZero;
MCNumberRef kMCRealOne;
MCNumberRef kMCRealMinusOne;

MCTypeInfoRef kMCNumberDivisionByZeroErrorTypeInfo;
MCTypeInfoRef kMCNumberOverflowErrorTypeInfo;

bool __MCNumberInitialize(void)
{
    if (!MCNumberCreateWithInteger(0, kMCIntegerZero))
        return false;
    
    if (!MCNumberCreateWithInteger(1, kMCIntegerOne))
        return false;
		
    if (!MCNumberCreateWithInteger(-1, kMCIntegerMinusOne))
        return false;
    
    if (!MCNumberCreateWithReal(0.0, kMCRealZero))
        return false;
    
    if (!MCNumberCreateWithReal(1.0, kMCRealOne))
        return false;
    
    if (!MCNumberCreateWithReal(-1.0, kMCRealMinusOne))
        return false;
		
    if (!MCErrorTypeInfoCreate(MCNAME("arithmetic"), MCSTR("division by zero"), kMCNumberDivisionByZeroErrorTypeInfo))
        return false;
    
    if (!MCErrorTypeInfoCreate(MCNAME("arithmetic"), MCSTR("numeric overflow"), kMCNumberOverflowErrorTypeInfo))
        return false;
    
    return true;
}

void __MCNumberFinalize(void)
{
    MCValueRelease(kMCIntegerZero);
    MCValueRelease(kMCIntegerOne);
    MCValueRelease(kMCIntegerMinusOne);
    
    MCValueRelease(kMCRealZero);
    MCValueRelease(kMCRealOne);
    MCValueRelease(kMCRealMinusOne);
}

////////////////////////////////////////////////////////////////////////////////
