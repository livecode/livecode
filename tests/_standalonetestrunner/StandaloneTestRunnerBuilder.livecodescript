script "StandaloneTestRunnerBuilder"
/*
Copyright (C) 2017 LiveCode Ltd.

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

local sResults
local sExitCode
local sRunStartTime
local sServerRoot
local sServerPort

-- timeout runs after 15 mins
constant kRunTimeout = 900

constant kMainstack = "StandaloneTestRunnerMainstack"

-- we re-use the deploy libraries where it's possible to do so
-- at the moment there is no emscripten deploy library and
-- the desktop deploy library can't be used because there's no
-- way to get the output and result code at present
constant kDeployPlatforms = "iOS,Android"

local sDidStartup = false
local sBuilderIsSetup = false

on startup
   local tTarget
   if the target begins with "card" then
      put the long id of the owner of the target into tTarget
   else
      put the long id of the target into tTarget
   end if
   
   if tTarget is the long id of me then 
      send "__Start" to me in 0
   end if
end startup

on openStack
   if the long id of the target is the long id of this card of me then
      send "__Start" to me in 0
   end if
end openStack

on __Start
   if sDidStartup then
      exit __Start
   end if
   put true into sDidStartup
   
   local tLogInfo
   if the platform is among the items of "Linux,MacOS" then
      put shell("tput sgr0") into tLogInfo["normal"]
      put shell("tput bold") into tLogInfo["bold"]
      put shell("tput setaf 1") into tLogInfo["red"]
      put shell("tput setaf 2") into tLogInfo["green"]
   end if
   
   if "development" is not in the environment then
      __Help
      write tLogInfo["bold"] & tLogInfo["red"] & "You must use the development engine to run this stack" & tLogInfo["normal"] & return to stderr
      quit 1 
   end if
   
   set the itemDelimiter to slash
   
   local tPath
   put the filename of me into tPath
   put "extensions/script-libraries/getopt/getopt.livecodescript" into item -3 to -1 of tPath
   start using stack tPath
   
   constant kOptions = "h,help v,verbose p,platform= n,noui a,all t,target= profile="
   
   local tArgs
   if the environment contains "development" then
      repeat with X = 1 to $#
         do "put $" & X & " into tArgs[" & X & "]"
      end repeat
   else
      put the commandArguments into tArgs
      delete variable tArgs[1]
   end if
   
   local tOptions
   put GetOpt(kOptions, tArgs) into tOptions
   
   if "help" is in the keys of tOptions["options"] then
      __Help
      quit 0
   end if
   
   if tOptions["options"]["platform"] is empty and \
         "all" is not in the keys of tOptions["options"] then
      if the number of elements of tOptions["options"] is 0 then
         -- we get here if we are just opening the stack in the IDE
         return empty
      else
         __Help
         write tLogInfo["bold"] & tLogInfo["red"] & "You must specify a build platform or all" & tLogInfo["normal"] & return to stderr
         quit 1
      end if
   end if
   
   local tPlatforms
   if "all" is in the keys of tOptions["options"] then
      put revSBAllPlatforms() into tPlatforms
      split tPlatforms by comma as set
      
      -- fiddle with the build platforms for 64 bit
      if tPlatforms["MacOSX"] then
         delete variable tPlatforms["MacOSX"]
         put true into tPlatforms["MacOSX x86-64"]
      end if
      put true into tPlatforms["Linux x64"]
   else
      put tOptions["options"]["platform"] into tPlatforms
      split tPlatforms by comma as set
   end if
   
   __SetupBuilder
   
   local tDeployTargets
   put revIDEDeployListTargets() into tDeployTargets
   
   local tPlatform, tTargets, tVersion
   repeat for each key tPlatform in tPlatforms
      put empty into sExitCode
      put empty into sResults
      
      put empty into tTargets
      put empty into tVersion
      
      if tPlatform is among the items of kDeployPlatforms then
         put tOptions["options"]["target"] into tTargets
         if tTargets is empty then
            if "all" is among the lines of the keys of tOptions["options"] then
               put tDeployTargets[tPlatform] into tTargets
            else
               put revIDEDeployGetTarget() into tTargets
               if tTargets is empty then
                  put line 1 of tDeployTargets[tPlatform] into tTargets
               end if
            end if
         else
            -- allow comma-delimited list of targets
            replace comma with return in tTargets
         end if
      end if
      
      if tTargets is empty then
         put return after tTargets
      end if
      
      local tTarget
      repeat for each line tTarget in tTargets
         -- allow specifying targets from different platforms
         if tTarget is not empty and \
               tTarget is not among the lines of tDeployTargets[tPlatform] then
            next repeat
         end if
         
         put tTarget into tOptions["options"]["target"]
         
         local tHaveEngine
         put false into tHaveEngine
         
         __ConfigureMainstackForPlatform tPlatform, tOptions["options"]
         
         switch word 1 of tPlatform
            case "MacOSX"
            case "Linux"
            case "Windows"
               put there is a file revSBEnginePath(tPlatform) into tHaveEngine
               break
            case "iOS"
               local tBuildFor
               if tTarget contains "Simulator" then
                  put word 2 to -1 of tTarget into tBuildFor
               else
                  put "Device" into tBuildFor
               end if
               put word 3 of tPlatform into tVersion
               delete word 2 to -1 of tPlatform
               dispatch function "revMobileRuntimeFolder" to stack "revSaveAsIOSStandalone" with tBuildFor, tVersion
               if the result is not empty then
                  dispatch function "mapFilePath" to stack "revSaveAsIOSStandalone" with the result & slash & "Standalone"
                  put there is a file the result into tHaveEngine
               end if
               break
            case "Android"
               dispatch function "revMobileRuntimeFolder" to stack "revSaveAsAndroidStandalone" with tPlatform
               if the result is not empty then
                  dispatch function "mapFilePath" to stack "revSaveAsAndroidStandalone" with the result & slash & "Standalone"
                  put there is a file the result into tHaveEngine
               end if
               break
            case "Emscripten"
               dispatch function "revGetEmscriptenFiles" to stack "revSaveAsEmscriptenStandalone"
               local tFiles
               put the result into tFiles
               put there is a file tFiles[1] into tHaveEngine
               break
         end switch
         
         if not tHaveEngine then
            write tLogInfo["bold"] & tLogInfo["red"] & "No engine for" && tPlatform && tTarget & tLogInfo["normal"] & return to stderr
         else
            write tLogInfo["bold"] & tLogInfo["green"] & "Build and run" && tPlatform && tTarget & tLogInfo["normal"] & return to stdout
            
            __BuildAndRun tPlatform, tOptions["options"]
            local tResult
            put the result into tResult
            
            if sResults is empty then
               put tResult into sResults
            end if
            
            if "verbose" is in the keys of tOptions["options"] or $V is 1 then
               write sResults & return to stdout
            else
               write tResult & return to stdout
            end if
            
            if sExitCode is not an integer or sExitCode is 0 then 
               next repeat
            end if
            
            __CleanupMainstack
            __CleanupBuilder
            
            quit sExitCode
         end if
      end repeat
   end repeat
   
   __CleanupMainstack
   __CleanupBuilder
   
   quit 0
end __Start

private command __Help
   local tHelp
   put "Standalone Test Runner Builder" & return & return & \
         "-h,--help       You're reading it" & return & \
         "-p,--platform   Specify the build platform to build and run e.g --platform" && quote & "MacOSX x86-64,Emscripten" & quote & return & \
         "-a,--all        Build and run all platforms" & return & \
         "-t,--target     Specify the build target name for mobile builds. If not specified and the all flag is set" & return & \
         "                then all targets will be run. e.g --target" && quote & "iPad Simulator 10.3" & quote & return & \
         "--profile       Specify the iOS provisioning profile UUID. Only necessary for device builds." & return & \
         "-a,--all        Build and run all platforms" & return & \
         "-v,--verbose    Output all results instead of just failures" & return into tHelp
   
   write tHelp to stdout
end __Help

on errorDialog pError
   if not sDidStartup then
      pass errorDialog
   end if
   
   write pError & return to stderr
   quit 1
end errorDialog

/**

Build and run on a single platform

Description:
Use this command if you only want to build and run a single platform
and don't want to do so from the command line.

Parameters:
pBuildPlatform (string):
A standalone builder builder build platform string

pOptions (optional array):
Extra options for the build or run. Eg noui -> true will add the -ui flag to
desktop runs.
- `noui` - boolean
- `target` - a deploy target
- `profile` - a provisioning profile uuid for iOS

*/

