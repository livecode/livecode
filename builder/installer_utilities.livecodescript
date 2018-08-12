script "InstallerUtilities"
# This is the back-end installer, updater and uninstaller code. 
# This code doesn't know anything about the product it is installing, and should be capable
# of being re-used for products other than LiveCode. It reads a "manifest" file, which contains
# a list of files to be installed and some metadata. The files are then installed according to command
# line options. Uninstallation uses a "record" file, which is created by the installer. This is basically the same
# as the manifest file, but with a little less information.

constant kRunOnce = "HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\RunOnce"

################################################################################

local sIsFaceless
local sFacelessLog

private function isInstallerEnvironment
   return (the environment begins with "installer")
end isInstallerEnvironment

################################################################################
#
#   FRONT-END FUNCTIONS
#

local sInstallerStack
local sInstallerProcess
local sInstallerError

command installerRun pPath, pRecord, pOptions, pWithoutElevation
   local tCommand
   if isInstallerEnvironment() then
      put the effective filename of stack "Installer" into tCommand
   else
      put specialFolderPath("Desktop") & "/installbackend.exe" into tCommand
   end if
   
   if tCommand contains space then
      put quote & tCommand & quote into tCommand
   end if
   
   local tArguments
   put "doinstall -path" && quote & pPath & quote && "-record" && quote & pRecord & quote into tArguments
   
   repeat for each key tOption in pOptions
      if pOptions[tOption] is "true" then
         put space & "-enable" & tOption after tArguments
      else if pOptions[tOption] is "false" then
         put space & "-disable" & tOption after tArguments
      else
         put space & tOption && quote & pOptions[tOption] & quote after tArguments
      end if
   end repeat
   
   put empty into sInstallerError
   put tCommand && tArguments into sInstallerProcess
   put the long id of stack "InstallerUI" of stack "Installer" into sInstallerStack
   
   -- Make sure we do a log entry non-admin side. This ensures it has the correct owner on Linux.
   log "Commencing install action with:" && sInstallerProcess
   
   if pWithoutElevation or elevationNotSupported() then
      open process sInstallerProcess for binary update
   else
      do "open elevated process sInstallerProcess for binary update"
   end if
   if the result is not empty then
      if the result is "access denied" then
         installerDispatchFinished "Request to launch elevated installer slave failed."
      else
         installerDispatchFinished "Could not launch installer slave."
      end if
      exit installerRun
   end if

   installerMonitor
end installerRun

command installerRunUninstall pPath, pRecord, pWithoutElevation
   local tCommand
   if isInstallerEnvironment() then
      put the effective filename of stack "Installer" into tCommand
   else
      put "C:/Users/Mark/Desktop/installbackend.exe" into tCommand
   end if
   
   if tCommand contains space then
      put quote & tCommand & quote into tCommand
   end if
   
   local tArguments
   put "douninstall -path" && quote & pPath & quote && "-record" && quote & pRecord & quote into tArguments
   
   put empty into sInstallerError
   put tCommand && tArguments into sInstallerProcess
   put the long id of stack "UninstallerUI" of stack "Installer" into sInstallerStack
   
   -- Make sure we do a log entry non-admin side. This ensures it has the correct owner on Linux.
   log "Commencing uninstall action with:" && sInstallerProcess
   
   if pWithoutElevation or elevationNotSupported() then
      open process sInstallerProcess for binary update
   else
      do "open elevated process sInstallerProcess for binary update"
   end if
   if the result is not empty then
      if the result is "access denied" then
         installerDispatchFinished "Request to launch elevated uninstaller slave failed."
      else
         installerDispatchFinished "Could not launch uninstaller slave."
      end if
      exit installerRunUninstall
   end if
   
   installerMonitor
end installerRunUninstall

-- Temporary function to determine if elevation is not supported on linux
private function elevationNotSupported
   if the platform is not "linux" then
      return false
   end if
   
   set the itemDelimiter to ":"
   repeat for each item tPath in $PATH
      if there is a file (tPath & slash & "gksu") then
         return false
      end if
   end repeat
   
   return true
end elevationNotSupported

command installerMonitor
   -- We loop each time until there is no more data to read from the slave process.
   set the itemDelimiter to tab
   
   repeat forever
      -- If the slave process has gone away unexpectedly, dispatch a finished.
      if sInstallerProcess is not among the lines of the openProcesses then
         put empty into sInstallerProcess
         installerDispatchFinished "Installer slave quit without eof."
         exit repeat
      end if
      
      -- Attempt to read a line from the installer process
      read from process sInstallerProcess for 1 line in 0 millisecs
      switch the result
         case empty
            -- If there was no error, process the line (first eliminate the return char at end)
            get line 1 of it
            if item 1 of it is "error" then
               put item 2 of it into sInstallerError
            else if item 1 of it is "phase" then
               installerDispatchPhase item 2 of it
            else if item 1 of it is "progress" then
               installerDispatchProgress item 2 of it, item 3 of it
            else if item 1 of it is "done" then
               close process sInstallerProcess
               put empty into sInstallerProcess
               installerDispatchFinished
               exit repeat
            end if
            break
         case "timed out"
            -- If the read timed out, exit the repeat
            exit repeat
            break
         case "eof"
         default
            -- If there was an eof or unexpected error, close the process and dispatch
            -- finished
            close process sInstallerProcess
            put empty into sInstallerProcess
            installerDispatchFinished "Installer slave quit unexpectedly."
            exit repeat
            break
      end switch
   end repeat
   
   -- If we still have a process, then send an update message in a short period of time
   if sInstallerProcess is not empty then
      send "installerMonitor" to me in 20 millisecs
   end if
end installerMonitor

command installerCancel
   if sInstallerProcess is not empty and sInstallerProcess is among the lines of the openProcesses then
      write "cancel" & return to process sInstallerProcess
   end if
end installerCancel

private command installerDispatchPhase pPhase
   dispatch "installerPhaseChanged" to this card of stack sInstallerStack with pPhase
end installerDispatchPhase

private command installerDispatchProgress pPercentage, pMessage
   dispatch "installerProgressChanged" to this card of stack sInstallerStack with pPercentage, pMessage
end installerDispatchProgress

private command installerDispatchFinished pDefaultError
   if sInstallerError is empty then
      put pDefaultError into sInstallerError
   end if
   
   dispatch "installerFinished" to this card of stack sInstallerStack with sInstallerError
   put empty into sInstallerError
end installerDispatchFinished

################################################################################
#
#    BACK-END FUNCTIONS
#

-- InstallerUI mode requires no command line options and opens the 'InstallerUI'
-- substack to continue the install. If, however, there is a second parameter it is
-- taken to be where the install should occur - skipping the target selection screen.
command runInstallerUI
   -- We are showing a UI, so hide this stack and switch the process
   -- to foreground (required for Mac OS X).
   hide me
   
   -- Configure the UI options and switch to the installer UI stack.
   go invisible to card "Launch" of stack "InstallerUI"
end runInstallerUI

