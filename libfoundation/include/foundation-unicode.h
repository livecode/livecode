/* Copyright (C) 2015 LiveCode Ltd.
 
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

#ifndef __MC_FOUNDATION_UNICODE__
#define __MC_FOUNDATION_UNICODE__

#include "foundation.h"
#include "foundation-locale.h"

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

constexpr inline codepoint_t
MCUnicodeSurrogatesToCodepoint(uint16_t first, uint16_t second)
{
    return 0x10000 + ((first & 0x3FF) << 10) + (second & 0x3FF);
}

/* Split p_codepoint into a surrogate pair, storing the leading
 * component in r_leading and the trailing component in
 * r_trailing.  If p_codepoint is in the BMP, set r_leading
 * to p_codepoint, clear r_trailing, and return false. */
inline bool MCUnicodeCodepointToSurrogates(codepoint_t p_codepoint,
                                           unichar_t& r_leading,
                                           unichar_t& r_trailing)
{
    if (p_codepoint < 0x10000)
    {
        r_leading = MCNarrowCast<unichar_t>(p_codepoint);
        r_trailing = 0;
        return false;
    }
    else
    {
        p_codepoint -= 0x10000;
        r_leading = 0xD800 + (p_codepoint >> 10);
        r_trailing = 0xDC00 + (p_codepoint & 0x3FF);
        return true;
    }
}

////////////////////////////////////////////////////////////////////////////////

// Unicode character properties
enum MCUnicodeProperty
{
    // Binary properties
    kMCUnicodePropertyAlphabetic = 0,
    kMCUnicodePropertyASCIIHex,
    kMCUnicodePropertyBidiControl,
    kMCUnicodePropertyBidiMirrored,
    kMCUnicodePropertyDash,
    kMCUnicodePropertyDefaultIgnorable,
    kMCUnicodePropertyDeprecated,
    kMCUnicodePropertyDiacritic,
    kMCUnicodePropertyExtender,
    kMCUnicodePropertyFullCompositionExclusion,
    kMCUnicodePropertyGraphemeBase,
    kMCUnicodePropertyGraphemeExtended,
    kMCUnicodePropertyGraphemeLink,
    kMCUnicodePropertyHexDigit,
    kMCUnicodePropertyHyphen,
    kMCUnicodePropertyIDContinue,
    kMCUnicodePropertyIDStart,
    kMCUnicodePropertyIdeographic,
    kMCUnicodePropertyIDSBinaryOperator,
    kMCUnicodePropertyIDSTrinaryOperator,
    kMCUnicodePropertyJoinControl,
    kMCUnicodePropertyLocalOrderException,
    kMCUnicodePropertyLowercase,
    kMCUnicodePropertyMath,
    kMCUnicodePropertyNoncharacterCodePoint,
    kMCUnicodePropertyQuotationMark,
    kMCUnicodePropertyRadical,
    kMCUnicodePropertySoftDotted,
    kMCUnicodePropertyTerminalPunctuation,
    kMCUnicodePropertyUnifiedIdeograph,
    kMCUnicodePropertyUppercase,
    kMCUnicodePropertyWhiteSpace,
    kMCUnicodePropertyXIDContinue,
    kMCUnicodePropertyXIDStart,
    kMCUnicodePropertyCaseSensitive,
    kMCUnicodePropertySTerminal,
    kMCUnicodePropertyVariationSelector,
    kMCUnicodePropertyNFDInert,
    kMCUnicodePropertyNFKDInert,
    kMCUnicodePropertyNFCInert,
    kMCUnicodePropertyNFKCInert,
    kMCUnicodePropertySegmentStarter,
    kMCUnicodePropertyPatternSyntax,
    kMCUnicodePropertyPatternWhiteSpace,
    kMCUnicodePropertyPosixAlnum,
    kMCUnicodePropertyPosixBlank,
    kMCUnicodePropertyPosixGraph,
    kMCUnicodePropertyPosixPrint,
    kMCUnicodePropertyPosixHexDigit,
    kMCUnicodePropertyCased,
    kMCUnicodePropertyCaseIgnorable,
    kMCUnicodePropertyChangesWhenLowercased,
    kMCUnicodePropertyChangesWhenUppercased,
    kMCUnicodePropertyChangesWhenTitlecased,
    kMCUnicodePropertyChangesWhenCaseFolded,
    kMCUnicodePropertyChangesWhenCaseMapped,
    kMCUnicodePropertyChangesWhenNFKCCaseFolded,
    kMCUnicodePropertyLastBinary = kMCUnicodePropertyChangesWhenNFKCCaseFolded,
    
