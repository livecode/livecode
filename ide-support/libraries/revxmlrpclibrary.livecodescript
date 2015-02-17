script "revxmlrpclibrary"

on revLoadLibrary
   insert the script of me into back
end revLoadLibrary

on revUnloadLibrary
   remove the script of me from back
end revUnloadLibrary

----- LiveCode XML-RPC library ------

/*
* Version history :
* -----------------
* 
* Date      Who  Version  Comments
* --------  ---  -------  ------------------------------------------------------------------
* 20040516  js   2.3a3    first alpha version of the library
* 20040615  js   2.3a4    fixed a few issues upon code review
* 20040802  js   2.3b2    fixed a few issues ; improved error checking
*                         added GetHost, GetPort, GetPath, GetProtocol, GetMethod, GetSocket,
*                               GetResponse, GetRequest, GetHeaders functions
*                         added SetPort, DeleteDocument and DeleteAllDocuments handlers
* 20040803  js   2.3b2    added Documents function
* 20110523 mw 4.6.1-dp-1    changed Revolution to LiveCode
* 
*/

constant kXMLRPC_SimpleTypes = "base64,boolean,dateTime.iso8601,double,i4,int,string"
constant kXMLRPC_ComplexTypes = "array,struct"
constant kXMLRPC_DefaultHost = "localhost"
constant kXMLRPC_DefaultPort = "80"
constant kXMLRPC_DefaultPath = "RPC2"
constant kXMLRPC_DefaultProtocol = "http"

local sXMLRPC_Data

function revXMLRPC_CreateRequest pHost, pPort, pPath, pProtocol
  local tXMLText,tXMLTree
  put "<?xml version=" & quote & "1.0" & quote & "?>" & return & \
      "<methodCall><methodName /><params /></methodCall>" into tXMLText
  put revCreateXMLTree(tXMLText,false,true,false) into tXMLTree
  -- save the XML tree ID in the local data variable
  put tXMLTree into sXMLRPC_Data[tXMLTree,"id"]
  -- now set the additional information if extra params are sent
  if pHost is not empty then put pHost into sXMLRPC_Data[tXMLTree,"host"]
  if pPort is not empty then put pPort into sXMLRPC_Data[tXMLTree,"port"]
  if pPath is not empty then put pPath into sXMLRPC_Data[tXMLTree,"path"]
  if pProtocol is not empty then put pProtocol into sXMLRPC_Data[tXMLTree,"protocol"]
  return tXMLTree
end revXMLRPC_CreateRequest

on revXMLRPC_DeleteDocument pXMLRPCDocID
  if sXMLRPC_Data[pXMLRPCDocID,"id"] is empty then return "xmlrpcerr : bad document id" && pXMLRPCDocID
  ## introduced this as prefered synonym for revXMLRPC_Free in 2.5b2 for consistency with XML-library
  local tKeys, tKey
  -- clean up the local variables
  put the keys of sXMLRPC_Data into tKeys
  filter tKeys with (pXMLRPCDocID & ",*")
  repeat for each line tKey in tKeys
    delete local sXMLRPC_Data[tKey]
  end repeat
  -- delete the xml tree
  revDeleteXMLTree pXMLRPCDocID
end revXMLRPC_DeleteDocument

on revXMLRPC_DeleteAllDocuments
  ## introduced this call in 2.5b2 for ease of use
  local tDocuments, tDocument
  -- clean up all documents
  put revXMLRPC_Documents() into tDocuments
  repeat for each line tDocument in tDocuments
    revXMLRPC_DeleteDocument tDocument
  end repeat
end revXMLRPC_DeleteAllDocuments

on revXMLRPC_Free pXMLRPCDocID
  if sXMLRPC_Data[pXMLRPCDocID,"id"] is empty then return "xmlrpcerr : bad document id" && pXMLRPCDocID
  ## reworked this to a synonym for revXMLRPC_DeleteDocument in 2.5b2 for consistency with XML-library
  revXMLRPC_DeleteDocument pXMLRPCDocID
end revXMLRPC_Free

on revXMLRPC_FreeAll
  ## introduced this synonym to revXMLRPC_DeleteAllDocuments call in 2.5b2 for ease of use
  revXMLRPC_DeleteAllDocuments
end revXMLRPC_FreeAll

function revXMLRPC_Documents
  ## introduced this call in 2.5b2 for ease of use
  local tKeys, tKey, tDocuments
  put the keys of sXMLRPC_Data into tKeys
  filter tKeys with ("*,id")
  repeat for each line tKey in tKeys
    put sXMLRPC_Data[tKey] & return after tDocuments
  end repeat
  return char 1 to -2 of tDocuments -- without the trailing return
