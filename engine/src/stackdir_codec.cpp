/*                                                                   -*- c++ -*-
Copyright (C) 2003-2014 Runtime Revolution Ltd.

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

#include "foundation-text.h"

#include "util.h"

#include "stackdir.h"

#include "stackdir_private.h"

#include <unicode/uchar.h>

/* This file contains code relating to encoding and decoding the
 * values stored in Expanded Livecode Stackfiles.
 */

/* ================================================================
 * File-local declarations
 * ================================================================ */

struct _MCStackdirIOScanner
{
	/* Filter for iterating over string */
	MCTextFilter *m_filter;

	/* Current logical file position */
	uindex_t m_line;
	uindex_t m_column;

	/* Contains the next token */
	MCStackdirIOToken m_token;

	/* True if the next token has already been peeked. */
	bool m_has_peek;
};

/* Normalize and case-fold a string (Normalized Form C, Simple
 * Case-Fold). */
static bool MCStackdirNormalizeString (MCStringRef p_string, MCStringRef & r_normalized);

/* Test whether a character is a hexadecimal digit */
static bool MCStackdirIsHexDigit (codepoint_t p_char);

static bool MCStackdirFormatBoolean (MCBooleanRef p_bool, MCStringRef & r_literal);
static bool MCStackdirFormatInteger (MCNumberRef p_number, MCStringRef & r_literal);
static bool MCStackdirFormatReal (MCNumberRef p_number, MCStringRef & r_literal);
static bool MCStackdirFormatNumber (MCNumberRef p_number, MCStringRef & r_literal);
static bool MCStackdirFormatData (MCDataRef p_data, MCStringRef & r_literal);
static bool MCStackdirFormatString (MCStringRef p_string, MCStringRef & r_literal);
static bool MCStackdirFormatName (MCNameRef p_name, MCStringRef & r_literal);

/* Reset the token, freeing any currently-held resources */
static void MCStackdirIOScannerResetToken (MCStackdirIOScannerRef scanner, MCStackdirIOToken & x_token);

/* Test whether the token is unset */
static inline bool MCStackdirIOTokenIsNone (MCStackdirIOToken & p_token);

/* Test whether the token indicates an error */
static inline bool MCStackdirIOTokenIsError (MCStackdirIOToken & p_token);

/* Obtain the current character in the scanner. Returns false if there
 * is no valid current character. */
static inline bool MCStackdirIOScannerGetChar (MCStackdirIOScannerRef scanner, codepoint_t & r_char);

/* Move to the next character in the scanner. Returns true if there is
 * another character available. */
static inline bool MCStackdirIOScannerNextChar (MCStackdirIOScannerRef scanner);

static bool MCStackdirIOScanEOF (MCStackdirIOScannerRef scanner, MCStackdirIOToken & r_token);
static bool MCStackdirIOScanNewline (MCStackdirIOScannerRef scanner, MCStackdirIOToken & r_token);
static bool MCStackdirIOScanSpace (MCStackdirIOScannerRef scanner, MCStackdirIOToken & r_token);
static bool MCStackdirIOScanStorageSeparator (MCStackdirIOScannerRef scanner, MCStackdirIOToken & r_token);
static bool MCStackdirIOScanExternalIndicator (MCStackdirIOScannerRef scanner, MCStackdirIOToken & r_token);
static bool MCStackdirIOScanNumber (MCStackdirIOScannerRef scanner, MCStackdirIOToken & r_token);
static bool MCStackdirIOScanUnquotedString (MCStackdirIOScannerRef scanner, MCStackdirIOToken & r_token);
static bool MCStackdirIOScanString (MCStackdirIOScannerRef scanner, MCStackdirIOToken & r_token);
static bool MCStackdirIOScanData (MCStackdirIOScannerRef scanner, MCStackdirIOToken & r_token);
static bool MCStackdirIOScanSharedHeader (MCStackdirIOScannerRef scanner, MCStackdirIOToken & r_token);
static bool MCStackdirIOScanFlag (MCStackdirIOScannerRef scanner, MCStackdirIOToken & r_token);

/* Table of scanner functions. These are called in order */
typedef bool (*MCStackdirIOScanFunc)(MCStackdirIOScannerRef, MCStackdirIOToken &);
static const MCStackdirIOScanFunc kMCStackdirIOScanFuncs[] =
	{
		MCStackdirIOScanEOF,
		MCStackdirIOScanNewline,
		MCStackdirIOScanSpace,
		MCStackdirIOScanStorageSeparator,
		MCStackdirIOScanExternalIndicator,
		MCStackdirIOScanFlag,
		MCStackdirIOScanString,
		MCStackdirIOScanData,
		MCStackdirIOScanNumber,
		MCStackdirIOScanSharedHeader,
		MCStackdirIOScanUnquotedString,
		NULL,
	};

/* ================================================================
 * Formatting
 * ================================================================ */

/* ----------------------------------------------------------------
 * Formatting utility functions
 * ---------------------------------------------------------------- */

static bool
MCStackdirNormalizeString (MCStringRef p_string, MCStringRef & r_normalized)
{
	MCAutoStringRef t_normalized, t_result;
	if (!(MCStringNormalizedCopyNFC (p_string, &t_normalized) &&
		  MCStringMutableCopy (*t_normalized, &t_result) &&
		  MCStringFold (*t_result, kMCStringOptionCompareCaseless)))
		return false;
	r_normalized = MCValueRetain (*t_result);
	return true;
}

/* ----------------------------------------------------------------
 * Boolean formatting
 * ---------------------------------------------------------------- */

static bool
MCStackdirFormatBoolean (MCBooleanRef p_bool, MCStringRef & r_literal)
{
	if (p_bool)
		return MCStringCopy (kMCStackdirTrueLiteral, r_literal);
	else
		return MCStringCopy (kMCStackdirFalseLiteral, r_literal);
}

/* ----------------------------------------------------------------
 * Number formatting
 * ---------------------------------------------------------------- */

static bool
MCStackdirFormatInteger (MCNumberRef p_number, MCStringRef & r_literal)
{
	return MCStringFormat (r_literal, "%+i",
						   MCNumberFetchAsInteger (p_number));
}

