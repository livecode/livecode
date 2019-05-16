script "InstallerInstallerUIUpdateCheckCardBehavior"
########################################################################################

constant kUpdateURL = "http://livecode.com/livecode/update/livecode"
constant kUpdateInterval = 86400
constant kAnnoyInterval = 600

local sAnnoyMessage
local sUpdateData
local sSelectedUpdate

########################################################################################

on preOpenCard
   put empty into sUpdateData
   put empty into sSelectedUpdate
   
   hide control "Cancel"
   hide control "Back"
   hide control "Next"
   put empty into field "Page Description"
   set the uLabel of group "skip" of me to "Skip This Version"
   set the uLabel of group "remind" of me to "Remind Me Later"
   repeat with tIndex = 1 to 4
      set the uVisible of group ("update_" & tIndex) of me to false
   end repeat
   set the text of field "Marketing" to empty   
   set the textStyle of field "Heading" of me to "bold"
   set the textSize of field "Heading" of me to 24
   set the text of field "Heading" of me to "Update Checking"
   setTitleField the long id of field "Page Title", "Updater", "LiveCode Updater"   
   hide group "UpdatesAvailable"
   hide group "NoUpdatesAvailable"
   send "startLoading" to group "loader" of group "UpdateChecking"
   show group "UpdateChecking"
   
   pass preOpenCard
end preOpenCard

on openCard    
   send "autoUpdate" to me in 0 seconds
end openCard

on closeCard
   if the platform is "MacOS" and the environment is "installer" then
      do "_internal bounceCancel"
   end if
   if sAnnoyMessage is not empty then
      cancel sAnnoyMessage
   end if
   pass closeCard
end closeCard

-- This is the start of the auto update process.
-- At this point the installer is still in the background.
-- If any errors are encountered the installer should just silenty fail.
-- If any updates are found, the installer should be brought to the foreground.
--
command autoUpdate
   if the uMode of me is "foreground" then
      set the visible of this stack to true
      set the processType to "foreground"
   end if
   
   preferencesLoad
   if the result is not empty then
      actionExitUpdateProcess
   end if
   
   -- All update downloads and prefs should be stored in the LiveCoe update folder.
   -- Make sure this folder exists
   --
   utilityEnsureFolder getUpdateFolderLocation()
   if the result is not empty then
      actionExitUpdateProcess
   end if
   
   -- Load the update pref files, making sure we haven't checked for an update recently.
   --
   local tLastUpdateCheckTime, tVersionSkipList
   parseUpdatePrefFiles tLastUpdateCheckTime, tVersionSkipList
   if the result is not empty or (the uMode of me is not "foreground" and tLastUpdateCheckTime is not empty and tLastUpdateCheckTime > (the seconds - kUpdateInterval)) then
      actionExitUpdateProcess
   end if
   
   -- Contact the server to see if there are any updates.
   -- If there are no errors contacting the server, store the time of update check.
   --
   -- It's important to silenty fail if no local credentials are found.  If this is the case, the install
   -- has been activated via the comamnd line, and is most likely being run in a network environment
   -- (e.g. school, university).  Updating should be left up to administrators (we could maybe also check by license type?).
   --
   local tUpdateData
   checkForUpdates tUpdateData
   if the result is not empty then
      actionExitUpdateProcess
   end if
   put the seconds into URL ("file:" & getUpdateLastCheckFileLocation())
   if not (tUpdateData is an array) or the keys of tUpdateData is empty then
      actionExitUpdateProcess
   end if
   
   processUpdates tUpdateData, tVersionSkipList, sUpdateData
   if the result is not empty or not (sUpdateData is an array) or the keys of sUpdateData is empty then
      actionExitUpdateProcess
   end if
   
   uiPopulate
   if the result is not empty then
      actionExitUpdateProcess
   end if
   
   -- If we have reached this point, there have been no errors and there are updates available.
   -- Bring the installer to the foreground.
   --
   showUpdater
   if the result is not empty then
      actionExitUpdateProcess
   end if  
end autoUpdate

########################################################################################

