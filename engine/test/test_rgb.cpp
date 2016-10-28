/* Copyright (C) 2016 LiveCode Ltd.

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

#include "prefix.h"
#include "util.h"
#include "rgb.cpp"

static char to_lower(char p_char)
{
	if ('A' <= p_char && p_char <= 'Z')
		p_char += 32;
	
	return p_char;
}

// We assume color name strings are ASCII
static compare_t string_caseless_compare(const char *left, const char *right)
{
	uindex_t t_left_length, t_right_length;
	t_left_length = strlen(left);
	t_right_length = strlen(right);
	
	for(;;)
	{
		if (t_left_length == 0 || t_right_length == 0)
			break;

		index_t t_diff;
		t_diff = to_lower(*left++) - tolower(*right++);
		if (t_diff != 0)
			return t_diff;
		
		t_left_length -= 1;
		t_right_length -= 1;
	}
	
	return t_left_length - t_right_length;
}

TEST(rgb, color_table)
//
// Checks that the entries of rgb color table are in alphabetical order.
//
{
	uint4 color_table_size;
	color_table_size = sizeof(color_table) / sizeof(color_table[0]);

	ASSERT_GE(color_table_size, (unsigned)1);

	for(uint4 i = 0; i < color_table_size - 1; i++) {
		EXPECT_LT(string_caseless_compare(color_table[i].token,
		                                  color_table[i+1].token), 0)
			<< "\"" << color_table[i+1].token << "\""
			<< " comes before "
			<< "\"" << color_table[i].token << "\"";
	}
}
