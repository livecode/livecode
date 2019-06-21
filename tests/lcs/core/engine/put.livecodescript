script "CoreEnginePut"
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


on _TestPutBeforeInvalidContainer pContainer
	local tDo
	put "put" && quote & foo & quote && "before" && pContainer into tDo
	do tDo
end _TestPutBeforeInvalidContainer

on _TestPutIntoInvalidContainer pContainer
    local tDo
	put "put" && quote & foo & quote && "into" && pContainer into tDo
	do tDo
end _TestPutIntoInvalidContainer

on _TestPutAfterInvalidContainer pContainer
    local tDo
	put "put" && quote & foo & quote && "after" && pContainer into tDo
	do tDo
end _TestPutAfterInvalidContainer

on TestPutBeforeIntoAfterInvalidContainer

	local tCardID, tStackID

	create stack "myTestStack"
	put it into tStackID
	
	set the defaultstack to tStackID
	put the long ID of this card into tCardID
	
	
	// card
	TestAssertThrow "put before invalid container" && tCardID, "_TestPutBeforeInvalidContainer", the long id of me, \
			"EE_CHUNK_SETNOTACONTAINER", tCardID
	TestAssertThrow "put into invalid container" , "_TestPutIntoInvalidContainer", the long id of me, \
			"EE_CHUNK_SETNOTACONTAINER", tCardID
	TestAssertThrow "put after invalid container" , "_TestPutAfterInvalidContainer", the long id of me, \
			"EE_CHUNK_SETNOTACONTAINER", tCardID
			
	// stack 
	TestAssertThrow "put before invalid container" && tStackID , "_TestPutBeforeInvalidContainer", the long id of me, \
			"EE_CHUNK_SETNOTACONTAINER", tStackID
	TestAssertThrow "put into invalid container" , "_TestPutIntoInvalidContainer", the long id of me, \
			"EE_CHUNK_SETNOTACONTAINER", tStackID
	TestAssertThrow "put after invalid container" , "_TestPutAfterInvalidContainer", the long id of me, \
			"EE_CHUNK_SETNOTACONTAINER", tStackID
			
	// widget
	local tWidgetID
	create widget "myClock" as "com.livecode.widget.clock"
	put it into tWidgetID
	TestAssertThrow "put before invalid container" && tWidgetID, "_TestPutBeforeInvalidContainer", the long id of me, \
			"EE_CHUNK_SETNOTACONTAINER", tWidgetID
	TestAssertThrow "put into invalid container" , "_TestPutIntoInvalidContainer", the long id of me, \
			"EE_CHUNK_SETNOTACONTAINER", tWidgetID
	TestAssertThrow "put after invalid container" , "_TestPutAfterInvalidContainer", the long id of me, \
			"EE_CHUNK_SETNOTACONTAINER", tWidgetID


	repeat for each item tContainer in "scrollbar,group,player,graphic,eps"
		local tID
		local tDo
      	put "create" && tContainer && "myContainer" into tDo
      	do tDo
      	put it into tID
		TestAssertThrow "put before invalid container:" && tContainer, "_TestPutBeforeInvalidContainer", the long id of me, \
			"EE_CHUNK_SETNOTACONTAINER", tID
		TestAssertThrow "put into invalid container" && tContainer, "_TestPutIntoInvalidContainer", the long id of me, \
			"EE_CHUNK_SETNOTACONTAINER", tID
		TestAssertThrow "put after invalid container" && tContainer, "_TestPutAfterInvalidContainer", the long id of me, \
			"EE_CHUNK_SETNOTACONTAINER", tID
		delete tID
	end repeat
	
	delete stack "myTestStack"
end TestPutBeforeIntoAfterInvalidContainer

on TestPutBeforeIntoAfterValidContainer

	local tStackID

	create stack "myTestStack"
	put it into tStackID
	
	set the defaultstack to tStackID

	// button case
    create button "myNewButton"
    put "bar" into button "myNewButton"
    put "foo" before button "myNewButton"
    put "baz" after button "myNewButton"
    TestAssert "Can put before|into|after a button", the text of button "myNewButton" is "foobarbaz"
    delete button "myNewButton"
    
    
	// image case
    repeat for each item tWhere in "before,into,after"
      create image "foo"
      local tDo
      put "put empty" && tWhere && "image" && quote & "foo" & quote into tDo
      do tDo
      TestAssert "Can put" && tWhere && "an image", the text of image "foo" is empty
      delete image "foo"
   end repeat
   
   
end TestPutBeforeIntoAfterValidContainer


// Test MCVariable::modify variant
on TestPutAfterData
	local tVar
	put numToByte(1) into tVar
	put numToByte(1) after tVar
	TestAssert "put after preserves binary string", \
		tVar is strictly a binary string
end TestPutAfterData

// Test MCEngineExecPutIntoVariable ValueRef chunk variant
on TestPutAfterDataChunk
	local tVar, tArray
	put numToByte(1) into tVar
	put numToByte(1) after byte 1 of tVar
	TestAssert "put after chunk preserves binary string", \
		tVar is strictly a binary string
end TestPutAfterDataChunk

// Test MCVariable::modify_ctxt variant
on TestPutAfterDataExecValue
	local tVar, tArray
	put numToByte(1) into tVar
	put numToByte(1) into tArray[1]
	put tArray[1] after tVar
	TestAssert "put as execvalue after preserves binary string", \
		tVar is strictly a binary string
end TestPutAfterDataExecValue

// Test MCEngineExecPutIntoVariable ExecValue chunk variant
on TestPutAfterDataChunkExecValue
	local tVar, tArray
	put numToByte(1) into tVar
	put numToByte(1) into tArray[1]
	put tArray[1] after byte 1 of tVar
	TestAssert "put as execvalue after chunk preserves binary string", \
		tVar is strictly a binary string
end TestPutAfterDataChunkExecValue