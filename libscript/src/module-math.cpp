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

extern "C" MC_DLLEXPORT void MCMathEvalRealToPowerOfReal(double p_left, double p_right, double& r_output)
{
    if (p_right == 0)
    {
        r_output = 1.0;
        return;
    }

    r_output = pow(p_left, p_right);
}

extern "C" MC_DLLEXPORT void MCMathEvalNumberToPowerOfNumber(MCNumberRef p_left, MCNumberRef p_right, MCNumberRef& r_output)
{
    double t_left, t_right;
    t_left = MCNumberFetchAsReal(p_left);
    t_right = MCNumberFetchAsReal(p_right);
    
    double t_result;
    MCMathEvalRealToPowerOfReal(t_left, t_right, t_result);
    
    // if (!ctxt . HasError())
    MCNumberCreateWithReal(t_result, r_output);
}

extern "C" MC_DLLEXPORT void MCMathEvalBase10LogReal(double p_operand, double& r_output)
{
    r_output = log10(p_operand);
}

extern "C" MC_DLLEXPORT void MCMathEvalBase10LogNumber(MCNumberRef p_operand, MCNumberRef& r_output)
{
    double t_operand;
    t_operand = MCNumberFetchAsReal(p_operand);
    
    double t_result;
    MCMathEvalBase10LogReal(t_operand, t_result);
    
    // if (!ctxt . HasError())
    MCNumberCreateWithReal(t_result, r_output);
}

extern "C" MC_DLLEXPORT void MCMathEvalNaturalLogReal(double p_operand, double& r_output)
{
    r_output = log(p_operand);
}

extern "C" MC_DLLEXPORT void MCMathEvalNaturalLogNumber(MCNumberRef p_operand, MCNumberRef& r_output)
{
    double t_operand;
    t_operand = MCNumberFetchAsReal(p_operand);
    
    double t_result;
    MCMathEvalNaturalLogReal(t_operand, t_result);
    
    // if (!ctxt . HasError())
    MCNumberCreateWithReal(t_result, r_output);
}

extern "C" MC_DLLEXPORT void MCMathEvalExpReal(double p_operand, double& r_output)
{
    r_output = exp(p_operand);
}

extern "C" MC_DLLEXPORT void MCMathEvalExpNumber(MCNumberRef p_operand, MCNumberRef& r_output)
{
    double t_operand;
    t_operand = MCNumberFetchAsReal(p_operand);
    
    double t_result;
    MCMathEvalExpReal(t_operand, t_result);
    
    // if (!ctxt . HasError())
    MCNumberCreateWithReal(t_result, r_output);
}

extern "C" MC_DLLEXPORT void MCMathEvalSinReal(double p_operand, double& r_output)
{
    r_output = sin(p_operand);
}

extern "C" MC_DLLEXPORT void MCMathEvalSinNumber(MCNumberRef p_operand, MCNumberRef& r_output)
{
    double t_operand;
    t_operand = MCNumberFetchAsReal(p_operand);
    
    double t_result;
    MCMathEvalSinReal(t_operand, t_result);
    
    // if (!ctxt . HasError())
    MCNumberCreateWithReal(t_result, r_output);
}

extern "C" MC_DLLEXPORT void MCMathEvalCosReal(double p_operand, double& r_output)
{
    r_output = cos(p_operand);
}

extern "C" MC_DLLEXPORT void MCMathEvalCosNumber(MCNumberRef p_operand, MCNumberRef& r_output)
{
    double t_operand;
    t_operand = MCNumberFetchAsReal(p_operand);
    
    double t_result;
    MCMathEvalCosReal(t_operand, t_result);
    
    // if (!ctxt . HasError())
    MCNumberCreateWithReal(t_result, r_output);
}

extern "C" MC_DLLEXPORT void MCMathEvalTanReal(double p_operand, double& r_output)
{
    r_output = tan(p_operand);
}

extern "C" MC_DLLEXPORT void MCMathEvalTanNumber(MCNumberRef p_operand, MCNumberRef& r_output)
{
    double t_operand;
    t_operand = MCNumberFetchAsReal(p_operand);
    
    double t_result;
    MCMathEvalTanReal(t_operand, t_result);
    
    // if (!ctxt . HasError())
    MCNumberCreateWithReal(t_result, r_output);
}

extern "C" MC_DLLEXPORT void MCMathEvalAbsInteger(integer_t p_operand, integer_t& r_output)
{
    r_output = MCAbs(p_operand);
}

extern "C" MC_DLLEXPORT void MCMathEvalAbsReal(double p_operand, double& r_output)
{
    r_output = MCAbs(p_operand);
}

extern "C" MC_DLLEXPORT void MCMathEvalAbsNumber(MCNumberRef p_operand, MCNumberRef& r_output)
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

extern "C" MC_DLLEXPORT void MCMathEvalTruncInteger(integer_t p_operand, integer_t& r_output)
{
    r_output = p_operand;
}

extern "C" MC_DLLEXPORT void MCMathEvalTruncReal(double p_operand, double& r_output)
{
    r_output = trunc(p_operand);
}

extern "C" MC_DLLEXPORT void MCMathEvalTruncNumber(MCNumberRef p_operand, MCNumberRef& r_output)
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

extern "C" MC_DLLEXPORT void MCMathEvalMinInteger(integer_t p_left, integer_t p_right, integer_t& r_output)
{
    r_output = MCMin(p_left, p_right);
}

extern "C" MC_DLLEXPORT void MCMathEvalMinReal(double p_left, double p_right, double& r_output)
{
    r_output = MCMin(p_left, p_right);
}

extern "C" MC_DLLEXPORT void MCMathEvalMinNumber(MCNumberRef p_left, MCNumberRef p_right, MCNumberRef& r_output)
{
    double t_left, t_right, t_result;
    t_left = MCNumberFetchAsReal(p_left);
    t_right = MCNumberFetchAsReal(p_right);
    
    MCMathEvalMinReal(t_left, t_right, t_result);
    
    MCNumberCreateWithReal(t_result, r_output);
}

extern "C" MC_DLLEXPORT void MCMathEvalMaxInteger(integer_t p_left, integer_t p_right, integer_t& r_output)
{
    r_output = MCMax(p_left, p_right);
}

extern "C" MC_DLLEXPORT void MCMathEvalMaxReal(double p_left, double p_right, double& r_output)
{
    r_output = MCMax(p_left, p_right);
}

extern "C" MC_DLLEXPORT void MCMathEvalMaxNumber(MCNumberRef p_left, MCNumberRef p_right, MCNumberRef& r_output)
{
    double t_left, t_right, t_result;
    t_left = MCNumberFetchAsReal(p_left);
    t_right = MCNumberFetchAsReal(p_right);
    
    MCMathEvalMaxReal(t_left, t_right, t_result);
    
    MCNumberCreateWithReal(t_result, r_output);
}

extern "C" MC_DLLEXPORT void MCMathEvalRandomReal(double& r_output)
{
    r_output = MCMathRandom();
}

////////////////////////////////////////////////////////////////////////////////////////////////////

extern "C" MC_DLLEXPORT void MCMathEvalConvertToBase10(MCStringRef p_operand, integer_t p_source_base, integer_t& r_output)
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

extern "C" MC_DLLEXPORT void MCMathEvalConvertFromBase10(integer_t p_operand, integer_t p_dest_base, MCStringRef& r_output)
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

extern "C" MC_DLLEXPORT void MCMathEvalConvertBase(MCStringRef p_operand, integer_t p_source_base, integer_t p_dest_base, MCStringRef& r_output)
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
