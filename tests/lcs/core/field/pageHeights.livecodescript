script "CoreFieldPageHeights"
/*
Copyright (C) 2018 LiveCode Ltd.

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

on TestPageHeights
    
   create field "testPageHeightsField" 
   
   // Recreate field similar to the one of the attached stack to bug 21491
   set the textFont of field 1 to "Segoe UI"
   set the textSize of field 1 to 13
   set the textAlign of field 1 to "left"
   set the fixedLineHeight of field 1 to true
   set the margins of field 1 to 6
   set the width of field 1 to 474
   set the height of field 1 to 692
   
   local tFieldContentFilename
   set the itemDel to slash
   put item 1 to -2 of the filename of me into tFieldContentFilename
   put slash & "_pageHeightsFieldContent.txt" after tFieldContentFilename
   set the htmlText of field 1 to url ("file:/"&tFieldContentFilename)
   
   local tObservedPageHeights
   put the pageHeights of field 1 into tObservedPageHeights
   
   TestAssert "The number of lines of the pageheights is 24", the number of lines in tObservedPageHeights is 24
   delete field 1
    
end TestPageHeights
