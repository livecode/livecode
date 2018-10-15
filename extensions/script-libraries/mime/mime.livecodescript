script "com.livecode.library.mime"
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

on extensionInitialize
   if the target is not me then
      pass extensionInitialize
   end if
   
   insert the script of me into back
   
   if the environment contains "development" then
      set the _ideoverride of me to true
   end if
   
   __Initialize
end extensionInitialize

on extensionFinalize
   if the target is not me then
      pass extensionFinalize
   end if
   
   remove the script of me from back
end extensionFinalize

/**
Title: MIME Library

Version: 1.0.0

Author: LiveCode

Description:
This script library implements portions of Multipurpose Internet Mail 
Extensions (MIME) as described in RFCs 2045 - 2049. Use this library to 
implement common data formats such as email message bodies and form 
encoding in HTTP.
*/

local sMIMETypes

/**

The MIME version this implemented in the library

Example:
put "MIME-Version:" && mimeTheMIMEVersion() & crlf after tHeaders

Description:
Use the <mimeTheMIMEVersion> function create a `MIME-Version` header.

*/

function mimeTheMIMEVersion
   return "1.0"
end mimeTheMIMEVersion

/**

Determine the MIME type for a file extension

Example:
put "Content-Type:" && mimeTheMIMETypeOfExtension("xls") & crlf after tHeaders

Parameters:
pExtension (string):
A file extension

Description:
The mime types listed are the default mime types in an Apache web server installation
for a given file extension. If the file extension is not found then `application/octet-stream`
will be returned.

Returns:
A mime type for the given file extension.

*/

function mimeTheMIMETypeOfExtension pExtension
   local tMIMEType
   
   set the caseSensitive to true
   put sMIMETypes[pExtension] into tMIMEType
   
   if tMIMEType is empty then
      set the caseSensitive to false
      put sMIMETypes[pExtension] into tMIMEType
   end if
   
   if tMIMEType is empty then
      return "application/octet-stream" for value
   else
      return tMIMEType for value
   end if
end mimeTheMIMETypeOfExtension

/**

List extensions that match a MIME type or MIME type wildcard pattern

Example:
put line 1 of mimeTheExtensionsOfMIMEType("application/epub+zip") into tExtension

Example:
put mimeTheExtensionsOfMIMEType("image/*") into tImageExtensions

Parameters:
pMIMEType (string):
A MIME type or wildcard pattern for matching a MIME type

Description:
List the extensions associated with a given MIME type or with a MIME type matching
a wildcard pattern.

Returns:
A list of extensions or empty if none were found.

*/

function mimeTheExtensionsOfMIMEType pMimeType
   local tExtensions
   filter elements of sMIMETypes with wildcard pattern pMimeType into tExtensions
   
   set the caseSensitive to true
   
   local tExtensionList
   repeat for each key tExtension in tExtensions
      put true into tExtensionList[tExtension]
   end repeat
   
   combine tExtensionList with return as set
   sort tExtensionList
   
   return tExtensionList
end mimeTheExtensionsOfMIMEType

/**

Encode data in content transfer encoding

Example:
put tHeaders & crlf & crlf & \
      mimeEncodeForMIMETransfer(tEmailBody, "quoted-printable") into tContentA[1]

Parameters:
pData:
The data to be encoded. If the parameter is not binary data then it will
be encoded to binary as UTF-8 encoded text before encoding with the
requested form. 

pEncoding (enum):
The encoding to return the data in
- "quoted-printable": Quoted printable encoding. Most likely used for text with many ASCII characters
- "base64": Base 64 encoding

Description:
Use the <mimeEncodeForMIMETransfer> function to encode data for transfer

Returns:
The encoded data

References:
textEncode (function)

*/

function mimeEncodeForMIMETransfer pData, pEncoding
   if pData is not strictly a binary string then
      put textEncode(pData, "UTF-8") into pData
   end if
   
   switch pEncoding
      case "quoted-printable"
         return __EncodeAsQuotedPrintable(pData)
      case "base64"
         return __EncodeAsBase64(pData)
      default
         return "unsupported encoding" for error
   end switch
end mimeEncodeForMIMETransfer

private function __EncodeAsBase64 pData
   return replaceText(base64Encode(pData), return, crlf)
end __EncodeAsBase64

private function __EncodeAsQuotedPrintable pData
   local tData
   local tLineLength = 1
   local tNumBytes
   
   put the number of bytes of pData into tNumBytes
   
   local tIndex = 1
   repeat for each byte tByte in pData
      if tLineLength is 76 then
         put "=" & crlf after tData
         put 1 into tLineLength
      end if
      
      local tByteNum
      put byteToNum(tByte) into tByteNum
      
      local tPrevByteNum
      
      switch
         case tByteNum is 10 and tPrevByteNum is not 13
            put crlf after tData
            put 1 into tLineLength
            break 
         case (tByteNum is 9 or tByteNum is 32) and  tLineLength is not 76 and tIndex is not tNumBytes
         case tByteNum >= 33 and tByteNum <= 126 and tByteNum is not 61
            put tByte after tData
            add 1 to tLineLength
            break
         default
            if tLineLength >= 74 and \
                  not (tLineLength is 74 and tIndex is tNumBytes) then
               put "=" & crlf after tData
               put 3 into tLineLength
            end if
            
            local tConverted
            put baseConvert(tByteNum,10,16) into tConverted
            if the length of tConverted is 1 then
               put 0 before tConverted
            end if
            put "=" & tConverted after tData
            add 3 to tLineLength
            break
      end switch
      
      put tByteNum into tPrevByteNum
      add 1 to tIndex
   end repeat
   
   return tData
end __EncodeAsQuotedPrintable

/**

Build a multipart mime content type from an array of pre-encoded content

Example:
put "text/html" into tParamA["type"]
put mimeEncodeAsMIMEMultipartDocument(tContentA, "related", tParamA) into tMailBody

Description:
This command takes an array of pre-encoded content and creates a multipart
content type header and boundary to build a single multipart encoded value.

Parameters:
pContentA (array):
A numerically indexed array of pre-encoded parts including headers. If no headers are present
a leading crlf should be present.

pMultipartType (optional string):
The type of multipart content. If not provided the default type will be `mixed`.

pParamA (optional array):
Array of key : value pairs of additional options to be included in the Content-Type header

Returns:
The multipart encoded content

*/

function mimeEncodeAsMIMEMultipartDocument pContentA, pMultipartType, pParamA
   if pMultipartType is empty then
      put "mixed" into pMultipartType
   end if
   
   local tBoundary
   put "Boundary-" & uuid() into tBoundary
   
   local tData
   put "Content-Type: multipart/" & pMultipartType into tData
   
   local tParam
   repeat for each key tParam in pParamA
      put ";" & crlf & tab & tParam & "=" & quote & pParamA[tParam] & quote after tData
   end repeat
   
   put ";" & crlf & tab & "boundary=" & tBoundary & crlf & crlf after tData
   
   local tIndex
   repeat with tIndex = 1 to the number of elements of pContentA
      put crlf & "--" & tBoundary & crlf & pContentA[tIndex] after tData
   end repeat
   
   put crlf & "--" & tBoundary & "--" & crlf & crlf after tData
   
   return tData for value
end mimeEncodeAsMIMEMultipartDocument


/**

Generate multipart content from a field control

Example:
put mimeEncodeFieldAsMIMEMultipartDocument(the long id of field "email") into tBody
SendEmail tBody

Description:
Parses the content of a field to create multipart content

Parameters:
pField (string):
The long id of the field to encode

The result:
An error message if an error was encountered

It:
Set to the multipart content generated from the field content

*/

function mimeEncodeFieldAsMIMEMultipartDocument pField
   if there is not a pField then
      return "Object does not exist" for error
   end if
   
   local tAlternativeA
   
   put "Content-Type: text/plain; charset=UTF-8" & crlf & \
         "Content-Transfer-Encoding: quoted-printable" & crlf & crlf after tAlternativeA[1]
   
   put mimeEncodeForMIMETransfer(the text of pField, "quoted-printable") after tAlternativeA[1]
   
   put "Content-Type: text/html; charset=UTF-8" & crlf & \
         "Content-Transfer-Encoding: quoted-printable" & crlf & crlf into tAlternativeA[2]
   
   local tHTML
   put the htmlText of pField into tHTML
   
   local tImages
   __ParseImages tHTML, tImages
   
   put "<!DOCTYPE HTML PUBLIC " & quote & "-//W3C//DTD HTML 4.01 Transitional//EN" & quote & ">" & return & \
         "<html>" & return & \
         "<head>" & return & \
         "<meta http-equiv=" & quote & "content-type" & quote & " content=" & quote & "text/html; charset=UTF-8" & quote & ">" & return & \
         "</head>" & return before tHTML
   
   put "</html>" after tHTML
   
   put mimeEncodeForMIMETransfer(tHTML, "quoted-printable") after tAlternativeA[2]
   
   local tRelatedA
   put mimeEncodeAsMIMEMultipartDocument(tAlternativeA, "alternative") into tRelatedA[1]
   
   local tIndex = 1
   local tImage
   repeat for each key tImage in tImages
      if tImage is an integer then
         resolve image id tImage relative to pField
      else
         resolve image tImage relative to pField
      end if
      if it is not empty then
         add 1 to tIndex
         put "Content-Type: image/" & the paintCompression of it & crlf & \
               "Content-Transfer-Encoding: base64" & crlf & \
               "Content-ID: <" & tImages[tImage] & ">" & crlf \
               & tab & "name="& quote & tImage & "." & the paintCompression of it & quote & crlf & \
               "Content-Disposition: inline;" & crlf \
               & tab & "filename="& quote & tImage & "." & the paintCompression of it & quote & crlf & crlf into tRelatedA[tIndex]
         
         put mimeEncodeForMIMETransfer(the text of it, "base64") after tRelatedA[tIndex]
      end if
   end repeat
   
   if the number of elements of tRelatedA > 1 then
      local tParamA
      put "text/html" into tParamA["type"]
      
      return mimeEncodeAsMIMEMultipartDocument(tRelatedA, "related", tParamA) for value
   end if
   
   return tRelatedA[1] for value
end mimeEncodeFieldAsMIMEMultipartDocument

private command __ParseImages @xHTML, @rImages
   local tHTML
   
   local tStart, tEnd
   repeat while matchChunk(xHTML,"(?i)<img src=" & quote & "([^" & quote & "]*)" & quote,tStart,tEnd)
      local tID
      put char tStart to tEnd of xHTML into tID
      
      if tID begins with "http" then
         put char 1 to tEnd of xHTML after tHTML
      else
         
         if rImages[tID] is empty then
            put uuid() into rImages[tID]
         end if
         
         put char 1 to tStart - 1 of xHTML & "cid:" & rImages[tID] after tHTML
      end if
      delete char 1 to tEnd of xHTML
   end repeat
   
   put tHTML before xHTML
