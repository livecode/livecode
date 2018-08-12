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
#include "foundation-span.h"
#include "foundation-auto.h"

TEST(properlist, createwithforeignvalues_bridgable)
{
    float t_floats[] = { 1.1, 2.1, 3.1, 5, 10 };
    size_t t_float_count = sizeof(t_floats) / sizeof(t_floats[0]);
    
    MCAutoProperListRef t_float_list;
    ASSERT_TRUE(MCProperListCreateWithForeignValues(kMCFloatTypeInfo, MCMakeSpan(t_floats), &t_float_list));
    ASSERT_EQ(MCProperListGetLength(*t_float_list), t_float_count);
    for(size_t i = 0; i < t_float_count; i++)
    {
        MCValueRef t_element = MCProperListFetchElementAtIndex(*t_float_list, i);
        ASSERT_EQ(MCValueGetTypeInfo(t_element), kMCNumberTypeInfo);
        if (MCValueGetTypeInfo(t_element) == kMCNumberTypeInfo)
        {
            ASSERT_EQ(MCNumberFetchAsReal((MCNumberRef)t_element), t_floats[i]);
        }
    }
}

TEST(properlist, createwithforeignvalues_unbridgable)
{
    float t_floats[] = { 1.1, 2.1, 3.1, 5, 10 };
    void *t_pointers[] = { nullptr, &t_floats[1], nullptr, &t_floats[2] };
    size_t t_pointer_count = sizeof(t_pointers) / sizeof(t_pointers[0]);
    
    MCAutoProperListRef t_pointer_list;
    ASSERT_TRUE(MCProperListCreateWithForeignValues(kMCPointerTypeInfo, MCMakeSpan(t_pointers), &t_pointer_list));
    ASSERT_EQ(MCProperListGetLength(*t_pointer_list), t_pointer_count);
    for(size_t i = 0; i < t_pointer_count; i++)
    {
        MCValueRef t_element = MCProperListFetchElementAtIndex(*t_pointer_list, i);
        ASSERT_EQ(MCValueGetTypeInfo(t_element), kMCPointerTypeInfo);
        if (MCValueGetTypeInfo(t_element) == kMCPointerTypeInfo)
        {
            ASSERT_EQ(*static_cast<void **>(MCForeignValueGetContentsPtr(((MCForeignValueRef)t_element))), t_pointers[i]);
        }
    }
}
