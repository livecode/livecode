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
	return "M1664 698Q1664 720 1638 746L1275 1100 1361 1600Q1362 1607 1362 1620 1362 1641 1351.5 1655.5 1341 1670 1321 1670 1302 1670 1281 1658L832 1422 383 1658Q361 1670 343 1670 322 1670 311.5 1655.5 301 1641 301 1620 301 1614 303 1600L389 1100 25 746Q0 719 0 698 0 661 56 652L558 579 783 124Q802 83 832 83 862 83 881 124L1106 579 1608 652Q1664 661 1664 698Z"
end getPathData

on TestSetup
	TestLoadExtension "com.livecode.library.widgetutils"
	TestLoadExtension "com.livecode.library.iconSVG"
	TestLoadExtension "com.livecode.widget.svgPath"
	create widget "testSvgPath" as "com.livecode.widget.svgPath"
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
	set the rect of widget "testSvgPath" to "0,0,100,100"
	TestAssert "scaledHeight with maintainAspectRatio = true", round(the scaledHeight of widget "testSvgPath",3) is 95.373

	set the rect of widget "testSvgPath" to "0,0,150,100"
	TestAssert "scaledWidth with maintainAspectRatio = true", round(the scaledWidth of widget "testSvgPath",3) is 104.852

	set the maintainAspectRatio of widget "testSvgPath" to false
	TestAssert "scaledHeight with maintainAspectRatio = false", the scaledHeight of widget "testSvgPath" is 100
	TestAssert "scaledWidth with maintainAspectRatio = false", the scaledWidth of widget "testSvgPath" is 150
end TestScaledSizing