-- Update/Buy Now button clicked.
-- For paid releases, navigate to store and close updater.
-- For free releases, download updater and install.
--
on actionUpdate
   switch sUpdateData[sSelectedUpdate]["type"]
      case "stable"
      case "maintainence"
      case "beta"
      case "development"
         set the uRemoteURL of card "UpdateDownload" to sUpdateData[sSelectedUpdate]["url"]
         set the uRemoteChecksum of card "UpdateDownload" to sUpdateData[sSelectedUpdate]["checksum"]
         go card "UpdateDownload"
         break         
   end switch
end actionUpdate

-- Skip This Release button click.
-- Append the version to the skip file and exit installer.
--
on actionSkip
   put sUpdateData[sSelectedUpdate]["buildnumber"] & return after URL ("file:" & getUpdateVersionSkipFileLocation())
   actionCancel
end actionSkip

-- Remind Me Later button clicked.
-- Just quite installer
--
on actionLater
   actionCancel
end actionLater

-- An update version have been selected by one on the update nav buttons on the left hand side.
-- Flag this update and the selected update and update the ui with the appropriate details.
--
on actionSelect pType
   put pType into sSelectedUpdate
   lock screen
   uiPopulateNavButtonStates
   uiPopulateActionButtons
   uiPopulateMktText
   unlock screen
end actionSelect

on actionExitUpdateProcess
   if the uMode of me is "foreground" then
      lock screen
      set the text of field "Heading" of me to "No Updates"
      hide group "UpdatesAvailable"
      show group "NoUpdatesAvailable"
      hide group "UpdateChecking"
      unlock screen
      resetAll
      exit to top
   else
      actionCancel
   end if
end actionExitUpdateProcess

########################################################################################

-- Bring the updater to the foreground.
-- On OS X we bounce the Dock icon.  On Windows we create a status icon with a balloon.
-- The above is done using custom commands added to the installer.
-- The doc icon bounces and the status balloon is shown periodically until the user reacts.
-- To be updated on Linux.  For the moment we just show the installer.
--
on showUpdater
   set the processType to "foreground"   
   switch the platform
      case "MacOS"
         set the icon to resolveImageId("icon")
         if the environment is "installer" then
            do "_internal bounce"
            send "showUpdater" to me in kAnnoyInterval seconds
            put the result into sAnnoyMessage
         end if
         break
      case "Win32"
         set the statusIcon to resolveImageId("status-icon")
         set the statusIconTooltip to "LiveCode Updater"         
         if the environment is "installer" then 
            local tTitle, tMsg
            put "A new version of LiveCode is available" into tTitle
            put "Your version of LiveCode is out of date.  Click to here to begin updating." into tMsg
            do "_internal showBalloon tTitle tMsg"
            send "showUpdater" to me in kAnnoyInterval seconds
            put the result into sAnnoyMessage
         end if
         break
      case "Linux"
         set the visible of this stack to true
         break
   end switch
   return empty
end showUpdater

-- Dock icon clicked.  Stop bouncing.
--
on appleEvent p1, p2, p3
   if p1 & p2 is "aevtrapp" then
      set the visible of this stack to true
      if sAnnoyMessage is not empty then
         cancel sAnnoyMessage
      end if
   end if
   pass appleEvent
end appleEvent

on statusIconClick
   set the visible of this stack to true
   if sAnnoyMessage is not empty then
      cancel sAnnoyMessage
   end if
end statusIconClick

on mouseDown
   if the platform is "MacOS" and the environment is "installer" then
      do "_internal bounceCancel"
   end if
   pass mouseDown
end mouseDown

########################################################################################

-- Parse the update preference files, returning the last time an update was check for
-- (in seconds) and a return delimited list of versions to skip.
--
-- Returns empty if successful, an error string if not.
--
private command parseUpdatePrefFiles @rLastUpdateCheckTime, @rVersionSkipList
   local tUpatePrefFile
   put getUpdateLastCheckFileLocation() into tUpatePrefFile
   if there is a file tUpatePrefFile then
      get URL ("file:" & tUpatePrefFile)
      if word 1 of it is not empty and word 1 of it is an integer then
         put word 1 of it into rLastUpdateCheckTime
      end if
   end if
   put getUpdateVersionSkipFileLocation() into tUpatePrefFile
   if there is a file tUpatePrefFile then
      put URL ("file:" & tUpatePrefFile) into rVersionSkipList
   end if
   return empty
