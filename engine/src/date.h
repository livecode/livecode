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

#ifndef __DATETIME__
#define __DATETIME__

struct MCDateTime
{
	int4 year;
	int4 month;
	int4 day;
	int4 hour;
	int4 minute;
	int4 second;
	int4 bias;
};

#define DATETIME_ITEM_YEAR (1 << 0)
#define DATETIME_ITEM_MONTH (1 << 1)
#define DATETIME_ITEM_DAY (1 << 2)
#define DATETIME_ITEM_HOUR (1 << 3)
#define DATETIME_ITEM_MINUTE (1 << 4)
#define DATETIME_ITEM_SECOND (1 << 5)

#define DATETIME_ITEM_DATE (DATETIME_ITEM_YEAR | DATETIME_ITEM_MONTH | DATETIME_ITEM_DAY)
#define DATETIME_ITEM_TIME (DATETIME_ITEM_HOUR | DATETIME_ITEM_MINUTE | DATETIME_ITEM_SECOND)

// We need to support the following rules:
//   1) Day of month - (month, day)
//   2) Day of week in month:
//        (month, day of week, day index)
//      e.g. last Sunday in the month (month, SUNDAY, -1)
//           second Monday in the month (month, MONDAY, 2)
//   3) Day of week after day:
//        (month, day of week, day)
//      e.g. first Sunday on/after the 8th (month, SUNDAY, 8)
//   4) Day of week before day:
//        (month, day of week, day)
//      e.g. last Sunday on/before the 25th (month, SUNDAY, 25)
//
// We also need the time at which the switch happens - this can
// be WALL_TIME, STANDARD_TIME or UTC_TIME.
//
// Month is in range (0-12)
// Day of week is in range (0-7)
// Day index is in range (-5..5)
// Day is in range (0-31)
//
// Therefore we could (in theory) store an entire rule in 16-bits:
//   2 bits for rule type
//   4 bits for month
//   3 bits for day of week
//   5 bits for day or day index
//   2 bits for time mode
//   16 bits for time offset
// (although this limits the precision of the time offset)
//
// This is useful to note for encoding and storage of time zone
// DST rules - but when in a locale structure we want speed
// so we will store them unpacked - which needs 8 bytes per
// rule, plus 8 bytes to store the year range.

#define DATETIME_ZONE_TIME_WALL 0
#define DATETIME_ZONE_TIME_STANDARD 1
#define DATETIME_ZONE_TIME_UTC 2

#define DATETIME_ZONE_RULE_DAY_OF_MONTH (0 << 4)
#define DATETIME_ZONE_RULE_DAY_OF_WEEK (1 << 4)
#define DATETIME_ZONE_RULE_DAY_OF_WEEK_AFTER (2 << 4)
#define DATETIME_ZONE_RULE_DAY_OF_WEEK_BEFORE (3 << 4)

struct MCDateTimeZoneDSTRuleEntry
{
	int1 flags;
	int1 month;
	int1 day;
	int1 weekday;
	int4 time;
};

struct MCDateTimeZoneDSTRule
{
	int4 start_year;
	int4 finish_year;
	int4 bias;
	MCDateTimeZoneDSTRuleEntry start;
	MCDateTimeZoneDSTRuleEntry finish;
};

struct MCDateTimeZone
{
	int4 bias;
	uint4 dst_rule_count;
	MCDateTimeZoneDSTRule *dst_rules;
};

struct MCDateTimeLocale
{
	MCStringRef weekday_names[7];
	MCStringRef abbrev_weekday_names[7];
	MCStringRef month_names[12];
	MCStringRef abbrev_month_names[12];
	MCStringRef date_formats[3];
	MCStringRef time_formats[2];
	MCStringRef time24_formats[2];
	MCStringRef time_morning_suffix;
	MCStringRef time_evening_suffix;
	
	MCDateTimeLocale()
	{
		for (unsigned i = 0; i < 7; i++)
			weekday_names[i] = MCValueRetain(kMCEmptyString);
		for (unsigned i = 0; i < 7; i++)
			abbrev_weekday_names[i] = MCValueRetain(kMCEmptyString);
		for (unsigned i = 0; i < 12; i++)
			month_names[i] = MCValueRetain(kMCEmptyString);
		for (unsigned i = 0; i < 12; i++)
			abbrev_month_names[i] = MCValueRetain(kMCEmptyString);
		for (unsigned i = 0; i < 3; i++)
			date_formats[i] = MCValueRetain(kMCEmptyString);
		for (unsigned i = 0; i < 2; i++)
			time_formats[i] = MCValueRetain(kMCEmptyString);
		for (unsigned i = 0; i < 2; i++)
			 time24_formats[i] = MCValueRetain(kMCEmptyString);
		time_morning_suffix = MCValueRetain(kMCEmptyString);
		time_evening_suffix = MCValueRetain(kMCEmptyString);
	}
	
