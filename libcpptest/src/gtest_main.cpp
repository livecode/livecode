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
#include "MCTapListener.h"

int main(int argc, char **argv) {
	testing::InitGoogleTest(&argc, argv);

	for (int i = 0; i < argc; i++) {
		char* arg = argv[i];
		if (arg[0] == '-' &&
			arg[1] == '-' &&
			arg[2] == 't' &&
			arg[3] == 'a' &&
			arg[4] == 'p' &&
			arg[5] == '=') {

			testing::TestEventListeners& listeners
				= testing::UnitTest::GetInstance()->listeners();
			listeners.Append(new MCTapListener(&arg[6]));
		}
	}

	return RUN_ALL_TESTS();
}
