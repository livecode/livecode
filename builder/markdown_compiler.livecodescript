script "MarkdownCompiler"
/*

Compiles LiveCode flavoured markdown files to HTML

MD:
##... <headings>

* <unordered list element>
1. <ordered list element>

*<italic text>*

**<bold text>**

tab tab ... <indented text>

Images
![<alt id>](<image file>)

Videos
!! [<alt id>](<video file>)

Links
[<link text>](<link url>)

!- <Note:> -!
` <code> `

|table header 1|table header 2|
--------------------------------
|table contents|table contents|
|table contents|table contents|
   
   */
   
   global gImgRegex, gLinkRegex, gVidRegex
   global gDocumentPrefix, gDocumentSuffix
   global gHeading, gItalic, gList, gIndent, gImg
   global gAnchorCount, gExtension
   
on libraryStack
   markdownInit
end libraryStack
   
command markdownInit
   put 1 into gAnchorCount
   
   put "md" into gExtension
   
   put "<html>" into gDocumentPrefix
   put "</html>" into gDocumentSuffix
   
   put "#" into gHeading
   put "*" into gItalic -- bold is "**"
   put "*" into gList
   put "  " into gIndent
   put "!" into gImg
   put "!\[(.*)\]\((.*)\)" into gImgRegex
   put "!!\[.*\]\((.*)\)" into gVidRegex
   put "\[([^\]]*)\]\(([^\)]*)\)" into gLinkRegex
   end markdownInit

function getIndentMargin pIndents
   return (quote & "margin-left:" & 2*pIndents & "em;" & quote)
end getIndentMargin

function processFileToHTML pFile, @xContents
   local tFileName
   local tFileUrl
   local tFileText
   local tBody
   put pFile & "." & gExtension into tFileName
   put "file:" & notesPath() & slash & tFileName into tFileUrl
   put url tFileUrl into tFileText
   if tFileText is not empty then
      markdownToHTML tFileText, 1, 0, xContents, tBody, false, true
   end if
   return tBody
end processFileToHTML

function parseEnclosingMarkup pText
   local tRegex
   local tFound
   put "(\/\*|\*\*|\*|`|!-)"  into tRegex
   local tStart, tEnd
   if matchChunk(pText, tRegex, tStart, tEnd) is false then
      return pText
   else 
      local tParsed
      local tInsides
      if char tStart-1 of pText is "\" then 
         put char 1 to tStart-2 of pText & char tStart to tEnd of pText into tParsed
         put char tEnd + 1 to -1 of pText into tInsides
         put tParsed & parseEnclosingMarkup(tInsides) into tParsed
         return tParsed
      end if
      local tWhich
      put char 1 to tStart - 1 of pText into tParsed
      put char tStart to tEnd of pText into tWhich
      put char tEnd + 1 to -1 of pText into pText
      if tWhich is "/*" then
         put "[^\\](\*\/)" into tRegex
         get matchChunk(pText, tRegex, tStart, tEnd)
      else if tWhich is "*" then
         put "[^\\](\*)" into tRegex
         put matchChunk(pText, tRegex, tStart, tEnd) into tFound
         repeat while tFound is true and char tStart + 1 of pText is "*"
            put tInsides & char 1 to tStart + 1 of pText into tInsides
            put char tStart + 2 to -1 of pText into pText
            put matchChunk(pText, tRegex, tStart, tEnd) into tFound
         end repeat
         if tFound is false then
            put tInsides into pText
         end if
      else if tWhich is "**" then
         put "[^\\](\*\*)" into tRegex
         put matchChunk(pText, tRegex, tStart, tEnd) into tFound
      else if tWhich is "!-" then
         put "[^\\](-!)" into tRegex
         put matchChunk(pText, tRegex, tStart, tEnd) into tFound
      else 
         put "[^\\](" & tWhich & ")" into tRegex
         put matchChunk(pText, tRegex, tStart, tEnd) into tFound
      end if
      if tFound is false then
         return tParsed & parseEnclosingMarkup(pText)
      end if
      put tInsides & char 1 to tStart - 1 of pText into tInsides
      put char tEnd + 1 to -1 of pText into pText
      if tWhich is "**" then
         put tParsed & "<strong>" & parseEnclosingMarkup(tInsides) & "</strong>" into tParsed
      else if tWhich is "*" then
         put tParsed & "<em>" & parseEnclosingMarkup(tInsides) & "</em>" into tParsed
      else if tWhich is "`" then 
         put tParsed & "<code>" & parseEnclosingMarkup(tInsides) & "</code>" into tParsed
      else if tWhich is "!-" then
         put tParsed & "<p class=" & quote & "warning" & quote & ">" & parseEnclosingMarkup(tInsides) & "</p>" into tParsed
      else if tWhich is "/*" then
         put tParsed & "<p class=" & quote & "note" & quote & ">" & parseEnclosingMarkup(tInsides) & "</p>" into tParsed
      end if
      put tParsed & parseEnclosingMarkup(pText) into tParsed
      return tParsed
   end if
