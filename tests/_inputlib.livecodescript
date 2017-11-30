script "testInputLibrary"
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

on revLoadLibrary
   if the target is not me then
      pass revLoadLibrary
   end if
   insert the script of me into back
end revLoadLibrary

local sTestStrings, sTestNumbers, sTestConversionStrings, sTestFilePaths, sArrayData
local sInitialised

function TestGetInputFilesFolder
   set the itemDelimiter to slash
   return item 1 to -2 of the filename of me & slash & "lcs-input-files"
end TestGetInputFilesFolder

private function getTestFile pTestFile
   local tContent, tFilePath
   
   put TestGetInputFilesFolder() & slash & pTestFile into tFilePath
   
   // test files are written without BOM
   open file tFilePath for "utf8" text read
   read from file tFilePath until EOF
   
   put it into tContent
   
   close file tFilePath
   return tContent
end getTestFile

on setUpValues
   if sInitialised then
      exit setUpValues
   end if
   
   put true into sInitialised
   
   //load in strings
   local tStrings
   put getTestFile("testStrings.txt") into tStrings
   put empty into sArrayData
   
   local tFirstLine
   put line 1 of tStrings into tFirstLine
   delete line 1 of tStrings
   
   repeat for each line tLine in tStrings
      local tType, tNumber
      put item 1 of tLine into tType
      put item 2 of tLine into tNumber
      
      put item 3 of tLine into sTestStrings[tType][tNumber][item 3 of tFirstLine]
      put item 4 of tLine into sTestStrings[tType][tNumber][item 4 of tFirstLine]
      put item 5 of tLine into sTestStrings[tType][tNumber][item 5 of tFirstLine]
      put item 6 of tLine into sTestStrings[tType][tNumber][item 6 of tFirstLine]
   end repeat
   
   
   //load in numbers
   local tNumbers
   put getTestFile("testNumbers.txt") into tNumbers
   
   repeat for each line tLine in tNumbers
      put item 2 of tLine into sTestNumbers[item 1 of tLine][1]
      put item 3 of tLine into sTestNumbers[item 1 of tLine][2]
   end repeat
   
   
   // Load in conversion strings
   local tConversionStrings
   put getTestFile("testConversionStrings.txt") into tConversionStrings
   
   repeat for each line tLine in tConversionStrings
      put item 2 of tLine into sTestConversionStrings[item 1 of tLine]["lowerCase"]
      put item 3 of tLine into sTestConversionStrings[item 1 of tLine]["upperCase"]
      put item 4 of tLine into sTestConversionStrings[item 1 of tLine]["lowerCaseNumber"]
      put item 5 of tLine into sTestConversionStrings[item 1 of tLine]["upperCaseNumber"]
      put item 6 of tLine into sTestConversionStrings[item 1 of tLine]["lowerCaseByte"]
      put item 7 of tLine into sTestConversionStrings[item 1 of tLine]["upperCaseByte"]
   end repeat
   
   
   // Load in file paths
   local tFilePaths
   put getTestFile("testFilePaths.txt") into tFilePaths
   
   repeat for each line tLine in tFilePaths
      put item 2 of tLine into sTestFilePaths[item 1 of tLine]
   end repeat
   
   
   // Load in array data
   local tArrayData
   put getTestFile("testArrayData.txt") into tArrayData
   
   repeat for each line tLine in tArrayData
      put item 2 to -1 of tLine into sArrayData[item 1 of tLine]
   end repeat
end setUpValues

function testString pType,pNumber, pValue
   setupValues
   if pValue is empty then put "string" into pValue   
   put "String:" && pType, pNumber, pValue && sTestStrings[pType][pNumber][pValue]& return after url ("binfile:" & specialFolderPath("desktop")  & "/log.txt")
   return sTestStrings[pType][pNumber][pValue]
end testString

function testNumber pType,pNumber
   setupValues
   return sTestNumbers[pType][pNumber]
end testNumber

function testConversionString pStringType, pConversionType
   setupValues
   return sTestConversionStrings[pStringType][pConversionType]
end testConversionString

function testFilePath pType
   setupValues
   return sTestFilePaths[pType]
end testFilePath

function testFolderList
   setupValues
   local tFileList
   repeat for each element tFile in sTestFilePaths
      put tFile & return after tFileList
   end repeat
   delete the last character of tFileList
   return tFileList
end testFolderList

function testFileList
   setupValues
   local tFileList
   repeat for each element tFile in sTestFilePaths
      put tFile & ".txt" & return after tFileList
   end repeat
   delete the last character of tFileList
   return tFileList
end testFileList

function testArrayData pKey
   setupValues
   return sArrayData[pKey]
end testArrayData

function TestGetInputFile pFilename
    return TestGetInputFilesFolder() & slash & pFilename
end TestGetInputFile
