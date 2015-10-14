script "CoreFiles"
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

on TestFiles2


set the defaultFolder to specialFolderPath("temporary")

put "evalThereIsAFolderTest" into tTestFolder
create folder tTestFolder

TestAssert "test", tTestFolder is among the lines of the directories

delete folder tTestFolder

repeat for each line tLine in the directories
	TestAssert "test", there is a folder tLine
	TestAssert "test", there is not a file tLine
end repeat

end TestFiles2
on TestFiles5
 // 12039

put the defaultFolder into tOldCwd
set the defaultFolder to specialFolderPath("temporary")

put "evalfilestest.txt" into tTestfile
put "some text" into url ("binfile:" & tTestfile)
TestAssert "test", tTestfile is among the lines of the files
delete file tTestfile
TestAssert "test", tTestfile is not among the lines of the files

set the defaultFolder to tOldCwd

repeat for each line tLine in the files
if char -4 to -1 of tLine is not "sock" then
	TestDiagnostic tLine
	TestAssert "files are files", there is a file tLine
	TestAssert "files are not folders", there is not a folder tLine
end if
end repeat

end TestFiles5
on TestFiles6


// set up test keys & values
if the platform is "Win32" then
local tTestKey = "HKEY_CURRENT_USER\Software\LiveCode\tests\ListRegistry\"
repeat for each item tKey in "Key1,Key2,Key1\Key1"
	TestAssert "test", setRegistry(tTestKey & tKey & "\value", "value of" && tKey)
end repeat

TestAssert "test", listRegistry(tTestKey) is ("Key1" & return & "Key2")
TestAssert "test", listRegistry(tTestKey & "Key1") is "Key1"
TestAssert "test", listRegistry(tTestKey & "Key2") is empty
end if

end TestFiles6
on TestFiles7


if the platform is "win32" then
	local tFolder, tShortFolder
	put the longfilepath of the defaultFolder into tFolder
	put the shortfilepath of tFolder into tShortFolder
	
	TestAssert "test", (the longfilepath of tShortFolder) is tFolder

else if the platform is "MacOS" then
	TestAssert "test", (the longfilepath of the defaultFolder) is (the defaultFolder)

else
// Doesn't run on other platforms
TestAssert "test", true
end if

end TestFiles7
on TestFiles8


set the defaultFolder to specialFolderPath("temporary")

local tFileName
put "openFilesTest.txt" into tFileName

open file tFileName for write

TestAssert "test", tFileName is among the lines of the openFiles

close file tFileName

end TestFiles8
on TestFiles11

TestAssert "test", the processId is an integer

end TestFiles11
on TestFiles14


if the platform is "win32" then
	local tShortFolder, tLongFolder
	
	put the shortfilepath of the defaultFolder into tShortFolder
	put the longfilepath of the defaultFolder into tLongFolder

	// check path components are in 8.3 format
	set the itemdelimiter to "/"
	repeat for each item tPart in tShortFolder
		local tLength, tOffset
		put the number of chars in tPart into tLength
		put offset(".", tPart) into tOffset
		if tOffset is 0 then
			TestAssert "test", tLength <= 8
		else
			TestAssert "test", (tOffset <= 9) and (tLength - tOffset <= 3)
		end if
	end repeat
	
	TestAssert "test", (the longfilepath of tShortFolder) is tLongFolder

else
	TestAssert "test", (the shortfilepath of the defaultFolder) is (the defaultFolder)
end if

end TestFiles14
on TestFiles15


local tFolder, tHome
put specialFolderPath("Temporary") into tFolder
TestAssert "test", tFolder is not empty
TestAssert "test", there is a folder tFolder

// windows numeric folder codes
if the platform is "win32" then
	put specialFolderPath("home") into tHome

	// user app data
	put specialFolderPath("0x1a") into tFolder
	TestAssert "test", tFolder is not empty
	TestAssert "test", there is a folder tFolder
	
	TestAssert "test", (tHome is not empty) and (tFolder is not tHome) and (tFolder begins with tHome)
end if

// mac 4-letter codes
if the platform is "macos" then
	put specialFolderPath("home") into tHome

	// user public folder
	put specialFolderPath("pubb") into tFolder
	TestAssert "test", tFolder is not empty
	TestAssert "test", there is a folder tFolder
	
	TestAssert "test", (tHome is not empty) and (tFolder is not tHome) and (tFolder begins with tHome)