end parseUpdatePrefFiles

-- Contact the server to see if there are any updates for this user.
-- Place any update data returned by the server in rUpdateData.
--
-- Returns empty if successful, an error string if not.
--
private command checkForUpdates @rUpdateData
   -- The livecode.lcld in the licenses folder stores the email address
   -- and the (md5 hash of the) password for the currently activated user.
   --
   -- MM-2013-06-13: Since community no longer requires activation, don't
   -- fail if no user details found.
   --
   get getLicensesFolderLocation() & "/livecode.lcld"
   if there is no file it then
      get getLicensesFolderLocation() & "/livecode-community.lcld"
   end if
   if there is no file it then
      --return "no lcld file"
   end if
   
   local tActivationDetails
   put url ("file:" & it) into tActivationDetails
   if the first line of tActivationDetails is not "---- LIVECODE ACTIVATION DETAILS (R1) ----" or \
         the last line of tActivationDetails is not "---- LIVECODE ACTIVATION DETAILS (R1) ----" then
      --return "invalid lcld file"
   end if
   delete the first line of tActivationDetails
   delete the last line of tActivationDetails
   split tActivationDetails by return and "="
   
   if tActivationDetails["email"] is empty then
      --return "no username"
   end if
   if tActivationDetails["password"] is empty then
      --return "no password"
   end if
   
   -- MM-2013-06-13: Since we may no longer have the user's activaiton details,
   -- the server instead generates us a uniquie ID for the user. Check to see if one has
   -- already been generated and if so, pass on to the server.
   --
   local tUUID
   put URL ("binfile:" & getUpdateUniqueUserIDFileLocation()) into tUUID
   
   -- The server communcation is in two steps.  Fist we connect with the sever, sending thre username.
   -- If successful, we'll have a session on the server.
   --
   local tSessionID, tChallenge
   serverLogin tActivationDetails["email"], tUUID, tSessionID, tChallenge
   if the result is not empty  then
      return the result
   end if
   
   -- Secondly, we send a digest of the password baack to the server.
   -- If successful, we'll get back a list of updates for the current user.
   --
   serverList tActivationDetails["password"], tSessionID, tChallenge, getBuildNumber(), getPlatformString(), rUpdateData
   if the result is not empty  then
      return the result
   end if 
   
   return empty
end checkForUpdates

