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

#ifndef __MC_FOUNDATION_LEGACY__
#define __MC_FOUNDATION_LEGACY__

#ifndef __MC_FOUNDATION__
#include "foundation.h"
#endif

////////////////////////////////////////////////////////////////////////////////

class MCString;

typedef MCStringOptions     MCCompareOptions;
#define kMCCompareExact     kMCStringOptionCompareExact
#define kMCCompareCaseless  kMCStringOptionCompareCaseless

////////////////////////////////////////////////////////////////////////////////

// This method is used by the old-style array saving code to convert a value to
// a string for serialization.
bool MCValueConvertToStringForSave(MCValueRef value, MCStringRef& r_string);

////////////////////////////////////////////////////////////////////////////////

bool MCStringCreateWithOldString(const MCString&, MCStringRef &r_string);
bool MCStringIsEqualToOldString(MCStringRef string, const MCString& oldstring, MCCompareOptions options);

// Attempt to interpret the given string as a base-10 integer. It returns false
// if conversion fails, otherwise 'r_integer' is set to the result.
bool MCStringToInteger(MCStringRef string, integer_t& r_integer);

inline bool MCStringToInt16(MCStringRef string, int16_t& r_integer)
{
	integer_t t_integer;
	if (!MCStringToInteger(string, t_integer))
		return false;
	if (t_integer < INT16_MIN || t_integer > INT16_MAX)
		return false;
	r_integer = (int16_t)t_integer;
	return true;
}

inline bool MCStringToUInt16(MCStringRef string, uint16_t& r_integer)
{
	integer_t t_integer;
	if (!MCStringToInteger(string, t_integer))
		return false;
	if (t_integer < (integer_t) UINT16_MIN || t_integer > (integer_t) UINT16_MAX)
		return false;
	r_integer = (uint16_t)t_integer;
	return true;
}

// Attempt to interpret the given string as a double. It returns false if conversion
// fails, otherwise 'r_real' is set to the result.
bool MCStringToDouble(MCStringRef string, double& r_double);

////////////////////////////////////////////////////////////////////////////////

MCString MCDataGetOldString(MCDataRef data);

////////////////////////////////////////////////////////////////////////////////

enum IO_stat
{
    IO_NORMAL,
    IO_NONE,
    IO_ERROR,
    IO_EOF,
    IO_TIMEOUT
};

struct MCSystemFileHandle;
typedef MCSystemFileHandle * IO_handle;
class MCObjectInputStream;
class MCObjectOutputStream;

// Returns true if the array is of sequence type (numeric keys starting at one
// with no holes).
bool MCArrayIsSequence(MCArrayRef array);

// SN-2015-06-15: [[ Bug 15457 ]] Returns true if the array a dense, numeric
//  sequence - but does not have to start with 1.
bool MCArrayIsNumericSequence(MCArrayRef self, index_t &r_start_index);

// Constructs a string containing the list of all keys in the array separated by
// the given delimiter.
bool MCArrayListKeys(MCArrayRef array, char delimiter, MCStringRef& r_list);

// Returns the size of the array on disk when saved to an objectstream. If
// 'nested_only' is set, it only includes keys of the array which are arrays.
uint32_t MCArrayMeasureForStreamLegacy(MCArrayRef array, bool nested_only);
// Returns true if the array contains any values which are arrays.
bool MCArrayIsNestedLegacy(MCArrayRef array);
// Loads keys from the IO_handle, adding them to the array.
IO_stat MCArrayLoadFromHandleLegacy(MCArrayRef array, IO_handle stream);
// Loads keys from the InputStream, adding them to the array.
IO_stat MCArrayLoadFromStreamLegacy(MCArrayRef array, MCObjectInputStream& stream);
// Saves keys to the IO_handle.
IO_stat MCArraySaveToHandleLegacy(MCArrayRef array, IO_handle stream);
// Saves keys to the outputstream. If 'nested_only' is true, only keys with
// array values are saved.
IO_stat MCArraySaveToStreamLegacy(MCArrayRef array, bool nested_only, MCObjectOutputStream& stream);

