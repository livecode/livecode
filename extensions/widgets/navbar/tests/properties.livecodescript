script "NavBarWidgetProperties"
/*
Copyright (C) 2015 - 2016 LiveCode Ltd.

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
   TestLoadExtension "com.livecode.library.iconsvg"
   TestLoadExtension "com.livecode.widget.navbar"
   create widget "testNavBar" as "com.livecode.widget.navbar"
end TestSetup

on TestTeardown
   delete widget "testNavBar"
end TestTeardown

on TestCreateNavBar
   TestAssert "created navbar exists", there is a widget "testNavBar"
end TestCreateNavBar


on TestSetBGOpacity
   set the opaque of widget "testNavBar" to false
   TestAssert "set backgroundOpacity to false", the opaque of widget "testNavBar" is false
   
   set the opaque of widget "testNavBar" to true
   TestAssert "set backgroundOpacity to true", the opaque of widget "testNavBar" is true
end TestSetBGOpacity

on TestSetActionStyle
   set the itemStyle of widget "testNavBar" to "text"
   TestAssert "actionStyle set to text", the itemStyle of widget "testNavBar" is "text"
   
   set the itemStyle of widget "testNavBar" to "icons"
   TestAssert "actionStyle set to icons", the itemStyle of widget "testNavBar" is "icons"
   
   set the itemStyle of widget "testNavBar" to "both"
   TestAssert "actionStyle set to both", the itemStyle of widget "testNavBar" is "both"
end TestSetActionStyle

on TestSetHilitedItem
	set the hilitedItem of widget "testNavBar" to 1
   	TestAssert "hilitedItem set to 1", the hilitedItem of widget "testNavBar" is 1
   	
   	set the hilitedItem of widget "testNavBar" to 2
   	TestAssert "hilitedItem set to 2", the hilitedItem of widget "testNavBar" is 2
end TestSetHilitedItem

on TestSetHilitedItemName
	set the hilitedItemName of widget "testNavBar" to "contacts"
   	TestAssert "hilitedItemName set to Contacts", the hilitedItemName of widget "testNavBar" is "contacts"
   	
   	set the hilitedItemName of widget "testNavBar" to "search"
   	TestAssert "hilitedItemName set to Search", the hilitedItemName of widget "testNavBar" is "search"
end TestSetHilitedItemName
