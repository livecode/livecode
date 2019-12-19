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

#include "foundation-unicode.h"
#include "foundation-unicode-private.h"

#include "unicode/uchar.h"
#include "unicode/ustring.h"
#include "unicode/brkiter.h"
#include "unicode/coll.h"
#include "unicode/normalizer2.h"
#include "unicode/udata.h"
#include "unicode/uclean.h"

#include "foundation-auto.h"
#include "foundation-text.h"
#include "foundation-string-hash.h"

#include <limits>


// This array converts from the MCUnicodeProperty enumeration to the corresponding
// UProperty value used by the ICU library.
//
// These *must* be in the same order as the MCUnicodeProperty enumeration
const UProperty MCUnicodePropToICUProp[] =
{
    UCHAR_ALPHABETIC,
    UCHAR_ASCII_HEX_DIGIT,
    UCHAR_BIDI_CONTROL,
    UCHAR_BIDI_MIRRORED,
    UCHAR_DASH,
    UCHAR_DEFAULT_IGNORABLE_CODE_POINT,
    UCHAR_DEPRECATED,
    UCHAR_DIACRITIC,
    UCHAR_EXTENDER,
    UCHAR_FULL_COMPOSITION_EXCLUSION,
    UCHAR_GRAPHEME_BASE,
    UCHAR_GRAPHEME_EXTEND,
    UCHAR_GRAPHEME_LINK,
    UCHAR_HEX_DIGIT,
    UCHAR_HYPHEN,
    UCHAR_ID_CONTINUE,
    UCHAR_ID_START,
    UCHAR_IDEOGRAPHIC,
    UCHAR_IDS_BINARY_OPERATOR,
    UCHAR_IDS_TRINARY_OPERATOR,
    UCHAR_JOIN_CONTROL,
    UCHAR_LOGICAL_ORDER_EXCEPTION,
    UCHAR_LOWERCASE,
    UCHAR_MATH,
    UCHAR_NONCHARACTER_CODE_POINT,
    UCHAR_QUOTATION_MARK,
    UCHAR_RADICAL,
    UCHAR_SOFT_DOTTED,
    UCHAR_TERMINAL_PUNCTUATION,
    UCHAR_UNIFIED_IDEOGRAPH,
    UCHAR_UPPERCASE,
    UCHAR_WHITE_SPACE,
    UCHAR_XID_CONTINUE,
    UCHAR_XID_START,
    UCHAR_CASE_SENSITIVE,
    UCHAR_S_TERM,
    UCHAR_VARIATION_SELECTOR,
    UCHAR_NFD_INERT,
    UCHAR_NFKD_INERT,
    UCHAR_NFC_INERT,
    UCHAR_NFKC_INERT,
    UCHAR_SEGMENT_STARTER,
    UCHAR_PATTERN_SYNTAX,
    UCHAR_PATTERN_WHITE_SPACE,
    UCHAR_POSIX_ALNUM,
    UCHAR_POSIX_BLANK,
    UCHAR_POSIX_GRAPH,
    UCHAR_POSIX_PRINT,
    UCHAR_POSIX_XDIGIT,
    UCHAR_CASED,
    UCHAR_CASE_IGNORABLE,
    UCHAR_CHANGES_WHEN_LOWERCASED,
    UCHAR_CHANGES_WHEN_UPPERCASED,
    UCHAR_CHANGES_WHEN_TITLECASED,
    UCHAR_CHANGES_WHEN_CASEFOLDED,
    UCHAR_CHANGES_WHEN_CASEMAPPED,
    UCHAR_CHANGES_WHEN_NFKC_CASEFOLDED,
    
    UCHAR_BIDI_CLASS,
    UCHAR_BLOCK,
    UCHAR_CANONICAL_COMBINING_CLASS,
    UCHAR_DECOMPOSITION_TYPE,
    UCHAR_EAST_ASIAN_WIDTH,
    UCHAR_GENERAL_CATEGORY,
    UCHAR_JOINING_GROUP,
    UCHAR_JOINING_TYPE,
    UCHAR_LINE_BREAK,
    UCHAR_NUMERIC_TYPE,
    UCHAR_SCRIPT,
    UCHAR_HANGUL_SYLLABLE_TYPE,
    UCHAR_NFD_QUICK_CHECK,
    UCHAR_NFKD_QUICK_CHECK,
    UCHAR_NFC_QUICK_CHECK,
    UCHAR_NFKC_QUICK_CHECK,
    UCHAR_LEAD_CANONICAL_COMBINING_CLASS,
    UCHAR_TRAIL_CANONICAL_COMBINING_CLASS,
    UCHAR_GRAPHEME_CLUSTER_BREAK,
    UCHAR_SENTENCE_BREAK,
    UCHAR_WORD_BREAK,
    UCHAR_BIDI_PAIRED_BRACKET_TYPE,
    
    UCHAR_GENERAL_CATEGORY_MASK,
    
    UCHAR_NUMERIC_VALUE,
    
    UCHAR_BIDI_MIRRORING_GLYPH,
    UCHAR_SIMPLE_CASE_FOLDING,
    UCHAR_SIMPLE_LOWERCASE_MAPPING,
    UCHAR_SIMPLE_TITLECASE_MAPPING,
    UCHAR_SIMPLE_UPPERCASE_MAPPING,
    UCHAR_BIDI_PAIRED_BRACKET,
    
    UCHAR_AGE,
    UCHAR_CASE_FOLDING,
    UCHAR_ISO_COMMENT,
    UCHAR_LOWERCASE_MAPPING,
    UCHAR_NAME,
    UCHAR_TITLECASE_MAPPING,
    UCHAR_UNICODE_1_NAME,
    UCHAR_UPPERCASE_MAPPING
};

////////////////////////////////////////////////////////////////////////////////

extern unsigned char s_icudata[];
bool __MCUnicodeInitialize()
{
    bool t_success = true;

    UErrorCode t_error = U_ZERO_ERROR;
    udata_setCommonData(s_icudata, &t_error);
    t_success = t_error == U_ZERO_ERROR;

    // FG-2014-07-28: [[ Bugfix 12974 ]]
    // This is required to work around an ICU crashing bug in 52.1 - according
    // to the ICU docs, it is completely un-necessary. It also only happens in
    // Linux standalones, strangely.
    if (t_success)
    {
        u_init(&t_error);
        t_success = t_error == U_ZERO_ERROR;
    }

    return t_success;
}

void __MCUnicodeFinalize()
{
    ;
}

////////////////////////////////////////////////////////////////////////////////

bool MCUnicodeGetBinaryProperty(codepoint_t p_codepoint, MCUnicodeProperty p_property)
{
    return !!u_hasBinaryProperty(p_codepoint, MCUnicodePropToICUProp[p_property]);
}

int32_t MCUnicodeGetIntegerProperty(codepoint_t p_codepoint, MCUnicodeProperty p_property)
{
    return u_getIntPropertyValue(p_codepoint, MCUnicodePropToICUProp[p_property]);
}

double MCUnicodeGetFloatProperty(codepoint_t p_codepoint, MCUnicodeProperty p_property)
{
    if (p_property != kMCUnicodePropertyNumericValue)
		return std::numeric_limits<double>::quiet_NaN();
    return u_getNumericValue(p_codepoint);
}

codepoint_t MCUnicodeGetCharacterProperty(codepoint_t p_codepoint, MCUnicodeProperty p_property)
{
    // Only a subset of the string properties are supported here
    switch (p_property)
    {
        case kMCUnicodePropertyBidiMirroringGlyph:
            return u_charMirror(p_codepoint);
            
        case kMCUnicodePropertyBidiPairedBracket:
            return u_getBidiPairedBracket(p_codepoint);
            
        case kMCUnicodePropertySimpleLowercaseMapping:
            return u_tolower(p_codepoint);
            
        case kMCUnicodePropertySimpleUppercaseMapping:
            return u_toupper(p_codepoint);
            
        case kMCUnicodePropertySimpleTitlecaseMapping:
            return u_totitle(p_codepoint);
            
        case kMCUnicodePropertySimpleCaseFolding:
            return u_foldCase(p_codepoint, U_FOLD_CASE_DEFAULT);
        
        default:
            return ~codepoint_t(0);
    }
}

const unichar_t* MCUnicodeGetStringProperty(codepoint_t p_codepoint, MCUnicodeProperty p_property)
{
    // This function is really hacky and unpleasant and could do with fixing
    
    // Delete any pre-existing string we have
    // NOTE: this is not thread-safe nor re-entrant
    static unichar_t *s_pointer = nil;
    if (s_pointer != nil)
        delete[] s_pointer;
    
    // Which property do we want?
    UErrorCode t_error = U_ZERO_ERROR;
    switch (p_property)
    {
        case kMCUnicodePropertyAge:
            // NOT YET SUPPORTED
            s_pointer = nil;
            break;
            
        case kMCUnicodePropertyCaseFolding:
        {
            // Case-fold the codepoint
            s_pointer = new (nothrow) unichar_t[16];
            icu::UnicodeString t_string;
            t_string.append(UChar32(p_codepoint));
            t_string.foldCase();
            t_string.extract(s_pointer, 16, t_error);
            break;
        }
            
        case kMCUnicodePropertyISOComment:
            // The comment string is always empty
            s_pointer = nil;
            break;
            
        case kMCUnicodePropertyLowercaseMapping:
        {
            // Lowercase the codepoint
            s_pointer = new (nothrow) unichar_t[16];
            icu::UnicodeString t_string;
            t_string.append(UChar32(p_codepoint));
            t_string.toLower();
            t_string.extract(s_pointer, 16, t_error);
            break;
        }
            
        case kMCUnicodePropertyName:
        {
            // We assume that this is sufficient for a character name
            s_pointer = new (nothrow) unichar_t[256];
            char *t_temp = new (nothrow) char[256];
            uindex_t t_length;
            
            t_length = u_charName(p_codepoint, U_UNICODE_CHAR_NAME, t_temp, 255, &t_error);
            
            // The name is in ASCII but we want UTF-16
            for (uindex_t i = 0; i < t_length; i++)
            {
                s_pointer[i] = t_temp[i];
            }
            s_pointer[t_length] = 0;
            
            delete[] t_temp;
            break;
        }
            
            
        case kMCUnicodePropertyTitlecaseMapping:
        {
            // NOT YET SUPPORTED
            s_pointer = nil;
            break;
        }
            
        case kMCUnicodePropertyUnicode1Name:
        {
            // We assume that this is sufficient for a character name
            s_pointer = new (nothrow) unichar_t[256];
            char *t_temp = new (nothrow) char[256];
            uindex_t t_length;
            
            t_length = u_charName(p_codepoint, U_UNICODE_10_CHAR_NAME, t_temp, 255, &t_error);
            
            // The name is in ASCII but we want UTF-16
            for (uindex_t i = 0; i < t_length; i++)
            {
                s_pointer[i] = t_temp[i];
            }
            s_pointer[t_length] = 0;
            
            delete[] t_temp;
            break;
        }
            
        case kMCUnicodePropertyUppercaseMapping:
        {
            // Uppercase the codepoint
            s_pointer = new (nothrow) unichar_t[16];
            icu::UnicodeString t_string;
            t_string.append(UChar32(p_codepoint));
            t_string.toUpper();
            t_string.extract(s_pointer, 16, t_error);
            break;
        }
            
        default:
            s_pointer = nil;
    }
    
    return s_pointer;
}