	MCDateTimeLocale& operator= (MCDateTimeLocale& other)
	{
		for (unsigned i = 0; i < 7; i++)
			weekday_names[i] = MCValueRetain(other.weekday_names[i]);
		for (unsigned i = 0; i < 7; i++)
			abbrev_weekday_names[i] = MCValueRetain(other.abbrev_weekday_names[i]);
		for (unsigned i = 0; i < 12; i++)
			month_names[i] = MCValueRetain(other.month_names[i]);
		for (unsigned i = 0; i < 12; i++)
			abbrev_month_names[i] = MCValueRetain(other.abbrev_month_names[i]);
		for (unsigned i = 0; i < 3; i++)
			date_formats[i] = MCValueRetain(other.date_formats[i]);
		for (unsigned i = 0; i < 2; i++)
			time_formats[i] = MCValueRetain(other.time_formats[i]);
		for (unsigned i = 0; i < 2; i++)
			time24_formats[i] = MCValueRetain(other.time24_formats[i]);
		time_morning_suffix = MCValueRetain(other.time_morning_suffix);
		time_evening_suffix = MCValueRetain(other.time_evening_suffix);
			
		return *this;
	}
	
	~MCDateTimeLocale()
	{
		for (unsigned i = 0; i < 7; i++)
			MCValueRelease(weekday_names[i]);
		for (unsigned i = 0; i < 7; i++)
			MCValueRelease(abbrev_weekday_names[i]);
		for (unsigned i = 0; i < 12; i++)
			MCValueRelease(month_names[i]);
		for (unsigned i = 0; i < 12; i++)
			MCValueRelease(abbrev_month_names[i]);
		for (unsigned i = 0; i < 3; i++)
			MCValueRelease(date_formats[i]);
		for (unsigned i = 0; i < 2; i++)
			MCValueRelease(time_formats[i]);
		for (unsigned i = 0; i < 2; i++)
			 MCValueRelease(time24_formats[i]);
		MCValueRelease(time_morning_suffix);
		MCValueRelease(time_evening_suffix);
	}
};

typedef uint4 MCDateTimeFormat;
enum
{
	DATETIME_FORMAT_NONE,

	DATETIME_FORMAT_ITEMS,
	DATETIME_FORMAT_MIME,

	DATETIME_FORMAT_SHORT_ENGLISH_DATE,
	DATETIME_FORMAT_ABBREV_ENGLISH_DATE,
	DATETIME_FORMAT_LONG_ENGLISH_DATE,
	DATETIME_FORMAT_SHORT_ENGLISH_TIME,
	DATETIME_FORMAT_LONG_ENGLISH_TIME,
	DATETIME_FORMAT_SHORT_ENGLISH_TIME24,
	DATETIME_FORMAT_LONG_ENGLISH_TIME24,

	DATETIME_FORMAT_SHORT_SYSTEM_DATE,
	DATETIME_FORMAT_ABBREV_SYSTEM_DATE,
	DATETIME_FORMAT_LONG_SYSTEM_DATE,
	DATETIME_FORMAT_SHORT_SYSTEM_TIME,
	DATETIME_FORMAT_LONG_SYSTEM_TIME
};

extern bool MCDateTimeInitialize();
extern bool MCDateTimeFinalize();

extern void MCD_date(MCExecContext& ctxt, Properties p_format, MCStringRef& r_date);
extern void MCD_time(MCExecContext& ctxt, Properties p_format, MCStringRef& r_time);
extern bool MCD_monthnames(MCExecContext& ctxt, Properties p_format, MCListRef& r_list);
extern bool MCD_weekdaynames(MCExecContext& ctxt, Properties p_format, MCListRef& r_list);
extern void MCD_dateformat(MCExecContext& ctxt, Properties p_format, MCStringRef& r_dateformat);

extern bool MCD_convert(MCExecContext& ctxt, MCValueRef p_input,
						Convert_form p_primary_from, Convert_form p_secondary_from,
						Convert_form p_primary_to, Convert_form p_secondary_to,
						MCStringRef& r_converted);

extern bool MCD_convert_to_datetime(MCExecContext& ctxt, MCValueRef p_input, Convert_form p_primary_from, Convert_form p_secondary_from, MCDateTime &r_datetime);
extern bool MCD_convert_from_datetime(MCExecContext& ctxt, MCDateTime p_datetime, Convert_form p_primary_from, Convert_form p_secondary_from, MCValueRef &r_output);

extern void MCD_getlocaleformats();

#endif
