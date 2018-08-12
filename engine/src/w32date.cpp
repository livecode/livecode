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
#include "w32dsk-legacy.h"

#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"

#include "param.h"
#include "mcerror.h"

#include "util.h"
#include "date.h"
#include "osspec.h"

#include "globals.h"

#include "foundation-locale.h"

#if !defined(LOCALE_SSHORTTIME)
#define LOCALE_SSHORTTIME             0x00000079   // Returns the preferred short time format (ie: no seconds, just h:mm)
#endif

////////////////////////////////////////////////////////////////////////////////

static void tm_to_datetime(bool p_local, const struct tm& p_tm, MCDateTime& r_datetime)
{
	r_datetime . year = 1900 + p_tm . tm_year;
	r_datetime . month = p_tm . tm_mon + 1;
	r_datetime . day = p_tm . tm_mday;
	r_datetime . hour = p_tm . tm_hour;
	r_datetime . minute = p_tm . tm_min;
	r_datetime . second = p_tm . tm_sec;
	if (p_local)
	{
		r_datetime . bias = -_timezone / 60;
		if (p_tm . tm_isdst)
			r_datetime . bias -= _dstbias / 60;
	}
	else
		r_datetime . bias = 0;
}

static void datetime_to_tm(bool p_local, const MCDateTime& p_datetime, struct tm& r_tm)
{
	r_tm . tm_year = p_datetime . year - 1900;
	r_tm . tm_mon = p_datetime . month - 1;
	r_tm . tm_mday = p_datetime . day;
	r_tm . tm_hour = p_datetime . hour;
	r_tm . tm_min = p_datetime . minute;
	r_tm . tm_sec = p_datetime . second;
	if (p_local)
		r_tm . tm_isdst = -1;
	else
		r_tm . tm_isdst = 0;
}

void MCS_getlocaldatetime(MCDateTime& r_datetime)
{
	__time64_t t_time;
	_time64(&t_time);

	struct tm t_tm;
	_localtime64_s(&t_tm, &t_time);

	tm_to_datetime(true, t_tm, r_datetime);
}

bool MCS_datetimetolocal(MCDateTime& x_datetime)
{
	struct tm t_universal_datetime;
	datetime_to_tm(false, x_datetime, t_universal_datetime);

	__time64_t t_time;
	t_time = _mkgmtime64(&t_universal_datetime);
	if (t_time == -1)
		return false;

	struct tm t_local_tm;
	_localtime64_s(&t_local_tm, &t_time);

	tm_to_datetime(true, t_local_tm, x_datetime);

	return true;
}

bool MCS_datetimetouniversal(MCDateTime& x_datetime)
{
	struct tm t_local_datetime;
	datetime_to_tm(true, x_datetime, t_local_datetime);

	__time64_t t_universal_time;
	t_universal_time = _mktime64(&t_local_datetime);
	if (t_universal_time == -1)
		return false;

	struct tm t_universal_tm;
	_gmtime64_s(&t_universal_tm, &t_universal_time);

	tm_to_datetime(false, t_universal_tm, x_datetime);

	return true;
}

bool MCS_datetimetoseconds(const MCDateTime& p_datetime, double& r_seconds)
{
	struct tm t_universal_tm;
	datetime_to_tm(false, p_datetime, t_universal_tm);

	__time64_t t_universal_time;
	t_universal_time = _mkgmtime64(&t_universal_tm);
	if (t_universal_time == -1)
		return false;

	r_seconds = (double)t_universal_time;

	return true;
}

bool MCS_secondstodatetime(double p_seconds, MCDateTime& r_datetime)
{
	__time64_t t_universal_time;
	t_universal_time = (__time64_t)(p_seconds + 0.5);

	struct tm t_universal_tm;
	_gmtime64_s(&t_universal_tm, &t_universal_time);

	tm_to_datetime(false, t_universal_tm, r_datetime);

	return true;
}

////////////////////////////////////////////////////////////////////////////////

static MCDateTimeLocale *s_datetime_locale = NULL;

static MCStringRef string_prepend(MCStringRef p_string, unichar_t p_prefix)
{
	MCStringRef t_new;
	MCStringFormat(t_new, "%lc%@", p_prefix, p_string);
	return t_new;
}

static MCStringRef windows_query_locale(uint4 t_index)
{
	// Allocate a buffer for the locale information
	int t_buf_size;
	t_buf_size = GetLocaleInfoW(LOCALE_USER_DEFAULT, t_index, NULL, 0);
	wchar_t* t_buffer = new (nothrow) wchar_t[t_buf_size];
	
	// Get the locale information and create a StringRef from it
	if (GetLocaleInfoW(LOCALE_USER_DEFAULT, t_index, t_buffer, t_buf_size) == 0)
		return MCValueRetain(kMCEmptyString);
	MCStringRef t_string;
	MCStringCreateWithChars(t_buffer, MCU_max(0, t_buf_size - 1), t_string);
	delete[] t_buffer;
	
	return t_string;
}

