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
#include "foundation-text.h"

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

bool MCUnicodeCodepointToSurrogates(codepoint_t t_codepoint, unichar_t* r_surrogates)
{
    if (t_codepoint < 0x10000)
    {
        *r_surrogates = (unichar_t)t_codepoint;
        return false;
    }
    else
    {
        r_surrogates[0] = (((t_codepoint - 0x10000) & 0xFFC00) >> 10) + 0xD800;
        r_surrogates[1] = (t_codepoint & 0x3FF) + 0xDC00;
        return true;
    }
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
    // Create the filters
    MCTextFilter *t_first_filter = MCTextFilterCreate(p_first, p_first_length, kMCStringEncodingUTF16, p_option);
    MCTextFilter *t_second_filter = MCTextFilterCreate(p_second, p_second_length, kMCStringEncodingUTF16, p_option);
    
    while (t_first_filter->HasData() && t_second_filter->HasData())
    {
        // NOTE: this is a simple codepoint compare and produces a binary
        // ordering rather than a lexical ordering.
        int32_t t_diff;
        t_diff = t_first_filter->GetNextCodepoint() - t_second_filter->GetNextCodepoint();
        if (t_diff != 0)
        {
            MCTextFilterRelease(t_first_filter);
            MCTextFilterRelease(t_second_filter);
            return t_diff;
        }
        
        t_first_filter->AdvanceCursor();
        t_second_filter->AdvanceCursor();
    }
    
    bool t_first_longer = t_first_filter->HasData();
    bool t_second_longer = t_second_filter->HasData();
    
    MCTextFilterRelease(t_first_filter);
    MCTextFilterRelease(t_second_filter);
    
    if (t_first_longer)
        return 1;
    else if (t_second_longer)
        return -1;
    else
        return 0;
}

bool MCUnicodeBeginsWith(const unichar_t *p_first, uindex_t p_first_length,
                         const unichar_t *p_second, uindex_t p_second_length,
                         MCUnicodeCompareOption p_option)
{
    // Check for a shared prefix
    uindex_t t_first_match_length, t_second_match_length;
    MCUnicodeSharedPrefix(p_first, p_first_length, p_second, p_second_length, p_option, t_first_match_length, t_second_match_length);
    return t_second_match_length == p_second_length;
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
    uindex_t ignored;
    return MCUnicodeFirstIndexOf(p_string, p_string_length, p_needle, p_needle_length, p_option, ignored);
}

bool MCUnicodeFirstIndexOf(const unichar_t *p_string, uindex_t p_string_length,
                           const unichar_t *p_needle, uindex_t p_needle_length,
                           MCUnicodeCompareOption p_option, uindex_t &r_index)
{
    // Avoid potential problems
    if (p_string_length == 0 || p_needle_length == 0)
        return false;
    
    // Shortcut for native char - for which we are sure to have only one char to compare, and no composing characters
    if (p_needle_length == 1 && *p_needle < 0x10)
        return MCUnicodeFirstIndexOfChar(p_string, p_string_length, *p_needle, p_option, r_index);
    
    // Create filter chains for the strings being searched
    MCTextFilter* t_string_filter = MCTextFilterCreate(p_string, p_string_length, kMCStringEncodingUTF16, p_option);
    MCTextFilter* t_needle_filter = MCTextFilterCreate(p_needle, p_needle_length, kMCStringEncodingUTF16, p_option);

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
            MCUnicodeSharedPrefix(p_string + t_offset, p_string_length - t_offset, p_needle, p_needle_length, p_option, t_string_matched_len, t_needle_matched_len);
            if (t_needle_matched_len == p_needle_length)
            {
                r_index = t_offset;
                MCTextFilterRelease(t_string_filter);
                MCTextFilterRelease(t_needle_filter);
                return true;
            }
        }
        
        t_string_filter->AdvanceCursor();
    }
    
    // No match was found
    MCTextFilterRelease(t_string_filter);
    MCTextFilterRelease(t_needle_filter);
    return false;
}

bool MCUnicodeLastIndexOf(const unichar_t *p_string, uindex_t p_string_length,
                          const unichar_t *p_needle, uindex_t p_needle_length,
                          MCUnicodeCompareOption p_option, uindex_t &r_index)
{
    // Avoid potential problems
    if (p_string_length == 0 || p_needle_length == 0)
        return false;
    
    // Shortcut
    if (p_needle_length == 1)
        return MCUnicodeFirstIndexOfChar(p_string, p_string_length, *p_needle, p_option, r_index);
    
    // Create filter chains for the strings being searched
    MCTextFilter* t_string_filter = MCTextFilterCreate(p_string, p_string_length, kMCStringEncodingUTF16, p_option);
    MCTextFilter* t_needle_filter = MCTextFilterCreate(p_needle, p_needle_length, kMCStringEncodingUTF16, p_option);
    
    // We only want the first codepoint of the needle (for now)
    codepoint_t t_needle_start = t_needle_filter->GetNextCodepoint();
    
    // Search for the beginning of the needle
    bool t_found = false;
    while (t_string_filter->HasData())
    {
        codepoint_t t_cp = t_string_filter->GetNextCodepoint();
        if (t_cp == t_needle_start)
        {
            // Do a fresh string comparison at this point
            t_string_filter->MarkText();
            uindex_t t_offset = t_string_filter->GetMarkedLength() - 1;
            uindex_t t_string_matched_len, t_needle_matched_len;
            MCUnicodeSharedPrefix(p_string + t_offset, p_string_length - t_offset, p_needle, p_needle_length, p_option, t_string_matched_len, t_needle_matched_len);
            if (t_needle_matched_len == p_needle_length)
            {
                r_index = t_offset;
                t_found = true;
            }
        }
        
        t_string_filter->AdvanceCursor();
    }
    
    // Return whether a match was found
    MCTextFilterRelease(t_string_filter);
    MCTextFilterRelease(t_needle_filter);
    return t_found;
}

