script "CoreStringsByteoffset"
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

// Function provided by Bernd Niggemann as part of bug report 
// #21704
// We have changed the implementation to use byteOffset rather than offset
function allOffsets pDelim, pString, pCaseSensitive, pNoOverlap
   local tNewPos, tPos, tResult, tLen
   
   set the caseSensitive to pCaseSensitive is true
   
   put 0 into tPos
   if pNoOverlap then
      put the len of pDelim - 1 into tLen
      repeat forever
         put byteoffset(pDelim, pString, tPos) into tNewPos
         if tNewPos = 0 then exit repeat
         add tNewPos to tPos
         put tPos,"" after tResult
         add tLen to tPos
      end repeat
   else
      repeat forever
         put byteoffset(pDelim, pString, tPos) into tNewPos
         if tNewPos = 0 then exit repeat
         add tNewPos to tPos
         put tPos,"" after tResult
      end repeat
   end if
   
   if tResult is empty then return 0
   else return char 1 to -2 of tResult
end allOffsets


on TestByteOffsetNonEmptyNeedleEmptyHaystack
   local tNeedle, tHaystack
   put "DEF" into tNeedle
   put empty into tHaystack

   TestAssert "byteoffset of non-empty needle in empty haystack", byteoffset(tNeedle, tHaystack) is 0

end TestByteOffsetNonEmptyNeedleEmptyHaystack


on TestByteOffsetNonEmptyNeedleNonEmptyHaystack
   local tNeedle, tHaystack
   put "DEF" into tNeedle
   put "ABCDEF" into tHaystack

   TestAssert "byteoffset of non-empty needle in non-empty haystack", byteoffset(tNeedle, tHaystack) is 4

end TestByteOffsetNonEmptyNeedleNonEmptyHaystack


on TestByteOffsetEmptyNeedleEmptyHaystack
   local tNeedle, tHaystack
   put empty into tNeedle
   put empty into tHaystack

   TestAssert "byteoffset of empty needle in empty haystack", byteoffset(tNeedle, tHaystack) is 0

end TestByteOffsetEmptyNeedleEmptyHaystack


on TestByteOffsetEmptyNeedleNonEmptyHaystack
   local tNeedle, tHaystack
   put empty into tNeedle
   put "ABCDEF" into tHaystack

   TestAssert "byteoffset of empty needle in non-empty haystack", byteoffset(tNeedle, tHaystack) is 0

end TestByteOffsetEmptyNeedleNonEmptyHaystack

on TestByteOffsetBinaryEncodingShouldNotGiveAnswer
   local tNeedle, tHaystack
   put "Rejkyavik" into tNeedle
   put "R" into tHaystack

   TestAssert "byteoffset of needler longer than haystack should return 0", byteoffset(tNeedle, tHaystack) is 0
end TestByteOffsetBinaryEncodingShouldNotGiveAnswer

on TestByteOffsetBinaryEncodingShouldGiveAnswer
   local tNeedle, tHaystack
   put "j" into tNeedle
   put "Rejkyavik" into tHaystack

   TestAssert "byteoffset of non-empty needle in non-empty haystack", byteoffset(tNeedle, tHaystack) is 3
end TestByteOffsetBinaryEncodingShouldGiveAnswer

on TestByteOffsetBinaryEncodingShouldNotGiveAnswerEncoded
   local tNeedle, tHaystack
   put textEncode("Rejkyavik", "UTF-32") into tNeedle
   put textEncode("R", "UTF-32") into tHaystack

   TestAssert "byteoffset of needler longer than haystack should return 0", byteoffset(tNeedle, tHaystack) is 0
end TestByteOffsetBinaryEncodingShouldNotGiveAnswerEncoded

on TestByteOffsetBinaryEncodingShouldGiveAnswerEncoded
   local tNeedle, tHaystack
   put textEncode("j", "UTF-32") into tNeedle
   put textEncode("Rejkyavik", "UTF-32") into tHaystack

   TestAssert "byteoffset of non-empty needle in non-empty haystack", byteoffset(tNeedle, tHaystack) is 9
end TestByteOffsetBinaryEncodingShouldGiveAnswerEncoded


on TestByteOffsetBinaryEncodingShouldGiveAnswerEncodedChineseChars
   local tNeedle, tHaystack
   put textEncode("aaaaaaa", "UTF-32") into tNeedle
   put textEncode("𠜎aa𠜎aaa ばいしんせい ばいしんせい aaaaaaaaaaaaaaaaaaa c aaaaaaaaaaaaaaaaaaa", "UTF-32") into tHaystack

   local tExpectedResult
   put "89,93,97,101,105,109,113,117,121,125,129,133,137,177,181,185,189,193,197,201,205,209,213,217,221,225" into tExpectedResult

   TestAssert "byteoffset of non-empty needle in non-empty haystack with chinese characters",  allOffsets(tNeedle, tHaystack, true, false) is tExpectedResult
end TestByteOffsetBinaryEncodingShouldGiveAnswerEncodedChineseChars

// Test whether we can correctly detect offsets in strings that contain
// native unicode chars, in a case in-sensitive manner, in a different text
on TestOffsetMultipleUnicodeCharsInSensitiveText2
   local tNeedle, tHaystack

   put textEncode("aaa", "UTF-32") into tNeedle
   put textEncode("aa 𠜎 aaaaaaaaaaaaa↘𠜎aaaa", "UTF-32") into tHaystack

   local tExpectedOffsets
   put "21,25,29,33,37,41,45,49,53,57,61,81,85" into tExpectedOffsets

   put allOffsets(tNeedle, tHaystack, true, false)

   TestAssert "the offsets match the expected offsets", tExpectedOffsets is allOffsets(tNeedle, tHaystack, true, false)
end TestOffsetMultipleUnicodeCharsInSensitiveText2

// Test whether we can correctly detect offsets in strings that contain
// native unicode chars, in a case in-sensitive manner
on TestOffsetMultipleUnicodeCharsInSensitive2
   local tNeedle, tHaystack

   put textEncode("aaa", "UTF-32") into tNeedle
   put textEncode("aa 𠜎 aaaaaaaaAAAaa↘𠜎aaaa", "UTF-32") into tHaystack

   local tExpectedOffsets
   put "21,25,29,33,37,41,45,49,53,57,61,81,85" into tExpectedOffsets

   put allOffsets(tNeedle, tHaystack, false, false)

   TestAssert "the offsets match the expected offsets", tExpectedOffsets is not allOffsets(tNeedle, tHaystack, false, false)
end TestOffsetMultipleUnicodeCharsInSensitive2

// Test whether we can correctly detect offsets in strings that contain
// native unicode chars, in a case sensitive manner
on TestOffsetMultipleUnicodeCharsInSensitive3
   local tNeedle, tHaystack

   put textEncode("𐀁", "UTF-32") into tNeedle
   put textEncode("𐀁𐀁𐀁𐀁𐀁𐀁", "UTF-32") into tHaystack

   local tExpectedOffsets
   put "1,3,5,7,9,11,13,15,17,19,21" into tExpectedOffsets

   put allOffsets(tNeedle, tHaystack, false, false)

   TestAssert "the offsets match the expected offsets", tExpectedOffsets is allOffsets(tNeedle, tHaystack, true, false)
end TestOffsetMultipleUnicodeCharsInSensitive3