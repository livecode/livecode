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

#include "w32prefix.h"

#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"

#include "param.h"
#include "mcerror.h"
#include "execpt.h"
#include "util.h"
#include "date.h"
#include "osspec.h"

#include "globals.h"

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

static char *string_prepend(const char *trunk, const char *prefix)
{
	char *t_new_string;
	t_new_string = new char[strlen(trunk) + strlen(prefix) + 1];
	strcpy(t_new_string, prefix);
	strcat(t_new_string, trunk);
	delete trunk;
	return t_new_string;
}

static char *windows_query_locale(uint4 t_index)
{
	char t_buffer[80];
	if (GetLocaleInfoA(LOCALE_USER_DEFAULT, t_index, t_buffer, 80) == 0)
		t_buffer[0] = '\0';

	return strdup(t_buffer);
}

static void windows_convert_date_format(const char *winstring, char *ansistring)
{
	const char *sptr = winstring;
	char *dptr = ansistring;
	while (*sptr)
	{
		if (*sptr == '\'')
		{
			while (*++sptr && *sptr != '\'')
				*dptr++ = *sptr;
			if (*sptr)
				sptr++;
		}
		else
			if (*sptr == 'd' || *sptr == 'M' || *sptr == 'y')
			{
				char t = *sptr;
				*dptr++ = '%';
				uint2 ntimes = 0;
				while (*sptr == t)
				{
					ntimes++;
					sptr++;
				}
				switch (t)
				{
				case 'd':
					{
						switch (ntimes)
						{
						case 1:
							*dptr++ = '#'; //1
						case 2:
							*dptr++ = 'd'; //01
							break;
						case 3:
							*dptr++ = 'a'; //abbrev weekday
							break;
						case 4:
							*dptr++ = 'A'; //full weekday
							break;
						}
						break;
					}
				case 'M':
					{
						switch (ntimes)
						{
						case 1:
							*dptr++ = '#'; //1
						case 2:
							*dptr++ = 'm'; //01
							break;
						case 3:
							*dptr++ = 'b'; //abbrev month
							break;
						case 4:
							*dptr++ = 'B'; //full month
							break;
						}
					}
					break;
				case 'y':
					{
						switch (ntimes)
						{
						case 1:
							*dptr++ = '#'; //1
						case 2:
							*dptr++ = 'y'; //abbrev month
							break;
						case 4:
							*dptr++ = 'Y'; //full month
							break;
						}
					}
					break;
				}
			}
			else
				*dptr++ = *sptr++;
	}
	*dptr = '\0';
}

static void windows_convert_time_format(const char *winstring, char *ansistring)
{
	const char *sptr = winstring;
	char *dptr = ansistring;
	while (*sptr)
	{
		if (*sptr == '\'')
		{
			while (*++sptr && *sptr != '\'')
				*dptr++ = *sptr;
			if (*sptr)
				sptr++;
		}
		else
			if (*sptr == 'h' || *sptr == 'H' || *sptr == 'm' || *sptr == 's' || *sptr == 't')
			{
				char t = *sptr;
				*dptr++ = '%';
				uint2 ntimes = 0;
				while (*sptr == t)
				{
					ntimes++;
					sptr++;
				}
				switch (t)
				{
				case 'h':
					{
						switch(ntimes)
						{
						case 1:
							*dptr++ = '#';
						case 2:
							*dptr++ = 'I';
							break;
						}
					}
				break;
				case 'H':
					{
						switch(ntimes)
						{
						case 1:
							*dptr++ = '#';
						case 2:
							*dptr++ = 'H';
							break;
						}
					}
				break;
				case 'm':
					{
						switch(ntimes)
						{
						case 1:
							*dptr++ = '#';
						case 2:
							*dptr++ = 'M';
							break;
						}
					}
				break;
				case 's':
					{
						switch(ntimes)
						{
						case 1:
							*dptr++ = '#';
						case 2:
							*dptr++ = 'S';
							break;
						}
					}
				break;
				case 't':
					*dptr++ = 'p';
				break;
				}
			}
			else
				*dptr++ = *sptr++;
	}
	*dptr = '\0';
}

