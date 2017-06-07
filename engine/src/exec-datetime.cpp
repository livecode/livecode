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
#include "mcio.h"

#include "globals.h"
#include "osspec.h"
#include "date.h"

#include "exec.h"

////////////////////////////////////////////////////////////////////////////////

MC_EXEC_DEFINE_EVAL_METHOD(DateTime, Milliseconds, 1)
MC_EXEC_DEFINE_EVAL_METHOD(DateTime, Seconds, 1)
MC_EXEC_DEFINE_EVAL_METHOD(DateTime, Ticks, 1)
MC_EXEC_DEFINE_EVAL_METHOD(DateTime, Date, 1)
MC_EXEC_DEFINE_EVAL_METHOD(DateTime, DateOfFormat, 2)
MC_EXEC_DEFINE_EVAL_METHOD(DateTime, Time, 1)
MC_EXEC_DEFINE_EVAL_METHOD(DateTime, DateFormat, 1)
MC_EXEC_DEFINE_EVAL_METHOD(DateTime, MonthNames, 1)
MC_EXEC_DEFINE_EVAL_METHOD(DateTime, WeekDayNames, 1)
MC_EXEC_DEFINE_EVAL_METHOD(DateTime, IsADate, 2)
MC_EXEC_DEFINE_EVAL_METHOD(DateTime, IsNotADate, 2)
MC_EXEC_DEFINE_EXEC_METHOD(DateTime, Convert, 6)
MC_EXEC_DEFINE_EXEC_METHOD(DateTime, ConvertIntoIt, 6)
MC_EXEC_DEFINE_GET_METHOD(DateTime, GetTwelveTime, 1)
MC_EXEC_DEFINE_SET_METHOD(DateTime, SetTwelveTime, 1)
MC_EXEC_DEFINE_GET_METHOD(DateTime, GetDate, 2)
MC_EXEC_DEFINE_GET_METHOD(DateTime, GetTime, 2)
MC_EXEC_DEFINE_GET_METHOD(DateTime, GetMilliseconds, 1)
MC_EXEC_DEFINE_GET_METHOD(DateTime, GetLongMilliseconds, 1)
MC_EXEC_DEFINE_GET_METHOD(DateTime, GetSeconds, 1)
MC_EXEC_DEFINE_GET_METHOD(DateTime, GetLongSeconds, 1)
MC_EXEC_DEFINE_GET_METHOD(DateTime, GetTicks, 1)
MC_EXEC_DEFINE_GET_METHOD(DateTime, GetLongTicks, 1)
MC_EXEC_DEFINE_GET_METHOD(DateTime, GetMonthNames, 2)
MC_EXEC_DEFINE_GET_METHOD(DateTime, GetWeekDayNames, 2)
MC_EXEC_DEFINE_GET_METHOD(DateTime, GetDateFormat, 2)

////////////////////////////////////////////////////////////////////////////////

void MCDateTimeEvalMilliseconds(MCExecContext& ctxt, real64_t& r_real)
{
	MCDateTimeGetMilliseconds(ctxt, r_real);
}

void MCDateTimeEvalSeconds(MCExecContext& ctxt, real64_t& r_seconds)
{
	MCDateTimeGetSeconds(ctxt, r_seconds);
}

void MCDateTimeEvalTicks(MCExecContext& ctxt, real64_t& r_ticks)
{
	MCDateTimeGetTicks(ctxt, r_ticks);
}

////////////////////////////////////////////////////////////////////////////////

void MCDateTimeEvalDateOfFormat(MCExecContext& ctxt, MCStringRef p_format, MCStringRef& r_string)
{
    MCDateTimeGetDateOfFormat(ctxt, p_format, r_string);
}

void MCDateTimeEvalDate(MCExecContext& ctxt, MCStringRef& r_string)
{
	MCDateTimeGetDate(ctxt, P_UNDEFINED, r_string);
}

void MCDateTimeEvalTime(MCExecContext& ctxt, MCStringRef& r_string)
{
	MCDateTimeGetTime(ctxt, P_UNDEFINED, r_string);
}

////////////////////////////////////////////////////////////////////////////////

void MCDateTimeEvalDateFormat(MCExecContext& ctxt, MCStringRef& r_string)
{
	MCDateTimeGetDateFormat(ctxt, P_UNDEFINED, r_string);
}

void MCDateTimeEvalMonthNames(MCExecContext& ctxt, MCStringRef& r_string)
{
	MCDateTimeGetMonthNames(ctxt, P_UNDEFINED, r_string);
}

void MCDateTimeEvalWeekDayNames(MCExecContext& ctxt, MCStringRef& r_string)
{
	MCDateTimeGetWeekDayNames(ctxt, P_UNDEFINED, r_string);
}

