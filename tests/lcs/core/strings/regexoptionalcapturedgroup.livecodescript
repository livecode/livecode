script "CoreStringsRegexOptCapturedGroup"

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


on TestRegexWithOptionalCapturedGroup
	local aRegex, aText
	local p0, p1, p2, p3
	local v1, v2
	
	-- see Bug 17245
	-- check that second group is not empty when 1st is.
	
	put "(a)?(B)" into aRegex
	put "B" into aText
	
	TestAssert "matchChunk should match", matchChunk( aText, aRegex, p0,p1,p2,p3) is true
	
	TestAssert "... and get empty for p0 and p1", ( p0 & p1 ) is empty
	
	TestAssert "... and get 1 for p2 and p3", ( p2 is 1) and (p3 is 1)
	
	TestAssert "matchText should match", matchText( aText, aRegex, v0, v1) is true
	
	TestAssert "... and get empty for v0", v0 is empty
	
	TestAssert "... and get B for v1", v1 is "B"
	
	
end TestRegexWithOptionalCapturedGroup