end parseEnclosingMarkup

function countOccurrences pOccurrer, pText
   local tOffset
   put offset(pOccurrer, pText) into tOffset
   if tOffset is 0 then 
      return 0
   else
      local tEscape
      put char tOffset -1 of pText into tEscape
      put char tOffset + 1 to -1 of pText into pText
      if tEscape is "\" then
         return countOccurrences(pOccurrer, pText)
      else
         return 1 + countOccurrences(pOccurrer, pText)
      end if
   end if 
end countOccurrences

function countConsecutive pOccurrer, pText
   local tCount, tLength
   put the number of chars in pOccurrer into tLength
   put 0 into tCount
   repeat while pText begins with pOccurrer
      add 1 to tCount
      delete char 1 to tLength of pText
   end repeat
   return tCount
end countConsecutive

function getAnchor pTitle
   local tLink
   put replaceText(pTitle, quote, "_") into tLink
   put word 1 of tLink & gAnchorCount into tLink
   return "id=" & quote & tLink & quote
end getAnchor 

command appendLink pIndent, pTitle, @rContents 
   local tLink
   put replaceText(pTitle, quote, "_") into tLink
   put word 1 of tLink & gAnchorCount into tLink
   put rContents & "<a " into rContents
   put rContents & "style = " & quote & "margin-left:" & pIndent-1 & "em;" & quote & " " into rContents
   put rContents & "href=" & quote & "#" & tLink & quote & ">" & pTitle & "</a><br>" & CR into rContents
   add 1 to gAnchorCount
end appendLink

command addLink @pText, pLinkText, pUrl
   local tLinkStart
   local tLinkEnd
   get matchChunk(pText, "(" & gLinkRegex & ")", tLinkStart, tLinkEnd)
   local tBegin
   local tEnd
   put char 0 to tLinkStart-1 of pText into tBegin
   put char tLinkEnd + 1 to -1 of pText into tEnd
   if pLinkText is empty then
      put pUrl into pLinkText
   end if
   put tBegin & "<a href=" & quote & pUrl & quote & ">" & pLinkText &"</a>" & tEnd into pText
end addLink

command addVideo @pText, pUrl
   local tVidStart
   local tVidEnd
   get matchChunk(pText, "(" & gVidRegex & ")", tVidStart, tVidEnd)
   local tBegin
   local tEnd
   put char 0 to tVidStart-1 of pText into tBegin
   put char tVidEnd + 1 to -1 of pText into tEnd
   local tVidOpts
   put " height=" & quote & "100%" & quote & " width=" & quote & "100%" & quote & " frameborder=" & quote & "0" & quote  into tVidOpts
   put tBegin & "<iframe src=" & quote & pUrl & quote & tVidOpts & "></iframe>" & tEnd into pText
end addVideo

function createTableLine pLine, pIsHeader
   local tNewLine
   local tOffset
   put "<tr>" & CR into tNewLine
   set the itemdelimiter to "|"
   repeat with i = 1 to the number of items of pLine
      if pIsHeader then
         put tNewLine & "<th>" & item i of pLine & "</th>" & CR into tNewLine
      else
         put tNewLine & "<td>" & item i of pLine & "</td>" & CR into tNewLine
      end if
   end repeat  
   put tNewLine & "</tr>" into tNewLine
   return tNewLine
end createTableLine

function markdownFileToHTML pFileName
   local tText
   put url ("file:" & pFileName) into tText
   markdownInit
   local tContents
   local tBody
   markdownToHTML "", tText, 0, 0, tContents, tBody, false, true
   return (tContents & CR & tBody)
end markdownFileToHTML

