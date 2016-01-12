script "CoreStringsChunkOffset"
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

constant kLanguages = "roman greek deseret"
constant kGreekMapping = "αβψδεφγηιξκλμνοπqρστθωςχυζ"
constant kDeseretMapping = "𐐨𐐩𐐪𐐫𐐬𐐭𐐮𐐯𐐰𐐱𐐲𐐳𐐴𐐵𐐶𐐷𐐸𐐹𐐺𐐻𐐼𐐽𐐾𐐿𐑀𐑁"

function mapLetter pChar, pLanguage
   local tNum
   put charToNum(pChar) into tNum
   	
   local tAlphabet
   if pLanguage is "greek" then
      put kGreekMapping into tAlphabet
   else if pLanguage is "deseret" then
      put kDeseretMapping into tAlphabet
   end if
   	
   switch 
      -- Upper case roman letter
      case 65 <= tNum and tNum <= 90
         return toUpper(char tNum - 64 of tAlphabet)
         -- lower case roman letter
      case 97 <= tNum and tNum <= 122
         return char tNum - 96 of tAlphabet
      default
         return pChar
   end switch
end mapLetter

function mapWord pWord, pLanguage
   local tResult
   repeat for each char tChar in pWord
      put mapLetter(tChar, pLanguage) after tResult
   end repeat
   return tResult
end mapWord

local sLanguage
function map pInput
   switch sLanguage
      case "roman"
         return pInput
      case "greek"
      case "deseret"
         return mapWord(pInput, sLanguage)
   end switch	
end map

on TestItemOffset
   TestAssert "empty needle returns 0", itemOffset(empty, "a,b,c") is 0
   
   repeat for each word sLanguage in kLanguages
      repeat for each word tMatch in "caseless exact"
         set the caseSensitive to tMatch is "exact"
         
         set the wholeMatches to false
         	
         TestAssert tMatch && "partial : partial item offset at start" && sLanguage, itemOffset(map("oo"), map("foof,barb,dezd")) is 1
         TestAssert tMatch && "partial : partial item offset in middle" && sLanguage, itemOffset(map("ar"), map("foof,barb,dezd")) is 2
         TestAssert tMatch && "partial : partial item offset at end" && sLanguage, itemOffset(map("ez"), map("foof,barb,dezd")) is 3
         
         TestAssert tMatch && "partial : partial list item offset" && sLanguage, itemOffset(map("of,barb,de"), map("foof,barb,dezd")) is 1
         
         set the wholeMatches to true
         TestAssert tMatch && "whole : whole item offset at start" && sLanguage, itemOffset(map("foof"), map("foof,barb,dezd")) is 1
         TestAssert tMatch && "whole : whole item offset in middle" && sLanguage, itemOffset(map("barb"), map("foof,barb,dezd")) is 2
         TestAssert tMatch && "whole : whole item offset at end" && sLanguage, itemOffset(map("dezd"), map("foof,barb,dezd")) is 3
         TestAssert tMatch && "whole : whole list item offset" && sLanguage, itemOffset(map("barb,dezd"), map("foof,barb,dezd")) is 2
         
         TestAssert tMatch && "whole : not partial item offset at start" && sLanguage, itemOffset(map("fo"), map("foof,barb,dezd")) is 0
         TestAssert tMatch && "whole : not partial item offset in middle" && sLanguage, itemOffset(map("ar"), map("foof,barb,dezd")) is 0
         TestAssert tMatch && "whole : not partial item offset at end" && sLanguage, itemOffset(map("zd"), map("foof,barb,dezd")) is 0
         TestAssert tMatch && "whole : not partial list item offset" && sLanguage, itemOffset(map("of,barb,de"), map("foof,barb,dezd")) is 0
         
         TestAssert tMatch && "whole : whole repeated item offset" && sLanguage, itemOffset(map("barb"), map("foofbarb,foof,barb,dezd")) is 3
         TestAssert tMatch && "whole : whole repeated list item offset" && sLanguage, itemOffset(map("barb,dezd"), map("bard,dezdfoof,foof,barb,dezd")) is 4
      end repeat
      
      set the wholeMatches to true
      set the caseSensitive to false
      TestAssert "caseless : different cased item offset" && sLanguage, itemOffset(map("BARB"), map("foof,barb,dezd")) is 2
      set the caseSensitive to true
      TestAssert "exact : not different cased item offset" && sLanguage, itemOffset(map("BARB"), map("foof,barb,dezd")) is 0
      
      set the wholeMatches to true
      set the caseSensitive to false
      TestAssert "caseless : skipped once whole item offset" && sLanguage, itemOffset(map("dezd"), map("foof,barb,dezd"), 1) is 2
      TestAssert "caseless : skipped too many whole item offset" && sLanguage, itemOffset(map("dezd"), map("foof,barb,dezd"), 3) is 0
      
      TestAssert "caseless : skipped once whole repeated item offset" && sLanguage, itemOffset(map("dezd"), map("dezd,foof,barb,dezd"), 1) is 3
      TestAssert "caseless : skipped too many whole repeated item offset" && sLanguage, itemOffset(map("dezd"), map("dezd,foof,barb,dezd"), 4) is 0
   end repeat
