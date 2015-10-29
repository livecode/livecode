script "ReleaseNotesBuilder"
/*

This stack implements the command "compileReleaseNotes <release number>"
which creates a pdf file of the release notes from various markdown files.

The notes directory should contain 
- "platforms.md" and "setup.md"
- optional files "issues.md" containing any known issues, "proposed_changes.md" and "user_guide_changes.md"
- an optional file "overview.md", if this release has any specific focus or direction
- the feature and bugfix files used to generate the notes.

Each file details a single bug fix or feature implementation, according to the following specifications:

The file should be named "bugfix-<bug number>.md"  or  "feature-<feature number>.md".
It should have a title as the first line, prefixed by a hash symbol.
# <title>

For images, use the syntax
![<alt id>](<image number>.<extension>)
The image must be named bugfix-<bug number>-<image number>.<extension>

Otherwise the file should be LiveCode flavoured Markdown.

Single line files will be added to the table of sepcific bug fixes along with a link to their entry 
on Bugzilla. Multiline files will have their own subsection in the release notes.
   
   */
   
   global gExperimental, gPageBreak, gAnchor
   
   function releaseNotesCSS
      return url ("file:" & builderSystemFolder() & slash & "release_notes_css_prefix.txt")
end releaseNotesCSS

command releaseNotesBuilderInitialize
   start using stack (builderSystemFolder() & slash & "markdown_compiler.livecodescript")
   
   put 1 into gAnchor     
   put "<p style=" & quote & "color:red; font-weight:bold" & quote & ">Important: This feature is currently experimental. " into gExperimental
   put gExperimental & "This means that it may not be complete, or may fail in some circumstances that you would expect it to work. " into gExperimental
   put gExperimental & "Please do not be afraid to try it out as we need feedback to develop it further.</p>" & CR into gExperimental
   put CR & "<div style=" & quote & "page-break-after: always;" & quote & "><span style=" & quote & "display: none;" & quote & ">&nbsp;</span></div>" & CR into gPageBreak
   
   set the defaultfolder to builderRepoFolder()
   set hideconsolewindows to true   
end releaseNotesBuilderInitialize

command releaseNotesBuilderFinalize
   stop using stack (builderSystemFolder() & slash & "markdown_compiler.livecodescript")
end releaseNotesBuilderFinalize

