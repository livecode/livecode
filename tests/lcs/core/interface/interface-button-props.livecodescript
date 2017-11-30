script "CoreButtonProps"
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

on TestUnhighlight
	create group
	create button in group 1
	create button in group 1

	unhilite button 1
	TestAssert "no hilite on button 1", not the hilite of button 1
	hilite button 2
	TestAssert "still dehilited", not the hilite of button 1

	dehilite button 2
	TestAssert "no hilite on button 2", not the hilite of button 2
	hilite button 1
	TestAssert "still dehilited", not the hilite of button 2
end TestUnhighlight

on TestHighlightedButtonID
	create group
	create button in group 1
	create button in group 1

	TestAssert "empty hilite on button creation", the hilitedbuttonid of group 1 is 0
	set the hilitedbuttonid of group 1 to the short id of button 2
	TestAssert "button 1 of group 1 is unhilited", not the hilite of button 1
	hilite button 1
	TestAssert "hilitedbuttonid is button 1 of group 1", the hilitedbuttonid of group 1 is the short id of button 1
	TestAssert "hilitedbuttonname is button 1 of group 1", the hilitedbuttonname of group 1 is the short name of button 1

	unhilite button 1
	TestAssert "button 1 of group 1 is unhilited", not the hilite of button 1
	hilite button 2
	TestAssert "hilitedbuttonid is button 2 of group 1", the hilitedbuttonid of group 1 is the short id of button 2
	TestAssert "hilitedbuttonname is button 2 of group 1", the hilitedbuttonname of group 1 is the short name of button 2

	dehilite button 2
	TestAssert "button 2 of group 1 is unhilited", not the hilite of button 2
	set the hilitedbuttonid of group 1 to the short id of button 1
	TestAssert "hilitedbuttonid is button 1 of group 1", the hilitedbuttonid of group 1 is the short id of button 1
	TestAssert "hilitedbuttonname is button 1 of group 1", the hilitedbuttonname of group 1 is the short name of button 1
end TestHighlightedButtonID

on TestHighlightBorder
	create button
	TestAssert "hiliteborder is true on button creation", the hiliteborder of button 1 is true

	set the hiliteborder of button 1 to false
	TestAssert "change hiliteborder from true to false", the hiliteborder of button 1 is false

	set the hiliteborder of button 1 to true
	TestAssert "change hiliteborder from false to true", the hiliteborder of button 1
	
	set the hiliteborder of button 1 to false
	TestAssert "change hiliteborder from true to false", the hiliteborder of button 1 is false
end TestHighlightBorder

on TestHighlightFill
	create button

	-- Consistent with LiveCode 6.7
	TestAssert "hilitefill is true on button creation", the hilitefill of button 1

	set the hilitefill of button 1 to false
	TestAssert "button hilitefill set to false", the hilitefill of button 1 is false

	set the hilitefill of button 1 to true
	TestAssert "button hilitefill set to true", the hilitefill of button 1
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
	TestAssert "sharedhilite is true on button creation", the sharedhilite of button 1 is true

	set the sharedhilite of button 1 to false
	TestAssert "sharedhilite set to false", the sharedhilite of button 1 is false

	set the sharedhilite of button 1 to true
	TestAssert "sharedhilite set to true", the sharedhilite of button 1
end TestSharedHighlight

on TestShowHighlight
	create button
	TestAssert "showhilite is false on button creation", the showhilite of button 1 is false

	set the showhilite of button 1 to true
	TestAssert "showhilite set to true", the showhilite of button 1

	set the showhilite of button 1 to false
	TestAssert "showhilite set to false", the showhilite of button 1 is false
end TestShowHighlight

on TestHighlightColorDefault
	create button
	TestAssert "default button hilitecolor", the hilitecolor of button 1 is empty
end TestHighlightColorDefault

on TestHighlightColors
	create button

	set the hilitecolor of button 1 to "red"
	TestAssert "hilitecolor set to red", the hilitecolor of button 1 is "red"

	set the hilitecolor of button 1 to "255,255,0"
	TestAssert "hilitecolor set to RGB value", the hilitecolor of button 1 is "255,255,0"

	set the hilitecolor of button 1 to "#336633"
	TestAssert "hilitecolor set to hex value", the hilitecolor of button 1 is "#336633"

#	set the hilitecolor of button 1 to 1234
#	TestAssert "hilitecolor set to 1234", the hilitecolor of button 1 is "1234"
	delete button 1
end TestHighlightColors

on TestHighlighted
	create button
	TestAssert "hilited false on button creation", the hilited of button 1 is false

	set the hilited of button 1 to true
	TestAssert "hilited set to true", the hilited of button 1

	set the hilited of button 1 to false
	TestAssert "hilited set to false", the hilited of button 1 is false
end TestHighlighted

on TestButtonAutoHighlightDefault
	create button
	TestAssert "button autohilite is true on button creation", the autohilite of button 1 is true
end TestButtonAutoHighlightDefault

on TestButtonAutoHighlight
	create button

	set the autohilite of button 1 to true
	TestAssert "autohilite set to true from initial state", the autohilite of button 1

	set the autohilite of button 1 to false
	TestAssert "autohilite set to false from true", the autohilite of button 1 is false

	set the autohilite of button 1 to true
	TestAssert "autohilite set to true from false", the autohilite of button 1
end TestButtonAutoHighlight

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

