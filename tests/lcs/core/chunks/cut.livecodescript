script "CoreChunksCut"
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

on TestSetup
	if the environment contains "command line" then
		return "SKIP UI-only test"
	end if
end TestSetup

on TestCutVar
	create field
	put "a" & return & "b" into field 1
	
	local tToCut
	put "line 1 of field 1" into tToCut
	
	cut tToCut
	
	TestDiagnostic the text of field 1
	TestDiagnostic the clipboardData["text"]
	
	TestAssert "cut obj text chunk in variable", field 1 is "b"
	TestAssert "clipboard after cut obj text chunk in variable", \ 
		the clipboardData["text"] is ("a")
end TestCutVar