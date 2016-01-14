script "revSaveAsEmscriptenStandalone"
/*
Copyright (C) 2015 LiveCode Ltd.

This file is part of LiveCode.

LiveCode is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License v3 as published by the Free
Software Foundation.

LiveCode is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with LiveCode.  If not see <http://www.gnu.org/licenses/>.  */

-- Support for creating HTML5 standalones using Emscripten

private command logDebug pStep, pMessage
   write "[" & pStep & "]" && pMessage & return to stderr
end logDebug

----------------------------------------------------------------
-- Standalone generation
----------------------------------------------------------------

command revSaveAsEmscriptenStandalone pStack, pOutputFolder
   
   logDebug "top", "----- Begin Emscripten standalone"
   
   start using stack "revSBLibrary"
   
   -- FIXME
   --dispatch "revIDEDeployEmscriptenInitialize" to stack "revDeployLibrary"
   
   local tSettings
   put the customProperties["cRevStandaloneSettings"] of stack pStack \
         into tSettings
   
   ---------- Pre-compute standalone build parameters
   
   -- Initial stack file
   local tStackFile
   put the effective filename of stack pStack into tStackFile
   
   -- Base directory
   local tBaseFolder
   set the itemDelimiter to slash
   put item 1 to -2 of tStackFile into tBaseFolder
   set the itemDelimiter to comma
   
   -- Other parameters
   local tName, tFiles, tAuxStackFiles, tExtensions
   put tSettings["name"] into tName
   put tSettings["files"] into tFiles
   put tSettings["auxiliary_stackfiles"] into tAuxStackFiles
   put tSettings["extensions"] into tExtensions

   -- If the datagrid is used, then include the library as an aux stack
   if "data grid templates" is in the subStacks of stack pStack and there is a stack "revDataGridLibrary" then
      put the effective filename of stack "revDataGridLibrary" & return after tAuxStackFiles
   end if

   local tError
   try
      ---------- Check deployment entitlement
      revStandaloneProgress "Checking deployment entitlement..."
      
      if not isPermittedDeployment() then
         throw "license does not permit HTML5 deployment"
      end if
      
      ---------- Create a working directory
      local tBuildFolder
      put pOutputFolder into tBuildFolder
      
      logDebug "top", "Output folder: " && pOutputFolder
      
      ---------- Create new zip archive
      revStandaloneProgress "Creating standalone archive..."
      
      local tBuildZip
      put tBuildFolder & slash & "standalone.zip" into tBuildZip
      
      logDebug "top", "Archive:" && tBuildZip
      
      revZipOpenArchive tBuildZip, "write"
      
      if the result is not empty then
         throw "could not create standalone archive"
      end if
      
      -- Store template files
      local tZipTemplateFile, tZipTemplateFolder, tZipTemplatePath
      put getStandaloneTemplateFolder() into tZipTemplateFolder
      
      repeat for each element tZipTemplateFile in getStandaloneTemplateFiles()
         put tZipTemplateFolder & slash & tZipTemplateFile into tZipTemplatePath
         logDebug "template", "Storing" && tZipTemplatePath
         revZipAddUncompressedItemWithFile tBuildZip, tZipTemplateFile, tZipTemplatePath
         if the result is not empty then
            throw "could not store template files in standalone archive"
         end if
      end repeat
      
      -- Define some paths within the zip archive
      local tBootPath, tStandalonePath
      local tBootStackPath, tAuxStackPath, tStartupStackPath
      local tExtensionsPath
      put "boot" into tBootPath
      put tBootPath & slash & "__startup.livecode" into tStartupStackPath
      put tBootPath & slash & "standalone" into tStandalonePath
      put tStandalonePath & slash & "__boot.livecode" into tBootStackPath
      put tBootPath & slash & "auxiliary_stackfiles" into tAuxStackPath
      put tBootPath & slash & "extensions" into tExtensionsPath
      
      -- Store the initial stack
      local tBootHash
      storeBootStack tBuildZip, tBootStackPath, tBuildFolder, tStackFile, tBootHash

      -- Store auxiliary stacks
      local tZipAuxPath, tAuxStackFile
      repeat for each line tAuxStackFile in tAuxStackFiles
         set the itemdelimiter to slash
         put item -1 of tAuxStackFile into tZipAuxPath
         put tAuxStackPath & slash before tZipAuxPath
         
         logDebug "aux", "Storing" && tAuxStackFile
         revZipAddUncompressedItemWithFile tBuildZip, tAuxStackPath, tAuxStackFile
         if the result is not empty then
            throw "could not store auxiliary stack in standalone archive"
         end if
      end repeat
      
      -- Store extensions
      storeExtensions tBuildZip, tExtensions, tExtensionsPath
      
      -- Deploy and store the startup script
      storeStartupStack tBuildZip, tStartupStackPath, tBuildFolder, tBootHash
      
      -- Store extra, copied files
      storeAssets tBuildZip, tFiles, tStandalonePath, tBaseFolder
      
      -- Close zip file
      revZipCloseArchive tBuildZip
      if the result is not empty then
         throw "could not create standalone archive:" && the result
      end if
      
      ---------- Copy the engine into the build folder
      revStandaloneProgress "Preparing engine files..."
      local tEngineFile
      repeat for each element tEngineFile in getEngineFiles()
         logDebug "engine", "Copying" && tEngineFile
         revSBCopyFileToFolder tEngineFile, tBuildFolder, \
               "revStandaloneProgressCallback", the long id of me
         
         if the result is not empty then
            throw "could not copy engine file" && the result
         end if
      end repeat
      
      ---------- Create HTML page
      
      -- FIXME just copies the Emscripten-generated page into place
      revStandaloneProgress "Preparing HTML page..."
      
      local tHtmlFile
      put tBuildFolder & slash & tName & ".html" into tHtmlFile
      logDebug "html", "Creating" && tHtmlFile
      logDebug "html", "Copying" && getHtmlTemplateFile()
      revSBCopyFileToFile mapBinPath(getHtmlTemplateFile()), tHtmlFile, \
            "revStandaloneProgressCallback", the long id of me
      
   catch tError
	if tError is empty then
		put "an unknown error occurred" into tError
	end if

      -- Cleanup on failure
      if there is a folder pOutputFolder then
         revDeleteFolder pOutputFolder
      end if
   end try
   
   local tOutcome
   if tError is not empty then
      put "FAILURE" into tOutcome
   else
      put "SUCCESS" into tOutcome
   end if
   
   logDebug "top", "----- (" & tOutcome & ") Emscripten Standalone"
   
   if tError is not empty then
      throw tError
   end if
   
