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
            return MCHashInt32(self -> integer);
#elif kMCNumberSignedIntegerMax <= INT64_MAX
            return MCHashInt64(self -> integer);
#elif
#error "NOT SUPPORTED - Hash for int > 64-bit"
#endif
        case __kMCNumberRepUnsignedInteger:
#if kMCNumberSignedIntegerMax <= INT32_MAX
            return MCHashUInt32(self -> unsigned_integer);
#elif kMCNumberSignedIntegerMax <= INT64_MAX
            return MCHashUInt64(self -> unsigned_integer);
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

//////////

// This type must be big enough to be able to hold the result of any arithmetic
// binary operation performed on a combination of MCNumberSignedInteger and
// MCNumberUnsignedInteger.
#if kMCNumberSignedIntegerMax <= INT32_MAX
typedef int64_t bigint_t;
#elif kMCNumberSignedIntegerMax <= INT64_MAX
struct bigint_t
{
    int64_t lo;
    int64_t hi;
};

// -x == ~x + 1
operator bigint_t operator - (bigint_t x)
{
    bigint_t r;
    r . lo = ~x . lo;
    r . hi = ~x . hi;
    
    r . lo += 1;
    if (r . lo == 0)
        r . hi += 1;
    
    return r;
}

operator bigint_t operator + (bigint_t x, bigint_t y)
{
    bigint_t r;
    r = x;
    r . lo += y . lo;
    r . hi += y . hi;
    r . hi += (r . lo < x . lo);
    return r;
}

operator bigint_t operator - (bigint_t x, bigint_t y)
{
    return x + (-y);
}

bigint_t bigint_mul(int64_t x, int64_t y)
{
    uint64_t ux, uy;
    ux = abs(x);
    uy = abs(y);
    
    uint32_t xhi, xlo;
    xhi = ux >> 32;
    xlo = ux & 0xFFFFFFFF;
    
    uint32_t yhi, ylo;
    yhi = uy >> 32;
    ylo = uy & 0xFFFFFFFF;
    
    uint64 x0, x1, x2, x3;
    x0 = (uint64_t)xhi * yhi;
    x1 = (uint64_t)xlo * yhi;
    x2 = (uint64_t)xhi * ylo;
    x3 = (uint64_t)xlo * ylo;
    
    x2 += x3 >> 32;
    x2 += x1;
    x0 += x2 < x1 ? (1 << 32) | 0;
    
    bigint_t ur;
    ur . lo = ((x2 & 0xFFFFFFFF) << 32) + (x3 & 0xFFFFFFFF);
    ur . hi = x0 + (x2 >> 32);
    
    if (sgn(x) != sgn(y))
        return -ur;
    
    return ur;
}

// x = xlo + xhi * 2^64
// y = ylo + yhi * 2^64
// x * y = (xlo + xhi * 2^64) + (ylo + yhi * 2^64)
//       = (xlo * ylo) + (xlo * yhi * 2^64) + (ylo * xhi * 2^64) + (xhi * yhi * 2^128)
// Thus cutting out the high part of the 128-bit result gives us:
//   x * y = LO(xlo * ylo) + HI(xlo * ylo) << 64 + LO(xlo * yhi) << 64 + LO(ylo * xhi) << 64
operator bigint_t operator * (bigint_t x, bigint_t y)
{
    // We need the full 128-bit result of lo * lo.
    bigint_t r;
    r = bigint_mul(x . lo, y . lo);
    
    // We only need the lo 64-bit result of lo * hi.
    r . hi += x . lo * y . hi;
    
    // We only need the lo 64-bit result of hi * lo.
    r . hi += x . hi * y . lo;
    
    return r;
}

operator bigint_t operator / (bigint_t x, bigint_t y)
{
}

#else
#error "bigint_t not implemented for > 64-bit ints"
#endif

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
        return __flooring_real_div(x, y);
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
        return __flooring_real_mod(x, y);
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

