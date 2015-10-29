script "CoreHighlight"
/*
Copyright (C) 2015 Runtime Revolution Ltd.

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
	set the listbehavior of field "tf" to true
	set the multiplehilites of field "tf" to true
	set the noncontiguoushilites of field "tf" to true

	set the hilitedLines of field "tf" to empty
	TestAssert "empty hilitedlines", the selectedText of field "tf" is empty

	set the hilitedlines of field "tf" to 1,3
	TestAssert "noncontiguous multiple hilites", the selectedText of field "tf" is ("abc" & return & "ghi")

	# no noncontiguous hilites
	# this should fail
	set the noncontiguoushilites of field "tf" to false
	set the hilitedlines of field "tf" to 1,3
	TestAssert "no contiguous hilites", the selectedText of field "tf" is not ("abc" & return & "ghi")

	# no multiple hilites
	# this should also fail
	set the noncontiguoushilites of field "tf" to true
	set the multiplehilites of field "tf" to false
	set the hilitedlines of field "tf" to 1,3
	TestAssert "no multiple hilites", the selectedText of field "tf" is not ("abc" & return & "ghi")

	set the hilitedLines of field "tf" to empty
	TestAssert "empty hilitedlines", the selectedText of field "tf" is empty

	set the multiplehilites of field "tf" to false
	set the noncontiguoushilites of field "tf" to false
	set the hilitedline of field "tf" to 3
	TestAssert "just one hilited line", the selectedText of field "tf" is "ghi"
end TestHilitedLinesAndMultipleHighlights

on TestUnhighlight
	create group
	create button in group 1
	create button in group 1

	unhilite button 1
	TestAssert "no hilite on button 1", not the hilite of button 1
	hilite button 1
	TestAssert "still dehilited", the hilite of button 1

	unhilite button 1
	TestAssert "no hilite on button 1", not the hilite of button 1
	highlight button 2
	TestAssert "test", the hilite of button 2

	dehilite button 2
	TestAssert "no hilite on button 2", not the hilite of button 2
	hilite button 1
	TestAssert "still dehilited", the hilite of button 1

	dehilite button 1
	TestAssert "no hilite on button 1", not the hilite of button 1
	highlight button 1
	TestAssert "still dehilited", the hilite of button 1
end TestUnhighlight

on TestHighlightedButtonID
	create group
	create button in group 1
	create button in group 1

	TestAssert "empty hilite on button creation", the hilitedbuttonid of group 1 is 0
	set the highlitedbuttonid of group 1 to the short id of button 2
	TestAssertBroken "button 1 of group 1 is unhilited", not the hilite of button 1, "bug 16281"
	hilite button 1
	TestAssertBroken "hilitedbuttonid is button 1 of group 1", the hilitedbuttonid of group 1 is the short id of button 1, "bug 16281"
	TestAssertBroken "hilitedbuttonname is button 1 of group 1", the hilitedbuttonname of group 1 is the short name of button 1, "bug 16281"

	unhilite button 1
	TestAssertBroken "button 1 of group 1 is unhilited", not the hilite of button 1, "bug 16281"
	highlight button 2
	TestAssertBroken "hilitedbuttonid is button 2 of group 1", the hilitedbuttonid of group 1 is the short id of button 2, "bug 16281"
	TestAssertBroken "hilitedbuttonname is button 2 of group 1", the hilitedbuttonname of group 1 is the short name of button 2, "bug 16281"

	dehilite button 2
	TestAssertBroken "button 2 of group 1 is unhilited", not the hilite of button 2, "bug 16281"
	set the highlitedbuttonid of group 1 to the short id of button 1
	TestAssertBroken "hilitedbuttonid is button 1 of group 1", the hilitedbuttonid of group 1 is the short id of button 1, "bug 16281"
	TestAssertBroken "hilitedbuttonname is button 1 of group 1", the hilitedbuttonname of group 1 is the short name of button 1, "bug 16281"
end TestHighlightedButtonID

on TestHighlightBorder
	create button
	TestAssert "hiliteborder is false on button creation", the hiliteborder of button 1 is false

	set the hiliteborder of button 1 to true
	TestAssert "hiliteborder set to true", the hiliteborder of button 1

	set the hiliteborder of button 1 to false
	TestAssert "change hiliteborder from true to false", the hiliteborder of button 1 is false

	set the hiliteborder of button 1 to true
	TestAssert "change hiliteborder from false to true", the hiliteborder of button 1
end TestHighlightBorder

on TestHighlightFill
	create button
	TestAssert "hilitefill is false on button creation", the hilitefill of button 1 is false

	set the hilitefill of button 1 to true
	TestAssert "button hilitefill set to true", the hilitefill of button 1

	set the hilitefill of button 1 to false
	TestAssert "button hilitefill set to false", the hilitefill of button 1 is false
end TestHighlightFill

on TestHighlightedIcon
	create button
	TestAssert "hilitedIcon is zero after button creation", the hilitedIcon of button 1 is 0
	set the hilitedIcon of button 1 to 1234
	TestAssert "initial hilitedIcon assignment", the hilitedIcon of button 1 is 1234

	set the hilitedIcon of button 1 to 42
	TestAssert "change of hilitedIcon", the hilitedIcon of button 1 is 42
end TestHighlightedIcon

on TestSharedHighlight
	create button
	TestAssert "sharedhilite is false on button creation", the sharedhilite of button 1 is false

	set the sharedhilite of button 1 to true
	TestAssert "sharedhilite set to true", the sharedhilite of button 1
	
	set the sharedhilite of button 1 to false
	TestAssert "sharedhilite set to false", the sharedhilite of button 1 is false
end TestSharedHighlight

on TestShowHighlight
	create button
	TestAssert "showhilite is false on button creation", the showhilite of button 1 is false

	set the showhilite of button 1 to true
	TestAssert "showhilite set to true", the showhilite of button 1

	set the showhilite of button 1 to false
	TestAssert "showhilite set to false", the showhilite of button 1 is false
end TestShowHighlight

on TestHighlightColors
	create field
	# not sure what the default should be
	#TestAssert "default field hilitecolor", the hilitecolor of field 1 is empty

	set the hilitecolor of field 1 to "red"
	TestAssert "hilitecolor set to red", the hilitecolor of field 1 is "red"

	set the hilitecolor of field 1 to "255,255,0"
	TestAssert "hilitecolor set to RGB value", the hilitecolor of field 1 is "255,255,0"

	set the hilitecolor of field 1 to "#336633"
	TestAssert "hilitecolor set to hex value", the hilitecolor of field 1 is "#336633"

	set the hilitecolor of field 1 to 1234
	TestAssert "hilitecolor set to 1234", the hilitecolor of field 1 is "1234"
end TestHighlightColors

on TestLinkHighlightColors
	set the linkhilitecolor to "blue"
	TestAssertBroken "linkhilitecolor set to blue", the linkhilitecolor is "blue", "bug 16280"

	set the linkhilitecolor to "255,255,0"
	TestAssertBroken "linkhilitecolor set to RGB value", the linkhilitecolor is "255,255,0", "bug 16280"

	set the linkhilitecolor to "#336633"
	TestAssertBroken "linkhilitecolor set to hex value", the linkhilitecolor is "#336633", "bug 16280"
end TestLinkHighlightColors

on TestHighlighted
	create button
	TestAssert "hilited false on button creation", the hilited of button 1 is false

	set the hilited of button 1 to true
	TestAssert "hilited set to true", the hilited of button 1

	set the hilited of button 1 to false
	TestAssert "hilited set to false", the hilited of button 1 is false
end TestHighlighted

on TestFieldAutoHighlight
	create field
	TestAssert "field autohilite is false on field creation", the autohilite of field 1 is false

	set the autohilite of field 1 to true
	TestAssert "autohilite set to true from initial state", the autohilite of field 1

	set the autohilite of field 1 to false
	TestAssert "autohilite set to false from true", the autohilite of field 1 is false

	set the autohilite of field 1 to true
	TestAssert "autohilite set to true from false", the autohilite of field 1
end TestFieldAutoHighlight

on TestButtonAutoHighlight
	create button
	TestAssert "button autohilite is false on button creation", the autohilite of button 1 is false

	set the autohilite of button 1 to true
	TestAssert "autohilite set to true from initial state", the autohilite of button 1

	set the autohilite of button 1 to false
	TestAssert "autohilite set to false from true", the autohilite of button 1 is false

	set the autohilite of button 1 to true
	TestAssert "autohilite set to true from false", the autohilite of button 1
end TestButtonAutoHighlight

on TestToggleHighlights
	create field
	TestAssert "togglehilites is false on field creation", the togglehilites of field 1 is false

	set the togglehilites of field 1 to true
	TestAssert "togglehilites set to true from initial state", the togglehilites of field 1

	set the togglehilites of field 1 to false
	TestAssert "togglehilites set to false from true", the togglehilites of field 1 is false

	set the togglehilites of field 1 to true
	TestAssert "togglehilites set to true from false", the togglehilites of field 1
end TestToggleHighlights

on TestThreedHighlight
	create field
	TestAssert "threedhilite is false on field creation", the threedhilite of field 1 is false

	set the listbehavior of field 1 to true

	set the threedhilite of field 1 to true
	TestAssert "threedhilite set to true from initial state", the threedhilite of field 1

	set the threedhilite of field 1 to false
	TestAssert "threedhilite set to false from true", the threedhilite of field 1 is false

	set the threedhilite of field 1 to true
	TestAssert "threedhilite set to true from false", the threedhilite of field 1
end TestThreedHighlight

on TestHighlightedButton
	create group
	create button in group 1
	create button in group 1
	
	TestAssert "default hilitedButton", the hilitedButton of group 1 is 0
	set the hilitedButton of group 1 to 1
	TestAssert "set hilitedButton in group", the hilitedButton of group 1 is 1
	set the hilitedButton of group 1 to 2
	TestAssert "change hilitedButton in group", the hilitedButton of group 1 is 2
	set the hilitedButton of group 1 to 1
	TestAssert "reset hilitedButton in group", the hilitedButton of group 1 is 1
end TestHighlightedButton

