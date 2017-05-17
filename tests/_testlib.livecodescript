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
	set the lockErrorDialogs to true
end revLoadLibrary

----------------------------------------------------------------
-- Helper functions
----------------------------------------------------------------

local sOutputToVariable
local sOutput

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
   if sOutputToVariable then
      put pMessage after sOutput
   else
      write textEncode(pMessage, "UTF8") to stdout
   end if
end _TestWriteOutput

private function _TestGetBuiltExtensionsFolder
   local tPath, tRepoPath
   put specialfolderpath("engine") into tPath
   put TestGetEngineRepositoryPath() into tRepoPath

   # Find the built extensions folder
   set the itemdelimiter to slash
   repeat while tPath is not tRepoPath and tPath is not empty
      if there is a folder (tPath & slash & "packaged_extensions") then
         return (tPath & slash & "packaged_extensions")
      end if
      delete item -1 of tPath
   end repeat
end _TestGetBuiltExtensionsFolder

private command _TestLoadExtension pFolder, pPath
   if there is a folder (pFolder & slash & "resources") then
      do "load extension from file pPath with resource path (pFolder & slash & " & quote & "resources" & quote & ")"
   else
      do "load extension from file pPath"
   end if
   return the result
end _TestLoadExtension

----------------------------------------------------------------
-- Unit test library functions
----------------------------------------------------------------

on TestOutputToVariable
   put true into sOutputToVariable
end TestOutputToVariable

function TestFetchAndClearOutput
   if not sOutputToVariable then
      return empty
   end if

   local tOutput
   put sOutput into tOutput
   put empty into sOutput
   return tOutput
end TestFetchAndClearOutput

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

on TestAssertThrow pDescription, pHandler, pTarget, pErrorCode, pParam
   local tError
   try
      dispatch pHandler to pTarget with pParam
    catch tError
    end try

    TestAssert pDescription, pErrorCode is item 1 of line 1 of tError
end TestAssertThrow

on TestAssertDoesNotThrow pDescription, pHandler, pTarget, pParam
  local tError
  try
    dispatch pHandler to pTarget with pParam
  catch tError
  end try

  TestAssert pDescription, tError is empty 
end TestAssertDoesNotThrow

on ErrorDialog pError
   TestAssert "test runs without error - " && line 1 of pError \
      , false
   TestDiagnostic pError
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

   local tError
   put "extension" && pName && "not found" into tError

   local tExtensionsFolder
   put _TestGetBuiltExtensionsFolder() into tExtensionsFolder

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
      _TestLoadExtension tExtensionFolder, tExtensionFile
   end if
   put the result into tError

   if tError is not empty then
      write tError & return to stderr
      quit 1
   end if

end TestLoadExtension

-- This loads an extension whose lcb source sits in the same folder as the
-- current test.
on TestLoadAuxillaryExtension pName
	local tBasePath, tExtraPath
	set the itemDelimiter to slash
	put item 1 to -2 of the filename of this stack into tBasePath
	repeat while the last item of tBasePath is not "tests"
		put item -1 of tBasePath & slash before tExtraPath
		delete the last item of tBasePath
	end repeat

	local tModuleFile
	put tBasePath & "/../_tests/_build/" & tExtraPath & pName & ".lcm" into tModuleFile

	load extension from data url ("binfile:" & tModuleFile)
	if the result is not empty then
		throw "Failed to load auxillary extension:" && the result && tModuleFile
	end if
end TestLoadAuxillaryExtension

on TestLoadAllExtensions
   local tExtFolder
   put _TestGetBuiltExtensionsFolder() into tExtFolder

   local tExtensions
   put folders(tExtFolder) into tExtensions

   local tFiles, tExtensionsA
   local tHasCompiled, tSource

   # Collect all the valid (and compiled) extension files
   repeat for each line tFolder in tExtensions
      if tFolder begins with "." then
         next repeat
      end if
      put false into tHasCompiled
      put empty into tSource
      put tExtFolder & slash before tFolder
      put files(tFolder) into tFiles
      repeat for each line tFile in tFiles
         if tFile ends with ".lcb" then
            put tFile into tSource
         else if tFile is "module.lcm" then
            put true into tHasCompiled
         end if
         if tHasCompiled and tSource is not empty then
         	local tPath
         	put tFolder & slash & tSource into tPath
            put tFolder into tExtensionsA[tPath]
            exit repeat
         end if
      end repeat
   end repeat

   # Use the lc-compile --deps option to sort by dependency
   local tLCCompile, tLCBFiles
   put the keys of tExtensionsA into tLCBFiles
   replace return with " " in tLCBFiles
   set the itemdelimiter to slash
   put item 1 to -2 of tExtFolder & slash & "lc-compile" into tLCCompile
   put shell(tLCCompile && "--deps order --" && tLCBFiles) into tLCBFiles
   if the result is not 0 then
      write the result & return to stderr
      quit 1
   end if

   # Load the extensions in order
   repeat for each line tExtFile in tLCBFiles
      _TestLoadExtension tExtensionsA[tExtFile], tExtensionsA[tExtFile] & slash & "module.lcm"
   end repeat
end TestLoadAllExtensions

on TestRepeat pDesc, pHandler, pTarget, pTimeOut, pParamsArray
	# Construct a dispatch command with all the desired parameters
	local tDoString, tParams, tResult
	put "dispatch pHandler to pTarget" into tDoString

	repeat with tParam = 1 to the number of elements in pParamsArray
		if tParams is empty then
        	put " with pParamsArray[" & tParam & "]" into tParams
        else
        	put ", pParamsArray[" & tParam & "]" after tParams
        end if
    end repeat

    put tParams after tDoString
    put "; put the result into tResult" after tDoString

    put false into tResult

    local tTimer, tTimeout
    put false into tTimeout
    put the millisecs into tTimerStart

    repeat while tResult is false and not tTimeout
       wait 1 millisecs with messages
	   do tDoString
       if the millisecs - tTimerStart > pTimeOut then
          put true into tTimeOut
       end if
	end repeat

    TestAssert pDesc, tResult

    if (not tResult) and tTimeOut then
		TestDiagnostic pDesc & "- timed out"
	end if
end TestRepeat

on TestRunStack pOptions, pStackFilePath
	local tEnginePath
	set the itemDelimiter to ":"
	put item 2 of the address into tEnginePath

	local tCommand
	put format("\"%s\" %s \"%s\"", tEnginePath, pOptions, pStackFilePath) into tCommand

	local tOutput
	put shell(tCommand) into tOutput

	return the result
end TestRunStack
