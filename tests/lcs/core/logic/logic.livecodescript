script "CoreLogic"
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



constant kIsA_types = "array,boolean,color,date,integer,number,point,rect,ascii string"
local sValueToType


private command __compare pLeft, pOperator, pRight, pShouldSucceed
   local tSuccess
   put value(merge("pLeft [[pOperator]] pRight")) into tSuccess
   if not pShouldSucceed then
      put not tSuccess into tSuccess
   end if
   
   TestAssert merge("'[[pLeft]]' [[pOperator]] '[[pRight]]'"), tSuccess
end __compare



// TestAssert message cannot contain '#'
private function _fixSharp pMess
   replace "#" with "[sharp]" in pMess
   return pMess
end _fixSharp

private function IsMacDesktop
   if the platform is not "MacOS" then
      return false
   end if
   
   if the environment is "server" then
      return false
   end if
   
   return true
end IsMacDesktop

private command __checkVariableType pVariable, pType
   repeat for each item tType in kIsA_types
      if tType is among the items of pType then
         TestAssert _fixSharp(merge("'Variable containing '[[pVariable]]' is a [[tType]]")), value("pVariable is a " & tType)
      else
         TestAssert _fixSharp(merge("'Variable containing '[[pVariable]]' is not a [[tType]]")), value("pVariable is not a " & tType)
      end if
   end repeat
end __checkVariableType

on TestIsA
   // Array
   local tArray
   put "value" into tArray["key"]
   __checkVariableType tArray, "array"
   
   // Boolean
   __checkVariableType true, "boolean,ascii string"
   __checkVariableType TrUe, "boolean,ascii string"
   __checkVariableType "TRUE", "boolean,ascii string"
   __checkVariableType "TruE", "boolean,ascii string"
   __checkVariableType "true", "boolean,ascii string"
   
   __checkVariableType false, "boolean,ascii string"
   __checkVariableType FalSE, "boolean,ascii string"
   __checkVariableType "FALSE", "boolean,ascii string"
   __checkVariableType "FAlSe", "boolean,ascii string"
   __checkVariableType "false", "boolean,ascii string"
   
   // 0 and 1  are not booleans
   __checkVariableType 1, "integer,number,color,date,ascii string"
   __checkVariableType 0, "integer,number,color,date,ascii string"
   
   // colour name
   __checkVariableType "red", "color,ascii string"
   
   // RGB colour
   __checkVariableType "123,123,123", "color,ascii string"
   
   // HTML colour
   __checkVariableType "#efefef", "color,ascii string"
   
   // Date
   __checkVariableType the date, "date,ascii string"
   
   // Positive integer
   __checkVariableType 42, "integer,number,date,color,ascii string"
   
   // Negative integer - is a date on Mac Desktop only.
   if IsMacDesktop() then
      __checkVariableType -42, "integer,number,date,color,ascii string"
   else
      __checkVariableType -42, "integer,number,color,ascii string"
   end if
   
   // Positive real
   __checkVariableType 3.14, "number,date,ascii string"
   
   // Negative real - is a date on Mac Desktop only.
   if IsMacDesktop() then
      __checkVariableType -3.14, "number,date,ascii string"
   else
      __checkVariableType -3.14, "number,ascii string"
   end if
   
   // Positive scientific notation - integer
   __checkVariableType 3.14E2, "number,date,integer,ascii string"
   
   // Positive scientific notation - real
   __checkVariableType 3.141E2, "number,date,ascii string"
   
   // String as integer
   __checkVariableType "   100", "number,integer,color,date,ascii string"
   
   // Hexadecimal numbers
   __checkVariableType "0x100", "number,integer,color,date,ascii string"
   __checkVariableType "0xffw", "ascii string" // Invalid
   
   // 0
   __checkVariableType 0, "integer,number,date,color,ascii string"
   
   // String as number
   __checkVariableType "42", "integer,number,color,date,ascii string"
   
   // Infinity
   local tInfinity
   put "infinity" into tInfinity
   __checkVariableType tInfinity, "number,integer,ascii string"
   
   // Point
   local tPoint
   put 12,23 into tPoint
   __checkVariableType tPoint, "point,ascii string"
   
   // Rect
   local tRect
   put 0,0,12,12 into tRect
   __checkVariableType tRect, "rect,ascii string"
   
   // ASCII string
   __checkVariableType "some text", "ascii string"
   
   // numToChar(128)
   __checkVariableType numToChar(128), ""
   
   // Unicode string
   __checkVariableType "ыщьуерштп", ""
   
   // Empty
   __checkVariableType empty, ""
