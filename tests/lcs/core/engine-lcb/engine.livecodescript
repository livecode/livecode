script "CoreEngineExtensionsEngineModule"
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

on TestSetup
   TestSkipIfNot "lcb"
   
   TestLoadAuxiliaryExtension "_engine"
   TestLoadAuxiliaryExtension "_widget_engine"
end TestSetup

on TestExtensionLibraryDelimiterAccess
   set the itemDelimiter to "a"
   set the lineDelimiter to "b"
   set the columnDelimiter to "c"
   set the rowDelimiter to "d"

   local tDelimiters
   put TestExtensionDelimiterAccess() into tDelimiters
   combine tDelimiters with comma

   TestDiagnostic tDelimiters
   TestAssert "library handler can access delimiters", tDelimiters is "a,b,c,d"
end TestExtensionLibraryDelimiterAccess

//////////

on TestExtensionExecuteScript
    -- Implicit object in libraries is this card of the defaultStack
    TestExtensionExecuteScript_Do "return the long id of this card", empty, empty
    TestAssert "execute script in library works", the result is the long id of this card of the defaultStack

    -- Implicit object in widgets is the widget
    local tWidget
    create widget as "com.livecode.lcs_tests.core.engine_lcb.widget_engine"
    put it into tWidget
    set the scriptString of tWidget to "return the long id of me"
    TestAssert "execute script in widget works", the scriptResult of tWidget is tWidget

    local tParams
    put "foobar" into tParams[1]
    set the scriptParams of tWidget to tParams
    set the scriptString of tWidget to "set the uTest of me to param(1)"
    get the scriptResult of tWidget
    TestAssert "execute script in widget with params works", the uTest of tWidget is "foobar"

    create stack "TempStack"
    TestExtensionExecuteScript_Do "return the short name of me", the long id of stack "TempStack", empty
    TestAssert "execute script in object works", the result is the short name of stack "TempStack"
    delete stack "TempStack"

    local tArgs
    put 200 into tArgs[1]
    TestExtensionExecuteScript_Do "return param(1)", empty, tArgs
    TestAssert "execute script with arguments works", the result is 200

    create stack "TempStack2"
    TestExtensionExecuteScript_Do "return the short name of me & param(1)", the long id of stack "TempStack2", tArgs
    TestAssert "execute script in object with arguments works", the result is "TempStack2200"
    delete stack "TempStack2"
end TestExtensionExecuteScript

/////////

/*
On server, extension logs go directly to stderr, so skip these tests.
*/

local sLastLog
on logChanged pMsg
   put pMsg into sLastLog
end logChanged

on TestExtensionLogInteger
	TestSkipIf "environment", "server"
	
	// log 1
	get TestExtensionLog_Integer()
	wait for 0 with messages
	TestAssert "extension log integer", sLastLog is 1
end TestExtensionLogInteger

on TestExtensionLogList
	TestSkipIf "environment", "server"
	
	// log [1,2,3]
	get TestExtensionLog_List()
	wait for 0 with messages
	TestAssert "extension log list", sLastLog is "[1, 2, 3]"
end TestExtensionLogList

on TestExtensionLogArray
	TestSkipIf "environment", "server"
	
	// log {a: 1}
	get TestExtensionLog_Array()
	wait for 0 with messages
	TestAssert "extension log array", sLastLog is "{a: 1}"
end TestExtensionLogArray

on TestExtensionLogString
	TestSkipIf "environment", "server"
	
	// log "log"
	get TestExtensionLog_String()
	wait for 0 with messages
	TestAssert "extension log string", sLastLog is quote & "log" & quote
end TestExtensionLogString

on TestExtensionLogScriptObject
   TestSkipIf "environment", "server"
	
  // resolve script object "button 1"
  // log the result
  create button
  get TestExtensionLog_ScriptObject()
  wait for 0 with messages
  TestDiagnostic sLastLog
  TestAssert "extension log unnamed scriptobject", \
              sLastLog is format("<script object button id %d of card id %d of stack %s>", \
                                  the id of button 1, \
                                  the id of this card, \
                                  quote & the short name of this stack & quote)

  set the name of button 1 to "foo"
  get TestExtensionLog_ScriptObject()
  wait for 0 with messages
  TestDiagnostic sLastLog
  TestAssert "extension log named scriptobject", \
              sLastLog is format("<script object button %s of card id %d of stack %s>", \
                                  quote & the short name of button 1 & quote, \
                                  the id of this card, \
                                  quote & the short name of this stack & quote)

  get TestExtensionLog_DeletedScriptObject()
  wait for 0 with messages
  TestDiagnostic sLastLog
  TestAssert "extension log deleted scriptobject", sLastLog is "<deleted script object>"
end TestExtensionLogScriptObject

local sCallbackTarget
on CallerCallback
	TestDiagnostic "called"
	put the long id of the target into sCallbackTarget
end CallerCallback

on TestEngineCaller
   local tButton
   create button
   put it into tButton
   set the script of tButton to "on doTest;TestEngine_Caller" \
      && quote & "CallerCallback" & quote & ";end doTest"
    send "doTest" to tButton
    TestAssert "library handler can get caller", \
    	the long id of sCallbackTarget is the long id of tButton
end TestEngineCaller

//////////

on TestEngineResolvePath
    local tDefaultStack
    put the defaultStack into tDefaultStack

    local tWidget
    create widget as "com.livecode.lcs_tests.core.engine_lcb.widget_engine"
    put it into tWidget

    local tPath
    set the pathExplicit of tWidget to "_testrunner.livecodescript"
    put the pathExplicit of tWidget into tPath
    TestAssert \
      "resolve path relative to explicit object in widget", \
      tPath is the filename of stack "TestRunner"

    TestAssert \
      "resolve path relative to explicit object in library", \
      TestEngine_ResolveFile("_testrunner.livecodescript", \
      the long id of stack "TestRunner") \
      is the filename of stack "TestRunner"

    TestAssert \
      "resolve path relative to implicit object in library", \
      TestEngine_ResolveFile("engine.livecodescript", "") \
      is the filename of me

    -- test implicit widget with the default stack elsewhere
    set the defaultStack to "TestRunner"
    set the pathImplicit of tWidget to "engine.livecodescript"
    put the pathImplicit of tWidget into tPath
    TestAssert \
      "resolve path relative to implicit widget", \
      tPath is the filename of me

    set the defaultStack to tDefaultStack
end TestEngineResolvePath
