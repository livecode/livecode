/* Copyright (C) 2013 Runtime Revolution Ltd.
 
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

codepoint_t MCUnicodeSurrogatesToCodepoint(uint16_t first, uint16_t second);
// We assume that the unichar_t pointer already has enough memory to handle the addition of the surrogate pair
// Returns true in case the codepoint actually generated a surrogate pair
bool MCUnicodeCodepointToSurrogates(codepoint_t t_codepoint, unichar_t* r_surrogates);

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
    kMCUnicodeCompareOptionExact = 0,       // Codepoint (not code unit!) equality
    kMCUnicodeCompareOptionNormalised = 1,  // Normalise inputs before comparison
    kMCUnicodeCompareOptionCaseless = 2,    // Both normalise and case fold
    kMCUnicodeCompareOptionFolded = 3,      // Case fold inputs before comparison
};

////////////////////////////////////////////////////////////////////////////////

// Compares two strings using the locale-independent collator
int32_t MCUnicodeCompare(const unichar_t *p_first, uindex_t p_first_length,
                         const unichar_t *p_second, uindex_t p_second_length,
                         MCUnicodeCompareOption);

// Returns whether the first string begins with the second
bool MCUnicodeBeginsWith(const unichar_t *p_first, uindex_t p_first_length,
                         const unichar_t *p_second, uindex_t p_second_length,
                         MCUnicodeCompareOption);

// Returns whether the first string ends with the second
bool MCUnicodeEndsWith(const unichar_t *p_first, uindex_t p_first_length,
                       const unichar_t *p_second, uindex_t p_second_length,
                       MCUnicodeCompareOption);

// Returns whether the string contains the given substring
bool MCUnicodeContains(const unichar_t *p_string, uindex_t p_string_length,
                       const unichar_t *p_needle, uindex_t p_needle_length,
                       MCUnicodeCompareOption);

// Returns the index of the first occurence of the substring in the given string
bool MCUnicodeFirstIndexOf(const unichar_t *p_string, uindex_t p_string_length,
                           const unichar_t *p_needle, uindex_t p_needle_length,
                           MCUnicodeCompareOption, uindex_t &r_index);

// Returns the index of the last occurence of the substring in the given string
bool MCUnicodeLastIndexOf(const unichar_t *p_string, uindex_t p_string_length,
                          const unichar_t *p_needle, uindex_t p_needle_length,
                          MCUnicodeCompareOption, uindex_t &r_index);

// Returns the index of the first occurence of the codepoint in the given string
bool MCUnicodeFirstIndexOfChar(const unichar_t *p_string, uindex_t p_string_length,
                               codepoint_t p_needle, MCUnicodeCompareOption, uindex_t &r_index);

// Returns the index of the last occurence of the codepoint in the given string
bool MCUnicodeLastIndexOfChar(const unichar_t *p_string, uindex_t p_string_length,
                              codepoint_t p_needle, MCUnicodeCompareOption, uindex_t &r_index);

// Returns the length in both sequences of a matching prefix
void MCUnicodeSharedPrefix(const unichar_t *p_string, uindex_t p_string_length,
                           const unichar_t *p_prefix, uindex_t p_prefix_length,
                           MCUnicodeCompareOption p_option, uindex_t &r_len_in_string, uindex_t &r_len_in_prefix);

// Returns the length in both sequences of a matching suffix
void MCUnicodeSharedSuffix(const unichar_t *p_string, uindex_t p_string_length,
                           const unichar_t *p_prefix, uindex_t p_prefix_length,
                           MCUnicodeCompareOption p_option, uindex_t &r_len_in_string, uindex_t &r_len_in_prefix);

// Searches a string for a given substring and returns the range that was equal
// to the substring (note that this may be a different length to the substring!)
bool MCUnicodeFind(const unichar_t *p_string, uindex_t p_string_length,
                   const unichar_t *p_needle, uindex_t p_needle_length,
                   MCUnicodeCompareOption, MCRange &r_matched_range);

// Hashes the given string, ignoring case or normalisation differences if
// requested (i.e the string is folded or normalised before hashing)
hash_t MCUnicodeHash(const unichar_t *p_string, uindex_t p_string_length,
                     MCUnicodeCompareOption);


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


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////



#endif  /* ifndef __MC_FOUNDATION_UNICODE__ */
