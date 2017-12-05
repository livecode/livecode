script "StandaloneExtensionsJava"
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

local sSupportStack, sExtensionName
on TestSetup   
   TestSkipIfNot "jvm"
   put "com.livecode.library.javatest" into sExtensionName
   __InstallExtension sExtensionName
   
   start using stack "revSBLibrary"
		
   set the itemdelimiter to slash
   local tSupportStack
   put the filename of me into tSupportStack
   put "_support.livecodescript" into item -1 of tSupportStack
   put tSupportStack into sSupportStack
   start using stack sSupportStack
end TestSetup

private function __GetExtFolder
	return TestGetEngineRepositoryPath() & slash & \
	   "_tests/_build/packaged_extensions/" & sExtensionName
end __GetExtFolder

private command __InstallExtension
   revIDEExtensionLoad "lcb", sExtensionName, \
         __GetExtFolder(), "0.0.0", "installed", \
         "", "true", "javatest.lcb"
end __InstallExtension

on TestTeardown
  -- StandaloneBuilderCleanUpStandalones
   stop using stack sSupportStack
end TestTeardown

private function _TestScriptForInclusion
   set the itemdelimiter to slash
   local tExternalTest
   put the filename of me into tExternalTest
   put "/_java.livecodescript" into item -1 of tExternalTest
   return textDecode(url("binfile:" & tExternalTest), "utf-8")
end _TestScriptForInclusion

on TestStandaloneInclusions
   local tDir
   set the itemdelimiter to slash
   set the defaultfolder to item 1 to -2 of the filename of me
   
   put "_TestSavingStandaloneExtensions" into tDir
   
   create folder tDir
   _TestBuildStandaloneWithExtension tDir, sExtensionName
   
   revDeleteFolder tDir
end TestStandaloneInclusions

private command _TestBuildStandaloneWithExtension pDir, pExtension   
   local tStackFilename, tScript, tSettingsA
   put the folder & "/" & pDir & "/" & pExtension & "-standalone.livecode" into tStackFilename
   put _TestScriptForInclusion() into tScript
   put pExtension into tSettingsA["extensions"]
   put __GetExtFolder() & slash & "code/jvm/TestJar.jar" into \
      tSettingsA["jarFiles"]

   local tDesc
   put "standalone with" && pExtension && "startup" into tDesc
   StandaloneBuilderTestCreateAndSaveStackAsStandalone tDesc, \
      tStackFilename, tScript, tSettingsA
end _TestBuildStandaloneWithExtension
