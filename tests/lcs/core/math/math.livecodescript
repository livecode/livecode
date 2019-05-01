script "CoreMath"
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

on TestMath1


TestAssert "test", abs(12.3) is 12.3
TestAssert "test", abs(-12.3) is 12.3

end TestMath1
on TestMath2



TestAssert "test", acos(-1) is pi
TestAssert "test", acos(-0.5) is 2 * pi / 3
TestAssert "test", acos(0) is pi / 2
TestAssert "test", acos(0.5) is pi / 3
TestAssert "test", acos(1) is 0

//repeat with i = -1 to 1 step 0.01
//	TestAssert "test", cos(acos(i)) is i
//end repeat

end TestMath2
on TestMathNumberPlusNumber


TestAssert "test", (7 + 3) is 10
TestAssert "test", (8 + 3) is 11
TestAssert "test", (9 + 3) is 12

TestAssert "test", (-7 + 3) is -4
TestAssert "test", (-8 + 3) is -5
TestAssert "test", (-9 + 3) is -6

TestAssert "test", (7 + -3) is 4
TestAssert "test", (8 + -3) is 5
TestAssert "test", (9 + -3) is 6

TestAssert "test", (-7 + -3) is -10
TestAssert "test", (-8 + -3) is -11
TestAssert "test", (-9 + -3) is -12

end TestMathNumberPlusNumber
on TestMathArrayPlusNumber


local tA1, tA2

repeat with i = 1 to 10
	put i into tA1[i]
end repeat

put tA1 + 3 into tA2

repeat with i = 1 to 10
	TestAssert "test", tA2[i] is (tA1[i] + 3)
end repeat

// empty array entry is 0
put empty into tA1
put empty into tA1[0]
put tA1 + 3 into tA2
TestAssert "test", tA2[0] is 3

end TestMathArrayPlusNumber
on TestMathArrayPlusArray


local tA1, tA2, tA3

repeat with i = 1 to 10
	put i into tA1[i]
	put (11 - i) into tA2[i]
end repeat

put tA1 + tA2 into tA3

repeat with i = 1 to 10
	TestAssert "test", tA3[i] is (tA1[i] + tA2[i])
end repeat

end TestMathArrayPlusArray

on TestAnnuity
	TestAssert "test annuity formula", annuity(0.02, 25) is ( 1 - ( 1 + 0.02 ) ^ ( - 25 ) ) / 0.02
	TestAssert "test annuity zero rate", annuity(0, 25) is 25
end TestAnnuity

on TestCompound
	TestAssert "test compound formula", compound(0.02, 25) is (1 + 0.02) ^ 25
end TestCompound

on TestMath4



TestAssert "test", asin(-1) is -(pi) / 2
TestAssert "test", asin(-0.5) is -(pi) / 6
TestAssert "test", asin(0) is 0
TestAssert "test", asin(0.5) is pi / 6
TestAssert "test", asin(1) is pi / 2

//repeat with i = -1 to 1 step 0.01
//	TestAssert "test", sin(asin(i)) is i
//end repeat

end TestMath4
on TestMath5


TestAssert "test", atan2( 0,  1) is 0
TestAssert "test", atan2( 1,  1) is pi / 4
TestAssert "test", atan2( 1,  0) is pi / 2
TestAssert "test", atan2( 1, -1) is 3 * pi / 4
TestAssert "test", atan2( 0, -1) is pi
TestAssert "test", atan2(-1, -1) is -3 * pi / 4
TestAssert "test", atan2(-1,  0) is -pi / 2
TestAssert "test", atan2(-1,  1) is -pi / 4

end TestMath5

on TestMath7

TestAssert "test", baseConvert("10", 10, 10) is "10"
TestAssert "test", baseConvert("10", 16, 10) is "16"
TestAssert "test", baseConvert("10", 10, 16) is "A"

end TestMath7
on TestMath8


