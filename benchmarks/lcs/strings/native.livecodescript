script "StringsNative"
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

local sLinesText, sItemsText, sCasedChunksText, sWords, sPartialWords

private command _SetupData
   local tItemsText, tLinesText, tCasedChunkText
   BenchmarkLoadNativeTextFile "dictionary.native.txt"
   put the result into tLinesText
   
   local tWord, tWords, tPartialWords, tInterval
   put the number of lines of tLinesText div 10 into tInterval
   repeat with i = 1 to 10
      put line i * tInterval of tLinesText into tWord
      if (i mod 2) is not 0 then
         put toUpper(tWord) into tWord
      end if
      
      put tWord & tWord & tWord & tWord into tWords[i]
      put char 5 to -5 of tWords[i] into tPartialWords[i]
      
      put tWords[i] into line i * tInterval of tLinesText
   end repeat
   
   put tLinesText into tItemsText
   replace return with comma in tItemsText
   put tLinesText into tCasedChunkText
   replace "a" with "z" in tCasedChunkText
   replace return with "a" in tCasedChunkText
   
   put tLinesText into sLinesText
   put tItemsText into sItemsText
   put tCasedChunkText into sCasedChunksText
   put tPartialWords into sPartialWords
   put tWords into sWords
end _SetupData

on BenchmarkOffset
   _SetupData
   
   local tLinesText, tWords
   put sLinesText into tLinesText
   put sWords into tWords
   
   repeat for each word tMode in "Exact Caseless"
      if tMode is "Exact" then
         set the caseSensitive to true
      else
         set the caseSensitive to false
      end if
      
      local tLinesTextToUse
      if not the caseSensitive then
         put toUpper(tLinesText) into tLinesTextToUse
      else
         put tLinesText into tLinesTextToUse
      end if
      
      BenchmarkStartTiming tMode
      repeat 100 times
         get offset(tWords[1], tLinesTextToUse)
         get offset(tWords[2], tLinesTextToUse)
         get offset(tWords[3], tLinesTextToUse)
         get offset(tWords[4], tLinesTextToUse)
         get offset(tWords[5], tLinesTextToUse)
         get offset(tWords[6], tLinesTextToUse)
         get offset(tWords[7], tLinesTextToUse)
         get offset(tWords[8], tLinesTextToUse)
         get offset(tWords[9], tLinesTextToUse)
         get offset(tWords[10], tLinesTextToUse)
      end repeat
      BenchmarkStopTiming
   end repeat
end BenchmarkOffset

on BenchmarkContains
   _SetupData
   
   local tLinesText, tWords
   put sLinesText into tLinesText
   put sWords into tWords
   
   repeat for each word tMode in "Exact Caseless"
      if tMode is "Exact" then
         set the caseSensitive to true
      else
         set the caseSensitive to false
      end if
      
      local tLinesTextToUse
      if not the caseSensitive then
         put toUpper(tLinesText) into tLinesTextToUse
      else
         put tLinesText into tLinesTextToUse
      end if
      
      BenchmarkStartTiming tMode
      repeat 100 times
         get tLinesTextToUse contains tWords[1]
         get tLinesTextToUse contains tWords[2]
         get tLinesTextToUse contains tWords[3]
         get tLinesTextToUse contains tWords[4]
         get tLinesTextToUse contains tWords[5]
         get tLinesTextToUse contains tWords[6]
         get tLinesTextToUse contains tWords[7]
         get tLinesTextToUse contains tWords[8]
         get tLinesTextToUse contains tWords[9]
         get tLinesTextToUse contains tWords[10]
      end repeat
      BenchmarkStopTiming
   end repeat
end BenchmarkContains

on BenchmarkContainsLargeNeedle
   local tHaystack, tNeedle
   put "FOOFbarbFOOFbarb" into tHaystack
   repeat 16 times
      put tHaystack after tHaystack
   end repeat
   put "1234" into char -4 to -1 of tHaystack
   put char -1024 to -1 of tHaystack into tNeedle
   
   repeat for each word tMode in "Exact Caseless"
      if tMode is "Exact" then
         set the caseSensitive to true
      else
         set the caseSensitive to false
      end if
      
      local tHaystackToUse
      if not the caseSensitive then
         put toUpper(tHaystack) into tHaystackToUse
      else
         put tHaystack into tHaystackToUse
      end if
      
      BenchmarkStartTiming tMode
      repeat 10 times
         get tHaystackToUse contains tNeedle
      end repeat
      BenchmarkStopTiming
   end repeat
