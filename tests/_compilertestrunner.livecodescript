script "CompilerTestRunner"
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

private function getTestRunnerBehavior
   -- Compute the filename of the compiler test runner behavior
   local tTestsPath, tStack
   set the itemdelimiter to "/"
   put item 1 to -2 of the effective filename of me into tTestsPath
   put tTestsPath & "/_compilertestrunnerbehavior.livecodescript" into tStack
   return the long id of stack tStack
end getTestRunnerBehavior

on startup
   local tTestRunner
   put getTestRunnerBehavior() into tTestRunner
   set the behavior of me to tTestRunner
   send "CompilerTestRunnerMain" to me in 0
end startup

----------------------------------------------------------------
-- Top-level actions
----------------------------------------------------------------

command CompilerTestRunner_ValidateEnvironment
   if $LC_COMPILE is empty then
      return "LC_COMPILE environment var must be set to lc-compile to use"
   end if
   return empty
end CompilerTestRunner_ValidateEnvironment

command CompilerTestRunner_DoUsage pStatus
   write "Usage: _compilertestrunner.livecodescript run [SCRIPT [COMMAND]]" & return to stderr
   quit pStatus
end CompilerTestRunner_DoUsage

command CompilerTestRunner_DoRunTest pTestInfo, @rOutput, @rExitStatus
   -- Emit the code for the test, then attempt to compile it
   local tTestFile, tTestInterfaceFile
   put tempName() into tTestFile
   put tempName() into tTestInterfaceFile
   put textEncode(pTestInfo["code"], "utf8") into url ("binfile:" & tTestFile)
   reportCompilerTestDiag format("output test source file to '%s'", tTestFile)
   reportCompilerTestDiagWithLineNumbers pTestInfo["code"]
   
   -- Build the command line
   local tCompilerCmdLine
   put format("%s --interface %s %s", $LC_COMPILE, tTestInterfaceFile, tTestFile) into tCompilerCmdLine
   reportCompilerTestDiag format("compile command is %s", tCompilerCmdLine)

   -- Try to execute the compilation
   local tCompilerOutput, tCompilerExitStatus
   put shell(tCompilerCmdLine) into tCompilerOutput
   put the result into tCompilerExitStatus
   reportCompilerTestDiag "compile command output"

   -- Remove the test file
   delete file tTestFile
   delete file tTestInterfaceFile

   -- The output from the subprocesses will be native encoded utf-8.
   put textDecode(tCompilerOutput, "utf8") into tCompilerOutput
   put tCompilerOutput into rOutput
   put tCompilerExitStatus into rExitStatus
end CompilerTestRunner_DoRunTest

function CompilerTestRunner_FileExtension
   return "compilertest"
end CompilerTestRunner_FileExtension

function CompilerTestRunner_DoesOutputSatisfyAssertion pCompilerOutput, pCompilerExitCode, pAssertion, pTestInfo
   if item 1 of pAssertion is "success" then
      return pCompilerExitCode is 0
   end if

   local tAssertionType, tAssertionPartialMsg, tAssertionPos
   put item 1 of pAssertion into tAssertionType
   put item 2 of pAssertion into tAssertionPartialMsg
   put item 3 of pAssertion into tAssertionPos

   -- Trim off any quotes around the partial message
   if char 1 of tAssertionPartialMsg is quote then
      delete char 1 of tAssertionPartialMsg
      delete char -1 of tAssertionPartialMsg
   end if

   set the itemDelimiter to ":"

   local tState
   put "search" into tState
   repeat for each line tOutputLine in pCompilerOutput
      local tFile, tLine, tColumn, tType, tMessage
      local tSourceLine, tCharOffset, tOutputChar, tMarker

      if tState is "search" then
         -- The format of each output line is:
         --   <file>:<line>:<col>: (error|warning): <msg>
         put item 1 of tOutputLine into tFile
         put item 2 of tOutputLine into tLine
         put item 3 of tOutputLine into tColumn
         put word 1 of item 4 of tOutputLine into tType
         put item 5 to -1 of tOutputLine into tMessage

         -- If the assertion type doesn't match, continue
         if tAssertionType is not tType then
            next repeat
         end if

         -- If the assertion message is not within the message, continue
         if tAssertionPartialMsg is not in tMessage then
            next repeat
         end if

         -- If the position does not match, continue
         if pTestInfo["positions"][tAssertionPos] is not tLine then
            next repeat
         end if

         -- If we get here, we have a match so are successful!
         put "source line" into tState

      else if tState is "source line" then
         -- The format of the source line output is a space followed
         -- by the corresponding line of the input source file.  The
         -- compiler may replace any of the source characters with
         -- spaces (for example, to remove control characters).
         
         put " " & line tLine of pTestInfo["code"] into tSourceLine
         
         repeat with tCharOffset = 1 to the number of chars in tOutputLine
            put char tCharOffset of tOutputLine into tOutputChar
            if tOutputChar is not " " and \
                  tOutputChar is not char tCharOffset of tSourceLine then
               return false
            end if
         end repeat

         put "marker" into tState

      else if tState is "marker" then
         -- The marker should point at the correct column within the
         -- input source line

         -- TODO the marker position should come from the position
         -- indicated in the code block, not from the position
         -- reported by the compiler.
         put format(merge("%[[tColumn]]s^"),"") into tMarker

         if tOutputLine is tMarker then
            return true
         else
            return false
         end if
      end if
   end repeat
   
   -- We failed to find a suitable output line, so failure
   return false
end CompilerTestRunner_DoesOutputSatisfyAssertion