    // Integer properties
    kMCUnicodePropertyBidiClass,
    kMCUnicodePropertyBlock,
    kMCUnicodePropertyCanonicalCombiningClass,
    kMCUnicodePropertyDecompositionType,
    kMCUnicodePropertyEastAsianWidth,
    kMCUnicodePropertyGeneralCategory,
    kMCUnicodePropertyJoiningGroup,
    kMCUnicodePropertyJoiningType,
    kMCUnicodePropertyLineBreak,
    kMCUnicodePropertyNumericType,
    kMCUnicodePropertyScript,
    kMCUnicodePropertyHangulSyllableType,
    kMCUnicodePropertyNFDQuickCheck,
    kMCUnicodePropertyNFKDQuickCheck,
    kMCUnicodePropertyNFCQuickCheck,
    kMCUnicodePropertyNFKCQuickCheck,
    kMCUnicodePropertyLeadCanonicalCombiningClass,
    kMCUnicodePropertyTrailCanonicalCombiningClass,
    kMCUnicodePropertyGraphemeClusterBreak,
    kMCUnicodePropertySentenceBreak,
    kMCUnicodePropertyWordBreak,
    kMCUnicodePropertyBidiPairedBracketType,
    kMCUnicodePropertyLastInteger = kMCUnicodePropertyBidiPairedBracketType,
    
    // Bitmask properties
    kMCUnicodePropertyGeneralCategoryMask,
    kMCUnicodePropertyLastBitmask = kMCUnicodePropertyGeneralCategoryMask,
    
    // Floating-point properties
    kMCUnicodePropertyNumericValue,
    kMCUnicodePropertyLastFloatingPoint = kMCUnicodePropertyNumericValue,
    
    // Character properties
    kMCUnicodePropertyBidiMirroringGlyph,
    kMCUnicodePropertySimpleCaseFolding,
    kMCUnicodePropertySimpleLowercaseMapping,
    kMCUnicodePropertySimpleTitlecaseMapping,
    kMCUnicodePropertySimpleUppercaseMapping,
    kMCUnicodePropertyBidiPairedBracket,
    kMCUnicodePropertyLastCharacter = kMCUnicodePropertyBidiPairedBracket,
    
    // String properties
    kMCUnicodePropertyAge,
    kMCUnicodePropertyCaseFolding,
    kMCUnicodePropertyISOComment,
    kMCUnicodePropertyLowercaseMapping,
    kMCUnicodePropertyName,
    kMCUnicodePropertyTitlecaseMapping,
    kMCUnicodePropertyUnicode1Name,
    kMCUnicodePropertyUppercaseMapping,
    kMCUnicodePropertyLastString = kMCUnicodePropertyUppercaseMapping,
   
};

// Character categories
enum MCUnicodeCategory
{
    kMCUnicodeCategoryUnassigned,
    
    // Letters
    kMCUnicodeCategoryGeneralOther,
    kMCUnicodeCategoryUppercaseLetter,
    kMCUnicodeCategoryLowercaseLetter,
    kMCUnicodeCategoryTitlecaseLetter,
    kMCUnicodeCategoryModifierLetter,
    kMCUnicodeCategoryOtherLetter,
    
    // Marks
    kMCUnicodeCategoryNonSpacingMark,
    kMCUnicodeCategoryEnclosingMark,
    kMCUnicodeCategoryCombiningSpaceMark,
    
    // Numerics
    kMCUnicodeCategoryDecimalDigitNumber,
    kMCUnicodeCategoryLetterNumber,
    kMCUnicodeCategoryOtherNumber,
    
    // Separators
    kMCUnicodeCategorySpaceSeparator,
    kMCUnicodeCategoryLineSeparator,
    kMCUnicodeCategoryParagraphSeparator,
    
    // Controls and misc
    kMCUnicodeCategoryControlChar,
    kMCUnicodeCategoryFormatChar,
    kMCUnicodeCategoryPrivateUseChar,
    kMCUnicodeCategorySurrogate,
    
