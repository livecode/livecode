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
	TestSkipIfNot "ide"
	
	TestAssert "set brush with revicons stack not loaded", SetBrushNoIcons() is not 2
end TestBrushWithNoIcons

on TestInitialPaintConditions
	TestSkipIfNot "ide"
	get the long id of stack (TestGetIDERepositoryPath() & "/Toolset/palettes/revicons.rev")

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

	TestAssert "the penPattern is initially empty", the penPattern is empty
	TestAssert "the brushPattern is initially empty", the brushPattern is empty
	TestAssert "the polySides is initially 4", the polySides is 4

	TestAssert "the roundEnds is initially false", the roundEnds is false
	TestAssert "the slices is initially 16", the slices is 16
	TestAssert "the penColor is initially 0,0,0", the penColor is "0,0,0"
	TestAssert "the brushColor is initially 255,255,255", the brushColor is "255,255,255"
end TestInitialPaintConditions

on TestBadBrush100
	TestSkipIfNot "ide"
	get the long id of stack (TestGetIDERepositoryPath() & "/Toolset/palettes/revicons.rev")

	set the brush to 8
	TestAssert "no brush with id=100", SetBrush100() is not 100
	TestAssert "brush image not found", the brush is 8
end TestBadBrush100

on TestBadBrushZero
	TestSkipIfNot "ide"
	get the long id of stack (TestGetIDERepositoryPath() & "/Toolset/palettes/revicons.rev")

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
	TestSkipIfNot "ide"
	get the long id of stack (TestGetIDERepositoryPath() & "/Toolset/palettes/revicons.rev")

	set the brush to 30
	TestAssert "brush < 37 unchanged", the brush is 30
end TestBrush

on TestRemappedBrush
	TestSkipIfNot "ide"
	get the long id of stack (TestGetIDERepositoryPath() & "/Toolset/palettes/revicons.rev")

	local tBrush
	
	repeat with tBrush = 101 to 136
		set the brush to tBrush
		TestAssert "setting brush to" && tBrush && "subtracts 100", the brush is (tBrush-100)
	end repeat
end TestRemappedBrush

on TestBrushInBounds
	TestSkipIfNot "ide"
	get the long id of stack (TestGetIDERepositoryPath() & "/Toolset/palettes/revicons.rev")

	local tBrush
	
	repeat with tBrush = 1 to 36
		set the brush to tBrush
		TestAssert "setting brush to" && tBrush && "is unchanged", the brush is tBrush
	end repeat
end TestBrushInBounds

on TestBrushBoundaries
	TestSkipIfNot "ide"
	get the long id of stack (TestGetIDERepositoryPath() & "/Toolset/palettes/revicons.rev")

#	set the brush to 99
#	TestAssert "brush = 99 unchanged", the brush is 99
	set the brush to 137
	TestAssert "brush can be set to a pattern", the brush is 137
end TestBrushBoundaries

on TestSpray
	TestSkipIfNot "ide"
	get the long id of stack (TestGetIDERepositoryPath() & "/Toolset/palettes/revicons.rev")

	set the spray to 2
	TestAssert "directly setting the spray", the spray is 2
end TestSpray

on TestEraser
	TestSkipIfNot "ide"
	get the long id of stack (TestGetIDERepositoryPath() & "/Toolset/palettes/revicons.rev")

	set the eraser to 8
	TestAssert "directly setting the eraser", the eraser is 8
end TestEraser

on TestResetPaint
	TestSkipIfNot "ide"
	get the long id of stack (TestGetIDERepositoryPath() & "/Toolset/palettes/revicons.rev")

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

on _TestGraphicPointOverflowHandler1
	local tPoints

	repeat with i = 1 to 256
		repeat with j = 1 to 256
			put (i,j) & cr after tPoints
		end repeat
	end repeat

	create graphic 
	set the points of it to tPoints
end _TestGraphicPointOverflowHandler1
	
on _TestGraphicPointOverflowHandler2
	local tPoints

	repeat with i = 1 to 256
		repeat with j = 1 to 256
			put (i,j) & cr after tPoints
		end repeat
	end repeat

	create graphic "g1"
	set the markerPoints of it to tPoints
end _TestGraphicPointOverflowHandler2

on _TestGraphicPointOverflowHandler3
	local tPoints

	repeat with i = 1 to 256
		repeat with j = 1 to 256
			put (i,j) & cr after tPoints
		end repeat
	end repeat

	create graphic "g2"
	set the relativePoints of it to tPoints