static bool
MCStackdirFormatReal (MCNumberRef p_number, MCStringRef & r_literal)
{
	MCAutoStringRef t_format, t_result;
	bool t_success;
	/* We can always accurately represent a double precision integer
	 * using 17 decimal digits, i.e. one digit to the left of the
	 * decimal point and 16 to the right.
	 *
	 * BUG This might not work in the future if MCStringFormat ever
	 * starts obeying the C locale.
	 */
	t_success = MCStringFormat (&t_format, "%+.16g",
								 MCNumberFetchAsReal (p_number));

	/* We need to ensure that the result always includes a '.' or an
	 * 'e' character so that it is correctly distinguished from an
	 * integer */
	bool t_found = false;
	if (t_success)
	{
		const char *t_cstring = MCStringGetCString (*t_format);
		for ( ; *t_cstring != 0; ++t_cstring)
		{
			if (*t_cstring == 'e' || *t_cstring == '.')
			{
				t_found = true;
				break;
			}
		}

		if (!t_found)
			t_success = MCStringFormat (&t_result, "%@.", *t_format);
		else
			t_success = MCStringCopy (*t_format, &t_result);
	}

	if (t_success)
		t_success = MCStringCopy (*t_result, r_literal);

	return t_success;
}

static bool
MCStackdirFormatNumber (MCNumberRef p_number, MCStringRef & r_literal)
{
	if (MCNumberIsInteger (p_number))
		return MCStackdirFormatInteger (p_number, r_literal);
	else
		return MCStackdirFormatReal (p_number, r_literal);
}

/* ----------------------------------------------------------------
 * Data formatting
 * ---------------------------------------------------------------- */

static bool
MCStackdirFormatData (MCDataRef p_data, MCStringRef & r_literal)
{
	/* Trivial case */
	uindex_t t_source_length = MCDataGetLength (p_data);
	if (t_source_length == 0)
	{
		r_literal = MCValueRetain (MCSTR ("<>"));
		return true;
	}

	bool t_success = true;

	/* We consider string encoding iff p_data contains valid UTF8.
	 * Unfortunately, MCStringDecode doesn't do validation, so test by
	 * doing a round-trip. */
	MCAutoStringRef t_string;
	MCAutoDataRef t_roundtrip_data;
	t_success = (MCStringDecode (p_data, kMCStringEncodingUTF8,
								 false, &t_string) &&
				 MCStringEncode (*t_string, kMCStringEncodingUTF8,
								 false, &t_roundtrip_data));

	bool t_valid_utf8 = (t_success &&
						 (MCDataCompareTo (p_data, *t_roundtrip_data) == 0));

	/* Reset t_success at this point. If any failure occurred up to
	 * here, assume it means that we can't use a string representation
	 * for the data. */
	t_success = true;

	/* Only use the string format if the encoded string is shorter
	 * than the corresponding base64 format. */
	/* base64 uses approximately 4 output bytes for every 3 input
	 * bytes.  We add an additional 2 bytes to the estimate to
	 * compensate for the fact that the encoded string will have "..."
	 * delimiters. */
	uindex_t t_base64_length_estimate =
		2 + (uindex_t) rint (ceil ((double) t_source_length * 4 / 3));

	MCAutoStringRef t_formatted_string;
	bool t_as_string = false;
	t_as_string = (t_valid_utf8 &&
				   MCStackdirFormatString (*t_string, &t_formatted_string) &&
				   MCStringGetLength (*t_formatted_string) < t_base64_length_estimate);

	MCAutoStringRef t_result;
	if (t_as_string)
	{
		t_success = MCStringMutableCopy (*t_formatted_string, &t_result);
	}
	else
	{
		MCAutoStringRef t_base64;
		MCU_base64encode (p_data, &t_base64);

		/* Note that we have to manually remove all the newline
		 * characters inserted by MCU_base64encode. */
		t_success = (MCStringMutableCopy (*t_base64, &t_result) &&
					 MCStringFindAndReplace (*t_result, kMCLineEndString,
											 kMCEmptyString,
											 kMCStringOptionCompareExact));
	}

	/* Add "<>" delimiters */
	if (t_success)
		t_success = (MCStringPrependCodepoint (*t_result, '<') &&
					 MCStringAppendCodepoint (*t_result, '>'));

	if (t_success)
		r_literal = MCValueRetain (*t_result);

	return t_success;
}

/* ----------------------------------------------------------------
 * String formatting
 * ---------------------------------------------------------------- */

static bool
MCStackdirFormatString (MCStringRef p_string, MCStringRef & r_literal)
{
	MCTextFilter *t_filter;
	t_filter = MCTextFilterCreate (p_string, kMCStringOptionCompareExact);

	MCAutoStringRef t_result;
	/* UNCHECKED */ MCStringCreateMutable (2 + MCStringGetLength (p_string),
										   & t_result);

	/* Add delimiter at start */
	if (!MCStringAppendChar (*t_result, '"')) return false;

	/* Build escaped string */
	while (t_filter->HasData())
	{
		codepoint_t t_source = t_filter->GetNextCodepoint ();
		t_filter->AdvanceCursor ();

		/* Always escape '"' and '\' */
		if (t_source == '"' || t_source == '\\')
		{
			if (!(MCStringAppendChar (*t_result, '\\') &&
				  MCStringAppendCodepoint (*t_result, t_source)))
				return false;
			continue;
		}

		/* Unicode categories L, M, N, P, S and the space character
		 * U+0020 can be passed through directly. */
		uint32_t t_mask = U_GET_GC_MASK (t_source);
		if (t_source == ' ' ||
			t_mask & (U_GC_L_MASK | U_GC_M_MASK | U_GC_N_MASK |
					  U_GC_P_MASK | U_GC_S_MASK))
		{
			if (!MCStringAppendCodepoint (*t_result, t_source))
				return false;
			continue;
		}

		MCStringAppendChar (*t_result, '\\');

		/* The newline and carriage return characters have special
		 * escape sequences */
		if (t_source == '\n')
		{
			if (!MCStringAppendChar (*t_result, 'n')) return false;
			continue;
		}
		if (t_source == '\r')
		{
			if (!MCStringAppendChar (*t_result, 'r')) return false;
			continue;
		}

		/* Encode character in hexadecimal. */
		char t_hexc[8];
		MCAutoStringRef t_hex;
		sprintf (t_hexc, "%08x", t_source);
		if (!MCStringCreateWithCString (t_hexc, &t_hex)) return false;

		if (t_source <= 0xffU) /* Unicode codepoints < 0xff can use \xhh */
		{
			if (!(MCStringAppendChar (*t_result, 'x') &&
				  MCStringAppendSubstring (*t_result, *t_hex, MCRangeMake (6, -1))))
				return false;
			continue;
		}
		else if (t_source <= 0xffffU) /* Unicode codepoints < 0xffff can use \uhh */
		{
			if (!(MCStringAppendChar (*t_result, 'x') &&
				  MCStringAppendSubstring (*t_result, *t_hex, MCRangeMake (4, -1))))
				return false;
			continue;
		}
		else /* All other codepoints can use \Uhhhhhhhh */
		{
			if (!(MCStringAppendChar (*t_result, 'x') &&
				  MCStringAppend (*t_result, *t_hex)))
			return false;
		}
	}

	/* Add delimiter at end */
	if (!MCStringAppendChar (*t_result, '"')) return false;

	return MCStringCopy (*t_result, r_literal);
}