end revSaveAsEmscriptenStandalone

----------------------------------------------------------------
-- LiveCode Builder extensions
----------------------------------------------------------------

private command storeExtensions pZip, pExtensions, pExtensionsPath
   if pExtensions is empty then
      exit storeExtensions
   end if
   
   -- Put extensions in the correct order for loading
   local tExtensions
   put revIDEExtensionsOrderByDependency(pExtensions) into tExtensions
   
   -- Store the list of extensions
   storeExtensionsList pZip, tExtensions, pExtensionsPath
   
   -- Store each extension's data
   local tKind
   repeat for each line tKind in tExtensions
      storeExtensionByKind pZip, tKind, pExtensionsPath
   end repeat
end storeExtensions

private command storeExtensionsList pZip, pExtensions, pExtensionsPath
   local tData, tZipPath
   
   put textEncode(pExtensions, "UTF-8") into tData
   
   put pExtensionsPath & slash & "extensions.txt" into tZipPath
   
   logDebug "lcb", "Storing" && tZipPath
   
   revZipAddUncompressedItemWithData pZip, tZipPath, "tData"
   
   if the result is not empty then
      LogDebug "lcb", the result
      throw "could not store extension list in standalone archive"
   end if
end storeExtensionsList

private command storeExtensionByKind pZip, pKind, pExtensionsPath
   local tInstallFolder
   put revIDEExtensionProperty(pKind, "install_path") into tInstallFolder
   
   local tFiles
   put getExtensionFilesByKind(tInstallFolder) into tFiles
   
   local tZipPath, tFilePath
   repeat for each element tFilePath in tFiles
      put pExtensionsPath & slash & pKind & slash & tFilePath into tZipPath
      put tInstallFolder & slash before tFilePath
      
      logDebug "lcb", "Storing" && tFilePath
      revZipAddUncompressedItemWithFile pZip, tZipPath, tFilePath
      if the result is not empty then
         throw "could not store extension file in standalone archive"
      end if
   end repeat
