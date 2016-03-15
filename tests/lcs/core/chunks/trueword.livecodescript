script "CoreChunksTrueWord"
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

on TestTrueWordRanges
   TestAssert "overlong prefix", trueWord -4 to -2 of "three trueWord sentence" is "three trueWord"
   TestAssert "overlong suffix", trueWord 2 to 4 of "three trueWord sentence" is "trueWord sentence"
end TestTrueWordRanges

on TestTrueWordRangesEmpty
  TestAssert "empty prefix", trueWord -5 to -4 of "three trueWord sentence" is ""
  TestAssert "empty suffix", trueWord 4 to 5 of "three trueWord sentence" is ""
  TestAssert "empty range", trueWord 5 to 4 of "three trueWord sentence" is ""
end TestTrueWordRangesEmpty

on TestTrueWordRangesSingle
  TestAssert "single trueWord range", trueWord 1 to 1 of "three trueWord sentence" is "three"
  TestAssert "single trueWord range from back", trueWord -2 to -2 of "three trueWord sentence" is "trueWord"
end TestTrueWordRangesSingle

on TestTrueWordExpressions
  TestAssert "single trueWord", trueWord 1 of "three trueWord sentence" is "three"
  TestAssert "single trueWord from back", trueWord -3 of "three trueWord sentence" is "three"
end TestTrueWordExpressions

on TestTrueWordExpressionsEmpty
  TestAssert "single trueWord", trueWord 4 of "three trueWord sentence" is ""
  TestAssert "single trueWord", trueWord -4 of "three trueWord sentence" is ""
end TestTrueWordExpressionsEmpty

on TestTrueWordLineBreakCount
  TestAssert "count with line break", the number of trueWords in ("one" & return & "two") is 2
end TestTrueWordLineBreakCount

on TestTrueWordLineBreakExpression
  TestAssert "single trueWord", trueWord 2 of ("one" & return & "two") is "two"
end TestTrueWordLineBreakExpression

on TestTrueWordCount
  TestAssert "count", the number of trueWords in "three trueWord sentence" is 3
end TestTrueWordCount

on TestLastTrueWord
	TestAssert "last trueWord", the last trueWord of "three trueWord sentence" is "sentence"
end TestLastTrueWord