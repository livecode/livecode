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

#include "core.h"
#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"

#include "util.h"
#include "globals.h"
#include "region.h"

#include "uidc.h"

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

#ifdef OLD_GRAPHICS
bool MCRegionCalculateMask(MCRegionRef region, int32_t width, int32_t height, MCBitmap*& r_mask)
{
	MCRectangle t_rect;
	t_rect = MCRegionGetBoundingBox(region);
	
	MCBitmap *t_mask;
	t_mask = MCscreen -> createimage(1, width, height, True, 255, False, False);
	
	for(int y = 0; y < MCMin(t_rect . y, height); y++)
		memset(t_mask -> data + t_mask -> bytes_per_line * y, 0, t_mask -> bytes_per_line);

	if (t_rect . x > 0 || t_rect . x + t_rect . width < width)
	{
		for(int y = MCMin(t_rect . y, height); y < MCMin(t_rect . y + t_rect . height, height); y++)
		{
			char *t_row;
			t_row = t_mask -> data + t_mask -> bytes_per_line * y;
			
			for(int x = 0; x < MCMin(t_rect . x, width); x++)
				t_row[x / 8] &= ~(1 << (7 - (x & 7)));
			for(int x = MCMin(t_rect . x + t_rect . width, width); x < width; x++)
				t_row[x / 8] &= ~(1 << (7 - (x & 7)));
		}
	}
		
	for(int y = MCMin(t_rect . y + t_rect . height, height); y < height; y++)
		memset(t_mask -> data + t_mask -> bytes_per_line * y, 0, t_mask -> bytes_per_line);
		
	r_mask = t_mask;

	return true;
}
#endif

typedef bool (*MCRegionForEachRectCallback)(void *context, const MCRectangle& rect);
bool MCRegionForEachRect(MCRegionRef region, MCRegionForEachRectCallback callback, void *context)
{
	return false;
}
