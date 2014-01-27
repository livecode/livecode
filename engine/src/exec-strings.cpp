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

////////////////////////////////////////////////////////////////////////////////

MC_EXEC_DEFINE_EVAL_METHOD(Strings, ToLower, 2)
MC_EXEC_DEFINE_EVAL_METHOD(Strings, ToUpper, 2)
MC_EXEC_DEFINE_EVAL_METHOD(Strings, NumToChar, 2)
MC_EXEC_DEFINE_EVAL_METHOD(Strings, CharToNum, 2)
MC_EXEC_DEFINE_EVAL_METHOD(Strings, NumToByte, 2)
MC_EXEC_DEFINE_EVAL_METHOD(Strings, ByteToNum, 2)
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
MC_EXEC_DEFINE_EVAL_METHOD(Strings, IsAmongTheTokensOf, 3)
MC_EXEC_DEFINE_EVAL_METHOD(Strings, IsNotAmongTheTokensOf, 3)
MC_EXEC_DEFINE_EVAL_METHOD(Strings, IsAmongTheWordsOf, 3)
MC_EXEC_DEFINE_EVAL_METHOD(Strings, IsNotAmongTheWordsOf, 3)
MC_EXEC_DEFINE_EVAL_METHOD(Strings, IsAmongTheLinesOf, 3)
MC_EXEC_DEFINE_EVAL_METHOD(Strings, IsNotAmongTheLinesOf, 3)
MC_EXEC_DEFINE_EVAL_METHOD(Strings, IsAmongTheItemsOf, 3)
MC_EXEC_DEFINE_EVAL_METHOD(Strings, IsNotAmongTheItemsOf, 3)
MC_EXEC_DEFINE_EVAL_METHOD(Strings, ItemOffset, 4)
MC_EXEC_DEFINE_EVAL_METHOD(Strings, LineOffset, 4)
MC_EXEC_DEFINE_EVAL_METHOD(Strings, WordOffset, 4)
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
        MCStringsEvalNumToNativeChar(ctxt, p_codepoint, (MCStringRef&)r_character);
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

void MCStringsEvalLength(MCExecContext& ctxt, MCStringRef p_string, integer_t& r_length)
{
	// Ensure that the returned length is in codepoints
    MCRange t_cp_range, t_cu_range;
    t_cu_range = MCRangeMake(0, MCStringGetLength(p_string));
    /* UNCHECKED */ MCStringUnmapIndices(p_string, kMCDefaultCharChunkType, t_cu_range, t_cp_range);
    
    r_length = t_cp_range.length;
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
                    memcpy(&t_hexa_num[1], format, 2*sizeof(unichar_t));
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
	MCStringOptions t_compare_option = ctxt.GetCaseSensitive() ? kMCStringOptionCompareExact : kMCStringOptionCompareCaseless;
	r_result = MCStringContains(p_whole, p_part, t_compare_option);
}

void MCStringsEvalDoesNotContain(MCExecContext& ctxt, MCStringRef p_whole, MCStringRef p_part, bool& r_result)
{
	MCStringOptions t_compare_option = ctxt.GetCaseSensitive() ? kMCStringOptionCompareExact : kMCStringOptionCompareCaseless;
	r_result = !MCStringContains(p_whole, p_part, t_compare_option);
}

void MCStringsEvalBeginsWith(MCExecContext& ctxt, MCStringRef p_whole, MCStringRef p_part, bool& r_result)
{
	MCStringOptions t_compare_option = ctxt.GetCaseSensitive() ? kMCStringOptionCompareExact : kMCStringOptionCompareCaseless;
	r_result = MCStringBeginsWith(p_whole, p_part, t_compare_option);
}

void MCStringsEvalEndsWith(MCExecContext& ctxt, MCStringRef p_whole, MCStringRef p_part, bool& r_result)
{
	MCStringOptions t_compare_option = ctxt.GetCaseSensitive() ? kMCStringOptionCompareExact : kMCStringOptionCompareCaseless;
	r_result = MCStringEndsWith(p_whole, p_part, t_compare_option);
}

////////////////////////////////////////////////////////////////////////////////

void MCStringsEvalIsAmongTheTokensOf(MCExecContext& ctxt, MCStringRef p_token, MCStringRef p_string, bool& r_result)
{
	// IM-2012-08-24 Note - this relies on the old parser stuff, not sure if this
	// will be replaced or dropped

	MCCompareOptions t_options = ctxt.GetCaseSensitive() ? kMCCompareExact : kMCCompareCaseless;

	MCScriptPoint sp(p_string);
	Parse_stat ps = sp.nexttoken();
	while (ps != PS_ERROR && ps != PS_EOF)
	{
        if (MCStringIsEqualTo(p_token, sp.gettoken_stringref(), t_options))
		{
			r_result = true;
			return;
		}
		ps = sp.nexttoken();
	}

	r_result = false;
}

