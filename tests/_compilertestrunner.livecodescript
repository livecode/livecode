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

-- FIXME provide this on the command line
constant kLogFilename = "_compiler_test_suite.log"

on startup
   try
      CompilerTestRunnerMain
   catch tError
      write "ERROR: " & tError & return to stderr
   end try
   quit 0
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

command CompilerTestRunnerMain
   local tInfo
   put getCommandLineInfo() into tInfo
   logInit

   switch tInfo["args"][1]
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
end CompilerTestRunnerMain

private command doRun pInfo
   local tScript, tCommand, tAnalysis
   put pInfo["args"][2] into tScript
   put pInfo["args"][3] into tCommand

   if $LC_COMPILE is empty then
      throw "LC_COMPILE environment var must be set to lc-compile to use"
   end if

   runLoadLibrary pInfo

   if tScript is empty then
      runAllScripts pInfo
   else if tCommand is empty then
      runTestScript pInfo, tScript
   else
      runTestCommand pInfo, tScript, tCommand
   end if

   put the result into tAnalysis

   -- Save the log to file.
   -- We process to binary data ourselves to ensure encoding and
   -- line endings are appropriate.
   local tLogForWriting
   put textEncode(tAnalysis["log"], "utf8") into tLogForWriting
   if the platform is "win32" then
      replace return with numToChar(13) & numToChar(10) in tLogForWriting
   end if
   put tLogForWriting into url ("binfile:" & kLogFilename)

   if TesterTapGetWorstResult(tAnalysis) is "FAIL" then
      quit 1
   end if
end doRun

private command doUsage pStatus
   write "Usage: _compilertestrunner.livecodescript run [SCRIPT [COMMAND]]" & return to stderr
   quit pStatus
end doUsage

on ErrorDialog pExecutionError
   write "ERROR:" && pExecutionError & return to stderr
   quit 1
end ErrorDialog

----------------------------------------------------------------
-- Support for running tests
----------------------------------------------------------------

-- Add the test runner library stack to the backscripts
private command runLoadLibrary pInfo
	-- Compute the filename of the library stack
	local tFilename
	put pInfo["self-script"] into tFilename

	set the itemDelimiter to slash
	put "_testerlib.livecodescript" into item -1 of tFilename

	-- Load the library
	local tStackname
	put the name of stack tFilename into tStackname

	send "revLoadLibrary" to stack tStackname
end runLoadLibrary

-- Run all the test scripts that can be found below the current
-- directory
private command runAllScripts pInfo
   local tFile, tAnalysis
   repeat for each element tFile in TesterGetTestFileNames("", "compilertest")
      runCompilerTestScript pInfo, tFile
      put TesterTapCombine(tAnalysis, the result) into tAnalysis
   end repeat
   runPrintSummary(tAnalysis)

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

private command runTestProcessOutput pScriptfile, pCommand, pOutput
   -- Create test log
   local tTestLog
   put "###" && TesterGetPrettyTestName(pScriptFile, "compilertest") && pCommand \
         & return & return into tTestLog
   put pOutput & return after tTestLog

   -- Analyse the results and print a summary line
   local tTapResults
   put TesterTapAnalyse(tTestLog) into tTapResults

   logSummaryLine tTapResults, (TesterGetPrettyTestName(pScriptFile, "compilertest") & ":" && pCommand)

   return tTapResults
end runTestProcessOutput

-- Run a specific named test command tCommand in a script file
-- tScriptFile
private command runCompilerTest pInfo, pScriptFile, pTest
   local tCommandLine

   -- First we need to process the specified test in script file

   local tTestInfo
   processCompilerTest pInfo, pScriptFile, pTest, tTestInfo

   -- Next we emit the code for the test, then attempt to compile it
   local tTestFile, tTestInterfaceFile
   put tempName() into tTestFile
   put tempName() into tTestInterfaceFile
   put textEncode(tTestInfo["code"], "utf8") into url ("binfile:" & tTestFile)
   reportCompilerTestDiag format("output test source file to '%s'", tTestFile)
   reportCompilerTestDiag tTestInfo["code"]

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
   reportCompilerTestDiag tCompilerOutput

   -- Process the assertions and make sure each one matches up
   repeat for each line tAssertion in tTestInfo["assertions"]
      if doesCompilerOutputSatisfyAssertion(tCompilerOutput, tCompilerExitStatus, tAssertion, tTestInfo["positions"]) then
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

   runTestProcessOutput pScriptFile, pTest, tTestOutput
   return the result
end runCompilerTest

-- Print out a table of statistics
private command runPrintSummary pAnalysis
   local tSummaryString, tTotal, tDecoration

   put TesterTapGetTestCount(pAnalysis) into tTotal

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

private function doesCompilerOutputSatisfyAssertion pCompilerOutput, pCompilerExitCode, pAssertion, pPositions
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
   repeat for each line tOutputLine in pCompilerOutput
      local tFile, tLine, tColumn, tType, tMessage

      -- The format of each output line is:
      --   <file>:<line>:<col>: (error|warning): <msg>
      put item 1 of tOutputLine into tFile
      put item 2 of tOutputLine into tLine
      put item 3 of tOutputLine into tColumn
      put word 1 of item 4 of tOutputLine into tType
      put item 5 of tOutputLine into tMessage

      -- If the assertion type doesn't match, continue
      if tAssertionType is not tType then
         next repeat
      end if

      -- If the assertion message is not within the message, continue
      if tAssertionPartialMsg is not in tMessage then
         next repeat
      end if

      -- If the position does not match, continue
      if pPositions[tAssertionPos] is not tLine then
         next repeat
      end if

      -- If we get here, we have a match so are successful!
      return true
   end repeat

   -- We failed to find a suitable output line, so failure
   return false
end doesCompilerOutputSatisfyAssertion

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

   read from file pScriptFile until end
   put it into tScript

   close file pScriptFile

   -- First extract the test block we are wanting to run
   local tLine, tLineNumber, tTestLineNumber, tState
   local tName, tCode, tExpectation, tExpectationReason, tAssertions, tPositions
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
            put "code" into tState
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

                  reportCompilerTestDiag format("found assert error %s for position %s at line %d", word 2 of tLine, word 4 of tLine, word 6 of tLine, tLineNumber)

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
         else
            -- We've encountered an illegal directive
            reportCompilerTestError pScriptFile, tLineNumber, format("unknown directive '%s'", tToken)
         end if
      else if tState is empty then
         -- We only allow empty lines outside of test blocks
         if word 1 to -1 of tLine is not empty then
            reportCompilerTestError pScriptFile, tLineNumber, "junk outside of test clause"
         end if
      else if tState is "code" then
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
            put tLineNumber - tTestLineNumber + 1 into tPositions[tPositionName]

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
end processCompilerTest

private command reportCompilerTestError pFile, pLine, pMessage
   throw format("'%s', line %d: %s", pFile, pLine, pMessage)
end reportCompilerTestError

private command reportCompilerTestDiag pMessage
   if $LCC_VERBOSE is not empty then
      repeat for each line tLine in pMessage
         write "DIAG:" && tLine & return to stderr
      end repeat
   end if
end reportCompilerTestDiag

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
   put TesterTapGetTestCount(pTapResults) into tTotal

   put TesterTapGetWorstResult(pTapResults) into tWorst
   put logHighLight(the toUpper of tWorst) into tMessage
   put ":" after tMessage
   if the number of chars in tWorst < 5 then
      put space after tMessage
   end if

   put space & pTestName after tMessage

   put space & "[" & tPassed & "/" & tTotal & "]" after tMessage
   write tMessage & return to stdout
end logSummaryLine
