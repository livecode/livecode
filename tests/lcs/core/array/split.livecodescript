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
   local tResult, tExpected, tError

   put "a,b" into tResult
   put empty into tExpected
   put tResult into tExpected[1]
   try
      split tResult by empty
      TestAssert "split (native, empty)", false
   catch tError
      TestAssert "split (native, empty)", true
   end try

   put "a:b,c:d" into tResult
   put empty into tExpected
   put "b,c:d" into tExpected["a"]
   try
      split tResult by empty and ":"
      TestAssert "split key (native, empty, native)", false
   catch tError
      TestAssert "split key (native, empty, native)", true
   end try

   put "a:b,c:d" into tResult
   put empty into tExpected
   put empty into tExpected["a:b"]
   put empty into tExpected["c:d"]
   try
      split tResult by "," and empty
      TestAssert "split key (native, native, empty)", false
   catch tError
      TestAssert "split key (native, native, empty)", true
   end try

   put "a:b,c:d" into tResult
   put empty into tExpected
   put empty into tExpected[tResult]
   try
      split tResult by empty and empty
      TestAssert "split key (native, empty, empty)", false
   catch tError
      TestAssert "split key (native, empty, empty)", true
   end try

end TestSplitByEmpty



on TestSplitByColumn
   local tResult, tExpected

   put "a,b:c,d" into tResult
   put empty into tExpected
   put "a:c" into tExpected[1]
   put "b:d" into tExpected[2]
   set the columnDel to ","
   set the rowDel to ":"
   split tResult by column

   TestAssert "split column (native, native, native)", tResult is tExpected

   ----------

   put "a,b:c,d" into tResult
   set the columnDel to empty
   set the rowDel to ":"
   try
      split tResult by column
      TestAssert "split column (native, empty, native)", false
   catch tError
      TestAssert "split column (native, empty, native)", true
   end try

   put "a,b:c,d" into tResult
   set the columnDel to ","
   set the rowDel to empty
   try
      split tResult by column
      TestAssert "split column (native, native, empty)", false
   catch tError
      TestAssert "split column (native, native, empty)", true
   end try
end TestSplitByColumn



on TestSplitByRow
   local tResult, tExpected

   put "a,b:c,d" into tResult
   put empty into tExpected
   put "a,b" into tExpected[1]
   put "c,d" into tExpected[2]
   set the columnDel to ","
   set the rowDel to ":"
   split tResult by row
   TestAssert "split row (native, native, native)", tResult is tExpected

   -- The column delimiter isn't actually used when splitting by row.
   put "a,b:c,d" into tResult
   set the columnDel to empty
   set the rowDel to ":"
   split tResult by row
   TestAssert "split row (native, empty, native)", tResult is tExpected

   ----------

   put "a,b:c,d" into tResult
   set the columnDel to ","
   set the rowDel to empty
   try
      split tResult by row
      TestAssert "split row (native, native, empty)", false
   catch tError
      TestAssert "split row (native, native, empty)", true
   end try
end TestSplitByRow



on TestSplitAsSet
   local tResult, tExpected, tError

   put "a:b" into tResult
   put empty into tExpected
   put true into tExpected["a"]
   put true into tExpected["b"]
   split tResult with ":" as set
   TestAssert "split set (native, native)", tResult is tExpected

   ----------

   put "a:b" into tResult
   try
      split tResult with empty as set
      TestAssert "split set (native, empty)", false
   catch tError
      TestAssert "split set (native, empty)", true
   end try
end TestSplitAsSet
