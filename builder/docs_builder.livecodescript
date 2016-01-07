script "DocsBuilder"
# The max number allowed
constant kFilesPerClump = 50

# Probably not a good idea to change this...
constant kClumpNamePrefix = "revDocClump_"

private function docsBuilderGetSourceDirectory
   return builderDocsFolder()
end docsBuilderGetSourceDirectory

private function docsBuilderGetOutputDirectory
   return builderBuiltDocsFolder()
end docsBuilderGetOutputDirectory

command docsBuilderProgressUpdate pProgress, pMessage
   builderLog "message", pMessage
end docsBuilderProgressUpdate

command errorShow pError
   builderLog "error", pError
   throw pError
end errorShow

command docsBuilderRun pEdition, pVersion
   set the itemdelimiter to "."
   if item 1 of pVersion > 7 then   
      start using stack (builderRepoFolder() & slash & "ide-support/revdocsparser.livecodescript")
      docsBuilderGenerateDocsNew
      exit docsBuilderRun
   end if
   
   local tSourceDir, tOutputDir
   put docsBuilderGetSourceDirectory() into tSourceDir
   put docsBuilderGetOutputDirectory() & "/packaged_xml" into tOutputDir
   
   start using stack (builderIdeRepoFolder() & slash & "Toolset/palettes/revdocumentationlibrary.rev")
   
   builderLog "report", "Building docs into" && tOutputDir
   buildDocumentation tSourceDir & "/dictionary", tSourceDir & "/glossary", tOutputDir & "/dictionary", tOutputDir & "/glossary", tOutputDir, "docsBuilderProgressUpdate", the long id of me
   builderLog "report", "Building docs into" && tOutputDir && "complete"   
end docsBuilderRun

command buildDocumentation pDictionaryInputDirectory, pGlossaryInputDirectory, pDictionaryOutputDirectory, pGlossaryOutputDirectory, pManifestOutputDirectory, pCallback, pCallbackTarget
   initializeProgress pCallback, pCallbackTarget
   
   reportProgress "Cleaning Directory", 0, "Cleaning Existing Structure"
   
   # Create the subdirectories we need.
   pathEnsure (pDictionaryOutputDirectory)
   pathEnsure (pGlossaryOutputDirectory)
   
   # Ensure the subDirectories we need were created.
   if there is no folder pDictionaryOutputDirectory then
      errorShow "libDocumentClumpGenerateAll", "Invalid output directory: " & pDictionaryOutputDirectory
      exit buildDocumentation
   end if
   if there is no folder pGlossaryOutputDirectory then
      errorShow "libDocumentClumpGenerateAll", "Invalid output directory: " & pGlossaryOutputDirectory
      exit buildDocumentation
   end if   
   
   # Clean any existing clumps out of memory to prevent interference.
   deleteExistingClumpsFromMemory
   
   local tFileToDirectory, tLinkLookup
   # Get the the file reference information   
   libDocumentationListXMLFiles pGlossaryInputDirectory, tFileToDirectory
   reportProgress "Glossary", 0, "Listed XML files"
   
   libDocumentationListXMLFiles pDictionaryInputDirectory, tFileToDirectory
   reportProgress "Dictionary", 0, "Listed XML files"
      
   #Get the cross reference information for the internal links
   libDocumentationCreateDocRefs tFileToDirectory
   put libCreateLookupTable (tFileToDirectory) into tLinkLookup
   
   # Build XRef symmetry - create symmetry entries in the lookup array
   libDocumentationCreateXRefSymmertry tFileToDirectory, tLinkLookup
   
   # Compile the dictionary and glossary into a collection of stacks with
   # compressed custom properties.
   compileDictionaryIntoStacks pDictionaryInputDirectory, pDictionaryOutputDirectory, tFileToDirectory, tLinkLookup
   compileGlossaryIntoStacks pGlossaryInputDirectory, pGlossaryOutputDirectory, tFileToDirectory, tLinkLookup
   
   # Create a manifest in the packaged_xml directory as the files are dynamic.
   createManifest pManifestOutputDirectory
   
   reportProgress empty, 100, "Done"
end buildDocumentation

local sProgress
local sProgressCallback
local sProgressCallbackTarget
local sProgressStepStatus

// HSC - tested - complies with current dictionary format
private command initializeProgress pCallback, pCallbackTarget
   put empty into sProgress
   put empty into sProgressStepStatus
   
   # The callback is optional.
   if pCallback is not empty and pCallbackTarget is not empty then
      put pCallback into sProgressCallback
      put pCallbackTarget into sProgressCallbackTarget
   end if
end initializeProgress

local sLastTime
private command reportProgress pStep, pPercentage, pMessage
   # This lets us configure the approximate proportions each step is of the whole process,
   # so that the progress is fairly smooth. (These are quite rough...)
   local tProportion
   switch pStep
      case "Dictionary"
         put 20 into tProportion
         break
      case "Glossary"
         put 1 into tProportion
         break
      case "Creating glossary index"
         put 4 into tProportion
         break
      case "Manifest"
         put "4" into tProportion
         break
      case "Cleaning Directory"
         put 11 into tProportion
         break
      case "Creating dictionary index"
         put 33 into tProportion
         break
      case "Verifying dictionary"
         put 37 into tProportion
         break
   end switch
   
   # When a step is complete, add its proportion to the current progress.
   # The current progess is the overall progress plus the step percentage.
   local tStepPercentage
   if pPercentage = 100 then
      if sProgressStepStatus[pStep] is not "Complete" then
         put "Complete" into sProgressStepStatus[pStep]
         add tProportion to sProgress
      end if
      put 0 into tStepPercentage
   else
      put ((pPercentage / 100) * tProportion) into tStepPercentage
   end if
   
   local tCurrentProgress
   put sProgress + tStepPercentage into tCurrentProgress
   
   # Here we can send a message back to the caller to indicate progress
   # of the docs building, but for now we just display progress on the card.
   if sProgressCallback is not empty and sProgressCallbackTarget is not empty then
      send sProgressCallback && "tCurrentProgress, pMessage" to sProgressCallbackTarget
   end if
   
   if the mode of me is not 0 and the visible of me then
      if the millisecs - sLastTime > 500 then
         statusSet pMessage
         set the thumbPosition of scrollbar 1 of me to tCurrentProgress
         put the millisecs into sLastTime
      end if
   end if
end reportProgress

private command statusSet pMessage
   put pMessage into field "Status" of me
end statusSet

// HSC - tested - complies with current dictionary format
private command deleteExistingClumpsFromMemory
   repeat for each line tStack in the mainstacks
      if tStack begins with "revDocClump_" then
         lock messages
         delete stack tStack
         unlock messages
      end if
   end repeat
end deleteExistingClumpsFromMemory

private command compileGlossaryIntoStacks pInputDirectory, pOutputDirectory, @pFileToDirectory, @pLinkLookup
   local tArrIndex, tFilePath, tRelFilePath, tKeyNumber
   
   local tOldPath
   put the directory into tOldPath
   
   local tPath
   put pInputDirectory into tPath
   set the directory to tPath
   
   reportProgress "Glossary", 0, "Listed XML files"
   
   -- Now loop through the list of files and add the file data to the relevant clump
   local tFileNumber, tClumpNumber, tClumpName, tCount = 1
   repeat with tArrIndex = 1 to the number of elements of pFileToDirectory
      -- Work out which clump to add the file to from its name (number)
      if pFileToDirectory[tArrIndex]["type"] is "glossary" then 
         put ((tArrIndex - 1) div kFilesPerClump) + 1 into tClumpNumber
         put kClumpNamePrefix & tClumpNumber into tClumpName
         
         if there is no stack tClumpName then
            libDocumentationClumpCreate tClumpNumber, pOutputDirectory
         end if
         
         -- Get the relative file path of the entry
         put pFileToDirectory[tArrIndex]["path"] into tFilepath
         set the itemDelimiter to slash
         put item -3 to -1 of tFilepath into tRelFilepath
         
         put libDocumentationClumpAddTo(tClumpName, tArrIndex, pFileToDirectory, pOutputDirectory, pLinkLookup) into tKeyNumber
         
         reportProgress "Glossary", ((tArrIndex / the number of elements of pFileToDirectory) * 100), "Added glossary entry: " & item -1 of tFilePath
         //reportProgress "Glossary", ((tArrIndex / the number of lines of pFileToDirectory) * 100), "Added glossary entry: " & item -1 of tFilePath
         
         set the itemDelimiter to comma
         
      end if
   end repeat
   
   -- Clean up
   repeat for each line tStack in the mainStacks
      if char 1 to (the length of kClumpNamePrefix) of tStack is kClumpNamePrefix then
         delete stack tStack
      end if
   end repeat
   
   reportProgress "Glossary", 100, "Glossary Built"
   
   # Build the index for the xml files
   libDocumentationBuildGlossaryIndex pFileToDirectory, pOutputDirectory
end compileGlossaryIntoStacks

// HSC - tested - complies with current dictionary format
private command libDocumentationListXMLFiles pDirectory, @rFileToPath
   local tOldDirectory
   put the directory into tOldDirectory
   set the directory to pDirectory
   
   local tDirectories
   put the folders into tDirectories
   filter tDirectories without ".."
   
   local tFiles, tItems
   put the number of elements of rFileToPath into tItems
   repeat for each line tDirectory in tDirectories
      set the directory to pDirectory & slash & tDirectory
      put the files into tFiles
      filter tFiles with "*.xml"
      repeat for each line tFile in tFiles
         if tFile is not "" then
            add 1 to tItems
            put pDirectory & slash & tDirectory & slash & tFile into rFileToPath[tItems]["path"]
            put tFile into rFileToPath[tItems]["name"]
            if the length of tDirectory is 1 then
               put "glossary" into rFileToPath[tItems]["type"]
            else
               put tDirectory into rFileToPath[tItems]["type"]
            end if
         end if
      end repeat
   end repeat
   
   set the directory to tOldDirectory
   
end libDocumentationListXMLFiles

private command libDocumentationCreateDocRefs @rFileToDirectory
   local tTree, tName, tArrIndex
   
   repeat with tArrIndex = 1 to the number of elements of rFileToDirectory
      
      open file rFileToDirectory[tArrIndex]["path"] for read
      read from file rFileToDirectory[tArrIndex]["path"] until EOF
      // extract the <name> or <term> from the files and update the rFileToDirectory information
      
      put revCreateXMLTree(it, false, true, false) into tTree
      if tTree is not an integer then
         errorShow "Failed to process preferences file with error: " & tTree
         return empty
      end if
      
      put revXMLNodeContents(tTree, "doc/name") into tName
      if matchText(tName,"xmlerr") then
         put revXMLNodeContents(tTree, "doc/term") into tName
      end if
      if matchText(tName,"xmlerr")  then
         //breakpoint
      end if
      
      // Build XRef symmetry - add see also links to the array structure
      local tXrefs, tXref, tXrefToReplace, tTextStart, tTextEnd, tOffsetFound
      put libGetTextLocation (it, "<references>", "</references>", tTextStart, tTextEnd) into tXrefs
      if tTextStart is not  0 and tTextEnd is not 0 then
         replace "<references>" with "" in tXrefs
         put true into tOffsetFound
         set the itemDelimiter to space
         repeat while tOffsetFound
            // need to include the tStartTags in another repeat loop this will be for the new files.
            put libGetTextLocation (tXrefs, "<", ">", tTextStart, tTextEnd) into tXrefToReplace
            if tTextStart is 0 and tTextEnd is 0 then
               put false into tOffsetFound
            else
               put tXrefToReplace into tXref
               replace "<" with "" in tXref
               replace ">" with "" in tXref
               replace "tag=" with "" in tXref
               replace quote with "" in tXref
               replace tXrefToReplace with tXref in tXrefs
               if character 1 of tXref is not "/" then
                  put tXref & return after rFileToDirectory[tArrIndex]["xrefs"]
               end if
            end if
         end repeat
      end if
      
      // need the following line as revXMLNodeContents decides to turn "&amp;" into "&"
      put escapeEntities (tName) into tName
      
      put tName into rFileToDirectory[tArrIndex]["ref"]
      close file rFileToDirectory[tArrIndex]["path"]
   end repeat