-- Take the raw list of updates from the server and process it for display.
-- Remove any updates the user doesn't want to see. Form into an array of up to
-- 3 updates - free, paid and test.
--
-- Returns empty if successful, an error string if not.
--
private command processUpdates pUpdateData, pVersionSkipList, @rProcessesUpdateData
   local tIndex
   put 1 into tIndex
   repeat while pUpdateData["update." & tIndex & ".ver"] is not empty
      
      -- Repeat through each update retunred by the server.  Each update is of the following form:
      --   update.<n>.ver = e.g. 5.0.0-dp-2
      --   update.<n>.buildnumber = 1455
      --   update.<n>.url = link to download releae for free updates, link to store for paid updates
      --   update.<n>.type = free|paid|free-test|paid-test
      -- where n is an integer starting from 1.
      --
      if pUpdateData["update." & tIndex & ".buildnumber"] is not empty and \
            pUpdateData["update." & tIndex & ".url"] is not empty and \
            pUpdateData["update." & tIndex & ".type"] is not empty
      then
         
         if pUpdateData["update." & tIndex & ".buildnumber"] is among the lines of pVersionSkipList then
            add 1 to tIndex
            next repeat
         end if
         if getVersionInstalled(pUpdateData["update." & tIndex & ".ver"], pUpdateData["update." & tIndex & ".buildnumber"]) or pUpdateData["update." & tIndex & ".buildnumber"] is getBuildNumber() then
            add 1 to tIndex
            next repeat
         end if
         
         -- Make sure free updates have a checksum.  Free updates will be automatically downloaded
         -- and installed.  The checksum is needed by the download process.
         --
         if  pUpdateData["update." & tIndex & ".checksum"] is empty then
            add 1 to tIndex
            next repeat
         end if
         
         local tType
         switch pUpdateData["update." & tIndex & ".type"]
            case "stable"
               put "stable" into tType
               break
            case "maintainence"
               put "maintainence" into tType
               break
            case "beta"
               put "beta" into tType
               break
            case "development"
               put "development" into tType
               break               
            default
               add 1 to tIndex
               next repeat
         end switch
         
         if tType is "stable" and the cNotifyMeOfStableReleases of stack "revPreferences" is false then
            add 1 to tIndex
            next repeat
         end if
         if tType is "maintainence" and the cNotifyMeOfMaintainenceReleases of stack "revPreferences" is false then
            add 1 to tIndex
            next repeat
         end if
         if tType is "beta" and the cNotifyMeOfBetaReleases of stack "revPreferences" is false then
            add 1 to tIndex
            next repeat
         end if
         if tType is "development" and the cNotifyMeOfDevelopmentReleases of stack "revPreferences" is false then
            add 1 to tIndex
            next repeat
         end if
         
         -- Ignore this release is we have already have one of this type.
         --
         if rProcessesUpdateData[tType] is an array then
            add 1 to tIndex
            next repeat
         end if
         
         put pUpdateData["update." & tIndex & ".url"] into rProcessesUpdateData[tType]["url"]
         put pUpdateData["update." & tIndex & ".ver"] into rProcessesUpdateData[tType]["version"]
         put pUpdateData["update." & tIndex & ".type"] into rProcessesUpdateData[tType]["type"]
         put pUpdateData["update." & tIndex & ".checksum"] into rProcessesUpdateData[tType]["checksum"]
         put pUpdateData["update." & tIndex & ".buildnumber"] into rProcessesUpdateData[tType]["buildnumber"]         
         
         -- Exctract any marketing data.  Marketing data is of the following form:
         --   update.<n>.marketing.<m>.content = base64Encoded marketing text
         --   update.<n>.marketing.<m>.type = plain|unicode|utf8|rtf|html
         -- where m is an integer starting from 1.
         --
         -- This allows for richly formatted marketing text and the store to aggregate marketing messages
         -- of different formats.
         --
         local tMktIndex
         put 1 into tMktIndex
         repeat while pUpdateData["update." & tIndex & ".marketing." & tMktIndex & ".type"] is not empty
            put base64decode(pUpdateData["update." & tIndex & ".marketing." & tMktIndex & ".content"]) into rProcessesUpdateData[tType]["marketing"][tMktIndex]["content"]
            put pUpdateData["update." & tIndex & ".marketing." & tMktIndex & ".type"] into rProcessesUpdateData[tType]["marketing"][tMktIndex]["type"]
            add 1 to tMktIndex
         end repeat
         
         -- If no marketing data, then use defaults.
         --
         if tMktIndex is 1 then
            switch pUpdateData["update." & tIndex & ".type"]
               case "stable"
                  put the uDefaultMktingStableContent of me into rProcessesUpdateData[tType]["marketing"][tMktIndex]["content"]
                  put the uDefaultMktingStableType of me into rProcessesUpdateData[tType]["marketing"][tMktIndex]["type"]
                  break
               case "maintainence"
                  put the uDefaultMktingMaintainenceContent of me into rProcessesUpdateData[tType]["marketing"][tMktIndex]["content"]
                  put the uDefaultMktingMaintainenceType of me into rProcessesUpdateData[tType]["marketing"][tMktIndex]["type"]
                  break
               case "beta"
                  put the uDefaultMktingBetaContent of me into rProcessesUpdateData[tType]["marketing"][tMktIndex]["content"]
                  put the uDefaultMktingBetaType of me into rProcessesUpdateData[tType]["marketing"][tMktIndex]["type"]
                  break
               case "development"
                  put the uDefaultMktingDevelopmentContent of me into rProcessesUpdateData[tType]["marketing"][tMktIndex]["content"]
                  put the uDefaultMktingDevelopmentType of me into rProcessesUpdateData[tType]["marketing"][tMktIndex]["type"]
                  break
            end switch              
         end if
         
      end if
      add 1 to tIndex
   end repeat
   return empty
