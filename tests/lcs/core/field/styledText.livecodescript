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

private function RandomColor
   local tColor
   repeat 3	
      put random(256) - 1 & comma after tColor
   end repeat
   delete the last char of tColor
   return tColor
end RandomColor

on TestStyledTextColorsRoundTrip
   repeat for each item tItem in "paragraph border,paragraph background,character foreground,character background"
      _TestStyledTextColorRoundTrip tItem
   end repeat
end TestStyledTextColorsRoundTrip

private command _TestStyledTextColorRoundTrip pWhich
   local tField, tColor, tExportColor, tImportColor, tStyled
   create field "styled"
   put it into tField
   put RandomColor() into tColor
   put "something" into field "styled"
   switch pWhich
      case "paragraph border"
         set the borderColor of line 1 of field "styled" to tColor
         break
      case "paragraph background"
         set the backColor of line 1 of field "styled" to tColor
         break
      case "character foreground"
         set the foreColor of char 1 of field "styled" to tColor
         break
      case "character background"
         set the backColor of char 1 of field "styled" to tColor
         break
   end switch
   put the styledText of field "styled" into tStyled
   put empty into field "styled"
   put "something" into field "styled"
   set the styledText of field "styled" to tStyled
   switch pWhich
      case "paragraph border"
         put tStyled[1]["style"]["borderColor"] into tExportColor
         put the borderColor of line 1 of field "styled" into tImportColor
         break
      case "paragraph background"
         put tStyled[1]["style"]["backgroundColor"] into tExportColor
         put the backColor of line 1 of field "styled" into tImportColor
         break
      case "character foreground"
         put tStyled[1]["runs"][1]["style"]["textColor"] into tExportColor
         put the foreColor of char 1 of field "styled" into tImportColor
         break
      case "character background"
         put tStyled[1]["runs"][1]["style"]["backgroundColor"] into tExportColor
         put the backColor of char 1 of field "styled" into tImportColor
         break
   end switch
   delete field "styled"
   
   TestAssert "styledText export" && pWhich && "color correctly", \
         tColor is tExportColor
   
   TestAssert "styledText" && pWhich && "colour round-trips correctly", \
         tImportColor is tColor
end _TestStyledTextColorRoundTrip
