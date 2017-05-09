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

on StandaloneBuilderSaveAsStandalone pStack	
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