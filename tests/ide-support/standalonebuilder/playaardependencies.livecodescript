script "StandaloneExtensionsPlayAARDependencies"
/*
Copyright (C) 2019 LiveCode Ltd.

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
   __InstallExtension \
         "com.livecode.library.playaardependencytest-base_1_0_0", \
         "playaardependencytest-base_1_0_0.lcb"
   __InstallExtension \
         "com.livecode.library.playaardependencytest-base_2_0_0", \
         "playaardependencytest-base_2_0_0.lcb"
   __InstallExtension \
         "com.livecode.library.playaardependencytest-base_basement_1_0_0", \
         "playaardependencytest-base_basment_1_0_0.lcb"
   
   start using stack "revSBLibrary"
   
   local tSupportStack
   set the itemdelimiter to slash
   put the filename of me into tSupportStack
   put "_support.livecodescript" into item -1 of tSupportStack
   put tSupportStack into sSupportStack
   start using stack sSupportStack
   set the itemDel to comma
end TestSetup

on TestTeardown
   stop using stack sSupportStack
end TestTeardown

private function __GetExtFolder pExtName
   return TestGetEngineRepositoryPath() & slash & \
         "_tests/_build/packaged_extensions/" & pExtName
end __GetExtFolder

private command __InstallExtension pExtName, pExtSourceName
   revIDEExtensionLoad "lcb", pExtName, \
         __GetExtFolder(pExtName), "0.0.0", "installed", \
         "", "true", pExtSourceName
end __InstallExtension

on TestPlayAARSingleDependency
   local tDesc
   put "standalone with single play aar dependency" into tDesc
   
   local tStandaloneFolder
   set the itemdelimiter to slash
   put item 1 to -2 of the filename of me * slash & "__TestPlayAARIncusions" \
         into tStandaloneFolder   
   set the itemDel to comma
   
   -- Single extension with single play service dependency.
   local tExtensions
   put "com.livecode.library.playaardependencytest-base_1_0_0" into tExtensions
   
   local tExpectedSettingsA
   put "1.0.0" into tExpectedSettingsA["android,play dependencies"]["base"]
   
   create folder tStandaloneFolder
   __TestPlayAARDependencies tDesc, tStandaloneFolder, tExtensions, tExpectedSettingsA
   revDeleteFolder tStandaloneFolder
end TestPlayAARSingleDependency

on TestPlayAARMultipleDependencies
   local tDesc
   put "standalone with multiple play aar dependencies" into tDesc
   
   local tStandaloneFolder
   set the itemdelimiter to slash
   put item 1 to -2 of the filename of me * slash & "__TestPlayAARIncusions" \
         into tStandaloneFolder   
   set the itemDel to comma
   
   -- Single extension with multiple play service dependecies.
   local tExtensions
   put "com.livecode.library.playaardependencytest-base_basement_1_0_0" \
         into tExtensions
   
   local tExpectedSettingsA
   put "1.0.0" into tExpectedSettingsA["android,play dependencies"]["base"]
   put "1.0.0" into tExpectedSettingsA["android,play dependencies"]["basement"]
   
   create folder tStandaloneFolder
   __TestPlayAARDependencies tDesc, tStandaloneFolder, tExtensions, \
         tExpectedSettingsA
   revDeleteFolder tStandaloneFolder
end TestPlayAARMultipleDependencies

on TestPlayAARMultipleExtensions
   local tDesc
   put "standalone with multiple extensions with compatible play dependencies" \
         into tDesc
   
   local tStandaloneFolder
   set the itemdelimiter to slash
   put item 1 to -2 of the filename of me * slash & "__TestPlayAARIncusions" \
         into tStandaloneFolder   
   set the itemDel to comma
   
   -- Multiple extensions which depend on the same play service.
   local tExtensions
   put "com.livecode.library.playaardependencytest-base_1_0_0" & return & \
         "com.livecode.library.playaardependencytest-base_basement_1_0_0" \
         into tExtensions
   
   local tExpectedSettingsA
   put "1.0.0" into tExpectedSettingsA["android,play dependencies"]["base"]
   
   create folder tStandaloneFolder
   __TestPlayAARDependencies tDesc, tStandaloneFolder, tExtensions, \
         tExpectedSettingsA
   revDeleteFolder tStandaloneFolder
end TestPlayAARMultipleExtensions

on TestPlayAARVersionMismatch
   local tDesc
   put "standalone with multiple extensions with incompatible play dependencies" \
         into tDesc
   
   local tStandaloneFolder
   set the itemdelimiter to slash
   put item 1 to -2 of the filename of me * slash & "__TestPlayAARIncusions" \
         into tStandaloneFolder   
   set the itemDel to comma
   
   -- Multiple extensions which depend on the same play service but incompatible
   -- versions. Should result in an error.
   local tExtensions
   put "com.livecode.library.playaardependencytest-base_1_0_0" & return & \
         "com.livecode.library.playaardependencytest-base_2_0_0" \
         into tExtensions
   
   create folder tStandaloneFolder
   
   local tResult
   __TestPlayAARDependencies tDesc, tStandaloneFolder, tExtensions
   put the result into tResult
   TestAssert tDesc && ":" && tResult, tResult is not empty
   
   revDeleteFolder tStandaloneFolder
end TestPlayAARVersionMismatch

private command __TestPlayAARDependencies pDesc, pStandaloneFolder, pExtensions, pExpectedSettingA
   local tStackFilename
   put pStandaloneFolder & slash & "-standaloneplayaardependency.livecode" \
         into tStackFilename
   
   local tInitialSettingsA
   put pExtensions into tInitialSettingsA["extensions"]
   
   if pExpectedSettingA is empty then
      StandaloneBuilderTestSettings pDesc, tStackFilename, \
            tInitialSettingsA, pExpectedSettingA
      return the result
   else
      StandaloneBuilderBuildWithSettings pDesc, tStackFilename, \
            tInitialSettingsA
      return the result
   end if
end __TestPlayAARDependencies
