script "StandaloneTestRunner"

/*
Copyright (C) 2017 LiveCode Ltd.

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

local sTestFiles
local sTestFile
local sTestsRoot
local sAnalysis
local sTestRootFile
local sCommandNames
local sCommand

constant kDefaultLogFilename = "_test_suite.log"

local sLogFile

on startup
   if there is a stack $1 then
      -- minimal support for spawning a child process via TestRunStack
      dispatch "startup" to stack $1
      
      -- just in case it doesn't quit itself
      quit 0
   end if
   
   put the folder & slash & kDefaultLogFilename into sLogFile
   open file sLogFile for "utf8" text  write
   
   set the behavior of me to the long id of stack (specialFolderPath("resources") & slash & "tests" & slash & "_testrunnerbehavior.livecodescript")
   
   TestRunnerInvokeLoadLibraries
   TestOutputToVariable
   TestSetExecErrorMap the uErrorMap of me
   
   put specialFolderPath("resources") & slash & "tests" into sTestsRoot
   
   start using stack (sTestsRoot & slash & "_testerlib.livecodescript")
   
   -- Run tests
   put TesterGetTestFileNames(sTestsRoot) into sTestFiles
   
   if the platform is among the items of "iOS,Android" then
      TesterLogSetCallbackURL the uCallbackURL of this stack
   end if
   
   DoNextFile
   pass startup
end startup

on shutdown
   close file sLogFile
   pass shutdown
end shutdown

on DoNextCommand
   if the number of elements of sCommandNames is 0 then
      DoNextFile
      exit DoNextCommand
   end if
   
   local tIndex
   put item 1 of the extents of sCommandNames into tIndex
   put sCommandNames[tIndex] into sCommand
   delete variable sCommandNames[tIndex]
   
   TestRunnerInvokeTestCommand sTestFile, sCommand
   
   local tOutput
   put TestFetchAndClearOutput() into tOutput
   
   replace numToChar(13) with numToChar(10) in tOutput
   
   write tOutput & return to file sLogFile
   
   TestRunnerProcessOutput sTestRootFile, sCommand, tOutput
   put TesterTapCombine(sAnalysis, the result) into sAnalysis
   
   send "DoNextCommand" to me in 0 milliseconds
end DoNextCommand

on DoNextFile
   if the number of elements of sTestFiles is 0 then
      TesterRunPrintSummary sAnalysis
      
      local tExitCode
      if sAnalysis["fail"] is not 0 then
         put 2 into tExitCode
      else
         put 0 into tExitCode
      end if
      
      if the platform is among the items of "iOS,Android" then
         post "^exit^" & tExitCode to url (the uCallbackURL of this stack)
      end if
      
      quit tExitCode
   end if
   
   local tIndex
   put item 1 of the extents of sTestFiles into tIndex
   put sTestFiles[tIndex] into sTestFile
   delete variable sTestFiles[tIndex]
   
   if sTestFile contains "ide-support" then
      DoNextFile
      exit DoNextFile
   end if
   
   -- Ensure that the test stack filename is absolute
   if the first char of sTestFile is not slash then
      put sTestsRoot & slash before sTestFile
   end if
   
   put sTestFile into sTestRootFile
   delete char 1 to length(sTestsRoot & slash) of sTestRootFile
   
   put TesterParseTestCommandNames(sTestFile) into sCommandNames
   
   DoNextCommand
end DoNextFile


