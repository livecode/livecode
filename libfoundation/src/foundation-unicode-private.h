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

#ifndef __MC_FOUNDATION_UNICODE_PRIVATE__
#define __MC_FOUNDATION_UNICODE_PRIVATE__

#include "foundation-locale.h"
#include "foundation-unicode.h"

#include "unicode/uloc.h"

////////////////////////////////////////////////////////////////////////////////

// Given a LocaleRef, returns the underlying ICU locale object
const icu::Locale& MCLocaleGetICULocale(MCLocaleRef);

// Converts from an ICU string to a StringRef
bool MCStringCreateWithICUString(icu::UnicodeString&, MCStringRef &r_string);

// Converts from a StringRef to an ICU string
bool MCStringConvertToICUString(MCStringRef p_string, icu::UnicodeString&);

////////////////////////////////////////////////////////////////////////////////

#endif  // ifndef __MC_FOUNDATION_UNICODE_PRIVATE__
