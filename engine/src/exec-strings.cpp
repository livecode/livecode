/* Copyright (C) 2003-2013 Runtime Revolution Ltd.

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

#include "scriptpt.h"
#include "util.h"

#include "exec.h"
#include "exec-strings.h"

#include "chunk.h"

////////////////////////////////////////////////////////////////////////////////

MC_EXEC_DEFINE_EVAL_METHOD(Strings, ToLower, 2)
MC_EXEC_DEFINE_EVAL_METHOD(Strings, ToUpper, 2)
MC_EXEC_DEFINE_EVAL_METHOD(Strings, NumToChar, 2)
MC_EXEC_DEFINE_EVAL_METHOD(Strings, CharToNum, 2)
MC_EXEC_DEFINE_EVAL_METHOD(Strings, NumToByte, 2)
MC_EXEC_DEFINE_EVAL_METHOD(Strings, ByteToNum, 2)
MC_EXEC_DEFINE_EVAL_METHOD(Strings, TextDecode, 2);
MC_EXEC_DEFINE_EVAL_METHOD(Strings, TextEncode, 2);
MC_EXEC_DEFINE_EVAL_METHOD(Strings, NormalizeText, 2);
MC_EXEC_DEFINE_EVAL_METHOD(Strings, CodepointProperty, 2);
MC_EXEC_DEFINE_EVAL_METHOD(Strings, Length, 2)
MC_EXEC_DEFINE_EVAL_METHOD(Strings, MatchText, 4)
MC_EXEC_DEFINE_EVAL_METHOD(Strings, MatchChunk, 4)
MC_EXEC_DEFINE_EVAL_METHOD(Strings, ReplaceText, 4)
MC_EXEC_DEFINE_EVAL_METHOD(Strings, Format, 3)
MC_EXEC_DEFINE_EVAL_METHOD(Strings, Merge, 2)
MC_EXEC_DEFINE_EVAL_METHOD(Strings, Concatenate, 3)
MC_EXEC_DEFINE_EVAL_METHOD(Strings, ConcatenateWithSpace, 3)
MC_EXEC_DEFINE_EVAL_METHOD(Strings, ConcatenateWithComma, 3)
MC_EXEC_DEFINE_EVAL_METHOD(Strings, Contains, 3)
MC_EXEC_DEFINE_EVAL_METHOD(Strings, DoesNotContain, 3)
MC_EXEC_DEFINE_EVAL_METHOD(Strings, BeginsWith, 3)
MC_EXEC_DEFINE_EVAL_METHOD(Strings, EndsWith, 3)
MC_EXEC_DEFINE_EVAL_METHOD(Strings, IsAmongTheLinesOf, 3)
MC_EXEC_DEFINE_EVAL_METHOD(Strings, IsNotAmongTheLinesOf, 3)
MC_EXEC_DEFINE_EVAL_METHOD(Strings, IsAmongTheParagraphsOf, 3)
MC_EXEC_DEFINE_EVAL_METHOD(Strings, IsNotAmongTheParagraphsOf, 3)
MC_EXEC_DEFINE_EVAL_METHOD(Strings, IsAmongTheSentencesOf, 3)
MC_EXEC_DEFINE_EVAL_METHOD(Strings, IsNotAmongTheSentencesOf, 3)
MC_EXEC_DEFINE_EVAL_METHOD(Strings, IsAmongTheItemsOf, 3)
MC_EXEC_DEFINE_EVAL_METHOD(Strings, IsNotAmongTheItemsOf, 3)
MC_EXEC_DEFINE_EVAL_METHOD(Strings, IsAmongTheWordsOf, 3)
MC_EXEC_DEFINE_EVAL_METHOD(Strings, IsNotAmongTheWordsOf, 3)
MC_EXEC_DEFINE_EVAL_METHOD(Strings, IsAmongTheTrueWordsOf, 3)
MC_EXEC_DEFINE_EVAL_METHOD(Strings, IsNotAmongTheTrueWordsOf, 3)
MC_EXEC_DEFINE_EVAL_METHOD(Strings, IsAmongTheTokensOf, 3)
MC_EXEC_DEFINE_EVAL_METHOD(Strings, IsNotAmongTheTokensOf, 3)
MC_EXEC_DEFINE_EVAL_METHOD(Strings, IsAmongTheCharsOf, 3)
MC_EXEC_DEFINE_EVAL_METHOD(Strings, IsNotAmongTheCharsOf, 3)
MC_EXEC_DEFINE_EVAL_METHOD(Strings, IsAmongTheCodepointsOf, 3)
MC_EXEC_DEFINE_EVAL_METHOD(Strings, IsNotAmongTheCodepointsOf, 3)
MC_EXEC_DEFINE_EVAL_METHOD(Strings, IsAmongTheCodeunitsOf, 3)
MC_EXEC_DEFINE_EVAL_METHOD(Strings, IsNotAmongTheCodeunitsOf, 3)
MC_EXEC_DEFINE_EVAL_METHOD(Strings, IsAmongTheBytesOf, 3)
MC_EXEC_DEFINE_EVAL_METHOD(Strings, IsNotAmongTheBytesOf, 3)
MC_EXEC_DEFINE_EVAL_METHOD(Strings, LineOffset, 4)
MC_EXEC_DEFINE_EVAL_METHOD(Strings, ParagraphOffset, 4)
MC_EXEC_DEFINE_EVAL_METHOD(Strings, SentenceOffset, 4)
MC_EXEC_DEFINE_EVAL_METHOD(Strings, ItemOffset, 4)
MC_EXEC_DEFINE_EVAL_METHOD(Strings, WordOffset, 4)
MC_EXEC_DEFINE_EVAL_METHOD(Strings, TrueWordOffset, 4)
MC_EXEC_DEFINE_EVAL_METHOD(Strings, TokenOffset, 4)
MC_EXEC_DEFINE_EVAL_METHOD(Strings, CodepointOffset, 4)
MC_EXEC_DEFINE_EVAL_METHOD(Strings, CodeunitOffset, 4)
MC_EXEC_DEFINE_EVAL_METHOD(Strings, ByteOffset, 4)
MC_EXEC_DEFINE_EVAL_METHOD(Strings, Offset, 4)
MC_EXEC_DEFINE_EVAL_METHOD(Strings, IsAscii, 2)
MC_EXEC_DEFINE_EVAL_METHOD(Strings, IsNotAscii, 2)
MC_EXEC_DEFINE_EXEC_METHOD(Strings, Replace, 3)
MC_EXEC_DEFINE_EXEC_METHOD(Strings, FilterWildcard, 5)
MC_EXEC_DEFINE_EXEC_METHOD(Strings, FilterRegex, 5)
MC_EXEC_DEFINE_EXEC_METHOD(Strings, FilterWildcardIntoIt, 4)
MC_EXEC_DEFINE_EXEC_METHOD(Strings, FilterRegexIntoIt, 4)

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
		if (!MCStringCopySubstring(p_string, MCRangeMake(t_current, t_found - t_current), t_strings[t_count++]))
			return false;

		t_current = t_found + 1;
	}

	if (!t_strings.Extend(t_count + 1))
		return false;
	if (!MCStringCopySubstring(p_string, MCRangeMake(t_current, MCStringGetLength(p_string) - t_current), t_strings[t_count++]))
		return false;

	t_strings.Take(r_strings, r_count);
	return true;
}

////////////////////////////////////////////////////////////////////////////////

void MCStringsEvalToLower(MCExecContext& ctxt, MCStringRef p_string, MCStringRef& r_lower)
{
	MCAutoStringRef t_string;
	if (MCStringMutableCopy(p_string, &t_string) &&
		MCStringLowercase(*t_string, kMCSystemLocale) &&
		MCStringCopy(*t_string, r_lower))
		return;

	ctxt.Throw();
}

void MCStringsEvalToUpper(MCExecContext& ctxt, MCStringRef p_string, MCStringRef& r_upper)
{
	MCAutoStringRef t_string;
	if (MCStringMutableCopy(p_string, &t_string) &&
		MCStringUppercase(*t_string, kMCSystemLocale) &&
		MCStringCopy(*t_string, r_upper))
		return;

	ctxt.Throw();
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

void MCStringsEvalCharToNum(MCExecContext& ctxt, MCValueRef p_character, uinteger_t& r_codepoint)
{
	// This function has to be backwards compatible and do the broken stuff...
    MCAutoDataRef t_data;
    ctxt.ConvertToData(p_character, &t_data);
    // In case of empty string as input, the result must be 0
    if (MCDataIsEmpty(*t_data))
    {
        r_codepoint = 0;
        return;
    }
    else if (ctxt.GetUseUnicode())
    {
        if (MCDataGetLength(*t_data) >= 2)
        {
            const uint16_t *t_val;
            t_val = (const uint16_t*)MCDataGetBytePtr(*t_data);
            r_codepoint = *t_val;
            return;
        }
    }
    else
    {
        r_codepoint = MCDataGetByteAtIndex(*t_data, 0);
        return;
    }
    
    r_codepoint = ~0;
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

void MCStringsEvalNumToByte(MCExecContext& ctxt, integer_t p_byte, MCStringRef& r_byte)
{
	char_t t_byte_as_char;
	t_byte_as_char = (char_t)p_byte;
	if (MCStringCreateWithNativeChars(&t_byte_as_char, 1, r_byte))
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
    t_cleaned.New(MCStringGetLength(p_encoding) + 1);   // Cannot exceed incoming length
    
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
    r_match = 0 != MCR_exec(t_compiled, p_string);
    uindex_t t_match_index = 1;
    
    for (uindex_t i = 0; t_success && i < p_result_count; i++)
    {
        if (r_match && t_compiled->matchinfo[t_match_index].rm_so != -1)
        {
            uindex_t t_start, t_length;
            t_start = t_compiled->matchinfo[t_match_index].rm_so;
            t_length = t_compiled->matchinfo[t_match_index].rm_eo - t_start;
            t_success = MCStringCopySubstring(p_string, MCRangeMake(t_start, t_length), r_results[i]);
            
            if (++t_match_index >= NSUBEXP)
                t_match_index = 0;
        }
        else
            r_results[i] = MCValueRetain(kMCEmptyString);
    }
    
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
    r_match = 0 != MCR_exec(t_compiled, p_string);
    uindex_t t_match_index = 1;
    
    for (uindex_t i = 0; t_success && i + 1 < p_result_count; i += 2)
    {
        if (r_match && t_compiled->matchinfo[t_match_index].rm_so != -1)
        {
            t_success = MCStringFormat(r_results[i], "%d", t_compiled->matchinfo[t_match_index].rm_so + 1);
            if (t_success)
                t_success = MCStringFormat(r_results[i + 1], "%d", t_compiled->matchinfo[t_match_index].rm_eo);
            
            if (++t_match_index >= NSUBEXP)
                t_match_index = 0;
        }
        else
        {
            r_results[i] = MCValueRetain(kMCEmptyString);
            r_results[i + 1] = MCValueRetain(kMCEmptyString);
        }
    }
    
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

    bool t_success = true;
    
    MCAutoStringRef t_result;
    t_success = MCStringCreateMutable(0, &t_result);
    
    uindex_t t_source_length = MCStringGetLength(p_string);
    uindex_t t_source_offset = 0;
	MCStringRef t_substring;
	t_substring = nil;
    
    while (t_success && t_source_offset < t_source_length && MCStringCopySubstring(p_string, MCRangeMake(t_source_offset, MCStringGetLength(p_string) - (t_source_offset)), t_substring) && MCR_exec(t_compiled, t_substring))
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

		if (t_substring != nil)
		{
			MCValueRelease(t_substring);
			t_substring = nil;
		}

        
        if (MCStringGetCharAtIndex(p_pattern, 0) == '^')
            break;
    }
    
    MCAutoStringRef t_post_match;
    if (t_success)
        t_success = MCStringCopySubstring(p_string, MCRangeMake(t_source_offset, t_source_length - t_source_offset), &t_post_match) &&
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

    const unichar_t *t_pos = *t_wide_string;
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
                    format += 2;
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
            char_t* t_native_format;
            uindex_t t_cformat_length;
            char_t* t_cstart;

            /* UNCHECKED */ MCStringCreateWithChars(t_unicode_start, format - t_start, &t_substring);
            /* UNCHECKED */ t_auto_native . Lock(*t_substring, t_native_format, t_cformat_length);
            t_cstart = t_native_format;

            char newFormat[40];
            char *dptr = newFormat;

			integer_t width = 0;
			uinteger_t precision = 0;

			bool useShort = false;
			uinteger_t whichValue = PTR_VALUE;
			const char *end;

            *dptr++ = *t_native_format++;
            while (*t_native_format == '-' || *t_native_format == '#' || *t_native_format == '0'
                || *t_native_format == ' ' || *t_native_format == '+')
                *dptr++ = *t_native_format++;
            if (isdigit((uint1)*t_native_format))
			{
                width = strtol((const char*)t_native_format, (char **)&end, 10);
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

                MCAutoStringRefAsCString t_cstring;
                t_success = t_cstring . Lock(*t_string);
                if (t_success)
                    t_success = MCStringAppendFormat(*t_result, newFormat, *t_cstring);
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
					if (ctxt . GetHandler() != nil)
						ctxt.GetHandler()->eval(t_ctxt, *t_expression, &t_value);
					else
						ctxt.GetHandlerList()->eval(t_ctxt, *t_expression, &t_value);
				}
				else
				{
					if (ctxt . GetHandler() != nil)
						ctxt.GetHandler()->doscript(t_ctxt, *t_expression);
					else
						ctxt.GetHandlerList()->doscript(t_ctxt, *t_expression);
					t_value = MCresult->getvalueref();
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
	MCAutoStringRef t_string;
	return MCStringMutableCopy(p_left, &t_string) &&
		MCStringAppend(*t_string, p_right) &&
		MCStringCopy(*t_string, r_result);
}

