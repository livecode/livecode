script "CoreStackProps"
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

on TestStackNameProp
	create stack "Test"
	
	local tVar
	put "Test 1,2,3" into tVar
	set the name of stack "Test" to tVar
	TestAssert "Invalid chars in name are replaced with '_'", there is a stack "Test 1_2_3"
	
	# Bug 18343
	TestAssert "Variable used to set stack name is unmodified", tVar is "Test 1,2,3"
end TestStackNameProp

on TestCreateControlInCard	
	create stack "Temp"

	create card "Other"
	create card "Other2"

	create button "Test" in card "Other"

	TestAssert "test if the button was created in the correct card" , there is a button "Test" of card "Other"
end TestCreateControlInCard

on TestCreateCardInStack	
	create stack "Temp"
	create stack "Temp2"

	create card "Other" in stack "Temp"


	TestAssert "test if the card was created in the correct stack" , there is a card "Other" of stack "Temp"
end TestCreateCardInStack

on TestCreateWidgetInCard
	create stack "Temp"

	create card "Other" in stack "Temp"
	create card "Other 2"

	create widget "My Navbar" as "com.livecode.widget.navbar" in card "Other" of stack "Temp"

	TestAssert "test if the widget was created in the correct card", there is a widget "My Navbar" of card "Other" of stack "Temp"
end TestCreateWidgetInCard

on TestCreateControlInContainerWithDefaultStackLocked
	create stack "Temp"
	
	set the cantModify of stack "Temp" to true
	
	create stack "Temp2"
	
	set the defaultStack to "Temp2"
	
	create group "Bar"
	
	set the defaultStack to "Temp"
	
	create button "Foo" in group "Bar" of stack "Temp2"

	TestAssert "testing no exceptions were thrown" , true
end TestCreateControlInContainerWithDefaultStackLocked