script "revdeploylibraryemscripten"
/* Copyright (C) 2017 LiveCode Ltd.

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

constant kLinuxBrowserList = "Firefox=firefox/firefox,Firefox Beta=firefox_beta/firefox,Firefox Aurora=firefox_aurora/firefox,Firefox Nightly=firefox_nightly/firefox,Chrome Stable=google-chrome-stable/google-chrome-stable,Chrome=google-chrome/google-chrome"
constant kPrivateModeSuffix = "[Private Mode]"

local sBrowsers
local sServerPort
local sHTTPFileSpace

command deployInitialize
   local tBrowserList
   put __deployGetBrowserList() into tBrowserList
   split tBrowserList by return and comma
   put tBrowserList into sBrowsers
   
   repeat for each key tBrowser in tBrowserList
      if __deployBrowserSupportsPrivateMode(tBrowser) then
         put tBrowserList[tBrowser] into sBrowsers[tBrowser && kPrivateModeSuffix]
      end if
   end repeat
   
   put the temporary folder & slash & "livecode-emscripten-builds" into sHTTPFileSpace
   if not (there is a folder sHTTPFileSpace) then
      create folder sHTTPFileSpace
   end if
end deployInitialize

function deployIsAvailable
   get deployListTargets()
   return it is not empty
end deployIsAvailable

function deployListTargets
   local tTargets
   put the keys of sBrowsers into tTargets
   sort tTargets
   if tTargets is not empty then
      put return before tTargets
   end if
   put "Default Browser" before tTargets
   return tTargets   
end deployListTargets

-- Create a local HTTP server using the HTTPd library,
-- build the HTML 5 standalone then launch the local URL
-- for the standalone in the chosen browser.
--
command deployDo pTargetStack, pBrowser
   local tError
   put empty into tError
   
   if tError is empty then
      revStandaloneProgress "Configuring HTTP server..."      
      
      if sServerPort is not empty and sServerPort is not among the lines of the openSockets then
         httpdStop sServerPort
         put empty into sServerPort
      end if
      
      if sServerPort is empty then
         httpdStart "deployEmscriptenNewHTTPRequest", 0
         if the result is not empty or it is not an integer then
            put "Could not start HTTP server" && the result && it into tError
         else
            put it into sServerPort
         end if
      end if
   end if
   
   local tBuildFolder, tStandalonePath
   if tError is empty then
      revStandaloneProgress "Building standalone..."      
      
      local tSettings
      put revSBGetSettings(pTargetStack, true) into tSettings
      
      put sHTTPFileSpace & slash & tSettings["name"] into tBuildFolder
      if there is a folder tBuildFolder then
         revDeleteFolder tBuildFolder
      end if
      create folder tBuildFolder
      
      put urlencode(tSettings["name"]) & slash & urlencode(tSettings["name"]) & ".html" into tStandalonePath      
      
      try
         dispatch "revSaveAsEmscriptenStandalone" to stack "revsaveasemscriptenstandalone" with \
               pTargetStack, tBuildFolder, tSettings
      catch tException
         put "Unable to build app for testing:" && tException into tError
      end try
   end if
   
   if tError is empty then
      revStandaloneProgress "Launching URL..."      
      
      local tPrivateMode
      put pBrowser ends with kPrivateModeSuffix into tPrivateMode
      __deployLaunchURLInBrowser "http://localhost:" & sServerPort & slash & tStandalonePath, sBrowsers[pBrowser], tPrivateMode
      if the result is not empty then
         put "Could not open URL" && the result into tError
      end if
   end if
   
   if tError is not empty and there is a folder tBuildFolder then
      revDeleteFolder tBuildFolder
   end if
   
   if "revStandaloneProgress" is among the lines of the openStacks then
      close stack "revStandaloneProgress"
   end if
   
   return tError
end deployDo

////////////////////////////////////////////////////////////////////////////////

private function __wrapInQuotes pString
   return quote & pString & quote
end __wrapInQuotes

-- Returns a return delimited list of the form <browser>,<path to exe>
-- Use an internal command on OSX, the registery in Windows and a static list on Linux.
-- Not implemented on other platforms - we just use the default browser here.
--
private function __deployGetBrowserList
   local tBrowserList
   
   switch the platform
      case "MacOS"
         _internal list browsers
         put the result into tBrowserList
         break
         
      case "Win32"
         -- According to https://msdn.microsoft.com/en-us/library/dd203067(VS.85).aspx
         -- browsers should register in the registry at HKEY_LOCAL_MACHINE\SOFTWARE\Clients\StartMenuInternet
         --
         local tBrowsers
         put listRegistry("HKEY_LOCAL_MACHINE\SOFTWARE\Clients\StartMenuInternet") into tBrowsers
         repeat for each line tBrowser in tBrowsers
            if tBrowserList is not empty then
               put return after tBrowserList
            end if
            put queryRegistry("HKEY_LOCAL_MACHINE\SOFTWARE\Clients\StartMenuInternet\" & tBrowser & backslash) & comma & \
                  queryRegistry("HKEY_LOCAL_MACHINE\SOFTWARE\Clients\StartMenuInternet\" & tBrowser & "\shell\open\command\") after tBrowserList
         end repeat
         replace quote with empty in tBrowserList
         break
         
      case "Linux"
         -- For linux, we have a list of known browsers.
         -- Check to see if the exe is in the path or installed in the user's home dir.
         --
         local tBrowserArray
         put kLinuxBrowserList into tBrowserArray
         split tBrowserArray by comma and "="
         
         set the itemDel to slash
         repeat for each key tBrowser in tBrowserArray
            get shell("which" && item 1 of tBrowserArray[tBrowser])
            if it is not empty then
               put it into tBrowserList[tBrowser]
            else
               local tUserBrowserPath
               put $HOME & slash & tBrowserArray[tBrowser] into tUserBrowserPath
               if there is a file tUserBrowserPath then
                  put tUserBrowserPath into tBrowserList[tBrowser]
               end if
            end if
         end repeat
         set the itemDel to comma
         
         combine tBrowserList with return and comma
         break
         
      default
         put empty into tBrowserList
         break
   end switch
   
   return tBrowserList
end __deployGetBrowserList

-- Add any exta command line args that we might want to pass to the browser.
-- e.g. javascript consoles, debug modes etc
--
-- Chrome - https://src.chromium.org/viewvc/chrome/trunk/src/chrome/common/chrome_switches.cc
-- Firefox - https://developer.mozilla.org/en-US/docs/Mozilla/Command_Line_Options
-- Opera - http://www.opera.com/docs/switches/
-- Safari doesn't support command line flags. We could potentially use applescript on OS X, though it's somewhat hacky.
--
private function __deployGetBrowserArgs pBrowser
   return empty
end __deployGetBrowserArgs pBrowser

private function __deployBrowserSupportsPrivateMode pBrowser
   return __deployGetBrowserPrivateModeArgs(pBrowser) is not empty
end __deployBrowserSupportsPrivateMode

private function __deployGetBrowserPrivateModeArgs pBrowser
   switch
      case pBrowser contains "chrome"
         return "--incognito"
      case pBrowser contains "iexplore"
         return "-private"
      case pBrowser contains "firefox"
         -- We use the open command on OS X to launch browsers.
         -- This takes care of managing multiple instances - if already running, the exisiting instance will be brought to the foreground with the given URL.
         -- However, this means that if the browser is already open, command line args are lost.
         -- So when we want to pass args, we force start a new instance - most browsers handle this appropriately.
         -- However, firefox complains and exits (when running multiple instances on OS X), which means we can't fully support private mode using this method.
         -- It will only work if firefox is not already running (or we could try a nasty apple script hack but that susceptible to menu/shortcut changes).
         if the platform is "MacOS" then
            return empty
         end if
         return "-private-window"
      case pBrowser contains "opera"
         return "-newprivatetab"
      default
         return empty
   end switch
end __deployGetBrowserPrivateModeArgs

private command __deployLaunchURLInBrowser pURL, pBrowser, pPrivateMode
   if pBrowser is empty then
      launch URL pURL
      return the result
   else
      local tArgs
      put __deployGetBrowserArgs(pBrowser) into tArgs
      if pPrivateMode then
         if tArgs is not empty then
            put space after tArgs
         end if
         put __deployGetBrowserPrivateModeArgs(pBrowser) after tArgs
      end if
      
      switch the platform
         case "MacOS"
            if tArgs is not empty then
               -- If we want to pass any command line args to the browser, we need to force start a new instance using -n
               -- Most browsers appear to handle relaunches in a graceful way.
               put "-n --args " before tArgs
            end if
            return shell("open -a" && __wrapInQuotes(pBrowser) && tArgs && __wrapInQuotes(pURL))
            
         case "Win32"
            local tHideConsoleWindows
            put the hideConsoleWindows into tHideConsoleWindows
            set the hideConsoleWindows to true
            get shell(__wrapInQuotes(pBrowser) && tArgs && __wrapInQuotes(pURL))
            set the hideConsoleWindows to tHideConsoleWindows            
            return it
            
         case "Linux"
            return shell(__wrapInQuotes(pBrowser) &&  tArgs && __wrapInQuotes(pURL))
            
         default
            launch URL pURL
            return the result
      end switch      
   end if
end __deployLaunchURLInBrowser

-- Callback from the HTTP deamon.
-- In this case we just want to serve the requested file
-- if it exists in the temp file space.
-- POST requests to stdio.html are log messages from emscripten.
-- Display these in the message box.
--
on deployEmscriptenNewHTTPRequest pSocketID, pRequest
   local tPath   
   set the itemDel to slash
   repeat for each item tSubPath in pRequest["resource"]
      if tPath is not empty then
         put slash after tPath
      end if
      put urlDecode(tSubPath) after tPath
   end repeat
   set the itemDel to comma
   put sHTTPFileSpace & slash before tPath
   
   if there is a file tPath then
      httpdResponse pSocketID, 200, URL ("binfile:" & tPath)
   else
      set the itemDel to slash
      if the last item pRequest["resource"] is "stdio.html" and pRequest["method"] is "POST" then
         local tContent
         put urlDecode(pRequest["content"]) into tContent
         
         -- Parse and display emscripten log messages in the message box.
         -- Log messages from emscripten appear to be of the form
         -- ^<err|out|pageload|exit>^[count^]<message>
         -- Output a message header then the message body itself
         local tMsgType, tMsgTypeCount, tMsgBody
         put item 2 to -2 of pRequest["resource"] after msg
         if matchText(tContent, "^\^(.+)\^([0-9]+)\^(.*)$", tMsgType, tMsgTypeCount, tMsgBody) then
            put " -" && tMsgType && "#" & tMsgTypeCount & return after msg
            if tMsgBody is not empty then
               put tMsgBody & return after msg
            end if
         else if matchText(tContent, "^\^(.+)\^(.*)$", tMsgType, tMsgBody) then
            put " -" && tMsgType & return after msg
            if tMsgBody is not empty then
               put tMsgBody & return after msg
            end if
         else
            put tContent & return after msg
         end if
         
         httpdResponse pSocketID, 200
      else
         httpdResponse pSocketID, 404
      end if
      set the itemDel to comma
   end if   
end deployEmscriptenNewHTTPRequest
