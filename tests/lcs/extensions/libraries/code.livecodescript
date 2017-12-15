script "TestExtensionCodeLibrary"
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
	TestSkipIfNot "lcb"
	TestLoadExtension "com.livecode.library.ini"
end TestSetup

on TestLoadCodeLibrary
	TestAssert "library mapping is set for code library", the revLibraryMapping["inih"] is not empty
	
	local tFileData
	put url ("file:" & TestGetInputFile("test.ini")) into tFileData
	
	local tIni
	put iniToArray(tFileData) into tIni
	
	TestAssert "library extension can load code library", tIni["user"]["name"] is "Bob Smith"
end TestLoadCodeLibrary
