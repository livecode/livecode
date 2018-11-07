script "CoreArrayLiterals"
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

on TestSetup
   /*
   NOTE: Due to bridging effects - this is useless at the moment to check
         for internment / uniquing.
   TestSkipIfNot "lcb"
   TestLoadAuxiliaryExtension "_valueutils"
   wait 1 second
   */
end TestSetup

on TestSequenceLiteral
	local tConstantLiteral
	put [ true, 1, pi, "Hello", [ true, 1, pi, "Hello" ] ] into tConstantLiteral
	TestAssert "constant seq literal creation works", tConstantLiteral[1] is true and \
													  tConstantLiteral[2] is 1 and \
													  tConstantLiteral[3] is pi and \
													  tConstantLiteral[4] is "Hello" and \
													  tConstantLiteral[5] is [ true, 1, pi, "Hello" ]

	local tDynamicLiteral
	put [ _Identity(true), _Identity(1), _Identity(pi), _Identity("Hello"), [ _Identity(true), _Identity(1), _Identity(pi), _Identity("Hello") ] ] into tDynamicLiteral
	TestAssert "dynamic seq literal creation works", tDynamicLiteral[1] is true and \
													 tDynamicLiteral[2] is 1 and \
													 tDynamicLiteral[3] is pi and \
													 tDynamicLiteral[4] is "Hello" and \
													 tDynamicLiteral[5] is [ _Identity(true), _Identity(1), _Identity(pi), _Identity("Hello") ]
end TestSequenceLiteral

on TestArrayLiteral
	local tConstantLiteral
	put { "true": true, \
		  "1": 1, \
		  "pi": pi, \
		  "Hello": "Hello" } into tConstantLiteral
	TestAssert "constant arr literal creation works", tConstantLiteral["true"] is true and \
													  tConstantLiteral["1"] is 1 and \
													  tConstantLiteral["pi"] is pi and \
													  tConstantLiteral["Hello"] is "Hello"

	local tDynamicLiteral
	put { "true": _Identity(true), \
	      "1": _Identity(1), \
	      "pi": _Identity(pi), \
	      "Hello": _Identity("Hello") } into tDynamicLiteral
	TestAssert "dynamic arr literal creation works", tDynamicLiteral["true"] is true and \
													 tDynamicLiteral["1"] is 1 and \
													 tDynamicLiteral["pi"] is pi and \
													 tDynamicLiteral["Hello"] is "Hello"

	local tSemiDynamicLiteral
	put { "true": true, \
		  "1": 1, \
	      "pi": _Identity(pi), \
	      "Hello": _Identity("Hello") } into tSemiDynamicLiteral
	TestAssert "semi-dynamic arr literal creation works", tSemiDynamicLiteral["true"] is true and \
															 tSemiDynamicLiteral["1"] is 1 and \
															 tSemiDynamicLiteral["pi"] is pi and \
															 tSemiDynamicLiteral["Hello"] is "Hello"
/*
	local tCSDynamicLiteral
	put { "true": true, \
		  "1": 1, \
		  "hello": "hello", \
	      "pi": _Identity(pi), \
	      "Hello": _Identity("Hello") } into tCSDynamicLiteral
	TestAssert "cs-dynamic (ci) literal creation works", tCSDynamicLiteral["true"] is true and \
															 tCSDynamicLiteral["1"] is 1 and \
															 tCSDynamicLiteral["pi"] is pi and \
															 tCSDynamicLiteral["Hello"] is "Hello" and \
															 the number of elements in tCSDynamicLiteral is 4

	set the caseSensitive to true
	put { "true": true, \
		  "1": 1, \
		  "hello": "hello", \
	      "pi": _Identity(pi), \
	      "Hello": _Identity("Hello") } into tCSDynamicLiteral
	TestAssert "cs-dynamic (cs) arr literal creation works", tCSDynamicLiteral["true"] is true and \
															 tCSDynamicLiteral["1"] is 1 and \
															 tCSDynamicLiteral["pi"] is pi and \
															 tCSDynamicLiteral["hello"] is "hello" and \
															 tCSDynamicLiteral["Hello"] is "Hello" and \
															 the number of elements in tCSDynamicLiteral is 5
*/
end TestArrayLiteral

/* _Identity just returns the input value. However, it has to be considered a
 * dynamic function as it uses the message path. */
function _Identity pValue
	return pValue
end _Identity

function SeqToString pValue
	combine pValue by comma
	return pValue
end SeqToString
