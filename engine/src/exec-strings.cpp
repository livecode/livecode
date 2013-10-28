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

#include "scriptpt.h"
#include "util.h"

#include "exec.h"

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
MC_EXEC_DEFINE_EXEC_METHOD(Strings, Replace, 3)
MC_EXEC_DEFINE_EXEC_METHOD(Strings, Filter, 4)

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
		MCStringLowercase(*t_string) &&
		MCStringCopy(*t_string, r_lower))
		return;

	ctxt.Throw();
}

void MCStringsEvalToUpper(MCExecContext& ctxt, MCStringRef p_string, MCStringRef& r_upper)
{
	MCAutoStringRef t_string;
	if (MCStringMutableCopy(p_string, &t_string) &&
		MCStringUppercase(*t_string) &&
		MCStringCopy(*t_string, r_upper))
		return;

	ctxt.Throw();
}

////////////////////////////////////////////////////////////////////////////////

void MCStringsEvalNumToChar(MCExecContext& ctxt, integer_t p_codepoint, MCStringRef& r_character)
{
	if (!ctxt . GetUseUnicode())
	{
		char_t t_codepoint_as_char;
		t_codepoint_as_char = (char_t)p_codepoint;
		if (MCStringCreateWithNativeChars(&t_codepoint_as_char, 1, r_character))
			return;
	}
	else
	{
		unichar_t t_codepoint_as_unichar;
		t_codepoint_as_unichar = (unichar_t)p_codepoint;
		if (MCStringCreateWithNativeChars((char_t *)&t_codepoint_as_unichar, 2, r_character))
			return;
	}

	ctxt . Throw();
}

void MCStringsEvalCharToNum(MCExecContext& ctxt, MCStringRef p_character, MCValueRef& r_codepoint)
{
	if (!ctxt . GetUseUnicode())
	{
		if (MCStringGetLength(p_character) >= 1)
		{
			MCNumberRef t_codepoint;
			if (MCNumberCreateWithInteger(MCStringGetNativeCharAtIndex(p_character, 0), t_codepoint))
			{
				r_codepoint = t_codepoint;
				return;
			}
			ctxt . Throw();
			return;
		}
	}
	else
	{
		if (MCStringGetLength(p_character) >= 2)
		{
			char_t t_first_byte, t_second_byte;
			t_first_byte = MCStringGetNativeCharAtIndex(p_character, 0);
			t_second_byte = MCStringGetNativeCharAtIndex(p_character, 1);

			integer_t t_codepoint;
#ifdef __LITTLE_ENDIAN__
			t_codepoint = t_first_byte | (t_second_byte << 8);
#else
			t_codepoint = t_second_byte | (t_first_byte << 8);
#endif
			MCNumberRef t_codepoint_as_number;
			if (MCNumberCreateWithInteger(t_codepoint, t_codepoint_as_number))
			{
				r_codepoint = (MCValueRef)t_codepoint_as_number;
				return;
			}

			ctxt . Throw();
			return;
		}
	}
	r_codepoint = MCValueRetain(kMCEmptyString);
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
	if (MCStringGetLength(p_byte) == 1)
	{
		r_codepoint = MCStringGetNativeCharAtIndex(p_byte, 0);
		return;
	}

	ctxt . LegacyThrow(EE_BYTETONUM_BADSOURCE);
}

////////////////////////////////////////////////////////////////////////////////

void MCStringsEvalLength(MCExecContext& ctxt, MCStringRef p_string, integer_t& r_length)
{
	r_length = MCStringGetLength(p_string);
}

////////////////////////////////////////////////////////////////////////////////

#include "regex.h"

index_t MCregexfrontier = 0;

/*
bool MCStringsGetCachedPattern(MCStringRef p_pattern, regexp*& r_compiled)
{
    for (uinteger_t i = 0; i < PATTERN_CACHE_SIZE; i++)
        if (MCregexpatterns[i] != nil && MCStringIsEqualTo(p_pattern, MCregexpatterns[i], kMCStringOptionCompareExact))
        {
            r_compiled = MCregexcache[i];
            return true;
        }
    return false;
}

bool MCStringsCachePattern(MCStringRef p_pattern, regexp* p_compiled)
{
    MCStringRef t_string;
    if (!MCStringCopy(p_pattern, t_string))
        return false;
    
    MCValueRelease(MCregexpatterns[MCregexfrontier]);
    MCR_free(MCregexcache[MCregexfrontier]);
    MCregexpatterns[MCregexfrontier] = t_string;
    MCregexcache[MCregexfrontier] = p_compiled;
    
    MCregexfrontier = (MCregexfrontier + 1) % PATTERN_CACHE_SIZE;
    
    return true;
}
*/