end libDocumentationCreateDocRefs

command libDocumentationCreateXRefSymmertry @rFileToDirectory, @rLinkLookup
   local tArrIndex, tReferenceTo
   repeat with tArrIndex = 1 to the number of elements of rFileToDirectory
      if "xrefs" is among the keys of rFileToDirectory[tArrIndex] then
         repeat for each line tReferenceTo in rFileToDirectory[tArrIndex]["xrefs"]
            if rFileToDirectory[tArrIndex]["type"] && rFileToDirectory[tArrIndex]["ref"] is not among the lines of rFileToDirectory[rLinkLookup[tReferenceTo]]["xrefs"] then
               put rFileToDirectory[tArrIndex]["type"] && rFileToDirectory[tArrIndex]["ref"] & return after rFileToDirectory[rLinkLookup[tReferenceTo]]["xrefs"]
            end if
         end repeat
      end if
   end repeat
end libDocumentationCreateXRefSymmertry

private command createManifest pDirectory
   reportProgress "Manifest", 0, "Listing files for manifest"
   
   local tFiles
   put utilityEnumerateDirectory(pDirectory) into tFiles
   
   reportProgress "Manifest", 70, "Finished listing manifest files"
   
   local tManifest
   repeat for each line tFile in tFiles
      if the last char of tFile is slash then
         # Don't include directories
         next repeat
      end if
      if tFile ends with "/Manifest" then
         next repeat
      end if
      put "file," & slash & tFile & return after tManifest
   end repeat
   delete the last char of tManifest
   
   local tResult
   put tManifest into url ("file:" & pDirectory & "/Manifest")
   put the result into tResult
   if tResult is not empty then
      errorShow "Failed to write manifest at location: "  & pDirectory & "/Manifest" & return & "With error: " & tResult
      exit createManifest
   end if
   
   reportProgress "Manifest", 100, "Finished creating manifest"
end createManifest

# Stolen from standalone builder.
private function utilityEnumerateDirectory pDirectory, pPrefix
   local tOldDirectory, tResult
   put the folder into tOldDirectory
   set the folder to pDirectory
   
   # OK-2008-09-10 : Bug 7147 - Prevent the possibility of an infinite loop by
   # returning empty if the directory cannot be set to pDirectory
   if the folder is not pDirectory then
      return empty
   end if
   
   repeat for each line tSubDirectory in the folders
      if tSubDirectory is among the items of ".,.." then
         next repeat
      end if
      # For our purposes we need the directory first, so it can be created ahead of file creation..
      put pPrefix & tSubDirectory & "/" & return after tResult
      get utilityEnumerateDirectory(pDirectory & "/" & tSubDirectory, pPrefix & tSubDirectory & "/")
      if it is not empty then
         put it & return after tResult
      end if
   end repeat
   local tFileSize
   repeat for each line tFile in the files
      put pPrefix & tFile & return after tResult
   end repeat
   set the folder to tOldDirectory
   delete the last char of tResult
   return tResult
end utilityEnumerateDirectory

private function extractFileNumberFromPath pPath
   set the itemDelimiter to slash
   get item -1 of pPath
   set the itemDelimiter to "."
   return item 1 of it
end extractFileNumberFromPath

private command compileDictionaryIntoStacks pInputDirectory, pOutputDirectory, @pFileToDirectory, @pLinkLookup
   local tDocDirectory, tDirectory, tDirectories, tFile, tFiles, tFilepath
   local tCurrentClumpNumber, tIndexData, tKeyNumber, tRelFilepath, tOutputDirectory
   local tXMLFiles, tFileToPath
   
   local tOldPath
   put the directory into tOldPath
   
   local tPath
   put pInputDirectory into tPath
   set the directory to tPath
   
   reportProgress "Dictionary", 0, "Listed XML files"
   
   -- Now loop through the list of files and add the file data to the relevant clump
   local tFileNumber, tClumpNumber, tClumpName, tCount = 1
   repeat with tArrIndex = 1 to the number of elements of pFileToDirectory
      -- Work out which clump to add the file to from its name (number)
      if pFileToDirectory[tArrIndex]["type"] is not "glossary" then 
         put ((tArrIndex - 1) div kFilesPerClump) + 1 into tClumpNumber
         put kClumpNamePrefix & tClumpNumber into tClumpName
         
         if there is no stack tClumpName then
            libDocumentationClumpCreate tClumpNumber, pOutputDirectory
         end if
         
         -- Get the relative file path of the entry
         put pFileToDirectory[tArrIndex]["path"] into tFilepath
         set the itemDelimiter to slash
         put item -3 to -1 of tFilepath into tRelFilepath

         put libDocumentationClumpAddTo(tClumpName, tArrIndex, pFileToDirectory, pOutputDirectory, pLinkLookup) into tKeyNumber
         
         if tArrIndex mod 10 is 0 then
            reportProgress "Dictionary", ((tArrIndex / the number of elements of pFileToDirectory) * 100), "Added dictionary entry: " & item -1 of tFilePath
         end if
         
         set the itemDelimiter to comma
         
      end if
   end repeat
   
   -- Clean up
   repeat for each line tStack in the mainStacks
      if char 1 to (the length of kClumpNamePrefix) of tStack is kClumpNamePrefix then
         delete stack tStack
      end if
   end repeat
   
   reportProgress "Dictionary", 100, "Dictionary built"
   
   # Build the index for the xml files
   libDocumentationBuildDictionaryIndex pFileToDirectory, pOutputDirectory
   
   # Verify that the documentation is complete, we only bother doing this with the dictionary,
   # as broken glossary entries don't matter as much.
   local tResult
   libDocumentationVerifyAll "dictionary/", pOutputDirectory
   put the result into tResult
   if the result is not empty then
      throw "generation_failed - " & "libDocumentationClumpGenerateAll", "The following docs failed to build: " & return & tResult
      exit to top
   end if
end compileDictionaryIntoStacks

private function getIndexFileName pDirectory
   if pDirectory is "dictionary/" then
      return "dict.index"
   else
      return "glos.index"
   end if
end getIndexFileName

private command libDocumentationVerifyAll pDocDirectory, pOutputDirectory
   reportProgress "Verifying dictionary", 0, "Verifying dictionary"
   
   local tIndexPath
   local tIndexFileName
   put getIndexFileName(pDocDirectory) into tIndexFileName
   
   set the itemDelimiter to slash
   put item 1 to -3 of pOutputDirectory & slash & tIndexFileName into tIndexPath
   set the itemDelimiter to comma
   
   local tIndex
   put url ("binfile:" & tIndexPath) into tIndex
   put arrayDecode(tIndex) into tIndex
   
   local tDocsDirectory
   set the itemDelimiter to slash
   put item 1 to -3 of pOutputDirectory into tDocsDirectory
   set the itemDelimiter to comma
   
   local tResult
   local tType, tTag
   set the itemDelimiter to tab
   
   local tTotal
   put the number of lines of the keys of tIndex into tTotal
   
   local tLineNumber
   put 1 into tLineNumber
   repeat for each key tLine in tIndex
      put "dictionary/" into tType
      put tIndex[tLine]["ID"] into tTag
      
      local tData
      try
         put revDocumentationRetrieve(tType, tTag, tDocsDirectory) into tData
         if tData is empty then
            put "Failed to load dictionary entry: " & tTag && tType & return after tResult
         end if
      catch tError
         put "Failed to load dictionary entry: " & tTag && tType & return after tResult
      end try
      
      if tLineNumber mod 10 = 0 then
         reportProgress "Verifying dictionary", ((tLineNumber / tTotal) * 100), "Verified dictionary entry: " & tLine
      end if
      add 1 to tLineNumber
   end repeat
   
   reportProgress "Verifying dictionary", 100, "Dictionary verified"
   
   return tResult
end libDocumentationVerifyAll

private command libDocumentationBuildGlossaryIndex pFileToDirectory, pOutputDirectory
   local tStep
   put "Creating glossary index" into tStep
   
   reportProgress tStep, 0, tStep
   
   local tIndexFileName
   put getIndexFileName("glossary/") into tIndexFileName
   
   local tIndexFile
   set the itemDelimiter to slash
   put item 1 to -3 of pOutputDirectory & slash & tIndexFileName into tIndexFile
   
   local tIndex, tPath, tLine
   
   local tArrIndex
   
   repeat with tArrIndex = 1 to the number of elements of pFileToDirectory
      if pFileToDirectory[tArrIndex]["type"] is "glossary" then
         put pFileToDirectory[tArrIndex]["path"] into tPath
         put indexGlossaryLine(tPath, tArrIndex) into tLine
         put tLine & return after tIndex
      end if
      reportProgress tStep, ((tArrIndex / the number of elements of pFileToDirectory) * 100), "Indexed glossary file: " & item -1 of tPath
   end repeat
   set the itemDelimiter to comma
   delete the last char of tIndex
   
   local tResult
   put  tIndex into url ("file:" & tIndexFile)
   put the result  into tResult
   if tResult is not empty then
      errorShow "Failed to write to file: " & tIndexFile & " with error : " & tResult
      exit libDocumentationBuildGlossaryIndex
   end if
   
   reportProgress tStep, 100, "Index creation finished"
end libDocumentationBuildGlossaryIndex

// HSC - tested - complies with current dictionary format
private command libDocumentationBuildDictionaryIndex pFileToDirectory, pOutputDirectory
   local tStep
   put "Creating dictionary index" into tStep
   
   reportProgress tStep, 0, tStep
   
   local tIndexFileName
   put getIndexFileName("dictionary/") into tIndexFileName
   
   local tIndexFile
   set the itemDelimiter to slash
   put item 1 to -3 of pOutputDirectory & slash & tIndexFileName into tIndexFile
   
   local tIndex, tLine, tArrIndex, tPath
   local tLineNumber
   put 0 into tLineNumber
   
   repeat with tArrIndex = 1 to the number of elements of pFileToDirectory
      if pFileToDirectory[tArrIndex]["type"] is not "glossary" then
         add 1 to tLineNumber
         put pFileToDirectory[tArrIndex]["path"] into tPath
         put indexLine(tPath, tArrIndex) into tLine
         put tLine into tIndex[tLineNumber]
         local tSynonyms
         put expandSynonyms(tPath, tArrIndex) into tSynonyms
         if tSynonyms is an array then
            repeat for each key tKey in tSynonyms
               add 1 to tLineNumber
               put tSynonyms[tKey]  into tIndex[tLineNumber]
            end repeat
         end if
         
      end if
      reportProgress tStep, ((tArrIndex / the number of elements of pFileToDirectory) * 100), "Indexed dictionary file: " & item -1 of tPath
   end repeat
   
   set the itemDelimiter to comma
   local tResult
   put arrayEncode(tIndex) into url ("binfile:" & tIndexFile)
   put the result into tResult
   if tResult is not empty then
      errorShow "Failed to write to file: " & tIndexFile & " with error: " & tResult
      exit libDocumentationBuildDictionaryIndex
   end if
   reportProgress tStep, 100, "Index creation finished"
end libDocumentationBuildDictionaryIndex