end BenchmarkContainsLargeNeedle

on BenchmarkBeginsWith
   _SetupData
   
   repeat for each word tMode in "Exact Caseless"
      if tMode is "Exact" then
         set the caseSensitive to true
      else
         set the caseSensitive to false
      end if
      
      local tLinesText, tWords
      put sLinesText into tLinesText
      put sWords into tWords
      
      local tPrefixes
      repeat with i = 1 to 10
         put tWords[i] & tWords[i] & tWords[i] & tWords[i] & tWords[i] into tWords[i]
         put tWords[i] & tWords[i] & tWords[i] & tWords[i] & tWords[i] into tWords[i]
         put tWords[i] & tWords[i] & tWords[i] & tWords[i] & tWords[i] into tWords[i]
         put tWords[i] & tWords[i] & tWords[i] & tWords[i] & tWords[i] into tWords[i]
         put tWords[i] & tWords[i] & tWords[i] & tWords[i] & tWords[i] into tWords[i]
         put tWords[i] & tWords[i] & tWords[i] & tWords[i] & tWords[i] into tWords[i]
         put char 1 to (the number of chars in tWords[i] div 2) of tWords[i] into tPrefixes[i]
         if not the caseSensitive then
            put toLower(tWords[i]) into tWords[i]
            put toUpper(tPrefixes[i]) into tPrefixes[i]
         end if
      end repeat
      
      BenchmarkStartTiming tMode
      repeat 1000 times
         get tWords[1] begins with tPrefixes[1]
         get tWords[2] begins with tPrefixes[2]
         get tWords[3] begins with tPrefixes[3]
         get tWords[4] begins with tPrefixes[4]
         get tWords[5] begins with tPrefixes[5]
         get tWords[6] begins with tPrefixes[6]
         get tWords[7] begins with tPrefixes[7]
         get tWords[8] begins with tPrefixes[8]
         get tWords[9] begins with tPrefixes[9]
         get tWords[10] begins with tPrefixes[10]
      end repeat
      BenchmarkStopTiming
   end repeat
end BenchmarkBeginsWith

on BenchmarkEndsWith
   _SetupData
   
   repeat for each word tMode in "Exact Caseless"
      if tMode is "Exact" then
         set the caseSensitive to true
      else
         set the caseSensitive to false
      end if
      
      local tLinesText, tWords
      put sLinesText into tLinesText
      put sWords into tWords
      
      local tSuffixes
      repeat with i = 1 to 10
         put tWords[i] & tWords[i] & tWords[i] & tWords[i] & tWords[i] into tWords[i]
         put tWords[i] & tWords[i] & tWords[i] & tWords[i] & tWords[i] into tWords[i]
         put tWords[i] & tWords[i] & tWords[i] & tWords[i] & tWords[i] into tWords[i]
         put tWords[i] & tWords[i] & tWords[i] & tWords[i] & tWords[i] into tWords[i]
         put tWords[i] & tWords[i] & tWords[i] & tWords[i] & tWords[i] into tWords[i]
         put tWords[i] & tWords[i] & tWords[i] & tWords[i] & tWords[i] into tWords[i]
         put char -(the number of chars in tWords[i] div 2) to -1 of tWords[i] into tSuffixes[i]
         if not the caseSensitive then
            put toLower(tWords[i]) into tWords[i]
            put toUpper(tSuffixes[i]) into tSuffixes[i]
         end if
      end repeat
      
      BenchmarkStartTiming tMode
      repeat 1000 times
         get tWords[1] ends with tSuffixes[1]
         get tWords[2] ends with tSuffixes[2]
         get tWords[3] ends with tSuffixes[3]
         get tWords[4] ends with tSuffixes[4]
         get tWords[5] ends with tSuffixes[5]
         get tWords[6] ends with tSuffixes[6]
         get tWords[7] ends with tSuffixes[7]
         get tWords[8] ends with tSuffixes[8]
         get tWords[9] ends with tSuffixes[9]
         get tWords[10] ends with tSuffixes[10]
      end repeat
      BenchmarkStopTiming
   end repeat
