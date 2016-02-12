script "HeaderBarWidgetProperties"
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

on TestSetup
   TestLoadExtension "com.livecode.library.widgetutils"
   TestLoadExtension "com.livecode.library.iconSVG"
   TestLoadExtension "com.livecode.widget.headerBar"
   create widget "testHeader" as "com.livecode.widget.headerBar"
end TestSetup

on TestTeardown
   delete widget "testHeader"
end TestTeardown

on TestCreateHeader
   TestAssert "created header exists", there is a widget "testHeader"
end TestCreateHeader

on TestSetTheme
   set the widgetTheme of widget "testHeader" to "Android"
   TestAssert "theme set to Android", the widgetTheme of widget "testHeader" is "Android"
   
   set the widgetTheme of widget "testHeader" to "iOS"
   TestAssert "theme set to iOS", the widgetTheme of widget "testHeader" is "iOS"
end TestSetTheme

on TestSetBGOpacity
   set the backgroundOpacity of widget "testHeader" to false
   TestAssert "set backgroundOpacity to false", the backgroundOpacity of widget "testHeader" is false
   
   set the backgroundOpacity of widget "testHeader" to true
   TestAssert "set backgroundOpacity to false", the backgroundOpacity of widget "testHeader" is true
end TestSetBGOpacity

on TestSetActionStyle
   set the actionStyle of widget "testHeader" to "names"
   TestAssert "actionStyle set to names", the actionStyle of widget "testHeader" is "names"
   
   set the actionStyle of widget "testHeader" to "icons"
   TestAssert "actionStyle set to icons", the actionStyle of widget "testHeader" is "icons"
end TestSetActionStyle

