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

#include "osxprefix.h"

#include "core.h"
#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"

#include "util.h"
#include "globals.h"
#include "region.h"
#include "uidc.h"

bool MCRegionCreate(MCRegionRef& r_region)
{
	RgnHandle t_region;
	t_region = NewRgn();
	if (t_region == nil)
		return false;
	r_region = (MCRegionRef)t_region;
	return true;
}

void MCRegionDestroy(MCRegionRef self)
{
	if (self == nil)
		return;
	DisposeRgn((RgnHandle)self);
}

bool MCRegionIsEmpty(MCRegionRef self)
{
	return EmptyRgn((RgnHandle)self);
}

bool MCRegionIsRect(MCRegionRef self)
{
	return IsRegionRectangular((RgnHandle)self);
}

bool MCRegionIsComplex(MCRegionRef self)
{
	return !MCRegionIsEmpty(self) && !MCRegionIsRect(self);
}

bool MCRegionTouchesRect(MCRegionRef self, const MCRectangle& rect)
{
	Rect t_rect;
	SetRect(&t_rect, rect . x, rect . y, rect . x + rect . width, rect . y + rect . height);
	return RectInRgn(&t_rect, (RgnHandle)self);
}

MCRectangle MCRegionGetBoundingBox(MCRegionRef self)
{
	Rect t_rect;
	GetRegionBounds((RgnHandle)self, &t_rect);
	return MCU_make_rect(t_rect . left, t_rect . top, t_rect . right - t_rect . left, t_rect . bottom - t_rect . top);
}

bool MCRegionSetEmpty(MCRegionRef self)
{
	SetEmptyRgn((RgnHandle)self);
	return true;
}

bool MCRegionSetRect(MCRegionRef self, const MCRectangle& rect)
{
	SetRectRgn((RgnHandle)self, rect . x, rect . y, rect . x + rect . width, rect . y + rect . height);
	return true;
}

bool MCRegionIncludeRect(MCRegionRef self, const MCRectangle& rect)
{
	Rect t_rect;
	SetRect(&t_rect, rect . x, rect . y, rect . x + rect . width, rect . y + rect . height);
	RgnHandle t_rect_rgn;
	t_rect_rgn = NewRgn();
	RectRgn(t_rect_rgn, &t_rect);
	UnionRgn((RgnHandle)self, t_rect_rgn, (RgnHandle)self);
	DisposeRgn(t_rect_rgn);
	return true;
}

bool MCRegionExcludeRect(MCRegionRef self, const MCRectangle& rect)
{
	Rect t_rect;
	SetRect(&t_rect, rect . x, rect . y, rect . x + rect . width, rect . y + rect . height);
	RgnHandle t_rect_rgn;
	t_rect_rgn = NewRgn();
	RectRgn(t_rect_rgn, &t_rect);
	DiffRgn((RgnHandle)self, t_rect_rgn, (RgnHandle)self);
	DisposeRgn(t_rect_rgn);
	return true;
}

bool MCRegionOffset(MCRegionRef self, int32_t p_dx, int32_t p_dy)
{
	OffsetRgn((RgnHandle)self, p_dx, p_dy);
	return true;
}

#ifdef OLD_GRAPHICS
bool MCRegionCalculateMask(MCRegionRef self, int32_t w, int32_t h, MCBitmap*& r_mask)
{
	// Create a pixmap
	Pixmap t_image;
	t_image = MCscreen -> createpixmap(w, h, 1, False);
	
	// Draw into the pixmap's port
	CGrafPtr t_old_port;
	GDHandle t_old_device;
	GetGWorld(&t_old_port, &t_old_device);
	SetGWorld((CGrafPtr)t_image -> handle . pixmap, NULL);
	
	BackColor(whiteColor);
	ForeColor(blackColor);
	
	Rect t_rect;
	SetRect(&t_rect, 0, 0, w, h);
	EraseRect(&t_rect);
	
	PaintRgn((RgnHandle)self);
	
	SetGWorld(t_old_port, t_old_device);
	
	// Fetch the pixmap as a bitmap
	MCBitmap *t_bitmap;
	t_bitmap = MCscreen -> getimage(t_image, 0, 0, w, h, False);
	
	// Discard the pixmap
	MCscreen -> freepixmap(t_image);
	
	r_mask = t_bitmap;
	
	return true;
}
#endif

struct MCRegionConvertToCGRectsState
{
	CGRect *rects;
	uint32_t count;
};

static OSStatus MCRegionConvertToCGRectsCallback(UInt16 p_message, RgnHandle p_region, const Rect *p_rect, void *p_state)
{
	MCRegionConvertToCGRectsState *state;
	state = (MCRegionConvertToCGRectsState *)p_state;

	if (p_message != kQDRegionToRectsMsgParse)
		return noErr;
	
	if (!MCMemoryResizeArray(state -> count + 1, state -> rects, state -> count))
		return memFullErr;
	
	state -> rects[state -> count - 1] . origin . x = p_rect -> left;
	state -> rects[state -> count - 1] . origin . y = p_rect -> top;
	state -> rects[state -> count - 1] . size . width = p_rect -> right - p_rect -> left;
	state -> rects[state -> count - 1] . size . height = p_rect -> bottom - p_rect -> top;
	
	return noErr;
}

bool MCRegionConvertToCGRects(MCRegionRef self, void*& r_cgrects, uint32_t& r_cgrect_count)
{
	MCRegionConvertToCGRectsState t_state;
	t_state . rects = nil;
	t_state . count = 0;
	
	if (QDRegionToRects((RgnHandle)self, kQDParseRegionFromTopLeft, MCRegionConvertToCGRectsCallback, &t_state) != noErr)
	{
		MCMemoryDeleteArray(t_state . rects);
		return false;
	}
	
	r_cgrects = t_state . rects;
	r_cgrect_count = t_state . count;
	
	return true;
}

typedef bool (*MCRegionForEachRectCallback)(void *context, const MCRectangle& rect);
bool MCRegionForEachRect(MCRegionRef region, MCRegionForEachRectCallback callback, void *context)
{
	return false;
}