bool MCUnicodeGetProperty(const unichar_t *p_chars, uindex_t p_char_count, MCUnicodeProperty p_property,
                          MCUnicodePropertyType p_property_type, void *x_result_array)
{
    // Scan through the character string
    uindex_t t_offset = 0;
    while (t_offset < p_char_count)
    {
        // Is this a simple character or a surrogate?
        // If the surrogate is not valid, just ignore the error
        codepoint_t t_char = p_chars[t_offset];
        uindex_t t_advance = 1;
        if (MCUnicodeCodepointIsLeadingSurrogate(t_char) &&
            (t_offset + 1) < p_char_count &&
            MCUnicodeCodepointIsTrailingSurrogate(p_chars[t_offset + 1]))
        {
            t_char = MCUnicodeCombineSurrogates(t_char, p_chars[t_offset + 1]);
            t_advance = 2;
        }
        
        // Look up the property
        //
        // The general behaviour of this is to retrieve the property at its full
        // precision and then attempt to convert it to the requested type. If
        // the conversion would alter the value, it is rejected and the function
        // call fails. Otherwise it moves on to the next character.
        //
        // When UTF-16 surrogate pairs are encountered, the slots in the result
        // array for the upper and lower surrogates are set to the same value
        // (that being the value of the property for the corresponding Unicode
        // codepoint rather than the surrogates as individual values). 
        switch (p_property_type)
        {
            case kMCUnicodePropertyTypeBool:
            {
                bool t_prop;
                t_prop = MCUnicodeGetBinaryProperty(t_char, p_property);
                ((bool*)x_result_array)[t_offset] = t_prop;
                if (t_advance == 2)
                    ((bool*)x_result_array)[t_offset + 1] = t_prop;
                break;
            }
                
            case kMCUnicodePropertyTypeUint8:
            {
                int32_t t_prop;
                t_prop = MCUnicodeGetIntegerProperty(t_char, p_property);
                if (t_prop < 0 || uint32_t(t_prop) > UINT8_MAX)
                    return false;
                ((uint8_t*)x_result_array)[t_offset] = uint8_t(t_prop);
                if (t_advance == 2)
                    ((uint8_t*)x_result_array)[t_offset + 1] = uint8_t(t_prop);
                break;
            }
                
            case kMCUnicodePropertyTypeUint16:
            {
                int32_t t_prop;
                t_prop = MCUnicodeGetIntegerProperty(t_char, p_property);
                if (t_prop < 0 || uint32_t(t_prop) > UINT16_MAX)
                    return false;
                ((uint16_t*)x_result_array)[t_offset] = uint16_t(t_prop);
                if (t_advance == 2)
                    ((uint16_t*)x_result_array)[t_offset + 1] = uint16_t(t_prop);
                break;
            }
                
            case kMCUnicodePropertyTypeUint32:
            {
                int32_t t_prop;
                t_prop = MCUnicodeGetIntegerProperty(t_char, p_property);
                if (t_prop < 0 || uint32_t(t_prop) > UINT32_MAX)
                    return false;
                ((uint32_t*)x_result_array)[t_offset] = uint32_t(t_prop);
                if (t_advance == 2)
                    ((uint32_t*)x_result_array)[t_offset + 1] = uint32_t(t_prop);
                break;
            }
                
            case kMCUnicodePropertyTypeFloat:
            {
                // There is no checking to ensure that the double can be losslessly
                // converted to a float; doing so would make certain values (e.g.
                // 1/3) unrepresentable as floats. The FPU should also handle
                // out-of-range values correctly, turning them into infinities.
                //
                // On the other hand, (hopefully) nobody expects floating-point
                // values to be exact in the first place...
                double t_prop;
                t_prop = MCUnicodeGetFloatProperty(t_char, p_property);
                ((float*)x_result_array)[t_offset] = float(t_prop);
                if (t_advance == 2)
                    ((float*)x_result_array)[t_offset + 1] = float(t_prop);
                break;
            }
                
            case kMCUnicodePropertyTypeDouble:
            {
                double t_prop;
                t_prop = MCUnicodeGetFloatProperty(t_char, p_property);
                ((double*)x_result_array)[t_offset] = t_prop;
                if (t_advance == 2)
                    ((double*)x_result_array)[t_offset + 1] = t_prop;
                break;
            }
                
            case kMCUnicodePropertyTypeCharacter:
            {
                codepoint_t t_prop;
                t_prop = MCUnicodeGetCharacterProperty(t_char, p_property);
                ((codepoint_t*)x_result_array)[t_offset] = t_prop;
                if (t_advance == 2)
                    ((codepoint_t*)x_result_array)[t_offset + 1] = t_prop;
                break;
            }
                
            case kMCUnicodePropertyTypeString:
            {
                const unichar_t *t_prop;
                t_prop = MCUnicodeGetStringProperty(t_char, p_property);
                ((const unichar_t**)x_result_array)[t_offset] = t_prop;
                if (t_advance == 2)
                    ((const unichar_t**)x_result_array)[t_offset + 1] = t_prop;
				break;
            }
        
            default:
                // Shouldn't get here
                MCAssert(false);
        }
        
        // Move on to the next UTF-16 character
        t_offset += t_advance;
    }
    
    // All done
    return true;
}

const char* MCUnicodeGetPropertyValueName(MCUnicodeProperty p_prop, int32_t p_value)
{
    return u_getPropertyValueName(MCUnicodePropToICUProp[p_prop], p_value, U_LONG_PROPERTY_NAME);
}

////////////////////////////////////////////////////////////////////////////////

// Where possible, the u_hasBinaryProperty and u_charType functions are
// preferred to u_is* as the u_is* functions have legacy behaviours and do not
// match the Standard Recommendations in UTS #18

bool MCUnicodeIsAlphabetic(codepoint_t p_codepoint)
{
    return !!u_hasBinaryProperty(p_codepoint, UCHAR_ALPHABETIC);
}

bool MCUnicodeIsLowercase(codepoint_t p_codepoint)
{
    return !!u_hasBinaryProperty(p_codepoint, UCHAR_LOWERCASE);
}

bool MCUnicodeIsUppercase(codepoint_t p_codepoint)
{
    return !!u_hasBinaryProperty(p_codepoint, UCHAR_UPPERCASE);
}

bool MCUnicodeIsTitlecase(codepoint_t p_codepoint)
{
    return !!u_istitle(p_codepoint);
}

bool MCUnicodeIsWhitespace(codepoint_t p_codepoint)
{
    return !!u_hasBinaryProperty(p_codepoint, UCHAR_WHITE_SPACE);
}

bool MCUnicodeIsPunctuation(codepoint_t p_codepoint)
{
    return !!u_ispunct(p_codepoint);
}

bool MCUnicodeIsDigit(codepoint_t p_codepoint)
{
    return u_charType(p_codepoint) == U_DECIMAL_DIGIT_NUMBER;
}

bool MCUnicodeIsHexDigit(codepoint_t p_codepoint)
{
    return !!u_hasBinaryProperty(p_codepoint, UCHAR_POSIX_XDIGIT);
}

bool MCUnicodeIsAlnum(codepoint_t p_codepoint)
{
    return !!u_hasBinaryProperty(p_codepoint, UCHAR_POSIX_ALNUM);
}

bool MCUnicodeIsBlank(codepoint_t p_codepoint)
{
    return !!u_hasBinaryProperty(p_codepoint, UCHAR_POSIX_BLANK);
}

bool MCUnicodeIsControl(codepoint_t p_codepoint)
{
    return u_charType(p_codepoint) == U_CONTROL_CHAR;
}

bool MCUnicodeIsGraphic(codepoint_t p_codepoint)
{
    return !!u_hasBinaryProperty(p_codepoint, UCHAR_POSIX_GRAPH);
}

bool MCUnicodeIsPrinting(codepoint_t p_codepoint)
{
    return !!u_hasBinaryProperty(p_codepoint, UCHAR_POSIX_PRINT);
}

bool MCUnicodeIsIdentifierInitial(codepoint_t p_codepoint)
{
    return !!u_hasBinaryProperty(p_codepoint, UCHAR_ID_START);
}

bool MCUnicodeIsIdentifierContinue(codepoint_t p_codepoint)
{
    return !!u_hasBinaryProperty(p_codepoint, UCHAR_ID_CONTINUE);
}

////////////////////////////////////////////////////////////////////////////////

bool MCUnicodeNormaliseNFC(const unichar_t *p_in, uindex_t p_in_length,
                           unichar_t *&r_out, uindex_t &r_out_length)
{
    // Get the instance of the NFC normaliser
    UErrorCode t_error;
    const Normalizer2 *t_normaliser;
    t_error = U_ZERO_ERROR;
    t_normaliser = icu::Normalizer2::getNFCInstance(t_error);
    if (U_FAILURE(t_error))
        return false;
    
    // Create an ICU string for the input string
    icu::UnicodeString t_input(p_in, p_in_length);
    
    // Normalise
    icu::UnicodeString t_output;
    t_output = t_normaliser->normalize(t_input, t_error);
    if (U_FAILURE(t_error))
        return false;
    
    // Allocate the output buffer
    MCAutoArray<unichar_t> t_buffer;
    if (!t_buffer.New(t_output.length()))
        return false;
    
    // Copy the normalised string to the buffer
    t_output.extract(t_buffer.Ptr(), t_buffer.Size(), t_error);
    if (U_FAILURE(t_error))
        return false;
    
    // All done
    t_buffer.Take(r_out, r_out_length);
    return true;
}

bool MCUnicodeNormaliseNFKC(const unichar_t *p_in, uindex_t p_in_length,
                            unichar_t *&r_out, uindex_t &r_out_length)
{
    // Get the instance of the NFKC normaliser
    UErrorCode t_error;
    const Normalizer2 *t_normaliser;
    t_error = U_ZERO_ERROR;
    t_normaliser = icu::Normalizer2::getNFKCInstance(t_error);
    if (U_FAILURE(t_error))
        return false;
    
    // Create an ICU string for the input string
    icu::UnicodeString t_input(p_in, p_in_length);
    
    // Normalise
    icu::UnicodeString t_output;
    t_output = t_normaliser->normalize(t_input, t_error);
    if (U_FAILURE(t_error))
        return false;
    
    // Allocate the output buffer
    MCAutoArray<unichar_t> t_buffer;
    if (!t_buffer.New(t_output.length()))
        return false;
    
    // Copy the normalised string to the buffer
    t_output.extract(t_buffer.Ptr(), t_buffer.Size(), t_error);
    if (U_FAILURE(t_error))
        return false;
    
    // All done
    t_buffer.Take(r_out, r_out_length);
    return true;
}

bool MCUnicodeNormaliseNFD(const unichar_t *p_in, uindex_t p_in_length,
                           unichar_t *&r_out, uindex_t &r_out_length)
{
    
    // Get the instance of the NFD normaliser
    UErrorCode t_error;
    const Normalizer2 *t_normaliser;
    t_error = U_ZERO_ERROR;
    t_normaliser = icu::Normalizer2::getNFDInstance(t_error);
    if (U_FAILURE(t_error))
        return false;
    
    // Create an ICU string for the input string
    icu::UnicodeString t_input(p_in, p_in_length);
    
    // Normalise
    icu::UnicodeString t_output;
    t_output = t_normaliser->normalize(t_input, t_error);
    if (U_FAILURE(t_error))
        return false;
    
    // Allocate the output buffer
    MCAutoArray<unichar_t> t_buffer;
    if (!t_buffer.New(t_output.length()))
        return false;
    
    // Copy the normalised string to the buffer
    t_output.extract(t_buffer.Ptr(), t_buffer.Size(), t_error);
    if (U_FAILURE(t_error))
        return false;
    
    // All done
    t_buffer.Take(r_out, r_out_length);
    return true;
}

bool MCUnicodeNormaliseNFKD(const unichar_t *p_in, uindex_t p_in_length,
                            unichar_t *&r_out, uindex_t &r_out_length)
{
    // Get the instance of the NFC normaliser
    UErrorCode t_error;
    const Normalizer2 *t_normaliser;
    t_error = U_ZERO_ERROR;
    t_normaliser = icu::Normalizer2::getNFCInstance(t_error);
    if (U_FAILURE(t_error))
        return false;
    
    // Create an ICU string for the input string
    icu::UnicodeString t_input(p_in, p_in_length);
    
    // Normalise
    icu::UnicodeString t_output;
    t_output = t_normaliser->normalize(t_input, t_error);
    if (U_FAILURE(t_error))
        return false;
    
    // Allocate the output buffer
    MCAutoArray<unichar_t> t_buffer;
    if (!t_buffer.New(t_output.length()))
        return false;
    
    // Copy the normalised string to the buffer
    t_output.extract(t_buffer.Ptr(), t_buffer.Size(), t_error);
    if (U_FAILURE(t_error))
        return false;
    
    // All done
    t_buffer.Take(r_out, r_out_length);
    return true;
}

////////////////////////////////////////////////////////////////////////////////

bool MCUnicodeIsNormalisedNFC(const unichar_t *p_string, uindex_t p_length)
{
    // Get the instance of the NFC normaliser
    UErrorCode t_error;
    const UNormalizer2 *t_normalizer;
    t_error = U_ZERO_ERROR;
    t_normalizer = unorm2_getNFCInstance(&t_error);
    if (U_FAILURE(t_error))
        return false;
    
    // Check for normalisation
    UBool t_result;
    t_result = unorm2_isNormalized(t_normalizer, p_string, p_length, &t_error);
    if (U_FAILURE(t_error))
        return false;
    return !!t_result;
}

bool MCUnicodeIsNormalisedNFKC(const unichar_t *p_string, uindex_t p_length)
{
    // Get the instance of the NFKC normaliser
    UErrorCode t_error;
    const UNormalizer2 *t_normalizer;
    t_error = U_ZERO_ERROR;
    t_normalizer = unorm2_getNFKCInstance(&t_error);
    if (U_FAILURE(t_error))
        return false;
    
    // Check for normalisation
    UBool t_result;
    t_result = unorm2_isNormalized(t_normalizer, p_string, p_length, &t_error);
    if (U_FAILURE(t_error))
        return false;
    return !!t_result;
}

bool MCUnicodeIsNormalisedNFD(const unichar_t *p_string, uindex_t p_length)
{
    // Get the instance of the NFD normaliser
    UErrorCode t_error;
    const UNormalizer2 *t_normalizer;
    t_error = U_ZERO_ERROR;
    t_normalizer = unorm2_getNFDInstance(&t_error);
    if (U_FAILURE(t_error))
        return false;
    
    // Check for normalisation
    UBool t_result;
    t_result = unorm2_isNormalized(t_normalizer, p_string, p_length, &t_error);
    if (U_FAILURE(t_error))
        return false;
    return !!t_result;
}

bool MCUnicodeIsNormalisedNKD(const unichar_t *p_string, uindex_t p_length)
{
    // Get the instance of the NFKD normaliser
    UErrorCode t_error;
    const UNormalizer2 *t_normalizer;
    t_error = U_ZERO_ERROR;
    t_normalizer = unorm2_getNFKDInstance(&t_error);
    if (U_FAILURE(t_error))
        return false;
    
    // Check for normalisation
    UBool t_result;
    t_result = unorm2_isNormalized(t_normalizer, p_string, p_length, &t_error);
    if (U_FAILURE(t_error))
        return false;
    return !!t_result;
}

////////////////////////////////////////////////////////////////////////////////

