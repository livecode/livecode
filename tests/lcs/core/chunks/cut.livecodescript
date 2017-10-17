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
	TestSkipIf "environment", "command line"
	TestSkipIfNot "clipboard"
end TestSetup

on TestCutVar
	create stack
	set the defaultStack to the short name of it
	
	local tField
	create field
	put it into tField
	set the text of tField to ("a" & return & "b")
	
	local tToCut
	put "line 1 of" && tField into tToCut
	
	cut tToCut
	
	TestAssert "cut obj text chunk in variable", the text of tField is "b"
	TestAssert "clipboard after cut obj text chunk in variable", \ 
		the clipboardData["text"] is ("a" & return)
end TestCutVar