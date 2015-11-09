script "JSONLibrary"
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

on TestSetup
	TestLoadExtension "com.livecode.library.json"
end TestSetup

on TestExportArray
   local tArray
   put "value" into tArray["key"]
   
   local tExpected
   put "{" & quote & "key" & quote & ":" & quote & "value" & quote & "}" into tExpected
   
   local tJson
   put JsonExport(tArray) into tJson
   # Ignore whitespace
   replace " " with empty in tJson
   
   TestAssert "simple array export", tExpected is tJson
end TestExportArray

on TestImportArray
   local tJson, tArray
   put "{" & quote & "key" & quote & ":" & quote & "value" & quote & "}" into tJson
   put JsonImport(tJson) into tArray
   
   TestAssert "simple array import", tArray["key"] is "value"
end TestImportArray
