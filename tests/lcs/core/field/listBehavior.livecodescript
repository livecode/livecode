script "CoreFieldListBehavior"
/*
Copyright (C) 2019 LiveCode Ltd.

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

on TestListBehaviorHilitedLines
	/* the hilitedLines of a field only work if the field is open so open
	 * this stack */
	go this stack

	reset the templateField
	create field "Test"
	set the dontWrap of field "Test" to true
	set the autoHilite of field "Test" to true
	set the listBehavior of field "Test" to true
	set the nonContiguousHilites of field "Test" to true
	set the multipleHilites of field "Test" to true
	set the text of field "Test" to format("first\nsecond\nthird\nfourth\nfifth")

	TestAssert "hilitedLines start empty", the hilitedLines of field "Test" is empty

	set the hilitedLines of field "Test" to "2,4"
	TestAssert "hilitedLines roundtrip", the hilitedLines of field "Test" is "2,4"

	set the width of field "Test" to the width of field "Test" * 2
	set the hilitedLines of field "Test" to "2,4"
	TestAssert "hilitedLines survive field resize", the hilitedLines of field "Test" is "2,4"

	set the hilitedLines of field "Test" to "2,4"
	focus on field "Test"
	focus on nothing
	TestAssert "hilitedLines survive focus out", the hilitedLines of field "Test" is "2,4"

	set the hilitedLines of field "Test" to "2,4"
	focus on field "Test"
	TestAssert "hilitedLines survive focus in", the hilitedLines of field "Test" is "2,4"
end TestListBehaviorHilitedLines
