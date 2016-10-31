script "com.livecode.script-library.oauth2"
/*
Copyright (C) 2016 LiveCode Ltd.

This file is part of LiveCode.

LiveCode is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License v3 as published by the Free
Software Foundation.

LiveCode is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with LiveCode.  If not see <http://www.gnu.org/licenses/>. 
*/

on revLoadLibrary
   if the target is not me then
      pass revLoadLibrary
   end if
   
   insert the script of me into back
   
   if the environment contains "development" then
      set the _ideoverride of me to true
      revSBAddAvailableLibrary the short name of me, "OAuth2"
      revSBAddDependencyForInclusion "scriptLibraries", "OAuth2", "externals", "mergJSON"
   end if
end revLoadLibrary

on revUnloadLibrary
   if the target is not me then
      pass revUnloadLibrary
   end if
   
   remove the script of me from back
end revUnloadLibrary

/**

Present an authorization dialog for any web service that supports OAuth2 Authorization Code Flow 

Requires mergJSON

*/

constant kRedirectURL = "http://localhost"

local sUniqueRefs

/**

Present an authorization dialog for any web service that supports OAuth2 Authorization Code Flow

Description:
In order to handle the redirect the library accepts socket connections on localhost on a
configurable port. The redirect uri configured when setting up your application with the
web service should be `http://localhost:port` where `port` is the port that can be
configured with `OAuth2Set "port", port`. The default port is 49152 and it is recommended
to use the range 49152-65535.

> **Warning:** The client secret should be kept securely when distributing an application in
> order to protect your application from malicious use. The recommended way to do this is to
> include the client secret into a script in a password protected stack. If that is not possible
> allow users to configure their own application with the web service and enter their own
> client id and secret into a preference instead of distributing your client id and  secret.

Example:
constant kAuthURL = "https://slack.com/oauth/authorize"
constant kTokenURL = "https://slack.com/api/oauth.access"
constant kClientID = "XXXXXXXXX.XXXXXXXX"
constant kClientSecret = "XXXXXXXXXXXXXXXXXXXXX"
constant kScopes = "incoming-webhook"

OAuth2 kAuthURL, kTokenURL, kClientID, kClientSecret, kScopes, 54303
if the result is not empty then
   answer error "Not authorized!" 
else
   local tAuth
   put it into tAuth
   local tMessage
   ask question "What do you want to send?"
   if it is empty then
      exit mouseUp
   end if
   
   put it into tMessage["text"]
   put ArrayToJSON(tMessage) into tMessage
   
   set the httpHeaders to "Content-type: application/json" & \
         return & "Authorization: token " & sAuth["access_token"]
   post tMessage to url tAuth["incoming_webhook"]["url"]
end if

Parameters:
pAuthURL (string): The URL to present for the authorization page. This can be obtained from the
API documentation of the service being authorized.

pTokenURL (string): The URL to obtain the authorization token from once an authorization code is
sent to the redirect uri. This can be obtained from the API documentation of the service being
authorized.

pClientID (string): The application client ID obtained when setting up your application with
the web service.

pClientSecret (string): The application client secret obtained when setting up your application with
the web service.

pScopes (string): A comma delimited list of authorization scopes. Valid scopes will be found 
in the API documentation of the service being authorized.

pPort (optional integer): The port to use for the redirect uri. Default value is 49152.

pParams (optional array): An array of additional key -> value pairs of extra parameters to be sent to
the authorization url. Some services implement additional options that require extra parameters.

The result:
An error string if an error occured during authorization

It:
An array containing the parsed JSON data returned by the token url

*/

