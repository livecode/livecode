script "CoreFiles"
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

on TestRegistryKeys
   // set up test keys & values
   if the platform is "Win32" then
      local tTestKey = "HKEY_CURRENT_USER\Software\LiveCode\tests\ListRegistry\"
      repeat for each item tKey in "Key1,Key2,Key1\Key1"
         TestAssert merge("set Registry key [[tTestKey]]"), setRegistry(tTestKey & tKey & "\value", "value of" && tKey)
      end repeat
      
      TestAssert merge("Key [tTestKey]] has right value"), listRegistry(tTestKey) is ("Key1" & return & "Key2")
      TestAssert merge("Key [tTestKey]]Key1 has right value"), listRegistry(tTestKey & "Key1") is "Key1"
      TestAssert merge("Key [tTestKey]]Key2 has right value"), listRegistry(tTestKey & "Key2") is empty
   else
      testSkip
      testSkip
      testSkip
   end if
end TestRegistryKeys

on TestShortFilePath
   if the platform is "win32" then
      local tFolder, tShortFolder
      put the longfilepath of the defaultFolder into tFolder
      put the shortfilepath of tFolder into tShortFolder
      	
      TestAssert "test", (the longfilepath of tShortFolder) is tFolder
      
   else if the environment is not "mobile" then
      TestAssert "test", (the longfilepath of the defaultFolder) is (the defaultFolder)
   else
      TestSkip
   end if
end TestShortFilePath


on TestFileCreationWithOpenForWrite
   set the defaultFolder to specialFolderPath("temporary")
   
   local tFileName
   put "openFilesTest.txt" into tFileName
   
   open file tFileName for write
   
   TestAssert "Open file for write creates the file", tFileName is among the lines of the openFiles
   
   close file tFileName
end TestFileCreationWithOpenForWrite


on TestProcessID
   TestAssert "processID is an integer", the processId is an integer
end TestProcessID


on TestShortFilePath2
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
   else
      TestSkip
   end if
end TestShortFilePath2


private command __testFolderExists pSkip, pFolder, pFolderType
   local tMess
   put merge("[[pFolderType]] folder exists") into tMess
   if pSkip then
      TestSkip tMess
   else
      TestAssert tMess, there is a folder pFolder
   end if
end __testFolderExists

private command __testSpecialFolder pSkip, pSpecialFolder, pFolderName
   if pFolderName is empty then
      put pSpecialFolder into pFolderName
   end if
   
   __testFolderExists  pSkip, specialFolderPath(pSpecialFolder), pFolderName
end __testSpecialFolder

