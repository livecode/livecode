#include "gtest/gtest.h"

#include "prefix.h"

#include "globdefs.h"
#include "parsedef.h"
#include "filedefs.h"

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
