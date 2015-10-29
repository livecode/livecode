script "CoreChunksChar"
/*
Copyright (C) 2015 Runtime Revolution Ltd.

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

on TestCharRanges
 TestAssert "overlong prefix", char -4 to -2 of "foo" is "fo"
 TestAssert "overlong suffix", char 2 to 4 of "foo" is "oo"
end TestCharRanges

on TestCharRangesEmpty
  TestAssert "empty prefix", char -5 to -4 of "foo" is ""
  TestAssert "empty suffix", char 4 to 5 of "foo" is ""
  TestAssert "empty range", char 5 to 4 of "foo" is ""
end TestCharRangesEmpty

on TestCharRangesSingle
  TestAssert "single char range", char 1 to 1 of "foo" is "f"
  TestAssert "single char range from back", char -3 to -3 of "foo" is "f"
end TestCharRangesSingle

on TestCharExpressions
  TestAssert "single char", char 1 of foo is "f"
  TestAssert "single char", char -3 of foo is "f"
end TestCharExpressions

on TestCharExpressionsEmpty
  TestAssert "single char", char 4 of foo is ""
  TestAssert "single char", char -4 of foo is ""
end TestCharExpressionsEmpty

on TestBug16124
 TestAssert "char -2 to -1 of 7 is 7", char -2 to -1 of 7 is 7
end TestBug16124