end __ParseImages


/**

Create data to be sent as an email

Example:
put mimeEncodeFieldAsMIMEMultipartDocument(the long id of field "email") into tBody
mimeEncodeAsMIMEEmail tBody, "sales@livecode.com", field "email", , "Purchase order"

local tBytes, tSettings, tResponseHeaders
put "username" into tSettings["username"]
put "p@$$w0rd" into tSettings["password"]

local tResult
put tsNetSmtpSync("smtps://mail.livecode.com:465", "sales@livecode.com", field "email", it, tResponseHeaders,tBytes,tSettings) into tResult
if tResult begins with "tsneterr:" then
   answer error "Error:" & word 2 to -1 of tResult
end if

Description:
Creates the data to be sent as an email via an SMTP server

Parameters:
pBody (string):
The pre-encoded body of the email. See the <mimeEncodeFieldAsMIMEMultipartDocument>
<function> for an easy way to generate the body from field content.

pFrom (string):
From email address

pTo (string):
Recipient email addresses with one per line

pCc (string):
Copy to email addresses with one per line

pSubject (string):
Email subject line

pAttachmentA (array): A numerically indexed (1..N) multi-dimensional
array. Each element describes a different attachment. The elements of
the array may contain the following keys.

- "filepath": The full path to the file. Optional if the "data" element has a value
- "data": The file data. Optional if the "filepath" element has a value
- "name": The file name. Optional if the "filepath" element has a value
- "mimetype": The file mime type. Optional if the "filepath" or "name" element has an extension

The result:
An error message if an error was encountered

It:
Set to the email body including headers

*/

command mimeEncodeAsMIMEEmail pBody, pFrom, pTo, pCc, pSubject, pAttachmentA
   local tHeaders
   
   put "Date:" && the internet date & crlf into tHeaders
   put "From:" && pFrom & crlf after tHeaders
   replace return with ", " in pTo
   put "To:" && pTo & crlf  after tHeaders
   
   if pCc is not empty then
      replace return with ", " in pCc
      put "Cc:" && pCc & crlf  after tHeaders
   end if
   
   if pSubject is not an ascii string then
      local tSubject
      put mimeEncodeForMIMETransfer(pSubject, "quoted-printable") into tSubject
      put "=?UTF-8?Q?" & tSubject & "?=" into pSubject
   end if
   put "Subject:" && pSubject & crlf after tHeaders
   
   put "MIME-Version:" && mimeTheMIMEVersion() & crlf after tHeaders
   
   if pAttachmentA is an array then
      
      local tMixedA
      put pBody into tMixedA[1]
      
      repeat with tIndex = 1 to the number of elements of pAttachmentA
         if pAttachmentA[tIndex] is not an array then
            next repeat
         end if
         
         if pAttachmentA[tIndex]["data"] is empty then
            if there is not a file pAttachmentA[tIndex]["filepath"] then
               next repeat
            end if
            
            put url ("binfile:" & pAttachmentA[tIndex]["filepath"]) into pAttachmentA[tIndex]["data"]
         end if
         
         if pAttachmentA[tIndex]["name"] is empty then
            if pAttachmentA[tIndex]["filepath"] is empty then
               next repeat
            end if
            
            set the itemDelimiter to slash
            put the last item of pAttachmentA[tIndex]["filepath"] into pAttachmentA[tIndex]["name"]
         end if
         
         if pAttachmentA[tIndex]["mimetype"] is empty then
            set the itemDelimiter to "."
            local tExtension
            put the last item of pAttachmentA[tIndex]["name"] into tExtension
            put mimeTheMIMETypeOfExtension(tExtension) into pAttachmentA[tIndex]["mimetype"]
         end if
         
         put "Content-Type:" && pAttachmentA[tIndex]["mimetype"] & ";" & crlf \
               & tab & "name="& quote & pAttachmentA[tIndex]["name"] & quote & crlf & \
               "Content-Transfer-Encoding: base64" & crlf & \
               "Content-Disposition: attachment;" & crlf \
               & tab & "filename="& quote & pAttachmentA[tIndex]["name"] & quote & crlf & crlf into tMixedA[tIndex+1]
         
         put mimeEncodeForMIMETransfer(pAttachmentA[tIndex]["data"], "base64") after tMixedA[tIndex+1]
      end repeat
      
   end if
   
   if the number of elements of tMixedA > 1 then
      return tHeaders & mimeEncodeAsMIMEMultipartDocument(tMixedA, "mixed") for value
   end if
   
   return tHeaders & pBody for value
end mimeEncodeAsMIMEEmail


