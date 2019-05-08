script "CoreLogicIsStrictly"
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

private command __TestIsStrictly pValue, pExpected, pExpectResult
   local tType, tExpression, tDescription
   repeat for each item tType in \
      "nothing,a boolean,an integer,a real,a string,a binary string,an array"

      if tType is pExpected then
         put merge("[[pExpected]] is strictly [[tType]]") into tDescription
         put merge("pValue is strictly [[tType]]") into tExpression
      else
         put merge("[[pExpected]] is not strictly [[tType]]") into tDescription
         put merge("pValue is not strictly [[tType]]") into tExpression
      end if

      TestAssert tDescription, value(tExpression)
   end repeat
end __TestIsStrictly

function __GenerateNothing
   local tNothing
   put param(1) into tNothing
   return tNothing
end __GenerateNothing

on TestStrictlyNothing
  -- FIXME can't currently generate nothing values in LCS
  -- __TestIsStrictly __GenerateNothing(), "nothing"
end TestStrictlyNothing

on TestStrictlyBoolean
   __TestIsStrictly true, "a boolean"
   __TestIsStrictly false, "a boolean"
   __TestIsStrictly true and true, "a boolean"
   __TestIsStrictly false and true, "a boolean"
end TestStrictlyBoolean

on TestStrictlyInteger
   __TestIsStrictly the number of items in "_", "an integer"
   __TestIsStrictly zero, "an integer"
   __TestIsStrictly one, "an integer"
   __TestIsStrictly two, "an integer"
   __TestIsStrictly three, "an integer"
   __TestIsStrictly four, "an integer"
   __TestIsStrictly five, "an integer"
   __TestIsStrictly six, "an integer"
   __TestIsStrictly seven, "an integer"
   __TestIsStrictly eight, "an integer"
   __TestIsStrictly nine, "an integer"
   __TestIsStrictly ten, "an integer"
end TestStrictlyInteger

on TestStrictlyReal
   __TestIsStrictly infinity, "a real"
   __TestIsStrictly pi, "a real"
end TestStrictlyReal

on TestStrictlyString
   __TestIsStrictly "nothing", "a string"
   __TestIsStrictly 1, "a string"
   __TestIsStrictly 1.0, "a string"
   __TestIsStrictly null, "a string"
   __TestIsStrictly empty, "a string"
end TestStrictlyString

on TestStrictlyBinaryString
   __TestIsStrictly textEncode("nothing","utf-8"), "a binary string"
end TestStrictlyBinaryString

on TestStrictlyArray
   local tVal
   put 1 into tVal[1]

   __TestIsStrictly tVal, "an array"
end TestStrictlyArray
