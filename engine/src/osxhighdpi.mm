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
#include "parsedef.h"
#include "filedefs.h"
#include "objdefs.h"

#include "dispatch.h"

#include "graphics.h"
#include "stacklst.h"

#include <AppKit/AppKit.h>

////////////////////////////////////////////////////////////////////////////////

typedef float (*backingScaleFactorIMP)(id);

// IM-2014-01-23; [[ HiDPI ]] Returns the backing scale of the display. Note that this will only be available on OSX versions 10.7 and later.
bool MCOSXGetDisplayPixelScale(NSScreen *p_display, MCGFloat &r_scale)
{
	static backingScaleFactorIMP s_backingScaleFactor = nil;
	static bool s_initialized = false;
	
	if (!s_initialized)
	{
		// Make sure the NSScreen object has a "backingScale" method - present from 10.7 onward
		s_initialized = true;
		if (![NSScreen instancesRespondToSelector:@selector(backingScaleFactor)])
			return false;
		
		// Objects can handle messages not specified in their header, but there seems to be no way to specify the return type,
		// so here we get the method implementation and cast to a function with the expected return type (float).
		s_backingScaleFactor = (backingScaleFactorIMP)[NSScreen instanceMethodForSelector:@selector(backingScaleFactor)];
	}
	
	if (s_backingScaleFactor == nil)
		return false;
	
	r_scale = s_backingScaleFactor(p_display);
	return true;
}

// IM-2014-01-27: [[ HiDPI ]] OSX Supports pixel scaling on retina displays
bool MCResPlatformSupportsPixelScaling(void)
{
	return true;
}

// IM-2014-01-27: [[ HiDPI ]] HiDPI support can be enabled/disabled on OSX by recreating
// each stack window with/without the kWindowFrameWorkScaledAttribute flag
bool MCResPlatformCanChangePixelScaling(void)
{
	return true;
}

// IM-2014-01-30: [[ HiDPI ]] Cannot set pixel scale on OSX
bool MCResPlatformCanSetPixelScale(void)
{
	return false;
}

// IM-2014-01-30: [[ HiDPI ]] Pixel scale is 1.0 on OSX
MCGFloat MCResPlatformGetDefaultPixelScale(void)
{
	return 1.0;
}

// IM-2014-03-14: [[ HiDPI ]] UI scale is 1.0 on OSX
MCGFloat MCResPlatformGetUIDeviceScale(void)
{
	return 1.0;
}

// IM-2014-01-30: [[ HiDPI ]] Reopen windows when usePixelScale is changed
void MCResPlatformHandleScaleChange(void)
{
	// Global use-pixel-scaling value has been updated, so now we just need to reopen any open stack windows
	MCstacks->reopenallstackwindows();
}

////////////////////////////////////////////////////////////////////////////////
