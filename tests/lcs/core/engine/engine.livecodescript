script "CoreEngine"
/*
Copyright (C) 2015 Runtime Revolution Ltd.

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

on TestEngine1
create button
put the long id of button 1 into tButton
set the script of tButton to "on evalBackScriptsTest" & return & \
		"end evalBackScriptsTest" & return

 
insert the script of tButton into back

TestAssert "test", tButton is among the lines of the backscripts

remove the script of tButton from back

TestAssert "test", tButton is not among the lines of the backscripts

end TestEngine1

on TestEngine2

TestAssert "test", buildnumber() is an integer
end TestEngine2

on TestEngine3

TestAssert "test", the commandNames is not empty
TestAssert "test", "ask" is among the lines of the commandNames
TestAssert "test", "put" is among the lines of the commandNames
end TestEngine3

on TestEngine4

TestAssert "test", the constantNames is not empty
TestAssert "test", "one" is among the lines of the constantNames
TestAssert "test", "two" is among the lines of the constantNames
end TestEngine4

on TestEngine5

TestAssert "test", the environment is not empty
TestAssert "test", the environment is among the items of "development,standalone application,helper application,browser,mobile,command line,server"
end TestEngine5

on TestEngine6

create button
put the long id of button 1 into tButton
set the script of tButton to "on evalFrontScriptsTest" & return & \
		"end evalFrontScriptsTest" & return

TestAssert "test", tButton is not among the lines of the frontscripts

insert the script of tButton into front

TestAssert "test", tButton is among the lines of the frontscripts

remove the script of tButton from front

TestAssert "test", tButton is not among the lines of the frontscripts

end TestEngine6

on TestEngine7

TestAssert "test", the functionNames is not empty
TestAssert "test", "functionNames" is among the lines of the functionNames
end TestEngine7

on TestEngine8

global gEvalGlobalsTest
put "value" into gEvalGlobalsTest
TestAssert "test", "gEvalGlobalsTest" is among the items of the globals
end TestEngine8


on TestEngine9

local tScript
put "local sScript" & return & \
	"on TestMe pParam" & return & \
	"	global gVariableNamesGlobal" & return & \
	"	local tLocal" & return & \
	"	set the cVariableNames of me to the variableNames" & return & \
	"end TestMe" into tScript

create button
set the script of button 1 to tScript
local tParam = "abcd"
send "TestMe tParam" to button 1

TestAssert "test", the number of lines in the cVariableNames of button 1 is 4
TestAssert "test", line 1 of the cVariableNames of button 1 is "pParam"
if "textDecode" is among the lines of the functionNames then
   TestAssert "test", line 2 of the cVariableNames of button 1 is "it,tLocal"
else
   TestAssert "test", line 2 of the cVariableNames of button 1 is "tLocal"
end if 
TestAssert "test", line 3 of the cVariableNames of button 1 is "sScript"
TestAssert "test", line 4 of the cVariableNames of button 1 is "gVariableNamesGlobal"

end TestEngine9

on TestEngine10

if the platform is "win32" then
  TestAssert "test", the machine is "x86"
else if the platform is "iphone" then
  TestAssert "test", the machine is "iPod Touch" or the machine is "iPhone" or the machine is "iPhone Simulator" or the machine is "iPad" or the machine is "iPad Simulator"
else if the platform is "Android" then
  TestAssert "test", true
else
   // need to create Tests for other platforms
  TestAssert "test", true
end if
end TestEngine10

on TestEngine11


create field "tf"
set the text of field "tf" to "abcd"
set the script of field "tf" to "on TestMe" & return & \
	"	set the cMe of me to me" & return & \
	"end TestMe"

send "TestMe" to field "tf"
TestAssert "test", the cMe of field "tf" is the text of field "tf"
end TestEngine11

on TestEngine12

create button
TestAssert "test", (the long id of the owner of the last button) is (the long id of the current card)

group the last button
TestAssert "test", (the long id of the owner of the last button) is (the long id of the last group)

end TestEngine12

on TestEngine13


local tMessage
put "pendingMessageTestMessage" into tMessage

TestAssert "test", tMessage is not among the items of the pendingmessages

send tMessage to me in 0 seconds

TestAssert "test", tMessage is among the items of the pendingmessages

end TestEngine13

on TestEngine14


TestAssert "test", the platform is not empty
TestAssert "test", the platform is among the items of "Win32,MacOS,Linux,iphone,android,html5"

end TestEngine14
on TestEngine15


TestAssert "test", the processor is not empty
TestAssertBroken "test", the processor is among the items of "unknown,x86,motorola powerpc,i386,arm", "use uname or similar to check"

end TestEngine15
on TestEngine16

TestAssert "test", the propertyNames is not empty
TestAssert "test", "name" is among the lines of the propertyNames
end TestEngine16

on TestEngine17


open file "non-existant file" for read
TestAssert "test", the result is "can't open that file"

end TestEngine17

on TestEngine18


local tLimits
put the scriptlimits into tLimits
TestAssert "test", the number of items in tLimits is 4
repeat for each item tLimit in tLimits
  TestAssert "test", tLimit is an integer
end repeat

end TestEngine18
on TestEngine19


local tError
open file "this file should not exist" for read
put the syserror into tError

TestAssert "test", tError is an integer
TestAssert "test", tError is not 0

end TestEngine19

on TestEngine20

TestAssert "test", the systemVersion is not empty

end TestEngine20
on TestEngine21


local tVal
// variable value
put 1 into tValue
TestAssert "test", value("tValue") is 1

// array values
put 1 into tValue["a"]
put 2 into tValue["b"]
TestAssert "test", value("tValue") is an array

// arithmetic
TestAssert "test", value("1 + 4") is 5
// string operations
TestAssert "test", value("abc && def") is "abc def"
// function call
TestAssert "test", value("sum(1, 2, 3)") is 6

// evaluation within the current object
TestAssert "test", value("the long id of me") is the long id of me

end TestEngine21

on TestEngine22

	create button
  TestAssert "test", value("the long id of me", button 1) is the long id of button 1

end TestEngine22

on TestEngine23


local tScript
put "local sScript" & return & \
	"global gScriptGlobal" & return & \
	"global gSharedGlobal" & return & \
	"on TestMe pParam" & return & \
	"	local tLocal" & return & \
	"	global gHandlerGlobal" & return & \
	"	global gSharedGlobal" & return & \
	"	set the cVariableNames of me to the variableNames" & return & \
	"end TestMe" into tScript

create button
set the script of button 1 to tScript
local tParam = "abcd"
send "TestMe tParam" to button 1

TestAssert "test", the number of lines in the cVariableNames of button 1 is 4
TestAssert "test", line 1 of the cVariableNames of button 1 is "pParam"
if "textDecode" is among the lines of the functionNames then
TestAssert "test", line 2 of the cVariableNames of button 1 is "it,tLocal"
else
TestAssert "test", line 2 of the cVariableNames of button 1 is "tLocal"
end if 
TestAssert "test", line 3 of the cVariableNames of button 1 is "sScript"

local tGlobals
put line 4 of the cVariableNames of button 1 into tGlobals

TestAssert "test", "gScriptGlobal" is among the items of tGlobals
TestAssert "test", "gHandlerGlobal" is among the items of tGlobals
TestAssert "test", "gSharedGlobal" is among the items of tGlobals

delete item itemOffset("gSharedGlobal", tGlobals) of tGlobals
TestAssert "test", "gSharedGlobal" is not among the items of tGlobals

end TestEngine23

on TestEngine24

local tItem3

set the itemDelimiter to "."
TestAssert "test", the number of items in the version is 3
TestAssert "test", item 1 of the version is an integer
TestAssert "test", item 2 of the version is an integer
put item 3 of the version into tItem3
set the itemDelimiter to "-"
TestAssert "test", item 1 of tItem3 is an integer

end TestEngine24

on TestEngine25


create button

set the script of button 1 to "on mouseUp;set the ID of the templaTestack to 2000;end mouseUp"

send "mouseUp" to button 1 in 10

wait 5

cancel item 1 of last line of the pendingMessages

wait 10

TestAssert "test", the ID of the templatestack is 1001

end TestEngine25
on TestEngine26


local tArray

put "foo" into tArray[1]

delete variable tArray[1]

TestAssert "test", 1 is not among the keys of tArray

end TestEngine26

on TestEngine27


local tVar

put "foo,bar,baz" into tVar

delete item 2 to 3 of tVar

TestAssert "test", tVar is "foo"

end TestEngine27

on TestEngine28


do "set the cDidThis of this stack to 100"

TestAssert "test", the cDidThis of this stack is 100

end TestEngine28

on TestEngine29


create button

set the script of button 1 to "function foo;return true;end foo"

insert the script of button 1 into front

create button 

set the script of button 2 to "function bar;return true;end bar"

insert the script of button 2 into back

TestAssert "test", (foo() and bar())

end TestEngine29
on TestEngine30


create field

set the script of this card to "on shutdownRequest;set the resizeable of this stack to false; end shutdownRequest"

quit

TestAssert "test", the resizeable of this stack is false

end TestEngine30
on TestEngine31


set the failed of this stack to false

create button

set the script of button 1 to "command foo;set the failed of this stack to false;end foo"

create button

set the script of button 2 to "command foo;set the failed of this stack to true;end foo"

insert the script of button 1 into back

insert the script of button 1 into front 
 
insert the script of button 2 into back

insert the script of button 2 into back

foo

remove the script of button 2 from back

remove the script of button 2 from front

foo

TestAssert "test", (not the failed of this stack) 

end TestEngine31


on TestEngine33


create stack "Test1"

set the script of stack "Test1" to "on libraryStack;global tVar;put true into tVar; end libraryStack"

global tVar

start using stack "Test1"

TestAssert "test", tVar

close stack "Test1"

end TestEngine33

on TestEngine35


create stack "Test1"

set the script of stack "Test1" to "on releaseStack;global tVar;put true into tVar; end releaseStack"

global tVar

start using stack "Test1"

stop using stack "Test1"

TestAssert "test", tVar

close stack "Test1"

end TestEngine35

on TestEngine36


local tTime

put the millisecs into tTime

wait 2 millisecs

TestAssert "test", the millisecs - tTime >= 2

end TestEngine36
