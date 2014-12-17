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

extern "C" MC_DLLEXPORT void MCArithmeticExecAddIntegerToInteger(integer_t p_number, integer_t& x_target)
{
    if (p_number > 0 && INTEGER_MAX - p_number < x_target)
        // overflow
        return;
    else if (p_number < 0 && INTEGER_MIN - p_number > x_target)
        // underflow
        return;
    else
        x_target += p_number;
}

extern "C" MC_DLLEXPORT void MCArithmeticExecAddRealToReal(double p_number, double& x_target)
{
    x_target += p_number;
}

extern "C" MC_DLLEXPORT void MCArithmeticExecAddNumberToNumber(MCNumberRef p_number, MCNumberRef& x_target)
{    
    double t_target, t_number;
    t_target = MCNumberFetchAsReal(x_target);
    t_number = MCNumberFetchAsReal(p_number);
    
    MCArithmeticExecAddRealToReal(t_number, t_target);
    
    MCAutoNumberRef t_new_number;
    MCNumberCreateWithReal(t_target, &t_new_number);
    
    MCValueAssign(x_target, *t_new_number);
}

extern "C" MC_DLLEXPORT void MCArithmeticExecSubtractIntegerFromInteger(integer_t p_number, integer_t& x_target)
{
    if (p_number > 0 && INTEGER_MIN + p_number > x_target)
        // overflow
        return;
    else if (p_number < 0 && INTEGER_MAX + p_number < x_target)
        // underflow
        return;
    else
        x_target -= p_number;
}

extern "C" MC_DLLEXPORT void MCArithmeticExecSubtractRealFromReal(double p_number, double& x_target)
{
    x_target -= p_number;
}

extern "C" MC_DLLEXPORT void MCArithmeticExecSubtractNumberFromNumber(MCNumberRef p_number, MCNumberRef& x_target)
{
    double t_target, t_number;
    t_target = MCNumberFetchAsReal(x_target);
    t_number = MCNumberFetchAsReal(p_number);
    
    MCArithmeticExecSubtractRealFromReal(t_number, t_target);
    
    MCAutoNumberRef t_new_number;
    MCNumberCreateWithReal(t_target, &t_new_number);
    
    MCValueAssign(x_target, *t_new_number);
}

extern "C" MC_DLLEXPORT void MCArithmeticExecMultiplyIntegerByInteger(integer_t& x_target, integer_t p_number)
{
    if (p_number > 0 && INTEGER_MAX / p_number < x_target)
        // overflow
        return;
    else if (p_number < 0 && INTEGER_MIN / p_number > x_target)
        // underflow
        return;
    else
        x_target *= p_number;
}

extern "C" MC_DLLEXPORT void MCArithmeticExecMultiplyRealByReal(double& x_target, double p_number)
{
    x_target *= p_number;
}

extern "C" MC_DLLEXPORT void MCArithmeticExecMultiplyNumberByNumber(MCNumberRef& x_target, MCNumberRef p_number)
{
    double t_target, t_number;
    t_target = MCNumberFetchAsReal(x_target);
    t_number = MCNumberFetchAsReal(p_number);
    
    MCArithmeticExecMultiplyRealByReal(t_target, t_number);
    
    MCAutoNumberRef t_new_number;
    MCNumberCreateWithReal(t_target, &t_new_number);
    
    MCValueAssign(x_target, *t_new_number);
}

extern "C" MC_DLLEXPORT void MCArithmeticExecDivideIntegerByInteger(integer_t& x_target, integer_t p_number)
{
    x_target /= p_number;
}

extern "C" MC_DLLEXPORT void MCArithmeticExecDivideRealByReal(double& x_target, double p_number)
{
    x_target /= p_number;
}

extern "C" MC_DLLEXPORT void MCArithmeticExecDivideNumberByNumber(MCNumberRef& x_target, MCNumberRef p_number)
{
    double t_target, t_number;
    t_target = MCNumberFetchAsReal(x_target);
    t_number = MCNumberFetchAsReal(p_number);
    
    MCArithmeticExecDivideRealByReal(t_target, t_number);
    
    MCAutoNumberRef t_new_number;
    MCNumberCreateWithReal(t_target, &t_new_number);
    
    MCValueAssign(x_target, *t_new_number);
}

