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

#ifndef __MC_FOUNDATION_PRIVATE__
#define __MC_FOUNDATION_PRIVATE__

////////////////////////////////////////////////////////////////////////////////

#ifdef __LINUX__
extern const char *__MCSysCharset;
#endif

////////////////////////////////////////////////////////////////////////////////

// Flags word:
//   31...28 : typecode
//
//   0 : is_real (number)
//     : is_unicode (string)
//   1 : is_mutable (string)
//

enum
{
	// If set, then this value is in the unique table.
	kMCValueFlagIsInterred = 1 << 27,
};

struct __MCValue
{
	uint32_t references;
	uint32_t flags;
};

//////////

struct __MCNull: public __MCValue
{
};

//////////

struct __MCBoolean: public __MCValue
{
};

//////////

enum
{
	kMCNumberFlagIsReal = 1 << 0,
};

struct __MCNumber: public __MCValue
{
	union
	{
		integer_t integer;
		real64_t real;
	};
};

//////////

enum
{
	// If set then the string is indirect (i.e. contents is within another
	// immutable string).
	kMCStringFlagIsIndirect = 1 << 0,
	// If set then the string is mutable.
	kMCStringFlagIsMutable = 1 << 1,
	// If set then the string is not native
	kMCStringFlagIsNotNative = 1 << 2,
    // If set, the string contains no non-BMP characters
    kMCStringFlagIsSimple = 1 << 3,
    // If set, the string has been checked for simplicity
    kMCStringFlagIsChecked = 1 << 4,
    // If set, the string has NO combining chars
    kMCStringFlagIsUncombined = 1 << 5,
    // If set, the string has been converted to a number
    kMCStringFlagHasNumber = 1 << 6,
    // If set, indicates that the string can be losslessly nativized
    kMCStringFlagCanBeNative = 1 << 7
};

enum
{
    kMCStringFlagSetFalse,
    kMCStringFlagSetTrue,
    kMCStringFlagNoChange
};


typedef unichar_t strchar_t;
// Does not yet use foundation unicode
#define MCStrCharMapToNative(x) MCUnicodeCharMapToNativeLossy(x)
#define MCStrCharMapFromNative(x) MCUnicodeCharMapFromNative(x)
#define MCStrCharMapToUnicode(x) (x)
#define MCStrCharsMapFromUnicode(x, y, z, w) (MCMemoryCopy(z, x, y * sizeof(strchar_t)), w = y)
#define MCStrCharsMapFromNative(x, y, z) MCUnicodeCharsMapFromNative(y, z, x)

