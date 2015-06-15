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

#ifndef __MC_FOUNDATION__
#define __MC_FOUNDATION__

////////////////////////////////////////////////////////////////////////////////

#ifdef __ANDROID__
#define __PLATFORM_IS_ANDROID__
#endif

#ifdef __APPLE__
#include "TargetConditionals.h"
#endif

////////////////////////////////////////////////////////////////////////////////
//
//  MACRO UNDEFINITIONS
//

// __VISUALC__ will be defined if the compiler being used is MS Visual C.
#undef __VISUALC__
// __GCC__ will be defined if the compiler being used is GCC
#undef __GCC__

// __WINDOWS__ will be defined if the Windows platform is the target.
#undef __WINDOWS__
// __MAC__ will be defined if the Mac platform is the target.
#undef __MAC__
// __LINUX__ will be defined if the Linux platform is the target.
#undef __LINUX__
// __SOLARIS__ will be defined if the Solaris platform is the target.
#undef __SOLARIS__
// __IOS__ will be defined if the iOS platform is the target.
#undef __IOS__
// __ANDROID__ will be defined if the Android platform is the target.
#undef __ANDROID__
// __WINDOWS_MOBILE__ will be defined if the WindowsCE platform is the target.
#undef __WINDOWS_MOBILE__
// __LINUX_MOBILE__ will be defined if the Linux mobile platform is the target.
#undef __LINUX_MOBILE__

// __32_BIT__ will be defined if the target processor is 32-bit.
#undef __32_BIT__
// __64_BIT__ will be defined if the target processor is 64-bit.
#undef __64_BIT__	

// __LITTLE_ENDIAN__ will be defined if the target processor uses LE byte-order.
#undef __LITTLE_ENDIAN__
// __LITTLE_ENDIAN__ will be defined if the target processor uses BE byte-order.
#undef __BIG_ENDIAN__

// __I386__ will be defined if the target processor is i386.
#undef __I386__
// __X86_64__ will be defined if the target processor is x86-64.
#undef __X86_64__
// __PPC__ will be defined if the target processor is PowerPC.
#undef __PPC__
// __PPC_64__ will be defined if the target processor is PowerPC-64.
#undef __PPC_64__
// __SPARC__ will be defined if the target processor is Sparc.
#undef __SPARC__
// __SPARC_64__ will be defined if the target processor is Sparc-64.
#undef __SPARC_64__
// __ARM__ will be defined if the target processor is 32-bit ARM.
#undef __ARM__
// __ARM64__ will be defined if the target processor is 64-bit ARM.
#undef __ARM64__

// __SMALL__ will be defined if pointers are 32-bit and indicies are 32-bit.
#undef __SMALL__
// __MEDIUM__ will be defined if pointers are 64-bit and indicies are 32-bit.
#undef __MEDIUM__
// __HUGE__ will be defined if pointers are 64-bit and indicies are 64-bit.
#undef __HUGE__

// __WINDOWS_1252__ will be defined if it is the native charset of the platform.
#undef __WINDOWS_1252__
// __MACROMAN__ will be defined if it is the native charset of the platform.
#undef __MACROMAN__
// __ISO_8859_1__ will be defined if it is the native charset of the platform.
#undef __ISO_8859_1__

// __CRLF__ will be defined if the native line ending is CR LF.
#undef __CRLF__
// __CR__ will be defined if the native line ending is CR.
#undef __CR__
// __LF__ will be defined if the native line ending is LF.
#undef __LF__

// __HAS_CORE_FOUNDATION__ will be defined if the platform has the CF libraries.
#undef __HAS_CORE_FOUNDATION__

////////////////////////////////////////////////////////////////////////////////
//
//  CONFIGURE DEFINITIONS FOR WINDOWS
//

#if defined(_MSC_VER) && !defined(WINCE)

// Compiler
#define __VISUALC__ 1

// Platform
#define __WINDOWS__ 1

// Architecture
#if defined(_M_IX86)
#define __32_BIT__ 1 
#define __LITTLE_ENDIAN__ 1
#define __I386__ 1
#define __LP32__ 1
#define __SMALL__ 1
#elif defined(_M_X64)
#define __64_BIT__ 1
#define __LITTLE_ENDIAN__ 1
#define __X86_64__ 1
#define __LLP64__ 1
#define __HUGE__ 1
#else
#error Unknown target architecture
#endif

// Native char set
#define __WINDOWS_1252__ 1 

// Native line endings
#define __CRLF__ 1

#endif

////////////////////////////////////////////////////////////////////////////////
//
//  CONFIGURE DEFINITIONS FOR MAC
//

#if defined(__GNUC__) && defined(__APPLE__) && !defined(TARGET_OS_IPHONE)

// Compiler
#define __GCC__ 1

// Platform
#define __MAC__ 1

// Architecture
#if defined(__i386)
#define __32_BIT__ 1
#define __LITTLE_ENDIAN__ 1
#define __I386__ 1
#define __LP32__ 1
#define __SMALL__ 1
#elif defined(__ppc__)
#define __32_BIT__ 1 
#define __BIG_ENDIAN__ 1 
#define __PPC__ 1
#define __LP32__ 1
#define __SMALL__ 1
#elif defined(__x86_64__)
#define __64_BIT__ 1
#define __LITTLE_ENDIAN__ 1
#define __X86_64__ 1
#define __LP64__ 1
#define __HUGE__ 1
#endif

// Native char set
#define __MACROMAN__ 1

// Native line endings
#define __CR__ 1

// Presence of CoreFoundation
#define __HAS_CORE_FOUNDATION__

#endif

////////////////////////////////////////////////////////////////////////////////
//
//  CONFIGURE DEFINITIONS FOR LINUX
//

#if defined(__GNUC__) && !defined(__APPLE__) && !defined(__PLATFORM_IS_ANDROID__)

// Compiler
#define __GCC__

// Platform
#define __LINUX__

// Architecture
#if defined(__i386)
#define __32_BIT__
#define __LITTLE_ENDIAN__
#define __I386__
#define __LP32__ 1
#define __SMALL__
#elif defined(__x86_64__)
#define __64_BIT__
#define __LITTLE_ENDIAN__
#define __X86_64__
#define __LP64__ 1
#define __HUGE__
#elif defined(__arm__)
#define __32_BIT__
#define __LITTLE_ENDIAN__
#define __ARM__
#define __LP32__ 1
#define __SMALL__
#endif

// Native char set
#define __ISO_8859_1__

// Native line endings
#define __LF__

#endif

////////////////////////////////////////////////////////////////////////////////
//
//  CONFIGURE DEFINITIONS FOR IOS
//

#if defined(__GNUC__) && defined(__APPLE__) && defined(TARGET_OS_IPHONE)

// Compiler
#define __GCC__ 1

// Platform
#define __IOS__ 1

// Architecture
#if defined(__i386)
#define __32_BIT__ 1
#define __LITTLE_ENDIAN__ 1
#define __I386__ 1
#define __LP32__ 1
#define __SMALL__ 1
#elif defined(__ppc__)
#define __32_BIT__ 1 
#define __BIG_ENDIAN__ 1 
#define __PPC__ 1
#define __LP32__ 1
#define __SMALL__ 1
#elif defined(__x86_64__)
#define __64_BIT__ 1
#define __LITTLE_ENDIAN__ 1
#define __X86_64__ 1
#define __LP64__ 1
#define __HUGE__ 1
#elif defined(__arm__)
#define __32_BIT__ 1
#define __LITTLE_ENDIAN__ 1
#define __ARM__ 1
#define __LP32__ 1
#define __SMALL__ 1 
#elif defined(__arm64__)
#define __64_BIT__ 1
#define __LITTLE_ENDIAN__ 1
#define __ARM64__
#define __LP64__ 1
#define __HUGE__ 1
#endif

// Native char set
#define __MACROMAN__ 1

// Native line endings
#define __CR__ 1

// Presence of CoreFoundation
#define __HAS_CORE_FOUNDATION__

#endif

////////////////////////////////////////////////////////////////////////////////
//
//  CONFIGURE DEFINITIONS FOR ANDROID
//

#if defined(__GNUC__) && !defined(__APPLE__) && defined(__PLATFORM_IS_ANDROID__)

// Compiler
#define __GCC__

// Platform
#define __ANDROID__

// Architecture
#if defined(__i386)
#define __32_BIT__
#define __LITTLE_ENDIAN__
#define __I386__
#define __LP32__ 1
#define __SMALL__
#elif defined(__x86_64__)
#define __64_BIT__
#define __LITTLE_ENDIAN__
#define __X86_64__
#define __LP64__ 1
#define __HUGE__
#elif defined(__arm__)
#define __32_BIT__
#define __LITTLE_ENDIAN__
#define __ARM__
#define __LP32__ 1
#define __SMALL__
#endif

// Native char set
#define __ISO_8859_1__

// Native line endings
#define __LF__

#endif

////////////////////////////////////////////////////////////////////////////////
//
//  CONFIGURE DEFINITIONS FOR WINDOWS MOBILE
//

////////////////////////////////////////////////////////////////////////////////
//
//  CONFIGURE DEFINITIONS FOR LINUX MOBILE
//

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
//
//  FIXED WIDTH INTEGER TYPES
//

#if !defined(__VISUALC__)
#	define __HAVE_STDINT_H__
#	define __STDC_LIMIT_MACROS
#	include <stdint.h>
#endif

#if !defined(__HAVE_STDINT_H__)
typedef unsigned char uint8_t;
typedef signed char int8_t;
typedef unsigned short uint16_t;
typedef signed short int16_t;
typedef unsigned int uint32_t;
typedef signed int int32_t;

// MDW-2013-04-15: [[ x64 ]] added 64-bit-safe typedefs
#if !defined(uint64_t)
#define _UINT64_T
#ifndef __LP64__
typedef unsigned long long int uint64_t;
#else
typedef unsigned long int uint64_t;
#endif
#endif
#if !defined(int64_t)
#define _INT64_T
#ifndef __LP64__
typedef signed long long int int64_t;
#else
typedef signed long int int64_t;
#endif
#endif

#define UINT8_MAX (255U)
#define INT8_MIN (-128)
#define INT8_MAX (127)

#define UINT16_MAX (65535U)
#define INT16_MIN (-32768)
#define INT16_MAX (32767)

#define UINT32_MAX (4294967295U)
#define INT32_MIN (-2147483647 - 1L)
#define INT32_MAX (2147483647)

#define UINT64_MAX (18446744073709551615ULL)
#define INT64_MIN (-9223372036854775808LL)
#define INT64_MAX (9223372036854775807LL)
#endif /* !__HAVE_STDINT_H__ */

#define UINT8_MIN (0U)
#define UINT16_MIN (0U)
#define UINT32_MIN (0U)
#define UINT64_MIN (0ULL)

////////////////////////////////////////////////////////////////////////////////
//
//  DERIVED INTEGER TYPES
//

#ifdef __32_BIT__

typedef int32_t integer_t;
typedef uint32_t uinteger_t;

typedef int32_t intenum_t;
typedef uint32_t intset_t;

