script "CoreInterface"
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

on TestInterface1

TestAssert "test", the capsLockKey is "up"

end TestInterface1


on TestInterface11

TestAssert "test", the colornames is not empty
TestAssert "test", "red" is among the lines of the colornames
TestAssert "test", "green" is among the lines of the colornames
TestAssert "test", "blue" is among the lines of the colornames

end TestInterface11
on TestInterface12

TestAssert "test", the commandKey is "up"

end TestInterface12
on TestInterface18

create stack "Test"
set the defaultstack to "Test"
create field "tf"

set the text of field "tf" to "abcd"
find characters "b" in field "tf"
TestAssert "test", the foundChunk is "char 2 to 2 of field 1" or the foundChunk is "char 2 to 2 of field 2"

find characters "x" in field "tf"
TestAssert "test", the foundChunk is empty

end TestInterface18
on TestInterface19

create stack "Test"
set the defaultstack to "Test"
create field "tf"

set the text of field "tf" to "abcd"
find characters "b" in field "tf"
TestAssert "test", the foundField is "field 1" or the foundField is "field 2"

find characters "x" in field "tf"
TestAssert "test", the foundField is empty

end TestInterface19
on TestInterface20
create field "tf"

set the text of field "tf" to "abcd" & return & "efgh"
--find "efgh" in field "tf"

TestAssertBroken "test", false /*the foundLine is "line 2 of field 1" or the foundLine is "line 2 of field 2" */, "16178"

--find characters "x" in field "tf"
TestAssertBroken "test", false /*the foundLine is empty*/, "16178"

end TestInterface20
on TestInterface21

create stack "Test"
set the defaultstack to "Test"
create field "tf"

set the text of field "tf" to "abcd"
select char 2 to 2 of field "tf"
put the selectedloc of field "tf" into tCharLoc

find characters "b" in field "tf"
TestAssert "test", the foundLoc is tCharLoc

find characters "x" in field "tf"
TestAssert "test", the foundLoc is empty

end TestInterface21
on TestInterface22

create stack "Test"
set the defaultstack to "Test"
create field "tf"

set the text of field "tf" to "abc def"
find "a" in field "tf"
TestAssert "test", the foundText is "abc"

find "x" in field "tf"
TestAssert "test", the foundText is empty

end TestInterface22
on TestInterface23

TestAssert "test", globalloc((0,0)) is (the topleft of this stack)



end TestInterface23
on TestInterface24

	create image
	create image
	
	//TestAssert "test", failure intersect("not an image", image 2)

end TestInterface24
on TestInterface25


local tAlpha, tBlackImage

repeat with i = 1 to 100
	put numToByte(0) after tAlpha[0]
	put numToByte(127) after tAlpha[1]
	put numToByte(255) after tAlpha[2]
	
	put numToByte(0)&numToByte(0)&numToByte(0)&numToByte(0) after tBlackImage
end repeat


local tImage1, tImage2
create image
put the long id of the last image into tImage1
create image
put the long id of the last image into tImage2

set the rect of tImage1 to 0,0,10,10
set the rect of tImage2 to 0,0,10,10

repeat with i = 0 to 2
	set the alphadata of tImage1 to tAlpha[i]
	repeat with j = i to 2
		set the alphadata of tImage2 to tAlpha[j]
		TestAssert "test", intersect(tImage1, tImage2)
	end repeat
end repeat


set the topleft of tImage2 to the topright of tImage1
TestAssert "test", not intersect(tImage1, tImage2)

end TestInterface25
on TestInterface26

	create image
	create image
	
	//TestAssert "test", failure intersect("not an image", image 2, 0)
	//TestAssert "test", failure intersect(image 1, image 2, "invalid threshold")

end TestInterface26
local tImage1, tImage2
create image
put the long id of the last image into tImage1
create image
put the long id of the last image into tImage2

set the rect of tImage1 to 0,0,10,10
set the rect of tImage2 to 0,0,10,10

set the imagedata of tImage1 to tBlackImage
set the imagedata of tImage2 to tBlackImage


set the alphadata of tImage1 to tZeroAlpha
set the alphadata of tImage2 to tZeroAlpha
TestAssert "test", intersect(tImage1, tImage2, "bounds")
TestAssert "test", not intersect(tImage1, tImage2, "pixels")
TestAssert "test", not intersect(tImage1, tImage2, "opaque pixels")

