script "CoreFieldReplaceInField"
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

on TestReplaceInField
   open this stack
   create field "Text"
   
   set the htmlText of field "Text" to "<p><b>foo</b> <i>foobar</i> <i><b>foobar</b></i> <u>foo</u></p>"
   replace "foo" with "baz" in field "Text" preserving styles
   TestAssert "replace in whole field preserving styles", the htmlText of field "Text" is "<p><b>baz</b> <i>bazbar</i> <i><b>bazbar</b></i> <u>baz</u></p>"
   
   set the htmlText of field "Text" to "<p><b>foo</b> <i>foobar</i> <i><b>foobar</b></i> <u>foo</u></p>"
   replace "foo" with "baz" in field "Text" replacing styles
   TestAssert "replace in whole field preserving styles", the htmlText of field "Text" is "<p>baz baz<i>bar</i> baz<i><b>bar</b></i> baz</p>"
   
   set the htmlText of field "Text" to "<p><b>foo</b> <i>foobar</i> <i><b>foobar</b></i> <u>foo</u></p>"
   replace "foo" with "baz" in word 2 to 3 of field "Text" preserving styles
   TestAssert "replace in range of field preserving styles", the htmlText of field "Text" is "<p><b>foo</b> <i>bazbar</i> <i><b>bazbar</b></i> <u>foo</u></p>"
   
   set the htmlText of field "Text" to "<p><b>foo</b> <i>foobar</i> <i><b>foobar</b></i> <u>foo</u></p>"
   replace "foo" with "baz" in word 2 to 3 of field "Text" replacing styles
   TestAssert "replace in range of field preserving styles", the htmlText of field "Text" is "<p><b>foo</b> baz<i>bar</i> baz<i><b>bar</b></i> <u>foo</u></p>"
   
   delete field "Text"
end TestReplaceInField
