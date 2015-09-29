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

#include "foundation.h"

extern "C" MC_DLLEXPORT_DEF void MCLogicEvalNot(bool p_bool, bool& r_result)
{
    r_result = !p_bool;
}

extern "C" MC_DLLEXPORT_DEF void MCLogicEvalIsEqualTo(bool p_left, bool p_right, bool& r_result)
{
    r_result = (p_left == p_right);
}

extern "C" MC_DLLEXPORT_DEF void MCLogicEvalIsNotEqualTo(bool p_left, bool p_right, bool& r_result)
{
    r_result = (p_left != p_right);
}

// AL-2015-02-10: [[ Bug 14538 ]] Native function named incorrectly
extern "C" MC_DLLEXPORT_DEF MCStringRef MCLogicExecFormatBoolAsString(bool p_operand)
{
    return MCValueRetain(p_operand ? kMCTrueString : kMCFalseString);
}

extern "C" MC_DLLEXPORT_DEF MCValueRef MCLogicExecParseStringAsBool(MCStringRef p_operand)
{
    if (MCStringIsEqualTo(p_operand, kMCTrueString, kMCStringOptionCompareCaseless))
        return MCValueRetain(kMCTrue);
    else if (MCStringIsEqualTo(p_operand, kMCFalseString, kMCStringOptionCompareCaseless))
        return MCValueRetain(kMCFalse);
    else
        return MCValueRetain(kMCNull);
}

// AL-2015-02-10: [[ Bug 14538 ]] Native function named incorrectly
extern "C" MC_DLLEXPORT_DEF void MCLogicEvalBoolFormattedAsString(bool p_operand, MCStringRef& r_output)
{
    r_output = MCLogicExecFormatBoolAsString(p_operand);
}

extern "C" MC_DLLEXPORT_DEF void MCLogicEvalStringParsedAsBool(MCStringRef p_operand, MCValueRef& r_output)
{
    r_output = MCLogicExecParseStringAsBool(p_operand);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

extern "C" bool com_livecode_logic_Initialize(void)
{
    return true;
}

extern "C" void com_livecode_logic_Finalize(void)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