bool MCUnicodeLowercase(MCLocaleRef p_locale,
                        const unichar_t *p_in, uindex_t p_in_length,
                        unichar_t *&r_out, uindex_t &r_out_length)
{
    // Do the casing using icu::UnicodeString. Note that a locale is required.
    UErrorCode t_error = U_ZERO_ERROR;
    icu::UnicodeString t_input(p_in, p_in_length);
    t_input.toLower(MCLocaleGetICULocale(p_locale));
    
    // Allocate the output buffer
    MCAutoArray<unichar_t> t_buffer;
    if (!t_buffer.New(t_input.length() + 1))
        return false;
    
    // Extract the converted characters
    t_input.extract(t_buffer.Ptr(), t_input.length(), t_error);
    if (U_FAILURE(t_error))
        return false;
    
    // Done
    t_buffer.Take(r_out, r_out_length);
    r_out_length--;
    r_out[r_out_length] = 0;
    return true;
}

bool MCUnicodeUppercase(MCLocaleRef p_locale,
                        const unichar_t *p_in, uindex_t p_in_length,
                        unichar_t *&r_out, uindex_t &r_out_length)
{
    // Do the casing using icu::UnicodeString. Note that a locale is required.
    UErrorCode t_error = U_ZERO_ERROR;
    icu::UnicodeString t_input(p_in, p_in_length);
    t_input.toUpper(MCLocaleGetICULocale(p_locale));
    
    // Allocate the output buffer
    MCAutoArray<unichar_t> t_buffer;
    if (!t_buffer.New(t_input.length()))
        return false;
    
    // Extract the converted characters
    t_input.extract(t_buffer.Ptr(), t_buffer.Size(), t_error);
    if (U_FAILURE(t_error))
        return false;
    
    // Done
    t_buffer.Take(r_out, r_out_length);
    return true;
}

bool MCUnicodeTitlecase(MCLocaleRef p_locale,
                        const unichar_t *p_in, uindex_t p_in_length,
                        unichar_t *&r_out, uindex_t &r_out_length)
{
    // The ICU UnicodeString class is convenient for case transformations
    UErrorCode t_error = U_ZERO_ERROR;
    icu::UnicodeString t_input(p_in, p_in_length);
    
    // By supplying no specific break iterator, the default is used
    t_input.toTitle(NULL, MCLocaleGetICULocale(p_locale));
    
    // Allocate the output buffer
    MCAutoArray<unichar_t> t_buffer;
    if (!t_buffer.New(t_input.length()))
        return false;
    
    // Extract the converted characters
    t_input.extract(t_buffer.Ptr(), t_buffer.Size(), t_error);
    if (U_FAILURE(t_error))
        return false;
    
    // Done
    t_buffer.Take(r_out, r_out_length);
    return true;
}

static void __MCUnicodeSimpleCaseFold(icu::UnicodeString& p_string)
{
    icu::UnicodeString t_temp;
    int32_t i = 0;
    
    while(i < p_string.length())
    {
        t_temp.append(u_tolower(p_string.char32At(i)/*, U_FOLD_CASE_DEFAULT*/));
        i = p_string.moveIndex32(i, 1);
    }
    
    p_string.setTo(t_temp);
}

bool MCUnicodeCaseFold(const unichar_t *p_in, uindex_t p_in_length,
                       unichar_t *&r_out, uindex_t &r_out_length)
{
    // The ICU UnicodeString class is convenient for case transformations
    UErrorCode t_error = U_ZERO_ERROR;
    icu::UnicodeString t_input(p_in, p_in_length);
    
    // Unlike the other case transformations, case folding does not depend on
    // locale. Its only option is whether to use the special Turkish rules for
    // casefolding dotless i and dotted l but we don't support this.
    __MCUnicodeSimpleCaseFold(t_input);
    
    // Allocate the output buffer
    MCAutoArray<unichar_t> t_buffer;
    if (!t_buffer.New(t_input.length()))
        return false;
    
    // Extract the converted characters
    t_input.extract(t_buffer.Ptr(), t_buffer.Size(), t_error);
    if (U_FAILURE(t_error))
        return false;
    
    // Done
    t_buffer.Take(r_out, r_out_length);
    return true;
}

////////////////////////////////////////////////////////////////////////////////

int32_t MCUnicodeCompare(const void *p_first, uindex_t p_first_length, bool p_first_native,
                         const void *p_second, uindex_t p_second_length, bool p_second_native,
                         MCUnicodeCompareOption p_option)
{
    // Create the filters
	MCAutoPointer<MCTextFilter> t_first_filter =
			MCTextFilterCreate(p_first, p_first_length, p_first_native ? kMCStringEncodingNative : kMCStringEncodingUTF16, p_option);
	
    MCAutoPointer<MCTextFilter> t_second_filter =
			MCTextFilterCreate(p_second, p_second_length, p_second_native ? kMCStringEncodingNative : kMCStringEncodingUTF16, p_option);
    
    while (t_first_filter->HasData() && t_second_filter->HasData())
    {
        // NOTE: this is a simple codepoint compare and produces a binary
        // ordering rather than a lexical ordering.
        int32_t t_diff;
        t_diff = t_first_filter->GetNextCodepoint() - t_second_filter->GetNextCodepoint();
        if (t_diff != 0)
        {
            return t_diff;
        }
        
        t_first_filter->AdvanceCursor();
        t_second_filter->AdvanceCursor();
    }
    
    bool t_first_longer = t_first_filter->HasData();
    bool t_second_longer = t_second_filter->HasData();
    
    if (t_first_longer)
        return 1;
    else if (t_second_longer)
        return -1;
    else
        return 0;
}

bool MCUnicodeBeginsWith(const void *p_first, uindex_t p_first_length, bool p_first_native,
                         const void *p_second, uindex_t p_second_length, bool p_second_native,
                         MCUnicodeCompareOption p_option, uindex_t *r_first_match_length)
{
    // Check for a shared prefix
    uindex_t t_first_match_length, t_second_match_length;
    MCUnicodeSharedPrefix(p_first, p_first_length, p_first_native, p_second, p_second_length, p_second_native, p_option, t_first_match_length, t_second_match_length);
    if (t_second_match_length != p_second_length)
        return false;
    if (r_first_match_length != nil)
        *r_first_match_length = t_first_match_length;
    return true;
}

bool MCUnicodeEndsWith(const void *p_first, uindex_t p_first_length, bool p_first_native,
                       const void *p_second, uindex_t p_second_length, bool p_second_native,
                         MCUnicodeCompareOption p_option, uindex_t *r_first_match_length)
{
    // Check for a shared suffix
    uindex_t t_first_match_length, t_second_match_length;
    MCUnicodeSharedSuffix(p_first, p_first_length, p_first_native, p_second, p_second_length, p_second_native, p_option, t_first_match_length, t_second_match_length);
    if (t_second_match_length != p_second_length)
        return false;
    if (r_first_match_length != nil)
        *r_first_match_length = t_first_match_length;
    return true;
}

bool MCUnicodeContains(const void *p_string, uindex_t p_string_length, bool p_string_native,
                       const void *p_needle, uindex_t p_needle_length, bool p_needle_native,
                       MCUnicodeCompareOption p_option)
{
    uindex_t ignored;
    return MCUnicodeFirstIndexOf(p_string, p_string_length, p_string_native, p_needle, p_needle_length, p_needle_native, p_option, ignored);
}

bool MCUnicodeFirstIndexOf(const void *p_string, uindex_t p_string_length, bool p_string_native,
                           const void *p_needle, uindex_t p_needle_length, bool p_needle_native,
                           MCUnicodeCompareOption p_option, uindex_t &r_index)
{
    // Avoid potential problems
    if (p_string_length == 0 || p_needle_length == 0)
        return false;
    
    // Shortcut for native char - for which we are sure to have only one char to compare, and no composing characters
	if (p_needle_length == 1)
	{
		codepoint_t t_needle;
		bool t_ok_fast;
		if (p_needle_native)
		{
			t_needle = codepoint_t(*reinterpret_cast<const char_t *>(p_needle));
			t_ok_fast = t_needle < 0x80; /* Needle is one ASCII char */
		}
		else
		{
			t_needle = codepoint_t(*reinterpret_cast<const unichar_t *>(p_needle));
			t_ok_fast = t_needle < 0xd800; /* Needle is in BMP */
		}

		if (t_ok_fast)
		{
			// if we got here, the string should not have been native.
			MCAssert(!p_string_native);
			return MCUnicodeFirstIndexOfChar((const unichar_t *)p_string, p_string_length, t_needle, p_option, r_index);
		}
	}
    
    // Create filter chains for the strings being searched
	MCAutoPointer<MCTextFilter> t_string_filter =
			MCTextFilterCreate(p_string, p_string_length, p_string_native ? kMCStringEncodingNative : kMCStringEncodingUTF16, p_option);
	
	MCAutoPointer<MCTextFilter> t_needle_filter =
			MCTextFilterCreate(p_needle, p_needle_length, p_needle_native ? kMCStringEncodingNative : kMCStringEncodingUTF16, p_option);

    // We only want the first codepoint of the needle (for now)
    codepoint_t t_needle_start = t_needle_filter->GetNextCodepoint();
    
    // Search for the beginning of the needle
    while (t_string_filter->HasData())
    {
        codepoint_t t_cp = t_string_filter->GetNextCodepoint();
        if (t_cp == t_needle_start)
        {
            // Do a fresh string comparison at this point
            t_string_filter->MarkText();
            uindex_t t_offset = t_string_filter->GetMarkedLength() - 1;
            uindex_t t_string_matched_len, t_needle_matched_len;
            MCUnicodeSharedPrefix((const char *)p_string + (p_string_native ? t_offset : (t_offset * 2)), p_string_length - t_offset, p_string_native, p_needle, p_needle_length, p_needle_native, p_option, t_string_matched_len, t_needle_matched_len);
            if (t_needle_matched_len == p_needle_length)
            {
                r_index = t_offset;
                return true;
            }
        }
        
        t_string_filter->AdvanceCursor();
    }
    
    // No match was found
    return false;
}

bool MCUnicodeLastIndexOf(const void *p_string, uindex_t p_string_length, bool p_string_native,
                          const void *p_needle, uindex_t p_needle_length, bool p_needle_native,
                          MCUnicodeCompareOption p_option, uindex_t &r_index)
{
    // Avoid potential problems
    if (p_string_length == 0 || p_needle_length == 0)
        return false;
    
    // Shortcut for native char - for which we are sure to have only one char to compare, and no composing characters
    if (p_needle_length == 1 && *(codepoint_t *)p_needle < 0x10)
    {
        // if we got here, the string should not have been native.
        MCAssert(!p_string_native);
        return MCUnicodeLastIndexOfChar((const unichar_t *)p_string, p_string_length, *(codepoint_t *)p_needle, p_option, r_index);
    }
    
    // Create filter chains for the strings being searched
	MCAutoPointer<MCTextFilter> t_string_filter =
			MCTextFilterCreate(p_string, p_string_length, p_string_native ? kMCStringEncodingNative : kMCStringEncodingUTF16, p_option, true);
	
	MCAutoPointer<MCTextFilter> t_needle_filter =
			MCTextFilterCreate(p_needle, p_needle_length, p_needle_native ? kMCStringEncodingNative : kMCStringEncodingUTF16, p_option, true);
    
    // We only want the last codepoint of the needle (for now)
    codepoint_t t_needle_end = t_needle_filter->GetNextCodepoint();
    
    // Search for the end of the needle
    while (t_string_filter->HasData())
    {
        codepoint_t t_cp = t_string_filter->GetNextCodepoint();
        if (t_cp == t_needle_end)
        {
            // Do a fresh string comparison at this point
            t_string_filter->MarkText();
            uindex_t t_offset = p_string_length - t_string_filter->GetMarkedLength();
            uindex_t t_string_matched_len, t_needle_matched_len;
            MCUnicodeSharedSuffix(p_string, t_offset, p_string_native, p_needle, p_needle_length, p_needle_native, p_option, t_string_matched_len, t_needle_matched_len);
            if (t_needle_matched_len == p_needle_length)
            {
                r_index = t_offset - t_string_matched_len;
                return true;
            }
        }
        
        t_string_filter->AdvanceCursor();
    }
    
    // No match was found
    return false;
}

bool MCUnicodeFirstIndexOfChar(const unichar_t *p_string, uindex_t p_string_length,
                               codepoint_t p_needle, MCUnicodeCompareOption p_option,
                               uindex_t &r_index)
{
    // Create filter chain for the string being searched
	MCAutoPointer<MCTextFilter> t_string_filter =
			MCTextFilterCreate(p_string, p_string_length, kMCStringEncodingUTF16, p_option);
	
	// Process the needle codepoint according to the string options.
	// We use NFC for normalization, so all single char unicode strings
	// are already normalized. Therefore we just need to fold if
	// caseless or folded.
	codepoint_t t_processed_needle;
	if (p_option == kMCUnicodeCompareOptionFolded || p_option == kMCUnicodeCompareOptionCaseless)
		t_processed_needle = MCUnicodeGetCharacterProperty(p_needle, kMCUnicodePropertySimpleCaseFolding);
	else
		t_processed_needle = p_needle;
		
    // Loop until we find the character
    while (t_string_filter->HasData())
    {
        codepoint_t t_cp = t_string_filter->GetNextCodepoint();
        if (t_cp == t_processed_needle)
        {
            t_string_filter->MarkText();
            r_index = t_string_filter->GetMarkedLength() - 1;
            return true;
        }
        
        t_string_filter->AdvanceCursor();
    }
    
    // Could not find the character
    return false;
}