// Modified to use foundation-unicode
#define MCStrCharFoldSimple(x) MCUnicodeGetCharacterProperty(x, kMCUnicodePropertySimpleCaseFolding)
#define MCStrCharsEqualExact(x, y, z, w) (MCUnicodeCompare(x, y, z, w, kMCUnicodeCompareOptionExact) == 0)
#define MCStrCharsEqualCaseless(x, y, z, w) (MCUnicodeCompare(x, y, z, w, kMCUnicodeCompareOptionCaseless) == 0)
#define MCStrCharsEqualNonliteral(x, y, z, w) (MCUnicodeCompare(x, y, z, w, kMCUnicodeCompareOptionNormalised) == 0)
#define MCStrCharsEqualFolded(x, y, z, w) (MCUnicodeCompare(x, y, z, w, kMCUnicodeCompareOptionFolded) == 0)
#define MCStrCharsCompareExact(x, y, z, w) MCUnicodeCompare(x, y, z, w, kMCUnicodeCompareOptionExact)
#define MCStrCharsCompareCaseless(x, y, z, w) MCUnicodeCompare(x, y, z, w, kMCUnicodeCompareOptionCaseless)
#define MCStrCharsCompareNonliteral(x, y, z, w) MCUnicodeCompare(x, y, z, w, kMCUnicodeCompareOptionNormalised)
#define MCStrCharsCompareFolded(x, y, z, w) MCUnicodeCompare(x, y, z, w, kMCUnicodeCompareOptionFolded)
#define MCStrCharsBeginsWithExact(x, y, z, w) MCUnicodeBeginsWith(x, y, z, w, kMCUnicodeCompareOptionExact)
#define MCStrCharsBeginsWithCaseless(x, y, z, w) MCUnicodeBeginsWith(x, y, z, w, kMCUnicodeCompareOptionCaseless)
#define MCStrCharsBeginsWithNonliteral(x, y, z, w) MCUnicodeBeginsWith(x, y, z, w, kMCUnicodeCompareOptionNormalised)
#define MCStrCharsBeginsWithFolded(x, y, z, w) MCUnicodeBeginsWith(x, y, z, w, kMCUnicodeCompareOptionFolded)
#define MCStrCharsEndsWithExact(x, y, z, w) MCUnicodeEndsWith(x, y, z, w, kMCUnicodeCompareOptionExact)
#define MCStrCharsEndsWithCaseless(x, y, z, w) MCUnicodeEndsWith(x, y, z, w, kMCUnicodeCompareOptionCaseless)
#define MCStrCharsEndsWithNonliteral(x, y, z, w) MCUnicodeEndsWith(x, y, z, w, kMCUnicodeCompareOptionNormalised)
#define MCStrCharsEndsWithFolded(x, y, z, w) MCUnicodeEndsWith(x, y, z, w, kMCUnicodeCompareOptionFolded)
#define MCStrCharsContainsExact(x, y, z, w) MCUnicodeContains(x, y, z, w, kMCUnicodeCompareOptionExact)
#define MCStrCharsContainsCaseless(x, y, z, w) MCUnicodeContains(x, y, z, w, kMCUnicodeCompareOptionCaseless)
#define MCStrCharsContainsNonliteral(x, y, z, w) MCUnicodeContains(x, y, z, w, kMCUnicodeCompareOptionNormalised)
#define MCStrCharsContainsFolded(x, y, z, w) MCUnicodeContains(x, y, z, w, kMCUnicodeCompareOptionFolded)
#define MCStrCharsFirstIndexOfExact(x, y, z, w, r) MCUnicodeFirstIndexOf(x, y, z, w, kMCUnicodeCompareOptionExact, r)
#define MCStrCharsFirstIndexOfCaseless(x, y, z, w, r) MCUnicodeFirstIndexOf(x, y, z, w, kMCUnicodeCompareOptionCaseless, r)
#define MCStrCharsFirstIndexOfNonliteral(x, y, z, w, r) MCUnicodeFirstIndexOf(x, y, z, w, kMCUnicodeCompareOptionNormalised, r)
#define MCStrCharsFirstIndexOfFolded(x, y, z, w, r) MCUnicodeFirstIndexOf(x, y, z, w, kMCUnicodeCompareOptionFolded, r)
#define MCStrCharsLastIndexOfExact(x, y, z, w, r) MCUnicodeLastIndexOf(x, y, z, w, kMCUnicodeCompareOptionExact, r)
#define MCStrCharsLastIndexOfCaseless(x, y, z, w, r) MCUnicodeLastIndexOf(x, y, z, w, kMCUnicodeCompareOptionCaseless, r)
#define MCStrCharsLastIndexOfNonliteral(x, y, z, w, r) MCUnicodeLastIndexOf(x, y, z, w, kMCUnicodeCompareOptionNormalised, r)
#define MCStrCharsLastIndexOfFolded(x, y, z, w, r) MCUnicodeLastIndexOf(x, y, z, w, kMCUnicodeCompareOptionFolded, r)
#define MCStrCharsFirstIndexOfCharExact(x, y, z, r) MCUnicodeFirstIndexOfChar(x, y, z, kMCUnicodeCompareOptionExact, r)
#define MCStrCharsFirstIndexOfCharCaseless(x, y, z, r) MCUnicodeFirstIndexOfChar(x, y, z, kMCUnicodeCompareOptionCaseless, r)
#define MCStrCharsFirstIndexOfCharNonliteral(x, y, z, r) MCUnicodeFirstIndexOfChar(x, y, z, kMCUnicodeCompareOptionNormalised, r)
#define MCStrCharsFirstIndexOfCharFolded(x, y, z, r) MCUnicodeFirstIndexOfChar(x, y, z, kMCUnicodeCompareOptionFolded, r)
#define MCStrCharsLastIndexOfCharExact(x, y, z, r) MCUnicodeLastIndexOfChar(x, y, z, kMCUnicodeCompareOptionExact, r)
#define MCStrCharsLastIndexOfCharCaseless(x, y, z, r) MCUnicodeLastIndexOfChar(x, y, z, kMCUnicodeCompareOptionCaseless, r)
#define MCStrCharsLastIndexOfCharNonliteral(x, y, z, r) MCUnicodeLastIndexOfChar(x, y, z, kMCUnicodeCompareOptionNormalised, r)
#define MCStrCharsLastIndexOfCharFolded(x, y, z, r) MCUnicodeLastIndexOfChar(x, y, z, kMCUnicodeCompareOptionFolded, r)
#define MCStrCharsSharedPrefixExact(x, y, z, w, r, s) MCUnicodeSharedPrefix(x, y, z, w, kMCUnicodeCompareOptionExact, r, s)
#define MCStrCharsSharedPrefixCaseless(x, y, z, w, r, s) MCUnicodeSharedPrefix(x, y, z, w, kMCUnicodeCompareOptionCaseless, r, s)
#define MCStrCharsSharedPrefixNonliteral(x, y, z, w, r, s) MCUnicodeSharedPrefix(x, y, z, w, kMCUnicodeCompareOptionNormalised, r, s)
#define MCStrCharsSharedPrefixFolded(x, y, z, w, r, s) MCUnicodeSharedPrefix(x, y, z, w, kMCUnicodeCompareOptionFolded, r, s)
#define MCStrCharsSharedSuffixExact(x, y, z, w, r, s) MCUnicodeSharedSuffix(x, y, z, w, kMCUnicodeCompareOptionExact, r, s)
#define MCStrCharsSharedSuffixCaseless(x, y, z, w, r, s) MCUnicodeSharedSuffix(x, y, z, w, kMCUnicodeCompareOptionCaseless, r, s)
#define MCStrCharsSharedSuffixNonliteral(x, y, z, w, r, s) MCUnicodeSharedSuffix(x, y, z, w, kMCUnicodeCompareOptionNormalised, r, s)
#define MCStrCharsSharedSuffixFolded(x, y, z, w, r, s) MCUnicodeSharedSuffix(x, y, z, w, kMCUnicodeCompareOptionFolded, r, s)
#define MCStrCharsFindExact(x, y, z, w, r) MCUnicodeFind(x, y, z, w, kMCUnicodeCompareOptionExact, r)
#define MCStrCharsFindCaseless(x, y, z, w, r) MCUnicodeFind(x, y, z, w, kMCUnicodeCompareOptionCaseless, r)
#define MCStrCharsFindNonliteral(x, y, z, w, r) MCUnicodeFind(x, y, z, w, kMCUnicodeCompareOptionNormalised, r)
#define MCStrCharsFindFolded(x, y, z, w, r) MCUnicodeFind(x, y, z, w, kMCUnicodeCompareOptionFolded, r)
#define MCStrCharsHashExact(x, y) MCUnicodeHash(x, y, kMCUnicodeCompareOptionExact)
#define MCStrCharsHashCaseless(x, y) MCUnicodeHash(x, y, kMCUnicodeCompareOptionCaseless)
#define MCStrCharsHashNonliteral(x, y) MCUnicodeHash(x, y, kMCUnicodeCompareOptionNormalised)
#define MCStrCharsHashFolded(x, y) MCUnicodeHash(x, y, kMCUnicodeCompareOptionFolded)