////////////////////////////////////////////////////////////////////////////////

void MCDateTimeEvalIsADate(MCExecContext& ctxt, MCValueRef p_value, bool& r_result)
{
	MCAutoStringRef t_string, t_converted;
	r_result = ctxt.ConvertToString(p_value, &t_string) &&
               MCD_convert(ctxt, *t_string, CF_UNDEFINED, CF_UNDEFINED, NULL,
					   CF_SECONDS, CF_UNDEFINED, NULL, &t_converted);
}

void MCDateTimeEvalIsNotADate(MCExecContext& ctxt, MCValueRef p_value, bool& r_result)
{
	MCDateTimeEvalIsADate(ctxt, p_value, r_result);
	r_result = !r_result;
}

////////////////////////////////////////////////////////////////////////////////

void MCDateTimeExecConvert(MCExecContext &ctxt, MCStringRef p_input, int p_from_first, int p_from_second, MCStringRef p_from_format, int p_to_first, int p_to_second, MCStringRef p_to_format, MCStringRef &r_output)
{
	if (!MCD_convert(ctxt, p_input, (Convert_form)p_from_first, (Convert_form)p_from_second, p_from_format, (Convert_form)p_to_first, (Convert_form)p_to_second, p_to_format, r_output))
	{
		MCStringCopy(p_input, r_output);
		ctxt .SetTheResultToStaticCString("invalid date");
	}
    // PM-2014-12-01: [[ Bug 14123 ]] Make sure the result is empty if conversion is successful
    else
        ctxt . SetTheResultToEmpty();
}
void MCDateTimeExecConvertIntoIt(MCExecContext &ctxt, MCStringRef p_input, int p_from_first, int p_from_second, MCStringRef p_from_format, int p_to_first, int p_to_second, MCStringRef p_to_format)
{
	MCAutoStringRef t_output;
	MCDateTimeExecConvert(ctxt, p_input, p_from_first, p_from_second, p_from_format, p_to_first, p_to_second, p_to_format, &t_output);
	ctxt . SetItToValue(*t_output);
}

////////////////////////////////////////////////////////////////////////////////

void MCDateTimeGetTwelveTime(MCExecContext &ctxt, bool& r_value)
{
	r_value = MCtwelvetime == True;
}

void MCDateTimeSetTwelveTime(MCExecContext &ctxt, bool p_value)
{
	MCtwelvetime = p_value ? True : False;
}

////////////////////////////////////////////////////////////////////////////////

void MCDateTimeGetDate(MCExecContext &ctxt, Properties p_type, MCStringRef& r_value)
{
	MCD_date(ctxt, p_type, r_value);
}

void MCDateTimeGetDateOfFormat(MCExecContext &ctxt, MCStringRef p_format, MCStringRef& r_value)
{
    MCD_date_of_format(ctxt, p_format, r_value);
}

void MCDateTimeGetTime(MCExecContext &ctxt, Properties p_type, MCStringRef& r_value)
{
	MCD_time(ctxt, p_type, r_value);
}

void MCDateTimeGetMilliseconds(MCExecContext &ctxt, double& r_value)
{
	r_value = floor(MCS_time() * 1000.0);
}

void MCDateTimeGetLongMilliseconds(MCExecContext &ctxt, double& r_value)
{
	r_value = MCS_time() * 1000.0;
}

void MCDateTimeGetSeconds(MCExecContext &ctxt, double& r_value)
{
	r_value = floor(MCS_time());
}

void MCDateTimeGetLongSeconds(MCExecContext &ctxt, double& r_value)
{
	r_value = MCS_time();
}

void MCDateTimeGetTicks(MCExecContext &ctxt, double& r_value)
{
	r_value = floor(MCS_time() * 60.0);
}

void MCDateTimeGetLongTicks(MCExecContext &ctxt, double& r_value)
{
	r_value = MCS_time() * 60.0;
}

void MCDateTimeGetMonthNames(MCExecContext &ctxt, Properties p_type, MCStringRef& r_value)
{
	MCAutoListRef t_list;
	if (MCD_monthnames(ctxt, p_type, &t_list) && MCListCopyAsString(*t_list, r_value))
		return;

	ctxt.Throw();
}

void MCDateTimeGetWeekDayNames(MCExecContext &ctxt, Properties p_type, MCStringRef& r_value)
{
	MCAutoListRef t_list;
	if (MCD_weekdaynames(ctxt, p_type, &t_list) && MCListCopyAsString(*t_list, r_value))
		return;

	ctxt.Throw();
}

void MCDateTimeGetDateFormat(MCExecContext &ctxt, Properties p_type, MCStringRef& r_value)
{
	MCD_dateformat(ctxt, p_type, r_value);
}
