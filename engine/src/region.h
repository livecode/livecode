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

#ifndef __MC_REGION__
#define __MC_REGION__

typedef struct __MCRegion *MCRegionRef;

bool MCRegionCreate(MCRegionRef& r_region);
void MCRegionDestroy(MCRegionRef region);

bool MCRegionIsEmpty(MCRegionRef region);
bool MCRegionIsRect(MCRegionRef region);
bool MCRegionIsComplex(MCRegionRef region);

bool MCRegionTouchesRect(MCRegionRef region, const MCRectangle& rect);

MCRectangle MCRegionGetBoundingBox(MCRegionRef region);

bool MCRegionSetEmpty(MCRegionRef region);
bool MCRegionSetRect(MCRegionRef region, const MCRectangle& rect);

bool MCRegionIncludeRect(MCRegionRef region, const MCRectangle& rect);
bool MCRegionExcludeRect(MCRegionRef region, const MCRectangle& rect);

bool MCRegionUnion(MCRegionRef dst, MCRegionRef x, MCRegionRef y);

bool MCRegionOffset(MCRegionRef region, int32_t dx, int32_t dy);

#ifdef OLD_GRAPHICS
bool MCRegionCalculateMask(MCRegionRef region, int32_t width, int32_t height, MCBitmap*& r_mask);
#endif

typedef bool (*MCRegionForEachRectCallback)(void *context, const MCRectangle& rect);
bool MCRegionForEachRect(MCRegionRef region, MCRegionForEachRectCallback callback, void *context);

#ifdef _WINDOWS_DESKTOP
bool MCRegionConvertToDeviceAndClip(MCRegionRef region, MCSysContextHandle dc);
bool MCRegionSetAsWindowShape(MCRegionRef region, MCSysWindowHandle window);
#endif

#ifdef _MAC_DESKTOP
bool MCRegionConvertToCGRects(MCRegionRef region, void*& r_cgrects, uint32_t& r_cgrect_count);
#endif

#endif
