script "ReleaseNotesBuilder"
constant kExtensionLcbTypes = "widgets,libraries,modules"
constant kExtensionLcbStrings = "widget,library,module"
constant kExtensionScriptTypes = "script-libraries"

local sMarkdownText
local sVersion
local sOutputPath

private command Initialize pVersion, pOutputPath
   if pOutputPath is empty then
      throw "Release notes output path must be specified"
      exit Initialize
   end if

   start using stack (builderSystemFolder() & slash & "edition_utilities.livecodescript")
   set the defaultfolder to builderRepoFolder()
   set the hideconsolewindows to true
   
   put empty into sMarkdownText
   put pVersion into sVersion
   put pOutputPath into sOutputPath
end Initialize

private command Finalize

end Finalize

command releaseNotesBuilderRun pEdition, pVersion, pReleaseType, pOutputDir
   local tError
   builderLog "report", "Building release notes for version" && pVersion
   
   try
      Initialize pVersion, pOutputDir
      
      BaseCreate
      
      local tComponents
      put builderComponentFolders() into tComponents
      
      -- special case some extra folders
      local tIndex
      put the number of elements of tComponents + 1 into tIndex
      put builderRepoFolder() & "/docs" into tComponents[tIndex]["folder"]
      put "engine" into tComponents[tIndex]["metadata"]["category"]
      put "community" into tComponents[tIndex]["metadata"]["edition"]
      
      add 1 to tIndex
      put builderRepoFolder() & "/ide" into tComponents[tIndex]["folder"]
      put "ide" into tComponents[tIndex]["metadata"]["category"]
      put "community" into tComponents[tIndex]["metadata"]["edition"]
      
      put the number of elements of tComponents + 1 into tIndex
      put builderRepoFolder() & "/docs/lcb" into tComponents[tIndex]["folder"]
      put "builder" into tComponents[tIndex]["metadata"]["category"]
      put "" into tComponents[tIndex]["metadata"]["edition"]
      
      local tPath, tTypePath
      local tReposA, tDefaultEdition
      put builderPrivateRepoFolder() into tReposA["indy"]
      put builderRepoFolder() into tReposA["community"]
      
      repeat for each key tDefaultEdition in tReposA
         put tReposA[tDefaultEdition] into tPath

         repeat for each item tType in kExtensionLcbTypes,kExtensionScriptTypes
            put tPath & slash & "extensions" & slash & tType into tTypePath
            if there is not a folder tTypePath then
               next repeat
            end if
            
            repeat for each line tFolder in FileGetSubFolders(tTypePath)
               put the number of elements of tComponents + 1 into tIndex
               put tFolder into tComponents[tIndex]["folder"]
               put "extension" into tComponents[tIndex]["metadata"]["category"]
               
               local tEdition
               put ExtensionsGetEdition(tFolder) into tEdition
               if tEdition is empty then
                  put tDefaultEdition into tEdition
               end if
               put tEdition into tComponents[tIndex]["metadata"]["edition"]

               put ExtensionsGetSectionName(tFolder) into tComponents[tIndex]["metadata"]["section"]
            end repeat
         end repeat
      end repeat
      
      ComponentsCreate tComponents
      DictionaryCreate tComponents
      PreviousCreate
      
      OutputNotes
      OutputUpdates
      
      Finalize
      
      return builderFileGetUTF8Contents(OutputGetUpdatesFilename("html"))
   catch tError
      put tError
      builderLog "error", tError
   end try
end releaseNotesBuilderRun

private function PathGetLastComponent pPath
   set the itemdelimiter to slash
   return item -1 of pPath
end PathGetLastComponent

private command ComponentsCreateForEdition pEdition, pCollated, pBugInfo
   local tDisplayName
   if pEdition is not empty then
      put editionDisplayName(pEdition) into tDisplayName
   end if
   repeat for each item tCategory in builderComponentCategories()
      local tTitle
      if tDisplayName is not empty then
         put merge("# LiveCode [[ tDisplayName ]] [[ tCategory ]] changes") into tTitle
      else
         put merge("# LiveCode [[ tCategory ]] changes") into tTitle
      end if
      if NotesHaveContent(pCollated[pEdition][tCategory], pBugInfo[pEdition][tCategory]) then
         MarkdownAppend "notes", tTitle
         MarkdownAppend "updates", tTitle
         		
         NotesGenerate tCategory, pCollated[pEdition][tCategory], 1
         BugGenerate pBugInfo[pEdition][tCategory], tCategory
      end if
   end repeat
