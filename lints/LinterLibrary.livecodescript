script "LinterLibrary"
-- Array of linting rules
local sLinterRules

----------------------------------------------------------------
-- Library management
----------------------------------------------------------------

on revLoadLibrary
   if the target is me then
      insert the script of me into back
   end if
end revLoadLibrary

on revUnloadLibrary
   if the target is me then
      remove the script of me from back
   end if
end revUnloadLibrary

----------------------------------------------------------------
-- Utilities
----------------------------------------------------------------

private function _GetFolderFiles pFolder
   local tFilesList

   put files(pFolder) into tFilesList
   assert the result is not empty

   split tFilesList with return
   return tFilesList
end _GetFolderFiles

private function _GetFolderSubFolders pFolder
   local tFoldersList

   put folders(pFolder) into tFoldersList
   assert the result is not empty

   filter tFoldersList without ".."
   split tFoldersList with return
   return tFoldersList   
end _GetFolderSubFolders

private function _IsAbsolutePath pPath
   if pPath begins with "/" then
      return true
   end if
   
   if the platform is "Win32" then
      if matchText(pPath, "^[a-zA-Z]:/") then
         return true
      end if
   end if
   
   return false
end _IsAbsolutePath

private function _GetAbsolutePath pPath
   if _IsAbsolutePath(pPath) then
      return pPath
   end if
   
   return the defaultFolder & slash & pPath
end _GetAbsolutePath

private function _GetFileContents pFile
   local tContents
   put url ("binfile:" & pFile) into tContents

   -- FIXME proper error handling
   if the result is not empty then
      throw "Failed to open file" & return & \
            pFile & return & the result
   end if

   return tContents
end _GetFileContents

----------------------------------------------------------------
-- Linter state management
----------------------------------------------------------------

private function _GetLintLibrary
   return the long id of stack "LintLibrary"
end _GetLintLibrary

private command _Reset
   local tStack
   put _GetLintLibrary() into tStack
   
   set the cLocation of tStack to empty
   set the cContents of tStack to empty
   set the cInterpretation of tStack to empty
   set the cIsPermissive of tStack to false
   set the cIsFixEnabled of tStack to false
   
   set the cMessages of tStack to empty
   set the cIsModified of tStack to false
end _Reset

private command _SetTarget pPath, pInterpretation, pContents
   local tStack, tLocation
   put _GetLintLibrary() into tStack
   
   put pPath into tLocation[1]
   set the cLocation of tStack to tLocation
   
   set the cInterpretation of tStack to pInterpretation
   set the cContents of tStack to pContents
   set the cIsModified of tStack to false
end _SetTarget

private command _SetPermissive pIsPermissive
   set the cIsPermissive of _GetLintLibrary() to pIsPermissive
end _SetPermissive

private command _SetFixEnabled pIsFixEnabled
   set the cIsFixEnabled of _GetLintLibrary() to pIsFixEnabled
end _SetFixEnabled

on LinterAddMessageCallback pTarget, pHandler
   local tCallback, tCallbacks, tStack
   put pTarget into tCallback["target"]
   put pHandler into tCallback["handler"]
   
   put _GetLintLibrary() into tStack
   put the cMessageCallbacks of tStack into tCallbacks
   put tCallback into tCallbacks[1 + the number of elements in tCallbacks]
   set the cMessageCallbacks of tStack to tCallbacks
end LinterAddMessageCallback

function LinterGetMessages
   return the cMessages of _GetLintLibrary()
end LinterGetMessages

private command _LogMessage pType, pMessage
   local tCallback
   repeat for each element tCallback in \
         the cMessageCallbacks of _GetLintLibrary()
      dispatch tCallback["handler"] \
            to tCallback["target"] \
            with pType, empty, pMessage, false
   end repeat
end _LogMessage

----------------------------------------------------------------
-- Scan implementation
----------------------------------------------------------------

private function _IsRuleApplicable pRule, pFilePath
   if "include" is among the keys of pRule and \
         not matchText(pFilePath, pRule["include"]) then
      return false
   end if
   if "exclude" is among the keys of pRule and \
         matchText(pFilePath, pRule["exclude"]) then
      return false
   end if
   return true
end _IsRuleApplicable

private command _PrepareBinary pFilePath, pInterpretation
   local tBinary
   put _GetFileContents(pFilePath) into tBinary
   _SetTarget pFilePath, pInterpretation, tBinary
end _PrepareBinary

private command _PrepareText pFilePath, pInterpretation
   local tEncoding, tBinary, tText

   -- Get text encoding, defaulting to UTF-8
   put word 2 of pInterpretation into tEncoding
   if tEncoding is empty then
      put "utf-8" into tEncoding
   end if

   put _GetFileContents(pFilePath) into tBinary
   put textDecode(tBinary, tEncoding) into tText

   _SetTarget pFilePath, pInterpretation, tText

   -- Validate encoding
   if textEncode(tText, tEncoding) is not tBinary then
      LintProblem merge("Invalid [[tEncoding]] text")
   end if