/* ----------------------------------------------------------------
 * Name formatting
 * ---------------------------------------------------------------- */

/* Test whether p_name can be represented as an unquoted name
 * string. */
static bool
MCStackdirFormatName_TestUnquotable (MCStringRef p_string)
{
	if (MCStringIsEmpty (p_string)) return false;

	MCTextFilter *t_filter;
	t_filter = MCTextFilterCreate (p_string, kMCStringOptionCompareExact);
	MCAssert (t_filter);

	bool valid_unquoted = true;
	bool first_char = true;

	while (t_filter->HasData())
	{
		codepoint_t t_codepoint = t_filter->GetNextCodepoint();
		t_filter->AdvanceCursor();

		/* The first character must be a letter. */
		if (!first_char)
		{
			if (t_codepoint >= '0' && t_codepoint <= '9') continue;
			if (t_codepoint == '_') continue;
			if (t_codepoint == '-') continue;
			if (t_codepoint == '.') continue;
		}
		first_char = false;

		if (t_codepoint >= 'A' && t_codepoint <= 'Z') continue;
		if (t_codepoint >= 'a' && t_codepoint <= 'z') continue;

		valid_unquoted = false;
		break;
	}

	delete t_filter;

	return valid_unquoted;
}

static bool
MCStackdirFormatName (MCNameRef p_name, MCStringRef & r_literal)
{
	MCStringRef t_name_string = MCNameGetString (p_name);
	MCAutoStringRef t_normalized;
	/* UNCHECKED */ MCStackdirNormalizeString (t_name_string,
											   &t_normalized);

	if (MCStackdirFormatName_TestUnquotable (*t_normalized))
	{
		r_literal = MCValueRetain (*t_normalized);
		return true;
	}
	return MCStackdirFormatString (*t_normalized, r_literal);
}

/* ----------------------------------------------------------------
 * Filename formatting
 * ---------------------------------------------------------------- */

enum {
	kMCStackdirFilenameMaxLen = 255,
};

/* ================================================================
 * Scanning
 * ================================================================ */

/* ----------------------------------------------------------------
 * Scanning utility functions
 * ---------------------------------------------------------------- */

static bool
MCStackdirIsHexDigit (codepoint_t p_char)
{
	bool t_valid_hex = false;
	t_valid_hex |= (p_char >= '0' && p_char <= '9');
	t_valid_hex |= (p_char >= 'a' && p_char <= 'f');
	t_valid_hex |= (p_char >= 'A' && p_char <= 'F');
	return t_valid_hex;
}

static void
MCStackdirIOScannerResetToken (MCStackdirIOScannerRef scanner,
							   MCStackdirIOToken & x_token)
{
	/* Reset the token */
	MCValueRelease (x_token.m_value);

	x_token.m_line = scanner->m_line;
	x_token.m_column = scanner->m_column;
	x_token.m_type = kMCStackdirIOTokenTypeNone;
	x_token.m_value = nil;
}

void
MCStackdirIOTokenCopy (MCStackdirIOToken & p_src,
					   MCStackdirIOToken & r_dest)
{
	MCValueRelease (r_dest.m_value);

	r_dest.m_line = p_src.m_line;
	r_dest.m_column = p_src.m_column;
	r_dest.m_type = p_src.m_type;

	if (p_src.m_value != nil)
		r_dest.m_value = MCValueRetain (p_src.m_value);
	else
		r_dest.m_value = nil;
}

static inline bool
MCStackdirIOTokenIsNone (MCStackdirIOToken & p_token)
{
	return (p_token.m_type == kMCStackdirIOTokenTypeNone);
}

static inline bool
MCStackdirIOTokenIsError (MCStackdirIOToken & p_token)
{
	return (p_token.m_type == kMCStackdirIOTokenTypeError);
}

static inline bool
MCStackdirIOScannerGetChar (MCStackdirIOScannerRef scanner,
							codepoint_t & r_char)
{
	if (!scanner->m_filter->HasData()) return false;
	r_char = scanner->m_filter->GetNextCodepoint ();
	return true;
}

static inline bool
MCStackdirIOScannerNextChar (MCStackdirIOScannerRef scanner)
{
	/* This function also takes care of tracking the column number. */
	if (scanner->m_filter->AdvanceCursor ())
	{
		++scanner->m_column;
		return true;
	}
	return false;
}

/* ----------------------------------------------------------------
 * Scanning special tokens
 * ---------------------------------------------------------------- */

static bool
MCStackdirIOScanEOF (MCStackdirIOScannerRef scanner,
					 MCStackdirIOToken & r_token)
{
	codepoint_t t_char;
	if (MCStackdirIOScannerGetChar (scanner, t_char)) return false;

	r_token.m_type = kMCStackdirIOTokenTypeEOF;
	return true;
}

static bool
MCStackdirIOScanNewline (MCStackdirIOScannerRef scanner,
						 MCStackdirIOToken & r_token)
{
	codepoint_t t_char;
	bool t_found_newline = false;
	MCStackdirIOScannerGetChar (scanner, t_char);
	if (t_char == '\n')
	{
		MCStackdirIOScannerNextChar (scanner);
		t_found_newline = true;
	}
	else if (t_char == '\r')
	{
		t_found_newline = true;
		 /* To cope with \r\n line endings, we need to check if the
		  * next character is \n, and if so, step over it. */
		if (MCStackdirIOScannerNextChar (scanner))
		{
			MCStackdirIOScannerGetChar (scanner, t_char);
			if (t_char == '\n')
				MCStackdirIOScannerNextChar (scanner);
		}
	}

	if (!t_found_newline) return false;

	/* Update the scanner's line and column counters */
	scanner->m_column = 1;
	scanner->m_line += 1;

	r_token.m_type = kMCStackdirIOTokenTypeNewline;
	return true;
}

