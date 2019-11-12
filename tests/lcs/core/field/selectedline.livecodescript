script "CoreFieldSelectedLine"
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

on TestSetup
   create stack "Test"
   set the defaultStack to "Test"
   create field "Test"
end TestSetup

private command __ResetField
   put format("a\nb\nc\nd\ne\nf\ng\nh\n") into field "Test"
end __ResetField

on TestCutSelectedLine
   set the defaultStack to "Test"
   __ResetField
   cut line 4 of field "Test"
   TestAssert "cut line removes line", the text of field "Test" is format("a\nb\nc\ne\nf\ng\nh\n")
   TestDiagnostic the text of field "Test"
   TestAssert "cut line includes line delimiter", the clipboardData["text"] is "d" & return
   TestDiagnostic the clipboardData["text"]
   
   __ResetField
   select before line 4 of field "Test"
   cut the selectedline
   TestAssert "cut selectedLine removes line", the text of field "Test" is format("a\nb\nc\ne\nf\ng\nh\n")
   TestDiagnostic the text of field "Test"
   TestAssert "cut selectedLine includes line delimiter", the clipboardData["text"] is "d" & return
   TestDiagnostic the clipboardData["text"]
   
   __ResetField
   select line 4 of field "Test"
   cut
   TestAssert "select line then cut removes line", the text of field "Test" is format("a\nb\nc\ne\nf\ng\nh\n")
   TestDiagnostic the text of field "Test"
   TestAssert "cut line includes line delimiter", the clipboardData["text"] is "d" & return
   TestDiagnostic the clipboardData["text"]
   
   __ResetField
   select before line 4 of field "Test"
   select the selectedline
   cut
   TestAssert "select selectedLine then cut removes line", the text of field "Test" is format("a\nb\nc\ne\nf\ng\nh\n")
   TestDiagnostic the text of field "Test"
   TestAssert "cut selectedLine includes line delimiter", the clipboardData["text"] is "d" & return
   TestDiagnostic the clipboardData["text"]
end TestCutSelectedLine

on TestCopySelectedLine
   set the defaultStack to "Test"
   __ResetField
   copy line 4 of field "Test"
   TestAssert "copy line includes line delimiter", the clipboardData["text"] is "d" & return
   TestDiagnostic the clipboardData["text"]
   
   __ResetField
   select before line 4 of field "Test"
   copy the selectedLine
   TestAssert "copy selectedLine includes line delimiter", the clipboardData["text"] is "d" & return
   TestDiagnostic the clipboardData["text"]
   
   __ResetField
   select line 4 of field "Test"
   copy
   TestAssert "select line and copy includes line delimiter", the clipboardData["text"] is "d" & return
   TestDiagnostic the clipboardData["text"]
   
   __ResetField
   select before line 4 of field "Test"
   select the selectedline
   copy
   TestAssert "select selectedline and copy includes line delimiter", the clipboardData["text"] is "d" & return
   TestDiagnostic the clipboardData["text"]
end TestCopySelectedLine

on TestPasteSelectedLine
   set the clipboardData["text"] to "i"
   set the defaultStack to "Test"
   __ResetField
   select line 4 of field "Test"
   paste
   TestAssert "select line and paste includes line delimiter", the text of field "Test" is format("a\nb\nc\nie\nf\ng\nh\n")
   TestDiagnostic the text of field "Test"

   __ResetField
   select before line 4 of field "Test"
   select the selectedline
   paste
   TestAssert "select selectedline and paste includes line delimiter", the text of field "Test" is format("a\nb\nc\nie\nf\ng\nh\n")
   TestDiagnostic the text of field "Test"
end TestPasteSelectedLine

on TestDeleteSelectedLine
   set the defaultStack to "Test"
   __ResetField
   delete line 4 of field "Test"
   TestAssert "delete line removes line", the text of field "Test" is format("a\nb\nc\ne\nf\ng\nh\n")
   TestDiagnostic the text of field "Test"
   
   __ResetField
   select before line 4 of field "Test"
   delete the selectedLine
   TestAssert "select the selectedLine then delete removes line", the text of field "Test" is format("a\nb\nc\ne\nf\ng\nh\n")
end TestDeleteSelectedLine

