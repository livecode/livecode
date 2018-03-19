script "TestExtensionUtilsPackage"
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
along with LiveCode.  If not see <http://www.gnu.org/licenses/>.
*/

on TestSetup
   TestSkipIfNot "externals", "revzip,revxml"   
   TestLoadExternal "revzip"
   TestLoadExternal "revxml"   
   
   local tLibFilename
   put TestGetEngineRepositoryPath() & \
         "/extensions/script-libraries/extension-utils" & \ 
         "/extension-utils.livecodescript" into tLibFilename

   start using stack tLibFilename
end TestSetup

private function __PackageFile
   local tPackage
   set the itemdelimiter to slash
   put the effective filename of me into tPackage
   put "package/test.lce" into item -1 of tPackage
   return tPackage
end __PackageFile

constant kExtData = "name:com.livecode.library.test;author:LiveCode;version:0.0.0;type:library;title:Test"
on TestExtensionUtilsValidatePackage
   local tManifestData, tResult
   extensionValidateLCEPackage __PackageFile(), tManifestData
   put the result into tResult
   TestDiagnostic tResult
   
   TestAssert "valid package was validated", tResult is empty

   set the linedelimiter to ";"
   set the itemdelimiter to ":"
   repeat for each line tLine in kExtData
      TestAssert "package validation fetched" && item 1 of tLine && \
         "from manifest", tManifestData[item 1 of tLine] is \
         item 2 of tLine
   end repeat
end TestExtensionUtilsValidatePackage
