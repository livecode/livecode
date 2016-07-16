script "CoreStringTokenOffset"
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

on TestTokenOffset
   local tNeedle, tHaystack
   put "put" && quote & "foo" & quote & "&" & quote & "bar" & quote && "into tVar" into tHaystack

   local tToken = 1
   repeat for each token tNeedle in "put foo & bar into tVar"
      TestAssert "tokenoffset" && tNeedle && "in string", tokenOffset(tNeedle, tHaystack) is tToken
      add 1 to tToken
   end repeat
end TestTokenOffset