end storeExtensionByKind

-- For community (GPL) standalones, we need to include the extension in
-- "the preferred form for modification".  I.e. with all of its source code
-- and documentation.  Otherwise, just include compiled extension + resources
private function getExtensionFilesByKind pInstallFolder
   local tExtensionModule, tFiles, tFileCount, tResourceFiles, tFilePath
   
   if isCommercialDeployment() then
      put empty into tFiles
      put 0 into tFileCount
      
      -- All resource files
      put scanFolder(pInstallFolder & slash & "resources") into tResourceFiles
      -- Make resource file paths relative to module install directory
      repeat for each element tFilePath in tResourceFiles
         add 1 to tFileCount
         put "resources" & slash & tFilePath into tFiles[tFileCount]
      end repeat
      
      -- Compiled module file
      add 1 to tFileCount
      put "module.lcm" into tFiles[tFileCount]
      
   else
      -- Include *every* file in the extension
      put scanFolder(pInstallFolder) into tFiles
      
   end if
   
   return tFiles
end getExtensionFilesByKind

----------------------------------------------------------------
-- Startup stack
----------------------------------------------------------------

private command storeStartupStack pZip, pStartupPath, pBuildFolder, pBootHash
   local tTempStack, tTempStackPath, tStartupScript, tDeployInfo

   ---------- Generate the startup stack
   logDebug "startup", "Generating startup stack"

   put pBuildFolder & slash & "revEmscriptenStandaloneStartup.livecode" into tTempStackPath

   put getStartupScript(pBootHash) into tStartupScript

   create invisible stack "revEmscriptenStandaloneStartup"
   put it into tTempStack
   set the script of tTempStack to tStartupScript
   if the result is not empty then
      throw "could not generate startup stack:" && the result
   end if

   save tTempStack as tTempStackPath
   if the result is not empty then
      throw "could not save startup stack:" && the result
   end if
   close tTempStack

   put tTempStackPath into tDeployInfo["output"]
   put empty into tDeployInfo["engine"]

   ---------- Perform startup stack deployment
   logDebug "startup", "Deploying startup stack"

   _internal deploy emscripten tDeployInfo
   if the result is not empty then
      throw "could not deploy startup stack:" && the result
   end if

   ---------- Store startup stack
   logDebug "startup", "Storing" && tTempStackPath
   local tStackData

   -- We do this using data so that we can immediately delete the
   -- temporary stack file.  If used the file, we'd have to keep the
   -- file around until the zip file is finished.

   open file tTempStackPath for binary read
   read from file tTempStackPath until end
   put it into tStackData
   close file tTempStackPath

   revZipAddUncompressedItemWithData pZip, pStartupPath, "tStackData"
   if the result is not empty then
      throw "could not store startup stack in standalone archive:" && the result
   end if

   ---------- Cleanup
   delete file tTempStackPath

end storeStartupStack

-- Create the startup script by processing the startup stack template.
-- Engine-specific and standalone-specific data is substituted into the
-- template in order 
private function getStartupScript pStackHash
   local tTemplateFile, tScript
   local tStackHashString
   
   -- Load template
   put getStartupTemplateFile() into tTemplateFile
   open file tTemplateFile for "UTF-8" text read
   if the result is not empty then
      throw tTemplateFile & ":" && the result
   end if
   read from file tTemplateFile until end
   put it into tScript
   if the result is not "eof" then
      throw tTemplateFile & ":" && the result
   end if
   close file tTemplateFile
   
   -- Trim to the actual script
   delete line 1 of tScript
      
   -- Compute a hex string representation of the boot stack hash
   put empty into tStackHashString
   local tByte, tByteString
   repeat for each byte tByte in pStackHash
      put baseConvert(byteToNum(tByte), 10, 16) into tByteString
      switch the number of chars in tByteString
         case 1
            put "0" after tStackHashString
         case 2
            put tByteString after tStackHashString
      end switch
   end repeat
   
   -- Make substitutions in the startup script
   replace "@ENGINE_VERSION@" with the version in tScript
   replace "@BOOT_HASH@" with tStackHashString in tScript
   
   return tScript
