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

#include <foundation.h>
#include <foundation-auto.h>

#include "foundation-private.h"

////////////////////////////////////////////////////////////////////////////////

bool MCListCreateMutable(char_t p_delimiter, MCListRef& r_list)
{
	__MCList *self;
	if (!__MCValueCreate(kMCValueTypeCodeList, self))
		return false;

	MCStringCreateWithNativeChars((const char_t *)&p_delimiter, 1, self -> delimiter);

	r_list = self;

	return true;
}

bool MCListCreateMutable(MCStringRef p_delimiter, MCListRef& r_list)
{
	__MCList *self;
	if (!__MCValueCreate(kMCValueTypeCodeList, self))
		return false;
    
	self -> delimiter = MCValueRetain(p_delimiter);
    
	r_list = self;
    
	return true;
}

MC_DLLEXPORT_DEF
bool MCListAppend(MCListRef self, MCValueRef p_value)
{
	__MCAssertIsList(self);
	MCAssert(nil != p_value);

	bool t_first = self->buffer == nil;
	if (t_first)
		if (!MCStringCreateMutable(0, self -> buffer))
			return false;

	MCStringRef t_string = nil;

	switch (MCValueGetTypeCode(p_value))
	{
	case kMCValueTypeCodeName:
		t_string = MCNameGetString((MCNameRef)p_value);
		break;

	case kMCValueTypeCodeString:
		t_string = (MCStringRef)p_value;
		break;

	case kMCValueTypeCodeBoolean:
		t_string = kMCTrue == (MCBooleanRef)p_value ? kMCTrueString : kMCFalseString;
		break;

	case kMCValueTypeCodeList:
		t_string = ((MCListRef)p_value)->buffer;
		if (t_string == nil)
			t_string = kMCEmptyString;
		break;

	default:
		if (!MCStringFormat(t_string, "%@", p_value))
		{
			return false;
		}
		break;
	}
	if (!t_first && !MCStringAppend(self -> buffer, self -> delimiter))
		return false;

	return MCStringAppend(self -> buffer, t_string);
}

MC_DLLEXPORT_DEF
bool MCListCopy(MCListRef self, MCListRef& r_list)
{
	__MCAssertIsList(self);
    
    // If we are immutable, just bump the reference count
	if (!(self -> flags & kMCListFlagIsMutable))
    {
        r_list = MCValueRetain(self);
        return true;
    }
    
    // Create a new list
    MCListRef t_new_list;
    if (!MCListCreateMutable(self -> delimiter, t_new_list))
        return false;
    
    // Mark the new list as immutable
    t_new_list -> flags &= ~kMCListFlagIsMutable;
    
    // Create an immutable copy of the list contents
    if (self -> buffer == nil)
    {
        t_new_list -> buffer = nil;
    }
    else if (!MCStringCopy(self -> buffer, t_new_list -> buffer))
    {
        MCValueRelease(t_new_list);
        return false;
    }
    
    r_list = t_new_list;
	return true;
}

MC_DLLEXPORT_DEF
bool MCListCopyAndRelease(MCListRef self, MCListRef& r_list)
{
	__MCAssertIsList(self);

    // If there are no other references, just clear the mutable flag
    if (self -> references == 1)
    {
        self -> flags &= ~kMCListFlagIsMutable;
        r_list = self;
        return true;
    }
    
    // Otherwise perform a normal copy operation followed by a release
    if (!MCListCopy(self, r_list))
        return false;
    MCValueRelease(self);
    return true;
}

MC_DLLEXPORT_DEF
bool MCListCopyAsString(MCListRef self, MCStringRef& r_string)
{
	__MCAssertIsList(self);

	MCStringRef t_string;
	if (self -> buffer != nil)
		t_string = self -> buffer;
	else
		t_string = kMCEmptyString;

	if (!MCStringCopy(t_string, r_string))
		return false;

	return true;
}

MC_DLLEXPORT_DEF
bool MCListCopyAsStringAndRelease(MCListRef self, MCStringRef& r_string)
{
	if (!MCListCopyAsString(self, r_string))
		return false;

	MCValueRelease(self);

	return true;
}

MC_DLLEXPORT_DEF
bool MCListAppendFormat(MCListRef self, const char *p_format, ...)
{
	__MCAssertIsList(self);
	MCAssert(nil != p_format);

	bool t_success;
	t_success = true;

	MCStringRef t_string;
	t_string = nil;
	
	va_list t_args;
	va_start(t_args, p_format);
	t_success = MCStringFormatV(t_string, p_format, t_args);
	va_end(t_args);
	
	if (t_success)
		t_success = MCListAppend(self, t_string);
	
	if (t_string != nil)
		MCValueRelease(t_string);
	
	return t_success;
}

MC_DLLEXPORT_DEF
bool MCListAppendNativeChars(MCListRef self, const char_t *p_chars, uindex_t p_char_count)
{
	__MCAssertIsList(self);
	MCAssert(p_chars != nil);

	bool t_first = self->buffer == nil;
	if (t_first)
		if (!MCStringCreateMutable(0, self -> buffer))
			return false;

	if (!t_first && !MCStringAppend(self -> buffer, self -> delimiter))
		return false;

	return MCStringAppendNativeChars(self -> buffer, p_chars, p_char_count);
}

MC_DLLEXPORT_DEF
bool MCListAppendSubstring(MCListRef self, MCStringRef p_string, MCRange p_range)
{
	return MCListAppendFormat(self, "%*@", &p_range, p_string);
}

MC_DLLEXPORT_DEF
bool MCListIsEmpty(MCListRef self)
{
	__MCAssertIsList(self);
	return self -> buffer == nil;
}

////////////////////////////////////////////////////////////////////////////////

MC_DLLEXPORT_DEF MCListRef kMCEmptyList;

bool __MCListInitialize(void)
{
	if (!MCListCreateMutable('\n', kMCEmptyList))
		return false;

	return true;
}

void __MCListFinalize(void)
{
}

void __MCListDestroy(__MCList *self)
{
    MCValueRelease(self -> delimiter);
	MCValueRelease(self -> buffer);
}

hash_t __MCListHash(__MCList *self)
{
	if (self -> buffer == nil)
		return MCStringHash(kMCEmptyString, kMCStringOptionCompareExact);
	return MCStringHash(self -> buffer, kMCStringOptionCompareExact);
}

bool __MCListIsEqualTo(__MCList *list, __MCList *other_list)
{
	return MCStringIsEqualTo(list -> buffer != nil ? list -> buffer : kMCEmptyString, other_list -> buffer != nil ? other_list -> buffer : kMCEmptyString, kMCStringOptionCompareExact);
}

bool __MCListCopyDescription(__MCList *self, MCStringRef& r_string)
{
	MCAutoStringRef t_self_string;
	if (!MCListCopyAsString (self, &t_self_string))
		return false;
	return MCValueCopyDescription(*t_self_string, r_string);
}

bool __MCListImmutableCopy(__MCList *self, bool p_release, __MCList*& r_immutable_value)
{
	if (!p_release)
        return MCListCopy(self, r_immutable_value);
    
    return MCListCopyAndRelease(self, r_immutable_value);
}

////////////////////////////////////////////////////////////////////////////////
