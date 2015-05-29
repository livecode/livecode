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
   set the defaultFolder to tModulePath
   put the files into tInstalled
   filter tInstalled with "*.lci"
   
   local tSourcePaths
   put tScript into tSourcePaths[1]
   put tEngine into tSourcePaths[2]
   
   local tCount
   put 1 into tCount
   
   set the itemdelimiter to "."
   local tFiles
   repeat for each element tElement in tSourcePaths
      set the defaultfolder to tElement
      local tDocsA
      put the files into tFiles
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
   
   set the defaultfolder to (pRepoPath & slash & "docs" & slash & "builder")
   put the files into tFiles
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
   set the defaultFolder to tModulePath
   put the files into tInstalled
   filter tInstalled with "*.lci"
   
   local tSourcePaths
   put tScript into tSourcePaths[1]
   put tEngine into tSourcePaths[2]
   
   local tCount
   put 1 into tCount
   
   set the itemdelimiter to "."
   local tFiles
   repeat for each element tElement in tSourcePaths
      set the defaultfolder to tElement
      local tDocsA
      put the files into tFiles
      filter tFiles with regex pattern ".*(\.(mlc|lcb))+"
      # Check to see if the corresponding interface file exists so that we 
      # don't add anything to the dictionary that isn't actually there.
      repeat for each line tFile in tFiles
         local tInterfaceFile
         put "com.livecode." & tFile into tInterfaceFile
         put "lci" into item -1 of tInterfaceFile
         if replaceText(tInterfaceFile, "-", "") is not among the lines of tInstalled then next repeat
         get revDocsGenerateDocsFileFromModular(url ("file:" & tFile))
         if it is not empty then
            put it into tDocsA[tCount]
            add 1 to tCount
         end if
      end repeat
   end repeat
   
   set the defaultfolder to (pRepoPath & slash & "docs" & slash & "builder")
   put the files into tFiles
   filter tFiles with "*.lcdoc"
   repeat for each line tFile in tFiles
      get url("file:" & tFile)
      if it is not empty then
         put it into tDocsA[tCount]
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
* <revDocsParseScriptToLibraryArray> creates a library array from the inline docs of a script
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
Summary : Creates a library array from the given script

pObjName: The name of the object whose script is being parsed
pText: The script to be parsed
pAuthor: The author name
*/

function revDocsParseScriptToLibraryArray pObjName, pText, pAuthor
   local tLibraryA
   
   put pObjName into tLibraryA["name"]
   put pAuthor into tLibraryA["author"]
   
   local tBlocksA, tParsedA
   put revDocsParseScriptDocs(pText) into tBlocksA
   put revDocsParseDocBlocks(tBlocksA) into tParsedA
   
   local tCount
   put 1 into tCount
   repeat for each key tEntry in tParsedA
      put tParsedA[tEntry] into tLibraryA["doc"][tCount]
      put tEntry into tLibraryA["doc"][tCount]["name"]
      add 1 to tCount
   end repeat
   
   return tLibraryA
end revDocsParseScriptToLibraryArray

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
      set the defaultfolder to tRoot
      repeat for each line tLine in the folders
         if tLine is ".." then next repeat
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
end revDocsParseDictionaryToLibraryArray

/*
Summary: Takes a .lcdoc file and formats it as a JSON array.
*/

function revDocsFormatAPIAsJSON pLibraryName, pAuthor, pAPIFile
   # Get the data
   local tText, tData
   put url ("binfile:" & pAPIFile) into tData
   put textDecode(tData, "utf-8") into tText
   
   return revDocsFormatDocTextAsJSON(tText, pLibraryName, pAuthor)
end revDocsFormatAPIAsJSON

/*
Summary: Takes a .lcdoc file and parses it into a library array.
*/
function revDocsParseFileToArray pFile, pType, @xCount, @xLibraryA
   # Get the data
   local tText, tData
   put url ("binfile:" & pFile) into tData
   put textDecode(tData, "utf-8") into tText
   
   return revDocsParseFileContentsToArray(tText, pType, xCount, xLibraryA)
end revDocsParseFileToArray

function revDocsParseFileContentsToArray pText, pType, @xCount, @xLibraryA
   # Extract and parse the comment blocks
   local tParsedA
   switch pType
      case "modular"
         put revDocsParseDocText(pText) into tParsedA
         break
      case "script"
         local tBlocksA
         put revDocsParseScriptDocs(pText) into tBlocksA
         put revDocsParseDocBlocks(tBlocksA) into tParsedA
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
   set the defaultfolder to pDir
   
   local tFiles
   put the files into tFiles
   filter tFiles with regex pattern ".*(\.(mlc|lcb))+"
   
   local tDocsA, tCount
   
   put 1 into tCount
   repeat for each line tFile in tFiles
      get revDocsGenerateDocsFileFromModular(url ("file:" & tFile))
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
   return revDocsGenerateDocsFileFromModular(url ("file:" & pFile))
