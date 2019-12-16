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
    if the target is not me then
       pass revLoadLibrary
    end if
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

private function _TestGetBuildFolder
   local tPath, tRepoPath
   put specialfolderpath("engine") into tPath
   put TestGetEngineRepositoryPath() into tRepoPath

   # Find the built extensions folder
   set the itemdelimiter to slash
   repeat while tPath is not tRepoPath and tPath is not empty
      if there is a folder (tPath & slash & "packaged_extensions") then
         return tPath
      end if
      delete item -1 of tPath
   end repeat	
end _TestGetBuildFolder

private function _TestGetBuiltExtensionsFolder
   return _TestGetBuildFolder() & slash & "packaged_extensions"
end _TestGetBuiltExtensionsFolder

private command _TestLoadExtension pFolder, pPath
   local tCodeFolder
   put pFolder & "/code" into tCodeFolder
   
   if there is a folder tCodeFolder then
      local tCodeFolders
      put folders(tCodeFolder) into tCodeFolders
      switch the platform
         case "MacOS"
            filter tCodeFolders with "*-mac*"
            break
         case "Win32"
            filter tCodeFolders with "*-win*"
            break
         default
            filter tCodeFolders with "*-"& toLower(the platform) & "*"
            break
      end switch
      
      local tFilteredCodeFolders
      filter tCodeFolders with the processor & "-*" into tFilteredCodeFolders
      split tFilteredCodeFolders by return as set
      if the platform is "MacOS" then
         -- explicit processor should take precedence over universal builds but
         -- in the event multiple libraries are included and some are universal
         -- we must merge
         local tUniveralFilteredCodeFolders
         filter tCodeFolders with "universal-*" into tUniveralFilteredCodeFolders
         split tUniveralFilteredCodeFolders by return as set
         union tFilteredCodeFolders with tUniveralFilteredCodeFolders
      end if
      
      repeat for each key tFolder in tFilteredCodeFolders
         local tLibraries
         put files(tCodeFolder & "/" & tFolder) & return & \
               folders(tCodeFolder & "/" & tFolder) into tLibraries
         filter tLibraries without ".*"
         
         if tLibraries is not empty then
            set the itemDelimiter to "."
            repeat for each line tLibrary in tLibraries
               -- remove extension
               delete the last item of tLibrary
               if tLibrary is not empty then
                  set the revLibraryMapping[tLibrary] to tCodeFolder & "/" & tFolder & "/" & tLibrary
               end if
            end repeat
         end if
      end repeat
   end if
   if there is a folder (pFolder & slash & "resources") then
      do "load extension from file pPath with resource path (pFolder & slash & " & quote & "resources" & quote & ")"
   else
      do "load extension from file pPath"
   end if
   return the result
end _TestLoadExtension

function TestBuildErrorMap pErrorFile, pErrorPrefix, pZerothError
   local tSourceFile
   put TestGetEngineRepositoryPath() & slash & "engine/src/" & \
      pErrorFile into tSourceFile
   
   local tSource
   put textDecode(url("binfile:" & tSourceFile), "utf-8") into tSource
   
   local tErrorMap, tCode
   repeat for each line tLine in tSource
      if tLine is empty then next repeat
      if word 1 of tLine begins with pErrorPrefix & "_" & pZerothError then 
         next repeat
      end if
      
      if tCode is empty and \
            matchText(tLine, ".*\/\/ \{" & pErrorPrefix & "-([0-9]{4})\}.*", tCode) then
         next repeat
      end if
      
      local tError
      put item 1 of (word 1 of tLine) into tError
      if tError begins with pErrorPrefix & "_" or \ 
            tError begins with "NOTUSED__" & pErrorPrefix & "_" then
         if tCode is empty then 
            throw "no error code found for" && tError
         end if
         if tError begins with (pErrorPrefix & "_UNUSED_") then
            if tError is not (pErrorPrefix & "_UNUSED_" & tCode) then
               throw "unused error name" && tError && "does not match code" \
                     && tCode && "change to" && pErrorPrefix & "_UNUSED_" & tCode
            end if
         end if
         if tErrorMap[tCode] is not empty then
            throw "duplicate error code" && tCode && "found for" && tError && \
               "change to" && the number of elements of tErrorMap + 1
         end if
         if tCode - 1 is not the number of elements of tErrorMap then
            throw "incorrect error code" && tCode && "for" && tError && \
               "change to" && the number of elements of tErrorMap + 1
         end if

         put tError into tErrorMap[tCode]
         put empty into tCode
      end if
   end repeat

   return tErrorMap
