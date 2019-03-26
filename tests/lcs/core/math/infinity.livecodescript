script "CoreMathInfinity"
/*
Copyright (C) 2018 LiveCode Ltd.

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

command _TestInfinityDivOperator
    get inf div 1 is inf
end _TestInfinityDivOperator

on TestInfinityDivOperator
    TestAssertBrokenThrow "Infinity is unchanged by div operator", "_TestInfinityDivOperator", "Bug 14316", \
        the long id of me, "EE_DIV_RANGE"
end TestInfinityDivOperator

on TestInfinitySubtractOperator
    TestAssert "Infinity is unchanged by subtract operator", inf - 1 is inf
end TestInfinitySubtractOperator

on TestInfinityOverOperator
   TestAssert "Infinity is unchanged by over operator", inf / 2 is inf
end TestInfinityOverOperator

on TestInfinityAddOperator
   TestAssert "Infinity is unchanged by add operator", inf + 1 is inf
end TestInfinityAddOperator

command _TestInfinityMultiplyOperator
    get inf * 2 is inf
end _TestInfinityMultiplyOperator

on TestInfinityMultiplyOperator
    TestAssertBrokenThrow "Infinity is unchanged by multiply operator", "_TestInfinityMultiplyOperator", "Bug 14316", \
        the long id of me, "EE_MULTIPLY_RANGE"
end TestInfinityMultiplyOperator

command _TestInfinityPowerOperator
    get inf ^ 2 is inf
end _TestInfinityPowerOperator

on TestInfinityPowerOperator
    TestAssertBrokenThrow "Infinity is unchanged by power operator", "_TestInfinityPowerOperator", "Bug 14316", \
        the long id of me, "EE_POW_RANGE"
end TestInfinityPowerOperator