end revDocsGenerateDocsFileFromModularFile

/*
Convert inline docs to the standard format (that can be parsed without code blocks)
*/

function revDocsGenerateDocsFileFromModular pText
   local tEntryType
   put empty into tEntryType
   
   local tInComment
   put false into tInComment
   
   local tComment, tEntryName, tParams
   local tHandlers, tPhrases, tTypes
   local tData
   local tNameLine, tSyntaxLine, tBindingLines
   local tModuleDescription, tModuleName, tModuleType
   local tIsPhrase, tPhraseName
   repeat for each line tLine in pText
      // Check to see if this is the start of a block of code or a block of comments 
      get word 1 of tLine 
      if not tInComment and tEntryType is empty then
         if it is "module" or it is "widget" or it is "library" then
            put word 2 to -1 of tLine into tModuleName
            put it into tModuleType
            if tComment is not empty then
               put tComment into tModuleDescription
               put empty into tComment
            end if
            next repeat
         else if it is "handler" then
            put it into tEntryType
         else if it is "syntax" then
            put it into tEntryType
            if word 4 of tLine is "phrase" then
               put true into tIsPhrase
               put word 2 of tLine into tPhraseName
            end if
         else if it is "public" then
            if word 2 of tLine is "handler" then
               put "handler" into tEntryType
               put tLine into tHandlers[token 3 of tLine]
            else if word 2 of tLine is "foreign" then
               if word 3 of tLine is "handler" then
                  put tLine into tHandlers[token 4 of tLine]
               else if word 3 of tLine is "type" then
                  put tLine into tTypes[token 4 of tLine]
               end if
            end if
         end if
         if it begins with "/*" then
            // if we have a new comment block, discard the previous one
            put empty into tComment
            local tCommentOffset
            put true into tInComment
            put offset("/*", tLine) into tCommentOffset
            put char tCommentOffset + 2 to -1 of tLine into tLine
            // If there's nothing else on this line, don't add it.
            if word 1 of tLine is empty then next repeat
         end if
      end if
      
      // If we're in a handler, check to see if this is the end of a block of code.
      if tEntryType is not empty  then
         // ignore empty lines
         if word 1 of tLine is empty then
            next repeat
         end if
         // If so, then flush all the data
         if it is "end" then
            // Don't add an API entry when there are no docs
            if tIsPhrase then
               put tSyntaxLine into tPhrases[tPhraseName]
            end if
            
            if tComment is not empty then
               if tEntryType is "syntax" then
                  revDocsUpdateModularSyntaxDocBlocks tEntryType, tNameLine, tSyntaxLine, tBindingLines, tComment, tHandlers, tPhrases, tData
               else
                  revDocsUpdateDocBlocks true, tEntryType, tNameLine, tComment, tData
               end if
            end if
            put empty into tComment
            put empty into tNameLine
            put empty into tEntryType
            put empty into tSyntaxLine
            put empty into tBindingLines
            put false into tIsPhrase
            put empty into tPhraseName
            // Otherwise gather the data from this line
         else if tEntryType is not "syntax" then
            // Name, Syntax & Params all on the same line for non-syntax
            if tNameLine is empty then
               put tLine into tNameLine
               put tLine into tSyntaxLine
            end if
         else
            // For syntax, the order is: name line, syntax line, param lines.
            if tNameLine is empty then
               put tLine into tNameLine
            else if tSyntaxLine is empty then
               put tLine into tSyntaxLine
            else if it is "begin" then 
               next repeat
            else
               put tLine & CR after tBindingLines
            end if
         end if
      end if
      // If we're in comments check to see if this is the end of a block of comments
      if tInComment then
         if word -1 of tLine ends with "*/" then
            put false into tInComment
            put offset("*/", tLine) into tCommentOffset
            put char 1 to tCommentOffset - 1 of tLine into tLine
            // If there's nothing else on this line, don't add it.
            if word 1 of tLine is empty then
               next repeat
            end if
         else if tComment is empty and word 1 of tLine is empty then
            next repeat
         end if
         put tLine & CR after tComment
      end if
   end repeat
   
   # Allow inline module description as valid docs
   if tModuleDescription is empty and tData is empty then
      logError "No documentation included"
      return empty
   end if
   
   if tModuleName is empty then
      logError "No extension name found"
   end if
   
   local tOutput
   local tStart, tEnd
   put "Library: " & tModuleName & CR & CR into tOutput
   if tModuleType is "module" then
      put "Type: library" & CR & CR after tOutput
   else
      put "Type: widget" & CR & CR after tOutput
   end if
   local tTmpArray
   put "Description:" & CR & tModuleDescription after tOutput
   put tOutput into tTmpArray["comments"]
   put revDocsFormatInlineComments(tTmpArray) into tOutput
   repeat for each key tKey in tData
      put "Name: " & tKey & CR & CR after tOutput
      put "Type: " & tData[tKey]["type"] & CR & CR after tOutput
      put "Syntax: " & word 1 to -1 of tData[tKey]["syntax"][1] & CR & CR after tOutput
      
      put revDocsFormatInlineComments(tData[tKey]) & CR & CR after tOutput
   end repeat
   return tOutput