TestAssert "test", (baseConvert("00000000", 16, 10) bitAnd baseConvert("0F3C5A69", 16, 10)) is baseConvert("00000000", 16, 10)
TestAssert "test", (baseConvert("FFFFFFFF", 16, 10) bitAnd baseConvert("0F3C5A69", 16, 10)) is baseConvert("0F3C5A69", 16, 10)
TestAssert "test", (baseConvert("33333333", 16, 10) bitAnd baseConvert("0F3C5A69", 16, 10)) is baseConvert("03301221", 16, 10)
TestAssert "test", (baseConvert("CCCCCCCC", 16, 10) bitAnd baseConvert("0F3C5A69", 16, 10)) is baseConvert("0C0C4848", 16, 10)
TestAssert "test", (baseConvert("55555555", 16, 10) bitAnd baseConvert("0F3C5A69", 16, 10)) is baseConvert("05145041", 16, 10)
TestAssert "test", (baseConvert("AAAAAAAA", 16, 10) bitAnd baseConvert("0F3C5A69", 16, 10)) is baseConvert("0A280A28", 16, 10)
TestAssert "test", (baseConvert("66666666", 16, 10) bitAnd baseConvert("0F3C5A69", 16, 10)) is baseConvert("06244260", 16, 10)
TestAssert "test", (baseConvert("99999999", 16, 10) bitAnd baseConvert("0F3C5A69", 16, 10)) is baseConvert("09181809", 16, 10)

end TestMath8
on TestMath9


TestAssert "test", bitNot baseConvert("0F3C5A69", 16, 10) is baseConvert("F0C3A596", 16, 10)

end TestMath9
on TestMath10


TestAssert "test", (baseConvert("00000000", 16, 10) bitOr baseConvert("0F3C5A69", 16, 10)) is baseConvert("0F3C5A69", 16, 10)
TestAssert "test", (baseConvert("FFFFFFFF", 16, 10) bitOr baseConvert("0F3C5A69", 16, 10)) is baseConvert("FFFFFFFF", 16, 10)
TestAssert "test", (baseConvert("33333333", 16, 10) bitOr baseConvert("0F3C5A69", 16, 10)) is baseConvert("3F3F7B7B", 16, 10)
TestAssert "test", (baseConvert("CCCCCCCC", 16, 10) bitOr baseConvert("0F3C5A69", 16, 10)) is baseConvert("CFFCDEED", 16, 10)
TestAssert "test", (baseConvert("55555555", 16, 10) bitOr baseConvert("0F3C5A69", 16, 10)) is baseConvert("5F7D5F7D", 16, 10)
TestAssert "test", (baseConvert("AAAAAAAA", 16, 10) bitOr baseConvert("0F3C5A69", 16, 10)) is baseConvert("AFBEFAEB", 16, 10)
TestAssert "test", (baseConvert("66666666", 16, 10) bitOr baseConvert("0F3C5A69", 16, 10)) is baseConvert("6F7E7E6F", 16, 10)
TestAssert "test", (baseConvert("99999999", 16, 10) bitOr baseConvert("0F3C5A69", 16, 10)) is baseConvert("9FBDDBF9", 16, 10)

end TestMath10
on TestMath11


TestAssert "test", (baseConvert("00000000", 16, 10) bitXor baseConvert("0F3C5A69", 16, 10)) is baseConvert("0F3C5A69", 16, 10)
TestAssert "test", (baseConvert("FFFFFFFF", 16, 10) bitXor baseConvert("0F3C5A69", 16, 10)) is baseConvert("F0C3A596", 16, 10)
TestAssert "test", (baseConvert("33333333", 16, 10) bitXor baseConvert("0F3C5A69", 16, 10)) is baseConvert("3C0F695A", 16, 10)
TestAssert "test", (baseConvert("CCCCCCCC", 16, 10) bitXor baseConvert("0F3C5A69", 16, 10)) is baseConvert("C3F096A5", 16, 10)
TestAssert "test", (baseConvert("55555555", 16, 10) bitXor baseConvert("0F3C5A69", 16, 10)) is baseConvert("5A690F3C", 16, 10)
TestAssert "test", (baseConvert("AAAAAAAA", 16, 10) bitXor baseConvert("0F3C5A69", 16, 10)) is baseConvert("A596F0C3", 16, 10)
TestAssert "test", (baseConvert("66666666", 16, 10) bitXor baseConvert("0F3C5A69", 16, 10)) is baseConvert("695A3C0F", 16, 10)
TestAssert "test", (baseConvert("99999999", 16, 10) bitXor baseConvert("0F3C5A69", 16, 10)) is baseConvert("96A5C3F0", 16, 10)

