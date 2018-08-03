script "revDocsParser"
/* 
Tests the features of the inline documentation parser & display

Synonyms: none

Associations: stack, revIdeDocsLibrary

pArrayParam (array): An array parameter
{ key : "key1"
value: "value1"
key : "key2"
value (array) : A sub-array
{ key (integer) : an integer key
value (string) : the description of the value at this index }
optional key : an optional key
value (string): the value associated with this optional key }

pEnumParam (enum): An enum parameter
-"option 1" : description of option 1
- option 2 : description of option 2
- option 3 : description of option 3 

pOptionalParam (optional integer): An integer parameter
pParam: A param afterward with no space
The result: Sets the result to empty
Returns (string): Description of string

Example:
// Some inline docs for myHandler...
on myHandler pParam
   // do something
end myHandler


// Some inline docs for myOtherHandler...
on myOtherHandler
   // do something other
end myOtherHandler

Description: 
Should have examples of *every* type of docs element in order to **make sure** they display correctly.
Can have some *markdown* in it:
* List
* Of
* Things

References:
myFavourite.command,ANOther.function

Tags: documentation, meta 
*/

function revDocsInlineDocsParsingTest pArrayParam, pEnumParam, pOptionalParam, pParam
   return empty
end revDocsInlineDocsParsingTest

function revDocsGetBuiltinModuleList pModuleInterfacePath, pRepoPath
   local tModulePath, tEngine, tScript, tListA
   
   put pModuleInterfacePath into tModulePath
   put pRepoPath & slash & "engine" & slash & "src" into tEngine
   put pRepoPath & slash & "libscript" & slash & "src" into tScript
   
   local tInstalled
   put files(tModulePath) into tInstalled
   filter tInstalled with "*.lci"
   
   local tSourcePaths
   put tScript into tSourcePaths[1]
   put tEngine into tSourcePaths[2]
   
   local tCount
   put 1 into tCount
   
   set the itemdelimiter to "."
   local tFiles
   repeat for each element tElement in tSourcePaths
      local tDocsA
      put files(tElement) into tFiles
      filter tFiles with regex pattern ".*(\.(mlc|lcb))+"
      # Check to see if the corresponding interface file exists so that we 
      # don't add anything to the dictionary that isn't actually there.
      repeat for each line tFile in tFiles
         local tInterfaceFile
         put "com.livecode." & tFile into tInterfaceFile
         put "lci" into item -1 of tInterfaceFile
         if replaceText(tInterfaceFile, "-", "") is not among the lines of tInstalled then next repeat
         put true into tListA[tElement & slash & tFile]
      end repeat
   end repeat
   
   put files(pRepoPath & slash & "docs" & slash & "builder") into tFiles
   filter tFiles with "*.lcdoc"
   repeat for each line tFile in tFiles
      put true into tListA[pRepoPath & slash & "docs" & slash & "builder" & slash & tFile]
   end repeat
   
   return the keys of tListA
end revDocsGetBuiltinModuleList

function revDocsGenerateDocsFilesFromBuiltIn pModuleInterfacePath, pRepoPath
   local tModulePath, tEngine, tScript
   
   put pModuleInterfacePath into tModulePath
   put pRepoPath & slash & "engine" & slash & "src" into tEngine
   put pRepoPath & slash & "libscript" & slash & "src" into tScript
   
   local tInstalled
   put files(tModulePath) into tInstalled
   filter tInstalled with "*.lci"
   
   local tSourcePaths
   put tScript into tSourcePaths[1]
   put tEngine into tSourcePaths[2]
   
   local tCount
   put 1 into tCount
   
   set the itemdelimiter to "."
   local tFiles
   repeat for each element tElement in tSourcePaths
      local tDocsA
      put files(tElement) into tFiles
      filter tFiles with regex pattern ".*(\.(mlc|lcb))+"
      # Check to see if the corresponding interface file exists so that we 
      # don't add anything to the dictionary that isn't actually there.
      repeat for each line tFile in tFiles
         local tInterfaceFile
         put "com.livecode." & tFile into tInterfaceFile
         put "lci" into item -1 of tInterfaceFile
         if replaceText(tInterfaceFile, "-", "") is not among the lines of tInstalled then next repeat
         get revDocsGenerateDocsFileFromModularFile(tFile)
         if it is not empty then
            put it into tDocsA[tCount]
            add 1 to tCount
         end if
      end repeat
   end repeat
   
   put files(pRepoPath & slash & "docs" & slash & "builder") into tFiles
   filter tFiles with "*.lcdoc"
   repeat for each line tFile in tFiles
      local tContents
      put revDocsUtf8FileContents(tFile) into tContents
      if tContents is not empty then
         put tContents into tDocsA[tCount]
         add 1 to tCount
      end if
   end repeat
   
   return tDocsA
end revDocsGenerateDocsFilesFromBuiltIn

/*
Summary: Formats a libraries array as JSON.

pLibrariesA (array): A libraries array is a numerically keyed array of library arrays. 

Description:
Each element of the numerically keyed <pLibrariesA> is a library array. 
The latter consists of three keys, "name", "author", and "doc". The first two of these are strings.
The third is an array. The "doc" subarray is numerically keyed. 
Each element of it represents the documentation for a handler contained in the library.

There are a number of ways of creating a library array:
* <revDocsParseDirectoryToLibraryArray> creates a library array from all of the (non-inline) docs entries of a given type in a directory
* <revDocsParseDictionaryToLibraryArray> creates the dictionary library array

*/
function revDocsFormatLibrariesArrayAsJSON pLibrariesA
   local tJSON
   repeat for each element tLibraryA in pLibrariesA
      get revDocsFormatLibraryArrayAsJSON(tLibraryA) 
      if it is not empty then 
         put it & comma after tJSON
      end if
   end repeat
   
   delete the last char of tJSON
   return tJSON
end revDocsFormatLibrariesArrayAsJSON

/* 
Summary: Create a library array with all the docs data from the specified directory

Parameters:
pLibraryName (string): The name of the library
pAuthor(string): The author of the library
pRootDir(string): The root directory to search for files
pType (enum): The type of docs to parse
- "modular": This will take any inline docs in .lcb files as well as all standard .lcdoc files in the directory
- "script": This will take any inline docs in .livecode files as well as all standard .lcdoc files in the directory
- "dictionary": This will take the standard .lcdoc files in the directory and parse as dicitonary entries

Returns(array): An array consisting of all the parsed docs data
{ key: "name"
value(string): the name of the library
key: "author"
value(string): the author of the library
key: "doc"
value(array): an array, with one key per handler or syntax element
{ key(string): the name of the handler or syntax element
value(array): an array of all the docs elements for this handler or piece of syntax
}
}
*/

function revDocsParseDirectoryToLibraryArray pLibraryName, pAuthor, pRootDir, pType, pRecursive
   local tLibraryA
   if there is a folder pRootDir then
      put pLibraryName into tLibraryA["name"]
      put pAuthor into tLibraryA["author"]
      put pType into tLibraryA["type"]
      
      if pType is "dictionary" then
         put revDocsParseDictionaryToLibraryArray(pRootDir) into tLibraryA["doc"]
      else
         put __revDocsParseDirectoryToLibraryArray(pType, pRootDir, pRecursive) into tLibraryA["doc"]
      end if
   end if
   
   if tLibraryA["doc"] is not empty then
      return tLibraryA
   end if
   
   return empty
end revDocsParseDirectoryToLibraryArray


/*
Summary: Parses the current directory into a library array.

pRootDir (string): The path to the root directory of the dictionary
*/

function revDocsParseDictionaryToLibraryArray pRootDir
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
      repeat for each line tLine in folders(tRoot)
         if tLine is ".." then next repeat
         get files(tRoot & slash & tLine)
         filter it with "*.lcdoc"
         repeat for each line tFile in it
            if the environment is "development" and \
                not revTestEnvironment() then
               wait 0 with messages
            end if
            local tFullPath
            put tRoot & slash & tLine & slash & tFile into tFullPath
            put revDocsUtf8FileContents(tFullPath) into tText
            put revDocsParseDocText(tText, tFullPath) into tParsedA
            put tParsedA["doc"][1] into tLibraryA[tCount]
            add 1 to tCount
         end repeat
      end repeat
   end repeat
   return tLibraryA
end revDocsParseDictionaryToLibraryArray

/*
Summary: Takes a .lcdoc file and formats it as a JSON array.
*/

function revDocsFormatAPIAsJSON pLibraryName, pAuthor, pAPIFile
   # Get the data
   local tText
   put revDocsUtf8FileContents(pAPIFile) into tText
   
   return revDocsFormatDocTextAsJSON(pAPIFile, tText, pLibraryName, pAuthor)
end revDocsFormatAPIAsJSON

/*
Summary: Takes a .lcdoc file and parses it into a library array.
*/
function revDocsParseFileToArray pFile, pType, @xCount, @xLibraryA
   # Get the data
   local tText
   put revDocsUtf8FileContents(pFile) into tText
   
   return revDocsParseFileContentsToArray(pFile, tText, pType, xCount, xLibraryA)
end revDocsParseFileToArray

function revDocsParseFileContentsToArray pFile, pText, pType, @xCount, @xLibraryA
   # Extract and parse the comment blocks
   local tParsedA
   switch pType
      case "modular"
         put revDocsParseDocText(pText, pFile) into tParsedA
         break
      case "script"
         local tBlocksA
         put revDocsParseScriptDocs(pText) into tBlocksA
         put revDocsParseDocBlocks(tBlocksA, pFIle) into tParsedA
         break
   end switch
   
   
   # Add to the library array
   repeat for each key tEntry in tParsedA
      put tParsedA[tEntry] into xLibraryA[xCount]
      put tEntry into xLibraryA[xCount]["name"]
      add 1 to xCount
   end repeat
   return empty
end revDocsParseFileContentsToArray

function revDocsGenerateDocsFilesFromModularDirectory pDir  
   local tFiles
   put files(pDir) into tFiles
   filter tFiles with regex pattern ".*(\.(mlc|lcb))+"
   
   local tDocsA, tCount
   
   put 1 into tCount
   repeat for each line tFile in tFiles
      get revDocsGenerateDocsFileFromModularFile(tFile)
      if it is not empty then
         put it into tDocsA[tCount]
         add 1 to tCount
      end if
   end repeat
   
   return tDocsA
end revDocsGenerateDocsFilesFromModularDirectory

/*
Summary: Extracts the inline docs from a .lcb file

pFile: The path to the .lcb file to extract docs from

Returns (string): A string consisting of all the docs for the syntax and handlers present in the .lcb file

Description:
<revDocsGenerateDocsFileFromModularFile> is used when packaging a widget to create its API documentation.
It generates the Library and Type elements from the declaration in the <pFile> (either widget or library), and extracts
the comment block that precedes any initial declaration for use as the library-level Description element.
It then extracts the comment blocks that precede syntax and handler definitions in <pFile>, and generates the 
Name, Type, Syntax, and Associated elements for each entry, as well as the parameter types.

Tags: Package building
*/

function revDocsGenerateDocsFileFromModularFile pFile
   local tContents
   put revDocsUtf8FileContents(pFile) into tContents
   return revDocsGenerateDocsFileFromModular(tContents, pFile)
end revDocsGenerateDocsFileFromModularFile

/*
Convert inline docs to the standard format (that can be parsed without code blocks)
*/

function revDocsGenerateDocsFileFromModular pText, pFilename
   return revDocsGenerateDocsFileFromText(pText, pFilename)
end revDocsGenerateDocsFileFromModular

