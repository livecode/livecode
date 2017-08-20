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
   TestSkipIfNot "lcb"
   TestLoadAuxillaryExtension "_valueutils"
   wait 1 second
end TestSetup

on TestSequenceLiteral
	local tConstantLiteral
	put [ true, 1, pi, "Hello", [ true, 1, pi, "Hello" ] ] into tConstantLiteral
	TestAssert "constant literal creation works", tConstantLiteral[1] is true and \
												  tConstantLiteral[2] is 1 and \
												  tConstantLiteral[3] is pi and \
												  tConstantLiteral[4] is "Hello" and \
												  tConstantLiteral[5] is [ true, 1, pi, "Hello", [ true, 1, pi, "Hello" ] ]

	--TestAssert "constant literal is interred", IsValueUnique(tConstantLiteral)
	--TestAssert "constant literal is unique", IsUniquelyEqualTo(tConstantLiteral, [ true, 1, pi, "hello" ])

	local tDynamicLiteral
	put [ _Identity(true), _Identity(1), _Identity(pi), _Identity("Hello"), [ _Identity(true), _Identity(1), _Identity(pi), _Identity("Hello") ] ] into tDynamicLiteral
	TestAssert "dynamic literal creation works", tDynamicLiteral[1] is true and \
												 tDynamicLiteral[2] is 1 and \
												 tDynamicLiteral[3] is pi and \
												 tDynamicLiteral[4] is "Hello" and \
												 tDynamicLiteral[5] is [ _Identity(true), _Identity(1), _Identity(pi), _Identity("Hello") ]

	local tConstTime
	put the long seconds into tConstTime
	repeat 100000 times
		--get [ true, 1, pi, "Hello", [ true, 1, pi, "Hello" ] ] is [ true, 1, pi, "Hello", [ true, 1, pi, "Hello" ] ]
		get tConstantLiteral is tConstantLiteral --[ true, 1, pi, "Hello", [ true, 1, pi, "Hello" ] ]
	end repeat
	put the long seconds - tConstTime into tConstTime

	local tDynTime
	put the long seconds into tDynTime
	repeat 100000 times
		get tDynamicLiteral is [ true, 1, pi, "Hello", [ true, 1, pi, "Hello" ] ]
	end repeat
	put the long seconds - tDynTime into tDynTime

	TestDiagnostic tConstTime && tDynTime
end TestSequenceLiteral

on TestArrayLiteral
	local tDynamicLiteral
	put { _Identity("true"): _Identity(true), \
	      _Identity("1"): _Identity(1), \
	      _Identity("pi"): _Identity(pi), \
	      _Identity("Hello"): _Identity("Hello") } into tDynamicLiteral
	TestAssert "dynamic literal creation works", tDynamicLiteral["true"] is true and \
												 tDynamicLiteral["1"] is 1 and \
												 tDynamicLiteral["pi"] is pi and \
												 tDynamicLiteral["Hello"] is "Hello"
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
