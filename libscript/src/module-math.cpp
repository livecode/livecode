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
#include "foundation-math.h"

#include <float.h>

// Older versions of MSVC don't supply "trunc"
#ifdef _WIN32
double trunc(double f) { return f < 0 ? ceil(f) : floor(f); }
#endif

void MCMathEvalRealToPowerOfReal(double p_left, double p_right, double& r_output)
{
    if (p_right == 0)
    {
        r_output = 1.0;
        return;
    }
    
    if (p_right > 0 && p_left > 0 && p_left > (log(FLT_MAX)/log(p_right)))
    {
        // overflow
        return;
    }
    r_output = pow(p_left, p_right);
}

void MCMathEvalIntegerToPowerOfInteger(integer_t p_left, integer_t p_right, integer_t& r_output)
{
    if (p_right == 0)
    {
        r_output = 1;
        return;
    }
    
    double t_result;
    MCMathEvalRealToPowerOfReal(p_left, p_right, t_result);
    
    if (t_result > INTEGER_MAX || t_result < INTEGER_MIN)
        // error
        return;
    
    integer_t t_int_result;
    t_int_result = (integer_t)t_result;
    if (t_int_result != t_result)
        // error
        return;
    
    r_output = t_int_result;
}

void MCMathEvalNumberToPowerOfNumber(MCNumberRef p_left, MCNumberRef p_right, MCNumberRef& r_output)
{
    double t_left, t_right;
    t_left = MCNumberFetchAsReal(p_left);
    t_right = MCNumberFetchAsReal(p_right);
    
    double t_result;
    MCMathEvalRealToPowerOfReal(t_left, t_right, t_result);
    
    // if (!ctxt . HasError())
    MCNumberCreateWithReal(t_result, r_output);
}

void MCMathEvalBase10LogReal(double p_operand, double& r_output)
{
    r_output = log10(p_operand);
}

void MCMathEvalBase10LogInteger(integer_t p_operand, double& r_output)
{
    MCMathEvalBase10LogReal(p_operand, r_output);
}

void MCMathEvalBase10LogNumber(MCNumberRef p_operand, MCNumberRef& r_output)
{
    double t_operand;
    t_operand = MCNumberFetchAsReal(p_operand);
    
    double t_result;
    MCMathEvalBase10LogReal(t_operand, t_result);
    
    // if (!ctxt . HasError())
    MCNumberCreateWithReal(t_result, r_output);
}

void MCMathEvalNaturalLogReal(double p_operand, double& r_output)
{
    r_output = log(p_operand);
}

void MCMathEvalNaturalLogInteger(integer_t p_operand, double& r_output)
{
    MCMathEvalNaturalLogReal(p_operand, r_output);
}

void MCMathEvalNaturalLogNumber(MCNumberRef p_operand, MCNumberRef& r_output)
{
    double t_operand;
    t_operand = MCNumberFetchAsReal(p_operand);
    
    double t_result;
    MCMathEvalNaturalLogReal(t_operand, t_result);
    
    // if (!ctxt . HasError())
    MCNumberCreateWithReal(t_result, r_output);
}

void MCMathEvalExpReal(double p_operand, double& r_output)
{
    r_output = exp(p_operand);
}

void MCMathEvalExpInteger(integer_t p_operand, double& r_output)
{
    MCMathEvalExpReal(p_operand, r_output);
}

void MCMathEvalExpNumber(MCNumberRef p_operand, MCNumberRef& r_output)
{
    double t_operand;
    t_operand = MCNumberFetchAsReal(p_operand);
    
    double t_result;
    MCMathEvalExpReal(t_operand, t_result);
    
    // if (!ctxt . HasError())
    MCNumberCreateWithReal(t_result, r_output);
}

void MCMathEvalSinReal(double p_operand, double& r_output)
{
    r_output = sin(p_operand);
}

void MCMathEvalSinInteger(integer_t p_operand, double& r_output)
{
    MCMathEvalSinReal(p_operand, r_output);
}

void MCMathEvalSinNumber(MCNumberRef p_operand, MCNumberRef& r_output)
{
    double t_operand;
    t_operand = MCNumberFetchAsReal(p_operand);
    
    double t_result;
    MCMathEvalSinReal(t_operand, t_result);
    
    // if (!ctxt . HasError())
    MCNumberCreateWithReal(t_result, r_output);
}

void MCMathEvalCosReal(double p_operand, double& r_output)
{
    r_output = cos(p_operand);
}

void MCMathEvalCosInteger(integer_t p_operand, double& r_output)
{
    MCMathEvalCosReal(p_operand, r_output);
}

void MCMathEvalCosNumber(MCNumberRef p_operand, MCNumberRef& r_output)
{
    double t_operand;
    t_operand = MCNumberFetchAsReal(p_operand);
    
    double t_result;
    MCMathEvalCosReal(t_operand, t_result);
    
    // if (!ctxt . HasError())
    MCNumberCreateWithReal(t_result, r_output);
}

void MCMathEvalTanReal(double p_operand, double& r_output)
{
    r_output = tan(p_operand);
}

void MCMathEvalTanInteger(integer_t p_operand, double& r_output)
{
    MCMathEvalTanReal(p_operand, r_output);
}