template<typename T> inline bool __MCNumberOverflowingBinaryOperation(double x, double y, MCNumberRef& r_z)
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
        case __kMCNumberBinaryOpTypeSignedIntegerBySignedInteger:
        {
            integer_t t_signed_value;
            if (!T::signed_by_signed(x -> integer, y -> integer,t_signed_value))
                return __MCNumberOverflowingBinaryOperation<T>(x -> integer, y -> integer, r_z);
            return MCNumberCreateWithInteger(t_signed_value, r_z);
        }
        break;
        case __kMCNumberBinaryOpTypeSignedIntegerByUnsignedInteger:
        {
            integer_t t_signed_value;
            if (!T::signed_by_unsigned(x -> integer, y -> unsigned_integer, t_signed_value))
                return __MCNumberOverflowingBinaryOperation<T>(x -> integer, y -> unsigned_integer, r_z);
            return MCNumberCreateWithInteger(t_signed_value, r_z);
        }
        break;
        case __kMCNumberBinaryOpTypeSignedIntegerByDouble:
            return __MCNumberDoubleBinaryOperation<T>(x -> integer, y -> real, r_z);
        case __kMCNumberBinaryOpTypeUnsignedIntegerBySignedInteger:
        {
            integer_t t_signed_value;
            if (!T::unsigned_by_signed(x -> unsigned_integer, y -> integer, t_signed_value))
                return __MCNumberOverflowingBinaryOperation<T>(x -> unsigned_integer, y -> integer, r_z);
            return MCNumberCreateWithInteger(t_signed_value, r_z);
        }
        break;
        case __kMCNumberBinaryOpTypeUnsignedIntegerByUnsignedInteger:
        {
            uinteger_t t_unsigned_value;
            if (!T::unsigned_by_unsigned(x -> unsigned_integer, y -> unsigned_integer, t_unsigned_value))
                return __MCNumberOverflowingBinaryOperation<T>(x -> unsigned_integer, y -> unsigned_integer, r_z);
            return MCNumberCreateWithUnsignedInteger(t_unsigned_value, r_z);
        }
        break;
        case __kMCNumberBinaryOpTypeUnsignedIntegerByDouble:
            return __MCNumberDoubleBinaryOperation<T>(x -> unsigned_integer, y -> real, r_z);
        case __kMCNumberBinaryOpTypeDoubleBySignedInteger:
            return __MCNumberDoubleBinaryOperation<T>(x -> real, y -> integer, r_z);
        case __kMCNumberBinaryOpTypeDoubleByUnsignedInteger:
            return __MCNumberDoubleBinaryOperation<T>(x -> real, y -> unsigned_integer, r_z);
        case __kMCNumberBinaryOpTypeDoubleByDouble:
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

static inline bool double_add(double x, double y, MCNumberRef& r_z)
{
    return MCNumberCreateWithReal(x + y, r_z);
}

static inline bool unsigned_add(MCNumberUnsignedInteger x, MCNumberUnsignedInteger y, MCNumberRef& r_z)
{
    MCNumberUnsignedInteger uz;
    if (LIKELY(__checked_unsigned_add(x, y, &uz)))
        return MCNumberCreateWithUnsignedInteger(uz, r_z);
    
    return false;
}

static inline bool signed_add(MCNumberSignedInteger x, MCNumberSignedInteger y, MCNumberRef& r_z)
{
    MCNumberSignedInteger sz;
    if (LIKELY(__checked_signed_add(x, y, &sz)))
        return MCNumberCreateWithInteger(sz, r_z);
    
    // If adding two signed numbers together fails, then the only case which
    // can succeed is if they are both non-negative.
    if (LIKELY(x >= 0 && y >= 0))
        return unsigned_add(x, y, r_z);
    
    return false;
}

////

static inline bool double_subtract(double x, double y, MCNumberRef& r_z)
{
    return MCNumberCreateWithReal(x - y, r_z);
}

static inline bool unsigned_subtract(MCNumberUnsignedInteger x, MCNumberUnsignedInteger y, MCNumberRef& r_z)
{
    MCNumberUnsignedInteger uz;
    if (LIKELY(__checked_unsigned_subtract(x, y, &uz)))
        return MCNumberCreateWithUnsignedInteger(uz, r_z);
    
    // If the above failed, then x < y.
    uz = y - x;
    if (LIKELY(uz <= __negate_negative_signed(kMCNumberSignedIntegerMin)))
        return MCNumberCreateWithInteger(-uz, r_z);
    
    return false;
}

