script "LcRunTestRunner"
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

/*
These tests check basic lc-run command-line functionality.  The LCB
test suite, which runs in lc-run, is an in-depth test of most of
lc-run's capabilities, but doesn't exercise all of the ways that
lc-run can be invoked.

Only add tests here that can't be written as an LCB test.
*/

on startup
   try
      LcRunTestMain
   catch tError
      write "ERROR: " & tError & return to stderr
      quit 1
   end try
   quit 0
end startup

on LcRunTestMain
   loadLibrary
   
   -- Sanity check
   if $LC_RUN is empty then
      throw "LC_RUN environment variable must be set"
   end if
   if $LCM_DIR is empty then
      throw "LCM_DIR environment variable must be set"
   end if
   
   if not (buildAssembly("_libs.lca", "_lib1.lcm,_lib2.lcm") and \
         buildAssembly("_all.lca", "_main.lcm,_libs.lca")) then
      throw "Failed to construct bytecode assemblies"
   end if
   
   local tSuccess
   put true into tSuccess
   
   -- Test basic functionality
   put tSuccess and \
         doRunTest("basic", "_lib1.lcm,_lib2.lcm", "_main.lcm", "12") into tSuccess
   
   -- Test lib assembly
   put tSuccess and \
         doRunTest("libraries assembly", "_libs.lca", "_main.lcm", "12") into tSuccess
   
   -- Test full assembly
   put tSuccess and \
         doRunTest("full assembly", empty, "_all.lca", "12") into tSuccess
   
   if tSuccess then
      quit 0
   else
      quit 1
   end if   
end LcRunTestMain

-- Construct a bytecode assembly by concatenating a bunch of LCB
-- module files
private function buildAssembly pAssembly, pInputs
   local tAssemblyData, tInput
   
   repeat for each item tInput in pInputs
      put url ("binfile:" & getBuildPath(tInput)) after tAssemblyData
   end repeat
   
   put tAssemblyData into url ("binfile:" & getBuildPath(pAssembly))
   
   return true
end buildAssembly

private function doRunTest pName, pLibs, pMain, pExpectedOutput
   local tCommand, tLib
   put $LC_RUN into tCommand
   if pLibs is not empty then
      repeat for each item tLib in pLibs
         put getBuildPath(tLib) into tLib
         put merge(" -l [[tLib]]") after tCommand
      end repeat
   end if
   put " " & getBuildPath(pMain) after tCommand
   
   local tOutput, tExitStatus
   put shell(tCommand) into tOutput
   put the result into tExitStatus
   
   if tExitStatus is not 0 then
      write tOutput & return to stdout
      TesterLog "fail", merge("[[pName]] (exited: [[tExitStatus]])")
      return false
   else if tOutput is not pExpectedOutput then
      write tOutput & return to stdout
      TesterLog "fail", merge("[[pName]] (incorrect output)")
      return false
   else
      TesterLog "pass", pName
      return true
   end if
end doRunTest

private function getBuildPath pBaseName
   return $LCM_DIR & "/" & pBaseName
end getBuildPath

on loadLibrary   
   -- Find tester library filename...
   local tTesterLibFile
   set the itemDelimiter to "/"
   put the filename of me into tTesterLibFile
   put "_testerlib.livecodescript" into item -1 of tTesterLibFile
   
   -- ...and load the stack
   local tStackName
   put the name of stack tTesterLibFile into tStackName
   
   send "revLoadLibrary" to stack tStackName
end loadLibrary