bool MCStringsCompilePattern(MCStringRef p_pattern, regexp*& r_compiled, bool casesensitive)
{
    r_compiled = MCR_compile(p_pattern, casesensitive);
    return r_compiled != nil;    
}

//////////

void MCStringsEvalMatchText(MCExecContext& ctxt, MCStringRef p_string, MCStringRef p_pattern, MCStringRef* r_results, uindex_t p_result_count, bool& r_match)
{
    regexp* t_compiled = nil;
//    if (!MCStringsGetCachedPattern(p_pattern, t_compiled))
//    {
        if (!MCStringsCompilePattern(p_pattern, t_compiled, true /* casesensitive */))
        {
            ctxt.LegacyThrow(EE_MATCH_BADPATTERN);
            return;
        }
//        if (!MCStringsCachePattern(p_pattern, t_compiled))
//        {
//            ctxt.Throw();
//            return;
//        }
//    }
    
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
//    if (!MCStringsGetCachedPattern(p_pattern, t_compiled))
//    {
        if (!MCStringsCompilePattern(p_pattern, t_compiled, true /* casesensitive */))
        {
            ctxt.LegacyThrow(EE_MATCH_BADPATTERN);
            return;
        }
//        if (!MCStringsCachePattern(p_pattern, t_compiled))
//        {
//            ctxt.Throw();
//            return;
//        }
//    }
    
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
//    if (!MCStringsGetCachedPattern(p_pattern, t_compiled))
//    {
        if (!MCStringsCompilePattern(p_pattern, t_compiled, true /* casesensitive */))
        {
            ctxt.LegacyThrow(EE_REPLACETEXT_BADPATTERN);
            return;
        }
//        if (!MCStringsCachePattern(p_pattern, t_compiled))
//        {
//            ctxt.Throw();
//            return;
//        }
//    }
    
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

	const char *format = (const char*) MCStringGetNativeCharPtr(p_format);

	t_success = MCStringCreateMutable(0, &t_result);

	while (t_success && *format != '\0')
	{
		MCValueRef t_value = nil;
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
					char buffer[3];
					char *end;
					memcpy(buffer, format, 2);
					buffer[2] = '\0';
					t_result_char = (char)strtoul(buffer, (char **)&end, 16);
					format += end - buffer - 1;
				}
				break;
			default:
				if (isdigit((uint1)*format))
				{
					const char *sptr = format;
					while (isdigit((uint1)*format) && format - sptr < 3)
						t_result_char = (t_result_char << 3) + (*format++ - '0');
					format--;
				}
				break;
			}
			t_success = MCStringAppendNativeChars(*t_result, &t_result_char, 1);
			format++;
			continue;
		}
		else if (*format != '%')
		{
			const char *startptr = format;
			while (*format && *format != '%' && *format != '\\')
				format++;
			t_success = MCStringAppendNativeChars(*t_result, (const char_t*)startptr, format - startptr);
			continue;
		}
		else if (format[1] == '%')
		{
			t_success = MCStringAppendNativeChars(*t_result, (const char_t*)format, 1);
			format += 2;
			continue;
		}
		else
		{
			char newFormat[40];
			char *dptr = newFormat;

			integer_t width = 0;
			uinteger_t precision = 0;

			bool useShort = false;
			uinteger_t whichValue = PTR_VALUE;
			const char *end;

			*dptr++ = *format++;
			while (*format == '-' || *format == '#' || *format == '0'
				|| *format == ' ' || *format == '+')
				*dptr++ = *format++;
			if (isdigit((uint1)*format))
			{
				width = strtol(format, (char **)&end, 10);
				format = end;
			}
			else if (*format == '*')
			{
				if (!PopParam(p_params, p_param_count, t_value) || !ctxt.ConvertToInteger(t_value, width))
				{
					ctxt.LegacyThrow(EE_FORMAT_BADSOURCE, t_value);
					return;
				}
				t_value = nil;
				format++;
			}
			if (width != 0)
			{
				sprintf(dptr, "%d", width);
				while (*++dptr)
					;
			}
			if (*format == '.')
				*dptr++ = *format++;
			if (isdigit((uint1)*format))
			{
				precision = strtoul(format, (char **)&end, 10);
				format = end;
			}
			else if (*format == '*')
			{
				if (!PopParam(p_params, p_param_count, t_value) || !ctxt.ConvertToUnsignedInteger(t_value, precision))
				{
					ctxt.LegacyThrow(EE_FORMAT_BADSOURCE, t_value);
					return;
				}
				t_value = nil;
				format++;
			}
			if (precision != 0)
			{
				sprintf(dptr, "%d", precision);
				while (*++dptr)
					;
			}
			if (*format == 'l')
				format++;
			else
				if (*format == 'h')
				{
					useShort = true;
					*dptr++ = *format++;
				}
			*dptr++ = *format;
			*dptr = 0;
			switch (*format)
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
			format++;
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

				t_success = MCStringAppendFormat(*t_result, newFormat, MCStringGetCString(*t_string));
				break;
			}
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
				t_expression_end = t_start + 2;
				while (t_expression_end + 1 < t_length)
				{
					if (MCStringGetCharAtIndex(*t_merged, t_expression_end) == ']' &&
						MCStringGetCharAtIndex(*t_merged, t_expression_end + 1) == ']')
					{
						t_expression_end += 2;
						t_match = true;
						t_is_expression = true;
						break;
					}
					else
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
					ctxt.GetHandler()->eval(t_ctxt, *t_expression, &t_value);
				}
				else
				{
					ctxt.GetHandler()->doscript(t_ctxt, *t_expression);
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
		if (MCStringIsEqualToOldString(p_token, sp.gettoken_oldstring(), t_options))
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
	
	MCRange t_range;
	if (!MCStringFind(p_string, MCRangeMake(0, MCStringGetLength(p_string)), p_chunk, t_options, &t_range))
		return false;
		
	char_t t_delimiter;
	t_delimiter = p_chunk_type == CT_ITEM ? ctxt . GetItemDelimiter() : ctxt . GetLineDelimiter();
	
	if (t_range . offset != 0 &&
		MCStringGetNativeCharAtIndex(p_string, t_range . offset - 1) != t_delimiter)
		return false;
		
	if (t_range . offset + t_range . length != MCStringGetLength(p_string) &&
		MCStringGetNativeCharAtIndex(p_string, t_range . offset + t_range . length) != t_delimiter)
		return false;
		
	return true;
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
	if (!MCStringFind(p_string, MCRangeMake(t_first_chunk_range . offset, MCStringGetLength(p_string) - t_first_chunk_range . offset), p_item, t_options, &t_range))
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

#define OPEN_BRACKET '['
#define CLOSE_BRACKET ']'

bool match(const char *s, uindex_t s_length, const char *p, uindex_t p_length, bool casesensitive)
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
						return ok ? match(s, s_length - s_index, p, p_length - p_index, casesensitive) : false;
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
					if (match(s++, s_length - s_index++, p, p_length - p_index, casesensitive))
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

bool MCStringsExecFilterMatch(MCStringRef p_source, MCStringRef p_pattern, MCStringOptions p_options)
{
	return match(MCStringGetCString(p_source), MCStringGetLength(p_source), MCStringGetCString(p_pattern), MCStringGetLength(p_pattern), p_options != kMCCompareCaseless); 
}

MCStringRef MCStringsExecFilterLines(MCStringRef p_source, MCStringRef p_pattern, bool p_without, MCStringOptions p_options)
{
	uint32_t t_length = MCStringGetLength(p_source);
	if (t_length == 0)
		return kMCEmptyString;

	uint4 offset = 0;
	MCStringRef t_output;
	MCStringCreateMutable(0, t_output);

	// OK-2010-01-11: Bug 7649 - Filter command was incorrectly removing empty lines.
	// Now ignores line delimiter for matching but includes it in the append.

	uindex_t t_return_offset = 0;
	uindex_t t_last_offset = 0;
	MCStringRef t_line;
	bool t_found = true;
	while (t_found)
	{
		t_found = MCStringFirstIndexOfChar(p_source, '\n', t_last_offset, kMCCompareCaseless, t_return_offset);
		if (!t_found) //last line
			MCStringCopySubstring(p_source, MCRangeMake(t_last_offset, t_length - t_last_offset), t_line);
		else
			MCStringCopySubstring(p_source, MCRangeMake(t_last_offset, t_return_offset - t_last_offset), t_line);
		
		if (MCStringsExecFilterMatch(t_line, p_pattern, p_options) != p_without)
		{
			if (!t_found)
				MCStringAppend(t_output, t_line);
			else
				MCStringAppendSubstring(t_output, p_source, MCRangeMake(t_last_offset, 1 + t_return_offset - t_last_offset)); 
		}
		t_last_offset = t_return_offset + 1;
	}
	
	if (MCStringGetLength(t_output) != 0)
		return t_output;
	else
		return kMCEmptyString;
}

void MCStringsExecFilter(MCExecContext& ctxt, MCStringRef p_source, MCStringRef p_pattern, bool p_without, MCStringRef &r_result)
{
	MCStringCopy(MCStringsExecFilterLines(p_source, p_pattern, p_without, ctxt . GetCaseSensitive() ? kMCStringOptionCompareExact: kMCStringOptionCompareCaseless), r_result);
}
////////////////////////////////////////////////////////////////////////////////
