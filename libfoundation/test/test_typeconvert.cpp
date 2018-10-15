/*                                                                     -*-C++-*-
 * Copyright (C) 2016 LiveCode Ltd.
 *
 * This file is part of LiveCode.
 *
 * LiveCode is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License v3 as published
 * by the Free Software Foundation.
 *
 * LiveCode is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with LiveCode.  If not see
 * <http://www.gnu.org/licenses/>.
 */

#include "gtest/gtest.h"

#include "foundation.h"
#include "foundation-auto.h"

TEST(typeconvert, narrowcast)
{
    /* MCNarrowCast never does any checking */
    EXPECT_EQ(MCNarrowCast<int>(4.0), 4);
    EXPECT_EQ(MCNarrowCast<int>(4.5), 4);
}

TEST(typeconvert, narrow)
{
    uint8_t dummy_uint8_t = 0;
    EXPECT_TRUE(MCNarrow(INT8_MAX, dummy_uint8_t));
    EXPECT_EQ(dummy_uint8_t, INT8_MAX);
    EXPECT_FALSE(MCNarrow(UINT8_MAX + 1, dummy_uint8_t));
    EXPECT_EQ(dummy_uint8_t, INT8_MAX); /* Unmodified */
    EXPECT_FALSE(MCNarrow(INT8_MIN, dummy_uint8_t));
    EXPECT_EQ(dummy_uint8_t, INT8_MAX); /* Unmodified */

    int dummy_int_t = 0;
    EXPECT_TRUE(MCNarrow(3.0, dummy_int_t));
    EXPECT_EQ(dummy_int_t, 3);
    EXPECT_FALSE(MCNarrow(-1.5, dummy_int_t));
    EXPECT_EQ(dummy_int_t, 3); /* Unmodified */
}

TEST(typeconvert, string_integer)
//
// Checks string-to-integer conversion
//
{
	integer_t t_value;

	t_value = 0;
	EXPECT_FALSE(MCTypeConvertStringToLongInteger(kMCEmptyString, t_value));
	EXPECT_EQ(t_value, 0);

	t_value = 0;
	EXPECT_TRUE(MCTypeConvertStringToLongInteger(MCSTR("42"), t_value));
	EXPECT_EQ(t_value, 42);

	t_value = 0;
	EXPECT_TRUE(MCTypeConvertStringToLongInteger(MCSTR("0x42"), t_value));
	EXPECT_EQ(t_value, 0x42);

	t_value = 0;
	EXPECT_FALSE(MCTypeConvertStringToLongInteger(MCSTR("42 43"), t_value));
	EXPECT_EQ(t_value, 0);

	t_value = 0;
	EXPECT_TRUE(MCTypeConvertStringToLongInteger(MCSTR(" 42 "), t_value));
	EXPECT_EQ(t_value, 42);

	t_value = 0;
	EXPECT_TRUE(MCTypeConvertStringToLongInteger(MCSTR("+42"), t_value));
	EXPECT_EQ(t_value, 42);

	t_value = 0;
	EXPECT_TRUE(MCTypeConvertStringToLongInteger(MCSTR("-42"), t_value));
	EXPECT_EQ(t_value, -42);

	t_value = 0;
	EXPECT_FALSE(MCTypeConvertStringToLongInteger(MCSTR("0x"), t_value));
	EXPECT_EQ(t_value, 0);
}

TEST(typeconvert, string_real)
//
// Checks string-to-integer conversion
//
{
	real64_t t_value;

	t_value = 0;
	EXPECT_FALSE(MCTypeConvertStringToReal(kMCEmptyString, t_value, false));
	EXPECT_EQ(t_value, 0);

	t_value = 0;
	EXPECT_TRUE(MCTypeConvertStringToReal(MCSTR("42"), t_value, false));
	EXPECT_EQ(t_value, 42);

	t_value = 0;
	EXPECT_TRUE(MCTypeConvertStringToReal(MCSTR("0x42"), t_value, false));
	EXPECT_EQ(t_value, 0x42);

	t_value = 0;
	EXPECT_FALSE(MCTypeConvertStringToReal(MCSTR("42 43"), t_value, false));
	EXPECT_EQ(t_value, 0);

	t_value = 0;
	EXPECT_TRUE(MCTypeConvertStringToReal(MCSTR(" 42 "), t_value, false));
	EXPECT_EQ(t_value, 42);

	t_value = 0;
	EXPECT_TRUE(MCTypeConvertStringToReal(MCSTR("+42"), t_value, false));
	EXPECT_EQ(t_value, 42);

	t_value = 0;
	EXPECT_TRUE(MCTypeConvertStringToReal(MCSTR("-42"), t_value, false));
	EXPECT_EQ(t_value, -42);

	t_value = 0;
	EXPECT_FALSE(MCTypeConvertStringToReal(MCSTR("0x"), t_value, false));
	EXPECT_EQ(t_value, 0);
}
