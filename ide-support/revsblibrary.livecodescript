script "revSBLibrary"
#?library
# Standalone Builder Library functions
#?author
# Marcus van Houdt

on extensionInitialize
   if the target is me then
      insert the script of me into back
      
      __InitialiseScriptLibraries
      __InitialiseInclusionDependencies
      __InitialiseBuiltInInclusions
   end if
end extensionInitialize

on extensionFinalize
   if the target is me then
      remove the script of me from back
   end if
end extensionFinalize

local sScriptLibraries

-- Some higher level wrappers..

#?semantics
# Copies the file pSourceFile to the folder pTargetFolder (which is not created if it does not exist).
#?author
# Marcus van Houdt
on revSBCopyFileToFolder pSourceFile, pTargetFolder, pCallback, pCallbackTarget
   if there is no folder pTargetFolder then
      return "target folder not found"
   end if
   set the itemDelimiter to "/"
   local tTargetFile
   put pTargetFolder & "/" & item -1 of pSourceFile into tTargetFile
   __revSBCopyFile pSourceFile, tTargetFile, pCallback, pCallbackTarget
   return the result
end revSBCopyFileToFolder

on revSBCopyFile pSourceFile, pTargetFile, pCallback, pCallbackTarget
   __revSBCopyFile pSourceFile, pTargetFile, pCallback, pCallbackTarget
   return the result
end revSBCopyFile

on revSBCopyFileToFile pSourceFile, pTargetFile, pCallback, pCallbackTarget
   __revSBCopyFile pSourceFile, pTargetFile, pCallback, pCallbackTarget
   return the result
end revSBCopyFileToFile

on revSBCopyFolder pSource, pTarget, pCallback, pCallbackTarget
   __revSBCopyFolder pSource, pTarget, pCallback, pCallbackTarget
   return the result
end revSBCopyFolder

# Slightly unfortunate naming, perhaps, but what do I call it folderToFolder?!
on revSBCopyFolderToDestination pSourceFolder, pDestinationFolder, pCallback, pCallbackTarget, pSettings
   if there is no folder pDestinationFolder then
      return "destination folder not found"
   end if
   set the itemDelimiter to "/"
   local tTargetFolder
   put pDestinationFolder & "/" & item -1 of pSourceFolder into tTargetFolder
   __revSBCopyFolder pSourceFolder, tTargetFolder, pCallback, pCallbackTarget, pSettings
   return the result
end revSBCopyFolderToDestination

on revSBReadFile pFile, pMode, @rData, pCallback, pCallbackTarget
   local tResult
   
   if tResult is empty then
      if there is no file pFile then
         put "file not found" into tResult
      end if
   end if
   
   local tTotalSize
   if tResult is empty then
      local tOldFolder
      put the defaultFolder into tOldFolder
      set the itemDelimiter to "/"
      set the defaultFolder to item 1 to -2 of pFile
      get the detailed files
      filter it with (the urlEncode of item -1 of pFile) & "*"
      set the itemDelimiter to ","
      put item 2 of it into tTotalSize
      set the defaultFolder to tOldFolder
   end if
   
   if tResult is empty then
      if pMode is "binary" then
         open file pFile for binary read
      else
         open file pFile for text read
      end if
   end if
   put the result into tResult
   
   local tAmountRead
   if tResult is empty then
      if pCallbackTarget is not empty and there is a pCallbackTarget then
         send pCallback to pCallbackTarget
      end if
      repeat forever
         read from file pFile for 524288
         if the result is not empty and the result is not "eof" then
            put the result into tResult
            exit repeat
         end if
         put it after rData
         if the result is "eof" or it is empty then
            exit repeat
         end if
         add 524288 to tAmountRead
         if pCallbackTarget is not empty and there is a pCallbackTarget then
            send pCallback && tAmountRead, tTotalSize to pCallbackTarget
         end if
      end repeat
   end if
   if tResult is empty then
      if pCallbackTarget is not empty and there is a pCallbackTarget then
         send pCallback && tAmountRead, tTotalSize to pCallbackTarget
      end if
   end if
   
   if tResult is empty then
      close file pFile
   end if
   
   return tResult
end revSBReadFile


local sLog = "false"

private command log pMessage
   #put pMessage & return after msg
end log



# This handler has not been tested..
on revSBWriteFile pFile, pMode, @pData, pCallback, pCallbackTarget, pLog
   local tResult
   
   put pLog into sLog
   
   if tResult is empty then
      if "binary" is in pMode then
         if "update" is in pMode then
            open file pFile for binary update
         else
            open file pFile for binary write
         end if
      else
         if "update" is in pMode then
            open file pFile for text update
         else
            open file pFile for text write
         end if
      end if
   end if
   put the result into tResult
   
   log "Result of opening file: " & tResult
   
   local tTotalSize
   local tCurrentCount
   if tResult is empty then
      put the number of chars of pData into tTotalSize
      put 1 into tCurrentCount
      if pCallbackTarget is not empty and there is a pCallbackTarget then
         send pCallback to pCallbackTarget
      end if
      repeat forever
         get char tCurrentCount to (tCurrentCount + 524288 - 1) of pData
         if it is empty then
            exit repeat
         end if
         write it to file pFile
         if the result is not empty then
            put the result into tResult
            exit repeat
         end if
         add 524288 to tCurrentCount
         if pCallbackTarget is not empty and there is a pCallbackTarget then
            send pCallback && tCurrentCount, tTotalSize to pCallbackTarget
         end if
      end repeat
   end if
   if tResult is not empty then
      if pCallbackTarget is not empty and there is a pCallbackTarget then
         send pCallback && tTotalSize, tTotalSize to pCallbackTarget
      end if
   end if
   
   if tResult is empty then
      close file pFile
   end if
   
   return tResult
end revSBWriteFile


---

#?semantics
# Copies a folder and all it's contents, under which we understand:
# - (sub)folders
# - (ordinary) files (including finder shortcuts etc)
# - bsd symlinks (TO DO)
#?author
# Marcus van Houdt
private command __revSBCopyFolder pSource, pTarget, pCallback, pCallbackTarget, pSettings
   local tResourceList
   local tResult
   
   put revSBReadLink(pSource) into pSource
   
   local tOldFolder
   put the defaultfolder into tOldFolder
   if there is no folder pSource then
      put "no such source folder found" into tResult
   end if
   
   # OK-2008-09-10 : Bug 7147
   set the defaultFolder to pSource
   if the defaultFolder is not pSource then
      put "can't enter source folder" into tResult
   end if
   
   local tTotalNumber
   put __EnumerateFolder(pSource) into tResourceList
   put the number of lines of tResourceList into tTotalNumber
   
   if there is no folder pTarget then
      __revSBEnsureFolder pTarget -- create it and all its parents as required...
      put the result into tResult
   end if
   
   local tNumberCopied
   put 0 into tNumberCopied
   if tResult is empty then
      if pCallbackTarget is not empty and there is a pCallbackTarget then
         send pCallback to pCallbackTarget -- with empty paramaters..
      end if
      repeat for each line tResource in tResourceList
         add 1 to tNumberCopied
         if char -1 of tResource is "/" then
            -- create the folder, stat the source
            local tSubFolder
            put pTarget & "/" & char 1 to -2 of tResource into tSubFolder
            if there is not a folder tSubFolder then
               create folder tSubFolder
               if the result is not empty then
                  put "cannot create subfolder ", the result , tResource into tResult
                  exit repeat
               end if
            end if
            if the platform is "macos" or the platform is "Linux" then -- stat it and set some flags
               local tUser, tGroup
               set the itemDelimiter to "/"
               set the defaultFolder to pSource & "/" & item 1 to -2 of tResource
               get the detailed folders
               filter it with (the urlEncode of item -1 of tResource) & "*"
               set the itemDelimiter to ","
               put item 8 of it into tUser
               put item 9 of it into tGroup
               if tUser is not empty or tGroup is not empty then
                  get shell("chown" && tUser & ":" & tGroup && quote & pTarget & "/" & char 1 to -2 of tResource & quote)
               end if
            end if
            # Creating a folder takes no time, so do not send a callback.
         else if tResource is not empty then
            local tFileCallback
            put pCallback && tNumberCopied, tTotalNumber & comma into tFileCallback
            __revSBCopyFile pSource & "/" & tResource, pTarget & "/" & tResource, tFileCallback, pCallbackTarget, pSettings
            if the result is not empty then
               put the result into tResult
               exit repeat
            end if
         end if
         -- Send the callback ones every 50 items, say, if for some reason no files were encountered..
         if tNumberCopied mod 50 is 0 and pCallbackTarget is not empty and there is a pCallbackTarget then
            send pCallback && tNumberCopied, tTotalNumber to pCallbackTarget
         end if
      end repeat
   end if
   
   -- make sure we finish on a "full" callback
   if tResult is empty then
      if pCallbackTarget is not empty and there is a pCallbackTarget then
         send pCallback && tTotalNumber, tTotalNumber to pCallbackTarget
      end if
   end if
   
   set the defaultFolder to tOldFolder
   
   if tResult is not empty then
      revStandaloneAddWarning tResult & ":" && quote & pSource & quote
   end if
   
   return tResult
end __revSBCopyFolder

#?semantics
# Copy a file and it's attributes for as much as we can.
# Sends a callback with the number of bytes currently copied and the number left to copy..
#?author
# Marcus van Houdt
private command __revSBCopyFile pSourceFile, pTargetFile, pCallback, pCallbackTarget, pSettings
   -- Stat the source file
   local tOldFolder, tSize, tPermissions, tUser, tGroup, tFileType
   local tResult
   
   local tSourceFileIsLink
   if tResult is empty then
      if __readlink(pSourceFile) is not empty then
         put true into tSourceFileIsLink
      else if there is no file pSourceFile then
         put "cannot find source file" into tResult
      end if
   end if
   
   if tResult is empty then
      put the defaultFolder into tOldFolder
      set the itemDelimiter to "/"
      set the defaultFolder to item 1 to -2 of pSourceFile
      
      get the detailed files
      filter it with (the urlEncode of item -1 of pSourceFile) & "*"
      set the itemDelimiter to ","
      put item 2 of it into tSize
      put item 8 of it into tUser
      put item 9 of it into tGroup
      put item 10 of it into tPermissions
      put item 11 of it into tFileType
      
      if there is a file pTargetFile then
         delete file pTargetFile
         if the result is not empty then
            put "cannot overwrite existing target file" into tResult
         end if
      end if
      
      local tOldFileType
      if tResult is empty then
         put the fileType into tOldFileType
         if tFileType is not empty then
            set the fileType to tFileType
         else
            set the fileType to "????????"
         end if
      end if
   end if
   
   if tSourceFileIsLink then
      if tResult is empty then
         # __readlink only works for MacOS and Linux so no need for a switch here
         get shell("cp -a" && quote & pSourceFile & quote && \
               quote & pTargetfile & quote)
         if it is not empty then
            put "cannot copy symbolic link" into tResult
         end if
         # Copying a symbolic link takes no time, so do not send a callback.
      end if
   else # regular file
      if tResult is empty then
         open file pTargetFile for binary write
         if the result is not empty then
            put "cannot create target file" into tResult
         end if
      end if
      
      if tResult is empty then
         open file pSourceFile for binary read
         if the result is not empty then
            put "cannot read from source file", the result into tResult
         end if
      end if
      
      local tAmountCopied
      if tResult is empty then
         if pCallbackTarget is not empty and there is a pCallbackTarget then
            send pCallback && 0, tSize to pCallbackTarget
         end if
         repeat forever
            read from file pSourceFile for 1048576
            if the result is not empty and the result is not "eof" then
               put "cannot read from source file", the result into tResult
               exit repeat
            end if
            if it is empty then
               exit repeat
            end if
            write it to file pTargetFile
            if the result is not empty then
               put "cannot write to target file" into tResult
               exit repeat
            end if
            add the number of chars of it to tAmountCopied
            if pCallbackTarget is not empty and there is a pCallbackTarget then
               send pCallback && tAmountCopied, tSize to pCallbackTarget
            end if
         end repeat
      end if
      
      if pTargetFile is among the lines of the openFiles then
         close file pTargetFile
      end if
      if pSourceFile is among the lines of the openFiles then
         close file pSourceFile
      end if
   end if
   
   # OK-2007-11-30 : Modified to work for Linux too
   if tResult is empty and the platform is "macos" or the platform is "Linux" then
      if tPermissions is not empty then
         get shell("chmod" && tPermissions && quote & pTargetfile & quote)
      end if
      if tUser is not empty or tGroup is not empty then
         get shell("chown" && tUser & ":" & tGroup && quote & pTargetFile & quote)
      end if
   end if
   
   -- Remove unneeded slices from the MacOS externals
   set the itemDel to slash
   if pSettings["externals"] contains item -1 of pTargetFile then
      revSBRemoveUnneededSlicesFromMacCode pTargetFile, pSettings
   end if
   
   if tOldFolder is not empty then
      set the defaultFolder to tOldFolder
   end if
   if tOldFileType is not empty then
      set the fileType to tOldFileType
   end if
   
   /* WORKAROUND: BUG 22255 
    *
    * Multiple shell calls in a tight loop can cause a deadlock on macOS */
   wait 0 with messages
   
   return tResult
end __revSBCopyFile


#?semantics
# Creates a folder and all parents as required.
#?author
# Marcus van Houdt
on revSBEnsureFolder pFolder
   __revSBEnsureFolder pFolder
   return the result
end revSBEnsureFolder

command revSBRemoveUnneededSlicesFromMacCode pTargetFile, pSettings
   if the platform is "macos" and pSettings is not empty then
      local tRemove
      if pSettings["MacOSX x86-64"] is true and pSettings["MacOSX x86-32"] is not true then
         put "i386" into tRemove
      else if pSettings["MacOSX x86-32"] is true and pSettings["MacOSX x86-64"] is not true then
         put "x86_64" into tRemove
      end if
      
      if tRemove is not empty then
         replace " " with "\ " in pTargetFile
         local tShell
         put "lipo " & pTargetFile & " -remove " & tRemove & " -output " & pTargetFile into tShell
         get shell(tShell)
      end if
   end if
   
