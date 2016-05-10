script "CoreStringsByteoffset"
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

on TestByteOffsetNonEmptyNeedleEmptyHaystack
   local tNeedle, tHaystack
   put "DEF" into tNeedle
   put empty into tHaystack

   TestAssert "byteoffset of non-empty needle in empty haystack", byteoffset(tNeedle, tHaystack) is 0

end TestByteOffsetNonEmptyNeedleEmptyHaystack


on TestByteOffsetNonEmptyNeedleNonEmptyHaystack
   local tNeedle, tHaystack
   put "DEF" into tNeedle
   put "ABCDEF" into tHaystack

   TestAssert "byteoffset of non-empty needle in non-empty haystack", byteoffset(tNeedle, tHaystack) is 4

end TestByteOffsetNonEmptyNeedleNonEmptyHaystack


on TestByteOffsetEmptyNeedleEmptyHaystack
   local tNeedle, tHaystack
   put empty into tNeedle
   put empty into tHaystack

   TestAssert "byteoffset of empty needle in empty haystack", byteoffset(tNeedle, tHaystack) is 0

end TestByteOffsetEmptyNeedleEmptyHaystack


on TestByteOffsetEmptyNeedleNonEmptyHaystack
   local tNeedle, tHaystack
   put empty into tNeedle
   put "ABCDEF" into tHaystack

   TestAssert "byteoffset of empty needle in non-empty haystack", byteoffset(tNeedle, tHaystack) is 0

end TestByteOffsetEmptyNeedleNonEmptyHaystack