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


TEST(lextable, factor_table)
//
// Checks that the entries of factor_table are in alphabetical order.
//
{
	extern LT factor_table[];
	extern const uint4 factor_table_size;

	ASSERT_GE(factor_table_size, 1);

	for(uint4 i = 0; i < factor_table_size - 1; i++) {
		EXPECT_LT(strcmp(factor_table[i].token, factor_table[i+1].token), 0)
			<< "\"" << factor_table[i+1].token << "\""
			<< " comes before "
			<< "\"" << factor_table[i].token << "\"";
	}
}
