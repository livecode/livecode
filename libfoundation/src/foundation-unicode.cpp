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

#include "foundation-unicode.h"
#include "foundation-unicode-private.h"

#include "unicode/uchar.h"
#include "unicode/ustring.h"
#include "unicode/brkiter.h"
#include "unicode/coll.h"
#include "unicode/normalizer2.h"

#include "foundation-auto.h"

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

bool __MCUnicodeInitialize()
{
    return true;
}

void __MCUnicodeFinalize()
{
    ;
}

////////////////////////////////////////////////////////////////////////////////

codepoint_t MCUnicodeSurrogatesToCodepoint(uint16_t p_lead, uint16_t p_trail)
{
    return 0x10000 + ((p_lead & 0x3FF) << 10) + (p_trail & 0x3FF);
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
            s_pointer = new unichar_t[16];
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
            s_pointer = new unichar_t[16];
            icu::UnicodeString t_string;
            t_string.append(UChar32(p_codepoint));
            t_string.toLower();
            t_string.extract(s_pointer, 16, t_error);
            break;
        }
            
        case kMCUnicodePropertyName:
        {
            // We assume that this is sufficient for a character name
            s_pointer = new unichar_t[256];
            char *t_temp = new char[256];
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
            s_pointer = new unichar_t[256];
            char *t_temp = new char[256];
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
            s_pointer = new unichar_t[16];
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
        if (0xD800 <= t_char && t_char < 0xDC00 && (t_offset + 1) < p_char_count)
        {
            codepoint_t t_upper = p_chars[t_offset + 1];
            if (0xDC00 <= t_upper && t_upper < 0xE000)
            {
                t_char = (t_char - 0xD800) + ((t_upper - 0xDC00) << 10);
                t_advance = 2;
            }
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
                if (t_prop < UINT8_MIN || t_prop > UINT8_MAX)
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
                if (t_prop < UINT16_MIN || t_prop > UINT16_MAX)
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
                if (t_prop < UINT32_MIN || t_prop > UINT32_MAX)
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
        t_temp.append(u_toupper(p_string.char32At(i)/*, U_FOLD_CASE_DEFAULT*/));
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

int32_t MCUnicodeCompare(const unichar_t *p_first, uindex_t p_first_length,
                         const unichar_t *p_second, uindex_t p_second_length,
                         MCUnicodeCompareOption p_option)
{
    // This is a bit more complicated than a plain comparison and requires the
    // construction of UnicodeString objects.
    UErrorCode t_error = U_ZERO_ERROR;
    icu::UnicodeString t_first(p_first, p_first_length);
    icu::UnicodeString t_second(p_second, p_second_length);
    
    // Normalise, if required
    if (p_option == kMCUnicodeCompareOptionCaseless || p_option == kMCUnicodeCompareOptionNormalised)
    {
        // Construct the normaliser
        const icu::Normalizer2 *t_nfc = icu::Normalizer2::getNFCInstance(t_error);
        t_first = t_nfc->normalize(t_first, t_error);
        t_second = t_nfc->normalize(t_second, t_error);
    }
    
    // Case-fold, if required
    if (p_option == kMCUnicodeCompareOptionCaseless || p_option == kMCUnicodeCompareOptionFolded)
    {
        __MCUnicodeSimpleCaseFold(t_first);
        __MCUnicodeSimpleCaseFold(t_second);
    }
    
    // Perform the comparison
    return t_first.compareCodePointOrder(t_second);
}

bool MCUnicodeBeginsWith(const unichar_t *p_first, uindex_t p_first_length,
                         const unichar_t *p_second, uindex_t p_second_length,
                         MCUnicodeCompareOption p_option)
{
    // This is a bit more complicated than a plain comparison and requires the
    // construction of UnicodeString objects.
    UErrorCode t_error = U_ZERO_ERROR;
    icu::UnicodeString t_first(p_first, p_first_length);
    icu::UnicodeString t_second(p_second, p_second_length);
    
    // Normalise, if required
    if (p_option == kMCUnicodeCompareOptionCaseless || p_option == kMCUnicodeCompareOptionNormalised)
    {
        // Construct the normaliser
        const icu::Normalizer2 *t_nfc = icu::Normalizer2::getNFCInstance(t_error);
        t_first = t_nfc->normalize(t_first, t_error);
        t_second = t_nfc->normalize(t_second, t_error);
    }
    
    // Case-fold, if required
    if (p_option == kMCUnicodeCompareOptionCaseless || p_option == kMCUnicodeCompareOptionFolded)
    {
        __MCUnicodeSimpleCaseFold(t_first);
        __MCUnicodeSimpleCaseFold(t_second);
    }
    
    // Perform the comparison
    return t_first.startsWith(t_second);
}

bool MCUnicodeEndsWith(const unichar_t *p_first, uindex_t p_first_length,
                         const unichar_t *p_second, uindex_t p_second_length,
                         MCUnicodeCompareOption p_option)
{
    // This is a bit more complicated than a plain comparison and requires the
    // construction of UnicodeString objects.
    UErrorCode t_error = U_ZERO_ERROR;
    icu::UnicodeString t_first(p_first, p_first_length);
    icu::UnicodeString t_second(p_second, p_second_length);
    
    // Normalise, if required
    if (p_option == kMCUnicodeCompareOptionCaseless || p_option == kMCUnicodeCompareOptionNormalised)
    {
        // Construct the normaliser
        const icu::Normalizer2 *t_nfc = icu::Normalizer2::getNFCInstance(t_error);
        t_first = t_nfc->normalize(t_first, t_error);
        t_second = t_nfc->normalize(t_second, t_error);
    }
    
    // Case-fold, if required
    if (p_option == kMCUnicodeCompareOptionCaseless || p_option == kMCUnicodeCompareOptionFolded)
    {
        __MCUnicodeSimpleCaseFold(t_first);
        __MCUnicodeSimpleCaseFold(t_second);
    }
    
    // Perform the comparison
    return t_first.endsWith(t_second);
}

bool MCUnicodeContains(const unichar_t *p_string, uindex_t p_string_length,
                       const unichar_t *p_needle, uindex_t p_needle_length,
                       MCUnicodeCompareOption p_option)
{
    // This is a bit more complicated than a plain comparison and requires the
    // construction of UnicodeString objects.
    UErrorCode t_error = U_ZERO_ERROR;
    icu::UnicodeString t_string(p_string, p_string_length);
    icu::UnicodeString t_needle(p_needle, p_needle_length);
    
    // Normalise, if required
    if (p_option == kMCUnicodeCompareOptionCaseless || p_option == kMCUnicodeCompareOptionNormalised)
    {
        // Construct the normaliser
        const icu::Normalizer2 *t_nfc = icu::Normalizer2::getNFCInstance(t_error);
        t_string = t_nfc->normalize(t_string, t_error);
        t_needle = t_nfc->normalize(t_needle, t_error);
    }
    
    // Case-fold, if required
    if (p_option == kMCUnicodeCompareOptionCaseless || p_option == kMCUnicodeCompareOptionFolded)
    {
        __MCUnicodeSimpleCaseFold(t_string);
        __MCUnicodeSimpleCaseFold(t_needle);
    }
    
    // Perform the comparison
    return t_string.indexOf(t_needle) >= 0;
}

bool MCUnicodeFirstIndexOf(const unichar_t *p_string, uindex_t p_string_length,
                           const unichar_t *p_needle, uindex_t p_needle_length,
                           MCUnicodeCompareOption p_option, uindex_t &r_index)
{
    // This is a bit more complicated than a plain comparison and requires the
    // construction of UnicodeString objects.
    UErrorCode t_error = U_ZERO_ERROR;
    icu::UnicodeString t_string(p_string, p_string_length);
    icu::UnicodeString t_needle(p_needle, p_needle_length);
    
    // Normalise, if required
    if (p_option == kMCUnicodeCompareOptionCaseless || p_option == kMCUnicodeCompareOptionNormalised)
    {
        // Construct the normaliser
        const icu::Normalizer2 *t_nfc = icu::Normalizer2::getNFCInstance(t_error);
        t_string = t_nfc->normalize(t_string, t_error);
        t_needle = t_nfc->normalize(t_needle, t_error);
    }
    
    // Case-fold, if required
    if (p_option == kMCUnicodeCompareOptionCaseless || p_option == kMCUnicodeCompareOptionFolded)
    {
        __MCUnicodeSimpleCaseFold(t_string);
        __MCUnicodeSimpleCaseFold(t_needle);
    }
    
    // Perform the comparison
    int32_t t_index = t_string.indexOf(t_needle);
    if (t_index < 0)
        return false;
    r_index = t_index;
    return true;
}

bool MCUnicodeLastIndexOf(const unichar_t *p_string, uindex_t p_string_length,
                          const unichar_t *p_needle, uindex_t p_needle_length,
                          MCUnicodeCompareOption p_option, uindex_t &r_index)
{
    // This is a bit more complicated than a plain comparison and requires the
    // construction of UnicodeString objects.
    UErrorCode t_error = U_ZERO_ERROR;
    icu::UnicodeString t_string(p_string, p_string_length);
    icu::UnicodeString t_needle(p_needle, p_needle_length);
    
    // Normalise, if required
    if (p_option == kMCUnicodeCompareOptionCaseless || p_option == kMCUnicodeCompareOptionNormalised)
    {
        // Construct the normaliser
        const icu::Normalizer2 *t_nfc = icu::Normalizer2::getNFCInstance(t_error);
        t_string = t_nfc->normalize(t_string, t_error);
        t_needle = t_nfc->normalize(t_needle, t_error);
    }
    
    // Case-fold, if required
    if (p_option == kMCUnicodeCompareOptionCaseless || p_option == kMCUnicodeCompareOptionFolded)
    {
        __MCUnicodeSimpleCaseFold(t_string);
        __MCUnicodeSimpleCaseFold(t_needle);
    }
    
    // Perform the comparison
    int32_t t_index = t_string.lastIndexOf(t_needle);
    if (t_index < 0)
        return false;
    r_index = t_index;
    return true;
}

bool MCUnicodeFirstIndexOfChar(const unichar_t *p_string, uindex_t p_string_length,
                               codepoint_t p_needle, MCUnicodeCompareOption p_option,
                               uindex_t &r_index)
{
    // Use the simple case, if possible
    if (p_option == kMCUnicodeCompareOptionExact)
    {
        unichar_t *t_found;
        t_found = u_memchr32(p_string, p_needle, p_string_length);
        if (t_found == NULL)
            return false;
        r_index = t_found - p_string;
        return true;
    }
    
    // Not simple; do the full comparison
    unichar_t t_buffer[2];
    uindex_t t_length;
    if (p_needle <= 0xFFFF)
    {
        t_buffer[0] = p_needle;
        t_length = 1;
    }
    else
    {
        p_needle -= 0x10000;
        t_buffer[0] = (p_needle >> 10) + 0xD800;
        t_buffer[1] = (p_needle & 0x3FF) + 0xDC00;
        t_length = 2;
    }
    
    return MCUnicodeFirstIndexOf(p_string, p_string_length, t_buffer, t_length, p_option, r_index);
}

bool MCUnicodeLastIndexOfChar(const unichar_t *p_string, uindex_t p_string_length,
                               codepoint_t p_needle, MCUnicodeCompareOption p_option,
                               uindex_t &r_index)
{
    // Use the simple case, if possible
    if (p_option == kMCUnicodeCompareOptionExact)
    {
        unichar_t *t_found;
        t_found = u_memrchr32(p_string, p_needle, p_string_length);
        if (t_found == NULL)
            return false;
        r_index = t_found - p_string;
        return true;
    }
    
    // Not simple; do the full comparison
    unichar_t t_buffer[2];
    uindex_t t_length;
    if (p_needle <= 0xFFFF)
    {
        t_buffer[0] = p_needle;
        t_length = 1;
    }
    else
    {
        p_needle -= 0x10000;
        t_buffer[0] = (p_needle >> 10) + 0xD800;
        t_buffer[1] = (p_needle & 0x3FF) + 0xDC00;
        t_length = 2;
    }
    
    return MCUnicodeLastIndexOf(p_string, p_string_length, t_buffer, t_length, p_option, r_index);
}

////////////////////////////////////////////////////////////////////////////////

int32_t MCUnicodeCollate(MCLocaleRef p_locale, MCUnicodeCollateOption p_options,
                         const unichar_t *p_first, uindex_t p_first_length,
                         const unichar_t *p_second, uindex_t p_second_length)
{
    // Create a collation object for the given locale
    UErrorCode t_error = U_ZERO_ERROR;
    icu::Collator* t_collator;
    t_collator = icu::Collator::createInstance(MCLocaleGetICULocale(p_locale), t_error);
    
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
    
    // Do the comparison
    UCollationResult t_result;
    t_result = t_collator->compare(p_first, p_first_length, p_second, p_second_length, t_error);
    
    // Dispose of the collator
    delete t_collator;
    
    // The UCollationResult type maps UCOL_{GREATER,EQUAL,LESS} to +1,0,-1
    return int32_t(t_result);
}

bool MCUnicodeCreateSortKey(MCLocaleRef p_locale, MCUnicodeCollateOption p_options,
                            const unichar_t *p_string, uindex_t p_string_length,
                            byte_t *&r_key, uindex_t &r_key_length)
{
    // Create a collation object for the given locale
    UErrorCode t_error = U_ZERO_ERROR;
    icu::Collator* t_collator;
    t_collator = icu::Collator::createInstance(MCLocaleGetICULocale(p_locale), t_error);
    
    // Ensure the collator was created properly
    if (U_FAILURE(t_error))
        return false;
    
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
    
    // If an error occurred when setting attributes, abort
    if (U_FAILURE(t_error))
    {
        delete t_collator;
        return false;
    }
    
    // Find the length of the sort key that will be generated
    uindex_t t_key_length;
    t_key_length = t_collator->getSortKey(p_string, p_string_length, NULL, 0);
    
    // Allocate memory for the sort key
    MCAutoArray<byte_t> t_key;
    if (!t_key.New(t_key_length))
    {
        delete t_collator;
        return false;
    }
    
    // Generate the sort key
    t_collator->getSortKey(p_string, p_string_length, t_key.Ptr(), t_key.Size());
    
    // Clean up and return
    delete t_collator;
    t_key.Take(r_key, r_key_length);
    return true;
}

////////////////////////////////////////////////////////////////////////////////

// SN-2014-04-03 [[ Bug 12075 ]] Buttons that contain Hebrew Text is in wrong order
// Need to define the function to resolve the text direction out of the block
enum bidi_directional_override_status
{
    kMCBidiOverrideNeutral = 0,
    kMCBidiOverrideLTR,
    kMCBidiOverrideRTL,
};

struct bidi_stack_entry
{
    uint8_t level;
    bidi_directional_override_status override;
    bool isolate;
    
    void clear()
    {
        level = 0;
        override = kMCBidiOverrideNeutral;
        isolate = false;
    }
};

struct level_run
{
    uindex_t start, length;
    uindex_t irs;
    level_run *next_run, *prev_run;
};

struct isolating_run_sequence
{
    uint8_t sos, eos;
    level_run *first_run, *last_run;
};

static bool bidiIncrementISRIndex(uint8_t *classes, level_run*& x_run, uindex_t &x_index)
{
    if (x_run == nil)
        return false;
    
    if (++x_index < x_run->start + x_run->length)
    {
        // Ignore BNs
        if (classes[x_index] == kMCUnicodeDirectionBoundaryNeutral)
            return bidiIncrementISRIndex(classes, x_run, x_index);
        return true;
    }
    
    x_run = x_run -> next_run;
    if (x_run == nil)
        return false;
    
    x_index = x_run -> start - 1;
    return bidiIncrementISRIndex(classes, x_run, x_index);
}

static uint8_t bidiPeekNextISRCharClass(uint8_t *classes, uint8_t alternative, level_run* p_run, uindex_t p_index)
{
    if (!bidiIncrementISRIndex(classes, p_run, p_index))
        return alternative;
    return classes[p_index];
}

static bool bidiDecrementISRIndex(uint8_t *classes, level_run*& x_run, uindex_t& x_index)
{
    if (x_run == nil || x_index == 0)
        return false;
    
    if (--x_index <= x_run -> start)
    {
        x_run = x_run -> prev_run;
        if (x_run != nil)
            x_index = x_run -> start + x_run -> length;
    }
    
    if (x_run == nil)
        return false;
    
    // Ignore BNs
    if (classes[x_index] == kMCUnicodeDirectionBoundaryNeutral)
        return bidiDecrementISRIndex(classes, x_run, x_index);
    
    return true;
}

static uint8_t bidiPeekPrevISRCharClass(uint8_t *classes, uint8_t alternative, level_run* p_run, uindex_t p_index)
{
    if (!bidiDecrementISRIndex(classes, p_run, p_index))
        return alternative;
    return classes[p_index];
}

static void bidiApplyRuleW1(isolating_run_sequence& irs, uint8_t *classes)
{
    // ---- RULE W1 -----
    // Non-spacing marks
    level_run *t_run = irs.first_run;
    uindex_t t_index = t_run->start;
    uint8_t t_class;
    do
    {
        t_class = classes[t_index];
        if (t_class == kMCUnicodeDirectionNonSpacingMark)
        {
            uint8_t t_before = bidiPeekPrevISRCharClass(classes, irs.sos, t_run, t_index);
            if (t_before == kMCUnicodeDirectionRightToLeftIsolate
                || t_before == kMCUnicodeDirectionLeftToRightIsolate
                || t_before == kMCUnicodeDirectionFirstStrongIsolate
                || t_before == kMCUnicodeDirectionPopDirectionalIsolate)
            {
                classes[t_index] = kMCUnicodeDirectionOtherNeutral;
            }
            else
            {
                classes[t_index] = t_before;
            }
        }
    }
    while (bidiIncrementISRIndex(classes, t_run, t_index));
}

static void bidiApplyRuleW2(isolating_run_sequence& irs, uint8_t *classes)
{
    // ----- RULE W2 -----
    // Search backwards from European numbers for strong types
    level_run *t_run = irs.first_run;
    uindex_t t_index = t_run->start;
    uint8_t t_class;
    do
    {
        t_class = classes[t_index];
        if (t_class == kMCUnicodeDirectionEuropeanNumber)
        {
            uint8_t t_strong;
            level_run *t_run_b = t_run;
            uindex_t t_index_b = t_index;
            bool t_valid;
            do
            {
                t_valid = bidiDecrementISRIndex(classes, t_run_b, t_index_b);
                if (t_valid)
                    t_strong = classes[t_index_b];
            }
            while (t_valid
                   && t_strong != kMCUnicodeDirectionRightToLeft
                   && t_strong != kMCUnicodeDirectionLeftToRight
                   && t_strong != kMCUnicodeDirectionRightToLeftArabic);
            
            if (!t_valid)
                t_strong = irs.sos;
            
            if (t_strong == kMCUnicodeDirectionRightToLeftArabic)
                classes[t_index] = kMCUnicodeDirectionArabicNumber;
        }
    }
    while (bidiIncrementISRIndex(classes, t_run, t_index));
}

static void bidiApplyRuleW3(isolating_run_sequence& irs, uint8_t *classes)
{
    // ----- RULE W3 -----
    // Change all Arabic Letters to RTL
    level_run *t_run = irs.first_run;
    uindex_t t_index = t_run->start;
    uint8_t t_class;
    do
    {
        t_class = classes[t_index];
        if (t_class == kMCUnicodeDirectionRightToLeftArabic)
            classes[t_index] = kMCUnicodeDirectionRightToLeft;
    }
    while (bidiIncrementISRIndex(classes, t_run, t_index));
}

static void bidiApplyRuleW4(isolating_run_sequence& irs, uint8_t *classes)
{
    // ----- RULE W4 -----
    // EN ES EN -> EN EN EN
    // EN CS EN -> EN EN EN
    // AN CS EN -> AN AN AN
    level_run *t_run = irs.first_run;
    uindex_t t_index = t_run->start;
    uint8_t t_class;
    uint8_t t_prev_class = irs.sos;
    do
    {
        uint8_t t_peek;
        t_class = classes[t_index];
        t_peek = bidiPeekNextISRCharClass(classes, irs.eos, t_run, t_index);
        
        if (t_class == kMCUnicodeDirectionEuropeanNumberSeparator)
        {
            if (t_prev_class == kMCUnicodeDirectionEuropeanNumber
                && t_peek == kMCUnicodeDirectionEuropeanNumber)
            {
                classes[t_index] = kMCUnicodeDirectionEuropeanNumber;
            }
        }
        
        else if (t_class == kMCUnicodeDirectionCommonNumberSeparator)
        {
            if (t_prev_class == kMCUnicodeDirectionEuropeanNumber
                && t_peek == kMCUnicodeDirectionEuropeanNumber)
            {
                classes[t_index] = kMCUnicodeDirectionEuropeanNumber;
            }
            
            else if (t_prev_class == kMCUnicodeDirectionArabicNumber
                     && t_peek == kMCUnicodeDirectionArabicNumber)
            {
                classes[t_index] = kMCUnicodeDirectionArabicNumber;
            }
        }
        
        t_prev_class = t_class;
    }
    while (bidiIncrementISRIndex(classes, t_run, t_index));
}

static void bidiApplyRuleW5(isolating_run_sequence& irs, uint8_t *classes)
{
    // ----- RULE W5 -----
    // Sequences of ET adjacent to EN turn into EN
    level_run *t_run = irs.first_run;
    uindex_t t_index = t_run->start;
    uint8_t t_class;
    bool t_in_en = false;
    do
    {
        t_class = classes[t_index];
        
        // Are we already expanding an EN?
        if (t_in_en)
        {
            if (t_class == kMCUnicodeDirectionEuropeanNumber)
                ;
            else if (t_class == kMCUnicodeDirectionEuropeanNumberTerminator)
                classes[t_index] = kMCUnicodeDirectionEuropeanNumber;
            else
                t_in_en = false;
            
        }
        else if (t_class == kMCUnicodeDirectionEuropeanNumberTerminator)
        {
            // Scan along a run of European number terminators
            uindex_t t_start = t_index;
            level_run *t_start_run = t_run;
            bool t_valid = true;
            while (t_class == kMCUnicodeDirectionEuropeanNumberTerminator
                   && (t_valid = bidiIncrementISRIndex(classes, t_run, t_index)))
                
                
                // Did we find a European number?
                if (t_valid && classes[t_index] == kMCUnicodeDirectionEuropeanNumber)
                {
                    // Set all of the terminators to EN
                    while (t_start != t_index)
                    {
                        classes[t_start] = kMCUnicodeDirectionEuropeanNumber;
                        bidiIncrementISRIndex(classes, t_start_run, t_start);
                    }
                }
        }
    }
    while (bidiIncrementISRIndex(classes, t_run, t_index));
}

static void bidiApplyRuleW6(isolating_run_sequence& irs, uint8_t *classes)
{
    // ----- RULE W6 -----
    // Separators and terminators become neutral
    level_run *t_run = irs.first_run;
    uindex_t t_index = t_run->start;
    uint8_t t_class;
    do
    {
        t_class = classes[t_index];
        switch (t_class)
        {
            case kMCUnicodeDirectionEuropeanNumberTerminator:
            case kMCUnicodeDirectionEuropeanNumberSeparator:
            case kMCUnicodeDirectionCommonNumberSeparator:
                classes[t_index] = kMCUnicodeDirectionOtherNeutral;
                break;
        }
    }
    while (bidiIncrementISRIndex(classes, t_run, t_index));
}

static void bidiApplyRuleW7(isolating_run_sequence& irs, uint8_t *classes)
{
    // ----- RULE W7 -----
    // Search backwards from EN for first strong type and make them L if L found
    level_run *t_run = irs.first_run;
    uindex_t t_index = t_run->start;
    uint8_t t_class;
    do
    {
        t_class = classes[t_index];
        if (t_class == kMCUnicodeDirectionEuropeanNumber)
        {
            // Search backwards for the first strong type
            uint8_t t_strong;
            level_run *t_run_b = t_run;
            uindex_t t_index_b = t_index;
            bool t_valid;
            do
            {
                t_valid = bidiDecrementISRIndex(classes, t_run_b, t_index_b);
                if (t_valid)
                    t_strong = classes[t_index_b];
            }
            while (t_valid
                   && t_strong != kMCUnicodeDirectionRightToLeft
                   && t_strong != kMCUnicodeDirectionLeftToRight);
            
            if (!t_valid)
                t_strong = irs.sos;
            
            if (t_strong == kMCUnicodeDirectionLeftToRight)
                classes[t_index] = kMCUnicodeDirectionLeftToRight;
        }
    }
    while (bidiIncrementISRIndex(classes, t_run, t_index));
}

static bool bidiIsNI(uint8_t p_class)
{
    switch (p_class)
    {
        case kMCUnicodeDirectionBlockSeparator:
        case kMCUnicodeDirectionSegmentSeparator:
        case kMCUnicodeDirectionWhiteSpaceNeutral:
        case kMCUnicodeDirectionOtherNeutral:
        case kMCUnicodeDirectionFirstStrongIsolate:
        case kMCUnicodeDirectionLeftToRightIsolate:
        case kMCUnicodeDirectionRightToLeftIsolate:
        case kMCUnicodeDirectionPopDirectionalIsolate:
            return true;
    }
    
    return false;
}

static bool bidiIsRForNIRun(uint8_t p_class)
{
    switch (p_class)
    {
        case kMCUnicodeDirectionRightToLeft:
        case kMCUnicodeDirectionEuropeanNumber:
        case kMCUnicodeDirectionArabicNumber:
            return true;
    }
    
    return false;
}

static void bidiApplyRuleN0(isolating_run_sequence& irs, uint8_t *classes)
{
    // TODO
}

static void bidiApplyRuleN1(isolating_run_sequence& irs, uint8_t *classes, uint8_t *levels)
{
    // ----- RULE N1 -----
    level_run *t_run = irs.first_run;
    uindex_t t_index = t_run->start;
    uint8_t t_class;
    do
    {
        t_class = classes[t_index];
        if (bidiIsNI(t_class))
        {
            uindex_t t_before_index, t_after_index;
            level_run *t_before_run;
            
            // Scan backwards for a strong direction
            uint8_t t_strong_before;
            uindex_t t_index_temp = t_index;
            level_run *t_run_temp = t_run;
            bool t_valid;
            do
            {
                t_valid = bidiDecrementISRIndex(classes, t_run_temp, t_index_temp);
                if (t_valid)
                    t_strong_before = classes[t_index_temp];
            }
            while (t_valid && bidiIsNI(t_strong_before));
            
            t_before_index = t_index_temp + 1;
            t_before_run = t_run_temp;
            
            if (!t_valid)
            {
                t_strong_before = irs.sos;
                t_before_run = irs.first_run;
            }
            
            // Scan forwards for a strong direction
            uint8_t t_strong_after;
            t_index_temp = t_index;
            t_run_temp = t_run;
            do
            {
                t_valid = bidiIncrementISRIndex(classes, t_run_temp, t_index_temp);
                if (t_valid)
                    t_strong_after = classes[t_index_temp];
            }
            while (t_valid && bidiIsNI(t_strong_after));
            
            t_after_index = t_index_temp;
            
            if (!t_valid)
                t_strong_after = irs.eos;
            
            // Do both have the same direction?
            if (bidiIsRForNIRun(t_strong_before) == bidiIsRForNIRun(t_strong_after))
            {
                uint8_t t_new_class;
                t_new_class = bidiIsRForNIRun(t_strong_after)
                ? kMCUnicodeDirectionRightToLeft
                : kMCUnicodeDirectionLeftToRight;
                
                // This run needs to take on the direction
                while (t_before_index < t_after_index)
                {
                    classes[t_before_index] = t_new_class;
                    bidiIncrementISRIndex(classes, t_before_run, t_before_index);
                }
            }
            
            t_index = t_after_index + 1;
        }
    }
    while (bidiIncrementISRIndex(classes, t_run, t_index));
}

static void bidiApplyRuleN2(isolating_run_sequence& irs, uint8_t *classes, uint8_t *levels)
{
    // ----- RULE N2 -----
    // Remaining NIs take the embedding level
    level_run *t_run = irs.first_run;
    uindex_t t_index = t_run->start;
    uint8_t t_class;
    do
    {
        t_class = classes[t_index];
        if (bidiIsNI(t_class))
        {
            if (levels[t_index] & 1)
                classes[t_index] = kMCUnicodeDirectionRightToLeft;
            else
                classes[t_index] = kMCUnicodeDirectionLeftToRight;
        }
    }
    while (bidiIncrementISRIndex(classes, t_run, t_index));
}

static void bidiApplyRuleI1(isolating_run_sequence& irs, uint8_t *classes, uint8_t *levels)
{
    // ----- RULE I1 -----
    // Characters with an even embedding level
    level_run *t_run = irs.first_run;
    uindex_t t_index = t_run->start;
    uint8_t t_class;
    do
    {
        t_class = classes[t_index];
        if ((levels[t_index] & 1) == 0)
        {
            if (t_class == kMCUnicodeDirectionRightToLeft)
                levels[t_index] += 1;
            else if (t_class == kMCUnicodeDirectionArabicNumber
                     || t_class == kMCUnicodeDirectionEuropeanNumber)
                levels[t_index] += 2;
        }
    }
    while (bidiIncrementISRIndex(classes, t_run, t_index));
}

static void bidiApplyRuleI2(isolating_run_sequence& irs, uint8_t *classes, uint8_t *levels)
{
    // ----- RULE I2 -----
    // Characters with an odd embedding level
    level_run *t_run = irs.first_run;
    uindex_t t_index = t_run->start;
    uint8_t t_class;
    do
    {
        t_class = classes[t_index];
        if ((levels[t_index] & 1))
        {
            if (t_class == kMCUnicodeDirectionLeftToRight
                || t_class == kMCUnicodeDirectionEuropeanNumber
                || t_class == kMCUnicodeDirectionArabicNumber)
                levels[t_index] += 1;
        }
    }
    while (bidiIncrementISRIndex(classes, t_run, t_index));
}

extern bool MCUnicodeCodepointIsHighSurrogate(codepoint_t codepoint);
extern bool MCUnicodeCodepointIsLowSurrogate(codepoint_t codepoint);

uint8_t MCUnicodeGetFirstStrongIsolate(MCStringRef p_string, uindex_t p_offset)
{
    // From TR9:
    //  P1. Split the text into separate paragraphs. A paragraph separator is
    //      kept with the previous paragraph. Within each paragraph, apply all
    //      the other rules of this algorithm. (Already done by this stage)
    //
    //  P2. In each paragraph, find the first character of type L, AL, or R
    //      while skipping over any characters between an isolate initiator and
    //      its matching PDI or, if it has no matching PDI, the end of the
    //      paragraph.
    //
    //  P3. If a character is found in P2 and it is of type AL or R, then set
    //      the paragraph embedding level to one; otherwise, set it to zero
    
    bool t_found = false;
    uindex_t t_depth = 0;
    uint8_t t_level = 0;
    while (!t_found && p_offset < MCStringGetLength(p_string))
    {
        codepoint_t t_char;
        t_char = MCStringGetCharAtIndex(p_string, p_offset);
        
        // Get the surrogate pair, if required
        uindex_t t_increment = 1;
        codepoint_t t_low;
        if (MCUnicodeCodepointIsHighSurrogate(t_char) &&
            MCUnicodeCodepointIsLowSurrogate(t_low = MCStringGetCharAtIndex(p_string, p_offset + 1)))
        {
            t_char = MCUnicodeSurrogatesToCodepoint(t_char, t_low);
            t_increment = 2;
        }
        
        // Get the directional category for this codepoint
        int32_t t_dir;
        t_dir = MCUnicodeGetIntegerProperty(t_char, kMCUnicodePropertyBidiClass);
        
        // Is this an isolate initiator?
        if (t_dir == kMCUnicodeDirectionLeftToRightIsolate
            || t_dir == kMCUnicodeDirectionRightToLeftIsolate)
        {
            t_depth++;
        }
        
        // Is this an isolate terminator?
        if (t_dir == kMCUnicodeDirectionPopDirectionalIsolate && t_depth > 0)
        {
            t_depth--;
        }
        
        // Is this a codepoint with a strong direction?
        if (t_depth == 0 && t_dir == kMCUnicodeDirectionLeftToRight)
        {
            t_level = 0;
            t_found = true;
        }
        else if (t_depth == 0 && (t_dir == kMCUnicodeDirectionRightToLeft
                                  || t_dir == kMCUnicodeDirectionRightToLeftArabic))
        {
            t_level = 1;
            t_found = true;
        }
        
        p_offset += t_increment;
    }
    
    return t_level;
}

void MCUnicodeResolveDirection(MCStringRef p_string, intenum_t p_base_level, uint8_t *&r_levels, uindex_t& r_level_size)
{
    uindex_t t_length;
    t_length = MCStringGetLength(p_string);
    
    // Map every codepoint in the string to its bidi class
    MCAutoArray<uint8_t> t_classes;
    /* UNCHECKED */ t_classes.New(t_length);
    MCUnicodeGetProperty(MCStringGetCharPtr(p_string), t_length, kMCUnicodePropertyBidiClass, kMCUnicodePropertyTypeUint8, t_classes.Ptr());
    
    // Create an array to store the BiDi level of each character
    uint8_t *t_levels;
    MCMemoryAllocate(t_length, t_levels);
    
    // Directional status stack
    MCAutoArray<bidi_stack_entry> t_stack;
    /* UNCHECKED */ t_stack.New(256);
    uindex_t t_depth = 0;
    t_stack[t_depth].clear();
    
    // Counters
    const uindex_t MAX_DEPTH = 125;
    uindex_t t_current_level = p_base_level;
    uindex_t t_overflow_isolates = 0;
    uindex_t t_overflow_embedding = 0;
    uindex_t t_valid_isolates = 0;
    
    // Isolating run sequences
    MCAutoArray<level_run> t_runs;
    MCAutoArray<isolating_run_sequence> t_irs;
    
    // UNICODE BIDIRECTIONAL ALGORITHM BEGIN
    
    // ----- RULE X1 -----
    //  Process each character iteratively, applying rules X2-X8
    for (uindex_t i = 0; i < t_length; i++)
    {
        uint8_t t_class = t_classes[i];
        bool t_formatting = false;
        bool t_to_remove = false;
        
        // ----- RULE X2 -----
        // Handle RLEs
        if (t_class == kMCUnicodeDirectionRightToLeftEmbedding && t_overflow_isolates == 0 && t_overflow_embedding == 0)
        {
            t_formatting = true;
            t_to_remove = true;
            if (t_depth < MAX_DEPTH)
            {
                // Level is the next odd level > current level
                t_stack[++t_depth].clear();
                t_stack[t_depth].level = t_current_level = t_current_level + 1 + (t_current_level & 1);
            }
            else if (t_overflow_isolates == 0)
            {
                t_overflow_embedding++;
            }
        }
        
        // ----- RULE X3 -----
        // Handle LREs
        if (t_class == kMCUnicodeDirectionLeftToRightEmbedding)
        {
            t_formatting = true;
            t_to_remove = true;
            if (t_depth < MAX_DEPTH && t_overflow_isolates == 0 && t_overflow_embedding == 0)
            {
                // Level is the next even level > current level
                t_stack[++t_depth].clear();
                t_stack[t_depth].level = t_current_level = t_current_level + 2 - (t_current_level & 1);
            }
            else if (t_overflow_isolates == 0)
            {
                t_overflow_embedding++;
            }
        }
        
        // ----- RULE X4 -----
        // Handle RLOs
        if (t_class == kMCUnicodeDirectionRightToLeftOverride)
        {
            t_formatting = true;
            t_to_remove = true;
            if (t_depth < MAX_DEPTH && t_overflow_isolates == 0 && t_overflow_embedding == 0)
            {
                // Level is the next odd level > current level
                t_stack[++t_depth].clear();
                t_stack[t_depth].level = t_current_level = t_current_level + 1 + (t_current_level & 1);
                t_stack[t_depth].override = kMCBidiOverrideRTL;
            }
            else if (t_overflow_isolates == 0)
            {
                t_overflow_embedding++;
            }
        }
        
        // ----- RULE X5 -----
        // Handle LROs
        if (t_class == kMCUnicodeDirectionLeftToRightOverride)
        {
            t_formatting = true;
            t_to_remove = true;
            if (t_depth < MAX_DEPTH && t_overflow_isolates == 0 && t_overflow_embedding == 0)
            {
                // Level is the next even level > current level
                t_stack[++t_depth].clear();
                t_stack[t_depth].level = t_current_level = t_current_level + 2 - (t_current_level & 1);
                t_stack[t_depth].override = kMCBidiOverrideLTR;
            }
            else if (t_overflow_isolates == 0)
            {
                t_overflow_embedding++;
            }
        }
        
        // ----- RULE X5c -----
        // Handle FSIs
        if (t_class == kMCUnicodeDirectionFirstStrongIsolate)
        {
            // Calculate the first strong isolate and handle as an RLI or LRI
            uint8_t t_fsi_level = MCUnicodeGetFirstStrongIsolate(p_string, i);
            if (t_fsi_level == kMCUnicodeDirectionLeftToRight)
                t_class = kMCUnicodeDirectionLeftToRightIsolate;
            else // t_fsi_level == kMCUnicodeDirectionRightToLeft
                t_class = kMCUnicodeDirectionRightToLeftIsolate;
        }
        
        // ----- RULE X5a -----
        // Handle RLIs
        if (t_class == kMCUnicodeDirectionRightToLeftIsolate)
        {
            t_formatting = true;
            t_levels[i] = t_current_level;
            if (t_depth < MAX_DEPTH && t_overflow_isolates == 0 && t_overflow_embedding == 0)
            {
                // Level is the next odd level > current level
                t_stack[++t_depth].clear();
                t_stack[t_depth].level = t_current_level = t_current_level + 1 + (t_current_level & 1);
                t_stack[t_depth].isolate = true;
                t_valid_isolates++;
            }
            else
            {
                t_overflow_isolates++;
            }
        }
        
        // ----- RULE X5b -----
        // Handle LRIs
        if (t_class == kMCUnicodeDirectionLeftToRightIsolate)
        {
            t_formatting = true;
            t_levels[i] = t_current_level;
            if (t_depth < MAX_DEPTH && t_overflow_isolates == 0 && t_overflow_embedding == 0)
            {
                // Level is the next even level > current level
                t_stack[++t_depth].clear();
                t_stack[t_depth].level = t_current_level = t_current_level + 2 - (t_current_level & 1);
                t_stack[t_depth].isolate = true;
                t_valid_isolates++;
            }
            else
            {
                t_overflow_isolates++;
            }
        }
        
        // ----- RULE X6a -----
        // Handle PDIs
        if (t_class == kMCUnicodeDirectionPopDirectionalIsolate)
        {
            if (t_overflow_isolates > 0)
                t_overflow_isolates--;
            else if (t_valid_isolates != 0)
            {
                t_overflow_embedding = 0;
                while (t_stack[t_depth].isolate == false)
                    t_current_level = t_stack[--t_depth].level;
                t_current_level = t_stack[--t_depth].level;
            }
            
            t_levels[i] = t_current_level;
            t_formatting = true;
        }
        
        // ----- RULE X7 -----
        // Handle PDFs
        if (t_class == kMCUnicodeDirectionPopDirectionalFormat)
        {
            if (t_overflow_isolates > 0)
                ;
            else if (t_overflow_embedding > 0)
                t_overflow_embedding--;
            else if (t_stack[t_depth].isolate == false && t_depth > 0)
                t_current_level = t_stack[--t_depth].level;
            
            t_formatting = true;
            t_to_remove = true;
        }
        
        // ----- RULE X6 -----
        // Handle non-formatting characters
        if (!t_formatting
            && t_class != kMCUnicodeDirectionBlockSeparator
            && t_class != kMCUnicodeDirectionBoundaryNeutral)
        {
            t_levels[i] = t_current_level;
            if (t_stack[t_depth].override == kMCBidiOverrideLTR)
                t_classes[i] = kMCUnicodeDirectionLeftToRight;
            else if (t_stack[t_depth].override == kMCBidiOverrideRTL)
                t_classes[i] = kMCUnicodeDirectionRightToLeft;
        }
        
        // ----- RULE X9 -----
        // Remove embedding/override formatting characters
        if (t_to_remove || t_class == kMCUnicodeDirectionBoundaryNeutral)
        {
            t_classes[i] = kMCUnicodeDirectionBoundaryNeutral;
            t_levels[i] = t_current_level;
        }
    }
    
    // ----- RULE X10 -----
    // Compute the isolating run sequences and apply rules W1-W7, N0-N2 and I1-I2
    
    // X10: compute the set of level runs
    uindex_t t_run_cursor = 0;
    while (t_run_cursor < t_length)
    {
        uindex_t t_run_start = t_run_cursor;
        uint8_t t_this_level = t_levels[t_run_cursor];
        while (t_run_cursor < t_length && t_levels[t_run_cursor] == t_this_level)
            t_run_cursor++;
        
        uindex_t t_run_number = t_runs.Size();
        t_runs.Extend(t_run_number + 1);
        t_runs[t_run_number].start = t_run_start;
        t_runs[t_run_number].length = t_run_cursor - t_run_start;
    }
    
    // X10: compute the set of isolating run sequences
    for (uindex_t i = 0; i < t_runs.Size(); i++)
    {
        // TODO: figure out WTH TR9 is going on about here. Somewhat unclear...
        t_runs[i].irs = i;
        t_runs[i].next_run = nil;
        t_runs[i].prev_run = nil;
        
        uindex_t t_irs_number = t_irs.Size();
        t_irs.Extend(t_irs_number + 1);
        t_irs[t_irs_number].first_run = &t_runs[i];
        t_irs[t_irs_number].last_run = &t_runs[i];
    }
    
    // X10: compute the start-of-sequence and end-of-sequence types
    for (uindex_t i = 0; i < t_irs.Size(); i++)
    {
        // SOS calculation
        uindex_t t_preceding = t_irs[i].first_run->start;
        uint8_t t_sos_level = t_levels[t_preceding];
        uint8_t t_preceding_level = p_base_level;
        while (t_preceding > 0 && t_classes[--t_preceding] == kMCUnicodeDirectionBoundaryNeutral)
            ;
        
        // EOS calculation
        // NOTE: this might be wrong if the last char in the sequence is FSI/LRI/RLI without matching PDI
        uindex_t t_succeeding = t_irs[i].last_run->start + t_irs[i].last_run->length - 1;
        uint8_t t_eos_level = t_levels[t_succeeding];
        uint8_t t_succeeding_level = p_base_level;
        while (t_succeeding < (t_length-1) && t_classes[++t_succeeding] == kMCUnicodeDirectionBoundaryNeutral)
            ;
        
        // Direction is R if the higher level is odd, L otherwise
        t_sos_level = t_sos_level < t_preceding_level ? t_preceding_level : t_sos_level;
        t_eos_level = t_eos_level < t_succeeding_level ? t_succeeding_level : t_eos_level;
        if (t_sos_level & 1)
            t_irs[i].sos = kMCUnicodeDirectionRightToLeft;
        else
            t_irs[i].sos = kMCUnicodeDirectionLeftToRight;
        if (t_eos_level & 1)
            t_irs[i].eos = kMCUnicodeDirectionRightToLeft;
        else
            t_irs[i].eos = kMCUnicodeDirectionLeftToRight;
    }
    
    // X10: for each isolating run sequence...
    for (uindex_t i = 0; i < t_irs.Size(); i++)
    {
        isolating_run_sequence &irs = t_irs[i];
        uint8_t *classes = t_classes.Ptr();
        uint8_t *levels = t_levels;
        bidiApplyRuleW1(irs, classes);
        bidiApplyRuleW2(irs, classes);
        bidiApplyRuleW3(irs, classes);
        bidiApplyRuleW4(irs, classes);
        bidiApplyRuleW5(irs, classes);
        bidiApplyRuleW6(irs, classes);
        bidiApplyRuleW7(irs, classes);
        bidiApplyRuleN0(irs, classes);
        bidiApplyRuleN1(irs, classes, levels);
        bidiApplyRuleN2(irs, classes, levels);
        bidiApplyRuleI1(irs, classes, levels);
        bidiApplyRuleI2(irs, classes, levels);
    }
    
    r_levels = t_levels;
    r_level_size = t_length;
}