end TestMath11
on TestMath12


TestAssert "test", compound(0.5, 20) is ((1 + 0.5) ^ 20)

end TestMath12
on TestMath13


TestAssert "test", cos(pi) is -1
TestAssert "test", cos(2 * pi / 3) is -0.5
TestAssert "test", cos(pi / 2) is 0
TestAssert "test", cos(pi / 3) is 0.5
TestAssert "test", cos(0) is 1

//repeat with i = 0 to 90
//	TestAssert "test", acos(cos(i / (2*pi))) is i / (2*pi)
//end repeat

end TestMath13
on TestMathNumberDivNumber


TestAssert "test", (7 div 3) is 2
TestAssert "test", (8 div 3) is 2
TestAssert "test", (9 div 3) is 3

TestAssert "test", (-7 div 3) is -2
TestAssert "test", (-8 div 3) is -2
TestAssert "test", (-9 div 3) is -3

TestAssert "test", (7 div -3) is -2
TestAssert "test", (8 div -3) is -2
TestAssert "test", (9 div -3) is -3

TestAssert "test", (-7 div -3) is 2
TestAssert "test", (-8 div -3) is 2
TestAssert "test", (-9 div -3) is 3

end TestMathNumberDivNumber
on TestMathArrayDivNumber

local tA1, tA2

repeat with i = 1 to 10
	put i into tA1[i]
end repeat

put tA1 div 3 into tA2

repeat with i = 1 to 10
	TestAssert "test", tA2[i] is (tA1[i] div 3)
end repeat

// empty array entry is 0
put empty into tA1
put empty into tA1[0]
put tA1 div 3 into tA2
TestAssert "test", tA2[0] is 0

end TestMathArrayDivNumber
on TestMathArrayDivArray


local tA1, tA2, tA3

repeat with i = 1 to 10
	put i into tA1[i]
	put (11 - i) into tA2[i]
end repeat

put tA1 div tA2 into tA3

repeat with i = 1 to 10
	TestAssert "test", tA3[i] is (tA1[i] div tA2[i])
end repeat

end TestMathArrayDivArray
on TestMath14


TestAssert "test", exp(0) is 1
TestAssert "test", exp(1) ^ 5 is exp(5)
TestAssert "test", exp(2) ^ 5 is exp(2 * 5)
TestAssert "test", exp(2) * exp(5) is exp(2 + 5)

end TestMath14
on TestMath15


TestAssert "test", exp1(10) is exp(10) - 1

end TestMath15
on TestMath16


local tVal = 1

repeat with i = 0 to 10
	TestAssert "test", exp10(i) is tVal
	multiply tVal by 10
end repeat

end TestMath16
on TestMath17


local tVal = 1

repeat with i = 0 to 10
	TestAssert "test", exp2(i) is tVal
	multiply tVal by 2
end repeat

end TestMath17
on TestMath18


TestAssert "test", 123 is an integer
TestAssert "test", not (123 is not an integer)

TestAssert "test", "123" is an integer
TestAssert "test", "abc" is not an integer

TestAssert "test", 1.2 is not an integer

TestAssert "test", empty is not an integer

end TestMath18
on TestMath19


TestAssert "test", 123 is a number
TestAssert "test", not (123 is not a number)

TestAssert "test", "123" is a number
TestAssert "test", "abc" is not a number

TestAssert "test", 1.2 is a number

TestAssert "test", empty is not a number