    // Punctuation
    kMCUnicodeCategoryDashPunctuation,
    kMCUnicodeCategoryStartPunctuation,
    kMCUnicodeCategoryEndPunctuation,
    kMCUnicodeCategoryConnectorPunctuation,
    kMCUnicodeCategoryOtherPunctuation,
    kMCUnicodeCategoryInitialPunctuation,
    kMCUnicodeCategoryFinalPunctuation,
    
    // Symbols
    kMCUnicodeCategoryMathSymbol,
    kMCUnicodeCategoryCurrencySymbol,
    kMCUnicodeCategoryModifierSymbol,
    kMCUnicodeCategoryOtherSymbol
};

// Character direction classes
enum MCUnicodeDirection
{
    kMCUnicodeDirectionLeftToRight,
    kMCUnicodeDirectionRightToLeft,
    kMCUnicodeDirectionEuropeanNumber,
    kMCUnicodeDirectionEuropeanNumberSeparator,
    kMCUnicodeDirectionEuropeanNumberTerminator,
    kMCUnicodeDirectionArabicNumber,
    kMCUnicodeDirectionCommonNumberSeparator,
    kMCUnicodeDirectionBlockSeparator,
    kMCUnicodeDirectionSegmentSeparator,
    kMCUnicodeDirectionWhiteSpaceNeutral,
    kMCUnicodeDirectionOtherNeutral,
    kMCUnicodeDirectionLeftToRightEmbedding,
    kMCUnicodeDirectionLeftToRightOverride,
    kMCUnicodeDirectionRightToLeftArabic,
    kMCUnicodeDirectionRightToLeftEmbedding,
    kMCUnicodeDirectionRightToLeftOverride,
    kMCUnicodeDirectionPopDirectionalFormat,
    kMCUnicodeDirectionNonSpacingMark,
    kMCUnicodeDirectionBoundaryNeutral,
    kMCUnicodeDirectionFirstStrongIsolate,
    kMCUnicodeDirectionLeftToRightIsolate,
    kMCUnicodeDirectionRightToLeftIsolate,
    kMCUnicodeDirectionPopDirectionalIsolate
};

// Types used when retrieving properties
enum MCUnicodePropertyType
{
    kMCUnicodePropertyTypeBool,         // bool
    kMCUnicodePropertyTypeUint8,        // uint8_t
    kMCUnicodePropertyTypeUint16,       // uint16_t
    kMCUnicodePropertyTypeUint32,       // uint32_t
    kMCUnicodePropertyTypeFloat,        // float
    kMCUnicodePropertyTypeDouble,       // double
    kMCUnicodePropertyTypeCharacter,    // unichar_t
    kMCUnicodePropertyTypeString        // unichar_t[] (null-terminated)
};

////////////////////////////////////////////////////////////////////////////////

// Retrieves a character's property of the specified type. If the type does not
// match the property's actual type, the result is undefined. For string
// properties, the pointer is internal and should not be modified nor freed.
bool                MCUnicodeGetBinaryProperty(codepoint_t p_codepoint, MCUnicodeProperty);
int32_t             MCUnicodeGetIntegerProperty(codepoint_t p_codepoint, MCUnicodeProperty);
double              MCUnicodeGetFloatProperty(codepoint_t p_codepoint, MCUnicodeProperty);
codepoint_t         MCUnicodeGetCharacterProperty(codepoint_t p_codepoint, MCUnicodeProperty);
const unichar_t*    MCUnicodeGetStringProperty(codepoint_t p_codepoint, MCUnicodeProperty);

// Batch retrieval of character properties. For characters that are surrogates,
// both positions in the output array receive the same properties. If the type
// cannot be converted (e.g. incompatible or not representable) this call fails.
bool    MCUnicodeGetProperty(const unichar_t *p_chars, uindex_t p_char_count,
                             MCUnicodeProperty, MCUnicodePropertyType,
                             void *x_result_array);

// Returns the name of a Unicode property. The returned pointer is internal and
// should not be modified. (Unicode property value names are always ASCII).
const char*    MCUnicodeGetPropertyValueName(MCUnicodeProperty, int32_t p_prop_value);