private command __Initialize
   -- MIME to extension mapping generated via the following command:
   -- curl http://svn.apache.org/repos/asf/httpd/httpd/trunk/docs/conf/mime.types 
   --   | egrep -v ^#
   --   | awk '{ for (i=2; i<=NF; i++) {print "put \""$1"\" into sMIMETypes[\""$i"\"\]"}}' 
   --   | sort
   
   put "application/andrew-inset" into sMIMETypes["ez"]
   put "application/applixware" into sMIMETypes["aw"]
   put "application/atom+xml" into sMIMETypes["atom"]
   put "application/atomcat+xml" into sMIMETypes["atomcat"]
   put "application/atomsvc+xml" into sMIMETypes["atomsvc"]
   put "application/ccxml+xml" into sMIMETypes["ccxml"]
   put "application/cdmi-capability" into sMIMETypes["cdmia"]
   put "application/cdmi-container" into sMIMETypes["cdmic"]
   put "application/cdmi-domain" into sMIMETypes["cdmid"]
   put "application/cdmi-object" into sMIMETypes["cdmio"]
   put "application/cdmi-queue" into sMIMETypes["cdmiq"]
   put "application/cu-seeme" into sMIMETypes["cu"]
   put "application/davmount+xml" into sMIMETypes["davmount"]
   put "application/docbook+xml" into sMIMETypes["dbk"]
   put "application/dssc+der" into sMIMETypes["dssc"]
   put "application/dssc+xml" into sMIMETypes["xdssc"]
   put "application/ecmascript" into sMIMETypes["ecma"]
   put "application/emma+xml" into sMIMETypes["emma"]
   put "application/epub+zip" into sMIMETypes["epub"]
   put "application/exi" into sMIMETypes["exi"]
   put "application/font-tdpfr" into sMIMETypes["pfr"]
   put "application/font-woff" into sMIMETypes["woff"]
   put "application/gml+xml" into sMIMETypes["gml"]
   put "application/gpx+xml" into sMIMETypes["gpx"]
   put "application/gxf" into sMIMETypes["gxf"]
   put "application/hyperstudio" into sMIMETypes["stk"]
   put "application/inkml+xml" into sMIMETypes["ink"]
   put "application/inkml+xml" into sMIMETypes["inkml"]
   put "application/ipfix" into sMIMETypes["ipfix"]
   put "application/java-archive" into sMIMETypes["jar"]
   put "application/java-serialized-object" into sMIMETypes["ser"]
   put "application/java-vm" into sMIMETypes["class"]
   put "application/javascript" into sMIMETypes["js"]
   put "application/json" into sMIMETypes["json"]
   put "application/jsonml+json" into sMIMETypes["jsonml"]
   put "application/lost+xml" into sMIMETypes["lostxml"]
   put "application/mac-binhex40" into sMIMETypes["hqx"]
   put "application/mac-compactpro" into sMIMETypes["cpt"]
   put "application/mads+xml" into sMIMETypes["mads"]
   put "application/marc" into sMIMETypes["mrc"]
   put "application/marcxml+xml" into sMIMETypes["mrcx"]
   put "application/mathematica" into sMIMETypes["ma"]
   put "application/mathematica" into sMIMETypes["mb"]
   put "application/mathematica" into sMIMETypes["nb"]
   put "application/mathml+xml" into sMIMETypes["mathml"]
   put "application/mbox" into sMIMETypes["mbox"]
   put "application/mediaservercontrol+xml" into sMIMETypes["mscml"]
   put "application/metalink+xml" into sMIMETypes["metalink"]
   put "application/metalink4+xml" into sMIMETypes["meta4"]
   put "application/mets+xml" into sMIMETypes["mets"]
   put "application/mods+xml" into sMIMETypes["mods"]
   put "application/mp21" into sMIMETypes["m21"]
   put "application/mp21" into sMIMETypes["mp21"]
   put "application/mp4" into sMIMETypes["mp4s"]
   put "application/msword" into sMIMETypes["doc"]
   put "application/msword" into sMIMETypes["dot"]
   put "application/mxf" into sMIMETypes["mxf"]
   put "application/octet-stream" into sMIMETypes["bin"]
   put "application/octet-stream" into sMIMETypes["bpk"]
   put "application/octet-stream" into sMIMETypes["deploy"]
   put "application/octet-stream" into sMIMETypes["dist"]
   put "application/octet-stream" into sMIMETypes["distz"]
   put "application/octet-stream" into sMIMETypes["dms"]
   put "application/octet-stream" into sMIMETypes["dump"]
   put "application/octet-stream" into sMIMETypes["elc"]
   put "application/octet-stream" into sMIMETypes["lrf"]
   put "application/octet-stream" into sMIMETypes["mar"]
   put "application/octet-stream" into sMIMETypes["pkg"]
   put "application/octet-stream" into sMIMETypes["so"]
   put "application/oda" into sMIMETypes["oda"]
   put "application/oebps-package+xml" into sMIMETypes["opf"]
   put "application/ogg" into sMIMETypes["ogx"]
   put "application/omdoc+xml" into sMIMETypes["omdoc"]
   put "application/onenote" into sMIMETypes["onepkg"]
   put "application/onenote" into sMIMETypes["onetmp"]
   put "application/onenote" into sMIMETypes["onetoc"]
   put "application/onenote" into sMIMETypes["onetoc2"]
   put "application/oxps" into sMIMETypes["oxps"]
   put "application/patch-ops-error+xml" into sMIMETypes["xer"]
   put "application/pdf" into sMIMETypes["pdf"]
   put "application/pgp-encrypted" into sMIMETypes["pgp"]
   put "application/pgp-signature" into sMIMETypes["asc"]
   put "application/pgp-signature" into sMIMETypes["sig"]
   put "application/pics-rules" into sMIMETypes["prf"]
   put "application/pkcs10" into sMIMETypes["p10"]
   put "application/pkcs7-mime" into sMIMETypes["p7c"]
   put "application/pkcs7-mime" into sMIMETypes["p7m"]
   put "application/pkcs7-signature" into sMIMETypes["p7s"]
   put "application/pkcs8" into sMIMETypes["p8"]
   put "application/pkix-attr-cert" into sMIMETypes["ac"]
   put "application/pkix-cert" into sMIMETypes["cer"]
   put "application/pkix-crl" into sMIMETypes["crl"]
   put "application/pkix-pkipath" into sMIMETypes["pkipath"]
   put "application/pkixcmp" into sMIMETypes["pki"]
   put "application/pls+xml" into sMIMETypes["pls"]
   put "application/postscript" into sMIMETypes["ai"]
   put "application/postscript" into sMIMETypes["eps"]
   put "application/postscript" into sMIMETypes["ps"]
   put "application/prs.cww" into sMIMETypes["cww"]
   put "application/pskc+xml" into sMIMETypes["pskcxml"]
   put "application/rdf+xml" into sMIMETypes["rdf"]
   put "application/reginfo+xml" into sMIMETypes["rif"]
   put "application/relax-ng-compact-syntax" into sMIMETypes["rnc"]
   put "application/resource-lists+xml" into sMIMETypes["rl"]
   put "application/resource-lists-diff+xml" into sMIMETypes["rld"]
   put "application/rls-services+xml" into sMIMETypes["rs"]
   put "application/rpki-ghostbusters" into sMIMETypes["gbr"]
   put "application/rpki-manifest" into sMIMETypes["mft"]
   put "application/rpki-roa" into sMIMETypes["roa"]
   put "application/rsd+xml" into sMIMETypes["rsd"]
   put "application/rss+xml" into sMIMETypes["rss"]
   put "application/rtf" into sMIMETypes["rtf"]
   put "application/sbml+xml" into sMIMETypes["sbml"]
   put "application/scvp-cv-request" into sMIMETypes["scq"]
   put "application/scvp-cv-response" into sMIMETypes["scs"]
   put "application/scvp-vp-request" into sMIMETypes["spq"]
   put "application/scvp-vp-response" into sMIMETypes["spp"]
   put "application/sdp" into sMIMETypes["sdp"]
   put "application/set-payment-initiation" into sMIMETypes["setpay"]
   put "application/set-registration-initiation" into sMIMETypes["setreg"]
   put "application/shf+xml" into sMIMETypes["shf"]
   put "application/smil+xml" into sMIMETypes["smi"]
   put "application/smil+xml" into sMIMETypes["smil"]
   put "application/sparql-query" into sMIMETypes["rq"]
   put "application/sparql-results+xml" into sMIMETypes["srx"]
   put "application/srgs" into sMIMETypes["gram"]
   put "application/srgs+xml" into sMIMETypes["grxml"]
   put "application/sru+xml" into sMIMETypes["sru"]
   put "application/ssdl+xml" into sMIMETypes["ssdl"]
   put "application/ssml+xml" into sMIMETypes["ssml"]
   put "application/tei+xml" into sMIMETypes["tei"]
   put "application/tei+xml" into sMIMETypes["teicorpus"]
   put "application/thraud+xml" into sMIMETypes["tfi"]
   put "application/timestamped-data" into sMIMETypes["tsd"]
   put "application/vnd.3gpp.pic-bw-large" into sMIMETypes["plb"]
   put "application/vnd.3gpp.pic-bw-small" into sMIMETypes["psb"]
   put "application/vnd.3gpp.pic-bw-var" into sMIMETypes["pvb"]
   put "application/vnd.3gpp2.tcap" into sMIMETypes["tcap"]
   put "application/vnd.3m.post-it-notes" into sMIMETypes["pwn"]
   put "application/vnd.accpac.simply.aso" into sMIMETypes["aso"]
   put "application/vnd.accpac.simply.imp" into sMIMETypes["imp"]
   put "application/vnd.acucobol" into sMIMETypes["acu"]
   put "application/vnd.acucorp" into sMIMETypes["acutc"]
   put "application/vnd.acucorp" into sMIMETypes["atc"]
   put "application/vnd.adobe.air-application-installer-package+zip" into sMIMETypes["air"]
   put "application/vnd.adobe.formscentral.fcdt" into sMIMETypes["fcdt"]
   put "application/vnd.adobe.fxp" into sMIMETypes["fxp"]
   put "application/vnd.adobe.fxp" into sMIMETypes["fxpl"]
   put "application/vnd.adobe.xdp+xml" into sMIMETypes["xdp"]
   put "application/vnd.adobe.xfdf" into sMIMETypes["xfdf"]
   put "application/vnd.ahead.space" into sMIMETypes["ahead"]
   put "application/vnd.airzip.filesecure.azf" into sMIMETypes["azf"]
   put "application/vnd.airzip.filesecure.azs" into sMIMETypes["azs"]
   put "application/vnd.amazon.ebook" into sMIMETypes["azw"]
   put "application/vnd.americandynamics.acc" into sMIMETypes["acc"]
   put "application/vnd.amiga.ami" into sMIMETypes["ami"]
   put "application/vnd.android.package-archive" into sMIMETypes["apk"]
   put "application/vnd.anser-web-certificate-issue-initiation" into sMIMETypes["cii"]
   put "application/vnd.anser-web-funds-transfer-initiation" into sMIMETypes["fti"]
   put "application/vnd.antix.game-component" into sMIMETypes["atx"]
   put "application/vnd.apple.installer+xml" into sMIMETypes["mpkg"]
   put "application/vnd.apple.mpegurl" into sMIMETypes["m3u8"]
   put "application/vnd.aristanetworks.swi" into sMIMETypes["swi"]
   put "application/vnd.astraea-software.iota" into sMIMETypes["iota"]
   put "application/vnd.audiograph" into sMIMETypes["aep"]
   put "application/vnd.blueice.multipass" into sMIMETypes["mpm"]
   put "application/vnd.bmi" into sMIMETypes["bmi"]
   put "application/vnd.businessobjects" into sMIMETypes["rep"]
   put "application/vnd.chemdraw+xml" into sMIMETypes["cdxml"]
   put "application/vnd.chipnuts.karaoke-mmd" into sMIMETypes["mmd"]
   put "application/vnd.cinderella" into sMIMETypes["cdy"]
   put "application/vnd.claymore" into sMIMETypes["cla"]
   put "application/vnd.cloanto.rp9" into sMIMETypes["rp9"]
   put "application/vnd.clonk.c4group" into sMIMETypes["c4d"]
   put "application/vnd.clonk.c4group" into sMIMETypes["c4f"]
   put "application/vnd.clonk.c4group" into sMIMETypes["c4g"]
   put "application/vnd.clonk.c4group" into sMIMETypes["c4p"]
   put "application/vnd.clonk.c4group" into sMIMETypes["c4u"]
   put "application/vnd.cluetrust.cartomobile-config" into sMIMETypes["c11amc"]
   put "application/vnd.cluetrust.cartomobile-config-pkg" into sMIMETypes["c11amz"]
   put "application/vnd.commonspace" into sMIMETypes["csp"]
   put "application/vnd.contact.cmsg" into sMIMETypes["cdbcmsg"]
   put "application/vnd.cosmocaller" into sMIMETypes["cmc"]
   put "application/vnd.crick.clicker" into sMIMETypes["clkx"]
   put "application/vnd.crick.clicker.keyboard" into sMIMETypes["clkk"]
   put "application/vnd.crick.clicker.palette" into sMIMETypes["clkp"]
   put "application/vnd.crick.clicker.template" into sMIMETypes["clkt"]
   put "application/vnd.crick.clicker.wordbank" into sMIMETypes["clkw"]
   put "application/vnd.criticaltools.wbs+xml" into sMIMETypes["wbs"]
   put "application/vnd.ctc-posml" into sMIMETypes["pml"]
   put "application/vnd.cups-ppd" into sMIMETypes["ppd"]
   put "application/vnd.curl.car" into sMIMETypes["car"]
   put "application/vnd.curl.pcurl" into sMIMETypes["pcurl"]
   put "application/vnd.dart" into sMIMETypes["dart"]
   put "application/vnd.data-vision.rdz" into sMIMETypes["rdz"]
   put "application/vnd.dece.data" into sMIMETypes["uvd"]
   put "application/vnd.dece.data" into sMIMETypes["uvf"]
   put "application/vnd.dece.data" into sMIMETypes["uvvd"]
   put "application/vnd.dece.data" into sMIMETypes["uvvf"]
   put "application/vnd.dece.ttml+xml" into sMIMETypes["uvt"]
   put "application/vnd.dece.ttml+xml" into sMIMETypes["uvvt"]
   put "application/vnd.dece.unspecified" into sMIMETypes["uvvx"]
   put "application/vnd.dece.unspecified" into sMIMETypes["uvx"]
   put "application/vnd.dece.zip" into sMIMETypes["uvvz"]
   put "application/vnd.dece.zip" into sMIMETypes["uvz"]
   put "application/vnd.denovo.fcselayout-link" into sMIMETypes["fe_launch"]
   put "application/vnd.dna" into sMIMETypes["dna"]
   put "application/vnd.dolby.mlp" into sMIMETypes["mlp"]
   put "application/vnd.dpgraph" into sMIMETypes["dpg"]
   put "application/vnd.dreamfactory" into sMIMETypes["dfac"]
   put "application/vnd.ds-keypoint" into sMIMETypes["kpxx"]
   put "application/vnd.dvb.ait" into sMIMETypes["ait"]
   put "application/vnd.dvb.service" into sMIMETypes["svc"]
   put "application/vnd.dynageo" into sMIMETypes["geo"]
   put "application/vnd.ecowin.chart" into sMIMETypes["mag"]
   put "application/vnd.enliven" into sMIMETypes["nml"]
   put "application/vnd.epson.esf" into sMIMETypes["esf"]
   put "application/vnd.epson.msf" into sMIMETypes["msf"]
   put "application/vnd.epson.quickanime" into sMIMETypes["qam"]
   put "application/vnd.epson.salt" into sMIMETypes["slt"]
   put "application/vnd.epson.ssf" into sMIMETypes["ssf"]
   put "application/vnd.eszigno3+xml" into sMIMETypes["es3"]
   put "application/vnd.eszigno3+xml" into sMIMETypes["et3"]
   put "application/vnd.ezpix-album" into sMIMETypes["ez2"]
   put "application/vnd.ezpix-package" into sMIMETypes["ez3"]
   put "application/vnd.fdf" into sMIMETypes["fdf"]
   put "application/vnd.fdsn.mseed" into sMIMETypes["mseed"]
   put "application/vnd.fdsn.seed" into sMIMETypes["dataless"]
   put "application/vnd.fdsn.seed" into sMIMETypes["seed"]
   put "application/vnd.flographit" into sMIMETypes["gph"]
   put "application/vnd.fluxtime.clip" into sMIMETypes["ftc"]
   put "application/vnd.framemaker" into sMIMETypes["book"]
   put "application/vnd.framemaker" into sMIMETypes["fm"]
   put "application/vnd.framemaker" into sMIMETypes["frame"]
   put "application/vnd.framemaker" into sMIMETypes["maker"]
   put "application/vnd.frogans.fnc" into sMIMETypes["fnc"]
   put "application/vnd.frogans.ltf" into sMIMETypes["ltf"]
   put "application/vnd.fsc.weblaunch" into sMIMETypes["fsc"]
   put "application/vnd.fujitsu.oasys" into sMIMETypes["oas"]
   put "application/vnd.fujitsu.oasys2" into sMIMETypes["oa2"]
   put "application/vnd.fujitsu.oasys3" into sMIMETypes["oa3"]
   put "application/vnd.fujitsu.oasysgp" into sMIMETypes["fg5"]
   put "application/vnd.fujitsu.oasysprs" into sMIMETypes["bh2"]
   put "application/vnd.fujixerox.ddd" into sMIMETypes["ddd"]
   put "application/vnd.fujixerox.docuworks" into sMIMETypes["xdw"]
   put "application/vnd.fujixerox.docuworks.binder" into sMIMETypes["xbd"]
   put "application/vnd.fuzzysheet" into sMIMETypes["fzs"]
   put "application/vnd.genomatix.tuxedo" into sMIMETypes["txd"]
   put "application/vnd.geogebra.file" into sMIMETypes["ggb"]
   put "application/vnd.geogebra.tool" into sMIMETypes["ggt"]
   put "application/vnd.geometry-explorer" into sMIMETypes["gex"]
   put "application/vnd.geometry-explorer" into sMIMETypes["gre"]
   put "application/vnd.geonext" into sMIMETypes["gxt"]
   put "application/vnd.geoplan" into sMIMETypes["g2w"]
   put "application/vnd.geospace" into sMIMETypes["g3w"]
   put "application/vnd.gmx" into sMIMETypes["gmx"]
   put "application/vnd.google-earth.kml+xml" into sMIMETypes["kml"]
   put "application/vnd.google-earth.kmz" into sMIMETypes["kmz"]
   put "application/vnd.grafeq" into sMIMETypes["gqf"]
   put "application/vnd.grafeq" into sMIMETypes["gqs"]
   put "application/vnd.groove-account" into sMIMETypes["gac"]
   put "application/vnd.groove-help" into sMIMETypes["ghf"]
   put "application/vnd.groove-identity-message" into sMIMETypes["gim"]
   put "application/vnd.groove-injector" into sMIMETypes["grv"]
   put "application/vnd.groove-tool-message" into sMIMETypes["gtm"]
   put "application/vnd.groove-tool-template" into sMIMETypes["tpl"]
   put "application/vnd.groove-vcard" into sMIMETypes["vcg"]
   put "application/vnd.hal+xml" into sMIMETypes["hal"]
   put "application/vnd.handheld-entertainment+xml" into sMIMETypes["zmm"]
   put "application/vnd.hbci" into sMIMETypes["hbci"]
   put "application/vnd.hhe.lesson-player" into sMIMETypes["les"]
   put "application/vnd.hp-hpgl" into sMIMETypes["hpgl"]
   put "application/vnd.hp-hpid" into sMIMETypes["hpid"]
   put "application/vnd.hp-hps" into sMIMETypes["hps"]
   put "application/vnd.hp-jlyt" into sMIMETypes["jlt"]
   put "application/vnd.hp-pcl" into sMIMETypes["pcl"]
   put "application/vnd.hp-pclxl" into sMIMETypes["pclxl"]
   put "application/vnd.hydrostatix.sof-data" into sMIMETypes["sfd-hdstx"]
   put "application/vnd.ibm.minipay" into sMIMETypes["mpy"]
   put "application/vnd.ibm.modcap" into sMIMETypes["afp"]
   put "application/vnd.ibm.modcap" into sMIMETypes["list3820"]
   put "application/vnd.ibm.modcap" into sMIMETypes["listafp"]
   put "application/vnd.ibm.rights-management" into sMIMETypes["irm"]
   put "application/vnd.ibm.secure-container" into sMIMETypes["sc"]
   put "application/vnd.iccprofile" into sMIMETypes["icc"]
   put "application/vnd.iccprofile" into sMIMETypes["icm"]
   put "application/vnd.igloader" into sMIMETypes["igl"]
   put "application/vnd.immervision-ivp" into sMIMETypes["ivp"]
   put "application/vnd.immervision-ivu" into sMIMETypes["ivu"]
   put "application/vnd.insors.igm" into sMIMETypes["igm"]
   put "application/vnd.intercon.formnet" into sMIMETypes["xpw"]
   put "application/vnd.intercon.formnet" into sMIMETypes["xpx"]
   put "application/vnd.intergeo" into sMIMETypes["i2g"]
   put "application/vnd.intu.qbo" into sMIMETypes["qbo"]
   put "application/vnd.intu.qfx" into sMIMETypes["qfx"]
   put "application/vnd.ipunplugged.rcprofile" into sMIMETypes["rcprofile"]
   put "application/vnd.irepository.package+xml" into sMIMETypes["irp"]
   put "application/vnd.is-xpr" into sMIMETypes["xpr"]
   put "application/vnd.isac.fcs" into sMIMETypes["fcs"]
   put "application/vnd.jam" into sMIMETypes["jam"]
   put "application/vnd.jcp.javame.midlet-rms" into sMIMETypes["rms"]
   put "application/vnd.jisp" into sMIMETypes["jisp"]
   put "application/vnd.joost.joda-archive" into sMIMETypes["joda"]
   put "application/vnd.kahootz" into sMIMETypes["ktr"]
   put "application/vnd.kahootz" into sMIMETypes["ktz"]
   put "application/vnd.kde.karbon" into sMIMETypes["karbon"]
   put "application/vnd.kde.kchart" into sMIMETypes["chrt"]
   put "application/vnd.kde.kformula" into sMIMETypes["kfo"]
   put "application/vnd.kde.kivio" into sMIMETypes["flw"]
   put "application/vnd.kde.kontour" into sMIMETypes["kon"]
   put "application/vnd.kde.kpresenter" into sMIMETypes["kpr"]
   put "application/vnd.kde.kpresenter" into sMIMETypes["kpt"]
   put "application/vnd.kde.kspread" into sMIMETypes["ksp"]
   put "application/vnd.kde.kword" into sMIMETypes["kwd"]
   put "application/vnd.kde.kword" into sMIMETypes["kwt"]
   put "application/vnd.kenameaapp" into sMIMETypes["htke"]
   put "application/vnd.kidspiration" into sMIMETypes["kia"]
   put "application/vnd.kinar" into sMIMETypes["kne"]
   put "application/vnd.kinar" into sMIMETypes["knp"]
   put "application/vnd.koan" into sMIMETypes["skd"]
   put "application/vnd.koan" into sMIMETypes["skm"]
   put "application/vnd.koan" into sMIMETypes["skp"]
   put "application/vnd.koan" into sMIMETypes["skt"]
   put "application/vnd.kodak-descriptor" into sMIMETypes["sse"]
   put "application/vnd.las.las+xml" into sMIMETypes["lasxml"]
   put "application/vnd.llamagraphics.life-balance.desktop" into sMIMETypes["lbd"]
   put "application/vnd.llamagraphics.life-balance.exchange+xml" into sMIMETypes["lbe"]
   put "application/vnd.lotus-1-2-3" into sMIMETypes["123"]
   put "application/vnd.lotus-approach" into sMIMETypes["apr"]
   put "application/vnd.lotus-freelance" into sMIMETypes["pre"]
   put "application/vnd.lotus-notes" into sMIMETypes["nsf"]
   put "application/vnd.lotus-organizer" into sMIMETypes["org"]
   put "application/vnd.lotus-screencam" into sMIMETypes["scm"]
   put "application/vnd.lotus-wordpro" into sMIMETypes["lwp"]
   put "application/vnd.macports.portpkg" into sMIMETypes["portpkg"]
   put "application/vnd.mcd" into sMIMETypes["mcd"]
   put "application/vnd.medcalcdata" into sMIMETypes["mc1"]
   put "application/vnd.mediastation.cdkey" into sMIMETypes["cdkey"]
   put "application/vnd.mfer" into sMIMETypes["mwf"]
   put "application/vnd.mfmp" into sMIMETypes["mfm"]
   put "application/vnd.micrografx.flo" into sMIMETypes["flo"]
   put "application/vnd.micrografx.igx" into sMIMETypes["igx"]
   put "application/vnd.mif" into sMIMETypes["mif"]
   put "application/vnd.mobius.daf" into sMIMETypes["daf"]
   put "application/vnd.mobius.dis" into sMIMETypes["dis"]
   put "application/vnd.mobius.mbk" into sMIMETypes["mbk"]
   put "application/vnd.mobius.mqy" into sMIMETypes["mqy"]
   put "application/vnd.mobius.msl" into sMIMETypes["msl"]
   put "application/vnd.mobius.plc" into sMIMETypes["plc"]
   put "application/vnd.mobius.txf" into sMIMETypes["txf"]
   put "application/vnd.mophun.application" into sMIMETypes["mpn"]
   put "application/vnd.mophun.certificate" into sMIMETypes["mpc"]
   put "application/vnd.mozilla.xul+xml" into sMIMETypes["xul"]
   put "application/vnd.ms-artgalry" into sMIMETypes["cil"]
   put "application/vnd.ms-cab-compressed" into sMIMETypes["cab"]
   put "application/vnd.ms-excel" into sMIMETypes["xla"]
   put "application/vnd.ms-excel" into sMIMETypes["xlc"]
   put "application/vnd.ms-excel" into sMIMETypes["xlm"]
   put "application/vnd.ms-excel" into sMIMETypes["xls"]
   put "application/vnd.ms-excel" into sMIMETypes["xlt"]
   put "application/vnd.ms-excel" into sMIMETypes["xlw"]
   put "application/vnd.ms-excel.addin.macroenabled.12" into sMIMETypes["xlam"]
   put "application/vnd.ms-excel.sheet.binary.macroenabled.12" into sMIMETypes["xlsb"]
   put "application/vnd.ms-excel.sheet.macroenabled.12" into sMIMETypes["xlsm"]
   put "application/vnd.ms-excel.template.macroenabled.12" into sMIMETypes["xltm"]
   put "application/vnd.ms-fontobject" into sMIMETypes["eot"]
   put "application/vnd.ms-htmlhelp" into sMIMETypes["chm"]
   put "application/vnd.ms-ims" into sMIMETypes["ims"]
   put "application/vnd.ms-lrm" into sMIMETypes["lrm"]
   put "application/vnd.ms-officetheme" into sMIMETypes["thmx"]
   put "application/vnd.ms-pki.seccat" into sMIMETypes["cat"]
   put "application/vnd.ms-pki.stl" into sMIMETypes["stl"]
   put "application/vnd.ms-powerpoint" into sMIMETypes["pot"]
   put "application/vnd.ms-powerpoint" into sMIMETypes["pps"]
   put "application/vnd.ms-powerpoint" into sMIMETypes["ppt"]
   put "application/vnd.ms-powerpoint.addin.macroenabled.12" into sMIMETypes["ppam"]
   put "application/vnd.ms-powerpoint.presentation.macroenabled.12" into sMIMETypes["pptm"]
   put "application/vnd.ms-powerpoint.slide.macroenabled.12" into sMIMETypes["sldm"]
   put "application/vnd.ms-powerpoint.slideshow.macroenabled.12" into sMIMETypes["ppsm"]
   put "application/vnd.ms-powerpoint.template.macroenabled.12" into sMIMETypes["potm"]
   put "application/vnd.ms-project" into sMIMETypes["mpp"]
   put "application/vnd.ms-project" into sMIMETypes["mpt"]
   put "application/vnd.ms-word.document.macroenabled.12" into sMIMETypes["docm"]
   put "application/vnd.ms-word.template.macroenabled.12" into sMIMETypes["dotm"]
   put "application/vnd.ms-works" into sMIMETypes["wcm"]
   put "application/vnd.ms-works" into sMIMETypes["wdb"]
   put "application/vnd.ms-works" into sMIMETypes["wks"]
   put "application/vnd.ms-works" into sMIMETypes["wps"]
   put "application/vnd.ms-wpl" into sMIMETypes["wpl"]
   put "application/vnd.ms-xpsdocument" into sMIMETypes["xps"]
   put "application/vnd.mseq" into sMIMETypes["mseq"]
   put "application/vnd.musician" into sMIMETypes["mus"]
   put "application/vnd.muvee.style" into sMIMETypes["msty"]
   put "application/vnd.mynfc" into sMIMETypes["taglet"]
   put "application/vnd.neurolanguage.nlu" into sMIMETypes["nlu"]
   put "application/vnd.nitf" into sMIMETypes["nitf"]
   put "application/vnd.nitf" into sMIMETypes["ntf"]
   put "application/vnd.noblenet-directory" into sMIMETypes["nnd"]
   put "application/vnd.noblenet-sealer" into sMIMETypes["nns"]
   put "application/vnd.noblenet-web" into sMIMETypes["nnw"]
   put "application/vnd.nokia.n-gage.data" into sMIMETypes["ngdat"]
   put "application/vnd.nokia.n-gage.symbian.install" into sMIMETypes["n-gage"]
   put "application/vnd.nokia.radio-preset" into sMIMETypes["rpst"]
   put "application/vnd.nokia.radio-presets" into sMIMETypes["rpss"]
   put "application/vnd.novadigm.edm" into sMIMETypes["edm"]
   put "application/vnd.novadigm.edx" into sMIMETypes["edx"]
   put "application/vnd.novadigm.ext" into sMIMETypes["ext"]
   put "application/vnd.oasis.opendocument.chart" into sMIMETypes["odc"]
   put "application/vnd.oasis.opendocument.chart-template" into sMIMETypes["otc"]
   put "application/vnd.oasis.opendocument.database" into sMIMETypes["odb"]
   put "application/vnd.oasis.opendocument.formula" into sMIMETypes["odf"]
   put "application/vnd.oasis.opendocument.formula-template" into sMIMETypes["odft"]
   put "application/vnd.oasis.opendocument.graphics" into sMIMETypes["odg"]
   put "application/vnd.oasis.opendocument.graphics-template" into sMIMETypes["otg"]
   put "application/vnd.oasis.opendocument.image" into sMIMETypes["odi"]
   put "application/vnd.oasis.opendocument.image-template" into sMIMETypes["oti"]
   put "application/vnd.oasis.opendocument.presentation" into sMIMETypes["odp"]
   put "application/vnd.oasis.opendocument.presentation-template" into sMIMETypes["otp"]
   put "application/vnd.oasis.opendocument.spreadsheet" into sMIMETypes["ods"]
   put "application/vnd.oasis.opendocument.spreadsheet-template" into sMIMETypes["ots"]
   put "application/vnd.oasis.opendocument.text" into sMIMETypes["odt"]
   put "application/vnd.oasis.opendocument.text-master" into sMIMETypes["odm"]
   put "application/vnd.oasis.opendocument.text-template" into sMIMETypes["ott"]
   put "application/vnd.oasis.opendocument.text-web" into sMIMETypes["oth"]
   put "application/vnd.olpc-sugar" into sMIMETypes["xo"]
   put "application/vnd.oma.dd2+xml" into sMIMETypes["dd2"]
   put "application/vnd.openofficeorg.extension" into sMIMETypes["oxt"]
   put "application/vnd.openxmlformats-officedocument.presentationml.presentation" into sMIMETypes["pptx"]
   put "application/vnd.openxmlformats-officedocument.presentationml.slide" into sMIMETypes["sldx"]
   put "application/vnd.openxmlformats-officedocument.presentationml.slideshow" into sMIMETypes["ppsx"]
   put "application/vnd.openxmlformats-officedocument.presentationml.template" into sMIMETypes["potx"]
   put "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet" into sMIMETypes["xlsx"]
   put "application/vnd.openxmlformats-officedocument.spreadsheetml.template" into sMIMETypes["xltx"]
   put "application/vnd.openxmlformats-officedocument.wordprocessingml.document" into sMIMETypes["docx"]
   put "application/vnd.openxmlformats-officedocument.wordprocessingml.template" into sMIMETypes["dotx"]
   put "application/vnd.osgeo.mapguide.package" into sMIMETypes["mgp"]
   put "application/vnd.osgi.dp" into sMIMETypes["dp"]
   put "application/vnd.osgi.subsystem" into sMIMETypes["esa"]
   put "application/vnd.palm" into sMIMETypes["oprc"]
   put "application/vnd.palm" into sMIMETypes["pdb"]
   put "application/vnd.palm" into sMIMETypes["pqa"]
   put "application/vnd.pawaafile" into sMIMETypes["paw"]
   put "application/vnd.pg.format" into sMIMETypes["str"]
   put "application/vnd.pg.osasli" into sMIMETypes["ei6"]
   put "application/vnd.picsel" into sMIMETypes["efif"]
   put "application/vnd.pmi.widget" into sMIMETypes["wg"]
   put "application/vnd.pocketlearn" into sMIMETypes["plf"]
   put "application/vnd.powerbuilder6" into sMIMETypes["pbd"]
   put "application/vnd.previewsystems.box" into sMIMETypes["box"]
   put "application/vnd.proteus.magazine" into sMIMETypes["mgz"]
   put "application/vnd.publishare-delta-tree" into sMIMETypes["qps"]
   put "application/vnd.pvi.ptid1" into sMIMETypes["ptid"]
   put "application/vnd.quark.quarkxpress" into sMIMETypes["qwd"]
   put "application/vnd.quark.quarkxpress" into sMIMETypes["qwt"]
   put "application/vnd.quark.quarkxpress" into sMIMETypes["qxb"]
   put "application/vnd.quark.quarkxpress" into sMIMETypes["qxd"]
   put "application/vnd.quark.quarkxpress" into sMIMETypes["qxl"]
   put "application/vnd.quark.quarkxpress" into sMIMETypes["qxt"]
   put "application/vnd.realvnc.bed" into sMIMETypes["bed"]
   put "application/vnd.recordare.musicxml" into sMIMETypes["mxl"]
   put "application/vnd.recordare.musicxml+xml" into sMIMETypes["musicxml"]
   put "application/vnd.rig.cryptonote" into sMIMETypes["cryptonote"]
   put "application/vnd.rim.cod" into sMIMETypes["cod"]
   put "application/vnd.rn-realmedia" into sMIMETypes["rm"]
   put "application/vnd.rn-realmedia-vbr" into sMIMETypes["rmvb"]
   put "application/vnd.route66.link66+xml" into sMIMETypes["link66"]
   put "application/vnd.sailingtracker.track" into sMIMETypes["st"]
   put "application/vnd.seemail" into sMIMETypes["see"]
   put "application/vnd.sema" into sMIMETypes["sema"]
   put "application/vnd.semd" into sMIMETypes["semd"]
   put "application/vnd.semf" into sMIMETypes["semf"]
   put "application/vnd.shana.informed.formdata" into sMIMETypes["ifm"]
   put "application/vnd.shana.informed.formtemplate" into sMIMETypes["itp"]
   put "application/vnd.shana.informed.interchange" into sMIMETypes["iif"]
   put "application/vnd.shana.informed.package" into sMIMETypes["ipk"]
   put "application/vnd.simtech-mindmapper" into sMIMETypes["twd"]
   put "application/vnd.simtech-mindmapper" into sMIMETypes["twds"]
   put "application/vnd.smaf" into sMIMETypes["mmf"]
   put "application/vnd.smart.teacher" into sMIMETypes["teacher"]
   put "application/vnd.solent.sdkm+xml" into sMIMETypes["sdkd"]
   put "application/vnd.solent.sdkm+xml" into sMIMETypes["sdkm"]
   put "application/vnd.spotfire.dxp" into sMIMETypes["dxp"]
   put "application/vnd.spotfire.sfs" into sMIMETypes["sfs"]
   put "application/vnd.stardivision.calc" into sMIMETypes["sdc"]
   put "application/vnd.stardivision.draw" into sMIMETypes["sda"]
   put "application/vnd.stardivision.impress" into sMIMETypes["sdd"]
   put "application/vnd.stardivision.math" into sMIMETypes["smf"]
   put "application/vnd.stardivision.writer" into sMIMETypes["sdw"]
   put "application/vnd.stardivision.writer" into sMIMETypes["vor"]
   put "application/vnd.stardivision.writer-global" into sMIMETypes["sgl"]
   put "application/vnd.stepmania.package" into sMIMETypes["smzip"]
   put "application/vnd.stepmania.stepchart" into sMIMETypes["sm"]
   put "application/vnd.sun.xml.calc" into sMIMETypes["sxc"]
   put "application/vnd.sun.xml.calc.template" into sMIMETypes["stc"]
   put "application/vnd.sun.xml.draw" into sMIMETypes["sxd"]
   put "application/vnd.sun.xml.draw.template" into sMIMETypes["std"]
   put "application/vnd.sun.xml.impress" into sMIMETypes["sxi"]
   put "application/vnd.sun.xml.impress.template" into sMIMETypes["sti"]
   put "application/vnd.sun.xml.math" into sMIMETypes["sxm"]
   put "application/vnd.sun.xml.writer" into sMIMETypes["sxw"]
   put "application/vnd.sun.xml.writer.global" into sMIMETypes["sxg"]
   put "application/vnd.sun.xml.writer.template" into sMIMETypes["stw"]
   put "application/vnd.sus-calendar" into sMIMETypes["sus"]
   put "application/vnd.sus-calendar" into sMIMETypes["susp"]
   put "application/vnd.svd" into sMIMETypes["svd"]
   put "application/vnd.symbian.install" into sMIMETypes["sis"]
   put "application/vnd.symbian.install" into sMIMETypes["sisx"]
   put "application/vnd.syncml+xml" into sMIMETypes["xsm"]
   put "application/vnd.syncml.dm+wbxml" into sMIMETypes["bdm"]
   put "application/vnd.syncml.dm+xml" into sMIMETypes["xdm"]
   put "application/vnd.tao.intent-module-archive" into sMIMETypes["tao"]
   put "application/vnd.tcpdump.pcap" into sMIMETypes["cap"]
   put "application/vnd.tcpdump.pcap" into sMIMETypes["dmp"]
   put "application/vnd.tcpdump.pcap" into sMIMETypes["pcap"]
   put "application/vnd.tmobile-livetv" into sMIMETypes["tmo"]
   put "application/vnd.trid.tpt" into sMIMETypes["tpt"]
   put "application/vnd.triscape.mxs" into sMIMETypes["mxs"]
   put "application/vnd.trueapp" into sMIMETypes["tra"]
   put "application/vnd.ufdl" into sMIMETypes["ufd"]
   put "application/vnd.ufdl" into sMIMETypes["ufdl"]
   put "application/vnd.uiq.theme" into sMIMETypes["utz"]
   put "application/vnd.umajin" into sMIMETypes["umj"]
   put "application/vnd.unity" into sMIMETypes["unityweb"]
   put "application/vnd.uoml+xml" into sMIMETypes["uoml"]
   put "application/vnd.vcx" into sMIMETypes["vcx"]
   put "application/vnd.visio" into sMIMETypes["vsd"]
   put "application/vnd.visio" into sMIMETypes["vss"]
   put "application/vnd.visio" into sMIMETypes["vst"]
   put "application/vnd.visio" into sMIMETypes["vsw"]
   put "application/vnd.visionary" into sMIMETypes["vis"]
   put "application/vnd.vsf" into sMIMETypes["vsf"]
   put "application/vnd.wap.wbxml" into sMIMETypes["wbxml"]
   put "application/vnd.wap.wmlc" into sMIMETypes["wmlc"]
   put "application/vnd.wap.wmlscriptc" into sMIMETypes["wmlsc"]
   put "application/vnd.webturbo" into sMIMETypes["wtb"]
   put "application/vnd.wolfram.player" into sMIMETypes["nbp"]
   put "application/vnd.wordperfect" into sMIMETypes["wpd"]
   put "application/vnd.wqd" into sMIMETypes["wqd"]
   put "application/vnd.wt.stf" into sMIMETypes["stf"]
   put "application/vnd.xara" into sMIMETypes["xar"]
   put "application/vnd.xfdl" into sMIMETypes["xfdl"]
   put "application/vnd.yamaha.hv-dic" into sMIMETypes["hvd"]
   put "application/vnd.yamaha.hv-script" into sMIMETypes["hvs"]
   put "application/vnd.yamaha.hv-voice" into sMIMETypes["hvp"]
   put "application/vnd.yamaha.openscoreformat" into sMIMETypes["osf"]
   put "application/vnd.yamaha.openscoreformat.osfpvg+xml" into sMIMETypes["osfpvg"]
   put "application/vnd.yamaha.smaf-audio" into sMIMETypes["saf"]
   put "application/vnd.yamaha.smaf-phrase" into sMIMETypes["spf"]
   put "application/vnd.yellowriver-custom-menu" into sMIMETypes["cmp"]
   put "application/vnd.zul" into sMIMETypes["zir"]
   put "application/vnd.zul" into sMIMETypes["zirz"]
   put "application/vnd.zzazz.deck+xml" into sMIMETypes["zaz"]
   put "application/voicexml+xml" into sMIMETypes["vxml"]
   put "application/widget" into sMIMETypes["wgt"]
   put "application/winhlp" into sMIMETypes["hlp"]
   put "application/wsdl+xml" into sMIMETypes["wsdl"]
   put "application/wspolicy+xml" into sMIMETypes["wspolicy"]
   put "application/x-7z-compressed" into sMIMETypes["7z"]
   put "application/x-abiword" into sMIMETypes["abw"]
   put "application/x-ace-compressed" into sMIMETypes["ace"]
   put "application/x-apple-diskimage" into sMIMETypes["dmg"]
   put "application/x-authorware-bin" into sMIMETypes["aab"]
   put "application/x-authorware-bin" into sMIMETypes["u32"]
   put "application/x-authorware-bin" into sMIMETypes["vox"]
   put "application/x-authorware-bin" into sMIMETypes["x32"]
   put "application/x-authorware-map" into sMIMETypes["aam"]
   put "application/x-authorware-seg" into sMIMETypes["aas"]
   put "application/x-bcpio" into sMIMETypes["bcpio"]
   put "application/x-bittorrent" into sMIMETypes["torrent"]
   put "application/x-blorb" into sMIMETypes["blb"]
   put "application/x-blorb" into sMIMETypes["blorb"]
   put "application/x-bzip" into sMIMETypes["bz"]
   put "application/x-bzip2" into sMIMETypes["boz"]
   put "application/x-bzip2" into sMIMETypes["bz2"]
   put "application/x-cbr" into sMIMETypes["cb7"]
   put "application/x-cbr" into sMIMETypes["cba"]
   put "application/x-cbr" into sMIMETypes["cbr"]
   put "application/x-cbr" into sMIMETypes["cbt"]
   put "application/x-cbr" into sMIMETypes["cbz"]
   put "application/x-cdlink" into sMIMETypes["vcd"]
   put "application/x-cfs-compressed" into sMIMETypes["cfs"]
   put "application/x-chat" into sMIMETypes["chat"]
   put "application/x-chess-pgn" into sMIMETypes["pgn"]
   put "application/x-conference" into sMIMETypes["nsc"]
   put "application/x-cpio" into sMIMETypes["cpio"]
   put "application/x-csh" into sMIMETypes["csh"]
   put "application/x-debian-package" into sMIMETypes["deb"]
   put "application/x-debian-package" into sMIMETypes["udeb"]
   put "application/x-dgc-compressed" into sMIMETypes["dgc"]
   put "application/x-director" into sMIMETypes["cct"]
   put "application/x-director" into sMIMETypes["cst"]
   put "application/x-director" into sMIMETypes["cxt"]
   put "application/x-director" into sMIMETypes["dcr"]
   put "application/x-director" into sMIMETypes["dir"]
   put "application/x-director" into sMIMETypes["dxr"]
   put "application/x-director" into sMIMETypes["fgd"]
   put "application/x-director" into sMIMETypes["swa"]
   put "application/x-director" into sMIMETypes["w3d"]
   put "application/x-doom" into sMIMETypes["wad"]
   put "application/x-dtbncx+xml" into sMIMETypes["ncx"]
   put "application/x-dtbook+xml" into sMIMETypes["dtb"]
   put "application/x-dtbresource+xml" into sMIMETypes["res"]
   put "application/x-dvi" into sMIMETypes["dvi"]
   put "application/x-envoy" into sMIMETypes["evy"]
   put "application/x-eva" into sMIMETypes["eva"]
   put "application/x-font-bdf" into sMIMETypes["bdf"]
   put "application/x-font-ghostscript" into sMIMETypes["gsf"]
   put "application/x-font-linux-psf" into sMIMETypes["psf"]
   put "application/x-font-otf" into sMIMETypes["otf"]
   put "application/x-font-pcf" into sMIMETypes["pcf"]
   put "application/x-font-snf" into sMIMETypes["snf"]
   put "application/x-font-ttf" into sMIMETypes["ttc"]
   put "application/x-font-ttf" into sMIMETypes["ttf"]
   put "application/x-font-type1" into sMIMETypes["afm"]
   put "application/x-font-type1" into sMIMETypes["pfa"]
   put "application/x-font-type1" into sMIMETypes["pfb"]
   put "application/x-font-type1" into sMIMETypes["pfm"]
   put "application/x-freearc" into sMIMETypes["arc"]
   put "application/x-futuresplash" into sMIMETypes["spl"]
   put "application/x-gca-compressed" into sMIMETypes["gca"]
   put "application/x-glulx" into sMIMETypes["ulx"]
   put "application/x-gnumeric" into sMIMETypes["gnumeric"]
   put "application/x-gramps-xml" into sMIMETypes["gramps"]
   put "application/x-gtar" into sMIMETypes["gtar"]
   put "application/x-hdf" into sMIMETypes["hdf"]
   put "application/x-install-instructions" into sMIMETypes["install"]
   put "application/x-iso9660-image" into sMIMETypes["iso"]
   put "application/x-java-jnlp-file" into sMIMETypes["jnlp"]
   put "application/x-latex" into sMIMETypes["latex"]
   put "application/x-lzh-compressed" into sMIMETypes["lha"]
   put "application/x-lzh-compressed" into sMIMETypes["lzh"]
   put "application/x-mie" into sMIMETypes["mie"]
   put "application/x-mobipocket-ebook" into sMIMETypes["mobi"]
   put "application/x-mobipocket-ebook" into sMIMETypes["prc"]
   put "application/x-ms-application" into sMIMETypes["application"]
   put "application/x-ms-shortcut" into sMIMETypes["lnk"]
   put "application/x-ms-wmd" into sMIMETypes["wmd"]
   put "application/x-ms-wmz" into sMIMETypes["wmz"]
   put "application/x-ms-xbap" into sMIMETypes["xbap"]
   put "application/x-msaccess" into sMIMETypes["mdb"]
   put "application/x-msbinder" into sMIMETypes["obd"]
   put "application/x-mscardfile" into sMIMETypes["crd"]
   put "application/x-msclip" into sMIMETypes["clp"]
   put "application/x-msdownload" into sMIMETypes["bat"]
   put "application/x-msdownload" into sMIMETypes["com"]
   put "application/x-msdownload" into sMIMETypes["dll"]
   put "application/x-msdownload" into sMIMETypes["exe"]
   put "application/x-msdownload" into sMIMETypes["msi"]
   put "application/x-msmediaview" into sMIMETypes["m13"]
   put "application/x-msmediaview" into sMIMETypes["m14"]
   put "application/x-msmediaview" into sMIMETypes["mvb"]
   put "application/x-msmetafile" into sMIMETypes["emf"]
   put "application/x-msmetafile" into sMIMETypes["emz"]
   put "application/x-msmetafile" into sMIMETypes["wmf"]
   put "application/x-msmetafile" into sMIMETypes["wmz"]
   put "application/x-msmoney" into sMIMETypes["mny"]
   put "application/x-mspublisher" into sMIMETypes["pub"]
   put "application/x-msschedule" into sMIMETypes["scd"]
   put "application/x-msterminal" into sMIMETypes["trm"]
   put "application/x-mswrite" into sMIMETypes["wri"]
   put "application/x-netcdf" into sMIMETypes["cdf"]
   put "application/x-netcdf" into sMIMETypes["nc"]
   put "application/x-nzb" into sMIMETypes["nzb"]
   put "application/x-pkcs12" into sMIMETypes["p12"]
   put "application/x-pkcs12" into sMIMETypes["pfx"]
   put "application/x-pkcs7-certificates" into sMIMETypes["p7b"]
   put "application/x-pkcs7-certificates" into sMIMETypes["spc"]
   put "application/x-pkcs7-certreqresp" into sMIMETypes["p7r"]
   put "application/x-rar-compressed" into sMIMETypes["rar"]
   put "application/x-research-info-systems" into sMIMETypes["ris"]
   put "application/x-sh" into sMIMETypes["sh"]
   put "application/x-shar" into sMIMETypes["shar"]
   put "application/x-shockwave-flash" into sMIMETypes["swf"]
   put "application/x-silverlight-app" into sMIMETypes["xap"]
   put "application/x-sql" into sMIMETypes["sql"]
   put "application/x-stuffit" into sMIMETypes["sit"]
   put "application/x-stuffitx" into sMIMETypes["sitx"]
   put "application/x-subrip" into sMIMETypes["srt"]
   put "application/x-sv4cpio" into sMIMETypes["sv4cpio"]
   put "application/x-sv4crc" into sMIMETypes["sv4crc"]
   put "application/x-t3vm-image" into sMIMETypes["t3"]
   put "application/x-tads" into sMIMETypes["gam"]
   put "application/x-tar" into sMIMETypes["tar"]
   put "application/x-tcl" into sMIMETypes["tcl"]
   put "application/x-tex" into sMIMETypes["tex"]
   put "application/x-tex-tfm" into sMIMETypes["tfm"]
   put "application/x-texinfo" into sMIMETypes["texi"]
   put "application/x-texinfo" into sMIMETypes["texinfo"]
   put "application/x-tgif" into sMIMETypes["obj"]
   put "application/x-ustar" into sMIMETypes["ustar"]
   put "application/x-wais-source" into sMIMETypes["src"]
   put "application/x-x509-ca-cert" into sMIMETypes["crt"]
   put "application/x-x509-ca-cert" into sMIMETypes["der"]
   put "application/x-xfig" into sMIMETypes["fig"]
   put "application/x-xliff+xml" into sMIMETypes["xlf"]
   put "application/x-xpinstall" into sMIMETypes["xpi"]
   put "application/x-xz" into sMIMETypes["xz"]
   put "application/x-zmachine" into sMIMETypes["z1"]
   put "application/x-zmachine" into sMIMETypes["z2"]
   put "application/x-zmachine" into sMIMETypes["z3"]
   put "application/x-zmachine" into sMIMETypes["z4"]
   put "application/x-zmachine" into sMIMETypes["z5"]
   put "application/x-zmachine" into sMIMETypes["z6"]
   put "application/x-zmachine" into sMIMETypes["z7"]
   put "application/x-zmachine" into sMIMETypes["z8"]
   put "application/xaml+xml" into sMIMETypes["xaml"]
   put "application/xcap-diff+xml" into sMIMETypes["xdf"]
   put "application/xenc+xml" into sMIMETypes["xenc"]
   put "application/xhtml+xml" into sMIMETypes["xht"]
   put "application/xhtml+xml" into sMIMETypes["xhtml"]
   put "application/xml" into sMIMETypes["xml"]
   put "application/xml" into sMIMETypes["xsl"]
   put "application/xml-dtd" into sMIMETypes["dtd"]
   put "application/xop+xml" into sMIMETypes["xop"]
   put "application/xproc+xml" into sMIMETypes["xpl"]
   put "application/xslt+xml" into sMIMETypes["xslt"]
   put "application/xspf+xml" into sMIMETypes["xspf"]
   put "application/xv+xml" into sMIMETypes["mxml"]
   put "application/xv+xml" into sMIMETypes["xhvml"]
   put "application/xv+xml" into sMIMETypes["xvm"]
   put "application/xv+xml" into sMIMETypes["xvml"]
   put "application/yang" into sMIMETypes["yang"]
   put "application/yin+xml" into sMIMETypes["yin"]
   put "application/zip" into sMIMETypes["zip"]
   put "audio/adpcm" into sMIMETypes["adp"]
   put "audio/basic" into sMIMETypes["au"]
   put "audio/basic" into sMIMETypes["snd"]
   put "audio/midi" into sMIMETypes["kar"]
   put "audio/midi" into sMIMETypes["mid"]
   put "audio/midi" into sMIMETypes["midi"]
   put "audio/midi" into sMIMETypes["rmi"]
   put "audio/mp4" into sMIMETypes["m4a"]
   put "audio/mp4" into sMIMETypes["mp4a"]
   put "audio/mpeg" into sMIMETypes["m2a"]
   put "audio/mpeg" into sMIMETypes["m3a"]
   put "audio/mpeg" into sMIMETypes["mp2"]
   put "audio/mpeg" into sMIMETypes["mp2a"]
   put "audio/mpeg" into sMIMETypes["mp3"]
   put "audio/mpeg" into sMIMETypes["mpga"]
   put "audio/ogg" into sMIMETypes["oga"]
   put "audio/ogg" into sMIMETypes["ogg"]
   put "audio/ogg" into sMIMETypes["spx"]
   put "audio/s3m" into sMIMETypes["s3m"]
   put "audio/silk" into sMIMETypes["sil"]
   put "audio/vnd.dece.audio" into sMIMETypes["uva"]
   put "audio/vnd.dece.audio" into sMIMETypes["uvva"]
   put "audio/vnd.digital-winds" into sMIMETypes["eol"]
   put "audio/vnd.dra" into sMIMETypes["dra"]
   put "audio/vnd.dts" into sMIMETypes["dts"]
   put "audio/vnd.dts.hd" into sMIMETypes["dtshd"]
   put "audio/vnd.lucent.voice" into sMIMETypes["lvp"]
   put "audio/vnd.ms-playready.media.pya" into sMIMETypes["pya"]
   put "audio/vnd.nuera.ecelp4800" into sMIMETypes["ecelp4800"]
   put "audio/vnd.nuera.ecelp7470" into sMIMETypes["ecelp7470"]
   put "audio/vnd.nuera.ecelp9600" into sMIMETypes["ecelp9600"]
   put "audio/vnd.rip" into sMIMETypes["rip"]
   put "audio/webm" into sMIMETypes["weba"]
   put "audio/x-aac" into sMIMETypes["aac"]
   put "audio/x-aiff" into sMIMETypes["aif"]
   put "audio/x-aiff" into sMIMETypes["aifc"]
   put "audio/x-aiff" into sMIMETypes["aiff"]
   put "audio/x-caf" into sMIMETypes["caf"]
   put "audio/x-flac" into sMIMETypes["flac"]
   put "audio/x-matroska" into sMIMETypes["mka"]
   put "audio/x-mpegurl" into sMIMETypes["m3u"]
   put "audio/x-ms-wax" into sMIMETypes["wax"]
   put "audio/x-ms-wma" into sMIMETypes["wma"]
   put "audio/x-pn-realaudio" into sMIMETypes["ra"]
   put "audio/x-pn-realaudio" into sMIMETypes["ram"]
   put "audio/x-pn-realaudio-plugin" into sMIMETypes["rmp"]
   put "audio/x-wav" into sMIMETypes["wav"]
   put "audio/xm" into sMIMETypes["xm"]
   put "chemical/x-cdx" into sMIMETypes["cdx"]
   put "chemical/x-cif" into sMIMETypes["cif"]
   put "chemical/x-cmdf" into sMIMETypes["cmdf"]
   put "chemical/x-cml" into sMIMETypes["cml"]
   put "chemical/x-csml" into sMIMETypes["csml"]
   put "chemical/x-xyz" into sMIMETypes["xyz"]
   put "image/bmp" into sMIMETypes["bmp"]
   put "image/cgm" into sMIMETypes["cgm"]
   put "image/g3fax" into sMIMETypes["g3"]
   put "image/gif" into sMIMETypes["gif"]
   put "image/ief" into sMIMETypes["ief"]
   put "image/jpeg" into sMIMETypes["jpe"]
   put "image/jpeg" into sMIMETypes["jpeg"]
   put "image/jpeg" into sMIMETypes["jpg"]
   put "image/ktx" into sMIMETypes["ktx"]
   put "image/png" into sMIMETypes["png"]
   put "image/prs.btif" into sMIMETypes["btif"]
   put "image/sgi" into sMIMETypes["sgi"]
   put "image/svg+xml" into sMIMETypes["svg"]
   put "image/svg+xml" into sMIMETypes["svgz"]
   put "image/tiff" into sMIMETypes["tif"]
   put "image/tiff" into sMIMETypes["tiff"]
   put "image/vnd.adobe.photoshop" into sMIMETypes["psd"]
   put "image/vnd.dece.graphic" into sMIMETypes["uvg"]
   put "image/vnd.dece.graphic" into sMIMETypes["uvi"]
   put "image/vnd.dece.graphic" into sMIMETypes["uvvg"]
   put "image/vnd.dece.graphic" into sMIMETypes["uvvi"]
   put "image/vnd.djvu" into sMIMETypes["djv"]
   put "image/vnd.djvu" into sMIMETypes["djvu"]
   put "image/vnd.dvb.subtitle" into sMIMETypes["sub"]
   put "image/vnd.dwg" into sMIMETypes["dwg"]
   put "image/vnd.dxf" into sMIMETypes["dxf"]
   put "image/vnd.fastbidsheet" into sMIMETypes["fbs"]
   put "image/vnd.fpx" into sMIMETypes["fpx"]
   put "image/vnd.fst" into sMIMETypes["fst"]
   put "image/vnd.fujixerox.edmics-mmr" into sMIMETypes["mmr"]
   put "image/vnd.fujixerox.edmics-rlc" into sMIMETypes["rlc"]
   put "image/vnd.ms-modi" into sMIMETypes["mdi"]
   put "image/vnd.ms-photo" into sMIMETypes["wdp"]
   put "image/vnd.net-fpx" into sMIMETypes["npx"]
   put "image/vnd.wap.wbmp" into sMIMETypes["wbmp"]
   put "image/vnd.xiff" into sMIMETypes["xif"]
   put "image/webp" into sMIMETypes["webp"]
   put "image/x-3ds" into sMIMETypes["3ds"]
   put "image/x-cmu-raster" into sMIMETypes["ras"]
   put "image/x-cmx" into sMIMETypes["cmx"]
   put "image/x-freehand" into sMIMETypes["fh"]
   put "image/x-freehand" into sMIMETypes["fh4"]
   put "image/x-freehand" into sMIMETypes["fh5"]
   put "image/x-freehand" into sMIMETypes["fh7"]
   put "image/x-freehand" into sMIMETypes["fhc"]
   put "image/x-icon" into sMIMETypes["ico"]
   put "image/x-mrsid-image" into sMIMETypes["sid"]
   put "image/x-pcx" into sMIMETypes["pcx"]
   put "image/x-pict" into sMIMETypes["pct"]
   put "image/x-pict" into sMIMETypes["pic"]
   put "image/x-portable-anymap" into sMIMETypes["pnm"]
   put "image/x-portable-bitmap" into sMIMETypes["pbm"]
   put "image/x-portable-graymap" into sMIMETypes["pgm"]
   put "image/x-portable-pixmap" into sMIMETypes["ppm"]
   put "image/x-rgb" into sMIMETypes["rgb"]
   put "image/x-tga" into sMIMETypes["tga"]
   put "image/x-xbitmap" into sMIMETypes["xbm"]
   put "image/x-xpixmap" into sMIMETypes["xpm"]
   put "image/x-xwindowdump" into sMIMETypes["xwd"]
   put "message/rfc822" into sMIMETypes["eml"]
   put "message/rfc822" into sMIMETypes["mime"]
   put "model/iges" into sMIMETypes["iges"]
   put "model/iges" into sMIMETypes["igs"]
   put "model/mesh" into sMIMETypes["mesh"]
   put "model/mesh" into sMIMETypes["msh"]
   put "model/mesh" into sMIMETypes["silo"]
   put "model/vnd.collada+xml" into sMIMETypes["dae"]
   put "model/vnd.dwf" into sMIMETypes["dwf"]
   put "model/vnd.gdl" into sMIMETypes["gdl"]
   put "model/vnd.gtw" into sMIMETypes["gtw"]
   put "model/vnd.mts" into sMIMETypes["mts"]
   put "model/vnd.vtu" into sMIMETypes["vtu"]
   put "model/vrml" into sMIMETypes["vrml"]
   put "model/vrml" into sMIMETypes["wrl"]
   put "model/x3d+binary" into sMIMETypes["x3db"]
   put "model/x3d+binary" into sMIMETypes["x3dbz"]
   put "model/x3d+vrml" into sMIMETypes["x3dv"]
   put "model/x3d+vrml" into sMIMETypes["x3dvz"]
   put "model/x3d+xml" into sMIMETypes["x3d"]
   put "model/x3d+xml" into sMIMETypes["x3dz"]
   put "text/cache-manifest" into sMIMETypes["appcache"]
   put "text/calendar" into sMIMETypes["ics"]
   put "text/calendar" into sMIMETypes["ifb"]
   put "text/css" into sMIMETypes["css"]
   put "text/csv" into sMIMETypes["csv"]
   put "text/html" into sMIMETypes["htm"]
   put "text/html" into sMIMETypes["html"]
   put "text/n3" into sMIMETypes["n3"]
   put "text/plain" into sMIMETypes["conf"]
   put "text/plain" into sMIMETypes["def"]
   put "text/plain" into sMIMETypes["in"]
   put "text/plain" into sMIMETypes["list"]
   put "text/plain" into sMIMETypes["log"]
   put "text/plain" into sMIMETypes["text"]
   put "text/plain" into sMIMETypes["txt"]
   put "text/prs.lines.tag" into sMIMETypes["dsc"]
   put "text/richtext" into sMIMETypes["rtx"]
   put "text/sgml" into sMIMETypes["sgm"]
   put "text/sgml" into sMIMETypes["sgml"]
   put "text/tab-separated-values" into sMIMETypes["tsv"]
   put "text/troff" into sMIMETypes["man"]
   put "text/troff" into sMIMETypes["me"]
   put "text/troff" into sMIMETypes["ms"]
   put "text/troff" into sMIMETypes["roff"]
   put "text/troff" into sMIMETypes["t"]
   put "text/troff" into sMIMETypes["tr"]
   put "text/turtle" into sMIMETypes["ttl"]
   put "text/uri-list" into sMIMETypes["uri"]
   put "text/uri-list" into sMIMETypes["uris"]
   put "text/uri-list" into sMIMETypes["urls"]
   put "text/vcard" into sMIMETypes["vcard"]
   put "text/vnd.curl" into sMIMETypes["curl"]
   put "text/vnd.curl.dcurl" into sMIMETypes["dcurl"]
   put "text/vnd.curl.mcurl" into sMIMETypes["mcurl"]
   put "text/vnd.curl.scurl" into sMIMETypes["scurl"]
   put "text/vnd.dvb.subtitle" into sMIMETypes["sub"]
   put "text/vnd.fly" into sMIMETypes["fly"]
   put "text/vnd.fmi.flexstor" into sMIMETypes["flx"]
   put "text/vnd.graphviz" into sMIMETypes["gv"]
   put "text/vnd.in3d.3dml" into sMIMETypes["3dml"]
   put "text/vnd.in3d.spot" into sMIMETypes["spot"]
   put "text/vnd.sun.j2me.app-descriptor" into sMIMETypes["jad"]
   put "text/vnd.wap.wml" into sMIMETypes["wml"]
   put "text/vnd.wap.wmlscript" into sMIMETypes["wmls"]
   put "text/x-asm" into sMIMETypes["asm"]
   put "text/x-asm" into sMIMETypes["s"]
   put "text/x-c" into sMIMETypes["c"]
   put "text/x-c" into sMIMETypes["cc"]
   put "text/x-c" into sMIMETypes["cpp"]
   put "text/x-c" into sMIMETypes["cxx"]
   put "text/x-c" into sMIMETypes["dic"]
   put "text/x-c" into sMIMETypes["h"]
   put "text/x-c" into sMIMETypes["hh"]
   put "text/x-fortran" into sMIMETypes["f"]
   put "text/x-fortran" into sMIMETypes["f77"]
   put "text/x-fortran" into sMIMETypes["f90"]
   put "text/x-fortran" into sMIMETypes["for"]
   put "text/x-java-source" into sMIMETypes["java"]
   put "text/x-nfo" into sMIMETypes["nfo"]
   put "text/x-opml" into sMIMETypes["opml"]
   put "text/x-pascal" into sMIMETypes["p"]
   put "text/x-pascal" into sMIMETypes["pas"]
   put "text/x-setext" into sMIMETypes["etx"]
   put "text/x-sfv" into sMIMETypes["sfv"]
   put "text/x-uuencode" into sMIMETypes["uu"]
   put "text/x-vcalendar" into sMIMETypes["vcs"]
   put "text/x-vcard" into sMIMETypes["vcf"]
   put "video/3gpp" into sMIMETypes["3gp"]
   put "video/3gpp2" into sMIMETypes["3g2"]
   put "video/h261" into sMIMETypes["h261"]
   put "video/h263" into sMIMETypes["h263"]
   put "video/h264" into sMIMETypes["h264"]
   put "video/jpeg" into sMIMETypes["jpgv"]
   put "video/jpm" into sMIMETypes["jpgm"]
   put "video/jpm" into sMIMETypes["jpm"]
   put "video/mj2" into sMIMETypes["mj2"]
   put "video/mj2" into sMIMETypes["mjp2"]
   put "video/mp4" into sMIMETypes["mp4"]
   put "video/mp4" into sMIMETypes["mp4v"]
   put "video/mp4" into sMIMETypes["mpg4"]
   put "video/mpeg" into sMIMETypes["m1v"]
   put "video/mpeg" into sMIMETypes["m2v"]
   put "video/mpeg" into sMIMETypes["mpe"]
   put "video/mpeg" into sMIMETypes["mpeg"]
   put "video/mpeg" into sMIMETypes["mpg"]
   put "video/ogg" into sMIMETypes["ogv"]
   put "video/quicktime" into sMIMETypes["mov"]
   put "video/quicktime" into sMIMETypes["qt"]
   put "video/vnd.dece.hd" into sMIMETypes["uvh"]
   put "video/vnd.dece.hd" into sMIMETypes["uvvh"]
   put "video/vnd.dece.mobile" into sMIMETypes["uvm"]
   put "video/vnd.dece.mobile" into sMIMETypes["uvvm"]
   put "video/vnd.dece.pd" into sMIMETypes["uvp"]
   put "video/vnd.dece.pd" into sMIMETypes["uvvp"]
   put "video/vnd.dece.sd" into sMIMETypes["uvs"]
   put "video/vnd.dece.sd" into sMIMETypes["uvvs"]
   put "video/vnd.dece.video" into sMIMETypes["uvv"]
   put "video/vnd.dece.video" into sMIMETypes["uvvv"]
   put "video/vnd.dvb.file" into sMIMETypes["dvb"]
   put "video/vnd.fvt" into sMIMETypes["fvt"]
   put "video/vnd.mpegurl" into sMIMETypes["m4u"]
   put "video/vnd.mpegurl" into sMIMETypes["mxu"]
   put "video/vnd.ms-playready.media.pyv" into sMIMETypes["pyv"]
   put "video/vnd.uvvu.mp4" into sMIMETypes["uvu"]
   put "video/vnd.uvvu.mp4" into sMIMETypes["uvvu"]
   put "video/vnd.vivo" into sMIMETypes["viv"]
   put "video/webm" into sMIMETypes["webm"]
   put "video/x-f4v" into sMIMETypes["f4v"]
   put "video/x-fli" into sMIMETypes["fli"]
   put "video/x-flv" into sMIMETypes["flv"]
   put "video/x-m4v" into sMIMETypes["m4v"]
   put "video/x-matroska" into sMIMETypes["mk3d"]
   put "video/x-matroska" into sMIMETypes["mks"]
   put "video/x-matroska" into sMIMETypes["mkv"]
   put "video/x-mng" into sMIMETypes["mng"]
   put "video/x-ms-asf" into sMIMETypes["asf"]
   put "video/x-ms-asf" into sMIMETypes["asx"]
   put "video/x-ms-vob" into sMIMETypes["vob"]
   put "video/x-ms-wm" into sMIMETypes["wm"]
   put "video/x-ms-wmv" into sMIMETypes["wmv"]
   put "video/x-ms-wmx" into sMIMETypes["wmx"]
   put "video/x-ms-wvx" into sMIMETypes["wvx"]
   put "video/x-msvideo" into sMIMETypes["avi"]
   put "video/x-sgi-movie" into sMIMETypes["movie"]
   put "video/x-smv" into sMIMETypes["smv"]
   put "x-conference/x-cooltalk" into sMIMETypes["ice"]
end __Initialize