end TestMath19
on TestMath20


TestAssert "test", ln(1) is 0

repeat with i = 1 to 10
	TestAssert "test", ln(exp(i)) is i
end repeat

end TestMath20
on TestMath21


repeat with i = 1 to 10
	TestAssert "test", ln1(i) is ln(1 + i)
end repeat

end TestMath21
on TestMath22


local tVal = 1
repeat with i = 0 to 10
	TestAssert "test", log10(tVal) is i
	multiply tVal by 10
end repeat

end TestMath22
on TestMath23


local tVal = 1
repeat with i = 0 to 10
	TestAssert "test", log2(tVal) is i
	multiply tVal by 2
end repeat

end TestMath23
on TestMathSquareMatrices


local tIdentity, tA1, tA2, tA3

put 1 into tIdentity[0,0]
put 0 into tIdentity[0,1]
put 0 into tIdentity[1,0]
put 1 into tIdentity[1,1]

local tVal = 1
repeat with r = 0 to 1
	repeat with c = 0 to 1
		put tVal into tA1[r,c]
		add 1 to tVal
	end repeat
end repeat

repeat with r = 0 to 1
	repeat with c = 0 to 1
		put tVal into tA2[r,c]
		add 1 to tVal
	end repeat
end repeat

TestAssert "test", matrixMultiply(tIdentity, tA1) is tA1
TestAssert "test", matrixMultiply(tA1, tIdentity) is tA1

put matrixMultiply(tA1, tA2) into tA3
TestAssert "test", matrixMultiply(tA2, tA1) is not tA3

TestAssert "test", tA3[0,0] is (tA1[0,0] * tA2[0,0]) + (tA1[0,1] * tA2[1,0])
TestAssert "test", tA3[0,1] is (tA1[0,0] * tA2[0,1]) + (tA1[0,1] * tA2[1,1])
TestAssert "test", tA3[1,0] is (tA1[1,0] * tA2[0,0]) + (tA1[1,1] * tA2[1,0])
TestAssert "test", tA3[1,1] is (tA1[1,0] * tA2[0,1]) + (tA1[1,1] * tA2[1,1])

end TestMathSquareMatrices
on TestMathNonSquareMatrices

local tA1, tA2, tA3

local tVal = 1
repeat with r = 0 to 2
	repeat with c = 0 to 3
		put tVal into tA1[r,c]
		add 1 to tVal
	end repeat
end repeat

repeat with r = 0 to 3
	repeat with c = 0 to 4
		put tVal into tA2[r,c]
		add 1 to tVal
	end repeat
end repeat

put matrixMultiply(tA1, tA2) into tA3
TestAssert "test", tA3 is an array
TestAssert "test", the extents of tA3 is ("0,2" & return & "0,4")

local tTotal
repeat with i = 0 to 2
	repeat with j = 0 to 4
		put 0 into tTotal
		repeat with k = 0 to 3
			add tA1[i,k] * tA2[k, j] to tTotal
		end repeat
		TestAssert "test", tTotal is tA3[i,j]
	end repeat
end repeat
end TestMathNonSquareMatrices

on TestMath24


TestAssert "test", max() is 0
TestAssert "test", max(1.23) is 1.23
TestAssert "test", max("1, 2, 3, 4, 5") is 5
TestAssert "test", max(1, 5, 2, 4, 3) is 5

local tArray
repeat with i = 1 to 5
	put 6 - i into tArray[i]
end repeat
TestAssert "test", max(tArray) is 5

end TestMath24
on TestMath25


TestAssert "test", median() is 0
TestAssert "test", median(1.23) is 1.23
TestAssert "test", median("1, 2, 3, 4, 5") is 3
TestAssert "test", median(1, 5, 2, 4, 3) is 3
TestAssert "test", median(1, 5, 2, 4, 3, 6) is 3.5

local tArray
repeat with i = 1 to 5
	put (( i + 3 ) mod 5) + 1 into tArray[i]
end repeat
TestAssert "test", median(tArray) is 3

