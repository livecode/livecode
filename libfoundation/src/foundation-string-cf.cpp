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

#define integer_t cf_integer_t
#include <CoreFoundation/CoreFoundation.h>
#undef integer_t

////////////////////////////////////////////////////////////////////////////////

bool MCStringCreateWithCFString(CFStringRef p_cf_string, MCStringRef& r_string)
{
	bool t_success;
	t_success = true;
	
	CFIndex t_string_length;
	t_string_length = CFStringGetLength(p_cf_string);
	
	CFIndex t_buffer_size;
	t_buffer_size = CFStringGetMaximumSizeForEncoding(t_string_length, kCFStringEncodingUnicode) + 1;
	
	MCAutoPointer<byte_t> t_buffer;
	t_success = MCMemoryNewArray(t_buffer_size, &t_buffer);
	
	CFIndex t_used_size;
	if (t_success)
		CFStringGetBytes(p_cf_string, CFRangeMake(0, t_string_length), kCFStringEncodingUnicode, '?', false, (UInt8*)*t_buffer, t_buffer_size, &t_used_size);
	
	if (t_success)
		t_success = MCStringCreateWithChars((unichar_t *)*t_buffer, t_used_size / 2, r_string);
	
	return t_success;
}

////////////////////////////////////////////////////////////////////////////////