extern "C" MC_DLLEXPORT void MCArithmeticEvalIntegerPlusInteger(integer_t p_left, integer_t p_right, integer_t& r_output)
{
    MCArithmeticExecAddIntegerToInteger(p_left, p_right);
    
    //if no error
    r_output = p_right;
}

extern "C" MC_DLLEXPORT void MCArithmeticEvalRealPlusReal(double p_left, double p_right, double& r_output)
{
    MCArithmeticExecAddRealToReal(p_left, p_right);
    
    //if no error
    r_output = p_right;
}

extern "C" MC_DLLEXPORT void MCArithmeticEvalNumberPlusNumber(MCNumberRef p_left, MCNumberRef p_right, MCNumberRef& r_output)
{
    MCNumberRef t_number;
    MCNumberCreateWithReal(MCNumberFetchAsReal(p_right), t_number);
    MCArithmeticExecAddNumberToNumber(p_left, t_number);
    
    //if no error
    r_output = t_number;
    return;
    
    //else
    MCValueRelease(t_number);
}

extern "C" MC_DLLEXPORT void MCArithmeticEvalIntegerMinusInteger(integer_t p_left, integer_t p_right, integer_t& r_output)
{
    MCArithmeticExecSubtractIntegerFromInteger(p_right, p_left);
    
    //if no error
    r_output = p_left;
}

extern "C" MC_DLLEXPORT void MCArithmeticEvalRealMinusReal(double p_left, double p_right, double& r_output)
{
    MCArithmeticExecSubtractRealFromReal(p_right, p_left);
    
    //if no error
    r_output = p_left;
}

extern "C" MC_DLLEXPORT void MCArithmeticEvalNumberMinusNumber(MCNumberRef p_left, MCNumberRef p_right, MCNumberRef& r_output)
{
    MCNumberRef t_number;
    MCNumberCreateWithReal(MCNumberFetchAsReal(p_left), t_number);
    MCArithmeticExecSubtractNumberFromNumber(p_right, t_number);
    
    //if no error
    r_output = t_number;
    return;
    
    //else
    MCValueRelease(t_number);
}

extern "C" MC_DLLEXPORT void MCArithmeticEvalIntegerTimesInteger(integer_t p_left, integer_t p_right, integer_t& r_output)
{
    MCArithmeticExecMultiplyIntegerByInteger(p_left, p_right);
    
    //if no error
    r_output = p_left;
}

extern "C" MC_DLLEXPORT void MCArithmeticEvalRealTimesReal(double p_left, double p_right, double& r_output)
{
    MCArithmeticExecMultiplyRealByReal(p_left, p_right);
    
    //if no error
    r_output = p_left;
}

extern "C" MC_DLLEXPORT void MCArithmeticEvalNumberTimesNumber(MCNumberRef p_left, MCNumberRef p_right, MCNumberRef& r_output)
{
    MCNumberRef t_number;
    MCNumberCreateWithReal(MCNumberFetchAsReal(p_right), t_number);
    MCArithmeticExecMultiplyNumberByNumber(t_number, p_left);
    
    //if no error
    r_output = t_number;
    return;
    
    //else
    MCValueRelease(t_number);
}

extern "C" MC_DLLEXPORT void MCArithmeticEvalIntegerOverInteger(integer_t p_left, integer_t p_right, integer_t& r_output)
{
    MCArithmeticExecDivideIntegerByInteger(p_left, p_right);
    
    //if no error
    r_output = p_left;
}

extern "C" MC_DLLEXPORT void MCArithmeticEvalRealOverReal(double p_left, double p_right, double& r_output)
{
    MCArithmeticExecDivideRealByReal(p_left, p_right);
    
    //if no error
    r_output = p_left;
}

