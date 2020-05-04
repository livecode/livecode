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


#include <stdio.h>
#include <utility>

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
    
    // The mask for the typecode bits.
    kMCValueFlagsTypeCodeMask = 0xf0000000,
    
    // Names store the hash value in the flags word.
    kMCValueFlagsNameHashBits = 28,
    kMCValueFlagsNameHashMask = (1 << kMCValueFlagsNameHashBits) - 1,
};

struct __MCValue
{
	uint32_t references;
	uint32_t flags;
};

//////////

enum
{
    kMCTypeInfoTypeCodeMask = 0xff,
    kMCTypeInfoFlagHandlerIsForeign = 1 << 8,
    kMCTypeInfoFlagHandlerIsVariadic = 1 << 9,
    
    // We use typecodes well above the fixed ones we have to
    // indicate 'special' typeinfo (i.e. those with no real
    // valueref type underneath).
    kMCTypeInfoTypeIsAny = 255,
    kMCTypeInfoTypeIsNamed = 254,
    kMCTypeInfoTypeIsAlias = 253,
    kMCTypeInfoTypeIsOptional = 252,
    kMCTypeInfoTypeIsForeign = 251,
};

struct MCHandlerTypeLayout
{
    MCHandlerTypeLayout *next;
    int abi;
    char cif[1];
};

struct __MCTypeInfo: public __MCValue
{
    union
    {
        struct
        {
            MCNameRef name;
            MCTypeInfoRef typeinfo;
        } named, alias;
        struct
        {
            MCTypeInfoRef basetype;
        } optional;
        struct
        {
            MCRecordTypeFieldInfo *fields;
            uindex_t field_count;
        } record;
        struct
        {
            /* forward declaration, to avoid including FFI header here */
            typedef struct _ffi_type ffi_type;

            MCHandlerTypeFieldInfo *fields;
            uindex_t field_count;
            MCTypeInfoRef return_type;
            ffi_type** layout_args;
            MCHandlerTypeLayout *layouts;
        } handler;
        struct
        {
            MCNameRef domain;
            MCStringRef message;
        } error;
        struct
        {
            MCValueCustomCallbacks callbacks;
            MCTypeInfoRef base;
        } custom;
        struct
        {
            MCForeignTypeDescriptor descriptor;
            void *ffi_layout_type;
        } foreign;
    };
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
    kMCStringFlagIsBasic = 1 << 3,
    // If set, the string has been checked for simplicity
    kMCStringFlagIsChecked = 1 << 4,
    // If set, the string is basic and contains NO combining chars
    kMCStringFlagIsTrivial = 1 << 5,
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
#ifdef __32_BIT__
            uindex_t char_count;
            union
            {
                unichar_t *chars;
                char_t *native_chars;
            };
            double numeric_value;
            uindex_t capacity;
            /* The padding is here to ensure the size of the struct is 32-bytes
             * on all platforms. This ensures consistency between Win and UNIX
             * ABIs which have slightly different rules concerning double
             * alignment. */
            uint32_t __padding;
#else
            uindex_t char_count;
            uindex_t capacity;
            union
            {
                unichar_t *chars;
                char_t *native_chars;
            };
            double numeric_value;
#endif
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
            uindex_t capacity;
            byte_t *bytes;
        };
    };
};

//////////