end revSBRemoveUnneededSlicesFromMacCode

private command __revSBEnsureFolder pFolder
   set the itemDel to "/"
   if there is no folder pFolder and pFolder is not empty then
      __revSBEnsureFolder item 1 to -2 of pFolder
      if the result is not empty then
         return the result
      end if
      create folder pFolder
      return the result
   end if
end __revSBEnsureFolder

function revSBEnumerateFolder pFolder, pRecursive
   return __EnumerateFolder(pFolder, "", pRecursive)
end revSBEnumerateFolder

private function __EnumerateFolder pFolder, pPrefix, pRecursive
   local tRecursive
   -- recursive by default
   put pRecursive is not false into tRecursive
   
   local tResult
   local tSubFolder
   
   repeat for each line tSubFolder in folders(pFolder)
      if tSubFolder is among the items of ".,.." then
         next repeat
      end if
      # For our purposes we need the folder first, so it can be created ahead of file creation..
      appendToStringList pPrefix & tSubFolder & "/", tResult
      if tRecursive then
         get __EnumerateFolder(pFolder & "/" & tSubFolder, pPrefix & tSubFolder & "/")
         if it is not empty then
            appendToStringList it, tResult
         end if
      end if
   end repeat
   
   local tFile
   repeat for each line tFile in files(pFolder)
      appendToStringList pPrefix & tFile, tResult
   end repeat
   
   return tResult
end __EnumerateFolder

function revSBCalculateRelativePath pRoot, pFile
   local tResult, tCount
   set the itemDel to "/"
   put 1 into tCount
   local tIsFolder
   put there is a folder pFile into tIsFolder
   repeat for each item tNode in pRoot
      if item 1 to tCount of pRoot is item 1 to tCount of pFile then
      else
         put "../" before tResult
         if item tCount of pFile is not empty then
            put item tCount of pFile & "/" after tResult
         end if
      end if
      add 1 to tCount
   end repeat
   put "./" before tResult
   put item tCount to -1 of pFile after tResult
   
   if char -1 of tResult is "/" then
      delete char -1 of tResult
   end if
   
   return tResult
end revSBCalculateRelativePath

function revSBCalculateAbsolutePath pRoot, pRelativeFile
   if pRelativeFile is empty then
      return __revSBCalculateAbsolutePath(pRoot)
   else
      local tRoot
      put pRoot into tRoot
      if char -1 of tRoot is "/" then
         delete char -1 of tRoot
      end if
      if char 1 of pRelativeFile is "/" then
         return __revSBCalculateAbsolutePath(tRoot & pRelativeFile)
      else
         return __revSBCalculateAbsolutePath(tRoot & "/" & pRelativeFile)
      end if
   end if
end revSBCalculateAbsolutePath

private function __revSBCalculateAbsolutePath pPath
   local tPath
   set the itemDelimiter to "/"
   repeat for each item tPart in pPath
      switch tPart
         case "."
            next repeat
         case ".."
            delete the last item of tPath
            break
         default
            put tPart & "/" after tPath
      end switch
   end repeat
   if tPath is not "/" then
      delete the last char of tPath
   end if
   return tPath
end __revSBCalculateAbsolutePath


#?Semantics
# Returns "absolute" or "relative" depending on the type of path.
function revSBPathType pPath
   if the platform is "macos" then
      if char 1 of pPath is "/" then
         return "absolute"
      else
         return "relative"
      end if
   end if
   if the platform is "win32" then
      if char 2 of pPath is ":" then
         return "absolute"
      else
         return "relative"
      end if
   end if
end revSBPathType

function revSBGenerateUUID
   _internal call "generate_uuid"
   return the result
end revSBGenerateUUID

private function __readlink pPath
   if the platform is "Win32" then
      return empty
   end if
   
   local tLink
   put shell("readlink " & quote & pPath & quote) into tLink
   if the last char of tLink is cr then
      delete the last char of tLink
   end if
   return tLink
end __readlink

// Resolve symbolic links
private function revSBReadLink pPath
   repeat forever
      local tLink
      put __readlink(pPath) into tLink
      
      if tLink is empty then
         return pPath
      end if
      
      if the first char of tLink is not slash then
         set the itemdel to slash
         put revSBCalculateAbsolutePath(item 1 to -2 of pPath, tLink) into tLink
      end if
      
      put tLink into pPath
   end repeat
end revSBReadLink

----------

local sWarnings

command revStandaloneResetWarnings
   put empty into sWarnings
end revStandaloneResetWarnings

function revStandaloneGetWarnings
   return sWarnings
end revStandaloneGetWarnings

command revStandaloneAddWarning pWarning
   if pWarning is not among the lines of sWarnings then
      put pWarning & return after sWarnings
   end if
end revStandaloneAddWarning

----------

command revStandaloneProgress pMessage
   if not revSBSuppressDialogs() then
      send "revUpdateStandaloneProgress pMessage" to stack "revStandaloneSettings"
   end if
end revStandaloneProgress

# This is an event handler, not a command.
on revStandaloneProgressCallback
   if not revSBSuppressDialogs() then
      wait 0 ticks with messages
   end if
end revStandaloneProgressCallback

command revStandaloneProgressFinished
   if not revSBSuppressDialogs() then
      close stack "revStandaloneProgress"
   end if
end revStandaloneProgressFinished

----------

# AL-2015-01-29: [[ Scriptify revSaveAsStandalone ]] Move some general utility code into revSBLibrary
function revFolderNameIllegalChars
   return "\/:*?<>|" & quote & return
end revFolderNameIllegalChars

function revFolderNameIsIllegal pName
   repeat for each char tChar in revFolderNameIllegalChars()
      if pName contains tChar then
         return true
      end if
   end repeat
   
   # This appears to be the max length from looking at older code. There was also a limit of 
   # 27 chars on Mac, but i think this is left over from classic so removed it.
   if the length of pName > 246 then
      return true
   end if
   
   return false
end revFolderNameIsIllegal

function revFolderNameMakeLegal pName
   local tLegalName
   put pName into tLegalName
   
   repeat for each char tChar in revFolderNameIllegalChars()
      replace tChar with space in tLegalName
   end repeat
   
   return tLegalName
end revFolderNameMakeLegal

// List of all desktop target platforms + architecture
constant kDesktopTargets = "Windows,Windows x86-64,MacOSX x86-32,MacOSX x86-64,Linux,Linux x64,Linux armv6-hf"
function revSBDesktopTargets
   return kDesktopTargets
end revSBDesktopTargets

constant kAdditionalTargets = "iOS,Android,Emscripten"
function revSBAdditionalTargets
   return kAdditionalTargets
end revSBAdditionalTargets

function revSBTargetToPlatform pTarget
   if pTarget begins with "MacOSX" then
      return "MacOSX"
   else if pTarget begins with "Linux" then
      return "Linux"
   else if pTarget begins with "Windows" then
      return "Windows"
   else
      return pTarget
   end if
end revSBTargetToPlatform

-- item 1 of the architectures should be the default architecture
constant kWindowsArchitectures = "x86-32,x86-64"
constant kMacOSXArchitectures = "x86-32,x86-64"
constant kLinuxArchitectures =  "x86-32,x86-64,armv6-hf"
constant kAndroidArchitectures =  "armv7,arm64,x86,x86_64"
constant kiOSArchitectures =  "armv7,arm64"
constant kEmscriptenArchitectures =  "js"

function revSBPlatformArchitectures pPlatform
   switch pPlatform
      case "Windows"
         return kWindowsArchitectures
      case "MacOSX"
         return kMacOSXArchitectures
      case "Linux"
         return kLinuxArchitectures
      case "Android"
         return kAndroidArchitectures
      case "iOS"
         return kiOSArchitectures
      case "Emscripten"
         return kEmscriptenArchitectures
      default
         return empty
   end switch
end revSBPlatformArchitectures

private function __ArchitectureSynonyms pArchitecture
   switch pArchitecture
      case "x86-32"
         return pArchitecture,"x86,i386"
      case "x86-64"
         return pArchitecture,"x64,x86_64"
   end switch
   
   return pArchitecture
end __ArchitectureSynonyms

private command __ExternalArchitecture pPlatform, @xName, @rArchitecture
   local tArchitectures
   put revSBPlatformArchitectures(pPlatform) into tArchitectures
   
   local tArchitecture,tFoundArchitecture
   repeat for each item tArchitecture in tArchitectures
      local tArchitectureIdentifier
      repeat for each item tArchitectureIdentifier in __ArchitectureSynonyms(tArchitecture)
         if xName ends with tArchitectureIdentifier then
            put tArchitecture into tFoundArchitecture 
            set the itemDelimiter to "-"
            if the number of items of xName > 1 then
               delete item -(the number of items of tArchitectureIdentifier) to -1 of xName
            else
               set the itemDelimiter to "_"
               if the number of items of xName > 1 then
                  delete item -(the number of items of tArchitectureIdentifier) to -1 of xName
               end if
            end if
            set the itemDelimiter to comma
            
            exit repeat
         end if
      end repeat
      
      if tFoundArchitecture is not empty then
         exit repeat
      end if
   end repeat
   
   if tFoundArchitecture is empty then
      if pPlatform is "MacOSX" then
         put tArchitectures into tFoundArchitecture
      else
         put item 1 of tArchitectures into tFoundArchitecture
      end if
   end if
   
   put tFoundArchitecture into rArchitecture
end __ExternalArchitecture

private function __TargetArchitecture pTarget
   local tPlatform
   put revSBTargetToPlatform(pTarget) into tPlatform
   
   local tArchitectures
   put revSBPlatformArchitectures(tPlatform) into tArchitectures
   
   local tArchitecture
   repeat for each item tArchitecture in tArchitectures
      local tArchitectureIdentifier
      repeat for each item tArchitectureIdentifier in __ArchitectureSynonyms(tArchitecture)
         if pTarget ends with tArchitectureIdentifier then
            return tArchitecture
         end if
      end repeat
   end repeat
   
   return item 1 of tArchitectures
end __TargetArchitecture

constant kDefaultCreator             = "????"
constant kDefaultDocumentType        = ""
constant kDefaultDocumentExtension   = ""
constant kDefaultDynamicMemory       = "true"
constant kDefaultMinimumSize         = 15000
constant kDefaultPreferredSize       = 15000
constant kDefaultRegion              = 1
constant kDefaultRelease             = "Final"
constant kDefaultVersionNumber       = "1.0.0.0"
constant kDefaultScriptLibraries     = ""
constant kAskDialog                  = "true"
constant kAnswerDialog               = "true"
constant kCursors                    = "false"

// SN-2015-01-21: [[ Bug 14371 ]] 'MacOSX' has been replaced by 'MacOSX x86-32'
constant kInitialTargets = "Windows,MacOSX x86-64,Linux"

on revSBDefaultStandaloneSettings pStack, @xSettingsA
   local tPlatform,tName
   -- name
   if xSettingsA["name"] = "" then put pStack into xSettingsA["name"]
   put char 1 to 246 of xSettingsA["name"] into xSettingsA["name"]
   if xSettingsA["inclusions"] = "" then put "search" into xSettingsA["inclusions"]
   if xSettingsA["OSX,name"] = "" then put pStack into xSettingsA["OSX,name"]
   if xSettingsA["Windows,ProductName"] = "" then put pStack into xSettingsA["Windows,ProductName"]
   -- main resources
   if xSettingsA["askDialog"] = "" then 
      put kAskDialog into xSettingsA["askDialog"]
      -- assume no properties set
      if xSettingsA["scriptLibraries"] = "" then put replaceText(kDefaultScriptLibraries,",",cr) into xSettingsA["scriptLibraries"]
      
      put revEnvironmentRuntimePath() & "/Windows/x86-32/Support/Sample Icons" into tName
      if xSettingsA["Windows,iconFile"] = "" then put tName & "/genericapp.ico" into xSettingsA["Windows,iconFile"]
      if xSettingsA["Windows,documenticonFile"] = "" then put tName & "/genericdoc.ico" into xSettingsA["Windows,documenticonFile"]
   end if
   if xSettingsA["answerDialog"] = "" then put kAnswerDialog into xSettingsA["answerDialog"]
   if xSettingsA["cursors"] = "" then put kCursors into xSettingsA["cursors"]
   -- version
   set the itemDel to "."
   repeat with X=1 to the number of items of kDefaultVersionNumber
      if xSettingsA["Windows,fileversion"&X] = "" then put item X of kDefaultVersionNumber into xSettingsA["Windows,fileversion"&X]
      if xSettingsA["Windows,productversion"&X] = "" then put item X of kDefaultVersionNumber into xSettingsA["Windows,productversion"&X]
   end repeat
   set the itemDel to ","
   if xSettingsA["OSX,longVersion"] = "" then put pStack&&kDefaultVersionNumber into xSettingsA["OSX,longVersion"]
   if xSettingsA["OSX,info"] = "" then put pStack&&"Version"&&kDefaultVersionNumber into xSettingsA["OSX,info"]
   if xSettingsA["OSX,shortVersion"] = "" then put kDefaultVersionNumber into xSettingsA["OSX,shortVersion"]
   if xSettingsA["Windows,FileDescription"] = "" then put pStack&&kDefaultVersionNumber&&"for Windows" into xSettingsA["Windows,FileDescription"]
   -- document types
   if the number of chars of xSettingsA["OSX,signature"] <> 4 then put kDefaultCreator into xSettingsA["OSX,signature"]
   if xSettingsA["OSX,documentType"] = "" then put kDefaultDocumentType into xSettingsA["OSX,documentType"]
   if xSettingsA["OSX,documentExtension"] = "" then put kDefaultDocumentExtension into xSettingsA["OSX,documentExtension"]
   -- set the unix stuff to true because they will only be imported into unix builds anyway
   if xSettingsA["UNIX,colorChooser"] = "" then put true into xSettingsA["UNIX,colorChooser"]
   if xSettingsA["UNIX,printerChooser"] = "" then put true into xSettingsA["UNIX,printerChooser"]
   if xSettingsA["UNIX,pageSetup"] = "" then put true into xSettingsA["UNIX,pageSetup"]
   if xSettingsA["UNIX,fileSelector"] = "" then put true into xSettingsA["UNIX,fileSelector"]
   
   -- build for every platform we have engines for
   repeat for each item tTarget in kInitialTargets
      if tTarget is "MacOSX x86-64" then 
         if xSettingsA["MacOSX x86-32"] then next repeat
      end if
      if xSettingsA[tTarget] = "" then
         put revEngineCheck(tTarget) into xSettingsA[tTarget]
      end if
   end repeat
   
   -- copyright
   get the seconds
   convert it to dateItems
   if xSettingsA["OSX,copyright"] = "" then put item 1 of it&&the cUserOrganisation of stack "Home"&&"All rights reserved worldwide" into xSettingsA["OSX,copyright"]
   if xSettingsA["OSX,identifier"] = "" then put "com."&replaceText(toLower(the cUserOrganisation of stack "Home")," ","")&"."&replaceText(toLower(xSettingsA["name"])," ","") into xSettingsA["OSX,identifier"]
   if xSettingsA["Windows,LegalCopyright"] = "" then put item 1 of it&&the cUserOrganisation of stack "Home"&&"All rights reserved worldwide" into xSettingsA["Windows,LegalCopyright"]
   if xSettingsA["Windows,companyname"] = "" then put the cUserOrganisation of stack "Home" into xSettingsA["Windows,companyname"]
   
   -- Since the intel choice, set the default here:
   -- MM-2014-03-21: [[ PPC Support Dropped ]] Only create standalone setting for Intel (for OS X) by default.
   if xSettingsA["MacOSX x86-64"] is empty and xSettingsA["MacOSX x86-32"] is empty then
      put true into xSettingsA["MacOSX x86-64"]
   end if
   
   -- Auto-remove stack passwords from community stacks
   if not revEnvironmentEditionProperty("password_protection") then
      if xSettingsA["password"] is not empty then
         delete variable xSettingsA["password"]
      end if
   end if
