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

#include "prefix.h"

#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"
#include "mcio.h"

#include "globals.h"
#include "handler.h"
#include "variable.h"
#include "hndlrlst.h"
#include "osspec.h"

#include "scriptpt.h"
#include "util.h"

#include "exec.h"
#include "exec-strings.h"

#include "chunk.h"
#include "date.h"

#include "foundation-chunk.h"
#include "patternmatcher.h"

////////////////////////////////////////////////////////////////////////////////

bool MCStringsSplit(MCStringRef p_string, codepoint_t p_separator, MCStringRef*&r_strings, uindex_t& r_count)
{
	uindex_t t_current = 0;
	uindex_t t_found = 0;
    
	MCAutoStringRefArray t_strings;
    
	uindex_t t_count = 0;
    
	while (MCStringFirstIndexOfChar(p_string, p_separator, t_current, kMCStringOptionCompareExact, t_found))
	{
		if (!t_strings.Extend(t_count + 1))
			return false;
		if (!MCStringCopySubstring(p_string, MCRangeMakeMinMax(t_current, t_found), t_strings[t_count++]))
			return false;
        
		t_current = t_found + 1;
	}
    
	if (!t_strings.Extend(t_count + 1))
		return false;
	if (!MCStringCopySubstring(p_string, MCRangeMakeMinMax(t_current, MCStringGetLength(p_string)), t_strings[t_count++]))
		return false;
    
	t_strings.Take(r_strings, r_count);
	return true;
}

bool MCStringsSplit(MCStringRef p_string, MCStringRef p_separator, MCStringRef*&r_strings, uindex_t& r_count)
{
	uindex_t t_current = 0;
	uindex_t t_found = 0;

	MCAutoStringRefArray t_strings;

	uindex_t t_count = 0;

	while (MCStringFirstIndexOf(p_string, p_separator, t_current, kMCStringOptionCompareExact, t_found))
	{
		if (!t_strings.Extend(t_count + 1))
			return false;
		if (!MCStringCopySubstring(p_string, MCRangeMakeMinMax(t_current, t_found), t_strings[t_count++]))
			return false;

		t_current = t_found + MCStringGetLength(p_separator);
	}

	if (!t_strings.Extend(t_count + 1))
		return false;
	if (!MCStringCopySubstring(p_string, MCRangeMakeMinMax(t_current, MCStringGetLength(p_string)), t_strings[t_count++]))
		return false;

	t_strings.Take(r_strings, r_count);
	return true;
}

////////////////////////////////////////////////////////////////////////////////

void MCStringsEvalToLower(MCExecContext& ctxt, MCStringRef p_string, MCStringRef& r_lower)
{
	MCStringRef t_string = nil;
	if (!MCStringMutableCopy(p_string, t_string) ||
		!MCStringLowercase(t_string, kMCSystemLocale) ||
		!MCStringCopyAndRelease(t_string, r_lower))
	{
		MCValueRelease(t_string);
		ctxt.Throw();
	}
}

void MCStringsEvalToUpper(MCExecContext& ctxt, MCStringRef p_string, MCStringRef& r_upper)
{
	MCStringRef t_string = nil;
	if (!MCStringMutableCopy(p_string, t_string) ||
		!MCStringUppercase(t_string, kMCSystemLocale) ||
		!MCStringCopyAndRelease(t_string, r_upper))
	{
		MCValueRelease(t_string);
		ctxt.Throw();
	}
}

////////////////////////////////////////////////////////////////////////////////

void MCStringsEvalNumToChar(MCExecContext &ctxt, uinteger_t p_codepoint, MCValueRef& r_character)
{
    // Choose the correct type based on the Unicode setting of the context
    if (ctxt.GetUseUnicode())
    {
        // For compatibility, ignore the upper 16 bits
        uint16_t t_val;
        t_val = p_codepoint;
        
        // To maintain backwards compatibility, this needs to return a dataref
        MCAutoDataRef t_data;
        /* UNCHECKED */ MCDataCreateWithBytes((byte_t*)&t_val, 2, &t_data);
        r_character = MCValueRetain(*t_data);
    }
    else
        // AL-2014-03-26: [[ Bug 11996 ]] For backwards compatibility, we must
        //  use codepoint % 256 here.
        MCStringsEvalNumToNativeChar(ctxt, p_codepoint % 256, (MCStringRef&)r_character);
}

void MCStringsEvalNumToNativeChar(MCExecContext& ctxt, uinteger_t p_codepoint, MCStringRef& r_character)
{
    // Is the supplied codepoint valid for a native char?
    if (p_codepoint < 256)
    {
        char_t t_char;
        t_char = p_codepoint;
        if (MCStringCreateWithNativeChars(&t_char, 1, r_character))
            return;
    }
    else
    {
        // Invalid character number
        if (MCStringCreateWithNativeChars((char_t*)"?", 1, r_character))
            return;
    }

	ctxt . Throw();
}

void MCStringsEvalNumToUnicodeChar(MCExecContext &ctxt, uinteger_t p_codepoint, MCStringRef& r_character)
{
    // Check that the codepoint is within the encodable range for UTF-16 (values
    // outside this range may be representable in other encodings but are not
    // permissable as Unicode codepoints)
    //
    // Note that this only checks that the codepoint is in the first 16 planes
    // and not that the codepoint is valid in other ways.
    if (p_codepoint > 0x10FFFF)
    {
        // Invalid codepoint. Substitute the Unicode replacement character
        p_codepoint = 0xFFFD;
    }
    
    // Does this need encoding as a surrogate pair?
    if (p_codepoint > 0xFFFF)
    {
        // Encode into the leading and trailing surrogates
        unichar_t t_lead, t_trail;
        p_codepoint -= 0x10000;
        t_lead = (p_codepoint >> 10) + 0xD800;
        t_trail = (p_codepoint & 0x3FF) + 0xDC00;
        
        // Turn this into a string
        unichar_t t_string[2] = { t_lead, t_trail };
        if (MCStringCreateWithChars(t_string, 2, r_character))
            return;
    }
    else
    {
        // No special encoding is required
        unichar_t t_char = p_codepoint;
        if (MCStringCreateWithChars(&t_char, 1, r_character))
            return;
    }
    
    // Something went wrong
    ctxt . Throw();
}

void MCStringsEvalCharToNum(MCExecContext& ctxt, MCValueRef p_character, MCValueRef& r_codepoint)
{
	// This function has to be backwards compatible and do the broken stuff...
    MCAutoDataRef t_data;
    ctxt.ConvertToData(p_character, &t_data);
    // In case of empty string as input, the result must be empty
    if (MCDataIsEmpty(*t_data))
    {
        r_codepoint = MCValueRetain(kMCEmptyString);
        return;
    }
    else if (ctxt.GetUseUnicode())
    {
        if (MCDataGetLength(*t_data) >= 2)
        {
            const uint16_t *t_val;
            t_val = (const uint16_t*)MCDataGetBytePtr(*t_data);
            /* UNCHECKED */ MCNumberCreateWithUnsignedInteger(*t_val, (MCNumberRef&)r_codepoint);
            return;
        }
    }
    else
    {
        /* UNCHECKED */ MCNumberCreateWithUnsignedInteger(MCDataGetByteAtIndex(*t_data, 0), (MCNumberRef&)r_codepoint);
        return;
    }
    
    /* UNCHECKED */ MCNumberCreateWithUnsignedInteger(~0, (MCNumberRef&)r_codepoint);
}

void MCStringsEvalNativeCharToNum(MCExecContext& ctxt, MCStringRef p_character, uinteger_t& r_codepoint)
{
    // An empty string must return 0
    if (MCStringIsEmpty(p_character))
    {
        r_codepoint = 0;
        return;
    }
    // Otherwise, only accept strings containing a single character
    if (MCStringGetLength(p_character) == 1)
    {
        r_codepoint = MCStringGetNativeCharAtIndex(p_character, 0);
        return;
    }
    
    ctxt.Throw();
}

void MCStringsEvalUnicodeCharToNum(MCExecContext& ctxt, MCStringRef p_character, uinteger_t& r_codepoint)
{
    // If the string has two characters, it must consist of surrogates
    if (MCStringGetLength(p_character) == 2)
    {
        // Check for a valid surrogate pair
        unichar_t t_lead, t_trail;
        t_lead = MCStringGetCharAtIndex(p_character, 0);
        t_trail = MCStringGetCharAtIndex(p_character, 1);
        if (0xD800 <= t_lead && t_lead <= 0xDBFF && 0xDC00 <= t_trail && t_trail <= 0xDFFF)
        {
            // Valid surrogate pair
            r_codepoint = 0x10000 + ((t_lead - 0xD800) << 10) + (t_trail - 0xDC00);
            return;
        }
    }
    else if (MCStringGetLength(p_character) == 1)
    {
        // Just take the value in the string
        r_codepoint = MCStringGetCodepointAtIndex(p_character, 0);
        return;
    }
    else if (MCStringIsEmpty(p_character))
    {
        // Empty string must return 0
        r_codepoint = 0;
        return;
    }
    
    ctxt.Throw();
}

// AL-2014-10-21: [[ Bug 13740 ]] numToByte should return a DataRef
void MCStringsEvalNumToByte(MCExecContext& ctxt, integer_t p_byte, MCDataRef& r_byte)
{
	byte_t t_byte_as_char;
	t_byte_as_char = (byte_t)p_byte;
	if (MCDataCreateWithBytes(&t_byte_as_char, 1, r_byte))
		return;

	ctxt . Throw();
}

void MCStringsEvalByteToNum(MCExecContext& ctxt, MCStringRef p_byte, integer_t& r_codepoint)
{
    if (MCStringIsEmpty(p_byte))
    {
        r_codepoint = 0;
        return;
    }
    else if (MCStringGetLength(p_byte) == 1)
	{
		r_codepoint = MCStringGetNativeCharAtIndex(p_byte, 0);
		return;
	}

	ctxt . LegacyThrow(EE_BYTETONUM_BADSOURCE);
}

////////////////////////////////////////////////////////////////////////////////

struct MCStringEncodingName
{
    const char *m_name;
    MCStringEncoding m_encoding;
};

// These names must match the formatting of character encoding names as output
// by the algorithm described in section 1.4 of the Unicode Consortium TR22.
static MCStringEncodingName MCStringEncodingNames[] =
{
    {"ascii", kMCStringEncodingASCII},
    {"iso88591", kMCStringEncodingISO8859_1},
    {"macroman", kMCStringEncodingMacRoman},
    {"native", kMCStringEncodingNative},
    {"utf16", kMCStringEncodingUTF16},
    {"utf16be", kMCStringEncodingUTF16BE},
    {"utf16le", kMCStringEncodingUTF16LE},
    {"utf32", kMCStringEncodingUTF32},
    {"utf32be", kMCStringEncodingUTF32BE},
    {"utf32le", kMCStringEncodingUTF32LE},
    {"utf8", kMCStringEncodingUTF8},
    {"cp1252", kMCStringEncodingWindows1252},
    
    {NULL, 0},
};

bool MCStringsEvalTextEncoding(MCStringRef p_encoding, MCStringEncoding &r_encoding)
{
    // First, map the incoming charset name according to the rules in Unicode TR22:
    //  Delete all characters except a-zA-Z0-9
    //  Map A-Z to a-z
    //  From left-to-right, delete each 0 not preceded by a digit
    MCAutoArray<char_t> t_cleaned;
    if (!t_cleaned.New(MCStringGetLength(p_encoding) + 1))   // Cannot exceed incoming length
        return false;
    
    uindex_t t_cleaned_len;
    t_cleaned_len = 0;
    
    bool t_in_number;
    t_in_number = false;
    for (uindex_t i = 0; i < MCStringGetLength(p_encoding); i++)
    {
        unichar_t t_char;
        t_char = MCStringGetCharAtIndex(p_encoding, i);
        
        // All important characters are in the ASCII range (filtering here
        // avoids problems with the subsequent tolower call and non-ASCII chars)
        if (t_char > 128)
        {
            t_in_number = false;
            continue;
        }

        t_char = tolower(t_char);
        
        if (t_char >= 'a' && t_char <= 'z')
        {
            t_in_number = false;
            t_cleaned[t_cleaned_len++] = t_char;
        }
        else if (t_char == '0' && t_in_number)
        {
            t_cleaned[t_cleaned_len++] = t_char;
        }
        else if (t_char >= '1' && t_char <= '9')
        {
            t_in_number = true;
            t_cleaned[t_cleaned_len++] = t_char;
        }
    }
    
    // Terminate the cleaned name
    t_cleaned[t_cleaned_len++] = '\0';
    
    // Search the encoding table
    uindex_t t_index;
    t_index = 0;
    while (MCStringEncodingNames[t_index].m_name != NULL)
    {
        if (strncmp(MCStringEncodingNames[t_index].m_name, (const char*)&t_cleaned[0], t_cleaned_len) == 0)
        {
            r_encoding = MCStringEncodingNames[t_index].m_encoding;
            return true;
        }
        
        t_index++;
    }
    
    // Encoding could not be recognised
    return false;
}

