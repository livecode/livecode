script "CoreStringOffset"
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

on TestOffsetSingleUnicodeChar
   local tNeedle, tHaystack
   -- Native needle
   put "B" into tNeedle
   -- Unicode pattern
   put "aAbBcCdDeEfFgG" & numToCodepoint(0x3B1) into tHaystack

   set the caseSensitive to true
   TestAssert "offset of native needle in unicode string - case-sensitive", offset(tNeedle, tHaystack) is 4

   set the caseSensitive to false
   TestAssert "offset of native needle in unicode string - caseless", offset(tNeedle, tHaystack) is 3
end TestOffsetSingleUnicodeChar
