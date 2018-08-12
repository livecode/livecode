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

#include "foundation-locale.h"
#include "foundation-unicode-private.h"

#include "unicode/brkiter.h"
#include "unicode/calendar.h"
#include "unicode/datefmt.h"
#include "unicode/gregocal.h"
#include "unicode/locid.h"
#include "unicode/numfmt.h"
#include "unicode/rbnf.h"
#include "unicode/timezone.h"
#include "unicode/uclean.h"

#include "foundation.h"
#include "foundation-auto.h"


////////////////////////////////////////////////////////////////////////////////

struct __MCTimeZone
{
    // The associated ICU timezone object
    icu::TimeZone& m_icu_timezone;
    
    // Cached unique time zone name
    MCStringRef m_name;
    
public:
    
    __MCTimeZone(icu::TimeZone& p_icu_timezone)
    : m_icu_timezone(p_icu_timezone)
    {
        m_name = MCValueRetain(kMCEmptyString);
    }
    
    ~__MCTimeZone()
    {
        delete &m_icu_timezone;
        MCValueRelease(m_name);
    }
};


struct __MCBreakIterator
{
    // The ICU break iterator object
    icu::BreakIterator& m_icu_iter;
    
    // String currently being used by the break iterator
    icu::UnicodeString* m_icu_string;
    
public:
    
    __MCBreakIterator(icu::BreakIterator& p_icu_iter)
    : m_icu_iter(p_icu_iter), m_icu_string(nil)
    {
        ;
    }
    
    ~__MCBreakIterator()
    {
        // No longer deletes the ICU:BreakIterator, but needs to cleanup the
        // ICU:UnicodeString stored
//        delete &m_icu_iter;
        delete m_icu_string;
    }
};


struct __MCLocale
{
    // Reference count
    uindex_t m_refcount;
    
    // The associated ICU locale object
    icu::Locale& m_icu_locale;
    
    // Cached values of the locale's properties
    MCStringRef m_name;
    MCStringRef m_language;
    MCStringRef m_country;
    MCStringRef m_script;
    
    // Cached number formatter objects
    icu::NumberFormat *m_nf_default;
    icu::NumberFormat *m_nf_decimal;
    icu::NumberFormat *m_nf_currency;
    icu::NumberFormat *m_nf_percent;
    icu::NumberFormat *m_nf_scientific;
    icu::NumberFormat *m_nf_spellout;
    icu::NumberFormat *m_nf_ordinal;
    icu::NumberFormat *m_nf_duration;
    icu::NumberFormat *m_nf_numbering_system;
    icu::NumberFormat *m_nf_currency_iso;
    icu::NumberFormat *m_nf_currency_plural;
    
    // Time zone and calendar objects
    // TODO: ability to cache multiple if it turns out these change frequently
    __MCTimeZone        *m_tz;
    icu::Calendar       *m_cal;
    
    // TODO: maybe a smarter way of caching pattern-based formatters to allow
    // the use of more than one of each simultaneously?
    icu::NumberFormat   *m_nf_pattern;
    icu::DateFormat     *m_df_pattern;
    
    // Date formatter object (gets updated to use the specified date/time
    // formats whenever a format call is made)
    icu::DateFormat     *m_df;
    
    // Cached break iterators
    icu::BreakIterator  *m_character;
    icu::BreakIterator  *m_word;
    icu::BreakIterator  *m_line;
    icu::BreakIterator  *m_sentence;
    icu::BreakIterator  *m_title;

    
public:
    
    __MCLocale(icu::Locale& p_icu_locale)
    : m_refcount(1), m_icu_locale(p_icu_locale), m_nf_default(nil),
        m_nf_decimal(nil), m_nf_currency(nil), m_nf_percent(nil),
        m_nf_scientific(nil), m_nf_spellout(nil), m_nf_ordinal(nil),
        m_nf_duration(nil), m_nf_numbering_system(nil), m_nf_currency_iso(nil),
        m_nf_currency_plural(nil), m_tz(nil), m_cal(nil), m_nf_pattern(nil),
        m_df_pattern(nil), m_df(nil), m_character(nil), m_word(nil), m_line(nil),
        m_sentence(nil), m_title(nil)
    {
        m_name = MCValueRetain(kMCEmptyString);
        m_language = MCValueRetain(kMCEmptyString);
        m_country = MCValueRetain(kMCEmptyString);
        m_script = MCValueRetain(kMCEmptyString);
    }
    
    ~__MCLocale()
    {
        delete &m_icu_locale;
        MCValueRelease(m_name);
        MCValueRelease(m_language);
        MCValueRelease(m_country);
        MCValueRelease(m_script);
        
        /*delete m_nf_default;
        delete m_nf_decimal;
        delete m_nf_currency;
        delete m_nf_percent;
        delete m_nf_scientific;
        delete m_nf_spellout;
        delete m_nf_ordinal;
        delete m_nf_duration;
        delete m_nf_numbering_system;
        delete m_nf_currency_iso;
        delete m_nf_currency_plural;
        delete m_tz;
        delete m_cal;
        delete m_nf_pattern;
        delete m_df_pattern;
        delete m_df;*/
        delete m_character;
        delete m_word;
        delete m_sentence;
        delete m_line;
        delete m_title;
    }
};