on TestSpecialFolderPath
   exit TestSpecialFolderPath
   local tEngine,tResources
   local tSkip

   put false into tSkip

   put specialFolderPath("engine") into tEngine
   __testFolderExists tSkip, tEngine, "engine"

   local tFolder, tHome
   
   put specialFolderPath("resources") into tResources
   if the platform is "macos" and the environment is "standalone" then
      // Standalone on Mac have their non-executable resource in a specific folder
      TestAssert "Resources folder on standalone Mac app in is  Resources/_MacOS", tResources ends with "Resources/_MacOS"
   else if the environment is "mobile" then
      testAssert "Resources and Engine specialfolderPaths are the same on mobile", tResources is tEngine
   else
      TestSkip "resources folder exists"
   end if
   
   // Only Linux and HTML5 do not have a Document special folder
   put the platform contains "linux" or the platform is "html5" into tSkip
   __testSpecialFolder tSkip, "documents"
   
   // Only mobiles have Cache special folder
   put the environment is not "mobile" into tSkip
   __testSpecialFolder tSkip, "cache"
   
   put the platform is "android" into tSkip
   __testSpecialFolder tSkip, "home"
   __testSpecialFolder tSkip, "temporary"
   
   // Not all the platforms actually have a Desktop folder - such as servers
   // so we only check that the path returned is not empty.
   if the environment is "mobile" or the platform is "html5" then
      TestSkip  "specialFolderPath(desktop) is not empty"
   else
      TestAssert "specialFolderPath(desktop) is not empty", specialFolderPath("desktop") is not empty
   end if
   
   put the environment is "mobile" or the platform contains "linux" or the platform is "html5" into tSkip
   __testSpecialFolder tSkip, "system"
   __testSpecialFolder tSkip, "support"

   put the environment is "mobile" or the platform contains "linux" into tSkip
   __testSpecialFolder tSkip, "fonts"
   
   //////////////////////////////////////////////////////////////////////////////////////////
   TestDiagnostic "Windows numeric folder codes"
   put the platform is not "win32" into tSkip
   __testSpecialFolder tSkip, "0x001a", "User app"
   __testSpecialFolder tSkip, "0x0023", "All user app"
   __testSpecialFolder tSkip, "0x000d", "My Music"
   __testSpecialFolder tSkip, "0x00e", "My Videos"
   __testSpecialFolder tSkip, "0x0027", "My Pictures"
   __testSpecialFolder tSkip, "0x000a", "Recycle bin"
   __testSpecialFolder tSkip, "start"
   
   
   //////////////////////////////////////////////////////////////////////////////////////////
   // Mac-specific
   TestDiagnostic "Mac-specific special folders"
   put the platform is not "macos" into tSkip
   
   // Preferences folder
   __testSpecialFolder tSkip, "Preferences"
   
   // mac 4-letter codes
   __testSpecialFolder tSkip, "desk", "Desktop"
   __testSpecialFolder tSkip, "trsh", "Trash"
   __testSpecialFolder tSkip, "empt", "Empty trash"
   __testSpecialFolder tSkip, "font", "font"
   __testSpecialFolder tSkip, "pref", "preferences"
   __testSpecialFolder tSkip, "sprf", "PreferencePanes"
   __testSpecialFolder tSkip, "temp", "Temporary files"
   __testSpecialFolder tSkip, "vtmp", "Temporary cachedf files"
   __testSpecialFolder tSkip, "apps", "Applications"
   __testSpecialFolder tSkip, "root", "Root"
   __testSpecialFolder tSkip, "dtop", "System top level"
   __testSpecialFolder tSkip, "dlib", "Library"
   __testSpecialFolder tSkip, "usrs", "Users"
   __testSpecialFolder tSkip, "cusr", "Current user"
   __testSpecialFolder tSkip, "sdat", "Shared"
   
   __testSpecialFolder tSkip, "docs", "User docs"
   __testSpecialFolder tSkip, "pdoc", "User pictures"
   __testSpecialFolder tSkip, "mdoc", "User movies"
   __testSpecialFolder tSkip, "pubb", "User public"
   
   ////////////////////////////////////////////////////////////////////////////////
   // iOS-specific
   put the platform is not "ios" into tSkip
   __testSpecialFolder tSkip, "library"
end TestSpecialFolderPath


on TestTempName
   TestSkipIf "platform", "HTML5"
   local tPath
   put tempname() into tPath
   
   TestAssert "tempName does not return twice the same", tempname() is not tPath
   TestAssert "tempName does not return empty", tPath is not empty
   TestAssert "tempName does not return an existing filename", there is not a file tPath
end TestTempName


on TestThereIsAFile
   set the defaultFolder to specialFolderPath("temporary")
   
   local tTestFile
   put "evalThereIsAFileTest.txt" into tTestFile
   put "some text" into url ("binfile:" & tTestfile)
   
   TestAssert merge("after creation - there is a file [[tTestFile]]"), there is a file tTestFile
   TestAssert merge("after creation - not(there is not a file [[tTestFile]]"), not (there is not a file tTestFile)
   
   delete file tTestfile
   
   TestAssert merge("after deletion - not (there is a file [[tTestFile]]"), not (there is a file tTestFile)
   TestAssert merge("after deletion - there is not a file [[tTestFile]]"), there is not a file tTestFile
end TestThereIsAFile


on TestThereIsAFolder
   set the defaultFolder to specialFolderPath("temporary")
   
   local tTestFolder
   put "evalThereIsAFolderTest" into tTestFolder
   create folder tTestFolder
   
   TestAssert merge("after creation - there is a folder [[tTestFolder]]"), there is a folder tTestFolder
   TestAssert merge("after creation - there is a folder[[tTestFolder]]"), not (there is not a folder tTestFolder)
   
   delete folder tTestFolder
   
   TestAssert merge("after deletion - not (there is a folder [[tTestFolder]]"), not (there is a folder tTestFolder)
   TestAssert merge("after deletion - not (there is a folder [[tTestFolder]]"), there is not a folder tTestFolder
end TestThereIsAFolder