# This function does the following:
# 1. Opens the file pDocument and creates an xml tree of the contents.
# 2. Extract the "synonyms" node and processes it to obtain a list of the synonyms.
# 3. For each synonym, creates an index line linking to pDocument
# 4. Returns the list of index lines
private function expandSynonyms pDocument, pFilename
   local tData
   
   put url ("binfile:" & pDocument) into tData
   if tData is empty then
      errorShow "No data for file: " & pDocument
      exit expandSynonyms
   end if
   
   local tTree
   put revCreateXMLTree(tData, false, true, false) into tTree
   
   local tRoot
   
   put revXMLRootNode(tTree) into tRoot
   
   if tTree is not a number then
      errorShow "The document " & pDocument & " cannot be parsed by revXML"
      exit expandSynonyms
   end if
   
   if tRoot is not "doc" then
      errorShow "The document " & pDocument & " is not a valid dictionary entry because its root node is not <doc>"
      exit expandSynonyms
   end if
   
   # Synonyms are stored as:
   # <synonyms>
   #   <synonym>wd</synonym>
   #   <synonym>window</synonym>
   # revXMLChildNames gives us a return-delimited list of each node containing a synonym.
   local tNodes
   put revXMLChildNames(tTree, "/doc/synonyms", return, "synonym", true) into tNodes
   if tNodes is empty or item 1 of tNodes is "xmlerr" then
      # No synonyms found, exit now...
      return empty
   end if
   
   local tIndexLine
   put indexLine(pDocument, pFilename) into tIndexLine
   
   local tName
   --   set the itemDelimiter to tab
   --   put item 2 of tIndexLine into tName
   --   set the itemDelimiter to comma
   put tIndexLine["Keyword"] into tName
   
   local tSynonym, tSynonyms, tContent
   
   local tCount
   put 0 into tCount
   repeat for each line tNode in tNodes
      add 1 to tCount
      put revXMLNodeContents(tTree, "/doc/synonyms/" & tNode) into tContent
      
      # For each one, copy the index line from the main doc, but replace the doc's name with the synonym.
      put tIndexLine into tSynonym
      
      --set the itemDelimiter to tab
      # Replace the main name
      # put tContent into item 2 of tSynonym
      put tContent into tSynonym["Keyword"]
      
      # Replace any references to the name in the syntax (hopefully this won't cause irregular syntax problems...)
      --get item 4 of tSynonym
      get tSynonym["Syntax"]
      replace tName with tContent in it
      --put it into item 4 of tSynonym
      put it into tSynonym["Syntax"]
      
      --set the itemDelimiter to comma
      
      --put tSynonym & return after tSynonyms
      put tSynonym into tSynonyms[tCount]
   end repeat
   --delete the last char of tSynonyms
   
   revDeleteXMLTree tTree
   
   return tSynonyms
end expandSynonyms

-- Given the path to an xml documentation entry (dictionary only),
-- returns the line of text in the dictionary index that corresponds
-- to the document.
// HSC - tested - complies with current dictionary format
private function indexLine pDoc, pIndex
   
   local tFilename
   set the itemDelimiter to slash
   put item -1 of pDoc into tFilename
   
   local tData
   put url ("file:" & pDoc) into tData
   
   local tArray
   put pIndex into tArray["ID"]
   
   local tTree
   put revCreateXMLTree(tData, false, true, false) into tTree
   if tTree is not a number then
      breakpoint 
   end if
   
   local tName, tCategory
   
   put escapeEntities(revXMLNodeContents(tTree, "doc/name")) into tArray["Keyword"]
   put revXMLNodeContents(tTree, "doc/type") into tArray["Type"]
   
   if item 1 of tArray["Keyword"] is "xmlerr" then
      errorShow "Could not extract name from the dictionary file : " & pDoc & " with error: " & tName
      exit indexLine
   end if
   
   if item 1 of tArray["Type"] is "xmlerr" then
      errorShow "Could not extract type from the glossary file : " & pDoc & " with error: " & tCategory
      exit indexLine
   end if
   
   local tSyntax
   set the itemDelimiter to comma
   put extractNodeXML(tTree, "doc/syntax/example") into tSyntax
   if item 1 of tSyntax is "xmlerr" then
      put empty into tArray["Syntax"]
   else
      put auxFormatSyntax(tSyntax) into tArray["Syntax"]
   end if
   
   local tSynonymInfo, tSynonyms
   put revXMLChildContents(tTree, "doc/synonyms", slash, comma, true, 1) into tSynonymInfo
   repeat for each item tChild in tSynonymInfo
      set the itemDelimiter to slash
      put item 2 to -1 of tChild & comma after tSynonyms
      set the itemDelimiter to comma
   end repeat
   if tSynonyms is not empty then
      delete the last char of tSynonyms
   end if
   
   put tSynonyms into tArray["Synonyms"]
   
   local tVersion 
   set the itemDelimiter to comma
   put revXMLAttribute(tTree, "doc/history/introduced", "version") into tVersion
   if item 1 of tVersion is "xmlerr" then
      put empty into tArray["Version"]
   else
      put tVersion into tArray["Version"]
   end if
   
   put revDocumentationVersionDate(tArray["Version"]) into tArray["Date"]
   put empty into tArray["Notes"]
   
   local tLibrary
   put revXMLNodeContents(tTree, "doc/library") into tLibrary
   if item 1 of tLibrary is "xmlerr" then
      put empty into tArray["Library"]
   else
      put tLibrary into tArray["Library"]
   end if
   
   local tObjects
   put revXMLChildNames(tTree,"doc/objects",return,,false) into tObjects
   repeat for each word tObject in "Button Field Graphic Scrollbar Image Player Card Stack Group"
      if lower (tObject) is among the lines of tObjects or tObjects is "any object" then
         put merge("<img src=[[quote]]201233[[quote]] />") into tArray[tObject]
      else
         put empty into tArray[tObject]
      end if
   end repeat
   
   local tClass, tLine
   put revXMLChildNames(tTree,"doc/classes",return,,false) into tClass
   
   put empty into tArray["Platforms"]
   
   if "desktop" is among the lines of tClass then
      put "Desktop, " after tArray["Platforms"] 
   end if
   
   if "server" is among the lines of tClass then
      put "Server, " after tArray["Platforms"] 
   end if
   
   if "web" is among the lines of tClass then
      put "Web, " after tArray["Platforms"] 
   end if
   
   if "mobile" is among the lines of tClass then
      put "Mobile, " after tArray["Platforms"] 
   end if
   
   if tArray["Platforms"] is not empty then
      put item 1 to -2 of tArray["Platforms"] into tArray["Platforms"]
   else
      //put "" into tArray["Platforms"]
      put "Desktop, Server, Web, Mobile" into tArray["Platforms"]
   end if
   
   local tSecurity
   put revXMLChildNames(tTree,"doc/security",return,,false) into tSecurity
   put empty into tArray["Security"]
   local tCapitalEntry
   repeat for each line tLine in tSecurity
      put tLine into tCapitalEntry
      put toUpper (character 1 of tCapitalEntry) into character 1 of tCapitalEntry
      put tArray["Security"] & tCapitalEntry & comma & space into tArray["Security"]
   end repeat   
   if tArray["Security"] is not empty then
      put item 1 to -2 of tArray["Security"] into tArray["Security"]
   else
      put "None required" into tArray["Security"]
   end if
   
   // We are not supporting Products anymore
   put "" into tArray["Products"]
   
   local tPlatforms
   put extractPlatformInformation(tTree) into tPlatforms
   //HSC-ND put tPlatforms into tArray["Platforms"]
   put tPlatforms into tArray["Operating Systems"]
   
   revDeleteXMLTree tTree
   
   return tArray
end indexLine

function indexGlossaryLine pDoc, pIndex
   local tFilename
   set the itemDelimiter to slash
   put item -1 of pDoc into tFilename
   
   local tData
   put url ("file:" & pDoc) into tData
   
   local tTree
   put revCreateXMLTree(tData, false, true, false) into tTree
   if tTree is not a number then
      errorShow "Could not parse the glossary file: " & pDoc & " with error: " & tTree
      exit indexGlossaryLine
   end if
   
   local tName, tCategory
   put revXMLNodeContents(tTree, "doc/term") into tName
   put revXMLNodeContents(tTree, "doc/categories/category") into tCategory
   
   if item 1 of tName is "xmlerr" then
      errorShow "Could not extract name from the glossary file : " & pDoc & " with error: " & tName
      exit indexGlossaryLine
   end if
   
   if item 1 of tCategory is "xmlerr" then
      errorShow "Could not extract category from the glossary file : " & pDoc & " with error: " & tCategory
      exit indexGlossaryLine
   end if
   
   local tLine
   put pIndex & tab & tName & tab & tCategory into tLine
   
   return tLine
end indexGlossaryLine

private function extractPlatformInformation pTree, pMetadata
   local tPlatforms
   # Information about the 4 desktop platforms is already contained in each document,
   # first extract this.
   //if revXMLAttribute(pTree, "doc/platforms", "mac/") then
   //put "MacOS," after tPlatforms
   //end if
   
   local tChildNames
   put revXMLChildNames(pTree,"doc/platforms",return,,false) into tChildNames
   
   if "mac" is among the lines of tChildNames then
      put "Mac OS X," after tPlatforms
   end if
   
   if "windows" is among the lines of tChildNames then
      put "Windows," after tPlatforms
   end if
   
   if "linux" is among the lines of tChildNames then
      put "Linux," after tPlatforms
   end if
   
    if "ios" is among the lines of tChildNames then
      put "iOS," after tPlatforms
   end if
   
    if "android" is among the lines of tChildNames then
      put "Android," after tPlatforms
   end if
   delete the last char of tPlatforms
   --delete the last char of tPlatforms
   return tPlatforms
end extractPlatformInformation

private function extractNodeXML pTree, pName
   local tXML
   put revXMLText(pTree, pName, false) into tXML
   if item 1 of tXML is "xmlerr" then
      return tXML
   end if
   
   # Chop off the "<pName>" at the start as we don't want this and also the "</pName>" at the end
   local tName
   set the itemDelimiter to slash
   put item -1 of pName into tName
   set the itemDelimiter to comma
   
   delete char 1 to (the length of tName + 2) of tXML
   delete char -(the length of tName + 3) to -1 of tXML
   return tXML
end extractNodeXML

private function auxFormatSyntax pSyntax
   local tSyntax, tOffset
   local tReduced = "false"
   
   put pSyntax into tSyntax
   
   if the number of lines of tSyntax > 1 then
      put true into tReduced
      put line 1 of tSyntax into tSyntax
   end if
   
   if the last char of tSyntax is return then delete the last char of tSyntax
   
   put offset("</p>",tSyntax) into tOffset
   if tOffset <> 0 then add 4 to tOffset
   if (tOffset <> 0) and (tOffset < the length of tSyntax) then
      put char 1 to tOffset - 5 of tSyntax & "</p>" into tSyntax
      put true into tReduced
   end if
   
   replace "<p>" with empty in tSyntax
   replace "</p>" with empty in tSyntax
   replace tab with space in tSyntax
   
   if tReduced then put " ..." after tSyntax
   
   -- Remove all style tags. This is a workaround for the tabstops bug
   -- remove this line to allow the original style information for syntax
   -- to reappear in the dictionary.
   put replaceText(tSyntax, "(<[/a-zA-Z0-9]+>)", "") into tSyntax
   
   return tSyntax
end auxFormatSyntax

private function escapeEntities pText
   replace "&" with "&amp;" in pText
   replace quote with "&quot;" in pText
   replace "<" with "&lt;" in pText
   replace ">" with "&gt;" in pText
   return pText
end escapeEntities

// HSC - tested - complies with current dictionary format
private command libDocumentationClumpCreate pNumber, pDir
   local tClumpName
   ---------
   put kClumpNamePrefix & pNumber into tClumpName
   local tResult
   create invisible stack tClumpName
   save it as pDir & slash & tClumpName & ".rev"
   put the result into tResult
   if tResult is not empty then
      errorShow "Failed to save file to disk at location: " & pDir & slash & tClumpName & ".rev" & " with result: " & tResult
      exit libDocumentationClumpCreate
   end if
   return tClumpName
end libDocumentationClumpCreate