bool MCUnicodeLastIndexOfChar(const unichar_t *p_string, uindex_t p_string_length,
                               codepoint_t p_needle, MCUnicodeCompareOption p_option,
                               uindex_t &r_index)
{
    // Create filter chain for the string being searched
	MCAutoPointer<MCTextFilter> t_string_filter =
			MCTextFilterCreate(p_string, p_string_length, kMCStringEncodingUTF16, p_option, true);
    
    // Loop until we find the character
    while (t_string_filter->HasData())
    {
        codepoint_t t_cp = t_string_filter->GetNextCodepoint();
        if (t_cp == p_needle)
        {
            t_string_filter->MarkText();
            r_index = p_string_length - t_string_filter->GetMarkedLength();
            return true;
        }
        
        t_string_filter->AdvanceCursor();
    }
    
    // Could not find the character
    return false;
}

void MCUnicodeSharedPrefix(const void *p_string, uindex_t p_string_length, bool p_string_native,
                           const void *p_prefix, uindex_t p_prefix_length, bool p_prefix_native,
                           MCUnicodeCompareOption p_option, uindex_t &r_len_in_string, uindex_t &r_len_in_prefix)
{
    // Avoid degenerate cases
    if (p_string_length == 0 || p_prefix_length == 0)
    {
        r_len_in_string = r_len_in_prefix = 0;
        return;
    }
    
    // Set up the filter chains for the strings
	MCAutoPointer<MCTextFilter> t_string_filter =
			MCTextFilterCreate(p_string, p_string_length, p_string_native ? kMCStringEncodingNative : kMCStringEncodingUTF16, p_option);
	
	MCAutoPointer<MCTextFilter> t_prefix_filter =
			MCTextFilterCreate(p_prefix, p_prefix_length, p_prefix_native ? kMCStringEncodingNative : kMCStringEncodingUTF16, p_option);
    
    // Keep looping until the strings no longer match
    while (t_string_filter->GetNextCodepoint() == t_prefix_filter->GetNextCodepoint())
    {
        t_string_filter->MarkText();
        t_prefix_filter->MarkText();
        
        t_string_filter->AdvanceCursor();
        t_prefix_filter->AdvanceCursor();
        
        if (!t_string_filter->HasData() || !t_prefix_filter->HasData())
        {
            // We need to read the next codepoint to update the cursor position - and place it at the end of the last matching character
            t_string_filter -> GetNextCodepoint();
            t_prefix_filter -> GetNextCodepoint();
            break;
        }
    }
    
    // GetNextCodepoint reads the next codepoint, but doesn't update the read index.
    // We need to get the actual last matching index, we need to mark the last character as being the same - indeed
    // And we'll get the length minus 1, since GetMarkedLength returns the marked position + 1 - which is not the actual
    // position in a string when the last character compared is more than a byte long.
    t_string_filter->MarkText();
    t_prefix_filter->MarkText();
    
    // Return the lengths in each. Note we don't accept here to avoid matching
    // subsequences of normalised runs of combining chars.
    r_len_in_string = t_string_filter->GetMarkedLength() - 1;
    r_len_in_prefix = t_prefix_filter->GetMarkedLength() - 1;
}

void MCUnicodeSharedSuffix(const void *p_string, uindex_t p_string_length, bool p_string_native,
                           const void *p_suffix, uindex_t p_suffix_length, bool p_suffix_native,
                           MCUnicodeCompareOption p_option, uindex_t &r_len_in_string, uindex_t &r_len_in_suffix)
{
    // Avoid degenerate cases
    if (p_string_length == 0 || p_suffix_length == 0)
    {
        r_len_in_string = r_len_in_suffix = 0;
        return;
    }
    
    // Set up the filter chains for the strings
	MCAutoPointer<MCTextFilter> t_string_filter =
			MCTextFilterCreate(p_string, p_string_length, p_string_native ? kMCStringEncodingNative : kMCStringEncodingUTF16, p_option, true);
	MCAutoPointer<MCTextFilter> t_suffix_filter =
			MCTextFilterCreate(p_suffix, p_suffix_length, p_suffix_native ? kMCStringEncodingNative : kMCStringEncodingUTF16, p_option, true);
    
    // Keep looping until the strings no longer match
    while (t_string_filter->GetNextCodepoint() == t_suffix_filter->GetNextCodepoint())
    {
        t_string_filter->MarkText();
        t_suffix_filter->MarkText();
        
        t_string_filter->AdvanceCursor();
        t_suffix_filter->AdvanceCursor();
        
        if (!t_string_filter->HasData() || !t_suffix_filter->HasData())
        {
            // We need to read the next codepoint to update the cursor position - and place it at the end of the last matching character
            t_string_filter -> GetNextCodepoint();
            t_suffix_filter -> GetNextCodepoint();
            break;
        }
    }
    
    // GetNextCodepoint reads the next codepoint, but doesn't update the read index.
    // We need to get the actual last matching index, we need to mark the last character as being the same - indeed
    // And we'll get the length minus 1, since GetMarkedLength returns the marked position + 1 - which is not the actual
    // position in a string when the last character compared is more than a byte long.
    t_string_filter->MarkText();
    t_suffix_filter->MarkText();
    
    // Return the lengths in each. Note we don't accept here to avoid matching
    // subsequences of normalised runs of combining chars.
    r_len_in_string = t_string_filter->GetMarkedLength() - 1;
    r_len_in_suffix = t_suffix_filter->GetMarkedLength() - 1;
}

bool MCUnicodeFind(const void *p_string, uindex_t p_string_length, bool p_string_native,
                   const void *p_needle, uindex_t p_needle_length, bool p_needle_native,
                   MCUnicodeCompareOption p_option, MCRange &r_matched_range)
{
    // Handle some degenerate cases
    if (p_string_length == 0 || p_needle_length == 0)
        return false;
    
    // Attempt a match at each position within the string
    uindex_t t_offset = 0;
    while (t_offset < p_string_length)
    {
        // Calculate the prefix length - if this is equal to the length of the
        // needle then a match has occurred.
        uindex_t t_matched_length_string, t_matched_length_needle;
        MCUnicodeSharedPrefix((const char *)p_string + (p_string_native ? t_offset : (t_offset * 2)), p_string_length - t_offset, p_string_native, p_needle, p_needle_length, p_needle_native, p_option, t_matched_length_string, t_matched_length_needle);
        if (t_matched_length_needle == p_needle_length)
        {
            r_matched_range = MCRangeMake(t_offset, t_matched_length_string);
            return true;
        }
        
        t_offset++;
    }
    
    // String could not be found
    return false;
}

hash_t MCUnicodeHash(const unichar_t *p_string, uindex_t p_string_length, MCUnicodeCompareOption p_option)
{
    // Create a filter for the string
    MCAutoPointer<MCTextFilter> t_filter =
			MCTextFilterCreate(p_string, p_string_length, kMCStringEncodingUTF16, p_option);

    MCHashCharsContext t_context;
    while (t_filter->HasData())
    {
        t_context.consume(t_filter->GetNextCodepoint());
        t_filter->AdvanceCursor();
    }
    return t_context;
}

////////////////////////////////////////////////////////////////////////////////

MCUnicodeCollateOption MCUnicodeCollateOptionFromCompareOption(MCUnicodeCompareOption p_option)
{
    intenum_t t_option;
    
    if (p_option == kMCUnicodeCompareOptionExact)
        t_option = kMCUnicodeCollateOptionStrengthIdentical;
    else if (p_option == kMCUnicodeCompareOptionNormalised)
    {
        t_option = kMCUnicodeCollateOptionStrengthIdentical;
        t_option |= kMCUnicodeCollateOptionAutoNormalise;
    }
    else if (p_option == kMCUnicodeCompareOptionFolded)
        t_option = kMCUnicodeCollateOptionStrengthTertiary;
    else
    {
        t_option = kMCUnicodeCollateOptionStrengthSecondary;
        t_option |= kMCUnicodeCollateOptionAutoNormalise;
    }
    
    return (MCUnicodeCollateOption)t_option;
}

bool MCUnicodeCreateCollator(MCLocaleRef p_locale, MCUnicodeCollateOption p_options, MCUnicodeCollatorRef& r_collator)
{
    // Create a collation object for the given locale
    UErrorCode t_error = U_ZERO_ERROR;
    icu::Collator* t_collator;
    t_collator = icu::Collator::createInstance(MCLocaleGetICULocale(p_locale), t_error);
    
    // If we couldn't create a collator for the given locale, create a default one
    if (t_collator == NULL)
    {
        t_error = U_ZERO_ERROR;
        t_collator = icu::Collator::createInstance(t_error);
        if (t_collator == NULL)
            return false;
    }
    
    // Set the collation options
    // Note that the enumerated strengths have the same values as the ICU enum
    switch (p_options & kMCUnicodeCollateOptionStrengthMask)
    {
        case kMCUnicodeCollateOptionStrengthPrimary:
            t_collator->setStrength(icu::Collator::PRIMARY);
            break;
            
        case kMCUnicodeCollateOptionStrengthSecondary:
            t_collator->setStrength(icu::Collator::SECONDARY);
            break;
            
        case kMCUnicodeCollateOptionStrengthTertiary:
            t_collator->setStrength(icu::Collator::TERTIARY);
            break;
            
        case kMCUnicodeCollateOptionStrengthQuaternary:
            t_collator->setStrength(icu::Collator::QUATERNARY);
            break;
            
        case kMCUnicodeCollateOptionStrengthIdentical:
            t_collator->setStrength(icu::Collator::IDENTICAL);
            break;
            
        default:
            // Use the default strength
            break;
    }
    
    if (p_options & kMCUnicodeCollateOptionAutoNormalise)
        t_collator->setAttribute(UCOL_NORMALIZATION_MODE, UCOL_ON, t_error);
    
    if (p_options & kMCUnicodeCollateOptionNumeric)
        t_collator->setAttribute(UCOL_NUMERIC_COLLATION, UCOL_ON, t_error);
    
    if (p_options & kMCUnicodeCollateOptionIgnorePunctuation)
        t_collator->setAttribute(UCOL_ALTERNATE_HANDLING, UCOL_SHIFTED, t_error);
 
    r_collator = t_collator;
    
    return true;
}

void MCUnicodeDestroyCollator(MCUnicodeCollatorRef p_collator)
{
    icu::Collator* t_collator;
    t_collator = (icu::Collator *)p_collator;
    delete t_collator;
}

int32_t MCUnicodeCollate(MCLocaleRef p_locale, MCUnicodeCollateOption p_options,
                         const unichar_t *p_first, uindex_t p_first_length,
                         const unichar_t *p_second, uindex_t p_second_length)
{
    MCUnicodeCollatorRef t_collator_ref;
    if (!MCUnicodeCreateCollator(p_locale, p_options, t_collator_ref))
        return 0;
    
    int32_t t_result;
    t_result = MCUnicodeCollateWithCollator(t_collator_ref, p_first, p_first_length, p_second, p_second_length);
    
    MCUnicodeDestroyCollator(t_collator_ref);
    
    return t_result;
}

bool MCUnicodeCreateSortKey(MCLocaleRef p_locale, MCUnicodeCollateOption p_options,
                            const unichar_t *p_string, uindex_t p_string_length,
                            byte_t *&r_key, uindex_t &r_key_length)
{
    MCUnicodeCollatorRef t_collator;
    if (!MCUnicodeCreateCollator(p_locale, p_options, t_collator))
        return false;
    
    bool t_success;
    t_success = MCUnicodeCreateSortKeyWithCollator(t_collator, p_string, p_string_length, r_key, r_key_length);
    
    MCUnicodeDestroyCollator(t_collator);
    
    return t_success;
}

bool MCUnicodeCreateSortKeyWithCollator(MCUnicodeCollatorRef p_collator,
                                        const unichar_t *p_string, uindex_t p_string_length,
                                        byte_t *&r_key, uindex_t &r_key_length)
{
    icu::Collator* t_collator;
    t_collator = (icu::Collator *)p_collator;
    
    // Find the length of the sort key that will be generated
    uindex_t t_key_length;
    t_key_length = (unsigned)t_collator->getSortKey(p_string, (signed)p_string_length, NULL, 0);
    
    // Allocate memory for the sort key
    MCAutoArray<byte_t> t_key;
    if (!t_key.New(t_key_length))
        return false;
    
    // Generate the sort key
    t_collator->getSortKey(p_string, (signed)p_string_length, t_key.Ptr(), (signed)t_key.Size());
    
    t_key.Take(r_key, r_key_length);
    
    return true;
}

int32_t MCUnicodeCollateWithCollator(MCUnicodeCollatorRef p_collator,
                                  const unichar_t *p_first, uindex_t p_first_length,
                                  const unichar_t *p_second, uindex_t p_second_length)
{
    
    UErrorCode t_error = U_ZERO_ERROR;
    
    icu::Collator* t_collator;
    t_collator = (icu::Collator *)p_collator;
    
    // Do the comparison
    UCollationResult t_result;
    t_result = t_collator->compare(p_first, (signed)p_first_length, p_second, (signed)p_second_length, t_error);
    
    // The UCollationResult type maps UCOL_{GREATER,EQUAL,LESS} to +1,0,-1
    return int32_t(t_result);
}

////////////////////////////////////////////////////////////////////////////////

