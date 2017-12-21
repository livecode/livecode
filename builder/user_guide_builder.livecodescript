script "UserGuideBuilder"
local sMarkdownText
local sVersion
local sOutputPath
local sEdition

private command Initialize pEdition, pVersion, pOutputPath
   if pOutputPath is empty then
      throw "Guide output path must be specified"
      exit Initialize
   end if
   
   set the defaultfolder to builderRepoFolder()
   set the hideconsolewindows to true
   
   put empty into sMarkdownText
   put pVersion into sVersion
   put pOutputPath into sOutputPath
   put pEdition into sEdition
end Initialize

private command Finalize

end Finalize

command guideBuilderRun pEdition, pVersion, pReleaseType, pOutputDir
   local tError
   builderLog "report", "Building user guide for version" && pVersion
   
   try
      Initialize pEdition, pVersion, pOutputDir
      
      BaseCreate
            
      OutputGuide
      
      Finalize
   catch tError
      builderLog "error", tError
   end try
end guideBuilderRun

----------------------------------------------------------------
-- Output
----------------------------------------------------------------

private function OutputGetGuideFilename tSuffix, pVersion
   if pVersion is empty then
      put sVersion into pVersion
   end if
   
   local tBasename
   put merge("LiveCodeUserGuide-") & replaceText(pVersion, "[-,\.]", "_") into tBasename
   return sOutputPath & slash & tBasename & "." & tSuffix
end OutputGetGuideFilename

private function OutputGetGuideTitle
   return merge("LiveCode [[sVersion]] User Guide")
end OutputGetGuideTitle

private command OutputGuide
   OutputGuideJson
   OutputGuideHtml
   OutputGuidePdf
end OutputGuide

private function OutputEscape pMarkdown, pEscapeHtml
   -- Try to do the minimum possible escaping to be able to insert the
   -- markdown into a JavaScript string
   replace "\" with "\\" in pMarkdown
   replace "'" with "\'" in pMarkdown
   replace return with "\n" in pMarkdown
   
   if pEscapeHtml then
      replace "<" with "&lt;" in pMarkdown
      replace ">" with "&gt;" in pMarkdown
   end if
   return pMarkdown
end OutputEscape

private command OutputGuideJson
   local tGuideJsonPath
   put builderGuideFolder(sEdition) & slash & "distributed_guide.js" into tGuideJsonPath
   
   local tGuideJson
   put FileGetUTF8Contents(tGuideJsonPath) into tGuideJson
   
   local tJS
   put "var tUserGuideData =" & return & "{" & return & tab & quote & "guides" & quote & ":[" into tJS
   put tGuideJson after tJS
   put return & tab & "]" & return & "}" after tJS
   
   local tGuideJSPath
   put OutputGetGuideFilename("js") into tGuideJSPath
   FileSetUTF8Contents tGuideJSPath, tJS
end OutputGuideJson

private command OutputGuideHtml
   local tHtmlTemplatePath
   put builderSystemFolder() & "/user-guide-template.html" into tHtmlTemplatePath
   
   local tHtml
   put FileGetUTF8Contents(tHtmlTemplatePath) into tHtml
   
   replace "@PREFIX@" with OutputEscape(sMarkdownText["guide"], false) in tHtml
   replace "@GUIDE_JSON@" with OutputGetGuideFilename("js") in tHtml
   
   local tOutpath
   put OutputGetGuideFilename("html") into tOutpath
   
   builderLog "report", merge("Writing [[tOutpath]]")
   FileSetUTF8Contents tOutpath, tHtml
end OutputGuideHtml

private command OutputGuidePdf
   local tCommand
   
   -- Use wkhtmltopdf to convert the HTML representation
   if $WKHTMLTOPDF is not empty then
      put $WKHTMLTOPDF into tCommand
   else
      put builderRepoFolder() & "/builder/wkhtmltopdf" into tCommand
   end if
   
   local tTitle, tDate, tHtmlPath, tPdfPath
   put OutputGetGuideTitle() into tTitle
   put the date into tDate
   put OutputGetGuideFilename("html") into tHtmlPath
   put OutputGetGuideFilename("pdf") into tPdfPath
   
   local tArgs
   put empty into tArgs
   put merge("--header-right '[[tTitle]] [[tDate]]' ") after tArgs
   put "--header-font-size 8 --header-spacing 5 " after tArgs
   put "--footer-center [page] --footer-font-size 8 --footer-spacing 5 " after tArgs
   put "--margin-top 30 --margin-bottom 20 --margin-left 20 --margin-right 20 " after tArgs
   put "--enable-internal-links --encoding UTF-8 " after tArgs
   
   local tExitCode
   builderLog "report", merge("Generating [[tPdfPath]] with [[tCommand]]")
   get shell(tCommand && tArgs && tHtmlPath && tPdfPath)
   put the result into tExitCode

   if tExitCode is not 0 then
      throw merge("Failed to run [[tCommand]]: exit code [[tExitCode]]")
   end if
