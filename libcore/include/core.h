////////////////////////////////////////////////////////////////////////////////
//
//  Private Header File:
//    core.h
//
//  Description:
//    This file contains core methods abstracting away from the C runtime-
//    library. It is not yet ready for general use throughout the engine and
//    should currently only be used by the capsule and deploy modules.
//
//  Changes:
//    2009-07-04 MW Created.
//
////////////////////////////////////////////////////////////////////////////////

#ifndef __MC_CORE__
#define __MC_CORE__

////////////////////////////////////////////////////////////////////////////////

#ifndef _STDARG_H
#include <stdarg.h>
#endif

////////////////////////////////////////////////////////////////////////////////

typedef unsigned char uint8_t;
typedef signed char int8_t;
typedef unsigned short uint16_t;
typedef signed short int16_t;
typedef unsigned int uint32_t;
typedef signed int int32_t;

// MDW 2013-04.15: only typedef if necessary
#if !defined(uint64_t)
	#ifdef __LP64__
		typedef unsigned long int uint64_t;
	#else
		typedef unsigned long long int uint64_t;
	#endif
#endif

#if !defined(int64_t)
	#ifdef __LP64__
		typedef long int int64_t;
	#else
		typedef long long int int64_t;
	#endif
#endif

typedef uint32_t uindex_t;
typedef int32_t index_t;
typedef uint32_t hash_t;
typedef int32_t compare_t;

#if (defined(_MACOSX) || defined(TARGET_SUBPLATFORM_IPHONE)) && !defined(_SIZE_T)
	typedef long unsigned int size_t;
#endif

#if defined(_LINUX) && !defined(_SIZE_T)
	// MDW-2013-04-15: [[ x64 ]] make 64-bit safe
	#ifdef __LP64__
		typedef long unsigned int size_t;
	#else
		typedef unsigned int size_t;
	#endif
#endif

#ifndef _UINTPTR_T
	#define _UINTPTR_T
	// MDW-2013-04-15: [[ x64 ]] make 64-bit safe
	#ifdef __LP64__
		typedef uint64_t uintptr_t;
	#else
		typedef uint32_t uintptr_t;
	#endif
#endif

#ifndef _INTPTR_T
	#define _INTPTR_T
	// MDW-2013-04-15: [[ x64 ]] make 64-bit safe
	#ifdef __LP64__
		typedef int64_t intptr_t;
	#else
		typedef int32_t intptr_t;
	#endif
#endif

typedef char char_t;

#if defined(_WIN32) || defined(_WINCE)
typedef wchar_t unichar_t;
typedef wchar_t *BSTR;
#else
typedef uint16_t unichar_t;
#endif

#if defined(_MACOSX) || defined(TARGET_SUBPLATFORM_IPHONE)
typedef const struct __CFString * CFStringRef;
typedef const struct __CFData * CFDataRef;
#endif

#ifndef nil
#define nil 0
#endif

#if defined(_MACOSX) && defined(__LP64__)
#define _MACOSX_NOCARBON
#endif

#if defined(_WINDOWS)
typedef char *va_list;
#elif defined(_MACOSX)
typedef __builtin_va_list va_list;
#elif defined(_LINUX)
typedef __builtin_va_list va_list;
#endif

#if defined(_MOBILE) && defined(TARGET_SUBPLATFORM_ANDROID)
typedef uint32_t size_t;
#endif

////////////////////////////////////////////////////////////////////////////////

#ifdef _DEBUG
extern void __MCAssert(const char *file, uint32_t line, const char *message);
#define MCAssert(m_expr) (void)( (!!(m_expr)) || (__MCAssert(__FILE__, __LINE__, #m_expr), 0) )

extern void __MCLog(const char *file, uint32_t line, const char *format, ...);
#define MCLog(m_format, ...) __MCLog(__FILE__, __LINE__, m_format, __VA_ARGS__)

extern void __MCLogWithTrace(const char *file, uint32_t line, const char *format, ...);
#define MCLogWithTrace(m_format, ...) __MCLogWithTrace(__FILE__, __LINE__, m_format, __VA_ARGS__)

#else
#define MCAssert(expr)
#define MCLog(m_format, ...) 
#define MCLogWithTrace(m_format, ...)
#endif

////////////////////////////////////////////////////////////////////////////////

// IM. 2011-01-18 blocked out currently unused error codes which conflict with libexternal
#if 0
enum
{
	kMCErrorNone,
	kMCErrorNoMemory
};