-- UninstallerUI mode requires no command line options and opens the
-- 'UinstallerUI' substack to continue the uninstall.
command runUninstallerUI
   -- We are showing a UI, so hide this stack and switch the process
   -- to foreground (required for Mac OS X).
   hide me
   
   -- Configure the UI options and switch to the uninstaller UI stack
   go invisible to card "Launch" stack "UninstallerUI"
end runUninstallerUI

-- Faceless install option
command runFacelessInstall
   put true into sIsFaceless
   
   try
      send "runFaceless" to stack "InstallerUI"
   catch tError
      quit 1
   end try
   quit 0
end runFacelessInstall

-- Faceless uninstall option
command runFacelessUninstall
   put true into sIsFaceless
   
   try
      send "runFaceless" to stack "UninstallerUI"
   catch tError
      quit 1
   end try
   quit 0
end runFacelessUninstall

-- InstallerActions mode is backend process for installation. It reads the
-- following command line options:
--     -path <target> : the location the install should occur
--     -record <target> : where the record file should be placed
--     -enable-* : enable the given option
--     -disable-* : disable the given option
-- The record and path options are mandatory.
command runInstallerActions
   log "Installer actions launched from '" & $0 & "'."
   
   try
      -- The installer requires the parameter '-path' to determine where
      -- it is reinstalling.
      local tPath
      put ensureStringOption("path") into tPath
      
      -- Next parse the record parameter.
      local tRecord
      put ensureStringOption("record") into tRecord
      
      -- Now parse the boolean enable/disable options.
      local tOptions
      put parseBooleanOptions() into tOptions
      
      -- Run the install action.
      actionInstall tPath, tRecord, tOptions
   catch tError
      quit 1
   end try
   quit 0
end runInstallerActions

command runUninstallerActions
   log "Uninstaller actions launched from '" & $0 & "'."
   
   try
      -- The installer requires the parameter '-path' to determine where
      -- it is reinstalling.
      local tPath
      put ensureStringOption("path") into tPath
      
      -- Next parse the record parameter.
      local tRecord
      put ensureStringOption("record") into tRecord
      
      -- Now parse the boolean enable/disable options.
      local tOptions
      put parseBooleanOptions() into tOptions
      
      -- Run the uninstall action.
      reportPhase "Uninstall"
      actionUninstall tPath, tRecord, false
   catch tError
      quit 1
   end try
   quit 0
end runUninstallerActions

##########

function parseBooleanOptions
   local tOptions
   repeat with x = 1 to $#
      local tParam
      put value("$" & x) into tParam
      if tParam begins with "-enable-" then
         put true into tOptions[char 9 to -1 of tParam]
         log "Enabled option '" & char 9 to -1 of tParam & "'."
      else if tParam begins with "-disable-" then
         put false into tOptions[char 9 to -1 of tParam]
         log "Disabled option '" & char 9 to -1 of tParam & "'."
      end if
   end repeat
   return tOptions
end parseBooleanOptions

function parseStringOption pName 
   repeat with x = 1 to $#
      local tParam
      put value("$" & x) into tParam
      
      if tParam is ("-" & pName) then
         if x + 1 <= $# then
            local tValue
            put value("$" & (x  + 1)) into tValue
            
            -- The subsequent parameter must be of the form "...".
            if char 1 of tValue is not "-" then
               log "Option -" & pName && "has value '" & char 1 to -1 of tValue & "'."
               if char 1 of tValue is quote then
                  delete char 1 of tValue
                  delete char -1 of tValue
               end if
               return tValue
            end if
         end if
         -- If this was the last parameter, or the next parameter was an option
         -- then its an error.
         log "Option -" & pName && "specified with no value"
         reportError "Invocation error (no parameter for option)."
      end if
   end repeat
   
   return empty
end parseStringOption

function ensureStringOption pName
   local tValue
   put parseStringOption(pName) into tValue
   if tValue is empty then
      log "No -" & pName && "option specified."
      reportError "Invocation error (expected option not present)"
   end if
   return tValue
end ensureStringOption

################################################################################
#
#    BACK-END INSTALLER IMPLEMENTATION
#

