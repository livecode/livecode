script "CoreFieldSpaceAbove"
/*
Copyright (C) 2019 LiveCode Ltd.

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

on TestSpaceAboveAndSpaceBelow

   open this stack
   create fld "myFld"
   put "Line one & return & Line 2 & return & Line 3" into fld "myFld"
   TestAssert "getting the effective spaceAbove of line 2", the effective spaceAbove of line 2 of fld "myFld" is 0
   TestAssert "getting the effective spaceBelow of line 2", the effective spaceBelow of line 2 of fld "myFld" is 0

   local tSpaceAbove, tSpaceBelow
   
   put random(5) into tSpaceAbove
   set the spaceAbove of line 2 of fld "myFld" to tSpaceAbove
   TestAssert "Setting the spaceAbove of line 2", the spaceAbove of line 2 of fld "myFld" is tSpaceAbove
   
   put random(5) into tSpaceBelow
   set the spaceBelow of line 2 of fld "myFld" to tSpaceBelow
   TestAssert "Setting the spaceBelow of line 2", the spaceBelow of line 2 of fld "myFld" is tSpaceBelow
   
   put random(5) into tSpaceAbove
   set the spaceAbove of line 2 of fld "myFld" to tSpaceAbove
   TestAssert "Setting the spaceAbove of line 2 does not affect the spaceBelow", the spaceBelow of line 2 of fld "myFld" is tSpaceBelow

   put random(5) into tSpaceBelow
   set the spaceBelow of line 2 of fld "myFld" to tSpaceBelow
   TestAssert "Setting the spaceBelow of line 2 does not affect the spaceAbove", the spaceAbove of line 2 of fld "myFld" is tSpaceAbove

   delete fld "myFld"
end TestSpaceAboveAndSpaceBelow
