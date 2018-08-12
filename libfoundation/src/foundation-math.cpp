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

#include <foundation-math.h>

#include "foundation-auto.h"
#include "foundation-private.h"

////////////////////////////////////////////////////////////////////////////////

bool MCMathConvertFromBase10(uint32_t p_value, bool p_negative, integer_t p_dest_base, MCStringRef& r_result)
{
    char_t result[64];
    char_t *dptr = &result[63];
    do
    {
        uint16_t digit = p_value % p_dest_base;
        p_value /= p_dest_base;
        if (digit >= 10)
            *dptr-- = digit - 10 + 'A';
        else
            *dptr-- = digit + '0';
    }
    while (p_value);
    if (p_negative)
        *dptr-- = '-';
    dptr++;
    
    return MCStringCreateWithNativeChars(dptr, 64 - (dptr - result), r_result);
}

bool MCMathConvertToBase10(MCStringRef p_source, integer_t p_source_base, bool& r_negative, uinteger_t& r_result, bool& r_error)
{
    uint32_t t_value;
    t_value = 0;
    
    bool t_negative;
    t_negative = false;
    
    MCAutoStringRefAsNativeChars t_auto_native;
    const char_t* t_native;
    uindex_t t_length;
    
    if (!t_auto_native . Lock(p_source, t_native, t_length))
        return false;
    
    // MW-2008-01-31: [[ Bug 5841 ]] Added in some more strict error checking to
    //   stop baseConvert attempting to convert strings with digits outside the
    //   source-base.
    bool t_error;
    t_error = false;
    
    if (!t_error && t_length == 0)
        t_error = true;
    
    uint32_t i;
    if (!t_error)
    {
        if (t_native[0] == '+')
            i = 1;
        else if (t_native[0] == '-')
        {
            i = 1;
            t_negative = true;
        }
        else
            i = 0;
    }
    
    while(!t_error && i < t_length)
    {
        t_value *= p_source_base;
        char_t source = MCNativeCharUppercase(t_native[i]);
        if (isdigit((uint8_t)source))
        {
            if (source - '0' >= p_source_base)
                t_error = true;
            else
                t_value += source - '0';
        }
        else if (source >= 'A' && source < 'A' + p_source_base - 10)
            t_value += source - 'A' + 10;
        else
            t_error = true;
        
        i += 1;
    }
    
    if (t_error)
    {
        r_error = true;
        return false;
    }
    
    r_negative = t_negative;
    r_result = t_value;
    return true;
}

////////////////////////////////////////////////////////////////////////////////

bool __MCMathInitialize()
{
    return true;
}

void __MCMathFinalize()
{
    
}

////////////////////////////////////////////////////////////////////////////////
