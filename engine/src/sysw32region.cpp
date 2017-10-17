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

#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"

#include "util.h"
#include "globals.h"
#include "region.h"

#include "w32dc.h"

#include "graphics_util.h"

////////////////////////////////////////////////////////////////////////////////

extern bool create_temporary_mono_dib(HDC p_dc, uint4 p_width, uint4 p_height, HBITMAP& r_bitmap, void*& r_bits);

////////////////////////////////////////////////////////////////////////////////

bool MCWin32RegionCreate(HRGN &r_region)
{
	HRGN t_rgn;
	t_rgn = CreateRectRgn(0, 0, 0, 0);
	if (t_rgn == nil)
		return false;

	r_region = t_rgn;
	return true;
}

void MCWin32RegionDestroy(HRGN p_region)
{
	if (p_region == nil)
		return;

	DeleteObject(p_region);
}

bool MCWin32RegionOffset(HRGN p_region, int32_t p_dx, int32_t p_dy)
{
	return OffsetRgn(p_region, p_dx, p_dy) != 0;
}

bool MCWin32RegionIncludeRect(HRGN p_region, const MCRectangle& p_rect)
{
	HRGN t_rect_rgn;
	t_rect_rgn = CreateRectRgn(p_rect . x, p_rect . y, p_rect . x + p_rect . width, p_rect . y + p_rect . height);
	if (t_rect_rgn == nil ||
		CombineRgn(p_region, p_region, t_rect_rgn, RGN_OR) == ERROR)
		return false;
	DeleteObject(t_rect_rgn);
	return true;
}

bool MCWin32RegionForEachRect(HRGN p_region, MCRegionForEachRectCallback p_callback, void *p_context)
{
	DWORD t_size;
	t_size = GetRegionData(p_region, 0, NULL);
	
	RGNDATA *t_buffer;
	if (!MCMemoryAllocate(t_size, t_buffer))
		return false;

	GetRegionData(p_region, t_size, t_buffer);

	RGNDATAHEADER *t_header;
	t_header = &t_buffer -> rdh;
	
	bool t_success = true;

	RECT *t_rects;
	t_rects = (RECT *)(t_header + 1);
	for(uint32_t i = 0; t_success && i < t_header -> nCount; i++)
	{
		MCRectangle t_rect;
		t_rect.x = (int2)t_rects[i] . left;
		t_rect.y = (int2)t_rects[i] . top;
		t_rect.width = (uint2)(t_rects[i] . right - t_rects[i] . left);
		t_rect.height = (uint2)(t_rects[i] . bottom - t_rects[i] . top);
		t_success = p_callback(p_context, t_rect);
	}

	MCMemoryDeallocate(t_buffer);

	return t_success;
}

////////////////////////////////////////////////////////////////////////////////

static uint32_t s_rect_count;
static bool HRGNToMCGRegionCallback(void *p_context, const MCRectangle &p_rect)
{
	MCGRegionRef t_region;
	t_region = static_cast<MCGRegionRef>(p_context);

	s_rect_count++;

	return MCGRegionAddRect(t_region, MCRectangleToMCGIntegerRectangle(p_rect));
}

bool MCWin32HRGNToMCGRegion(HRGN p_region, MCGRegionRef &r_region)
{
	MCGRegionRef t_region;
	t_region = nil;

	if (!MCGRegionCreate(t_region))
		return false;

	s_rect_count = 0;
	if (MCWin32RegionForEachRect(p_region, HRGNToMCGRegionCallback, t_region))
	{
		r_region = t_region;
		return true;
	}


	MCGRegionDestroy(t_region);
	return false;
}

static bool MCGRegionToHRGNCallback(void *p_context, const MCGIntegerRectangle &p_rect)
{
	HRGN t_region;
	t_region = static_cast<HRGN>(p_context);

	return MCWin32RegionIncludeRect(t_region, MCRectangleFromMCGIntegerRectangle(p_rect));
}

bool MCWin32MCGRegionToHRGN(MCGRegionRef p_region, HRGN &r_region)
{
	HRGN t_region;
	t_region = nil;

	if (!MCWin32RegionCreate(t_region))
		return false;

	if (MCGRegionIterate(p_region, MCGRegionToHRGNCallback, t_region))
	{
		r_region = t_region;
		return true;
	}

	MCWin32RegionDestroy(t_region);
	return false;
}

////////////////////////////////////////////////////////////////////////////////

bool MCRegionConvertToDeviceAndClip(MCRegionRef p_region, MCSysContextHandle p_dc)
{
	HRGN t_hrgn;
	t_hrgn = nil;
	/* UNCHECKED */ MCWin32MCGRegionToHRGN((MCGRegionRef)p_region, t_hrgn);

	DWORD t_size;
	t_size = GetRegionData(t_hrgn, 0, NULL);
	
	RGNDATA *t_buffer;
	t_buffer = (RGNDATA *)new uint8_t[t_size];
		
	GetRegionData(t_hrgn, t_size, t_buffer);

	LPtoDP((HDC)p_dc, (POINT *)&t_buffer -> rdh . rcBound, (t_buffer -> rdh . nCount + 1) * 2);

	HRGN t_device_rgn;
	t_device_rgn = ExtCreateRegion(NULL, t_size, t_buffer);
	SelectClipRgn((HDC)p_dc, t_device_rgn);
	DeleteObject(t_device_rgn);

	DeleteObject(t_hrgn);

	delete t_buffer;

	return true;
}

bool MCRegionSetAsWindowShape(HRGN p_region, MCSysWindowHandle p_window)
{
	SetWindowRgn((HWND)p_window, p_region, TRUE);
	return true;
}

////////////////////////////////////////////////////////////////////////////////
