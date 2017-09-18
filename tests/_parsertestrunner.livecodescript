script "ParserTestRunner"
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
   -- Compute the filename of the test runner
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
   
   try
      CompilerTestRunnerMain
   catch tError
      write "ERROR: " & tError & return to stderr
   end try
end startup

----------------------------------------------------------------
-- Top-level actions
----------------------------------------------------------------

command CompilerTestRunner_DoUsage pStatus
   write "Usage: _parsertestrunner.livecodescript run [SCRIPT [COMMAND]]" & return to stderr
   quit pStatus
end CompilerTestRunner_DoUsage

private function DoCompile pScript
   local tCompilerOutput, tStackName
   lock messages
   create stack
   put the short name of it into tStackName
   set the script of it to pScript
   put the result into tCompilerOutput
   delete stack tStackName
   unlock messages
   return tCompilerOutput
end DoCompile

constant kExplicitVarsError = "error: test ambiguity with explicit vars"
command CompilerTestRunner_DoRunTest pTestInfo, @rOutput, @rExitStatus
   -- Attempt to parse the code from the file
   reportCompilerTestDiagWithLineNumbers pTestInfo["code"]

   local tCompilerOutput, tCompilerExitStatus
    
   -- Try to execute the compilation
   local tRestoreState
   repeat for each key tProp in pTestInfo["set"]
	  do "set the" && tProp && "to" && pTestInfo["set"][tProp]
   end repeat
   
   -- Ensure explicitvars is not significant if it hasn't been set 
   if "explicitvariables" is not among the keys of pTestInfo["set"] then
      local tExplicitOutput, tNonExplicitOutput
      set the explicitvars to true
      put DoCompile(pTestInfo["code"]) into tExplicitOutput
      set the explicitvars to false
      put DoCompile(pTestInfo["code"]) into tNonExplicitOutput
      if tExplicitOutput is tNonExplicitOutput then
         put tExplicitOutput into tCompilerOutput
      else 
         put kExplicitVarsError into tCompilerOutput
      end if      
   else
      put DoCompile(pTestInfo["code"]) into tCompilerOutput
   end if
   
   if tCompilerOutput is empty then
      put 0 into tCompilerExitStatus
   else
      put 1 into tCompilerExitStatus
   end if
   
   put tCompilerOutput into rOutput
   put tCompilerExitStatus into rExitStatus
end CompilerTestRunner_DoRunTest

function CompilerTestRunner_FileExtension
   return "parsertest"
end CompilerTestRunner_FileExtension

local sErrorMap
private command __BuildErrorMap
   if sErrorMap is not empty then
      exit __BuildErrorMap
   end if 
   
   try
      put TestBuildErrorMap("parseerrors.h", "PE", "UNDEFINED") into sErrorMap
   catch tError
      write "ERROR: " & tError & return to stderr
      quit 1
   end try
end __BuildErrorMap

function CompilerTestRunner_DoesOutputSatisfyAssertion pCompilerOutput, pCompilerExitCode, pAssertion, pTestInfo
   __BuildErrorMap
   
   if pCompilerOutput is kExplicitVarsError then
      return false
   end if
   
   if item 1 of pAssertion is "success" then
      return pCompilerExitCode is 0
   end if
   
   local tAssertionType, tErrorDescription, tAssertionPos
   put item 1 of pAssertion into tAssertionType
   put item 2 of pAssertion into tErrorDescription
   put item 3 of pAssertion into tAssertionPos
   
   local tOutputLine
   put line 1 of pCompilerOutput into tOutputLine
   
   -- The format of the error line is:
   --   <code>,<line>,<col>
   local tFile, tLine, tColumn, tCode, tMessage
   put format("%04d", item 1 of tOutputLine) into tCode
   put item 2 of tOutputLine into tLine
   put item 3 of tOutputLine into tColumn
   
   -- If the assertion type doesn't match, continue
   if tAssertionType is not "error" then
      return false
   end if
   
   -- If the assertion message is not within the message, continue
   if tErrorDescription is not sErrorMap[tCode] then
      return false
   end if
   
   -- If the position does not match, continue
   if pTestInfo["positions"][tAssertionPos] is not tLine then
      return false
   end if
   	 
   -- If we get here, we have a match so are successful!
   return true
end CompilerTestRunner_DoesOutputSatisfyAssertion
