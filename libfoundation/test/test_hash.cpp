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

#include "gtest/gtest.h"

#include "foundation.h"
#include "foundation-auto.h"
#include "foundation-unicode.h"
#include "foundation-system.h"

#include <limits>

static_assert(sizeof(hash_t) == sizeof(uint32_t),
              "Update hash tests for this hash size");

/* ----------------------------------------------------------------
 * Numeric hashes
 * ---------------------------------------------------------------- */

template <typename T>
T random_pos_int()
{
    return std::numeric_limits<T>::max() * MCSRandomReal();
}

TEST(hash, integer)
{
    auto t_uinteger = random_pos_int<uinteger_t>();
    EXPECT_EQ(MCHashUInteger(0), {0});
    EXPECT_EQ(MCHashUInteger(1), 2654435761U);
    EXPECT_NE(MCHashUInteger(t_uinteger), MCHashUInteger(0));

    auto t_integer = random_pos_int<integer_t>();
    EXPECT_EQ(MCHashInteger(t_integer), MCHashUInteger(t_integer));
    EXPECT_EQ(MCHashInteger(-t_integer), MCHashInteger(t_integer));
    EXPECT_EQ(MCHashInteger(INT_MIN), MCHashUInteger(uinteger_t(INT_MAX) + 1));

    EXPECT_EQ(MCHashUSize(t_uinteger), MCHashUInteger(t_uinteger));

    auto t_usize =
        (random_pos_int<size_t>() & ~size_t(UINT_MAX)) | size_t(t_uinteger);
    if (sizeof(t_usize) != sizeof(t_uinteger))
        EXPECT_NE(MCHashUSize(t_usize), MCHashUInteger(t_uinteger));

    auto t_ssize = random_pos_int<ssize_t>();
    EXPECT_EQ(MCHashSize(t_ssize), MCHashUSize(t_ssize));
    EXPECT_EQ(MCHashSize(-t_ssize), MCHashSize(t_ssize));
    EXPECT_EQ(MCHashSize(SSIZE_MIN), MCHashUSize(size_t(SSIZE_MAX) + 1));
}

TEST(hash, floating_point)
{
    EXPECT_EQ(MCHashDouble(0), MCHashUInteger(0));

    auto t_uinteger = random_pos_int<uinteger_t>();
    double t_intpart = t_uinteger;    EXPECT_EQ(MCHashDouble(t_intpart), MCHashUInteger(t_uinteger));
    EXPECT_EQ(MCHashDouble(-t_intpart), MCHashUInteger(t_uinteger));

    auto t_fraction = MCSRandomReal();
    EXPECT_NE(MCHashDouble(t_fraction), MCHashUInteger(0));
    EXPECT_EQ(MCHashDouble(t_fraction), MCHashDouble(-t_fraction));

    auto t_double = t_fraction + t_intpart;
    EXPECT_NE(MCHashDouble(t_double), MCHashDouble(t_intpart));
    EXPECT_NE(MCHashDouble(t_double), MCHashDouble(t_fraction));
    EXPECT_EQ(MCHashDouble(-t_double), MCHashDouble(t_double));
}

TEST(hash, pointer)
{
    EXPECT_EQ(MCHashPointer(nullptr), MCHashUInteger(0));

    bool t_target = false;
    EXPECT_NE(MCHashPointer(&t_target), MCHashPointer(nullptr));
}

/* ----------------------------------------------------------------
 * Blob hashes
 * ---------------------------------------------------------------- */

TEST(hash, bytes)
{
    const byte_t k_zeros[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  0,  0};
    const byte_t k_order[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12};

    EXPECT_EQ(MCHashBytes(k_zeros), {0});
    EXPECT_EQ(MCHashBytes(k_order), {107654892});
    /* Check that hashing isn't length-dependent */
    EXPECT_NE(MCHashBytes(k_order, sizeof(k_order) / 2),
              MCHashBytes(k_order));

    auto t_random_bytes_hash = [](hash_t& h) -> hash_t {
        MCAutoDataRef t_bytes;
        if (!MCSRandomData(256, &t_bytes))
            return false;
        h = MCHashBytes(MCDataGetSpan<const byte_t>(*t_bytes));
        return true;
    };
    hash_t t_left, t_right;
    ASSERT_TRUE(t_random_bytes_hash(t_left) && t_random_bytes_hash(t_right));
    EXPECT_NE(t_left, t_right);
}

