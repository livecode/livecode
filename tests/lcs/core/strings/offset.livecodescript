script "CoreStringOffset"
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

on TestOffsetSingleUnicodeChar
   local tNeedle, tHaystack
   -- Native needle
   put "B" into tNeedle
   -- Unicode pattern
   put "aAbBcCdDeEfFgG" & numToCodepoint(0x3B1) into tHaystack

   set the caseSensitive to true
   TestAssert "offset of native needle in unicode string - case-sensitive", offset(tNeedle, tHaystack) is 4

   set the caseSensitive to false
   TestAssert "offset of native needle in unicode string - caseless", offset(tNeedle, tHaystack) is 3
end TestOffsetSingleUnicodeChar

// Function provided by Bernd Niggemann as part of bug report 
// #21704
function allOffsets pDelim, pString, pCaseSensitive, pNoOverlap
   local tNewPos, tPos, tResult, tLen
   
   set the caseSensitive to pCaseSensitive is true
   
   put 0 into tPos
   if pNoOverlap then
      put the len of pDelim - 1 into tLen
      repeat forever
         put offset(pDelim, pString, tPos) into tNewPos
         if tNewPos = 0 then exit repeat
         add tNewPos to tPos
         put tPos,"" after tResult
         add tLen to tPos
      end repeat
   else
      repeat forever
         put offset(pDelim, pString, tPos) into tNewPos
         if tNewPos = 0 then exit repeat
         add tNewPos to tPos
         put tPos,"" after tResult
      end repeat
   end if
   
   if tResult is empty then return 0
   else return char 1 to -2 of tResult
end allOffsets

// Test whether we can correctly detect offsets in strings that contain
// native unicode chars, in a case sensitive manner
on TestOffsetMultipleUnicodeCharsSensitiveText1
	local tNeedle, tHaystack

	put "aaaaaaa" into tNeedle
	put "𠜎aa𠜎aaa ばいしんせい ばいしんせい aaaaaaaaaaaaaaaaaaa c aaaaaaaaaaaaaaaaaaa" into tHaystack

	local tExpectedOffsets
	put "23,24,25,26,27,28,29,30,31,32,33,34,35,45,46,47,48,49,50,51,52,53,54,55,56,57" into tExpectedOffsets

	TestAssert "the offsets match the expected offsets", tExpectedOffsets is allOffsets(tNeedle, tHaystack, true, false)
end TestOffsetMultipleUnicodeCharsSensitiveText1

// Test whether we can correctly detect offsets in strings that contain
// native unicode chars, in a case in-sensitive manner, in a different text
on TestOffsetMultipleUnicodeCharsInSensitiveText2
	local tNeedle, tHaystack

	put "aaa" into tNeedle
	put "aa 𠜎 aaaaaaaaaaaaa↘𠜎aaaa" into tHaystack

	local tExpectedOffsets
	put "6,7,8,9,10,11,12,13,14,15,16,21,22" into tExpectedOffsets

	TestAssert "the offsets match the expected offsets", tExpectedOffsets is allOffsets(tNeedle, tHaystack, true, false)
end TestOffsetMultipleUnicodeCharsInSensitiveText2

// Test whether we can correctly detect offsets in strings that contain
// native unicode chars, in a case in-sensitive manner
on TestOffsetMultipleUnicodeCharsInSensitive2
	local tNeedle, tHaystack

	put "aaa" into tNeedle
	put "aa 𠜎 aaaaaaaaAAAaa↘𠜎aaaa" into tHaystack

	local tExpectedOffsets
	put "6,7,8,9,10,11,12,13,14,15,16,21,22" into tExpectedOffsets

	TestAssert "the offsets match the expected offsets", tExpectedOffsets is allOffsets(tNeedle, tHaystack, false, false)
end TestOffsetMultipleUnicodeCharsInSensitive2

// Test whether we can correctly detect offsets in strings that contain
// native unicode chars, in a case sensitive manner
on TestOffsetMultipleUnicodeCharsInSensitive3
	local tNeedle, tHaystack

	put "𐀁" into tNeedle
	put "𐀁𐀁𐀁𐀁𐀁𐀁" into tHaystack

	local tExpectedOffsets
	put "1,2,3,4,5,6" into tExpectedOffsets

	TestAssert "the offsets match the expected offsets", tExpectedOffsets is allOffsets(tNeedle, tHaystack, true, false)
end TestOffsetMultipleUnicodeCharsInSensitive3