command releaseNotesBuilderRun pEdition, pVersion, pReleaseType
   builderLog "report", "Building release notes for version" && pVersion
   
   releaseNotesBuilderInitialize
   
   local tTags
   put shell("git tag") into tTags
   
   sortTags tTags
   
   local tContents
   local tBody
   
   builderLog "message", "Creating base release notes"
   
   local tOverview
   put notesFileToHTML("overview.md", pVersion, tContents) into tOverview
   if tOverview is empty then
      put tBody & newSection("Overview", 1, tContents) into tBody
      put "This document describes all the changes that have been made for LiveCode " & pVersion & ", including bug fixes and new syntax." into tOverview
   end if
   put tBody & tOverview & CR into tBody
   
   local tIssues
   put notesFileToHTML("issues.md", pVersion, tContents) into tIssues
   if tIssues is empty then
      put tBody & newSection("Known Issues", 1, tContents) into tBody
      put "There are no known issues with this release." into tIssues
   end if
   put tBody & tIssues & CR into tBody
   
   put tBody & notesFileToHTML("platforms.md", pVersion, tContents) into tBody
   
   put tBody & gPageBreak into tBody
   
   put tBody & notesFileToHTML("setup.md", pVersion, tContents) into tBody
   put tBody & notesFileToHTML("proposed_changes.md", pVersion, tContents) into tBody
   
   put tBody & gPageBreak into tBody
   
   put tBody & newSection("Engine changes", 1, tContents) into tBody
   
   builderLog "message", "Adding entries for bugfixes and features"
   local tUpdateFeatures
   local tUpdateBugs
   put tBody & notesFolderToHTML(false, notesPath(), pVersion, tTags, tContents, tUpdateFeatures, tUpdateBugs) into tBody
   
   put tBody & newSection("IDE changes", 1, tContents) into tBody
   
   builderLog "message", "Adding entries for ide bugfixes and features"
   put tBody & notesFolderToHTML(true, notesPath(), pVersion, tTags, tContents, tUpdateFeatures, tUpdateBugs) into tBody
   put tBody & gPageBreak into tBody
   
   put tBody & newSection("LiveCode Builder changes", 1, tContents) into tBody
   
   builderLog "message", "Adding entries for lcb bugfixes and features"
   put tBody & notesFolderLCBToHTML(pVersion, tTags, tContents) into tBody
   put tBody & gPageBreak into tBody
   
   builderLog "message", "Adding entries for dictionary and user guide changes"
   local tChangeList
   local tChangeStatusList
   getDictionaryChanges pVersion, tTags, tChangeStatusList, tChangeList
   
   if tChangeStatusList is not empty then
      put tBody & newSection("Dictionary additions", 1, tContents) into tBody
      put tBody & "<ul>" & CR & tChangeStatusList & "</ul>" into tBody
   end if
   
   local tUserGuideChanges
   put notesFileToHTML("user_guide_changes.md", pVersion, tContents) into tUserGuideChanges
   if tUserGuideChanges is not empty then
      put tBody & tUserGuideChanges into tBody
   end if
   
   if tChangeList is not empty then
      put tBody & newSection("Dictionary changes", 1, tContents) into tBody
      put tBody & "<ul>" & CR & tChangeList & "</ul>" into tBody
   end if
   
   put tBody & gPageBreak into tBody
   
   builderLog "message", "Adding links to previous notes"
   put tBody & newSection("Previous Release Notes", 1, tContents) into tBody
   put tBody & getPastNotes(tTags) into tBody
   
   local tNotesFileURL
   local tNotesFile
   builderEnsureFolder  builderRepoFolder() & slash & targetPath()
   put builderRepoFolder() & slash & targetPath() & slash & "LiveCodeNotes-" & replaceText(pVersion, "[-,\.]", "_") into tNotesFile
   put "file:" & tNotesFile & ".html" into tNotesFileURL
   
   local tNotesPrefix
   local tNotesSuffix
   local tTitle
   put "LiveCode " & pVersion & " Release Notes" into tTitle
   put "<html><br>" & CR & releaseNotesCSS() & CR into tNotesPrefix
   put tNotesPrefix & "<h1 style = " & quote & "text-align : center" & quote & ">" & tTitle & "</h1>" & CR into tNotesPrefix
   put tNotesPrefix & "<h2>Table of contents</h2>" & CR into tNotesPrefix
   put "</html>" into tNotesSuffix   
   
   put tNotesPrefix & tContents & gPageBreak & tBody & tNotesSuffix into url tNotesFileURL
   
   builderLog "message", "Converting release notes to PDF"
   HTMLToPDF tNotesFile, tTitle
   
   builderLog "message", "Copying PDF release notes into IDE"
   put URL ("binfile:" & tNotesFile & ".pdf") into URL ("binfile:" & builderIDEFolder() & "/Release Notes.pdf")
   
   builderLog "message", "Generating updater text"
   local tUpdateText
   filter tUpdateFeatures without ""
   filter tUpdateBugs without ""
   exportUpdateText pVersion, tUpdateFeatures, tUpdateBugs, pReleaseType
   put the result into tUpdateText
   
   releaseNotesBuilderFinalize 
   builderLog "report", "Building release notes for version" && pVersion && "complete"
   
   return tUpdateText
end releaseNotesBuilderRun

command HTMLToPDF pName, pTitle
   local tShell
   local tOptions
   local tCommand

   put " --header-right " & quote & pTitle & " " & the date & quote into tOptions
   put tOptions & " --header-font-size 8 --header-spacing 5 --footer-center [page] --footer-font-size 8 --footer-spacing 5 " into tOptions
   put tOptions & "--margin-top 30 --margin-bottom 20 --margin-left 20 --margin-right 20 --enable-internal-links " into tOptions

   if $WKHTMLTOPDF is empty then
      put builderRepoFolder() & slash & "builder" & slash & "wkhtmltopdf" into tCommand
   else
      put $WKHTMLTOPDF into tCommand
   end if

   put quote & tCommand & quote & tOptions  & pName & ".html " & pName & ".pdf" into tShell

   get shell(tShell)

   if the result is not 0 then
      builderLog "error", "Failed to run wkhtmltopdf"
      throw "wkhtmltopdf failed"
   end if
