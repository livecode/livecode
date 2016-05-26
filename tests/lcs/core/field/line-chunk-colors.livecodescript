script "CoreFieldLineChunkColors"
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

on TestColorsOfLineChunk
	create field "Test"
	put "a" & return & "b" into field "Test"
	set the borderColor of line 1 of field "Test" to 127,127,127
	set the backgroundColor of line 1 of field "Test" to 128,128,128
	TestDiagnostic the borderColor of line 1 of field "Test"
	TestAssert "borderColor of line of field sets correctly", the borderColor of line 1 of field "Test" is "127,127,127"
	TestAssert "backgroundColor of line of field sets correctly", the backgroundColor of line 1 of field "Test" is "128,128,128"
end TestColorsOfLineChunk
