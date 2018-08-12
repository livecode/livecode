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

#ifndef __MC_FOUNDATION_STDLIB__
#define __MC_FOUNDATION_STDLIB__

////////////////////////////////////////////////////////////////////////////////

#ifdef __WINDOWS__

#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <float.h>

extern "C" __declspec(noalias) void __cdecl free(void *memory);
extern "C" __declspec(noalias) void * __cdecl malloc(size_t size);
extern "C" __declspec(noalias) void * __cdecl realloc(void *memory, size_t size);

extern "C" unsigned long __cdecl strtoul(const char *str, char **endptr, int radix);
extern "C" long __cdecl strtol(const char *str, char **endptr, int radix);
extern "C" double __cdecl strtod(const char *str, char **endptr);

extern "C" int __cdecl _vscprintf(const char *format, va_list args);
extern "C" int __cdecl vsprintf(char *string, const char *format, va_list args);
extern "C" int __cdecl sprintf(char *string, const char *format, ...);

extern "C" void __cdecl qsort(void *, size_t, size_t, int(__cdecl*)(const void *, const void *));

extern "C" double __cdecl floor(double x);
extern "C" double __cdecl fmod(double x, double y);

#else

#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <ctype.h>
#include <float.h>

#endif

////////////////////////////////////////////////////////////////////////////////

#ifndef _MSC_VER

// Work-around for the Android headers being strict
#if !defined(va_copy) && defined(__va_copy)
#  define va_copy __va_copy
#endif

#endif

////////////////////////////////////////////////////////////////////////////////

#endif

