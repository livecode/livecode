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

extern "C" MC_DLLEXPORT void MCMathFoundationExecRoundRealToNearest(double& x_target)
{
	if (x_target < 0.0)
        x_target = ceil(x_target - 0.5);
	else
        x_target = floor(x_target + 0.5);
}

extern "C" MC_DLLEXPORT void MCMathFoundationExecRoundNumberToNearest(MCNumberRef& x_target)
{
    double t_target = MCNumberFetchAsReal(x_target);
    MCMathFoundationExecRoundRealToNearest(t_target);
    
    MCAutoNumberRef t_new_number;
    if (!MCNumberCreateWithReal(t_target, &t_new_number))
        return;
    
    MCValueAssign(x_target, *t_new_number);
}

extern "C" MC_DLLEXPORT void MCMathFoundationEvalRoundedRealToNearest(double p_target, double& r_output)
{
    MCMathFoundationExecRoundRealToNearest(p_target);
    r_output = p_target;
}

extern "C" MC_DLLEXPORT void MCMathFoundationEvalRoundedNumberToNearest(MCNumberRef p_target, MCNumberRef& r_output)
{
    double t_target = MCNumberFetchAsReal(p_target);
    MCMathFoundationExecRoundRealToNearest(t_target);
    
    if (!MCNumberCreateWithReal(t_target, r_output))
        return;
}

extern "C" MC_DLLEXPORT void MCMathFoundationEvalFloorReal(double p_target, double& r_output)
{
    r_output = floor(p_target);
}

extern "C" MC_DLLEXPORT void MCMathFoundationEvalCeilingReal(double p_target, double& r_output)
{
    r_output = ceil(p_target);
}

extern "C" MC_DLLEXPORT void MCMathFoundationEvalFloorNumber(MCNumberRef p_target, MCNumberRef& r_output)
{
    double t_target = MCNumberFetchAsReal(p_target);
    MCMathFoundationEvalFloorReal(t_target, t_target);
    
    if (!MCNumberCreateWithReal(t_target, r_output))
        return;
}

extern "C" MC_DLLEXPORT void MCMathFoundationEvalCeilingNumber(MCNumberRef p_target, MCNumberRef& r_output)
{
    double t_target = MCNumberFetchAsReal(p_target);
    MCMathFoundationEvalCeilingReal(t_target, t_target);
    
    if (!MCNumberCreateWithReal(t_target, r_output))
        return;
}

extern "C" MC_DLLEXPORT void MCMathFoundationEvalPi(double& r_output)
{
    r_output = 3.141592653589793238462643;
}