////////////////////////////////////////////////////////////////////////////////

// These are classed as 'legacy' at the moment as (ideally) they won't be
// necessary when unicodification has happened - but we shall see...
bool MCListAppendCString(MCListRef list, const char *cstring);
bool MCListAppendOldString(MCListRef list, const MCString& oldstring);
bool MCListAppendInteger(MCListRef list, integer_t value);
bool MCListAppendUnsignedInteger(MCListRef self, uinteger_t p_value);

////////////////////////////////////////////////////////////////////////////////

int UTF8ToUnicode(const char * lpSrcStr, int cchSrc, uint16_t * lpDestStr, int cchDest);
int UnicodeToUTF8(const uint16_t *lpSrcStr, int cchSrc, char *lpDestStr, int cchDest);

////////////////////////////////////////////////////////////////////////////////
bool MCCStringClone(const char *s, char*& r_s);
bool MCCStringCloneSubstring(const char *p_string, uint32_t p_length, char*& r_new_string);
bool MCCStringAppend(char *& x_string, const char *p_suffix);
bool MCCStringFormat(char*& r_string, const char *format, ...);
bool MCCStringFormatV(char*& r_string, const char *format, va_list args);
bool MCCStringAppendFormat(char*& x_string, const char *format, ...);

void MCCStringFree(char *s);

bool MCCStringEqual(const char *x, const char *y);
bool MCCStringEqualCaseless(const char *x, const char *y);
bool MCCStringEqualSubstring(const char *x, const char *y, index_t length);
bool MCCStringEqualSubstringCaseless(const char *x, const char *y, index_t length);

bool MCCStringBeginsWith(const char *string, const char *prefix);
bool MCCStringEndsWith(const char *string, const char *suffix);
bool MCCStringBeginsWithCaseless(const char *string, const char *prefix);
bool MCCStringEndsWithCaseless(const char *string, const char *suffix);

uint32_t MCCStringLength(const char *s);

bool MCCStringSplit(const char *string, char p_separator, char**& r_elements, uint32_t& r_element_count);
bool MCCStringCombine(const char * const *p_elements, uint32_t p_element_count, char p_separator, char*& r_string);

void MCCStringArrayFree(char **cstrings, uint32_t count);

bool MCCStringToNative(const char *string, char*& r_new_string);
bool MCCStringFromNative(const char *string, char*& r_new_string);

bool MCCStringToUnicode(const char *string, unichar_t*& r_unicode_string);
bool MCCStringFromUnicode(const unichar_t* unicode_string, char*& r_string);

bool MCCStringFromUnicodeSubstring(const unichar_t *p_chars, uint32_t p_char_count, char*& r_string);
bool MCCStringFromNativeSubstring(const char *p_string, uint32_t p_length, char*& r_cstring);

bool MCCStringTokenize(const char *string, char**& r_elements, uint32_t& r_element_count);

bool MCCStringFirstIndexOf(const char *p_string, char p_search, uint32_t &r_index);
bool MCCStringFirstIndexOf(const char *p_string, const char *p_search, uint32_t &r_index);
bool MCCStringLastIndexOf(const char *p_string, char p_search, uint32_t &r_index);
bool MCCStringLastIndexOf(const char *p_string, const char *p_search, uint32_t &r_index);
bool MCCStringContains(const char *p_haystack, const char *p_needle);

bool MCCStringIsEmpty(const char *);
bool MCCStringIsInteger(const char *);

////////////////////////////////////////////////////////////////////////////////

enum MCFileSystemEntryType
{
	kMCFileSystemEntryFile,
	kMCFileSystemEntryFolder,
	kMCFileSystemEntryLink,
	kMCFileSystemEntryPackage
};

struct MCFileSystemEntry
{
	MCFileSystemEntryType type;
	MCStringRef filename;
};

typedef bool (*MCFileSystemListCallback)(void *context, const MCFileSystemEntry& entry);

bool MCFileSystemListEntries(const char *folder, uint32_t options, MCFileSystemListCallback callback, void *p_context);

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