#if !defined(__HAVE_STDINT_H__)
#	if defined(__WINDOWS__)
typedef signed int intptr_t;
typedef unsigned int uintptr_t;
typedef unsigned int size_t;
#	elif defined(__LINUX__)
typedef signed int intptr_t;
typedef unsigned int uintptr_t;
typedef unsigned int size_t;
#	elif defined(__ANDROID__)
typedef signed int intptr_t;
typedef unsigned int uintptr_t;
typedef unsigned int size_t;
#	else
typedef long signed int intptr_t;
typedef long unsigned int uintptr_t;
typedef unsigned long size_t;
#	endif
#endif /* !__HAVE_STDINT_H__ */

#define INTEGER_MIN INT32_MIN
#define INTEGER_MAX INT32_MAX
#define UINTEGER_MIN UINT32_MIN
#define UINTEGER_MAX UINT32_MAX

#define UINTPTR_MIN UINT32_MIN

#if !defined(__HAVE_STDINT_H__)
#define INTPTR_MIN INT32_MIN
#define INTPTR_MAX INT32_MAX
#define UINTPTR_MAX UINT32_MAX
#endif /* !__HAVE_STDINT_H__ */

#else /* !__32_BIT__ */

typedef int32_t integer_t;
typedef uint32_t uinteger_t;

typedef int32_t intenum_t;
typedef uint32_t intset_t;

#if !defined(__HAVE_STDINT_H__)
#	ifndef _UINTPTR_T
#		define _UINTPTR_T
#		ifdef __LP64__
typedef uint64_t uintptr_t;
#		else
typedef uint32_t uintptr_t;
#		endif
#	endif

#	ifndef _INTPTR_T
#		define _INTPTR_T
#		ifdef __LP64__
typedef int64_t intptr_t;
#		else
typedef int32_t intptr_t;
#		endif
#	endif
#endif /* !__HAVE_STDINT_H__ */

#define INTEGER_MIN INT32_MIN
#define INTEGER_MAX INT32_MAX
#define UINTEGER_MIN UINT32_MIN
#define UINTEGER_MAX UINT32_MAX

#define UINTPTR_MIN UINT64_MIN

#if !defined(__HAVE_STDINT_H__)
#define INTPTR_MIN INT64_MIN
#define INTPTR_MAX INT64_MAX
#define UINTPTR_MAX UINT64_MAX
#endif /* !__HAVE_STDINT_H__ */

#endif /* !__32_BIT__ */

#if defined(__SMALL__) || defined(__MEDIUM__)

typedef uint32_t uindex_t;
typedef int32_t index_t;
typedef uint32_t hash_t;
typedef int32_t compare_t;

#define UINDEX_MIN UINT32_MIN
#define UINDEX_MAX UINT32_MAX
#define INDEX_MIN INT32_MIN
#define INDEX_MAX INT32_MAX

#else

typedef uint32_t uindex_t;
typedef int32_t index_t;
typedef uint64_t hash_t;
typedef int64_t compare_t;

#define UINDEX_MIN UINT32_MIN
#define UINDEX_MAX UINT32_MAX
#define INDEX_MIN INT32_MIN
#define INDEX_MAX INT32_MAX

#endif

typedef int64_t filepos_t;

////////////////////////////////////////////////////////////////////////////////
//
//  FLOATING-POINT TYPES
//

typedef double real64_t;
typedef float real32_t;

typedef double float64_t;
typedef float float32_t;



////////////////////////////////////////////////////////////////////////////////
//  SN-2014-06-25 [[ FieldCoordinates ]] typedef moved here
//
//  COORDINATES
//
typedef float32_t coord_t;

////////////////////////////////////////////////////////////////////////////////
//
//  CHAR AND STRING TYPES
//

// The 'char_t' type is used to hold a native encoded char.
typedef unsigned char char_t;

// The 'byte_t' type is used to hold a char in a binary string (native).
typedef uint8_t byte_t;

// The 'codepoint_t' type is used to hold a single Unicode codepoint (20-bit
// value).
typedef uint32_t codepoint_t;

// The 'unichar_t' type is used to hold a UTF-16 codeunit.
#ifdef __WINDOWS__
typedef wchar_t unichar_t;
#else
typedef uint16_t unichar_t;
#endif

#ifdef __WINDOWS__
typedef wchar_t *BSTR;
#endif

#if defined(__MAC__) || defined(__IOS__)
typedef const struct __CFString *CFStringRef;
typedef const struct __CFData *CFDataRef;
#endif

////////////////////////////////////////////////////////////////////////////////
//
//  POINTER TYPES
//

#if defined(__cplusplus) /* C++ */
#	if defined(__GCC__)
#		define nil __null
#	else
#		define nil uintptr_t(0)
#	endif

#else /* C */
#	if defined(__GCC__)
#		define nil __null
#	else
#		define nil ((void*)0)
#	endif

#endif

////////////////////////////////////////////////////////////////////////////////
//
//  STRUCTURE TYPES
//

struct MCRange
{
	uindex_t offset;
	uindex_t length;
};

////////////////////////////////////////////////////////////////////////////////
//
//  VALUE TYPES
//

typedef void *MCValueRef;
typedef struct __MCNull *MCNullRef;
typedef struct __MCBoolean *MCBooleanRef;
typedef struct __MCNumber *MCNumberRef;
typedef struct __MCString *MCStringRef;
typedef struct __MCName *MCNameRef;
typedef struct __MCData *MCDataRef;
typedef struct __MCArray *MCArrayRef;
typedef struct __MCList *MCListRef;
typedef struct __MCSet *MCSetRef;
typedef struct __MCStream *MCStreamRef;

// Forward declaration
typedef struct __MCLocale* MCLocaleRef;

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
//
//  REQUIRED STANDARD INCLUDES
//

#include <foundation-stdlib.h>
#include <math.h>

////////////////////////////////////////////////////////////////////////////////
//
//  MINIMUM FUNCTIONS
//

//inline uint32_t MCMin(uint32_t a, uint32_t b) { return a < b ? a : b; }
//inline int32_t MCMin(int32_t a, int32_t b) { return a < b ? a : b; }
//inline uint64_t MCMin(uint64_t a, uint64_t b) { return a < b ? a : b; }
//inline int64_t MCMin(int64_t a, int64_t b) { return a < b ? a : b; }
//inline double MCMin(double a, double b) { return a < b ? a : b; }
//inline float MCMin(float a, float b) { return a < b ? a : b; }
template <class T, class U> inline T MCMin(T a, U b) { return a < b ? a : b; }

////////////////////////////////////////////////////////////////////////////////
//
//  MAXIMUM FUNCTIONS
//

//inline uint32_t MCMax(uint32_t a, uint32_t b) { return a > b ? a : b; }
//inline int32_t MCMax(int32_t a, int32_t b) { return a > b ? a : b; }
//inline uint64_t MCMax(uint64_t a, uint64_t b) { return a > b ? a : b; }
//inline int64_t MCMax(int64_t a, int64_t b) { return a > b ? a : b; }
//inline double MCMax(double a, double b) { return a > b ? a : b; }
//inline float MCMax(float a, float b) { return a > b ? a : b; }
template <class T, class U> inline T MCMax(T a, U b) { return a > b ? a : b; }

////////////////////////////////////////////////////////////////////////////////
//
//  ABSOLUTE VALUE FUNCTIONS
//

inline uint32_t MCAbs(int32_t a) { return a < 0 ? -a : a; }
inline uint64_t MCAbs(int64_t a) { return a < 0 ? -a : a; }
inline float MCAbs(float a) { return fabsf(a); }
inline double MCAbs(double a) { return fabs(a); }

////////////////////////////////////////////////////////////////////////////////
//
//  SIGN FUNCTIONS
//

inline compare_t MCSgn(int32_t a) { return a < 0 ? -1 : (a > 0 ? 1 : 0); }
inline compare_t MCSgn(int64_t a) { return a < 0 ? -1 : (a > 0 ? 1 : 0); }

////////////////////////////////////////////////////////////////////////////////
//
//  COMPARE FUNCTIONS
//

// SN-2015-01-07: [[ iOS-64bit ]] Update the MCCompare functions
inline compare_t MCCompare(int a, int b) { return a < b ? -1 : (a > b ? 1 : 0); }
inline compare_t MCCompare(unsigned int a, unsigned int b) { return a < b ? -1 : (a > b ? 1 : 0); }
inline compare_t MCCompare(long a, long b) { return a < b ? -1 : (a > b ? 1 : 0); }
inline compare_t MCCompare(unsigned long a, unsigned long b) { return a < b ? -1 : (a > b ? 1 : 0); }
inline compare_t MCCompare(long long a, long long b) { return a < b ? -1 : (a > b ? 1 : 0); }
inline compare_t MCCompare(unsigned long long a, unsigned long long b) { return a < b ? -1 : (a > b ? 1 : 0); }


////////////////////////////////////////////////////////////////////////////////
//
//  COMPARE FUNCTIONS
//

inline bool MCIsPowerOfTwo(uint32_t x) { return (x & (x - 1)) == 0; }

inline float MCClamp(float value, float min, float max) {return MCMax(min, MCMin(max, value));}

////////////////////////////////////////////////////////////////////////////////
//
//  BYTE ORDER FUNCTIONS
//

enum MCByteOrder
{
	kMCByteOrderUnknown,
	kMCByteOrderLittleEndian,
	kMCByteOrderBigEndian,
};

inline MCByteOrder MCByteOrderGetCurrent(void)
{
#ifdef __LITTLE_ENDIAN__
	return kMCByteOrderLittleEndian;
#else
	return kMCByteOrderBigEndian;
#endif
}

inline uint16_t MCSwapInt16(uint16_t x)
{
	return (x >> 8) | (x << 8);
}

inline uint32_t MCSwapInt32(uint32_t x)
{
	return (x >> 24) | ((x >> 8) & 0xff00) | ((x & 0xff00) << 8) | (x << 24);
}

inline uint64_t MCSwapInt64(uint64_t x)
{
	return (x >> 56) | ((x >> 40) & 0xff00) | ((x >> 24) & 0xff0000) | ((x >> 8) & 0xff000000) |
			((x & 0xff000000) << 8) | ((x & 0xff0000) << 24) | ((x & 0xff00) << 40) | (x << 56);
}

#ifdef __LITTLE_ENDIAN__

inline uint16_t MCSwapInt16BigToHost(uint16_t x) {return MCSwapInt16(x);}
inline uint32_t MCSwapInt32BigToHost(uint32_t x) {return MCSwapInt32(x);}
inline uint64_t MCSwapInt64BigToHost(uint64_t x) {return MCSwapInt64(x);}

inline uint16_t MCSwapInt16HostToBig(uint16_t x) {return MCSwapInt16(x);}
inline uint32_t MCSwapInt32HostToBig(uint32_t x) {return MCSwapInt32(x);}
inline uint64_t MCSwapInt64HostToBig(uint64_t x) {return MCSwapInt64(x);}

inline uint16_t MCSwapInt16LittleToHost(uint16_t x) {return x;}
inline uint32_t MCSwapInt32LittleToHost(uint32_t x) {return x;}
inline uint64_t MCSwapInt64LittleToHost(uint64_t x) {return x;}

