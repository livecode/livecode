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

#ifndef __GRAPHICS_UTIL_H_
#define __GRAPHICS_UTIL_H_

#if defined(_WINDOWS) || defined(_WINDOWS_SERVER)
inline double roundf(float x)
{
	return floorf(x + 0.5f);
}
#endif

////////////////////////////////////////////////////////////////////////////////

inline MCRectangle MCRectangleMake(int16_t x, int16_t y, uint16_t width, uint16_t height)
{
	MCRectangle t_rect;
	t_rect.x = x;
	t_rect.y = y;
	t_rect.width = width;
	t_rect.height = height;
	
	return t_rect;
}

inline MCRectangle MCRectangleOffset(const MCRectangle &p_rect, int32_t p_dx, int32_t p_dy)
{
	return MCRectangleMake(p_rect.x + p_dx, p_rect.y + p_dy, p_rect.width, p_rect.height);
}

////////////////////////////////////////////////////////////////////////////////
// MCRectangle32 Utility Functions

inline void MCRectangle32Set(MCRectangle32 &x_rect, int32_t x, int32_t y, int32_t width, int32_t height)
{
	x_rect.x = x;
	x_rect.y = y;
	x_rect.width = width;
	x_rect.height = height;
}

inline MCRectangle32 MCRectangle32Make(int32_t x, int32_t y, int32_t width, int32_t height)
{
	MCRectangle32 t_rect;
	MCRectangle32Set(t_rect, x, y, width, height);
	
	return t_rect;
}

inline MCRectangle32 MCRectangle32Offset(const MCRectangle32 &p_rect, int32_t x_off, int32_t y_off)
{
	return MCRectangle32Make(p_rect.x + x_off, p_rect.y + y_off, p_rect.width, p_rect.height);
}

inline MCRectangle32 MCRectangle32Intersect(const MCRectangle32 &a, const MCRectangle32 &b)
{
	int32_t t_left, t_top, t_right, t_bottom;
	t_left = MCMax(a.x, b.x);
	t_top = MCMax(a.y, b.y);
	t_right = MCMin(a.x + a.width, b.x + b.width);
	t_bottom = MCMin(a.y + a.height, b.y + b.height);
	
	uint32_t t_width, t_height;
	t_width = MCMax(0, t_right - t_left);
	t_height = MCMax(0, t_bottom - t_top);
	
	return MCRectangle32Make(t_left, t_top, t_width, t_height);
}

////////////////////////////////////////////////////////////////////////////////
// Rectangle conversion functions

inline MCRectangle32 MCRectangle32FromMCRectangle(const MCRectangle &p_rect)
{
	return MCRectangle32Make(p_rect.x, p_rect.y, p_rect.width, p_rect.height);
}

inline MCRectangle MCRectangle32ToMCRectangle(const MCRectangle32 &p_rect)
{
	return MCRectangleMake(p_rect.x, p_rect.y, p_rect.width, p_rect.height);
}

inline MCGRectangle MCRectangleToMCGRectangle(const MCRectangle &p_rect)
{
	return MCGRectangleMake(p_rect.x, p_rect.y, p_rect.width, p_rect.height);
}

inline MCGRectangle MCRectangle32ToMCGRectangle(const MCRectangle32 &p_rect)
{
	return MCGRectangleMake(p_rect.x, p_rect.y, p_rect.width, p_rect.height);
}

////////////////////////////////////////////////////////////////////////////////

inline MCRectangle32 MCGRectangleGetInt32Bounds(MCGRectangle p_rect)
{	
	int32_t t_left, t_right, t_top, t_bottom;
	t_left = floor(p_rect.origin.x);
	t_top = floor(p_rect.origin.y);
	t_right = ceil(p_rect.origin.x + p_rect.size.width);
	t_bottom = ceil(p_rect.origin.y + p_rect.size.height);

	int32_t t_width, t_height;
	t_width = t_right - t_left;
	t_height = t_bottom - t_top;
	
	// [[ Bug 11349 ]] Out of bounds content displayed since getting integer
	//   bounds of an empty rect is not empty.
	if (p_rect . size . width == 0.0f || p_rect . size . height == 0.0f)
	{
		t_width = 0;
		t_height = 0;
	}
	
	MCRectangle32 t_rect;
	t_rect = MCRectangle32Make(t_left, t_top, t_width, t_height);
	
	return t_rect;
}

