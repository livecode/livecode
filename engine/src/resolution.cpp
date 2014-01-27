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

#include "prefix.h"

#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"

#include "dispatch.h"

#include "uidc.h"
#include "execpt.h"

#include "graphics.h"
#include "resolution.h"

////////////////////////////////////////////////////////////////////////////////

// IM-2014-01-24: [[ HiDPI ]] Removed unused "use system pixel scale" functions

static MCGFloat s_res_pixel_scale = 1.0;

void MCResHandleScaleChanged()
{
	// Trigger update of stack windows
	MCdispatcher->sync_stack_windows();
}

MCGFloat MCResGetPixelScale(void)
{
	return s_res_pixel_scale;
}

void MCResSetPixelScale(MCGFloat p_scale, bool p_send_update)
{
	MCGFloat t_old_scale;
	t_old_scale = MCResGetPixelScale();
	
	s_res_pixel_scale = p_scale;
	
	if (p_send_update && p_scale != t_old_scale)
		MCResHandleScaleChanged();
}

////////////////////////////////////////////////////////////////////////////////

static bool s_use_pixel_scaling = true;

void MCResInitPixelScaling(void)
{
	// If pixel scaling is available then use it by default
	s_use_pixel_scaling = MCResPlatformSupportsPixelScaling();
}

void MCResSetUsePixelScaling(bool p_use_scaling)
{
	if (p_use_scaling == s_use_pixel_scaling || !MCResPlatformCanChangePixelScaling())
		return;
	
	s_use_pixel_scaling = p_use_scaling;
	
	MCResPlatformSetUsePixelScaling(p_use_scaling);
}

bool MCResGetUsePixelScaling(void)
{
	return s_use_pixel_scaling;
}

////////////////////////////////////////////////////////////////////////////////

void MCResListScreenPixelScales(MCExecPoint &ep, bool p_plural)
{
	const MCDisplay *t_displays;
	t_displays = nil;
	uint32_t t_display_count;
	t_display_count = 0;
	
	t_display_count = MCscreen->getdisplays(t_displays, False);
	uint32_t t_limit;
	t_limit = p_plural ? t_display_count : 1;
	
	ep.clear();
	for (uint32_t i = 0; i < t_limit; i++)
		ep.concatreal(t_displays[i].pixel_scale, EC_RETURN, i == 0);
}

////////////////////////////////////////////////////////////////////////////////
