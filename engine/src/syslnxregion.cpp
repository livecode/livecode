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

#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"

#include "util.h"
#include "globals.h"
#include "region.h"

#include "lnxdc.h"


bool MCLinuxRegionCreate(GdkRegion* &r_region)
{
	GdkRegion *t_rgn;
	t_rgn = gdk_region_new();
	if (t_rgn == nil)
		return false;

	r_region = t_rgn;
	return true;
}

void MCLinuxRegionDestroy(GdkRegion* p_region)
{
	if (p_region == nil)
		return;
    
	gdk_region_destroy((GdkRegion*)p_region);
}

bool MCLinuxRegionIncludeRect(GdkRegion* p_region, const MCGIntegerRectangle& p_rect)
{
	GdkRectangle t_rect;
	t_rect . x = p_rect.origin.x;
	t_rect . y = p_rect.origin.y;
	t_rect . width = p_rect.size.width;
	t_rect . height = p_rect.size.height;
    gdk_region_union_with_rect(p_region, &t_rect);
	return true;
}

static bool MCLinuxMCGRegionToRegionCallback(void *p_context, const MCGIntegerRectangle &p_rect)
{
	return MCLinuxRegionIncludeRect(*static_cast<GdkRegion**>(p_context), p_rect);
}

bool MCLinuxMCGRegionToRegion(MCGRegionRef p_region, GdkRegion* &r_region)
{
	bool t_success;
	t_success = true;
	
	GdkRegion* t_region;
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

bool MCLinuxRegionToMCGRegion(GdkRegion *p_region, MCGRegionRef &r_region)
{
    MCGRegionCreate(r_region);
    
    GdkRectangle *t_rects;
    gint t_nrects;
	gdk_region_get_rectangles(p_region, &t_rects, &t_nrects);
    for (gint i = 0; i < t_nrects; i++)
    {
        MCGIntegerRectangle t_rect;
        t_rect.origin.x = t_rects[i].x;
        t_rect.origin.y = t_rects[i].y;
        t_rect.size.width = t_rects[i].width;
        t_rect.size.height = t_rects[i].height;
        MCGRegionAddRect(r_region, t_rect);
    }
    
    g_free(t_rects);
    return true;
}

#if 0

bool MCRegionIsRect(MCRegionRef p_region)
{
	GdkRectangle t_bounds;
    gdk_region_get_clipbox((GdkRegion*)p_region, &t_bounds);
	return gdk_region_rect_equal((GdkRegion*)p_region, &t_bounds);
}

MCRectangle MCRegionGetBoundingBox(MCRegionRef p_region)
{
	GdkRectangle t_box;
    gdk_region_get_clipbox((GdkRegion*)p_region, &t_box);
	return MCU_make_rect(t_box . x, t_box . y, t_box . width, t_box . height);
}

bool MCRegionSetEmpty(MCRegionRef p_region)
{
    gdk_region_subtract((GdkRegion*)p_region, (GdkRegion*)p_region);
    return true;
}

bool MCRegionSetRect(MCRegionRef p_region, const MCRectangle& p_rect)
{
	GdkRectangle t_rect;
	t_rect . x = p_rect . x;
	t_rect . y = p_rect . y;
	t_rect . width = p_rect . width;
	t_rect . height = p_rect . height;
    MCRegionSetEmpty(p_region);
    gdk_region_union_with_rect((GdkRegion*)p_region, &t_rect);
	return true;
}

bool MCRegionOffset(MCRegionRef p_region, int32_t p_dx, int32_t p_dy)
{
	gdk_region_offset((GdkRegion*)p_region, p_dx, p_dy);
	return true;
}

bool MCRegionIncludeRect(MCRegionRef p_region, const MCRectangle& p_rect)
{
	GdkRectangle t_rect;
	t_rect . x = p_rect . x;
	t_rect . y = p_rect . y;
	t_rect . width = p_rect . width;
	t_rect . height = p_rect . height;
	gdk_region_union_with_rect((GdkRegion*)p_region, &t_rect);
	return true;
}

bool MCRegionUnion(MCRegionRef p_dst, MCRegionRef p_left, MCRegionRef p_right)
{
	p_dst = (MCRegionRef)gdk_region_copy((GdkRegion*)p_left);
    gdk_region_union((GdkRegion*)p_dst, (GdkRegion*)p_right);
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
bool MCRegionForEachRect(MCRegionRef region, MCRegionForEachRectCallback callback, void *context)
{
	if (callback == nil)
		return false;
	
    GdkRectangle *t_rects;
    gint t_nrects;
	gdk_region_get_rectangles((GdkRegion*)region, &t_rects, &t_nrects);
    for (gint i = 0; i < t_nrects; i++)
    {
        MCRectangle t_rect;
        t_rect = MCU_make_rect(t_rects[i].x, t_rects[i].y, t_rects[i].width, t_rects[i].height);
        callback(context, t_rect);
    }
    
    g_free(t_rects);
}

#endif