struct __MCString: public __MCValue
{
    union
    {
        MCStringRef string;
        struct
        {
            uindex_t char_count;
            union
            {
                unichar_t *chars;
                char_t *native_chars;
            };
            uindex_t capacity;
        };
    };
};

//////////

enum
{
    // The data are mutable
    kMCDataFlagIsMutable = 1 << 0,
    // The data are indirect (i.e. contents is within another immutable data ref).
    kMCDataFlagIsIndirect = 1 << 1,
};

// AL-2014-11-12: [[ Bug 13987 ]] Implement copy on write for MCDataRef
struct __MCData: public __MCValue
{
    union
    {
        MCDataRef contents;
        struct
        {
            uindex_t byte_count;
            byte_t *bytes;
            uindex_t capacity;
        };
    };
};

//////////

struct __MCName: public __MCValue
{
	__MCName *next;
	__MCName *key;
	MCStringRef string;
	hash_t hash;
};

//////////

enum
{
	// The first 6 bits of the flags hold the capacity index.
	kMCArrayFlagCapacityIndexMask = (1 << 6) - 1,
	// If set then the array is mutable.
	kMCArrayFlagIsMutable = 1 << 6,
	// If set then the array is indirect (i.e. contents is within another
	// immutable array).
	kMCArrayFlagIsIndirect = 1 << 7,
};