end BenchmarkEndsWith

on BenchmarkDelimitedChunkOffsetWithSkip
   _SetupData
   
   repeat for each word tMode in "Exact Caseless"
      local tSkip, tNumber
      
      if tMode is "Exact" then
         set the caseSensitive to true
      else
         set the caseSensitive to false
      end if
      
      local tLinesText, tItemsText, tCasedChunksText
      put sLinesText into tLinesText
      put sItemsText into tItemsText
      put sCasedChunksText into tCasedChunksText
      
      if not the caseSensitive then
         put toUpper(sLinesText) into tLinesText
         put toUpper(sItemsText) into tItemsText
         put toUpper(sCasedChunksText) into tCasedChunksText
      end if
      
      --------
      set the lineDelimiter to return
      
      put 0 into tSkip
      BenchmarkStartTiming tMode && "Partial LineOffset With Skip"
      repeat 5000 times
         put lineOffset("th", tLinesText, tSkip) into tNumber
         if tNumber is 0 then
            exit repeat
         end if
         add tNumber to tSkip
      end repeat
      BenchmarkStopTiming
      
      --------
      set the itemDelimiter to comma
      
      put 0 into tSkip
      BenchmarkStartTiming tMode && "Partial ItemOffset With Skip"
      repeat 5000 times
         put itemOffset("th", tItemsText, tSkip) into tNumber
         if tNumber is 0 then
            exit repeat
         end if
         add tNumber to tSkip
      end repeat
      BenchmarkStopTiming
      
      --------
      set the itemDelimiter to "a"
      
      put 0 into tSkip
      BenchmarkStartTiming tMode && "Partial CasedChunkOffset With Skip"
      repeat 5000 times
         put itemOffset("th", tCasedChunksText, tSkip) into tNumber
         if tNumber is 0 then
            exit repeat
         end if
         add tNumber to tSkip
      end repeat
      BenchmarkStopTiming
   end repeat
end BenchmarkDelimitedChunkOffsetWithSkip

