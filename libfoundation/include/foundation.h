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
// __EMSCRIPTEN__ will be defined if Emscripten JavaScript is the target
//#undef __EMSCRIPTEN__ // It will be defined by the compiler

// __32_BIT__ will be defined if the target processor is 32-bit.
#undef __32_BIT__
// __64_BIT__ will be defined if the target processor is 64-bit.
#undef __64_BIT__	

// __LITTLE_ENDIAN__ will be defined if the target processor uses LE byte-order.
#undef __LITTLE_ENDIAN__
// __LITTLE_ENDIAN__ will be defined if the target processor uses BE byte-order.
#undef __BIG_ENDIAN__

// __i386__ will be defined if the target processor is i386.
#undef __i386__
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
// __LARGE__ will be defined if pointers are 64-bit and indicies are 64-bit.
#undef __LARGE__

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

// __LP64__ will be defined if longs and pointers are 64 bits.
#undef __LP64__

// __HAS_CORE_FOUNDATION__ will be defined if the platform has the CF libraries.
#undef __HAS_CORE_FOUNDATION__

// __HAS_MULTIPLE_ABIS__ will be defined if the platform has more than one
// function ABI
#undef __HAS_MULTIPLE_ABIS__

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
#define __i386__ 1
#define __LP32__ 1
#define __SMALL__ 1
#define __HAS_MULTIPLE_ABIS__ 1
#elif defined(_M_X64)
#define __64_BIT__ 1
#define __LITTLE_ENDIAN__ 1
#define __X86_64__ 1
#define __LLP64__ 1
#define __MEDIUM__ 1
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

#if defined(__GNUC__) && defined(__APPLE__) && !TARGET_OS_IPHONE

// Compiler
#define __GCC__ 1

// Platform
#define __MAC__ 1

// Architecture
#if defined(__i386)
#define __32_BIT__ 1
#define __LITTLE_ENDIAN__ 1
#define __i386__ 1
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
#define __MEDIUM__ 1
#endif

// Native char set
#define __MACROMAN__ 1

// Native line endings
#define __CR__ 1

// Presence of CoreFoundation
#define __HAS_CORE_FOUNDATION__ (1)

#endif

////////////////////////////////////////////////////////////////////////////////
//
//  CONFIGURE DEFINITIONS FOR LINUX
//

#if defined(__GNUC__) && !defined(__APPLE__) && !defined(__PLATFORM_IS_ANDROID__) && !defined(__EMSCRIPTEN__)

// Compiler
#define __GCC__ 1

// Platform
#define __LINUX__ 1

// Architecture
#if defined(__i386)
#define __32_BIT__ 1
#define __LITTLE_ENDIAN__ 1
#define __i386__ 1
#define __LP32__ 1
#define __SMALL__
#elif defined(__x86_64__)
#define __64_BIT__ 1
#define __LITTLE_ENDIAN__ 1
#define __X86_64__ 1
#define __LP64__ 1
#define __MEDIUM__ 1
#elif defined(__arm__)
#define __32_BIT__ 1
#define __LITTLE_ENDIAN__ 1
#define __ARM__ 1
#define __LP32__ 1
#define __SMALL__ 1
#elif defined(__aarch64__)
#define __64_BIT__ 1
#define __LITTLE_ENDIAN__ 1
#define __ARM64__ 1
#define __LP64__ 1
#define __MEDIUM__ 1
#endif

// Native char set
#define __ISO_8859_1__ 1

// Native line endings
#define __LF__ 1

#endif

////////////////////////////////////////////////////////////////////////////////
//
//  CONFIGURE DEFINITIONS FOR IOS
//

#if defined(__GNUC__) && defined(__APPLE__) && TARGET_OS_IPHONE

// Compiler
#define __GCC__ 1

// Platform
#define __IOS__ 1

// Architecture
#if defined(__i386)
#define __32_BIT__ 1
#define __LITTLE_ENDIAN__ 1
#define __i386__ 1
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
#define __MEDIUM__ 1
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
#define __MEDIUM__ 1
#endif

// Native char set
#define __MACROMAN__ 1

// Native line endings
#define __CR__ 1

// Presence of CoreFoundation
#define __HAS_CORE_FOUNDATION__ (1)

#endif

////////////////////////////////////////////////////////////////////////////////
//
//  CONFIGURE DEFINITIONS FOR ANDROID
//

#if defined(__GNUC__) && !defined(__APPLE__) && defined(__PLATFORM_IS_ANDROID__)

// Compiler
#define __GCC__ (1)

// Platform
#define __ANDROID__ (1)

// Architecture
#if defined(__i386)
#define __32_BIT__ (1)
#define __LITTLE_ENDIAN__ (1)
#define __i386__ (1)
#define __LP32__ (1)
#define __SMALL__ (1)
#elif defined(__x86_64__)
#define __64_BIT__ (1)
#define __LITTLE_ENDIAN__ (1)
#define __X86_64__ (1)
#define __LP64__ (1)
#define __MEDIUM__ (1)
#elif defined(__arm__)
#define __32_BIT__ (1)
#define __LITTLE_ENDIAN__ (1)
#define __ARM__ (1)
#define __LP32__ (1)
#define __SMALL__ (1)
#elif defined(__aarch64__)
#define __64_BIT__ 1
#define __LITTLE_ENDIAN__ 1
#define __ARM64__ 1
#define __LP64__ 1
#define __MEDIUM__ 1
#endif

// Native char set
#define __ISO_8859_1__ (1)

// Native line endings
#define __LF__ (1)

#endif

////////////////////////////////////////////////////////////////////////////////
//
//  CONFIGURE DEFINITIONS FOR EMSCRIPTEN JS

#if defined(__EMSCRIPTEN__)

// Nasty, evil hack -- remove me
#define __LITTLE_ENDIAN__ 1

// Compiler
#define __GCC__ (1)

// Architecture
#define __32_BIT__ (1)
#define __LP32__ (1)
#define __SMALL__ (1)

// Native char set
#define __ISO_8859_1__ (1)

// Native line endings
#define __LF__ (1)

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
//  COMPILER HINT MACROS
//

// If we are using GCC or Clang, we can give the compiler various
// hints. This is particularly useful for Clang's static analysis
// feature.
#if defined(__GNUC__) || defined (__clang__) || defined (__llvm__)
#  define ATTRIBUTE_NORETURN  __attribute__((__noreturn__))
#  define ATTRIBUTE_UNUSED __attribute__((__unused__))
#  define ATTRIBUTE_PURE __attribute__((__pure__))
#else
#  define ATTRIBUTE_NORETURN
#  define ATTRIBUTE_UNUSED
#  define ATTRIBUTE_PURE
#endif

////////////////////////////////////////////////////////////////////////////////
//
//  SYMBOL EXPORTS
//

/* MC_DLLEXPORT should be applied to declarations.  MC_DLLEXPORT_DEF
 * should be applied to definitions. */
#ifdef _WIN32
/* On Windows, declaring something as having "dllexport" storage
 * modifies the naming of the corresponding symbol, so the export
 * attribute must be attached to declarations (and possibly to the
 * definition *as well* if no separate declaration appears) */
#  ifdef _MSC_VER
#    define MC_DLLEXPORT __declspec(dllexport)
#  else
#    define MC_DLLEXPORT __attribute__((dllexport))
#  endif
#  define MC_DLLEXPORT_DEF MC_DLLEXPORT
#else
/* On non-Windows platforms, the external visibility of a symbol is
 * simply a property of its definition (i.e. whether or not it should
 * appear in the list of exported symbols). */
#  define MC_DLLEXPORT
#  define MC_DLLEXPORT_DEF __attribute__((__visibility__("default"), __used__))
#endif

////////////////////////////////////////////////////////////////////////////////
//
//  C++ COMPATIBILITY
//

#ifndef __has_feature
#  define __has_feature(x)	0
#endif
#ifndef __has_extension
#  define __has_extension(x) __has_feature(x)
#endif

// Ensure we have alignof(...) available
#if defined(__cplusplus) && !__has_feature(cxx_alignof)
// Testing __cplusplus isn't sufficient as some compilers changed the value before being fully-conforming
#  if defined(__clang__)
     // No need for a version check; Clang supports __has_feature as a built-in
     // so if we get here, it isn't supported
#    define alignof(x)      __alignof__(x)
#  elif defined(__GNUC__)
     // GCC added C++11 alignof(x) in GCC 4.8
#    if __GNUC__ < 4 || (__GNUC__ == 4 && __GNUC_MINOR__ < 8)
#      define alignof(x)    __alignof__(x)
#    endif
#  elif defined(_MSC_VER)
     // MSVC added C++11 alignof(x) in Visual Studio 2012 (compiler version 11.0, _MSC_VER 1700)
#    if (_MSC_VER < 1700)
#      define alignof(x)    __alignof(x)
#    endif
#  else
#    error Do not know how to get alignof(x) on this compiler
#  endif
#endif

////////////////////////////////////////////////////////////////////////////////
//
//  FIXED WIDTH INTEGER TYPES
//

#if !defined(__STDC_LIMIT_MACROS)
#	define __STDC_LIMIT_MACROS 1
#endif
#include <stddef.h>
#include <stdint.h>
#include <stddef.h>
#include <limits.h>

#ifndef UINT8_MIN
#define UINT8_MIN (0U)
#endif
#ifndef UINT16_MIN
#define UINT16_MIN (0U)
#endif
#ifndef UINT32_MIN
#define UINT32_MIN (0U)
#endif
#ifndef UINT64_MIN
#define UINT64_MIN (0ULL)
#endif

////////////////////////////////////////////////////////////////////////////////
//
//  DERIVED INTEGER TYPES
//

#undef __HAVE_SSIZE_T__
#if !defined(__VISUALC__)
#	define __HAVE_SSIZE_T__ 1
#endif /* !__VISUALC__ */

#ifdef __32_BIT__

typedef int32_t integer_t;
typedef uint32_t uinteger_t;

typedef int32_t intenum_t;
typedef uint32_t intset_t;

#define INTEGER_MIN INT32_MIN
#define INTEGER_MAX INT32_MAX
#define UINTEGER_MIN UINT32_MIN
#define UINTEGER_MAX UINT32_MAX

#define UINTPTR_MIN UINT32_MIN

#else /* !__32_BIT__ */

typedef int32_t integer_t;
typedef uint32_t uinteger_t;

typedef int32_t intenum_t;
typedef uint32_t intset_t;

#define INTEGER_MIN INT32_MIN
#define INTEGER_MAX INT32_MAX
#define UINTEGER_MIN UINT32_MIN
#define UINTEGER_MAX UINT32_MAX

#define UINTPTR_MIN UINT64_MIN

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

#elif defined(__LARGE__)

// This memory model would allow 64-bit indicies in Foundation types. This,
// however, has never been tested and so is only here for completeness at
// this time.

typedef uint64_t uindex_t;
typedef int64_t index_t;
typedef uint64_t hash_t;
typedef int64_t compare_t;

#define UINDEX_MIN UINT64_MIN
#define UINDEX_MAX UINT64_MAX
#define INDEX_MIN INT64_MIN
#define INDEX_MAX INT64_MAX

#else

#error No memory model defined for this platform and architecture combination

#endif

#if !defined(__HAVE_SSIZE_T__)
typedef intptr_t ssize_t;

#	define SSIZE_MAX INTPTR_MAX
#endif

#define SIZE_MIN UINTPTR_MIN
#define SSIZE_MIN INTPTR_MIN

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

// The 'byte_t' type is used to hold a char in a binary string
// (native).  This cannot be anything other than to be "unsigned char"
// because we require the "sizeof" C++ operator to return sizes in
// units of byte_t, and because we require it to be valid to cast to a
// "byte_t*" in order to examine the object representation of a value.
typedef unsigned char byte_t;

// Constants used to represent the minimum and maximum values of a byte_t.
// We require bytes to be 8 bits in size.
static_assert(CHAR_BIT == 8, "Byte size is not 8 bits");
#define BYTE_MIN (0)
#define BYTE_MAX (255)

// The 'codepoint_t' type is used to hold a single Unicode codepoint (20-bit
// value).
typedef uint32_t codepoint_t;

// Constant to be used to represent an invalid codepoint (e.g. "no
// further characters)
#define CODEPOINT_NONE UINT32_MAX

// Constants used to represent the minimum and maximum values of a codepoint_t.
#define CODEPOINT_MIN 0
#define CODEPOINT_MAX 0x10ffff

// The 'unichar_t' type is used to hold a UTF-16 codeunit.
#ifdef __WINDOWS__
typedef wchar_t unichar_t;
#else
typedef uint16_t unichar_t;
#endif

// Constants used to represent the minimum and maximum values of a unichar_t.
#define UNICHAR_MIN UINT16_MIN
#define UNICHAR_MAX UINT16_MAX

#ifdef __WINDOWS__
typedef wchar_t *BSTR;
#endif

#if defined(__HAS_CORE_FOUNDATION__)
typedef const void *CFTypeRef;
typedef const struct __CFBoolean *CFBooleanRef;
typedef const struct __CFNumber *CFNumberRef;
typedef const struct __CFString *CFStringRef;
typedef const struct __CFData *CFDataRef;
typedef const struct __CFArray *CFArrayRef;
typedef const struct __CFDictionary *CFDictionaryRef;
#endif

////////////////////////////////////////////////////////////////////////////////
//
//  POINTER TYPES
//

#if defined(__cplusplus) /* C++ */
#   define nil nullptr

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
typedef struct __MCTypeInfo *MCTypeInfoRef;
typedef struct __MCNull *MCNullRef;
typedef struct __MCBoolean *MCBooleanRef;
typedef struct __MCNumber *MCNumberRef;
typedef struct __MCString *MCStringRef;
typedef struct __MCName *MCNameRef;
typedef struct __MCData *MCDataRef;
typedef struct __MCArray *MCArrayRef;
typedef struct __MCHandler *MCHandlerRef;
typedef struct __MCList *MCListRef;
typedef struct __MCSet *MCSetRef;
typedef struct __MCRecord *MCRecordRef;
typedef struct __MCError *MCErrorRef;
typedef struct __MCStream *MCStreamRef;
typedef struct __MCProperList *MCProperListRef;
typedef struct __MCForeignValue *MCForeignValueRef;
typedef struct __MCJavaObject *MCJavaObjectRef;
typedef struct __MCObjcObject *MCObjcObjectRef;

// Forward declaration
typedef struct __MCLocale* MCLocaleRef;

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
//
//  REQUIRED STANDARD INCLUDES
//

// Required so that MSVC's <math.h> defines non-standard mathematical constants
#define _USE_MATH_DEFINES

#include <foundation-stdlib.h>
#include <math.h>

////////////////////////////////////////////////////////////////////////////////
//
//  NEW AND DELETE OPERATORS
//

#ifdef __cplusplus
# include <new>
using std::nothrow;
#endif

////////////////////////////////////////////////////////////////////////////////
//
//  NARROWING CONVERSIONS
//

#include <utility>
#include <type_traits>

/* A searchable way to do narrowing casts of numeric values (e.g. from
 * uint32_t to uint8_t or from uindex_t to std::ptrdiff_t).  Use in
 * preference to a C-style cast or a raw static_cast.  Should be used
 * when you're totally certain that overflow/underflow has been
 * logically ruled out elsewhere. */
template <typename To, typename From>
inline constexpr To MCNarrowCast(From p_from) noexcept
{
    return static_cast<To>(std::forward<From>(p_from));
}

/* Checked narrowing conversion of numeric values.  Use when there's a
 * possibility that the input value might not fit into the output
 * type, and you want to check.  Note that this is safe to use in
 * generic/template code; if To can represent all values of From, it
 * optimises to nothing.*/
template <typename To, typename From>
inline bool MCNarrow(From p_from, To& r_result)
{
    To t_to = static_cast<To>(p_from);
    if (static_cast<From>(t_to) != p_from)
        return false;
    if ((std::is_signed<From>::value != std::is_signed<To>::value) &&
        ((t_to < To{}) != (p_from < From{})))
        return false;
    r_result = t_to;
    return true;
}

////////////////////////////////////////////////////////////////////////////////
//
//  MINIMUM FUNCTIONS
//

// TODO: re-write when we adopt C++11
template <class T, class U> inline T MCMin(T a, U b) { return a < b ? a : T(b); }

////////////////////////////////////////////////////////////////////////////////
//
//  MAXIMUM FUNCTIONS
//

// TODO: re-write when we adopt C++11
template <class T, class U> inline T MCMax(T a, U b) { return a > b ? a : T(b); }

////////////////////////////////////////////////////////////////////////////////
//
//  ABSOLUTE VALUE FUNCTIONS
//

inline uint32_t MCAbs(int32_t a) { return uint32_t(a < 0 ? -a : a); }
inline uint64_t MCAbs(int64_t a) { return uint64_t(a < 0 ? -a : a); }
inline float MCAbs(float a) { return fabsf(a); }
inline double MCAbs(double a) { return fabs(a); }

////////////////////////////////////////////////////////////////////////////////
//
//  SIGN FUNCTIONS
//

template <class T> inline compare_t MCSgn(T a) { return a < 0 ? -1 : (a > 0 ? 1 : 0); }

////////////////////////////////////////////////////////////////////////////////
//
//  COMPARE FUNCTIONS
//

template <typename T> inline compare_t MCCompare(T a, T b) { return ((a < b) ? -1 : ((a > b) ? 1 : 0)); }

////////////////////////////////////////////////////////////////////////////////
//
//  COMPARE FUNCTIONS
//

template <typename T> inline bool MCIsPowerOfTwo(T x) { return (x & (x - 1)) == 0; }

template <typename T, typename U, typename V>
inline T MCClamp(T value, U min, V max) {
	return MCMax(MCMin(value, max), min);
}

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

#ifdef __LITTLE_ENDIAN__
const MCByteOrder kMCByteOrderHost = kMCByteOrderLittleEndian;
#else
const MCByteOrder kMCByteOrderHost = kMCByteOrderBigEndian;
#endif

constexpr MCByteOrder MCByteOrderGetCurrent(void)
{
	return kMCByteOrderHost;
}

constexpr uint8_t MCSwapInt(uint8_t x) { return x; }
constexpr int8_t MCSwapInt(int8_t x) { return x; }

constexpr uint16_t MCSwapInt(uint16_t x)
{
	return (uint16_t)(x >> 8) | (uint16_t)(x << 8);
}
constexpr int16_t MCSwapInt(int16_t x) { return int16_t(MCSwapInt(uint16_t(x))); }

constexpr uint32_t MCSwapInt(uint32_t x)
{
	return (x >> 24) | ((x >> 8) & 0xff00) | ((x & 0xff00) << 8) | (x << 24);
}
constexpr int32_t MCSwapInt(int32_t x) { return int32_t(MCSwapInt(uint32_t(x))); }

constexpr uint64_t MCSwapInt(uint64_t x)
{
	return (x >> 56) | ((x >> 40) & 0xff00) | ((x >> 24) & 0xff0000) | ((x >> 8) & 0xff000000) |
			((x & 0xff000000) << 8) | ((x & 0xff0000) << 24) | ((x & 0xff00) << 40) | (x << 56);
}
constexpr int64_t MCSwapInt(int64_t x) { return int16_t(MCSwapInt(uint64_t(x))); }

constexpr uint16_t MCSwapInt16(uint16_t x) { return MCSwapInt(x); }
constexpr uint32_t MCSwapInt32(uint32_t x) { return MCSwapInt(x); }
constexpr uint64_t MCSwapInt64(uint64_t x) { return MCSwapInt(x); }

template <typename T>
constexpr T MCSwapIntBigToHost(T x)
{
	return (kMCByteOrderHost == kMCByteOrderBigEndian) ? x : MCSwapInt(x);
}
template <typename T>
constexpr T MCSwapIntHostToBig(T x) { return MCSwapIntBigToHost(x); }

template <typename T>
constexpr T MCSwapIntNetworkToHost(T x) { return MCSwapIntBigToHost(x); }
template <typename T>
constexpr T MCSwapIntHostToNetwork(T x) { return MCSwapIntHostToBig(x); }

template <typename T>
constexpr T MCSwapIntLittleToHost(T x)
{
	return (kMCByteOrderHost == kMCByteOrderLittleEndian) ? x : MCSwapInt(x);
}
template <typename T>
constexpr T MCSwapIntHostToLittle(T x) { return MCSwapIntLittleToHost(x); }

