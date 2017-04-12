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

inline void MCU_skip_spaces(MCSpan<const char>& x_span)
{
	while (!x_span.empty() && isspace(uint8_t(x_span[0])))
	{
		++x_span;
	}
}

static integer_t MCU_strtol(MCSpan<const char> p_chars,
                            char c,
                            bool reals,
                            bool octals,
                            bool& done,
                            MCSpan<const char>& r_remainder)
{
	done = false;
	MCU_skip_spaces(p_chars);
	if (p_chars.empty())
		return 0;
	bool negative = false;
	integer_t value = 0;
	if (*p_chars == '-' || *p_chars == '+')
	{
		negative = *p_chars == '-';
		++p_chars;
	}
	if (p_chars.empty())
		return 0;
	uinteger_t startlength = p_chars.size();
	uint16_t base = 10;
	if (!p_chars.empty() && *p_chars == '0')
    {
	    if (p_chars.size() > 2 && MCNativeCharFold(*(p_chars + 1)) == 'x')
		{
			base = 16;
			p_chars += 2;
		}
		else
        {
			if (octals)
			{
				base = 8;
				p_chars += 1;
			}
        }
    }
	while (!p_chars.empty())
	{
		if (isdigit((uint8_t)*p_chars))
		{
			integer_t v = *p_chars - '0';
			if (base < 16 && value > INTEGER_MAX / base - v)  // prevent overflow
				return 0;
			value *= base;
			value += v;
		}
		else
			if (isspace((uint8_t)*p_chars))
			{
				MCU_skip_spaces(p_chars);
				if (!p_chars.empty() && *p_chars == c)
				{
					++p_chars;
				}
				break;
			}
			else
				if (!p_chars.empty() && c && *p_chars == c)
				{
					++p_chars;
					break;
				}
				else
					if (*p_chars == '.')
					{
						if (startlength > 1)
						{
							if (reals)
							{
								// MDW-2013-06-09: [[ Bug 10964 ]] Round integral values to nearest
								//   (consistent with getuint4() and round()).
								if (*(p_chars+1) > '4')
								{
									value++;
								}
								do
								{
									++p_chars;
								}
								while (!p_chars.empty() && isdigit((uint8_t)*p_chars));
							}
							else
								do
								{
									++p_chars;
								}
								while (!p_chars.empty() && *p_chars == '0');
							if (p_chars.empty())
								break;
							if (*p_chars == c || isspace((uint8_t)*p_chars))
							{
								++p_chars;
								break;
							}
						}
						return 0;
					}
					else
					{
						char t_char = MCNativeCharFold(*p_chars);
						if (base == 16 && t_char >= 'a' && t_char <= 'f')
						{
							value *= base;
							value += t_char - 'a' + 10;
						}
						else
							return 0;
					}
		++p_chars;
	}
	if (negative)
		value = -value;
	MCU_skip_spaces(p_chars);
	done = true;
	r_remainder = p_chars;
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

	MCU_skip_spaces(p_chars);
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

	p_chars += (t_end - t_buff);
	MCU_skip_spaces(p_chars);
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
