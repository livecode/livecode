script "revDeployLibrary"

on revLoadLibrary
   insert the script of me into back
end revLoadLibrary

on revUnloadLibrary
   remove the script of me from back
end revUnloadLibrary

////////////////////////////////////////////////////////////////////////////////

-- A stack can be configured to be deployable to any number of different platforms.
-- This presents a usability problem when switching between stacks configured for
-- different platforms.
-- To resolve this, we keep track of the last selected target for each platform and the
-- order in which other platforms were last selected.
-- The 'Test Target' menu disables all targets for which the current stack is not
-- configured for.
-- The current 'Test Target' is determined to be the first platform in the platform choice
-- history for which the stack is configured. This is then used to pick the last chosen
-- target for this platform.

constant kSupportedPlatforms = "iOS Android"

-- The mapping from deploy target to platform
local sDeployTargetPlatforms

-- The currently chosen deploy target for each platform
local sDeployTargets
-- The ordered list of platforms, in order of last choice
local sDeployPlatforms

-- Used to stop DeployAction being executed inside itself
local sDeployActionInProgress

-- This method is invoked by the menu bar when the user elects to test the
-- current stack.
command revIDEDeployAction
   if sDeployActionInProgress then
      exit revIDEDeployAction
   end if
   
   -- Get the stack we are dealing with
   local tStack
   put the mainStack of the topStack into tStack
   
   -- Make sure we save the stack if it needs it
   set the defaultStack to tStack
   if not revSaveCheck(tStack) then
      exit revIDEDeployAction
   end if
   
   -- Get the target
   local tTarget
   put revIDEDeployGetTarget() into tTarget
   
   -- Get the target platform
   local tPlatform
   put sDeployTargetPlatforms[tTarget] into tPlatform
   
   put true into sDeployActionInProgress
   
   -- Invoke the action in the appropriate platform-specific library
   local tError
   if there is a stack ("revDeployLibrary" & tPlatform)  then
      dispatch "deployDo" to stack ("revDeployLibrary" & tPlatform)  with tStack, tTarget
      put the result into tError
   else
      put "Unknown deployment platform - " & tPlatform into tError
   end if
   
   -- Report an error if one occured
   if tError is not empty then
      if revTestEnvironment() then
         return tError
      else
         answer error tError
      end if
   end if
   
   put false into sDeployActionInProgress
end revIDEDeployAction

-- This method is invoked (currently) by the standalone builder to deploy a mobile
-- platform (not currently used as more work is needed to extricate standalone
-- building from the standalone settings main stack).
command revIDEDeployBuild pPlatform, pTargetStack, pOutputFolder
   -- Make sure we are dealing with main stack
   put the mainStack of stack pTargetStack into pTargetStack
   
   -- Dispatch the action to the platform-specific library
   dispatch "deployBuild" to stack ("revDeployLibrary" & pPlatform) with pTargetStack, pOutputFolder
end revIDEDeployBuild

-- This method is invoked by the menu bar to construct the current list of targets.
-- It returns an array, one element per platform; each element consisting of the list
-- of possible targets.
function revIDEDeployListTargets
   local tTargets
   repeat for each word tPlatform in kSupportedPlatforms
      dispatch function "deployListTargets" to stack ("revDeployLibrary" & tPlatform) 
      if it is "handled" and the result is not empty then
         put the result into tTargets[tPlatform]
      end if
   end repeat
   return tTargets
end revIDEDeployListTargets

-- This method is invoked by the menu bar when the user selects a different deploy
-- target.
command revIDEDeploySetTarget pTarget
   local tPlatform
   put sDeployTargetPlatforms[pTarget] into tPlatform
   delete item itemOffset(tPlatform, sDeployPlatforms) of sDeployPlatforms
   put tPlatform, sDeployPlatforms into sDeployPlatforms
   put pTarget into sDeployTargets[tPlatform]
   set the cDeployPlatforms of stack "revPreferences" to sDeployPlatforms
   set the cDeployTargets of stack "revPreferences" to sDeployTargets
   revIDEDeployUpdate
end revIDEDeploySetTarget

-- This method returns the current target of the 'Deploy' action.
function revIDEDeployGetTarget
   if the mode of the topStack is 1 then
      local tStack
      put the mainStack of the topStack into tStack
      repeat for each item tPlatform in sDeployPlatforms
         if the cRevStandaloneSettings[tPlatform] of stack tStack then
            return sDeployTargets[tPlatform]
         end if
      end repeat
   end if
   
   return empty
end revIDEDeployGetTarget

-- This method returns true if deployment is possible with the given platform to the
-- current top stack
function revIDEDeployIsPossible pPlatform
   if the mode of the topStack is not 1 then
      return false
   end if
   
   local tStack
   put the mainStack of the topStack into tStack
   return the cRevStandaloneSettings[pPlatform] of stack tStack
end revIDEDeployIsPossible

-- This method updates the enable state of the 'Test' button on the menubar based on
-- the current topStack.
command revIDEDeployUpdate
   -- Enable the deploy button appropriately
   set the enabled of button "Deploy" of stack "revMenuBar" to revIDEDeployGetTarget() is not empty
end revIDEDeployUpdate