bool MCUnicodeMapToNativeSingleton_Windows1252(uinteger_t x, char_t& r_char)
{
	static const uinteger_t s_singletons[] =
	{
		0x8c000152, 0x9c000153, 0x8a000160, 0x9a000161, 0x9f000178, 0x8e00017d,
		0x9e00017e, 0x83000192, 0x880002c6, 0x980002dc, 0x96002013, 0x97002014,
		0x91002018, 0x92002019, 0x8200201a, 0x9300201c, 0x9400201d, 0x8400201e,
		0x86002020, 0x87002021, 0x95002022, 0x85002026, 0x89002030, 0x8b002039,
		0x9b00203a, 0x800020ac, 0x99002122
	};
    
	uinteger_t t_low, t_high;
	t_low = 0;
	t_high = sizeof(s_singletons) / sizeof(uinteger_t);
	while(t_low < t_high)
	{
		uinteger_t t_mid;
		t_mid = t_low + (t_high - t_low) / 2;
		
		uinteger_t t_singleton;
		t_singleton = s_singletons[t_mid] & 0xffffff;
        
		if (x < t_singleton)
			t_high = t_mid;
		else if (x > t_singleton)
			t_low = t_mid + 1;
		else
		{
			r_char = s_singletons[t_mid] >> 24;
			return true;
		}
	}
    
	return false;
}

bool MCUnicodeMapToNativeSingleton_MacRoman(uinteger_t x, char_t& r_char)
{
	static const uinteger_t s_singletons[] =
	{
		0xca0000a0, 0xc10000a1, 0xa20000a2, 0xa30000a3, 0xb40000a5, 0xa40000a7,
		0xac0000a8, 0xa90000a9, 0xbb0000aa, 0xc70000ab, 0xc20000ac, 0xa80000ae,
		0xf80000af, 0xa10000b0, 0xb10000b1, 0xab0000b4, 0xb50000b5, 0xa60000b6,
		0xe10000b7, 0xfc0000b8, 0xbc0000ba, 0xc80000bb, 0xc00000bf, 0xcb0000c0,
		0xe70000c1, 0xe50000c2, 0xcc0000c3, 0x800000c4, 0x810000c5, 0xae0000c6,
		0x820000c7, 0xe90000c8, 0x830000c9, 0xe60000ca, 0xe80000cb, 0xed0000cc,
		0xea0000cd, 0xeb0000ce, 0xec0000cf, 0x840000d1, 0xf10000d2, 0xee0000d3,
		0xef0000d4, 0xcd0000d5, 0x850000d6, 0xaf0000d8, 0xf40000d9, 0xf20000da,
		0xf30000db, 0x860000dc, 0xa70000df, 0x880000e0, 0x870000e1, 0x890000e2,
		0x8b0000e3, 0x8a0000e4, 0x8c0000e5, 0xbe0000e6, 0x8d0000e7, 0x8f0000e8,
		0x8e0000e9, 0x900000ea, 0x910000eb, 0x930000ec, 0x920000ed, 0x940000ee,
		0x950000ef, 0x960000f1, 0x980000f2, 0x970000f3, 0x990000f4, 0x9b0000f5,
		0x9a0000f6, 0xd60000f7, 0xbf0000f8, 0x9d0000f9, 0x9c0000fa, 0x9e0000fb,
		0x9f0000fc, 0xd80000ff, 0xf5000131, 0xce000152, 0xcf000153, 0xd9000178,
		0xc4000192, 0xf60002c6, 0xff0002c7, 0xf90002d8, 0xfa0002d9, 0xfb0002da,
		0xfe0002db, 0xf70002dc, 0xfd0002dd, 0xbd0003a9, 0xb90003c0, 0xd0002013,
		0xd1002014, 0xd4002018, 0xd5002019, 0xe200201a, 0xd200201c, 0xd300201d,
		0xe300201e, 0xa0002020, 0xe0002021, 0xa5002022, 0xc9002026, 0xe4002030,
		0xdc002039, 0xdd00203a, 0xda002044, 0xdb0020ac, 0xaa002122, 0xb6002202,
		0xc6002206, 0xb800220f, 0xb7002211, 0xc300221a, 0xb000221e, 0xba00222b,
		0xc5002248, 0xad002260, 0xb2002264, 0xb3002265, 0xd70025ca, 0xf000f8ff,
		0xde00fb01, 0xdf00fb02
	};
	
	uinteger_t t_low, t_high;
	t_low = 0;
	t_high = sizeof(s_singletons) / sizeof(uinteger_t);
	while(t_low < t_high)
	{
		uinteger_t t_mid;
		t_mid = t_low + (t_high - t_low) / 2;
		
		uinteger_t t_singleton;
		t_singleton = s_singletons[t_mid] & 0xffffff;
		
		if (x < t_singleton)
			t_high = t_mid;
		else if (x > t_singleton)
			t_low = t_mid + 1;
		else
		{
			r_char = s_singletons[t_mid] >> 24;
			return true;
		}
	}
	
	return false;
}

bool MCUnicodeMapToNativeSingleton_ISO8859_1(uinteger_t x, char_t& r_char)
{
	static const uinteger_t s_singletons[] =
	{
		0xa50000a5,
		0xa70000a7,
		0xa90000a9,
		0xaa0000aa,
		0xab0000ab,
		0xac0000ac,
		0xad0000ad,
		0xae0000ae,
		0xaf0000af,
		0xb00000b0,
		0xb10000b1,
		0xb20000b2,
		0xb30000b3,
		0xb50000b5,
		0xb60000b6,
		0xb70000b7,
		0xb90000b9,
		0xba0000ba,
		0xbb0000bb,
		0xbc000152,
		0xbd000153,
		0xa6000160,
		0xa8000161,
		0xbe000178,
		0xb400017d,
		0xb800017e,
		0xa40020ac
	};
	
	uinteger_t t_low, t_high;
	t_low = 0;
	t_high = sizeof(s_singletons) / sizeof(uinteger_t);
	while(t_low < t_high)
	{
		uinteger_t t_mid;
		t_mid = t_low + (t_high - t_low) / 2;
		
		uinteger_t t_singleton;
		t_singleton = s_singletons[t_mid] & 0xffffff;
		
		if (x < t_singleton)
			t_high = t_mid;
		else if (x > t_singleton)
			t_low = t_mid + 1;
		else
		{
			r_char = s_singletons[t_mid] >> 24;
			return true;
		}
	}
	
	return false;
}

////////////////////////////////////////////////////////////////////////////////

bool MCUnicodeMapToNativePair_Windows1252(uinteger_t x, uinteger_t y, char_t& r_char)
{
	static const uinteger_t s_pairs[] =
	{
        /* A */ 0xC0410300, 0xC1410301, 0xC2410302, 0xC3410303, 0xC4410308, 0xC541030A,
        /* C */ 0xC7430327,
        /* E */ 0xC8450300, 0xC9450301, 0xCA450302, 0xCB450308,
        /* I */ 0xCC490300, 0xCD490301, 0xCE490302, 0xCF490308,
        /* N */ 0xD14E0303,
        /* O */ 0xD24F0300, 0xD34F0301, 0xD44F0302, 0xD54F0303, 0xD64F0308,
        /* U */ 0xD9550300, 0xDA550301, 0xDB550302, 0xDC550308,
        /* S */ 0x8A53030C,
        /* Y */ 0xDD590301, 0x9F590308,
        /* Z */ 0x8E5A030C,
        
        /* a */ 0xE0610300, 0xE1610301, 0xE2610302, 0xE3610303, 0xE4610308, 0xE561030A,
        /* c */ 0xE7630327,
        /* e */ 0xE8650300, 0xE9650301, 0xEA650302, 0xEB650308,
        /* i */ 0xEC690300, 0xED690301, 0xEE690302, 0xEF690308,
        /* n */ 0xF16E0303,
        /* i */ 0xF26F0300, 0xF36F0301, 0xF46F0302, 0xF56F0303, 0xF66F0308,
        /* u */ 0xF9750300, 0xFA750301, 0xFB750302, 0xFC750308,
        /* s */ 0x9A73030C,
        /* y */ 0xFD790301, 0xFF790308,
        /* z */ 0x9E7A030C
	};
    
	uinteger_t z;
	z = (x << 16) | y;
    
	uinteger_t t_low, t_high;
	t_low = 0;
	t_high = sizeof(s_pairs) / sizeof(uinteger_t);
	while(t_low < t_high)
	{
		uinteger_t t_mid;
		t_mid = t_low + (t_high - t_low) / 2;
		
		uinteger_t t_singleton;
		t_singleton = s_pairs[t_mid] & 0xffffff;
        
		if (z < t_singleton)
			t_high = t_mid;
		else if (z > t_singleton)
			t_low = t_mid + 1;
		else
		{
			r_char = s_pairs[t_mid] >> 24;
			return true;
		}
	}
    
	return false;
}

bool MCUnicodeMapToNativePair_MacRoman(uinteger_t x, uinteger_t y, char_t& r_char)
{
	static const uinteger_t s_pairs[] =
	{
		0xAD3D0338,0xCB410300,0xE7410301,0xE5410302,0xCC410303,0x80410308,
		0x8141030A,0x82430327,0xE9450300,0x83450301,0xE6450302,0xE8450308,
		0xED490300,0xEA490301,0xEB490302,0xEC490308,0x844E0303,0xF14F0300,
		0xEE4F0301,0xEF4F0302,0xCD4F0303,0x854F0308,0xF4550300,0xF2550301,
		0xF3550302,0x86550308,0xD9590308,0x88610300,0x87610301,0x89610302,
		0x8B610303,0x8A610308,0x8C61030A,0x8D630327,0x8F650300,0x8E650301,
		0x90650302,0x91650308,0x93690300,0x92690301,0x94690302,0x95690308,
		0x966E0303,0x986F0300,0x976F0301,0x996F0302,0x9B6F0303,0x9A6F0308,
		0x9D750300,0x9C750301,0x9E750302,0x9F750308,0xD8790308
	};
	
	uinteger_t z;
	z = (x << 16) | y;
	
	uinteger_t t_low, t_high;
	t_low = 0;
	t_high = sizeof(s_pairs) / sizeof(uinteger_t);
	while(t_low < t_high)
	{
		uinteger_t t_mid;
		t_mid = t_low + (t_high - t_low) / 2;
		
		uinteger_t t_singleton;
		t_singleton = s_pairs[t_mid] & 0xffffff;
		
		if (z < t_singleton)
			t_high = t_mid;
		else if (z > t_singleton)
			t_low = t_mid + 1;
		else
		{
			r_char = s_pairs[t_mid] >> 24;
			return true;
		}
	}
	
	return false;
}

bool MCUnicodeMapToNativePair_ISO8859_1(uinteger_t x, uinteger_t y, char_t& r_char)
{
	static const uinteger_t s_pairs[] =
	{
		/* S */ 0xA653030C,
		/* Y */ 0xBE590308,
		/* Z */ 0x8E5A030C,
		/* s */ 0xA873030C,
		/* z */ 0xB87A030C
	};
	
	uinteger_t z;
	z = (x << 16) | y;
	
	uinteger_t t_low, t_high;
	t_low = 0;
	t_high = sizeof(s_pairs) / sizeof(uinteger_t);
	while(t_low < t_high)
	{
		uinteger_t t_mid;
		t_mid = t_low + (t_high - t_low) / 2;
		
		uinteger_t t_singleton;
		t_singleton = s_pairs[t_mid] & 0xffffff;
		
		if (z < t_singleton)
			t_high = t_mid;
		else if (z > t_singleton)
			t_low = t_mid + 1;
		else
		{
			r_char = s_pairs[t_mid] >> 24;
			return true;
		}
	}
	
	return false;
}

////////////////////////////////////////////////////////////////////////////////

uinteger_t MCUnicodeMapFromNativeSingleton_Windows1252(unsigned char p_char)
{
	static const unichar_t s_mapping[] =
	{
		0x20ac, 0x0081, 0x201a, 0x0192, 0x201e, 0x2026, 0x2020, 0x2021, 0x02c6, 0x2030, 0x0160, 0x2039,
		0x0152, 0x008d, 0x017d, 0x008f, 0x0090, 0x2018, 0x2019, 0x201c, 0x201d, 0x2022, 0x2013, 0x2014,
		0x02dc, 0x2122, 0x0161, 0x203a, 0x0153, 0x009d, 0x017e, 0x0178
	};
	return s_mapping[p_char - 0x80];
}