end TestIsA

on TestAnd
   TestAssert "false and false", (false and false) is false
   TestAssert "false and true", (false and true) is false
   TestAssert "true and false", (true and false) is false 
   TestAssert "true and true", (true and true) is true
   
   repeat for each item tbool1 in "true,false"
      repeat for each item tbool2 in "true,false"
         TestAssert merge("[[tbool1]] and [[tbool2]] is [[tbool2]] and [[tbool2]]"), (tbool1 and tbool2) is (tbool2 and tbool1)
      end repeat
   end repeat
end TestAnd

on TestLogicArrays
   -- Equal case
   local tLeftArray, tRightArray
   put 10 into tLeftArray["1"]
   put "foo" into tLeftArray["bar"]
   put 10 into tRightArray["1"]
   put "foo" into tRightArray["bar"]
   __compare tLeftArray, "is", tRightArray, true
   __compare tLeftArray, "=", tRightArray, true
   
   -- Left bigger than right
   put true into tLeftArray["baz"]
   __compare tLeftArray, "is", tRightArray, false
   __compare tLeftArray, "=", tRightArray, false
   
   -- Right bigger than left
   delete variable tLeftArray["baz"]
   put true into tRightArray["baz"]
   __compare tLeftArray, "is", tRightArray, false
   __compare tLeftArray, "=", tRightArray, false
   
   -- Unequal case
   delete variable tRightArray["baz"]
   put 100 into tRightArray["1"]
   __compare tLeftArray, "is", tRightArray, false
   __compare tLeftArray, "=", tRightArray, false
end TestLogicArrays


on TestLogicNumbers
   __compare 100, "is","  100", true
   __compare 100, "=","  100", true
   __compare 100, "is","  1001", false
   __compare 100, "=","  1001", false
   __compare "abc", "is","abc", true
   __compare "abc", "=","abc", true
   __compare "abc", "is","abcd", false
   __compare "abc", "=","abcd", false
   __compare 100, "is",100, true
   __compare 100, "=",100, true
   __compare 100, "is",1000, false
   __compare 100, "=",1000, false
end TestLogicNumbers

// Builds the appropriate message, according to the operator and emptiness of the variable
private function __getVariableTypeMessage pOperator, pVariable, pIsEmpty
   local tMess
   put merge("variable [[pOperator]] empty") into tMess
   if pVariable is an array then
      put "array " before tMess
   end if
   
   put "empty " before tMess
   if not pIsEmpty then
      put "non-" before tMess
   end if
   
   return tMess
end __getVariableTypeMessage

private command __checkEmptyForOperator pOperator, pVariable, pIsEmpty
      local tResult, tMess
      put value(merge("pVariable [[pOperator]] empty")) into tResult
      
      put __getVariableTypeMessage(pOperator, pVariable, pIsEmpty) into tMess
      
      TestAssert tMess, pIsEmpty is tResult
end __checkEmptyForOperator

// Check the is (not) empty for a variable
private command __checkEmpty pVariable, pIsEmpty
   __checkEmptyForOperator "=", pVariable, pIsEmpty
   __checkEmptyForOperator "is", pVariable, pIsEmpty
   __checkEmptyForOperator "<>", pVariable, not pIsEmpty
   __checkEmptyForOperator "is not", pVariable, not pIsEmpty
end __checkEmpty

on TestLogicEmpty
   local tLeft
   
   // Check an empty variable
   __checkEmpty tLeft, true
   
   // Check array
   put 100 into tLeft[1]
   __checkEmpty tLeft, false
   
   // Check array with removed key
   delete local tLeft[1]
   __checkEmpty tLeft, true
   
   // Check string
   put "string" into tLeft
   __checkEmpty tLeft, false
   
   // Check empty
   __checkEmpty empty, true
