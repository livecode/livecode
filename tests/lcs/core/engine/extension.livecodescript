script "CoreEngineExtensions"
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

constant kBytecodePath = "../_tests/_build/lcs/core/engine"

local sTestExtension

on TestSetup
    TestLoadExtension "com.livecode.library.json"
    TestLoadExtension "com.livecode.widget.clock"

    buildMultiModuleExtension "_extension,_extension_support"
    if the result is not empty then
       throw "Failed to build test support extension:" && the result
    end if
    put it into sTestExtension

    load extension from data sTestExtension
    if the result is not empty then
       throw "Failed to load test support extension:" && the result
    end if
end TestSetup

private command buildMultiModuleExtension pInputs, pOutput
    -- Build a multi-module extension
    local tBasename, tBytecode, tFile, tResult
    repeat for each item tBasename in pInputs
       put merge("[[kBytecodePath]]/[[tBasename]].lcm") into tFile
       put url("binfile:" & tFile) after tBytecode
       put the result into tResult
       if tResult is not empty then
          return merge("Failed to load [[tFile]]: [[tResult]]") for error
       end if
    end repeat

    return tBytecode for value
end buildMultiModuleExtension

/////////

on TestUnloadLibraryModule
    unload extension "com.livecode.library.json"
    TestAssert "library extension unloaded", the result is empty
end TestUnloadLibraryModule

/////////

command _TestUnloadWidgetModuleAfterDeletion_DoDeleteAndUnload
    delete widget "TestWidget"
    unload extension "com.livecode.widget.clock"
    TestAssert "widget extension unloaded", the result is empty
end _TestUnloadWidgetModuleAfterDeletion_DoDeleteAndUnload

on TestUnloadWidgetModuleAfterDeletion
    create stack "TestStack"
    create widget "TestWidget" as "com.livecode.widget.clock"

    -- Make sure we delete and then unload widget in a nested wait so that it
    -- isn't immediately deleted.
    send "_TestUnloadWidgetModuleAfterDeletion_DoDeleteAndUnload" to me in 0 millisecs
    wait 1 millisecond with messages
end TestUnloadWidgetModuleAfterDeletion

/////////

function TestExtensionBridgeNames_Version
   return the version
end TestExtensionBridgeNames_Version

on TestExtensionBridgeNames
   TestAssert "LCS names bridge to LCB strings in execute script", \
         TestExtensionBridgeNames_ExecuteScript()
   TestAssert "LCS names bridge to LCB strings in send", \
         TestExtensionBridgeNames_Send()
end TestExtensionBridgeNames

on TestExtensionBridgeNumbers
   TestAssert "LCS numbers bridge to LCB numbers in execute script", \
         TestExtensionBridgeNumbers_ExecuteScript()
end TestExtensionBridgeNumbers

on TestExtensionBridgeEmptyArrays
   TestAssert "LCS empty arrays bridge to LCB empty lists in library handlers", \
         TestExtensionBridgeArrays_EmptyToEmptyList(empty)
end TestExtensionBridgeEmptyArrays

//////////

on TestMultiModuleExtensions
   TestAssert "support modules are not in extension list", \
         "com.livecode.lcs_tests.core.extension.support" is not among \
         the lines of the loadedExtensions
   TestAssert "main modules are in extension list", \
         "com.livecode.lcs_tests.core.extension" is among \
         the lines of the loadedExtensions
   TestAssert "extension support module handlers work", \
         TestExtensionSupportModule_Handler()

   -- Check that unloading an extension also unloads support modules
   -- (allowing them to be loaded again)
   unload extension "com.livecode.lcs_tests.core.extension"
   TestAssert "unload multi-module extension", the result is empty

   load extension from data sTestExtension
   TestAssert "reload multi-module extension", the result is empty

   -- Check that extensions with incorrect support module names don't
   -- load
   unload extension "com.livecode.lcs_tests.core.extension"
   buildMultiModuleExtension "_extension_support,_extension"
   if the result is not empty then
      throw "Failed to build assembly:" && the result
   end if
   load extension from data it
   TestAssert "can't load multi-module extensions with bad module names", \
         the result is not empty
end TestMultiModuleExtensions

//////////

command CallLibraryHandlerWithString
   get TestExtensionLibraryHandler("foobar")
end CallLibraryHandlerWithString

command CallLibraryHandlerWithTooFewParams
   get TestExtensionLibraryHandler()
end CallLibraryHandlerWithTooFewParams

command CallLibraryHandlerWithTooManyParams
   get TestExtensionLibraryHandler(1, 2)
end CallLibraryHandlerWithTooManyParams

on TestExtensionLibraryHandlerCallErrors
   /* In all cases in this test we expect the error stack to be:
    *    <specific error>
    *    219, ...
    * Here '219' is the error in handler error which the call to the actual
    * TestExtensionLibraryHandler function will generate. */

   TestAssertThrow "library handler call throws correct error on type error", \
         "CallLibraryHandlerWithString", \
         the long id of me, \
         ("EE_INVOKE_TYPEERROR", "EE_FUNCTION_BADFUNCTION")

   TestAssertThrow "library handler call throws correct error on too few args error", \
         "CallLibraryHandlerWithTooFewParams", \
         the long id of me, \
         ("EE_INVOKE_TOOFEWARGS", "EE_FUNCTION_BADFUNCTION")

   TestAssertThrow "library handler call throws correct error on too many args error", \
         "CallLibraryHandlerWithTooManyParams", \
         the long id of me, \
         ("EE_INVOKE_TOOMANYARGS", "EE_FUNCTION_BADFUNCTION")
end TestExtensionLibraryHandlerCallErrors

//////////

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
   -- See Anomaly 19137
   TestExtensionExecuteScript_Do "return the long id of this card", empty, empty
   TestAssert "execute script works", the result is the long id of this card

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

local sLastLog
on logChanged pMsg
   put pMsg into sLastLog
end logChanged

on TestExtensionLogInteger
	// log 1
	get TestExtensionLog_Integer()
	wait for 0 with messages
	TestAssert "extension log integer", sLastLog is 1
end TestExtensionLogInteger

on TestExtensionLogList
	// log [1,2,3]
	get TestExtensionLog_List()
	wait for 0 with messages
	TestAssert "extension log list", sLastLog is "[1, 2, 3]"
end TestExtensionLogList

on TestExtensionLogArray
	// log {a: 1}
	get TestExtensionLog_Array()
	wait for 0 with messages
	TestAssert "extension log array", sLastLog is "{a: 1}"
end TestExtensionLogArray

on TestExtensionLogString
	// log "log"
	get TestExtensionLog_String()
	wait for 0 with messages
	TestAssert "extension log string", sLastLog is quote & "log" & quote
end TestExtensionLogString

on TestExtensionLogScriptObject
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