end OutputGuidePdf

----------------------------------------------------------------
-- Base file sections
----------------------------------------------------------------

private command BaseCreateTitle
   MarkdownAppend "guide", "<h1 class=" & quote & "title" & quote & ">" & OutputGetGuideTitle() & "</h1>" & return
end BaseCreateTitle

private command BaseCreateContents
   MarkdownAppend "guide", "[TOC]"
end BaseCreateContents

private command BaseCreate
   builderLog "report", "Creating base user guide"
   
   BaseCreateTitle
   BaseCreateContents

end BaseCreate

----------------------------------------------------------------
-- File helpers
----------------------------------------------------------------

private function FolderListFiles pFolder, pExtension
   local tDefaultFolder
   put the defaultFolder into tDefaultFolder
   
   local tFiles
   set the defaultFolder to pFolder
   put the files into tFiles
   
   if pExtension is not empty then
      filter tFiles with "*." & pExtension
   end if
   
   set the defaultFolder to tDefaultFolder
   
   return tFiles
end FolderListFiles

private function FileGetBasename pPath
   set the itemdelimiter to slash
   put item -1 of pPath into pPath
   set the itemdelimiter to "."
   if 1 < the number of items in pPath then
      put item 1 to -2 of pPath into pPath
   end if
   return pPath
end FileGetBasename

private function FileGetPath pType
   switch pType
      case "ide-guides"
         return builderIDEFolder() & "/Documentation/guides"
      case "repo-guides"
         return builderRepoFolder() & "/docs/guides"
      default
         return pType
   end switch
end FileGetPath

private function FileGetUTF8Contents pPath
   return FileGetContents(pPath, "UTF-8")
end FileGetUTF8Contents

private function FileGetContents pPath, pEncoding
   local tContents, tResult
   
   if pEncoding is empty then
      open file pPath for binary read
   else
      open file pPath for pEncoding text read
   end if
   
   put the result into tResult
   if tResult is not empty then
      throw merge("Failed to open [[pPath]]: [[tResult]]")
   end if
   
   read from file pPath until end
   put the result into tResult
   if tResult is not empty and tResult is not "eof" then
      throw merge("Failed to read [[pPath]]: [[tResult]]")
   end if
   put it into tContents
   
   close file pPath -- unchecked
   
   return tContents
end FileGetContents

private command FileSetUTF8Contents pPath, pContents
   local tResult
   
   open file pPath for "UTF-8" text write
   put the result into tResult
   if tResult is not empty then
      throw merge("Failed to open [[pPath]]: [[tResult]]")
   end if
   
   write pContents to file pPath
   put the result into tResult
   if tResult is not empty then
      throw merge("Failed to write [[pPath]]: [[tResult]]")
   end if
   
   close file pPath -- unchecked
end FileSetUTF8Contents

----------------------------------------------------------------
-- Markdown helpers
----------------------------------------------------------------

private command MarkdownAppend pKey, pText
   put pText & return after sMarkdownText[pKey]
end MarkdownAppend

/*
Markdown files can have a YAML-like metadata block at the start.
The format is very inflexible.  The block is delimited by lines
of exactly three hyphens "---", and every line in between *must*
be blank or contain a "key: value" pair.  For example:

---
# Comment
version: 8.0.0-dp-9
---

*/
private command MarkdownSplitMetadata pText, @rMarkdown, @rMetadata
   -- Check for a metadata block
   if the first line of pText is not "---" then
      put empty into rMetadata
      put pText into rMarkdown
      exit MarkdownSplitMetadata
   end if
   
   -- Search for a terminal "---" line
   local tMetadataEnd
   put lineOffset("---", pText, 1) into tMetadataEnd
   if tMetadataEnd is 1 then
      throw "Unterminated metadata header"
   end if
   
   local tLine
   set the itemdelimiter to colon
   repeat for each line tLine in (line 2 to tMetadataEnd of pText)
      put word 1 to -1 of tLine into tLine
      if item 1 of tLine is empty or char 1 of tLine is "#" then
         next repeat
      else
         put word 1 to -1 of (item 2 to -1 of tLine) into rMetadata[item 1 of tLine]
      end if
   end repeat
   
   put line (tMetadataEnd + 2) to -1 of pText into rMarkdown
end MarkdownSplitMetadata

----------------------------------------------------------------
-- HTML helper functions
----------------------------------------------------------------

private function HtmlEscape pText
   replace "&" with "&amp;" in pText
   replace "<" with "&lt;" in pText
   replace ">" with "&gt;" in pText
   replace quote with "&quot;" in pText
   return ptext
end HtmlEscape
