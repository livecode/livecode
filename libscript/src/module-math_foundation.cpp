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

void MCMathFoundationExecRoundDownReal(double& x_target)
{
    x_target = floor(x_target + 0.5);
}

void MCMathFoundationExecRoundUpReal(double& x_target)
{
    x_target = ceil(x_target - 0.5);
}

extern "C" void MCMathFoundationExecRoundToNearestReal(double& x_target)
{
	if (x_target < 0.0)
        x_target = ceil(x_target - 0.5);
	else
        x_target = floor(x_target + 0.5);
}

extern "C" void MCMathFoundationExecRoundDownNumber(MCNumberRef& x_target)
{
    double t_target = MCNumberFetchAsReal(x_target);
    MCMathFoundationExecRoundDownReal(t_target);
    
    MCValueRelease(x_target);
    MCNumberCreateWithReal(t_target, x_target);
}

extern "C" void MCMathFoundationExecRoundUpNumber(MCNumberRef& x_target)
{
    double t_target = MCNumberFetchAsReal(x_target);
    MCMathFoundationExecRoundUpReal(t_target);
    
    MCValueRelease(x_target);
    MCNumberCreateWithReal(t_target, x_target);
}

extern "C" void MCMathFoundationExecRoundToNearestNumber(MCNumberRef& x_target)
{
    double t_target = MCNumberFetchAsReal(x_target);
    MCMathFoundationExecRoundToNearestReal(t_target);
    
    MCValueRelease(x_target);
    MCNumberCreateWithReal(t_target, x_target);
}

extern "C" void MCMathFoundationEvalFloorReal(double p_target, double& r_output)
{
    r_output = floor(p_target);
}

extern "C" void MCMathFoundationEvalCeilingReal(double p_target, double& r_output)
{
    r_output = ceil(p_target);
}

extern "C" void MCMathFoundationEvalFloorNumber(MCNumberRef p_target, MCNumberRef& r_output)
{
    double t_target = MCNumberFetchAsReal(p_target);
    MCMathFoundationEvalFloorReal(t_target, t_target);
    
    if (!MCNumberCreateWithReal(t_target, r_output))
        return;
}

void MCMathFoundationEvalCeilingNumber(MCNumberRef p_target, MCNumberRef& r_output)
{
    double t_target = MCNumberFetchAsReal(p_target);
    MCMathFoundationEvalCeilingReal(t_target, t_target);
    
    if (!MCNumberCreateWithReal(t_target, r_output))
        return;
}