command BuildAndRun pBuildPlatform, pOptions
   __SetupBuilder
   
   __ConfigureMainstackForPlatform pBuildPlatform, pOptions
   
   put empty into sExitCode
   put empty into sResults
   
   local tResult
   __BuildAndRun pBuildPlatform, pOptions
   put the result into tResult
   
   __CleanupMainstack
   __CleanupBuilder
   
   -- use the result here so that people can use dispatch
   return tResult
end BuildAndRun

private command __SetupMainstack
   set the itemDelimiter to slash
   
   local tFilename
   put item 1 to -2 of the filename of me & slash & "StandaloneTestRunner.livecodescript" into tFilename
   
   clone stack tFilename
   set the name of it to kMainstack
   set the scriptOnly of stack kMainstack to false
   set the filename of stack kMainstack to the tempName
   set the uErrorMap of stack kMainstack to TestBuildErrorMap("executionerrors.h", "EE", "UNDEFINED")
   
   local tTestsFolder
   put item 1 to -3 of the filename of me &"/*" into tTestsFolder
   
   set the cRevStandaloneSettings["files"] of stack kMainstack to tTestsFolder
   set the cRevStandaloneSettings["inclusions"] of stack kMainstack to "select"
   set the cRevStandaloneSettings["brushes"] of stack kMainstack to true
   set the cRevStandaloneSettings["cursors"] of stack kMainstack to true
