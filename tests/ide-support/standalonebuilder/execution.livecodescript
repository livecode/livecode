script "StandaloneBuilding"
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

local sSupportStack
on TestSetup   
   start using stack "revSBLibrary"
		
   set the itemdelimiter to slash
   local tSupportStack
   put the filename of me into tSupportStack
   put "_support.livecodescript" into item -1 of tSupportStack
   put tSupportStack into sSupportStack
   start using stack sSupportStack
end TestSetup

on TestTeardown
   StandaloneBuilderCleanUpStandalones
   stop using stack sSupportStack
end TestTeardown

constant kResourcesFolders = "Sample Projects,Examples"
on TestBuildStandalone
	local tBase
	put revIDESpecialFolderPath("IDE") & "/Resources" into tBase
	repeat for each item tFolder in kResourcesFolders
		_TestBuildStandalonesOfFolder tBase & slash & tFolder
	end repeat
end TestBuildStandalone

private command _TestBuildStandalonesOfFolder pFolder
	local tFiles
	revIDEPushDefaultFolder pFolder
	put the files into tFiles
	TestDiagnostic "files:" && tFiles
	repeat for each line tFile in tFiles
		if revIDEFileHasStackfileExtension(tFile) then
			_TestBuildStandalone pFolder & slash & tFile
		end if
	end repeat
	
	revIDEPopDefaultFolder
end _TestBuildStandalonesOfFolder

private command _TestBuildStandalone pStackPath
	local tStackName, tResult
	put the short name of stack pStackPath into tStackName
	
	TestDiagnostic "Building standalone -" && pStackPath
	
	local tStandalonePath
	StandaloneBuilderSaveAsStandalone tStackName, tStandalonePath

	put the result into tResult
	
	TestAssert tStackName && "- builds successfully", tResult is empty
	
	TestAssert tStackName && "- no warnings", revStandaloneGetWarnings() is empty
end _TestBuildStandalone