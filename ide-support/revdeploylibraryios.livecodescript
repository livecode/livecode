script "revDeployLibraryIOS"
////////////////////////////////////////////////////////////////////////////////

local sDeploySimulators
local sDeploySimulatorToolset
local sDeploySimulatorVersion
local sDeploySimulatorFamily
local sDeploySimulatorIsRunning

-- Used to catch the return from the iphone external callback
local sDeploySimulationStatus

-- List of device SDKs that thje currently selected versions of XCode support.
local sDeviceSDKs

////////////////////////////////////////////////////////////////////////////////

-- Called on startup to initialize the deployment platform if possible.
command deployInitialize
   if the platform is not "macos" then
      exit deployInitialize
   end if
   
   deployAutoconfigureSDKs
   deployUpdateSDKs
end deployInitialize

-- Invoked to check whether the platform is available for deployment
function deployIsAvailable
   local tEnabled
   
   if the platform is not "macos" then
      put false into tEnabled
   else if sDeploySimulators is not an array then
      put false into tEnabled
   else
      put true into tEnabled
   end if
   
   return tEnabled
end deployIsAvailable

-- Invoked to fetch the list of targets for this simulation platform
function deployListTargets
   local tTargets
   
   -- SN-2015-02-18: [[ ARM64 ]] We dropped support for 
   --  iOS Simulators < 5.1
   put "5.1,6.1,7.1,8.2" into tAvailableSimulators
   
   repeat for each key tKey in sDeploySimulators
      if tKey is among the items of tAvailableSimulators then
         put "iPad Simulator" && tKey & return after tTargets
      end if
      if tKey is among the items of tAvailableSimulators then
         put "iPhone Simulator" && tKey & return after tTargets
      end if
   end repeat
   delete the last char of tTargets
   
   sort tTargets text ascending
   
   return tTargets
end deployListTargets

