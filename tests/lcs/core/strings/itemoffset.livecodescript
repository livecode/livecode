script "CoreStringItemOffset"
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

on TestItemOffsetWithEmptyItem
   set the wholeMatches to true
   set the itemDelimiter to "|"
   TestAssert "itemOffset with wholeMatches returns correct index when target preceded by non-empty item", \
              itemOffset("6", "|2|6") is 3
   TestAssert "itemOffset with wholeMatches returns correct index when target preceded by empty item", \
              itemOffset("6", "|6|") is 2
   TestAssert "itemOffset with wholeMatches returns correct index when target is first item", \
              itemOffset("6", "6|") is 1
end TestItemOffsetWithEmptyItem