command addImage @pText, pAlt, pSrc
   local tImgStart
   local tImgEnd
   get matchChunk(pText, "(" & gImgRegex & ")", tImgStart, tImgEnd)
   local tBegin
   local tEnd
   put char 0 to tImgStart - 1 of pText into tBegin
   put char tImgEnd + 1 to -1 of pText into tEnd
   put tBegin & "<img alt=" & quote & pAlt & quote & " src=" & quote & pSrc & quote & "></img>" & tEnd into pText
end addImage

function markdownToHTML @pText, pHLevel, pCLevel, @xContents, pTopLevel
   replace "<" with "&lt;" in pText
   replace ">" with "&gt;" in pText
   
   local tParsedBody
   local tParsedContents
   local tNumLines
   put the number of lines of pText into tNumLines
   
   local tCurLine
   local tCurHeading
   local tHValue
   local tListDepth
   local tListPrefixStart
   local tListPrefixEnd
   local tQuoteDepth
   local tIsOrdered
   local tNumIndents
   local tClosers
   
   local tImageUrl
   local tVidUrl
   local tAltText
   local tLinkRef
   local tLinkText
   local tStart
   local tEnd
   
   local tSectionLine
   local tLine
   
   repeat with x = 1 to tNumLines
      if tClosers is empty then
         put 0 into tListDepth
         put 0 into tQuoteDepth
      end if
      put line x of pText into tCurLine
      
      # tCurLine - line currently being parsed
      # tParsedBody - text already parsed
      # tClosers - corresponding closing tags of any open ones
      
      # whenever the line starts and ends with | then we are definitely in a table
      if tCurLine begins with "|" and tCurLine ends with "|" then
         if tClosers is "</table>" then    # if we are already in a table then continue it
            put createTableLine(tCurLine, false) into tCurLine
         else # otherwise start a new table
            if tClosers is not empty then
               put tParsedBody & tClosers & CR  into tParsedBody
               put empty into tClosers
            end if
            put tParsedBody & "<table>" & CR into tParsedBody
            put "</table>" into tClosers
            if matchText(line x+1 of pText, "[^-]") then # if the next line is not all -
               put createTableLine(tCurLine, false) into tCurLine # then continue the table as usual
               else
                  put createTableLine(tCurLine, true) into tCurLine # otherwise this line is a table header 
                  add 1 to x #and the next line should be skipped
               end if
            end if
         else
            if tClosers is "</table>" then # if this line is not a table line then close the table tag
               put tParsedBody & tClosers & CR into tParsedBody
               put empty into tClosers
            end if
         end if
         
         #  remove any indents from the beginning of the line and record the number
         put countConsecutive(gIndent, tCurLine) into tNumIndents
         put char (the number of chars in gIndent * tNumIndents) + 1 to -1 of tCurLine into tCurLine
         
         # check if the line starts with any list characters. If it starts with *, check if there are an odd number, otherwise the star is for bold / italic markup.
         if (tCurLine begins with gList and countOccurrences (gList, tCurLine) mod 2 is 1) or (matchChunk(tCurLine, "^([0-9]+\.[[0-9]*\.*]*)", tListPrefixStart, tListPrefixEnd)) then
            if tClosers begins with "</p>" then # if we were in a paragraph then close the tag
               put tParsedBody & char 1 to 4 of tClosers & CR into tParsedBody
               put char 5 to -1 of tClosers into tClosers
               put 0 into tQuoteDepth
            end if
            if tCurLine begins with gList then # check if the list should be ordered or unordered
               put char 2 to -1 of tCurLine into tCurLine
               put false into tIsOrdered
            else
               put char tListPrefixEnd + 1 to -1 of tCurLine into tCurLine
               put true into tIsOrdered
            end if
            if tListDepth is tNumIndents + 1 then # we are just continuing a list
               if tIsOrdered and char 3 of tClosers is "u" then
                  put tParsedBody & "</ul>" & CR & "<ol>" & CR into tParsedBody
                  put "</ol>" & char 6 to -1 of tClosers into tClosers
               else if tIsOrdered is false and char 3 of tClosers is "o" then
                  put tParsedBody & "</ol>" & CR & "<ul>" & CR into tParsedBody
                  put "</ul>" & char 6 to -1 of tClosers into tClosers
               end if
               put "<li>" & tCurLine into tCurLine
            else 
               if tListDepth <= tNumIndents then # we are adding nesting to a list
                  repeat with i = 1 to tNumIndents + 1 - tListDepth # open the appropriate number of list tags
                     if tIsOrdered then 
                        put tParsedBody & "<ol>" & CR into tParsedBody
                        put "</ol>" & tClosers into tClosers
                     else
                        put tParsedBody & "<ul>" & CR into tParsedBody
                        put "</ul>" & tClosers into tClosers
                     end if
                  end repeat
               else # we are substracting nesting from a list
                  repeat with i = 1 to tListDepth - 1 - tNumIndents # close the appropriate number of list tags
                     put tParsedBody & char 1 to 5 of tClosers & CR into tParsedBody
                     put char 6 to -1 of tClosers into tClosers
                  end repeat
               end if
               put "<li>" & tCurLine into tCurLine # this line is a list item
               put tNumIndents + 1 into tListDepth # update the list nesting count
            end if
         else # this line is either not part of a list, or it is a line break in a list
            if tListDepth is not 0 then 
               if tListDepth is tNumIndents then # it is a line break in a list item
                  put "<br>" after line -1 of tParsedBody
               else # not in a list, so close the appropriate number of tags
                  repeat with i = 1 to tListDepth
                     put tParsedBody & char 1 to 5 of tClosers & CR into tParsedBody
                     put char 6 to -1 of tClosers into tClosers
                  end repeat
                  put 0 into tListDepth
               end if
            end if
         end if
         
         if tCurLine begins with gHeading then # if this is a heading then
            if tClosers is not empty then
               put tParsedBody & tClosers & CR into tParsedBody # close all open tags
               put empty into tClosers
            end if
            put 0 into tHValue
            repeat while char tHValue + 1 of tCurLine is gHeading # count the hashes
               add 1 to tHValue 
            end repeat
            if matchText(char tHvalue + 1 to -1 of tCurLine, "(.*) \(contents\)", tCurHeading) then # if we require a table of contents for this section
               local tHashes
               put empty into tHashes
               repeat with j = 0 to tHValue
                  put tHashes & "#" into tHashes
               end repeat
               local tLocalConts
               local tLocalBody
               put x + 1 into tSectionLine
               repeat until ((line tSectionLine of pText begins with "#") and (line tSectionLine of pText begins with tHashes is false)) or tSectionLine > tNumLines # find out where the section ends
                  add 1 to tSectionLine
               end repeat
               local tSection
               put line x + 1 to tSectionLine -1 of pText into tSection
               put markdownToHTML(tSection, pHLevel, pCLevel - 1, tLocalConts, false) into tLocalBody # and recursively call markdownToHTML on that section, including its 'local' contents in the body
               put tParsedBody & "<h" & pHLevel + tHValue & " " & getAnchor(tCurHeading) & ">" & tCurHeading & "</h" & pHLevel + tHValue & ">" into tParsedBody
               put tParsedBody & CR & tLocalConts & CR & tLocalBody into tParsedBody
               put empty into tCurLine
               put tSectionLine - 1 into x
            else # otherwise it's just a new section
               put  char tHValue + 1 to -1 of tCurLine into tCurHeading
               put "<h" & pHLevel + tHValue & " " & getAnchor(tCurHeading) & ">" & tCurHeading & "</h" & pHLevel + tHValue & ">" into tCurLine
            end if
            appendLink pCLevel + tHValue,tCurHeading, tParsedContents # add it to the table of contents
         else
            if tCurLine begins with "!-" or tCurLine begins with "/*" or tCurLine begins with "`" then # for blocks of warnings (!-), notes (/*) or code (`), don't do any further processing to the block
               local tClosing
               switch
                  case tCurLine begins with "!-"
                     put "-!" into tClosing
                     break
                  case tCurLine begins with "/*"
                     put "*/" into tClosing
                     break
                  case tCurLine begins with "`"
                     put "`" into tClosing
                     break
               end switch
               if tClosers is not empty then # close any open tags
                  put tParsedBody & tClosers & CR into tParsedBody
                  put empty into tClosers
               end if
               
               # Ensure this is a multiline element
               if not char 2 to -1 of tCurLine contains tClosing then
                  put x into tSectionLine
                  put tCurLine into tLine
                  if tNumIndents > 0 then # if there were indents before the block marker, put them inside the block
                     local tTabs
                     repeat with k = 1 to tNumIndents
                        put tTabs & gIndent into tTabs
                     end repeat
                     if tLine begins with "`" then
                        put char 1 of tLine & tTabs & char 2 to -1 of tLine into tLine
                     else
                        put char 1 to 2 of tLine  & tTabs & char 3 to -1 of tLine into tLine
                     end if
                     put empty into tTabs
                  end if
                  repeat until tLine ends with tClosing or  tSectionLine is tNumLines # find the end of the section
                     local tLinkUrl
                     repeat while matchText(tLine,gLinkRegex, tLinkText, tLinkUrl) # linkify any links
                        addLink tLine, tLinkText, tLinkUrl
                     end repeat
                     put tParsedBody & tLine & "<br>" & CR into tParsedBody # adding <br> tags to each line
                     add 1 to tSectionLine
                     put line tSectionLine of pText into tLine
                  end repeat
                  repeat while matchText(tLine,gLinkRegex, tLinkText, tLinkUrl) # check for links in the last line
                     addLink tLine, tLinkText, tLinkUrl
                  end repeat
                  put tParsedBody & tLine & CR into tParsedBody
                  put tSectionLine into x
                  put empty into tCurLine
               end if
            else if tCurLine is empty then # if the line is empty
               if tClosers is not empty then
                  put tParsedBody & tClosers & CR into tParsedBody
                  put empty into tClosers
               end if
            else 
               if tListDepth is 0 and tNumIndents <> tQuoteDepth then # if the indent has changed
                  if tClosers is not empty then
                     put tParsedBody & tClosers & CR into tParsedBody 
                     put empty into tClosers
                  end if
                  put 0 into tQuoteDepth
                  if tNumIndents is not 0 then # if there is still indent then open a paragraph appropriately
                     put tParsedBody & "<p style=" & getIndentMargin(tNumIndents) & ">" & CR into tParsedBody
                     put tNumIndents into tQuoteDepth
                     put "</p>" & tClosers into tClosers
                  end if
               end if
               if tClosers is empty then
                  put "<p>" & tCurLine into tCurLine
                  put "</p>" into tClosers
               end if
               if tClosers is "</p>" then
                  put tCurLine & "<br>" into tCurLine
               end if
            end if
         end if
         repeat while matchChunk(tCurLine, "(" & gVidRegex & ")", tStart, tEnd) # linkify any links
            if char tStart -1 of tCurLine is "\" then # unless there is an escape character
               put tParsedBody & char 1 to tStart - 2 of tCurLine &  char tStart to tEnd of tCurLine into tParsedBody
               put char tEnd + 1 to -1 of tCurLine into tCurLine
            else
               get matchText(tCurLine, gVidRegex, tVidUrl)
               addVideo tCurLine, tVidUrl
            end if
         end repeat
         repeat while matchChunk(tCurLine, "(" & gImgRegex & ")", tStart, tEnd) # imagify any images
            if char tStart -1 of tCurLine is "\" then # unless there is an escape character
               put tParsedBody & char 1 to tStart - 2 of tCurLine &  char tStart to tEnd of tCurLine into tParsedBody
               put char tEnd + 1 to -1 of tCurLine into tCurLine
            else
               get matchText(tCurLine,gImgRegex, tAltText, tImageUrl)
               addImage tCurLine, tAltText, tImageUrl
            end if
         end repeat
         repeat while matchChunk(tCurLine, "(" & gLinkRegex & ")", tStart, tEnd) # linkify any links
            if char tStart -1 of tCurLine is "\" then # unless there is an escape character
               put tParsedBody & char 1 to tStart - 2 of tCurLine &  char tStart to tEnd of tCurLine into tParsedBody
               put char tEnd + 1 to -1 of tCurLine into tCurLine
            else
               get matchText(tCurLine, gLinkRegex, tLinkText, tLinkUrl)
               addLink tCurLine, tLinkText, tLinkUrl
            end if
         end repeat
         if tCurLine is not empty then # if the current line has anything in it after that onslaught, then add it to the parsed body
            put tParsedBody & tCurLine & CR into tParsedBody
         end if
      end repeat
      
      # now close any open tags
      put tParsedBody & tClosers into tParsedBody
      
      # and deal with the 'inline' markdown elements (but not in recursive calls)
      if pTopLevel then
         put parseEnclosingMarkup(tParsedBody) into tParsedBody
      end if
      
      put xContents & tParsedContents into xContents
      return tParsedBody
end markdownToHTML
