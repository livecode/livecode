script "CoreGraphics"
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

on TestGraphicsColors


TestAssert "test", "red" is a color
TestAssert "test", not ("red" is not a color)

# Bug 157 (!) - all integers are colors
TestSkip "test", /* not (257 is a color) */ "bug 157"

TestAssert "test", "0,0,0" is a color

TestAssert "test", "abc" is not a color
TestAssert "test", 1.5 is not a color

TestAssert "test", "0,0" is not a color
TestAssert "test", "0,0,0,0" is not a color
TestAssert "test", "0,abc,0" is not a color
TestAssert "test", "red,green,blue" is not a color

end TestGraphicsColors
on TestGraphicsPoints


TestAssert "test", "123,456" is a point
TestAssert "test", not ("123,456" is not a point)

TestAssert "test", 123 is not a point
TestAssert "test", 1.5 is not a point
TestAssert "test", "abc" is not a point

TestAssert "test", "0,0,0" is not a point

TestAssert "test", "0,abc" is not a point

TestAssert "test", empty is not a point

end TestGraphicsPoints
on TestGraphicsRectangles


TestAssert "test", "12,34,56,78" is a rectangle
TestAssert "test", not ("12,34,56,78" is not a rectangle)

TestAssert "test", 123 is not a rectangle
TestAssert "test", 1.5 is not a rectangle
TestAssert "test", "abc" is not a rectangle

TestAssert "test", "0,0,0" is not a rectangle
TestAssert "test", "0,0,0,0,0" is not a rectangle

TestAssert "test", "0,0,0,abc" is not a rectangle

TestAssert "test", empty is not a rectangle

end TestGraphicsRectangles

on TestGraphicsIsWithin

TestAssert "test", "0,0" is within "-5,-10,5,10"
TestAssert "test", "-5,-10" is within "-5,-10,5,10"
TestAssert "test", "4,9" is within "-5,-10,5,10"
TestAssert "test", not ("-6,-11" is within "-5,-10,5,10")
TestAssert "test", not ("5,10" is within "-5,-10,5,10")

end TestGraphicsIsWithin

on TestGraphicsIsNotWithin

TestAssert "test", not ("0,0" is not within "-5,-10,5,10")
TestAssert "test", not ("-5,-10" is not within "-5,-10,5,10")
TestAssert "test", not ("4,9" is not within "-5,-10,5,10")
TestAssert "test", "-6,-11" is not within "-5,-10,5,10"
TestAssert "test", "5,10" is not within "-5,-10,5,10"

end TestGraphicsIsNotWithin

-- new brush mapping for 101..135
-- brush < 101 = brush
-- 101 < brush < 137 = brush - 100
-- brush > 136 = brush

on TestBrushWithNoIcons
	TestAssert "set brush with revicons stack not loaded", SetBrushNoIcons() is not 2
end TestBrushWithNoIcons

on TestInitialPaintConditions
#	open stack TestGetIDERepositoryPath() & "/Toolset/palettes/revicons.rev"
	TestAssert "brush is initially 8", the brush is 8
	TestAssert "spray is initially 34", the spray is 34
	TestAssert "eraser is initially 2", the eraser is 2
	
	TestAssert "the centered is initially false", the centered is false
	TestAssert "the filled is initially false", the filled is false
	
	TestAssert "the grid is initially true", the grid is true

	TestAssert "the gridSize is initially 4", the gridSize is 4
	TestAssert "the lineSize is initially 0", the lineSize is 0

	TestAssert "the multiple is initially false", the multiple is false
	TestAssert "the multiSpace is initially 1", the multiSpace is 1

# not sure if this should work. it doesn't in previous builds either
	TestAssert "the penPattern is initially empty", the penPattern is empty
	TestAssert "the brushPattern is initially empty", the brushPattern is empty
	TestAssert "the polySides is initially 4", the polySides is 4

	TestAssert "the roundEnds is initially false", the roundEnds is false
	TestAssert "the slices is initially 16", the slices is 16
	TestAssert "the penColor is initially 0,0,0", the penColor is "0,0,0"
	TestAssert "the brushColor is initially 255,255,255", the brushColor is "255,255,255"
end TestInitialPaintConditions

on TestBadBrush100
	open stack TestGetIDERepositoryPath() & "/Toolset/palettes/revicons.rev"
	set the brush to 8
	TestAssert "no brush with id=100", SetBrush100() is not 100
	TestAssert "brush image not found", the brush is 8
end TestBadBrush100

on TestBadBrushZero
	open stack TestGetIDERepositoryPath() & "/Toolset/palettes/revicons.rev"
	set the brush to 8
	TestAssert "no brush with id=0", SetBrushZero() is not zero
	TestAssert "brush image not found", the brush is 8
end TestBadBrushZero

function SetBrushNoIcons
	try
		set the brush to 2
	catch e
		TestDiagnostic e
	end try
	return the brush
end SetBrushNoIcons

function SetBrush100
	try
		set the brush to 100
	catch e
		TestDiagnostic e
	end try
	return the brush
end SetBrush100

function SetBrushZero
	try
		set the brush to 0
	catch e
		TestDiagnostic e
	end try
	return the brush
end SetBrushZero

on TestBrush
	open stack TestGetIDERepositoryPath() & "/Toolset/palettes/revicons.rev"
	set the brush to 30
	TestAssert "brush < 37 unchanged", the brush is 30