////////////////////////////////////////////////////////////////////////////////

MCLocaleRef kMCLocaleBasic;
MCLocaleRef MCLdefault;
MCLocaleRef MCLcurrent;

MCTimeZoneRef kMCTimeZoneSystem;
MCTimeZoneRef kMCTimeZoneUTC;

////////////////////////////////////////////////////////////////////////////////

bool __MCLocaleInitialize()
{
    bool t_success;
    t_success = true;
    
    // Create the well-known locales
    // The default locale to be used needs to be determined using setlocale
    if (t_success)
        t_success = MCLocaleCreateWithName(MCSTR("en_US"), kMCLocaleBasic);
    
    // Create the well-known time zones
    //if (t_success)
    //    t_success = MCTimeZoneCreate(kMCEmptyString, kMCTimeZoneSystem);
    //if (t_success)
    //    t_success = MCTimeZoneCreate(MCSTR("UTC"), kMCTimeZoneUTC);
    
    return t_success;
}

void __MCLocaleFinalize()
{
    // Destroy the well-known time zones
    MCValueRelease(kMCTimeZoneUTC);
    MCValueRelease(kMCTimeZoneSystem);

    // Destroy the well-known locales
    MCLocaleRelease(kMCLocaleBasic);
}

////////////////////////////////////////////////////////////////////////////////

const icu::Locale& MCLocaleGetICULocale(MCLocaleRef p_locale)
{
    MCAssert(p_locale != nil);
    return p_locale->m_icu_locale;
}

bool MCStringCreateWithICUString(icu::UnicodeString& p_string, MCStringRef &r_string)
{
    return MCStringCreateWithChars(p_string.getBuffer(), p_string.length(), r_string);
}

bool MCStringConvertToICUString(MCStringRef p_string, icu::UnicodeString &r_string)
{
    r_string.setTo(MCStringGetCharPtr(p_string), MCStringGetLength(p_string));
    return true;
}

////////////////////////////////////////////////////////////////////////////////

bool MCLocaleCreateWithName(MCStringRef p_name, MCLocaleRef &r_locale)
{
    // This is a really nasty hack used to work around the fact that nativising
    // a string requires a locale in the first place...
    const char *t_name;
    if (MCStringIsEqualToCString(p_name, "en_US", kMCStringOptionCompareExact))
        t_name = "en_US";
    else
        t_name = (const char *)MCStringGetNativeCharPtr(p_name);
    
    // Create a new ICU locale
    icu::Locale *t_icu_locale;
    t_icu_locale = new icu::Locale(t_name, NULL, NULL, NULL);
    
    // Convert it into an engine locale
    __MCLocale *t_locale;
    t_locale = new (nothrow) __MCLocale(*t_icu_locale);
    
    // All done
    r_locale = t_locale;
    return true;
}

bool MCLocaleCreateWithLCID(uint32_t LCID, MCLocaleRef &r_locale)
{
    // Convert the LCID to an ICU locale ID. First get the size of the ICU ID
    UErrorCode t_error = U_ZERO_ERROR;
    uindex_t t_length = uloc_getLocaleForLCID(LCID, NULL, 0, &t_error);
    if (U_FAILURE(t_error) && t_error != U_BUFFER_OVERFLOW_ERROR)
        return false;
    
    // Then allocate some memory for the ID
    t_error = U_ZERO_ERROR;
    MCAutoArray<char> t_buffer;
    if (!t_buffer.New(t_length + 1))
        return false;
    
    // Get the locale ID and create a locale with it
    uloc_getLocaleForLCID(LCID, t_buffer.Ptr(), t_buffer.Size(), &t_error);
    if (U_FAILURE(t_error))
        return false;
    
    MCAutoStringRef t_name;
    if (!MCStringCreateWithNativeChars((const char_t*)t_buffer.Ptr(), t_buffer.Size(), &t_name))
        return false;
    
    return MCLocaleCreateWithName(*t_name, r_locale);
}

MCLocaleRef MCLocaleRetain(MCLocaleRef p_locale)
{
    MCAssert(p_locale != nil);
    p_locale->m_refcount++;
    return p_locale;
}

void MCLocaleRelease(MCLocaleRef p_locale)
{
    MCAssert(p_locale != nil);
    if (--p_locale->m_refcount == 0)
        delete p_locale;
}

////////////////////////////////////////////////////////////////////////////////

MCStringRef MCLocaleGetName(MCLocaleRef p_locale)
{
    MCAssert(p_locale != nil);
    
    // Get the data if it has not been cached
    if (MCStringIsEmpty(p_locale->m_name))
    {
        MCStringRef t_name;
        /* UNCHECKED */ MCStringCreateWithCString(p_locale->m_icu_locale.getName(), t_name);
        MCValueAssign(p_locale->m_name, t_name);
    }
    
    return p_locale->m_name;
}

MCStringRef MCLocaleGetLanguage(MCLocaleRef p_locale)
{
    MCAssert(p_locale != nil);
    
    // Get the data if it has not been cached
    if (MCStringIsEmpty(p_locale->m_language))
    {
        MCStringRef t_language;
        /* UNCHECKED */ MCStringCreateWithCString(p_locale->m_icu_locale.getLanguage(), t_language);
        MCValueAssign(p_locale->m_language, t_language);
    }
    
    return p_locale->m_language;
}

