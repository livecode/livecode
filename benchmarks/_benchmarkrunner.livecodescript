script "BenchmarkRunner"
/*
Copyright (C) 2015 LiveCode Ltd.

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

-- This is the message dispatched before invoking each benchmark command
constant kSetupMessage = "BenchmarkSetup"
-- And this message is dispatched *after* each benchmark command
constant kTeardownMessage = "BenchmarkTearDown"

on startup
   send "BenchmarkRunnerMain" to me in 0
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
   local tCommandArguments
   if the version begins with "6.7" then
      put $0 into tSelfCommand[1]
      if the environment is "command line" then
         put "-ui" into tCommandArguments[1]
      end if
      repeat with i = 1 to $#
         put value("$" & i) into tCommandArguments[the number of elements in tCommandArguments + 1]
      end repeat
   else
      do "put the commandName into tSelfCommand[1]"
      do "put the commandArguments into tCommandArguments"
   end if
   repeat for each element tRawArg in tCommandArguments
      
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

command BenchmarkRunnerMain
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
   quit 0
end BenchmarkRunnerMain

private command doInvoke pInfo
   put pInfo["args"][2] into pInfo["invoke"]["script"]
   put pInfo["args"][3] into pInfo["invoke"]["command"]

   invokeLoadLibrary pInfo
   invokeBenchmark pInfo
end doInvoke

private command doRun pInfo
   local tScript, tCommand, tLog
   put pInfo["args"][2] into tScript
   put pInfo["args"][3] into tCommand
   if tCommand is not empty then
      runSingleCommand pInfo, tScript, tCommand
   else if tScript is not empty and there is no folder tScript then
      runSingleScript pInfo, tScript
   else
      runAllScripts pInfo, tScript
   end if
   
   put the result into tLog
   
   -- Save the log to file
   local tLogToWrite
   put tLog into tLogToWrite
   if the platform is "win32" then
      replace return with numToChar(13) & numToChar(10) in tLogToWrite
   end if
   put tLogToWrite into url ("binfile:" & kLogFilename)
end doRun

private command doUsage pStatus
   write "Usage: _benchmarkrunner.livecodescript run [SCRIPT [COMMAND]]" & return to stderr
   quit pStatus
end doUsage

on errorDialog pExecutionError
   write "ERROR:" && pExecutionError & return to stderr
   quit 1
end errorDialog

----------------------------------------------------------------
-- Support for invoking test commands
----------------------------------------------------------------

-- Execute a benchmark
private command invokeBenchmark pInfo
   local tStackName

   -- This should auto-load the test script
   put the name of stack pInfo["invoke"]["script"] into tStackName

   -- Dispatch the test setup command, and the test command itself
   dispatch kSetupMessage to tStackName
   dispatch pInfo["invoke"]["command"] to tStackName
   dispatch kTeardownMessage to tStackName
end invokeBenchmark

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
   put "_benchmarklib.livecodescript" into item -1 of tFilename

   return tFilename
end invokeGetLibraryStack

----------------------------------------------------------------
-- Support for running tests
----------------------------------------------------------------

-- Run all the benchmark scripts that can be found below the current
-- directory
private command runAllScripts pInfo, pFolder
   local tFile, tLog
   repeat for each element tFile in runGetBenchmarkFileNames(pFolder)
      runSingleScript pInfo, tFile
      put the result after tLog
   end repeat

   return tLog
end runAllScripts

-- Run the benchmarks found in one specific script file
private command runSingleScript pInfo, pScriptFile
   local tCommand, tLog, tMetadata

   repeat for each element tCommand in runGetBenchmarkCommandNames(pScriptFile)
      runSingleCommand pInfo, pScriptFile, tCommand
      put the result after tLog
   end repeat
   
   return tLog
end runSingleScript 

-- Run a specific named benchmark command tCommand in a script file
-- tScriptFile
private command runSingleCommand pInfo, pScriptFile, pCommand
   local tBenchmarkName
   put char 10 to -1 of pCommand into tBenchmarkName
   write "Running " & tBenchmarkName & "..." & return to stdout
   
   local tArg, tCommandLine
   repeat for each element tArg in pInfo["self-command"]
      put tArg & " " after tCommandLine
   end repeat
   
   put "invoke" && pScriptFile && pCommand after tCommandLine
   
   -- Invoke the test in a subprocess.  This ensures that we can detect
   -- if a crash occurs
   local tBenchmarkTimes, tBenchmarkExitStatus
   put shell(tCommandLine) into tBenchmarkTimes
   put the result into tBenchmarkExitStatus
   
   -- The output from the shell will be a list of tab separated lines.
   --   <variantname> tab <timetaken>
   
   -- Check the exit status and process for logging.
   if tBenchmarkExitStatus is not empty then
      write tab & "failed" & return to stdout
      write tBenchmarkTimes & return to stdout
      return tBenchmarkName & tab & "failed" & return
   end if

   filter tBenchmarkTimes without "*TIS/TSM*"
   
   local tLog
   set the itemDelimiter to tab
   repeat for each line tResult in tBenchmarkTimes
      if item 1 of tResult is not empty then
         write tab & item 1 of tResult & space & item 2 of tResult & " ms" & return to stdout
      else
         write tab & item 2 of tResult & " ms" & return to stdout
      end if
      put tBenchmarkName & tab & tResult & return after tLog
   end repeat
   
   return tLog
end runSingleCommand

-- Get all livecode script files beneath the CWD, apart from
-- filenames starting with "." or "_"
private function runGetBenchmarkFileNames pFolder
   local tFiles, tCount

   put empty into tFiles
   put 0 into tCount

   runGetBenchmarkFileNames_Recursive the defaultfolder & slash & pFolder, pFolder, tFiles, tCount

   return tFiles
end runGetBenchmarkFileNames

-- Helper command used by runGetTestFileNames
private command runGetBenchmarkFileNames_Recursive pPath, pRelPath, @xFiles, @xCount
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

      runGetBenchmarkFileNames_Recursive tFolderPath, tFolder, xFiles, xCount
   end repeat

   -- Restore the CWD
   set the defaultfolder to tSaveFolder
end runGetBenchmarkFileNames_Recursive

-- Get a number-indexed array contain the names of all "test"
-- commands in pFilename.
private function runGetBenchmarkCommandNames pFilename
   local tScript

   -- Get the contents of the file
   put url ("file:" & pFilename) into tScript

   -- Scan the file for "on Benchmark*" definitions
   local tCommandNames, tCount, tLine, tName

   repeat for each line tLine in tScript
      if token 1 of tLine is not "on" then
         next repeat
      end if

      put token 2 of tLine into tName

      if not (tName begins with "Benchmark") then
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
end runGetBenchmarkCommandNames

-- Prettify a test name by removing a ".livecodescript" suffix
private function runGetPrettyBenchmarkName pFilename
   if pFilename ends with ".livecodescript" then
      set the itemDelimiter to "."
      return item 1 to -2 of pFileName
   end if
end runGetPrettyBenchmarkName
