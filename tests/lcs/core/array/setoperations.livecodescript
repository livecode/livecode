script "CoreArraySetOp"
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

/*
Semantics of array union:

function ArrayUnion(pLeft, pRight, pRecursive)
	repeat for each key tKey in pRight
		if tKey is not among the keys of pLeft then
			put pRight[tKey] into pLeft[tKey]
		else if pRecursive then
			put ArrayUnion(pLeft[tKey], pRight[tKey], true) into pLeft[tKey]
		end if
	end repeat
	
	return pLeft
end ArrayUnion

*/
on TestArrayUnion
	
	local tString1	
	put "a" into tString1
	union tString1 with "b"
	TestAssert "union: left non-array, right non-array", tString1 is "a"
	
	local tString2, tFixedArray
	put "a" into tString2
	put "b" into tFixedArray[1]
	union tString2 with tFixedArray
	TestAssert "union: left non-array, right array", tString2[1] is "b"
	
	local tArray1
	put "a" into tArray1[1]
	union tArray1 with "b"
	TestAssert "union: left array, right non-array", tArray1[1] is "a"
	
	local tArray2
	put "a" into tArray2[1]
	union tArray2 with tFixedArray
	TestAssert "union: left array, right array, same key", tArray2[1] is "a"
	
	local tArray3
	put "c" into tArray3[2]
	union tArray3 with tFixedArray
	TestAssert "union: left array, right array, existing key", tArray3[2] is "c"
	TestAssert "union: left array, right array, additional key", tArray3[1] is "b"
end TestArrayUnion

on TestArrayUnionInto
	local tLeft, tRight, tOut
	put "a" into tLeft[1]
	put "b" into tRight[2]
	union tLeft with tRight into tOut
	TestAssert "union into: left array unchanged",  \
			the number of elements in tLeft is 1 and \
			tLeft[1] is "a"
	TestAssert "union into: right array unchanged",  \
			the number of elements in tRight is 1 and \
			tRight[2] is "b"
	TestAssert "union into: out correct",  \
			the number of elements in tOut is 2 and \
			tOut[1] is "a" and \
			tOut[2] is "b"
end TestArrayUnionInto

on TestNestedArrayNonRecursiveUnion

	local tLeftArray2, tRightArray2
	put "a" into tLeftArray2[1]
	put "b" into tRightArray2[1][1]
		
	union tLeftArray2 with tRightArray2
	TestAssert "non-recursive union: left value non-array, right value array", tLeftArray2[1] is "a"
	
	local tLeftArray4, tRightArray4
	put "a" into tLeftArray4[1][1]
	put "b" into tRightArray4[1][2]
	union tLeftArray4 with tRightArray4
	TestAssert "non-recursive union: left value array, right value array, additional key", tLeftArray4[1][2] is empty
	
end TestNestedArrayNonRecursiveUnion

on TestNestedArrayRecursiveUnion

	local tLeftArray1, tRightArray1
	put "a" into tLeftArray1[1]
	put "b" into tRightArray1[1]
	union tLeftArray1 with tRightArray1 recursively
	TestAssert "recursive union: left value non-array, right value non-array", tLeftArray1[1] is "a"

	local tLeftArray2, tRightArray2
	put "a" into tLeftArray2[1]
	put "b" into tRightArray2[1][1]
		
	union tLeftArray2 with tRightArray2 recursively
	TestAssert "recursive union: left value non-array, right value array", tLeftArray2[1][1] is "b"
	
	local tLeftArray3, tRightArray3
	put "a" into tLeftArray3[1][1]
	put "b" into tRightArray3[1]
	union tLeftArray3 with tRightArray3 recursively
	TestAssert "recursive union: left value array, right value non-array", tLeftArray3[1][1] is "a"
	
	local tLeftArray4, tRightArray4
	put "a" into tLeftArray4[1][1]
	put "b" into tRightArray4[1][2]
	union tLeftArray4 with tRightArray4 recursively
	TestAssert "recursive union: left value array, right value array, existing key", tLeftArray4[1][1] is "a"
	TestAssert "recursive union: left value array, right value array, additional key", tLeftArray4[1][2] is "b"
	
end TestNestedArrayRecursiveUnion

