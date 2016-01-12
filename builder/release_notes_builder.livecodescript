script "ReleaseNotesBuilder"
local sMarkdownText
local sVersion
local sOutputPath

private command Initialize pVersion, pOutputPath
   start using stack (builderSystemFolder() & slash & "markdown_compiler.livecodescript")
   set the defaultfolder to builderRepoFolder()
   set the hideconsolewindows to true
   
   put empty into sMarkdownText
   put pVersion into sVersion
   put pOutputPath into sOutputPath
end Initialize

private command Finalize
   stop using stack (builderSystemFolder() & slash & "markdown_compiler.livecodescript")
end Finalize

command releaseNotesBuilderRun pEdition, pVersion, pReleaseType, pOutputDir
   local tError
   builderLog "report", "Building release notes for version" && pVersion
   
   try
      Initialize pVersion, pOutputDir
      
      BaseCreate
      V1NotesCreate "engine", "Engine changes"
      V1NotesCreate "ide", "IDE changes"
      V2NotesCreate "lcb", "LiveCode Builder changes"
      ExtensionsCreate
      DictionaryCreate
      PreviousCreate
      
      OutputNotes
      OutputUpdates
      
      Finalize
      
      return FileGetUTF8Contents(OutputGetUpdatesFilename("html"))
   catch tError
      builderLog "error", tError
   end try
end releaseNotesBuilderRun

private function NotesGetExperimentalText
   return "<div class=" & quote & "experimental" & quote & ">" & \
         "Important: This feature is currently experimental.  This means it may not be complete, or may fail in some circumstances that you would expect it to work.  Please do not be afraid to try it out as we need feedback to develop it further." & \
         "</div>"
end NotesGetExperimentalText

----------------------------------------------------------------
-- Output
----------------------------------------------------------------

private function OutputGetNotesFilename tSuffix, pVersion
   if pVersion is empty then
      put sVersion into pVersion
   end if
   
   local tBasename
   put "LiveCodeNotes-" & replaceText(pVersion, "[-,\.]", "_") into tBasename
   return sOutputPath & slash & tBasename & "." & tSuffix
end OutputGetNotesFilename

private function OutputGetUpdatesFilename tSuffix, pVersion
   if pVersion is empty then
      put sVersion into pVersion
   end if
   
   local tBasename
   put "LiveCodeUpdates-" & replaceText(pVersion, "[-,\.]", "_") into tBasename
   return sOutputPath & slash & tBasename & "." & tSuffix
end OutputGetUpdatesFilename

private function OutputGetNotesUrl tSuffix, pVersion
   put replaceText(pVersion, "[-,\.]", "_") into pVersion
   
   return "https://downloads.livecode.com/livecode/" & pVersion & \
         "/LiveCodeNotes-" & pVersion & "." & tSuffix
end OutputGetNotesUrl

private function OutputGetNotesTitle
   return merge("LiveCode [[sVersion]] Release Notes")
end OutputGetNotesTitle

private command OutputNotes
   OutputNotesMarkdown
   OutputNotesHtml
   OutputNotesPdf
end OutputNotes

private command OutputNotesMarkdown
   local tPath
   put OutputGetNotesFilename("md") into tPath
   builderLog "report", merge("Writing [[tPath]]")
   FileSetUTF8Contents tPath, sMarkdownText["notes"]
end OutputNotesMarkdown

private command OutputNotesHtml
   -- Try to do the minimum possible escaping to be able to insert the
   -- markdown into a JavaScript string
   local tEscaped
   put sMarkdownText["notes"] into tEscaped
   replace "\" with "\\" in tEscaped
   replace "'" with "\'" in tEscaped
   replace return with "\n" in tEscaped
   
   local tHtmlTemplatePath
   put builderSystemFolder() & "/release-notes-template.html" into tHtmlTemplatePath
   
   local tHtml
   put FileGetUTF8Contents(tHtmlTemplatePath) into tHtml
   replace "@MARKDOWN@" with tEscaped in tHtml
   
   local tOutpath
   put OutputGetNotesFilename("html") into tOutpath
   
   builderLog "report", merge("Writing [[tOutpath]]")
   FileSetUTF8Contents tOutpath, tHtml
end OutputNotesHtml

private command OutputNotesPdf
   local tCommand
   
   -- Use wkhtmltopdf to convert the HTML representation
   if $WKHTMLTOPDF is not empty then
      put $WKHTMLTOPDF into tCommand
   else
      put builderRepoFolder() & "/builder/wkhtmltopdf" into tCommand
   end if
   
   local tTitle, tDate, tHtmlPath, tPdfPath
   put OutputGetNotesTitle() into tTitle
   put the date into tDate
   put OutputGetNotesFilename("html") into tHtmlPath
   put OutputGetNotesFilename("pdf") into tPdfPath
   
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
end OutputNotesPdf

private command OutputUpdates  
   OutputUpdatesMarkdown
   OutputUpdatesHtml
end OutputUpdates

private command OutputUpdatesMarkdown
   local tPath
   put OutputGetUpdatesFilename("md") into tPath
   builderLog "report", merge("Writing [[tPath]]")
   FileSetUTF8Contents tPath, sMarkdownText["updates"] 
end OutputUpdatesMarkdown