end revXMLRPC_Documents

on revXMLRPC_AddParam pXMLRPCDocID, pParamType, pParamValue
  if sXMLRPC_Data[pXMLRPCDocID,"id"] is empty then return "xmlrpcerr : bad document id" && pXMLRPCDocID
  ## if it is a valid doc ID, proceed
  local tHiType,tLoType,tParamData,tKeys,tKey
  local tParamsNode
  set the itemDelimiter to ":"
  put item 1 of pParamType into tHiType
  put item 2 of pParamType into tLoType
  set the itemDelimiter to comma
  switch tHiType
  case "array"
    -- open the value, array and data tags
    put "<value><array><data>" into tParamData
    -- default data type in arrays is string
    if tLoType is empty then put "string" into tLoType
    if tLoType is not among the items of kXMLRPC_SimpleTypes
    then return "xmlrpcerr : received unknown parameter type" && pParamType
    -- add all the values
    put the keys of pParamValue into tKeys
    sort lines in tKeys numeric
    repeat for each line tKey in tKeys
      put "<value><" & tLoType & ">" & pParamValue[tKey] & "</" & tLoType & "></value>" after tParamData
    end repeat
    -- close the value, array and data tags
    put "</data></array></value>" after tParamData
    break
  case "xml"
    -- the parameter is raw xml ; surround with value tag
    put "<value>" & pParamValue & "</value>" into tParamData
    break
  default
    if tHiType is among the items of kXMLRPC_SimpleTypes
    then put "<value><" & tHiType & ">" & pParamValue & "</" & tHiType & "></value>" into tParamData
    else return "xmlrpcerr : received unknown parameter type" && pParamType
    break
  end switch
  -- add the data into the xml tree, and pass any error if applicable
  put "/" & revXMLRootNode(pXMLRPCDocID) & "/params" into tParamsNode
  revAppendXML pXMLRPCDocID, tParamsNode, "<param>" & tParamData & "</param>"
  if the result is not empty then return the result
end revXMLRPC_AddParam

on revXMLRPC_DeleteParam pXMLRPCDocID, pParamNumber
  if sXMLRPC_Data[pXMLRPCDocID,"id"] is empty then return "xmlrpcerr : bad document id" && pXMLRPCDocID
  ## if it is a valid doc ID, proceed
  local tParamNode
  put "/" & revXMLRootNode(pXMLRPCDocID) & "/params/param[" & pParamNumber & "]" into tParamNode
  -- delete the param node from the xml tree, and pass any error if applicable
  revDeleteXMLNode pXMLRPCDocID, tParamNode
  if the result is not empty then return the result
end revXMLRPC_DeleteParam

function revXMLRPC_GetParamCount pXMLRPCDocID
  if sXMLRPC_Data[pXMLRPCDocID,"id"] is empty then return "xmlrpcerr : bad document id" && pXMLRPCDocID
  ## if it is a valid doc ID, proceed
  local tParamsNode
  put "/" & revXMLRootNode(pXMLRPCDocID) & "/params" into tParamsNode
  return revXMLNumberOfChildren(pXMLRPCDocID, tParamsNode,,0)
end revXMLRPC_GetParamCount

function revXMLRPC_GetParam pXMLRPCDocID, pParamNumber, pType
  if sXMLRPC_Data[pXMLRPCDocID,"id"] is empty then return "xmlrpcerr : bad document id" && pXMLRPCDocID
  ## if it is a valid doc ID, proceed
  local tParamCount, tValueNode, tValueType, tDataNode
  put revXMLRPC_GetParamCount(pXMLRPCDocID) into tParamCount
  if pParamNumber > tParamCount then return "xmlrpcerr : there are only" && tParamCount && "params"
  put "/" & revXMLRootNode(pXMLRPCDocID) & "/params/param[" & pParamNumber & "]/value" into tValueNode
  put revXMLChildNames(pXMLRPCDocID,tValueNode,return,,false) into tValueType
  put tValueNode & "/" & tValueType into tDataNode
  if pType is "xml" or pType is empty then
    put revXMLText(pXMLRPCDocID,tDataNode) into tParamData
  else if pType is not tValueType then
    put "xmlrpcerr : requested type" && pType && "does not match the value type" && tValueType into tParamData
  else if pType is among the items of kXMLRPC_ComplexTypes then
    put revXMLText(pXMLRPCDocID,tDataNode) into tParamData
  else if pType is among the items of kXMLRPC_SimpleTypes then
    put revXMLNodeContents(pXMLRPCDocID,tDataNode) into tParamData
  else -- if we got all the way here, it is an unkown data type
    put "xmlrpcerr : encountered unknown data type" && tValueType && "while parsing response data" into tParamData
  end if
  return tParamData
