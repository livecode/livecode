script "Cheat Sheet"
/*
Currently this cannot be run from the command line as it requires
the ability to colorize scripts.
*/
on startup
   send "CreateCheatSheetMain" to me in 0
end startup

private function getCommandLineInfo
   local tArg, tArgs, tInArgs
   put false into tInArgs
   
   repeat for each element tArg in the commandArguments
      if tInArgs then
         put tArg into tArgs[1 + the number of elements in tArgs]
      else
         if tArg ends with ".livecodescript" then
            put true into tInArgs
         end if
      end if
   end repeat
   
   return tArgs
end getCommandLineInfo

constant kLanguageArgs = "livecode,livecode_builder,python,javascript"
constant kSourceLanguages = "livecode_builder,python,javascript"
constant kTargetLanguages = "livecode,livecode_builder"
private function ProcessArgs pArgs
   local tArgs
   repeat with x = 1  to the number of elements in pArgs
      switch pArgs[x]
         case "--from"
         case "--to"
         case"--language"
            local tLanguage
            put pArgs[x + 1] into tLanguage
            if tLanguage is not among the items of kLanguageArgs then
               return merge("invalid language [[tLanguage]]")
            end if
            put tLanguage into tArgs[char 3 to -1 of pArgs[x]]
         case "--folder"
            put pArgs[x+1] into tArgs["folder"]
         case "--markdown"
            put true into tArgs["markdown"]
            break
         default
            if pArgs[x] begins with "--" then
               return merge("invalid argument [[pArgs[x]]]")
            end if
      end switch
   end repeat
   if (pArgs["from"] is empty) is not (pArgs["to"] is empty) then
      if pArgs["from"] is empty then
         return "missing source language - use --from"
      else
         return "missing target language - use --to"
      end if
   else if pArgs["language"] is not empty and \
         pArgs["from"] is not empty then
      return "--language and --from / --to forms mutually exclusive"
   end if
   return tArgs
end ProcessArgs

local sArray
command CreateCheatSheetMain
   put CheatSheetArray() into sArray
   if sArray is not an array then
      write sArray & return to stderr
      quit 1
   end if
   
   local tArgs
   put getCommandLineInfo() into tArgs
   
   local tProcessedArgs
   put ProcessArgs(tArgs) into tProcessedArgs
   if tProcessedArgs is not an array then
      write tProcessedArgs & return to stderr
      quit 1
   end if
   
   if tProcessedArgs["folder"] is empty then
      write "must specify output folder" & return to stderr
      quit 1
   end if
   
   if tArgs["language"] is empty and tArgs["from"] is empty then
      CreateAllCheatSheets tArgs["folder"], tArgs["markdown"]
   else if tArgs["language"] is not empty then
      CreateCheatSheet tArgs["language"], tArgs["folder"], tArgs["markdown"]
   else
      CreateCheatSheetFrom tArgs["from"], tArgs["to"], tArgs["folder"], tArgs["markdown"]
   end if
end CreateCheatSheetMain

local sConfigured, sTargetStack
private command ConfigureStack
   if sConfigured then
      exit ConfigureStack
   end if
   put true into sConfigured
   lock screen
   set the name of the templateStack to "Cheat Sheet Stack"
   create stack
   put it into sTargetStack
   set the width of sTargetStack to 595
   set the height of sTargetStack to 842
   set the topleft of sTargetStack to 200,200
   unlock screen
   set the defaultStack to "Cheat Sheet Stack"
end ConfigureStack

command ClearAll
   if exists(sTargetStack) then
      delete stack sTargetStack
   end if
   put false into sConfigured
end ClearAll

/**
Create all available cheat sheets

Parameters:
pOutputFolder: The location to create the cheat sheets

Description:
This command creates cheat sheets for all languages for which cheat
sheets are available and outputs PDFs in <pOutputFolder>.
*/
command CreateAllCheatSheets pOutputFolder, pMarkdown
   -- First generate each combination of something -> lcs or lcb
   repeat for each item tFrom in kSourceLanguages
      repeat for each item tTo in kTargetLanguages
         if tFrom is tTo then next repeat
         CreateCheatSheetFrom tFrom, tTo, pOutputFolder, pMarkdown
      end repeat
   end repeat
   
   -- Then do single cheat sheets for lcs or lcb
   repeat for each item tItem in kTargetLanguages
      CreateCheatSheet tItem, pOutputFolder, pMarkdown
   end repeat
