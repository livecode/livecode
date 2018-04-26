script "CoreInterfaceUI"
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
   TestSkipIfNot "ui"
   TestSkipIf "environment", "server"
   if the platform is "HTML5" then
      return "SKIP bug 16544"
   end if
end TestSetup

on TestInterface2
create stack "Test"
set the defaultstack to "Test"
create field "tf"

set the rect of field "tf" to "100,100,200,200"
set the text of field "tf" to "a"
get the formattedRect of char 1 of field "tf"

local tPoint
put (item 1 of it + item 3 of it) div 2 & "," & (item 2 of it + item 4 of it) div 2 into tPoint

click button 3 at tPoint
if TestIsInStandalone() then
   TestAssertBroken "the clickChar is a", the clickChar is "a", "not sure"
else
TestAssert "the clickChar is a", the clickChar is "a"
end if

click at "0,0"
if TestIsInStandalone() then
   TestAssertBroken "the clickChar is empty", the clickChar is empty, "not sure"
else
   TestAssert "the clickChar is empty", the clickChar is empty
end if
end TestInterface2

on TestInterface3
create stack "Test"
set the defaultstack to "Test"
create field

set the rect of the last field to 100,100,200,200
set the text of the last field to "a"
get the formattedRect of char 1 of field 1

click button 3 at (item 1 of it + item 3 of it) div 2, (item 2 of it + item 4 of it) div 2
if TestIsInStandalone() then
   TestAssertBroken "the clickCharChunk is char 1 to 1 of field 1", the clickCharChunk is "char 1 to 1 of field 1", "not sure"
else
   TestAssert "the clickCharChunk is char 1 to 1 of field 1", the clickCharChunk is "char 1 to 1 of field 1"
end if

click at 0,0
if TestIsInStandalone() then
   TestAssertBroken "the clickCharChunk is empty", the clickCharChunk is empty, "not sure"
else
   TestAssert "the clickCharChunk is empty", the clickCharChunk is empty
end if

end TestInterface3

on TestInterface4
create stack "Test"
set the defaultstack to "Test"
create field "tf"

set the rect of field "tf" to 100,100,200,200
set the text of field "tf" to "abcd"
get the formattedRect of char 1 of field "tf"

click button 3 at (item 1 of it + item 3 of it) div 2, (item 2 of it + item 4 of it) div 2
if TestIsInStandalone() then
   TestAssertBroken "the clickChunk is char 1 to 4 of field 1 or the clickChunk is char 1 to 4 of field 2", the clickChunk is "char 1 to 4 of field 1" or the clickChunk is "char 1 to 4 of field 2", "not sure"
else
   TestAssert "the clickChunk is char 1 to 4 of field 1 or the clickChunk is char 1 to 4 of field 2", the clickChunk is "char 1 to 4 of field 1" or the clickChunk is "char 1 to 4 of field 2"
end if

click at 0,0
if TestIsInStandalone() then
   TestAssertBroken "the clickChunk is empty", the clickChunk is empty, "not sure"
else
   TestAssert "the clickChunk is empty", the clickChunk is empty
end if

end TestInterface4

on TestInterface5
create stack "Test"
set the defaultstack to "Test"
create field "tf1"
set the rect of field "tf1" to 100,100,200,200

create stack "Test"
set the defaultstack to "Test"
create field "tf2"
set the rect of field "tf2" to 200,200,300,300

click at the loc of field "tf1"
TestAssert "test", the clickField is "field 1" or the clickField is "field 2"

click at the loc of field "tf2"
TestAssert "test", the clickField is "field 2" or the clickField is "field 3"

click at 0,0
TestAssert "test", the clickField is empty or the clickField is "field 1"

end TestInterface5

on TestInterface6
create stack "Test"
set the defaultstack to "Test"

click at 50,100
TestAssert "test", the clickH is 50

click at 0,0
TestAssert "test", the clickH is 0

end TestInterface6