end revXMLRPC_GetParam

function revXMLRPC_GetParamType pXMLRPCDocID, pParamNumber
  if sXMLRPC_Data[pXMLRPCDocID,"id"] is empty then return "xmlrpcerr : bad document id" && pXMLRPCDocID
  ## if it is a valid doc ID, proceed
  local tParamCount, tValueNode
  put revXMLRPC_GetParamCount(pXMLRPCDocID) into tParamCount
  if pParamNumber > tParamCount then return "xmlrpcerr : there are only" && tParamCount && "params"
  put "/" & revXMLRootNode(pXMLRPCDocID) & "/params/param[" & pParamNumber & "]/value" into tValueNode
  return revXMLChildNames(pXMLRPCDocID,tValueNode,return,,false)
end revXMLRPC_GetParamType

function revXMLRPC_GetParamNode pXMLRPCDocID, pParamNumber
  if sXMLRPC_Data[pXMLRPCDocID,"id"] is empty then return "xmlrpcerr : bad document id" && pXMLRPCDocID
  ## if it is a valid doc ID, proceed
  local tParamCount, tValueNode
  put revXMLRPC_GetParamCount(pXMLRPCDocID) into tParamCount
  if pParamNumber > tParamCount then return "xmlrpcerr : there are only" && tParamCount && "params"
  return "/" & revXMLRootNode(pXMLRPCDocID) & "/params/param[" & pParamNumber & "]"
end revXMLRPC_GetParamNode

on revXMLRPC_SetMethod pXMLRPCDocID, pMethodName
  if sXMLRPC_Data[pXMLRPCDocID,"id"] is empty then return "xmlrpcerr : bad document id" && pXMLRPCDocID
  ## if it is a valid doc ID, proceed
  revPutIntoXMLNode pXMLRPCDocID, "/methodCall/methodName", pMethodName
end revXMLRPC_SetMethod

function revXMLRPC_GetMethod pXMLRPCDocID
  if sXMLRPC_Data[pXMLRPCDocID,"id"] is empty then return "xmlrpcerr : bad document id" && pXMLRPCDocID
  ## if it is a valid doc ID, proceed
  return revXMLNodeContents(pXMLRPCDocID, "/methodCall/methodName")
end revXMLRPC_GetMethod

on revXMLRPC_SetHost pXMLRPCDocID, pHost, pPort
  if sXMLRPC_Data[pXMLRPCDocID,"id"] is empty then return "xmlrpcerr : bad document id" && pXMLRPCDocID
  ## if it is a valid doc ID, proceed
  put pHost into sXMLRPC_Data[pXMLRPCDocID,"host"]
  put pPort into sXMLRPC_Data[pXMLRPCDocID,"port"]
end revXMLRPC_SetHost

on revXMLRPC_SetPort pXMLRPCDocID, pPort
  if sXMLRPC_Data[pXMLRPCDocID,"id"] is empty then return "xmlrpcerr : bad document id" && pXMLRPCDocID
  ## if it is a valid doc ID, proceed
  put pPort into sXMLRPC_Data[pXMLRPCDocID,"port"]
end revXMLRPC_SetPort

function revXMLRPC_GetHost pXMLRPCDocID
  if sXMLRPC_Data[pXMLRPCDocID,"id"] is empty then return "xmlrpcerr : bad document id" && pXMLRPCDocID
  ## if it is a valid doc ID, proceed
  return sXMLRPC_Data[pXMLRPCDocID,"host"]
end revXMLRPC_GetHost

function revXMLRPC_GetPort pXMLRPCDocID
  if sXMLRPC_Data[pXMLRPCDocID,"id"] is empty then return "xmlrpcerr : bad document id" && pXMLRPCDocID
  ## if it is a valid doc ID, proceed
  return sXMLRPC_Data[pXMLRPCDocID,"port"]
end revXMLRPC_GetPort

on revXMLRPC_SetPath pXMLRPCDocID, pPath
  if sXMLRPC_Data[pXMLRPCDocID,"id"] is empty then return "xmlrpcerr : bad document id" && pXMLRPCDocID
  ## if it is a valid doc ID, proceed
  put pPath into sXMLRPC_Data[pXMLRPCDocID,"path"]
