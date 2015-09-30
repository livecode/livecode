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

extern "C" MC_DLLEXPORT_DEF void MCBitwiseEvalBitwiseAnd(integer_t p_left, integer_t p_right, integer_t& r_output)
{
    r_output = p_left & p_right;
}

extern "C" MC_DLLEXPORT_DEF void MCBitwiseEvalBitwiseOr(integer_t p_left, integer_t p_right, integer_t& r_output)
{
    r_output = p_left | p_right;
}

extern "C" MC_DLLEXPORT_DEF void MCBitwiseEvalBitwiseXor(integer_t p_left, integer_t p_right, integer_t& r_output)
{
    r_output = p_left ^ p_right;
}

extern "C" MC_DLLEXPORT_DEF void MCBitwiseEvalBitwiseNot(integer_t p_operand, integer_t& r_output)
{
    r_output = ~p_operand;
}

/* Compute the maximum shift possible in C for a given operand. */
template <typename T>
static inline void
MCBitwiseEvalBitwiseShiftCount (T p_operand, uinteger_t & p_shift)
{
	/* Maximum shift count for which the C operator is defined */
	uinteger_t t_max_shift = (sizeof(T) << 3) - 1;
	p_shift = MCMin (p_shift, t_max_shift);
}

extern "C" MC_DLLEXPORT_DEF void
MCBitwiseEvalBitwiseShiftRight (integer_t p_operand,
                                uinteger_t p_shift,
                                integer_t & r_output)
{
	MCBitwiseEvalBitwiseShiftCount (p_operand, p_shift);
	r_output = p_operand >> p_shift;
}

extern "C" MC_DLLEXPORT_DEF void
MCBitwiseEvalBitwiseShiftLeft (integer_t p_operand,
                               uinteger_t p_shift,
                               integer_t& r_output)
{
	MCBitwiseEvalBitwiseShiftCount (p_operand, p_shift);
	integer_t t_shifted = p_operand << p_shift;

	/* Overflow check */
	if (p_operand != t_shifted >> p_shift)
	{
		MCErrorCreateAndThrow (kMCGenericErrorTypeInfo, "reason",
		                       MCSTR("overflow in bitwise operation"), nil);
		return;
	}

	r_output = t_shifted;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

extern "C" bool com_livecode_bitwise_Initialize(void)
{
    return true;
}

extern "C" void com_livecode_bitwise_Finalize(void)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef _TEST
void MCBitwiseRunTests()
{
    
}
#endif

