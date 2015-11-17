script "revSaveAsStandalone"

#####################################################################
#  Standalone Application Builder
#  @Version: 1.2.1
#
#  Developed from Distribution Builder Code by Monte goulding
#  @Email: monte@sweattechnologies.com
#
#  The Standalone Application Builder relies on a custom property
#  set in each stack named cRevStandaloneSettings.
#
#  Platform specific properties are in muti-dimensional form
#  <platform>,<setting>
#
#  Stacks included in the Standalone Application are the stackFiles
#  of the standalone and all of their stackFiles recusively. In
#  this way the file dependencies in development are transferred into
#  the application without user intervention.
#
#####################################################################

private function revListFolders pPath
   local tOldFolder, tFolders
   
   put the folder into tOldFolder
   
   set the folder to pPath
   put the folders into tFolders
   set the folder to tOldFolder
   
   filter tFolders without "[.][.]" -- filter out ".." folder
   return tFolders
end revListFolders

private function revListFiles pPath
   local tOldFolder, tFiles
   
   put the folder into tOldFolder
   
   set the folder to pPath
   put the files into tFiles
   set the folder to tOldFolder
   
   return tFiles   
end revListFiles

local sDirectoryList        -- the list of build directories
local sPlatformList         -- the list of build platforms
local sResetA               -- stored info for resetting to the pre-build state
local sGenericNumber        -- a simple counter for the generic file naming
local sStandaloneSettingsA  -- a copy of the cRevStandaloneSettings of stack pFolder
local sIconA                -- array for icons

# OK-2007-08-09 : Temporary storage for list of user script libraries. This is required to support building for ialiMac OS Classic
local sUserScriptLibraries

-- MW-2013-11-06: [[ LinuxX64 ]] Add Linux x64 to the list of supported platforms.
-- MM-2014-03-21: [[ PPC Support Dropped ]] Remove Mac PPC and Mac Universal from supported platforms.
-- FG-2014-08-19: [[ RPi ]] Add Linux ARMv6-HF to the list of supported platforms
constant kPlatforms = "Windows,MacOSX x86-32,Linux,Linux x64,Linux armv6-hf,Emscripten"

-- list of info fields for the windows engine
constant kVersionFlds = "Comments,CompanyName,FileDescription,FileVersion,InternalName,LegalCopyright,LegalTrademarks,OriginalFilename,PrivateBuild,ProductName,ProductVersion,SpecialBuild"

# AL-2015-01-29: [[ Scriptify revSaveAsStandalone ]] Initialise script locals here to replace custom props of stack
local sPatternsA, sWindowsMaxLengthsA, sInitialised
on initialiseLibrary
   if sInitialised is true then
      exit initialiseLibrary
   end if
   # AL-2015-01-29: [[ Scriptify revSaveAsStandalone ]] The library autodetection uses various patterns to find library handlers being used.
   # Geometry
   put "cRevGeometry" into sPatternsA["Geometry"]["Property"]
   
   # Animation
   put "cRevAnimation" into sPatternsA["Animation"]["Property"]
   
   # Table
   put "cRevTable" into sPatternsA["Table"]["Property"]
   
   # Database
   put "cRevDatabase" into sPatternsA["Database"]["Property"]
   put "revDatabase,revdb_" into sPatternsA["Database"]["Root"]
   put "revCloseCursor,revCloseDatabase,revCommitDatabase,revCurrentRecord,revCurrentRecordIsFirst,revCurrentRecordIsLast,revDataFromQuery,revExecuteSQL,revMoveToFirstRecord,revMoveToLastRecord,revMoveToNextRecord,revMoveToPreviousRecord,revNumberOfRecords,revOpenDatabase,revOpenDatabases,revQueryDatabase,revQueryDatabaseBLOB,revQueryResult,revRollBackDatabase" into sPatternsA["Database"]["Script"]
   
   # Internet
   put "libUrl" into sPatternsA["Internet"]["Root"]
   put "cachedURL,cachedURLs,ftp,http,https,load,post,resetAll,URL,URLStatus" into sPatternsA["Internet"]["Script"]
   
   # XML
   put "revXML" into sPatternsA["XMLRPC"]["Root"]
   put "revSetDatabaseDriverPath,revAddXMLNode,revAppendXML,revCreateXMLTree,revCreateXMLTreeFromFile,revDeleteAllXMLTrees,revDeleteXMLNode,revDeleteXMLTree,revPutIntoXMLNode,revSetXMLAttribute" into sPatternsA["XMLRPC"]["Script"]
   
   # Speech
   put "revIsSpeaking,revSetSpeechPitch,revSetSpeechSpeed,revSetSpeechVoice,revSpeak,revSpeechVoices,revStopSpeech,revUnloadSpeech" into sPatternsA["Speech"]["Script"]
   
   # Video Grabber
   put "revCloseVideoGrabber,revInitializeVideoGrabber,revPreviewVideo,revRecordVideo,revSetVideoGrabberRect,revSetVideoGrabSettings,revStopPreviewingVideo,revStopRecordingVideo,revVideoFrameImage,revVideoGrabDialog,revVideoGrabIdle,revVideoGrabSettings" into sPatternsA["Video Grabber"]["Script"]
   
   # SSL & Encryption 
   // AL-2015-04-28: [[ Bug 15288 ]] 'https' requires revsecurity.
   put "secure,encrypt,decrypt,cipherNames,randomBytes,https" into sPatternsA["SSL & Encryption"]["Script"]
   
   # Printing
   put "revPrintField,revPrintText,revShowPrintDialog" into sPatternsA["Printing"]["Script"]
   
   # Browser
   put "revBrowser,xBrowser" into sPatternsA["Browser"]["Script"]
   
   # revFont
   put "revFontLoad,revFontUnload"into sPatternsA["Font Support"]["Script"]
   
   # revZip
   put "revZip" into sPatternsA["Revolution Zip"]["Script"]
   
   put true into sInitialised
end initialiseLibrary

function getWindowsMaxLength pField
   return sWindowsMaxLengthsA[pField]
end getWindowsMaxLength

#####################################################################
# Main public API for the Standalone Application Builder
#
# Both parameters are optional.
# @param pStack  = the stack to become a standalone to build a stack other than the topStack
# @param pFolder = the build folder to avoid the script asking
#
#####################################################################

local sStubPath

# OK-2009-05-22 : Added ability to remember default folder
private function getBuildFolder pStack, pFolder, pSettings
   # If the standalone builder has been passed a folder to build in from outside, we just use this.
   if pFolder is not empty then
      return pFolder
   end if
   
   # This means the user has chosen to build directly to the default folder without prompt (and the folder exists) so we just go for it.
   if pSettings["defaultBuildFolder"] is not empty  and pSettings["automaticallyBuildInDefaultFolder"] and there is a folder (pSettings["defaultBuildFolder"]) then
      return pSettings["defaultBuildFolder"]
   end if
   
   # If a default folder is chosen and exists, but the user does not want to automatically build there, we 
   # prompt as normal, but with the difference being that we use the specified folder as the default path,
   # saving browsing for it.
   local tFolder
   if pSettings["defaultBuildFolder"] is not empty and there is a folder (pSettings["defaultBuildFolder"]) then
      
      if pStack is among the lines of the openStacks and the visible of stack pStack and the mode of stack pStack <= 2 then
         set the defaultStack to pStack
         answer folder "Choose a folder to build in:" with pSettings["defaultBuildFolder"] 
      else
         answer folder "Choose a folder to build in:" with pSettings["defaultBuildFolder"] 
      end if
   else
      if pStack is among the lines of the openStacks and the visible of stack pStack and the mode of stack pStack <= 2 then
         set the defaultStack to pStack
         answer folder "Choose a folder to build in:"
      else
         answer folder "Choose a folder to build in:" 
      end if
   end if
   
   if it is "" or the result is "Cancel" then
      --exit to top
      return empty
   end if
   
   put it into tFolder
   
   # If the folder was changed, then we flag the stack as edited so it gets remembered
   if the cRevStandaloneSettings["defaultBuildFolder"] of stack pStack is not tFolder then
      set the cRevStandaloneSettings["defaultBuildFolder"] of stack pStack to tFolder
      revSetEdited pStack
   end if
   
   return tFolder
end getBuildFolder

on revSaveAsStandalone pStack, pFolder, pPreview
   initialiseLibrary
   
   global gREVStackStatus
   
   start using stack "revSBLibrary"
   
   revStandaloneResetWarnings
   
   local tError, tStandaloneFolder
   
   if pStack = empty then
      put the short name of the topStack into pStack
   end if
   if the fileName of stack pStack = empty then
      put "edited" into gREVStackStatus[pStack]
   end if
   
   set the defaultStack to pStack
   
   # The following line used to be in revCreateStandalone, however, if we check below we see that pStack would always be the mainstack
   # as that is the stack that has its cRevStandaloneSettings custom property set. This explains the line "the mainstack of pStack" here.
   put the customProperties["cRevStandaloneSettings"] of stack (the mainstack of stack pStack) into sStandaloneSettingsA
   
   -- MM-2014-03-21: [[ PPC Support Dropped ]] Assume all mac builds to now be Intel.
   if sStandaloneSettingsA["MacOSX"] then
      delete variable sStandaloneSettingsA["MacOSX"]
      put true into sStandaloneSettingsA["MacOSX x86-32"]
   end if
   if sStandaloneSettingsA["MacOSX PowerPC-32"] then
      delete variable sStandaloneSettingsA["MacOSX PowerPC-32"]
      put true into sStandaloneSettingsA["MacOSX x86-32"]
   end if
   
   -- Make sure there is actually something to build..
   # Check if any platforms have actually been selected..
   local tCanBuild
   put false into tCanBuild
   
   local tMessage 
   put "You have not selected any platforms to build for. Please do so using Standalone Settings." into tMessage
   repeat for each item tPlatform in kPlatforms
      put tCanBuild or sStandaloneSettingsA[tPlatform] into tCanBuild
   end repeat
   put tCanBuild or sStandaloneSettingsA["ios"] into tCanBuild
   put tCanBuild or sStandaloneSettingsA["android"] into tCanBuild
   put tCanBuild or sStandaloneSettingsA["emscripten"] into tCanBuild
   if not tCanBuild then
      if revTestEnvironment() then
         return tError
      else
         answer error tMessage
      end if
      exit revSaveAsStandalone
   end if
   
   # OK-2008-03-13 : Bug 5874. Do a complete check to see if the stack needs to be saved here
   # so we can remove the later check from revCloseOpenStacks.
   local tEdited
   put false into tEdited
   
   # Find out if the stack or any substack has been edited
   global gREVStackStatus
   if gREVStackStatus[the short name of stack pStack] is "edited" then
      put true into tEdited
   else
      local tSubstacks
      put the substacks of stack pStack into tSubstacks
      repeat for each line tSubstack in tSubstacks
         if gREVStackStatus[the short name of stack tSubstack] is "edited" then
            put true into tEdited
         end if
      end repeat
   end if
   
   # OK-2010-03-31: Ensure that the standalone name doesn't contain stuff that is not allowed in folder names,
   # otherwise this could cause problems. In particular a return char at the end of the name causes the cleanup
   # routines to try and delete the parent folder, which can result in data loss. The data loss potential has also
   # been eliminated at a lower level by redirecting all calls to revDeleteFolder through a wrapper, which checks that
   # the folder is safe to delete using some rather strict tests. This may result in ocassional problems with cleanup not
   # working.
   if folderNameIsIllegal(sStandaloneSettingsA["name"]) then
      put folderNameMakeLegal(sStandaloneSettingsA["name"]) into sStandaloneSettingsA["name"]
      set the cRevStandaloneSettings["name"] of stack pStack to sStandaloneSettingsA["name"]
      put true into tEdited
      revStandaloneAddWarning "Standalone name was changed because it contained illegal characters"
   end if
   
   # If any have been edited, force the save dialog by setting edited on the mainstack
   if tEdited then
      revSetEdited the short name of stack pStack
   end if
   
   # OK-2009-05-22 : Added ability to remember default folder. We now do the folder check before saving the stack,
   # so that if the user chose a different folder, it can be remembered for next time.
   local tFolder
   put getBuildFolder(pStack, pFolder, sStandaloneSettingsA) into tFolder
   if tFolder is empty then
      exit to top
   end if
   
   put tFolder into pFolder
   put tFolder into sStandaloneSettingsA["defaultBuildFolder"]
   
   # Do the check and continue building as normal.
   if revSaveCheck(pStack) then
      revStandaloneProgress "Preparing to build standalone..."
      
      put the mainStack of stack pStack into pStack
      set the defaultStack to pStack
      try
         if sStandaloneSettingsA["ios"] or sStandaloneSettingsA["android"] or sStandaloneSettingsA["emscripten"] then
            local tOutputMobileFolder
            revOutputDirectories tFolder & slash
            put the result into tOutputMobileFolder
            if there is no folder tOutputMobileFolder then
               create folder tOutputMobileFolder
            end if
            
            ## [[ Bug 12014 ]]
            ## On Window if iOS and Android are selected iOS could not be deselected ut Android building was never reached
            ## previously IF sSandaloneSettingsA["ios"] then ELSE if sStandaloneSettingsA["android"] THEN
            ## meant Android building was not being reached/built
            
            if sStandaloneSettingsA["ios"] then
               --revIDEDeployBuild "ios", pStack, tOutputMobileFolder
               ## Bug 10868
               ## If windows and iOS checked (copied from Mac) then ignore
               if the platform is "Win32" then
                  ## Do nothing
               else
                  dispatch "revSaveAsMobileStandalone" to stack "revSaveAsIOSStandalone" with \
                        pStack, tOutputMobileFolder & slash & sStandaloneSettingsA["name"] & ".app", "Device"
               end if
            end if
            
            if sStandaloneSettingsA["android"] then
               --revIDEDeployBuild "android", pStack, tOutputMobileFolder
               dispatch "revSaveAsMobileStandalone" to stack "revSaveAsAndroidStandalone" with \
                     pStack, tOutputMobileFolder & slash & sStandaloneSettingsA["name"] & ".apk", "Build"
            end if
            
            if sStandaloneSettingsA["emscripten"] then
               dispatch "revSaveAsEmscriptenStandalone" to stack "revSaveAsEmscriptenStandalone" with \
                     pStack, tOutputMobileFolder, "Build"
            end if
         else
            revCreateStandalone pStack,pFolder
         end if
      catch tError
         revAbort
         if word 1 of tError = "Standalone:" then 
            if revTestEnvironment() then
               return word 2 to -1 of tError
            else
               answer error word 2 to -1 of tError
            end if
         else  
            if revTestEnvironment() then
               return tError
            else
               answer error "There was an error while saving the standalone application"&cr&tError
            end if
         end if
         exit to top
      end try
      
      revReset
      
      if revStandaloneGetWarnings() is not empty then
         set the cWarnings of stack "revBuildResults" to revStandaloneGetWarnings()
      else
         if not revTestEnvironment() then
            answer information "Standalone application saved successfully."
         end if
         
         if pPreview is not "false" and sStubPath is not empty then
            launch document sStubPath
         end if
      end if
   end if
end revSaveAsStandalone

