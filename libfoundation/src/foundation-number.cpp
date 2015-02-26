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

static bool __MCNumberCreate(MCNumberRef& r_number)
{
    if (!__MCValueCreate(kMCValueTypeCodeNumber, r_number))
        return false;
    
    return true;
}

static inline void __MCNumberAssignDouble(__MCNumber& self, double p_value)
{
    self . flags |= kMCNumberFlagDoubleRep;
    self . real = p_value;
}

static inline void __MCNumberAssignSigned(__MCNumber& self, MCNumberSignedInteger p_value)
{
    // We don't set the flags here because this method is used immediately after
    // __MCNumberCreate so we are already signed rep.
    self . real = p_value;
}

static inline void __MCNumberAssignUnsigned(__MCNumber& self, MCNumberUnsignedInteger p_value)
{
    self . flags |= kMCNumberFlagUnsignedIntegerRep;
    self . real = p_value;
}

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

MCNumberSignedInteger MCNumberFetchAsInteger(MCNumberRef self)
{
    switch(__MCNumberGetRepType(self))
    {
        case __kMCNumberRepSignedInteger:
            return self -> integer;
        case __kMCNumberRepUnsignedInteger:
            return self -> unsigned_integer < kMCNumberSignedIntegerMax ? (MCNumberSignedInteger)self -> unsigned_integer : kMCNumberSignedIntegerMax;
        case __kMCNumberRepDouble:
            return self -> real < 0.0 ? (MCNumberSignedInteger)(self -> real - 0.5) : (MCNumberSignedInteger)(self -> real + 0.5);
        default:
            MCUnreachable();
            break;
    }
    
    return 0;
}