end processUpdates

########################################################################################

private command uiPopulate
   lock screen
   if the number lines in the keys of sUpdateData > 1 then
      get "Updates Available"
   else
      get "Update Available"
   end if
   set the text of field "Heading" of me to it
   setTitleField the long id of field "Page Title", it, "LiveCode Updater"
   set the uWide of group "skip" of me to true
   set the uWide of group "remind" of me to true
   uiPopulateNavButtons
   uiPopulateActionButtons
   uiPopulateMktText   
   show group "UpdatesAvailable"
   hide group "NoUpdatesAvailable"
   hide group "UpdateChecking"
   unlock screen
   return empty
end uiPopulate

-- The nav buttons are the buttons on the left hand side that allow the user to navigate between
-- the updates they have avaiable.  There will be a maximum of three, and they will be in the order
-- free paid test.
--
private command uiPopulateNavButtons
   local tNavButtonIndex
   put 1 into tNavButtonIndex
   repeat for each word tType in "stable maintainence beta development"
      if sUpdateData[tType] is not an array then
         next repeat
      end if
      
      -- If we have not yet selected an update, select this one.
      -- This means that by default, the first update is selected.
      --
      if sSelectedUpdate is empty then
         put tType into sSelectedUpdate
      end if
      
      -- We strip out the gm part of the version number unless the update is a new gm.
      -- e.g. The current version is 4.6.4-gm-1, the update is 4.6.4-gm-2.
      --
      local tVersion
      put sUpdateData[tType]["version"] into tVersion
      if tVersion contains "gm" then
         set the itemDel to "-"
         if item 1 of tVersion is not the version then
            put item 1 of tVersion into tVersion
         end if
         set the itemDel to comma
      end if
      
      -- We use the default state to indicate that this is the currently selected update.
      --
      --set the uLabel of group ("update_" & tNavButtonIndex) of me to "LiveCode" & return & tVersion
      set the uLabel of group ("update_" & tNavButtonIndex) of me to tVersion
      set the uType of group ("update_" & tNavButtonIndex) of me to tType
      set the uDefault of group ("update_" & tNavButtonIndex) of me to (tNavButtonIndex is 1)
      set the uVisible of group ("update_" & tNavButtonIndex) of me to true
      
      add 1 to tNavButtonIndex
   end repeat
end uiPopulateNavButtons

private command uiPopulateNavButtonStates
   --repeat with tIndex = 1 to 3
   repeat with tIndex = 1 to 4
      if there is a group ("update_" & tIndex) of me and the visible of group ("update_" & tIndex) of me then
         set the uDefault of group ("update_" & tIndex) of me to (the uType of group ("update_" & tIndex) of me is sSelectedUpdate)
      end if
   end repeat
end uiPopulateNavButtonStates

-- The action buttons are the three buttons along the bottom of the stack.
-- They are "Remind Me Later", "Skip This Version" and "Byt/Update Now".
--
-- We only ever need to update the but/update now button label.
--
private command uiPopulateActionButtons
   switch sUpdateData[sSelectedUpdate]["type"]
      case "stable"
      case "maintainence"
      case "beta"
      case "development"
         set the uLabel of group "update" of me to "Update Now"
         break
   end switch
   set the uDefault of group "update" of me to true
end uiPopulateActionButtons

private command uiPopulateMktText
   --set the uFormattedTextArray of control "Marketing Text" to sUpdateData[sSelectedUpdate]["marketing"]
   set the uFormattedTextArray of control "Marketing" to sUpdateData[sSelectedUpdate]["marketing"]
end uiPopulateMktText

########################################################################################

