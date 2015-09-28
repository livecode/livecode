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

#include "lnxprefix.h"

#include "core.h"
#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"

#include "util.h"
#include "globals.h"
#include "region.h"

#include "lnxdc.h"


bool MCLinuxRegionCreate(Region &r_region)
{
	Region t_rgn;
	t_rgn = XCreateRegion();
	if (t_rgn == nil)
		return false;

	r_region = t_rgn;
	return true;
}

void MCLinuxRegionDestroy(Region p_region)
{
	if (p_region == nil)
		return;

	XDestroyRegion(p_region);
}

bool MCLinuxRegionIncludeRect(Region p_region, const MCGIntegerRectangle& p_rect)
{
	XRectangle t_rect;
	t_rect . x = p_rect.origin.x;
	t_rect . y = p_rect.origin.y;
	t_rect . width = p_rect.size.width;
	t_rect . height = p_rect.size.height;
	XUnionRectWithRegion(&t_rect, (Region)p_region, (Region)p_region);
	return true;
}

static bool MCLinuxMCGRegionToRegionCallback(void *p_context, const MCGIntegerRectangle &p_rect)
{
	return MCLinuxRegionIncludeRect(*static_cast<Region*>(p_context), p_rect);
}

bool MCLinuxMCGRegionToRegion(MCGRegionRef p_region, Region &r_region)
{
	bool t_success;
	t_success = true;
	
	Region t_region;
	t_region = nil;
	
	t_success = MCLinuxRegionCreate(t_region);
	
	if (t_success)
		t_success = MCGRegionIterate(p_region, MCLinuxMCGRegionToRegionCallback, &t_region);
	
	if (t_success)
	{
		r_region = t_region;
		return true;
	}
	
	MCLinuxRegionDestroy(t_region);
	
	return false;
}

#if 0

bool MCRegionIsRect(MCRegionRef p_region)
{
	XRectangle t_box;
	XClipBox((Region)p_region, &t_box);
	return XRectInRegion((Region)p_region, t_box . x, t_box . y, t_box . width, t_box . height);
}

MCRectangle MCRegionGetBoundingBox(MCRegionRef p_region)
{
	XRectangle t_box;
	XClipBox((Region)p_region, &t_box);
	return MCU_make_rect(t_box . x, t_box . y, t_box . width, t_box . height);
}

bool MCRegionSetEmpty(MCRegionRef p_region)
{
	XSubtractRegion((Region)p_region, (Region)p_region, (Region)p_region);
	return true;
}

bool MCRegionSetRect(MCRegionRef p_region, const MCRectangle& p_rect)
{
	XRectangle t_rect;
	t_rect . x = p_rect . x;
	t_rect . y = p_rect . y;
	t_rect . width = p_rect . width;
	t_rect . height = p_rect . height;
	XSubtractRegion((Region)p_region, (Region)p_region, (Region)p_region);
	XUnionRectWithRegion(&t_rect, (Region)p_region, (Region)p_region);
	return true;
}

bool MCRegionOffset(MCRegionRef p_region, int32_t p_dx, int32_t p_dy)
{
	XOffsetRegion((Region)p_region, p_dx, p_dy);
	return true;
}

bool MCRegionUnion(MCRegionRef p_dst, MCRegionRef p_left, MCRegionRef p_right)
{
	XUnionRegion((Region)p_left, (Region)p_right, (Region)p_dst);
	return true;
}

#ifdef OLD_GRAPHICS
bool MCRegionCalculateMask(MCRegionRef p_region, int32_t p_width, int32_t p_height, MCBitmap*& r_mask)
{
	r_mask = ((MCScreenDC *)MCscreen) -> regiontomask(p_region, p_width, p_height);
	return true;
}
#endif

// IM-2013-10-04: [[ FullscreenMode ]] Implement Linux version of MCRegionForEachRect()
// Note: There is no X11 function for getting the components of a region, so for now
// just use the bounding box
bool MCRegionForEachRect(MCRegionRef region, MCRegionForEachRectCallback callback, void *context)
{
	if (callback == nil)
		return false;
	
	return callback(context, MCRegionGetBoundingBox(region));
}

#endif

