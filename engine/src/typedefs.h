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

#ifndef __MC_TYPEDEFS__
#define __MC_TYPEDEFS__

// IEEE floating-point limits

#ifndef DBL_DIG
#define DBL_DIG         15                      /* # of decimal digits of precision */
#define DBL_EPSILON     2.2204460492503131e-016 /* smallest such that 1.0+DBL_EPSILON != 1.0 */
#define DBL_MANT_DIG    53                      /* # of bits in mantissa */
#define DBL_MAX         1.7976931348623158e+308 /* max value */
#define DBL_MAX_10_EXP  308                     /* max decimal exponent */
#define DBL_MAX_EXP     1024                    /* max binary exponent */
#define DBL_MIN         2.2250738585072014e-308 /* min positive value */
#define DBL_MIN_10_EXP  (-307)                  /* min decimal exponent */
#define DBL_MIN_EXP     (-1021)                 /* min binary exponent */
#define _DBL_RADIX      2                       /* exponent radix */
#define _DBL_ROUNDS     1                       /* addition rounding: near */
#endif

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

// New-style (C99) integer definitions and limits


#ifndef _WIN32
#include <stdint.h>
#else

typedef unsigned char uint8_t;
typedef signed char int8_t;
typedef unsigned short uint16_t;
typedef signed short int16_t;
typedef unsigned int uint32_t;
typedef signed int int32_t;

// MDW-2013-04-15: [[ x64 ]] added 64-bit-safe typedefs
#if !defined(uint64_t)
	#ifndef __LP64__
		typedef unsigned long long int uint64_t;
	#else
		typedef unsigned long int uint64_t;
	#endif
#endif
#if !defined(int64_t)
	#ifndef __LP64__
		typedef signed long long int int64_t;
	#else
		typedef signed long int int64_t;
	#endif
#endif

#endif

typedef float float32_t;
typedef double float64_t;

typedef float real32_t;
typedef double real64_t;

typedef uint32_t uindex_t;
typedef int32_t index_t;

typedef uint32_t codepoint_t;

#ifndef UINT8_MAX
#define UINT8_MIN 0
#define UINT8_MAX 255

#define INT8_MIN -128
#define INT8_MAX 127

#define UINT16_MIN 0
#define UINT16_MAX 65535

#define INT16_MIN -32768
#define INT16_MAX 32767

#define UINT32_MIN 0
#define UINT32_MAX 0xFFFFFFFFU

#define INT32_MIN -2147483647
#define INT32_MAX 0x7FFFFFFF

#define UINT64_MIN 0
#define UINT64_MAX 0xFFFFFFFFFFFFFFFFULL

#define INT64_MIN 
#define INT64_MAX 0x7FFFFFFFFFFFFFFFLL
#endif

// These are non-standard
#define UINT8_MIN   0
#define UINT16_MIN  0
#define UINT32_MIN  0
#define UINT64_MIN  0

#define UINDEX_MIN UINT32_MIN
#define UINDEX_MAX UINT32_MAX

// Pointer defines

// MDW-2013-04-15: [[ x64 ]] added 64-bit-safe typedefs
#ifndef _UINTPTR_T
	#define _UINTPTR_T
	#ifdef __LP64__
		typedef uint64_t uintptr_t;
	#else
		typedef uint32_t uintptr_t;
	#endif
#endif

#ifndef _INTPTR_T
	#define _INTPTR_T
	#ifdef __LP64__
		typedef int64_t intptr_t;
	#else
		typedef int32_t intptr_t;
	#endif
#endif

// Null pointer defines (nil is preferred)
// PM-2015-03-31: [[ Bug 15090 ]] Better definition of nil/NULL fixes crashes in 64 bit iOS
#ifndef NULL

#if defined(__cplusplus) /* C++ */
#	if defined(__GCC__)
#		define NULL __null
#	else
#		define NULL uintptr_t(0)
#	endif

#else /* C */
#	if defined(__GCC__)
#		define NULL __null
#	else
#		define NULL ((void*)0)
#	endif

#endif


#endif

#ifndef nil
#   define nil NULL
#endif

// Boolean definitions

typedef unsigned char Boolean;

#ifndef Bool
#define Bool int
#endif

#define False 0
#define True 1
#define Mixed 2

// Coordinates
typedef float32_t coord_t;

#endif