command OAuth2 pAuthURL, pTokenURL, pClientID, pClientSecret, pScopes, pPort, pParams
   local tPort = 49152
   
   if pPort is an integer then
      put pPort into tPort
   end if
   
   local tURL
   put pAuthURL & "?response_type=code" into tURL
   
   put "&client_id=" & urlEncode(pClientID) after tURL
   
   replace space with "%20" in pScopes
   put "&scope=" & urlEncode(pScopes) after tURL
   
   local tUniqueRef
   put uuid() into tUniqueRef
   put "&state=" & tUniqueRef after tURL
   
   local tKey
   repeat for each key tKey in pParams
      replace space with "%20" in pParams[tKey]
      put "&" & tKey & "=" & urlEncode(pParams[tKey]) after tURL
   end repeat
   
   put "&redirect_uri=" & urlEncode(kRedirectURL & ":" & tPort & "/") after tURL
   
   local tBrowserRect
   if the environment is not "mobile" then
      reset the templateStack
      create invisible stack tUniqueRef
      set the title of stack tUniqueRef to "Authenticate"
      
      local tOldDefault
      put the defaultStack into tOldDefault
      set the defaultStack to tUniqueRef
      
      set the width of stack tUniqueRef to 640
      set the height of stack tUniqueRef to 600
      put 0,0,640,600 into tBrowserRect
   else
      put the effective working screenrect into tBrowserRect
   end if
   
   create widget tUniqueRef as "com.livecode.widget.browser"
   set the rect of widget tUniqueRef to tBrowserRect
   set the url of widget tUniqueRef to tURL
   
   local tOldInterface
   put the defaultNetworkInterface into tOldInterface
   set the defaultNetworkInterface to "127.0.0.1"
   accept connections on port tPort with message "__NewConnection"
   set the defaultNetworkInterface to tOldInterface
   
   put tPort into sUniqueRefs[tUniqueRef]
   
   if the environment is not "mobile" then
      close stack tUniqueRef
      set the style of stack tUniqueRef to "modal"
      set the visible of stack tUniqueRef to true
      set the destroyStack of stack tUniqueRef to true
      
      set the defaultStack to tOldDefault
      
      sheet tUniqueRef
   else
      wait while exists(widget tUniqueRef) with messages
   end if
   
   local tResult
   put the dialogData into tResult
   
   if tResult["code"] is not empty then
      local tParams
      put "grant_type=authorization_code" into tParams
      put "&client_id=" & urlEncode(pClientID) after tParams
      put "&client_secret=" & urlEncode(pClientSecret) after tParams
      put "&code=" & urlEncode(tResult["code"]) after tParams
      put "&redirect_uri=" & urlEncode(kRedirectURL & ":" & tPort & "/") after tParams
      
      local tResponse
      set the httpHeaders to "Accept: application/json"
      
      post tParams to pTokenURL
      put JSONToArray(it) into tResponse
      
      return tResponse for value
   else
      return tResult["error"] for error
   end if
end OAuth2


on __NewConnection pSocketID
   read from socket pSocketID until crlf & crlf with message "__HandleRequest"
end __NewConnection
 
on __HandleRequest pSocketID, pData
   close socket pSocketID
   
   local tRequest
   put word 2 of pData into tRequest
   split tRequest by "?"
   split tRequest[2] by "&" and "="
   
   local tUniqueRef
   put tRequest[2]["state"] into tUniqueRef
   
   local tPort
   put sUniqueRefs[tUniqueRef] into tPort
   
   if tPort is not empty then
      
      set the dialogData to tRequest[2]
      
      delete variable sUniqueRefs[tUniqueRef]
      
      -- stop accepting only if there aren't any other dialogs
      -- using the same port
      local tStopAccepting = true
      local tOtherPort
      repeat for each element tOtherPort in sUniqueRefs
         if tOtherPort is tPort then
            put false into tStopAccepting
            exit repeat
         end if
      end repeat
      
      if tStopAccepting then
         close socket tPort
      end if
      
      if the environment is "mobile" then
         delete widget tUniqueRef
      else
         close stack tUniqueRef
      end if
   else
      
      -- The OAuth2 spec requires the state parameter
      -- to be returned verbatim to prevent cross site
      -- request forgery so this is an error state on an
      -- unknown request... so we just burn them all
      
      repeat for each key tUniqueRef in sUniqueRefs
         close socket sUniqueRefs[tUniqueRef]
         if the environment is "mobile" then
            delete widget tUniqueRef
         else
            close stack tUniqueRef
         end if
      end repeat
   end if
end __HandleRequest
