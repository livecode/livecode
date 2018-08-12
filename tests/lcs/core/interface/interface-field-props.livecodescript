script "CoreFieldProps"
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

on TestHilitedLinesAndMultipleHighlights
	create stack "Test"
	set the defaultstack to "Test"
	create field "tf"

	set the text of field "tf" to "abc" & return & "def" & return & "ghi"
	TestAssert "multiplehilites on field creation", the multiplehilites of field "tf" is false
	TestAssert "noncontiguoushilites on field creation", the noncontiguoushilites of field "tf" is false

	set the listbehavior of field "tf" to true
	set the multiplehilites of field "tf" to true
	set the noncontiguoushilites of field "tf" to true

	set the hilitedLines of field "tf" to empty
	TestAssert "empty hilitedlines", the selectedText of field "tf" is empty

	set the hilitedlines of field "tf" to 1,3
	TestAssert "noncontiguous multiple hilites", the selectedText of field "tf" is ("abc" & return & "ghi")

	# no noncontiguous hilites
	set the noncontiguoushilites of field "tf" to false
	TestAssert "multiplehilites on field creation", the multiplehilites of field "tf" is true
	TestAssert "noncontiguoushilites on field creation", the noncontiguoushilites of field "tf" is false

	# no multiple hilites
	set the noncontiguoushilites of field "tf" to true
	set the multiplehilites of field "tf" to false
	TestAssert "multiplehilites on field creation", the multiplehilites of field "tf" is false
	TestAssert "noncontiguoushilites on field creation", the noncontiguoushilites of field "tf" is true

	set the hilitedLines of field "tf" to empty
	TestAssert "empty hilitedlines", the selectedText of field "tf" is empty
end TestHilitedLinesAndMultipleHighlights

on TestHighlightColors
	create field
	TestAssert "default field hilitecolor", the hilitecolor of field 1 is empty

	set the hilitecolor of field 1 to "red"
	TestAssert "hilitecolor set to red", the hilitecolor of field 1 is "red"

	set the hilitecolor of field 1 to "255,255,0"
	TestAssert "hilitecolor set to RGB value", the hilitecolor of field 1 is "255,255,0"

	set the hilitecolor of field 1 to "#336633"
	TestAssert "hilitecolor set to hex value", the hilitecolor of field 1 is "#336633"

#	set the hilitecolor of field 1 to 1234
#	TestAssert "hilitecolor set to 1234", the hilitecolor of field 1 is "1234"
end TestHighlightColors

on TestFieldAutoHighlightDefault
	create field
	TestAssert "autohilite is false on field creation", the autohilite of field 1 is true
end TestFieldAutoHighlightDefault

on TestFieldAutoHighlight
	create field

	set the autohilite of field 1 to true
	TestAssert "autohilite set to true from initial state", the autohilite of field 1

	set the autohilite of field 1 to false
	TestAssert "autohilite set to false from true", the autohilite of field 1 is false

	set the autohilite of field 1 to true
	TestAssert "autohilite set to true from false", the autohilite of field 1
end TestFieldAutoHighlight

on TestToggleHighlightDefault
	create field
	TestAssert "togglehilites is false on field creation", the togglehilites of field 1 is false
end TestToggleHighlightDefault

on TestToggleHighlights
	create field

	set the togglehilites of field 1 to true
	TestAssert "togglehilites set to true from initial state", the togglehilites of field 1

	set the togglehilites of field 1 to false
	TestAssert "togglehilites set to false from true", the togglehilites of field 1 is false

	set the togglehilites of field 1 to true
	TestAssert "togglehilites set to true from false", the togglehilites of field 1
end TestToggleHighlights

on TestThreedHighlightDefault
	create field
	TestAssert "threedhilite is false on field creation", the threedhilite of field 1 is false
end TestThreedHighlightDefault

on TestThreedHighlight
	create field

	set the listbehavior of field 1 to true

	set the threedhilite of field 1 to true
	TestAssert "threedhilite set to true from initial state", the threedhilite of field 1

	set the threedhilite of field 1 to false
	TestAssert "threedhilite set to false from true", the threedhilite of field 1 is false

	set the threedhilite of field 1 to true
	TestAssert "threedhilite set to true from false", the threedhilite of field 1
end TestThreedHighlight

