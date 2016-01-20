script "revSBLibrary"
#?library
# Standalone Builder Library functions
#?author
# Marcus van Houdt

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
on revSBCopyFolderToDestination pSourceFolder, pDestinationFolder, pCallback, pCallbackTarget
  if there is no folder pDestinationFolder then
    return "destination folder not found"
  end if
  set the itemDelimiter to "/"
  local tTargetFolder
  put pDestinationFolder & "/" & item -1 of pSourceFolder into tTargetFolder
  __revSBCopyFolder pSourceFolder, tTargetFolder, pCallback, pCallbackTarget
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
private command __revSBCopyFolder pSource, pTarget, pCallback, pCallbackTarget
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
   put __revSBEnumerateFolder(pSource) into tResourceList
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
            create folder (pTarget & "/" & char 1 to -2 of tResource)
            if the result is not empty then
               put "cannot create subfolder ", the result , tResource into tResult
               exit repeat
            end if
            if the platform is "macos" then -- stat it and set some flags
               local tUser, tGroup, tPermissions
               set the itemDelimiter to "/"
               set the defaultFolder to pSource & "/" & item 1 to -2 of tResource
               get the detailed folders
               filter it with (the urlEncode of item -1 of tResource) & "*"
               set the itemDelimiter to ","
               put item 8 of it into tUser
               put item 9 of it into tGroup
               put item 10 of it into tPermissions
               if tUser is not empty or tGroup is not empty then
                  get shell("chown" && tUser & ":" & tGroup && quote & pTarget & "/" & char 1 to -2 of tResource & quote)
               end if
               if tPermissions is not empty and tPermissions is a number then
                  get shell("chmod" && tPermissions && quote & pTarget & "/" & char 1 to -2 of tResource & quote)
               end if
            end if
            # Creating a folder takes no time, so do not send a callback.
         else if tResource is not empty then
            local tFileCallback
            put pCallback && tNumberCopied, tTotalNumber & comma into tFileCallback
            __revSBCopyFile pSource & "/" & tResource, pTarget & "/" & tResource, tFileCallback, pCallbackTarget
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
   
   return tResult
end __revSBCopyFolder

#?semantics
# Copy a file and it's attributes for as much as we can.
# Sends a callback with the number of bytes currently copied and the number left to copy..
#?author
# Marcus van Houdt
private command __revSBCopyFile pSourceFile, pTargetFile, pCallback, pCallbackTarget
  -- Stat the source file
  local tOldFolder, tDataForkSize, tRsrcForkSize, tPermissions, tUser, tGroup, tFileType
  local tResult
  
  if tResult is empty then
    if there is no file pSourceFile then
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
    put item 2 of it into tDataForkSize
    put item 3 of it into tRsrcForkSize
    put item 8 of it into tUser
    put item 9 of it into tGroup
    put item 10 of it into tPermissions
    put item 11 of it into tFileType
     
    local tTotalSize
    put tDataForkSize + tRsrcForkSize into tTotalSize
     
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
      send pCallback && 0, tTotalSize to pCallbackTarget
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
        send pCallback && tAmountCopied, tTotalSize to pCallbackTarget
      end if
    end repeat
  end if
  
  if pTargetFile is among the lines of the openFiles then
    close file pTargetFile
  end if
  if pSourceFile is among the lines of the openFiles then
    close file pSourceFile
  end if
   
  if tResult is empty then
    if tRsrcForkSize is not 0 and tRsrcForkSize is not empty and the platform is "macos" then
      set the itemDelimiter to ","
      repeat for each line tRsrcDetails in getResources(pSourceFile)
        get copyResource(pSourceFile, pTargetFile, item 1 of tRsrcDetails, item 2 of tRsrcDetails, item 2 of tRsrcDetails)
        add item 4 of tRsrcDetails to tAmountCopied
        if pCallbackTarget is not empty and there is a pCallbackTarget then
          send pCallback && tAmountCopied, tTotalSize to pCallbackTarget
        end if
      end repeat
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
   
  if tOldFolder is not empty then
    set the defaultFolder to tOldFolder
  end if
  if tOldFileType is not empty then
    set the fileType to tOldFileType
  end if
   
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

function revSBEnumerateFolder pFolder
  return __revSBEnumerateFolder(pFolder)
end revSBEnumerateFolder