// HSC - tested - complies with current dictionary format
private function libDocumentationClumpAddTo pClumpName, pFileIndex, pFileToDirectory, @pOutputDirectory, @pLinkLookup
   -- adds mapping pFilepath -> pClumpName.N to clump
   local tKeyNumber, tPropName, tDir, tData
   local tFilename, tRemainder   
   
   put pFileIndex mod kFilesPerClump into tRemainder
   if tRemainder is 0 then
      put kFilesPerClump into tRemainder
   end if
   
   put tRemainder into tKeyNumber
   
   put "c" & tKeyNumber into tPropName
   
   put url ("binfile:" & pFileToDirectory[pFileIndex]["path"]) into tData
   
   if tData is empty then
      errorShow "No data for file: " & pFileToDirectory[pFileIndex]["path"]
      exit libDocumentationClumpAddTo
   end if
   
   // convert the new XML format back to the old type
   libDocumentationOldFormat tData, pFileToDirectory, pLinkLookup
   
   put the compress of tData into tData
   set the tPropName of stack pClumpName to tData
   
   # OK-2007-09-03 : Work around an engine bug to ensure that we don't have any problems
   # saving to mounted network drives on OS X
   # save stack pClumpName
   local tResult
   if there is a file (the filename of stack pClumpName) then
      delete file (the filename of stack pClumpName)
      put the result into tResult
      if tResult is not empty then
         errorShow "Failed to delete old stack file: " & tResult
         exit libDocumentationClumpAddTo
      end if
   end if
   
   save stack pClumpName
   put the result into tResult
   if tResult is not empty then
      errorShow "Failed to save clump: " & pClumpName & " (" & tResult & ")"
      exit libDocumentationClumpAddTo
   end if
   
   set the directory to tDir
   return tKeyNumber
end libDocumentationClumpAddTo

private function libGetRelatedEntries tData
   local tResult, tTextStart, tTextEnd, tTextToProcess, tTextLine
   put libGetTextLocation (tData, "<related>", "</related>", tTextStart, tTextEnd) into tTextToProcess
   repeat for each line tTextLine in tTextToProcess
      set the itemdelimiter to slash
      put item 3 of tTextLine into tTextLine
      if tTextLine is empty then
         next repeat
      end if
      set the itemdelimiter to ">"
      put item 0 to 1 of tTextLine into tTextLine
      put char 0 to -6 of tTextLine into tTextLine
      if tTextLine is a number then
         put tTextLine & comma after tResult
      end if
   end repeat
   if tResult is not empty then
      delete the last char of tResult
   end if
   return tResult
end libGetRelatedEntries
   
private function libGetSynonyms pTree
   local tResult
   put revXMLChildContents(pTree,"doc/synonyms",comma, return,"false", -1) into tResult
   if char 1 to 6 of tResult is "xmlerr" then
      put empty into tResult
   end if
   return tResult
end libGetSynonyms
   
private function libGetTextLocation, pData, pStartToken, pEndToken, @rStart, @rEnd
   local tTextFound
   put empty into tTextFound
   put offset (pStartToken, pData) into rStart
   put offset (pEndToken, pData) into rEnd
   if rStart > rEnd then
      breakpoint
   end if
   if rStart is not 0 and rEnd is not 0 then
      put rEnd + the length of pEndToken - 1 into rEnd
      put char rStart to rEnd of pData into tTextFound
   end if
   return tTextFound
end libGetTextLocation

function libGetTextText pText
   local tTextText, tTextStart, tTextEnd
   put offset (">", pText) into tTextStart
   if tTextStart is 0 or tTextStart is the length of pText then
      breakpoint
   end if
   put offset("</", pText, tTextStart) into tTextEnd
   if tTextEnd is 0 then
      breakpoint
   end if
   put char tTextStart + 1 to tTextStart + tTextEnd - 1 of pText into tTextText
   return tTextText
end libGetTextText

private function libCreateLookupTable pFileToDirectory
   local tLookup, tArrayIndex
   repeat with tArrayIndex = 1 to the number of elements of pFileToDirectory
      put tArrayIndex into tLookup [pFileToDirectory[tArrayIndex]["type"] && pFileToDirectory[tArrayIndex]["ref"]]
   end repeat
   return tLookup
end libCreateLookupTable

function libGetLinkText pText, pType, @pFileToDirectory, @pLinkLookup
   local tLinkText, tArrayIndex, tNotFound
   put true into tNotFound
   if offset ("tag=", pText) > 0 then
      // we are using the extened notation
      set the itemdelimiter to quote
      put item 2 of pText into tLinkText
   else
      // the link is implicit in the name
      put libGetTextText (pText) into tLinkText
   end if
   // test that we can access the element we are looking for
   if pLinkLookup[pType && tLinkText] is not a number then
      breakpoint
   end if
   // look up and substitute natural name with file name
   set the itemdelimiter to slash
   // repeat with tArrayIndex = 1 to the number of elements of pFileToDirectory
   // if pFileToDirectory[tArrayIndex]["type"] is pType and pFileToDirectory[tArrayIndex]["ref"] is tLinkText then
   // put item -3 to -1 of pFileToDirectroy[tArrayIndex]["path"] & slash & tArrayIndex & ".xml" into tLinkText
   // put item -3 to -2 of pFileToDirectory[tArrayIndex]["path"] & slash & tArrayIndex & ".xml" into tLinkText
   put item -3 to -2 of pFileToDirectory[pLinkLookup[pType && tLinkText]]["path"] & slash & pLinkLookup[pType && tLinkText] & ".xml" into tLinkText
   //put false into tNotFound
   // exit repeat
   // end if
   //end repeat
   //if tNotFound is true then
   // breakpoint
   // end if
   return tLinkText
end libGetLinkText

private command libReplaceLinks @rData, @rFileToDirectory, @rLinkLookup
   local tOffsetFound, tOffsetSkip, tStartTags, tStartTagsLink, tEndTags, tTag, tTextStart, tTextEnd, tTextToReplace, tReplaceWith, tLinkText, tTextText
   put "<glossary tag=,<command tag=,<constant tag=,<control_st tag=,<function tag=,<keyword tag=,<message tag=,<object tag=,<operator tag=,<property tag=" into tStartTagsLink
   put "<glossary>,<command>,<constant>,<control_st>,<function>,<keyword>,<message>,<object>,<operator>,<property>" into tStartTags
   put "</glossary>,</command>,</constant>,</control_st>,</function>,</keyword>,</message>,</object>,</operator>,</property>" into tEndTags
   put true into tOffsetFound
   put 0 into tOffsetSkip
   set the itemdelimiter to comma
   set the caseSensitive to true   
   repeat with tTag = 1 to the number of items in tStartTags
      put true into tOffsetFound
      repeat while tOffsetFound
         // need to include the tStartTags in another repeat loop this will be for the new files.
         put libGetTextLocation (rData, item tTag of tStartTagsLink, item tTag of tEndTags, tTextStart, tTextEnd) into tTextToReplace
         if tTextStart is 0 and tTextEnd is 0 then
            put false into tOffsetFound
         else
            // generate the text that is to be inserted
            put "<docRef src=" & quote & "--link--" & quote & ">--text--</docRef>" into tReplaceWith
            put libGetLinkText (tTextToReplace, char 2 to -2 of item tTag of tStartTags, rFileToDirectory, rLinkLookup) into tLinkText
            put libGetTextText (tTextToReplace) into tTextText
            // replace the text here
            replace "--link--" with tLinkText in tReplaceWith
            replace "--text--" with tTextText in tReplaceWith
            replace tTextToReplace with tReplaceWith in rData
         end if
      end repeat
   end repeat
end libReplaceLinks

private command libUpdateSyntaxExamples @rData
   // updates the <syntax><example></example></syntax> notation
   local tTextToReplace, tReplaceWith, tTextStart, tTextEnd
   put libGetTextLocation (rData, "<syntax>", "</syntax>", tTextStart, tTextEnd) into tTextToReplace
   if tTextToReplace is empty then
      breakpoint
   else
      put tTextToReplace into tReplaceWith
      replace "<example>" with "<p>" in tReplaceWith
      replace "</example>" with "</p>" in tReplaceWith
      replace "<p><p>" with "<p>" in tReplaceWith
      replace "</p></p>" with "</p>" in tReplaceWith
      replace tTextToReplace with tReplaceWith in rData
   end if
end libUpdateSyntaxExamples

private command libUpdateObjects @rData
   // updates the <objects>...</objects> notation
   local tTextToReplace, tReplaceWith, tTextStart, tTextEnd
   put libGetTextLocation (rData, "<objects>", "</objects>", tTextStart, tTextEnd) into tTextToReplace
   if tTextToReplace is empty then
      breakpoint
   else
      put "<objects>--text--</objects>" into tReplaceWith
      if offset ("<card/>", tTextToReplace) > 0 then
         replace "--text--" with "card,--text--" in tReplaceWith
      end if
      if offset ("<stack/>", tTextToReplace) > 0 then
         replace "--text--" with "stack,--text--" in tReplaceWith
      end if
      if offset ("<group/>", tTextToReplace) > 0 then
         replace "--text--" with "group,--text--" in tReplaceWith
      end if
      if offset ("<field/>", tTextToReplace) > 0 then
         replace "--text--" with "field,--text--" in tReplaceWith
      end if
      if offset ("<button/>", tTextToReplace) > 0 then
         replace "--text--" with "button,--text--" in tReplaceWith
      end if
      if offset ("<graphic/>", tTextToReplace) > 0 then
         replace "--text--" with "graphic,--text--" in tReplaceWith
      end if
      if offset ("<scrollbar/>", tTextToReplace) > 0 then
         replace "--text--" with "scrollbar,--text--" in tReplaceWith
      end if
      if offset ("<player/>", tTextToReplace) > 0 then
         replace "--text--" with "player,--text--" in tReplaceWith
      end if
      if offset ("<image/>", tTextToReplace) > 0 then
         replace "--text--" with "image,--text--" in tReplaceWith
      end if
      if offset ("<AndroidScroller/>", tTextToReplace) > 0 then
         replace "--text--" with "AndroidScroller,--text--" in tReplaceWith
      end if
      if offset ("<AndroidBrowser/>", tTextToReplace) > 0 then
         replace "--text--" with "AndroidBrowser,--text--" in tReplaceWith
      end if
      if offset ("<AndroidPlayer/>", tTextToReplace) > 0 then
         replace "--text--" with "AndroidPlayer,--text--" in tReplaceWith
      end if
      if offset ("<iosBrowser/>", tTextToReplace) > 0 then
         replace "--text--" with "iosBrowser,--text--" in tReplaceWith
      end if
      if offset ("<iosScroller/>", tTextToReplace) > 0 then
         replace "--text--" with "iosScroller,--text--" in tReplaceWith
      end if
      if offset ("<iosPlayer/>", tTextToReplace) > 0 then
         replace "--text--" with "iosPlayer,--text--" in tReplaceWith
      end if
      if offset ("<iosTextInput/>", tTextToReplace) > 0 then
         replace "--text--" with "iosTextInput,--text--" in tReplaceWith
      end if      
      replace ",--text--" with "" in tReplaceWith
      replace "--text--" with "" in tReplaceWith
      replace tTextToReplace with tReplaceWith in rData
   end if
end libUpdateObjects

private command libUpdateReferences @rData, @rFileToDirectory, @rLinkLookup
   // updates the <references></references> notation
   //replace "<references>" with "<related>" in rData
   //replace "</references>" with "</related>" in rData
   
   # Build XRef symmetry - create symmetry entries in the lookup array
   local tTextToReplace, tReplaceWith, tTextStart, tTextEnd, tReferenceTo, tName, tType, tNewReference, tCapitalEntry
   put libGetTextLocation (rData, "<references>", "</references>", tTextStart, tTextEnd) into tTextToReplace
   if tTextStart > 0 and tTextEnd > 0 then
      put tTextToReplace into tReplaceWith
      replace "<references>" with "<related>" in tReplaceWith
      replace "</references>" with "</related>" in tReplaceWith
      put libGetTextLocation (rData, "<type>", "</type>", tTextStart, tTextEnd) into tType
      if tTextStart > 0 and tTextEnd > 0 then
         put libGetTextLocation (rData, "<name>", "</name>", tTextStart, tTextEnd) into tName
         if tTextStart > 0 and tTextEnd > 0 then
            replace "<type>" with "" in tType
            replace "</type>" with "" in tType
            replace "<name>" with "" in tName
            replace "</name>" with "" in tName
            //set the itemDelimiter to space
            repeat for each line tReferenceTo in rFileToDirectory[rLinkLookup[tType && tName]]["xrefs"]
               put "<" & word 1 of tReferenceTo && "tag=" & quote & word 2 to -1 of tReferenceTo & quote & ">" into tNewReference
               if offset (tNewReference, tReplaceWith) is 0 then
                  put word 1 of tReferenceTo into tCapitalEntry
                  put toUpper (character 1 of tCapitalEntry) into character 1 of tCapitalEntry 
                  put word 2 to -1 tReferenceTo && tCapitalEntry & "</" & word 1 tReferenceTo & ">" after tNewReference
                  replace "</related>" with "  " & tNewReference & return & "</related>" in tReplaceWith
               end if
            end repeat
         end if
      end if
      replace tTextToReplace with tReplaceWith in rData
   end if