extern "C" MC_DLLEXPORT void MCArithmeticEvalNumberOverNumber(MCNumberRef p_left, MCNumberRef p_right, MCNumberRef& r_output)
{
    MCNumberRef t_number;
    MCNumberCreateWithReal(MCNumberFetchAsReal(p_left), t_number);
    MCArithmeticExecDivideNumberByNumber(t_number, p_right);
    
    //if no error
    r_output = t_number;
    return;
    
    //else
    MCValueRelease(t_number);
}

extern "C" MC_DLLEXPORT void MCArithmeticEvalIntegerModInteger(integer_t p_left, integer_t p_right, integer_t& r_output)
{
    if (p_right == 0)
        return;
    
    r_output = fmod(double(p_left), double(p_right));
}

extern "C" MC_DLLEXPORT void MCArithmeticEvalRealModReal(double p_left, double p_right, double& r_output)
{
    double n = 0.0;
    n = p_left / p_right;
    //if (n == MCinfinity)
    
    r_output = fmod(p_left, p_right);
}

extern "C" MC_DLLEXPORT void MCArithmeticEvalNumberModNumber(MCNumberRef p_left, MCNumberRef p_right, MCNumberRef& r_output)
{
    double t_left, t_right;
    t_left = MCNumberFetchAsReal(p_left);
    t_right = MCNumberFetchAsReal(p_right);
    
    double t_result;
    MCArithmeticEvalRealModReal(t_left, t_right, t_result);
    
    MCNumberCreateWithReal(t_result, r_output);
}

extern "C" MC_DLLEXPORT void MCArithmeticEvalIntegerWrapInteger(integer_t p_left, integer_t p_right, integer_t& r_output)
{
    if (p_right == 0)
        return;
    
    integer_t t_y;
	t_y = p_left > 0 ? p_right : -p_right;
	if (p_left >= 0)
		r_output = (fmod(double(p_left - 1), double(t_y)) + 1);
	else
		r_output = -(fmod(double(-p_left - 1), double(t_y)) + 1);
}

extern "C" MC_DLLEXPORT void MCArithmeticEvalRealWrapReal(double p_left, double p_right, double& r_output)
{
    double n = 0.0;
    n = p_left / p_right;
    //if (n == MCinfinity)
    
    double t_y;
	t_y = p_left > 0 ? p_right : -p_right;
	if (p_left >= 0)
		r_output = (fmod(p_left - 1, t_y) + 1);
	else
		r_output = -(fmod(-p_left - 1, t_y) + 1);
}

extern "C" MC_DLLEXPORT void MCArithmeticEvalNumberWrapNumber(MCNumberRef p_left, MCNumberRef p_right, MCNumberRef& r_output)
{
    double t_left, t_right;
    t_left = MCNumberFetchAsReal(p_left);
    t_right = MCNumberFetchAsReal(p_right);
    
    double t_result;
    MCArithmeticEvalRealWrapReal(t_left, t_right, t_result);
    
    MCNumberCreateWithReal(t_result, r_output);
}

extern "C" MC_DLLEXPORT void MCArithmeticEvalIntegerIsGreaterThanInteger(integer_t p_left, integer_t p_right, bool& r_output)
{
    r_output = (p_left > p_right);
}

extern "C" MC_DLLEXPORT void MCArithmeticEvalRealIsGreaterThanReal(double p_left, double p_right, bool& r_output)
{
    r_output = (p_left > p_right);
}

extern "C" MC_DLLEXPORT void MCArithmeticEvalNumberIsGreaterThanNumber(MCNumberRef p_left, MCNumberRef p_right, bool& r_output)
{
    r_output = (MCNumberFetchAsReal(p_left) > MCNumberFetchAsReal(p_right));
}

extern "C" MC_DLLEXPORT void MCArithmeticEvalIntegerIsGreaterThanOrEqualToInteger(integer_t p_left, integer_t p_right, bool& r_output)
{
    r_output = (p_left >= p_right);
}

