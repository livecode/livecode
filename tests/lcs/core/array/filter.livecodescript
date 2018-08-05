script "CoreArrayFilter"
/*
Copyright (C) 2016 LiveCode Ltd.

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

/*
	Filter wildcard special characters:
	* - matches any number of occurrences of any character
	? - matches a single occurrence of any character
	[abc] - matches any of a, b or c
	[a-m] - matches any lowercase character in the first half of the alphabet
	[!abc] - matches any character which is not a, b or c
	
	The last three of these can be combined to create more complex character classes, for
	example [!abcA-C] - matches any character which is not a, b or c (upper or lower case)
*/
on __testFilterRangeAndOptions pIsNative, pLower, pMid, pUpper, pKeys
	local tTestType
	if pIsNative then
		put "native" into tTestType
	else
		put "non-native" into tTestType
	end if
	
	if pKeys then
		put "array keys " before tTestType
	else
		put "array elements " before tTestType
	end if
	
	local tSource, tPattern
	
	# Option tests
	put pLower into tSource[pLower]
	put "[" & pLower & pMid & "]" into tPattern
	
	if pKeys then
	   filter keys of tSource with tPattern
	else
		filter elements of tSource with tPattern
	end if
	TestAssert "filter with matching option pattern" && tTestType, the keys of tSource is pLower
	
	put empty into tSource
	put pUpper into tSource[pUpper]
	if pKeys then
	   filter keys of tSource with tPattern
	else
		filter elements of tSource with tPattern
	end if
	TestAssert "filter with non-matching option pattern" && tTestType, the keys of tSource is empty
	
	put "[!" & pLower & pMid & "]" into tPattern		
	put empty into tSource
	put pUpper into tSource[pUpper]
	if pKeys then
	   filter keys of tSource with tPattern
	else
		filter elements of tSource with tPattern
	end if
	TestAssert "filter with matching negated option pattern" && tTestType, the keys of tSource is pUpper
	
	# Range tests
	put empty into tSource
	put pMid into tSource[pMid]
	put "[" & pLower & "-" & pUpper & "]" into tPattern
	if pKeys then
	   filter keys of tSource with tPattern
	else
		filter elements of tSource with tPattern
	end if
	TestAssert "filter with match within range pattern" && tTestType, the keys of tSource is pMid
	
	put empty into tSource
	put pLower into tSource[pLower]
	put "[" & pLower & "-" & pMid & "]" into tPattern
	if pKeys then
	   filter keys of tSource with tPattern
	else
		filter elements of tSource with tPattern
	end if
	TestAssert "filter with match on range pattern lower bound" && tTestType, the keys of tSource is pLower
	
	put empty into tSource
	put pMid into tSource[pMid]
	if pKeys then
	   filter keys of tSource with tPattern
	else
		filter elements of tSource with tPattern
	end if
	TestAssert "filter with match on range pattern upper bound" && tTestType, the keys of tSource is pMid
	
	put empty into tSource
	put pUpper into tSource[pUpper]
	if pKeys then
	   filter keys of tSource with tPattern
	else
		filter elements of tSource with tPattern
	end if
	TestDiagnostic tSource
	TestAssert "filter with non-match outside range pattern" && tTestType, the keys of tSource is empty

	put empty into tSource
	put pUpper into tSource[pUpper]
	put "[!" & pLower & "-" & pMid & "]" into tPattern
	if pKeys then
	   filter keys of tSource with tPattern
	else
		filter elements of tSource with tPattern
	end if
	TestAssert "filter with match outside negated range pattern" && tTestType, the keys of tSource is pUpper
	
	# Range options
	put empty into tSource
	put pUpper into tSource[pUpper]
	put "[" & pLower & "-" & pMid & pLower & "-" & pUpper & "]" into tPattern
	if pKeys then
	   filter keys of tSource with tPattern
	else
		filter elements of tSource with tPattern
	end if
	TestAssert "filter with match within option range pattern" && tTestType, the keys of tSource is pUpper
	
	# Range + options
	put empty into tSource
	put pUpper into tSource[pUpper]
	put "[" & pLower & "-" & pMid & pLower & pMid & pUpper & "]" into tPattern
	if pKeys then
	   filter keys of tSource with tPattern
	else
		filter elements of tSource with tPattern
	end if
	TestAssert "filter with matching option after range pattern" && tTestType, the keys of tSource is pUpper	
end __testFilterRangeAndOptions

