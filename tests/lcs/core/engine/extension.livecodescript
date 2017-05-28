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

on TestSetup
    TestLoadExtension "com.livecode.library.json"
    TestLoadExtension "com.livecode.widget.clock"

    TestDiagnostic the defaultfolder
    load extension from file "../_tests/_build/lcs/core/engine/_extension.lcm"
    if the result is not empty then
       throw "Failed to load test support extension:" && the result
    end if
end TestSetup

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
