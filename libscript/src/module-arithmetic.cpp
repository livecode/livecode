/* Copyright (C) 2003-2013 Runtime Revolution Ltd.
 
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

#include <foundation.h>
#include <foundation-auto.h>

#include <float.h>

// We use 'checked' forms of all the (non-integral) number operations at the moment.
// This includes not allowing non-finite numbers out of Parse.
//
// When we have an improved (zero-cost!) implementation of context variables
// how to handle 'invalid' numbers will be controlled by such.

#define MC_ARITHMETIC_BINARY_OP(OpName, TargettedName, ResultName) \
    extern "C" MC_DLLEXPORT void MCArithmeticExec##TargettedName(MCNumberRef p_source, MCNumberRef& x_target) \
    { \
        MCNumberRef t_result; \
        if (!MCNumber##OpName(x_target, p_source, t_result)) \
            return; \
        MCValueAssign(x_target, t_result); \
    } \
    extern "C" MC_DLLEXPORT void MCArithmeticEval##ResultName(MCNumberRef p_left, MCNumberRef p_right, MCNumberRef& r_result) \
    { \
        MCNumber##OpName(p_left, p_right, r_result); \
    }

#define MC_ARITHMETIC_BINARY_OP_R(OpName, TargettedName, ResultName) \
    extern "C" MC_DLLEXPORT void MCArithmeticExec##TargettedName(MCNumberRef& x_target, MCNumberRef p_source ) \
    { \
        MCNumberRef t_result; \
        if (!MCNumber##OpName(x_target, p_source, t_result)) \
            return; \
        MCValueAssign(x_target, t_result); \
    } \
    extern "C" MC_DLLEXPORT void MCArithmeticEval##ResultName(MCNumberRef p_left, MCNumberRef p_right, MCNumberRef& r_result) \
    { \
        MCNumber##OpName(p_left, p_right, r_result); \
    }

#define MC_ARITHMETIC_EXPR_BINARY_OP(OpName, ResultName) \
    extern "C" MC_DLLEXPORT void MCArithmeticEval##ResultName(MCNumberRef p_left, MCNumberRef p_right, MCNumberRef& r_result) \
    { \
        MCNumber##OpName(p_left, p_right, r_result); \
    }

#define MC_ARITHMETIC_COMPARE_OP(OpName) \
    extern "C" MC_DLLEXPORT void MCArithmeticEvalNumber##OpName##Number(MCNumberRef p_left, MCNumberRef p_right, bool& r_output) \
    { \
        r_output = MCNumber##OpName(p_left, p_right); \
    } \
    extern "C" MC_DLLEXPORT void MCArithmeticEvalInteger##OpName##Integer(MCNumberRef p_left, MCNumberRef p_right, bool& r_output) \
    { \
        r_output = MCNumberIntegral##OpName(p_left, p_right); \
    }

MC_ARITHMETIC_BINARY_OP(FiniteAdd, AddNumberToNumber, NumberPlusNumber)
MC_ARITHMETIC_BINARY_OP(IntegralAdd, AddIntegerToInteger, IntegerPlusInteger)
MC_ARITHMETIC_BINARY_OP(FiniteSubtract, SubtractNumberFromNumber, NumberMinusNumber)
MC_ARITHMETIC_BINARY_OP(IntegralSubtract, SubtractIntegerFromInteger, IntegerMinusInteger)
MC_ARITHMETIC_BINARY_OP_R(FiniteMultiply, MultiplyNumberByNumber, NumberTimesNumber)
MC_ARITHMETIC_BINARY_OP_R(IntegralMultiply, MultiplyIntegerByInteger, IntegerTimesInteger)
MC_ARITHMETIC_BINARY_OP_R(FiniteDivide, DivideNumberByNumber, NumberOverNumber)

MC_ARITHMETIC_EXPR_BINARY_OP(FiniteDiv, NumberDivNumber)
MC_ARITHMETIC_EXPR_BINARY_OP(IntegralDiv, IntegerDivInteger)
MC_ARITHMETIC_EXPR_BINARY_OP(FiniteMod, NumberModNumber)
MC_ARITHMETIC_EXPR_BINARY_OP(IntegralMod, IntegerModInteger)
//MC_ARITHMETIC_EXPR_BINARY_OP(Wrap, NumberWrapNumber)
//MC_ARITHMETIC_EXPR_BINARY_OP(IntegralWrap, IntegerWrapInteger)

MC_ARITHMETIC_COMPARE_OP(IsEqualTo)
MC_ARITHMETIC_COMPARE_OP(IsNotEqualTo)
MC_ARITHMETIC_COMPARE_OP(IsGreaterThan)
MC_ARITHMETIC_COMPARE_OP(IsGreaterThanOrEqualTo)
MC_ARITHMETIC_COMPARE_OP(IsLessThan)
MC_ARITHMETIC_COMPARE_OP(IsLessThanOrEqualTo)

extern "C" MC_DLLEXPORT void MCArithmeticEvalPlusNumber(MCNumberRef p_operand, MCNumberRef& r_output)
{
    r_output = MCValueRetain(p_operand);
}

extern "C" MC_DLLEXPORT void MCArithmeticEvalPlusInteger(MCNumberRef p_operand, MCNumberRef& r_output)
{
    r_output = MCValueRetain(p_operand);
}

extern "C" MC_DLLEXPORT void MCArithmeticEvalMinusNumber(MCNumberRef p_operand, MCNumberRef& r_output)
{
    MCNumberNegate(p_operand, r_output);
}

extern "C" MC_DLLEXPORT void MCArithmeticEvalMinusInteger(MCNumberRef p_operand, MCNumberRef& r_output)
{
    MCNumberIntegralNegate(p_operand, r_output);
}

extern "C" MC_DLLEXPORT void MCArithmeticEvalEqualToInteger(integer_t p_left, integer_t p_right, bool& r_output)
{
    r_output = p_left == p_right;
}

extern "C" MC_DLLEXPORT void MCArithmeticEvalEqualToReal(double p_left, double p_right, bool& r_output)
{
    r_output = p_left == p_right;
}

extern "C" MC_DLLEXPORT void MCArithmeticEvalEqualToNumber(MCNumberRef p_left, MCNumberRef p_right, bool& r_output)
{
    r_output = MCNumberFetchAsReal(p_left) == MCNumberFetchAsReal(p_right);
}

extern "C" MC_DLLEXPORT void MCArithmeticEvalNotEqualToInteger(integer_t p_left, integer_t p_right, bool& r_output)
{
    r_output = p_left != p_right;
}

extern "C" MC_DLLEXPORT void MCArithmeticEvalNotEqualToReal(double p_left, double p_right, bool& r_output)
{
    r_output = p_left != p_right;
}

extern "C" MC_DLLEXPORT void MCArithmeticEvalNotEqualToNumber(MCNumberRef p_left, MCNumberRef p_right, bool& r_output)
{
    r_output = MCNumberFetchAsReal(p_left) != MCNumberFetchAsReal(p_right);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

extern "C" MC_DLLEXPORT MCStringRef MCArithmeticExecFormatNumberAsString(MCNumberRef p_operand)
{
    MCStringRef t_string;
    if (!MCNumberFormat(p_operand, t_string))
        return nil;
    
    return t_string;
}

extern "C" MC_DLLEXPORT MCNumberRef MCArithmeticExecParseStringAsNumber(MCStringRef p_operand)
{
    MCNumberRef t_number;
    if (!MCNumberTryToParse(p_operand, t_number))
        return nil;
    if (!MCNumberIsFinite(t_number))
        return nil;
    return t_number;
}

extern "C" MC_DLLEXPORT MCProperListRef MCArithmeticExecParseListOfStringAsListOfNumber(MCProperListRef p_list_of_string)
{
    MCAutoProperListRef t_output;
    if (!MCProperListCreateMutable(&t_output))
        return nil;
    
    for(uindex_t i = 0; i < MCProperListGetLength(p_list_of_string); i++)
    {
        MCValueRef t_element;
        t_element = MCProperListFetchElementAtIndex(p_list_of_string, i);
        if (MCValueGetTypeCode(t_element) != kMCValueTypeCodeString)
        {
            MCErrorThrowGeneric(MCSTR("not a list of string"));
            return nil;
        }
        
        MCAutoNumberRef t_numeric_element;
        if (!MCNumberTryToParse((MCStringRef)t_element, &t_numeric_element))
            return nil;
        
        if (!MCProperListPushElementOntoBack(*t_output, *t_numeric_element != nil ? (MCValueRef)*t_numeric_element : (MCValueRef)kMCNull))
            return nil;
    }
    
    MCAutoProperListRef t_list;
    if (!MCProperListCopy(*t_output, &t_list))
        return nil;
    
    return t_list . Take();
}

extern "C" MC_DLLEXPORT void MCArithmeticEvalNumberFormattedAsString(MCNumberRef p_operand, MCStringRef& r_output)
{
    r_output = MCArithmeticExecFormatNumberAsString(p_operand);
}

extern "C" MC_DLLEXPORT void MCArithmeticEvalStringParsedAsNumber(MCStringRef p_operand, MCNumberRef& r_output)
{
    r_output = MCArithmeticExecParseStringAsNumber(p_operand);
}

extern "C" MC_DLLEXPORT void MCArithmeticEvalListOfStringParsedAsListOfNumber(MCProperListRef p_list_of_string, MCValueRef& r_output)
{
    r_output = MCArithmeticExecParseListOfStringAsListOfNumber(p_list_of_string);
}