end ComponentsCreateForEdition

private command ComponentsCreate pComponents
   BuilderLog "report", "Creating component release notes"
   
   local tCollated, tBugInfo
   repeat for each element tComponent in pComponents
      ComponentsCreatePath tComponent, tCollated, tBugInfo
   end repeat
   
   repeat for each item tEdition in editionNames()
      ComponentsCreateForEdition tEdition, tCollated, tBugInfo
   end repeat
   ComponentsCreateForEdition "", tCollated, tBugInfo
end ComponentsCreate

private command ComponentsCreatePath pComponent, @xCollated, @xBugInfo
   local tSkip = true
   try
      put there is not a folder (pComponent["folder"] & "/notes") into tSkip
   end try
   
   set the itemDelimiter to slash
   if tSkip then
      builderLog "report", "Skipping no notes for component" && the last item of pComponent["folder"]
      exit ComponentsCreatePath
   end if
   
   builderLog "report", "Creating release notes for" && the last item of pComponent["folder"]
   
   if pComponent["metadata"]["section"] is empty then
      -- just because something is a LCB module or a script library does not mean
      -- we necessarily want to give it a separate section in the release notes
      -- for example a widget or library for use purely within the IDE should
      -- just merge into the IDE notes. 
      ScanAndCollatePath \
            pComponent["folder"], \
            xCollated[pComponent["metadata"]["edition"]][pComponent["metadata"]["category"]], \
            xBugInfo[pComponent["metadata"]["edition"]][pComponent["metadata"]["category"]]
   else
      local tCollatedSection, tToMerge
      ScanAndCollatePath \
            pComponent["folder"], \
            tCollatedSection, \
            xBugInfo[pComponent["metadata"]["edition"]][pComponent["metadata"]["category"]]
      
      -- Note the use of AppendToSection here means multiple components can be merged
      -- into the same sub-section of the notes. For example, engine -> Mobile or 
      -- ide -> Script Editor
      put 1 into tToMerge["__count"]
      put tCollatedSection into tToMerge[1]
      put pComponent["metadata"]["section"] into tToMerge[1]["__name"]
      
      if NotesHaveContent(tCollatedSection) then
         NotesMerge \
               tToMerge, \
               xCollated[pComponent["metadata"]["edition"]][pComponent["metadata"]["category"]]
      end if
   end if
end ComponentsCreatePath

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

private function OutputGetGuideFilename
   builderEnsureFolder builderBuiltGuidesFolder()
   return builderBuiltGuidesFolder() & slash & "Release Notes.md"
end OutputGetGuideFilename

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

   OutputNotesGuide
end OutputNotes

private command OutputNotesMarkdown
   local tPath
   put OutputGetNotesFilename("md") into tPath
   builderLog "report", merge("Writing [[tPath]]")
   builderFileSetUTF8Contents tPath, sMarkdownText["notes"]
end OutputNotesMarkdown

private command OutputNotesGuide
   local tPath
   put OutputGetGuideFilename() into tPath
   builderLog "report", merge("Writing [[tPath]]")
   builderFileSetUTF8Contents tPath, sMarkdownText["notes"]
end OutputNotesGuide

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
   put builderFileGetUTF8Contents(tHtmlTemplatePath) into tHtml
   replace "@MARKDOWN@" with tEscaped in tHtml
   
   local tOutpath
   put OutputGetNotesFilename("html") into tOutpath
   
   builderLog "report", merge("Writing [[tOutpath]]")
   builderFileSetUTF8Contents tOutpath, tHtml
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
   builderFileSetUTF8Contents tPath, sMarkdownText["updates"] 
end OutputUpdatesMarkdown

