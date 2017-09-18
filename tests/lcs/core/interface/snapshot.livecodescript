script "CoreInterfaceSnapshot"
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

on _TestImportScreenSnapshotFromRectHandler
    import snapshot from rectangle 100,100,500,400
    
    TestAssert "long id of new image is long id of it",  the long id of it is the long id of the last image
    TestAssertBroken "long id of image is it", it is the long id of the last image
end _TestImportScreenSnapshotFromRectHandler

on TestImportScreenSnapshotFromRectNoUI
   TestSkipIf "ui"
   TestSkipIf "platform", "HTML5"
   TestAssertThrow "should now throw an error", "_TestImportScreenSnapshotFromRectHandler",\
        the long id of me, "EE_SNAPSHOT_FAILED"
end TestImportScreenSnapshotFromRectNoUI

on TestImportScreenSnapshotFromRectUI
    TestSkipIfNot "ui"
    TestSkipIf "environment", "server"
    TestSkipIf "platform", "HTML5"

    _TestImportScreenSnapshotFromRectHandler
end TestImportScreenSnapshotFromRectUI

on _TestImportSnapshotFromStackHandler
    create stack "Untitled 1"

    import snapshot from window the windowId of stack "Untitled 1"
    TestAssert "long id of new image is long id of it",  the long id of it is the long id of the last image
    TestAssertBroken "long id of image is it", it is the long id of the last image
end _TestImportSnapshotFromStackHandler

on TestImportSnapshotFromStack
    if the environment is not "command line" then
        return "SKIP TEST"
    end if
    TestAssertThrow "should now throw an error", "_TestImportSnapshotFromStackHandler",\
        the long id of me, "EE_SNAPSHOT_FAILED"
end TestImportSnapshotFromStack

on TestImportSnapshotFromObject
    create button "b1"

    import snapshot from button "b1"

    TestAssert "long id of new image is long id of it",  the long id of it is the long id of the last image
    TestAssertBroken "long id of image is it", it is the long id of the last image
end TestImportSnapshotFromObject

on TestImportSnapshotFromRectOfObject
   create field "field 1"
   set the rect of field "field 1" to 0,0,100,100
   
   import snapshot from rectangle 0,0,100,100 of field "field 1"
   TestAssert "long id of new image is long id of it",  the long id of it is the long id of the last image
   TestAssertBroken "long id of image is it", it is the long id of the last image
end TestImportSnapshotFromRectOfObject

