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

#if defined(_ANDROID_MOBILE) && !defined(__LP64__)
#include <time64.h>
#define sys_time_t time64_t
#define sys_localtime localtime64
#define sys_mktime mktime64
#define sys_gmtime gmtime64
#define sys_timegm timegm64
#elif defined(_LINUX_DESKTOP) || defined(_LINUX_SERVER) || defined(_DARWIN_SERVER) || defined(__EMSCRIPTEN__) || defined(_ANDROID_MOBILE)
#include <time.h>
#define sys_time_t time_t
#define sys_localtime localtime
#define sys_mktime mktime
#define sys_gmtime gmtime
#define sys_timegm timegm
#else
#error 'sysunxdate.cpp' not supported on this platform
#endif

////////////////////////////////////////////////////////////////////////////////

static void tm_to_datetime(bool p_local, const struct tm *p_tm, MCDateTime& r_datetime)
{
	r_datetime . year = 1900 + p_tm -> tm_year;
	r_datetime . month = p_tm -> tm_mon + 1;
	r_datetime . day = p_tm -> tm_mday;
	r_datetime . hour = p_tm -> tm_hour;
	r_datetime . minute = p_tm -> tm_min;
	r_datetime . second = p_tm -> tm_sec;
	
	// MW-2008-03-15: [[ Bug 6075 ]] The bias member is actually in minutes not hours, so this should
	//   be a division by 60.
	if (p_local)
		r_datetime . bias = p_tm -> tm_gmtoff / 60;
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

////////////////////////////////////////////////////////////////////////////////

void MCS_getlocaldatetime(MCDateTime& r_datetime)
{
	struct tm *tmp; 
	sys_time_t t;
	
	t = time(NULL);
	tmp = sys_localtime(&t);
	tm_to_datetime(true, tmp, r_datetime);
}

bool MCS_datetimetouniversal(MCDateTime& x_datetime)
{
	struct tm tm_local ;
	datetime_to_tm( true, x_datetime, tm_local);
	
	sys_time_t t;
	t = sys_mktime(&tm_local);
	
	if ( t == -1 ) 
		return (false);
	
	struct tm *tm_uni ;
	tm_uni = sys_gmtime(&t);
	
	tm_to_datetime(false, tm_uni, x_datetime) ;
	return (true);
}

bool MCS_datetimetolocal(MCDateTime& x_datetime)
{
	struct tm tmp;
	datetime_to_tm(false, x_datetime, tmp);		// Adjust the datetime to tm format
	
	// MW-2008-03-15: [[ Bug 6075 ]] 'mktime' converts local time to seconds, but we need to convert universal
	//   time to seconds. To do this we use the glibc function 'timegm'.
	sys_time_t t;
	t = sys_timegm(&tmp);							// Convert the tm struct to time_t (number of seconds);
	if (t == -1)
		return false;
	
	struct tm *ltm;
	ltm = sys_localtime(&t);						// Create a tm format structure from the time_t returned above
	tm_to_datetime(true, ltm, x_datetime); 		// And convert back to the datetime that Rev wants.
	
	return ( true ); 
}

bool MCS_datetimetoseconds(const MCDateTime& x_datetime, double& r_seconds)
{
	struct tm tmp;
	datetime_to_tm(false, x_datetime, tmp);
	
	// MW-2008-03-15: [[ Bug 6075 ]] 'mktime' converts local time to seconds, but we need to convert universal
	//   time to seconds. To do this we use the glibc function 'timegm'.
	sys_time_t t ;
	t = sys_timegm(&tmp);
	if ( t == -1 ) 
		return False ;
	r_seconds = (double)t ;
	return True ;
}

bool MCS_secondstodatetime(double p_seconds, MCDateTime& r_datetime)
{
	sys_time_t 
	t = (sys_time_t)(p_seconds + 0.5);
	
	struct tm *tmp;
	
	tmp = sys_gmtime(&t);
	tm_to_datetime(false, tmp, r_datetime);
	
	return true;
}

////////////////////////////////////////////////////////////////////////////////

#if defined(_LINUX_DESKTOP) || defined(_LINUX_SERVER)

#include <locale.h>
#include <langinfo.h>
#include <nl_types.h>

static MCDateTimeLocale *s_datetime_locale = nil;

static MCStringRef string_prepend(MCStringRef t_string, unichar_t t_char)
{
	MCStringRef t_result;
	MCStringFormat(t_result, "%lc%@", t_char, t_string);
	MCValueRelease(t_string);
	return t_result;
}


// PM-2015-09-07: [[ Bug 9942 ]] On Linux, the short/abbr system time should
//  not return the seconds
static MCStringRef remove_seconds(MCStringRef p_input)
{
    MCStringRef t_new_string;
    t_new_string = NULL;
    MCRange t_range;

    if (MCStringFind(p_input, MCRangeMake(0, UINDEX_MAX), MCSTR(":%S"), kMCStringOptionCompareExact, &t_range))
    {
        // If :%S is found, then we remove it
        MCStringRef t_mutable_copy;
        if (MCStringMutableCopy(p_input, t_mutable_copy))
        {
        	if (!MCStringRemove(t_mutable_copy, t_range)
        			|| !MCStringCopyAndRelease(t_mutable_copy, t_new_string))
        		MCValueRelease(t_mutable_copy);
        }
    }
    
    if (t_new_string == NULL)
    {
        // If removing ':%S' failed, or wasn't necessary, we copy the whole string
        t_new_string = MCValueRetain(p_input);
    }
	
	return t_new_string;
}


static MCStringRef query_locale(uint4 t_index)
{
	char *t_buffer;
	MCStringRef t_result;
	t_buffer = nl_langinfo(t_index);
    // SN-2015-04-07: [[ Bug 15161 ]] We get a system string, not a C-string.
    MCStringCreateWithSysString(t_buffer, t_result);
	return t_result;
}

static MCStringRef swap_time_tokens(MCStringRef p_instr)
{
	MCStringRef t_new;
	MCStringMutableCopy(p_instr, t_new);
	MCStringFindAndReplaceChar(t_new, 'l', 'I', kMCStringOptionCompareExact);
	MCStringFindAndReplaceChar(t_new, 'p', 'P', kMCStringOptionCompareExact);
	return t_new;
}

static void cache_locale(void)
{
	if (s_datetime_locale != NULL)
		return;

	s_datetime_locale = new (nothrow) MCDateTimeLocale;

	// OK-2007-05-23: Fix for bug 5035. Adjusted to ensure that first element of weekday names is always Sunday.

	s_datetime_locale -> weekday_names[0] = query_locale(DAY_1);
	s_datetime_locale -> abbrev_weekday_names[0] = query_locale(ABDAY_1);

	for (uint4 t_index = 0; t_index < 6; ++t_index)
	{
		s_datetime_locale -> weekday_names[t_index + 1] = query_locale(DAY_2 + t_index);
		s_datetime_locale -> abbrev_weekday_names[t_index + 1] = query_locale(ABDAY_2 + t_index);
	}

	for(uint4 t_index = 0; t_index < 12; ++t_index)
	{
		s_datetime_locale -> month_names[t_index] = query_locale(MON_1 + t_index);
		s_datetime_locale -> abbrev_month_names[t_index] = query_locale(ABMON_1 + t_index);
	}

	
	s_datetime_locale -> date_formats[0] = string_prepend(query_locale(D_FMT), '^');
	s_datetime_locale -> date_formats[1] = MCSTR("%a, %b %#d, %#Y");
	s_datetime_locale -> date_formats[2] = MCSTR("%A, %B %#d, %#Y");

    MCAutoStringRef t_time_ampm;
    t_time_ampm = query_locale(T_FMT_AMPM);
    
    // AL-2014-01-16: [[ Bug 11672 ]] If the locale doesn't use AM/PM, then always use 24-hour time.
    if (MCStringIsEmpty(*t_time_ampm))
    {
        s_datetime_locale -> time24_formats[0] = MCSTR("!%H:%M");
        s_datetime_locale -> time24_formats[1] = MCSTR("!%H:%M:%S");
    }
    else
    {
        // PM-2015-09-07: [[ Bug 9942 ]] On Linux, the short/abbr system time
        //  should not return the seconds
        MCStringRef t_short_time;
        t_short_time = string_prepend(swap_time_tokens(*t_time_ampm), '!');

        s_datetime_locale -> time_formats[0] = remove_seconds(t_short_time);
        s_datetime_locale -> time_formats[1] = swap_time_tokens(*t_time_ampm);

        // string_prepend() returns a new StringRef, which we must release
        MCValueRelease(t_short_time);
    }
    
	s_datetime_locale -> time24_formats[0] = MCSTR("!%H:%M");
	s_datetime_locale -> time24_formats[1] = MCSTR("!%H:%M:%S");

	s_datetime_locale -> time_morning_suffix = MCSTR("AM");
	s_datetime_locale -> time_evening_suffix = MCSTR("PM");
	

}

const MCDateTimeLocale *MCS_getdatetimelocale(void)
{
	if (s_datetime_locale == NULL)
	{
		char *old_locale, *stored_locale;
		old_locale = setlocale(LC_TIME, NULL);		// Query the current locale
		stored_locale = strdup(old_locale);
	
		setlocale(LC_TIME, "");						// Set the locale using the LANG environment
		cache_locale();
		setlocale(LC_TIME, stored_locale);			// Restore the locale
		free(stored_locale);
	}
	
	return s_datetime_locale;
}

#elif defined(_ANDROID_MOBILE) || defined(_DARWIN_SERVER)

const MCDateTimeLocale *MCS_getdatetimelocale(void)
{
	extern MCDateTimeLocale *g_basic_locale;
	return g_basic_locale;
}

#endif

////////////////////////////////////////////////////////////////////////////////
