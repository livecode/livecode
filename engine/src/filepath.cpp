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

#include "prefix.h"

#include "foundation.h"
#include "foundation-auto.h"

bool MCPathIsRemoteURL(MCStringRef p_path)
{
	return MCStringBeginsWithCString(p_path, (char_t*)"http://", kMCStringOptionCompareCaseless) ||
	MCStringBeginsWithCString(p_path, (char_t*)"https://", kMCStringOptionCompareCaseless) ||
	MCStringBeginsWithCString(p_path, (char_t*)"ftp://", kMCStringOptionCompareCaseless);
}

bool MCPathBeginsWithSeparator(MCStringRef p_path)
{
	return MCStringGetLength(p_path) > 0 && MCStringGetNativeCharAtIndex(p_path, 0) == '/';
}

bool MCPathBeginsWithWindowsDriveLetter(MCStringRef p_path)
{
	return MCStringGetLength(p_path) > 1 && MCStringGetNativeCharAtIndex(p_path, 1) == ':';
}

bool MCPathIsAbsolute(MCStringRef p_path)
{
	return MCPathBeginsWithSeparator(p_path) || MCPathBeginsWithWindowsDriveLetter(p_path);
}

bool MCPathEndsWithSeparator(MCStringRef p_path)
{
	uindex_t t_length;
	t_length = MCStringGetLength(p_path);
	
	if (MCStringGetNativeCharAtIndex(p_path, t_length - 1) == '/')
		return true;
	
	if (t_length == 2 && MCPathBeginsWithWindowsDriveLetter(p_path))
		return true;
	
	return false;
}

bool MCPathAppend(MCStringRef p_base, MCStringRef p_path, MCStringRef &r_new)
{
	if (MCStringIsEmpty(p_path))
		return MCStringCopy(p_base, r_new);
	
	if (MCStringIsEmpty(p_base))
		return MCStringCopy(p_path, r_new);
	
	MCAutoStringRef t_new;
	if (!MCStringMutableCopy(p_base, &t_new))
		return false;
	
	uindex_t t_offset;
	t_offset = 0;
	
	// skip over any extraneous path separators in the suffix
	while (t_offset < MCStringGetLength(p_path) && MCStringGetNativeCharAtIndex(p_path, t_offset) == '/')
		t_offset++;
	
	if (!MCPathEndsWithSeparator(*t_new))
	{
		if (t_offset > 0)
			// suffix begins with at least one path separator so copy that
			t_offset--;
		else
			// need to add separator between prefix & suffix.
			if (!MCStringAppendNativeChar(*t_new, '/'))
				return false;
	}
	
	if (!MCStringAppendSubstring(*t_new, p_path, MCRangeMakeMinMax(t_offset, MCStringGetLength(p_path))))
		return false;
	
	return MCStringCopy(*t_new, r_new);
}

