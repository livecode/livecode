script "CoreEngine"
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

on TestBuidNumber
TestAssert "Build number", buildnumber() is an integer
end TestBuidNumber

on TestCommandNames
TestAssert "CommandNames is not empty", the commandNames is not empty
TestAssert "CommandNames contains 'ask'", "ask" is among the lines of the commandNames
TestAssert "CommandNames contains 'put'", "put" is among the lines of the commandNames
end TestCommandNames


on TestConstantNames
TestAssert "ConstantName is not empty", the constantNames is not empty
TestAssert "'one' is among the constant names", "one" is among the lines of the constantNames
TestAssert "'two' is among the constant names", "two" is among the lines of the constantNames
end TestConstantNames


on TestEnvironment
TestAssert "environment is not empty", the environment is not empty

constant kValidEnvironments = "development,standalone application,helper application,browser,mobile,command line,server"
TestAssert merge("environment is in [[kValidEnvironments]]"), the environment is among the items of kValidEnvironments
end TestEnvironment


on TestBackScripts
local tButton
create button

put the long id of button 1 into tButton
set the script of tButton to "on evalBackScriptsTest" & return & \
"end evalBackScriptsTest" & return

insert the script of tButton into back

TestAssert merge("Script of [[tButton]] is in the backscripts"), tButton is among the lines of the backscripts

remove the script of tButton from back

TestAssert merge("Script of [[tButton]] has been removed from the backscripts"), tButton is not among the lines of the backscripts
end TestBackScripts


on TestFrontScripts
local tButton
create button
put the long id of button 1 into tButton
set the script of tButton to "on evalFrontScriptsTest" & return & \
"end evalFrontScriptsTest" & return

insert the script of tButton into front

TestAssert merge("Script of [[tButton]] is in the frontscripts"), tButton is among the lines of the frontscripts

remove the script of tButton from front

TestAssert merge("Script of [[tButton]] is in the frontscripts"), tButton is not among the lines of the frontscripts
end TestFrontScripts


on TestFunctionNames
TestAssert "functionNames is not empty", the functionNames is not empty
TestAssert "'functionNames' is amongst the functionNames", "functionNames" is among the lines of the functionNames
end TestFunctionNames


on TestGlobals
global gEvalGlobalsTest
put "value" into gEvalGlobalsTest
TestAssert "global variable added in the globals", "gEvalGlobalsTest" is among the items of the globals
end TestGlobals


on TestMachine
   TestSkipIf "platform", "HTML5"
if the platform is "win32" then
TestAssert "Machine is x86", the machine is "x86"
else if the platform is "iphone" then
TestAssert "iPhone machine", the machine is "iPod Touch" or the machine is "iPhone" or the machine is "iPhone Simulator" or the machine is "iPad" or the machine is "iPad Simulator"
else
TestAssert "the machine is not empty", the machine is not empty
end if
end TestMachine


on TestCustomProperty
create field "tf"
set the text of field "tf" to "abcd"
set the script of field "tf" to "on TestMe" & return & \
"	set the cMe of me to me" & return & \
"end TestMe"

send "TestMe" to field "tf"
TestAssert "Field text set as custom prop", the cMe of field "tf" is the text of field "tf"
end TestCustomProperty


on TestOwner
create button
TestAssert "Card owns newly created button", (the long id of the owner of the last button) is (the long id of the current card)

group the last button
TestAssert "Now-grouped button is owned by the group", (the long id of the owner of the last button) is (the long id of the last group)
end TestOwner


on TestPendingMessage
local tMessage
put "pendingMessageTestMessage" into tMessage

TestAssert merge("[[tMessage]] is not in pending messages"), tMessage is not among the items of the pendingmessages

send tMessage to me in 0 seconds

TestAssert merge("[[tMessage]] is now in the pending messages"), tMessage is among the items of the pendingmessages
end TestPendingMessage


on TestPlatform
TestAssert "test", the platform is among the items of "Win32,MacOS,Linux,iphone,android,html5"
end TestPlatform


on TestProcessor
   TestSkipIf "platform", "HTML5"
local tProcessors

if the platform is "Win32" then
put "x86" into tProcessors
else if the platform is "android" then
put "arm,i386" into tProcessors
else if the platform is "iphone" then
put "arm,arm64,i386,x86_64" into tProcessors
else if the platform is among the items of "MacOS,Linux" then
   // Mac and Linux
   put "x86,x86_64" into tProcessors
else
   -- Unknown platform for "the processor"
end if

TestAssert "the processor is correct", the processor is among the items of tProcessors
end TestProcessor


on TestPropertyNames
TestAssert "PropertyNames is not empty", the propertyNames is not empty
TestAssert "'name' is among the propertyNames", "name" is among the lines of the propertyNames
end TestPropertyNames


on TestOpenNonExistantFile
open file "non-existant file" for read
TestAssert "Can't open non-existant file", the result is "can't open that file"
end TestOpenNonExistantFile

