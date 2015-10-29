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

extern "C" __declspec(noalias) void __cdecl free(void *memory);
extern "C" __declspec(noalias) void * __cdecl malloc(size_t size);
extern "C" __declspec(noalias) void * __cdecl realloc(void *memory, size_t size);

extern "C" unsigned long __cdecl strtoul(const char *str, char **endptr, int radix);
extern "C" double __cdecl strtod(const char *str, char **endptr);

extern "C" int __cdecl _vscprintf(const char *format, va_list args);
extern "C" int __cdecl vsprintf(char *string, const char *format, va_list args);
extern "C" int __cdecl sprintf(char *string, const char *format, ...);

extern "C" void __cdecl qsort(void *, size_t, size_t, int(__cdecl*)(const void *, const void *));

extern "C" double __cdecl floor(double x);
extern "C" double __cdecl fmod(double x, double y);

#endif

////////////////////////////////////////////////////////////////////////////////

#ifdef __MAC__

////////// stdarg.h

typedef __builtin_va_list va_list;
#define va_start(v, l) __builtin_va_start(v, l)
#define va_end(v) __builtin_va_end(v)
#define va_arg(v, l) __builtin_va_arg(v, l)

////////// stdio.h

#define BUFSIZ 1024
#define EOF (-1)

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

typedef struct __sFILE FILE;

extern "C" FILE *__stdinp;
extern "C" FILE *__stdoutp;
extern "C" FILE *__stderrp;

#define stdin __stdinp
#define stdout __stdoutp
#define stderr __stderrp

extern "C" int sprintf(char * __restrict, const char * __restrict, ...);
extern "C" int vsprintf(char * __restrict, const char * __restrict, va_list);
extern "C" int vsnprintf(char * __restrict string, size_t count, const char * __restrict format, va_list args);

extern "C" int fprintf(FILE * __restrict, const char * __restrict, ...);
extern "C" int vfprintf(FILE * __restrict, const char * __restrict, va_list);

extern "C" int sscanf(const char *__restrict string, const char *__restrict format, ...);

extern "C" FILE *fopen(const char *__restrict filename, const char *__restrict mode);
extern "C" int fclose(FILE *file);
extern "C" int feof(FILE *file);
extern "C" int fseek(FILE *file, long offset, int mode);
extern "C" long ftell(FILE *file);
extern "C" size_t fwrite(const void *__restrict buffer, size_t, size_t, FILE *__restrict);
extern "C" size_t fread(void *__restrict buffer, size_t, size_t, FILE *__restrict);
extern "C" int fileno(FILE *);

////////// stdlib.h

extern "C" void free(void *memory);
extern "C" void *malloc(size_t size);
extern "C" void *realloc(void *memory, size_t size);

extern "C" int atoi(const char *string);
extern "C" long strtol(const char *string, char **endptr, int radix);
extern "C" unsigned long strtoul(const char *string, char **endptr, int radix);
extern "C" double strtod(const char *string, char **endptr);

extern "C" void qsort(void *, size_t, size_t, int(*)(const void *, const void *));
extern "C" void qsort_r(void *, size_t, size_t, void *, int (*)(void *, const void *, const void *));

extern "C" void *bsearch(const void *, const void *, size_t, size_t, int (*)(const void *, const void *));

extern "C" void exit(int code);
extern "C" void abort(void);

////////// string.h

extern "C" void *memset(void *, int, size_t);
extern "C" void *memcpy(void *, const void *, size_t);
extern "C" void *memmove(void *, const void *, size_t);
extern "C" int memcmp(const void *, const void *, size_t);

extern "C" size_t strlen(const char *);

////////// math.h

extern "C" double sqrt(double x);
extern "C" double fabs(double x);
extern "C" double fmod(double x, double y);
extern "C" double sin(double x);
extern "C" double cos(double x);
extern "C" double tan(double x);
extern "C" double asin(double x);
extern "C" double acos(double x);
extern "C" double atan(double x);
extern "C" double atan2(double x, double y);
extern "C" double floor(double x);
extern "C" double ceil(double x);
extern "C" double ldexp(double x, int y);
extern "C" double pow(double x, double y);
extern "C" double exp(double x);
extern "C" double log(double x);
extern "C" double log10(double x);

////////// errno.h

#define EINVAL 22
#define ERANGE 34

////////// signal.h

#define SIGKILL 9
#define SIGTERM 15

////////// assert.h

extern "C" void __assert_rtn(const char *, const char *, int, const char *);
extern "C" void __eprintf(const char *, const char *, unsigned, const char *);

#define __assert(e, file, line) \
    __eprintf ("%s:%u: failed assertion `%s'\n", file, line, e)
#define assert(e) \
    (__builtin_expect(!(e), 0) ? __assert_rtn(__func__, __FILE__, __LINE__, #e) : (void)0)

////////// time.h

typedef long time_t;
extern "C" time_t time(time_t *);

////////// ctype.h

#include <ctype.h>
#undef isnumber

#endif

////////////////////////////////////////////////////////////////////////////////

#ifdef __LINUX__

#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <ctype.h>

#endif

////////////////////////////////////////////////////////////////////////////////

#ifdef __ANDROID__

#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <ctype.h>

// Work-around for the Android headers being strict
#if !defined(va_copy) && defined(__va_copy)
#  define va_copy __va_copy
#endif

#endif

////////////////////////////////////////////////////////////////////////////////

#ifdef __IOS__

#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <ctype.h>

#endif

////////////////////////////////////////////////////////////////////////////////

#endif