constexpr uint16_t MCSwapInt16BigToHost(uint16_t x) {return MCSwapIntBigToHost(x);}
constexpr uint32_t MCSwapInt32BigToHost(uint32_t x) {return MCSwapIntBigToHost(x);}
constexpr uint64_t MCSwapInt64BigToHost(uint64_t x) {return MCSwapIntBigToHost(x);}

constexpr uint16_t MCSwapInt16HostToBig(uint16_t x) {return MCSwapIntHostToBig(x);}
constexpr uint32_t MCSwapInt32HostToBig(uint32_t x) {return MCSwapIntHostToBig(x);}
constexpr uint64_t MCSwapInt64HostToBig(uint64_t x) {return MCSwapIntHostToBig(x);}

constexpr uint16_t MCSwapInt16LittleToHost(uint16_t x) {return MCSwapIntLittleToHost(x);}
constexpr uint32_t MCSwapInt32LittleToHost(uint32_t x) {return MCSwapIntLittleToHost(x);}
constexpr uint64_t MCSwapInt64LittleToHost(uint64_t x) {return MCSwapIntLittleToHost(x);}

constexpr uint16_t MCSwapInt16HostToLittle(uint16_t x) {return MCSwapIntHostToLittle(x);}
constexpr uint32_t MCSwapInt32HostToLittle(uint32_t x) {return MCSwapIntHostToLittle(x);}
constexpr uint64_t MCSwapInt64HostToLittle(uint64_t x) {return MCSwapIntHostToLittle(x);}

constexpr uint16_t MCSwapInt16NetworkToHost(uint16_t x) {return MCSwapIntNetworkToHost(x);}
constexpr uint32_t MCSwapInt32NetworkToHost(uint32_t x) {return MCSwapIntNetworkToHost(x);}
constexpr uint64_t MCSwapInt64NetworkToHost(uint64_t x) {return MCSwapIntNetworkToHost(x);}

constexpr uint16_t MCSwapInt16HostToNetwork(uint16_t x) {return MCSwapIntHostToNetwork(x);}
constexpr uint32_t MCSwapInt32HostToNetwork(uint32_t x) {return MCSwapIntHostToNetwork(x);}
constexpr uint64_t MCSwapInt64HostToNetwork(uint64_t x) {return MCSwapIntHostToNetwork(x);}

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

inline MCRange MCRangeMakeMinMax(uindex_t p_min, uindex_t p_max)
{
	if (p_min > p_max)
		return MCRangeMake(p_max, 0);
	return MCRangeMake(p_min, p_max - p_min);
}

inline MCRange MCRangeSetMinimum(const MCRange &p_range, uindex_t p_min)
{
	return MCRangeMakeMinMax(p_min, p_range.offset + p_range.length);
}

inline MCRange MCRangeSetMaximum(const MCRange &p_range, uindex_t p_max)
{
	return MCRangeMakeMinMax(p_range.offset, p_max);
}

inline MCRange MCRangeIncrementOffset(const MCRange &p_range, uindex_t p_increment)
{
	return MCRangeSetMinimum(p_range, p_range.offset + p_increment);
}

inline bool MCRangeIsEqual(const MCRange &p_left, const MCRange &p_right)
{
	return p_left.offset == p_right.offset && p_left.length == p_right.length;
}

inline bool MCRangeIsEmpty(const MCRange &p_range)
{
	return p_range.length == 0;
}

