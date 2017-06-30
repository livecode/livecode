script "TestRunnerBehavior"
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

constant kDefaultLogFilename = "_test_suite.log"

----------------------------------------------------------------
-- Top-level actions
----------------------------------------------------------------

function getCommandLineInfo
   local tRawArg, tSelfCommand, tSelfScript, tInArgs, tArgs

   put false into tInArgs

   -- Treat everything up to & including the first
   -- ".livecodescript" file as the command for running the test
   -- runner, and everything after it as test runner arguments
   put the commandName into tSelfCommand[1]
   repeat for each element tRawArg in the commandArguments

      if tInArgs then
         put tRawArg into tArgs[1 + the number of elements in tArgs]
      else
         put tRawArg into tSelfCommand[1 + the number of elements in tSelfCommand]
         if tRawArg ends with ".livecodescript" then
            put tRawArg into tSelfScript
            put true into tInArgs
         end if
      end if

   end repeat

   local tInfo
   put tSelfCommand into tInfo["self-command"]
   put tSelfScript into tInfo["self-script"]
   put tArgs into tInfo["args"]

   return tInfo
end getCommandLineInfo

command TestRunnerMain
   local tInfo
   put getCommandLineInfo() into tInfo

   switch tInfo["args"][1]
      case "invoke"
         doInvoke tInfo
         break
      case "run"
         doRun tInfo
         break
      case "--help"
      case "-h"
      case "help"
         doUsage 0
         break
      default
         doUsage 1
         break
   end switch
   
   send "DoQuit" to me
end TestRunnerMain

on DoQuit
   -- Allow errorDialog message to be sent
   if "errorDialog" is in the pendingMessages then
 	  send "DoQuit" to me in 0 millisecs
   else
      quit 0
   end if
end DoQuit

private command doInvoke pInfo
   put pInfo["args"][2] into pInfo["invoke"]["script"]
   put pInfo["args"][3] into pInfo["invoke"]["command"]
   
   dispatch "TestRunnerImpl_DoInvoke" to me with pInfo
end doInvoke

private command doRun pInfo
   TestRunnerRunLoadLibraries

   local tAnalysis
   dispatch "TestRunnerImpl_DoRun" to me with pInfo
   put the result into tAnalysis
   
   -- Save the log to file.
   -- We process to binary data ourselves to ensure encoding and
   -- line endings are appropriate.
   local tLogForWriting
   put textEncode(tAnalysis["log"], "utf8") into tLogForWriting
   if the platform is "win32" then
      replace return with numToChar(13) & numToChar(10) in tLogForWriting
   end if
   
   local tLogFilename
   put $TEST_SUITE_LOG_FILE into tLogFilename
   if tLogFilename is empty then
      put kDefaultLogFilename into tLogFilename
   end if
   
   put tLogForWriting into url ("binfile:" & tLogFilename)
   
   if TesterTapGetWorstResult(tAnalysis) is "FAIL" then
      if $V is not empty then
         write tLogForWriting to stdout
   	  end if
      quit 1
   end if
end doRun

private command doUsage pStatus
   dispatch "TestRunnerImpl_DoUsage" to me with pStatus
end doUsage

private function getStackFolder
   local tFilename
   put the effective filename of this me into tFilename
   
   set the itemDelimiter to slash
   return item 1 to -2 of tFilename
end getStackFolder

private command TestRunnerLoadLibraries pLibraries
   local tLibrary, tStackName, tStackFile
   repeat for each item tLibrary in pLibraries
      -- Compute the filename of the library stack and load it
      put getStackFolder() & slash & tLibrary & ".livecodescript" into tStackFile
      put the name of stack tStackFile into tStackName
      
      send "revLoadLibrary" to stack tStackname
   end repeat
end TestRunnerLoadLibraries

----------------------------------------------------------------
-- Support for invoking test commands
----------------------------------------------------------------

-- Add the unit test library stack and the input library to the backscripts
constant kInvokeLibraries = "_testlib,_inputlib"
command TestRunnerInvokeLoadLibraries
   TestRunnerLoadLibraries kInvokeLibraries
