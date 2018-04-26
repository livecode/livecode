script "CoreFilters"
/*
Copyright (C) 2015-2017 LiveCode Ltd.

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

on TestFiltersBase64decode

TestAssert "test", base64decode("YWJjMTIz") is "abc123"

end TestFiltersBase64decode
on TestFiltersBase64encode

TestAssert "test", base64encode("abc123") is "YWJjMTIz"

end TestFiltersBase64encode
on TestFiltersConversionTests


local tVar = "initial value"
TestAssert "test", binaryDecode("x", "abcd", tVar) is 0
TestAssert "test", tVar is "initial value"

TestAssert "test", binaryDecode("a3", "abcd", tVar) is 1
TestAssert "test", tVar is "abc"

TestAssert "test", binaryDecode("A3", "ab cd", tVar) is 1
TestAssert "test", tVar is "ab"

TestAssert "test", binaryDecode("b16", "abcd", tVar) is 1
TestAssert "test", tVar is "10000110" & "01000110"

TestAssert "test", binaryDecode("B16", "abcd", tVar) is 1
TestAssert "test", tVar is "01100001" & "01100010"

TestAssert "test", binaryDecode("h4", "abcd", tVar) is 1
TestAssert "test", tVar is "16" & "26"

TestAssert "test", binaryDecode("H4", "abcd", tVar) is 1
TestAssert "test", tVar is "61" & "62"

TestAssert "test", binaryDecode("c1", numtoByte(255), tVar) is 1
TestAssert "test", tVar is -1

TestAssert "test", binaryDecode("C1", numtoByte(255), tVar) is 1
TestAssert "test", tVar is 255

TestAssert "test", binaryDecode("s1", numtoByte(255)&numToByte(255), tVar) is 1
TestAssert "test", tVar is -1

TestAssert "test", binaryDecode("S1", numtoByte(255)&numToByte(255), tVar) is 1
TestAssert "test", tVar is 65535

TestAssert "test", binaryDecode("i1", numtoByte(255)&numToByte(255)&numtoByte(255)&numToByte(255), tVar) is 1
TestAssert "test", tVar is -1

TestAssert "test", binaryDecode("I1", numtoByte(255)&numToByte(255)&numtoByte(255)&numToByte(255), tVar) is 1
TestAssert "test", tVar is 4294967295

TestAssert "test", binaryDecode("n1", numtoByte(255)&numToByte(0), tVar) is 1
TestAssert "test", tVar is -256

TestAssert "test", binaryDecode("m1", numtoByte(255)&numToByte(0), tVar) is 1
TestAssert "test", tVar is 65280

TestAssert "test", binaryDecode("N1", numtoByte(255)&numToByte(255)&numtoByte(0)&numToByte(0), tVar) is 1
TestAssert "test", tVar is -65536

TestAssert "test", binaryDecode("M1", numtoByte(255)&numToByte(255)&numtoByte(0)&numToByte(0), tVar) is 1
TestAssert "test", tVar is 4294901760

end TestFiltersConversionTests

on TestFiltersConversionTests2


TestAssert "test", binaryEncode("x3", "abcd") is null&null&null

TestAssert "test", binaryEncode("a3", "abcd") is "abc"

TestAssert "test", binaryEncode("A3", "ab") is "ab "

TestAssert "test", binaryEncode("b16", "10000110" & "01000110") is "ab"

TestAssert "test", binaryEncode("B16", "01100001" & "01100010") is "ab"

TestAssert "test", binaryEncode("h4", "16" & "26") is "ab"

TestAssert "test", binaryEncode("H4", "61" & "62") is "ab"

TestAssert "test", binaryEncode("c1", -1) is numtoByte(255)

TestAssert "test", binaryEncode("C1", 255) is numtoByte(255)

TestAssert "test", binaryEncode("s1", -1) is numtoByte(255)&numToByte(255)

TestAssert "test", binaryEncode("S1", 65535) is numtoByte(255)&numToByte(255)

TestAssert "test", binaryEncode("i1", -1) is numtoByte(255)&numToByte(255)&numtoByte(255)&numToByte(255)

TestAssert "test", binaryEncode("I1", 4294967295) is numtoByte(255)&numToByte(255)&numtoByte(255)&numToByte(255)

TestAssert "test", binaryEncode("n1", -256) is numtoByte(255)&numToByte(0)

TestAssert "test", binaryEncode("m1", 65280) is numtoByte(255)&numToByte(0)

TestAssert "test", binaryEncode("N1", -65536) is numtoByte(255)&numToByte(255)&numtoByte(0)&numToByte(0)

TestAssert "test", binaryEncode("M1", 4294901760) is numtoByte(255)&numToByte(255)&numtoByte(0)&numToByte(0)

end TestFiltersConversionTests2

on TestFiltersCompress

TestAssert "test", compress("hello world") is not "hello world"

end TestFiltersCompress


on TestFiltersFunctionality

TestAssert "test", decompress(compress(empty)) is empty
TestAssert "test", decompress(compress("hello world")) is "hello world"

end TestFiltersFunctionality

on TestFiltersIsoToMac
TestAssert "test", isoToMac(numToChar(225)) is numToChar(135)

end TestFiltersIsoToMac

on TestFiltersMacToIso

TestAssert "test", macToIso(numToChar(135)) is numToChar(225)
end TestFiltersMacToIso 

on TestFiltersMd5digest


TestAssert "test", md5digest(empty) is binaryEncode("h*", "4dd1c89df8002b409e089089ce8f24e7")
TestAssert "test", md5digest("abcd") is binaryEncode("h*", "2ecf17c47472ee39593f42dce2f733f1")

end TestFiltersMd5digest
on TestFiltersSha1digest


TestAssert "test", sha1digest(empty) is binaryEncode("h*", "ad933aeee5b6b4d02355fbfe59068109fa8d7090")
TestAssert "test", sha1digest("abcd") is binaryEncode("h*", "18efb8ef7875c6e3bc2224f6e87548372819a7fc")

end TestFiltersSha1digest

private function MessageDigest_ToString pDigest
   local tString, tByte
   repeat for each byte tByte in pDigest
      put format("%02x", byteToNum(tByte)) after tString
   end repeat
   return tString
end MessageDigest_ToString

private command MessageDigest_Check pType, pMessage, pExpected
   local tDigest
   put MessageDigest_ToString(messageDigest(pMessage, pType)) into tDigest
   if tDigest is not pExpected then
      TestDiagnostic merge("Expected: [[pExpected]]")
      TestDiagnostic merge("Computed: [[tDigest]]")
   end if
   if pMessage is empty then
      put "empty" into pMessage
   end if
   TestAssert merge("[[pType]] [[pMessage]]"), tDigest is pExpected
end MessageDigest_Check

on TestFiltersMessageDigest
   MessageDigest_Check "md5", "", "d41d8cd98f00b204e9800998ecf8427e"

   -- Test vectors from http://www.di-mgt.com.au/sha_testvectors.html

   MessageDigest_Check "sha-1", "", "da39a3ee5e6b4b0d3255bfef95601890afd80709"
   MessageDigest_Check "SHA-1", "abc", "a9993e364706816aba3e25717850c26c9cd0d89d"

   MessageDigest_Check "sha-224", "", "d14a028c2a3a2bc9476102bb288234c415a2b01f828ea62ac5b3e42f"
   MessageDigest_Check "sha-224", "abc", "23097d223405d8228642a477bda255b32aadbce4bda0b3f7e36c9da7"
   MessageDigest_Check "sha-256", "", "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"
   MessageDigest_Check "sha-256", "abc", "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad"
   MessageDigest_Check "sha-384", "", "38b060a751ac96384cd9327eb1b1e36a21fdb71114be07434c0cc7bf63f6e1da274edebfe76f65fbd51ad2f14898b95b"
   MessageDigest_Check "sha-384", "abc", "cb00753f45a35e8bb5a03d699ac65007272c32ab0eded1631a8b605a43ff5bed8086072ba1e7cc2358baeca134c825a7"
   MessageDigest_Check "sha-512", "", "cf83e1357eefb8bdf1542850d66d8007d620e4050b5715dc83f4a921d36ce9ce47d0d13c5d85f2b0ff8318d2877eec2f63b931bd47417a81a538327af927da3e"
   MessageDigest_Check "sha-512", "abc", "ddaf35a193617abacc417349ae20413112e6fa4e89a97ea20a9eeee64b55d39a2192992a274fc1a836ba3c23a3feebbd454d4423643ce80e2a9ac94fa54ca49f"

   MessageDigest_Check "sha3-224", "", "6b4e03423667dbb73b6e15454f0eb1abd4597f9a1b078e3f5b5a6bc7"
   MessageDigest_Check "sha3-224", "abc", "e642824c3f8cf24ad09234ee7d3c766fc9a3a5168d0c94ad73b46fdf"
   MessageDigest_Check "sha3-256", "", "a7ffc6f8bf1ed76651c14756a061d662f580ff4de43b49fa82d80a4b80f8434a"
   MessageDigest_Check "sha3-256", "abc", "3a985da74fe225b2045c172d6bd390bd855f086e3e9d525b46bfe24511431532"
   MessageDigest_Check "sha3-384", "", "0c63a75b845e4f7d01107d852e4c2485c51a50aaaa94fc61995e71bbee983a2ac3713831264adb47fb6bd1e058d5f004"
   MessageDigest_Check "sha3-384", "abc", "ec01498288516fc926459f58e2c6ad8df9b473cb0fc08c2596da7cf0e49be4b298d88cea927ac7f539f1edf228376d25"
   MessageDigest_Check "sha3-512", "", "a69f73cca23a9ac5c8b567dc185a756e97c982164fe25859e0d1dcc1475c80a615b2123af1f5f94c11e3e9402c3ac558f500199d95b6d3e301758586281dcd26"
   MessageDigest_Check "sha3-512", "abc", "b751850b1a57168a5693cd924b6b096e08f621827444f70d884f5d0240d2712e10e116e9192af3c91a7ec57647e3934057340b4cf408d5a56592f8274eec53f0"

end TestFiltersMessageDigest

on TestFiltersUrldecode

TestAssert "test", urldecode("+") is " "
TestAssert "test", urldecode("%3F") is "?"
TestAssert "test", urldecode("abcde") is "abcde"

end TestFiltersUrldecode
on TestFiltersUrlencode

TestAssert "test", urlencode(" ") is "+"
TestAssert "test", urlencode("?") is "%3F"
TestAssert "test", urlencode("abcde") is "abcde"

end TestFiltersUrlencode
