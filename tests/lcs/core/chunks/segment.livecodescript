script "CoreChunksSegment"
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

on TestSegmentRanges
 TestAssert "overlong prefix", segment -4 to -2 of "three segment sentence" is "three segment"
 TestAssert "overlong suffix", segment 2 to 4 of "three segment sentence" is "segment sentence"
end TestSegmentRanges

on TestSegmentRangesEmpty
  TestAssert "empty prefix", segment -5 to -4 of "three segment sentence" is ""
  TestAssert "empty suffix", segment 4 to 5 of "three segment sentence" is ""
  TestAssert "empty range", segment 5 to 4 of "three segment sentence" is ""
end TestSegmentRangesEmpty

on TestSegmentRangesSingle
  TestAssert "single segment range", segment 1 to 1 of "three segment sentence" is "three"
  TestAssert "single segment range from back", segment -2 to -2 of "three segment sentence" is "segment"
end TestSegmentRangesSingle

on TestSegmentExpressions
  TestAssert "single segment", segment 1 of "three segment sentence" is "three"
  TestAssert "single segment from back", segment -3 of "three segment sentence" is "three"
end TestSegmentExpressions

on TestSegmentExpressionsQuoted
  local tVar
  put quote & "first segment" & quote && "and" && quote & "third segment" & quote into tVar
  TestAssert "single segment quoted", segment 2 of tVar is "and"
  TestAssert "single segment quoted from back", segment -2 of tVar is "and"
end TestSegmentExpressionsQuoted

on TestSegmentExpressionsEmpty
  TestAssert "single segment", segment 4 of "three segment sentence" is ""
  TestAssert "single segment", segment -4 of "three segment sentence" is ""
end TestSegmentExpressionsEmpty

on TestSegmentLineBreakCount
  TestAssert "count with line break", the number of segments in ("one" & return & "two") is 2
  TestAssert "count with line break quoted", the number of segments in (quote & "one" & return & "two" & quote) is 2
end TestSegmentLineBreakCount

on TestSegmentLineBreakExpression
  TestAssert "single segment", segment 2 of ("one" & return & "two") is "two"
  TestAssert "single segment quoted", segment 2 of (quote & "one" & return & "two" & quote) is ("two" & quote)
end TestSegmentLineBreakExpression

on TestSegmentCount
  TestAssert "count", the number of segments in "three segment sentence" is 3
  TestAssert "count", the number of segments in (quote & "one segment sentence" & quote) is 1
end TestSegmentCount

on TestBug16161
  local tLine, tExpected
  put "assert true (comma is" && quote & "," & quote && "AND comma is numToChar(44))" into tLine
  put "(comma is" && quote & "," & quote && "AND comma is numToChar(44))" into tExpected
  TestAssert "comma is not word delimiter", word 3 to -1 of tLine is tExpected
end TestBug16161