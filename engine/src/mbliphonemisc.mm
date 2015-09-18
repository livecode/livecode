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

#include "textlayout.h"

#include <CoreText/CoreText.h>

////////////////////////////////////////////////////////////////////////////////

int32_t MCCustomPrinterComputeFontSize(void *font)
{
	return CTFontGetSize((CTFontRef)font);
}

////////////////////////////////////////////////////////////////////////////////

char *MCSystemLowercaseInternational(const MCString& p_string)
{
	char *t_lc_string;
	t_lc_string = p_string . clone();
	for(uindex_t i = 0; t_lc_string[i] != '\0'; i++)
		t_lc_string[i] = MCS_tolower(t_lc_string[i]);
	return t_lc_string;
}

int MCSystemCompareInternational(MCStringRef p_left, MCStringRef p_right)
{
	CFStringRef t_left_ref, t_right_ref;
    /* UNCHECKED */ MCStringConvertToCFStringRef(p_left, t_left_ref);
    /* UNCHECKED */ MCStringConvertToCFStringRef(p_right, t_right_ref);

	
	// MW-2013-03-12: [[ Bug 10445 ]] Make sure we compare the string 'localized'.
	int t_result;
	t_result = CFStringCompare(t_left_ref, t_right_ref, kCFCompareLocalized);
	
	CFRelease(t_left_ref);
	CFRelease(t_right_ref);
	
	return t_result;
}

////////////////////////////////////////////////////////////////////////////////
