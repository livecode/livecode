script "ReleaseNoteV1Lint"
on Lint
   local tLocation, tName
   put LintGetLocation() into tLocation

   set the itemdelimiter to slash
   put item -1 of tLocation[1] into tName

   set the itemdelimiter to "."
   put item 1 of tName into tName

   -- Standard/special filenames
   set the itemdelimiter to ","
   if tName is among the items of \
         "overview,issues,platforms,setup,proposed_changes" then
      exit Lint
   end if

   local tContents
   put LintGetTextContents() into tContents
   replace (numToCodepoint(13) & numToCodepoint(10)) with return in tContents
   replace numToCodepoint(13) with return in tContents

   if tName begins with "bugfix" then
      LintBugfix tName, tContents
   else if tName begins with "feature" then
      LintFeature tName, tContents
   else
      LintProblem "Inappropriate release note filename"
   end if
end Lint

private command LintBugfix pName, pContents
   local tBugId
   get matchText(pName, "(?i)^bugfix-(\d*)($|-)", tBugId)
   if tBugId is empty then
      LintProblem "Invalid bugfix release note filename"
   end if

   local tBody, tLineNum
   LintMetadata pContents, 1, tBody, tLineNum
   LintHeader tBody, tLineNum, tBody, tLineNum
end LintBugfix

private command LintFeature pName, pContents
   local tDesc
   get matchText(pName, "(?i)^feature-(.*)", tDesc)
   if tDesc is empty then
      LintProblem "Invalid feature release note filename"
   end if

   local tBody, tLineNum
   LintMetadata pContents, 1, tBody, tLineNum
   LintHeader tBody, tLineNum, tBody, tLineNum
   if (word 1 to -1 of tBody) is empty then
      LintProblemWithPosition tLineNum, 1, \
            "Feature release note must have a body"
   end if
end LintFeature

private command LintMetadata pContents, pLineOffset, @rRest, @rRestLineOffset
   -- Check if the file contains a metadata header
   if line 1 of pContents is not "---" then
      -- No header
      put pContents into rRest
      put pLineOffset into rRestLineOffset
      exit LintMetadata
   end if

   -- Find the end of the metadata
   local tEndLine
   put lineOffset("---", pContents, 1) into tEndLine
   if tEndLine <= 1 then
      LintProblemWithPosition pLineOffset, 1, "Unterminated metadata header"
      put line 2 to -1 of pContents into rRest
      put (pLineOffset+1) into rRestLineOffset
      exit LintMetadata
   else
      put line (tEndLine + 2) to -1 of pContents into rRest
      put pLineOffset + tEndLine + 1 into rRestLineOffset
   end if

   local tLine, tTrimmedLine, tLineNum, tColNum, tKey, tValue
   put 2 into tLineNum
   set the itemDelimiter to ":"
   repeat for each line tLine in (line 2 to tEndLine of pContents)
      put word 1 to -1 of tLine into tTrimmedLine
      if tTrimmedLine is empty or char 1 of tTrimmedLine is "#" then
         next repeat
      end if

      if item 1 of tTrimmedLine is empty then
         LintProblemWithPosition \
               tLineNum + pLineOffset - 1, offset(tTrimmedLine, tLine), \
               "Empty metadata key"
      end if

      add 1 to tLineNum
   end repeat
end LintMetadata

private command LintHeader pContents, pLineOffset, @rRest, @rRestLineOffset
   local tLineNum, tLine, tTrimmedLine, tHashes
   put 1 into tLineNum
   repeat for each line tLine in pContents
      put word 1 to -1 of tLine into tTrimmedLine
      if tTrimmedLine is not empty then
         get matchText(tTrimmedLine, "^(#*)", tHashes)
         switch (the length of tHashes)
            case 0
               -- No heading
               LintProblemWithPosition \
                     tLineNum + pLineOffset - 1, offset(tTrimmedLine, tLine), \
                     "Release note must have a heading"
               put pContents into rRest
               put pLineOffset into rRestLineOffset
               exit LintHeader
            case 1
               -- No problem
               put line (tLineNum + 1) to -1 of pContents into rRest
               put pLineOffset + tLineNum into rRestLineOffset
               exit LintHeader
            default
               -- Heading is too deep
               LintProblemWithPosition \
                     tLineNum + pLineOffset - 1, offset(tTrimmedLine, tLine), \
                     "Release note must start with level 1 heading"
               put line (tLineNum + 1) to -1 of pContents into rRest
               put pLineOffset + tLineNum into rRestLineOffset
               exit LintHeader
         end switch
      end if
      add 1 to tLineNum
   end repeat

   -- No header at all
   LintProblemWithPosition \
         pLineOffset, 1, \
         "Release note must have a heading"
   put pContents into rRest
   put pLineOffset into rRestLineOffset
end LintHeader
