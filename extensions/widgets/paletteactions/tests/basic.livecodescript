script "PaletteActions"
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

on TestSetup
	TestLoadExtension "com.livecode.library.widgetutils"
	TestLoadExtension "com.livecode.library.iconsvg"
	TestLoadExtension "com.livecode.widget.paletteactions"
end TestSetup

on TestSortOrder
	create widget "Actions" as "com.livecode.widget.paletteactions"
	
	local tArray
	repeat with x = 1 to 10
		put x into tArray[x]["name"]
	end repeat
	set the navData of widget "Actions" to tArray
	
	local tData
	put the navData of widget "Actions" into tData
	
	get the keys of tData
	repeat for each line tLine in it
		TestAssert "palette nav items stay in numeric order", tData[tLine]["name"] + 0 is tLine + 0
	end repeat
end TestSortOrder