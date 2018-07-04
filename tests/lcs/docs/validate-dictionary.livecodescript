script "DocsValidateDictionary"
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

constant kAPITypes = "command,function,property,message,constant,keyword,control structure,operator,statement,expression,handler"
constant kGlossaryTypes = "library,glossary,object,widget,module"
constant kOS = "iOS,Android,Mac,Windows,Linux,RPi,html5"
constant kPlatform = "desktop,mobile,server,filemaker"

on TestSetup
   TestSkipIfNot "docs"
   
   -- Only run these tests on desktop platforms
   if the platform is not among the items of "MacOS,Windows,Linux" then
      return "SKIP Tests are not runnable on" && the platform
   end if
   
   local tDocsParser
   put TestGetEngineRepositoryPath() & "/ide-support/revdocsparser.livecodescript" into tDocsParser
   start using stack tDocsParser
   
    create stack "Test"
end TestSetup


on TestDocsValidateDictionary
   local tDocsPath, tDocsArray
   put TestGetEngineRepositoryPath() & "/docs" into tDocsPath
   put revDocsParseDictionaryToLibraryArray(tDocsPath) into tDocsArray
   
   __TestDocsLibraryArray tDocsPath, tDocsArray
end TestDocsValidateDictionary

on TestDocsValidateExtractedLibraries
   local tExtractedDocsFolder
   put TestGetExtractedDocsFolder() into tExtractedDocsFolder
   
    -- There are not necessarily any docs in the extracted docs folder now.
    -- Re-uncomment if and when there are.
   //TestAssert "extracted docs folder exists", there is a folder tExtractedDocsFolder
   if there is not a folder tExtractedDocsFolder then
      exit TestDocsValidateExtractedLibraries
   end if
   
   local tFiles
   put files(tExtractedDocsFolder) into tFiles
   
   TestAssert "some files are in the extracted docs folder", tFiles is not empty
   
   filter tFiles with "*.lcdoc"
   
   TestAssert "some lcdoc files are in the extracted docs folder", tFiles is not empty
   
   set the itemDelimiter to "."
   
   local tFile
   repeat for each line tFile in tFiles
      local tDocsPath, tDocsArray
      put tExtractedDocsFolder & slash & tFile into tDocsPath
      
      put revDocsParseDocFileToLibraryArray(tDocsPath, item -2 of tFile, "LiveCode") into tDocsArray
      
      TestAssert tDocsPath & ":" && "type of" && tFile && "is library", tDocsArray["type"] is "library"
      TestAssert tDocsPath & ":" && "name of" && tFile && "is not empty", tDocsArray["name"] is not empty
      TestAssert tDocsPath & ":" && "display name of" && tFile && "is not empty", tDocsArray["display name"] is not empty
      TestAssert tDocsPath & ":" && "author name of" && tFile && "is not empty", tDocsArray["author"] is not empty
      TestAssert tDocsPath & ":" && "description of" && tFile && "is not empty", tDocsArray["description"] is not empty
      
      __TestDocsLibraryArray tDocsPath, tDocsArray["doc"]
   end repeat
end TestDocsValidateExtractedLibraries