end CreateAllCheatSheets

/**
Create a cheat sheet for the given language

Parameters:
pLanguage (enum):
- "livecode": Create a cheat sheet for LiveCode
- "livecode_builder": Create a cheat sheet for LiveCode
- "python": Create a cheat sheet for Python to LiveCode transition
- "javascript": Create a cheat sheet for JavaScript to LiveCode transition

pOutputFolder: The location to create the cheat sheet

Description:
This command creates a cheat sheet for the given language and outputs as
a PDF in <pOutputFolder> with filename "cheat-sheet-'<pLanguage>'.pdf".
*/
command CreateCheatSheet pLanguage, pOutputFolder, pMarkdown
   put CheatSheetArray() into sArray
   if sArray is not an array then
      throw sArray
   end if
   if pMarkdown then
      BuildCheatSheetMarkdown pLanguage, "", pOutputFolder
   else
      ConfigureStack
      BuildCheatSheet pLanguage, "", pOutputFolder
      ClearAll
   end if
end CreateCheatSheet

command CreateCheatSheetFrom pFromLanguage, pToLanguage, pOutputFolder, pMarkdown
   put CheatSheetArray() into sArray
   if sArray is not an array then
      throw sArray
   end if
   if pMarkdown then
      BuildCheatSheetMarkdown pFromLanguage, pToLanguage, pOutputFolder
   else
      ConfigureStack
      BuildCheatSheet pFromLanguage, pToLanguage, pOutputFolder
      ClearAll
   end if
end CreateCheatSheetFrom

private command CreatePDF pNumCards, pOutputFile
   local tPDFPath
   ## Path to the pdf file we want to create
   put pOutputFile into tPDFPath
   
   open printing to pdf tPDFPath
   if the result is "Cancel" then
      ## The user has cancelled printing
      exit CreatePDF
   else
      repeat with x = 1 to pNumCards
         print card x of this stack into 0,0, \
               the width of this stack, the height of this stack
         if x is not pNumCards then
            print break
         end if
      end repeat
      
   end if
   close printing
end CreatePDF

constant kTitleTextSize = 16
command BuildCheatSheet pFromLanguage, pToLanguage, pOutputFolder
   lock screen
   set the style of the templatefield to "transparent"
   set the lockText of the templatefield to true
   set the traversalOn of the templatefield to false
   local tTitle, tFilename, tFromDisplayName
   put LanguageToDisplayName(pFromLanguage) into tFromDisplayName
   if pToLanguage is empty then
      put merge("[[tFromDisplayName]] Cheat Sheet") into tTitle
      put merge("cheat-sheet-[[pFromLanguage]].pdf") into tFilename
   else
      local tToDisplayName
      put LanguageToDisplayName(pToLanguage) into tToDisplayName
      put merge("[[tFromDisplayName]] - [[tToDisplayName]] Cheat Sheet") into tTitle
      put merge("cheat-sheet-[[pFromLanguage]]-[[pToLanguage]].pdf") into tFilename
   end if
   
   local tTitleHeight, tTitleGroup
   create group "Title" in this card of sTargetStack
   put it into tTitleGroup
   set the backgroundBehavior of tTitleGroup to true
   create field "title" in tTitleGroup
   set the textAlign of it to "center"
   set the text of it to tTitle
   set the width of it to the width of sTargetStack
   set the topleft of it to 0,0
   set the textSize of it to kTitleTextSize
   set the height of it to the formattedHeight of it
   put the height of it into tTitleHeight
   
   local tNumCards
   if pToLanguage is empty then
      BuildCheatSheetForLiveCode pFromLanguage, tTitleHeight
   else
      BuildCheatSheetForLanguage pFromLanguage, pToLanguage, tTitleHeight
   end if
   put the result into tNumCards
   CreatePDF tNumCards, pOutputFolder & slash & tFilename
   unlock screen
end BuildCheatSheet