function revDocsGenerateDocsFileFromText pText, pSource
   local tEntryType
   put empty into tEntryType
   
   local tInComment, tInEntry, tEntryEnded
   put false into tInComment
   put false into tInEntry
   
   local tFirstWord, tEntryData
   local tComment, tData
   local tHandlerData, tPhraseData
   
   local tLibraryData, tAPIData
   revDocsExtractDocBlocks pText, pSource, tAPIData, tLibraryData
   
   # Allow inline module description as valid docs
   if tAPIData is empty then
      logWarning "No handlers documented", pSource
   end if
   
   if tLibraryData["name"] is empty then
      logError "No extension name found", pSource
   end if
   
   local tOutput
   put revDocsFormatInlineComments(tLibraryData, true) into tOutput
   
   if tOutput is empty then
      logWarning "Invalid library-level docs", pSource
   end if
   
   repeat for each key tKey in tAPIData
      local tFormatted
      put revDocsFormatInlineComments(tAPIData[tKey], false) into tFormatted
      if tFormatted is empty then
         logWarning "No docs for element" && tKey, pSource
      else
         put tFormatted & return & return after tOutput
      end if
   end repeat
   return tOutput
end revDocsGenerateDocsFileFromText

##################################################
#
#          PARSING UTILITIES
#
##################################################

/*
Extracts the comment blocks that live in between functions which contain the documentation for those functions 

pText: The script to extract comment blocks from
pSource : The source of the script (either a filename or an object id)
Returns (array) : An array containing the details for the comment blocks associated with each api entry

*/
on revDocsExtractDocBlocks pText, pSource, @rAPIData, @rLibraryData
   local tEntryType
   put empty into tEntryType
   
   local tInComment, tInEntry, tEntryEnded
   put false into tInComment
   put false into tInEntry
   
   local tFirstWord, tEntryData, tTrimLine
   local tComment, tCommentOffset, tData
   local tHandlerData, tPhraseData
   
   local tLibraryData, tAPIData
   
   # Keep track of if this is a widget module or not
   local tIsWidget
   put false into tIsWidget
   
   # If the source is an object, then we are parsing script
   local tIsScript
   put there is a pSource into tIsScript
   
   local tIsLCIDL = false
   
   repeat for each line tLine in pText
      # Check to see if this is the start of a handler / syntax or a block of comments 
      put word 1 of tLine into tFirstWord
      
      # By default, the entry doesn't end on this line
      put false into tEntryEnded
      
      # If we are neither in a comment block or in handler / syntax then 
      if not tInComment and not tInEntry then
         # In almost all cases, we will be in a new entry
         put true into tInEntry
         
         switch tFirstWord
            case "external"
               put true into tIsLCIDL
               put "library" into tEntryData["type"]
               put word 2 to -1 of tLine into tEntryData["name"]
               put false into tInEntry
               put true into tEntryData["need_docs"]
               break
            case "module"
            case "library"
               put tFirstWord into tEntryData["type"]
               put word 2 to -1 of tLine into tEntryData["name"]
               put true into tEntryEnded
               put true into tEntryData["need_docs"]
               break
            case "widget"
               put true into tIsWidget
               put "widget" into tEntryData["type"]
               put word 2 to -1 of tLine into tEntryData["name"]
               put true into tEntryEnded
               put true into tEntryData["need_docs"]
               break
            case "metadata"
               # Handle library metadata as a special case
               local tSecondWord
               put word 2 of tLine into tSecondWord
               if tSecondWord is among the items of "author,os,platforms,version" then
                  put tSecondWord & ":" && value(word 4 of tLine) into tLine
                  put Offset(lf & "Name:", tLibraryData["comments"]) into tCommentOffset
                  if tCommentOffset is 0 then 
                     put tLine & lf after tLibraryData["comments"]
                  else
                     # insert metadata above property definitions
                     put lf & tline & lf before char tCommentOffset of tLibraryData["comments"]
                  end if
               end if
               # Metadata is not treated as an entry
               put false into tInEntry
               break
            case "handler"
               put "handler" into tEntryData["type"]
               # Handler is private by default, so don't document
               put true into tEntryData["private"]
               break
            case "private"
               if word 2 of tLine is "handler" then
                  # Don't document private handler
                  put true into tEntryData["private"]
                  # eat the word 'private'
                  put word 2 to -1 of tLine into tLine
               else
                  put false into tInEntry
               end if
               break
            case "public"
               if word 2 of tLine is "handler" then
                  put "handler" into tEntryData["type"]
                  # eat the word 'public'
                  put word 2 to -1 of tLine into tLine
                  put token 2 of tLine into tEntryData["name"]
                  
                  # If we have a widget, then public handlers are not in the message path and so don't need docs.
                  put not tIsWidget into tEntryData["need_docs"]
               else 
                  put true into tEntryData["private"]
                  if word 2 of tLine is "foreign" then
                     if word 3 of tLine is "handler" then
                        put "handler" into tEntryData["type"]
                     else if word 3 of tLine is "type" then
                        put "type" into tEntryData["type"]
                     end if
                     # eat the words 'public foreign'
                     put word 3 to -1 of tLine into tLine
                     put true into tEntryEnded
                  end if
               end if
               break
            case "syntax"
               put word 2 of tLine into tEntryData["name"]
               if word 4 of tLine is "phrase" then
                  put "phrase" into tEntryData["type"]
               else
                  put "syntax" into tEntryData["type"]
                  put true into tEntryData["need_docs"]
               end if
               break
            case "property"
               put word 2 of tLine into tEntryData["name"]
               put "property" into tEntryData["type"]
               # Properties are one-liners
               put true into tEntryEnded
               put true into tEntryData["need_docs"]
               break
            case "on"
               if tIsLCIDL then
                  put false into tInEntry
                  break
               end if
               put "command" into tEntryData["type"]
               put word 2 of tLine into tEntryData["name"]
               put false into tEntryData["need_docs"]
               break
            case "tail"
               if not tIsLCIDL then
                  put false into tInEntry
                  break
               end if
               -- eat tail in lcidl
               put word 2 of tLine into tFirstWord
               put word 2 to -1 of tLine into tLine
            case "command"
            case "function"
               if tFirstWord is "function" then
                  put "function" into tEntryData["type"]
               else
                  put "command" into tEntryData["type"]
               end if
               put word 2 of tLine into tEntryData["name"]
               put true into tEntryData["need_docs"]
               break
            default
               # This is not a new entry
               put false into tInEntry
               # Check if this is a new comment block
               if tFirstWord begins with "/**" and \
                  not (tFirstWord begins with "/**/") then
                  
                  # If this is script, and this is an initial block of comments,
                  # then treat as the library description
                  if tIsScript and tAPIData is empty and tComment is not empty then
                     put "library" into tLibraryData["type"]
                     put tComment into tLibraryData["comments"]
                     put the short name of pSource into tLibraryData["name"]
                  end if
                  
                  if tIsLCIDL and tAPIData is empty and tComment is not empty then
                     put tEntryData into tLibraryData
                     put tComment into tLibraryData["comments"]
                  end if
                  
                  # Discard any previous comments
                  put empty into tComment
                  put true into tInComment
                  put offset("/**", tLine) into tCommentOffset
                  put char (tCommentOffset + 3) to -1 of tLine into tLine
                  
                  # If there's nothing else on this line, don't add it.
                  if word 1 of tLine is empty then next repeat
               end if
               break
         end switch
      end if
      
      # If we're in comments check to see if this is the end of a block of comments
      if tInComment then
         if word -1 of tLine ends with "*/" then
            put false into tInComment
            put offset("*/", tLine) into tCommentOffset
            delete char tCommentOffset to -1 of tLine
            
            # Remove another star if this is a double-star comment closer
            if char -1 of tLine is "*" then
               delete char -1 of tLine
            end if
            
            if tIsLCIDL then
               put false into tInEntry
            end if
            
            # If there's nothing else on this line, don't add it.
            if word 1 of tLine is empty then
               next repeat
            end if
         else if tComment is empty and tFirstWord is empty then
            next repeat
         end if
         put tLine & return after tComment
         next repeat
      end if
      
      # If we are in an entry, add the line to the current data
      if tInEntry then
         # handle line continuations
         if char -2 of tData is "\" then
            delete char -2 to -1 of tData
         # ignore empty lines
         else if tFirstWord is empty then
            if tIsLCIDL then
               put true into tEntryEnded
            else
               next repeat
            end if
         else if tFirstWord is "end" then
            put true into tEntryEnded
         end if
         # continuation character can have spaces afterwards [[Bug 19927]]
         # trim trailing spaces before adding
         if matchText(tLine, "^(.*\\)\s*$", tTrimLine) then
            put tTrimLine & return after tData
         else
            put tLine & return after tData
         end if
      end if
      
      # If the entry is ended, convert all the collected data to a structured array
      if tEntryEnded then
         # Extract data from the syntax / handler if comments are not empty
         if tComment is not empty then
            switch tEntryData["type"]
               case "library"
               case "widget"
               case "module"
                  put tComment into tEntryData["comments"]
                  put tEntryData into tLibraryData
                  break
               case "handler"
                  revDocsParseHandler line 1 of tData, tHandlerData
                  revDocsUpdateDocBlocks true, "handler", line 1 of tData, tComment, tAPIData
                  break
               case "function"
               case "command"
                  if tIsLCIDL then
                     revDocsUpdateDocBlocksForLCIDL  tEntryData["name"], tEntryData["type"], tData, tComment, tAPIData
                  else
                     revDocsUpdateDocBlocks false, tEntryData["type"], line 1 of tData, tComment, tAPIData
                  end if
                  break
               case "type"
                  break
               case "phrase"
                  put tData into tPhraseData[word 2 of tData]
                  break
               case "syntax"
                  revDocsUpdateModularSyntaxDocBlocks tData, tComment, tHandlerData, tPhraseData, tAPIData
                  break
               case "property"
                  revDocsUpdateDocBlocksForProperty tData, tComment, tAPIData
                  break
            end switch
            
         else
            if tEntryData["need_docs"] then
               logError "Required documentation not found for" && tEntryData["type"] && tEntryData["name"], pSource
            end if
         end if
         put empty into tEntryData
         put empty into tComment
         put empty into tData
         put false into tInEntry
      end if
   end repeat
   
   put tAPIData into rAPIData
   put tLibraryData into rLibraryData
end revDocsExtractDocBlocks 

private command log pOutput
   write pOutput & return to stdout
end log

private command logError pError, pFilename
   log pFilename & ":" && "error:" && pError
end logError

private command logWarning pWarning, pFilename
   log pFilename & ":" && "warning:" && pWarning
end logWarning

private function extensionsFromType pType
   switch pType
      case "module"
         return ".*\.(lcb|lcdoc)"
      case "script"
         return ".*\.(livecode|lcdoc)"
      case "dictionary"
         return ".*\.lcdoc"
   end switch
end extensionsFromType

private function __revDocsParseDirectoryToLibraryArray pType, pRootDir, pRecursive
   if there is not a folder pRootDir then
      return empty
   end if
   
   local tFiles, tExtensionRegex
   put extensionsFromType(pType) into tExtensionRegex
   put files(pRootDir) into tFiles
   filter tFiles with regex pattern tExtensionRegex
   
   local tCount
   put 1 into tCount
   
   local tLibraryA
   if tFiles is not empty then
      repeat for each line tFile in tFiles
         revDocsParseFileToArray(tFile, pType, tCount, tLibraryA)
      end repeat
   end if
   
   if pRecursive then
      local tSubDirA
      repeat for each line tLine in the folders
         if tLine is ".." then
            next repeat
         end if
         put __revDocsParseDirectoryToLibraryArray(pType, pRootDir & slash & tLine, pRecursive) into tSubDirA
         repeat for each key tEntry in tSubDirA
            put tSubDirA[tEntry] into tLibraryA[tCount]
            add 1 to tCount
         end repeat
      end repeat
   end if
   
   return tLibraryA
end __revDocsParseDirectoryToLibraryArray

