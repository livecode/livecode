script "DocsBuilder"

command docsBuilderProgressUpdate pProgress, pMessage
   builderLog "message", pMessage
end docsBuilderProgressUpdate

command logError pError
   local tContext
   // get the context of the calling handler: <handler>:<linenumber>
   put item -2 to -1 of line -2 of the executionContexts into tContext
   builderLog "error", pError, tContext
end logError

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

command docsBuilderRun pEdition, pVersion
   start using stack (builderRepoFolder() & slash & "ide-support/revdocsparser.livecodescript")
   
   builderExtUnpack pEdition
   
   builderLog "report", "Building guide into" && builderGuideFolder(pEdition) & slash & "distributed_guide.js"
   pathEnsure (builderGuideFolder(pEdition))
   docsBuilderGenerateDistributedGuide pEdition
   builderLog "report", "Building api into" && builderAPIFolder(pEdition) & slash & "distributed_api.js"
   pathEnsure (builderAPIFolder(pEdition))
   docsBuilderGenerateDistributedAPI pEdition
   
   docsBuilderGenerateZip pEdition, pVersion

   builderLog "report", "Built docs"
end docsBuilderRun

private command addToList pElement, @xArray
   local tCount
   put the number of elements in xArray into tCount
   put pElement into xArray[tCount + 1]
end addToList

private command addFolderToListIfExists pFolder, pThrowIfNot, @xArray
   if there is a folder pFolder then
      addToList pFolder, xArray
      exit addFolderToListIfExists
   end if
   
   if pThrowIfNot then
      logError "Expected docs folder" && pFolder && "doesn't exist"
   else 
      builderLog "message", "Folder" && pFolder && "doesn't exist"
   end if
end addFolderToListIfExists

private function RepoEditionDocsFolder pEdition
   if editionIsInPrivateRepo(pEdition) then
      return builderPrivateRepoFolder() & slash & "docs" & slash & toLower(pEdition)
   else
      return builderRepoFolder() & slash & "docs" & slash & toLower(pEdition)
   end if
end RepoEditionDocsFolder

private command docsBuilderGetGuideFolders pEdition, @xFolders
   local tRepoGuides
   put builderRepoFolder() & slash & "docs" & slash & "guides" into tRepoGuides
   addFolderToListIfExists tRepoGuides, true, xFolders
   
   local tIDEGuides
   put builderIDEDocsFolder() & slash & "guides" into tIDEGuides
   addFolderToListIfExists tIDEGuides, true, xFolders
   
   repeat for each item tEdition in "indy,business"
      if editionCompare(pEdition, tEdition) >= 0 then
         local tGuides
         put RepoEditionDocsFolder(tEdition) & slash & "guides" into tGuides
         addFolderToListIfExists tGuides, false, xFolders
      end if
   end repeat

   addFolderToListIfExists builderBuiltGuidesFolder(), false, xFolders
end docsBuilderGetGuideFolders

private command docsBuilderAddExtFolders pEdition, @xFolders
   local tDefaultFolder
   put the defaultFolder into tDefaultFolder
   
   local tTargetFolder
   put builderUnpackFolder() & slash & "Ext" into tTargetFolder
   set the defaultFolder to tTargetFolder
   
   repeat for each line tFolder in the folders
      if tFolder is ".." then next repeat
      addFolderToListIfExists tTargetFolder & slash & tFolder, true, xFolders
   end repeat
   
   set the defaultFolder to tDefaultFolder
end docsBuilderAddExtFolders

private command docsBuilderGetAdditionalFolders pEdition, @xFolders
   docsBuilderAddExtFolders pEdition, xFolders
end docsBuilderGetAdditionalFolders

private command docsBuilderGetDictionaryFolders pEdition, @xFolders
   local tCommunityDocs
   put builderRepoFolder() & slash & "docs" into tCommunityDocs
   addFolderToListIfExists tCommunityDocs, true, xFolders
   
   repeat for each item tEdition in "indy,business"
      if editionCompare(pEdition, tEdition) >= 0 then
         addFolderToListIfExists RepoEditionDocsFolder(tEdition), tEdition is "indy", xFolders
      end if
   end repeat
end docsBuilderGetDictionaryFolders