bool MCStringsConcatenateWithChar(MCStringRef p_left, MCStringRef p_right, unichar_t p_char, MCStringRef& r_result)
{
	MCAutoStringRef t_string;
	return MCStringMutableCopy(p_left, &t_string) &&
		MCStringAppendChar(*t_string, p_char) &&
		MCStringAppend(*t_string, p_right) &&
		MCStringCopy(*t_string, r_result);
}

void MCStringsEvalConcatenate(MCExecContext& ctxt, MCStringRef p_left, MCStringRef p_right, MCStringRef& r_result)
{
	if (MCStringsConcatenate(p_left, p_right, r_result))
		return;

	ctxt.Throw();
}

void MCStringsEvalConcatenateWithSpace(MCExecContext& ctxt, MCStringRef p_left, MCStringRef p_right, MCStringRef& r_result)
{
	if (MCStringsConcatenateWithChar(p_left, p_right, ' ', r_result))
		return;

	ctxt.Throw();
}

void MCStringsEvalConcatenateWithComma(MCExecContext& ctxt, MCStringRef p_left, MCStringRef p_right, MCStringRef& r_result)
{
	if (MCStringsConcatenateWithChar(p_left, p_right, ',', r_result))
		return;

	ctxt.Throw();
}

////////////////////////////////////////////////////////////////////////////////