end _PrepareText

private command _PrepareFile pFilePath, pRule
   local tInterpretation
   if "interpretation" is among the keys of pRule then
      put pRule["interpretation"] into tInterpretation
   else
      put "binary" into tInterpretation
   end if
   
   switch word 1 of tInterpretation
      case "binary"
         _PrepareBinary pFilePath, tInterpretation
         break
      case "text"
         _PrepareText pFilePath, tInterpretation
         break
      default
         -- FIXME handle unrecognized interpretation
         break
   end switch
end _PrepareFile

private command _DispatchLints pRule
   _SetPermissive (pRule["permissive"] is true)
   
   repeat for each element tLintName in pRule["lints"]
      -- FIXME check the dispatch result
      set the cCurrentLint of _GetLintLibrary() to tLintName
      dispatch "Lint" to stack tLintName
      if it is not handled then
         throw merge("Failed to run lint [[tLintName]]")
      end if
   end repeat
   set the cCurrentLint of _GetLintLibrary() to empty
end _DispatchLints

private command _ScanFile pFilePath
   local tAbsolutePath, tRule
   put _GetAbsolutePath(pFilePath) into tAbsolutePath
      
   repeat for each element tRule in sLinterRules
      if _IsRuleApplicable(tRule, tAbsolutePath) then
         _PrepareFile tAbsolutePath, tRule
         _DispatchLints tRule
      end if
   end repeat
end _ScanFile

-- Recurse over directories
private command _ScanFolder pFolderPath
   local tFile, tFolder
   repeat while pFolderPath ends with "/"
      delete char -1 of pFolderPath
   end repeat

   repeat for each element tFile in _GetFolderFiles(pFolderPath)
      put pFolderPath & slash before tFile
      _ScanFile tFile
   end repeat
   repeat for each element tFolder in _GetFolderSubfolders(pFolderPath)
      if tFolder begins with "." then
         next repeat
      end if
      put pFolderPath & slash before tFolder
      _ScanFolder tFolder
   end repeat
end _ScanFolder

-- Scan a path without knowing whether it's a file or a folder
private command _ScanPath pPath
   if there is a file pPath then
      _ScanFile pPath
   else if there is a folder pPath then
      _ScanFolder pPath
   else
      -- FIXME do something if the path doesn't exist?
   end if
end _ScanPath

----------------------------------------------------------------
-- Accessors
----------------------------------------------------------------

-- Return the folder containing this library.  This is where all
-- the rest of the lints should live.
function LinterGetBaseFolder
   local tFolder
   set the itemDelimiter to slash
   put item 1 to -2 of the filename of this me into tFolder
   if tFolder is empty then
      return the defaultFolder
   else
      return tFolder
   end if
end LinterGetBaseFolder

-- Return all stacks that appear to be lints.
--
-- Any open stack that has a name ending with "lint" is
-- considered to be a lint.
function LinterGetLintStacks
   get the openStacks
   filter it with regex pattern "[lL]int$"
   split it with return
   return it
end LinterGetLintStacks

----------------------------------------------------------------
-- High-level commands
----------------------------------------------------------------

-- Try to load the lint stacks.
command LinterLoadLints
   local tFolder, tFiles, tFile
   put LinterGetBaseFolder() into tFolder
   
   put _GetFolderFiles(tFolder) into tFiles
   
   repeat for each element tFile in tFiles
      if char 1 of tFile is in "._" then
         next repeat
      end if
      if not (tFile ends with ".livecodescript") then
         next repeat
      end if
      
      put tFolder & slash before tFile
      
      -- Ensure file is loaded
      get the name of stack tFile
   end repeat
end LinterLoadLints

-- Try to load the lint rules
command LinterLoadRules
   local tRulesPath, tJson, tError
   put LinterGetBaseFolder() & slash & "rules.json" into tRulesPath
   
   put textdecode(url ("binfile:" & tRulesPath), "utf-8") into tJson
   put JSONImport(tJson) into sLinterRules
end LinterLoadRules

-- Scan a file, or all files found within a directory
-- (recursively)
command LinterScanPath pPath, pFix
   LinterLoadLints
   LinterLoadRules
   
   dispatch "revLoadLibrary" to stack "LintLibrary"
   _Reset
   _SetFixEnabled (pFix is true)

   if pPath is empty then
      put the defaultFolder into pPath
   end if
   _ScanPath pPath
   
   return LinterGetMessages()
end LinterScanPath
