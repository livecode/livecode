script "com.livecode.library.messageauthentication"
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
end extensionInitialize

on extensionFinalize
   if the target is not me then
      pass extensionFinalize
   end if
   
   remove the script of me from back
end extensionFinalize

/**
Title: Message Authentication Library

Version: 1.0.0

Author: LiveCode

Description:
This script library implements message authentication codes
*/

/**
Compute a message authentication code.

Syntax:
messageAuthenticationCode(<message>, <key>, <codeType>)

Example:
local tHMAC
put messageAuthenticationCode("My Data", "My Secret", "HMAC-SHA-256") into tHMAC

Description:

<messageAuthenticationCode> provides a way to check the integrity of
information transmitted over or stored in an unreliable medium is a
prime necessity in the world of open computing and communications.
Mechanisms that provide such integrity check based on a secret key are
usually called "message authentication codes" (MAC). Typically, message
authentication codes are used between two parties that share a secret
key in order to validate information transmitted between these
parties. <messageAuthenticationCode> also uses a secret key for calculation
of the message authentication values.

Parameters:

message (data): A <binary data> string.

key (data): A <binary data> string.

codeType(enum): The cryptographic hash function to use
- "HMAC-SHA3-224":
- "HMAC-SHA3-256":
- "HMAC-SHA3-384":
- "HMAC-SHA3-512":
- "HMAC-SHA-224":
- "HMAC-SHA-256":
- "HMAC-SHA-384":
- "HMAC-SHA-512":
- "HMAC-SHA-1": Use only for backwards compatibility
- "HMAC-MD5": Use only for backwards compatibility

Returns(data):
Compute a message digest of <message>

References: messageDigest (function), binary data (glossary)

*/

function messageAuthenticationCode pMessage, pKey, pCodeType
   if pCodeType begins with "HMAC-" then
      return __HMAC(pMessage, pKey, char 6 to -1 of pCodeType)
   else
      throw "Unsupported code type"
   end if
end messageAuthenticationCode

constant kIpad = 0x36
constant kOpad = 0x5C

private function __HMAC pMessage, pKey, pDigestType
   local tBlockSize
   put __BlockSize(pDigestType) into tBlockSize
   if the result is not empty then
      throw the result
   end if
   
   --    (1) append zeros to the end of K to create a B byte string
   --    (e.g., if K is of length 20 bytes and B=64, then K will be
   --    appended with 44 zero bytes 0x00)
   
   if the length of pKey > tBlockSize then
      put messageDigest(pKey, pDigestType) into pKey
   end if
   
   repeat while the length of pKey < tBlockSize
      put null after pKey
   end repeat
   
   --    (2) XOR (bitwise exclusive-OR) the B byte string computed in step
   --    (1) with ipad
   
   local tByte
   local tIpadSecret
   repeat for each byte tByte in pKey
      put numToByte(byteToNum(tByte) bitXOR kIpad) after tIpadSecret
   end repeat
   
   --    (3) append the stream of data 'text' to the B byte string resulting
   --    from step (2)
   
   put tIpadSecret before pMessage
   
   --    (4) apply H to the stream generated in step (3)
   
   local tHash
   put messageDigest(pMessage, pDigestType) into tHash
   
   --    (5) XOR (bitwise exclusive-OR) the B byte string computed in
   --    step (1) with opad
   
   local tOpadSecret
   repeat for each byte tByte in pKey
      put numToByte(byteToNum(tByte) bitXOR kOpad) after tOpadSecret
   end repeat
   
   --    (6) append the H result from step (4) to the B byte string
   --    resulting from step (5)
   
   put tOpadSecret before tHash
   
   --    (7) apply H to the stream generated in step (6) and output
   --    the result
   
   return messageDigest(tHash, pDigestType)
end __HMAC

private function __BlockSize pDigestType
   switch pDigestType
      case "MD5"
      case "SHA-1"
      case "SHA-224"
      case "SHA-256"
         return 64 for value
      case "SHA-384"
      case "SHA-512"
         return 128 for value
      case "SHA3-224"
         return 144 for value
      case "SHA3-256"
         return 136 for value
      case "SHA3-384"
         return 104 for value
      case "SHA3-512"
         return 72 for value
   end switch
   
   return "Unimplemented message digest type" for error
end __BlockSize