void MCStringsEvalIsNotAmongTheTokensOf(MCExecContext& ctxt, MCStringRef p_token, MCStringRef p_string, bool& r_result)
{
	MCStringsEvalIsAmongTheTokensOf(ctxt, p_token, p_string, r_result);
	r_result = !r_result;
}

bool MCStringsIterateWords(MCExecContext& ctxt, uindex_t& x_index, MCStringRef p_string, MCRange& r_word_range)
{
	const char_t *t_str_ptr = MCStringGetNativeCharPtr(p_string);
	uindex_t t_length = MCStringGetLength(p_string);
	if (x_index < t_length)
	{
		uindex_t t_word_start;

		while (x_index < t_length && isspace(t_str_ptr[x_index]))
			x_index++;

		if (x_index == t_length)
			return false;

		t_word_start = x_index;

		if (t_str_ptr[x_index] == '"')
		{
			x_index++;
			while (x_index < t_length && t_str_ptr[x_index] != '"' && t_str_ptr[x_index] != '\n')
				x_index++;
			if (x_index < t_length)
				x_index++;
		}
		else
			while (x_index < t_length && !isspace(t_str_ptr[x_index]))
				x_index++;

		r_word_range = MCRangeMake(t_word_start, x_index - t_word_start);
		return true;
	}

	return false;
}

bool MCStringsIterateChunks(MCExecContext& ctxt, uindex_t& x_index, MCStringRef p_string, Chunk_term p_chunk_type, MCRange& r_chunk_range)
{
	if (p_chunk_type == CT_WORD)
		return MCStringsIterateWords(ctxt, x_index, p_string, r_chunk_range);

	codepoint_t t_delim = p_chunk_type == CT_LINE ? ctxt.GetLineDelimiter() : ctxt.GetItemDelimiter();

	uindex_t t_length = MCStringGetLength(p_string);

	if (x_index < t_length)
	{
		uindex_t t_offset;
		if (MCStringFirstIndexOfChar(p_string, t_delim, x_index, kMCStringOptionCompareExact, t_offset))
		{
			r_chunk_range = MCRangeMake(x_index, t_offset - x_index);
			x_index = t_offset + 1;
		}
		else
		{
			r_chunk_range = MCRangeMake(x_index, t_length - x_index);
			x_index = t_length;
		}
		return true;
	}
	return false;
}

//////////

bool MCStringsIsAmongTheSplitChunksOf(MCExecContext& ctxt, MCStringRef p_item, MCStringRef p_string, Chunk_term p_chunk_type)
{
	uindex_t t_index = 0;
	MCRange t_range;
	MCStringOptions t_options = ctxt.GetCaseSensitive() ? kMCStringOptionCompareExact: kMCStringOptionCompareCaseless;
	while (MCStringsIterateChunks(ctxt, t_index, p_string, p_chunk_type, t_range))
	{
		if (MCStringSubstringIsEqualTo(p_string, t_range, p_item, t_options))
			return true;
	}
	return false;
}

void MCStringsEvalIsAmongTheWordsOf(MCExecContext& ctxt, MCStringRef p_word, MCStringRef p_string, bool& r_result)
{
	r_result = MCStringsIsAmongTheSplitChunksOf(ctxt, p_word, p_string, CT_WORD);
}

void MCStringsEvalIsNotAmongTheWordsOf(MCExecContext& ctxt, MCStringRef p_word, MCStringRef p_string, bool& r_result)
{
	r_result = !MCStringsIsAmongTheSplitChunksOf(ctxt, p_word, p_string, CT_WORD);
}


