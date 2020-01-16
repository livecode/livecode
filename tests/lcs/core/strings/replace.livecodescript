script "CoreStringReplace"
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

on TestReplace
   local tNeedle, tNewNeedle, tHaystack
   put "é" into tNeedle
   put "e" into tNewNeedle
   put "héllo" into tHaystack
   replace tNeedle with tNewNeedle in tHaystack
   TestAssert "replace composed with base in composed", tHaystack is "hello"
   
   put normalizeText("é", "NFD") into tNeedle
   put "e" into tNewNeedle
   put "héllo" into tHaystack
   replace tNeedle with tNewNeedle in tHaystack
   TestAssert "replace decomposed with base in composed", tHaystack is "hello"
   
   put "é" into tNeedle
   put "e" into tNewNeedle
   put normalizeText("héllo", "NFD") into tHaystack
   replace tNeedle with tNewNeedle in tHaystack
   TestAssert "replace composed with base in decomposed", tHaystack is "hello"
   
   put normalizeText("é", "NFD") into tNeedle
   put "e" into tNewNeedle
   put normalizeText("héllo", "NFD") into tHaystack
   replace tNeedle with tNewNeedle in tHaystack
   TestAssert "replace decomposed with base in decomposed", tHaystack is "hello"
end TestReplace

on TestReplace_Bug22536
   try
      local tPattern
      put "и" into tPattern
      put tPattern after tPattern
      get the number of chars in tPattern
      get textDecode("foobardsdadadasdasd", "native")
      if true then
         replace tPattern with empty in it
      end if
   catch tError
   end try
   TestAssert "replace never-native unciode in native string", tError is empty
end TestReplace_Bug22536
