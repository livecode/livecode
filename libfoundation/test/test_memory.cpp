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

#include "foundation-private.h"

#define GTEST_COUT std::cerr << "[          ] [ INFO ]"

template<typename T>
void _memory_value_size(const char *p_name)
{
    GTEST_COUT << " sizeof(" << p_name << ") = " << sizeof(T) << std::endl;
}

void _memory_malloc_size(int p_size)
{
    char *t_blocks[16];
    for(int i = 0; i < 16; i++)
        t_blocks[i] = (char *)malloc(p_size);
    GTEST_COUT << " malloc deltas for " << p_size << " =";
    for(int i = 1; i < 16; i++)
        std::cerr << " " << (t_blocks[i] - t_blocks[i - 1]);
    std::cerr << std::endl;
    for(int i = 0; i < 16; i++)
        free(t_blocks[i]);
}

TEST(memory, information)
{
    /* Dump out the struct sizes of the core value types */
    _memory_value_size<__MCNull>("MCNull");
    _memory_value_size<__MCBoolean>("MCBoolean");
    _memory_value_size<__MCNumber>("MCNumber");
    _memory_value_size<__MCName>("MCName");
    _memory_value_size<__MCString>("MCString");
    _memory_value_size<__MCData>("MCData");
    _memory_value_size<__MCArray>("MCArray");
    _memory_value_size<__MCList>("MCList");
    _memory_value_size<__MCSet>("MCSet");
    _memory_value_size<__MCProperList>("MCProperList");
    
    /* Check the veracity of the malloc implementation with regards small objects */
    _memory_malloc_size(8);
    _memory_malloc_size(16);
    _memory_malloc_size(24);
    _memory_malloc_size(32);
    _memory_malloc_size(48);
    _memory_malloc_size(64);
}

TEST(memory, sizes)
{
#ifdef __32_BIT__
    EXPECT_EQ(sizeof(__MCNull), 8);
    EXPECT_EQ(sizeof(__MCBoolean), 8);
    EXPECT_EQ(sizeof(__MCNumber), 16);
    EXPECT_EQ(sizeof(__MCName), 24);
    EXPECT_EQ(sizeof(__MCString), 32);
    EXPECT_EQ(sizeof(__MCData), 20);
    EXPECT_EQ(sizeof(__MCArray), 16);
    EXPECT_EQ(sizeof(__MCList), 16);
    EXPECT_EQ(sizeof(__MCSet), 16);
    EXPECT_EQ(sizeof(__MCProperList), 16);
#else
    EXPECT_EQ(sizeof(__MCNull), 8);
    EXPECT_EQ(sizeof(__MCBoolean), 8);
    EXPECT_EQ(sizeof(__MCNumber), 16);
    EXPECT_EQ(sizeof(__MCName), 32);
    EXPECT_EQ(sizeof(__MCString), 32);
    EXPECT_EQ(sizeof(__MCData), 24);
    EXPECT_EQ(sizeof(__MCArray), 24);
    EXPECT_EQ(sizeof(__MCList), 24);
    EXPECT_EQ(sizeof(__MCSet), 24);
    EXPECT_EQ(sizeof(__MCProperList), 24);
#endif
}