on __testFilterWildcard pIsNative, pString, pKeys
	if pString is empty then
		TestFail "filter wildcard string not valid"
		exit __testFilterWildcard
	end if

	# Single character wildcard
	local tSource, tPattern, tTestType
	
	if pKeys then
		put "array keys" into tTestType
	else
		put "array elements" into tTestType
	end if

	put "?" into tPattern
	
	if pIsNative then
		put empty into tSource[empty]
		if pKeys then
		   filter keys of tSource with tPattern
		else
			filter elements of tSource with tPattern
		end if
		TestAssert "filter single char wildcard doesn't match empty" && tTestType, the keys of tSource is empty
	end if
	
	if pIsNative then
		put " native" after tTestType
	else
		put " non-native" after tTestType
	end if
	
	local tChar
	put any char of pString into tChar
	put empty into tSource
	put tChar into tSource[tChar]
	if pKeys then
		filter keys of tSource with tPattern
	else
		filter elements of tSource with tPattern
	end if
	TestAssert "filter single char wildcard matches single char" && tTestType, the keys of tSource is tChar
	
	put empty into tSource
	put tChar & tChar into tSource[tChar & tChar]
	filter tSource with tPattern
	TestAssert "filter single char wildcard doesn't match multiple chars" && tTestType, the keys of tSource is empty

	put pString into tPattern
	put "?" into any char of tPattern
	put pString into tSource[pString]
	if pKeys then
		filter keys of tSource with tPattern
	else
		filter elements of tSource with tPattern
	end if
	TestAssert "filter single char wildcard matches in string" && tTestType, the keys of tSource is pString

	# Asterisk
	put "*" into tPattern
	
	if pIsNative then
		put empty into tSource
		put empty into tSource[empty]
		if pKeys then
			filter keys of tSource with tPattern
		else
			filter elements of tSource with tPattern
		end if
		TestAssert "filter asterisk matches empty", the number of elements of tSource is 1
	end if
	
	put empty into tSource
	put pString into tSource[pString]
	if pKeys then
		filter keys of tSource with tPattern
	else
		filter elements of tSource with tPattern
	end if
	TestAssert "filter asterisk matches multiple chars" && tTestType, the keys of tSource is pString
	
	put pString & "**" into tPattern
	put empty into tSource
	put pString into tSource[pString]
	if pKeys then
		filter keys of tSource with tPattern
	else
		filter elements of tSource with tPattern
	end if
	TestAssert "filter asterisks at end don't affect match" && tTestType, the keys of tSource is pString
	
	put "*" & pString into tPattern
	put empty into tSource
	put pString into tSource[pString]
	if pKeys then
		filter keys of tSource with tPattern
	else
		filter elements of tSource with tPattern
	end if
	TestAssert "filter simple asterisk recursion" && tTestType, the keys of tSource is pString
end __testFilterWildcard

on TestFilterWildcardNative
	__testFilterRangeAndOptions true, "a", "b", "c", false
	__testFilterWildcard true, "abc", false
	__testFilterRangeAndOptions true, "a", "b", "c", true
	__testFilterWildcard true, "abc", true
end TestFilterWildcardNative

on TestFilterWildcardNonNative
	local tGClef, tFClef, tCClef
	put numToCodepoint(0x1d11e) into tGClef
	put numToCodepoint(0x1d121) into tCClef
	put numToCodepoint(0x1d122) into tFClef

	__testFilterRangeAndOptions false, tGClef, tCClef, tFClef, true
	__testFilterWildcard false, tGClef & tCClef & tFClef, true
	__testFilterRangeAndOptions false, tGClef, tCClef, tFClef, false
	__testFilterWildcard false, tGClef & tCClef & tFClef, false
end TestFilterWildcardNonNative

on TestFilterSourceConversion
	local tSource, tTest
	put "foo" into tSource["foo"]
	put "bar" into tSource["bar"]
	
	filter tSource with "f*" into tTest
	TestAssert "filter implicit lines of array source", tTest is empty
	
	filter lines of tSource with "f*" into tTest
	TestAssert "filter explicit lines of array source", tTest is empty
	
	filter tSource with "f*" into tTest
	TestAssert "filter explicit items of array source", tTest is empty
	
	put "foo" & return & "bar" into tSource
	
	filter keys of tSource with "f*" into tTest
	TestAssert "filter explicit keys of string source", tTest is empty
	
	filter elements of tSource with "f*" into tTest
	TestAssert "filter explicit elements of string source", tTest is empty
end TestFilterSourceConversion

on TestFilterRegex
	local tSource, tTest
	put "foo" into tSource["foo"]
	put "bar" into tSource["bar"]
	
	filter keys of tSource with regex pattern "f.*" into tTest
	TestAssert "filter keys of array with regex", the keys of tTest is "foo"
	
	put "foo" into tSource["foo"]
	put "bar" into tSource["bar"]
	
	filter elements of tSource with regex pattern "f.*" into tTest
	TestAssert "filter elements of array with regex", the keys of tTest is "foo"
end TestFilterRegex

on TestFilterExpression
	local tSource, tTest
	put "foo" into tSource["foo"]
	put "bar" into tSource["bar"]
	
	filter keys of tSource where each begins with "f" into tTest
	TestAssert "filter keys of array where expression", the keys of tTest is "foo"
	
	put "foo" into tSource["foo"]
	put "bar" into tSource["bar"]
	
	filter elements of tSource where each begins with "f" into tTest
	TestAssert "filter elements of array where expression", the keys of tTest is "foo"
end TestFilterExpression