struct __MCName: public __MCValue
{
#ifdef __32_BIT__
    __MCName *next;
    __MCName *key;
    MCStringRef string;
    hash_t hash;
#else
	uintptr_t next;
	uintptr_t key;
	MCStringRef string;
#endif
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

//////////

enum
{
	// If set then the list is mutable.
	kMCProperListFlagIsMutable = 1 << 0,
    // If set then the list is indirect (i.e. contents is within another
	// immutable list).
	kMCProperListFlagIsIndirect = 1 << 1,
};

struct __MCProperList: public __MCValue
{
	union
	{
		MCProperListRef contents;
        struct
        {
            MCValueRef *list;
            uindex_t length;
        };
	};
};

////////

enum
{
    // The data are mutable
    kMCRecordFlagIsMutable = 1,
};

struct __MCRecord: public __MCValue
{
    MCTypeInfoRef typeinfo;
    MCValueRef *fields;
};

////////

struct MCErrorFrame
{
    MCErrorFrame *caller;
    MCValueRef target;
    uindex_t row;
    uindex_t column;
};

struct __MCError: public __MCValue
{
    MCTypeInfoRef typeinfo;
    MCStringRef message;
    MCArrayRef info;
    MCErrorFrame *backtrace;
};

////////

struct __MCCustomValue: public __MCValue
{
    MCTypeInfoRef typeinfo;
};

////////

struct __MCForeignValue: public __MCValue
{
    union
    {
        MCTypeInfoRef typeinfo;
        uint64_t _dummy;
    };
};

////////

#ifdef __HAS_MULTIPLE_ABIS__
struct __MCHandlerClosureWithAbi
{
	__MCHandlerClosureWithAbi *next;
	int abi;
	void *closure;
	void *function_ptr;
};
#endif

struct __MCHandler: public __MCValue
{
    MCTypeInfoRef typeinfo;
    const MCHandlerCallbacks *callbacks;
	/* We store the closure with default ABI in the value. */
    void *closure;
    void *function_ptr;
#ifdef __HAS_MULTIPLE_ABIS__
	/* All closures with non-default ABIs are stored in a linked list. */
	__MCHandlerClosureWithAbi *other_closures;
#endif
    char context[1];
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

/* Allocate a value ref structure that has enough space to hold an
 * instance of ValueType, possibly with some extra space tacked on the
 * end. */
template <typename ValueType>
inline bool __MCValueCreateExtended(MCValueTypeCode p_type_code,
                                    size_t p_extra_space,
                                    ValueType*& r_value)
{
    __MCValue* t_new = nullptr;
    if (__MCValueCreate(p_type_code, sizeof(ValueType) + p_extra_space, t_new))
        return r_value = static_cast<ValueType*>(t_new), r_value != nullptr;
    return false;
}

//////////

bool __MCUnicodeInitialize();
void __MCUnicodeFinalize();

bool __MCLocaleInitialize();
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

bool __MCProperListInitialize(void);
void __MCProperListFinalize(void);
void __MCProperListDestroy(__MCProperList *list);
hash_t __MCProperListHash(__MCProperList *list);
bool __MCProperListIsEqualTo(__MCProperList *list, __MCProperList *other_list);
bool __MCProperListCopyDescription(__MCProperList *list, MCStringRef& r_string);
bool __MCProperListImmutableCopy(__MCProperList *list, bool release, __MCProperList*& r_immutable_value);

bool __MCRecordInitialize(void);
void __MCRecordFinalize(void);
void __MCRecordDestroy(__MCRecord *data);
hash_t __MCRecordHash(__MCRecord *self);
bool __MCRecordIsEqualTo(__MCRecord *self, __MCRecord *p_other_data);
bool __MCRecordCopyDescription(__MCRecord *self, MCStringRef &r_description);
bool __MCRecordImmutableCopy(__MCRecord *self, bool p_release, __MCRecord *&r_immutable_value);

bool __MCErrorInitialize();
void __MCErrorFinalize();
void __MCErrorDestroy(__MCError *error);
hash_t __MCErrorHash(__MCError *error);
bool __MCErrorIsEqualTo(__MCError *error, __MCError *other_error);
bool __MCErrorCopyDescription(__MCError *error, MCStringRef& r_string);

bool __MCTypeInfoInitialize(void);
void __MCTypeInfoFinalize(void);
void __MCTypeInfoDestroy(__MCTypeInfo *self);
hash_t __MCTypeInfoHash(__MCTypeInfo *self);
bool __MCTypeInfoIsEqualTo(__MCTypeInfo *self, __MCTypeInfo *other_self);
bool __MCTypeInfoCopyDescription(__MCTypeInfo *self, MCStringRef& r_description);
MCTypeInfoRef __MCTypeInfoResolve(__MCTypeInfo *self);

uindex_t __MCRecordTypeInfoGetFieldCount (__MCTypeInfo *self);

bool __MCForeignValueInitialize(void);
void __MCForeignValueFinalize(void);
void __MCForeignValueDestroy(__MCForeignValue *self);
hash_t __MCForeignValueHash(__MCForeignValue *self);
bool __MCForeignValueIsEqualTo(__MCForeignValue *self, __MCForeignValue *other_self);
bool __MCForeignValueCopyDescription(__MCForeignValue *self, MCStringRef& r_description);

void __MCHandlerDestroy(__MCHandler *self);
hash_t __MCHandlerHash(__MCHandler *self);
bool __MCHandlerIsEqualTo(__MCHandler *self, __MCHandler *other_self);
bool __MCHandlerCopyDescription(__MCHandler *self, MCStringRef& r_description);

bool __MCStreamInitialize(void);
void __MCStreamFinalize(void);

bool __MCJavaInitialize(void);
void __MCJavaFinalize(void);

bool __MCObjcInitialize(void);
void __MCObjcFinalize(void);

/* Default implementations of each of the function members of struct &
 * MCValueCustomCallbacks */
MCTypeInfoRef __MCCustomValueResolveTypeInfo(__MCValue *p_value);
bool __MCCustomCopyDescription (MCValueRef self, MCStringRef & r_desc);

void __MCCustomDefaultDestroy(MCValueRef);
bool __MCCustomDefaultCopy(MCValueRef, bool, MCValueRef &);
bool __MCCustomDefaultEqual(MCValueRef, MCValueRef);
hash_t __MCCustomDefaultHash(MCValueRef);
bool __MCCustomDefaultDescribe(MCValueRef, MCStringRef &);
bool __MCCustomDefaultIsMutable(MCValueRef);
bool __MCCustomDefaultMutableCopy(MCValueRef, bool, MCValueRef &);

////////////////////////////////////////////////////////////////////////////////

// Fold the given native char for caseless comparison.
char_t MCNativeCharFold(char_t c);

// Lowercase the given char
char_t MCNativeCharLowercase(char_t c);

// Uppercase the given char
char_t MCNativeCharUppercase(char_t c);

// Format the given string - the return string is NUL formatted.
bool MCNativeCharsFormatV(char_t*& r_string, uindex_t& r_size, const char *format, va_list args);

//////////

bool MCUnicodeCharsMapToNative(const unichar_t *uchars, uindex_t uchar_count, char_t *nchars, uindex_t& r_nchar_count, char_t invalid);
void MCUnicodeCharsMapFromNative(const char_t *chars, uindex_t char_count, unichar_t *uchars);

uindex_t MCUnicodeCharsMapToUTF8(const unichar_t *wchars, uindex_t wchar_count, byte_t *utf8bytes, uindex_t utf8byte_count);
uindex_t MCUnicodeCharsMapFromUTF8(const byte_t *utf8bytes, uindex_t utf8byte_count, unichar_t *wchars, uindex_t wchar_count);

bool MCUnicodeCharMapToNative(unichar_t uchar, char_t& r_nchar);
char_t MCUnicodeCharMapToNativeLossy(unichar_t nchar);
unichar_t MCUnicodeCharMapFromNative(char_t nchar);

////////////////////////////////////////////////////////////////////////////////
// INTERNAL MCVALUE TYPE ASSERTIONS
//

inline void
__MCAssertResolvedTypeInfo(MCTypeInfoRef x, bool (*p)(MCTypeInfoRef))
{
	MCResolvedTypeInfo r;
	MCAssert(MCTypeInfoResolve(x, r) && p(r.type));
}

#define __MCAssertValueType(x,T) MCAssert(MCValueGetTypeCode(x) == kMCValueTypeCode##T)

// A valid ValueRef must have references > 0 and flags != -1
#define __MCAssertIsValue(x)    MCAssert(((__MCValue *)x) -> flags != UINT32_MAX && ((__MCValue *)x) -> references > 0);

#define __MCAssertIsNumber(x)   __MCAssertValueType(x,Number)
#define __MCAssertIsName(x)     __MCAssertValueType(x,Name)
#define __MCAssertIsString(x)   __MCAssertValueType(x,String)
#define __MCAssertIsData(x)     __MCAssertValueType(x,Data)
#define __MCAssertIsArray(x)    __MCAssertValueType(x,Array)
#define __MCAssertIsList(x)     __MCAssertValueType(x,List)
#define __MCAssertIsSet(x)      __MCAssertValueType(x,Set)
#define __MCAssertIsTypeInfo(x) __MCAssertValueType(x,TypeInfo)
#define __MCAssertIsError(x)    __MCAssertValueType(x,Error)
#define __MCAssertIsRecord(x)   __MCAssertValueType(x,Record)
#define __MCAssertIsHandler(x)  __MCAssertValueType(x,Handler)
#define __MCAssertIsProperList(x) __MCAssertValueType(x,ProperList)

#define __MCAssertIsLocale(x)   MCAssert(nil != (x)) /* FIXME */

#define __MCAssertIsMutableString(x) MCAssert(MCStringIsMutable(x))
#define __MCAssertIsMutableData(x)   MCAssert(MCDataIsMutable(x))

#define __MCAssertIsErrorTypeInfo(x) __MCAssertResolvedTypeInfo(x, MCTypeInfoIsError)
#define __MCAssertIsForeignTypeInfo(x) __MCAssertResolvedTypeInfo(x, MCTypeInfoIsForeign)

////////////////////////////////////////////////////////////////////////////////
// ALGORITHM TEMPLATES
//

/* Efficiently reverse the order of elements in an array, in-place.
 * The algorithm works from the middle of the array outwards, swapping
 * the elements at each end.  The ElementType must be swappable,
 * either by being trivially copyable or by implementing a
 * std::swap()-compatible swap operation that can be found by ADL. */
/* TODO[C++11] Obsolete this macro by using std::reverse from
 * <algorithm>.  Would require MCSpan to support STL iteration. */
template <typename ElementType, typename IndexType>
inline void MCInplaceReverse(ElementType* x_elements, IndexType p_num_elements)
{
    using std::swap;
    MCAssert(x_elements != nullptr || p_num_elements == 0);
    for (auto t_count = p_num_elements/2; t_count > 0; --t_count)
        swap(x_elements[t_count - 1],
             x_elements[p_num_elements - t_count]);
}

////////////////////////////////////////////////////////////////////////////////

#endif