private command serverLogin pUserName, pUUID, @rSessionID, @rChallenge
   local tPostData, tResult
   put "action=login" & return & \
         "email=" & pUserName & return & \
         "uid=" & pUUID into tPostData
   post encodeForPost(tPostData) to kUpdateUrl
   if the result is not empty then
      return the result
   end if
   put it into tResult
   split tResult by return and "="
   if tResult["result"] is empty then
      return "error on server login"
   end if
   if tResult["result"] is not "success" then
      return tResult["result"] 
   end if
   if tResult["session"] is empty or tResult["challenge"] is empty then
      return "missing challenge or session"
   end if
   put tResult["uid"] into URL ("binfile:" & getUpdateUniqueUserIDFileLocation())
   put tResult["session"] into rSessionID
   put hexDecode(tResult["challenge"]) into rChallenge
   return empty
end serverLogin

private command serverList pPassword, pSessionID, pChallenge, pBuildNumber, pPlatform, @rData
   local tPostData, tResult
   put "action=list" & return & \
         "session=" & pSessionID & return & \
         "token=" & hexEncode(md5Digest(hexDecode(pPassword) & md5Digest(pChallenge))) & return & \
         "build=" & pBuildNumber & return & \
         "platform=" & pPlatform & return & \
         "type=" & getInstallerType() into tPostData
   post encodeForPost(tPostData) to kUpdateUrl
   if the result is not empty then
      return the result
   end if
   put it into tResult
   split tResult by return and "="
   if tResult["result"] is empty then
      return "error on server list"
   end if
   if tResult["result"] is not "success" then
      return tResult["result"] 
   end if
   delete variable tResult["result"]
   put tResult into rData
   return empty
end serverList

private function encodeForPost pData
   local tPostData
   set the itemDelimiter to "="
   repeat for each line tLine in pData
      put item 1 of tLine & "=" & the urlEncode of item 2 to -1 of tLine & "&" after tPostData
   end repeat
   delete the last char of tPostData
   return tPostData
end encodeForPost

private function hexEncode pValue
   local tHexValue
   if pValue is empty then
      return empty
   end if
   get binaryDecode("H" & the length of pValue * 2, pValue, tHexValue)
   return tHexValue
end hexEncode

private function hexDecode pHexValue
   if pHexValue is empty then
      return empty
   end if
   return binaryEncode("H" & the length of pHexValue, pHexValue)
end hexDecode

########################################################################################

-- Returns true if the given vesion is installed.
--
-- Makes assumption based on license files - if there is a license file for
-- the version, the user has downloaed, installed and activated.
--
-- For gm releases, the gm part of the version is stripped, so we can't differentaite
-- between license files for gm-1, gm-2, gm-3 etc.
-- If there is a license file (e.g. 4.6.4), we can only assume gm-1 is installed.
--
-- This licensing stack now been updated to include the build number in the license file.
-- This solves the GM issue.
--
private function getVersionInstalled pVersion, pBuildNumber
   --   if pVersion contains "-gm-" then
   --      if not (pVersion ends with "-gm-1") then
   --         return false
   --      else
   --         set the itemDel to "-"
   --         put item 1 of pVersion into pVersion
   --         set the itemDel to comma
   --      end if
   --   end if
   replace "." with "_" in pVersion
   if pVersion contains "-gm-" then
      set the itemDel to "-"
      put item 1 of pVersion into pVersion
      set the itemDel to comma
   end if
   if getInstallerType() is "community" then
      return there is a file (getLicensesFolderLocation()  & "/livecode-community-" & pVersion & "-" & pBuildNumber & ".lclk")
   else
      return there is a file (getLicensesFolderLocation()  & "/livecode-" & pVersion & "-" & pBuildNumber & ".lclk")
   end if
end getVersionInstalled

private function getLicensesFolderLocation
   switch the platform
      case "Win32"
         return specialFolderPath(0x001a) & "/RunRev/Licenses"
      case "MacOS"
         return specialFolderPath("home") & "/Library/Application Support/RunRev/Licenses"
      case "Linux"
         return specialFolderPath("home") & "/.runrev/licenses"
   end switch
end getLicensesFolderLocation