static void windows_abbreviate_format(char *sptr)
{
	while (*sptr)
	{
		if (*sptr == '%')
		{
			if (*++sptr == '#')
				sptr++; //skip
			if (*sptr == 'A' || *sptr == 'B')
				*sptr = MCS_tolower(*sptr);
		}
		sptr++;
	}
}

static char *windows_query_date_format(uint4 p_index, bool p_abbreviate)
{
	char t_buffer[80];
	if (GetLocaleInfoA(LOCALE_USER_DEFAULT, p_index, t_buffer, 80) == 0)
		t_buffer[0] = '\0';

	char t_converted_buffer[128];
	windows_convert_date_format(t_buffer, t_converted_buffer);
	if (p_abbreviate)
		windows_abbreviate_format(t_converted_buffer);

	return strdup(t_converted_buffer);
}

static char *windows_query_time_format(uint4 p_index)
{
	char t_buffer[80];
	if (MCmajorosversion >= 0x0601)
	{
		// some windows 7 properties only accessible through unicode version of GetLocaleInfo()
		WCHAR t_wbuffer[80];
		DWORD t_length;
		if ((t_length = GetLocaleInfoW(LOCALE_USER_DEFAULT, p_index, t_wbuffer, 80)) == 0)
			t_wbuffer[0] = '\0';

		// convert unicode to ansi
		uint32_t t_out_bytes = 0;
		MCS_unicodetomultibyte((char*)t_wbuffer, t_length * sizeof(WCHAR), t_buffer, 80, t_out_bytes, LCH_ENGLISH);
	}
	else
	{
		if (GetLocaleInfoA(LOCALE_USER_DEFAULT, p_index, t_buffer, 80) == 0)
			t_buffer[0] = '\0';
	}

	char t_converted_buffer[128];
	windows_convert_time_format(t_buffer, t_converted_buffer);

	return strdup(t_converted_buffer);
}

static void windows_cache_locale(void)
{
	if (s_datetime_locale != NULL)
		return;

	s_datetime_locale = new MCDateTimeLocale;

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

	s_datetime_locale -> date_formats[0] = string_prepend(windows_query_date_format(LOCALE_SSHORTDATE, false), "^");
	s_datetime_locale -> date_formats[1] = windows_query_date_format(LOCALE_SLONGDATE, true);
	s_datetime_locale -> date_formats[2] = windows_query_date_format(LOCALE_SLONGDATE, false);

	// AL-2013-02-08: [[ Bug 9942 ]] Allow appropriate versions of Windows to retrieve the short time format.
	if (MCmajorosversion >= 0x0601)
		s_datetime_locale -> time_formats[0] = string_prepend(windows_query_time_format(LOCALE_SSHORTTIME), "!");
	else
		s_datetime_locale -> time_formats[0] = string_prepend(windows_query_time_format(LOCALE_STIMEFORMAT), "!");

	s_datetime_locale -> time_formats[1] = string_prepend(windows_query_time_format(LOCALE_STIMEFORMAT), "!");

	s_datetime_locale -> time24_formats[0] = string_prepend(windows_query_time_format(LOCALE_STIMEFORMAT), "!");
	s_datetime_locale -> time24_formats[1] = string_prepend(windows_query_time_format(LOCALE_STIMEFORMAT), "!");
	
	// AL-2013-02-08: [[ Bug 9945 ]] Retrieve locale-specific AM & PM designators.
	s_datetime_locale -> time_morning_suffix = windows_query_locale(LOCALE_S1159);
	s_datetime_locale -> time_evening_suffix = windows_query_locale(LOCALE_S2359);
}

const MCDateTimeLocale *MCS_getdatetimelocale(void)
{
	windows_cache_locale();
	return s_datetime_locale;
}

////////////////////////////////////////////////////////////////////////////////
