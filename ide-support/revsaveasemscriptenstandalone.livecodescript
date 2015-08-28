script "revSaveAsEmscriptenStandalone"
/*
Copyright (C) 2015 Runtime Revolution Ltd.

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
   local tName, tFiles
   put tSettings["name"] into tName
   put tSettings["files"] into tFiles
   
   local tError
   try
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
      local tBootPath, tStandalonePath, tBootStackPath
      put "boot" into tBootPath
      put tBootPath & slash & "standalone" into tStandalonePath
      put tStandalonePath & slash & "__boot.livecode" into tBootStackPath
      
      -- Store the initial stack
      logDebug "boot-stack", "Storing" && tStackFile
      revZipAddUncompressedItemWithFile tBuildZip, tBootStackPath, tStackFile
      if the result is not empty then
         throw "could not store initial stack in standalone archive"
      end if
      
      -- Close zip file
      revZipCloseArchive tBuildZip
      if the result is not empty then
         throw "could not create standalone archive"
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

-- Return a number-indexed array of engine files that need to be
-- copied into the standalone output folder.
private function getEngineFiles
   local tFiles
   
   -- Engine JavaScript
   put mapBinPath("standalone-" & revLicenseType() & "-" & the version & ".js") into tFiles[1]
   
   -- Memory initialisation file
   put mapBinPath("standalone-" & revLicenseType() & "-" & the version & ".html.mem") into tFiles[2]
   
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
   local tSaveFolder, tTemplateRoot, tFiles, tCount
   
   -- Short-circuit if the template directory is missing
   put getStandaloneTemplateFolder() into tTemplateRoot
   if there is no folder tTemplateRoot then
      logDebug "template", "No template in" && tTemplateRoot
      return empty
   end if
   
   put 0 into tCount
   put empty into tFiles
   put the defaultFolder into tSaveFolder
   
   set the defaultfolder to tTemplateRoot
   getStandaloneTemplateFiles_Impl tCount, tFiles, empty
   
   set the defaultFolder to tSaveFolder
   return tFiles
end getStandaloneTemplateFiles

private command getStandaloneTemplateFiles_Impl @xCount, @xFiles, pRelPath
   local tFile, tFilePath, tSaveFolder
   
   logDebug "template", "Scanning" && the defaultfolder
   
   -- List files
   repeat for each line tFile in the files
      -- Skip dotfiles
      if tFile begins with "." then
         next repeat
      end if

      add 1 to xCount
      put pRelPath & slash & tFile into tFilePath
      put tFilePath into xFiles[xCount]
      logDebug "template", xCount & ":" && slash & tFilePath
   end repeat
   
   -- Recurse
   put the defaultFolder into tSaveFolder
   repeat for each line tFolder in the folders
      if tFolder is "." or tFolder is ".." then
         next repeat
      end if
      
      set the defaultFolder to tSaveFolder & slash & tFolder
      if pRelPath is not empty then
         put pRelPath & slash before tFolder
      end if
      getStandaloneTemplateFiles_Impl xCount, xFiles, tFolder
   end repeat
   set the defaultFolder to tSaveFolder
end getStandaloneTemplateFiles_Impl

-- Get the filename of the Emscripten standalone template HTML file.
-- This has some standalone-dependent JavaScript substituted into it
-- to instantiate the engine.
private function getHtmlTemplateFile
   return mapBinPath("standalone-" & revLicenseType() & "-" & the version & ".html")
end getHtmlTemplateFile

-- Get the directory containing the Emscripten runtime
private function getRuntimeFolder
   
   local tRelFolder, tOverridePath, tUserPath
   
   put slash & "Emscripten" into tRelFolder
   
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