inline uint16_t MCSwapInt16HostToLittle(uint16_t x) {return x;}
inline uint32_t MCSwapInt32HostToLittle(uint32_t x) {return x;}
inline uint64_t MCSwapInt64HostToLittle(uint64_t x) {return x;}

inline uint16_t MCSwapInt16NetworkToHost(uint16_t x) {return MCSwapInt16(x);}
inline uint32_t MCSwapInt32NetworkToHost(uint32_t x) {return MCSwapInt32(x);}
inline uint64_t MCSwapInt64NetworkToHost(uint64_t x) {return MCSwapInt64(x);}

inline uint16_t MCSwapInt16HostToNetwork(uint16_t x) {return MCSwapInt16(x);}
inline uint32_t MCSwapInt32HostToNetwork(uint32_t x) {return MCSwapInt32(x);}
inline uint64_t MCSwapInt64HostToNetwork(uint64_t x) {return MCSwapInt64(x);}

#else

inline uint16_t MCSwapInt16BigToHost(uint16_t x) {return x;}
inline uint32_t MCSwapInt32BigToHost(uint32_t x) {return x;}
inline uint64_t MCSwapInt64BigToHost(uint64_t x) {return x;}

inline uint16_t MCSwapInt16HostToBig(uint16_t x) {return x;}
inline uint32_t MCSwapInt32HostToBig(uint32_t x) {return x;}
inline uint64_t MCSwapInt64HostToBig(uint64_t x) {return x;}

inline uint16_t MCSwapInt16LittleToHost(uint16_t x) {return MCSwapInt16(x);}
inline uint32_t MCSwapInt32LittleToHost(uint32_t x) {return MCSwapInt32(x);}
inline uint64_t MCSwapInt64LittleToHost(uint64_t x) {return MCSwapInt64(x);}

inline uint16_t MCSwapInt16HostToLittle(uint16_t x) {return MCSwapInt16(x);}
inline uint32_t MCSwapInt32HostToLittle(uint32_t x) {return MCSwapInt32(x);}
inline uint64_t MCSwapInt64HostToLittle(uint64_t x) {return MCSwapInt64(x);}

inline uint16_t MCSwapInt16NetworkToHost(uint16_t x) {return x;}
inline uint32_t MCSwapInt32NetworkToHost(uint32_t x) {return x;}
inline uint64_t MCSwapInt64NetworkToHost(uint64_t x) {return x;}

inline uint16_t MCSwapInt16HostToNetwork(uint16_t x) {return x;}
inline uint32_t MCSwapInt32HostToNetwork(uint32_t x) {return x;}
inline uint64_t MCSwapInt64HostToNetwork(uint64_t x) {return x;}

#endif

////////////////////////////////////////////////////////////////////////////////
//
//  RANGE FUNCTIONS
//

