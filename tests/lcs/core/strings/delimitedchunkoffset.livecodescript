script "CoreStringsChunkOffset"
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

on TestItemOffset
  TestAssert "empty needle returns 0", itemOffset(empty, "a,b,c") is 0

  repeat for each word tMatch in "caseless exact"
    set the caseSensitive to tMatch is "exact"

    set the wholeMatches to false
    TestAssert tMatch && "partial : partial item offset at start", itemOffset("oo", "foof,barb,dezd") is 1
    TestAssert tMatch && "partial : partial item offset in middle", itemOffset("ar", "foof,barb,dezd") is 2
    TestAssert tMatch && "partial : partial item offset at end", itemOffset("ez", "foof,barb,dezd") is 3

    TestAssert tMatch && "partial : partial list item offset", itemOffset("of,barb,de", "foof,barb,dezd") is 1

    set the wholeMatches to true
    TestAssert tMatch && "whole : whole item offset at start", itemOffset("foof", "foof,barb,dezd") is 1
    TestAssert tMatch && "whole : whole item offset in middle", itemOffset("barb", "foof,barb,dezd") is 2
    TestAssert tMatch && "whole : whole item offset at end", itemOffset("dezd", "foof,barb,dezd") is 3
    TestAssert tMatch && "whole : whole list item offset", itemOffset("barb,dezd", "foof,barb,dezd") is 2

    TestAssert tMatch && "whole : not partial item offset at start", itemOffset("fo", "foof,barb,dezd") is 0
    TestAssert tMatch && "whole : not partial item offset in middle", itemOffset("ar", "foof,barb,dezd") is 0
    TestAssert tMatch && "whole : not partial item offset at end", itemOffset("zd", "foof,barb,dezd") is 0
    TestAssert tMatch && "whole : not partial list item offset", itemOffset("of,barb,de", "foof,barb,dezd") is 0
  end repeat

  set the wholeMatches to true
  set the caseSensitive to false
  TestAssert "caseless : different cased item offset", itemOffset("BARB", "foof,barb,dezd") is 2
  set the caseSensitive to true
  TestAssert "exact : not different cased item offset", itemOffset("BARB", "foof,barb,dezd") is 0

  set the wholeMatches to true
  set the caseSensitive to false
  TestAssert "caseless : skipped once whole item offset", itemOffset("dezd", "foof,barb,dezd", 1) is 2
  TestAssert "caseless : skipped too many whole item offset", itemOffset("dezd", "foof,barb,dezd", 3) is 0
end TestItemOffset

on TestLineOffset
  set the lineDelimiter to comma

  TestAssert "empty needle returns 0", lineOffset(empty, "a,b,c") is 0

  repeat for each word tMatch in "caseless exact"
    set the caseSensitive to tMatch is "exact"

    set the wholeMatches to false
    TestAssert tMatch && "partial : partial item offset at start", lineOffset("oo", "foof,barb,dezd") is 1
    TestAssert tMatch && "partial : partial item offset in middle", lineOffset("ar", "foof,barb,dezd") is 2
    TestAssert tMatch && "partial : partial item offset at end", lineOffset("ez", "foof,barb,dezd") is 3

    TestAssert tMatch && "partial : partial list item offset", lineOffset("of,barb,de", "foof,barb,dezd") is 1

    set the wholeMatches to true
    TestAssert tMatch && "whole : whole item offset at start", lineOffset("foof", "foof,barb,dezd") is 1
    TestAssert tMatch && "whole : whole item offset in middle", lineOffset("barb", "foof,barb,dezd") is 2
    TestAssert tMatch && "whole : whole item offset at end", lineOffset("dezd", "foof,barb,dezd") is 3
    TestAssert tMatch && "whole : whole list item offset", lineOffset("barb,dezd", "foof,barb,dezd") is 2

    TestAssert tMatch && "whole : not partial item offset at start", lineOffset("fo", "foof,barb,dezd") is 0
    TestAssert tMatch && "whole : not partial item offset in middle", lineOffset("ar", "foof,barb,dezd") is 0
    TestAssert tMatch && "whole : not partial item offset at end", lineOffset("zd", "foof,barb,dezd") is 0
    TestAssert tMatch && "whole : not partial list item offset", lineOffset("of,barb,de", "foof,barb,dezd") is 0
  end repeat

  set the wholeMatches to true
  set the caseSensitive to false
  TestAssert "caseless : different cased item offset", lineOffset("BARB", "foof,barb,dezd") is 2
  set the caseSensitive to true
  TestAssert "exact : not different cased item offset", lineOffset("BARB", "foof,barb,dezd") is 0

  set the wholeMatches to true
  set the caseSensitive to false
  TestAssert "caseless : skipped once whole item offset", lineOffset("dezd", "foof,barb,dezd", 1) is 2
  TestAssert "caseless : skipped too many whole item offset", lineOffset("dezd", "foof,barb,dezd", 3) is 0
end TestLineOffset

on TestChunkOffset
  set the itemDelimiter to "a"

  set the wholeMatches to true

  set the caseSensitive to false
  TestAssert "caseless lower delimiter chunk offset", itemOffset("...", "$$$a...a%%%") is 2
  TestAssert "caseless upper delimiter chunk offset", itemOffset("...", "$$$A...A%%%") is 2

  set the caseSensitive to true
  TestAssert "exact lower delimiter chunk offset", itemOffset("...", "$$$a...a%%%") is 2
  TestAssert "exact not upper delimiter chunk offset", itemOffset("...", "$$$A...A%%%") is 0
end TestChunkOffset