end revSBDefaultStandaloneSettings

function revSBRemoveUnlicensedTargetsFromSettings @xSettingsA
   -- don't build for any platform we are not licensed for
   local tRemoved
   repeat for each item tTarget in kDesktopTargets, kAdditionalTargets
      if xSettingsA[tTarget] then
         if not revSBLicensedToDeployToTarget(tTarget) then
            put false into xSettingsA[tTarget]
            put true into tRemoved[tTarget]
         end if
      end if
   end repeat
   return the keys of tRemoved
end revSBRemoveUnlicensedTargetsFromSettings

command revSBRelativeStackFilesList pStack, @rStackFiles
   local tLine,tChars,tMainStackPath
   
   local tFullStackFiles
   revSBGetStackFilesList pStack, tFullStackFiles
   
   local tRelativeStackFiles
   set the itemDelimiter to slash
   put item 1 to -2 of line 1 of tFullStackFiles & "/" into tMainStackPath
   
   repeat for each line tLine in tFullStackFiles
      put utilityMakePathRelative(tLine, tMainStackPath) into tLine
      appendToStringList tLine, tRelativeStackFiles
   end repeat
   
   put tFullStackFiles into rStackFiles["full"]
   put tRelativeStackFiles into rStackFiles["relative"]
end revSBRelativeStackFilesList

command revSBGetStackFilesList pStack, @rStackFiles
   local tStackFiles
   put the effective fileName of stack pStack into tStackFiles
   __RecursiveStackFileList pStack, tStackFiles
   
   put tStackFiles into rStackFiles
end revSBGetStackFilesList

private command __RecursiveStackFileList pStack, @xStackFiles
   local tFileName
   set the itemDelimiter to slash
   put item 1 to -2 of the fileName of stack pStack & slash into tFileName
   
   local tStackFiles
   put the stackFiles of stack pStack into tStackFiles
   split tStackFiles by return and comma
   
   set the itemDel to comma
   local tStack
   repeat for each key tStack in tStackFiles
      if there is not a stack tStackFiles[tStack] then put tFileName before tStackFiles[tStack]
      if there is a stack tStackFiles[tStack] then
         local tEffectiveFilename
         put the effective fileName of stack  tStackFiles[tStack] into tEffectiveFilename
         if tEffectiveFilename is not among the lines of xStackFiles then
            appendToStringList tEffectiveFilename, xStackFiles
            __RecursiveStackFileList tStack, xStackFiles
         end if
      end if
   end repeat
   repeat for each line tStack in the substacks of stack pStack
      __RecursiveStackFileList tStack, xStackFiles
   end repeat
end __RecursiveStackFileList

function revDownloadEngine pEngine
  if revEngineCheck(pEngine) then
    return true
  else 
    return false
  end if
end revDownloadEngine

function revEngineCheck pEngine
   -- MW-2013-06-13: [[ CloneAndRun ]] Assume appropriate things are there if not installed.
   if not revEnvironmentIsInstalled() then
      return true
   end if
   
   -- MW-2013-11-06: [[ LinuxX64 ]] Make sure we check both runtime and user runtime paths
   local tPaths
   put revEnvironmentRuntimePath() & return & revEnvironmentUserRuntimePath() into tPaths
   repeat for each line tPath in tPaths
      if tPath is empty then
         next repeat
      end if
      
      -- MM-2014-03-21: [[ PPC Support Dropped ]] Assume all OSX builds to be Intel.
      switch pEngine
         case "MacOSX"
         case "MacOSX PowerPC-32"
         case "MacOSX x86-32"
         case "MacOSX x86-64"
            return there is a directory (tPath & "/Mac OS X/x86-32/Standalone.app")
         case "Windows"
            if there is a file (tPath & "/Windows/x86-32/Standalone") then
               return true
            end if
            break
         case "Windows x86-64"
            if there is a file (tPath & "/Windows/x86-64/Standalone") then
               return true
            end if
            break
            # OK-2007-08-07 : Added linux support
         case "Linux"
            if there is a file (tPath & "/Linux/x86-32/Standalone") then
               return true
            end if
            break
            -- MW-2013-11-06: [[ LinuxX64 ]] Check to see if the 64-bit engine is present
         case "Linux x64"
            if there is a file (tPath & "/Linux/x86-64/Standalone") then
               return true
            end if
            break
            -- FG-2014-08-19: [[ RPi ]] Check to see if the ARMv6 engine is present
         case "Linux ARMv6-HF"
            if there is a file (tPath & "/Linux/armv6-hf/Standalone") then
               return true
            end if
            break
         case "Emscripten"
            if there is a folder (tPath & "/Emscripten/js") then
               return true
            end if
            break
      end switch
   end repeat
   return false
end revEngineCheck

# OK-2007-07-09 : Bug 4592, Abstracted this code as it was duplicated in four buttons

# Parameters
#   pTargetPlatform : The platform for which the icon applies to. Either "Mac OS", "Mac OS X", "Windows" or "Unix"
# Description
#   Prompts the user to choose an icon file as appropriate to the platform. Returns the full path of the file, or empty
#   if no file was chosen.
function utilityChooseIconFile pTargetPlatform
  local tPrompt
  put "Select icon file: " into tPrompt
  
  local tTypes
  put "All files|*" & return into tTypes
  
  # Roughly preserves old behavior, just using answer file with type instead of filter.
  local tIconFile
  if pTargetPlatform is "Mac OS X" then
    put "Icon Resource Files|icns|ICNS" after tTypes
  else if pTargetPlatform is "Windows" then
    put "Icon Files|ico|ICO" after tTypes
  end if
  
  if the number of lines of tTypes is 2 then
    answer file tPrompt with type (line 2 of tTypes) or type (line 1 of tTypes)
  else
    answer file tPrompt with type (line 1 of tTypes)
  end if
  put it into tIconFile
  
  if tIconFile is empty then
    return empty
  end if
  
  return tIconFile
end utilityChooseIconFile

# Parameters
#   pFile : The full path to a file
#   pFolder : The full path of the folder to attempt to make the file relative to.
# Description
#   Simple function that attempts to make the file path pPath relative to the folder path pFolder. If pFile cannot be
#   be make relative, ie because it is not in pFolder then the function returns pFile. If pFile can be made relative
#   then the relative path from pFolder to pFile is returned.
function utilityMakePathRelative pFile, pFolder
  local tFolder
  put pFolder into tFolder
  if the last char of tFolder is not slash then
    put slash after tFolder
  end if
  
  local tFile
  put pFile into tFile
  
  local tCharCount
  put the number of chars of tFolder into tCharCount
  if char 1 to tCharCount of tFile = tFolder then
    delete char 1 to tCharCount of tFile
  end if
  
  return tFile
end utilityMakePathRelative

//////////////////////////////////////////////////////////////////////////////////////////////////////////////

# AL-2015-02-02: [[ Scriptify IDE ]] As we remove buttons from revLIbrary, we'll need to add to
# this function to return the 'common' name of each of the libraries.
function revSBScriptLibraryNameFromStackName pStack
   # AL-2015-03-05: [[ Bug 14813 ]] Internet and Printing are the only script library naming exceptions
   switch pStack
      case "revLibUrl"
         return "Internet"
      case "revPrintLibrary"
         return "Printing"
      default
         local tLibrary
         repeat for each element tLibrary in sScriptLibraries
            if tLibrary["id"] is pStack then
               return tLibrary["title"]
            end if
         end repeat
         
         if pStack begins with "rev" then
            delete char 1 to 3 of pStack
            if pStack ends with "Library" then
               delete char -7 to -1 of pStack
            end if
            put toUpper(char 1 of pStack) into char 1 of pStack
         end if
         break
   end switch
   
   return pStack
end revSBScriptLibraryNameFromStackName

function revSBStackNameFromScriptLibraryName pLib
   # AL-2015-03-05: [[ Bug 14813 ]] Internet and Printing are the only script library naming exceptions
   switch pLib
      case "Internet"
         return "revLibUrl"
      case "Printing"
         return "revPrintLibrary"
      default
         local tLibrary
         repeat for each element tLibrary in sScriptLibraries
            if tLibrary["title"] is pLib then
               return tLibrary["id"]
            end if
         end repeat
         
         return "rev" & pLib & "Library"
   end switch
end revSBStackNameFromScriptLibraryName

constant kScriptLibraries = "Animation,DataGrid,Geometry,Internet,Printing,Table,XMLRPC"
private function __ScriptLibraries
   -- rev script libraries
   # AL-2105-03-05: [[ Bug 14813 ]] Use static list of libs rather than generate from loaded libs list
   // SN-2015-10-19: [[ Bug 16238 ]] Add DataGrid library amongst the libraries
   local tLibs
   put kScriptLibraries into tLibs
   replace comma with return in tLibs
   
   -- user script libraries
   local tEnvironmentScripts
   put revAbsoluteFolderListing(revEnvironmentResourcesPath("Script Libraries")) into tEnvironmentScripts
   
   local tUserScripts
   put revAbsoluteFolderListing(revEnvironmentUserResourcesPath("Script Libraries")) into tUserScripts
   
   local tFiles
   put revCombineFilePaths(tUserScripts,tEnvironmentScripts) into tFiles
   repeat for each line tLine in tFiles
      if there is a stack tLine then 
         put return & the short name of stack tLine after tLibs
      end if
   end repeat
   
   return tLibs
end __ScriptLibraries

private function revSBPlatformToRuntimeFolderName pPlatform
   -- Ensure folder casing is correct
   switch pPlatform
      case "MacOSX"
         return "Mac OS X"
      case "iOS"
         return "iOS"
      case "Windows"
         return "Windows"
      case "Linux"
         return "Linux"
      case "Android"
         return "Android"
      case "Emscripten"
         return "Emscripten"
   end switch
   
   return empty
end revSBPlatformToRuntimeFolderName

function revSBExternalsMapping
   return "revbrowser Browser,revdb Database,revspeech Speech,revvideograbber Video Grabber,revxml XML,revzip Revolution Zip"
end revSBExternalsMapping

function revSBDatabaseDriversMapping
   return "mysql MySQL,odbc ODBC,postgresql PostgreSQL,sqlite SQLite,oracle Oracle"
end revSBDatabaseDriversMapping

function revSBFileIsFontFile pFile
   return (pFile ends with ".ttf" or pFile ends with ".ttc" or pFile ends with ".otf")
end revSBFileIsFontFile

-- MW-2013-06-13: [[ CloneAndRun ]] If we aren't installed, then generate the externals
--   list by hand.
private function revSBExternalsBuildList pPlatform
   -- For now, only allow externals for the current build platform
   local tExpectedPlatform
   switch the platform
      case "MacOS"
         put "MacOSX" into tExpectedPlatform
         break
      case "Win32" 
         put "Windows" into tExpectedPlatform
         break
      case "Linux"
         put "Linux" into tExpectedPlatform
         break
      default
         break
   end switch
   if tExpectedPlatform is not pPlatform then
      return empty
   end if
   
   return revExternalsList()
end revSBExternalsBuildList

private function revSBDesktopExternalsFromFolder pFolder
   -- Find all relevant Externals.txt files
   local tFilesList
   put revSBEnumerateFolder(pFolder) into tFilesList
   filter tFilesList with "*/Externals.txt"
   
   local tExternalsInfo, tListA
   repeat for each line tLine in tFilesList
      put url("file:" & pFolder & slash & tLine) into tExternalsInfo
      repeat for each line tLine in tExternalsInfo
         put true into tListA[item 1 of tLine]
      end repeat
   end repeat
   
   return tListA
end revSBDesktopExternalsFromFolder

private function revSBDesktopExternalsList pPlatform
   local tPlatformFolder
   put revSBPlatformToRuntimeFolderName(pPlatform) into tPlatformFolder
   
   local tListA
   
   local tTargetFolder
   put revEnvironmentRuntimePath() & slash & tPlatformFolder into tTargetFolder
   union tListA with revSBDesktopExternalsFromFolder(tTargetFolder)
   
   put revEnvironmentUserRuntimePath() & slash & tPlatformFolder into tTargetFolder
   union tListA with revSBDesktopExternalsFromFolder(tTargetFolder)
   
   return the keys of tListA
