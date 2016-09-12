script "com.livecode.script-library.simctl"
/**

Library for interfacing with iOS simulator using simctl

Requires:

- mergJSON (external)

*/

constant kPathToSimctl = "Contents/Developer/usr/bin/simctl"

-- listing
constant kListDevices = "${simctl} list -j devices"
constant kListDeviceTypes = "${simctl} list -j devicetypes"
constant kListDeviceRuntimes = "${simctl} list -j runtimes"

-- boot
constant kBootDevice = "${simctl} boot ${udid}"

-- create
constant kCreateDevice = "${simctl} create ${name} ${devicetypeidentifier} ${runtimeidentifier}"

-- delete
constant kDeleteDevice = "${simctl} delete ${udid}"

-- erase
constant kEraseDevice = "${simctl} erase ${udid}"

-- install
constant kInstallAppOnDevice = "${simctl} install ${udid} ${path}"

-- launch
constant kLaunchAppOnDevice = "${simctl} launch ${udid} ${applicationid} ${args}"

local sDevices
local sSimctl

/**

May be called more than once with different paths. Calling again will
with the same path will cause all the simulators to be reset for that
Xcode version.

Example:
simctlSetXcodePath "/Applications/Xcode.app"

Parameters:
pXcodePath (string): The full path to Xcode.app

The result:
An error string if an error occured

*/

command simctlSetXcodePath pXcodePath
   if there is not a folder pXcodePath then
      return "App does not exist" && pXcodePath for error
   end if
   
   local tVars
   put pXcodePath & slash & kPathToSimctl into tVars["simctl"]
   
   if there is not a file tVars["simctl"] then
      return "simctl could not be found" && tVars["simctl"] for error
   end if
   
   -- clear existing settings
   delete variable sDevices[tVars["simctl"]]
   
   local tDevices
   put __JSONCommand(kListDevices, tVars) into tDevices
   
   local tDeviceRuntimes
   put __JSONCommand(kListDeviceRuntimes, tVars) into tDeviceRuntimes
   
   -- remove unavailable devices
   local tIndex
   repeat for each key tIndex in tDeviceRuntimes["runtimes"]
      local tRuntime
      put tDeviceRuntimes["runtimes"][tIndex]["name"] into tRuntime
      
      put tVars["simctl"] into sSimctl[tRuntime]
      
      local tDevice
      repeat for each element tDevice in tDevices["devices"][tRuntime]
         put tDevice["udid"] into sDevices[tRuntime][tDevice["name"]]
      end repeat
      
   end repeat
   return empty for error
end simctlSetXcodePath

/**

Get the available devices

Returns:
An array of available devices in the form:
array[<sdk>][<devicetype>] -> udid

*/
function simctlAvailableDevices
   return sDevices
end simctlAvailableDevices

/**

Install an app on a simulator

Parameters:
- pSDK (string): An SDK as returned by `simctlAvailableDevices` e.g `iOS 10.0`
- pDeviceType (String): A device type as returned by `simctlAvailableDevices` e.g `iPhone 6 Plus`
- pAppPath (String): The full path to an app to install

*/
command simctlInstallAppOnDevice pSDK, pDeviceType, pAppPath
   local tCommand
   put kInstallAppOnDevice into tCommand
   
   if there is not a file sSimctl[pSDK] then
      return "simctl could not be found" && sSimctl[pSDK] for error
   end if
   
   replace "${simctl}" with sSimctl[pSDK] in tCommand
   
   if sDevices[pSDK][pDeviceType] is empty then
      return "device type could not be found" for error
   end if
   
   replace "${udid}" with sDevices[pSDK][pDeviceType] in tCommand
   
   replace "${path}" with quote & pAppPath & quote in tCommand
   
   get shell(tCommand)
   
   --!TODO determine success from results
end simctlInstallAppOnDevice

/**

Launch an app on a simulator

Parameters:
- pSDK (string): An SDK as returned by `simctlAvailableDevices` e.g `iOS 10.0`
- pDeviceType (String): A device type as returned by `simctlAvailableDevices` e.g `iPhone 6 Plus`
- pAppID (String): An app id of an already installed app
- pArgs (String): Any additional preformated arguments to launch the app with

*/
command simctlLaunchAppOnDevice pSDK, pDeviceType, pAppID, pArgs
   local tCommand
   put kLaunchAppOnDevice into tCommand
   
   if there is not a file sSimctl[pSDK] then
      return "simctl could not be found" && sSimctl[pSDK] for error
   end if
   
   replace "${simctl}" with sSimctl[pSDK] in tCommand
   
   if sDevices[pSDK][pDeviceType] is empty then
      return "device type could not be found" for error
   end if
   
   replace "${udid}" with sDevices[pSDK][pDeviceType] in tCommand
   
   replace "${applicationid}" with pAppID in tCommand
   
   replace "${args}" with pArgs in tCommand
   
   open process tCommand for neither
   
   --!TODO determine success if possible
end simctlLaunchAppOnDevice

private function __JSONCommand pCommand, pVars
   local tVar
   repeat for each key tVar in pVars
      replace "${" & tVar & "}" with pVars[tVar] in pCommand
   end repeat
   
   local tResult
   put shell(pCommand) into tResult
   
   -- try and strip out stderr...
   repeat until char 1 of tResult is "{" or tResult is empty
      delete char 1 of tResult
   end repeat
   
   if tResult is empty then
      return empty
   else
      return __JSONToArray(tResult)
   end if
end __JSONCommand

private function __JSONToArray pJSON
   local tArray,tKeys
   repeat for each line tKey in mergJSONDecode(pJSON,"tArray")
      put __JSONToArray(tArray[tKey]) into tArray[tKey]
   end repeat
   return tArray
end __JSONToArray
