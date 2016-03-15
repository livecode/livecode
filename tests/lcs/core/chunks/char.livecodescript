script "CoreChunksChar"
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

on TestCharRanges
 TestAssert "overlong prefix", char -4 to -2 of "foo" is "fo"
 TestAssert "overlong suffix", char 2 to 4 of "foo" is "oo"
end TestCharRanges

on TestCharRangesEmpty
  TestAssert "empty prefix", char -5 to -4 of "foo" is ""
  TestAssert "empty suffix", char 4 to 5 of "foo" is ""
  TestAssert "empty range", char 5 to 4 of "foo" is ""
end TestCharRangesEmpty

on TestCharRangesSingle
  TestAssert "single char range", char 1 to 1 of "foo" is "f"
  TestAssert "single char range from back", char -3 to -3 of "foo" is "f"
end TestCharRangesSingle

on TestCharExpressions
  TestAssert "single char", char 1 of foo is "f"
  TestAssert "single char", char -3 of foo is "f"
end TestCharExpressions

on TestCharExpressionsEmpty
  TestAssert "single char", char 4 of foo is ""
  TestAssert "single char", char -4 of foo is ""
end TestCharExpressionsEmpty

on TestBug16124
 TestAssert "char -2 to -1 of 7 is 7", char -2 to -1 of 7 is 7
end TestBug16124

constant kOther = "0x1D11E" // MUSICAL SYMBOL G CLEF
constant kControl = "0x12" // DEVICE CONTROL TWO
constant kRegional = "0x1F1E6" // REGIONAL INDICATOR SYMBOL LETTER A
constant kSpacing = "0x0E33" // THAI CHARACTER SARA AM

constant kExtend = "0x200C" // ZERO-WIDTH NON-JOINER
constant kPrepend = "0x0" // No characters in Prepend class yet

constant kHangulL = "0x1100" // HANGUL CHOSEONG KIYEOK
constant kHangulV = "0x11A2" // HANGUL CHOSEONG SSANGYEORINHIEUH
constant kHangulT = "0xD7FB" // HANGUL JONGSEONG PHIEUPH-THIEUTH
constant kHangulLV = "0xAC1C" // HANGUL SYLLABLE GAE
constant kHangulLVT = "0xAC02" // HANGUL SYLLABLE GAGG

constant kFirstCharType = 0
constant kOtherIndex = 0
constant kCRIndex = 1
constant kLFIndex = 2
constant kControlIndex = 3
constant kRegionalIndex = 4
constant kSpacingIndex = 5

constant kExtendIndex = 6

constant kFirstHangulSyllable = 7
constant kHangulLIndex = 7
constant kHangulVIndex = 8
constant kHangulTIndex = 9
constant kHangulLVIndex = 10
constant kHangulLVTIndex = 11
constant kLastHangulSyllable = 11

constant kLastCharType = 11

-- No characters in Prepend class yet
constant kPrependIndex = 12

private function itemSum pLine
	return item 1 of pLine + item 2 of pLine
end itemSum

private on DoGraphemeTest pString, pLeftDesc, pRightDesc, pShouldBreak
	local tAssert
	put "break between" && pLeftDesc && "and" && pRightDesc into tAssert

	if pShouldBreak then	
		TestAssert "count check:" && tAssert, the number of chars in pString is 2
		TestAssert "chunk check:" && tAssert, char 1 of pString is not pString
	else
		TestAssert "count check: don't" && tAssert, the number of chars in pString is 1
		TestAssert "chunk check: don't" && tAssert, char 1 of pString is pString
	end if
end DoGraphemeTest

private function isHangul pIndex
	return pIndex >= kFirstHangulSyllable and pIndex <= kLastHangulSyllable
end isHangul