////////////////////////////////////////////////////////////////////////////////

// Shortcuts to (approximate) equivalents of the standard is_* ctype functions
bool    MCUnicodeIsAlphabetic(codepoint_t);
bool    MCUnicodeIsLowercase(codepoint_t);
bool    MCUnicodeIsUppercase(codepoint_t);
bool    MCUnicodeIsTitlecase(codepoint_t);
bool    MCUnicodeIsWhitespace(codepoint_t);
bool    MCUnicodeIsPunctuation(codepoint_t);
bool    MCUnicodeIsDigit(codepoint_t);
bool    MCUnicodeIsHexDigit(codepoint_t);
bool    MCUnicodeIsAlnum(codepoint_t);
bool    MCUnicodeIsBlank(codepoint_t);
bool    MCUnicodeIsControl(codepoint_t);
bool    MCUnicodeIsGraphic(codepoint_t);
bool    MCUnicodeIsPrinting(codepoint_t);

// Shortcuts for identifier name properties
bool    MCUnicodeIsIdentifierInitial(codepoint_t);      // Can begin an identifier
bool    MCUnicodeIsIdentifierContinue(codepoint_t);     // Can continue an identifier


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

bool    MCUnicodeNormaliseNFC(const unichar_t* p_in, uindex_t p_in_length,
                              unichar_t* &r_out, uindex_t &r_out_length);
bool    MCUnicodeNormaliseNFKC(const unichar_t* p_in, uindex_t p_in_length,
                               unichar_t* &r_out, uindex_t &r_out_length);
bool    MCUnicodeNormaliseNFD(const unichar_t* p_in, uindex_t p_in_length,
                              unichar_t* &r_out, uindex_t &r_out_length);
bool    MCUnicodeNormaliseNFKD(const unichar_t* p_in, uindex_t p_in_length,
                               unichar_t* &r_out, uindex_t &r_out_length);

////////////////////////////////////////////////////////////////////////////////

bool    MCUnicodeIsNormalisedNFC(const unichar_t *p_string, uindex_t p_length);
bool    MCUnicodeIsNormalisedNFKC(const unichar_t *p_string, uindex_t p_length);
bool    MCUnicodeIsNormalisedNFD(const unichar_t *p_string, uindex_t p_length);
bool    MCUnicodeIsNormalisedNFKD(const unichar_t *p_string, uindex_t p_length);


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////


// Casing options are locale-dependent
bool    MCUnicodeLowercase(MCLocaleRef,
                           const unichar_t *p_in, uindex_t p_in_length,
                           unichar_t* &r_out, uindex_t &r_out_length);
bool    MCUnicodeUppercase(MCLocaleRef,
                           const unichar_t *p_in, uindex_t p_in_length,
                           unichar_t* &r_out, uindex_t &r_out_length);
bool    MCUnicodeTitlecase(MCLocaleRef,
                                   const unichar_t *p_in, uindex_t p_in_length,
                                   unichar_t* &r_out, uindex_t &r_out_length);

// Case folding does not depend on locale
bool    MCUnicodeCaseFold(const unichar_t *p_in, uindex_t p_in_length,
                                unichar_t* &r_out, uindex_t &r_out_length);


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////


// Comparison options
enum MCUnicodeCompareOption
{
    kMCUnicodeCompareOptionExact = kMCStringOptionCompareExact,       // Codepoint (not code unit!) equality
	kMCUnicodeCompareOptionNormalised = kMCStringOptionCompareNonliteral,  // Normalise inputs before comparison
	kMCUnicodeCompareOptionFolded = kMCStringOptionCompareFolded,      // Case fold inputs before comparison
    kMCUnicodeCompareOptionCaseless = kMCStringOptionCompareCaseless,    // Both normalise and case fold
};

////////////////////////////////////////////////////////////////////////////////

// Compares two strings using the locale-independent collator
int32_t MCUnicodeCompare(const void *p_first, uindex_t p_first_length, bool p_first_native,
                         const void *p_second, uindex_t p_second_length, bool p_second_native,
                         MCUnicodeCompareOption);

// Returns whether the first string begins with the second
bool MCUnicodeBeginsWith(const void *p_first, uindex_t p_first_length, bool p_first_native,
                         const void *p_second, uindex_t p_second_length, bool p_second_native,
                         MCUnicodeCompareOption, uindex_t *r_first_match_length);