end revSBDesktopExternalsList

private function revSBMobileExternalsFromFolder pFolder
   local tFilesList
   put revSBEnumerateFolder(pFolder) into tFilesList
   
   -- Trim down to the actual files
   local tFilesA
   set the itemdelimiter to slash
   repeat for each line tLine in tFilesList
      put true into tFilesA[item -1 of tLine]
   end repeat
   
   local tFixedExternals
   put revSBExternalsMapping() into tFixedExternals
   
   local tListA
   repeat for each item tItem in tFixedExternals
      if tFilesA[word 1 of tItem] then
         put true into tListA[word 2 to -1 of tItem]
      end if
   end repeat

   return tListA
end revSBMobileExternalsFromFolder

private function revSBMobileExternalsList pPlatform
   -- Since the mobile Runtime folders don't have Externals.txt, 
   -- just check which externals are available from the fixed list
   local tPlatformFolder
   put revSBPlatformToRuntimeFolderName(pPlatform) into tPlatformFolder
   
   local tListA
   
   local tTargetFolder
   put revEnvironmentRuntimePath() & slash & tPlatformFolder into tTargetFolder
   union tListA with revSBMobileExternalsFromFolder(tTargetFolder)
   
   put revEnvironmentUserRuntimePath() & slash & tPlatformFolder into tTargetFolder
   union tListA with revSBMobileExternalsFromFolder(tTargetFolder)
   
   return the keys of tListA
end revSBMobileExternalsList

-- Check in the IDE and user-defined externals locations for 
-- Externals.txt files
function revSBExternalsList pPlatform
   if not revEnvironmentIsInstalled() then
      return revSBExternalsBuildList(pPlatform)
   end if
   
   switch pPlatform
      case "MacOSX"
      case "Linux"
      case "Windows"
         return revSBDesktopExternalsList(pPlatform)
      case "iOS"
      case "Android"
         return revSBMobileExternalsList(pPlatform)
      default 
         return empty
   end switch
end revSBExternalsList

-- MW-2013-06-13: [[ CloneAndRun ]] If we aren't installed, then generate the externals
--   list by hand.
private function revSBDatabaseDriversBuildList pPlatform
   -- For now, only allow externals for the current build platform
   local tExpectedPlatform
   switch the platform
      case "MacOS"
         put "MacOSX" into tExpectedPlatform
         break
      case "Win32" 
         put "Windows" into tExpectedPlatform
         break
      case "Linux"
         put "Linux" into tExpectedPlatform
         break
      default
         break
   end switch
   if tExpectedPlatform is not pPlatform then
      return empty
   end if
   
   return revDatabaseDriverList()
end revSBDatabaseDriversBuildList

private function revSBDesktopDatabaseDriversFromFolder pFolder
   -- Find all relevant Database Drivers.txt files
   local tFilesList
   put revSBEnumerateFolder(pFolder) into tFilesList
   filter tFilesList with "*/Database Drivers.txt"
   
   local tDriversInfo, tListA
   repeat for each line tLine in tFilesList
      put url("file:" & pFolder & slash & tLine) into tDriversInfo
      repeat for each line tLine in tDriversInfo
         put true into tListA[item 1 of tLine]
      end repeat
   end repeat
   
   return tListA
end revSBDesktopDatabaseDriversFromFolder

-- Check in the IDE and user-defined DB driver locations for 
-- Database Drivers.txt files
private function revSBDesktopDatabaseDriversList pPlatform
   local tPlatformFolder
   put revSBPlatformToRuntimeFolderName(pPlatform) into tPlatformFolder
   
   local tListA
   
   local tTargetFolder
   put revEnvironmentRuntimePath() & slash & tPlatformFolder into tTargetFolder
   union tListA with revSBDesktopDatabaseDriversFromFolder(tTargetFolder)
   
   put revEnvironmentUserRuntimePath() & slash & tPlatformFolder into tTargetFolder
   union tListA with revSBDesktopDatabaseDriversFromFolder(tTargetFolder)
   
   return the keys of tListA
end revSBDesktopDatabaseDriversList

private function revSBMobileDatabaseDriversFromFolder pFolder
   local tFilesList
   put revSBEnumerateFolder(pFolder) into tFilesList
   
   -- Trim down to the actual files
   local tFilesA
   set the itemdelimiter to slash
   repeat for each line tLine in tFilesList
      put true into tFilesA[item -1 of tLine]
   end repeat
   
   local tFixedDrivers
   put revSBDatabaseDriversMapping() into tFixedDrivers
   
   local tListA
   set the itemdelimiter to comma
   repeat for each item tItem in tFixedDrivers
      if tFilesA["Db" & word 1 of tItem] then
         put true into tListA[word 2 to -1 of tItem]
      end if
   end repeat

   return tListA
end revSBMobileDatabaseDriversFromFolder

private function revSBMobileDatabaseDriversList pPlatform
   -- Since the mobile Runtime folders don't have Externals.txt, 
   -- just check which DB drivers are available from the fixed list
   local tPlatformFolder
   put revSBPlatformToRuntimeFolderName(pPlatform) into tPlatformFolder
   
   local tListA
   
   local tTargetFolder
   put revEnvironmentRuntimePath() & slash & tPlatformFolder into tTargetFolder
   union tListA with revSBMobileDatabaseDriversFromFolder(tTargetFolder)
   
   put revEnvironmentUserRuntimePath() & slash & tPlatformFolder into tTargetFolder
   union tListA with revSBMobileDatabaseDriversFromFolder(tTargetFolder)
   
   return the keys of tListA
end revSBMobileDatabaseDriversList

function revSBDatabaseDriversList pPlatform
   if not revEnvironmentIsInstalled() then
      return revSBDatabaseDriversBuildList(pPlatform)
   end if
   
   switch pPlatform
      case "MacOSX"
      case "Linux"
      case "Windows"
         return revSBDesktopDatabaseDriversList(pPlatform)
      case "iOS"
      case "Android"
         return revSBMobileDatabaseDriversList(pPlatform)
      default 
         return empty
   end switch
end revSBDatabaseDriversList

-- Inclusions that are or require binary blobs from the 
-- Runtime folder
function revSBBuiltinInclusions pPlatform
   # AL-2105-03-05: [[ Bug 14813 ]] Use array key method to ignore duplicates
   local tPath,tSave,tLibrariesA,tBtn,tExternals
   -- rev externals
   get revSBExternalsList(pPlatform)
   repeat for each line tLine in it
      put empty into tLibrariesA[tLine]
   end repeat

   if pPlatform is not "emscripten" then
   put empty into tLibrariesA["SSL & Encryption"]
      put empty into tLibrariesA["Revolution Zip"]
   end if
   
   if "Browser" is among the keys of tLibrariesA then
      put empty into tLibrariesA["Browser (CEF)"]
   end if

   return the keys of tLibrariesA
end revSBBuiltinInclusions

function revSBLcextFileIsZipArchive pFile, @rError
   open file pFile for binary read
   if the result is empty then
      read from file pFile for 4 bytes
      close file pFile
   end if
   if it is empty then
      put "could not open external -" && pFile into rError
      return false
   end if
   
   return byte 1 of it is "P" and byte 2 of it is "K" and \ 
         byte 3 of it is numToByte(3) and byte 4 of it is numToByte(4)
end revSBLcextFileIsZipArchive

function revSBLcextExternalHasFolder pExternal, pPlatform
   local tContents, tFolder
   put revSBPlatformToRuntimeFolderName(pPlatform) into tFolder
   
   revZipOpenArchive pExternal, "read"
   if the result is empty then
      put revZipEnumerateItems(pExternal) into tContents
      filter tContents with tFolder & "/*"
   end if
   return tContents is not empty
end revSBLcextExternalHasFolder

function revSBLcextFileIsMobileExternal pFile, pPlatform
   local tError
   return revSBLcextFileIsZipArchive(pFile, tError) and \
         revSBLcextExternalHasFolder(pFile, pPlatform)
end revSBLcextFileIsMobileExternal

private function __FilterExternalsForMobile pList, pPlatform
   local tLcext
   filter pList with "*.lcext" into tLcext
   
   local tLcextd
   filter pList with "*.lcextd/" into tLcextd
   
   local tList, tError
   repeat for each line tLine in tLcext
      -- look inside the lcext zip to see if the platform is supported (in any architecture)
      if revSBLcextFileIsMobileExternal(tLine, pPlatform) then
         if tList is empty then
            put tLine into tList
         else
            put return & tLine after tList
         end if
      end if
   end repeat
   
   repeat for each line tLine in tLcextd
      if there is a folder (tLine & pPlatform) then
         delete the last char of tLine
         if tList is empty then
            put tLine into tList
         else
            put return & tLine after tList
         end if
      end if
   end repeat
   
   return tList
end __FilterExternalsForMobile

-- filter file (and folder!) list by expected file extension
private function __FilterExternalsForDesktop pList, pPlatform
   local tExtension
   switch pPlatform
      case "MacOSX" 
         -- .bundles are folders
         put "bundle/" into tExtension
         break
      case "Windows"
         put "dll" into tExtension
         break
      case "Linux"
         put "so" into tExtension
         break
   end switch
   filter pList with "*." & tExtension
   
   return pList
end __FilterExternalsForDesktop

function revSBExternalNameFromFile pPlatform, pFile
   local tName
   set the itemdel to "."
   put item 1 of pFile into tName
   
   local tArch
   __ExternalArchitecture pPlatform, tName, tArch
   
   return tName
end revSBExternalNameFromFile

-- This is currently just used for externals that are present in the
-- 'Ext' folder in the app bundle or equivalent
local sExternalsLocationA
function revSBAdditionalExternalsList pPlatform
   if sExternalsLocationA[pPlatform] is empty then
      local tFolders
      put revSBAdditionalExternalFolders() into tFolders
      
      -- For these externals we have to go by the file extensions that exist
      -- to check if the platform is supported
      repeat for each line tFolder in tFolders
         local tExternalFolder
         repeat for each line tExternalFolder in folders(tFolder)
            if tExternalFolder begins with "." then next repeat
            
            if there is a file (tFolder & slash & tExternalFolder & slash & "ide_only") then
               next repeat
            end if
            
            local tContents
            put __EnumerateFolder(tFolder & slash & tExternalFolder, tFolder & slash & tExternalFolder & slash, false) into tContents
            
            local tRawList
            switch pPlatform
               case "ios"
               case "android"
                  put __FilterExternalsForMobile(tContents, pPlatform) into tRawList
                  break
               case "windows"
               case "macosx"
               case "linux"
                  put __FilterExternalsForDesktop(tContents, pPlatform) into tRawList
                  break
               default
                  put empty into tRawList
                  break
            end switch
            
            split tRawList by return
            set the itemdelimiter to "."
            set the linedelimiter to slash
            repeat for each element tExtPath in tRawList
               local tExtension, tExternalName
               put item -1 of line -1 of tExtPath into tExtension
               put item 1 to -2 of line -1 of tExtPath into tExternalName
               
               set the itemDelimiter to comma
               local tArchitectures
               if pPlatform is among the items of kAdditionalTargets then
                  put pPlatform into tArchitectures
               else
                  __ExternalArchitecture pPlatform, tExternalName, tArchitectures
               end if
               
               local tArchitecture
               repeat for each item tArchitecture in tArchitectures
                  put tExtPath into sExternalsLocationA[pPlatform][tExternalName][tArchitecture]
               end repeat
               
               set the itemDelimiter to "."
            end repeat
            set the linedelimiter to return
         end repeat
      end repeat
   end if
   return the keys of sExternalsLocationA[pPlatform]
end revSBAdditionalExternalsList

-- Ensure the name used for platforms is consistent
constant kMacPlatform = "MacOSX"
constant kWindowsPlatform = "Windows"
constant kLinuxPlatform = "Linux"
constant kiOSPlatform = "iOS"
constant kAndroidPlatform = "Android"
constant kEmscriptenPlatform = "Emscripten"

function revSBAllPlatforms
   return kMacPlatform & comma & \
         kWindowsPlatform & comma & \
         kLinuxPlatform & comma & \
         kiOSPlatform & comma & \
         kAndroidPlatform & comma & \
         kEmscriptenPlatform
end revSBAllPlatforms

function revSBAllDesktopPlatforms
   return kMacPlatform & comma & \
         kWindowsPlatform & comma & \
         kLinuxPlatform
end revSBAllDesktopPlatforms

function revSBAllMobilePlatforms
   return kiOSPlatform & comma & \
         kAndroidPlatform
end revSBAllMobilePlatforms

function revSBNormalizePlatforms pOS, pPlatforms
   local tPlatformList
   if pOS is empty then
      if pPlatforms contains "mobile" then
         put revSBAllMobilePlatforms() & comma after tPlatformList
      end if
      if pPlatforms contains "desktop" or pPlatforms contains "server" then
         put revSBAllDesktopPlatforms() & comma after tPlatformList
      end if
   else
      if pOS contains "mac" then
         put kMacPlatform & comma after tPlatformList
      end if
      if pOS contains "win" then
         put kWindowsPlatform & comma after tPlatformList
      end if
      if pOS contains "linux" then
         put kLinuxPlatform & comma after tPlatformList
      end if
      if pOS contains "ios" then
         put kiOSPlatform & comma after tPlatformList
      end if
      if pOS contains "android" then
         put kAndroidPlatform & comma after tPlatformList
      end if
      if pOS contains "html5" then
         put kEmscriptenPlatform & comma after tPlatformList
      end if
   end if
   if tPlatformList is empty then
      put revSBAllPlatforms() into tPlatformList
   else
      delete char -1 of tPlatformList
   end if
   return tPlatformList
end revSBNormalizePlatforms

private command addToList pElement, @xArray
   put pElement into xArray[the number of elements in xArray + 1]
end addToList

private function revSBAvailableWidgets
   return revSBAvailableExtensions("widget")
end revSBAvailableWidgets