end __SetupMainstack

private command __ConfigureMainstackForPlatform pBuildPlatform, pOptions
   if there is not a stack kMainstack then
      __SetupMainstack
   end if
   
   local tInclusions, tAllInclusions
   local tIndex = 1
   put revSBAvailableInclusions() into tAllInclusions
   
   set the itemDelimiter to comma
   repeat for each element tElement in tAllInclusions
      if tElement["id"] is "com.livecode.library.remotedebugger" then
         next repeat
      end if
      
      -- BUG 19928
      if pBuildPlatform is not "Android" and "android" is in tElement["id"] then
         next repeat
      end if
      
      repeat for each item tPlatform in tElement["platforms"]
         if pBuildPlatform begins with tPlatform then
            put tElement into tInclusions[the number of elements of tInclusions + 1]
            next repeat
         end if
      end repeat
   end repeat
   revSBUpdateInclusions tInclusions, ,kMainstack
   
   local tSettingsA
   put the customProperties["cRevStandaloneSettings"] of stack kMainstack into tSettingsA
   
   set the itemDelimiter to comma
   repeat for each item tPlatform in revSBDesktopTargets(),revSBAdditionalTargets()
      put tPlatform is pBuildPlatform into tSettingsA[tPlatform]
   end repeat
   
   switch pBuildPlatform
      case "iOS"
         put pOptions["profile"] into tSettingsA["ios,profile"]
         put "com.livecode.tests.standalonerunner" into tSettingsA["ios,bundle id"]
         break
      case "Android"
         put true into tSettingsA["android,application permissions"]["INTERNET"]
         put "com.livecode.tests.standalonerunner" into tSettingsA["android,identifier"]
         break
   end switch
   
   set the customProperties["cRevStandaloneSettings"] of stack kMainstack to tSettingsA
   
   if pBuildPlatform is among the items of "iOS,Android" then
      set the uCallbackURL of stack kMainstack to "http://localhost:" & sServerPort & "/stdio.html"
   end if
   save stack kMainstack
end __ConfigureMainstackForPlatform
   
private command __CleanupMainstack
   if there is a stack kMainstack then
      delete file the filename of stack kMainstack
      delete stack kMainstack
   end if
end __CleanupMainstack

private command __BuildAndRun pBuildPlatform, pOptions
   put the seconds into sRunStartTime
   
   if pBuildPlatform is among the items of kDeployPlatforms then
      local tTarget
      if pOptions["target"] is empty then
         local tTargets
         put revIDEDeployListTargets() into tTargets
         
         put revIDEDeployGetTarget() into tTarget
         if tTarget is empty or tTarget is not among the lines of tTargets[pBuildPlatform] then
            put line 1 of tTargets[pBuildPlatform] into tTarget
         end if
      else
         put pOptions["target"] into tTarget
      end if
      dispatch "deployDo" to stack ("revDeployLibrary" & pBuildPlatform) with kMainstack, tTarget
      
      if the result is not empty then
         return the result for error
      end if
      
      __Wait
      break
   else
      local tTargetFolder
      put the tempName into tTargetFolder
      create folder tTargetFolder
      
      local tError, tStandalonePaths
      revDoSaveAsStandalone kMainstack, tTargetFolder, ,tStandalonePaths
      
      if the result is not empty then
         return the result for error
      end if
      
      local tPath
      put tStandalonePaths[pBuildPlatform] into tPath
      
      if pBuildPlatform is "Emscripten" then
         __RunEmscripten tPath
      else
         -- escape the executable path
         switch the platform
            case "Linux"
               if $TRAVIS is true then
                  put true into pOptions["noui"]
               end if
            case "MacOS"
               replace space with "\ " in tPath
               break
            case "Win32"
               put shortFilePath(tPath) into tPath
               break
         end switch
         
         -- run in noui on desktop if requested
         if pOptions["noui"] then
            put " -ui" after tPath
         end if
         put shell(tPath) into sResults
         put the result into sExitCode
      end if
   end if
   
   local tResults
   put sResults into tResults
   
   revDeleteFolder tTargetFolder
   
   if sExitCode is 3 then
      return tResults & return & "TIMEOUT" for error
   else if sExitCode is 2 then
      -- tests failed
      filter tResults with "not ok -*"
      return tResults for error
   else if sExitCode is 1 then
      -- execution error
      filter tResults with "[0-9]*,*"
      return tResults for error
   else
      return empty for value
   end if
