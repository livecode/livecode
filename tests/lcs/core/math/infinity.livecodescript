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

on TestInfinityDivOperator
    TestAssert "Infinity is unchanged by div operator", inf div 2 is inf
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

on TestInfinityMultiplyOperator
    TestAssert "Infinity is unchanged by multiply operator", inf * 2 is inf
end TestInfinityMultiplyOperator

on TestInfinityPowerOperator
    TestAssert "Infinity is unchanged by powwe operator", inf ^ 2 is inf
end TestInfinityPowerOperator