private function __revSBEnumerateFolder pFolder, pPrefix
   local tOldFolder, tResult
   put the folder into tOldFolder
   set the folder to pFolder
   
   # OK-2008-09-10 : Bug 7147 - Prevent the possibility of an infinite loop by
   # returning empty if the folder cannot be set to pFolder
   if the folder is not pFolder then
      return empty
   end if
   
   repeat for each line tSubFolder in the folders
      if tSubFolder is among the items of ".,.." then
         next repeat
      end if
      # For our purposes we need the folder first, so it can be created ahead of file creation..
      put pPrefix & tSubFolder & "/" & return after tResult
      get __revSBEnumerateFolder(pFolder & "/" & tSubFolder, pPrefix & tSubFolder & "/")
      if it is not empty then
         put it & return after tResult
      end if
   end repeat
   local tFileSize
   repeat for each line tFile in the files
      #put (item 2 of tFile) + (item 3 of tFile) into tFileSize
      #add tFileSize to pTotalSize
      put pPrefix & tFile & return after tResult
      #put pPrefix & (the urlDecode of item 1 of tFile) & tab & tFileSize & return after tResult
   end repeat
   set the folder to tOldFolder
   delete the last char of tResult
   return tResult
end __revSBEnumerateFolder

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
   send "revUpdateStandaloneProgress pMessage" to stack "revStandaloneSettings"
end revStandaloneProgress

# This is an event handler, not a command.
on revStandaloneProgressCallback
  wait 0 ticks with messages
end revStandaloneProgressCallback

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

// SN-2015-01-21: [[ Bug 14371 ]] 'MacOSX' has been replaced by 'MacOSX x86-32'
constant kPlatforms = "Windows,MacOSX x86-32,Linux"

constant kDefaultCreator             = "????"
constant kDefaultDocumentType        = ""
constant kDefaultDocumentExtension   = ""
constant kDefaultDynamicMemory       = "true"
constant kDefaultMinimumSize         = 15000
constant kDefaultPreferredSize       = 15000
constant kDefaultRegion              = 1
constant kDefaultRelease             = "Final"
constant kDefaultVersionNumber       = "1.0.0.0"
constant kDefaultScriptLibraries     = "Internet,Geometry,Printing"
constant kAskDialog                  = "true"
constant kAnswerDialog               = "true"
constant kCursors                    = "false"

