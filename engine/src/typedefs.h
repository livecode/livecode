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