end TestBuildErrorMap

local sExecErrorMap
private command __TestEnsureExecErrorMap
   if sExecErrorMap is not empty then
      exit __TestEnsureExecErrorMap
   end if

   put TestBuildErrorMap("executionerrors.h", "EE", "UNDEFINED") \
   	   into sExecErrorMap
end __TestEnsureExecErrorMap

function TestErrorMatches pError, pCode
	__TestEnsureExecErrorMap

	local tErrorNumber
	put format("%04d", item 1 of line 1 of pError) into tErrorNumber

	return sExecErrorMap[tErrorNumber] is pCode
end TestErrorMatches

----------------------------------------------------------------
-- Unit test library functions
----------------------------------------------------------------

on TestSetExecErrorMap pErrorMap
   put pErrorMap into sExecErrorMap
end TestSetExecErrorMap

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

function _TestHandlerThrows pHandler, pTarget, pErrorCodes, pParam
   local tError
   try
      dispatch pHandler to pTarget with pParam
   catch tError
   end try

   TestDiagnostic tError

   local tSuccess
   put true into tSuccess
   repeat for each item tErrorCode in pErrorCodes
      put tSuccess and TestErrorMatches(tError, tErrorCode) into tSuccess
      delete the first line of tError
   end repeat
   return tSuccess
end _TestHandlerThrows

on TestAssertThrow pDescription, pHandler, pTarget, pErrorCodes, pParam
   local tSuccess
   put _TestHandlerThrows(pHandler, pTarget, pErrorCodes, pParam) into tSuccess
   TestAssert pDescription, tSuccess
end TestAssertThrow

on TestAssertBrokenThrow pDescription, pHandler, pReason, pTarget, pErrorCodes, pParam
   local tSuccess
   put _TestHandlerThrows(pHandler, pTarget, pErrorCodes, pParam) into tSuccess
   TestAssertBroken pDescription, tSuccess, pReason
end TestAssertBrokenThrow

on TestAssertDoesNotThrow pDescription, pHandler, pTarget, pParam
  local tError
  try
    dispatch pHandler to pTarget with pParam
  catch tError
  end try

  TestAssert pDescription, tError is empty 
end TestAssertDoesNotThrow

local sError
on ErrorDialog pError
	put pError into sError
end ErrorDialog

on TestAssertErrorDialog pDescription, pErrorCode
	wait for messages
	TestAssert pDescription, TestErrorMatches(sError, pErrorCode)
	put empty into sError
end TestAssertErrorDialog

function TestGetUncaughtErrorDialog
	return sError
end TestGetUncaughtErrorDialog

function TestGetEngineRepositoryPath
  set the itemdelimiter to "/"
  return item 1 to -3 of the filename of me
end TestGetEngineRepositoryPath

function TestGetIDERepositoryPath
  set the itemdelimiter to "/"
  return item 1 to -3 of the filename of me & slash & "ide"
end TestGetIDERepositoryPath

function TestGetExtractedDocsFolder
   return _TestGetBuildFolder() & slash & "extracted_docs"
end TestGetExtractedDocsFolder

function TestGetPackagedExtensionsFolder
   return _TestGetBuildFolder() & slash & "packaged_extensions"
end TestGetPackagedExtensionsFolder

on TestLoadExtension pName
   if pName is among the lines of the loadedExtensions then
      return empty
   end if
   
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
      put the result into tError
   end if

   if tError is not empty then
      write tError & return to stderr
      quit 1
   end if

end TestLoadExtension

