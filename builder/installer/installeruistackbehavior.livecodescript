script "InstallerInstallerUIStackBehavior"
######################################################################

on relaunch
   return "background"
end relaunch

on preOpenStack
   // PM-2016-02-27: [[ Bug 13723]] Use a card border 
   if the platform is "win32" or the platform contains "linux" then
      local tCards, tCurrentCard, tControlsA
      
      put the cardNames of stack "InstallerUI" of stack "Installer" into tCards
      repeat with tCardIndex = 1 to the number of lines in tCards
         put line tCardIndex of tCards into tCurrentCard
         set the showBorder of card tCurrentCard of stack "InstallerUI" of stack "Installer" to true
      end repeat
   end if
   
   -- Sort out fonts and sizes
   configureFonts the short name of me
   
   -- Make sure there's no error state
   set the uError of card "Finish" to empty
   
   if $2 is "-update" then
      perfromUpdateCheck
   else if $2 is "-foregroundupdate" then
      perfromForegroundUpdateCheck
   else
      performNormalInstall
   end if
end preOpenStack

-- Default handling for next/back/cancel buttons
on actionNext
   go next card
end actionNext

on actionBack
   go prev card
end actionBack

on actionCancel
   quitInstaller
end actionCancel

command quitInstaller
   if the environment is "installer" then
      set the itemDel to "."
      if the platform is "MacOS" and item 1 to 2 of the systemVersion < 10.7 and $3 is "-unmount" then
         set the itemDel to slash
         if item 2 of the effective filename of this stack is "Volumes" then
            --get shell ("diskutil unmount force" && quote &  item 1 to 3 of the effective filename of this stack & quote)
         end if
      end if
      lock messages
      quit
   else
      resetAll
      exit to top
   end if
end quitInstaller

######################################################################

command performNormalInstall
   if $2 is not empty then
      set the loc of this stack to $2
   else
      set the loc of this stack to the screenLoc
   end if
   set the visible of this stack to true
   set the processType to "foreground"
   go card "Welcome"
end performNormalInstall

command perfromUpdateCheck
   set the uMode of card "UpdateCheck" to "background"
   set the loc of this stack to the screenloc
   go card "UpdateCheck"
end perfromUpdateCheck

command perfromForegroundUpdateCheck 
   set the uMode of card "UpdateCheck" to "foreground"
   set the loc of this stack to the screenloc
   go card "UpdateCheck"
end perfromForegroundUpdateCheck

######################################################################

local sMovingOffset

on mouseDown
   put the clickLoc into sMovingOffset
end mouseDown

on mouseUp
   put empty into sMovingOffset
end mouseUp

on mouseRelease
   put empty into sMovingOffset
end mouseRelease

on mouseMove
   if sMovingOffset is not empty then
      get the globalLoc of the mouseLoc
      set the topLeft of me to item 1 of it - item 1 of sMovingOffset, item 2 of it - item 2 of sMovingOffset
   end if
end mouseMove

######################################################################

local sInstallTypeSetting
local sCustomTargetSetting
local sCustomLocationSetting
local sCustomDesktopShortcutSetting
local sCustomStartMenuShortcutsSetting

function settingsInstallType
   if sInstallTypeSetting is empty then
      return "all users"
   end if
   return sInstallTypeSetting
end settingsInstallType

command setInstallTypeSetting pInstallType
   put pInstallType into sInstallTypeSetting
end setInstallTypeSetting

function settingsTarget
   if sInstallTypeSetting is not "custom" then
      return settingsInstallType()
   end if
   
   if sCustomTargetSetting is empty then
      put "all users" into sCustomTargetSetting
   end if
   
   return sCustomTargetSetting
end settingsTarget

function settingsLocation
   if settingsInstallType() is not "custom" then
      return defaultLocationSetting()
   end if
   
   if sCustomLocationSetting is empty then
      put defaultLocationSetting() into sCustomLocationSetting
   end if
   
   return sCustomLocationSetting
end settingsLocation

function settingsDesktopShortcut
   if  settingsInstallType() is not "custom" then
      return true
   end if
   
   if sCustomDesktopShortcutSetting is empty then
      if the platform is "win32" then
         put true into sCustomDesktopShortcutSetting
      else
         put false into sCustomDesktopShortcutSetting
      end if
   end if
   
   return sCustomDesktopShortcutSetting
end settingsDesktopShortcut

function settingsStartMenuShortcuts
   if  settingsInstallType() is not "custom" then
      return true
   end if
   
   if sCustomStartMenuShortcutsSetting is empty then
      if the platform is not "macos" then
         put true into sCustomStartMenuShortcutsSetting
      else
         put false into sCustomStartMenuShortcutsSetting
      end if
   end if
   
   return sCustomStartMenuShortcutsSetting
end settingsStartMenuShortcuts

function defaultLocationSetting
   if settingsTarget() is "all users" then
      return defaultAllUserLocation()
   else
      return defaultThisUserLocation()
   end if
   return "<unknown platform>"
end defaultLocationSetting

