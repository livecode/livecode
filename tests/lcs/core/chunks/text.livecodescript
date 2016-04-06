script "CoreChunksText"
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

on TestDeleteSegmentOfLine
	local tLines
	put "a" & return & "b" & return & "c" into tLines
	delete word 1 of line 2 of tLines
	TestAssert "delete word of line comprising word does not delete line", the number of lines in tLines is 3
end TestDeleteSegmentOfLine

on TestDeleteItemOfLine
	local tLines
	put "a" & return & "b" & return & "c" into tLines
	delete item 1 of line 2 of tLines
	TestAssert "delete item of line comprising item does not delete line", the number of lines in tLines is 3
end TestDeleteItemOfLine

on TestPutIntoItemAdd
	local tItems
	put "a" into tItems
	put "b" into item 2 of tItems
	TestAssert "force item addition expression", tItems is "a,b"
	
	put "a" into tItems
	put "b" into item 3 to 4 of tItems
	TestAssert "force item addition range", tItems is "a,,b"
	
	put "a" into tItems
	put "b" into the third item of tItems
	TestAssert "force item addition ordinal", tItems is "a,,b"
end TestPutIntoItemAdd

on TestPutIntoItemReplace
	local tItems
	put "a,b" into tItems
	put "c" into item 2 of tItems
	TestAssert "item replacement expression", tItems is "a,c"
	
	put "a,b,c,d" into tItems
	put "e" into item 3 to 4 of tItems
	TestAssert "item replacement range", tItems is "a,b,e"
	
	put "a,b,c" into tItems
	put "d" into the middle item of tItems
	TestAssert "item replacement ordinal", tItems is "a,d,c"
end TestPutIntoItemReplace

