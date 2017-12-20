script "ExtractFontAwesome"
on preOpenStack
   set the visible of me to true
   create button "Extract Font Awesome"
end preOpenStack

on mouseUp
   if the short name of the target is "Extract Font Awesome" then
      if there is a file (specialFolderPath("home") & "/Font-Awesome/src/icons.yml") then
         ExtractFontAwesome specialFolderPath("home") & "/Font-Awesome"
      else
         answer folder "Choose font awesome repo"
         if it is empty then
            exit mouseUp
         end if
         ExtractFontAwesome it
      end if
   end if
end mouseUp

command ExtractFontAwesome pRepo
   local tYAML
   put url ("file:" & pRepo & "/_config.yml") into tYAML
   YAMLToArray tYAML
   
   local tInfo
   put it into tInfo
   
   put url ("file:" & pRepo & slash & tInfo["icon_meta"]) into tYAML
   
   YAMLToArray tYAML
   
   local tIconInfo
   put it["icons"] into tIconInfo
   
   local tFile
   put pRepo & "/fonts/fontawesome-webfont.svg" into tFile
   local tID
   put revXmlCreateTreeFromFile(tFile, false, true, false) into tID
   
   if tID is not an integer then
      exit ExtractFontAwesome
   end if
   
   local tChildNames
   put revXMLChildNames(tID, "/svg/defs/font", return, "glyph", true) into tChildNames
   
   local tArray
   repeat for each line tChildName in tChildNames
      local tName, tCodepoint, tPath
      put revXMLAttribute(tID, "/svg/defs/font/" & tChildName , "unicode") into tCodepoint
      if tCodepoint begins with "xmlerr" then
         next repeat
      end if
      
      put tolower(baseConvert(codepointToNum(textDecode(tCodepoint, "UTF8")),10,16)) into tCodepoint
      
      put revXMLAttribute(tID, "/svg/defs/font/" & tChildName , "d") into tPath
      if tPath begins with "xmlerr" then
         next repeat
      end if
      
      put empty into tName
      repeat for each element tIcon in tIconInfo
         if tIcon["unicode"] is tCodepoint then
            put tolower(tIcon["name"]) into tName
         end if
      end repeat
      if tName is empty then
         next repeat
      end if
      
      -- workaround slash in name
      replace "1/2" with "half" in tName
      replace "3/4" with "three quarter" in tName
      replace "1/4" with "one quarter" in tName
      replace slash with space in tName
      
      put tPath into tArray[tName]["svg"]
      put tCodepoint into tArray[tName]["codepoint"]
   end repeat
   
   revXMLDeleteTree tID
   
   local tGlyphs
   put ArrayToJSON(tArray, "object", true) into tGlyphs
   replace return with " \" & return in tGlyphs
   put "private constant kFontawesome is \" & return before tGlyphs
   
   local tFilename
   put the filename of me into tFilename
   set the itemDelimiter to slash
   put "iconsvg.lcb" into item -2 to -1 of tFilename
   
   local tFileData
   put url ("binfile:" & tFilename) into tFileData
   
   local tStart, tEnd, tDummy
   if matchChunk(tFileData,"(?i)(/\* begin glyph extract from fontawesome \*/)",tDummy, tStart) and \
         matchChunk(tFileData,"(?i)(/\* end glyph extract from fontawesome \*/)", tEnd, tDummy) then
      
      add 2 to tStart
      subtract 2 from tEnd
      put tGlyphs into char tStart to tEnd of tFileData
   end if
   
   if matchChunk(tFileData,"(?i)(/\* begin version extract from fontawesome \*/)",tDummy, tStart) and \
         matchChunk(tFileData,"(?i)(/\* end version extract from fontawesome \*/)", tEnd, tDummy) then
      
      add 2 to tStart
      subtract 1 from tEnd
      
      local tVersion
      put "   return" && quote & tInfo["fontawesome"]["version"] & quote & return & "   " into tVersion
      
      put tVersion into char tStart to tEnd of tFileData
   end if
   put tFileData into url ("binfile:" & tFilename)