struct __MCArrayKeyValue
{
	MCNameRef key;
	uintptr_t value;
};

struct __MCArray: public __MCValue
{
	union
	{
		MCArrayRef contents;
		struct
		{
			__MCArrayKeyValue *key_values;
			uindex_t key_value_count;
		};
	};
};

//////////

enum
{
	kMCListFlagIsMutable = 1 << 0,
};

struct __MCList: public __MCValue
{
	MCStringRef delimiter;
	MCStringRef buffer;
};

////////

enum
{
	kMCSetFlagIsMutable = 1 << 0,
};

struct __MCSet: public __MCValue
{
	uindex_t *limbs;
	uindex_t limb_count;
};

////////

struct __MCCustomValue: public __MCValue
{
	const MCValueCustomCallbacks *callbacks;
};

////////////////////////////////////////////////////////////////////////////////

extern const uindex_t __kMCValueHashTableSizes[];
extern const uindex_t __kMCValueHashTableCapacities[];

bool __MCValueCreate(MCValueTypeCode type_code, size_t size, __MCValue*& r_value);
void __MCValueDestroy(__MCValue *value);

bool __MCValueImmutableCopy(__MCValue *value, bool release, __MCValue*& r_new_value);

inline MCValueTypeCode __MCValueGetTypeCode(__MCValue *self)
{
	return (self -> flags >> 28);
}

template<class T> inline bool __MCValueCreate(MCValueTypeCode p_type_code, T*& r_value)
{
	__MCValue *t_value;
	if (__MCValueCreate(p_type_code, sizeof(T), t_value))
		return r_value = (T *)t_value, true;
	return false;
}

//////////

bool __MCUnicodeInitialize();
bool __MCLocaleInitialize();
void __MCUnicodeFinalize();
void __MCLocaleFinalize();

bool __MCValueInitialize(void);
void __MCValueFinalize(void);

bool __MCStringInitialize(void);
void __MCStringFinalize(void);
void __MCStringDestroy(__MCString *string);
hash_t __MCStringHash(__MCString *string);
bool __MCStringIsEqualTo(__MCString *string, __MCString *other_string);
bool __MCStringCopyDescription(__MCString *string, MCStringRef& r_string);
bool __MCStringImmutableCopy(__MCString *string, bool release, __MCString*& r_immutable_value);

