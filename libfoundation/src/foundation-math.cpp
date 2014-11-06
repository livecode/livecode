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

#include "foundation-auto.h"
#include "foundation-private.h"

////////////////////////////////////////////////////////////////////////////////

static uint32_t s_random_seed;

////////////////////////////////////////////////////////////////////////////////

#define N       16
#define RMASK    ((uint32_t)(1 << (N - 1)) + (1 << (N - 1)) - 1)
#define LOW(x)  ((uint32_t)(x) & RMASK)
#define HIGH(x) LOW((x) >> N)
#define MUL(x, y, z)    { uint32_t l = (uint32_t)(x) * (uint32_t)(y); \
(z)[0] = LOW(l); (z)[1] = HIGH(l); }
#define CARRY(x, y)     ((uint32_t)(x) + (uint32_t)(y) > RMASK)
#define ADDEQU(x, y, z) (z = CARRY(x, (y)), x = LOW(x + (y)))
#define X0      0x330E
#define X1      0xABCD
#define X2      0x1234
#define A0      0xE66D
#define A1      0xDEEC
#define A2      0x5
#define C       0xB
#define SET3(x, x0, x1, x2)     ((x)[0] = (x0), (x)[1] = (x1), (x)[2] = (x2))
#define SEED(x0, x1, x2) (SET3(x, x0, x1, x2), SET3(a, A0, A1, A2), c = C)

static uint32_t x[3] = { X0, X1, X2 }, a[3] = { A0, A1, A2 }, c = C;

void MCMathRandomSeed()
{
    SEED(X0, LOW(s_random_seed), HIGH(s_random_seed));
}

real64_t MCMathRandom()
{
    static real64_t two16m = 1.0 / (1L << N);
    uint32_t p[2], q[2], r[2], carry0, carry1;
    
    MUL(a[0], x[0], p);
    ADDEQU(p[0], c, carry0);
    ADDEQU(p[1], carry0, carry1);
    MUL(a[0], x[1], q);
    ADDEQU(p[1], q[0], carry0);
    MUL(a[1], x[0], r);
    x[2] = LOW(carry0 + carry1 + CARRY(p[1], r[0]) + q[1] + r[1] +
               a[0] * x[2] + a[1] * x[1] + a[2] * x[0]);
    x[1] = LOW(p[1] + r[0]);
    x[0] = LOW(p[0]);
    return (two16m * (two16m * (two16m * x[0] + x[1]) + x[2]));
}

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
    char_t* t_native;
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
    // initialise random seed
    // s_random_seed = (int32_t)(intptr_t)&MCdispatcher + MCS_getpid() + (int4)time(NULL);
    MCMathRandomSeed();
    
    return true;
}

void __MCMathFinalize()
{
    
}

////////////////////////////////////////////////////////////////////////////////
