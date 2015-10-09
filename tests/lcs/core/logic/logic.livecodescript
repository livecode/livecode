script "CoreLogic"
/*
Copyright (C) 2015 Runtime Revolution Ltd.

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

on TestLogic1

TestAssert "test", (false and false) is false
TestAssert "test", (false and true) is false
TestAssert "test", (true and false) is false 
TestAssert "test", (true and true) is true

repeat for each item tbool1 in "true,false"
	repeat for each item tbool2 in "true,false"
		TestAssert "test", (tbool1 and tbool2) is (tbool2 and tbool1)
	end repeat
end repeat
end TestLogic1

on TestLogic2


TestAssert "test", (true is a boolean)
TestAssert "test", not (true is not a boolean)

TestAssert "test", (false is a boolean)
TestAssert "test", "true" is a boolean
TestAssert "test", "false" is a boolean

TestAssert "test", "False" is a boolean
TestAssert "test", "FALSE" is a boolean
TestAssert "test", "fAlSe" is a boolean
TestAssert "test", "tRuE" is a boolean

TestAssert "test", "abc" is not a boolean
TestAssert "test", 1 is not a boolean
TestAssert "test", 1.23 is not a boolean

TestAssert "test", empty is not a boolean

end TestLogic2
on TestLogicArrays


-- Equal case
local tLeftArray, tRightArray
put 10 into tLeftArray["1"]
put "foo" into tLeftArray["bar"]
put 10 into tRightArray["1"]
put "foo" into tRightArray["bar"]
TestAssert "test", tLeftArray is tRightArray
TestAssert "test", tLeftArray = tRightArray

-- Left bigger than right
put true into tLeftArray["baz"]
TestAssert "test", not (tLeftArray is tRightArray)
TestAssert "test", not (tLeftArray = tRightArray)

-- Right bigger than left
delete variable tLeftArray["baz"]
put true into tRightArray["baz"]
TestAssert "test", not (tLeftArray is tRightArray)
TestAssert "test", not (tLeftArray = tRightArray)

-- Unequal case
delete variable tRightArray["baz"]
put 100 into tRightArray["1"]
TestAssert "test", not (tLeftArray is tRightArray)
TestAssert "test", not (tLeftArray = tRightArray)

end TestLogicArrays
on TestLogicNumbers

TestAssert "test", 100 is "  100"
TestAssert "test", 100 = "  100"
TestAssert "test", not (100 is "  1001")
TestAssert "test", not (100 = "  1001")
TestAssert "test", "abc" is "abc"
TestAssert "test", "abc" = "abc"
TestAssert "test", not ("abc" is "abcd")
TestAssert "test", not ("abc" = "abcd")
TestAssert "test", 100 is 100
TestAssert "test", 100 = 100
TestAssert "test", not (100 is 1000)
TestAssert "test", not (100 = 1000)

end TestLogicNumbers
on TestLogicBooleans

TestAssert "test", (0 is 0) is (1 is 1)
TestAssert "test", (0 is 0) = (1 is 1)
TestAssert "test", not ((0 is 0) is (1 is 2))
TestAssert "test", not ((0 is 0) = (1 is 2))

end TestLogicBooleans
on TestLogicMisc

local tLeft
put 100 into tLeft[1]
TestAssert "test", not (tLeft is empty)
TestAssert "test", not (tLeft = empty)
TestAssert "test", not (empty is tLeft)
TestAssert "test", not (empty = tLeft)

end TestLogicMisc
on TestLogic3


TestAssert "test", 10 > 1
TestAssert "test", not (10 > 10)
TestAssert "test", not (1 > 10)

TestAssert "test", "c" > "b"
TestAssert "test", not ("c" > "c")
TestAssert "test", not ("b" > "c")

TestAssert "test", "   0x100" > "   1"
TestAssert "test", not ("   0x100" > "   0x100")
TestAssert "test", not ("   1" > "   0x100")

local tLeftArray, tRightArray
put 100 into tLeftArray[1]
put 100 into tRightArray[1]
TestAssert "test", not (tLeftArray > tRightArray)
TestAssert "test", not (tRightArray > tLeftArray)

end TestLogic3
on TestLogic4


TestAssert "test", 10 >= 1
TestAssert "test", 10 >= 10
TestAssert "test", not (1 >= 10)

TestAssert "test", "c" > "b"
TestAssert "test", "c" >= "c"
TestAssert "test", not ("b" > "c")

TestAssert "test", "   0x100" > "   1"
TestAssert "test", "   0x100" >= "   0x100"
TestAssert "test", not ("   1" > "   0x100")

local tLeftArray, tRightArray
put 100 into tLeftArray[1]
put 100 into tRightArray[1]
TestAssert "test", tLeftArray >= tRightArray
TestAssert "test", tRightArray >= tLeftArray

end TestLogic4
on TestLogic5


TestAssert "test", 1 < 10
TestAssert "test", not (1 < 1)
TestAssert "test", not (10 < 1)

TestAssert "test", "b" < "c"
TestAssert "test", not ("b" < "b")
TestAssert "test", not ("c" < "b")

TestAssert "test", "   1" < "   0x100"
TestAssert "test", not ("   0x100" < "   0x100")
TestAssert "test", not ("   0x100" < "   1")

local tLeftArray, tRightArray
put 100 into tLeftArray[1]
put 100 into tRightArray[1]
TestAssert "test", not (tLeftArray < tRightArray)
TestAssert "test", not (tRightArray < tLeftArray)

end TestLogic5
on TestLogic6


TestAssert "test", 1 <= 10
TestAssert "test", 10 <= 10
TestAssert "test", not (10 <= 1)

TestAssert "test", "b" <= "c"
TestAssert "test", "b" <= "b"
TestAssert "test", not ("c" <= "b")

TestAssert "test", "   1" <= "   0x100"
TestAssert "test", "   0x100" <= "   0x100"
TestAssert "test", not ("   0x100" <= "   1")

local tLeftArray, tRightArray
put 100 into tLeftArray[1]
put 100 into tRightArray[1]
TestAssert "test", tLeftArray <= tRightArray
TestAssert "test", tRightArray <= tLeftArray

end TestLogic6
on TestLogicNumbers2

TestAssert "test", not (100 is not "  100")
TestAssert "test", not (100 <> "  100")
TestAssert "test", 100 is not "  1001"
TestAssert "test", 100 <> "  1001"
TestAssert "test", not ("abc" is not "abc")
TestAssert "test", not ("abc" <> "abc")
TestAssert "test", "abc" is not "abcd"
TestAssert "test", "abc" <> "abcd"
TestAssert "test", not (100 is not 100)
TestAssert "test", not (100 <> 100)
TestAssert "test", 100 is not 1000
TestAssert "test", 100 <> 1000

end TestLogicNumbers2
on TestLogicBooleans2

TestAssert "test", not ((0 is 0) is not (1 is 1))
TestAssert "test", not ((0 is 0) <> (1 is 1))
TestAssert "test", (0 is 0) is not (1 is 2)
TestAssert "test", (0 is 0) <> (1 is 2)

end TestLogicBooleans2

on TestLogic7


TestAssert "test", (not false) is true
TestAssert "test", (not true) is false

repeat for each item tbool in "true,false"
	TestAssert "test", (not (not tbool) ) is tbool
end repeat

end TestLogic7
on TestLogic8


TestAssert "test", (false or false) is false
TestAssert "test", (false or true) is true
TestAssert "test", (true or false) is true
TestAssert "test", (true or true) is true

repeat for each item tbool1 in "true,false"
	repeat for each item tbool2 in "true,false"
		TestAssert "test", (tbool1 or tbool2) is (tbool2 or tbool1)
	end repeat
end repeat

end TestLogic8