end HTMLToPDF

function getPastNotes pTags
   local tNumTags
   filter pTags without "*rc*"
   filter pTags without "*dp*"
   put number of lines in pTags into tNumTags
   local tTable
   put "<table>" & CR into tTable
   local i
   put 1 into i
   local tLink
   repeat while i < 21 and i < tNumTags + 1
      local tVersion
      put line -i of pTags into tVersion
      put tTable & "<tr>" & CR & "<td>" & tVersion & " Release Notes</td>" & CR into tTable
      put replaceText(tVersion, "\.", "_") into tVersion
      put "http://downloads.livecode.com/livecode/" & tVersion & "/LiveCodeNotes-" & tVersion & ".pdf" into tLink
      put tTable & "<td><a href=" & quote & tLink & quote & ">" & tLink & "</a></td>" & CR into tTable
      put tTable & "</tr>" & CR into tTable
      add 1 to i
   end repeat
   put tTable & "</table>" into tTable
   return tTable
end getPastNotes

function newSection pSection, pLevel, @xContents
   local tSection 
   put "<h" & pLevel + 1 & " " & getAnchor(pSection) & ">" & pSection & "</h" & pLevel + 1 & ">" & CR into tSection
   appendLink pLevel, pSection, xContents
   return tSection
end newSection

function notesFileToHTML pFileName, pVersion, @xContents
   local tFileUrl
   local tFileText
   local tBody
   put empty into tBody
   put "file:" & notesPath() & slash & pFileName into tFileUrl
   put url tFileUrl into tFileText
   if tFileText is not empty then
      replace "<version>" with pVersion in tFileText
      put markdownToHTML(tFileText, 1, 0, xContents, true) into tBody
   end if
   return tBody
end notesFileToHTML

function makeBugTable pBugs, pBold
   local tBugs
   local tNoID
   local tBugTable
   local tBugUrl
   local tBold
   put "<table>" into tBugTable
   repeat for each line tLine in pBugs
      if item 1 of tLine is not a number then
         put tNoID & CR & "<tr><td></td>" & CR into tNoID
         if pBold then
            put tNoID & "<td><strong>" & item 2 of tLine & "</strong></td></tr>" into tNoID
         else
            put tNoID & "<td>" & item 2 of tLine & "</td></tr>" into tNoID
         end if
      else
         put "http://quality.runrev.com/show_bug.cgi?id=" & item 1 of tLine into tBugUrl
         if pBold then
            put tBugs & CR & "<tr><td><strong>" & item 1 of tLine & "</strong></td>" & CR into tBugs
            put tBugs & "<td><a href=" & quote & tBugUrl & quote & "><strong>" & item 2 of tLine & "</strong></a></td></tr>" into tBugs
         else 
            put tBugs & CR & "<tr><td>" & item 1 of tLine & "</td>" & CR into tBugs
            put tBugs & "<td><a href=" & quote & tBugUrl & quote & ">" & item 2 of tLine & "</a></td></tr>" into tBugs
         end if
      end if
   end repeat
   put tBugTable & tNoID & tBugs & CR & "</table>" into tBugTable
   return tBugTable
end makeBugTable

function getTerms pFileList
   local tTermList, tFile, tTerm
   repeat for each line tLine in pFileList
      put url ("file:" & tLine) into tFile
      get matchText(tFile, "<term>(.*)</term>", tTerm)
      put tTermList & tTerm & CR into tTermList
   end repeat
   filter tTermList without empty
   return tTermList
end getTerms

