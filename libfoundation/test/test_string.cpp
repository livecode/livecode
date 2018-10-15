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
#include "foundation-unicode.h"
#include "foundation-auto.h"


TEST(string, creation)
//
// Checks string creation.
//
{
	MCAutoStringRef t_string;

	const char* t_c_string = "Hello";
	ASSERT_TRUE(
		MCStringCreateWithBytes(
			reinterpret_cast<const byte_t*>(t_c_string),
			5,
			kMCStringEncodingASCII,
			false,
			&t_string
		)
	);

	ASSERT_STREQ(t_c_string, MCStringGetCString(*t_string));
}

TEST(string, format_int)
//
// Checks that basic integer formatting using MCStringFormat works
//
{
	MCAutoStringRef t_string;

	ASSERT_TRUE(MCStringFormat(&t_string, "%i %i", 1, 2));
	ASSERT_STREQ(MCStringGetCString(*t_string), "1 2");
}

TEST(string, format_double_f)
//
// Checks that floating-point formatting using %f works
//
{
	MCAutoStringRef t_string;

	ASSERT_TRUE(MCStringFormat(&t_string, "%.2f %.2f", 1.0, 2.0));
	ASSERT_STREQ(MCStringGetCString(*t_string), "1.00 2.00");
}

TEST(string, format_double_g)
//
// Checks that floating-point formatting using %g works
//
{
	MCAutoStringRef t_string;

	ASSERT_TRUE(MCStringFormat(&t_string, "%g %g", 1.0, 2.0));
	ASSERT_STREQ(MCStringGetCString(*t_string), "1 2");
}

TEST(string, format_int_and_float)
//
// Checks that combinations of integer and floating-point parameters to MCStringFormat work
//
{
	MCAutoStringRef t_string;

	ASSERT_TRUE(MCStringFormat(&t_string, "%i %.2f %u %g", 1, 2.0, 3, 4.0));
	ASSERT_STREQ(MCStringGetCString(*t_string), "1 2.00 3 4");
}

TEST(string, format_string)
//
// Checks that C-string formats work
//
{
	MCAutoStringRef t_string;

	const char* t_cstring = "abcdef";
	ASSERT_TRUE(MCStringFormat(&t_string, "%.3s %s", t_cstring, t_cstring));
	ASSERT_STREQ(MCStringGetCString(*t_string), "abc abcdef");
}

TEST(string, format_stringref)
//
// Checks that stringref arguments to MCStringFormat work
//
{
	MCAutoStringRef t_string;
	MCStringRef kTestString = MCSTR("hello, world");

	ASSERT_TRUE(MCStringFormat(&t_string, "%@", kTestString));
	ASSERT_STREQ(MCStringGetCString(*t_string), MCStringGetCString(kTestString));
}

TEST(string, format_stringref_with_range)
//
// Checks that stringref subrange arguments to MCStringFormat work
//
{
	MCAutoStringRef t_string;
	MCStringRef kTestString = MCSTR("hello, world");

	MCRange t_range = MCRangeMake(0, 5);
	ASSERT_TRUE(MCStringFormat(&t_string, "%*@", &t_range, kTestString));
	ASSERT_STREQ(MCStringGetCString(*t_string), "hello");
}

TEST(string, format_null_valueref)
//
// Checks that a nullptr valueref maps to (null) [ Bug 19866 ]
//
{	MCAutoStringRef t_string;

	ASSERT_TRUE(MCStringFormat(&t_string, "%@", nullptr));
	ASSERT_STREQ(MCStringGetCString(*t_string), "(null)");
}

TEST(string, format_everything)
//
// Checks formatting with lots of different options
//
{
	MCAutoStringRef t_string;
	MCStringRef kTestString = MCSTR("hello, world!");

	MCRange t_range1 = MCRangeMake(0, 5);
	MCRange t_range2 = MCRangeMake(12, 1);

	ASSERT_TRUE(MCStringFormat(&t_string, "Test: %*@%s%*@ %u%g %i%i%i %.3f", &t_range1, kTestString,
		"?", &t_range2, kTestString, 4, 2.0, 0, 1, 2, 0.007));
	ASSERT_STREQ(MCStringGetCString(*t_string), "Test: hello?! 42 012 0.007");
}

static void check_bidi_of_surrogate_range(int p_lower, int p_upper)
{
    int t_size = p_upper - p_lower;

		MCAutoArray<unichar_t> t_pua_chars;
		ASSERT_TRUE(t_pua_chars.Resize(t_size * 2));
    for(int i = 0; i < t_size; i++)
    {
        MCUnicodeCodepointToSurrogates(i + p_lower,
                                       t_pua_chars[i*2], t_pua_chars[i*2 + 1]);
    }

		MCAutoArray<uint8_t> t_props;
		ASSERT_TRUE(t_props.Resize(t_size));
    MCUnicodeGetProperty(t_pua_chars.Ptr(), t_size, kMCUnicodePropertyBidiClass, kMCUnicodePropertyTypeUint8, t_props.Ptr());

    for(int i = 0; i < t_size; i++)
    {
            ASSERT_TRUE(t_props[i] == kMCUnicodeDirectionLeftToRight);
    }
}

TEST(string, surrogate_unicode_props)
//
// Checks that MCUnicodeGetProperty correctly deals with surrogates
// (Regression test for Bug 19045)
//
{
    const int kSPUA_A_Lower = 0xF0000;
    const int kSPUA_A_Upper = 0xFFFFD + 1; // non-inclusive
    check_bidi_of_surrogate_range(kSPUA_A_Lower, kSPUA_A_Upper);

    const int kSPUA_B_Lower = 0x100000;
    const int kSPUA_B_Upper = 0x10FFFD + 1; // non-inclusive
    check_bidi_of_surrogate_range(kSPUA_B_Lower, kSPUA_B_Upper);
}