end revXMLRPC_SetPath

function revXMLRPC_GetPath pXMLRPCDocID
  if sXMLRPC_Data[pXMLRPCDocID,"id"] is empty then return "xmlrpcerr : bad document id" && pXMLRPCDocID
  ## if it is a valid doc ID, proceed
  return sXMLRPC_Data[pXMLRPCDocID,"path"]
end revXMLRPC_GetPath

on revXMLRPC_SetProtocol pXMLRPCDocID, pProtocol
  if sXMLRPC_Data[pXMLRPCDocID,"id"] is empty then return "xmlrpcerr : bad document id" && pXMLRPCDocID
  ## if it is a valid doc ID, proceed
  put pProtocol into sXMLRPC_Data[pXMLRPCDocID,"protocol"]
end revXMLRPC_SetProtocol

function revXMLRPC_GetProtocol pXMLRPCDocID
  if sXMLRPC_Data[pXMLRPCDocID,"id"] is empty then return "xmlrpcerr : bad document id" && pXMLRPCDocID
  ## if it is a valid doc ID, proceed
  return sXMLRPC_Data[pXMLRPCDocID,"protocol"]
end revXMLRPC_GetProtocol

on revXMLRPC_SetSocket pXMLRPCDocID, pSocket
  if sXMLRPC_Data[pXMLRPCDocID,"id"] is empty then return "xmlrpcerr : bad document id" && pXMLRPCDocID
  ## if it is a valid doc ID, proceed
  if pSocket is not among the lines of the openSockets
  then return "xmlrpcerr :" && pSocket && "is not an open socket"
  put pSocket into sXMLRPC_Data[pXMLRPCDocID,"socket"]
end revXMLRPC_SetSocket

function revXMLRPC_GetSocket pXMLRPCDocID
  if sXMLRPC_Data[pXMLRPCDocID,"id"] is empty then return "xmlrpcerr : bad document id" && pXMLRPCDocID
  ## if it is a valid doc ID, proceed
  return sXMLRPC_Data[pXMLRPCDocID,"socket"]
end revXMLRPC_GetSocket

