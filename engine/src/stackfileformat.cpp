/* Copyright (C) 2016 LiveCode Ltd.
 
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

#include "prefix.h"

#include "stackfileformat.h"

static char s_metacard_version_string[kMCStackFileMetaCardVersionStringLength] = kMCStackFileMetaCardVersionString;

// Future file format versions will always still compare greater than MAX_STACKFILE_VERSION,
// so it is ok that r_version does not accurately reflect future version values.
// TODO: change this, and comparisons for version >= 7000 / 5500, etc
bool MCStackFileParseVersionNumber(const char *p_buffer, uint32_t &r_version)
{
	if (!MCCStringBeginsWith(p_buffer, kMCStackFileVersionStringPrefix))
		return false;
	
	p_buffer += kMCStackFileVersionStringPrefixLength;
	
	uint32_t t_version;
	t_version = 0;
	
	for (uint32_t i = 0; i < 4; i++)
	{
		t_version *= 10;
		
		// The header version can now consist of any alphanumeric characters
		// They map to numbers as follows:
		// 0-9 -> 0-9
		// A-Z -> 10-35
		// a-z -> 36-61
		
		char t_char;
		t_char = p_buffer[i];
		if ('0' <= t_char && t_char <= '9')
			t_version += (uint1)(t_char - '0');
		else if ('A' <= t_char && t_char <= 'Z')
			t_version += (uint1)(t_char - 'A' + 10);
		else if ('a' <= t_char && t_char <= 'z')
			t_version += (uint1)(t_char - 'a' + 36);
		else
			return false;
	}
	
	r_version = t_version;
	
	return true;
}

// map the version to one of the supported stack file versions
uint32_t MCStackFileMapToSupportedVersion(uint32_t p_version)
{
	if (p_version >= kMCStackFileFormatVersion_8_1)
		return kMCStackFileFormatVersion_8_1;
	
	if (p_version >= kMCStackFileFormatVersion_8_0)
		return kMCStackFileFormatVersion_8_0;
	
	if (p_version >= kMCStackFileFormatVersion_7_0)
		return kMCStackFileFormatVersion_7_0;
	
	if (p_version >= kMCStackFileFormatVersion_5_5)
		return kMCStackFileFormatVersion_5_5;
	
	if (p_version >= kMCStackFileFormatVersion_2_7)
		return kMCStackFileFormatVersion_2_7;
	
	return p_version;
}

void MCStackFileGetHeaderForVersion(uint32_t p_version, const char *&r_header, uint32_t &r_size)
{
	switch (MCStackFileMapToSupportedVersion(p_version))
	{
		case kMCStackFileFormatVersion_8_1:
			r_header = kMCStackFileVersionString_8_1;
			r_size = kMCStackFileVersionStringLength;
			break;
			
		case kMCStackFileFormatVersion_8_0:
			r_header = kMCStackFileVersionString_8_0;
			r_size = kMCStackFileVersionStringLength;
			break;
			
		case kMCStackFileFormatVersion_7_0:
			r_header = kMCStackFileVersionString_7_0;
			r_size = kMCStackFileVersionStringLength;
			break;
			
		case kMCStackFileFormatVersion_5_5:
			r_header = kMCStackFileVersionString_5_5;
			r_size = kMCStackFileVersionStringLength;
			break;
			
		case kMCStackFileFormatVersion_2_7:
			r_header = kMCStackFileVersionString_2_7;
			r_size = kMCStackFileVersionStringLength;
			break;
			
		default:
			r_header = s_metacard_version_string;
			r_size = kMCStackFileMetaCardVersionStringLength;
			break;
	}
}

