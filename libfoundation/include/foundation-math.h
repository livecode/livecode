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

#ifndef __MC_FOUNDATION_MATH__
#define __MC_FOUNDATION_MATH__

#ifndef __MC_FOUNDATION__
#include <foundation.h>
#endif

////////////////////////////////////////////////////////////////////////////////

void MCMathRandomSeed();
real64_t MCMathRandom();

// Convert from base 10 to a string in the desired base. Destination base should be between 2 and 32 inclusive.
bool MCMathConvertFromBase10(uint32_t p_value, bool p_negative, integer_t p_dest_base, MCStringRef& r_result);

// Convert from string in the specified base to base 10. Source base should be between 2 and 32 inclusive.
// error is true if there were characters in the string outside the source base.
bool MCMathConvertToBase10(MCStringRef p_source, integer_t p_source_base, bool& r_negative, uinteger_t& r_result, bool& r_error);

////////////////////////////////////////////////////////////////////////////////

#endif
