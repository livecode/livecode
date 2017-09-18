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

#include "osxprefix.h"

#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"

#include "util.h"
#include "globals.h"
#include "region.h"
#include "uidc.h"

/*
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

bool MCRegionOffset(MCRegionRef self, int32_t p_dx, int32_t p_dy)
{
	OffsetRgn((RgnHandle)self, p_dx, p_dy);
	return true;
}
*/

struct __MCRegion
{
	MCRectangle rect;
};

bool MCRegionCreate(MCRegionRef& r_region)
{
	return MCMemoryNew(r_region);
}

void MCRegionDestroy(MCRegionRef self)
{
	MCMemoryDelete(self);
}

bool MCRegionIsEmpty(MCRegionRef self)
{
	return MCU_empty_rect(self -> rect);
}

bool MCRegionIsRect(MCRegionRef region)
{
	return true;
}

bool MCRegionIsComplex(MCRegionRef region)
{
	return false;
}

bool MCRegionTouchesRect(MCRegionRef region, const MCRectangle& rect)
{
	return false;
}

MCRectangle MCRegionGetBoundingBox(MCRegionRef self)
{
	return self -> rect;
}

bool MCRegionSetEmpty(MCRegionRef self)
{
	MCU_set_rect(self -> rect, 0, 0, 0, 0);
	return true;
}

bool MCRegionSetRect(MCRegionRef self, const MCRectangle& p_rect)
{
	self -> rect = p_rect;
	return true;
}

bool MCRegionIncludeRect(MCRegionRef self, const MCRectangle& p_rect)
{
	self -> rect = MCU_union_rect(self -> rect, p_rect);
	return true;
}

bool MCRegionExcludeRect(MCRegionRef self, const MCRectangle& p_rect)
{
	return true;
}

bool MCRegionOffset(MCRegionRef self, int32_t p_dx, int32_t p_dy)
{
	self -> rect = MCU_offset_rect(self -> rect, p_dx, p_dy);
	return true;
}


bool MCRegionUnion(MCRegionRef self, MCRegionRef x, MCRegionRef y)
{
    self -> rect = MCU_union_rect(x -> rect, y -> rect);
	return true;
}

static inline CGRect MCRectangleToCGRect(const MCRectangle &p_rect)
{
	return CGRectMake(p_rect.x, p_rect.y, p_rect.width, p_rect.height);
}

struct MCRegionConvertToCGRectsState
{
	CGRect *rects;
	uint32_t count;
};

static bool MCRegionConvertToCGRectsCallback(void *p_state, const MCRectangle &p_rect)
{
	MCRegionConvertToCGRectsState *state;
	state = (MCRegionConvertToCGRectsState *)p_state;

	if (!MCMemoryResizeArray(state -> count + 1, state -> rects, state -> count))
		return false;
	
	state -> rects[state -> count - 1] = MCRectangleToCGRect(p_rect);
	
	return true;
}

bool MCRegionConvertToCGRects(MCRegionRef self, void*& r_cgrects, uint32_t& r_cgrect_count)
{
	MCRegionConvertToCGRectsState t_state;
	t_state . rects = nil;
	t_state . count = 0;
	
	if (!MCRegionForEachRect(self, MCRegionConvertToCGRectsCallback, &t_state))
	{
		MCMemoryDeleteArray(t_state . rects);
		return false;
	}
	
	r_cgrects = t_state . rects;
	r_cgrect_count = t_state . count;
	
	return true;
}

////////////////////////////////////////////////////////////////////////////////

// IM-2013-10-04: [[ FullscreenMode ]] Implement OSX version of MCRegionForEachRect

struct MCRegionForEachRectContext
{
	MCRegionForEachRectCallback callback;
	void *context;
};

static inline MCRectangle MCMacRectToMCRect(const Rect &p_rect)
{
	MCRectangle t_rect;
	t_rect.x = p_rect.left;
	t_rect.y = p_rect.top;
	t_rect.width = p_rect.right - p_rect.left;
	t_rect.height = p_rect.bottom - p_rect.top;
	
	return t_rect;
}

/*
static OSStatus MCRegionForEachRectQDCallback(UInt16 p_message, RgnHandle p_region, const Rect *p_rect, void *p_state)
{
	if (p_message != kQDRegionToRectsMsgParse)
		return noErr;
	
	MCRegionForEachRectContext *t_context;
	t_context = static_cast<MCRegionForEachRectContext*>(p_state);
	
	MCRectangle t_rect;
	t_rect = MCMacRectToMCRect(*p_rect);
	
	if (t_context->callback(t_context->context, t_rect))
		return noErr;
	else
		return abortErr;
}

bool MCRegionForEachRect(MCRegionRef region, MCRegionForEachRectCallback callback, void *context)
{
	MCRegionForEachRectContext t_context;
	t_context.callback = callback;
	t_context.context = context;
	
	return noErr == QDRegionToRects((RgnHandle)region, kQDParseRegionFromTopLeft, MCRegionForEachRectQDCallback, &t_context);
}
*/

typedef bool (*MCRegionForEachRectCallback)(void *context, const MCRectangle& rect);
bool MCRegionForEachRect(MCRegionRef region, MCRegionForEachRectCallback callback, void *context)
{
	// IM-2013-09-30: [[ FullscreenMode ]] Implement for mobile
	
	// region is just a single rect
	return callback(context, region->rect);
}

////////////////////////////////////////////////////////////////////////////////

