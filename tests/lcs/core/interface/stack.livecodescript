script "CoreStackProps"
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

on TestStackNameProp
	create stack "Test"
	
	local tVar
	put "Test 1,2,3" into tVar
	set the name of stack "Test" to tVar
	TestAssert "Invalid chars in name are replaced with '_'", there is a stack "Test 1_2_3"
	
	# Bug 18343
	TestAssert "Variable used to set stack name is unmodified", tVar is "Test 1,2,3"
end TestStackNameProp

