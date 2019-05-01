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
    _TestBinaryOperator "inf", 1, "plus", "inf"
    _TestBinaryOperator "-inf", 1, "plus", "-inf"

    _TestBinaryOperator "inf", 1, "minus", "inf"
    _TestBinaryOperator "-inf", 1, "minus", "-inf"
    _TestBinaryOperator 1, "inf", "minus", "-inf"
    _TestBinaryOperator 1, "-inf", "minus", "inf"

    _TestBinaryOperator "inf", 2, "times", "inf"
    _TestBinaryOperator "-inf", 2, "times", "-inf"

    _TestBinaryOperator "inf", 2, "over", "inf"
    _TestBinaryOperator "-inf", 2, "over", "-inf"
    _TestBinaryOperator 2, "inf", "over", 0
    _TestBinaryOperator 2, "-inf", "over", 0

    _TestBinaryOperator "inf", 2, "to the power of", "inf"
    _TestBinaryOperator "-inf", 2, "to the power of", "inf"
    _TestBinaryOperator "-inf", 3, "to the power of", "-inf"
    _TestBinaryOperator 2, "inf", "to the power of", "inf"
    _TestBinaryOperator 2, "-inf", "to the power of", 0

    _TestBinaryOperator "inf", 1, "added to", "inf"
    _TestBinaryOperator "-inf", 1, "added to", "-inf"
    _TestBinaryOperator 1, "inf", "added to", "inf"
    _TestBinaryOperator 1, "-inf", "added to", "-inf"

    _TestBinaryOperator 1, "inf", "subtracted from", "inf"
    _TestBinaryOperator 1, "-inf", "subtracted from", "-inf"
    _TestBinaryOperator "inf", 1, "subtracted from", "-inf"
    _TestBinaryOperator "-inf", 1, "subtracted from", "inf"

    _TestBinaryOperator "inf", 2, "multiplied by", "inf"
    _TestBinaryOperator "-inf", 2, "multiplied by", "-inf"
    _TestBinaryOperator 2, "inf", "multiplied by", "inf"
    _TestBinaryOperator 2, "-inf", "multiplied by", "-inf"

    _TestBinaryOperator "inf", 2, "divided by", "inf"
    _TestBinaryOperator "-inf", 2, "divided by", "-inf"
    _TestBinaryOperator 2, "inf", "divided by", 0
    _TestBinaryOperator 2, "-inf", "divided by", 0

    _TestBinaryOperator "inf", 2, "div", "inf"
    _TestBinaryOperator "-inf", 2, "div", "-inf"
    _TestBinaryOperator 2, "inf", "div", 0
    _TestBinaryOperator 2, "-inf", "div", 0

    _TestBinaryOperator 2, "inf", "mod", 2
    _TestBinaryOperator 2, "-inf", "mod", 2

    _TestBinaryOperator 2, "inf", "wrap", 2
    _TestBinaryOperator 2, "-inf", "wrap", 2

    _TestBinaryOperator 1, "inf", "atan2", 0
    _TestBinaryOperator "inf", 1, "atan2", pi/2
    _TestBinaryOperator "inf", "inf", "atan2", pi/4
    _TestBinaryOperator 1, "-inf", "atan2", pi
    _TestBinaryOperator "-inf", 1, "atan2", -pi/2
    _TestBinaryOperator "-inf", "-inf", "atan2", -3*pi/4
end TestInfinityBinaryOperators

private command _TestUnaryOperator pOperator, pOperand, pExpected
    local tResult, tExpr
    put pOperator & "(" & pOperand & ")" into tExpr
    put value(tExpr) into tResult
    TestAssert tExpr && "is" && pExpected, \
            tResult is pExpected
end _TestUnaryOperator

on TestInfinityUnaryOperators
    _TestUnaryOperator "atan", "inf", pi/2
    _TestUnaryOperator "atan", "-inf", -pi/2

    _TestUnaryOperator "exp", "inf", "inf"
    _TestUnaryOperator "exp", "-inf", 0
    _TestUnaryOperator "exp1", "inf", "inf"
    _TestUnaryOperator "exp1", "-inf", -1
    _TestUnaryOperator "exp2", "inf", "inf"
    _TestUnaryOperator "exp2", "-inf", 0
    _TestUnaryOperator "exp10", "inf", "inf"
    _TestUnaryOperator "exp10", "-inf", 0

    _TestUnaryOperator "ln", "inf", "inf"
    _TestUnaryOperator "ln1", "inf", "inf"
    _TestUnaryOperator "log2", "inf", "inf"
    _TestUnaryOperator "log10", "inf", "inf"

    _TestUnaryOperator "sqrt", "inf", "inf"
end TestInfinityUnaryOperators

on TestInfinityNaryOperators
    repeat for each item tItem in "sum,min,max,avg,median"
        _TestUnaryOperator tItem, "inf", "inf"
        if the platform is "win32" then
            TestSkip tItem & "(-inf) is -inf", "Bug 22038"
        else
            _TestUnaryOperator tItem, "-inf", "-inf"
        end if
    end repeat

    _TestBinaryOperator 2, "inf", "min", 2
    _TestBinaryOperator 2, "-inf", "min", "-inf"

    _TestBinaryOperator 2, "inf", "max", "inf"
    _TestBinaryOperator 2, "-inf", "max", 2

    TestAssert "Result of median(2,inf,inf) is inf", median(2,"inf","inf") is "inf"
    TestAssert "Result of median(2,-inf,-inf) is -inf", median(2,"-inf","-inf") is "-inf"
end TestInfinityNaryOperators
