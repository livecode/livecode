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

#include "w32prefix.h"

#include "core.h"
#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"

#include "util.h"
#include "globals.h"
#include "region.h"

#include "w32dc.h"

////////////////////////////////////////////////////////////////////////////////

extern bool create_temporary_mono_dib(HDC p_dc, uint4 p_width, uint4 p_height, HBITMAP& r_bitmap, void*& r_bits);

////////////////////////////////////////////////////////////////////////////////

bool MCRegionCreate(MCRegionRef& r_region)
{
	HRGN t_rgn;
	t_rgn = CreateRectRgn(0, 0, 0, 0);
	if (t_rgn == nil)
		return false;

	r_region = (MCRegionRef)t_rgn;
	return true;
}

void MCRegionDestroy(MCRegionRef p_region)
{
	if (p_region == nil)
		return;

	DeleteObject((HGDIOBJ)p_region);
}

bool MCRegionIsRect(MCRegionRef p_region)
{
	RECT t_box;
	return GetRgnBox((HRGN)p_region, &t_box) == SIMPLEREGION;
}

MCRectangle MCRegionGetBoundingBox(MCRegionRef p_region)
{
	RECT t_box;
	GetRgnBox((HRGN)p_region, &t_box);
	return MCU_make_rect((int2)t_box . left, (int2)t_box . top, (uint2)(t_box . right - t_box . left), (uint2)(t_box . bottom - t_box . top));
}

bool MCRegionSetEmpty(MCRegionRef p_region)
{
	return SetRectRgn((HRGN)p_region, 0, 0, 0, 0) != 0;
}

bool MCRegionSetRect(MCRegionRef p_region, const MCRectangle& p_rect)
{
	return SetRectRgn((HRGN)p_region, p_rect . x, p_rect . y, p_rect . x + p_rect . width, p_rect . y + p_rect . height) != 0;
}

bool MCRegionOffset(MCRegionRef p_region, int32_t p_dx, int32_t p_dy)
{
	return OffsetRgn((HRGN)p_region, p_dx, p_dy) != 0;
}

bool MCRegionIncludeRect(MCRegionRef p_region, const MCRectangle& p_rect)
{
	HRGN t_rect_rgn;
	t_rect_rgn = CreateRectRgn(p_rect . x, p_rect . y, p_rect . x + p_rect . width, p_rect . y + p_rect . height);
	if (t_rect_rgn == nil ||
		CombineRgn((HRGN)p_region, (HRGN)p_region, t_rect_rgn, RGN_OR) == ERROR)
		return false;
	DeleteObject(t_rect_rgn);
	return true;
}

#ifdef OLD_GRAPHICS
bool MCRegionCalculateMask(MCRegionRef p_region, int32_t p_width, int32_t p_height, MCBitmap*& r_mask)
{
	// Our src HDC
	HDC t_src_dc;
	t_src_dc = ((MCScreenDC *)MCscreen) -> getsrchdc();

	// Create a temporary 1-bit image
	HBITMAP t_bitmap;
	void *t_bits;
	if (!create_temporary_mono_dib(t_src_dc, p_width, p_height, t_bitmap, t_bits))
		return false;

	// Select it into a context
	HGDIOBJ t_old_bitmap;
	t_old_bitmap = SelectObject(t_src_dc, t_bitmap);

	// Fill it with black
	SelectObject(t_src_dc, GetStockObject(BLACK_BRUSH));
	SelectObject(t_src_dc, GetStockObject(NULL_PEN));
	Rectangle(t_src_dc, 0, 0, p_width, p_height);

	// Select the clip region, change the color to white and fill again
	SelectClipRgn(t_src_dc, (HRGN)p_region);
	SelectObject(t_src_dc, GetStockObject(WHITE_BRUSH));
	Rectangle(t_src_dc, 0, 0, p_width, p_height);

	// Unselect the bitmap
	SelectObject(t_src_dc, t_old_bitmap);

	// Now create an bitmap and copy the image data into it
	MCBitmap *t_mask;
	t_mask = MCscreen -> createimage(1, p_width, p_height, True, 0, False, False);
	memcpy(t_mask -> data, t_bits, t_mask -> bytes_per_line * p_height);

	// Delete the temporary image and return!
	DeleteObject(t_bitmap);

	r_mask = t_mask;

	return true;
}
#endif

bool MCRegionForEachRect(MCRegionRef p_region, MCRegionForEachRectCallback p_callback, void *p_context)
{
	DWORD t_size;
	t_size = GetRegionData((HRGN)p_region, 0, NULL);
	
	RGNDATA *t_buffer;
	if (!MCMemoryAllocate(t_size, t_buffer))
		return false;

	GetRegionData((HRGN)p_region, t_size, t_buffer);

	RGNDATAHEADER *t_header;
	t_header = &t_buffer -> rdh;
	
	bool t_success = true;

	RECT *t_rects;
	t_rects = (RECT *)(t_header + 1);
	for(uint32_t i = 0; t_success && i < t_header -> nCount; i++)
	{
		MCRectangle t_rect;
		t_rect . x = (int2)t_rects[i] . left;
		t_rect . y = (int2)t_rects[i] . top;
		t_rect . width = (uint2)(t_rects[i] . right - t_rects[i] . left);
		t_rect . height = (uint2)(t_rects[i] . bottom - t_rects[i] . top);
		t_success = p_callback(p_context, t_rect);
	}

	MCMemoryDeallocate(t_buffer);

	return t_success;
}

////////////////////////////////////////////////////////////////////////////////

bool MCRegionConvertToDeviceAndClip(MCRegionRef p_region, MCSysContextHandle p_dc)
{
	DWORD t_size;
	t_size = GetRegionData((HRGN)p_region, 0, NULL);
	
	RGNDATA *t_buffer;
	t_buffer = (RGNDATA *)new uint8_t[t_size];
		
	GetRegionData((HRGN)p_region, t_size, t_buffer);

	LPtoDP((HDC)p_dc, (POINT *)&t_buffer -> rdh . rcBound, (t_buffer -> rdh . nCount + 1) * 2);

	HRGN t_device_rgn;
	t_device_rgn = ExtCreateRegion(NULL, t_size, t_buffer);
	SelectClipRgn((HDC)p_dc, t_device_rgn);
	DeleteObject(t_device_rgn);

	delete t_buffer;

	return true;
}

bool MCRegionSetAsWindowShape(MCRegionRef p_region, MCSysWindowHandle p_window)
{
	SetWindowRgn((HWND)p_window, (HRGN)p_region, TRUE);
	return true;
}

////////////////////////////////////////////////////////////////////////////////
