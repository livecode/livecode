script "TestLibrary"
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
	insert the script of me into back
end revLoadLibrary

----------------------------------------------------------------
-- Helper functions
----------------------------------------------------------------

private function _TestValidateCount pCount
   if pCount is not an integer or pCount <= 0 then
      throw "Bad test count '" & pCount & "': must be positive integer"
   end if
   return pCount
end _TestValidateCount

private function _TestValidateDescription pDescription
   if the number of lines in pDescription > 1 then
      throw "Bad test description '" & line 1 of pDescription & "...': multiple lines"
   end if
   
   if "0123456789" contains codepoint 1 of pDescription then
      throw "Bad test description '" & pDescription & "': starts with digit"
   end if
   
   if pDescription contains "#" then
      throw "Bad test description '" & pDescription & "': contains '#'"
   end if
   
   return line 1 of pDescription
end _TestValidateDescription

private function _TestValidateReason pReason
   if the number of lines in pReason > 1 then
      throw "Bad test directive reason '" & line 1 of pReason & "...': multiple lines"
   end if
   
   if pReason contains "#" then
      throw "Bad test directive reason '" & pReason & "': contains '#'"
   end if
   
   return line 1 of pReason
end _TestValidateReason

private function _TestValidateDirective pDirective
   switch pDirective
      case empty
         return empty
      case "TODO"
         return "TODO"
      case "SKIP"
         return "SKIP"
      default
         throw "Bad test directive '" & line 1 of pDirective & "'"
   end switch
end _TestValidateDirective

-- Used by top level assertion functions to generate output
private command _TestOutput pIsOkay, pDescription, pDirective, pReason
   local tDescription, tDirective, tReason
   put _TestValidateDescription(pDescription) into tDescription
   put _TestValidateDirective(pDirective) into tDirective
   put _TestValidateReason(pReason) into tReason
   
   local tMessage
   if pIsOkay then
      put "ok" into tMessage
   else
      put "not ok" into tMessage
   end if
   
   if tDescription is not empty then
      put " - " & tDEscription after tMessage
   end if
   
   if tDirective is not empty then
      put " # " & tDirective after tMessage
      
      if tReason is not empty then
         put " " & tReason after tMessage
      end if
   end if
   
   _TestWriteOutput tMessage & return
end _TestOutput

private command _TestWriteOutput pMessage
   -- As stdout is considered to be a 'native' stream, we encode to UTF-8 first
   -- then will unencode in the test runner.
   write textEncode(pMessage, "UTF8") to stdout
end _TestWriteOutput

----------------------------------------------------------------
-- Unit test library functions
----------------------------------------------------------------

on TestPlan pCount
   _TestWriteOutput "1.." & _TestValidateCount(pCount) & return
end TestPlan

on TestDiagnostic pMessage
   local tLine
   repeat for each line tLine in pMessage
      _TestWriteOutput "#" && tLine & return
   end repeat
end TestDiagnostic

on TestSkip pDescription, pReasonSkipped
   _TestOutput true, pDescription, "SKIP", pReasonSkipped
end TestSkip

on TestAssert pDescription, pExpectTrue
   _TestOutput pExpectTrue, pDescription, empty, empty
end TestAssert

on TestAssertBroken pDescription, pExpectTrue, pReasonBroken
   _TestOutput pExpectTrue, pDescription, "TODO", pReasonBroken
end TestAssertBroken

on ErrorDialog executionError, parseError
   write executionError & return to stderr
   quit 1
end ErrorDialog

function TestGetEngineRepositoryPath
  set the itemdelimiter to "/"
  return item 1 to -3 of the filename of me
end TestGetEngineRepositoryPath

function TestGetIDERepositoryPath
  set the itemdelimiter to "/"
  return item 1 to -3 of the filename of me & slash & "ide"
end TestGetIDERepositoryPath

on TestLoadExtension pName	
   set the itemdelimiter to "."
   local tExtensionUnzipFolder
   put pName into tExtensionUnzipFolder

   local tPath, tRepoPath
   put specialfolderpath("engine") into tPath
   put TestGetEngineRepositoryPath() into tRepoPath
   
   local tError
   put "extension" && pName && "not found" into tError
   
   # Find the built extensions folder
   local tExtensionsFolder
   set the itemdelimiter to slash
   repeat while tPath is not tRepoPath and tPath is not empty
      if there is a folder (tPath & slash & "packaged_extensions") then
         put (tPath & slash & "packaged_extensions") into tExtensionsFolder
         exit repeat
      end if
      delete item -1 of tPath
   end repeat
   
   local tExtensionFolder
   if tExtensionsFolder is not empty then
      if there is a folder (tExtensionsFolder & slash & tExtensionUnzipFolder) then
        put (tExtensionsFolder & slash & tExtensionUnzipFolder) into tExtensionFolder
      end if
   end if
   
   local tExtensionFile
   if tExtensionFolder is not empty then
      if there is a file (tExtensionFolder & slash & "module.lcm") then
         put (tExtensionFolder & slash & "module.lcm") into tExtensionFile
      end if
   end if
   
   if tExtensionFile is not empty then
      if there is a folder (tExtensionFolder & slash & "resources") then
         do "load extension from file tExtensionFile with resource path (tExtensionFolder & slash & " & quote & "resources" & quote
      else
         do "load extension from file tExtensionFile"
      end if
      
      put the result into tError
   end if
   
   if tError is not empty then
      write tError & return to stderr
      quit 1
   end if
   
end TestLoadExtension