static bool MCStringsIsAmongTheChunksOfRange(MCExecContext& ctxt, MCStringRef p_chunk, MCStringRef p_string, Chunk_term p_chunk_type, MCStringOptions p_options, MCRange p_range)
{
	MCRange t_range;
	if (!MCStringFind(p_string, p_range, p_chunk, p_options, &t_range))
		return false;
    
	char_t t_delimiter;
	t_delimiter = p_chunk_type == CT_ITEM ? ctxt . GetItemDelimiter() : ctxt . GetLineDelimiter();
	
    // if there is no delimiter to the left then continue searching the string.
	if (t_range . offset != 0 &&
		MCStringGetNativeCharAtIndex(p_string, t_range . offset - 1) != t_delimiter)
		return MCStringsIsAmongTheChunksOfRange(ctxt, p_chunk, p_string, p_chunk_type, p_options, MCRangeMake(t_range . offset + t_range . length, p_range . length));
    
    // if there is no delimiter to the right then continue searching the string.
	if (t_range . offset + t_range . length != MCStringGetLength(p_string) &&
		MCStringGetNativeCharAtIndex(p_string, t_range . offset + t_range . length) != t_delimiter)
		return MCStringsIsAmongTheChunksOfRange(ctxt, p_chunk, p_string, p_chunk_type, p_options, MCRangeMake(t_range . offset + t_range . length, p_range . length));
    
	return true;
}

// MW-2013-01-21: The (current) behavior of 'is among' compares chunks if
//   the pattern is empty. Otherwise, it (essentially) searches for the
//   pattern in the string and ensures it has a delimiter either side.
//   i.e. "a,b" is among the items of "a,b,c" -- TRUE
//        "a,b" is among the items of "aa,b,cc" -- FALSE

static bool MCStringsIsAmongTheChunksOf(MCExecContext& ctxt, MCStringRef p_chunk, MCStringRef p_string, Chunk_term p_chunk_type)
{
	// First search for the pattern.
	MCStringOptions t_options;
	t_options = ctxt.GetCaseSensitive() ? kMCStringOptionCompareExact: kMCStringOptionCompareCaseless;
	
    return MCStringsIsAmongTheChunksOfRange(ctxt, p_chunk, p_string, p_chunk_type, t_options, MCRangeMake(0, UINDEX_MAX));
}

void MCStringsEvalIsAmongTheLinesOf(MCExecContext& ctxt, MCStringRef p_line, MCStringRef p_string, bool& r_result)
{
	if (MCValueIsEmpty(p_line))
		r_result = MCStringsIsAmongTheSplitChunksOf(ctxt, p_line, p_string, CT_LINE);
	else
		r_result = MCStringsIsAmongTheChunksOf(ctxt, p_line, p_string, CT_LINE);
}

void MCStringsEvalIsNotAmongTheLinesOf(MCExecContext& ctxt, MCStringRef p_line, MCStringRef p_string, bool& r_result)
{
	if (MCValueIsEmpty(p_line))
		r_result = !MCStringsIsAmongTheSplitChunksOf(ctxt, p_line, p_string, CT_LINE);
	else
		r_result = !MCStringsIsAmongTheChunksOf(ctxt, p_line, p_string, CT_LINE);
}

void MCStringsEvalIsAmongTheItemsOf(MCExecContext& ctxt, MCStringRef p_item, MCStringRef p_string, bool& r_result)
{
	if (MCValueIsEmpty(p_item))
		r_result = MCStringsIsAmongTheSplitChunksOf(ctxt, p_item, p_string, CT_ITEM);
	else
		r_result = MCStringsIsAmongTheChunksOf(ctxt, p_item, p_string, CT_ITEM);
}

void MCStringsEvalIsNotAmongTheItemsOf(MCExecContext& ctxt, MCStringRef p_item, MCStringRef p_string, bool& r_result)
{
	if (MCValueIsEmpty(p_item))
		r_result = !MCStringsIsAmongTheSplitChunksOf(ctxt, p_item, p_string, CT_ITEM);
	else
		r_result = !MCStringsIsAmongTheChunksOf(ctxt, p_item, p_string, CT_ITEM);
}

////////////////////////////////////////////////////////////////////////////////

bool MCStringsSplitChunkOffset(MCExecContext& ctxt, MCStringRef p_item, MCStringRef p_string, Chunk_term p_chunk_type, uindex_t p_skip, uindex_t& r_offset)
{
	uindex_t t_index = 0;
	r_offset = 1;
	MCRange t_range;
	MCStringOptions t_options = ctxt.GetCaseSensitive() ? kMCStringOptionCompareExact: kMCStringOptionCompareCaseless;
	while (MCStringsIterateChunks(ctxt, t_index, p_string, p_chunk_type, t_range))
	{
		if (p_skip > 0)
			p_skip--;
		else
		{
			if (ctxt.GetWholeMatches())
			{
				if (MCStringSubstringIsEqualTo(p_string, t_range, p_item, t_options))
					return true;
			}
			else
			{
				if (MCStringSubstringContains(p_string, t_range, p_item, t_options))
					return true;
			}
			r_offset++;
		}
	}
	return false;
}

