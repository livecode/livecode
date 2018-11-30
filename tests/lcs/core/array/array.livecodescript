script "CoreArray"
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

constant kNumericKeys = "1,42,51,72,89"
constant kSequenceKeys = "1,2,3,4,5"

constant kStringArray = "string"
constant kNumericArray = "numeric"
constant kSequenceArray = "sequence"

local sSequenceTemplate
local sStringTemplate
local sNumericTemplate

private function __stringTemplate
   if sStringTemplate is empty then
      local tValues, tKeys
      
      put testArrayData("stringValues") into tValues
      put testArrayData("stringKeys") into tKeys
      
      repeat with x = 1 to 5
         put item x of tValues into sStringTemplate[item x of tKeys]
      end repeat
   end if
   
   return sStringTemplate
end __stringTemplate

private function __sequenceTemplate
   if sSequenceTemplate is empty then
      local tValues
      
      put testArrayData("stringValues") into tValues
      
      repeat with x = 1 to 5
         put item x of tValues into sSequenceTemplate[item x of kSequenceKeys]
      end repeat
   end if
   
   return sSequenceTemplate
end __sequenceTemplate

private function __numericTemplate
   if sNumericTemplate is empty then
      local tValues
      
      put testArrayData("stringValues") into tValues
      
      repeat with x = 1 to 5
         put item x of tValues into sNumericTemplate[item x of kNumericKeys]
      end repeat
   end if
   
   return sNumericTemplate
end __numericTemplate

private function __keysAreEqual pLeftKeys, pRightKeys
   sort pLeftKeys
   sort pRightKeys
   
   return pLeftKeys is pRightKeys
end __keysAreEqual

private function __keysForArray pArrayType
   if pArrayType is kStringArray then
      return testArrayData("stringKeys")
   else if pArrayType is kNUmericArray then
      return kNumericKeys
   else
      return kSequenceKeys
   end if
end __keysForArray

on TestExtents
   local  tSequenceKeys, tStringKeys, tNumericKeys
   put __keysForArray(kStringArray) into tStringKeys
   put __keysForArray(kSequenceArray) into tSequenceKeys
   put __keysForArray(kNumericArray) into tNumericKeys
   
   TestAssert "Extents of non-array", the extents of empty is empty
   TestAssert "Extents of string array",  the extents of __stringTemplate() is empty
   TestAssert "Extents of sequence array", the extents of __sequenceTemplate() is (item 1 of tSequenceKeys, item -1 of tSequenceKeys)
   TestAssert "Extents of numeric array", the extents of __numericTemplate() is (item 1 of tNumericKeys, item -1 of tNumericKeys)
   
   // Get the extents with dimensions
   local tDimensionalArray
   repeat with x = 1 to 5
      local tKey
      put item x of tSequenceKeys, item x of tNumericKeys into tKey
      put "value" into tDimensionalArray[tKey]
   end repeat
   
   local tExpectedExtents
   put item 1 of tSequenceKeys, item -1 of tSequenceKeys & LF & \
         item 1 of tNumericKeys, item -1 of tNumericKeys into tExpectedExtents
   
   TestAssert "Extents of dimensional arrays", the extents of tDimensionalArray is tExpectedExtents
end TestExtents

on TestExtentsDecimal
   local tArray
   repeat with i=1 to 5
      put "true" into tArray[i]
   end repeat
   put "false" into tArray[1.0]
   
   TestAssertBroken "Extents of array with a decimal key", the extents of tArray is empty, "bug 21476"
   
   delete variable tArray[1.0]
   local tSequenceKeys
   put __keysForArray(kSequenceArray) into tSequenceKeys
   TestAssert "Extents of array without a decimal key", the extents of tArray is (item 1 of tSequenceKeys, item -1 of tSequenceKeys)
   
   put "false" into tArray[pi]
   TestAssertBroken "Extents of array with another decimal key", the extents of tArray is empty, "bug 21476"
   
end TestExtentsDecimal