function revDocsFormatInlineComments pDataA, pIsLibrary
   local tComment
   put pDataA["comments"] into tComment
   
   local tElementsA
   put revDocsExtractElements(tComment) into tElementsA
   
   local tEntriesA
   put revDocsGroupElements(tElementsA) into tEntriesA
   
   # If tEntries is empty, there was an error (docs empty, or no Name element)
   if tEntriesA is empty then
      return empty
   end if
   
   local tEntryA, tElementA, tElement
   
   local tHasA
   
   local tParamsFound
   put false into tParamsFound
   
   local tDocData
   
   local tStart, tEnd
   put 1 into tStart
   put the number of elements in tEntriesA into tEnd
   if tEntriesA[0] is not empty then 
      subtract 1 from tStart
      subtract 1 from tEnd
   end if
   
   local tName, tType, tSyntax, tEntryDoc
   repeat with x = tStart to tEnd
      put empty into tHasA
      put empty into tEntryDoc
      put empty into tSyntax
      put tEntriesA[x] into tEntryA
      repeat with y = 1 to the number of elements in tEntryA["elements"]
         put tEntryA["elements"][y] into tElementA
         
         put tElementA["name"] into tElement
         
         put true into tHasA[tElement]
         
         if tElement is "Name" then
            put tElementA["content"] into tName
            next repeat
         else if tElement is "Type" then
            put tElementA["content"] into tType
            next repeat
         else if tElement is "Syntax" then
            put tElementA["content"] into tSyntax[the number of elements of tSyntax + 1]
            next repeat
         end if
         
         if tElement is "References" then
            repeat for each element tPhrase in pDataA["phrases"]
               put comma & tPhrase & "(phrase)" after tElementA["content"]
            end repeat
         end if
         if tElement is "output" then
            put "Returns" into tElementA["name"]
         end if
         if tElementA["name"] is "Returns" then
            if tElementA["type"] is empty then
               get pDataA["variants"][1]["return value"]["type"]
               if it is not empty then
                  put it into tElementA["type"]
               end if
            end if
         else if tElement is "The result" then
            if tElementA["type"] is empty then
               get pDataA["variants"][1]["the result"]["type"]
               if it is not empty then
                  put it into tElementA["type"]
               end if
            end if
         else
            repeat for each element tVariant in pDataA["variants"]
               repeat for each element tParam in tVariant
                  if tParam["name"] is tElement then
                     if not tParamsFound then
                        put "Parameters:" & return after tEntryDoc
                        put true into tParamsFound
                     end if
                     if tElementA["type"] is empty then
                        # add the param details if they were not included
                        if pDataA["phrases"][tElement] is not empty then
                           put "<" & word 1 to -1 of pDataA["phrases"][tElement] & ">" into tElementA["type"]
                        else if tParam["type"] is not empty then
                           put tParam["type"] into tElementA["type"]
                        end if
                     end if
                     if tParam["mode"] is not "in" then
                        put tParam["mode"] & " " before tElementA["type"]
                     end if
                  end if
               end repeat
            end repeat
         end if
         
         # Now output back to the doc data
         put tElementA["name"] into tElement
         
         if tElementA["type"] is not empty then
            put tElement & "(" & tElementA["type"] & "):" && tElementA["content"] & return after tEntryDoc
         else
            if the number of lines in tElementA["content"] > 1 then              
               local tDeleteTab
               put false into tDeleteTab
               put tElement & ":" & return after tEntryDoc
               repeat for each line tLine in tElementA["content"]
                  if line 1 of tElementA["content"] begins with tab then
                     put true into tDeleteTab
                  end if
                  if tDeleteTab and tLine begins with tab then
                     delete char 1 of tLine
                  end if
                  put tLine & return after tEntryDoc
               end repeat
            else
               put tElement & ":" && tElementA["content"] & return after tEntryDoc
            end if
         end if
         put return after tEntryDoc
      end repeat
      if tHasA["Syntax"] is not true then
         put pDataA["syntax"] into tSyntax
      end if
      repeat with z = 1 to the number of elements in tSyntax
         put "Syntax:" && tSyntax[z] & return & return before tEntryDoc
      end repeat
      
      if tHasA["Type"] is not true then
         put pDataA["type"] into tType
      end if
      put "Type:" && tType & return & return before tEntryDoc
      
      if tHasA["Name"] is not true then
         put pDataA["name"] into tName
      end if
      if pIsLibrary and x = 0 then
         put "Library:" && tName & return & return before tEntryDoc
      else
         put "Name:" && tName & return & return before tEntryDoc
      end if
      
      if tHasA["Parameters"] is not true and pDataA["params"] is an array then
         put "Parameters:" & return after tEntryDoc
         repeat with tParam = 1 to the number of elements of pDataA["params"]
            put pDataA["params"][tParam]["name"] after tEntryDoc
            if pDataA["params"][tParam]["type"] is not empty then
               put " (" & pDataA["params"][tParam]["type"] & ")" after tEntryDoc
            end if
            put ":" & return &  pDataA["params"][tParam]["description"] & return & return after tEntryDoc
         end repeat
      end if
      
      repeat for each item tKey in "The result,Returns,it"
         if tHasA[tKey] is not true and pDataA[tKey] is an array then
            put tKey after tEntryDoc
            if pDataA[tKey]["type"] is not empty then
               put " (" & pDataA[tKey]["type"] & ")" after tEntryDoc
            end if
            put ":" & return &  pDataA[tKey]["description"] & return & return after tEntryDoc
         end if
      end repeat
      
      if tHasA["References"] is not true then
         if pDataA["phrases"] is not empty then
            put "References: " after tEntryDoc
            repeat for each element tElement in pDataA["phrases"]
               put tElement & "(phrase)" & comma after tEntryDoc
            end repeat
            delete the last char of tEntryDoc
            put return & return after tEntryDoc
         end if
      end if
      put tEntryDoc after tDocData
   end repeat
   
   return tDocData
end revDocsFormatInlineComments

function revDocsCollectGlossarySynonyms pRoot
   local tGlossaryA, tText, tName, tSynonyms
   local tFolders
   put folders(pRoot & slash & "glossary") into tFolders
   repeat for each line tLine in tFolders
      if tLine is ".." then next repeat
      get files(pRoot & slash & "glossary" & slash & tLine)
      filter it with "*.lcdoc"
      if it is not empty then
         repeat for each line tFile in it
            if tFile begins with "." then next repeat
            local tFullPath
            put pRoot & slash & "glossary" & slash & \ 
                  tLine & slash & tFile into tFullPath
            put revDocsUtf8FileContents(tFullPath) into tText
            get lineOffset("Name:", tText)
            put word 2 to -1 of line it of tText into tName
            if tName is empty then
               next repeat
            end if
            get lineOffset("Synonyms:", tText)
            put word 2 to -1 of line it of tText into tSynonyms
            if tSynonyms is empty then
               next repeat
            end if
            
            repeat for each item tItem in tSynonyms
               put tName into tGlossaryA[tItem]
            end repeat
            
         end repeat
      end if
   end repeat
   return tGlossaryA
end revDocsCollectGlossarySynonyms

private function __elementRegex
   # Regex used to determine if this line starts a new element
   # Matched substrings are element name, (optional) param type, same line element content
   return "^ *([tT]he [rR]esult|\w+) *(?:\(( *(?:optional |in |out |inout )?<?\w*>?) *\))? *: *(.*)"
end __elementRegex

private function __arrayElementRegex
   # Regex used to extract sub-array elements
   # Essentially the same as the element regex except quotation marks are allowed
   # as keys can obviously be string literals.
   return "^ *([\w" & quote & "]+) *(?:\(( *(?:optional |in |out |inout )?<?\w*>?) *\))? *: *(.*)"
end __arrayElementRegex

constant kAlwaysRecognizeAsElement = "Description"

function revDocsExtractElementsWithRegex pText, pRegex
   local tElementRegex
   put pRegex into tElementRegex
   
   # Array to store the elements
   local tElementsA
   local tElementCount
   put 0 into tElementCount
   
   # Store the content that belongs to this element
   local tElementA, tAccumulatedContent
   
   # Flush the previous accumulation of content when we see a new element
   local tFlushAccumulated
   put false into tFlushAccumulated
   
   # Variables to aid extraction of array parameter description
   local tInArrayDescription
   put false into tInArrayDescription
   local tArrayDepth
   put 0 into tArrayDepth
   
   local tInternalBlankLines
   repeat for each line tLine in pText
      # Variables to store regex matches
      local tNextElementName, tParamType, tAfterElement
      # Variable to keep track of whether element contains multiple lines
      local tInMultipleLineElement
      
      get word 1 of tLine
      if tInArrayDescription then
         # Keep track of array depth
         if it begins with "{" then
            add 1 to tArrayDepth
            put tLine & return after tAccumulatedContent
            next repeat
         end if
         if word -1 of tLine ends with "}" then
            subtract 1 from tArrayDepth
            put tLine & return after tAccumulatedContent
            if tArrayDepth <= 0 then
               put false into tInArrayDescription
            end if
            next repeat
         end if
      end if
      if tArrayDepth > 0 then
         put tLine & return after tAccumulatedContent
         next repeat
      end if
      
      # If this matches the element definition regex, flush the accumulated content to the elements array
      # otherwise just append to the accumulated content
      
      if word 1 of tLine is empty then
         # AL-2015-09-03: [[ Bug 15866 ]] If we have an empty line and there is already content for this element,
         #  then the blank line is intentional whitespace, so keep track of the number
         if tAccumulatedContent is not empty then
            add 1 to tInternalBlankLines
         end if
         next repeat
      else if matchText(tLine, tElementRegex, tNextElementName, tParamType, tAfterElement) then
         --BWM-2017-11-22: [[ Bug 19543 ]] A word (optionaly with something that looks like 
         --a ParamType) immediately following content should be treated as part of that
         --content instead of a new element.
         if word 1 of tAfterElement is empty and tAccumulatedContent is not empty and \
               tInternalBlankLines is 0 and tInMultipleLineElement and \
               tNextElementName is not among the items of kAlwaysRecognizeAsElement then
            put tLine & return after tAccumulatedContent
            next repeat
         end if
         if tAccumulatedContent is not empty and tElementCount is 0 then
            add 1 to tElementCount
         end if
         if tElementCount is not 0 then
            put tElementA into tElementsA[tElementCount]
            if tElementA["name"] is empty then
               put "Summary" into tElementsA[tElementCount]["name"]
            end if
            delete the last char of tAccumulatedContent
            put tAccumulatedContent into tElementsA[tElementCount]["content"]
         end if
         add 1 to tElementCount
         
         put empty into tAccumulatedContent
         put 0 into tInternalBlankLines
         # Prevent propagation of element details
         put empty into tElementA
         put tNextElementName into tElementA["name"]
         
         if tParamType is not empty then
            put tParamType into tElementA["type"]
            if tParamType is "array" then
               put true into tInArrayDescription
            end if
         end if
         put empty into tParamType
         
         if word 1 of tAfterElement is not empty then
            put word 1 to -1 of tAfterElement & return into tAccumulatedContent
         end if
         put false into tInMultipleLineElement
      else
         # AL-2015-09-03: [[ Bug 15866 ]] Include internal blank lines in content
         repeat tInternalBlankLines
            put return after tAccumulatedContent
         end repeat
         put 0 into tInternalBlankLines
         put tLine & return after tAccumulatedContent
         put true into tInMultipleLineElement
      end if
   end repeat
   
   # Add any remaining text to the last element
   if tAccumulatedContent is not empty and tElementCount is 0 then
      add 1 to tElementCount
   end if
   if tElementCount is not 0 then
      put tElementA into tElementsA[tElementCount]
      if tElementA["name"] is empty then
         put "Summary" into tElementsA[tElementCount]["name"]
      end if
      put tAccumulatedContent into tElementsA[tElementCount]["content"]
   end if
   
   return tElementsA
end revDocsExtractElementsWithRegex

/*
Utility function to extract any elements matching the docs element style
*/
function revDocsExtractElements pText
   # Use the element-matching regex
   return revDocsExtractElementsWithRegex(pText, __elementRegex())
end revDocsExtractElements