void MCStringsEvalWordOffset(MCExecContext& ctxt, MCStringRef p_chunk, MCStringRef p_string, uindex_t p_start_offset, uindex_t& r_result)
{
	if (!MCStringsSplitChunkOffset(ctxt, p_chunk, p_string, CT_WORD, p_start_offset, r_result))
		r_result = 0;
}

void MCStringsEvalOffset(MCExecContext& ctxt, MCStringRef p_chunk, MCStringRef p_string, uindex_t p_start_offset, uindex_t& r_result)
{
	MCStringOptions t_options = ctxt.GetCaseSensitive() ? kMCStringOptionCompareExact: kMCStringOptionCompareCaseless;
	if (!MCStringFirstIndexOf(p_string, p_chunk, p_start_offset, t_options, r_result))
		r_result = 0;
	else
		r_result = r_result + 1 - p_start_offset;
}

// MW-2013-01-21: item/line offset do not currently operate on a 'split' basis.
//   Instead, they return the index of the chunk in which p_chunk starts and if
//   wholeMatches is true, then before and after the found range must be the del
//   or eos. e.g.
//     itemOffset("a,b", "aa,b,cc") => 1 if wholeMatches false, 0 otherwise
//     itemOffset("b,c", "a,b,c") => 2

bool MCStringsChunkOffset(MCExecContext& ctxt, MCStringRef p_item, MCStringRef p_string, Chunk_term p_chunk_type, uindex_t p_skip, uindex_t& r_offset)
{
	MCStringOptions t_options;
	t_options = ctxt.GetCaseSensitive() ? kMCStringOptionCompareExact : kMCStringOptionCompareCaseless;
	
	// Skip ahead to the first chunk of interest.
	uindex_t t_index;
	MCRange t_first_chunk_range;
	t_index = 0;
	t_first_chunk_range = MCRangeMake(0, MCStringGetLength(p_string));
	while(p_skip > 0 && MCStringsIterateChunks(ctxt, t_index, p_string, p_chunk_type, t_first_chunk_range))
		p_skip -= 1;
	
	// If we skip past the last chunk, we are done.
	if (p_skip > 0)
		return false;
	
	// If we can't find the chunk in the remainder of the string, we are done.
	MCRange t_range;
	if (!MCStringFind(p_string, MCRangeMake(t_index, MCStringGetLength(p_string) - t_index), p_item, t_options, &t_range))
		return false;
	
	// Work out the delimiter.
	char_t t_delimiter;
	t_delimiter = p_chunk_type == CT_ITEM ? ctxt . GetItemDelimiter() : ctxt . GetLineDelimiter();
	
	// If we are in wholeMatches mode, ensure the delimiter is either side.
	if (ctxt . GetWholeMatches())
	{
		if (t_range . offset > 0 &&
			MCStringGetNativeCharAtIndex(p_string, t_range . offset - 1) != t_delimiter)
			return false;
		if (t_range . offset + t_range . length < MCStringGetLength(p_string) &&
			MCStringGetNativeCharAtIndex(p_string, t_range . offset + t_range . length) != t_delimiter)
			return false;
	}
	
	// Now count the number of delimiters between the start of the first chunk
	// and the start of the found string.
	r_offset = 1 + MCStringCountChar(p_string, MCRangeMake(t_first_chunk_range . offset, t_range . offset - t_first_chunk_range . offset), t_delimiter, t_options);

	return true;
}

void MCStringsEvalItemOffset(MCExecContext& ctxt, MCStringRef p_chunk, MCStringRef p_string, uindex_t p_start_offset, uindex_t& r_result)
{
	if (!MCStringsChunkOffset(ctxt, p_chunk, p_string, CT_ITEM, p_start_offset, r_result))
		r_result = 0;
}

void MCStringsEvalLineOffset(MCExecContext& ctxt, MCStringRef p_chunk, MCStringRef p_string, uindex_t p_start_offset, uindex_t& r_result)
{
	if (!MCStringsChunkOffset(ctxt, p_chunk, p_string, CT_LINE, p_start_offset, r_result))
		r_result = 0;
}

////////////////////////////////////////////////////////////////////////////////