uinteger_t MCUnicodeMapFromNativeSingleton_MacRoman(unsigned char p_char)
{
	static const unichar_t s_mapping[] =
	{
		0x00c4, 0x00c5, 0x00c7, 0x00c9, 0x00d1, 0x00d6, 0x00dc, 0x00e1, 0x00e0, 0x00e2, 0x00e4, 0x00e3,
		0x00e5, 0x00e7, 0x00e9, 0x00e8, 0x00ea, 0x00eb, 0x00ed, 0x00ec, 0x00ee, 0x00ef, 0x00f1, 0x00f3,
		0x00f2, 0x00f4, 0x00f6, 0x00f5, 0x00fa, 0x00f9, 0x00fb, 0x00fc, 0x2020, 0x00b0, 0x00a2, 0x00a3,
		0x00a7, 0x2022, 0x00b6, 0x00df, 0x00ae, 0x00a9, 0x2122, 0x00b4, 0x00a8, 0x2260, 0x00c6, 0x00d8,
		0x221e, 0x00b1, 0x2264, 0x2265, 0x00a5, 0x00b5, 0x2202, 0x2211, 0x220f, 0x03c0, 0x222b, 0x00aa,
		0x00ba, 0x03a9, 0x00e6, 0x00f8, 0x00bf, 0x00a1, 0x00ac, 0x221a, 0x0192, 0x2248, 0x2206, 0x00ab,
		0x00bb, 0x2026, 0x00a0, 0x00c0, 0x00c3, 0x00d5, 0x0152, 0x0153, 0x2013, 0x2014, 0x201c, 0x201d,
		0x2018, 0x2019, 0x00f7, 0x25ca, 0x00ff, 0x0178, 0x2044, 0x20ac, 0x2039, 0x203a, 0xfb01, 0xfb02,
		0x2021, 0x00b7, 0x201a, 0x201e, 0x2030, 0x00c2, 0x00ca, 0x00c1, 0x00cb, 0x00c8, 0x00cd, 0x00ce,
		0x00cf, 0x00cc, 0x00d3, 0x00d4, 0xf8ff, 0x00d2, 0x00da, 0x00db, 0x00d9, 0x0131, 0x02c6, 0x02dc,
		0x00af, 0x02d8, 0x02d9, 0x02da, 0x00b8, 0x02dd, 0x02db, 0x02c7
	};
	return s_mapping[p_char - 0x80];
}

uinteger_t MCUnicodeMapFromNativeSingleton_ISO8859_1(unsigned char p_char)
{
	// There are no native singleton special cases for ISO8859-1.
	return 0;
}

////////////////////////////////////////////////////////////////////////////////

bool MCUnicodeCodepointIsCombiner(uinteger_t x)
{
	static const uinteger_t s_combiners[] =
	{
		0x4f000300, 0x20000350, 0x04000483, 0x2d000591, 0x010005bf, 0x020005c1,
		0x020005c4, 0x010005c7, 0x06000610, 0x1400064b, 0x01000670, 0x070006d6,
		0x060006df, 0x020006e7, 0x040006ea, 0x01000711, 0x1b000730, 0x090007eb,
		0x0100093c, 0x0100094d, 0x04000951, 0x010009bc, 0x010009cd, 0x01000a3c,
		0x01000a4d, 0x01000abc, 0x01000acd, 0x01000b3c, 0x01000b4d, 0x01000bcd,
		0x01000c4d, 0x02000c55, 0x01000cbc, 0x01000ccd, 0x01000d4d, 0x01000dca,
		0x03000e38, 0x04000e48, 0x02000eb8, 0x04000ec8, 0x02000f18, 0x01000f35,
		0x01000f37, 0x01000f39, 0x02000f71, 0x01000f74, 0x04000f7a, 0x01000f80,
		0x03000f82, 0x02000f86, 0x01000fc6, 0x01001037, 0x01001039, 0x0100135f,
		0x01001714, 0x01001734, 0x010017d2, 0x010017dd, 0x010018a9, 0x03001939,
		0x02001a17, 0x01001b34, 0x01001b44, 0x09001b6b, 0x0b001dc0, 0x02001dfe,
		0x0d0020d0, 0x010020e1, 0x0b0020e5, 0x0600302a, 0x02003099, 0x0100a806,
		0x0100fb1e, 0x0400fe20, 0x01010a0d, 0x01010a0f, 0x03010a38, 0x01010a3f,
		0x0501d165, 0x0601d16d, 0x0801d17b, 0x0701d185, 0x0401d1aa, 0x0301d242
	};
	
	uinteger_t t_low, t_high;
	t_low = 0;
	t_high = sizeof(s_combiners) / sizeof(uinteger_t);
	while(t_low < t_high)
	{
		uinteger_t t_mid;
		t_mid = t_low + (t_high - t_low) / 2;
		
		uinteger_t t_start, t_finish;
		t_start = s_combiners[t_mid] & 0xffffff;
		t_finish = t_start + (s_combiners[t_mid] >> 24);
		
		if (x < t_start)
			t_high = t_mid;
		else if (x > t_finish)
			t_low = t_mid + 1;
		else
			return true;
	}
	
	return false;
}

bool MCUnicodeCodepointIsIdeographicLookup(uinteger_t x)
{
	static const uinteger_t s_ideographs[] =
	{
        0x0e010e3a, 0x0e400e4e, 0x0e810e82, 0x0e840e84, 0x0e870e88, 0x0e8a0e8a, 0x0e8d0e8d, 0x0e940e97,
        0x0e990e9f, 0x0ea10ea3, 0x0ea50ea5, 0x0ea70ea7, 0x0eaa0eab, 0x0ead0eb9, 0x0ebb0ebd, 0x0ec00ec4,
        0x0ec60ec6, 0x0ec80ecd, 0x0edc0edd, 0x10001021, 0x10231027, 0x1029102a, 0x102c1032, 0x10361039,
        0x10501059, 0x11001159, 0x115f11a2, 0x11a811f9, 0x178017d3, 0x17d717d7, 0x17dc17dd, 0x1950196d,
        0x19701974, 0x198019a9, 0x19b019c9, 0x19de19df, 0x2e802e99, 0x2e9b2ef3, 0x2f002fd5, 0x2ff02ffb,
        0x30003000, 0x30033004, 0x30063007, 0x30123013, 0x30203029, 0x3030303a, 0x303d303f, 0x30423042,
        0x30443044, 0x30463046, 0x30483048, 0x304a3062, 0x30643082, 0x30843084, 0x30863086, 0x3088308d,
        0x308f3094, 0x309f309f, 0x30a230a2, 0x30a430a4, 0x30a630a6, 0x30a830a8, 0x30aa30c2, 0x30c430e2,
        0x30e430e4, 0x30e630e6, 0x30e830ed, 0x30ef30f4, 0x30f730fa, 0x30ff30ff, 0x3105312c, 0x3131318e,
        0x319031b7, 0x31c031cf, 0x3200321e, 0x32203243, 0x325032fe, 0x33004db5, 0x4e009fbb, 0xa000a014,
        0xa016a48c, 0xa490a4c6, 0xac00d7a3, 0xf900fa2d, 0xfa30fa6a, 0xfa70fad9, 0xfe30fe34, 0xfe45fe46,
        0xfe49fe4f, 0xfe51fe51, 0xfe58fe58, 0xfe5ffe66, 0xfe68fe68, 0xfe6bfe6b, 0xff02ff03, 0xff06ff07,
        0xff0aff0b, 0xff0dff0d, 0xff0fff19, 0xff1cff1e, 0xff20ff3a, 0xff3cff3c, 0xff3eff5a, 0xff5cff5c,
        0xff5eff5e, 0xffe2ffe4,
	};
	
	uinteger_t t_low, t_high;
	t_low = 0;
	t_high = sizeof(s_ideographs) / sizeof(uinteger_t);
	while(t_low < t_high)
	{
		uinteger_t t_mid;
		t_mid = t_low + (t_high - t_low) / 2;
		
		uinteger_t t_start, t_finish;
		t_start = s_ideographs[t_mid] >> 16;
		t_finish = s_ideographs[t_mid] & 0xffff;
        
		if (x < t_start)
			t_high = t_mid;
		else if (x > t_finish)
			t_low = t_mid + 1;
		else
			return true;
	}
	
	return false;
}

uinteger_t MCUnicodeCodepointGetBreakClass(uinteger_t x)
{
	static const uinteger_t s_break_classes[] =
	{
        0x00000021, 0x000e0045, 0x00210001, 0x00220002, 0x00270006, 0x00290001, 0x003f0001, 0x005b0002,
        0x005d0001, 0x007b0002, 0x007d0001, 0x007f0015, 0x00860065, 0x00a00003, 0x00ab0002, 0x00bb0002,
        0x03000139, 0x034f0003, 0x0350002d, 0x035c001b, 0x03630031, 0x0483000d, 0x04880005, 0x059100b1,
        0x05bf0001, 0x05c10005, 0x05c4000d, 0x060c0001, 0x06100015, 0x061b0001, 0x061e0005, 0x064b004d,
        0x066a0001, 0x06700001, 0x06d40001, 0x06d60019, 0x06de0019, 0x06e70005, 0x06ea000d, 0x07110001,
        0x07300069, 0x07a60029, 0x07eb0021, 0x07f90001, 0x09010009, 0x093c0001, 0x093e003d, 0x0951000d,
        0x09620005, 0x09810009, 0x09bc0001, 0x09be0019, 0x09c70005, 0x09cb0009, 0x09d70001, 0x09e20005,
        0x0a010009, 0x0a3c0001, 0x0a3e0011, 0x0a470005, 0x0a4b0009, 0x0a700005, 0x0a810009, 0x0abc0001,
        0x0abe001d, 0x0ac70009, 0x0acb0009, 0x0ae20005, 0x0b010009, 0x0b3c0001, 0x0b3e0015, 0x0b470005,
        0x0b4b0009, 0x0b560005, 0x0b820001, 0x0bbe0011, 0x0bc60009, 0x0bca000d, 0x0bd70001, 0x0c010009,
        0x0c3e0019, 0x0c460009, 0x0c4a000d, 0x0c550005, 0x0c820005, 0x0cbc0001, 0x0cbe0019, 0x0cc60009,
        0x0cca000d, 0x0cd50005, 0x0ce20005, 0x0d020005, 0x0d3e0015, 0x0d460009, 0x0d4a000d, 0x0d570001,
        0x0d820005, 0x0dca0001, 0x0dcf0015, 0x0dd60001, 0x0dd8001d, 0x0df20005, 0x0f080003, 0x0f0c0003,
        0x0f0d0011, 0x0f120003, 0x0f140001, 0x0f180005, 0x0f350001, 0x0f370001, 0x0f390001, 0x0f3a0002,
        0x0f3b0001, 0x0f3c0002, 0x0f3d0009, 0x0f710035, 0x0f800011, 0x0f860005, 0x0f90001d, 0x0f99008d,
        0x0fc60001, 0x135f0001, 0x169b0002, 0x169c0001, 0x17120009, 0x17320009, 0x17520005, 0x17720005,
        0x180b0009, 0x180e0003, 0x18a90001, 0x1920002d, 0x1930002d, 0x19440005, 0x1a170011, 0x1b000011,
        0x1b340041, 0x1b6b0021, 0x1dc00029, 0x1dfe0005, 0x20070003, 0x200c000d, 0x20110003, 0x2018001e,
        0x202a0011, 0x202f0003, 0x20390006, 0x20450002, 0x20460001, 0x20600003, 0x206a0015, 0x207d0002,
        0x207e0001, 0x208d0002, 0x208e0001, 0x20d0007d, 0x23290002, 0x232a0001, 0x275b000e, 0x27620005,
        0x27680002, 0x27690001, 0x276a0002, 0x276b0001, 0x276c0002, 0x276d0001, 0x276e0002, 0x276f0001,
        0x27700002, 0x27710001, 0x27720002, 0x27730001, 0x27740002, 0x27750001, 0x27c50002, 0x27c60001,
        0x27e60002, 0x27e70001, 0x27e80002, 0x27e90001, 0x27ea0002, 0x27eb0001, 0x29830002, 0x29840001,
        0x29850002, 0x29860001, 0x29870002, 0x29880001, 0x29890002, 0x298a0001, 0x298b0002, 0x298c0001,
        0x298d0002, 0x298e0001, 0x298f0002, 0x29900001, 0x29910002, 0x29920001, 0x29930002, 0x29940001,
        0x29950002, 0x29960001, 0x29970002, 0x29980001, 0x29d80002, 0x29d90001, 0x29da0002, 0x29db0001,
        0x29fc0002, 0x29fd0001, 0x2e000036, 0x2e1c0006, 0x30010005, 0x30080002, 0x30090001, 0x300a0002,
        0x300b0001, 0x300c0002, 0x300d0001, 0x300e0002, 0x300f0001, 0x30100002, 0x30110001, 0x30140002,
        0x30150001, 0x30160002, 0x30170001, 0x30180002, 0x30190001, 0x301a0002, 0x301b0001, 0x301d0002,
        0x301e0005, 0x302a0015, 0x30990005, 0xa8020001, 0xa8060001, 0xa80b0001, 0xa8230011, 0xa8760005,
        0xfb1e0001, 0xfd3e0002, 0xfd3f0001, 0xfe00003d, 0xfe110005, 0xfe150005, 0xfe170002, 0xfe180001,
        0xfe20000d, 0xfe350002, 0xfe360001, 0xfe370002, 0xfe380001, 0xfe390002, 0xfe3a0001, 0xfe3b0002,
        0xfe3c0001, 0xfe3d0002, 0xfe3e0001, 0xfe3f0002, 0xfe400001, 0xfe410002, 0xfe420001, 0xfe430002,
        0xfe440001, 0xfe470002, 0xfe480001, 0xfe500001, 0xfe520001, 0xfe560005, 0xfe590002, 0xfe5a0001,
        0xfe5b0002, 0xfe5c0001, 0xfe5d0002, 0xfe5e0001, 0xfeff0003, 0xff010001, 0xff080002, 0xff090001,
        0xff0c0001, 0xff0e0001, 0xff1f0001, 0xff3b0002, 0xff3d0001, 0xff5b0002, 0xff5d0001, 0xff5f0002,
        0xff600005, 0xff620002, 0xff630005, 0xfff90009,
	};
    
	uinteger_t t_low, t_high;
	t_low = 0;
	t_high = sizeof(s_break_classes) / sizeof(uinteger_t);
	while(t_low < t_high)
	{
		uinteger_t t_mid;
		t_mid = t_low + (t_high - t_low) / 2;
		
		uinteger_t t_start, t_finish;
		t_start = s_break_classes[t_mid] >> 16;
		t_finish = t_start + ((s_break_classes[t_mid] & 0xffff) >> 2);
        
		if (x < t_start)
			t_high = t_mid;
		else if (x > t_finish)
			t_low = t_mid + 1;
		else
			return s_break_classes[t_mid] & 0x3;
	}
    
	return 0;
}

