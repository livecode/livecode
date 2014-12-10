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

extern "C" MC_DLLEXPORT void MCBitwiseEvalBitwiseAnd(integer_t p_left, integer_t p_right, integer_t& r_output)
{
    r_output = p_left & p_right;
}

extern "C" MC_DLLEXPORT void MCBitwiseEvalBitwiseOr(integer_t p_left, integer_t p_right, integer_t& r_output)
{
    r_output = p_left | p_right;
}

extern "C" MC_DLLEXPORT void MCBitwiseEvalBitwiseXor(integer_t p_left, integer_t p_right, integer_t& r_output)
{
    r_output = p_left ^ p_right;
}

extern "C" MC_DLLEXPORT void MCBitwiseEvalBitwiseNot(integer_t p_operand, integer_t& r_output)
{
    r_output = ~((int32_t)p_operand);
}

extern "C" MC_DLLEXPORT void MCBitwiseEvalBitwiseShift(integer_t p_operand, integer_t p_shift, integer_t& r_output)
{
    if (p_shift < 0)
        // truncate towards 0
        r_output = (integer_t)(p_operand >> abs(p_shift));
    else
        r_output = p_operand << p_shift;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef _TEST
void MCBitwiseRunTests()
{
    
}
#endif

