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

#include "date.h"

#include <CoreFoundation/CoreFoundation.h>

////////////////////////////////////////////////////////////////////////////////

static void datetime_to_time(bool p_local, const MCDateTime& p_datetime, CFAbsoluteTime& r_time);
static void time_to_datetime(bool p_local, CFAbsoluteTime p_time, MCDateTime& r_datetime);
static MCDateTimeLocale *do_cache_locale(void);

void MCS_getlocaldatetime(MCDateTime& r_datetime)
{
	CFAbsoluteTime t_system_time;
	t_system_time = CFAbsoluteTimeGetCurrent();
	
	time_to_datetime(true, t_system_time, r_datetime);
}

bool MCS_datetimetouniversal(MCDateTime& x_datetime)
{
	CFAbsoluteTime t_time;
	datetime_to_time(true, x_datetime, t_time);
	time_to_datetime(false, t_time, x_datetime);
	return true;
}

bool MCS_datetimetolocal(MCDateTime& x_datetime)
{
	CFAbsoluteTime t_time;
	datetime_to_time(false, x_datetime, t_time);
	time_to_datetime(true, t_time, x_datetime);
	return true;
}

bool MCS_datetimetoseconds(const MCDateTime& p_datetime, double& r_seconds)
{
	CFAbsoluteTime t_time;
	datetime_to_time(false, p_datetime, t_time); 
	r_seconds = t_time + kCFAbsoluteTimeIntervalSince1970;
	return true;
}

bool MCS_secondstodatetime(double p_seconds, MCDateTime& r_datetime)
{
	CFAbsoluteTime t_time;
	t_time = p_seconds - kCFAbsoluteTimeIntervalSince1970;
	time_to_datetime(false, t_time, r_datetime);	
	return true;
}

const MCDateTimeLocale *MCS_getdatetimelocale(void)
{
	return do_cache_locale();
}
	
////////////////////////////////////////////////////////////////////////////////

static MCDateTimeLocale *s_locale_info = NULL;
static CFTimeZoneRef s_locale_timezone = NULL;

char *osx_cfstring_to_cstring(CFStringRef p_string, bool p_release = true)
{
	bool t_success;
	t_success = true;
	
	if (p_string == NULL)
		t_success = false;
	
	char *t_cstring;
	t_cstring = NULL;
	if (t_success)
	{
		CFIndex t_string_length;
		t_string_length = CFStringGetLength(p_string);
		
		CFIndex t_buffer_size;
		t_buffer_size = CFStringGetMaximumSizeForEncoding(t_string_length, kCFStringEncodingMacRoman) + 1;
		t_cstring = (char *)malloc(t_buffer_size);
		
		if (t_cstring != NULL)
		{
			// MW-2012-03-15: [[ Bug 9935 ]] Use CFStringGetBytes() so that '?' is substituted for any non-
			//   mappable chars.
			CFIndex t_used;
			CFStringGetBytes(p_string, CFRangeMake(0, CFStringGetLength(p_string)), kCFStringEncodingMacRoman, '?', False, (UInt8*)t_cstring, t_buffer_size, &t_used);
			t_cstring[t_used] = '\0';
		}
		else
			t_success = false;
		
		if (t_success)
			t_cstring = (char *)realloc(t_cstring, strlen(t_cstring) + 1);
	}
	
	if (!t_success)
	{
		if (t_cstring != NULL)
			free(t_cstring);
		t_cstring = NULL;
	}
	
	if (p_string != NULL && p_release)
		CFRelease(p_string);
	
	return t_cstring;
}