enum
{
	kMCUnicodeWordBreakClassNone,
	kMCUnicodeWordBreakClassKatakana,
	kMCUnicodeWordBreakClassALetter,
	kMCUnicodeWordBreakClassMidLetter,
	kMCUnicodeWordBreakClassMidNum,
	kMCUnicodeWordBreakClassMidNumLet,
	kMCUnicodeWordBreakClassNumeric,
	kMCUnicodeWordBreakClassExtendNumLet
};

uinteger_t MCUnicodeCodepointGetWordBreakClass(uinteger_t x)
{
	static uinteger_t s_word_break_classes[] =
	{
		0x00270005, 0x002c0004, 0x002e0004, 0x0030004e, 0x003a0003, 0x003b0004, 0x004100ca, 0x005f0007,
		0x006100ca, 0x00aa0002, 0x00ad0002, 0x00b50002, 0x00b70003, 0x00ba0002, 0x00c000b2, 0x00d800f2,
		0x00f80e4a, 0x02c6005a, 0x02e00022, 0x02ec0002, 0x02ee0002, 0x030003a2, 0x0376000a, 0x037a001a,
		0x037e0004, 0x03860002, 0x03870003, 0x03880012, 0x038c0002, 0x038e009a, 0x03a30292, 0x03f70452,
		0x04830502, 0x0531012a, 0x05590002, 0x05610132, 0x05890004, 0x05910162, 0x05bf0002, 0x05c1000a,
		0x05c4000a, 0x05c70002, 0x05d000d2, 0x05f0001a, 0x05f40003, 0x0600001a, 0x060c000c, 0x06100052,
		0x062101ea, 0x0660004e, 0x066b0006, 0x066c0004, 0x066e032a, 0x06d5009a, 0x06ea002a, 0x06f0004e,
		0x06fa0012, 0x06ff0002, 0x070f01da, 0x074d0322, 0x07c0004e, 0x07ca015a, 0x07f80004, 0x07fa0002,
		0x090101c2, 0x093c008a, 0x09500022, 0x0958005a, 0x0966004e, 0x0971000a, 0x097b0022, 0x09810012,
		0x0985003a, 0x098f000a, 0x099300aa, 0x09aa0032, 0x09b20002, 0x09b6001a, 0x09bc0042, 0x09c7000a,
		0x09cb001a, 0x09d70002, 0x09dc000a, 0x09df0022, 0x09e6004e, 0x09f0000a, 0x0a010012, 0x0a05002a,
		0x0a0f000a, 0x0a1300aa, 0x0a2a0032, 0x0a32000a, 0x0a35000a, 0x0a38000a, 0x0a3c0002, 0x0a3e0022,
		0x0a47000a, 0x0a4b0012, 0x0a510002, 0x0a59001a, 0x0a5e0002, 0x0a66004e, 0x0a70002a, 0x0a810012,
		0x0a850042, 0x0a8f0012, 0x0a9300aa, 0x0aaa0032, 0x0ab2000a, 0x0ab50022, 0x0abc004a, 0x0ac70012,
		0x0acb0012, 0x0ad00002, 0x0ae0001a, 0x0ae6004e, 0x0b010012, 0x0b05003a, 0x0b0f000a, 0x0b1300aa,
		0x0b2a0032, 0x0b32000a, 0x0b350022, 0x0b3c0042, 0x0b47000a, 0x0b4b0012, 0x0b56000a, 0x0b5c000a,
		0x0b5f0022, 0x0b66004e, 0x0b710002, 0x0b82000a, 0x0b85002a, 0x0b8e0012, 0x0b92001a, 0x0b99000a,
		0x0b9c0002, 0x0b9e000a, 0x0ba3000a, 0x0ba80012, 0x0bae005a, 0x0bbe0022, 0x0bc60012, 0x0bca001a,
		0x0bd00002, 0x0bd70002, 0x0be6004e, 0x0c010012, 0x0c05003a, 0x0c0e0012, 0x0c1200b2, 0x0c2a004a,
		0x0c350022, 0x0c3d003a, 0x0c460012, 0x0c4a001a, 0x0c55000a, 0x0c58000a, 0x0c60001a, 0x0c66004e,
		0x0c82000a, 0x0c85003a, 0x0c8e0012, 0x0c9200b2, 0x0caa004a, 0x0cb50022, 0x0cbc0042, 0x0cc60012,
		0x0cca001a, 0x0cd5000a, 0x0cde0002, 0x0ce0001a, 0x0ce6004e, 0x0d02000a, 0x0d05003a, 0x0d0e0012,
		0x0d1200b2, 0x0d2a007a, 0x0d3d003a, 0x0d460012, 0x0d4a001a, 0x0d570002, 0x0d60001a, 0x0d66004e,
		0x0d7a002a, 0x0d82000a, 0x0d85008a, 0x0d9a00ba, 0x0db30042, 0x0dbd0002, 0x0dc00032, 0x0dca0002,
		0x0dcf002a, 0x0dd60002, 0x0dd8003a, 0x0df2000a, 0x0e310002, 0x0e340032, 0x0e47003a, 0x0e50004e,
		0x0eb10002, 0x0eb4002a, 0x0ebb000a, 0x0ec8002a, 0x0ed0004e, 0x0f000002, 0x0f18000a, 0x0f20004e,
		0x0f350002, 0x0f370002, 0x0f390002, 0x0f3e004a, 0x0f49011a, 0x0f71009a, 0x0f86002a, 0x0f90003a,
		0x0f99011a, 0x0fc60002, 0x102b009a, 0x1040004e, 0x1056001a, 0x105e0012, 0x10620012, 0x10670032,
		0x1071001a, 0x1082005a, 0x108f0002, 0x1090004e, 0x10a0012a, 0x10d00152, 0x10fc0002, 0x110002ca,
		0x115f021a, 0x11a8028a, 0x12000242, 0x124a001a, 0x12500032, 0x12580002, 0x125a001a, 0x12600142,
		0x128a001a, 0x12900102, 0x12b2001a, 0x12b80032, 0x12c00002, 0x12c2001a, 0x12c80072, 0x12d801c2,
		0x1312001a, 0x13180212, 0x135f0002, 0x1380007a, 0x13a002a2, 0x1401135a, 0x166f003a, 0x168100ca,
		0x16a00252, 0x16ee0012, 0x17000062, 0x170e0032, 0x172000a2, 0x1740009a, 0x17600062, 0x176e0012,
		0x1772000a, 0x17b400fa, 0x17dd0002, 0x17e0004e, 0x180b0012, 0x1810004e, 0x182002ba, 0x18800152,
		0x190000e2, 0x1920005a, 0x1930005a, 0x1946004e, 0x19b00082, 0x19c8000a, 0x19d0004e, 0x1a0000da,
		0x1b00025a, 0x1b50004e, 0x1b6b0042, 0x1b800152, 0x1bae000a, 0x1bb0004e, 0x1c0001ba, 0x1c40004e,
		0x1c4d0012, 0x1c50004e, 0x1c5a011a, 0x1d000732, 0x1dfe08ba, 0x1f18002a, 0x1f20012a, 0x1f48002a,
		0x1f50003a, 0x1f590002, 0x1f5b0002, 0x1f5d0002, 0x1f5f00f2, 0x1f8001a2, 0x1fb60032, 0x1fbe0002,
		0x1fc20012, 0x1fc60032, 0x1fd0001a, 0x1fd6002a, 0x1fe00062, 0x1ff20012, 0x1ff60032, 0x200b0022,
		0x2018000d, 0x20240005, 0x20270003, 0x202a0022, 0x203f000f, 0x20440004, 0x20540007, 0x20600022,
		0x206a002a, 0x20710002, 0x207f0002, 0x20900022, 0x20d00102, 0x21020002, 0x21070002, 0x210a004a,
		0x21150002, 0x21190022, 0x21240002, 0x21260002, 0x21280002, 0x212a001a, 0x212f0052, 0x213c001a,
		0x21450022, 0x214e0002, 0x21600142, 0x24b6019a, 0x2c000172, 0x2c300172, 0x2c60007a, 0x2c710062,
		0x2c800322, 0x2d00012a, 0x2d3001aa, 0x2d6f0002, 0x2d8000b2, 0x2da00032, 0x2da80032, 0x2db00032,
		0x2db80032, 0x2dc00032, 0x2dc80032, 0x2dd00032, 0x2dd80032, 0x2de000fa, 0x2e2f0002, 0x30050002,
		0x302a002a, 0x30310021, 0x303b000a, 0x3099000a, 0x309b0009, 0x30a002d1, 0x30fc0019, 0x31050142,
		0x313102ea, 0x31a000ba, 0x31f00079, 0x32d00171, 0x330002b9, 0xa0002462, 0xa5000862, 0xa610007a,
		0xa620004e, 0xa62a000a, 0xa64000fa, 0xa6620082, 0xa67c000a, 0xa67f00c2, 0xa7170042, 0xa7220332,
		0xa78b000a, 0xa7fb0162, 0xa840019a, 0xa8800222, 0xa8d0004e, 0xa900004e, 0xa90a011a, 0xa930011a,
		0xaa0001b2, 0xaa40006a, 0xaa50004e, 0xac00fffa, 0xcc005d1a, 0xd800fffa, 0xf80007fa, 0xfb000032,
		0xfb130022, 0xfb1d005a, 0xfb2a0062, 0xfb380022, 0xfb3e0002, 0xfb40000a, 0xfb43000a, 0xfb46035a,
		0xfbd30b52, 0xfd5001fa, 0xfd9201aa, 0xfdf0005a, 0xfe00007a, 0xfe100004, 0xfe130003, 0xfe140004,
		0xfe200032, 0xfe33000f, 0xfe4d0017, 0xfe500004, 0xfe520005, 0xfe540004, 0xfe550003, 0xfe700022,
		0xfe760432, 0xfeff0002, 0xff070005, 0xff0c0004, 0xff0e0005, 0xff1a0003, 0xff1b0004, 0xff2100ca,
		0xff3f0007, 0xff4100ca, 0xff6601b9, 0xff9e0102, 0xffc2002a, 0xffca002a, 0xffd2002a, 0xffda0012,
		0xfff90012
	};
    
	uinteger_t t_low, t_high;
	t_low = 0;
	t_high = sizeof(s_word_break_classes) / sizeof(uinteger_t);
	while(t_low < t_high)
	{
		uinteger_t t_mid;
		t_mid = t_low + (t_high - t_low) / 2;
		
		uinteger_t t_start, t_finish;
		t_start = s_word_break_classes[t_mid] >> 16;
		t_finish = t_start + ((s_word_break_classes[t_mid] & 0xffff) >> 3);
        
		if (x < t_start)
			t_high = t_mid;
		else if (x > t_finish)
			t_low = t_mid + 1;
		else
			return s_word_break_classes[t_mid] & 0x7;
	}
    
	return 0;
}