void MCStringsEvalTextDecode(MCExecContext& ctxt, MCStringRef p_encoding, MCDataRef p_data, MCStringRef &r_string)
{
    MCStringEncoding t_encoding;
    if (!MCStringsEvalTextEncoding(p_encoding, t_encoding))
    {
        ctxt.LegacyThrow(EE_TEXTDECODE_BADENCODING);
        return;
    }
    
    MCAutoStringRef t_string;
    if (!MCStringDecode(p_data, t_encoding, false, r_string))
    {
        ctxt.LegacyThrow(EE_TEXTDECODE_FAILED);
        return;
    }
}

void MCStringsEvalTextEncode(MCExecContext& ctxt, MCStringRef p_encoding, MCStringRef p_string, MCDataRef &r_data)
{
    MCStringEncoding t_encoding;
    if (!MCStringsEvalTextEncoding(p_encoding, t_encoding))
    {
        ctxt.LegacyThrow(EE_TEXTENCODE_BADENCODING);
        return;
    }
    
    MCAutoDataRef t_data;
    if (!MCStringEncode(p_string, t_encoding, false, r_data))
    {
        ctxt.LegacyThrow(EE_TEXTENCODE_FAILED);
        return;
    }
}

void MCStringsEvalNormalizeText(MCExecContext& ctxt, MCStringRef p_text, MCStringRef p_form, MCStringRef &r_string)
{
    bool t_success;
    if (MCStringIsEqualToCString(p_form, "NFC", kMCStringOptionCompareCaseless))
    {
        t_success = MCStringNormalizedCopyNFC(p_text, r_string);
    }
    else if (MCStringIsEqualToCString(p_form, "NFD", kMCStringOptionCompareCaseless))
    {
        t_success = MCStringNormalizedCopyNFD(p_text, r_string);
    }
    else if (MCStringIsEqualToCString(p_form, "NFKC", kMCStringOptionCompareCaseless))
    {
        t_success = MCStringNormalizedCopyNFKC(p_text, r_string);
    }
    else if (MCStringIsEqualToCString(p_form, "NFKD", kMCStringOptionCompareCaseless))
    {
        t_success = MCStringNormalizedCopyNFKD(p_text, r_string);
    }
    else
    {
        ctxt.LegacyThrow(EE_NORMALIZETEXT_BADFORM);
        return;
    }
    
    if (!t_success)
        ctxt.Throw();
}

struct MCUnicodePropertyMapping
{
    MCUnicodeProperty m_propid;
    const char *m_name;
};

const MCUnicodePropertyMapping MCUnicodePropertyMap[] =
{
    // Missing properties that are present in TR44:
    //  Name Alias
    //  Script Extensions
    //  Decomposition Mapping
    //  Composition Exclusion
    //  FC NFKC Closure (deprecated)
    //  Expands On NFC (deprecated)
    //  Expands On NFD (deprecated)
    //  Expands On NFKC (deprecated)
    //  Expands On NFKD (deprecated)
    //  NFKC Casefold
    //  Ideographic
    //  Unicode Radical Stroke
    //  Indic Matra Category
    //  Indic Syllabic Category
    
    // Properties we include but TR44 lacks: (capitalised in the table)
    //  Case Sensitive
    //  NFC Inert
    //  NFD Inert
    //  NFKC Inert
    //  NFKD Inert
    //  Segment Starter
    //  POSIX Alnum
    //  POSIX Blank
    //  POSIX Graph
    //  POSIX Print
    //  POSIX Hex Digit
    //  Lead Canonical Combining Class
    //  Trail Canonical Combining Class
    //  General Category Mask
    
    {kMCUnicodePropertyAlphabetic, "Alphabetic"},
    {kMCUnicodePropertyASCIIHex, "ASCII Hex Digit"},
    {kMCUnicodePropertyBidiControl, "Bidi Control"},
    {kMCUnicodePropertyBidiMirrored, "Bidi Mirrored"},
    {kMCUnicodePropertyDash, "Dash"},
    {kMCUnicodePropertyDefaultIgnorable, "Default Ignorable Codepoint"},
    {kMCUnicodePropertyDeprecated, "Deprecated"},
    {kMCUnicodePropertyDiacritic, "Diacritic"},
    {kMCUnicodePropertyExtender, "Extender"},
    {kMCUnicodePropertyFullCompositionExclusion, "Full Composition Exclusion"},
    {kMCUnicodePropertyGraphemeBase, "Grapheme Base"},
    {kMCUnicodePropertyGraphemeExtended, "Grapheme Extend"},
    {kMCUnicodePropertyGraphemeLink, "Grapheme Link"},
    {kMCUnicodePropertyHexDigit, "Hex Digit"},
    {kMCUnicodePropertyHyphen, "Hyphen"},
    {kMCUnicodePropertyIDContinue, "ID Continue"},
    {kMCUnicodePropertyIDStart, "ID Start"},
    {kMCUnicodePropertyIDSBinaryOperator, "IDS Binary Operator"},
    {kMCUnicodePropertyIDSTrinaryOperator, "IDS Trinary Operator"},
    {kMCUnicodePropertyJoinControl, "Join Control"},
    {kMCUnicodePropertyLocalOrderException, "Logical Order Exception"},
    {kMCUnicodePropertyLowercase, "Lowercase"},
    {kMCUnicodePropertyMath, "Math"},
    {kMCUnicodePropertyNoncharacterCodePoint, "Noncharacter Codepoint"},
    {kMCUnicodePropertyQuotationMark, "Quotation Mark"},
    {kMCUnicodePropertyRadical, "Radical"},
    {kMCUnicodePropertySoftDotted, "Soft Dotted"},
    {kMCUnicodePropertyTerminalPunctuation, "Terminal Punctuation"},
    {kMCUnicodePropertyUnifiedIdeograph, "Unified Ideograph"},
    {kMCUnicodePropertyUppercase, "Uppercase"},
    {kMCUnicodePropertyWhiteSpace, "White Space"},
    {kMCUnicodePropertyXIDContinue, "XID Continue"},
    {kMCUnicodePropertyXIDStart, "XID Start"},
    {kMCUnicodePropertyCaseSensitive, "CASE SENSITIVE"},
    {kMCUnicodePropertySTerminal, "STerm"},
    {kMCUnicodePropertyVariationSelector, "Variation Selector"},
    {kMCUnicodePropertyNFDInert, "NFD INERT"},
    {kMCUnicodePropertyNFKDInert, "NFKD INERT"},
    {kMCUnicodePropertyNFCInert, "NFC INERT"},
    {kMCUnicodePropertyNFKCInert, "NFKC INERT"},
    {kMCUnicodePropertySegmentStarter, "SEGMENT STARTER"},
    {kMCUnicodePropertyPatternSyntax, "Pattern Syntax"},
    {kMCUnicodePropertyPatternWhiteSpace, "Pattern White Space"},
    {kMCUnicodePropertyPosixAlnum, "POSIX ALNUM"},
    {kMCUnicodePropertyPosixBlank, "POSIX BLANK"},
    {kMCUnicodePropertyPosixGraph, "POSIX GRAPH"},
    {kMCUnicodePropertyPosixPrint, "POSIX PRINT"},
    {kMCUnicodePropertyPosixHexDigit, "POSIX HEX DIGIT"},
    {kMCUnicodePropertyCased, "Cased"},
    {kMCUnicodePropertyCaseIgnorable, "Case Ignorable"},
    {kMCUnicodePropertyChangesWhenLowercased, "Changes When Lowercased"},
    {kMCUnicodePropertyChangesWhenUppercased, "Changes When Uppercased"},
    {kMCUnicodePropertyChangesWhenTitlecased, "Changes When Titlecased"},
    {kMCUnicodePropertyChangesWhenCaseFolded, "Changes When Casefolded"},
    {kMCUnicodePropertyChangesWhenCaseMapped, "Changes When Casemapped"},
    {kMCUnicodePropertyChangesWhenNFKCCaseFolded, "Changes When NFKC Casefolded"},
    
    {kMCUnicodePropertyBidiClass, "Bidi Class"},
    {kMCUnicodePropertyBlock, "Block"},
    {kMCUnicodePropertyCanonicalCombiningClass, "Canonical Combining Class"},
    {kMCUnicodePropertyDecompositionType, "Decomposition Type"},
    {kMCUnicodePropertyEastAsianWidth, "East Asian Width"},
    {kMCUnicodePropertyGeneralCategory, "General Category"},
    {kMCUnicodePropertyJoiningGroup, "Joining Group"},
    {kMCUnicodePropertyJoiningType, "Joining Type"},
    {kMCUnicodePropertyLineBreak, "Line Break"},
    {kMCUnicodePropertyNumericType, "Numeric Type"},
    {kMCUnicodePropertyScript, "Script"},
    {kMCUnicodePropertyHangulSyllableType, "Hangul Syllable Type"},
    {kMCUnicodePropertyNFDQuickCheck, "NFD Quick Check"},
    {kMCUnicodePropertyNFKDQuickCheck, "NFKD Quick Check"},
    {kMCUnicodePropertyNFCQuickCheck, "NFC Quick Check"},
    {kMCUnicodePropertyNFKCQuickCheck, "NFKC Quick Check"},
    {kMCUnicodePropertyLeadCanonicalCombiningClass, "LEAD CANONICAL COMBINING CLASS"},
    {kMCUnicodePropertyTrailCanonicalCombiningClass, "TRAIL CANONICAL COMBINING CLASS"},
    {kMCUnicodePropertyGraphemeClusterBreak, "Grapheme Cluster Break"},
    {kMCUnicodePropertySentenceBreak, "Sentence Break"},
    {kMCUnicodePropertyWordBreak, "Word Break"},
    {kMCUnicodePropertyBidiPairedBracketType, "Bidi Paired Bracket Type"},
    
    {kMCUnicodePropertyGeneralCategoryMask, "GENERAL CATEGORY MASK"},
    
    {kMCUnicodePropertyNumericValue, "Numeric Value"},
    
    {kMCUnicodePropertyBidiMirroringGlyph, "Bidi Mirroring Glyph"},
    {kMCUnicodePropertySimpleCaseFolding, "Simple Case Folding"},
    {kMCUnicodePropertySimpleLowercaseMapping, "Simple Lowercase Mapping"},
    {kMCUnicodePropertySimpleTitlecaseMapping, "Simple Titlecase Mapping"},
    {kMCUnicodePropertySimpleUppercaseMapping, "Simple Uppercase Mapping"},
    {kMCUnicodePropertyBidiPairedBracket, "Bidi Paired Bracket"},
    
    {kMCUnicodePropertyAge, "Age"},
    {kMCUnicodePropertyCaseFolding, "Case Folding"},
    {kMCUnicodePropertyISOComment, "ISO Comment"},
    {kMCUnicodePropertyLowercaseMapping, "Lowercase Mapping"},
    {kMCUnicodePropertyName, "Name"},
    {kMCUnicodePropertyTitlecaseMapping, "Titlecase Mapping"},
    {kMCUnicodePropertyUnicode1Name, "Unicode 1 Name"},
    {kMCUnicodePropertyUppercaseMapping, "Uppercase Mapping"},
    
    {MCUnicodeProperty(0), NULL},
};

