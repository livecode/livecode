script "StandaloneExtensions"
/*
Copyright (C) 2017 LiveCode Ltd.

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

private function _TestScriptForInclusion
   set the itemdelimiter to slash
   local tExternalTest
   put the filename of me into tExternalTest
   put "/_widget.livecodescript" into item -1 of tExternalTest
   return textDecode(url("binfile:" & tExternalTest), "utf-8")
end _TestScriptForInclusion

on TestStandaloneInclusions
   local tDir
   set the itemdelimiter to slash
   set the defaultfolder to item 1 to -2 of the filename of me
   
   put "_TestSavingStandaloneExtensions" into tDir
   
   create folder tDir
   repeat for each key tWidget in (revIDEWidgets())
      _TestBuildStandaloneWithWidget tDir, tWidget
   end repeat
   
   revDeleteFolder tDir
end TestStandaloneInclusions

private command _TestBuildStandaloneWithWidget pDir, pWidget
   local tStackName, tStackID
   create stack
   put it into tStackID
   put the short name of tStackID into tStackName
   
   local tStackFilename
   put the folder & "/" & pDir & "/" & pWidget & "-standalone.livecode" into tStackFilename
   set the filename of tStackID to tStackFilename
   
   set the itemdelimiter to comma
   
   local tScript
   put _TestScriptForInclusion() into tScript
   
   set the script of tStackId to tScript
   
   set the cWidgetKind of tStackID to pWidget
   set the cRevStandaloneSettings["extensions"] of tStackID to pWidget
   
   local tDesc
   put "standalone with" && pWidget && "startup" into tDesc

   StandaloneBuilderTestSaveStackAsStandalone tDesc, \
      tStackFilename, tStackID
end _TestBuildStandaloneWithWidget