static inline bool signed_subtract(MCNumberSignedInteger x, MCNumberSignedInteger y, MCNumberRef& r_z)
{
    // If x is >= 0 and y is >= 0 then there can be no overflow
    // If x is >= 0 and y is < 0 then there could be overflow but it will be representable
    // If x is < 0 and y is >= 0 then if there is overflow it is unrepresentable
    // If x is < 0 and y is < 0 then if there is overflow it is representable
    MCNumberSignedInteger sz;
    if (LIKELY(__checked_signed_subtract(x, y, &sz)))
        return MCNumberCreateWithInteger(sz, r_z);
    
    // If x >= 0 then y < 0.
    if (LIKELY(x >= 0))
        return unsigned_add(x, __negate_negative_signed(y), r_z);
    
    // If y < 0 then x < 0
    if (LIKELY(y < 0))
        return unsigned_subtract(__negate_negative_signed(y), __negate_negative_signed(x), r_z);
        
    return false;
}

////

static inline bool double_multiply(double x, double y, MCNumberRef& r_z)
{
    return MCNumberCreateWithReal(x * y, r_z);
}

static inline bool unsigned_multiply(MCNumberUnsignedInteger x, MCNumberUnsignedInteger y, MCNumberRef& r_z)
{
    MCNumberUnsignedInteger uz;
    if (LIKELY(__checked_unsigned_multiply(x, y, &uz)))
        return MCNumberCreateWithUnsignedInteger(uz, r_z);

    return false;
}

static inline bool signed_multiply(MCNumberSignedInteger x, MCNumberSignedInteger y, MCNumberRef& r_z)
{
    MCNumberSignedInteger sz;
    if (LIKELY(__checked_signed_multiply(x, y, &sz)))
        return MCNumberCreateWithInteger(sz, r_z);
    
    if (LIKELY(x >= 0 && y >= 0))
        return unsigned_multiply(x, y, r_z);
    
    if (LIKELY(x < 0 && y < 0))
        return unsigned_multiply(__negate_negative_signed(x), __negate_negative_signed(y), r_z);
    
    return false;
}

////

static inline bool double_div(double x, double y, MCNumberRef& r_z)
{
    return MCNumberCreateWithReal(__flooring_real_div(x, y), r_z);
}

static inline bool unsigned_div(MCNumberUnsignedInteger x, MCNumberUnsignedInteger y, MCNumberRef& r_z)
{
    if (y == 0)
        return false;
    
    return MCNumberCreateWithUnsignedInteger(x / y, r_z);
}

static inline bool signed_div(MCNumberSignedInteger x, MCNumberSignedInteger y, MCNumberRef& r_z)
{
    if (y == 0)
        return false;
    
    if (y == -1 && x == kMCNumberSignedIntegerMin)
        return MCNumberCreateWithUnsignedInteger(__negate_negative_signed(kMCNumberSignedIntegerMin), r_z);
    
    return MCNumberCreateWithInteger(x / y, r_z);
}

////

static inline bool double_mod(double x, double y, MCNumberRef& r_z)
{
    return MCNumberCreateWithReal(__flooring_real_mod(x, y), r_z);
}

static inline bool unsigned_mod(MCNumberUnsignedInteger x, MCNumberUnsignedInteger y, MCNumberRef& r_z)
{
    return MCNumberCreateWithUnsignedInteger(x % y, r_z);
}

static inline bool signed_mod(MCNumberSignedInteger x, MCNumberSignedInteger y, MCNumberRef& r_z)
{
    return MCNumberCreateWithInteger(__flooring_integral_mod(x, y), r_z);
}


////