static MCStringRef windows_convert_date_format(MCStringRef p_format)
{
	MCStringRef t_output;
	MCStringCreateMutable(0, t_output);
	uindex_t t_offset = 0;
	
	while (t_offset < MCStringGetLength(p_format))
	{
		unichar_t t_char;
		t_char = MCStringGetCharAtIndex(p_format, t_offset++);
		
		if (t_char == '\'')
		{
			// Copy quoted strings to the output with no conversion
			while ((t_char = MCStringGetCharAtIndex(p_format, t_offset++)) != '\'')
				   MCStringAppendChar(t_output, t_char);
		}
		else
		{
			// Is this a day/month/year specifier?
			if (t_char == 'd' || t_char == 'M' || t_char == 'y')
			{
				// Count the number of consecutive identical characters
				unichar_t t_want = t_char;
				int t_count = 1;
				while (MCStringGetCharAtIndex(p_format, t_offset) == t_want)
                {
					t_count++;
                    t_offset++;
                }
				
				// Append the correct formatting instruction
				switch (t_char)
				{
				case 'd':
					if (t_count == 1)
						MCStringAppendFormat(t_output, "%%#d");
					else if (t_count == 2)
						MCStringAppendFormat(t_output, "%%d");
					else if (t_count == 3)
						MCStringAppendFormat(t_output, "%%a");
					else if (t_count == 4)
						MCStringAppendFormat(t_output, "%%A");
					break;
						
				case 'M':
					if (t_count == 1)
						MCStringAppendFormat(t_output, "%%#m");
					else if (t_count == 2)
						MCStringAppendFormat(t_output, "%%m");
					else if (t_count == 3)
						MCStringAppendFormat(t_output, "%%b");
					else if (t_count == 4)
						MCStringAppendFormat(t_output, "%%B");
					break;
						
				case 'y':
					if (t_count == 1)
						MCStringAppendFormat(t_output, "%%#y");
					else if (t_count == 2)
						MCStringAppendFormat(t_output, "%%y");
					else if (t_count == 4)
						MCStringAppendFormat(t_output, "%%Y");
					break;
				}
			}
			else
			{
				// Unknown character, copy it to the output
				MCStringAppendChar(t_output, t_char);
			}
		}
	}
	
	MCValueRelease(p_format);
	return t_output;
}

static MCStringRef windows_convert_time_format(MCStringRef p_format)
{
	MCStringRef t_output;
	MCStringCreateMutable(0, t_output);
	uindex_t t_offset = 0;
	
	while (t_offset < MCStringGetLength(p_format))
	{
		unichar_t t_char;
		t_char = MCStringGetCharAtIndex(p_format, t_offset++);
		
		if (t_char == '\'')
		{
			// Copy quoted strings to the output with no conversion
			while ((t_char = MCStringGetCharAtIndex(p_format, t_offset++)) != '\'')
				MCStringAppendChar(t_output, t_char);
		}
		else
		{
			// Is this a day/month/year specifier?
			if (t_char == 'h' || t_char == 'H' || t_char == 'm' || t_char == 's' || t_char == 't')
			{
				// Count the number of consecutive identical characters
				unichar_t t_want = t_char;
				int t_count = 1;
				while ((t_char = MCStringGetCharAtIndex(p_format, t_offset)) == t_want)
                {
                    t_count++;
                    t_offset++;
                }
				
				// Append the correct formatting instruction
				switch (t_want)
				{
					case 'h':
						if (t_count == 1)
							MCStringAppendFormat(t_output, "%%#I");
						else if (t_count == 2)
							MCStringAppendFormat(t_output, "%%I");
						break;
						
					case 'H':
						if (t_count == 1)
							MCStringAppendFormat(t_output, "%%#H");
						else if (t_count == 2)
							MCStringAppendFormat(t_output, "%%H");
						break;
						
					case 'm':
						if (t_count == 1)
							MCStringAppendFormat(t_output, "%%#M");
						else if (t_count == 2)
							MCStringAppendFormat(t_output, "%%M");
						break;
						
					case 's':
						if (t_count == 1)
							MCStringAppendFormat(t_output, "%%#S");
						else if (t_count == 2)
							MCStringAppendFormat(t_output, "%%S");
						break;
						
					case 't':
						MCStringAppendFormat(t_output, "%%p");
				}
			}
			else
			{
				// Unknown character, copy it to the output
				MCStringAppendChar(t_output, t_char);
			}
		}
	}
	
	MCValueRelease(p_format);
	return t_output;
}