end getStartupScript

----------------------------------------------------------------
-- Boot stack
----------------------------------------------------------------

private command storeBootStack pZip, pBootStackPath, pBuildFolder, pStackFile, @rStackHash
   logDebug "boot-stack", "Storing" && pStackFile
   
   local tStackData
   put getPreparedStackAsData(pStackFile, pBuildFolder) into tStackData
   
   -- Compute the SHA-1 hash of the first 1 kB of the boot stack
   -- This is needed when building the startup script
   put the sha1Digest of (byte 1 to 1024 of tStackData) into rStackHash
   
   revZipAddUncompressedItemWithData pZip, pBootStackPath, "tStackData"
   if the result is not empty then
      throw "could not store initial stack in standalone archive"
   end if   
end storeBootStack

----------------------------------------------------------------
-- Extra files from "copy files"
----------------------------------------------------------------

private command storeAssets pZip, pFiles, pStandalonePath, pBaseFolder
   -- Get a list of all the files to copy
   local tPath, tFiles, tCount
   
   put empty into tFiles
   put 0 into tCount
   
   repeat for each line tPath in pFiles
      getAssetFilesFromPath tPath, pBaseFolder, tFiles, tCount
   end repeat
   
   -- Store all the files in the zip archive
   local tCopyInfo, tZipPath
   
   repeat for each element tCopyInfo in tFiles
      logDebug "asset", "Storing" && tCopyInfo["source"]
      
      put pStandalonePath & slash & tCopyInfo["dest"] into tZipPath
      revZipAddUncompressedItemWithFile pZip, tZipPath, tCopyInfo["source"]
      if the result is not empty then
         throw "could not store asset file in standalone archive"
      end if
   end repeat
end storeAssets

private command getAssetFilesFromPath pPath, pBaseFolder, @xFiles, @xCount
   local tSourcePath, tAssetPath, tFile
   
   mapAssetPath pPath, pBaseFolder, tSourcePath, tAssetPath
   
   if there is a file tSourcePath then
      add 1 to xCount
      put tSourcePath into xFiles[xCount]["source"]
      put tAssetPath into xFiles[xCount]["dest"]
   else if there is a folder tSourcePath then
      repeat for each element tFile in scanFolder(tSourcePath)
         add 1 to xCount
         put tSourcePath & slash & tFile into xFiles[xCount]["source"]
         put tAssetPath & slash & tFile into xFiles[xCount]["dest"]
      end repeat
   else
      throw "could not store asset from '" & pPath & "': not a file or directory"
   end if
end getAssetFilesFromPath

----------------------------------------------------------------
-- Stack manipulation functions
----------------------------------------------------------------

-- Modifies a copy of <pStackFile> to prepare it for deployment,
-- and then returns the modified stack file's contents as data. 
-- <pStackFile> is not modified.
private function getPreparedStackAsData pStackFile, pBuildFolder
   local tStack, tError, tResult
   local tTempFile
   local tStandaloneSettings
   local tStackData
   
   put the long id of stack pStackFile into tStack
   
   if revIDEStackNameIsIDEStack(the name of tStack) then
      -- Don't manipulate IDE stacks if for some reason someone
      -- tries to put one into a standalone
      put the filename of tStack into tTempFile
      
   else
      try
         
         -- Get a temporary filename
         put getTemporaryBuildFile(pBuildFolder, ".livecode") into tTempFile
         
         -- Clear standalone settings
         if isCommercialDeployment() then
            set the customProperties["cRevStandaloneSettings"] of tStack to empty
         else
            -- For Community edition deployment to retain everything
            -- but the deployment password (which isn't relevant to
            -- Community edition anyway).  Keeping the deployment
            -- settings is GPL-required information.
            put the customProperties["cRevStandaloneSettings"] of tStack into tStandaloneSettings
            delete variable tStandaloneSettings["password"]
            set the customProperties["cRevStandaloneSettings"] of tStack to tStandaloneSettings
         end if
         
         -- Save the modified stack to the temporary file, and
         -- destroy it
         lock messages
         lock screen
         
         save tStack as tTempFile with newest format
         put the result into tResult
         if tResult is not empty then
            throw tTempFile & ":" && tResult
         end if
         
         set the filename of tStack to pStackFile
         set the cantDelete of tStack to false
         delete tStack
         
         unlock screen
         unlock messages
         
      catch tError
         if there is a file tTempFile then
            delete file tTempFile
         end if
      end try
      
      if tError is not empty then 
         throw tError
      end if
   end if
   
   
   -- Compute the SHA-1 hash of the first 1 kB of the boot stack
   open file tTempFile for binary read
   if the result is not empty then
      throw tTempFile & ":" && the result
   end if
   read from file tTempFile until end
   put it into tStackData
   if the result is not "eof" then
      throw tTempFile & ":" && the result
   end if
   close file tTempFile
      
   return tStackData
