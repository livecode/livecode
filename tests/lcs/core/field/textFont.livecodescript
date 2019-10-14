script "CoreFieldTextFont"
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

on TestEffectiveTextFont
   open this stack
   create field "myField"
   repeat with x=1 to 4
      put "line" & x & CR after field "myField"
   end repeat
   set the textfont of field "myField" to "Comic San MS"
   
   set the textFont of line 2 of field "myField" to "Arial"
   set the textFont of line 3 of field "myField" to "Cambria"
   set the textFont of line 4 of field "myField" to "Tahoma"
   
   TestAssert "Get textfont of line 1",  the textFont of line 1 of field "myField" is ""
   TestAssert "Get effective textfont of line 1",  the effective textFont of line 1 of field "myField" is "Comic San MS"
   
   TestAssert "Get textfont of line 2",  the textFont of line 2 of field "myField" is "Arial"
   TestAssert "Get effective textfont of line 2",  the effective textFont of line 2 of field "myField" is "Arial"
   
   TestAssert "Get textfont of line 3",  the textFont of line 3 of field "myField" is "Cambria"
   TestAssert "Get effective textfont of line 3",  the effective textFont of line 3 of field "myField" is "Cambria"
   
   TestAssert "Get textfont of line 4",  the textFont of line 4 of field "myField" is "Tahoma"
   TestAssert "Get effective textfont of line 4",  the effective textFont of line 4 of field "myField" is "Tahoma"
   
   delete field "myField"
end TestEffectiveTextFont