/* ----------------------------------------------------------------
 * String hashes
 * ---------------------------------------------------------------- */

hash_t string_hash_exact(MCStringRef s)
{
    return MCStringHash(s, kMCStringOptionCompareExact);
}

hash_t string_hash_caseless(MCStringRef s)
{
    return MCStringHash(s, kMCStringOptionCompareCaseless);
}

template<size_t N>
MCAutoStringRef string_create_utf8(const char (&utf8)[N])
{
    MCAutoStringRef s;
    MCStringCreateWithBytes(reinterpret_cast<const byte_t*>(&utf8),
                            N-1, kMCStringEncodingUTF8, false, &s);
    return s;
}

template<size_t N>
MCAutoStringRef string_create_native(const char (&native)[N])
{
    MCAutoStringRef s;
    MCStringCreateWithNativeChars((const char_t *)native, N-1, &s);
    return s;
}

TEST(hash, native_string)
{
    MCAutoStringRef t_mixed = string_create_native("LiveCode");
    MCAutoStringRef t_lower = string_create_native("livecode");

    /* Check a couple of specific values */
    EXPECT_EQ(string_hash_exact(kMCEmptyString), {2166136261});
    EXPECT_EQ(string_hash_exact(*t_mixed), {2613309534});

    /* Check folded vs non-folded */
    EXPECT_NE(string_hash_exact(*t_mixed), string_hash_exact(*t_lower));
    EXPECT_EQ(string_hash_caseless(*t_mixed),  string_hash_caseless(*t_lower));
}

TEST(hash, unicode_string)
{
    MCAutoStringRef t_cat =         string_create_utf8(u8"\U0001F63A");
    MCAutoStringRef t_private_use = string_create_utf8(u8"\U0000F63A");
    MCAutoStringRef t_unibyte =     string_create_utf8(u8"\U0000003A");
    MCAutoStringRef t_lambda =      string_create_utf8(u8"\U000003BB");

    MCAutoStringRef t_bmp_mixed =
        string_create_utf8(u8"\u039a\u03b1\u03bb\u03b7\u03bc\u03ad\u03c1\u03b1");
    MCAutoStringRef t_bmp_lower =
        string_create_utf8(u8"\u03ba\u03b1\u03bb\u03b7\u03bc\u03ad\u03c1\u03b1");

    MCAutoStringRef t_nonbmp_mixed =
        string_create_utf8(u8"\U0001F984 Friendship is Magic!");
    MCAutoStringRef t_nonbmp_lower =
        string_create_utf8(u8"\U0001F984 friendship is magic!");

    /* Check a specific value */
    EXPECT_EQ(string_hash_exact(*t_lambda), {2244387611});

    /* Check that upper bytes of codepoints are included in hash */
    EXPECT_NE(string_hash_exact(*t_private_use), string_hash_exact(*t_unibyte));
    EXPECT_NE(string_hash_exact(*t_cat), string_hash_exact(*t_private_use));

    /* Check folded vs non-folded */
    EXPECT_NE(string_hash_exact(*t_bmp_mixed),
              string_hash_exact(*t_bmp_lower));
    EXPECT_EQ(string_hash_caseless(*t_bmp_mixed),
              string_hash_caseless(*t_bmp_lower));
    EXPECT_NE(string_hash_exact(*t_nonbmp_mixed),
              string_hash_exact(*t_nonbmp_lower));
    EXPECT_EQ(string_hash_caseless(*t_nonbmp_mixed),
              string_hash_caseless(*t_nonbmp_mixed));
}