MCStringRef MCLocaleGetCountry(MCLocaleRef p_locale)
{
    MCAssert(p_locale != nil);
    
    // Get the data if it has not been cached
    if (MCStringIsEmpty(p_locale->m_country))
    {
        MCStringRef t_country;
        /* UNCHECKED */ MCStringCreateWithCString(p_locale->m_icu_locale.getCountry(), t_country);
        MCValueAssign(p_locale->m_country, t_country);
    }
    
    return p_locale->m_country;
}

MCStringRef MCLocaleGetScript(MCLocaleRef p_locale)
{
    MCAssert(p_locale != nil);
    
    // Get the data if it has not been cached
    if (MCStringIsEmpty(p_locale->m_script))
    {
        MCStringRef t_script;
        /* UNCHECKED */ MCStringCreateWithCString(p_locale->m_icu_locale.getScript(), t_script);
        MCValueAssign(p_locale->m_script, t_script);
    }
    
    return p_locale->m_script;
}

bool MCLocaleCopyNameLocalised(MCLocaleRef p_locale, MCLocaleRef p_in_language, MCStringRef &r_string)
{
    MCAssert(p_locale != nil);
    MCAssert(p_in_language != nil);
    
    // The display name is returned as a UnicodeString
    icu::UnicodeString t_name;
    p_locale->m_icu_locale.getDisplayName(p_in_language->m_icu_locale, t_name);
    
    // Extract the contents of the string
    return MCStringCreateWithICUString(t_name, r_string);
}

bool MCLocaleCopyLanguageLocalised(MCLocaleRef p_locale, MCLocaleRef p_in_language, MCStringRef &r_string)
{
    MCAssert(p_locale != nil);
    MCAssert(p_in_language != nil);
    
    // The display language is returned as a UnicodeString
    icu::UnicodeString t_language;
    p_locale->m_icu_locale.getDisplayLanguage(p_in_language->m_icu_locale, t_language);
    
    // Extract the contents of the string
    return MCStringCreateWithICUString(t_language, r_string);
}

bool MCLocaleCopyCountryLocalised(MCLocaleRef p_locale, MCLocaleRef p_in_language, MCStringRef &r_string)
{
    MCAssert(p_locale != nil);
    MCAssert(p_in_language != nil);
    
    // The display country name is returned as a UnicodeString
    icu::UnicodeString t_country;
    p_locale->m_icu_locale.getDisplayCountry(p_in_language->m_icu_locale, t_country);
    
    // Extract the contents of the string
    return MCStringCreateWithICUString(t_country, r_string);
}

bool MCLocaleCopyScriptLocalised(MCLocaleRef p_locale, MCLocaleRef p_in_language, MCStringRef &r_string)
{
    MCAssert(p_locale != nil);
    MCAssert(p_in_language != nil);
    
    // The display script name is returned as a UnicodeString
    icu::UnicodeString t_script;
    p_locale->m_icu_locale.getDisplayScript(p_in_language->m_icu_locale, t_script);
    
    // Extract the contents of the string
    return MCStringCreateWithICUString(t_script, r_string);
}

MCLocaleTextLayout MCLocaleGetLineDirection(MCLocaleRef p_locale)
{
    MCAssert(p_locale != nil);
    
    UErrorCode t_error = U_ZERO_ERROR;
    return MCLocaleTextLayout(uloc_getLineOrientation(p_locale->m_icu_locale.getName(), &t_error));
}

MCLocaleTextLayout MCLocaleGetCharacterDirection(MCLocaleRef p_locale)
{
    MCAssert(p_locale != nil);
    
    UErrorCode t_error = U_ZERO_ERROR;
    return MCLocaleTextLayout(uloc_getCharacterOrientation(p_locale->m_icu_locale.getName(), &t_error));
}

////////////////////////////////////////////////////////////////////////////////