on BenchmarkDelimitedChunkOffset
   _SetupData
   
   repeat for each word tMode in "Exact Caseless"
      if tMode is "Exact" then
         set the caseSensitive to true
      else
         set the caseSensitive to false
      end if
      
      local tPartialWords, tWords, tLinesText, tItemsText, tCasedChunksText
      put sPartialWords into tPartialWords
      put sWords into tWords
      put sLinesText into tLinesText
      put sItemsText into tItemsText
      put sCasedChunksText into tCasedChunksText
      
      set the wholeMatches to false
      BenchmarkStartTiming tMode && "Partial LineOffset"
      repeat 100 times
         get lineOffset(tPartialWords[1], tLinesText)
         get lineOffset(tPartialWords[2], tLinesText)
         get lineOffset(tPartialWords[3], tLinesText)
         get lineOffset(tPartialWords[4], tLinesText)
         get lineOffset(tPartialWords[5], tLinesText)
         get lineOffset(tPartialWords[6], tLinesText)
         get lineOffset(tPartialWords[7], tLinesText)
         get lineOffset(tPartialWords[8], tLinesText)
         get lineOffset(tPartialWords[9], tLinesText)
         get lineOffset(tPartialWords[10], tLinesText)
      end repeat
      BenchmarkStopTiming
      
      --------
      set the wholeMatches to true
      BenchmarkStartTiming tMode && "Whole LineOffset"
      repeat 100 times
         get lineOffset(tWords[1], tLinesText)
         get lineOffset(tWords[2], tLinesText)
         get lineOffset(tWords[3], tLinesText)
         get lineOffset(tWords[4], tLinesText)
         get lineOffset(tWords[5], tLinesText)
         get lineOffset(tWords[6], tLinesText)
         get lineOffset(tWords[7], tLinesText)
         get lineOffset(tWords[8], tLinesText)
         get lineOffset(tWords[9], tLinesText)
         get lineOffset(tWords[10], tLinesText)
      end repeat
      BenchmarkStopTiming
      
      --------
      set the wholeMatches to false
      BenchmarkStartTiming tMode && "Partial ItemOffset"
      repeat 100 times
         get itemOffset(tPartialWords[1], tItemsText)
         get itemOffset(tPartialWords[2], tItemsText)
         get itemOffset(tPartialWords[3], tItemsText)
         get itemOffset(tPartialWords[4], tItemsText)
         get itemOffset(tPartialWords[5], tItemsText)
         get itemOffset(tPartialWords[6], tItemsText)
         get itemOffset(tPartialWords[7], tItemsText)
         get itemOffset(tPartialWords[8], tItemsText)
         get itemOffset(tPartialWords[9], tItemsText)
         get itemOffset(tPartialWords[10], tItemsText)
      end repeat
      BenchmarkStopTiming
      
      --------
      set the wholeMatches to true
      BenchmarkStartTiming tMode && "Whole ItemOffset"
      repeat 100 times
         get itemOffset(tWords[1], tItemsText)
         get itemOffset(tWords[2], tItemsText)
         get itemOffset(tWords[3], tItemsText)
         get itemOffset(tWords[4], tItemsText)
         get itemOffset(tWords[5], tItemsText)
         get itemOffset(tWords[6], tItemsText)
         get itemOffset(tWords[7], tItemsText)
         get itemOffset(tWords[8], tItemsText)
         get itemOffset(tWords[9], tItemsText)
         get itemOffset(tWords[10], tItemsText)
      end repeat
      BenchmarkStopTiming
      
      --------
      
      set the wholeMatches to false
      if the caseSensitive then
         set the itemDelimiter to "a"
      else
         set the itemDelimiter to "A"
      end if
      BenchmarkStartTiming tMode && "Partial CasedChunkOffset"
      repeat 100 times
         get itemOffset(tPartialWords[1], tCasedChunksText)
         get itemOffset(tPartialWords[2], tCasedChunksText)
         get itemOffset(tPartialWords[3], tCasedChunksText)
         get itemOffset(tPartialWords[4], tCasedChunksText)
         get itemOffset(tPartialWords[5], tCasedChunksText)
         get itemOffset(tPartialWords[6], tCasedChunksText)
         get itemOffset(tPartialWords[7], tCasedChunksText)
         get itemOffset(tPartialWords[8], tCasedChunksText)
         get itemOffset(tPartialWords[9], tCasedChunksText)
         get itemOffset(tPartialWords[10], tCasedChunksText)
      end repeat
      BenchmarkStopTiming
      
      --------
      set the wholeMatches to true
      if the caseSensitive then
         set the itemDelimiter to "a"
      else
         set the itemDelimiter to "A"
      end if
      BenchmarkStartTiming tMode && "Whole CasedChunkOffset"
      repeat 100 times
         get itemOffset(tWords[1], tCasedChunksText)
         get itemOffset(tWords[2], tCasedChunksText)
         get itemOffset(tWords[3], tCasedChunksText)
         get itemOffset(tWords[4], tCasedChunksText)
         get itemOffset(tWords[5], tCasedChunksText)
         get itemOffset(tWords[6], tCasedChunksText)
         get itemOffset(tWords[7], tCasedChunksText)
         get itemOffset(tWords[8], tCasedChunksText)
         get itemOffset(tWords[9], tCasedChunksText)
         get itemOffset(tWords[10], tCasedChunksText)
      end repeat
      BenchmarkStopTiming
   end repeat
end BenchmarkDelimitedChunkOffset

