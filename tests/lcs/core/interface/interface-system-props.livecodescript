script "CoreSystemProps"
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

on TestLinkHighlightColors
	set the linkhilitecolor to "blue"
	TestAssertBroken "linkhilitecolor set to blue", the linkhilitecolor is "blue", "bug 16280"

	set the linkhilitecolor to "255,255,0"
	TestAssertBroken "linkhilitecolor set to RGB value", the linkhilitecolor is "255,255,0", "bug 16280"

	set the linkhilitecolor to "#336633"
	TestAssertBroken "linkhilitecolor set to hex value", the linkhilitecolor is "#336633", "bug 16280"
end TestLinkHighlightColors