bool MCNumberAdd(MCNumberRef x, MCNumberRef y, MCNumberRef& r_z)
{
    switch(__MCNumberComputeBinaryType(x, y))
    {
        case __kMCNumberBinaryOpTypeSignedIntegerBySignedInteger:
            if (LIKELY(signed_add(x -> integer, y -> integer, r_z)))
                return true;

            return double_add(x -> integer, y -> integer, r_z);
            
        case __kMCNumberBinaryOpTypeSignedIntegerByUnsignedInteger:
            if (LIKELY(x -> integer >= 0))
            {
                if (LIKELY(unsigned_add(x -> integer, y -> unsigned_integer, r_z)))
                    return true;
            }
            else
            {
                if (LIKELY(unsigned_subtract(y -> integer, __negate_negative_signed(x -> integer), r_z)))
                    return true;
            }
            
            return double_add(x -> integer, y -> unsigned_integer, r_z);
            
        case __kMCNumberBinaryOpTypeUnsignedIntegerBySignedInteger:
            if (LIKELY(y -> integer >= 0))
            {
                if (LIKELY(unsigned_add(x -> unsigned_integer, y -> integer, r_z)))
                    return true;
            }
            else
            {
                if (LIKELY(unsigned_subtract(x -> unsigned_integer, __negate_negative_signed(x -> integer), r_z)))
                    return true;
            }
            
            return double_add(x -> unsigned_integer, y -> integer, r_z);
            
        case __kMCNumberBinaryOpTypeUnsignedIntegerByUnsignedInteger:
            if (LIKELY(unsigned_add(x -> unsigned_integer, y -> unsigned_integer, r_z)))
                return true;
            
            return double_add(x -> unsigned_integer, y -> integer, r_z);
            
        case __kMCNumberBinaryOpTypeSignedIntegerByDouble:
            return double_add(x -> integer, y -> real, r_z);
            
        case __kMCNumberBinaryOpTypeUnsignedIntegerByDouble:
            return double_add(x -> unsigned_integer, y -> real, r_z);
            
        case __kMCNumberBinaryOpTypeDoubleBySignedInteger:
            return double_add(x -> real, y -> integer, r_z);
            
        case __kMCNumberBinaryOpTypeDoubleByUnsignedInteger:
            return double_add(x -> real, y -> unsigned_integer, r_z);
            
        case __kMCNumberBinaryOpTypeDoubleByDouble:
            return double_add(x -> real, y -> real, r_z);
            
        default:
            MCUnreachable();
            break;
    }
    
    return false;
}

bool MCNumberSubtract(MCNumberRef x, MCNumberRef y, MCNumberRef& r_z)
{
    switch(__MCNumberComputeBinaryType(x, y))
    {
        case __kMCNumberBinaryOpTypeSignedIntegerBySignedInteger:
            if (LIKELY(signed_subtract(x -> integer, y -> integer, r_z)))
                return true;

            return double_subtract(x -> integer, y -> integer, r_z);
            
        case __kMCNumberBinaryOpTypeSignedIntegerByUnsignedInteger:
            if (LIKELY(x -> integer >= 0))
            {
                if (LIKELY(unsigned_subtract(x -> integer, y -> unsigned_integer, r_z)))
                    return true;
            }
            
            return double_subtract(x -> integer, y -> unsigned_integer, r_z);
            
        case __kMCNumberBinaryOpTypeUnsignedIntegerBySignedInteger:
            if (LIKELY(y -> integer >= 0))
            {
                if (LIKELY(unsigned_subtract(x -> unsigned_integer, y -> integer, r_z)))
                    return true;
            }
            else
            {
                if (LIKELY(unsigned_add(x -> unsigned_integer, __negate_negative_signed(y -> integer), r_z)))
                    return true;
            }
            
            return double_subtract(x -> unsigned_integer, y -> integer, r_z);
            
        case __kMCNumberBinaryOpTypeUnsignedIntegerByUnsignedInteger:
            if (LIKELY(unsigned_subtract(x -> unsigned_integer, y -> unsigned_integer, r_z)))
                return true;
            
            return double_subtract(x -> unsigned_integer, y -> integer, r_z);
            
        case __kMCNumberBinaryOpTypeSignedIntegerByDouble:
            return double_subtract(x -> integer, y -> real, r_z);
            
        case __kMCNumberBinaryOpTypeUnsignedIntegerByDouble:
            return double_subtract(x -> unsigned_integer, y -> real, r_z);
            
        case __kMCNumberBinaryOpTypeDoubleBySignedInteger:
            return double_subtract(x -> real, y -> integer, r_z);
            
        case __kMCNumberBinaryOpTypeDoubleByUnsignedInteger:
            return double_subtract(x -> real, y -> unsigned_integer, r_z);
            
        case __kMCNumberBinaryOpTypeDoubleByDouble:
            return double_subtract(x -> real, y -> real, r_z);
            
        default:
            MCUnreachable();
            break;
    }
    
    return false;
}