end TestMath25
on TestMath26


TestAssert "test", min() is 0
TestAssert "test", min(1.23) is 1.23
TestAssert "test", min("1, 2, 3, 4, 5") is 1
TestAssert "test", min(3, 1, 5, 2, 4) is 1

local tArray
repeat with i = 1 to 5
	put 6 - i into tArray[i]
end repeat
TestAssert "test", min(tArray) is 1

end TestMath26
on TestMathNumberModNumber


TestAssert "test", (7 mod 3) is 1
TestAssert "test", (8 mod 3) is 2
TestAssert "test", (9 mod 3) is 0

TestAssert "test", (-7 mod 3) is -1
TestAssert "test", (-8 mod 3) is -2
TestAssert "test", (-9 mod 3) is 0

TestAssert "test", (7 mod -3) is 1
TestAssert "test", (8 mod -3) is 2
TestAssert "test", (9 mod -3) is 0

TestAssert "test", (-7 mod -3) is -1
TestAssert "test", (-8 mod -3) is -2
TestAssert "test", (-9 mod -3) is 0

end TestMathNumberModNumber
on TestMathArrayModNumber


local tA1, tA2

repeat with i = 1 to 10
	put i into tA1[i]
end repeat

put tA1 mod 3 into tA2

repeat with i = 1 to 10
	TestAssert "test", tA2[i] is (tA1[i] mod 3)
end repeat

// empty array entry is 0
put empty into tA1
put empty into tA1[0]
put tA1 mod 3 into tA2
TestAssert "test", tA2[0] is 0

end TestMathArrayModNumber
on TestMathArrayModArray


local tA1, tA2, tA3

repeat with i = 1 to 10
	put i into tA1[i]
	put (11 - i) into tA2[i]
end repeat

put tA1 mod tA2 into tA3

repeat with i = 1 to 10
	TestAssert "test", tA3[i] is (tA1[i] mod tA2[i])
end repeat

end TestMathArrayModArray
on TestMathNumberTimesNumber


TestAssert "test", (7 * 3) is 21
TestAssert "test", (8 * 3) is 24
TestAssert "test", (9 * 3) is 27

TestAssert "test", (-7 * 3) is -21
TestAssert "test", (-8 * 3) is -24
TestAssert "test", (-9 * 3) is -27

TestAssert "test", (7 * -3) is -21
TestAssert "test", (8 * -3) is -24
TestAssert "test", (9 * -3) is -27

TestAssert "test", (-7 * -3) is 21
TestAssert "test", (-8 * -3) is 24
TestAssert "test", (-9 * -3) is 27

end TestMathNumberTimesNumber
on TestMathArrayTimesNumber


local tA1, tA2

repeat with i = 1 to 10
	put i into tA1[i]
end repeat

put tA1 * 3 into tA2

repeat with i = 1 to 10
	TestAssert "test", tA2[i] is (tA1[i] * 3)
end repeat

// empty array entry is 0
put empty into tA1
put empty into tA1[0]
put tA1 * 3 into tA2
TestAssert "test", tA2[0] is 0

end TestMathArrayTimesNumber
on TestMathArrayTimesArray


local tA1, tA2, tA3

repeat with i = 1 to 10
	put i into tA1[i]
	put (11 - i) into tA2[i]
end repeat

put tA1 * tA2 into tA3

repeat with i = 1 to 10
	TestAssert "test", tA3[i] is (tA1[i] * tA2[i])
end repeat

end TestMathArrayTimesArray
on TestMathNumberDividedByNumber


TestAssert "test", (7 / 2) is 3.5
TestAssert "test", (8 / 2) is 4
TestAssert "test", (9 / 2) is 4.5

TestAssert "test", (-7 / 2) is -3.5
TestAssert "test", (-8 / 2) is -4
TestAssert "test", (-9 / 2) is -4.5

TestAssert "test", (7 / -2) is -3.5
TestAssert "test", (8 / -2) is -4
TestAssert "test", (9 / -2) is -4.5