end TestBrush

on TestRemappedBrush
	local tBrush
	
	open stack TestGetIDERepositoryPath() & "/Toolset/palettes/revicons.rev"
	repeat with tBrush = 101 to 136
		set the brush to tBrush
		TestAssert "setting brush to" && tBrush && "subtracts 100", the brush is (tBrush-100)
	end repeat
end TestRemappedBrush

on TestBrushInBounds
	local tBrush
	
	open stack TestGetIDERepositoryPath() & "/Toolset/palettes/revicons.rev"
	repeat with tBrush = 1 to 36
		set the brush to tBrush
		TestAssert "setting brush to" && tBrush && "is unchanged", the brush is tBrush
	end repeat
end TestBrushInBounds

on TestBrushBoundaries
	open stack TestGetIDERepositoryPath() & "/Toolset/palettes/revicons.rev"
#	set the brush to 99
#	TestAssert "brush = 99 unchanged", the brush is 99
	set the brush to 137
	TestAssert "brush can be set to a pattern", the brush is 137
end TestBrushBoundaries

on TestSpray
	open stack TestGetIDERepositoryPath() & "/Toolset/palettes/revicons.rev"
	set the spray to 2
	TestAssert "directly setting the spray", the spray is 2
end TestSpray

on TestEraser
	open stack TestGetIDERepositoryPath() & "/Toolset/palettes/revicons.rev"
	set the eraser to 8
	TestAssert "directly setting the eraser", the eraser is 8
end TestEraser

on TestResetPaint
	open stack TestGetIDERepositoryPath() & "/Toolset/palettes/revicons.rev"
	reset paint
	TestAssert "brush is reset to 8", the brush is 8
	TestAssert "spray is reset to 34", the spray is 34
	TestAssert "eraser is reset to 2", the eraser is 2
	
	TestAssert "the centered is reset to false", the centered is false
	TestAssert "the filled is reset to false", the filled is false
	TestAssert "the grid is reset to false", the grid is false

	TestAssert "the gridSize is reset to 8", the gridSize is 8
	TestAssert "the lineSize is reset to 1", the lineSize is 1

	TestAssert "the multiple is reset to false", the multiple is false
	TestAssert "the multiSpace is reset to 1", the multiSpace is 1

# not sure if this should be empty. it isn't in previous builds either
	TestAssert "the penPattern is reset to empty", the penPattern is empty
	TestAssert "the brushPattern is reset to empty", the brushPattern is empty
	TestAssert "the polySides is reset to 4", the polySides is 4

	TestAssert "the roundEnds is reset to false", the roundEnds is false
	TestAssert "the slices is reset to 16", the slices is 16
	TestAssert "the penColor is reset to 0,0,0", the penColor is "0,0,0"
	TestAssert "the brushColor is reset to 255,255,255", the brushColor is "255,255,255"
end TestResetPaint

on TestTeardown
	if there is a stack "revicons" then
		close stack "revicons"
	end if
end TestTeardown


command _TestGraphicsPointOverflowNoThrow
	local tPoints

	repeat with i = 1 to 256
		repeat with j = 1 to 256
			if i is 256 and j is 256 then
				exit repeat 
				exit repeat
			end if
			put (i,j) & cr after tPoints
		end repeat
	end repeat

	TestAssert "the number of gen points is correct", the number of lines in tPoints is 65535

	create graphic
	set the points of it to tPoints
	set the markerPoints of it to tPoints
end _TestGraphicsPointOverflowNoThrow

on TestGraphicPointOverflow
	local tE1, tE2, tE3, tE4, tE5, tPoints

	repeat with i = 1 to 256
		repeat with j = 1 to 256
			put (i,j) & cr after tPoints
		end repeat
	end repeat

	TestAssert "generated correct number of points", the number of lines in tPoints is 65536

	try
		create graphic 
		set the points of it to tPoints
	catch tError
		put tError into tE1
	end try

	try 
		create graphic "g1"
		set the markerPoints of it to tPoints
	catch tError
		put tError into tE2
	end try

	try
		create graphic "g2"
		set the relativePoints of it to tPoints
	catch tError
		put tError into tE3
	end try

	create graphic "g3"
	set the filled of graphic "g3" to false
	set the style of graphic "g3" to "polygon"
	set the points of graphic "g3" to line 1 to 65535 of tPoints

	try
		set the filled of graphic "g3" to true
	catch tError
		put tError into tE4
	end try

	create graphic "g4"
	set the style of graphic "g4" to "polygon"
	set the filled of graphic "g4" to true
	set the points of graphic "g4" to line 1 to 65534 of tPoints

	try
		set the points of graphic "g4" to line 1 to 65535 of tPoints
	catch tError
		put tError into tE5
	end try

	TestAssert "didn't throw", true
	TestAssert "throws (points)", item 1 of line 1 of tE1 is "0896"
	TestAssert "throws (markerPoints)", item 1 of line 1 of tE2 is "0896"
	TestAssert "throws (relativePoints)", item 1 of line 1 of tE3 is "0896"
	TestAssert "throws (switchOpaque)", item 1 of line 1 of tE4 is "0896"
	TestAssert "throws (setOpaque)", item 1 of line 1 of tE5 is "0896"
end TestGraphicPointOverflow