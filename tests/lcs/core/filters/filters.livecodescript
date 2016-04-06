script "CoreFilters"
/*
Copyright (C) 2015 LiveCode Ltd.

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