set the alphadata of tImage2 to tHalfAlpha
TestAssert "test", intersect(tImage1, tImage2, "bounds")
TestAssert "test", not intersect(tImage1, tImage2, "pixels")
TestAssert "test", not intersect(tImage1, tImage2, "opaque pixels")

set the alphadata of tImage2 to tFullAlpha
TestAssert "test", intersect(tImage1, tImage2, "bounds")
TestAssert "test", not intersect(tImage1, tImage2, "pixels")
TestAssert "test", not intersect(tImage1, tImage2, "opaque pixels")

set the alphadata of tImage1 to tHalfAlpha
set the alphadata of tImage2 to tHalfAlpha
TestAssert "test", intersect(tImage1, tImage2, "bounds")
TestAssert "test", intersect(tImage1, tImage2, "pixels")
TestAssert "test", not intersect(tImage1, tImage2, "opaque pixels")

set the alphadata of tImage2 to tFullAlpha
TestAssert "test", intersect(tImage1, tImage2, "bounds")
TestAssert "test", intersect(tImage1, tImage2, "pixels")
TestAssert "test", not intersect(tImage1, tImage2, "opaque pixels")

set the alphadata of tImage1 to tFullAlpha
set the alphadata of tImage2 to tFullAlpha
TestAssert "test", intersect(tImage1, tImage2, "bounds")
TestAssert "test", intersect(tImage1, tImage2, "pixels")
TestAssert "test", intersect(tImage1, tImage2, "opaque pixels")


set the alphadata of tImage1 to tHalfAlpha
set the alphadata of tImage2 to tHalfAlpha
TestAssert "test", intersect(tImage1, tImage2, 127)
TestAssert "test", not intersect(tImage1, tImage2, 128)


set the topleft of tImage2 to the topright of tImage1
TestAssert "test", not intersect(tImage1, tImage2, "bounds")
#
on TestInterface28

TestAssert "test", localloc(the topleft of this stack) is (0,0)

//TestAssert "test", failure localloc("not a point")

end TestInterface28
on TestInterface29

TestAssert "test", the mainstack of this stack is among the lines of the mainstacks

end TestInterface29
on TestInterface30

TestAssert "test", the mouse is "up"

end TestInterface30
on TestInterface37

TestAssert "test", the movingControls is empty
create button
move button 1 relative 50,50 in 1 second without waiting
TestAssert "test", the long id of button 1 is among the lines of the movingControls
wait 1 second

end TestInterface37
on TestInterface38

open this stack
TestAssert "test", the short name of this stack is among the lines of the openStacks

end TestInterface38
on TestInterface39

TestAssert "test", the optionKey is "up"

end TestInterface39
on TestInterface41


TestAssert "test", the screencolors is the exp2 of the screendepth

end TestInterface41
on TestInterface42


TestAssert "test", the screendepth is an integer

end TestInterface42
on TestInterface43


local tRect, tLoc
put the screenrect into tRect
put the screenloc into tLoc

TestAssert "test", the number of items in tLoc is 2

TestAssert "test", item 1 of tLoc is ((item 1 of tRect + item 3 of tRect) div 2)
TestAssert "test", item 2 of tLoc is ((item 2 of tRect + item 4 of tRect) div 2)

end TestInterface43

on TestInterface45


TestAssert "test", the screenrect is a rect

repeat for each line trect in the screenrects
	TestAssert "test", trect is a rect
end repeat

end TestInterface45
on TestInterface46

create stack "Test"
set the defaultstack to "Test"
create field "tf"

set the text of field "tf" to "abcd"

select char 1 to -1 of field "tf"
TestAssert "test", the selectedChunk is "char 1 to 4 of field 1" or the selectedChunk is "char 1 to 4 of field 2"

select empty
TestAssert "test", the selectedChunk is empty

end TestInterface46
on TestInterface47

create stack "Test"
set the defaultstack to "Test"
create field "tf"

set the text of field "tf" to "abcdef"

select char 1 of field "tf"
TestAssert "test", the selectedField is "field 1" or the selectedField is "field 2"

select empty
TestAssert "test", the selectedField is empty

end TestInterface47
on TestInterface48
create stack "Test"
set the defaultstack to "Test"

TestAssert "test", the selectedimage is empty

create image
choose select tool
drag from the loc of the last image to the bottomright of the last image

TestAssert "test", the selectedimage is "image 1"

end TestInterface48
on TestInterface49

create stack "Test"
set the defaultstack to "Test"
create field "tf"

set the text of field "tf" to "abc" & return & "def"