static bool MCStringsMapCodepointPropertyNameToID(MCStringRef p_name, MCUnicodeProperty& r_propid)
{
    // Treat spaces and underscores as identical (for compatibility with Unicode's official names)
    MCAutoStringRef t_name;
    /* UNCHECKED */ MCStringMutableCopy(p_name, &t_name);
    /* UNCHECKED */ MCStringFindAndReplaceChar(*t_name, '_', ' ', kMCStringOptionCompareExact);
    
    const MCUnicodePropertyMapping *t_mapping;
    t_mapping = MCUnicodePropertyMap;
    while (t_mapping->m_name != NULL)
    {
        if (MCStringIsEqualToCString(*t_name, t_mapping->m_name, kMCStringOptionCompareCaseless))
        {
            r_propid = t_mapping->m_propid;
            return true;
        }
        
        t_mapping++;
    }
    
    return false;
}

void MCStringsEvalCodepointProperty(MCExecContext& ctxt, MCStringRef p_codepoint, MCStringRef p_property, MCValueRef &r_value)
{
    // Verify that we have a single codepoint
    if (MCStringGetLength(p_codepoint) != 1
        && (MCStringGetLength(p_codepoint) != 2 || !MCStringIsValidSurrogatePair(p_codepoint, 0)))
    {
        ctxt.LegacyThrow(EE_CODEPOINTPROPERTY_BADCODEPOINT);
        return;
    }
    
    codepoint_t t_codepoint;
    t_codepoint = MCStringGetCodepointAtIndex(p_codepoint, 0);
    
    // Look up the property ID
    MCUnicodeProperty t_propid;
    if (!MCStringsMapCodepointPropertyNameToID(p_property, t_propid))
    {
        ctxt.LegacyThrow(EE_CODEPOINTPROPERTY_BADPROPERTY);
        return;
    }
    
    if (t_propid <= kMCUnicodePropertyLastBinary)
    {
        if (MCUnicodeGetBinaryProperty(t_codepoint, t_propid))
            r_value = MCValueRetain(kMCTrue);
        else
            r_value = MCValueRetain(kMCFalse);
    }
    else if (t_propid <= kMCUnicodePropertyLastBitmask) // Includes integers
    {
        int32_t t_value;
        t_value = MCUnicodeGetIntegerProperty(t_codepoint, t_propid);
        
        // Attempt to map the property value to a string. If it isn't possible,
        // return the property's integer value instead.
        const char *t_prop_string;
        t_prop_string = MCUnicodeGetPropertyValueName(t_propid, t_value);
        if (t_prop_string != nil)
            /* UNCHECKED */ MCStringCreateWithCString(t_prop_string, (MCStringRef&)r_value);
        else
            /* UNCHECKED */ MCNumberCreateWithInteger(t_value, (MCNumberRef&)r_value);
    }
    else if (t_propid <= kMCUnicodePropertyLastFloatingPoint)
    {
        double t_value;
        t_value = MCUnicodeGetFloatProperty(t_codepoint, t_propid);
        /* UNCHECKED */ MCNumberCreateWithReal(t_value, (MCNumberRef&)r_value);
    }
    else if (t_propid <= kMCUnicodePropertyLastCharacter)
    {
        codepoint_t t_value;
        t_value = MCUnicodeGetCharacterProperty(t_codepoint, t_propid);
        /* UNCHECKED */ MCStringCreateWithBytes((const byte_t*)&t_value, 4, kMCStringEncodingUTF32, false, (MCStringRef&)r_value);
    }
    else if (t_propid <= kMCUnicodePropertyLastString)
    {
        const unichar_t *t_value;
        t_value = MCUnicodeGetStringProperty(t_codepoint, t_propid);
        uindex_t t_length = 0;
        while (t_value && t_value[t_length] != 0)
            t_length++;
        /* UNCHECKED */ MCStringCreateWithChars(t_value, t_length, (MCStringRef&)r_value);
    }
    else
    {
        // Shouldn't get here
        MCAssert(false);
        ctxt.LegacyThrow(EE_CODEPOINTPROPERTY_BADPROPERTY);
        return;
    }
}

////////////////////////////////////////////////////////////////////////////////

void MCStringsEvalLength(MCExecContext& ctxt, MCStringRef p_string, integer_t& r_length)
{
	// Ensure that the returned length is in chars
    MCRange t_cp_range, t_cu_range;
    t_cu_range = MCRangeMake(0, MCStringGetLength(p_string));
    /* UNCHECKED */ MCStringUnmapIndices(p_string, kMCCharChunkTypeGrapheme, t_cu_range, t_cp_range);
    
    r_length = t_cp_range . length;
}

////////////////////////////////////////////////////////////////////////////////

#include "regex.h"

index_t MCregexfrontier = 0;

bool MCStringsCompilePattern(MCStringRef p_pattern, regexp*& r_compiled, bool casesensitive)
{
    r_compiled = MCR_compile(p_pattern, casesensitive);
    return r_compiled != nil;    
}

//////////

void MCStringsEvalMatchText(MCExecContext& ctxt, MCStringRef p_string, MCStringRef p_pattern, MCStringRef* r_results, uindex_t p_result_count, bool& r_match)
{
    regexp* t_compiled = nil;

    if (!MCStringsCompilePattern(p_pattern, t_compiled, true /* casesensitive */))
    {
        ctxt.LegacyThrow(EE_MATCH_BADPATTERN);
        return;
    }
    
    bool t_success = true;
    r_match = 0 != MCR_exec(t_compiled, p_string, MCRangeMake(0, MCStringGetLength(p_string)));
    uindex_t t_match_index = 1;
    
    for (uindex_t i = 0; t_success && i < p_result_count; i++)
    {
        if (r_match && t_compiled->matchinfo[t_match_index].rm_so != -1)
        {
            uindex_t t_start, t_length;
            t_start = t_compiled->matchinfo[t_match_index].rm_so;
            t_length = t_compiled->matchinfo[t_match_index].rm_eo - t_start;
            t_success = MCStringCopySubstring(p_string, MCRangeMake(t_start, t_length), r_results[i]);
        }
        else
            r_results[i] = MCValueRetain(kMCEmptyString);
        
        // SN-02-06-2014 [[ Bug 12574 ]] REGEX : matchText result not as expected
        // Once a pattern didn't match, the index was stuck on this non-matching index
        if (++t_match_index >= NSUBEXP)
            t_match_index = 0;
    }
    
	delete t_compiled;
	
    if (t_success)
        return;
    
    for (uindex_t i = 0; i < p_result_count; i++)
        MCValueRelease(r_results[i]);
    
    ctxt.Throw();
}

void MCStringsEvalMatchChunk(MCExecContext& ctxt, MCStringRef p_string, MCStringRef p_pattern, MCStringRef* r_results, uindex_t p_result_count, bool& r_match)
{
    regexp* t_compiled = nil;
    
    if (!MCStringsCompilePattern(p_pattern, t_compiled, true /* casesensitive */))
    {
        ctxt.LegacyThrow(EE_MATCH_BADPATTERN);
        return;
    }

    bool t_success = true;
    r_match = 0 != MCR_exec(t_compiled, p_string, MCRangeMake(0, MCStringGetLength(p_string)));
    uindex_t t_match_index = 1;
    
    for (uindex_t i = 0; t_success && i + 1 < p_result_count; i += 2)
    {
        if (r_match && t_compiled->matchinfo[t_match_index].rm_so != -1)
        {
			MCRange t_cu_range, t_char_range;
			t_cu_range.offset = t_compiled->matchinfo[t_match_index].rm_so;
			t_cu_range.length = t_compiled->matchinfo[t_match_index].rm_eo - t_compiled->matchinfo[t_match_index].rm_so;
			MCStringUnmapIndices(p_string,
								 kMCCharChunkTypeGrapheme,
								 t_cu_range,
								 t_char_range);
            t_success = MCStringFormat(r_results[i], "%d", t_char_range.offset + 1);
            if (t_success)
                t_success = MCStringFormat(r_results[i + 1], "%d", t_char_range.offset + t_char_range.length);
        }
        else
        {
            r_results[i] = MCValueRetain(kMCEmptyString);
            r_results[i + 1] = MCValueRetain(kMCEmptyString);
        }
        // matchChunk did set empty values of positionVarsList with regex "(a)?(B)" applied on "B"
        // if an optional capture group didn't match (which is valid), index wasn't updated.
        if (++t_match_index >= NSUBEXP)
            t_match_index = 0;
    }
    
	delete t_compiled;
	
    if (t_success)
    {
        if ((p_result_count & 1) == 1)
            r_results[p_result_count - 1] = MCValueRetain(kMCEmptyString);
        return;
    }
    
    for (uindex_t i = 0; i < p_result_count; i++)
        MCValueRelease(r_results[i]);
    
    ctxt.Throw();
}

//////////

void MCStringsEvalReplaceText(MCExecContext& ctxt, MCStringRef p_string, MCStringRef p_pattern, MCStringRef p_replacement, MCStringRef& r_result)
{
    regexp* t_compiled = nil;

    if (!MCStringsCompilePattern(p_pattern, t_compiled, true /* casesensitive */))
    {
        ctxt.LegacyThrow(EE_REPLACETEXT_BADPATTERN);
        return;
    }
    
    /* MCR_exec needs to use a unicode string as we use PCRE compiled with 16-bit
     * unit support. We copy the string here so that we aren't re-copying each
     * call to MCR_exec. */
    MCAutoStringRef t_unicode_string;
    if (!MCStringUnicodeCopy(p_string, &t_unicode_string))
    {
		delete t_compiled;
        ctxt.Throw();
        return;
    }

    bool t_success = true;
    
    MCAutoStringRef t_result;
    t_success = MCStringCreateMutable(0, &t_result);
    
    uindex_t t_source_length = MCStringGetLength(p_string);
    uindex_t t_source_offset = 0;
    
    while (t_success && t_source_offset < t_source_length && MCR_exec(t_compiled, *t_unicode_string, MCRangeMakeMinMax(t_source_offset, MCStringGetLength(p_string))))
    {
        uindex_t t_start = t_compiled->matchinfo[0].rm_so;
        uindex_t t_end = t_compiled->matchinfo[0].rm_eo;
        
        if (t_start == t_end)
            break;
        
        // Copy the bit before the found match into the target buffer
        MCAutoStringRef t_pre_match;
        if (t_success)
            t_success = MCStringCopySubstring(p_string, MCRangeMake(t_source_offset, t_start), &t_pre_match) &&
                MCStringAppend(*t_result, *t_pre_match);
        
        // Copy the replacement string into the target buffer where the found match was
        if (t_success)
            t_success = MCStringAppend(*t_result, p_replacement);
        
        // Begin searching again after the end of the match
        t_source_offset += t_end;
        
        if (MCStringGetCharAtIndex(p_pattern, 0) == '^')
            break;
    }
    
	delete t_compiled;
	
    MCAutoStringRef t_post_match;
    if (t_success)
        t_success = MCStringCopySubstring(p_string, MCRangeMakeMinMax(t_source_offset, t_source_length), &t_post_match) &&
            MCStringAppend(*t_result, *t_post_match);
    
    if (t_success)
        t_success = MCStringCopy(*t_result, r_result);
    
    if (t_success)
        return;
    
    ctxt.Throw();
}

////////////////////////////////////////////////////////////////////////////////

#define INT_VALUE 0
#define PTR_VALUE 1
#define DOUBLE_VALUE 2

bool PopParam(MCValueRef*& x_params, uindex_t& x_count, MCValueRef& r_value)
{
	if (x_count == 0)
		return false;
	r_value = *x_params++;
	x_count--;

	return true;
}

