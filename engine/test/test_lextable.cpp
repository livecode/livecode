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

#include "prefix.h"
#include "parsedef.h"
#include "scriptpt.h"

TEST(lextable, constant_table)
//
// Checks that the entries of constant_table are in alphabetical order.
//
{
    extern const Cvalue *constant_table;
	extern const uint4 constant_table_size;

	ASSERT_GE(constant_table_size, (unsigned)1);

	for(uint4 i = 0; i < constant_table_size - 1; i++) {
		EXPECT_LT(strcmp(constant_table[i].token, constant_table[i+1].token), 0)
			<< "\"" << constant_table[i+1].token << "\""
			<< " comes before "
			<< "\"" << constant_table[i].token << "\"";
	}
}


TEST(lextable, table_pointer)
//
// Checks that the entries of factor_table are in alphabetical order.
//
{
	extern const LT * const table_pointers[];
	extern const uint4 table_pointers_size;

	extern const uint2 table_sizes[];
	extern const uint4 table_sizes_size;

	ASSERT_EQ(table_pointers_size, table_sizes_size);

	for (uint4 i = 0; i < table_pointers_size; i++) {

		const LT* table = table_pointers[i];
		const uint4 table_size = table_sizes[i];

		ASSERT_GE(table_size, (unsigned)1);

		for (uint4 j = 0; j < table_size - 1; j++) {
			EXPECT_LT(strcmp(table[j].token, table[j+1].token), 0)
				<< "\"" << table[j+1].token << "\""
				<< " comes before "
				<< "\"" << table[j].token << "\"";
		}
	}
}