-- This command performs all actions related to an install. Any errors that occur cause 'reportError'
-- to be executed which will terminate. If an error occurs during an install, cleanup will be performed
-- before exiting.
command actionInstall pPath, pRecord, pOptions
   local tManifest, tManifestItems
   
   -- Make sure the filetype is empty so extensions are use
   set the fileType to empty
   
   -- If there is a 'log' option then set the log file (but only if we are faceless)
   if sIsFaceless and pOptions["log"] is not empty then
      put pOptions["log"] into sFacelessLog
   end if
   
   -- If there is a record file, then run an uninstall first.
   if there is a file pRecord then
      reportPhase "Uninstall"
      actionUninstall pPath, pRecord, true
   end if
   
   -- We are in 'install' phase
   reportPhase "Install"
   
   -- Open the internal payload archive that was inserted at deploy time.
   log "Install: Opening payload archive."
   payloadOpen pOptions["archive"]
   
   -- Extract the manifest from the payload. The manifest will be returned to us as a standard text
   -- file (i.e. LF line endings).
   log "Install: Extracting payload manifest."
   installExtractManifest tManifest
   replace "[[installFolder]]" with pPath in tManifest
   
   -- Parse the manifest into an array of items.
   log "Install: Parsing manifest."
   installParseManifest tManifest, tManifestItems
   
   -- Calculate item sizes/progress for each manifest item
   log "Install: Instrumenting manifest with progress."
   installInstrumentManifest tManifestItems
   
   -- Make sure we have the record file ready to go
   log "Install: Creating record file at '" & pRecord & "'"
   installEnsureRecord pRecord
   
   -- We need the target installation folder before doing anything else.
   reportProgress 0, "Creating installation folder."
   installEnsureFolder pPath
   installRecord pRecord, "folder", pPath
   
   -- We use try/catch here to ensure that we do an implicit uninstall if an error
   -- occurs.
   try
      -- Now loop through all the items in the manifest performing appropriate steps in each case.
      repeat for each element tManifestItem in tManifestItems
         -- Check for cancellation and report it if present
         reportCheckCancel
         
         -- If the 'delay' option is specified, then wait for a bit (this is to help testing)
         if pOptions["delay"] then
            wait 5 millisecs
         end if
         
         -- Now perform item specific actions
         switch tManifestItem["type"]
            -- An empty type means this was a comment or empty line, so we do nothing.
            case empty
               break
            case "file"
               reportProgress tManifestItem["progress"], "Extracting file '" & tManifestItem["path"] & "'"
               payloadExtractItemToFile tManifestItem["name"], tManifestItem["base_name"], tManifestItem["path"]
               installRecord pRecord, tManifestItem["type"], tManifestItem["path"]
               break
            case "executable"
               reportProgress tManifestItem["progress"], "Extracting executable file '" & tManifestItem["path"] & "'"
               payloadExtractItemToFile tManifestItem["name"], tManifestItem["base_name"], tManifestItem["path"]
               installMakeExecutable tManifestItem["path"]
               installRecord pRecord, tManifestItem["type"], tManifestItem["path"]
               break
            case "folder"
               reportProgress tManifestItem["progress"], "Creating folder '" & tManifestItem["path"] & "'"
               installEnsureFolder tManifestItem["path"]
               installRecord pRecord, tManifestItem["type"], tManifestItem["path"]
               break
            case "uninstaller"
               reportProgress tManifestItem["progress"], "Installing uninstaller to '" & tManifestItem["path"] & "'"
               installCopyUninstaller tManifestItem["path"]
               installRecord pRecord, "uninstaller", tManifestItem["path"]
               break
            case "registry key"
               reportProgress tManifestItem["progress"], "Creating registry key '" & tManifestItem["key"] & "' with value '" & tManifestItem["value"] & "'"
               get tManifestItem["key"]
               if pOptions["allusers"] then
                  replace "HKEY_CURRENT_USER" with "HKEY_LOCAL_MACHINE" in it
               else
                  replace "HKEY_LOCAL_MACHINE" with "HKEY_CURRENT_USER" in it
               end if
               installSetRegistryKey it, tManifestItem["value"], tManifestItem["value_type"]
               installRecord pRecord, "registry key", it
               break
            case "registry value"
               reportProgress tManifestItem["progress"], "Creating registry entry '" & tManifestItem["entry"] & "' in key '" & tManifestItem["key"] & "' with value '" & tManifestItem["value"] & "'"
               get tManifestItem["key"]
               if pOptions["allusers"] then
                  replace "HKEY_CURRENT_USER" with "HKEY_LOCAL_MACHINE" in it
               else
                  replace "HKEY_LOCAL_MACHINE" with "HKEY_CURRENT_USER" in it
               end if
               installSetRegistryValue it, tManifestItem["entry"], tManifestItem["value"], tManifestItem["value_type"]
               installRecord pRecord, "registry value", it & "\" & tManifestItem["entry"]
               break
            case "shortcut"
               set the itemDelimiter to slash
               if (pOptions["desktopshortcut"] and item 1 of tManifestItem["path"] is "Desktop") or \
                     (pOptions["startmenu"] and item 1 of tManifestItem["path"] is "Programs") or \
                     item 1 of tManifestItem["path"] is "Plugins" then
                  reportProgress tManifestItem["progress"], "Creating shortcut '" & tManifestItem["path"] & "'"
                  installMakeShortcut tManifestItem["path"], tManifestItem["target"], pOptions["allusers"]
                  installRecord pRecord, "shortcut", tManifestItem["path"], pOptions["allusers"]
               end if
               set the itemDelimiter to slash
               break
            case "desktop-application"
               if pOptions["startmenu"] then
                  reportProgress tManifestItem["progress"], "Installing application menu entry for '" & tManifestItem["name"] & "'"
                  installDesktopApplication pRecord, tManifestItem["name"], tManifestItem["source"], pPath, pOptions["allusers"]
               end if
               break
            case "desktop-icon"
               if pOptions["startmenu"] then
                  reportProgress tManifestItem["progress"], "Installing icon for '" & tManifestItem["name"] & "'"
                  installDesktopIcon pRecord, tManifestItem["name"], tManifestItem["source"], pOptions["allusers"]
               end if
               break
            case "rename"
               reportProgress tManifestItem["progress"], "Moving to final destination"
               installRename pRecord, tManifestItem["source"], tManifestItem["target"]
               break
            case "record"
               -- Legacy item type - ignored for now
               break
            default
               log "Install: Invalid manifest item type '" & tManifestItem["type"] & "'"
               reportError "Internal error (illegal manifest item type found)."
               break
         end switch
      end repeat
   catch tError
      -- If an execution error occurs, then we log it and leave the installation how it is, otherwise
      -- it was a caught error and we attempt to uninstall before anything else.
      if item 1 of tError is not a number then
         reportPhase "Cleanup"
         actionUninstall pPath, pRecord, true
      else
         log "Install: Execution error - " & tError
         reportError "Internal error."
         put "internal" into tError
      end if
   end try
   
   log "Install: Closing payload archive."
   payloadClose
   
   -- If there was no error, report that we are done. Otherwise, if this was not a 'cancel' error,
   -- then throw it.
   if tError is empty or tError is "cancel" then
      reportFinished
   else
      throw tError
   end if
end actionInstall

private command installExtractManifest @rManifest
   -- Extract the item to the target variable
   payloadExtractItemToVariable "manifest.txt", rManifest
   
   -- Make sure we normalize line-endings to Rev standard LFs
   replace (numToChar(13) & numToChar(10)) with numToChar(10) in rManifest
   replace numToChar(13) with numToChar(10) in rManifest
end installExtractManifest

private command installParseManifest pManifest, @rManifestItems
   log "ParseManifest: Parsing manifest of length" && the number of chars in pManifest
   -- Manifest file uses 'tabs' as item delimiter
   set the itemDelimiter to tab
   
   -- Start off with an empty list of items
   put empty into rManifestItems
   
   -- Loop through each line, creating items as we go
   repeat for each line tLine in pManifest
      
      -- If the line is just whitepace, ignore it
      if word 1 to -1 of tLine is empty then
         next repeat
      end if
      
      -- If the first non-whitepsace char is '#', or '--' then ignore it
      if word 1 of tLine begins with "#" or \
            word 1 of tLine begins with "--" then
         next repeat
      end if
      
      -- Decode into the the  'tItem' array
      local tItem
      
      -- First parse the type portion (item 1)
      --    [ shared ] ( folder | registry | file | executable | shortcut | uninstaller )
      local tType, tShared
      put item 1 of tLine into tItem["type"]
      if word 1 of tType is "shared" then
         put word 2 of tType into tItem["type"]
         put true into tItem["shared"]
      else
         put false into tItem["shared"]
      end if
      
      -- Now decode type specific actions
      switch tItem["type"]
         case "folder"
            put item 2 of tLine into tItem["path"]
            break
         case "file"
         case "executable"
            put item 2 of tLine into tItem["path"]
            put item 3 of tLine into tItem["name"]
            put item 4 of tLine into tItem["base_name"]
            break
         case "registry key"
            put item 2 of tLine into tItem["key"]
            put item 3 of tLine into tItem["value"]
            put item 4 of tLine into tItem["value_type"]
            replace "/" with "\" in tItem["key"]
            replace "\\" with "/" in tItem["key"]
            replace "HKCU\" with "HKEY_CURRENT_USER\" in tItem["key"]
            replace "HKLM\" with "HKEY_LOCAL_MACHINE\" in tItem["key"]
            break
         case "registry value"
            get offset("//", item 2 of tLine)
            put char 1 to it -1 of  item 2 of tLine into tItem["key"]
            put char it + 2 to -1 of item 2 of tLine into tItem["entry"]
            put item 3 of tLine into tItem["value"]
            put item 4 of tLine into tItem["value_type"]
            replace "/" with "\" in tItem["key"]
            replace "\\" with "/" in tItem["key"]
            replace "HKCU\" with "HKEY_CURRENT_USER\" in tItem["key"]
            replace "HKLM\" with "HKEY_LOCAL_MACHINE\" in tItem["key"]
            break
         case "shortcut"
            put item 2 of tLine into tItem["path"]
            put item 3 of tLine into tItem["target"]
            break
         case "uninstaller"
            put item 2 of tLine into tItem["path"]
            break
         case "rename"
            put item 2 of tLine into tItem["target"]
            put item 3 of tLine into tItem["source"]
            break
         case "record"
            -- This is a no-op now, the record path is passed in by the UI
            break
         case "desktop-icon"
         case "desktop-application"
            put item 2 of tLine into tItem["name"]
            put item 3 of tLine into tItem["source"]
            break
         default
            log "ParseManifest:    invalid manifest item type '" & tItem["type"] & "'"
            reportError "Internal error (illegal manifest item type found)."
            break
      end switch
      
      -- Finally add the item to the item array
      put tItem into rManifestItems[the number of elements of rManifestItems + 1]
   end repeat