/* ----------------------------------------------------------------
 * Scanning trivial tokens
 * ---------------------------------------------------------------- */

static bool
MCStackdirIOScanSpace (MCStackdirIOScannerRef scanner,
					   MCStackdirIOToken & r_token)
{
	codepoint_t t_char;
	MCStackdirIOScannerGetChar (scanner, t_char);

	bool t_found_space = false;
	while (t_char == ' ')
	{
		t_found_space = true;
		if (!MCStackdirIOScannerNextChar (scanner)) break;
		MCStackdirIOScannerGetChar (scanner, t_char);
	}
	if (!t_found_space) return false;

	r_token.m_type = kMCStackdirIOTokenTypeSpace;
	return true;
}

static bool
MCStackdirIOScanStorageSeparator (MCStackdirIOScannerRef scanner,
								  MCStackdirIOToken & r_token)
{
	codepoint_t t_char;
	MCStackdirIOScannerGetChar (scanner, t_char);
	if (t_char == ':')
	{
		MCStackdirIOScannerNextChar (scanner);
		r_token.m_type = kMCStackdirIOTokenTypeStorageSeparator;
		return true;
	}
	return false;
}

static bool
MCStackdirIOScanExternalIndicator (MCStackdirIOScannerRef scanner,
								   MCStackdirIOToken & r_token)
{
	codepoint_t t_char;
	MCStackdirIOScannerGetChar (scanner, t_char);
	if (t_char == '&')
	{
		MCStackdirIOScannerNextChar (scanner);
		r_token.m_type = kMCStackdirIOTokenTypeExternalIndicator;
		return true;
	}
	return false;
}

/* ----------------------------------------------------------------
 * Scanning numbers
 * ---------------------------------------------------------------- */

/* This implements a scanner that supports the following syntax:
 *
 * NUMBER         ::= [SIGN] (INTEGER_NUMBER | REAL_NUMBER)
 * INTEGER_NUMBER ::= NONZERODIGIT DIGIT* | "0"
 * REAL_NUMBER    ::= POINT_REAL | EXPONENT_REAL
 * POINT_REAL     ::= [INT_PART] FRACTION | INT_PART "."
 * EXPONENT_REAL  ::= (INT_PART | POINT_REAL) EXPONENT
 * INT_PART       ::= DIGIT+
 * FRACTION       ::= "." DIGIT+
 * EXPONENT       ::= "e" [SIGN] DIGIT+
 * SIGN           ::= "+" | "-"
 * DIGIT          ::= "0"..."9"
 * NONZERODIGIT   ::= "1"..."9"
 */

/* Both integers and real numbers may have an optional sign prefix.
 * It defaults to '+' if not specified. */
static bool
MCStackdirIOScanNumberSign (MCStackdirIOScannerRef scanner,
							MCStackdirIOToken & r_token,
							integer_t & r_sign)
{
	codepoint_t t_char;

	r_sign = 1;
	if (!MCStackdirIOScannerGetChar (scanner, t_char))
		return false;

	if (t_char == '+' || t_char == '-')
	{
		r_sign = (t_char == '+') ? 1 : -1;
		MCStackdirIOScannerNextChar (scanner);
		return true;
	}
	return false;
}

/* Scan the integer part of a number, which must be a sequence of
 * decimal digits not starting with a zero, or zero (for integers) or
 * a sequence of decimal digits (for reals).  We scan into a uint64_t
 * because it's guaranteed to be able to store the full mantissa of a
 * double (if for some reason someone decides to write it out as an
 * integer). */
static bool
MCStackdirIOScanNumberInteger (MCStackdirIOScannerRef scanner,
							   MCStackdirIOToken & r_token,
							   uint64_t & r_integer_part,
							   bool & r_is_valid_integer)
{
	codepoint_t t_char;

	bool t_found_digit = false;
	r_is_valid_integer = true;
	r_integer_part = 0;

	while (MCStackdirIOScannerGetChar (scanner, t_char))
	{
		/* Check if the character is a digit. */
		if (!(t_char >= '0' && t_char <= '9')) break;
		t_found_digit = true;

		/* Integers aren't allowed to start with a leading 0 */
		if (t_char == '0' && r_integer_part == 0)
			r_is_valid_integer = false;

		/* Add each digit into the integer part, checking for
		 * overflow. */
		uint64_t t_new = (r_integer_part * 10) + (t_char - '0');
		if (t_new < r_integer_part)
		{
			/* Overflow */
			r_token.m_type = kMCStackdirIOTokenTypeError;
			return false;
		}

		r_integer_part = t_new;
		MCStackdirIOScannerNextChar (scanner);
	}
	return t_found_digit;
}

/* Scan the fraction part of a real number.  It has to be a '.'
 * followed by some sequence of digits.  This is constructed into a
 * double precision integer. */
/* BUG This is a naive implementation that doesn't check for or
 * compensate for rounding errors. */
static bool
MCStackdirIOScanNumberFraction (MCStackdirIOScannerRef scanner,
								MCStackdirIOToken & r_token,
								real64_t & r_fraction_part)
{
	codepoint_t t_char;

	r_fraction_part = 0;

	/* The fraction part must start with a '.' */
	if (!MCStackdirIOScannerGetChar (scanner, t_char) ||
		t_char != '.')
		return false;
	MCStackdirIOScannerNextChar (scanner);

	/* Ensure that we can detect an over- or underflow */
	errno = 0;

	real64_t t_places = 0;
	while (MCStackdirIOScannerGetChar (scanner, t_char))
	{
		/* Check that the character is a digit */
		if (!(t_char >= '0' && t_char <= '9')) break;

		/* Add each digit into the fraction part */
		uint32_t t_digit = t_char - '0';
		r_fraction_part += t_digit * pow (10, - (++t_places));

		MCStackdirIOScannerNextChar (scanner);
	}

	/* Underflow check */
	if (errno == ERANGE)
	{
		r_token.m_type = kMCStackdirIOTokenTypeError;
		return false;
	}

	return true;
}

