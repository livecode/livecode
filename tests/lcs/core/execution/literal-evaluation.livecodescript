script "CoreExecutionLiteralEvaluation"
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
along with LiveCode.  If not see <http://www.gnu.org/licenses/>.  */

on TestEvaluateNumericLiteral
   set the numberFormat to "00"
   TestAssert "numeric literal remains string", (2 & empty) is "2"
   TestAssert "numeric literal becomes number in numeric context", ((2 + 0) & empty) is "02"

   local tArray
   put "2" into tArray["2"]
   TestAssert "numeric literal remains string in computation context", (tArray[2] + 1) is "03"
end TestEvaluateNumericLiteral

on TestEvaluateHexSigned
   -- Hexadecimal literals should be parsed as *signed* numbers (contrary to expectations!)
   TestAssert "hexadecimal literals are signed", (0xffffffff is -1)
   TestAssert "hexadecimal literals are signed in arithmetic contexts", ((0xffffffff + 0) is -1)
end TestEvaluateHexSigned

on TestEvaluateHexOverflowIgnored
   -- Legacy behaviour is to ignore overflow of hex literals
   TestAssert "hexadecimal overflow is ignored", (0x10000002A+0) is 42
end TestEvaluateHexOverflowIgnored

on TestParseOctals
   -- The parser currently fails to parse octal numeric literals correctly
   set the convertOctals to true
   TestAssertBroken "parser octal conversion of strings", ("010"+0) is 8
   TestAssertBroken "parser octal conversion of numbers", (010+0) is 8

   -- The legacy behaviour of the parser is to accept invalid octal literals!
   TestAssert "parser octal conversion legacy behaviour", "018" is 16
end TestParseOctals
