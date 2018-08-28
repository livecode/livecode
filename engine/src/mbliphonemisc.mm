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

bool MCSystemRequestPermission(MCStringRef p_permission, bool& r_granted)
{
    // Not implemented
    return false;
}

bool MCSystemPermissionExists(MCStringRef p_permission, bool& r_exists)
{
    // Not implemented
    return false;
}

bool MCSystemHasPermission(MCStringRef p_permission, bool& r_permission_granted)
{
    // Not implemented
    return false;
}

////////////////////////////////////////////////////////////////////////////////