end TestRunnerInvokeLoadLibraries

-- Execute a test
command TestRunnerInvokeTestCommand pScriptFile, pCommand
   local tStackName, tResult, tState
   
   put the mainstacks into tState["mainstacks"]
   put the openSockets into tState["opensockets"]
   put the openProcesses into tState["openprocesses"]
   put the openFiles into tState["openfiles"]
   
   reset the templateStack
   reset the templateCard
   reset the templateGroup
   reset the templateButton
   reset the templateField
   reset the templateImage
   reset the templateGraphic
   reset the templatePlayer
   
   set the tool to "browse"
   
   -- This should auto-load the test script
   put the name of stack pScriptFile into tStackName
   
   -- Check that the stack script actually compiles
   lock messages
   set the script of tStackname to the script of tStackname
   put the result into tResult
   unlock messages
   if tResult is not empty then
      TestDiagnostic tResult
   end if
   TestAssert "script compiles", tResult is empty
   
   -- Don't run test if script doesn't compile
   if tResult is not empty then
      exit TestRunnerInvokeTestCommand
   end if
   
   try
      dispatch "TestSetup" to tStackName
      put the result into tResult
   catch tError
      if word 1 of tError is "SKIP" then
         put tError into tResult
      else
         TestAssert "setup test", false
         TestDiagnostic tError   
      end if
   end try 
   
   if word 1 of tResult is "SKIP" then
      TestSkip pCommand, word 2 to -1 of tResult
   else
      -- Run the actual test itself
      try
         dispatch pCommand to tStackName
      catch tError
         if word 1 of tError is "SKIP" then
            TestSkip pCommand, word 2 to -1 of tError
         else
            TestAssert "execute test:" && line 1 of tError, false
            TestDiagnostic tError   
            -- don't exit here as test cleanup may still be useful
         end if
      end try
      
      -- Do common cleanup tasks
      try
         dispatch "TestTearDown" to tStackName
         
         -- cleanup
         delete tStackName
         
         local tNewState
         put the opensockets into tNewState
         repeat for each line tLine in tNewState
            if tLine is not among the lines of tState["opensockets"] then
               close socket tLine
            end if
         end repeat
         
         put the openprocesses into tNewState
         repeat for each line tLine in tNewState
            if tLine is not among the lines of tState["openprocesses"] then
               close process tLine
            end if
         end repeat
         
         put the openfiles into tNewState
         repeat for each line tLine in tNewState
            if tLine is not among the lines of tState["openfiles"] then
               close file tLine
            end if
         end repeat
         
         put the mainstacks into tNewState
         repeat for each line tLine in tNewState
            if tLine is not among the lines of tState["mainstacks"] then
               delete stack tLine
            end if
         end repeat
      catch tError
         TestAssert "tear down test", false
         TestDiagnostic tError  
      end try
      
      -- Check for uncaught errors
      put TestGetUncaughtErrorDialog() into tError
      if tError is not empty then
         TestAssert "uncaught error dialog", line 1 of tError
         TestDiagnostic tError
      end if

   end if
end TestRunnerInvokeTestCommand

----------------------------------------------------------------
-- Support for running tests
----------------------------------------------------------------

-- Add the test runner library stack to the backscripts
constant kRunLibraries = "_testerlib,_testlib"
command TestRunnerRunLoadLibraries
   TestRunnerLoadLibraries kRunLibraries
end TestRunnerRunLoadLibraries

command TestRunnerProcessOutput pScriptfile, pCommand, pOutput
   -- Create test log
   local tTestLog
   put "###" && TesterGetPrettyTestName(pScriptFile) && pCommand \
         & return & return into tTestLog
   put pOutput & return after tTestLog
   
   -- Analyse the results and print a summary line
   local tTapResults
   put TesterTapAnalyse(tTestLog) into tTapResults

   TesterLogSummaryLine tTapResults, (TesterGetPrettyTestName(pScriptFile) & ":" && pCommand)

   return tTapResults
end TestRunnerProcessOutput
