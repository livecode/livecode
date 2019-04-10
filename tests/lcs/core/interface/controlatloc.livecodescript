script "CoreControlAtLoc"
/*
Copyright (C) 2019 LiveCode Ltd.

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


on TestControlAtLoc
	TestSkipIfNot "lcb"
	
	create stack "Test"
	set the defaultStack to "Test"
	
	local tControl, tLoc
	repeat for each item tType in "button,field,scrollbar,image,player,graphic,widget,group"
		switch tType
		case "graphic"
			do format("create %s", tType)
			set the opaque of it to true
			set the style of it to "rectangle"
			break
		case "group"
			do format("create %s", tType)
			set the opaque of it to true
			set the rect of it to 0,0,100,100
			break
		case "widget"
			TestLoadExtension "com.livecode.library.widgetutils"
			TestLoadExtension "com.livecode.widget.clock"
			create widget "TestWidget" as "com.livecode.widget.clock"
			break
		default
			do format("create %s", tType)
		end switch
		put it into tControl
		TestDiagnostic tControl

		put the loc of tControl into tLoc
		TestDiagnostic tLoc
		TestDiagnostic "hit" && controlAtLoc(tLoc)
		TestAssert format("visible %s controlAtLoc", tType), the long id of controlAtLoc(tLoc) is tControl
		set the visible of tControl to false
		TestAssert format("invisible %s controlAtLoc", tType), controlAtLoc(tLoc) is empty
		delete tControl
	end repeat
end TestControlAtLoc