private command __nameToKeyAndType pName, @rKey, @rType
   put pName into rKey
   switch pName
      case "Summary"
      case "Type"
      case "Description"
      case "Changes"
      case "Introduced"
      case "Deprecated"
      case "Edition"
      case "Version"
      case "Author"
      case "SVGIcon"
         put "standard" into rType
         break
      case "Example"
      case "Syntax"
         put "multiple" into rType
         break
      case "Tags"
      case "OS"
      case "Platforms"
      case "Security"
      case "Synonyms"
      case "Associations"
      case "Requires"
         put "items" into rType
         break
      case "The result"
      case "It"
      case "Return"
         put "return" into rType
         break
      case "Returns"
         put "return" into rType
         put "return" into rKey
         break
      case "Related"
         put "references" into rKey
         put "references" into rType
         break
      case "References"
      case "Uses"
         put "references" into rType
         break
      case "Tag"
         put "tags" into rKey
         put "items" into rType
         break
      case "Name"
      case "Title"
         put "display name" into rKey
         put "standard" into rType
         break
      case "Library"
         put "name" into rKey
         put "standard" into rType
         break
      case "Parameters"
         put "parameters" into rType
         break
      default
         # "Value" is a common parameter name, so special case it 
      case "Value"
         put pName into rKey
         put "param" into rType
         break
   end switch
end __nameToKeyAndType

function revDocsParseElements pExtractedA, pBlocksA, pFilename
   local tParsedA
   
   local tParams
   put pBlocksA["params"] into tParams
   
   # Keep track of the most recent standard element key, in case we have falsely identified element.
   local tLastKey
   local tExtraA
   put "description" into tLastKey
   
   # If we have an unknown docs element which is not under a Parameters: element, then
   # append it to the previous element. 
   local tDoingParams
   put false into tDoingParams
   
   repeat with tElementNum = 1 to the number of elements of pExtractedA
      local tElementA
      put pExtractedA[tElementNum] into tElementA
      
      local tKey, tType
      __nameToKeyAndType tElementA["name"], tKey, tType
      
      if tType is not "param" then
         # If any of these elements are already parsed and we are 'doing params'
         # then remaining ones should be treated as param elements
         if tType is "standard" and tParsedA[tKey] is not empty and tDoingParams then
            put "param" into tType
         else
            put false into tDoingParams
         end if
      end if
      
      switch tType
         case "standard"
            # Put standard elements into the array with no further processing
            put tElementA["content"] into tParsedA[tKey]
            break
         case "multiple"
            # There might be more than one occurrence of these elements
            get the number of elements of tParsedA[tKey]
            put tElementA["content"] into tParsedA[tKey][it + 1]
            break
         case "items"
            # These are all single-line, comma-delimited
            local tCount
            put 1 into tCount
            repeat for each item tItem in tElementA["content"]
               put tolower(word 1 to -1 of tItem) into tParsedA[tKey][tCount]
               add 1 to tCount
            end repeat
            break
         case "references"
            # References require a bit of custom treatment
            put revDocsParseReferences(tElementA["content"]) into tParsedA[tKey]
            break
         case "param"
            local tRefParam, tFormattedA
            put false into tRefParam
            # Check to see if this is a param and add warning to error state
            if tParams[("@" & tKey)] then
               put true into tRefParam
            else if tParams[tKey] is not true then
               if tKey is not "Value" then
                  # If there was no explicit 'Parameters' declaration, and there is no match, append to previous
                  # Otherwise, warn and output anyway
                  if tDoingParams then
                     logWarning tKey && "in" && pExtractedA[1]["content"] && "is not an element type, and does not match a param name.", pFilename
                  else
                     # 'unparse' the element
                     put return & tKey & ":" & tElementA["content"] after tExtraA[tLastKey]
                     break
                  end if
               end if
            end if
            
            # Fall through if this is a property and there is a 'Value' element
            if tKey is not "Value" or tParsedA["type"] is not "property" then
               get the number of elements of tParsedA["params"] + 1
               put revDocsParseDescriptionForType(tElementA["type"], tElementA["content"]) into tParsedA["params"][it]
               put tRefParam into tParsedA["params"][it]["refparam"]
               put tKey into tParsedA["params"][it]["name"]
               break
            end if
         case "return"
            put revDocsParseDescriptionForType(tElementA["type"], tElementA["content"]) into tParsedA["value"][tKey]
            break
         case "parameters"
            put true into tDoingParams
            break
         default
            logWarning "Invalid type returned from nameToKeyAndType", pFilename
            break
      end switch
      if tType is "standard" then
         put tKey into tLastKey
      end if
   end repeat
   
   repeat for each key tKey in tExtraA
      put tExtraA[tKey] after tParsedA[tKey]
   end repeat
   
   # Transfer all docs information calculated from code
   repeat for each item tItem in "type,syntax,variants,display name"
      if tParsedA[tItem] is empty then
         put pBlocksA[tItem] into tParsedA[tItem]
      end if
   end repeat
   
   if tParsedA["display name"] is empty then
      put tParsedA["name"] into tParsedA["display name"]
   end if
   if tParsedA["description"] is empty then
      put tParsedA["summary"] into tParsedA["description"]
   end if
   
   return tParsedA
end revDocsParseElements

private command __updateParamsFromSyntaxLine pLine, @xParamsA
   local tParamStart, tParamEnd
   repeat while matchChunk(pLine, "< *(\w+) *>", tParamStart, tParamEnd)
      put true into xParamsA[char tParamStart to tParamEnd of pLine]
      put char tParamEnd to -1 of pLine into pLine
   end repeat
end __updateParamsFromSyntaxLine

function revDocsFormatDocFileAsJSON pFile, pLibraryName, pAuthor
   local tFileContents
   put revDocsUtf8FileContents(pFile) into tFileContents
   return revDocsFormatDocTextAsJSON(pFile, tFileContents, pLibraryName, pAuthor)
end revDocsFormatDocFileAsJSON

function revDocsFormatDocTextAsJSON pFile, pText, pLibraryName, pAuthor
   if pText is empty then
      return empty
   end if
   
   # Parse the doc text into a library array
   local tLIbraryA
   put revDocsParseDocTextToLibraryArray(pFile, pText, pLibraryName, pAuthor) into tLibraryA
   return revDocsFormatLibraryArrayAsJSON(tLibraryA)
end revDocsFormatDocTextAsJSON

function revDocsParseDocFileToLibraryArray pFile, pLibraryName, pAuthor
   local tFileContents
   put revDocsUtf8FileContents(pFile) into tFileContents
   return revDocsParseDocTextToLibraryArray(pFile, tFileContents, pLibraryName, pAuthor)
end revDocsParseDocFileToLibraryArray

function revDocsParseDocTextToLibraryArray pFile, pText, pLibraryName, pAuthor
   local tOverridesA
   if pLibraryName is not empty then
      put pLibraryName into tOverridesA["display name"]
   end if
   if pAuthor is not empty then
      put pAuthor into tOverridesA["author"]
   end if 
   return revDocsParseDocText(pText, pFile, "", tOverridesA)
end revDocsParseDocTextToLibraryArray

function revDocsParseDocFile pFile
   local tFileContents
   put revDocsUtf8FileContents(pFile) into tFileContents
   return revDocsParseDocText(tFileContents, pFile)
end revDocsParseDocFile

function revDocsGroupElements pElementsA
   local tLibraryName
   
   local tElementsA
   put pElementsA into tElementsA
   local tEntriesA, tEntryCount
   put 0 into tEntryCount
   
   local tEntryElementCount
   put 1 into tEntryElementCount
   
   local tParsedA
   # Get library-level data
   local tStartNum
   put 1 into tStartNum
   repeat forever
      get tElementsA[tStartNum]["name"]
      switch it
         case "Name"
         case ""
            exit repeat
         default
            put tElementsA[tStartNum]["content"] into tEntriesA[tEntryCount]["elements"][tEntryElementCount]["content"]
            put it into tEntriesA[tEntryCount]["elements"][tEntryElementCount]["name"]
            add 1 to tStartNum
            add 1 to tEntryElementCount
            break
      end switch
   end repeat
   
   put 0 into tEntryElementCount
   repeat with tElementNum = tStartNum to the number of elements in tElementsA
      add 1 to tEntryElementCount
      local tElementA
      put tElementsA[tElementNum] into tElementA
      
      if tElementA["name"] is "Name" then
         add 1 to tEntryCount
         put 1 into tEntryElementCount
      end if
      
      put tElementA into tEntriesA[tEntryCount]["elements"][tEntryElementCount]
   end repeat
   return tEntriesA
   
end revDocsGroupElements

-- Elements that when defined at the library level
-- apply to all docs in the api
constant kInheritedElements = "author,version,associations,tags,requires,uses,os,platforms"
-- Elements that should be in the library's actual docs entry
constant kLibraryEntryElements = "name,display name,summary,description,type,example,references,edition,os,platforms,associations"
function revDocsParseDocText pText, pFilename, pDefaults, pOverrides
   local tElementsA
   put revDocsExtractElements(pText) into tElementsA
   
   local tLibraryName
   local tEntriesA, tEntryCount
   put 0 into tEntryCount
   
   local tParsedA
   put pDefaults into tParsedA
   
   # Get library-level data
   local tStartNum
   put 1 into tStartNum
   repeat forever
      get tElementsA[tStartNum]["name"]
      switch it
         case "Name"
         case empty
            exit repeat
         case "Library"
            put tElementsA[tStartNum]["content"] into tParsedA["library"]
            add 1 to tStartNum
            break
         case "Title"
         case "Summary"
         case "Description"
         case "Author"
         case "Type"
         case "Version"
         case "Requires"
         case "Uses"
         case "SVGIcon"
         case "Summary"
         case "OS"
         case "Platforms"
            // Add top-level tags to all library entries
         case "Tags"
            // Library entries can have examples
         case "Example"
         case "References"
         case "Edition"
            put tElementsA[tStartNum]["content"] into tParsedA[it]
            add 1 to tStartNum
            break
         default
            logError "Error:" && it && "is invalid library-level element", pFilename
            exit repeat
      end switch
   end repeat
   
   local tEntryElementCount
   put 0 into tEntryElementCount
   
   local tSeen
   
   put tParsedA["library"] into tParsedA["Associations"]
   repeat with tElementNum = tStartNum to the number of elements in tElementsA
      add 1 to tEntryElementCount
      local tElementA
      put tElementsA[tElementNum] into tElementA
      
      if tElementA["name"] is "Name" then
         repeat for each key tKey in tParsedA
            if not tSeen[tKey] and \
                  tParsedA[tKey] is not empty and \
                  tKey is among the items of kInheritedElements and \
                  tEntryCount is not 0 then
               put tKey into tEntriesA[tEntryCount]["elements"][tEntryElementCount]["name"]
               put tParsedA[tKey] into tEntriesA[tEntryCount]["elements"][tEntryElementCount]["content"]
               add 1 to tEntryElementCount
            end if
         end repeat
         
         add 1 to tEntryCount
         put 1 into tEntryElementCount
         delete variable tSeen 
      end if
      
      put true into tSeen[tElementA["name"]]
      
      if tElementA["name"] is "Syntax" then
         __updateParamsFromSyntaxLine tElementA["content"], tEntriesA[tEntryCount]["params"]
      end if
      
      if tElementA["name"] is "Associations" then
         if tParsedA["library"] is not empty and tParsedA["library"] is not among the items of tElementA["content"] then
            put tParsedA["library"] & comma before tElementA["content"]
         end if
      end if
      
      if tElementA["name"] is "Tags" and tParsedA["tags"] is not empty then
         put tParsedA["tags"] & comma before tElementA["content"]
      end if
      
      put tElementA into tEntriesA[tEntryCount]["elements"][tEntryElementCount]
   end repeat
   
   // Add association and tag to the last element, if they were not found
   repeat for each key tKey in tParsedA
      if not tSeen[tKey] and \
            tParsedA[tKey] is not empty and \
            tKey is among the items of kInheritedElements and \
            tEntryCount is not 0 then
         add 1 to tEntryElementCount
         put tKey into tEntriesA[tEntryCount]["elements"][tEntryElementCount]["name"]
         put tParsedA[tKey] into tEntriesA[tEntryCount]["elements"][tEntryElementCount]["content"]
      end if
   end repeat
   
   // Add a library entry, if there was one
   local tHasLibraryEntry, tLibraryA
   put tParsedA["library"] is not empty into tHasLibraryEntry
   if tHasLibraryEntry then
      local tLibraryElementsA
      local tLibCount
      put 1 into tLibCount
      add 1 to tEntryCount
      
      -- Use kLibraryEntryElements to determine what library-level docs actually
      -- appear in the library entry in the dictionary. Everything else is 
      -- essentially metadata
      repeat for each key tKey in tParsedA
         if tParsedA[tKey] is not empty then
            put tKey into tLibraryElementsA[tLibCount]["name"]            
            put tParsedA[tKey] into tLibraryElementsA[tLibCount]["content"]
            add 1 to tLibCount
         end if
      end repeat
      
      put revDocsParseElements(tLibraryElementsA, empty, pFilename) into tLibraryA
   end if
   
   repeat for each key tKey in tEntriesA
      put revDocsParseElements(tEntriesA[tKey]["elements"], tEntriesA[tKey], pFilename) into tLibraryA["doc"][tKey]
   end repeat
   
   if tHasLibraryEntry then  
      repeat for each key tLibElementKey in tLibraryA
         if tLibElementKey is among the items of kLibraryEntryElements then
            put tLibraryA[tLibElementKey] into tLibraryA["doc"][0][tLibElementKey]
         end if
      end repeat 
      
      // Use overrides if specified
      repeat for each key tKey in pOverrides
         put pOverrides[tKey] into tLibraryA["doc"][0][tKey]
         put pOverrides[tKey] into tLibraryA[tKey]
      end repeat
   end if
   return tLibraryA