inline MCRectangle MCGRectangleGetIntegerBounds(MCGRectangle p_rect)
{
	return MCRectangle32ToMCRectangle(MCGRectangleGetInt32Bounds(p_rect));
}

inline MCRectangle MCGRectangleGetIntegerInterior(MCGRectangle p_rect)
{
	int32_t t_left, t_right, t_top, t_bottom;
	t_left = ceil(p_rect.origin.x);
	t_top = ceil(p_rect.origin.y);
	t_right = floor(p_rect.origin.x + p_rect.size.width);
	t_bottom = floor(p_rect.origin.y + p_rect.size.height);
	
	MCRectangle t_rect;
	t_rect = MCRectangleMake(t_left, t_top, t_right - t_left, t_bottom - t_top);
	
	return t_rect;
}

inline MCRectangle MCGRectangleGetIntegerRect(const MCGRectangle &p_rect)
{
	int32_t t_left, t_right, t_top, t_bottom;
	t_left = roundf(p_rect.origin.x);
	t_top = roundf(p_rect.origin.y);
	t_right = roundf(p_rect.origin.x + p_rect.size.width);
	t_bottom = roundf(p_rect.origin.y + p_rect.size.height);
	
	return MCRectangleMake(t_left, t_top, t_right - t_left, t_bottom - t_top);
}

////////////////////////////////////////////////////////////////////////////////

static inline MCPoint MCPointMake(int16_t x, int16_t y)
{
	MCPoint t_point;
	t_point.x = x;
	t_point.y = y;

	return t_point;
}

static inline MCPoint MCGPointToMCPoint(const MCGPoint &p_point)
{
	return MCPointMake(p_point.x, p_point.y);
}

inline MCGPoint MCPointToMCGPoint(MCPoint p_point, MCGFloat p_adjustment = 0.0f)
{
	MCGPoint t_point;
	t_point . x = (MCGFloat) p_point . x + p_adjustment;
	t_point . y = (MCGFloat) p_point . y + p_adjustment;
	return t_point;
}

////////////////////////////////////////////////////////////////////////////////

inline MCRectangle32 MCRectangle32GetTransformedBounds(const MCRectangle32 &p_rect, const MCGAffineTransform &p_transform)
{
	return MCGRectangleGetInt32Bounds(MCGRectangleApplyAffineTransform(MCRectangle32ToMCGRectangle(p_rect), p_transform));
}

inline MCRectangle32 MCRectangle32GetTransformedBounds(const MCRectangle &p_rect, const MCGAffineTransform &p_transform)
{
	return MCRectangle32GetTransformedBounds(MCRectangle32FromMCRectangle(p_rect), p_transform);
}

inline MCRectangle MCRectangleGetTransformedBounds(const MCRectangle &p_rect, const MCGAffineTransform &p_transform)
{
	return MCGRectangleGetIntegerBounds(MCGRectangleApplyAffineTransform(MCRectangleToMCGRectangle(p_rect), p_transform));
}

inline MCRectangle MCRectangleGetTransformedBounds(const MCRectangle32 &p_rect, const MCGAffineTransform &p_transform)
{
	return MCGRectangleGetIntegerBounds(MCGRectangleApplyAffineTransform(MCRectangle32ToMCGRectangle(p_rect), p_transform));
}

inline MCRectangle MCRectangleGetTransformedInterior(const MCRectangle &p_rect, const MCGAffineTransform &p_transform)
{
	return MCGRectangleGetIntegerInterior(MCGRectangleApplyAffineTransform(MCRectangleToMCGRectangle(p_rect), p_transform));
}

inline MCRectangle MCRectangleGetTransformedRect(const MCRectangle &p_rect, const MCGAffineTransform &p_transform)
{
	return MCGRectangleGetIntegerRect(MCGRectangleApplyAffineTransform(MCRectangleToMCGRectangle(p_rect), p_transform));
}

static inline MCRectangle MCRectangleGetScaledBounds(const MCRectangle &p_rect, MCGFloat p_scale)
{
	return MCRectangleGetTransformedBounds(p_rect, MCGAffineTransformMakeScale(p_scale, p_scale));
}

