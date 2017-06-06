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

on TestImportScreenSnapshotFromRect
    if the environment is "command line" then
      return "SKIP GUI-only tests"
    end if
    import snapshot from rectangle 100,100,500,400
    
    TestAssert "test set it", it is the long id of the last image
end TestImportScreenSnapshotFromRect

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