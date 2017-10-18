script "CoreFilesBOM"
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

// Tests correction implemented for Bug 18998

constant kStackName = "utf8bom"
constant kTestFile = "utf8bom.livecodescript"

on TestLoadBomScriptOnlyStackFile
   local tUrlBinfilePath
   local tScriptOnlyStack
   
   put "binfile:" & TestGetInputFilesFolder() & slash & kTestFile \
      into tUrlBinfilePath
   
   open stack url tUrlBinfilePath
   
   // Technically this is testing the opening of a stack that
   // is coming from a chunk.  Most likely source would be URL
   // "binfile:" and "http(s):", but others could be possible.
   
   TestAssert "Open UTF8 BOM Script-Only stack file as URL binfile", \
      kStackName is among the lines of the openStacks
end TestLoadBomScriptOnlyStackFile

on TestLoadBomScriptOnlyStackFromString
   local tScriptOnlyStack
   
   put numToByte(239) & numToByte(187) & numToByte(191) & \
      "script" && quote & kStackName & quote & cr & "--" \
      into tScriptOnlyStack
   
   open stack tScriptOnlyStack
   
   TestAssert "Open UTF8 BOM Script-Only stack from string", \
      kStackName is among the lines of the openStacks
end TestLoadBomScriptOnlyStackFromString

on TestLoadNoBomScriptOnlyStackFromString
   local tScriptOnlyStack
   
   put "script" && quote & kStackName & quote & cr & "--" \
      into tScriptOnlyStack
   
   open stack tScriptOnlyStack
   
   TestAssert "Open Script-Only stack without BOM from string", \
      kStackName is among the lines of the openStacks
end TestLoadNoBomScriptOnlyStackFromString