static inline MCRectangle MCRectangleGetScaledInterior(const MCRectangle &p_rect, MCGFloat p_scale)
{
	return MCRectangleGetTransformedInterior(p_rect, MCGAffineTransformMakeScale(p_scale, p_scale));
}

inline MCPoint MCPointTransform(const MCPoint &p_point, const MCGAffineTransform &p_transform)
{
	return MCGPointToMCPoint(MCGPointApplyAffineTransform(MCPointToMCGPoint(p_point), p_transform));
}

////////////////////////////////////////////////////////////////////////////////

inline MCGFloat MCGAffineTransformGetEffectiveScale(const MCGAffineTransform &p_transform)
{
	return MCMax(MCAbs(p_transform.a), MCAbs(p_transform.d));
}

////////////////////////////////////////////////////////////////////////////////

inline MCGPoint MCGRectangleGetCenter(const MCGRectangle &p_rect)
{
	return MCGPointMake(p_rect.origin.x + p_rect.size.width / 2.0, p_rect.origin.y + p_rect.size.height / 2.0);
}

inline MCGRectangle MCGRectangleCenterOnPoint(const MCGRectangle &p_rect, const MCGPoint &p_point)
{
	return MCGRectangleMake(p_point.x - p_rect.size.width / 2.0, p_point.y - p_rect.size.height / 2.0, p_rect.size.width, p_rect.size.height);
}

inline MCGRectangle MCGRectangleCenterOnRect(const MCGRectangle &p_rect_a, const MCGRectangle &p_rect_b)
{
	return MCGRectangleCenterOnPoint(p_rect_a, MCGRectangleGetCenter(p_rect_b));
}

////////////////////////////////////////////////////////////////////////////////

#if defined(TARGET_SUBPLATFORM_ANDROID)

#include "mblandroidtypeface.h"

static inline MCGFont MCFontStructToMCGFont(MCFontStruct *p_font)
{
	MCAndroidFont *t_android_font;
	t_android_font = (MCAndroidFont*)p_font-> fid;
	
	MCGFont t_font;
	t_font . size = t_android_font -> size;
	t_font . ascent = p_font -> ascent;
	t_font . descent = p_font -> descent;
	t_font . fid = t_android_font -> typeface;
	t_font . style = 0;
	t_font . ideal = false;
	return t_font;
}

#elif defined(_MAC_DESKTOP) || defined(_MAC_SERVER)

static inline MCGFont MCFontStructToMCGFont(MCFontStruct *p_font)
{
	MCGFont t_font;
	t_font . size = p_font -> size;
	t_font . ascent = p_font -> ascent;
	t_font . descent = p_font -> descent;
	t_font . style = p_font -> style;
	t_font . fid = p_font -> fid;
	t_font . ideal = false;
	return t_font;
}

#elif defined(_LINUX_DESKTOP) || defined(_LINUX_SERVER)

#include "lnxflst.h"

static inline MCGFont MCFontStructToMCGFont(MCFontStruct *p_font)
{
	MCGFont t_font;
	if (p_font == nil)
	{
		MCMemoryClear(&t_font, sizeof(t_font));
		return t_font;
	}
	
	t_font . size = p_font -> size;
	t_font . ascent = p_font -> ascent;
	t_font . descent = p_font -> descent;
	t_font . fid = static_cast<MCNewFontStruct *>(p_font) -> description;
	t_font . style = 0;
	t_font . ideal = false;
	return t_font;
}

#elif defined(_WINDOWS_DESKTOP) || defined(_WINDOWS_SERVER)

static inline MCGFont MCFontStructToMCGFont(MCFontStruct *p_font)
{
	MCGFont t_font;
	t_font . size = p_font -> size;
	t_font . ascent = p_font -> ascent;
	t_font . descent = p_font -> descent;
	t_font . fid = p_font -> fid;
	t_font . style = 0;
	t_font . ideal = p_font -> printer == True;
	return t_font;
}

#else

static inline MCGFont MCFontStructToMCGFont(MCFontStruct *p_font)
{
	MCGFont t_font;
	t_font . size = p_font -> size;
	t_font . ascent = p_font -> ascent;
	t_font . descent = p_font -> descent;
	t_font . fid = p_font -> fid;
	t_font . style = 0;
	t_font . ideal = false;
	return t_font;
}

#endif

#endif // __GRAPHICS_UTIL_H_