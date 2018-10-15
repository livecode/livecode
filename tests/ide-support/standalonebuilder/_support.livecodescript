script "StandaloneBuilderTestSupport"
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

on libraryStack
end libraryStack

function GetBuildPlatform
	switch the platform
		case "MacOS"
			if the processor is "x86_64" then
				return "MacOSX x86-64"
			else
				return "MacOSX x86-32"
			end if
			break
		case "Windows"
			return "Windows"
			break
		case "Linux"
			if the processor is "x86_64" then
				return "Linux x64"
			else
				return "Linux"
			end if
			break
	end switch
end GetBuildPlatform

private function GetTargetFolder
	local tFolder
	put TestGetIDERepositoryPath() & "/_tests/_build" into tFolder
	revSBEnsureFolder tFolder
	return tFolder
end GetTargetFolder

on StandaloneBuilderCleanupStandalones 
	local tTargetFolder
	put GetTargetFolder() into tTargetFolder
	revDeleteFolder tTargetFolder	
end StandaloneBuilderCleanupStandalones

on StandaloneBuilderSaveAsStandalone pStack, @xStandalonePath
	local tBuildPlatform
	put GetBuildPlatform() into tBuildPlatform
	
	local tTargetFolder
	put GetTargetFolder() into tTargetFolder
	
	repeat for each item tPlatform in revSBDesktopTargets()
		if tPlatform is tBuildPlatform then
			set the cRevStandaloneSettings[tPlatform] of stack pStack to true
		else
			set the cRevStandaloneSettings[tPlatform] of stack pStack to false
		end if
	end repeat

	local tResult, tStandalonePaths
	revDoSaveAsStandalone pStack, tTargetFolder, , tStandalonePaths
	put the result into tResult

	put tStandalonePaths[tBuildPlatform] into xStandalonePath

	TestDiagnostic tResult	
	return tResult
end StandaloneBuilderSaveAsStandalone

function StandaloneBuilderResources pStack, pExecutablePath
	local tResources
	
	set the itemdelimiter to slash
	switch GetBuildPlatform()
	   case "MacOSX x86-64"
	   case "MacOSX x86-32"
	      put item 1 to -3 of pExecutablePath & "/Resources/_MacOS" into tResources
	      break
       default
          put item 1 to -2 of pExecutablePath into tResources
    end switch
    return tResources
end StandaloneBuilderResources

private command _TestCreateAndSaveStackAsStandalone pDescription, pStackId, pStackFilename, pScript, pSettingsA
   set the script of pStackId to pScript
   repeat for each key tKey in pSettingsA
      set the cRevStandaloneSettings[tKey] of pStackId to \
         pSettingsA[tKey]
   end repeat
	
   StandaloneBuilderTestSaveStackAsStandalone pDescription, \
      pStackFileName, pStackId
end _TestCreateAndSaveStackAsStandalone

command StandaloneBuilderTestCreateAndSaveStackAsStandalone pDescription, pStackFilename, pScript, pSettingsA
   create stack
   _TestCreateAndSaveStackAsStandalone pDescription, it, pStackFilename, pScript, pSettingsA
end StandaloneBuilderTestCreateAndSaveStackAsStandalone

command StandaloneBuilderTestCreateAndSaveScriptOnlyStackAsStandalone pDescription, pStackFilename, pScript, pSettingsA
   create script only stack
   _TestCreateAndSaveStackAsStandalone pDescription, it, pStackFilename, pScript, pSettingsA
end StandaloneBuilderTestCreateAndSaveScriptOnlyStackAsStandalone

command StandaloneBuilderTestSaveStackAsStandalone pDescription, pStackFilename, pStackID
   local tStackName
   put the short name of pStackID into tStackName   
   
   set the filename of pStackID to pStackFilename
   save stack tStackName as pStackFilename
   
   revIDESaveStack pStackID	

   local tExePath
   _TestBuildStandalone pStackFilename, tExePath
   if the result is not empty then
      TestAssert "building standalone", false
      exit StandaloneBuilderTestSaveStackAsStandalone
   end if
   
   TestAssert "building standalone", true
    
   TestDiagnostic "location" && tExePath
   
   TestAssert "standalone in expected location", there is a file tExePath
   
   local tResult, tShellCmd
   put quote & tExePath & quote into tShellCmd
   if the environment contains "command line" then
      put " -ui" after tShellCmd
   end if
   get shell(tShellCmd)
   put the result into tResult
   
   if tResult is not empty then
      TestDiagnostic "standalone quit with" && tResult & ":" && it
   end if
      
   TestAssert pDescription, tResult is empty
   
   TestDiagnostic revStandaloneGetWarnings()
end StandaloneBuilderTestSaveStackAsStandalone

private command _TestBuildStandalone pStackPath, @rStandalonePath
	local tStackName, tResult
	put the short name of stack pStackPath into tStackName
	
	TestDiagnostic "Building standalone -" && pStackPath
	
	StandaloneBuilderSaveAsStandalone tStackName, rStandalonePath
	return the result
end _TestBuildStandalone

-- Build a standalone purely to test whether the stack had the 
-- appropriate settings prior to deploy
command StandaloneBuilderTestSettings pDescription, pStackFileName, pInitialSettingsA, pExpectedSettingsA
   local tStackName, tStackID
   create stack
   put it into tStackID
   put the short name of tStackID into tStackName   
   
   repeat for each key tKey in pInitialSettingsA
      set the cRevStandaloneSettings[tKey] of tStackId to \
         pInitialSettingsA[tKey]
   end repeat   
   
   set the filename of tStackID to pStackFilename
   save stack tStackName
   
   revIDESaveStack tStackID	

   local tExePath
   _TestBuildStandalone pStackFilename, tExePath
   if the result is not empty then
      TestAssert "building standalone", false
      exit StandaloneBuilderTestSettings
   end if
   
   TestAssert "building standalone", true
   
   local tLastSettingsA
   put revStandaloneLastUsedSettings() into tLastSettingsA
   
   repeat for each key tKey in pExpectedSettingsA
      TestAssert pDescription && "-" && tKey, \
         pExpectedSettingsA[tKey] is tLastSettingsA[tKey]
   end repeat
end StandaloneBuilderTestSettings