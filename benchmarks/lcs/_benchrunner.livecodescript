script "BenchRunner"
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
constant kLogFilename = "_benchmark_suite.log"

on startup
   send "BenchRunnerMain" to me in 0
end startup

----------------------------------------------------------------
-- Command-line processing
----------------------------------------------------------------

private function getCommandLineInfo
   local tRawArg, tSelfCommand, tSelfScript, tInArgs, tArgs
   
   put false into tInArgs
   
   -- Treat everything up to & including the first
   -- ".livecodescript" file as the command for running the bench
   -- runner, and everything after it as bench runner arguments
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

command BenchRunnerMain
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
end BenchRunnerMain

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
      runBenchScript pInfo, tScript
   end if
   
   put the result into tAnalysis
end doRun

private command doUsage pStatus
   write "Usage: _benchrunner.livecodescript run [SCRIPT]" & return to stderr
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
   
   -- Dispatch the test command
   dispatch pInfo["invoke"]["command"] to tStackName
end invokeTest

----------------------------------------------------------------
-- Support for running tests
----------------------------------------------------------------

-- Run all the test scripts that can be found below the current
-- directory
private command runAllScripts pInfo
   local tFile, tAnalysis
   repeat for each element tFile in runGetBenchFileNames()
      runBenchScript pInfo, tFile
      put the result after tAnalysis
   end repeat
   runPrintSummary tAnalysis
   
   -- Save the log to file
   open file kLogFilename for "UTF-8" text write
   write tAnalysis["log"] to file kLogFilename
   close file kLogFilename
   
   return tAnalysis
end runAllScripts

private command runBenchProcessOutput pScriptfile, pOutput
   -- Create test log
   local tTestLog
   put "###" && runGetPrettyBenchName(pScriptFile) \
         & return & return into tTestLog
   put pOutput & return after tTestLog
   
   return tTestLog
end runBenchProcessOutput

-- Run the benchmark found in one specific script file
private command runBenchScript pInfo, pScriptFile
   local tCommandLine
   
   -- Get the path to this engine
   put pInfo["self-command"][1] into tCommandLine
   
   -- Modify it so that it points to server-community instead
   set the itemDelimiter to slash
   if the platform is "macos" then
      put "server-community" into item -4 to -1 of tCommandLine
   else
      put "server-community" into item -1 of tCommandLine
   end if
   
   put " " & pScriptFile after tCommandLine
   
   -- Invoke the test in a subprocess.  This ensures that we can detect
   -- if a crash occurs
   local tBenchOutput, tBenchExitStatus
   put shell(tCommandLine) into tBenchOutput
   
   put the result into tBenchExitStatus
   write tBenchOutput to stdout
   -- Check the exit status.
   if tBenchExitStatus is not empty then
      put return after tBenchOutput
      put "FAILED" after tBenchOutput
   end if
   
   runBenchProcessOutput pScriptFile, tBenchOutput
   return the result
end runBenchScript

-- Get all livecode script files beneath the CWD, apart from
-- filenames starting with "." or "_"
private function runGetBenchFileNames
   local tFiles, tCount
   
   put empty into tFiles
   put 0 into tCount
   
   runGetBenchFileNames_Recursive the defaultfolder, empty, tFiles, tCount
   
   return tFiles
end runGetBenchFileNames

-- Helper command used by runGetBenchFileNames
private command runGetBenchFileNames_Recursive pPath, pRelPath, @xFiles, @xCount
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
      
      runGetBenchFileNames_Recursive tFolderPath, tFolder, xFiles, xCount
   end repeat
   
   -- Restore the CWD
   set the defaultfolder to tSaveFolder
end runGetBenchFileNames_Recursive

-- Prettify a test name by removing a ".livecodescript" suffix
private function runGetPrettyBenchName pFilename
   if pFilename ends with ".livecodescript" then
      set the itemDelimiter to "."
      return item 1 to -2 of pFileName
   end if
end runGetPrettyBenchName

-- Print out a table of statistics
private command runPrintSummary pAnalysis
end runPrintSummary

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
   /*if the platform is not "Linux" and the platform is not "MacOS" then
      put false into sLogInfo
   end if

   -- Check if colouring is possible
   local tTput
   put shell("tput colors") into tTput
   put the result
   if the result is not empty or tTput <= 8 then
      put false into sLogInfo
   end if

   -- Get colours
   put shell("tput sgr0") into sLogInfo["normal"]
   put shell("tput bold") into sLogInfo["bold"]
   put shell("tput setaf 1") into sLogInfo["red"]
   put shell("tput setaf 2") into sLogInfo["green"]
   put shell("tput setaf 3") into sLogInfo["yellow"]
   put shell("tput setaf 6") into sLogInfo["cyan"]*/
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