command getDictionaryChanges pVersion, pTags, @rStatusList, @rOtherList
   put empty into rStatusList
   put empty into rOtherList
   
   local tFrom, tExtension
   if pVersion begins with "8.0" then
      put "f9fb4130b570fd7df3b8e1526bbfe6074e798098" into tFrom #initial commit of dictionary
      put "lcdoc" into tExtension
      set the wholematches to false
   else
      put "xml" into tExtension
      if pVersion begins with "6.1" or pVersion is empty then
         put "81c1b70f3c86c0a85a184c8773001570ba2c7559" into tFrom #initial commit of dictionary
      else
         local tIndex
         put the number of lines in pTags into tIndex
         repeat while tIndex > 1 and line tIndex of pTags begins with char 1 to 3 of pVersion
            subtract 1 from tIndex
         end repeat
         put line tIndex of pTags into tFrom
      end if
   end if
   local tModifiedEntries
   put shell("git diff --name-only " & tFrom & "...HEAD -- " & dictPath()) into tModifiedEntries
   
   repeat for each line tLine in tModifiedEntries
      local tFile
      if there is not a file (item 1 of tLine) then next repeat
      if not (tLine ends with tExtension) then next repeat
      put url ("file:" & item 1 of tLine) into tFile
      
      if tExtension is "xml" then
         local tVersion
         local tTree
         put revCreateXMLTree(tFile, false, true, false) into tTree
         local tName
         local tType
         local tEntry
         put revXMLNodeContents(tTree, "doc/name") into tName
         put revXMLNodeContents(tTree, "doc/type") into tType
         put "<strong>" & tName & "</strong> (<em>" & tType & "</em>)" into tEntry
         put revXMLAttribute(tTree, "doc/history/introduced", "version") into tVersion
      else
         put word 2 to -1 of line lineOffset("Name:", tFile) of tFile into tName
         put word 2 to -1 of line lineOffset("Type:", tFile) of tFile into tType
         put word 2 to -1 of line lineOffset("Introduced:", tFile) of tFile into tVersion
         put "<strong>" & tName & "</strong> (<em>" & tType & "</em>)" into tEntry
      end if
      if (pVersion begins with tVersion) then
         #it was added in this version
         put rStatusList & "<li>" & tEntry into rStatusList
         put rStatusList & " has been added to the dictionary." & CR into rStatusList
      else
         put rOtherList & "<li>The entry for " & tEntry & " has been updated." & CR into rOtherList
      end if     
   end repeat
end getDictionaryChanges

function fetchNameOnlyDiff pFromTag, pToTag, pPath
   return shell("git diff --name-only " & pFromTag & "..." & pToTag & " --  " & pPath)
end fetchNameOnlyDiff

function relevantTags pVersion, pTags
   local tEnd
   local tStart
   repeat for each line tLine in pTags
      if tLine begins with char 1 to 3 of pVersion then
         exit repeat
      end if
      add 1 to tStart
   end repeat
   put line tStart to -1 of pTags into pTags
   
   repeat for each line tLine in pTags
      if not (tLine begins with char 1 to 3 of pVersion) then
         if not (tEnd is 0) then
            exit repeat
         end if
      end if
      add 1 to tEnd
   end repeat
   
   put line 1 to tEnd of pTags into pTags
   if (pVersion is not line -1 of pTags) then
      put pTags & CR & "HEAD" into pTags
   end if
   return pTags
end relevantTags