function TestGetBinariesPath
   local tEngineFolder
   put specialfolderpath("engine") into tEngineFolder

   set the itemdelimiter to slash
   if the platform is "MacOS" and the environment is not "server" then
      return item 1 to -4 of tEngineFolder
   end if

   return tEngineFolder
end TestGetBinariesPath

on TestLoadExternal pExternal
   local tEnginePath
   put specialfolderpath("engine") into tEnginePath

   local tBinariesPath, tExtension
   set the itemdelimiter to slash
   if the platform is "MacOS" then
      put "bundle" into tExtension
   else if the platform is "linux" then
      put "so" into tExtension
   end if

   put TestGetBinariesPath() into tBinariesPath

   set the externals of the templateStack to tBinariesPath & slash & \
      pExternal & "." & tExtension

   create stack pExternal && "External"
   start using it
   if the externalCommands of it is empty then
      write "Cannot load external" && pExternal & return to stderr
      quit 1
   end if
   
   -- Ensure drivers can be found
   if pExternal is "revdb" then
      revSetDatabaseDriverPath tBinariesPath
   end if
end TestLoadExternal 

private function __GetCaller
   local tLineNum
   put -1 into tLineNum
   repeat while item 1 of line tLineNum of the executionContexts \
      is the long id of this me
      subtract 1 from tLineNum
   end repeat
   get item 1 to -3 of line tLineNum of the executionContexts
   if there is not an it then
      delete item -1 of it
   end if
   return it
end __GetCaller

private function __StackOfObject pLongID
   local tOffset
   put wordOffset("stack",pLongID) into tOffset
   return word tOffset to -1 of pLongID
end __StackOfObject

-- This loads an extension whose lcb source sits in the same folder as the
-- current test.
on TestLoadAuxiliaryExtension pName, pResourceFolder
   local tBytecodeFile
   put TestGetBuiltBytecodeFile(pName) into tBytecodeFile
   if pResourceFolder is empty then
      load extension from data url ("binfile:" & tBytecodeFile)
   else
      load extension from data url ("binfile:" & tBytecodeFile) \
            with resource path pResourceFolder
   end if
   
   if the result is not empty then
      throw "Failed to load auxiliary extension:" && \
            the result && tBytecodeFile
   end if
end TestLoadAuxiliaryExtension

constant kTestsBuildPath = "_tests/_build/"
function TestGetBuiltBytecodeFile pName
   local tBasePath, tExtraPath
   set the itemDelimiter to slash
   put item 1 to -2 of the effective filename of \
         __StackOfObject(__GetCaller()) into tBasePath
         
   local tTestsOffset
   set the wholematches to true
   put itemOffset("tests", tBasePath) into tTestsOffset
   put item tTestsOffset + 1 to -1 of tBasePath into tExtraPath
   put item 1 to tTestsOffset - 1 of tBasePath into tBasePath
   
   local tModuleFile
   put tBasePath & "/" & kTestsBuildPath & tExtraPath & slash & \ 
      pName & ".lcm" into tModuleFile
   return tModuleFile
end TestGetBuiltBytecodeFile

function TestGetTestBuildFolder
	return TestGetEngineRepositoryPath() & slash & kTestsBuildPath
end TestGetTestBuildFolder

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

    local tTimer, tTimeout, tTimerStart
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

command TestRunStack pOptions, pStackFilePath, pArgs
	local tEnginePath
	set the itemDelimiter to ":"
	put item 2 of the address into tEnginePath

	local tCommand
	put format("\"%s\" %s \"%s\" %s", tEnginePath, pOptions, pStackFilePath, pArgs) into tCommand

	local tOutput
	put shell(tCommand) into tOutput
   if the result is not empty then
      return the result for error
   else
      return tOutput for value
   end if
end TestRunStack

function TestIsInStandalone
   return "StandaloneTestRunnerMainstack" is among the lines of the mainstacks
end TestIsInStandalone

on TestSkipIf pRequirement, pOptions
   if __MeetsRequirements(pRequirement, pOptions) then
      if pOptions is not empty then
         put " :" & pOptions after pRequirement
      end if
      throw "SKIP test incompatible with" && pRequirement
   end if
