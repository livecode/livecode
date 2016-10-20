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

TEST(rgb, color_table)
//
// Checks that the entries of rgb color table are in alphabetical order.
//
{
	uint4 color_table_size;
	color_table_size = sizeof(color_table) / sizeof(color_table[0]);

	ASSERT_GE(color_table_size, (unsigned)1);

	for(uint4 i = 0; i < color_table_size - 1; i++) {
		EXPECT_LT(MCU_strcasecmp(color_table[i].token, color_table[i+1].token), 0)
			<< "\"" << color_table[i+1].token << "\""
			<< " comes before "
			<< "\"" << color_table[i].token << "\"";
	}
}
