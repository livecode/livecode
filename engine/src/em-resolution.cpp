/*                                                                     -*-c++-*-

Copyright (C) 2003-2015 LiveCode Ltd.

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

#include "em-util.h"

#include "sysdefs.h"

#include "graphics.h"
#include "resolution.h"

#include <emscripten.h>

/* ================================================================
 * Resolution independence
 * ================================================================ */

/* FIXME use emscripten_get_device_pixel_ratio() */

void
MCResPlatformInitPixelScaling()
{
}

bool
MCResPlatformSupportsPixelScaling()
{
	return false;
}

bool
MCResPlatformCanChangePixelScaling()
{
	return false;
}

bool
MCResPlatformCanSetPixelScale()
{
	return false;
}

MCGFloat
MCResPlatformGetDefaultPixelScale()
{
	MCEmscriptenNotImplemented();
	return NAN;
}

MCGFloat
MCResPlatformGetUIDeviceScale()
{
	MCEmscriptenNotImplemented();
	return NAN;
}

void
MCResPlatformHandleScaleChange()
{
	MCEmscriptenNotImplemented();
}