end TestSkipIf

on TestSkipIfNot pRequirement, pOptions
   if not __MeetsRequirements(pRequirement, pOptions) then
      if pOptions is not empty then
         put " :" & pOptions after pRequirement
      end if
      throw "SKIP test requires" && pRequirement
   end if
end TestSkipIfNot

private function __MeetsRequirements pRequirement, pOptions
   switch pRequirement
      case "ide"
      case "docs"
      case "lcb"
         return not TestIsInStandalone()
         break
      case "standalone"
         return TestIsInStandalone()
         break
      case "securitypermissions"
         if "set" is among the items of pOptions then
            return not TestIsInStandalone()
         end if
         break
      case "platform"
         return the platform is among the items of pOptions
         break
      case "processor"
         return the processor is among the items of pOptions
         break
      case "environment"
         return the environment is among the items of pOptions
         break
      case "stack"
         repeat for each item tStack in pOptions
            if there is not a stack tStack then
               return false
            end if
         end repeat
         return true
         break
      case "clipboard"
      case "wait"
      case "security"
         return the platform is not "HTML5"
      case "write"
         return the platform is not "HTML5" and ("disk" is in the securityPermissions or the secureMode is false)
      case "ui"
         return not (the environment ends with "command line")
      case "desktop"
         return the platform is among the items of "MacOS,Win32,Linux"
      case "mobile"
         return the platform is among the items of "Android,iOS"
      case "external"
         -- TODO: make externals tests work in standalone test builder
         if TestIsInStandalone() then
            return false
         end if
         
         if "revpdfprinter" is among the items of pOptions \
         	and the environment is "server" then
         	return false
         end if
         return the platform is not "HTML5"
      case "database"
         if not __MeetsRequirements("external", "revdb") then
            return false
         end if
         if "odbc" is among the items of pOptions or \ 
               "postgresql" is among the items of pOptions then
            return __MeetsRequirements("desktop")
         end if  
         return the platform is not "HTML5"
      case "jvm"
         return __MeetsRequirements("platform", "MacOS,Linux")
   end switch
   return true
end __MeetsRequirements

command TestEnsureJVM
   TestSkipIfNot "jvm"

   if $JAVA_HOME is empty then
      local tPath
      switch the platform
         case "MacOS"
            put word 1 to -1 of shell("/usr/libexec/java_home") into tPath
            break
         case "Linux"
            local tJAVAC
            put "/bin/javac" into tJAVAC
            put word 1 to -1 of shell("/usr/bin/env readlink -f /usr" & tJAVAC) into tPath
            if there is a file tPath and tPath ends with tJAVAC then
               set the itemDelimiter to slash
               delete item -2 to -1 of tPath
            end if
            break
      end switch
      if there is a folder tPath then
         put tPath into $JAVA_HOME
      end if
   end if
end TestEnsureJVM

command TestPropRoundTrip pObjectType, pProperty, pValues
   local tStack
   put "TestRoundTrip" & pProperty into tStack
   create stack tStack
   set the defaultStack to tStack

   set the filename of stack tStack to the tempname

   local tObject
   if pObjectType is not "stack" then
      do "create" && pObjectType
      put it into tObject
   else
      put the long id of stack tStack into tObject
   end if

   repeat for each element tValue in pValues
      _TestPropRoundTrip tStack, tObject, pProperty, tValue
   end repeat

   delete file the filename of stack tStack
   delete stack tStack
end TestPropRoundTrip

private command _TestPropRoundTrip pStack, pObject, pProperty, pValue
	set the pProperty of pObject to pValue
	TestAssert format("round trip %s '%s'", pProperty, pValue), \
			the pProperty of pObject is pValue
	TestDiagnostic format("%s is '%s'", pProperty, the pProperty of pObject)

	save stack pStack
	delete stack pStack
	TestAssert format("round trip to disk %s '%s'", pProperty, pValue), \
			the pProperty of pObject is pValue
	TestDiagnostic format("%s is '%s'", pProperty, the pProperty of pObject)
end _TestPropRoundTrip
