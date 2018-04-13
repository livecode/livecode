script "CoreEngineCall"
/*
Copyright (C) 2018 LiveCode Ltd.

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

on TestCallWithArgs
	local tStack
	create stack 
	put it into tStack
	set the script of tStack to "on setVar pValue; set the cVar of me to pValue; end setVar"
	
	local tVar
	put uuid() into tVar
	call "setVar" of tStack with tVar
	TestAssert "call with args", \
		the cVar of tStack is tVar
end TestCallWithArgs

on TestCallWithArgsInMessage
	local tStack
	create stack 
	put it into tStack
	set the script of tStack to "on setVar pValue; set the cVar of me to pValue; end setVar"
	
	local tVar
	put uuid() into tVar
	call "setVar tVar" of tStack
	TestAssert "call with args in message", \
		the cVar of tStack is tVar
end TestCallWithArgsInMessage

on _TestCallWithArgsInMessageAndExplicit
	local tStack
	create stack 
	put it into tStack
	set the script of tStack to "on setVar pValue; set the cVar of me to pValue; end setVar"
	
	local tVar
	put uuid() into tVar
	call "setVar tVar" of tStack with tVar
end _TestCallWithArgsInMessageAndExplicit

on TestCallWithArgsInMessageAndExplicit
	TestAssertThrow "call with args in message and explicit throws", \
		"_TestCallWithArgsInMessageAndExplicit", the long id of me, \
		"EE_HANDLER_BADSTATEMENT"
end TestCallWithArgsInMessageAndExplicit

on TestCallWithArgsByReference
	local tStack
	create stack 
	put it into tStack
	set the script of tStack to "on setVar @xValue; put uuid() into xValue; end setVar"
	
	local tVar, tCheckVar
	put uuid() into tVar
	put tVar into tCheckVar
	call "setVar" of tStack with tVar
	TestAssert "call with args by reference", \
		tVar is not tCheckVar
end TestCallWithArgsByReference

on TestCallWidget
	TestSkipIfNot "lcb"
   TestLoadAuxiliaryExtension "_widget_call"
   
	create stack "WidgetCall"
   set the defaultStack to "WidgetCall"

   create widget "Test" as "com.livecode.lcs_tests.core.widget_call"
   
	call widget "TestWidget_Foo" of widget "Test"
	TestAssert "call widget", the result is "Bar"
		
	call widget "TestWidget_FooArg" of widget "Test" with "Baz"
	TestAssert "call widget with args", the result is "BarBaz"
	
	local tArg
	put "Baz" into tArg
	call widget "TestWidget_FooOutArg" of widget "Test" with tArg
	TestAssert "call widget with out arg", tArg is "Bar"
	
	TestAssertThrow "call widget with non-existant handler", \
		"_TestCallWithNonExistantHandler", the long id of me, \
		"EE_INVOKE_HANDLERNOTFOUND"
		
	create widget "Test2" as "com.livecode.lcs_tests.core.widget_call_non_existant"
	
	TestAssertThrow "call widget with extension not loaded", \
		"_TestCallWithNonExistantExtension", the long id of me, \
		"EE_INVOKE_EXTENSIONNOTFOUND"
end TestCallWidget

on _TestCallWithNonExistantHandler
	call widget "TestWidget_DoesNotExist" of widget "Test" of stack "WidgetCall"
end _TestCallWithNonExistantHandler

on _TestCallWithNonExistantExtension
	call widget "TestWidget_DoesNotExist" of widget "Test2" of stack "WidgetCall"
end _TestCallWithNonExistantExtension