TestAssert "test", (-7 / -2) is 3.5
TestAssert "test", (-8 / -2) is 4
TestAssert "test", (-9 / -2) is 4.5

end TestMathNumberDividedByNumber
on TestMathArrayDividedByNumber


local tA1, tA2

repeat with i = 1 to 10
	put i into tA1[i]
end repeat

put tA1 / 3 into tA2

repeat with i = 1 to 10
	TestAssert "test", tA2[i] is (tA1[i] / 3)
end repeat

// empty array entry is 0
put empty into tA1
put empty into tA1[0]
put tA1 / 3 into tA2
TestAssert "test", tA2[0] is 0

end TestMathArrayDividedByNumber
on TestMathArrayDividedByArray


local tA1, tA2, tA3

repeat with i = 1 to 10
	put i into tA1[i]
	put (11 - i) into tA2[i]
end repeat

put tA1 / tA2 into tA3

repeat with i = 1 to 10
	TestAssert "test", tA3[i] is (tA1[i] / tA2[i])
end repeat

end TestMathArrayDividedByArray
on TestMath27


TestAssert "test", 5 ^ 2 is 25
TestAssert "test", 0.5 ^ 3 is 0.125

TestAssert "test", 4 ^ 0.5 is 2

repeat with i = 1 to 100
	TestAssert "test", i ^ 0 is 1
end repeat

TestAssert "test", 2 ^ -1 is 0.5

repeat with i = 1 to 100
	TestAssert "test", i ^ -1 is 1 / i
end repeat

end TestMath27
on TestMath28


TestAssert "test", random(1) is 1

local tRand
repeat with i = 1 to 100
	put random(10) into tRand
	TestAssert "test", tRand >= 1 and tRand <= 10
end repeat

end TestMath28
on TestMath30


TestAssert "test", sin(pi / 2) is 1
TestAssert "test", sin(pi / 6) is 0.5
TestAssert "test", sin(0) is 0
TestAssert "test", sin(-pi / 6) is -0.5
TestAssert "test", sin(-pi / 2) is -1

//repeat with i = -45 to 45
//	TestAssert "test", asin(sin(i / (2*pi))) is i / (2*pi)
//end repeat

end TestMath30
on TestMath31


TestAssert "test", sqrt(0.25) is 0.5

repeat with i = 1 to 10 step 0.1
	TestAssert "test", sqrt(i) ^ 2 is i
end repeat

end TestMath31

on TestMathNumberMinusNumber


TestAssert "test", (7 - 3) is 4
TestAssert "test", (8 - 3) is 5
TestAssert "test", (9 - 3) is 6

TestAssert "test", (-7 - 3) is -10
TestAssert "test", (-8 - 3) is -11
TestAssert "test", (-9 - 3) is -12

TestAssert "test", (7 - -3) is 10
TestAssert "test", (8 - -3) is 11
TestAssert "test", (9 - -3) is 12

TestAssert "test", (-7 - -3) is -4
TestAssert "test", (-8 - -3) is -5
TestAssert "test", (-9 - -3) is -6

end TestMathNumberMinusNumber
on TestMathArrayMinusNumber


local tA1, tA2

repeat with i = 1 to 10
	put i into tA1[i]
end repeat

put tA1 - 3 into tA2

repeat with i = 1 to 10
	TestAssert "test", tA2[i] is (tA1[i] - 3)
end repeat

// empty array entry is 0
put empty into tA1
put empty into tA1[0]
put tA1 - 3 into tA2
TestAssert "test", tA2[0] is -3

end TestMathArrayMinusNumber
on TestMathArrayMinusArray


local tA1, tA2, tA3

repeat with i = 1 to 10
	put i into tA1[i]
	put (11 - i) into tA2[i]
end repeat

put tA1 - tA2 into tA3

repeat with i = 1 to 10
	TestAssert "test", tA3[i] is (tA1[i] - tA2[i])
end repeat

end TestMathArrayMinusArray
on TestMath33


