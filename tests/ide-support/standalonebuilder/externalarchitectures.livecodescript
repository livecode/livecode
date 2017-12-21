script "ExternalArchitectures"
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

on TestSetup   
   start using stack "revSBLibrary"
end TestSetup

on TestTeardown
   
end TestTeardown

private function __ExtensionForPlatform pPlatform
   switch pPlatform
      case "Windows"
         return ".dll"
      case "Mac"
         return ".bundle"
      case "Linux"
         return ".so"
      case "iOS"
      case "Android"
         return ".lcext"
      default
         return empty
   end switch
end __ExtensionForPlatform

private function __ArchitectureSynonyms pArchitecture
   switch pArchitecture
      case "x86"
         return pArchitecture,"x86_32,x86-32,i386"
      case "x86_64"
         return pArchitecture,"x86-64,x64"
      case "armv6_hf"
         return pArchitecture,"armv6-hf"
   end switch
   
   return pArchitecture
end __ArchitectureSynonyms

command TestExternalNameFromFile
   repeat for each item tPlatform in revSBAllPlatforms()
      local tName
      put "External" & __ExtensionForPlatform(tPlatform) into tName
      TestAssert "External name found correctly for " & tName, revSBExternalNameFromFile(tPlatform, tName) is "External" 
      repeat for each item tArch in revSBPlatformArchitectures(tPlatform)
         repeat for each item tArchSynonym in __ArchitectureSynonyms(tArch) 
            put "External-" & tArchSynonym & __ExtensionForPlatform(tPlatform) into tName
            TestAssert "External name found correctly for " & tName, revSBExternalNameFromFile(tPlatform, tName) is "External"
            put "External_" & tArchSynonym & __ExtensionForPlatform(tPlatform) into tName
            TestAssert "External name found correctly for " & tName, revSBExternalNameFromFile(tPlatform, tName) is "External"
         end repeat
      end repeat
   end repeat
end TestExternalNameFromFile
