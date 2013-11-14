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

#include "unicode/locid.h"

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
    
public:
    
    __MCLocale(icu::Locale& p_icu_locale)
    : m_icu_locale(p_icu_locale)
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

////////////////////////////////////////////////////////////////////////////////

MCLocaleRef MCLbasic;
MCLocaleRef MCLdefault;
MCLocaleRef MCLcurrent;

////////////////////////////////////////////////////////////////////////////////

bool __MCLocaleInitialize()
{
    bool t_success;
    t_success = true;
    
    // Create the well-known locales
    if (t_success)
        t_success = MCLocaleCreateWithName(MCSTR("en_US"), MCLbasic);
    if (t_success)
        t_success = MCLocaleCreateDefault(MCLdefault);
    if (t_success)
        t_success = MCLocaleCreateDefault(MCLcurrent);
    
    return true;
}

void __MCLocaleFinalize()
{
    // Destroy the well-known locals
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

void MCLocaleRelease(MCLocaleRef p_locale)
{
    MCAssert(p_locale != nil);
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
