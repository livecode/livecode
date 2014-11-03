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

#include <unicode/uchar.h>

/* This file contains code relating to encoding and decoding the
 * values stored in Expanded Livecode Stackfiles.
 */

/* ================================================================
 * File-local declarations
 * ================================================================ */

/* Normalize and case-fold a string (Normalized Form C, Simple
 * Case-Fold). */
static bool MCStackdirNormalizeString (MCStringRef p_string, MCStringRef & r_normalized);

static bool MCStackdirFormatBoolean (MCBooleanRef p_bool, MCStringRef & r_literal);
static bool MCStackdirFormatInteger (MCNumberRef p_number, MCStringRef & r_literal);
static bool MCStackdirFormatReal (MCNumberRef p_number, MCStringRef & r_literal);
static bool MCStackdirFormatNumber (MCNumberRef p_number, MCStringRef & r_literal);
static bool MCStackdirFormatData (MCDataRef p_data, MCStringRef & r_literal);
static bool MCStackdirFormatString (MCStringRef p_string, MCStringRef & r_literal);
static bool MCStackdirFormatName (MCNameRef p_name, MCStringRef & r_literal);

/* ================================================================
 * Utility functions
 * ================================================================ */

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

/* ================================================================
 * Booleans
 * ================================================================ */

static bool
MCStackdirFormatBoolean (MCBooleanRef p_bool, MCStringRef & r_literal)
{
	return MCStringCopy (p_bool ? MCSTR ("true") : MCSTR ("false"),
						 r_literal);
}

/* ================================================================
 * Numbers
 * ================================================================ */

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
	t_success = MCStringFormat (&t_format, "%+16g",
								 MCNumberFetchAsReal (p_number));

	/* We need to ensure that the result always includes a '.' or an
	 * 'e' character so that it is correctly distinguished from an
	 * integer */
	bool t_found = false;
	if (t_success)
	{
		t_found = MCStringWildcardMatch (*t_format,
										 MCRangeMake (0, -1),
										 MCSTR ("[.e]"),
										 kMCStringOptionCompareExact);

		if (t_found)
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

/* ================================================================
 * Data
 * ================================================================ */

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
	/* base64 uses approximately 4 output bytes for every 3 input bytes */
	uindex_t t_base64_length_estimate =
		(uindex_t) rint (ceil ((double) t_source_length * 4 / 3));

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

/* ================================================================
 * Strings
 * ================================================================ */

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

/* ================================================================
 * Names
 * ================================================================ */

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

	while (t_filter->HasData())
	{
		codepoint_t t_codepoint = t_filter->GetNextCodepoint();
		t_filter->AdvanceCursor();

		if (t_codepoint >= '0' && t_codepoint <= '9') continue;
		if (t_codepoint >= 'A' && t_codepoint <= 'Z') continue;
		if (t_codepoint >= 'a' && t_codepoint <= 'z') continue;
		if (t_codepoint == '_') continue;
		if (t_codepoint == '-') continue;
		if (t_codepoint == '.') continue;

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

/* ================================================================
 * Filenames
 * ================================================================ */

enum {
	kMCStackdirFilenameMaxLen = 255,
};

/* ================================================================
 * Main entry points
 * ================================================================ */

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
		return MCSTR ("array");
	default:
		MCUnreachable ();
	}
}
