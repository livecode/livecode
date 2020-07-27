script "CoreFilesFolders"
/*
Copyright (C) 2015-2016 LiveCode Ltd.

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

on TestFolderInListing
   set the defaultFolder to specialFolderPath("temporary")
   
   local tTestFolder
   put "evalThereIsAFolderTest" into tTestFolder
   create folder tTestFolder
   
   TestAssert merge("[[tTestFolder]] is in the folder listing"), tTestFolder is among the lines of the directories
   
   delete folder tTestFolder
   
   repeat for each line tLine in the directories
      TestAssert merge("Folder [[tLine]] is a folder"), there is a folder tLine
      TestAssert merge("Folder [[tLine]] is not a file"), there is not a file tLine
   end repeat
   
end TestFolderInListing

on TestFilesOfFolder
   local tFolder, tTempContents, tTestFile, tTestPath
   put specialFolderPath("temporary") into tFolder

   put "evalfilestest.txt" into tTestFile
   put tFolder & slash & tTestFile into tTestPath
   put "some text" into url ("file:" & tTestPath)

   TestAssert "list non-current directory", tTestFile is among the lines of files(tFolder)

   TestAssert "list non-current directory (detailed)", the number of items in line 1 of files(tFolder, "detailed") > 1

   TestAssert "list non-current directory (detailed-utf8)", the number of items in line 1 of files(tFolder, "detailed-utf8") > 1

   local tOldFolder
   put the defaultFolder into tOldFolder
   set the defaultFolder to tFolder

   TestAssert "list current directory", tTestFile is among the lines of files()

   set the defaultFolder to tOldFolder
   delete file tTestPath
end TestFilesOfFolder

on TestFoldersOfFolder
   local tFolder, tTestFolder, tTestPath
   put specialFolderPath("temporary") into tFolder
   put "evalfolderstest" into tTestFolder
   put tFolder & slash & tTestFolder into tTestPath
   create folder tTestPath
   TestDiagnostic the result

   TestAssert "list non-current directory", tTestFolder is among the lines of folders(tFolder)

   TestAssert "list non-current directory (detailed)", the number of items in line 2 of folders(tFolder, "detailed") > 1

   TestAssert "list non-current directory (detailed-utf8)", the number of items in line 2 of folders(tFolder, "detailed-utf8") > 1

   local tOldFolder
   put the defaultFolder into tOldFolder
   set the defaultFolder to tFolder

   TestAssert "list current directory", tTestFolder is among the lines of folders()

   set the defaultFolder to tOldFolder
   delete folder tTestPath
end TestFoldersOfFolder

on TestBugfix12039
   // 12039
   local tOldCwd, tTestFile
   put the defaultFolder into tOldCwd
   set the defaultFolder to specialFolderPath("temporary")
   
   put "evalfilestest.txt" into tTestfile
   put "some text" into url ("binfile:" & tTestfile)
   TestAssert merge("[[tTestFile]] is amongs the files"), tTestfile is among the lines of the files
   
   delete file tTestfile
   TestAssert merge("[[tTestFile]] is not amongst the files after deletion"), tTestfile is not among the lines of the files
   
   set the defaultFolder to tOldCwd
   
   repeat for each line tLine in the files
      if char -4 to -1 of tLine is not "sock" then
         TestDiagnostic tLine
         TestAssert "files are files", there is a file tLine
         TestAssert "files are not folders", there is not a folder tLine
      end if
   end repeat
   
end TestBugfix12039

on TestFoldersFirstLine
   // ".." must be the first folder listed
   local tFolders
   put the folders into tFolders
   
   TestAssert "Fist line of the folders is always  '..'", first line of tFolders is ".."
end TestFoldersFirstLine

on TestEmptyFolder
   // Bug 16223: if no folder is listed by 'the folders', there should still
   // be ".." at the head of the list
   local tFolders, tNewFolder, tOldCwd
   put the defaultFolder into tOldCwd
   
   // Create a new, empty folder
   put "files__TestEmptyFolder" into tNewFolder
   set the defaultFolder to specialFolderPath("temporary")
   
   create folder tNewFolder

   TestAssert "An empty non-current folder returns '..'", folders(tNewFolder) is ".."
   TestAssert "An empty non-current folder returns empty files", files(tNewFolder) is empty
   TestAssert "An empty non-current folder returns empty detailed files", files(tNewFolder, "detailed") is empty
   TestAssert "An empty non-current folder returns empty detailed-utf8 files", files(tNewFolder, "detailed-utf8") is empty
   
   set the defaultFolder to tNewFolder
   
   put the folders into tFolders
   
   TestAssert "An empty current folder returns '..'", the folders is ".."
   TestAssert "An empty current folder returns empty files", the files is empty
   TestAssert "An empty current folder returns empty detailed files", the detailed files is empty
   
   delete folder tNewFolder
   set the defaultFolder to tOldCwd
end TestEmptyFolder

on TestFoldersOfTilde
   TestSkipIfNot "platform", "MacOS,Linux"
	TestAssert "folders(~) is the same as folders($HOME)", folders($HOME) is folders("~")
end TestFoldersOfTilde

on TestFilesOfTilde
   TestSkipIfNot "platform", "MacOS,Linux"
	TestAssert "files(~) is the same as files($HOME)", files($HOME) is files("~")
end TestFilesOfTilde

on TestResolveFolderAlias
    if the platform is "Win32" then
    	local tFolder
	    put the longfilepath of the defaultFolder into tFolder

	    local tFolderAlias
	    put tFolder & ".lnk" into tFolderAlias

	    create alias tFolderAlias to folder tFolder

	    local tRef
	    put the aliasReference of tFolderAlias into tRef

	    TestAssert "test", tRef is not empty
	
	    delete file tFolderAlias
    end if
end TestResolveFolderAlias

on TestNonExistentFolder
    local tUUID
    put uuid() into tUUID

    set the folder to tUUID
    TestAssert "set the folder to non-existent folder", the result is "can't open directory"
    TestAssert "The folders of non-existent folder", folders(tUUID) is empty
    TestAssert "The files of non-existent folder", files(tUUID) is empty
    TestAssert "The detailed files of non-existent folder", files(tUUID, "detailed") is empty
    TestAssert "The detailed-utf8 files of non-existent folder", files(tUUID, "detailed-utf8") is empty
end TestNonExistentFolder

on TestBugfix22213
   local tTestFolder
   // Create a new, empty folder
   put specialFolderPath("temporary") & "/TestUTF8" into tTestFolder
   create folder tTestFolder

   put empty into url ("file:" & tTestFolder & "/😊.txt")
   local tFiles, tFile
   put files(tTestFolder, "detailed") into tFiles
   put item 1 of line 1 of tFiles into tFile
   TestAssertBroken "The detailed files are native encoded before urlEncoding", \
      textDecode(urlDecode(tFile), "native") is "😊.txt", "Bug 22213"

   put files(tTestFolder, "detailed-utf8") into tFiles
   put item 1 of line 1 of tFiles into tFile
   TestAssert "The detailed-utf8 files are utf8 encoded before urlEncoding", \
      textDecode(urlDecode(tFile), "utf8") is "😊.txt"

   delete file (tTestFolder & "/😊.txt")

   local tFolders, tFolder
   create folder (tTestFolder & "/😊")
   put folders(tTestFolder, "detailed") into tFolders
   put item 1 of line 2 of tFolders into tFolder
   TestAssertBroken "The detailed folders are native encoded before urlEncoding", \
      textDecode(urlDecode(tFolder), "native") is "😊", "Bug 22213"

   put folders(tTestFolder, "detailed-utf8") into tFolders
   TestDiagnostic tFolders
   put item 1 of line 2 of tFolders into tFolder
   TestAssert "The detailed-utf8 folders are utf8 encoded before urlEncoding", \
      textDecode(urlDecode(tFolder), "utf8") is "😊"

   delete folder (tTestFolder & "/😊")
   delete folder tTestFolder
end TestBugfix22213