// Returns whether the first string ends with the second
bool MCUnicodeEndsWith(const void *p_first, uindex_t p_first_length, bool p_first_native,
                       const void *p_second, uindex_t p_second_length, bool p_second_native,
                       MCUnicodeCompareOption, uindex_t *r_first_match_length);

// Returns whether the string contains the given substring
bool MCUnicodeContains(const void *p_string, uindex_t p_string_length, bool p_string_native,
                       const void *p_needle, uindex_t p_needle_length, bool p_needle_native,
                       MCUnicodeCompareOption);

// Returns the index of the first occurence of the substring in the given string
bool MCUnicodeFirstIndexOf(const void *p_string, uindex_t p_string_length, bool p_string_native,
                           const void *p_needle, uindex_t p_needle_length, bool p_needle_native,
                           MCUnicodeCompareOption, uindex_t &r_index);

// Returns the index of the last occurence of the substring in the given string
bool MCUnicodeLastIndexOf(const void *p_string, uindex_t p_string_length, bool p_string_native,
                          const void *p_needle, uindex_t p_needle_length, bool p_needle_native,
                          MCUnicodeCompareOption, uindex_t &r_index);

// Returns the index of the first occurence of the codepoint in the given string
bool MCUnicodeFirstIndexOfChar(const unichar_t *p_string, uindex_t p_string_length,
                               codepoint_t p_needle, MCUnicodeCompareOption, uindex_t &r_index);

// Returns the index of the last occurence of the codepoint in the given string
bool MCUnicodeLastIndexOfChar(const unichar_t *p_string, uindex_t p_string_length,
                              codepoint_t p_needle, MCUnicodeCompareOption, uindex_t &r_index);

// Returns the length in both sequences of a matching prefix
void MCUnicodeSharedPrefix(const void *p_string, uindex_t p_string_length, bool p_string_native,
                           const void *p_needle, uindex_t p_needle_length, bool p_needle_native,
                           MCUnicodeCompareOption p_option, uindex_t &r_len_in_string, uindex_t &r_len_in_prefix);

// Returns the length in both sequences of a matching suffix
void MCUnicodeSharedSuffix(const void *p_string, uindex_t p_string_length, bool p_string_native,
                           const void *p_needle, uindex_t p_needle_length, bool p_needle_native,
                           MCUnicodeCompareOption p_option, uindex_t &r_len_in_string, uindex_t &r_len_in_suffix);

// Searches a string for a given substring and returns the range that was equal
// to the substring (note that this may be a different length to the substring!)
bool MCUnicodeFind(const void *p_string, uindex_t p_string_length, bool p_string_native,
                   const void *p_needle, uindex_t p_needle_length, bool p_needle_native,
                   MCUnicodeCompareOption p_option, MCRange &r_matched_range);

// Hashes the given string, ignoring case or normalisation differences if
// requested (i.e the string is folded or normalised before hashing)
hash_t MCUnicodeHash(const unichar_t *p_string, uindex_t p_string_length,
                     MCUnicodeCompareOption);

// Returns true if source_chars are a match for pattern_chars under the rules
// of LiveCode wildcard matches.
bool MCUnicodeWildcardMatch(const void *source_chars, uindex_t source_length, bool p_source_native, const void *pattern_chars, uindex_t pattern_length, bool p_pattern_native, MCUnicodeCompareOption p_option);

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////


// Collation options to use when comparing strings
// The interpretation of strength values is locale specific but is roughly this:
//
//  Primary:    Base characters
//  Secondary:  Accents
//  Tertiary:   Case differences and variants
//  Quaternary: Punctuation (when punctuation is ignored at higher levels)
//  Identical:  Tie breaker if all others are identical
//
//  kMCUnicodeCollateOptionAutoNormalise:
//      Normalise the text to NFD before performing the comparison
//
//  kMCUnicodeCollateOptionNumeric:
//      Compare strings of digits by numeric value rather than lexical order
//
//  kMCUnicodeCollateOptionIgnorePunctuation:
//      Ignore punctuation differences (e.g. spaces, hyphens) when comparing
enum MCUnicodeCollateOption
{
    // Use the locale's default collation options
    kMCUnicodeCollateOptionDefault              = 0,
    
