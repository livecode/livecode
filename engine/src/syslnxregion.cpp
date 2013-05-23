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

bool MCRegionCreate(MCRegionRef& r_region)
{
	Region t_rgn;
	t_rgn = XCreateRegion();
	if (t_rgn == nil)
		return false;

	r_region = (MCRegionRef)t_rgn;
	return true;
}

void MCRegionDestroy(MCRegionRef p_region)
{
	if (p_region == nil)
		return;

	XDestroyRegion((Region)p_region);
}

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

bool MCRegionIncludeRect(MCRegionRef p_region, const MCRectangle& p_rect)
{
	XRectangle t_rect;
	t_rect . x = p_rect . x;
	t_rect . y = p_rect . y;
	t_rect . width = p_rect . width;
	t_rect . height = p_rect . height;
	XUnionRectWithRegion(&t_rect, (Region)p_region, (Region)p_region);
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