enum
{
	kMCErrorGroupCore = 0,
	kMCErrorGroupPlugin = 1024
};
#endif

bool MCThrow(uint32_t error);
bool MCRethrow(uint32_t new_error);

////////////////////////////////////////////////////////////////////////////////

// This method returns an unitialized block of memory of the given size in
// block. An error is raised if allocation fails.
bool MCMemoryAllocate(uindex_t size, void*& r_block);

// This method allocates a block of memory of size block_size and copies the
// data from block into it.
bool MCMemoryAllocateCopy(const void *p_block, uindex_t p_block_size, void*& r_block);

// This method reallocates a block of memory allocated using MCMemoryAllocate.
// Any new space allocated is uninitialized. The new pointer to the block is
// returned. Note that the block may move regardless of new size. If the input
// block is nil, it is treated as an allocate call.
bool MCMemoryReallocate(void *block, uindex_t new_size, void*& r_new_block);

// This method deallocates a block of memory allocated using MCMemoryAllocate,
// or subsequently reallocated using MCMemoryReallocate. The block passed in
// may be nil.
void MCMemoryDeallocate(void *block);

//////////

template<typename T> bool MCMemoryAllocate(uindex_t p_size, T*& r_block)
{
	void *t_block;
	if (MCMemoryAllocate(p_size, t_block))
	{
		r_block = static_cast<T *>(t_block);
		return true;
	}
	return false;
}

template<typename T> bool MCMemoryAllocateCopy(const T *p_block, uindex_t p_block_size, T*& r_block)
{
	void *t_block;
	if (MCMemoryAllocateCopy(p_block, p_block_size, t_block))
	{
		r_block = static_cast<T *>(t_block);
		return true;
	}
	return false;
}

template<typename T> bool MCMemoryReallocate(T *p_block, uindex_t p_new_size, T*& r_new_block)
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

// This method allocates a fixed size record, and initializes its bytes to
// zero. An error is raised if allocation fails. Note that blocks allocated
// using this method are not necessarily interchangeable with the other
// memory allocation functions.
bool MCMemoryNew(uindex_t size, void*& r_record);

// This method deletes a fixed size record that was allocated with MCMemoryNew.
void MCMemoryDelete(void *p_record);

//////////

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

// This method provides type-safe construction of an object.
template<typename T> bool MCMemoryNew(T*& r_record)
{
	void *t_record;
	if (MCMemoryNew(sizeof(T), t_record))
	{
		// Notice here we use the 'placement-new' operator we defined above to
		// do the type conversion. This ensures any type-specific initialization
		// is done.
		r_record = new(t_record, true) T;

		return true;
	}
	return false;
}

template<typename T> void MCMemoryDelete(T* p_record)
{
	// Notice here we invoke the destructor for the type directly. This ensures
	// any type-specific finalization is done.
	p_record -> ~T();

	MCMemoryDelete(static_cast<void *>(p_record));
}

#ifdef _DEBUG
#ifdef redef_new
#undef redef_new
#define new new(__FILE__, __LINE__)
#endif
#endif

////////////////////////////////////////////////////////////////////////////////

// This method allocates a resizable array of elements of the given size. All
// the bytes in the returned array are set to zero. An error is raised if
// allocation failes.
bool MCMemoryNewArray(uindex_t p_count, uindex_t p_size, void*& r_array);

// This method resizes an array, initializing any new bytes allocated to 0.
bool MCMemoryResizeArray(uindex_t p_new_count, uindex_t p_size, void*& x_array, uindex_t& x_count);

// This method deallocates an array that was allocated or reallocated using the
// previous two methods.
void MCMemoryDeleteArray(void *p_array);

//////////

template<typename T> inline bool MCMemoryNewArray(uindex_t p_count, T*& r_array)
{
	void *t_array;
	if (MCMemoryNewArray(p_count, sizeof(T), t_array))
		return r_array = static_cast<T *>(t_array), true;
	return false;
}

template<typename T> inline bool MCMemoryResizeArray(uindex_t p_new_count, T*& r_array, uindex_t& x_count)
{
	void *t_array;
	t_array = r_array;
	if (MCMemoryResizeArray(p_new_count, sizeof(T), t_array, x_count))
		return r_array = static_cast<T *>(t_array), true;
	return false;
}

////////////////////////////////////////////////////////////////////////////////