private function folderNameIllegalChars
   return revFolderNameIllegalChars()
end folderNameIllegalChars

private function folderNameIsIllegal pName
   return revFolderNameIsIllegal(pName)
end folderNameIsIllegal

private function folderNameMakeLegal pName
   return revFolderNameMakeLegal(pName)
end folderNameMakeLegal

private function revStandalonePlatformDetails pPlatform
   local tDetailsA

   -- SN-2015-03-04: [[ IDE Restructure ]] Externals folder contains the externals,
   --  and Support contains support libraries (revpdfprinter and revsecurity):
   --  they should have their own keys in the details array.
   local tArchFolder
   switch
      case pPlatform contains "MacOSX"
         -- MM-2014-03-21: [[ PPC Support Dropped ]] We now only support intel Mac builds. Assume all mac builds to be intel.
         -- MW-2013-06-13: [[ CloneAndRun ]] If not installated type, then take from the binaries folder.
         put "Mac OS X/x86-32" into tArchFolder
         put ".bundle" into tDetailsA["loadable_extension"]
         put ".dylib" into tDetailsA["shared_extension"]
         put "MacOSX" into  tDetailsA["platform"]
         put "x86-32" into tDetailsA["architecture"]
         break
      case pPlatform = "Windows"
         put "Windows/x86-32" into tArchFolder
         put ".dll" into tDetailsA["loadable_extension"]
         put "Windows" into tDetailsA["platform"]
         put "x86-32" into tDetailsA["architecture"]
         break
      default
         if pPlatform is "Linux" then
            put "Linux/x86-32" into tArchFolder
            put "x86-32" into tDetailsA["architecture"]
         else if pPlatform is "Linux x64" then
            -- MW-2013-11-06: [[ LinuxX64 ]] Compute the correct path for 64-bit linux.
            put "Linux/x86-64" into tArchFolder
            put "x86-64" into tDetailsA["architecture"]
         else -- if pPlatform is "Linux armv6-hf" then
            -- FG-2014-08-19: [[ RPi ]] Compute the correct path for 64-bit Linux
            put "Linux/armv6-hf" into tArchFolder
            put "armv6-hf" into tDetailsA["architecture"]
         end if
         put ".so" into tDetailsA["loadable_extension"]
         put "Linux" into tDetailsA["platform"]
   end switch

   -- SN-2015-03-04: [[ CloneAndRun ]] Refactor the setting of externals / support location
   if revEnvironmentIsInstalled() then
      put revEnvironmentRuntimePath() & slash & tArchFolder into tRuntimeFolder
      put tRuntimeFolder & slash & "Support" into tDetailsA["support_folder"]
      put tRuntimeFolder & slash & "Externals" into tDetailsA["externals_folder"]
   else
      put revEnvironmentBinariesPath() into tDetailsA["externals_folder"]
      put revEnvironmentBinariesPath() into tDetailsA["support_folder"]
   end if
   return tDetailsA
end revStandalonePlatformDetails

#####################################################################
# Main use-case controller for the Standalone Application Builder
#####################################################################

private command revCreateStandalone pStack, pFolder
   local tStackFileList, tEngineSourceFile, tCount, tStackName, tStandalonePath
   local tStackPath, tStackFileName, tDirectory, tStackSourceFile, tName, tStandaloneSettingsA
   
   # OK-2007-12-18 : Bug 3612  Changed sGenericNumber from 1 to 0.
   put 0 into sGenericNumber
   lock cursor
   set cursor to watch
   lock messages
   
   revStandaloneProgress "Loading settings..."
   
   # Set empty values to defaults
   revDefaultStandaloneSettings pStack
   
   if there is a file (pFolder & "/" & sStandaloneSettingsA["name"]) then
      put " Standalone" after sStandaloneSettingsA["name"]
      put empty into tCount
      repeat while (there is a file (pFolder & "/" & sStandaloneSettingsA["name"] & tCount) or there is a folder (pFolder & "/" & sStandaloneSettingsA["name"] & tCount))
         add 1 to tCount
      end repeat
      put sStandaloneSettingsA["name"] & tCount into sStandaloneSettingsA["name"]
   end if
   
   -- Clear left over state information from previous build
   put empty into sDirectoryList
   put empty into sPlatformList
   
   # OK-2007-08-09
   put empty into sUserScriptLibraries
   
   put empty into sStubPath
   
   -- generic icons if none set
   set the itemDelimiter to slash
   
   put revEnvironmentRuntimePath() & "/Windows/x86-32/Support/Sample Icons" into tName
   if tStandaloneSettingsA["Windows,iconFile"] = "" then 
      put tName & "/genericapp.ico" into tStandaloneSettingsA["Windows,iconFile"]
      -- OK-2007-12-14 : This block appears to be redundant.
      --    if "Windows" is among the lines of sPlatformList and there is no file tStandaloneSettingsA["Windows,iconFile"] then
      --      revStandaloneAddWarning "Windows, generic icon file not found:" && quote & tName & "/genericapp.ico" & quote
      --    end if
   end if
   if tStandaloneSettingsA["Windows,documenticonFile"] = "" then 
      put tName & "/genericdoc.ico" into tStandaloneSettingsA["Windows,documenticonFile"]
      -- OK-2007-12-14 : This block appears to be redundant.
      --    if "Windows" is among the lines of sPlatformList and there is no tStandaloneSettingsA["Windows,documenticonFile"] then
      --      revStandaloneAddWarning "Windows, generic document icon not found:" && quote & tName & "/genericdoc.ico" & quote
      --    end if
   end if
   
   -- get all the stackfiles
   put revRelativeStackFilesList(pStack) into tStackFileList
   
   if sStandaloneSettingsA["inclusions"] = "search" then 
      revSmartChecking tStackFileList, (sStandaloneSettingsA["inclusions"] is "search"), false
   end if
   
   put the effective filename of stack pStack into tStackFileName
   put item 1 to -2 of the effective filename of stack pStack into tStackPath
   
   -- icon and resource file relative path handling
   if there is a file (tStackPath&"/" & sStandaloneSettingsA["MacOS,includeResources"]) then 
      put tStackPath & "/" before sStandaloneSettingsA["MacOS,includeResources"]
   end if
   
   if there is a file (tStackPath&"/" & sStandaloneSettingsA["Windows,iconFile"]) then
      put tStackPath & "/" before sStandaloneSettingsA["Windows,iconFile"]
   end if
   
   if there is not a file (sStandaloneSettingsA["Windows,iconFile"]) and sStandaloneSettingsA["Windows"] is true and sStandaloneSettingsA["Windows,iconFile"] is not empty then
      revStandaloneAddWarning "Icon does not exist:" & sStandaloneSettingsA["Windows,iconFile"]
   end if
   
   if there is not a file sStandaloneSettingsA["Windows,iconFile"] then
      put item 1 to -2 of tName & "/Sample Icons/genericapp.ico" into sStandaloneSettingsA["Windows,iconFile"]
   end if
   
   if there is a file (tStackPath & "/" & sStandaloneSettingsA["Windows,documentIconFile"]) then
      put tStackPath & "/" before sStandaloneSettingsA["Windows,documentIconFile"]
   end if
   
   if there is not a file sStandaloneSettingsA["Windows,documentIconFile"] then
      put item 1 to -2 of tName & "/Sample Icons/genericdoc.ico" into sStandaloneSettingsA["Windows,documenticonFile"]
   end if
   
   -- OK-2008-03-24 : Bug 6171. Restructured a little to ensure that the icon can be either relative or absolute.
   -- If the icon cannot be found at the exact location, assume its relative to the stack and put the stack path before it.
   if there is no file sStandaloneSettingsA["OSX,iconFile"] and sStandaloneSettingsA["OSX,iconFile"] is not empty then
      put tStackPath & "/" before sStandaloneSettingsA["OSX,iconFile"]
   end if
   
   -- If the icon cannot be found (and we are building for OS X and its not empty) then add a warning.
   if sStandaloneSettingsA["MACOSX"] is true and there is no file sStandaloneSettingsA["OSX,iconFile"] and sStandaloneSettingsA["OSX,iconFile"] is not empty then
      revStandaloneAddWarning "Icon does not exist:" & tStackPath & "/" & sStandaloneSettingsA["OSX,iconFile"]
   end if
   
   if there is a file (tStackPath & "/" & sStandaloneSettingsA["OSX,documenticonFile"]) then
      put tStackPath & "/" before sStandaloneSettingsA["OSX,documenticonFile"]
   end if
   if there is a file (tStackPath & "/" & sStandaloneSettingsA["OSX,plist"]) then
      put tStackPath & "/" before sStandaloneSettingsA["OSX,plist"]
   end if
   
   put empty into sDirectoryList
   put the directory into sResetA["Directory"]
   
   -- close the open stacks (ask to save if need be)
   revCloseOpenStacks tStackFileList, pStack
   
   put item -1 of the effective filename of stack tStackFileName into tStackName
   if not (tStackName ends with ".livecode" or tStackName ends with ".rev") then 
      put ".rev" after tStackName
   end if
   
   -- pre build message
   try
      dispatch "savingStandalone" to stack tStackFileName
   end try
   
   --build a list of output directories
   if last char of pFolder is not slash then 
      put slash after pFolder
   end if
   
   local tOutDir
   revOutputDirectories pFolder
   put the result into tOutDir
   
   --so now we have a list of output directories in sDirectoryList
   put the fileType into sResetA["FileType"]
   put 1 into tCount
   
   repeat for each line tPlatform in sPlatformList
      put line tCount of sDirectoryList into tDirectory
      -- copy files to each directory
      -- put the path to the engine into tEngineSourceFile[tPlatform]
      
      -- MW-2013-06-13: [[ CloneAndRun ]] If not installed, then fetch the engine locally.
      if revEnvironmentIsInstalled() then
         # MW-2009-06-24: For Mac OS X, engine source is two lines, first is PPC (if any), second
         #   is x86 (if any). Both implies universal build.
         -- MM-2014-03-21: [[ PPC Support Dropped ]] We now only support intel Mac builds. Assume all mac builds to be intel.
         get revEnvironmentRuntimePath() --into tEngineSourceFile[tPlatform]
         switch tPlatform
            case "MacOSX"
            case "MacOSX PowerPC-32"
            case "MacOSX x86-32"
               // SN-2015-07-09: [[ StandaloneDeployment ]] Gypified Standalone-<edition>.app building does not
               //  allow to create broken applications, so that the executable file in it bears the edition.
               local tEdition
               put the editionType into tEdition
               if tEdition is "professional" then
                  put "Commercial" into tEdition
               else
                  put the upper of char 1 of tEdition into char 1 of tEdition
               end if
               put it & "/Mac OS X/x86-32/Standalone.app/Contents/MacOS/Standalone-" & tEdition into line 1 of tEngineSourceFile[tPlatform]
               break
            case "Windows"
               put it & "/Windows/x86-32/Standalone" into tEngineSourceFile[tPlatform]
               break
            case "Linux"
               put it & "/Linux/x86-32/Standalone" into tEngineSourceFile[tPlatform]
               break
               -- MW-2013-11-06: [[ LinuxX64 ]] Compute the correct path for 64-bit linux.
            case "Linux x64"
               put it & "/Linux/x86-64/Standalone" into tEngineSourceFile[tPlatform]
               break
               -- FG-2014-08-19: [[ RPi ]] Compute the correct path for ARMv6 Linux
            case "Linux armv6-hf"
               put it & "/Linux/armv6-hf/Standalone" into tEngineSourceFile[tPlatform]
               break
            default
               put "error" into tEngineSourceFile[tPlatform]
               break
         end switch
      else
         -- Make sure the platform is correct
         // SN-2015-05-15: [[ DeployAllPlatforms ]] Allow the deployment to happen
         //  for another platform, from the repository.
         local tBinariesPath
         
         if tPlatform contains "MacOSX" and the platform is not "macos" or \
               tPlatform contains "Linux" and the platform is not "linux" or \
               tPlatform is "Windows" and the platform is not "win32" then
            put revEnvironmentNonNativeBinariesPath(tPlatform) into tBinariesPath
         else
            put revEnvironmentBinariesPath() into tBinariesPath
         end if
         
         switch word 1 of tPlatform
            case "MacOSX"
               if revLicenseType() is "community" then
                  put tBinariesPath & slash & "Standalone-Community.app/Contents/MacOS/Standalone-Community" into tEngineSourceFile[tPlatform]
               else
                  put tBinariesPath & slash & "Standalone-Commercial.app/Contents/MacOS/Standalone-Commercial" into tEngineSourceFile[tPlatform]
               end if
               break;
            case "Windows"
               if revLicenseType() is "community" then
                  put tBinariesPath & slash & "standalone-community.exe" into tEngineSourceFile[tPlatform]
               else
                  put tBinariesPath & slash & "standalone-commercial.exe" into tEngineSourceFile[tPlatform]
               end if
               break
            case "Linux"
               if revLicenseType() is "community" then
                  put tBinariesPath & slash & "standalone-community" into tEngineSourceFile[tPlatform]
               else
                  put tBinariesPath & slash & "standalone-commercial" into tEngineSourceFile[tPlatform]
               end if
               break
            default
               put empty into tEngineSourceFile[tPlatform]
               break
         end switch
      end if
      
      -- Necessary as some "users" might rename the actual executable file, arghhhh!
      if (line 1 of tEngineSourceFile[tPlatform] is not empty and there is not a file line 1 of tEngineSourceFile[tPlatform]) and \
            (line 2 of tEngineSourceFile[tPlatform] is not empty and there is not a file line 2 of tEngineSourceFile[tPlatform]) then
         throw "Standalone: Ensure that you have not renamed the main LiveCode executable file: " &cr& tEngineSourceFile[tPlatform]
      end if
      put tDirectory & sStandaloneSettingsA["name"] into tStandalonePath[tPlatform]
      if tPlatform is "Windows" then
         put ".exe" after tStandalonePath[tPlatform]
      end if
      put tDirectory & tStackName into tStackSourceFile[tPlatform]
      
      add 1 to tCount
   end repeat
   
   -- copy resources (inclusions) to files
   revCopyResources pStack, tStackFileList
   -- loop over the platforms to build
   put 1 into tCount
   repeat for each line tPlatform in sPlatformList
      # OK-2007-08-09: Copy the script libraries to the stack on a per-platform basis, overwriting the old group each time.
      revCopyScriptLibraries tStackSourceFile[tPlatform], tPlatform
      
      revCopyFiles line tCount of sDirectoryList,tStackPath
      
      -- copy database drivers
      revCopyDatabaseDrivers tPlatform, tStackSourceFile[tPlatform], tStandalonePath[tPlatform], tEngineSourceFile[tPlatform]
      -- copy externals
      revCopyExternals tPlatform, tStackSourceFile[tPlatform], tEngineSourceFile[tPlatform], tStandalonePath[tPlatform]
      
      // IM-2015-10-14: [[ BrowserWidget ]] Copy CEF files needed for the browser widget.
      // IM-2014-03-20: [[ revBrowserCEF ]] Ensure CEF support files are copied if requested
      local tRevBrowser, tLibBrowser
      put "Browser (CEF)" is among the lines of sStandaloneSettingsA["scriptLibraries"] into tRevBrowser
      put "com.livecode.extensions.livecode.browser" is among the lines of sStandaloneSettingsA["extensions"] into tLibBrowser
      revCopyCEFResources tPlatform, tStandalonePath[tPlatform], tRevBrowser, tLibBrowser
      
      revBuildStandaloneDeploy tPlatform, tStackSourceFile[tPlatform], tEngineSourceFile[tPlatform], tStandalonePath[tPlatform]
      
      add 1 to tCount
   end repeat
   
   -- post build message
   try
      dispatch "standaloneSaved" to stack tStackFileName with pFolder
   end try
   return tOutDir