end revDocsGenerateDocsFileFromModular

##################################################
#
#          PARSING UTILITIES
#
##################################################

private command logError pError
   put pError & CR after url ("file:" & specialfolderpath("Desktop") & slash & "log.txt")
end logError

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
   set the defaultfolder to pRootDir
   put the files into tFiles
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

function revDocsFormatInlineComments pDataA
   
   local tElementsA
   put revDocsExtractElements(pDataA["comments"]) into tElementsA
   
   local tEntriesA
   put revDocsGroupElements(tElementsA) into tEntriesA
   
   local tEntryA, tElementA, tElement
   
   local tHasReferences
   
   local tParamsFound
   put false into tParamsFound
   
   local tDocData
   repeat with x = 0 to the number of elements in tEntriesA - 1
      put false into tHasReferences
      put tEntriesA[x] into tEntryA
      repeat with y = 1 to the number of elements in tEntryA["elements"]
         put tEntryA["elements"][y] into tElementA
         
         put tElementA["name"] into tElement
         if tElement is "References" then
            put true into tHasReferences
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
                        put "Parameters:" & CR after tDocData
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
         if tElementA["type"] is not empty then
            put tElement & "(" & tElementA["type"] & "):" && tElementA["content"] & CR after tDocData
         else
            if the number of lines in tElementA["content"] > 1 then              
               local tDeleteTab
               put false into tDeleteTab
               put tElement & ":" & CR after tDocData
               repeat for each line tLine in tElementA["content"]
                  if line 1 of tElementA["content"] begins with tab then
                     put true into tDeleteTab
                  end if
                  if tDeleteTab and tLine begins with tab then
                     delete char 1 of tLine
                  end if
                  put tLine & CR after tDocData
               end repeat
            else
               put tElement & ":" && tElementA["content"] & CR after tDocData
            end if
         end if
         put CR after tDocData
      end repeat
      if tHasReferences is false then
         if pDataA["phrases"] is not empty then
            put "References: " after tDocData
            repeat for each element tElement in pDataA["phrases"]
               put tElement & "(phrase)" & comma after tDocData
            end repeat
            delete the last char of tDocData
            put CR & CR after tDocData
         end if
      end if
   end repeat
   return tDocData
end revDocsFormatInlineComments

function revDocsParseScriptDocs pText
   return revDocsExtractDocBlocksFromScript(pText)
end revDocsParseScriptDocs

function revDocsCollectGlossarySynonyms pRoot
   local tGlossaryA, tText, tName, tSynonyms
   set the defaultfolder to pRoot & slash & "glossary"
   local tFolders
   put the folders into tFolders
   repeat for each line tLine in tFolders
      if tLine is ".." then next repeat
      set the defaultfolder to pRoot & slash & "glossary" & slash &  tLine
      get the files
      filter it with "*.lcdoc"
      if it is not empty then
         repeat for each line tFile in it
            if tFile begins with "." then next repeat
            put url ("file:" & pRoot & slash & "glossary" & slash & tLine & slash & tFile) into tText
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
   return "^ *([tT]he [rR]esult|\w*) *(?:\(( *(?:optional |in |out |inout )?<?\w*>?) *\))? *: *(.*)"
end __elementRegex

/*
Utility function to extract any elements matching the docs element style
*/
function revDocsExtractElements pText
   # Get the element-matching regex
   local tElementRegex
   put __elementRegex() into tElementRegex
   
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
   
   repeat for each line tLine in pText
      # Variables to store regex matches
      local tNextElementName, tParamType, tAfterElement
      
      get word 1 of tLine
      if tInArrayDescription then
         # Keep track of array depth
         if it begins with "{" then
            add 1 to tArrayDepth
            put tLine & CR after tAccumulatedContent
            next repeat
         end if
         if word -1 of tLine ends with "}" then
            subtract 1 from tArrayDepth
            put tLine & CR after tAccumulatedContent
            if tArrayDepth <= 0 then
               put false into tInArrayDescription
            end if
            next repeat
         end if
      end if
      if tArrayDepth > 0 then
         put tLine & CR after tAccumulatedContent
         next repeat
      end if
      
      # If this matches the element definition regex, flush the accumulated content to the elements array
      # otherwise just append to the accumulated content
      
      if word 1 of tLine is empty then
         next repeat
      else if matchText(tLine, tElementRegex, tNextElementName, tParamType, tAfterElement) then        
         if tElementCount is not 0 then
            put tElementA into tElementsA[tElementCount]
            delete the last char of tAccumulatedContent
            put tAccumulatedContent into tElementsA[tElementCount]["content"]
         end if
         add 1 to tElementCount
         
         put empty into tAccumulatedContent
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
            put word 1 to -1 of tAfterElement & CR into tAccumulatedContent
         end if
      else
         put tLine & CR after tAccumulatedContent
      end if
   end repeat
   
   # Add any remaining text to the last element
   put tElementA into tElementsA[tElementCount]
   put tAccumulatedContent into tElementsA[tElementCount]["content"]
   
   return tElementsA
