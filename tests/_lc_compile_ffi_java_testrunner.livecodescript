script "LCCompileFFIJavaTestRunner"
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
   if $LC_COMPILE_FFI_JAVA is empty then
      return "LC_COMPILE_FFI_JAVA environment var must be set to lc-compile-ffi-java to use"
   end if
   return empty
end CompilerTestRunner_ValidateEnvironment

private command CompilerTestRunner_DoUsage pStatus
   write "Usage: _lc_compile_ffi_java_testrunner.livecodescript run [SCRIPT [COMMAND]]" & return to stderr
   quit pStatus
end CompilerTestRunner_DoUsage

----------------------------------------------------------------
-- Support for running tests
----------------------------------------------------------------

command CompilerTestRunner_DoRunTest pTestInfo, @rOutput, @rExitStatus
   -- Next we emit the code for the test, then attempt to compile it
   local tTestFile, tTestInterfaceFile
   put tempName() into tTestFile
   put tempName() into tTestInterfaceFile
   put textEncode(pTestInfo["code"], "utf8") into url ("binfile:" & tTestFile)
   reportCompilerTestDiag format("output test source file to '%s'", tTestFile)
   reportCompilerTestDiagWithLineNumbers pTestInfo["code"]
   
   -- Build the command line
   local tJavaDSLParserCmdLine
   put format("%s --check %s %s", $LC_COMPILE_FFI_JAVA, tOutputFile, tTestFile) into tJavaDSLParserCmdLine
   reportCompilerTestDiag format("compile command is %s", tJavaDSLParserCmdLine)

   -- Try to execute the compilation
   local tJavaDSLParserOutput, tJavaDSLParserExitStatus
   put shell(tJavaDSLParserCmdLine) into tJavaDSLParserOutput
   put the result into tJavaDSLParserExitStatus
   reportCompilerTestDiag "parse command output"

   -- If the parse was successful, check the operation is idempotent
   if tJavaDSLParserExitStatus is 0 then
   	  local tFirstOutput, tSecondOutput, tIsIdempotent
	  put textDecode(url("binfile:" & tOutputFile), "utf8") into tFirstOutput
      put format("%s --check %s %s", $LC_COMPILE_FFI_JAVA, tOutputFile, tTestFile) into tJavaDSLParserCmdLine
      local tJavaDSLParserSecondOutput, tJavaDSLParserSecondExitStatus
      put shell(tJavaDSLParserCmdLine) into tJavaDSLParserSecondOutput
   	  put the result into tJavaDSLParserSecondExitStatus
   	  if tJavaDSLParserSecondExitStatus is not 0 then
   	     put false into tIsIdempotent
   	  else
   	     put textDecode(url("binfile:" & tOutputFile), "utf8") into tSecondOutput
   	     put tFirstOutput is tSecondOutput into tIsIdempotent
   	  end if
   	  
   	  if tIsIdempotent then
         put "ok - " after tTestOutput
      else
         put "not ok - " after tTestOutput
      end if
      put "parse is idempotent" & return after tTestOutput
   end if

   -- Remove the test file
   delete file tTestFile
   delete file tTestInterfaceFile

   -- The output from the subprocesses will be native encoded utf-8.
   put textDecode(tJavaDSLParserOutput, "utf8") into tJavaDSLParserOutput
   put tJavaDSLParserOutput into rOutput
   put tJavaDSLParserExitStatus into rExitStatus
end CompilerTestRunner_DoRunTest

function CompilerTestRunner_FileExtension
   return "javabindtest"
end CompilerTestRunner_FileExtension

function CompilerTestRunner_DoesOutputSatisfyAssertion pJavaDSLParserOutput, pJavaDSLParserExitCode, pAssertion, pInfo
   if item 1 of pAssertion is "success" then
      return pJavaDSLParserExitCode is 0
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
   repeat for each line tOutputLine in pJavaDSLParserOutput
      local tFile, tLine, tColumn, tType, tMessage

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
      if pInfo["positions"][tAssertionPos] is not tLine then
         next repeat
      end if

      -- If we get here, we have a match so are successful!
      return true
   end repeat

   -- We failed to find a suitable output line, so failure
   return false
end CompilerTestRunner_DoesOutputSatisfyAssertion