// INTERNAL FUNCTION
// Gets the number formatter for the given options, creating it if required
/*
static icu::NumberFormat* MCNumberFormatterGet(MCLocaleRef p_locale, MCNumberFormatStyle p_style)
{
    MCAssert(p_locale != nil);
    
    // Which number formatter are we looking for?
    UNumberFormatStyle t_style;
    icu::NumberFormat **t_formatter;
    UErrorCode t_error = U_ZERO_ERROR;
    bool t_rule_based = false;
    switch (p_style)
    {
        case kMCNumberFormatStyleDecimal:
            t_formatter = &p_locale->m_nf_decimal;
            t_style = UNUM_DECIMAL;
            break;
            
        case kMCNumberFormatStyleCurrency:
            t_formatter = &p_locale->m_nf_currency;
            t_style = UNUM_CURRENCY;
            break;
            
        case kMCNumberFormatStylePercent:
            t_formatter = &p_locale->m_nf_percent;
            t_style = UNUM_PERCENT;
            break;
        
        case kMCNumberFormatStyleScientific:
            t_formatter = &p_locale->m_nf_scientific;
            t_style = UNUM_SCIENTIFIC;
            break;
            
        case kMCNumberFormatStyleSpellout:
            t_formatter = &p_locale->m_nf_spellout;
            t_style = UNUM_SPELLOUT;
            t_rule_based = true;
            break;
            
        case kMCNumberFormatStyleOrdinal:
            t_formatter = &p_locale->m_nf_ordinal;
            t_style = UNUM_ORDINAL;
            t_rule_based = true;
            break;
            
        case kMCNumberFormatStyleDuration:
            t_formatter = &p_locale->m_nf_duration;
            t_style = UNUM_DURATION;
            t_rule_based = true;
            break;
            
        case kMCNumberFormatStyleNumberingSystem:
            t_formatter = &p_locale->m_nf_numbering_system;
            t_style = UNUM_NUMBERING_SYSTEM;
            break;
            
        case kMCNumberFormatStyleCurrencyISO:
            t_formatter = &p_locale->m_nf_currency_iso;
            t_style = UNUM_CURRENCY_ISO;
            break;
            
        case kMCNumberFormatStyleCurrencyPlural:
            t_formatter = &p_locale->m_nf_currency_plural;
            t_style = UNUM_CURRENCY_PLURAL;
            break;
            
        case kMCNumberFormatStyleDefault:
            t_formatter = &p_locale->m_nf_default;
            t_style = UNUM_DEFAULT;
            break;
            
        default:
            // Others are not (yet) supported
            MCAssert(false);
    }
    
    // Has the requested number formatter been created yet?
    if (*t_formatter == nil)
    {
        // Create the requested formatter
        if (!t_rule_based)
            *t_formatter = icu::NumberFormat::createInstance(p_locale->m_icu_locale, t_style, t_error);
        else
            *t_formatter = icu::RuleBasedNumberFormat::createInstance(p_locale->m_icu_locale, t_style, t_error);
    }
    
    // If the creation failed, this will return a nil pointer
    return *t_formatter;
}

// INTERNAL FUNCTION
// Gets a number formatter using a pattern
static icu::NumberFormat* MCNumberFormatterGetWithPattern(MCLocaleRef p_locale, MCStringRef p_pattern)
{
    MCAssert(p_locale != nil);
    MCAssert(p_pattern != nil);
    
    // TODO: implement
    return nil;
}
*/
////////////////////////////////////////////////////////////////////////////////
/*
bool MCLocaleFormatInteger(MCLocaleRef p_locale, MCNumberFormatStyle p_style, int64_t p_int, MCStringRef &r_string)
{
    MCAssert(p_locale != nil);
    
    icu::NumberFormat *t_formatter;
    t_formatter = MCNumberFormatterGet(p_locale, p_style);
    if (t_formatter == nil)
        return false;
    
    icu::UnicodeString t_string;
    t_formatter->format(p_int, t_string);
    return !t_string.isEmpty() && MCStringCreateWithICUString(t_string, r_string);
}

bool MCLocaleFormatReal(MCLocaleRef p_locale, MCNumberFormatStyle p_style, real64_t p_real, MCStringRef &r_string)
{
    MCAssert(p_locale != nil);
    
    icu::NumberFormat *t_formatter;
    t_formatter = MCNumberFormatterGet(p_locale, p_style);
    if (t_formatter == nil)
        return false;
    
    icu::UnicodeString t_string;
    t_formatter->format(p_real, t_string);
    return !t_string.isEmpty() && MCStringCreateWithICUString(t_string, r_string);
}

bool MCLocaleFormatIntegerWithPattern(MCLocaleRef p_locale, MCStringRef p_pattern, int64_t p_int, MCStringRef &r_string)
{
    MCAssert(p_locale != nil);
    MCAssert(p_pattern != nil);
    
    icu::NumberFormat *t_formatter;
    t_formatter = MCNumberFormatterGetWithPattern(p_locale, p_pattern);
    if (t_formatter == nil)
        return false;
    
    icu::UnicodeString t_string;
    t_formatter->format(p_int, t_string);
    return !t_string.isEmpty() && MCStringCreateWithICUString(t_string, r_string);
}

bool MCLocaleFormatRealWithPattern(MCLocaleRef p_locale, MCStringRef p_pattern, real64_t p_real, MCStringRef &r_string)
{
    MCAssert(p_locale != nil);
    MCAssert(p_pattern != nil);
    
    icu::NumberFormat *t_formatter;
    t_formatter = MCNumberFormatterGetWithPattern(p_locale, p_pattern);
    if (t_formatter == nil)
        return false;
    
    icu::UnicodeString t_string;
    t_formatter->format(p_real, t_string);
    return !t_string.isEmpty() && MCStringCreateWithICUString(t_string, r_string);
}

bool MCLocaleParseInteger(MCLocaleRef p_locale, MCNumberFormatStyle p_style, MCStringRef p_string, int64_t &r_int)
{
    MCAssert(p_locale != nil);
    MCAssert(p_string != nil);
    
    icu::NumberFormat *t_formatter;
    t_formatter = MCNumberFormatterGet(p_locale, p_style);
    if (t_formatter == nil)
        return false;
    
    UErrorCode t_error = U_ZERO_ERROR;
    icu::UnicodeString t_string;
    if (!MCStringConvertToICUString(p_string, t_string))
        return false;
    
    icu::Formattable t_formattable;
    t_formatter->parse(t_string, t_formattable, t_error);
    if (U_FAILURE(t_error))
        return false;
    
    r_int = t_formattable.getInt64();
    return true;
}

bool MCLocaleParseReal(MCLocaleRef p_locale, MCNumberFormatStyle p_style, MCStringRef p_string, real64_t &r_real)
{
    MCAssert(p_locale != nil);
    MCAssert(p_string != nil);
    
    icu::NumberFormat *t_formatter;
    t_formatter = MCNumberFormatterGet(p_locale, p_style);
    if (t_formatter == nil)
        return false;
    
    UErrorCode t_error = U_ZERO_ERROR;
    icu::UnicodeString t_string;
    if (!MCStringConvertToICUString(p_string, t_string))
        return false;
    
    icu::Formattable t_formattable;
    t_formatter->parse(t_string, t_formattable, t_error);
    if (U_FAILURE(t_error))
        return false;
    
    r_real = t_formattable.getDouble();
    return true;
}

bool MCLocaleParseIntegerWithPattern(MCLocaleRef p_locale, MCStringRef p_pattern, MCStringRef p_string, int64_t &r_int)
{
    MCAssert(p_locale != nil);
    MCAssert(p_pattern != nil);
    MCAssert(p_string != nil);
    
    icu::NumberFormat *t_formatter;
    t_formatter = MCNumberFormatterGetWithPattern(p_locale, p_pattern);
    if (t_formatter == nil)
        return false;
    
    UErrorCode t_error = U_ZERO_ERROR;
    icu::UnicodeString t_string;
    if (!MCStringConvertToICUString(p_string, t_string))
        return false;
    
    icu::Formattable t_formattable;
    t_formatter->parse(t_string, t_formattable, t_error);
    if (U_FAILURE(t_error))
        return false;
    
    r_int = t_formattable.getInt64();
    return true;
}

bool MCLocaleParseIntegerWithDouble(MCLocaleRef p_locale, MCStringRef p_pattern, MCStringRef p_string, real64_t &r_real)
{
    MCAssert(p_locale != nil);
    MCAssert(p_pattern != nil);
    MCAssert(p_string != nil);
    
    icu::NumberFormat *t_formatter;
    t_formatter = MCNumberFormatterGetWithPattern(p_locale, p_pattern);
    if (t_formatter == nil)
        return false;
    
    UErrorCode t_error = U_ZERO_ERROR;
    icu::UnicodeString t_string;
    if (!MCStringConvertToICUString(p_string, t_string))
        return false;
    
    icu::Formattable t_formattable;
    t_formatter->parse(t_string, t_formattable, t_error);
    if (U_FAILURE(t_error))
        return false;
    
    r_real = t_formattable.getDouble();
    return true;
}
*/
////////////////////////////////////////////////////////////////////////////////
/*
bool MCTimeZoneCreate(MCStringRef p_name, MCTimeZoneRef &r_tz)
{
    MCAssert(p_name != nil);
    
    // Create the ICU time zone object
    icu::TimeZone *t_tz;
    if (MCStringIsEmpty(p_name))
        t_tz = icu::TimeZone::createDefault();
    else
    {
        icu::UnicodeString t_string;
        if (!MCStringConvertToICUString(p_name, t_string))
            return false;
    
        t_tz = icu::TimeZone::createTimeZone(t_string);
    }
    
    if (t_tz == nil)
        return false;
    
    // Create the LiveCode time zone object
    r_tz = new (nothrow) __MCTimeZone(*t_tz);
    return true;
}

void MCTimeZoneRelease(MCTimeZoneRef p_tz)
{
    MCAssert(p_tz != nil);
    delete p_tz;
}
*/
////////////////////////////////////////////////////////////////////////////////
/*
// INTERNAL FUNCTION
// Gets the appropriate calendar object for the given locale/time-zone pair
static icu::Calendar* MCCalendarGet(MCLocaleRef p_locale, MCTimeZoneRef p_tz)
{
    MCAssert(p_locale != nil);
    MCAssert(p_tz != nil);
    
    // Create a calendar object if it has not already been done
    UErrorCode t_error = U_ZERO_ERROR;
    if (p_locale->m_cal == nil)
    {
        p_locale->m_cal = icu::Calendar::createInstance(t_error);
        if (U_FAILURE(t_error))
            return nil;
    }
    
    // Set up the calendar object with the appropriate settings
    p_locale->m_cal->setTimeZone(p_tz->m_icu_timezone);
    
    // All done
    return p_locale->m_cal;
}
*/
////////////////////////////////////////////////////////////////////////////////
/*
bool MCTimeZoneHasDaylightSavings(MCTimeZoneRef p_tz)
{
    MCAssert(p_tz != nil);
    
    return p_tz->m_icu_timezone.useDaylightTime();
}

bool MCTimeZoneIsDateDST(MCTimeZoneRef p_tz, MCAbsoluteTime p_abs)
{
    MCAssert(p_tz != nil);
    
    // The date needs to be converted to an absolute time for the TimeZone API
    UErrorCode t_error = U_ZERO_ERROR;
    return p_tz->m_icu_timezone.inDaylightTime(p_abs, t_error);
}

int32_t MCTimeZoneGetRawOffset(MCTimeZoneRef p_tz)
{
    MCAssert(p_tz != nil);
    
    return p_tz->m_icu_timezone.getRawOffset();
}

int32_t MCTimeZoneGetOffsetAtTime(MCTimeZoneRef p_tz, MCAbsoluteTime p_abs)
{
    MCAssert(p_tz != nil);
    
    // Get the offset at the specified time
    UErrorCode t_error = U_ZERO_ERROR;
    int32_t t_dst, t_raw;
    p_tz->m_icu_timezone.getOffset(p_abs, false, t_raw, t_dst, t_error);
    return t_dst;
}
*/
//MCStringRef MCTimeZoneGetName(MCTimeZoneRef p_tz)
//{
//    MCAssert(p_tz != nil);
//
//    // Create the cached name if not already done
//    if (MCStringIsEmpty(p_tz->m_name))
//    {
//        MCAutoStringRef t_name;
//        icu::UnicodeString t_icu_name;
//        p_tz->m_icu_timezone.getID(t_icu_name);
//        /* UNCHECKED */ MCStringCreateWithICUString(t_icu_name, &t_name);
//        MCValueAssign(p_tz->m_name, *t_name);
//    }
//
//    return p_tz->m_name;
//}
/*
bool MCTimeZoneCopyNameLocalised(MCTimeZoneRef p_tz, MCLocaleRef p_locale, MCTimeZoneDisplayType p_style, bool p_in_dst, MCStringRef &r_name)
{
    MCAssert(p_tz != nil);
    MCAssert(p_locale != nil);
    
    // Query ICU for the localised name
    icu::UnicodeString t_name;
    p_tz->m_icu_timezone.getDisplayName(p_in_dst, icu::TimeZone::EDisplayType(p_style), p_locale->m_icu_locale, t_name);
    return MCStringCreateWithICUString(t_name, r_name);
}
*/
////////////////////////////////////////////////////////////////////////////////
/*
MCDay MCLocaleGetFirstDayOfWeek(MCLocaleRef p_locale)
{
    MCAssert(p_locale != nil);
    
    // Grab the calendar object. If possible, don't update the time zone
    icu::Calendar *t_cal = p_locale->m_cal;
    if (t_cal == nil)
        t_cal = MCCalendarGet(p_locale, kMCTimeZoneSystem);
    
    // Query for the first day of the week, remembering that ICU days start at 0
    return MCDay(t_cal->getFirstDayOfWeek() - 1);
}

bool MCLocaleCalendarIsGregorian(MCLocaleRef p_locale)
{
    // Check by seeing if a calendar instance is a Gregorian calendar object
    // Grab the calendar object. If possible, don't update the time zone
    icu::Calendar *t_cal = p_locale->m_cal;
    if (t_cal == nil)
        t_cal = MCCalendarGet(p_locale, kMCTimeZoneSystem);
    
    return t_cal->getDynamicClassID() == icu::GregorianCalendar::getStaticClassID();
}
*/
////////////////////////////////////////////////////////////////////////////////
/*
bool MCDateCreateWithAbsoluteTime(MCLocaleRef p_locale, MCTimeZoneRef p_tz, MCAbsoluteTime p_abs, MCShinyNewDate& r_date)
{
    MCAssert(p_locale != nil);
    MCAssert(p_tz != nil);
    
    // Get the calendar object appropriate for this conversion
    icu::Calendar *t_cal;
    t_cal = MCCalendarGet(p_locale, p_tz);
    if (t_cal == nil)
        return false;
    
    // Tell the calendar object the absolute time and ask it for the various
    // fields of the local time. All get filled in because we don't know which
    // of the fields are desired.
    UErrorCode t_error = U_ZERO_ERROR;
    t_cal->setTime(p_abs, t_error);
    if (U_FAILURE(t_error))
        return false;
    
    // Get the raw values. These will be corrected later, if required
    r_date.m_year = t_cal->get(UCAL_YEAR, t_error);
    r_date.m_era = t_cal->get(UCAL_ERA, t_error);
    r_date.m_month = t_cal->get(UCAL_MONTH, t_error);
    r_date.m_day = t_cal->get(UCAL_DAY_OF_MONTH, t_error);
    r_date.m_hour = t_cal->get(UCAL_HOUR_OF_DAY, t_error);
    r_date.m_minute = t_cal->get(UCAL_MINUTE, t_error);
    r_date.m_second = t_cal->get(UCAL_SECOND, t_error);
    r_date.m_milliseconds = t_cal->get(UCAL_MILLISECOND, t_error);
    r_date.m_day_of_year = t_cal->get(UCAL_DAY_OF_YEAR, t_error);
    r_date.m_week_of_year = t_cal->get(UCAL_WEEK_OF_YEAR, t_error);
    r_date.m_day_of_week = t_cal->get(UCAL_DAY_OF_WEEK, t_error);
    r_date.m_week_of_month = t_cal->get(UCAL_WEEK_OF_MONTH, t_error);
    r_date.m_day_of_week_in_month = t_cal->get(UCAL_DAY_OF_WEEK_IN_MONTH, t_error);
    
    // If any of the above failed, abort
    if (U_FAILURE(t_error))
        return false;
    
    // Correct the date values that were written. Some of the ICU fields are
    // zero-based while the LiveCode fields are more "natural" and 1-based
    r_date.m_month++;
    r_date.m_day_of_week++;
    
    // AD/BC handling for Gregorian calendars
    if (t_cal->getDynamicClassID() == icu::GregorianCalendar::getStaticClassID())
    {
        if (r_date.m_era == icu::GregorianCalendar::BC)
        {
            r_date.m_year = -r_date.m_year;
        }
    }
    
    // All done
    return true;
}

bool MCDateTimeConvertToAbsoluteTime(MCLocaleRef p_locale, MCTimeZoneRef p_tz, const MCShinyNewDate& p_date, MCAbsoluteTime &r_abs)
{
    MCAssert(p_locale != nil);
    MCAssert(p_tz != nil);
    
    // Get the calendar object appropriate for the conversion
    icu::Calendar *t_cal;
    t_cal = MCCalendarGet(p_locale, p_tz);
    if (t_cal == nil)
        return false;
    
    // AD/BC handing for Gregorian calendars
    if (t_cal->getDynamicClassID() == icu::GregorianCalendar::getStaticClassID()
        && p_date.m_year < 0)
    {
        if (p_date.m_year > 0)
        {
            t_cal->set(UCAL_ERA, icu::GregorianCalendar::AD);
            t_cal->set(UCAL_YEAR, p_date.m_year);
        }
        else if (p_date.m_year < 0)
        {
            t_cal->set(UCAL_ERA, icu::GregorianCalendar::BC);
            t_cal->set(UCAL_YEAR, -p_date.m_year);
        }
        else
        {
            // The year 0 is not present in either AD or BC
            return false;
        }

    }
    else
    {
        t_cal->set(UCAL_ERA, p_date.m_era);
        t_cal->set(UCAL_YEAR, p_date.m_year);
    }
    
    // Some of the fields are always used
    t_cal->set(UCAL_HOUR_OF_DAY, p_date.m_hour);
    t_cal->set(UCAL_MINUTE, p_date.m_minute);
    t_cal->set(UCAL_SECOND, p_date.m_second);
    t_cal->set(UCAL_MILLISECOND, p_date.m_milliseconds);
    
    // Which fields to set in the calendar object depends on which are valid
    // NOTE: month and day_of_week are 1-based in LiveCode and 0-based in ICU
    if (p_date.m_month != 0 && p_date.m_day != 0)
    {
        t_cal->set(UCAL_MONTH, p_date.m_month - 1);
        t_cal->set(UCAL_DAY_OF_MONTH, p_date.m_day);
    }
    else if (p_date.m_month != 0 && p_date.m_day_of_week != 0 && p_date.m_day_of_week_in_month != 0)
    {
        t_cal->set(UCAL_MONTH, p_date.m_month - 1);
        t_cal->set(UCAL_DAY_OF_WEEK, p_date.m_day_of_week - 1);
        t_cal->set(UCAL_DAY_OF_WEEK_IN_MONTH, p_date.m_day_of_week_in_month);
    }
    else if (p_date.m_month != 0 && p_date.m_week_of_month != 0 && p_date.m_day_of_week != 0)
    {
        t_cal->set(UCAL_MONTH, p_date.m_month - 1);
        t_cal->set(UCAL_WEEK_OF_MONTH, p_date.m_week_of_month);
        t_cal->set(UCAL_DAY_OF_WEEK, p_date.m_day_of_week - 1);
    }
    else if (p_date.m_week_of_year != 0 && p_date.m_day_of_week != 0)
    {
        t_cal->set(UCAL_WEEK_OF_YEAR, p_date.m_week_of_year);
        t_cal->set(UCAL_DAY_OF_WEEK, p_date.m_day_of_week - 1);
    }
    else if (p_date.m_day_of_year != 0)
    {
        t_cal->set(UCAL_DAY_OF_YEAR, p_date.m_day_of_year);
    }
    else
    {
        // No valid combination of parameters was found
        return false;
    }
    
    // All the fields have been set. Extract the absolute time
    UErrorCode t_error = U_ZERO_ERROR;
    r_abs = t_cal->getTime(t_error);
    return U_SUCCESS(t_error);
}
*/
////////////////////////////////////////////////////////////////////////////////