on revDefaultStandaloneSettings pStack
   local tStandaloneSettingsA,tPlatform,tName
   put the customproperties["cRevStandaloneSettings"] of stack pStack into tStandaloneSettingsA
   -- name
   if tStandaloneSettingsA["name"] = "" then put pStack into tStandaloneSettingsA["name"]
   put char 1 to 246 of tStandaloneSettingsA["name"] into tStandaloneSettingsA["name"]
   if tStandaloneSettingsA["inclusions"] = "" then put "search" into tStandaloneSettingsA["inclusions"]
   if tStandaloneSettingsA["OSX,name"] = "" then put pStack into tStandaloneSettingsA["OSX,name"]
   if tStandaloneSettingsA["Windows,ProductName"] = "" then put pStack into tStandaloneSettingsA["Windows,ProductName"]
   -- main resources
   if tStandaloneSettingsA["askDialog"] = "" then 
      put kAskDialog into tStandaloneSettingsA["askDialog"]
      -- assume no properties set
      if tStandaloneSettingsA["scriptLibraries"] = "" then put replaceText(kDefaultScriptLibraries,",",cr) into tStandaloneSettingsA["scriptLibraries"]
      
      put revEnvironmentRuntimePath() & "/Windows/x86-32/Support/Sample Icons" into tName
      if tStandaloneSettingsA["Windows,iconFile"] = "" then put tName & "/genericapp.ico" into tStandaloneSettingsA["Windows,iconFile"]
      if tStandaloneSettingsA["Windows,documenticonFile"] = "" then put tName & "/genericdoc.ico" into tStandaloneSettingsA["Windows,documenticonFile"]
   end if
   if tStandaloneSettingsA["answerDialog"] = "" then put kAnswerDialog into tStandaloneSettingsA["answerDialog"]
   if tStandaloneSettingsA["cursors"] = "" then put kCursors into tStandaloneSettingsA["cursors"]
   -- version
   set the itemDel to "."
   repeat with X=1 to the number of items of kDefaultVersionNumber
      if tStandaloneSettingsA["Windows,fileversion"&X] = "" then put item X of kDefaultVersionNumber into tStandaloneSettingsA["Windows,fileversion"&X]
      if tStandaloneSettingsA["Windows,productversion"&X] = "" then put item X of kDefaultVersionNumber into tStandaloneSettingsA["Windows,productversion"&X]
   end repeat
   set the itemDel to ","
   if tStandaloneSettingsA["OSX,longVersion"] = "" then put pStack&&kDefaultVersionNumber into tStandaloneSettingsA["OSX,longVersion"]
   if tStandaloneSettingsA["OSX,info"] = "" then put pStack&&"Version"&&kDefaultVersionNumber into tStandaloneSettingsA["OSX,info"]
   if tStandaloneSettingsA["OSX,shortVersion"] = "" then put kDefaultVersionNumber into tStandaloneSettingsA["OSX,shortVersion"]
   if tStandaloneSettingsA["Windows,FileDescription"] = "" then put pStack&&kDefaultVersionNumber&&"for Windows" into tStandaloneSettingsA["Windows,FileDescription"]
   -- document types
   if the number of chars of tStandaloneSettingsA["OSX,signature"] <> 4 then put kDefaultCreator into tStandaloneSettingsA["OSX,signature"]
   if tStandaloneSettingsA["OSX,documentType"] = "" then put kDefaultDocumentType into tStandaloneSettingsA["OSX,documentType"]
   if tStandaloneSettingsA["OSX,documentExtension"] = "" then put kDefaultDocumentExtension into tStandaloneSettingsA["OSX,documentExtension"]
   -- set the unix stuff to true because they will only be imported into unix builds anyway
   if tStandaloneSettingsA["UNIX,colorChooser"] = "" then put true into tStandaloneSettingsA["UNIX,colorChooser"]
   if tStandaloneSettingsA["UNIX,printerChooser"] = "" then put true into tStandaloneSettingsA["UNIX,printerChooser"]
   if tStandaloneSettingsA["UNIX,pageSetup"] = "" then put true into tStandaloneSettingsA["UNIX,pageSetup"]
   if tStandaloneSettingsA["UNIX,fileSelector"] = "" then put true into tStandaloneSettingsA["UNIX,fileSelector"]
   -- build for every platform we have engines for
   repeat for each item tPlatform in kPlatforms
      if tStandaloneSettingsA[tPlatform] = "" then 
            put revEngineCheck(tPlatform) into tStandaloneSettingsA[tPlatform]
      end if
   end repeat
   
   -- copyright
   get the seconds
   convert it to dateItems
   if tStandaloneSettingsA["OSX,copyright"] = "" then put item 1 of it&&the cUserOrganisation of stack "Home"&&"All rights reserved worldwide" into tStandaloneSettingsA["OSX,copyright"]
   if tStandaloneSettingsA["OSX,identifier"] = "" then put "com."&replaceText(toLower(the cUserOrganisation of stack "Home")," ","")&"."&replaceText(toLower(tStandaloneSettingsA["name"])," ","") into tStandaloneSettingsA["OSX,identifier"]
   if tStandaloneSettingsA["Windows,LegalCopyright"] = "" then put item 1 of it&&the cUserOrganisation of stack "Home"&&"All rights reserved worldwide" into tStandaloneSettingsA["Windows,LegalCopyright"]
   if tStandaloneSettingsA["Windows,companyname"] = "" then put the cUserOrganisation of stack "Home" into tStandaloneSettingsA["Windows,companyname"]
   
   -- Since the intel choice, set the default here:
   -- MM-2014-03-21: [[ PPC Support Dropped ]] Only create standalone setting for Intel (for OS X) by default.
   if tStandaloneSettingsA["MacOSX x86-32"] is empty then put true into tStandaloneSettingsA["MacOSX x86-32"]
   
   -- Auto-remove stack passwords from community stacks
   if revLicenseType() is "community" then
      if tStandaloneSettingsA["password"] is not empty then
         delete variable tStandaloneSettingsA["password"]
      end if
   end if
   
   set the customproperties["cRevStandaloneSettings"] of stack pStack to tStandaloneSettingsA
end revDefaultStandaloneSettings

local sStackFiles