bool MCNumberMultiply(MCNumberRef x, MCNumberRef y, MCNumberRef& r_z)
{
    switch(__MCNumberComputeBinaryType(x, y))
    {
        case __kMCNumberBinaryOpTypeSignedIntegerBySignedInteger:
            if (LIKELY(signed_multiply(x -> integer, y -> integer, r_z)))
                return true;
            
            if (x -> integer >= 0 && y -> integer >= 0)
            {
                if (LIKELY(unsigned_multiply(x -> integer, y -> integer, r_z)))
                    return true;
            }
            else if (x -> integer < 0 && y -> integer < 0)
            {
                if (LIKELY(unsigned_multiply(__negate_negative_signed(x -> integer), __negate_negative_signed(y -> integer), r_z)))
                    return true;
            }
                
            return double_multiply(x -> integer, y -> integer, r_z);
            
        case __kMCNumberBinaryOpTypeSignedIntegerByUnsignedInteger:
            if (LIKELY(x -> integer >= 0))
            {
                if (LIKELY(unsigned_multiply(x -> integer, y -> unsigned_integer, r_z)))
                    return true;
            }
            
            return double_multiply(x -> integer, y -> unsigned_integer, r_z);
            
        case __kMCNumberBinaryOpTypeUnsignedIntegerBySignedInteger:
            if (LIKELY(y -> integer >= 0))
            {
                if (LIKELY(unsigned_multiply(x -> unsigned_integer, y -> integer, r_z)))
                    return true;
            }
                
            return double_multiply(x -> integer, y -> unsigned_integer, r_z);
            
        case __kMCNumberBinaryOpTypeUnsignedIntegerByUnsignedInteger:
            if (LIKELY(unsigned_multiply(x -> unsigned_integer, y -> unsigned_integer, r_z)))
                return true;
            
            return double_multiply(x -> unsigned_integer, y -> integer, r_z);
            
        case __kMCNumberBinaryOpTypeSignedIntegerByDouble:
            return double_multiply(x -> integer, y -> real, r_z);
            
        case __kMCNumberBinaryOpTypeUnsignedIntegerByDouble:
            return double_multiply(x -> unsigned_integer, y -> real, r_z);
            
        case __kMCNumberBinaryOpTypeDoubleBySignedInteger:
            return double_multiply(x -> real, y -> integer, r_z);
            
        case __kMCNumberBinaryOpTypeDoubleByUnsignedInteger:
            return double_multiply(x -> real, y -> unsigned_integer, r_z);
            
        case __kMCNumberBinaryOpTypeDoubleByDouble:
            return double_multiply(x -> real, y -> real, r_z);
            
        default:
            MCUnreachable();
            break;
    }
    
    return false;
}

bool MCNumberDivide(MCNumberRef x, MCNumberRef y, MCNumberRef& r_z)
{
    // We special case '/' as it always produces a real result.
    switch(__MCNumberComputeBinaryType(x, y))
    {
        case __kMCNumberBinaryOpTypeSignedIntegerBySignedInteger:
            return double_divide(x -> integer, y -> integer, r_z);
        case __kMCNumberBinaryOpTypeSignedIntegerByUnsignedInteger:
            return double_divide(x -> integer, y -> unsigned_integer, r_z);
        case __kMCNumberBinaryOpTypeSignedIntegerByDouble:
            return double_divide(x -> integer, y -> real, r_z);
        case __kMCNumberBinaryOpTypeUnsignedIntegerBySignedInteger:
            return double_divide(x -> unsigned_integer, y -> integer, r_z);
        case __kMCNumberBinaryOpTypeUnsignedIntegerByUnsignedInteger:
            return double_divide(x -> unsigned_integer, y -> unsigned_integer, r_z);
        case __kMCNumberBinaryOpTypeUnsignedIntegerByDouble:
            return double_divide(x -> unsigned_integer, y -> real, r_z);
        case __kMCNumberBinaryOpTypeDoubleBySignedInteger:
            return double_divide(x -> real, y -> integer, r_z);
        case __kMCNumberBinaryOpTypeDoubleByUnsignedInteger:
            return double_divide(x -> real, y -> unsigned_integer, r_z);
        case __kMCNumberBinaryOpTypeDoubleByDouble:
            return double_divide(x -> real, y -> real, r_z);
        default:
            MCUnreachable();
            break;
    }
    
    return false;
}

