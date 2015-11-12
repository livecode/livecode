script "CoreArraySplit"
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

-- Attempting to split a string with an empty delimiter should throw an error
private function IsSplitByEmptyError pString
   local tVar
   put pString into tVar
   try
      split tVar by ""
      return false
   catch tError
      return true
   end try
end IsSplitByEmptyError

on TestSplitByEmpty
   -- Bug 16407
   TestAssert "split native by empty", IsSplitByEmptyError("abc")
   TestAssert "split unicode by empty", IsSplitByEmptyError("abc" & numToCodePoint(0x2192))
end TestSplitByEmpty
