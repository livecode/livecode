/* Copyright (C) 2003-2015 LiveCode Ltd.

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

#include "util.h"
#include "execpt.h"
#include "date.h"
#include "globals.h"
#include "osspec.h"

#include "core.h"

MCDateTimeLocale g_english_locale =
{
	{"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"},
	{"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"},
	{"January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"},
	{"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"},
	{
		"^%#m/%#d/%y", // DATETIME_FORMAT_SHORT_ENGLISH_DATE
		"%a, %b %#d, %#Y", // DATETIME_FORMAT_ABBREV_ENGLISH_DATE
		"%A, %B %#d, %#Y", // DATETIME_FORMAT_LONG_ENGLISH_DATE
	},
	{
		"!%#I:%M %p", // DATETIME_FORMAT_SHORT_ENGLISH_TIME
		"!%#I:%M:%S %p" // DATETIME_FORMAT_LONG_ENGLISH_TIME
	},
	{
		"!%H:%M", // DATETIME_FORMAT_SHORT_ENGLISH_TIME24
		"!%H:%M:%S" // DATETIME_FORMAT_LONG_ENGLISH_TIME24
	},
	"AM", "PM"
};

static int1 s_datetime_month_length[] = {31,28,31,30,31,30,31,31,30,31,30,31};

static const char *s_internet_date_format = "!%a, %#d %b %Y %H:%M:%S %z";
static const char *s_items_date_format = "!%#Y,%#m,%#d,%#H,%#M,%#S,%#w";

///////////////////////////////////////////////////////////////////////////////
//
// Date Format Specifiers
//
// %a - abbreviated weekday name
// %A - full weekday name
// %b - abbreviated month name
// %B - full month name
// %w - day of the week
// %d - day of the month
// %m - month number
// %y - two-digit year
// %Y - four-digit year
//
// %I - 12-hour hour noon/midnight = 12
// %H - 24-hour hour
// %J - 12-hour noon/midnight = 0
// %M - minutes
// %S - seconds
// %p - AM/PM designator
// %z - time-zone bias
//
// dateitems - %#Y,%#m,%#d,%#h,%#m,%#s,%#w
// mime - %a, %d %b %Y %H:%M:%S %z
// short english - %#m/%#d/%y
// abbrev english - %a, %b %d, %#Y
// long english - %A, %B %d, %#Y
//

static char *append_string(char *p_buffer, const char *p_string, int p_length = -1)
{
	if (p_length == -1)
		p_length = strlen(p_string);

	memcpy(p_buffer, p_string, p_length);

	return p_buffer + p_length;
}

static char *append_number(char *p_buffer, int p_number, int p_pad, bool p_sign = false)
{
	if (p_number == 0)
	{
		if (p_pad == 0)
			p_pad = 1;
			
		int t_index;
		t_index = 0;
	
		if (p_sign)
			p_buffer[t_index++] = '+';

		for(;p_pad > 0; p_pad -= 1)
			p_buffer[t_index++] = '0';

		return p_buffer + t_index;
	}

	int t_abs_number;
	t_abs_number = MCU_abs(p_number);

	char t_decimal[16];
	int t_length;
	t_length = 0;

	while(t_abs_number != 0)
	{
		t_decimal[15 - t_length] = '0' + (t_abs_number % 10);
		t_abs_number /= 10;
		t_length += 1;
	}

	if (p_pad != 0)
		while(t_length < p_pad)
		{
			t_decimal[15 - t_length] = '0';
			t_length += 1;
		}

	if (p_number < 0)
	{
		t_decimal[15 - t_length] = '-';
		t_length += 1;
	}
	else if (p_sign)
	{
		t_decimal[15 - t_length] = '+';
		t_length += 1;
	}

	memcpy(p_buffer, t_decimal + 16 - t_length, t_length);

	return p_buffer + t_length;
}

static bool match_prefix(const char * const *p_table, uint4 p_table_length, const char*& p_input, uint4& p_input_length, int4& r_index)
{
	for(uint4 t_index = 0; t_index < p_table_length; ++t_index)
	{
		uint4 t_length;
		t_length = strlen(p_table[t_index]);
		if (t_length <= p_input_length && MCU_strncasecmp(p_table[t_index], p_input, t_length) == 0)
		{
			r_index = t_index + 1;
			p_input += t_length;
			p_input_length -= t_length;
			return true;
		}
	}
	return false;
}

static bool match_string(const char *p_string, const char*& p_input, uint4& p_input_length)
{
	uint4 t_length;
	
	t_length = strlen(p_string);
	if (t_length > p_input_length)
		return false;

	if (MCU_strncasecmp(p_string, p_input, t_length) == 0)
	{
		p_input += t_length;
		p_input_length -= t_length;
		return true;
	}

	return false;
}

static bool match_number(const char*& p_input, uint4& p_input_length, int4& r_number)
{
	const char *t_input;
	t_input = p_input;

	uint4 t_input_length;
	t_input_length = p_input_length;

	if (t_input_length == 0)
		return false;

	bool t_negative;
	t_negative = *t_input == '-';
	if (*t_input == '-' || *t_input == '+')
		t_input += 1, t_input_length -= 1;

	if (t_input_length == 0)
		return false;

	if (!isdigit(*t_input))
		return false;

	int4 t_number;
	t_number = 0;
	while(t_input_length > 0 && isdigit(*t_input))
	{
		t_number = t_number * 10 + *t_input - '0';
		t_input_length -= 1;
		t_input += 1;
	}

	if (t_negative)
		t_number = -t_number;

	p_input = t_input;
	p_input_length = t_input_length;

	r_number = t_number;

	return true;	
}

static int4 datetime_compute_dayofweek(const MCDateTime& p_datetime)
{
	uint4 t_year, t_month, t_day;
	
	t_year = p_datetime . year;
	t_month = p_datetime . month;
	t_day = p_datetime . day;

	if (t_month < 3)
	{
		t_month += 12;
		t_year -= 1;
	}

	return 1 + (t_day + (2 * t_month) + (6 * (t_month + 1) / 10) + t_year + t_year / 4 - t_year / 100 + t_year / 400 + 1) % 7;
}

static bool datetime_parse(const MCDateTimeLocale *p_locale, int4 p_century_cutoff, bool p_loose, const char *p_format, const char*& x_input, uint4& x_input_length, MCDateTime& r_datetime, int& r_valid_dateitems)
{
	const char *t_input;
	t_input = x_input;

	uint4 t_input_length;
	t_input_length = x_input_length;

	int4 t_year, t_month, t_day;
	int4 t_hour, t_minute, t_second;
	int4 t_bias, t_dayofweek;
	bool t_is_afternoon;

	int t_valid_dateitems;
	t_valid_dateitems = 0;

	t_year = 0;
	t_month = 0;
	t_day = 0;
	t_hour = 0;
	t_minute = 0;
	t_second = 0;
	t_bias = 0;
	t_is_afternoon = false;

	bool t_loose_components;
	bool t_loose_separators;

	if (*p_format == '!')
	{
		t_loose_components = false;
		t_loose_separators = false;
		p_format++;
	}
	else if (*p_format == '^' && p_loose)
	{
		t_loose_components = true;
		t_loose_separators = false;
		p_format++;
	}
	else
	{
		t_loose_components = p_loose;
		t_loose_separators = p_loose;
	}

	while(*p_format != '\0')
	{
		// If t_skip is true, we want to skip to the next specifier
		//
		bool t_skip;
		t_skip = false;

		bool t_valid;
		t_valid = false;

		if (*p_format == '%')
		{
			p_format += 1;
			if (*p_format == '#')
				p_format += 1;

			while(t_input_length > 0 && *t_input == ' ')
				t_input += 1, t_input_length -= 1;

			// MW-2007-09-11: [[ Bug 5293 ]] Fix the problem where you can't mix abbrev/long forms
			//   of month names and weekday names.
			switch(*p_format)
			{
			case 'a':
				if (!match_prefix(p_locale -> abbrev_weekday_names, 7, t_input, t_input_length, t_dayofweek))
				{
					// MW-2008-03-24: [[ Bug 6183 ]] Hmmm... Wishful thinking here - there are definitely 7 days of
					//   the week, not 12 as was previously asserted.
					if (t_loose_components)
						if (!match_prefix(p_locale -> weekday_names, 7, t_input, t_input_length, t_dayofweek))
							t_skip = true; // Weekday names are optional, so skip to the next specifier
						else
							t_valid = true;
				}
				else
					t_valid = true;
			break;
			case 'A':
				if (!match_prefix(p_locale -> weekday_names, 7, t_input, t_input_length, t_dayofweek))
				{
					// MW-2008-03-24: [[ Bug 6183 ]] Hmmm... Wishful thinking here - there are definitely 7 days of
					//   the week, not 12 as was previously asserted.
					if (t_loose_components)
						if (!match_prefix(p_locale -> abbrev_weekday_names, 7, t_input, t_input_length, t_dayofweek))
							t_skip = true; // Weekday names are optional, so skip to the next specifier
						else
							t_valid = true;
				}
				else
					t_valid = true;
			break;
			case 'b':
				if (!match_prefix(p_locale -> abbrev_month_names, 12, t_input, t_input_length, t_month))
					if (!t_loose_components || !match_prefix(p_locale -> month_names, 12, t_input, t_input_length, t_month))
						return false;
				t_valid_dateitems |= DATETIME_ITEM_MONTH;
				t_valid = true;
			break;
			case 'B':
				if (!match_prefix(p_locale -> month_names, 12, t_input, t_input_length, t_month))
					if (!t_loose_components || !match_prefix(p_locale -> abbrev_month_names, 12, t_input, t_input_length, t_month))
						return false;
				t_valid_dateitems |= DATETIME_ITEM_MONTH;
				t_valid = true;
			break;
			case 'w':
				if (!match_number(t_input, t_input_length, t_dayofweek))
					return false;
				t_valid = true;
			break;
			case 'd':
				if (!match_number(t_input, t_input_length, t_day))
					return false;
				t_valid_dateitems |= DATETIME_ITEM_DAY;
				t_valid = true;
			break;
			case 'm':
				if (!match_number(t_input, t_input_length, t_month))
					return false;
				t_valid_dateitems |= DATETIME_ITEM_MONTH;
				t_valid = true;
			break;
			case 'y':
			case 'Y':
			{
				int t_original_length;
				t_original_length = t_input_length;
				if (!match_number(t_input, t_input_length, t_year))
				{
					if (t_loose_components)
						t_skip = true; // Year specification is optional, so skip to the next specifier
				}
				else if (t_year < 100 && t_original_length - t_input_length <= 2)
				{
					if (t_year < p_century_cutoff)
						t_year += 100;
					t_year += 1900;

					t_valid_dateitems |= DATETIME_ITEM_YEAR;
					t_valid = true;
				}
				else
				{
					t_valid_dateitems |= DATETIME_ITEM_YEAR;
					t_valid = true;
				}
			}
			break;
			case 'J':
				if (!match_number(t_input, t_input_length, t_hour))
					return false;
				t_valid_dateitems |= DATETIME_ITEM_HOUR;
				t_valid = true;
			break;
			case 'I':
				if (!match_number(t_input, t_input_length, t_hour))
					return false;
				if (t_hour == 12)
					t_hour = 0;
				t_valid_dateitems |= DATETIME_ITEM_HOUR;
				t_valid = true;
			break;
			case 'H':
				if (!match_number(t_input, t_input_length, t_hour))
					return false;
				t_valid_dateitems |= DATETIME_ITEM_HOUR;
				t_valid = true;
			break;
			case 'M':
				if (!match_number(t_input, t_input_length, t_minute))
					return false;
				t_valid_dateitems |= DATETIME_ITEM_MINUTE;
				t_valid = true;
			break;
			case 'S':
				if (!match_number(t_input, t_input_length, t_second))
					return false;
				t_valid_dateitems |= DATETIME_ITEM_SECOND;
				t_valid = true;
			break;
			case 'p':
				if (p_locale -> time_evening_suffix[0] != '\0' && match_string(p_locale -> time_evening_suffix, t_input, t_input_length))
					t_is_afternoon = true;
				else if (p_locale -> time_morning_suffix[0] != '\0' && !match_string(p_locale -> time_morning_suffix, t_input, t_input_length))
					return false;
				t_valid = true;
			break;
			case 'z':
				if (!match_number(t_input, t_input_length, t_bias)) // TOO LOOSE!
					return false;
				t_bias = (t_bias / 100) * 60 + (t_bias % 100);
				t_valid = true;
			break;
			}

		}
		// FG-2013-10-08 [[ Bugfix 11261 ]]
		// If there is an extra space in the format string, treat it as a match
		else if (*p_format != *t_input && !isspace(*p_format))
		{
			// Unrecognised padding is optional, so advance and skip
			if (t_loose_separators)
				t_skip = true;
			else if (t_loose_components)
			{
				const char *t_lookahead;
				t_lookahead = p_format + 1;

				while(*t_lookahead != '\0' && *t_lookahead != '%')
					t_lookahead += 1;

				if (*t_lookahead == '\0')
					t_skip = true;
				else if (t_lookahead[2] == '\0' || t_lookahead[3] == '\0')
					t_skip = true;
			}

			if (t_input_length > 0)
			{
				t_input += 1;
				t_input_length -= 1;
			}
		}
		else
		{
			// FG-2013-09-10 [[ Bug 11162 ]]
			// One or more spaces in the format string should accept any number of input spaces
			// FG-2013-10-08 [[ Bug 11261 ]]
			// Over-incrementing of format pointer caused internet dates to parse incorrectly
			if (isspace(*p_format))
			{
				while (t_input_length > 0 && isspace(*t_input))
					t_input += 1, t_input_length -= 1;
				
				// Format is incremented past the current char below so just
				// remove additional spaces here and leave one for it.
				while (p_format[1] != '\0' && isspace(p_format[1]))
					p_format++;
			}
			else
			{
				if (t_input_length > 0)
				{
					t_input += 1;
					t_input_length -= 1;
				}
			}
			t_valid = true;
		}

		if (t_skip)
		{
			while(*p_format != '\0' && *p_format != '%')
				p_format += 1;

			while(t_input_length > 0 && isspace(*t_input))
				t_input_length -= 1, t_input += 1;
		}
		else
			p_format += 1;

		if (!t_valid && !t_skip)
			return false;
	}

	if (t_input_length > 0 && !isspace(*t_input))
		return false;

	while(t_input_length > 0 && isspace(*t_input))
		t_input_length -= 1, t_input += 1;

	// Year => Month <=> Day
	// Second => Minute <=> Hour

	if ((t_valid_dateitems & (DATETIME_ITEM_YEAR | DATETIME_ITEM_MONTH)) == DATETIME_ITEM_YEAR)
		return false;
	if ((t_valid_dateitems & (DATETIME_ITEM_MONTH | DATETIME_ITEM_DAY)) != 0 && (t_valid_dateitems & (DATETIME_ITEM_MONTH | DATETIME_ITEM_DAY)) != (DATETIME_ITEM_MONTH | DATETIME_ITEM_DAY))
		return false;

	if ((t_valid_dateitems & (DATETIME_ITEM_SECOND | DATETIME_ITEM_MINUTE)) == DATETIME_ITEM_SECOND)
		return false;
	if ((t_valid_dateitems & (DATETIME_ITEM_HOUR | DATETIME_ITEM_MINUTE)) != 0 && (t_valid_dateitems & (DATETIME_ITEM_MINUTE | DATETIME_ITEM_HOUR)) != (DATETIME_ITEM_MINUTE | DATETIME_ITEM_HOUR))
		return false;

	if (t_is_afternoon)
		t_hour += 12;

	r_datetime . bias = t_bias;

	if ((t_valid_dateitems & DATETIME_ITEM_SECOND) != 0)
		r_datetime . second = t_second;

	if ((t_valid_dateitems & DATETIME_ITEM_MINUTE) != 0)
		r_datetime . minute = t_minute;

	if ((t_valid_dateitems & DATETIME_ITEM_HOUR) != 0)
		r_datetime . hour = t_hour;

	if ((t_valid_dateitems & DATETIME_ITEM_DAY) != 0)
		r_datetime . day = t_day;

	if ((t_valid_dateitems & DATETIME_ITEM_MONTH) != 0)
		r_datetime . month = t_month;

	if ((t_valid_dateitems & DATETIME_ITEM_YEAR) != 0)
		r_datetime . year = t_year;

	x_input = t_input;
	x_input_length = t_input_length;
	r_valid_dateitems |= t_valid_dateitems;

	return true;
}

static char *datetime_format(const MCDateTimeLocale *p_locale, const char *p_format, const MCDateTime& p_datetime, char *p_buffer)
{
	int4 t_year, t_month, t_day;
	int4 t_hour, t_minute, t_second;
	int4 t_dayofweek, t_bias;

	t_year = p_datetime . year;
	t_month = p_datetime . month;
	t_day = p_datetime . day;
	t_hour = p_datetime . hour;
	t_minute = p_datetime . minute;
	t_second = p_datetime . second;
	t_dayofweek = datetime_compute_dayofweek(p_datetime);
	t_bias = p_datetime . bias;

	if (*p_format == '!' || *p_format == '^')
		p_format += 1;

	while(*p_format != '\0')
	{
		if (*p_format == '%')
		{
			bool t_pad;
			char t_unit;

			p_format++;
			if (*p_format == '#')
			{
				t_pad = false;
				p_format += 1;
			}
			else
				t_pad = true;

			t_unit = *p_format++;

			switch(t_unit)
			{
			case 'a': p_buffer = append_string(p_buffer, p_locale -> abbrev_weekday_names[t_dayofweek - 1]); break; // Abbreviated day of week
			case 'A': p_buffer = append_string(p_buffer, p_locale -> weekday_names[t_dayofweek - 1]); break; // Full day of week
			case 'b': p_buffer = append_string(p_buffer, p_locale -> abbrev_month_names[t_month - 1]); break; // Abbreviated month
			case 'B': p_buffer = append_string(p_buffer, p_locale -> month_names[t_month - 1]); break; // Full month
			case 'w': p_buffer = append_number(p_buffer, t_dayofweek, 0); break; // Day of week
			case 'd': p_buffer = append_number(p_buffer, t_day, t_pad ? 2 : 0); break; // Day of month
			case 'm': p_buffer = append_number(p_buffer, t_month, t_pad ? 2 : 0); break; // Month of year
			case 'y': p_buffer = append_number(p_buffer, t_year % 100, t_pad ? 2 : 0); break; // 2-digit year
			case 'Y': p_buffer = append_number(p_buffer, t_year, t_pad ? 4 : 0); break; // 4-digit year
			case 'I': p_buffer = append_number(p_buffer, (t_hour % 12) == 0 ? 12 : t_hour % 12, t_pad ? 2 : 0); break; // 12-hour hour
			case 'J': p_buffer = append_number(p_buffer, t_hour % 12, t_pad ? 2 : 0); break; // 12-hour 0-based hour
			case 'H': p_buffer = append_number(p_buffer, t_hour, t_pad ? 2 : 0); break; // 24-hour hour
			case 'M': p_buffer = append_number(p_buffer, t_minute, t_pad ? 2 : 0); break; // minutes
			case 'S': p_buffer = append_number(p_buffer, t_second, t_pad ? 2 : 0); break; // seconds
			case 'p': p_buffer = append_string(p_buffer, t_hour < 12 ? p_locale -> time_morning_suffix : p_locale -> time_evening_suffix); break; // 12-hour identifier
			case 'z': p_buffer = append_number(p_buffer, (t_bias / 60) * 100 + t_bias % 60, 4, true); break; // timezone
			case '%': *p_buffer++ = '%'; break;
			}
		}
		else
			*p_buffer++ = *p_format++;
	}

	return p_buffer;
}

static inline void datetime_normalize_value(int4 p_minimum, int4 p_maximum, int4& x_value, int4& x_overflow)
{
	while(x_value < p_minimum)
	{
		x_value += (p_maximum - p_minimum);
		x_overflow -= 1;
	}

	while(x_value >= p_maximum)
	{
		x_value -= (p_maximum - p_minimum);
		x_overflow += 1;
	}
}

static inline bool datetime_get_gregorian_leap_year(int4 p_year)
{
	return ((p_year & 0x3) == 0) && ((p_year % 100) != 0 || (p_year % 400) == 0);
}

static void datetime_normalize(MCDateTime& x_datetime)
{
	MCDateTime t_datetime;
	t_datetime = x_datetime;

	datetime_normalize_value(0, 60, t_datetime . second, t_datetime . minute);
	datetime_normalize_value(0, 60, t_datetime . minute, t_datetime . hour);
	datetime_normalize_value(0, 24, t_datetime . hour, t_datetime . day);
	datetime_normalize_value(1, 13, t_datetime . month, t_datetime . year);
	
	assert(t_datetime . second >= 0 && t_datetime . second < 60);
	assert(t_datetime . minute >= 0 && t_datetime . minute < 60);
	assert(t_datetime . hour >= 0 && t_datetime . hour < 24);
	assert(t_datetime . month >= 1 && t_datetime . month < 13);
	
	while(t_datetime . day < 1)
	{
		t_datetime . month -= 1;
		if (t_datetime . month < 1)
		{
			t_datetime . month += 12;
			t_datetime . year -= 1;
		}

		t_datetime . day += s_datetime_month_length[t_datetime . month - 1];
		if (t_datetime . month == 2 && datetime_get_gregorian_leap_year(t_datetime . year))
			t_datetime . day += 1;
	}

	while(t_datetime . day > s_datetime_month_length[t_datetime . month - 1])
	{
		if (t_datetime . month == 2 && datetime_get_gregorian_leap_year(t_datetime . year))
			if (t_datetime . day == 29)
				break;
			else
				t_datetime . day -= 1;

		t_datetime . day -= s_datetime_month_length[t_datetime . month - 1];
		t_datetime . month += 1;
		if (t_datetime . month > 12)
		{
			t_datetime . month -= 12;
			t_datetime . year += 1;
		}
	}
	
	x_datetime = t_datetime;
}

static bool datetime_validate(const MCDateTime& p_datetime)
{
	if (p_datetime . month < 1 || p_datetime . month > 12)
		return false;

	if (p_datetime . day < 1)
		return false;

	int t_month_length;
	if (p_datetime . month == 2 && datetime_get_gregorian_leap_year(p_datetime . year))
		t_month_length = 29;
	else
		t_month_length = s_datetime_month_length[p_datetime . month - 1];

	if (p_datetime . day > t_month_length)
		return false;

	if (p_datetime . hour < 0 || p_datetime . hour > 23)
		return false;

	if (p_datetime . minute < 0 || p_datetime . minute > 59)
		return false;

	if (p_datetime . second < 0 || p_datetime . second > 59)
		return false;

	return true;
}

///////////////////////////////////////////////////////////////////////////////

void MCD_decompose_format(MCExecPoint& p_context, uint4 p_format, uint4& r_length, bool& r_system)
{
	if (p_format > CF_SYSTEM)
	{
		r_system = true;
		r_length = p_format - CF_SYSTEM;
	}
	else if (p_format > CF_ENGLISH)
	{
		r_system = false;
		r_length = p_format - CF_ENGLISH;
	}
	else
	{
		r_system = p_context . getusesystemdate() == True;
		r_length = p_format;
	}
}

void MCD_date(Properties p_format, MCExecPoint& p_output)
{
	MCDateTime t_datetime;
	MCS_getlocaldatetime(t_datetime);

	p_output . clear();

	bool t_use_system;
	uint4 t_length;
	MCD_decompose_format(p_output, p_format, t_length, t_use_system);

	if (t_length != P_SHORT && t_length != P_ABBREVIATE && t_length != P_LONG && t_length != P_INTERNET)
		t_length = P_SHORT;

	const MCDateTimeLocale *t_locale;
	if (t_use_system && t_length != P_INTERNET)
		t_locale = MCS_getdatetimelocale();
	else
		t_locale = &g_english_locale;

	const char *t_date_format;
	if (t_length == P_INTERNET)
		t_date_format = s_internet_date_format;
	else
		t_date_format = t_locale -> date_formats[t_length - P_SHORT];

	char t_buffer[256];
	uint4 t_buffer_length;
	t_buffer_length = datetime_format(t_locale, t_date_format, t_datetime, t_buffer) - t_buffer;
	p_output . copysvalue(t_buffer, t_buffer_length);
}

void MCD_time(Properties p_format, MCExecPoint& p_output)
{
	MCDateTime t_datetime;
	MCS_getlocaldatetime(t_datetime);

	p_output . clear();

	bool t_use_system;
	uint4 t_length;
	MCD_decompose_format(p_output, p_format, t_length, t_use_system);

	if (t_length != P_LONG && t_length != P_INTERNET)
		t_length = P_ABBREVIATE;

	const MCDateTimeLocale *t_locale;
	if (t_use_system && t_length != P_INTERNET)
		t_locale = MCS_getdatetimelocale();
	else
		t_locale = &g_english_locale;

	const char *t_date_format;
	if (t_length == P_INTERNET)
		t_date_format = s_internet_date_format;
	else if (MCtwelvetime)
		t_date_format = t_locale -> time_formats[t_length - P_ABBREVIATE];
	else
		t_date_format = t_locale -> time24_formats[t_length - P_ABBREVIATE];

	char t_buffer[256];
	uint4 t_buffer_length;
	t_buffer_length = datetime_format(t_locale, t_date_format, t_datetime, t_buffer) - t_buffer;
	p_output . copysvalue(t_buffer, t_buffer_length);
}

void MCD_monthnames(Properties p_format, MCExecPoint& p_output)
{
	bool t_use_system;
	uint4 t_length;
	MCD_decompose_format(p_output, p_format, t_length, t_use_system);

	if (t_length == P_UNDEFINED)
		t_length = P_LONG;

	const MCDateTimeLocale *t_locale;
	if (t_use_system && t_length != P_SHORT)
		t_locale = MCS_getdatetimelocale();
	else
		t_locale = &g_english_locale;

	p_output . clear();

	for(uint4 t_month = 1; t_month <= 12; ++t_month)
	{
		if (t_length == P_SHORT)
			p_output . concatuint(t_month, EC_RETURN, t_month == 1);
		else if (t_length == P_ABBREVIATE)
			p_output . concatcstring(t_locale -> abbrev_month_names[t_month - 1], EC_RETURN, t_month == 1);
		else
			p_output . concatcstring(t_locale -> month_names[t_month - 1], EC_RETURN, t_month == 1);
	}
}

void MCD_weekdaynames(Properties p_format, MCExecPoint& p_output)
{
	bool t_use_system;
	uint4 t_length;
	MCD_decompose_format(p_output, p_format, t_length, t_use_system);

	if (t_length == P_UNDEFINED)
		t_length = P_LONG;

	const MCDateTimeLocale *t_locale;
	if (t_use_system && t_length != P_SHORT)
		t_locale = MCS_getdatetimelocale();
	else
		t_locale = &g_english_locale;

	p_output . clear();

	for(uint4 t_weekday = 1; t_weekday <= 7; ++t_weekday)
	{
		if (t_length == P_SHORT)
			p_output . concatuint(t_weekday, EC_RETURN, t_weekday == 1);
		else if (t_length == P_ABBREVIATE)
			p_output . concatcstring(t_locale -> abbrev_weekday_names[t_weekday - 1], EC_RETURN, t_weekday == 1);
		else
			p_output . concatcstring(t_locale -> weekday_names[t_weekday - 1], EC_RETURN, t_weekday == 1);
	}
}

void MCD_dateformat(Properties p_length, MCExecPoint& p_output)
{
	const MCDateTimeLocale *t_locale;

	int t_length;
	t_length = p_length;

	if (t_length >= CF_SYSTEM)
		t_length -= CF_SYSTEM;

	if (t_length >= CF_ENGLISH)
	{
		t_length -= CF_ENGLISH;
		t_locale = &g_english_locale;
	}
	else
		t_locale = MCS_getdatetimelocale();

	if (t_length != P_SHORT && t_length != P_ABBREVIATE && t_length != P_LONG)
		t_length = P_SHORT;

	const char *t_format;
	t_format = t_locale -> date_formats[t_length - P_SHORT];
	if (*t_format == '!' || *t_format == '^')
		t_format += 1;

	// Note that as the locales are either static, or cached the format strings
	// are essentially static.
	p_output . setstaticcstring(t_format);
}

// This function returns true if the format is a time.
//
static bool MCD_decompose_convert_format(MCExecPoint& p_context, int p_form, const MCDateTimeLocale*& r_locale, const char*& r_format)
{
	bool t_use_system;

	if (p_form >= CF_SYSTEM)
	{
		t_use_system = true;
		p_form -= CF_SYSTEM;
	}
	else if (p_form >= CF_ENGLISH)
	{
		t_use_system = false;
		p_form -= CF_ENGLISH;
	}
	else
		t_use_system = p_context . getusesystemdate() == True;

	r_locale = t_use_system ? MCS_getdatetimelocale() : &g_english_locale;

	switch(p_form)
	{
	case CF_INTERNET:
	case CF_INTERNET_DATE:
		r_format = s_internet_date_format;
	break;
	
	case CF_DATEITEMS:
		r_format = s_items_date_format;
	break;

	case CF_TIME:
	case CF_SHORT_TIME:
	case CF_ABBREV_TIME:
		r_format = MCtwelvetime ? r_locale -> time_formats[0] : r_locale -> time24_formats[0];
		return true;
		//r_is_date = false;
	break;

	case CF_LONG_TIME:
		r_format = MCtwelvetime ? r_locale -> time_formats[1] : r_locale -> time24_formats[1];
		return true;
		//r_is_date = false;
	break;

	case CF_DATE:
	case CF_SHORT_DATE:
		r_format = r_locale -> date_formats[0];
		//r_is_date = true;
	break;

	case CF_ABBREV_DATE:
		r_format = r_locale -> date_formats[1];
		//r_is_date = true;
	break;

	case CF_LONG_DATE:
		r_format = r_locale -> date_formats[2];
		//r_is_date = true;
	break;

	default:
		assert(false);
	break;
	}

	return false;
}

// Function:
//   MCD_convert
// Semantics:
//   Convert a date/time in format (p_primary_from && p_secondary_from) to a date/time 
//   in format (p_primary_to && p_secondary_to).
//
//   If no from format is specified, the convert command will attempt to parse as follows:
//     1) dateitems - STRICT
//     2) internet date - STRICT
//     3) a date then an optional time - LOOSE
//     4) a time then an optional date - LOOSE
//
//   If the 'useSystemDate' context property is set, the system date/time formats will be used,
//   otherwise the english date/time formats will be used.
//
//   When parsing a date (loosely) the following rules are observed:
//     1) for non-short dates, separating characters are ignored (except in localized strings)
//     2) weekday is optional
//     3) year is optional
//     4) a year can be any size, a 2-digit year is interpreted using centuryCutOff
//     5) a month name can be abbreviated or full
//
//   When specifying a from form strict parsing will be used with the following
//   relaxations:
//     1) weekdays are optional (they are skipped up to the next token)
//     2) years are optional (current year is used)
//
//   In the case only a date is specified, the time will be taken to be midnight *local-time*.
//   In the case only a time is specified, the date will be taken to be today's date.
//
//   All times are considered local except seconds which is universal.
//
//   The GMT bias is correctly taken into account for the internet date.
//

// We have to make time parsing more permissive. This is because of the introduction of system
// times when nobody's scripts are ready for it.
//
static bool convert_parse_time(MCExecPoint& p_context, const MCDateTimeLocale *p_locale, const char*& x_input, uint4& x_input_length, MCDateTime& x_datetime, int& x_valid_dateitems)
{
	if (p_locale != &g_english_locale)
		if (convert_parse_time(p_context, &g_english_locale, x_input, x_input_length, x_datetime, x_valid_dateitems))
			return true;

	for(uint4 t_format = 0; t_format < 4; ++t_format)
	{
		const char *t_time_format;
		if (t_format >= 2)
			t_time_format = p_locale -> time24_formats[3 - t_format];
		else
			t_time_format = p_locale -> time_formats[1 - t_format];
		if (datetime_parse(p_locale, p_context . getcutoff(), true, t_time_format, x_input, x_input_length, x_datetime, x_valid_dateitems))
			return true;
	}

	return false;
}

bool MCD_convert_to_datetime(MCExecPoint &p_context, Convert_form p_primary_from, Convert_form p_secondary_from, MCDateTime &r_datetime)
{
    bool t_success = true;
    
    MCDateTime t_datetime;
    
	// Make sure 'empty' is not a date
	if (p_context . getsvalue() . getlength() == 0)
		return false;
    
	if (p_primary_from != CF_UNDEFINED)
	{
		const char *t_input;
		uint4 t_input_length;
		t_input = p_context . getsvalue() . getstring();
		t_input_length = p_context . getsvalue() . getlength();
        
		if (p_primary_from == CF_SECONDS)
		{
			if (p_context . ton() != ES_NORMAL)
				return false;
            
			t_success = MCS_secondstodatetime(p_context . getnvalue(), t_datetime);
		}
		else if (p_primary_from == CF_DATEITEMS)
		{
			int t_valid_dateitems;
			t_valid_dateitems = 0;
            
			if (!datetime_parse(&g_english_locale, p_context . getcutoff(), false, s_items_date_format, t_input, t_input_length, t_datetime, t_valid_dateitems) || t_input_length != 0)
				return false;
            
			datetime_normalize(t_datetime);
            
			t_success = MCS_datetimetouniversal(t_datetime);
		}
		else if (p_primary_from == CF_INTERNET_DATE)
		{
			int t_valid_dateitems;
			t_valid_dateitems = 0;
            
			if (!datetime_parse(&g_english_locale, p_context . getcutoff(), false, s_internet_date_format, t_input, t_input_length, t_datetime, t_valid_dateitems) || t_input_length != 0)
				return false;
            
			if (!datetime_validate(t_datetime))
				return false;
            
			t_datetime . minute -= t_datetime . bias;
			t_datetime . bias = 0;
            
			datetime_normalize(t_datetime);
		}
		else
		{
			const MCDateTimeLocale *t_locale;
			const char *t_date_format;
            
			int t_valid_dateitems;
			t_valid_dateitems = 0;
            
			bool t_is_time;
			t_is_time = MCD_decompose_convert_format(p_context, p_primary_from, t_locale, t_date_format);
			if (t_is_time && !convert_parse_time(p_context, t_locale, t_input, t_input_length, t_datetime, t_valid_dateitems))
				return false;
			else if (!t_is_time && !datetime_parse(t_locale, p_context . getcutoff(), true, t_date_format, t_input, t_input_length, t_datetime, t_valid_dateitems))
				return false;
            
			if (p_secondary_from != CF_UNDEFINED)
			{
				t_is_time = MCD_decompose_convert_format(p_context, p_secondary_from, t_locale, t_date_format);
				if (t_is_time && !convert_parse_time(p_context, t_locale, t_input, t_input_length, t_datetime, t_valid_dateitems))
					return false;
				else if (!t_is_time && !datetime_parse(t_locale, p_context . getcutoff(), true, t_date_format, t_input, t_input_length, t_datetime, t_valid_dateitems))
					return false;
			}
			
            
			if (t_input_length != 0)
				return false;
            
			if ((t_valid_dateitems & DATETIME_ITEM_DATE) != DATETIME_ITEM_DATE)
			{
				MCDateTime t_today;
				MCS_getlocaldatetime(t_today);
                
				if ((t_valid_dateitems & DATETIME_ITEM_DAY) == 0)
					t_datetime . day = t_today . day;
                
				if ((t_valid_dateitems & DATETIME_ITEM_MONTH) == 0)
					t_datetime . month = t_today . month;
                
				if ((t_valid_dateitems & DATETIME_ITEM_YEAR) == 0)
					t_datetime . year = t_today . year;
			}
            
			if ((t_valid_dateitems & DATETIME_ITEM_HOUR) == 0)
			{
				t_datetime . hour = 0;
				t_datetime . minute = 0;
				t_datetime . second = 0;
				t_datetime . bias = 0;
			}
			else if ((t_valid_dateitems & DATETIME_ITEM_SECOND) == 0)
				t_datetime . second = 0;
			
			if (!datetime_validate(t_datetime))
				return false;
            
			t_success = MCS_datetimetouniversal(t_datetime);
		}
	}
	else if (p_context . ton() == ES_NORMAL)
	{
		double t_seconds;
		t_seconds = p_context . getnvalue();
        
		if (t_seconds < SECONDS_MIN || t_seconds > SECONDS_MAX)
			return false;
        
		t_success = MCS_secondstodatetime(t_seconds, t_datetime);
	}
	else if (p_context . getsvalue() . getlength() != 0)
	{
		const MCDateTimeLocale *t_locale;
		if (p_context . getusesystemdate())
			t_locale = MCS_getdatetimelocale();
		else
			t_locale = &g_english_locale;
        
		const char *t_input;
		uint4 t_input_length;
		t_input = p_context . getsvalue() . getstring();
		t_input_length = p_context . getsvalue() . getlength();
        
		int t_valid_dateitems;
		t_valid_dateitems = 0;
        
		// Order for dates:
		//   long date
		//   abbrev date
		//   short date
		//
		// Order for times:
		//   long time
		//   short time
		//   long time 24
		//   short time 24
        
		if (datetime_parse(&g_english_locale, p_context . getcutoff(), false, s_items_date_format, t_input, t_input_length, t_datetime, t_valid_dateitems) && t_input_length == 0)
		{
			datetime_normalize(t_datetime);
			t_success = MCS_datetimetouniversal(t_datetime);
		}
		else if (datetime_parse(&g_english_locale, p_context . getcutoff(), false, s_internet_date_format, t_input, t_input_length, t_datetime, t_valid_dateitems) && t_input_length == 0)
		{
			if (!datetime_validate(t_datetime))
				return false;
            
			t_datetime . minute -= t_datetime . bias;
			t_datetime . bias = 0;
            
			datetime_normalize(t_datetime);
		}
		else
		{
			bool t_date_valid;
			bool t_time_valid;
            
			t_date_valid = false;
			t_time_valid = false;
            
			while(t_input_length > 0 && !(t_date_valid && t_time_valid))
			{
				bool t_changed;
				t_changed = false;
                
				if (!t_date_valid)
				{
					for(uint4 t_format = 0; t_format < 3; ++t_format)
						if (datetime_parse(t_locale, p_context . getcutoff(), true, t_locale -> date_formats[2 - t_format], t_input, t_input_length, t_datetime, t_valid_dateitems))
						{
							t_date_valid = true;
							t_changed = true;
							break;
						}
				}
                
				if (!t_time_valid)
				{
					if (convert_parse_time(p_context, t_locale, t_input, t_input_length, t_datetime, t_valid_dateitems))
					{
						t_time_valid = true;
						t_changed = true;
					}
				}
                
				if (!t_changed)
					break;
			}
            
			if (t_input_length > 0)
				return false;
            
			if ((t_valid_dateitems & DATETIME_ITEM_DATE) != DATETIME_ITEM_DATE)
			{
				MCDateTime t_today;
				MCS_getlocaldatetime(t_today);
                
				if ((t_valid_dateitems & DATETIME_ITEM_DAY) == 0)
					t_datetime . day = t_today . day;
                
				if ((t_valid_dateitems & DATETIME_ITEM_MONTH) == 0)
					t_datetime . month = t_today . month;
                
				if ((t_valid_dateitems & DATETIME_ITEM_YEAR) == 0)
					t_datetime . year = t_today . year;
			}
            
			if ((t_valid_dateitems & DATETIME_ITEM_HOUR) == 0)
			{
				t_datetime . hour = 0;
				t_datetime . minute = 0;
				t_datetime . second = 0;
				t_datetime . bias = 0;
			}
			else if ((t_valid_dateitems & DATETIME_ITEM_SECOND) == 0)
				t_datetime . second = 0;
			
			if (!datetime_validate(t_datetime))
				return false;
            
			t_success = MCS_datetimetouniversal(t_datetime);
		}
	}
    
    if (t_success)
        r_datetime = t_datetime;
    
    return t_success;
}

bool MCD_convert_from_datetime(MCExecPoint &p_context, Convert_form p_primary_to, Convert_form p_secondary_to, MCDateTime &p_datetime)
{
    bool t_success = true;
    
	if (p_primary_to == CF_SECONDS)
	{
		double t_seconds;
        
		t_success = MCS_datetimetoseconds(p_datetime, t_seconds);
        
		if (!t_success)
			return False;
        
		p_context . setnvalue(t_seconds);
	}
	else 
	{
		if (p_primary_to == CF_INTERNET_DATE || p_secondary_to == CF_DATEITEMS)
			p_secondary_to = CF_UNDEFINED;
        
		t_success = MCS_datetimetolocal(p_datetime);
        
		if (!t_success)
			return False;
        
		const MCDateTimeLocale *t_locale;
		const char *t_date_format;
        
		char t_buffer[256];
		int t_buffer_length;
        
		MCD_decompose_convert_format(p_context, p_primary_to, t_locale, t_date_format);
		t_buffer_length = datetime_format(t_locale, t_date_format, p_datetime, t_buffer) - t_buffer;
		p_context . setchars(t_buffer, t_buffer_length);
        
		if (p_secondary_to != CF_UNDEFINED)
		{
			MCD_decompose_convert_format(p_context, p_secondary_to, t_locale, t_date_format);
			t_buffer_length = datetime_format(t_locale, t_date_format, p_datetime, t_buffer) - t_buffer;
			p_context . concatchars(t_buffer, t_buffer_length, EC_SPACE, false);
		}
	}
    
    return t_success;
}

Boolean MCD_convert(MCExecPoint& p_context, Convert_form p_primary_from, Convert_form p_secondary_from, Convert_form p_primary_to, Convert_form p_secondary_to)
{
	bool t_success;
	t_success = true;

	// MM-2012-03-01: [[ BUG 10006]] Primaries and secondaries mixed up
	MCDateTime t_datetime;

	t_success = MCD_convert_to_datetime(p_context, p_primary_from, p_secondary_from, t_datetime);
    
	p_context . clear();

    if (t_success)
        t_success = MCD_convert_from_datetime(p_context, p_primary_to, p_secondary_to, t_datetime);
    
	return t_success;
}

///////////////////////////////////////////////////////////////////////////////