private function revSBAvailableLibraries
   return revSBAvailableExtensions("library")
end revSBAvailableLibraries

-- LCB Extensions
private function revSBAvailableExtensions pType
   local tExtensions, tInclusions
   put revIDEExtensions(pType, "installed") into tExtensions
   repeat for each key tID in tExtensions
      local tExtension, tOS, tPlatforms
      put tExtensions[tID]["os"] into tOS
      put tExtensions[tID]["platforms"] into tPlatforms
      if tOS is empty and tPlatforms is empty then
         -- LCB extensions are available on all platforms by default
         put revSBAllPlatforms() into tExtension["platforms"]
      else
         put revSBNormalizePlatforms(tOS, tPlatforms) into tExtension["platforms"]
      end if
      put pType into tExtension["type"]
      put tExtensions[tID]["title"] into tExtension["title"]
      put tID into tExtension["id"]
      addToList tExtension, tInclusions
   end repeat
   return tInclusions
end revSBAvailableExtensions

private command __InitialiseScriptLibraries
   local tLibraries
   put __ScriptLibraries() into tLibraries
   repeat for each line tLine in tLibraries
      revSBAddAvailableLibrary revSBStackNameFromScriptLibraryName(tLine), tLine
   end repeat
end __InitialiseScriptLibraries

private function __AvailableScriptLibraries
   if sScriptLibraries is not an array then
      __InitialiseScriptLibraries
   end if
   return sScriptLibraries
end __AvailableScriptLibraries

command revSBAddAvailableLibrary pID, pTitle, pSettingsA
   local tLibrary
   put pID into tLibrary["id"]
   put pTitle into tLibrary["title"]
   put pSettingsA into tLibrary["settings"]
   put "script library" into tLibrary["type"]
   
   -- liburl and the datagrid are supported on all platforms
   -- until we have meta data associated with each lib (that includes platforms supported)
   -- just handle the special cases directly 
   if pTitle is among the items of "Internet,DataGrid" then
      put revSBAllPlatforms() into tLibrary["platforms"]
   else
      put revSBAllDesktopPlatforms() & comma & \
            revSBAllMobilePlatforms() into tLibrary["platforms"]
   end if
   
   addToList tLibrary, sScriptLibraries
end revSBAddAvailableLibrary

constant kResources = "Cursors|cursors,Answer Dialog|answerDialog,Ask Dialog|askDialog,Brushes|brushes,Magnify|magnify,PDF Printer|pdfPrinter,Print Dialogs|revolutionPrintDialogs"
private function revSBAvailableResources
   local tInclusions, tResources
   put kResources into tResources
   split tResources by "," and "|"
   
   repeat for each key tTitle in tResources
      local tInclusion
      put tResources[tTitle] into tInclusion["id"]
      put tTitle into tInclusion["title"]
      put "resource" into tInclusion["type"]
      put revSBAllDesktopPlatforms() into tInclusion["platforms"]
      if tTitle is "PDF Printer" then
         put comma & kiOSPlatform & comma & kAndroidPlatform after tInclusion["platforms"]
      end if
      addToList tInclusion, tInclusions
   end repeat
   return tInclusions
end revSBAvailableResources

-- Additional in-IDE external paths can be added here
private function revSBAdditionalExternalFolders
   return revEnvironmentExtPath()
end revSBAdditionalExternalFolders

-- Use the specified getter to fetch all the data about the inclusions
-- of the given type for each platform.
-- pGetter must take the platform as an argument.
private function revSBAvailablePerPlatformInclusions pGetter, pType
   local tList, tInclusions
   repeat for each item tPlatform in revSBAllPlatforms()
      local tPlatformList
      dispatch function pGetter to me with tPlatform 
      put the result into tPlatformList
      
      repeat for each line tTitle in tPlatformList
         if tList[tTitle] is empty then
            local tInclusion
            put tTitle into tInclusion["id"]
            put tTitle into tInclusion["title"]
            put pType into tInclusion["type"]
            put tPlatform into tInclusion["platforms"]
            put tInclusion into tList[tTitle]
         else
            put comma & tPlatform after tList[tTitle]["platforms"]
         end if
      end repeat
   end repeat
   
   repeat for each element tInclusion in tList
      addToList tInclusion, tInclusions
   end repeat
   
   return tInclusions
end revSBAvailablePerPlatformInclusions

private function revSBAdditionalExternals
   return revSBAvailablePerPlatformInclusions("revSBAdditionalExternalsList", "external")
end revSBAdditionalExternals

private function revSBAvailableBuiltinInclusions
   return revSBAvailablePerPlatformInclusions("revSBBuiltinInclusions", "inclusion")
end revSBAvailableBuiltinInclusions

private function revSBAvailableDatabaseDrivers
   return revSBAvailablePerPlatformInclusions("revSBDatabaseDriversList", "database driver")
end revSBAvailableDatabaseDrivers

/**
Summary: Returns an array of available inclusions

Returns: A numerically keyed array.
Each element is an array with the following keys:
- "title" : The display name of the extension
- "type" : One of "widget", "library", "external", "script library", "inclusion", "database driver"
- "platforms" : A comma-delimited list of the available platforms
- "id" : A unique identifier for the inclusion. This is used to actually find the resource
when including in a standalone. Where the id is the same as the title, it is expected
that the mapping is resolved later on.
*/
function revSBAvailableInclusions
   local tInclusions
   repeat for each element tInclusion in revSBAvailableWidgets()
      addToList tInclusion, tInclusions
   end repeat
   
   repeat for each element tInclusion in revSBAvailableLibraries()
      addToList tInclusion, tInclusions
   end repeat
   
   repeat for each element tInclusion in __AvailableScriptLibraries()
      addToList tInclusion, tInclusions
   end repeat
   
   repeat for each element tInclusion in revSBAvailableDatabaseDrivers()
      addToList tInclusion, tInclusions
   end repeat
   
   repeat for each element tInclusion in revSBAvailableBuiltinInclusions()
      addToList tInclusion, tInclusions
   end repeat
   
   repeat for each element tInclusion in revSBAvailableResources()
      addToList tInclusion, tInclusions
   end repeat
   
   repeat for each element tInclusion in revSBAdditionalExternals()
      addToList tInclusion, tInclusions
   end repeat
   
   return tInclusions
end revSBAvailableInclusions

private command inclusionKeyAndIdentifier pElement, @rKey, @rId
   switch pElement["type"]
      case "widget"
      case "library"
         put "extensions" into rKey
         put pElement["id"] into rId
         break
      case "script library"
         put "scriptLibraries" into rKey
         put pElement["title"] into rId
         break
      case "external"
         put "externals" into rKey
         put pElement["id"] into rId
         break
      case "database driver"
         put "databaseDrivers" into rKey
         put pElement["id"] into rId
         break
      case "resource"
         put pElement["id"] into rKey
         put true into rId
         break
         -- at the moment all other inclusions go through the scriptLibraries key
      case "inclusion"
         put "scriptLibraries" into rKey
         put pElement["id"] into rId
         break
   end switch
end inclusionKeyAndIdentifier

command revSBUpdateInclusions pInclusions, pExclusions, pStackName
   local tSettingsKey, tIdentifier, tNewSettings
   
   -- first remove all exclusions
   repeat for each element tElement in pExclusions
      inclusionKeyAndIdentifier tElement, tSettingsKey, tIdentifier
      set the cRevStandaloneSettings[tSettingsKey] of stack pStackName to empty
   end repeat
   
   -- now add all inclusions
   repeat for each element tElement in pInclusions
      inclusionKeyAndIdentifier tElement, tSettingsKey, tIdentifier
      
      if tNewSettings[tSettingsKey] is empty then
         put tIdentifier into tNewSettings[tSettingsKey]
      else
         put return & tIdentifier after tNewSettings[tSettingsKey]
      end if
   end repeat
   
   repeat for each key tKey in tNewSettings
      set the cRevStandaloneSettings[tKey] of stack pStackName to tNewSettings[tKey]
   end repeat
end revSBUpdateInclusions

function revSBInclusionToMobileKey pName, pType, pPlatform
   local tToInclude
   switch pName
      case "pdfPrinter"
         put "revpdfprinter" into tToInclude
         break
      case "SSL & Encryption"
         put "revsecurity" into tToInclude
         break
      default
         switch pType
            case "database driver"
               local tDBDriverID, tDBDrivers
               put revSBDatabaseDriversMapping() into tDBDrivers
               put "db" & word 1 of item itemOffset(pName, tDBDrivers) of tDBDrivers into tToInclude
               break
            case "external"
            default
               local tExternals
               put revSBExternalsMapping() into tExternals
               put word 1 of item itemOffset(pName, tExternals) of tExternals into tToInclude
               break
         end switch
         break
   end switch
   
   if tToInclude is not empty then
      return pPlatform & ",include" && tToInclude 
   end if
   
   return empty
end revSBInclusionToMobileKey

-- Mobile standalone builders need to use this to convert from new inclusion settings
command revSBConvertSettingsForPlatform @xSettings, pPlatform
   -- Sanitise the list of externals and route through copy files mechanism
   local tAvailableExternals
   put revSBAdditionalExternalsList(pPlatform) into tAvailableExternals
   repeat for each line tLine in xSettings["externals"]
      if tLine is among the lines of tAvailableExternals then
         appendToStringList "!" & tLine, xSettings["files"]
      end if
   end repeat
   
   -- Convert from desktop specific keys to mobile
   if xSettings["pdfPrinter"] then
      put "true" into xSettings[revSBInclusionToMobileKey("pdfPrinter", "", pPlatform)]
   end if
   
   repeat for each line tLine in xSettings["databaseDrivers"]
      put "true" into xSettings[revSBInclusionToMobileKey(tLine, "database driver", pPlatform)]
   end repeat
   
   repeat for each line tLine in xSettings["scriptLibraries"]
      put "true" into xSettings[revSBInclusionToMobileKey(tLine, "external", pPlatform)]
   end repeat
end revSBConvertSettingsForPlatform

function revSBIncludedExternalPath pTarget, pName
   local tPlatform
   put revSBTargetToPlatform(pTarget) into tPlatform
   
   if sExternalsLocationA[tPlatform] is empty then
      get revSBExternalsList(tPlatform)
   end if
   
   local tArchitecture
   if tPlatform is among the items of kAdditionalTargets then
      put tPlatform into tArchitecture
   else
      put __TargetArchitecture(pTarget) into tArchitecture
   end if
   
   return sExternalsLocationA[tPlatform][pName][tArchitecture]
end revSBIncludedExternalPath

private command __ResolveIncludedExternal pTarget, @xFiles, @rHasResources
   local tHasResources
   put false into tHasResources
   
   -- Added to support Ext inclusion
   if xFiles begins with "!" then
      
      local tExtPath
      put revSBIncludedExternalPath(pTarget, char 2 to -1 of xFiles) into tExtPath
      -- if the file prefixed with ! was not a named external, treat normally
      if tExtPath is not empty then
         put tExtPath into xFiles
         
         set the itemDelimiter to slash
         
         local tResources
         put item 1 to -2 of tExtPath & slash & "resources" into tResources
         if there is a folder tResources then
            local tContents
            put __EnumerateFolder(tResources, tResources & slash, false) into tContents
            
            local tPlatform
            put revSBTargetToPlatform(pTarget) into tPlatform
            if tPlatform is not "iOS" then
               -- remove nib files as we don't need them on anything but iOS
               filter tContents without "*.nib"
            end if
            
            if tContents is not empty then
               appendToStringList tContents, xFiles
               put true into tHasResources
            end if
         end if
         
      end if
   end if
   put tHasResources into rHasResources
end __ResolveIncludedExternal

command revSBResolveCopyFilesList pBaseFolder, pTarget, pSettings, @rFileData
   local tComputedFileData
   put pSettings["files_computed"] into tComputedFileData
   
   set the itemDelimiter to slash
   repeat for each line tFile in pSettings["files"]
      local tResolvedFile, tHasResources
      __ResolveIncludedExternal pTarget, tFile, tHasResources
      
      -- tFile may now list multiple files if it was an external with a resources directory
      repeat for each line tResolvedFile in tFile
         local tDataA, tFlattenStructure
         
         if tResolvedFile ends with "/" then
            delete char -1 of tResolvedFile
         end if
         
         if the last item of tResolvedFile is "*" then
            delete the last item of tResolvedFile
         end if
         
         local tIsAbsolute, tName
         put tResolvedFile begins with "/" or char 2 to 3 of tResolvedFile is ":/" into tIsAbsolute
         
         # Flatten structure of external resources
         if tIsAbsolute or tHasResources then
            put item -1 of tResolvedFile into tDataA["name"]
         else
            put tResolvedFile into tDataA["name"]
         end if
         
         if not tIsAbsolute then
            put pBaseFolder & slash before tResolvedFile
         end if
         put tResolvedFile into tDataA["resolved"]
         
         addToList tDataA, tComputedFileData
      end repeat
   end repeat
   put tComputedFileData into rFileData
end revSBResolveCopyFilesList

private command addToStringList pAppend, pValue, @xList
   if pValue is among the lines of xList then
      exit addToStringList
   end if
   
   if xList is empty then
      put pValue into xList
   else if pAppend then
      put return & pValue after xList
   else
      put pValue & return before xList
   end if
end addToStringList

private command appendToStringList pValue, @xList
   addToStringList true, pValue, xList
end appendToStringList

private command prependToStringList pValue, @xList
   addToStringList false, pValue, xList
end prependToStringList

private command __LoadStackOnStartup pStackName, @xAuxStackFiles, @xStartupScript, @xGeneratedStartupScript
   appendToStringList the effective filename of stack pStackName, xAuxStackFiles
   appendToStringList "revInternal__LoadLibrary" && quote & pStackName & quote, xStartupScript
   
   -- allow libraries to generate some additional startup script
   dispatch "extensionStartupScript" to stack pStackName
   if it is "handled" then
      appendToStringList the result, xGeneratedStartupScript
   end if
end __LoadStackOnStartup

