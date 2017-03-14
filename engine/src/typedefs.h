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

#ifndef __MC_TYPEDEFS__
#define __MC_TYPEDEFS__

// IEEE floating-point limits

#ifndef DBL_EPSILON
#define DBL_EPSILON     2.2204460492503131e-016 /* smallest such that 1.0+DBL_EPSILON != 1.0 */
#endif

#ifndef DBL_DIG
#define DBL_DIG         15                      /* # of decimal digits of precision */
#define DBL_MANT_DIG    53                      /* # of bits in mantissa */
#define DBL_MAX         1.7976931348623158e+308 /* max value */
#define DBL_MAX_10_EXP  308                     /* max decimal exponent */
#define DBL_MAX_EXP     1024                    /* max binary exponent */
#define DBL_MIN         2.2250738585072014e-308 /* min positive value */
#define DBL_MIN_10_EXP  (-307)                  /* min decimal exponent */
#define DBL_MIN_EXP     (-1021)                 /* min binary exponent */
#endif

#define _DBL_RADIX      2                       /* exponent radix */
#define _DBL_ROUNDS     1                       /* addition rounding: near */

#ifndef FLT_DIG
#define FLT_DIG         6                       /* # of decimal digits of precision */
#define FLT_EPSILON     1.192092896e-07F        /* smallest such that 1.0+FLT_EPSILON != 1.0 */
#define FLT_GUARD       0
#define FLT_MANT_DIG    24                      /* # of bits in mantissa */
#define FLT_MAX         3.402823466e+38F        /* max value */
#define FLT_MAX_10_EXP  38                      /* max decimal exponent */
#define FLT_MAX_EXP     128                     /* max binary exponent */
#define FLT_MIN         1.175494351e-38F        /* min positive value */
#define FLT_MIN_10_EXP  (-37)                   /* min decimal exponent */
#define FLT_MIN_EXP     (-125)                  /* min binary exponent */
#define FLT_NORMALIZE   0
#define FLT_RADIX       2                       /* exponent radix */
#define FLT_ROUNDS      1                       /* addition rounding: near */
#endif

// Old-style integer definitions and limits

typedef unsigned char   uint1;
typedef signed   char   int1;
typedef unsigned short  uint2;
typedef          short  int2;
typedef unsigned int    uint4;
typedef          int    int4;
typedef float           real4;
typedef double          real8;

#define MAXUINT1 0xFFU
#define MAXINT1  0x7F
#define MAXUINT2 0xFFFFU
#define MAXINT2  0x7FFF
#define MININT2  -32768
#define MAXUINT4 0xFFFFFFFFU
#define MAXINT4  0x7FFFFFFF
#define MININT4  -2147483647

#define MAXREAL8 DBL_MAX

#define U1L 8
#define I1L 8
#define U2L 8
#define I2L 8
#define U4L 16
#define I4L 16
#define U8L 32
#define U8L 32
#define R4L 64
#define R8L 384

// Boolean definitions

#ifndef __MACTYPES__
typedef bool Boolean;
#endif
static const bool True = true;
static const bool False = false;

#ifndef Bool
#define Bool int
#endif

#endif