command docsBuilderGenerateDistributedGuide pEdition
   local tGuideFoldersA
   docsBuilderGetGuideFolders pEdition, tGuideFoldersA
   docsBuilderGetAdditionalFolders  pEdition, tGuideFoldersA
   
   local tGuideFoldersWithLocationA
   repeat for each element tElement in tGuideFoldersA
      local tGuideWithLocation
      put tElement into tGuideWIthLocation["folder"]
      # All these guides will be in the ide guide folder when installed
      put "ide" into tGuideWithLocation["location"]
      addToList tGuideWithLocation, tGuideFoldersWithLocationA
   end repeat
   
   local tOrderedGuidesA, tOrderFolder
   put builderIDEDocsFolder() & slash & "guides" into tOrderFolder
   put revDocsOrderedGuideData(tGuideFoldersWithLocationA, tOrderFolder) into tOrderedGuidesA
   
   local tGuideData
   repeat for each element tGuideA in tOrderedGuidesA
      docsBuilderProgressUpdate "", "Building guide" && tGuideA["file"]
      put tab & "{" after tGuideData
      put return & revDocsFormatGuideDataAsJSON(tGuideA) after tGuideData
      put return & tab & "}," after tGuideData
   end repeat
   delete the last char of tGuideData
   put textEncode(tGuideData, "utf-8") into url ("binfile:" & builderGuideFolder(pEdition) & slash & "distributed_guide.js")
end docsBuilderGenerateDistributedGuide

private function docsFileGetUTF8Contents pPath, pConvertLineEndings
   local tData, tText
   put url ("binfile:" & pPath) into tData
   put textDecode(tData, "utf-8") into tText
   if pConvertLineEndings then
      replace numToChar(13) & numToChar(10) with numToChar(10) in tText
      replace numToChar(13) with numToChar(10) in tText
   end if
   return tText
end docsFileGetUTF8Contents

-- Find all lcdoc files in pFolder and add them to the list of docs 
-- data in xLibraryA
private command addToDictionaryFromFolder pFolder, @xLibraryA
   local tDefaultFolder
   put the defaultFolder into tDefaultFolder
   
   docsBuilderProgressUpdate "", "Building dictionary from folder" && pFolder
   set the defaultfolder to pFolder
   get the files
   filter it with "*.lcdoc"
   repeat for each line tFile in it
      local tText, tParsedA
      wait 0 with messages
      put docsFileGetUTF8Contents(pFolder & slash & tFile, true) into tText
      put revDocsParseDocText(tText) into tParsedA
      repeat for each element tDoc in tParsedA["doc"]
         -- TODO: Work out why something empty is returned
         if tDoc["display name"] is empty then next repeat
         addToList tDoc, xLibraryA
      end repeat
   end repeat
   set the defaultFolder to tDefaultFolder
end addToDictionaryFromFolder

/*
Summary: Parses the current directory into a library array.

pRootDirs (array): An array of paths to the relevant root folders of the 
dictionary.
*/

command docsBuilderParseDictionaryToLibraryArray pRootDirs, @xDocsA
   local tDefaultFolder
   put the defaultFolder into tDefaultFolder
   
   # Get the list of canonical glossary entries
   local tGlossaryA
   repeat for each element tRootDir in pRootDirs
      union tGlossaryA with revDocsCollectGlossarySynonyms(tRootDir)
   end repeat
   
   repeat for each element tRootDir in pRootDirs      
      local tDictionaryRoot, tGlossaryRoot
      put tRootDir & slash & "dictionary" into tDictionaryRoot
      put tRootDir & slash & "glossary" into tGlossaryRoot
      
      repeat for each item tRoot in (tDictionaryRoot & "," & tGlossaryRoot)
         set the defaultfolder to tRoot
         repeat for each line tLine in the folders
            if tLine is ".." then next repeat
            addToDictionaryFromFolder tRoot & slash & tLine, xDocsA
         end repeat
      end repeat
   end repeat
   set the defaultFolder to tDefaultFolder
end docsBuilderParseDictionaryToLibraryArray

command docsBuilderParseAdditionalFoldersToLibraryArray pDirs, @xDocsA
   local tFolder
   repeat for each element tFolder in pDirs      
      addToDictionaryFromFolder tFolder, xDocsA
   end repeat
end docsBuilderParseAdditionalFoldersToLibraryArray

function docsBuilderParseDictionary pLibraryName, pAuthor, pRootDirsA, pAdditionalDirsA, pRecursive
   local tLibraryA
   
   put revDocsModifyForUrl(pLibraryName) into tLibraryA["name"]
   put pLibraryName into tLibraryA["display name"]
   put pAuthor into tLibraryA["author"]
   put "dictionary" into tLibraryA["type"]
   
   -- Special case for the dictionary / glossary data extraction
   docsBuilderParseDictionaryToLibraryArray pRootDirsA, tLibraryA["doc"]
   
   -- Add any additional docs to the dictionary
   docsBuilderParseAdditionalFoldersToLibraryArray pAdditionalDirsA, tLibraryA["doc"]
   
   if tLibraryA["doc"] is not empty then
      return tLibraryA
   end if
   
   return empty
end docsBuilderParseDictionary