private command __AddModuleSources pExtension, pInstallFolder, pTargetFolder, @xSettingsA
   if not revEnvironmentEditionProperty("open_source_required") then
      exit __AddModuleSources
   end if
   -- Include main source file, if present
   local tSourceFileData, tSourceFile
   put revIDEExtensionProperty(pExtension, "source_file") \
         into tSourceFile
   if there is a file tSourceFile then
      put pInstallFolder & slash & tSourceFile into tSourceFileData["resolved"]
      put pTargetFolder & slash & tSourceFile into tSourceFileData["name"]
      addToList tSourceFileData, xSettingsA["files_computed"]
   end if
   
   -- Include support source files in standalone, if present
   local tSupportFiles
   put revIDEExtensionProperty(pExtension, "support_files") \
         into tSupportFiles
   repeat for each line tLine in tSupportFiles
      local tSupportFileData
      put pInstallFolder & slash & tLine into tSupportFileData["resolved"]
      put pTargetFolder & slash & tLine into tSupportFileData["name"]
      addToList tSupportFileData, xSettingsA["files_computed"]
   end repeat
end __AddModuleSources

command revSBUpdateSettingsForExtensions pTarget, @xSettingsA
   local tLoadIndex, tDependencyOrder
   put revIDEExtensionsOrderByDependency(xSettingsA["extensions"]) \
         into tDependencyOrder
   
   repeat for each line tExtension in tDependencyOrder
      add 1 to tLoadIndex
      
      local tTargetResourceFolder
      put "extension" & tLoadIndex into tTargetResourceFolder
      
      local tExtensionData
      put tExtension into tExtensionData["id"]
      
      local tInstallFolder
      put revIDEExtensionProperty(tExtension, "install_path") \
            into tInstallFolder
      
      __AddModuleSources tExtension, tInstallFolder, tTargetResourceFolder, \
            xSettingsA
      
      local tResourcesFolder
      put revIDEExtensionProperty(tExtension, "install_path") & \
            slash & "resources" into tResourcesFolder
      if there is a folder tResourcesFolder then
         local tContents
         put __EnumerateFolder(tResourcesFolder, "", true) into tContents
         repeat for each line tLine in tContents
            local tFileData
            put tResourcesFolder & slash & tLine into tFileData["resolved"]

            -- __EnumerateFolder list folders and we only want to add files
            if there is a folder tFileData["resolved"] then
               next repeat
            end if

            put tTargetResourceFolder & slash & tLine into tFileData["name"]
            addToList tFileData, xSettingsA["files_computed"]
         end repeat
         
         # Add resource mappings to the deploy params
         local tPlatform
         put revSBTargetToPlatform(pTarget) into tPlatform
         
         revSBAddLibraryMapping tPlatform, tExtension & slash & "resources", \
               tTargetResourceFolder, xSettingsA
      end if
      put tExtensionData into xSettingsA["extensions_computed"][tLoadIndex]
   end repeat
end revSBUpdateSettingsForExtensions

constant kMappingSeparator = ":"
command revSBAddLibraryMapping pPlatform, pName, pLocation, @xSettingsA
   local tMapping
   put pName & kMappingSeparator & "./" & pLocation into tMapping
   appendToStringList tMapping, xSettingsA[pPlatform & ",library"]
end revSBAddLibraryMapping

command revSBUpdateDeployParams pStack, pTarget, pStandaloneSettings, @xDeployParams
   local tModules, tAuxiliaryStackfiles, tStartupScript
   local tGeneratedStartupScript, tExternals, tLibraryMapping
   
   # AL-2015-02-04: [[ Standalone Resources ]] Add (universal) external names to the deploy parameters
   put pStandaloneSettings["externals"] into tExternals
   
   revSBAddInclusion "Common", "scriptLibraries", pStandaloneSettings
   
   if pStandaloneSettings["includeProfiles"] is not empty then 
      revSBAddInclusion "Profiles", "scriptLibraries", pStandaloneSettings
   end if
   
   -- auxiliary_stackfiles key might not be empty
   put pStandaloneSettings["auxiliary_stackfiles"] into tAuxiliaryStackfiles
   
   if pTarget is not "emscripten" then
      appendToStringList "revInternal_SetJAVA_HOME", tStartupScript
   end if
   
   if pStandaloneSettings["jarFiles"] is not empty then
      appendToStringList "revInternal_SetCLASSPATH", tStartupScript
   end if
   
   # AL-2015-04-12: [[ Standalone Extensions ]] Add chosen extensions to the standalone inclusions
   # AL-2015-07-21: [[ Standalone Extensions ]] Automatically include dependecies, and load in order
   local tValidExtensionsA
   repeat for each element tExtensionDataA in pStandaloneSettings["extensions_computed"]
      local tExtensionFileDataA
      put revIDEExtensionFileData(tExtensionDataA["id"]) into tExtensionFileDataA
      if tExtensionFileDataA["type"] is "lcb" then
         appendToStringList tExtensionFileDataA["file"], tModules
      else if tExtensionFileDataA["type"] is "lcs" then
         __LoadStackOnStartup tExtensionDataA["id"], tAuxiliaryStackFiles, tStartupScript, tGeneratedStartupScript
      else
         revStandaloneAddWarning "Extension" && tExtensionDataA["id"] && "not found"
      end if
   end repeat
   
   # Add resource mappings to the deploy params
   local tPlatform
   put revSBTargetToPlatform(pTarget) into tPlatform
   put pStandaloneSettings[tPlatform & ",library"] into tLibraryMapping
   
   local tLibraryStack, tIncludeDataGrid
   repeat for each line tLibrary in pStandaloneSettings["scriptLibraries"]
      -- Special-case data grid at the moment, as it is not initialised 
      -- using extensionInitialize
      if tLibrary is "DataGrid" then
         put true into tIncludeDataGrid
         next repeat
      end if
      # AL-2015-03-05: [[ Bug 14813 ]] Transform script library display name to stack name at deploy time
      put revSBStackNameFromScriptLibraryName(tLibrary) into tLibraryStack
      if there is a stack tLibraryStack then
         __LoadStackOnStartup tLibraryStack, tAuxiliaryStackFiles, tStartupScript, tGeneratedStartupScript
      end if
   end repeat
   
   local tLoadedLibraries
   put revInternal__ListLoadedLibraries() into tLoadedLibraries
   split tLoadedLibraries by return as set
   
   repeat for each line tLibrary in pStandaloneSettings["externals"]
      -- check if there are any supporting libraries for the external and add
      local tExtPath
      put revSBIncludedExternalPath(pTarget, tLibrary) into tExtPath
      if tExtPath is not empty then
         set the itemDelimiter to slash
         delete the last item of tExtPath
         
         -- only include currently loaded libraries
         repeat for each key tLibrary in tLoadedLibraries
            if there is a stack tLibrary and the filename of stack tLibrary begins with tExtPath then
               __LoadStackOnStartup tLibrary, tAuxiliaryStackFiles, tStartupScript, tGeneratedStartupScript
            end if
         end repeat
      end if
   end repeat
   
   appendToStringList tGeneratedStartupScript, tStartupScript
   
   -- Make sure we include the initialisation library (at the top of the list) if needed
   if tAuxiliaryStackFiles is not empty then
      local tRevInitialisationLibrary
      put the effective filename of stack "revInitialisationLibrary" into tRevInitialisationLibrary
      local tInitScript
      put "insert the script of stack" && quote & "revInitialisationLibrary" & quote && "into back" into tInitScript
      prependToStringList tRevInitialisationLibrary, tAuxiliaryStackFiles
      prependToStringList tInitScript, tStartupScript
   end if
   
   -- If the datagrid is used, then include the library as an aux stack
   if tIncludeDataGrid or "data grid templates" is in the subStacks of stack pStack and there is a stack "revDataGridLibrary" then
      revSBAddDataGridToAuxiliaryStackFiles tAuxiliaryStackFiles
   end if
   
   put tAuxiliaryStackFiles into xDeployParams["auxiliary_stackfiles"]
   put tStartupScript into xDeployParams["startup_script"]
   put tModules into xDeployParams["modules"]
   put tExternals into xDeployParams["externals"]
   put tLibraryMapping into xDeployParams["library"]
end revSBUpdateDeployParams

command revSBAddDataGridToAuxiliaryStackFiles @xAuxiliaryStackFiles
   local tStackFiles
   put the stackFiles of stack "revDataGridLibrary" into tStackFiles
   split tStackFiles by return and comma
   repeat for each key tStack in tStackFiles
      appendToStringList the effective filename of stack tStack, xAuxiliaryStackFiles
   end repeat
   appendToStringList the effective filename of stack "revDataGridLibrary", xAuxiliaryStackFiles
end revSBAddDataGridToAuxiliaryStackFiles

local sBuiltInInclusions

private command __InitialiseBuiltInInclusions
   revSBAddBuiltInInclusion "ios", "scriptLibraries", "Internet"
   revSBAddBuiltInInclusion "android", "scriptLibraries", "Internet"
end __InitialiseBuiltInInclusions

command revSBAddBuiltInInclusion pPlatform, pInclusionType, pInclusion
   put true into sBuiltInInclusions[pPlatform][pInclusionType][pInclusion]
end revSBAddBuiltInInclusion

command revSBOverrideBuiltInInclusion pPlatform, pInclusionType, pInclusion
   delete variable  sBuiltInInclusions[pPlatform][pInclusionType][pInclusion]
end revSBOverrideBuiltInInclusion

command revSBRemoveInclusions pStack, pPlatform, pConfirm, @xSettings
   local tName, tKey
   repeat for each key tKey in sBuiltInInclusions[pPlatform]
      repeat for each key tName in sBuiltInInclusions[pPlatform][tKey]
         
         local tInclude
         put xSettings["overrideInclusions"][tName] into tInclude
         if pConfirm and xSettings["overrideInclusions"][tName] is empty then
            local tSilenceWarning
            dispatch "revConfirmInclusion" to stack "revStandaloneSettings" with tName, tInclude, tSilenceWarning
            
            if tSilenceWarning then
               local tOverride
               put xSettings["overrideInclusions"] into tOverride
               put tInclude into tOverride[tName]
               set the cRevStandaloneSettings["overrideInclusions"] of stack pStack to tOverride
               revIDESetEdited pStack
            end if
         end if
         
         if tInclude is not true then
            local tLineNo
            put lineOffset(tName, xSettings[tKey]) into tLineNo
            if tLineNo is not 0 then
               delete line tLineNo of xSettings[tKey]
            end if
         end if
      end repeat
   end repeat
end revSBRemoveInclusions

private command revSBListWidgetsOfStack pStack, @xWidgetKindsA
   local tNumCards, tNumWidgets, tKind
   put the number of cards of pStack into tNumCards
   repeat with tCard = 1 to tNumCards
      put the number of widgets of card tCard of pStack into tNumWidgets
      repeat with tWidget = 1 to tNumWidgets
         put the kind of widget tWidget of card tCard of pStack into tKind
         put "" into xWidgetKindsA[tKind]
      end repeat
   end repeat
end revSBListWidgetsOfStack

function revSBListWidgetsInUse pStack
   local tStackFilesList, tStacksList, tWidgetKindsA
   revSBGetStackFilesList pStack, tStackFilesList
   repeat for each line tStackFile in tStackFilesList
      put the short name of stack tStackFile & return \ 
            & the substacks of stack tStackFile into tStacksList
      repeat for each line tStack in tStacksList
         revSBListWidgetsOfStack the long id of stack tStack, tWidgetKindsA
      end repeat
   end repeat
   return the keys of tWidgetKindsA
end revSBListWidgetsInUse

function revSBSuppressDialogs
   if the environment is "development" then
      return revTestEnvironment()
   end if
   return true
end revSBSuppressDialogs

function revSBStandaloneOutputDirectory pName, pCount, pFolder, pPlatform, pMultiple
   # OK-2010-02-17: Bug 8608 - Deploying stack with trailing whitespace in name fails on Windows
   # because the folder created doesn't match the name passed to create folder command.
   local tFolderName
   put pName & pCount into tFolderName
   if the platform is "Win32" or pPlatform is "Windows" then
      put replaceText(tFolderName, "[ \t]+$", empty) into tFolderName
   end if
   
   local tDirectory
   if pMultiple then
      put pFolder & tFolderName & "/" & pPlatform & "/" into tDirectory
   else
      put pFolder & tFolderName & "/" into tDirectory
   end if
   return tDirectory
end revSBStandaloneOutputDirectory

command revSBAddInclusion pInclusion, pKey, @xSettings
   if pInclusion is among the lines of xSettings[pKey]  then
      exit revSBAddInclusion
   end if
   
   if xSettings[pKey] is not empty then put return after xSettings[pKey]
   put pInclusion after xSettings[pKey]
end revSBAddInclusion

local sDependencies
private command __InitialiseInclusionDependencies
   // CEF requires revbrowser
   revSBAddDependencyForInclusion "scriptLibraries", "Browser (CEF)", "scriptLibraries", "Browser"
   
   // Database drivers require revdb
   local tDBName
   repeat for each item tDatabaseDriver in revSBDatabaseDriversMapping()
      put word 2 to -1 of tDatabaseDriver into tDBName
      revSBAddDependencyForInclusion "databaseDrivers", tDBName, "scriptLibraries", "Database"
   end repeat
   
   // XMLRPC requires revxml
   revSBAddDependencyForInclusion "scriptLibraries", "XMLRPC", "scriptLibraries", "XML"
end __InitialiseInclusionDependencies

command revSBAddDependencyForInclusion pInclusionType, pInclusion, pDependencyType, pDependency, pPlatforms
   if pPlatforms is empty then
      put revSBAllPlatforms() into pPlatforms
   end if
   repeat for each item tPlatform in pPlatforms
      put true into sDependencies[pInclusionType][pInclusion][pDependencyType][pDependency][tPlatform]
   end repeat
end revSBAddDependencyForInclusion

command revSBRemoveDependencyForInclusion pInclusionType, pInclusion, pDependencyType, pDependency
   delete variable sDependencies[pInclusionType][pInclusion][pDependencyType][pDependency]