function notesFolderToHTML pIsIDE, pFolder, pVersion, pTags, @xContents, @rUpdateFeatures, @rUpdateBugs
   local tBody
   local tNumTags
   put relevantTags(pVersion, pTags) into pTags
   put the number of lines in pTags into tNumTags
   
   local tList
   local tFirstVersion
   local tCurVersion
   put line 1 of pTags into tFirstVersion
   put line -1 of pTags into tCurVersion
   if pIsIDE then
      put fetchIDENameOnlyDiff(tFirstVersion, tCurVersion) into tList
   else
      put fetchNameOnlyDiff(tFirstVersion, tCurVersion, pFolder) into tList
   end if
   local tDiff
   local tId
   local tFileText
   local tTitle
   local tNumLines
   local tOneLiners
   local tBugNumber
   local tBugTable
   local tBugTableContents
   local tExperimental
   local tParsedFile
   local tIndex
   local tBoldBugs
   put tNumTags into tIndex
   repeat while tIndex > 1 and tList is not empty
      put line tIndex of pTags into tCurVersion
      if tCurVersion is "HEAD" or tCurVersion is pVersion then
         put true into tBoldBugs
         put pVersion into tCurVersion
      else
         put false into tBoldBugs
      end if
      
      if pIsIDE then
         put fetchIDENameOnlyDiff(line tIndex - 1 of pTags, line tIndex of pTags) into tDiff
      else
         put fetchNameOnlyDiff(line tIndex - 1 of pTags, line tIndex of pTags, pFolder) into tDiff
      end if
      
      local tBugList
      local tFeatureList
      put tDiff into tBugList
      put tDiff into tFeatureList
      filter tBugList with "*bugfix*.md"
      filter tFeatureList with "*feature*.md"
      set the itemdelimiter to "-"
      sort lines in tBugList descending numeric by item 2 of each 
      sort lines in tFeatureList by item 2 of each
      put tFeatureList & CR & tBugList into tDiff
      
      repeat for each line tLine in tDiff
         if tLine is among the lines of tList then
            local tFile
            if pIsIDE then
               put "ide/" & tLine into tFile
            else
               put tLine into tFile
            end if
            put url ("file:" & tFile) into tFileText
            put the number of lines of tFileText into tNumLines
            
            // AL-2014-11-14: Still a one-liner if the bugfix note contains a second empty line
            if tNumLines is 2 and word 1 of line 2 of tFileText is empty then
               delete line 2 of tFileText
            end if
            
            if tLine contains "bugfix" then
               local tOffset
               get matchText(tLine, "-(.*)\.", tId)
               put offset("#", tFileText) into tOffset
               put char tOffset+1 to -1 of line 1 of tFileText into tTitle
               put tOneLiners & tId & "," & tTitle & CR into tOneLiners
               put tId & " - " & tTitle & CR after rUpdateBugs
            end if
            if tNumLines > 1 and not (tNumLines is 2 and the last line of tFileText is not empty) then
               if tLine contains "feature" then put char 2 to -1 of line 1 of tFileText & CR after rUpdateFeatures
               put markdownToHTML(tFileText, 2, 1, xContents, true) into tParsedFile
               if tParsedFile is not empty then
                  set the itemdelimiter to "."
                  replace "<img src=" & quote with "<img src=" & quote & builderRepoFolder() & slash & item 1 to -2 of tLine & "-" in tParsedFile
                  if line 1 of tParsedFile contains "experimental" then
                     replace "experimental" with pVersion & " - experimental" in line 1 of tParsedFile
                     put tParsedFile & gExperimental into tParsedFile
                  else 
                     put " (" & tCurVersion & ")" after char -6 of line 1 of tParsedFile
                  end if
                  put tBody & tParsedFile  into tBody
               end if
            end if
            filter tList without tLine
         end if
      end repeat
      if tOneLiners is not empty then
         appendBugs tCurVersion, tOneLiners, tBoldBugs, tBugTable, tBugTableContents
         put empty into tOneLiners
      end if
      subtract 1 from tIndex
   end repeat
   put xContents & tBugTableContents into xContents
   put tBody & tBugTable into tBody
   return tBody
end notesFolderToHTML

command appendBugs pVersion, pBugs, pBold, @xTable,@xContents
   put xTable & newSection("Specific bug fixes (" & pVersion & ")" , 2, xContents) into xTable
   if pBold then
      put xTable & "<i>(bug fixes specific to the current build are highlighted in bold, reverted bug fixes are stricken through)</i>" & CR into xTable
   end if
   put xTable & makeBugTable(pBugs, pBold) into xTable
end appendBugs

function notesFolderLCBToHTML pVersion, pTags, @xContents
   local tBody
   local tNumTags
   put relevantTags(pVersion, pTags) into pTags
   put the number of lines in pTags into tNumTags
   
   local tList
   local tFirstVersion, tCurVersion
   put line 1 of pTags into tFirstVersion
   put line -1 of pTags into tCurVersion
   put fetchNameOnlyDiff(tFirstVersion, tCurVersion, notesPathLCB()) into tList
   
   local tDataA
   repeat for each line tFile in tList
      addLCBReleaseNoteData tFile, tDataA
   end repeat
   
   local tConglomeratedMarkdown
   put noteDataLCBToMD(tDataA) into tConglomeratedMarkdown
   return markdownToHTML(tConglomeratedMarkdown, 2, 1, xContents, true)