-- Invoked when the Deploy/Simulate button is clicked on the menubar
command deployDo pTargetStack, pSimulator, pAppBundle
   -- The error that occured (if any)
   local tError, tRunningTest
   put empty into tError
   
   put revTestEnvironment() into tRunningTest
   
   -- First attempt to rebuild the current top-stack
   local tTempFolder
   if tError is empty then
      put tempName() into tTempFolder
      create folder tTempFolder
      if the result is not empty then
         put "Unable to create temporary folder" into tError
      end if
   end if
   
   -- Compute what version and family to use based on the plist
   local tVersion, tFamily
   if tError is empty then
      put word 3 of pSimulator into tVersion
      put word 1 of pSimulator into tFamily
      
      -- First make sure the minimum version is adhered to
      if tVersion < the cRevStandaloneSettings["ios,minimum version"] of stack pTargetStack then
         put the cRevStandaloneSettings["ios,minimum version"] of stack pTargetStack into tVersion
         if tFamily is not among the lines of deployGetSimulatorFamilies(tVersion) then
            put line 1 of deployGetSimulatorFamilies(tVersion) into tFamily
         end if
      end if
      
      -- If the app is only for iPad, then force to iPad family
      if the cRevStandaloneSettings["ios,device family"] of stack pTargetStack is 2 then
         put "iPad" into tFamily
         if tFamily is not among the lines of deployGetSimulatorFamilies(tVersion) then
            put empty into tVersion
            get deployGetSimulatorVersions()
            sort it descending
            repeat for each line tLine in it
               if "iPad" is among the lines of deployGetSimulatorFamilies(tLine) then
                  put tLine into tVersion
                  exit repeat
               end if
            end repeat
            if tVersion is empty then
               put "App is specified as iPad only, but no iPad simulators are installed." into tError
            end if
         end if
      end if
      
      -- Finally check we have a suitable minimum version
      if tVersion < the cRevStandaloneSettings["ios,minimum version"] of stack pTargetStack then
         put "No simulators matching specified family or minimum version are installed." into tError
      end if
   end if
   
   local tTempAppBundle
   if tError is empty then
      if pAppBundle is empty then
         put tTempFolder & slash & the cREVStandaloneSettings["name"] of stack pTargetStack & ".app" into tTempAppBundle
         try
            dispatch "revSaveAsMobileStandalone" to stack "revSaveAsIOSStandalone" with \
                  pTargetStack, tTempAppBundle, "Simulator" && tVersion
         catch tException
            put "Unable to build app for simulation:" && tException into tError
         end try
      else
         put pAppBundle into tTempAppBundle
      end if 
   end if
   
   -- Next phase is to ensure we have the right simulator running
   if tError is empty then
      revStandaloneProgress "Configuring simulator..."
      
      -- Make sure any currently running app is terminated
      if sDeploySimulatorIsRunning then
         try
            revIPhoneTerminateAppInSimulator
         end try
         
         -- Make sure we wait until the app has finished terminating
         repeat while revIPhoneAppIsRunning()
            wait 100 millisecs with messages
         end repeat
         
         put false into sDeploySimulatorIsRunning
      end if
      
      -- If we have to change the toolset, make sure we terminate any
      -- *other* running simulators first
      get sDeploySimulators[tVersion]
      if it is not sDeploySimulatorToolset then
         revIDEDeployIOSKillOtherSimulators it
         try
            -- MW-2012-03-01: [[ Bug ]] If the developer root is inside the Xcode bundle then we must
            --   set the DYLD_FRAMEWORK_PATH to the app's OtherFrameworks folder.
            -- MM-2012-06-20: [[ Bug ]] Support any bundle that contains XCode - not just XCode.app
            local tOldFrameworkPath
            if it contains "Xcode" and it contains ".app" then
               put $DYLD_FRAMEWORK_PATH into tOldFrameworkPath
               put it & "/../OtherFrameworks" into $DYLD_FRAMEWORK_PATH
            end if
            revIPhoneSetToolset it
            if it contains "Xcode" and it contains ".app" then
               put tOldFrameworkPath into $DYLD_FRAMEWORK_PATH
            end if
         end try
         put it into sDeploySimulatorToolset
      end if
      
      -- Next try to switch the sdk
      try
         revIPhoneSetSimulatorSDK tVersion
      catch tException
         put "Unable to start simulator:" && tException into tError
      end try
   end if
   
   -- Finally try and launch the app
   if tError is empty then
      revStandaloneProgress "Launching app..."
      
      put empty into sDeploySimulationStatus
      try
         revIPhoneLaunchAppInSimulator tTempAppBundle, tFamily, "revIDEDeployIOSLaunchCallback"
      catch tException
      end try
      
      --MM-2011-09-28: The simulator appears to time out in Lion if not at front.
      --
      --get "tell application" && quote & "iPhone Simulator" & quote & "to activate"
      --do it as "applescript"
      
      local tRunawayTimer
      put the millisecs + 10000 into tRunawayTimer
      repeat forever
         -- First see if the simulator has launched
         if sDeploySimulationStatus is not empty then
            exit repeat
         end if
         
         -- Now see if the runaway timer has elapsed
         
         -- If running in the test system do not show warning dialog
         if not tRunningTest then
            if the millisecs > tRunawayTimer then
               answer "The simulator seems to be taking a long time to launch the application. Do you wish to continue trying?" with "Yes" and "No"
               if it is "No" then
                  revIPhoneTerminateAppInSimulator
                  -- Make sure we get any callback from the simulator
                  wait 10 millisecs with messages
                  put false into sDeploySimulatorIsRunning
                  exit repeat
               end if
               put the millisecs + 10000 into tRunawayTimer
            end if
         end if
         -- Otherwise wait with messages for a while
         wait for 50 millisecs with messages
      end repeat
      
      if sDeploySimulationStatus is "started" then
         put true into sDeploySimulatorIsRunning
         -- Make sure we bring the simulator to front
         --do "tell application" && quote & "iPhone SImulator" & quote & return & \
               --"activate" & return & \
               --"end tell" as "applescript" 
      else if sDeploySimulationStatus begins with "error:" then
         put "Unable to start simulation:" && word 2 to -1 of sDeploySimulationStatus into tError
      end if
   end if
   
   if "revStandaloneProgress" is among the lines of the openStacks then
      close stack "revStandaloneProgress"
   end if
   
   if tTempFolder is not empty and \
         there is a folder tTempFolder then
      get shell("rm -r" && quote & tTempFolder & quote)
   end if
   
   return tError
end deployDo

command deployBuild pTargetStack, pOutputFolder
end deployBuild

-- This callback is invoked by the iOS simulator interconnect external when
-- the status of a launch request changes.
command revIDEDeployIOSLaunchCallback pStatus
   put pStatus into sDeploySimulationStatus
end revIDEDeployIOSLaunchCallback

////////////////////////////////////////////////////////////////////////////////

-- Two iOS simulator versions can't be running at once as they interfere
-- with each other in various unpleasant ways. This handler kills all the
-- simulator versions apart from the one specified in 'pKeep'.
private command revIDEDeployIOSKillOtherSimulators pKeep
   get shell("ps ax")
   put "/Platforms/iPhoneSimulator.platform" after pKeep
   repeat for each line tProcess in it
      if tProcess contains "/MacOS/iPhone Simulator" then
         if not ((word 5 of tProcess) begins with (pKeep & slash)) then
            get shell("kill " && word 1 of tProcess)
         end if
      end if
   end repeat
end revIDEDeployIOSKillOtherSimulators