end revDocsExtractElements

private command __nameToKeyAndType pName, @rKey, @rType
   put pName into rKey
   switch pName
      case "Summary"
      case "Type"
      case "Description"
      case "Changes"
      case "Introduced"
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
         put "references" into rType
         break
      case "Tag"
         put "tags" into rKey
         put "items" into rType
         break
      case "Name"
         put "display name" into rKey
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
/*
Summary: Iterates through the keys of <pBlocksA> parsing the "comments" element

Parameters:
pBlocksA (array): An array representing all the docs data for each handler / syntax element
{ key (string): the handler name, dictionary entry or lcb syntax name
value (array): The array of details for this element
{ key: "comments"
value (string) : the comment block associated with this element }
key: "params"
value (string): a comma-delimited list of the parameters associated with this element
}

Returns (array): An array consisting of a key for each docs element, containing its corresponding value
*/

function revDocsParseDocBlocks pBlocksA
   local tParsedBlocksA
   repeat for each key tHandler in pBlocksA
      
      if pBlocksA[tHandler]["comments"] is empty then
         next repeat
      end if
      
      # Extract the elements from the comment block
      local tExtractedA
      put revDocsExtractElements(pBlocksA[tHandler]["comments"]) into tExtractedA
      
      # Then parse them
      local tParsedA
      put revDocsParseElements(tExtractedA, pBlocksA[tHandler]) into tParsedA
      
      put tParsedA into tParsedBlocksA[tHandler]
   end repeat
   
   return tParsedBlocksA
end revDocsParseDocBlocks

function revDocsParseElements pExtractedA, pBlocksA
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
               put word 1 to -1 of tItem into tParsedA[tKey][tCount]
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
                     logError "Warning:" && tKey && "in" && pExtractedA[1]["content"] && "is not an element type, and does not match a param name."
                  else
                     # 'unparse' the element
                     put CR & tKey & ":" & tElementA["content"] after tExtraA[tLastKey]
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
            logError "Invalid type returned from nameToKeyAndType"
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
   put url("binfile:" & pFile) into tFileContents
   return revDocsFormatDocTextAsJSON(tFileContents, pLibraryName, pAuthor)
end revDocsFormatDocFileAsJSON

function revDocsFormatDocTextAsJSON pText, pLibraryName, pAuthor
   if pText is empty then
      return empty
   end if
   
   # Parse the doc text into a library array
   local tLIbraryA
   put revDocsParseDocTextToLibraryArray(pText, pLibraryName, pAuthor) into tLibraryA
   return revDocsFormatLibraryArrayAsJSON(tLibraryA)
end revDocsFormatDocTextAsJSON

function revDocsParseDocFileToLibraryArray pFile, pLibraryName, pAuthor
   local tFileContents
   put url("binfile:" & pFile) into tFileContents
   return revDocsParseDocTextToLibraryArray(tFileContents, pLibraryName, pAuthor)
end revDocsParseDocFileToLibraryArray

function revDocsParseDocTextToLibraryArray pText, pLibraryName, pAuthor
   local tLibraryA
   put revDocsParseDocText(pText) into tLibraryA
   if pLibraryName is not empty then
      put pLibraryName into tLibraryA["name"]
   end if
   put pAuthor into tLibraryA["author"]
   return tLibraryA
end revDocsParseDocTextToLibraryArray

function revDocsParseDocFile pFile
   local tFileContents
   put url("binfile:" & pFile) into tFileContents
   return revDocsParseDocText(tFileContents)
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
            exit repeat
         default
            put tElementsA[tStartNum]["content"] into tEntriesA[tEntryCount]["elements"][tEntryElementCount]["content"]
            put it into tEntriesA[tEntryCount]["elements"][tEntryElementCount]["name"]
            add 1 to tStartNum
            add 1 to tEntryElementCount
            break
         case ""
            logError "Error: docs may be empty"
            exit repeat
         default
            logError "Error:" && it && "is invalid library-level element"
            exit repeat
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