end libUpdateReferences

private command libUpdateClassification @rData
   // updates the <classification></classification> notation
   replace "<classification>" with "<categories>" in rData
   replace "</classification>" with "</categories>" in rData
end libUpdateClassification

private command libUpdateSummary @rData
   replace "<summary>" with "<summary><p>" in rData
   replace "</summary>" with "</p></summary>" in rData
end libUpdateSummary

private command libUpdateHistory @rData
   // updates the <histroy></history> notation
   replace "<history>" with "" in rData
   replace "</history>" with "" in rData
end libUpdateHistory

private command libUpdatePlatforms @rData
   // updates the <platforms>...</platforms> notation
   replace "<platforms>" with "" in rData
   replace "</platforms>" with "" in rData
   replace "<ios/>" with "<ios support=" & quote & "true" & quote && "/>" in rData
   replace "<android/>" with "<android support=" & quote & "true" & quote && "/>" in rData
   replace "<mac/>" with "<osx support=" & quote & "true" & quote && "/>" in rData
   replace "<windows/>" with "<win32 support=" & quote & "true" & quote && "/>" in rData
   replace "<linux/>" with "<unix support=" & quote & "true" & quote && "/>" in rData
end libUpdatePlatforms

private command libUpdateClasses @rData
   // updates the <classes>...</classes> notation
   local tTextToReplace, tReplaceWith, tTextStart, tTextEnd
   put libGetTextLocation (rData, "<classes>", "</classes>", tTextStart, tTextEnd) into tTextToReplace
   if tTextToReplace is empty then
      breakpoint
   else
      put "<class>--and----text--</class>" into tReplaceWith
      if offset ("<desktop/>", tTextToReplace) > 0 then
         replace "--and--" with", " in tReplaceWith
         replace "--text--" with "--and--Desktop--text--" in tReplaceWith
      end if
      if offset ("<server/>", tTextToReplace) > 0 then
         replace "--and--" with", " in tReplaceWith
         replace "--text--" with "--and--Server--text--" in tReplaceWith
      end if
      if offset ("<web/>", tTextToReplace) > 0 then
         replace "--and--" with", " in tReplaceWith
         replace "--text--" with "--and--Web--text--" in tReplaceWith
      end if
            if offset ("<mobile/>", tTextToReplace) > 0 then
         replace "--and--" with", " in tReplaceWith
         replace "--text--" with "--and--Mobile--text--" in tReplaceWith
      end if
      replace ", , " with "" in tReplaceWith
      replace "--and----text--" with "" in tReplaceWith
      replace "--text--" with "" in tReplaceWith
      replace ", --and--" with "" in tReplaceWith
      replace "--and--" with " and " in tReplaceWith
      replace tTextToReplace with tReplaceWith in rData
   end if
end libUpdateClasses

private command libUpdateSecurity @rData
   // updates the <security>...</security> notation
   local tTextToReplace, tReplaceWith, tTextStart, tTextEnd, tTempReplaceWith
   put libGetTextLocation (rData, "<security>", "</security>", tTextStart, tTextEnd) into tTextToReplace
   if tTextToReplace is empty then
      breakpoint
   else
      put "<security>--and----text--</security>" into tReplaceWith
      if offset ("<disk access=" & quote & "???" & quote & "/>", tTextToReplace)  > 0 then
         replace "--and--" with", " in tReplaceWith
         replace "--text--" with "--and--Disk--text--" in tReplaceWith
      end if
      if offset ("<disk/>", tTextToReplace) > 0 then
         replace "--and--" with", " in tReplaceWith
         replace "--text--" with "--and--Disk--text--" in tReplaceWith
      end if
      if offset ("<network/>", tTextToReplace) > 0 then
         replace "--and--" with", " in tReplaceWith
         replace "--text--" with "--and--Network--text--" in tReplaceWith
      end if
      if offset ("<process/>", tTextToReplace) > 0 then 
         replace "--and--" with", " in tReplaceWith
         replace "--text--" with "--and--Process--text--" in tReplaceWith
      end if
      if offset ("<registry access=" & quote & "read-only" & quote & "/>", tTextToReplace) > 0 then
         replace "--and--" with", " in tReplaceWith
         replace "--text--" with "--and--RegistryRead--text--" in tReplaceWith
      end if
      if offset ("<registry access=" & quote & "read-write" & quote & "/>", tTextToReplace) > 0 then
         replace "--and--" with", " in tReplaceWith
         replace "--text--" with "--and--RegistryWrite--text--" in tReplaceWith
      end if
      if offset ("<stack/>", tTextToReplace) > 0 then
         replace "--and--" with", " in tReplaceWith
         replace "--text--" with "--and--Stack--text--" in tReplaceWith
      end if
      if offset ("<printing/>", tTextToReplace) > 0 then
         replace "--and--" with", " in tReplaceWith
         replace "--text--" with "--and--Printing--text--" in tReplaceWith
      end if
      if offset ("<privacy/>", tTextToReplace) > 0 then
         replace "--and--" with", " in tReplaceWith
         replace "--text--" with "--and--Privacy--text--" in tReplaceWith
      end if
      if offset ("<applescript/>", tTextToReplace) > 0 then
         replace "--and--" with", " in tReplaceWith
         replace "--text--" with "--and--AppleScript--text--" in tReplaceWith
      end if
      if offset ("<doalternate/>", tTextToReplace) > 0 then
         replace "--and--" with", " in tReplaceWith
         replace "--text--" with "--and--DoAlternate--text--" in tReplaceWith
      end if
      replace ", , " with "" in tReplaceWith
      replace "--and----text--" with "" in tReplaceWith
      replace "--text--" with "" in tReplaceWith
      replace ", --and--" with "" in tReplaceWith
      replace "--and--" with " and " in tReplaceWith
      replace tTextToReplace with tReplaceWith in rData
   end if
end libUpdateSecurity

private command libUpdateExampleExamples @rData
   // updates the <examples><example></example></examples> notation
   local tTextToReplace, tReplaceWith, tTextStart, tTextEnd, tCurrentExampleToReplace, tCurrentExampleToReplaceWith, tDoubleLineSpace, tExampleFound, tFirstExample
   put true into tFirstExample
   put true into tExampleFound
   put libGetTextLocation (rData, "<examples>", "</examples>", tTextStart, tTextEnd) into tTextToReplace
   if tTextToReplace is empty then
      breakpoint
   else
      if offset ("</p><p>", tTextToReplace) > 0 then
         put true into tDoubleLineSpace
      else
         put false into tDoubleLineSpace
      end if
      put tTextToReplace into tReplaceWith
      
      repeat while true
         put libGetTextLocation (tReplaceWith, "<example>", "</example>", tTextStart, tTextEnd) into tCurrentExampleToReplace
         if tCurrentExampleToReplace is empty then
            exit repeat
         end if
         put tCurrentExampleToReplace into tCurrentExampleToReplaceWith
         
         if tDoubleLineSpace is true and tFirstExample is false then
            replace "<example>" with "<p/><p>" in tCurrentExampleToReplaceWith
         else
            replace "<example>" with "<p>" in tCurrentExampleToReplaceWith
         end if
         replace "</example>" with "</p>" in tCurrentExampleToReplaceWith
         replace tCurrentExampleToReplace with tCurrentExampleToReplaceWith in tReplaceWith
         if tFirstExample is true then
            put false into tFirstExample
         end if
      end repeat
      
      replace "<examples>" with "<example>" in tReplaceWith
      replace "</examples>" with "</example>" in tReplaceWith
      replace "<p><p>" with "<p>" in tReplaceWith
      replace "</p></p>" with "</p>" in tReplaceWith
      replace tTextToReplace with tReplaceWith in rData
   end if
end libUpdateExampleExamples

