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

Otherwise the file should be RunRev flavoured Markdown.

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
   put tBody & notesFolderToHTML(pVersion, tTags, tContents, tUpdateFeatures, tUpdateBugs) into tBody
   
   
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
   put " --header-right " & quote & pTitle & " " & the date & quote into tOptions
   put tOptions & " --header-font-size 8 --header-spacing 5 --footer-center [page] --footer-font-size 8 --footer-spacing 5 " into tOptions
   put tOptions & "--margin-top 30 --margin-bottom 20 --margin-left 20 --margin-right 20 --enable-internal-links " into tOptions
   if there is no file (builderRepoFolder() & slash & "builder" & slash & "wkhtmltopdf") then
      builderLog "error", "Can't find wkhtmltopdf"
      throw "Can't find wkhtmltopdf"
   end if
   put toLower(quote & builderRepoFolder()) & slash & "builder" & slash & "wkhtmltopdf" & quote & tOptions  & pName & ".html " & pName & ".pdf" into tShell
   get shell(tShell)
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
   
   local tDictInit
   put "81c1b70f3c86c0a85a184c8773001570ba2c7559" into tDictInit #initial commit of dictionary
   
   local tFrom
   if pVersion begins with "6.1" or pVersion is empty then
      put tDictInit into tFrom
   else
      local tIndex
      put the number of lines in pTags into tIndex
      repeat while tIndex > 1 and line tIndex of pTags begins with char 1 to 3 of pVersion
         subtract 1 from tIndex
      end repeat
      put line tIndex of pTags into tFrom
   end if
   
   local tModifiedEntries
   put shell("git diff --name-only " & tFrom & "...HEAD -- " & dictPath()) into tModifiedEntries
   
   repeat for each line tLine in tModifiedEntries
      local tFile
      if there is not a file (item 1 of tLine) then next repeat
      put url ("file:" & item 1 of tLine) into tFile
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
      if (pVersion begins with tVersion) then
         #it was added in this version
         put rStatusList & "<li>" & tEntry into rStatusList
         put rStatusList & " has been added to the dictionary." & CR into rStatusList
      else
         put rOtherList & "<li>The entry for " & tEntry & " has been updated." & CR into rOtherList
      end if     
   end repeat
end getDictionaryChanges

function notesFolderToHTML pVersion, pTags, @xContents, @rUpdateFeatures, @rUpdateBugs
   local tBody
   local tNumTags
   
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
   put the number of lines in pTags into tNumTags
   
   local tList
   local tFirstVersion
   local tCurVersion
   put line 1 of pTags into tFirstVersion
   put line -1 of pTags into tCurVersion
   put shell("git diff --name-only " & tFirstVersion & "..." & tCurVersion & " --  " & notesPath()) into tList
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
      put shell ("git diff --name-only " &  line tIndex-1 of pTags & "..." & line tIndex of pTags & " -- " & notesPath()) into tDiff
      
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
            put url ("file:" & tLine) into tFileText
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