on TestDocsValidateExtensionDocs
   local tExtensionsFolder
   put TestGetPackagedExtensionsFolder() into tExtensionsFolder
   
   TestAssert "packaged extensions folder exists", there is a folder tExtensionsFolder
   
   local tExtensions
   put folders(tExtensionsFolder) into tExtensions
   filter tExtensions without ".*"
   
   repeat for each line tExtension in tExtensions
      local tFiles
      put files(tExtensionsFolder & slash & tExtension) into tFiles
      
      filter tFiles with "*.lcdoc"
      
      TestAssert tExtension && "has docs file", tFiles is not empty
      
      set the itemDelimiter to "."
      
      local tFile
      repeat for each line tFile in tFiles
         local tDocsPath, tDocsArray
         put tExtensionsFolder & slash & tExtension & slash & tFile into tDocsPath
         
         if url("file:" & tDocsPath) is empty then
            TestAssertBroken tDocsPath & ": non-empty docs", false
            next repeat
         end if
         
         put revDocsParseDocFileToLibraryArray(tDocsPath, tExtension, "LiveCode") into tDocsArray
         
         TestAssert tDocsPath & ":" && "type of" && tFile && "is not empty", tDocsArray["type"] is not empty
         TestAssert tDocsPath & ":" && "display name of" && tFile && "is not empty", tDocsArray["display name"] is not empty
         TestAssert tDocsPath & ":" && "author name of" && tFile && "is not empty", tDocsArray["author"] is not empty
         
         -- At the moment, allow docs to have either summary or description
         local tExtensionNoSummary, tExtensionNoDescription
         put tDocsArray["summary"] is empty into tExtensionNoSummary
         put tDocsArray["description"] is empty into tExtensionNoDescription
         
         if tExtensionNoSummary and tExtensionNoDescription then
            TestAssert tDocsPath & ":" && "summary of" && tFile && "is not empty", false
            TestAssert tDocsPath & ":" && "description of" && tFile && "is not empty", false
         else if tExtensionNoSummary then
            TestAssertBroken tDocsPath & ":" && "summary of" && tFile && "is not empty", false
         else if tExtensionNoDescription then
            TestAssertBroken tDocsPath & ":" && "description of" && tFile && "is not empty", false
         else
            TestAssert tDocsPath & ":" && "summary of" && tFile && "is not empty", true
            TestAssert tDocsPath & ":" && "description of" && tFile && "is not empty", true
         end if
         
         __TestDocsLibraryArray tDocsPath, tDocsArray["doc"], tDocsArray["type"] is not "module"
      end repeat
   end repeat
end TestDocsValidateExtensionDocs