inline MCRange MCRangeIntersection(const MCRange &p_left, const MCRange &p_right)
{
	uindex_t t_start, t_end;
	t_start = MCMax(p_left.offset, p_right.offset);
	t_end = MCMin(p_left.offset + p_left.length, p_right.offset + p_right.length);
	
	return MCRangeMakeMinMax(t_start, t_end);
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
//  DEBUG HANDLING
//

#if defined(_DEBUG)
#	define DEBUG_LOG 1
#endif

#define MCUnreachableReturn(x) { MCUnreachable(); return x;}

// If we are using GCC or Clang, we can give the compiler the hint that the
// assertion functions do not return. This is particularly useful for Clang's
// static analysis feature.
#if defined(__GNUC__) || defined (__clang__) || defined (__llvm__)
#define ATTRIBUTE_NORETURN  __attribute__((__noreturn__))
#else
#define ATTRIBUTE_NORETURN
#endif

#if defined(DEBUG_LOG)

extern void __MCAssert(const char *file, uint32_t line, const char *message) ATTRIBUTE_NORETURN;
#define MCAssert(m_expr) (void)( (!!(m_expr)) || (__MCAssert(__FILE__, __LINE__, #m_expr), 0) )

extern void __MCLog(const char *file, uint32_t line, const char *format, ...);
#define MCLog(...) __MCLog(__FILE__, __LINE__, __VA_ARGS__)

extern void __MCLogWithTrace(const char *file, uint32_t line, const char *format, ...);
#define MCLogWithTrace(...) __MCLogWithTrace(__FILE__, __LINE__, __VA_ARGS__)

extern void __MCUnreachable(void) ATTRIBUTE_NORETURN;
#define MCUnreachable() __MCUnreachable();

#else

#define MCAssert(expr)  (void)(0 ? (expr) : 0)

#define MCLog(...) (void)(0 ? (__VA_ARGS__) : 0)

#define MCLogWithTrace(...) (void)(0 ? (__VA_ARGS__) : 0)

#if defined(__clang__) || (defined(__GNUC__) && (__GNUC__ >  4 || (__GNUC__ == 4 && __GNUC_MINOR__ > 5)))

#define MCUnreachable() __builtin_unreachable()

#else // Neither GCC >= 4.5 or Clang compiler

#define MCUnreachable()

#endif

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

extern "C" {
    
////////////////////////////////////////////////////////////////////////////////
//
//  BYTE-WISE OPERATIONS
//

// Clear the given block of memory to all 0's.
inline void MCMemoryClear(void *dst, size_t size) { memset(dst, 0, size); }

// Clear the given block of memory to all 0's, ensuring that the
// compiler never optimises it out.  Use this when clearing sensitive
// data from memory.
MC_DLLEXPORT void MCMemoryClearSecure(byte_t* dst, size_t size);

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

//////////

}

// Clear the memory of the given structure to all 0's
template <typename T> void inline MCMemoryClear(T&p_struct)
{
	MCMemoryClear(&p_struct, sizeof(T));
}

// Securely clear the memory of the given structure to all 0's
template <typename T>
void inline MCMemoryClearSecure(T& p_struct)
{
    MCMemoryClearSecure(reinterpret_cast<byte_t*>(&p_struct),
                        sizeof(p_struct));
}

// Re-initialise an object to its default-constructed state
template <typename T> void inline MCMemoryReinit(T& p_object)
{
    // Run the destructor then default constructor
    p_object.~T();
    new (&p_object) T();
}

extern "C" {

////////////////////////////////////////////////////////////////////////////////
//
//  RESIZABLE BLOCK ALLOCATION (UNINITIALIZED)
//

// This method returns an unitialized block of memory of the given size in
// block. An error is raised if allocation fails.
MC_DLLEXPORT bool MCMemoryAllocate(size_t size, void*& r_block);

// This method returns an block of memory containing the same contents as the
// provided one.
MC_DLLEXPORT bool MCMemoryAllocateCopy(const void *block, size_t size, void*& r_new_block);

// This method reallocates a block of memory allocated using MCMemoryAllocate.
// Any new space allocated is uninitialized. The new pointer to the block is
// returned. Note that the block may move regardless of new size. If the input
// block is nil, it is treated as an allocate call.
MC_DLLEXPORT bool MCMemoryReallocate(void *block, size_t new_size, void*& r_new_block);

// This method deallocates a block of memory allocated using MCMemoryAllocate,
// or subsequently reallocated using MCMemoryReallocate. The block passed in
// may be nil.
MC_DLLEXPORT void MCMemoryDeallocate(void *block);

//////////

}
    
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

template<typename T> void MCMemoryDeallocate(T* p_block) {
    MCMemoryDeallocate (static_cast<void*>(p_block));
}

extern "C" {
    
////////////////////////////////////////////////////////////////////////////////
//
//  FIXED-SIZE RECORD ALLOCATION (INITIALIZED)
//

// This method allocates a fixed size record, and initializes its bytes to
// zero. An error is raised if allocation fails.
//
// Note that a block allocated with New must be freed with Delete.
MC_DLLEXPORT bool MCMemoryNew(size_t size, void*& r_record);

// This method deletes a fixed size record that was allocated with MCMemoryNew.
MC_DLLEXPORT void MCMemoryDelete(void *p_record);

//////////

// SN-2014-06-19 [[ Bug 12651 ]] back key can not work, and it crush
// The placement new is needed in MCMemorytNew
// to properly allocate the classes in MCMessageEvent::create

}

template<typename T> bool MCMemoryNew(T*& r_record)
{
	void *t_record;
	if (MCMemoryNew(sizeof(T), t_record))
	{
        r_record = new (t_record) T;
		return true;
	}
	return false;
}

template<typename T> void MCMemoryDelete(T* p_record)
{
	MCMemoryDelete(static_cast<void *>(p_record));
}

// Allocates a block of memory for an object and default-constructs it
// (basically, ::operator new)
template <typename T>
bool MCMemoryCreate(T*& r_object)
{
    // Allocate the memory then default-construct
    if (!MCMemoryNew(r_object))
        return false;
    
    new (r_object) T();
    return true;
}

// De-allocates a block of memory after running the object's destructor
// (basically, ::operator delete).
template <typename T>
void MCMemoryDestroy(T* p_object)
{
    // Run the object's destructor then delete it
    p_object->~T();
    MCMemoryDelete(p_object);
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

// Array allocator that default-constructs all elements
template <typename T>
bool MCMemoryNewArrayInit(uindex_t p_count, T*& r_array, uindex_t& r_count)
{
    if (MCMemoryNewArray(p_count, r_array, r_count))
    {
        // Default-construct all elements in the array
        for (uindex_t i = 0; i < r_count; i++)
            new (&r_array[i]) T();
        return true;
    }
    
    return false;
}

// Array allocator that default-constructs all elements
template <typename T>
bool MCMemoryNewArrayInit(uindex_t p_count, T*& r_array)
{
    uindex_t t_alloc_count;
    return MCMemoryNewArrayInit(p_count, r_array, t_alloc_count);
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

template <typename T>
bool MCMemoryResizeArrayInit(uindex_t p_new_count, T*& x_array, uindex_t& x_count)
{
    // Capture the current count before resizing
    uindex_t t_current_count = x_count;
    if (MCMemoryResizeArray(p_new_count, x_array, x_count))
    {
        // Default construct any new elements that were allocated
        for (uindex_t i = t_current_count; i < p_new_count; i++)
            new (&x_array[i]) T();
        return true;
    }
    
    return false;
}

template<typename T> void MCMemoryDeleteArray(T* p_array)
{
	MCMemoryDeleteArray(static_cast<void *>(p_array));
}

// Array deleter that runs the destructor for each element
template <typename T>
void MCMemoryDeleteArray(T* p_array, uindex_t N)
{
    // Run the destructor for each of the elements
    for (size_t i = 0; i < N; i++)
        p_array[i].~T();
    
    // Destroy the array
    MCMemoryDeleteArray(p_array);
}

extern "C" {

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
//
//  HASHING UTILITIES
//

// CAVEAT: The values returned by any Hash function are not guaranteed to remain
//   the same from version to version. In particular, never serialize a hash
//   value - recompute on unserialization of the object.

// Return a hash for the given bool.
MC_DLLEXPORT hash_t MCHashBool(bool);

// Return a hash for the given integer.
MC_DLLEXPORT hash_t MCHashInteger(integer_t);
MC_DLLEXPORT hash_t MCHashUInteger(uinteger_t);
MC_DLLEXPORT hash_t MCHashSize(ssize_t);
MC_DLLEXPORT hash_t MCHashUSize(size_t);
MC_DLLEXPORT hash_t MCHashInt64(int64_t);
MC_DLLEXPORT hash_t MCHashUInt64(uint64_t);

// Return a hash value for the given double - note that (hopefully!) hashing
// an integer stored as a double will be the same as hashing the integer.
MC_DLLEXPORT hash_t MCHashDouble(double d);

// Returns a hash value for the given pointer.
MC_DLLEXPORT hash_t MCHashPointer(const void *p);

// Returns a hash value for the given sequence of bytes.
MC_DLLEXPORT hash_t MCHashBytes(const void *bytes, size_t byte_count);

// Returns a hash value for the given sequence of bytes, continuing a previous
// hashing sequence (byte_count should be a multiple of 4).
MC_DLLEXPORT hash_t MCHashBytesStream(hash_t previous, const void *bytes, size_t byte_count);

// Returns a hash value for the given sequence of native chars. The chars are
// folded before being processed.
MC_DLLEXPORT hash_t MCHashNativeChars(const char_t *chars,
                                      size_t char_count);

// Returns a hash value for the given sequence of code units. The chars are
// normalized and folded before being processed.
MC_DLLEXPORT hash_t MCHashChars(const unichar_t *chars,
                                size_t char_count);

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
    kMCValueTypeCodeProperList,
	kMCValueTypeCodeCustom,
	kMCValueTypeCodeRecord,
	kMCValueTypeCodeHandler,
	kMCValueTypeCodeTypeInfo,
    kMCValueTypeCodeError,
    kMCValueTypeCodeForeignValue,
};

enum
{
	kMCValueCustomHeaderSize = sizeof(uint32_t) + sizeof(uint32_t) + sizeof(uintptr_t)
};

/* If you add a new function pointer to this structure, don't forget
 * to add a default implementation of the function to
 * foundation-custom.cpp */
struct MCValueCustomCallbacks
{
	bool is_singleton : 1;
	void (*destroy)(MCValueRef value);
	bool (*copy)(MCValueRef value, bool release, MCValueRef& r_value);
	bool (*equal)(MCValueRef value, MCValueRef other_value);
	hash_t (*hash)(MCValueRef value);
	bool (*describe)(MCValueRef value, MCStringRef& r_desc);
    
    bool (*is_mutable)(MCValueRef value);
    bool (*mutable_copy)(MCValueRef, bool release, MCValueRef& r_value);
};

// Create a custom value with the given callbacks.
MC_DLLEXPORT bool MCValueCreateCustom(MCTypeInfoRef typeinfo, size_t extra_bytes, MCValueRef& r_value);

// Fetch the typecode of the given value.
MC_DLLEXPORT MCValueTypeCode MCValueGetTypeCode(MCValueRef value);

// Fetch the typeinfo of the given value.
MC_DLLEXPORT MCTypeInfoRef MCValueGetTypeInfo(MCValueRef value);

// Fetch the retain count.
MC_DLLEXPORT uindex_t MCValueGetRetainCount(MCValueRef value);

// This only works for custom valuerefs at the moment!
MC_DLLEXPORT bool MCValueIsMutable(MCValueRef value);

MC_DLLEXPORT bool MCValueIsEmpty(MCValueRef value);
MC_DLLEXPORT bool MCValueIsArray(MCValueRef value);

// Reduce the reference count of the given value by one, destroying the value
// if it reaches 0. Note that (for convience) 'value' can be nil, it which case
// the call has no effect.
MC_DLLEXPORT void MCValueRelease(MCValueRef value);

// Increment the reference count of the given value by one.
MC_DLLEXPORT MCValueRef MCValueRetain(MCValueRef value);

// Copies the given value ensuring the resulting value is immutable (which is
// why it can fail).
MC_DLLEXPORT bool MCValueCopy(MCValueRef value, MCValueRef& r_immutable_copy);
MC_DLLEXPORT bool MCValueCopyAndRelease(MCValueRef value, MCValueRef& r_immutable_copy);

// Copies the given value as a mutable value - only works for custom valuerefs at the moment.
MC_DLLEXPORT bool MCValueMutableCopy(MCValueRef value, MCValueRef& r_immutable_copy);
MC_DLLEXPORT bool MCValueMutableCopyAndRelease(MCValueRef value, MCValueRef& r_immutable_copy);

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
MC_DLLEXPORT bool MCValueIsEqualTo(MCValueRef value, MCValueRef other_value);

// Returns a hash value for the (exact) content of the value - in particular
// no folding is done for strings or names. Note that if the hash of two values
// is equal it does not mean the values are equal; although the converse is true
// (by definition of IsEqualTo).
MC_DLLEXPORT hash_t MCValueHash(MCValueRef value);

// Returns a string description of the given value suitable for display and
// debugging (although not necessarily for general string formatting).
MC_DLLEXPORT bool MCValueCopyDescription(MCValueRef value, MCStringRef& r_desc);

// Returns true if pointer comparison is for the value is enough to determine
// equality. This is always true of booleans, nulls and names; other values
// must be interred first.
MC_DLLEXPORT bool MCValueIsUnique(MCValueRef value);

// Inter the given value returning a new (immutable) value. Any two values that
// are equal as defined by IsEqualTo will inter to the same object. i.e. For
// interred values ref equivalence (i.e. comparing the pointers) is equivalent
// to IsEqualTo.
//
// Note that for singleton objects (nulls, booleans and names) interring just
// bumps the reference count and returns the same value as they already satisfy
//   x == y iff IsEqualTo(x, y)
//
MC_DLLEXPORT bool MCValueInter(MCValueRef value, MCValueRef& r_unique_value);

// As the 'Inter' method except that 'value' will be released. This allows
// optimization in some cases as the original value can be (potentially) reused
// to build the unique one (cutting down on copying).
//
MC_DLLEXPORT bool MCValueInterAndRelease(MCValueRef value, MCValueRef& r_unique_value);

// Fetch the 'extra bytes' field for the given custom value.
inline void *MCValueGetExtraBytesPtr(MCValueRef value) { return ((uint8_t *)value) + kMCValueCustomHeaderSize; }

#if defined(_DEBUG)
// Emit a debug log message containing the description of the value
MC_DLLEXPORT void MCValueLog(MCValueRef);
#endif
    
#if defined(__HAS_CORE_FOUNDATION__)
MC_DLLEXPORT bool MCValueCreateWithCFTypeRef(CFTypeRef p_cf_value, bool p_use_lists, MCValueRef& r_cf_value);
MC_DLLEXPORT bool MCValueConvertToCFTypeRef(MCValueRef p_value, bool p_use_lists, CFTypeRef& r_cf_value);
#endif
    
//////////

}

template<typename T> inline T MCValueRetain(T value)
{
	return (T)MCValueRetain((MCValueRef)value);
}

// Utility function for assigning to MCValueRef vars.
template<typename T> inline void MCValueAssign(T& dst, T src)
{
	if (src == dst)
		return;

	MCValueRetain(src);
	MCValueRelease(dst);
	dst = src;
}

// Utility function for assigning to MCValueRef vars.
template<typename T> inline void MCValueAssignAndRelease(T& dst, T src)
{
	if (src == dst)
		return;

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

template<typename T> inline bool MCValueCreateCustom(MCTypeInfoRef type, size_t p_extra_bytes, T& r_value)
{
	MCValueRef t_value;
	if (MCValueCreateCustom(type, p_extra_bytes, t_value))
		return r_value = (T)t_value, true;
	return false;
}

extern "C" {

////////////////////////////////////////////////////////////////////////////////
//
//  TYPE (META) DEFINITIONS
//

// A TypeInfoRef is a description of a type. TypeInfo's are uniqued objects with
// equality of typeinfo's defined by the typeinfo's kind. Once created, typeinfo's
// are equal iff their pointers are equal. (Note equal is not the same as conformance!)

// The 'any' type is essentially a union of all typeinfos.
MC_DLLEXPORT extern MCTypeInfoRef kMCAnyTypeInfo;

MC_DLLEXPORT MCTypeInfoRef MCAnyTypeInfo(void) ATTRIBUTE_PURE;

// These are typeinfos for all the 'builtin' valueref types.
MC_DLLEXPORT extern MCTypeInfoRef kMCNullTypeInfo;
MC_DLLEXPORT extern MCTypeInfoRef kMCBooleanTypeInfo;
MC_DLLEXPORT extern MCTypeInfoRef kMCNumberTypeInfo;
MC_DLLEXPORT extern MCTypeInfoRef kMCStringTypeInfo;
MC_DLLEXPORT extern MCTypeInfoRef kMCNameTypeInfo;
MC_DLLEXPORT extern MCTypeInfoRef kMCDataTypeInfo;
MC_DLLEXPORT extern MCTypeInfoRef kMCArrayTypeInfo;
MC_DLLEXPORT extern MCTypeInfoRef kMCSetTypeInfo;
MC_DLLEXPORT extern MCTypeInfoRef kMCListTypeInfo;
MC_DLLEXPORT extern MCTypeInfoRef kMCProperListTypeInfo;

MC_DLLEXPORT MCTypeInfoRef MCNullTypeInfo(void) ATTRIBUTE_PURE;
MC_DLLEXPORT MCTypeInfoRef MCBooleanTypeInfo(void) ATTRIBUTE_PURE;
MC_DLLEXPORT MCTypeInfoRef MCNumberTypeInfo(void) ATTRIBUTE_PURE;
MC_DLLEXPORT MCTypeInfoRef MCStringTypeInfo(void) ATTRIBUTE_PURE;
MC_DLLEXPORT MCTypeInfoRef MCNameTypeInfo(void) ATTRIBUTE_PURE;
MC_DLLEXPORT MCTypeInfoRef MCDataTypeInfo(void) ATTRIBUTE_PURE;
MC_DLLEXPORT MCTypeInfoRef MCArrayTypeInfo(void) ATTRIBUTE_PURE;
MC_DLLEXPORT MCTypeInfoRef MCSetTypeInfo(void) ATTRIBUTE_PURE;
MC_DLLEXPORT MCTypeInfoRef MCListTypeInfo(void) ATTRIBUTE_PURE;
MC_DLLEXPORT MCTypeInfoRef MCProperListTypeInfo(void) ATTRIBUTE_PURE;

MC_DLLEXPORT extern MCTypeInfoRef kMCBoolTypeInfo;
MC_DLLEXPORT MCTypeInfoRef MCForeignBoolTypeInfo(void) ATTRIBUTE_PURE;

MC_DLLEXPORT extern MCTypeInfoRef kMCUInt8TypeInfo;
MC_DLLEXPORT extern MCTypeInfoRef kMCSInt8TypeInfo;
MC_DLLEXPORT extern MCTypeInfoRef kMCUInt16TypeInfo;
MC_DLLEXPORT extern MCTypeInfoRef kMCSInt16TypeInfo;
MC_DLLEXPORT extern MCTypeInfoRef kMCUInt32TypeInfo;
MC_DLLEXPORT extern MCTypeInfoRef kMCSInt32TypeInfo;
MC_DLLEXPORT extern MCTypeInfoRef kMCUInt64TypeInfo;
MC_DLLEXPORT extern MCTypeInfoRef kMCSInt64TypeInfo;
MC_DLLEXPORT MCTypeInfoRef MCForeignUInt8TypeInfo(void) ATTRIBUTE_PURE;
MC_DLLEXPORT MCTypeInfoRef MCForeignSInt8TypeInfo(void) ATTRIBUTE_PURE;
MC_DLLEXPORT MCTypeInfoRef MCForeignUInt16TypeInfo(void) ATTRIBUTE_PURE;
MC_DLLEXPORT MCTypeInfoRef MCForeignSInt16TypeInfo(void) ATTRIBUTE_PURE;
MC_DLLEXPORT MCTypeInfoRef MCForeignUInt32TypeInfo(void) ATTRIBUTE_PURE;
MC_DLLEXPORT MCTypeInfoRef MCForeignSInt32TypeInfo(void) ATTRIBUTE_PURE;
MC_DLLEXPORT MCTypeInfoRef MCForeignUInt64TypeInfo(void) ATTRIBUTE_PURE;
MC_DLLEXPORT MCTypeInfoRef MCForeignSInt64TypeInfo(void) ATTRIBUTE_PURE;

MC_DLLEXPORT extern MCTypeInfoRef kMCFloatTypeInfo;
MC_DLLEXPORT extern MCTypeInfoRef kMCDoubleTypeInfo;
MC_DLLEXPORT MCTypeInfoRef MCForeignFloatTypeInfo(void) ATTRIBUTE_PURE;
MC_DLLEXPORT MCTypeInfoRef MCForeignDoubleTypeInfo(void) ATTRIBUTE_PURE;

MC_DLLEXPORT extern MCTypeInfoRef kMCPointerTypeInfo;
MC_DLLEXPORT MCTypeInfoRef MCForeignPointerTypeInfo(void) ATTRIBUTE_PURE;

MC_DLLEXPORT extern MCTypeInfoRef kMCUIntSizeTypeInfo;
MC_DLLEXPORT extern MCTypeInfoRef kMCSIntSizeTypeInfo;
MC_DLLEXPORT MCTypeInfoRef MCForeignUIntSizeTypeInfo(void) ATTRIBUTE_PURE;
MC_DLLEXPORT MCTypeInfoRef MCForeignSIntSizeTypeInfo(void) ATTRIBUTE_PURE;

MC_DLLEXPORT extern MCTypeInfoRef kMCUIntPtrTypeInfo;
MC_DLLEXPORT extern MCTypeInfoRef kMCSIntPtrTypeInfo;
MC_DLLEXPORT MCTypeInfoRef MCForeignUIntPtrTypeInfo(void) ATTRIBUTE_PURE;
MC_DLLEXPORT MCTypeInfoRef MCForeignSIntPtrTypeInfo(void) ATTRIBUTE_PURE;

MC_DLLEXPORT extern MCTypeInfoRef kMCNaturalUIntTypeInfo;
MC_DLLEXPORT extern MCTypeInfoRef kMCNaturalSIntTypeInfo;
MC_DLLEXPORT extern MCTypeInfoRef kMCNaturalFloatTypeInfo;
MC_DLLEXPORT MCTypeInfoRef MCForeignNaturalUIntTypeInfo(void) ATTRIBUTE_PURE;
MC_DLLEXPORT MCTypeInfoRef MCForeignNaturalSIntTypeInfo(void) ATTRIBUTE_PURE;
MC_DLLEXPORT MCTypeInfoRef MCForeignNaturalFloatTypeInfo(void) ATTRIBUTE_PURE;

MC_DLLEXPORT extern MCTypeInfoRef kMCCBoolTypeInfo;
MC_DLLEXPORT MCTypeInfoRef MCForeignCBoolTypeInfo(void) ATTRIBUTE_PURE;

MC_DLLEXPORT extern MCTypeInfoRef kMCCCharTypeInfo;
MC_DLLEXPORT extern MCTypeInfoRef kMCCUCharTypeInfo;
MC_DLLEXPORT extern MCTypeInfoRef kMCCSCharTypeInfo;
MC_DLLEXPORT extern MCTypeInfoRef kMCCUShortTypeInfo;
MC_DLLEXPORT extern MCTypeInfoRef kMCCSShortTypeInfo;
MC_DLLEXPORT extern MCTypeInfoRef kMCCUIntTypeInfo;
MC_DLLEXPORT extern MCTypeInfoRef kMCCSIntTypeInfo;
MC_DLLEXPORT extern MCTypeInfoRef kMCCULongTypeInfo;
MC_DLLEXPORT extern MCTypeInfoRef kMCCSLongTypeInfo;
MC_DLLEXPORT extern MCTypeInfoRef kMCCULongLongTypeInfo;
MC_DLLEXPORT extern MCTypeInfoRef kMCCSLongLongTypeInfo;
MC_DLLEXPORT MCTypeInfoRef MCForeignCCharTypeInfo(void) ATTRIBUTE_PURE;
MC_DLLEXPORT MCTypeInfoRef MCForeignCUCharTypeInfo(void) ATTRIBUTE_PURE;
MC_DLLEXPORT MCTypeInfoRef MCForeignCSCharTypeInfo(void) ATTRIBUTE_PURE;
MC_DLLEXPORT MCTypeInfoRef MCForeignCUShortTypeInfo(void) ATTRIBUTE_PURE;
MC_DLLEXPORT MCTypeInfoRef MCForeignCSShortTypeInfo(void) ATTRIBUTE_PURE;
MC_DLLEXPORT MCTypeInfoRef MCForeignCUIntTypeInfo(void) ATTRIBUTE_PURE;
MC_DLLEXPORT MCTypeInfoRef MCForeignCSIntTypeInfo(void) ATTRIBUTE_PURE;
MC_DLLEXPORT MCTypeInfoRef MCForeignCULongTypeInfo(void) ATTRIBUTE_PURE;
MC_DLLEXPORT MCTypeInfoRef MCForeignCSLongTypeInfo(void) ATTRIBUTE_PURE;
MC_DLLEXPORT MCTypeInfoRef MCForeignCULongLongTypeInfo(void) ATTRIBUTE_PURE;
MC_DLLEXPORT MCTypeInfoRef MCForeignCSLongLongTypeInfo(void) ATTRIBUTE_PURE;

MC_DLLEXPORT extern MCTypeInfoRef kMCUIntTypeInfo;
MC_DLLEXPORT extern MCTypeInfoRef kMCSIntTypeInfo;
MC_DLLEXPORT MCTypeInfoRef MCForeignUIntTypeInfo(void) ATTRIBUTE_PURE;
MC_DLLEXPORT MCTypeInfoRef MCForeignSIntTypeInfo(void) ATTRIBUTE_PURE;

//////////

// Returns true if the typeinfo is an alias.
MC_DLLEXPORT bool MCTypeInfoIsAlias(MCTypeInfoRef typeinfo);

// Returns true if the typeinfo is a name.
MC_DLLEXPORT bool MCTypeInfoIsNamed(MCTypeInfoRef typeinfo);

// Returns true if the typeinfo is an optional typeinfo.
MC_DLLEXPORT bool MCTypeInfoIsOptional(MCTypeInfoRef typeinfo);

// Returns true if the typeinfo is of record type.
MC_DLLEXPORT bool MCTypeInfoIsRecord(MCTypeInfoRef typeinfo);

// Returns true if the typeinfo is of handler type.
MC_DLLEXPORT bool MCTypeInfoIsHandler(MCTypeInfoRef typeinfo);

// Returns true if the typeinfo is of error type.
MC_DLLEXPORT bool MCTypeInfoIsError(MCTypeInfoRef typeinfo);

// Returns true if the typeinfo is of foreign type.
MC_DLLEXPORT bool MCTypeInfoIsForeign(MCTypeInfoRef typeinfo);
    
// Returns true if the typeinfo is of custom type.
MC_DLLEXPORT bool MCTypeInfoIsCustom(MCTypeInfoRef typeinfo);

// Returns the default value for the given type if it has one, otherwise nil.
MC_DLLEXPORT MCValueRef MCTypeInfoGetDefault(MCTypeInfoRef typeinfo);
    
// Typeinfo's form a chain with elements in the chain potentially providing critical
// information about the specified type. This structure describes the represented
// type, after a typeinfo chain has been suitably processed.
struct MCResolvedTypeInfo
{
    bool is_optional : 1;
    MCTypeInfoRef named_type;
    MCTypeInfoRef type;
};

// Resolves the given typeinfo to the base typeinfo (either a bound named typeinfo,
// or an anonymous non-meta typeinfo) if possible, and indicates if it is optional.
MC_DLLEXPORT bool MCTypeInfoResolve(MCTypeInfoRef typeinfo, MCResolvedTypeInfo& r_resolution);

// Returns true if the source typeinfo can be assigned to a slot with the target
// typeinfo. It is assumed that 'source' is a concrete typeinfo (one which has
// come from an actual value - in particular this means it will not be optional).
// It is further assumed that target is resolvable (i.e. if named, it has a binding).
//
// Conformance follows the following rules in order:
//   - if source is undefined (kMCNullTypeInfo), then target must be optional.
//   - if source is of foreign type, then target must either be the source's bridge
//     type, the source type, or one of the source's supertypes.
//   - if target is of foreign type, then source must be target's bridge type.
//   - if source is of record type, then target must be the same type or one of source's
//     supertypes.
//   - if source is builtin then target must be the same builtin type.
//
MC_DLLEXPORT bool MCTypeInfoConforms(MCTypeInfoRef source, MCTypeInfoRef target);

MC_DLLEXPORT bool MCResolvedTypeInfoConforms(const MCResolvedTypeInfo& source, const MCResolvedTypeInfo& target);

//////////

// Creates a typeinfo for one of the builtin typecodes.
MC_DLLEXPORT bool MCBuiltinTypeInfoCreate(MCValueTypeCode typecode, MCTypeInfoRef& r_target);

//////////

enum MCForeignPrimitiveType : uint8_t
{
    kMCForeignPrimitiveTypeVoid,
    kMCForeignPrimitiveTypeBool,
    kMCForeignPrimitiveTypeUInt8,
    kMCForeignPrimitiveTypeSInt8,
    kMCForeignPrimitiveTypeUInt16,
    kMCForeignPrimitiveTypeSInt16,
    kMCForeignPrimitiveTypeUInt32,
    kMCForeignPrimitiveTypeSInt32,
    kMCForeignPrimitiveTypeUInt64,
    kMCForeignPrimitiveTypeSInt64,
    kMCForeignPrimitiveTypeFloat32,
    kMCForeignPrimitiveTypeFloat64,
    kMCForeignPrimitiveTypePointer,
};

struct MCForeignTypeDescriptor
{
    size_t size;
    MCTypeInfoRef basetype;
    MCTypeInfoRef bridgetype;
    MCForeignPrimitiveType *layout;
    uindex_t layout_size;
    bool (*initialize)(void *contents);
    void (*finalize)(void *contents);
    bool (*defined)(void *contents);
    bool (*move)(const MCForeignTypeDescriptor* desc, void *source, void *target);
    bool (*copy)(const MCForeignTypeDescriptor* desc, void *source, void *target);
    bool (*equal)(const MCForeignTypeDescriptor* desc, void *left, void *right, bool& r_equal);
    bool (*hash)(const MCForeignTypeDescriptor* desc, void *contents, hash_t& r_hash);
    bool (*doimport)(const MCForeignTypeDescriptor* desc, void *contents, bool release, MCValueRef& r_value);
    bool (*doexport)(const MCForeignTypeDescriptor* desc, MCValueRef value, bool release, void *contents);
	bool (*describe)(const MCForeignTypeDescriptor* desc, void *contents, MCStringRef & r_desc);
    
    /* The promotedtype typeinfo is the type to which this type must be promoted
     * when passed through variadic parameters. The 'promote' method does the
     * promotion. */
    MCTypeInfoRef promotedtype;
    /* Promote the value in contents as necessary. The slot ptr must be big enough
     * to hold the promotedtype. */
    void (*promote)(void *contents);
};

MC_DLLEXPORT bool MCForeignTypeInfoCreate(const MCForeignTypeDescriptor *descriptor, MCTypeInfoRef& r_typeinfo);

MC_DLLEXPORT const MCForeignTypeDescriptor *MCForeignTypeInfoGetDescriptor(MCTypeInfoRef typeinfo);
MC_DLLEXPORT void *MCForeignTypeInfoGetLayoutType(MCTypeInfoRef typeinfo);

//////////

// Creates a type which is alias for another type.
MC_DLLEXPORT bool MCAliasTypeInfoCreate(MCNameRef name, MCTypeInfoRef target, MCTypeInfoRef& r_alias_typeinfo);

// Returnts the name of the alias.
MC_DLLEXPORT MCNameRef MCAliasTypeInfoGetName(MCTypeInfoRef typeinfo);

// Returns the target typeinfo.
MC_DLLEXPORT MCTypeInfoRef MCAliasTypeInfoGetTarget(MCTypeInfoRef typeinfo);

//////////

// Creates a type which refers to a named type. Named types are resolved by binding
// them to another type. It is an error to bind a bound named type, and an
// error to attempt to resolve an unbound named type.
MC_DLLEXPORT bool MCNamedTypeInfoCreate(MCNameRef name, MCTypeInfoRef& r_named_typeinfo);

// Fetch the name of the named typeinfo.
MC_DLLEXPORT MCNameRef MCNamedTypeInfoGetName(MCTypeInfoRef typeinfo);
    
// Returns true if the given named type is bound.
MC_DLLEXPORT bool MCNamedTypeInfoIsBound(MCTypeInfoRef typeinfo);

// Returns the bound typeinfo, or nil if the type is unbound.
MC_DLLEXPORT MCTypeInfoRef MCNamedTypeInfoGetBoundTypeInfo(MCTypeInfoRef typeinfo);

// Bind the given named type to the target type. The bound_type cannot be a named
// type.
MC_DLLEXPORT bool MCNamedTypeInfoBind(MCTypeInfoRef typeinfo, MCTypeInfoRef bound_type);

// Unbind the given named type.
MC_DLLEXPORT bool MCNamedTypeInfoUnbind(MCTypeInfoRef typeinfo);

// Resolve the given named type to its underlying type.
MC_DLLEXPORT bool MCNamedTypeInfoResolve(MCTypeInfoRef typeinfo, MCTypeInfoRef& r_bound_type);

//////////

// Creates an optional type. It is not allowed to create an optional type of an
// optional type. (Note optional named types are allowed, even if the named type
// is optional).
MC_DLLEXPORT bool MCOptionalTypeInfoCreate(MCTypeInfoRef base, MCTypeInfoRef& r_optional_typeinfo);

// Returns the base type of the given optional type.
MC_DLLEXPORT MCTypeInfoRef MCOptionalTypeInfoGetBaseTypeInfo(MCTypeInfoRef typeinfo);

//////////

// Create a typeinfo describing a custom typeinfo.
MC_DLLEXPORT bool MCCustomTypeInfoCreate(MCTypeInfoRef base, const MCValueCustomCallbacks *callbacks, MCTypeInfoRef& r_typeinfo);

MC_DLLEXPORT const MCValueCustomCallbacks *MCCustomTypeInfoGetCallbacks(MCTypeInfoRef typeinfo);

//////////

struct MCRecordTypeFieldInfo
{
	MCNameRef name;
	MCTypeInfoRef type;
};

// Create a description of a record with the given fields.
MC_DLLEXPORT bool MCRecordTypeInfoCreate(const MCRecordTypeFieldInfo *fields, index_t field_count, MCTypeInfoRef& r_typeinfo);

// Return the number of fields in the record.
MC_DLLEXPORT uindex_t MCRecordTypeInfoGetFieldCount(MCTypeInfoRef typeinfo);

// Return the name of the field at the given index.
MC_DLLEXPORT MCNameRef MCRecordTypeInfoGetFieldName(MCTypeInfoRef typeinfo, uindex_t index);

// Return the type of the field at the given index.
MC_DLLEXPORT MCTypeInfoRef MCRecordTypeInfoGetFieldType(MCTypeInfoRef typeinfo, uindex_t index);

//////////

// Handler types describe the signature of a function.

enum MCHandlerTypeFieldMode
{
	kMCHandlerTypeFieldModeIn,
	kMCHandlerTypeFieldModeOut,
	kMCHandlerTypeFieldModeInOut,
    kMCHandlerTypeFieldModeVariadic,
};

struct MCHandlerTypeFieldInfo
{
	MCTypeInfoRef type;
	MCHandlerTypeFieldMode mode;
};

// Create a description of a handler with the given signature.
// If field_count is negative, the fields array must be terminated by
// an MCHandlerTypeFieldInfo where name is null.
MC_DLLEXPORT bool MCHandlerTypeInfoCreate(const MCHandlerTypeFieldInfo *fields, index_t field_count, MCTypeInfoRef return_type, MCTypeInfoRef& r_typeinfo);

// Create a description of a foreign handler with the given signature.
// If field_count is negative, the fields array must be terminated by
// an MCHandlerTypeFieldInfo where name is null.
//
// Note: Foreign handlers and handlers are interchangeable for the most part. The
//   distinction is made so that the FFI knows when it needs to bridge from an
//   MCHandlerRef to a C function ptr.
MC_DLLEXPORT bool MCForeignHandlerTypeInfoCreate(const MCHandlerTypeFieldInfo *fields, index_t field_count, MCTypeInfoRef return_type, MCTypeInfoRef& r_typeinfo);

// Returns true if the handler is of foreign type.
MC_DLLEXPORT bool MCHandlerTypeInfoIsForeign(MCTypeInfoRef typeinfo);

// Returns true if the handler is variadic.
MC_DLLEXPORT bool MCHandlerTypeInfoIsVariadic(MCTypeInfoRef typeinfo);
    
// Get the return type of the handler. A return-type of kMCNullTypeInfo means no
// value is returned.
MC_DLLEXPORT MCTypeInfoRef MCHandlerTypeInfoGetReturnType(MCTypeInfoRef typeinfo);

// Get the number of parameters the handler takes. If the handler is variadic,
// this returns the number of fixed parameters.
MC_DLLEXPORT uindex_t MCHandlerTypeInfoGetParameterCount(MCTypeInfoRef typeinfo);

// Return the mode of the index'th parameter.
MC_DLLEXPORT MCHandlerTypeFieldMode MCHandlerTypeInfoGetParameterMode(MCTypeInfoRef typeinfo, uindex_t index);

// Return the type of the index'th parameter.
MC_DLLEXPORT MCTypeInfoRef MCHandlerTypeInfoGetParameterType(MCTypeInfoRef typeinfo, uindex_t index);
    
// Returns the 'native' layout ptr (an ffi_cif) for the handler type
MC_DLLEXPORT bool MCHandlerTypeInfoGetLayoutType(MCTypeInfoRef typeinfo, int abi, void*& r_cif);
    
//////////

MC_DLLEXPORT bool MCErrorTypeInfoCreate(MCNameRef domain, MCStringRef message, MCTypeInfoRef& r_typeinfo);

MC_DLLEXPORT MCNameRef MCErrorTypeInfoGetDomain(MCTypeInfoRef error);
MC_DLLEXPORT MCStringRef MCErrorTypeInfoGetMessage(MCTypeInfoRef error);

//////////

// Create a named typeinfo bound to an error typeinfo.
MC_DLLEXPORT bool MCNamedErrorTypeInfoCreate(MCNameRef p_name, MCNameRef p_domain, MCStringRef p_message, MCTypeInfoRef &r_typeinfo);

// Create a named typeinfo bound to a custom typeinfo.
MC_DLLEXPORT bool MCNamedCustomTypeInfoCreate(MCNameRef p_name, MCTypeInfoRef base, const MCValueCustomCallbacks *callbacks, MCTypeInfoRef& r_typeinfo);
	
// Create a named typeinfo bound to a foreign typeinfo.
MC_DLLEXPORT bool MCNamedForeignTypeInfoCreate(MCNameRef p_name, const MCForeignTypeDescriptor *p_descriptor, MCTypeInfoRef& r_typeinfo);

//////////
    
MC_DLLEXPORT bool MCJavaVMInitialize();
MC_DLLEXPORT void MCJavaVMFinalize();

MC_DLLEXPORT MCTypeInfoRef MCJavaGetObjectTypeInfo();
MC_DLLEXPORT bool MCJavaCreateJavaObjectTypeInfo();
    
MC_DLLEXPORT bool MCJavaObjectCreate(void *value, MCJavaObjectRef& r_obj);
MC_DLLEXPORT void *MCJavaObjectGetObject(const MCJavaObjectRef p_obj);

// Check that a Java param string is compatible with a handler signature typeinfo
MC_DLLEXPORT bool MCJavaCheckSignature(MCTypeInfoRef p_signature, MCStringRef p_params, MCStringRef p_return, int p_call_type);

// Call a Java method using the JNI
MC_DLLEXPORT bool MCJavaCallJNIMethod(MCNameRef p_class, void *p_method_id, int p_call_type, MCTypeInfoRef p_signature, void *r_return, void **p_args, uindex_t p_arg_count);

// Get a Java method pointer for a given method in a class
MC_DLLEXPORT void *MCJavaGetMethodId(MCNameRef p_class_name, MCStringRef p_method_name, MCStringRef p_arguments, MCStringRef p_return, int p_call_type);

// Get the name of a Java class from an instance of that class
MC_DLLEXPORT bool MCJavaGetJObjectClassName(MCJavaObjectRef p_object, MCStringRef &r_name);
// Convert a Java object wrapping a jstring to a String Ref
MC_DLLEXPORT bool MCJavaConvertJStringToStringRef(MCJavaObjectRef p_object, MCStringRef &r_string);
// Convert a String Ref to a Java object wrapping a jstring
MC_DLLEXPORT bool MCJavaConvertStringRefToJString(MCStringRef p_string, MCJavaObjectRef &r_object);
// Convert a Java object wrapping a jByteArray to a Data Ref
MC_DLLEXPORT bool MCJavaConvertJByteArrayToDataRef(MCJavaObjectRef p_object, MCDataRef &r_data);
// Convert a Data Ref to a Java object wrapping a jByteArray
MC_DLLEXPORT bool MCJavaConvertDataRefToJByteArray(MCDataRef p_data, MCJavaObjectRef &r_object);

////////////////////////////////////////////////////////////////////////////////
//
//  BOOLEAN DEFINITIONS
//

MC_DLLEXPORT extern MCNullRef kMCNull;

////////////////////////////////////////////////////////////////////////////////
//
//  BOOLEAN DEFINITIONS
//

MC_DLLEXPORT extern MCBooleanRef kMCFalse;
MC_DLLEXPORT extern MCBooleanRef kMCTrue;

MC_DLLEXPORT bool MCBooleanCreateWithBool(bool value, MCBooleanRef& r_boolean);
    
#if defined(__HAS_CORE_FOUNDATION__)
MC_DLLEXPORT bool MCBooleanCreateWithCFBooleanRef(CFBooleanRef p_cf_number, MCBooleanRef& r_number);
MC_DLLEXPORT bool MCBooleanConvertToCFBooleanRef(MCBooleanRef p_number, CFBooleanRef& r_cf_number);
#endif
    
////////////////////////////////////////////////////////////////////////////////
//
//  NUMBER DEFINITIONS
//

MC_DLLEXPORT bool MCNumberCreateWithInteger(integer_t value, MCNumberRef& r_number);
MC_DLLEXPORT bool MCNumberCreateWithUnsignedInteger(uinteger_t value, MCNumberRef& r_number);
MC_DLLEXPORT bool MCNumberCreateWithReal(real64_t value, MCNumberRef& r_number);

MC_DLLEXPORT compare_t MCNumberCompareTo(MCNumberRef self, MCNumberRef p_other_self);

MC_DLLEXPORT bool MCNumberIsInteger(MCNumberRef number);
MC_DLLEXPORT bool MCNumberIsReal(MCNumberRef number);

MC_DLLEXPORT integer_t MCNumberFetchAsInteger(MCNumberRef number);
MC_DLLEXPORT uinteger_t MCNumberFetchAsUnsignedInteger(MCNumberRef number);
MC_DLLEXPORT real64_t MCNumberFetchAsReal(MCNumberRef number);

MC_DLLEXPORT bool MCNumberStrictFetchAsIndex(MCNumberRef number,
                                             index_t& r_index);

MC_DLLEXPORT bool MCNumberParseOffsetPartial(MCStringRef p_string, uindex_t offset, uindex_t &r_chars_used, MCNumberRef &r_number);

MC_DLLEXPORT bool MCNumberParseOffset(MCStringRef p_string, uindex_t offset, uindex_t char_count, MCNumberRef &r_number);
MC_DLLEXPORT bool MCNumberParse(MCStringRef string, MCNumberRef& r_number);
MC_DLLEXPORT bool MCNumberParseInteger(MCStringRef string, MCNumberRef& r_number);
MC_DLLEXPORT bool MCNumberParseUnicodeChars(const unichar_t *chars, uindex_t char_count, MCNumberRef& r_number);
    
#if defined(__HAS_CORE_FOUNDATION__)
MC_DLLEXPORT bool MCNumberCreateWithCFNumberRef(CFNumberRef p_cf_number, MCNumberRef& r_number);
MC_DLLEXPORT bool MCNumberConvertToCFNumberRef(MCNumberRef p_number, CFNumberRef& r_cf_number);
#endif
    
MC_DLLEXPORT extern MCNumberRef kMCZero;
MC_DLLEXPORT extern MCNumberRef kMCOne;
MC_DLLEXPORT extern MCNumberRef kMCMinusOne;

#define DBL_INT_MAX     (1LL << DBL_MANT_DIG)   /* the maximum integer faithfully representable by a double */
#define DBL_INT_MIN     (-DBL_INT_MAX)          /* the minimum integer faithfully representable by a double */

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
	// The UTF-8 string encoding.  In LiveCode, this permits overlong
	// sequences when decoding, but does not generate them when
	// encoding.
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
    // Compare strings without normalization but with case folding
    kMCStringOptionCompareFolded = 2,
	// Compare the strings codepoint for codepoint after normalization and
	// folding.
	kMCStringOptionCompareCaseless = 3,
    
    // If this bit is set it means the strings are normalized.
    kMCStringOptionNormalizeBit = 1 << 0,
    // If this bit is set it means the strings are folded.
    kMCStringOptionFoldBit = 1 << 1,
};

/////////

// The empty string.
MC_DLLEXPORT extern MCStringRef kMCEmptyString;

// The default string for the 'true' boolean value.
MC_DLLEXPORT extern MCStringRef kMCTrueString;

// The default string for the 'false' boolean value.
MC_DLLEXPORT extern MCStringRef kMCFalseString;

// The default string for the 'mixed' value of chunk properties.
MC_DLLEXPORT extern MCStringRef kMCMixedString;

// The default string for ','.
MC_DLLEXPORT extern MCStringRef kMCCommaString;

// The default string for '\n'.
MC_DLLEXPORT extern MCStringRef kMCLineEndString;

// The default string for '\t'.
MC_DLLEXPORT extern MCStringRef kMCTabString;

// The default string for '\0'.
MC_DLLEXPORT extern MCStringRef kMCNulString;

/////////

// Creates an MCStringRef wrapping the given constant c-string. Note that
// the c-string must be a C static string.
MC_DLLEXPORT MCStringRef MCSTR(const char *string);

MC_DLLEXPORT const char *MCStringGetCString(MCStringRef p_string);
MC_DLLEXPORT bool MCStringIsEqualToCString(MCStringRef string, const char *cstring, MCStringOptions options);
MC_DLLEXPORT bool MCStringSubstringIsEqualToCString(MCStringRef string, MCRange p_range, const char *cstring, MCStringOptions options);
    
// Create an immutable string from the given bytes, interpreting them using
// the specified encoding.
MC_DLLEXPORT bool MCStringCreateWithBytes(const byte_t *bytes, uindex_t byte_count, MCStringEncoding encoding, bool is_external_rep, MCStringRef& r_string);
MC_DLLEXPORT bool MCStringCreateWithBytesAndRelease(byte_t *bytes, uindex_t byte_count, MCStringEncoding encoding, bool is_external_rep, MCStringRef& r_string);

// Create an immutable string from the given unicode char sequence.
MC_DLLEXPORT bool MCStringCreateWithChars(const unichar_t *chars, uindex_t char_count, MCStringRef& r_string);
MC_DLLEXPORT bool MCStringCreateWithCharsAndRelease(unichar_t *chars, uindex_t char_count, MCStringRef& r_string);

// Create an immutable string from the given unicode char sequence and force it
// to be unicode.
MC_DLLEXPORT bool MCStringCreateUnicodeWithChars(const unichar_t *chars, uindex_t char_count, MCStringRef& r_string);

// Create an immutable string from the given NUL terminated unicode char sequence.
MC_DLLEXPORT bool MCStringCreateWithWString(const unichar_t *wstring, MCStringRef& r_string);
MC_DLLEXPORT bool MCStringCreateWithWStringAndRelease(unichar_t *wstring, MCStringRef& r_string);

// Create an immutable string from the given native char sequence.
MC_DLLEXPORT bool MCStringCreateWithNativeChars(const char_t *chars, uindex_t char_count, MCStringRef& r_string);
MC_DLLEXPORT bool MCStringCreateWithNativeCharsAndRelease(char_t *chars, uindex_t char_count, MCStringRef& r_string);
MC_DLLEXPORT bool MCStringCreateWithNativeCharBufferAndRelease(char_t* buffer, uindex_t char_count, uindex_t buffer_length, MCStringRef& r_string);

// Create an immutable string from the given (native) c-string.
MC_DLLEXPORT bool MCStringCreateWithCString(const char *cstring, MCStringRef& r_string);
MC_DLLEXPORT bool MCStringCreateWithCStringAndRelease(char *cstring /*delete[]*/, MCStringRef& r_string);

#ifdef __HAS_CORE_FOUNDATION__
// Create a string from a CoreFoundation string object.
MC_DLLEXPORT bool MCStringCreateWithCFStringRef(CFStringRef cf_string, MCStringRef& r_string);
#endif

// Create a string from a Pascal-style (counted) string. This always uses the MacRoman encoding.
MC_DLLEXPORT bool MCStringCreateWithPascalString(const unsigned char* pascal_string, MCStringRef& r_string);
    
#if !defined(__WINDOWS__)
// Create a string from a C string in the system encoding
MC_DLLEXPORT bool MCStringCreateWithSysString(const char *sys_string, MCStringRef &r_string);
#endif

#if defined(__WINDOWS__)
// Create a string from a Windows BSTR
MC_DLLEXPORT bool MCStringCreateWithBSTR(const BSTR p_bstr, MCStringRef& r_string);
#endif

// Creates a string from existing strings. The first variant exists to provide
// an optimised implementation in the (very common) case of only two strings.
MC_DLLEXPORT bool MCStringCreateWithStrings(MCStringRef& r_string, MCStringRef p_one, MCStringRef p_two);
//MC_DLLEXPORT bool MCStringCreateWithStrings(MCStringRef& r_string, MCStringRef p_one, MCStringRef p_two, MCStringRef p_three, ...);

// Creates a string from existing strings, using the given in-fix character
MC_DLLEXPORT bool MCStringCreateWithStringsAndSeparator(MCStringRef& r_string, unichar_t t_separator, MCStringRef p_one, MCStringRef p_two);
//MC_DLLEXPORT bool MCStringCreateWithStringsAndSeparator(MCStringRef& r_string, unichar_t t_separator, MCStringRef p_one, MCStringRef p_two, MCStringRef p_three, ...);

// Create a mutable string with the given initial capacity. Note that the
// initial capacity is only treated as a hint, the string will extend itself
// as necessary.
MC_DLLEXPORT bool MCStringCreateMutable(uindex_t initial_capacity, MCStringRef& r_string);

/////////

// Encode the given string with the specified encoding. Characters which cannot
// be represented in the target encoding are replaced by '?'.
MC_DLLEXPORT bool MCStringEncode(MCStringRef string, MCStringEncoding encoding, bool is_external_rep, MCDataRef& r_data);
MC_DLLEXPORT bool MCStringEncodeAndRelease(MCStringRef string, MCStringEncoding encoding, bool is_external_rep, MCDataRef& r_data);

// Decode the given data, intepreting in the given encoding.
MC_DLLEXPORT bool MCStringDecode(MCDataRef data, MCStringEncoding encoding, bool is_external_rep, MCStringRef& r_string);
MC_DLLEXPORT bool MCStringDecodeAndRelease(MCDataRef data, MCStringEncoding encoding, bool is_external_rep, MCStringRef& r_string);

// SN-2015-07-27: [[ Bug 15379 ]] We can need to build a string from data,
//  without any ASCII value conversion.
bool MCStringCreateUnicodeStringFromData(MCDataRef p_data, bool p_is_external_rep, MCStringRef& r_string);

/////////

// Create an immutable string, built using the given format specification and
// argument list.
MC_DLLEXPORT bool MCStringFormat(MCStringRef& r_string, const char *format, ...);
MC_DLLEXPORT bool MCStringFormatV(MCStringRef& r_string, const char *format, va_list args);

/////////

// Copy the given string as immutable.
MC_DLLEXPORT bool MCStringCopy(MCStringRef string, MCStringRef& r_new_string);

// Copy the given string as immutable, releasing the original.
MC_DLLEXPORT bool MCStringCopyAndRelease(MCStringRef string, MCStringRef& r_new_string);

// Copy the given string as mutable.
MC_DLLEXPORT bool MCStringMutableCopy(MCStringRef string, MCStringRef& r_new_string);

// Copy the given string as mutable, releasing the original.
MC_DLLEXPORT bool MCStringMutableCopyAndRelease(MCStringRef string, MCStringRef& r_new_string);

/////////

// Copy a substring of the given string as immutable.
MC_DLLEXPORT bool MCStringCopySubstring(MCStringRef string, MCRange range, MCStringRef& r_substring);

// Copy a substring of the given string as immutable, releasing the original.
MC_DLLEXPORT bool MCStringCopySubstringAndRelease(MCStringRef string, MCRange range, MCStringRef& r_substring);

// Copy a substring of the given string as mutable.
MC_DLLEXPORT bool MCStringMutableCopySubstring(MCStringRef string, MCRange range, MCStringRef& r_substring);

// Copy a substring of the given string as mutable, releasing the original.
MC_DLLEXPORT bool MCStringMutableCopySubstringAndRelease(MCStringRef string, MCRange range, MCStringRef& r_substring);

/////////

// Copy a string, reversing its contents
MC_DLLEXPORT bool MCStringCopyReversed(MCStringRef string, MCStringRef& r_reversed);

/////////

// Returns true if the string is mutable
MC_DLLEXPORT bool MCStringIsMutable(const MCStringRef string);

// Returns true if the string is the empty string.
MC_DLLEXPORT bool MCStringIsEmpty(MCStringRef string);
    
// Returns true if the string is a (strict) integer string <-?(0|[1-9][0-9]*)>*/
MC_DLLEXPORT bool MCStringIsInteger(MCStringRef string);

// Returns true if the the string only requires native characters to represent.
MC_DLLEXPORT bool MCStringCanBeNative(MCStringRef string);

// Returns true if under the given comparison conditions, string cannot be represented natively.
MC_DLLEXPORT bool MCStringCantBeEqualToNative(MCStringRef string, MCStringOptions p_options);


// Returns true if the string is stored as native chars.
MC_DLLEXPORT bool MCStringIsNative(MCStringRef string);

// Returns true if the string only requires BMP characters to represent.
MC_DLLEXPORT bool MCStringIsBasic(MCStringRef string);

// Returns true if the string's grapheme -> codeunit mapping is one-to-one.
// Note that trivial entails basic, as any SMP codepoints are multi-codeunit.
MC_DLLEXPORT bool MCStringIsTrivial(MCStringRef string);

/////////

// Returns the number of chars that make up the string. Note that a char is
// considered to be UTF-16 code unit. This is a strict upper bound for the
// length of the native char or unicode char sequence backing the string as
// natively encoded strings are at most the same length as their unicode encoded
// counterparts (the subtleties being surrogate pairs map to '?', and things
// like e,acute map to 'e-acute').
MC_DLLEXPORT uindex_t MCStringGetLength(const MCStringRef string);

// Return a pointer to the char backing-store if possible. Note that if this
// method returns nil, then GetChars() must be used to fetch the contents as
// unicode codeunits.
MC_DLLEXPORT const unichar_t *MCStringGetCharPtr(MCStringRef string);

// Return a pointer to the native char backing-store if possible. Note that if
// the method returns nil, then GetNativeChars() must be used to fetch the contents
// in native encoding.
MC_DLLEXPORT const char_t *MCStringGetNativeCharPtr(MCStringRef string);
// The native length may be different from the string char count.
MC_DLLEXPORT const char_t *MCStringGetNativeCharPtrAndLength(MCStringRef self, uindex_t& r_native_length);

// Returns the Unicode codepoint at the given index
MC_DLLEXPORT codepoint_t MCStringGetCodepointAtIndex(MCStringRef string, uindex_t index);

// Returns the char at the given index.
MC_DLLEXPORT unichar_t MCStringGetCharAtIndex(MCStringRef string, uindex_t index);

// Returns the native char at the given index.
MC_DLLEXPORT char_t MCStringGetNativeCharAtIndex(MCStringRef string, uindex_t index);

// Returns the Unicode codepoint (not UTF-16 char) at the given codepoint index
MC_DLLEXPORT codepoint_t MCStringGetCodepointAtIndex(MCStringRef string, uindex_t index);

// Returns the sequence of chars making up the given range in 'chars' and returns
// the number of chars generated. If 'chars' is nil, just the number of chars that
// would be generated is returned.
MC_DLLEXPORT uindex_t MCStringGetChars(MCStringRef string, MCRange range, unichar_t *chars);

// Returns the sequence of native chars making up the given range in 'chars' and
// returns the number of chars generated. If 'chars' is nil, just the number of chars
// that would be generated is returned. Any unmappable chars get generated as '?'.
MC_DLLEXPORT uindex_t MCStringGetNativeChars(MCStringRef string, MCRange range, char_t *chars);

// Nativize self
MC_DLLEXPORT void MCStringNativize(MCStringRef string);

// Create a native copy of p_string
MC_DLLEXPORT bool MCStringNativeCopy(MCStringRef p_string, MCStringRef& r_copy);

// Create a Unicode copy of p_string
MC_DLLEXPORT bool MCStringUnicodeCopy(MCStringRef p_string, MCStringRef& r_copy);

// Maps from a codepoint (character) range to a code unit (StringRef) range
MC_DLLEXPORT bool MCStringMapCodepointIndices(MCStringRef, MCRange p_codepoint_range, MCRange& r_string_range);

// Maps from a code unit (StringRef) range to a codepoint (character) range
MC_DLLEXPORT bool MCStringUnmapCodepointIndices(MCStringRef, MCRange p_string_range, MCRange &r_codepoint_range);

// Maps from a grapheme (visual character) range to a code unit (StringRef) range
MC_DLLEXPORT bool MCStringMapGraphemeIndices(MCStringRef, MCRange p_grapheme_range, MCRange& r_string_range);

// Maps from a code unit (StringRef) range to a grapheme (visual character) range
MC_DLLEXPORT bool MCStringUnmapGraphemeIndices(MCStringRef, MCRange p_string_range, MCRange& r_grapheme_range);

// Maps from a word range to a codeunit (StringRef) range
MC_DLLEXPORT bool MCStringMapTrueWordIndices(MCStringRef, MCLocaleRef, MCRange p_word_range, MCRange& r_string_range);

// Maps from a codeunit (StringRef) range to a word range
MC_DLLEXPORT bool MCStringUnmapTrueWordIndices(MCStringRef, MCLocaleRef, MCRange p_string_range, MCRange &r_word_range);

// Maps from a sentence range to a codeunit (StringRef) range
MC_DLLEXPORT bool MCStringMapSentenceIndices(MCStringRef, MCLocaleRef, MCRange p_sentence_range, MCRange& r_string_range);

// Maps from a codeunit (StringRef) range to a sentence range
MC_DLLEXPORT bool MCStringUnmapSentenceIndices(MCStringRef, MCLocaleRef, MCRange p_string_range, MCRange &r_sentence_range);

// Maps from a paragraph range to a codeunit (StringRef) range
MC_DLLEXPORT bool MCStringMapParagraphIndices(MCStringRef, MCLocaleRef, MCRange p_paragraph_range, MCRange& r_string_range);

// Maps from a codeunit (StringRef) range to a word range
MC_DLLEXPORT bool MCStringUnmapParagraphIndices(MCStringRef, MCLocaleRef, MCRange p_string_range, MCRange &r_paragraph_range);

// Returns true if the codepoint is alphabetic or numeric.
MC_DLLEXPORT bool MCStringCodepointIsWordPart(codepoint_t p_codepoint);

// Returns the index of the beginning of the next grapheme after p_from.
uindex_t MCStringGraphemeBreakIteratorAdvance(MCStringRef self, uindex_t p_from);
// Returns the index of the beginning of the previous grapheme before p_from.
uindex_t MCStringGraphemeBreakIteratorRetreat(MCStringRef self, uindex_t p_from);
// Flexible grapheme/codepoint/codeunit mapping used for "char" chunk expressions
enum MCCharChunkType
{
    kMCCharChunkTypeCodeunit,   // UTF-16 code units
    kMCCharChunkTypeCodepoint,  // Unicode codepoint values
    kMCCharChunkTypeGrapheme,   // Graphical character boundaries
};

const MCCharChunkType kMCDefaultCharChunkType = kMCCharChunkTypeGrapheme;

MC_DLLEXPORT bool MCStringMapIndices(MCStringRef, MCCharChunkType, MCRange p_char_range, MCRange &r_codeunit_range);
MC_DLLEXPORT bool MCStringUnmapIndices(MCStringRef, MCCharChunkType, MCRange p_codeunit_range, MCRange &r_char_range);

/////////

// Converts the contents of the string to bytes using the given encoding. The caller
// takes ownership of the byte array. Note that the returned array is NUL terminated,
// but this is not reflected in the byte count.
MC_DLLEXPORT bool MCStringConvertToBytes(MCStringRef string, MCStringEncoding encoding, bool is_external_rep, byte_t*& r_bytes, uindex_t& r_byte_count);

// [[ Bug 12204 ]] textEncode ASCII support is actually native
// Converts the contents of the string to ASCII characters - excluding the characters from the extended range
MC_DLLEXPORT bool MCStringConvertToAscii(MCStringRef self, char_t *&r_chars, uindex_t& r_char_count);

// Converts the contents of the string to unicode. The caller takes ownership of the
// char array. Note that the returned array is NUL terminated, but this is not
// reflected in the char count.
MC_DLLEXPORT bool MCStringConvertToUnicode(MCStringRef string, unichar_t*& r_chars, uindex_t& r_char_count);

// Converts the contents of the string to native - using '?' as the unmappable char.
// The caller takes ownership of the char array. Note that the returned array is NUL
// terminated, but this is not reflected in the char count.
MC_DLLEXPORT bool MCStringConvertToNative(MCStringRef string, char_t*& r_chars, uindex_t& r_char_count);

// Normalizes and converts to native
MC_DLLEXPORT bool MCStringNormalizeAndConvertToNative(MCStringRef string, char_t*& r_chars, uindex_t& r_char_count);

// Converts the contents of the string to UTF-8. The caller takes ownership of the
// char array. Note that the returned array is NUL terminated but this is not
// reflected in the char count.
MC_DLLEXPORT bool MCStringConvertToUTF8(MCStringRef string, char*& r_chars, uindex_t& r_char_count);

// Converts the contents of the string to UTF-32.
MC_DLLEXPORT bool MCStringConvertToUTF32(MCStringRef self, uint32_t *&r_codepoints, uinteger_t &r_char_count);

// Normalizes and converts to c-string
MC_DLLEXPORT bool MCStringNormalizeAndConvertToCString(MCStringRef string, char*& r_cstring);

// Converts the content to char_t*
MC_DLLEXPORT bool MCStringConvertToCString(MCStringRef string, char*& r_cstring);

// Converts the content to wchar_t*
MC_DLLEXPORT bool MCStringConvertToWString(MCStringRef string, unichar_t*& r_wstring);

// Converts the content to unicode_t*
MC_DLLEXPORT bool MCStringConvertToUTF8String(MCStringRef string, char*& r_utf8string);

#ifdef __HAS_CORE_FOUNDATION__
// Converts the content to CFStringRef
MC_DLLEXPORT bool MCStringConvertToCFStringRef(MCStringRef string, CFStringRef& r_cfstring);
#endif

#ifdef __WINDOWS__
MC_DLLEXPORT bool MCStringConvertToBSTR(MCStringRef string, BSTR& r_bstr);
#endif

// Converts the given string ref to a string in the system encoding.
// Note that the 'bytes' ptr is typed as 'char', however it should be considered
// as an opaque sequence of bytes with an 'unknowable' encoding.
// Note that the output string is allocated with an implicit NUL byte, but this
// is not included in the byte_count.
MC_DLLEXPORT bool MCStringConvertToSysString(MCStringRef string, char*& r_bytes, size_t& r_byte_count);

/////////

// Returns the hash of the given string, processing as according to options.
MC_DLLEXPORT hash_t MCStringHash(MCStringRef string, MCStringOptions options);

// Returns true if the two strings are equal, processing as appropriate according
// to options.
MC_DLLEXPORT bool MCStringIsEqualTo(MCStringRef string, MCStringRef other, MCStringOptions options);
MC_DLLEXPORT bool MCStringIsEqualToNativeChars(MCStringRef string, const char_t *chars, uindex_t char_count, MCStringOptions options);
MC_DLLEXPORT bool MCStringSubstringIsEqualToNativeChars(MCStringRef self, MCRange p_range, const char_t *p_chars, uindex_t p_char_count, MCStringOptions p_options);

// Returns true if the substring is equal to the other, according to options
MC_DLLEXPORT bool MCStringSubstringIsEqualTo(MCStringRef string, MCRange range, MCStringRef p_other, MCStringOptions p_options);
MC_DLLEXPORT bool MCStringSubstringIsEqualToSubstring(MCStringRef string, MCRange range, MCStringRef p_other, MCRange other_range, MCStringOptions p_options);

// Returns -1, 0, or 1, depending on whether left < 0, left == right or left > 0,
// processing as appropriate according to options. The ordering used is codepoint-
// wise lexicographic.
MC_DLLEXPORT compare_t MCStringCompareTo(MCStringRef string, MCStringRef other, MCStringOptions options);

// Returns true if the string begins with the prefix string, processing as
// appropriate according to options.
// If 'r_string_match_length' is used, then it will contain the length of the
// match in 'string'. This might not be the same as the length of 'prefix' due
// to case folding and normalization concerns in unicode strings.
MC_DLLEXPORT bool MCStringBeginsWith(MCStringRef string, MCStringRef prefix, MCStringOptions options, uindex_t *r_string_match_length = nil);
MC_DLLEXPORT bool MCStringSharedPrefix(MCStringRef self, MCRange p_range, MCStringRef p_prefix, MCStringOptions p_options, uindex_t& r_string_match_length);
MC_DLLEXPORT bool MCStringBeginsWithCString(MCStringRef string, const char_t *prefix_cstring, MCStringOptions options);

// Returns true if the string ends with the suffix string, processing as
// appropriate according to options.
// If 'r_string_match_length' is used, then it will contain the length of the
// match in 'string'. This might not be the same as the length of 'suffix' due
// to case folding and normalization concerns in unicode strings.
MC_DLLEXPORT bool MCStringEndsWith(MCStringRef string, MCStringRef suffix, MCStringOptions options, uindex_t *r_string_match_length = nil);
MC_DLLEXPORT bool MCStringSharedSuffix(MCStringRef self, MCRange p_range, MCStringRef p_suffix, MCStringOptions p_options, uindex_t& r_string_match_length);
MC_DLLEXPORT bool MCStringEndsWithCString(MCStringRef string, const char_t *suffix_cstring, MCStringOptions options);

// Returns true if the string contains the given needle string, processing as
// appropriate according to options.
MC_DLLEXPORT bool MCStringContains(MCStringRef string, MCStringRef needle, MCStringOptions options);

// Returns true if the substring contains the given needle string, processing as
// appropriate according to options.
MC_DLLEXPORT bool MCStringSubstringContains(MCStringRef string, MCRange range, MCStringRef needle, MCStringOptions options);

//////////

// Find the first offset of needle in string, on or after index 'after',
// processing as appropriate according to options.
MC_DLLEXPORT bool MCStringFirstIndexOf(MCStringRef string, MCStringRef needle, uindex_t after, MCStringOptions options, uindex_t& r_offset);
MC_DLLEXPORT bool MCStringFirstIndexOfStringInRange(MCStringRef string, MCStringRef p_needle, MCRange p_range, MCStringOptions p_options, uindex_t& r_offset);

// Find the first offset of needle in string - where needle is a Unicode character
// (note it is a codepoint, not unichar - i.e. a 20-bit value).
MC_DLLEXPORT bool MCStringFirstIndexOfChar(MCStringRef string, codepoint_t needle, uindex_t after, MCStringOptions options, uindex_t& r_offset);
// Find the first offset of needle in given range of string - where needle is a Unicode character
// (note it is a codepoint, not unichar - i.e. a 20-bit value).
MC_DLLEXPORT bool MCStringFirstIndexOfCharInRange(MCStringRef self, codepoint_t p_needle, MCRange p_range, MCStringOptions p_options, uindex_t& r_offset);

// Find the last offset of needle in string, on or before index 'before',
// processing as appropriate according to options.
MC_DLLEXPORT bool MCStringLastIndexOf(MCStringRef string, MCStringRef needle, uindex_t before, MCStringOptions options, uindex_t& r_offset);
MC_DLLEXPORT bool MCStringLastIndexOfStringInRange(MCStringRef string, MCStringRef p_needle, MCRange p_range, MCStringOptions p_options, uindex_t& r_offset);

// Find the last offset of needle in string - where needle is a Unicode character
// (note it is a codepoint, not unichar - i.e. a 20-bit value).
MC_DLLEXPORT bool MCStringLastIndexOfChar(MCStringRef string, codepoint_t needle, uindex_t before, MCStringOptions options, uindex_t& r_offset);

// Search 'range' of 'string' for 'needle' processing as appropriate to optiosn
// and returning any located string in 'result'. If the result is false, no range is
// returned.
MC_DLLEXPORT bool MCStringFind(MCStringRef string, MCRange range, MCStringRef needle, MCStringOptions options, MCRange* r_result);

// Search 'range' of 'string' for 'needle' processing as appropriate to options
// and returning the number of occurrences found.
MC_DLLEXPORT uindex_t MCStringCount(MCStringRef string, MCRange range, MCStringRef needle, MCStringOptions options);
MC_DLLEXPORT uindex_t MCStringCountChar(MCStringRef string, MCRange range, codepoint_t needle, MCStringOptions options);
    
//////////

// Find the first index of separator in string processing as according to
// options, then split the string into head and tail - the strings either side
// of the needle.
MC_DLLEXPORT bool MCStringDivide(MCStringRef string, MCStringRef separator, MCStringOptions options, MCStringRef& r_head, MCStringRef& r_tail);
MC_DLLEXPORT bool MCStringDivideAtChar(MCStringRef string, codepoint_t separator, MCStringOptions options, MCStringRef& r_head, MCStringRef& r_tail);
MC_DLLEXPORT bool MCStringDivideAtIndex(MCStringRef self, uindex_t p_offset, MCStringRef& r_head, MCStringRef& r_tail);

//////////

// Break the string into ranges inbetween the given delimiter char. A trailing
// empty range is ignored. The caller is responsible for deleting the returned
// array.
MC_DLLEXPORT bool MCStringBreakIntoChunks(MCStringRef string, codepoint_t separator, MCStringOptions options, MCRange*& r_ranges, uindex_t& r_range_count);

//////////
    
// Search 'range' of 'string' for 'needle' processing as appropriate to options
// and taking into account 'delimiter' and 'skip'.
// The function searches for 'needle' after 'skip' occurrences of 'delimiter'.
// The total number of delimiters encountered before 'needle' is found is
// returned in 'r_index'.
// If 'r_found' is not nil, it will return the range of the needle string in
// 'string'.
// If 'r_before' is not nil, it will return the range of the last delimiter before
// the found 'needle' string.
// If 'r_after' is not nil, it will return the range of the first delimiter after
// the found 'needle' string.
// If 'needle' is not found in the given range (after skipping) then false is
// returned.
// Additionally, if 'needle' is the empty string, then
// Note: The search done for 'needle' is as a string, in particular 'needle'
//   can contain 'delimiter'.
// Note: If needle is the empty string then false will be returned.
MC_DLLEXPORT bool MCStringDelimitedOffset(MCStringRef string, MCRange range, MCStringRef needle, MCStringRef delimiter, uindex_t skip, MCStringOptions options, uindex_t& r_index, MCRange *r_found, MCRange *r_before, MCRange *r_after);

MC_DLLEXPORT bool MCStringForwardDelimitedRegion(MCStringRef string,
                                                 MCRange range,
                                                 MCStringRef delimiter,
                                                 MCRange region,
                                                 MCStringOptions options,
                                                 MCRange& r_range);

//////////
    
// Transform the string to its folded form as specified by 'options'. The folded
// form of a string is that which is used to perform comparisons.
//
// Note that 'string' must be mutable, it is a fatal runtime error if it is not.
MC_DLLEXPORT bool MCStringFold(MCStringRef string, MCStringOptions options);

// Lowercase the string.
//
// Note that 'string' must be mutable, it is a fatal runtime error if it is not.
MC_DLLEXPORT bool MCStringLowercase(MCStringRef string, MCLocaleRef p_in_locale);

// Uppercase the string.
//
// Note that 'string' must be mutable, it is a fatal runtime error if it is not.
MC_DLLEXPORT bool MCStringUppercase(MCStringRef string, MCLocaleRef p_in_locale);

/////////

// Append suffix to string.
//
// Note that 'string' must be mutable, it is a fatal runtime error if it is not.
MC_DLLEXPORT bool MCStringAppend(MCStringRef string, MCStringRef suffix);
MC_DLLEXPORT bool MCStringAppendSubstring(MCStringRef string, MCStringRef suffix, MCRange range);
MC_DLLEXPORT bool MCStringAppendChars(MCStringRef string, const unichar_t *chars, uindex_t count);
MC_DLLEXPORT bool MCStringAppendNativeChars(MCStringRef string, const char_t *chars, uindex_t count);
MC_DLLEXPORT bool MCStringAppendChar(MCStringRef string, unichar_t p_char);
MC_DLLEXPORT bool MCStringAppendNativeChar(MCStringRef string, char_t p_char);
MC_DLLEXPORT bool MCStringAppendCodepoint(MCStringRef string, codepoint_t p_codepoint);

// Prepend prefix to string.
//
// Note that 'string' must be mutable, it is a fatal runtime error if it is not.
MC_DLLEXPORT bool MCStringPrepend(MCStringRef string, MCStringRef prefix);
MC_DLLEXPORT bool MCStringPrependSubstring(MCStringRef string, MCStringRef suffix, MCRange range);
MC_DLLEXPORT bool MCStringPrependChars(MCStringRef string, const unichar_t *chars, uindex_t count);
MC_DLLEXPORT bool MCStringPrependNativeChars(MCStringRef string, const char_t *chars, uindex_t count);
MC_DLLEXPORT bool MCStringPrependChar(MCStringRef string, unichar_t p_char);
MC_DLLEXPORT bool MCStringPrependNativeChar(MCStringRef string, char_t p_char);
MC_DLLEXPORT bool MCStringPrependCodepoint(MCStringRef string, codepoint_t p_codepoint);

// Insert new_string into string at offset 'at'.
//
// Note that 'string' must be mutable, it is a fatal runtime error if it is not.
MC_DLLEXPORT bool MCStringInsert(MCStringRef string, uindex_t at, MCStringRef new_string);
MC_DLLEXPORT bool MCStringInsertSubstring(MCStringRef string, uindex_t at, MCStringRef new_string, MCRange range);
MC_DLLEXPORT bool MCStringInsertChars(MCStringRef string, uindex_t at, const unichar_t *chars, uindex_t count);
MC_DLLEXPORT bool MCStringInsertNativeChars(MCStringRef string, uindex_t at, const char_t *chars, uindex_t count);
MC_DLLEXPORT bool MCStringInsertChar(MCStringRef string, uindex_t at, unichar_t p_char);
MC_DLLEXPORT bool MCStringInsertNativeChar(MCStringRef string, uindex_t at, char_t p_char);
MC_DLLEXPORT bool MCStringInsertCodepoint (MCStringRef string, uindex_t p_at, codepoint_t p_codepoint);

// Remove 'range' characters from 'string'.
//
// Note that 'string' must be mutable, it is a fatal runtime error if it is not.
MC_DLLEXPORT bool MCStringRemove(MCStringRef string, MCRange range);

// Retain only 'range' characters from 'string'.
//
// Note that 'string' must be mutable, it is a fatal runtime error if it is not.
MC_DLLEXPORT bool MCStringSubstring(MCStringRef string, MCRange range);

// Replace 'range' characters in 'string' with 'replacement'.
//
// Note that 'string' must be mutable, it is a fatal runtime error if it is not.
MC_DLLEXPORT bool MCStringReplace(MCStringRef string, MCRange range, MCStringRef replacement);

// Pad the end of the string with count copies of value. If value is nil then count
// uninitialized bytes will be inserted after at.
//
// Note that 'string' must be mutable.
MC_DLLEXPORT bool MCStringPad(MCStringRef string, uindex_t at, uindex_t count, MCStringRef value);

// Resolves the directionality of the string and returns true if it is left to right.
// (Uses MCBidiFirstStrongIsolate to determine directionality).
MC_DLLEXPORT bool MCStringResolvesLeftToRight(MCStringRef p_string);

// Find and replace all instances of pattern in target with replacement.
//
// Note that 'string' must be mutable.
MC_DLLEXPORT bool MCStringFindAndReplace(MCStringRef string, MCStringRef pattern, MCStringRef replacement, MCStringOptions options);
MC_DLLEXPORT bool MCStringFindAndReplaceChar(MCStringRef string, codepoint_t pattern, codepoint_t replacement, MCStringOptions options);

MC_DLLEXPORT bool MCStringWildcardMatch(MCStringRef source, MCRange source_range, MCStringRef pattern, MCStringOptions p_options);

/////////

// Append a formatted string to another string.
//
// Note that 'string' must be mutable, it is a fatal runtime error if it is not.
MC_DLLEXPORT bool MCStringAppendFormat(MCStringRef string, const char *format, ...);
MC_DLLEXPORT bool MCStringAppendFormatV(MCStringRef string, const char *format, va_list args);

//////////

MC_DLLEXPORT bool MCStringSplit(MCStringRef string, MCStringRef element_del, MCStringRef key_del, MCStringOptions options, MCArrayRef& r_array);
MC_DLLEXPORT bool MCStringSplitColumn(MCStringRef string, MCStringRef col_del, MCStringRef row_del, MCStringOptions options, MCArrayRef& r_array);

//////////

// Proper list versions of string splitting
MC_DLLEXPORT bool MCStringSplitByDelimiter(MCStringRef self, MCStringRef p_elem_del, MCStringOptions p_options, MCProperListRef& r_list);

//////////

// Converts two surrogate pair code units into a codepoint
MC_DLLEXPORT codepoint_t MCStringSurrogatesToCodepoint(unichar_t p_lead, unichar_t p_trail);

// Converts a codepoint to UTF-16 code units and returns the number of units
MC_DLLEXPORT unsigned int MCStringCodepointToSurrogates(codepoint_t, unichar_t (&r_units)[2]);

// Returns true if the code unit at the given index and the next code unit form
// a valid surrogate pair. Lone lead or trail code units are not valid pairs.
MC_DLLEXPORT bool MCStringIsValidSurrogatePair(MCStringRef, uindex_t);

// Returns true if the codeunit at the given index in the string is the
// beginning of a new grapheme cluster.
MC_DLLEXPORT bool MCStringIsGraphemeClusterBoundary(MCStringRef, uindex_t);
    
//////////

// Normalises a string into the requested form
MC_DLLEXPORT bool MCStringNormalizedCopyNFC(MCStringRef, MCStringRef&);
MC_DLLEXPORT bool MCStringNormalizedCopyNFD(MCStringRef, MCStringRef&);
MC_DLLEXPORT bool MCStringNormalizedCopyNFKC(MCStringRef, MCStringRef&);
MC_DLLEXPORT bool MCStringNormalizedCopyNFKD(MCStringRef, MCStringRef&);

//////////

// Utility to avoid multiple number conversion from a string when possible
MC_DLLEXPORT bool MCStringSetNumericValue(MCStringRef self, double p_value);
MC_DLLEXPORT bool MCStringGetNumericValue(MCStringRef self, double &r_value);

enum MCStringLineEndingStyle
{
    kMCStringLineEndingStyleLF,
    kMCStringLineEndingStyleCR,
    kMCStringLineEndingStyleCRLF,
#if defined(__CRLF__)
    kMCStringLineEndingStyleLegacyNative = kMCStringLineEndingStyleCRLF,
#elif defined(__CR__)
    kMCStringLineEndingStyleLegacyNative = kMCStringLineEndingStyleCR,
#elif defined(__LF__)
    kMCStringLineEndingStyleLegacyNative = kMCStringLineEndingStyleLF,
#else
#error Unknown default line ending style (no __CRLF__, __CR__, __LF__ definition)
#endif
};

typedef uint32_t MCStringLineEndingOptions;
enum
{
  /* Normalize any occurrence of PS (paragraph separator) to the specified line-ending style */
  kMCStringLineEndingOptionNormalizePSToLineEnding = 1 << 0,
  /* Normalize any occurrence of LS (line separator) to VT (vertical tab) */
  kMCStringLineEndingOptionNormalizeLSToVT = 1 << 1,
};

// Converts LF, CR, CRLF line endings in p_input to p_to_style line endings and
// places the result into r_output. PS to p_to_style and LS to VTAB conversions
// are performed if the appropriate flags are set in p_options. Detected line
// endings of p_input are returned in r_original_style.
MC_DLLEXPORT bool
MCStringNormalizeLineEndings(MCStringRef p_input, 
                             MCStringLineEndingStyle p_to_style, 
                             MCStringLineEndingOptions p_options,
                             MCStringRef& r_output, 
                             MCStringLineEndingStyle* r_original_style);

////////////////////////////////////////////////////////////////////////////////
//
//  NAME DEFINITIONS
//

// Like MCSTR but for NameRefs
MC_DLLEXPORT MCNameRef MCNAME(const char *);

// Create a name using the given string.
MC_DLLEXPORT bool MCNameCreate(MCStringRef string, MCNameRef& r_name);
// Create a name using chars.
MC_DLLEXPORT bool MCNameCreateWithChars(const unichar_t *chars, uindex_t count, MCNameRef& r_name);
// Create a name using native chars.
MC_DLLEXPORT bool MCNameCreateWithNativeChars(const char_t *chars, uindex_t count, MCNameRef& r_name);
// Create a name using an index
MC_DLLEXPORT bool MCNameCreateWithIndex(index_t p_index, MCNameRef& r_name);
    
// Create a name using the given string, releasing the original.
MC_DLLEXPORT bool MCNameCreateAndRelease(MCStringRef string, MCNameRef& r_name);

// Looks for an existing name matching the given string.
MC_DLLEXPORT MCNameRef MCNameLookupCaseless(MCStringRef string);
// Looks for an existing name matching the given index.
MC_DLLEXPORT MCNameRef MCNameLookupIndex(index_t p_index);

// Returns a unsigned integer which can be used to order a table for a binary
// search.
MC_DLLEXPORT uintptr_t MCNameGetCaselessSearchKey(MCNameRef name);

// Returns the string content of the name.
MC_DLLEXPORT MCStringRef MCNameGetString(MCNameRef name);

// Returns true if the given name is the empty name.
MC_DLLEXPORT bool MCNameIsEmpty(MCNameRef name);

// Returns true if the names are equal under the options specified by options.
MC_DLLEXPORT bool MCNameIsEqualTo(MCNameRef left, MCNameRef right, MCStringOptions p_options);

// Returns true if the names are equal caselessly.
MC_DLLEXPORT bool MCNameIsEqualToCaseless(MCNameRef left, MCNameRef right);

// The empty name object;
MC_DLLEXPORT extern MCNameRef kMCEmptyName;

MC_DLLEXPORT extern MCNameRef kMCTrueName;
MC_DLLEXPORT extern MCNameRef kMCFalseName;

////////////////////////////////////////////////////////////////////////////////
//
//  DATA DEFINITIONS
//
// Immutable data methods

MC_DLLEXPORT extern MCDataRef kMCEmptyData;

MC_DLLEXPORT bool MCDataCreateWithBytes(const byte_t *p_bytes, uindex_t p_byte_count, MCDataRef& r_data);
MC_DLLEXPORT bool MCDataCreateWithBytesAndRelease(byte_t *p_bytes, uindex_t p_byte_count, MCDataRef& r_data);

// Creates data from existing data. The first variant exists to provide an
// optimised implementation in the (very common) case of only two DataRefs.
MC_DLLEXPORT bool MCDataCreateWithData(MCDataRef& r_string, MCDataRef p_one, MCDataRef p_two);
//MC_DLLEXPORT bool MCDataCreateWithData(MCDataRef& r_string, MCDataRef p_one, MCDataRef p_two, MCDataRef p_three, ...);

MC_DLLEXPORT bool MCDataConvertStringToData(MCStringRef string, MCDataRef& r_data);

MC_DLLEXPORT bool MCDataIsEmpty(MCDataRef p_data);

MC_DLLEXPORT uindex_t MCDataGetLength(MCDataRef p_data);
/* Return a pointer to the data string's underlying byte array.  Note
 * that if p_data is empty (i.e. has length zero) then
 * MCDataGetBytePtr() may return null. */
MC_DLLEXPORT const byte_t *MCDataGetBytePtr(MCDataRef p_data);

MC_DLLEXPORT byte_t MCDataGetByteAtIndex(MCDataRef p_data, uindex_t p_index);

MC_DLLEXPORT hash_t MCDataHash(MCDataRef p_data);
MC_DLLEXPORT bool MCDataIsEqualTo(MCDataRef p_left, MCDataRef p_right);

MC_DLLEXPORT compare_t MCDataCompareTo(MCDataRef p_left, MCDataRef p_right);

// Mutable data methods

MC_DLLEXPORT bool MCDataCreateMutable(uindex_t p_initial_capacity, MCDataRef& r_data);

MC_DLLEXPORT bool MCDataCopy(MCDataRef p_data, MCDataRef& r_new_data);
MC_DLLEXPORT bool MCDataCopyAndRelease(MCDataRef p_data, MCDataRef& r_new_data);
MC_DLLEXPORT bool MCDataMutableCopy(MCDataRef p_data, MCDataRef& r_mutable_data);
MC_DLLEXPORT bool MCDataMutableCopyAndRelease(MCDataRef p_data, MCDataRef& r_mutable_data);

MC_DLLEXPORT bool MCDataCopyRange(MCDataRef data, MCRange range, MCDataRef& r_new_data);
MC_DLLEXPORT bool MCDataCopyRangeAndRelease(MCDataRef data, MCRange range, MCDataRef& r_new_data);
MC_DLLEXPORT bool MCDataMutableCopyRange(MCDataRef data, MCRange range, MCDataRef& r_new_data);
MC_DLLEXPORT bool MCDataMutableCopyRangeAndRelease(MCDataRef data, MCRange range, MCDataRef& r_new_data);

MC_DLLEXPORT bool MCDataIsMutable(const MCDataRef p_data);

MC_DLLEXPORT bool MCDataAppend(MCDataRef r_data, MCDataRef p_suffix);
MC_DLLEXPORT bool MCDataAppendBytes(MCDataRef r_data, const byte_t *p_bytes, uindex_t p_byte_count);
MC_DLLEXPORT bool MCDataAppendByte(MCDataRef r_data, byte_t p_byte);

MC_DLLEXPORT bool MCDataPrepend(MCDataRef r_data, MCDataRef p_prefix);
MC_DLLEXPORT bool MCDataPrependBytes(MCDataRef r_data, const byte_t *p_bytes, uindex_t p_byte_count);
MC_DLLEXPORT bool MCDataPrependByte(MCDataRef r_data, byte_t p_byte);

MC_DLLEXPORT bool MCDataInsert(MCDataRef r_data, uindex_t p_at, MCDataRef p_new_data);
MC_DLLEXPORT bool MCDataInsertBytes(MCDataRef self, uindex_t p_at, const byte_t *p_bytes, uindex_t p_byte_count);

MC_DLLEXPORT bool MCDataRemove(MCDataRef r_data, MCRange p_range);
MC_DLLEXPORT bool MCDataReplace(MCDataRef r_data, MCRange p_range, MCDataRef p_new_data);
MC_DLLEXPORT bool MCDataReplaceBytes(MCDataRef r_data, MCRange p_range, const byte_t *p_new_data, uindex_t p_byte_count);

MC_DLLEXPORT bool MCDataReverse(MCDataRef);

MC_DLLEXPORT bool MCDataPad(MCDataRef data, byte_t byte, uindex_t count);

MC_DLLEXPORT bool MCDataContains(MCDataRef p_data, MCDataRef p_needle);
MC_DLLEXPORT bool MCDataBeginsWith(MCDataRef p_data, MCDataRef p_needle);
MC_DLLEXPORT bool MCDataEndsWith(MCDataRef p_data, MCDataRef p_needle);

MC_DLLEXPORT bool MCDataFirstIndexOf(MCDataRef p_data, MCDataRef p_chunk, MCRange t_range, uindex_t& r_index);
MC_DLLEXPORT bool MCDataLastIndexOf(MCDataRef p_data, MCDataRef p_chunk, MCRange t_range, uindex_t& r_index);

#if defined(__HAS_CORE_FOUNDATION__)
MC_DLLEXPORT bool MCDataCreateWithCFDataRef(CFDataRef p_cf_data, MCDataRef& r_data);
MC_DLLEXPORT bool MCDataConvertToCFDataRef(MCDataRef p_data, CFDataRef& r_cf_data);
#endif

////////////////////////////////////////////////////////////////////////////////
//
//  ARRAY DEFINITIONS
//

MC_DLLEXPORT extern MCArrayRef kMCEmptyArray;

// Create an immutable array containing the given keys and values.
MC_DLLEXPORT bool MCArrayCreate(bool case_sensitive, const MCNameRef *keys, const MCValueRef *values, uindex_t length, MCArrayRef& r_array);

// Create an empty mutable array.
MC_DLLEXPORT bool MCArrayCreateMutable(MCArrayRef& r_array);

// Make an immutable copy of the given array. If the 'copy and release' form is
// used then the original array is released (has its reference count reduced by
// one).
MC_DLLEXPORT bool MCArrayCopy(MCArrayRef array, MCArrayRef& r_new_array);
MC_DLLEXPORT bool MCArrayCopyAndRelease(MCArrayRef array, MCArrayRef& r_new_array);

// Make a mutable copy of the given array. If the 'copy and release' form is
// used then the original array is released (has its reference count reduced by
// one).
MC_DLLEXPORT bool MCArrayMutableCopy(MCArrayRef array, MCArrayRef& r_new_array);
MC_DLLEXPORT bool MCArrayMutableCopyAndRelease(MCArrayRef array, MCArrayRef& r_new_array);

// Returns 'true' if the given array is mutable.
MC_DLLEXPORT bool MCArrayIsMutable(MCArrayRef array);

// Returns the number of elements in the array.
MC_DLLEXPORT uindex_t MCArrayGetCount(MCArrayRef array);

// Fetch the value from the array with the given key. The returned value is
// not retained. If being stored elsewhere ValueCopy should be used to make an
// immutable copy first. If 'false' is returned it means the key was not found
// (in this case r_value is undefined).
MC_DLLEXPORT bool MCArrayFetchValue(MCArrayRef array, bool case_sensitive, MCNameRef key, MCValueRef& r_value);
// Store the given value into the array. The value is copied appropriately so
// that the original can still be modified without affecting the copy in the
// array.
MC_DLLEXPORT bool MCArrayStoreValue(MCArrayRef array, bool case_sensitive, MCNameRef key, MCValueRef value);
// Remove the given key from the array.
MC_DLLEXPORT bool MCArrayRemoveValue(MCArrayRef array, bool case_sensitive, MCNameRef key);

// Fetches index i in the given (sequence) array.
MC_DLLEXPORT bool MCArrayFetchValueAtIndex(MCArrayRef array, index_t index, MCValueRef& r_value);
// Store index i in the given (sequence) array.
MC_DLLEXPORT bool MCArrayStoreValueAtIndex(MCArrayRef array, index_t index, MCValueRef value);
// Remove index i from the given (sequence) array.
MC_DLLEXPORT bool MCArrayRemoveValueAtIndex(MCArrayRef array, index_t index);

// Fetch the value from the array on the given path. The returned value is
// not retained. If being stored elsewhere ValueCopy should be used to make an
// immutable copy first. If 'false' is returned it means the key was not found
// (in this case r_value is undefined).
MC_DLLEXPORT bool MCArrayFetchValueOnPath(MCArrayRef array, bool case_sensitive, const MCNameRef *path, uindex_t path_length, MCValueRef& r_value);
// Store the given value into the array on the given path. The value is retained
// and not copied before being inserted.
MC_DLLEXPORT bool MCArrayStoreValueOnPath(MCArrayRef array, bool case_sensitive, const MCNameRef *path, uindex_t path_length, MCValueRef value);
// Remove the value on the given path in the array.
MC_DLLEXPORT bool MCArrayRemoveValueOnPath(MCArrayRef array, bool case_sensitive, const MCNameRef *path, uindex_t path_length);

// Apply the callback function to each element of the array. Do not modify the
// array within the callback as this will cause undefined behavior.
typedef bool (*MCArrayApplyCallback)(void *context, MCArrayRef array, MCNameRef key, MCValueRef value);
MC_DLLEXPORT bool MCArrayApply(MCArrayRef array, MCArrayApplyCallback callback, void *context);

// Iterate through the array. 'iterator' should be initialized to zero the first
// time the method is called, and should then be called until it returns 'false'.
// A return value of 'false' means no further elements. Do not modify the array
// inbetween calls to Iterate as this will cause undefined behavior.
MC_DLLEXPORT bool MCArrayIterate(MCArrayRef array, uintptr_t& iterator, MCNameRef& r_key, MCValueRef& r_value);

// Returns true if the given array is the empty array.
MC_DLLEXPORT bool MCArrayIsEmpty(MCArrayRef self);
    
#if defined(__HAS_CORE_FOUNDATION__)
// If p_use_lists is true, then any arrays which look like sequences will be
// converted to MCProperListRef / CFArrayRef (depending on direction).
    
MC_DLLEXPORT bool MCArrayCreateWithCFDictionaryRef(CFDictionaryRef p_cf_dictionary, bool p_use_lists, MCArrayRef& r_array);
MC_DLLEXPORT bool MCArrayConvertToCFDictionaryRef(MCArrayRef p_value, bool p_use_lists, CFDictionaryRef& r_cf_value); 
MC_DLLEXPORT bool MCArrayCreateWithCFArrayRef(CFArrayRef p_cf_array, bool p_use_lists, MCArrayRef& r_array);
// Attempt to convert the array to a CFArrayRef. If the array cannot be
// converted, r_cf_value is set to nullptr and true is returned.
MC_DLLEXPORT bool MCArrayConvertToCFArrayRef(MCArrayRef p_value, bool p_use_lists, CFArrayRef& r_cf_value);
#endif

// Attempt to convert the array to a properlist. If the array cannot be
// converted to a list, r_list is set to nullptr and true is returned.
MC_DLLEXPORT bool MCArrayConvertToProperList(MCArrayRef p_array, MCProperListRef& r_list);
    
////////////////////////////////////////////////////////////////////////////////
//
//  LIST DEFINITIONS
//

MC_DLLEXPORT extern MCListRef kMCEmptyList;

// Create a list from a static list of values - NOT IMPLEMENTED YET.
// bool MCListCreate(char delimiter, const MCValueRef *values, uindex_t value_count, MCListRef& r_list);

}

// Create a mutable list - initially empty.
bool MCListCreateMutable(char_t delimiter, MCListRef& r_list);
bool MCListCreateMutable(MCStringRef p_delimiter, MCListRef& r_list);

extern "C" {

// Eventually this will accept any value type, but for now - just strings, names, and booleans.
MC_DLLEXPORT bool MCListAppend(MCListRef list, MCValueRef value);

// Append a substring to the list.
MC_DLLEXPORT bool MCListAppendSubstring(MCListRef list, MCStringRef value, MCRange range);

// Append a sequence of native chars as an element.
MC_DLLEXPORT bool MCListAppendNativeChars(MCListRef list, const char_t *chars, uindex_t char_count);

// Append a formatted string as an element.
MC_DLLEXPORT bool MCListAppendFormat(MCListRef list, const char *format, ...);

// Make an immutable copy of the list.
MC_DLLEXPORT bool MCListCopy(MCListRef list, MCListRef& r_new_list);

// Makes an immutable copy of the list and release the list
MC_DLLEXPORT bool MCListCopyAndRelease(MCListRef list, MCListRef& r_new_list);

// Make a copy of the list as a string.
MC_DLLEXPORT bool MCListCopyAsString(MCListRef list, MCStringRef& r_string);

// Make an copy of the list as a string and release the list.
MC_DLLEXPORT bool MCListCopyAsStringAndRelease(MCListRef list, MCStringRef& r_string);

// Returns true if nothing has been added to the list
MC_DLLEXPORT bool MCListIsEmpty(MCListRef list);

////////////////////////////////////////////////////////////////////////////////
//
//  SET DEFINITIONS
//

extern MCSetRef kMCEmptySet;

MC_DLLEXPORT bool MCSetCreateSingleton(uindex_t element, MCSetRef& r_set);
MC_DLLEXPORT bool MCSetCreateWithIndices(uindex_t *elements, uindex_t element_count, MCSetRef& r_set);
MC_DLLEXPORT bool MCSetCreateWithLimbsAndRelease(uindex_t *limbs, uindex_t limb_count, MCSetRef& r_set);

MC_DLLEXPORT bool MCSetCreateMutable(MCSetRef& r_set);

MC_DLLEXPORT bool MCSetCopy(MCSetRef set, MCSetRef& r_new_set);
MC_DLLEXPORT bool MCSetCopyAndRelease(MCSetRef set, MCSetRef& r_new_set);

MC_DLLEXPORT bool MCSetMutableCopy(MCSetRef set, MCSetRef& r_new_set);
MC_DLLEXPORT bool MCSetMutableCopyAndRelease(MCSetRef set, MCSetRef& r_new_set);

MC_DLLEXPORT bool MCSetIsMutable(MCSetRef self);

MC_DLLEXPORT bool MCSetIsEmpty(MCSetRef set);
MC_DLLEXPORT bool MCSetIsEqualTo(MCSetRef set, MCSetRef other_set);

MC_DLLEXPORT bool MCSetContains(MCSetRef set, MCSetRef other_set);
MC_DLLEXPORT bool MCSetIntersects(MCSetRef set, MCSetRef other_set);

MC_DLLEXPORT bool MCSetContainsIndex(MCSetRef set, uindex_t index);

MC_DLLEXPORT bool MCSetIncludeIndex(MCSetRef set, uindex_t index);
MC_DLLEXPORT bool MCSetExcludeIndex(MCSetRef set, uindex_t index);

MC_DLLEXPORT bool MCSetUnion(MCSetRef set, MCSetRef other_set);
MC_DLLEXPORT bool MCSetDifference(MCSetRef set, MCSetRef other_set);
MC_DLLEXPORT bool MCSetIntersect(MCSetRef set, MCSetRef other_set);

MC_DLLEXPORT bool MCSetIterate(MCSetRef set, uindex_t& x_iterator, uindex_t& r_element);
MC_DLLEXPORT bool MCSetList(MCSetRef set, uindex_t*& r_element, uindex_t& r_element_count);

////////////////////////////////////////////////////////////////////////////////
//
//  RECORD DEFINITIONS
//

MC_DLLEXPORT bool MCRecordCreate(MCTypeInfoRef typeinfo, const MCValueRef *values, uindex_t value_count, MCRecordRef& r_record);

MC_DLLEXPORT bool MCRecordCreateMutable(MCTypeInfoRef p_typeinfo, MCRecordRef& r_record);

MC_DLLEXPORT bool MCRecordCopy(MCRecordRef record, MCRecordRef& r_new_record);
MC_DLLEXPORT bool MCRecordCopyAndRelease(MCRecordRef record, MCRecordRef& r_new_record);

MC_DLLEXPORT bool MCRecordMutableCopy(MCRecordRef record, MCRecordRef& r_new_record);
MC_DLLEXPORT bool MCRecordMutableCopyAndRelease(MCRecordRef record, MCRecordRef& r_new_record);

MC_DLLEXPORT bool MCRecordIsMutable(MCRecordRef self);

MC_DLLEXPORT bool MCRecordFetchValue(MCRecordRef record, MCNameRef field, MCValueRef& r_value);
MC_DLLEXPORT bool MCRecordStoreValue(MCRecordRef record, MCNameRef field, MCValueRef value);

MC_DLLEXPORT bool MCRecordEncodeAsArray(MCRecordRef record, MCArrayRef & r_array);
MC_DLLEXPORT bool MCRecordDecodeFromArray(MCArrayRef array, MCTypeInfoRef p_typeinfo, MCRecordRef & r_record);

MC_DLLEXPORT bool MCRecordIterate(MCRecordRef record, uintptr_t& x_iterator, MCNameRef& r_field, MCValueRef& r_value);
    
////////////////////////////////////////////////////////////////////////////////
//
//  HANDLER DEFINITIONS
//

enum MCHandlerQueryType
{
    kMCHandlerQueryTypeNone,
    kMCHandlerQueryTypeObjcSelector,
};
    
struct MCHandlerCallbacks
{
    size_t size;
    void (*release)(void *context);
    bool (*invoke)(void *context, MCValueRef *arguments, uindex_t argument_count, MCValueRef& r_value);
	bool (*describe)(void *context, MCStringRef& r_desc);
    bool (*query)(void *context, MCHandlerQueryType type, void *r_info);
};

MC_DLLEXPORT bool MCHandlerCreate(MCTypeInfoRef typeinfo, const MCHandlerCallbacks *callbacks, void *context, MCHandlerRef& r_handler);

MC_DLLEXPORT void *MCHandlerGetContext(MCHandlerRef handler);
MC_DLLEXPORT const MCHandlerCallbacks *MCHandlerGetCallbacks(MCHandlerRef handler);
    
/* Invoke the given handler with the specified arguments. If an error is thrown
 * then false is returned.
 * Note: The normal version must be called from the engine thread. Use
 *       ExternalInvoke if the current thread is unknown. */
MC_DLLEXPORT bool MCHandlerInvoke(MCHandlerRef handler, MCValueRef *arguments, uindex_t argument_count, MCValueRef& r_value);
MC_DLLEXPORT bool MCHandlerExternalInvoke(MCHandlerRef handler, MCValueRef *arguments, uindex_t argument_count, MCValueRef& r_value);

/* Invoke the given handler with the specified arguments. If an error is thrown
 * then the error object is returned.
 * Note: The normal version must be called from the engine thread. Use
 *       ExternalInvoke if the current thread is unknown. */
MC_DLLEXPORT /*copy*/ MCErrorRef MCHandlerTryToInvokeWithList(MCHandlerRef handler, MCProperListRef& x_arguments, MCValueRef& r_value);
MC_DLLEXPORT /*copy*/ MCErrorRef MCHandlerTryToExternalInvokeWithList(MCHandlerRef handler, MCProperListRef& x_arguments, MCValueRef& r_value);
   
/* Create a C function ptr which calls the given handler ref. The function ptr is
 * stored in the handler ref and is freed when it is. On 32-bit Win32, the calling
 * convention used is __cdecl. */
MC_DLLEXPORT bool MCHandlerGetFunctionPtr(MCHandlerRef handler, void*& r_func_ptr);

/* Create a C function ptr with a specific ABI. The function ptr is stored in the
 * handler ref and is freed when it is. The ABI argument only has an effect on 32-bit
 * Win32, on all other platforms it is ignored. */
enum MCHandlerAbiKind
{
	kMCHandlerAbiDefault,
	kMCHandlerAbiStdCall,
	kMCHandlerAbiThisCall,
	kMCHandlerAbiFastCall,
	kMCHandlerAbiCDecl,
	kMCHandlerAbiPascal,
	kMCHandlerAbiRegister
};
MC_DLLEXPORT bool MCHandlerGetFunctionPtrWithAbi(MCHandlerRef handler, MCHandlerAbiKind p_convention, void*& r_func_ptr);

////////////////////////////////////////////////////////////////////////////////
//
//  ERROR DEFINITIONS
//

MC_DLLEXPORT extern MCTypeInfoRef kMCOutOfMemoryErrorTypeInfo;
MC_DLLEXPORT extern MCTypeInfoRef kMCGenericErrorTypeInfo;
MC_DLLEXPORT extern MCTypeInfoRef kMCUnboundTypeErrorTypeInfo;
MC_DLLEXPORT extern MCTypeInfoRef kMCUnimplementedErrorTypeInfo;

MC_DLLEXPORT bool MCErrorCreate(MCTypeInfoRef typeinfo, MCArrayRef info, MCErrorRef& r_error);
MC_DLLEXPORT bool MCErrorCreateS(MCErrorRef& r_error,
								 MCTypeInfoRef typeinfo,
								 ...);
MC_DLLEXPORT bool MCErrorCreateV(MCErrorRef& r_error,
								 MCTypeInfoRef typeinfo,
								 va_list args);

MC_DLLEXPORT bool MCErrorCreateWithMessage(MCTypeInfoRef typeinfo, MCStringRef message, MCArrayRef info, MCErrorRef & r_error);
MC_DLLEXPORT bool MCErrorCreateWithMessageS(MCErrorRef& r_error,
											MCTypeInfoRef typeinfo,
											MCStringRef message,
											...);
MC_DLLEXPORT bool MCErrorCreateWithMessageV(MCErrorRef& r_error,
											MCTypeInfoRef typeinfo,
											MCStringRef message,
											va_list args);
	
MC_DLLEXPORT bool MCErrorUnwind(MCErrorRef error, MCValueRef target, uindex_t row, uindex_t column);

MC_DLLEXPORT MCNameRef MCErrorGetDomain(MCErrorRef error);
MC_DLLEXPORT MCArrayRef MCErrorGetInfo(MCErrorRef error);
MC_DLLEXPORT MCStringRef MCErrorGetMessage(MCErrorRef error);

MC_DLLEXPORT uindex_t MCErrorGetDepth(MCErrorRef error);
MC_DLLEXPORT MCValueRef MCErrorGetTargetAtLevel(MCErrorRef error, uindex_t level);
MC_DLLEXPORT uindex_t MCErrorGetRowAtLevel(MCErrorRef error, uindex_t row);
MC_DLLEXPORT uindex_t MCErrorGetColumnAtLevel(MCErrorRef error, uindex_t column);

// Create and throw an error. The arguments are used to build the info dictionary.
// They should be a sequence of pairs (const char *key, MCValueRef value), and finish
// with nil.
MC_DLLEXPORT bool MCErrorCreateAndThrow(MCTypeInfoRef typeinfo, ...);

MC_DLLEXPORT bool MCErrorCreateAndThrowWithMessage(MCTypeInfoRef typeinfo, MCStringRef message_format, ...);
    
// Throw the given error code (local to the current thread).
MC_DLLEXPORT bool MCErrorThrow(MCErrorRef error);

// Catch the current error code (on the current thread) if any and clear it.
MC_DLLEXPORT bool MCErrorCatch(MCErrorRef& r_error);

// Resets the error state on the current thread. This call is equivalent to:
//     MCAutoErrorRef t_error;
//     MCErrorCatach(&t_error);
MC_DLLEXPORT void MCErrorReset(void);

// Returns true if there is an error pending on the current thread.
MC_DLLEXPORT bool MCErrorIsPending(void);

// Returns any pending error (on the current thread) without clearing it.
MC_DLLEXPORT MCErrorRef MCErrorPeek(void);


// Throw an out of memory error.
MC_DLLEXPORT bool MCErrorThrowOutOfMemory(void);
    
// Throw an unbound type error.
MC_DLLEXPORT bool MCErrorThrowUnboundType(MCTypeInfoRef type);
    
// Throw an unimplemented error.
MC_DLLEXPORT bool MCErrorThrowUnimplemented(MCStringRef thing);

// Throw a generic runtime error (one that hasn't had a class made for it yet).
// The message argument is optional (nil if no message).
MC_DLLEXPORT bool MCErrorThrowGeneric(MCStringRef message);
    
// Throw a generic runtime error with formatted message.
MC_DLLEXPORT bool MCErrorThrowGenericWithMessage(MCStringRef message, ...);

////////////////////////////////////////////////////////////////////////////////
//
//  FOREIGN DEFINITIONS
//

MC_DLLEXPORT extern MCTypeInfoRef kMCForeignImportErrorTypeInfo;
MC_DLLEXPORT extern MCTypeInfoRef kMCForeignExportErrorTypeInfo;

MC_DLLEXPORT bool MCForeignValueCreate(MCTypeInfoRef typeinfo, void *contents, MCForeignValueRef& r_value);
MC_DLLEXPORT bool MCForeignValueCreateAndRelease(MCTypeInfoRef typeinfo, void *contents, MCForeignValueRef& r_value);

MC_DLLEXPORT void *MCForeignValueGetContentsPtr(MCValueRef value);

MC_DLLEXPORT bool MCForeignValueExport(MCTypeInfoRef typeinfo, MCValueRef value, MCForeignValueRef& r_value);

////////////////////////////////////////////////////////////////////////////////
//
//  STREAM DEFINITIONS
//

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
MC_DLLEXPORT bool MCStreamCreate(const MCStreamCallbacks *callbacks, size_t extra_bytes, MCStreamRef& r_stream);

// Fetch the callback pointer attached to the stream - this can be used for type-
// checking.
MC_DLLEXPORT const MCStreamCallbacks *MCStreamGetCallbacks(MCStreamRef stream);

// Fetch the start of the 'extra bytes' for the stream.
MC_DLLEXPORT inline void *MCStreamGetExtraBytesPtr(MCStreamRef stream) { return ((uint8_t *)MCValueGetExtraBytesPtr(stream)) + kMCStreamHeaderSize; }

// Memory-based input stream

// Create a read-only stream targetting a memory block. The lifetime of the
// memory block must exceed that of the stream.
MC_DLLEXPORT bool MCMemoryInputStreamCreate(const void *block, size_t size, MCStreamRef& r_stream);

// Memory-based output stream

// Create a write-only stream that outputs to memory.
MC_DLLEXPORT bool MCMemoryOutputStreamCreate(MCStreamRef& r_stream);
// Finish the memory output and return the buffer. The stream will continue
// to work, but will be reset to empty and a new buffer accumulated.
MC_DLLEXPORT bool MCMemoryOutputStreamFinish(MCStreamRef stream, void*& r_buffer, size_t& r_size);

// Capabilities

// If a stream is readable it means that the 'Read' method work.
MC_DLLEXPORT bool MCStreamIsReadable(MCStreamRef stream);
// If a stream is writable it means that the 'Write' method works.
MC_DLLEXPORT bool MCStreamIsWritable(MCStreamRef stream);
// If a stream is markable it means that the stream supports the 'Mark' and
// 'Reset' methods.
MC_DLLEXPORT bool MCStreamIsMarkable(MCStreamRef stream);
// If a stream is seekable it means that the stream supports the 'Tell' and
// 'Seek' methods.
MC_DLLEXPORT bool MCStreamIsSeekable(MCStreamRef stream);

// Readable streams

// Returns the number of bytes available on the stream without blocking (or
// hitting eof).
MC_DLLEXPORT bool MCStreamGetAvailableForRead(MCStreamRef stream, size_t& r_available);
// Attempt to read 'amount' bytes into the given buffer.
MC_DLLEXPORT bool MCStreamRead(MCStreamRef stream, void *buffer, size_t amount);

// Returns the number of bytes space available on the stream for writing
// without blocking.
MC_DLLEXPORT bool MCStreamGetAvailableForWrite(MCStreamRef stream, size_t& r_available);
// Attempt to write 'amount' bytes to the output stream.
MC_DLLEXPORT bool MCStreamWrite(MCStreamRef stream, const void *buffer, size_t amount);

// Readable and writable streams.

// If reading or writing a single byte would result in failure due to end of
// file, 'finished' will contain true.
MC_DLLEXPORT bool MCStreamIsFinished(MCStreamRef stream, bool& r_finished);

// Attempt to skip 'amount' bytes. This call only succeeds if the stream pointer
// is 'amount' bytes away from eod.
MC_DLLEXPORT bool MCStreamSkip(MCStreamRef stream, size_t amount);

// Markable streams only

// Record the current location in the stream. If the stream head moves no further
// than 'read_limit' bytes forward, then the 'Reset' method will allow a return to
// the mark.
MC_DLLEXPORT bool MCStreamMark(MCStreamRef stream, size_t read_limit);
// Reset the stream to the last recorded mark.
MC_DLLEXPORT bool MCStreamReset(MCStreamRef stream);

// Seekable streams only

// Fetch the current stream position.
MC_DLLEXPORT bool MCStreamTell(MCStreamRef stream, filepos_t& r_position);
// Set the current stream position.
MC_DLLEXPORT bool MCStreamSeek(MCStreamRef stream, filepos_t position);

// Simple byte-based serialization / unserialization functions. These are all
// wrappers around read/write, and assume no higher-level structure.

// Fixed-size unsigned and signed integer functions.
MC_DLLEXPORT bool MCStreamReadUInt8(MCStreamRef stream, uint8_t& r_value);
MC_DLLEXPORT bool MCStreamReadUInt16(MCStreamRef stream, uint16_t& r_value);
MC_DLLEXPORT bool MCStreamReadUInt32(MCStreamRef stream, uint32_t& r_value);
MC_DLLEXPORT bool MCStreamReadUInt64(MCStreamRef stream, uint64_t& r_value);
MC_DLLEXPORT bool MCStreamReadInt8(MCStreamRef stream, int8_t& r_value);
MC_DLLEXPORT bool MCStreamReadInt16(MCStreamRef stream, int16_t& r_value);
MC_DLLEXPORT bool MCStreamReadInt32(MCStreamRef stream, int32_t& r_value);
MC_DLLEXPORT bool MCStreamReadInt64(MCStreamRef stream, int64_t& r_value);

MC_DLLEXPORT bool MCStreamWriteUInt8(MCStreamRef stream, uint8_t value);
MC_DLLEXPORT bool MCStreamWriteUInt16(MCStreamRef stream, uint16_t value);
MC_DLLEXPORT bool MCStreamWriteUInt32(MCStreamRef stream, uint32_t value);
MC_DLLEXPORT bool MCStreamWriteUInt64(MCStreamRef stream, uint64_t value);
MC_DLLEXPORT bool MCStreamWriteInt8(MCStreamRef stream, int8_t value);
MC_DLLEXPORT bool MCStreamWriteInt16(MCStreamRef stream, int16_t value);
MC_DLLEXPORT bool MCStreamWriteInt32(MCStreamRef stream, int32_t value);
MC_DLLEXPORT bool MCStreamWriteInt64(MCStreamRef stream, int64_t value);

// Variable-sized unsigned and signing integer functions. These methods use
// the top-bit of successive bytes to indicate whether more bytes follow - each
// byte encodes 7 bits of information.
MC_DLLEXPORT bool MCStreamReadCompactUInt32(MCStreamRef stream, uint32_t& r_value);
MC_DLLEXPORT bool MCStreamReadCompactUInt64(MCStreamRef stream, uint64_t& r_value);
MC_DLLEXPORT bool MCStreamReadCompactSInt32(MCStreamRef stream, uint32_t& r_value);
MC_DLLEXPORT bool MCStreamReadCompactSInt64(MCStreamRef stream, uint64_t& r_value);

// Fixed-size binary-floating point functions.
MC_DLLEXPORT bool MCStreamReadFloat(MCStreamRef stream, float& r_value);
MC_DLLEXPORT bool MCStreamReadDouble(MCStreamRef stream, double& r_value);

MC_DLLEXPORT bool MCStreamWriteFloat(MCStreamRef stream, float value);
MC_DLLEXPORT bool MCStreamWriteDouble(MCStreamRef stream, double value);

// Known valueref functions - these assume the given type is present.
MC_DLLEXPORT bool MCStreamReadBoolean(MCStreamRef stream, MCBooleanRef& r_boolean);
MC_DLLEXPORT bool MCStreamReadNumber(MCStreamRef stream, MCNumberRef& r_number);
MC_DLLEXPORT bool MCStreamReadName(MCStreamRef stream, MCNameRef& r_name);
MC_DLLEXPORT bool MCStreamReadString(MCStreamRef stream, MCStringRef& r_string);
MC_DLLEXPORT bool MCStreamReadArray(MCStreamRef stream, MCArrayRef& r_array);
MC_DLLEXPORT bool MCStreamReadSet(MCStreamRef stream, MCSetRef& r_set);

// Variant valueref functions - these tag the data with the type, allowing
// easy encoding/decoding of any value type (that supports serialization).
MC_DLLEXPORT bool MCStreamReadValue(MCStreamRef stream, MCValueRef& r_value);

////////////////////////////////////////////////////////////////////////////////
//
//  PROPER LIST DEFINITIONS
//

// The type describing options for list sorting.
typedef uint32_t MCProperListSortType;
enum
{
	// Sort using a codepoint by codepoint comparison.
	kMCProperListSortTypeText = 0,
	// Sort using a byte by byte comparison.
	kMCProperListSortTypeBinary = 1,
    // Sort by collation according to the system locale.
	kMCProperListSortTypeInternational = 2,
	// Sorts numerically
	kMCProperListSortTypeNumeric = 3,
    // Sorts chronologically by date / time
	kMCProperListSortTypeDateTime = 4,
};

/////////

MC_DLLEXPORT extern MCProperListRef kMCEmptyProperList;

// Create an immutable list containing the given values.
MC_DLLEXPORT bool MCProperListCreate(const MCValueRef *values, uindex_t length, MCProperListRef& r_list);

// Create an immutable list containing valuerefs built from a sequence of
// raw values. The raw values should be sequential in memory, size(type) apart.
// If the foreign type does not bridge (has no import method), then a boxed
// foreign value is created (MCForeignValueRef).
MC_DLLEXPORT bool MCProperListCreateWithForeignValues(MCTypeInfoRef type, const void *values, uindex_t value_count, MCProperListRef& r_list);

// Create a c-array of foreign values from the given list. The original list is
// not released. Ownership of the foreign value array is passed to the caller.
// p_typeinfo must be a foreign typeinfo. If any value in the list is not of the
// given type, or cannot be exported as that type, this returns false.
MC_DLLEXPORT bool MCProperListConvertToForeignValues(MCProperListRef list, MCTypeInfoRef p_typeinfo, void*& r_values_ptr, uindex_t& r_values_count);
    
// Create an empty mutable list.
MC_DLLEXPORT bool MCProperListCreateMutable(MCProperListRef& r_list);

// Create an immutable list taking ownership of the given array of
// values.  Takes ownership of both the underlying MCValueRef
// references, and the p_values buffer.
bool MCProperListCreateAndRelease(MCValueRef *p_values, uindex_t p_length, MCProperListRef& r_list);
    
// Make an immutable copy of the given list. If the 'copy and release' form is
// used then the original list is released (has its reference count reduced by
// one).
MC_DLLEXPORT bool MCProperListCopy(MCProperListRef list, MCProperListRef& r_new_list);
MC_DLLEXPORT bool MCProperListCopyAndRelease(MCProperListRef list, MCProperListRef& r_new_list);

// Make a mutable copy of the given list. If the 'copy and release' form is
// used then the original list is released (has its reference count reduced by
// one).
MC_DLLEXPORT bool MCProperListMutableCopy(MCProperListRef list, MCProperListRef& r_new_list);
MC_DLLEXPORT bool MCProperListMutableCopyAndRelease(MCProperListRef list, MCProperListRef& r_new_list);

// Returns 'true' if the given list is mutable.
MC_DLLEXPORT bool MCProperListIsMutable(MCProperListRef list);

// Returns the number of elements in the list.
MC_DLLEXPORT uindex_t MCProperListGetLength(MCProperListRef list);

// Returns true if the given list is the empty list.
MC_DLLEXPORT bool MCProperListIsEmpty(MCProperListRef list);

// Iterate over the elements in the list.
MC_DLLEXPORT bool MCProperListIterate(MCProperListRef list, uintptr_t& x_iterator, MCValueRef& r_element);

// Apply the callback to each element of list. The contents should not be modified.
typedef bool (*MCProperListApplyCallback)(void *context, MCValueRef element);
MC_DLLEXPORT bool MCProperListApply(MCProperListRef list, MCProperListApplyCallback p_callback, void *context);

// Apply the callback to each element of list to create a new list.
typedef bool (*MCProperListMapCallback)(void *context, MCValueRef element, MCValueRef& r_new_element);
MC_DLLEXPORT bool MCProperListMap(MCProperListRef list, MCProperListMapCallback p_callback, MCProperListRef& r_new_list, void *context);

// Sort list by comparing elements using the provided callback.
typedef compare_t (*MCProperListQuickSortCallback)(const MCValueRef *left, const MCValueRef *right);
MC_DLLEXPORT bool MCProperListSort(MCProperListRef list, bool p_reverse, MCProperListQuickSortCallback p_callback);

typedef compare_t (*MCProperListCompareElementCallback)(void *context, const MCValueRef left, const MCValueRef right);
MC_DLLEXPORT bool MCProperListStableSort(MCProperListRef list, bool p_reverse, MCProperListCompareElementCallback p_callback, void *context);

MC_DLLEXPORT bool MCProperListReverse(MCProperListRef list);

// Fetch the first element of the list. The returned value is not retained.
MC_DLLEXPORT MCValueRef MCProperListFetchHead(MCProperListRef list);
MC_DLLEXPORT // Fetch the last element of the list. The returned value is not retained.
MCValueRef MCProperListFetchTail(MCProperListRef list);
// Fetch the element of the list at the specified index. The returned value is not retained.
MC_DLLEXPORT MCValueRef MCProperListFetchElementAtIndex(MCProperListRef list, uindex_t p_index);

// Copy the elements at the specified range as a list.
MC_DLLEXPORT bool MCProperListCopySublist(MCProperListRef list, MCRange p_range, MCProperListRef& r_elements);

MC_DLLEXPORT bool MCProperListPushElementOntoFront(MCProperListRef list, MCValueRef p_value);
MC_DLLEXPORT bool MCProperListPushElementOntoBack(MCProperListRef list, MCValueRef p_value);
// Pushes all of the values in p_values onto the end of the list
MC_DLLEXPORT bool MCProperListPushElementsOntoFront(MCProperListRef self, const MCValueRef *p_values, uindex_t p_length);
MC_DLLEXPORT bool MCProperListPushElementsOntoBack(MCProperListRef self, const MCValueRef *p_values, uindex_t p_length);

MC_DLLEXPORT bool MCProperListAppendList(MCProperListRef list, MCProperListRef p_value);

// The returned value is owned by the caller.
MC_DLLEXPORT bool MCProperListPopFront(MCProperListRef list, MCValueRef& r_value);
MC_DLLEXPORT bool MCProperListPopBack(MCProperListRef list, MCValueRef& r_value);

MC_DLLEXPORT bool MCProperListInsertElement(MCProperListRef list, MCValueRef p_value, index_t p_index);
MC_DLLEXPORT bool MCProperListInsertElements(MCProperListRef list, const MCValueRef *p_value, uindex_t p_length, index_t p_index);
MC_DLLEXPORT bool MCProperListInsertList(MCProperListRef list, MCProperListRef p_value, index_t p_index);

MC_DLLEXPORT bool MCProperListRemoveElement(MCProperListRef list, uindex_t p_index);
MC_DLLEXPORT bool MCProperListRemoveElements(MCProperListRef list, uindex_t p_start, uindex_t p_finish);

MC_DLLEXPORT bool MCProperListFirstIndexOfElement(MCProperListRef list, MCValueRef p_needle, uindex_t p_after, uindex_t& r_offset);
MC_DLLEXPORT bool MCProperListFirstIndexOfElementInRange(MCProperListRef list, MCValueRef p_needle, MCRange p_range, uindex_t& r_offset);

MC_DLLEXPORT bool MCProperListLastIndexOfElementInRange(MCProperListRef list, MCValueRef p_needle, MCRange p_range, uindex_t & r_offset);

MC_DLLEXPORT bool MCProperListFirstOffsetOfList(MCProperListRef list, MCProperListRef p_needle, uindex_t p_after, uindex_t& r_offset);
MC_DLLEXPORT bool MCProperListFirstOffsetOfListInRange(MCProperListRef list, MCProperListRef p_needle, MCRange p_range, uindex_t & r_offset);
MC_DLLEXPORT bool MCProperListLastOffsetOfListInRange(MCProperListRef list, MCProperListRef p_needle, MCRange p_range, uindex_t & r_offset);

MC_DLLEXPORT bool MCProperListIsEqualTo(MCProperListRef list, MCProperListRef p_other);

MC_DLLEXPORT bool MCProperListBeginsWithList(MCProperListRef list, MCProperListRef p_prefix);
MC_DLLEXPORT bool MCProperListEndsWithList(MCProperListRef list, MCProperListRef p_suffix);

MC_DLLEXPORT bool MCProperListIsListOfType(MCProperListRef list, MCValueTypeCode p_type);
MC_DLLEXPORT bool MCProperListIsHomogeneous(MCProperListRef list, MCValueTypeCode& r_type);
    
#if defined(__HAS_CORE_FOUNDATION__)
MC_DLLEXPORT bool MCProperListCreateWithCFArrayRef(CFArrayRef p_cf_dictionary, bool p_use_lists, MCProperListRef& r_list);
MC_DLLEXPORT bool MCProperListConvertToCFArrayRef(MCProperListRef p_list, bool p_use_lists, CFArrayRef& r_list);
#endif
    
MC_DLLEXPORT bool MCProperListConvertToArray(MCProperListRef p_list, MCArrayRef& r_array);
    
////////////////////////////////////////////////////////////////////////////////
//
//  OBJC DEFINITIONS
//

/* The ObjcObject type manages the lifetime of the obj-c object it contains.
 * Specifcally, it sends 'release' to the object when the ObjcObject is dropped */
MC_DLLEXPORT extern MCTypeInfoRef kMCObjcObjectTypeInfo;
MC_DLLEXPORT MCTypeInfoRef MCObjcObjectTypeInfo(void) ATTRIBUTE_PURE;

/* The ObjcId type describes an id which is passed into, or out of an obj-c
 * method with no implicit action on its reference count. */
MC_DLLEXPORT extern MCTypeInfoRef kMCObjcIdTypeInfo;
MC_DLLEXPORT MCTypeInfoRef MCObjcIdTypeInfo(void) ATTRIBUTE_PURE;

/* The ObjcRetainedId type describes an id which is passed into, or out of an
 * obj-c method and is expected to already have been retained. (i.e. the
 * caller or callee expects to receive it with +1 ref count). */
MC_DLLEXPORT extern MCTypeInfoRef kMCObjcRetainedIdTypeInfo;
MC_DLLEXPORT MCTypeInfoRef MCObjcRetainedIdTypeInfo(void) ATTRIBUTE_PURE;

/* The ObjcAutoreleasedId type describes an id which has been placed in the
 * innermost autorelease pool before being returned to the caller. */
MC_DLLEXPORT extern MCTypeInfoRef kMCObjcAutoreleasedIdTypeInfo;
MC_DLLEXPORT MCTypeInfoRef MCObjcAutoreleasedIdTypeInfo(void) ATTRIBUTE_PURE;

/* The ObjcObjectCreateWithId function creates an ObjcObject out of a raw id
 * value, retaining it to make sure it owns a reference to it. */
MC_DLLEXPORT bool MCObjcObjectCreateWithId(void *value, MCObjcObjectRef& r_obj);

/* The ObjcObjectCreateWithId function creates an ObjcObject out of a raw id
 * value, taking a +1 reference count from it (i.e. it assumes the value has
 * already been retained before being called). */
MC_DLLEXPORT bool MCObjcObjectCreateWithRetainedId(void *value, MCObjcObjectRef& r_obj);

/* The ObjcObjectCreateWithAutoreleasedId function creates an ObjcObject out of
 * a raw id value which is in the innermost autorelease pool. Currently this
 * means that it retains it. */
MC_DLLEXPORT bool MCObjcObjectCreateWithAutoreleasedId(void *value, MCObjcObjectRef& r_obj);

/* The ObjcObjectGetId function returns the raw id value contained within
 * an ObjcObject. The retain count of the id remains unchanged. */
MC_DLLEXPORT void *MCObjcObjectGetId(MCObjcObjectRef obj);

/* The ObjcObjectGetRetainedId function returns the raw id value contained within
 * an ObjcObject. The id is retained before being returned. */
MC_DLLEXPORT void *MCObjcObjectGetRetainedId(MCObjcObjectRef obj);

/* The ObjcObjectGetAutoreleasedId function returns the raw id value contained within
 * an ObjcObject. The id is autoreleased before being returned. */
MC_DLLEXPORT void *MCObjcObjectGetAutoreleasedId(MCObjcObjectRef obj);

/* Create an ObjcObject containing an instance of the com_livecode_MCObjcFormalDelegate class,
 * mapping protocol methods to MCHandlerRefs, with
 * a context parameter. */
MC_DLLEXPORT bool MCObjcCreateDelegateWithContext(MCStringRef p_protocol_name, MCArrayRef p_handler_mapping, MCValueRef p_context, MCObjcObjectRef& r_object);

/* Create an ObjcObject containing an instance of  the com_livecode_MCObjcFormalDelegate class,
 * mapping protocol methods to MCHandlerRefs. */
MC_DLLEXPORT bool MCObjcCreateDelegate(MCStringRef p_protocol_name, MCArrayRef p_handler_mapping, MCObjcObjectRef& r_object);

/* Create an ObjcObject containing an instance of  the com_livecode_MCObjcInformalDelegate class,
 * mapping informal protocol methods specified as a list of foreign handler to MCHandlerRefs, with
 * a context parameter. */
MC_DLLEXPORT bool MCObjcCreateInformalDelegateWithContext(MCProperListRef p_foreign_handlers, MCArrayRef p_handler_mapping, MCValueRef p_context, MCObjcObjectRef& r_object);
    
/* Create an ObjcObject containing an instance of  the com_livecode_MCObjcInformalDelegate class,
 * mapping informal protocol methods specified as a list of foreign handler to MCHandlerRefs. */
MC_DLLEXPORT bool MCObjcCreateInformalDelegate(MCProperListRef p_foreign_handlers, MCArrayRef p_handler_mapping, MCObjcObjectRef& r_object);
    
////////////////////////////////////////////////////////////////////////////////

enum MCPickleFieldType
{
    kMCPickleFieldTypeNone,
    kMCPickleFieldTypeByte,
    kMCPickleFieldTypeUIndex,
    kMCPickleFieldTypeIntEnum,
    kMCPickleFieldTypeValueRef,
    kMCPickleFieldTypeStringRef,
    kMCPickleFieldTypeNameRef,
    kMCPickleFieldTypeTypeInfoRef,
    kMCPickleFieldTypeArrayOfByte,
    kMCPickleFieldTypeArrayOfUIndex,
    kMCPickleFieldTypeArrayOfValueRef,
    kMCPickleFieldTypeArrayOfNameRef,
    kMCPickleFieldTypeArrayOfTypeInfoRef,
    kMCPickleFieldTypeArrayOfRecord,
    kMCPickleFieldTypeArrayOfVariant,
};

struct MCPickleRecordFieldInfo
{
    // The kind (native type) of the field.
    MCPickleFieldType kind;
    // The name of the field.
    const char *tag;
    // The offset of the field within the record.
    size_t field_offset;
    // The offset of an associated field within the record
    // Variable sized arrays use this for the count field.
    size_t aux_field_offset;
    // Extra information about the field.
    // For callback fields, this is the function pointer.
    // For array of variant or record fields, this is the pickleinfo for the element.
    void *extra;
};

struct MCPickleRecordInfo
{
    size_t size;
    MCPickleRecordFieldInfo *fields;
};

struct MCPickleVariantCaseInfo
{
    int kind;
    MCPickleRecordInfo *record;
};

struct MCPickleVariantInfo
{
    size_t kind_offset;
    MCPickleVariantCaseInfo *cases;
};

#define MC_PICKLE_BEGIN_RECORD(Type) \
    struct __##Type##_PickleImp { \
        typedef Type __struct; \
        static MCPickleRecordFieldInfo __fields[]; \
        static MCPickleRecordInfo __info; \
    }; \
    MCPickleRecordInfo __##Type##_PickleImp::__info = { sizeof(Type), __##Type##_PickleImp::__fields }; \
    MCPickleRecordInfo *k##Type##PickleInfo = &__##Type##_PickleImp::__info; \
    MCPickleRecordFieldInfo __##Type##_PickleImp::__fields[] = {
#define MC_PICKLE_END_RECORD() \
	{ kMCPickleFieldTypeNone, nil, 0, 0, nil } \
    };

#define MC_PICKLE_BEGIN_VARIANT(Type, Kind) \
    struct __##Type##_PickleImp { \
        typedef Type __struct; \
        static MCPickleVariantCaseInfo __cases[]; \
        static MCPickleVariantInfo __info; \
    }; \
    MCPickleVariantInfo __##Type##_PickleImp::__info = { offsetof(Type, Kind), __##Type##_PickleImp::__cases }; \
    MCPickleVariantInfo *k##Type##PickleInfo = &__##Type##_PickleImp::__info; \
    MCPickleVariantCaseInfo __##Type##_PickleImp::__cases[] = {