function revRelativeStackFilesList pStack
  local tLine,tChars,tStackFiles,tMainStackPath
  put revGetStackFilesList(pStack) into tStackFiles["full"]
  set the itemDel to "/"
  put item 1 to -2 of line 1 of tStackFiles["full"] & "/" into tMainStackPath
  put the number of chars of tMainStackPath into tChars
  repeat for each line tLine in tStackFiles["full"]
    if char 1 to tChars of tLine = tMainStackPath then delete char 1 to tChars of tLine
    put tLine & cr after tStackFiles["relative"]
  end repeat
  delete char -1 of tStackFiles["relative"]
  return tStackFiles
end revRelativeStackFilesList

function revGetStackFilesList pStack
  put "" into sStackFiles
  put the effective fileName of stack pStack into sStackFiles
  revRecursiveStackFileList pStack
  return sStackFiles
end revGetStackFilesList

on revRecursiveStackFileList pStack
  local tLine,tFileName
  set the itemDel to "/"
  put item 1 to -2 of the fileName of stack pStack&"/" into tFileName
  set the itemDel to ","
  repeat for each line tLine in the stackFiles of stack pStack
    if there is not a stack item 2 of tLine then put tFileName before item 2 of tLine
    if there is a stack item 2 of tLine then
      if the effective fileName of stack item 2 of tLine is not among the lines of sStackFiles then 
        put cr& the effective fileName of stack item 2 of tLine after sStackFiles
        revRecursiveStackFileList item 2 of tLine
      end if
    end if
  end repeat
  repeat for each line tLine in the substacks of stack pStack
    revRecursiveStackFileList tLine
  end repeat
end revRecursiveStackFileList

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
            return there is a directory (tPath & "/Mac OS X/x86-32/Standalone.app")
         case "Windows"
            if there is a file (tPath & "/Windows/x86-32/Standalone") then
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
  if pTargetPlatform is "Mac OS X" and ((the platform is "MacOS" and char 1 of the systemVersion is "1") or (the platform is "Win32") or (the platform is "Linux")) then
    put "Icon Resource Files|icns|ICNS" after tTypes
  else if pTargetPlatform is "Windows" and (the platform is "Win32" or the platform is "Linux" or (the platform is "MacOS" and char 1 of the systemVersion is "1")) then
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

command revSBResaveScriptOnlyStackAsProperStackFile pStackName, pStackFilePath, pOutputStackFilePath
   lock messages
   if there is a stack pStackName then
      set the name of stack pStackName to "__keep_this_around__"
   end if
   create invisible stack "__temp_real_stack__"
   set the script of stack "__temp_real_stack__" to the script of stack pStackFilePath
   set the name of stack "__temp_real_stack__" to pStackName
   save stack pStackName as pOutputStackFilePath
   delete stack pStackName
   if there is a stack "__keep_this_around__" then
      set the name of stack "__keep_this_around__" to pStackName
   end if
   unlock messages
end revSBResaveScriptOnlyStackAsProperStackFile

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
         return "rev" & pLib & "Library"
   end switch
   return pLib
end revSBStackNameFromScriptLibraryName

function revSBLibraryList
   # AL-2105-03-05: [[ Bug 14813 ]] Use array key method to ignore duplicates
   local tPath,tSave,tLibrariesA,tBtn,tExternals
   -- rev script libraries
   # AL-2105-03-05: [[ Bug 14813 ]] Use static list of libs rather than generate from loaded libs list
   // SN-2015-10-19: [[ Bug 16238 ]] Add DataGrid library amongst the libraries
   repeat for each item tItem in "Animation,DataGrid,Geometry,Internet,Printing,Table,XMLRPC"
      put empty into tLibrariesA[tItem]
   end repeat
   
   -- rev externals
   get revExternalsList()
   repeat for each line tLine in it
      put empty into tLibrariesA[tLine]
   end repeat
   
   -- user script libraries
   
   local tEnvironmentScripts
   put revAbsoluteFolderListing(revEnvironmentResourcesPath("Script Libraries")) into tEnvironmentScripts
   
   local tUserScripts
   put revAbsoluteFolderListing(revEnvironmentUserResourcesPath("Script Libraries")) into tUserScripts
   
   local tFiles
   put revCombineFilePaths(tUserScripts,tEnvironmentScripts) into tFiles
   repeat for each line tLine in tFiles
      if there is a stack tLine then put empty into tLibrariesA[ the short name of stack tLine]
   end repeat
   put empty into tLibrariesA["SSL & Encryption"]
   if "Browser" is among the keys of tLibrariesA then
      put empty into tLibrariesA["Browser (CEF)"]
   end if

   return the keys of tLibrariesA
end revSBLibraryList
