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

#include "MCTapListener.h"

#include <iostream>

using ::testing::UnitTest;
using ::testing::TestCase;
using ::testing::TestInfo;
using ::testing::TestResult;
using ::testing::TestPartResult;


MCTapListener::MCTapListener(const char* log_file)
	: m_log(log_file, std::ofstream::out),
	  m_test_part_result(TestPartResult::kSuccess, "", 0, "")
{
	if (!m_log.good()) {
		std::cerr
			<< "Failed to open TAP log file ("
			<< log_file
			<< ")"
			<< std::endl;
	}
}


MCTapListener::~MCTapListener()
{
}


// Fired before any test activity starts.
void MCTapListener::OnTestProgramStart(const UnitTest& unit_test)
{
}


// Fired before each iteration of tests starts.  There may be more
// than one iteration if GTEST_FLAG(repeat) is set. iteration is the
// iteration index, starting from 0.
void MCTapListener::OnTestIterationStart(
	const UnitTest& unit_test,
	int iteration
)
{
	int total_count = unit_test.total_test_count();
	if (m_log.good() && total_count > 0) {
		m_log << "1.." << total_count << std::endl;
	}
}


// Fired before environment set-up for each iteration of tests starts.
void MCTapListener::OnEnvironmentsSetUpStart(const UnitTest& unit_test)
{
}


// Fired after environment set-up for each iteration of tests ends.
void MCTapListener::OnEnvironmentsSetUpEnd(const UnitTest& unit_test)
{

}


// Fired before the test case starts.
void MCTapListener::OnTestCaseStart(const TestCase& test_case)
{
	if (m_log.good()) {
		m_log << "\n### " << test_case.name() << "\n" << std::endl;
	}
}


// Fired before the test starts.
void MCTapListener::OnTestStart(const TestInfo& test_info)
{

}


// Fired after a failed assertion or a SUCCEED() invocation.
void MCTapListener::OnTestPartResult(const TestPartResult& test_part_result)
{
	m_test_part_result = test_part_result;
}


// Fired after the test ends.
void MCTapListener::OnTestEnd(const TestInfo& test_info)
{
	if (m_log.good()) {

		const TestResult* test_result = test_info.result();

		if (test_result->Passed()) {
			m_log << "ok - ";
		} else {
			m_log << "not ok - ";
		}

		m_log << test_info.test_case_name() << "." << test_info.name() << "\n";

		if (test_result->Passed()) {
			m_log
				<< "# elapsed time "
				<< test_result->elapsed_time()
				<< "ms";

		} else {
			m_log
				<< "# "
				<< m_test_part_result.file_name()
				<< ":"
				<< m_test_part_result.line_number()
				<< "\n#    ";

			const std::string& message = m_test_part_result.message();
			for (std::string::const_iterator it = message.begin();
				 it != message.end();
				 ++it) {
				if (*it == '\n') {
					m_log << "\n#    ";
				} else {
					m_log << *it;
				}
			}
		}

		m_log << "\n" << std::endl;
	}
}


// Fired after the test case ends.
void MCTapListener::OnTestCaseEnd(const TestCase& test_case)
{
	if (m_log.good()) {
		m_log
			<< "### total elapsed time "
			<< test_case.elapsed_time()
			<< "ms\n"
			<< std::endl;
	}
}


// Fired before environment tear-down for each iteration of tests
// starts.
void MCTapListener::OnEnvironmentsTearDownStart(const UnitTest& unit_test)
{
}


// Fired after environment tear-down for each iteration of tests ends.
void MCTapListener::OnEnvironmentsTearDownEnd(const UnitTest& unit_test)
{
}


// Fired after each iteration of tests finishes.
void MCTapListener::OnTestIterationEnd(
	const UnitTest& unit_test,
	int iteration
)
{
}


// Fired after all test activities have ended.
void MCTapListener::OnTestProgramEnd(const UnitTest& unit_test)
{
}