function revDocsParseDocText pText
   local tElementsA
   put revDocsExtractElements(pText) into tElementsA
   
   local tLibraryName
   local tEntriesA, tEntryCount
   put 0 into tEntryCount
   
   local tParsedA
   # Get library-level data
   local tStartNum
   put 1 into tStartNum
   repeat forever
      get tElementsA[tStartNum]["name"]
      switch it
         case "Name"
            exit repeat
         case "Library"
            put tElementsA[tStartNum]["content"] into tParsedA["name"]
            add 1 to tStartNum
            break
         case "Description"
         case "Author"
         case "Type"
            // Add top-level tags to all library entries
         case "Tags"
            put tElementsA[tStartNum]["content"] into tParsedA[it]
            add 1 to tStartNum
            break
         case ""
            logError "Error: docs may be empty"
            exit repeat
         default
            logError "Error:" && it && "is invalid library-level element"
            exit repeat
      end switch
   end repeat
   
   local tEntryElementCount
   put 0 into tEntryElementCount
   
   local tSeenAssociations
   put false into tSeenAssociations
   
   local tSeenTags
   put false into tSeenTags
   
   local tAssociationsDefault
   put "Associations" into tAssociationsDefault["name"]
   put tParsedA["name"] into tAssociationsDefault["content"]
   repeat with tElementNum = tStartNum to the number of elements in tElementsA
      add 1 to tEntryElementCount
      local tElementA
      put tElementsA[tElementNum] into tElementA
      
      if tElementA["name"] is "Name" then
         # Add the library name to the associations of this entry
         if tParsedA["name"] is not empty and tSeenAssociations is not true and tEntryCount is not 0 then
            put tAssociationsDefault into tEntriesA[tEntryCount]["elements"][tEntryElementCount]
            add 1 to tEntryElementCount
         end if
         if tParsedA["tags"] is not empty and tSeenTags is not true and tEntryCount is not 0 then
            put "Tags" into tEntriesA[tEntryCount]["elements"][tEntryElementCount]["name"]
            put tParsedA["tags"] into tEntriesA[tEntryCount]["elements"][tEntryElementCount]["content"]
            add 1 to tEntryElementCount
         end if
         add 1 to tEntryCount
         put 1 into tEntryElementCount
         put false into tSeenAssociations   
         put false into tSeenTags
      end if
      
      if tElementA["name"] is "Syntax" then
         __updateParamsFromSyntaxLine tElementA["content"], tEntriesA[tEntryCount]["params"]
      end if
      
      if tElementA["name"] is "Associations" then
         put true into tSeenAssociations
         if tParsedA["name"] is not empty and tParsedA["name"] is not among the items of tElementA["content"] then
            put tParsedA["name"] & comma before tElementA["content"]
         end if
      end if
      
      if tElementA["name"] is "Tags" and tParsedA["tags"] is not empty then
         put true into tSeenTags
         put tParsedA["tags"] & comma before tElementA["content"]
      end if
      
      put tElementA into tEntriesA[tEntryCount]["elements"][tEntryElementCount]
   end repeat
   
   // Add association and tag to the last element, if they were not found
   if tParsedA["name"] is not empty and tSeenAssociations is not true and tEntryCount is not 0 then
      put tAssociationsDefault into tEntriesA[tEntryCount]["elements"][tEntryElementCount]
      add 1 to tEntryElementCount
   end if
   if tParsedA["tags"] is not empty and tSeenTags is not true and tEntryCount is not 0 then
      put "Tags" into tEntriesA[tEntryCount]["elements"][tEntryElementCount]["name"]
      put tParsedA["tags"] into tEntriesA[tEntryCount]["elements"][tEntryElementCount]["content"]
      add 1 to tEntryElementCount
   end if
   
   // Add a library entry
   local tLibraryElements
   local tCount
   put 1 into tCount
   add 1 to tEntryCount
   repeat for each item tItem in "name,description,author,type"
      if tParsedA[tItem] is not empty then
         put tItem into tEntriesA[tEntryCount]["elements"][tCount]["name"]
         put tParsedA[tItem] into tEntriesA[tEntryCount]["elements"][tCount]["content"]
         add 1 to tCount
      end if
   end repeat
   
   repeat for each key tKey in tEntriesA
      put revDocsParseElements(tEntriesA[tKey]["elements"], tEntriesA[tKey]) into tParsedA["doc"][tKey]
   end repeat
   
   return tParsedA
end revDocsParseDocText

