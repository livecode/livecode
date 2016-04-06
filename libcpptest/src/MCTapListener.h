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

#ifndef __MC_TAP_LISTENER__
#define __MC_TAP_LISTENER__

#include <fstream>
#include "gtest/gtest.h"

////////////////////////////////////////////////////////////////////////////////

class MCTapListener : public ::testing::TestEventListener {
public:
	MCTapListener(const char* log_file);
	virtual ~MCTapListener();

	// Fired before any test activity starts.
	virtual void OnTestProgramStart(const ::testing::UnitTest& unit_test);

	// Fired before each iteration of tests starts.  There may be more
	// than one iteration if GTEST_FLAG(repeat) is set. iteration is the
	// iteration index, starting from 0.
	virtual void OnTestIterationStart(
		const ::testing::UnitTest& unit_test,
		int iteration
	);

	// Fired before environment set-up for each iteration of tests starts.
	virtual void OnEnvironmentsSetUpStart(const ::testing::UnitTest& unit_test);

	// Fired after environment set-up for each iteration of tests ends.
	virtual void OnEnvironmentsSetUpEnd(const ::testing::UnitTest& unit_test);

	// Fired before the test case starts.
	virtual void OnTestCaseStart(const ::testing::TestCase& test_case);

	// Fired before the test starts.
	virtual void OnTestStart(const ::testing::TestInfo& test_info);

	// Fired after a failed assertion or a SUCCEED() invocation.
	virtual void OnTestPartResult(
		const ::testing::TestPartResult& test_part_result
	);

	// Fired after the test ends.
	virtual void OnTestEnd(const ::testing::TestInfo& test_info);

	// Fired after the test case ends.
	virtual void OnTestCaseEnd(const ::testing::TestCase& test_case);

	// Fired before environment tear-down for each iteration of tests starts.
	virtual void OnEnvironmentsTearDownStart(
		const ::testing::UnitTest& unit_test
	);

	// Fired after environment tear-down for each iteration of tests ends.
	virtual void OnEnvironmentsTearDownEnd(const ::testing::UnitTest& unit_test);

	// Fired after each iteration of tests finishes.
	virtual void OnTestIterationEnd(
		const ::testing::UnitTest& unit_test,
		int iteration
	);

	// Fired after all test activities have ended.
	virtual void OnTestProgramEnd(const ::testing::UnitTest& unit_test);

private:
	std::ofstream m_log;
	::testing::TestPartResult m_test_part_result;
};

////////////////////////////////////////////////////////////////////////////////
#endif
