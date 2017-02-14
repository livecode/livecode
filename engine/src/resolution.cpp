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

#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"
#include "globals.h"
#include "dispatch.h"

#include "uidc.h"

#include "exec.h"

#include "graphics.h"
#include "resolution.h"

////////////////////////////////////////////////////////////////////////////////

// IM-2014-01-30: [[ HiDPI ]] Enable / disable pixel scaling
static bool s_res_use_pixel_scaling = true;
static MCGFloat s_res_pixel_scale = 1.0;

////////////////////////////////////////////////////////////////////////////////

MCGFloat MCResGetPixelScale(void)
{
	return s_res_pixel_scale;
}

MCGFloat MCResGetUIScale(void)
{
	return MCResGetPixelScale() / MCResPlatformGetUIDeviceScale();
}

void MCResSetPixelScale(MCGFloat p_scale)
{
	// IM-2014-01-30: [[ HiDPI ]] Return if pixel scaling is not in use
	if (!s_res_use_pixel_scaling)
		return;
	
	if (p_scale == s_res_pixel_scale)
		return;

	s_res_pixel_scale = p_scale;
	
	// IM-2014-01-30: [[ HiDPI ]] Use per-platform change handler
	MCResPlatformHandleScaleChange();
}

////////////////////////////////////////////////////////////////////////////////

void MCResInitPixelScaling(void)
{
	// IM-2014-08-14: [[ Bug 12372 ]] Perform platform-specific setup.
	MCResPlatformInitPixelScaling();

	// If pixel scaling is available then use it by default
	s_res_use_pixel_scaling = MCResPlatformSupportsPixelScaling();
	
	// IM-2014-01-30: [[ HiDPi ]] Initialise pixel scale to the default for this platform
	if (s_res_use_pixel_scaling)
		s_res_pixel_scale = MCResPlatformGetDefaultPixelScale();
	else
		s_res_pixel_scale = 1.0;
}

void MCResSetUsePixelScaling(bool p_use_scaling)
{
	if (p_use_scaling == s_res_use_pixel_scaling || !MCResPlatformCanChangePixelScaling())
		return;
	
	s_res_use_pixel_scaling = p_use_scaling;
	
	// IM-2014-01-30: [[ HiDPI ]] Reset pixel scale value
	if (s_res_use_pixel_scaling)
		s_res_pixel_scale = MCResPlatformGetDefaultPixelScale();
	else
		s_res_pixel_scale = 1.0;
	
	MCResPlatformHandleScaleChange();
}

bool MCResGetUsePixelScaling(void)
{
	return s_res_use_pixel_scaling;
}

////////////////////////////////////////////////////////////////////////////////

void MCResListScreenPixelScales(bool p_plural, uindex_t& r_count, double *&r_list)
{
	const MCDisplay *t_displays;
	t_displays = nil;
	uint32_t t_display_count;
	t_display_count = 0;
	
	t_display_count = MCscreen->getdisplays(t_displays, False);
	uint32_t t_limit;
	t_limit = p_plural ? t_display_count : 1;
	
    MCAutoArray<double> t_list;
    if (t_list.New(t_limit))
        for (uint32_t i = 0; i < t_limit; i++)
            t_list[i] = t_displays[i].pixel_scale;

    t_list . Take(r_list, r_count);
}

////////////////////////////////////////////////////////////////////////////////
