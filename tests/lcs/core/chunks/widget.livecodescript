script "CoreChunksWidget"
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

on TestWidgetSubchunk
	-- Bug 18948
	create widget "me" as ""
	set the script of it to "function doTest; return the short name of button 1 of me; end doTest"
	
	local tResult
	dispatch function doTest to widget 1
	put the result into tResult
	
	TestAssert "'objchunk' of me in non-group non-behavior control is me", tResult is "me"
end TestWidgetSubchunk