bool MCLocaleBreakIteratorCreate(MCLocaleRef p_locale, MCBreakIteratorType p_type, MCBreakIteratorRef &r_iter)
{
    MCAssert(p_locale != nil);
    
    // Create the iterator with the requested type
    icu::BreakIterator *t_iter = nullptr;
    UErrorCode t_error = U_ZERO_ERROR;
    switch (p_type)
    {
        case kMCBreakIteratorTypeCharacter:
            if (p_locale->m_character == nil)
                p_locale->m_character = icu::BreakIterator::createCharacterInstance(p_locale->m_icu_locale, t_error);
            t_iter = p_locale->m_character;
            break;
            
        case kMCBreakIteratorTypeWord:
            if (p_locale->m_word == nil)
                p_locale->m_word = icu::BreakIterator::createWordInstance(p_locale->m_icu_locale, t_error);
            t_iter = p_locale->m_word;
            break;
            
        case kMCBreakIteratorTypeLine:
            if (p_locale->m_line == nil)
                p_locale->m_line = icu::BreakIterator::createLineInstance(p_locale->m_icu_locale, t_error);
            t_iter = p_locale->m_line;
            break;
            
        case kMCBreakIteratorTypeSentence:
            if (p_locale->m_sentence == nil)
                p_locale->m_sentence = icu::BreakIterator::createSentenceInstance(p_locale->m_icu_locale, t_error);
            t_iter = p_locale->m_sentence;
            break;
            
        case kMCBreakIteratorTypeTitle:
            if (p_locale->m_title == nil)
                p_locale->m_title = icu::BreakIterator::createTitleInstance(p_locale->m_icu_locale, t_error);
            t_iter = p_locale->m_title;
            break;
            
        default:
            // Shouldn't get here
            MCAssert(false);
    }
    
    if (U_FAILURE(t_error))
        return false;
    
    // Use the ICU break iterator object to create the engine object
    r_iter = new (nothrow) __MCBreakIterator(*t_iter);
    return true;
}

