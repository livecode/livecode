script "StandaloneInclusions"
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

private function _TestScriptForInclusion pWhich
   set the itemdelimiter to slash
   local tExternalTest
   put the filename of me into tExternalTest
   put "/externals/" & pWhich & ".livecodescript" into item -1 of tExternalTest
   return textDecode(url("binfile:" & tExternalTest), "utf-8")
end _TestScriptForInclusion

on TestStandaloneInclusions
   local tDir
   set the itemdelimiter to slash
   set the defaultfolder to item 1 to -2 of the filename of me
   
   put "_TestSavingStandalone" into tDir
   
   create folder tDir
   
   local tStackFilename
   put the folder & "/" & tDir & "/stack.livecode" into tStackFilename
   
   set the itemdelimiter to comma
   repeat for each item tItem in "xml,security,zip,sqlite"
      _TestStandaloneWithInclusion tStackFilename, tItem
   end repeat
   
   revDeleteFolder tDir
end TestStandaloneInclusions

private command _TestStandaloneWithInclusion pStackFilename, pItem
   local tScript
   put _TestScriptForInclusion(pItem) into tScript

   local tDesc
   put "standalone with" && pItem && "inclusion startup" into tDesc

   StandaloneBuilderTestCreateAndSaveStackAsStandalone tDesc, \
      pStackFilename, tScript
end _TestStandaloneWithInclusion