end installParseManifest

private command installInstrumentManifest @xManifest
   -- First loop through putting the running total into the progress field
   local tCurrentTotal
   put 0 into tCurrentTotal
   repeat with i = 1 to the number of elements of xManifest
      if xManifest[i]["type"] is among the items of "file,executable" then
         add item 3 of payloadDescribeItem(xManifest[i]["name"])  to tCurrentTotal
      else
         add 100 to tCurrentTotal
      end if
      put tCurrentTotal into xManifest[i]["progress"]
   end repeat
   
   -- Now normalize the progress field's to percentages
   repeat with i = 1 to the number of elements of xManifest
      divide xManifest[i]["progress"] by (tCurrentTotal / 100.0)
   end repeat
end installInstrumentManifest

private command installEnsureFolder pPath
   log "EnsureFolder: Ensure '" & pPath & "' exists."
   
   -- MM-2011-12-12: Allow Windows network drive installaitons.
   if pPath begins with "//" then
      get 4
   else
      get 2
   end if
   
   -- Otherwise make sure we create all folders up to and including
   -- our target folder.
   set the itemDelimiter to slash
   repeat with i = it to the number of items of pPath
      local tFolderPrefix, tShortFolderPrefix
      put item 1 to i of pPath into tFolderPrefix
      put installShortenPath(tFolderPrefix) into tShortFolderPrefix
      if there is a folder tShortFolderPrefix or  tShortFolderPrefix is empty then
         next repeat
      end if
      
      log "EnsureFolder:    creating folder '" & tShortFolderPrefix & "'."
      create folder tShortFolderPrefix
      if the result is not empty then
         log "EnsureFolder:    folder creation failed -" && the result
         reportError "Unable to create required folder at '" & pPath & "'."
      end if
   end repeat
end installEnsureFolder

private command installCopyUninstaller pTarget
   log "CopyUninstaller: Place uninstaller at '" & pTarget & "'"
   if the platform is "macos" then
      -- MM - copy the uninstaller app folder
      --
      set the itemDel to slash
      installCopyUninstallerFolder item 1 to - 4 of the effective filename of stack "Installer", pTarget
      if the result is not empty then
         log "CopyUninstaller:    copying failed -" && the result
         reportError "Unable to copy uninstaller to '" & pTarget & "'."
      end if
      set the itemDel to comma
   else
      put url ("binfile:" & the effective filename of stack "Installer") into url ("binfile:" & installShortenPath(pTarget))
      
      if the result is not empty then
         log "CopyUninstaller:    copying failed -" && the result
         reportError "Unable to copy uninstaller to '" & pTarget & "'."
      end if
      
      // SN-2014-10-28: [[ Bug 16410 ]] The uninstaller needs the execution permission
      // (on Linux)
      if the platform contains "linux" then
         local tChmodResult
         get shell("chmod +x " & quote & installShortenPath(pTarget) & quote)
         if it is not empty then
            log "CopyUninstaller:    setting executable permission failed - " & it
            reportError "Unable to copy uninstaller to '" & pTarget & "'."
         end if
      end if
   end if
end installCopyUninstaller

-- MM - recursive copying of a folder, making sure to set the executable bit of the installer itself
--
private command installCopyUninstallerFolder pSrcFolder, pDestFolder
   local tOldFolder
   put the folder into tOldFolder
   if not (there is a folder pDestFolder) then
      log "CopyUninstaller: creating folder" && pDestFolder
      create folder pDestFolder
      if the result is not empty then
         return pDestFolder && the result
      end if
   end if
   set the folder to pSrcFolder
   repeat for each line tFolder in the folders
      if not (tFolder begins with ".") then
         installCopyUninstallerFolder pSrcFolder & slash & tFolder, pDestFolder & slash & tFolder
         if the result is not empty then
            return pDestFolder & slash & tFolder && the result
         end if
      end if
   end repeat
   repeat for each line tFile in the files
      log "CopyUninstaller: copying file" && pDestFolder & slash & tFile
      put url ("binfile:" & pSrcFolder & slash & tFile) into url ("binfile:" & pDestFolder & slash & tFile)
      if the result is not empty then
         return pDestFolder & slash & tFile && the result
      end if
      if tFile is "installer" then
         installMakeExecutable pDestFolder & slash & tFile
      end if
   end repeat
   set the folder to tOldFolder
   return empty
end installCopyUninstallerFolder

private command installMakeExecutable pPath
   log "MakeExecutable: Ensuring file '" & pPath & "' is executable"
   
   -- Only need to set executable bit on mac and linux
   if the platform is not among the items of "macos,linux" then
      exit installMakeExecutable
   end if
   
   -- Use the 'chmod' shell command
   local tCommand, tCommandResult
   put "chmod 755 " & quote & pPath & quote into tCommand
   put shell(tCommand) into tCommandResult
   if the result is not 0 then
      log "MakeExecutable:    'chmod' failed - " & tCommandResult
      reportError "Failed to mark file as executable."
   end if
end installMakeExecutable

