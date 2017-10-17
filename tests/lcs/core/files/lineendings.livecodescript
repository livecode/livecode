script "CoreFilesLineEndings"
/*
Copyright (C) 2016 LiveCode Ltd.

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


// Tests verify that a script only stack retains line endings when
// edited within LiveCode.  Bug 17810

constant kTestDirectory = "__CoreFilesLineEndings"
constant kTestFile = "__CoreFilesLineEndings/LineEndingTest.livecodescript"
constant kTestStack = "LineEndingTest"

command TestSetup
   create folder kTestDirectory
end TestSetup

command TestTearDown
   local tFile, tFolder
   put the defaultfolder into tFolder
   set the defaultfolder to kTestDirectory
   repeat for each line tFile in the files
      delete file tFile
   end repeat
   set the defaultfolder to tFolder
   delete folder kTestDirectory
end TestTearDown


on TestMacLineEndingScriptOnlyStack
   local tStackText, tLineEnding, tFileContents
   
   put numToByte(13) into tLineEnding
   
   put "script" && quote & kTestStack & quote & \
      tLineEnding & "--" & tLineEnding into tStackText
   
   put tStackText into url ("binfile:" & kTestFile)
   
   open stack kTestFile
   save stack kTestStack
   
   put url ("binfile:" & kTestFile) into tFileContents
   
   TestAssert "Script-Only stack retains Mac line endings", \
      tStackText is char 4 to -1 of tFileContents
end TestMacLineEndingScriptOnlyStack


on TestWindowsLineEndingScriptOnlyStack
   local tStackText, tLineEnding
   
   put numToByte(13) & numToByte(10) into tLineEnding

   put "script" && quote & kTestStack & quote & \
      tLineEnding & "--" & tLineEnding into tStackText
   
   put tStackText into url ("binfile:" & kTestFile)
   
   open stack kTestFile
   save stack kTestStack
   
   put url ("binfile:" & kTestFile) into tFileContents
   
   TestAssert "Script-Only stack retains Windows line endings", \
      tStackText is char 4 to -1 of tFileContents
end TestWindowsLineEndingScriptOnlyStack


on TestUnixLineEndingScriptOnlyStack
   local tStackText, tLineEnding
   
   put numToByte(10) into tLineEnding

   put "script" && quote & kTestStack & quote & \
      tLineEnding & "--" & tLineEnding into tStackText
   
   put tStackText into url ("binfile:" & kTestFile)
   
   open stack kTestFile
   save stack kTestStack
   
   put url ("binfile:" & kTestFile) into tFileContents
   
   TestAssert "Script-Only stack retains Unix line endings", \
      tStackText is char 4 to -1 of tFileContents
end TestUnixLineEndingScriptOnlyStack
