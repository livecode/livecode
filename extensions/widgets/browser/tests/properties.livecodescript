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
	TestLoadExtension "com.livecode.browser"
	TestLoadExtension "com.livecode.extensions.livecode.browser"
end TestSetup

on TestSetHTMLText
	local tHTML
	put "<html><head>TestData</head><body>HTMLText Test Content</body><html>" into tHTML
	
	create widget "testBrowser" as "com.livecode.extensions.livecode.browser"
	set the htmltext of "testBrowser" to tHTML
	
	TestAssert "url property unset by htmltext", the url of "testBrowser" is empty
	TestAssert "htmltext property roundtrip", the htmltext of "testBrowser" is tHTML
	
	delete widget "testBrowser"
end TestSetHTMLText

on TestSetUrl
	local tHTML
	put "<html><head>TestData</head><body>HTMLText Test Content</body><html>" into tHTML
	
	local tUrl
	put "https://livecode.com" into tUrl
	
	create widget "testBrowser" as "com.livecode.extensions.livecode.browser"
	set the htmltext of "testBrowser" to tHTML
	
	set the url of "testBrowser" to tUrl
	
	TestAssert "htmltext property unset by url", the htmltext of "testBrowser" is not tHTML
	TestAssert "url property roundtrip", the url of "testBrowser" is tUrl

	delete widget "testBrowser"	
end TestSetUrl