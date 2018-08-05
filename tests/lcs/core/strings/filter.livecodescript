script "CoreStringFilter"
/*
Copyright (C) 2015 LiveCode Ltd.

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
on __testFilterRangeAndOptions pIsNative, pLower, pMid, pUpper
	local tTestType
	if pIsNative then
		put "native" into tTestType
	else
		put "non-native" into tTestType
	end if
	
	local tSource, tPattern
	
	# Option tests
	put pLower into tSource
	put "[" & pLower & pMid & "]" into tPattern
	
	filter tSource with tPattern
	TestAssert "filter with matching option pattern" && tTestType, tSource is pLower
	
	put pUpper into tSource
	filter tSource with tPattern
	TestAssert "filter with non-matching option pattern" && tTestType, tSource is empty
	
	put "[!" & pLower & pMid & "]" into tPattern		
	put pUpper into tSource
	filter tSource with tPattern
	TestAssert "filter with matching negated option pattern" && tTestType, tSource is pUpper
	
	# Range tests
	put pMid into tSource
	put "[" & pLower & "-" & pUpper & "]" into tPattern
	filter tSource with tPattern
	TestAssert "filter with match within range pattern" && tTestType, tSource is pMid
	
	put pLower into tSource
	put "[" & pLower & "-" & pMid & "]" into tPattern
	filter tSource with tPattern
	TestAssert "filter with match on range pattern lower bound" && tTestType, tSource is pLower
	
	put pMid into tSource
	filter tSource with tPattern
	TestAssert "filter with match on range pattern upper bound" && tTestType, tSource is pMid
	
	put pUpper into tSource
	filter tSource with tPattern
	TestDiagnostic tSource
	TestAssert "filter with non-match outside range pattern" && tTestType, tSource is empty

	put pUpper into tSource
	put "[!" & pLower & "-" & pMid & "]" into tPattern
	filter tSource with tPattern
	TestAssert "filter with match outside negated range pattern" && tTestType, tSource is pUpper
	
	# Range options
	put pUpper into tSource
	put "[" & pLower & "-" & pMid & pLower & "-" & pUpper & "]" into tPattern
	filter tSource with tPattern
	TestAssert "filter with match within option range pattern" && tTestType, tSource is pUpper
	
	# Range + options
	put pUpper into tSource
	put "[" & pLower & "-" & pMid & pLower & pMid & pUpper & "]" into tPattern
	filter tSource with tPattern
	TestAssert "filter with matching option after range pattern" && tTestType, tSource is pUpper	
end __testFilterRangeAndOptions

on __testFilterWildcard pIsNative, pString
	if pString is empty then
		TestFail "filter wildcard string not valid"
		exit __testFilterWildcard
	end if

	# Single character wildcard
	local tSource, tPattern

	put "?" into tPattern
	
	if pIsNative then
		put return into tSource
		filter tSource with tPattern
		TestAssert "filter single char wildcard doesn't match empty", tSource is empty
	end if
	
	local tTestType
	if pIsNative then
		put "native" into tTestType
	else
		put "non-native" into tTestType
	end if
	
	local tChar
	put any char of pString into tChar
		
	put tChar into tSource
	filter tSource with tPattern
	TestAssert "filter single char wildcard matches single char" && tTestType, tSource is tChar
	
	put tChar & tChar into tSource
	filter tSource with tPattern
	TestAssert "filter single char wildcard doesn't match multiple chars" && tTestType, tSource is empty

	put pString into tPattern
	put "?" into any char of tPattern
	put pString into tSource
	filter tSource with tPattern
	TestAssert "filter single char wildcard matches in string" && tTestType, tSource is pString

	# Asterisk
	put "*" into tPattern
	
	if pIsNative then
		put return into tSource
		filter tSource with tPattern
		TestAssert "filter asterisk matches empty", tSource is return
	end if
	
	put pString into tSource
	filter tSource with tPattern
	TestAssert "filter asterisk matches mulitple chars" && tTestType, tSource is pString
	
	put pString & "**" into tPattern
	put pString into tSource
	filter tSource with tPattern
	TestAssert "filter asterisks at end don't affect match" && tTestType, tSource is pString
	
	put "*" & pString into tPattern
	put pString into tSource
	filter tSource with tPattern
	TestAssert "filter simple asterisk recursion" && tTestType, tSource is pString
end __testFilterWildcard

on TestFilterWildcardNative
	__testFilterRangeAndOptions true, "a", "b", "c"
	__testFilterWildcard true, "abc"
end TestFilterWildcardNative

on TestFilterWildcardNonNative
	local tGClef, tFClef, tCClef
	put numToCodepoint(0x1d11e) into tGClef
	put numToCodepoint(0x1d121) into tCClef
	put numToCodepoint(0x1d122) into tFClef

	__testFilterRangeAndOptions false, tGClef, tCClef, tFClef	
	__testFilterWildcard false, tGClef & tCClef & tFClef
end TestFilterWildcardNonNative

on __testBug12644 pIsNative, pString
	local tTestType
	if pIsNative then
		put "native" into tTestType
	else
		put "non-native" into tTestType
	end if
	
	local tSource, tPattern
	
	put "*-U-*" into tPattern
	put "01-A-001" & pString into tSource
	
	filter tSource with tPattern
	TestAssert "filter false positive bug 12644", tSource is empty
end __testBug12644

on TestBug12644
	__testBug12644 true, "word"
	__testBug12644 false, "뤻뤼뤾"
end TestBug12644

on __testBug16584 pIsNative, pString
	local tTestType
	if pIsNative then
		put "native" into tTestType
	else
		put "non-native" into tTestType
	end if
	
	local tSource, tPattern
	
	put pString into tPattern
	put "*" into char 1 of tPattern
	put "*" into char 3 to 5 of tPattern
	put pString into tSource
	
	filter tSource with tPattern
	TestAssert "filter asterisk false negative bug 16584", tSource is pString
end __testBug16584

on TestBug16584
	__testBug16584 true, "apple"
	__testBug16584 false, "뤻뤼뤼뤾룜"	
end TestBug16584

on TestBug15443
	local tTest
	put "NÖJE" into tTest
	filter tTest with "nöje"
	TestAssert "filter macroman native bug 15443", tTest is not empty
end TestBug15443

on TestFilterExpression
	local tSource, tTest
	put "foo,bar,baz" into tSource
	
	filter items of tSource where each begins with "f" into tTest
	TestAssert "filter items of string", tTest is "foo"
end TestFilterExpression
