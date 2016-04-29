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