select char 1 to -1 of field "tf"
TestAssert "test", the selectedLine is "line 1 to 2 of field 1" or the selectedLine is "line 1 to 2 of field 2"

select empty
TestAssert "test", the selectedLine is empty

end TestInterface49
on TestInterface105

create stack "Test"
set the defaultstack to "Test"
create field "tf"

set the text of field "tf" to "abc" & return & "def"

select line 1 of field "tf"
TestAssert "test", the selectedLoc is item 1 to 2 of (the formattedrect of line 1 of field "tf")

select empty
TestAssert "test", the selectedLoc is empty

end TestInterface105

on TestInterface50


select empty

TestAssert "test", the selectedobject is empty


create stack "Test"
set the defaultstack to "Test"
create button
create field

select button 1 and field 1

TestAssert "test", the number of lines in (the selectedobject) is 2
TestAssert "test", the long id of button 1 is among the lines of the selectedobject
TestAssert "test", the long id of field 1 is among the lines of the selectedobject

set the selected of field 1 to false

TestAssert "test", the selectedobject is the long id of button 1

end TestInterface50

on TestInterface51

create stack "Test"
set the defaultstack to "Test"
create field

set the text of the last field to "abc"

select char 1 to -1 of the last field
TestAssert "test", the selectedText is "abc"

select empty
TestAssert "test", the selectedText is empty

end TestInterface51
on TestInterface52

create stack "Test"
set the defaultstack to "Test"
create field "tf"

set the text of field "tf" to "abc" & return & "def" & return & "ghi"
set the listbehavior of field "tf" to true
set the multiplehilites of field "tf" to true
set the noncontiguoushilites of field "tf" to true

set the hilitedlines of field "tf" to 1,3

TestAssert "test", the selectedText of field "tf" is ("abc" & return & "ghi")

set the hilitedLines of field "tf" to empty
TestAssert "test", the selectedText of field "tf" is empty

end TestInterface52
on TestInterface53

TestAssert "test", the shiftKey is "up"

end TestInterface53
on TestInterface55
create stack "Test"
set the defaultstack to "Test"
TestAssert "test", the filename of this stack is among the lines of the stacks

end TestInterface55
on TestInterface56


TestAssert "test", word 1 of the topStack is "stack"

end TestInterface56
on TestInterface57


TestAssert "test", the waitdepth is an integer

global gWaitDepthTestResult
create button
set the script of button 1 to "on waitDepthTest" & cr & \
"global gWaitDepthTestResult" & cr & \
"put the waitdepth into gWaitDepthTestResult" & cr & \
"end waitDepthTest"

send "waitDepthTest" to button 1 in 0 seconds
wait 0 seconds with messages

TestAssert "test", gWaitDepthTestResult is (1 + the waitdepth)

end TestInterface57
on TestInterface59


create graphic
set the style of it to "oval"
set the rect of it to 0,0,200,200
set the opaque of it to true

TestAssert "test", within(graphic 1, (50, 50))
TestAssert "test", within(graphic 1, (150, 50))
TestAssert "test", within(graphic 1, (150, 150))
TestAssert "test", within(graphic 1, (50, 150))

TestAssert "test", not within(graphic 1, (25, 25))
TestAssert "test", not within(graphic 1, (175, 25))
TestAssert "test", not within(graphic 1, (175, 175))
TestAssert "test", not within(graphic 1, (25, 175))

end TestInterface59
on TestInterface60

// create image mask with alternating (checkerboard) opaque / transparent pixels
// and black imagedata

local tMaskdata, tImageData
repeat with y = 1 to 100
	repeat with x = 1 to 100
		if (x + y) mod 2 is 0 then
			put numtobyte(255) after tmaskdata
		else
			put null after tmaskdata
		end if
		put null&null&null&null after tImagedata
	end repeat
end repeat

create image
set the rect of it to 0, 0, 100, 100
set the imagedata of it to tImagedata
set the maskdata of it to tMaskdata

TestAssert "test", within(image 1, (0, 0))
TestAssert "test", not within(image 1, (1, 0))
TestAssert "test", within(image 1, (2, 0))
TestAssert "test", not within(image 1, (2, 1))
TestAssert "test", within(image 1, (3, 1))
TestAssert "test", not within(image 1, (3, 2))

end TestInterface60

on TestInterface64


create button

clone invisible button 1 as "foo"

TestAssert "test", the short name of it is "foo" and the visible of it is false

end TestInterface64
on TestInterface65


global tClosed

put false into tClosed