void MCStringsEvalFormat(MCExecContext& ctxt, MCStringRef p_format, MCValueRef* p_params, uindex_t p_param_count, MCStringRef& r_result)
{
	MCAutoStringRef t_result;
	bool t_success = true;

    MCAutoStringRefAsWString t_wide_string;
    t_wide_string . Lock(p_format);

    const unichar_t *format = *t_wide_string;

	t_success = MCStringCreateMutable(0, &t_result);

	while (t_success && *format != '\0')
	{
		MCValueRef t_value = nil;

        // All the unicode chars must be ignored from the format, and just copied
        const unichar_t* t_start;
        t_start = format;
        while (*format && *format >= 256)
            format++;

        // Case in which a copy of unicode characters is needed
        if (format != t_start)
        {
            t_success = MCStringAppendChars(*t_result, t_start, format - t_start);
            continue;
        }

		if (*format == '\\')
		{
            char_t t_result_char = 0;
			switch (*++format)
			{
			case 'a':
				t_result_char = '\a';
				break;
			case 'b':
				t_result_char = '\b';
				break;
			case 'f':
				t_result_char = '\f';
				break;
			case 'n':
				t_result_char = '\n';
				break;
			case 'r':
				t_result_char = '\r';
				break;
			case 't':
				t_result_char = '\t';
				break;
			case 'v':
				t_result_char = '\v';
				break;
			case '\\':
				t_result_char = '\\';
				break;
			case '?':
				t_result_char = '?';
				break;
			case '\'':
				t_result_char = '\'';
				break;
			case '"':
				t_result_char = '"';
				break;
			case 'x':
                if (isxdigit(*++format))
                {
                    // The next 2 chars are supposed to be hexadecimal digits
                    // MCNumberParseUnicodeChars expects a '0xFF' format to recognize a
                    // hexadecimal input
                    unichar_t t_hexa_num[5];
                    t_hexa_num[0] = '0';
                    t_hexa_num[1] = 'x';
                    // AL-2014-04-02: [[ Bug 12065 ]] Typo in target array index (t_hexa_num[1]) caused crash.
                    memcpy(&t_hexa_num[2], format, 2*sizeof(unichar_t));
                    t_hexa_num[4] = '\0';

                    MCAutoNumberRef t_number;
                    /* UNCHECKED */ MCNumberParseUnicodeChars(t_hexa_num, 4, &t_number);

                    t_result_char = MCNumberFetchAsUnsignedInteger(*t_number);
                    format++;
				}
				break;
			default:
                if (isdigit(*format))
				{
                    const unichar_t *sptr = format;
                    while (isdigit(*format) && format - sptr < 3)
						t_result_char = (t_result_char << 3) + (*format++ - '0');
					format--;
				}
				break;
			}
            t_success = MCStringAppendNativeChar(*t_result, t_result_char);
			format++;
			continue;
		}
		else if (*format != '%')
		{
            const unichar_t *startptr = format;
			while (*format && *format != '%' && *format != '\\')
				format++;
            t_success = MCStringAppendChars(*t_result, startptr, format - startptr);
			continue;
		}
		else if (format[1] == '%')
		{
            t_success = MCStringAppendChars(*t_result, format, 1);
			format += 2;
			continue;
		}
		else
        {
            // We need to create a substring containing all the following non-unicode chars
            // which much be parsed as a format.
            const unichar_t* t_unicode_start;
            t_unicode_start = format;

            while (*format && *format < 256)
                format++;

            MCAutoStringRef t_substring;
            MCAutoStringRefAsNativeChars t_auto_native;
            const char_t* t_native_format;
            uindex_t t_cformat_length;
            const char_t* t_cstart;

            if (!MCStringCreateWithChars(t_unicode_start, format - t_start, &t_substring) ||
            			!t_auto_native . Lock(*t_substring, t_native_format, t_cformat_length))
						{
							ctxt.LegacyThrow(EE_NO_MEMORY);
							return;
						}
            t_cstart = t_native_format;

            char newFormat[40];
            char *dptr = newFormat;

			integer_t width = 0;
			uinteger_t precision = 0;

			bool useShort = false;
			uinteger_t whichValue = PTR_VALUE;
			const char *end;
            const char_t *prefix_zero;
            prefix_zero = nil;
            
            bool t_zero_pad;
            t_zero_pad = false;
            
            *dptr++ = *t_native_format++;
            while (*t_native_format == '-' || *t_native_format == '#' || *t_native_format == '0'
                || *t_native_format == ' ' || *t_native_format == '+')
            {
                // AL-2014-11-19: [[ Bug 14059 ]] Record position of last zero.
                if (*t_native_format == '0')
                    prefix_zero = t_native_format;
                *dptr++ = *t_native_format++;
            }
            if (isdigit((uint1)*t_native_format))
			{
                width = strtol((const char*)t_native_format, (char **)&end, 10);
                
                // AL-2014-11-19: [[ Bug 14059 ]] If last zero was immediately before the first non-zero digit then pad with zeroes.
                if (prefix_zero == t_native_format - 1)
                    t_zero_pad = true;
                
                t_native_format = (char_t*)end;
			}
            else if (*t_native_format == '*')
			{
				if (!PopParam(p_params, p_param_count, t_value) || !ctxt.ConvertToInteger(t_value, width))
				{
					ctxt.LegacyThrow(EE_FORMAT_BADSOURCE, t_value);
					return;
				}
				t_value = nil;
                t_native_format++;
			}
			if (width != 0)
			{
				sprintf(dptr, "%d", width);
				while (*++dptr)
					;
			}
            if (*t_native_format == '.')
                *dptr++ = *t_native_format++;
            if (isdigit((uint1)*t_native_format))
			{
                precision = strtoul((const char*)t_native_format, (char **)&end, 10);
                t_native_format = (char_t*)end;
			}
            else if (*t_native_format == '*')
			{
				if (!PopParam(p_params, p_param_count, t_value) || !ctxt.ConvertToUnsignedInteger(t_value, precision))
				{
					ctxt.LegacyThrow(EE_FORMAT_BADSOURCE, t_value);
					return;
				}
				t_value = nil;
                t_native_format++;
			}
			if (precision != 0)
			{
				sprintf(dptr, "%d", precision);
				while (*++dptr)
					;
			}
            if (*t_native_format == 'l')
                t_native_format++;
			else
                if (*t_native_format == 'h')
				{
					useShort = true;
                    *dptr++ = *t_native_format++;
				}
            *dptr++ = *t_native_format;
			*dptr = 0;
            switch (*t_native_format)
			{
			case 'i':
				dptr[-1] = 'd';
			case 'd':
			case 'o':
			case 'u':
			case 'x':
			case 'X':
			case 'c':
				whichValue = INT_VALUE;
				break;
			case 's':
                // If there is a zero width, then remove it.
                if (prefix_zero != nil)
                {
                    dptr -= 1;
                    dptr[0] = '\0';
                }
                // AL-2014-10-30: [[ Bug 13876 ]] Use internal MCStringRef format specifier (%@) when %s is used
                //  to preserve non-native chars in string.
                dptr[-1] = '@';
				break;
			case 'e':
			case 'E':
			case 'f':
			case 'g':
			case 'G':
				whichValue = DOUBLE_VALUE;
				break;
			default:
				ctxt.LegacyThrow(EE_FORMAT_BADSOURCE);
				return;
			}
            t_native_format++;
			switch (whichValue)
			{
			case DOUBLE_VALUE:
				real64_t t_real;
				if (!PopParam(p_params, p_param_count, t_value) || !ctxt.ConvertToReal(t_value, t_real))
				{
					ctxt.LegacyThrow(EE_FORMAT_BADSOURCE, t_value);
					return;
				}
				t_success = MCStringAppendFormat(*t_result, newFormat, t_real);
				break;
			case INT_VALUE:
				integer_t t_integer;
				if (!PopParam(p_params, p_param_count, t_value) || !ctxt.ConvertToInteger(t_value, t_integer))
				{
					ctxt.LegacyThrow(EE_FORMAT_BADSOURCE, t_value);
					return;
				}
				if (useShort)
					t_success = MCStringAppendFormat(*t_result, newFormat, (short)t_integer);
				else
					t_success = MCStringAppendFormat(*t_result, newFormat, t_integer);
				break;
			default:
				MCAutoStringRef t_string;
				if (!PopParam(p_params, p_param_count, t_value) || !ctxt.ConvertToString(t_value, &t_string))
				{
					ctxt.LegacyThrow(EE_FORMAT_BADSOURCE, t_value);
					return;
				}
                    
                if (t_success)
                {
                    // AL-2014-10-30: If there is a width parameter, we need to jump through some hoops to make
                    //  sure the correct result is returned for non-native strings - namely pad the string with enough
                    //  spaces so that the result has the specified number of graphemes.
                    if (width != 0)
                    {
                        uindex_t t_length = MCStringGetLength(*t_string);
                        MCRange t_range;
                        t_range = MCRangeMake(0, t_length);
                        // Find the grapheme length of 
                        MCStringUnmapGraphemeIndices(*t_string, t_range, t_range);
                        
                        // If the width sub-specifier is greater than the grapheme length of the string, then pad appropriately
                        if (width > (integer_t) t_range . length)
                        {
                            // AL-2014-11-19: [[ Bug 14059 ]] Pad with zeroes if the appropriate specifier flag was used
                            if (t_zero_pad)
                                t_success = MCStringAppendFormat(*t_result, "%0*s%@", width - t_range . length, "", *t_string);
                            else
                                t_success = MCStringAppendFormat(*t_result, "%*s%@", width - t_range . length, "", *t_string);
                        }
                        else
                            t_success = MCStringAppendFormat(*t_result, "%@", *t_string);
                    }
                    else
                        // AL-2014-10-30: [[ Bug 13876 ]] Don't convert to cstring for format
                        t_success = MCStringAppendFormat(*t_result, newFormat, *t_string);
                }
				break;
			}

            // Update the position in the unicode format depending
            // on what has been parsed
            format = t_unicode_start + (t_native_format - t_cstart);
		}
	}

	if (t_success && MCStringCopy(*t_result, r_result))
		return;

	ctxt.Throw();
}

////////////////////////////////////////////////////////////////////////////////

bool MCStringsMerge(MCExecContext& ctxt, MCStringRef p_format, MCStringRef& r_string)
{
	if (MCStringGetLength(p_format) == 0)
	{
		r_string = MCValueRetain(kMCEmptyString);
		return true;
	}

	MCAutoStringRef t_merged;
	uindex_t t_start = 0;
	uindex_t t_length;
	t_length = MCStringGetLength(p_format);

	if (!MCStringMutableCopy(p_format, &t_merged))
		return false;

	while (t_start + 1 < t_length)
	{
		uindex_t t_expression_end = 0;

		bool t_match = false;
		bool t_is_expression = false;

		switch(MCStringGetCharAtIndex(*t_merged, t_start))
		{
		case '<':
			if (MCStringGetCharAtIndex(*t_merged, t_start + 1) == '?')
			{
				t_expression_end = t_start + 2;
				while (t_expression_end + 1 < t_length)
				{
					if (MCStringGetCharAtIndex(*t_merged, t_expression_end) == '?' &&
						MCStringGetCharAtIndex(*t_merged, t_expression_end + 1) == '>')
					{
						t_expression_end += 2;
						t_match = true;
						t_is_expression = false;
						break;
					}
					else
						t_expression_end++;
				}
			}
			break;
		case '[':
			if (MCStringGetCharAtIndex(*t_merged, t_start + 1) == '[')
			{
                // AL-2013-10-15 [[ Bug 11274 ]] Merge function should ignore square bracket if part of inner expression
                uint4 t_skip;
                t_skip = 0;
				t_expression_end = t_start + 2;
				while (t_expression_end + 1 < t_length)
				{
                    if (MCStringGetCharAtIndex(*t_merged, t_expression_end) == '[')
                        t_skip++;
					else if (MCStringGetCharAtIndex(*t_merged, t_expression_end) == ']')
                    {
                        if (t_skip > 0)
                            t_skip--;
						else if (MCStringGetCharAtIndex(*t_merged, t_expression_end + 1) == ']')
                        {
                            t_expression_end += 2;
                            t_match = true;
                            t_is_expression = true;
                            break;
                        }
                    }
                    t_expression_end++;
				}
			}
			break;
		default:
			break;
		}
		if (t_match)
		{
			bool t_valid = true;

			MCAutoStringRef t_replacement;
			uindex_t t_expression_length = t_expression_end - t_start;
			if (t_expression_length <= 4)
				t_replacement = kMCEmptyString;
			else
			{
				MCAutoValueRef t_value;
				MCAutoStringRef t_expression;
				
				if (!MCStringCopySubstring(*t_merged, MCRangeMake(t_start + 2, t_expression_length - 4), &t_expression))
					return false;

				MCExecContext t_ctxt(ctxt);

				MCerrorlock++;
				if (t_is_expression)
				{
                    // SN-2015-06-03: [[ Bug 11277 ]] MCHandler::eval refactored
                    ctxt.eval(t_ctxt, *t_expression, &t_value);
				}
				else
                {
                    // SN-2015-06-03: [[ Bug 11277 ]] MCHandler::doscript refactored
                    ctxt.doscript(t_ctxt, *t_expression, 0, 0);

					t_value = MCresult->getvalueref();
                    // SN-2014-08-11: [[ Bug 13139 ]] The result must be emptied after a doscript()
                    ctxt . SetTheResultToEmpty();
				}
				t_valid = !t_ctxt.HasError();
				MCerrorlock--;

				if (t_valid && !ctxt.ForceToString(*t_value, &t_replacement))
					return false;
			}

			if (t_valid)
			{
				if (!MCStringReplace(*t_merged, MCRangeMake(t_start, t_expression_length), *t_replacement))
					return false;

				uindex_t t_replacement_length = MCStringGetLength(*t_replacement);
				t_length = t_length + t_replacement_length - t_expression_length;
				t_start += t_replacement_length;
			}
			else
				t_start += t_expression_length;

		}
		else
			t_start++;
	}

	return MCStringCopy(*t_merged, r_string);
}