end notesFolderLCBToHTML

function noteSubsectionDataToMD pLevel, pDataA
   local tHashes
   repeat pLevel
      put "#" after tHashes
   end repeat
   
   local tMD
   repeat for each key tSubsection in pDataA
      put tHashes && tSubsection & return after tMD
      if pDataA[tSubsection]["data"] is not empty then
         put pDataA[tSubsection]["data"] & return after tMD
      end if
      if pDataA[tSubsection]["subsection"] is not empty then
         put noteSubsectionDataToMD(pLevel + 1, pDataA[tSubsection]["subsection"]) & return after tMD
      end if
   end repeat
   return tMD
end noteSubsectionDataToMD

function noteDataLCBToMD pDataA
   local tMD
   repeat for each key tSection in pDataA
      put "#" && tSection & return after tMD
      if pDataA[tSection]["subsection"] is not empty then
         put noteSubsectionDataToMD(2, pDataA[tSection]["subsection"]) & return after tMD
      end if
      if pDataA[tSection]["bugs"] is not empty then
         put "##" && "Bugs fixed" & return after tMD
         put pDataA[tSection]["bugs"] & return after tMD
      end if
   end repeat
   return tMD
end noteDataLCBToMD

function numberOfHashes pWord
   local tCount
   put 0 into tCount
   repeat while char tCount + 1 of pWord is "#"
      add 1 to tCount
   end repeat
   return tCount
end numberOfHashes

function lineIsBugInfo pLine
   return matchText(pLine, "^ *# *\[[0-9]*\]")
end lineIsBugInfo

on addDataToSection pLevel, @xData, @xSectionA
   repeat while xData is not empty
      get line 1 of xData
      if word 1 of it is empty then 
         delete line 1 of xData
         next repeat
      end if
      local tCount
      put numberOfHashes(word 1 of it) into tCount
      if tCount is 0 then
         if xSectionA["data"] is empty then
            put it into xSectionA["data"]
         else
            put return & it after xSectionA["data"]
         end if
         delete line 1 of xData
      else if pLevel >= tCount then
         exit addDataToSection
      else
         local tSection
         put word 2 to -1 of it into tSection
         delete line 1 of xData
         addDataToSection pLevel + 1, xData, xSectionA["subsection"][tSection]
      end if
   end repeat
end addDataToSection

on addLCBReleaseNoteData pFile, @xDataA
   local tNoteData
   put url ("file:" & pFile) into tNoteData
   
   local tCount, tSection
   
   repeat while tNoteData is not empty
      get line 1 of tNoteData
      if word 1 of it is empty then 
         delete line 1 of tNoteData
         next repeat
      end if
      put numberOfHashes(word 1 of it) into tCount
      # tCount should always be 1
      if tCount is 1 then
         # If this is not a bug number, then it is a section
         if lineIsBugInfo(it) then
            if xDataA[tSection]["bugs"] is empty then
               put word 2 to -1 of it into xDataA[tSection]["bugs"]
            else
               put return & word 2 to -1 of it after xDataA[tSection]["bugs"]
            end if
            delete line 1 of tNoteData
         else
            put word 2 to -1 of it into tSection
            delete line 1 of tNoteData
            addDataToSection 1, tNoteData, xDataA[tSection]
         end if
      else
         // throw error?
         delete line 1 of tNoteData
      end if
      
   end repeat
end addLCBReleaseNoteData

function dictPath
   # path to the dictionary folder in git
   return "docs" & slash & "dictionary"
end dictPath

function notesPath
   # return the path to the folder of release notes gubbins
   return "docs" & slash & "notes"
end notesPath

function targetPath
   return "_build" & slash & "notes"
end targetPath

function notesPathLCB
   return "docs" & slash & "lcb" & slash & "notes"
end notesPathLCB

function notesPathIDE
   return "ide" & slash & "notes"
end notesPathIDE