on TestArrayRecursiveUnionInto
	local tLeft, tRight, tOut
	put "a" into tLeft[1][1]
	put "b" into tRight[1][2]
	union tLeft with tRight recursively into tOut
	TestAssert "union into: left array unchanged",  \
			the number of elements in tLeft is 1 and \
			the number of elements in tLeft[1] is 1 and \
			tLeft[1][1] is "a"
	TestAssert "union into: right array unchanged",  \
			the number of elements in tRight is 1 and \
			the number of elements in tRight[1] is 1 and \
			tRight[1][2] is "b"
	TestAssert "union into: out correct",  \
			the number of elements in tOut is 1 and \
			the number of elements in tOut[1] is 2 and \
			tOut[1][1] is "a" and \
			tOut[1][2] is "b"
end TestArrayRecursiveUnionInto

/*
Semantics of array symmetric difference:

function ArraySymmetricDifference(pLeft, pRight)
	repeat for each key tKey in pRight
		if tKey is not among the keys of pLeft then
			put pRight[tKey] into pLeft[tKey]
		else 
			delete variable pLeft[tKey]
		end if
	end repeat
	
	return pLeft
end ArraySymmetricDifference

*/
on TestArraySymmetricDifference
	local tLeftNonArray, tRightNonArray

	put "a" into tLeftNonArray
	put "b" into tRightNonArray
	symmetric difference tLeftNonArray with tRightNonArray
	TestAssert "symmetric difference: left non-array, right non-array", tLeftNonArray is "a"
	
	local tRightArray
	put "a" into tLeftNonArray
	put "b" into tRightArray[1]
	symmetric difference tLeftNonArray with tRightArray
	TestAssert "symmetric difference: left non-array, right array", tLeftNonArray[1] is "b"
	
	local tLeftArray
	put "a" into tLeftArray[1]
	symmetric difference tLeftArray with tRightNonArray
	TestAssert "symmetric difference: left array, right non-array", tLeftArray[1] is "a"
	
	put empty into tLeftArray
	put "a" into tLeftArray[1]
	put empty into tRightArray
	put "b" into tRightArray[1]
	symmetric difference tLeftArray with tRightArray
	TestAssert "symmetric difference: left array, right array, same key", \
			tLeftArray is not an array
	
	put empty into tLeftArray
	put "a" into tLeftArray[1]
	put empty into tRightArray
	put "b" into tRightArray[1]
	put "c" into tRightArray[2]
	symmetric difference tLeftArray with tRightArray
	TestAssert "symmetric difference: left array, right array, right superset of left", \
			"1" is not among the keys of tLeftArray and tLeftArray[2] is "c"

	put empty into tLeftArray
	put "a" into tLeftArray[1]
	put "c" into tLeftArray[2]
	put empty into tRightArray
	put "b" into tRightArray[1]
	symmetric difference tLeftArray with tRightArray
	TestAssert "symmetric difference: left array, right array, right subset of left", \
			"1" is not among the keys of tLeftArray and tLeftArray[2] is "c"

	put empty into tLeftArray
	put "a" into tLeftArray[1]
	put "c" into tLeftArray[2]
	put empty into tRightArray
	put "b" into tRightArray[1]
	put "d" into tRightArray[3]
	symmetric difference tLeftArray with tRightArray
	TestAssert "symmetric difference: left array, right array, neither is subset of other", \
			the number of elements in tLeftArray is 2 and \
			"1" is not among the keys of tLeftArray and \
			tLeftArray[2] is "c" and \
			tLeftArray[3] is "d"
end TestArraySymmetricDifference

on TestArraySymmetricDifferenceInto
	local tLeft, tRight, tOut
	put "a" into tLeft[1]
	put "b" into tRight[2]
	symmetric difference tLeft with tRight into tOut
	TestAssert "symmetric difference into: left array unchanged",  \
			the number of elements in tLeft is 1 and \
			tLeft[1] is "a"
	TestAssert "symmetric difference into: right array unchanged",  \
			the number of elements in tRight is 1 and \
			tRight[2] is "b"
	TestAssert "symmetric difference into: out correct",  \
			the number of elements in tOut is 2 and \
			tOut[1] is "a" and \
			tOut[2] is "b"
end TestArraySymmetricDifferenceInto