end revDocsParseDocText

/*
Removes the unnecessary (from the point of view of docs) parts of a lcb syntax declaration
*/
private function formatSyntaxLine pLine, pPhrases
   // Currently have to use &lt; and &gt; for < and >
   local tStart,tEnd
   local tEscapedQuote
   put "\" & quote into tEscapedQuote
   
   local tType, tParam, tPhrases
   repeat while matchChunk(pLine, "( *: *\w* *)>", tStart, tEnd)
      put empty into tType
      put empty into tParam
      if matchText(pLine, "< *(\w*) *: *(\w*) *>", tParam, tType) and tType is not "Expression" and tType is among the keys of pPhrases then
         put replaceText(pLine, "< *" & tParam & " *: *" & tType & " *>", formatSyntaxLine(line 2 of pPhrases[tType], pPhrases)) into pLine
      else
         delete char tStart to tEnd of pLine
      end if
   end repeat
   
   # Replace isolated <
   if matchChunk(pLine, tEscapedQuote & "[^ ]*(<)[^ ]*" & tEscapedQuote, tStart, tEnd) then
      put "&lt;" into char tStart of pLine
   end if
   
   # Replace isolated >
   if matchChunk(pLine, tEscapedQuote & "[^ ]*(>)[^ ]*" & tEscapedQuote, tStart, tEnd) then
      put "&gt;" into char tStart of pLine
   end if
   
   # Replace options 
   # Eg ( "is" <IsNot=false> | "is not" <IsNot=true>) becomes ("is" | "is not")
   put replaceText(pLine, "[\(\|] *< *(\w*)=[^>]*> *[\)\|]", ")") into pLine
   put replaceText(pLine, " *< *(\w*)=[^>]*> *", " ") into pLine
   
   # Remove quotation marks
   replace quote with empty in pLine
   return pLine
end formatSyntaxLine

command revDocsParseHandler pHandler, @xHandlers
   local tHandlerParams, tHandlerResult, tHandlerResult2
   get matchText(pHandler, "\((.*)\)(?: as (\w*)(?: (\w*))?)?", tHandlerParams, tHandlerResult, tHandlerResult2)
   
   local tHandlerName
   put token 2 of pHandler into tHandlerName
   
   local tData, tParamData
   repeat for each item tItem in tHandlerParams
      put empty into tParamData
      put word 2 of tItem into tParamData["name"]
      put word 4 to -1 of tItem into tParamData["type"]
      put word 1 of tItem into tParamData["mode"]
      put tParamData into tData[the number of elements of tData + 1]
   end repeat   
   put tData into xHandlers[tHandlerName]["params"]
   
   if tHandlerResult is not "undefined" then
      if tHandlerResult is "optional" then
         put " " & tHandlerResult2 after tHandlerResult
      end if
      put tHandlerResult into xHandlers[tHandlerName]["the result"]["type"]
   end if
end revDocsParseHandler

on revDocsUpdateDocBlocksForProperty pData, pComment, @xDataA
   local tName
   put token 1 to -1 of word 2 of pData into tName
   put pComment into xDataA[tName]["comments"]
   put "property" into xDataA[tName]["type"]
   put tName into xDataA[tName]["name"]
end revDocsUpdateDocBlocksForProperty

command revDocsUpdateModularSyntaxDocBlocks pData, pComment, pHandlersA, pPhrasesA, @xDataA
   local tNameLine, tSyntaxLine, tBindingLines, tDeprecated
   local tCurrentLine
   put 1 into tCurrentLine
   put line tCurrentLine of pData into tNameLine
   add 1 to tCurrentLine
   if word 1 to -1 of line tCurrentLine of pData begins with "deprecate" then
      put token 1 to -1 of word 4 of line tCurrentLine of pData into tDeprecated
      add 1 to tCurrentLine
   end if
   
   put line tCurrentLine of pData into tSyntaxLine
   repeat while tSyntaxLine ends with "\"
      add 1 to tCurrentLine
      put line tCurrentLine of pData into char -1 of tSyntaxLine
   end repeat
   add 1 to tCurrentLine
   
   repeat with x = tCurrentLine to the number of lines in pData
      get line x of pData
      if it begins with "end" then
         exit repeat
      end if
      put it into tBindingLines[the number of elements of tBindingLines + 1]
   end repeat
   
   local tEntryName
   put word 2 of tNameLine into tEntryName
   
   put formatSyntaxLine(tSyntaxLine, pPhrasesA) into xDataA[tEntryName]["syntax"][1]
   
   local tParam, tParams
   // Parse parameters from syntax line
   local tFirst, tItem
   put true into tFirst
   set the itemdelimiter to "<"
   repeat for each item tItem in xDataA[tEntryName]["syntax"][1]
      if tFirst is not true then
         put token 1 of tItem & comma after tParams
      end if
      put false into tFirst
   end repeat
   delete the last char of tParams
   
   set the itemdelimiter to comma
   
   local tParamTypes, tHandler, tCount
   put 1 into tCount
   // determine the param types from the handler bindings
   repeat for each element tLine in tBindingLines
      put pHandlersA[token 1 of tLine] into tHandler
      if tHandler is empty then 
         next repeat
      end if
      // extract from brackets
      local tBindingParams
      get matchText(tLine, "\((.*)\)", tBindingParams)
      // get types
      local tItemNum, tParamName
      
      repeat with tItemNum = 1 to the number of elements in tHandler["params"]
         put tHandler["params"][tItemNum] into tItem
         put word 1 of item tItemNum of tBindingParams into tParamName
         if tParamName is "output" then
            put tItem["type"] into tParamTypes["return value"]["type"]
         else
            get the number of elements of tParamTypes + 1
            put tParamName into tParamTypes[it]["name"]
            put tItem["type"] into tParamTypes[it]["type"]
            put tItem["mode"] into tParamTypes[it]["mode"]
         end if
      end repeat   
      if tHandler["the result"] is not empty then
         put tHandler["the result"]["type"] into tParamTypes["the result"]["type"]
      end if
      put tParamTypes into xDataA[tEntryName]["variants"][tCount]
      add 1 to tCount
   end repeat
   
   put char 1 to offset(" with ", word 4 to -1 of tNameLine) -1 of word 4 to -1 of tNameLine into xDataA[tEntryName]["type"]
   put word -1 of xDataA[tEntryName]["type"] into xDataA[tEntryName]["type"]
   put "syntax" into xDataA[tEntryName]["kind"]
   put pComment into xDataA[tEntryName]["comments"]
   if tDeprecated is not empty then
      put tDeprecated into xDataA[tEntryName]["deprecated"]
   end if
   put tEntryName into xDataA[tEntryName]["name"]
end revDocsUpdateModularSyntaxDocBlocks

command revDocsUpdateDocBlocks pModular, pType, pLine, pComment, @xDataA
   if pComment is empty then
      exit revDocsUpdateDocBlocks
   end if
   
   get word 1 of pLine
   if it is "private" then
      exit revDocsUpdateDocBlocks
   end if
   
   if it is "public" then
      put word 2 to -1 of pLine into pLine
   end if
   
   local tEntryName, tHandlerParams
   if pModular then
      get matchText(pLine, "(\w*)\((.*)\)", tEntryName, tHandlerParams)
   else
      put word 2 of pLine into tEntryName
      put word 3 to -1 of line 1 of pLine into tHandlerParams
   end if
   
   local tParam, tParams
   # Get handler params w/out spaces
   repeat for each item tParam in tHandlerParams
      if pModular then
         put word 2 of tParam & comma after tParams
      else
         put word 1 of tParam & comma after tParams
      end if
   end repeat
   delete the last char of tParams
   
   local tSyntax
   local tParamArray
   local tParamsSyntax
   repeat for each item tItem in tParams
      put true into tParamArray[tItem]
      if tItem begins with "@" then
         delete char 1 of tItem
      end if
      put "<" & tItem & ">," after tParamsSyntax
   end repeat
   delete the last char of tParamsSyntax
   if pType is "function" or pType is "handler" then
      put "(" before tParamsSyntax
      put ")" after tParamsSyntax
      put tEntryName & tParamsSyntax into tSyntax
   else
      put tEntryName && tParamsSyntax into tSyntax
   end if
   
   put tSyntax into xDataA[tEntryName]["syntax"][1]
   put tParamArray into xDataA[tEntryName]["params"]
   put pType into xDataA[tEntryName]["type"]
   put tEntryName into xDataA[tEntryName]["name"]
   
   # Output the comments that preceded this function definition
   put pComment into xDataA[tEntryName]["comments"]
   # Distinguish from syntax
   put "handler" into xDataA[tEntryName]["kind"]
end revDocsUpdateDocBlocks

command revDocsUpdateDocBlocksForLCIDL pEntryName, pType, pData, pComment, @xDataA
   if pComment is empty then
      exit revDocsUpdateDocBlocksForLCIDL
   end if
   
   local tSyntax
   local tParamArray
   local tParamsSyntax
   local tValue
   
   local tParam
   repeat for each line tLine in line 2 to -1 of pData
      if the number of words of tLine is 0 then
         next repeat
      end if
      
      split tLine by "//"
      
      if word 1 of tLine[1] is not "return" then
         local tOptional
         put word 1 of tLine[1] is "optional" into tOptional
         
         if tOptional then
            delete word 1 of tLine[1]
         end if
         put word 2 of tLine[1] into tParam
         
         if tOptional then
            put "[<" & tParam & ">], " after tParamsSyntax
         else
            put "<" & tParam & ">, " after tParamsSyntax
         end if
         
         get the number of elements of tParamArray + 1
         
         put "out" is in word 1 of tLine[1] into tParamArray[it]["refparam"]
         put tParam into tParamArray[it]["name"]
         
         if word 4 of tLine[1] contains "string" then
            put "string" into tParamArray[it]["type"]
         else if word 4 of tLine[1] contains "data" then
            put "data" into tParamArray[it]["type"]
         else
            put word 4 of tLine[1] into tParamArray[it]["type"]
         end if
         
         if tOptional then
            put "optional " before tParamArray[it]["type"]
         end if
         put word 1 to -1 of tLine[2] into tParamArray[it]["description"]
      else
         put word 2 of tLine[1] into tValue["type"]
         put word 1 to -1 of tLine[2] into tValue["description"] 
      end if
   end repeat
   delete char -2 to -1 of tParamsSyntax
   
   if pType is "function" then
      put "(" before tParamsSyntax
      put ")" after tParamsSyntax
      put pEntryName & tParamsSyntax into tSyntax
   else
      put pEntryName && tParamsSyntax into tSyntax
   end if
   
   put tSyntax into xDataA[pEntryName]["syntax"][1]
   put tParamArray into xDataA[pEntryName]["params"]
   put pType into xDataA[pEntryName]["type"]
   put pEntryName into xDataA[pEntryName]["name"]
   if pType is "function" then
      put tValue into xDataA[pEntryName]["return value"]
   else
      put tValue into xDataA[pEntryName]["the result"]
   end if
   put empty into tValue
   
   # Output the comments that preceded this function definition
   put pComment into xDataA[pEntryName]["comments"]
   
   # Distinguish from syntax
   put "handler" into xDataA[pEntryName]["kind"]
