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

private function q pString
   replace "'" with quote in pString
   return pString
end q

on TestParagraphTabStopsRoundtrip
   create stack "Test"
   set the defaultStack to "Test"

   create field "TestField"
   
   // set up styledText array with text and tabAlign
   local tStyledText
   put "left" & tab & "center" & tab & "right" into tStyledText[1]["runs"][1]["text"]
   put "100,200,300" into tStyledText[1]["style"]["tabStops"]
   
   set the styledText of field "TestField" to tStyledText
   TestAssert "styledText supports tabStops", the tabStops of line 1 of field "TestField" is "100,200,300"
   
   put the styledText of field "testField" into tStyledText
   TestAssert "styledText roundtrips tabStops", tStyledText[1]["style"]["tabStops"] is "100,200,300"

   // clear field styles
   set the text of field "testField" to "left" & tab & "center" & tab & "right"
   set the tabStops of field "testField" to "10,20,30"

   put the styledText of field "testField" into tStyledText
   TestAssert "styledText doesn't include inherited tabStops", "tabStops" is not among the keys of tStyledText[1]["style"]
   
   put the effective styledText of field "testField" into tStyledText
   TestAssert "effective styledText does include inherited tabStops", tStyledText[1]["style"]["tabStops"] is "10,20,30"
   
   delete stack "Test"
end TestParagraphTabStopsRoundtrip

on TestParagraphTabAlignRoundtrip
   create stack "Test"
   set the defaultStack to "Test"

   create field "TestField"
   
   // set up styledText array with text and tabAlign
   local tStyledText
   put "left" & tab & "center" & tab & "right" into tStyledText[1]["runs"][1]["text"]
   put "left,center,right" into tStyledText[1]["style"]["tabAlign"]
   

   set the styledText of field "TestField" to tStyledText
   TestAssert "styledText supports tabAlign", the tabAlign of line 1 of field "TestField" is "left,center,right"
   
   put the styledText of field "testField" into tStyledText
   TestAssert "styledText roundtrips tabAlign", tStyledText[1]["style"]["tabAlign"] is "left,center,right"

   // clear field styles
   set the text of field "testField" to "left" & tab & "center" & tab & "right"
   set the tabAlign of field "testField" to "right,center,left"

   put the styledText of field "testField" into tStyledText
   TestAssert "styledText doesn't include inherited tabAlign", "tabAlign" is not among the keys of tStyledText[1]["style"]
   
   put the effective styledText of field "testField" into tStyledText
   TestAssert "effective styledText does include inherited tabAlign", tStyledText[1]["style"]["tabAlign"] is "right,center,left"
   
   delete stack "Test"
end TestParagraphTabAlignRoundtrip

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

on TestParagraphHidden
   create stack "Test"
   set the defaultStack to "Test"
   
   create field "TestField"
   
   // set up styledText array with text and tabAlign
   local tStyledText
   put "test" into tStyledText[1]["runs"][1]["text"]
   put true into tStyledText[1]["style"]["hidden"]
   
   
   set the styledText of field "TestField" to tStyledText
   TestAssert "styledText supports hidden", the hidden of line 1 of field "TestField"
   
   put the styledText of field "testField" into tStyledText
   TestAssert "styledText roundtrips hidden", tStyledText[1]["style"]["hidden"]
   
   // clear field styles
   set the text of field "testField" to "test"
   set the hidden of field "testField" to true
   
   put the styledText of field "testField" into tStyledText
   TestAssert "styledText doesn't include inherited hidden", "hidden" is not among the keys of tStyledText[1]["style"]
   
   put the effective styledText of field "testField" into tStyledText
   TestAssert "effective styledText includes hidden key but not inherited value", "hidden" is among the keys of tStyledText[1]["style"] and not tStyledText[1]["style"]
   
   delete stack "Test"
end TestParagraphHidden

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