command BuildCheatSheetMarkdown pFromLanguage, pToLanguage, pOutputFolder
   local tTitle, tFilename, tFromDisplayName
   put LanguageToDisplayName(pFromLanguage) into tFromDisplayName
   if pToLanguage is empty then
      put merge("[[tFromDisplayName]] Cheat Sheet") into tTitle
      put merge("[[tFromDisplayName]] Cheat Sheet.md") into tFilename
   else
      local tToDisplayName
      put LanguageToDisplayName(pToLanguage) into tToDisplayName
      put merge("[[tFromDisplayName]] - [[tToDisplayName]] Cheat Sheet") into tTitle
      put merge("[[tFromDisplayName]] - [[tToDisplayName]] Cheat Sheet.md") into tFilename
   end if
   
   local tMarkdown
   put "#" && tTitle & return into tMarkdown
   
   if pToLanguage is empty then
      put BuildCheatSheetMarkdownForLiveCode(pFromLanguage) after tMarkdown
   else
      put BuildCheatSheetMarkdownForLanguage(pFromLanguage, pToLanguage) after tMarkdown
   end if
   
   put textEncode(tMarkdown, "utf-8") into url("binfile:" & pOutputFolder & slash & tFilename)
end BuildCheatSheetMarkdown

private function LanguageToKey pLanguage
   switch pLanguage
      case "python"
         return "py"
      case "javascript"
         return "js"
      case "livecode"
         return "lc"
      case "livecode_builder"
         return "lcb"
   end switch
end LanguageToKey

private function LanguageToHighlightJS pLanguage
   switch pLanguage
      case "python"
         return "python"
      case "javascript"
         return "javascript"
      case "livecode"
         return "livecode"
      case "livecode_builder"
         return "livecodebuilder"
   end switch
end LanguageToHighlightJS

private function LanguageToDisplayName pLanguage
   switch pLanguage
      case "python"
         return "Python"
      case "javascript"
         return "JavaScript"
      case "livecode"
         return "LiveCode"
      case "livecode_builder"
         return "LiveCode Builder"
   end switch
end LanguageToDisplayName

command BuildCheatSheetForLanguage pFromLanguage, pToLanguage, pTitleBottom
   local tContent, tColumns, tColNum, tKeys
   local tCurCard, tTop, tToFit
   put 1 into tCurCard
   put pTitleBottom into tTop
   
   repeat for each key tKey in sArray["content"]
      BuildRowForLanguage sArray["content"][tKey], pFromLanguage, pToLanguage
   end repeat
   put the keys of sArray["content"] into tToFit
   sort tToFit ascending numeric
   repeat while tToFit is not empty
      PositionRowsForLanguage pTitleBottom, tTop, sArray["content"], tToFit, \
            pFromLanguage, pToLanguage, tCurCard
   end repeat
   return tCurCard
end BuildCheatSheetForLanguage

function BuildCheatSheetMarkdownForLanguage pFromLanguage, pToLanguage
   local tMarkdown
   repeat for each element tContent in sArray["content"]
      local tRowMarkdown
      put BuildRowMarkdownForLanguage(tContent, pFromLanguage, pToLanguage) \
            into tRowMarkdown
      if tRowMarkdown is empty then
         next repeat
      end if
      put tRowMarkdown & return after tMarkdown
   end repeat
   return tMarkdown
end BuildCheatSheetMarkdownForLanguage