private command installMakeShortcut pPath, pTarget, pAllUsers
   log "MakeShortcut: Creating shortcut '" & pPath & "' to '" & pTarget & "'"
   
   set the itemDelimiter to slash
   
   -- Determine the base folder for the shortcut
   local tBaseFolder
   if item 1 of pPath is "Desktop" then
      if pAllUsers then
         put specialFolderPath(0x0019) into tBaseFolder
      else
         put specialFolderPath("desktop") into tBaseFolder
      end if
   else if item 1 of pPath is "Programs" then
      if pAllUsers then
         put specialFolderPath(0x0017) into tBaseFolder
      else
         put specialFolderPath(0x0002) into tBaseFolder
      end if
   else if item 1 of pPath is "Plugins" then
      if the platform is "linux" then
         put $HOME & slash & ".mozilla/plugins" into tBaseFolder
      else if the platform is "macos" then
         if pAllUsers then
            put "/Library/Internet Plug-Ins" into tBaseFolder
         else
            put specialFolderPath("cusr") & slash & "Library" & slash & "Internet Plug-Ins" into tBaseFolder
         end if
      end if
   end if
   
   -- If the basefolder was not found, then this type of shortcut is
   -- not supported on this platform.
   if tBaseFolder is empty then
      exit installMakeShortcut
   end if
   
   -- Remove the first item as it just indicates the shortcut type
   delete item 1 of pPath
   
   -- If there is no target, this is just a folder creation
   if pTarget is empty then
      installEnsureFolder tBaseFolder & slash & pPath
      exit installMakeShortcut
   end if
   
   -- Make sure the appropriate folder structure is present
   if the number of items of pPath > 1 then
      installEnsureFolder tBaseFolder & slash & (item 1 to -2  of pPath)
   end if
   
   -- Create the shortcut
   if the platform is "win32" then
      put ".lnk" after pPath
   end if
   if the platform is not "macos" then
      create alias tBaseFolder & slash & pPath to file pTarget
      if the result is not empty then
         log "MakeShortcut:    'create alias' failed -" && the result
         reportError "Failed to create " & item 1 of pPath & " shortcut."
      end if
   else
      get shell("ln -s" && quote & pTarget & quote && quote & tBaseFolder & slash & pPath & quote)
      if it is not empty then
         log "MakeShortcut:    'ln' failed -" && it
         reportError "Failed to create " & item 1 of pPAth & " shortcut."
      end if
   end if
end installMakeShortcut

private command installDesktopApplication pRecord, pName, pSource, pInstallPath, pAllUsers
   local tTargetPath
   put xdgDataPath(pAllUsers) & slash & "applications" into tTargetPath
   
   -- Make sure the target path exists
   installEnsureFolder tTargetPath
   
   local tData
   payloadExtractItemToVariable pSource, tData
   replace "[[installFolder]]" with pInstallPath in tData
   put tData into url ("binfile:" & tTargetPath & slash & pName & ".desktop")
   get shell("touch" && tTargetPath)
   
   installRecord pRecord, "file", tTargetPath & slash & pName & ".desktop"
end installDesktopApplication

private command installDesktopIcon pRecord, pName, pSource, pAllUsers
   local tTargetPath
   put xdgDataPath(pAllUsers) & slash & "icons/hicolor/48x48/apps" into tTargetPath
   
   -- Make sure the target path exists
   installEnsureFolder tTargetPath
   
   payloadExtractItemToFile pSource, empty, tTargetPath & slash & pName & ".png"
   get shell("touch" && tTargetPath)
   
   -- Regenerate the icon cache
   -- Errors are ignored as the utility may not be present
   get shell("gtk-update-icon-cache" && xdgDataPath(pAllUsers) & "/icons/hicolor")
   
   installRecord pRecord, "file", tTargetPath & slash & pName & ".png"
end installDesktopIcon

private command installEnsureRecord pRecord
   log "EnsureRecord: Ensuring new record exists at '" & pRecord & "'"
   -- Make sure the target folder exists, note we can't use 'ensurefolder'
   -- as this requires a writable record.
   set the itemDelimiter to slash
   installEnsureFolder item 1 to -2 of pRecord
   set the itemDelimiter to comma
   
   -- Now ensure the file is created and is empty
   put empty into url ("file:" & installShortenPath(pRecord))
   if the result is not empty then
      log "EnsureRecord:    failed to create record - " & the result
      reportError "Internal error (couldn't create install record)."
   end if
end installEnsureRecord

