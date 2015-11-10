script "CoreUnicodeFiles"
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

// Returns true pResult is an error for the command read
private function invalidReadResult pResult
   if pResult is not empty and pResult is not "EOF" then
      return true
   end if
   return false
end invalidReadResult

private function getTextFile  pEncoding, pIsDecomposed, pHasBOM
   local tBom, tDecomposed
   if pHasBOM then
      put "BOM" into tBom
   else
      put "noBOM" into tBom
   end if
   
   if pIsDecomposed then
      put "_NFD" into tDecomposed
   end if
   
   return merge("[[pEncoding]][[tDecomposed]]_[[tBom]].txt")
end getTextFile

private function getStackFolder
   local tStackFolder
   set the itemDelimiter to slash
   
   return item 1 to -2 of the filename of this stack
end getStackFolder

// Open the appropriate file according to the encoding, decomposition and presence
// of a BOM.
// Return true on success, false on failure
//
private function openFile pFile, pEncoding
   open file pFile for pEncoding text read
   return the result is empty
end openFile

// Open the appropriate relying on the BOM to find out the encoding
private function openFileGuessingEncoding pFile
   open file pFile for text read
   return the result is empty
end openFileGuessingEncoding

// Read from the file char by char, and put what is read in rContent
// Return true in success, false on failure
private function readCharByChar pFile, @rContent
   local tContent
   put empty into tResult
   repeat until tResult is "EOF"
      read from file pFile for 1 chars
      
      put the result into tResult
      if invalidReadResult(tResult) then
         return false
      end if
      
      put it after tContent
   end repeat
   
   put tContent into rContent
   return true
end readCharByChar

// Read until EOF from pFile
// Put the content read in rContent
// Returns true on success, false on failure
private function readAll pFile, @rContent
   local tContent
   
   read from file pFile until EOF
   
   if invalidReadResult(the result) then
      return false
   end if
   
   put it into rContent
   return true
end readAll

// Read until å
// Put the content read in rContent
// Returns true on success, false on failure
private function readUntil pFile, @rContent
   local tContent
   
   read from file pFile until "å"
   
   if invalidReadResult(the result) then
      return false
   end if
   
   put it into rContent
   return true
end readUntil

// Close pFile
private command closeFile pFile
   close file pFile
end closeFile

constant kReadCharByChar = "charByChar"
constant kReadAll = "all"
constant kReadUntil = "until"

constant kNoBom = false
constant kBom = true

constant kGuessEncoding = true
constant kDontGuessEncoding = false

constant kReadAllContent = "aв€🜁åo̫͂"
constant kReadUntilFirstRead = "aв€🜁"
constant kReadUntilRemaining = "åo̫͂"

/*command TestAssert pTest, pSuccess
if not pSuccess then
   put pTest & LF after messages
end if
end TestAssert

command TestAssertBroken pTest, pSuccess
   if pSuccess then
      put pTest & LF after messages
   end if
end TestAssertBroken*/

// Helper to use TestAssertBroken when needed
private command __testAssert pMess, pValue, pBroken
   put pMess
   if pBroken then
      TestAssertBroken pMess, pValue
   else
      TestAssert pMess, pValue
   end if
end __testAssert

// Run all the tests for an encoding.
//  Test for each combination of:
//   - presence of BOM in the file,
//   - opening of the file by guessing the encoding,
//   - decomposed file
//
private command runTest pEncoding, pReadType, pSkipFailures
   repeat for each item tBom in kNoBom,kBOM
      repeat for each item tGuess in kGuessEncoding,kDontGuessEncoding
         repeat for each item tDecomposed in "false,true"
            local tFilename, tTextFile
            put getTextFile(pEncoding, tDecomposed, tBom) into tTextFile
            put getStackFolder() & slash & tTextFile into tFilename
            
            if tGuess then
               get openFileGuessingEncoding(tFilename)
            else
               get openFile(tFilename, pEncoding)
            end if
            
            // Now read using the method asked for.
            // Note that reading from a file without a BOM and trying to 
            // guess the encoding from the BOM is an expected failure
            local tContent, tExpectedFailure
            put (not tBom) and tGuess into tExpectedFailure
            
            // We can't mix expected failures and successful tests in a single test
            if pSkipFailures is tExpectedFailure then
               next repeat
            end if
            
            switch pReadType
               case kReadAll
                  get readAll(tFilename, tContent)
                  __TestAssert merge("readAll from [[tTextFile]]"), tContent is kReadAllContent, tExpectedFailure
                  break
               case kReadCharByChar
                  get readCharByChar(tFilename, tContent)
                  __TestAssert merge("readCharByChar from [[tTextFile]]"), tContent is kReadAllContent, tExpectedFailure
                  break
               case kReadUntil
                  // We read until, and then the remaining part
                  get readUntil(tFilename, tContent)
                  TestAssert merge("readUntil from [[tTextFile]] - first read"), tContent is kReadUntilFirstRead, tExpectedFailure
                  get readAll(tFilename, tContent)
                  TestAssert merge("readUntil from [[tTextFile]] - second read"), tContent is kReadUntilRemaining, tExpectedFailure
                  break
            end switch
            
            closeFile tFile
         end repeat
      end repeat
   end repeat
