script "revhtml5urllibrary"
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

on extensionInitialize
   if the target is not me then
      exit extensionInitialize
   end if

   if the environment is "development" then
      revSBAddDependencyForInclusion "scriptLibraries", "Internet", "scriptLibraries", "html5url", "emscripten"
   end if

   if the platform is "HTML5" then
      libURLSetDriver the long id of me
   end if
end extensionInitialize

on extensionFinalize
   if the target is not me then
      exit extensionFinalize
   end if

   if the environment is "development" then
      revSBRemoveDependencyForInclusion "scriptLibraries", "Internet", "scriptLibraries", "html5url"
   end if

   if the platform is "HTML5" then
      libURLSetDriver empty
   end if
end extensionFinalize

--------------------------------------------------------------------------------
-- LibURL driver config
--------------------------------------------------------------------------------

function ulExtInitDriver
   -- note that me in this instance is liburl (since liburl loads drivers by
   -- setting them as a behaviour and we're a liburl driver)
   -- so, we need to send any javascript callbacks to liburl, which will then
   -- propogate them up to the driver
   set the cJavascriptHandlers of me to "ulExtXMLHTTPCallback"

   do "LiveCodeLibURL.init()" as "javascript"
   if the result is not empty then
      return "Error initialising javascript library:" && the result
   end if

   return empty
end ulExtInitDriver

command ulExtRemoveDriver
   __ulExtXMLHTTPDestroyAll
end ulExtRemoveDriver

command ulExtResetDriver
   ulExtRemoveDriver
   return ulExtInitDriver()
end ulExtResetDriver

function ulExtIsBlocked
   return false
end ulExtIsBlocked

command ulExtSetLogField pField

end ulExtSetLogField

--------------------------------------------------------------------------------
-- Main entry points
--------------------------------------------------------------------------------

command ulExtHandleRequest pUrl, pAction, pHeaders, pPostData, pProxyUrl
   local tProtocol
   set the itemDel to colon
   put item 1 of pURL into tProtocol
   set the itemDel to comma
   if tProtocol is not among the words of "http https" then
      __ulExtSetURLStatus pURL, "error", "Protocol" && tProtocol && "is not supported", false
      __ulExtCleanupURL pURL
      exit ulExtHandleRequest
   end if

   local tAsync
   put pAction is "getData" and ulExtIsLoadReq(pUrl) into tAsync

   local tMethod
   switch pAction
      case "getData"
         put "GET" into tMethod
         break
      case "deleteData"
         put "DELETE" into tMethod
         break
      case "putData"
         put "PUT" into tMethod
         break
      case "postData"
         put "POST" into tMethod
         break
      default
         __ulExtSetURLStatus pURL, "error", "Action" && pAction && "is not supported", false
         __ulExtCleanupURL pURL
         exit ulExtHandleRequest
         break
   end switch

   -- liburl maintains its own cache so ideally we want to prevent the browser
   -- doing any setting the Cache-Control header will go some way to achieving
   -- this (depending on server config and browser) but appears to clash with
   -- corsresponse header (Access-Control-Allow-Origin) in some browsers
   -- so don't set for the moment
   -- put "Cache-Control: no-cache" & return after pHeaders
   -- put "If-Modified-Since: Sat, 1 Jan 2000 00:00:00 GMT" & return after pHeaders
   -- put "Expires: Sat, 1 Jan 2000 00:00:00 GMT" & return after pHeaders

   local tRequest
   put __ulExtXMLHTTPCreate(pURL, tMethod, tAsync) into tRequest
   if not __ulExtIsValidXMLHTTP(tRequest) then
      __ulExtSetURLStatus pURL, "error", tRequest, false
      __ulExtCleanupURL pURL
      exit ulExtHandleRequest
   end if

   __ulExtXMLHTTPSetTimeout tRequest, the socketTimeoutInterval
   if the result is not empty then
      __ulExtSetURLStatus pURL, "error", the result, false
      __ulExtCleanupURL pURL
      exit ulExtHandleRequest
   end if

   __ulExtXMLHTTPSetHeaders tRequest, pHeaders
   if the result is not empty then
      __ulExtSetURLStatus pURL, "error", the result, false
      __ulExtCleanupURL pURL
      exit ulExtHandleRequest
   end if

   local tBody
   if pAction is among the words of "postData putData" then
      put pPostData into tBody
   else
      put empty into tBody
   end if

   local tResult
   __ulExtXMLHTTPSend tRequest, tBody
   put the result into tResult

   -- at this point asynchronous requests (i.e. load url) will be working in the
   -- background, so we're now done
   if tAsync and tResult is empty then
      exit ulExtHandleRequest
   end if

   if the result is not empty then
      __ulExtSetURLStatus pURL, "error", tResult, false
   else
      ulExtSetErrorStatus pUrl, empty
   end if

   -- at this point we will have potentially gotten a response from the server
   -- (even if it was technically an error - 404, 5xx etc)
   -- so store the headers and response against the request
   ulExtSetLastRHHeaders __ulExtXMLHTTPGetResponseHeaders(tRequest)
   ulExtSetLastHTTPHeaders __ulExtXMLHTTPGetRequestHeaders(tRequest)
   ulExtSetData pUrl, __ulExtXMLHTTPGetResponse(tRequest)

   __ulExtCleanupURL pURL
end ulExtHandleRequest

command ulExtCancelRequest pUrl
   __ulExtCleanupURL pURL
end ulExtCancelRequest

--------------------------------------------------------------------------------
-- LibURL settings
--------------------------------------------------------------------------------

command ulExtSetSSLVerification pWhich
   -- XMLHTTP requests always verifiy SSL certs
   if not pWhich then
      return "Unsupported"
   end if
end ulExtSetSSLVerification

command ulExtFollowHttpRedirects pWhich
   -- XMLHTTP requests always follow redirects
   if not pWhich then
      return "Unsupported"
   end if
end ulExtFollowHttpRedirects

--------------------------------------------------------------------------------
-- FTP handlers
--------------------------------------------------------------------------------

function ulExtFtpCommand pCommand, pHost, pUser, pPass
   return "Unsupported"
end ulExtFtpCommand

command ulExtSetFtpMode pMode
   return "Unsupported"
end ulExtSetFtpMode

command ulExtSetFtpListCommand pCommand
   return "Unsupported"
end ulExtSetFtpListCommand

--------------------------------------------------------------------------------
-- XMl HTTP request accessors
--------------------------------------------------------------------------------

-- each request will be given a unique identifier that will be used to access it
-- liburl refers to requests by url so maintain a map between url and id
-- we could use the url as the id but doing so will cause potential future limitations
-- (eg. multiple async posts to the same url with differnt post data)
local sRequestMap

private function __ulExtXMLHTTPToURL pRequest
   return __ulExtXMLHTTPGetURL(pRequest)
end __ulExtXMLHTTPToURL

private function __ulExtXMLHTTPFromURL pURL
   return sRequestMap[pURL]
end __ulExtXMLHTTPFromURL

private command __ulExtXMLHTTPDestroyAll
   repeat for each element tRequest in sRequestMap
      __ulExtXMLHTTPDestroy tRequest
   end repeat
end __ulExtXMLHTTPDestroyAll

private function __ulExtIsValidXMLHTTP pRequest
   return __ulExtCallJS("isValidRequest", pRequest)
end __ulExtIsValidXMLHTTP

private function __ulExtXMLHTTPCreate pURL, pMethod, pAsync
   get __ulExtCallJS("requestCreate", pURL, pMethod, pAsync)
   put it into sRequestMap[pURL]
   return it
end __ulExtXMLHTTPCreate

private command __ulExtXMLHTTPDestroy self
   delete variable sRequestMap[__ulExtXMLHTTPGetURL(self)]
   get __ulExtCallJS("requestDestroy", self)
   return empty
end __ulExtXMLHTTPDestroy

private command __ulExtXMLHTTPSend self, pBody
   return __ulExtCallJS("requestSend", self, pBody)
end __ulExtXMLHTTPSend

private command __ulExtXMLHTTPCancel self
   get __ulExtCallJS("requestCancel", self)
   return empty
end __ulExtXMLHTTPCancel

private command __ulExtXMLHTTPSetTimeout self, pTimeout
   return __ulExtCallJS("requestSetTimeout", self, pTimeout)
end __ulExtXMLHTTPSetTimeout

private command __ulExtXMLHTTPSetHeaders self, pHeaders
   set the itemDel to ":"
   repeat for each line tHeader in pHeaders
      get __ulExtCallJS("requestSetHeader", self, item 1 of tHeader, word 1 to -1 of item 2 to -1 of tHeader)
      if it is not empty then
         return it
      end if
   end repeat
   return empty
end __ulExtXMLHTTPSetHeaders

private function __ulExtXMLHTTPGetRequestHeaders self
   return __ulExtCallJS("requestGetRequestHeaders", self)
end __ulExtXMLHTTPGetRequestHeaders

private function __ulExtXMLHTTPGetResponseHeaders self
   return __ulExtCallJS("requestGetResponseHeaders", self)
end __ulExtXMLHTTPGetResponseHeaders

private function __ulExtXMLHTTPGetResponse self
   return __ulExtCallJS("requestGetResponse", self)
end __ulExtXMLHTTPGetResponse

private function __ulExtXMLHTTPGetStatus self
   return __ulExtCallJS("requestGetStatus", self)
end __ulExtXMLHTTPGetStatus

private function __ulExtXMLHTTPGetStatusCode self
   return __ulExtCallJS("requestGetStatusCode", self)
end __ulExtXMLHTTPGetStatusCode

private function __ulExtXMLHTTPGetURL self
   return __ulExtCallJS("requestGetURL", self)
end __ulExtXMLHTTPGetURL

--------------------------------------------------------------------------------
-- Javascript callback
--------------------------------------------------------------------------------

on ulExtXMLHTTPCallback pRequest, pURL, pEvent, pLoaded, pTotal
   -- the progress, timeout, error and load events are the final events in a requests life cycle
   -- we only care about these for async (i.e. load) requests where we send the load complete callback
   -- synchronous requests will get this data directly from the original send call
   switch pEvent
      case "loadstart"
         __ulExtSetURLStatus pURL, "contacted", empty, true
         __ulExtSetURLStatus pURL, "requested", empty, true
         break

      case "progress"
         if pTotal is -1 then
            __ulExtSetURLStatus pURL, "loading", format("loading,%d", pLoaded), true
         else
            __ulExtSetURLStatus pURL, "loading", format("loading,%d,%d", pLoaded, pTotal), true
         end if
         break

      case "abort"
         if ulExtIsLoadReq(pUrl) then
            __ulExtSetURLStatus pURL, "error", "Requeset cancelled", true
            ulSendMessage pURL
            __ulExtCleanupURL pURL
         end if
         break

      case "timeout"
         if ulExtIsLoadReq(pUrl) then
            __ulExtSetURLStatus pURL, "timeout", empty, true
            ulSendMessage pURL
            __ulExtCleanupURL pURL
         end if
         break

      case "error"
         if ulExtIsLoadReq(pUrl) then
            local tError
            put __ulExtXMLHTTPGetStatus(pRequest) into tError
            if tError is empty then
               put "Request failed" &&  __ulExtXMLHTTPGetStatusCode(pRequest) into tError
            end if
            __ulExtSetURLStatus pURL, "error", tError, true

            -- even though the request failed, still store any response from the server
            ulExtSetLastRHHeaders __ulExtXMLHTTPGetResponseHeaders(pRequest)
            ulExtSetLastHTTPHeaders __ulExtXMLHTTPGetRequestHeaders(pRequest)
            ulExtSetAsLoaded pUrl, __ulExtXMLHTTPGetResponse(pRequest)

            ulSendMessage pURL
            __ulExtCleanupURL pURL
         end if
         break

      case "load"
         if ulExtIsLoadReq(pUrl) then
            ulExtSetErrorStatus pUrl, empty
            ulExtSetLastRHHeaders __ulExtXMLHTTPGetResponseHeaders(pRequest)
            ulExtSetLastHTTPHeaders __ulExtXMLHTTPGetRequestHeaders(pRequest)

            if ulExtIsFileTransfer(pUrl) is not empty then
               -- XMLHTTP doesn't support download to file, so just write the data to file directy
               ulExtSetLoadStatus pUrl, "downloaded"
               put __ulExtXMLHTTPGetResponse(pRequest)  into URL ("binfile:" & ulExtIsFileTransfer(pUrl))
            else
               ulExtSetLoadStatus pUrl, "cached"
               ulExtSetAsLoaded pUrl, __ulExtXMLHTTPGetResponse(pRequest)
            end if

            ulSendMessage pURL
            __ulExtCleanupURL pURL
         end if
         break
   end switch
end ulExtXMLHTTPCallback

--------------------------------------------------------------------------------

private command __ulExtSetURLStatus pURL, pStatus, pMessage, pSendCallback
   if pStatus is "error" then
      ulExtSetErrorStatus pUrl, pMessage
   else
      ulExtSetErrorStatus pUrl, empty
   end if

   if ulExtIsLoadReq(pUrl) then
      ulExtSetLoadStatus pUrl, pStatus
   end if

   if pSendCallback then
      if pMessage is not empty then
         ulSendCallback pURL, pMessage
      else
         ulSendCallback pURL, pStatus
      end if
   end if
end __ulExtSetURLStatus

private command __ulExtCleanupURL pURL
   local tRequest
   put __ulExtXMLHTTPFromURL(pURL) into tRequest
   if tRequest is not empty then
      __ulExtXMLHTTPDestroy tRequest
   end if
   ulExtRemoveLoadingVars pURL
end __ulExtCleanupURL

-- Utililty handler that calls the given function in the javascript url library
-- Pass the function name and any parameters
private function __ulExtCallJS pFunction
   local FUNCTIONNAME
   put pFunction into FUNCTIONNAME

   -- build the list or parameters escaping any quotes
   local PARAMLIST
   repeat with tParamNo = 2 to the paramCount
      local tParam
      put param(tParamNo) into tParam
      if tParam is not a number and tParam is not among the words of "true false" then
         replace backslash with "\\" in tParam
         replace quote with backslash & quote in tParam
         replace return with "\n" in tParam
         replace cr with "\r" in tParam
         put quote & tParam & quote into tParam
      end if
      if PARAMLIST is not empty then
         put comma after PARAMLIST
      end if
      put tParam after PARAMLIST
   end repeat

   local tJS
   put "LiveCodeLibURL.[[FUNCTIONNAME]]([[PARAMLIST]])" into tJS
   put merge(tJS) into tJS
   do tJS as "javascript"
   return the result
end __ulExtCallJS
