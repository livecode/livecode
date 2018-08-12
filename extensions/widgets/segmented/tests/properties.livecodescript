script "SegmentedControlProperties"
/*
Copyright (C) 2017 LiveCode Ltd.

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
	TestLoadExtension "com.livecode.library.iconsvg"
	TestLoadExtension "com.livecode.library.widgetutils"
	TestLoadExtension "com.livecode.library.scriptitems"
	TestLoadExtension "com.livecode.widget.segmented"
end TestSetup

-- Bug 19720
on TestSetItemCount
	create stack
	create widget as "com.livecode.widget.segmented"
	set the itemCount of it to 1
	TestAssert "set segment count", the itemCount of it is 1
	
	set the itemCount of it to 5
	TestAssert "set segment count", the itemCount of it is 5
end TestSetItemCount
