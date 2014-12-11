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

#include "foundation.h"

extern "C" MC_DLLEXPORT void MCLogicEvalNot(bool p_bool, bool& r_result)
{
    r_result = !p_bool;
}

extern "C" MC_DLLEXPORT void MCLogicEvalIsEqualTo(bool p_left, bool p_right, bool& r_result)
{
    r_result = (p_left == p_right);
}

extern "C" MC_DLLEXPORT void MCLogicEvalIsNotEqualTo(bool p_left, bool p_right, bool& r_result)
{
    r_result = (p_left != p_right);
}

extern "C" MC_DLLEXPORT void MCLogicEvalBooleanFormattedAsString(bool p_operand, MCStringRef& r_output)
{
    r_output = MCValueRetain(p_operand ? kMCTrueString : kMCFalseString);
}

extern "C" MC_DLLEXPORT void MCLogicEvalStringParsedAsBoolean(MCStringRef p_operand, bool& r_output)
{
    if (MCStringIsEqualTo(p_operand, kMCTrueString, kMCStringOptionCompareCaseless))
        r_output = true;
    else if (MCStringIsEqualTo(p_operand, kMCFalseString, kMCStringOptionCompareCaseless))
        r_output = false;
    else
        // Throw
        return;
}