private command installSetRegistryKey pKey, pValue, pType
   log "SetRegistryKey: Setting key '" & pKey & "' to '" & pValue & "' of type" && pType
   
   if pType is "number" then
      put "dword" into pType
      put binaryEncode("i", pValue) into pValue
   else if pType is "path" then
      replace "/" with "\" in pValue
   end if
   
   set the itemDelimiter to "\"
   installEnsureRegistryKey item 1 to -2 of pKey
   
   get setRegistry(pKey & "\", pValue, pType)
   if the result is not empty and (the result is not "can't delete value" or pValue is not empty) then
      log "SetRegistryKey:    failed to set key - " & the result
      reportError "Unable to set registry key '" & pKey & "' to required value."
   end if
end installSetRegistryKey

private command installSetRegistryValue pKey, pEntry, pValue, pType
   log "SetRegistryValue: Setting value '" & pEntry & "' of key '" & pKey & "' to '" & pValue & "' of type" && pType
   
   if pType is "number" then
      put "dword" into pType
      put binaryEncode("i", pValue) into pValue
   else if pType is "path" then
      replace "/" with "\" in pValue
   end if
   
   installEnsureRegistryKey pKey
   
   get setRegistry(pKey & "\" & pEntry, pValue, pType)
   if the result is not empty then
      log "SetRegistryValue:   failed to set value of key -" && the result
      reportError "Unable to set registry value '" & pValue & "' of key '" & pKey & "' to required setting."
   end if
end installSetRegistryValue

private command installEnsureRegistryKey pKey
   log "EnsureRegistryKey: Ensure registry key '" & pKey & "' exists"
   set the itemDelimiter to "\"
   repeat with x = 2 to the number of items of pKey
      local tKeyPrefix
      put item 1 to x of pKey into tKeyPrefix
      get queryRegistry(tKeyPrefix)
      if the result is not "can't find key" then
         next repeat
      end if
      
      log "EnsureRegistryKey:    creating registry key '" & tKeyPrefix & "'"
      get setRegistry(tKeyPrefix & "\", empty)
      if the result is not empty then
         log "EnsureRegistryKey:    registry key creation failed -" && the result
         reportError "Unable to create required registry key at '" & pKey & "'."
      end if
   end repeat
end installEnsureRegistryKey

private command installRename pRecord, pSource, pTarget
   log "Rename: Renaming '" & pSource & "' to '" & pTarget & "'"
   rename installShortenPath(pSource) to installShortenPath(pTarget)
   if the result is not empty then
      log "Rename:    failed -" && the result
      reportError "Unable to move components to final location."
   end if
   installRecord pRecord, "rename", pTarget, pSource
end installRename

private command installRecord pRecord, pType, pPath, pPath2
   -- Shorten the path first
   local tShortRecord
   put installShortenPath(pRecord) into tShortRecord
   
   -- Now enter the item in the record - note we open and close the file each entry. This ensures
   -- that each action is written atomically, in case the installer crashes or suffers some other fate.
   log "Record: Recording item of type '" & pType & "' with path '" & pPath & "'"
   open file tShortRecord for text append
   if the result is not empty then
      log "Record:    failed to open record - " && the result
      reportError "Internal error (couldn't update install record)."
   end if
   write pType & tab & pPath & tab & pPath2 & return to file tShortRecord
   if the result is not empty then
      log "Record:    failed to write to record - " && the result
      reportError "Internal error (couldn't write to install record)."
   end if
   close file tShortRecord
end installRecord

-- This method uses 'shortFilePath' to reduce the length of the given path to the shortened
-- 8.3 format -- if on Windows. This allows the installer to handle much longer paths than would
-- otherwise be possible.
private function installShortenPath pLongPath
   if the platform is not "win32" then
      return pLongPath
   end if
   
   local tShortPath
   set the itemDelimiter to "/"
   put item 1 of pLongPath into tShortPath
   repeat with i = 2 to the number of items in pLongPath
      get item -1 of shortFilePath(tShortPath & slash & item i of pLongPath)
      if the result is not empty then
         put slash & item i to -1 of pLongPath after tShortPath
         exit repeat
      end if
      put slash & item -1 of it after tShortPath
   end repeat
   
   return tShortPath
end installShortenPath

-- This function returns the 'XDG' data path for this or all users.
-- Note that, at this time, it does not use the XDG_DATA_DIRS var to work out where to
-- put 'all users' files. The reason here is that the default on Ubuntu seems wrong to me for
-- an application that can work in both KDE and Gnome. It is my feeling that the correct
-- place for us to use is /usr/share, since we install into /opt.
private function xdgDataPath pAllUsers
   if pAllUsers then
      return "/usr/share"
   end if
   
   if $XDG_DATA_HOME is empty then
      return $HOME & slash & ".local/share"
   end if
   
   return $XDG_DATA_HOME
end xdgDataPath

################################################################################
#
#    BACK-END UNINSTALLER IMPLEMENTATION
#

-- Uninstall things at <path>, using record <record>. If <preinstall> is true, then the uninstall
-- is part of an install and so doesn't do the self-deletion trick.
command actionUninstall pPath, pRecord, pPreInstall
   -- Load the record file
   local tRecordFile
   log "Uninstall: Loading record file '" & pRecord & "'"
   put url ("file:" & installShortenPath(pRecord)) into tRecordFile
   if the result is not empty then
      log "Uninstall:    failed -" && the result
      reportError "Internal error (couldn't read install record)"
   end if
   
   -- Now split the record item by line so we can loop through in reverse order
   local tRecordItems, tTotalCount
   put tRecordFile into tRecordItems
   split tRecordItems by return
   put the number of elements of tRecordItems into tTotalCount
   
   -- Now do the looping, keeping note of the uninstaller record which we do last
   local tUninstaller, tNonEmptyFolders, tCurrentCount
   put 0 into tCurrentCount
   repeat with x = the number of elements of tRecordItems down to 1
      local tType, tParam, tParam2, tShortParam
      set the itemDelimiter to tab
      put item 1 of tRecordItems[x] into tType
      put item 2 of tRecordItems[x] into tParam
      put item 3 of tRecordItems[x] into tParam2
      set the itemDelimiter to comma
      
      local tProgress
      add 1 to tCurrentCount
      put tCurrentCount / tTotalCount * 100.0 into tProgress
      switch tType
         case "file"
         case "executable"
            put installShortenPath(tParam) into tShortParam
            reportProgress tProgress, "Removing file at '" & tParam & "'"
            if there is a file tShortParam then
               delete file tShortParam
               if the result is not empty then
                  log "Uninstall: Removal of file '" & tParam & "' failed -" && the result
               end if
            else
               log "Uninstall: File '" & tParam & "' not found, so ignoring"
            end if
            break
         case "folder"
            put installShortenPath(tParam) into tShortParam
            reportProgress tProgress, "Removing folder at '" & tParam & "'"
            if there is a folder tShortParam then
               if uninstallCanRemoveFolder(tParam) then
                  delete folder tShortParam
                  if the result is not empty then
                     log "Uninstall: Removal of folder '" & tParam & "' failed -" && the result
                  end if
               else
                  log "Uninstall: Folder '" & tParam & "' not empty, so can't remove"
                  put tParam & return after tNonEmptyFolders
               end if
            else
               log "Uninstall: Folder '" & tParam & "' not found, so ignoring"
            end if
            break
         case "shortcut"
            reportProgress tProgress, "Removing shortcut '" & tParam & "'"
            
            local tShortcut
            set the itemDelimiter to slash
            if item 1 of tParam is "Desktop" then
               if tParam2 then
                  put specialFolderPath(0x0019) & slash & item 2 to -1 of tParam into tShortcut
               else
                  put specialFolderPath("Desktop") & slash & item 2 to -1 of tParam into tShortcut
               end if
            else if item 1 of tParam is "Programs" then
               if tParam2 then
                  put specialFolderPath(0x0017) & slash & item 2 to -1 of tParam into tShortcut
               else
                  put specialFolderPath(0x0002) & slash & item 2 to -1 of tParam into tShortcut
               end if
            else if item 1 of tParam is "Plugins" then
               if the platform is "linux" then
                  put $HOME & slash & ".mozilla/plugins" & slash & item 2 to -1 of tParam into tShortcut
               else if the platform is "macos" then
                  if tParam2 then
                     put "/Library/Internet Plug-Ins" & slash & item 2 to -1 of tParam into tShortcut
                  else
                     put specialFolderPath("cusr") & slash & "Library" & slash & "Internet Plug-Ins" & slash & item 2 to -1 of tParam into tShortcut
                  end if
               end if
               log "Plugin shortcut to remove" && tShortcut
            end if
            set the itemDelimiter to comma
            if the platform is "Win32" and there is a folder tShortcut then
               if uninstallCanRemoveFolder(tShortcut) then
                  delete folder tShortcut
                  if the result is not empty then
                     log "Uninstall: Removal of shortcut folder '" & tShortcut & "' failed -" && the result
                  end if
               else
                  log "Uninstall: Shortcut folder '" & tShortcut & "' not empty, so can't remove"
               end if
            else
               if the platform is "win32" then
                  put ".lnk" after tShortcut
               end if
               if the platform is "linux" or the platform is "macos" then
                  get shell("rm" && quote & tShortcut & quote)
                  log it && the result
               else
                  delete file tShortcut
               end if
               if the result is not empty then
                  log "Uninstall: Removal of file '" & tShortcut & "' failed -" && the result
               end if
            end if
            break
         case "rename"
            reportProgress tProgress, "Renaming item at '" & tParam & "' to '" & tParam2 & "'"
            rename installShortenPath(tParam) to installShortenPath(tParam2)
            if the result is not empty then
               log "Uninstall: Rename of item '" & tParam & "' failed -" && the result
            end if
         case "registry key"
            reportProgress tProgress, "Removing registry key at '" & tParam & "'"
            get queryRegistry(tParam & "\")
            if the result is empty then
               if uninstallCanRemoveRegistryKey(tParam) then
                  get deleteRegistry(tParam)
                  if the result is not empty then
                     log "Uninstall: Removal of registry key '" & tParam & "' failed -" && the result
                  end if
               else
                  log "Uninstall: Registry key '" & tParam & "' not empty, so can't remove"
               end if
            end if
            break
         case "registry value"
            reportProgress tProgress, "Removing registry value '" & tParam & "'"
            get queryRegistry(tParam)
            if the result is empty then
               get setRegistry(tParam, empty)
               if the result is not empty then
                  log "Uninstall: Removal of registry value '" & tParam & "' failed -" && the result
               end if
            end if
            break
         case "uninstaller"
            reportProgress tProgress, "Removing uninstaller at '" & tParam & "'"
            delete file installShortenPath(tParam)
            if there is a file installShortenPath(tParam) then
               put tParam into tUninstaller
            end if
            break
      end switch
   end repeat
   
   -- Finally we remove the uninstaller (if any) and the record...
   log "Uninstall: Removing installer record at '" & pRecord & "'"
   delete file installShortenPath(pRecord)
   
   -- And try again to remove any outstanding folders
   repeat for each line tFolder in tNonEmptyFolders
      if uninstallCanRemoveFolder(tFolder) then
         log "Uninstall: Attempting to remove outstanding folder '" & tFolder & "'"
         delete folder installShortenPath(tFolder)
         if the result is not empty then
            log "Uninstall:    removal failed -" && the result
         end if
      end if
   end repeat
   
   -- If we are on Windows, then launch a special self-deleting batch file
   -- to eliminate the remaining things. This batch file simply sits and spins
   -- trying to delete the uninstaller file every 5 seconds. It quits when
   -- it succeeds in doing so.
   if the platform is "win32" and tUninstaller is not empty and not pPreInstall then
      local tBatchFile, tBatchFilePath, tBatchCommand
      put tempName() & ".bat" into tBatchFilePath
      put "@ECHO OFF" & return after tBatchFile
      put "CD %windir%" & return after tBatchFile
      put ":Repeat" & return after tBatchFile
      put "CHOICE /N /C AB /D A /T 1 >NUL:" & return after tBatchFile
      get installShortenPath(tUninstaller)
      replace slash with "\" in it
      put "DEL" && quote & it & quote && "2>NUL:" & return after tBatchFile
      put "IF EXIST" && quote & it & quote && "goto Repeat" & return after tBatchFile
      repeat for each line tFolder in tNonEmptyFolders
         put "RMDIR" && quote & tFolder & quote && "2>NUL:" & return after tBatchFile
      end repeat
      get installShortenPath(tBatchFilePath)
      replace slash with "\" in it
      put "DEL" && quote & it & quote && "2>NUL:" && "&& EXIT" & return after tBatchFile
      put tBatchFile into url ("file:" & tBatchFilePath)
      
      put "cmd.exe /Q /C " & quote & "start /MIN" && it & quote into tBatchCommand
      
      log "Uninstall: Running self-deletion batch file at '" & tBatchFilePath & "'"
      set the hideConsoleWindows to true
      open process tBatchCommand for neither
      if the result is empty then
         wait 10 millisecs with messages
         close process tBatchCommand
      else
         log "Uninstall:    execution failed -" && the result
      end if
   end if
   
   -- Remove any docs cache for this version
   uninstallRemoveDocsCache 
   
   -- Notify the master that uninstalling is done (but only if this is not a pre-install uninstall)
   if not pPreInstall then
      reportFinished true
   end if
end actionUninstall

private command uninstallSetWindowsShellCommand
   set the hideConsoleWindows to true
   if $COMSPEC is not empty then
      set the shellCommand to $COMSPEC
   else
      --just in case $COMSPEC is not set
      set the shellCommand to "cmd.exe"
   end if
end uninstallSetWindowsShellCommand

private function uninstallWindowsFromUnixPath pUnixPath
   replace "/" with "\" in pUnixPath
   return pUnixPath
end uninstallWindowsFromUnixPath

private command uninstallDeleteFolder pSrcFolder
   if the platform is "Win32" then
      uninstallSetWindowsShellCommand
      get shell ("rmdir /s /q" && uninstallWindowsFromUnixPath(quote&pSrcFolder&quote))
   else
      get shell ("rm -rf" && quote&pSrcFolder&quote)
   end if
   return the result
end uninstallDeleteFolder

private command uninstallRemoveDocsCache
   local tDocsCache
   switch the platform
      case "Linux"
         put "~/.runrev/documentationcache" into tDocsCache
         break
      case "MacOS"
         put specialFolderPath("cusr") & "/Library/Application Support/RunRev/Documentation Cache" into tDocsCache
         break
      case "Win32"
         put specialFolderPath(0x001c) & "/RunRev/Documentation Cache" into tDocsCache
         break
   end switch
   
   local tEdition
   put tolower(getInstallerType()) into tEdition
   
   # The docs cache is per-version and edition
   local tVersion
   put the version into tVersion
   put replaceText(tVersion, "[-,\.]", "_") into tVersion
   put slash & tVersion & "_" & tEdition  after tDocsCache
   
   log "Uninstall: removing docs cache at" && tDocsCache
   
   if there is a folder tDocsCache then
      uninstallDeleteFolder tDocsCache
      if the result is not empty then
         log "Uninstall: failed to remove docs cache"
      end if
   end if
end uninstallRemoveDocsCache

private function uninstallCanRemoveFolder pPath
   local tOldFolder, tCanRemove
   put the folder into tOldFolder
   set the folder to installShortenPath(pPath)
   put false into tCanRemove
   if the files is empty then
      if the folders is ".." then
         put true into tCanRemove
      end if
   end if
   set the folder to tOldFolder
   return tCanRemove
end uninstallCanRemoveFolder

private function uninstallCanRemoveRegistryKey pKey
   return listRegistry(pKey) is empty
end uninstallCanRemoveRegistryKey

################################################################################
#
#    PAYLOAD ACCESS FUNCTIONS
#

local sPayloadArchive

private command payloadOpen pArchive
   if isInstallerEnvironment() then
      if the platform is "MacOS" then
         local tPayload
         set the itemDel to slash
         put item 1 to -3 of the filename of stack "Installer" & slash & "Resources/payload" into tPayload
         set the itemDel to comma
         do "_internal payload open tPayload"
      else
         do "_internal payload open"
      end if
   else
      put pArchive into sPayloadArchive
      revZipOpenArchive pArchive, "read"
   end if
   if the result is not empty then
      log "Opening payload failed -" && the result
      reportError "Internal error (payload archive wouldn't open)."
   end if
end payloadOpen

private command payloadClose
   if isInstallerEnvironment() then
      do "_internal payload close"
   else
      revZipCloseArchive sPayloadArchive
      put empty into sPayloadArchive
   end if
   return empty
end payloadClose

private function payloadDescribeItem pItem
   local it
   if isInstallerEnvironment() then
      do "_internal payload describe pItem"
      if the result is not empty then
         get the result
      end if
   else
      get revZipDescribeItem(sPayloadArchive, pItem)
   end if
   if item 3 of it is not a number then
      log "Describing item '" & pItem & "' failed -" && it
      reportError "Internal error (couldn't get payload item description)."
   end if
   return it
end payloadDescribeItem

private command payloadExtractItemToVariable pItem, @rData
   if isInstallerEnvironment() then
      do "_internal payload extract pItem; put it into rData"
   else
      revZipExtractItemToVariable sPayloadArchive, pItem, "rData"
   end if
   if the result is not empty then
      log "Payload: Extracting to variable failed for '" & pItem & "' - " & the result
      reportError "Internal error (couldn't extract payload item)."
   end if
   return the result
end payloadExtractItemToVariable

private command payloadExtractItemToFile pItem, pBaseItem, pFile
   local tShortFile
   put installShortenPath(pFile) into tShortFile
   if isInstallerEnvironment() then
      if pBaseItem is empty then
         log "Payload: Extracting item '" & pItem & "' to file '" & pFile & "'"
         do "_internal payload extract pItem to tShortFile"
      else
         log "Payload: Extracting item '" & pItem & "' based on item '" & pBaseItem & "' to file '" & pFile & "'"
         do "_internal payload patch pItem to pBaseItem into tShortFile"
      end if
   else
      revZipExtractItemToFile sPayloadArchive, pItem, tShortFile
   end if
   if the result is not empty then
      log "Payload: Extracting to file '" & pFile & "' failed for item '" & pItem & "' - " & the result
      reportError "Internal error (couldn't extract payload item to file)."
   end if
   return the result
end payloadExtractItemToFile

################################################################################
#
#    MASTER/SLAVE COMMUNICATION FUNCTIONS
#

private command reportCheckCancel
   if sIsFaceless then
      exit reportCheckCancel
   end if
   
   if isInstallerEnvironment() then
      -- Attempt to read 1 line from the parent
      read from stdin for 1 line in 0 millisecs
      
      switch the result
         -- If there was no error and the line reads 'cancel' we throw a cancellation msg
         case empty
            get line 1 of it
            if it is "cancel" then
               throw "cancel"
            end if
            break
            
            -- If there was a 'timeout' error then we carry on as normal
         case "timed out"
            break
            
            -- Otherwise, some other error means communication with the master has gone, so we throw
            -- an error (i.e. eof)
         default
            log "reportCheckCancel: eof when reading from master"
            throw "error"
      end switch
   end if
end reportCheckCancel

private command reportPhase pPhase
   if not sIsFaceless then
      if isInstallerEnvironment() then
         write "phase" & tab & pPhase & return to stdout
      else
         log "Phase Changed to -" && pPhase
      end if
   else
      write "Entering phase -" && pPhase & return to stdout
   end if
end reportPhase

private command reportProgress pProgress, pMessage
   if not sIsFaceless then
      if isInstallerEnvironment() then
         replace "\" with "\\" in pMessage
         replace return with "\n" in pMessage
         write "progress" & tab & pProgress & tab & pMessage & return to stdout
      else
         log pMessage
      end if
   end if
end reportProgress

private command reportFinished pIsUninstall
   if not sIsFaceless then
      if isInstallerEnvironment() then
         log "Done"
         write "done" & return to stdout
      end if
   else
      if pIsUninstall then
         write "Successfully uninstalled." & return to stdout
      else
         write "Installation succeeded." & return to stdout
      end if
   end if
end reportFinished

private command reportError pMessage
   if not sIsFaceless then
      if isInstallerEnvironment() then
         replace "\" with "\\" in pMessage
         replace return with "\n" in pMessage
         write "error" & tab & pMessage & return to stdout
         throw "error"
      else
         put "ERROR" & tab & pMessage & return after field "Report" of me
         throw "oh dear - there's been an error!"
      end if
   else
      write "Installation failed -" && pMessage & return to stdout
      throw "error"
   end if
end reportError

################################################################################
#
#    LOGGING FUNCTIONS
#

command log pMessage
   if not sIsFaceless then
      local tFile
      switch the platform
         case "linux"
            if $EUID is zero then
               put "~root/.runrev/logs" into tFile
            else
               put $HOME & "/.runrev/logs" into tFile
            end if
            put "/" & toLower(the uProduct of stack "Installer") & "_installer_log.txt" after tFile
            break
         case "macos"
            put specialFolderPath("cusr") & "/Library/Logs/RunRev/" & the uProduct of stack "Installer" & "InstallerLog.txt"  into tFile
            break
         case "win32"
            -- Make sure we put logs in the 'Local' AppData section.
            put specialFolderPath(0x001c) & "/RunRev/Logs/" & the uProduct of stack "Installer" & "InstallerLog.txt" into tFile
            break
      end switch
      
      if there is no file tFile then
         set the itemDelimiter to slash
         repeat with x = 1 to the number of items of tFile - 1
            if there is no folder (item 1 to x of tFile) then
               create folder (item 1 to x of tFile)
            end if
         end repeat
         set the itemDelimiter to comma
      end if
      
      open file tFile for text append
      write "[" && the internet date && "]" && ":" && pMessage & return to file tFile
      close file tFile
   else if sFacelessLog is not empty then
      open file sFacelessLog for text append
      if the result is empty then
         write "[" && the internet date && "]" && ":" && pMessage & return to file sFacelessLog
         close file sFacelessLog
      end if
   else
      write "[" && the internet date && "]" && ":" && pMessage & return to stdout
   end if
end log

////////////////////////////////////////////////////////////////////////////////

// Utility functions removed from the "Tools Installer" stack.

function resolveImageId pImage
   return the id of image (pImage & ".png") of stack "Resources"
end resolveImageId

function resolveImageLongId pImage
   return the long id of image (pImage & ".png") of stack "Resources"
end resolveImageLongId

function getInstallerType
   return the cInstallerType of card "Resources" of stack "Resources"
end getInstallerType

command configureFonts pStack
   switch the platform
      case "win32"
         if word 2 of the systemVersion >= 6  and "Segoe UI" is among the lines of the fontNames then
            set the textFont of stack pStack to "Segoe UI"
            set the textSize of stack pStack to 12
         else if word 2 of the systemVersion >= 5 and "Tahoma" is among the lines of the fontNames then
            set the textFont of stack pStack to "Tahoma"
            set the textSize of stack pStack to 11
         else
            set the textFont of stack pStack to "MS Sans Serif"
            set the textSize of stack pStack to 10
         end if
         break
      case "linux"
         set the textFont of stack pStack to "Helvetica"
         set the textSize of stack pStack to 12
         break
      case "macos"
         set the textFont of stack pStack to "Lucida Grande"
         set the textSize of stack pStack to 11
         break
   end switch
end configureFonts

command setTitleField pField, pTitle, pSubTitle
   if pSubTitle is empty then
      set the text of pField to pTitle
      set the textStyle of word 1 to -1 of pField to "bold"
      set the textColor of word 1 to -1 of pField to "#444444"
   else
      set the text of pField to pSubTitle && "//" && pTitle
      set the textColor of word 1 to -1 of pField to "#555555"
      set the textStyle of word (-the number of words of pTitle) to -1 of pField to "bold"
      set the textColor of word (-the number of words of pTitle) to -1 of pField to "#444444"
   end if
end setTitleField
