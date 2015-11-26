script "CoreArrayIntersect"
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

// pArrayType: one of kStringArray, kNumericArray, kSequenceArray
private command  __testIntersect pArrayType
   local tStringArray, tNumberArray, tCommonArray
   local tValues, tKeys, tTemplateArray
   
   if pArrayType is kStringArray then
      put testArrayData("stringKeys") into tKeys
      put __stringTemplate() into tTemplateArray
   else if pArrayType is kNumericArray then
      put kNumericKeys into tKeys
      put __numericTemplate() into tTemplateArray
   else
      put kSequenceKeys into tKeys
      put __sequenceTemplate() into tTemplateArray
   end if
   
   put testArrayData("stringValues") into tValues
   
   // Only the first item will match the template
   put item 1 of tValues into tCommonArray[item 1 of tKeys]
   put item 2 of tValues into tCommonArray[item 2 of tKeys]
   
   // Now add a key that will not intersect with the template array
   // and a matching key - even if the value is not the same as it common array)
   // ( we have non-matching keys in both String and Template array)
   put tCommonArray into tStringArray
   put item 3 of tValues into tStringArray[item 2 of tKeys]
   put item 3 of tValues into tStringArray[item 3 of tKeys & "other"]
   
   intersect tStringArray with tTemplateArray
   
   TestAssert merge("Intersect [[pArrayType]] arrays"), the keys of tStringArray is the keys of tCommonArray
end __testIntersect

on TestIntersectString
   __testIntersect kStringArray
end TestIntersectString

on  TestIntersectSequence
   __testIntersect kSequenceArray
end TestIntersectSequence

on TestIntersectNumeric
   __testIntersect kNumericArray
end TestIntersectNumeric

// pArrayType: one of kStringArray, kNumericArray, kSequenceArray
private command __testIntersectRecursive pArrayType
   local tLeftArray, tRightArray, tCommonArray
   local tKeys
   
   if pArrayType is kStringArray then
      put testArrayData("stringKeys") into tKeys
   else if pArrayType is kNumericArray then
      put kNumericKeys into tKeys
   else
      put kSequenceKeys into tKeys
   end if
   
   // Set the common array
   put __numericTemplate() into tCommonArray[item 1 of tKeys]
   put __sequenceTemplate() into tCommonArray[item 2 of tKeys]
   put __stringTemplate() into tCommonArray[item 3 of tKeys]
   
   put tCommonArray into tLeftArray
   put tCommonArray into tRightArray
   
   // Create sub-array that misses one key
   local tCrippledStringArray
   put __stringTemplate() into tCrippledStringArray
   delete local tCrippledStringArray[line 1 of the keys of tCrippledStringArray]
   
   put tCrippledStringArray into tRightArray[item 4 of tKeys]
   put tCrippledStringArray into tCommonArray[item 4 of tKeys]
   put __stringTemplate() into tLeftArray[item 4 of tKeys]
   
   intersect tLeftArray with tRightArray recursively
   
   local tSuccess
   put the keys of tLeftArray is the keys of tCommonArray into tSuccess
   
   if tSuccess then
      repeat for each key tKey in tLeftArray
         if the keys of tLeftArray[tKey] is not the keys of tCommonArray[tKey] then
            put false into tSuccess
            exit repeat
         end if
      end repeat
   end if
   
   testAssert merge("Recursice intersect for [[pArrayType]] array"), tSuccess
end __testIntersectRecursive

// Test with string keys
on TestIntersectStringRecursive
   __testIntersectRecursive kStringArray
end TestIntersectStringRecursive


// Test with numeric keys
on TestIntersectSequenceRecursive
   __testIntersectRecursive kSequenceArray
end TestIntersectSequenceRecursive

on TestIntersectNumberRecursive
   __testIntersectRecursive kNumericArray
end TestIntersectNumberRecursive
