script "CoreChunksField"
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

on TestFieldChunkInsert
	create field
	set the text of field 1 to ("a" & return & "b")
	put "x" into item 2 of line 1 of field 1
	
	TestAssert "insert chunk on correct line", the text of field 1 is ("a,x" & return & "b")
end TestFieldChunkInsert