void MCMathEvalTanNumber(MCNumberRef p_operand, MCNumberRef& r_output)
{
    double t_operand;
    t_operand = MCNumberFetchAsReal(p_operand);
    
    double t_result;
    MCMathEvalTanReal(t_operand, t_result);
    
    // if (!ctxt . HasError())
    MCNumberCreateWithReal(t_result, r_output);
}

void MCMathEvalAbsInteger(integer_t p_operand, integer_t& r_output)
{
    r_output = MCAbs(p_operand);
}

void MCMathEvalAbsReal(double p_operand, double& r_output)
{
    r_output = MCAbs(p_operand);
}

void MCMathEvalAbsNumber(MCNumberRef p_operand, MCNumberRef& r_output)
{
    if (MCNumberIsInteger(p_operand))
    {
        integer_t t_abs;
        MCMathEvalAbsInteger(MCNumberFetchAsInteger(p_operand), t_abs);
        MCNumberCreateWithInteger(t_abs, r_output);
        return;
    }
    
    double t_abs_real;
    MCMathEvalAbsReal(MCNumberFetchAsReal(p_operand), t_abs_real);
    MCNumberCreateWithReal(t_abs_real, r_output);
}

void MCMathEvalTruncInteger(integer_t p_operand, integer_t& r_output)
{
    r_output = p_operand;
}

void MCMathEvalTruncReal(double p_operand, double& r_output)
{
    r_output = trunc(p_operand);
}

void MCMathEvalTruncNumber(MCNumberRef p_operand, MCNumberRef& r_output)
{
    if (MCNumberIsInteger(p_operand))
    {
        integer_t t_abs;
        MCMathEvalAbsInteger(MCNumberFetchAsInteger(p_operand), t_abs);
        MCNumberCreateWithInteger(t_abs, r_output);
        return;
    }
    
    double t_abs_real;
    MCMathEvalAbsReal(MCNumberFetchAsReal(p_operand), t_abs_real);
    MCNumberCreateWithReal(t_abs_real, r_output);
}

void MCMathEvalMinInteger(integer_t p_left, integer_t p_right, integer_t& r_output)
{
    r_output = MCMin(p_left, p_right);
}

void MCMathEvalMinReal(double p_left, double p_right, double& r_output)
{
    r_output = MCMin(p_left, p_right);
}

void MCMathEvalMinNumber(MCNumberRef p_left, MCNumberRef p_right, MCNumberRef& r_output)
{
    double t_left, t_right, t_result;
    t_left = MCNumberFetchAsReal(p_left);
    t_right = MCNumberFetchAsReal(p_right);
    
    MCMathEvalMinReal(t_left, t_right, t_result);
    
    MCNumberCreateWithReal(t_result, r_output);
}

void MCMathEvalMaxInteger(integer_t p_left, integer_t p_right, integer_t& r_output)
{
    r_output = MCMax(p_left, p_right);
}

void MCMathEvalMaxReal(double p_left, double p_right, double& r_output)
{
    r_output = MCMax(p_left, p_right);
}

void MCMathEvalMaxNumber(MCNumberRef p_left, MCNumberRef p_right, MCNumberRef& r_output)
{
    double t_left, t_right, t_result;
    t_left = MCNumberFetchAsReal(p_left);
    t_right = MCNumberFetchAsReal(p_right);
    
    MCMathEvalMaxReal(t_left, t_right, t_result);
    
    MCNumberCreateWithReal(t_result, r_output);
}

void MCMathEvalRandomReal(double& r_output)
{
    r_output = MCMathRandom();
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void MCMathEvalConvertToBase10(MCStringRef p_operand, integer_t p_source_base, integer_t& r_output)
{
    // p_source_base must be an integer between 2 and 32
    bool t_negative;
    uinteger_t t_result;
    bool t_error;
    if (MCMathConvertToBase10(p_operand, p_source_base, t_negative, t_result, t_error))
    {
        if ((t_negative && t_result > INTEGER_MAX) || (!t_negative && t_result > abs(INTEGER_MIN)))
        {
            // overflow
        }
        else
            r_output = t_negative ? -t_result : t_result;
    }
}

void MCMathEvalConvertFromBase10(integer_t p_operand, integer_t p_dest_base, MCStringRef& r_output)
{
    // p_dest_base must be an integer between 2 and 32
    if (p_operand < 0)
    {
        if (MCMathConvertFromBase10(-p_operand, true, p_dest_base, r_output))
            return;
    }
    else
    {
        if (MCMathConvertFromBase10(p_operand, false, p_dest_base, r_output))
            return;
    }
    
//    ctxt . Throw();
}

void MCMathEvalConvertBase(MCStringRef p_operand, integer_t p_source_base, integer_t p_dest_base, MCStringRef& r_output)
{
    // p_source_base and p_dest_base must be integers between 2 and 32
    bool t_negative;
    uinteger_t t_result;
    bool t_error;
    if (MCMathConvertToBase10(p_operand, p_source_base, t_negative, t_result, t_error))
    {
        if (MCMathConvertFromBase10(t_result, t_negative, p_dest_base, r_output))
            return;
    }
    
//    ctxt . Throw();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
