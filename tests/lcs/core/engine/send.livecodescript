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
	TestAssertThrow "send throws correct error", __SendError, \ 
		the long id of me, "EE_CHUNK_NOSTACK"
end TestSendError

on TestSendDeleteMe
	local tStack
	create stack
	send "--execute this" & return & "delete me" to it

	TestAssert "execute 'delete me' using send", there is not a stack it
end TestSendDeleteMe