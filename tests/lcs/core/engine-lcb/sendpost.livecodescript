script "CoreEngineLCBSendAndPost"
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

//////////

on TestSetup
   TestSkipIfNot "lcb"
   
   TestLoadAuxiliaryExtension "_widget_sendpost"
   TestLoadAuxiliaryExtension "_library_sendpost"
end TestSetup

//////////

local sPost
on sendMessage pArg
	if pArg is empty then
		put "handled" into sPost
	else
    	put pArg into sPost
    end if

	put " " & word 1 of the target after sPost
end sendMessage

getProp postData
	return sPost
end postData

function sendFunction pArg
	local tResult
	if pArg is empty then
		put "handled" into tResult
	else
    	put pArg into tResult
    end if
    
	put " " & word 1 of the target after tResult
	return tResult
end sendFunction

command sendCommand pArg
	local tResult
	if pArg is empty then
		put "handled" into tResult
	else
    	put pArg into tResult
    end if
    
	put " " & word 1 of the target after tResult
	return tResult
end sendCommand

private function _TestPostData pMsg, pArg
	local tData
	put pMsg into tData["mode"]
	put "sendMessage" into tData["message"]
	if pArg is not empty then
		put pArg into tData["arg"]
	end if
	return tData
end _TestPostData

private function _TestSendData pMsg, pArg
	local tData
	put pMsg into tData["mode"]
	if pMsg contains "function" then
		put "sendFunction" into tData["message"]
	else
		put "sendCommand" into tData["message"]
	end if
	if pArg is not empty then
		put pArg into tData["arg"]
	end if
	return tData
end _TestSendData

private command _TestWidgetSend pMsg, pArg
	local tData
	put _TestSendData(pMsg, pArg) into tData
	
	local tHasArg
	if pArg is not empty then
		put true into tHasArg
	else
		put false into tHasArg
	end if 
	create widget as "com.livecode.lcs_tests.core.engine_lcb.widget_sendpost"
	set the behavior of it to the long id of me
	set the testData of it to tData
	
	local tResult, tAssertion
	put the testData of it into tResult
	put "widget" && pMsg into tAssertion
	
	if tHasArg then
		TestAssert tAssertion && "with arg", (tResult["result"] is pArg && "widget")
	else
		TestAssert tAssertion, (tResult["result"] is "handled widget")
	end if
end _TestWidgetSend

on TestWidgetSend
	_TestWidgetSend "send"
end TestWidgetSend

on TestWidgetSendFunction
	_TestWidgetSend "send function"
end TestWidgetSendFunction

on TestWidgetSendCommand
	_TestWidgetSend "send command"
end TestWidgetSendCommand

on TestWidgetSendWithArguments
	_TestWidgetSend "send args", "arg"
end TestWidgetSendWithArguments

on TestWidgetSendFunctionWithArguments
	_TestWidgetSend "send function args", "arg"
end TestWidgetSendFunctionWithArguments

on TestWidgetSendCommandWithArguments
	_TestWidgetSend "send command args", "arg"
end TestWidgetSendCommandWithArguments

private command _TestWidgetPost pMsg, pArg
	put empty into sPost
	
	local tData
	put _TestPostData(pMsg, pArg) into tData
	
	local tHasArg
	if pArg is not empty then
		put true into tHasArg
	else
		put false into tHasArg
	end if 
	
	local tWidget
	create widget as "com.livecode.lcs_tests.core.engine_lcb.widget_sendpost"
	put it into tWidget
	set the behavior of tWidget to the long id of me
	set the testData of tWidget to tData
	
	local tResult, tAssertion
	put "widget" && pMsg into tAssertion
	
	wait 0 with messages

	put the postData of tWidget into tResult
	if tHasArg then
		TestAssert tAssertion && "with arg", (tResult is pArg && "widget")
	else
		TestAssert tAssertion, (tResult is "handled widget")
	end if
end _TestWidgetPost

on TestWidgetPost
	_TestWidgetPost "post"
end TestWidgetPost

on TestWidgetPostWithArguments
	_TestWidgetPost "post args", "arg"
end TestWidgetPostWithArguments

private command _TestLibrarySend pMsg, pArg, pObj
	local tData
	put _TestSendData(pMsg, pArg) into tData
	
	local tAssertion, tExpectedResult
	put "library" && pMsg into tAssertion	
	if pArg is not empty then
		put " with arg" after tAssertion
		put pArg into tExpectedResult		
	else
		put "handled" into tExpectedResult		
	end if 
	
	local tResult
	if pObj then
		put " stack" after tExpectedResult
		put Test_SendPostToObj(tData, the name of me) into tResult
	else
		put " card" after tExpectedResult	
		put Test_SendPost(tData) into tResult
	end if

	TestAssert tAssertion, (tResult["result"] is tExpectedResult)
end _TestLibrarySend

on TestLibrarySend
	_TestLibrarySend "send"
end TestLibrarySend

on TestLibrarySendFunction
	_TestLibrarySend "send function"
end TestLibrarySendFunction

on TestLibrarySendCommand
	_TestLibrarySend "send command"
end TestLibrarySendCommand

on TestLibrarySendWithArguments
	_TestLibrarySend "send args", "arg"
end TestLibrarySendWithArguments

on TestLibrarySendFunctionWithArguments
	_TestLibrarySend "send function args", "arg"
end TestLibrarySendFunctionWithArguments

on TestLibrarySendCommandWithArguments
	_TestLibrarySend "send command args", "arg"
end TestLibrarySendCommandWithArguments

private command _TestLibraryPost pMsg, pArg, pObj
	put empty into sPost
	
	local tData
	put _TestPostData(pMsg, pArg) into tData

	local tAssertion, tExpectedResult
	put "library" && pMsg into tAssertion	
	if pArg is not empty then
		put " with arg" after tAssertion
		put pArg into tExpectedResult		
	else
		put "handled" into tExpectedResult		
	end if 
	
	local tResult
	if pObj then
		put " stack" after tExpectedResult
		Test_SendPostToObj tData, the name of me
	else
		put " card" after tExpectedResult	
		Test_SendPost tData
	end if

	wait 0 with messages

	TestAssert tAssertion, (sPost is tExpectedResult)
end _TestLibraryPost

on TestLibraryPost
	_TestLibraryPost "post"
end TestLibraryPost

on TestLibraryPostWithArguments
	_TestLibraryPost "post args", "arg"
end TestLibraryPostWithArguments

on TestLibrarySendToObj
	_TestLibrarySend "send", "", true
end TestLibrarySendToObj

on TestLibrarySendToObjFunction
	_TestLibrarySend "send function", "", true
end TestLibrarySendToObjFunction

on TestLibrarySendToObjCommand
	_TestLibrarySend "send command", "", true
end TestLibrarySendToObjCommand

on TestLibrarySendToObjWithArguments
	_TestLibrarySend "send args", "arg", true
end TestLibrarySendToObjWithArguments

on TestLibrarySendToObjFunctionWithArguments
	_TestLibrarySend "send function args", "arg", true
end TestLibrarySendToObjFunctionWithArguments

on TestLibrarySendToObjCommandWithArguments
	_TestLibrarySend "send command args", "arg", true
end TestLibrarySendToObjCommandWithArguments

on TestLibraryPostToObj
	_TestLibraryPost "post", "", true
end TestLibraryPostToObj

on TestLibraryPostToObjWithArguments
	_TestLibraryPost "post args", "arg", true
end TestLibraryPostToObjWithArguments