/*
Semantics of array intersect:

function ArrayIntersect(pLeft, pRight, pRecursive)
	repeat for each key tKey in pLeft
		if tKey is not among the keys of pRight then
			delete variable pLeft[tKey]
		else if pRecursive then
			put ArrayIntersect(pLeft[tKey], pRight[tKey], true) into pLeft[tKey]
		end if
	end repeat
	
	return pLeft
end ArrayIntersect
*/

on TestArrayIntersect

	local tString1	
	put "a" into tString1
	intersect tString1 with "b"
	TestAssert "intersect: left non-array, right non-array", tString1 is "a"
	
	local tString2, tFixedArray
	put "a" into tString2
	put "b" into tFixedArray[1]
	intersect tString2 with tFixedArray
	TestAssert "intersect: left non-array, right array", tString2 is "a"
	
	local tArray1
	put "a" into tArray1[1]
	intersect tArray1 with "b"
	TestAssert "intersect: left array, right non-array", tArray1 is empty
	
	local tArray2
	put "a" into tArray2[1]
	intersect tArray2 with tFixedArray
	TestAssert "intersect: left array, right array, same key", tArray2[1] is "a"
	
	local tLeftArray3, tRightArray3
	put "a" into tLeftArray3[1]
	put "b" into tLeftArray3[2]
	put "c" into tRightArray3[2]
	put "d" into tRightArray3[3]
	intersect tLeftArray3 with tRightArray3
	TestAssert "intersect: left array, right array, key present in left not right", tLeftArray3[1] is empty	
	TestAssert "intersect: left array, right array, key present in right not left", tLeftArray3[3] is empty
	TestAssert "intersect: left array, right array, key present in left and right", tLeftArray3[2] is "b"

end TestArrayIntersect

on TestArrayIntersectInto
	local tLeft, tRight, tOut
	put "a" into tLeft[1]
	put "c" into tLeft[2]
	put "b" into tRight[2]
	put "d" into tRight[3]
	intersect tLeft with tRight into tOut
	TestAssert "intersect into: left array unchanged",  \
			the number of elements in tLeft is 2 and \
			tLeft[1] is "a" and tLeft[2] is "c"
	TestAssert "intersect into: right array unchanged",  \
			the number of elements in tRight is 2 and \
			tRight[2] is "b" and tRight[3] is "d"
	TestAssert "intersect into: out correct",  \
			the number of elements in tOut is 1 and \
			tOut[2] is "c"
end TestArrayIntersectInto

on TestNestedArrayNonRecursiveIntersect

	local tLeftArray2, tRightArray2
	put "a" into tLeftArray2[1]
	put "b" into tRightArray2[1][1]
		
	intersect tLeftArray2 with tRightArray2
	TestAssert "non-recursive intersect: left value non-array, right value array", tLeftArray2[1] is "a"
	
	local tLeftArray4, tRightArray4
	put "a" into tLeftArray4[1][1]
	put "b" into tRightArray4[1][2]
	intersect tLeftArray4 with tRightArray4
	TestAssert "non-recursive intersect: left value array, right value array, additional key", tLeftArray4[1][1] is "a"
	
end TestNestedArrayNonRecursiveIntersect

on TestNestedArrayRecursiveIntersect

	local tLeftArray1, tRightArray1
	put "a" into tLeftArray1[1]
	put "b" into tRightArray1[1]
	intersect tLeftArray1 with tRightArray1 recursively
	TestAssert "recursive intersect: left value non-array, right value non-array", tLeftArray1[1] is "a"

	local tLeftArray2, tRightArray2
	put "a" into tLeftArray2[1]
	put "b" into tRightArray2[1][1]
		
	intersect tLeftArray2 with tRightArray2 recursively
	TestAssert "recursive intersect: left value non-array, right value array", tLeftArray2[1] is "a"
	
	local tLeftArray3, tRightArray3
	put "a" into tLeftArray3[1][1]
	put "b" into tRightArray3[1]
	intersect tLeftArray3 with tRightArray3 recursively
	TestAssert "recursive intersect: left value array, right value non-array", tLeftArray3[1] is empty
	
	local tLeftArray4, tRightArray4
	put "a" into tLeftArray4[1][1]
	put "b" into tLeftArray4[1][2]
	put "c" into tRightArray4[1][2]
	put "d" into tRightArray4[1][3]
	intersect tLeftArray4 with tRightArray4 recursively
	TestAssert "recursive intersect: left value array, right value array, key present in left not right", tLeftArray4[1][1] is empty	
	TestAssert "recursive intersect: left value array, right value array, key present in right not left", tLeftArray4[1][3] is empty
	TestAssert "recursive intersect: left value array, right value array, key present in left and right", tLeftArray4[1][2] is "b"
	
