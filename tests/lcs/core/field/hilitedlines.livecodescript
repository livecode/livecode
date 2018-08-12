script "CoreFieldHilitedLines"
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

constant kFieldName = "myTestField"

private command createField
   open this stack
   create field kFieldName
   put 1 & return & 2 & return & 3 into field kFieldName
   set the listbehavior of field kFieldName to true
   set the multiplehilites of field kFieldName to true
   set the noncontiguoushilites of field kFieldName to true
end createField


on TestEmptyInTheBeginning
   local tHilitedLines
   createField
   set the hilitedlines of field kFieldName to empty,1,2,3
   put the hilitedlines of field kFieldName into tHilitedLines
   
   TestAssert "empty in the beginning", tHilitedLines is "1,2,3"
end TestEmptyInTheBeginning


on TestEmptyInTheMiddle
   local tHilitedLines
   createField
   
   set the hilitedlines of field kFieldName to 1,empty,3
   put the hilitedlines of field kFieldName into tHilitedLines

   TestAssert "empty in the middle", tHilitedLines is "1,3"
end TestEmptyInTheMiddle

on TestEmptyInTheEnd
   local tHilitedLines
   createField
   
   set the hilitedlines of field kFieldName to 1,2,empty
   put the hilitedlines of field kFieldName into tHilitedLines
   
   TestAssert "empty in the end", tHilitedLines is "1,2"
end TestEmptyInTheEnd


on TestNonContiguousSelection
   createField
   
   set the hilitedlines of field kFieldName to 1,3
   
   TestAssert "selectedText of a list field with selected lines", the hilitedLines of field kFieldName is "1,3"
   
   set the hilitedLines of field kFieldName to empty
   TestAssert "selectedText of a list field without selected lines", the hilitedLines of field kFieldName is empty
end TestNonContiguousSelection

on TestCanSelectTextOf65535Length
    createField

    put format("%065535d",0) into field kFieldName

    select char 1 to -1 of field kFieldName

    TestAssert "no crash implies correctly selected text" , true    
end TestCanSelectTextOf65535Length