void MCStringsEvalMerge(MCExecContext& ctxt, MCStringRef p_format, MCStringRef& r_string)
{
	if (MCStringsMerge(ctxt, p_format, r_string))
		return;

	ctxt.Throw();
}

////////////////////////////////////////////////////////////////////////////////

bool MCStringsConcatenate(MCStringRef p_left, MCStringRef p_right, MCStringRef& r_result)
{
    return MCStringCreateWithStrings(r_result, p_left, p_right);
}

bool MCStringsConcatenateWithChar(MCStringRef p_left, MCStringRef p_right, unichar_t p_char, MCStringRef& r_result)
{
    return MCStringCreateWithStringsAndSeparator(r_result, p_char, p_left, p_right);
}

void MCStringsEvalConcatenate(MCExecContext& ctxt, MCStringRef p_left, MCStringRef p_right, MCStringRef& r_result)
{
	if (MCStringsConcatenate(p_left, p_right, r_result))
		return;
    
    // SN-2015-01-13: [[ Bug 14354 ]] To reproduce the previous behaviour, we want to abort
    // any loop if in case of a memory error.
    ctxt . LegacyThrow(EE_NO_MEMORY);
    MCabortscript = True;
}

bool MCDataConcatenate(MCDataRef p_left, MCDataRef p_right, MCDataRef& r_result)
{
    return MCDataCreateWithData(r_result, p_left, p_right);
}

void MCStringsEvalConcatenate(MCExecContext& ctxt, MCDataRef p_left, MCDataRef p_right, MCDataRef& r_result)
{
	if (MCDataConcatenate(p_left, p_right, r_result))
		return;
    
    // SN-2015-01-13: [[ Bug 14354 ]] To reproduce the previous behaviour, we want to abort
    // any loop if in case of a memory error.
    ctxt . LegacyThrow(EE_NO_MEMORY);
    MCabortscript = True;
}

void MCStringsEvalConcatenateWithSpace(MCExecContext& ctxt, MCStringRef p_left, MCStringRef p_right, MCStringRef& r_result)
{
	if (MCStringsConcatenateWithChar(p_left, p_right, ' ', r_result))
		return;
    
    // SN-2015-01-13: [[ Bug 14354 ]] To reproduce the previous behaviour, we want to abort
    // any loop if in case of a memory error.
    ctxt . LegacyThrow(EE_NO_MEMORY);
    MCabortscript = True;
}

void MCStringsEvalConcatenateWithComma(MCExecContext& ctxt, MCStringRef p_left, MCStringRef p_right, MCStringRef& r_result)
{
	if (MCStringsConcatenateWithChar(p_left, p_right, ',', r_result))
        return;
    
    // SN-2015-01-13: [[ Bug 14354 ]] To reproduce the previous behaviour, we want to abort
    // any loop if in case of a memory error.
    ctxt . LegacyThrow(EE_NO_MEMORY);
    MCabortscript = True;
}

////////////////////////////////////////////////////////////////////////////////

static bool MCStringsCheckGraphemeBoundaries(MCStringRef p_string, MCRange p_range)
{
    return MCStringIsGraphemeClusterBoundary(p_string, p_range . offset)
        && MCStringIsGraphemeClusterBoundary(p_string, p_range . offset + p_range . length);
}

void MCStringsEvalContains(MCExecContext& ctxt, MCStringRef p_whole, MCStringRef p_part, bool& r_result)
{
    // AL-2014-10-23: [[ Bug 13770 ]] Strings don't contain the empty string
    if (MCStringIsEmpty(p_part))
    {
        r_result = false;
        return;
    }
    
	MCStringOptions t_compare_option = ctxt.GetStringComparisonType();
    
    bool t_found;
    MCRange t_range;
    t_found = MCStringFind(p_whole, MCRangeMake(0, MCStringGetLength(p_whole)), p_part, t_compare_option, &t_range);
    if (!t_found)
    {
        r_result = false;
        return;
    }
    
    r_result = MCStringsCheckGraphemeBoundaries(p_whole, t_range);
}

void MCStringsEvalDoesNotContain(MCExecContext& ctxt, MCStringRef p_whole, MCStringRef p_part, bool& r_result)
{
    bool t_result;
    MCStringsEvalContains(ctxt, p_whole, p_part, t_result);
    r_result = !t_result;
}

void MCStringsEvalBeginsWith(MCExecContext& ctxt, MCStringRef p_whole, MCStringRef p_part, bool& r_result)
{
	MCStringOptions t_compare_option = ctxt.GetStringComparisonType();
    
    bool t_found;
    uindex_t t_self_length;
    t_found = MCStringBeginsWith(p_whole, p_part, t_compare_option, &t_self_length);
    if (!t_found)
    {
        r_result = false;
        return;
    }
    
    MCRange t_range;
    t_range = MCRangeMake(0, t_self_length);
    
    r_result = MCStringsCheckGraphemeBoundaries(p_whole, t_range);
}

void MCStringsEvalEndsWith(MCExecContext& ctxt, MCStringRef p_whole, MCStringRef p_part, bool& r_result)
{
	MCStringOptions t_compare_option = ctxt.GetStringComparisonType();
    
    bool t_found;
    uindex_t t_self_length;
    t_found = MCStringEndsWith(p_whole, p_part, t_compare_option, &t_self_length);
    if (!t_found)
    {
        r_result = false;
        return;
    }
    
    // MW-2014-10-24: [[ Bug 13787 ]] Make sure we calculate the correct range.
    MCRange t_range;
    t_range = MCRangeMake(MCStringGetLength(p_whole) - t_self_length, t_self_length);
    
    r_result = MCStringsCheckGraphemeBoundaries(p_whole, t_range);
}

////////////////////////////////////////////////////////////////////////////////

bool MCStringsEvalIsAmongTheChunksOf(MCExecContext& ctxt, MCStringRef p_chunk, MCStringRef p_text, Chunk_term p_chunk_type)
{
    MCChunkType t_type;
    t_type = MCChunkTypeFromChunkTerm(p_chunk_type);
    
    MCAutoPointer<MCTextChunkIterator> tci = 
            MCStringsTextChunkIteratorCreate(ctxt, p_text, p_chunk_type);

    bool t_result;
    t_result = tci -> IsAmong(p_chunk);
    
    return t_result;

}

void MCStringsEvalIsAmongTheLinesOf(MCExecContext& ctxt, MCStringRef p_chunk, MCStringRef p_string, bool& r_result)
{
    r_result = MCStringsEvalIsAmongTheChunksOf(ctxt, p_chunk, p_string, CT_LINE);
}

void MCStringsEvalIsNotAmongTheLinesOf(MCExecContext& ctxt, MCStringRef p_chunk, MCStringRef p_string, bool& r_result)
{
    MCStringsEvalIsAmongTheLinesOf(ctxt, p_chunk, p_string, r_result);
	r_result = !r_result;
}

void MCStringsEvalIsAmongTheParagraphsOf(MCExecContext& ctxt, MCStringRef p_chunk, MCStringRef p_string, bool& r_result)
{
    r_result = MCStringsEvalIsAmongTheChunksOf(ctxt, p_chunk, p_string, CT_PARAGRAPH);
}

void MCStringsEvalIsNotAmongTheParagraphsOf(MCExecContext& ctxt, MCStringRef p_chunk, MCStringRef p_string, bool& r_result)
{
    MCStringsEvalIsAmongTheParagraphsOf(ctxt, p_chunk, p_string, r_result);
	r_result = !r_result;
}

void MCStringsEvalIsAmongTheSentencesOf(MCExecContext& ctxt, MCStringRef p_chunk, MCStringRef p_string, bool& r_result)
{
    r_result = MCStringsEvalIsAmongTheChunksOf(ctxt, p_chunk, p_string, CT_SENTENCE);
}

void MCStringsEvalIsNotAmongTheSentencesOf(MCExecContext& ctxt, MCStringRef p_chunk, MCStringRef p_string, bool& r_result)
{
    MCStringsEvalIsAmongTheSentencesOf(ctxt, p_chunk, p_string, r_result);
	r_result = !r_result;
}

void MCStringsEvalIsAmongTheItemsOf(MCExecContext& ctxt, MCStringRef p_chunk, MCStringRef p_string, bool& r_result)
{
    r_result = MCStringsEvalIsAmongTheChunksOf(ctxt, p_chunk, p_string, CT_ITEM);
}

void MCStringsEvalIsNotAmongTheItemsOf(MCExecContext& ctxt, MCStringRef p_chunk, MCStringRef p_string, bool& r_result)
{
    MCStringsEvalIsAmongTheItemsOf(ctxt, p_chunk, p_string, r_result);
	r_result = !r_result;
}

void MCStringsEvalIsAmongTheWordsOf(MCExecContext& ctxt, MCStringRef p_chunk, MCStringRef p_string, bool& r_result)
{
    r_result = MCStringsEvalIsAmongTheChunksOf(ctxt, p_chunk, p_string, CT_WORD);
}

void MCStringsEvalIsNotAmongTheWordsOf(MCExecContext& ctxt, MCStringRef p_chunk, MCStringRef p_string, bool& r_result)
{
    MCStringsEvalIsAmongTheWordsOf(ctxt, p_chunk, p_string, r_result);
	r_result = !r_result;
}

void MCStringsEvalIsAmongTheTrueWordsOf(MCExecContext& ctxt, MCStringRef p_chunk, MCStringRef p_string, bool& r_result)
{
    r_result = MCStringsEvalIsAmongTheChunksOf(ctxt, p_chunk, p_string, CT_TRUEWORD);
}

void MCStringsEvalIsNotAmongTheTrueWordsOf(MCExecContext& ctxt, MCStringRef p_chunk, MCStringRef p_string, bool& r_result)
{
    MCStringsEvalIsAmongTheTrueWordsOf(ctxt, p_chunk, p_string, r_result);
	r_result = !r_result;
}

void MCStringsEvalIsAmongTheTokensOf(MCExecContext& ctxt, MCStringRef p_chunk, MCStringRef p_string, bool& r_result)
{
    r_result = MCStringsEvalIsAmongTheChunksOf(ctxt, p_chunk, p_string, CT_TOKEN);
}

void MCStringsEvalIsNotAmongTheTokensOf(MCExecContext& ctxt, MCStringRef p_chunk, MCStringRef p_string, bool& r_result)
{
    MCStringsEvalIsAmongTheTokensOf(ctxt, p_chunk, p_string, r_result);
	r_result = !r_result;
}

void MCStringsEvalIsAmongTheCharsOf(MCExecContext& ctxt, MCStringRef p_chunk, MCStringRef p_string, bool& r_result)
{
    r_result = MCStringsEvalIsAmongTheChunksOf(ctxt, p_chunk, p_string, CT_CHARACTER);
}

void MCStringsEvalIsNotAmongTheCharsOf(MCExecContext& ctxt, MCStringRef p_chunk, MCStringRef p_string, bool& r_result)
{
    MCStringsEvalIsAmongTheCharsOf(ctxt, p_chunk, p_string, r_result);
	r_result = !r_result;
}

