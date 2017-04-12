script "JSONTestSuite"
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

on TestSetup
   TestLoadExtension "com.livecode.library.json"

   set the itemDelimiter to slash
   set the defaultFolder to ((item 1 to -2 of the filename of this stack) & slash & "JSONTestSuite")
   TestDiagnostic the defaultFolder
end TestSetup

/*
These tests originate from the JSON test suite available from
https://github.com/nst/JSONTestSuite.

The various parser implementations that the upstream repo contains,
along with its test control files and result data, have been
discarded, keeping only the JSON fragment test cases.  The
"test_parsing_broken.json" and "test_parsing_skipped.json" list
appropriate test cases.  This makes sure updated test cases from
upstream can be imported with a minimum of hassle.

*/

private command attemptLoad pFilename
  local tText
  local tUrlResult
  put url ("file:" & pFilename) into tText
  put the result into tUrlResult
  if tUrlResult is not empty then
     TestDiagnostic merge("Failed to load [[pFilename]]: [[tUrlResult]]")
     return "read failed" for error
  end if

  return tText for value
end attemptLoad

private command attemptParse pJson
   local tError
   try
      return JsonImport(pJson) for value
   catch tError
      return tError for error
   end try
end attemptParse

private function getDescription pFilename
   put char 3 to -6 of pFilename into pFilename
   replace "_" with " " in pFilename
   replace "#" with "U+0023" in pFilename
   return pFilename
end getDescription

local sMeta

private command scanMeta pType, pCategory
   local tFilename
   put merge("[[pType]]_[[pCategory]].json") into tFilename
   TestDiagnostic merge("Loading [[pCategory]] test info from [[tFilename]]")
   get url ("file:" & tFilename)
   if it is not empty then
      put JsonImport(it) into sMeta[pCategory]
   end if
end scanMeta

private function checkMeta pFilename, pCategory
   return sMeta[pCategory][pFilename]
end checkMeta

private command scanSkipped pType
   scanMeta pType, "skipped"
end scanSkipped

-- We skip files which test for implementation-defined
-- semantics
private function isSkipped pFilename
   return checkMeta(pFilename, "skipped")
end isSkipped

private command scanBroken pType
   scanMeta pType, "broken"
end scanBroken

private function isBroken pFilename
   return checkMeta(pFilename, "broken")
end isBroken

private command scanImpl pType
   scanMeta pType, "impl"
end scanImpl

private function isSupposedToParse pFilename
   switch char 1 of pFilename
      case "y"
         return true
      case "n"
         return false
      case "i"
         return (checkMeta(pFilename, "impl") is not false)
   end switch
end isSupposedToParse

on TestParsing
   local tFilename, tDescription, tSuccess
   local tSkipReason, tBrokenReason, tResult

   scanSkipped "test_parsing"
   scanBroken "test_parsing"
   scanImpl "test_parsing"

   repeat for each line tFilename in files("test_parsing")
      put getDescription(tFilename) into tDescription
      TestDiagnostic merge("Processing [[tFilename]]")

      put isSkipped(tFilename) into tSkipReason
      if tSkipReason is not empty then
         TestSkip tDescription, tSkipReason
         next repeat
      end if

      put true into tSuccess
      if tSuccess then
         attemptLoad merge("test_parsing/[[tFilename]]")
         put the result into tResult
         if tResult is not empty then
            put false into tSuccess
            TestDiagnostic tResult
         end if
      end if

      if tSuccess then
         attemptParse it
         put the result into tResult
         if tResult is not empty then
            put false into tSuccess
            TestDiagnostic tResult
         else
            TestDiagnostic "Parsed successfully"
         end if
      end if

      put isBroken(tFilename) into tBrokenReason
      put (tSuccess and isSupposedToParse(tFilename)) or \
            (not tSuccess and not isSupposedToParse(tFilename)) \
            into tSuccess

      if tBrokenReason is not empty then
         TestAssertBroken tDescription, tSuccess, tBrokenReason
      else
         TestAssert tDescription, tSuccess
      end if
   end repeat
end TestParsing
