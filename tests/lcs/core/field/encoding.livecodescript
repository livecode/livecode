script "CoreFieldEncoding"
/*
Copyright (C) 2018 LiveCode Ltd.

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

on TestEncodingOfCharChunk
   create stack "Test"
   set the defaultStack to "Test"

   create field "TestField"
   put "a" & numToCodepoint(0x1d11e) into field "TestField"

   TestAssert "encoding of native field chunk is 'native'", \
      the encoding of char 1 to 1 of field "TestField" is "native"

   TestAssert "encoding of non-native field chunk is 'unicode'", \
      the encoding of char 2 to 2 of field "TestField" is "unicode"

   TestAssert "encoding of mixed field chunk is 'unicode'", \
      the encoding of char 1 to 2 of field "TestField" is "unicode"
      
end TestEncodingOfCharChunk
