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

#ifndef __MC_FOUNDATION_STRING__
#define __MC_FOUNDATION_STRING__

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
	
	// Map 'native' encoding to the appropriate concrete encoding depending
	// on platform.
#ifdef __WINDOWS_1252__
	kMCStringEncodingNative = kMCStringEncodingWindows1252,
#elif defined(__MACROMAN__)
	kMCStringEncodingNative = kMCStringEncodingMacRoman,
#elif defined(__ISO_8859_1__)
	kMCStringEncodingNative = kMCStringEncodingISO8859_1,
#endif
	
	// Map UTF-16 and UTF-32 encoding to the appropriate concrete encoding
	// depending on host byte-order.
#ifdef __LITTLE_ENDIAN__
	kMCStringEncodingUTF16 = kMCStringEncodingUTF16LE,
	kMCStringEncodingUTF32 = kMCStringEncodingUTF32LE,
#elif defined(__BIG_ENDIAN__)
	kMCStringEncodingUTF16 = kMCStringEncodingUTF16BE,
	kMCStringEncodingUTF32 = kMCStringEncodingUTF32BE,
#else
#error Processor endian-type not defined
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
};

/////////

// The empty string.
extern MCStringRef kMCEmptyString;

// The default string for the 'true' boolean value.
extern MCStringRef kMCTrueString;

// The default string for the 'false' boolean value.
extern MCStringRef kMCFalseString;

/////////

// Create an immutable string from the given bytes, interpreting them using
// the specified encoding.
bool MCStringCreateWithBytes(const byte_t *bytes, uindex_t byte_count, MCStringEncoding encoding, MCStringRef& r_string);

// Create an immutable string from the given unicode char sequence.
bool MCStringCreateWithChars(const unichar_t *chars, uindex_t char_count, MCStringRef& r_string);

// Create an immutable string from the given NUL terminated unicode char sequence.
bool MCStringCreateWithWString(const unichar_t *wstring, MCStringRef& r_string);

// Create an immutable string from the given native char sequence.
bool MCStringCreateWithNativeChars(const char_t *chars, uindex_t char_count, MCStringRef& r_string);
bool MCStringCreateWithNativeCharsAndRelease(char_t *chars, uindex_t char_count, MCStringRef& r_string);

#ifdef __HAS_CORE_FOUNDATION__
// Create a string from a CoreFoundation string object.
bool MCStringCreateWithCFString(CFStringRef cf_string, MCStringRef& r_string);
#endif

// Create a mutable string with the given initial capacity. Note that the
// initial capacity is only treated as a hint, the string will extend itself
// as necessary.
bool MCStringCreateMutable(uindex_t initial_capacity, MCStringRef& r_string);

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
bool MCStringMutableCopyAndRelease(MCStringRef string, MCRange range, MCStringRef& r_substring);

/////////

// Returns true if the string is mutable
bool MCStringIsMutable(const MCStringRef string);

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

// Return a pointer to the byte backing-store if possible. For this method to
// succeed the string must be binary (native), otherwise nil will be returned.
const byte_t *MCStringGetBytePtr(MCStringRef string);

// Returns the char at the given index.
unichar_t MCStringGetCharAtIndex(MCStringRef string, uindex_t index);

// Returns the native char at the given index.
char_t MCStringGetNativeCharAtIndex(MCStringRef string, uindex_t index);

// Returns the sequence of chars making up the given range in 'chars' and returns
// the number of chars generated. If 'chars' is nil, just the number of chars that
// would be generated is returned.
uindex_t MCStringGetChars(MCStringRef string, MCRange range, unichar_t *chars);

// Returns the sequence of native chars making up the given range in 'chars' and
// returns the number of chars generated. If 'chars' is nil, just the number of chars
// that would be generated is returned. Any unmappable chars get generated as '?'.
uindex_t MCStringGetNativeChars(MCStringRef string, MCRange range, char_t *chars);

/////////

// Converts the contents of the string to unicode. The caller takes ownership of the
// char array. Note that the returned array is NUL terminated, but this is not
// reflected in the char count.
bool MCStringConvertToUnicode(MCStringRef string, unichar_t*& r_chars, uindex_t& r_char_count);

// Converts the contents of the string to native - using '?' as the unmappable char.
// The caller takes ownership of the char array. Note that the returned array is NUL
// terminated, but this is not reflected in the char count.
bool MCStringConvertToNative(MCStringRef string, char_t*& r_chars, uindex_t& r_char_count);