command exportUpdateText pVersion, pFeatures, pBugs, pReleaseType
   local tHtmlText
   put "<p>LiveCode " & pVersion & " is now available.<br>" & CR into tHtmlText
   set the casesensitive to false
   if pReleaseType is not "stable" then
      put "<strong>Warning: this is not a stable release. Please ensure you back up your stacks before testing them.</strong><br>" & CR after tHtmlText
      put "If you do not wish to be notified of test releases, uncheck the appropriate boxes in the Updates section of your LiveCode Preferences.<br><br>" & CR after tHtmlText
   end if
   put "This " & pReleaseType & " release contains the following changes:</p>" & CR & "<ul>" & CR after tHtmlText
   repeat for each line tLine in pFeatures
      put "<li>" & tLine & "</li>" & CR after tHtmlText
   end repeat
   local tNumBugs
   put the number of lines in pBugs into tNumBugs
   put "<li>" & tNumBugs & " bug fixes:" & CR & "<ul>" & CR after tHtmlText
   repeat with i = 1 to tNumBugs
      put "<li>" & line i of pBugs & "</li>" & CR after tHtmlText
   end repeat
   put "</ul>" & CR & "</li>" & CR & "</ul>" & CR after tHtmlText
   
   put "<p><strong>AUTO UPDATER</strong></p>" & CR after tHtmlText
   put "<p>The auto updater pops up whenever a new release of LiveCode is available. There are 4 types of release: <br>" & CR after tHtmlText
   put "<strong>Stable:</strong> The release has been through a full testing cycle and is deemed stable.<br>" & CR after tHtmlText
   put "<strong>Maintenance:</strong> The release is the previous stable release with fixes ready for testing.<br>" & CR after tHtmlText
   put "<strong>Beta:</strong> The release contains new features which are complete and ready for testing.<br>" & CR after tHtmlText
   put "<strong>Development:</strong> The release contains untested new features.<br>" & CR after tHtmlText
   
   return tHtmlText
end exportUpdateText

command sortTags @rTags
   local tSortedTags
   set the itemdelimiter to "-"
   local tGM, tRCs, tDPs
   local tVersion, tPrevVersion
   repeat for each line tLine in rTags
      put item 1 of tLine into tVersion
      if tVersion is not tPrevVersion then 
         sort tDPs ascending numeric by item 3 of each
         sort tRCs ascending numeric by item 3 of each
         put tDPs & tRCs & tGM after tSortedTags
         put empty into tRCs
         put empty into tDPs
         put empty into tGM
      end if
      put tVersion into tPrevVersion
      if item 2 of tLine is empty then
         put tLine & CR into tGM
      else if item 2 of tLine is "dp" then
         put tLine & CR after tDPs
      else if item 2 of tLine is "rc" then
         put tLine & CR after tRCs
      end if
   end repeat
   sort tDPs ascending numeric by item 3 of each
   sort tRCs ascending numeric by item 3 of each
   put tDPs & tRCs & tGM after tSortedTags
   delete the last char of tSortedTags
   put tSortedTags into rTags
end sortTags

function fetchIDENameOnlyDiff pFromTag, pToTag
   local tFirstCommit, tLastCommit
   fetchIDECommits pFromTag, pToTag, tFirstCommit, tLastCommit
   
   local tDefaultFolder
   put the defaultFolder into tDefaultFolder
   set the defaultFolder to tDefaultFolder & slash & "ide"
   
   local tDiff
   put shell("git diff --name-only" && tFirstCommit & "..." & tLastCommit && "--" && "notes") into tDiff
   set the defaultFolder to tDefaultFolder
   return tDiff
end fetchIDENameOnlyDiff

function fetchIDEDiff pFromTag, pToTag
   return shell("git diff" && pFromTag & "..." & pToTag & " --  ide")
end fetchIDEDiff

on fetchIDECommits pFirstVersion, pLastVersion, @rFirst, @rLast
   local tDiff
   put fetchIDEDiff(pFirstVersion, pLastVersion) into tDiff
   
   local tString
   put "Subproject commit (.*)" into tString

   get matchText(tDiff, "\-" & tString, rFirst)
   get matchText(tDiff, "\+" & tString, rLast)
end fetchIDECommits
