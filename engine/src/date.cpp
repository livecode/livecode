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

#include "date.h"
#include "globals.h"
#include "osspec.h"

#include "exec.h"

MCDateTimeLocale *g_basic_locale;

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

bool MCDateTimeInitialize()
{
	g_basic_locale = (MCDateTimeLocale*)malloc(sizeof(MCDateTimeLocale));
	
	g_basic_locale->weekday_names[0] = MCSTR("Sunday");
	g_basic_locale->weekday_names[1] = MCSTR("Monday");
	g_basic_locale->weekday_names[2] = MCSTR("Tuesday");
	g_basic_locale->weekday_names[3] = MCSTR("Wednesday");
	g_basic_locale->weekday_names[4] = MCSTR("Thursday");
	g_basic_locale->weekday_names[5] = MCSTR("Friday");
	g_basic_locale->weekday_names[6] = MCSTR("Saturday");
	
	g_basic_locale->abbrev_weekday_names[0] = MCSTR("Sun");
	g_basic_locale->abbrev_weekday_names[1] = MCSTR("Mon");
	g_basic_locale->abbrev_weekday_names[2] = MCSTR("Tue");
	g_basic_locale->abbrev_weekday_names[3] = MCSTR("Wed");
	g_basic_locale->abbrev_weekday_names[4] = MCSTR("Thu");
	g_basic_locale->abbrev_weekday_names[5] = MCSTR("Fri");
	g_basic_locale->abbrev_weekday_names[6] = MCSTR("Sat");
	
	g_basic_locale->month_names[0] = MCSTR("January");
	g_basic_locale->month_names[1] = MCSTR("February");
	g_basic_locale->month_names[2] = MCSTR("March");
	g_basic_locale->month_names[3] = MCSTR("April");
	g_basic_locale->month_names[4] = MCSTR("May");
	g_basic_locale->month_names[5] = MCSTR("June");
	g_basic_locale->month_names[6] = MCSTR("July");
	g_basic_locale->month_names[7] = MCSTR("August");
	g_basic_locale->month_names[8] = MCSTR("September");
	g_basic_locale->month_names[9] = MCSTR("October");
	g_basic_locale->month_names[10]= MCSTR("November");
	g_basic_locale->month_names[11]= MCSTR("December");
	
	g_basic_locale->abbrev_month_names[0] = MCSTR("Jan");
	g_basic_locale->abbrev_month_names[1] = MCSTR("Feb");
	g_basic_locale->abbrev_month_names[2] = MCSTR("Mar");
	g_basic_locale->abbrev_month_names[3] = MCSTR("Apr");
	g_basic_locale->abbrev_month_names[4] = MCSTR("May");
	g_basic_locale->abbrev_month_names[5] = MCSTR("Jun");
	g_basic_locale->abbrev_month_names[6] = MCSTR("Jul");
	g_basic_locale->abbrev_month_names[7] = MCSTR("Aug");
	g_basic_locale->abbrev_month_names[8] = MCSTR("Sep");
	g_basic_locale->abbrev_month_names[9] = MCSTR("Oct");
	g_basic_locale->abbrev_month_names[10]= MCSTR("Nov");
	g_basic_locale->abbrev_month_names[11]= MCSTR("Dec");
	
    // SN-2014-09-17: [[ Bug 13460 ]] No hashtag for the year
	g_basic_locale->date_formats[0] = MCSTR("^%#m/%#d/%y");
	g_basic_locale->date_formats[1] = MCSTR("%a, %b %#d, %#Y");
	g_basic_locale->date_formats[2] = MCSTR("%A, %B %#d, %#Y");
	
	g_basic_locale->time_formats[0] = MCSTR("!%#I:%M %p");
	g_basic_locale->time_formats[1] = MCSTR("!%#I:%M:%S %p");
	
	g_basic_locale->time24_formats[0] = MCSTR("!%H:%M");
	g_basic_locale->time24_formats[1] = MCSTR("!%H:%M:%S");

	g_basic_locale->time_morning_suffix = MCSTR("AM");
	g_basic_locale->time_evening_suffix = MCSTR("PM");
	
	return true;
}

bool MCDateTimeFinalize()
{
	g_basic_locale->~MCDateTimeLocale();
	return true;
}