void MCLocaleBreakIteratorRelease(MCBreakIteratorRef p_iter)
{
    MCAssert(p_iter != nil);
    
    // No longer does anything to the break iterators, which are cached by the locale
    delete p_iter;
}

////////////////////////////////////////////////////////////////////////////////

bool MCLocaleBreakIteratorSetText(MCBreakIteratorRef p_iter, MCStringRef p_string)
{
    MCAssert(p_iter != nil);
    MCAssert(p_string != nil);
    
    icu::UnicodeString *t_string = new icu::UnicodeString;
    if (!MCStringConvertToICUString(p_string, *t_string))
        return false;

    delete p_iter->m_icu_string;
    p_iter->m_icu_string = t_string;

    p_iter->m_icu_iter.setText(*t_string);
    return true;
}

uindex_t MCLocaleBreakIteratorAdvance(MCBreakIteratorRef p_iter)
{
    MCAssert(p_iter != nil);
    
    // Advance the iterator and check to see whether it has finished
    int32_t t_result;
    t_result = p_iter->m_icu_iter.next();
    return (t_result == icu::BreakIterator::DONE) ? kMCLocaleBreakIteratorDone : t_result;
}

uindex_t MCLocaleBreakIteratorNext(MCBreakIteratorRef p_iter, uindex_t p_count)
{
    MCAssert(p_iter != nil);
    
    int32_t t_result;
    t_result = p_iter->m_icu_iter.next(p_count);
    return (t_result == icu::BreakIterator::DONE) ? kMCLocaleBreakIteratorDone : t_result;
}