end TestNestedArrayRecursiveIntersect

on TestArrayRecursiveIntersectInto
	local tLeft, tRight, tOut
	put "a" into tLeft[1][1]
	put "c" into tLeft[1][2]
	put "b" into tRight[1][2]
	put "d" into tRight[1][3]
	intersect tLeft with tRight recursively into tOut
	TestAssert "intersect into: left array unchanged",  \
			the number of elements in tLeft is 1 and \
			the number of elements in tLeft[1] is 2 and \
			tLeft[1][1] is "a" and tLeft[1][2] is "c"
	TestAssert "intersect into: right array unchanged",  \
			the number of elements in tRight is 1 and \
			the number of elements in tRight[1] is 2 and \
			tRight[1][2] is "b" and tRight[1][3] is "d"
	TestAssert "intersect into: out correct",  \
			the number of elements in tOut is 1 and \
			the number of elements in tOut[1] is 1 and \
			tOut[1][2] is "c"
end TestArrayRecursiveIntersectInto

/*
Semantics of array difference:

function ArrayDifference(pLeft, pRight)
	repeat for each key tKey in pLeft
		if tKey is among the keys of pRight then
			delete variable pLeft[tKey]
		end if
	end repeat
	
	return pLeft
end ArrayDifference

*/
on TestArrayDifference
	local tLeftNonArray, tRightNonArray

	put "a" into tLeftNonArray
	put "b" into tRightNonArray
	difference tLeftNonArray with tRightNonArray
	TestAssert "difference: left non-array, right non-array", tLeftNonArray is "a"
	
	local tRightArray
	put "a" into tLeftNonArray
	put "b" into tRightArray[1]
	difference tLeftNonArray with tRightArray
	TestAssert "difference: left non-array, right array", tLeftNonArray is "a"
	
	local tLeftArray
	put "a" into tLeftArray[1]
	difference tLeftArray with tRightNonArray
	TestAssert "difference: left array, right non-array", tLeftArray[1] is "a"
	
	put empty into tLeftArray
	put "a" into tLeftArray[1]
	put empty into tRightArray
	put "b" into tRightArray[1]
	difference tLeftArray with tRightArray
	TestAssert "difference: left array, right array, same key", \
			tLeftArray is not an array
	
	put empty into tLeftArray
	put "a" into tLeftArray[1]
	put empty into tRightArray
	put "b" into tRightArray[1]
	put "c" into tRightArray[2]
	difference tLeftArray with tRightArray
	TestAssert "difference: left array, right array, right superset of left", \
			tLeftArray is not an array

	put empty into tLeftArray
	put "a" into tLeftArray[1]
	put "c" into tLeftArray[2]
	put empty into tRightArray
	put "b" into tRightArray[1]
	difference tLeftArray with tRightArray
	TestAssert "difference: left array, right array, right subset of left", \
			the number of elements in tLeftArray is 1 and \
			tLeftArray[2] is "c"

	put empty into tLeftArray
	put "a" into tLeftArray[1]
	put "c" into tLeftArray[2]
	put empty into tRightArray
	put "b" into tRightArray[1]
	put "d" into tRightArray[3]
	difference tLeftArray with tRightArray
	TestAssert "difference: left array, right array, neither is subset of other", \
			the number of elements in tLeftArray is 1 and \
			tLeftArray[2] is "c"
end TestArrayDifference

on TestArrayDifferenceInto
	local tLeft, tRight, tOut
	put "a" into tLeft[1]
	put "c" into tLeft[2]
	put "b" into tRight[2]
	put "d" into tRight[3]
	difference tLeft with tRight into tOut
	TestAssert "difference into: left array unchanged",  \
			the number of elements in tLeft is 2 and \
			tLeft[1] is "a" and tLeft[2] is "c"
	TestAssert "difference into: right array unchanged",  \
			the number of elements in tRight is 2 and \
			tRight[2] is "b" and tRIght[3] is "d"
	TestAssert "difference into: out correct",  \
			the number of elements in tOut is 1 and \
			tOut[1] is "a"
end TestArrayDifferenceInto