end revSBRemoveDependencyForInclusion

command revSBUpdateForDependencies pPlatform, pArchs, pSDK, @xSettings
   local tDependencies
   repeat for each key tDependentKey in sDependencies
      repeat for each key tDependent in sDependencies[tDependentKey]
         if tDependent is among the lines of xSettings[tDependentKey] then
            put sDependencies[tDependentKey][tDependent] into tDependencies
            repeat for each key tRequiredKey in tDependencies
               repeat for each key tRequired in tDependencies[tRequiredKey]
                  if tDependencies[tRequiredKey][tRequired][pPlatform] then
                     revSBAddInclusion tRequired, tRequiredKey, xSettings
                  end if
               end repeat
            end repeat
         end if
      end repeat
   end repeat
   
   -- Update the settings with any additional extension dependencies
   repeat for each line tExtension in xSettings["extensions"]
      local tAndroidMetadataA
      put revIDEExtensionProperty(tExtension, "android") into tAndroidMetadataA
      repeat for each item tPermission in tAndroidMetadataA["permissions"]
         put true into xSettings["android,application permissions"][tPermission]
      end repeat
      repeat for each item tFeature in tAndroidMetadataA["features"]
         local tRequiredMetadata
         put tFeature & ".required" into tRequiredMetadata
         split tRequiredMetadata by "."
         put tAndroidMetadataA[tRequiredMetadata] into \
               xSettings["android,device capabilities"][tFeature]
      end repeat
      
      local tPlayVersion
      put tAndroidMetadataA["play"]["version"]into tPlayVersion
      repeat for each item tPlayDep in tAndroidMetadataA["play"]["deps"]
         appendToStringList tPlayDep & "-" & tPlayVersion, xSettings["android,play dependencies"]
      end repeat
      
      revSBUpdateExtensionCodeInclusions tExtension, pPlatform, pArchs, pSDK, xSettings
      revSBUpdatePerExtensionSettings tExtension, pPlatform, xSettings
   end repeat
end revSBUpdateForDependencies

-- Merge any settings under cRevStandaloneSettings[<ext id>]
command revSBUpdatePerExtensionSettings pExtension, pPlatform, @xSettings
   repeat for each key tKey in xSettings[pExtension]
      if xSettings[tKey] is not empty then
         put return & xSettings[pExtension][tKey] after xSettings[tKey]
      else
         put xSettings[pExtension][tKey] into xSettings[tKey]
      end if
   end repeat
end revSBUpdatePerExtensionSettings

command revSBEnsurePerExtensionSettings pPlatform, pSettings
   local tMissingA
   repeat for each line tExtension in pSettings["extensions"]
      local tInfoA, tRequiredForPlatform
      put revIDEExtensionStandaloneSettingsInfo(tExtension) into tInfoA
      repeat for each key tKey in tInfoA
         set the itemdelimiter to ","
         put (pPlatform is among the items of tInfoA[tKey]["required"]) \
               into tRequiredForPlatform
         
         if not tRequiredForPlatform then
            next repeat
         end if
         
         set the itemdelimiter to ";"
         repeat for each item tItem in tKey
            if pSettings[tExtension][tItem] is empty then
               addToList tInfoA[tKey]["label"], tMissingA[tExtension]
               exit repeat
            end if
         end repeat
      end repeat
   end repeat
   
   local tMissing
   repeat for each key tKey in tMissingA
      appendToStringList "Missing standalone settings for" && tKey & ":", tMissing
      repeat for each element tElement in tMissingA[tKey]
         appendToStringList tab & tElement, tMissing
      end repeat
   end repeat
   
   if tMissing is not empty then
      appendToStringList "Please set on standalone inclusions pane", tMissing
   end if
   return tMissing
end revSBEnsurePerExtensionSettings

command revSBUpdateExtensionCodeInclusions pExtension, pPlatform, pArchs, pSDK, @xSettings
   local tExtCodeFolder, tCodeFolders
   put revIDEExtensionProperty(pExtension, "install_path") & \
         slash & "code" into tExtCodeFolder
   
   put folders(tExtCodeFolder) into tCodeFolders
   filter tCodeFolders without ".*"
   
   repeat for each line tCodeFolder in tCodeFolders
      __UpdateSettingsFromCodeFolder tExtCodeFolder, tCodeFolder, pPlatform, pArchs, pSDK, xSettings
   end repeat
   
   if not revEnvironmentIsInstalled() and \
         tExtCodeFolder begins with revEnvironmentExtensionsPath() then
      -- include builds for other platforms
      local tPlatforms
      if pPlatform is "android" then
         repeat for each item tItem in pArchs
            appendToStringList pPlatform && tItem, tPlatforms
         end repeat
      else
         put pPlatform into tPlatforms
      end if
      
      repeat for each line tPlatform in tPlatforms
         local tNonNativeExtCodeFolder
         put revEnvironmentNonNativeBinariesPath(tPlatform, pSDK) & \
               "/packaged_extensions/" & pExtension & "/code" into tNonNativeExtCodeFolder
         if tNonNativeExtCodeFolder is not tExtCodeFolder and \
               there is a folder tNonNativeExtCodeFolder then
            put folders(tNonNativeExtCodeFolder) into tCodeFolders
            filter tCodeFolders without ".*"
            
            repeat for each line tCodeFolder in tCodeFolders
               __UpdateSettingsFromCodeFolder tNonNativeExtCodeFolder, tCodeFolder, pPlatform, pArchs, pSDK, xSettings
            end repeat
         end if
      end repeat
   end if
end revSBUpdateExtensionCodeInclusions

private command __UpdateSettingsFromCodeFolder pPath, pCodeFolderName, pPlatform, pArchs, pSDK, @xSettings
   local tFiles, tFolderPath
   put pPath & slash & pCodeFolderName into tFolderPath
   put files(tFolderPath) into tFiles
   filter tFiles without ".*"
   switch pCodeFolderName
      case "jvm-android"
         local tAndroidJarFiles, tAndroidAarFiles
         put tFiles into tAndroidJarFiles
         filter tAndroidJarFiles with "*.jar"
         put tFiles into tAndroidAarFiles
         filter tAndroidAarFiles with "*.aar"
         if pPlatform is "android" then
            repeat for each line tLine in tAndroidJarFiles
               appendToStringList tFolderPath & slash & tLine, xSettings["jarFiles"]
            end repeat
            repeat for each line tLine in tAndroidAarFiles
               appendToStringList tFolderPath & slash & tLine, xSettings["aarFiles"]
            end repeat
            if there is a file (tFolderPath & slash & "AndroidManifest.xml") then
               appendToStringList tFolderPath & slash & "AndroidManifest.xml", xSettings["androidManifests"]
            end if
         end if
         break
      case "jvm"
         repeat for each line tLine in tFiles
            appendToStringList tFolderPath & slash & tLine, xSettings["jarFiles"]
         end repeat
         break
      default
         replace comma with return in pArchs
         replace "-" with "_" in pArchs
         replace "i386" with "x86" in pArchs
         replace "x64" with "x86_64" in pArchs
         
         if pPlatform is "MacOSX" then
            put "mac" into pPlatform
         else if pPlatform is "Windows" then
            put "win32" into pPlatform
         end if
         
         if pPlatform is among the items of "mac,ios" then
            if the last char of pArchs is not return then
               put return after pArchs
            end if
            put "universal" after pArchs
         end if
         
         -- on iOS we first look for an exact SDK match for device and simulator
         -- then fallback to a universal binary covering both device and simulator
         -- then fallback to a universal binary compiled against an unknown SDK
         set the itemDelimiter to "-"
         if pPlatform is "ios" and item 3 of pCodeFolderName is not pSDK then
            repeat for each word  tOS in "iphoneos iphonesimulator"
               if pSDK begins with tOS then
                  delete char 1 to length(tOS) of pSDK
               end if
            end repeat
            if item 3 of pCodeFolderName is not empty and  \
                  item 3 of pCodeFolderName is not "sdk" & pSDK then
               break
            end if
         end if
         
         if item 2 of pCodeFolderName is pPlatform and \
               item 1 of pCodeFolderName is among the lines of pArchs then
            repeat for each line tLine in tFiles
               if tLine is empty then
                  next repeat
               end if
               appendToStringList tFolderPath & slash & tLine, xSettings["extension_code_resources"]
            end repeat
            
            if pPlatform is among the words of "mac ios" then
               local tFolders
               put folders(tFolderPath) into tFolders
               filter tFolders without ".*"
               repeat for each line tLine in tFolders
                  if tLine is empty then
                     next repeat
                  end if
                  appendToStringList tFolderPath & slash & tLine, xSettings["extension_code_resources"]
               end repeat
            end if
         end if
         break
   end switch
end __UpdateSettingsFromCodeFolder

command revSBSanitiseExternalsList pTarget, @xExternals
   local tAvailableExternals, tFixedExternals, tValidExternals, tPlatform
   put revSBTargetToPlatform(pTarget) into tPlatform
   put revSBAdditionalExternalsList(tPlatform) into tAvailableExternals
   put revSBExternalsMapping() into tFixedExternals
   split tFixedExternals by comma and space
   put the keys of tFixedExternals into tFixedExternals
   
   local tValid
   repeat for each line tAddedExternal in xExternals
      put false into tValid
      if tAddedExternal is among the lines of tAvailableExternals then
         put true into tValid
      end if
      
      if tAddedExternal is among the lines of tFixedExternals then
         put true into tValid
      end if
      
      if tValid then
         if tValidExternals is empty then
            put tAddedExternal into tValidExternals
         else
            put return & tAddedExternal after tValidExternals
         end if
      end if
   end repeat
   put tValidExternals into xExternals
end revSBSanitiseExternalsList

------------------- SMART CHECKING --------------------

local sPatternsA, sSearchInitialised
on initialiseSearchForInclusions
   if sSearchInitialised then
      exit initialiseSearchForInclusions
   end if
   # AL-2015-01-29: [[ Scriptify revSaveAsStandalone ]] The library autodetection uses various patterns to find library handlers being used.
   # Geometry
   put "cRevGeometry" into sPatternsA["Geometry"]["Property"]
   put "scriptLibraries" into sPatternsA["Geometry"]["Key"]
   
   # Animation
   put "cRevAnimation" into sPatternsA["Animation"]["Property"]
   put "scriptLibraries" into sPatternsA["Animation"]["Key"]
   
   # Table
   put "cRevTable" into sPatternsA["Table"]["Property"]
   put "scriptLibraries" into sPatternsA["Table"]["Key"]
   
   # Database
   put "cRevDatabase" into sPatternsA["Database"]["Property"]
   put "revDatabase,revdb_" into sPatternsA["Database"]["Root"]
   put "revCloseCursor,revCloseDatabase,revCommitDatabase,revCurrentRecord,revDataFromQuery,revExecuteSQL,revMoveToFirstRecord,revMoveToLastRecord,revMoveToNextRecord,revMoveToPreviousRecord,revNumberOfRecords,revOpenDatabase,revQueryDatabase,revQueryDatabaseBLOB,revQueryResult,revRollBackDatabase" into sPatternsA["Database"]["Script"]
   put "scriptLibraries" into sPatternsA["Database"]["Key"]
   
   # Internet
   put "libUrl" into sPatternsA["Internet"]["Root"]
   put "URL,resetAll" into sPatternsA["Internet"]["Script"]
   put "scriptLibraries" into sPatternsA["Internet"]["Key"]
   
   # XML
   put "revXML" into sPatternsA["XML"]["Root"]
   put "revSetDatabaseDriverPath,revAddXMLNode,revAppendXML,revCreateXMLTree,revCreateXMLTreeFromFile,revDeleteAllXMLTrees,revDeleteXMLNode,revDeleteXMLTree,revPutIntoXMLNode,revSetXMLAttribute" into sPatternsA["XML"]["Script"]
   put "scriptLibraries" into sPatternsA["XML"]["Key"]
   
    # XMLRPC
   put "revXMLRPC_" into sPatternsA["XMLRPC"]["Root"]
   put "scriptLibraries" into sPatternsA["XMLRPC"]["Key"]
   
   # Speech
   put "revIsSpeaking,revSetSpeechPitch,revSetSpeechSpeed,revSetSpeechVoice,revSpeak,revSpeechVoices,revStopSpeech,revUnloadSpeech" into sPatternsA["Speech"]["Script"]
   put "scriptLibraries" into sPatternsA["Speech"]["Key"]
   
   # Video Grabber
   put "revCloseVideoGrabber,revInitializeVideoGrabber,revPreviewVideo,revRecordVideo,revSetVideoGrabberRect,revSetVideoGrabSettings,revStopPreviewingVideo,revStopRecordingVideo,revVideoFrameImage,revVideoGrabDialog,revVideoGrabIdle,revVideoGrabSettings" into sPatternsA["Video Grabber"]["Script"]
   put "scriptLibraries" into sPatternsA["Video Grabber"]["Key"]
   
   # SSL & Encryption 
   // AL-2015-04-28: [[ Bug 15288 ]] 'https' requires revsecurity.
   put "secure,encrypt,decrypt,cipherNames,randomBytes,https" into sPatternsA["SSL & Encryption"]["Script"]
   put "scriptLibraries" into sPatternsA["SSL & Encryption"]["Key"]
   
   # Printing
   put "revPrintField,revPrintText,revShowPrintDialog" into sPatternsA["Printing"]["Script"]
   put "scriptLibraries" into sPatternsA["Printing"]["Key"]
   
   # Browser
   put "revBrowser,xBrowser" into sPatternsA["Browser"]["Script"]
   put "scriptLibraries" into sPatternsA["Browser"]["Key"]
   
   # revZip
   put "revZip" into sPatternsA["Revolution Zip"]["Script"]
   put "scriptLibraries" into sPatternsA["Revolution Zip"]["Key"]
   
   repeat for each item tDBDriver in revSBDatabaseDriversMapping()
      local tDBName, tDB
      put word 1 of tDBDriver into tDB
      put word 2 to -1 of tDBDriver into tDBName
      put "revOpenDatabase.*\(.*" & quote & tDB & quote & comma \
            & "revdb_connect.*\(.*" & quote & tDB & quote into sPatternsA[tDBName]["Regex"]
      put "databaseDrivers" into sPatternsA[tDBName]["Key"]
   end repeat
   
   put true into sSearchInitialised