-- This method is called on startup to initialize the deployment library. This includes
-- attempting to auto-configure any SDKs and such if necessary.
command revIDEDeployInitialize
   repeat for each word tPlatform in kSupportedPlatforms
      dispatch command "deployInitialize" to stack ("revDeployLibrary" & tPlatform) 
   end repeat
   
   local tDeployPlatforms, tDeployTargets
   put the cDeployPlatforms of stack "revPreferences" into tDeployPlatforms
   put the cDeployTargets of stack "revPreferences" into tDeployTargets
   if tDeployPlatforms is not empty then
      put tDeployPlatforms into sDeployPlatforms
      put tDeployTargets into sDeployTargets
   else if the cSimulatorFamily of stack "revPreferences" is not empty then
      put "ios," into sDeployPlatforms
      put the cSimulatorFamily of stack "revPreferences" && "Simulator" && the cSimulatorVersion of stack "revPreferences" into sDeployTargets["ios"] 
   end if
end revIDEDeployInitialize

on revIDEDeployTargetsChanged
   put empty into sDeployTargetPlatforms
   repeat for each word tPlatform in kSupportedPlatforms
      dispatch function "deployListTargets" to stack ("revDeployLibrary" & tPlatform) 
      if it is "handled" and the result is not empty then
         repeat for each line tTarget in the result
            put tPlatform into sDeployTargetPlatforms[tTarget]
         end repeat
      end if
   end repeat
end revIDEDeployTargetsChanged

////////////////////////////////////////////////////////////////////////////////
//
//  Utility functions used by mobile save as standalone implementations.
//

local sStackFiles

function revIDEDeployRelativeStackFilesList pStack
  local tLine,tChars,tStackFiles,tMainStackPath
  put revIDEDeployGetStackFilesList(pStack) into tStackFiles["full"]
  set the itemDel to "/"
  put item 1 to -2 of line 1 of tStackFiles["full"] & "/" into tMainStackPath
  put the number of chars of tMainStackPath into tChars
  repeat for each line tLine in tStackFiles["full"]
    if char 1 to tChars of tLine = tMainStackPath then delete char 1 to tChars of tLine
    put tLine & cr after tStackFiles["relative"]
  end repeat
  delete char -1 of tStackFiles["relative"]
  return tStackFiles
end revIDEDeployRelativeStackFilesList

private function revIDEDeployGetStackFilesList pStack
  put "" into sStackFiles
  put the effective fileName of stack pStack into sStackFiles
  revIDEDeployRecursiveStackFileList pStack
  return sStackFiles
end revIDEDeployGetStackFilesList

private command revIDEDeployRecursiveStackFileList pStack
  local tLine,tFileName
  set the itemDel to "/"
  put item 1 to -2 of the fileName of stack pStack&"/" into tFileName
  set the itemDel to ","
  repeat for each line tLine in the stackFiles of stack pStack
    if there is not a stack item 2 of tLine then put tFileName before item 2 of tLine
    if there is a stack item 2 of tLine then
      if the effective fileName of stack item 2 of tLine is not among the lines of sStackFiles then 
        put cr& the effective fileName of stack item 2 of tLine after sStackFiles
        revIDEDeployRecursiveStackFileList item 2 of tLine
      end if
    end if
  end repeat
  repeat for each line tLine in the substacks of stack pStack
    revIDEDeployRecursiveStackFileList tLine
  end repeat
end revIDEDeployRecursiveStackFileList

////////////////////////////////////////////////////////////////////////////////
//
//  Preferences require access to various specific aspects of deployment config, so these
//  calls provide this in a sane manner.
//

function revIDEDeployIOSIsValidSDK pVersion, pPath
   dispatch function "deployIsValidSDK" to stack "revDeployLibraryiOS" with pVersion, pPath
   return the result
end revIDEDeployIOSIsValidSDK

function revIDEDeployIOSGetSimulatorVersions
   dispatch function "deployGetSimulatorVersions" to stack "revDeployLibraryiOS"
   return the result
end revIDEDeployIOSGetSimulatorVersions

function revIDEDeployIOSGetSDKVersions
   dispatch function "deployGetSDKVersions" to stack "revDeployLibraryiOS"
   return the result
end revIDEDeployIOSGetSDKVersions

command revIDEDeployIOSUpdateSimulators
   dispatch "deployUpdateSimulators" to stack "revDeployLibraryiOS"
end revIDEDeployIOSUpdateSimulators

command revIDEDeployIOSUpdateSDKs
   dispatch "deployUpdateSDKs" to stack "revDeployLibraryiOS"
end revIDEDeployIOSUpdateSDKs

function revIDEDeployIOSGetDeviceSDK pVersion
   dispatch function "deployGetDeviceSDK" to stack "revDeployLibraryiOS" with pVersion
   return the result
end revIDEDeployIOSGetDeviceSDK

function revIDEDeployIOSSanatizeSystemVersion
   dispatch function "deploySanatizeSystemVersion" to stack "revDeployLibraryiOS"
   return the result
end revIDEDeployIOSSanatizeSystemVersion

////////////////////////////////////////////////////////////////////////////////
//
//  Preferences require access to various specific aspects of deployment config, so these
//  calls provide this in a sane manner.
//

function revIDEDeployAndroidIsValidSDK pPath
   dispatch function "deployIsValidSDK" to stack "revDeployLibraryAndroid" with pPath
   return the result
end revIDEDeployAndroidIsValidSDK

function revIDEDeployAndroidIsValidJDK pPath
   dispatch function "deployIsValidJDK" to stack "revDeployLibraryAndroid" with pPath
   return the result
end revIDEDeployAndroidIsValidJDK

function revIDEDeployAndroidGetJDK
   dispatch function "deployGetJDK" to stack "revDeployLibraryAndroid"
   return the result
end revIDEDeployAndroidGetJDK

command revIDEDeployAndroidInitialize
   dispatch "deployInitialize" to stack "revDeployLibraryAndroid"
end revIDEDeployAndroidInitialize

////////////////////////////////////////////////////////////////////////////////