extern "C" MC_DLLEXPORT void MCArithmeticEvalRealIsGreaterThanOrEqualToReal(double p_left, double p_right, bool& r_output)
{
    r_output = (p_left >= p_right);
}

extern "C" MC_DLLEXPORT void MCArithmeticEvalNumberIsGreaterThanOrEqualToNumber(MCNumberRef p_left, MCNumberRef p_right, bool& r_output)
{
    r_output = (MCNumberFetchAsReal(p_left) >= MCNumberFetchAsReal(p_right));
}

extern "C" MC_DLLEXPORT void MCArithmeticEvalIntegerIsLessThanInteger(integer_t p_left, integer_t p_right, bool& r_output)
{
    r_output = (p_left < p_right);
}

extern "C" MC_DLLEXPORT void MCArithmeticEvalRealIsLessThanReal(double p_left, double p_right, bool& r_output)
{
    r_output = (p_left < p_right);
}

extern "C" MC_DLLEXPORT void MCArithmeticEvalNumberIsLessThanNumber(MCNumberRef p_left, MCNumberRef p_right, bool& r_output)
{
    r_output = (MCNumberFetchAsReal(p_left) < MCNumberFetchAsReal(p_right));
}

extern "C" MC_DLLEXPORT void MCArithmeticEvalIntegerIsLessThanOrEqualToInteger(integer_t p_left, integer_t p_right, bool& r_output)
{
    r_output = (p_left <= p_right);
}

extern "C" MC_DLLEXPORT void MCArithmeticEvalRealIsLessThanOrEqualToReal(double p_left, double p_right, bool& r_output)
{
    r_output = (p_left <= p_right);
}

extern "C" MC_DLLEXPORT void MCArithmeticEvalNumberIsLessThanOrEqualToNumber(MCNumberRef p_left, MCNumberRef p_right, bool& r_output)
{
    r_output = (MCNumberFetchAsReal(p_left) <= MCNumberFetchAsReal(p_right));
}

extern "C" MC_DLLEXPORT void MCArithmeticEvalPlusInteger(integer_t p_operand, integer_t& r_output)
{
    r_output = p_operand;
}

extern "C" MC_DLLEXPORT void MCArithmeticEvalPlusReal(double p_operand, double& r_output)
{
    r_output = p_operand;
}

extern "C" MC_DLLEXPORT void MCArithmeticEvalPlusNumber(MCNumberRef p_operand, MCNumberRef& r_output)
{
    r_output = MCValueRetain(p_operand);
}

extern "C" MC_DLLEXPORT void MCArithmeticEvalMinusInteger(integer_t p_operand, integer_t& r_output)
{
    r_output = -p_operand;
}

extern "C" MC_DLLEXPORT void MCArithmeticEvalMinusReal(double p_operand, double& r_output)
{
    r_output = -p_operand;
}

extern "C" MC_DLLEXPORT void MCArithmeticEvalMinusNumber(MCNumberRef p_operand, MCNumberRef& r_output)
{
    if (MCNumberIsInteger(p_operand))
        MCNumberCreateWithInteger(-MCNumberFetchAsInteger(p_operand), r_output);
    else
        MCNumberCreateWithReal(-MCNumberFetchAsReal(p_operand), r_output);
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

extern "C" MC_DLLEXPORT void MCArithmeticEvalNumberFormattedAsString(MCNumberRef p_operand, MCStringRef& r_output)
{
    MCStringFormat(r_output, "%f", MCNumberFetchAsReal(p_operand));
}

extern "C" MC_DLLEXPORT void MCArithmeticEvalStringParsedAsNumber(MCStringRef p_operand, MCNumberRef& r_output)
{
    double t_converted;
    if (!MCTypeConvertStringToReal(p_operand, t_converted))
    {
        MCErrorCreateAndThrow(kMCGenericErrorTypeInfo, "reason", MCSTR("input string is not a number"), nil);
        return;
    }
    
    if (!MCNumberCreateWithReal(t_converted, r_output))
        return;
}


