script "CoreFileIO"
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

on TestReadForFixedEof
   local tFile
   put tempName() into tFile
   put "hello world" into url ("binfile:" & tFile)
   
   local tResult
   open file tFile for binary read
   read from file tFile for 16284 bytes
   put the result into tResult
   close file tFile

   TestAssert "reading past end of file returns eof", tResult is "eof"
end TestReadForFixedEof

on TestImportEPS
   local tStackPath
   
   put the filename of this stack into tStackPath
   
   set the itemDel to "/"
   
   put item 1 to -2 of tStackPath into tStackPath
   
   set the defaultFolder to tStackPath
   
   import eps from file "icon_android.eps"
   
   TestAssert "eps object created", there is an eps 1
 end TestImportEPS 