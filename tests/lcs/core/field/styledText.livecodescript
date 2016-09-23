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
   TestAssert "styledText doesn't include inherited tabAlign", tabAlign is not among the keys of tStyledText[1]["style"]
   
   put the effective styledText of field "testField" into tStyledText
   TestAssert "effective htmlText does include inherited tabAlign", tStyledText[1]["style"]["tabAlign"] is "right,center,left"

   delete stack "Test"
end TestParagraphTabAlignRoundtrip