    kMCUnicodeCollateOptionStrengthPrimary      = 1,
    kMCUnicodeCollateOptionStrengthSecondary    = 2,
    kMCUnicodeCollateOptionStrengthTertiary     = 3,
    kMCUnicodeCollateOptionStrengthQuaternary   = 4,
    kMCUnicodeCollateOptionStrengthIdentical    = 15,
    
    kMCUnicodeCollateOptionStrengthMask         = 0x0F,
    
    kMCUnicodeCollateOptionAutoNormalise        = 0x10,
    kMCUnicodeCollateOptionNumeric              = 0x20,
    kMCUnicodeCollateOptionIgnorePunctuation    = 0x40
};

////////////////////////////////////////////////////////////////////////////////

// Returns -ve, 0, +ve if first string is <, ==, > second string respectively.
// If many comparisons are expected, it may be faster to create and compare sort
// keys rather than doing direct string comparisons.
int32_t MCUnicodeCollate(MCLocaleRef, MCUnicodeCollateOption,
                            const unichar_t* p_first, uindex_t p_first_len,
                            const unichar_t* p_second, uindex_t p_second_len);

// Returns the appropriate collation options from the string comparison type.
MCUnicodeCollateOption MCUnicodeCollateOptionFromCompareOption(MCUnicodeCompareOption p_option);

// Creates a sort key using the given collator that can be used for simple
// binary comparisons of strings. In general, the sort key will be distinct from
// the string itself and cannot be used to recover the source string.
//
// IMPORTANT:
//      Sort keys are not stable across Unicode versions and should therefore
//      not be stored in stacks. Store the source string instead.
bool    MCUnicodeCreateSortKey(MCLocaleRef, MCUnicodeCollateOption,
                               const unichar_t* p_in, uindex_t p_in_length,
                               byte_t* &r_out, uindex_t &r_out_length);

/////

// The collator reference - this is not a valueref!
typedef void *MCUnicodeCollatorRef;

// Creates a collation object which can be reused.
bool MCUnicodeCreateCollator(MCLocaleRef locale, MCUnicodeCollateOption options, MCUnicodeCollatorRef& r_collator);

// Destroys a previously create collator.
void MCUnicodeDestroyCollator(MCUnicodeCollatorRef collator);

int32_t MCUnicodeCollateWithCollator(MCUnicodeCollatorRef collator,
                                     const unichar_t* p_first, uindex_t p_first_len,
                                     const unichar_t* p_second, uindex_t p_second_len);

// Create a sort key using the given collator.
bool MCUnicodeCreateSortKeyWithCollator(MCUnicodeCollatorRef collator,
                                        const unichar_t* p_in, uindex_t p_in_length,
                                        byte_t* &r_out, uindex_t &r_out_length);

////////////////////////////////////////////////////////////////////////////////

// SN-2014-04-16
// Relocating of the engine/src/unicode.h here as they are now needed by foundation-bidi

// Returns true if the given codepoint has a non-zero combining class, this
// is not inlined as it requires table lookup and looping.
extern bool MCUnicodeCodepointIsCombiner(uinteger_t x);

extern uinteger_t MCUnicodeMapFromNativeSingleton_Windows1252(unsigned char x);
extern uinteger_t MCUnicodeMapFromNativeSingleton_MacRoman(unsigned char x);
extern uinteger_t MCUnicodeMapFromNativeSingleton_ISO8859_1(unsigned char x);

extern bool MCUnicodeMapToNativeSingleton_Windows1252(uinteger_t x, char_t& r_char);
extern bool MCUnicodeMapToNativeSingleton_MacRoman(uinteger_t x, char_t& r_char);
extern bool MCUnicodeMapToNativeSingleton_ISO8859_1(uinteger_t x, char_t& r_char);

extern bool MCUnicodeMapToNativePair_Windows1252(uinteger_t x, uinteger_t y, char_t& r_char);
extern bool MCUnicodeMapToNativePair_MacRoman(uinteger_t x, uinteger_t y, char_t& r_char);
extern bool MCUnicodeMapToNativePair_ISO8859_1(uinteger_t x, uinteger_t y, char_t& r_char);

