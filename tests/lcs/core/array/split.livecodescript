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

on TestSplit
   local tResult, tExpected, tRArrow, tLArrow
   put numToCodePoint(0x2192) into tRArrow
   put numToCodePoint(0x2190) into tLArrow

   -- Split by single char
   put "a,b" into tResult
   put empty into tExpected
   put "a" into tExpected[1]
   put "b" into tExpected[2]
   split tResult by ","
   TestAssert "split (native, native)", tResult is tExpected

   put "a," & tRArrow into tResult
   put empty into tExpected
   put "a" into tExpected[1]
   put tRArrow into tExpected[2]
   split tResult by ","
   TestAssert "split (unicode, native)", tResult is tExpected

   put "a" & tRArrow & "b" into tResult
   put empty into tExpected
   put "a" into tExpected[1]
   put "b" into tExpected[2]
   split tResult by tRArrow
   TestAssert "split (unicode, unicode)", tResult is tExpected

   ----------

   put "a:,b" into tResult
   split tResult by ":,"
   TestAssert "split multi (native, native)", tResult is tExpected

   put "a" & tRArrow & tLArrow & "b" into tResult
   split tResult by (tRArrow & tLArrow)
   TestAssert "split multi (unicode, unicode)", tResult is tExpected

   ----------

   put "a:b,c:d" into tResult
   put empty into tExpected
   put "b" into tExpected["a"]
   put "d" into tExpected["c"]
   split tResult by "," and ":"
   TestAssert "split key (native, native)", (tResult is tExpected)

   put "a:b" & tRArrow & "c:d" into tResult
   split tResult by tRArrow and ":"
   TestAssert "split key (unicode, unicode, native)", (tResult is tExpected)

   put "a" & tRArrow & "b,c" & tRArrow & "d" into tResult
   split tResult by "," and tRArrow
   TestAssert "split key (unicode, native, unicode)", (tResult is tExpected)

   ----------

   put "a,b" into tResult
   put empty into tExpected
   put empty into tExpected["a"]
   put empty into tExpected["b"]
   split tResult by "," and ":"
   TestAssert "split key missing (native, native, native)", tResult is tExpected

   put tRArrow & comma & tLArrow into tResult
   put empty into tExpected
   put empty into tExpected[tRArrow]
   put empty into tExpected[tLArrow]
   split tResult by "," and ":"
   TestAssert "split key missing (unicode, native, native)", tResult is tExpected
end TestSplit



on TestSplitByEmpty
   local tResult, tExpected, tRArrow
   put numToCodePoint(0x2192) into tRArrow

   put "a,b" into tResult
   put empty into tExpected
   put tResult into tExpected[1]
   split tResult by empty
   TestAssert "split (native, empty)", tResult is tExpected

   put "a" & tRArrow & "b" into tResult
   put empty into tExpected
   put tResult into tExpected[1]
   split tResult by empty
   TestAssert "split (unicode, empty)", tResult is tExpected

   ----------

   put "a:b,c:d" into tResult
   put empty into tExpected
   put "b,c:d" into tExpected["a"]
   split tResult by empty and ":"
   TestAssert "split key (native, empty, native)", tResult is tExpected

   put "a:b,c:" & tRArrow into tResult
   put empty into tExpected
   put char 3 to -1 of tResult into tExpected["a"]
   split tResult by empty and ":"
   TestAssert "split key (unicode, empty, native)", tResult is tExpected

   ----------

   put "a:b,c:d" into tResult
   put empty into tExpected
   put empty into tExpected["a:b"]
   put empty into tExpected["c:d"]
   split tResult by "," and empty
   TestAssert "split key (native, native, empty)", tResult is tExpected

   put tRArrow & ":b,c:" & tRArrow into tResult
   put empty into tExpected
   put empty into tExpected[tRArrow & ":b"]
   put empty into tExpected["c:" & tRArrow]
   split tResult by "," and empty
   TestAssert "split key (unicode, native, empty)", tResult is tExpected

   ----------

   put "a:b,c:d" into tResult
   put empty into tExpected
   put empty into tExpected[tResult]
   split tResult by empty and empty
   TestAssert "split key (native, empty, empty)", tResult is tExpected

   put "a:b,c:" & tRArrow into tResult
   put empty into tExpected
   put empty into tExpected[tResult]
   split tResult by empty and empty
   TestAssert "split key (unicode, empty, empty)", tResult is tExpected

end TestSplitByEmpty
