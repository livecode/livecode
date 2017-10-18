script "CoreInterfaceImport"
/*
Copyright (C) 2017 LiveCode Ltd.

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

-- Utility function that returns the folder containing this test
private function _TestPath
  set the itemDelimiter to slash
  return item 1 to -2 of the filename of me
end _TestPath

on TestImportPaintInGroup
   local tTargetGroup
   create group
   put it into tTargetGroup

   local tImportedImage, tImageFilenameFullPath
   put TestGetInputFile("tiny.png") into tImageFilenameFullPath
   import paint from file tImageFilenameFullPath in tTargetGroup
   put it into tImportedImage
   
   TestAssert "Now-grouped image is owned by the group", \
      the long id of the owner of tImportedImage is tTargetGroup
end TestImportPaintInGroup