end getPreparedStackAsData

----------------------------------------------------------------
-- File & path helper functions
----------------------------------------------------------------

-- Map a compiled file's path into an absolute filesystem path.  This
-- is used to cope with running from a git repository working tree.
private function mapBinPath pPath
   if there is a file pPath then
      return pPath
   end if
   
   if revEnvironmentIsInstalled() then
      -- We're running from an installed version of LiveCode
      return getRuntimeFolder() & slash & pPath
   else
      -- We're running from a git checkout
      return getRepoBinariesFolder() & slash & pPath
   end if
end mapBinPath

-- Map a resource file's path into an absolute filesystem path.  This
-- is used to cope with running from a git repository working tree.
private function mapResPath pPath
   if there is a file pPath then
      return pPath
   end if

   if revEnvironmentIsInstalled() then
      return getRuntimeFolder() & slash & pPath
   else
      return getRepoResourceFolder() & slash & pPath
   end if
end mapResPath

-- Map a path pPath for asset inclusion.
--
-- Interprets pPath as a user-provided asset source path, defined
-- relative to pBaseFolder.  Computes the absolute path of the file to
-- read from into rSourcePath, and the relative asset path to store the
-- data in into rDestPath.
--
-- This is designed to be compatible with computeAssetLocation in the
-- revSaveAsAndroidStandalone stack.
--
-- FIXME This may have problems if pPath is a root directory or if
-- pPath is not normalized.
private command mapAssetPath pPath pBaseFolder @rSourcePath @rDestPath
   local tIsAbsolute
   put pPath begins with "/" or char 2 to 3 of pPath is ":/" into tIsAbsolute
   
   set the itemDelimiter to slash
   if the last item of pPath is "*" then
      delete the last item of pPath
   end if
   
   if tIsAbsolute then
      put item -1 of pPath into rDestPath
      put pPath into rSourcePath
   else
      put pPath into rDestPath
      put pBaseFolder & slash & pPath into rSourcePath
   end if
end mapAssetPath

-- Scan a directory, returning a number-indexed array of files found.
-- The returned paths are relative to the specified directory.
private function scanFolder pFolder
   local tTemplateRoot, tFiles, tCount, tError
   
   -- Short-circuit if pFolder is missing
   if there is no folder pFolder then
      return empty
   end if
   
   put 0 into tCount
   put empty into tFiles
   
   revIDEPushDefaultFolder pFolder
   try
      scanFolder_recurse tCount, tFiles, empty
   catch tError
   end try
   
   revIDEPopDefaultFolder
   
   if tError is not empty then
      throw tError
   end if
   
   return tFiles
end scanFolder

-- Recursive function used by scanFolder()
private command scanFolder_recurse @xCount, @xFiles, pRelPath
   local tFile, tFilepath, tSaveFolder

   -- List files
   repeat for each line tFile in the files
      -- Skip dotfiles
      if tFile begins with "." then
         next repeat
      end if

      add 1 to xCount
      put tFile into tFilePath
      if pRelPath is not empty then
         put pRelPath & slash before tFilePath
      end if
      put tFilePath into xFiles[xCount]
   end repeat

   -- Recurse
   put the defaultfolder into tSaveFolder
   repeat for each line tFolder in the folders
      if tFolder begins with "." then
         next repeat
      end if

      set the defaultfolder to tSaveFolder & slash & tFolder
      if pRelPath is not empty then
         put pRelPath & slash before tFolder
      end if
      scanFolder_recurse xCount, xFiles, tFolder
   end repeat

   set the defaultFolder to tSaveFolder
