script "CoreEngineSend"
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

on TestSendParams
	local tScript, tStack
	put "on testParameters p1, p2; return p2; end testParameters" \
		into tScript
	create stack
	put it into tStack
	set the script of tStack to tScript
	
	local tSendScript
	put "--ensure this is executed as script not a handler name" \
		into tSendScript
	put return & "testParameters 1, 2" after tSendScript
	
	send tSendScript to tStack
	TestAssert "send script with multiple params", the result is "2"
end TestSendParams

/*
It is known that there is code in the wild relying on the following 
(undocumented) form of the send command, so we add a test here to ensure 
backwards compatibility until such a point as it is deemed possible or 
necessary to break.
*/
on TestSendLegacy
	local tStack
	create stack
	put it into tStack
	set the defaultStack to the short name of tStack
	
	local tButton
	create button
	put it into tButton
	
	local tSendScript
	put "delete tButton" into tSendScript
	
	send tSendScript to tStack
	TestAssert "send script as chunk of script legacy", \
		there is not a button 1 of tStack
end TestSendLegacy

private function MessageExists pId
	repeat for each line tLine in the pendingMessages
		if item 1 of tLine is pId then
			return true
		end if
	end repeat
	return false
end MessageExists


on TestSendToSubobj
	local tStack, tMsgId
	create stack
	put the short name of it into tStack
	set the defaultStack to tStack
	create button
	send "mouseUp" to it in 5 millisecs
	put the result into tMsgId
	delete stack tStack

	TestAssert "message canceled on delete", not MessageExists(tMsgId)
end TestSendToSubobj

on __SendError
	send "--execute this" & return & "put the name of stack nonexistent"
end __SendError

on TestSendError
	TestAssertThrow "send throws correct error", "__SendError", \ 
		the long id of me, "EE_CHUNK_NOSTACK"
end TestSendError

on TestSendDeleteMe
	local tStack
	create stack
	send "--execute this" & return & "delete me" to it

	TestAssert "execute 'delete me' using send", there is not a stack it
end TestSendDeleteMe

on TestSendScript
	local tStack
	create stack
	put it into tStack
	set the defaultStack to the short name of tStack
	create button
	
	local tSendScript
	put "delete button 1 of me" into tSendScript
	
	send script tSendScript to tStack
	TestAssert "send script to object", \
		there is not a button 1 of tStack
end TestSendScript

on TestSendScriptEvaluation
	local tVar
	put "Something" into tVar
	
	local tStack
	create stack 
	put it into tStack
	set the script of tStack to "on setVar pValue; set the cVar of me to pValue; end setVar"
	-- tVar should be treated as UQL in target context
	send script "setVar tVar" to tStack
	TestAssert "send script param evaluated in target context", \
		the cVar of tStack is "tVar"
		
	-- tVar should be evaluated in the current context
	send "setVar tVar" to tStack
	TestAssert "send param evaluated in current context", \
		the cVar of tStack is "Something"
end TestSendScriptEvaluation

on TestSendWithArgs
	local tStack
	create stack 
	put it into tStack
	set the script of tStack to "on setVar pValue; set the cVar of me to pValue; end setVar"
	
	local tVar
	put uuid() into tVar
	send "setVar" to tStack with tVar
	TestAssert "send with args", \
		the cVar of tStack is tVar
end TestSendWithArgs

on TestSendInTimeWithArgs
	TestSkipIfNot "wait"
	
	local tStack
	create stack 
	put it into tStack
	set the script of tStack to "on setVar pValue; set the cVar of me to pValue; end setVar"
	
	local tVar
	put uuid() into tVar
	send "setVar" to tStack in 0 with tVar
	wait 0 milliseconds with messages
	TestAssert "send in time with args", \
		the cVar of tStack is tVar
end TestSendInTimeWithArgs

on _TestSendWithArgsInMessageAndExplicit
	local tStack
	create stack 
	put it into tStack
	set the script of tStack to "on setVar pValue; set the cVar of me to pValue; end setVar"
	
	local tVar
	put uuid() into tVar
	send "setVar tVar" to tStack with tVar
end _TestSendWithArgsInMessageAndExplicit

on TestSendWithArgsInMessageAndExplicit
	TestAssertThrow "send with args in message and explicit throws", \
		"_TestSendWithArgsInMessageAndExplicit", the long id of me, \
		"EE_HANDLER_BADSTATEMENT"
end TestSendWithArgsInMessageAndExplicit

on TestSendWithArgsByReference
	local tStack
	create stack 
	put it into tStack
	set the script of tStack to "on setVar @xValue; put uuid() into xValue; end setVar"
	
	local tVar, tCheckVar
	put uuid() into tVar
	put tVar into tCheckVar
	send "setVar" to tStack with tVar
	TestAssert "send with args by reference", \
			tVar is not tCheckVar
end TestSendWithArgsByReference

on TestSendWidget
	TestSkipIfNot "lcb"
   TestLoadAuxiliaryExtension "_widget_call"
   
	create stack "WidgetCall"
   set the defaultStack to "WidgetCall"

   create widget "Test" as "com.livecode.lcs_tests.core.widget_call"
   
	send widget "TestWidget_Foo" to widget "Test"
	TestAssert "send widget", the result is "Bar"
		
	send widget "TestWidget_FooArg" to widget "Test" with "Baz"
	TestAssert "send widget with args", the result is "BarBaz"
	
	local tArg
	put "Baz" into tArg
	send widget "TestWidget_FooOutArg" to widget "Test" with tArg
	TestAssert "send widget with out arg", tArg is "Bar"
	
	TestAssertThrow "send widget with non-existant handler", \
		"_TestSendWithNonExistantHandler", the long id of me, \
		"EE_INVOKE_HANDLERNOTFOUND"
		
	create widget "Test2" as "com.livecode.lcs_tests.core.widget_call_non_existant"
	
	TestAssertThrow "send widget with extension not loaded", \
		"_TestSendWithNonExistantExtension", the long id of me, \
		"EE_INVOKE_EXTENSIONNOTFOUND"
		
	set the defaultStack to the short name of me
	send widget "TestWidget_DefaultStack" to widget "Test" of stack "WidgetCall"
	TestAssert "send widget changes defaultstack", the result is the name of stack "WidgetCall"
end TestSendWidget

on _TestSendWithNonExistantHandler
	send widget "TestWidget_DoesNotExist" to widget "Test" of stack "WidgetCall"
end _TestSendWithNonExistantHandler

on _TestSendWithNonExistantExtension
	send widget "TestWidget_DoesNotExist" to widget "Test2" of stack "WidgetCall"
end _TestSendWithNonExistantExtension

on TestSendWidgetInTimeWithArgs
	TestSkipIfNot "wait"
	TestSkipIfNot "lcb"
   TestLoadAuxiliaryExtension "_widget_call"
   
	create stack "WidgetCall"
   set the defaultStack to "WidgetCall"

   create widget "Test" as "com.livecode.lcs_tests.core.widget_call"
   
	local tVar
	put uuid() into tVar
	send widget "TestWidget_SetProp" to widget "Test" with "uUUID", tVar
	TestAssert "send widget in time with args", \
			the uUUID of widget "Test" is tVar
	
	put uuid() into tVar
	send widget "TestWidget_SetProp" to widget "Test" in 0 with "uUUID", tVar
	wait 50 milliseconds with messages
	TestAssert "send widget in time with args", \
			the uUUID of widget "Test" is tVar
end TestSendWidgetInTimeWithArgs
