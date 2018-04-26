script "MacNativeSingleLineFieldWidgetProperties"
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

on TestSetup
	TestLoadExtension "com.livecode.library.widgetutils"
	TestLoadExtension "com.livecode.library.scriptitems"
	TestLoadExtension "com.livecode.widget.native.mac.textfield"
	create widget "TestWidget" as "com.livecode.widget.native.mac.textfield"
end TestSetup

on TestText
	set the text of widget "TestWidget" to "something"
	TestAssert "text property of Mac field non-empty", \
		the text of widget "TestWidget" is "something"
	
	set the text of widget "TestWidget" to empty
	TestAssert "text property of Mac field empty", \
		the text of widget "TestWidget" is empty
end TestText

on TestShowBorder
	set the showBorder of widget "TestWidget" to false
	TestAssert "showBorder property of Mac field false", \
		the showBorder of widget "TestWidget" is false
	set the showBorder of widget "TestWidget" to true
	TestAssert "showBorder property of Mac field true", \
		the showBorder of widget "TestWidget" is true	
end TestShowBorder

on TestShowFocusBorder
	set the showFocusBorder of widget "TestWidget" to false
	TestAssert "showFocusBorder property of Mac field false", \
		the showFocusBorder of widget "TestWidget" is false
	set the showFocusBorder of widget "TestWidget" to true
	TestAssert "showFocusBorder property of Mac field true", \
		the showFocusBorder of widget "TestWidget" is true	
end TestShowFocusBorder