static MCStringRef windows_abbreviate_format(MCStringRef p_format)
{
	MCStringRef t_new;
	/* UNCHECKED */ MCStringMutableCopyAndRelease(p_format, t_new);
	/* UNCHECKED */ MCStringFindAndReplaceChar(t_new, 'A', 'a', kMCStringOptionCompareExact);
	/* UNCHECKED */ MCStringFindAndReplaceChar(t_new, 'B', 'b', kMCStringOptionCompareExact);
	return t_new;
}

static MCStringRef windows_query_date_format(uint4 p_index, bool p_abbreviate)
{
	MCStringRef t_win_format = windows_query_locale(p_index);
	MCStringRef t_format = windows_convert_date_format(t_win_format);
	
	if (p_abbreviate)
		return windows_abbreviate_format(t_format);
	
	return t_format;
}

static MCStringRef windows_query_time_format(uint4 p_index)
{
	MCStringRef t_win_format = windows_query_locale(p_index);
	return windows_convert_time_format(t_win_format);
}

static void windows_cache_locale(void)
{
	if (s_datetime_locale != NULL)
		return;

	s_datetime_locale = new (nothrow) MCDateTimeLocale;

	// OK-2007-05-23: Fix for bug 5035. Adjusted to ensure that first element of weekday names is always Sunday.
	s_datetime_locale -> weekday_names[0] = windows_query_locale(LOCALE_SDAYNAME7);
	s_datetime_locale -> abbrev_weekday_names[0] = windows_query_locale(LOCALE_SABBREVDAYNAME7);

	for (uint4 t_index = 0; t_index < 6; ++t_index)
	{
		s_datetime_locale -> weekday_names[t_index + 1] = windows_query_locale(LOCALE_SDAYNAME1 + t_index);
		s_datetime_locale -> abbrev_weekday_names[t_index + 1] = windows_query_locale(LOCALE_SABBREVDAYNAME1 + t_index);
	}

	for(uint4 t_index = 0; t_index < 12; ++t_index)
	{
		s_datetime_locale -> month_names[t_index] = windows_query_locale(LOCALE_SMONTHNAME1 + t_index);
		s_datetime_locale -> abbrev_month_names[t_index] = windows_query_locale(LOCALE_SABBREVMONTHNAME1 + t_index);
	}

	s_datetime_locale -> date_formats[0] = string_prepend(windows_query_date_format(LOCALE_SSHORTDATE, false), '^');
	s_datetime_locale -> date_formats[1] = windows_query_date_format(LOCALE_SLONGDATE, true);
	s_datetime_locale -> date_formats[2] = windows_query_date_format(LOCALE_SLONGDATE, false);

	// AL-2013-02-08: [[ Bug 9942 ]] Allow appropriate versions of Windows to retrieve the short time format.
	if (MCmajorosversion >= 0x0601)
		s_datetime_locale -> time_formats[0] = string_prepend(windows_query_time_format(LOCALE_SSHORTTIME), '!');
	else
		s_datetime_locale -> time_formats[0] = string_prepend(windows_query_time_format(LOCALE_STIMEFORMAT), '!');

	s_datetime_locale -> time_formats[1] = string_prepend(windows_query_time_format(LOCALE_STIMEFORMAT), '!');

	s_datetime_locale -> time24_formats[0] = string_prepend(windows_query_time_format(LOCALE_STIMEFORMAT), '!');
	s_datetime_locale -> time24_formats[1] = string_prepend(windows_query_time_format(LOCALE_STIMEFORMAT), '!');
	
	// AL-2013-02-08: [[ Bug 9945 ]] Retrieve locale-specific AM & PM designators.
	s_datetime_locale -> time_morning_suffix = windows_query_locale(LOCALE_S1159);
	s_datetime_locale -> time_evening_suffix = windows_query_locale(LOCALE_S2359);
}

const MCDateTimeLocale *MCS_getdatetimelocale(void)
{
	windows_cache_locale();
	return s_datetime_locale;
}

MCLocaleRef MCS_getsystemlocale()
{
	// Get the identifier for the system locale
	LCID t_system_lcid;
	t_system_lcid = GetUserDefaultLCID();

	// Create a locale object
	MCLocaleRef t_locale;
	/* UNCHECKED */ MCLocaleCreateWithLCID(t_system_lcid, t_locale);
	return t_locale;
}

////////////////////////////////////////////////////////////////////////////////