private command libUpdateParameters @rData
   // updates the
   //    <parmeters>
   //        <parameter>
   //            <name></name>
   //            <description></description>
   //            <options title="">
   //                <option>
   //                    <value></value>
   //                    <description></description>
   //                </option>
   //            </options>
   //        </parameter>
   //  </parameters>
   local tTextToReplace , tTextToReplaceWith, tCurrentParameterToReplace, tCurrentParameterToReplaceWith, tCurrentParameterNameToReplace, tCurrentParameterNameToReplaceWith
   local tCurrentParameterDescriptionToReplace, tCurrentParameterDescriptionToReplaceWith, tCurrentParameterOptionsToReplace, tCurrentParameterOptionsToReplaceWith
   local tCurrentParameterOptionsOptionToReplace, tCurrentParameterOptionsOptionToReplaceWith
   local tCurrentParameterOptionsOptionItemToReplace, tCurrentParameterOptionsOptionItemToReplaceWith
   local tCurrentParameterOptionsOptionDescriptionToReplace, tCurrentParameterOptionsOptionDescriptionToReplaceWith
   local tCurrentParameterOptionsOptionValuesToReplace, tCurrentParameterOptionsOptionValuesToReplaceWith
   local tCurrentParameterOptionsOptionValuesValueToReplace, tCurrentParameterOptionsOptionValuesValueToReplaceWith
   local tCurrentParameterOptionsOptionValuesValueNameToReplace, tCurrentParameterOptionsOptionValuesValueNameToReplaceWith
   local tCurrentParameterOptionsOptionValuesValueDescriptionToReplace, tCurrentParameterOptionsOptionValuesValueDescriptionToReplaceWith
   local tTextStart, tTextEnd, tStrip, tOptionsTitle, tOptionsTitleWith
   // little test from here
      //if "<legacy_id>2448</legacy_id>" is among the words of rData or "<legacy_id>3388</legacy_id>" is among the words of rData then
         //put tstLine into tstLine
      //end if
   // little test to here      
   put libGetTextLocation (rData, "<parameters>", "</parameters>", tTextStart, tTextEnd) into tTextToReplace
   if the number of lines in tTextToReplace <= 2 and the number of characters in tTextToReplace < 30 then
      if tTextToReplace is not empty then
         replace tTextToReplace with "" in rData
      end if
   else
      put tTextToReplace into tTextToReplaceWith
      // HANDLE THE PARAMETERS
      repeat while true
         put libGetTextLocation (tTextToReplaceWith, "<parameter>", "</parameter>", tTextStart, tTextEnd) into tCurrentParameterToReplace
         if tCurrentParameterToReplace is empty then
            exit repeat
         end if
         put tCurrentParameterToReplace into tCurrentParameterToReplaceWith
         put libGetTextLocation (tCurrentParameterToReplaceWith, "<name>", "</name>", tTextStart, tTextEnd) into tCurrentParameterNameToReplace
         if tCurrentParameterNameToReplace is not empty then
            put tCurrentParameterNameToReplace into tCurrentParameterNameToReplaceWith
            replace "<name>" with "<i>" in tCurrentParameterNameToReplaceWith
            replace "</name>" with "</i><#S#><#S#>" in tCurrentParameterNameToReplaceWith
            replace tCurrentParameterNameToReplace with tCurrentParameterNameToReplaceWith in tCurrentParameterToReplaceWith
         end if
         put libGetTextLocation (tCurrentParameterToReplaceWith, "<description>", "</description>", tTextStart, tTextEnd) into tCurrentParameterDescriptionToReplace
         if tCurrentParameterDescriptionToReplace is not empty then
            put tCurrentParameterDescriptionToReplace into tCurrentParameterDescriptionToReplaceWith
            replace "<description>" with "<#E#><#E#> - " in tCurrentParameterDescriptionToReplaceWith
            replace "</description>" with "" in tCurrentParameterDescriptionToReplaceWith
            replace tCurrentParameterDescriptionToReplace with tCurrentParameterDescriptionToReplaceWith in tCurrentParameterToReplaceWith
         end if
         
         // HANDLE OPTIONS
         repeat while true
            put libGetTextLocation (tCurrentParameterToReplaceWith, "<options", "</options>", tTextStart, tTextEnd) into tCurrentParameterOptionsToReplace
            if tCurrentParameterOptionsToReplace is empty then
               exit repeat
            end if
            put tCurrentParameterOptionsToReplace into tCurrentParameterOptionsToReplaceWith
            // HANDLE THE OPTIONS TITLE
            put libGetTextLocation (tCurrentParameterOptionsToReplaceWith, "title=" & quote, quote & ">", tTextStart, tTextEnd) into tOptionsTitle
            put tOptionsTitle into tOptionsTitleWith
            
            if tOptionsTitleWith is not ("title="&quote&quote&">") and tOptionsTitleWith is not empty then
               replace "title=" & quote with ("<p><u>") in tOptionsTitleWith
               replace quote & ">" with "</u></p><ul>" in tOptionsTitleWith
               replace tOptionsTitle with tOptionsTitleWith in tCurrentParameterOptionsToReplaceWith
            else
               replace tOptionsTitle with "<ul>"  in tCurrentParameterOptionsToReplaceWith
            end if
            repeat while true
               put libGetTextLocation (tCurrentParameterOptionsToReplaceWith, "<option>", "</option>", tTextStart, tTextEnd) into tCurrentParameterOptionsOptionToReplace
               if tCurrentParameterOptionsOptionToReplace is empty then
                  exit repeat
               end if
               put tCurrentParameterOptionsOptionToReplace into tCurrentParameterOptionsOptionToReplaceWith
               put libGetTextLocation (tCurrentParameterOptionsOptionToReplaceWith, "<item>", "</item>", tTextStart, tTextEnd) into tCurrentParameterOptionsOptionItemToReplace
               if tCurrentParameterOptionsOptionItemToReplace is not empty then
                  put tCurrentParameterOptionsOptionItemToReplace into tCurrentParameterOptionsOptionItemToReplaceWith
                  replace "<item>" with "<li><b>" in tCurrentParameterOptionsOptionItemToReplaceWith
                  // HSC - replace "</item>" with "</b><#S#><#S#></li>" in tCurrentParameterOptionsOptionItemToReplaceWith
                  replace "</item>" with "</b><#S#><#S#>" in tCurrentParameterOptionsOptionItemToReplaceWith
                  replace tCurrentParameterOptionsOptionItemToReplace with tCurrentParameterOptionsOptionItemToReplaceWith in tCurrentParameterOptionsOptionToReplaceWith
               end if
               put libGetTextLocation (tCurrentParameterOptionsOptionToReplaceWith, "<description>", "</description>", tTextStart, tTextEnd) into tCurrentParameterOptionsOptionDescriptionToReplace
               if tCurrentParameterOptionsOptionDescriptionToReplace is not empty then
                  put tCurrentParameterOptionsOptionDescriptionToReplace into tCurrentParameterOptionsOptionDescriptionToReplaceWith
                  replace "<description>" with "<#E#><#E#> - " in tCurrentParameterOptionsOptionDescriptionToReplaceWith
                  // HSC - replace "</description>" with "" in tCurrentParameterOptionsOptionDescriptionToReplaceWith
                  replace "</description>" with "</li>" in tCurrentParameterOptionsOptionDescriptionToReplaceWith
                  replace tCurrentParameterOptionsOptionDescriptionToReplace with tCurrentParameterOptionsOptionDescriptionToReplaceWith in tCurrentParameterOptionsOptionToReplaceWith
               else
                  if offset ("<description/>", tCurrentParameterOptionsOptionToReplaceWith) > 0 then
                     // HSC - replace "<description/>" with "<#E#><#E#>" in tCurrentParameterOptionsOptionToReplaceWith
                     replace "<description/>" with "<#E#><#E#></li>" in tCurrentParameterOptionsOptionToReplaceWith
                  end if
               end if
               
               // HANDLE VALUES
               put libGetTextLocation (tCurrentParameterOptionsOptionToReplaceWith, "<values>", "</values>", tTextStart, tTextEnd) into tCurrentParameterOptionsOptionValuesToReplace
               if tCurrentParameterOptionsOptionValuesToReplace is not empty then
                  put tCurrentParameterOptionsOptionValuesToReplace into tCurrentParameterOptionsOptionValuesToReplaceWith
                  repeat while true
                     put libGetTextLocation (tCurrentParameterOptionsOptionValuesToReplaceWith, "<value>", "</value>", tTextStart, tTextEnd) into tCurrentParameterOptionsOptionValuesValueToReplace
                     if tCurrentParameterOptionsOptionValuesValueToReplace is empty then
                        exit repeat
                     end if
                     put tCurrentParameterOptionsOptionValuesValueToReplace into tCurrentParameterOptionsOptionValuesValueToReplaceWith
                     put libGetTextLocation (tCurrentParameterOptionsOptionValuesValueToReplaceWith, "<name>", "</name>", tTextStart, tTextEnd) into tCurrentParameterOptionsOptionValuesValueNameToReplace
                     if offset ("<description/>", tCurrentParameterOptionsOptionValuesValueToReplaceWith) > 0
                     then
                        replace "<description/>" with "" in tCurrentParameterOptionsOptionValuesValueToReplaceWith
                        if tCurrentParameterOptionsOptionValuesValueNameToReplace is not empty then
                           put tCurrentParameterOptionsOptionValuesValueNameToReplace into tCurrentParameterOptionsOptionValuesValueNameToReplaceWith
                           replace "<name>" with "" in tCurrentParameterOptionsOptionValuesValueNameToReplaceWith
                           replace "</name>" with "" in tCurrentParameterOptionsOptionValuesValueNameToReplaceWith
                           replace tCurrentParameterOptionsOptionValuesValueNameToReplace with tCurrentParameterOptionsOptionValuesValueNameToReplaceWith in tCurrentParameterOptionsOptionValuesValueToReplaceWith
                        end if
                     else
                        if tCurrentParameterOptionsOptionValuesValueNameToReplace is not empty then
                           put tCurrentParameterOptionsOptionValuesValueNameToReplace into tCurrentParameterOptionsOptionValuesValueNameToReplaceWith
                           replace "<name>" with "" in tCurrentParameterOptionsOptionValuesValueNameToReplaceWith
                           replace "</name>" with "<#S#><#S#>" in tCurrentParameterOptionsOptionValuesValueNameToReplaceWith
                           replace tCurrentParameterOptionsOptionValuesValueNameToReplace with tCurrentParameterOptionsOptionValuesValueNameToReplaceWith in tCurrentParameterOptionsOptionValuesValueToReplaceWith
                        end if
                        put libGetTextLocation (tCurrentParameterOptionsOptionValuesValueToReplaceWith, "<description>", "</description>", tTextStart, tTextEnd) into tCurrentParameterOptionsOptionValuesValueDescriptionToReplace
                        if tCurrentParameterOptionsOptionValuesValueDescriptionToReplace is not empty then
                           put tCurrentParameterOptionsOptionValuesValueDescriptionToReplace into tCurrentParameterOptionsOptionValuesValueDescriptionToReplaceWith
                           replace "<description>" with "<#E#><#E#> - " in tCurrentParameterOptionsOptionValuesValueDescriptionToReplaceWith
                           replace "</description>" with "" in tCurrentParameterOptionsOptionValuesValueDescriptionToReplaceWith
                           replace tCurrentParameterOptionsOptionValuesValueDescriptionToReplace with tCurrentParameterOptionsOptionValuesValueDescriptionToReplaceWith in tCurrentParameterOptionsOptionValuesValueToReplaceWith
                        end if
                     end if
                     replace "<value>" with "<li>" in tCurrentParameterOptionsOptionValuesValueToReplaceWith
                     replace "</value>" with "</li>" in tCurrentParameterOptionsOptionValuesValueToReplaceWith
                     replace tCurrentParameterOptionsOptionValuesValueToReplace with tCurrentParameterOptionsOptionValuesValueToReplaceWith in tCurrentParameterOptionsOptionValuesToReplaceWith
                  end repeat
                  replace "<values>" with "<ul type="&quote&"circle"&quote&">" in tCurrentParameterOptionsOptionValuesToReplaceWith
                  replace "</values>" with "</ul>" in tCurrentParameterOptionsOptionValuesToReplaceWith
                  replace tCurrentParameterOptionsOptionValuesToReplace with tCurrentParameterOptionsOptionValuesToReplaceWith in tCurrentParameterOptionsToReplaceWith
               end if               
               replace "<option>" with "" in tCurrentParameterOptionsOptionToReplaceWith
               //HSC - replace "</option>" with "</ul>" in tCurrentParameterOptionsOptionToReplaceWith
               replace "</option>" with "" in tCurrentParameterOptionsOptionToReplaceWith
               replace tCurrentParameterOptionsOptionToReplace with tCurrentParameterOptionsOptionToReplaceWith in tCurrentParameterOptionsToReplaceWith
            end repeat
            replace "<options" with "" in tCurrentParameterOptionsToReplaceWith
            replace "</options>" with "</ul>" in tCurrentParameterOptionsToReplaceWith
            replace tCurrentParameterOptionsToReplace with tCurrentParameterOptionsToReplaceWith in tCurrentParameterToReplaceWith
         end repeat
         replace "<parameter>" with "<p>" in tCurrentParameterToReplaceWith
         replace "</parameter>" with "</p>" in tCurrentParameterToReplaceWith
         replace tCurrentParameterToReplace with tCurrentParameterToReplaceWith in tTextToReplaceWith
      end repeat
      
      replace "<parameters>" with "<p><b>Parameters:</b></p>" in tTextToReplaceWith
      replace "</parameters>" with "" in tTextToReplaceWith
      // STRIP OUT ALL MARKED BREAKS
      repeat while true
         put libGetTextLocation (tTextToReplaceWith, "<#S#><#S#>", "<#E#><#E#>", tTextStart, tTextEnd) into tStrip
         if tStrip is empty then
            exit repeat
         end if
         replace tStrip with "" in tTextToReplaceWith
      end repeat
      // STRIP OUT THE <description/> tags
      replace tTextToReplace with tTextToReplaceWith in rData
   end if
   replace "    " with "" in rData
   return rData
end libUpdateParameters