end ExtractFontAwesome

constant kMultiLineModeNone = 0
constant kMultiLineModeLiteral = 1
constant kMultiLineModeFolded = 2

command YAMLToArray pYaml
   local tInDocument = true
   local tPath
   local tArray
   local tPathLists
   local tReferences
   local tMultiLineMode
   local tCharNo
   
   put kMultiLineModeNone into tMultiLineMode
   
   local tMultiLine
   repeat for each line tLine in pYaml
      -- ignore directives
      if tLine begins with "%" then
         next repeat
      end if
      
      if tLine begins with "---" then
         put true into tInDocument
         next repeat
      end if
      
      if not tInDocument then
         next repeat
      end if
      
      if tLine begins with "..." then
         if tMultiLineMode is not kMultiLineModeNone then
            __ClearQuotes tMultiLine
            put tMultiLine into tArray[tPath]
            put kMultiLineModeNone into tMultiLineMode
         end if
         put false into tInDocument
         next repeat
      end if
      
      local tPathElement, tListItem
      put __PathElement(tLine, tListItem, tPathLists) into tPathElement
      
      -- comment lines
      put __UnquotedCharOffset("#", tLine) into tCharNo
      if tCharNo > 0 then
         delete char tCharNo to -1 of tLine
      end if
      
      -- remain in multiline until indent is lower
      if tMultiLineMode is not kMultiLineModeNone then
         if tPathElement < the number of elements of tPath + 1 and tLine is not empty then
            __ClearQuotes tMultiLine
            put tMultiLine into tArray[tPath]
            put kMultiLineModeNone into tMultiLineMode
         else
            -- add back any stripped literal indents
            if tPathElement > the number of elements of tPath + 1 then
               repeat for tPathElement - (the number of elements of tPath + 1)
                  put "    " before tLine
               end repeat
            end if
            
            -- empty lines and indented lines are literal even in folded mode
            if tMultiLineMode is kMultiLineModeLiteral then
               if tMultiLine is not empty then
                  put return after tMultiLine
               end if
               put tLine after tMultiLine
            else
               if tLine is empty then
                  put return after tMultiLine
               else if char 1 of tLine is space or \
                     the last line of tMultiLine is empty or \
                     the last line of tMultiLine begins with space then
                  if tMultiLine is not empty then
                     put return after tMultiLine
                  end if
                  put tLine after tMultiLine
               else
                  if tMultiLine is not empty then
                     put space after tMultiLine
                  end if
                  put tLine after tMultiLine
               end if
            end if
            
            next repeat
         end if
      end if
      
      -- empty lines
      if tLine is empty then
         next repeat
      end if
      
      local tElements
      put the number of elements of tPath into tElements
      if tPathLists[tPathElement] and \
            tListItem and \
            tPath[tPathElement] is an integer then
         add 1 to tPath[tPathElement]
         repeat with tIndex = tElements down to tPathElement + 1
            delete variable tPath[tIndex]
            delete variable tPathLists[tIndex]
         end repeat
         add 1 to tPathElement
      else
         if tPathElement < tElements then
            repeat with tIndex = tElements down to tPathElement + 1
               delete variable tPath[tIndex]
               delete variable tPathLists[tIndex]
            end repeat
         else if tPathElement > tElements + 1 then
            return "invalid YAML" for error
         end if
         
         put tListItem into tPathLists[tPathElement]
         if tListItem then
            put 1 into tPath[tPathElement]
            add 1 to tPathElement
         end if
      end if
      
      -- handle line with just list marker
      if the number of words of tLine is 0 then
         next repeat
      end if
      
      -- map
      put __UnquotedCharOffset(":", tLine) into tCharNo
      if tCharNo > 0 then
         local tKey
         
         put char 1 to tCharNo-1 of tLine into tKey
         put char tCharNo+1 to -1 of tLine into tLine
         __ClearQuotes tKey
         
         if tKey is empty then
            return "invalid YAML" for error
         end if
         
         put false into tPathLists[tPathElement]
         put tKey into tPath[tPathElement]
         
         -- clean whitespace
         put word 1 to -1 of tLine into tLine
         
         if tLine is not empty then
            -- referenced element
            local tRef
            if tLine begins with "&" then
               put word 1 of tLine into tRef
               put "*" into char 1 of tRef
               delete word 1 of tLine
            else
               put empty into tRef
            end if
            
            -- check for referenced element
            if word 1 of tLine is among the keys of tReferences then
               put tArray[tReferences[word 1 of tLine]] into tLine
            end if
            
            -- store referenced element
            if tRef is not empty then
               put tPath into tReferences[tRef]
            end if
            
            -- ignore explicit typing
            if tLine begins with "!" then
               delete word 1 of tLine
            end if
            
            if tLine is "|" then
               put kMultiLineModeLiteral into tMultiLineMode
               put empty into tMultiLine
            else if tLine is ">" then
               put kMultiLineModeFolded into tMultiLineMode
               put empty into tMultiLine
            else
               __AddToArray tLine, tArray[tPath]
            end if
         end if
      else
         __AddToArray tLine, tArray[tPath]
      end if
   end repeat
   
   if tMultiLineMode is not kMultiLineModeNone then
      __ClearQuotes tMultiLine
      put tMultiLine into tArray[tPath]
   end if
   
   return tArray for value