// Find <h(n)> elements and make them <h(n+pAmount)>
private command MakeHeadersSmaller @xHTML, pAmount
   local tStart, tEnd, tOldEnd, tNumber
   put 1 into tOldEnd
   repeat while matchChunk(char tOldEnd to -1 of xHTML, \
         "<(?:\/)?h([0-9]+)>", tStart, tEnd)
      add tOldEnd - 1 to tEnd
      add tOldEnd - 1 to tStart
      put char tStart to tEnd of xHTML into tNumber
      add pAmount to tNumber
      put tNumber into char tStart to tEnd of xHTML
      put tEnd into tOldEnd
      add the number of chars in tNumber - (tEnd - tStart) + 1 \
            to tOldEnd
   end repeat
end MakeHeadersSmaller

// Find (at least) double blank lines between specified tags
// and add empty paragraph
private command RespectBlankLinesBetween @xHTML, pFromTagSuffix, pToTagPrefix
   put replaceText(xHtml, pFromTagSuffix & ">\n{2,}<" & pToTagPrefix, \
         pFromTagSuffix & ">" & return & "<p> </p>" & \
         return & "<" & pToTagPrefix) into xHtml
end RespectBlankLinesBetween
   
private command EnsureMergMarkdown
   builderExtUnpack "Community"
   local tMergMarkdown
   put builderUnpackFolder("Community") & slash & "Ext" into tMergMarkdown
   local tFolders
   put folders(tMergMarkdown) into tFolders
   filter tFolders with "mergMarkdown*"
   if tFolders is empty then
      builderLog "error", "couldn't download mergMarkdown"
   end if
   put slash & line 1 of tFolders & slash & "mergMarkdown" after tMergMarkdown
   
   switch the platform
      case "macos"
         put ".bundle" after tMergMarkdown
         break
      case "linux"
         if the processor is "x86_64" then
            put "-x64.so" after tMergMarkdown
         else
            put ".so" after tMergMarkdown
         end if
         break
      case "win32"
         if the processor is "x86_64" then
            put "-x86_64.dll" after tMergMarkdown
         else
            put "-x86.dll" after tMergMarkdown
         end if
         break
   end switch
   set the externals of the templatestack to tMergMarkdown
   lock messages
   create stack
   unlock messages
   start using it
end EnsureMergMarkdown

command OutputUpdatesHtml
   local tHtml
   EnsureMergMarkdown
   put mergMarkdownToXHTML(sMarkdownText["updates"] \
         ,true,true,true,true,false,true,true,true) into tHtml
   
   // Shrink all headers by 2
   MakeHeadersSmaller tHTML, 2
   
   // Add extra blank paragraphs between any tag & 
   // subsequent header
   RespectBlankLinesBetween tHTML, "", "h"
      
   // Add extra blank paragraphs between paragraphs
   RespectBlankLinesBetween tHTML, "p", "p"
   
   local tPath
   put OutputGetUpdatesFilename("html") into tPath
   builderLog "report", merge("Writing [[tPath]]")
   builderFileSetUTF8Contents tPath, tHtml
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
   
   MarkdownAppend "notes", BaseReadFile("breaking_changes")
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
   put FileGetPath("base") & slash & pBasename & ".md" into tPath
   if there is a file tPath then
      return builderFileGetUTF8Contents(tPath)
   else
      return empty
   end if
end BaseReadFile

private command BaseCreateUpdates
   MarkdownAppend "updates", merge("LiveCode [[sVersion]] is now available.") & return
   MarkdownAppend "updates", merge("The full [[sVersion]] release notes are available on the LiveCode website.") & return
   MarkdownAppend "updates", "Changes in this release include:" & return
end BaseCreateUpdates

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
private command NotesScan pType, @xScan
   local tTags, tNumTags
   
   put GitGetRelevantTags(pType) into tTags
   put the number of lines in tTags into tNumTags
   
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
      
      NotesScanVersion pType, tVersion, tVersionFiles, xScan
      
      add 1 to tTagOffset
   end repeat
   
end NotesScan

private command NotesScanVersion pType, pVersion, pFiles, @xScan
   
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
         put builderFileGetUTF8Contents(tFile) into tContents
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
      else
         -- filter out touched files with version headers
         set the itemDelimiter to "-"
         if item 1 of tVersion is not item 1 of pVersion then
            builderLog "report", "Filtering touched file with version header" && tFile
            next repeat
         end if
      end if
      
      put tFileInfo["metadata"]["basename"] into tBasename
      if tBasename is empty then
         put FileGetBasename(tFile) into tBasename
      end if
      
      put tBasename into tFileInfo["basename"]
      put tFileInfo into xScan[tVersion][tFile]
   end repeat