create stack "testStack"

set the script of stack "testStack" to "global tClosed;on closeStack;put true into tClosed; end closeStack"

set the defaultStack to "testStack"

close the defaultStack

TestAssert "test", tClosed

end TestInterface65
on TestInterface67


create button

copy button 1 to this stack

TestAssert "test", there is button 1 on this stack

end TestInterface67
on TestInterface68


create card "test"

TestAssert "test", the short name of it is "test" and there is a card 2

end TestInterface68
on TestInterface70


create stack "test"

TestAssert "test", there is a stack "test"

end TestInterface70
on TestInterface72


create stack "Test"
set the defaultstack to "Test"
create field

put "fooosfsdgsdglj" into field 1

delete char 4 to -1 of field 1

TestAssert "test", field 1 is "foo"

end TestInterface72
on TestInterface74


create button

disable button 1

TestAssert "test", the disabled of button 1

end TestInterface74
on TestInterface75


create button

set the disabled of button 1 to true

enable button 1

TestAssert "test", not the disabled of button 1

end TestInterface75
on TestInterface76
create stack "Test"
set the defaultstack to "Test"

local tId

put the long id of the current card into tId

create stack "Test"
set the defaultstack to "Test"
create field

put "foo" into field 1

create card

find "foo"

TestAssert "test", the long id of the current card is tId

end TestInterface76

on TestInterface79


create button

hilite button 1

TestAssert "test", the hilite of button 1

end TestInterface79



on TestInterface83

create stack "Test"
set the defaultstack to "Test"

create card

create field

put the long id of the current card into field 1

push card

create card

pop card

TestAssert "test", (the long id of the current card is field 1)

end TestInterface83
on TestInterface84

create stack "Test"
set the defaultstack to "Test"

create card

create card

push card 1

create field

pop card into field 1

TestAssert "test", (the long id of card 1 is field 1)

end TestInterface84

on TestInterface88


set the ID of the templatestack to 2000

reset the templatestack

TestAssert "test", the ID of the templatestack is 1001

end TestInterface88
on TestInterface90


create stack "Test"
set the defaultstack to "Test"
create field

put foo into field 1

select text of field 1 

TestAssert "test", the selection is "foo"

end TestInterface90
on TestInterface91

create stack "Test"
set the defaultstack to "Test"

create button

create button

select button 1 and button 2

select empty

TestAssert "test", not (the selectedObjects contains the long id of button 1 or the selectedObjects contains the long id of button 2)

end TestInterface91
on TestInterface106

create stack "Test"
set the defaultstack to "Test"

create button

create button

select button 1 and button 2

TestAssert "test", the selectedObjects contains the long id of button 1 and the selectedObjects contains the long id of button 2

end TestInterface106
on TestInterface92

create stack "Test"
set the defaultstack to "Test"

create button "SelTest"

set the style of button "SelTest" to "menu"

set the menuMode of button "SelTest" to "Option"

set the text of button "SelTest" to "bar" & CR & "foo"

select line 2 of button "SelTest"

TestAssert "test", the label of button "SelTest" is "foo"

end TestInterface92
on TestInterface93


create stack "Test"
set the defaultstack to "Test"
create field

put "foo bar" into field 1

select word 1 of field 1

TestAssert "test", the selection is "foo"

end TestInterface93
on TestInterface95


local tContainer

put "b,d,f,g,c,h,e,a" into tContainer

sort items of tContainer

TestAssert "test", tContainer is "a,b,c,d,e,f,g,h" 

end TestInterface95
on TestInterface107


create stack "Test"
set the defaultstack to "Test"
create field "tf"

put "b" & CR & "d" & CR & "a" & CR & "c" into field "tf"

sort field "tf"
TestAssert "test", line 1 of field "tf" is "a" and line 2 of field "tf" is "b" and line 3 of field "tf" is "c" and line 4 of field "tf" is "d"

end TestInterface107
on TestInterface99


create button

set the loc of button 1 to 0,0

move button 1 from the loc of button 1 to 10, 10 in 10 without waiting

wait 20

stop moving button 1

wait 8

TestAssert "test", not (the loc of button 1 is 0,0)

end TestInterface99
-- no key options working properly
on TestInterface101


create stack "Test"
set the defaultstack to "Test"
create field "fi"

put bar into field "fi"

set the script of field "fi" to "on keyDown;select me;delete;undo;end keyDown"

type "a"

TestAssert "test", the text of field "fi" is "bar"

end TestInterface101