bool MCNumberDiv(MCNumberRef x, MCNumberRef y, MCNumberRef& r_z)
{
    switch(__MCNumberComputeBinaryType(x, y))
    {
        case __kMCNumberBinaryOpTypeSignedIntegerBySignedInteger:
            if (LIKELY(signed_div(x -> integer, y -> integer, r_z)))
                return true;
            
            // We only get here is y -> integer == 0.
            return double_div(x -> integer, y -> integer, r_z);
            
        case __kMCNumberBinaryOpTypeSignedIntegerByUnsignedInteger:
            // Unsigneds > signeds so result is always zero.
            r_z = MCValueRetain(kMCIntegerZero);
            return true;
            
        case __kMCNumberBinaryOpTypeUnsignedIntegerBySignedInteger:
            if (LIKELY(unsigned_div_signed(x -> unsigned_integer, y -> integer, r_z)))
                    return true;
            
            return double_div(x -> integer, y -> unsigned_integer, r_z);
            
        case __kMCNumberBinaryOpTypeUnsignedIntegerByUnsignedInteger:
            if (LIKELY(unsigned_div(x -> unsigned_integer, y -> unsigned_integer, r_z)))
                return true;
            
            return double_div(x -> unsigned_integer, y -> integer, r_z);
            
        case __kMCNumberBinaryOpTypeSignedIntegerByDouble:
            return double_div(x -> integer, y -> real, r_z);
            
        case __kMCNumberBinaryOpTypeUnsignedIntegerByDouble:
            return double_div(x -> unsigned_integer, y -> real, r_z);
            
        case __kMCNumberBinaryOpTypeDoubleBySignedInteger:
            return double_div(x -> real, y -> integer, r_z);
            
        case __kMCNumberBinaryOpTypeDoubleByUnsignedInteger:
            return double_div(x -> real, y -> unsigned_integer, r_z);
            
        case __kMCNumberBinaryOpTypeDoubleByDouble:
            return double_div(x -> real, y -> real, r_z);
            
        default:
            MCUnreachable();
            break;
    }
    
    return false;
}

