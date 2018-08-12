script "CoreExecutionTypeConversion"
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

on TestDoubleToInt
   TestAssert "round up positive reals in integer expressions", char 0.5 of "ab" is "a" 
   TestAssert "round down negative reals in integer expressions", char -0.5 of "ab" is "b" 
end TestDoubleToInt

on TestDoubleToUInt
   create button
   create button
   relayer it before layer 0.5
   TestAssert "round up positive reals in unsigned integer expressions", the layer of it is 1
end TestDoubleToUInt

on TestStrictUInt
   create button
   local tEmpty
   TestAssert "empty variable does not convert to 0 in object ID", not exists(button id tEmpty)
end TestStrictUInt

on TestJunkAfterNumber -- Bug 18651
   TestAssert "whitespace after number", "0    " is a number
   TestAssert "digits after number", "0 9" is not a number
   TestAssert "junk after number", "0 foo" is not a number
end TestJunkAfterNumber
