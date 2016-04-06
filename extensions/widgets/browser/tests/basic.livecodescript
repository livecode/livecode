script "browserWidget"
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
	TestLoadExtension "com.livecode.widget.browser"
end TestSetup

on TestCreateBrowser
	create widget "testBrowser" as "com.livecode.widget.browser"
	TestAssert "created browser exists", there is a widget "testBrowser"
	delete widget "testBrowser"
end TestCreateBrowser

on TestSetHTMLText
	local tHTML
	put "<html><head>TestData</head><body>HTMLText Test Content</body><html>" into tHTML
	
	create widget "testBrowser" as "com.livecode.widget.browser"
	set the htmltext of widget "testBrowser" to tHTML
	
	TestAssert "url property unset by htmltext", the url of widget "testBrowser" is empty
	TestAssert "htmltext property roundtrip", the htmltext of widget "testBrowser" is tHTML
	
	delete widget "testBrowser"
end TestSetHTMLText

on TestSetUrlToEmpty
	create widget "testBrowser" as "com.livecode.widget.browser"
	set the url of widget "testBrowser" to "http://livecode.com"
	
	// Now unset the url - this should clear the contents of the browser
	set the url of widget "testBrowser" to empty
	
	TestAssert "cleared url is empty", the url of widget "testBrowser" is empty
	TestAssert "cleared htmlText is empty", the htmlText of widget "testBrowser" is empty
	
	delete widget "testBrowser"
end TestSetUrlToEmpty
