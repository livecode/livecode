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
   
   local tStackFilename, tScript, tSettings
   if pWhich is "mainstack" then
	  put the folder & "/" & tDir & "/stack.livecodescript" into tStackFilename   
	  put _TestScriptOnlyStandaloneStackScript() into tScript
   else if pWhich is "auxiliary" then
   	  put the folder & "/" & tDir & "/stack.livecode" into tStackFilename   
	  put _TestScriptOnlyAuxiliaryStackStackScript() into tScript
	  put "Internet" into tSettings["scriptLibraries"]
	  put "select" into tSettings["inclusions"]
   end if
   
   local tDesc
   put "standalone with script-only" && pWhich && "startup" into tDesc
   StandaloneBuilderTestCreateAndSaveStackAsStandalone tDesc, \
      tStackFilename, tScript, tSettings

   revDeleteFolder tDir
end _TestScriptOnlyDeployStack

on TestScriptOnlyDeployStacks
   repeat for each item tItem in "mainstack,auxiliary"
      _TestScriptOnlyDeployStack tItem
   end repeat
end TestScriptOnlyDeployStacks