extern bool MCUnicodeCodepointIsIdeographicLookup(uinteger_t x);

// Returns true if there's a word break between x and y with the given context.
// Note that this is only a crude approximation to the full Unicode rules.
extern bool MCUnicodeCanBreakWordBetween(uinteger_t xc, uinteger_t x, uinteger_t y, uinteger_t yc);

////////////////////////////////////////////////////////////////////////////////

// Returns true if the given codepoint should be treated as ideographic when
// breaking
inline bool MCUnicodeCodepointIsIdeographic(uinteger_t x)
{
	if (x < 0x0E01)
		return false;
	return MCUnicodeCodepointIsIdeographicLookup(x);
}

// Returns the break class of the codepoint. If Bit 0 is set, breaking is
// prohibited before, it Bit 1 is set, breaking is prohibited after
extern uinteger_t MCUnicodeCodepointGetBreakClass(uinteger_t x);

// Returns true if the given codepoint is in the high surrogate area
inline bool MCUnicodeCodepointIsLeadingSurrogate(codepoint_t x)
{
	if (x < 0xD800)
		return false;
    
	if (x > 0xDBFF)
		return false;
    
	return true;
}

inline bool MCUnicodeCodepointIsHighSurrogate(uinteger_t x)
{
	return MCUnicodeCodepointIsLeadingSurrogate(x);
}


// Returns true if the given codepoint is in the low surrogate area
inline bool MCUnicodeCodepointIsTrailingSurrogate(codepoint_t x)
{
	if (x < 0xDC00)
		return false;
	if (x > 0xDFFF)
		return false;
	return true;
}

inline bool MCUnicodeCodepointIsLowSurrogate(uinteger_t x)
{
	return MCUnicodeCodepointIsTrailingSurrogate(x);
}

inline uindex_t MCUnicodeCodepointGetCodeunitLength(codepoint_t cp)
{
	if (cp > UNICHAR_MAX)
		return 2;
	return 1;
}

// Returns true if the given codepoint is a base character
inline bool MCUnicodeCodepointIsBase(uinteger_t x)
{
	if (x < 0x0300)
		return true;
    
	return !MCUnicodeCodepointIsCombiner(x);
}

// Returns the codepoint formed by combining a low and high surrogate
inline codepoint_t MCUnicodeCombineSurrogates(unichar_t p_leading, unichar_t p_trailing)
{
	return 0x10000U + (((p_leading - 0xD800U) << 10) | (p_trailing - 0xDC00U));
}

// Compute and advance the current surrogate pair (used by MCUnicodeCodepointAdvance to
// help the compiler make good choices about inlining - effectively a 'trap' to a very
// rare case).
inline uinteger_t MCUnicodeCodepointAdvanceSurrogate(const unichar_t* p_input, uinteger_t p_length, uinteger_t& x_index)
{
	if (x_index + 1 < p_length && MCUnicodeCodepointIsLowSurrogate(p_input[1]))
	{
        // FG-2014-10-23: [[ Bugfix 13761 ]] Codepoint was calculated incorrectly
        uinteger_t t_codepoint;
		t_codepoint = MCUnicodeCombineSurrogates(p_input[x_index], p_input[x_index + 1]);
		x_index += 2;
		return t_codepoint;
	}
    
	uinteger_t t_codepoint;
	t_codepoint = p_input[x_index];
    
	x_index += 1;
    
	return t_codepoint;
}

// Consume and return the next codepoint - this automatically combines surrogates. If the
// surrogate is invalid it is still returned (i.e. the expectation is it will be processed
// as an illegal character).
inline uinteger_t MCUnicodeCodepointAdvance(const unichar_t* p_input, uinteger_t p_length, uinteger_t& x_index)
{
	uinteger_t t_codepoint;
	t_codepoint = p_input[x_index];
    
	if (MCUnicodeCodepointIsHighSurrogate(t_codepoint))
		return MCUnicodeCodepointAdvanceSurrogate(p_input, p_length, x_index);
    
	x_index += 1;
    
	return t_codepoint;
}

////////////////////////////////////////////////////////////////////////////////

