script "CoreChunksCompound"
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

on TestBug17112
	local tLines
	put "a" & return & "b" into tLines
	TestAssert "char range of line end overrun", char 1 to 100 of line 1 of tLines is "a"
end TestBug17112

on TestBug17391
   local tVar
   put "a" & return & "b" into tVar
   TestAssert "char range of line start overrun", char 3 to -1 of line 1 of tVar is ""
end TestBug17391

on TestPutIntoItemOfLineAdd
	local tLines
	put "a" & return & "1" into tLines
	put "b" into item 2 of line 1 of tLines
	TestAssert "force item of line addition expression", tLines is ("a,b" & return & "1")
	
	put "a" & return & "1" into tLines
	put "b" into item 3 to 4 of line 1 of tLines
	TestAssert "force item of line addition range", tLines is ("a,,b" & return & "1")
	
	put "a" & return & "1" into tLines
	put "b" into the third item of line 1 of tLines
	TestAssert "force item of line addition ordinal", tLines is ("a,,b" & return & "1")
end TestPutIntoItemOfLineAdd

on TestPutIntoItemOfLineReplace
	local tLines
	put "a,b" & return & "1" into tLines
	put "c" into item 2 of line 1 of tLines
	TestAssert "item of line replacement expression", tLines is ("a,c" & return & "1")
	
	put "a,b,c,d" & return & "1" into tLines
	put "e" into item 3 to 4 of line 1 of tLines
	TestAssert "item of line replacement range", tLines is ("a,b,e" & return & "1")
	
	put "a,b,c" & return & "1" into tLines
	put "d" into the middle item of line 1 of tLines
	TestAssert "item of line replacement ordinal", tLines is ("a,d,c" & return & "1")
end TestPutIntoItemOfLineReplace