-- MM-2012-09-18: Update the internal list of simulators and SDKs.
-- Replaced deployUpdateSimulators, since we now maintain more than a single device SDK.
--
command deployUpdateSDKs
   if the platform is not "macos" then
      put empty into sDeploySimulators
   end if
   
   local tOldFolder
   put the folder into tOldFolder
   
   -- This will be an array of simulator versions mapping to their SDK
   local tSimulators
   put empty into tSimulators
   
   local tDeviceSDKs
   put empty into tDeviceSDKs
   
   -- Compute the SDKs to check
   local tSDKs
   put the cMobileSupport["ios.sdks"] of stack "revPreferences" into tSDKs
   
   -- Now loop through the SDKs building the list of available sims
   repeat for each line tSDK in tSDKs
      set the folder to tSDK & "/Platforms/iPhoneSimulator.platform/Developer/SDKs"
      if the result is empty then
         -- Include each simulator that isn't < 5.1 in the array, and ignore any
         -- above 8.2 as that's the upper limit on supported versions for the
         -- current release.
         repeat for each line tSim in (the folders & return & the files)
            get char 16 to -5 of tSim
            
            -- MM-2012-09-18: We no no longer support simulators before 4.3.
            -- PM-2015-02-06: We no no longer support simulators before 5.1.
            if it < "5.1" then
               next repeat
            end if
            
            -- MW-2012-03-21: Allow anything up to and including 8.1 support.
            -- SN-2015-03-12: [[ Bug 14866 ]] We don't want to have an upper limit anymore.
            --  If the new Xcode version brings compilation error, that will anyway be
            --  part of a fix, but we want to keep any transition, first possible
            --if it > "8.1" then
               --next repeat
            --end if
            
            if tSimulators[it] is not empty then
               next repeat
            end if
            
            put tSDK into tSimulators[it]
         end repeat        
      end if
      
      -- Look for SDK version 5.0, 5.1 and 6.0, used for device builds.
      --
      set the folder to tSDK & "/Platforms/iPhoneOS.platform/Developer/SDKs"
      if the result is empty then
         repeat for each line tDevSDK in (the folders & return & the files)
            get char 9 to -5 of tDevSDK
            if it is not among the words of "5.1 6.0 6.1 7.0 7.1 8.0 8.1 8.2" then
               next repeat
            end if
            if tDeviceSDKs[it] is not empty then
               next repeat
            end if
            put tSDK into tDeviceSDKs[it]
         end repeat        
      end if     
      
   end repeat
   
   put tSimulators into sDeploySimulators
   put tDeviceSDKs into sDeviceSDKS
   
   set the folder to tOldFolder
   
   revIDEDeployTargetsChanged
end deployUpdateSDKs

-- SN-2015-03-12: [[ Bug 14866 ]] Move the command updateSDKs from
-- card "Mobile" of stack "revpreferencesgui" here, to make any update
-- after a new Xcode release more convenient
-- MM-2013-10-08: We now only build for Arm v7, reducing the SDKs users now require.
function deployCheckLatestSDKIsInstalled @rTooltip, @rAvailableSDKs
   local tSDKStatus, tValid
   put empty into tSDKStatus   
   put true into tValid
   
   get revIDEDeployIOSGetSDKVersions()
   
   -- PM-2015-02-06: [[ Bug 14422 ]] Removed OSX 10.6 and iOS 5.0 support
   if revIDEDeployIOSSanatizeSystemVersion() < 100800 then
      if "6.1" is not among the lines of it then
         put false into tValid
         put "In order to perfom device builds, you must have the iOS 6.1 SDK configured." & return & "The iOS 6.1 SDK comes with XCode 4.6." into tSDKStatus
      end if
   else if revIDEDeployIOSSanatizeSystemVersion() < 100900 then
      if "7.1" is not among the lines of it then
         put false into tValid
         put "In order to perfom device builds, you must have the iOS 7.1 SDK configured." & return & "The iOS 7.1 SDK comes with XCode 5.1." into tSDKStatus
      end if
   else
      -- SN-2015-03-12: [[ Bug 14866 ]] iOS SDK 8.2 is now the valid SDK
      if "8.2" is not among the lines of it then
         put false into tValid
         put "In order to perfom device builds, you must have the iOS 8.2 SDK configured." & return & "The iOS 8.2 SDK comes with XCode 6.2." into tSDKStatus
      end if
   end if
   
   if it is empty then
      get "(none)"
   else
      replace return with comma & space in it
   end if
   
   put it into rAvailableSDKs
   put tSDKStatus into rSDKStatus
   
   return tValid
end deployCheckLatestSDKIsInstalled

-- Invoked to get the list of simulators available
function deployGetSimulatorVersions
   get the keys of sDeploySimulators
   sort it ascending
   return it
