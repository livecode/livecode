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

#ifndef __MC_REGION__
#define __MC_REGION__

#ifndef __MC_GRAPHICS__
#include "graphics.h"
#endif


bool MCRegionCreate(MCRegionRef& r_region);
void MCRegionDestroy(MCRegionRef region);

bool MCRegionIsEmpty(MCRegionRef region);
bool MCRegionIsRect(MCRegionRef region);
bool MCRegionIsComplex(MCRegionRef region);

MCRectangle MCRegionGetBoundingBox(MCRegionRef region);

bool MCRegionSetEmpty(MCRegionRef region);
bool MCRegionSetRect(MCRegionRef region, const MCRectangle& rect);

bool MCRegionIncludeRect(MCRegionRef region, const MCRectangle& rect);

bool MCRegionAddRegion(MCRegionRef p_region, MCRegionRef p_other);

bool MCRegionOffset(MCRegionRef region, int32_t dx, int32_t dy);

bool MCRegionTransform(MCRegionRef p_region, const MCGAffineTransform &p_transform, MCRegionRef &r_transformed_region);

typedef bool (*MCRegionForEachRectCallback)(void *context, const MCRectangle& rect);
bool MCRegionForEachRect(MCRegionRef region, MCRegionForEachRectCallback callback, void *context);

#ifdef _WINDOWS_DESKTOP
bool MCRegionConvertToDeviceAndClip(MCRegionRef region, MCSysContextHandle dc);
#endif

#ifdef _MAC_DESKTOP
bool MCRegionConvertToCGRects(MCRegionRef region, void*& r_cgrects, uint32_t& r_cgrect_count);
#endif

#endif