void MCMemoryClear(void *dst, uindex_t size);
void MCMemoryCopy(void *dst, const void *src, uindex_t size);
void MCMemoryMove(void *dst, const void *src, uindex_t size);
bool MCMemoryEqual(const void *left, const void *right, uindex_t size);
compare_t MCMemoryCompare(const void *left, const void *right, uindex_t size);
hash_t MCMemoryHash(const void *src, uindex_t size);

////////////////////////////////////////////////////////////////////////////////

// Split a string into an array of 'tokens', where a token is separated by space
// characters. If a token starts with a double-quote, it continues until a
// terminating quote.
//
// The array returned should be freed with MCMemoryDeleteArray, and the strings
// it contains, freed with MCCStringFree.
//
bool MCCStringTokenize(const char *string, char**& r_elements, uint32_t& r_element_count);

// Split a string into an array of substrings separated by the specified character.
// substrings are all characters between separators not including separators,
// and will create empty strings where there are neighbouring separators.
//
// The array returned should be freed with MCMemoryDeleteArray, and the strings
// it contains, freed with MCCStringFree.
//
bool MCCStringSplit(const char *string, char p_separator, char**& r_elements, uint32_t& r_element_count);
bool MCCStringCombine(const char * const *p_elements, uint32_t p_element_count, char p_separator, char*& r_string);

bool MCCStringFormat(char*& r_string, const char *format, ...);
bool MCCStringFormatV(char*& r_string, const char *format, va_list args);
bool MCCStringAppendFormat(char*& x_string, const char *format, ...);
bool MCCStringAppendFormatV(char*& r_string, const char *format, va_list args);
bool MCCStringClone(const char *s, char*& r_s);
bool MCCStringCloneSubstring(const char *p_string, uint32_t p_length, char*& r_new_string);
bool MCCStringAppend(char*& x_string, const char *string);
void MCCStringFree(char *s);

bool MCCStringArrayClone(const char *const *strings, uint32_t count, char**& r_new_strings);
void MCCStringArrayFree(char **cstrings, uint32_t count);

bool MCCStringIsEmpty(const char *s);
bool MCCStringIsInteger(const char *s);

uint32_t MCCStringLength(const char *s);

bool MCCStringToUnicode(const char *string, unichar_t*& r_unicode_string);

bool MCCStringFromUnicode(const unichar_t* unicode_string, char*& r_string);
bool MCCStringFromUnicodeSubstring(const unichar_t* unicode_string, uint32_t length, char*& r_string);

bool MCCStringToNative(const char *string, char*& r_string);
bool MCCStringFromNative(const char *string, char*& r_string);
bool MCCStringFromNativeSubstring(const char *string, uint32_t length, char*& r_string);

#if defined(_MACOSX) || defined(TARGET_SUBPLATFORM_IPHONE)
bool MCCStringToCFString(const char *string, CFStringRef& r_cfstring);
bool MCCStringFromCFString(CFStringRef cfstring, char*& r_cstring);
#endif

#ifdef _WINDOWS
bool MCCStringToBSTR(const char *string, BSTR& r_bstr);
bool MCCStringFromBSTR(BSTR bstr, char*& r_cstring);
#endif

compare_t MCCStringCompare(const char *x, const char *y);

bool MCCStringEqual(const char *x, const char *y);
bool MCCStringEqualCaseless(const char *x, const char *y);
bool MCCStringEqualSubstring(const char *x, const char *y, index_t length);
bool MCCStringEqualSubstringCaseless(const char *x, const char *y, index_t length);

bool MCCStringContains(const char *string, const char *needle);
bool MCCStringContainsCaseless(const char *string, const char *needle);

bool MCCStringBeginsWith(const char *string, const char *prefix);
bool MCCStringBeginsWithCaseless(const char *string, const char *prefix);
bool MCCStringEndsWith(const char *string, const char *suffix);
bool MCCStringEndsWithCaseless(const char *string, const char *suffix);

bool MCCStringToInteger(const char *string, int32_t& r_value);
bool MCCStringToCardinal(const char *string, uint32_t& r_value);

bool MCCStringFirstIndexOf(const char *p_string, char p_search, uint32_t &r_index);
bool MCCStringFirstIndexOf(const char *p_string, const char *p_search, uint32_t &r_index);
bool MCCStringLastIndexOf(const char *p_string, char p_search, uint32_t &r_index);
bool MCCStringLastIndexOf(const char *p_string, const char *p_search, uint32_t &r_index);

////////////////////////////////////////////////////////////////////////////////

// A simple class that handles auto-deletion of a C-string
class MCAutoCString
{
public:
	MCAutoCString(void)
	{
		m_cstring = nil;
	}