command __TestDocsLibraryArray pDocsPath, pLibrary, pTestSyntax
   local tElement
   
   local tReferences
   repeat for each element tElement in pLibrary
      put true into tReferences[tElement["type"]][tElement["display name"]]
   end repeat
   
   repeat for each element tElement in pLibrary
      local tValue
      local tIndex
      
      -- test name required
      TestAssert pDocsPath & ":" && "name of" && tElement["display name"] && tElement["type"] && "not empty", tElement["display name"] is not empty
      
      -- test type is valid
      TestAssert pDocsPath & ":" && "type of" && tElement["display name"] &&  tElement["type"] && "is valid", tElement["type"] is among the items of (kAPITypes,kGlossaryTypes)
      
      -- test syntaxes exist for API entries
      local tSyntaxParams
      if tElement["type"] is among the items of kAPITypes then
         TestAssert pDocsPath & ":" && "API entry for" && tElement["display name"] && tElement["type"] && "has syntax", tElement["syntax"] is an array
         
         -- parse out params to validate against documented params
         put 1 into tIndex
         repeat for each element tValue in tElement["syntax"]
            -- ensure matching brackets
            CheckBrackets tValue, "syntax" && tIndex && "of" && tElement["display name"] && tElement["type"]
            
            split tValue by "<"
            local tSplitIndex
            repeat with tSplitIndex = 2 to the number of elements of tValue
               split tValue[tSplitIndex] by ">"
               split tValue[tSplitIndex][1] by "|"
               put true into tSyntaxParams[tValue[tSplitIndex][1][1]]
            end repeat
            add 1 to tIndex
         end repeat
         
         -- test summary required
         TestAssert pDocsPath & ":" && "summary of" && tElement["display name"] && tElement["type"] && "not empty", tElement["summary"] is not empty
         
      end if
      
      -- !TODO test associations are valid
      
      -- test OS
      repeat for each element tValue in tElement["OS"]
         TestAssert pDocsPath & ":" && "OS entry" && tValue && "for" && tElement["display name"] && tElement["type"] && "is valid", tValue is among the items of kOS
      end repeat
      
      -- test platform
      repeat for each element tValue in tElement["Platform"]
         TestAssert pDocsPath & ":" && "Platform entry" && tValue && "for" && tElement["display name"] && tElement["type"] && "is valid", tValue is among the items of kPlatform
      end repeat
      
      -- test examples
      put 1 into tIndex
      if pTestSyntax is not false and \
         tElement["display name"] is not among the words of "sessionName errorObject" then
         repeat for each element tValue in tElement["Example"]
            
         if word 1 of tValue is among the words of "end case catch" then
               next repeat
            end if
            
            --some scrips have ... to indicate user code
            replace "..." with empty in tValue
            
            --some html entities
            replace "&gt;" with ">" in tValue
            replace "&lt;" with "<" in tValue
            
            local tLine
            local tRequiresWrap
            put true into tRequiresWrap
            repeat for each line tLine in tValue
               if word 1 of tLine is among the words of "on private command function before after setprop getprop" then
                  put false into tRequiresWrap
                  exit repeat
               end if
            end repeat
            
            local tWrapWith
            put empty into tWrapWith
            if tRequiresWrap then
               get matchText(tValue, "exit (.+)", tWrapWith)
               if tWrapWith is empty or word 1 of tWrapWith is among the words of "repeat if switch to" then
                  put "mouseUp" into tWrapWith
               end if
            end if
            
            local tTempValue
            if tRequiresWrap then
               put "on" && tWrapWith & return & tValue & return & "end" && tWrapWith into tTempValue
            else
               put tValue into tTempValue
            end if
            
            set the script of stack "Test" to tTempValue
            local tResult
            put the result into tResult
            
            --!TODO this should not be necessary if we fix all these docs but baby steps!!!
            -- check for lack of vairable declaration but only auto fix for 1 variable
            if tResult is not empty then
               -- require variable declaration
               if item 1 of line 1 of tResult is among the items of 14,78,209,423,507 and "(" is not in item 4 of line 1 of tResult then
                  put "local" && item 4 of line 1 of tResult & return before tTempValue
                  set the script of stack "Test" to tTempValue
                  put the result into tResult
               else if the number of lines of tValue is 1 then
                  --!TODO this should not be necessary if we fix all these docs but baby steps!!!
                  -- check for half baked examples
                  set the itemDelimiter to "("
                  switch 
                     case word 1 of tValue is not among the lines of the commandNames
                     case item 1 of tValue is among the lines of the functionNames and the number of items of tValue > 1
                        put "put " before tValue
                        break
                     case word 1 of tValue is "repeat"
                        put return & "end repeat" after tValue
                        break
                     case word -1 of tValue is "then"
                        put return & "end if" after tValue
                        break
                  end switch
                  set the itemDelimiter to comma
                  
                  if tRequiresWrap then
                     put "on" && tWrapWith & return & tValue & return & "end" && tWrapWith into tTempValue
                  end if
                  
                  set the script of stack "Test" to tTempValue
                  put the result into tResult
               end if
            end if
            
            TestAssert pDocsPath & ":" && "example" && tIndex && "of" && tElement["display name"] && tElement["Type"] && "compiles", tResult is empty
            
            if tResult is not empty then
               TestDiagnostic "Example:" & return & tTempValue  & return & "Script Parsing Error:" & return & \
                     line (item 1 of line 1 of tResult) of the scriptParsingErrors && "-" && item 4 of line 1 of tResult
            end if
            add 1 to tIndex
         end repeat
      end if
      
      -- skip parameter tests for properties
      if tElement["type"] is not "property" then
         
         -- test parameters
         local tDocumentedParams
         repeat for each element tValue in tElement["params"] 
            put true into tDocumentedParams[tValue["name"]]
         end repeat
         
         local tUnion
         put tDocumentedParams into tUnion
         union tUnion with tSyntaxParams
         
         local tParam, tType
         repeat for each key tParam in tUnion
            
            local tParamInGlossary
            put false into tParamInGlossary
            
            -- determine if the param specified a type
            SplitParam tParam
            
            -- params in syntax section may be documented in glossary
            TestAssert pDocsPath & ":" && "param" && tParam[1] && "in param section" && tElement["display name"] && tElement["Type"], tDocumentedParams[tParam[1]] -- or tParamInGlossary
            
            -- params in param section should be in the syntax section otherwise what are they documenting
            TestAssert pDocsPath & ":" && "param" && tParam[1] && "in syntax section" && tElement["display name"] && tElement["Type"], tSyntaxParams[tParam[1]]
         end repeat
         
      end if
      
      delete variable tDocumentedParams
      delete variable tSyntaxParams
      
      -- test description required
      TestAssert pDocsPath & ":" && "description of" && tElement["display name"] && tElement["type"] && "not empty", tElement["Description"] is not empty
      
      -- !TODO parse all summaries and descriptions etc
      -- * test all links are listed in references
      -- * test for ambiguous links
      
   end repeat
end __TestDocsLibraryArray

command SplitParam @pParam
   -- find type
   split pParam by "("
   split pParam[2] by ")"
   
   -- trim
   put word 1 to -1 of pParam[1] into pParam[1]
   put word 1 to -1 of pParam[2][1] into pParam[2]
end SplitParam