inline MCRange MCRangeMake(uindex_t p_offset, uindex_t p_length)
{
	MCRange t_range;
	t_range . offset = p_offset;
	t_range . length = p_length;
	return t_range;
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
//
//  STARTUP / SHUTDOWN HANDLING
//

bool MCInitialize(void);
void MCFinalize(void);

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
//
//  ERROR HANDLING
//

typedef uint32_t MCErrorCode;
enum
{
	kMCErrorNone,
	
	kMCErrorOutOfMemory,
};

typedef void (*MCErrorHandler)(MCErrorCode code);

// Throw the given error code (local to the current thread).
bool MCErrorThrow(MCErrorCode code);

// Catch the current error code (on the current thread) if any and clear it.
MCErrorCode MCErrorCatch(void);

// Returns true if there is an error pending on the current thread.
bool MCErrorIsPending(void);

// Returns any pending error (on the current thread) without clearing it.
MCErrorCode MCErrorPeek(void);

// Sets the error handler - called whenever MCErrorThrow is called.
void MCErrorSetHandler(MCErrorHandler handler);

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
//
//  DEBUG HANDLING
//

#ifdef _DEBUG

// If we are using GCC or Clang, we can give the compiler the hint that the
// assertion functions do not return. This is particularly useful for Clang's
// static analysis feature.
#if defined(__GNUC__) || defined (__clang__) || defined (__llvm__)
#define ATTRIBUTE_NORETURN  __attribute__((__noreturn__))
#else
#define ATTRIBUTE_NORETURN
#endif

extern void __MCAssert(const char *file, uint32_t line, const char *message) ATTRIBUTE_NORETURN;
#define MCAssert(m_expr) (void)( (!!(m_expr)) || (__MCAssert(__FILE__, __LINE__, #m_expr), 0) )

extern void __MCLog(const char *file, uint32_t line, const char *format, ...);
#define MCLog(m_format, ...) __MCLog(__FILE__, __LINE__, m_format, __VA_ARGS__)

extern void __MCLogWithTrace(const char *file, uint32_t line, const char *format, ...);
#define MCLogWithTrace(m_format, ...) __MCLogWithTrace(__FILE__, __LINE__, m_format, __VA_ARGS__)

extern void __MCUnreachable(void) ATTRIBUTE_NORETURN;
#define MCUnreachable() __MCUnreachable();

#else

#define MCAssert(expr)

#define MCLog(m_format, ...) 

#define MCLogWithTrace(m_format, ...)

#define MCUnreachable()

#endif

#define MC_CONCAT(X,Y) MC_CONCAT_(X,Y)
#define MC_CONCAT_(X,Y) X ## Y

#if (__cplusplus >= 201103L)
#define MCStaticAssert(expr) static_assert(expr, #expr)
#else
template<bool> struct __MCStaticAssert;
template<> struct __MCStaticAssert<true> { };
#define MCStaticAssert(expr)																						\
enum { MC_CONCAT(__MCSA_,__LINE__) = sizeof(__MCStaticAssert<expr>) }
#endif


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
//
//  BYTE-WISE OPERATIONS
//

// Clear the given block of memory to all 0's.
inline void MCMemoryClear(void *dst, size_t size) { memset(dst, 0, size); }

// Fill the given block of memory with the given (byte) value.
inline void MCMemoryFill(void *dst, size_t size, uint8_t value) { memset(dst, value, size); }

// Copy size bytes from src to dst, the blocks must not overlap.
inline void MCMemoryCopy(void *dst, const void *src, size_t size) { memcpy(dst, src, size); }

// Move size bytes from src to dst, the blocks may overlap.
inline void MCMemoryMove(void *dst, const void *src, size_t size) { memmove(dst, src, size); }

// Compare size bytes from left and right, returning true if they are all equal.
inline bool MCMemoryEqual(const void *left, const void *right, size_t size) { return memcmp(left, right, size) == 0; }

// Compare size bytes from left and right. The return value is < 0, == 0 or > 0
// depending on whether left < right, left == right or left > right when
// compared using byte-wise lexicographic ordering.
inline compare_t MCMemoryCompare(const void *left, const void *right, size_t size) { return memcmp(left, right, size); }

////////////////////////////////////////////////////////////////////////////////
//
//  RESIZABLE BLOCK ALLOCATION (UNINITIALIZED)
//

// This method returns an unitialized block of memory of the given size in
// block. An error is raised if allocation fails.
bool MCMemoryAllocate(size_t size, void*& r_block);

// This method returns an block of memory containing the same contents as the
// provided one.
bool MCMemoryAllocateCopy(const void *block, size_t size, void*& r_new_block);

// This method reallocates a block of memory allocated using MCMemoryAllocate.
// Any new space allocated is uninitialized. The new pointer to the block is
// returned. Note that the block may move regardless of new size. If the input
// block is nil, it is treated as an allocate call.
bool MCMemoryReallocate(void *block, size_t new_size, void*& r_new_block);

// This method deallocates a block of memory allocated using MCMemoryAllocate,
// or subsequently reallocated using MCMemoryReallocate. The block passed in
// may be nil.
void MCMemoryDeallocate(void *block);

//////////

template<typename T> bool MCMemoryAllocate(size_t p_size, T*& r_block)
{
	void *t_block;
	if (MCMemoryAllocate(p_size, t_block))
	{
		r_block = static_cast<T *>(t_block);
		return true;
	}
	return false;
}

template<typename T> bool MCMemoryAllocateCopy(const T *p_block, size_t p_block_size, T*& r_block)
{
	void *t_block;
	if (MCMemoryAllocateCopy(p_block, p_block_size, t_block))
	{
		r_block = static_cast<T *>(t_block);
		return true;
	}
	return false;
}

template<typename T> bool MCMemoryReallocate(T *p_block, size_t p_new_size, T*& r_new_block)
{
	void *t_new_block;
	if (MCMemoryReallocate(p_block, p_new_size, t_new_block))
	{
		r_new_block = static_cast<T *>(t_new_block);
		return true;
	}
	return false;
}

////////////////////////////////////////////////////////////////////////////////
//
//  FIXED-SIZE RECORD ALLOCATION (INITIALIZED)
//

// This method allocates a fixed size record, and initializes its bytes to
// zero. An error is raised if allocation fails.
//
// Note that a block allocated with New must be freed with Delete.
bool MCMemoryNew(size_t size, void*& r_record);

// This method deletes a fixed size record that was allocated with MCMemoryNew.
void MCMemoryDelete(void *p_record);

//////////

// SN-2014-06-19 [[ Bug 12651 ]] back key can not work, and it crush
// The placement new is needed in MCMemorytNew
// to properly allocate the classes in MCMessageEvent::create

#ifdef _DEBUG
#ifdef new
#undef new
#define redef_new
#endif
#endif

inline void *operator new (size_t, void *p_block, bool)
{
	return p_block;
}

template<typename T> bool MCMemoryNew(T*& r_record)
{
	void *t_record;
	if (MCMemoryNew(sizeof(T), t_record))
	{
        r_record = new(t_record, true) T;
		return true;
	}
	return false;
}

template<typename T> void MCMemoryDelete(T* p_record)
{
	MCMemoryDelete(static_cast<void *>(p_record));
}


////////////////////////////////////////////////////////////////////////////////
//
//  RESIZEABLE ARRAY ALLOCATION (INITIALIZED)
//

// This method allocates a resizable array of elements of the given size. All
// the bytes in the returned array are set to zero. An error is raised if
// allocation fails.
//
// Note that a block allocated with NewArray must be freed with DeleteArray.
bool MCMemoryNewArray(uindex_t count, size_t size, void*& r_array, uindex_t& r_count);
bool MCMemoryNewArray(uindex_t count, size_t size, void*& r_array);

// This method resizes an array, initializing any new bytes allocated to 0. If
// x_array is nil then it is treated as a NewArray call.
//
// Note that a block (re-)allocated with ResizeArray must be freed with DeleteArray.
bool MCMemoryResizeArray(uindex_t new_count, size_t size, void*& x_array, uindex_t& x_count);

// This method deallocates an array that was allocated or reallocated using the
// previous two methods.
void MCMemoryDeleteArray(void *array);

/////////

template<typename T> bool MCMemoryNewArray(uindex_t p_count, T*& r_array, uindex_t& r_count)
{
	void *t_array;
	if (MCMemoryNewArray(p_count, sizeof(T), t_array, r_count))
		return r_array = static_cast<T *>(t_array), true;
	return false;
}

template<typename T> bool MCMemoryNewArray(uindex_t p_count, T*& r_array)
{
	void *t_array;
	if (MCMemoryNewArray(p_count, sizeof(T), t_array))
		return r_array = static_cast<T *>(t_array), true;
	return false;
}

template<typename T> inline bool MCMemoryResizeArray(uindex_t p_new_count, T*& x_array, uindex_t& x_count)
{
	void *t_array;
	t_array = x_array;
	if (MCMemoryResizeArray(p_new_count, sizeof(T), t_array, x_count))
	{
		x_array = static_cast<T *>(t_array);
		return true;
	}
	return false;
}

template<typename T> void MCMemoryDeleteArray(T* p_array)
{
	MCMemoryDeleteArray(static_cast<void *>(p_array));
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
//
//  HASHING UTILITIES
//

// CAVEAT: The values returned by any Hash function are not guaranteed to remain
//   the same from version to version. In particular, never serialize a hash
//   value - recompute on unserialization of the object.

// Return a hash for the given integer.
hash_t MCHashInteger(integer_t i);

// Return a hash value for the given double - note that (hopefully!) hashing
// an integer stored as a double will be the same as hashing the integer.
hash_t MCHashDouble(double d);

// Returns a hash value for the given pointer.
hash_t MCHashPointer(void *p);

// Returns a hash value for the given sequence of bytes.
hash_t MCHashBytes(const void *bytes, size_t byte_count);

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
//
//  VALUE DEFINITIONS
//

typedef uint32_t MCValueTypeCode;
enum
{
	kMCValueTypeCodeNull,
	kMCValueTypeCodeBoolean,
	kMCValueTypeCodeNumber,
	kMCValueTypeCodeName,
	kMCValueTypeCodeString,
    kMCValueTypeCodeData,
	kMCValueTypeCodeArray,
	kMCValueTypeCodeList,
	kMCValueTypeCodeSet,
	kMCValueTypeCodePoint,
	KMCValueTypeCodeRectangle,
	kMCValueTypeCodeCustom,
};

enum
{
	kMCValueCustomHeaderSize = 12
};

struct MCValueCustomCallbacks
{
	bool is_singleton : 1;
	void (*destroy)(MCValueRef value);
	bool (*copy)(MCValueRef value, bool release, MCValueRef& r_value);
	bool (*equal)(MCValueRef value, MCValueRef other_value);
	hash_t (*hash)(MCValueRef value);
	bool (*describe)(MCValueRef value, MCStringRef& r_desc);
};

// Create a custom value with the given callbacks.
bool MCValueCreateCustom(const MCValueCustomCallbacks *callbacks, size_t extra_bytes, MCValueRef& r_value);

// Fetch the typecode of the given value.
MCValueTypeCode MCValueGetTypeCode(MCValueRef value);

bool MCValueIsEmpty(MCValueRef value);
bool MCValueIsArray(MCValueRef value);

// Reduce the reference count of the given value by one, destroying the value
// if it reaches 0. Note that (for convience) 'value' can be nil, it which case
// the call has no effect.
void MCValueRelease(MCValueRef value);

// Increment the reference count of the given value by one.
MCValueRef MCValueRetain(MCValueRef value);

// Copies the given value ensuring the resulting value is immutable (which is
// why it can fail).
bool MCValueCopy(MCValueRef value, MCValueRef& r_immutable_copy);
bool MCValueCopyAndRelease(MCValueRef value, MCValueRef& r_immutable_copy);

// Compares the two values in an exact fashion and returns true if they are
// equal. If the values are of different types, then they are not equal;
// otherwise comparison is as follows:
//   - null: the null object is a singleton and so two null values will
//     always be equal.
//   - boolean: the 'true' and 'false' objects are singletons and equal
//     to themselves but not each other.
//   - number: the two numbers must have exactly the same value (bit-wise).
//     A real is only equal to an integer if it has no fractional part
//     (i.e. no rounding is done).
//   - string: the two strings are equal iff they are of the same length and
//     comprise identical codepoint sequences (as unicode strings).
//   - array: the two arrays are equal iff they have the same keys and the
//     values of each corresponding keys are equal.
bool MCValueIsEqualTo(MCValueRef value, MCValueRef other_value);

// Returns a hash value for the (exact) content of the value - in particular
// no folding is done for strings or names. Note that if the hash of two values
// is equal it does not mean the values are equal; although the converse is true
// (by definition of IsEqualTo).
hash_t MCValueHash(MCValueRef value);

// Returns a string description of the given value suitable for display and
// debugging (although not necessarily for general string formatting).
bool MCValueCopyDescription(MCValueRef value, MCStringRef& r_desc);

// Returns true if pointer comparison is for the value is enough to determine
// equality. This is always true of booleans, nulls and names; other values
// must be interred first.
bool MCValueIsUnique(MCValueRef value);

// Inter the given value returning a new (immutable) value. Any two values that
// are equal as defined by IsEqualTo will inter to the same object. i.e. For
// interred values ref equivalence (i.e. comparing the pointers) is equivalent
// to IsEqualTo.
//
// Note that for singleton objects (nulls, booleans and names) interring just
// bumps the reference count and returns the same value as they already satisfy
//   x == y iff IsEqualTo(x, y)
//
bool MCValueInter(MCValueRef value, MCValueRef& r_unique_value);

// As the 'Inter' method except that 'value' will be released. This allows
// optimization in some cases as the original value can be (potentially) reused
// to build the unique one (cutting down on copying).
//
bool MCValueInterAndRelease(MCValueRef value, MCValueRef& r_unique_value);

// Fetch the 'extra bytes' field for the given custom value.
inline void *MCValueGetExtraBytesPtr(MCValueRef value) { return ((uint8_t *)value) + kMCValueCustomHeaderSize; }

//////////

template<typename T> inline T MCValueRetain(T value)
{
	return (T)MCValueRetain((MCValueRef)value);
}

// Utility function for assigning to MCValueRef vars.
template<typename T> inline void MCValueAssign(T& dst, T src)
{
	MCValueRetain(src);
	MCValueRelease(dst);
	dst = src;
}

template<typename T> inline bool MCValueInter(T value, T& r_value)
{
	MCValueRef t_unique_value;
	if (MCValueInter(value, t_unique_value))
		return r_value = (T)t_unique_value, true;
	return false;
}

template<typename T> inline bool MCValueInterAndRelease(T value, T& r_value)
{
	MCValueRef t_unique_value;
	if (MCValueInterAndRelease(value, t_unique_value))
		return r_value = (T)t_unique_value, true;
	return false;
}

template<typename T> inline bool MCValueCreateCustom(const MCValueCustomCallbacks *p_callbacks, size_t p_extra_bytes, T& r_value)
{
	MCValueRef t_value;
	if (MCValueCreateCustom(p_callbacks, p_extra_bytes, t_value))
		return r_value = (T)t_value, true;
	return false;
}

////////////////////////////////////////////////////////////////////////////////
//
//  BOOLEAN DEFINITIONS
//

extern MCNullRef kMCNull;

////////////////////////////////////////////////////////////////////////////////
//
//  BOOLEAN DEFINITIONS
//

extern MCBooleanRef kMCFalse;
extern MCBooleanRef kMCTrue;

////////////////////////////////////////////////////////////////////////////////
//
//  NUMBER DEFINITIONS
//

bool MCNumberCreateWithInteger(integer_t value, MCNumberRef& r_number);
bool MCNumberCreateWithUnsignedInteger(uinteger_t value, MCNumberRef& r_number);
bool MCNumberCreateWithReal(real64_t value, MCNumberRef& r_number);

bool MCNumberIsInteger(MCNumberRef number);
bool MCNumberIsReal(MCNumberRef number);

integer_t MCNumberFetchAsInteger(MCNumberRef number);
uinteger_t MCNumberFetchAsUnsignedInteger(MCNumberRef number);
real64_t MCNumberFetchAsReal(MCNumberRef number);

bool MCNumberParseOffset(MCStringRef p_string, uindex_t offset, uindex_t char_count, MCNumberRef &r_number);
bool MCNumberParse(MCStringRef string, MCNumberRef& r_number);
bool MCNumberParseUnicodeChars(const unichar_t *chars, uindex_t char_count, MCNumberRef& r_number);

extern MCNumberRef kMCZero;
extern MCNumberRef kMCOne;
extern MCNumberRef kMCMinusOne;

////////////////////////////////////////////////////////////////////////////////
//
//  NAME DEFINITIONS
//

// Like MCSTR but for NameRefs
MCNameRef MCNAME(const char *);

// Create a name using the given string.
bool MCNameCreate(MCStringRef string, MCNameRef& r_name);
// Create a name using chars.
bool MCNameCreateWithChars(const unichar_t *chars, uindex_t count, MCNameRef& r_name);
// Create a name using native chars.
bool MCNameCreateWithNativeChars(const char_t *chars, uindex_t count, MCNameRef& r_name);

// Create a name using the given string, releasing the original.
bool MCNameCreateAndRelease(MCStringRef string, MCNameRef& r_name);

// Looks for an existing name matching the given string.
MCNameRef MCNameLookup(MCStringRef string);

// Returns a unsigned integer which can be used to order a table for a binary
// search.
uintptr_t MCNameGetCaselessSearchKey(MCNameRef name);

// Returns the string content of the name.
MCStringRef MCNameGetString(MCNameRef name);

// Returns true if the given name is the empty name.
bool MCNameIsEmpty(MCNameRef name);

// Returns true if the names are equal (caselessly). Note that MCNameIsEqualTo
// is *not* the same as MCValueIsEqualTo as it is a comparison up to case (of
// the name's string) rather than exact.
bool MCNameIsEqualTo(MCNameRef left, MCNameRef right);

// The empty name object;
extern MCNameRef kMCEmptyName;

////////////////////////////////////////////////////////////////////////////////
//
//  STRING DEFINITIONS
//

// The enumerated type describing the encoding of a sequence of bytes.
typedef uint32_t MCStringEncoding;
enum
{
	// The (7-bit) ASCII encoding.
	kMCStringEncodingASCII,
	// The standard Windows (Latin-1) encoding.
	kMCStringEncodingWindows1252,
	// The standard Mac (Latin-1) encoding.
	kMCStringEncodingMacRoman,
	// The standard Linux (Latin-1) encoding.
	kMCStringEncodingISO8859_1,
	// The UTF-8 string encoding.
	kMCStringEncodingUTF8,
	// The UTF-16 string encoding in little endian byte-order.
	kMCStringEncodingUTF16LE,
	// The UTF-16 string encoding in big endian byte-order.
	kMCStringEncodingUTF16BE,
	// The UTF-32 string encoding in little endian byte-order.
	kMCStringEncodingUTF32LE,
	// The UTF-32 string encoding in big endian byte-order.
	kMCStringEncodingUTF32BE,
    
    kMCStringEncodingUTF16,
    kMCStringEncodingUTF32,
    
	// Map 'native' encoding to the appropriate concrete encoding depending
	// on platform.
#ifdef __WINDOWS_1252__
	kMCStringEncodingNative = kMCStringEncodingWindows1252,
#elif defined(__MACROMAN__)
	kMCStringEncodingNative = kMCStringEncodingMacRoman,
#elif defined(__ISO_8859_1__)
	kMCStringEncodingNative = kMCStringEncodingISO8859_1,
#endif
};

// The type describing options for various string methods.
typedef uint32_t MCStringOptions;
enum
{
	// Compare the strings codepoint for codepoint.
	kMCStringOptionCompareExact = 0,
	// Compare the strings codepoint for codepoint after normalization.
	kMCStringOptionCompareNonliteral = 1,
	// Compare the strings codepoint for codepoint after normalization and
	// folding.
	kMCStringOptionCompareCaseless = 2,
    // Compare strings without normalization but with case folding
    kMCStringOptionCompareFolded = 3,
};

/////////

// The empty string.
extern MCStringRef kMCEmptyString;

// The default string for the 'true' boolean value.
extern MCStringRef kMCTrueString;

// The default string for the 'false' boolean value.
extern MCStringRef kMCFalseString;

// The default string for the 'mixed' value of chunk properties.
extern MCStringRef kMCMixedString;

// The default string for ','.
extern MCStringRef kMCCommaString;

// The default string for '\n'.
extern MCStringRef kMCLineEndString;

// The default string for '\t'.
extern MCStringRef kMCTabString;

/////////

// Creates an MCStringRef wrapping the given constant c-string. Note that
// the c-string must be a C static string.
MCStringRef MCSTR(const char *string);

const char *MCStringGetCString(MCStringRef p_string);
bool MCStringIsEqualToCString(MCStringRef string, const char *cstring, MCStringOptions options);

// Create an immutable string from the given bytes, interpreting them using
// the specified encoding.
bool MCStringCreateWithBytes(const byte_t *bytes, uindex_t byte_count, MCStringEncoding encoding, bool is_external_rep, MCStringRef& r_string);
bool MCStringCreateWithBytesAndRelease(byte_t *bytes, uindex_t byte_count, MCStringEncoding encoding, bool is_external_rep, MCStringRef& r_string);

// Create an immutable string from the given unicode char sequence.
bool MCStringCreateWithChars(const unichar_t *chars, uindex_t char_count, MCStringRef& r_string);
bool MCStringCreateWithCharsAndRelease(unichar_t *chars, uindex_t char_count, MCStringRef& r_string);

// Create an immutable string from the given NUL terminated unicode char sequence.
bool MCStringCreateWithWString(const unichar_t *wstring, MCStringRef& r_string);
bool MCStringCreateWithWStringAndRelease(unichar_t *wstring, MCStringRef& r_string);

// Create an immutable string from the given native char sequence.
bool MCStringCreateWithNativeChars(const char_t *chars, uindex_t char_count, MCStringRef& r_string);
bool MCStringCreateWithNativeCharsAndRelease(char_t *chars, uindex_t char_count, MCStringRef& r_string);

// Create an immutable string from the given (native) c-string.
bool MCStringCreateWithCString(const char *cstring, MCStringRef& r_string);
bool MCStringCreateWithCStringAndRelease(char *cstring, MCStringRef& r_string);

#ifdef __HAS_CORE_FOUNDATION__
// Create a string from a CoreFoundation string object.
bool MCStringCreateWithCFString(CFStringRef cf_string, MCStringRef& r_string);
bool MCStringCreateWithCFStringAndRelease(CFStringRef cf_string, MCStringRef& r_string);
#endif

#ifdef __LINUX__
// Create a string from a C string in the system encoding
bool MCStringCreateWithSysString(const char *sys_string, MCStringRef &r_string);
#endif

// Create a mutable string with the given initial capacity. Note that the
// initial capacity is only treated as a hint, the string will extend itself
// as necessary.
bool MCStringCreateMutable(uindex_t initial_capacity, MCStringRef& r_string);

/////////

// Encode the given string with the specified encoding. Characters which cannot
// be represented in the target encoding are replaced by '?'.
bool MCStringEncode(MCStringRef string, MCStringEncoding encoding, bool is_external_rep, MCDataRef& r_data);
bool MCStringEncodeAndRelease(MCStringRef string, MCStringEncoding encoding, bool is_external_rep, MCDataRef& r_data);

// Decode the given data, intepreting in the given encoding.
bool MCStringDecode(MCDataRef data, MCStringEncoding encoding, bool is_external_rep, MCStringRef& r_string);
bool MCStringDecodeAndRelease(MCDataRef data, MCStringEncoding encoding, bool is_external_rep, MCStringRef& r_string);

/////////

// Create an immutable string, built using the given format specification and
// argument list.
bool MCStringFormat(MCStringRef& r_string, const char *format, ...);
bool MCStringFormatV(MCStringRef& r_string, const char *format, va_list args);

/////////

// Copy the given string as immutable.
bool MCStringCopy(MCStringRef string, MCStringRef& r_new_string);

// Copy the given string as immutable, releasing the original.
bool MCStringCopyAndRelease(MCStringRef string, MCStringRef& r_new_string);

// Copy the given string as mutable.
bool MCStringMutableCopy(MCStringRef string, MCStringRef& r_new_string);

// Copy the given string as mutable, releasing the original.
bool MCStringMutableCopyAndRelease(MCStringRef string, MCStringRef& r_new_string);

/////////

// Copy a substring of the given string as immutable.
bool MCStringCopySubstring(MCStringRef string, MCRange range, MCStringRef& r_substring);

// Copy a substring of the given string as immutable, releasing the original.
bool MCStringCopySubstringAndRelease(MCStringRef string, MCRange range, MCStringRef& r_substring);

// Copy a substring of the given string as mutable.
bool MCStringMutableCopySubstring(MCStringRef string, MCRange range, MCStringRef& r_substring);

// Copy a substring of the given string as mutable, releasing the original.
bool MCStringMutableCopySubstringAndRelease(MCStringRef string, MCRange range, MCStringRef& r_substring);

/////////

// Returns true if the string is mutable
bool MCStringIsMutable(const MCStringRef string);

// Returns true if the string is the empty string.
bool MCStringIsEmpty(MCStringRef string);

// Returns true if the the string only requires native characters to represent.
bool MCStringCanBeNative(MCStringRef string);

// Returns true if under the given comparison conditions, string cannot be represented natively.
bool MCStringCantBeNative(MCStringRef string, MCStringOptions p_options);

// Returns true if the string is stored as native chars.
bool MCStringIsNative(MCStringRef string);

// Returns true if the string only requires BMP characters to represent.
bool MCStringIsSimple(MCStringRef string);

// Returns true if the string only comprises non-combining characters.
bool MCStringIsUncombined(MCStringRef string);

/////////

// Returns the number of chars that make up the string. Note that a char is
// considered to be UTF-16 code unit. This is a strict upper bound for the
// length of the native char or unicode char sequence backing the string as
// natively encoded strings are at most the same length as their unicode encoded
// counterparts (the subtleties being surrogate pairs map to '?', and things
// like e,acute map to 'e-acute').
uindex_t MCStringGetLength(const MCStringRef string);

// Return a pointer to the char backing-store if possible. Note that if this
// method returns nil, then GetChars() must be used to fetch the contents as
// unicode codeunits.
const unichar_t *MCStringGetCharPtr(MCStringRef string);

// Return a pointer to the native char backing-store if possible. Note that if
// the method returns nil, then GetNativeChars() must be used to fetch the contents
// in native encoding.
const char_t *MCStringGetNativeCharPtr(MCStringRef string);
// The native length may be different from the string char count.
const char_t *MCStringGetNativeCharPtrAndLength(MCStringRef self, uindex_t& r_native_length);

// Returns the Unicode codepoint at the given codepoint index
codepoint_t MCStringGetCodepointAtIndex(MCStringRef string, uindex_t index);

// Returns the char at the given index.
unichar_t MCStringGetCharAtIndex(MCStringRef string, uindex_t index);

// Returns the native char at the given index.
char_t MCStringGetNativeCharAtIndex(MCStringRef string, uindex_t index);

// Returns the Unicode codepoint (not UTF-16 char) at the given codepoint index
codepoint_t MCStringGetCodepointAtIndex(MCStringRef string, uindex_t index);

// Returns the sequence of chars making up the given range in 'chars' and returns
// the number of chars generated. If 'chars' is nil, just the number of chars that
// would be generated is returned.
uindex_t MCStringGetChars(MCStringRef string, MCRange range, unichar_t *chars);

// Returns the sequence of native chars making up the given range in 'chars' and
// returns the number of chars generated. If 'chars' is nil, just the number of chars
// that would be generated is returned. Any unmappable chars get generated as '?'.
uindex_t MCStringGetNativeChars(MCStringRef string, MCRange range, char_t *chars);

// Nativize self
void MCStringNativize(MCStringRef string);

// Create a native copy of p_string
bool MCStringNativeCopy(MCStringRef p_string, MCStringRef& r_copy);

// Maps from a codepoint (character) range to a code unit (StringRef) range
bool MCStringMapCodepointIndices(MCStringRef, MCRange p_codepoint_range, MCRange& r_string_range);

// Maps from a code unit (StringRef) range to a codepoint (character) range
bool MCStringUnmapCodepointIndices(MCStringRef, MCRange p_string_range, MCRange &r_codepoint_range);

// Maps from a grapheme (visual character) range to a code unit (StringRef) range
bool MCStringMapGraphemeIndices(MCStringRef, MCLocaleRef, MCRange p_grapheme_range, MCRange& r_string_range);

// Maps from a code unit (StringRef) range to a grapheme (visual character) range
bool MCStringUnmapGraphemeIndices(MCStringRef, MCLocaleRef, MCRange p_string_range, MCRange& r_grapheme_range);

// Maps from a word range to a codeunit (StringRef) range
bool MCStringMapTrueWordIndices(MCStringRef, MCLocaleRef, MCRange p_word_range, MCRange& r_string_range);

// Maps from a codeunit (StringRef) range to a word range
bool MCStringUnmapTrueWordIndices(MCStringRef, MCLocaleRef, MCRange p_string_range, MCRange &r_word_range);

// Maps from a sentence range to a codeunit (StringRef) range
bool MCStringMapSentenceIndices(MCStringRef, MCLocaleRef, MCRange p_sentence_range, MCRange& r_string_range);

// Maps from a codeunit (StringRef) range to a sentence range
bool MCStringUnmapSentenceIndices(MCStringRef, MCLocaleRef, MCRange p_string_range, MCRange &r_sentence_range);

// Maps from a paragraph range to a codeunit (StringRef) range
bool MCStringMapParagraphIndices(MCStringRef, MCLocaleRef, MCRange p_paragraph_range, MCRange& r_string_range);

// Maps from a codeunit (StringRef) range to a word range
bool MCStringUnmapParagraphIndices(MCStringRef, MCLocaleRef, MCRange p_string_range, MCRange &r_paragraph_range);

// Returns true if the codepoint is alphabetic or numeric.
bool MCStringCodepointIsWordPart(codepoint_t p_codepoint);

// Flexible grapheme/codepoint/codeunit mapping used for "char" chunk expressions
enum MCCharChunkType
{
    kMCCharChunkTypeCodeunit,   // UTF-16 code units
    kMCCharChunkTypeCodepoint,  // Unicode codepoint values
    kMCCharChunkTypeGrapheme,   // Graphical character boundaries
};

const MCCharChunkType kMCDefaultCharChunkType = kMCCharChunkTypeGrapheme;

bool MCStringMapIndices(MCStringRef, MCCharChunkType, MCRange p_char_range, MCRange &r_codeunit_range);
bool MCStringUnmapIndices(MCStringRef, MCCharChunkType, MCRange p_codeunit_range, MCRange &r_char_range);

/////////

// Converts the contents of the string to bytes using the given encoding. The caller
// takes ownership of the byte array. Note that the returned array is NUL terminated,
// but this is not reflected in the byte count.
bool MCStringConvertToBytes(MCStringRef string, MCStringEncoding encoding, bool is_external_rep, byte_t*& r_bytes, uindex_t& r_byte_count);

// [[ Bug 12204 ]] textEncode ASCII support is actually native
// Converts the contents of the string to ASCII characters - excluding the characters from the extended range
bool MCStringConvertToAscii(MCStringRef self, char_t *&r_chars, uindex_t& r_char_count);

// Converts the contents of the string to unicode. The caller takes ownership of the
// char array. Note that the returned array is NUL terminated, but this is not
// reflected in the char count.
bool MCStringConvertToUnicode(MCStringRef string, unichar_t*& r_chars, uindex_t& r_char_count);

// Converts the contents of the string to native - using '?' as the unmappable char.
// The caller takes ownership of the char array. Note that the returned array is NUL
// terminated, but this is not reflected in the char count.
bool MCStringConvertToNative(MCStringRef string, char_t*& r_chars, uindex_t& r_char_count);

// Normalizes and converts to native
bool MCStringNormalizeAndConvertToNative(MCStringRef string, char_t*& r_chars, uindex_t& r_char_count);

// Converts the contents of the string to UTF-8. The caller takes ownership of the
// char array. Note that the returned array is NUL terminated but this is not
// reflected in the char count.
bool MCStringConvertToUTF8(MCStringRef string, char*& r_chars, uindex_t& r_char_count);

// Converts the contents of the string to UTF-32.
bool MCStringConvertToUTF32(MCStringRef self, uint32_t *&r_codepoints, uinteger_t &r_char_count);

// Normalizes and converts to c-string
bool MCStringNormalizeAndConvertToCString(MCStringRef string, char*& r_cstring);

// Converts the content to char_t*
bool MCStringConvertToCString(MCStringRef string, char*& r_cstring);

// Converts the content to wchar_t*
bool MCStringConvertToWString(MCStringRef string, unichar_t*& r_wstring);

// Converts the content to unicode_t*
bool MCStringConvertToUTF8String(MCStringRef string, char*& r_utf8string);

#if defined(__MAC__) || defined (__IOS__)
// Converts the content to CFStringRef
bool MCStringConvertToCFStringRef(MCStringRef string, CFStringRef& r_cfstring);
#endif

#ifdef __WINDOWS__
bool MCStringConvertToBSTR(MCStringRef string, BSTR& r_bstr);
#endif

// Converts the given string ref to a string in the system encoding.
// Note that the 'bytes' ptr is typed as 'char', however it should be considered
// as an opaque sequence of bytes with an 'unknowable' encoding.
// Note that the output string is allocated with an implicit NUL byte, but this
// is not included in the byte_count.
bool MCStringConvertToSysString(MCStringRef string, char*& r_bytes, size_t& r_byte_count);

/////////

// Returns the hash of the given string, processing as according to options.
hash_t MCStringHash(MCStringRef string, MCStringOptions options);

// Returns true if the two strings are equal, processing as appropriate according
// to options.
bool MCStringIsEqualTo(MCStringRef string, MCStringRef other, MCStringOptions options);
bool MCStringIsEqualToNativeChars(MCStringRef string, const char_t *chars, uindex_t char_count, MCStringOptions options);

// Returns true if the substring is equal to the other, according to options
bool MCStringSubstringIsEqualTo(MCStringRef string, MCRange range, MCStringRef p_other, MCStringOptions p_options);
bool MCStringSubstringIsEqualToSubstring(MCStringRef string, MCRange range, MCStringRef p_other, MCRange other_range, MCStringOptions p_options);

// Returns -1, 0, or 1, depending on whether left < 0, left == right or left > 0,
// processing as appropriate according to options. The ordering used is codepoint-
// wise lexicographic.
compare_t MCStringCompareTo(MCStringRef string, MCStringRef other, MCStringOptions options);

// Returns true if the string begins with the prefix string, processing as
// appropriate according to options.
bool MCStringBeginsWith(MCStringRef string, MCStringRef prefix, MCStringOptions options);
bool MCStringSharedPrefix(MCStringRef self, MCRange p_range, MCStringRef p_prefix, MCStringOptions p_options, uindex_t& r_self_match_length);
bool MCStringBeginsWithCString(MCStringRef string, const char_t *prefix_cstring, MCStringOptions options);

// Returns true if the string ends with the suffix string, processing as
// appropriate according to options.
bool MCStringEndsWith(MCStringRef string, MCStringRef suffix, MCStringOptions options);
bool MCStringSharedSuffix(MCStringRef self, MCRange p_range, MCStringRef p_suffix, MCStringOptions p_options, uindex_t& r_self_match_length);
bool MCStringEndsWithCString(MCStringRef string, const char_t *suffix_cstring, MCStringOptions options);

// Returns true if the string contains the given needle string, processing as
// appropriate according to options.
bool MCStringContains(MCStringRef string, MCStringRef needle, MCStringOptions options);

// Returns true if the substring contains the given needle string, processing as
// appropriate according to options.
bool MCStringSubstringContains(MCStringRef string, MCRange range, MCStringRef needle, MCStringOptions options);

//////////

// Find the first offset of needle in string, on or after index 'after',
// processing as appropriate according to options.
bool MCStringFirstIndexOf(MCStringRef string, MCStringRef needle, uindex_t after, MCStringOptions options, uindex_t& r_offset);
// Find the first offset of needle in string - where needle is a Unicode character
// (note it is a codepoint, not unichar - i.e. a 20-bit value).
bool MCStringFirstIndexOfChar(MCStringRef string, codepoint_t needle, uindex_t after, MCStringOptions options, uindex_t& r_offset);
// Find the first offset of needle in given range of string - where needle is a Unicode character
// (note it is a codepoint, not unichar - i.e. a 20-bit value).
bool MCStringFirstIndexOfCharInRange(MCStringRef self, codepoint_t p_needle, MCRange p_range, MCStringOptions p_options, uindex_t& r_offset);

// Find the last offset of needle in string, on or before index 'before',
// processing as appropriate according to options.
bool MCStringLastIndexOf(MCStringRef string, MCStringRef needle, uindex_t before, MCStringOptions options, uindex_t& r_offset);
// Find the last offset of needle in string - where needle is a Unicode character
// (note it is a codepoint, not unichar - i.e. a 20-bit value).
bool MCStringLastIndexOfChar(MCStringRef string, codepoint_t needle, uindex_t before, MCStringOptions options, uindex_t& r_offset);

// Search 'range' of 'string' for 'needle' processing as appropriate to optiosn
// and returning any located string in 'result'. If the result is false, no range is
// returned.
bool MCStringFind(MCStringRef string, MCRange range, MCStringRef needle, MCStringOptions options, MCRange* r_result);

// Search 'range' of 'string' for 'needle' processing as appropriate to options
// and returning the number of occurances found.
uindex_t MCStringCount(MCStringRef string, MCRange range, MCStringRef needle, MCStringOptions options);
uindex_t MCStringCountChar(MCStringRef string, MCRange range, codepoint_t needle, MCStringOptions options);

//////////

// Find the first index of separator in string processing as according to
// options, then split the string into head and tail - the strings either side
// of the needle.
bool MCStringDivide(MCStringRef string, MCStringRef separator, MCStringOptions options, MCStringRef& r_head, MCStringRef& r_tail);
bool MCStringDivideAtChar(MCStringRef string, codepoint_t separator, MCStringOptions options, MCStringRef& r_head, MCStringRef& r_tail);
bool MCStringDivideAtIndex(MCStringRef self, uindex_t p_offset, MCStringRef& r_head, MCStringRef& r_tail);

//////////

// Break the string into ranges inbetween the given delimiter char. A trailing
// empty range is ignored. The caller is responsible for deleting the returned
// array.
bool MCStringBreakIntoChunks(MCStringRef string, codepoint_t separator, MCStringOptions options, MCRange*& r_ranges, uindex_t& r_range_count);

//////////

// Transform the string to its folded form as specified by 'options'. The folded
// form of a string is that which is used to perform comparisons.
//
// Note that 'string' must be mutable, it is a fatal runtime error if it is not.
bool MCStringFold(MCStringRef string, MCStringOptions options);

// Lowercase the string.
//
// Note that 'string' must be mutable, it is a fatal runtime error if it is not.
bool MCStringLowercase(MCStringRef string, MCLocaleRef p_in_locale);

// Uppercase the string.
//
// Note that 'string' must be mutable, it is a fatal runtime error if it is not.
bool MCStringUppercase(MCStringRef string, MCLocaleRef p_in_locale);

/////////

// Append suffix to string.
//
// Note that 'string' must be mutable, it is a fatal runtime error if it is not.
bool MCStringAppend(MCStringRef string, MCStringRef suffix);
bool MCStringAppendSubstring(MCStringRef string, MCStringRef suffix, MCRange range);
bool MCStringAppendChars(MCStringRef string, const unichar_t *chars, uindex_t count);
bool MCStringAppendNativeChars(MCStringRef string, const char_t *chars, uindex_t count);
bool MCStringAppendChar(MCStringRef string, unichar_t p_char);
bool MCStringAppendNativeChar(MCStringRef string, char_t p_char);

// Prepend prefix to string.
//
// Note that 'string' must be mutable, it is a fatal runtime error if it is not.
bool MCStringPrepend(MCStringRef string, MCStringRef prefix);
bool MCStringPrependSubstring(MCStringRef string, MCStringRef suffix, MCRange range);
bool MCStringPrependChars(MCStringRef string, const unichar_t *chars, uindex_t count);
bool MCStringPrependNativeChars(MCStringRef string, const char_t *chars, uindex_t count);
bool MCStringPrependChar(MCStringRef string, unichar_t p_char);
bool MCStringPrependNativeChar(MCStringRef string, char_t p_char);

// Insert new_string into string at offset 'at'.
//
// Note that 'string' must be mutable, it is a fatal runtime error if it is not.
bool MCStringInsert(MCStringRef string, uindex_t at, MCStringRef new_string);
bool MCStringInsertSubstring(MCStringRef string, uindex_t at, MCStringRef new_string, MCRange range);
bool MCStringInsertChars(MCStringRef string, uindex_t at, const unichar_t *chars, uindex_t count);
bool MCStringInsertNativeChars(MCStringRef string, uindex_t at, const char_t *chars, uindex_t count);
bool MCStringInsertChar(MCStringRef string, uindex_t at, unichar_t p_char);
bool MCStringInsertNativeChar(MCStringRef string, uindex_t at, char_t p_char);

// Remove 'range' characters from 'string'.
//
// Note that 'string' must be mutable, it is a fatal runtime error if it is not.
bool MCStringRemove(MCStringRef string, MCRange range);

// Retain only 'range' characters from 'string'.
//
// Note that 'string' must be mutable, it is a fatal runtime error if it is not.
bool MCStringSubstring(MCStringRef string, MCRange range);

// Replace 'range' characters in 'string' with 'replacement'.
//
// Note that 'string' must be mutable, it is a fatal runtime error if it is not.
bool MCStringReplace(MCStringRef string, MCRange range, MCStringRef replacement);

// Pad the end of the string with count copies of value. If value is nil then count
// uninitialized bytes will be inserted after at.
//
// Note that 'string' must be mutable.
bool MCStringPad(MCStringRef string, uindex_t at, uindex_t count, MCStringRef value);

// Resolves the directionality of the string and returns true if it is left to right.
// (Uses MCBidiFirstStrongIsolate to determine directionality).
bool MCStringResolvesLeftToRight(MCStringRef p_string);

// Find and replace all instances of pattern in target with replacement.
//
// Note that 'string' must be mutable.
bool MCStringFindAndReplace(MCStringRef string, MCStringRef pattern, MCStringRef replacement, MCStringOptions options);
bool MCStringFindAndReplaceChar(MCStringRef string, codepoint_t pattern, codepoint_t replacement, MCStringOptions options);

bool MCStringWildcardMatch(MCStringRef source, MCRange source_range, MCStringRef pattern, MCStringOptions p_options);

/////////

// Append a formatted string to another string.
//
// Note that 'string' must be mutable, it is a fatal runtime error if it is not.
bool MCStringAppendFormat(MCStringRef string, const char *format, ...);
bool MCStringAppendFormatV(MCStringRef string, const char *format, va_list args);

//////////

bool MCStringSplit(MCStringRef string, MCStringRef element_del, MCStringRef key_del, MCStringOptions options, MCArrayRef& r_array);
bool MCStringSplitColumn(MCStringRef string, MCStringRef col_del, MCStringRef row_del, MCStringOptions options, MCArrayRef& r_array);

//////////

// Converts two surrogate pair code units into a codepoint
codepoint_t MCStringSurrogatesToCodepoint(unichar_t p_lead, unichar_t p_trail);

// Converts a codepoint to UTF-16 code units and returns the number of units
unsigned int MCStringCodepointToSurrogates(codepoint_t, unichar_t (&r_units)[2]);

// Returns true if the code unit at the given index and the next code unit form
// a valid surrogate pair. Lone lead or trail code units are not valid pairs.
bool MCStringIsValidSurrogatePair(MCStringRef, uindex_t);

//////////

// Normalises a string into the requested form
bool MCStringNormalizedCopyNFC(MCStringRef, MCStringRef&);
bool MCStringNormalizedCopyNFD(MCStringRef, MCStringRef&);
bool MCStringNormalizedCopyNFKC(MCStringRef, MCStringRef&);
bool MCStringNormalizedCopyNFKD(MCStringRef, MCStringRef&);

//////////

// Utility to avoid multiple number conversion from a string when possible
bool MCStringSetNumericValue(MCStringRef self, double p_value);
bool MCStringGetNumericValue(MCStringRef self, double &r_value);

////////////////////////////////////////////////////////////////////////////////
//
//  DATA DEFINITIONS
//
// Immutable data methods

extern MCDataRef kMCEmptyData;

bool MCDataCreateWithBytes(const byte_t *p_bytes, uindex_t p_byte_count, MCDataRef& r_data);
bool MCDataCreateWithBytesAndRelease(byte_t *p_bytes, uindex_t p_byte_count, MCDataRef& r_data);

bool MCDataConvertStringToData(MCStringRef string, MCDataRef& r_data);

bool MCDataIsEmpty(MCDataRef p_data);

uindex_t MCDataGetLength(MCDataRef p_data);
const byte_t *MCDataGetBytePtr(MCDataRef p_data);

byte_t MCDataGetByteAtIndex(MCDataRef p_data, uindex_t p_index);

hash_t MCDataHash(MCDataRef p_data);
bool MCDataIsEqualTo(MCDataRef p_left, MCDataRef p_right);

compare_t MCDataCompareTo(MCDataRef p_left, MCDataRef p_right);

// Mutable data methods

bool MCDataCreateMutable(uindex_t p_initial_capacity, MCDataRef& r_data);

bool MCDataCopy(MCDataRef p_data, MCDataRef& r_new_data);
bool MCDataCopyAndRelease(MCDataRef p_data, MCDataRef& r_new_data);
bool MCDataMutableCopy(MCDataRef p_data, MCDataRef& r_mutable_data);
bool MCDataMutableCopyAndRelease(MCDataRef p_data, MCDataRef& r_mutable_data);

bool MCDataCopyRange(MCDataRef data, MCRange range, MCDataRef& r_new_data);
bool MCDataCopyRangeAndRelease(MCDataRef data, MCRange range, MCDataRef& r_new_data);
bool MCDataMutableCopyRange(MCDataRef data, MCRange range, MCDataRef& r_new_data);
bool MCDataMutableCopyRangeAndRelease(MCDataRef data, MCRange range, MCDataRef& r_new_data);

bool MCDataIsMutable(const MCDataRef p_data);

bool MCDataAppend(MCDataRef r_data, MCDataRef p_suffix);
bool MCDataAppendBytes(MCDataRef r_data, const byte_t *p_bytes, uindex_t p_byte_count);
bool MCDataAppendByte(MCDataRef r_data, byte_t p_byte);

bool MCDataPrepend(MCDataRef r_data, MCDataRef p_prefix);
bool MCDataPrependBytes(MCDataRef r_data, const byte_t *p_bytes, uindex_t p_byte_count);
bool MCDataPrependByte(MCDataRef r_data, byte_t p_byte);

bool MCDataInsert(MCDataRef r_data, uindex_t p_at, MCDataRef p_new_data);
bool MCDataInsertBytes(MCDataRef self, uindex_t p_at, const byte_t *p_bytes, uindex_t p_byte_count);

bool MCDataRemove(MCDataRef r_data, MCRange p_range);
bool MCDataReplace(MCDataRef r_data, MCRange p_range, MCDataRef p_new_data);
bool MCDataReplaceBytes(MCDataRef r_data, MCRange p_range, const byte_t *p_new_data, uindex_t p_byte_count);

bool MCDataPad(MCDataRef data, byte_t byte, uindex_t count);

// convert the given data to CFDataRef
#if defined(__MAC__) || defined (__IOS__)
bool MCDataConvertToCFDataRef(MCDataRef p_data, CFDataRef& r_cfdata);
#endif

////////////////////////////////////////////////////////////////////////////////
//
//  ARRAY DEFINITIONS
//

extern MCArrayRef kMCEmptyArray;

// Create an immutable array containing the given keys and values.
bool MCArrayCreate(bool case_sensitive, const MCNameRef *keys, const MCValueRef *values, uindex_t length, MCArrayRef& r_array);

// Create an empty mutable array.
bool MCArrayCreateMutable(MCArrayRef& r_array);

// Make an immutable copy of the given array. If the 'copy and release' form is
// used then the original array is released (has its reference count reduced by
// one).
bool MCArrayCopy(MCArrayRef array, MCArrayRef& r_new_array);
bool MCArrayCopyAndRelease(MCArrayRef array, MCArrayRef& r_new_array);

// Make a mutable copy of the given array. If the 'copy and release' form is
// used then the original array is released (has its reference count reduced by
// one).
bool MCArrayMutableCopy(MCArrayRef array, MCArrayRef& r_new_array);
bool MCArrayMutableCopyAndRelease(MCArrayRef array, MCArrayRef& r_new_array);

// Returns 'true' if the given array is mutable.
bool MCArrayIsMutable(MCArrayRef array);

// Returns the number of elements in the array.
uindex_t MCArrayGetCount(MCArrayRef array);

// Fetch the value from the array with the given key. The returned value is
// not retained. If being stored elsewhere ValueCopy should be used to make an
// immutable copy first. If 'false' is returned it means the key was not found
// (in this case r_value is undefined).
bool MCArrayFetchValue(MCArrayRef array, bool case_sensitive, MCNameRef key, MCValueRef& r_value);
// Store the given value into the array. The value is copied appropriately so
// that the original can still be modified without affecting the copy in the
// array.
bool MCArrayStoreValue(MCArrayRef array, bool case_sensitive, MCNameRef key, MCValueRef value);
// Remove the given key from the array.
bool MCArrayRemoveValue(MCArrayRef array, bool case_sensitive, MCNameRef key);

// Fetches index i in the given (sequence) array.
bool MCArrayFetchValueAtIndex(MCArrayRef array, index_t index, MCValueRef& r_value);
// Store index i in the given (sequence) array.
bool MCArrayStoreValueAtIndex(MCArrayRef array, index_t index, MCValueRef value);
// Remove index i from the given (sequence) array.
bool MCArrayRemoveValueAtIndex(MCArrayRef array, index_t index);

// Fetch the value from the array on the given path. The returned value is
// not retained. If being stored elsewhere ValueCopy should be used to make an
// immutable copy first. If 'false' is returned it means the key was not found
// (in this case r_value is undefined).
bool MCArrayFetchValueOnPath(MCArrayRef array, bool case_sensitive, const MCNameRef *path, uindex_t path_length, MCValueRef& r_value);
// Store the given value into the array on the given path. The value is retained
// and not copied before being inserted.
bool MCArrayStoreValueOnPath(MCArrayRef array, bool case_sensitive, const MCNameRef *path, uindex_t path_length, MCValueRef value);
// Remove the value on the given path in the array.
bool MCArrayRemoveValueOnPath(MCArrayRef array, bool case_sensitive, const MCNameRef *path, uindex_t path_length);

// Apply the callback function to each element of the array. Do not modify the
// array within the callback as this will cause undefined behavior.
typedef bool (*MCArrayApplyCallback)(void *context, MCArrayRef array, MCNameRef key, MCValueRef value);
bool MCArrayApply(MCArrayRef array, MCArrayApplyCallback callback, void *context);

// Iterate through the array. 'iterator' should be initialized to zero the first
// time the method is called, and should then be called until it returns 'false'.
// A return value of 'false' means no further elements. Do not modify the array
// inbetween calls to Iterate as this will cause undefined behavior.
bool MCArrayIterate(MCArrayRef array, uintptr_t& iterator, MCNameRef& r_key, MCValueRef& r_value);

// Returns true if the given array is the empty array.
bool MCArrayIsEmpty(MCArrayRef self);

////////////////////////////////////////////////////////////////////////////////
//
//  LIST DEFINITIONS
//

extern MCListRef kMCEmptyList;

// Create a list from a static list of values - NOT IMPLEMENTED YET.
// bool MCListCreate(char delimiter, const MCValueRef *values, uindex_t value_count, MCListRef& r_list);

// Create a mutable list - initially empty.
bool MCListCreateMutable(char_t delimiter, MCListRef& r_list);
bool MCListCreateMutable(MCStringRef p_delimiter, MCListRef& r_list);

// Eventually this will accept any value type, but for now - just strings, names, and booleans.
bool MCListAppend(MCListRef list, MCValueRef value);

// Append a substring to the list.
bool MCListAppendSubstring(MCListRef list, MCStringRef value, MCRange range);

// Append a sequence of native chars as an element.
bool MCListAppendNativeChars(MCListRef list, const char_t *chars, uindex_t char_count);

// Append a formatted string as an element.
bool MCListAppendFormat(MCListRef list, const char *format, ...);

// Make an immutable copy of the list.
bool MCListCopy(MCListRef list, MCListRef& r_new_list);

// Makes an immutable copy of the list and release the list
bool MCListCopyAndRelease(MCListRef list, MCListRef& r_new_list);

// Make a copy of the list as a string.
bool MCListCopyAsString(MCListRef list, MCStringRef& r_string);

// Make an copy of the list as a string and release the list.
bool MCListCopyAsStringAndRelease(MCListRef list, MCStringRef& r_string);

// Returns true if nothing has been added to the list
bool MCListIsEmpty(MCListRef list);

////////////////////////////////////////////////////////////////////////////////
//
//  SET DEFINITIONS
//

extern MCSetRef kMCEmptySet;

bool MCSetCreateSingleton(uindex_t element, MCSetRef& r_set);
bool MCSetCreateWithIndices(uindex_t *elements, uindex_t element_count, MCSetRef& r_set);
bool MCSetCreateWithLimbsAndRelease(uindex_t *limbs, uindex_t limb_count, MCSetRef& r_set);

bool MCSetCreateMutable(MCSetRef& r_set);

bool MCSetCopy(MCSetRef set, MCSetRef& r_new_set);
bool MCSetCopyAndRelease(MCSetRef set, MCSetRef& r_new_set);

bool MCSetMutableCopy(MCSetRef set, MCSetRef& r_new_set);
bool MCSetMutableCopyAndRelease(MCSetRef set, MCSetRef& r_new_set);

bool MCSetIsMutable(MCSetRef self);

bool MCSetIsEmpty(MCSetRef set);
bool MCSetIsEqualTo(MCSetRef set, MCSetRef other_set);

bool MCSetContains(MCSetRef set, MCSetRef other_set);
bool MCSetIntersects(MCSetRef set, MCSetRef other_set);

bool MCSetContainsIndex(MCSetRef set, uindex_t index);

bool MCSetIncludeIndex(MCSetRef set, uindex_t index);
bool MCSetExcludeIndex(MCSetRef set, uindex_t index);

bool MCSetUnion(MCSetRef set, MCSetRef other_set);
bool MCSetDifference(MCSetRef set, MCSetRef other_set);
bool MCSetIntersect(MCSetRef set, MCSetRef other_set);

bool MCSetIterate(MCSetRef set, uindex_t& x_iterator, uindex_t& r_element);
bool MCSetList(MCSetRef set, uindex_t*& r_element, uindex_t& r_element_count);

////////////////////////////////////////////////////////////////////////////////
//
//  POINT DEFINITIONS
//

////////////////////////////////////////////////////////////////////////////////
//
//  RECTANGLE DEFINITIONS
//

////////////////////////////////////////////////////////////////////////////////
//
//  STREAM DEFINITIONS
//

extern MCStreamRef kMCStdinStream;
extern MCStreamRef kMCStdoutStream;
extern MCStreamRef kMCStderrStream;

// Basic stream creation.

struct MCStreamCallbacks
{
	// Cleanup all resources associated with the stream.
	void (*destroy)(MCStreamRef stream);
	// Returns true if attempting to read or write anymore will result in eof.
	bool (*is_finished)(MCStreamRef stream, bool& r_finished);
	// Get the number of bytes available for read on the stream immediately
	// without needing to block (or hitting eof).
	bool (*get_available_for_read)(MCStreamRef stream, size_t& r_amount);
	// Read the given number of bytes in the buffer - this call only succeeds
	// if amount bytes are successfully read.
	bool (*read)(MCStreamRef stream, void *buffer, size_t amount);
	// Get the number of bytes available for writing to the stream immediately
	// without needing to block (or hitting eof).
	bool (*get_available_for_write)(MCStreamRef stream, size_t& r_amount);
	// Write the given number of bytes to the stream - this call only succeeds
	// if amount bytes are successfully written.
	bool (*write)(MCStreamRef stream, const void *buffer, size_t amount);
	// Skip the given number of bytes - this call only suceeds if there are amount
	// bytes to skip.
	bool (*skip)(MCStreamRef stream, size_t amount);
	// Record the current location in the stream and ensure that as long as no
	// more than 'limit' bytes are read, 'reset' can be called to return to the
	// mark.
	bool (*mark)(MCStreamRef stream, size_t limit);
	// Return the stream to the previously 'marked' position.
	bool (*reset)(MCStreamRef stream);
	// Return the stream position.
	bool (*tell)(MCStreamRef stream, filepos_t& r_position);
	// Set the stream position - position must be within the extent of the stream,
	// attempts to seek past the end of the stream is an error.
	bool (*seek)(MCStreamRef stream, filepos_t position);
};

enum
{
	kMCStreamHeaderSize = sizeof(MCStreamCallbacks *)
};

// Create a stream running using the giving callbacks. Note that the lifetime
// of 'callbacks' must exceed that of the stream.
bool MCStreamCreate(const MCStreamCallbacks *callbacks, size_t extra_bytes, MCStreamRef& r_stream);

// Fetch the callback pointer attached to the stream - this can be used for type-
// checking.
const MCStreamCallbacks *MCStreamGetCallbacks(MCStreamRef stream);

// Fetch the start of the 'extra bytes' for the stream.
inline void *MCStreamGetExtraBytesPtr(MCStreamRef stream) { return ((uint8_t *)MCValueGetExtraBytesPtr(stream)) + kMCStreamHeaderSize; }

// Memory-based input stream

// Create a read-only stream targetting a memory block. The lifetime of the
// memory block must exceed that of the stream.
bool MCMemoryInputStreamCreate(const void *block, size_t size, MCStreamRef& r_stream);

// Memory-based output stream

// Create a write-only stream that outputs to memory.
bool MCMemoryOutputStreamCreate(MCStreamRef& r_stream);
// Finish the memory output and return the buffer. The stream will continue
// to work, but will be reset to empty and a new buffer accumulated.
bool MCMemoryOutputStreamFinish(MCStreamRef stream, void*& r_buffer, size_t& r_size);

// Capabilities

// If a stream is readable it means that the 'Read' method work.
bool MCStreamIsReadable(MCStreamRef stream);
// If a stream is writable it means that the 'Write' method works.
bool MCStreamIsWritable(MCStreamRef stream);
// If a stream is markable it means that the stream supports the 'Mark' and
// 'Reset' methods.
bool MCStreamIsMarkable(MCStreamRef stream);
// If a stream is seekable it means that the stream supports the 'Tell' and
// 'Seek' methods.
bool MCStreamIsSeekable(MCStreamRef stream);

// Readable streams

// Returns the number of bytes available on the stream without blocking (or
// hitting eof).
bool MCStreamGetAvailableForRead(MCStreamRef stream, size_t& r_available);
// Attempt to read 'amount' bytes into the given buffer.
bool MCStreamRead(MCStreamRef stream, void *buffer, size_t amount);

// Returns the number of bytes space available on the stream for writing
// without blocking.
bool MCStreamGetAvailableForWrite(MCStreamRef stream, size_t& r_available);
// Attempt to write 'amount' bytes to the output stream.
bool MCStreamWrite(MCStreamRef stream, const void *buffer, size_t amount);

// Readable and writable streams.

// If reading or writing a single byte would result in failure due to end of
// file, 'finished' will contain true.
bool MCStreamIsFinished(MCStreamRef stream, bool& r_finished);

// Attempt to skip 'amount' bytes. This call only succeeds if the stream pointer
// is 'amount' bytes away from eod.
bool MCStreamSkip(MCStreamRef stream, size_t amount);

// Markable streams only

// Record the current location in the stream. If the stream head moves no further
// than 'read_limit' bytes forward, then the 'Reset' method will allow a return to
// the mark.
bool MCStreamMark(MCStreamRef stream, size_t read_limit);
// Reset the stream to the last recorded mark.
bool MCStreamReset(MCStreamRef stream);

// Seekable streams only

// Fetch the current stream position.
bool MCStreamTell(MCStreamRef stream, filepos_t& r_position);
// Set the current stream position.
bool MCStreamSeek(MCStreamRef stream, filepos_t position);

// Simple byte-based serialization / unserialization functions. These are all
// wrappers around read/write, and assume no higher-level structure.

// Fixed-size unsigned and signed integer functions.
bool MCStreamReadUInt8(MCStreamRef stream, uint8_t& r_value);
bool MCStreamReadUInt16(MCStreamRef stream, uint16_t& r_value);
bool MCStreamReadUInt32(MCStreamRef stream, uint32_t& r_value);
bool MCStreamReadUInt64(MCStreamRef stream, uint64_t& r_value);
bool MCStreamReadInt8(MCStreamRef stream, int8_t& r_value);
bool MCStreamReadInt16(MCStreamRef stream, int16_t& r_value);
bool MCStreamReadInt32(MCStreamRef stream, int32_t& r_value);
bool MCStreamReadInt64(MCStreamRef stream, int64_t& r_value);

// Variable-sized unsigned and signing integer functions. These methods use
// the top-bit of successive bytes to indicate whether more bytes follow - each
// byte encodes 7 bits of information.
bool MCStreamReadCompactUInt32(MCStreamRef stream, uint32_t& r_value);
bool MCStreamReadCompactUInt64(MCStreamRef stream, uint64_t& r_value);
bool MCStreamReadCompactSInt32(MCStreamRef stream, uint32_t& r_value);
bool MCStreamReadCompactSInt64(MCStreamRef stream, uint64_t& r_value);

// Fixed-size binary-floating point functions.
bool MCStreamReadFloat(MCStreamRef stream, float& r_value);
bool MCStreamReadDouble(MCStreamRef stream, double& r_value);

// Known valueref functions - these assume the given type is present.
bool MCStreamReadBoolean(MCStreamRef stream, MCBooleanRef& r_boolean);
bool MCStreamReadNumber(MCStreamRef stream, MCNumberRef& r_number);
bool MCStreamReadName(MCStreamRef stream, MCNameRef& r_name);
bool MCStreamReadString(MCStreamRef stream, MCStringRef& r_string);
bool MCStreamReadArray(MCStreamRef stream, MCArrayRef& r_array);
bool MCStreamReadSet(MCStreamRef stream, MCSetRef& r_set);

// Variant valueref functions - these tag the data with the type, allowing
// easy encoding/decoding of any value type (that supports serialization).
bool MCStreamReadValue(MCStreamRef stream, MCValueRef& r_value);

////////////////////////////////////////////////////////////////////////////////

#endif
