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

/* Filter for escaping characters in stackdir strings */
class MCStackdirStringEscapeFilter : public MCTextFilter
{
public:
	MCStackdirStringEscapeFilter () : m_state_len (0) {}
	~MCStackdirStringEscapeFilter () {}

	virtual codepoint_t GetNextCodepoint ()
	{
		/* If we have some escape sequence characters queued up,
		 * return the next one. */
		if (m_state_len > 0)
			return m_state[m_state_len + 1];
		return 0;
	}

	virtual bool AdvanceCursor ()
	{
		/* If the state vector was previously non-empty, shift to the
		 * next position. */
		if (m_state_len > 0)
			--m_state_len;

		/* If the state vector hasn't been drained yet, we're done. */
		if (m_state_len > 0)
			return true;

		/* Otherwise, repopulate the state with the next source
		 * character */
		if (!PrevFilter () -> AdvanceCursor ()) return false;
		codepoint_t t_source = PrevFilter () -> GetNextCodepoint ();

		/* The '"' and '\' characters must always be escaped */
		if (t_source == '"' || t_source == '\\' )
		{
			m_state[1] = '\\';
			m_state[0] = t_source;
			m_state_len = 2;
			return true;
		}

		/* Unicode categories L, M, N, P, S and the space character
		 * U+0020 can be passed through directly. */
		uint32_t t_mask = U_GET_GC_MASK (t_source);
		if (t_source == ' ' ||
			t_mask & (U_GC_L_MASK | U_GC_M_MASK | U_GC_N_MASK |
					  U_GC_P_MASK | U_GC_S_MASK))
		{
			m_state[0] = t_source;
			m_state_len = 1;
			return true;
		}

		/* The newline and carriage return characters have special
		 * escape sequences */
		if (t_source == '\n')
		{
			m_state[1] = '\\';
			m_state[0] = 'n';
			m_state_len = 2;
			return true;
		}

		if (t_source == '\r')
		{
			m_state[1] = '\\';
			m_state[0] = 'r';
			m_state_len = 2;
			return true;
		}

		/* Place codepoint into state array in (reversed!) hexadecimal
		 * format */
		unsigned char t_hex[8];
		sprintf ((char *) t_hex, "%08X", t_source);
		for (int i = 0; i < 8; ++i)
			m_state[i] = t_hex[7-i];

		/* Unicode codepoints < 0xff can use \xhh format */
		if (t_source <= 0xffU)
		{
			m_state[3] = '\\';
			m_state[2] = 'x';
			m_state_len = 4;
			return true;
		}

		/* Unicode codepoints < 0xffff can use \uhhhh format */
		if (t_source <= 0xffffU)
		{
			m_state[5] = '\\';
			m_state[4] = 'u';
			m_state_len = 6;
			return true;
		}

		/* Otherwise, store full codepoint using \Uhhhhhhhh format */
		if (t_source <= 0xffffffffU)
		{
			m_state[9] = '\\';
			m_state[8] = 'U';
			m_state_len = 10;
			return true;
		}

		MCUnreachable ();
		return false;
	}

	virtual bool HasData () const
	{
		return m_state_len > 0 || PrevFilter ()->HasData ();
	}

	/* Unimplemented -- they're not needed. */
	virtual void MarkText () { MCUnreachable (); }
	virtual uindex_t GetMarkedLength() { MCUnreachable (); }

private:
	/* The maximum length that one codepoint can be encoded to is 10
	 * characters (U+xxxxxxxx) */
	enum { kMCStackdirStringMaxEscape = 10 };

	/* The current state being stored. N.b. the state is stored at the
	 * start of the state array, but in reverse order. */
	uindex_t m_state_len;
	codepoint_t m_state[kMCStackdirStringMaxEscape];
};

static bool
MCStackdirFormatString (MCStringRef p_string, MCStringRef & r_literal)
{
	MCTextFilter *t_filter;
	MCStackdirStringEscapeFilter t_escape_filter;
	t_filter = MCTextFilterCreate (p_string, kMCStringOptionCompareExact);
	t_escape_filter.PlaceAfter (t_filter);

	/* Note that t_filter should be automatically cleaned up by the
	 * destructor of t_escape_filter when this function returns. */

	MCAutoStringRef t_result;
	/* UNCHECKED */ MCStringCreateMutable (2 + MCStringGetLength (p_string),
										   & t_result);

	/* Build escaped string */
	while (t_escape_filter.HasData())
	{
		if (!MCStringAppendCodepoint (*t_result,
									  t_escape_filter.GetNextCodepoint ()))
			return false;
	}

	/* Add delimiters at start and end */
	if (!(MCStringPrependCodepoint (*t_result, '"') &&
		  MCStringAppendCodepoint (*t_result, '"')))
		return false;

	r_literal = MCValueRetain (*t_result);
	return true;
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
		MCStringCreateWithCString (t_hexc, &t_hex);

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
	case kMCValueTypeCodeArray:
		return MCSTR ("array");
	default:
		MCUnreachable ();
	}
}
