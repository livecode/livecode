script "CoreFieldLineProperties"
/*
Copyright (C) 2019 LiveCode Ltd.

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

local sTests

on TestSetup
	_AddTest true, "textAlign", "center", "right"
	_AddTest false, "listStyle", "square", "circle"
	_AddTest false, "listDepth", "5", "10"
	_AddTest true, "vGrid", "true", "false"
	_AddTest true, "hGrid", "true", "false"
	_AddTest true, "dontWrap", "true", "false"
	_AddTest false, "borderWidth", "5", "10"
	_AddTest true, "firstIndent", "5", "10"
	_AddTest false, "leftIndent", "5", "10"
	_AddTest false, "rightIndent", "5", "10"
	_AddTest false, "spaceAbove", "5", "10"
	_AddTest false, "spaceBelow", "5", "10"
	_AddTest true, "tabStops", "100", "200"
	_AddTest true, "tabWidths", "100", "200"
	_AddTest true, "tabAlign", "center", "right"
	_AddTest false, "listIndex", "10", "20"
	_AddTest false, "backgroundColor", "255,0,0", "0,0,255"
	_AddTest false, "borderColor", "255,0,0", "0,0,255"
end TestSetup

private command _AddTest pFieldAndLine, pProp, pValue1, pValue2
	local tTest
	put pFieldAndLine into tTest["effective"]
	put pProp into tTest["property"]
	put pValue1 into tTest["value-1"]
	put pValue2 into tTest["value-2"]
	put tTest into sTests[the number of elements in sTests + 1]
end _AddTest

on TestEqualLineProperties
	repeat for each element tTest in sTests
		_TestEqualLineProperty tTest["property"], tTest["value-1"]
	end repeat

	/* metadata can't be mixed so handle separately */
	_TestEqualLineProperty "metadata", "foo"
end TestEqualLineProperties

private command _TestEqualLineProperty pProp, pValue
	create field "Test"

	repeat with tExtra = 0 to 4
		local tValid
		put 0 into tValid
		repeat with tStart = 1 to 5 - tExtra
			set the text of field "Test" to format("first\nsecond\nthird\nfourth\nfifth")
			set the pProp of line tStart to tStart + tExtra of field "Test" to pValue
			if the pProp of line tStart to tStart + tExtra of field "Test" is pValue then
				add 1 to tValid
			end if
		end repeat
		TestAssert format("roundtrip equal %s over %d lines", pProp, tExtra + 1), tValid is 5 - tExtra
	end repeat

	delete field "Test"
end _TestEqualLineProperty

on TestEqualEffectiveLineProperties
	repeat for each element tTest in sTests
		if not tTest["effective"] then
			next repeat
		end if
		_TestEqualEffectiveLineProperties tTest["property"], tTest["value-1"]
	end repeat
end TestEqualEffectiveLineProperties

on _TestEqualEffectiveLineProperties pProp, pValue
	create field "Test"

	set the pProp of field "Test" to pValue
	set the text of field "Test" to format("first\nsecond\nthird\nfourth\nfifth")
	repeat with i = 1 to 5
		if (i mod 2) is 1 then
			set the pProp of line i of field "Test" to pValue
		end if
	end repeat

	repeat with tExtra = 1 to 4
		local tValid
		put 0 into tValid
		repeat with tStart = 1 to 5 - tExtra
			if the effective pProp of line tStart to tStart + tExtra of field "Test" is pValue then
				add 1 to tValid
			end if
		end repeat
		TestAssert format("roundtrip effective equal %s over %d lines", pProp, tExtra + 1), tValid is 5 - tExtra

		put 0 into tValid
		repeat with tStart = 1 to 5 - tExtra
			if the pProp of line tStart to tStart + tExtra of field "Test" is "mixed" then
				add 1 to tValid
			end if
		end repeat
		TestAssert format("roundtrip non-effective equal %s over %d lines", pProp, tExtra + 1), tValid is 5 - tExtra
	end repeat

	delete field "Test"
end _TestEqualEffectiveLineProperties

on TestMixedLineProperties
	repeat for each element tTest in sTests
		_TestMixedLineProperty tTest["property"], tTest["value-1"], tTest["value-2"]
	end repeat
end TestMixedLineProperties

private command _TestMixedLineProperty pProp, pValue1, pValue2
	create field "Test"

	set the text of field "Test" to format("first\nsecond\nthird\nfourth\nfifth")
	repeat with i = 1 to 5
		if (i mod 2) is 1 then
			set the pProp of line i of field "Test" to pValue1
		else
			set the pProp of line i of field "Test" to pValue2
		end if
	end repeat

	repeat with tExtra = 1 to 4
		local tValid
		put 0 into tValid
		repeat with tStart = 1 to 5 - tExtra
			if the pProp of line tStart to tStart + tExtra of field "Test" is "mixed" then
				add 1 to tValid
			end if
		end repeat
		TestAssert format("roundtrip mixed %s over %d lines", pProp, tExtra + 1), tValid is 5 - tExtra
	end repeat

	delete field "Test"
end _TestMixedLineProperty

on TestMixedEffectiveLineProperties
	repeat for each element tTest in sTests
		if not tTest["effective"] then
			next repeat
		end if
		_TestMixedEffectiveLineProperties tTest["property"], tTest["value-1"], tTest["value-2"]
	end repeat
end TestMixedEffectiveLineProperties

on _TestMixedEffectiveLineProperties pProp, pValue1, pValue2
	create field "Test"

	set the pProp of field "Test" to pValue2
	set the text of field "Test" to format("first\nsecond\nthird\nfourth\nfifth")
	repeat with i = 1 to 5
		if (i mod 2) is 1 then
			set the pProp of line i of field "Test" to pValue1
		end if
	end repeat

	repeat with tExtra = 1 to 4
		local tValid
		put 0 into tValid
		repeat with tStart = 1 to 5 - tExtra
			if the effective pProp of line tStart to tStart + tExtra of field "Test" is "mixed" then
				add 1 to tValid
			end if
		end repeat
		TestAssert format("roundtrip effective mixed %s over %d lines", pProp, tExtra + 1), tValid is 5 - tExtra

		put 0 into tValid
		repeat with tStart = 1 to 5 - tExtra
			if the pProp of line tStart to tStart + tExtra of field "Test" is "mixed" then
				add 1 to tValid
			end if
		end repeat
		TestAssert format("roundtrip non-effective mixed %s over %d lines", pProp, tExtra + 1), tValid is 5 - tExtra
	end repeat

	delete field "Test"
end _TestMixedEffectiveLineProperties

on TestLinePropertyRoundTrip
	repeat for each element tTest in sTests
		if not tTest["effective"] then
			next repeat
		end if

		local tValues
		put tTest["value-1"] into tValues[1]
		put tTest["value-2"] into tValues[2]

		TestPropRoundTrip "field", tTest["property"], tValues
	end repeat
end TestLinePropertyRoundTrip