bool MCUnicodeFirstIndexOfChar(const unichar_t *p_string, uindex_t p_string_length,
                               codepoint_t p_needle, MCUnicodeCompareOption p_option,
                               uindex_t &r_index)
{
    // Create filter chain for the string being searched
    MCTextFilter* t_string_filter = MCTextFilterCreate(p_string, p_string_length, kMCStringEncodingUTF16, p_option);
    
    // Loop until we find the character
    while (t_string_filter->HasData())
    {
        codepoint_t t_cp = t_string_filter->GetNextCodepoint();
        if (t_cp == p_needle)
        {
            t_string_filter->MarkText();
            r_index = t_string_filter->GetMarkedLength() - 1;
            MCTextFilterRelease(t_string_filter);
            return true;
        }
        
        t_string_filter->AdvanceCursor();
    }
    
    // Could not find the character
    MCTextFilterRelease(t_string_filter);
    return false;
}

bool MCUnicodeLastIndexOfChar(const unichar_t *p_string, uindex_t p_string_length,
                               codepoint_t p_needle, MCUnicodeCompareOption p_option,
                               uindex_t &r_index)
{
    // Create filter chain for the string being searched
    MCTextFilter* t_string_filter = MCTextFilterCreate(p_string, p_string_length, kMCStringEncodingUTF16, p_option);
    
    // Loop until we reach the end of the string
    bool t_found = false;
    while (t_string_filter->HasData())
    {
        codepoint_t t_cp = t_string_filter->GetNextCodepoint();
        if (t_cp == p_needle)
        {
            t_string_filter->MarkText();
            r_index = t_string_filter->GetMarkedLength() - 1;
            t_found = true;
        }
        
        t_string_filter->AdvanceCursor();
    }
    
    // Return whether the character was found
    MCTextFilterRelease(t_string_filter);
    return t_found;
}

void MCUnicodeSharedPrefix(const unichar_t *p_string, uindex_t p_string_length, const unichar_t *p_prefix, uindex_t p_prefix_length, MCUnicodeCompareOption p_option, uindex_t &r_len_in_string, uindex_t &r_len_in_prefix)
{
    // Avoid degenerate cases
    if (p_string_length == 0 || p_prefix_length == 0)
    {
        r_len_in_string = r_len_in_prefix = 0;
        return;
    }
    
    // Set up the filter chains for the strings
    MCTextFilter *t_string_filter = MCTextFilterCreate(p_string, p_string_length, kMCStringEncodingUTF16, p_option);
    MCTextFilter *t_prefix_filter = MCTextFilterCreate(p_prefix, p_prefix_length, kMCStringEncodingUTF16, p_option);
    
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
    
    MCTextFilterRelease(t_string_filter);
    MCTextFilterRelease(t_prefix_filter);
}

bool MCUnicodeFind(const unichar_t *p_string, uindex_t p_string_length, const unichar_t *p_needle, uindex_t p_needle_length, MCUnicodeCompareOption p_option, MCRange &r_matched_range)
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
        MCUnicodeSharedPrefix(p_string + t_offset, p_string_length - t_offset, p_needle, p_needle_length, p_option, t_matched_length_string, t_matched_length_needle);
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
    MCTextFilter *t_filter = MCTextFilterCreate(p_string, p_string_length, kMCStringEncodingUTF16, p_option);
    
    // Fowler-Noll-Vo 1a hash function
    if (sizeof(hash_t) == sizeof(uint64_t))
    {
        // 64-bit variant
        const uint64_t kPrime = 1099511628211ULL;
        const uint64_t kOffset = 14695981039346656037ULL;
        uint64_t t_hash = kOffset;
        
        while (t_filter->HasData())
        {
            unichar_t t_char;
            t_char = t_filter->GetNextCodepoint();
            t_filter->AdvanceCursor();
            
            // Hash the first byte of the codeunit
            t_hash ^= t_char & 0xFF;
            t_hash *= kPrime;
            
            // Hash the second byte of the codeunit
            t_hash ^= t_char >> 8;
            t_hash *= kPrime;
        }
        
        MCTextFilterRelease(t_filter);
        return t_hash;
    }
    else
    {
        // 32-bit variant
        const uint32_t kPrime = 16777619UL;
        const uint32_t kOffset = 2166136261UL;
        uint32_t t_hash = kOffset;
        
        while (t_filter->HasData())
        {
            unichar_t t_char;
            t_char = t_filter->GetNextCodepoint();
            t_filter->AdvanceCursor();
            
            // Hash the first byte of the codeunit
            t_hash ^= t_char & 0xFF;
            t_hash *= kPrime;
            
            // Hash the second byte of the codeunit
            t_hash ^= t_char >> 8;
            t_hash *= kPrime;
        }
        
        MCTextFilterRelease(t_filter);
        return t_hash;
    }
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
