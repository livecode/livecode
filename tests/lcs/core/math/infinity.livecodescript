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

private function StringEqualToInf pNumber, pNegative
    local tInfPlusJunk
    put "inf plus some junk" into tInfPlusJunk
    if pNegative then
        put "-" before tInfPlusJunk
    end if
    return offset(pNumber, tInfPlusJunk) is 1
end StringEqualToInf

command TestInfinityString
    TestAssert "inf is not padded when converted to string", \
            StringEqualToInf("inf"+0, false)

    TestAssert "-inf is not padded when converted to string", \
            StringEqualToInf("-inf"+0, true)
end TestInfinityString

private command _TestBinaryOperator pLeft, pRight, pOperator, pExpected
    local tResult, tExpr
    switch pOperator
    case "plus"
        put pLeft + pRight into tResult
        break
    case "minus"
        put pLeft - pRight into tResult
        break
    case "times"
        put pLeft * pRight into tResult
        break
    case "over"
        put pLeft / pRight into tResult
        break
    case "to the power of"
        put pLeft ^ pRight into tResult
        break
    case "added to"
        add pLeft to pRight
        put pRight into tResult
        break
    case "subtracted from"
        subtract pLeft from pRight
        put pRight into tResult
        break
    case "multiplied by"
        multiply pLeft by pRight
        put pLeft into tResult
        break
    case "divided by"
        divide pLeft by pRight
        put pLeft into tResult
        break
    case "div"
        put pLeft div pRight into tResult
        break
    case "mod"
        put pLeft mod pRight into tResult
        break
    case "wrap"
        put pLeft wrap pRight into tResult
        break
    default
        put pOperator & "(" & pLeft & "," & pRight & ")" into tExpr
        put value(tExpr) into tResult
        break
    end switch
    if tExpr is empty then
        TestAssert "Result of" && pLeft && pOperator && pRight && "is" && pExpected, \
                tResult is pExpected
    else
        TestAssert "Result of" && tExpr && "is" && pExpected, \
                tResult is pExpected
    end if
end _TestBinaryOperator

on TestInfinityBinaryOperators
    _TestBinaryOperator infinity, 1, "plus", infinity
    _TestBinaryOperator -infinity, 1, "plus", -infinity

    _TestBinaryOperator infinity, 1, "minus", infinity
    _TestBinaryOperator -infinity, 1, "minus", -infinity
    _TestBinaryOperator 1, infinity, "minus", -infinity
    _TestBinaryOperator 1, -infinity, "minus", infinity

    _TestBinaryOperator infinity, 2, "times", infinity
    _TestBinaryOperator -infinity, 2, "times", -infinity

    _TestBinaryOperator infinity, 2, "over", infinity
    _TestBinaryOperator -infinity, 2, "over", -infinity
    _TestBinaryOperator 2, infinity, "over", 0
    _TestBinaryOperator 2, -infinity, "over", 0

    _TestBinaryOperator infinity, 2, "to the power of", infinity
    _TestBinaryOperator -infinity, 2, "to the power of", infinity
    _TestBinaryOperator -infinity, 3, "to the power of", -infinity
    _TestBinaryOperator 2, infinity, "to the power of", infinity
    _TestBinaryOperator 2, -infinity, "to the power of", 0

    _TestBinaryOperator infinity, 1, "added to", infinity
    _TestBinaryOperator -infinity, 1, "added to", -infinity
    _TestBinaryOperator 1, infinity, "added to", infinity
    _TestBinaryOperator 1, -infinity, "added to", -infinity

    _TestBinaryOperator 1, infinity, "subtracted from", infinity
    _TestBinaryOperator 1, -infinity, "subtracted from", -infinity
    _TestBinaryOperator infinity, 1, "subtracted from", -infinity
    _TestBinaryOperator -infinity, 1, "subtracted from", infinity

    _TestBinaryOperator infinity, 2, "multiplied by", infinity
    _TestBinaryOperator -infinity, 2, "multiplied by", -infinity
    _TestBinaryOperator 2, infinity, "multiplied by", infinity
    _TestBinaryOperator 2, -infinity, "multiplied by", -infinity

    _TestBinaryOperator infinity, 2, "divided by", infinity
    _TestBinaryOperator -infinity, 2, "divided by", -infinity
    _TestBinaryOperator 2, infinity, "divided by", 0
    _TestBinaryOperator 2, -infinity, "divided by", 0

    _TestBinaryOperator infinity, 2, "div", infinity
    _TestBinaryOperator -infinity, 2, "div", -infinity
    _TestBinaryOperator 2, infinity, "div", 0
    _TestBinaryOperator 2, -infinity, "div", 0

    _TestBinaryOperator 2, infinity, "mod", 2
    _TestBinaryOperator 2, -infinity, "mod", 2

    _TestBinaryOperator 2, infinity, "wrap", 2
    _TestBinaryOperator 2, -infinity, "wrap", 2

    _TestBinaryOperator 1, infinity, "atan2", 0
    _TestBinaryOperator infinity, 1, "atan2", pi/2
    _TestBinaryOperator infinity, infinity, "atan2", pi/4
    _TestBinaryOperator 1, -infinity, "atan2", pi
    _TestBinaryOperator -infinity, 1, "atan2", -pi/2
    _TestBinaryOperator -infinity, -infinity, "atan2", -3*pi/4
end TestInfinityBinaryOperators

private command _TestUnaryOperator pOperator, pOperand, pExpected
    local tResult, tExpr
    put pOperator & "(" & pOperand & ")" into tExpr
    put value(tExpr) into tResult
    TestAssert tExpr && "is" && pExpected, \
            tResult is pExpected
end _TestUnaryOperator

on TestInfinityUnaryOperators
    _TestUnaryOperator "atan", infinity, pi/2
    _TestUnaryOperator "atan", -infinity, -pi/2

    _TestUnaryOperator "exp", infinity, infinity
    _TestUnaryOperator "exp", -infinity, 0
    _TestUnaryOperator "exp1", infinity, infinity
    _TestUnaryOperator "exp1", -infinity, -1
    _TestUnaryOperator "exp2", infinity, infinity
    _TestUnaryOperator "exp2", -infinity, 0
    _TestUnaryOperator "exp10", infinity, infinity
    _TestUnaryOperator "exp10", -infinity, 0

    _TestUnaryOperator "ln", infinity, infinity
    _TestUnaryOperator "ln1", infinity, infinity
    _TestUnaryOperator "log2", infinity, infinity
    _TestUnaryOperator "log10", infinity, infinity

    _TestUnaryOperator "sqrt", infinity, infinity
end TestInfinityUnaryOperators

on TestInfinityNaryOperators
    repeat for each item tItem in "sum,min,max,avg,median"
        _TestUnaryOperator tItem, infinity, infinity
        if the platform is "win32" then
            TestSkip tItem & "(-inf) is -inf", "Bug 22038"
        else
            _TestUnaryOperator tItem, -infinity, -infinity
        end if
    end repeat

    _TestBinaryOperator 2, infinity, "min", 2
    _TestBinaryOperator 2, -infinity, "min", -infinity

    _TestBinaryOperator 2, infinity, "max", infinity
    _TestBinaryOperator 2, -infinity, "max", 2

    TestAssert "Result of median(2,inf,inf) is inf", median(2,infinity,infinity) is infinity
    TestAssert "Result of median(2,-inf,-inf) is -inf", median(2,-infinity,-infinity) is -infinity
end TestInfinityNaryOperators