on BenchmarkDelimitedChunkAmong
   _SetupData
   
   repeat for each word tMode in "Exact Caseless"
      if tMode is "Exact" then
         set the caseSensitive to true
      else
         set the caseSensitive to false
      end if
      
      local tPartialWords, tWords, tLinesText, tItemsText, tCasedChunksText
      put sPartialWords into tPartialWords
      put sWords into tWords
      put sLinesText into tLinesText
      put sItemsText into tItemsText
      put sCasedChunksText into tCasedChunksText
      
      --------
      BenchmarkStartTiming tMode && "AmongTheLinesOf"
      repeat 100 times
         get tWords[1] is among the lines of tLinesText
         get tWords[2] is among the lines of tLinesText
         get tWords[3] is among the lines of tLinesText
         get tWords[4] is among the lines of tLinesText
         get tWords[5] is among the lines of tLinesText
         get tWords[6] is among the lines of tLinesText
         get tWords[7] is among the lines of tLinesText
         get tWords[8] is among the lines of tLinesText
         get tWords[9] is among the lines of tLinesText
         get tWords[10] is among the lines of tLinesText
      end repeat
      BenchmarkStopTiming
      
      --------
      set the wholeMatches to true
      BenchmarkStartTiming tMode && "AmongTheItemsOf"
      repeat 100 times
         get tWords[1] is among the items of tItemsText
         get tWords[2] is among the items of tItemsText
         get tWords[3] is among the items of tItemsText
         get tWords[4] is among the items of tItemsText
         get tWords[5] is among the items of tItemsText
         get tWords[6] is among the items of tItemsText
         get tWords[7] is among the items of tItemsText
         get tWords[8] is among the items of tItemsText
         get tWords[9] is among the items of tItemsText
         get tWords[10] is among the items of tItemsText
      end repeat
      BenchmarkStopTiming
      
      --------
      set the wholeMatches to true
      if the caseSensitive then
         set the itemDelimiter to "a"
      else
         set the itemDelimiter to "A"
      end if
      BenchmarkStartTiming tMode && "AmongTheCasedChunksOf"
      repeat 100 times
         get tWords[1] is among the items of tCasedChunksText
         get tWords[2] is among the items of tCasedChunksText
         get tWords[3] is among the items of tCasedChunksText
         get tWords[4] is among the items of tCasedChunksText
         get tWords[5] is among the items of tCasedChunksText
         get tWords[6] is among the items of tCasedChunksText
         get tWords[7] is among the items of tCasedChunksText
         get tWords[8] is among the items of tCasedChunksText
         get tWords[9] is among the items of tCasedChunksText
         get tWords[10] is among the items of tCasedChunksText
      end repeat
      BenchmarkStopTiming
   end repeat
end BenchmarkDelimitedChunkAmong

on BenchmarkDelimitedChunkOf
   _SetupData
   
   repeat for each word tMode in "Exact Caseless"
      if tMode is "Exact" then
         set the caseSensitive to true
      else
         set the caseSensitive to false
      end if
      
      local tPartialWords, tWords, tLinesText, tItemsText, tCasedChunksText
      put sPartialWords into tPartialWords
      put sWords into tWords
      put sLinesText into tLinesText
      put sItemsText into tItemsText
      put sCasedChunksText into tCasedChunksText
      
      BenchmarkStartTiming tMode && "LineOf"
      repeat 10000 times
         get line 10 of tLinesText
         get line 100 of tLinesText
         get line 1000 of tLinesText
         get line 10000 of tLinesText
      end repeat
      BenchmarkStopTiming
      
      BenchmarkStartTiming tMode && "ItemOf"
      repeat 10000 times
         get item 10 of tItemsText
         get item 100 of tItemsText
         get item 1000 of tItemsText
         get item 10000 of tItemsText
      end repeat
      BenchmarkStopTiming
      
      if the caseSensitive then
         set the itemDelimiter to "a"
      else
         set the itemDelimiter to "A"
      end if
      BenchmarkStartTiming tMode && "CasedChunkOf"
      repeat 10000 times
         get item 10 of tCasedChunksText
         get item 100 of tCasedChunksText
         get item 1000 of tCasedChunksText
         get item 10000 of tCasedChunksText
      end repeat
      BenchmarkStopTiming
   end repeat
end BenchmarkDelimitedChunkOf

