script "TestRunner"
/*
Copyright (C) 2015 Runtime Revolution Ltd.

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

-- FIXME provide this on the command line
constant kLogFilename = "_test_suite.log"

-- This is the message dispatched before invoking each test command
constant kSetupMessage = "TestSetup"
-- And this message is dispatched *after* each test command
constant kTeardownMessage = "TestTearDown"

on startup
   send "TestRunnerMain" to me in 0
end startup

----------------------------------------------------------------
-- Command-line processing
----------------------------------------------------------------

private function getCommandLineInfo
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

----------------------------------------------------------------
-- Top-level actions
----------------------------------------------------------------

command TestRunnerMain
   local tInfo
   put getCommandLineInfo() into tInfo
   logInit
   
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
   quit 0
end TestRunnerMain

private command doInvoke pInfo
   put pInfo["args"][2] into pInfo["invoke"]["script"]
   put pInfo["args"][3] into pInfo["invoke"]["command"]
   
   invokeLoadLibrary pInfo
   invokeTest pInfo
end doInvoke

private command doRun pInfo
   local tScript, tCommand, tAnalysis
   put pInfo["args"][2] into tScript
   put pInfo["args"][3] into tCommand
   if tScript is empty then
      runAllScripts pInfo
   else if tCommand is empty then
      runTestScript pInfo, tScript
   else
      runTestCommand pInfo, tScript, tCommand
   end if
   
   put the result into tAnalysis
   if tapGetWorstResult(tAnalysis) is "FAIL" then
      quit 1
   end if
end doRun

private command doUsage pStatus
   write "Usage: _testrunner.livecodescript run [SCRIPT [COMMAND]]" & return to stderr
   quit pStatus
end doUsage

on ErrorDialog pExecutionError
   write "ERROR:" && pExecutionError & return to stderr
   quit 1
end ErrorDialog

----------------------------------------------------------------
-- Support for invoking test commands
----------------------------------------------------------------

-- Execute a test
private command invokeTest pInfo
   local tStackName
   
   -- This should auto-load the test script
   put the name of stack pInfo["invoke"]["script"] into tStackName
   
   -- Dispatch the test setup command, and the test command itself
   dispatch kSetupMessage to tStackName
   dispatch pInfo["invoke"]["command"] to tStackName
   dispatch kTeardownMessage to tStackName
end invokeTest

-- Add the unit test library stack to the backscripts
private command invokeLoadLibrary pInfo
   local tStackName, tStackFile
   
   -- This should auto-load the library
   put invokeGetLibraryStack(pInfo) into tStackFile
   put the name of stack tStackFile into tStackName
   
   -- Add the library to the backscripts
   insert the script of stack tStackName into back
end invokeLoadLibrary

-- Return the filename of the unit test library stack
private function invokeGetLibraryStack pInfo
   local tFilename
   put pInfo["self-script"] into tFilename
   
   set the itemDelimiter to slash
   put "_testlib.livecodescript" into item -1 of tFilename
   
   return tFilename
end invokeGetLibraryStack

----------------------------------------------------------------
-- Support for running tests
----------------------------------------------------------------

-- Run all the test scripts that can be found below the current
-- directory
private command runAllScripts pInfo
   local tFile, tAnalysis
   repeat for each element tFile in runGetTestFileNames()
      runTestScript pInfo, tFile
      put tapCombine(tAnalysis, the result) into tAnalysis
   end repeat
   runPrintSummary(tAnalysis)
   
   -- Save the log to file
   open file kLogFilename for "UTF-8" text write
   write tAnalysis["log"] to file kLogFilename
   close file kLogFilename
   
   return tAnalysis
end runAllScripts

-- Output a skipped line in the results for eachcommand in the given test file
private command runTestSkipAll pInfo, pScriptFile, pReason
   local tSkipTemplate, tAnalysis
   put "ok - [[tCommand]] # SKIP" && pReason into tSkipTemplate

   repeat for each element tCommand in runGetTestCommandNames(pScriptFile)
      runTestProcessOutput pScriptFile, tCommand, merge(tSkipTemplate)
      put tapCombine(tAnalysis, the result) into tAnalysis
   end repeat
end runTestSkipAll

-- Run the tests found in one specific script file
private command runTestScript pInfo, pScriptFile
   local tCommand, tAnalysis, tMetadata
   put runGetTestMetadata(pScriptFile) into tMetadata
   
   -- Only run tests that require UI if possible
   local tCanRunUITests
   put true into tCanRunUITests
   repeat for each element tElement in pInfo["self-command"]
      if tElement is "-ui" then
         put false into tCanRunUITests
         exit repeat
      end if
   end repeat
   
   if tMetadata["ui"] is true and not tCanRunUITests then
      runTestSkipAll pInfo, pScriptFile, "test requires UI mode"
      return the result
   end if
   
   repeat for each element tCommand in runGetTestCommandNames(pScriptFile)
      runTestCommand pInfo, pScriptFile, tCommand
      put tapCombine(tAnalysis, the result) into tAnalysis
   end repeat
   return tAnalysis
end runTestScript

private command runTestProcessOutput pScriptfile, pCommand, pOutput
   -- Create test log
   local tTestLog
   put "###" && runGetPrettyTestName(pScriptFile) && pCommand \
         & return & return into tTestLog
   put pOutput & return after tTestLog
   
   -- Analyse the results and print a summary line
   local tTapResults
   put tapAnalyse(tTestLog) into tTapResults

   logSummaryLine tTapResults, (runGetPrettyTestName(pScriptFile) & ":" && pCommand)
   
   return tTapResults
end runTestProcessOutput

-- Run a specific named test command tCommand in a script file
-- tScriptFile
private command runTestCommand pInfo, pScriptFile, pCommand
   local tArg, tCommandLine
   
   repeat for each element tArg in pInfo["self-command"]
      put tArg & " " after tCommandLine
   end repeat
   
   put "invoke" && pScriptFile && pCommand after tCommandLine
   
   -- Invoke the test in a subprocess.  This ensures that we can detect
   -- if a crash occurs
   local tTestOutput, tTestExitStatus
   put shell(tCommandLine) into tTestOutput
   put the result into tTestExitStatus
   
   -- Check the exit status.  If it suggests failure, add a "not ok" stanza
   -- to the tail of the TAP output
   if tTestExitStatus is not empty then
      put return after tTestOutput
      put "not ok # Subprocess exited with status" && \
            tTestExitStatus & return after tTestOutput
   end if
   
   runTestProcessOutput pScriptFile, pCommand, tTestOutput
   return the result
end runTestCommand

-- Get all livecode script files beneath the CWD, apart from
-- filenames starting with "." or "_"
private function runGetTestFileNames
   local tFiles, tCount
   
   put empty into tFiles
   put 0 into tCount
   
   runGetTestFileNames_Recursive the defaultfolder, empty, tFiles, tCount
   
   return tFiles
end runGetTestFileNames

-- Helper command used by runGetTestFileNames
private command runGetTestFileNames_Recursive pPath, pRelPath, @xFiles, @xCount
   -- Save the CWD
   local tSaveFolder
   put the defaultfolder into tSaveFolder
   set the defaultfolder to pPath
   
   -- Process files in the current directory
   local tFile
   repeat for each line tFile in the files
      if tFile ends with ".livecodescript" and \
            not (tFile begins with "." or tFile begins with "_") then
         
         if pRelPath is not empty then
            put pRelPath & slash before tFile
         end if
         
         add 1 to xCount
         put tFile into xFiles[xCount]
      end if
   end repeat
   
   -- Process subdirectories
   local tFolder, tFolderPath
   repeat for each line tFolder in the folders
      if tFolder begins with "." then
         next repeat
      end if
      
      put pPath & slash & tFolder into tFolderPath
      
      if pRelPath is not empty then
         put pRelPath & slash before tFolder
      end if
      
      runGetTestFileNames_Recursive tFolderPath, tFolder, xFiles, xCount
   end repeat
   
   -- Restore the CWD
   set the defaultfolder to tSaveFolder
end runGetTestFileNames_Recursive

-- Get a number-indexed array contain the names of all "test"
-- commands in pFilename.
private function runGetTestCommandNames pFilename
   local tScript
   
   -- Get the contents of the file
   open file pFilename for "UTF-8" text read
   if the result is not empty then
      throw the result
   end if
   
   read from file pFilename until end
   put it into tScript
   
   close file pFilename
   
   -- Scan the file for "on Test*" definitions
   local tCommandNames, tCount, tLine, tName
   
   repeat for each line tLine in tScript
      if token 1 of tLine is not "on" then
         next repeat
      end if

      put token 2 of tLine into tName

      if not (tName begins with "Test") then
         next repeat
      end if

      -- Exclude the test setup message
      if tName is kSetupMessage or tName is kTeardownMessage then
         next repeat
      end if

      add 1 to tCount
      put tName into tCommandNames[tCount]
   end repeat
   
   return tCommandNames
end runGetTestCommandNames

-- Prettify a test name by removing a ".livecodescript" suffix
private function runGetPrettyTestName pFilename
   if pFilename ends with ".livecodescript" then
      set the itemDelimiter to "."
      return item 1 to -2 of pFileName
   end if
end runGetPrettyTestName

-- Print out a table of statistics
private command runPrintSummary pAnalysis
   local tSummaryString, tTotal, tDecoration
   
   put tapGetTestCount(pAnalysis) into tTotal
   
   -- Format basic summary information
   if pAnalysis["xfail"] is 0 and pAnalysis["fail"] is 0 then
      put "All" && tTotal && "tests passed" into tSummaryString
      
   else if pAnalysis["fail"] is 0 then
      put "All" && tTotal && "tests behaved as expected" into tSummaryString
      
   else
      put pAnalysis["fail"] && "OF" && tTotal && "TESTS FAILED" into tSummaryString
   end if
   
   put return after tSummaryString
   
   -- Add extra summary info from expected failure & skip directives
   if pAnalysis["xpass"] > 0 then
      put tab & pAnalysis["xpass"] && "unexpected passes" & return after tSummaryString
   end if
   if pAnalysis["xfail"] > 0 then
      put tab & pAnalysis["xfail"] && "expected failures" & return after tSummaryString
   end if
   if pAnalysis["skip"] > 0 then
      put tab & pAnalysis["skip"] && "skipped" & return after tSummaryString
   end if
   
   put "================================================================" into tDecoration
   put tDecoration & return before tSummaryString
   put tDecoration & return after tSummaryString
   
   write tSummaryString to stdout
end runPrintSummary

-- Get all the metadata keys and values in the stack file
private function runGetTestMetadata pFilename
   local tScript
   
   -- Get the contents of the file
   open file pFilename for "UTF-8" text read
   if the result is not empty then
      throw the result
   end if
   
   read from file pFilename until end
   put it into tScript
   
   close file pFilename
   
   -- Scan the file for metadata - 
   -- Metadata is given in the form ### <key> : <value>
   
   local tKey, tValue, tMetadata
   repeat for each line tLine in tScript
      -- save some time by only looking at commented out lines
      if token 1 of tLine is not empty then
         next repeat
      end if
      
      -- remove whitespace
      put word 1 to -1 of tLine into tLine
      
      if not (tLine begins with "###") then
         next repeat
      end if
      
      -- remove hashes
      put char 4 to -1 of tLine into tLine
      
      set the itemdelimiter to ":"
      put word 1 to -1 of (item 1 of tLine) into tKey
      put word 1 to -1 of (item 2 of tLine) into tValue
      
      put tValue into tMetadata[tKey]
   end repeat
   
   return tMetadata
end runGetTestMetadata

----------------------------------------------------------------
-- Logging helpers
----------------------------------------------------------------

local sLogInfo

-- Figure out what the highlighting escape codes are for the terminal
--
-- FIXME this really doesn't work properly if LiveCode's stdout
-- *isn't* a TTY.
private command logInit
   -- We can only do colour on Linux and OS X
   if the platform is not "Linux" and the platform is not "MacOS" then
      put false into sLogInfo
   end if

   -- Check if colouring is possible
   local tTput
   put shell("tput colors") into tTput
   if the result is not empty or tTput <= 8 then
      put false into sLogInfo
   end if

   -- Get colours
   put shell("tput sgr0") into sLogInfo["normal"]
   put shell("tput bold") into sLogInfo["bold"]
   put shell("tput setaf 1") into sLogInfo["red"]
   put shell("tput setaf 2") into sLogInfo["green"]
   put shell("tput setaf 3") into sLogInfo["yellow"]
   put shell("tput setaf 6") into sLogInfo["cyan"]
end logInit

private function logHighlight pString
  if pString is "fail" then
     return sLogInfo["red"] & sLogInfo["bold"] & pString & sLogInfo["normal"]
  else if pString is "xfail" or pString is "xpass" then
     return sLogInfo["yellow"] & pString & sLogInfo["normal"]
  else if pString is "pass" then
     return sLogInfo["green"] & pString & sLogInfo["normal"]
  else if pString is "skip" then
     return sLogInfo["cyan"] & pString & sLogInfo["normal"]
  else
     return pString
  end if
end logHighlight

private command logSummaryLine pTapResults, pTestName
   local tTotal, tPassed, tWorst, tMessage

   put pTapResults["xpass"] + pTapResults["pass"] + pTapResults["skip"] into tPassed
   put tapGetTestCount(pTapResults) into tTotal

   put tapGetWorstResult(pTapResults) into tWorst
   put logHighLight(the toUpper of tWorst) into tMessage
   put ":" after tMessage
   if the number of chars in tWorst < 5 then
      put space after tMessage
   end if

   put space & pTestName after tMessage

   put space & "[" & tPassed & "/" & tTotal & "]" after tMessage
   write tMessage & return to stdout
end logSummaryLine

----------------------------------------------------------------
-- TAP support
----------------------------------------------------------------

-- Scan TAP output and extract summary of test results
private function tapAnalyse pTapData
   local tStats
   put 0 into tStats["pass"]
   put 0 into tStats["xpass"]
   put 0 into tStats["xfail"]
   put 0 into tStats["fail"]
   put 0 into tStats["skip"]
   put pTapData into tStats["log"]
   
   local tLine, tIsOkay, tIsTodo, tIsSkip
   local tTail, tDirective
   repeat for each line tLine in pTapData
      -- Check if the line is a test result and, if so, whether
      -- the test was successful
      if token 1 of tLine is "ok" then
         put true into tIsOkay
      else if token 1 of tLine is "not" and token 2 of tLine is "ok" then
         put false into tIsOkay
      else
         -- No test result on this line
         next repeat          
      end if
      
      -- Check if the test on this line was skipped or was expected to fail
      put false into tIsTodo
      put false into tIsSkip
      put char (offset("#", tLine) + 1) to -1 of tLine into tTail
      switch token 1 of tTail
         case "TODO"
            put true into tIsTodo
            break
         case "SKIP"
            put true into tIsSkip
            break
      end switch
      
      if tIsOkay then
         if tIsTodo then
            add 1 to tStats["xpass"]
         else if tIsSkip then
            add 1 to tStats["skip"]
         else
            add 1 to tStats["pass"]
         end if
      else
         if tIsTodo then
            add 1 to tStats["xfail"]
         else
            add 1 to tStats["fail"]
         end if
      end if
      
   end repeat
   
   return tStats
end tapAnalyse

-- Examine the statistics generated by tapAnalyse() and return a string
-- describing the test log's worst result
private function tapGetWorstResult pTapAnalysis
   local tResult
   repeat for each item tResult in "fail,xpass,xfail,pass,skip"
      if pTapAnalysis[tResult] > 0 then
         return tResult
      end if
   end repeat
   return "pass"
end tapGetWorstResult

-- Get the total number of tests from the statistics generated by
-- tapAnalyse()
private function tapGetTestCount pAnalysis
   return pAnalysis["pass"] + pAnalysis["xpass"] + pAnalysis["xfail"] + \
         pAnalysis["fail"] + pAnalysis["skip"]
end tapGetTestCount

-- Combine two sets of TAP results
private function tapCombine pAnalysisA, pAnalysisB
   local tCombined, tKey
   repeat for each key tKey in pAnalysisB
      if tKey is "log" then
         put pAnalysisA[tKey] & pAnalysisB[tKey] into tCombined[tKey]
      else
         put pAnalysisA[tKey] + pAnalysisB[tKey] into tCombined[tKey]
      end if
   end repeat
   return tCombined
end tapCombine
