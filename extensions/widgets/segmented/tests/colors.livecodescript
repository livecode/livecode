script "SegmentedControlColors"
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
	TestLoadExtension "com.livecode.library.iconSVG"
	TestLoadExtension "com.livecode.library.widgetutils"
	TestLoadExtension "com.livecode.widget.segmented"
end TestSetup

function randomColor
	local tColorArray
	repeat with x = 1 to 3
		put random(256) - 1 into tColorArray[x]
	end repeat
	combine tColorArray with comma
	return tColorArray
end randomColor

on TestColors
	create stack
	create widget as "com.livecode.widget.segmented"
	
	local tColor
	
	put randomColor() into tColor	
	set the segmentColor of it to tColor
	TestAssert "set segment color", the segmentColor of it is tColor
	
	put randomColor() into tColor
	set the segmentSelectedColor of it to tColor
	TestAssert "set segment selected color", the segmentSelectedColor of it is tColor
	
	put randomColor() into tColor
	set the segmentLabelColor of it to tColor
	TestAssert "set segment label color", the segmentLabelColor of it is tColor
	
	put randomColor() into tColor
	set the segmentSelectedLabelColor of it to tColor
	TestAssert "set segment selected color", the segmentSelectedLabelColor of it is tColor
end TestColors

on TestEmptyColors
	create stack
	create widget as "com.livecode.widget.segmented"
	
	local tColor
	
	put randomColor() into tColor	
	set the segmentColor of it to tColor
	set the segmentColor of it to empty
	TestAssert "set segment color empty", the segmentColor of it is empty
	
	put randomColor() into tColor
	set the segmentSelectedColor of it to tColor
	set the segmentSelectedColor of it to empty
	TestAssert "set segment selected color empty", the segmentSelectedColor of it is empty
	
	put randomColor() into tColor
	set the segmentLabelColor of it to tColor
	set the segmentLabelColor of it to empty
	TestAssert "set segment label color", the segmentLabelColor of it is empty
	
	put randomColor() into tColor
	set the segmentSelectedLabelColor of it to tColor
	set the segmentSelectedLabelColor of it to empty
	TestAssert "set segment selected color", the segmentSelectedLabelColor of it is empty
end TestEmptyColors

on TestDisplayText
	create stack
	create widget as "com.livecode.widget.segmented"
	set the segmentDisplay of it to "text"
	TestAssert "no runtime error when setting segment display to text", true
end TestDisplayText