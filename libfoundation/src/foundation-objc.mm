/* Copyright (C) 2003-2013 Runtime Revolution Ltd.
 
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

@implementation NSString (com_runrev_livecode_foundation_NSStringAdditions)

+ (NSString *)stringWithMCStringRef: (MCStringRef)p_string_ref;
{
	CFStringRef t_string;
	/* UNCHECKED */ MCStringConvertToCFStringRef(p_string_ref, t_string);
	return [((NSString *)t_string) autorelease];
}

+ (NSString *)stringWithMCNameRef: (MCNameRef)p_name_ref;
{
	CFStringRef t_string;
	/* UNCHECKED */ MCStringConvertToCFStringRef(MCNameGetString(p_name_ref), t_string);
	return [((NSString *)t_string) autorelease];
}

@end

////////////////////////////////////////////////////////////////////////////////

@implementation NSData (com_runrev_livecode_foundation_NSDataAdditions)

+ (NSData *)dataWithMCDataRef: (MCDataRef)p_data_ref;
{
	CFDataRef t_data;
	/* UNCHECKED */ MCDataConvertToCFDataRef(p_data_ref, t_data);
	return [((NSData *)t_data) autorelease];
}

@end

////////////////////////////////////////////////////////////////////////////////