function defaultAllUserLocation
   switch the platform
      case "win32"
         return specialFolderPath("0x0026") & slash & "RunRev/" & thisProductFolder()
      case "linux"
         return "/opt/livecode/" & thisProductFolder()
      case "macos"
         return specialFolderPath("apps") & slash & thisProductFolder()
   end switch
   return "<unknown platform>"
end defaultAllUserLocation

function defaultThisUserLocation
   switch the platform
      case "win32"
         return specialFolderPath("0x001a") & slash & "RunRev/Components/" & thisProductFolder()
      case "linux"
         return $HOME & slash & ".runrev/components/" & thisProductFolder()
      case "macos"
         return specialFolderPath("home") & slash & "Applications" & slash & thisProductFolder()
   end switch
   return "<unknown platform>"
end defaultThisUserLocation

command setCustomTargetSetting pNewSetting
   put pNewSetting into sCustomTargetSetting
end setCustomTargetSetting

command setCustomLocationSetting pNewSetting
   put pNewSetting into sCustomLocationSetting
end setCustomLocationSetting

command setCustomStartMenuShortcutsSetting pNewSetting
   put pNewSetting into sCustomStartMenuShortcutsSetting
end setCustomStartMenuShortcutsSetting

command setCustomDesktopShortcutSetting pNewSetting
   put pNewSetting into sCustomDesktopShortcutSetting
end setCustomDesktopShortcutSetting

function thisProductFolder
   switch the platform
      case "win32"
         return the uProductTitle of stack "Installer"
      case "linux"
         local tFolder
         put the uProductTag of stack "Installer" into tFolder
         put "." & the processor after tFolder
         return tFolder
      case "macos"
         return  the uProductTitle of stack "Installer" & ".app"
   end switch
   return "<unknown platform>"
end thisProductFolder

function getUpdateFolderLocation
   switch the platform
      case "MacOS"
         return specialFolderPath("home") & "/Library/Application Support/RunRev/Updates/LiveCode"
      case "Win32"
         return specialFolderPath(0x001c) & "/RunRev/Updates/LiveCode"
      case "Linux"
         return specialFolderPath("home") & "/.runrev/updates/livecode"
   end switch
end getUpdateFolderLocation

######################################################################

-- This function returns 'true' if the current settings will require admin privileges.
-- Originally this was done just by divining 'accessibility' of the location property, but
-- in actuality it is a function of platform and install type (this user/all users).
function settingsRequireAdminPrivileges
   
   -- On Linux we require admin to install 'all user' start menu shortcuts, regardless
   -- of target location.
   if the platform is "linux" then
      if settingsTarget() is "all users" and settingsStartMenuShortcuts() then
         return true
      end if
   end if
   
   -- On Windows, the same is true
   if the platform is "win32" then
      if settingsTarget() is "all users" and (settingsStartMenuShortcuts() or settingsDesktopShortcut()) then
         return true
      end if
   end if
   
   -- As there seems a rather unpleasant bug in 'open elevated process' on Mac OS X, we
   -- don't try and elevate there!
   --if the platform is "macos" then
   --return true
   --end if
   
   -- If we are not 'all users' or do not install any shortcuts *then* admin requirement is
   -- based on target location.
   return requiresAdminPrivileges(settingsLocation())
end settingsRequireAdminPrivileges

######################################################################

-- A target folder is suitable for installation if it doesn't contain a sub-folder which
-- we want to use. Or, that sub-folder is a rev install.
function locationIsSuitable pLocation
   if there is no folder pLocation then
      return true
   end if
   
   if locationIsExistingInstall(pLocation) then
      return true
   end if
   
   return false
end locationIsSuitable

-- On linux and windows, a target folder is an existing install if it contains '.setup.txt',
-- '.setup.exe' and a rev engine. On macosx, if it contains a rev app bundle of the same
-- name.
function locationIsExistingInstall pLocation
   if there is no folder pLocation then
      return false
   end if
   
   if the platform is not "macos" then
      if there is no file (pLocation & "/.setup.txt") or \
            (there is no file (pLocation & "/.setup.exe") and \
            there is no file (pLocation & "/.setup.x86") and \
            there is no file (pLocation & "/setup.x86") and \
            there is no file (pLocation & "/setup.x86_64")) then
         return false
      end if
      
      local tFiles, tOldFolder
      put the defaultfolder into tOldFolder
      set the defaultFolder to pLocation
      put the files into tFiles
      set the defaultFolder to tOldFolder
      
      repeat for each line tFile in tFiles
         // Case sensitive
         if matchText(tFile, "LiveCode.*\.(exe|x86|x86_64)") or \
               matchText(tFile, "livecode.*\.(exe|x86|x86_64)") then
            return true
         end if
      end repeat
      
      return false
   else
      if there is no folder pLocation then
         return false
      end if
   end if
   
   return true
end locationIsExistingInstall