function revXMLRPC_Execute pXMLRPCDocID
  if sXMLRPC_Data[pXMLRPCDocID,"id"] is empty then return "xmlrpcerr : bad document id" && pXMLRPCDocID
  if sXMLRPC_Data[pXMLRPCDocID,"request"] is not empty then return "xmlrpcerr : document id" && pXMLRPCDocID && "is a response"
  ## if it is a valid doc ID, proceed
  local tPath, tSocket, tHost, tPort, tURL
  local tPayload, tHTTPHeaders, tResponseData, tResponseID
  -- collect the host, port, path and protocol information
  put sXMLRPC_Data[pXMLRPCDocID,"host"] into tHost
  put sXMLRPC_Data[pXMLRPCDocID,"port"] into tPort
  put sXMLRPC_Data[pXMLRPCDocID,"path"] into tPath
  put sXMLRPC_Data[pXMLRPCDocID,"protocol"] into tProtocol
  -- insert defaults where applicable
  if tHost is empty then put kXMLRPC_DefaultHost into tHost
  if tPort is empty then put kXMLRPC_DefaultPort into tPort
  if tPath is empty then put kXMLRPC_DefaultPath into tPath
  if tProtocol is empty then put kXMLRPC_DefaultProtocol into tProtocol
  -- prepare the POST payload and HTTP headers
  put revXMLText(pXMLRPCDocID) into tPayload
  put "User-Agent: LiveCode/" & revAppVersion() && "(" & the platform & ")" & return & \
      "Host:" && tHost & return & \
      "Content-Type: text/xml" & return & \
      "Content-Length:" && length(tPayload) into tHTTPHeaders
  ## now fork depending on whether using an existing socket or not
  put sXMLRPC_Data[pXMLRPCDocID,"socket"] into tSocket
  if tSocket is not empty then
    -- use the existing connection
    if tSocket is not among the lines of the openSockets
    then return "xmlrpcerr :" && tSocket && "is not an open socket"
    -- make sure the connection stays open
    put return & "Connection: Keep-Alive" after tHTTPHeaders
    replace return with CRLF in tHTTPHeaders
    write "POST /" & tPath && toUpper(tProtocol) &"/1.1" & CRLF & \
        tHTTPHeaders & CRLF & CRLF & tPayload to socket tSocket
    read from socket tSocket until CRLF
    -- save the response and check the server status code
    put it into tServerResponse
    if word 2 of line 1 of tServerResponse is not "200" then
      -- make sure to read all the data, even if we ignore it
      read from socket tSocket until empty
      put "xmlrpcerr :" && word 2 to -1 of tServerResponse into sXMLRPC_Data[pXMLRPCDocID,"error"]
      return sXMLRPC_Data[pXMLRPCDocID,"error"]
    end if
    -- read the headers
    read from socket tSocket until (CRLF & CRLF)
    put it into tSRHTTPHeaders
    replace CRLF with return in tSRHTTPHeaders
    if line -1 of tSRHTTPHeaders is empty then delete line -1 of tSRHTTPHeaders
    -- read the response data for parsing
    read from socket tSocket until empty
    put it into tResponseData
  else
    -- use POST command from libURL
    put toLower(tProtocol) & "://" & tHost & ":" & tPort & "/" & tPath into tURL
    -- now post the payload to the URL
    put the httpHeaders into tSaveHTTPHeaders #mf 03/08/2005 Bug 2444
    set the httpHeaders to tHTTPHeaders
    post tPayload to URL tURL
    -- save the response and check the result
    put it into tResponseData
    if the result is not empty then
       put the result into sXMLRPC_Data[pXMLRPCDocID,"error"]
       
       # OK-2009-06-19 : Bug 8121 - Ensure httpHeaders global property is not changed by failed execution
       set the httpheaders to tSaveHTTPHeaders
       
      return sXMLRPC_Data[pXMLRPCDocID,"error"]
    end if
    -- don't forget to fetch the remote HTTP headers
    put libURLLastRHHeaders() into tSRHTTPHeaders
    set the httpHeaders to tSaveHTTPHeaders #mf 03/08/2005 Bug 2444
  end if
  -- parse the response
  put revCreateXMLTree(tResponseData,true,true,false) into tResponseID
  if tResponseID is not a number then
    put tResponseID into sXMLRPC_Data[pXMLRPCDocID,"error"]
    return sXMLRPC_Data[pXMLRPCDocID,"error"]
  end if
  put tResponseID into sXMLRPC_Data[tResponseID,"id"]
  -- save the related information with the response tree
  put pXMLRPCDocID into sXMLRPC_Data[tResponseID,"request"]
  -- copy the necessary connection information
  if tSocket is not empty then
    put tSocket into sXMLRPC_Data[tResponseID,"socket"]
  else
    put tHost into sXMLRPC_Data[tResponseID,"host"]
    put tPort into sXMLRPC_Data[tResponseID,"port"]
  end if
  put tPath into sXMLRPC_Data[tResponseID,"path"]
  -- save the response HTTP-headers
  put tSRHTTPHeaders into sXMLRPC_Data[tResponseID,"headers"]
  -- save a link to the response tree with the request information
  put tResponseID into sXMLRPC_Data[pXMLRPCDocID,"response"]
  -- finally return the response tree
  return tResponseID
end revXMLRPC_Execute

function revXMLRPC_Error pXMLRPCDocID
  if sXMLRPC_Data[pXMLRPCDocID,"id"] is empty then return "xmlrpcerr : bad document id" && pXMLRPCDocID
  ## if it is a valid doc ID, proceed
  return sXMLRPC_Data[pXMLRPCDocID,"error"]
end revXMLRPC_Error

function revXMLRPC_GetResponse pXMLRPCDocID
  if sXMLRPC_Data[pXMLRPCDocID,"id"] is empty then return "xmlrpcerr : bad document id" && pXMLRPCDocID
  ## if it is a valid doc ID, proceed
  return sXMLRPC_Data[pXMLRPCDocID,"response"]
end revXMLRPC_GetResponse

function revXMLRPC_GetRequest pXMLRPCDocID
  if sXMLRPC_Data[pXMLRPCDocID,"id"] is empty then return "xmlrpcerr : bad document id" && pXMLRPCDocID
  ## if it is a valid doc ID, proceed
  return sXMLRPC_Data[pXMLRPCDocID,"request"]
end revXMLRPC_GetRequest

function revXMLRPC_GetHeaders pXMLRPCDocID
  if sXMLRPC_Data[pXMLRPCDocID,"id"] is empty then return "xmlrpcerr : bad document id" && pXMLRPCDocID
  ## if it is a valid doc ID, proceed
  return sXMLRPC_Data[pXMLRPCDocID,"headers"]
end revXMLRPC_GetHeaders

--
