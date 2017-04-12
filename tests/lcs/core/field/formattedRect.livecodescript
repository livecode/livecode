script "CoreFieldFormattedRect"
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

on TestFormattedRectInLargeField
   create stack "Test"
   set the defaultStack to "Test"

   create field "TestField"
   set the rect of field "TestField" to 0,0,400,400
   set the fixedLineHeight of field "TestField" to true
   set the textHeight of field "TestField" to 10

   local tText
   repeat 10000 times
      put "foo" & return after tText
   end repeat
   set the text of field "TestField" to tText

   TestAssert "formattedRect of last line is correct", item 2 of the formattedRect of line 9999 of field "TestField" is 99984 and \
   														item 4 of the formattedRect of line 9999 of field "TestField" is 99994
end TestFormattedRectInLargeField