// MW-2008-11-03: [[ Bug ]] The rule ExtendNumLet * ExtendNumLet was missing thus stopping
//   a__b being selected as a single word.
bool MCUnicodeCanBreakWordBetween(uinteger_t xc, uinteger_t x, uinteger_t y, uinteger_t yc)
{
	// Our simplified rules are:
	//   WB5:                          ALetter * ALetter
	//   WB6:                          ALetter * (MidLetter | MidNumLet) ALetter
	//   WB7:  ALetter (MidLetter | MidNumLet) * ALetter
	//   WB8:                          Numeric * Numeric
	//   WB9:                          ALetter * Numeric
	//   WB10:                         Numeric * ALetter
	//   WB11:    Numeric (MidNum | MidNumLet) * Numeric
	//   WB12:                         Numeric * (MidNum | MidNumLet) Numeric
	//   WB13:                        Katakana * Katakana
	//   WB13a: (ALetter | Numeric | Katakana | ExtendNumLet) * ExtendNumLet
	//   WB13b:                   ExtendNumLet * (ALetter | Numeric | Katakana)
    
	// We perform these rule checks as follows:
	// if (x is ALetter)
	//   WB5
	//   WB9
	//   WB13a-ALetter
	//   WB6
	// if (x is Numeric)
	//   WB8
	//   WB10
	//   W13a-Numeric
	//   WB12
	// if (x is Katakana)
	//   WB13
	//   WB13a-Katakana
	// if (y is ALetter)
	//   WB13b-ALetter
	//   WB7
	// if (y is Numeric)
	//   WB13b-Numeric
	//   WB11
	// if (y is Katakana)
	//   WB13b-Katakana
	// if (y is ExtendNumLet)
	//   WB13-ExtendNumLet
    
	uinteger_t x_class;
	x_class = MCUnicodeCodepointGetWordBreakClass(x);
    
	uinteger_t y_class;
	y_class = MCUnicodeCodepointGetWordBreakClass(y);
    
	switch(x_class)
	{
        case kMCUnicodeWordBreakClassALetter:
            // WB5
            if (y_class == kMCUnicodeWordBreakClassALetter)
                return false;
            
            // WB9
            if (y_class == kMCUnicodeWordBreakClassNumeric)
                return false;
            
            // WB13a
            if (y_class == kMCUnicodeWordBreakClassExtendNumLet)
                return false;
            
            // WB6
            if (y_class == kMCUnicodeWordBreakClassMidLetter || y_class == kMCUnicodeWordBreakClassMidNumLet)
            {
                uinteger_t yc_class;
                yc_class = MCUnicodeCodepointGetWordBreakClass(yc);
                if (yc_class == kMCUnicodeWordBreakClassALetter)
                    return false;
            }
            break;
            
        case kMCUnicodeWordBreakClassNumeric:
            // WB8
            if (y_class == kMCUnicodeWordBreakClassNumeric)
                return false;
            
            // WB10
            if (y_class == kMCUnicodeWordBreakClassALetter)
                return false;
            
            // WB13a
            if (y_class == kMCUnicodeWordBreakClassExtendNumLet)
                return false;
            
            // WB12
            if (y_class == kMCUnicodeWordBreakClassMidNum || y_class == kMCUnicodeWordBreakClassMidNumLet)
            {
                uinteger_t yc_class;
                yc_class = MCUnicodeCodepointGetWordBreakClass(yc);
                if (yc_class == kMCUnicodeWordBreakClassNumeric)
                    return false;
            }
            break;
            
        case kMCUnicodeWordBreakClassKatakana:
            // WB13
            if (y_class == kMCUnicodeWordBreakClassKatakana)
                return false;
            
            // WB13a
            if (y_class == kMCUnicodeWordBreakClassExtendNumLet)
                return false;
            break;
	}
    
	switch(y_class)
	{
        case kMCUnicodeWordBreakClassALetter:
            // WB13b
            if (x_class == kMCUnicodeWordBreakClassExtendNumLet)
                return false;
            
            // WB7
            if (x_class == kMCUnicodeWordBreakClassMidLetter || x_class == kMCUnicodeWordBreakClassMidNumLet)
            {
                uinteger_t xc_class;
                xc_class = MCUnicodeCodepointGetWordBreakClass(xc);
                if (xc_class == kMCUnicodeWordBreakClassALetter)
                    return false;
            }
            break;
            
        case kMCUnicodeWordBreakClassNumeric:
            // WB13b
            if (x_class == kMCUnicodeWordBreakClassExtendNumLet)
                return false;
            
            // WB11
            if (x_class == kMCUnicodeWordBreakClassMidNum || x_class == kMCUnicodeWordBreakClassMidNumLet)
            {
                uinteger_t xc_class;
                xc_class = MCUnicodeCodepointGetWordBreakClass(xc);
                if (xc_class == kMCUnicodeWordBreakClassNumeric)
                    return false;
            }
            break;
            
        case kMCUnicodeWordBreakClassKatakana:
        case kMCUnicodeWordBreakClassExtendNumLet:
			// WB13b
            if (x_class == kMCUnicodeWordBreakClassExtendNumLet)
                return false;
            break;
	}
    
	return true;
}

////////////////////////////////////////////////////////////////////////////////

bool MCUnicodeWildcardMatch(const void *source_chars, uindex_t source_length, bool p_source_native, const void *pattern_chars, uindex_t pattern_length, bool p_pattern_native, MCUnicodeCompareOption p_option)
{
    // Set up the filter chains for the strings
    MCAutoPointer<MCTextFilter> t_source_filter =
        MCTextFilterCreate(source_chars, source_length, p_source_native ? kMCStringEncodingNative : kMCStringEncodingUTF16, p_option);
	
    MCAutoPointer<MCTextFilter> t_pattern_filter =
        MCTextFilterCreate(pattern_chars, pattern_length, p_pattern_native ? kMCStringEncodingNative : kMCStringEncodingUTF16, p_option);
    
    codepoint_t t_source_cp, t_pattern_cp;
    
    while (t_source_filter -> HasData() && t_pattern_filter -> HasData())
    {
        t_source_cp = t_source_filter -> GetNextCodepoint();
        t_pattern_cp = t_pattern_filter -> GetNextCodepoint();
        switch (t_pattern_cp)
        {
            case '[':
            {
                // Records whether we currently have a match for this bracket.
				bool ok = false;
                // Records whether we have yet seen anything to match within these brackets.
                bool t_character_found = false;
                // Store the 'lower limit' if we have a range.
                codepoint_t t_lower_limit = 0;
                
				int notflag = 0;
                t_pattern_filter -> AdvanceCursor();
                t_pattern_cp = t_pattern_filter -> GetNextCodepoint();
				if (t_pattern_cp == '!' )
				{
					notflag = 1;
                    t_pattern_filter -> AdvanceCursor();
                    t_pattern_cp = t_pattern_filter -> GetNextCodepoint();
				}
				while (t_pattern_filter -> HasData())
				{
                    t_pattern_filter -> MarkText();
					if (t_pattern_cp == ']' && t_character_found)
                    {
                        // if the bracket was close with no match found then return false;
                        if (!ok)
                            return false;
                        
                        // otherwise, recurse.
                        uindex_t t_sindex, t_pindex;
                        // update the read position.
                        t_source_filter -> AdvanceCursor();
                        t_source_filter -> GetNextCodepoint();
                        t_source_filter -> MarkText();
                        
                        t_pattern_filter -> AdvanceCursor();
                        t_pattern_filter -> GetNextCodepoint();
                        t_pattern_filter -> MarkText();
                        
                        t_sindex = t_source_filter -> GetMarkedLength() - 1;
                        t_pindex = t_pattern_filter -> GetMarkedLength() - 1;
                        
						return MCUnicodeWildcardMatch((const char *)source_chars + (p_source_native ? t_sindex : (t_sindex * 2)), source_length - t_sindex, p_source_native, (const char *)pattern_chars + (p_pattern_native ? t_pindex : (t_pindex * 2)), pattern_length - t_pindex, p_pattern_native, p_option);
                    }
					else
                    {
                        t_pattern_filter -> AdvanceCursor();
						if (t_pattern_cp == '-' && t_character_found && t_pattern_filter -> GetNextCodepoint() != ']')
                        {
                            // We have a char range (eg [a-z]), so skip past the '-',
                            // find the current pattern grapheme range and compare.
                            t_pattern_cp = t_pattern_filter -> GetNextCodepoint();
                            
							if (notflag)
							{
                                // wer're still ok if the current source grapheme falls outwith the appropriate range. Otherwise, we fail.
								if (t_source_cp < t_lower_limit || t_source_cp > t_pattern_cp)
									ok = true;
								else
									return false;
							}
							else
							{
                                // we're still ok if the current source grapheme falls within the appropriate range.
                                // If not, there may be other options within this pair of brackets
								if (t_source_cp >= t_lower_limit && t_source_cp <= t_pattern_cp)
									ok = true;
							}
						}
						else
						{
                            // This could be one of a choice of characters (eg [abc]).
							if (notflag)
							{
								if (t_source_cp != t_pattern_cp)
                                    ok = true;
								else
									return false;
							}
							else
                                if (t_source_cp == t_pattern_cp)
									ok = true;
                            
                            // record the codepoint in case it is the first character of a range.
                            t_lower_limit = t_pattern_cp;
                            t_character_found = true;
						}
                    }
                    t_pattern_cp = t_pattern_filter -> GetNextCodepoint();
				}
            }
                return false;
            case '?':
            {
                // Matches any character, so increment the pattern index.
                
                break;
            }
            case '*':
            {
                // consume any more * characters.
                while (t_pattern_filter -> HasData())
                {
                    if ((t_pattern_cp = t_pattern_filter -> GetNextCodepoint()) != '*')
                        break;
                    t_pattern_filter -> AdvanceCursor();
                }
                if (!t_pattern_filter -> HasData())
                    return true;
                
                // try and match the rest of the source string recursively.
                while (t_source_filter -> HasData())
                {
                    uindex_t t_sindex, t_pindex;
                    // if this is a candidate for a match, recurse.
                    if (t_source_cp == t_pattern_cp)
                    {
                        // AL-2014-06-24: [[ Bug 12644 ]] Can't advance the cursors here because then
                        //  we're eating the pattern codepoint for free resulting in false positives.
                        //  Just have to re-match in the recursive call.
                        
                        t_source_filter -> MarkText();
                        t_pattern_filter -> MarkText();
                        
                        t_sindex = t_source_filter -> GetMarkedLength() - 1;
                        t_pindex = t_pattern_filter -> GetMarkedLength() - 1;

                        if (MCUnicodeWildcardMatch((const char *)source_chars + (p_source_native ? t_sindex : (t_sindex * 2)), source_length - t_sindex, p_source_native, (const char *)pattern_chars + (p_pattern_native ? t_pindex : (t_pindex * 2)), pattern_length - t_pindex, p_pattern_native, p_option))
                            return true;
                    }
                    else if (t_pattern_cp == '?' || t_pattern_cp == '[')
                    {
                        t_source_filter -> MarkText();
                        t_pattern_filter -> MarkText();
                        
                        t_sindex = t_source_filter -> GetMarkedLength() - 1;
                        t_pindex = t_pattern_filter -> GetMarkedLength() - 1;
                        
                        if (MCUnicodeWildcardMatch((const char *)source_chars + (p_source_native ? t_sindex : (t_sindex * 2)), source_length - t_sindex, p_source_native, (const char *)pattern_chars + (p_pattern_native ? t_pindex : (t_pindex * 2)), pattern_length - t_pindex, p_pattern_native, p_option))
                            return true;
                    }
                    
                    // if we don't find a match, eat the source codepoint and continue.
                    t_source_filter -> AdvanceCursor();
                    t_source_cp = t_source_filter -> GetNextCodepoint();
                }
            }
                return false;
            case 0:
                return t_source_cp == 0;
            default:
                // default - just compare chars
                if (t_source_cp != t_pattern_cp)
                    return false;
                
                break;
		}
        t_source_filter -> AdvanceCursor();
        t_pattern_filter -> AdvanceCursor();
	}
    // Eat any remaining '*'s
    while (t_pattern_filter -> HasData())
    {
        if ((t_pattern_cp = t_pattern_filter -> GetNextCodepoint()) != '*')
            return false;
        t_pattern_filter -> AdvanceCursor();
    }
    
    if (t_source_filter -> HasData())
        return false;
    
    return true;
}

////////////////////////////////////////////////////////////////////////////////

// Implement rules GB 6 - 8 based on Hangul syllable type
static bool __MCUnicodeIsHangulClusterBoundary(int32_t p_left, int32_t p_right)
{
    switch (p_left)
    {
        case U_GCB_L:
            return p_right == U_GCB_T;
        case U_GCB_LV:
        case U_GCB_V:
            return p_right != U_GCB_V && p_right != U_GCB_T;
        case U_GCB_LVT:
        case U_GCB_T:
            return p_right != U_GCB_T;
        default:
            MCUnreachableReturn(true);
    }
}

static bool __MCUnicodeIsControl(int32_t p_gcb)
{
    return p_gcb == U_GCB_CR || p_gcb == U_GCB_LF || p_gcb == U_GCB_CONTROL;
}

static bool __MCUnicodeIsHangulSyllable(int32_t p_gcb)
{
    switch (p_gcb)
    {
        case U_GCB_L:
        case U_GCB_LV:
        case U_GCB_LVT:
        case U_GCB_T:
        case U_GCB_V:
            return true;
        default:
            break;
    }
    
    return false;
}

bool MCUnicodeIsGraphemeClusterBoundary(codepoint_t p_left, codepoint_t p_right)
{
    int32_t t_left_gcb;
    t_left_gcb = MCUnicodeGetIntegerProperty(p_left, kMCUnicodePropertyGraphemeClusterBreak);
    
    int32_t t_right_gcb;
    t_right_gcb = MCUnicodeGetIntegerProperty(p_right, kMCUnicodePropertyGraphemeClusterBreak);
    
    // We treat CR LF as 2 graphemes, contrary to GB 3
    /*
    if (t_left_gcb == U_GCB_CR && t_right_gcb == U_GCB_LF)
        return false;
    */
    
    // GB 4: Break after controls
    if (__MCUnicodeIsControl(t_left_gcb))
        return true;
    
    // GB 5: Break before controls
    if (__MCUnicodeIsControl(t_right_gcb))
        return true;
    
    // GB 6 - 8: Do not break Hangul syllable sequences.
    if (__MCUnicodeIsHangulSyllable(t_left_gcb) && __MCUnicodeIsHangulSyllable(t_right_gcb))
    {
        if (!__MCUnicodeIsHangulClusterBoundary(t_left_gcb, t_right_gcb))
            return false;
    }
    
    // GB 8a: Do not break between regional indicator symbols.
    if (t_left_gcb == U_GCB_REGIONAL_INDICATOR && t_right_gcb == U_GCB_REGIONAL_INDICATOR)
        return false;
    
    // GB 9: Do not break before extending characters.
    if (t_right_gcb == U_GCB_EXTEND)
        return false;

    // GB 9a: Do not break before SpacingMarks
    if (t_right_gcb == U_GCB_SPACING_MARK)
        return false;

    // GB 9b: Do not break after Prepend characters
    if (t_left_gcb == U_GCB_PREPEND)
        return false;
    
    // GB 10: Otherwise, break everywhere.
    return true;
}

////////////////////////////////////////////////////////////////////////////////