end TestLogicEmpty

/* 
Run comparison tests with the provided comparison operator
pExpectedResult stores a comma-separated list of results for
the following comparisons (11):
10 <op> 1
10 <op> 10
1  <op> 10
"car" <op> "cat"
"cat" <op> "cat"
"car" <op> "cat"
"  0x100" <op> 1
"   0x100" <op> "   0x100"
"1" <op> "   0x100"
leftArray <op> rightArray  *
rightArray <op> leftArray
true <op> true
true <op> false
false <op> true
false <op> false

* where leftArray and rightArray are both:
array: [1] = 100
*/
private command __runComparisons pOperator, pResultList
   __compare 10, pOperator, 1, item 1 of pResultList
   __compare 10, pOperator, 10, item 2 of pResultList
   __compare 1, pOperator, 10, item 3 of pResultList
   
   __compare "cat", pOperator, "car", item 4 of pResultList
   __compare "cat", pOperator, "cat", item 5 of pResultList
   __compare "car", pOperator, "cat", item 6 of pResultList
   
   __compare "   0x100", pOperator, "   1", item 7 of pResultList
   __compare "   0x100",  pOperator, "   0x100", item 8 of pResultList
   __compare "1", pOperator, "   0x100", item 9 of pResultList
   
   local tLeftArray, tRightArray
   put 100 into tLeftArray[1]
   put 100 into tRightArray[1]
   __compare tLeftArray, pOperator, tRightArray, item 10 of pResultList
   __compare tRightArray, pOperator, tLeftArray, item 11 of pResultList
   
   __compare true, pOperator, true, item 12 of pResultList
   __compare true, pOperator, false, item 13 of pResultList
   __compare false, pOperator, true, item 14 of pResultList
   __compare false, pOperator, false, item 15 of pResultList
end __runComparisons


on TestGreaterThan
   __runComparisons ">", "true,false,false,true,false,false,true,false,false,false,false,false,true,false,false"
end TestGreaterThan


on TestGreaterThanOrEqual
   __runComparisons ">=", "true,true,false,true,true,false,true,true,false,true,true,true,true,false,true"
end TestGreaterThanOrEqual


on TestLessThan
   __runComparisons "<", "false,false,true,false,false,true,false,false,true,false,false,false,false,true,false"
end TestLessThan


on TestLessThanOrEqual
   __runComparisons "<=", "false,true,true,fa;se,true,true,false,true,true,true,true,true,false,true,true"
end TestLessThanOrEqual

on TestEqual
   local tExpectedResults
   put "false,true,false,false,true,false,false,true,false,true,true,true,false,false,true" into tExpectedResults
   
   __runComparisons "=", tExpectedResults
   __runComparisons "is", tExpectedResults
end TestEqual

on TestNotEqual
   local tExpectedResults
   put "true,false,true,true,false,true,true,false,true,false,false,false,true,true,false" into tExpectedResults
   
   __runComparisons "<>", tExpectedResults
   __runComparisons "is not", tExpectedResults
end TestNotEqual


on TestNot
   TestAssert "not false is true", (not false) is true
   TestAssert "not true is false", (not true) is false
   
   repeat for each item tbool in "true,false"
      TestAssert merge("not not [[tbool]] is [[tbool]]"), (not (not tbool) ) is tbool
   end repeat
   
   TestAssert "not <string> is true", not "string" is true
end TestNot


on TestOr
   TestAssert "false or false", (false or false) is false
   TestAssert "false or true", (false or true) is true
   TestAssert "true or false", (true or false) is true
   TestAssert "true or true", (true or true) is true
   
   repeat for each item tbool1 in "true,false"
      repeat for each item tbool2 in "true,false"
         TestAssert merge("[[tbool1]] or [[tbool2]] is [[tbool2]] or [[tbool1]]"), (tbool1 or tbool2) is (tbool2 or tbool1)
      end repeat
   end repeat
end TestOr
