script "TesterLib"
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

/*
This script is a library of commands and functions that can be used by
test runner implementations.
*/

on revLoadLibrary
	insert the script of me into back
end revLoadLibrary

----------------------------------------------------------------
-- Script-only stack handling
----------------------------------------------------------------

-- Get all livecode script files beneath the CWD, apart from
-- filenames starting with "." or "_"
function TesterGetTestFileNames pBaseFolder, pExtension
   local tFiles, tCount

   put empty into tFiles
   put 0 into tCount

   if pBaseFolder is empty then
      put the defaultfolder into pBaseFolder
   end if

   if pExtension is empty then
      put "livecodescript" into pExtension
   end if

   TesterGetActualFileNames_Recursive pBaseFolder, empty, pExtension, tFiles, tCount

   return tFiles
end TesterGetTestFileNames

-- Helper command used by runGetTestFileNames
private command TesterGetActualFileNames_Recursive pPath, pRelPath, pExtension, @xFiles, @xCount
   -- Process files in the current directory
   local tFile
   repeat for each line tFile in files(pPath)
      if tFile ends with ("." & pExtension) and \
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
   repeat for each line tFolder in folders(pPath)
      if tFolder begins with "." then
         next repeat
      end if

      put pPath & slash & tFolder into tFolderPath

      if pRelPath is not empty then
         put pRelPath & slash before tFolder
      end if

      TesterGetActualFileNames_Recursive tFolderPath, tFolder, pExtension, xFiles, xCount
   end repeat
end TesterGetActualFileNames_Recursive

-- Get a number-indexed array contain the names of all "test"
-- commands in pFilename.
function TesterParseTestCommandNames pFilename
   local tScript
   -- Get the contents of the file
   local tResult
   open file pFilename for "UTF-8" text read
   put the result into tResult
   if tResult is not empty then
      throw tResult
   end if

   read from file pFilename until end
   put it into tScript
   close file pFilename

   -- Scan the file for "on Test*" definitions
   local tCommandNames, tCount, tLine, tName

   local tCommandNameSeen
   repeat for each line tLine in tScript
      if token 1 of tLine is not "on" then
         next repeat
      end if

      put token 2 of tLine into tName

      if not (tName begins with "Test") then
         next repeat
      end if

      if tCommandNameSeen[tName] then
         -- Duplicate test name
         write merge("Error: duplicate test name [[tName]] in [[pFilename]]\n") to stderr
         quit 1
      end if

      -- Exclude the test setup message
      if tName is "TestSetup" or tName is "TestTearDown" then
         next repeat
      end if

      add 1 to tCount
      put tName into tCommandNames[tCount]
      put true into tCommandNameSeen[tName]
   end repeat

   return tCommandNames
end TesterParseTestCommandNames

-- Prettify a test name by removing a ".livecodescript" suffix
function TesterGetPrettyTestName pFilename, pExtension
   if pExtension is empty then
      put "livecodescript" into pExtension
   end if

   if pFilename ends with ("." & pExtension) then
      set the itemDelimiter to "."
      return item 1 to -2 of pFileName
   end if
end TesterGetPrettyTestName

----------------------------------------------------------------
-- TAP support
----------------------------------------------------------------

-- Scan TAP output and extract summary of test results
function TesterTapAnalyse pTapData
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
end TesterTapAnalyse

-- Examine the statistics generated by tapAnalyse() and return a string
-- describing the test log's worst result
function TesterTapGetWorstResult pTapAnalysis
   local tResult
   repeat for each item tResult in "fail,xpass,xfail,pass,skip"
      if pTapAnalysis[tResult] > 0 then
         return tResult
      end if
   end repeat
   return "pass"
end TesterTapGetWorstResult

-- Get the total number of tests from the statistics generated by
-- tapAnalyse()
function TesterTapGetTestCount pAnalysis
   return pAnalysis["pass"] + pAnalysis["xpass"] + pAnalysis["xfail"] + \
         pAnalysis["fail"] + pAnalysis["skip"]
end TesterTapGetTestCount

-- Combine two sets of TAP results
function TesterTapCombine pAnalysisA, pAnalysisB
   local tCombined, tKey

   if pAnalysisA is an array and pAnalysisB is an array then
      repeat for each key tKey in pAnalysisB
         if tKey is "log" then
            put pAnalysisA[tKey] & pAnalysisB[tKey] into tCombined[tKey]
         else
            put pAnalysisA[tKey] + pAnalysisB[tKey] into tCombined[tKey]
         end if
      end repeat
   else if pAnalysisA is an array then
      put pAnalysisA into tCombined
   else if pAnalysisB is an array then
      put pAnalysisB into tCombined
   else
      put empty into tCombined
   end if
   return tCombined
end TesterTapCombine