bool __MCNameInitialize(void);
void __MCNameFinalize(void);
void __MCNameDestroy(__MCName *name);
hash_t __MCNameHash(__MCName *name);
bool __MCNameCopyDescription(__MCName *name, MCStringRef& r_string);

bool __MCNumberInitialize(void);
void __MCNumberFinalize(void);
hash_t __MCNumberHash(__MCNumber *number);
bool __MCNumberIsEqualTo(__MCNumber *number, __MCNumber *other_number);
bool __MCNumberCopyDescription(__MCNumber *number, MCStringRef& r_string);

bool __MCArrayInitialize(void);
void __MCArrayFinalize(void);
void __MCArrayDestroy(__MCArray *array);
hash_t __MCArrayHash(__MCArray *array);
bool __MCArrayIsEqualTo(__MCArray *array, __MCArray *other_array);
bool __MCArrayCopyDescription(__MCArray *array, MCStringRef& r_string);
bool __MCArrayImmutableCopy(__MCArray *array, bool release, __MCArray*& r_immutable_value);

bool __MCListInitialize(void);
void __MCListFinalize(void);
void __MCListDestroy(__MCList *list);
hash_t __MCListHash(__MCList *list);
bool __MCListIsEqualTo(__MCList *list, __MCList *other_list);
bool __MCListCopyDescription(__MCList *list, MCStringRef& r_string);
bool __MCListImmutableCopy(__MCList *list, bool release, __MCList*& r_immutable_value);

bool __MCSetInitialize(void);
void __MCSetFinalize(void);
void __MCSetDestroy(__MCSet *set);
hash_t __MCSetHash(__MCSet *set);
bool __MCSetIsEqualTo(__MCSet *set, __MCSet *other_set);
bool __MCSetCopyDescription(__MCSet *set, MCStringRef& r_string);
bool __MCSetImmutableCopy(__MCSet *set, bool release, __MCSet*& r_immutable_value);

bool __MCDataInitialize(void);
void __MCDataFinalize(void);
void __MCDataDestroy(__MCData *data);
hash_t __MCDataHash(__MCData *self);
bool __MCDataIsEqualTo(__MCData *self, __MCData *p_other_data);
bool __MCDataCopyDescription(__MCData *self, MCStringRef &r_description);
bool __MCDataImmutableCopy(__MCData *self, bool p_release, __MCData *&r_immutable_value);

////////////////////////////////////////////////////////////////////////////////

hash_t MCNativeCharsHash(const char_t *chars, uindex_t char_count, MCStringOptions p_options);

bool MCNativeCharsEqualExact(const char_t *left, uindex_t left_length, const char_t *right, uindex_t right_length);
bool MCNativeCharsEqualCaseless(const char_t *left, uindex_t left_length, const char_t *right, uindex_t right_length);

compare_t MCNativeCharsCompareExact(const char_t *left, uindex_t left_length, const char_t *right, uindex_t right_length);
compare_t MCNativeCharsCompareCaseless(const char_t *left, uindex_t left_length, const char_t *right, uindex_t right_length);

// Return the number of characters of prefix that are equal to those at the
// beginning of string.
uindex_t MCNativeCharsSharedPrefixExact(const char_t *string, uindex_t left_length, const char_t *suffix, uindex_t right_length);
uindex_t MCNativeCharsSharedPrefixCaseless(const char_t *string, uindex_t left_length, const char_t *suffix, uindex_t right_length);

// Return the number of characters of suffix that are equal to those at the
// end of string.
uindex_t MCNativeCharsSharedSuffixExact(const char_t *string, uindex_t left_length, const char_t *suffix, uindex_t right_length);
uindex_t MCNativeCharsSharedSuffixCaseless(const char_t *string, uindex_t left_length, const char_t *suffix, uindex_t right_length);

// Lowercase all the characters in-place.
void MCNativeCharsLowercase(char_t *chars, uindex_t char_count);