end revCreateStandalone

#####################################################################
# Check for open stacks, ask to save if need be then close them
#####################################################################

private command revCloseOpenStacks pStackFileList, pStack
   local tStackFile,tSubList,tStack,tSubStack
   revStandaloneProgress "Closing open stacks..."
   
   # OK-2008-03-13 : Bug 5874. Filter out the stack being built from the list as we have 
   # already done the save checking on it.
   local tCheckList
   put pStackFileList["full"] into tCheckList
   
   local tLineNumber
   set the wholeMatches to true
   put lineOffset(the effective filename of stack pStack, tCheckList) into tLineNumber
   if tLineNumber <> 0 then
      delete line tLineNumber of tCheckList
   end if
   
   repeat for each line tStackFile in tCheckList
      if the short name of stack tStackFile is among the lines of the mainStacks then
         put the subStacks of stack tStackFile into tSubList
         set the itemDel to "/"
         repeat for each line tStack in tSubList
            if the mode of stack tStack is not 0 then
               if not revSaveCheck(tStack) then
                  revAbort
                  # OK-2008-03-13 : Bug 5874
                  exit to top
               end if
            end if
         end repeat
         if the mode of stack tStackFile is not 0 then
            if not revSaveCheck(the short name of stack tStackFile) then
               revAbort
               # OK-2008-03-13 : Bug 5874
               exit to top
            end if
         end if
         set the itemDel to comma
      end if
   end repeat
   lock messages
   -- remove the stacks from memory and keep a note of them for later
   repeat for each line tStack in pStackFileList["full"]
      if the short name of stack tStack is among the lines of the mainStacks then
         put the subStacks of stack tStack into tSubList
         repeat for each line tSubStack in tSubList
            if the short name of stack tSubStack is among the lines of openstacks() then
               put the long name of stack tSubStack & cr after sResetA["Reopen"]
               
               # OK-2008-11-10 : Bug 7382 - Remember if stacks were in message path
               put stackIsInGlobalMessagePath(the long name of stack tSubStack) & return after sResetA["Restore Message Path"]
               
            end if
            #LG-2008-01-24
            #Bug 5110:  Changed code so substacks are closed instead of
            #deleted.
            --delete stack tSubStack
            close stack tSubStack
         end repeat
         if the short name of stack tStack is among the lines of openstacks() then
            put the long name of stack tStack & cr after sResetA["Reopen"]
            
            # OK-2008-11-10 : Bug 7382 - Remember if stacks were in message path
            put stackIsInGlobalMessagePath(the long name of stack tStack) & return after sResetA["Restore Message Path"]
         end if
         -- MM-2013-03-20: [[ Bug 10756 ]] Make sure we're allowed to delete the stack before deleteing.
         set the cantDelete of stack tStack to false
         delete stack tStack
      end if
   end repeat
end revCloseOpenStacks

# OK-2008-11-10 : Bug 7382 - Remember if stacks were in message path
private function stackIsInGlobalMessagePath pStack
   set the wholeMatches to true
   if the short name of pStack is among the lines of the stacksInUse then
      return "in use"
   end if
   
   if pStack is among the lines of the backscripts then
      return "backscript"
   end if
   
   if pStack is among the lines of the frontscripts then
      return "frontscript"
   end if
   
   return empty
end stackIsInGlobalMessagePath

private command restoreStackToMessagePath pStack, pMessagePathState
   switch pMessagePathState
      case empty
         exit restoreStackToMessagePath
         break
      case "in use"
         # Messages should be locked in to prevent a libraryStack message being sent to the stack in use.
         # The idea being that the standalone builder should intefere as little as possible with the built stacks.
         lock messages
         start using stack pStack
         unlock messages
         break
      case "backscript"
         # Note that the order of backscripts may not be preserved, not sure if this is a problem...
         lock messages
         insert the script of pStack into back
         unlock messages
         break
      case "frontscript"
         lock messages
         insert the script of pStack into front
         unlock messages
         break
      default
         # Just ignore this for now
   end switch
end restoreStackToMessagePath

#####################################################################
# Generate sDirectoryList and sPlatformList
#####################################################################


private command revOutputDirectories pFolder
   local tPlatform,tDirectory,tPlatforms,tCount
   revStandaloneProgress "Checking directories..."
   if there is a folder (pFolder & sStandaloneSettingsA["name"]) then
      put 1 into tCount
      repeat while there is a folder (pFolder & sStandaloneSettingsA["name"]&tCount)
         add 1 to tCount
      end repeat
   end if
   put 0 into tPlatforms
   repeat for each item tPlatform in kPlatforms
      if sStandaloneSettingsA[tPlatform] = true then
         add 1 to tPlatforms
      end if
   end repeat
   repeat for each item tPlatform in kPlatforms
      if sStandaloneSettingsA[tPlatform] = true then
         if not revDownloadEngine(tPlatform) then
            revStandaloneAddWarning "Could not build for " & tPlatform & " because required files are missing."
            next repeat
         end if
         
         # OK-2010-02-17: Bug 8608 - Deploying stack with trailing whitespace in name fails on Windows
         # because the folder created doesn't match the name passed to create folder command.
         local tFolderName
         put sStandaloneSettingsA["name"] & tCount into tFolderName
         if the platform is "Win32" or tPlatform is "Windows" then
            put replaceText(tFolderName, "[ \t]+$", empty) into tFolderName
         end if
         
         if tPlatforms > 1 then
            put pFolder & tFolderName & "/" & tPlatform & "/" into tDirectory
         else
            put pFolder & tFolderName & "/" into tDirectory
         end if
         if tPlatform contains "MacOSX" then 
            put sStandaloneSettingsA["name"] & ".app/Contents/MacOS/" after tDirectory
         end if
         put tDirectory & return after sDirectoryList
         put tPlatform & return after sPlatformList
      end if
   end repeat
   delete last char of sDirectoryList 
   delete last char of sPlatformList
   # Marcus: I need the root directory for U3, so return it here, nothing else uses a return
   return (pFolder & sStandaloneSettingsA["name"] & tCount)
end revOutputDirectories

# OK-2007-08-09 Copies the appropriate script libraries (i.e. group revLibraries of the appropriate revLibrary.rev)
# to the stack being built. Will remove any existing group called revLibraries from card 1 of the stack.
private command revCopyScriptLibraries pStack, pPlatform
   # AL-2015-02-03: [[ Scriptify IDE ]] Ensure revCommonLibrary is always included in a standalone
   revAddLibrary "Common"
   
   -- data grid
   // SN-2015-10-19: [[ Bug 16238 ]] DataGrid can be included on the wish of the user - in case the standalone stack
   //  will load stacks using DataGrid, for instance.
   if ("data grid templates" is in the subStacks of stack pStack or "DataGrid" is among the lines of sStandaloneSettingsA["scriptLibraries"]) \
         and there is a stack "revDataGridLibrary" then
      set the mainStack of stack "revDataGridLibrary" to pStack
   end if
   
   // AL-2015-04-28: [[ Bug 15244 ]] Include profiles library if needed
   if sStandaloneSettingsA["includeProfiles"] is not empty then 
      revAddLibrary "Profiles"
   end if
   
   /*
   set the cScriptLibraries of group "revLibraries" of card 1 of stack pStack to sUserScriptLibraries
   set the cAppIcon of group "revLibraries" of card 1 of stack pStack to sStandaloneSettingsA["OSX,appicon"]
   set the cSmallAppIcon of group "revLibraries" of card 1 of stack pStack to sStandaloneSettingsA["OSX,smallappicon"]
   */
end revCopyScriptLibraries

#####################################################################
# Copies general resources to the standalone stack and then does
# what it needs to to substacks etc.
#
# Note: The major difference between this code and the old DB code is
# here we do every thing to the stacks once then save them to each
# platform directory. In the old DB we repeated everything for each directory
# which meant that the cost of building for multiple platforms was far higher.
#####################################################################

private command revCopyResources pStack, pStackFileList
   local tCopyFiles, tLicenseType, tFileName, tLine, tSave, tStackFileSettingsA, tMainStackSubs
   local tLibraries, tOrigVisible, tPath, tCount, tRelativePath, tStack, tSubList, tStacksList, tVisibleA
   -- engine defaults are correct, except os 9 which is handled platform specific
   revStandaloneProgress "Copying resources..."
   lock messages
   put the visible of stack pStack into tVisibleA[pStack]
   
   set the visible of stack pStack to false
   toplevel pStack
   
   if the shadowColor of stack pStack = "" and the shadowPattern of stack pStack = "" then set the shadowColor of stack pStack to 172,172,172
   if the bottomColor of stack pStack = "" and the bottomPattern of stack pStack = "" then set the bottomColor of stack pStack to 128,128,128
   -- now add stuff to the mainstack
   -- need the substacks of the mainStack to avoid exporting all the stacks we're importing now
   -- it's better to do it this way than have heaps of extra saves or a separate mainstack script
   put the substacks of stack pStack into tMainStackSubs
   
   if sStandaloneSettingsA["answerDialog"] then
      put "answer dialog"&cr after tCopyFiles
      revAddDialogIcons
   end if
   if sStandaloneSettingsA["askDialog"] then
      revAddDialogIcons
      put "ask dialog" &cr after tCopyFiles
   end if
   
   if sStandaloneSettingsA["brushes"] then
      put "revCompatibilityBrushes1" & return after tCopyFiles
   end if
   
   -- PM-2015-06-24: [[ Bug 15535 ]] Make possible to use magnify palette in standalones
   if sStandaloneSettingsA["Magnify"] then
      put "Magnify" & return after tCopyFiles
   end if
   
   if sStandaloneSettingsA["revolutionPrintDialogs"] then
      put "Print Dialog" & return after tCopyFiles
      put "Page Setup Dialog" & return after tCopyFiles
   end if
   
   delete char -1 of tCopyFiles
   -- copy the resources into the standalone mainstack
   repeat for each line tLine in tCopyFiles
      if tLine is empty then next repeat
      if tLine is among the lines of the substacks of stack pStack then next repeat
      put the visible of stack tLine into tOrigVisible
      hide stack tLine
      clone stack tLine
      set the mainStack of stack ("copy of" && tLine) to pStack
      close stack tLine
      set the visible of stack tLine to tOrigVisible
      close stack ("copy of" && tLine)
      show stack ("copy of" && tLine)
      set the name of stack ("copy of" && tLine) to tLine
   end repeat
   put "" into tCopyfiles
   -- error reporting
   if sStandaloneSettingsA["errorDialog"] then
      set the cErrorsList of stack "revErrorReport" to the cErrorsList of cd 1 of stack "revErrorDisplay"
      set the cScriptErrors of stack "revErrorReport" to the cScriptErrors of cd 1 of stack "revErrorDisplay"
      set the htmltext of fld "info" of stack "revErrorReport" to sStandaloneSettingsA["errorDialog,htmltext"]
      if sStandaloneSettingsA["errorDialog,icon"] is not "" then
         set the icon of btn "icon" of stack "revErrorReport" to sStandaloneSettingsA["errorDialog,icon"]
         put true into sIconA[sStandaloneSettingsA["errorDialog,icon"]]
         show btn "icon" of stack "revErrorReport"
      else
         hide btn "icon" of stack "revErrorReport"
      end if
      if sStandaloneSettingsA["errorDialog,comments"] = true then
         show group "comments" of stack "revErrorReport"
      else
         hide group "comments" of stack "revErrorReport"
      end if
      if sStandaloneSettingsA["errorDialog,email"] is not "" then
         show btn "email" of stack "revErrorReport"
         set the cEmail of stack "revErrorReport" to sStandaloneSettingsA["errorDialog,email"]
      else
         hide btn "email" of stack "revErrorReport"
      end if
      if sStandaloneSettingsA["errorDialog,save"] = true then
         show btn "save" of stack "revErrorReport"
      else
         hide btn "save" of stack "revErrorReport"
      end if
      -- can leave it as copy of and it's neccesary since it's a substack revStandaloneSettings
      put "revErrorReport" & cr after tCopyFiles
   end if
   
   set the itemDelimiter to "/"
   
   repeat for each line tLine in tCopyFiles
      if tLine is among the lines of the substacks of stack pStack then next repeat
      set the visible of stack tLine to false
      clone stack tLine
      set the mainStack of stack ("copy of "&tLine) to pStack
      set the visible of stack tLine to true
      close stack ("copy of "&tLine)
      set the visible of stack ("copy of "&tLine) to true
      put "copy of "&tLine & cr after tLibraries
   end repeat
   delete char -1 of tLibraries
   
   # OK-2007-08-09 : Because the group "revLibraries" is created on a per-platform basis to support Mac OS Classic, 
   # we have to defer the setting of this property until after the group is copied, we store this value in a script
   # local variable instead, and set the property when the group is actually copied, in revCopyScriptLibraries.
   # set the cScriptLibraries of group "revLibraries" of card 1 of stack pStack to tLibraries
   put tLibraries into sUserScriptLibraries
   
   # OK-2007-08-09 : Moved to revCopyScriptLibraries
   --  set the cAppIcon of group "revLibraries" of cd 1 of stack pStack to sStandaloneSettingsA["OSX,appicon"]
   --  set the cSmallAppIcon of group "revLibraries" of cd 1 of stack pStack to sStandaloneSettingsA["OSX,smallappicon"]
   put true into sIconA[sStandaloneSettingsA["OSX,appicon"]]
   put true into sIconA[sStandaloneSettingsA["OSX,smallappicon"]]
   put "" into tCopyFiles
   put 1 into tCount
   repeat for each line tStackFile in pStackFileList["full"]
      --put "" into tCopyFiles
      put item 1 to -2 of line tCount of pStackFileList["relative"] into tRelativePath
      if tRelativePath is not empty then put "/" after tRelativePath
      put item -1 of tStackFile into tFileName
      if tRelativePath & tFileName is tStackFile then put empty into tRelativePath
      put the mainStack of stack tStackFile into tStack
      if tStack is not pStack then
         put the subStacks of stack tStackFile into tSubList
         put the visible of stack tStack into tVisibleA[tStack]
         
         set the visible of stack tStack to false
         toplevel tStack
      else
         put tMainStackSubs into tSubList
      end if
      if tSubList is not "" then
         put tStack & return & tSubList into tStacksList
      else
         put tStack into tStacksList
      end if
      repeat for each line tLine in tStacksList
         --export images, remove dev custom props
         if the cRevStandaloneSettings["destroyStack"] of stack tLine then
            set the destroyStack of stack tLine to true
         end if
         if the cRevStandaloneSettings["password"] of stack tLine is not "" then
            try
               set the password of stack tLine to the cRevStandaloneSettings["password"] of stack tLine
            catch someError
               beep
               answer warning "There was an error applying a password to stack" && tLine & "." with "OK"
            end try
         end if
         
         # OK-2007-10-16 : Bug 5460
         revEnumerateIcons tLine
         
         if sStandaloneSettingsA["copyreferencedfiles"] is true then
            revCopyReferencedFiles tLine, tRelativePath, sStandaloneSettingsA["ReferencedFilesDestination"]
         end if
      end repeat
      -- we need to do the icon stuff before we save the mainstack
      if tStack is not pStack then 
         close stack tStack
         set the visible of stack tStack to tVisibleA[tStack]
         put revSaveStackFile(tStack,tSubList,tRelativePath) & return after tCopyFiles
      end if
      add 1 to tCount
   end repeat
   revCopyIcons pStack
   
   
   -- set stackFiles of mainStack
   delete char -1 of tCopyFiles
   set the stackFiles of stack pStack to tCopyFiles
   close stack pStack
   set the visible of stack pStack to tVisibleA[pStack]
   get revSaveStackFile(pStack,tMainStackSubs)
