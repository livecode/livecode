script "CoreArrayOperation"
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

// Keeping the file, for non-yet-implemented tests for arrays
