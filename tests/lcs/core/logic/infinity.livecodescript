script "CoreLogicInfinity"
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

on TestInfinityComparisons
	TestAssert "strconst(infinity) is strconst(infinity)", "infinity" is "infinity"
	TestAssert "string(infinity) is strconst(infinity)", ("infinity" & empty) is "infinity"
	TestAssert "strconst(infinity) is string(infinity)", "infinity" is ("infinity" & empty)
	TestAssert "strconst(Infinity) is strconst(infinity)", "Infinity" is "infinity"
	TestAssert "string(Infinity) is strconst(infinity)", ("Infinity" & empty) is "infinity"
	TestAssert "strconst(Infinity) is string(infinity)", "Infinity" is ("infinity" & empty)
	TestAssert "strconst(infinity) is strconst(Infinity)", "infinity" is "Infinity"
	TestAssert "string(infinity) is strconst(Infinity)", ("infinity" & empty) is "Infinity"
	TestAssert "strconst(infinity) is string(Infinity)", "infinity" is ("Infinity" & empty)

	TestAssert "strconst(infinity) <= strconst(infinity)", "infinity" <= "infinity"
	TestAssert "string(infinity) <= strconst(infinity)", ("infinity" & empty) <= "infinity"
	TestAssert "strconst(infinity) <= string(infinity)", "infinity" <= ("infinity" & empty)
	TestAssert "strconst(Infinity) <= strconst(infinity)", "Infinity" <= "infinity"
	TestAssert "string(Infinity) <= strconst(infinity)", ("Infinity" & empty) <= "infinity"
	TestAssert "strconst(Infinity) <= string(infinity)", "Infinity" <= ("infinity" & empty)
	TestAssert "strconst(infinity) <= strconst(Infinity)", "infinity" <= "Infinity"
	TestAssert "string(infinity) <= strconst(Infinity)", ("infinity" & empty) <= "Infinity"
	TestAssert "strconst(infinity) <= string(Infinity)", "infinity" <= ("Infinity" & empty)

	TestAssert "strconst(infinity) >= strconst(infinity)", "infinity" >= "infinity"
	TestAssert "string(infinity) >= strconst(infinity)", ("infinity" & empty) >= "infinity"
	TestAssert "strconst(infinity) >= string(infinity)", "infinity" >= ("infinity" & empty)
	TestAssert "strconst(Infinity) >= strconst(infinity)", "Infinity" >= "infinity"
	TestAssert "string(Infinity) >= strconst(infinity)", ("Infinity" & empty) >= "infinity"
	TestAssert "strconst(Infinity) >= string(infinity)", "Infinity" >= ("infinity" & empty)
	TestAssert "strconst(infinity) >= strconst(Infinity)", "infinity" >= "Infinity"
	TestAssert "string(infinity) >= strconst(Infinity)", ("infinity" & empty) >= "Infinity"
	TestAssert "strconst(infinity) >= string(Infinity)", "infinity" >= ("Infinity" & empty)
end TestInfinityComparisons