// Uppercase all the characters in-place.
void MCNativeCharsUppercase(char_t *chars, uindex_t char_count);

// Fold the given native char for caseless comparison.
char_t MCNativeCharFold(char_t c);

// Lowercase the given char
char_t MCNativeCharLowercase(char_t c);

// Uppercase the given char
char_t MCNativeCharUppercase(char_t c);

// Format the given string - the return string is NUL formatted.
bool MCNativeCharsFormatV(char_t*& r_string, uindex_t& r_size, const char *format, va_list args);

//////////

//hash_t MCUnicodeCharsHashExact(const unichar_t *chars, uindex_t char_count);
//hash_t MCUnicodeCharsHashCaseless(const unichar_t *chars, uindex_t char_count);

//bool MCUnicodeCharsEqualExact(const unichar_t *left, uindex_t left_length, const unichar_t *right, uindex_t right_length);
//bool MCUnicodeCharsEqualCaseless(const unichar_t *left, uindex_t left_length, const unichar_t *right, uindex_t right_length);

//compare_t MCUnicodeCharsCompareExact(const unichar_t *left, uindex_t left_length, const unichar_t *right, uindex_t right_length);
//compare_t MCUnicodeCharsCompareCaseless(const unichar_t *left, uindex_t left_length, const unichar_t *right, uindex_t right_length);

// Return the number of characters of prefix that are equal to those at the
// beginning of string.
//uindex_t MCUnicodeCharsSharedPrefixExact(const unichar_t *string, uindex_t left_length, const unichar_t *suffix, uindex_t right_length);
//uindex_t MCUnicodeCharsSharedPrefixCaseless(const unichar_t *string, uindex_t left_length, const unichar_t *suffix, uindex_t right_length);

// Return the number of characters of suffix that are equal to those at the
// end of string.
//uindex_t MCUnicodeCharsSharedSuffixExact(const unichar_t *string, uindex_t left_length, const unichar_t *suffix, uindex_t right_length);
//uindex_t MCUnicodeCharsSharedSuffixCaseless(const unichar_t *string, uindex_t left_length, const unichar_t *suffix, uindex_t right_length);

// Lowercase all the characters in-place.
//void MCUnicodeCharsLowercase(unichar_t *chars, uindex_t char_count);

// Uppercase all the characters in-place.
//void MCUnicodeCharsUppercase(unichar_t *chars, uindex_t char_count);

//bool MCUnicodeCharsEqualExact(const unichar_t *left, uindex_t left_length, const unichar_t *right, uindex_t right_length);
//bool MCUnicodeCharsEqualCaseless(const unichar_t *left, uindex_t left_length, const unichar_t *right, uindex_t right_length);

bool MCUnicodeCharsMapToNative(const unichar_t *uchars, uindex_t uchar_count, char_t *nchars, uindex_t& r_nchar_count, char_t invalid);
void MCUnicodeCharsMapFromNative(const char_t *chars, uindex_t char_count, unichar_t *uchars);

uindex_t MCUnicodeCharsMapToUTF8(const unichar_t *wchars, uindex_t wchar_count, byte_t *utf8bytes, uindex_t utf8byte_count);
uindex_t MCUnicodeCharsMapFromUTF8(const byte_t *utf8bytes, uindex_t utf8byte_count, unichar_t *wchars, uindex_t wchar_count);

bool MCUnicodeCharMapToNative(unichar_t uchar, char_t& r_nchar);
char_t MCUnicodeCharMapToNativeLossy(unichar_t nchar);
unichar_t MCUnicodeCharMapFromNative(char_t nchar);

//unichar_t MCUnicodeCharFold(unichar_t);

//unichar_t MCUnicodeCharLowercase(unichar_t);

//unichar_t MCUnicodeCharUppercase(unichar_t);

////////////////////////////////////////////////////////////////////////////////

#endif