	~MCAutoCString(void)
	{
		MCCStringFree(m_cstring);
	}

	// Take a copy and hold a pointer to the given cstring
	bool AssignCString(const char *p_cstring)
	{
		if (m_cstring != nil)
			MCCStringFree(m_cstring);
		return MCCStringClone(p_cstring, m_cstring);
	}

	// Convert the given native string to a cstring and hold the pointer
	bool AssignNative(const char *p_native)
	{
		if (m_cstring != nil)
			MCCStringFree(m_cstring);
		return MCCStringFromNative(p_native, m_cstring);
	}

	// Convert the given unicode string to a native cstring and hold the pointer
	bool AssignUnicode(const unichar_t *p_unicode)
	{
		if (m_cstring != nil)
			MCCStringFree(m_cstring);
		return MCCStringFromUnicode(p_unicode, m_cstring);
	}

	// Borrow the held pointer
	operator const char *(void) const
	{
		return m_cstring;
	}

private:
	MCAutoCString(const MCAutoCString&) {}

	char *m_cstring;
};

////////////////////////////////////////////////////////////////////////////////

struct MCBinaryEncoder;

bool MCBinaryEncoderCreate(MCBinaryEncoder*& r_encoder);
void MCBinaryEncoderDestroy(MCBinaryEncoder *encoder);

void MCBinaryEncoderBorrow(MCBinaryEncoder *encoder, void*& r_buffer, uint32_t& r_buffer_length);

bool MCBinaryEncoderWriteBytes(MCBinaryEncoder *encoder, const void *data, uint32_t length);
bool MCBinaryEncoderWriteInt32(MCBinaryEncoder *encoder, int32_t p_value);
bool MCBinaryEncoderWriteUInt32(MCBinaryEncoder *encoder, uint32_t p_value);
bool MCBinaryEncoderWriteCBlob(MCBinaryEncoder *encoder, const void *data, uint32_t length);
bool MCBinaryEncoderWriteCString(MCBinaryEncoder *encoder, const char *cstring);

#ifdef _MACOSX
bool MCBinaryEncoderWriteCFData(MCBinaryEncoder *encoder, CFDataRef cfdata);
bool MCBinaryEncoderWriteCFString(MCBinaryEncoder *encoder, CFStringRef cfstring);
#endif

/////////

struct MCBinaryDecoder;

bool MCBinaryDecoderCreate(const void *p_buffer, uint32_t p_length, MCBinaryDecoder*& r_decoder);
void MCBinaryDecoderDestroy(MCBinaryDecoder *p_decoder);

bool MCBinaryDecoderReadBytes(MCBinaryDecoder *decoder, void *data, uint32_t count);
bool MCBinaryDecoderReadInt32(MCBinaryDecoder *decoder, int32_t& r_value);
bool MCBinaryDecoderReadUInt32(MCBinaryDecoder *decoder, uint32_t& r_value);
bool MCBinaryDecoderReadCBlob(MCBinaryDecoder *decoder, void*& r_data, uint32_t& r_length);
bool MCBinaryDecoderReadCString(MCBinaryDecoder *self, char *&r_cstring);

#ifdef _MACOSX
bool MCBinaryDecoderReadCFData(MCBinaryDecoder *decoder, CFDataRef& r_value);
bool MCBinaryDecoderReadCFString(MCBinaryDecoder *decoder, CFStringRef& r_value);
#endif

////////////////////////////////////////////////////////////////////////////////

inline uint32_t MCMin(uint32_t a, uint32_t b) { return a < b ? a : b; }
inline uint32_t MCMax(uint32_t a, uint32_t b) { return a > b ? a : b; }
inline int32_t MCMin(int32_t a, int32_t b) { return a < b ? a : b; }
inline int32_t MCMax(int32_t a, int32_t b) { return a > b ? a : b; }
inline int64_t MCMin(int64_t a, int64_t b) { return a < b ? a : b; }
inline int64_t MCMax(int64_t a, int64_t b) { return a > b ? a : b; }
inline int64_t MCMin(uint64_t a, uint64_t b) { return a < b ? a : b; }
inline int64_t MCMax(uint64_t a, uint64_t b) { return a > b ? a : b; }
inline double MCMin(double a, double b) { return a < b ? a : b; }
inline double MCMax(double a, double b) { return a > b ? a : b; }
inline float MCMin(float a, float b) { return a < b ? a : b; }
inline float MCMax(float a, float b) { return a > b ? a : b; }
inline uint32_t MCAbs(int32_t a) { return a < 0 ? -a : a; }
inline uint64_t MCAbs(int64_t a) { return a < 0 ? -a : a; }
inline compare_t MCSgn(int32_t a) { return a < 0 ? -1 : (a > 0 ? 1 : 0); }
inline compare_t MCSgn(int64_t a) { return a < 0 ? -1 : (a > 0 ? 1 : 0); }
inline compare_t MCCompare(int32_t a, int32_t b) { return a < b ? -1 : (a > b ? 1 : 0); }
inline compare_t MCCompare(uint32_t a, uint32_t b) { return a < b ? -1 : (a > b ? 1 : 0); }
inline compare_t MCCompare(int64_t a, int64_t b) { return a < b ? -1 : (a > b ? 1 : 0); }
inline compare_t MCCompare(uint64_t a, uint64_t b) { return a < b ? -1 : (a > b ? 1 : 0); }

