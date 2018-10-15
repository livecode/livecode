script "StandaloneStackfiles"
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

local sSupportStack, sInsideFolder, sOutsideFolder
constant kRelativeToMeInside = "_build"
constant kRelativeToMeOutside = "../../../_tests/_build/StandaloneStackfiles"
on TestSetup   
   start using stack "revSBLibrary"
		
   set the itemdelimiter to slash
   local tFolder
   put item 1 to -2 of the filename of me into tFolder
   
   put tFolder & slash & "_support.livecodescript" into sSupportStack
   start using stack sSupportStack
   
   put tFolder & slash & kRelativeToMeInside into sInsideFolder
   if there is not a folder sInsideFolder then
      create folder sInsideFolder
   end if
   
   put tFolder & slash & kRelativeToMeOutside into sOutsideFolder
   if there is not a folder sOutsideFolder then
      create folder sOutsideFolder
   end if
   
   if there is not a folder (sInsideFolder & slash & "stackfiles") then
      create folder (sInsideFolder & slash & "stackfiles")
   end if
   
   	_TestSetupStacks
end TestSetup

on TestTeardown
   StandaloneBuilderCleanUpStandalones
   stop using stack sSupportStack
   revDeleteFolder sInsideFolder
   revDeleteFolder sOutsideFolder
   _TestTeardownStacks
end TestTeardown

private function _RelativeFolderInside pFilename
   return "../" & kRelativeToMeInside
end _RelativeFolderInside

private function _RelativeFolderOutside
   return "../" & kRelativeToMeOutside
end _RelativeFolderOutside

private function _AbsoluteFolderInside
   return sInsideFolder
end _AbsoluteFolderInside

private function _AbsoluteFolderOutside
   return sOutsideFolder
end _AbsoluteFolderOutside

private command _TestSetupStacks
   create stack "relativeInside"
   set the filename of it to sInsideFolder & slash & "stackfiles/relativeInside.livecode"
   save stack "relativeInside"

   create stack "relativeOutside"
   set the filename of it to sOutsideFolder & slash & "relativeOutside.livecode"
   save stack "relativeOutside"
   
   create stack "absoluteInside"
   set the filename of it to sInsideFolder & slash & "stackfiles/absoluteInside.livecode"
   save stack "absoluteInside"
   
   create stack "absoluteOutside"
   set the filename of it to sOutsideFolder & slash & "absoluteOutside.livecode"
   save stack "absoluteOutside"

   local tStackName, tStackID
   create stack "main"
   put it into tStackID
   set the stackfiles of tStackID to tStackfiles
   set the filename of tStackID to sInsideFolder & slash & "main.livecode"
   save stack "main"
end _TestSetupStacks

private command _TestTeardownStacks
   delete stack "relativeInside"
   delete stack "relativeOutside"
   delete stack "absoluteInside"
   delete stack "absoluteOutside"
   delete stack "main"
end _TestTeardownStacks

on TestStandaloneStackFileInclusion
	--_TestStandaloneStackPath "stackfiles"
end TestStandaloneStackFileInclusion

on TestStandaloneCopyFilesInclusion
	_TestStandaloneStackPath "copy files"
end TestStandaloneCopyFilesInclusion

private function _GetStackFiles
   local tStackfiles
   put "relativeInside,stackfiles/relativeInside.livecode" into tStackfiles
   put return & "relativeOutside," & _RelativeFolderOutside() & "/relativeOutside.livecode" after tStackfiles
   put return & "absoluteInside," & _AbsoluteFolderInside() & "/stackfiles/absoluteInside.livecode" after tStackfiles
   put return & "absoluteOutside," & _AbsoluteFolderOutside() & "/absoluteOutside.livecode" after tStackfiles
   return tStackfiles
end _GetStackFiles

private function _GetCopyFiles
   local tStackfiles
   put "stackfiles/relativeInside.livecode" into tStackfiles
   put return & _RelativeFolderOutside() & "/relativeOutside.livecode" after tStackfiles
   put return & _AbsoluteFolderInside() & "/stackfiles/absoluteInside.livecode" after tStackfiles
   put return & _AbsoluteFolderOutside() & "/absoluteOutside.livecode" after tStackfiles
   return tStackfiles
end _GetCopyFiles

private command _TestStandaloneStackPath pWhich
   if pWhich is "stackfiles" then
      set the stackfiles of stack "main" to _GetStackFiles()
   else
      set the cRevStandaloneSettings["files"] of stack "main" to _GetCopyFiles()
   end if
   save stack "main"
   
   local tStandalonePath
   StandaloneBuilderSaveAsStandalone "main", tStandalonePath

   if the result is not empty then
      TestAssert "building standalone", false
      exit _TestStandaloneStackPath
   end if
   
   TestAssert "building standalone", true
   
   local tResources
   put StandaloneBuilderResources("main", tStandalonePath) into tResources
   
   TestDiagnostic tResources
   TestDiagnostic files(tResources)
   TestDiagnostic folders(tResources)
   
   TestAssert "preserve folder structure of relative file in" && pWhich \
      && "when under stack path", there is a file (tResources & \ 
      "/stackfiles/relativeInside.livecode")

   if pWhich is "stackfiles" then
      TestAssert "preserve folder structure of absolute file in" && pWhich \
         && "when under stack path", there is a file (tResources & \ 
         "/stackfiles/absoluteInside.livecode")

      TestAssert "discard folder structure of relative file in" && pWhich \
         && "when not under stack path", there is a file (tResources & \ 
         "/relativeOutside.livecode")
   else
      TestAssertBroken "preserve folder structure of absolute file in" && pWhich \
         && "when under stack path", there is a file (tResources & \ 
         "/stackfiles/absoluteInside.livecode"), "Bug 19002"

      TestAssertBroken "discard folder structure of relative file in" && pWhich \
         && "when not under stack path", there is a file (tResources & \ 
         "/relativeOutside.livecode"), "Bug 19002"
   end if 

   TestAssert "discard folder structure of absolute file in" && pWhich \
      && "when not under stack path", there is a file (tResources & \ 
      "/absoluteOutside.livecode")
   
end _TestStandaloneStackPath