void MCStringsEvalIsAmongTheCodepointsOf(MCExecContext& ctxt, MCStringRef p_chunk, MCStringRef p_string, bool& r_result)
{
    r_result = MCStringsEvalIsAmongTheChunksOf(ctxt, p_chunk, p_string, CT_CODEPOINT);
}

void MCStringsEvalIsNotAmongTheCodepointsOf(MCExecContext& ctxt, MCStringRef p_chunk, MCStringRef p_string, bool& r_result)
{
    MCStringsEvalIsAmongTheCodepointsOf(ctxt, p_chunk, p_string, r_result);
	r_result = !r_result;
}

void MCStringsEvalIsAmongTheCodeunitsOf(MCExecContext& ctxt, MCStringRef p_chunk, MCStringRef p_string, bool& r_result)
{
    r_result = MCStringsEvalIsAmongTheChunksOf(ctxt, p_chunk, p_string, CT_CODEUNIT);
}

void MCStringsEvalIsNotAmongTheCodeunitsOf(MCExecContext& ctxt, MCStringRef p_chunk, MCStringRef p_string, bool& r_result)
{
    MCStringsEvalIsAmongTheCodeunitsOf(ctxt, p_chunk, p_string, r_result);
	r_result = !r_result;
}

void MCStringsEvalIsAmongTheBytesOf(MCExecContext& ctxt, MCDataRef p_chunk, MCDataRef p_string, bool& r_result)
{
    if (MCDataIsEmpty(p_chunk))
    {
        r_result = false;
        return;
    }
    r_result = MCDataContains(p_string, p_chunk);
}

void MCStringsEvalIsNotAmongTheBytesOf(MCExecContext& ctxt, MCDataRef p_chunk, MCDataRef p_string, bool& r_result)
{
    MCStringsEvalIsAmongTheBytesOf(ctxt, p_chunk, p_string, r_result);
	r_result = !r_result;
}

////////////////////////////////////////////////////////////////////////////////

static uindex_t
__MCStringsEvalChunkOffset(MCExecContext& ctxt,
                           MCStringRef p_chunk,
                           MCStringRef p_string,
                           uindex_t p_start_offset,
                           Chunk_term p_chunk_type)
{
    MCChunkType t_type;
    t_type = MCChunkTypeFromChunkTerm(p_chunk_type);
    
    MCAutoPointer<MCTextChunkIterator> tci =
            MCStringsTextChunkIteratorCreate(ctxt, p_string, p_chunk_type);
    
    uindex_t t_offset = tci -> ChunkOffset(p_chunk, p_start_offset, nil, ctxt . GetWholeMatches());
    
    return t_offset;
}

// This function implements the semantics of the (line|item)Offset functions.
//
// If needle is empty, then the result is 0.
//
// First, <start_offset> delimiters are skipped - resulting in the result of the
// operation occuring on haystack starting at the first character after the last
// skipped delimiter.
//
// If needle is not in the revised haystack, then the result is 0.
//
// If wholeMatches is false, then the result is the number of delimiters between
// the start of the revised haystack and the first occurrence of needle.
//
// If wholeMatches is true, then if either side of the found needle is bos, eos
// or delimiter, the result is the same as for wholeMatches false; otherwise, the
// result is 0.
//
static void
__MCStringEvalDelimitedChunkOffset(MCExecContext& ctxt,
                                   MCStringRef p_needle,
                                   MCStringRef p_haystack,
                                   MCStringRef p_delimiter,
                                   uindex_t p_start_offset,
                                   uindex_t& r_result)
{
    uindex_t t_index;
    bool t_present;
    if (!ctxt . GetWholeMatches())
    {
        // If we aren't interested in whole matches, then we just search once.
        t_present = MCStringDelimitedOffset(p_haystack,
                                            MCRangeMake(0, MCStringGetLength(p_haystack)),
                                            p_needle,
                                            p_delimiter,
                                            p_start_offset,
                                            ctxt . GetStringComparisonType(),
                                            t_index,
                                            nil,
                                            nil,
                                            nil);
    }
    else
    {
        // If we do want whole matches, we need to make sure that we search until
        // we exhaust the haystack of occurrances of needle, or a needle is found
        // with a delimiter either side.
        
        // Initialize to search the whole of the haystack.
        MCRange t_range;
        t_range = MCRangeMake(0, MCStringGetLength(p_haystack));
        
        // As we iterate by searching through a decreasing suffix of the haystack
        // we must keep a running count of the index.
        t_index = 0;
        
        // The first time through we need to use the start offset.
        uindex_t t_start_offset;
        t_start_offset = p_start_offset;
        
        for(;;)
        {
            MCRange t_found, t_before, t_after;
            uindex_t t_relative_index;
            t_present = MCStringDelimitedOffset(p_haystack,
                                                t_range,
                                                p_needle,
                                                p_delimiter,
                                                t_start_offset,
                                                ctxt . GetStringComparisonType(),
                                                t_relative_index,
                                                &t_found,
                                                &t_before,
                                                &t_after);
        
            // If there are no more occurrances of needle, then we are done.
            if (!t_present)
                break;
            
            // Update the running index.
            t_index += t_relative_index;
            
            // If the found string has a delimiter either side, then we are
            // done.
            if (t_found . offset == t_before . offset + t_before . length &&
                t_found . offset + t_found . length == t_after . offset)
                break;
            
            // We must now update the search range to start after the after
            // delimiter.
            t_range = MCRangeMakeMinMax(t_after . offset + t_after . length,
                                        MCStringGetLength(p_haystack));
            
            // We have just skipped a delimiter (the 'after' one) so increment
            // the index.
            t_index += 1;
            
            // We don't want to skip any delimiters next time through.
            t_start_offset = 0;
        }
    }
    
    if (!t_present)
    {
        r_result = 0;
        return;
    }
    
    r_result = t_index - p_start_offset + 1;
}

void MCStringsEvalLineOffset(MCExecContext& ctxt, MCStringRef p_chunk, MCStringRef p_string, uindex_t p_start_offset, uindex_t& r_result)
{
    __MCStringEvalDelimitedChunkOffset(ctxt,
                                       p_chunk,
                                       p_string,
                                       ctxt . GetLineDelimiter(),
                                       p_start_offset,
                                       r_result);
}

void MCStringsEvalParagraphOffset(MCExecContext& ctxt, MCStringRef p_chunk, MCStringRef p_string, uindex_t p_start_offset, uindex_t& r_result)
{
    r_result = __MCStringsEvalChunkOffset(ctxt, p_chunk, p_string, p_start_offset, CT_PARAGRAPH);
}

void MCStringsEvalSentenceOffset(MCExecContext& ctxt, MCStringRef p_chunk, MCStringRef p_string, uindex_t p_start_offset, uindex_t& r_result)
{
    r_result = __MCStringsEvalChunkOffset(ctxt, p_chunk, p_string, p_start_offset, CT_SENTENCE);
}

void MCStringsEvalItemOffset(MCExecContext& ctxt, MCStringRef p_chunk, MCStringRef p_string, uindex_t p_start_offset, uindex_t& r_result)
{
    __MCStringEvalDelimitedChunkOffset(ctxt,
                                       p_chunk,
                                       p_string,
                                       ctxt . GetItemDelimiter(),
                                       p_start_offset,
                                       r_result);
}

void MCStringsEvalWordOffset(MCExecContext& ctxt, MCStringRef p_chunk, MCStringRef p_string, uindex_t p_start_offset, uindex_t& r_result)
{
    r_result = __MCStringsEvalChunkOffset(ctxt, p_chunk, p_string, p_start_offset, CT_WORD);
}

void MCStringsEvalTrueWordOffset(MCExecContext& ctxt, MCStringRef p_chunk, MCStringRef p_string, uindex_t p_start_offset, uindex_t& r_result)
{
    r_result = __MCStringsEvalChunkOffset(ctxt, p_chunk, p_string, p_start_offset, CT_TRUEWORD);
}

void MCStringsEvalTokenOffset(MCExecContext& ctxt, MCStringRef p_chunk, MCStringRef p_string, uindex_t p_start_offset, uindex_t& r_result)
{
    r_result = __MCStringsEvalChunkOffset(ctxt, p_chunk, p_string, p_start_offset, CT_TOKEN);
}

void MCStringsEvalCodepointOffset(MCExecContext& ctxt, MCStringRef p_chunk, MCStringRef p_string, uindex_t p_start_offset, uindex_t& r_result)
{
    r_result = __MCStringsEvalChunkOffset(ctxt, p_chunk, p_string, p_start_offset, CT_CODEPOINT);
}

void MCStringsEvalCodeunitOffset(MCExecContext& ctxt, MCStringRef p_chunk, MCStringRef p_string, uindex_t p_start_offset, uindex_t& r_result)
{
    r_result = __MCStringsEvalChunkOffset(ctxt, p_chunk, p_string, p_start_offset, CT_CODEUNIT);
}

void MCStringsEvalByteOffset(MCExecContext& ctxt, MCDataRef p_chunk, MCDataRef p_string, uindex_t p_start_offset, uindex_t& r_result)
{
    uindex_t t_result;
    t_result = 0;
    // SN-2014-09-05: [[ Bug 13346 ]] byteOffset is 0 if the byte is not found, and 'empty'
    // is by definition not found; getting in the loop ensures at least 1 is returned.
    
    if (MCDataFirstIndexOf(p_string, p_chunk, MCRangeMake(p_start_offset, UINDEX_MAX), t_result))
        t_result++;
    
    r_result = t_result;
}

void MCStringsEvalOffset(MCExecContext& ctxt, MCStringRef p_chunk, MCStringRef p_string, uindex_t p_start_offset, uindex_t& r_result)
{
	MCStringOptions t_options = ctxt.GetStringComparisonType();
    uindex_t t_offset;
    MCRange t_char_range, t_cu_range;
    // AL-2015-05-07: [[ Bug 15327 ]] Start offset is grapheme offset not codeunit, so map to grapheme offset first.
    MCStringMapIndices(p_string, kMCCharChunkTypeGrapheme, MCRangeMake(0, p_start_offset), t_cu_range);
	// AL-2014-05-27: [[ Bug 12517 ]] Offset should be 0 for an empty input string
	if (MCStringIsEmpty(p_chunk) || !MCStringFirstIndexOf(p_string, p_chunk, t_cu_range.length, t_options, t_offset))
		r_result = 0;
	else
    {
        // We want to get the grapheme length, not the codeunit one
        t_cu_range . offset = 0;
        t_cu_range . length = t_offset;
        MCStringUnmapIndices(p_string, kMCCharChunkTypeGrapheme, t_cu_range, t_char_range);
        
		r_result = t_char_range . length - p_start_offset + 1;
    }
}

////////////////////////////////////////////////////////////////////////////////

void MCStringsExecReplace(MCExecContext& ctxt, MCStringRef p_pattern, MCStringRef p_replacement, MCStringRef p_target)
{
	MCStringOptions t_options = ctxt.GetStringComparisonType();
	if (MCStringFindAndReplace(p_target, p_pattern, p_replacement, t_options))
		return;
	
	ctxt . Throw();
}

////////////////////////////////////////////////////////////////////////////////