static bool
MCStackdirIOScanNumberExponent (MCStackdirIOScannerRef scanner,
								MCStackdirIOToken & r_token,
								real64_t & r_exponent_factor)
{
	codepoint_t t_char;

	r_exponent_factor = 1.0;

	/* The exponent part must start with an 'e' */
	if (!MCStackdirIOScannerGetChar (scanner, t_char) ||
		t_char != 'e')
		return false;
	MCStackdirIOScannerNextChar (scanner);

	/* The exponent can have an optional sign */
	integer_t t_exponent_sign;
	MCStackdirIOScanNumberSign (scanner,
								r_token,
								t_exponent_sign);

	/* The exponent is an integer and is required to follow the
	 * 'e'. */
	bool t_ignored;
	uint64_t t_exponent_unsigned;
	if (!MCStackdirIOScanNumberInteger (scanner,
										r_token,
										t_exponent_unsigned,
										t_ignored))
	{
		r_token.m_type = kMCStackdirIOTokenTypeError;
		return false;
	}

	/* Attempt to create the exponent factor, checking for
	 * overflow. */
	real64_t t_exponent;
	errno = 0;
	t_exponent = copysign (t_exponent_unsigned, t_exponent_sign);
	r_exponent_factor = pow (10, t_exponent);

	if (errno == ERANGE)
	{
		r_token.m_type = kMCStackdirIOTokenTypeError;
		return false;
	}

	return true;
}

static bool
MCStackdirIOScanNumber (MCStackdirIOScannerRef scanner,
						MCStackdirIOToken & r_token)
{
	codepoint_t t_char;

	/* Parse sign, if present */
	bool t_have_sign;
	integer_t t_sign;
	t_have_sign = MCStackdirIOScanNumberSign (scanner,
											  r_token,
											  t_sign);

	/* Parse integer, if present */
	uint64_t t_integer_part;
	bool t_have_integer_part;
	bool t_is_valid_integer;
	t_have_integer_part = MCStackdirIOScanNumberInteger (scanner,
														 r_token,
														 t_integer_part,
														 t_is_valid_integer);

	/* Parse fraction part, if present */
	real64_t t_fraction_part;
	bool t_have_fraction_part;
	t_have_fraction_part = MCStackdirIOScanNumberFraction (scanner,
														   r_token,
														   t_fraction_part);

	/* If there's no integer or fraction part, this isn't a number. */
	if (!t_have_integer_part && !t_have_fraction_part)
	{
		/* Bare signs are a syntax error */
		if (t_have_sign)
			r_token.m_type = kMCStackdirIOTokenTypeError;
		return false;
	}

	/* Parse exponent part, if present */
	real64_t t_exponent_factor;
	bool t_have_exponent_part;
	t_have_exponent_part = MCStackdirIOScanNumberExponent (scanner,
														   r_token,
														   t_exponent_factor);

	real64_t t_real_value;
	integer_t t_integer_value;
	bool t_is_real = false;
	bool t_is_integer = false;

	if (t_have_exponent_part || t_have_fraction_part)
	{
		/* If there's an exponent or fraction part, this must be a
		 * real. */
		t_real_value = t_exponent_factor * ((real64_t) t_integer_part +
											t_fraction_part);
		t_real_value = copysign (t_real_value, t_sign);
		t_is_real = true;
	}
	else if (t_have_integer_part && t_is_valid_integer)
	{
		/* Alternatively, we might have an integer */
		if (t_integer_part < INT_MAX)
		{
			t_integer_value = t_integer_part * t_sign;
			t_is_integer = true;
		}
		else
		{
			/* Cope with big integers by falling back to floats */
			t_real_value = (real64_t) t_integer_part * t_sign;
			t_is_real = true;
		}
	}
	else
	{
		/* Some invalid syntax occurred */
		r_token.m_type = kMCStackdirIOTokenTypeError;
		return false;
	}

	if (t_is_real)
	{
		MCAutoNumberRef t_value;
		/* UNCHECKED */ MCNumberCreateWithReal (t_real_value, &t_value);
		r_token.m_value = MCValueRetain (*t_value);
		r_token.m_type = kMCStackdirIOTokenTypeNumber;
		return true;
	}

	if (t_is_integer)
	{
		MCAutoNumberRef t_value;
		/* UNCHECKED */ MCNumberCreateWithInteger (t_integer_value, &t_value);
		r_token.m_value = MCValueRetain (*t_value);
		r_token.m_type = kMCStackdirIOTokenTypeNumber;
		return true;
	}

	MCUnreachable ();
	return false;
}

/* ----------------------------------------------------------------
 * Scanning unquoted strings
 * ---------------------------------------------------------------- */

static bool
MCStackdirIOScanUnquotedString (MCStackdirIOScannerRef scanner,
								MCStackdirIOToken & r_token)
{
	MCAutoStringRef t_value;
	codepoint_t t_char;
	bool t_found = false;

	while (MCStackdirIOScannerGetChar (scanner, t_char))
	{
		bool t_valid_char = false;

		/* The first character must be a letter */
		if (t_found)
		{
			t_valid_char |= (t_char >= '0' && t_char <= '9');
			t_valid_char |= (t_char == '_');
			t_valid_char |= (t_char == '-');
			t_valid_char |= (t_char == '.');
		}

		t_valid_char |= (t_char >= 'a' && t_char <= 'z');
		t_valid_char |= (t_char >= 'A' && t_char <= 'Z');

		if (!t_valid_char) break;

		/* Only allocate storage for the result if we found a valid
		 * character. */
		if (!t_found)
		{
			/* UNCHECKED */ MCStringCreateMutable (1, &t_value);
			t_found = true;
		}

		/* UNCHECKED */ MCStringAppendCodepoint (*t_value, t_char);
		MCStackdirIOScannerNextChar (scanner);
	}

	if (t_found)
	{
		r_token.m_type = kMCStackdirIOTokenTypeUnquotedString;
		r_token.m_value = MCValueRetain (*t_value);
		return true;
	}
	return false;
}

/* ----------------------------------------------------------------
 * Scanning regular strings
 * ---------------------------------------------------------------- */

