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
 * Strings
 * ================================================================ */

enum kMCStackdirStringChar
{
	kMCStackdirStringCharStringDelimiter = 0x22, /* " */
	kMCStackdirStringCharEscapeCharacter = 0x5c, /* \ */
	kMCStackdirStringCharSpace = 0x20,
	kMCStackdirStringCharCarriageReturn = 0x0e,
	kMCStackdirStringCharLineFeed = 0x0a,
	kMCStackdirStringCharEscapeLineFeed = 0x6e,  /* n */
	kMCStackdirStringCharEscapeCarriageReturn = 0x72, /* r */
	kMCStackdirStringCharEscapeByte = 0x78, /* x */
	kMCStackdirStringCharEscapeShort = 0x75, /* u */
	kMCStackdirStringCharEscapeLong = 0x55, /* U */
	kMCStackdirStringCharDigitZero = 0x30,
	kMCStackdirStringCharUppercaseA = 0x41,
};

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
		if (t_source == kMCStackdirStringCharStringDelimiter ||
			t_source == kMCStackdirStringCharEscapeCharacter )
		{
			m_state[1] = kMCStackdirStringCharEscapeCharacter;
			m_state[0] = t_source;
			m_state_len = 2;
			return true;
		}

		/* Unicode categories L, M, N, P, S and the space character
		 * U+0020 can be passed through directly. */
		uint32_t t_mask = U_GET_GC_MASK (t_source);
		if (t_source == kMCStackdirStringCharSpace ||
			t_mask & (U_GC_L_MASK | U_GC_M_MASK | U_GC_N_MASK |
					  U_GC_P_MASK | U_GC_S_MASK))
		{
			m_state[0] = t_source;
			m_state_len = 1;
			return true;
		}

		/* The newline and carriage return characters have special
		 * escape sequences */
		if (t_source == kMCStackdirStringCharLineFeed)
		{
			m_state[1] = kMCStackdirStringCharEscapeCharacter;
			m_state[0] = kMCStackdirStringCharEscapeLineFeed;
			m_state_len = 2;
			return true;
		}

		if (t_source == kMCStackdirStringCharCarriageReturn)
		{
			m_state[1] = kMCStackdirStringCharEscapeCharacter;
			m_state[0] = kMCStackdirStringCharEscapeCarriageReturn;
			m_state_len = 2;
			return true;
		}

		/* Place codepoint into state array in hexadecimal format */
		for (uindex_t i = 0; i < sizeof (codepoint_t); ++i)
		{
			codepoint_t t_nibble = (t_source >> (4*i)) & 0xf;
			if (t_nibble < 10)
			{
				m_state[i] = kMCStackdirStringCharDigitZero + t_nibble;
			}
			else
			{
				m_state[i] = kMCStackdirStringCharUppercaseA + (t_nibble - 10);
			}
		}

		/* Unicode codepoints < 0xff can use \xhh format */
		if (t_source <= 0xffU)
		{
			m_state[3] = kMCStackdirStringCharEscapeCharacter;
			m_state[2] = kMCStackdirStringCharEscapeByte;
			m_state_len = 4;
			return true;
		}

		/* Unicode codepoints < 0xffff can use \uhhhh format */
		if (t_source <= 0xffffU)
		{
			m_state[5] = kMCStackdirStringCharEscapeCharacter;
			m_state[4] = kMCStackdirStringCharEscapeShort;
			m_state_len = 6;
			return true;
		}

		/* Otherwise, store full codepoint using \Uhhhhhhhh format */
		if (t_source <= 0xffffffffU)
		{
			m_state[9] = kMCStackdirStringCharEscapeCharacter;
			m_state[8] = kMCStackdirStringCharEscapeLong;
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
	if (!MCStringPrependCodepoint (*t_result,
								   kMCStackdirStringCharStringDelimiter))
		return false;
	if (!MCStringAppendCodepoint (*t_result,
								  kMCStackdirStringCharStringDelimiter))
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

		if (t_codepoint >= 0x30 && t_codepoint <= 0x39) continue; /* 0-9 */
		if (t_codepoint >= 0x41 && t_codepoint <= 0x5A) continue; /* A-Z */
		if (t_codepoint >= 0x61 && t_codepoint <= 0x7A) continue; /* a-z */
		if (t_codepoint == 0x5F) continue; /* _ */
		if (t_codepoint == 0x2D) continue; /* - */
		if (t_codepoint == 0x2E) continue; /* . */

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
 * Main entry points
 * ================================================================ */

bool
MCStackdirFormatLiteral (MCValueRef p_value, MCStringRef & r_literal)
{
	switch (MCValueGetTypeCode (p_value))
	{
	case kMCValueTypeCodeName:
		return MCStackdirFormatName ((MCNameRef) p_value, r_literal);
	case kMCValueTypeCodeString:
		return MCStackdirFormatString ((MCStringRef) p_value, r_literal);
	case kMCValueTypeCodeArray:
		return MCSTR ("array");
	default:
		MCUnreachable ();
	}
}
