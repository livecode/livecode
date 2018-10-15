/* Copyright (C) 2015 LiveCode Ltd.
 
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

#include "foundation.h"
#include "foundation-private.h"
#include "foundation-auto.h"

#define R8L 384

static MCSpan<const char>::const_iterator
skip_spaces(MCSpan<const char>::const_iterator p_start,
            MCSpan<const char>::const_iterator p_end)
{
    auto t_iter = p_start;
    while (t_iter != p_end && isspace(uint8_t(*t_iter)))
        ++t_iter;
    return t_iter;
}

static MCSpan<const char>
skip_spaces(MCSpan<const char> p_span)
{
    auto t_iter = skip_spaces(p_span.cbegin(), p_span.cend());
    return p_span.subspan(t_iter - p_span.cbegin());
}

static integer_t MCU_strtol(MCSpan<const char> p_char_span,
                            char c,
                            bool reals,
                            bool octals,
                            bool& done,
                            MCSpan<const char>& r_remainder)
{
	done = false;
    MCSpan<const char>::const_iterator t_start = p_char_span.cbegin(),
        t_end = p_char_span.cend(),
        t_chars = t_start;
    t_chars = skip_spaces(t_chars, t_end);
	if (t_chars == t_end)
		return 0;
	bool negative = false;
	integer_t value = 0;
	if (*t_chars == '-' || *t_chars == '+')
	{
		negative = *t_chars == '-';
		++t_chars;
	}
	if (t_chars == t_end)
		return 0;
	uinteger_t startlength = t_end - t_chars;
	uint16_t base = 10;
	if (t_chars != t_end && *t_chars == '0')
    {
	    if (t_end - t_chars > 2 && MCNativeCharFold(*(t_chars + 1)) == 'x')
		{
			base = 16;
			t_chars += 2;
		}
		else
        {
			if (octals)
			{
				base = 8;
				t_chars += 1;
			}
        }
    }
	while (t_chars != t_end)
	{
		if (isdigit((uint8_t)*t_chars))
		{
			integer_t v = *t_chars - '0';
			if (base < 16 && value > INTEGER_MAX / base - v)  // prevent overflow
				return 0;
			value *= base;
			value += v;
		}
		else
			if (isspace((uint8_t)*t_chars))
			{
                t_chars = skip_spaces(t_chars, t_end);
				if (t_chars != t_end && *t_chars == c)
				{
					++t_chars;
				}
				break;
			}
			else
				if (t_chars != t_end && c && *t_chars == c)
				{
					++t_chars;
					break;
				}
				else
					if (*t_chars == '.')
					{
						if (startlength > 1)
						{
							if (reals)
							{
								// MDW-2013-06-09: [[ Bug 10964 ]] Round integral values to nearest
								//   (consistent with getuint4() and round()).
								if (*(t_chars+1) > '4')
								{
									value++;
								}
								do
								{
									++t_chars;
								}
								while (t_chars != t_end && isdigit((uint8_t)*t_chars));
							}
							else
								do
								{
									++t_chars;
								}
								while (t_chars != t_end && *t_chars == '0');
							if (t_chars == t_end)
								break;
							if (*t_chars == c || isspace((uint8_t)*t_chars))
							{
								++t_chars;
								break;
							}
						}
						return 0;
					}
					else
					{
						char t_char = MCNativeCharFold(*t_chars);
						if (base == 16 && t_char >= 'a' && t_char <= 'f')
						{
							value *= base;
							value += t_char - 'a' + 10;
						}
						else
							return 0;
					}
		++t_chars;
	}
	if (negative)
		value = -value;
	t_chars = skip_spaces(t_chars, t_end);
	done = true;
	r_remainder = p_char_span.subspan(t_chars - t_start);
	return value;
}

static real64_t MCU_strtor8(MCSpan<const char> p_chars,
                            bool convertoctals,
                            bool & r_done)
{
	/* Attempt to convert as an integer */
	{
		MCSpan<const char> t_remainder;
		bool t_done = false;
		integer_t t_int_val = MCU_strtol(p_chars, '\0', false, convertoctals,
		                                 t_done, t_remainder);
		if (t_done)
		{
			r_done = t_remainder.empty();
			return t_int_val;
		}
	}

	r_done = false;

	p_chars = skip_spaces(p_chars);
	if (p_chars.empty())
		return 0;

	if (p_chars.size() > 1)
	{
		if ((MCNativeCharFold(p_chars[1]) == 'x' &&
		     (p_chars.size() == 2 || !isxdigit(p_chars[2]))) ||
		    (p_chars[1] == '+' || p_chars[1] == '-'))
		{
			return 0;
		}
	}

	/* We need a null-terminated buffer to pass to strtod().  Assume
	 * that all 64-bit real numbers can be represented unambiguously
	 * using at most R8L characters. */
	char t_buff[R8L + 1];
	if (p_chars.size() > R8L)
		return 0;
	memcpy(t_buff, p_chars.data(), p_chars.size());
	t_buff[p_chars.size()] = '\0';

	char *t_end = nil;
	real64_t t_value = strtod(t_buff, &t_end);
	if (t_end == t_buff)
		return 0;

	p_chars = skip_spaces(p_chars.subspan(t_end - t_buff));
	if (!p_chars.empty())
		return 0;

	r_done = true;
	return t_value;
}

MC_DLLEXPORT_DEF
bool MCTypeConvertStringToLongInteger(MCStringRef p_string, integer_t& r_converted)
{
    if (!MCStringCanBeNative(p_string))
        return false;
    
    MCAutoStringRefAsCString t_cstring;
	if (!t_cstring . Lock(p_string))
		return false;

	bool t_done = false;
	MCSpan<const char> t_remainder;
	integer_t t_converted = MCU_strtol(t_cstring.Span(), '\0', false, false,
	                                   t_done, t_remainder);
	if (t_done && t_remainder.empty())
	{
		r_converted = t_converted;
		return true;
	}
	else
	{
		return false;
	}
}

MC_DLLEXPORT_DEF
bool MCTypeConvertStringToReal(MCStringRef p_string, real64_t& r_converted, bool p_convert_octals)
{
    if (!MCStringCanBeNative(p_string))
        return false;
    
    MCAutoStringRefAsCString t_cstring;
	if (!t_cstring . Lock(p_string))
		return false;

	bool t_done = false;
	real64_t t_value = MCU_strtor8(t_cstring.Span(), p_convert_octals, t_done);
    
    if (t_done)
        r_converted = t_value;
    
    return t_done;
}


MC_DLLEXPORT_DEF
bool MCTypeConvertDataToReal(MCDataRef p_data, real64_t& r_converted, bool p_convert_octals)
{
    bool t_done = false;
    real64_t t_value = MCU_strtor8(MCDataGetSpan<const char>(p_data),
                                   p_convert_octals, t_done);
    
    if (t_done)
        r_converted = t_value;
    
    return t_done;
}

MC_DLLEXPORT_DEF
bool MCTypeConvertStringToBool(MCStringRef p_string, bool& r_converted)
{
    if (MCStringIsEqualTo(p_string, kMCTrueString, kMCStringOptionCompareCaseless))
    {
        r_converted = true;
        return true;
    }
    
    if (MCStringIsEqualTo(p_string, kMCFalseString, kMCStringOptionCompareCaseless))
    {
        r_converted = false;
        return true;
    }
    
    return false;
}