-- An install requires admin privileges if it needs them to:
--   a) if the location doesn't exist then to create it
--   b) if the location does exist, then to create a file inside it
--   c) if the location is an existing install, then to delete all files in it
--
function requiresAdminPrivileges pLocation
   if there is no folder pLocation then
      -- Find the first folder that does exist
      set the itemDelimiter to slash
      repeat with x = the number of items of pLocation down to 1
         if there is a folder (item 1 to x of pLocation) then
            exit repeat
         end if
      end repeat
      
      -- Now try to create the next level
      create folder (item 1 to x + 1 of pLocation)
      if there is no folder (item 1 to x + 1 of pLocation) then
         return true
      end if
      
      -- We successfully created it, so delete it and continue.
      delete folder (item 1 to x + 1 of pLocation)
      return false
   end if
   
   -- If the location is empty, make sure we can create something inside it
   if folderIsEmpty(pLocation) then
      create folder (pLocation & slash & "lctestfolder")
      if there is no folder (pLocation & slash & "lctestfolder") then
         return true
      end if
      
      delete folder (pLocation & slash & "lctestfolder")
      return false
   end if
   
   -- Finally, if we get here then the target folder must be an existing install. Thus
   -- recursively check to see if we can delete all files (and folders) in it.
   local tCanDelete
   put canDeleteFolder(pLocation) into tCanDelete
   return not tCanDelete
end requiresAdminPrivileges

######################################################################

function canDeleteFolder pLocation
   if the environment is not "installer" then
      return false
   end if
   
   local tOldFolder
   put the folder into tOldFolder
   set the folder to pLocation
   
   repeat for each line tFile in the files
      do "_internal canDeleteFile (pLocation & slash & tFile)"
      if not the result then
         set the folder to tOldFolder
         return false
      end if
   end repeat
   
   repeat for each line tFolder in the folders
      if tFolder is ".." then
         next repeat
      end if
      
      if not canDeleteFolder(pLocation & slash & tFolder) then
         set the folder to tOldFolder
         return false
      end if
   end repeat
   
   set the folder to tOldFolder
   
   return true
end canDeleteFolder

function folderIsEmpty pLocation
   local tIsEmpty
   
   local tOldFolder
   put the folder into tOldFolder
   set the folder to pLocation
   if the files is not empty then
      put false into tIsEmpty
   else if the folders is not ".." then
      put false into tIsEmpty
   else
      put true into tIsEmpty
   end if
   set the folder to tOldFolder
   
   return tIsEmpty
end folderIsEmpty

######################################################################

command runFaceless
   local tOptions
   parseOptions tOptions
   if the result is not empty then
      write "Usage: <exe> -ui [-location <path>] [-allusers] [-desktopshortcut] [-startmenu]" & return to stderr
      quit 1
   end if 
   
   -- Compute the target location based on install type, if non specified
   if tOptions["location"] is empty then
      if tOptions["allusers"] then
         put defaultAllUserLocation() into tOptions["location"]
      else
         put defaultThisUserLocation() into tOptions["location"]
      end if
   end if
   
   -- Make the log path absolute, if specific
   if tOptions["log"] is not empty then
      if char 1 of tOptions["log"] is not "/" and char 2 of tOptions["log"] is not ":" then
         put the folder & slash before tOptions["log"]
      end if
   end if
   
   actionInstall tOptions["location"], tOptions["location"] & slash & ".setup.txt", tOptions
end runFaceless

private command parseOptions @rOptions
   local tIndex
   -- Skip $0 (executable path) and $1 ('install')
   -- '-ui' is not included in the cmd line arg vars
   put 2 into tIndex
   repeat forever
      if tIndex >= $# then
         exit repeat
      end if
      
      local tParam, tNextParam
      put value("$" & tIndex) into tParam
      if tIndex < $# - 1 then
         put value("$" & (tIndex + 1)) into tNextParam
      else
         put empty into tNextParam
      end if
      if tParam begins with "-" then
         if tNextParam begins with "-" or tNextParam is empty then
            put true into rOptions[char 2 to -1 of tParam]
            add 1 to tIndex
         else
            put tNextParam into rOptions[char 2 to -1 of tParam]
            add 2 to tIndex
         end if
      else
         return "syntax error"
      end if
   end repeat
   
   return empty
end parseOptions

######################################################################

command utilityEnsureFolder pPath
   local tIndex
   set the itemDelimiter to "/"
   repeat with tIndex = 2 to the number of items of pPath
      if there is no folder (item 1 to tIndex of pPath) then
         create folder (item 1 to tIndex of pPath)
         if the result is not empty then
            return the result
         end if
      end if
   end repeat
   return empty
end utilityEnsureFolder

command utilityDeleteFolder pFolder
   local tOldFolder
   put the folder into tOldFolder
   set the folder to pFolder
   repeat for each line tFile in the files
      delete file pFolder & slash & tFile
      if the result is not empty then
         return the result
      end if
   end repeat
   repeat for each line tFolder in the folders
      if not (tFolder begins with ".") then
         utilityDeleteFolder pFolder & slash & tFolder
         if the result is not empty then
            return the result
         end if
      end if
   end repeat
   delete folder pFolder
   if the result is not empty then
      return the result
   end if
   set the folder to tOldFolder
   return empty
end utilityDeleteFolder

######################################################################