inline bool MCIsPowerOfTwo(uint32_t x) { return (x & (x - 1)) == 0; }

inline float MCClamp(float value, float min, float max) {return MCMax(min, MCMin(max, value));}

////////////////////////////////////////////////////////////////////////////////

inline uint32_t MCByteSwappedToHost32(uint32_t x)
{
#ifdef __LITTLE_ENDIAN__
	return ((x >> 24) | ((x >> 8) & 0xff00) | ((x & 0xff00) << 8) | (x << 24));
#else
	return x;
#endif
}

inline uint32_t MCByteSwappedFromHost32(uint32_t x)
{
#ifdef __LITTLE_ENDIAN__
	return ((x >> 24) | ((x >> 8) & 0xff00) | ((x & 0xff00) << 8) | (x << 24));
#else
	return x;
#endif
}

inline uint32_t MCSwapInt32HostToNetwork(uint32_t i)
{
#ifdef __LITTLE_ENDIAN__
	return ((i & 0xff) << 24) | ((i & 0xff00) << 8) | ((i & 0xff0000) >> 8) | ((i & 0xff000000) >> 24);
#else
	return i;
#endif
}

inline uint32_t MCSwapInt32NetworkToHost(uint32_t i)
{
#ifdef __LITTLE_ENDIAN__
	return ((i & 0xff) << 24) | ((i & 0xff00) << 8) | ((i & 0xff0000) >> 8) | ((i & 0xff000000) >> 24);
#else
	return i;
#endif
}

inline uint16_t MCSwapInt16HostToNetwork(uint16_t i)
{
#ifdef __LITTLE_ENDIAN__
	return ((i & 0xff) << 8) | ((i & 0xff00) >> 8);
#else
	return i;
#endif
}

inline uint16_t MCSwapInt16NetworkToHost(uint16_t i)
{
#ifdef __LITTLE_ENDIAN__
	return ((i & 0xff) << 8) | ((i & 0xff00) >> 8);
#else
	return i;
#endif
}

////////////////////////////////////////////////////////////////////////////////

void MCListPushBack(void *& x_list, void *element);
void *MCListPopBack(void *&x_list);
void MCListPushFront(void *& x_list, void *element);
void *MCListPopFront(void *&x_list);

void MCListRemove(void *& x_list, void *element);

//////////

template<typename T> inline void MCListPushBack(T*& x_list, T *p_element)
{
	void *t_list;
	t_list = x_list;
	MCListPushBack(t_list, p_element);
	x_list = static_cast<T *>(t_list);
}

template<typename T> inline T *MCListPopBack(T*& x_list)
{
	void *t_list, *t_element;
	t_list = x_list;
	t_element = MCListPopBack(t_list);
	x_list = static_cast<T *>(t_list);
	return static_cast<T *>(t_element);
}

template<typename T> inline void MCListPushFront(T*& x_list, T *p_element)
{
	void *t_list;
	t_list = x_list;
	MCListPushFront(t_list, p_element);
	x_list = static_cast<T *>(t_list);
}

template<typename T> inline T *MCListPopFront(T*& x_list)
{
	void *t_list, *t_element;
	t_list = x_list;
	t_element = MCListPopFront(t_list);
	x_list = static_cast<T *>(t_list);
	return static_cast<T *>(t_element);
}

template<typename T> inline void MCListRemove(T*& x_list, T *p_element)
{
	void *t_list;
	t_list = x_list;
	MCListRemove(t_list, p_element);
	x_list = static_cast<T *>(t_list);
}

////////////////////////////////////////////////////////////////////////////////

#endif