on BenchmarkDelimitedChunkRangeOf
   _SetupData
   
   repeat for each word tMode in "Exact Caseless"
      if tMode is "Exact" then
         set the caseSensitive to true
      else
         set the caseSensitive to false
      end if
      
      local tPartialWords, tWords, tLinesText, tItemsText, tCasedChunksText
      put sPartialWords into tPartialWords
      put sWords into tWords
      put sLinesText into tLinesText
      put sItemsText into tItemsText
      put sCasedChunksText into tCasedChunksText
      
      BenchmarkStartTiming tMode && "LineRangeOf"
      repeat 10000 times
         get line 1 to 10 of tLinesText
         get line 10 to 100 of tLinesText
         get line 100 to 1000 of tLinesText
         get line 1000 to 10000 of tLinesText
      end repeat
      BenchmarkStopTiming
      
      BenchmarkStartTiming tMode && "ItemRangeOf"
      repeat 10000 times
         get item 1 to 10 of tItemsText
         get item 10 to 100 of tItemsText
         get item 100 to 1000 of tItemsText
         get item 1000 to 10000 of tItemsText
      end repeat
      BenchmarkStopTiming
      
      if the caseSensitive then
         set the itemDelimiter to "a"
      else
         set the itemDelimiter to "A"
      end if
      BenchmarkStartTiming tMode && "CasedChunkRangeOf"
      repeat 10000 times
         get item 1 to 10 of tCasedChunksText
         get item 10 to 100 of tCasedChunksText
         get item 100 to 1000 of tCasedChunksText
         get item 1000 to 10000 of tCasedChunksText
      end repeat
      BenchmarkStopTiming
   end repeat
end BenchmarkDelimitedChunkRangeOf

on BenchmarkDelimitedReverseChunkOf
   _SetupData
   
   repeat for each word tMode in "Exact Caseless"
      if tMode is "Exact" then
         set the caseSensitive to true
      else
         set the caseSensitive to false
      end if
      
      local tPartialWords, tWords, tLinesText, tItemsText, tCasedChunksText
      put sPartialWords into tPartialWords
      put sWords into tWords
      put sLinesText into tLinesText
      put sItemsText into tItemsText
      put sCasedChunksText into tCasedChunksText
      
      BenchmarkStartTiming tMode && "ReverseLineOf"
      repeat 100 times
         get line -10 of tLinesText
         get line -100 of tLinesText
         get line -1000 of tLinesText
         get line -10000 of tLinesText
      end repeat
      BenchmarkStopTiming
      
      BenchmarkStartTiming tMode && "ReverseItemOf"
      repeat 100 times
         get item -10 of tItemsText
         get item -100 of tItemsText
         get item -1000 of tItemsText
         get item -10000 of tItemsText
      end repeat
      BenchmarkStopTiming
      
      if the caseSensitive then
         set the itemDelimiter to "a"
      else
         set the itemDelimiter to "A"
      end if
      BenchmarkStartTiming tMode && "ReverseCasedChunkOf"
      repeat 100 times
         get item -10 of tCasedChunksText
         get item -100 of tCasedChunksText
         get item -1000 of tCasedChunksText
         get item -10000 of tCasedChunksText
      end repeat
      BenchmarkStopTiming
   end repeat
end BenchmarkDelimitedReverseChunkOf
   
on BenchmarkBoundaryChunkOf
   _SetupData
   
   local tCasedChunksText
   put sCasedChunksText into tCasedChunksText
   
   BenchmarkStartTiming "CharOf"
   repeat 100000 times
      get char 1 of tCasedChunksText
      get char 10 of tCasedChunksText
      get char 100 of tCasedChunksText
      get char 1000 of tCasedChunksText
      get char 10000 of tCasedChunksText
   end repeat
   BenchmarkStopTiming
end BenchmarkBoundaryChunkOf

on BenchmarkBoundaryRangeChunkOf
   _SetupData
   
   local tCasedChunksText
   put sCasedChunksText into tCasedChunksText
   
   BenchmarkStartTiming "CharRangeOf"
   repeat 100000 times
      get char 1 to 10 of tCasedChunksText
      get char 10 to 100 of tCasedChunksText
      get char 100 to 1000 of tCasedChunksText
      get char 1000 to 10000 of tCasedChunksText
      get char 10000 to 100000 of tCasedChunksText
   end repeat
   BenchmarkStopTiming
end BenchmarkBoundaryRangeChunkOf