/*
Extracts the comment blocks that live in between functions which contain the documentation for those functions 

pScript: The script to extract comment blocks from
pObjId (optional id) : The id of the object that the script belongs to
Returns (array) : An array containing the details for the comment blocks associated with each handler

*/
function revDocsExtractDocBlocksFromScript pScript, pObjId
   local tData
   local tEntryType
   put empty into tEntryType
   
   local tInComment
   put false into tInComment
   
   local tComment, tParams, tFirstLine
   
   repeat for each line tLine in pScript
      // Check to see if this is the start of a block of code or a block of comments 
      get word 1 of tLine 
      if not tInComment and tEntryType is empty then
         if (it is "on") then 
            put "command" into tEntryType
         else if (it is "function" or it is "command") then
            put it into tEntryType
         end if
         
         if it begins with "/*" then
            // if we have a new comment block, discard the previous one
            put empty into tComment
            local tCommentOffset
            put true into tInComment
            put offset("/*", tLine) into tCommentOffset
            put char tCommentOffset + 2 to -1 of tLine into tLine
            // If there's nothing else on this line, don't add it.
            if word 1 of tLine is empty then next repeat
         end if
      end if
      
      // If we're in a handler, check to see if this is the end of a block of code.
      if tEntryType is not empty  then
         // ignore empty lines
         if word 1 of tLine is empty then
            next repeat
         end if
         // If so, then flush all the data
         if it is "end" then
            revDocsUpdateDocBlocks false, tEntryType, tFirstLine, tComment, tData
            put empty into tComment
            put empty into tFirstLine
            put empty into tEntryType
            // Otherwise gather the data from this line
         else 
            // Name, Syntax & Params all on the same line for non-syntax
            if tFirstLine is empty then
               put tLine into tFirstLine
            end if
         end if
      end if
      // If we're in comments check to see if this is the end of a block of comments
      if tInComment then
         if word -1 of tLine ends with "*/" then
            put false into tInComment
            put offset("*/", tLine) into tCommentOffset
            put char 1 to tCommentOffset - 1 of tLine into tLine
            // If there's nothing else on this line, don't add it.
            if word 1 of tLine is empty then
               next repeat
            end if
         else if tComment is empty and word 1 of tLine is empty then
            next repeat
         end if
         put tLine & CR after tComment
      end if
      
   end repeat
   return tData
end revDocsExtractDocBlocksFromScript

/* 
Summary: Parses a script's documentation into array format

pScript (string): The script to parse
pObjId (optional id) : The id of the object that the script belongs to
Returns (array): An array representing all the information contained in the inline docs

*/
function revDocsParseDocsFromScript pScript, pObjId
   local tLibraryA
   put revDocsExtractDocBlocksFromScript(pScript, pObjId) into tLibraryA
   
   return revDocsParseDocBlocks(tLibraryA["data"])
end revDocsParseDocsFromScript

function revDocsParseDocsFromID pObjId
   return revDocsParseDocsFromScript(the script of pObjId, pObjId)
end revDocsParseDocsFromID

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
         put replaceText(pLine, "< *" & tParam & " *: *" & tType & " *>", formatSyntaxLine(pPhrases[tType], pPhrases)) into pLine
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

command revDocsUpdateModularSyntaxDocBlocks pType, pNameLine, pSyntaxLine, pBindingLines, pComment, pHandlersA, pPhrases, @xDataA
   local tEntryName
   put word 2 of pNameLine into tEntryName
   
   put formatSyntaxLine(pSyntaxLine, pPhrases) into xDataA[tEntryName]["syntax"][1]
   
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
   repeat for each line tLine in pBindingLines
      put pHandlersA[token 1 of tLine] into tHandler
      if tHandler is empty then 
         next repeat
      end if
      // extract from brackets
      local tHandlerParams, tBindingParams
      local tHandlerResult, tHandlerResult2
      get matchText(tHandler, "\((.*)\)(?: as (\w*)(?: (\w*))?)?", tHandlerParams, tHandlerResult, tHandlerResult2)
      get matchText(tLine, "\((.*)\)", tBindingParams)
      // get types
      local tItemNum, tParamName

      repeat with tItemNum = 1 to the number of items in tHandlerParams
         put item tItemNum of tHandlerParams into tItem
         put word 1 of item tItemNum of tBindingParams into tParamName
         if tParamName is "output" then
            put word 4 to -1 of tItem into tParamTypes["return value"]["type"]
         else
            get the number of elements of tParamTypes + 1
            put tParamName into tParamTypes[it]["name"]
            put word 4 to -1 of tItem into tParamTypes[it]["type"]
            put word 1 of tItem into tParamTypes[it]["mode"]
         end if
      end repeat   
      if tHandlerResult is not "undefined" then
         if tHandlerResult is "optional" then
            put " " & tHandlerResult2 after tHandlerResult
         end if
         put tHandlerResult into tParamTypes["the result"]["type"]
      end if
      put tParamTypes into xDataA[tEntryName]["variants"][tCount]
      add 1 to tCount
   end repeat
   
   put char 1 to offset(" with precedence", word 4 to -1 of pNameLine) -1 of word 4 to -1 of pNameLine into xDataA[tEntryName]["type"]
   put word -1 of xDataA[tEntryName]["type"] into xDataA[tEntryName]["type"]
   
   // Output the comments that preceded this function definition
   put pComment into xDataA[tEntryName]["comments"]
   put "syntax" into xDataA[tEntryName]["kind"]
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
   
   # Output the comments that preceded this function definition
   put pComment into xDataA[tEntryName]["comments"]
   # Distinguish from syntax
   put "handler" into xDataA[tEntryName]["kind"]
