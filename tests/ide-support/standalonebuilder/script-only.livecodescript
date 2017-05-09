script "ScriptOnlyDeploy"
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

private function _TestScriptOnlyStandaloneStackScript
	local tScript
	put "on startup" & return after tScript
	put "quit 0" & return after tScript
	put "end startup" & return after tScript
	return tScript
end _TestScriptOnlyStandaloneStackScript

private function _TestScriptOnlyAuxiliaryStackStackScript
	local tScript
	put "on openStack" & return after tScript
	put "if (there is a stack" && quote & "revLibUrl" & quote \
		& ") then quit 0" & return after tScript
	put "write" && quote & "included aux stack not present" & quote \ 
	   && "to stdout" & return after tScript
	put "quit 1" & return after tScript
	put "end openStack" & return after tScript
	return tScript
end _TestScriptOnlyAuxiliaryStackStackScript

private command _TestScriptOnlyDeployStack pWhich
   local tDir
   set the itemdelimiter to slash
   set the defaultfolder to item 1 to -2 of the filename of me

   put "_TestSavingStandalone" into tDir
   
   create folder tDir
   
   local tStackName, tStackID

   local tStackFilename
   if pWhich is "mainstack" then
      create script only stack
   	  put it into tStackID
      put the short name of tStackID into tStackName
	  put the folder & "/" & tDir & "/stack.livecodescript" into tStackFilename   
	  set the script of tStackID to _TestScriptOnlyStandaloneStackScript()
   else if pWhich is "auxiliary" then
      create stack
   	  put it into tStackID
      put the short name of tStackID into tStackName
   	  put the folder & "/" & tDir & "/stack.livecode" into tStackFilename   
	  set the script of tStackID to _TestScriptOnlyAuxiliaryStackStackScript()
	  set the cRevStandaloneSettings["scriptLibraries"] of tStackID to "Internet"
	  set the cRevStandaloneSettings["inclusions"] of tStackID to "select"
   end if
   
   set the filename of tStackID to tStackFilename
   save stack tStackName as tStackFilename
   
   revIDESaveStack tStackID	

   _TestBuildStandalone tStackFilename
   if the result is not empty then
      TestAssert "building standalone", false
      exit _TestScriptOnlyDeployStack
   end if
   
   TestAssert "building standalone", true
   
   local tExe
   put StandaloneBuilderExecutable(tStackName) into tExe   
   TestDiagnostic "location" && tExe
   
   TestAssert "standalone in expected location", there is a file tExe
   
   local tResult, tShellCmd
   put quote & tExe & quote into tShellCmd
   if the environment contains "command line" then
      put " -ui" after tShellCmd
   end if
   get shell(tShellCmd)
   put the result into tResult
   
   if tResult is not empty then
      TestDiagnostic "standalone quit with" && tResult & ":" && it
   end if
      
   TestAssert "standalone with script-only" && pWhich && "startup", \
         tResult is empty

   revDeleteFolder tDir
end _TestScriptOnlyDeployStack

private command _TestBuildStandalone pStackPath
	local tStackName, tResult
	put the short name of stack pStackPath into tStackName
	
	TestDiagnostic "Building standalone -" && pStackPath
	
	StandaloneBuilderSaveAsStandalone tStackName
	return the result
end _TestBuildStandalone

on TestScriptOnlyDeployStacks
   repeat for each item tItem in "mainstack,auxiliary"
      _TestScriptOnlyDeployStack tItem
   end repeat
end TestScriptOnlyDeployStacks