constant kNumCols = 2
constant kMargin = 12
constant kTitlePadding = -8
constant kPadding = 5
constant kSectionTitleTextSize = 14
constant kCodeTextSize = 13
constant kTextSize = 13
command BuildRowForLanguage pContent, pFromLanguage, pToLanguage
   local tFromCodeText, tToCodeText
   put pContent[LanguageToKey(pFromLanguage)] into tFromCodeText
   put pContent[LanguageToKey(pToLanguage)] into tToCodeText
   
   -- Skip the row if no equivalent section
   if tFromCodeText is empty or tToCodeText is empty then
      exit BuildRowForLanguage
   end if
   
   local tColWidth, tWidth
   put the width of sTargetStack into tWidth
   put (tWidth - (kNumCols + 1)*kMargin) / kNumCols into tColWidth
   
   local tGroup, tTitle, tFromCode, tToCode, tText
   create group pContent["title"]
   put it into tGroup
   
   create field "title" in tGroup
   put it into tTitle
   set the width of tTitle to tWidth
   set the text of tTitle to pContent["title"]
   set the textSize of tTitle to kSectionTitleTextSize
   set the textStyle of tTitle to "underline"
   set the textAlign of tTitle to "center"
   
   create field "text" in tGroup
   put it into tText
   set the width of tText to tWidth
   set the text of tText to pContent["text"]
   set the textSize of tText to kTextSize
   set the textAlign of tText to "left"
   
   if pContent["text"] is empty then
      set the height of tText to 0
      set the bottom of tText to the bottom of tTitle
      hide tText
   else
      set the height of tText to the formattedHeight of tText
      set the top of tText to the bottom of tTitle + kTitlePadding
   end if
   
   create field "from" in tGroup
   put it into tFromCode
   set the width of tFromCode to tColWidth
   set the text of tFromCode to tFromCodeText
   set the textSize of tFromCode to kCodeTextSize
   set the left of tFromCode to kMargin
   if pFromLanguage is "livecode" then
      _internal script colorize char 1 to the number \ 
            of chars in tFromCodeText of tFromCode
   end if
   set the height of tFromCode to the formattedHeight of tFromCode
   set the top of tFromCode to the bottom of tText + kPadding
   
   create field "to" in tGroup
   put it into tToCode
   set the width of tToCode to tColWidth
   set the text of tToCode to tToCodeText
   set the textSize of tToCode to kCodeTextSize
   set the left of tToCode to kMargin*2 + tColWidth
   
   if pToLanguage is "livecode" then
      _internal script colorize char 1 to the number \ 
            of chars in tToCodeText of tToCode
   end if
   set the height of tToCode to the formattedHeight of tToCode
   set the top of tToCode to the bottom of tText + kPadding
end BuildRowForLanguage

private function escapeCode pCode
   replace "<" with "&lt;" in pCode
   replace ">" with "&gt;" in pCode
   return pCode
end escapeCode

function BuildRowMarkdownForLanguage pContent, pFromLanguage, pToLanguage
   local tFromCodeText, tToCodeText
   put pContent[LanguageToKey(pFromLanguage)] into tFromCodeText
   put pContent[LanguageToKey(pToLanguage)] into tToCodeText
   
   -- Skip the row if no equivalent section
   if tFromCodeText is empty or tToCodeText is empty then
      return empty
   end if
   
   local tMarkdown
   put "##" && pContent["title"] & return into tMarkdown
   
   if pContent["text"] is not empty then
      put pContent["text"] & return & return after tMarkdown
   end if
   
   put "<table>" after tMarkdown
   put "<tr><td>" && LanguageToDisplayName(pFromLanguage) && "</td><td>" && \
         LanguageToDisplayName(pToLanguage) && "</td></tr>" & return after tMarkdown
   put "<tr><td><pre><code class=" & quote & LanguageToHighlightJS(pToLanguage) \
         & quote & ">" & escapeCode(tFromCodeText) & "</code></pre></td>" & return after tMarkdown
   put "<td><pre><code class=" & quote & LanguageToHighlightJS(pFromLanguage) \
         & quote & ">" & escapeCode(tToCodeText) & "</code></pre></td></tr>" & return after tMarkdown
   put "</table>" & return after tMarkdown
   /*
   put "|" && LanguageToDisplayName(pFromLanguage) && "|" && \
         LanguageToDisplayName(pToLanguage) && "|" & return after tMarkdown
   put "|----------------|------------------|" & return after tMarkdown
   repeat with x = 1 to max(the number of lines in tFromCodeText, \
         the number of lines in tToCodeText)
      put "|" && "`" & line x of tFromCodeText & "`" && "|" && \
            "`" & line x of tToCodeText & "`" && "|" & return \
            after tMarkdown
   end repeat
   */
   return tMarkdown
end BuildRowMarkdownForLanguage

function RowHeight pGroupName
   local tCodeHeight
   if there is a field "code" of group pGroupName then
      put the height of field "code" of group pGroupName into tCodeHeight
   else
      put max(the height of field "from" of group pGroupName, \
            the height of field "to" of group pGroupName) into tCodeHeight
   end if
   
   local tTextHeight
   if there is a field "text" of group pGroupName then
      if the visible of field "text" of group pGroupName then
         put the height of field "text" of group pGroupName + \
               kPadding into tTextHeight
      end if
   end if
   
   return the height of field "title" of group pGroupName + \
         tTextHeight + tCodeHeight + kPadding + kTitlePadding
end RowHeight

