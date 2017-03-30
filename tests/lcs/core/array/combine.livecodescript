script "CoreArrayCombine"
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


on TestCombineStrings
   local tArray, tKeyList, tKeys, tValues
   local tUncombinedData
   
   put testArrayData("stringKeys") into tKeys
   put testArrayData("stringValues") into tValues
   
   repeat with x = 1 to the number of items in tKeys
      put item x of tValues into tArray[item x of tKeys]
      put item x of tKeys & tab & item x of tValues & return after tUncombinedData
   end repeat
   delete the last character of tUncombinedData
   
   combine  tArray with return and tab
   
   sort lines of tArray by item 1 of each
   sort lines of tUncombinedData by item 1 of each
   
   TestAssert "Combine strings", tUncombinedData is tArray
end TestCombineStrings


on TestCombineNumbers
   local tArray, tKeyList
   local tUncombinedData

   repeat with x = 1 to 5
      put x into tArray[x]
      put x & tab & x & return after tUncombinedData
   end repeat
   delete the last character of tUncombinedData
   combine tArray with return and tab
   
   TestAssert "Combine numbers", tUncombinedData is tArray
end TestCombineNumbers


on TestCombineMixed
   local tArray, tKeyList, tKeys, tValues
   local tUncombinedData
   
   put testArrayData("stringKeys") into tKeys
   put testArrayData("stringValues") into tValues
   
   repeat with x = 1 to the number of items in tKeys
      put item x of tValues into tArray[item x of tKeys]
      put item x of tKeys & tab & item x of tValues & return after tUncombinedData
   end repeat
   
   repeat with x = 1 to 5
      put x into tArray[x]
      put x & tab & x & return after tUncombinedData
   end repeat
   
   delete the last character of tUncombinedData
   combine  tArray with return and tab
   
   sort lines of tArray by item 1 of each
   sort lines of tUncombinedData by item 1 of each
   
   TestAssert "Combine numbers and strings", tUncombinedData is tArray
end TestCombineMixed


on TestArrayCombineByRow
   local tArray, tKeyList, tValues
   local tUncombinedData
   
   put testArrayData("stringValues") into tValues
   
   repeat with x = 1 to the number of items in tValues
      put item x of tValues into tArray[x]
      put item x of tValues & the rowDelimiter after tUncombinedData
   end repeat
   delete the last character of tUncombinedData
   
   combine tArray by row
   
   sort lines of tArray by item 1 of each
   sort lines of tUncombinedData by item 1 of each   
   
   TestAssert "Combine by row", tUncombinedData is tArray
end TestArrayCombineByRow


on TestArrayCombineByColumn
   local tArray, tKeyList, tValues
   local tUncombinedData
   
   put testArrayData("stringValues") into tValues
   
   repeat with x = 1 to the number of items in tValues
      put item x of tValues into tArray[x]
      put item x of tValues & the columnDelimiter after tUncombinedData
   end repeat
   delete the last character of tUncombinedData
   
   combine tArray by column
   
   sort lines of tArray by item 1 of each
   sort lines of tUncombinedData by item 1 of each   
   
   TestAssert "Combine by column", tUncombinedData is tArray
end TestArrayCombineByColumn