static bool
MCStackdirIOScanStringUnescape (MCStackdirIOScannerRef scanner,
								MCStackdirIOToken & r_token,
								codepoint_t & r_char)
{
	/* This should be called when the current character is an escape
	 * character */
	MCStackdirIOScannerNextChar (scanner);

	/* The escape sequence *must* begin with a valid escape type
	 * specifier. */
	if (!MCStackdirIOScannerGetChar (scanner, r_char))
	{
		r_token.m_type = kMCStackdirIOTokenTypeError;
		return false;
	}
	MCStackdirIOScannerNextChar (scanner);

	/* There are two types of escape: direct escapes, and escapes that
	 * take a number of hexadecimal characters as an argument. */
	uindex_t t_num_digits;
	switch (r_char)
	{
	case '"':
	case '\\':
		return true;
	case 'n':
		r_char = '\n';
		return true;
	case 'r':
		r_char = '\r';
		return true;
	case 'x':
		t_num_digits = 2;
		break;
	case 'u':
		t_num_digits = 4;
		break;
	case 'U':
		t_num_digits = 8;
		break;
	default:
		r_token.m_type = kMCStackdirIOTokenTypeError;
		return false;
	}

	char t_hex[9];

	/* Consume the right number of characters, ensuring that they are
	 * hexadecimal digits */
	codepoint_t t_hex_char;
	uindex_t i;
	for (i = 0; i < t_num_digits; ++i)
	{
		if (!MCStackdirIOScannerGetChar (scanner, t_hex_char))
		{
			r_token.m_type = kMCStackdirIOTokenTypeError;
			return false;
		}

		if (!MCStackdirIsHexDigit (t_hex_char))
		{
			r_token.m_type = kMCStackdirIOTokenTypeError;
			return false;
		}

		t_hex[i] = (char) t_hex_char;
		MCStackdirIOScannerNextChar (scanner);
	}
	t_hex[i] = 0; /* Trailing nul */

	/* Parse the hex string into a codepoint */
	r_char = strtoul (t_hex, NULL, 16);
	return true;
}

static bool
MCStackdirIOScanString (MCStackdirIOScannerRef scanner,
						MCStackdirIOToken & r_token)
{
	codepoint_t t_char;

	/* Strings start with a string delimiter */
	MCStackdirIOScannerGetChar (scanner, t_char);
	if (t_char != '"') return false;
	MCStackdirIOScannerNextChar (scanner);

	MCAutoStringRef t_value;
	/* UNCHECKED */ MCStringCreateMutable (0, &t_value);

	while (true)
	{
		if (!MCStackdirIOScannerGetChar (scanner, t_char))
		{
			r_token.m_type = kMCStackdirIOTokenTypeError;
			return false;
		}

		if (t_char == '"')
		{
			/* End of string. Make sure to step past the end
			 * delimiter! */
			MCStackdirIOScannerNextChar (scanner);
			break;
		}
		else if (t_char == '\\')
		{
			/* Escape character */
			if (!MCStackdirIOScanStringUnescape (scanner,
												 r_token,
												 t_char))
				return false;
			/* UNCHECKED */ MCStringAppendCodepoint (*t_value, t_char);
		}
		else
		{
			/* Normal character */
			MCStackdirIOScannerNextChar (scanner);
			/* UNCHECKED */ MCStringAppendCodepoint (*t_value, t_char);
		}
	}

	r_token.m_value = MCValueRetain (*t_value);
	r_token.m_type = kMCStackdirIOTokenTypeString;
	return true;
}

/* ----------------------------------------------------------------
 * Scanning data
 * ---------------------------------------------------------------- */

static bool
MCStackdirIOScanDataBase64 (MCStackdirIOScannerRef scanner,
							MCStackdirIOToken & r_token,
							MCDataRef &r_data)
{
	codepoint_t t_char;
	MCAutoStringRef t_base64;
	/* UNCHECKED */ MCStringCreateMutable (0, &t_base64);

	/* Build a string containing base64 encoded data */
	while (MCStackdirIOScannerGetChar (scanner, t_char))
	{
		bool t_valid_char = false;
		/* RFC4648 base64 dictionary */
		t_valid_char |= (t_char >= '0' && t_char <= '9');
		t_valid_char |= (t_char >= 'a' && t_char <= 'z');
		t_valid_char |= (t_char >= 'A' && t_char <= 'Z');
		t_valid_char |= (t_char == '+');
		t_valid_char |= (t_char == '/');
		t_valid_char |= (t_char == '=');

		if (!t_valid_char) break;

		/* UNCHECKED */ MCStringAppendCodepoint (*t_base64, t_char);
		MCStackdirIOScannerNextChar (scanner);
	}

	/* Decode data */
	MCU_base64decode (*t_base64, r_data);
	return true;
}

static bool
MCStackdirIOScanData (MCStackdirIOScannerRef scanner,
					  MCStackdirIOToken & r_token)
{
	codepoint_t t_char;
	MCAutoDataRef t_value;

	/* Data starts with a data start delimiter */
	if (!MCStackdirIOScannerGetChar (scanner, t_char) ||
		(t_char != '<')) return false;

	if (!MCStackdirIOScannerNextChar (scanner))
	{
		r_token.m_type = kMCStackdirIOTokenTypeError;
		return false;
	}

	/* The data might be in string format.  Otherwise, it's necessary
	 * to read base64 characters up to the next data delimiter. */
	if (MCStackdirIOScanString (scanner, r_token))
	{
		MCStringRef t_string = (MCStringRef) r_token.m_value;
		/* UNCHECKED */ MCStringEncodeAndRelease (t_string,
												  kMCStringEncodingUTF8,
												  false,
												  &t_value);
		r_token.m_value = nil;
	}
	else
	{
		MCStackdirIOScanDataBase64 (scanner,
									r_token,
									&t_value);
	}

	/* Check that the data *ends* with a date end delimiter */
	if (!MCStackdirIOScannerGetChar (scanner, t_char) ||
		(t_char != '>'))
	{
		r_token.m_type = kMCStackdirIOTokenTypeError;
		return false;
	}
	MCStackdirIOScannerNextChar (scanner);

	r_token.m_type = kMCStackdirIOTokenTypeData;
	r_token.m_value = MCValueRetain (*t_value);
	return true;
}

/* ----------------------------------------------------------------
 * Scanning _shared file UUID headers
 * ---------------------------------------------------------------- */