end revDocsUpdateDocBlocksForLCIDL

function revDocsParseReferences pDescription
   local tReferences
   
   local tRef, tType
   repeat for each item tItem in pDescription
      if matchText(tItem, "(.*)\((.*)\)", tRef, tType) then
         put word 1 to -1 of tRef into tRef
         put word 1 to -1 of tType into tType
         put tRef into tReferences[tType][the number of elements of tReferences[tType] + 1]
      end if
   end repeat
   return tReferences
end revDocsParseReferences

function revDocsParseDescriptionForType pType, pDescription
   if word 1 of pType is "optional" then
      // more formatting for optional type?
      put word 2 to -1 of pType into pType
   end if
   local tArray
   switch pType
      case "array"
         put revDocsParseDescriptionOfArray(pDescription) into tArray
         break
      case "enum"
         put revDocsParseDescriptionOfEnum(pDescription) into tArray
         break
      default
         put pDescription into tArray["description"]
         put pType into tArray["type"]
         break
   end switch
   return tArray
end revDocsParseDescriptionForType

function revDocsParseKeyValuePair pToParse
   local tArray
   local tOptional 
   put false into tOptional
   
   local tElementsA, tToParse
   repeat with x = 1 to 2
      put word 1 to -1 of line x of pToParse & return after tToParse
   end repeat
   put revDocsExtractElementsWithRegex(tToParse, __arrayElementRegex()) into tElementsA
   
   // flush out key value pair into tArray
   if word 1 of tElementsA[1]["type"] is "optional" then
      put true into tOptional
      put word 2 to -1 of tElementsA[1]["type"] into tElementsA[1]["type"]
   end if
   put tElementsA[1]["name"] into tArray["key"]["name"]
   put tElementsA[1]["type"] into tArray["key"]["type"]
   put tElementsA[1]["content"] into tArray["key"]["description"]
   put tOptional into tArray["key"]["optional"]
   
   put tElementsA[2]["name"] into tArray["value"]["name"]
   put tElementsA[2]["type"] into tArray["value"]["type"]
   put tElementsA[2]["content"] into tArray["value"]["description"]
   return tArray
end revDocsParseKeyValuePair

function revDocsParseSubArray pToParse
   local tArray
   local tStart, tFinish, tArrayDef
   
   local tOptional
   local tCount
   put 1 into tCount
   put false into tOptional
   repeat while pToParse is not empty
      
      if word 1 of line 1 of pToParse is empty then
         delete line 1 of pToParse
         next repeat
      end if
      
      put revDocsParseKeyValuePair(line 1 to 2 of pToParse) into tArray[tCount]
      
      local tOpenOffset, tCloseOffset, tUpdateOpen
      put 0 into tOpenOffset
      if tArray[tCount]["value"]["type"] is "array" then
         put offset("{", pToParse) into tOpenOffset
      end if
      
      // if there are no opening brackets, then we're done with this key-value pair
      if tOpenOffset is 0 then
         add 1 to tCount
         delete line 1 to 2 of pToParse
         next repeat
      end if
      
      local tNesting, tCurrent, tFirst
      put true into tFirst
      put 0 into tCurrent
      
      put tOpenOffset + 1 into tStart
      put offset("}", pToParse) into tCloseOffset
      put 0 into tNesting
      
      repeat while tFirst is true or tNesting is not 0
         put false into tFirst
         if tOpenOffset > 0 and tOpenOffset < tCloseOffset then
            add 1 to tNesting
            put true into tUpdateOpen
            put tOpenOffset into tCurrent
         else
            local tSubArray
            subtract 1 from tNesting
            put false into tUpdateOpen
            put tCloseOffset into tCurrent
            if tNesting is 0 then
               put char tStart to tCloseOffset - 1 of pToParse into tSubArray
               get revDocsParseSubArray(tSubArray)
               put it into tArray[tCount]["value"]["array"]
               add 1 to tCount
               put char tCloseOffset + 1 to -1 of pToParse into pToParse
               exit repeat
            end if
         end if
         if tUpdateOpen then
            get offset("{", pToParse, tCurrent) 
            if it is 0 then
               put -1 into tOpenOffset
            else
               put it + tCurrent into tOpenOffset
            end if
         else
            get offset("}", pToParse, tCurrent) 
            if it is 0 then
               // Add closing parethesis at the end if one is not found
               put "}" after pToParse 
               put length(pToParse) into tCloseOffset
            else
               put it + tCurrent into tCloseOffset
            end if
         end if
      end repeat
   end repeat
   return tArray
end revDocsParseSubArray

/*
Parses a parameter description according to the rules of documenting an array

pDescription (string) : The description to be formatted for an array type
Returns (array) : An array representing the data for the documented array

*/

function revDocsParseDescriptionOfArray pDescription
   local tCount
   put 1 into tCount
   
   local tArray, tOutput
   
   // Find extent of array definition
   get offset("{", pDescription)
   put word 1 to -1 of (char 1 to it - 1 of pDescription) & return into tOutput
   
   local tIndex, tArrayDef
   put "-1" into tIndex
   if offset("}", pDescription) is not 0 then
      repeat until char tIndex of pDescription is "}"
         subtract 1 from tIndex
      end repeat
      put word 1 to -1 of (char it + 1 to tIndex - 1 of pDescription) into tArrayDef
   end if
   
   put tOutput into tArray["description"]
   put "array" into tArray["type"]
   put revDocsParseSubArray(tArrayDef) into tArray["array"]
   return tArray
end revDocsParseDescriptionOfArray

/*
Parses a parameter description according to the rules of documenting an enum

pDescription (string) : The description to be formatted for an array type
Returns (array) : An array representing the data for the documented array

*/

function revDocsParseDescriptionOfEnum pDescription
   local tArray, tOutput, tInEnum
   local tCount
   put 1 into tCount
   repeat for each line tLine in pDescription
      
      # A blank line ends an enum and goes back to description
      if word 1 of tLine is empty then 
         if tInEnum then
            put false into tInEnum
         else
            next repeat
         end if
      end if
      
      local tValue, tOption, tDescription
      if matchText(tLine, " *[-\*] *([^:\(]*)(?:\(([^\)]*)\))? *: *(.*)", tValue, tOption, tDescription) then
         if tOption is not empty then
            put "(" & word 1 to -1 of tOption & ")" after tValue
         end if
         
         put word 1 to -1 of tValue into tArray["enum"][tCount]["value"]
         put word 1 to -1 of tDescription into tArray["enum"][tCount]["description"]
         add 1 to tCount
         put true into tInEnum
      else if tInEnum then
         put space & word 1 to -1 of tLine after tArray["enum"][tCount-1]["description"]
      else
         put tLine & return after tOutput
      end if
   end repeat
   
   put tOutput into tArray["description"]
   put "enum" into tArray["type"]
   return tArray
end revDocsParseDescriptionOfEnum

##################################################
#
#          JSON OUTPUT UTILITIES
#
##################################################

private function __tabs pNum
   local tOutput
   repeat pNum
      put tab after tOutput
   end repeat
   return tOutput
end __tabs

function revDocsFormatLibraryDocArrayAsJSON pLibraryA
   local tJSON, tCount, tDocA
   put pLibraryA["doc"] into tDocA
   repeat for each element tDoc in tDocA
      local tId
      add 1 to tCount
      put pLibraryA["name"] & "-" & tCount into tId
      local tDocJson
      put revDocsFormatLibraryDataAsJSON(tId, tDoc, pLibraryA) into tDocJson 
      if tDocJson is not empty then
         put tDocJson & comma after tJSON
      end if
   end repeat
   delete the last char of tJSON
   return tJSON
end revDocsFormatLibraryDocArrayAsJSON

function revDocsCreateAPIJSON pLibraryTitle, pLibraryName, pAuthor, pDocContentJSON
   local tJSON
   put "{" after tJSON
   put return & __tabs(2) & keyValue("display name",pLibraryTitle) & comma after tJSON
   put return & __tabs(2) & keyValue("name",pLibraryName) & comma after tJSON
   if pAuthor is not empty then
      put return & __tabs(2) & keyValue("author",pAuthor) & comma after tJSON
   end if
   put return & __tabs(2) & escape("doc") & ":[" after tJSON
   put pDocContentJSON after tJSON
   put "]" after tJSON
   put return & tab & "}" after tJSON
   return tJSON
end revDocsCreateAPIJSON

function revDocsFormatLibraryArrayAsJSON pLibraryA
   local tDocContent
   put revDocsFormatLibraryDocArrayAsJSON(pLibraryA) \
         into tDocContent
   return revDocsCreateAPIJSON(pLibraryA["display name"], pLibraryA["name"], \
         pLibraryA["author"], tDocContent)
end revDocsFormatLibraryArrayAsJSON

private function __revDocsParamIsDefinedType pType
   switch pType
      case "pointer"
      case "bool"
      case "int"
      case "uint"
      case "index"
      case "uindex"
      case "float"
      case "double"
      case "any"
      case "boolean"
      case "integer"
      case "real"
      case "number"
      case "string"
      case "data"
      case "binary string"
      case "array"
      case "list"
      case "undefined"
      case "enum"
      case "set"
      case "id"
         return false
      default
         return true
   end switch
end __revDocsParamIsDefinedType

function revDocsArrayParamToJSON pParamA, pDepth
   if the keys of pParamA is empty then
      return empty
   end if
   local tOutput, tPrefix
   put return & __tabs(pDepth) after tPrefix
   put escape("array") & ":[" after tOutput
   repeat for each element tKeyValue in pParamA
      put "{" after tOutput
      put tPrefix & tab & escape("key") & ": {"  after tOutput
      put tPrefix & tab & tab & keyValue("name", tKeyValue["key"]["name"]) & comma after tOutput
      put tPrefix & tab & tab & keyValue("type", tKeyValue["key"]["type"]) & comma after tOutput
      put tPrefix & tab & tab & keyValue("description", tKeyValue["key"]["description"]) & "}" & comma after tOutput
      put tPrefix & tab & escape("value") & ": {"  after tOutput
      put revDocsTypedParamToJSON(tKeyValue["value"], pDepth + 2) & "}" after tOutput
      put tPrefix & "}" & comma  after tOutput
   end repeat
   delete the last char of tOutput
   put "]" after tOutput
   return tOutput
end revDocsArrayParamToJSON

function revDocsEnumParamToJSON pParamA
   if the keys of pParamA is empty then
      return empty
   end if
   local tOutput, tPrefix
   put return & __tabs(4) into tPrefix
   put tPrefix & escape("enum") & ":[" after tOutput
   repeat for each element tKeyValue in pParamA
      put tPrefix & tab & "{" & keyValue("value", tKeyValue["value"]) & comma after tOutput
      put tPrefix & tab & keyValue("description", tKeyValue["description"]) & "}" & comma after tOutput
   end repeat
   delete the last char of tOutput
   put "]" after tOutput
   return tOutput
