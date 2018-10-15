script "CoreSecurity"
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

on TestSetup
   TestSkipIf "platform", "HTML5"
end TestSetup

on TestRSA
   
   local tPublicPKCS8, tPublicPKCS1, tPrivatePKCS8, tPrivatePKCS1
   put url ("binfile:" & TestGetInputFile("/public_key.pkcs8")) into tPublicPKCS8
   put url ("binfile:" & TestGetInputFile("/public_key.pkcs1")) into tPublicPKCS1
   put url ("binfile:" & TestGetInputFile("/private_key.pkcs8")) into tPrivatePKCS8
   put url ("binfile:" & TestGetInputFile("/private_key.pkcs1")) into tPrivatePKCS1
   
   __TestRoundTripRSA "foo", tPublicPKCS1, "PKCS1", tPrivatePKCS1, "PKCS1"
   __TestRoundTripRSA "foo", tPublicPKCS8, "PKCS8", tPrivatePKCS8, "PKCS8"
   __TestRoundTripRSA "foo", tPublicPKCS1, "PKCS1", tPrivatePKCS8, "PKCS8"
   __TestRoundTripRSA "foo", tPublicPKCS8, "PKCS8", tPrivatePKCS1, "PKCS1"
end TestRSA

on TestRSAPass
   local tPublicPKCS8, tPrivatePKCS1
   put url ("binfile:" & TestGetInputFile("/public_key_pass.pkcs8")) into tPublicPKCS8
   put url ("binfile:" & TestGetInputFile("/private_key_pass.pkcs1")) into tPrivatePKCS1
   
   encrypt "foo" using rsa with private key tPrivatePKCS1 and passphrase "foobar"
   decrypt it using rsa with public key tPublicPKCS8
   TestAssert "decrypt with public key good pass", it is "foo"
   
   get empty
   encrypt "foo" using rsa with public key tPublicPKCS8
   decrypt it using rsa with private key tPrivatePKCS1 and passphrase "foobar"
   TestAssert "decrypt with private key good pass", it is "foo"
   
   get empty
   encrypt "foo" using rsa with private key tPrivatePKCS1 and passphrase "baz"
   TestAssert "encrypt with private key bad pass", it is empty and the result is not empty
   
   get empty
   encrypt "foo" using rsa with public key tPublicPKCS8
   decrypt it using rsa with private key tPrivatePKCS1 and passphrase "baz"
   TestAssert "decrypt with private key bad pass", it is not "foo" and the result is not empty
end TestRSAPass

private command __TestRoundTripRSA pString, pPublicKey, pPublicKeyFormat, pPrivateKey, pPrivateKeyFormat
   local tResult
   encrypt pString using rsa with private key pPrivateKey
   put the result into tResult
   TestAssert "encrypt with private key" & tPass && pPrivateKeyFormat, tResult is empty
   
   -- skipping test because encrypt failed
   if tResult is empty then
      decrypt it using rsa with public key pPublicKey
      put the result into tResult
      TestAssert "decrypt with public key" & tPass && pPublicKeyFormat, it is pString
      TestDiagnostic tResult
   end if
   
   encrypt pString using rsa with public key pPublicKey
   put the result into tResult
   TestAssert "encrypt with public key" & tPass && pPublicKeyFormat, tResult is empty
   TestDiagnostic tResult

   -- skipping test because encrypt failed
   if tResult is empty then
      decrypt it using rsa with private key pPrivateKey
      TestAssert "decrypt with private key" & tPass && pPrivateKeyFormat, it is pString
   end if
end __TestRoundTripRSA
