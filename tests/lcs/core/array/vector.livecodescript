script "CoreArrayVector"
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

command DoTestVectorDotMismatchedArrays
   local tLeft, tRight
   put 1 into tLeft[1]
   put 2 into tLeft["a"]
   put 3 into tLeft["foo"]
   put 2 into tRight[1]
   put 3 into tRight["a"]
   put 5 into tRight["bar"]
   return vectorDotProduct(tLeft, tRight)
end DoTestVectorDotMismatchedArrays

on TestVectorDotProduct
   local tLeft, tRight

   put empty into tLeft
   put empty into tRight
   TestAssert "dot of empty vectors is 0", \
      vectorDotProduct(tLeft, tRight) is 0.0

   put empty into tLeft
   put empty into tRight
   put 1 into tLeft[1]
   put 2 into tLeft[2]
   put 3 into tLeft[3]
   put 2 into tRight[1]
   put 3 into tRight[2]
   put 5 into tRight[3]
   TestAssert "(1,2,3) dot (2,3,5) is 23", \
      vectorDotProduct(tLeft, tRight) is 23

   put empty into tLeft
   put empty into tRight
   put 1 into tLeft["foo"]
   put 2 into tLeft["bar"]
   put 3 into tLeft["baz"]
   put 2 into tRight["foo"]
   put 3 into tRight["bar"]
   put 5 into tRight["baz"]
   TestAssert "non-integer keys (1,2,3) dot (2,3,5) is 23", \
      vectorDotProduct(tLeft, tRight) is 23
   TestAssertThrow "dot of mismatched vectors is error", \
      "DoTestVectorDotMismatchedArrays", the long id of me, \ 
      "EE_VECTORDOT_MISMATCH"
end TestVectorDotProduct