end TestItemOffset

on TestLineOffset
   set the lineDelimiter to comma
   
   TestAssert "empty needle returns 0", lineOffset(empty, "a,b,c") is 0
   
   repeat for each word sLanguage in kLanguages
      repeat for each word tMatch in "caseless exact"
         set the caseSensitive to tMatch is "exact"
         
         set the wholeMatches to false
         
         TestAssert tMatch && "partial : partial line offset at start" && sLanguage, lineOffset(map("oo"), map("foof,barb,dezd")) is 1
         TestAssert tMatch && "partial : partial line offset in middle" && sLanguage, lineOffset(map("ar"), map("foof,barb,dezd")) is 2
         TestAssert tMatch && "partial : partial line offset at end" && sLanguage, lineOffset(map("ez"), map("foof,barb,dezd")) is 3
         
         TestAssert tMatch && "partial : partial list line offset" && sLanguage, lineOffset(map("of,barb,de"), map("foof,barb,dezd")) is 1
         
         set the wholeMatches to true
         TestAssert tMatch && "whole : whole line offset at start" && sLanguage, lineOffset(map("foof"), map("foof,barb,dezd")) is 1
         TestAssert tMatch && "whole : whole line offset in middle" && sLanguage, lineOffset(map("barb"), map("foof,barb,dezd")) is 2
         TestAssert tMatch && "whole : whole line offset at end" && sLanguage, lineOffset(map("dezd"), map("foof,barb,dezd")) is 3
         TestAssert tMatch && "whole : whole list line offset" && sLanguage, lineOffset(map("barb,dezd"), map("foof,barb,dezd")) is 2
         
         TestAssert tMatch && "whole : not partial line offset at start" && sLanguage, lineOffset(map("fo"), map("foof,barb,dezd")) is 0
         TestAssert tMatch && "whole : not partial line offset in middle" && sLanguage, lineOffset(map("ar"), map("foof,barb,dezd")) is 0
         TestAssert tMatch && "whole : not partial line offset at end" && sLanguage, lineOffset(map("zd"), map("foof,barb,dezd")) is 0
         TestAssert tMatch && "whole : not partial list line offset" && sLanguage, lineOffset(map("of,barb,de"), map("foof,barb,dezd")) is 0
         
         TestAssert tMatch && "whole : whole repeated line offset" && sLanguage, lineOffset(map("barb"), map("foofbarb,foof,barb,dezd")) is 3
         TestAssert tMatch && "whole : whole repeated list line offset" && sLanguage, lineOffset(map("barb,dezd"), map("bard,dezdfoof,foof,barb,dezd")) is 4
      end repeat
      
      set the wholeMatches to true
      set the caseSensitive to false
      TestAssert "caseless : different cased line offset" && sLanguage, lineOffset(map("BARB"), map("foof,barb,dezd")) is 2
      set the caseSensitive to true
      TestAssert "exact : not different cased line offset" && sLanguage, lineOffset(map("BARB"), map("foof,barb,dezd")) is 0
      
      set the wholeMatches to true
      set the caseSensitive to false
      TestAssert "caseless : skipped once whole line offset" && sLanguage, lineOffset(map("dezd"), map("foof,barb,dezd"), 1) is 2
      TestAssert "caseless : skipped too many whole line offset" && sLanguage, lineOffset(map("dezd"), map("foof,barb,dezd"), 3) is 0
      
      TestAssert "caseless : skipped once whole repeated line offset" && sLanguage, lineOffset(map("dezd"), map("dezd,foof,barb,dezd"), 1) is 3
      TestAssert "caseless : skipped too many whole repeated line offset" && sLanguage, lineOffset(map("dezd"), map("dezd,foof,barb,dezd"), 4) is 0
   end repeat
end TestLineOffset

on TestChunkOffset
   set the itemDelimiter to "a"
   
   set the wholeMatches to true
   
   set the caseSensitive to false
   TestAssert "caseless lower delimiter chunk offset", itemOffset("...", "$$$a...a%%%") is 2
   TestAssert "caseless upper delimiter chunk offset", itemOffset("...", "$$$A...A%%%") is 2
   
   set the caseSensitive to true
   TestAssert "exact lower delimiter chunk offset", itemOffset("...", "$$$a...a%%%") is 2
   TestAssert "exact not upper delimiter chunk offset", itemOffset("...", "$$$A...A%%%") is 0
end TestChunkOffset