on TestGraphemeBoundaries
	local tControl, tRegional, tSpacingMark
	local tExtend, tPrepend
	local tHangulL, tHangulV, tHangulT, tHangulLV, tHangulLVT
		
	local tArray

	put CR into tArray[kCRIndex]["char"]
	put LF into tArray[kLFIndex]["char"]
	
	put numToCodepoint(kOther) into tArray[kOtherIndex]["char"]
	
	put numToCodepoint(kControl) into tArray[kControlIndex]["char"]
	put numToCodepoint(kRegional) into tArray[kRegionalIndex]["char"]
	put numToCodepoint(kSpacing) into tArray[kSpacingIndex]["char"]
	
	put numToCodepoint(kExtend) into tArray[kExtendIndex]["char"]
	put numToCodepoint(kPrepend) into tArray[kPrependIndex]["char"]
	
	put numToCodepoint(kHangulL) into tArray[kHangulLIndex]["char"]
	put numToCodepoint(kHangulV) into tArray[kHangulVIndex]["char"]
	put numToCodepoint(kHangulT) into tArray[kHangulTIndex]["char"]
	put numToCodepoint(kHangulLV) into tArray[kHangulLVIndex]["char"]
	put numToCodepoint(kHangulLVT) into tArray[kHangulLVTIndex]["char"]
	
	put "carriage return" into tArray[kCRIndex]["description"]
	put "line feed" into tArray[kLFIndex]["description"]
	
	put "other" into tArray[kOtherIndex]["description"]
	
	put "control character" into tArray[kControlIndex]["description"]
	put "regional indicator" into tArray[kRegionalIndex]["description"]
	put "spacing mark" into tArray[kSpacingIndex]["description"]
	
	put "extend character" into tArray[kExtendIndex]["description"]
	put "prepend character" into tArray[kPrependIndex]["description"]
	
	put "hangul leading jamo" into tArray[kHangulLIndex]["description"]
	put "hangul vowel jamo" into tArray[kHangulVIndex]["description"]
	put "hangul trailing jamo" into tArray[kHangulTIndex]["description"]
	put "hangul LV syllable" into tArray[kHangulLVIndex]["description"]
	put "hangul LVT syllable" into tArray[kHangulLVTIndex]["description"]
	
	local tToTest
	-- Sort pairs in lexicographic order
	repeat with x = kFirstCharType to kLastCharType	
		repeat with y = kFirstCharType to kLastCharType
			if tToTest is empty then
				put x,y into tToTest
			else
				put return & x,y after tToTest
			end if
		end repeat
	end repeat

	sort tToTest ascending numeric by item 1 of each
	sort tToTest ascending numeric by itemSum(each)
	
	repeat for each line tLine in tToTest		
		local tLeft, tRight
		put item 1 of tLine into tLeft
		put item 2 of tLine into tRight
		
		local tLDesc, tRDesc
		put tArray[tLeft]["description"] into tLDesc
		put tArray[tRight]["description"] into tRDesc
		
		local tString
		put tArray[tLeft]["char"] & tArray[tRight]["char"] into tString
		
		--Ignore GB 3 - we treat CRLF as two graphemes 
		/*
		if tLeft is kCRIndex and tRight is kLFIndex then
			DoGraphemeTest tString, tLDesc, tRDesc, false
			next repeat
		end if
		*/
		if tLeft is kCRIndex or tLeft is kLFIndex or tLeft is kControlIndex then
			DoGraphemeTest tString, tLDesc, tRDesc, true
			next repeat
		end if
		
		if tRight is kCRIndex or tRight is kLFIndex or tRight is kControlIndex then
			DoGraphemeTest tString, tLDesc, tRDesc, true
			next repeat
		end if
		
		if isHangul(tLeft) and isHangul(tRight) then
			switch tLeft
			case kHangulLIndex
				if tRight is not kHangulTIndex then
					DoGraphemeTest tString, tLDesc, tRDesc, false
					next repeat
				end if
				break
			case kHangulLVIndex
			case kHangulVIndex
				if tRight is kHangulVIndex or tRight is kHangulTIndex then
					DoGraphemeTest tString, tLDesc, tRDesc, false
					next repeat
				end if
				break
			case kHangulLVTIndex
			case kHangulTIndex
				if tRight is kHangulTIndex then
					DoGraphemeTest tString, tLDesc, tRDesc, false
					next repeat
				end if
				break
			end switch
		end if
		
		if tLeft is kRegionalIndex and tRight is kRegionalIndex then
			DoGraphemeTest tString, tLDesc, tRDesc, false
			next repeat
		end if
		
		if tRight is kExtendIndex then
			DoGraphemeTest tString, tLDesc, tRDesc, false
			next repeat
		end if
		
		if tRight is kSpacingIndex then
			DoGraphemeTest tString, tLDesc, tRDesc, false
			next repeat
		end if
		
		if tLeft is kPrependIndex then
			DoGraphemeTest tString, tLDesc, tRDesc, false
			next repeat
		end if
		
		DoGraphemeTest tString, tLDesc, tRDesc, true		
	end repeat
	
end TestGraphemeBoundaries