end revCopyResources

private command revAddDialogIcons
   repeat for each line tPlatform in sPlatformList
      switch word 1 of tPlatform
         case "MacOSX"
            put true into sIconA[210092]
            put true into sIconA[210093]
            put true into sIconA[210094]
            put true into sIconA[210097]
            break
         case "Windows"
            put true into sIconA[210003]
            put true into sIconA[210004]
            put true into sIconA[210005]
            put true into sIconA[210011]
            break
         default
            put true into sIconA[210006]
            put true into sIconA[210007]
            put true into sIconA[210008]
            put true into sIconA[210010]
      end switch
   end repeat
end revAddDialogIcons

private function revSaveStackFile pStack, pSubList, pRelativePath
   local tStacksList, tLine, tFileName, tCopyFiles, tStackFileSettingsA, tSave
   revStandaloneProgress "Saving stack files..."
   put the customProperties["cRevStandaloneSettings"] of stack pStack into tStackFileSettingsA
   
   add 1 to sGenericNumber
   
   repeat for each line tLine in pSubList
      revSetStackProps tLine
      -- move substacks and save into data folders
      if tStackFileSettingsA["moveSubstacks"] = true then
         set the mainStack of stack tLine to tLine
         --cycle through sDirectoryList and build the files in each directory
         repeat for each line tDirectory in sDirectoryList
            put pRelativePath into tSave
            if tStackFileSettingsA["substackFolder"] is not "" then
               put tStackFileSettingsA["substackFolder"] & "/" after tSave
            end if
            
            put revSaveToFolder(tDirectory & tSave, tLine, tStackFileSettingsA["renameGeneric"]) into tFileName
            
            if tLine,tSave & "/" & tFileName is not among the lines of tCopyFiles then put tLine,tSave & "/" & tFileName & return after tCopyFiles
         end repeat
         -- MM-2013-04-05: [[ Bug 10756 ]] Make sure we're allowed to delete the stack before deleteing.
         set the cantDelete of stack tLine to false
         delete stack tLine
      end if
      add 1 to sGenericNumber
   end repeat
   revSetStackProps pStack
   if paramCount() = 2 then
      delete char -1 of tCopyFiles
      if the stackFiles of stack pStack is not "" then
         set the stackFiles of stack pStack to the stackFiles of stack pStack&return&tCopyFiles
      else
         set the stackFiles of stack pStack to tCopyFiles
      end if
   end if
   
   -- OK-2008-03-24 : Bug 6208. The mainstack must be saved after the substacks have 
   -- been saved / deleted, otherwise they remain in the built standalone, causing the wrong
   -- stacks to be used and resulting in the stacks not being saveable.
   repeat for each line tDirectory in sDirectoryList
      put revSaveToFolder(tDirectory & pRelativePath, pStack, false) into tFileName
   end repeat
   
   put pStack,pRelativePath&tFileName & return after tCopyFiles
   if tStackFileSettingsA["moveSubstacks"] is not true then
      repeat for each line tLine in pSubList
         put tLine,pRelativePath&tFileName & return after tCopyFiles
      end repeat
   end if
   -- MM-2013-04-05: [[ Bug 10756 ]] Make sure we're allowed to delete the stack before deleteing.
   set the cantDelete of stack pStack to false
   delete stack pStack
   delete char -1 of tCopyFiles
   return tCopyFiles
end revSaveStackFile

#####################################################################
# Search the image libraries for images to copy
# Required images are sIconA[<iconNumber>] = true
#
#####################################################################

private command revCopyIcons pStack
   local tIconFolder,tSaveDirectory,tStacksList,tCardNum,tImageNum, tEnvironmentIcons, tUserIcons, tFiles
   revStandaloneProgress "Copying icons..."
   -- create the stack to copy to
   lock screen
   if keys(sIconA) is not "" then
      create invisible stack "revCopiedIcons"
      set the destroyStack of stack "revCopiedIcons" to true
      set the mainStack of stack "revCopiedIcons" to pStack
      -- get the icon libraries
      
      put revAbsoluteFolderListing(revEnvironmentResourcesPath("Icon Libraries")) into tEnvironmentIcons
      put revAbsoluteFolderListing(revEnvironmentUserResourcesPath("Icon Libraries")) into tUserIcons
      put revCombineFilePaths(tUserIcons,tEnvironmentIcons) into tFiles
      
      put "revGeneralIcons" & return & "revCompatibilityIcons1" & return & "revGeneralPatterns" & return & "revCompatibilityPatterns1" & return & "revCompatibilityPatterns2" & return & "revCustomCursors" into tStacksList
      set the itemDel to slash
      repeat for each line tFile in tFiles
         if there is not a stack tFile then next repeat -- windows placeholder
         put return & the short name of stack tFile after tStacksList
      end repeat
      -- loop over each stack and copy any image that needs to be copied to "revCopiedIcons"
      repeat for each line tStack in tStacksList
         put the number of cds of stack tStack into tCardNum
         repeat with tCard =1 to tCardNum
            put the number of images of cd tCard of stack tStack into tImageNum
            repeat with tImage =1 to tImageNum
               if sIconA[the short id of image tImage of cd tCard of stack tStack] then
                  copy image tImage of cd tCard of stack tStack to stack "revCopiedIcons"
                  set the id of the last image of stack "revCopiedIcons" to the short id of image tImage of cd tCard of stack tStack
                  put false into sIconA[the short id of image tImage of cd tCard of stack tStack]
               end if
            end repeat
         end repeat
      end repeat
      close stack "revCopiedIcons"
   end if
end revCopyIcons

#####################################################################
# Copies the user selected files to the application directory
#####################################################################


private command revCopyFiles pDirectory, pStackPath
   local tName,tDir
   revStandaloneProgress "Copying files..."
   set the itemDel to "/"
   repeat for each line tFile in sStandaloneSettingsA["files"]
      if the last item of tFile is not "*" then
         if there is not a file tFile then
            put tFile into tName
            put pStackPath & "/" before tFile
         else
            put item -1 of tFile into tName
         end if
      else
         delete the last item of tFile
         --if there is not a folder tFile then
         if there is a folder (pStackpath & slash & tFile) then
            put tFile into tName
            put pStackpath & slash before tFile
         else
            put item -1 of tFile into tName
         end if
      end if
      
      put empty into tDir
      repeat for each item tFolder in item 1 to -2 of tName
         put tFolder & "/" after tDir
         if there is not a folder (pDirectory & tDir) then
            create folder pDirectory & tDir
         end if
      end repeat
      if revResourceExists(tFile) then
         ## EJB 2014-07-30: [[ Bug 12864 ]] Ensure a relative folder in Copy Files is converted to an absolute path
         --revCopyFile tFile, pDirectory & item 1 to -2 of tName
         if there is a file tFile then
            revSBCopyFileToFolder tFile, pDirectory & item 1 to -2 of tName, "revStandaloneProgressCallback", the long id of me
         else -- it's a folder
            revSBCopyFolderToDestination tFile, pDirectory & item 1 to -2 of tName, "revStandaloneProgressCallback", the long id of me
         end if
      else
         revStandaloneAddWarning "File to include in standalone not found:" && quote & tFile & quote
      end if
   end repeat
end revCopyFiles

private command revDeleteFile pWhich
   close file pWhich
   if the platform is "MacOS" then put empty into URL ("resfile:"&pWhich)
   put empty into URL ("binfile:"&pWhich)
   delete file pWHich
end revDeleteFile

#####################################################################
#
# revAbort is called if we need to stop the build process for whatever
# reason.
#
#####################################################################

# OK-2010-03-31: Ensure that user data cannot be lost on cleanup by redirecting all calls to revDeleteFolder
# via this command. The idea is that we check some basic stuff to ensure that if something goes wrong, we 
# don't delete anything important.
private command revStandaloneDeleteFolder pFolder
   if revStandaloneFolderSafeToDelete(pFolder) then
      revDeleteFolder pFolder
      return the result
   else
      # Best not to do anything here, its probably a cleanup issue so may not effect the built application,
      # additionally its maybe best if the user didn't know that rev nearly tried to delete the wrong folder...
   end if
end revStandaloneDeleteFolder

private function revStandaloneFolderSafeToDelete pFolder
   set the itemDelimiter to slash
   
   # If we don't know the name of what we're building, don't risk deleting a folder, this should ever happen unless
   # its a bug somewhere else in the builder.
   if sStandaloneSettingsA["name"] is empty then
      return false
   end if
   
   # Folders being deleted should always contain the name of the application being built, if not, something is wrong.
   if not (pFolder contains sStandaloneSettingsA["name"]) then
      return false
   end if
   
   # Finally, don't ever allow a folder with less than 2 items in its path to be deleted. This could be an issue if the user
   # tries to build the stack directly to the root of the drive, but its a nice safety check to have...
   if the number of items of pFolder < 2 then
      return false
   end if
   
   # Maybe we should add more checks here, but the above ones seem like they should cover everything
   
   return true
end revStandaloneFolderSafeToDelete

private command revCleanOutputFolders
   set the itemDel to "/"
   if sDirectoryList is not "" then
      if the number of lines of sDirectoryList > 1 then
         revStandaloneDeleteFolder item 1 to -2 of line 1 of sDirectoryList
      else
         revStandaloneDeleteFolder line 1 of sDirectoryList
      end if
   end if
end revCleanOutputFolders

private command revCleanBuildStacks
   repeat for each line tLine in sResetA["Reopen"]
      if there is a stack tLine then
         if the mode of stack tLine is not 0 then
            repeat for each line tSub in the substacks of stack tLine
               close stack tSub
            end repeat
            -- MM-2013-04-05: [[ Bug 10756 ]] Make sure we're allowed to delete the stack before deleteing.
            set the cantDelete of stack tLine to false
            delete stack tLine
         end if
      end if
   end repeat
   revReset
   close stack "revStandaloneProgress"
end revCleanBuildStacks

private command revAbort
   revCleanOutputFolders
   revCleanBuildStacks
end revAbort

command revReset
   unlock cursor
   lock screen
   lock messages
   delete variable sDirectoryList        -- the list of build directories
   delete variable sPlatformList         -- the list of build platforms
   delete variable sGenericNumber        -- a simple counter for the generic file naming
   delete variable sStandaloneSettingsA  -- a copy of the cRevStandaloneSettings of stack pFolder
   delete variable sIconA                -- array for icons
   set the directory to sResetA["Directory"]
   set the fileType to sResetA["fileType"]
   unlock messages
   try
      # MW-2009-07-18: Media does not have a datagrid library and so this breaks if we don't
      #   double check it exists there first.
      if there is a stack "revDataGridLibrary" then
         set the mainStack of stack "revDataGridLibrary" to "revDataGridLibrary"
      end if
      
      # TH-2008-07-08 :: Bug 6444, locking messages around this handler to prevent calling of preopencard handlers
      # in the stacks being built
      lock messages
      local tLineNumber
      put 0 into tLineNumber
      repeat for each line tLine in sResetA["Reopen"]
         add 1 to tLineNumber
         go tLine
         
         # OK-2008-11-10 : Bug 7382 - Restores the stack's position in the message path if there was one
         restoreStackToMessagePath tLine, line tLineNumber of sResetA["Restore Message Path"]
      end repeat
      unlock messages
   end try
   delete variable sResetA               -- stored info for resetting to the pre-build state
   close stack "revStandaloneProgress"
end revReset

