script "CoreStringsBoundaryChunk"
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

on TestForwardOrdinals
  TestAssert "char first", the first char of "abcdefghij" is "a"
  TestAssert "char second", the second char of "abcdefghij" is "b"
  TestAssert "char third", the third char of "abcdefghij" is "c"
  TestAssert "char fourth", the fourth char of "abcdefghij" is "d"
  TestAssert "char fifth", the fifth char of "abcdefghij" is "e"
  TestAssert "char sixth", the sixth char of "abcdefghij" is "f"
  TestAssert "char seventh", the seventh char of "abcdefghij" is "g"
  TestAssert "char eighth", the eighth char of "abcdefghij" is "h"
  TestAssert "char ninth", the ninth char of "abcdefghij" is "i"
  TestAssert "char tenth", the tenth char of "abcdefghij" is "j"
end TestForwardOrdinals