private command OutputUpdatesHtml
   local tHtml, tToc
   put markdownToHtml(sMarkdownText["updates"], 3, 0, tToc, true) into tHtml
   
   local tPath
   put OutputGetUpdatesFilename("html") into tPath
   builderLog "report", merge("Writing [[tPath]]")
   FileSetUTF8Contents tPath, tHtml
end OutputUpdatesHtml

----------------------------------------------------------------
-- Base file sections
----------------------------------------------------------------

private command BaseCreate
   builderLog "report", "Creating base release notes"
   
   BaseCreateTitle
   BaseCreateContents
   BaseCreateOverview
   BaseCreateIssues
   
   MarkdownAppend "notes", BaseReadFile("platforms")
   MarkdownAppend "notes", BaseReadFile("setup")
   MarkdownAppend "notes", BaseReadFile("proposed_changes")
   
   BaseCreateUpdates
end BaseCreate

private command BaseCreateTitle
   MarkdownAppend "notes", "<h1 class=" & quote & "title" & quote & ">" & OutputGetNotesTitle() & "</h1>" & return
end BaseCreateTitle

private command BaseCreateContents
   MarkdownAppend "notes", "[TOC]"
end BaseCreateContents

private command BaseCreateOverview
   local tOverview
   
   put BaseReadFile("overview") into tOverview
   
   if tOverview is empty then
      put "# Overview" & return & return after tOverview
      put merge("This document describes all the changes that have been made for LiveCode [[sVersion]], including bug fixes and new syntax.") after tOverview
   end if
   
   MarkdownAppend "notes", tOverview
end BaseCreateOverview

private command BaseCreateIssues
   local tIssues
   put BaseReadFile("issues") into tIssues
   
   if tIssues is empty then
      put "# Known Issues" & return & return after tIssues
      put "There are no known issues with this release." after tIssues
   end if
   
   MarkdownAppend "notes", tIssues
end BaseCreateIssues

private function BaseReadFile pBasename
   local tPath
   put FileGetPath() & slash & pBasename & ".md" into tPath
   if there is a file tPath then
      return FileGetUTF8Contents(tPath)
   else
      return empty
   end if
end BaseReadFile

private command BaseCreateUpdates
   MarkdownAppend "updates", merge("LiveCode [[sVersion]] is now available.") & return
   MarkdownAppend "updates", "Changes in this release include:" & return
end BaseCreateUpdates

----------------------------------------------------------------
-- Old-style release notes
----------------------------------------------------------------

/*
The IDE and the engine use "old-style" release notes.  These are
markdown fragments that have "feature" or "bugfix" in their
filenames.

Each file details a single bug fix or feature implementation.

* Name the files "bugfix-<bug number>.md" or
"feature-<description>.md"

* Put the title as the first line, e.g. "# <title>"

Single-line files are added to the table of bugfixes.  Multi-line
files get their own section in the release notes.
*/
private command V1NotesCreate pType, pTitle
   builderLog "report", merge("Creating [[pType]] release notes")
   
   MarkdownAppend "notes", merge("# [[pTitle]]")
   MarkdownAppend "updates", merge("# [[pTitle]]")
   
   -- First, gather all files, their contents, and their metadata
   local tScannedNotes
   put V1NotesScan(pType) into tScannedNotes
   
   -- Next generate markdown
   V1NotesGenerate pType, tScannedNotes
end V1NotesCreate

/*
Build up a datastructure containing all the (raw) data gathered
from the target directory.  The return value is a nested array:

{
"<version>": {
"<filename>": {
"basename": "<filename without suffix or path>",
"markdown": "<file content>",
"metadata": { <metadata key-value pairs> }
}
}
}
*/
private function V1NotesScan pType
   local tTags, tNumTags
   
   put GitGetRelevantTags(pType) into tTags
   put the number of lines in tTags into tNumTags
   
   local tResult
   put empty into tResult
   
   local tVersion, tBaseVersion, tPrevVersion, tTagOffset
   local tAllFiles, tVersionFiles, tFile
   put line 1 of tTags into tBaseVersion
   put tBaseVersion into tVersion
   
   -- Get a list of all files to be considered
   put GitGetChangedFiles(pType, tBaseVersion, line -1 of tTags) into tAllFiles
   
   put 2 into tTagOffset
   repeat while tTagOffset <= tNumTags
      put tVersion into tPrevVersion
      put line tTagOffset of tTags into tVersion
      
      -- Figure out which files should be considered for this version
      put empty into tVersionFiles
      repeat for each line tFile in GitGetChangedFiles(pType, tPrevVersion, tVersion)
         if tFile is among the lines of tAllFiles then
            put tFile & return after tVersionFiles
         end if
      end repeat
      
      V1NotesScanVersion pType, tVersion, tVersionFiles, tResult
      
      add 1 to tTagOffset
   end repeat
   
   return tResult
end V1NotesScan

