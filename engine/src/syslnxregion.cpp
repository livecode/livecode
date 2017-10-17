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


