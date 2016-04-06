/*                                                                     -*-c++-*-
Copyright (C) 2015 LiveCode Ltd.

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

#include <foundation.h>
#include <foundation-auto.h>

#include <time.h>

#ifndef _WINDOWS
#  include <sys/time.h>
#else
#  include <windows.h>
#endif

#if defined(__WINDOWS__)
static bool
MCDateGetTimeInfo(bool t_is_local,
                  struct tm & r_timeinfo,
                  long & r_timezone)
{
	if (t_is_local)
	{
		_get_timezone(&r_timezone);
	}
	else
	{
		r_timezone = 0;
	}

	time_t t_now;
	time(&t_now);

	/* Windows doesn't have localtime_r(), but it does have an equivalent
	 * function with the arguments in the opposite order! */
	if (0 != (t_is_local
	          ? localtime_s(&r_timeinfo, &t_now)
	          : gmtime_s(&r_timeinfo, &t_now)))
	{
		return false;
	}

	return true;
}

#elif defined(__MAC__) || defined(__IOS__)
static bool
MCDateGetTimeInfo(bool t_is_local,
                  struct tm & r_timeinfo,
                  long & r_timezone)
{
	time_t t_now;
	time(&t_now);

	if (NULL == (t_is_local
	             ? localtime_r(&t_now, &r_timeinfo)
	             : gmtime_r(&t_now, &r_timeinfo)))
	{
		return false;
	}

	r_timezone = r_timeinfo.tm_gmtoff;

	return true;
}

#elif defined(__LINUX__) || defined(__ANDROID__) || defined(__EMSCRIPTEN__)
static bool
MCDateGetTimeInfo(bool t_is_local,
                  struct tm & r_timeinfo,
                  long & r_timezone)
{
	time_t t_now;
	time (&t_now);

	if (NULL == (t_is_local
	             ? localtime_r(&t_now, &r_timeinfo)
	             : gmtime_r(&t_now, &r_timeinfo)))
	{
		return false;
	}

	if (t_is_local)
	{
		/* FIXME This may be expensive, but is probably required if
		 * MCDateGetTimeInfo() is to behave properly over summer time
		 * changes. */
		tzset();
		r_timezone = timezone;
	}
	else
	{
		r_timezone = 0;
	}

	return true;
}

#else
#	error "MCDateGetTimeInfo() not implemented for this platform"
#endif

static void
MCDateExecGetDate(bool t_is_local,
                  MCProperListRef & r_datetime)
{
	struct tm t_timeinfo;
	long t_timezone;

	if (!MCDateGetTimeInfo(t_is_local, t_timeinfo, t_timezone))
	{
		return;
	}

    // tm_mon is number of months since January
    t_timeinfo . tm_mon++;
    
    // tm_year is number of years since 1900
    t_timeinfo . tm_year += 1900;
    
	MCAutoNumberRef t_year, t_month, t_day, t_hour, t_minute, t_second,
		t_gmtoff;
	if (!(MCNumberCreateWithInteger (t_timeinfo.tm_year, &t_year) &&
	      MCNumberCreateWithInteger (t_timeinfo.tm_mon, &t_month) &&
	      MCNumberCreateWithInteger (t_timeinfo.tm_mday, &t_day) &&
	      MCNumberCreateWithInteger (t_timeinfo.tm_hour, &t_hour) &&
	      MCNumberCreateWithInteger (t_timeinfo.tm_min, &t_minute) &&
	      MCNumberCreateWithInteger (t_timeinfo.tm_sec, &t_second) &&
	      MCNumberCreateWithInteger (t_timezone, &t_gmtoff)))
		return;

	const MCValueRef t_elements[] = {*t_year, *t_month, *t_day,
	                                 *t_hour, *t_minute, *t_second,
	                                 *t_gmtoff};

	if (!MCProperListCreate (t_elements, 7, r_datetime))
        return;
}

extern "C" MC_DLLEXPORT_DEF void
MCDateExecGetLocalDate (MCProperListRef & r_datetime)
{
	MCDateExecGetDate(true, r_datetime);
}

extern "C" MC_DLLEXPORT_DEF void
MCDateExecGetUniversalDate(MCProperListRef & r_datetime)
{
	MCDateExecGetDate(false, r_datetime);
}

extern "C" MC_DLLEXPORT_DEF void
MCDateExecGetUniversalTime (double& r_time)
{
#ifndef _WINDOWS
    struct timeval tv;
    
    gettimeofday(&tv, NULL);
    r_time = tv.tv_sec + (double)tv.tv_usec / 1000000.0;
#else
	SYSTEMTIME t_localtime;
	FILETIME t_filetime;
	GetLocalTime(&t_localtime);
	SystemTimeToFileTime(&t_localtime, &t_filetime);

	// The Win32 filetime counts 100ns intervals since 1st Jan 1601
	uint64_t t_time_win32;
	double t_time_unix;
	t_time_win32 = (uint64_t(t_filetime.dwHighDateTime) << 32) | t_filetime.dwLowDateTime;
	t_time_unix = (double(t_time_win32) / 10000000.0) - 11644473600.0;

	r_time = t_time_unix;
#endif
}

////////////////////////////////////////////////////////////////////////////////////////////////////

extern "C" bool com_livecode_date_Initialize(void)
{
    return true;
}

extern "C" void com_livecode_date_Finalize(void)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
