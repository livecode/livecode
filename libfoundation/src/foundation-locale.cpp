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

#include "unicode/uloc.h"

////////////////////////////////////////////////////////////////////////////////

struct __MCLocale
{
    const icu::Locale& m_icu_locale;
};

////////////////////////////////////////////////////////////////////////////////

bool __MCLocaleInitialize()
{
    return true;
}

void __MCLocaleFinalize()
{
    ;
}

////////////////////////////////////////////////////////////////////////////////

const icu::Locale& MCLocaleGetICULocale(MCLocaleRef p_locale)
{
    MCAssert(p_locale != nil);
    return p_locale->m_icu_locale;
}
