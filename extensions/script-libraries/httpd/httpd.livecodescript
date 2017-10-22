script "com.livecode.library.httpd"
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
      pass extensionInitialize
   end if
   
   insert the script of me into back
   
   if the environment contains "development" then
      set the _ideoverride of me to true
   end if
end extensionInitialize

on extensionFinalize
   if the target is not me then
      pass extensionFinalize
   end if
   
   remove the script of me from back
end extensionFinalize

/**
Title: HTTPD Library

Version: 1.0.0

Author: LiveCode

Description: Handle HTTP requests in your application
*/

local sServers -- an array of server info keyed on server port
local sRequests -- an array of request info keyed on socket id

constant kDefaultPort = 8080

/**

Start a HTTP server

Example:
on mouseUp
   httpdStart "NewRequest", 12345, "My Server"
   launch url ("http://localhost:" & it)
end mouseUp

on NewRequest pSocketID, pRequest
   httpdResponse pSocketID, 200
end NewRequest

Description:
Start a HTTP server. Once started any HTTP requests to the server will
result in a callback to the caller with the message specified with pCallback.

If the callback will have two parameters. The socket ID of the request. And an
array describing the request. The array has the following keys:

- "method": The HTTP request method
- "resource": The request resource path e.g /images/foo.gif
- "parameters": For GET requests with parameters or application/x-www-form-urlencoded encoded
this key will be an array of parameters
- "headers": An array of request headers
- "content": The request body (empty for application/x-www-form-urlencoded)

Parameters:

pCallback (string):
A handler name to be sent to the caller when a new HTTP request
is handled by the server.

pPort (optional string):
The port to accept HTTP requests on. If empty then port 8080 will be
used and if the accept command fails with 8080 an available ephemeral
port will be used. If pPort is 0 an available ephemeral port will be
used.

pServerName (optional string):
Optional server name defaults to `LiveCode`

The result:
An error string if an error occurred

It:
The port the server was started on. Use this if pPort was empty or 0 to
determine which port was used.

*/
command httpdStart pCallback, pPort, pServerName
   if pServerName is empty then
      put "LiveCode" into pServerName
   end if
   
   local tPort
   if pPort is empty then
      put kDefaultPort into tPort
      accept connections on port tPort with message "__NewRequest"
      
      if the result is not empty then
         accept connections on port "0" with message "__NewRequest"
         if the result is not empty then
            return the result for error
         end if
      end if
   else
      put pPort into tPort
      accept connections on port tPort with message "__NewRequest"
      if the result is not empty then
         return the result for error
      end if
   end if
   
   put it into tPort
   
   put __GetCaller() into sServers[tPort]["target"]
   put pCallback into sServers[tPort]["callback"]
   put pServerName into sServers[tPort]["servername"]
   
   return tPort for value
end httpdStart

/**

Stop a HTTP server

Description:
Stop all or some HTTP servers by closing the socket and deleting
server metadata.

Parameters:

pPorts (optional string):
A return delimited list of server ports to stop. If empty all servers are
stopped.

*/
command httpdStop pPorts
   if pPorts is empty then
      -- stop all
      put the keys of sServers into pPorts
   end if
   
   repeat for each line tPort in pPorts
      close socket tPort
      delete variable sServers[tPort]
   end repeat
end httpdStop

/**

Respond to a HTTP request

Example:
on mouseUp
   httpdStart "NewRequest", 12345, "My Server"
   launch url ("http://localhost:" & it)
end mouseUp

on NewRequest pSocketID, pRequest
   local tPath
   put the effective filename of me into tPath
   set the itemDelimiter to slash
   
   put "files/" & pRequest["resource"] into the last item of tPath
   
   if there is a file tPath then
      local tData
      put url ("binfile:" & tPath) into tData
      httpdResponse pSocketID, 200, tData
   else
      -- not found
      httpdResponse pSocketID, 404
   end if
end NewRequest
   
Description:
Handle the callback set when starting the server and respond to the request according to your requirements.

Parameters:

pSocketID (string):
The socket ID sent to the request callback

pResponseCode (integer):
A HTTP response code. See [here](https://www.w3.org/Protocols/rfc2616/rfc2616-sec10.html) for more detail.

pContent (optional data):
This is the content to respond to the request with.

pHeaders (optional string):
Any additional headers to send with the response. Content-Length, Server, Date and Connection are set by default.

*/
   
