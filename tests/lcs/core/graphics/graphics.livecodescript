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