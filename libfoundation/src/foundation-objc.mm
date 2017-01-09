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
#include <foundation-objc.h>

////////////////////////////////////////////////////////////////////////////////

NSString *MCStringConvertToAutoreleasedNSString(MCStringRef p_string_ref)
{
	CFStringRef t_string;
	/* UNCHECKED */ MCStringConvertToCFStringRef(p_string_ref, t_string);
	return [((NSString *)t_string) autorelease];
}

NSString *MCNameConvertToAutoreleasedNSString(MCNameRef p_name_ref)
{
	CFStringRef t_string;
	/* UNCHECKED */ MCStringConvertToCFStringRef(MCNameGetString(p_name_ref), t_string);
	return [((NSString *)t_string) autorelease];
}

NSData *MCDataConvertToAutoreleasedNSData(MCDataRef p_data_ref)
{
	CFDataRef t_data;
	/* UNCHECKED */ MCDataConvertToCFDataRef(p_data_ref, t_data);
	return [((NSData *)t_data) autorelease];
}

////////////////////////////////////////////////////////////////////////////////

void MCCFAutorelease(const void *p_object)
{
    // CFAutorelease isn't exposed until MacOSX 10.9
    id t_object = (id)p_object;
    [t_object autorelease];
}

////////////////////////////////////////////////////////////////////////////////
