script "CompilerTestRunnerBehavior"
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
   put item 1 to -2 of the effective filename of this me into tTestsPath
   put tTestsPath & "/_testrunnerbehavior.livecodescript" into tStack
   return the long id of stack tStack
end getTestRunnerBehavior

on CompilerTestRunnerMain
   local tTestRunner
   put getTestRunnerBehavior() into tTestRunner
   set the behavior of this me to tTestRunner
   send "TestRunnerMain" to me in 0
end CompilerTestRunnerMain

command TestRunnerImpl_DoRun pInfo
   local tResult
   dispatch "CompilerTestRunner_ValidateEnvironment" to me
   put the result into tResult
   if tResult is not empty then
      throw tResult
   end if 
   
   local tScript, tCommand
   put pInfo["args"][2] into tScript
   put pInfo["args"][3] into tCommand
   if tScript is empty then
      runAllScripts pInfo
   else if tCommand is empty then
      runCompilerTestScript pInfo, tScript
   else
      runCompilerTest pInfo, tScript, tCommand
   end if
   return the result for value
end TestRunnerImpl_DoRun

command TestRunnerImpl_DoUsage pStatus
   dispatch "CompilerTestRunner_DoUsage" to me with pStatus
end TestRunnerImpl_DoUsage

----------------------------------------------------------------
-- Support for running tests
----------------------------------------------------------------

-- Run all the test scripts that can be found below the current
-- directory
private command runAllScripts pInfo
   local tFile, tAnalysis, tExtension
   dispatch function "CompilerTestRunner_FileExtension" to me
   put the result into tExtension
   repeat for each element tFile in TesterGetTestFileNames("", tExtension)
      runCompilerTestScript pInfo, tFile
      put TesterTapCombine(tAnalysis, the result) into tAnalysis
   end repeat
   TesterRunPrintSummary tAnalysis
   return tAnalysis
end runAllScripts

-- Run the tests found in one specific script file
private command runCompilerTestScript pInfo, pScriptFile
   local tTest, tAnalysis

   repeat for each element tTest in listCompilerTestsInFile(pScriptFile)
      runCompilerTest pInfo, pScriptFile, tTest
      put TesterTapCombine(tAnalysis, the result) into tAnalysis
   end repeat
   return tAnalysis
end runCompilerTestScript

command CompilerRunTestProcessOutput pScriptfile, pCommand, pOutput
   -- Create test log
   local tTestLog
   put "###" && TesterGetPrettyTestName(pScriptFile, "compilertest") && pCommand \
         & return & return into tTestLog
   put pOutput & return after tTestLog

   -- Analyse the results and print a summary line
   local tTapResults
   put TesterTapAnalyse(tTestLog) into tTapResults

   TesterLogSummaryLine tTapResults, (TesterGetPrettyTestName(pScriptFile, "compilertest") & ":" && pCommand)

   return tTapResults
end CompilerRunTestProcessOutput

-- Run a specific named test command tCommand in a script file
-- tScriptFile
private command runCompilerTest pInfo, pScriptFile, pTest
   local tCommandLine
   
   -- First we need to process the specified test in script file

   local tTestInfo
   try
      processCompilerTest pInfo, pScriptFile, pTest, tTestInfo
   catch tError
      write tError & return to stderr
      quit 1
   end try

   local tCompilerOutput, tCompilerExitStatus
   dispatch "CompilerTestRunner_DoRunTest" to me with tTestInfo, tCompilerOutput, tCompilerExitStatus
   reportCompilerTestDiag tCompilerOutput
   
   -- Process the assertions and make sure each one matches up
   local tTestOutput
   repeat for each line tAssertion in tTestInfo["assertions"]
      dispatch function "CompilerTestRunner_DoesOutputSatisfyAssertion" to me \
            with tCompilerOutput, tCompilerExitStatus, tAssertion, tTestInfo
      if the result is true then
         put "ok - " after tTestOutput
      else
         put "not ok - " after tTestOutput
      end if
      put compilerPrettyPrintAssertion(tAssertion, tTestInfo["positions"]) after tTestOutput
      if tTestInfo["expectation"] is not "PASS" then
         if tTestInfo["expectation"] is "FAIL" then
            put " # TODO" after tTestOutput
         else if tTestInfo["expectation"] is "SKIP" then
            put " # SKIP" after tTestOutput
         end if
         if tTestInfo["reason"] is not empty then
            put " " & tTestInfo["reason"] after tTestOutput
         end if
      end if
      put return after tTestOutput
   end repeat
   
   CompilerRunTestProcessOutput pScriptFile, pTest, tTestOutput
   return the result