#define MC_PICKLE_END_VARIANT() \
        { -1, nil } \
    };
#define MC_PICKLE_VARIANT_CASE(KindValue, Pickle) \
    { (int)KindValue, k##Pickle##PickleInfo },

#define MC_PICKLE_FIELD(FType, FField, FExtra) \
    { kMCPickleFieldType##FType, #FField, offsetof(__struct, FField), 0, (void *)FExtra },
#define MC_PICKLE_FIELD_AUX(FType, FField, FAuxField, FExtra) \
    { kMCPickleFieldType##FType, #FField, offsetof(__struct, FField), offsetof(__struct, FAuxField), (void *)FExtra },

#define MC_PICKLE_UINDEX(Field) MC_PICKLE_FIELD(UIndex, Field, 0)
#define MC_PICKLE_VALUEREF(Field) MC_PICKLE_FIELD(ValueRef, Field, 0)
#define MC_PICKLE_STRINGREF(Field) MC_PICKLE_FIELD(StringRef, Field, 0)
#define MC_PICKLE_NAMEREF(Field) MC_PICKLE_FIELD(NameRef, Field, 0)
#define MC_PICKLE_TYPEINFOREF(Field) MC_PICKLE_FIELD(TypeInfoRef, Field, 0)

#define MC_PICKLE_INTENUM(EType, EField) MC_PICKLE_FIELD(IntEnum, EField, k##EType##__Last)
#define MC_PICKLE_INTSET(SType, SField) MC_PICKLE_FIELD(IntEnum, SField, 0)

#define MC_PICKLE_ARRAY_OF_BYTE(Field, CountField) MC_PICKLE_FIELD_AUX(ArrayOfByte, Field, CountField, 0)
#define MC_PICKLE_ARRAY_OF_UINDEX(Field, CountField) MC_PICKLE_FIELD_AUX(ArrayOfUIndex, Field, CountField, 0)
#define MC_PICKLE_ARRAY_OF_VALUEREF(Field, CountField) MC_PICKLE_FIELD_AUX(ArrayOfValueRef, Field, CountField, 0)
#define MC_PICKLE_ARRAY_OF_NAMEREF(Field, CountField) MC_PICKLE_FIELD_AUX(ArrayOfNameRef, Field, CountField, 0)
#define MC_PICKLE_ARRAY_OF_TYPEINFOREF(Field, CountField) MC_PICKLE_FIELD_AUX(ArrayOfTypeInfoRef, Field, CountField, 0)
#define MC_PICKLE_ARRAY_OF_RECORD(Record, Field, CountField) MC_PICKLE_FIELD_AUX(ArrayOfRecord, Field, CountField, k##Record##PickleInfo)
#define MC_PICKLE_ARRAY_OF_VARIANT(Variant, Field, CountField) MC_PICKLE_FIELD_AUX(ArrayOfVariant, Field, CountField, k##Variant##PickleInfo)

// Read in the stream in the format conforming to the specified pickle info.
MC_DLLEXPORT bool MCPickleRead(MCStreamRef stream, MCPickleRecordInfo *info, void* r_record);

// Write the given record to the stream in the format conforming to the specified
// pickle info.
MC_DLLEXPORT bool MCPickleWrite(MCStreamRef stream, MCPickleRecordInfo *info, void* record);

// Release a record read in using MCPickleRead.
MC_DLLEXPORT void MCPickleRelease(MCPickleRecordInfo *info, void *record);

////////////////////////////////////////////////////////////////////////////////
//
//  TYPE CONVERSION
//

MC_DLLEXPORT bool MCTypeConvertStringToLongInteger(MCStringRef p_string, integer_t& r_converted);
MC_DLLEXPORT bool MCTypeConvertStringToReal(MCStringRef p_string, real64_t& r_converted, bool p_convert_octals = false);
MC_DLLEXPORT bool MCTypeConvertStringToBool(MCStringRef p_string, bool& r_converted);
MC_DLLEXPORT bool MCTypeConvertDataToReal(MCDataRef p_data, real64_t& r_converted, bool p_convert_octals = false);

}
    
#endif