command httpdResponse pSocketID, pResponseCode, pContent, pHeaders
   local tPort
   put sRequests[pSocketID]["port"] into tPort
   
   if tPort is empty then
      return "invalid socket id" for error
   end if
   
   replace return with crlf in pHeaders
   if pHeaders is not empty and not (pHeaders ends with crlf) then
      put crlf after pHeaders
   end if
   
   if pResponseCode is empty then
      put 200 into pResponseCode
   end if
   
   local tResponse
   put "HTTP/1.1" && pResponseCode && __ResponseCodeString(pResponseCode) & crlf & \
         "Date:" && the internet date & crlf & \
         "Server:" && sServers[tPort]["servername"] & crlf & \
         "Connection: close" & crlf & \
         pHeaders & \
         "Content-Length: " & the length of pContent & crlf & crlf before pContent
   
   write pContent & crlf to socket pSocketID with message "__ResponseWritten"
end httpdResponse

private function __GetCaller
   get item 1 to -3 of line -3 of the executionContexts
   if there is not an it then
      delete item -1 of it
   end if
   return it
end __GetCaller

on __NewRequest pSocketID, pLocalPort
   put pLocalPort into sRequests[pSocketID]["port"]
   
   read from socket pSocketID with message "__ReadRequest"
end __NewRequest

on __ExpectContinueWritten pSocketID
   read from socket pSocketID with message "__ReadContinuedRequest"
end __ExpectContinueWritten

private function __ParamsToArray pParams
   split pParams by "&" and "="
   
   local tParams
   repeat for each key tKey in pParams
      put urlDecode(pParams[tKey]) into tParams[urlDecode(tKey)]
   end repeat
   return tParams
end __ParamsToArray

on __ReadRequest pSocketID, pData
   local tPort
   put sRequests[pSocketID]["port"] into tPort
   
   if tPort is empty then
      -- can not repsond so just hang up
      close socket pSocketID
      exit __ReadRequest
   end if
   
   if there is not a sServers[tPort]["target"] then
      -- respond as server error
      httpdResponse pSocketID, 500, "Target" && sServers[tPort]["target"] && "does not exist!"
      exit __ReadRequest
   end if
   
   put word 1 of pData into  sRequests[pSocketID]["method"]
   
   put word 2 of pData into sRequests[pSocketID]["resource"]
   
   set the itemDelimiter to "?"
   put __ParamsToArray(item 2 to -1 of sRequests[pSocketID]["resource"]) into sRequests[pSocketID]["parameters"]
   put item 1 of sRequests[pSocketID]["resource"] into sRequests[pSocketID]["resource"]
   set the itemDelimiter to comma
   delete line 1 of pData
   
   __ParseHeaders pData, sRequests[pSocketID]["headers"]
   
   if sRequests[pSocketID]["headers"]["Expect"] is "100-continue" then
      write "100 Continue" to socket pSocketID with message "__ExpectContinueWritten"
      exit __ReadRequest
   end if
   
   __ReadContinuedRequest pSocketID, pData
end __ReadRequest

on __ReadContinuedRequest pSocketID, pData
   if sRequests[pSocketID]["content"] is empty then
      set the itemDelimiter to ";"
      switch item 1 of sRequests[pSocketID]["headers"]["Content-Type"]
         case "application/x-www-form-urlencoded"
            put __ParamsToArray(word 1 to -1 of pData) into sRequests[pSocketID]["parameters"]
            break
         case "multipart/form-data"
            -- todo
         default
            put pData into sRequests[pSocketID]["content"]
      end switch
   end if
   
   local tPort
   put sRequests[pSocketID]["port"] into tPort
   
   try
      dispatch sServers[tPort]["callback"] to sServers[tPort]["target"] with pSocketID, sRequests[pSocketID]
   catch tError
      httpResponse pSocketID, 500, tError
      
      --!TODO pass error in development mode
   end try