end runCompilerTest

----------------------------------------------------------------
-- Support for generating the compiler tests
----------------------------------------------------------------

-- Get a number-indexed array containing the names of all "test"
-- blocks in pFilename.
private function listCompilerTestsInFile pFilename
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
   local tTestNames, tCount, tLine, tName

   repeat for each line tLine in tScript
      if word 1 of tLine is not "%TEST" then
         next repeat
      end if

      put word 2 of tLine into tName

      add 1 to tCount
      put tName into tTestNames[tCount]
   end repeat

   return tTestNames
end listCompilerTestsInFile

private function compilerPrettyPrintAssertion pAssertion, pPositions
   if item 1 of pAssertion is "success" then
      return "compile succeeded"
   end if

   if item 1 of pAssertion is among the items of "error,warning" then
      return format("%s %s at line %d", item 1 of pAssertion, item 2 of pAssertion, pPositions[item 3 of pAssertion])
   end if

   return "UNKNOWN ASSERTION TYPE"
end compilerPrettyPrintAssertion

private command processCompilerTest pInfo, pScriptFile, pTest, @rCompilerTest
   -- Get the contents of the file
   open file pScriptFile for "UTF-8" text read
   if the result is not empty then
      throw format("can't open file '%s'", pScriptFile)
   end if
   
   local tScript
   read from file pScriptFile until end
   put it into tScript
   
   close file pScriptFile
   
   -- First extract the test block we are wanting to run
   local tLine, tLineNumber, tTestLineNumber, tState
   local tName, tCode, tExpectation, tExpectationReason, tAssertions, tPositions, tSet, tSetCount
   put empty into tLine
   put 0 into tLineNumber
   put 0 into tTestLineNumber
   put empty into tState
   put empty into tName
   put empty into tCode
   put empty into tExpectation
   put empty into tExpectationReason
   put empty into tAssertions
   put empty into tPositions
   put empty into tSet
   put 0 into tSetCount
   repeat for each line tLine in tScript
      add 1 to tLineNumber
      
      local tIsDirective
      put false into tIsDirective
      if tState is "code" then
         if word 1 of tLine begins with "%" and \
               not (word 1 of tLine begins with "%{") then
            put true into tIsDirective
         end if
      else
         if word 1 of tLine begins with "%" then
            put true into tIsDirective
         end if
      end if
      
      if tIsDirective then
         local tToken
         put word 1 of tLine into tToken
         
         if tToken is "%%" then
            -- We don't allow comment directives inside code blocks
            if tState is "code" then
               reportCompilerTestError pScriptFile, tLineNumber, "comment directives not allowed in code block"
            end if
            -- Ignore comment lines
            next repeat
         else if tToken is "%TEST" then
            -- If we are already in a test block, it is an error
            if tState is not empty then
               reportCompilerTestError pScriptFile, tLineNumber, "%TEST directive not allowed inside test block"
            end if
            
            -- We've started a new test clause, so record the name and move to
            -- code state.
            reportCompilerTestDiag format("found test '%s' at line %d", word 2 of tLine, tLineNumber)
            put word 2 of tLine into tName
            put tLineNumber + 1 into tTestLineNumber
            put empty into tCode
            put empty into tExpectation
            put empty into tAssertions
            put empty into tPositions
            put "set" into tState
            put 0 into tSetCount
         else if tToken is "%EXPECT" then
            -- We only allow %EXPECT directives after code blocks
            if tState is not "code" then
               reportCompilerTestError pScriptFile, tLineNumber, "%EXPECT directive only allowed after code block"
            end if
            
            -- If the expectation isn't one of PASS, FAIL, XFAIL, SKIP then it
            -- is an error
            if word 2 of tLine is not among the items of "PASS,FAIL,SKIP" then
               reportCompilerTestError pScriptFile, tLineNumber, format("invalid expectation '%s'", word 2 of tLine)
            end if
            
            if word 3 of tLine contains "#" then
               reportCompilerTestError pScriptFile, tLineNumber, format("expectation reason cannot contain '#'")
            end if
            
            -- Record the expectation, and move to assertions state.
            put word 2 of tLine into tExpectation
            put word 3 of tLine into tExpectationReason
            if char 1 of tExpectationReason is quote then
               delete char 1 of tExpectationReason
               delete char -1 of tExpectationReason
            end if
            put "assertions" into tState
         else if tToken is among the items of "%SUCCESS,%WARNING,%ERROR" then
            if tState is not "assertions" then
               reportCompilerTestError pScriptFile, tLineNumber, "assertion directive outside of assertion block"
            end if
            
            if tToken is "%SUCCESS" then
               if the number of words in tLine is not 1 then
                  reportCompilerTestError pScriptFile, tLineNumber, "incorrect syntax, expected: %SUCCESS"
               end if
               
               reportCompilerTestDiag format("found assert success at line %d", tLineNumber)
               put "success" & return after tAssertions
            else if tToken is "%WARNING" then
               if (the number of words in tLine is 4 and \
                     word 3 of tLine is "AT") then
                  
                  -- Check that the position exists
                  if tPositions[word 4 of tLine] is empty then
                     reportCompilerTestError pScriptFile, tLineNumber, format("unknown position '%s'", word 4 of tLine)
                  end if
                  
                  reportCompilerTestDiag format("found assert warning %s for position %s (with '%s') at line %d", word 2 of tLine, word 4 of tLine, word 6 of tLine, tLineNumber)
                  
                  put "warning", word 2 of tLine, word 4 of tLine & return after tAssertions
               else
                  reportCompilerTestError pScriptFile, tLineNumber, "incorrect syntax, expected: %WARNING <id> AT <position>"
               end if
            else if tToken is "%ERROR" then
               if (the number of words in tLine is 4 and \
                     word 3 of tLine is "AT") then
                  
                  -- Check that the position exists
                  if tPositions[word 4 of tLine] is empty then
                     reportCompilerTestError pScriptFile, tLineNumber, format("unknown position '%s'", word 4 of tLine)
                  end if
                  
                  reportCompilerTestDiag format("found assert error %s for position %s at line %d", word 2 of tLine, word 4 of tLine, tLineNumber)
                  
                  put "error", word 2 of tLine, word 4 of tLine & return after tAssertions
               else
                  reportCompilerTestError pScriptFile, tLineNumber, "incorrect syntax, expected: %ERROR <id> AT <position>"
               end if
            end if
         else if tToken is "%ENDTEST" then
            -- If we aren't in assertions state then it isn't a complete test
            -- clause
            if tState is not "assertions" then
               reportCompilerTestError pScriptFile, tLineNumber, "premature %ENDTEST directive"
            end if
            
            if tAssertions is empty then
               reportCompilerTestError pScriptFile, tLineNumber, "no assertions specified"
            end if
            
            if tAssertions contains "%SUCCESS" and \
                  tAssertions contains "%ERROR" then
               reportCompilerTestError pScriptFile, tLineNumber, "both %SUCCESS and %ERROR not allowed in the same set of assertions"
            end if
            
            -- If this is the test we are looking for, we are done
            if tName is pTest then
               exit repeat
            end if
            
            -- Move back to initial state
            put empty into tName
            put empty into tState
         else if tToken is "%SET" then
            if tState is not "set" then
               reportCompilerTestError pScriptFile, tLineNumber, "set directive must precede code"
            end if
            add 1 to tSetCount
            put word 3 of tLine into tSet[word 2 of tLine]
         else
            -- We've encountered an illegal directive
            reportCompilerTestError pScriptFile, tLineNumber, format("unknown directive '%s'", tToken)
         end if
      else if tState is empty then
         -- We only allow empty lines outside of test blocks
         if word 1 to -1 of tLine is not empty then
            reportCompilerTestError pScriptFile, tLineNumber, "junk outside of test clause"
         end if
      else if tState is "code" or tState is "set" then
         put "code" into tState
         -- If we are in a code block, then accumulate the code line after
         -- processing for position directives %{...}.
         
         -- We loop through the line, searching for %{...}, and accumulating the
         -- code in between as we go.
         local tPositionSkip
         put 0 into tPositionSkip
         repeat forever
            local tPositionOffset
            put offset("%{", tLine, tPositionSkip) into tPositionOffset
            if tPositionOffset is 0 then
               put char tPositionSkip to -1 of tLine after tCode
               exit repeat
            end if
            add tPositionSkip to tPositionOffset

            put char tPositionSkip to tPositionOffset - 1 of tLine after tCode

            -- Look for the end brace, it is an error if it is not there
            local tPositionEndOffset
            put offset("}", tLine, tPositionOffset) into tPositionEndOffset
            if tPositionEndOffset is 0 then
               reportCompilerTestError pScriptFile, tLineNumber, "unterminated named position"
            end if
            add tPositionOffset to tPositionEndOffset
            
            -- Check that the name is not empty
            local tPositionName
            put char tPositionOffset + 2 to tPositionEndOffset - 1 of tLine into tPositionName
            if tPositionName is empty then
               reportCompilerTestError pScriptFile, tLineNumber, "empty named position"
            end if
            
            -- Check that the position hasn't already been defined
            if tPositions[tPositionName] is not empty then
               reportCompilerTestError pScriptFile, tLineNumber, "named position already defined"
            end if
            
            reportCompilerTestDiag format("found position '%s' at line %d", tPositionName, tLineNumber)
            put tLineNumber - tTestLineNumber - tSetCount + 1 into tPositions[tPositionName]
            
            put tPositionEndOffset + 1 into tPositionSkip
         end repeat
         
         -- Add the newline to the code
         put return after tCode
      else if tState is "assertions" then
         -- We only allow directives inside assertion blocks
         if word 1 to -1 of tLine is not empty then
            reportCompilerTestError pScriptFile, tLineNumber, "junk inside assertion clause"
         end if
      end if
   end repeat

   if tName is empty then
      reportCompilerTestError pScriptFile, tLineNumber, format("test '%s' not found in file", pTest)
   end if
   
   -- We now have the code for the test, the expectation and a list of
   -- assertions.
   
   put tName into rCompilerTest["name"]
   put tCode into rCompilerTest["code"]
   put tPositions into rCompilerTest["positions"]
   put tExpectation into rCompilerTest["expectation"]
   put tExpectation into rCompilerTest["reason"]
   put tAssertions into rCompilerTest["assertions"]
   put tSet into rCompilerTest["set"]
end processCompilerTest

private command reportCompilerTestError pFile, pLine, pMessage
   throw format("'%s', line %d: %s", pFile, pLine, pMessage)
end reportCompilerTestError

command reportCompilerTestDiag pMessage
   if $LCC_VERBOSE is not empty then
      repeat for each line tLine in pMessage
         write "DIAG:" && tLine & return to stderr
      end repeat
   end if
end reportCompilerTestDiag

command reportCompilerTestDiagWithLineNumbers pMessage
   if $LCC_VERBOSE is not empty then
      local tLineNumber
      put 1 into tLineNumber
      repeat for each line tLine in pMessage
         write "DIAG:" && format("%4d", tLineNumber) && ":" && tLine & return to stderr
         add 1 to tLineNumber
      end repeat
   end if
end reportCompilerTestDiagWithLineNumbers