TestAssert "test", sum() is 0
TestAssert "test", sum(1.23) is 1.23
TestAssert "test", sum("1, 2, 3, 4, 5") is 15
TestAssert "test", sum(3, 1, 5, 2, 4) is 15

local tArray
repeat with i = 1 to 5
	put 6 - i into tArray[i]
end repeat
TestAssert "test", sum(tArray) is 15

end TestMath33
on TestMath34


TestAssert "test", tan(pi / 4) is 1
TestAssert "test", tan(0) is 0
TestAssert "test", tan(-pi / 4) is -1

//repeat with i = -44 to 44
//	TestAssert "test", atan(tan(i / (2*pi))) is i / (2*pi)
//end repeat

end TestMath34
on TestMathTransposeMatrix


local tA1, tA2, tA3

local tVal = 1
repeat with r = 0 to 2
	repeat with c = 0 to 4
		put tVal into tA1[r,c]
		add 1 to tVal
	end repeat
end repeat

put transpose(tA1) into tA2
TestAssert "test", the extents of tA2 is ("0,4" & return & "0,2")

repeat with r = 0 to 2
	repeat with c = 0 to 4
		TestAssert "test", tA1[r,c] is tA2[c,r]
	end repeat
end repeat

end TestMathTransposeMatrix
on TestMath35


TestAssert "test", trunc(1.0001) is 1
TestAssert "test", trunc(2.5) is 2
TestAssert "test", trunc(3.5001) is 3

TestAssert "test", trunc(-1.0001) is -1
TestAssert "test", trunc(-2.5) is -2
TestAssert "test", trunc(-3.5001) is -3

end TestMath35
on TestMathNumberWrapNumber


repeat with i = 1 to 10
	repeat with j = 1 to 20
		TestAssert "test", i wrap j is ((i - 1) mod j) + 1
		TestAssert "test", i wrap -j is i wrap j
		TestAssert "test", -i wrap j is -(i wrap j)
		TestAssert "test", -i wrap -j is -(i wrap j)
	end repeat
end repeat

end TestMathNumberWrapNumber
on TestMathArrayWrapNumber


local tA1, tA2

repeat with i = 1 to 10
	put i into tA1[i]
end repeat

put tA1 wrap 3 into tA2

repeat with i = 1 to 10
	TestAssert "test", tA2[i] is (tA1[i] wrap 3)
end repeat

// empty array entry is 0
put empty into tA1
put empty into tA1[0] 
put tA1 wrap 3 into tA2
TestAssert "test", tA2[0] is 0

end TestMathArrayWrapNumber
on TestMathArrayWrapArray


local tA1, tA2, tA3

repeat with i = 1 to 10
	put i into tA1[i]
	put (11 - i) into tA2[i]
end repeat

put tA1 wrap tA2 into tA3

repeat with i = 1 to 10
	TestAssert "test", tA3[i] is (tA1[i] wrap tA2[i])
end repeat

end TestMathArrayWrapArray

on _RandomMaxLimitTestThrows

	local tX, tN2

	put 2^53 into tX
	
	-- this loop computes the next representable 
	-- double after 2^53
	repeat with i = 1 to 128
  		if tX + i - tX is not 0 then
     		exit repeat
  		end if
	end repeat
	
	put tX+i into tX
	
	local rN1

	put random(tN1) into rN1
	
end _RandomMaxLimitTestThrows



on _RandomMaxLimitTestNoThrow

	local tN1, tN2

	put 2^53 into tN1

	put random(tN1) into rN1

end _RandomMaxLimitTestNoThrow

on TestRandomGenLimits

	TestAssertThrow "checks if bad input to random throws exception" , "_RandomMaxLimitTestThrows" , \
		the long id of me , "EE_RANDOM_BADSOURCE"

	TestAssertDoesNotThrow "checks if correct input to random doesn't throw exception" \ 
		, "_RandomMaxLimitTestNoThrow" , the long id of me

end TestRandomGenLimits