private command revSetMacOS pWhatFile, pPlatformCode, pEnginePath
   local tVers,tSize,tTempPlist,tDocTypeList,tDocExtList
   revStandaloneProgress "Building Mac OS components..."
   
   -- for OSX
   if pPlatformCode contains "MacOSX" then
      -- MW-2013-11-06: [[ Bug 11385 ]] Updated to escape all strings put in the plist (Mavericks
      --    doesn't like & - its stricter!)
      -- user chose a plist file
      if sStandaloneSettingsA["OSX,plist"] is not "" and there is a file sStandaloneSettingsA["OSX,plist"] then
         put uniDecode(uniEncode(URL ("file:" & sStandaloneSettingsA["OSX,plist"]), "UTF8")) into tTempPlist
      else
         set the itemDel to "/"
         --setting the different plist variables
         put URL ("file:" & item 1 to -3 of pEnginePath&"/Info.plist") into tTempPlist
         if sStandaloneSettingsA["OSX,info"] is not empty then
            put "<string>" & escapeForXml(sStandaloneSettingsA["OSX,info"]) & "</string>" into line lineoffset("<key>CFBundleGetInfoString</key>",tTempPlist) + 1 of tTempPlist
         end if
         if sStandaloneSettingsA["OSX,longVersion"] is not empty then
            put "<string>" & escapeForXml(sStandaloneSettingsA["OSX,longVersion"]) & "</string>" into line lineoffset("<key>CFBundleLongVersionString</key>",tTempPlist) + 1 of tTempPlist
         end if
         if sStandaloneSettingsA["OSX,shortVersion"] is not empty then
            -- MW-2011-01-28: [[ Bug 9316 ]] Make the bundle version the same as the short version string
            put "<string>" & escapeForXml(sStandaloneSettingsA["OSX,shortVersion"]) & "</string>" into line lineoffset("<key>CFBundleVersion</key>",tTempPlist) + 1 of tTempPlist
            put "<string>" & escapeForXml(sStandaloneSettingsA["OSX,shortVersion"]) & "</string>" into line lineoffset("<key>CFBundleShortVersionString</key>",tTempPlist) + 1 of tTempPlist
         end if
         if sStandaloneSettingsA["OSX,copyright"] is not empty then
            put "<string>" & escapeForXml(sStandaloneSettingsA["OSX,copyright"]) & "</string>" into line lineoffset("<key>NSHumanReadableCopyright</key>",tTempPlist) + 1 of tTempPlist
         end if
         if sStandaloneSettingsA["OSX,signature"] is not empty then
            put "<string>" & escapeForXml(sStandaloneSettingsA["OSX,signature"]) & "</string>" into line lineoffset("<key>CFBundleSignature</key>",tTempPlist) + 1 of tTempPlist
         end if
         if sStandaloneSettingsA["OSX,identifier"] is not empty then
            put "<string>" & escapeForXml(sStandaloneSettingsA["OSX,identifier"]) & "</string>" into line lineoffset("<key>CFBundleIdentifier</key>",tTempPlist) + 1 of tTempPlist
         end if
         if sStandaloneSettingsA["OSX,name"] is not empty then
            put "<string>" & escapeForXml(sStandaloneSettingsA["OSX,name"]) & "</string>" into line lineoffset("<key>CFBundleName</key>",tTempPlist) + 1 of tTempPlist
         end if
         put "<string>" & escapeForXml(sStandaloneSettingsA["name"]) & "</string>" into line lineoffset("<key>CFBundleExecutable</key>",tTempPlist) + 1 of tTempPlist
         if sStandaloneSettingsA["name"] is not empty then
            put "<string>" & escapeForXml(sStandaloneSettingsA["name"]) &&"Files"& "</string>" into line lineoffset("<key>CFBundleTypeName</key>",tTempPlist) + 1 of tTempPlist
         end if
         set the itemDelimiter to comma
         put empty into tDocTypeList
         repeat for each item tType in sStandaloneSettingsA["OSX,documentType"]
            put "<string>" & escapeForXml(tType) & "</string>" & cr after tDocTypeList
         end repeat
         if the last char of tDocTypeList is cr then
            delete the last char of tDocTypeList
         end if
         put escapeForXml(tDocTypeList) into line lineoffset("<key>CFBundleTypeOSTypes</key>",tTempPlist) + 2 of tTempPlist
         put empty into tDocExtList
         repeat for each item tType in sStandaloneSettingsA["OSX,documentExtension"]
            put "<string>" & escapeForXml(tType) & "</string>" & cr after tDocExtList
         end repeat
         if the last char of tDocExtList is cr then
            delete the last char of tDocExtList
         end if
         put escapeForXml(tDocExtList) into line lineoffset("<key>CFBundleTypeExtensions</key>",tTempPlist) + 2 of tTempPlist
      end if
      set the itemDel to "/"
      wait 0 ticks with messages
      put uniDecode(uniEncode(tTempPlist), "UTF8") into URL ("file:" & item 1 to -3 of pWhatFile&"/Info.plist")
      wait 0 ticks with messages
      if sStandaloneSettingsA["OSX,signature"] is not empty then
         put "APPL" & sStandaloneSettingsA["OSX,signature"] into URL ("file:" & item 1 to -3 of pWhatFile&"/PkgInfo")
      end if
      if there is not a folder (item 1 to -3 of pWhatFile & "/Resources") then
         create folder (item 1 to -3 of pWhatFile & "/Resources")
      end if
      
      -- MM-2014-03-21: [[ PPC Support Dropped ]] We no longer include universal builds so set icon prefix to intel standalone.
      local tIconPrefix
      put  revEnvironmentRuntimePath() & "/Mac OS X/x86-32/Standalone.app/Contents/Resources/Standalone" into tIconPrefix
      
      if sStandaloneSettingsA["OSX,iconFile"] is not empty then
         revSBCopyFile sStandaloneSettingsA["OSX,iconFile"], item 1 to -3 of pWhatFile & "/Resources/Standalone.icns", "revStandaloneProgressCallback", the long id of me
      else
         revSBCopyFile tIconPrefix & ".icns", item 1 to -3 of pWhatFile & "/Resources/Standalone.icns", "revStandaloneProgressCallback", the long id of me
      end if
      if sStandaloneSettingsA["OSX,documenticonFile"] is not empty then
         revSBCopyFile sStandaloneSettingsA["OSX,documenticonFile"], item 1 to -3 of pWhatFile & "/Resources/StandaloneDoc.icns", "revStandaloneProgressCallback", the long id of me
      else
         revSBCopyFile tIconPrefix & "Doc.icns", item 1 to -3 of pWhatFile & "/Resources/StandaloneDoc.icns", "revStandaloneProgressCallback", the long id of me
      end if
      
      constant kLanguagesToSupport = "da Dutch English fi French German Italian Japanese ko no Spanish sv zh_CN zh_TW"
      repeat for each word tWord in kLanguagesToSupport
         revSBCopyFolder item 1 to -3 of pEnginePath & "/Resources/" & tWord & ".lproj", item 1 to -3 of pWhatFile & "/Resources/" & tWord & ".lproj", "revStandaloneProgressCallback", the long id of me
      end repeat
      
      # MW-2011-05-23: [[ Bug 9543 ]] Don't include this - its a security hole.
      --if there is a file (item 1 to -3 of pEnginePath & "/Resources/LiveCode.rsrc") then
      --revSBCopyFile item 1 to -3 of pEnginePath & "/Resources/LiveCode.rsrc", item 1 to -3 of pWhatFile & "/Resources/" & sStandaloneSettingsA["name"] & ".rsrc", "RevStandaloneProgressCallback", the long id of me
      --end if
   end if
end revSetMacOS

-- MW-2013-11-06: [[ Bug 11385 ]] Utility function to make sure we don't have & in our
--    plists as Mavericks doesn't like it!
private function escapeForXml pString
   replace "&" with "&amp;" in pString
   return pString
end escapeForXml

#####################################################################
# revSetStackProps is a combination of revRemoveProps and revSetProfiles
# There's no point walking every object in the distribution twice
# if we can avoid it right???
#
# Uses revSetObjectPorops to reduce maintenance
#
# cRevStandaloneSettings are also deleted here.
######################################################################

private command revSetStackProps pStack
   # OK-2009-10-08 : Bug 2217 : This bug requires the addition of a more efficient "repeat for each control" loop to fix properly.
   # For now however, I've added this hack to allow users to work around the issue themselves.
   if the cREVKeepDevelopmentProperties of stack pStack then
      exit revSetStackProps
   end if
   local tCard,tControl,tSetTo,tRemoveAll,tIncludeList,tCardNum,tControlNum
   lock screen
   lock messages
   revStandaloneProgress "Removing development properties and setting profile options..."
   set the wholeMatches to true
   --remove profiles from all objects
   if sStandaloneSettingsA["setToProfile"] is not "" then put true into tSetTo
   -- stack properties
   set the customKeys["cREVStandaloneSettings"] of stack pStack to ""
   revSetObjectProps the long id of stack pStack,tSetTo,sStandaloneSettingsA["includeProfiles"]
   -- card properties
   put the number of cds in stack pStack into tCardNum
   repeat with tCard = 1 to tCardNum
      revSetObjectProps the long id of cd tCard of stack pStack,tSetTo,sStandaloneSettingsA["includeProfiles"]
      -- control properties
      put the number of controls of cd tCard of stack pStack into tControlNum
      repeat with tControl = 1 to tControlNum
         revSetObjectProps the long id of control tControl of cd tCard of stack pStack,tSetTo,sStandaloneSettingsA["includeProfiles"],tControl
      end repeat
   end repeat
end revSetStackProps

#####################################################################
#
# Removes the development properties and sets the profile options of an object
#
#####################################################################


private command revSetObjectProps pObject,pSetTo,pIncludeList
   local tKeys,tProfileSets,tLine,tFinalList
   revStandaloneProgress "Removing development properties and setting profile options..."
   put the customProperties["cREVGeneral"] of pObject into tKeys
   if pSetTo then put sStandaloneSettingsA["setToProfile"] into tKeys["profile"]
   if pIncludeList = "" then
      delete local tKeys["profile"]
      delete local tKeys["masterName"]
      delete local tKeys["profileList"]
   end if
   if pIncludeList is not "*" then
      --only include specific profiles in pObject
      put the customPropertySets of pObject into tProfileSets
      put tProfileSets into tFinalList
      repeat for each line tLine in tProfileSets
         if char 1 to 12 of tLine is "cREVDiverged" then
            delete char 1 to 12 of tLine
            if tLine is not among the lines of sStandaloneSettingsA["includeProfiles"] then
               delete line lineOffset(("cREVDiverged"&tLine),tFinalList) of tFinalList
            end if
         end if
      end repeat
      set the customPropertySets of pObject to tFinalList
   end if
   delete local tKeys["bookmarks"]
   delete local tKeys["handlerList"]
   delete local tKeys["scriptSelection"]
   delete local tKeys["prevHandler"]
   delete local tKeys["tempScript"]
   delete local tKeys["script"]
   delete local tKeys["scriptCheckSum"]
   if pSetTo then unlock messages --activate profile libraries
   set the customProperties["cREVGeneral"] of pObject to tKeys
   if pSetTo then lock messages --activate profile libraries
end revSetObjectProps

#####################################################################
# Loops over all the images and players, copies the referenced
# files to each build directory then sets the filename propety
# of the original stack.
######################################################################

# OK-2007-10-16 : Bug 5460. It seems that the code that populates the sIconA array was moved to revCopyReferencedFiles at some point
# but unfortunately because revCopyReferencedFiles is only called under certain circumstances, the correct icons / patterns are no 
# longer copied. I think this was introduced in 2.9.0-dp-1.
# This command finds the icons / patterns in use by the stack being built, and sticks their ids into sIconA.
# Parameters
#   pStack : The short name of the stack being built
private command revEnumerateIcons pStack
   repeat for each line tIcon in the patterns of stack pStack
      if tIcon > 0 and tIcon < 137 then add 136 to tIcon
      if sIconA[tIcon] = "" and tIcon is not "" then put true into sIconA[tIcon]
   end repeat
   
   local tCardNum
   put the number of cards in stack pStack into tCardNum
   repeat with tCard = 1 to tCardNum
      repeat for each line tIcon in the patterns of cd tCard of stack pStack
         if tIcon > 0 and tIcon < 137 then add 136 to tIcon
         if sIconA[tIcon] = "" and tIcon is not "" then put true into sIconA[tIcon]
      end repeat
      
      local tControlNum
      put the number of controls in cd tCard of stack pStack into tControlNum
      repeat with tControl = 1 to tControlNum
         if word 1 of the long name of control tControl of cd tCard of stack pStack = "button" then
            repeat for each item tIcon in "icon,disabledicon,hilitedicon,visitedicon,armedicon"
               put the tIcon of control tControl of cd tCard of stack pStack into tIcon
               if sIconA[tIcon] = "" and tIcon is not "" then put true into sIconA[tIcon]
            end repeat
         end if
         repeat for each line tIcon in the patterns of control tControl of cd tCard of stack pStack
            if tIcon > 0 and tIcon < 137 then 
               add 136 to tIcon
            end if
            
            if sIconA[tIcon] = "" and tIcon is not "" then
               put true into sIconA[tIcon]
            end if
         end repeat
      end repeat
   end repeat
end revEnumerateIcons

private command revCopyReferencedFiles pStack, pFutureFilePath, pReferencedFilesDestination
   local tControl, tCard, tChars, tOldFilePath, tFileName, tFutureFileName, tFutureFilePath, tIcon, \
         tCardNum, tControlNum
   lock screen
   
   revStandaloneProgress "Copying referenced media..."
   set the itemDel to "/"
   put item 1 to -2 of the effective fileName of stack pStack & "/" into tOldFilePath
   put the number of chars of tOldFilePath into tChars
   set the itemDel to ","
   
   put the number of cards in stack pStack into tCardNum
   repeat with tCard = 1 to tCardNum
      put the number of controls in cd tCard of stack pStack into tControlNum
      repeat with tControl = 1 to tControlNum
         --repeat for each line tDirectory in sDirectoryList
         set the itemDel to "/"
         local tObject
         put the long id of control tControl of card tCard of stack pStack into tObject
         
         # OK-2010-04-27: Bug 8744 - Check that the filename is not empty before doing this
         if word 1 of tObject is "image" and the filename of tObject is not empty then
            revCopyReferencedFile tObject, item 1 to -2 of the filename of stack pStack, pFutureFilePath, pReferencedFilesDestination
            put false into sIconA[tObject]
         end if
         if word 1 of tObject is "player" then
            revCopyReferencedFile tObject, item 1 to -2 of the filename of stack pSTack, pFutureFilePath, pReferencedFilesDestination
         end if
         --end repeat
         set the itemDel to ","
      end repeat
   end repeat
end revCopyReferencedFiles

private command revCopyReferencedFile pObject, pStackOwnerFolderOld, pStackOwnerFolderNewRelative, pRefFilesDestination
   local tFileName, tRelativeFilename
   set the itemDelimiter to "/"
   
   put the filename of pObject into tFileName
   
   repeat for each line tDestinationFolder in sDirectoryList
      local tStackOwnerFolderNew
      put tDestinationFolder & pStackOwnerFolderNewRelative into tStackOwnerFolderNew
      local tReferencedFilesDestination
      put tDestinationFolder & pRefFilesDestination into tReferencedFilesDestination
      
      if revSBPathType(tFilename) is "relative" then
         put revSBCalculateAbsolutePath(pStackOwnerFolderOld, tFilename) into tFileName
      end if
      
      -- tFilename now holds the absolute path to the file to include..
      local tNewFilename, tLength
      put length(pStackOwnerFolderOld) into tLength
      if char 1 to tLength of tFilename is pStackOwnerFolderOld then
         -- The +2 offset skips a "/".
         put tStackOwnerFolderNew & (char tLength + 2 to -1 of tFilename) into tNewFilename
         revSBEnsureFolder (item 1 to -2 of tNewFilename)
         revSBCopyFileToFile tFilename, tNewFilename, "revStandaloneProgressCallback", the long id of me
      else
         local tCount
         put empty into tCount
         local tLeaf
         put item -1 of tFilename into tLeaf
         repeat forever
            set the itemDelimiter to "."
            put item 1 to -2 of tLeaf & tCount & "." & item -1 of tLeaf into tLeaf
            set the itemDelimiter to "/"
            put tReferencedFilesDestination & "/" & tLeaf into tNewFilename
            if there is no file tNewFilename then
               exit repeat
            end if
            add 1 to tCount
         end repeat
         revSBEnsureFolder tReferencedFilesDestination
         revSBCopyFileToFile tFilename, tNewFilename, "revStandaloneProgressCallback", the long id of me
      end if
      
   end repeat
   -- NB: referencing a variable "out of scope", if we had it. Who cares.
   -- We can just pick a folder here out of sDirectoryList, the relative calculation should be the same in all cases!
   get revSBCalculateRelativePath(tStackOwnerFolderNew, tNewFilename)
   set the filename of pObject to it
end revCopyReferencedFile

#####################################################################
# Saves a stack to a folder and if need be creates the folders
# to save into.
#####################################################################

private function revSaveToFolder pFolder, pStack, pGeneric
   local tCheckFolder,tFileName
   revStandaloneProgress "Checking directory exists..."
   set the itemDel to "/"
   --  put item 1 of pFolder into tCheckFolder
   --  repeat for each item tItem in item 2 to -1 of pFolder
   --    put "/"&tItem after tCheckFolder
   --    if there is not a folder tCheckFolder then
   --      create folder tCheckFolder
   --    end if
   --  end repeat
   revSBEnsureFolder pFolder
   -- 2007-12-18 Bug 3612 Changed the sGenericNumber from 1 to 0.
   if pGeneric and sGenericNumber is not 0 then -- don't rename the first (mainstack) generically
      put sGenericNumber into tFileName
   else if the effective filename of stack pStack is not "" then
      put item -1 of the effective filename of stack pStack into tFileName
   else
      put the short name of stack pStack & ".rev" into tFileName
   end if
   
   # OK-2008-10-31 : Bug 7372 - This asssumes that the name of the stack doesn't
   # already contain ".rev". In this case, the stack ends up being called something.rev.rev,
   # and the standalone builder breaks further along because it makes assumptions about 
   # the stack's filename. I'm not 100% sure that this fix is correct...
   if not (tFileName ends with ".livecode" or tFileName ends with ".rev") then 
      put ".rev" after tFileName
   end if
   lock screen
   // SN-2015-07-01: [[ Bug 15259 ]] We don't want the saving to be trapped by
   //  a saveStackRequest handler in the standalone stack
   lock messages
   save stack pStack as pFolder&tFileName
   unlock messages
   return tFileName
end revSaveToFolder

#####################################################################
#
# Copies selected database drivers. The database externals are now
# part of the engine so we just need the drivers.
#
# This code assumes there's a file in the database_drivers folder
# for each database type (Valentina,Oracle,ODBC etc). These files
# are named <database type>.txt and have al tab delimited list of
# platforms and driver files (relative to the rev engine directory).
#
# For windows an extra item may be included to identify the location of the
# database DLL to be copied next to the engine.
#
#####################################################################

private function revStandaloneDatabaseDriversPath pPlatform
   local tCurrentFolder, tDetailsA
   put revStandalonePlatformDetails(pPlatform) into tDetailsA
   
   # There are no legacy drivers for linux as we didn't have a linux version at the time the legacy drivers were current
   # If the legacy drivers are turned on but the folder is not present, we default to using the normal drivers, as the user could 
   # have deleted it.
   if the cREVUseLegacyDrivers of stack "revPreferences" and pPlatform is not "Linux" then
      local tLegacyFolder
      put revEnvironmentUserRuntimePath() & slash & tDetailsA["platform"] & slash & tDetailsA["architecture"] & "/Externals/Legacy" into tLegacyFolder
      if there is a folder tLegacyFolder then
         return tLegacyFolder
      else
         # Trigger warning but continue anyway...
         revStandaloneAddWarning "Legacy database drivers not found, using latest drivers instead. Location was: " & tLegacyFolder
      end if
   end if
   
   put tDetailsA["externals_folder"] into tCurrentFolder
   if there is no folder tCurrentFolder then
      revStandaloneAddWarning "Database drivers not found at location: " & tCurrentFolder
   end if
   
   return tCurrentFolder
end revStandaloneDatabaseDriversPath

private command revCopyDatabaseDrivers pPlatform, pStackFilePath, pStandalonePath, pEnginePath
   local tOriginalDriverPath,tNewDriverPath,tHomePath,tStandaloneFolder,tPlatformFile,tCurrentFileType
   local tDriverPath,tLibPath,tResourceList,i,tDriver,tDriverFile
   
   revStandaloneProgress "Copying database drivers..."
   
   if sStandaloneSettingsA["databaseDrivers"] is empty then
      exit revCopyDatabaseDrivers
   end if
   
   # AL-2015-02-06: [[ SB Inclusions ]] Ensure revDatabaseLibrary stack is included
   revAddLibrary "Database"
   
   local tDBLibPath
   put revStandaloneDatabaseDriversPath(pPlatform) into tDBLibPath
   
   local tExtension, tSharedExtension, tPlatform, tArch
   local tDetailsA
   put revStandalonePlatformDetails(pPlatform) into tDetailsA
   put tDetailsA["loadable_extension"] into tExtension
   if tDetailsA["shared_extension"] is not empty then
      put tDetailsA["shared_extension"] into tSharedExtension
   else
      put tExtension into tSharedExtension
   end if
   put tDetailsA["platform"] into tPlatform
   put tDetailsA["architecture"] into tArch
   
   set the itemDel to "/"
   put pStandalonePath into tNewDriverPath
   put pStandalonePath into tStandaloneFolder
   put "" into item -1 of tStandaloneFolder
   put "Externals/database_drivers/" into item -1 of tNewDriverPath
   
   if there is not a folder (tStandaloneFolder&"Externals") then 
      create folder (tStandaloneFolder&"Externals")
   end if
   
   if not revResourceExists(tDBLibPath) then
      revStandaloneAddWarning tPlatform & ", cannot find database driver:" && quote & tDBLibPath & quote
   else
      revCopyModule tPlatform, "revdb", "revdb" & tExtension, tDBLibPath, tStandaloneFolder, "Externals", tPlatform is "MacOSX"
      if the result is not empty then
         revStandaloneAddWarning tPlatform && tArch & ", external" && the result & ":" && quote & tDBLibPath & slash & "revdb" & tExtension & quote
      else
         if tPlatform is "MacOSX" and there is no file (tStandaloneFolder & "Externals/revdb.bundle/Contents/PkgInfo") then
            -- missing PkgInfo file, so copy it in quietly
            put "BNDL????" into URL ("binfile:" & tStandaloneFolder & "Externals/revdb.bundle/Contents/PkgInfo")
         end if
         revAddExternal "revdb"
      end if
   end if
   
   if there is not a folder tNewDriverPath then 
      create folder tNewDriverPath
   end if
   
   -- loop over drivers
   repeat for each line tDriver in sStandaloneSettingsA["databaseDrivers"]
      
      # OK-2010-01-13 : No Oracle support
      if tPlatform is "Linux" and tDriver is "Oracle" then
         revStandaloneAddWarning "Linux, Oracle is not currently supported on this platform"
         next repeat
      else if tDriver is "Oracle" and line 3 of the revlicenseinfo is "Community" then
         -- SN-2015-03-06: [[ DBDriverInclusion ]] Under community, the Oracle database
         -- is not visible, and should simply be ignored as it will fail.
         next repeat
      end if
      
      put revDBDriverPath(tDriver, tPlatform, tArch) into tDriverPath
      if tDriverPath is empty then
         revStandaloneAddWarning tPlatform && tArch & ", database driver not found:" & quote & tDriver & quote
         next repeat
      end if
      
      if not revResourceExists(line 1 of tDriverPath) then
         revStandaloneAddWarning tPlatform && tArch & ", database driver file not found:" & quote & tDriverPath & quote
         next repeat
      end if
      
      # Copy the resource for possible two lines of the associated driver path
      local tLineCount
      put 1 into tLineCount
      
      local tResourceName
      put tolower(tDriver) into tResourceName
      repeat for each line tLine in tDriverPath
         if the number of lines in tDriverPath > 1 then
            put tLineCount after tResourceName
         end if
         revCopyModule tPlatform, tResourceName, item -1 of tLine, item 1 to -2 of tLine, tStandaloneFolder, "Externals/database_drivers", tPlatform is "MacOSX"
         if the result is not empty then
            revStandaloneAddWarning tPlatform && tArch & ", database driver" && the result & ":" && quote & tDriverPath & quote
         else
            if tPlatform is "MacOSX" and there is no file (tStandaloneFolder & "Externals/revdb.bundle/Contents/PkgInfo") then
               -- missing PkgInfo file, so copy it in quietly
               put "BNDL????" into URL ("binfile:" & tStandaloneFolder & "Externals/revdb.bundle/Contents/PkgInfo")
            end if
         end if
      end repeat
      
      # Special case MacOS Valentina driver
      if tPlatform is "MacOSX" and tDriver = "Valentina" then
         if there is not a folder (item 1 to -2 of tStandaloneFolder&"/Frameworks") then create folder (item 1 to -2 of tStandaloneFolder&"/Frameworks")
         revSBCopyFolderToDestination (item 1 to -2 of tStandaloneFolder&"/MacOS/Externals/database_drivers/VXCMD_macho.bundle/Contents/Frameworks/VSDK.framework"),(item 1 to -2 of tStandaloneFolder&"/Frameworks"), "revStandaloneProgressCallback", the long id of me
         if the result is not empty then
            revStandaloneAddWarning ("Mac x86-32, failed to copy Valentina framework")
         end if
      end if 
   end repeat
end revCopyDatabaseDrivers

#####################################################################
# Copy all the externals over. MacOS resource numbers are listed
# in the cExternalExternalResValues of this stack. See the note
# about adding external libraries to the build process for MacOS.
#####################################################################

private command revAddExternal pExternalName
   if pExternalName is among the lines of sStandaloneSettingsA["externals"]  then
      exit revAddExternal
   end if

   // AL-2015-05-07: [[ Bug 15320 ]] Don't duplicate external names in standalone settings
   if sStandaloneSettingsA["externals"] is not empty then put return after sStandaloneSettingsA["externals"]
   put pExternalName after sStandaloneSettingsA["externals"]
end revAddExternal

constant kSeparator = ":"
private command revAddMapping pPlatform, pName, pLocation
   if sStandaloneSettingsA[pPlatform & ",library"] is not empty then put return after sStandaloneSettingsA[pPlatform & ",library"]
   put pName & kSeparator & "./" & pLocation after sStandaloneSettingsA[pPlatform & ",library"]
end revAddMapping

# AL-2015-01-29: [[ Scriptify IDE ]] Factor out external copy function to ensure external is included in deploy params
private command revCopyModule pPlatform, pModuleName, pSourceFile, pSourceFolder, pStandaloneRoot, pTargetFolder, pIsFolder
   local tSourcePath, tTargetPath, tTargetModule
   put pSourceFolder & slash & pSourceFile into tSourcePath
   if pTargetFolder is not empty then
      put pTargetFolder & slash & pSourceFile into tTargetModule
   else
      put pSourceFile into tTargetModule
   end if
   put pStandaloneRoot & slash & tTargetModule into tTargetPath
   
   if pIsFolder then
      if there is no folder tSourcePath then
         return "not found"
      end if
      if there is a folder pStandaloneRoot & slash & pTargetFolder then
         return empty
      end if 
      revSBCopyFolderToDestination tSourcePath, pStandaloneRoot & slash & pTargetFolder, "revStandaloneProgressCallback", the long id of me
   else
      if there is no file tSourcePath then
         return "not found"
      end if
      if there is a file tTargetPath then
         return empty
      end if
      revSBCopyFile tSourcePath, tTargetPath, "revStandaloneProgressCallback", the long id of me
   end if
   
   if the result is not empty then
      return "failed to copy"
   end if
   
   revAddMapping pPlatform, pModuleName, tTargetModule
end revCopyModule

private command revCopyCEFResources pPlatform, pStandalonePath, pCopyRevBrowserResources, pCopyLibBrowserResources
   if not (pCopyRevBrowserResources or pCopyLibBrowserResources) then
      // No CEF resources required
      exit revCopyCEFResources
   end if
   
   // IM-2014-08-14: [[ LibCef2062 ]] Reorganisation of libcef framework files
   local tDetails
   put revStandalonePlatformDetails(pPlatform) into tDetails
   local tExternalPath
   put revExternalPath("Browser", tDetails["platform"], tDetails["architecture"]) into tExternalPath
   // AL-2015-03-13: [[ Bug 14936 ]] Switch where case takes an expression should not switch on a value
   local tCurrentLocation, tCEFResources, tCEFDest
   set the itemdel to slash
   put (item 1 to -2 of tExternalPath) & "/CEF" into tCEFResources
   put item 1 to -2 of pStandalonePath into tCurrentLocation
   switch
      case pPlatform contains "MacOSX"
         local tCEFFramework
         put (item 1 to -3 of pStandalonePath) & "/Frameworks" into tCEFFramework
         
         revSBEnsureFolder tCEFFramework
         if the result is not empty then
            revStandaloneAddWarning "MacOS, external failed to create folder:" && quote & tCEFFramework & quote
         end if
         revSBCopyFolderToDestination tCefResources & "/Chromium Embedded Framework.framework", tCEFFramework, "revStandaloneProgressCallback", the long id of me
         if the result is not empty then
            revStandaloneAddWarning "MacOS, external failed to copy:" && quote & tCefResources & "/Resources" & quote
         end if
         
         local tHelpers
         if pCopyRevBrowserResources then
            put "revbrowser-cefprocess.app" & return after tHelpers
            put "revbrowser-cefprocess EH.app" & return after tHelpers
            put "revbrowser-cefprocess NP.app" & return after tHelpers
         end if
         if pCopyLibBrowserResources then
            put "libbrowser-cefprocess.app" & return after tHelpers
            put "libbrowser-cefprocess EH.app" & return after tHelpers
            put "libbrowser-cefprocess NP.app" & return after tHelpers
         end if
         repeat for each line tHelper in tHelpers
            revSBCopyFolderToDestination tCefResources & slash & tHelper, tCEFFramework, "revStandaloneProgressCallback", the long id of me
            if the result is not empty then
               revStandaloneAddWarning "MacOS, external failed to copy:" && quote & tCefResources & slash & tHelper & quote
            end if
         end repeat
         break
      case pPlatform = "Windows"
         put tCurrentLocation & slash & "Externals/CEF" into tCEFDest
         
         revSBEnsureFolder tCEFDest
         if the result is not empty then
            revStandaloneAddWarning "Windows, external failed to create folder:" && quote & tCEFDest & quote
         end if
         
         local tCEFFiles
         // make sure we only copy the required helper process executable
         put revListFiles(tCEFResources) into tCEFFiles
         filter lines of tCEFFIles not matching "*.exe"
         if the last char of tCEFFiles is not return then
            put return after tCEFFiles
         end if
         if pCopyRevBrowserResources then
            put "revbrowser-cefprocess.exe" & return after tCEFFiles
         end if
         if pCopyLibBrowserResources then
            put "libbrowser-cefprocess.exe" & return after tCEFFiles
         end if
         repeat for each line tCEFFile in tCEFFiles
            revSBCopyFileToFolder tCefResources & slash & tCEFFile, tCEFDest, "revStandaloneProgressCallback", the long id of me
            if the result is not empty then
               revStandaloneAddWarning "Windows, external failed to copy:" && quote & tCefResources & slash & tCEFFile & quote
            end if
         end repeat
         
         revSBCopyFolderToDestination tCefResources & slash & "locales", tCEFDest, "revStandaloneProgressCallback", the long id of me
         if the result is not empty then
            revStandaloneAddWarning "Windows, external failed to copy:" && quote & tCefResources & slash & "locales" & quote
         end if
         break
      case pPlatform contains "Linux"
         put tCurrentLocation & slash & "Externals/CEF" into tCEFDest
         
         revSBEnsureFolder tCEFDest
         if the result is not empty then
            revStandaloneAddWarning "Linux, external failed to create folder:" && quote & tCEFDest & quote
         end if
         
         // make sure we only copy the required helper process executable
         put revListFiles(tCEFResources) into tCEFFiles
         filter lines of tCEFFiles not matching "*-cefprocess"
         // icudtl.dat needs to be placed alongside the executable
         filter lines of tCEFFiles not matching "icudtl.dat"

         // Copy files to CEF resource folder.
         repeat for each line tCEFFile in tCEFFiles
            revSBCopyFileToFolder tCefResources & slash & tCEFFile, tCEFDest, "revStandaloneProgressCallback", the long id of me
            if the result is not empty then
               revStandaloneAddWarning "Linux, external failed to copy:" && quote & tCefResources & slash & tCEFFile & quote
            end if
         end repeat
         
         revSBCopyFolderToDestination tCefResources & slash & "locales", tCEFDest, "revStandaloneProgressCallback", the long id of me
         if the result is not empty then
            revStandaloneAddWarning "Linux, external failed to copy:" && quote & tCefResources & slash & "locales" & quote
         end if

         // Copy files to standalone folder
         put "icudtl.dat" & return into tCEFFiles
         if pCopyRevBrowserResources then
            put "revbrowser-cefprocess" & return after tCEFFiles
         end if
         if pCopyLibBrowserResources then
            put "libbrowser-cefprocess" & return after tCEFFiles
         end if
         
         repeat for each line tCEFFile in tCEFFiles
            revSBCopyFileToFolder tCefResources & slash & tCEFFile, tCurrentLocation, "revStandaloneProgressCallback", the long id of me
            if the result is not empty then
               revStandaloneAddWarning "Linux, external failed to copy:" && quote & tCefResources & slash & tCEFFile & quote
            end if
         end repeat
         
         break
      default
         break
   end switch
end revCopyCEFResources

private command revCopyExternals pPlatform, pStackFilePath, pEnginePath, pStandalonePath
   local tExternalRes, tColors, tCursorName, tCurrentLocation, tExternalPath, tExternals, tExternalLibrary
   local tCopyFiles, tOrigVisible, tPath, tExternalFile, x, tColor, tPattern, tExternalName
   
   local tScriptLibraries
   put sStandaloneSettingsA["scriptLibraries"] into tScriptLibraries
   
   local tAppBundle, tCEFFramework, tCEFResources, tCEFDest
   // IM-2014-03-20: [[ revBrowserCEF ]] Ensure revbrowser is included if revbrowser-cef is selected
   if "Browser (CEF)"  is among the lines of tScriptLibraries and "Browser" is not among the lines of tScriptLibraries then
      // AL-2015-03-13: [[ Bug 14936 ]] tScriptLibraries doesn't necessarily have an empty line at the end
      put "Browser" & cr before tScriptLibraries
   end if
   
   revStandaloneProgress "Copying externals..."
   
   lock screen
   -- good time to do the platform specific resource moving
   -- because we need to resave the mainstack anyway
   set the itemDel to "/"
   put item 1 to -2 of pStandalonePath into tCurrentLocation
   set the itemDel to ","
   
   local tSupportFolder, tExtension, tSharedExtension, tPlatform, tArch
   local tDetailsA
   put revStandalonePlatformDetails(pPlatform) into tDetailsA
   -- SN-2015-03-04: [[ IDE Restructure ]] revpdfprinter and revsecurity are in the support folder
   put tDetailsA["support_folder"] into tSupportFolder
   put tDetailsA["loadable_extension"] into tExtension
   if tDetailsA["shared_extension"] is not empty then
      put tDetailsA["shared_extension"] into tSharedExtension
   else
      put tExtension into tSharedExtension
   end if
   put tDetailsA["platform"] into tPlatform
   put tDetailsA["architecture"] into tArch
   
   
   # OK-2010-04-26: Bug 8539 - PDF Printer not included
   if sStandaloneSettingsA["pdfPrinter"] then
      revCopyModule tPlatform, "revpdfprinter", "revpdfprinter" & tExtension, tSupportFolder, tCurrentLocation, "", tPlatform is "MacOSX"
      if the result is not empty then
         revStandaloneAddWarning tPlatform && tArch & ", Support, revpdfprinter" & tExtension && the result & ": required for PDF printing."
      else
         --revAddExternal "revpdfprinter"
      end if
   end if
   
   -- MM-2014-01-29: [[ OpenSSL ]] We now build our own version of OpenSSL. 
   if "SSL & Encryption" is among the lines of sStandaloneSettingsA["scriptLibraries"] then
      revCopyModule tPlatform, "revsecurity", "revsecurity" & tSharedExtension, tSupportFolder, tCurrentLocation, "", false
      if the result is not empty then
         revStandaloneAddWarning tPlatform && tArch & ", Support, revsecurity" & tSharedExtension && the result & ": required for SSL."
      else
         --revAddExternal "revsecurity"
      end if
   end if
   
   switch
      case pPlatform contains "MacOSX"
         if sStandaloneSettingsA["cursors"] then put "revMacCursors" & return after tCopyFiles
         break
      case pPlatform = "Windows"
         if sStandaloneSettingsA["cursors"] then put "revCursors" & return after tCopyFiles
         break
      default
         if sStandaloneSettingsA["cursors"] then 
            put "revLinuxCursors" & return after tCopyFiles
         end if
         if sStandaloneSettingsA["UNIX,fileSelector"] then 
            put "file selector" & return after tCopyFiles
         end if
         if sStandaloneSettingsA["UNIX,pageSetup"] then 
            put "page setup" & return after tCopyFiles
         end if
         if sStandaloneSettingsA["UNIX,printerChooser"] then 
            put "printer chooser" & return after tCopyFiles
         end if
         if sStandaloneSettingsA["UNIX,colorChooser"] then
            put "color chooser" & return after tCopyFiles
         end if
         break
   end switch
   delete char -1 of tCopyFiles
   
   -- copy the resources into the standalone mainstack
   local tWaitCount
   
   # OK-2009-12-16 : Bug 8470 - Messages need to be locked here to prevent the IDE going crazy
   lock messages
   repeat for each line tLine in tCopyFiles
      add 1 to tWaitCount
      if tWaitCount mod 10 is 0 then wait 0 ticks with messages
      if tLine is empty then next repeat
      put the visible of stack tLine into tOrigVisible
      hide stack tLine
      clone stack tLine
      set the mainStack of stack ("copy of" && tLine) to the short name of stack pStackFilePath
      close stack tLine
      set the visible of stack tLine to tOrigVisible
      close stack ("copy of" && tLine)
      show stack ("copy of" && tLine)
      set the name of stack ("copy of" && tLine) to tLine
   end repeat
   unlock messages
   -- copies the remaining external libraries
   
   -- hack
   unlock screen; unlock screen
   wait 0 ticks with messages -- allow for progress update
   
   if there is not a folder (tCurrentLocation & slash & "Externals") then 
      create folder (tCurrentLocation & slash & "Externals")
   end if
   
   set the itemDel to slash
   repeat for each line tExternalName in revExternalsList()
      
      -- check this item is selected for installation
      if tExternalName is not among the lines of tScriptLibraries then
         next repeat
      end if
      
      -- get the path to the external
      put revExternalPath(tExternalName, tPlatform, tArch) into tExternalPath
      
      -- SN-2015-04-15: [[ Bug 15231 ]] We don't want to copy from an empty path
      --  in case we cannot find any Runtime/<platform>/<arch>/Externals/externals.txt file
      if tExternalPath is empty then
         revStandaloneAddWarning tPlatform && tArch & ", external" && "No externals.txt config file found for" &&tExternalName
         next repeat
      end if
      
      -- SN-2015-03-06: [[ DBDriverInclusion ]] In case Database drivers were
      -- included, then the revdb external has been copied.
      -- No issue to copy over it on Windows / Linux, but copying a folder (revdb.bundle)
      -- will fail on Mac.
      local tSourceExternal
      put item -1 of tExternalPath into tSourceExternal
      if tSourceExternal contains "revdb" and \
            (there is a folder (tCurrentLocation & "/Externals/" &  tSourceExternal) \
            or there is a file (tCurrentLocation & "/Externals/" & tSourceExternal)) then
         next repeat
      end if
      
      // AL-2015-05-07: [[ Bug 15320 ]] Make sure universal external name is used.
      local tExternal
      set the itemdel to "."
      put item 1 of tSourceExternal into tExternal
      set the itemDel to slash 
      revCopyModule tPlatform, tExternal, tSourceExternal, item 1 to -2 of tExternalPath, tCurrentLocation, "Externals", tPlatform is "MacOSX"
      if the result is not empty then
         revStandaloneAddWarning tPlatform && tArch & ", external" && the result & ":" && quote & tExternalPath & quote
         next repeat
      else if tPlatform is "MacOSX" and there is no file (tCurrentLocation & slash & "Externals/" & (item -1 of tExternalPath) & "/Contents/PkgInfo") then
         if ".bundle" is in item -1 of tExternalPath then
            put "BNDL????" into URL ("binfile:" & (tCurrentLocation & slash & "Externals/" & (item -1 of tExternalPath) & "/Contents/PkgInfo"))
         end if
      end if
      revAddExternal tExternal
   end repeat
   
   // SN-2015-07-01: [[ Bug 15259 ]] We don't want the saving to be trapped by
   //  a saveStackRequest handler in the standalone stack
   lock messages
   save stack pStackFilePath
   unlock messages
   
   -- MM-2013-04-05: [[ Bug 10756 ]] Make sure we're allowed to delete the stack before deleteing.
   set the cantDelete of stack pStackFilePath to false
   delete stack pStackFilePath
end revCopyExternals

on tmpLog pMsg
   #put pMSg & return after msg
end tmpLog

private command log pMessage
   put pMessage & return after msg
end log

#####################################################################
# This is the engine attachment part. Ideally this can be re-used
# by the MetaCard Standalone Builder without much hassle.
#
# MetaCard developers will need to create a handler that loads
# the cRevStandaloneSettings into sStandaloneSettings
#####################################################################

private command revBuildStandaloneDeploy pPlatform, pStackFilePath, pEnginePath, pStandalonePath
   -- Make sure things like 'platform' are standard
   local tPlatformDetailsA
   put revStandalonePlatformDetails(pPlatform) into tPlatformDetailsA
   
   -- This array contains the parameters for the deploy command
   local tDeployInfo
   
   -- Allows the test system to build a standalone against any version   
   if revOverrideRuntimePath() is not empty then
      put revOverrideRuntimePath() into pEnginePath
   end if
   
   -- Set up file references common to all platforms
   put pStackFilePath into tDeployInfo["stackfile"]
   put pStandalonePath into tDeployInfo["output"]
   
   # AL-2015-01-29: [[ Scriptify IDE ]] Add the library inclusions to the deploy parameters
   repeat for each line tLibrary in sStandaloneSettingsA["scriptLibraries"]
      # AL-2015-03-05: [[ Bug 14813 ]] Transform script library display name to stack name at deploy time
      local tLibraryStack
      put revSBStackNameFromScriptLibraryName(tLibrary) into tLibraryStack
      if there is a stack tLibraryStack then
         if sStandaloneSettingsA["auxiliary_stackfiles"] is not empty then put return after sStandaloneSettingsA["auxiliary_stackfiles"]
         put the effective filename of stack tLibraryStack after sStandaloneSettingsA["auxiliary_stackfiles"]
         if sStandaloneSettingsA["startup_script"] is not empty then put return after sStandaloneSettingsA["startup_script"]
         put "revInternal__LoadLibrary" && tLibraryStack after sStandaloneSettingsA["startup_script"]
      end if
   end repeat
   if sStandaloneSettingsA["auxiliary_stackfiles"] is not empty then
      local tRevInitialisationLibrary
      put the effective filename of stack "revInitialisationLibrary" into tRevInitialisationLibrary
      if tRevInitialisationLibrary is not among the lines of sStandaloneSettingsA["auxiliary_stackfiles"] then
         put tRevInitialisationLibrary & cr before sStandaloneSettingsA["auxiliary_stackfiles"]
         put "insert the script of stack" && quote & "revInitialisationLibrary" & quote && "into back" & cr before sStandaloneSettingsA["startup_script"]
      end if
      repeat for each line tStackFile in sStandaloneSettingsA["auxiliary_stackfiles"]
         local tTempStackFile
         put the tempname into tTempStackFile
         revSBResaveScriptOnlyStackAsProperStackFile the short name of stack tStackFile, tStackFile, tTempStackFile
         put tTempStackFile & return after tDeployInfo["auxiliary_stackfiles"]
      end repeat
      delete the last char of tDeployInfo["auxiliary_stackfiles"]
   end if
   
   # AL-2015-01-29: [[ Scriptify IDE ]] Add user script libraries to deploy parameters
   repeat for each line tLine in sUserScriptLibraries
      put return & "if there is a stack" && tLine && "then library" && tLine after sStandaloneSettingsA["startup_script"]
   end repeat
   
   # AL-2015-01-29: [[ Scriptify IDE ]] Add OSX App icons to deploy parameters
   if tPlatformDetailsA["platform"] is "MacOSX" then
      if sStandaloneSettingsA["OSX,appicon"]  is not empty then
         put return & "revInternal__SetAppIcon" && sStandaloneSettingsA["OSX,appicon"] after sStandaloneSettingsA["startup_script"]
      end if
      if sStandaloneSettingsA["OSX,smallappicon"]  is not empty then
         put return & "revInternal__SetSmallAppIcon" && sStandaloneSettingsA["OSX,smallappicon"] after sStandaloneSettingsA["startup_script"]
      end if
   end if   
   
   # AL-2015-01-29: [[ Scriptify IDE ]] Add the startup script to the deploy parameters
   if sStandaloneSettingsA["startup_script"] is not empty then
      put sStandaloneSettingsA["startup_script"] into tDeployInfo["startup_script"]
   end if
   
   # AL-2015-02-04: [[ Standalone Resources ]] Add (universal) external names and resource mappings to the deploy parameters
   if sStandaloneSettingsA["externals"] is not empty then
      put sStandaloneSettingsA["externals"] into tDeployInfo["externals"]
   end if
   if sStandaloneSettingsA[tPlatformDetailsA["platform"] & ",library"] is not empty then
      put sStandaloneSettingsA[tPlatformDetailsA["platform"] & ",library"] into tDeployInfo["library"]
   end if
   
   # AL-2015-04-12: [[ Standalone Extensions ]] Add chosen extensions to the standalone inclusions
   # AL-2015-07-21: [[ Standalone Extensions ]] Automatically include dependecies, and load in order
   repeat for each line tExtension in revIDEExtensionsOrderByDependency(sStandaloneSettingsA["extensions"])
      local tExtensionFile
      put revIDEExtensionModuleFile(tExtension) into tExtensionFile
      if tDeployInfo["modules"] is empty then
         put tExtensionFile into tDeployInfo["modules"]
      else
         put return & tExtensionFile after tDeployInfo["modules"]
      end if
   end repeat
   
   if tPlatformDetailsA["platform"] is "MacOSX" then
      -- MW-2013-06-13: [[ CloneAndRun ]] If installed do things in the usual way, otherwise pass
      --    through in 'engine' (so it builds using archs present there).
      if revEnvironmentIsInstalled() then
         -- MM-2014-03-21: [[ PPC Support Dropped ]] We now only pass a single Intel engine path.
         put pEnginePath into tDeployInfo["engine_x86"]
      else
         put pEnginePath into tDeployInfo["engine"]
      end if
      -- Disabled for now - will enable when its been properly tested
      //if line 1 of pEnginePath is not empty and line 2 of pEnginePath is not empty then
      //put pStandalonePath & ".dat" into tDeployInfo["spill"]
      //end if
   else
      put pEnginePath into tDeployInfo["engine"]
   end if
   
   -- Set windows-specific info
   if tPlatformDetailsA["platform"] is "Windows" then
      -- Make sure we include the app icon if any
      if sStandaloneSettingsA["Windows,iconFile"] is not empty then
         put sStandaloneSettingsA["Windows,iconFile"] into tDeployInfo["appIcon"]
         -- MW-2013-06-14: [[ CloneAndRun ]] Use the icon from 'rsrc' if generic.
         if not revEnvironmentIsInstalled() and tDeployInfo["appIcon"] contains "genericapp.ico" then
            put revEnvironmentRepositoryPath() & slash & "engine/rsrc/standalone.ico" into tDeployInfo["appIcon"]
         end if
      end if
      
      -- Make sure we include the doc icon if any
      if sStandaloneSettingsA["Windows,documentIconFile"] is not empty then
         put sStandaloneSettingsA["Windows,documentIconFile"] into tDeployInfo["docIcon"]
         -- MW-2013-06-14: [[ CloneAndRun ]] Use the icon from 'rsrc' if generic.
         if not revEnvironmentIsInstalled() and tDeployInfo["docIcon"] contains "genericdoc.ico" then
            put revEnvironmentRepositoryPath() & slash & "engine/rsrc/document.ico" into tDeployInfo["docIcon"]
         end if
      end if
      
      -- Make sure we use the user's version info data
      put sStandaloneSettingsA["Windows,fileversion1"]&"."&sStandaloneSettingsA["Windows,fileversion2"]&"." & sStandaloneSettingsA["Windows,fileversion3"]&"."&sStandaloneSettingsA["Windows,fileversion4"] into sStandaloneSettingsA["Windows,FileVersion"]
      put sStandaloneSettingsA["Windows,productversion1"]&"."&sStandaloneSettingsA["Windows,productversion2"]&"." & sStandaloneSettingsA["Windows,productversion3"]&"."&sStandaloneSettingsA["Windows,productversion4"] into sStandaloneSettingsA["Windows,ProductVersion"]
      repeat for each item tField in kVersionFlds
         put sStandaloneSettingsA["Windows", tField] into tDeployInfo["version"][tField]
      end repeat
      
      // IM-2014-08-15: [[ Bug 13118 ]] Split manifest sections into separate template files.
      local tManifestData, tManifestPath, tManifestNeeded
      put empty into tManifestData
      put false into tManifestNeeded
      
      put revEnvironmentRuntimePath() &  "/Windows/x86-32/w32-manifest-template.xml" into tManifestPath
      if there is not a file tManifestPath then
         revStandaloneAddWarning "Windows, manifest template not found: " & quote & tManifestPath & quote
      else
         put url ("file:" & tManifestPath) into tManifestData
      end if
      
      # OK-2009-09-25 : UAC Manifest. If the user has choosen "default" from the drop-down, or the stack doesn't have this setting at all, we don't
      # include the manifest.
      local tManifestTrustInfo
      put empty into tManifestTrustInfo
      
      if sStandaloneSettingsA["Windows,uacExecutionLevel"] is not empty and sStandaloneSettingsA["Windows,uacExecutionLevel"] is not "default" then
         local tManifestTrustInfoPath
         put revEnvironmentRuntimePath() & "/Windows/x86-32/w32-manifest-template-trustinfo.xml" into tManifestTrustInfoPath
         
         if there is not a file tManifestTrustInfoPath then
            revStandaloneAddWarning "Windows, manifest trustinfo template not found: " & quote & tManifestTrustInfoPath & quote
         else
            put url ("file:" & tManifestTrustInfoPath) into tManifestTrustInfo
            replace "[[executionLevel]]" with sStandaloneSettingsA["Windows,uacExecutionLevel"] in tManifestTrustInfo
            put true into tManifestNeeded
         end if
      end if
      
      local tManifestApplication
      
      // IM-2014-08-15: [[ Bug 13118 ]] Pull in dpiAware section if set.
      if sStandaloneSettingsA["Windows,usePixelScaling"] is empty or sStandaloneSettingsA["Windows,usePixelScaling"] is true then
         local tManifestApplicationPath
         put revEnvironmentRuntimePath() & "/Windows/x86-32/w32-manifest-template-dpiaware.xml" into tManifestApplicationPath
         
         if there is not a file tManifestApplicationPath then
            revStandaloneAddWarning "Windows, manifest dpiaware template not found: " & quote & tManifestApplicationPath & quote
         else
            put url ("file:" & tManifestApplicationPath) into tManifestApplication
            put true into tManifestNeeded
         end if
      end if
      
      if tManifestNeeded and tManifestData is not empty then
         replace "[[trustInfo]]" with tManifestTrustInfo in tManifestData
         replace "[[application]]" with tManifestApplication in tManifestData
         
         local tManifestFile
         put the tempName into tManifestFile
         put tManifestData into url ("binfile:" & tManifestFile)
         put tManifestFile into tDeployInfo["manifest"]
      end if
   end if
   
   -- Resave the stack (not sure what this is for exactly, but was present
   -- in the original handler and serves our purposes to guarantee the right
   -- origin info).
   revStandaloneProgress "Preparing main stackfile..."
   wait 0 ticks with messages
   lock messages
   lock screen
   save stack pStackFilePath
   -- MM-2013-04-05: [[ Bug 10756 ]] Make sure we're allowed to delete the stack before deleteing.
   set the cantDelete of stack pStackFilePath to false
   delete stack pStackFilePath
   unlock screen
   unlock messages
   wait 0 ticks with messages
   
   -- Actually do the standalone engine build - we probably need some sort of
   -- error handling here :oD
   revStandaloneProgress "Attaching engine..."
   switch word 1 of pPlatform
      case "Windows"
         _internal deploy windows tDeployInfo
         break
      case "Linux"
         _internal deploy linux tDeployInfo
         break
      case "MacOSX"
         _internal deploy macosx tDeployInfo
         break
   end switch
   if the result is not empty then
      throw "Build failed for " & pPlatform & ":" && the result
   end if
   
   -- Again, present in the original, this will remove the stackfile we saved
   -- above.
   revDeleteFile pStackFilePath
   
   -- Make sure we finish off the bundle contents
   if pPlatform contains "MacOSX" then
      -- MM-2014-03-21: [[ PPC Support Dropped ]] Engine path should now only contain a single line (the Intel engine).
      revSetMacOS pStandalonePath, pPlatform, pEnginePath
      
      revRedirectMacOSResources pStandalonePath
   end if
end revBuildStandaloneDeploy

command revRedirectMacOSResources pStandalonePath
   local tOldRFolder, tNewRFolder
   set the itemDelimiter to slash
   put item 1 to -2 of pStandalonePath into tOldRFolder
   put (item 1 to -2 of tOldRFolder) & "/Resources/_MacOS" into tNewRFolder
   
   create folder (tOldRFolder & slash & "../Resources")
   create folder (tOldRFolder & slash & "../Resources/_MacOS")
   
   local tOldFolder
   put the folder into tOldFolder
   
   revRedirectMacOSResourcesRecurse tOldRFolder, tNewRFolder
   
   set the folder to tOldFolder
end revRedirectMacOSResources

command revRedirectMacOSResourcesRecurse pSourceFolder, pTargetFolder
   set the folder to pSourceFolder
   create folder pTargetFolder
   
   local tFiles
   repeat for each line tFile in the files
      if tFile begins with "." then
         next repeat
      end if
      if tFile ends with ".dylib" then
         next repeat
      end if
      if revRedirectMacOSResourcesIsExecutable(pSourceFolder & slash & tFile) then
         next repeat
      end if
      rename (pSourceFolder & slash & tFile) to (pTargetFolder & slash & tFile)
   end repeat
   
   local tFolder
   repeat for each line tFolder in the folders
      if tFolder begins with "." then
         next repeat
      end if
      if tFolder ends with ".bundle" then
         next repeat
      end if
      if tFolder ends with ".app" then
         next repeat
      end if
      revRedirectMacOSResourcesRecurse pSourceFolder & slash & tFolder, pTargetFolder & slash & tFolder
   end repeat
end revRedirectMacOSResourcesRecurse

function revRedirectMacOSResourcesIsExecutable pFile
   local tIsExecutable
   put false into tIsExecutable
   
   open file pFile for binary read
   read from file pFile for 1 int4
   if the result is not "eof" then
      if it is 0xCAFEBABE or it is 0xBEBAFECA or \
            it is 0xFEEDFACE or it is 0xCEFAEDFE then
         put true into tIsExecutable
      end if
   end if
   close file pFile
   
   return tIsExecutable
end revRedirectMacOSResourcesIsExecutable

------------------- SMART CHECKING --------------------

private command revSmartChecking pStackFilesList, pSearchForInclusions
   local tPath,tLine,tStack,tList,tHandler,tUserLibs,tStackFile,tCard,tControl,tSave,tStacksList,tDB,tProps,tCardNum,tControlNum,tBg,tEnvironmentIcons, tUserIcons
   local tFiles
   revStandaloneProgress "Auto detecting library inclusions..."
   lock screen
   lock messages
   
   if pSearchForInclusions then
      put empty into sStandaloneSettingsA["scriptLibraries"]
      put true into sStandaloneSettingsA["cursors"]
      put false into sStandaloneSettingsA["answerDialog"]
      put false into sStandaloneSettingsA["askDialog"]
      put false into sStandaloneSettingsA["brushes"]   --LG-2007-12-13: Bug 4370
      put false into sStandaloneSettingsA["Magnify"]  -- PM-2015-06-24: [[ Bug 15535 ]] Make possible to include magnify palette in standalones
      put false into sStandaloneSettingsA["pdfPrinter"]
      
      # OK-2007-11-13 : Support for LiveCode print dialogs
      put false into sStandaloneSettingsA["revolutionPrintDialogs"]
   end if
   
   put revAbsoluteFolderListing(revEnvironmentResourcesPath("Script Libraries")) into tEnvironmentIcons
   put revAbsoluteFolderListing(revEnvironmentUserResourcesPath("Script Libraries")) into tUserIcons
   
   repeat for each line tStackFile in pStackFilesList["full"]
      put the short name of stack tStackFile & return & the substacks of stack tStackFile into tStacksList
      repeat for each line tStack in tStacksList
         # OK-2009-09-07 : Value of password and passkey properties changed, the above test no longer works
         if the password of stack tStack then
            revStandaloneAddWarning "Could not auto-detect inclusions or security categories because stack is password protected"
            exit revSmartChecking
         end if
         
         revCheckObject the long name of stack tStack, tUserLibs, pSearchForInclusions
         put the number of cards of stack tStack into tCardNum
         repeat with tCard=1 to tCardNum
            revCheckObject the long id of cd tCard of stack tStack, tUserLibs, pSearchForInclusions
            put the number of controls of cd tCard of stack tStack into tControlNum
            repeat with tControl = 1 to tControlNum
               revCheckObject the long id of control tControl of cd tCard of stack tStack, tUserLibs, pSearchForInclusions
            end repeat
         end repeat
         repeat for each line tBg in the backgroundIDs of stack tStack
            revCheckObject the long id of bg id tBg of stack tStack, tUserLibs, pSearchForInclusions
         end repeat
      end repeat
   end repeat
   
   if pSearchForInclusions then
      put lineOffset("database", sStandaloneSettingsA["scriptLibraries"]) into tDB
      if tDB is not 0 then
         -- import all drivers because we don't know which they are using
         put revDatabaseDriverList() into sStandaloneSettingsA["databaseDrivers"]
         delete line tDB of sStandaloneSettingsA["scriptLibraries"]
      end if
   end if
end revSmartChecking

private command revAddLibrary pLibrary
   if pLibrary is among the lines of sStandaloneSettingsA["scriptLibraries"]  then
      exit revAddLibrary
   end if
   
   if sStandaloneSettingsA["scriptLibraries"] is not empty then put return after sStandaloneSettingsA["scriptLibraries"]
   put pLibrary after sStandaloneSettingsA["scriptLibraries"]
end revAddLibrary

private command revCheckObject pObject,pUserLibraries, pSearchForInclusions
   # AL-2015-01-29: [[ Scriptify revSaveAsStandalone ]] Use script locals to check for library inclusions
   if pSearchForInclusions then
      local tProps, tLib, tProperty
      put the customPropertySets of pObject into tProps
      repeat for each key tLib in sPatternsA
         # AL-2015-01-29: Don't search the script for handlers if this lib is already included
         if tLib is among the lines of sStandaloneSettingsA["scriptLibraries"]  then
            next repeat
         end if
         
         repeat for each item tProperty in sPatternsA[tLib]["Property"]
            if lineOffset(tProperty, tProps) is not 0 then
               revAddLibrary tLib
               # AL-2015-01-29: If we are including this lib then go on to the next one
               exit repeat
            end if
         end repeat
      end repeat
   end if
   
   # OK-2009-09-07 : Ensure script is never accessed without first checking for password or
   # standalone building will fail.
   if the password of stack (revTargetStack(pObject)) then
      exit revCheckObject
   end if
   
   local tScript
   put the script of pObject into tScript
   
   if pSearchForInclusions then
      if tScript is not empty then
         
         repeat for each key tLib in sPatternsA
            # AL-2015-01-29: Don't search the script for handlers if this lib is already included
            if tLib is among the lines of sStandaloneSettingsA["scriptLibraries"]  then
               next repeat
            end if
            
            local tHandler
            repeat for each item tHandler in sPatternsA[tLib]["Script"]
               if lineOffset(tHandler,tScript) is not 0 then
                  revAddLibrary tLib
                  # AL-2015-01-29: If we are including this lib then go on to the next one
                  exit repeat
               end if
            end repeat
            
            # AL-2015-01-29: Don't search the script for roots if this lib is already included
            if tLib is among the lines of sStandaloneSettingsA["scriptLibraries"]  then
               next repeat
            end if
            
            local tRoot
            repeat for each item tRoot in sPatternsA[tLib]["Root"]
               if lineOffset(tRoot,tScript) is not 0 then
                  revAddLibrary tLib
                  # AL-2015-01-29: If we are including this lib then go on to the next one
                  exit repeat
               end if
            end repeat
            
         end repeat
         
      end if
   end if
   
   if pSearchForInclusions then
      -- answer dialog
      if lineOffset("answer",tScript) is not 0 then
         put true into sStandaloneSettingsA["answerDialog"]
      end if
      -- ask dialog
      if lineOffset("ask",tScript) is not 0 then
         put true into sStandaloneSettingsA["askDialog"]
      end if
      -- pdf printer
      if lineOffset("open printing to pdf", tScript) is not 0 then
         put true into sStandaloneSettingsA["pdfPrinter"]
      end if
      
      # OK-2007-11-13 : Support for LiveCode print dialogs. The dialogs are included if there is any reference to systemPrintSelector in the script,
      # or if we are building for linux. (This might include the files more often than they are required).
      if offset("systemPrintSelector", tScript) is not 0 or "Linux" is among the lines of sPlatformList then
         put true into sStandaloneSettingsA["revolutionPrintDialogs"]
      end if
   end if
end revCheckObject

private function revResourceExists pResource
  # pResource is either a file, folder or bundle.
  return (there is a file pResource or there is a folder pResource)
end revResourceExists