on TestHiddenPropertyOfSelectedLine
   set the defaultStack to "Test"
   __ResetField
   set the hidden of line 4 of field "Test" to true
   TestAssert "hidden property of line is correct", the hidden of line 4 of field "Test" is true
   TestAssert "hidden property of line is correct after", the hidden of line 5 of field "Test" is false
   TestAssert "hidden property of line is correct before", the hidden of line 3 of field "Test" is false
   
   __ResetField
   select before line 4 of field "Test"
   set the hidden of the selectedLine to true
   TestAssert "hidden property of selectedLine is correct", the hidden of line 4 of field "Test" is true
   TestAssert "hidden property of selectedLine is correct after", the hidden of line 5 of field "Test" is false
   TestAssert "hidden property of selectedLine is correct before", the hidden of line 3 of field "Test" is false
end TestHiddenPropertyOfSelectedLine

on TestListStylePropertyOfSelectedLine
   set the defaultStack to "Test"
   __ResetField
   set the listStyle of line 4 of field "Test" to "disc"
   TestAssert "listStyle property of line is correct", the listStyle of line 4 of field "Test" is "disc"
   TestAssert "listStyle property of line is correct after", the listStyle of line 5 of field "Test" is empty
   TestAssert "listStyle property of line is correct before", the listStyle of line 3 of field "Test" is empty
   
   __ResetField
   select before line 4 of field "Test"
   set the listStyle of the selectedLine to "disc"
   TestAssert "listStyle property of selectedLine is correct", the listStyle of line 4 of field "Test" is "disc"
   TestAssert "listStyle property of selectedLine is correct after", the listStyle of line 5 of field "Test" is empty
   TestAssert "listStyle property of selectedLine is correct before", the listStyle of line 3 of field "Test" is empty
end TestListStylePropertyOfSelectedLine

on TestPutIntoSelectedLine
   set the defaultStack to "Test"
   __ResetField
   put "i" into line 4 of field "Test"
   TestAssert "put into line replaces text of line", the text of field "Test" is format("a\nb\nc\ni\ne\nf\ng\nh\n")
   TestDiagnostic the text of field "Test"
   
   __ResetField
   select before line 4 of field "Test"
   put "i" into the selectedLine
   TestAssert "put into selectedLine replaces text of line", the text of field "Test" is format("a\nb\nc\ni\ne\nf\ng\nh\n")
   TestDiagnostic the text of field "Test"
end TestPutIntoSelectedLine

on TestPutBeforeSelectedLine
   set the defaultStack to "Test"
   __ResetField
   put "i" before line 4 of field "Test"
   TestAssert "put before line inserts text before line", the text of field "Test" is format("a\nb\nc\nid\ne\nf\ng\nh\n")
   TestDiagnostic the text of field "Test"
   
   __ResetField
   select before line 4 of field "Test"
   put "i" before the selectedLine
   TestAssert "put before selectedLine inserts text before line", the text of field "Test" is format("a\nb\nc\nid\ne\nf\ng\nh\n")
   TestDiagnostic the text of field "Test"
end TestPutBeforeSelectedLine

on TestPutAfterSelectedLine
   set the defaultStack to "Test"
   __ResetField
   put "i" after line 4 of field "Test"
   TestAssert "put after line inserts text after line", the text of field "Test" is format("a\nb\nc\ndi\ne\nf\ng\nh\n")
   TestDiagnostic the text of field "Test"
   
   __ResetField
   select before line 4 of field "Test"
   put "i" after the selectedLine
   TestAssert "put after selectedLine inserts text after line", the text of field "Test" is format("a\nb\nc\ndi\ne\nf\ng\nh\n")
   TestDiagnostic the text of field "Test"
end TestPutAfterSelectedLine

on TestSelectedLineRoundTrip
   set the defaultStack to "Test"
   __ResetField
   select before line 4 of field "Test"
   TestAssert "selectedLine correct after select before line", the selectedLine is "line 4 of field 1"
   TestDiagnostic the selectedLine
   
   select the selectedLine
   TestAssert "selectedLine round trips with select the selectedLine", the selectedLine is "line 4 of field 1"
   TestDiagnostic the selectedLine
end TestSelectedLineRoundTrip