on TestFileOpOnClosedFile
   local tFileName
   put the tempName into tFileName
   
   open file tFileName for write
   close file tFileName
   
   write "Hello" to file tFileName
   TestAssert "write on closed file", the result is "File is not open for write"
   
   close file "Test.tmp"
   TestAssert "close a closed file", the result is "File is not open"
   
   delete file "Test.tmp"
end TestFileOpOnClosedFile


on TestDeleteFile
   local tFileName
   put the tempName into tFileName
   
   open file tFileName for write
   close file tFileName
   
   delete file tFileName
   
   TestAssert "Delete file", there is not a file tFileName
end TestDeleteFile


on TestLaunch
   local taskList
   
   if the platform is "Win32" then
      put shell("tasklist /V") into taskList
      
      if taskList contains "notepad.exe" then
         get shell("taskkill /IM notepad.exe")
      end if
      
      launch "notepad.exe"
      
      put shell("tasklist /V") into taskList
      
      TestAssert "Notepad has been launched", taskList contains "notepad.exe"
   else if the platform is "MacOS" then
      put shell("ps -ax") into taskList
      
      launch (specialfolderpath("apps") & slash & "textEdit.app")
      
      put shell("ps -ax") into taskList
      
      TestAssert "textEdit has been launched", taskList contains "textEdit.app"
      
      set the wholematches to false
      get lineOffset("textEdit.app", taskList)
      local tPID
      put word 1 of line it of taskList into tPID
      get shell ("kill" && tPID)
   else if the platform is "HTML5" then
      TestSkip "launch subprocess", "HTML5 platform does not support subprocesses"
   else
      testSkip
   end if	
end TestLaunch


on TestLaunchWith
   TestSkipIf "platform", "HTML5,iPhone,Android"

   set the defaultFolder to specialFolderPath("Desktop")
   
   open file "Test.txt" for write
   
   close file "Test.txt"
   
   local taskList
   
   put the openFiles into taskList
   
   if the platform is "Win32" then	
      launch "Test.txt" with "notepad"
      
      put shell("tasklist /V") into taskList
      
      TestAssert "file Text.txt lauched with Notepad", taskList contains "notepad.exe"
      
      get shell("taskkill /IM notepad.exe")
      
      delete file "Test.txt"
   else if the platform is "MacOS" then
      
      launch "Test.txt" with (specialfolderpath("apps") & slash & "textEdit.app")
      
      put shell("ps -ax") into taskList
      
      TestAssert "file Text.txt launched with TextEdit", taskList contains "textEdit"
      set the wholematches to false
      get lineOffset("textEdit.app", taskList)
      local tPID
      put word 1 of line it of taskList into tPID
      get shell ("kill" && tPID)	
   else
      testSkip
   end if	
end TestLaunchWith


on TestWriteInCacheFolder
   if the platform is not "Android" and the platform is not "iphone" then 
      testSkip
   else
      set the defaultFolder to specialFolderPath("cache")
      
      open file "Test.tmp" for write
      close file "Test.tmp"
      
      TestAssert "file created", there is a file "Test.tmp"
      
      delete file "Test.tmp"
   end if
end TestWriteInCacheFolder


on TestReadFromProcess
   local tOutput, tProcess
   
   if the platform is "Win32" then      
      put "cmd.exe" into tProcess
      open process tProcess for text update
      
      write "echo foo" & CR to process tProcess
      
      read from process tProcess until "echo foo"
      
      read from process tProcess for 1 line
      
      read from process tProcess for 1 line
      
      put char 1 to 3 of it into tOutput
      
      TestAssert "Reading from process", tOutput is "foo"
      
      close process tProcess
   else if the platform is "macos" or the platform contains "linux" then
      put "/bin/sh" into tProcess
      
      open process tProcess for binary update
      
      write "echo " & quote & "foo" & quote & LF to process tProcess
      
      read from process tProcess for 3 bytes
      put it into tOutput
      
      TestAssert "Reading from process", tOutput is "foo"
      
      close process tProcess
   else
      testSkip
   end if
end TestReadFromProcess


on TestRenameFile
   if the platform is "Android" or the platform is "iphone" then set the defaultFolder to specialFolderPath("cache")
   
   open file "Test.tmp" for write
   
   close file "Test.tmp"
   
   rename "Test.tmp" to "Test2.tmp"
   
   TestAssert "File renamed", there is a file "Test2.tmp" and there is not a file "Test.tmp"
   
   delete file "Test2.tmp"
end TestRenameFile