inline bool MCUnicodeMapToNative_Windows1252(const unichar_t *p_codepoints, uinteger_t p_length, char_t& r_char)
{
	if (p_length == 1)
	{
		if (p_codepoints[0] <= 0x7F || (p_codepoints[0] >= 0xA0 && p_codepoints[0] <= 0xFF))
		{
			r_char = (char_t)p_codepoints[0];
			return true;
		}
		return MCUnicodeMapToNativeSingleton_Windows1252(p_codepoints[0], r_char);
	}
    
	if (p_length == 2)
	{
		if (p_codepoints[0] < 'A' || p_codepoints[0] > 'z')
			return false;
        
		if (p_codepoints[1] < 0x0300 || p_codepoints[1] > 0x0327)
			return false;
        
		return MCUnicodeMapToNativePair_Windows1252(p_codepoints[0], p_codepoints[1], r_char);
	}
    
	return false;
}

inline bool MCUnicodeMapToNative_MacRoman(const unichar_t *p_codepoints, uinteger_t p_length, char_t& r_char)
{
	if (p_length == 1)
	{
		if (p_codepoints[0] <= 0x7f)
		{
			r_char = (char_t)p_codepoints[0];
			return true;
		}
		
		return MCUnicodeMapToNativeSingleton_MacRoman(p_codepoints[0], r_char);
	}
	
	if (p_length == 2)
	{
		if (p_codepoints[0] < 'A' || p_codepoints[0] > 'z')
			return false;
        
		if (p_codepoints[1] < 0x0300 || p_codepoints[1] > 0x0327)
			return false;
        
		return MCUnicodeMapToNativePair_MacRoman(p_codepoints[0], p_codepoints[1], r_char);
	}
    
	return false;
}

inline bool MCUnicodeMapToNative_ISO8859_1(const unichar_t *p_codepoints, uinteger_t p_length, char_t& r_char)
{
	if (p_length == 1)
	{
		if (p_codepoints[0] <= 0xff)
		{
			r_char = (char_t)p_codepoints[0];
			return true;
		}
		
		return false;
	}
    
	if (p_length == 2)
	{
		if (p_codepoints[0] < 'A' || p_codepoints[0] > 'y')
			return false;
        
		if (p_codepoints[1] < 0x0300 || p_codepoints[1] > 0x0327)
			return false;
        
		return MCUnicodeMapToNativePair_ISO8859_1(p_codepoints[0], p_codepoints[1], r_char);
	}
    
	return false;
}

////////////////////////////////////////////////////////////////////////////////

inline uinteger_t MCUnicodeMapFromNative_Windows1252(char_t p_native)
{
	if (p_native <= 0x7F)
		return p_native;
	if (p_native >= 0xA0)
		return p_native;
	return MCUnicodeMapFromNativeSingleton_Windows1252(p_native);
}

inline uinteger_t MCUnicodeMapFromNative_MacRoman(char_t p_native)
{
	if (p_native <= 0x7F)
		return p_native;
	return MCUnicodeMapFromNativeSingleton_MacRoman(p_native);
}

inline uinteger_t MCUnicodeMapFromNative_ISO8859_1(char_t p_native)
{
	return p_native;
}

////////////////////////////////////////////////////////////////////////////////

#if defined(__WINDOWS_1252__)
#define MCUnicodeMapFromNative(x) MCUnicodeMapFromNative_Windows1252(x)
#define MCUnicodeMapToNative(x, y, z) MCUnicodeMapToNative_Windows1252(x, y, z)
#elif defined(__MACROMAN__)
#define MCUnicodeMapFromNative(x) MCUnicodeMapFromNative_MacRoman(x)
#define MCUnicodeMapToNative(x, y, z) MCUnicodeMapToNative_MacRoman(x, y, z)
#elif defined(__ISO_8859_1__)
#define MCUnicodeMapFromNative(x) MCUnicodeMapFromNative_ISO8859_1(x)
#define MCUnicodeMapToNative(x, y, z) MCUnicodeMapToNative_ISO8859_1(x, y, z)
#else
#error Unknown native text encoding.
#endif

////////////////////////////////////////////////////////////////////////////////

bool MCUnicodeIsGraphemeClusterBoundary(codepoint_t p_left, codepoint_t p_right);

////////////////////////////////////////////////////////////////////////////////

#endif  /* ifndef __MC_FOUNDATION_UNICODE__ */