end scanFolder_recurse

-- Return a number-indexed array of engine files that need to be
-- copied into the standalone output folder.
private function getEngineFiles
   local tFiles
   
   -- Engine JavaScript
   put mapBinPath("standalone-" & getEngineType() & "-" & the version & ".js") into tFiles[1]
   
   -- Memory initialisation file
   put mapBinPath("standalone-" & getEngineType() & "-" & the version & ".html.mem") into tFiles[2]
   
   return tFiles
end getEngineFiles

-- Get the filename of the Emscripten standalone template folder.
-- This is a folder that contains the default filesystem
-- initialisation data that's automatically included in all
-- standalones (especially fonts).
private function getStandaloneTemplateFolder
   return mapResPath("emscripten-standalone-template")
end getStandaloneTemplateFolder

-- Return a number-indexed array of files in the Emscripten standalone
-- template folder.  The paths returned are relative to the path returned
-- by getStandaloneTemplateFolder()
private function getStandaloneTemplateFiles
   return scanFolder(getStandaloneTemplateFolder())
end getStandaloneTemplateFiles

-- Get the filename of the Emscripten standalone template HTML file.
-- This has some standalone-dependent JavaScript substituted into it
-- to instantiate the engine.
private function getHtmlTemplateFile
   return mapBinPath("standalone-" & getEngineType() & "-" & the version & ".html")
end getHtmlTemplateFile

-- Get the filename of the Emscripten standalone startup script-only
-- stack template.
private function getStartupTemplateFile
   return mapResPath("emscripten-startup-template.livecodescript")
end getStartupTemplateFile

-- Get the directory containing the Emscripten runtime
private function getRuntimeFolder
   
   local tRelFolder, tOverridePath, tUserPath
   
   put slash & "Emscripten" & slash & "js" into tRelFolder
   
   put revOverrideRuntimePath() into tOverridePath
   if tOverridePath is not empty then
      put tRelFolder after tOverridePath
      if there is a folder tOverridePath then
         return tOverridePath
      end if
   end if
   
   put revEnvironmentUserRuntimePath() into tUserPath
   if tUserPath is not empty then
      put tRelFolder after tUserPath
      if there is a folder tUserPath then
         return tUserPath
      end if
   end if
   
   return revEnvironmentRuntimePath() & tRelFolder
end getRuntimeFolder

-- Get the directory where Emscripten binaries end up (specifically,
-- the engine's JavaScript file).
private function getRepoBinariesFolder
	return revEnvironmentRepositoryPath() & slash & "emscripten-bin"
end getRepoBinariesFolder

-- Get the directory where Emscripten resources are kept
private function getRepoResourceFolder
   return revEnvironmentRepositoryPath() & slash & "engine" & slash & "rsrc"
end getRepoResourceFolder

-- Get a temporary filename in <pBuildFolder> with the extension
-- <pExtension>
private function getTemporaryBuildFile pBuildFolder pExtension
   local tFile
   repeat forever
      put pBuildFolder & slash & (the random of 1000000) & pExtension into tFile
      if there is not a file tFile then
         exit repeat
      end if
   end repeat
   return tFile
end getTemporaryBuildFile

----------------------------------------------------------------
-- Other utility functions
----------------------------------------------------------------

-- Check if the license permits HTML5 deployment
private function isPermittedDeployment
   if isCommercialDeployment() then
      return ("HTML5" is among the items of line 5 of the revLicenseInfo)
   else
      return true
   end if
end isPermittedDeployment

-- Check if deploying a commercial standalone
private function isCommercialDeployment
   if revLicenseType() is "community" then
      return false
   else
      return true
   end if
end isCommercialDeployment

-- Get the engine type string ("commercial" or "community")
private function getEngineType
   if isCommercialDeployment() then
      return "commercial"
   else
      return "community"
   end if
end getEngineType