end runTest

////////////////
// UTF-8
////////////////

// Tests for UTF-8 reading char by char
on TestUnicodeFiles1
   runTest "utf8", kReadCharByChar, true
end TestUnicodeFiles1

on TestUnicodeFiles2
   runTest "utf8", kReadCharByChar, false
end TestUnicodeFiles2

// Tests for UTF-8 reading all
on TestUnicodeFiles3
   runTest "utf8", kReadAll, true
end TestUnicodeFiles3

on TestUnicodeFiles4
   runTest "utf8", kReadAll, false
end TestUnicodeFiles4

// Tests for UTF-8 reading until
on TestUnicodeFiles5
   runTest "utf8", kReadUntil, true
end TestUnicodeFiles5

on TestUnicodeFiles6
   runTest "utf8", kReadUntil, false
end TestUnicodeFiles6

////////////////
// UTF-16
////////////////

// Tests for UTF-16 reading char by char
on TestUnicodeFiles7
   runTest "utf16", kReadCharByChar, true
end TestUnicodeFiles7

on TestUnicodeFiles8
   runTest "utf16", kReadCharByChar, false
end TestUnicodeFiles8

// Tests for UTF-16 reading all
on TestUnicodeFiles9
   runTest "utf16", kReadAll, true
end TestUnicodeFiles9

on TestUnicodeFiles10
   runTest "utf16", kReadAll, false
end TestUnicodeFiles10

// Tests for UTF-16 reading until
on TestUnicodeFiles11
   runTest "utf16", kReadUntil, true
end TestUnicodeFiles11

on TestUnicodeFiles12
   runTest "utf16", kReadUntil, false
end TestUnicodeFiles12

// Tests for UTF-16LE reading char by char
on TestUnicodeFiles13
   runTest "utf16LE", kReadCharByChar, true
end TestUnicodeFiles13

on TestUnicodeFiles14
   runTest "utf16LE", kReadCharByChar, false
end TestUnicodeFiles14

// Tests for UTF-16LE reading all
on TestUnicodeFiles15
   runTest "utf16LE", kReadAll, true
end TestUnicodeFiles15

on TestUnicodeFiles16
   runTest "utf16LE", kReadAll, false
end TestUnicodeFiles16

// Tests for UTF-16LE reading until
on TestUnicodeFiles17
   runTest "utf16LE", kReadUntil, true
end TestUnicodeFiles17

on TestUnicodeFiles18
   runTest "utf16LE", kReadUntil, false
end TestUnicodeFiles18

////////////////
// UTF-32
////////////////

// Tests for UTF-32 reading char by char
on TestUnicodeFiles19
   runTest "utf32", kReadCharByChar, true
end TestUnicodeFiles19

on TestUnicodeFiles20
   runTest "utf32", kReadCharByChar, false
end TestUnicodeFiles20

// Tests for UTF-32 reading all
on TestUnicodeFiles21
   runTest "utf32", kReadAll, true
end TestUnicodeFiles21

on TestUnicodeFiles22
   runTest "utf32", kReadAll, false
end TestUnicodeFiles22

// Tests for UTF-32 reading until
on TestUnicodeFiles23
   runTest "utf32", kReadUntil, true
end TestUnicodeFiles23

on TestUnicodeFiles24
   runTest "utf32", kReadUntil, false
end TestUnicodeFiles24

// Tests for UTF-32LE reading char by char
on TestUnicodeFiles25
   runTest "utf32LE", kReadCharByChar, true
end TestUnicodeFiles25

on TestUnicodeFiles26
   runTest "utf32LE", kReadCharByChar, false
end TestUnicodeFiles26

// Tests for UTF-32LE reading all
on TestUnicodeFiles27
   runTest "utf32LE", kReadAll, true
end TestUnicodeFiles27

on TestUnicodeFiles28
   runTest "utf32LE", kReadAll, false
end TestUnicodeFiles28

// Tests for UTF-32LE reading until
on TestUnicodeFiles29
   runTest "utf32LE", kReadUntil, true
end TestUnicodeFiles29

on TestUnicodeFiles30
   runTest "utf32LE", kReadUntil, false
end TestUnicodeFiles30

on runAll
   repeat with i=1 to 15
      do "TestUnicodeFiles" & i
   end repeat
   breakpoint
end runAll
