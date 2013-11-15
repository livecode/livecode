/* Copyright (C) 2013 Runtime Revolution Ltd.
 
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

#include "unicode/calendar.h"
#include "unicode/locid.h"
#include "unicode/numfmt.h"
#include "unicode/rbnf.h"
#include "unicode/timezone.h"

#include "foundation-auto.h"

////////////////////////////////////////////////////////////////////////////////

struct __MCLocale
{
    // The associated ICU locale object
    icu::Locale& m_icu_locale;
    
    // Cached values of the locale's properties
    MCStringRef m_name;
    MCStringRef m_language;
    MCStringRef m_country;
    MCStringRef m_script;
    
    // Reference count
    uindex_t m_refcount;
    
public:
    
    __MCLocale(icu::Locale& p_icu_locale)
    : m_icu_locale(p_icu_locale), m_refcount(1)
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
    }
};

struct __MCNumberFormatter
{
    // The associated ICU number format object
    icu::NumberFormat& m_icu_formatter;
    
public:
    
    __MCNumberFormatter(icu::NumberFormat& p_icu_formatter)
    : m_icu_formatter(p_icu_formatter)
    {
        ;
    }
    
    ~__MCNumberFormatter()
    {
        delete &m_icu_formatter;
    }
};

struct __MCCalendar
{
    // The associated ICU calendar object
    icu::Calendar& m_icu_calendar;
    
public:
    
    __MCCalendar(icu::Calendar& p_icu_calendar)
    : m_icu_calendar(p_icu_calendar)
    {
        ;
    }
    
    ~__MCCalendar()
    {
        delete &m_icu_calendar;
    }
};

////////////////////////////////////////////////////////////////////////////////

MCLocaleRef MCLbasic;
MCLocaleRef MCLdefault;
MCLocaleRef MCLcurrent;

MCNumberFormatterRef MCNFbasic;
MCNumberFormatterRef MCNFdecimal;
MCNumberFormatterRef MCNFcurrency;
MCNumberFormatterRef MCNFordinal;

MCStringRef MCTZdefault;
MCStringRef MCTZutc;

MCCalendarRef MCCdefault;
MCCalendarRef MCCutc;

////////////////////////////////////////////////////////////////////////////////

bool __MCLocaleInitialize()
{
    bool t_success;
    t_success = true;
    
    // DIRTY EVIL HACK FOR TESTING
    u_setDataDirectory("/Users/frasergordon/Workspace/livecode/prebuilt/data/icu");
    
    // Create the well-known locales
    if (t_success)
        t_success = MCLocaleCreateWithName(MCSTR("en_US"), MCLbasic);
    if (t_success)
        t_success = MCLocaleCreateDefault(MCLdefault);
    if (t_success)
        t_success = MCLocaleCreateDefault(MCLcurrent);
    
    // Create the well-known number formatters
    if (t_success)
        t_success = MCNumberFormatterCreate(MCLbasic, kMCNumberFormatStyleDecimal, MCNFbasic);

    // Do the rest of the processing associated with changing locale
    if (t_success)
        t_success = MCLocaleSetCurrentLocale(MCLcurrent);
    
    // Create the well-known time zones
    if (t_success)
    {
        // Get the length of the name of the system's time zone
        size_t t_length;
        UErrorCode t_error = U_ZERO_ERROR;
        t_length = ucal_getDefaultTimeZone(NULL, 0, &t_error);
        t_success = U_SUCCESS(t_error) || t_error == U_BUFFER_OVERFLOW_ERROR;
        t_error = U_ZERO_ERROR;
        
        // Create the time zone
        MCAutoArray<unichar_t> t_buffer;
        if (t_success)
            t_success = t_buffer.New(t_length + 1);
        
        if (t_success)
            ucal_getDefaultTimeZone(t_buffer.Ptr(), t_buffer.Size(), &t_error);
        
        if (t_success)
            t_success = U_SUCCESS(t_error);
        
        if (t_success)
            t_success = MCStringCreateWithChars(t_buffer.Ptr(), t_buffer.Size(), MCTZdefault);
        
        // The UTC time zone is much simpler
        MCTZutc = MCSTR("UTC");
    }
    
    return t_success;
}

void __MCLocaleFinalize()
{
    // Destroy the well-known time zones
    MCValueRelease(MCTZutc);
    MCValueRelease(MCTZdefault);
    
    // Destroy the well-known number formatters
    MCNumberFormatterRelease(MCNFbasic);
    MCNumberFormatterRelease(MCNFordinal);
    MCNumberFormatterRelease(MCNFcurrency);
    MCNumberFormatterRelease(MCNFdecimal);
    
    // Destroy the well-known locales
    MCLocaleRelease(MCLcurrent);
    MCLocaleRelease(MCLdefault);
    MCLocaleRelease(MCLbasic);
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

////////////////////////////////////////////////////////////////////////////////

bool MCLocaleCreateDefault(MCLocaleRef &r_locale)
{
    // Create a new ICU locale
    icu::Locale *t_icu_locale;
    t_icu_locale = new icu::Locale();
    
    // Convert it to an engine locale
    __MCLocale *t_locale;
    t_locale = new __MCLocale(*t_icu_locale);
    
    // All done
    r_locale = t_locale;
    return true;
}

bool MCLocaleCreateWithName(MCStringRef p_name, MCLocaleRef &r_locale)
{
    // Create a new ICU locale
    icu::Locale *t_icu_locale;
    t_icu_locale = new icu::Locale((const char*)MCStringGetNativeCharPtr(p_name), NULL, NULL, NULL);
    
    // Convert it into an engine locale
    __MCLocale *t_locale;
    t_locale = new __MCLocale(*t_icu_locale);
    
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

bool MCLocaleSetCurrentLocale(MCLocaleRef p_locale)
{
    MCAssert(p_locale != nil);
    
    bool t_success;
    t_success = true;
    
    // Make sure to retain the new locale before releasing the old in case they
    // are the same object and we destroy it from under ourselves...
    MCLocaleRetain(p_locale);
    MCLocaleRelease(MCLcurrent);
    MCLcurrent = p_locale;
    
    // Release the existing number formatter objects
    if (MCNFdecimal != nil)
        MCNumberFormatterRelease(MCNFdecimal);
    if (MCNFcurrency != nil)
        MCNumberFormatterRelease(MCNFcurrency);
    if (MCNFordinal != nil)
        MCNumberFormatterRelease(MCNFordinal);
    
    MCNFdecimal = MCNFcurrency = MCNFordinal = nil;
    
    // Create the new number formatter objects
    if (t_success)
        t_success = MCNumberFormatterCreate(p_locale, kMCNumberFormatStyleDecimal, MCNFdecimal);
    if (t_success)
        t_success = MCNumberFormatterCreate(p_locale, kMCNumberFormatStyleCurrency, MCNFcurrency);
    if (t_success)
        t_success = MCNumberFormatterCreate(p_locale, kMCNumberFormatStyleOrdinal, MCNFordinal);
    
    return t_success;
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

bool MCLocaleGetNameLocalised(MCLocaleRef p_locale, MCLocaleRef p_in_language, MCStringRef &r_string)
{
    MCAssert(p_locale != nil);
    MCAssert(p_in_language != nil);
    
    // The display name is returned as a UnicodeString
    icu::UnicodeString t_name;
    p_locale->m_icu_locale.getDisplayName(p_in_language->m_icu_locale, t_name);
    
    // Extract the contents of the string
    return MCStringCreateWithICUString(t_name, r_string);
}

bool MCLocaleGetLanguageLocalised(MCLocaleRef p_locale, MCLocaleRef p_in_language, MCStringRef &r_string)
{
    MCAssert(p_locale != nil);
    MCAssert(p_in_language != nil);
    
    // The display language is returned as a UnicodeString
    icu::UnicodeString t_language;
    p_locale->m_icu_locale.getDisplayLanguage(p_in_language->m_icu_locale, t_language);
    
    // Extract the contents of the string
    return MCStringCreateWithICUString(t_language, r_string);
}

bool MCLocaleGetCountryLocalised(MCLocaleRef p_locale, MCLocaleRef p_in_language, MCStringRef &r_string)
{
    MCAssert(p_locale != nil);
    MCAssert(p_in_language != nil);
    
    // The display country name is returned as a UnicodeString
    icu::UnicodeString t_country;
    p_locale->m_icu_locale.getDisplayCountry(p_in_language->m_icu_locale, t_country);
    
    // Extract the contents of the string
    return MCStringCreateWithICUString(t_country, r_string);
}

bool MCLocaleGetScriptLocalised(MCLocaleRef p_locale, MCLocaleRef p_in_language, MCStringRef &r_string)
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

bool MCNumberFormatterCreate(MCLocaleRef p_locale, MCNumberFormatStyle p_style, MCNumberFormatterRef &r_formatter)
{
    MCAssert(p_locale != nil);
    
    // Create the ICU number formatter with the given options
    UNumberFormatStyle t_style;
    icu::NumberFormat *t_formatter;
    UErrorCode t_error = U_ZERO_ERROR;
    bool t_rule_based = false;
    switch (p_style)
    {
        case kMCNumberFormatStyleDecimal:
            t_style = UNUM_DECIMAL;
            break;
            
        case kMCNumberFormatStyleCurrency:
            t_style = UNUM_CURRENCY;
            break;
            
        case kMCNumberFormatStylePercent:
            t_style = UNUM_PERCENT;
            break;
        
        case kMCNumberFormatStyleScientific:
            t_style = UNUM_SCIENTIFIC;
            break;
            
        case kMCNumberFormatStyleSpellout:
            t_style = UNUM_SPELLOUT;
            t_rule_based = true;
            break;
            
        case kMCNumberFormatStyleOrdinal:
            t_style = UNUM_ORDINAL;
            t_rule_based = true;
            break;
            
        case kMCNumberFormatStyleDuration:
            t_style = UNUM_DURATION;
            t_rule_based = true;
            break;
            
        case kMCNumberFormatStyleNumberingSystem:
            t_style = UNUM_NUMBERING_SYSTEM;
            break;
            
        case kMCNumberFormatStyleCurrencyISO:
            t_style = UNUM_CURRENCY_ISO;
            break;
            
        case kMCNumberFormatStyleCurrencyPlural:
            t_style = UNUM_CURRENCY_PLURAL;
            break;
            
        case kMCNumberFormatStyleDefault:
            t_style = UNUM_DEFAULT;
            break;
            
        default:
            // Others are not (yet) supported
            MCAssert(false);
    }
    
    if (!t_rule_based)
        t_formatter = icu::NumberFormat::createInstance(p_locale->m_icu_locale, t_style, t_error);
    else
        t_formatter = icu::RuleBasedNumberFormat::createInstance(p_locale->m_icu_locale, t_style, t_error);
    
    // DIRTY HACK FOR TESTING
    if (U_FAILURE(t_error))
        return t_rule_based;
    
    // Convert the ICU formatter into a LiveCode formatter
    r_formatter = new __MCNumberFormatter(*t_formatter);
    return true;
}

void MCNumberFormatterRelease(MCNumberFormatterRef p_formatter)
{
    MCAssert(p_formatter != nil);
    delete p_formatter;
}

////////////////////////////////////////////////////////////////////////////////

bool MCLocaleNumberFormatInteger(MCNumberFormatterRef p_formatter, int64_t t_num, MCStringRef &r_string)
{
    MCAssert(p_formatter != nil);
    
    icu::UnicodeString t_string;
    p_formatter->m_icu_formatter.format(t_num, t_string);
    return MCStringCreateWithICUString(t_string, r_string);
}

bool MCLocaleNumberFormatReal(MCNumberFormatterRef p_formatter, real64_t t_num, MCStringRef &r_string)
{
    MCAssert(p_formatter != nil);
    
    icu::UnicodeString t_string;
    p_formatter->m_icu_formatter.format(t_num, t_string);
    return MCStringCreateWithICUString(t_string, r_string);
}

bool MCLocaleNumberParseInteger(MCNumberFormatterRef p_formatter, MCStringRef p_string, int64_t &r_num)
{
    MCAssert(p_formatter != nil);
    
    UErrorCode t_error = U_ZERO_ERROR;
    icu::UnicodeString t_string(MCStringGetCharPtr(p_string), MCStringGetLength(p_string));
    icu::Formattable t_formattable;
    p_formatter->m_icu_formatter.parse(t_string, t_formattable, t_error);
    if (U_FAILURE(t_error))
        return false;
    
    r_num = t_formattable.getInt64();
    return true;
}

bool MCLocaleNumberParseReal(MCNumberFormatterRef p_formatter, MCStringRef p_string, real64_t &r_num)
{
    MCAssert(p_formatter != nil);
    
    UErrorCode t_error = U_ZERO_ERROR;
    icu::UnicodeString t_string(MCStringGetCharPtr(p_string), MCStringGetLength(p_string));
    icu::Formattable t_formattable;
    p_formatter->m_icu_formatter.parse(t_string, t_formattable, t_error);
    if (U_FAILURE(t_error))
        return false;
    
    r_num = t_formattable.getDouble();
    return true;
}

////////////////////////////////////////////////////////////////////////////////

bool MCCalendarCreate(MCLocaleRef p_locale, MCTimeZoneRef p_time_zone, MCCalendarType p_type, MCCalendarRef &r_calendar)
{
    MCAssert(p_locale != nil);
    MCAssert(p_time_zone != nil);
    
    // Create a time zone object describing the supplied time zone
    UErrorCode t_error = U_ZERO_ERROR;
    icu::UnicodeString t_tz_name(MCStringGetCharPtr(p_time_zone), MCStringGetLength(p_time_zone));
    icu::TimeZone *t_tz = icu::TimeZone::createTimeZone(t_tz_name);
    
    // Create the calendar. The calendar takes ownership of the time zone
    icu::Calendar *t_calendar;
    t_calendar = icu::Calendar::createInstance(t_tz, p_locale->m_icu_locale, t_error);
    if (U_FAILURE(t_error))
        return false;
    
    // Construct the LiveCode calendar object
    r_calendar = new __MCCalendar(*t_calendar);
    return true;
}

void MCCalendarRelease(MCCalendarRef p_calendar)
{
    MCAssert(p_calendar != nil);
    delete p_calendar;
}

////////////////////////////////////////////////////////////////////////////////