end if

end TestFiles15
on TestFiles16


local tPath
put tempname() into tPath

TestAssert "test", tempname() is not tPath
TestAssert "test", tPath is not empty
TestAssert "test", there is not a file tPath

end TestFiles16
on TestFiles17


set the defaultFolder to specialFolderPath("temporary")

put "evalThereIsAFileTest.txt" into tTestfile
put "some text" into url ("binfile:" & tTestfile)

TestAssert "test", there is a file tTestFile
TestAssert "test", not (there is not a file tTestFile)

delete file tTestfile

TestAssert "test", not (there is a file tTestFile)
TestAssert "test", there is not a file tTestFile

end TestFiles17
on TestFiles18


set the defaultFolder to specialFolderPath("temporary")

put "evalThereIsAFolderTest" into tTestFolder
create folder tTestFolder

TestAssert "test", there is a folder tTestFolder
TestAssert "test", not (there is not a folder tTestFolder)

delete folder tTestFolder

TestAssert "test", not (there is a folder tTestFolder)
TestAssert "test", there is not a folder tTestFolder

end TestFiles18
on TestFiles20

   
open file "Test.tmp" for write

close file "Test.tmp"

close file "Test.tmp"

TestAssert "test", the result is "File is not open"

delete file "Test.tmp"

end TestFiles20
on TestFiles22

   
open file "Test.tmp" for write

close file "Test.tmp"

delete file "Test.tmp"

TestAssert "test", there is not a file "Test.tmp"

end TestFiles22
on TestFiles23


local taskList

if the platform is "Win32" then
put shell("tasklist /V") into taskList

if taskList contains "notepad.exe" then
	get shell("taskkill /IM notepad.exe")
end if

launch "notepad.exe"

put shell("tasklist /V") into taskList

TestAssert "test", taskList contains "notepad.exe"
else if the platform is "MacOS" then
put shell("ps -ax") into taskList

launch (specialfolderpath("apps") & slash & "textEdit.app")

put shell("ps -ax") into taskList

TestAssert "test", taskList contains "textEdit.app"

set the wholematches to false
get lineOffset("textEdit.app", taskList)
local tPID
put word 1 of line it of taskList into tPID
get shell ("kill" && tPID)
end if	

end TestFiles23
on TestFiles24

   
set the defaultFolder to specialFolderPath("Desktop")

open file "Test.txt" for write

close file "Test.txt"

local taskList

put the openFiles into taskList

if the platform is "Win32" then	
launch "Test.txt" with "notepad"

put shell("tasklist /V") into taskList

TestAssert "test", taskList contains "notepad.exe"

get shell("taskkill /IM notepad.exe")

delete file "Test.txt"
else if the platform is "MacOS" then

launch "Test.txt" with (specialfolderpath("apps") & slash & "textEdit.app")

put shell("ps -ax") into taskList

TestAssert "test", taskList contains "textEdit"
set the wholematches to false
get lineOffset("textEdit.app", taskList)
local tPID
put word 1 of line it of taskList into tPID
get shell ("kill" && tPID)	

end if	

end TestFiles24
on TestFiles26


if the platform is "Android" or the platform is "iphone" then set the defaultFolder to specialFolderPath("cache")
open file "Test.tmp" for write

close file "Test.tmp"

TestAssert "test", there is a file "Test.tmp"

delete file "Test.tmp"

end TestFiles26
on TestFiles27


local tOutput

if the platform is "Win32" then	
put shell("tasklist /V") into taskList

if taskList contains "cmd.exe" then
	get shell("taskkill /IM notepad.exe")
end if

open process "cmd.exe" for text update

write "echo foo" & CR to process "cmd.exe"

read from process "cmd.exe" until "echo foo"

read from process "cmd.exe" for 1 line

read from process "cmd.exe" for 1 line

put char 1 to 3 of it into tOutput

TestAssert "test", tOutput is "foo"

close process "cmd.exe"
end if

end TestFiles27
on TestFiles28


if the platform is "Android" or the platform is "iphone" then set the defaultFolder to specialFolderPath("cache")
   
open file "Test.tmp" for write

close file "Test.tmp"

rename "Test.tmp" to "Test2.tmp"

TestAssert "test", there is a file "Test2.tmp" and there is not a file "Test.tmp"

delete file "Test2.tmp"

end TestFiles28