end revDocsUpdateDocBlocks

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
   
   local tElementsA
   put revDocsExtractElements(line 1 to 2 of pToParse) into tElementsA
   
   // flush out key value pair into tArray
   if word 1 of tElementsA[1]["type"] is "optional" then
      put true into tOptional
      put word 2 to -1 of tElementsA[1]["type"] into tElementsA[1]["type"]
   end if
   put tElementsA[1]["type"] into tArray["key"]["type"]
   put tElementsA[1]["content"] into tArray["key"]["description"]
   put tOptional into tArray["key"]["optional"]
   
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
   put word 1 to -1 of (char 1 to it - 1 of pDescription) & CR into tOutput
   
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
      else
         put tLine & CR after tOutput
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

function revDocsFormatLibraryArrayAsJSON pLibraryA
   local tJSON
   put "{" after tJSON
   put CR & __tabs(2) & keyValue("library",pLibraryA["name"]) & comma after tJSON
   put CR & __tabs(2) & keyValue("author",pLibraryA["author"]) & comma after tJSON
   put CR & __tabs(2) & escape("doc") & ":[" after tJSON
   
   local tCount
   put the number of elements in pLibraryA["doc"] into tCount
   
   if tCount is not 0 then
      repeat with tID = 1 to tCount
         put revDocsFormatLibraryDataAsJSON(tID, pLibraryA["doc"][tID]) & comma after tJSON
      end repeat
      delete the last char of tJSON
   end if
   put "]" after tJSON
   put CR & tab & "}" after tJSON
   return tJSON
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
   put CR & __tabs(pDepth) after tPrefix
   put escape("array") & ":[" after tOutput
   repeat for each element tKeyValue in pParamA
      put "{" after tOutput
      put tPrefix & tab & escape("key") & ": {"  after tOutput
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
   put CR & __tabs(4) into tPrefix
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
   put CR & __tabs(pDepth) after tPrefix
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
      put CR & __tabs(3) & "}" & comma after tOutput
   end repeat
   delete the last char of tOutput
   put "]" after tOutput
   return tOutput
end __revDocsFormatParamArrayAsJSON

function __revDocsFormatValuesArrayAsJSON pParamA
   local tOutput
   put "[" into tOutput 
   put pParamA
   local tReturnTypes
   put "return,value,it,the result" into tReturnTypes
   repeat for each item tReturn in tReturnTypes
      if pParamA[tReturn] is empty then
         next repeat
      end if
      put "{" after tOutput
      put tReturn into pParamA[tReturn]["name"]
      put revDocsTypedParamToJSON(pParamA[tReturn], 4, false) after tOutput
      put CR & __tabs(3) & "}" & comma after tOutput
   end repeat
   delete the last char of tOutput
   put "]" after tOutput
   return tOutput
end __revDocsFormatValuesArrayAsJSON

function __revDocsFormatExamplesArrayAsJSON pExampleA
   local tOutput
   put "[" into tOutput 
   repeat for each element tExample in pExampleA
      put "{" & CR & __tabs(4) & keyValue("script", tExample) after tOutput
      put CR & __tabs(3) & "}" & comma after tOutput
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
   
   return __tabs(pTabs) & escape(pKey) & ":[" & tTagList & "]" & comma & CR
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
      put CR & __tabs(4) & escape(__formatParamsAsItalic(tExample)) & comma after tOutput
   end repeat
   delete the last char of tOutput
   put CR & __tabs(3) & "]" after tOutput
   return tOutput
end __revDocsFormatDisplaySyntaxArrayAsJSON

function __revDocsFormatSyntaxArrayAsJSON pSyntaxA
   local tOutput
   put "[" into tOutput 
   repeat for each element tExample in pSyntaxA
      put CR & __tabs(4) & escape(tExample) & comma after tOutput
   end repeat
   delete the last char of tOutput
   put CR & __tabs(3) & "]" after tOutput
   return tOutput
end __revDocsFormatSyntaxArrayAsJSON

private function keyValue pKey, pElement
   return escape(tolower(pKey)) & ":" & escape(pElement)