end revDocsEnumParamToJSON

function revDocsTypedParamToJSON pParamA, pDepth, pRef
   if the keys of pParamA is empty then
      return empty
   end if
   local tOutput, tPrefix
   put return & __tabs(pDepth) after tPrefix
   put tPrefix & keyValue("name", pParamA["name"]) & comma into tOutput
   put tPrefix & keyValue("type", pParamA["type"]) & comma after tOutput
   if pRef is not false then
      put tPrefix & keyValue("refparam", pParamA["refparam"]) & comma after tOutput
   end if
   put tPrefix & keyValue("description", pParamA["description"]) after tOutput
   if word 1 of pParamA["type"] is "optional" then
      // more formatting for optional type?
   end if
   
   switch pParamA["type"]
      case "array"
         get revDocsArrayParamToJSON(pParamA["array"], pDepth)
         if it is not empty then
            put comma & tPrefix & it after tOutput
         end if
         break
      case "enum"
         get revDocsEnumParamToJSON(pParamA["enum"]) 
         if it is not empty then
            put comma & tPrefix & it after tOutput
         end if
         break
      default
         break
   end switch
   return tOutput
end revDocsTypedParamToJSON

function __revDocsFormatParamArrayAsJSON pParamA
   local tOutput
   put "[" into tOutput 
   repeat with x = 1 to the number of elements in pParamA
      put "{" after tOutput
      put revDocsTypedParamToJSON(pParamA[x], 4, true) after tOutput
      put return & __tabs(3) & "}" & comma after tOutput
   end repeat
   delete the last char of tOutput
   put "]" after tOutput
   return tOutput
end __revDocsFormatParamArrayAsJSON

function __revDocsFormatValuesArrayAsJSON pParamA
   local tOutput
   put "[" into tOutput 

   local tReturnTypes
   put "return,value,it,the result" into tReturnTypes
   repeat for each item tReturn in tReturnTypes
      if pParamA[tReturn] is empty then
         next repeat
      end if
      put "{" after tOutput
      put tReturn into pParamA[tReturn]["name"]
      put revDocsTypedParamToJSON(pParamA[tReturn], 4, false) after tOutput
      put return & __tabs(3) & "}" & comma after tOutput
   end repeat
   delete the last char of tOutput
   put "]" after tOutput
   return tOutput
end __revDocsFormatValuesArrayAsJSON

function __revDocsFormatExamplesArrayAsJSON pExampleA
   local tOutput
   put "[" into tOutput 
   repeat for each element tExample in pExampleA
      put "{" & return & __tabs(4) & keyValue("script", tExample) after tOutput
      put return & __tabs(3) & "}" & comma after tOutput
   end repeat
   delete the last char of tOutput
   put "]" after tOutput
   return tOutput
end __revDocsFormatExamplesArrayAsJSON

function revDocsItemListToJSONArray pArray, pKey, pTabs
   if pArray[pKey] is empty then
      return empty
   end if
   
   local tTagList
   repeat for each element tTag in pArray[pKey]
      put escape(tTag) & comma after tTagList
   end repeat
   delete the last char of tTagList
   
   if pTabs is empty then
      put 3 into pTabs
   end if
   
   return __tabs(pTabs) & escape(pKey) & ":[" & tTagList & "]" & comma & return
end revDocsItemListToJSONArray

function __formatParamsAsItalic pText
   local tStart, tEnd
   repeat while matchChunk(pText, "<([^i]|[^><\/]{2,})>", tStart, tEnd)
      put "</i" after char tEnd of pText
      put "i>" before char tStart of pText
   end repeat
   return pText
end __formatParamsAsItalic

function __revDocsFormatDisplaySyntaxArrayAsJSON pSyntaxA
   local tOutput
   put "[" into tOutput 
   repeat for each element tExample in pSyntaxA
      put return & __tabs(4) & escape(__formatParamsAsItalic(tExample)) & comma after tOutput
   end repeat
   delete the last char of tOutput
   put return & __tabs(3) & "]" after tOutput
   return tOutput
end __revDocsFormatDisplaySyntaxArrayAsJSON

function __revDocsFormatSyntaxArrayAsJSON pSyntaxA
   local tOutput
   put "[" into tOutput 
   repeat for each element tExample in pSyntaxA
      put return & __tabs(4) & escape(tExample) & comma after tOutput
   end repeat
   delete the last char of tOutput
   put return & __tabs(3) & "]" after tOutput
   return tOutput
end __revDocsFormatSyntaxArrayAsJSON

private function keyValue pKey, pElement
   return escape(tolower(pKey)) & ":" & escape(pElement)
end keyValue