command CheckBrackets pSyntax, pMessage
   local tCodepoint, tBrackets
   local tPrintDiagnostic = false
   repeat for each codepoint tCodepoint in pSyntax
      switch tCodepoint
         case "{"
            SetState tBrackets, "{}", "open", tPrintDiagnostic, pMessage
            break
         case "}"
            SetState tBrackets, "{}", "closed", tPrintDiagnostic, pMessage
            break
         case "["
            SetState tBrackets, "[]", "open", tPrintDiagnostic, pMessage
            break
         case "]"
            SetState tBrackets, "[]", "closed", tPrintDiagnostic, pMessage
            break
         case "("
            SetState tBrackets, "()", "open", tPrintDiagnostic, pMessage
            break
         case ")"
            SetState tBrackets, "()", "closed", tPrintDiagnostic, pMessage
            break
         case "<"
            SetState tBrackets, "<>", "open", tPrintDiagnostic, pMessage
            break
         case ">"
            SetState tBrackets, "<>", "closed", tPrintDiagnostic, pMessage
            break
         case "|"
            -- allow pipe for plural params
            TestAssert pMessage &&  "variant in curly brackets", GetState(tBrackets, "{}") is "open" and GetState(tBrackets, "<>") is "closed"
            if not (GetState(tBrackets, "{}") is "open" and GetState(tBrackets, "<>") is "closed") then
               put true into tPrintDiagnostic
            end if
            break
      end switch
   end repeat
   
   TestAssert pMessage && "matched pairs of curly brackets", tBrackets["{}"]["count"] is empty or tBrackets["{}"]["count"] is 0
   
   if not tPrintDiagnostic then
      put not (tBrackets["{}"]["count"] is empty or  tBrackets["{}"]["count"] is 0) into tPrintDiagnostic
   end if
   
   TestAssert pMessage && "matched pairs of square brackets", tBrackets["[]"]["count"] is empty or  tBrackets["[]"]["count"] is 0
   
   if not tPrintDiagnostic then
      put not (tBrackets["[]"]["count"] is empty or  tBrackets["[]"]["count"] is 0) into tPrintDiagnostic
   end if
   
   TestAssert pMessage && "matched parentheses", tBrackets["()"]["count"] is empty or  tBrackets["()"]["count"] is 0
   
   if not tPrintDiagnostic then
      put not (tBrackets["()"]["count"] is empty or  tBrackets["()"]["count"] is 0) into tPrintDiagnostic
   end if
   
   TestAssert pMessage && "matched angle brackets", tBrackets["<>"]["count"] is empty or  tBrackets["<>"]["count"] is 0
   
   if not tPrintDiagnostic then
      put not (tBrackets["<>"]["count"] is empty or  tBrackets["<>"]["count"] is 0) into tPrintDiagnostic
   end if
   
   if tPrintDiagnostic then
      TestDiagnostic pSyntax
   end if
end CheckBrackets

constant kBracketTypes = "{},[],(),<>"

command SetState @xBrackets, pType, pState, @xPrintDiagnostic, pMessage
   local tSize
   put the number of elements of xBrackets[pType] into tSize
   
   if pState is "open" then
      add 1 to tSize
   end if
   
   put pState into xBrackets[pType][tSize]["state"]
   
   local tType
   repeat for each item tType in kBracketTypes
      if tType is pType then
         next repeat
      end if
      
      local tOtherSize
      put the number of elements of xBrackets[tType] into tOtherSize
      
      if tOtherSize is 0 then
         next repeat
      end if
      
      if pState is "open" then
         if xBrackets[tType][tOtherSize]["state"] is "open" then
            put tOtherSize into xBrackets[pType][tSize][tType]
         else if xBrackets[pType][tSize][tType] is an integer and \
               xBrackets[tType][xBrackets[pType][tSize][tType]]["state"] is not "open" then
            -- check if any brackets open when we opened these are still open
            TestAssert pMessage &&  "brackets" && pType && tType && "nested correctly" , false
            put true into xPrintDiagnostic
         end if
      end if
   end repeat
end SetState

function GetState pBrackets, pType
   local tSize
   repeat with tSize = the number of elements of pBrackets[pType] down to 1
      if pBrackets[pType][tSize]["state"] is "open" then return "open"
   end repeat
   
   return "closed"
end GetState

on TestTeardown
   delete stack "revDocsParser"
   
   delete stack "Test"
end TestTeardown