end YAMLToArray


private function __UnquotedCharOffset pChar, pLine
   local tOffset = 0
   local tCharOffset
   local tInQuote = "false"
   
   repeat for each char tChar in pLine
      add 1 to tOffset
      if tChar is quote then
         put not tInQuote into tInQuote
      else if tChar is pChar and not tInQuote then
         return tOffset
      end if
   end repeat
   
   return 0
end __UnquotedCharOffset

private command __AddToArray pLine, @xElement
   local tElements, tKey
   -- trim whitespace
   if pLine is not an array then
      put word 1 to -1 of pLine into pLine
      if pLine begins with "[" and pLine ends with "]" then
         -- support single line flow sequence
         put char 2 to -2 of pLine into pLine
         split pLine by comma
         put the number of elements of pLine into tElements
         repeat with tIndex = 1 to tElements
            put word 1 to -1 of pLine[tIndex] into pLine[tIndex]
            __ClearQuotes pLine[tIndex]
         end repeat
      else if pLine begins with "{" and pLine ends with "}" then
         -- support single line flow sequence
         put char 2 to -2 of pLine into pLine
         split pLine by comma
         set the itemDelimiter to ":"
         
         local tLine
         put the number of elements of pLine into tElements
         repeat with tIndex = 1 to tElements
            put word 1 to -1 of pLine[tIndex] into pLine[tIndex]
            
            local tValue
            put item 1 of pLine[tIndex] into tKey
            __ClearQuotes tKey
            put item 2 to -1 of pLine[tIndex] into tValue
            __ClearQuotes tValue
            
            put tValue into tLine[tKey]
         end repeat
         put tLine into pLine
      else
         __ClearQuotes pLine
      end if
   end if
   
   put pLine into xElement
end __AddToArray

private command __ClearQuotes @xLine
   if xLine begins with quote and xLine ends with quote then
      put format( char 2 to -2 of xLine) into xLine
   else if xLine begins with "'" and xLine ends with "'" then
      put char 2 to -2 of xLine into xLine
   end if
end __ClearQuotes

private function __PathElement @xLine, @rListItem, pPathLists
   local tPathElement = 1
   
   put false into rListItem
   repeat
      if xLine begins with "  " then
         add 1 to tPathElement
      else if xLine begins with "- " then
         -- list
         put true into rListItem
      else
         return tPathElement
      end if
      delete char 1 to 2 of xLine
   end repeat
end __PathElement