private function getUpdateLastCheckFileLocation
   return getUpdateFolderLocation() & "/update_" & getInstallerType() & "_last_check_time.txt"
end getUpdateLastCheckFileLocation

private function getUpdateVersionSkipFileLocation
   return getUpdateFolderLocation() & "/update_" & getInstallerType() & "_version_skip_list.txt"
end getUpdateVersionSkipFileLocation

private function getUpdateUniqueUserIDFileLocation
   return getUpdateFolderLocation() & "/update_uuid.txt"
end getUpdateUniqueUserIDFileLocation

private function getBuildNumber
   --   get getUpdateFolderLocation() & "/update_override.txt"
   --   if there is a file it then
   --      return word 1 of URL ("file:" & it)
   --   end if
   return the buildNumber
end getBuildNumber

private function getPlatformString
   switch the platform
      case "Win32"
         return "windows" & slash & the processor
      case "MacOS"
         return "mac" & slash & the processor
      case "Linux"
         return "linux" & slash & the processor
   end switch
end getPlatformString

########################################################################################

-- Load the prefences stack, setting the appopraite defaults if necessary.
--
-- Returns empty if successful, an error string otherwise.
--
private command preferencesLoad
   local tPrefStack
   put getPreferencesStackLocation() into tPrefStack
   if not (there is a file tPrefStack) then
      return "preferences stack not found"
   end if
   start using stack tPrefStack
     
   if not ("cNeverNotifyMeOfReleases" is among the keys of customproperties of stack "revPreferences") then
      set the cNeverNotifyMeOfReleases of stack "revPreferences" to false 
   end if
   if not ("cNotifyMeOfStableReleases" is among the keys of customproperties of stack "revPreferences") then
      set the cNotifyMeOfStableReleases of stack "revPreferences" to true 
   end if
   if not ("cNotifyMeOfMaintainenceReleases" is among the keys of customproperties of stack "revPreferences") then
      set the cNotifyMeOfMaintainenceReleases of stack "revPreferences" to true 
   end if
   if not ("cNotifyMeOfBetaReleases" is among the keys of customproperties of stack "revPreferences") then
      set the cNotifyMeOfBetaReleases of stack "revPreferences" to true 
   end if
   if not ("cNotifyMeOfDevelopmentReleases" is among the keys of customproperties of stack "revPreferences") then
      set the cNotifyMeOfDevelopmentReleases of stack "revPreferences" to true 
   end if   
   
   return empty
end preferencesLoad

private command preferencesSave
   local tOldStackFileVersion
   put the stackFileVersion into tOldStackFileVersion
   set the stackFileVersion to "2.7"
   lock messages
   save stack "revPreferences"
   unlock messages
   set the stackFileVersion to tOldStackFileVersion
   return empty
end preferencesSave

private function getPreferencesStackLocation
   -- SN-2015-03-09: [[ LiveCode 7 Prefs ]] Make sure to get the
   -- right preference file, depending on the version
   local tLiveCodePost7Prefs, tLiveCodePre7Prefs, tLiveCodePrefsRoot
   switch the platform
      case "MacOS"
         put specialfolderpath("Preferences") & "/RunRev" into tLiveCodePrefsRoot
         break
      case "Win32"
         put specialFolderPath(0x001a) & "/RunRev/Preferences" into tLiveCodePrefsRoot
         break
      case "Linux"
         put specialFolderPath("home") & "/.runrev/preferences" into tLiveCodePrefsRoot
         break
   end switch
   
   put tLiveCodePrefsRoot & "/livecode.rev" into tLiveCodePre7Prefs
   put tLiveCodePrefsRoot & "/livecode7.rev" into tLiveCodePost7Prefs
   
   if the char 1 of the version < 7 then
      -- Pre 7
      return tLiveCodePre7Prefs
   else
      -- We only return the LiveCode 7 prefs path if the file exists
      -- We default to the pre-7 prefs otherwise.
      if there is a file tLiveCodePost7Prefs then
         return tLiveCodePost7Prefs
      else
         return tLiveCodePre7Prefs
      end if
   end if
end getPreferencesStackLocation

########################################################################################