end initialiseSearchForInclusions

command revSBSearchForInclusions pStack, @xSettings
   -- Ensure all properties and patterns are initialised
   initialiseSearchForInclusions
   local tStackFilesList
   revSBGetStackFilesList pStack, tStackFilesList
   
   revSmartChecking tStackFilesList, xSettings
end revSBSearchForInclusions

private command revSmartChecking pStackFilesList, @xSettings
   local tNewSettings
   put xSettings into tNewSettings
   local tPath,tLine,tStack,tList,tHandler,tUserLibs,tStackFile,tCard,tControl,tSave,tStacksList,tDB,tProps,tCardNum,tControlNum,tBg,tEnvironmentIcons, tUserIcons
   local tFiles
   
   lock messages
   
   if tNewSettings["inclusions"] is "search" then
      
      revStandaloneProgress "Auto detecting library inclusions..."
      
      put empty into tNewSettings["scriptLibraries"]
      put true into tNewSettings["cursors"]
      put false into tNewSettings["answerDialog"]
      put false into tNewSettings["askDialog"]
      put false into tNewSettings["brushes"]   --LG-2007-12-13: Bug 4370
      put false into tNewSettings["Magnify"]  -- PM-2015-06-24: [[ Bug 15535 ]] Make possible to include magnify palette in standalones
      put false into tNewSettings["pdfPrinter"]
      
      # OK-2007-11-13 : Support for LiveCode print dialogs
      put false into tNewSettings["revolutionPrintDialogs"]
      
      put empty into tNewSettings["databaseDrivers"]
      put empty into tNewSettings["externals"]
      
      put revAbsoluteFolderListing(revEnvironmentResourcesPath("Script Libraries")) into tEnvironmentIcons
      put revAbsoluteFolderListing(revEnvironmentUserResourcesPath("Script Libraries")) into tUserIcons
      
   end if
   
   local tExtensionIDs
   put xSettings["extensions"] into tExtensionIDs
   split tExtensionIDs by return as set
   
   repeat for each line tStackFile in pStackFilesList
      put the short name of stack tStackFile & return & the substacks of stack tStackFile into tStacksList
      repeat for each line tStack in tStacksList
         if tNewSettings["inclusions"] is "search" then
            # OK-2009-09-07 : Value of password and passkey properties changed, the above test no longer works
            if __ScriptIsLocked(tStack) then
               revStandaloneAddWarning "Could not auto-detect inclusions or security categories because stack is password protected"
               exit revSmartChecking
            end if
         end if
         
         revCheckObject the long name of stack tStack, tUserLibs, tNewSettings, tExtensionIDs
         put the number of cards of stack tStack into tCardNum
         repeat with tCard=1 to tCardNum
            revCheckObject the long id of cd tCard of stack tStack, tUserLibs, tNewSettings, tExtensionIDs
            put the number of controls of cd tCard of stack tStack into tControlNum
            repeat with tControl = 1 to tControlNum
               revCheckObject the long id of control tControl of cd tCard of stack tStack, tUserLibs, tNewSettings, tExtensionIDs
            end repeat
         end repeat
         repeat for each line tBg in the backgroundIDs of stack tStack
            revCheckObject the long id of bg id tBg of stack tStack, tUserLibs, tNewSettings, tExtensionIDs
         end repeat
      end repeat
   end repeat
   
   put the keys of tExtensionIDs into tNewSettings["extensions"]
   -- Update the output settings
   put tNewSettings into xSettings
   unlock messages
end revSmartChecking

private function __ScriptIsLocked pStack
   try
      get the script of stack pStack
   catch tError
      return true
   end try
   return false
end __ScriptIsLocked

private command revCheckObject pObject,pUserLibraries, @xSettings, @xWidgetKinds
   
   if word 1 of pObject is "widget" then
      put true into xWidgetKinds[the kind of pObject]
   end if
   
   if xSettings["inclusions"] is not "search" then
      return empty
   end if
   
   -- Keep the progress indicator updated
   revStandaloneProgress "Auto detecting library inclusions..."
   
   # AL-2015-01-29: [[ Scriptify revSaveAsStandalone ]] Use script locals to check for library inclusions
   local tProps, tLib, tProperty
   put the customPropertySets of pObject into tProps
   repeat for each key tLib in sPatternsA
      local tSettingsKey
      put sPatternsA[tLib]["Key"] into tSettingsKey
      
      # AL-2015-01-29: Don't search the script for handlers if this lib is already included
      if tLib is among the lines of xSettings[tSettingsKey]  then
         next repeat
      end if
      
      repeat for each item tProperty in sPatternsA[tLib]["Property"]
         if lineOffset(tProperty, tProps) is not 0 then
            revSBAddInclusion tLib, tSettingsKey, xSettings
            # AL-2015-01-29: If we are including this lib then go on to the next one
            exit repeat
         end if
      end repeat
   end repeat
   
   local tScript
   put the script of pObject into tScript
   
   if tScript is not empty then
      
      repeat for each key tLib in sPatternsA
         put sPatternsA[tLib]["Key"] into tSettingsKey
         
         # AL-2015-01-29: Don't search the script for handlers if this lib is already included
         if tLib is among the lines of xSettings[tSettingsKey]  then
            next repeat
         end if
         
         local tHandler
         repeat for each item tHandler in sPatternsA[tLib]["Script"]
            if lineOffset(tHandler,tScript) is not 0 then
               revSBAddInclusion tLib, tSettingsKey, xSettings
               # AL-2015-01-29: If we are including this lib then go on to the next one
               exit repeat
            end if
         end repeat
         
         # AL-2015-01-29: Don't search the script for roots if this lib is already included
         if tLib is among the lines of xSettings[tSettingsKey]  then
            next repeat
         end if
         
         local tRoot
         repeat for each item tRoot in sPatternsA[tLib]["Root"]
            if lineOffset(tRoot,tScript) is not 0 then
               revSBAddInclusion tLib, tSettingsKey, xSettings
               # AL-2015-01-29: If we are including this lib then go on to the next one
               exit repeat
            end if
         end repeat
         
         # Allow regex search patterns
         local tRegex
         repeat for each item tRegex in sPatternsA[tLib]["Regex"]
            if matchText(tScript,"(?i)"&tRegex) then
               revSBAddInclusion tLib, tSettingsKey, xSettings
               # AL-2015-01-29: If we are including this lib then go on to the next one
               exit repeat
            end if
         end repeat
      end repeat
      
      -- Detect widgets created in script
      repeat for each key tWidget in revIDEExtensions("widget","installed", false)
         if xWidgetKinds[tWidget] then
            next repeat
         end if
         
         if lineOffset(tWidget, tScript) is not 0 then
            put true into xWidgetKinds[tWidget]
         end if
      end repeat
      
      -- Detect use of library extensions
      local tHandlers
      repeat for each key tLibrary in revIDEExtensions("library","installed", false) 
         if tLibrary is among the lines of xSettings["extensions"] then
            next repeat
         end if
         
         put revIDEExtensionLibraryHandlers(tLibrary) into tHandlers
         repeat for each line tHandler in tHandlers
            if lineOffset(tHandler, tScript) is not 0 then
               revSBAddInclusion tLibrary, "extensions", xSettings
            end if
         end repeat
         
      end repeat
   end if
   
   local tInclusion, tSyntax
   repeat for each item tItem in "answerDialog answer,askDialog ask,pdfPrinter open printing to pdf"
      put word 1 of tItem into tInclusion
      put word 2 to -1 of tItem into tSyntax
      if not xSettings[tInclusion] and lineOffset(tSyntax, tScript) is not 0 then
         put true into xSettings[tInclusion]
      end if
   end repeat
   
   # OK-2007-11-13 : Support for LiveCode print dialogs. The dialogs are included if there is any reference to systemPrintSelector in the script,
   # or if we are building for linux. (This might include the files more often than they are required).
   if offset("systemPrintSelector", tScript) is not 0 then
      put true into xSettings["revolutionPrintDialogs"]
   end if
end revCheckObject


function revSBEnginePath pPlatform
   local tEdition
   put the editionType into tEdition
   // SN-2015-07-09: [[ StandaloneDeployment ]] Gypified Standalone-<edition>.app building does not
   //  allow to create broken applications, so that the executable file in it bears the edition.
   if tEdition is not "community" then
      put "Commercial" into tEdition
   end if
   
   -- MW-2013-06-13: [[ CloneAndRun ]] If not installed, then fetch the engine locally.
   if revEnvironmentIsInstalled() then
      # MW-2009-06-24: For Mac OS X, engine source is two lines, first is PPC (if any), second
      #   is x86 (if any). Both implies universal build.
      -- MM-2014-03-21: [[ PPC Support Dropped ]] We now only support intel Mac builds. Assume all mac builds to be intel.
      local tRuntimePath
      put revEnvironmentRuntimePath() into tRuntimePath
      switch pPlatform
         case "MacOSX"
         case "MacOSX PowerPC-32"
         case "MacOSX x86-32"
         case "MacOSX x86-64"
            // SN-2015-07-09: [[ StandaloneDeployment ]] Gypified Standalone-<edition>.app building does not
            //  allow to create broken applications, so that the executable file in it bears the edition.
            put the upper of char 1 of tEdition into char 1 of tEdition
            return tRuntimePath & "/Mac OS X/x86-32/Standalone.app/Contents/MacOS/Standalone-" & tEdition
            break
         case "Windows"
            return tRuntimePath & "/Windows/x86-32/Standalone"
            break
         case "Windows x86-64"
            return tRuntimePath & "/Windows/x86-64/Standalone"
            break
         case "Linux"
            return tRuntimePath & "/Linux/x86-32/Standalone"
            break
            -- MW-2013-11-06: [[ LinuxX64 ]] Compute the correct path for 64-bit linux.
         case "Linux x64"
            return tRuntimePath & "/Linux/x86-64/Standalone"
            break
            -- FG-2014-08-19: [[ RPi ]] Compute the correct path for ARMv6 Linux
         case "Linux armv6-hf"
            return tRuntimePath & "/Linux/armv6-hf/Standalone"
            break
         default
            return "error"
            break
      end switch
   else
      -- Make sure the platform is correct
      // SN-2015-05-15: [[ DeployAllPlatforms ]] Allow the deployment to happen
      //  for another platform, from the repository.
      local tBinariesPath
      
      if pPlatform contains "MacOSX" and the platform is not "macos" or \
            pPlatform contains "Linux" and the platform is not "linux" or \
            pPlatform is "Windows" and the platform is not "win32" then
         put revEnvironmentNonNativeBinariesPath(pPlatform) into tBinariesPath
      else
         put revEnvironmentBinariesPath() into tBinariesPath
      end if
      
      switch word 1 of pPlatform
         case "MacOSX"
            put the upper of char 1 of tEdition into char 1 of tEdition
            return tBinariesPath & "/Standalone-" & tEdition & ".app/Contents/MacOS/Standalone-" & tEdition
            break;
         case "Windows"
            return tBinariesPath & slash & "standalone-" & tEdition & ".exe"
            break
         case "Linux"
            return tBinariesPath & slash & "standalone-" & toLower(tEdition)
            break
         default
            return empty
            break
      end switch
   end if
end revSBEnginePath

function revSBLicensedToDeployToTarget pTarget
   -- We can only check the license parameters in development mode,
   -- and they are only set in UI mode. So just return true unless
   -- the mode is development.
   if the environment is not "development" then
      return true
   end if
   
   -- In development mode, check the revLicenseInfo
   local tLicensedTargets
   put line 5 of revLicenseInfo into tLicensedTargets
   
   local tPlatform
   put revSBTargetToPlatform(pTarget) into tPlatform
   
   local tLicensePlatform
   switch tPlatform
      case "MacOSX"
         put "Mac OS X" into tLicensePlatform
         break
      case "Emscripten"
         put "HTML5" into tLicensePlatform
         break
      default
         put tPlatform into tLicensePlatform
         break
   end switch
   
   return tLicensePlatform is among the items of tLicensedTargets
end revSBLicensedToDeployToTarget

function revSBNoPlatforms pSettings
   repeat for each item tTarget in kDesktopTargets, kAdditionalTargets
      if pSettings[tTarget] then
         return false
      end if
   end repeat
   
   return true
end revSBNoPlatforms

local sDeployInclusions

command revSBAddDeployInclusion pInclusion, pKey
   put empty into sDeployInclusions[pKey][pInclusion]
end revSBAddDeployInclusion

command revSBRemoveDeployInclusion pInclusion, pKey
   delete variable sDeployInclusions[pKey][pInclusion]
end revSBRemoveDeployInclusion

-- Standalone settings are always set on the mainstack
function revSBGetSettings pStack, pDeploy
   local tSettingsA
   put the customProperties["cRevStandaloneSettings"] of stack \
         (the mainstack of stack pStack) into tSettingsA
   
   # Set empty values to defaults
   revSBDefaultStandaloneSettings pStack, tSettingsA
   
   if pDeploy then
      local tKey, tInclusion
      repeat for each key tKey in sDeployInclusions
         repeat for each key tInclusion in sDeployInclusions[tKey]
            revSBAddInclusion tInclusion, tKey, tSettingsA
         end repeat
      end repeat
   end if
   
   # set library requirements
   local tLibraries
   put __AvailableScriptLibraries() into tLibraries
   repeat for each element tLibrary in tLibraries
      if tLibrary["title"] is among the lines of tSettingsA["scriptLibraries"] then
         union tLibrary["settings"] with tSettingsA recursively
         put tLibrary["settings"] into tSettingsA
      end if
   end repeat
   
   return tSettingsA
end revSBGetSettings

command revSBSetSettings pStack, pSettings
   set the customProperties["cRevStandaloneSettings"] of stack \
         (the mainstack of stack pStack) to pSettings
end revSBSetSettings