on TestInterface7
create stack "Test"
set the defaultstack to "Test"
create field "tf"

set the rect of field "tf" to 100,100,200,200
set the text of field "tf" to "abcd"
get the formattedRect of char 1 of field "tf"

click at (item 1 of it + item 3 of it) div 2, (item 2 of it + item 4 of it) div 2
TestAssert "test", the clickLine is "line 1 of field 1" or the clickLine is "line 1 of field 2"

click at 0,0
TestAssert "test", the clickLine is empty or the clickLine is "line 1 of field 1"

end TestInterface7

on TestInterface8

create stack "Test"
open stack "Test"

click at 50,100
TestAssert "test", the clickLoc is "50,100"

click at 0,0
TestAssert "test", the clickLoc is "0,0"

end TestInterface8

on TestInterface9
create stack "Test"
set the defaultstack to "Test"
create field "tf"

set the rect of field "tf" to 100,100,200,200
set the text of field "tf" to "abcdef"
get the formattedRect of word 1 of field "tf"

click at (item 1 of it + item 3 of it) div 2, (item 2 of it + item 4 of it) div 2
if TestIsInStandalone() then
   TestAssertBroken "test", the clickText is "abcdef", "not sure"
else
   TestAssert "test", the clickText is "abcdef"
end if

click at 0,0
TestAssert "test", the clickText is empty

end TestInterface9

on TestInterface10
create stack "Test"
set the defaultstack to "Test"

click at 50,100
TestAssert "test", the clickV is 100

click at 0,0
TestAssert "test", the clickV is 0

end TestInterface10

on TestInterface44


TestAssert "test", the screenname is not empty

if the platform is "win32" then
	TestAssert "test", the screenname is "local win32"
else if the platform is "MacOS" then 
	TestAssert "test", the screenname is "local Mac"  
else if the platform is "Linux" then
	local tScreenMajor, tScreenMinor
	TestAssert "screenname is well-formed X11 display ID", \
			matchText(the screenname, ":[0-9]+(?:\.[0-9]+)?")
end if

end TestInterface44

on TestInterface62
create stack "Test"
set the defaultstack to "Test"

create button

set the loc of button 1 to "0,0"

set the script of button 1 to "on mouseDown;set the cClicked of me to true;end mouseDown"

choose pointer tool

click at "0,0"

TestAssert "test", the selectedObject is button 1

choose browse tool

click at "0,0"

TestAssert "test", (the cClicked of button 1)

end TestInterface62
on TestInterface63
create stack "Test"
set the defaultstack to "Test"

create button

set the loc of button 1 to "0,0"

set the script of button 1 to "on mouseDown;set the resizeable of this stack to false;end mouseDown"

click button 1 at "0,0"

TestAssert "test", (the resizeable of this stack is false)

end TestInterface63

on TestBug11799Part1
 

create stack "Test"
set the defaultstack to "Test"
create field

focus on field 1

-- For some reason focusing doesn't happen until events are processed...
wait 0 seconds with messages

TestAssertBroken "test", the focusedObject is the long id of field 1, "11799"

end TestBug11799Part1
on TestBug11799Part2


create stack "Test"
set the defaultstack to "Test"
create field 1

focus on field 1

-- For some reason focusing doesn't happen until events are processed...
wait 0 seconds with messages

focus on nothing

-- After unfocusing, the card will be focused until events are processed...
-- ... at which point nothing will be.

TestAssertBroken "test", the focusedObject is the long id of this card, "11799"

end TestBug11799Part2

on TestInterface82

create stack "Test"
set the defaultstack to "Test"

create card

create field

push current card

pop card into field 1

TestAssert "test", the text of field 1 is the long id of the current card

end TestInterface82

on TestInterface85

create stack "Test"
set the defaultstack to "Test"

create card
create field 

push current card

pop card into field 1

TestAssert "test", field 1 is the long id of the current card

end TestInterface85