static char *osx_cf_format_time(CFLocaleRef p_locale, CFStringRef p_format, CFAbsoluteTime p_time)
{
	bool t_success;
	t_success = true;
	
	CFDateFormatterRef t_formatter;
	t_formatter = NULL;
	if (t_success)
	{
		t_formatter = CFDateFormatterCreate(NULL, p_locale, kCFDateFormatterNoStyle, kCFDateFormatterNoStyle);
		if (t_formatter == NULL)
			t_success = false;
	}
	
	CFTimeZoneRef t_gmt_timezone;
	t_gmt_timezone = NULL;
	if (t_success)
	{
		t_gmt_timezone = CFTimeZoneCreateWithTimeIntervalFromGMT(NULL, 0);
		if (t_gmt_timezone == NULL)
			t_success = false;
	}
	
	if (t_success)
		CFDateFormatterSetProperty(t_formatter, kCFDateFormatterTimeZone, t_gmt_timezone);
	
	char *t_formatted_time;
	t_formatted_time = NULL;
	if (t_success)
	{
		CFDateFormatterSetFormat(t_formatter, p_format);
		t_formatted_time = osx_cfstring_to_cstring(CFDateFormatterCreateStringWithAbsoluteTime(NULL, t_formatter, p_time));
		if (t_formatted_time == NULL)
			t_success = false;
	}
	
	if (t_gmt_timezone != NULL)
		CFRelease(t_gmt_timezone);
	
	if (t_formatter != NULL)
		CFRelease(t_formatter);
	
	return t_formatted_time;
}

static char *osx_cf_fetch_format(CFLocaleRef p_locale, CFDateFormatterStyle p_date_style, CFDateFormatterStyle p_time_style)
{
	bool t_success;
	t_success = true;
	
	CFDateFormatterRef t_formatter;
	t_formatter = NULL;
	if (t_success)
	{
		t_formatter = CFDateFormatterCreate(NULL, p_locale, p_date_style, p_time_style);
		if (t_formatter == NULL)
			t_success = false;
	}
	
	if (t_success)
	{
		CFStringRef t_calendar;
		t_calendar = (CFStringRef)CFDateFormatterCopyProperty(t_formatter, kCFDateFormatterCalendarName);
		if (t_calendar != kCFGregorianCalendar)
			t_success = false;
		
		if (t_calendar != NULL)
			CFRelease(t_calendar);
	}
	
	char *t_icu_format;
	t_icu_format = NULL;
	if (t_success)
	{
		t_icu_format = osx_cfstring_to_cstring(CFDateFormatterGetFormat(t_formatter), false);
		if (t_icu_format == NULL)
			t_success = false;
	}
	
	char *t_format;
	t_format = NULL;
	if (t_success)
	{
		t_format = (char *)malloc(strlen(t_icu_format) * 3 + 2);
		
		char *t_buffer;
		t_buffer = t_format;
		
		if (p_date_style == kCFDateFormatterShortStyle)
			*t_buffer++ = '^';
		else if (p_time_style != kCFDateFormatterNoStyle)
			*t_buffer++ = '!';
		
		char *t_input;
		t_input = t_icu_format;
		
		while(*t_input != '\0' && t_success)
		{
			char t_token;
			t_token = *t_input++;
			
			if (isalpha(t_token))
			{
				unsigned int t_count;
				t_count = 1;
				while(*t_input != '\0' && *t_input == t_token)
					t_count += 1, t_input++;
				
				const char *t_component;
				t_component = NULL;
				
				switch(t_token)
				{
					case 'y': // YEAR
						if (t_count == 2)
							t_component = "%y";
						else
							t_component = "%Y";
						break;
					case 'M': // MONTH IN YEAR
						if (t_count == 1)
							t_component = "%#m";
						else if (t_count == 2)
							t_component = "%m";
						else if (t_count == 3)
							t_component = "%b";
						else
							t_component = "%B";
						break;
					case 'd': // DAY IN MONTH
						if (t_count == 1)
							t_component = "%#d";
						else
							t_component = "%d";
						break;
					case 'E': // DAY IN WEEK
						if (t_count < 4)
							t_component = "%a";
						else
							t_component = "%A";
						break;
					case 'K': // ZERO-BASED TWELVE HOUR
						if (t_count == 1)
							t_component = "%#J";
						else
							t_component = "%J";
						break;
					case 'h': // ONE-BASED TWELVE HOUR
						if (t_count == 1)
							t_component = "%#I";
						else
							t_component = "%I";
						break;
					case 'H': // ZERO-BASED TWENTY-FOUR HOUR
						if (t_count == 1)
							t_component = "%#H";
						else
							t_component = "%H";
						break;
					case 'm': // MINUTE
						if (t_count == 1)
							t_component = "%#M";
						else
							t_component = "%M";
						break;
					case 's': // SECOND
						if (t_count == 1)
							t_component = "%#S";
						else
							t_component = "%S";
						break;
					case 'a': // PERIOD
						t_component = "%p";
						break;
						
					case 'G': // ERA
					case 'Z': // TIME ZONE
						t_component = NULL;
						break;
						
					case 'S': // MILLISECOND
					case 'D': // DAY IN YEAR
					case 'F': // DAY OF WEEK IN MONTH
					case 'w': // WEEK IN YEAR
					case 'W': // WEEK IN MONTH
					case 'k': // ONE-BASED TWENTY-FOUR HOUR
					default: 
						t_success = false;
						break;
				}
				
				if (t_component != NULL)
				{
					unsigned int t_component_length;
					t_component_length = strlen(t_component);
					memcpy(t_buffer, t_component, t_component_length);
					t_buffer += t_component_length;
				}
			}
			else if (t_token == '\'')
			{
				if (*t_input == '\'')
					*t_buffer++ = '\'';
				else
				{
					// MW-2013-02-13: [[ Bug 9947 ]] Make sure we advance the buffer ptr when
					//   copying verbatim chars!
					while(*t_input != '\0' && *t_input != '\'')
						*t_buffer++ = *t_input++;
				}
				
				t_input++;
			}
			else
				*t_buffer++ = t_token;
		}
		
		*t_buffer++ = '\0';
		
		t_format = (char *)realloc(t_format, t_buffer - t_format);
	}
	
	if (t_icu_format != NULL)
		free(t_icu_format);
	
	if (t_formatter != NULL)
		CFRelease(t_formatter);
	
	if (!t_success)
	{
		if (t_format != NULL)
			free(t_format);
		
		t_format = NULL;
	}
	
	return t_format;
}