bool MCLocaleBreakIteratorIsBoundary(MCBreakIteratorRef p_iter, uindex_t p_index)
{
    MCAssert(p_iter != nil);

    bool t_result;
    t_result = !!p_iter->m_icu_iter.isBoundary(p_index);
    
    return t_result;
}

uindex_t MCLocaleBreakIteratorBefore(MCBreakIteratorRef p_iter, uindex_t p_index)
{
    MCAssert(p_iter != nil);
    
    int32_t t_result;
    t_result = p_iter->m_icu_iter.preceding(p_index);
    return (t_result == icu::BreakIterator::DONE) ? kMCLocaleBreakIteratorDone : t_result;
}

uindex_t MCLocaleBreakIteratorAfter(MCBreakIteratorRef p_iter, uindex_t p_index)
{
    MCAssert(p_iter != nil);
    
    int32_t t_result;
    t_result = p_iter->m_icu_iter.following(p_index);
    return (t_result == icu::BreakIterator::DONE) ? kMCLocaleBreakIteratorDone : t_result;
}

bool MCLocaleWordBreakIteratorAdvance(MCStringRef self, MCBreakIteratorRef p_iter, MCRange& x_range)
{
    uindex_t t_start, t_left_break, t_right_break;
    t_right_break = x_range . offset + x_range . length;
    t_start = t_right_break;
    bool found = false;
    
    // Advance to the beginning of the specified range
    while (!found)
    {
        t_left_break = t_right_break;
        t_start = t_left_break;
        
        t_right_break = MCLocaleBreakIteratorAdvance(p_iter);
        if (t_right_break == kMCLocaleBreakIteratorDone)
            return false;
        
        // if the intervening chars contain a letter or number then it was a valid 'word'
        while (t_left_break < t_right_break)
        {
			codepoint_t t_cp =
				MCStringGetCodepointAtIndex(self, t_left_break);
			
			if (MCStringCodepointIsWordPart(t_cp))
				break;
			
			t_left_break += MCUnicodeCodepointGetCodeunitLength(t_cp);
        }
        
        if (t_left_break < t_right_break)
            found = true;
    }
    if (t_start == kMCLocaleBreakIteratorDone)
        return false;
    
    x_range .  offset = t_start;
    x_range . length = t_right_break - t_start;
    return true;
}