end NotesScanVersion

----------------------------------------------------------------
-- New-style notes
----------------------------------------------------------------

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
private command NotesCollate pType, pScanInfo, @xCollated, @xBugInfo
   local tTags, tFile, tTagCount, tVersion
   
   -- Collate notes in descending order of version, so newest appear at the top
   put the keys of pScanInfo into tTags
   GitSortTags tTags
   put the number of elements in pScanInfo into tTagCount
   
   repeat tTagCount times
      put line tTagCount of tTags into tVersion
      repeat for each key tFile in pScanInfo[tVersion]
         NotesCollateFile pType, tVersion, pScanInfo[tVersion][tFile], xCollated, xBugInfo
      end repeat
      subtract 1 from tTagCount
   end repeat
end NotesCollate

private command NotesCollateFile pType, pVersion, pFileInfo, @xCollated, @xBugInfo
   local tLine, tSectionPath, tBasename
   
   put pFileInfo["basename"] into tBasename
   
   local tBugId, tBugDesc, tBugPath
   local tLevel, tName, tOldLevel, tHaveContent
   put 0 into tLevel
   put 0 into tOldLevel
   put false into tHaveContent
   
   local tLineCount
   put the number of lines of pFileInfo["markdown"] into tLineCount
   repeat for each line tLine in pFileInfo["markdown"]
      -- First check if this is a bug info line
      local tOnlyAddBug
      NotesExtractBugInfo tBasename, tLine, tLineCount, tBugId, tBugDesc
      put it into tOnlyAddBug
      if tBugId is not empty then
         BugAddInfo xBugInfo, pVersion, tBugId, tBugDesc
         if tOnlyAddBug then
            next repeat
         else
            put empty into tBugId
            put empty into tBugDesc
         end if
      end if
      
      -- Second, check if it's a section control line
      put tLevel into tOldLevel
      NotesExtractSectionInfo tLine, tLevel, tName
      
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
         
         NotesUpdateSectionPath tLevel, tName, xCollated, tSectionPath
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
end NotesCollateFile

private command NotesUpdateSectionPath pLevel, pName, @xCollated, @xSectionPath
   if pLevel is 0 then
      exit NotesUpdateSectionPath
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
         exit NotesUpdateSectionPath
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
end NotesUpdateSectionPath

private command NotesExtractBugInfo pBaseName, pLine, pLineCount, @rId, @rDesc
   -- v2 notes are `# [int] description`
   if matchText(pLine, "^\s*#\s+\[(\w*)\]\s+(.*)$", rId, rDesc) then
      return true for value
   end if
   
   -- check if it's a v1 single line bugfix note
   split pBaseName with "-"
   if pBaseName[2] is an integer and \
         word 1 of pLine is "#" then
      put pBaseName[2] into rId
      put word 2 to -1 of pLine into rDesc
   end if
   
   return pLineCount is 1 for value
end NotesExtractBugInfo

private command NotesExtractSectionInfo pLine, @rLevel, @rName
   local tPrefix
   get matchText(pLine, "^\s*(#*)\s+(.*)$", tPrefix, rName)
   
   if tPrefix is not empty then
      put the number of chars in tPrefix into rLevel
   else
      put empty into rLevel
   end if
end NotesExtractSectionInfo

private command NotesMerge pLeftCollated, @xRightCollated
   if pLeftCollated is not an array then
      exit NotesMerge
   end if
   
   if word 1 to -1 of pLeftCollated["__markdown"] is not empty then
      if xRightCollated["__markdown"] is empty then
         put return & return after xRightCollated["__markdown"]
      end if
      put pLeftCollated["__markdown"] after xRightCollated["__markdown"]
   end if
   
   local tLeftID, tRightID
   repeat with tLeftID = 1 to pLeftCollated["__count"]
      local tNameFound
      put false into tNameFound
      repeat with tRightID = 1 to xRightCollated["__count"]
         if pLeftCollated[tLeftID]["__name"] is xRightCollated[tRightID]["__name"] then
            NotesMerge pLeftCollated[tLeftID], xRightCollated[tRightID]
            put true into tNameFound
         end if
      end repeat
      
      if not tNameFound then
         add 1 to xRightCollated["__count"]
         put pLeftCollated[tLeftID] into xRightCollated[xRightCollated["__count"]]
      end if
   end repeat
   