bool MCNumberMod(MCNumberRef x, MCNumberRef y, MCNumberRef& r_z)
{
    switch(__MCNumberComputeBinaryType(x, y))
    {
        case __kMCNumberBinaryOpTypeSignedIntegerBySignedInteger:
            if (LIKELY(signed_mod(x -> integer, y -> integer, r_z)))
                return true;
            
            // We only get here is y -> integer == 0.
            return double_div(x -> integer, y -> integer, r_z);
            
        case __kMCNumberBinaryOpTypeSignedIntegerByUnsignedInteger:
            // Unsigneds > signeds so result is always zero.
            r_z = MCValueRetain(kMCIntegerZero);
            return true;
            
        case __kMCNumberBinaryOpTypeUnsignedIntegerBySignedInteger:
            if (LIKELY(unsigned_div_signed(x -> unsigned_integer, y -> integer, r_z)))
                return true;
            
            return double_div(x -> integer, y -> unsigned_integer, r_z);
            
        case __kMCNumberBinaryOpTypeUnsignedIntegerByUnsignedInteger:
            if (LIKELY(unsigned_div(x -> unsigned_integer, y -> unsigned_integer, r_z)))
                return true;
            
            return double_div(x -> unsigned_integer, y -> integer, r_z);
            
        case __kMCNumberBinaryOpTypeSignedIntegerByDouble:
            return double_div(x -> integer, y -> real, r_z);
            
        case __kMCNumberBinaryOpTypeUnsignedIntegerByDouble:
            return double_div(x -> unsigned_integer, y -> real, r_z);
            
        case __kMCNumberBinaryOpTypeDoubleBySignedInteger:
            return double_div(x -> real, y -> integer, r_z);
            
        case __kMCNumberBinaryOpTypeDoubleByUnsignedInteger:
            return double_div(x -> real, y -> unsigned_integer, r_z);
            
        case __kMCNumberBinaryOpTypeDoubleByDouble:
            return double_div(x -> real, y -> real, r_z);
            
        default:
            MCUnreachable();
            break;
    }
    
    return false;
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

template<typename T> inline bool __MCNumberOverflowingIntegerBinaryOperation(int64_t x, int64_t y, MCNumberRef& r_z)
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

template<typename T> inline bool __MCNumberIntegerBinaryOperation(MCNumberRef x, MCNumberRef y, MCNumberRef& r_z)
{
    switch(__MCNumberComputeBinaryType(x, y))
    {
        case __kMCNumberBinaryOpTypeSignedIntegerBySignedInteger:
        {
            integer_t t_signed_value;
            if (!T::signed_by_signed(x -> integer, y -> integer,t_signed_value))
                return __MCNumberOverflowingIntegerBinaryOperation<T>(x -> integer, y -> integer, r_z);
            return MCNumberCreateWithInteger(t_signed_value, r_z);
        }
        break;
        case __kMCNumberBinaryOpTypeSignedIntegerByUnsignedInteger:
        {
            integer_t t_signed_value;
            if (!T::signed_by_unsigned(x -> integer, y -> unsigned_integer, t_signed_value))
                return __MCNumberOverflowingIntegerBinaryOperation<T>(x -> integer, y -> unsigned_integer, r_z);
            return MCNumberCreateWithInteger(t_signed_value, r_z);
        }
        break;
        case __kMCNumberBinaryOpTypeUnsignedIntegerBySignedInteger:
        {
            integer_t t_signed_value;
            if (!T::unsigned_by_signed(x -> unsigned_integer, y -> integer, t_signed_value))
                return __MCNumberOverflowingIntegerBinaryOperation<T>(x -> unsigned_integer, y -> integer, r_z);
            return MCNumberCreateWithInteger(t_signed_value, r_z);
        }
        break;
        case __kMCNumberBinaryOpTypeUnsignedIntegerByUnsignedInteger:
        {
            uinteger_t t_unsigned_value;
            if (!T::unsigned_by_unsigned(x -> unsigned_integer, y -> unsigned_integer, t_unsigned_value))
                return __MCNumberOverflowingIntegerBinaryOperation<T>(x -> unsigned_integer, y -> unsigned_integer, r_z);
            return MCNumberCreateWithUnsignedInteger(t_unsigned_value, r_z);
        }
        break;
        default:
            MCUnreachable();
            break;
    }
    
    return false;
}

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
    return __MCNumberIntegerBinaryOperation<__MCNumberOperationAdd>(x, y, r_z);
}

bool MCNumberIntegerSubtract(MCNumberRef x, MCNumberRef y, MCNumberRef& r_z)
{
    return __MCNumberIntegerBinaryOperation<__MCNumberOperationSubtract>(x, y, r_z);
}

bool MCNumberIntegerMultiply(MCNumberRef x, MCNumberRef y, MCNumberRef& r_z)
{
    return __MCNumberIntegerBinaryOperation<__MCNumberOperationMultiply>(x, y, r_z);
}

bool MCNumberIntegerDiv(MCNumberRef x, MCNumberRef y, MCNumberRef& r_z)
{
    // The representation of 0 for signed and unsigned ints is identical.
    if (y -> integer == 0)
        return __MCNumberThrowDivisionByZeroError();
    
    return __MCNumberIntegerBinaryOperation<__MCNumberOperationDiv>(x, y, r_z);
}

bool MCNumberIntegerMod(MCNumberRef x, MCNumberRef y, MCNumberRef& r_z)
{
    // The representation of 0 for signed and unsigned ints is identical.
    if (y -> integer == 0)
        return __MCNumberThrowDivisionByZeroError();
    
    return __MCNumberIntegerBinaryOperation<__MCNumberOperationMod>(x, y, r_z);
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