private function escape pString, pConvertLineEndings
   replace "\" with "\\" in pString
   replace quote with ("\" & quote) in pString
   
   if pConvertLineEndings is true then
      replace CRLF with "\n" in pString
      replace numToCodepoint(13) with "\n" in pString
   end if
   replace LF with "\n" in pString
   replace numToCodepoint(13) with "\r" in pString
   //replace "<" with "&lt;" in pString
   //replace ">" with "&gt;" in pString
   replace tab with "\t" in pString
   return (quote & pString & quote)
end escape

function revDocsModifyForURL pName
   replace " " with "_" in pName
   return urlencode(toLower(pName))
end revDocsModifyForURL

function outputArrayElement pElement, pArray, pTabCount, pLower
   if pLower then
      return outputElement(pElement, tolower(pArray[pElement]), pTabCount)
   else
      return outputElement(pElement, pArray[pElement], pTabCount)
   end if
end outputArrayElement

function outputElement pElement, pValue, pTabCount
   local tOutput
   repeat pTabCount
      put tab after tOutput
   end repeat
   put keyValue(pElement, pValue) & comma & return after tOutput
   return tOutput
end outputElement

function outputElementIfExists pElement, pArray, pTabCount
   if pArray[pElement] is not empty then
      return outputElement(pElement, pArray[pElement], pTabCount)
   end if
   return empty
end outputElementIfExists

function __revDocsFormatReferencesArrayAsJSON pReferencesA
   if pReferencesA is empty then
      return empty
   end if
   
   local tOutput
   put tab & tab & tab & escape("references") & ":{" & return into tOutput
   repeat for each key tKey in pReferencesA
      get revDocsItemListToJSONArray(pReferencesA, tKey, 4)
      put it after tOutput
   end repeat
   delete char -2 to -1 of tOutput
   put return & tab & tab & tab & "}," & return after tOutput
   return tOutput
end __revDocsFormatReferencesArrayAsJSON

function __revDocsFormatCustomArrayAsJSON pKey, pArray
   if pArray[pKey] is empty then
      return empty
   end if
   switch pKey
      case "syntax"
         return tab & tab & tab & escape("syntax") & ":" & __revDocsFormatSyntaxArrayAsJSON(pArray["syntax"]) & comma & return
      case "display syntax"
         return tab & tab & tab & escape("display syntax") & ":" & __revDocsFormatDisplaySyntaxArrayAsJSON(pArray["display syntax"]) & comma & return
      case "params"
         return tab & tab & tab & escape("parameters") & ":" & __revDocsFormatParamArrayAsJSON(pArray["params"]) & comma & return
      case "example"
         return tab & tab & tab & escape("examples") & ":" & __revDocsFormatExamplesArrayAsJSON(pArray["example"]) & comma & return 
      case "value"
         return tab & tab & tab & escape("value") & ":" & __revDocsFormatValuesArrayAsJSON(pArray["value"]) & comma & return
      default
         throw "invalid value " & pKey & "passed to __revDocsFormatCustomArrayAsJSON"
         return empty
   end switch
end __revDocsFormatCustomArrayAsJSON

/* 
Format the library array data as a JSON array as needed by the docs viewer
*/

function revDocsFormatLibraryDataAsJSON pID, pDocDataA, pLibraryDataA
   local tJSON
   put "{" & return into tJSON
   
   if pDocDataA["display name"] is empty then
      put pDocDataA["name"] into pDocDataA["display name"]
   else if pDocDataA["name"] is empty then
      put pDocDataA["display name"] into pDocDataA["name"]
   end if
   
   if pDocDataA["display name"] is empty and \
         pDocDataA["name"] is empty then
      logError "No doc name found for" && pID
      return empty
   end if
   
   # The 'name' should be urlencoded
   put revDocsModifyForURL(pDocDataA["name"]) into pDocDataA["name"]
   
   # ID
   put outputElement("id", pID, 3) after tJSON
   
   # Name
   put outputArrayElement("name", pDocDataA, 3) after tJSON
   
   # Display Name
   put outputArrayElement("display name", pDocDataA, 3) after tJSON
   
   # Library Name
   put outputElement("library", pLibraryDataA["name"], 3) after tJSON
   
   # Type
   put outputArrayElement("type", pDocDataA, 3, true) after tJSON
   
   # Syntax
   put __revDocsFormatCustomArrayAsJSON("syntax", pDocDataA) after tJSON
   
   # Display syntax
   if pDocDataA["syntax"] is not empty then
      put line 1 of pDocDataA["syntax"][1] into pDocDataA["display syntax"][1]
   else
      put pDocDataA["display name"] into pDocDataA["display syntax"][1]
   end if
   put __revDocsFormatCustomArrayAsJSON("display syntax", pDocDataA) after tJSON
   
   # Synonyms
   put revDocsItemListToJSONArray(pDocDataA, "synonyms") after tJSON
   
   # Associations
   put revDocsItemListToJSONArray(pDocDataA, "associations") after tJSON
   
   # Summary
   put outputElementIfExists("summary", pDocDataA, 3) after tJSON
   
   # Edition
   put outputElementIfExists("edition", pDocDataA, 3) after tJSON
   
   # Introduced
   put outputElementIfExists("introduced", pDocDataA, 3) after tJSON
   
   # Deprecated
   put outputElementIfExists("deprecated", pDocDataA, 3) after tJSON
   
   # OS
   put revDocsItemListToJSONArray(pDocDataA, "OS") after tJSON
   
   # Platforms
   put revDocsItemListToJSONArray(pDocDataA, "platforms") after tJSON
   
   # Security
   put revDocsItemListToJSONArray(pDocDataA, "security") after tJSON
   
   # Parameters
   put __revDocsFormatCustomArrayAsJSON("params", pDocDataA) after tJSON
   
   # Example
   put __revDocsFormatCustomArrayAsJSON("example", pDocDataA) after tJSON
   
   # Value
   put __revDocsFormatCustomArrayAsJSON("value", pDocDataA) after tJSON
   
   # Description
   put outputElementIfExists("description", pDocDataA, 3) after tJSON
   
   # References
   put __revDocsFormatReferencesArrayAsJSON(pDocDataA["references"]) after tJSON
   
   # Tags
   put revDocsItemListToJSONArray(pDocDataA, "tags") after tJSON
   
   # Changes
   put outputElementIfExists("changes", pDocDataA, 3) after tJSON
   
   delete char -2 to -1 of tJSON
   put return & tab & tab & "}" after tJSON
   return tJSON
   
end revDocsFormatLibraryDataAsJSON

##################################################
#
#        Docs API database
#
##################################################

constant kAPIDatabaseFile = "api.sqlite"

function revDocsAPIDatabasePath pFolder
   return pFolder & slash & kAPIDatabaseFile
end revDocsAPIDatabasePath

on revDocsOpenAPIDatabase pFolder
   local tConnection
   put revOpenDatabase("sqlite",revDocsAPIDatabasePath(pFolder),,,,,,) into tConnection
   
   if tConnection is not a number then
      return "unable to create sqlite API database" & return & the result
   end if
   
   return tConnection
end revDocsOpenAPIDatabase

on revDocsUpdateDatabase pConnection, pAPI, pLibraryA
   if pConnection is empty then
      return "no database connection"
   end if
   
   local tSQL, tResult
   
   # Create the libraries table. This might already exist, so do it unchecked.
   put "CREATE TABLE apis(api_id integer primary key, api_name text)" into tSQL
   revExecuteSQL pConnection, tSQL
   
   # Try an UPDATE first. If this fails, the entry might not exist already, so try an INSERT.
   put "UPDATE apis SET api_name = :1 WHERE api_name = :1" into tSQL
   revExecuteSQL pConnection, tSQL, "pAPI" 
   put the result into tResult
   if tResult is not a number or tResult is 0 then
      put "INSERT into apis VALUES(NULL, :1)" into tSQL
      revExecuteSQL pConnection, tSQL, "pAPI"
      put the result into tResult
   end if
   
   if tResult is not a number then
      return "unable to open table 'apis'" & return & tResult
   end if
   
   # Create the library data table. This might already exist, so do it unchecked.
   put "CREATE TABLE dictionary_data(api_id integer, library_name text, entry_name text, entry_type text, entry_data blob)" into tSQL
   revExecuteSQL pConnection, tSQL
   
   local tName
   put pLibraryA["display name"] into tName
   
   revDocsUpdateLibrary pConnection, pAPI, tName, pLibraryA["doc"]
   if the result is not empty then
      return "unable to update data for library" && tName & return & the result
   end if
   
   return empty
end revDocsUpdateDatabase

on revDocsUpdateLibrary pConnection, pAPI, pName, pDocDataA
   # Find the API id
   local tSQL, tAPIID
   put "SELECT api_id FROM apis WHERE api_name = :1" into tSQL
   put revDataFromQuery(comma, return, pConnection, tSQL, "pAPI") into tAPIID
   
   if the result is not a number then
      return "error finding api id for" && pAPI & return & the result
   end if
   
   local tDocA, tDocName
   repeat for each element tDocA in pDocDataA
      put tDocA["display name"] into tDocName

      local tType
      put tDocA["type"] into tType
      
      local tEncodedData, tResult
      put arrayEncode(tDocA) into tEncodedData
      # Try an UPDATE first. If this fails, the entry might not exist already, so try an INSERT.
      put "UPDATE dictionary_data SET entry_data = :1 WHERE library_name = :2" && \
            "AND entry_name = :3 AND entry_type = :4 AND api_id = :5" into tSQL
      revExecuteSQL pConnection, tSQL, "*btEncodedData", "pName", "tDocName", "tType", "tAPIID"
      put the result into tResult
      if tResult is not a number or tResult is 0 then
         put "INSERT into dictionary_data VALUES(:1, :2, :3, :4, :5)" into tSQL
         revExecuteSQL pConnection, tSQL, "tAPIID", "pName", "tDocName", "tType", "*btEncodedData"
         put the result into tResult
      end if
      
      if tResult is not a number then
         return "unable to insert data for" && pName & ":" & tDocName & return & the result
      end if
   end repeat
   return empty
end revDocsUpdateLibrary

on revDocsRemoveLibrary pConnection, pAPI, pName
   # Find the library id
   local tSQL, tAPIID
   put "SELECT api_id FROM apis WHERE api_name = :1" into tSQL
   put revDataFromQuery(comma, return, pConnection, tSQL, pAPI) into tAPIID
   
   if the result is not a number then
      return "error finding api id for" && pAPI & return & the result
   end if
   
   # Delete all entries associated with this library
   put "DELETE * FROM dictionary_data WHERE api_id = :1 AND library_name = :2" into tSQL
   revExecuteSQL pConnection, tSQL, "tAPIID", "pName"
   if the result is not a number then
      return "unable to delete data for" && pName & return & the result
   end if
   
   return empty
end revDocsRemoveLibrary


/*
Guide files can have a YAML-like metadata block at the start. The block 
is delimited by lines of exactly three hyphens "---", and every line in 
between *must* be blank or contain a "key: value" pair.  For example:
---
group: deployment
---
*/
private command revDocsSplitMetadata pText, @rMarkdown, @rMetadata
   -- Check for a metadata block
   if the first line of pText is not "---" then
      put empty into rMetadata
      put pText into rMarkdown
      exit revDocsSplitMetadata
   end if
   
   -- Search for a terminal "---" line
   local tMetadataEnd
   put lineOffset("---", pText, 1) into tMetadataEnd
   if tMetadataEnd is 1 then
      throw "Unterminated metadata header"
   end if
   
   put revDocsParseYaml(line 2 to tMetadataEnd of pText) into rMetadata
   put line (tMetadataEnd + 2) to -1 of pText into rMarkdown
end revDocsSplitMetadata

/*
Parse yaml into a structured array.
At the moment, we only deal with key value pairs, where the value is
either a string or a list.
*/
private function revDocsParseYaml pYaml
   local tYaml
   
   local tNumLines
   put the number of lines in pYaml into tNumLines
   set the itemdelimiter to colon
   
   local tCurLine, tLine
   repeat while tCurLine <= tNumLines
      put line tCurLine of pYaml into tLine
      put word 1 to -1 of tLine into tLine
      if item 1 of tLine is empty or char 1 of tLine is "#" then
         add 1 to tCurLine
         next repeat
      end if
      
      local tKey, tValue
      put item 1 of tLine into tKey
      if item 2 of tLine is empty then
         # parse a list
         local tList, tNextKey
         put lineOffset(":", pYaml, tCurLine) + tCurLine into tNextKey
         if tNextKey is tCurLine then
            put tNumLines + 1 into tNextKey
         end if
         put line tCurLine + 1 to tNextKey - 1 of pYaml into tList
         put revDocsParseYamlList(tList) into tValue
         put tNextKey into tCurLine
      else
         # simple key-value pair
         put word 1 to -1 of (item 2 to -1 of tLine) into tValue
         add 1 to tCurLine
      end if
      put tValue into tYaml[tKey]
   end repeat
   return tYaml
end revDocsParseYaml

private command addToList pElement, @xArray
   local tCount
   put the number of elements in xArray into tCount
   put pElement into xArray[tCount + 1]
end addToList

private function revDocsParseYamlList pYaml
   local tListA
   repeat for each line tLine in pYaml
      set the itemdelimiter to "-"
      put word 1 to -1 of tLine into tLine
      if the number of items of tLine is 0 or char 1 of tLine is "#" then
         next repeat
      end if
      
      addToList word 1 to -1 of item 2 of tLine, tListA
   end repeat
   return tListA
end revDocsParseYamlList

function revDocsGuideNameFromFile pFile
   set the itemdelimiter to "."
   return item 1 to -2 of pFile
end revDocsGuideNameFromFile

function revDocsGuideData pFolder, pLocation, pFile
   local tGuideContent, tGuideFile
   put pFolder & slash & pFile into tGuideFile
   put revDocsUtf8FileContents(tGuideFile) into tGuideContent
   
   local tGuideDataA
   if tGuideContent is not empty then
      revDocsSplitMetadata tGuideContent, tGuideDataA["data"], tGuideDataA["metadata"]
      
      replace "[toc]" with empty in tGuideDataA["data"]
      
      local tGuideName
      put revDocsGuideNameFromFile(pFile) into tGuideName
      
      put tGuideName into tGuideDataA["display name"]
      put revDocsModifyForURL(tGuideName) into tGuideDataA["name"]
      put pLocation into tGuideDataA["location"]
      
      put tGuideFile into tGuideDataA["file"]
   end if
   return tGuideDataA
end revDocsGuideData

function revDocsListGuides pFolder
   local tGuides
   put files(pFolder) into tGuides
   filter tGuides with "*.md"
   filter tGuides without "README.md"
   
   return tGuides
end revDocsListGuides

function revDocsGuideDataFromFolder pFolder, pLocation
   local tGuides
   put revDocsListGuides(pFolder) into tGuides

   local tGuideDataA, tKey
   repeat for each line tGuideFile in tGuides
      put pFolder & slash & tGuideFile into tKey
      put revDocsGuideData(pFolder, pLocation, tGuideFile) into tGuideDataA[tKey]
   end repeat

   return tGuideDataA
end revDocsGuideDataFromFolder

constant kNoGuideGroup = "no group"
private function groupOrder pOrderA, pGuideDataA, pKey
   local tGroup
   put pGuideDataA[pKey]["metadata"]["group"] into tGroup
   if tGroup is among the keys of pOrderA then
      return tGroup
   else 
      return kNoGuideGroup
   end if
end groupOrder

function revDocsUtf8FileContents pFile
   local tContents
   put url ("binfile:" & pFile) into tContents
   put textDecode(tContents, "utf-8") into tContents
   -- Since we are using url(binfile..), we need to convert the 
   -- line endings of the target file
   replace CRLF with LF in tContents
   replace numToCodepoint(13) with LF in tContents
   return tContents
end revDocsUtf8FileContents

constant kGuideOrderFile = "guide-order.yml"
private function groupOrderYaml pOrderFileLocation
   return revDocsUtf8FileContents(pOrderFileLocation & slash & kGuideOrderFile)
end groupOrderYaml

function revDocsGroupOrder pOrderFileLocation
   local tOrderYaml
   put groupOrderYaml(pOrderFileLocation) into tOrderYaml
   
   local tParsedA
   put revDocsParseYaml(line 2 to -2 of tOrderYaml) into tParsedA
   
   local tOrderA
   put tParsedA["group-order"] into tOrderA
   addToList kNoGuideGroup, tOrderA
   
   local tOrderLookupA
   repeat for each key tNumber in tOrderA
      local tGroup
      put tOrderA[tNumber] into tGroup
      put tNumber into tOrderLookupA[tGroup]
   end repeat
   
   return tOrderLookupA
end revDocsGroupOrder

function revDocsOrderedGuideData pGuideFoldersA, pOrderFileLocation
   local tGuideDataA
   repeat for each element tGuideWithLocation in pGuideFoldersA
      local tLocation, tFolder
      put tGuideWithLocation["location"] into tLocation
      put tGuideWithLocation["folder"] into tFolder
      union tGuideDataA with revDocsGuideDataFromFolder(tFolder, tLocation)
   end repeat
   
   local tGuides
   put the keys of tGuideDataA into tGuides
   
   # First sort by name
   sort tGuides by tGuideDataA[each]["display name"]
   
   # Then sort by group order
   local tGroupOrder
   put revDocsGroupOrder(pOrderFileLocation) into tGroupOrder
   repeat for each key tGuide in tGuideDataA
      local tGroup
      put groupOrder(tGroupOrder, tGuideDataA, tGuide) into tGroup
      if tGroup is kNoGuideGroup then
         put "Other" into tGroup
      else
         put toUpper(char 1 of tGroup) & char 2 to -1 of tGroup into tGroup
      end if
      
      put tGroup into tGuideDataA[tGuide]["group"]
   end repeat
   
   sort tGuides by tGroupOrder[tGuideDataA[each]["group"]]
   
   local tOrderedGuideDataA
   repeat for each line tGuideFile in tGuides
      addToList tGuideDataA[tGuideFile], tOrderedGuideDataA
   end repeat
   return tOrderedGuideDataA
end revDocsOrderedGuideData

function revDocsFormatGuideDataAsJSON pGuideDataA
   local tGuideName, tGuideData
   put tab & escape("name") & ":" & escape(revDocsModifyForURL(pGuideDataA["name"])) & comma & return after tGuideData 
   put tab & escape("display name") & ":" & escape(pGuideDataA["display name"]) & comma & return after tGuideData 
   put tab & escape("location") & ":" & escape(pGuideDataA["location"]) & comma & return after tGuideData
   put tab & escape("group") & ":" & escape(pGuideDataA["group"]) & comma & return after tGuideData
   put tab & escape("data") & ":" & escape(pGuideDataA["data"], true)  after tGuideData
   return tGuideData
end revDocsFormatGuideDataAsJSON


on pushNotificationRegistrationError pErrorMessage
   
end pushNotificationRegistrationError
