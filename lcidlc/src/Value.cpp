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

#include <stdlib.h>

#include "Value.h"
#include "CString.h"

////////////////////////////////////////////////////////////////////////////////

struct Value
{
	ValueType type;
	uint32_t references;
	union
	{
		bool boolean;
		int64_t integer;
		double real;
		struct
		{
			char *chars;
			uint32_t char_count;
		} string;
		struct
		{
			Value *next;
			ValueRef string;
		} name;
	};
};

static Value *s_names = nil;

////////////////////////////////////////////////////////////////////////////////

ValueRef ValueRetain(ValueRef self)
{
	if (self == nil)
		return nil;
		
	self -> references += 1;
	
	return self;
}

void ValueRelease(ValueRef self)
{
	if (self == nil)
		return;
		
	self -> references -= 1;
	
	if (self -> references != 0)
		return;
	
	switch(self -> type)
	{
	case kValueTypeEmpty:
	case kValueTypeBoolean:
	case kValueTypeInteger:
	case kValueTypeReal:
	break;
	
	case kValueTypeName:
		if (s_names == self)
			s_names = self -> name . next;
		else
		{
			Value *t_name;
			for(t_name = s_names; t_name != nil && t_name -> name . next != self; t_name = t_name -> name . next)
				;
            if (t_name != nil)
                t_name -> name . next = self -> name . next;
		}
		
		ValueRelease(self -> name . string);
	break;
	
	case kValueTypeString:
		MCMemoryDeleteArray(self -> string . chars);
	break;
	}
	
	MCMemoryDelete(self);
}

// MERG-2013-06-14: [[ ExternalsApiV5 ]] Implement type checking methods.

bool ValueIsEmpty(ValueRef self)
{
    if (self == nil)
		return false;
	return self->type == kValueTypeEmpty;
}

bool ValueIsBoolean(ValueRef self)
{
    if (self == nil)
		return false;
	return self->type == kValueTypeBoolean;
}

bool ValueIsInteger(ValueRef self)
{
    if (self == nil)
		return false;
	return self->type == kValueTypeInteger;
}

bool ValueIsReal(ValueRef self)
{
    if (self == nil)
		return false;
    return self->type == kValueTypeReal;
}

bool ValueIsString(ValueRef self)
{
    if (self == nil)
		return false;
	return self->type == kValueTypeString;
}

bool ValueIsName(ValueRef self)
{
    if (self == nil)
		return false;
	return self->type == kValueTypeName;
}

////////////////////////////////////////////////////////////////////////////////

bool NameCreateWithNativeChars(const char *p_chars, uindex_t p_char_count, ValueRef& r_value)
{
	bool t_success;
	t_success = true;
	
	for(Value *t_name = s_names; t_name != nil; t_name = t_name -> name . next)
		if (StringEqualToChars(t_name -> name . string, p_chars, p_char_count))
		{
			t_name -> references += 1;
			r_value = t_name;
			return true;
		}
		
	Value *t_string;
	t_string = nil;
	if (t_success)
		t_success = StringCreateWithNativeChars(p_chars, p_char_count, t_string);
	
	Value *self;
	self = nil;
	if (t_success)
		t_success = MCMemoryNew(self);
		
	if (t_success)
	{
		self -> references = 1;
		self -> type = kValueTypeName;
		self -> name . string = t_string;
		self -> name . next = s_names;
		s_names = self;
		
		r_value = self;
	}
	else
	{
		MCMemoryDelete(self);
		ValueRelease(t_string);
	}
	
	return t_success;
}

bool NameEqual(NameRef p_left_name, NameRef p_right_name)
{
	return p_left_name == p_right_name;
}

bool NameEqualCaseless(NameRef p_left_name, NameRef p_right_name)
{
	return p_left_name == p_right_name || MCCStringEqualCaseless(StringGetCStringPtr(p_left_name -> name . string), StringGetCStringPtr(p_right_name -> name . string));
}

bool NameEqualToCString(NameRef self, const char *p_cstring)
{
	return MCCStringEqual(StringGetCStringPtr(self -> name . string), p_cstring);
}