void MCStringsEvalContains(MCExecContext& ctxt, MCStringRef p_whole, MCStringRef p_part, bool& r_result)
{
	MCStringOptions t_compare_option = ctxt.GetStringComparisonType();
	r_result = MCStringContains(p_whole, p_part, t_compare_option);
}

void MCStringsEvalDoesNotContain(MCExecContext& ctxt, MCStringRef p_whole, MCStringRef p_part, bool& r_result)
{
	MCStringOptions t_compare_option = ctxt.GetStringComparisonType();
	r_result = !MCStringContains(p_whole, p_part, t_compare_option);
}

void MCStringsEvalBeginsWith(MCExecContext& ctxt, MCStringRef p_whole, MCStringRef p_part, bool& r_result)
{
	MCStringOptions t_compare_option = ctxt.GetStringComparisonType();
	r_result = MCStringBeginsWith(p_whole, p_part, t_compare_option);
}

void MCStringsEvalEndsWith(MCExecContext& ctxt, MCStringRef p_whole, MCStringRef p_part, bool& r_result)
{
    MCStringOptions t_compare_option = ctxt.GetStringComparisonType();
    r_result = MCStringEndsWith(p_whole, p_part, t_compare_option);
}

////////////////////////////////////////////////////////////////////////////////

bool MCStringsEvalIsAmongTheChunksOf(MCExecContext& ctxt, MCStringRef p_chunk, MCStringRef p_text, Chunk_term p_chunk_type)
{
    MCTextChunkIterator *tci;
    tci = new MCTextChunkIterator(p_chunk_type, p_text);
    bool t_result;
    t_result = tci -> isamong(ctxt, p_chunk);
    delete tci;
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
    uindex_t t_byte_count = MCDataGetLength(p_string);
    uindex_t t_chunk_byte_count = MCDataGetLength(p_chunk);
    
    const byte_t *t_bytes = MCDataGetBytePtr(p_string);
    const byte_t *t_chunk_bytes = MCDataGetBytePtr(p_chunk);
    
    bool t_found = false;
    for (uindex_t i = 0; i < t_byte_count - t_chunk_byte_count + 1; i++)
        if (MCMemoryCompare(t_bytes++, t_chunk_bytes, sizeof(byte_t) * t_chunk_byte_count) == 0)
        {
            t_found = true;
            break;
        }
    
    r_result = t_found;
}