end NotesMerge

private function NotesHaveContent pCollated, pBugInfo
   if pCollated is an array then
      if (word 1 to -1 of pCollated["__markdown"]) is not empty then
         return true
      end if
   
      -- Recurse into child nodes
      local tId
      repeat with tId = 1 to pCollated["__count"]
         if NotesHaveContent(pCollated[tId]) then
            return true
         end if
      end repeat
   end if

   if pBugInfo is an array and the number of elements in pBugInfo > 0 then
      return true
   end if
   
   return false
end NotesHaveContent

private command NotesGenerate pType, pCollated, pLevel
   MarkdownAppend "notes", NotesGenerateContent(pType, pCollated, pLevel)
   MarkdownAppend "updates", NotesGenerateContent(pType, pCollated, pLevel)
end NotesGenerate

-- If <pOnlyCurrent> is true, then only generate content that's brand new in
-- the release that notes are being generated for
private function NotesGenerateContent pType, pCollated, pLevel
   -- Compute the content from this node + all child nodes
   local tContent
   
   if (word 1 to -1 of pCollated["__markdown"]) is not empty then
      put pCollated["__markdown"] into tContent
      if the last char of tContent is not return then
         put return after tContent
      end if
   end if
   
   -- Recurse into all child nodes
   local tSectionContent, tHeader
   
   repeat with tId = 1 to pCollated["__count"]
      put NotesGenerateContent(pType, pCollated[tId], pLevel + 1) into tSectionContent
      
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
end NotesGenerateContent

private function ExtensionsGetSectionName pExtPath
   local tName, tType, tTypeOffset, tPrettyType
   
   set the itemdelimiter to slash
   put item -2 of pExtPath into tType
   set the itemdelimiter to comma
   
   put itemoffset(tType, kExtensionLcbTypes) into tTypeOffset
   put item tTypeOffset of kExtensionLcbStrings into tPrettyType
   
   put ExtensionsGetName(pExtPath) into tName
   if word -1 of tName is not tPrettyType then
      put space & tPrettyType after tName
   end if
   
   return tName
end ExtensionsGetSectionName

private function ExtensionsGetName pExtPath
   if there is a file ExtensionsGetManifestPath(pExtPath) then
      return ExtensionsGetLCBName(pExtPath)
   else if there is a file ExtensionsGetDocPath(pExtPath) then
      -- Script library
      return ExtensionsGetLCSName(pExtPath)
   end if   
   return empty
end ExtensionsGetName

private function ExtensionsGetLCSName pExtPath
   return ExtensionGetSingleLineElement(pExtPath, "Title")
end ExtensionsGetLCSName

private function ExtensionsGetEdition pExtPath
   if there is a file ExtensionsGetDocPath(pExtPath) then
      return ExtensionGetSingleLineElement(pExtPath, "Edition")
   end if
   return empty
end ExtensionsGetEdition

private function ExtensionGetSingleLineElement pExtPath, pElement
   local tDoc
   put builderFileGetContents(ExtensionsGetDocPath(pExtPath)) into tDoc
   
   local tRegex
   put "(?i)^" & pElement &":\s+(.*)" into tRegex
      
   local tValue
   repeat for each line tLine in tDoc
      get matchText(word 1 to -1 of tLine, tRegex, tValue)
      if tValue is not empty then
         return tValue
      end if
   end repeat
   return empty
end ExtensionGetSingleLineElement
   
private function ExtensionsGetLCBName pExtPath
   local tManifest, tXmlId
   put builderFileGetContents(ExtensionsGetManifestPath(pExtPath)) into tManifest
   put revXMLCreateTree(tManifest, true, true, false) into tXmlId
   
   if tXmlId begins with "xmlerr" then
      throw "Invalid extension manifest XML in [[tManifestFile]]"
   end if
   
   local tTargetName
   put textDecode(revXMLNodeContents(tXmlId, "/package/title"), "UTF-8") into tTargetName
   
   revXMLDeleteTree tXmlId
   if tTargetName begins with "xmlerr" then
      return empty
   end if
   return tTargetName