static CFAbsoluteTime osx_cf_datetime_to_time(int p_year, int p_month, int p_day, int p_hour, int p_minute, int p_second)
{
	CFGregorianDate t_date;
	t_date . year = p_year;
	t_date . month = p_month;
	t_date . day = p_day;
	t_date . hour = p_hour;
	t_date . minute = p_minute;
	t_date . second = p_second;
	
	CFAbsoluteTime t_time;
	t_time = CFGregorianDateGetAbsoluteTime(t_date, NULL);
	
	return t_time;
}

static bool osx_cf_cache_locale(MCDateTimeLocale *p_info)
{
	bool t_success;
	t_success = true;
	
	if (t_success)
	{
		if (CFDateFormatterCreate == NULL)
			t_success = false;
	}
	
	CFLocaleRef t_locale;
	t_locale = NULL;
	if (t_success)
	{
		t_locale = CFLocaleCopyCurrent();
		if (t_locale == NULL)
			t_success = false;
	}
	
	char *t_short_time_format;
	t_short_time_format = NULL;
	if (t_success)
	{
		t_short_time_format = osx_cf_fetch_format(t_locale, kCFDateFormatterNoStyle, kCFDateFormatterShortStyle);
		if (t_short_time_format == NULL)
			t_success = false;
	}
	
	char *t_long_time_format;
	t_long_time_format = NULL;
	if (t_success)
	{
		t_long_time_format = osx_cf_fetch_format(t_locale, kCFDateFormatterNoStyle, kCFDateFormatterMediumStyle);
		if (t_long_time_format == NULL)
			t_success = false;
	}
	
	char *t_short_date_format;
	t_short_date_format = NULL;
	if (t_success)
	{
		t_short_date_format = osx_cf_fetch_format(t_locale, kCFDateFormatterShortStyle, kCFDateFormatterNoStyle);
		if (t_short_date_format == NULL)
			t_success = false;
	}	
	
	char *t_long_date_format;
	t_long_date_format = NULL;
	if (t_success)
	{
		t_long_date_format = osx_cf_fetch_format(t_locale, kCFDateFormatterFullStyle, kCFDateFormatterNoStyle);
		if (t_long_date_format == NULL)
			t_success = false;
	}
	
	if (t_success)
	{
		for(unsigned int t_month = 1; t_month <= 12; ++t_month)
		{
			CFAbsoluteTime t_time;
			t_time = osx_cf_datetime_to_time(2001, t_month, 1, 12, 0, 0);
			p_info -> month_names[t_month - 1] = osx_cf_format_time(t_locale, CFSTR("MMMM"), t_time);
			p_info -> abbrev_month_names[t_month - 1] = osx_cf_format_time(t_locale, CFSTR("MMM"), t_time);
		}
		
		for(unsigned int t_day = 1; t_day <= 7; ++t_day)
		{
			CFAbsoluteTime t_time;
			t_time = osx_cf_datetime_to_time(2007, 4, 7 + t_day, 12, 0, 0);
			p_info -> weekday_names[t_day - 1] = osx_cf_format_time(t_locale, CFSTR("EEEE"), t_time);
			p_info -> abbrev_weekday_names[t_day - 1] = osx_cf_format_time(t_locale, CFSTR("EEE"), t_time);
		}
		
		p_info -> date_formats[0] = t_short_date_format;
		char *t_abbrev_date_format;
		t_abbrev_date_format = strdup(t_long_date_format);
		for(uint4 i = 0; t_abbrev_date_format[i] != '\0'; ++i)
			if (t_abbrev_date_format[i] == '%')
				if (t_abbrev_date_format[i + 1] == 'A')
					t_abbrev_date_format[i + 1] = 'a';
				else if (t_abbrev_date_format[i + 1] == 'B')
					t_abbrev_date_format[i + 1] = 'b';
		p_info -> date_formats[1] = t_abbrev_date_format;
		p_info -> date_formats[2] = t_long_date_format;
		
		p_info -> time_formats[0] = t_short_time_format;
		p_info -> time_formats[1] = t_long_time_format;
		
		p_info -> time24_formats[0] = t_short_time_format;
		p_info -> time24_formats[1] = t_long_time_format;
		
		p_info -> time_morning_suffix = osx_cf_format_time(t_locale, CFSTR("a"), osx_cf_datetime_to_time(2001, 1, 1, 6, 0, 0));
		p_info -> time_evening_suffix = osx_cf_format_time(t_locale, CFSTR("a"), osx_cf_datetime_to_time(2001, 1, 1, 18, 0, 0));
	}
	else
	{
		free(t_short_date_format);
		free(t_long_date_format);
		free(t_short_time_format);
		free(t_long_time_format);
	}
	
	if (t_locale != NULL)
		CFRelease(t_locale);
	
	return t_success;
}