static bool match_string(MCStringRef p_string, MCStringRef p_input, uindex_t &x_offset)
{
	uindex_t t_length;
	t_length = MCStringGetLength(p_string);
	MCRange t_range;
	t_range = MCRangeMake(x_offset, t_length);
	if (MCStringSubstringContains(p_input, t_range, p_string, kMCStringOptionCompareCaseless))
	{
		x_offset += t_length;
		return true;
	}
	
	return false;
}

static bool match_prefix(MCStringRef const * p_table, uint4 p_table_length, MCStringRef p_input, uindex_t &x_offset, int4& r_index)
{
	for(uint4 t_index = 0; t_index < p_table_length; ++t_index)
	{
		if (match_string(p_table[t_index], p_input, x_offset))
		{
			r_index = t_index + 1;
			return true;
		}
	}
	
	return false;
}

static bool match_number(MCStringRef p_input, uindex_t &x_offset, int4& r_number)
{
	uindex_t t_length;
	t_length = MCStringGetLength(p_input);

	unichar_t t_char;
	t_char = MCStringGetCharAtIndex(p_input, x_offset++);
	
	bool t_negative;
	t_negative = t_char == '-';
	if (t_char == '-' || t_char == '+')
		t_char = MCStringGetCharAtIndex(p_input, x_offset++);

	if (!isdigit(t_char))
		return false;

	int4 t_number;
	t_number = 0;
	while(x_offset <= t_length && isdigit(t_char))
	{
		t_number = t_number * 10 + t_char - '0';
		t_char = MCStringGetCharAtIndex(p_input, x_offset++);
	}
	
	x_offset--;

	if (t_negative)
		t_number = -t_number;

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

static bool datetime_parse(const MCDateTimeLocale *p_locale, int4 p_century_cutoff, bool p_loose, MCStringRef p_format, MCStringRef p_input, uindex_t &x_offset, MCDateTime& r_datetime, int& r_valid_dateitems)
{
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

	uindex_t t_in_offset = x_offset;
	uindex_t t_offset = 0;
	uindex_t t_length;
	uindex_t t_in_length;
	t_length = MCStringGetLength(p_format);
	t_in_length = MCStringGetLength(p_input);
	
	if (MCStringGetCharAtIndex(p_format, t_offset) == '!')
	{
		t_loose_components = false;
		t_loose_separators = false;
		t_offset++;
	}
	else if (MCStringGetCharAtIndex(p_format, t_offset) == '^' && p_loose)
	{
		t_loose_components = true;
		t_loose_separators = false;
		t_offset++;
	}
	else
	{
		t_loose_components = p_loose;
		t_loose_separators = p_loose;
	}

	while(t_offset < t_length)
	{
		unichar_t t_char;
		t_char = MCStringGetCharAtIndex(p_format, t_offset++);
		
		uindex_t t_old_offset;
		t_old_offset = t_in_offset;
		
		// If t_skip is true, we want to skip to the next specifier
		//
		bool t_skip;
		t_skip = false;

		bool t_valid;
		t_valid = false;

		if (t_char == '%')
		{
			t_char = MCStringGetCharAtIndex(p_format, t_offset++);
			if (t_char == '#')
				t_char = MCStringGetCharAtIndex(p_format, t_offset++);

			while(t_in_offset < t_in_length && MCStringGetCharAtIndex(p_input, t_in_offset) == ' ')
				t_in_offset++;

			// MW-2007-09-11: [[ Bug 5293 ]] Fix the problem where you can't mix abbrev/long forms
			//   of month names and weekday names.
			switch(t_char)
			{
			case 'a':
				if (!match_prefix(p_locale -> abbrev_weekday_names, 7, p_input, t_in_offset, t_dayofweek))
				{
					// MW-2008-03-24: [[ Bug 6183 ]] Hmmm... Wishful thinking here - there are definitely 7 days of
					//   the week, not 12 as was previously asserted.
					if (t_loose_components)
                    {
						if (!match_prefix(p_locale -> weekday_names, 7, p_input, t_in_offset, t_dayofweek))
							t_skip = true; // Weekday names are optional, so skip to the next specifier
						else
							t_valid = true;
                    }
				}
				else
					t_valid = true;
			break;
			case 'A':
				if (!match_prefix(p_locale -> weekday_names, 7, p_input, t_in_offset, t_dayofweek))
				{
					// MW-2008-03-24: [[ Bug 6183 ]] Hmmm... Wishful thinking here - there are definitely 7 days of
					//   the week, not 12 as was previously asserted.
					if (t_loose_components)
                    {
						if (!match_prefix(p_locale -> abbrev_weekday_names, 7, p_input, t_in_offset, t_dayofweek))
							t_skip = true; // Weekday names are optional, so skip to the next specifier
						else
							t_valid = true;
                    }
				}
				else
					t_valid = true;
			break;
			case 'b':
				if (!match_prefix(p_locale -> abbrev_month_names, 12, p_input, t_in_offset, t_month))
					if (!t_loose_components || !match_prefix(p_locale -> month_names, 12, p_input, t_in_offset, t_month))
						return false;
				t_valid_dateitems |= DATETIME_ITEM_MONTH;
				t_valid = true;
			break;
			case 'B':
				if (!match_prefix(p_locale -> month_names, 12, p_input, t_in_offset, t_month))
					if (!t_loose_components || !match_prefix(p_locale -> abbrev_month_names, 12, p_input, t_in_offset, t_month))
						return false;
				t_valid_dateitems |= DATETIME_ITEM_MONTH;
				t_valid = true;
			break;
			case 'w':
				if (!match_number(p_input, t_in_offset, t_dayofweek))
					return false;
				t_valid = true;
			break;
			case 'd':
				if (!match_number(p_input, t_in_offset, t_day))
					return false;
				t_valid_dateitems |= DATETIME_ITEM_DAY;
				t_valid = true;
			break;
			case 'm':
				if (!match_number(p_input, t_in_offset, t_month))
					return false;
				t_valid_dateitems |= DATETIME_ITEM_MONTH;
				t_valid = true;
			break;
			case 'y':
			case 'Y':
			{
				if (!match_number(p_input, t_in_offset, t_year))
				{
					if (t_loose_components)
						t_skip = true; // Year specification is optional, so skip to the next specifier
				}
				else if (t_year < 100 && t_in_offset - t_old_offset <= 2)
				{
					// Year was in a two-digit form
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
				if (!match_number(p_input, t_in_offset, t_hour))
					return false;
				t_valid_dateitems |= DATETIME_ITEM_HOUR;
				t_valid = true;
			break;
			case 'I':
				if (!match_number(p_input, t_in_offset, t_hour))
					return false;
				if (t_hour == 12)
					t_hour = 0;
				t_valid_dateitems |= DATETIME_ITEM_HOUR;
				t_valid = true;
			break;
			case 'H':
				if (!match_number(p_input, t_in_offset, t_hour))
					return false;
				t_valid_dateitems |= DATETIME_ITEM_HOUR;
				t_valid = true;
			break;
			case 'M':
				if (!match_number(p_input, t_in_offset, t_minute))
					return false;
				t_valid_dateitems |= DATETIME_ITEM_MINUTE;
				t_valid = true;
			break;
			case 'S':
				if (!match_number(p_input, t_in_offset, t_second))
					return false;
				t_valid_dateitems |= DATETIME_ITEM_SECOND;
				t_valid = true;
			break;
			case 'p':
				if (!MCStringIsEmpty(p_locale->time_evening_suffix) && match_string(p_locale -> time_evening_suffix, p_input, t_in_offset))
					t_is_afternoon = true;
				else if (!MCStringIsEmpty(p_locale->time_morning_suffix) && !match_string(p_locale -> time_morning_suffix, p_input, t_in_offset))
					return false;
				t_valid = true;
			break;
			case 'z':
				if (!match_number(p_input, t_in_offset, t_bias)) // TOO LOOSE!
					return false;
				t_bias = (t_bias / 100) * 60 + (t_bias % 100);
				t_valid = true;
			break;
			}

		}
		// FG-2013-10-08 [[ Bugfix 11261 ]]
		// If there is an extra space in the format string, treat it as a match
		else if (t_char != MCStringGetCharAtIndex(p_input, t_in_offset) && !isspace(t_char))
		{
			// Unrecognised padding is optional, so advance and skip
			if (t_loose_separators)
				t_skip = true;
			else if (t_loose_components)
			{
				while (t_offset < t_length && MCStringGetCharAtIndex(p_format, t_offset) != '%')
					t_offset++;
				
				/* ODDITY */
				// We don't quite understand this bit...
				index_t t_diff = t_length - t_offset;
				if (t_diff == 0 || t_diff == 2 || t_diff == 3)
					t_skip = true;
			}
		}
		else
		{
			// FG-2013-09-10 [[ Bug 11162 ]]
			// One or more spaces in the format string should accept any number of input spaces
			// FG-2013-10-08 [[ Bug 11261 ]]
			// Over-incrementing of format pointer caused internet dates to parse incorrectly
			if (isspace(t_char))
			{
				while (t_in_offset < t_in_length && isspace(MCStringGetCharAtIndex(p_input, t_in_offset)))
					t_in_offset++;
				
				// Format is incremented past the current char below so just
				// remove additional spaces here and leave one for it.
				while (MCStringGetCharAtIndex(p_format, t_offset + 1) != '\0' && isspace(MCStringGetCharAtIndex(p_format, t_offset + 1)))
					t_offset++;
			}
			else
			{
				if (t_in_offset < t_in_length)
					t_in_offset++;
			}
			t_valid = true;
		}

		if (t_skip)
		{
			while (t_offset < t_length && MCStringGetCharAtIndex(p_format, t_offset) != '%')
				t_offset++;

			while (t_in_offset < t_in_length && isspace(MCStringGetCharAtIndex(p_input, t_in_offset)))
				t_in_offset++;
		}

		if (!t_valid && !t_skip)
			return false;
	}
	
	if (t_offset < t_length && !isspace(MCStringGetCharAtIndex(p_input, t_offset)))
		return false;
	
	while (t_offset < t_length && isspace(MCStringGetCharAtIndex(p_input, t_offset)))
		t_offset++;

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

	x_offset = t_in_offset;
	r_valid_dateitems |= t_valid_dateitems;

	return true;
}

static void datetime_format(const MCDateTimeLocale *p_locale, MCStringRef p_format, const MCDateTime& p_datetime, MCStringRef &r_buffer)
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

	uindex_t t_offset = 0;
	unichar_t t_char;
	t_char = MCStringGetCharAtIndex(p_format, t_offset++);
	
	if (t_char == '!' || t_char == '^')
		t_char = MCStringGetCharAtIndex(p_format, t_offset++);
	
	MCStringRef t_buffer;
	/* UNCHECKED */ MCStringCreateMutable(0, t_buffer);

	while(t_char != '\0')
	{
		if (t_char == '%')
		{
			t_char = MCStringGetCharAtIndex(p_format, t_offset++);
			
			bool t_pad;
			if (t_char == '#')
			{
				t_pad = false;
				t_char = MCStringGetCharAtIndex(p_format, t_offset++);
			}
			else
				t_pad = true;

			switch(t_char)
			{
			case 'a': /* UNCHECKED */ MCStringAppend(t_buffer, p_locale -> abbrev_weekday_names[t_dayofweek - 1]); break; // Abbreviated day of week
			case 'A': /* UNCHECKED */ MCStringAppend(t_buffer, p_locale -> weekday_names[t_dayofweek - 1]); break; // Full day of week
			case 'b': /* UNCHECKED */ MCStringAppend(t_buffer, p_locale -> abbrev_month_names[t_month - 1]); break; // Abbreviated month
			case 'B': /* UNCHECKED */ MCStringAppend(t_buffer, p_locale -> month_names[t_month - 1]); break; // Full month
			case 'w': /* UNCHECKED */ MCStringAppendFormat(t_buffer, "%d", t_dayofweek); break; // Day of week
			case 'd': /* UNCHECKED */ MCStringAppendFormat(t_buffer, t_pad ? "%02d" : "%d", t_day); break; // Day of month
			case 'm': /* UNCHECKED */ MCStringAppendFormat(t_buffer, t_pad ? "%02d" : "%d", t_month); break; // Month of year
			case 'y': /* UNCHECKED */ MCStringAppendFormat(t_buffer, t_pad ? "%02d" : "%d", t_year % 100); break; // 2-digit year
			case 'Y': /* UNCHECKED */ MCStringAppendFormat(t_buffer, t_pad ? "%04d" : "%d", t_year); break; // 4-digit year
			case 'I': /* UNCHECKED */ MCStringAppendFormat(t_buffer, t_pad ? "%02d" : "%d", (t_hour % 12) == 0 ? 12 : t_hour % 12); break; // 12-hour hour
			case 'J': /* UNCHECKED */ MCStringAppendFormat(t_buffer, t_pad ? "%02d" : "%d", t_hour % 12); break; // 12-hour 0-based hour
			case 'H': /* UNCHECKED */ MCStringAppendFormat(t_buffer, t_pad ? "%02d" : "%d", t_hour); break; // 24-hour hour
			case 'M': /* UNCHECKED */ MCStringAppendFormat(t_buffer, t_pad ? "%02d" : "%d", t_minute); break; // minutes
			case 'S': /* UNCHECKED */ MCStringAppendFormat(t_buffer, t_pad ? "%02d" : "%d", t_second); break; // seconds
			case 'p': /* UNCHECKED */ MCStringAppend(t_buffer, t_hour < 12 ? p_locale -> time_morning_suffix : p_locale -> time_evening_suffix); break; // 12-hour identifier
			case 'z': /* UNCHECKED */ MCStringAppendFormat(t_buffer, "%+05d", (t_bias / 60) * 100 + t_bias % 60); break; // timezone
			case '%': /* UNCHECKED */ MCStringAppendChar(t_buffer, '%'); break;
			}
		}
		else
			/* UNCHECKED */ MCStringAppendChar(t_buffer, t_char);
		
		t_char = MCStringGetCharAtIndex(p_format, t_offset++);
	}

	/* UNCHECKED */ MCStringCopyAndRelease(t_buffer, r_buffer);
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
        {
			if (t_datetime . day == 29)
				break;
			else
				t_datetime . day -= 1;
        }
        
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

void MCD_decompose_format(MCExecContext &ctxt, uint4 p_format, uint4& r_length, bool& r_system)
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
		r_system = ctxt.GetUseSystemDate() == True;
		r_length = p_format;
	}
}

void MCD_date(MCExecContext &ctxt, Properties p_format, MCStringRef &r_date)
{
	MCDateTime t_datetime;
	MCS_getlocaldatetime(t_datetime);

	bool t_use_system;
	uint4 t_length;
	MCD_decompose_format(ctxt, p_format, t_length, t_use_system);

	if (t_length != P_SHORT && t_length != P_ABBREVIATE && t_length != P_LONG && t_length != P_INTERNET)
		t_length = P_SHORT;

	const MCDateTimeLocale *t_locale;
	if (t_use_system && t_length != P_INTERNET)
		t_locale = MCS_getdatetimelocale();
	else
		t_locale = g_basic_locale;

	MCStringRef t_date_format;
	if (t_length == P_INTERNET)
		t_date_format = MCSTR(s_internet_date_format);
	else
		t_date_format = t_locale -> date_formats[t_length - P_SHORT];

	datetime_format(t_locale, t_date_format, t_datetime, r_date);
}

void MCD_time(MCExecContext &ctxt, Properties p_format, MCStringRef &r_time)
{
	MCDateTime t_datetime;
	MCS_getlocaldatetime(t_datetime);

	bool t_use_system;
	uint4 t_length;
	MCD_decompose_format(ctxt, p_format, t_length, t_use_system);

	if (t_length != P_LONG && t_length != P_INTERNET)
		t_length = P_ABBREVIATE;

	const MCDateTimeLocale *t_locale;
	if (t_use_system && t_length != P_INTERNET)
		t_locale = MCS_getdatetimelocale();
	else
		t_locale = g_basic_locale;

	MCStringRef t_date_format;
	if (t_length == P_INTERNET)
		t_date_format = MCSTR(s_internet_date_format);
	else if (MCtwelvetime)
		t_date_format = t_locale -> time_formats[t_length - P_ABBREVIATE];
	else
		t_date_format = t_locale -> time24_formats[t_length - P_ABBREVIATE];

	datetime_format(t_locale, t_date_format, t_datetime, r_time);
}

bool MCD_monthnames(MCExecContext& ctxt, Properties p_format, MCListRef& r_list)
{
	bool t_use_system;
	uint4 t_length;
	MCD_decompose_format(ctxt, p_format, t_length, t_use_system);

	if (t_length == P_UNDEFINED)
		t_length = P_LONG;

	const MCDateTimeLocale *t_locale;
	if (t_use_system && t_length != P_SHORT)
		t_locale = MCS_getdatetimelocale();
	else
		t_locale = g_basic_locale;

	bool t_success = true;

	MCAutoListRef t_list;
	t_success = MCListCreateMutable('\n', &t_list);

	for(uint4 t_month = 1; t_success && t_month <= 12; ++t_month)
	{
		if (t_length == P_SHORT)
			t_success = MCListAppendInteger(*t_list, t_month);
		else if (t_length == P_ABBREVIATE)
			t_success = MCListAppend(*t_list, t_locale -> abbrev_month_names[t_month - 1]);
		else
			t_success = MCListAppend(*t_list, t_locale -> month_names[t_month - 1]);
	}

	return t_success && MCListCopy(*t_list, r_list);
}

bool MCD_weekdaynames(MCExecContext& ctxt, Properties p_format, MCListRef& r_list)
{
	bool t_use_system;
	uint4 t_length;
	MCD_decompose_format(ctxt, p_format, t_length, t_use_system);

	if (t_length == P_UNDEFINED)
		t_length = P_LONG;

	const MCDateTimeLocale *t_locale;
	if (t_use_system && t_length != P_SHORT)
		t_locale = MCS_getdatetimelocale();
	else
		t_locale = g_basic_locale;

	bool t_success = true;

	MCAutoListRef t_list;
	t_success = MCListCreateMutable('\n', &t_list);

	for(uint4 t_weekday = 1; t_success && t_weekday <= 7; ++t_weekday)
	{
		if (t_length == P_SHORT)
			t_success = MCListAppendInteger(*t_list, t_weekday);
		else if (t_length == P_ABBREVIATE)
			t_success = MCListAppend(*t_list, t_locale -> abbrev_weekday_names[t_weekday - 1]);
		else
			t_success = MCListAppend(*t_list, t_locale -> weekday_names[t_weekday - 1]);
	}

	return t_success && MCListCopy(*t_list, r_list);
}

void MCD_dateformat(MCExecContext &ctxt, Properties p_length, MCStringRef& r_dateformat)
{
	const MCDateTimeLocale *t_locale;

	int t_length;
	t_length = p_length;

	if (t_length >= CF_SYSTEM)
		t_length -= CF_SYSTEM;

	if (t_length >= CF_ENGLISH)
	{
		t_length -= CF_ENGLISH;
		t_locale = g_basic_locale;
	}
	else
		t_locale = MCS_getdatetimelocale();

	if (t_length != P_SHORT && t_length != P_ABBREVIATE && t_length != P_LONG)
		t_length = P_SHORT;

	MCStringRef t_format;
	t_format = MCValueRetain(t_locale -> date_formats[t_length - P_SHORT]);
	unichar_t t_char;
	t_char = MCStringGetCharAtIndex(t_format, 0);
	if (t_char == '!' || t_char == '^')
	{
		MCAutoStringRef t_new;
		/* UNCHECKED */ MCStringCopySubstring(t_format, MCRangeMakeMinMax(1, MCStringGetLength(t_format)), &t_new);
		MCValueAssign(t_format, *t_new);
	}

	// Note that as the locales are either static, or cached the format strings
	// are essentially static.
	r_dateformat = t_format;
}

// This function returns true if the format is a time.
//
static bool MCD_decompose_convert_format(MCExecContext &ctxt, int p_form, const MCDateTimeLocale*& r_locale, MCStringRef &r_format)
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
		t_use_system = ctxt.GetUseSystemDate() == True;

	r_locale = t_use_system ? MCS_getdatetimelocale() : g_basic_locale;

	switch(p_form)
	{
	case CF_INTERNET:
	case CF_INTERNET_DATE:
		r_format = MCSTR(s_internet_date_format);
	break;
	
	case CF_DATEITEMS:
		r_format = MCSTR(s_items_date_format);
	break;

	case CF_TIME:
	case CF_SHORT_TIME:
	case CF_ABBREV_TIME:
		r_format = MCValueRetain(MCtwelvetime ? r_locale -> time_formats[0] : r_locale -> time24_formats[0]);
		return true;
		//r_is_date = false;

	case CF_LONG_TIME:
		r_format = MCValueRetain(MCtwelvetime ? r_locale -> time_formats[1] : r_locale -> time24_formats[1]);
		return true;
		//r_is_date = false;

	case CF_DATE:
	case CF_SHORT_DATE:
		r_format = MCValueRetain(r_locale -> date_formats[0]);
		//r_is_date = true;
	break;

	case CF_ABBREV_DATE:
		r_format = MCValueRetain(r_locale -> date_formats[1]);
		//r_is_date = true;
	break;

	case CF_LONG_DATE:
		r_format = MCValueRetain(r_locale -> date_formats[2]);
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
static bool convert_parse_time(MCExecContext &ctxt, const MCDateTimeLocale *p_locale, MCStringRef p_input, uindex_t &x_offset, MCDateTime& x_datetime, int& x_valid_dateitems)
{
	if (p_locale != g_basic_locale)
		if (convert_parse_time(ctxt, g_basic_locale, p_input, x_offset, x_datetime, x_valid_dateitems))
			return true;

	for(uint4 t_format = 0; t_format < 4; ++t_format)
	{
		MCStringRef t_time_format;
		if (t_format >= 2)
			t_time_format = p_locale -> time24_formats[3 - t_format];
		else
			t_time_format = p_locale -> time_formats[1 - t_format];
		if (datetime_parse(p_locale, ctxt.GetCutOff(), true, t_time_format, p_input, x_offset, x_datetime, x_valid_dateitems))
			return true;
	}

	return false;
}

bool MCD_convert_to_datetime(MCExecContext &ctxt, MCValueRef p_input, Convert_form p_primary_from, Convert_form p_secondary_from, MCDateTime &r_datetime)
{
    bool t_success = true;
    
    MCDateTime t_datetime;
    
	// Make sure 'empty' is not a date
	if (MCValueIsEmpty(p_input))
		return false;
	
	uindex_t t_offset = 0;

	real64_t t_seconds;
	if (p_primary_from != CF_UNDEFINED)
	{

		if (p_primary_from == CF_SECONDS)
		{
			if (!ctxt.ConvertToReal(p_input, t_seconds))
				return false;

			t_success = MCS_secondstodatetime(t_seconds, t_datetime);
		}
		else if (p_primary_from == CF_DATEITEMS)
		{
			int t_valid_dateitems;
			t_valid_dateitems = 0;
            
			MCAutoStringRef t_string;
			if (!ctxt.ConvertToString(p_input, &t_string))
				return false;
			
			if (!datetime_parse(g_basic_locale, ctxt.GetCutOff(), false, MCSTR(s_items_date_format), *t_string, t_offset, t_datetime, t_valid_dateitems) || MCStringIsEmpty(*t_string))
				return false;
            
			datetime_normalize(t_datetime);
            
			t_success = MCS_datetimetouniversal(t_datetime);
		}
		else if (p_primary_from == CF_INTERNET_DATE)
		{
			int t_valid_dateitems;
			t_valid_dateitems = 0;
            
			MCAutoStringRef t_string;
			if (!ctxt.ConvertToString(p_input, &t_string))
				return false;
			
			if (!datetime_parse(g_basic_locale, ctxt.GetCutOff(), false, MCSTR(s_internet_date_format), *t_string, t_offset, t_datetime, t_valid_dateitems) || MCStringIsEmpty(*t_string))
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
			MCStringRef t_date_format;
            
			int t_valid_dateitems;
			t_valid_dateitems = 0;
            
			MCAutoStringRef t_string;
			if (!ctxt.ConvertToString(p_input, &t_string))
				return false;
			
			bool t_is_time;
			t_is_time = MCD_decompose_convert_format(ctxt, p_primary_from, t_locale, t_date_format);
			if (t_is_time && !convert_parse_time(ctxt, t_locale, *t_string, t_offset, t_datetime, t_valid_dateitems))
				return false;
			else if (!t_is_time && !datetime_parse(t_locale, ctxt.GetCutOff(), true, t_date_format, *t_string, t_offset, t_datetime, t_valid_dateitems))
				return false;
            
			if (p_secondary_from != CF_UNDEFINED)
			{
				t_is_time = MCD_decompose_convert_format(ctxt, p_secondary_from, t_locale, t_date_format);
				if (t_is_time && !convert_parse_time(ctxt, t_locale, *t_string, t_offset, t_datetime, t_valid_dateitems))
					return false;
				else if (!t_is_time && !datetime_parse(t_locale, ctxt.GetCutOff(), true, t_date_format, *t_string, t_offset, t_datetime, t_valid_dateitems))
					return false;
			}
			
            // AL-2014-03-04: [[ Bug 12104 ]] If we have more to parse here, then parsing failed.
			if (MCStringGetLength(*t_string) > t_offset)
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
	else if (ctxt.ConvertToReal(p_input, t_seconds))
	{
		if (t_seconds < SECONDS_MIN || t_seconds > SECONDS_MAX)
			return false;
        
		t_success = MCS_secondstodatetime(t_seconds, t_datetime);
	}
	else if (!MCValueIsEmpty(p_input))
	{
		const MCDateTimeLocale *t_locale;
		if (ctxt.GetUseSystemDate())
			t_locale = MCS_getdatetimelocale();
		else
			t_locale = g_basic_locale;
        
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
        
		MCAutoStringRef t_string;
		if (!ctxt.ConvertToString(p_input, &t_string))
			return false;
		
		if (datetime_parse(g_basic_locale, ctxt.GetCutOff(), false, MCSTR(s_items_date_format), *t_string, t_offset, t_datetime, t_valid_dateitems) && !MCStringIsEmpty(*t_string))
		{
			datetime_normalize(t_datetime);
			t_success = MCS_datetimetouniversal(t_datetime);
		}
		else if (datetime_parse(g_basic_locale, ctxt.GetCutOff(), false, MCSTR(s_internet_date_format), *t_string, t_offset, t_datetime, t_valid_dateitems) && MCStringIsEmpty(*t_string))
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
			
			uindex_t t_length;
			t_length = MCStringGetLength(*t_string);
			while(t_offset < t_length && !(t_date_valid && t_time_valid))
			{
				bool t_changed;
				t_changed = false;
                
				if (!t_date_valid)
				{
					for(uint4 t_format = 0; t_format < 3; ++t_format)
						if (datetime_parse(t_locale, ctxt.GetCutOff(), true, t_locale -> date_formats[2 - t_format], *t_string, t_offset, t_datetime, t_valid_dateitems))
						{
							t_date_valid = true;
							t_changed = true;
							break;
						}
				}
                
				if (!t_time_valid)
				{
					if (convert_parse_time(ctxt, t_locale, *t_string, t_offset, t_datetime, t_valid_dateitems))
					{
						t_time_valid = true;
						t_changed = true;
					}
				}
                
				if (!t_changed)
					break;
			}
            
			if (t_offset < t_length)
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

bool MCD_convert_from_datetime(MCExecContext &ctxt, MCDateTime p_datetime, Convert_form p_primary_to, Convert_form p_secondary_to, MCValueRef &r_output)
{
    bool t_success = true;
    
	if (p_primary_to == CF_SECONDS)
	{
		double t_seconds;
        
		t_success = MCS_datetimetoseconds(p_datetime, t_seconds);
        
		if (!t_success)
			return False;
        
		ctxt.SetTheResultToNumber(t_seconds);
		MCNumberRef t_number;
		MCNumberCreateWithReal(t_seconds, t_number);
		r_output = t_number;
	}
	else 
	{
		if (p_primary_to == CF_INTERNET_DATE || p_secondary_to == CF_DATEITEMS)
			p_secondary_to = CF_UNDEFINED;
        
		t_success = MCS_datetimetolocal(p_datetime);
        
		if (!t_success)
			return False;
        
		const MCDateTimeLocale *t_locale;
		MCAutoStringRef t_date_format;
        
		MCStringRef t_buffer;
        
		MCD_decompose_convert_format(ctxt, p_primary_to, t_locale, &t_date_format);
		datetime_format(t_locale, *t_date_format, p_datetime, t_buffer);
		
        
		if (p_secondary_to != CF_UNDEFINED)
		{
            MCAutoStringRef t_secondary_format;
			MCStringRef t_buffer_secondary;
			MCD_decompose_convert_format(ctxt, p_secondary_to, t_locale, &t_secondary_format);
			datetime_format(t_locale, *t_secondary_format, p_datetime, t_buffer_secondary);
			
			MCStringRef t_new;
			/* UNCHECKED */ MCStringFormat(t_new, "%@ %@", t_buffer, t_buffer_secondary);
			MCValueRelease(t_buffer);
			MCValueRelease(t_buffer_secondary);
			t_buffer = t_new;
		}
		
        // AL-2014-03-04: [[ Bug 12104 ]] The result should be empty if conversion is successful.
		//ctxt.SetTheResultToValue(t_buffer);
		r_output = t_buffer;
	}
    
    return t_success;
}

bool MCD_convert(MCExecContext &ctxt, MCValueRef p_input, Convert_form p_primary_from, Convert_form p_secondary_from, Convert_form p_primary_to, Convert_form p_secondary_to, MCStringRef &r_converted)
{
	bool t_success;
	t_success = true;

	// MM-2012-03-01: [[ BUG 10006]] Primaries and secondaries mixed up
	MCDateTime t_datetime;

	t_success = MCD_convert_to_datetime(ctxt, p_input, p_primary_from, p_secondary_from, t_datetime);

	MCAutoValueRef t_output;
    if (t_success)
        t_success = MCD_convert_from_datetime(ctxt, t_datetime, p_primary_to, p_secondary_to, &t_output);
	
	MCAutoStringRef t_string;
	if (t_success)
		t_success = ctxt.ConvertToString(*t_output, &t_string);
	
	if (t_success)
		r_converted = MCValueRetain(*t_string);
    
	return t_success;
}

///////////////////////////////////////////////////////////////////////////////