/////////

// Returns the hash of the given string, processing as according to options.
hash_t MCStringHash(MCStringRef string, MCStringOptions options);

// Returns true if the two strings are equal, processing as appropriate according
// to options.
bool MCStringIsEqualTo(MCStringRef string, MCStringRef other, MCStringOptions options);
bool MCStringIsEqualToNativeChars(MCStringRef string, const char_t *chars, uindex_t char_count, MCStringOptions options);

// Returns true if the substring is equal to the other, according to options
bool MCStringSubstringIsEqualTo(MCStringRef string, MCRange other, MCStringRef p_other, MCStringOptions p_options);

// Returns -1, 0, or 1, depending on whether left < 0, left == right or left > 0,
// processing as appropriate according to options. The ordering used is codepoint-
// wise lexicographic.
compare_t MCStringCompareTo(MCStringRef string, MCStringRef other, MCStringOptions options);

// Returns true if the string begins with the prefix string, processing as
// appropriate according to options.
bool MCStringBeginsWith(MCStringRef string, MCStringRef prefix, MCStringOptions options);
bool MCStringBeginsWithCString(MCStringRef string, const char_t *prefix_cstring, MCStringOptions options);

// Returns true if the string ends with the suffix string, processing as
// appropriate according to options.
bool MCStringEndsWith(MCStringRef string, MCStringRef suffix, MCStringOptions options);
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

// Find the last offset of needle in string, on or before index 'before',
// processing as appropriate according to options.
bool MCStringLastIndexOf(MCStringRef string, MCStringRef needle, uindex_t before, MCStringOptions options, uindex_t& r_offset);
// Find the last offset of needle in string - where needle is a Unicode character
// (note it is a codepoint, not unichar - i.e. a 20-bit value).
bool MCStringLastIndexOfChar(MCStringRef string, codepoint_t needle, uindex_t after, MCStringOptions options, uindex_t& r_offset);

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

// Transform the string to its folded form as specified by 'options'. The folded
// form of a string is that which is used to perform comparisons.
//
// Note that 'string' must be mutable, it is a fatal runtime error if it is not.
bool MCStringFold(MCStringRef string, MCStringOptions options);

// Lowercase the string.
//
// Note that 'string' must be mutable, it is a fatal runtime error if it is not.
bool MCStringLowercase(MCStringRef string);

// Uppercase the string.
//
// Note that 'string' must be mutable, it is a fatal runtime error if it is not.
bool MCStringUppercase(MCStringRef string);

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
bool MCStringPrependNativeChars(MCStringRef string, const char_t *chars, uindex_t count);

// Insert new_string into string at offset 'at'.
//
// Note that 'string' must be mutable, it is a fatal runtime error if it is not.
bool MCStringInsert(MCStringRef string, uindex_t at, MCStringRef new_string);

// Remove 'range' characters from 'string'.
//
// Note that 'string' must be mutable, it is a fatal runtime error if it is not.
bool MCStringRemove(MCStringRef string, MCRange range);

// Replace 'range' characters in 'string' with 'replacement'.
//
// Note that 'string' must be mutable, it is a fatal runtime error if it is not.
bool MCStringReplace(MCStringRef string, MCRange range, MCStringRef replacement);

// Pad the end of the string with count copies of value. If value is nil then count
// uninitialized bytes will be inserted after at.
//
// Note that 'string' must be mutable.
bool MCStringPad(MCStringRef string, uindex_t at, uindex_t count, MCStringRef value);

// Find and replace all instances of pattern in target with replacement.
//
// Note that 'string' must be mutable.
bool MCStringFindAndReplace(MCStringRef string, MCStringRef pattern, MCStringRef replacement, MCStringOptions options);
bool MCStringFindAndReplaceChar(MCStringRef string, char_t pattern, char_t replacement, MCStringOptions options);

/////////

// Append a formatted string to another string.
//
// Note that 'string' must be mutable, it is a fatal runtime error if it is not.
bool MCStringAppendFormat(MCStringRef string, const char *format, ...);
bool MCStringAppendFormatV(MCStringRef string, const char *format, va_list args);

//////////

bool MCStringSplit(MCStringRef string, MCStringRef element_del, MCStringRef key_del, MCStringOptions options, MCArrayRef& r_array);
bool MCStringSplitColumn(MCStringRef string, MCStringRef col_del, MCStringRef row_del, MCStringOptions options, MCArrayRef& r_array);

#endif
