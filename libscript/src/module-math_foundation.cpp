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

void MCMathFoundationExecRoundToNearestReal(double& x_target)
{
	if (x_target < 0.0)
        x_target = ceil(x_target - 0.5);
	else
        x_target = floor(x_target + 0.5);
}

void MCMathFoundationExecRoundDownNumber(MCNumberRef& x_target)
{
    double t_target = MCNumberFetchAsReal(x_target);
    MCMathFoundationExecRoundDownReal(t_target);
    
    MCValueRelease(x_target);
    MCNumberCreateWithReal(t_target, x_target);
}

void MCMathFoundationExecRoundUpNumber(MCNumberRef& x_target)
{
    double t_target = MCNumberFetchAsReal(x_target);
    MCMathFoundationExecRoundUpReal(t_target);
    
    MCValueRelease(x_target);
    MCNumberCreateWithReal(t_target, x_target);
}

void MCMathFoundationExecRoundToNearestNumber(MCNumberRef& x_target)
{
    double t_target = MCNumberFetchAsReal(x_target);
    MCMathFoundationExecRoundToNearestReal(t_target);
    
    MCValueRelease(x_target);
    MCNumberCreateWithReal(t_target, x_target);
}

void MCMathFoundationEvalFloorReal(double& x_target)
{
    x_target = floor(x_target);
}

void MCMathFoundationEvalCeilingReal(double& x_target)
{
    x_target = ceil(x_target);
}

void MCMathFoundationEvalFloorNumber(MCNumberRef& x_target)
{
    double t_target = MCNumberFetchAsReal(x_target);
    MCMathFoundationEvalFloorReal(t_target);
    
    MCValueRelease(x_target);
    MCNumberCreateWithReal(t_target, x_target);
}

void MCMathFoundationEvalCeilingNumber(MCNumberRef& x_target)
{
    double t_target = MCNumberFetchAsReal(x_target);
    MCMathFoundationEvalCeilingReal(t_target);
    
    MCValueRelease(x_target);
    MCNumberCreateWithReal(t_target, x_target);
}