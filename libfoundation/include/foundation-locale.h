/* Copyright (C) 2015 LiveCode Ltd.
 
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

#ifndef __MC_FOUNDATION_LOCALE__
#define __MC_FOUNDATION_LOCALE__

#include "foundation.h"


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////


// Locale text layouts
enum MCLocaleTextLayout
{
    kMCLocaleTextLayoutLTR = 0,     // Left to right
    kMCLocaleTextLayoutRTL = 1,     // Right to left
    kMCLocaleTextLayoutTTB = 2,     // Top to bottom
    kMCLocaleTextLayoutBTT = 3,     // Bottom to top
};

////////////////////////////////////////////////////////////////////////////////

// Opaque pointer type to a locale
typedef struct __MCLocale* MCLocaleRef;

// The basic (POSIX-compatible) locale
extern MCLocaleRef kMCBasicLocale;

// Gets a reference to the named locale or returns false
bool    MCLocaleCreateWithName(MCStringRef p_locale_name, MCLocaleRef &r_locale);

// Gets a reference to a locale using a Win32 LCID
bool    MCLocaleCreateWithLCID(uint32_t LCID, MCLocaleRef &r_locale);

// Increments the reference count for the given locale
MCLocaleRef MCLocaleRetain(MCLocaleRef p_locale);

// Releases a reference to a locale
void    MCLocaleRelease(MCLocaleRef p_locale);

////////////////////////////////////////////////////////////////////////////////

// Well-known locales. Do not change these directly!
extern MCLocaleRef kMCLocaleBasic;        // Compatible with older LiveCode versions

////////////////////////////////////////////////////////////////////////////////

// Changes the current locale to the given locale object and updates the rest of
// the engine global variables that depend on the locale (e.g number and date
// formatters).
//
// This releases the locale object held in MCLcurrent and takes ownership of the
// given locale object.
bool MCLocaleSetCurrentLocale(MCLocaleRef);

////////////////////////////////////////////////////////////////////////////////

// For the non-localised variants, the technical, non-display name is returned.
// The stringref belongs to the locale and should not be released.
// For example, the British English locale would return "en" for the language
// and "GB" for the country, corresponding to locale ID "en_GB".
//
// The localised variants return a name suitable for display to the user and
// are in the specified language. The stringref belongs to the caller and needs
// to be released after use. For example, the locale ID "fr_FR" formatted for
// display in "en_GB" would return language "French" and country "France".

// Returns the identifying name of the given locale
MCStringRef MCLocaleGetName(MCLocaleRef p_locale);
bool MCLocaleCopyNameLocalised(MCLocaleRef p_locale,
                              MCLocaleRef p_in_language,
                              MCStringRef &r_string);

// Returns the language code of the locale
MCStringRef MCLocaleGetLanguage(MCLocaleRef);
bool MCLocaleCopyLanguageLocalised(MCLocaleRef p_locale,
                                  MCLocaleRef p_in_language,
                                  MCStringRef &r_string);

// Returns the country code of the locale
MCStringRef MCLocaleGetCountry(MCLocaleRef);
bool MCLocaleCopyCountryLocalised(MCLocaleRef p_locale,
                                 MCLocaleRef p_in_language,
                                 MCStringRef &r_string);

// Returns the writing script used by the locale
MCStringRef MCLocaleGetScript(MCLocaleRef);
bool MCLocaleCopyScriptLocalised(MCLocaleRef p_locale,
                                 MCLocaleRef p_in_language,
                                 MCStringRef &r_string);

// Returns the line and character orientation of the locale
MCLocaleTextLayout MCLocaleGetLineDirection(MCLocaleRef);
MCLocaleTextLayout MCLocaleGetCharacterDirection(MCLocaleRef);


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////


// Number formatting styles
enum MCNumberFormatStyle
{
    kMCNumberFormatStyleDefault,
    
    kMCNumberFormatStyleDecimalPattern,
    kMCNumberFormatStyleDecimal,
    kMCNumberFormatStyleCurrency,
    kMCNumberFormatStylePercent,
    kMCNumberFormatStyleScientific,
    kMCNumberFormatStyleSpellout,
    kMCNumberFormatStyleOrdinal,
    kMCNumberFormatStyleDuration,
    kMCNumberFormatStyleNumberingSystem,
    kMCNumberFormatStylePatternRulebased,
    kMCNumberFormatStylePatternISO,
    kMCNumberFormatStyleCurrencyISO,
    kMCNumberFormatStyleCurrencyPlural
};

////////////////////////////////////////////////////////////////////////////////

// TODO: explain the pattern syntax for number formatting

// Converts from an integer or floating-point value to text
bool    MCLocaleFormatInteger(MCLocaleRef, MCNumberFormatStyle, int64_t, MCStringRef&);
bool    MCLocaleFormatReal(MCLocaleRef, MCNumberFormatStyle, real64_t, MCStringRef&);
bool    MCLocaleFormatIntegerWithPattern(MCLocaleRef, MCStringRef, int64_t, MCStringRef&);
bool    MCLocaleFormatRealWithPattern(MCLocaleRef, MCStringRef, real64_t, MCStringRef&);

// Converts from text to an integer or floating-point value
bool    MCLocaleParseInteger(MCLocaleRef, MCNumberFormatStyle, MCStringRef, int64_t&);
bool    MCLocaleParseReal(MCLocaleRef, MCNumberFormatStyle, MCStringRef, real64_t&);
bool    MCLocalePatternParseInteger(MCLocaleRef, MCStringRef, MCStringRef, int64_t&);
bool    MCLocalePatternParseReal(MCLocaleRef, MCStringRef, MCStringRef, real64_t&);


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////


// Chooses between a locale's default calendar and the Gregorian calendar
enum MCCalendarType
{
    kMCCalendarTypeDefault,
    kMCCalendarTypeGregorian
};

// Symbolic names for days of the week. LiveCode months are 1-based
enum MCDay
{
    kMCDaySunday = 1,
    kMCDayMonday,
    kMCDayTuesday,
    kMCDayWednesday,
    kMCDayThursday,
    kMCDayFriday,
    kMCDaySaturday
};

// Symbolic names for months of the year. LiveCode months are 1-based
enum MCMonth
{
    kMCMonthJanuary = 1,
    kMCMonthFebruary,
    kMCMonthMarch,
    kMCMonthApril,
    kMCMonthMay,
    kMCMonthJune,
    kMCMonthJuly,
    kMCMonthAugust,
    kMCMonthSeptember,
    kMCMonthOctober,
    kMCMonthNovember,
    kMCMonthDecember,
    kMCMonthUndecimber, // Thirteenth month; required for lunar calendars
};

// Time zone display name lengths
enum MCTimeZoneDisplayType
{
    kMCTimeZoneDisplayTypeShort = 1,
    kMCTimeZoneDisplayTypeLong,
    kMCTimeZoneDisplayTypeShortGeneric,
    kMCTimeZoneDisplayTimeLongGeneric,
    kMCTimeZoneDisplayTypeShortGMT,
    kMCTimeZoneDisplayTypeLongGMT,
    kMCTimeZoneDisplayTypeShortCommonlyUsed,
    kMCTimeZoneDisplayTypeGenericLocation
};

////////////////////////////////////////////////////////////////////////////////

// Time representation. Count of milliseconds since 1970-01-01 00:00:00 UTC
typedef double MCAbsoluteTime;

// Wall-clock time. Note that some fields are redundant
struct MCShinyNewDate
{
    // For Gregorian calendars, the era value is always 0 and BC dates are
    // indicated with negative year values. For non-Gregorian calendars, the
    // era value indicates the era to which the year value is relative.
    //
    // The day_of_week_in_month field can be negative, this indicates "last"
    // e.g -1 is last <day> of <month> and -2 is 2nd last <day> of <month>
    
    // When considering redundant fields, libfoundation uses the following
    // preference order (use of combinations outside of this are undefined):
    //  {month, day}
    //  {month, day_of_week, day_of_week_in_month}
    //  {month, week_of_month, day_of_week}
    //  {week_of_year, day_of_week}
    //  {day_of_year}
    //
    // All of these fields are 1-based so to remove a combination from
    // contention, simply set one of the fields in that combination to 0.
    
    // Preferred fields
    int32_t     m_year;         // Year of era
    int8_t      m_era;          // Equivalent of AD/BC for non-Gregorian calendars
    uint8_t     m_month;        // Month of year
    uint8_t     m_day;          // Day of month
    uint8_t     m_hour;         // Hour of day
    uint8_t     m_minute;       // Minute of hour
    uint8_t     m_second;       // Second of minute
    uint16_t    m_milliseconds; // Milliseconds of second
    
    // Redundant fields
    uint16_t    m_day_of_year;     // Number of days into the year
    uint8_t     m_week_of_year;    // Number of weeks into the year
    uint8_t     m_day_of_week;     // Number of days into the week
    uint8_t     m_week_of_month;   // Number of weeks into the month
    int8_t      m_day_of_week_in_month;    // The "2nd" part in "2nd Monday in April"
};

////////////////////////////////////////////////////////////////////////////////

// NOTE ON TIME ZONES
//
//  A time zone is identified using its TzData name, e.g.
//      America/Los_Angeles
//      Europe/London
//

// Opaque type representing a time zone
typedef struct __MCTimeZone* MCTimeZoneRef;

// Creates a time zone with the given identifier
bool    MCTimeZoneCreate(MCStringRef p_name, MCTimeZoneRef &r_tz);

// Destroys an existing time zone object
void    MCTimeZoneRelease(MCTimeZoneRef);

////////////////////////////////////////////////////////////////////////////////

extern MCTimeZoneRef    kMCTimeZoneSystem;  // The system time zone
extern MCTimeZoneRef    kMCTimeZoneUTC;     // The UTC time zone

////////////////////////////////////////////////////////////////////////////////

// Returns whether the given time zone implements daylight savings time
bool    MCTimeZoneHasDaylightSavings(MCTimeZoneRef);

// Returns whether the time zone was under DST at the given time
bool    MCTimeZoneIsDateDST(MCTimeZoneRef, MCAbsoluteTime);

// Returns the offset between the time zone outwith DST (in milliseconds)
int32_t  MCTimeZoneGetRawOffset(MCTimeZoneRef);

// Returns the offset from GMT at the given time (in milliseconds)
int32_t  MCTimeZoneGetOffsetAtTime(MCTimeZoneRef, const MCShinyNewDate&);

// Returns the name identifying the time zone (e.g. "Europe/London")
MCStringRef MCTimeZoneGetName(MCTimeZoneRef);

// Returns the name used to describe the time zone in the given locale
bool    MCTimeZoneCopyNameLocalised(MCTimeZoneRef, MCLocaleRef, MCTimeZoneDisplayType, bool p_in_dst, MCStringRef &r_name);

////////////////////////////////////////////////////////////////////////////////

// What is the first day of the week according to the given locale?
MCDay MCLocaleGetFirstDayOfWeel(MCLocaleRef);

// Is the calendar Gregorian or something else?
bool MCLocaleCalendarIsGregorian(MCLocaleRef);

////////////////////////////////////////////////////////////////////////////////

// These functions take both a time zone and locale as a time zone does not
// necessarily contain enough information for conversion (e.g. for locales that
// use non-Gregorian calendars)

// Converts from an absolute time to a local time
bool    MCDateCreateWithAbsoluteTime(MCLocaleRef, MCTimeZoneRef, MCAbsoluteTime, MCShinyNewDate&);

// Converts from a local time to an absolute time
bool    MCDateConvertToAbsolute(MCLocaleRef, MCTimeZoneRef, const MCShinyNewDate&, MCAbsoluteTime&);


////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////


// Date format styles
enum MCDateStyle
{
    kMCDateStyleDefault   = 0x00,
    kMCDateStyleFull      = 0x01,
    kMCDateStyleLong      = 0x02,
    kMCDateStyleMedium    = 0x03,
    kMCDateStyleShort     = 0x04,
    kMCDateStyleNone      = 0x05,
    
    // When set with one of the above options, the date is relative to the
    // current time (e.g. "yesterday", "today" and "tomorrow"). If a relative
    // date is not appropriate, the other date style will be used.
    kMCDateStyleRelative  = 0x80
};

////////////////////////////////////////////////////////////////////////////////

// Formats a date using a locale date/time style
bool    MCLocaleFormatDate(MCLocaleRef, MCDateStyle p_date, MCDateStyle p_time, MCTimeZoneRef, const MCShinyNewDate&, MCStringRef&);

// Formats a date using a specified pattern
bool    MCLocaleFormatDateWithPattern(MCLocaleRef, MCTimeZoneRef, MCStringRef, const MCShinyNewDate&, MCStringRef&);

// Parses a date using a locale date/time style
bool    MCLocaleParseDate(MCLocaleRef, MCDateStyle p_date, MCDateStyle p_time, MCStringRef, MCShinyNewDate&);

// Parses a date using a specified pattern
bool    MCLocaleParseDateWithPattern(MCLocaleRef, MCStringRef, MCStringRef, MCShinyNewDate&);

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////


// Break iterator types
enum MCBreakIteratorType
{
    kMCBreakIteratorTypeCharacter,  // Grapheme boundaries
    kMCBreakIteratorTypeWord,       // Word boundaries
    kMCBreakIteratorTypeLine,       // Line boundaries
    kMCBreakIteratorTypeSentence,   // Sentence boundaries
    kMCBreakIteratorTypeTitle       // Title casing boundaries
};

////////////////////////////////////////////////////////////////////////////////

// Opaque handle to a break iterator
typedef struct __MCBreakIterator* MCBreakIteratorRef;

// Creates a break iterator for the specified locale and type
bool    MCLocaleBreakIteratorCreate(MCLocaleRef, MCBreakIteratorType, MCBreakIteratorRef&);

// Destroys an existing break iterator
void    MCLocaleBreakIteratorRelease(MCBreakIteratorRef);

////////////////////////////////////////////////////////////////////////////////

// Value returned to indicate that there is no more breakable text
#define kMCLocaleBreakIteratorDone  ((uindex_t)-1)

// Sets the text associated with a break iterator
bool    MCLocaleBreakIteratorSetText(MCBreakIteratorRef, MCStringRef);

// Returns the index of the next break point or zero if not possible
uindex_t MCLocaleBreakIteratorAdvance(MCBreakIteratorRef);

// Advances by n boundaries
uindex_t MCLocaleBreakIteratorNext(MCBreakIteratorRef, uindex_t p_count);

// Returns whether the given index into the iterator's text is a boundary
bool    MCLocaleBreakIteratorIsBoundary(MCBreakIteratorRef, uindex_t);

// Finds the first boundary before the given code unit index
uindex_t MCLocaleBreakIteratorBefore(MCBreakIteratorRef, uindex_t);

// Finds the first boundary after the given code unit index
uindex_t MCLocaleBreakIteratorAfter(MCBreakIteratorRef, uindex_t);

// Custom advance method for word break iterator.
bool MCLocaleWordBreakIteratorAdvance(MCStringRef self, MCBreakIteratorRef p_iter, MCRange& x_range);

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////


#endif  /* ifndef __MC_FOUNDATION_LOCALE__ */