on TestScriptLimits
local tLimits
put the scriptlimits into tLimits
TestAssert "Script limits contains 4 items", the number of items in tLimits is 4
repeat for each item tLimit in tLimits
TestAssert "Each item is an integer", tLimit is an integer
end repeat
end TestScriptLimits


on TestSysError
local tError
open file "this file should not exist" for read
put the syserror into tError

TestAssert "sysError is an integer", tError is an integer
TestAssert "sysError is not 0", tError is not 0
end TestSysError


on TestSystemVersion
   if the platform is "HTML5" then
      TestAssert "the systemVersion is empty", the systemVersion is empty
   else
      TestAssert "the systemVersion is not empty", the systemVersion is not empty
   end if
end TestSystemVersion


on TestValue
local tValue
// variable value
put 1 into tValue
TestAssert "Integer value", value("tValue") is 1

// array values
put 1 into tValue["a"]
put 2 into tValue["b"]
TestAssert "Array value", value("tValue") is an array

// arithmetic
TestAssert "Arithmetic value", value("1 + 4") is 5

// string operations
TestAssert "Concatenation value", value("abc && def") is "abc def"

// function call
TestAssert "Function value", value("sum(1, 2, 3)") is 6

// evaluation within the current object
TestAssert "Current object value", value("the long id of me") is the long id of me
end TestValue


on TestValueWithParam
create button
TestAssert "'value' object parameter", value("the long id of me", button 1) is the long id of button 1
end TestValueWithParam


on TestVariableNames
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

TestAssert "Check variableName line count", the number of lines in the cVariableNames of button 1 is 4
TestAssert "Check params", line 1 of the cVariableNames of button 1 is "pParam"

TestAssert "Check handler locals", line 2 of the cVariableNames of button 1 is "it,tLocal"

TestAssert "Check script locals", line 3 of the cVariableNames of button 1 is "sScript"

local tGlobals
put line 4 of the cVariableNames of button 1 into tGlobals

TestAssert "Has 3 global variables", the number of items in tGlobals is 3
TestAssert "Check script global", "gScriptGlobal" is among the items of tGlobals
TestAssert "Checkhandler global", "gHandlerGlobal" is among the items of tGlobals
TestAssert "Check shared global", "gSharedGlobal" is among the items of tGlobals
end TestVariableNames


on TestVersion
local tItem3

set the itemDelimiter to "."
TestAssert "the version has 2 periods", the number of items in the version is 3
TestAssert "First element is a number ", item 1 of the version is an integer
TestAssert "Second element is a number", item 2 of the version is an integer

put item 3 of the version into tItem3
set the itemDelimiter to "-"
TestAssert "Third element is a number", item 1 of tItem3 is an integer

if the number of items of tItem3 > 1 then
testAssert "Non-GM build type is either dp or rc", item 2 of tItem3 is among the items of "dp-rc"
testAssert "Non-GM build type number is a number", item 3 of tItem3 is an integer
else
testSkip
testSkip
end if
end TestVersion


on TestCancelPendingMessage
   TestSkipIfNot "wait"
create button

set the script of button 1 to "on mouseUp;set the ID of the templateStack to 2000;end mouseUp"

send "mouseUp" to button 1 in 10
wait 5

cancel item 1 of last line of the pendingMessages
wait 10

TestAssert "Message has been cancelled", the ID of the templatestack is 1001
end TestCancelPendingMessage


on TestDeleteArrayValue
local tArray

put "foo" into tArray[1]

delete variable tArray[1]

TestAssert "Key value deleted", "1" is not among the keys of tArray
end TestDeleteArrayValue


on TestDeleteChunk
local tString

put "foo,bar,baz" into tString

delete item 2 to 3 of tString

TestAssert "items deleted", tString is "foo"
end TestDeleteChunk


on TestDeleteChunkofChunk
local tString
constant kLine = "foo,bar,baz"

put kLine & LF & kLine & LF & kLine into tString

delete item 2 to -1 of line 2 of tString

testAssert "item of line deleted", tString is (kLine & LF & "foo" & LF & kLine)
end TestDeleteChunkofChunk


on TestDo
do "set the cDidThis of this stack to 100"

TestAssert "Do command", the cDidThis of this stack is 100
end TestDo

local kDoAsMessage_ExecutionError = "execution error"
on TestDoAsJavaScript
    if the platform is not "HTML5" then
		exit TestDoAsJavaScript
	end if
	
	do "1 + '4';" as "javascript"
	TestAssert "Do as JavaScript", the result is "14"
	
	do "undefined;" as "javascript"
	TestAssert "Undefined values return empty", the result is empty
	
	do "function(x) { return x; }" as "javascript"
	TestAssert "Function return value unsupported", the result is kDoAsMessage_ExecutionError
	
	do "{ key1: 'value1', key2: 2 };" as "javascript"
	TestAssert "Object return value unsupported", the result is kDoAsMessage_ExecutionError
	
	do "[ 1, 2, 3, 4 ];" as "javascript"
	TestAssert "Array return value unsupported", the result is kDoAsMessage_ExecutionError
	
	do "6.02e23;" as "javascript"
	TestAssert "Number return value", the result is a number
	
	do "'Hello, World!';" as "javascript"
	TestAssert "String return value", the result is "Hello, World!"
	
