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

private function GetBuildPlatform
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
	return TestGetIDERepositoryPath() & "/_tests/_build"
end GetTargetFolder

on StandaloneBuilderCleanupStandalones 
	local tTargetFolder
	put GetTargetFolder() into tTargetFolder
	revDeleteFolder tTargetFolder	
end StandaloneBuilderCleanupStandalones

on StandaloneBuilderSaveAsStandalone pStack, @rPath
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

	local tResult
	revDoSaveAsStandalone pStack, tTargetFolder
	put the result into tResult

	put StandaloneBuilderExecutable(pStack) into rPath

	TestDiagnostic tResult	
	return tResult
end StandaloneBuilderSaveAsStandalone

function StandaloneBuilderExecutable pStack	
	local tBuildPlatform
	put GetBuildPlatform() into tBuildPlatform
	
	local tTargetFolder
	put GetTargetFolder() & slash into tTargetFolder
		
	local tStandaloneExecutable
	put	revSBStandaloneOutputDirectory(pStack, "", tTargetFolder, tBuildPlatform, false) \
		& pStack into tStandaloneExecutable

	if GetBuildPlatform() contains "MacOSX" then 
        put ".app/Contents/MacOS/" & pStack after tStandaloneExecutable
    end if
 
	return tStandaloneExecutable
end StandaloneBuilderExecutable

function StandaloneBuilderResources pStack
	local tExe, tResources
	put StandaloneBuilderExecutable(pStack) into tExe
	
	set the itemdelimiter to slash
	switch GetBuildPlatform()
	   case "MacOSX x86-64"
	   case "MacOSX x86-32"
	      put item 1 to -3 of tExe & "/Resources/_MacOS" into tResources
	      break
       default
          put item 1 to -2 of tExe into tResources
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
end StandaloneBuilderTestSaveStackAsStandalone

private command _TestBuildStandalone pStackPath, @rStandalonePath
	local tStackName, tResult
	put the short name of stack pStackPath into tStackName
	
	TestDiagnostic "Building standalone -" && pStackPath
	
	StandaloneBuilderSaveAsStandalone tStackName, rStandalonePath
	return the result
end _TestBuildStandalone
