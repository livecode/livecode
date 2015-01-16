/*                                                                     -*-c++-*-
Copyright (C) 2015 Runtime Revolution Ltd.

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

/* Windows doesn't have localtime_r(), but it does have an equivalent
 * function with the arguments in the opposite order! */
#if defined(__WINDOWS__)
#  define localtime_r(s,t) (_localtime_s(t,s))
#endif

extern "C" MC_DLLEXPORT void
MCDateExecGetLocalTime (MCProperListRef & r_datetime)
{
	struct tm t_timeinfo;
	time_t t_now;

	time (&t_now);
	if (NULL == localtime_r (&t_now, &t_timeinfo))
		return;

	MCAutoNumberRef t_year, t_month, t_day, t_hour, t_minute, t_second;
	if (!(MCNumberCreateWithInteger (t_timeinfo.tm_year, &t_year) &&
	      MCNumberCreateWithInteger (t_timeinfo.tm_mon, &t_month) &&
	      MCNumberCreateWithInteger (t_timeinfo.tm_mday, &t_day) &&
	      MCNumberCreateWithInteger (t_timeinfo.tm_hour, &t_hour) &&
	      MCNumberCreateWithInteger (t_timeinfo.tm_min, &t_minute) &&
	      MCNumberCreateWithInteger (t_timeinfo.tm_sec, &t_second)))
		return;

	const MCValueRef t_elements[] = {*t_year, *t_month, *t_day,
	                                 *t_hour, *t_minute, *t_second};

	/* UNCHECKED */ MCProperListCreate (t_elements, 6, r_datetime);
}