void MCStringsExecReplace(MCExecContext& ctxt, MCStringRef p_pattern, MCStringRef p_replacement, MCStringRef p_target)
{
	MCStringOptions t_options = ctxt.GetCaseSensitive() ? kMCStringOptionCompareExact: kMCStringOptionCompareCaseless;
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

bool MCRegexMatcher::match(MCStringRef s)
{
	return MCR_exec(compiled, s);
}

bool MCWildcardMatcher::compile(MCStringRef& r_error)
{
    // wildcard patterns are not compiled
    return true;
}

#define OPEN_BRACKET '['
#define CLOSE_BRACKET ']'

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

bool MCStringsExecWildcardMatch(MCStringRef p_source, MCStringRef p_pattern, bool casesensitive)
{
    MCAutoPointer<char> t_source, t_pattern;
    /* UNCHECKED */ MCStringConvertToCString(p_source, &t_source);
    /* UNCHECKED */ MCStringConvertToCString(p_pattern, &t_pattern);
	return MCStringsWildcardMatch(*t_source, MCStringGetLength(p_source), *t_pattern, MCStringGetLength(p_pattern), casesensitive);
}

bool MCWildcardMatcher::match(MCStringRef s)
{
	return MCStringsExecWildcardMatch(s, pattern, casesensitive);
}

void MCStringsExecFilterDelimited(MCExecContext& ctxt, MCStringRef p_source, bool p_without, char_t p_delimiter, MCPatternMatcher *p_matcher, MCStringRef &r_result)
{
	uint32_t t_length = MCStringGetLength(p_source);
	if (t_length == 0)
        MCStringCopy(kMCEmptyString, r_result);

	uint4 offset = 0;
    MCAutoStringRef t_output;
    MCStringCreateMutable(0, &t_output);

	// OK-2010-01-11: Bug 7649 - Filter command was incorrectly removing empty lines.
	// Now ignores delimiter for matching but includes it in the append.

	uindex_t t_return_offset = 0;
    uindex_t t_last_offset = 0;
	bool t_found = true;
    bool t_success = true;
	while (t_found && t_success)
	{
        MCAutoStringRef t_line;
		t_found = MCStringFirstIndexOfChar(p_source, p_delimiter, t_last_offset, kMCCompareCaseless, t_return_offset);
		if (!t_found) //last line or item
            t_success = MCStringCopySubstring(p_source, MCRangeMake(t_last_offset, t_length - t_last_offset), &t_line);
		else
            t_success = MCStringCopySubstring(p_source, MCRangeMake(t_last_offset, t_return_offset - t_last_offset), &t_line);
        
        if (t_success && p_matcher -> match(*t_line) != p_without)
		{
			if (!t_found)
                t_success = MCStringAppend(*t_output, *t_line);
			else
                t_success = MCStringAppendSubstring(*t_output, p_source, MCRangeMake(t_last_offset, 1 + t_return_offset - t_last_offset));
		}
		t_last_offset = t_return_offset + 1;
	}
	
    if (!t_success)
    {
        // IM-2013-07-26: [[ Bug 10774 ]] if filterlines fails throw a "no memory" error
        ctxt . LegacyThrow(EE_NO_MEMORY);
        MCStringCopy(kMCEmptyString, r_result);
    }
    else if (MCStringGetLength(*t_output) != 0)
        MCStringCopy(*t_output, r_result);
	else
        MCStringCopy(kMCEmptyString, r_result);
}

void MCStringsExecFilterWildcard(MCExecContext& ctxt, MCStringRef p_source, MCStringRef p_pattern, bool p_without, bool p_lines, MCStringRef &r_result)
{
    // Create the pattern matcher
	MCPatternMatcher *matcher;
    matcher = new MCWildcardMatcher(p_pattern, ctxt . GetCaseSensitive());
    
    MCStringsExecFilterDelimited(ctxt, p_source, p_without, p_lines ? ctxt . GetLineDelimiter() : ctxt . GetItemDelimiter(), matcher, r_result);
}

void MCStringsExecFilterRegex(MCExecContext& ctxt, MCStringRef p_source, MCStringRef p_pattern, bool p_without, bool p_lines, MCStringRef &r_result)
{
	// Create the pattern matcher
	MCPatternMatcher *matcher;
    matcher = new MCRegexMatcher(p_pattern, ctxt . GetCaseSensitive());
    
    MCAutoStringRef t_regex_error;
    if (!matcher -> compile(&t_regex_error))
    {
        ctxt . LegacyThrow(EE_MATCH_BADPATTERN);
        return;
    }
    
    MCStringsExecFilterDelimited(ctxt, p_source, p_without, p_lines ? ctxt . GetLineDelimiter() : ctxt . GetItemDelimiter(), matcher, r_result);
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