command docsBuilderGenerateDistributedAPI pEdition
   local tDictionaryFolders, tAdditionalFolders
   docsBuilderGetDictionaryFolders pEdition, tDictionaryFolders
   docsBuilderGetAdditionalFolders pEdition, tAdditionalFolders
   
   local tLibrariesA
   
   local tLCSDictionaryA
   put docsBuilderParseDictionary("LiveCode Script", "LiveCode", tDictionaryFolders, tAdditionalFolders, false) into tLCSDictionaryA
   
   if tLCSDictionaryA is empty then
      logError "Couldn't parse script dictionary data for" && pEdition
      exit docsBuilderGenerateDistributedAPI
   end if
   
   addToList tLCSDictionaryA, tLibrariesA
   
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
   
   local tLCBDictionaryA
   put 1 into tModularCount
   repeat for each element tElement in tModularA
      put revDocsParseDocText(tElement) into tParsedA
      repeat for each key tEntry in tParsedA["doc"]
         put tParsedA["doc"][tEntry] into tLCBDictionaryA["doc"][tModularCount]
         add 1 to tModularCount
      end repeat
      put empty into tParsedA
   end repeat
   put "LiveCode Builder" into tLCBDictionaryA["display name"]
   put revDocsModifyForUrl("LiveCode Builder") into tLCBDictionaryA["name"]
   put "LiveCode" into tLCBDictionaryA["author"]
   put "dictionary" into tLCBDictionaryA["type"]
   
   if tLCBDictionaryA is empty then
      logError "Couldn't parse builder dictionary data for" && pEdition
      exit docsBuilderGenerateDistributedAPI
   end if
   
   addToList tLCBDictionaryA, tLibrariesA
   
   local tDatagridA, tDGDocs
   put builderIDEDocsFolder() & "/dictionary/datagrid.lcdoc" into tDGDocs
   put revDocsParseDocFileToLibraryArray(tDGDocs, "Data Grid", "LiveCode") \
      into tDataGridA

   if tDataGridA is empty then
      builderLog "error", "Couldn't parse datagrid dictionary data for" && pEdition
      exit docsBuilderGenerateDistributedAPI
   end if
   
   put "dictionary" into tDataGridA["type"]
   
   addToList tDataGridA, tLibrariesA
   
   local tJSON
   put revDocsFormatLibrariesArrayAsJSON(tLibrariesA) into tJSON
   
   docsBuilderPopulateDatabase pEdition, tLibrariesA
   
   put textEncode(tJSON, "utf-8") into url ("binfile:" & builderAPIFolder(pEdition) & slash & "distributed_api.js")
end docsBuilderGenerateDistributedAPI

on docsBuilderPopulateDatabase pEdition, pLibrariesA
   -- Ensure any existing api database is not updated
   local tExistingAPI
   put revDocsAPIDatabasePath(builderAPIFolder(pEdition)) into tExistingAPI
   if there is a file tExistingAPI then
      delete file tExistingAPI
   end if
   
   local tConnection
   revDocsOpenAPIDatabase builderAPIFolder(pEdition)
   
   put the result into tConnection
   if tConnection is not a number then
      logError the result
      exit docsBuilderPopulateDatabase
   end if
   
   revExecuteSQL tConnection,"BEGIN TRANSACTION"
   
   repeat for each key tKey in pLibrariesA   	  
      builderLog "message", "adding library" && pLibrariesA[tKey]["display name"] && "to docs database"
      revDocsUpdateDatabase tConnection, pLibrariesA[tKey]
      if the result is not empty then
         logError the result
         exit docsBuilderPopulateDatabase
      end if
   end repeat
   
   revExecuteSQL tConnection,"COMMIT"
   revCloseDatabase tConnection
end docsBuilderPopulateDatabase

private command docsBuilderGenerateZip pEdition, pVersion
   -- Ensure edition string is in titlecase
   put editionTitleCase(pEdition) into pEdition
   put replaceText(pVersion, "[-,\.]", "_") into pVersion

   local tInputs, tZipPath
   
   -- Make a list of folders that need to make it into the archived docs
   -- FIXME This has to be kept in sync with the installer manifest, probably
   docsBuilderGetGuideFolders pEdition, tInputs
   put builderIDEDocsFolder() & slash & "html_viewer" into tInputs[the number of elements in tInputs + 1]
   
   -- Figure out a filename for the docs archive
   put builderOutputFolder() & slash & \
         merge("LiveCode[[pEdition]]Docs-[[pVersion]].zip") into tZipPath
   
   -- Use the "zip" command to store the data
   local tDocItem, tCmd, tCmdOutput
   repeat for each element tDocItem in tInputs
      builderLog "message", merge("Archiving [[tDocItem]]")
      put merge("cd $(dirname '[[tDocItem]]') && zip -r '[[tZipPath]]' $(basename '[[tDocItem]]')") into tCmd
      put shell(tCmd) into tCmdOutput
      if the result is not 0 then
         logError "Archiving failed:" && tCmdOutput
         exit docsBuilderGenerateZip
      end if
   end repeat

   builderLog "message", merge("Generated [[tZipPath]]")
end docsBuilderGenerateZip

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
