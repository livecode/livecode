script "CoreMathArithmeticCommmands"
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

----------

private function MapError pError, pSrcErr, pDstErr
   if item 1 of pError is pSrcErr then
      return "src"
   end if

   if item 1 of pError is pDstErr then
      return "dst"
   end if

   return empty
end MapError

----------

constant kAddSrcError = 10
constant kAddDstError = 9

private function DoAdd pSrc, pDst
   try
      add pSrc to pDst
   catch tError
   end try
   return MapError(tError, kAddSrcError, kAddDstError)
end DoAdd

on TestAdd
   local tSrc, tDst
   put "foo" into tSrc
   put 0 into tDst
   TestAssert "add bad to good is error", DoAdd(tSrc, tDst) is "src"
   TestAssert "add good to bad is error", DoAdd(tDst, tSrc) is "dst"
end TestAdd

----------

constant kSubtractSrcError = 579
constant kSubtractDstError = 578

private function DoSubtract pSrc, pDst
   try
      subtract pSrc from pDst
   catch tError
   end try
   return MapError(tError, kSubtractSrcError, kSubtractDstError)
end DoSubtract

on TestSubtract
   local tSrc, tDst
   put "foo" into tSrc
   put 0 into tDst
   TestAssert "subtract bad from good is error", DoSubtract(tSrc, tDst) is "src"
   TestAssert "subtract good from bad is error", DoSubtract(tDst, tSrc) is "dst"
end TestSubtract

----------

constant kMultiplySrcError = 334
constant kMultiplyDstError = 333

private function DoMultiply pSrc, pDst
   try
      multiply pDst by pSrc
   catch tError
   end try
   return MapError(tError, kMultiplySrcError, kMultiplyDstError)
end DoMultiply

on TestMultiply
   local tSrc, tDst
   put "foo" into tSrc
   put 0 into tDst
   TestAssert "multiply bad by good is error", DoMultiply(tSrc, tDst) is "src"
   TestAssert "multiply good by bad is error", DoMultiply(tDst, tSrc) is "dst"
end TestMultiply

----------

constant kDivideSrcError = 149
constant kDivideDstError = 148

private function DoDivide pSrc, pDst
   try
      divide pDst by pSrc
   catch tError
   end try
   return MapError(tError, kDivideSrcError, kDivideDstError)
end DoDivide

on TestDivide
   local tSrc, tDst
   put "foo" into tSrc
   put 0 into tDst
   TestAssert "divide bad by good is error", DoDivide(tSrc, tDst) is "src"
   TestAssert "divide good by bad is error", DoDivide(tDst, tSrc) is "dst"
end TestDivide

----------
