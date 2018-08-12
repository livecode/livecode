script "TestRunner"
/*
Copyright (C) 2015-2016 LiveCode Ltd.

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

private function getTestRunnerBehavior
   -- Compute the filename of the test runner
   local tTestsPath, tStack
   set the itemdelimiter to "/"
   put item 1 to -2 of the effective filename of me into tTestsPath
   put tTestsPath & "/_testrunnerbehavior.livecodescript" into tStack
   return the long id of stack tStack
end getTestRunnerBehavior

on startup
   local tTestRunner
   put getTestRunnerBehavior() into tTestRunner
   set the behavior of me to tTestRunner
   send "TestRunnerMain" to me in 0
end startup

----------------------------------------------------------------
-- Top-level actions
----------------------------------------------------------------

command TestRunnerImpl_DoInvoke pInfo
   TestRunnerInvokeLoadLibraries pInfo
   invokeTest pInfo
end TestRunnerImpl_DoInvoke

command TestRunnerImpl_DoRun pInfo
   local tScript, tCommand, tAnalysis, tFolder
   if there is a file pInfo["args"][2] then
      put pInfo["args"][2] into tScript
      put pInfo["args"][3] into tCommand
   else if there is a folder pInfo["args"][2] then
      put pInfo["args"][2] into tFolder
   end if
   
   if tScript is empty then
      runAllScripts pInfo, tFolder
   else if tCommand is empty then
      runTestScript pInfo, tScript
   else
      runTestCommand pInfo, tScript, tCommand
   end if
   return the result for value
end TestRunnerImpl_DoRun

command TestRunnerImpl_DoUsage pStatus
   write "Usage: _testrunner.livecodescript run [SCRIPT [COMMAND]]" & return to stderr
   quit pStatus
end TestRunnerImpl_DoUsage

----------------------------------------------------------------
-- Support for invoking test commands
----------------------------------------------------------------

-- Execute a test
private command invokeTest pInfo
   local tScriptFile, tCommand

   -- This should auto-load the test script
   put pInfo["invoke"]["script"] into tScriptFile
   put pInfo["invoke"]["command"] into tCommand
   TestRunnerInvokeTestCommand tScriptFile, tCommand
end invokeTest

----------------------------------------------------------------
-- Support for running tests
----------------------------------------------------------------

-- Run all the test scripts that can be found below the current
-- directory
private command runAllScripts pInfo, pFolder
   local tFile, tAnalysis
   repeat for each element tFile in TesterGetTestFileNames(pFolder)
      if pFolder is not empty then
         put pFolder & slash before tFile
      end if
      runTestScript pInfo, tFile
      put TesterTapCombine(tAnalysis, the result) into tAnalysis
   end repeat
   TesterRunPrintSummary tAnalysis

   return tAnalysis
end runAllScripts

-- Run the tests found in one specific script file
private command runTestScript pInfo, pScriptFile
   local tCommand, tAnalysis

   repeat for each element tCommand in TesterParseTestCommandNames(pScriptFile)
      runTestCommand pInfo, pScriptFile, tCommand
      put TesterTapCombine(tAnalysis, the result) into tAnalysis
   end repeat
   return tAnalysis
end runTestScript

-- Run a specific named test command tCommand in a script file
-- tScriptFile
private command runTestCommand pInfo, pScriptFile, pCommand
   local tArg, tCommandLine

   repeat for each element tArg in pInfo["self-command"]
      put quote & tArg & quote & " " after tCommandLine
   end repeat

   put "invoke" && pScriptFile && pCommand after tCommandLine

   -- Invoke the test in a subprocess.  This ensures that we can detect
   -- if a crash occurs
   local tTestOutput, tTestExitStatus
   put shell(tCommandLine) into tTestOutput
   put the result into tTestExitStatus
   
   -- The output from the subprocesses will be native encoded utf-8.
   put textDecode(tTestOutput, "utf8") into tTestOutput

   -- Check the exit status.  If it suggests failure, add a "not ok" stanza
   -- to the tail of the TAP output
   if tTestExitStatus is not empty then
      put return after tTestOutput
      put "not ok # Subprocess exited with status" && \
            tTestExitStatus & return after tTestOutput
   end if

   TestRunnerProcessOutput pScriptFile, pCommand, tTestOutput
   return the result
end runTestCommand
