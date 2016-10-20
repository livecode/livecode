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

inline void MCU_skip_spaces(const char *&sptr, uinteger_t &l)
{
	while (l && isspace((uint8_t)*sptr))
	{
		sptr++;
		l--;
	}
}

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
                            MCSpan<const char> r_remainder)
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

static integer_t MCU_strtol(const char *sptr, uindex_t &l, int8_t c, bool &done,
                            bool reals, bool octals)
{
	MCSpan<const char> t_remainder;
	integer_t t_result = MCU_strtol(MCMakeSpan(sptr, l), c, reals, octals,
	                                done, t_remainder);
	l = t_remainder.size();
	return t_result;
}

static real64_t MCU_strtor8(const char *&r_str, uindex_t &r_len, int8_t p_delim,  bool &r_done, bool convertoctals)
{
    const char *sptr = r_str;
    uindex_t l = r_len;
    bool done;
    integer_t i = MCU_strtol(sptr, l, p_delim, done, false, convertoctals);
    if (done)
    {
        r_done = done;
        return i;
    }
    sptr = r_str;
    l = MCMin(R8L - 1U, strnlen(sptr, r_len));
    MCU_skip_spaces(sptr, l);
    // bugs in MSL means we need to check these things
    // MW-2006-04-21: [[ Purify ]] This was incorrect - we need to ensure l > 1 before running most
    //   of these tests.
    if (l == 0 || (l > 1 && (((MCNativeCharFold((uint8_t)sptr[1]) == 'x' && (l == 2 || !isxdigit((uint8_t)sptr[2])))
                              || (sptr[1] == '+' || sptr[1] == '-')))))
    {
        r_done = false;
        return 0;
    }

    char buff[R8L];
    memcpy(buff, sptr, l);
    buff[l] = '\0';
    const char *newptr;
    
    real64_t t_value;
    t_value = strtod((char *)buff, (char **)&newptr);
    if (newptr == buff)
    {
        r_done = false;
        return 0;
    }
    
    l = buff + l - newptr;
    MCU_skip_spaces(newptr, l);
    if (l != 0)
    {
        r_done = false;
        return 0;
    }
    
    r_done = true;
    return t_value;
    
}

MC_DLLEXPORT_DEF
bool MCTypeConvertStringToLongInteger(MCStringRef p_string, integer_t& r_converted)
{
    if (!MCStringCanBeNative(p_string))
        return false;
    
    MCAutoStringRefAsCString t_cstring;
    t_cstring . Lock(p_string);
    
    bool t_done;
    uindex_t t_length = strlen(*t_cstring);
    r_converted = MCU_strtol(*t_cstring, t_length, '\0', t_done, false, false);
	return t_done;
}

MC_DLLEXPORT_DEF
bool MCTypeConvertStringToReal(MCStringRef p_string, real64_t& r_converted, bool p_convert_octals)
{
    if (!MCStringCanBeNative(p_string))
        return false;
    
    MCAutoStringRefAsCString t_cstring;
    t_cstring . Lock(p_string);
    
    const char *t_sptr = *t_cstring;
    uindex_t t_length = strlen(t_sptr);
    bool t_done;
    
    real64_t t_value = MCU_strtor8(t_sptr, t_length, '\0', t_done, p_convert_octals);
    
    if (t_done)
        r_converted = t_value;
    
    return t_done;
}


MC_DLLEXPORT_DEF
bool MCTypeConvertDataToReal(MCDataRef p_data, real64_t& r_converted, bool p_convert_octals)
{
    const char *t_sptr = (const char *)MCDataGetBytePtr(p_data);
	uindex_t t_length = MCDataGetLength(p_data);
	bool t_done;
    real64_t t_value = MCU_strtor8(t_sptr, t_length, '\0', t_done, p_convert_octals);
    
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