static MCDateTimeLocale *do_cache_locale(void)
{
	extern MCDateTimeLocale g_english_locale;
	
	if (s_locale_info != nil)
		return s_locale_info;
	
	s_locale_timezone = CFTimeZoneCopySystem();
	
	s_locale_info = new MCDateTimeLocale;
	
	if (!osx_cf_cache_locale(s_locale_info))
		*s_locale_info = g_english_locale;
	
	// MM-2011-09-14: [[ BZ 9721 ]] Make sure a value is returned
	return s_locale_info;
}

static void time_to_datetime(bool p_local, CFAbsoluteTime p_time, MCDateTime& r_datetime)
{
	do_cache_locale();
	
	CFGregorianDate t_sys_datetime;
	t_sys_datetime = CFAbsoluteTimeGetGregorianDate(p_time, p_local ? s_locale_timezone : NULL);
	
	r_datetime . year = t_sys_datetime . year;
	r_datetime . month = t_sys_datetime . month;
	r_datetime . day = t_sys_datetime . day;
	r_datetime . hour = t_sys_datetime . hour;
	r_datetime . minute = t_sys_datetime . minute;
	r_datetime . second = (int)t_sys_datetime . second;
	
	if (p_local)
		r_datetime . bias = ((int)CFTimeZoneGetSecondsFromGMT(s_locale_timezone, p_time)) / 60;
	else
		r_datetime . bias = 0;
}

static void datetime_to_time(bool p_local, const MCDateTime& p_datetime, CFAbsoluteTime& r_time)
{
	do_cache_locale();
	
	CFGregorianDate t_sys_datetime;
	t_sys_datetime . year = p_datetime . year;
	t_sys_datetime . month = p_datetime . month;
	t_sys_datetime . day = p_datetime . day;
	t_sys_datetime . hour = p_datetime . hour;
	t_sys_datetime . minute = p_datetime . minute;
	t_sys_datetime . second = p_datetime . second;
	
	r_time = CFGregorianDateGetAbsoluteTime(t_sys_datetime, p_local ? s_locale_timezone : NULL);
}

////////////////////////////////////////////////////////////////////////////////