end TestDoAsJavaScript

on TestAlternateLanguages
	if the platform is "HTML5" then
		TestAssert "JavaScript in HTML5 alternateLanguages", "javascript" is among the lines of the alternateLanguages
	end if
end TestAlternateLanguages

on TestFrontScriptBackScriptCall
create button

set the script of button 1 to "function foo;return true;end foo"

insert the script of button 1 into front

create button

set the script of button 2 to "function bar;return true;end bar"

insert the script of button 2 into back

TestAssert "Functions added to front and back scripts", (foo() and bar())
end TestFrontScriptBackScriptCall


on TestFrontScriptExecution
set the failed of this stack to false

create button
set the script of button 1 to "command foo;set the failed of this stack to false;end foo"

create button
set the script of button 2 to "command foo;set the failed of this stack to true;end foo"


insert the script of button 1 into back
insert the script of button 1 into front

insert the script of button 2 into back
insert the script of button 2 into front

foo
TestAssert "second implementation of 'foo' is executed", (the failed of this stack)

remove the script of button 2 from back

remove the script of button 2 from front

foo
TestAssert "First implementation of 'foo' is executed", (not the failed of this stack)
end TestFrontScriptExecution


on TestLibraryStackMessage
create stack "Test1"

set the script of stack "Test1" to "on libraryStack;global gTestGlobalVar;put true into gTestGlobalVar; end libraryStack"

global gTestGlobalVar

start using stack "Test1"

TestAssert "Global var created by libraryStack message", gTestGlobalVar

stop using stack "Test1"

TestAssert "global var not relased after stopping using library", gTestGlobalVar

close stack "Test1"
end TestLibraryStackMessage


on TestReleaseStackMessage
create stack "Test1"

set the script of stack "Test1" to "on releaseStack;global gTestGlobalVarRelease;put true into gTestGlobalVarRelease; end releaseStack"

global gTestGlobalVarRelease

start using stack "Test1"

stop using stack "Test1"

TestAssert "Global variable set by releaseStack message", gTestGlobalVarRelease

close stack "Test1"
end TestReleaseStackMessage


on TestMilliseconds
   TestSkipIfNot "wait"

local tTime

testAssert "Milliseconds is an integer", the milliseconds is an integer
put the millisecs into tTime

wait 2 millisecs

TestAssert "the milliseconds change with time", the millisecs - tTime >= 2
end TestMilliseconds


on TestSeconds
   TestSkipIfNot "wait"

local tTime

testAssert "the seconds is an integer", the seconds is an integer
put the secs into tTime

wait 2 secs

TestAssert "the seconds change with time", the secs - tTime >= 2
end TestSeconds


on TestTicks
   TestSkipIfNot "wait"

local tTime

testAssert "the ticks is an integer", the ticks is an integer
put the ticks into tTime

wait 2 ticks

TestAssert "the ticks change with time", the ticks - tTime >= 2
end TestTicks

on TestEffectiveRectIsUpdatedWhileScreenIsLocked
   local tRectBefore, tRectAfter, tEffectiveRectBefore, tEffectiveRectAfter
   
   local tLeft,tTop
   put 100 into tLeft
   put 200 into tTop
   
   create stack "TestStack"
   put the rect of stack "TestStack" into tRectBefore
   lock screen
   set the top of stack "TestStack" to the top of stack "TestStack" + tTop
   set the left of stack "TestStack" to the left of stack "TestStack" + tLeft
   put the effective rect of stack "TestStack" into tEffectiveRectBefore
   put the rect of stack "TestStack" into tRectAfter
   unlock screen
   put the effective rect of stack "TestStack" into tEffectiveRectAfter
   add tTop to item 2 of tRectBefore
   add tTop to item 4 of tRectBefore
   add tLeft to item 1 of tRectBefore
   add tLeft to item 3 of tRectBefore
   TestAssert "rect of stack is updated while screen is locked", tRectBefore is tRectAfter
   TestAssert "effective rect of stack is updated while screen is locked", tEffectiveRectBefore is tEffectiveRectAfter
   delete stack "TestStack" 
end TestEffectiveRectIsUpdatedWhileScreenIsLocked

on TestUngroupAndQuit
    -- Bug 21500: run a subprocess in which we ungroup a group and
    -- then quit, to test that it no longer crashes
   local tStackToRun, tOptions
   put the effective filename of me into tStackToRun
   set the itemdelimiter to slash
   if the environment is not "server" then
      put "_ungroupandquit.livecodescript" into item -1 of tStackToRun
      if the environment contains "command line" then
         put "-ui" into tOptions
      end if
   end if
   TestRunStack tOptions, tStackToRun
   TestAssert "ungroup does not crash on quit", the result is empty
end TestUngroupAndQuit