private command libUpdateDescription @rData
   local tTemp
   // strip the overview, parameters, value and comments tags out of the description tags 
   local tTextToReplace, tTextToReplaceSub, tTextToReplaceSubWith, tReplaceWith, tTextStart, tTextEnd, tTextStartSub, tTextEndSub, tPTagRequired
   put false into tPTagRequired
   put libGetTextLocation (rData, "<description>", "</description>", tTextStart, tTextEnd) into tTextToReplace
   if tTextToReplace is empty then
      breakpoint
   else
      put tTextToReplace into tReplaceWith
      // process the overview tags
      put libGetTextLocation (tReplaceWith, "<overview>", "</overview>", tTextStartSub, tTextEndSub) into tTextToReplaceSub
      if tTextToReplaceSub is not empty then
         put the number of characters in tTextToReplaceSub into tTemp
         if the number of lines in tTextToReplaceSub <= 2 and the number of characters in tTextToReplaceSub < 26 then
            replace tTextToReplaceSub with "" in tReplaceWith
         else
            replace "<overview>" with "<p>" in tReplaceWith
            replace "</overview>" with "</p>" in tReplaceWith
            put true into tPTagRequired
         end if
      end if
      // process the parameter tags
      put libGetTextLocation (tReplaceWith, "<parameters>", "</parameters>", tTextStartSub, tTextEndSub) into tTextToReplaceSub
      if tTextToReplaceSub is not empty then
         if the number of lines in tTextToReplaceSub <= 2 and the number of characters in tTextToReplaceSub < 30 then
            replace tTextToReplaceSub with "" in tReplaceWith
         else
            if tPTagRequired is true then
               //replace "<parameters>" with "<p/><p><b>Parameters:</b></p><p>" in tReplaceWith
               replace "<parameters>" with "<p/><p><b>Parameters:</b></p>" in tReplaceWith
            else
               //replace "<parameters>" with "<p><b>Parameters:</b></p><p>" in tReplaceWith
               replace "<parameters>" with "<p><b>Parameters:</b></p>" in tReplaceWith
               put true into tPTagRequired
            end if
            //replace "</parameters>" with "</p>" in tReplaceWith
            replace "</parameters>" with "" in tReplaceWith
            replace "<parameter>" with "<p>" in tReplaceWith
            replace "</parameter>" with "</p>" in tReplaceWith
         end if
      end if
      // process the value tags
      put libGetTextLocation (tReplaceWith, "<value>", "</value>", tTextStartSub, tTextEndSub) into tTextToReplaceSub
      if tTextToReplaceSub is not empty then
         put the number of characters in tTextToReplaceSub into tTemp
         if the number of lines in tTextToReplaceSub <= 2 and the number of characters in tTextToReplaceSub < 20 then
            replace tTextToReplaceSub with "" in tReplaceWith
         else
            if tPTagRequired is true then
               replace "<value>" with "<p/><p><b>Value:</b></p><p>" in tReplaceWith
            else
               replace "<value>" with "<p><b>Value:</b></p><p>" in tReplaceWith
               put true into tPTagRequired
            end if
            replace "</value>" with "</p>" in tReplaceWith
         end if
      end if
      // process the comments tags
      put libGetTextLocation (tReplaceWith, "<comments>", "</comments>", tTextStartSub, tTextEndSub) into tTextToReplaceSub
      if tTextToReplaceSub is not empty then
         put the number of characters in tTextToReplaceSub into tTemp
         if the number of lines in tTextToReplaceSub <= 2 and the number of characters in tTextToReplaceSub < 26 then
            replace tTextToReplaceSub with "" in tReplaceWith
         else
            if tPTagRequired is true then
               replace "<comments>" with "<p/><p><b>Comments:</b></p><p>" in tReplaceWith
            else
               replace "<comments>" with "<p><b>Comments:</b></p><p>" in tReplaceWith
               put true into tPTagRequired
            end if
            replace "</comments>" with "</p>" in tReplaceWith
         end if
      end if
      replace tTextToReplace with tReplaceWith in rData
   end if
end libUpdateDescription

private command libRemoveObsolete @rData
   // removes XML tags that contain no data
   // this is needed to suppress referring headings
   local tTextToReplace, tReplaceWith, tTextStart, tTextEnd
   replace "<library></library>" with "" in rData
   put libGetTextLocation (rData, "<objects>", "</objects>", tTextStart, tTextEnd) into tTextToReplace
   if the number of lines in tTextToReplace is 2 then
      replace tTextToReplace with "" in rData
   end if
   put libGetTextLocation (rData, "<synonyms>", "</synonyms>", tTextStart, tTextEnd) into tTextToReplace
   if the number of lines in tTextToReplace is 2 then
      replace tTextToReplace with "" in rData
   end if
   put libGetTextLocation (rData, "<security>", "</security>", tTextStart, tTextEnd) into tTextToReplace
   if the number of lines in tTextToReplace is 2 then
      replace tTextToReplace with "" in rData
   end if
   put libGetTextLocation (rData, "<syntax>", "</syntax>", tTextStart, tTextEnd) into tTextToReplace
   if the number of lines in tTextToReplace is 2 then
      replace tTextToReplace with "" in rData
   end if   
end libRemoveObsolete

private command libUpdateGeneral @rData
   local tTextStart, tTextEnd, tTextToUpdate
   // updates the <warning></warning> notation
   put libGetTextLocation (rData, "<warning>", "</warning>", tTextStart, tTextEnd) into tTextToUpdate
   if tTextToUpdate is not empty then
      replace "<warning>" with "<b>Warning: </b>" in rData
      replace "</warning>" with "" in rData
   end if
   // updates the <note></note> notation
   put libGetTextLocation (rData, "<note>", "</note>", tTextStart, tTextEnd) into tTextToUpdate
   if tTextToUpdate is not empty then
      replace "<note>" with "<b>Note: </b>" in rData
      replace "</note>" with "" in rData
   end if
   // updates the <cp_note></cp_note> notation
   put libGetTextLocation (rData, "<cp_note>", "</cp_note>", tTextStart, tTextEnd) into tTextToUpdate
   if tTextToUpdate is not empty then
      replace "<cp_note>" with "<b><img src=" & quote & "202652" & quote & "/>  Cross-platform note: </b>" in rData
      replace "</cp_note>" with "" in rData
   end if
   // updates the <caution></caution> notation
   put libGetTextLocation (rData, "<caution>", "</caution>", tTextStart, tTextEnd) into tTextToUpdate
   if tTextToUpdate is not empty then
      replace "<caution>" with "<b><img src=" & quote & "202650" & quote & "/> Caution! </b>" in rData
      replace "</caution>" with "" in rData
   end if
   // updates the <cp_caution></cp_caution> notation
   put libGetTextLocation (rData, "<cp_caution>", "</cp_caution>", tTextStart, tTextEnd) into tTextToUpdate
   if tTextToUpdate is not empty then
      replace "<cp_caution>" with "<b><img src=" & quote & "202654" & quote & "/> Cross-platform caution! </b>" in rData
      replace "</cp_caution>" with "" in rData
   end if
   // updates the <change></change> notation
   put libGetTextLocation (rData, "<change>", "</change>", tTextStart, tTextEnd) into tTextToUpdate
   if tTextToUpdate is not empty then
      replace "<change>" with "<b>Changes: </b>" in rData
      replace "</change>" with "" in rData
   end if
   // updates the <tip></tip> notation
   put libGetTextLocation (rData, "<tip>", "</tip>", tTextStart, tTextEnd) into tTextToUpdate
   if tTextToUpdate is not empty then
      replace "<tip>" with "<b><img src=" & quote & "202651" & quote & "/> Tip: </b>" in rData
      replace "</tip>" with "" in rData
   end if
   // updates the <important></important> notation
   put libGetTextLocation (rData, "<important>", "</important>", tTextStart, tTextEnd) into tTextToUpdate
   if tTextToUpdate is not empty then
      replace "<important>" with "<b><img src=" & quote & "202653" & quote & "/> Important! </b>" in rData
      replace "</important>" with "" in rData
   end if
 
   // strip out ASCII 13
   replace numToChar(13) & numToChar(10) with numToChar(10) in rData
   replace numToChar(13) with numToChar(10) in rData
end libUpdateGeneral

private command libUpdateBoldName @rData
   local tTextStart, tTextEnd, tNameWithTags, tName, tDescription, tDescriptionReplaceWith
   put libGetTextLocation (rData, "<name>", "</name>", tTextStart, tTextEnd) into tNameWithTags
   if tNameWithTags is empty then
      breakpoint
   else
      put tNameWithTags into tName
      replace "<name>" with "" in tName
      replace "</name>" with "" in tName
   end if
   if tName is empty then
      breakpoint
   else
      put libGetTextLocation (rData, "<description>", "</description>", tTextStart, tTextEnd) into tDescription
      put tDescription into tDescriptionReplaceWith
      replace tName with "<b>" & tName & "</b>" in tDescriptionReplaceWith
      replace "<b><b>" with "<b>" in tDescriptionReplaceWith
      replace "<b> <b>" with "<b>" in tDescriptionReplaceWith
      replace "</b></b>" with "</b>" in tDescriptionReplaceWith
      replace "</b> </b>" with "</b>" in tDescriptionReplaceWith
      replace tDescription with tDescriptionReplaceWith in rData
   end if
end libUpdateBoldName

private command libRemoveSynonyms @rData
   local tTextStart, tTextEnd, tTextToRemove
   put libGetTextLocation (rData, "<synonyms>", "</synonyms>", tTextStart, tTextEnd) into tTextToRemove
   if tTextToRemove is empty then
      breakpoint
   else
      replace tTextToRemove with "<synonyms>" & return & tab &"</synonyms>" in rData
   end if
end libRemoveSynonyms

private command libDocumentationOldFormat @rData, @rFileToDirectory, @rLinkLookup
   if offset ("<term>", rData) is 0 then
      // do the dictionary specific stuff
      libUpdateReferences rData, rFileToDirectory, rLinkLookup
   end if
   // now replace the links with the old type 
   libReplaceLinks rData, rFileToDirectory, rLinkLookup
   if offset ("<term>", rData) > 0 then
      // do the glossary specific stuff
      libRemoveSynonyms rData
   else
      // do the dictionary specific stuff
      libUpdateSyntaxExamples rData
      libUpdateObjects rData
      libUpdateClassification rData
      libUpdateHistory rData
      libUpdatePlatforms rData
      libUpdateClasses rData
      libUpdateSecurity rData
      libUpdateSummary rData
      libUpdateExampleExamples rData
      libUpdateParameters rData
      libUpdateDescription rData
      libRemoveObsolete rData
      libUpdateGeneral rData
      // little test from here
      //if "</ul>" is among the words of rData then
         //put tstLine into tstLine
      //end if
      // little test to here      
      //libUpdateBoldName rData
   end if
end libDocumentationOldFormat
   
// HSC - tested - complies with current dictionary format
private command pathEnsure pPath
   if pPath is empty then
      exit pathEnsure
   end if
   
   set the itemDelimiter to "/"
   local tPath
   repeat for each item tPart in pPath
      put tPart & "/" after tPath
      if there is no folder tPath then
         create folder tPath
      end if
   end repeat
end pathEnsure

private function listContainsAllItems pList, pItems
   repeat for each item tItem in pItems
      if tItem is among the items of pList then
         next repeat
      else
         return false
      end if
   end repeat
   return true
end listContainsAllItems

////////////////////////////////////////////////////////////////////////////////