end __BuildAndRun

private command __SetupBuilder
   if sBuilderIsSetup then
      exit __SetupBuilder
   end if
   
   __LoadIDE
   
   put true into sBuilderIsSetup
   
   set the hideConsoleWindows to true
   
   local tTestLib
   set the itemdelimiter to slash
   put item 1 to -3 of the filename of me & slash & "_testlib.livecodescript" into tTestLib
   start using stack tTestLib
   
   -- load httpd if necessary
   if there is not a stack "com.livecode.library.httpd" then
      local tFileName
      put the filename of me into tFilename
      set the itemDelimiter to slash
      put "extensions/script-libraries/httpd/httpd.livecodescript" into item -3 to -1 of tFilename
      get the name of stack tFilename
   end if
   
   if the long id of stack "com.livecode.library.httpd" is not among the lines of the backScripts then
      insert the script of stack "com.livecode.library.httpd" into back
   end if
   
   httpdStart "__StdioCallback"
   put it into sServerPort
end __SetupBuilder

private command __CleanupBuilder
   httpdStop
   
   put false into sBuilderIsSetup
end __CleanupBuilder

private command __Run pBuildPlatform, pOptions, pPath
   
   -- escape the executable path
   switch the platform
      case "Linux"
         if $TRAVIS is true then
            put true into pOptions["noui"]
         end if
      case "MacOS"
         replace space with "\ " in pPath
         break
      case "Win32"
         put shortFilePath(pPath) into pPath
         break
   end switch
   
   switch word 1 of pBuildPlatform
      case "Windows"
         set the hideConsoleWindows to true
      case "MacOSX"
      case "Linux"
         -- run in noui on desktop if requested
         if pOptions["noui"] then
            put " -ui" after pPath
         end if
         put shell(pPath) into sResults
         put the result into sExitCode
         break
      case "Emscripten"
         __RunEmscripten pPath
         break
      default
         return "Running" && pBuildPlatform && "not implemented yet" for error
   end switch
   
   local tResults
   put sResults into tResults
   
   if sExitCode is 2 then
      -- tests failed
      filter tResults with "not ok -*"
      return tResults for error
   else if sExitCode is 1 then
      -- execution error
      filter tResults with "[0-9]*,*"
      return tResults for error
   else
      return empty for value
   end if
end __Run

local sWaitTimeoutMsg

private command __Wait
   send "__WaitTimeout" to me in kRunTimeout seconds
   put the result into sWaitTimeoutMsg
   wait until sExitCode is an integer with messages
   cancel sWaitTimeoutMsg
end __Wait

on __WaitTimeout
   put 3 into sExitCode
end __WaitTimeout

private command __RunEmscripten pPath
   put pPath into sServerRoot
   launch url ("http://localhost:" & sServerPort & "/StandaloneTestRunnerMainstack.html")
   __Wait
end __RunEmscripten

on __StdioCallback pSocketID, pRequest
   if there is a file (sServerRoot & pRequest["resource"]) then
      local tData
      put url ("binfile:" & sServerRoot & pRequest["resource"]) into tData
      httpdResponse pSocketID, 200, tData
   else if pRequest["resource"] is "/stdio.html" then
      local tMsg
      put urlDecode(pRequest["content"]) into tMsg
      delete char 1 of tMsg
      split tMsg by "^"
      switch tMsg[1]
         case "out"
            put tMsg[3] & return after sResults
            break
         case "err"
            if tMsg[3] is -1 then
               put 1 into sExitCode
            else
               put tMsg[3] & return after sResults
            end if
            break
         case "exit"
            put tMsg[2] into sExitCode
            break
         case "pageload"
            break
      end switch
      httpdResponse pSocketID, 200
   else
      httpdResponse pSocketID, 404
   end if
end __StdioCallback

private command __LoadIDE
   if there is a stack "revidelibrary" and \
         the long id of stack "revidelibrary" is among the lines of the backScripts then
      exit __LoadIDE
   end if
   
   -- Compute the filename of the home stack
   local tFilename
   put the filename of me into tFilename
   
   set the itemDelimiter to slash
   put "ide/Toolset/home.livecodescript" into item -3 to -1 of tFilename
   
   local tStackName
   -- load home stack
   put the name of stack tFilename into tStackName
   
   -- Set the 'test environment' to true
   dispatch "revSetTestEnvironment" to stack tStackName with true
   	
   insert the script of tStackName into back
   	
   dispatch "startup" to tStackName	
   	
   open tStackName
end __LoadIDE
