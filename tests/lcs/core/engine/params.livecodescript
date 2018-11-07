script "CoreEngineCommandParams"
/*
Copyright (C) 2017 LiveCode Ltd.

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

command TestParams
	local tExpectedParams
	put format("params") into tExpectedParams
	TestAssert "params() returns stringified params", _TestParams(1, "foo", empty) is format("_TestParams(\"1\",\"foo\",\"\")")

	local tExpectedAllParams
	put 1 into tExpectedAllParams[1]
	put "foo" into tExpectedAllParams[2]
	put "bar" into tExpectedAllParams[3]
	put empty into tExpectedAllParams[4]
	TestAssert "params(1) returns all params", _TestParamsFromOne(1, "foo", "bar", empty) is tExpectedAllParams
	TestAssert "params(2) returns no params when one passed", _TestParamsFromTwo(1) is empty
	TestAssert "params(1, the paramCount) returns all params", _TestParamsFromOneToParamCount(1, "foo", "bar", empty) is tExpectedAllParams
	TestAssert "params(-the paramCount) returns all params", _TestParamsFromMinusParamCount(1, "foo", "bar", empty) is tExpectedAllParams
	TestAssert "params(-the paramCount, -1) returns all params", _TestParamsFromMinusParamCountToMinusOne(1, "foo", "bar", empty) is tExpectedAllParams
	TestAssert "params(0, the paramCount + 1) returns all params", _TestParamsFromZeroToParamCountPlusOne(1, "foo", "bar", empty) is tExpectedAllParams

	local tExpectedMiddleParams
	put "foo" into tExpectedMiddleParams[1]
	put "bar" into tExpectedMiddleParams[2]
	TestAssert "params(2, 3) returns middle two params", _TestParamsFromTwoToThree(1, "foo", "bar", empty) is tExpectedMiddleParams
	TestAssert "params(-3, -2) returns middle two params", _TestParamsFromMinusThreeToMinusTwo(1, "foo", "bar", empty) is tExpectedMiddleParams
	
	TestAssert "params(3, 2) returns no params", _TestParamsFromThreeToTwo(1, "foo", "bar", empty) is empty
	TestAssert "params(-2, -3) returns no params", _TestParamsFromMinusTwoToMinusThree(1, "foo", "bar", empty) is empty

	TestAssert "params(2, 3) returns no params when none passed", _TestParamsFromTwoToThree() is empty
end TestParams

private function _TestParams
	return the params
end _TestParams

private function _TestParamsFromOne
	return params(1)
end _TestParamsFromOne

private function _TestParamsFromTwo
	return params(2)
end _TestParamsFromTwo

private function _TestParamsFromOneToParamCount
	return params(1, the paramCount)
end _TestParamsFromOneToParamCount

private function _TestParamsFromMinusParamCount
	return params(-the paramCount)
end _TestParamsFromMinusParamCount

private function _TestParamsFromMinusParamCountToMinusOne
	return params(-the paramCount, -1)
end _TestParamsFromMinusParamCountToMinusOne

private function _TestParamsFromZeroToParamCountPlusOne
	return params(0, the paramCount + 1)
end _TestParamsFromZeroToParamCountPlusOne

private function _TestParamsFromTwoToThree
	return params(2, 3)
end _TestParamsFromTwoToThree

private function _TestParamsFromMinusThreeToMinusTwo
	return params(-3, -2)
end _TestParamsFromMinusThreeToMinusTwo

private function _TestParamsFromThreeToTwo
	return params(3, 2)
end _TestParamsFromThreeToTwo

private function _TestParamsFromMinusTwoToMinusThree
	return params(-2, -3)
end _TestParamsFromMinusTwoToMinusThree
