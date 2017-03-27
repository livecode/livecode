script "SvgPathWidgetProperties"
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

private function getPathData
	return "M10 10 H 90 V 90 H 10 L 10 10"
end getPathData

on TestSetup
	TestLoadExtension "com.livecode.library.widgetutils"
	TestLoadExtension "com.livecode.library.iconsvg"
	TestLoadExtension "com.livecode.widget.svgpath"
	create widget "testSvgPath" as "com.livecode.widget.svgpath"
end TestSetup

on TestTeardown
	delete widget "testSvgPath"
end TestTeardown

on TestCreateSvgPath
	TestAssert "created svgPath exists", exists(widget "testSvgPath")
end TestCreateSvgPath

on TestSetPath
	set the iconPath of widget "testSvgPath" to getPathData()
	TestAssert "iconPath set to default 'star' value", the iconPath of widget "testSvgPath" is getPathData()
end TestSetPath

on TestMaintainAspectRatio
	set the maintainAspectRatio of widget "testSvgPath" to true
	TestAssert "maintainAspectRatio set to true", the maintainAspectRatio of widget "testSvgPath" is true

	set the maintainAspectRatio of widget "testSvgPath" to false
	TestAssert "maintainAspectRatio set to false", the maintainAspectRatio of widget "testSvgPath" is false
end TestMaintainAspectRatio

on TestSetRect
	set the rect of widget "testSvgPath" to "0,0,100,100"
	TestAssert "the rect was set to 0,0,100,100", the rect of widget "testSvgPath" is "0,0,100,100"
end TestSetRect

on TestScaledSizing
	set the iconPath of widget "testSvgPath" to getPathData()
	set the maintainAspectRatio of widget "testSvgPath" to true

	set the rect of widget "testSvgPath" to "0,0,90,100"
	TestAssert "scaled sizing with maintainAspectRatio = true", (round(the scaledWidth of widget "testSvgPath") is 90 and round(the scaledHeight of widget "testSvgPath") is 90)

	set the maintainAspectRatio of widget "testSvgPath" to false
	TestAssert "scaled sizing with maintainAspectRatio = false", (round(the scaledWidth of widget "testSvgPath") is 90 and round(the scaledHeight of widget "testSvgPath") is 100)
end TestScaledSizing
