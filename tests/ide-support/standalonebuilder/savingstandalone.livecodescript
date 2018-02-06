script "SavingStandaloneMessage"
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

private function _TestSavingStandaloneStackScript
	local tScript
	put "on startup" & return after tScript
	put "write the cModified of me to stdout" & return after tScript
	put "quit 0" & return after tScript
	put "end startup" & return after tScript
	put "on savingStandalone" & return after tScript
	put "set the cModified of me to true" & return after tScript
	put "end savingStandalone" & return after tScript
	put "on standaloneSaved pFolder" & return after tScript
	put "set the cFolder of me to pFolder" & return after tScript
	put "end standaloneSaved" & return after tScript

	return tScript
end _TestSavingStandaloneStackScript

on TestSavingStandalone
   local tDir
   set the itemdelimiter to slash
   set the defaultfolder to item 1 to -2 of the filename of me

   put "_TestSavingStandalone" into tDir
   
   create folder tDir
   
   local tStackName, tStackID
   create stack
   put it into tStackID
   put the short name of tStackID into tStackName

   local tStackFilename
   put the folder & "/" & tDir & "/stack.livecode" into tStackFilename
   set the filename of tStackID to tStackFilename
   
   set the script of tStackID to _TestSavingStandaloneStackScript()
   
   set the cModified of tStackID to false
   
   revIDESaveStack tStackID

   local tExe
   _TestBuildStandalone tStackFilename, tExe

   if the result is not empty then
      TestAssert "building standalone", false
      exit TestSavingStandalone
   end if

   TestAssert "building standalone", true
   TestAssert "standalone state reloaded", the cModified of stack tStackName is not true

   
   TestDiagnostic "location" && tExe
   TestAssert "standalone in expected location", there is a file tExe
   
   local tOutput
   put shell(quote & tExe & quote && "-ui") into tOutput
   
   TestDiagnostic "shell output" && tOutput
   
   TestAssert "savingStandalone side-effect present in standalone", \ 
   		tOutput is "true"
   
   revDeleteFolder tDir
end TestSavingStandalone

private command _TestBuildStandalone pStackPath, @rPath
	local tStackName, tResult
	put the short name of stack pStackPath into tStackName
	
	TestDiagnostic "Building standalone -" && pStackPath
	
	StandaloneBuilderSaveAsStandalone tStackName, rPath
	return the result
end _TestBuildStandalone