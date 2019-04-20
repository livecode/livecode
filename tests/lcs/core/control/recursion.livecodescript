script "CoreControlRecursion"
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

on TestSelfRecursion
	local tA, tB

	put "a" into tA
	put "b" into tB
	TestAssert "non-ref even self recursion worked", \
					_DoSelfRecursion(10, tA, tB) is tA and \
						tA is "a" and \
						tB is "b"

	put "a" into tA
	put "b" into tB
	TestAssert "non-ref odd self recursion worked", \
					_DoSelfRecursion(11, tA, tB) is tB and \
						tA is "a" and \
						tB is "b"

	put "a" into tA
	put "b" into tB
	TestAssert "ref even self recursion worked", \
					_DoSelfRecursionRefs(10, tA, tB) is tA and \
						tA is "a" and \
						tB is "b"
						
	put "a" into tA
	put "b" into tB
	TestAssert "ref odd self recursion worked", \
					_DoSelfRecursionRefs(11, tA, tB) is tA and \
						tA is "b" and \
						tB is "a"

	put "a" into tA
	put "b" into tB
	TestAssert "non-ref private even self recursion worked", \
					_DoSelfRecursion_Private(10, tA, tB) is tA and \
						tA is "a" and \
						tB is "b"

	put "a" into tA
	put "b" into tB
	TestAssert "non-ref private odd self recursion worked", \
					_DoSelfRecursion_Private(11, tA, tB) is tB and \
						tA is "a" and \
						tB is "b"

	put "a" into tA
	put "b" into tB
	TestAssert "ref even private self recursion worked", \
					_DoSelfRecursionRefs_Private(10, tA, tB) is tA and \
						tA is "a" and \
						tB is "b"
						
	put "a" into tA
	put "b" into tB
	TestAssert "ref odd private self recursion worked", \
					_DoSelfRecursionRefs_Private(11, tA, tB) is tA and \
						tA is "b" and \
						tB is "a"
end TestSelfRecursion

function _DoSelfRecursion pCount, pA, pB
	local tC
	put pA into tC
	put pB into pA
	put tC into pB
	if pCount is 1 then
		return pA
	end if
	return _DoSelfRecursion(pCount - 1, pA, pB)
end _DoSelfRecursion

function _DoSelfRecursionRefs pCount, @pA, @pB
	local tC
	put pA into tC
	put pB into pA
	put tC into pB
	if pCount is 1 then
		return pA
	end if
	return _DoSelfRecursionRefs(pCount - 1, pA, pB)
end _DoSelfRecursionRefs

private function _DoSelfRecursion_Private pCount, pA, pB
	local tC
	put pA into tC
	put pB into pA
	put tC into pB
	if pCount is 1 then
		return pA
	end if
	return _DoSelfRecursion_Private(pCount - 1, pA, pB)
end _DoSelfRecursion_Private

private function _DoSelfRecursionRefs_Private pCount, @pA, @pB
	local tC
	put pA into tC
	put pB into pA
	put tC into pB
	if pCount is 1 then
		return pA
	end if
	return _DoSelfRecursionRefs_Private(pCount - 1, pA, pB)
end _DoSelfRecursionRefs_Private