void MCStringsEvalIsNotAmongTheBytesOf(MCExecContext& ctxt, MCDataRef p_chunk, MCDataRef p_string, bool& r_result)
{
    MCStringsEvalIsAmongTheBytesOf(ctxt, p_chunk, p_string, r_result);
	r_result = !r_result;
}

////////////////////////////////////////////////////////////////////////////////

uindex_t MCStringsChunkOffset(MCExecContext& ctxt, MCStringRef p_chunk, MCStringRef p_string, uindex_t p_start_offset, Chunk_term p_chunk_type)
{
    MCTextChunkIterator *tci;
    tci = new MCTextChunkIterator(p_chunk_type, p_string);
    uindex_t t_offset = tci -> chunkoffset(ctxt, p_chunk, p_start_offset);
    delete tci;
    return t_offset;
}

void MCStringsEvalLineOffset(MCExecContext& ctxt, MCStringRef p_chunk, MCStringRef p_string, uindex_t p_start_offset, uindex_t& r_result)
{
    r_result = MCStringsChunkOffset(ctxt, p_chunk, p_string, p_start_offset, CT_LINE);
}

void MCStringsEvalParagraphOffset(MCExecContext& ctxt, MCStringRef p_chunk, MCStringRef p_string, uindex_t p_start_offset, uindex_t& r_result)
{
    r_result = MCStringsChunkOffset(ctxt, p_chunk, p_string, p_start_offset, CT_PARAGRAPH);
}

void MCStringsEvalSentenceOffset(MCExecContext& ctxt, MCStringRef p_chunk, MCStringRef p_string, uindex_t p_start_offset, uindex_t& r_result)
{
    r_result = MCStringsChunkOffset(ctxt, p_chunk, p_string, p_start_offset, CT_SENTENCE);
}

void MCStringsEvalItemOffset(MCExecContext& ctxt, MCStringRef p_chunk, MCStringRef p_string, uindex_t p_start_offset, uindex_t& r_result)
{
    r_result = MCStringsChunkOffset(ctxt, p_chunk, p_string, p_start_offset, CT_ITEM);
}

void MCStringsEvalWordOffset(MCExecContext& ctxt, MCStringRef p_chunk, MCStringRef p_string, uindex_t p_start_offset, uindex_t& r_result)
{
    r_result = MCStringsChunkOffset(ctxt, p_chunk, p_string, p_start_offset, CT_WORD);
}

void MCStringsEvalTrueWordOffset(MCExecContext& ctxt, MCStringRef p_chunk, MCStringRef p_string, uindex_t p_start_offset, uindex_t& r_result)
{
    r_result = MCStringsChunkOffset(ctxt, p_chunk, p_string, p_start_offset, CT_TRUEWORD);
}

void MCStringsEvalTokenOffset(MCExecContext& ctxt, MCStringRef p_chunk, MCStringRef p_string, uindex_t p_start_offset, uindex_t& r_result)
{
    r_result = MCStringsChunkOffset(ctxt, p_chunk, p_string, p_start_offset, CT_TOKEN);
}

void MCStringsEvalCodepointOffset(MCExecContext& ctxt, MCStringRef p_chunk, MCStringRef p_string, uindex_t p_start_offset, uindex_t& r_result)
{
    r_result = MCStringsChunkOffset(ctxt, p_chunk, p_string, p_start_offset, CT_CODEPOINT);
}

void MCStringsEvalCodeunitOffset(MCExecContext& ctxt, MCStringRef p_chunk, MCStringRef p_string, uindex_t p_start_offset, uindex_t& r_result)
{
    r_result = MCStringsChunkOffset(ctxt, p_chunk, p_string, p_start_offset, CT_CODEUNIT);
}

