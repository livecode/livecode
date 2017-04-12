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
