script "CoreFieldHtmlText"
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

private function q pString
   replace "'" with quote in pString
   return pString
end q

on TestSuperscriptInField
   create stack "Test"
   set the defaultStack to "Test"

   create field "TestField"
   
   // The text contains a superscript e.g. The room is 10 m^2
   set the htmlText of field "TestField" to format("<p>The room is 10 m<sup shift=\"-4\">2</sup></p>")
   TestAssert "Setting the htmlText respects superscripts", the htmlText of field "TestField" is format("<p>The room is 10 m<sup shift=\"-4\">2</sup></p>")
   
   delete stack "Test"
end TestSuperscriptInField

on TestSurrogateRoundtrip
   create stack "Test"
   set the defaultStack to "Test"

   create field "TestField"

   set the htmlText of field "TestField" to "<p>&#78730;</p>"
   TestAssert "htmlText roundtrips surrogates", the htmlText of field "TestField" is "<p>&#78730;</p>"

   set the htmlText of field "TestField" to "<p metadata='&#78730;'></p>"
   TestAssert "htmlText roundtrips surrogates in paragraph metadata", the htmlText of field "TestField" is q("<p metadata='&#78730;'></p>")

   set the htmlText of field "TestField" to "<p><span metadata='&#78730;'>a</span></p>"
   TestAssert "htmlText roundtrips surrogates in span metadata", the htmlText of field "TestField" is q("<p><span metadata='&#78730;'>a</span></p>")

   set the htmlText of field "TestField" to "<p><img src='1000' char='&#78730;'></p>"
   TestAssert "htmlText roundtrips surrogates in imageSource char", the htmlText of field "TestField" is q("<p><img src='1000' char='&#78730;'></p>")

   delete stack "Test"
end TestSurrogateRoundtrip

on TestListStyleRoundtrip
   create stack "Test"
   set the defaultStack to "Test"
   
   create field "TestField"
   
   set the text of field "TestField" to "one" & CR & "two" & CR & "three"
   set the listStyle of line 1 of field "TestField" to "skip"
   set the listStyle of line 2 of field "TestField" to "disc"
   TestAssert "htmlText roundtrips listStyle beginning with skip", the htmlText of field "TestField" is the htmlText of field  "TestField"
   
   set the text of field "TestField" to "one" & CR & "two" & CR & "three"
   set the listStyle of line 1 of field "TestField" to "disc"
   set the listStyle of line 2 of field "TestField" to "skip"
   TestAssert "htmlText roundtrips listStyle containing  skip", the htmlText of field "TestField" is the htmlText of field  "TestField"
   
   set the text of field "TestField" to "one" & CR & "two" & CR & "three"
   set the listStyle of line 1 of field "TestField" to "disc"
   set the listStyle of line 3 of field "TestField" to "skip"
   TestAssert "htmlText roundtrips listStyle ending with  skip", the htmlText of field "TestField" is the htmlText of field  "TestField"
   
   delete field "TestField"
   delete stack "Test"
end TestListStyleRoundtrip

// Regression test for bug 20633 - note the bug here was that a NUL block is
// created which causes the 'char' reference to be wrong.
on TestAnnotationsAndVTab
   create stack "Test"
   set the defaultStack to "Test"

   create field "TestField"

   set the htmlText of field "TestField" to format("<p><a href=\"#Anchor1\">Anchor1&#11;</a><a href=\"#Anchor2\">Anchor2&#11;</a></p>")
   TestAssert "linkText on VTab does not elide with next linkText", the linkText of char 9 of field "TestField" is "#Anchor2"

   set the htmlText of field "TestField" to format("<p><span metadata=\"#Anchor1\">Anchor1&#11;</span><span metadata=\"#Anchor2\">Anchor2&#11;</span></p>")
   TestAssert "metadata on VTab does not elide with next metadata", the metadata of char 9 of field "TestField" is "#Anchor2"
end TestAnnotationsAndVTab