command PositionRowsForLanguage pTitleBottom, @xContentBottom, pContent, @xToFit, \
      pFromLanguage, pToLanguage, @xCurCard
   local tCurGroup, tLines, tPositioned
   put false into tPositioned
   put xToFit into tLines
   repeat for each line tLine in tLines
      put pContent[tLine]["title"] into tCurGroup
      if there is not a group tCurGroup then
         delete line (lineoffset(tLine, xToFit)) of xToFit
         next repeat
      end if
      
      local tRowHeight
      put RowHeight(tCurGroup) into tRowHeight
      if tRowHeight + xContentBottom > the height of sTargetStack then
         next repeat
      end if
      
      set the top of group tCurGroup to xContentBottom
      set the left of group tCurGroup to 0
      add tRowHeight to xContentBottom
      delete line (lineoffset(tLine, xToFit)) of xToFit
      put true into tPositioned
   end repeat
   
   if tPositioned then
      exit PositionRowsForLanguage
   end if
   
   // If we get here we need a new card
   repeat for each line tLine in xToFit
      delete group pContent[tLine]["title"]
   end repeat
   create card 
   add 1 to xCurCard
   put pTitleBottom into xContentBottom
   repeat for each line tLine in xToFit
      BuildRowForLanguage pContent[tLine], pFromLanguage, pToLanguage
   end repeat
end PositionRowsForLanguage

command BuildCheatSheetForLiveCode pLanguage, pTitleHeight
   local tCurCard, tColNum, tTop, tToFit
   put 1 into tCurCard
   put the keys of sArray["content"] into tToFit
   sort tToFit ascending numeric
   
   local tColWidth, tWidth
   put the width of sTargetStack into tWidth
   put (tWidth - (kNumCols + 1)*kMargin) / kNumCols into tColWidth
   
   repeat for each key tKey in sArray["content"]
      BuildGroupForTarget pLanguage, sArray["content"][tKey], tColWidth
   end repeat
   put pTitleHeight into tTop
   put 1 into tColNum
   repeat while tToFit is not empty
      repeat while tColNum <= kNumCols
         PositionRowsInColumn tColNum, tColWidth, pTitleHeight, tTop, sArray["content"], tToFit
      end repeat
      if tToFit is empty then
         exit repeat
      end if
      // If we get here we need a new card
      repeat for each line tLine in tToFit
         delete group sArray["content"][tLine]["title"]
      end repeat
      create card in sTargetStack
      add 1 to tCurCard
      put 1 into tColNum
      repeat for each line tLine in tToFit
         BuildGroupForTarget pLanguage, sArray["content"][tLine], tColWidth
      end repeat
   end repeat
   return tCurCard
end BuildCheatSheetForLiveCode

function BuildCheatSheetMarkdownForLiveCode pLanguage
   local tMarkdown
   repeat for each element tContent in sArray["content"]
      local tRowMarkdown
      put BuildGroupMarkdownForTarget(tContent, pLanguage) \
            into tRowMarkdown
      if tRowMarkdown is empty then
         next repeat
      end if
      put tRowMarkdown & return after tMarkdown
   end repeat
   return tMarkdown
end BuildCheatSheetMarkdownForLiveCode

function BuildGroupMarkdownForTarget pContent, pLanguage
   local tCodeText
   put pContent[LanguageToKey(pLanguage)] into tCodeText
   
   -- Skip the row if no equivalent section
   if tCodeText is empty then
      return empty
   end if
   
   local tMarkdown
   put "##" && pContent["title"] & return into tMarkdown
   
   if pContent["text"] is not empty then
      put pContent["text"] & return after tMarkdown
   end if
   
   put "```" & return after tMarkdown
   put tCodeText & return after tMarkdown
   put "```" & return after tMarkdown
   return tMarkdown
end BuildGroupMarkdownForTarget

