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

#ifndef __VALUE__
#define __VALUE__

#include "foundation.h"

////////////////////////////////////////////////////////////////////////////////

typedef struct Value *ValueRef;
typedef struct Value *StringRef;
typedef struct Value *NumberRef;
typedef struct Value *NameRef;

enum ValueType
{
	kValueTypeEmpty,
	kValueTypeBoolean,
	kValueTypeInteger,
	kValueTypeReal,
	kValueTypeString,
	kValueTypeName
};

ValueRef ValueRetain(ValueRef value);
void ValueRelease(ValueRef value);

bool ValueIsEmpty(ValueRef value);
bool ValueIsBoolean(ValueRef value);
bool ValueIsInteger(ValueRef value);
bool ValueIsReal(ValueRef value);
bool ValueIsString(ValueRef value);
bool ValueIsName(ValueRef value);

////////////////////////////////////////////////////////////////////////////////

bool NameCreateWithNativeChars(const char *p_chars, uindex_t p_char_count, ValueRef& r_value);
StringRef NameGetString(NameRef name);
const char *NameGetCString(NameRef name);
bool NameEqual(NameRef left_name, NameRef right_name);
bool NameEqualCaseless(NameRef left_name, NameRef right_name);
bool NameEqualToCString(NameRef name, const char *cstring);

////////////////////////////////////////////////////////////////////////////////

bool StringCreateWithNativeChars(const char *p_chars, uindex_t p_char_count, ValueRef& r_value);

const char *StringGetCStringPtr(StringRef string);
bool StringEqualToChars(StringRef string, const char *p_chars, uindex_t p_char_count);
bool StringEqualToCharsCaseless(StringRef string, const char *p_chars, uindex_t p_char_count);
bool StringEqualCaseless(StringRef string, StringRef string_b);

////////////////////////////////////////////////////////////////////////////////

bool NumberCreateWithNativeChars(const char *p_chars, uindex_t p_char_count, ValueRef& r_value);

bool NumberIsInteger(ValueRef value);

int64_t NumberGetInteger(ValueRef value);
double NumberGetReal(ValueRef value);

////////////////////////////////////////////////////////////////////////////////

// MERG-2013-06-14: [[ ExternalsApiV5 ]] Implement boolean ValueRef type.
bool BooleanCreateWithBool(bool value, ValueRef& r_value);

bool BooleanGetBool(ValueRef value);

#endif