static bool
MCStackdirIOScanSharedHeader (MCStackdirIOScannerRef scanner,
							  MCStackdirIOToken & r_token)
{
	codepoint_t t_char;

	/* Shared headers have to start with '[' */
	if (!MCStackdirIOScannerGetChar (scanner, t_char) ||
		(t_char != '[')) return false;

	if (!MCStackdirIOScannerNextChar (scanner))
	{
		r_token.m_type = kMCStackdirIOTokenTypeError;
		return false;
	}

	/* The contents of the header is a UUID in dashed hexadecimal
	 * format. */
	MCAutoStringRef t_value;
	/* UNCHECKED */ MCStringCreateMutable (0, &t_value);

	bool t_found_end = false;
	while (!t_found_end)
	{
		if (!MCStackdirIOScannerGetChar (scanner, t_char))
		{
			r_token.m_type = kMCStackdirIOTokenTypeError;
			return false;
		}

		if (t_char == ']')
		{
			t_found_end = true;
		}
		else if (MCStackdirIsHexDigit (t_char) || t_char == '-')
		{
			/* UNCHECKED */ MCStringAppendCodepoint (*t_value, t_char);
		}
		else
		{
			r_token.m_type = kMCStackdirIOTokenTypeError;
			return false;
		}

		MCStackdirIOScannerNextChar (scanner);
	}

	r_token.m_value = MCValueRetain (*t_value);
	r_token.m_type = kMCStackdirIOTokenTypeSharedHeader;
	return true;
}

/* ----------------------------------------------------------------
 * Scanning flags
 * ---------------------------------------------------------------- */

static bool
MCStackdirIOScanFlag (MCStackdirIOScannerRef scanner,
					  MCStackdirIOToken & r_token)
{
	codepoint_t t_char;

	/* Flags have to start with '!' */
	if (!MCStackdirIOScannerGetChar (scanner, t_char) ||
		(t_char != '!'))
		return false;

	MCStackdirIOScannerNextChar (scanner);
	r_token.m_type = kMCStackdirIOTokenTypeFlag;
	return true;
}

/* ================================================================
 * Main entry points
 * ================================================================ */

/* ----------------------------------------------------------------
 * Formatting
 * ---------------------------------------------------------------- */

bool
MCStackdirFormatFilename (MCStringRef p_string, MCStringRef p_suffix,
						  MCStringRef & r_filename)
{
	MCAutoStringRef t_normalized;
	if (!MCStackdirNormalizeString (p_string, &t_normalized))
		return false;

	uindex_t t_string_len = MCStringGetLength (*t_normalized);

	/* Empty filenames and unreasonably large filenames are not
	 * permitted */
	if (0 == t_string_len || t_string_len > kMCStackdirFilenameMaxLen)
		return false;

	MCAutoStringRef t_filename;
	if (!MCStringCreateMutable (t_string_len, &t_filename))
		return false;

	/* Valid filenames are less than 255 characters long and contain only
	 * the characters "[a-z0-9_-]".  They permit the escape sequences __,
	 * _xhh and _uhhhh. */
	for (uindex_t i = 0; i < t_string_len; ++i)
	{
		/* Don't attempt to build unreasonably large filenames */
		if (MCStringGetLength (*t_filename) >= kMCStackdirFilenameMaxLen)
			return false;

		unichar_t t_source = MCStringGetCharAtIndex (*t_normalized, i);

		/* a-z, 0-9 and - can be passed through */
		if ((t_source >= '0' && t_source <= '9') /* 0-9 */ ||
			(t_source >= 'a' && t_source <= 'z') /* a-z */ ||
			(t_source == '-')                    /* - */ )
		{
			if (!MCStringAppendChar (*t_filename, t_source))
				return false;
			continue;
		}

		if (!MCStringAppendChar (*t_filename, '_'))
			return false;

		if (t_source == '_')
		{
			/* __ escape sequence */
			if (!MCStringAppendChar (*t_filename, '_'))
				return false;
			continue;
		}

		char t_hexc[4];
		MCAutoStringRef t_hex;

		/* Encode character in hexadecimal. */
		sprintf (t_hexc, "%04hx", t_source);
		if (!MCStringCreateWithCString (t_hexc, &t_hex)) return false;

		/* Characters <= 0xff can use _xhh format. */
		if (t_source <= 0xffU)
		{
			if (!(MCStringAppendChar (*t_filename, 'x') &&
				  MCStringAppendSubstring (*t_filename, *t_hex, MCRangeMake (2, -1))))
				return false;
			continue;
		}

		/* Characters <= 0xffff can use _uhhhh format. */
		if (t_source <= 0xffffU)
		{
			if (!(MCStringAppendChar (*t_filename, 'u') &&
				  MCStringAppend (*t_filename, *t_hex)))
				return false;
			continue;
		}

		MCUnreachable ();
	}

	/* Append the normalized suffix and check that the resulting
	 * filename is short enough. */
	MCAutoStringRef t_normalized_suffix;
	if (!(MCStackdirNormalizeString (p_suffix, &t_normalized_suffix) &&
		  MCStringAppend (*t_filename, *t_normalized_suffix)))
		return false;

	if (MCStringGetLength (*t_filename) > kMCStackdirFilenameMaxLen)
		return false;

	r_filename = MCValueRetain (*t_filename);
	return true;
}

bool
MCStackdirFormatLiteral (MCValueRef p_value, MCStringRef & r_literal)
{
	switch (MCValueGetTypeCode (p_value))
	{
	case kMCValueTypeCodeName:
		return MCStackdirFormatName ((MCNameRef) p_value, r_literal);
	case kMCValueTypeCodeString:
		return MCStackdirFormatString ((MCStringRef) p_value, r_literal);
	case kMCValueTypeCodeData:
		return MCStackdirFormatData ((MCDataRef) p_value, r_literal);
	case kMCValueTypeCodeBoolean:
		return MCStackdirFormatBoolean ((MCBooleanRef) p_value, r_literal);
	case kMCValueTypeCodeNumber:
		return MCStackdirFormatNumber ((MCNumberRef) p_value, r_literal);
	case kMCValueTypeCodeArray:
		return MCStringCopy (kMCStackdirArrayLiteral, r_literal);
	default:
		MCUnreachable ();
	}
}

/* ----------------------------------------------------------------
 * Parsing
 *---------------------------------------------------------------- */

