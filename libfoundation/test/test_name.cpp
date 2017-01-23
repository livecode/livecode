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

TEST(name, index_equiv_strings)
{
    static index_t s_test_indicies[] =
    {
        INT32_MIN + 32,
        INT16_MIN,
        INT8_MIN,
        0,
        INT8_MAX,
        INT16_MAX,
        INT32_MAX - 32,
    };
    for(size_t i = 0; i < sizeof(s_test_indicies) / sizeof(s_test_indicies[0]); i++)
    {
        for(int j = -32; j <= 32; j++)
        {
            int64_t t_full_index = s_test_indicies[i] + j;
            index_t t_index = (index_t)t_full_index;
            char_t t_index_str[32];
            sprintf((char *)t_index_str, "%lld", t_full_index);
            
            // fprintf(stderr, "%lld, %d\n", t_full_index, t_index);
            
            MCNewAutoNameRef t_index_name, t_string_name;
            ASSERT_TRUE(MCNameCreateWithIndex(t_index, &t_index_name));
            ASSERT_TRUE(MCNameCreateWithNativeChars(t_index_str, strlen((char *)t_index_str), &t_string_name));
            // fprintf(stderr, "%s, %s\n", MCStringGetCString(MCNameGetString(*t_index_name)), MCStringGetCString(MCNameGetString(*t_string_name)));
            ASSERT_EQ(*t_index_name, *t_string_name);
        }
    }
}