command BuildGroupForTarget pLanguage, pContent, pWidth
   local tCodeText
   put pContent[LanguageToKey(pLanguage)] into tCodeText
   
   -- Skip the section if empty
   if tCodeText is empty then
      exit BuildGroupForTarget
   end if
   
   local tGroup, tTitle, tCode, tText
   create group pContent["title"] in this card of sTargetStack
   put it into tGroup
   
   create field "title" in tGroup
   put it into tTitle
   set the width of tTitle to pWidth
   set the text of tTitle to pContent["title"]
   set the textSize of tTitle to kSectionTitleTextSize
   set the textStyle of tTitle to "underline"
   set the textAlign of tTitle to "center"
   
   create field "text" in tGroup
   put it into tText
   set the width of tText to pWidth
   set the text of tText to pContent["text"]
   set the textSize of tText to kTextSize
   set the textAlign of tText to "left"
   
   if pContent["text"] is empty then
      set the height of tText to 0
      set the bottom of tText to the bottom of tTitle
      hide tText
   else
      set the height of tText to the formattedHeight of tText
      set the top of tText to the bottom of tTitle + kTitlePadding
   end if
   
   create field "code" in tGroup
   put it into tCode
   set the width of tCode to pWidth
   set the text of tCode to tCodeText
   set the textSize of tCode to kCodeTextSize
   
   if pLanguage is "livecode" then
      _internal script colorize char 1 to the number \ 
            of chars in pContent[LanguageToKey("livecode")] of tCode
   end if
   set the height of tCode to the formattedHeight of tCode
   set the loc of tCode to the loc of tTitle
   set the top of tCode to the bottom of tText + kPadding
end BuildGroupForTarget

command PositionRowsInColumn @xCol, pColWidth, pTitleBottom, @xContentBottom, pContent, @xToFit
   local tLastBottom, tLeft
   put xCol*kMargin + (xCol - 1)*pColWidth into tLeft
   
   local tCurGroup, tLines, tPositioned
   put false into tPositioned
   put xToFit into tLines
   repeat for each line tLine in tLines
      put pContent[tLine]["title"] into tCurGroup
      if there is not a group tCurGroup then
         delete line (lineoffset(tLine, xToFit)) of xToFit
         next repeat
      end if
      
      local tRowHeight
      put RowHeight(tCurGroup) into tRowHeight
      if tRowHeight + xContentBottom > the height of sTargetStack then
         next repeat
      end if
      
      set the top of group tCurGroup to xContentBottom
      set the left of group tCurGroup to tLeft
      add tRowHeight to xContentBottom
      delete line (lineoffset(tLine, xToFit)) of xToFit
      put true into tPositioned
   end repeat
   
   if tPositioned then
      exit PositionRowsInColumn
   end if
   
   // If we get here we need a new column
   add 1 to xCol
   put pTitleBottom into xContentBottom
end PositionRowsInColumn

function CheatSheetArray
   local tPath
   put the effective filename of me into tPath
   set the itemdelimiter to slash
   put "cheat-sheet.yml" into item -1 of tPath
   
   local tYAML
   put textDecode(url("binfile:" & tPath), "utf8") into tYAML
   
   YAMLToArray tYAML
   if the result is not empty then
      return the result
   end if
   return it
end CheatSheetArray

## YAMLToArray by Monte Goulding

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
      else
         -- comment lines
         split tLine by "#"
         put tLine[1] into tLine
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
      if tLine contains ":" then
         set the itemDelimiter to ":"
         local tKey
         put item 1 of tLine into tKey
         delete item 1 of tLine
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
               __ClearQuotes tLine
               put tLine into tArray[tPath]
            end if
         end if
         
      else
         put word 1 to -1 of tLine into tArray[tPath]
      end if
   end repeat
   
   if tMultiLineMode is not kMultiLineModeNone then
      __ClearQuotes tMultiLine
      put tMultiLine into tArray[tPath]
   end if
   
   return tArray for value
end YAMLToArray

private command __ClearQuotes @xLine
   if xLine begins with quote and xLine ends with quote then
      put format( char 2 to -2 of xLine) into xLine
   else if xLine begins with "'" and xLine ends with "'" then
      put char 2 to -2 of xLine into xLine
   end if
end __ClearQuotes

private function __PathElement @xLine, @rListItem, pPathLists
   local tPathElement = 1
   
   if xLine begins with "- " then
      put true into rListItem
      delete char 1 to 2 of xLine
      return tPathElement
   end if
   
   put false into rListItem
   repeat
      if xLine begins with "    " then
         add 1 to tPathElement
         if pPathLists[tPathElement] then
            add 1 to tPathElement
         end if
      else if xLine begins with "  - " then
         -- list
         add 1 to tPathElement
         put true into rListItem
      else
         if xLine begins with "  " then
            add 1 to tPathElement
            delete char 1 to 2 of xLine
         end if
         return tPathElement
      end if
      delete char 1 to 4 of xLine
   end repeat
end __PathElement
