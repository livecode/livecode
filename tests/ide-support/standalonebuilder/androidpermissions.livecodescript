script "StandaloneExtensionsAndroidPermissions"
/*
Copyright (C) 2018 LiveCode Ltd.

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
   put "com.livecode.library.androidpermissionstest" into sExtensionName
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
         "", "true", "androidpermissionstest.lcb"
end __InstallExtension

on TestTeardown
   --StandaloneBuilderCleanUpStandalones
   stop using stack sSupportStack
end TestTeardown

on TestStandaloneAndroidPermissionsSettings   
   local tDir
   set the itemdelimiter to slash
   set the defaultfolder to item 1 to -2 of the filename of me
   
   put "_TestSavingStandaloneExtensions" into tDir
   
   create folder tDir
   _TestStandalonePermissionsWithExtension tDir, sExtensionName
   
   revDeleteFolder tDir
end TestStandaloneAndroidPermissionsSettings  

private command _TestStandalonePermissionsWithExtension pDir, pExtension
   local tStackFilename
   put the folder & "/" & pDir & "/" & pExtension & "-standalonepermissions.livecode" into tStackFilename
   
   local tInitialSettingsA
   put pExtension into tInitialSettingsA["extensions"]
   
   local tExpectedSettingsA
   put true into \
      tExpectedSettingsA["android,application permissions"]["TEST"]
   put true into \
      tExpectedSettingsA["android,application permissions"]["TESTTWO"]
   put true into \
      tExpectedSettingsA["android,device capabilities"] \
      ["hardware.testrequired"]
   put false into \
      tExpectedSettingsA["android,device capabilities"] \
      ["hardware.testnotrequired"]      
   
   local tDesc
   put "standalone with" && pExtension && "permissions" into tDesc

   StandaloneBuilderTestSettings tDesc, tStackFilename, \
      tInitialSettingsA, tExpectedSettingsA
end _TestStandalonePermissionsWithExtension