end deployGetSimulatorVersions

-- MM-2012-09-13: Invoked to get the list of SDKs available
function deployGetSDKVersions
   get the keys of sDeviceSDKs
   sort it ascending
   return it
end deployGetSDKVersions

-- Invoked to get the list of devices valid for the given simulator
function deployGetSimulatorFamilies pSimulator
   if pSimulator >= "4.2" then
      return "iPhone" & return & "iPad"
   end if
   return "iPhone"
end deployGetSimulatorFamilies

-- MW-2012-03-21: Returns the version of SDK to use for device builds.
-- MM-2012-09-13: We now no longer have a single device SDK. Return the appropriate
-- SDK from the array for the given device version.
--
function deployGetDeviceSDK pVersion   
   return sDeviceSDKs[pVersion]
end deployGetDeviceSDK

-- Invoked on startup to 'auto-configure' the SDK roots, if they have not been explicitly
-- set. For now we just check to see if '/Developer' is either an old or new sdk.
-- We could use:
--   system_profiler SPApplicationsDataType | grep "Applications/Xcode.app"
-- However this can take a long time to complete so can't just 'be done' on startup.
--
-- MM-2012-08-18: Updated to support the new list style SDKs.
--
command deployAutoconfigureSDKs
   local tCurrent3_2, tCurrent4, tCurrent5, tCurrentList
   put the cMobileSupport["ios.sdk-3_2"] of stack "revPreferences" into tCurrent3_2
   put the cMobileSupport["ios.sdk-4"] of stack "revPreferences" into tCurrent4
   put the cMobileSupport["ios.sdk-5"] of stack "revPreferences" into tCurrent5
   put the cMobileSupport["ios.sdks"] of stack "revPreferences" into tCurrentList
   
   set the wholeMatches to true
   get tCurrentList
   repeat for each line tSDK in it
      if not deployIsValidSDK(tSDK) then
         delete line lineoffset(tSDK, tCurrentList) of tCurrentList
      end if
   end repeat
   
   if tCurrentList is empty then
      if deployIsValidSDK(tCurrent5) then
         put tCurrent5 after tCurrentList
      end if
      if deployIsValidSDK(tCurrent4) and tCurrent4 is not among the lines of tCurrentList then
         if tCurrentList is not empty then
            put return after tCurrentList
         end if
         put tCurrent4 after tCurrentList
      end if
      if deployIsValidSDK(tCurrent3_2) and tCurrent3_2 is not among the lines of tCurrentList then
         if tCurrentList is not empty then
            put return after tCurrentList
         end if
         put tCurrent3_2 after tCurrentList
      end if
   end if
   
   if tCurrentList is empty then
      get "/Applications/Xcode.app/Contents/Developer"
      if deployIsValidSDK(it) then
         put it after tCurrentList
      end if
      get "/Developer"
      if deployIsValidSDK(it) and it is not among the lines of tCurrentList then
         if tCurrentList is not empty then
            put return after tCurrentList
         end if
         put it after tCurrentList
      end if
   end if
   
   set the cMobileSupport["ios.sdks"] of stack "revPreferences" to tCurrentList
end deployAutoconfigureSDKs

////////////////////////////////////////////////////////////////////////////////

-- Invoked to check the given path is a valid 'new' (>= 3.2) SDK
-- MM-2012-09-18: Updated to support the new SDK list style preference.
-- We now allow XCode versions that have sim support greater tha 4.3.
--
function deployIsValidSDK pSDK
   if pSDK is empty then
      return false
   end if
   
   local tOldFolder, tSDKVersion
   put the defaultfolder into tOldFolder
   set the defaultfolder to pSDK & slash & "Platforms/iPhoneSimulator.platform/Developer/SDKs"
   
   put the files & LF & the folders into tSDKList
   
   // We accept any SDK having an iPhone simulator >= 5.1
   repeat for each line tSDK in tSDKList
      if matchtext(tSDK, "iPhoneSimulator([0-9]\.[0-9]).sdk", tSDKVersion) and \
            tSDKVersion > 5.1 then
         return true
      end if
   end repeat
   
   -- Otherwise it must be invalid
   return false
end deployIsValidSDK

////////////////////////////////////////////////////////////////////////////////

function deploySanatizeSystemVersion
   set the itemDel to "."
   get the systemVersion   
   if the length of item 2 of it is 0 then
      put 00 before item 2 of it
   else if the length of item 2 of it is 1 then
      put 0 before item 2 of it
   end if
   if the length of item 3 of it is 0 then
      put 00 before item 3 of it
   else if the length of item 3 of it is 1 then
      put 0 before item 3 of it
   end if   
   replace "." with empty in it
   return it
end deploySanatizeSystemVersion

////////////////////////////////////////////////////////////////////////////////