bool
MCStackdirParseFilename (MCStringRef p_filename,
						 MCStringRef p_suffix,
						 MCStringRef & r_string)
{
	/* If a non-empty suffix p_suffix is provided, it must be present
	 * (and is then removed before further processing) */
	MCAutoStringRef t_basename;
	if (MCStringIsEmpty (p_suffix))
	{
		&t_basename = MCValueRetain (p_filename);
	}
	else
	{
		if (!MCStringEndsWith (p_filename,
							   p_suffix,
							   kMCStringOptionCompareExact))
			return false;

		uindex_t t_filename_len = MCStringGetLength (p_filename);
		uindex_t t_suffix_len = MCStringGetLength (p_suffix);
		if (!MCStringCopySubstring (p_filename,
									MCRangeMake (0, t_filename_len-t_suffix_len),
									&t_basename))
			return false;
	}

	uindex_t t_basename_len = MCStringGetLength (*t_basename);

	MCAutoStringRef t_string;
	if (!MCStringCreateMutable (t_basename_len, &t_string))
		return false;

	/* Valid filenames are less than 255 characters long and contain
	 * only the characters "[a-z0-9_-]". They permit the escape
	 * sequences __, _xhh and _uhhhh. */
	for (uindex_t i = 0; i < t_basename_len; ++i)
	{
		/* Check that each character is valid */
		unichar_t t_source = MCStringGetCharAtIndex (*t_basename, i);

		bool t_valid = ((t_source >= '0' && t_source <= '9') ||
						(t_source >= 'a' && t_source <= 'z') ||
						(t_source == '-') ||
						(t_source == '_'));
		if (!t_valid) return false;

		/* If non-escape character, pass through */
		if (t_source != '_')
		{
			if (!MCStringAppendChar (*t_string, t_source)) return false;
			continue;
		}

		/* Process escape sequence */
		uindex_t t_num_digits;

		if (++i >= t_basename_len) return false;
		t_source = MCStringGetCharAtIndex (*t_basename, i);

		switch (t_source)
		{
		case '_': /* Literal '_' */
			if (!MCStringAppendChar (*t_string, '_'))
				return false;
			continue;

		case 'x': /* _xhh */
			t_num_digits = 2;
			break;
		case 'u': /* _uhhhh */
			t_num_digits = 4;
			break;

		default:
			return false;
		}

		/* Read hex characters */
		char t_hex[5];
		uindex_t j;
		for (j = 0; j < t_num_digits; ++j)
		{
			codepoint_t t_hex_char;

			if (++i >= t_basename_len) return false;
			t_hex_char = MCStringGetCharAtIndex (*t_basename, i);

			if (!MCStackdirIsHexDigit (t_hex_char)) return false;

			t_hex[j] = (char) t_hex_char;
		}
		t_hex[j] = 0; /* Trailing nul */

		/* Parse the hex string into a char */
		t_source = strtoul (t_hex, NULL, 16);
		if (!MCStringAppendChar (*t_string, t_source)) return false;
	}

	r_string = MCValueRetain (*t_string);
	return true;
}

/* ----------------------------------------------------------------
 * Scanning
 *---------------------------------------------------------------- */

bool
MCStackdirIOScannerNew (MCStringRef p_string,
						MCStackdirIOScannerRef & scanner)
{
	if (!MCMemoryNew (scanner)) return false;

	scanner->m_filter = MCTextFilterCreate (p_string,
											kMCStringOptionCompareExact);
	if (!scanner->m_filter)
	{
		MCMemoryDelete (scanner);
		return false;
	}

	scanner->m_line = 1;
	scanner->m_column = 1;

	return true;
}

void
MCStackdirIOScannerDestroy (MCStackdirIOScannerRef & scanner)
{
	if (scanner == nil) return;

	MCValueRelease (scanner->m_token.m_value);
	delete scanner->m_filter;

	MCMemoryDelete (scanner);

	scanner = nil;
}

bool
MCStackdirIOScannerConsume (MCStackdirIOScannerRef scanner,
							MCStackdirIOToken & r_token,
							MCStackdirIOTokenType p_accept_type)
{
	MCAssert (scanner != nil);

	bool t_result = MCStackdirIOScannerPeek (scanner,
											 r_token,
											 p_accept_type);
	scanner->m_has_peek = false;
	return t_result;
}

bool
MCStackdirIOScannerPeek (MCStackdirIOScannerRef scanner,
						 MCStackdirIOToken & r_token,
						 MCStackdirIOTokenType p_accept_type)
{
	MCAssert (scanner != nil);

	MCStackdirIOToken & t_token = scanner->m_token;

	/* Peeking multiple times in a row will just return the same
	 * token. */
	if (scanner->m_has_peek || MCStackdirIOTokenIsError (t_token))
	{
		MCStackdirIOTokenCopy (t_token, r_token);

		bool t_success = !MCStackdirIOTokenIsError (r_token);

		/* Handle a narrowing token type selection */
		if (p_accept_type != kMCStackdirIOTokenTypeNone)
			return t_success && t_token.m_type == p_accept_type;

		return t_success;
	}

	/* Reset the token */
	MCStackdirIOScannerResetToken (scanner, t_token);

	bool t_success = true;

	/* Try to match each token type by looping through the table of
	 * scanning functions in order. */
	for (int i = 0; kMCStackdirIOScanFuncs[i] != NULL; ++i)
	{
		MCStackdirIOScanFunc t_scan = kMCStackdirIOScanFuncs[i];
		if (t_scan (scanner, t_token)) break;
		if (MCStackdirIOTokenIsError (t_token))
		{
			t_success = false;
			break;
		}
	}

	/* If nothing was found, signal an error */
	if (t_success && MCStackdirIOTokenIsNone (t_token))
	{
		t_token.m_type = kMCStackdirIOTokenTypeError;
		t_success = false;
	}

	/* We always set this to true.  This ensures that if there's an
	 * error, repeated calls to MCStackdirIOScannerPeek(C) will
	 * continue to indicate the error without rescanning. */
	scanner->m_has_peek = true;

	MCStackdirIOTokenCopy (t_token, r_token);

	/* Handle a narrowing token type selection */
	if (p_accept_type != kMCStackdirIOTokenTypeNone)
		return t_success && t_token.m_type == p_accept_type;

	return t_success;
}