end ExtensionsGetLCBName

private function ExtensionsGetKind pExtPath
   -- Horrible-ish hack for extracting the "real" name of the LiveCode
   -- module.  See also tools/build-extensions.sh.
   local tShortName
   set the itemdelimiter to slash
   put item -1 of pExtPath into tShortName
   
   if there is no file merge("[[pExtPath]]/[[tShortName]].lcb") then
      return ExtensionsGetLCSKind(pExtPath, tShortName)
   else
      return ExtensionsGetLCBKind(pExtPath, tShortName)
   end if
end ExtensionsGetKind

private function ExtensionsGetLCSKind pExtPath, pShortName
   local tSource, tLine, tName
   if there is no file merge("[[pExtPath]]/[[pShortName]].livecodescript") then
      return empty
   end if
   put builderFileGetUTF8Contents(merge("[[pExtPath]]/[[pShortName]].livecodescript")) into tSource
   repeat for each line tLine in tSource
      get matchText(tLine, "(?i)^script" && quote & "(.*)" & quote, tName)

      if tName is not empty then
         exit repeat
      end if
   end repeat
   
   return tName
end ExtensionsGetLCSKind

private function ExtensionsGetLCBKind pExtPath, pShortName
   local tLcbSource, tLine, tModuleName
   put builderFileGetUTF8Contents(merge("[[pExtPath]]/[[pShortName]].lcb")) into tLcbSource
   repeat for each line tLine in tLcbSource
      get matchText(tLine, "(?i)^\s*(?:module|widget|library)\s+([\w.]*)", tModuleName)
      if tModuleName is not empty then
         exit repeat
      end if
   end repeat
   
   return tModuleName
end ExtensionsGetLCBKind

private function ExtensionsGetManifestPath pExtPath
   local tManifestFile
   put FileGetPath("built-extensions") into tManifestFile
   put slash & ExtensionsGetKind(pExtPath) after tManifestFile
   put slash & "manifest.xml" after tManifestFile
   return tManifestFile
end ExtensionsGetManifestPath

private function ExtensionsGetDocPath pExtPath
   local tDocFile
   put FileGetPath("built-extensions") into tDocFile
   put slash & ExtensionsGetKind(pExtPath) after tDocFile
   put slash & "api.lcdoc" after tDocFile
   return tDocFile
end ExtensionsGetDocPath

private command ScanAndCollatePath pExtPath, @xCollated, @xBugInfo
   local tNotesPath
   put merge("[[pExtPath]]/notes") into tNotesPath
   if there is not a folder tNotesPath then
      exit ScanAndCollatePath
   end if

   local tScan
   NotesScan tNotesPath, tScan
   NotesCollate pExtPath, tScan, xCollated, xBugInfo
end ScanAndCollatePath

----------------------------------------------------------------
-- Dictionary change generation
----------------------------------------------------------------

private command DictionaryCreate pComponents
   builderLog "report", "Creating dictionary release notes"
   
   local tAdded, tModified
   DictionaryScan pComponents, tAdded, tModified
   
   sort lines of tAdded ascending text
   sort lines of tModified ascending text
   
   if tAdded is not empty then
      MarkdownAppend "notes", "# Dictionary additions"
      MarkdownAppend "notes", tAdded
   end if
   --MarkdownAppend "notes", "# Dictionary changes"
   --MarkdownAppend "notes", tModified
end DictionaryCreate

private command DictionaryScan pComponents, @xAdded, @xModified
   -- Get list of changed files
   local tTags, tPermittedFiles, tChangedFiles
   
   repeat for each element tComponent in pComponents
      repeat for each item tFolder in "dictionary,glossary"
         local tPath
         put tComponent["folder"] & slash & tFolder into tPath
         if there is not a folder tPath then
            next repeat
         end if
         
         put GitGetRelevantTags(tPath) into tTags
         
         put GitGetChangedFiles(tPath, the first line of tTags, \
               the last line of tTags) into tChangedFiles
         
         -- Check whether each entry was added or just modified
         local tFile
         repeat for each line tFile in tChangedFiles
            if tFile ends with ".lcdoc" then
               DictionaryScanFile tFile, xAdded, xModified
            end if
         end repeat
      end repeat
   end repeat
