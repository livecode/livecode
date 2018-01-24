script "CoreFieldStyledText"
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

on TestStyledTextSplitRunRoundTrip
   local tFieldBefore, tFieldAfter
   create field
   put it into tFieldBefore
   create field
   put it into tFieldAfter
   
   local tBeforeText
   put "Line 1|Line 2" into tBeforeText
   set the text of tFieldBefore to tBeforeText
   
   local tStyledText
   put the styledText of tFieldBefore into tStyledText
   
   replace "|" with cr in tStyledText[1]["runs"][1]["text"]
   
   set the styledText of tFieldAfter to tStyledText
   replace "|" with cr in tBeforeText
   
   TestAssert "split styled text run doesn't lose data", \
      tBeforeText is the text of tFieldAfter
end TestStyledTextSplitRunRoundTrip

on TestStyledTextColorRoundTrip
   local tField, tColor
   create field
   put it into tField
   put "255,128,64" into tColor
   set the text of tField to "red"
   set the textColor of char 1 to -1 of tField to tColor
   
   set the styledText of tField to the styledText of tField
   
   TestAssert "styledText colour round-trips correctly", \
      the textColor of char 1 to -1 of tField is tColor
end TestStyledTextColorRoundTrip