end _TestGraphicPointOverflowHandler3

on _TestGraphicPointOverflowHandler4 tGraphic
	set the filled of tGraphic to true
end _TestGraphicPointOverflowHandler4

on _TestGraphicPointOverflowHandler5 tGraphic
	set the points of tGraphic[1] to line 1 to 65535 of tGraphic[2]
end _TestGraphicPointOverflowHandler5

on _TestGraphicPointOverflowHandler6 tParams
	set the relativePoints of tParams[1] to line 1 to 65535 of tParams[2]
end _TestGraphicPointOverflowHandler6

on _TestGraphicPointOverflowHandler7 tParams
	set the markerPoints of tParams[1] to line 1 to 65535 of tParams[2]
end _TestGraphicPointOverflowHandler7

on TestGraphicPointOverflow
	local tE1, tE2, tE3, tE4, tE5, tPoints

	repeat with i = 1 to 256
		repeat with j = 1 to 256
			put (i,j) & cr after tPoints
		end repeat
	end repeat

	TestAssert "generated correct number of points", the number of lines in tPoints is 65536

	TestAssertThrow "handler 1" , "_TestGraphicPointOverflowHandler1", the long id of me, \
		"EE_GRAPHIC_TOOMANYPOINTS"
	
	TestAssertThrow "handler 2", "_TestGraphicPointOverflowHandler2" , the long id of me, \
		"EE_GRAPHIC_TOOMANYPOINTS"
	
	TestAssertThrow "handler 3", "_TestGraphicPointOverflowHandler3", the long id of me, \
		"EE_GRAPHIC_TOOMANYPOINTS"
	
	create graphic "g3"
	set the filled of graphic "g3" to false
	set the style of graphic "g3" to "polygon"
	set the points of graphic "g3" to line 1 to 65535 of tPoints

	TestAssertThrow "handler 4", "_TestGraphicPointOverflowHandler4", the long id of me, \
		"EE_GRAPHIC_TOOMANYPOINTS" , the long id of graphic "g3"

	TestAssert "internal object unchanged (switchOpaque)", the filled of graphic "g3" is false
	
	local tClosedPoints, tParams

	create graphic "g4"
	set the style of graphic "g4" to "polygon"
	set the filled of graphic "g4" to true
	set the points of graphic "g4" to line 1 to 65534 of tPoints

	put the points of graphic "g4" into tClosedPoints

	put the long id of graphic "g4" into tParams[1]
	put tPoints into tParams[2]

	TestAssertThrow "handler 5", "_TestGraphicPointOverflowHandler5", the long id of me, \
		"EE_GRAPHIC_TOOMANYPOINTS", tParams

	TestAssert "internal object unchanged (setOpaque)", the points of graphic "g4" is tClosedPoints

	local tClosedRelativePoints,tE6

	create graphic "g5"
	set the style of graphic "g5" to "polygon"
	set the filled of graphic "g5" to true
	set the relativePoints of graphic "g5" to line 1 to 65534 of tPoints
	
	TestAssert "points added", the number of lines in the points of graphic "g5" is 65535
	put the points of graphic "g5" into tClosedRelativePoints

	put the long id of graphic "g5" into tParams[1]

	TestAssertThrow "handler 6", "_TestGraphicPointOverflowHandler6", the long id of me, \
		"EE_GRAPHIC_TOOMANYPOINTS", tParams
	TestAssert "internal object unchanged (set relative opaque)", the points of graphic "g5" is tClosedRelativePoints

	local tClosedMarkerPoints, tE7

	create graphic "g6"
	set the style of graphic "g6" to "polygon"
	set the markerFilled of graphic "g6" to true
	set the markerPoints of graphic "g6" to line 1 to 65534 of tPoints

	put the points of graphic "g6" into tClosedMarkerPoints
	put the long id of graphic "g6" into tParams[1]

	TestAssertThrow "handler 7", "_TestGraphicPointOverflowHandler7", the long id of me ,\
		"EE_GRAPHIC_TOOMANYPOINTS", tParams
	TestAssert "internal object unchanged (set marker opaque)", the points of graphic "g6" is tClosedMarkerPoints


	_TestGraphicsPointOverflowNoThrow
	TestAssert "didn't throw", true
end TestGraphicPointOverflow

on TestTeardown
	if there is a stack "revicons" then
		close stack "revicons"
	end if
end TestTeardown
