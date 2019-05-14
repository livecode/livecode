script "CoreEngineExtensionsCanvasModule"
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
   TestSkipIfNot "lcb"
   
   TestLoadAuxiliaryExtension "_canvas", TestGetInputFilesFolder()
   TestLoadAuxiliaryExtension "_widget_canvas"
end TestSetup

on TestCanvasGetPixelHeight
	TestAssert "canvas pixel height returned correctly", \
		CanvasTestPixelHeight()
end TestCanvasGetPixelHeight

on TestCanvasImageFromResourceFile
	TestAssert "canvas image from resource file not empty", \
		CanvasTestImageFromResourceFile()
end TestCanvasImageFromResourceFile

on TestCanvasImageFromFile
	local tDefaultStack
	put the defaultStack into tDefaultStack

	set the defaultStack to the short name of me
	
	local tWidget
	create widget as "com.livecode.lcs_tests.core.engine_lcb.widget_canvas"
	put it into tWidget

	TestAssert \
			"image from file in library path relative to current card of the defaultStack", \
			TestCanvas_ImageFromFile("_test_image.png")

	-- test implicit widget with the default stack elsewhere
	local tPath
	set the defaultStack to "TestRunner"
	-- will throw if not found
	set the imagePath of tWidget to "_test_image.png"
	put the imagePath of tWidget into tPath
	TestAssert \
			"image from file in widget path relative to widget", \
			tPath is "_test_image.png"

	set the defaultStack to tDefaultStack
end TestCanvasImageFromFile