private command V1NotesScanVersion pType, pVersion, pFiles, @xScan
   
   local tFile, tOldFile, tContents, tError, tFileInfo, tVersion, tBasename
   repeat for each line tFile in pFiles
      
      -- If this file was already processed once before, delete its
      -- record (we'll generate a new one)
      repeat for each key tVersion in xScan
         if tFile is among the keys of xScan[tVersion] then
            delete variable xScan[tVersion][tFile]
         end if
      end repeat
      
      try
         put FileGetUTF8Contents(tFile) into tContents
      catch tError
         builderLog "report", tERror
         next repeat
      end try
      
      -- Extract metadata & markdown sections
      MarkdownSplitMetadata tContents, tFileInfo["markdown"], tFileInfo["metadata"]
      
      -- Allow automatically-generated information to be overwritten by metadata
      put tFileInfo["metadata"]["version"] into tVersion
      if tVersion is empty then
         put pVersion into tVersion
      end if
      put tFileInfo["metadata"]["basename"] into tBasename
      if tBasename is empty then
         put FileGetBasename(tFile) into tBasename
      end if
      
      put tBasename into tFileInfo["basename"]
      put tFileInfo into xScan[tVersion][tFile]
   end repeat
end V1NotesScanVersion

private command V1NotesGenerate pType, pScanData
   local tBugInfo
   
   -- Process versions in reverse order
   local tTags, tTagCount
   put the keys of pScanData into tTags
   GitSortTags tTags
   
   put the number of lines in tTags into tTagCount
   
   local tTag
   repeat tTagCount times
      put line tTagCount of tTags into tTag
      v1NotesGenerateVersion pType, tTag, pScanData[tTag], tBugInfo
      subtract 1 from tTagCount
   end repeat
   
   BugGenerate tBugInfo
end V1NotesGenerate

private command V1NotesGenerateVersion pType, pVersion, pScanData, @xBugInfo
   local tFile, tFeatures, tBugfixes, tNote
   
   set the itemdelimiter to slash
   
   -- Split note files into features vs bugfixes
   local tFileInfo
   repeat for each key tFile in pScanData
      put pScanData[tFile] into tFileInfo
      
      if tFileInfo["basename"] contains "feature" then
         put tFileInfo["basename"] & slash & tFile & return after tFeatures
      else if tFileInfo["basename"] contains "bugfix" then
         put tFileInfo["basename"] & slash & tFile & return after tBugfixes
      end if
   end repeat
   
   -- Process features first
   sort lines of tFeatures by item 1 of each
   repeat for each line tNote in tFeatures
      put item 2 to -1 of tNote into tFile
      V1NotesGenerateFeature pType, pVersion, pScanData[tFile]
   end repeat
   
   -- Now process bugfixes
   sort lines of tBugfixes by item 1 of each
   repeat for each line tNote in tBugfixes
      put item 2 to -1 of tNote into tFile
      V1NotesGenerateBugfix pType, pVersion, pScanData[tFile], xBugInfo
   end repeat
end V1NotesGenerateVersion

private function V1NotesGetBugId pFileInfo
   local tBasename, tBugId
   put pFileInfo["basename"] into tBasename
   
   -- Try strict mode first, and issue a warning if no match
   get matchText(tBasename, "(?i)^bugfix-(\d*)$", tBugId)
   if tBugId is not empty then
      return tBugId
   end if
   
   -- Now try again, more tolerantly
   builderLog "warning", merge("Probably misnamed bug fix note '[[tBasename]]'")
   
   get matchText(pFileInfo["basename"], "(?i)bugfix-(\d*)", tBugId)
   if tBugId is empty then
      throw "Could not determine bug ID for '[[tBasename]]'"
   end if
   return tBugId
end V1NotesGetBugId

private command V1NotesGenerateBugfix pType, pVersion, pFileInfo, @xBugInfo
   local tBugId, tHeader, tBody
   
   put V1NotesGetBugId(pFileInfo) into tBugId
   
   V1NotesSplitHeader pType, pFileInfo, tHeader, tBody
   
   if tHeader is empty then
      exit V1NotesGenerateBugfix
   end if
   
   BugAddInfo xBugInfo, pVersion, tBugId, tHeader
   
   -- If the body is non-empty, generate a paragraph in the
   -- release notes for this bugfix
   if tBody is not empty then
      V1NotesGenerateBlock pVersion, tHeader, tBody
   end if
end V1NotesGenerateBugfix

private command V1NotesGenerateFeature pType, pVersion, pFileInfo
   local tBasename, tHeader, tBody
   put pFileInfo["basename"] into tBasename
   
   V1NotesSplitHeader pType, pFileInfo, tHeader, tBody
   
   if tHeader is empty then
      exit V1NotesGenerateFeature
   end if
   
   if tBody is empty then
      builderLog "warning", merge("Empty [[pType]] feature note '[[tBaseName]]'")
      exit V1NotesGenerateFeature
   end if
   
   V1NotesGenerateBlock pVersion, tHeader, tBody
end V1NotesGenerateFeature

private command V1NotesSplitHeader pType, pFileInfo, @rHeader, @rBody
   local tBody, tBasename
   put pFileInfo["markdown"] into tBody
   put pFileInfo["basename"] into tBasename
   
   -- Delete leading empty lines
   repeat while word 1 to -1 of (line 1 of tBody) is empty
      if tBody is empty then
         builderLog "warning", merge("Empty [[pType]] bugfix note '[[tBasename]]'")
         put empty into rHeader
         put empty into rBody
         exit V1NotesSplitHeader
      end if
      
      delete line 1 of tBody
   end repeat
   
   if word 1 of tBody is "#" then
      put word 2 to -1 of (line 1 of tBody) into rHeader
      put word 1 to -1 of (line 2 to -1 of tBody) into rBody
   end if
   
   if rHeader is empty then
      builderLog "warning", merge("Bad [[pType]] bugfix header in [[tBasename]]")
      exit V1NotesSplitHeader
   end if
end V1NotesSplitHeader

private command V1NotesGenerateBlock pVersion, pHeader, pBody
   if pVersion is "HEAD" then
      put sVersion into pVersion
   end if
   
   -- Handle "experimental" annotation
   local tAnnotation, tExperimental
   if the last word of pHeader is "(experimental)" then
      put word 1 to -2 of pHeader into pHeader
      put " - experimental" into tAnnotation
      put true into tExperimental
   else
      put false into tExperimental
   end if
   
   MarkdownAppend "notes", merge("## [[pHeader]] ([[pVersion]][[tAnnotation]])")
   MarkdownAppend "notes", pBody & return
   
   if tExperimental then
      MarkdownAppend "notes", NotesGetExperimentalText()
   end if
   
   -- Include only brand new notes in the updater notes
   if pVersion is sVersion then
      MarkdownAppend "updates", merge("## [[pHeader]]")
      MarkdownAppend "updates", pBody & return
      
      if tExperimental then
         MarkdownAppend "updates", NotesGetExperimentalText()
      end if
   end if
end V1NotesGenerateBlock

----------------------------------------------------------------
-- New-style notes
----------------------------------------------------------------

private command V2NotesCreate pType, pTitle
   builderLog "report", merge("Creating [[pType]] release notes")
   
   MarkdownAppend "notes", merge("# [[pTitle]]")
   MarkdownAppend "updates", merge("# [[pTitle]]")
   
   local tScannedNotes
   put V2NotesScan(pType) into tScannedNotes
   
   local tCollated, tBugInfo
   V2NotesCollate pType, tScannedNotes, tCollated, tBugInfo
   
   V2NotesGenerate pType, tCollated, 1
   
   BugGenerate tBugInfo
end V2NotesCreate

-- There isn't actually any difference between the information
-- that needs to be scanned for V2 notes and the information that
-- needs to be scanned for V1 notes.
private function V2NotesScan pType
   return V1NotesScan(pType)
end V2NotesScan

/*
Create a tree of section information from the results of scanning
the release notes:

{
"__count": <number of child sections>,
"__markdown": "<text>",
"__name": "<title of section>",
1: {
"__count": ...,
"__markdown" ...,
"__name", ...,
...
},
2: {
...
},
}
*/
private command V2NotesCollate pType, pScanInfo, @rCollated, @rBugInfo
   local tTags, tFile, tTagCount, tVersion
   
   -- Collate notes in descending order of version, so newest appear at the top
   put the keys of pScanInfo into tTags
   GitSortTags tTags
   put the number of elements in pScanInfo into tTagCount
   
   repeat tTagCount times
      put line tTagCount of tTags into tVersion
      repeat for each key tFile in pScanInfo[tVersion]
         V2NotesCollateFile pType, tVersion, pScanInfo[tVersion][tFile], rCollated, rBugInfo
      end repeat
      subtract 1 from tTagCount
   end repeat
end V2NotesCollate

private command V2NotesCollateFile pType, pVersion, pFileInfo, @xCollated, @xBugInfo
   local tLine, tSectionPath, tBasename
   
   put pFileInfo["basename"] into tBasename
   
   local tBugId, tBugDesc, tBugPath
   local tLevel, tName, tOldLevel, tHaveContent
   put 0 into tLevel
   put 0 into tOldLevel
   put false into tHaveContent
   
   repeat for each line tLine in pFileInfo["markdown"]
      -- First check if this is a bug info line
      V2NotesExtractBugInfo tLine, tBugId, tBugDesc
      if tBugId is not empty then
         BugAddInfo xBugInfo, pVersion, tBugId, tBugDesc
         next repeat
         
      end if
      
      -- Second, check if it's a section control line
      put tLevel into tOldLevel
      V2NotesExtractSectionInfo tLine, tLevel, tName
      
      if tLevel is not empty then
         if tName is empty then
            builderLog "warning", merge("Invalid section name in [[pType]] note [[tBasename]]")
            next repeat
         end if
         
         -- If changing to a section that's the same level or
         -- higher than the current section but without any
         -- intervening content, generate a warning
         if (not tHaveContent) and tLevel <= tOldLevel then
            builderLog "warning", merge("Section without content in [[pType]] note '[[tBasename]]'")
         end if
         
         V2NotesUpdateSectionPath tLevel, tName, xCollated, tSectionPath
         put false into tHaveContent
         next repeat
      end if
      
      -- Otherwise, just add it to the current section of the
      -- collated notes
      put tLine & return after xCollated[tSectionPath]["__markdown"]
      put pVersion into xCollated[tSectionPath]["__version"]
      
      if word 1 to -1 of tLine is not empty then
         put true into tHaveContent
      end if
   end repeat
end V2NotesCollateFile

private command V2NotesUpdateSectionPath pLevel, pName, @xCollated, @xSectionPath
   if pLevel is 0 then
      exit V2NotesUpdateSectionPath
   end if
   
   if not xSectionPath is an array then
      put 1 into xSectionPath[1]
      delete variable xSectionPath[1]
   end if
   
   -- Truncate the path to the level before the current one
   repeat while the number of elements in xSectionPath >= pLevel
      delete variable xSectionPath[the number of elements in xSectionPath]
   end repeat
   
   -- Find out how many subsections there already are at this level.
   -- It's necessary to do a few contortions to cope with the case that
   -- the truncated path is empty.
   local tPath, tCount
   put xSectionPath into tPath
   put "__count" into tPath[pLevel]
   put xCollated[tPath] into tCount
   
   -- Check to see if there is already a subsection with the specified name
   local tId
   repeat with tId = 1 to tCount
      put tId into tPath[pLevel]
      if xCollated[tPath]["__name"] is pName then
         put tId into xSectionPath[pLevel]
         exit V2NotesUpdateSectionPath
      end if
   end repeat
   
   -- No existing section matches, so add a new one.  Do the "cope with
   -- empty array" dance again.
   put "__count" into tPath[pLevel]
   add 1 to xCollated[tPath]
   put xCollated[tPath] into tId
   
   -- Initialise the new section record
   put tId into xSectionPath[pLevel]
   put pName into xCollated[xSectionPath]["__name"]
   put 0 into xCollated[xSectionPath]["__count"]
end V2NotesUpdateSectionPath

private command V2NotesExtractBugInfo pLine, @rId, @rDesc
   get matchText(pLine, "^\s*#\s+\[(\w*)\]\s+(.*)$", rId, rDesc)
end V2NotesExtractBugInfo

private command V2NotesExtractSectionInfo pLine, @rLevel, @rName
   local tPrefix
   get matchText(pLine, "^\s*(#*)\s+(.*)$", tPrefix, rName)
   
   if tPrefix is not empty then
      put the number of chars in tPrefix into rLevel
   else
      put empty into rLevel
   end if
end V2NotesExtractSectionInfo

-- Append collated section data from <pCollatedSection> as a new top-level
-- section in <xCollated> with <pName>.  If there is already a section
-- named <pName> in <xCollated>, generate an error.
private command V2NotesAppendSection pName, pCollatedSection, @xCollated
   local tId
   repeat with tId = 1 to xCollated["__count"]
      if xCollated[tId]["__name"] is pName then
         throw merge("Notes data already contains a section '[[pName]]'")
      end if
   end repeat
   
   add 1 to xCollated["__count"]
   put xCollated["__count"] into tId
   
   put pCollatedSection into xCollated[tId]
   put pName into xCollated[tId]["__name"]
end V2NotesAppendSection

private command V2NotesGenerate pType, pCollated, pLevel
   MarkdownAppend "notes", V2NotesGenerateContent(pType, pCollated, pLevel)
   MarkdownAppend "updates", V2NotesGenerateContent(pType, pCollated, pLevel, true)
end V2NotesGenerate

-- If <pOnlyCurrent> is true, then only generate content that's brand new in
-- the release that notes are being generated for
private function V2NotesGenerateContent pType, pCollated, pLevel, pOnlyCurrent
   -- Compute the content from this node + all child nodes
   local tContent
   
   if not (pOnlyCurrent is true and (pCollated["__version"] is not sVersion)) then
      if (word 1 to -1 of pCollated["__markdown"]) is not empty then
         put pCollated["__markdown"] into tContent
         if the last char of tContent is not return then
            put return after tContent
         end if
      end if
   end if
   
   -- Recurse into all child nodes
   local tSectionContent, tHeader
   
   repeat with tId = 1 to pCollated["__count"]
      put V2NotesGenerateContent(pType, pCollated[tId], pLevel + 1, pOnlyCurrent) into tSectionContent
      
      -- Only generate a subsection header if the subsection (and all
      -- the subsections it contains) doesn't contain any notes
      if tSectionContent is not empty then
         put "#" into tHeader
         repeat pLevel times
            put "#" after tHeader
         end repeat
         put " " & pCollated[tId]["__name"] & return after tHeader
         
         put return & tHeader after tContent
         put tSectionContent after tContent
      end if
   end repeat
   
   return tContent
end V2NotesGenerateContent

----------------------------------------------------------------
-- Extension release notes
----------------------------------------------------------------

constant kExtensionTypes = "widgets,libraries,modules"
constant kExtensionStrings = "widget,library,module"

private command ExtensionsCreate
   BuilderLog "report", "Creating extension release notes"
   MarkdownAppend "notes", "# LiveCode extension changes"
   MarkdownAppend "updates", "# LiveCode extension changes"
   
   local tType, tTypePath, tFolder
   local tCollated, tBugInfo
   repeat for each item tType in kExtensionTypes
      put FileGetPath("extensions") & slash & tType into tTypePath
      
      repeat for each line tFolder in FileGetSubFolders(tTypePath)
         ExtensionsCreatePath tFolder, tCollated, tBugInfo
      end repeat
      
   end repeat
   
   V2NotesGenerate "extensions", tCollated, 1
   BugGenerate tBugInfo
end ExtensionsCreate

private command ExtensionsCreatePath pExtPath, @xCollated, @xBugInfo
   set the itemdelimiter to slash
   builderLog "report", "Creating release notes for" && item -1 of pExtPath
   
   -- Scan and collate all information for this particular extension
   local tNotesPath
   put pExtPath & "/notes" into tNotesPath
   if there is not a folder tNotesPath then
      exit ExtensionsCreatePath
   end if
   
   local tScan, tExtCollated
   put V2NotesScan(tNotesPath) into tScan
   V2NotesCollate pExtPath, tScan, tExtCollated, xBugInfo
   
   -- Add the collated info in the top-level collation structure
   try
      local tName
      put ExtensionsGetSectionName(pExtPath) into tName
   catch tError
      builderLog "warning", tError
      exit ExtensionsCreatePath
   end try
      
   V2NotesAppendSection tName, tExtCollated, xCollated
end ExtensionsCreatePath

private function ExtensionsGetSectionName pExtPath
   local tName, tType, tTypeOffset, tPrettyType
   
   set the itemdelimiter to slash
   put item -2 of pExtPath into tType
   set the itemdelimiter to comma
   
   put itemoffset(tType, kExtensionTypes) into tTypeOffset
   put item tTypeOffset of kExtensionStrings into tPrettyType
   
   put ExtensionsGetName(pExtPath) into tName
   if word -1 of tName is not tPrettyType then
      put space & tPrettyType after tName
   end if
   
   return tName
end ExtensionsGetSectionName

private function ExtensionsGetName pExtPath
   -- Horrible-ish hack for extracting the "real" name of the LiveCode
   -- module.  See also tools/build-extensions.sh.
   local tShortName
   set the itemdelimiter to slash
   put item -1 of pExtPath into tShortName

   local tLcbSource, tLine, tModuleName
   put FileGetUTF8Contents(merge("[[pExtPath]]/[[tShortName]].lcb")) into tLcbSource
   repeat for each line tLine in tLcbSource
      get matchText(tLine, "(?i)^\s*(?:module|widget|library)\s+([\w.]*)", tModuleName)
      if tModuleName is not empty then
         exit repeat
      end if
   end repeat

   -- Try to figure out the path for the compiled manifest file
   local tManifestFile
   put FileGetPath("built-extensions") into tManifestFile
   put slash & tModuleName after tManifestFile
   put slash & "manifest.xml" after tManifestFile
   
   local tManifest, tXmlId
   put FileGetContents(tManifestFile) into tManifest
   put revXMLCreateTree(tManifest, true, true, false) into tXmlId
   
   if tXmlId begins with "xmlerr" then
      throw "Invalid extension manifest XML in [[tManifestFile]]"
   end if
   
   local tTargetName
   put textDecode(revXMLNodeContents(tXmlId, "/package/title"), "UTF-8") into tTargetName
   if tTargetName begins with "xmlerr" then
      return empty
   end if
   return tTargetName
end ExtensionsGetName

----------------------------------------------------------------
-- Dictionary change generation
----------------------------------------------------------------

-- N.b. we currently only look at lcdoc dictionary entries
constant kDictionaryCommitBase = "f9fb4130b570fd7df3b8e1526bbfe6074e798098"

private command DictionaryCreate
   builderLog "report", "Creating dictionary release notes"
   
   local tAdded, tModified
   DictionaryScan tAdded, tModified
   
   sort lines of tAdded ascending text
   sort lines of tModified ascending text
   
   MarkdownAppend "notes", "# Dictionary additions"
   MarkdownAppend "notes", tAdded
   --MarkdownAppend "notes", "# Dictionary changes"
   --MarkdownAppend "notes", tModified
end DictionaryCreate

private command DictionaryScan @xAdded, @xModified
   -- Get list of changed files
   local tTags, tPermittedFiles, tChangedFiles
   
   put GitGetRelevantTags("dictionary") into tTags
   
   put GitGetChangedFiles("dictionary", kDictionaryCommitBase, \
         the last line of tTags) into tPermittedFiles
   put GitGetChangedFiles("dictionary", the first line of tTags, \
         the last line of tTags) into tChangedFiles
   
   -- Check whether each entry was added or just modified
   local tFile
   repeat for each line tFile in tChangedFiles
      if tFile is among the lines of tPermittedFiles then
         DictionaryScanFile tFile, xAdded, xModified
      end if
   end repeat
end DictionaryScan

private command DictionaryScanFile pFile, @xAdded, @xModified
   local tContents
   put FileGetUTF8Contents(pFile) into tContents
   
   -- Extract basic metadata from the dictionary entry
   local tName, tType, tVersion
   put word 2 to -1 of line lineOffset("Name:", tContents) of tContents into tName
   put word 2 to -1 of line lineOffset("Type:", tContents) of tContents into tType
   put word 2 to -1 of line lineOffset("Introduced:", tContents) of tContents into tVersion
   
   local tEntry
   put merge("**[[tName]]** (*[[tType]]*)") into tEntry
   
   -- Check if the "Introduced" version matches the current version
   local tSourceVer, tTargetVer, tOffset, tMatch, tA, tB
   get matchText(tVersion, "^([0-9.]*)", tSourceVer)
   get matchText(sVersion, "^([0-9.]*)", tTargetVer)
   set the itemdelimiter to "."
   put true into tMatch
   repeat with tOffset = 1 to the number of items in tTargetVer
      put item tOffset of tSourceVer into tA
      put item tOffset of tTargetVer into tB
      if tA is empty then
         if (tB + 0) is not 0 then
            put false into tMatch
            exit repeat
         end if
      end if
      if (tA - tB) is not 0 then
         put false into tMatch
         exit repeat
      end if
   end repeat
   
   if tMatch then
      -- Dictionary entry was introduced in this version
      put merge("* [[tEntry]] has been added to the dictionary.") & return after xAdded
   else
      put merge("* The entry for [[tEntry]] has been updated.") & return after xModified
   end if
end DictionaryScanFile

----------------------------------------------------------------
-- List of release notes from previous releases
----------------------------------------------------------------

private command PreviousCreate
   builderLog "report", "Listing previous release notes"
   MarkdownAppend "notes", "# Previous release notes"
   
   local tTags, tTagCount
   put GitGetTags() into tTags
   put the number of lines in tTags into tTagCount
      
   local tVersion, tUrl
   repeat tTagCount times
      put line tTagCount of tTags into tVersion
      
      -- Match only "general availability" releases
      if not matchText(tVersion, "^[0-9.]*$") then
         
         -- Skip the current version!
      else if tVersion is sVersion then
         next repeat
         
      else      
         put OutputGetNotesUrl(tVersion) into tUrl
         MarkdownAppend "notes", merge("* [LiveCode [[tVersion]] Release Notes]([[tUrl]])")
      end if
      
      subtract 1 from tTagCount
   end repeat
   
end PreviousCreate

----------------------------------------------------------------
-- Bug table helpers
----------------------------------------------------------------

private command BugAddInfo @xBugInfo, pVersion, pID, pDescription
   put pId & comma & pDescription & return after xBugInfo[pVersion]
end BugAddInfo

private function BugUrl pId
   return merge("http://quality.livecode.com/show_bug.cgi?id=[[pId]]")
end BugUrl

private command BugGenerate pBugInfo
   V2BugGenerate pBugInfo
end BugGenerate

private command V1BugGenerate pBugInfo
   
   local tTags, tTagCount
   put the keys of pBugInfo into tTags
   GitSortTags tTags
   
   local tVersion, tBugs, tCurrent, tLine, tId, tDesc, tUrl
   local tPrettyVersion
   
   put the number of lines in tTags into tTagCount
   repeat tTagCount times
      put line tTagCount of tTags into tVersion
      
      -- Bugs fixed in the current build are shown in bold
      put (tVersion is sVersion or tVersion is "HEAD") into tCurrent
      
      put pBugInfo[tVersion] into tBugs
      sort lines of tBugs by item 1 of each
      
      -- Generate heading for table
      put tVersion into tPrettyVersion
      if tPrettyVersion is "HEAD" then 
         put sVersion into tPrettyVersion
      end if
      MarkdownAppend "notes", merge("## Specific bug fixes ([[tPrettyVersion]])")
      
      -- Generate the table itself
      -- Use HTML to allow using HTML classes to distinguish between
      if tCurrent then
         MarkdownAppend "notes", "<table class=" & quote & "currentbugs" & quote & ">"
      else
         MarkdownAppend "notes", "<table class=" & quote & "bugs" & quote & ">"
      end if
      repeat for each line tLine in tBugs
         put item 1 of tLine into tId
         put item 2 to -1 of tLine into tDesc
         put BugUrl(tId) into tUrl
         
         MarkdownAppend "notes", "<tr><td><a href=" & quote & tUrl & quote & ">" & tId & "</a>" & \
               "</td><td>" & tDesc & "</td></tr>"
      end repeat
      
      MarkDownAppend "notes", "</table>"
      
      subtract 1 from tTagCount
   end repeat
end V1BugGenerate

private command V2BugGenerate pBugInfo
   local tTags, tTagCount
   put the keys of pBugInfo into tTags
   GitSortTags tTags
   put the number of lines of tTags into tTagCount
   
   MarkdownAppend "notes", "## Specific bug fixes"
   
   local tVersion, tBugs, tPrettyVersion, tBugList
   local tLine, tId, tDesc, tUrl
   repeat tTagCount times
      put line tTagCount of tTags into tVersion
      
      put pBugInfo[tVersion] into tBugs
      sort lines of tBugs by item 1 of each
      
      put empty into tBugList
      
      -- Generate version info
      if tVersion is "HEAD" then
         put sVersion into tPrettyVersion
      else
         put tVersion into tPrettyVersion
      end if
      put merge("Bugs fixed in [[tPrettyVersion]]:") into tBugList
      
      -- Generate list of bug numbers with links
      repeat for each line tLine in tBugs
         put item 1 of tLine into tId
         put HtmlEscape(item 2 to -1 of tLine) into tDesc
         put BugUrl(tId) into tUrl
         
         put " <a href=" & quote & tUrl & quote && "title=" & quote & tDesc & quote & ">" & \
               tId & "</a>" & comma after tBugList
      end repeat
      put "." into char -1 of tBugList
      
      -- This version's bugs should be bold
      if tPrettyVersion is sVersion then
         put "**" before tBugList
         put "**" after tBugList
      end if
      
      MarkdownAppend "notes", "*" && tBugList
      
      -- Only this version's bugs should appear in the updater
      if tPrettyVersion is sVersion then
         MarkdownAppend "updates", "## Bugs fixed"
         put empty into tBugList
         repeat for each line tLine in tBugs
            put space & item 1 of tLine & comma after tBugList
         end repeat
         put "." into char -1 of tBugList
         MarkdownAppend "updates", tBugList & return
      end if
      
      subtract 1 from tTagCount
   end repeat
   
   MarkdownAppend "notes", empty
end V2BugGenerate

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

----------------------------------------------------------------
-- Git helper functions
----------------------------------------------------------------

private function GitRunInPath pPath, pCommand
   local tOutput, tExitCode
   put shell(merge("cd '[[pPath]]' && git [[pCommand]]")) into tOutput
   put the result into tExitCode
   if tExitCode is not 0 then
      throw merge("Failed to run 'git [[pCommand]]'") & return & tOutput
   end if
   return tOutput
end GitRunInPath

private function GitGetRepoPath pType
   switch pType
      case empty
      case "engine"
      case "lcb"
      case "dictionary"
         return builderRepoFolder()
      case "ide"
         return builderRepoFolder() & "/ide"
      default
         break
   end switch
   
   local tRepoPath
   put GitRunInPath(FileGetPath(pType), "rev-parse --show-toplevel") into tRepoPath
   
   return line 1 of tRepoPath
end GitGetRepoPath

private function GitGetTags pType
   local tTags, tExitCode
   
   put GitRunInPath(FileGetPath(pType), "tag") into tTags
   
   GitSortTags tTags
   
   return tTags
end GitGetTags

private command GitSortTags @xTags
   local tSorted
   local tGM, tRC, tDP
   local tVersion, tPrevVersion
   
   set the itemdelimiter to "-"
   
   repeat for each line tLine in xTags
      if tLine is "HEAD" then
         next repeat
      end if
      
      put item 1 of tLine into tVersion
      if tVersion is not tPrevVersion then
         sort tDP ascending numeric by item 3 of each
         sort tRC ascending numeric by item 3 of each
         put tDP & tRC & tGM after tSorted
         put empty into tRC
         put empty into tDP
         put empty into tGM
      end if
      put tVersion into tPrevVersion
      if item 2 of tLine is empty then
         put tLine & return into tGM
      else if item 2 of tLine is "dp" then
         put tLine & return after tDP
      else if item 2 of tLine is "rc" then
         put tLine & return after tRC
      end if
   end repeat
   sort tDP ascending numeric by item 3 of each
   sort tRC ascending numeric by item 3 of each
   put tDP & tRC & tGM after tSorted
   
   if "HEAD" is among the lines of xTags then
      put "HEAD" & return after tSorted
   end if
   delete the last char of tSorted
   put tSorted into xTags
end GitSortTags

private function GitGetRelevantTags pType
   local tTags, tStart, tEnd
   put GitGetTags(pType) into tTags
   
   set the itemdelimiter to "."
   
   repeat for each line tLine in tTags
      if tLine begins with item 1 to 2 of sVersion then
         exit repeat
      end if
      add 1 to tStart
   end repeat
   put line tStart to -1 of tTags into tTags
   
   repeat for each line tLine in tTags
      if not (tLine begins with item 1 to 2 of sVersion) then
         if not (tEnd is 0) then
            exit repeat
         end if
      end if
      add 1 to tEnd
   end repeat
   
   put line 1 to tEnd of tTags into tTags
   if (sVersion is not line -1 of tTags) then
      put return & "HEAD" after tTags
   end if
   
   return tTags
end GitGetRelevantTags

private function GitGetChangedFiles pType, pFirstVersion, pLastVersion
   local tPath, tFiles
   
   put FileGetPath(pType) into tPath
   put GitRunInPath(tPath, merge("diff --name-only [[pFirstVersion]]...[[pLastVersion]] -- .")) into tFiles
   
   local tFile, tRepoPath, tResult
   put empty into tResult
   put GitGetRepoPath(pType) into tRepoPath
   repeat for each line tFile in tFiles
      put tRepoPath & slash & tFile & return after tResult
   end repeat
   
   return tResult
end GitGetChangedFiles

----------------------------------------------------------------
-- File helpers
----------------------------------------------------------------

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
      case empty
      case "engine"
         return builderRepoFolder() & "/docs/notes"
      case "ide"
         return builderRepoFolder() & "/ide/notes"
      case "lcb"
         return builderRepoFolder() & "/docs/lcb/notes"
      case "output"
         return sOutputPath
      case "extensions"
         return builderRepoFolder() & "/extensions"
      case "built-extensions"
         builderFetchEngine
         return the result & "/packaged_extensions"
      case "dictionary"
         return builderRepoFolder() & "/docs/dictionary"
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

private function FileGetSubFolders pPath
   local tOldCwd, tFolder, tResult
   put the defaultfolder into tOldCwd
   set the defaultfolder to pPath
   if the result is not empty then
      throw merge("Failed to change to [[pPath]]: [[the result]]")
   end if
   
   repeat for each line tFolder in the folders
      if tFolder is ".." then
         next repeat
      end if
      put the defaultfolder & slash & tFolder & return after tResult
   end repeat
   
   set the defaultfolder to tOldCwd
   if the result is not empty then
      throw merge("Failed to change to [[pPath]]: [[the result]]")
   end if
   
   return tResult
end FileGetSubFolders