on __testIsAmong pUseIsAmong
   local tArray, tKeyList, tKeys
   
   put __keysForArray(kStringArray) into tKeys
   
   repeat for each item tKey in tKeys
      put tKey & LF after tKeyList
      put "value" into tArray[tKey]
   end repeat
   delete the last character of tKeyList
   
   local tMethodUsed
   if pUseIsAmong then
      put "is among" into tMethodused
   else
      put "is not among" into tMethodUsed
   end if
   
   local tSuccess
   put true into tSuccess
   repeat for each line tKey in tKeyList
      local tIsNotAmong
      if pUseIsAmong then
         put not (tKey is among the keys tArray) into tIsNotAmong
      else
         put tKey is not among the keys of tArray into tIsNotAmong
      end if
      
      if tIsNotAmong then
         put false into tSuccess
         exit repeat
      end if
   end repeat
   
   TestAssert "Every key is among the keys - using" & tMethodUsed, tSuccess
   
   // Test with a key that does not belong in the array keys
   local tInvalidKey
   put "not a valid key name" into tInvalidKey
   if pUseIsAmong then
      put not(tInvalidKey is among the keys of tArray) into tSuccess
   else
      put tInvalidKey is not among the keys of tArray into tSuccess
   end if
   
   TestAssert "Non-existing key is not among the keys - " & tMethodUsed, tSuccess
end __testIsAmong

on TestIsAmongTheKeys
   __testIsAmong true
end TestIsAmongTheKeys

on TestIsNotAmongTheKeys
   __testIsAmong false
end TestIsNotAmongTheKeys

on TestKeys
   local tKeyList, tArray
   repeat for each item tKey in __keysForArray(kStringArray)
      put "value" into tArray[tKey]
      put tKey & LF after tKeyList
   end repeat
   delete last char of tKeyList
   
   local tArrayKeys
   put the keys of tArray into tArrayKeys
   
   sort tArrayKeys
   sort tKeyList
   
   TestAssert "the keys return all the keys", tArrayKeys is tKeyList
end TestKeys

on TestMatrixMultiply
   local tArray1, tArray2, tArrayResult
   
   put 1 into tArray1[1,1]
   put 2 into tArray1[1,2]
   put 3 into tArray1[2,1]
   put 4 into tArray1[2,2]
   
   put 5 into tArray2[1,1]
   put 6 into tArray2[1,2]
   put 7 into tArray2[2,1]
   put 8 into tArray2[2,2]
   
   put 19 into tArrayResult[1,1]
   put 22 into tArrayResult[1,2]
   put 43 into tArrayResult[2,1]
   put 50 into tArrayResult[2,2]
   
   // Check that nothing changed in the params, and that the result is right
   local tArray1Copy, tArray2Copy, tMultiplied
   put tArray1 into tArray1Copy
   put tArray2 into tArray2Copy
   put matrixMultiply(tArray1,tArray2) into tMultiplied
   
   TestAssert "Matrix Multiply", tMultiplied is tArrayResult and tArray1 is tArray1Copy and tArray2 is tArray2Copy
end TestMatrixMultiply


on TestMatrixTranspose
   local tArray1, tArray2
   
   put 1 into tArray1[1,1]
   put 2 into tArray1[1,2]
   put 3 into tArray1[2,1]
   put 4 into tArray1[2,2]
   
   put 1 into tArray2[1,1]
   put 2 into tArray2[2,1]
   put 3 into tArray2[1,2]
   put 4 into tArray2[2,2]
   
   // Check that nothing changed in the param, and that the result is right
   local tArray1Copy, tTransposed
   put tArray1 into tArray1Copy
   put transpose(tArray1) into tTransposed
   
   TestAssert "transpose", tTransposed is tArray2 and tArray1 is tArray1Copy
end TestMatrixTranspose

private command __encodeDecodeArray pArray
   local  tEncodedArray, tDecodedArray
   put arrayEncode(pArray) into tEncodedArray
   put arrayDecode(tEncodedArray) into tDecodedArray
   
   TestAssert merge("[[pArrayType]] encoding / decoding"), tDecodedArray is pArray
end __encodeDecodeArray

private command __encodeDecodeArrayType pArrayType
   local tArray, tKeys, tValues
   
   put __keysForArray(pArrayType) into tKeys
   put testArrayData("stringValues") into tValues
   
   // Fill up the array
   repeat with x = 1 to the number of items in tKeys
      put item x of tValues into tArray[item x of tKeys]
   end repeat
   
   __encodeDecodeArray tArray
end __encodeDecodeArrayType


on TestEncodeDecode
   __encodeDecodeArrayType kStringArray
   __encodeDecodeArrayType kNumericArray
   __encodeDecodeArrayType kSequenceArray
   
   local tEmptyArray
   put 1 into tEmptyArray[1]
   delete local tEmptyArray[1]
   
   __encodeDecodeArray tEmptyArray
end TestEncodeDecode