end __ReadContinuedRequest

private command __ParseHeaders @xData, @rHeaders
   local tHeaders
   
   local tOffset
   put byteOffset(crlf&crlf, xData) into tOffset
   if tOffset is not 0 then
      add 3 to tOffset
      
      local tHeaderData
      put char 1 to tOffset of xData into tHeaderData
      delete char 1 to tOffset of xData
      replace crlf with return in tHeaderData
      split tHeaderData by return
      
      local tLastHeader
      set the itemDelimiter to ":"
      repeat with tIndex = 1 to the number of elements in tHeaderData
         if tHeaderData[tIndex] begins with space or \
               tHeaderData[tIndex] begins with tab then
            put space & word 1 to -1 of tHeaderData[tIndex] after tHeaders[tLastHeader]
         else
            put item 1 of tHeaderData[tIndex] into tLastHeader
            if tLastHeader is empty then next repeat
            
            delete item 1 of tHeaderData[tIndex]
            put word 1 to -1 of tHeaderData[tIndex] into tHeaders[tLastHeader]
         end if
      end repeat
   end if
   
   put tHeaders into rHeaders
end __ParseHeaders

on __ResponseWritten pSocketID
   close socket pSocketID
   delete variable sRequests[pSocketID]
end __ResponseWritten

on socketClosed pSocketID
   delete variable sRequests[pSocketID]
end socketClosed

on socketError pSocketID
   delete variable sRequests[pSocketID]
end socketError

private function __ResponseCodeString pCode
   switch pCode
      case 100 
         return "Continue"
      case 101 
         return "Switching Protocols"
      case 102
         return "Processing"
      case 200
         return "OK"
      case 201
         return "Created"
      case 202
         return "Accepted"
      case 203
         return "Non-Authoritative Information"
      case 204
         return "No Content"
      case 205
         return "Reset Content"
      case 206
         return "Partial Content"
      case 207
         return "Multi-Status"
      case 208
         return "Already Reported"
      case 226
         return "IM Used"
      case 300
         return "Multiple Choices"
      case 301
         return "Moved Permanently"
      case 302
         return "Found"
      case 303
         return "See Other"
      case 304
         return "Not Modified"
      case 305
         return "Use Proxy"
      case 306
         return "Switch Proxy"
      case 307
         return "Temporary Redirect"
      case 308
         return "Permanent Redirect"
      case 400
         return "Bad Request"
      case 401
         return "Unauthorized"
      case 402
         return "Payment Required"
      case 403
         return "Forbidden"
      case 404
         return "Not Found"
      case 405
         return "Method Not Allowed"
      case 406
         return "Not Acceptable"
      case 407
         return "Proxy Authentication Required"
      case 408
         return "Request Timeout"
      case 409
         return "Conflict"
      case 410
         return "Gone"
      case 411
         return "Length Required"
      case 412
         return "Precondition Failed"
      case 413
         return "Payload Too Large"
      case 414
         return "URI Too Long"
      case 415
         return "Unsupported Media Type"
      case 416
         return "Range Not Satisfiable"
      case 417
         return "Expectation Failed"
      case 418
         return "I'm a teapot"
      case 421
         return "Misdirected"
      case 422
         return "Unprocessable Entity"
      case 423
         return "Locked"
      case 424
         return "Failed Dependency"
      case 426
         return "Upgrade Required"
      case 428
         return "Precondition Required"
      case 429
         return "Too Many Requests"
      case 431
         return "Request Header Fields Too Large"
      case 451
         return "Unavailable For Legal Reasons"
      case 500
         return "Internal Server Error"
      case 501
         return "Not Implemented"
      case 502
         return "Bad Gateway"
      case 503
         return "Service Unavailable"
      case 504
         return "Gateway Timeout"
      case 505
         return "HTTP Version Not Supported"
      case 506
         return "Variant Also Negotiates"
      case 507
         return "Insufficient Storage"
      case 508
         return "Loop Detected"
      case 510
         return "Not Extended"
      case 511
         return "Network Authentication Required"
   end switch
   return empty
end __ResponseCodeString