void MCStringsEvalByteOffset(MCExecContext& ctxt, MCDataRef p_chunk, MCDataRef p_string, uindex_t p_start_offset, uindex_t& r_result)
{
    uindex_t t_byte_count = MCDataGetLength(p_string);
    uindex_t t_chunk_byte_count = MCDataGetLength(p_chunk);
    
    const byte_t *t_bytes = MCDataGetBytePtr(p_string);
    const byte_t *t_chunk_bytes = MCDataGetBytePtr(p_chunk);
    
    uindex_t t_offset;
    r_result = 0;
    
    for (t_offset = p_start_offset; t_offset < t_byte_count - t_chunk_byte_count + 1; t_offset++)
        if (MCMemoryCompare(t_bytes + t_offset, t_chunk_bytes, sizeof(byte_t) * t_chunk_byte_count) == 0)
        {
            r_result = t_offset - p_start_offset + 1;
            break;
        }
}

void MCStringsEvalOffset(MCExecContext& ctxt, MCStringRef p_chunk, MCStringRef p_string, uindex_t p_start_offset, uindex_t& r_result)
{
	MCStringOptions t_options = ctxt.GetStringComparisonType();
    uindex_t t_offset;
	if (!MCStringFirstIndexOf(p_string, p_chunk, p_start_offset, t_options, t_offset))
		r_result = 0;
	else
    {
        // We want to get the grapheme length, not the codeunit one
        MCRange t_cu_range, t_char_range;
        t_cu_range . offset = p_start_offset;
        t_cu_range . length = t_offset - p_start_offset;
        MCStringUnmapIndices(p_string, kMCCharChunkTypeGrapheme, t_cu_range, t_char_range);
        
		r_result = t_char_range . offset + t_char_range . length + 1;
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

// JS-2013-07-01: [[ EnhancedFilter ]] Implementation of pattern matching classes.
bool MCRegexMatcher::compile(MCStringRef& r_error)
{
	// MW-2013-07-01: [[ EnhancedFilter ]] Removed 'usecache' parameter as there's
	//   no reason not to use the cache.
	compiled = MCR_compile(pattern, casesensitive);
	if (compiled == nil)
	{
        MCR_copyerror(r_error);
		return false;
	}
    return true;
}

bool MCRegexMatcher::match(MCRange p_range)
{
    MCAutoStringRef t_string;
    MCStringCopySubstring(source, p_range, &t_string);
    
	return MCR_exec(compiled, *t_string);
}

bool MCWildcardMatcher::compile(MCStringRef& r_error)
{
    // wildcard patterns are not compiled
    return true;
}

#define OPEN_BRACKET '['
#define CLOSE_BRACKET ']'

/*
bool MCStringsWildcardMatch(const char *s, uindex_t s_length, const char *p, uindex_t p_length, bool casesensitive)
{
	uindex_t s_index = 0;
	uindex_t p_index = 0;
	uint1 scc, c;

	while (s_index < s_length)
	{
		scc = *s++;
		s_index++;
		c = *p++;
		p_index++;
		switch (c)
		{
		case OPEN_BRACKET:
			{
				bool ok = false;
				int lc = -1;
				int notflag = 0;

				if (*p == '!' )
				{
					notflag = 1;
					p++;
					p_index++;
				}
				while (p_index < p_length)
				{
					c = *p++;
					p_index++;
					if (c == CLOSE_BRACKET && lc >= 0)
						return ok ? MCStringsWildcardMatch(s, s_length - s_index, p, p_length - p_index, casesensitive) : false;
					else
						if (c == '-' && lc >= 0 && *p != CLOSE_BRACKET)
						{
							c = *p++;
							p_index++;
							if (notflag)
							{
								if (lc > scc || scc > c)
									ok = true;
								else
									return false;
							}
							else
							{
								if (lc < scc && scc <= c)
									ok = true;
							}
						}
						else
						{
							if (notflag)
							{
								if (scc != c)
									ok = true;
								else
									return false;
							}
							else
								if (scc == c)
									ok = true;
							lc = c;
						}
				}
			}
			return false;
		case '?':
			break;
		case '*':
			while (*p == '*')
			{
				p++;
				p_index++;
			}
			if (*p == 0)
				return true;
			--s;
			--s_index;
			c = *p;
			while (*s)
				if ((casesensitive ? c != *s : MCS_tolower(c) != MCS_tolower(*s))
				        && *p != '?' && *p != OPEN_BRACKET)
				{
					s++;
					s_index++;
				}
				else
					if (MCStringsWildcardMatch(s++, s_length - s_index++, p, p_length - p_index, casesensitive))
						return true;
			return false;
		case 0:
			return scc == 0;
		default:
			if (casesensitive)
			{
				if (c != scc)
					return false;
			}
			else
				if (MCS_tolower(c) != MCS_tolower(scc))
					return false;
			break;
		}
	}
	while (p_index < p_length && *p == '*')
	{
		p++;
		p_index++;
	}
	return p_index == p_length;
}

index_t MCStringsWildcardCompareChar(MCStringRef p_input, uindex_t p_string_cu_offset, MCStringRef p_pattern, uindex_t p_pattern_cu_offset, MCUnicodeCompareOption p_option, uindex_t &r_string_char_cu, uindex_t &r_pattern_char_cu)
{
    // Comparison of the characters
    MCRange t_string_cu_range;
    MCStringUnmapIndices(p_input, kMCCharChunkTypeGrapheme, MCRangeMake(p_string_cu_offset, 1), t_string_cu_range);
    
    MCRange t_pattern_cu_range;
    MCStringUnmapIndices(p_pattern, kMCCharChunkTypeGrapheme, MCRangeMake(p_pattern_cu_offset, 1), t_pattern_cu_range);
    
    r_string_char_cu = t_string_cu_range . length;
    r_pattern_char_cu = t_pattern_cu_range . length;
    
    return MCUnicodeCompare(MCStringGetCharPtr(p_input) + t_string_cu_range . offset,
                            t_string_cu_range . length,
                            MCStringGetCharPtr(p_pattern) + t_pattern_cu_range . offset,
                            t_pattern_cu_range . length,
                            p_option);
}

bool MCStringsExecWildcardMatch(MCStringRef p_string, uindex_t p_string_offset, MCStringRef p_pattern, uindex_t p_pattern_offset, bool casesensitive)
{
	uindex_t s_index = p_string_offset;
	uindex_t p_index = p_pattern_offset;
    
    uindex_t s_length = MCStringGetLength(p_string);
    uindex_t p_length = MCStringGetLength(p_pattern);
    
    uindex_t t_string_move;
    uindex_t t_pattern_move;
    
    MCUnicodeCompareOption t_comparison = casesensitive ? kMCUnicodeCompareOptionNormalised : kMCUnicodeCompareOptionCaseless;
    
	while (s_index < s_length)
	{        
		switch (MCStringGetCharAtIndex(p_pattern, p_index))
		{
            case OPEN_BRACKET:
			{
				bool ok = false;
                
                uindex_t t_last_char_offset = 0;
                unichar_t t_pattern_char;
                
				int notflag = 0;
                ++p_index;
                
				if (MCStringGetCharAtIndex(p_pattern, p_index) == '!' )
				{
					notflag = 1;
					p_index++;
				}
				while (p_index < p_length)
				{                    
					if (MCStringGetCharAtIndex(p_pattern, p_index) == CLOSE_BRACKET && t_last_char_offset != 0)
						return ok ? MCStringsExecWildcardMatch(p_string, ++s_index, p_pattern, ++p_index, casesensitive) : false;
					else
                    {
						if (MCStringGetCharAtIndex(p_pattern, p_index) == '-' && t_last_char_offset != 0 && MCStringGetCharAtIndex(p_pattern, ++p_index) != CLOSE_BRACKET)
                        {
							if (notflag)
							{
								if (MCStringsWildcardCompareChar(p_string, s_index, p_pattern, t_last_char_offset, t_comparison, t_string_move, t_pattern_move) < 0
                                        || MCStringsWildcardCompareChar(p_string, s_index, p_pattern, p_index, t_comparison, t_string_move, t_pattern_move) > 0)
									ok = true;
								else
									return false;
							}
							else
							{
								if (MCStringsWildcardCompareChar(p_string, s_index, p_pattern, t_last_char_offset, t_comparison, t_string_move, t_pattern_move) > 0
                                        && MCStringsWildcardCompareChar(p_string, s_index, p_pattern, p_index, t_comparison, t_string_move, t_pattern_move) <= 0)
									ok = true;
							}
						}
						else
						{
							if (notflag)
							{
								if (MCStringsWildcardCompareChar(p_string, s_index, p_pattern, p_index, t_comparison, t_string_move, t_pattern_move) != 0)
                                    ok = true;
								else
									return false;
							}
							else
                                if (MCStringsWildcardCompareChar(p_string, s_index, p_pattern, p_index, t_comparison, t_string_move, t_pattern_move) == 0)
									ok = true;
                            
                            t_last_char_offset = p_index;
                            p_index += t_pattern_move;
						}
                    }
				}
			}
                return false;
            case '?':
            {
                // get the offsets to jump over the next pattern and string chars
                MCStringsWildcardCompareChar(p_string, s_index, p_pattern, p_index, t_comparison, t_string_move, t_pattern_move);
                s_index += t_string_move;
                p_index += t_pattern_move;
                break;
            }
            case '*':
            {
                while (p_index != p_length && MCStringGetCharAtIndex(p_pattern, p_index) == '*')
                    p_index++;
                
                if (p_index == p_length)
                    return true;
                
                while (s_index != s_length)
                {
                    if ((MCStringsWildcardCompareChar(p_string, s_index, p_pattern, p_index, t_comparison, t_string_move, t_pattern_move) == 0
                            || MCStringGetCharAtIndex(p_pattern, p_index) == '?'
                            || MCStringGetCharAtIndex(p_pattern, p_index) == OPEN_BRACKET)
                            && MCStringsExecWildcardMatch(p_string, s_index, p_pattern, p_index, casesensitive))
                        return true;
                    
                    s_index += t_string_move;
                }
            }
                return false;
            case 0:
                return MCStringGetCharAtIndex(p_string, s_index) == 0;
            default:
                if (MCStringsWildcardCompareChar(p_string, s_index, p_pattern, p_index, t_comparison, t_string_move, t_pattern_move) == 0)
                {
                    s_index += t_string_move;
                    p_index += t_pattern_move;
                }
                else
                    return false;
                
                break;
		}
	}
	while (p_index < p_length && MCStringGetCharAtIndex(p_pattern, p_index) == '*')
		p_index++;
    
	return p_index == p_length;
}
*/
bool MCStringsExecWildcardMatch(MCStringRef p_string, MCBreakIteratorRef p_siter, MCRange p_srange, MCStringRef p_pattern, MCBreakIteratorRef p_piter, uindex_t p_pattern_offset, bool casesensitive)
{
    // Break iterators locate the grapheme boundaries. Whenever we need to compare chars,
    // we record the index, advance the iterator, and compare the intervening codeunits.
	uindex_t s_index = p_srange . offset;
	uindex_t p_index = p_pattern_offset;
    
    uindex_t s_end = s_index + p_srange . length;
    uindex_t p_end = MCStringGetLength(p_pattern);
    
    MCUnicodeCompareOption t_comparison = casesensitive ? kMCUnicodeCompareOptionNormalised : kMCUnicodeCompareOptionCaseless;
    
    const unichar_t *sptr = MCStringGetCharPtr(p_string);
    const unichar_t *pptr = MCStringGetCharPtr(p_pattern);
    
    // Codeunit ranges for the target source char pattern char, and previous pattern char
    // (for expressions like [a-z])
    MCRange t_srange, t_prange, t_lprange;
    
	while (s_index < s_end)
	{
        // set the source grapheme range
        t_srange . offset = s_index;
        s_index = MCLocaleBreakIteratorAfter(p_siter, s_index);
        t_srange . length = s_index - t_srange . offset;
        
		switch (MCStringGetCharAtIndex(p_pattern, p_index))
		{
            case OPEN_BRACKET:
			{
                // Records whether we currently have a match for this bracket.
				bool ok = false;
                
                t_lprange . offset = 0;
                
				int notflag = 0;
                p_index++;
                
				if (MCStringGetCharAtIndex(p_pattern, p_index) == '!' )
				{
					notflag = 1;
                    p_index++;
				}
				while (p_index < p_end)
				{
					if (MCStringGetCharAtIndex(p_pattern, p_index) == CLOSE_BRACKET && t_lprange . offset != 0)
                    {
                        // if the bracket was close with no match found then return false;
                        if (!ok)
                            return false;
                        
                        // otherwise, recurse.
                        p_srange . offset++;
                        p_srange . length--;
						return MCStringsExecWildcardMatch(p_string, p_siter, p_srange, p_pattern, p_piter, ++p_index, casesensitive);
                    }
					else
                    {
						if (MCStringGetCharAtIndex(p_pattern, p_index) == '-' && t_lprange . offset != 0 && MCStringGetCharAtIndex(p_pattern, p_index + 1) != CLOSE_BRACKET)
                        {
                            // We have a char range (eg [a-z]), so skip past the '-',
                            // find the current pattern grapheme range and compare.
                            t_prange . offset = ++p_index;
                            p_index = MCLocaleBreakIteratorAfter(p_piter, p_index);
                            t_prange . length = p_index - t_prange . offset;
                            
							if (notflag)
							{
                                // wer're still ok if the current source grapheme falls outwith the appropriate range. Otherwise, we fail.
								if (MCUnicodeCompare(sptr + t_srange . offset, t_srange . length, pptr + t_lprange . offset, t_lprange . length, t_comparison) < 0
                                    || MCUnicodeCompare(sptr + t_srange . offset, t_srange . length, pptr + t_prange . offset, t_prange . length, t_comparison) > 0)
									ok = true;
								else
									return false;
							}
							else
							{
                                 // we're still ok if the current source grapheme falls within the appropriate range.
                                 // If not, there may be other options within this pair of brackets
								if (MCUnicodeCompare(sptr + t_srange . offset, t_srange . length, pptr + t_lprange . offset, t_lprange . length, t_comparison) > 0
                                    && MCUnicodeCompare(sptr + t_srange . offset, t_srange . length, pptr + t_prange . offset, t_prange . length, t_comparison) <= 0)
									ok = true;
							}
						}
						else
						{
                            // This could be one of a choice of characters (eg [abc]).
                            t_prange . offset = p_index;
                            p_index = MCLocaleBreakIteratorAfter(p_piter, p_index);
                            t_prange . length = p_index - t_prange . offset;
							if (notflag)
							{
								if (MCUnicodeCompare(sptr + t_srange . offset, t_srange . length, pptr + t_prange . offset, t_prange . length, t_comparison) != 0)
                                    ok = true;
								else
									return false;
							}
							else
                                if (MCUnicodeCompare(sptr + t_srange . offset, t_srange . length, pptr + t_prange . offset, t_prange . length, t_comparison) == 0)
									ok = true;
                            
                            // record the grapheme in case it is the first character of a range.
                            t_lprange . offset = t_prange . offset;
                            t_lprange . length = t_prange . length;
						}
                    }
				}
			}
                return false;
            case '?':
            {
                // Matches any character, so increment the pattern index.
                p_index++;
                break;
            }
            case '*':
            {
                // consume any more * characters.
                while (++p_index < p_end && MCStringGetCharAtIndex(p_pattern, p_index) == '*');
                
                if (p_index == p_end)
                    return true;
                
                // Get the range of the next pattern grapheme
                t_prange . offset = p_index;
                p_index = MCLocaleBreakIteratorAfter(p_piter, p_index);
                t_prange . length = p_index - t_prange . offset;
                
                // try and match the rest of the source string recursively.
                while (t_srange . offset < s_end)
                {
                    // if this is a candidate for a match, recurse.
                    if (MCUnicodeCompare(sptr + t_srange . offset, t_srange . length, pptr + t_prange . offset, t_prange . length, t_comparison) == 0)
                    {
                        p_srange . length = p_srange . length - s_index + p_srange . offset;
                        p_srange . offset = s_index;
                        if (MCStringsExecWildcardMatch(p_string, p_siter, p_srange, p_pattern, p_piter, p_index, casesensitive))
                            return true;
                    }
                    else if (MCStringGetCharAtIndex(p_pattern, t_prange . offset) == '?' || MCStringGetCharAtIndex(p_pattern, t_prange . offset) == OPEN_BRACKET)
                    {
                        p_srange . length = p_srange . length - t_srange . offset + p_srange . offset;
                        p_srange . offset = t_srange . offset;
                        if (MCStringsExecWildcardMatch(p_string, p_siter, p_srange, p_pattern, p_piter, t_prange . offset, casesensitive))
                            return true;
                    }
                    
                    // Otherwise eat the char
                    t_srange . offset = s_index;
                    s_index = MCLocaleBreakIteratorAfter(p_siter, s_index);
                    t_srange . length = s_index - t_srange . offset;
                }
            }
                return false;
            case 0:
                return MCStringGetCharAtIndex(p_string, t_srange. offset) == 0;
            default:
                // default - just compare chars
                t_prange . offset = p_index;
                p_index = MCLocaleBreakIteratorAfter(p_piter, p_index);
                t_prange . length = p_index - t_prange . offset;
                if (MCUnicodeCompare(sptr + t_srange . offset, t_srange . length, pptr + t_prange . offset, t_prange . length, t_comparison) != 0)
                    return false;
                
                break;
		}
	}
    // Eat any remaining '*'s
	while (p_index < p_end && MCStringGetCharAtIndex(p_pattern, p_index) == '*')
		p_index++;
    
	return p_index == p_end;
}

bool MCWildcardMatcher::match(MCRange p_source_range)
{
	return MCStringsExecWildcardMatch(source, source_iter, p_source_range, pattern, pattern_iter, 0, casesensitive);
}

void MCStringsExecFilterDelimited(MCExecContext& ctxt, MCStringRef p_source, bool p_without, char_t p_delimiter, MCPatternMatcher *p_matcher, MCStringRef &r_result)
{
	uint32_t t_length = MCStringGetLength(p_source);
	if (t_length == 0)
        MCStringCopy(kMCEmptyString, r_result);

	uint4 offset = 0;
    MCAutoListRef t_output;
    MCListCreateMutable(p_delimiter, &t_output);

	// OK-2010-01-11: Bug 7649 - Filter command was incorrectly removing empty lines.
	// Now ignores delimiter for matching but includes it in the append.

	uindex_t t_return_offset = 0;
    uindex_t t_last_offset = 0;
	bool t_found = true;
    bool t_success = true;
    
    MCRange t_chunk_range;
    
	while (t_found && t_success)
	{
        MCAutoStringRef t_line;
		t_found = MCStringFirstIndexOfChar(p_source, p_delimiter, t_last_offset, kMCCompareExact, t_return_offset);
		if (!t_found) //last line or item
        {
            t_chunk_range . offset = t_last_offset;
            t_chunk_range . length = t_length - t_last_offset;
        }
		else
        {
            t_chunk_range . offset = t_last_offset;
            t_chunk_range . length = t_return_offset - t_last_offset;
        }
        
        if (t_success && (p_matcher -> match(t_chunk_range) != p_without))
        {
            MCAutoStringRef t_line;
            t_success = MCStringCopySubstring(p_source, t_chunk_range, &t_line) && MCListAppend(*t_output, *t_line);
        }

		t_last_offset = t_return_offset + 1;
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
	MCPatternMatcher *matcher;
    matcher = new MCWildcardMatcher(p_pattern, p_source, ctxt . GetCaseSensitive(), ctxt . GetFormSensitive());
    
    MCStringsExecFilterDelimited(ctxt, p_source, p_without, p_lines ? ctxt . GetLineDelimiter() : ctxt . GetItemDelimiter(), matcher, r_result);
    
    delete matcher;
}

void MCStringsExecFilterRegex(MCExecContext& ctxt, MCStringRef p_source, MCStringRef p_pattern, bool p_without, bool p_lines, MCStringRef &r_result)
{
	// Create the pattern matcher
	MCPatternMatcher *matcher;
    matcher = new MCRegexMatcher(p_pattern, p_source, ctxt . GetCaseSensitive(), ctxt . GetFormSensitive());
    
    MCAutoStringRef t_regex_error;
    if (!matcher -> compile(&t_regex_error))
    {
        delete matcher;
        ctxt . LegacyThrow(EE_MATCH_BADPATTERN);
        return;
    }
    
    MCStringsExecFilterDelimited(ctxt, p_source, p_without, p_lines ? ctxt . GetLineDelimiter() : ctxt . GetItemDelimiter(), matcher, r_result);
    
    delete matcher;
}

void MCStringsExecFilterWildcardIntoIt(MCExecContext& ctxt, MCStringRef p_source, MCStringRef p_pattern, bool p_without, bool p_lines)
{
    MCAutoStringRef t_result;
    MCStringsExecFilterWildcard(ctxt, p_source, p_pattern, p_without, p_lines, &t_result);
    
    if (*t_result != nil)
        ctxt . SetItToValue(*t_result);
    else
        ctxt . SetItToEmpty();
}

void MCStringsExecFilterRegexIntoIt(MCExecContext& ctxt, MCStringRef p_source, MCStringRef p_pattern, bool p_without, bool p_lines)
{
    MCAutoStringRef t_result;
    MCStringsExecFilterRegex(ctxt, p_source, p_pattern, p_without, p_lines, &t_result);

    if (*t_result != nil)
        ctxt . SetItToValue(*t_result);
    else
        ctxt . SetItToEmpty();
}

////////////////////////////////////////////////////////////////////////////////

void MCStringsEvalIsAscii(MCExecContext& ctxt, MCValueRef p_value, bool& r_result)
{
    MCAutoStringRef t_string;
	if (!ctxt . ConvertToString(p_value, &t_string))
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