private function escape pString, pConvertLineEndings
   replace "\" with "\\" in pString
   replace quote with ("\" & quote) in pString
   
   if pConvertLineEndings is true then
      replace (numToChar(13) & CR) with (CR & numToChar(13)) in pString
      replace (CR & numToChar(13)) with CR in pString
   end if
   replace CR with "\n" in pString
   //replace "<" with "&lt;" in pString
   //replace ">" with "&gt;" in pString
   replace tab with "\t" in pString
   return (quote & pString & quote)
end escape

command docsBuilderGenerateDocsNew
   start using stack (builderRepoFolder() & slash & "ide-support" & slash & "revdocsparser.livecodescript")
   builderLog "report", "Building guide into" && builderGuideFolder() & slash & "distributed_guide.js"
   pathEnsure (builderGuideFolder())
   docsBuilderGenerateDistributedGuide
   builderLog "report", "Building api into" && builderAPIFolder() & slash & "distributed_api.js"
   pathEnsure (builderAPIFolder())
   docsBuilderGenerateDistributedAPI   
   
   # Build the actual JSON data for the dictionary. This is needed until the docs are built on the fly in a 
   # suitable location - for the moment the dictionary will only display docs for preinstalled extensions.
   docsBuilderGenerateBuiltDocs
   
   builderLog "report", "Built docs"
end docsBuilderGenerateDocsNew

private function escape pString, pConvertLineEndings
   replace "\" with "\\" in pString
   replace quote with ("\" & quote) in pString
   
   if pConvertLineEndings is true then
      replace (numToChar(13) & CR) with (CR & numToChar(13)) in pString
      replace (CR & numToChar(13)) with CR in pString
   end if
   replace CR with "\n" in pString
   replace tab with "\t" in pString
   return (quote & pString & quote)
end escape

on docsBuilderGenerateBuiltDocs 
   local tExtensions
   
   local tDefaultFolder
   put the defaultFolder into tDefaultFolder
   set the defaultFolder to builderPackagedExtensionsFolder()
   put the folders into tExtensions
   filter tExtensions without ".."
   
   # Build guide data using distributed guide and extensions' guides
   docsBuilderGenerateBuiltGuide tExtensions
   # Build api data using distributed api and extensions' apis
   docsBuilderGenerateBuiltAPI tExtensions
   
   set the defaultFolder to tDefaultFolder
end docsBuilderGenerateBuiltDocs

private function builderGetDocsAPIData pExtensions
   local tData
   put url ("binfile:" & builderAPIFolder() & slash & "distributed_api.js") into tData
   
   local tFolder
   repeat for each line tExtensionFolder in pExtensions
      put builderPackagedExtensionsFolder() & slash & tExtensionFolder into tFolder
      
      set the defaultFolder to tFolder
      # Isolate the source file
      local tSource
      put the files into tSource
      filter tSource with "*.lcb"
      
      # Generate the docs text in lcdoc format
      local tLcdoc
      put revDocsGenerateDocsFileFromModularFile(tFolder & slash & tSource) into tLcdoc
      
      # Output the lcdoc file
      put textEncode(tLcdoc, "utf-8") into url ("binfile:" & tFolder & slash & "api.lcdoc")
      
      # Convert to JSON
      put revDocsFormatDocTextAsJSON(tFolder & slash & "api.lcdoc", tLcdoc,"", "LiveCode") into tExtensionAPI
      
      # add to the data
      if tExtensionAPI is not empty then
         put comma & tExtensionAPI after tData
      end if
   end repeat
   
   return tData
end builderGetDocsAPIData

private function builderGetDocsGuideData pExtensions
   local tData
   put url ("binfile:" & builderGuideFolder() & slash & "distributed_guide.js") into tData
   
   local tFolder
   repeat for each line tExtensionFolder in pExtensions
      put builderPackagedExtensionsFolder() & slash & tExtensionFolder into tFolder
      if there is a not file (tFolder & slash & "guide.md")  then next repeat
      
      local tGuide
      put url ("binfile:" & tFolder & slash & "guide.md") into tGuide
      put textDecode(tGuide, "utf-8") into tGuide
      
      # If there is a guide, add it to the guide data
      if tGuide is not empty then
         put "{" & CR after tExtensionGuide
         put tab & quote & "guide" & quote & ":" && quote & tFolder & quote & comma & CR after tExtensionGuide
         put quote & "data" & quote & ":" & escape(tGuide, true) & CR after tExtensionGuide
         put "}" after tExtensionGuide
         
         put comma & tExtensionGuide after tData
      end if
   end repeat
   
   return tData
end builderGetDocsGuideData

on docsBuilderGenerateBuiltGuide pExtensions
   local tData
   put "var tUserGuideData =" & CR & "{" & CR & tab & quote & "guides" & quote & ":[" into tData
   
   # Add the guide data from each of the packaged extensions
   put builderGetDocsGuideData(pExtensions) after tData
   
   put CR & tab & "]" & CR & "}" after tData
   
   put tData into url ("binfile:" & builderGuideFolder() & slash & "built_guide.js")
end docsBuilderGenerateBuiltGuide

on docsBuilderGenerateBuiltAPI pExtensions
   local tData
   put "var dictionary_data =" & CR & "{" & CR & tab & quote & "docs" & quote & ":[" after tData
   
   # Add the api data from each of the packaged extensions
   put builderGetDocsAPIData(pExtensions) after tData
   
   put CR & tab & "]" & CR & "}" after tData
   put tData into url ("binfile:" & builderAPIFolder() & slash & "built_api.js")
end docsBuilderGenerateBuiltAPI

/*
command errorShow pError
   builderLog "error", pError
   throw pError
end errorShow
*/
command docsBuilderGenerateDistributedGuide
   local tDefaultFolder
   put the defaultFolder into tDefaultFolder

   local tGuideFolders
   put builderIDEDocsFolder() & slash & "guides" into tGuideFolders
   put return & builderRepoFolder() & slash & "docs" & slash & "guides" after tGuideFolders
   
   local tGuideFolder, tGuideData, tGuideName
   repeat for each line tGuideFolder in tGuideFolders
      set the defaultfolder to tGuideFolder
   
      set the itemdelimiter to "."
      repeat for each line tFile in the files
         if tFile begins with "."  then next repeat
         docsBuilderProgressUpdate "", "Building guide" && tGuideFolder & slash & tFile
         put item 1 to -2 of tFile into tGuideName
         get url ("binfile:" & tGuideFolder & slash & tFile)
         if it is not empty then
            get textDecode(it, "utf-8")
            replace "[toc]" with empty in it
            put tab & "{" & CR after tGuideData
            put tab & escape("name") & ":" & escape(revDocsModifyForURL(tGuideName)) & comma & return after tGuideData 
            put tab & escape("display name") & ":" & escape(tGuideName) & comma & return after tGuideData 
            put tab & escape("location") & ":" & escape("ide") & comma & return after tGuideData
            put tab & escape("data") & ":" & escape(it, true) & return & tab & "}," after tGuideData
         end if
      end repeat
   end repeat
   delete the last char of tGuideData
   put textEncode(tGuideData, "utf-8") into url ("binfile:" & builderGuideFolder() & slash & "distributed_guide.js")
   
   set the defaultFolder to tDefaultFolder
end docsBuilderGenerateDistributedGuide

/*
Summary: Parses the current directory into a library array.

pRootDir (string): The path to the root directory of the dictionary
*/

function docsBuilderParseDictionaryToLibraryArray pRootDir
   if there is not a folder pRootDir then
      return empty
   end if
   
   # Get the list of canonical glossary entries
   local tGlossaryA
   put revDocsCollectGlossarySynonyms(pRootDir) into tGlossaryA
   
   local tDictionaryRoot, tGlossaryRoot
   put pRootDir & slash & "dictionary" into tDictionaryRoot
   put pRootDir & slash & "glossary" into tGlossaryRoot
   
   local tCount
   put 1 into tCount
   
   local tText, tLibraryA, tParsedA
   
   repeat for each item tRoot in (tDictionaryRoot & "," & tGlossaryRoot)
      set the defaultfolder to tRoot
      repeat for each line tLine in the folders
         if tLine is ".." then next repeat
         docsBuilderProgressUpdate "", "Building dictionary from folder" && tRoot & slash & tLine
         set the defaultfolder to tRoot & slash & tLine
         get the files
         filter it with "*.lcdoc"
         repeat for each line tFile in it
            wait 0 with messages
            put textDecode(url ("binfile:" & tRoot & slash & tLine & slash & tFile), "utf-8") into tText
            put revDocsParseDocText(tText) into tParsedA
            put tParsedA["doc"][1] into tLibraryA[tCount]
            add 1 to tCount
         end repeat
      end repeat
   end repeat
   return tLibraryA
end docsBuilderParseDictionaryToLibraryArray

function docsBuilderParseDictionary pLibraryName, pAuthor, pRootDir, pRecursive
   local tLibraryA
   if there is a folder pRootDir then
      put revDocsModifyForUrl(pLibraryName) into tLibraryA["name"]
      put pLibraryName into tLibraryA["display name"]
      put pAuthor into tLibraryA["author"]
      put "dictionary" into tLibraryA["type"]
      
      put docsBuilderParseDictionaryToLibraryArray(pRootDir) into tLibraryA["doc"]
   end if
   
   if tLibraryA["doc"] is not empty then
      return tLibraryA
   end if
   
   return empty
end docsBuilderParseDictionary
   
end docsBuilderParseScriptDirectory

command docsBuilderGenerateDistributedAPI
   local tList, tDictionaryFolder
   set the itemdelimiter to slash
   put builderDocsFolder() into tDictionaryFolder
   set the itemdelimiter to comma
   if there is not a folder tDictionaryFolder then
      answer "No dictionary data found at" && tDictionaryFolder
   end if
   put "LiveCode Script," & tDictionaryFolder & ",dictionary" into tList
   
   local tLibrariesA, tCount
   put 1 into tCount
   repeat for each line tLine in tList
      put docsBuilderParseDictionary(item 1 of tLine, "LiveCode", item 2 of tLine, false) into tLibrariesA[tCount]
      add 1 to tCount
   end repeat
   
   local tModuleList, tModularA, tModularCount, tBlocksA, tParsedA
   put 1 into tModularCount
   put revDocsGetBuiltinModuleList(builderModuleInterfaceFolder(), builderRepoFolder()) into tModuleList
   set the itemdelimiter to "."
   repeat for each line tLine in tModuleList
      docsBuilderProgressUpdate "", "Adding docs from" && tLine
      if item -1 of tLine is "lcdoc" then
         get url("file:" & tLine)
      else
         get revDocsGenerateDocsFileFromModular(url ("file:" & tLine))
      end if
      if it is not empty then
         put it into tModularA[tModularCount]
         add 1 to tModularCount
      end if
   end repeat
   
   put 1 into tModularCount
   repeat for each element tElement in tModularA
      put revDocsParseDocText(tElement) into tParsedA
      repeat for each key tEntry in tParsedA["doc"]
         put tParsedA["doc"][tEntry] into tLibrariesA[tCount]["doc"][tModularCount]
         add 1 to tModularCount
      end repeat
      put empty into tParsedA
   end repeat
   put "LiveCode Builder" into tLibrariesA[tCount]["display name"]
   put revDocsModifyForUrl("LiveCode Builder") into tLibrariesA[tCount]["name"]
   put "LiveCode" into tLibrariesA[tCount]["author"]
   put "dictionary" into tLibrariesA[tCount]["type"]
   add 1 to tCount
   
   local tJSON
   put revDocsFormatLibrariesArrayAsJSON(tLibrariesA) into tJSON
   
   docsBuilderPopulateDatabase tLibrariesA
   
   put textEncode(tJSON, "utf-8") into url ("binfile:" & builderAPIFolder() & slash & "distributed_api.js")
end docsBuilderGenerateDistributedAPI

on docsBuilderPopulateDatabase pLibrariesA
   local tConnection
   revDocsOpenAPIDatabase builderAPIFolder()
   
   put the result into tConnection
   if tConnection is not a number then
      builderLog "error", the result
      exit docsBuilderPopulateDatabase
   end if
   
   revExecuteSQL tConnection,"BEGIN TRANSACTION"
   
   repeat for each key tKey in pLibrariesA   	  
   	  builderLog "message", "adding library" && pLibrariesA[tKey]["display name"] && "to docs database"
      revDocsUpdateDatabase tConnection, pLibrariesA[tKey]
      if the result is not empty then
         builderLog "error", the result
         exit docsBuilderPopulateDatabase
      end if
   end repeat
   
   revExecuteSQL tConnection,"COMMIT"
   revCloseDatabase tConnection
end docsBuilderPopulateDatabase

--- LEGACY

function revDocumentationRetrieve pType, pTag, pFolder
   -- We know how many files per clump cuz of kFilesPerClump
   -- pType is either "dictionary/" or "glossary/"
   local tClumpNumber, tFileNumber, tPropNumber, tPropName
   local tClumpName, tDocData, tClumpPath, tType, tOpenClumps
   ---------
   
   put pType into tType
   if the last char of tType is not slash then put slash after tType
   
   if pFolder is empty then
      put revEnvironmentDocumentationPath() & slash & "packaged_xml/" & tType into tClumpPath
   else
      put pFolder & slash & "packaged_xml/" & tType into tClumpPath
   end if
   
   set the itemDelimiter to "."
   put item 1 of pTag into tFileNumber
   if tFileNumber is not a number then
      return empty
   end if
   put ((tFileNumber - 1) div kFilesPerClump) + 1 into tClumpNumber
   put ((tFileNumber - 1) mod kFilesPerClump) + 1 into tPropNumber
   
   put "c" & tPropNumber into tPropName
   put the mainStacks into tOpenClumps
   filter tOpenClumps with kClumpNamePrefix & "*"
   
   put kClumpNamePrefix & tClumpNumber into tClumpName
   
   put the tPropName of stack (tClumpPath & tClumpName & ".rev") into tDocData
   if tDocData is empty then
      return empty
   end if
   
   -- We've got the doc's compressed data, uncompress and return
   return the decompress of tDocData
end revDocumentationRetrieve

