script "CoreEngineLocalProperties"
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

constant kLineDel = "linedelimiter"
constant kColDel = "columndelimiter"
constant kItemDel = "itemdelimiter"
constant kRowDel = "rowdelimiter"
constant kOctals = "convertoctals"
constant kCentCutOff = "centurycutoff"
constant kCaseSens = "casesensitive"
constant kWholeMatches = "wholematches"

private function __listWithDelimiter pDelimiter
   return "Some" & pDelimiter & "delimited" & pDelimiter & "text"
end __listWithDelimiter

   // Check that the default property is kept in subfunction call
private command __checkDefaultValue pProperty
   local tSuccess
   if pProperty is kLineDel then
      put the lineDelimiter is return into tSuccess
   else if pProperty is kRowDel then
      put the rowDelimiter is return into tSuccess
   else if pProperty is kColDel then
      put the columnDelimiter is TAB into tSuccess
   else if pProperty is kItemDel then
      put the itemDelimiter is comma into tSuccess
   else if pProperty is kOctals then
      put the convertOctals is false into tSuccess
   else if pProperty is kCentCutOff then
      put the centuryCutoff is 35 into tSuccess
   else if pProperty is kCaseSens then
      put the casesensitive is false into tSuccess
   else if pProperty is kWholeMatches then
      put the wholematches is false into tSuccess
   end if
   
   TestAssert merge("default [[pProperty]]"), tSuccess
end __checkDefaultValue

// Change the delimiter to pValue
private command __changeLocalProperty pProperty, pValue
   if pProperty is kLineDel then
      set the lineDelimiter to pValue
   else if pProperty is kRowDel then
      set the rowDelimiter to pValue
   else if pProperty is kColDel then
      set the columnDelimiter to pValue
   else if pProperty is kItemDel then
      set the itemDelimiter to pValue
   else if pProperty is kOctals then
      set the convertOctals to pValue
   else if pProperty is kCentCutOff then
      set the centuryCutoff to pValue
   else if pProperty is kCaseSens then
      set the casesensitive to pValue
   else if pProperty is kWholeMatches then
      set the wholematches to pValue
   end if
end __changeLocalProperty

// Ensure that a local property is not kept between the handlers
private command __checkLocality pDelimiter, pNewValue
   __checkDefaultValue pDelimiter
   __changeLocalProperty pDelimiter, pNewValue
   __checkDefaultValue pDelimiter
end __checkLocality

private command __useDelimiter pDelimiter, pNewValue
   local tList, tChunkCount
   put __listWithDelimiter(pNewValue) into tList
   
   if pDelimiter is kLineDel then
      set the linedelimiter to pNewValue
      put the number of lines in tList into tChunkCOunt
   else if pDelimiter is kRowDel then
      set the rowdelimiter to pNewValue
      split tList by row
      put the number of elements in tList into tChunkCount
   else if pDelimiter is kColDel then
      set the columndelimiter to pNewValue
      split tList by column
      put the number of elements in tList into tChunkCount
   else if pDelimiter is kItemDel then
      set the itemDelimiter to pNewValue
      put the number of items in tList into tChunkCount
   end if
   
   TestAssert merge("[[pDelimiter]] internally understood"), tChunkCount is 3
end __useDelimiter

// Runs all the tests for a delimiter
private command __checkDelimiter pDelimiter, pDefaultValue
   local tSuccess
   
   __checkLocality pDelimiter, slash
   
   // Now run the tests with single, Unicode and multi-char delimiter
   __useDelimiter pDelimiter, slash
   __useDelimiter pDelimiter, "***"
   __useDelimiter pDelimiter, "Ц"
   
   // Try to set the delimiter to empty - should fail
   local tMess
   put merge("setting [[pDelimiter]] to empty should fail") into tMess
   try
      __changeProperty pDelimiter, empty
      
      TestAssert tMess, false
   catch tErr
      TestAssert tMess, true
   end try
end __checkDelimiter

on TestDelimiters
   __checkDelimiter kLineDel, return
   __checkDelimiter kRowDel, return
   __checkDelimiter kColDel, tab
   __checkDelimiter kItemDel, comma
end TestDelimiters

on TestWholeMatches
   __checkLocality kWholeMatches, true
   
   constant kNeedle = "delim"
   
   local tList
   
   // Test for lineoffset
   put __listWithDelimiter(LF) into tList
   TestAssert "lineoffset without wholematches", lineoffset(kNeedle, tList) is 2
   
   set the wholeMatches to true
   TestAssert "lineoffset with wholematches", lineoffset(kNeedle, tList) is 0
   
   // Test for itemoffset
   put __listWithDelimiter(comma) into tList
   
   set the wholeMatches to false
   TestAssert "itemoffset without wholematches", itemoffset(kNeedle, tList) is 2
   
   set the wholeMatches to true
   TestAssert "itemoffset with wholematches", itemoffset(kNeedle, tList) is 0
   
    // Test for wordoffset
   put __listWithDelimiter(space) into tList
   
   set the wholeMatches to false
   TestAssert "wordoffset without wholematches", wordoffset(kNeedle, tList) is 2
   
   set the wholeMatches to true
   TestAssert "wordoffset with wholematches", wordoffset(kNeedle, tList) is 0
end TestWholeMatches

on TestCaseSensitive
   __checkLocality  kCaseSens, true
   
   repeat for each item tStringType in "ascii,nativeAsUnicode,nativeNotUnicode,simpleUnicode,unicode,sigma,sharps"
      
      local tLowerCase, tUpperCase
      put testConversionString("ascii","lowerCase") into tLowerCase
      put testConversionString("ascii", "upperCase") into tUpperCase
      
      set the caseSensitive to true
      TestAssert merge("[[tStringType]] string, case-sensitive comparison"), tLowerCase is not tUpperCase
      
      set the caseSensitive to false
      TestAssert merge("[[tStringType]] string non-case-sensitive comparison"), tLowerCase is tUpperCase
   end repeat
end TestCaseSensitive

on TestCenturyCutOff
   __checkLocality kCentCutOff, 25
   
   put 25 into tCutOff
   set the centuryCutOff to tCutOff
   
   local tDate
   // Set the date above the centurycutoff
   put "1/1/" & tCutOff + 1 into tDate
   convert tDate to dateItems
   
   TestAssert "centuryCutOff, above", item 1 of tDate is ("19" & tCutOff + 1)
   
   // Set the date below the centurycutoff
   put "1/1/" & tCutOff - 1 into tDate
   convert tDate to dateitems
   
   TestAssert "centuryCutOff, below", item 1 of tDate is ("20" & tCutOff - 1)
end TestCenturyCutOff

on TestConvertOctals
   __checkLocality kOctals, true
   
   set the convertOctals to true
   TestAssert "ConvertOctal: true", "07" + "01" is "010"
   
   set the convertOctals to false
   TestAssert "ConvertOctal: true", "07" + "01" is "08"
end TestConvertOctals