void MCStringsExecFilterDelimited(MCExecContext& ctxt, MCStringRef p_source, bool p_without, MCStringRef p_delimiter, MCPatternMatcher *p_matcher, MCStringRef &r_result)
{
	uint32_t t_length = MCStringGetLength(p_source);
	if (t_length == 0)
        MCStringCopy(kMCEmptyString, r_result);

	MCAutoListRef t_output;
    MCListCreateMutable(p_delimiter, &t_output);

	// OK-2010-01-11: Bug 7649 - Filter command was incorrectly removing empty lines.
	// Now ignores delimiter for matching but includes it in the append.

    uindex_t t_last_offset = 0;
	bool t_found = true;
    bool t_success = true;
    
    MCRange t_chunk_range, t_found_range;
    MCStringOptions t_options = ctxt . GetStringComparisonType();
	while (t_found && t_success)
	{
        t_found = MCStringFind(p_source, MCRangeMake(t_last_offset, UINDEX_MAX), p_delimiter, t_options, &t_found_range);
		if (!t_found) //last line or item
        {
            t_chunk_range . offset = t_last_offset;
            t_chunk_range . length = t_length - t_last_offset;
        }
		else
        {
            t_chunk_range . offset = t_last_offset;
            t_chunk_range . length = t_found_range . offset - t_last_offset;
        }
        
        if (t_success && (p_matcher -> match(ctxt, t_chunk_range) != p_without))
        {
            MCAutoStringRef t_line;
            t_success = MCStringCopySubstring(p_source, t_chunk_range, &t_line) && MCListAppend(*t_output, *t_line);
        }

		t_last_offset = t_found_range . offset + t_found_range . length;
	}
	
    if (!t_success)
    {
        // IM-2013-07-26: [[ Bug 10774 ]] if filterlines fails throw a "no memory" error
        ctxt . LegacyThrow(EE_NO_MEMORY);
        MCStringCopy(kMCEmptyString, r_result);
    }
    else if (!MCListIsEmpty(*t_output))
        /* UNCHECKED */ MCListCopyAsString(*t_output, r_result);
	else
        r_result = MCValueRetain(kMCEmptyString);
}

void MCStringsExecFilterWildcard(MCExecContext& ctxt, MCStringRef p_source, MCStringRef p_pattern, bool p_without, bool p_lines, MCStringRef &r_result)
{
    // Create the pattern matcher
	MCWildcardMatcher t_matcher(p_pattern, p_source, ctxt . GetStringComparisonType());
    
    MCStringsExecFilterDelimited(ctxt, p_source, p_without, p_lines ? ctxt . GetLineDelimiter() : ctxt . GetItemDelimiter(), &t_matcher, r_result);
}

void MCStringsExecFilterRegex(MCExecContext& ctxt, MCStringRef p_source, MCStringRef p_pattern, bool p_without, bool p_lines, MCStringRef &r_result)
{
	// Create the pattern matcher
	MCRegexMatcher t_matcher(p_pattern, p_source, ctxt . GetStringComparisonType());
    
    MCAutoStringRef t_regex_error;
    if (!t_matcher.compile(&t_regex_error))
    {
        ctxt . LegacyThrow(EE_MATCH_BADPATTERN);
        return;
    }
    
    MCStringsExecFilterDelimited(ctxt, p_source, p_without, p_lines ? ctxt . GetLineDelimiter() : ctxt . GetItemDelimiter(), &t_matcher, r_result);
}

void MCStringsExecFilterExpression(MCExecContext& ctxt, MCStringRef p_source, MCExpression* p_expression, bool p_without, bool p_lines, MCStringRef &r_result)
{
    // Create the pattern matcher
    MCExpressionMatcher t_matcher(p_expression, p_source, ctxt . GetStringComparisonType());
    
    MCStringsExecFilterDelimited(ctxt, p_source, p_without, p_lines ? ctxt . GetLineDelimiter() : ctxt . GetItemDelimiter(), &t_matcher, r_result);
}

////////////////////////////////////////////////////////////////////////////////

void MCStringsEvalIsAscii(MCExecContext& ctxt, MCValueRef p_value, bool& r_result)
{
    // SN-2015-11-26: [[ Bug 16500 ]] Arrays will successfully convert to an
    // empty string, which is a valid ASCII string. Cut short in this case
    if (MCValueIsArray(p_value))
    {
        r_result = false;
        return;
    }

    MCAutoStringRef t_string;
	if (!ctxt . ConvertToString(p_value, &t_string))
    {
        r_result = false;
        return;
    }

    // SN-2015-11-27: [[ Bug 16504 ]] Empty strings are not a valid ascii string
    if (MCStringIsEmpty(*t_string))
    {
        r_result = false;
        return;
    }

    MCAutoPointer<char> temp;
    /* UNCHECKED */ MCStringConvertToCString(*t_string, &temp);
    const char *t_cstring;
    t_cstring = *temp;
    
    if (!MCStringIsEqualToCString(*t_string, t_cstring, kMCCompareExact))
    {
        r_result = false;
        return;
    }
    
    bool t_is_ascii;
    t_is_ascii = true;
    
    const uint1* t_chars = (const uint1 *) *temp;
    int t_length = MCStringGetLength(*t_string);
    for (int i=0; i < t_length ;i++)
        if (t_chars[i] > 127)
        {
            t_is_ascii = false;
            break;
        }
    
    r_result = t_is_ascii;
}

void MCStringsEvalIsNotAscii(MCExecContext& ctxt, MCValueRef p_value, bool& r_result)
{
    bool t_result;
    MCStringsEvalIsAscii(ctxt, p_value, t_result);
    r_result = !t_result;
}

////////////////////////////////////////////////////////////////////////////////

void MCStringsDoSort(MCSortnode *b, uint4 n, MCSortnode *t, Sort_type form, bool reverse, MCStringOptions p_options)
{
    if (n <= 1)
		return;
    
	uint4 n1 = n / 2;
	uint4 n2 = n - n1;
	MCSortnode *b1 = b;
	MCSortnode *b2 = b + n1;
    
	MCStringsDoSort(b1, n1, t, form, reverse, p_options);
	MCStringsDoSort(b2, n2, t, form, reverse, p_options);
    
	MCSortnode *tmp = t;
	while (n1 > 0 && n2 > 0)
	{
		// NOTE:
		//
		// This code assumes the types in the MCSortnodes are correct for the
		// requested sort type. Bad things will happen if this isn't true...
		bool first;
		switch (form)
		{
            case ST_INTERNATIONAL:
			{
                const unichar_t *t1, *t2;
                t1 = MCStringGetCharPtr(b1->svalue);
                t2 = MCStringGetCharPtr(b2->svalue);
                
                compare_t result = MCUnicodeCollate(kMCSystemLocale, MCUnicodeCollateOptionFromCompareOption((MCUnicodeCompareOption)p_options), t1, MCStringGetLength(b1->svalue), t2, MCStringGetLength(b2->svalue));
				first = reverse ? result >= 0 : result <= 0;
				break;
			}
                
            case ST_TEXT:
			{
				// This mode performs the comparison in a locale-independent,
                // case-sensitive manner. The strings are sorted by order of
                // codepoint values rather than any lexical sorting order.
                compare_t result = MCStringCompareTo(b1->svalue, b2->svalue, kMCStringOptionCompareExact);
                
				first = reverse ? result >= 0 : result <= 0;
				break;
			}
            case ST_BINARY:
            {
                compare_t result = MCDataCompareTo(b1->dvalue, b2->dvalue);
                
				first = reverse ? result >= 0 : result <= 0;
				break;
            }
            default:
			{
				first = reverse
                ? MCNumberFetchAsReal(b1->nvalue) >= MCNumberFetchAsReal(b2->nvalue)
                : MCNumberFetchAsReal(b1->nvalue) <= MCNumberFetchAsReal(b2->nvalue);
				break;
			}
        }
		if (first)
		{
			*tmp++ = *b1++;
			n1--;
		}
		else
		{
			*tmp++ = *b2++;
			n2--;
		}
	}
    
	for (uindex_t i = 0; i < n1; i++)
		tmp[i] = b1[i];
	for (uindex_t i = 0; i < (n - n2); i++)
		b[i] = t[i];
}

void MCStringsSort(MCSortnode *p_items, uint4 nitems, Sort_type p_dir, Sort_type p_form, MCStringOptions p_options)
{
    if (nitems > 1)
    {
        MCSortnode *tmp = new (nothrow) MCSortnode[nitems];
        MCStringsDoSort(p_items, nitems, tmp, p_form, p_dir == ST_DESCENDING, p_options);
        delete[] tmp;
    }
}

void MCStringsSortAddItem(MCExecContext &ctxt, MCSortnode *items, uint4 &nitems, int form, MCValueRef p_input, MCExpression *by)
{
    bool t_success;
    t_success = true;
    
	MCAutoValueRef t_output;
	if (by != NULL)
	{
		MCerrorlock++;
        if (p_input != nil)
            MCeach->set(ctxt, p_input);
        t_success = ctxt . EvalExprAsValueRef(by, EE_UNDEFINED, &t_output);
		MCerrorlock--;
	}
	else
        t_output = p_input;
	
	MCAutoStringRef t_converted;
	switch (form)
	{
        case ST_DATETIME:
            if (t_success && MCD_convert(ctxt, *t_output, CF_UNDEFINED, CF_UNDEFINED, CF_SECONDS, CF_UNDEFINED, &t_converted))
                if (ctxt.ConvertToNumber(*t_converted, items[nitems].nvalue))
                    break;
            
            /* UNCHECKED */ MCNumberCreateWithReal(-MAXREAL8, items[nitems].nvalue);
            break;
			
        case ST_NUMERIC:
            // AL-2014-07-21: [[ Bug 12847 ]] If output is empty, don't construe as 0 for sorting purposes
            if (t_success && !MCValueIsEmpty(*t_output))
            {
                if (ctxt.ConvertToNumber(*t_output, items[nitems].nvalue))
                    break;
                
                // AL-2014-10-14: [[ Bug 13664 ]] Try to parse numeric initial segment of string
                MCAutoStringRef t_string;
                if (ctxt . ConvertToString(*t_output, &t_string))
                {
                    uindex_t t_start, t_end, t_length;
                    t_length = MCStringGetLength(*t_string);
                    t_start = 0;
                    // if there are consecutive spaces at the beginning, skip them
                    while (t_start < t_length && MCUnicodeIsWhitespace(MCStringGetCharAtIndex(*t_string, t_start)))
                        t_start++;
                    
                    t_end = t_start;
                    while (t_end < t_length)
                    {
                        char_t t_char = MCStringGetNativeCharAtIndex(*t_string, t_end);
                        if (!isdigit((uint1)t_char) && t_char != '.' && t_char != '-' && t_char != '+')
                            break;
                        
                        t_end++;
                    }
                    
                    MCAutoStringRef t_numeric_part;
                    if (t_end != t_start &&
                        MCStringCopySubstring(*t_string, MCRangeMakeMinMax(t_start, t_end), &t_numeric_part) &&
                        ctxt . ConvertToNumber(*t_numeric_part, items[nitems].nvalue))
                        break;
                }
                
			}
            /* UNCHECKED */ MCNumberCreateWithReal(-MAXREAL8, items[nitems].nvalue);
            break;
        case ST_BINARY:
            if (t_success && ctxt.ConvertToData(*t_output, items[nitems].dvalue))
                break;
            items[nitems] . dvalue = MCValueRetain(kMCEmptyData);
        default:
            if (ctxt . GetCaseSensitive())
			{
                if (t_success && ctxt.ConvertToString(*t_output, items[nitems].svalue))
                    break;
            }
            else
            {
                MCStringRef t_fixed, t_mutable;
                t_fixed = nil;
                t_mutable = nil;
                if (t_success)
                    t_success = ctxt.ConvertToString(*t_output, t_fixed) &&
                                MCStringMutableCopyAndRelease(t_fixed, t_mutable) &&
                                MCStringLowercase(t_mutable, kMCSystemLocale) &&
                                MCStringCopyAndRelease(t_mutable, items[nitems].svalue);
                
                if (t_success)
                    break;
                
                MCValueRelease(t_fixed);
                MCValueRelease(t_mutable);
            }
            items[nitems].svalue = MCValueRetain(kMCEmptyString);
			
            break;
	}
	nitems++;
}

////////////////////////////////////////////////////////////////////////////////

typedef bool (*comparator_t)(void *context, uindex_t left, uindex_t right);
typedef void (*freer_t)(void *keys, uindex_t count);

static void MCStringsDoSortIndirect(uindex_t *b, uint4 n, uindex_t *t, comparator_t is_less_or_equal, void *context)
{
    if (n <= 1)
		return;
    
	uint4 n1 = n / 2;
	uint4 n2 = n - n1;
	uindex_t *b1 = b;
	uindex_t *b2 = b + n1;
    
	MCStringsDoSortIndirect(b1, n1, t, is_less_or_equal, context);
	MCStringsDoSortIndirect(b2, n2, t, is_less_or_equal, context);
    
	uindex_t *tmp = t;
	while (n1 > 0 && n2 > 0)
	{
		if (is_less_or_equal(context, *b1, *b2))
		{
			*tmp++ = *b1++;
			n1--;
		}
		else
		{
			*tmp++ = *b2++;
			n2--;
		}
	}
    
	for (uindex_t i = 0; i < n1; i++)
		tmp[i] = b1[i];
	for (uindex_t i = 0; i < (n - n2); i++)
		b[i] = t[i];
}