StringRef NameGetString(StringRef self)
{
	return self -> name . string;
}

const char *NameGetCString(NameRef self)
{
	return StringGetCStringPtr(self -> name . string);
}

////////////////////////////////////////////////////////////////////////////////

bool StringCreateWithNativeChars(const char *p_chars, uindex_t p_char_count, ValueRef& r_value)
{
	bool t_success;
	t_success = true;
	
	Value *self;
	self = nil;
	if (t_success)
		t_success = MCMemoryNew(self);
	
	char *t_chars;
	t_chars = nil;
	if (t_success)
		t_success = MCMemoryNewArray(p_char_count + 1, t_chars);
		
	if (t_success)
	{
		MCMemoryCopy(t_chars, p_chars, p_char_count);
		
		// MW-2012-09-04: [[ Bug ]] NUL terminator should be at char_count, not char_count+1.
		t_chars[p_char_count] = '\0';
		
		self -> references = 1;
		self -> type = kValueTypeString;
		self -> string . chars = t_chars;
		self -> string . char_count = p_char_count;
		
		r_value = self;
	}
	else
	{
		MCMemoryDeleteArray(t_chars);
		MCMemoryDelete(self);
	}
		
	return t_success;
}

bool StringEqualCaseless(StringRef left, StringRef right)
{
	if (left -> string . char_count != right -> string . char_count)
		return false;
	return MCCStringEqualCaseless(left -> string . chars, right -> string . chars);
}

bool StringEqualToChars(StringRef self, const char *p_chars, uindex_t p_char_count)
{
	if (self -> string . char_count != p_char_count)
		return false;
		
	return MCCStringEqualSubstring(self -> string . chars, p_chars, p_char_count);
}

const char *StringGetCStringPtr(StringRef self)
{
	return self -> string . chars;
}

////////////////////////////////////////////////////////////////////////////////

bool NumberCreateWithNativeChars(const char *p_chars, uindex_t p_char_count, ValueRef& r_value)
{
	bool t_success;
	t_success = true;
	
	char *t_cstring;
	t_cstring = nil;
	if (t_success)
		t_success = MCCStringCloneSubstring(p_chars, p_char_count, t_cstring);
	
	ValueRef self;
	self = nil;
	if (t_success)
		t_success = MCMemoryNew(self);
	
	if (t_success)
	{
		self -> references = 1;

		char *t_end;
#ifdef WIN32
		self -> integer = _strtoui64(t_cstring, &t_end, 10);
#else
		self -> integer = strtoull(t_cstring, &t_end, 10);
#endif
		if (*t_end == '\0')
			self -> type = kValueTypeInteger;
		else
		{
			self -> type = kValueTypeReal;
			self -> real = strtod(t_cstring, &t_end);
		}
	}
	
	if (t_success)
		r_value = self;
	else
		MCMemoryDelete(self);
	
	MCCStringFree(t_cstring);
	
	return t_success;
}

bool NumberIsInteger(NumberRef self)
{
	return self -> type == kValueTypeInteger;
}

int64_t NumberGetInteger(NumberRef self)
{
	return self -> integer;
}

double NumberGetReal(NumberRef self)
{
	if (self -> type == kValueTypeInteger)
		return (double)self -> integer;
	return self -> real;
}

////////////////////////////////////////////////////////////////////////////////

// MERG-2013-06-14: [[ ExternalsApiV5 ]] Implement boolean ValueRef methods.
bool BooleanCreateWithBool(bool value, ValueRef& r_value)
{
    bool t_success;
	t_success = true;
	
	Value *self;
	self = nil;
	if (t_success)
		t_success = MCMemoryNew(self);
    
	if (t_success)
	{
		self -> references = 1;
		self -> type = kValueTypeBoolean;
        self -> boolean = value;
        
		r_value = self;
	}
	else
	{
		MCMemoryDelete(self);
	}
	
	return t_success;

}

bool BooleanGetBool(ValueRef self)
{
    return self -> boolean;
}

////////////////////////////////////////////////////////////////////////////////

