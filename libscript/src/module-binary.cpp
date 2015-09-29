/* Copyright (C) 2003-2015 LiveCode Ltd.
 
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
#include <foundation-chunk.h>

extern "C" MC_DLLEXPORT_DEF void MCBinaryEvalConcatenateBytes(MCDataRef p_left, MCDataRef p_right, MCDataRef& r_output)
{
    MCAutoDataRef t_data;
    if (!MCDataMutableCopy(p_left, &t_data))
        return;
    
    if (!MCDataAppend(*t_data, p_right))
        return;
    
    if (!MCDataCopy(*t_data, r_output))
        return;
}

extern "C" MC_DLLEXPORT_DEF void MCBinaryExecPutBytesBefore(MCDataRef p_source, MCDataRef& x_target)
{
    MCAutoDataRef t_data;
    MCBinaryEvalConcatenateBytes(p_source, x_target == (MCDataRef)kMCNull ? kMCEmptyData : x_target, &t_data);
    
    if (MCErrorIsPending())
        return;
    
    MCValueAssign(x_target, *t_data);
}

extern "C" MC_DLLEXPORT_DEF void MCBinaryExecPutBytesAfter(MCDataRef p_source, MCDataRef& x_target)
{
    MCAutoDataRef t_data;
    MCBinaryEvalConcatenateBytes(x_target == (MCDataRef)kMCNull ? kMCEmptyData : x_target, p_source, &t_data);
    
    if (MCErrorIsPending())
        return;
    
    MCValueAssign(x_target, *t_data);
}

extern "C" MC_DLLEXPORT_DEF void MCBinaryEvalIsEqualTo(MCDataRef p_left, MCDataRef p_right, bool& r_result)
{
    r_result = MCDataIsEqualTo(p_left, p_right);
}

extern "C" MC_DLLEXPORT_DEF void MCBinaryEvalIsNotEqualTo(MCDataRef p_left, MCDataRef p_right, bool& r_result)
{
    r_result = !MCDataIsEqualTo(p_left, p_right);
}

extern "C" MC_DLLEXPORT_DEF void MCBinaryEvalIsLessThan(MCDataRef p_left, MCDataRef p_right, bool& r_result)
{
    r_result = MCDataCompareTo(p_left, p_right) < 0;
}

extern "C" MC_DLLEXPORT_DEF void MCBinaryEvalIsGreaterThan(MCDataRef p_left, MCDataRef p_right, bool& r_result)
{
    r_result = MCDataCompareTo(p_left, p_right) > 0;
}

extern "C" MC_DLLEXPORT_DEF void MCDataEvalEmpty(MCDataRef& r_output)
{
    r_output = MCValueRetain(kMCEmptyData);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

extern "C" bool com_livecode_binary_Initialize(void)
{
    return true;
}

extern "C" void com_livecode_binary_Finalize(void)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