static void MCStringsSortIndirect(uindex_t *p_items, uint4 nitems, comparator_t is_less_or_equal, void *context)
{
    if (nitems == 0)
        return;
    
    uindex_t *tmp = new (nothrow) uindex_t[nitems];
    MCStringsDoSortIndirect(p_items, nitems, tmp, is_less_or_equal, context);
    delete[] tmp;
}

static bool double_comparator_fwd(void *p_context, uindex_t p_left, uindex_t p_right)
{
    double *t_keys;
    t_keys = (double *)p_context;
    return t_keys[p_left] <= t_keys[p_right];
}

static bool double_comparator_rev(void *p_context, uindex_t p_left, uindex_t p_right)
{
    double *t_keys;
    t_keys = (double *)p_context;
    return t_keys[p_left] >= t_keys[p_right];
}

static void double_freer(void *keys, uindex_t count)
{
    double *t_doubles;
    t_doubles = (double *)keys;
    delete[] t_doubles;
}

static bool data_comparator_fwd(void *p_context, uindex_t p_left, uindex_t p_right)
{
    MCDataRef *t_keys;
    t_keys = (MCDataRef *)p_context;
    return MCDataCompareTo(t_keys[p_left], t_keys[p_right]) <= 0;
}

static bool data_comparator_rev(void *p_context, uindex_t p_left, uindex_t p_right)
{
    MCDataRef *t_keys;
    t_keys = (MCDataRef *)p_context;
    return MCDataCompareTo(t_keys[p_left], t_keys[p_right]) >= 0;
}

static bool string_comparator_fwd(void *p_context, uindex_t p_left, uindex_t p_right)
{
    MCStringRef *t_keys;
    t_keys = (MCStringRef *)p_context;
    return MCStringCompareTo(t_keys[p_left], t_keys[p_right], kMCStringOptionCompareExact) <= 0;
}

static bool string_comparator_rev(void *p_context, uindex_t p_left, uindex_t p_right)
{
    MCStringRef *t_keys;
    t_keys = (MCStringRef *)p_context;
    return MCStringCompareTo(t_keys[p_left], t_keys[p_right], kMCStringOptionCompareExact) >= 0;
}

static void valueref_freer(void *keys, uindex_t count)
{
    MCValueRef *t_values;
    t_values = (MCValueRef *)keys;
    for(uindex_t i = 0; i < count; i++)
        MCValueRelease(t_values[i]);
    delete[] t_values;
}

static bool MCStringCopyFoldedAndRelease(MCStringRef p_string, MCStringOptions p_options, MCStringRef& r_folded_string)
{
    if (p_options == kMCStringOptionCompareExact)
    {
        r_folded_string = p_string;
        return true;
    }
    
    if (!MCStringMutableCopyAndRelease(p_string, p_string))
        return false;
    
    if (!MCStringFold(p_string, p_options))
        return false;
    
    if (!MCStringCopyAndRelease(p_string, p_string))
        return false;
    
    r_folded_string = p_string;
    
    return true;
}

void MCStringsExecSort(MCExecContext& ctxt, Sort_type p_dir, Sort_type p_form, MCStringRef *p_items, uindex_t p_count, MCExpression *p_by, MCStringRef*& r_sorted_array, uindex_t& r_sorted_count)
{
    // If there are no items to sort, do nothing.
    if (p_count == 0)
        return;
    
    // Indicates if all items are stringrefs.
    bool t_all_strings;
    t_all_strings = true;
    
    // Process the items if there is a 'by'.
    MCValueRef *t_temp_items;
    MCValueRef *t_items;
    t_temp_items = nil;
    if (p_by != nil)
    {
        t_temp_items = new (nothrow) MCValueRef[p_count];
        MCerrorlock++;
        for(uindex_t i = 0; i < p_count; i++)
        {
            MCeach -> set(ctxt, p_items[i]);
            if (!ctxt . EvalExprAsValueRef(p_by, EE_UNDEFINED, t_temp_items[i]))
                t_temp_items[i] = MCValueRetain(p_items[i]);
            if (MCValueGetTypeCode(t_temp_items[i]) != kMCValueTypeCodeString)
                t_all_strings = false;
        }
        MCerrorlock--;
        t_items = t_temp_items;
    }
    else
        t_items = (MCValueRef *)p_items;
    
    // Build the vector of indicies to sort.
    uindex_t *t_indicies;
    t_indicies = new (nothrow) uindex_t[p_count];
    for(uindex_t i = 0; i < p_count; i++)
        t_indicies[i] = i;
    
    // Now generate the sort keys - what type these are will depend on the
    // type of sort.
    void *t_sort_keys;
    comparator_t t_sort_compare;
    freer_t t_sort_freer;
    
    switch(p_form)
    {
        case ST_DATETIME:
        {
            // DateTime is sorted by seconds.
            double *t_seconds;
            t_seconds = new (nothrow) double[p_count];
            for(uindex_t i = 0; i < p_count; i++)
            {
                MCDateTime t_datetime;
                if (!MCD_convert_to_datetime(ctxt, t_items[i], CF_UNDEFINED, CF_UNDEFINED, t_datetime) ||
                    !MCS_datetimetoseconds(t_datetime, t_seconds[i]))
                    t_seconds[i] = -MAXREAL8;
            }
            
            t_sort_keys = t_seconds;
            t_sort_compare = p_dir == ST_ASCENDING ? double_comparator_fwd : double_comparator_rev;
            t_sort_freer = double_freer;
        }
        break;
            
        case ST_NUMERIC:
        {
            double *t_numbers;
            t_numbers = new (nothrow) double[p_count];
            for(uindex_t i = 0; i < p_count; i++)
            {
                if (MCValueIsEmpty(t_items[i]))
                {
                    t_numbers[i] = -MAXREAL8;
                    continue;
                }
                
                if (!ctxt . ConvertToReal(t_items[i], t_numbers[i]))
                {
                    MCAutoStringRef t_string;
                    if (!ctxt . ConvertToString(t_items[i], &t_string))
                    {
                        t_numbers[i] = -MAXREAL8;
                        continue;
                    }
                    
                    uindex_t t_start, t_end, t_length;
                    t_length = MCStringGetLength(*t_string);
                    t_start = 0;
                    
                    // if there are consecutive spaces at the beginning, skip them
                    while (t_start < t_length && MCUnicodeIsWhitespace(MCStringGetCharAtIndex(*t_string, t_start)))
                        t_start++;
                    
                    t_end = t_start;
                    while (t_end < t_length)
                    {
                        char_t t_char = MCStringGetNativeCharAtIndex(*t_string, t_end);
                        if (!isdigit((uint1)t_char) && t_char != '.' && t_char != '-' && t_char != '+')
                            break;
                        
                        t_end++;
                    }
                    
                    MCAutoStringRef t_numeric_part;
                    if (t_end == t_start ||
                        !MCStringCopySubstring(*t_string, MCRangeMakeMinMax(t_start, t_end), &t_numeric_part) ||
                        !ctxt . ConvertToReal(*t_numeric_part, t_numbers[i]))
                    {
                        t_numbers[i] = -MAXREAL8;
                        continue;
                    }
                }
            }
            
            t_sort_keys = t_numbers;
            t_sort_compare = p_dir == ST_ASCENDING ? double_comparator_fwd : double_comparator_rev;;
            t_sort_freer = double_freer;
        }
        break;
            
        case ST_BINARY:
        {
            MCDataRef *t_datas;
            t_datas = new (nothrow) MCDataRef[p_count];
            for(uindex_t i = 0; i < p_count; i++)
            {
                if (!ctxt . ConvertToData(t_items[i], t_datas[i]))
                    t_datas[i] = MCValueRetain(kMCEmptyData);
            }
            
            t_sort_keys = t_datas;
            t_sort_compare = p_dir == ST_ASCENDING ? data_comparator_fwd : data_comparator_rev;
            t_sort_freer = valueref_freer;
        }
        break;
        
        case ST_TEXT:
        {
            MCStringOptions t_options;
            t_options = ctxt . GetStringComparisonType();
            if (t_options == kMCStringOptionCompareExact &&
                t_all_strings)
            {
                t_sort_keys = t_items;
                t_sort_compare = p_dir == ST_ASCENDING ? string_comparator_fwd : string_comparator_rev;
                t_sort_freer = nil;
            }
            else
            {
                MCStringRef *t_strings;
                t_strings = new (nothrow) MCStringRef[p_count];
                for(uindex_t i = 0; i < p_count; i++)
                {
                    if (!ctxt . ConvertToString(t_items[i], t_strings[i]) ||
                        !MCStringCopyFoldedAndRelease(t_strings[i], t_options, t_strings[i]))
                        t_strings[i] = MCValueRetain(kMCEmptyString);
                }
                
                t_sort_keys = t_strings;
                t_sort_compare = p_dir == ST_ASCENDING ? string_comparator_fwd : string_comparator_rev;
                t_sort_freer = valueref_freer;
            }
        }
        break;
            
        case ST_INTERNATIONAL:
        {
            MCUnicodeCollateOption t_options;
            t_options = MCUnicodeCollateOptionFromCompareOption((MCUnicodeCompareOption)ctxt . GetStringComparisonType());
            
            MCUnicodeCollatorRef t_collator;
            /* UNCHECKED */ MCUnicodeCreateCollator(kMCSystemLocale, t_options, t_collator);
            
            MCDataRef *t_datas;
            t_datas = new (nothrow) MCDataRef[p_count];
            for(uindex_t i = 0; i < p_count; i++)
            {
                MCAutoStringRef t_string;
                if (!ctxt . ConvertToString(t_items[i], &t_string))
                {
                    t_datas[i] = MCValueRetain(kMCEmptyData);
                    continue;
                }
                
                byte_t *t_key;
                uindex_t t_key_length;
                if (!MCUnicodeCreateSortKeyWithCollator(t_collator, MCStringGetCharPtr(*t_string), MCStringGetLength(*t_string), t_key, t_key_length))
                {
                    t_datas[i] = MCValueRetain(kMCEmptyData);
                    continue;
                }
                
                if (!MCDataCreateWithBytesAndRelease(t_key, t_key_length, t_datas[i]))
                {
                    free(t_key);
                    t_datas[i] = MCValueRetain(kMCEmptyData);
                    continue;
                }
            }
            
            MCUnicodeDestroyCollator(t_collator);
            
            t_sort_keys = t_datas;
            t_sort_compare = p_dir == ST_ASCENDING ? data_comparator_fwd : data_comparator_rev;
            t_sort_freer = valueref_freer;
        }
        break;
            
        default:
            delete[] t_indicies;
            MCUnreachableReturn();
    }
    
    MCStringsSortIndirect(t_indicies, p_count, t_sort_compare, t_sort_keys);
    
    if (t_sort_freer != nil)
        t_sort_freer(t_sort_keys, p_count);
    
    if (t_temp_items != nil)
    {
        for(uindex_t i = 0; i < p_count; i++)
            MCValueRelease(t_temp_items[i]);
        delete[] t_temp_items;
    }
    
    MCAutoArray<MCStringRef> t_sorted;
    for (uindex_t i = 0; i < p_count; i++)
        t_sorted . Push((MCStringRef)p_items[t_indicies[i]]);
    t_sorted . Take(r_sorted_array, r_sorted_count);
    
    delete[] t_indicies;
}

////////////////////////////////////////////////////////////////////////////////

// AL-2014-10-17: [[ BiDi ]] Returns the result of applying the bi-directional algorithm to text
void MCStringsEvalBidiDirection(MCExecContext& ctxt, MCStringRef p_string, MCStringRef& r_result)
{
    bool t_ltr;
    t_ltr = MCStringResolvesLeftToRight(p_string);
    
    if (MCStringCreateWithCString(t_ltr ? "ltr" : "rtl", r_result))
        return;
    
    ctxt . Throw();
}

////////////////////////////////////////////////////////////////////////////////