end DictionaryScan

private command DictionaryScanFile pFile, @xAdded, @xModified
   local tContents, tError
   try
      put builderFileGetUTF8Contents(pFile) into tContents
   catch tError
      BuilderLog "report", tError
      exit DictionaryScanFile
   end try
   
   -- Extract basic metadata from the dictionary entry
   local tName, tType, tVersion
   put word 2 to -1 of line lineOffset("Name:", tContents) of tContents into tName
   put word 2 to -1 of line lineOffset("Type:", tContents) of tContents into tType
   put word 2 to -1 of line lineOffset("Introduced:", tContents) of tContents into tVersion
   
   local tEntry
   put merge("**[[tName]]** (*[[tType]]*)") into tEntry
   
   -- Check if the "Introduced" version matches the current version.
   -- N.b. new dictionary entries are shown for all subsequent point
   -- releases within the same minor release, so dictionary entries
   -- added in 8.0.1 are listed in release notes for all subsequent
   -- 8.0.x releases but not in 8.1.x releases.
   local tSourceVer, tTargetVer, tOffset, tMatch, tA, tB
   get matchText(tVersion, "^([0-9.]*)", tSourceVer)
   get matchText(sVersion, "^([0-9.]*)", tTargetVer)
   set the itemdelimiter to "."
   put true into tMatch
   repeat with tOffset = 1 to min(2, the number of items in tTargetVer)
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
         put OutputGetNotesUrl("pdf", tVersion) into tUrl
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

private command BugGenerate pBugInfo, pBugCategory
   local tTags, tTagCount
   put the keys of pBugInfo into tTags
   GitSortTags tTags
   
   local tVersion, tBugs, tCurrent, tLine, tId, tDesc, tUrl
   local tPrettyVersion, tBugCount
   
   put the number of lines in tTags into tTagCount
   repeat tTagCount times
      put line tTagCount of tTags into tVersion
      
      -- Bugs fixed in the current build are shown in bold
      put (tVersion is sVersion or tVersion is "HEAD") into tCurrent
      
      put pBugInfo[tVersion] into tBugs
      sort lines of tBugs numeric ascending by item 1 of each
      
      -- Generate heading for table
      put tVersion into tPrettyVersion
      if tPrettyVersion is "HEAD" then 
         put sVersion into tPrettyVersion
      end if
      MarkdownAppend "notes", merge("## Specific [[pBugCategory]] bug fixes ([[tPrettyVersion]])")
      
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
         
         add 1 to tBugCount
      end repeat
      
      MarkDownAppend "notes", "</table>"
      
      subtract 1 from tTagCount
   end repeat
   
   if tBugCount is 1 then
      MarkDownAppend "updates", merge("1 [[pBugCategory]] bug fix.")
   else if tBugCount > 1 then
      MarkDownAppend "updates", merge("[[tBugCount]] [[pBugCategory]] bug fixes.")
   end if
end BugGenerate

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
   builderLog "debug", merge("git [[pCommand]]")
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
   local tTags, tStart, tEnd, sNumericVersion
   put GitGetTags(pType) into tTags
   
   -- Ignore a pre-release version suffix (e.g. -rc-1)
   set the itemdelimiter to "-"
   put item 1 of sVersion into sNumericVersion
   
   put 0 into tStart
   repeat for each line tLine in tTags
      if tLine begins with sNumericVersion then
         exit repeat
      end if
      add 1 to tStart
   end repeat
   put line tStart to -1 of tTags into tTags
   
   repeat for each line tLine in tTags
      if not (tLine begins with sNumericVersion) then
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
   
   local tDebug
   put tTags into tDebug
   replace return with space in tDebug
   builderLog "debug", merge("Relevant tags: [[tDebug]]")

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
      case "base"
         return builderRepoFolder() & "/docs/notes-base"
      default
         return pType
   end switch
end FileGetPath

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