end keyValue

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
   put keyValue(pElement, pValue) & comma & CR after tOutput
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
   put tab & tab & tab & escape("references") & ":{" & CR into tOutput
   repeat for each key tKey in pReferencesA
      get revDocsItemListToJSONArray(pReferencesA, tKey, 4)
      put it after tOutput
   end repeat
   delete char -2 to -1 of tOutput
   put CR & tab & tab & tab & "}," & CR after tOutput
   return tOutput
end __revDocsFormatReferencesArrayAsJSON

function __revDocsFormatCustomArrayAsJSON pKey, pArray
   if pArray[pKey] is empty then
      return empty
   end if
   switch pKey
      case "syntax"
         return tab & tab & tab & escape("syntax") & ":" & __revDocsFormatSyntaxArrayAsJSON(pArray["syntax"]) & comma & CR
      case "display syntax"
         return tab & tab & tab & escape("display syntax") & ":" & __revDocsFormatDisplaySyntaxArrayAsJSON(pArray["display syntax"]) & comma & CR
      case "params"
         return tab & tab & tab & escape("parameters") & ":" & __revDocsFormatParamArrayAsJSON(pArray["params"]) & comma & CR
      case "example"
         return tab & tab & tab & escape("examples") & ":" & __revDocsFormatExamplesArrayAsJSON(pArray["example"]) & comma & CR 
      case "value"
         return tab & tab & tab & escape("value") & ":" & __revDocsFormatValuesArrayAsJSON(pArray["value"]) & comma & CR
      default
         logError "invalid value " & pKey & "passed to __revDocsFormatCustomArrayAsJSON"
         return empty
   end switch
end __revDocsFormatCustomArrayAsJSON

/* 
Format the library array data as a JSON array as needed by the docs viewer
*/

function revDocsFormatLibraryDataAsJSON pID, pLibraryDataA
   local tJSON
   put "{" & CR into tJSON
   
   if pLibraryDataA["display name"] is empty then
      put pLibraryDataA["name"] into pLibraryDataA["display name"]
   else if pLibraryDataA["name"] is empty then
      put pLibraryDataA["display name"] into pLibraryDataA["name"]
   else
      logError "Error: no name parsed"
   end if
   
   # ID
   put outputElement("id", pID, 3) after tJSON
   
   # Name
   put outputArrayElement("name", pLibraryDataA, 3) after tJSON
   
   # Display Name
   put outputArrayElement("display name", pLibraryDataA, 3) after tJSON
   
   # Type
   put outputArrayElement("type", pLibraryDataA, 3, true) after tJSON
   
   # Syntax
   put __revDocsFormatCustomArrayAsJSON("syntax", pLibraryDataA) after tJSON
   
   # Display syntax
   if pLibraryDataA["syntax"] is not empty then
      put line 1 of pLibraryDataA["syntax"][1] into pLibraryDataA["display syntax"][1]
   else
      put pLibraryDataA["display name"] into pLibraryDataA["display syntax"][1]
   end if
   put __revDocsFormatCustomArrayAsJSON("display syntax", pLibraryDataA) after tJSON
   
   # Synonyms
   put revDocsItemListToJSONArray(pLibraryDataA, "synonyms") after tJSON
   
   # Associations
   put revDocsItemListToJSONArray(pLibraryDataA, "associations") after tJSON
   
   # Summary
   put outputElementIfExists("summary", pLibraryDataA, 3) after tJSON
   
   # Introduced
   put outputElementIfExists("introduced", pLibraryDataA, 3) after tJSON
   
   # OS
   put revDocsItemListToJSONArray(pLibraryDataA, "OS") after tJSON
   
   # Platforms
   put revDocsItemListToJSONArray(pLibraryDataA, "platforms") after tJSON
   
   # Security
   put revDocsItemListToJSONArray(pLibraryDataA, "security") after tJSON
   
   # Parameters
   put __revDocsFormatCustomArrayAsJSON("params", pLibraryDataA) after tJSON
   
   # Example
   put __revDocsFormatCustomArrayAsJSON("example", pLibraryDataA) after tJSON
   
   # Value
   put __revDocsFormatCustomArrayAsJSON("value", pLibraryDataA) after tJSON
   
   # Description
   put outputElementIfExists("description", pLibraryDataA, 3) after tJSON
   
   # References
   put __revDocsFormatReferencesArrayAsJSON(pLibraryDataA["references"]) after tJSON
   
   # Tags
   put revDocsItemListToJSONArray(pLibraryDataA, "tags") after tJSON
   
   # Changes
   put outputElementIfExists("changes", pLibraryDataA, 3) after tJSON
   
   delete char -2 to -1 of tJSON
   put CR & tab & tab & "}" after tJSON
   return tJSON
   
end revDocsFormatLibraryDataAsJSON
