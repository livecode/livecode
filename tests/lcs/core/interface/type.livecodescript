script "CoreType"
/*
Copyright (C) 2017 LiveCode Ltd.

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

on TestType
	TestSkipIfNot "wait"
   
	create stack "Test"
	set the defaultStack to "Test"
	set the script of stack "Test" to \
		"on rawKeyDown pKeyCode; set the uKeyCode of me to pKeyCode; pass rawKeyDown; end rawKeyDown" & return & \
		"on keyDown pChar; set the uChar of me to pChar pass keyDown; end keyDown"
	TestAssert "stack script compiled okay", the result is empty

	type "a"
	TestAssert "type ascii char 'a' maps correctly", \
			the uKeyCode of stack "Test" is charToNum("a") and \
			the uChar of stack "Test" is "a"

	type "A"
	TestAssert "type ascii char 'A' maps correctly", \
			the uKeyCode of stack "Test" is charToNum("A") and \
			the uChar of stack "Test" is "A"

	type numToCodepoint(0x00E9) -- code for lower case E-acute
	TestAssert "type native char" && numToCodepoint(0x00E9) && "maps correctly", \
			the uKeyCode of stack "Test" is charToNum(numToCodepoint(0x00E9)) and \
			the uChar of stack "Test" is numToCodepoint(0x00E9)

	type numToCodepoint(0x431) -- code for Cyrillic Be
	TestAssert "type bmp char" && numToCodepoint(0x431) && "maps correctly", \
			the uKeyCode of stack "Test" is (0x431 bitOr 2^24) and \
			the uChar of stack "Test" is numToCodepoint(0x431) 

	type numToCodepoint(0x1D110) -- code for music fermata
	TestAssert "type smp char" && numToCodepoint(0x1D110) && "maps correctly", \
			the uKeyCode of stack "Test" is (0x1D110 bitOr 2^24) and \
			the uChar of stack "Test" is numToCodepoint(0x1D110) 
end TestType