MCNumberUnsignedInteger MCNumberFetchAsUnsignedInteger(MCNumberRef self)
{
    switch(__MCNumberGetRepType(self))
    {
        case __kMCNumberRepSignedInteger:
            return self -> integer >= 0 ? (MCNumberUnsignedInteger)self -> integer : 0;
        case __kMCNumberRepUnsignedInteger:
            return self -> unsigned_integer;
        case __kMCNumberRepDouble:
            return self -> real < 0.0 ? 0.0 : (MCNumberUnsignedInteger)(self -> real + 0.5);
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
            return (real64_t)self -> integer;
        case __kMCNumberRepUnsignedInteger:
            return (real64_t)self -> unsigned_integer;
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
    
#if kMCNumberSignedIntegerMax <= INT32_MAX
    char *t_end;
    MCNumberSignedInteger t_integer;
    t_integer = strtol((const char *)t_chars, &t_end, 10);
    if (errno != ERANGE && *t_end == '\0')
        return MCNumberCreateWithInteger(t_integer, r_number);
    
    MCNumberUnsignedInteger t_uinteger;
    t_uinteger = strtoul((const char *)t_chars, &t_end, 10);
    if (errno != ERANGE && *t_end == '\0')
        return MCNumberCreateWithUnsignedInteger(t_uinteger, r_number);
#elif kMCNumberSignedIntegerMax <= INT64_MAX
    char *t_end;
    MCNumberSignedInteger t_integer;
    t_integer = strtoll((const char *)t_chars, &t_end, 10);
    if (errno != ERANGE && *t_end == '\0')
        return MCNumberCreateWithInteger(t_integer, r_number);
    
    MCNumberUnsignedInteger t_uinteger;
    t_uinteger = strtoull((const char *)t_chars, &t_end, 10);
    if (errno != ERANGE && *t_end == '\0')
        return MCNumberCreateWithUnsignedInteger(t_uinteger, r_number);
#else
#error "NOT SUPPORTED - TryToParse for int > 64-bit"
#endif
    
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
    switch(__MCNumberGetRepType(self))
    {
        case __kMCNumberRepSignedInteger:
            return MCStringFormat(r_string, kMCNumberSignedIntegerFormat, self -> integer);
        case __kMCNumberRepUnsignedInteger:
            return MCStringFormat(r_string, kMCNumberUnsignedIntegerFormat, self -> integer);
        case __kMCNumberRepDouble:
            return MCHashDouble(self -> real);
        default:
            MCUnreachable();
            break;
    }

    return false;
}

hash_t __MCNumberHash(__MCNumber *self)
{
    switch(__MCNumberGetRepType(self))
    {
        case __kMCNumberRepSignedInteger:
#if kMCNumberSignedIntegerMax <= INT32_MAX
            return MCHashInteger(self -> integer);
#elif kMCNumberSignedIntegerMax <= INT64_MAX
            return MCHashInteger((self -> integer & 0xFFFFFFFF) ^ (self -> integer >> 32));
#elif
#error "NOT SUPPORTED - Hash for int > 64-bit"
#endif
        case __kMCNumberRepUnsignedInteger:
#if kMCNumberSignedIntegerMax <= INT32_MAX
            return MCHashInteger((signed)self -> unsigned_integer);
#elif kMCNumberSignedIntegerMax <= INT64_MAX
            return MCHashInteger((signed)((self -> integer & 0xFFFFFFFF) ^ (self -> integer >> 32)));
#else
#error "NOT SUPPORTED - Hash for int > 64-bit"
#endif
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

#ifdef __clang__

#if kMCNumberSignedIntegerMax <= INT32_MAX

#define __checked_signed_add(x, y, z) __builtin_sadd_overflow((x), (y), (z))
#define __checked_signed_subtract(x, y, z) __builtin_ssub_overflow((x), (y), (z))
#define __checked_signed_multiply(x, y, z) __builtin_smul_overflow((x), (y), (z))
#define __checked_unsigned_add(x, y, z) __builtin_uadd_overflow((x), (y), (z))
#define __checked_unsigned_subtract(x, y, z) __builtin_usub_overflow((x), (y), (z))
#define __checked_unsigned_multiply(x, y, z) __builtin_umul_overflow((x), (y), (z))

#elif kMCNumberSignedIntegerMax <= INT64_MAX

#define __checked_signed_add(x, y, z) __builtin_saddll_overflow((x), (y), (z))
#define __checked_signed_subtract(x, y, z) __builtin_ssubll_overflow((x), (y), (z))
#define __checked_signed_multiply(x, y, z) __builtin_smulll_overflow((x), (y), (z))
#define __checked_unsigned_add(x, y, z) __builtin_uaddll_overflow((x), (y), (z))
#define __checked_unsigned_subtract(x, y, z) __builtin_usubll_overflow((x), (y), (z))
#define __checked_unsigned_multiply(x, y, z) __builtin_umulll_overflow((x), (y), (z))

#else

#error "NOT SUPPORTED - no clang builtins for int > 64-bit"

#endif

#else
static inline bool __checked_signed_add(MCNumberSignedInteger x, MCNumberSignedInteger y, MCNumberSignedInteger *z)
{
    if (((y > 0) && (x > (kMCNumberSignedIntegerMax - y))) ||
       ((y < 0) && (x < (kMCNumberSignedIntegerMin - y))))
        return false;
    
    *z = x + y;

    return true;
}

static inline bool __checked_signed_subtract(MCNumberSignedInteger x, MCNumberSignedInteger y, MCNumberSignedInteger *z)
{
    if ((y > 0 && x < kMCNumberSignedIntegerMin + y) ||
        (y < 0 && x > kMCNumberSignedIntegerMax + y))
        return false;
    
    *z = x - y;
    
    return true;
}

static inline bool __checked_signed_multiply(MCNumberSignedInteger x, MCNumberSignedInteger y, MCNumberSignedInteger *z)
{
#if kMCNumberSignedIntegerMax <= INT32_MAX
    int64_t w;
    w = (int64_t)x * (int64_t)y;
    if ((w > INT32_MAX) || (w < INT32_MIN))
        return false;
#else
    MCNumberSignedInteger w;
    w = x * y;
    if (y != 0 && w / y != x)
        return false;
#endif
    
    *z = (MCNumberSignedInteger)w;
    
    return true;
}

static inline bool __checked_unsigned_add(MCNumberUnsignedInteger x, MCNumberUnsignedInteger y, MCNumberUnsignedInteger *z)
{
    if (kMCNumberUnsignedIntegerMax - x < y)
        return false;
    
    *z = x + y;
    
    return true;
}

static inline bool __checked_unsigned_subtract(MCNumberUnsignedInteger x, MCNumberUnsignedInteger y, MCNumberUnsignedInteger *z)
{
    if (x < y)
        return false;
    
    *z = x - y;
    
    return true;
}

static inline bool __checked_unsigned_multiply(MCNumberUnsignedInteger x, MCNumberUnsignedInteger y, MCNumberUnsignedInteger *z)
{
#if kMCNumberSignedIntegerMax <= INT32_MAX
    uint64_t w;
    w = (uint64_t)x * (uint64_t)y;
    if (w > UINT32_MAX)
        return false;
#else
    MCNumberUnsignedInteger w;
    w = x * y;
    if (y != 0 && w / y != x)
        return false;
#endif
    
    *z = (MCNumberUnsignedInteger)w;
    
    return true;
}
#endif

// Returns the unsigned value which is the negation of the signed value x.
// Note that x is known to be strictly negative.
static inline MCNumberUnsignedInteger __negate_negative_signed(MCNumberSignedInteger x)
{
    return ((MCNumberUnsignedInteger)~x) + 1;
}

static inline double __flooring_real_div(double x, double y)
{
    return floor(x / y);
}

static inline double __flooring_real_mod(double x, double y)
{
    return x - floor(x / y) * y;
}

// C's % operator isn't quite what we want as the sign of the remainder
// is undefined if either x or y are negative.
template<typename T> inline T __flooring_integral_mod(T x, T y)
{
    if (x < y || y < 0)
        return x - (x / y) * y;
    return x % y;
}

////////////////////////////////////////////////////////////////////////////////

// The MCNumber operation implementation is optimized for working with the same
// data type. Other codepaths will be slower.

enum __MCNumberBinaryOpType
{
    __kMCNumberBinaryOpTypeSignedIntegerBySignedInteger = (kMCNumberFlagSignedIntegerRep << kMCNumberFlagRepBitCount) | kMCNumberFlagSignedIntegerRep,
    __kMCNumberBinaryOpTypeSignedIntegerByUnsignedInteger = (kMCNumberFlagSignedIntegerRep << kMCNumberFlagRepBitCount) | kMCNumberFlagUnsignedIntegerRep,
    __kMCNumberBinaryOpTypeSignedIntegerByDouble = (kMCNumberFlagSignedIntegerRep << kMCNumberFlagRepBitCount) | kMCNumberFlagDoubleRep,
    __kMCNumberBinaryOpTypeUnsignedIntegerBySignedInteger = (kMCNumberFlagUnsignedIntegerRep << kMCNumberFlagRepBitCount) | kMCNumberFlagSignedIntegerRep,
    __kMCNumberBinaryOpTypeUnsignedIntegerByUnsignedInteger = (kMCNumberFlagUnsignedIntegerRep << kMCNumberFlagRepBitCount) | kMCNumberFlagUnsignedIntegerRep,
    __kMCNumberBinaryOpTypeUnsignedIntegerByDouble = (kMCNumberFlagUnsignedIntegerRep << kMCNumberFlagRepBitCount) | kMCNumberFlagDoubleRep,
    __kMCNumberBinaryOpTypeDoubleBySignedInteger = (kMCNumberFlagDoubleRep << kMCNumberFlagRepBitCount) | kMCNumberFlagSignedIntegerRep,
    __kMCNumberBinaryOpTypeDoubleByUnsignedInteger = (kMCNumberFlagDoubleRep << kMCNumberFlagRepBitCount) | kMCNumberFlagUnsignedIntegerRep,
    __kMCNumberBinaryOpTypeDoubleByDouble = (kMCNumberFlagDoubleRep << kMCNumberFlagRepBitCount) | kMCNumberFlagDoubleRep,
};

static inline __MCNumberBinaryOpType __MCNumberComputeBinaryType(MCNumberRef x, MCNumberRef y)
{
    return (__MCNumberBinaryOpType)(((x -> flags & kMCNumberFlagRepMask) << kMCNumberFlagRepBitCount) | (y -> flags & kMCNumberFlagRepMask));
}

static inline bool __MCNumberBinaryOpIsIntegral(__MCNumberBinaryOpType t)
{
    return (t & ((kMCNumberFlagDoubleRep << kMCNumberFlagRepBitCount) |
                 kMCNumberFlagDoubleRep)) == 0;
}

//////////

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

template<typename T> inline bool __MCNumberComparisonOperation(MCNumberRef x, MCNumberRef y)
{
    switch(__MCNumberComputeBinaryType(x, y))
    {
        case __kMCNumberBinaryOpTypeSignedIntegerBySignedInteger:
            return T::signed_by_signed(x -> integer, y -> integer);
        case __kMCNumberBinaryOpTypeSignedIntegerByUnsignedInteger:
            return T::signed_by_unsigned(x -> integer, y -> unsigned_integer);
        case __kMCNumberBinaryOpTypeSignedIntegerByDouble:
            return T::double_by_double(x -> integer, y -> real);
        case __kMCNumberBinaryOpTypeUnsignedIntegerBySignedInteger:
            return T::unsigned_by_signed(x -> unsigned_integer, y -> integer);
        case __kMCNumberBinaryOpTypeUnsignedIntegerByUnsignedInteger:
            return T::unsigned_by_unsigned(x -> unsigned_integer, y -> unsigned_integer);
        case __kMCNumberBinaryOpTypeUnsignedIntegerByDouble:
            return T::double_by_double(x -> unsigned_integer, y -> real);
        case __kMCNumberBinaryOpTypeDoubleBySignedInteger:
            return T::double_by_double(x -> real, y -> integer);
        case __kMCNumberBinaryOpTypeDoubleByUnsignedInteger:
            return T::double_by_double(x -> real, y -> unsigned_integer);
        case __kMCNumberBinaryOpTypeDoubleByDouble:
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
            if (x -> integer == kMCNumberSignedIntegerMin)
                return MCNumberCreateWithUnsignedInteger(__negate_negative_signed(kMCNumberSignedIntegerMin), r_y);
            return MCNumberCreateWithInteger(-x -> integer, r_y);
            
        case __kMCNumberRepUnsignedInteger:
            if (x -> unsigned_integer > __negate_negative_signed(kMCNumberSignedIntegerMin))
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

#define LIKELY(x) (x)

////

static inline bool double_add(double x, double y, __MCNumber& r_z)
{
    __MCNumberAssignDouble(r_z, x + y);
    return true;
}

static inline bool unsigned_add(MCNumberUnsignedInteger x, MCNumberUnsignedInteger y, __MCNumber& r_z)
{
    MCNumberUnsignedInteger uz;
    if (LIKELY(__checked_unsigned_add(x, y, &uz)))
    {
        __MCNumberAssignUnsigned(r_z, uz);
        return true;
    }
    
    return false;
}

static inline bool signed_add(MCNumberSignedInteger x, MCNumberSignedInteger y, __MCNumber& r_z)
{
    MCNumberSignedInteger sz;
    if (LIKELY(__checked_signed_add(x, y, &sz)))
    {
        __MCNumberAssignUnsigned(r_z, sz);
        return true;
    }
    
    // If adding two signed numbers together fails, then the only case which
    // can succeed is if they are both non-negative.
    if (LIKELY(x >= 0 && y >= 0))
        return unsigned_add(x, y, r_z);
    
    return false;
}

////

static inline bool double_subtract(double x, double y, __MCNumber& r_z)
{
    __MCNumberAssignDouble(r_z, x - y);
    return true;
}

static inline bool unsigned_subtract(MCNumberUnsignedInteger x, MCNumberUnsignedInteger y, __MCNumber& r_z)
{
    MCNumberUnsignedInteger uz;
    if (LIKELY(__checked_unsigned_subtract(x, y, &uz)))
    {
        __MCNumberAssignUnsigned(r_z, uz);
        return true;
    }
    
    // If the above failed, then x < y.
    uz = y - x;
    if (LIKELY(uz <= __negate_negative_signed(kMCNumberSignedIntegerMin)))
    {
        __MCNumberAssignSigned(r_z, -uz);
        return true;
    }
    
    return false;
}

static inline bool signed_subtract(MCNumberSignedInteger x, MCNumberSignedInteger y, __MCNumber& r_z)
{
    // If x is >= 0 and y is >= 0 then there can be no overflow
    // If x is >= 0 and y is < 0 then there could be overflow but it will be representable
    // If x is < 0 and y is >= 0 then if there is overflow it is unrepresentable
    // If x is < 0 and y is < 0 then if there is overflow it is representable
    MCNumberSignedInteger sz;
    if (LIKELY(__checked_signed_subtract(x, y, &sz)))
    {
        __MCNumberAssignSigned(r_z, sz);
        return true;
    }
    
    // If x >= 0 then y < 0.
    if (LIKELY(x >= 0))
        return unsigned_add(x, __negate_negative_signed(y), r_z);
    
    // If y < 0 then x < 0
    if (LIKELY(y < 0))
        return unsigned_subtract(__negate_negative_signed(y), __negate_negative_signed(x), r_z);
        
    return false;
}

////

static inline bool double_multiply(double x, double y, __MCNumber& r_z)
{
    __MCNumberAssignDouble(r_z, x * y);
    return true;
}

static inline bool unsigned_multiply(MCNumberUnsignedInteger x, MCNumberUnsignedInteger y, __MCNumber& r_z)
{
    MCNumberUnsignedInteger uz;
    if (LIKELY(__checked_unsigned_multiply(x, y, &uz)))
    {
        __MCNumberAssignUnsigned(r_z, uz);
        return true;
    }

    return false;
}

static inline bool signed_multiply(MCNumberSignedInteger x, MCNumberSignedInteger y, __MCNumber& r_z)
{
    MCNumberSignedInteger sz;
    if (LIKELY(__checked_signed_multiply(x, y, &sz)))
    {
        __MCNumberAssignSigned(r_z, sz);
        return true;
    }
    
    if (LIKELY(x >= 0 && y >= 0))
        return unsigned_multiply(x, y, r_z);
    
    if (LIKELY(x < 0 && y < 0))
        return unsigned_multiply(__negate_negative_signed(x), __negate_negative_signed(y), r_z);
    
    return false;
}

////

static inline bool double_divide(double x, double y, __MCNumber& r_z)
{
    __MCNumberAssignDouble(r_z, x / y);
    return true;
}

////

static inline bool double_div(double x, double y, __MCNumber& r_z)
{
    __MCNumberAssignDouble(r_z, __flooring_real_div(x, y));
    return true;
}

// This method computes the correct result regardless of the range of x and y.
static inline bool any_unsigned_div(MCNumberUnsignedInteger x, MCNumberUnsignedInteger y, __MCNumber& r_z)
{
    // div by zero is unrepresentable
    if (y == 0)
        return false;
    
    MCNumberUnsignedInteger z;
    z = x / y;
    
    if (LIKELY(z <= kMCNumberSignedIntegerMax))
        __MCNumberAssignSigned(r_z, z);
    else
        __MCNumberAssignUnsigned(r_z, z);
    
    return true;
}

static inline bool signed_div(MCNumberSignedInteger x, MCNumberSignedInteger y, __MCNumber& r_z)
{
    // div by zero is unrepresentable
    if (y == 0)
        return false;
    
    // INT*_MIN/-1 is positive and representable as unsigned
    if (y == -1 && x == kMCNumberSignedIntegerMin)
    {
        __MCNumberAssignUnsigned(r_z, __negate_negative_signed(kMCNumberSignedIntegerMin));
        return true;
    }
    
    // abs(x / y) <= x and we've handled the overflow case so representable.
    __MCNumberAssignSigned(r_z, x / y);
    
    return true;
}

static inline bool unsigned_div_signed(MCNumberUnsignedInteger x, MCNumberSignedInteger y, __MCNumber& r_z)
{
    // If y > -1 then result is only representable if x == INTEGER_MAX.
    if (y == -1)
    {
        if (LIKELY(x == INTEGER_MAX))
        {
            __MCNumberAssignSigned(r_z, -INTEGER_MAX);
            return true;
        }
        
        return false;
    }
    
    // x >= 0 and y >= 0 means we have an unsigned division
    if (LIKELY(y >= 0))
        return any_unsigned_div(x, (MCNumberUnsignedInteger)y, r_z);
    
    // Otherwise we have negation of unsigned division.
    __MCNumberAssignSigned(r_z, -(MCNumberSignedInteger)(x / (MCNumberUnsignedInteger)y));
    
    return true;
}

////

static inline bool double_mod(double x, double y, __MCNumber& r_z)
{
    __MCNumberAssignDouble(r_z, __flooring_real_mod(x, y));
    return true;
}

// This method computes the correct result regardless of the range of x and y.
static inline bool any_unsigned_mod(MCNumberUnsignedInteger x, MCNumberUnsignedInteger y, __MCNumber& r_z)
{
    MCNumberUnsignedInteger z;
    z = x % y;
    if (LIKELY(z <= kMCNumberSignedIntegerMax))
        __MCNumberAssignSigned(r_z, z);
    else
        __MCNumberAssignUnsigned(r_z, z);
    
    return true;
}

static inline bool signed_mod(MCNumberSignedInteger x, MCNumberSignedInteger y, __MCNumber& r_z)
{
    __MCNumberAssignSigned(r_z, __flooring_integral_mod(x, y));
    return true;
}

static inline bool unsigned_mod_signed(MCNumberUnsignedInteger x, MCNumberSignedInteger y, __MCNumber& r_z)
{
    if (LIKELY(y >= 0))
        return any_unsigned_mod(x, (MCNumberUnsignedInteger)y, r_z);
    
    MCNumberUnsignedInteger z;
    z = x % (-y);
    if (LIKELY(z <= kMCNumberSignedIntegerMax))
        __MCNumberAssignSigned(r_z, z);
    else
        __MCNumberAssignUnsigned(r_z, z);
 
    return true;
}

////////////////////////////////////////////////////////////////////////////////

#define ensure_forceinline __attribute__((always_inline))

// The real operators can be templated since they are uniform - promote values
// to real and perform the operation.
template<bool (*OPERATOR)(double, double, __MCNumber&)> static ensure_forceinline bool __MCNumberTryToRealOp(__MCNumberBinaryOpType t, MCNumberRef x, MCNumberRef y, MCNumberRef& r_z)
{
    switch(__MCNumberComputeBinaryType(x, y))
    {
        case __kMCNumberBinaryOpTypeSignedIntegerBySignedInteger:
            return OPERATOR(x -> integer, y -> integer, *r_z);
            
        case __kMCNumberBinaryOpTypeSignedIntegerByUnsignedInteger:
            return OPERATOR(x -> integer, y -> unsigned_integer, *r_z);
            
        case __kMCNumberBinaryOpTypeUnsignedIntegerBySignedInteger:
            return OPERATOR(x -> unsigned_integer, y -> integer, *r_z);
            
        case __kMCNumberBinaryOpTypeUnsignedIntegerByUnsignedInteger:
            return OPERATOR(x -> unsigned_integer, y -> integer, *r_z);
            
        case __kMCNumberBinaryOpTypeSignedIntegerByDouble:
            return OPERATOR(x -> integer, y -> real, *r_z);
            
        case __kMCNumberBinaryOpTypeUnsignedIntegerByDouble:
            return OPERATOR(x -> unsigned_integer, y -> real, *r_z);
            
        case __kMCNumberBinaryOpTypeDoubleBySignedInteger:
            return OPERATOR(x -> real, y -> integer, *r_z);
            
        case __kMCNumberBinaryOpTypeDoubleByUnsignedInteger:
            return OPERATOR(x -> real, y -> unsigned_integer, *r_z);
            
        case __kMCNumberBinaryOpTypeDoubleByDouble:
            return OPERATOR(x -> real, y -> real, *r_z);
            
        default:
            MCUnreachable();
            break;
    }
    
    return false;
}

// The integer operators need per-operation code as we have to deal with two
// integer representations (signed and unsigned). These operations should only
// be called if t is a binary integer op.

static ensure_forceinline bool __MCNumberTryToIntegerAdd(__MCNumberBinaryOpType t, MCNumberRef x, MCNumberRef y, MCNumberRef& r_z)
{
    switch(__MCNumberComputeBinaryType(x, y))
    {
        case __kMCNumberBinaryOpTypeSignedIntegerBySignedInteger:
            if (LIKELY(signed_add(x -> integer, y -> integer, *r_z)))
                return true;
            
            return false;
            
        case __kMCNumberBinaryOpTypeSignedIntegerByUnsignedInteger:
            if (LIKELY(x -> integer >= 0))
            {
                if (LIKELY(unsigned_add(x -> integer, y -> unsigned_integer, *r_z)))
                    return true;
            }
            else
            {
                if (LIKELY(unsigned_subtract(y -> integer, __negate_negative_signed(x -> integer), *r_z)))
                    return true;
            }
            
            return false;
            
        case __kMCNumberBinaryOpTypeUnsignedIntegerBySignedInteger:
            if (LIKELY(y -> integer >= 0))
            {
                if (LIKELY(unsigned_add(x -> unsigned_integer, y -> integer, *r_z)))
                    return true;
            }
            else
            {
                if (LIKELY(unsigned_subtract(x -> unsigned_integer, __negate_negative_signed(x -> integer), *r_z)))
                    return true;
            }
            
            return false;
            
        case __kMCNumberBinaryOpTypeUnsignedIntegerByUnsignedInteger:
            if (LIKELY(unsigned_add(x -> unsigned_integer, y -> unsigned_integer, *r_z)))
                return true;
            
            return false;
            
        default:
            MCUnreachable();
            break;
    }
    
    return false;
}

static ensure_forceinline bool __MCNumberTryToIntegerSubtract(__MCNumberBinaryOpType t, MCNumberRef x, MCNumberRef y, MCNumberRef& r_z)
{
    switch(t)
    {
        case __kMCNumberBinaryOpTypeSignedIntegerBySignedInteger:
            if (LIKELY(signed_subtract(x -> integer, y -> integer, *r_z)))
                return true;
            
            return false;
            
        case __kMCNumberBinaryOpTypeSignedIntegerByUnsignedInteger:
            if (LIKELY(x -> integer >= 0))
            {
                if (LIKELY(unsigned_subtract(x -> integer, y -> unsigned_integer, *r_z)))
                    return true;
            }
            
            return false;
            
        case __kMCNumberBinaryOpTypeUnsignedIntegerBySignedInteger:
            if (LIKELY(y -> integer >= 0))
            {
                if (LIKELY(unsigned_subtract(x -> unsigned_integer, y -> integer, *r_z)))
                    return true;
            }
            else
            {
                if (LIKELY(unsigned_add(x -> unsigned_integer, __negate_negative_signed(y -> integer), *r_z)))
                    return true;
            }
            
            return false;
            
        case __kMCNumberBinaryOpTypeUnsignedIntegerByUnsignedInteger:
            if (LIKELY(unsigned_subtract(x -> unsigned_integer, y -> unsigned_integer, *r_z)))
                return true;
            
            return false;
            
        default:
            MCUnreachable();
            break;
    }
    
    return false;
}

static ensure_forceinline bool  __MCNumberTryToIntegerMultiply(__MCNumberBinaryOpType t, MCNumberRef x, MCNumberRef y, MCNumberRef& r_z)
{
    switch(t)
    {
        case __kMCNumberBinaryOpTypeSignedIntegerBySignedInteger:
            if (LIKELY(signed_multiply(x -> integer, y -> integer, *r_z)))
                return true;
            
            if (x -> integer >= 0 && y -> integer >= 0)
            {
                if (LIKELY(unsigned_multiply(x -> integer, y -> integer, *r_z)))
                    return true;
            }
            else if (x -> integer < 0 && y -> integer < 0)
            {
                if (LIKELY(unsigned_multiply(__negate_negative_signed(x -> integer), __negate_negative_signed(y -> integer), *r_z)))
                    return true;
            }
            
            return false;
            
        case __kMCNumberBinaryOpTypeSignedIntegerByUnsignedInteger:
            if (LIKELY(x -> integer >= 0))
            {
                if (LIKELY(unsigned_multiply(x -> integer, y -> unsigned_integer, *r_z)))
                    return true;
            }
            
            return false;
            
        case __kMCNumberBinaryOpTypeUnsignedIntegerBySignedInteger:
            if (LIKELY(y -> integer >= 0))
            {
                if (LIKELY(unsigned_multiply(x -> unsigned_integer, y -> integer, *r_z)))
                    return true;
            }
            
            return false;
            
        case __kMCNumberBinaryOpTypeUnsignedIntegerByUnsignedInteger:
            if (LIKELY(unsigned_multiply(x -> unsigned_integer, y -> unsigned_integer, *r_z)))
                return true;
            
            return false;
            
        default:
            MCUnreachable();
            break;
    }
    
    return false;
}

static ensure_forceinline bool __MCNumberTryToIntegerDiv(__MCNumberBinaryOpType t, MCNumberRef x, MCNumberRef y, MCNumberRef& r_z)
{
    switch(__MCNumberComputeBinaryType(x, y))
    {
        case __kMCNumberBinaryOpTypeSignedIntegerBySignedInteger:
            if (LIKELY(signed_div(x -> integer, y -> integer, *r_z)))
                return true;
            
            return false;
            
        case __kMCNumberBinaryOpTypeSignedIntegerByUnsignedInteger:
            // Unsigneds > signeds so result is always zero which is what
            // r_z is on input.
            return false;
            
        case __kMCNumberBinaryOpTypeUnsignedIntegerBySignedInteger:
            if (LIKELY(unsigned_div_signed(x -> unsigned_integer, y -> integer, *r_z)))
                return true;
            
            return false;
            
        case __kMCNumberBinaryOpTypeUnsignedIntegerByUnsignedInteger:
            if (LIKELY(any_unsigned_div(x -> unsigned_integer, y -> unsigned_integer, *r_z)))
                return true;
            
            return false;

        default:
            MCUnreachable();
            break;
    }
    
    return false;
}

static ensure_forceinline bool __MCNumberTryToIntegerMod(__MCNumberBinaryOpType t, MCNumberRef x, MCNumberRef y, MCNumberRef& r_z)
{
    switch(__MCNumberComputeBinaryType(x, y))
    {
        case __kMCNumberBinaryOpTypeSignedIntegerBySignedInteger:
            if (LIKELY(signed_mod(x -> integer, y -> integer, *r_z)))
                return true;
            
            // We only get here is y -> integer == 0.
            return false;
            
        case __kMCNumberBinaryOpTypeSignedIntegerByUnsignedInteger:
            // Unsigneds > signeds so result is always zero which is what
            // r_z is on input.
            return true;
            
        case __kMCNumberBinaryOpTypeUnsignedIntegerBySignedInteger:
            if (LIKELY(unsigned_mod_signed(x -> unsigned_integer, y -> integer, *r_z)))
                return true;
            
            return false;
            
        case __kMCNumberBinaryOpTypeUnsignedIntegerByUnsignedInteger:
            if (LIKELY(any_unsigned_mod(x -> unsigned_integer, y -> unsigned_integer, *r_z)))
                return true;
            
            return false;
            
        default:
            MCUnreachable();
            break;
    }
    
    return false;
}

////////////////////////////////////////////////////////////////////////////////

template
<
    bool (*INTEGER_OP)(__MCNumberBinaryOpType, MCNumberRef, MCNumberRef, MCNumberRef&),
    bool (*REAL_OP)(__MCNumberBinaryOpType, MCNumberRef, MCNumberRef, MCNumberRef&)
>
static ensure_forceinline bool __MCNumberPromotingOp(MCNumberRef x, MCNumberRef y, MCNumberRef& r_z)
{
    // Create a raw number this will be initialize to 0 signed int.
    MCNumberRef z;
    if (!__MCNumberCreate(z))
        return false;
    
    // If the op is bi-integral, then first try to do it in integers.
    __MCNumberBinaryOpType t_op;
    t_op = __MCNumberComputeBinaryType(x, y);
    if (LIKELY(__MCNumberBinaryOpIsIntegral(t_op)))
    {
        if (LIKELY(INTEGER_OP(t_op, x, y, z)))
            return r_z = z, true;
    }
    
    // If that failed, now try to to do it after promotion to reals.
    if (LIKELY(REAL_OP(t_op, x, y, z)))
        return r_z = z, true;
    
    // Failure so clean up.
    MCValueRelease(z);
    
    return false;
}

template
<
    bool (*REAL_OP)(__MCNumberBinaryOpType, MCNumberRef, MCNumberRef, MCNumberRef&)
>
static ensure_forceinline bool __MCNumberRealOp(MCNumberRef x, MCNumberRef y, MCNumberRef& r_z)
{
    // Create a raw number this will be initialize to 0 signed int.
    MCNumberRef z;
    if (!__MCNumberCreate(z))
        return false;
    
    // Attempt the operation.
    if (LIKELY(REAL_OP(__MCNumberComputeBinaryType(x, y), x, y, z)))
        return r_z = z, true;
    
    // Failure so clean up.
    MCValueRelease(z);
    
    return false;
    
}

template
<
bool (*INTEGER_OP)(__MCNumberBinaryOpType, MCNumberRef, MCNumberRef, MCNumberRef&)
>
static ensure_forceinline bool __MCNumberIntegerOp(MCNumberRef x, MCNumberRef y, MCNumberRef& r_z)
{
    // Create a raw number this will be initialize to 0 signed int.
    MCNumberRef z;
    if (!__MCNumberCreate(z))
        return false;
    
    // Attempt the operation.
    if (LIKELY(INTEGER_OP(__MCNumberComputeBinaryType(x, y), x, y, z)))
        return r_z = z, true;

    // Failure so clean up.
    MCValueRelease(z);
    
    return __MCNumberThrowOverflowError();
}


bool MCNumberAdd(MCNumberRef x, MCNumberRef y, MCNumberRef& r_z)
{
    return __MCNumberPromotingOp< __MCNumberTryToIntegerAdd, __MCNumberTryToRealOp<double_add> >(x, y, r_z);
}

bool MCNumberSubtract(MCNumberRef x, MCNumberRef y, MCNumberRef& r_z)
{
    return __MCNumberPromotingOp< __MCNumberTryToIntegerSubtract, __MCNumberTryToRealOp<double_subtract> >(x, y, r_z);
}

bool MCNumberMultiply(MCNumberRef x, MCNumberRef y, MCNumberRef& r_z)
{
    return __MCNumberPromotingOp< __MCNumberTryToIntegerMultiply, __MCNumberTryToRealOp<double_multiply> >(x, y, r_z);
}

bool MCNumberDivide(MCNumberRef x, MCNumberRef y, MCNumberRef& r_z)
{
    return __MCNumberRealOp<__MCNumberTryToRealOp<double_divide> >(x, y, r_z);
    
}

bool MCNumberDiv(MCNumberRef x, MCNumberRef y, MCNumberRef& r_z)
{
    return __MCNumberPromotingOp< __MCNumberTryToIntegerDiv, __MCNumberTryToRealOp<double_div> >(x, y, r_z);
}

bool MCNumberMod(MCNumberRef x, MCNumberRef y, MCNumberRef& r_z)
{
    return __MCNumberPromotingOp< __MCNumberTryToIntegerMod, __MCNumberTryToRealOp<double_mod> >(x, y, r_z);
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
        if (MCNumberFetchAsReal(y) != 0.0)
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
        if (MCNumberFetchAsReal(y) != 0.0)
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

bool MCNumberRealNegate(MCNumberRef x, MCNumberRef& r_y)
{
    return MCNumberCreateWithReal(-x -> real, r_y);
}

bool MCNumberRealAdd(MCNumberRef x, MCNumberRef y, MCNumberRef& r_z)
{
    return MCNumberCreateWithReal(x -> real + y -> real, r_z);
}

bool MCNumberRealSubtract(MCNumberRef x, MCNumberRef y, MCNumberRef& r_z)
{
    return MCNumberCreateWithReal(x -> real - y -> real, r_z);
}

bool MCNumberRealMultiply(MCNumberRef x, MCNumberRef y, MCNumberRef& r_z)
{
    return MCNumberCreateWithReal(x -> real * y -> real, r_z);
}

bool MCNumberRealDivide(MCNumberRef x, MCNumberRef y, MCNumberRef& r_z)
{
    return MCNumberCreateWithReal(x -> real / y -> real, r_z);
}

bool MCNumberRealDiv(MCNumberRef x, MCNumberRef y, MCNumberRef& r_z)
{
    return MCNumberCreateWithReal(__flooring_real_div(x -> real, y -> real), r_z);
}

bool MCNumberRealMod(MCNumberRef x, MCNumberRef y, MCNumberRef& r_z)
{
    return MCNumberCreateWithReal(__flooring_real_mod(x -> real, y -> real), r_z);
}

bool MCNumberRealIsEqualTo(MCNumberRef x, MCNumberRef y)
{
    return x -> real == y -> real;
}

bool MCNumberRealIsNotEqualTo(MCNumberRef x, MCNumberRef y)
{
    return x -> real != y -> real;
}

bool MCNumberRealIsLessThan(MCNumberRef x, MCNumberRef y)
{
    return x -> real < y -> real;
}

bool MCNumberRealIsLessThanOrEqualTo(MCNumberRef x, MCNumberRef y)
{
    return x -> real <= y -> real;
}

bool MCNumberRealIsGreaterThan(MCNumberRef x, MCNumberRef y)
{
    return x -> real > y -> real;
}

bool MCNumberRealIsGreaterThanOrEqualTo(MCNumberRef x, MCNumberRef y)
{
    return x -> real >= y -> real;
}

//////////

bool MCNumberFiniteRealNegate(MCNumberRef x, MCNumberRef& r_y)
{
    MCNumberRef y;
    if (!MCNumberRealNegate(x, y))
        return false;
    
    if (!MCNumberIsFinite(y))
        return __MCNumberThrowOverflowError();
    
    r_y = y;
    
    return true;
}

bool MCNumberFiniteRealAdd(MCNumberRef x, MCNumberRef y, MCNumberRef& r_z)
{
    MCNumberRef z;
    if (!MCNumberRealAdd(x, y, z))
        return false;
    
    if (!MCNumberIsFinite(z))
        return __MCNumberThrowOverflowError();
    
    r_z = z;
    
    return true;
    
}

bool MCNumberFiniteRealSubtract(MCNumberRef x, MCNumberRef y, MCNumberRef& r_z)
{
    MCNumberRef z;
    if (!MCNumberRealSubtract(x, y, z))
        return false;
    
    if (!MCNumberIsFinite(z))
        return __MCNumberThrowOverflowError();
    
    r_z = z;
    
    return true;
}

bool MCNumberFiniteRealMultiply(MCNumberRef x, MCNumberRef y, MCNumberRef& r_z)
{
    MCNumberRef z;
    if (!MCNumberRealMultiply(x, y, z))
        return false;
    
    if (!MCNumberIsFinite(z))
        return __MCNumberThrowOverflowError();
    
    r_z = z;
    
    return true;
}

bool MCNumberFiniteRealDivide(MCNumberRef x, MCNumberRef y, MCNumberRef& r_z)
{
    MCNumberRef z;
    if (!MCNumberRealDivide(x, y, z))
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

bool MCNumberFiniteRealDiv(MCNumberRef x, MCNumberRef y, MCNumberRef& r_z)
{
    MCNumberRef z;
    if (!MCNumberRealDiv(x, y, z))
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

bool MCNumberFiniteRealMod(MCNumberRef x, MCNumberRef y, MCNumberRef& r_z)
{
    MCNumberRef z;
    if (!MCNumberRealMod(x, y, z))
        return false;
    
    if (!MCNumberIsFinite(z))
        return __MCNumberThrowOverflowError();
    
    r_z = z;
    
    return true;
}

////////////////////////////////////////////////////////////////////////////////

template<typename T> inline bool __MCNumberIntegerComparisonOperation(MCNumberRef x, MCNumberRef y)
{
    switch(__MCNumberComputeBinaryType(x, y))
    {
        case __kMCNumberBinaryOpTypeSignedIntegerBySignedInteger:
            return T::signed_by_signed(x -> integer, y -> integer);
        case __kMCNumberBinaryOpTypeSignedIntegerByUnsignedInteger:
            return T::signed_by_unsigned(x -> integer, y -> unsigned_integer);
        case __kMCNumberBinaryOpTypeUnsignedIntegerBySignedInteger:
            return T::unsigned_by_signed(x -> unsigned_integer, y -> integer);
        case __kMCNumberBinaryOpTypeUnsignedIntegerByUnsignedInteger:
            return T::unsigned_by_unsigned(x -> unsigned_integer, y -> unsigned_integer);
        default:
            MCUnreachable();
            break;
    }
    
    return false;
}

bool MCNumberIntegerNegate(MCNumberRef x, MCNumberRef& r_y)
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

bool MCNumberIntegerAdd(MCNumberRef x, MCNumberRef y, MCNumberRef& r_z)
{
    return __MCNumberIntegerOp<__MCNumberTryToIntegerAdd>(x, y, r_z);
}

bool MCNumberIntegerSubtract(MCNumberRef x, MCNumberRef y, MCNumberRef& r_z)
{
    return __MCNumberIntegerOp<__MCNumberTryToIntegerSubtract>(x, y, r_z);
}

bool MCNumberIntegerMultiply(MCNumberRef x, MCNumberRef y, MCNumberRef& r_z)
{
    return __MCNumberIntegerOp<__MCNumberTryToIntegerMultiply>(x, y, r_z);
}

bool MCNumberIntegerDiv(MCNumberRef x, MCNumberRef y, MCNumberRef& r_z)
{
    // Create a raw number this will be initialize to 0 signed int.
    MCNumberRef z;
    if (!__MCNumberCreate(z))
        return false;
    
    // Attempt the operation.
    if (LIKELY(__MCNumberTryToIntegerDiv(__MCNumberComputeBinaryType(x, y), x, y, z)))
        return r_z = z, true;
    
    // Failure so clean up.
    MCValueRelease(z);
    
    if (MCNumberFetchAsInteger(y) == 0)
        return __MCNumberThrowDivisionByZeroError();
    
    return __MCNumberThrowOverflowError();
}

bool MCNumberIntegerMod(MCNumberRef x, MCNumberRef y, MCNumberRef& r_z)
{
    return __MCNumberIntegerOp<__MCNumberTryToIntegerMod>(x, y, r_z);
}

bool MCNumberIntegerIsEqualTo(MCNumberRef x, MCNumberRef y)
{
    return __MCNumberIntegerComparisonOperation<__MCNumberOperationIsEqualTo>(x, y);
}

bool MCNumberIntegerIsNotEqualTo(MCNumberRef x, MCNumberRef y)
{
    return __MCNumberIntegerComparisonOperation<__MCNumberOperationIsNotEqualTo>(x, y);
}

bool MCNumberIntegerIsLessThan(MCNumberRef x, MCNumberRef y)
{
    return __MCNumberIntegerComparisonOperation<__MCNumberOperationIsLessThan>(x, y);
}

bool MCNumberIntegerIsLessThanOrEqualTo(MCNumberRef x, MCNumberRef y)
{
    return __MCNumberIntegerComparisonOperation<__MCNumberOperationIsLessThanOrEqualTo>(x, y);
}

bool MCNumberIntegerIsGreaterThan(MCNumberRef x, MCNumberRef y)
{
    return __MCNumberIntegerComparisonOperation<__MCNumberOperationIsGreaterThan>(x, y);
}

bool MCNumberIntegerIsGreaterThanOrEqualTo(MCNumberRef x, MCNumberRef y)
{
    return __MCNumberIntegerComparisonOperation<__MCNumberOperationIsGreaterThanOrEqualTo>(x, y);
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
