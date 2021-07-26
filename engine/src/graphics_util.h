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

#ifndef __GRAPHICS_UTIL_H_
#define __GRAPHICS_UTIL_H_

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

inline MCRectangle MCRectangleOffset(const MCRectangle &p_rect, int16_t p_dx, int16_t p_dy)
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
	
	int32_t t_width, t_height;
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
	return MCRectangleMake(int16_t(MCClamp(p_rect.x, INT16_MIN, INT16_MAX)),
                           int16_t(MCClamp(p_rect.y, INT16_MIN, INT16_MAX)),
                           uint16_t(MCClamp(p_rect.width, 0, UINT16_MAX)),
                           uint16_t(MCClamp(p_rect.height, 0, UINT16_MAX)));
}

inline MCGRectangle MCRectangleToMCGRectangle(const MCRectangle &p_rect)
{
	return MCGRectangleMake(p_rect.x, p_rect.y, p_rect.width, p_rect.height);
}

inline MCGRectangle MCRectangle32ToMCGRectangle(const MCRectangle32 &p_rect)
{
	/* Possible loss of precision */
	return MCGRectangleMake(MCGFloat(p_rect.x),
	                        MCGFloat(p_rect.y),
	                        MCGFloat(p_rect.width),
	                        MCGFloat(p_rect.height));
}

inline MCRectangle32 MCRectangle32FromMCGIntegerRectangle(const MCGIntegerRectangle &p_rect)
{
	return MCRectangle32Make(p_rect.origin.x, p_rect.origin.y,
	                         int32_t(MCMin(p_rect.size.width, uint32_t(INT32_MAX))),
	                         int32_t(MCMin(p_rect.size.height, uint32_t(INT32_MAX))));
}

inline MCGIntegerRectangle MCRectangle32ToMCGIntegerRectangle(const MCRectangle32 &p_rect)
{
	return MCGIntegerRectangleMake(p_rect.x, p_rect.y,
                                   uint32_t(MCMax(p_rect.width, 0)),
                                   uint32_t(MCMax(p_rect.height, 0)));
}

////////////////////////////////////////////////////////////////////////////////

inline MCGIntegerRectangle MCRectangleToMCGIntegerRectangle(const MCRectangle &p_rect)
{
	return MCGIntegerRectangleMake(p_rect.x, p_rect.y, p_rect.width, p_rect.height);
}

inline MCRectangle MCRectangleFromMCGIntegerRectangle(const MCGIntegerRectangle &p_rect)
{
	return MCRectangleMake(int16_t(MCClamp(p_rect.origin.x,     INT16_MIN, INT16_MAX)),
                           int16_t(MCClamp(p_rect.origin.y,     INT16_MIN, INT16_MAX)),
	                       uint16_t(MCClamp(p_rect.size.width,  0U, uint32_t(UINT16_MAX))),
	                       uint16_t(MCClamp(p_rect.size.height, 0U, uint32_t(UINT16_MAX))));
}

////////////////////////////////////////////////////////////////////////////////

inline MCRectangle32 MCGRectangleGetInt32Bounds(const MCGRectangle &p_rect)
{
	return MCRectangle32FromMCGIntegerRectangle(MCGRectangleGetBounds(p_rect));
}

inline MCRectangle MCGRectangleGetIntegerBounds(MCGRectangle p_rect)
{
	return MCRectangleFromMCGIntegerRectangle(MCGRectangleGetBounds(p_rect));
}

/* There are a number of "correct" ways to convert a rectangle on the
 * real plane to a rectangle on the integer plane, and this template
 * function enables them to be performed correctly.
 *
 * The UpperAdjust() function is used to move upper left coordinates
 * onto the integer grid, and the LowerAdjust() function is used to
 * move lower right coordinates onto the integer grid.
 *
 * If AdjustPerimeter is true, then LowerAdjust is applied to the
 * right and bottom coordinates; otherwise, LowerAdjust is applied to
 * the width and height.
 *
 * Finally, the resulting rectangle is clamped to the extents of the
 * target integer plane.
 */
template<MCGFloat (UpperAdjust)(MCGFloat),
         MCGFloat (LowerAdjust)(MCGFloat),
         bool AdjustPerimeter>
inline MCRectangle
MCGRectangleGetIntegerRect(const MCGRectangle &p_rect)
{
	MCGFloat t_left, t_top, t_width, t_height;
	t_left = UpperAdjust(p_rect.origin.x);
	t_top = UpperAdjust(p_rect.origin.y);

	if (AdjustPerimeter)
	{
		t_width = LowerAdjust(p_rect.origin.x + p_rect.size.width) - t_left;
		t_height = LowerAdjust(p_rect.origin.y + p_rect.size.height) - t_top;
	}
	else
	{
		t_width = LowerAdjust(p_rect.size.width);
		t_height = LowerAdjust(p_rect.size.height);
	}

	return MCRectangleMake( int16_t(MCClamp(t_left,   INT16_MIN,  INT16_MAX)),
	                        int16_t(MCClamp(t_top,    INT16_MIN,  INT16_MAX)),
	                       uint16_t(MCClamp(t_width,  0,         UINT16_MAX)),
	                       uint16_t(MCClamp(t_height, 0,         UINT16_MAX)));
}

inline MCRectangle MCGRectangleGetIntegerInterior(MCGRectangle p_rect)
{
	return MCGRectangleGetIntegerRect<ceilf, floorf, true>(p_rect);
}

inline MCRectangle MCGRectangleGetIntegerExterior(const MCGRectangle& p_rect)
{
	return MCGRectangleGetIntegerRect<floorf, ceilf, true>(p_rect);
}

inline MCRectangle MCGRectangleGetIntegerRect(const MCGRectangle &p_rect)
{
	return MCGRectangleGetIntegerRect<roundf, roundf, true>(p_rect);
}

inline MCRectangle MCGRectangleGetIntegerFloorRect(const MCGRectangle &p_rect)
{
	return MCGRectangleGetIntegerRect<floorf, floorf, false>(p_rect);
}

inline MCRectangle MCGRectangleGetIntegerCeilingRect(const MCGRectangle &p_rect)
{
	return MCGRectangleGetIntegerRect<ceilf, ceilf, false>(p_rect);
}

////////////////////////////////////////////////////////////////////////////////

static inline MCPoint MCPointMake(int16_t x, int16_t y)
{
	MCPoint t_point;
	t_point.x = x;
	t_point.y = y;

	return t_point;
}

static inline bool MCPointIsEqual(const MCPoint &a, const MCPoint &b)
{
	return a.x == b.x && a.y == b.y;
}

static inline MCPoint MCGPointToMCPoint(const MCGPoint &p_point)
{
	return MCPointMake(int16_t(MCClamp(p_point.x, INT16_MIN, INT16_MAX)),
	                   int16_t(MCClamp(p_point.y, INT16_MIN, INT16_MAX)));
}

inline MCGPoint MCPointToMCGPoint(MCPoint p_point, MCGFloat p_adjustment = 0.0f)
{
	MCGPoint t_point;
	t_point . x = (MCGFloat) p_point . x + p_adjustment;
	t_point . y = (MCGFloat) p_point . y + p_adjustment;
	return t_point;
}

static inline MCPoint MCPointOffset(const MCPoint &p_point, int16_t p_x, int16_t p_y)
{
	return MCPointMake(MCClamp(p_point.x + p_x, INT16_MIN, INT16_MAX), MCClamp(p_point.y + p_y, INT16_MIN, INT16_MAX));
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

inline MCRectangle MCRectangleGetTransformedFloorRect(const MCRectangle &p_rect, const MCGAffineTransform &p_transform)
{
	return MCGRectangleGetIntegerFloorRect(MCGRectangleApplyAffineTransform(MCRectangleToMCGRectangle(p_rect), p_transform));
}

inline MCRectangle MCRectangleGetTransformedCeilingRect(const MCRectangle &p_rect, const MCGAffineTransform &p_transform)
{
	return MCGRectangleGetIntegerCeilingRect(MCGRectangleApplyAffineTransform(MCRectangleToMCGRectangle(p_rect), p_transform));
}

static inline MCRectangle MCRectangleGetScaledBounds(const MCRectangle &p_rect, MCGFloat p_scale)
{
	return MCRectangleGetTransformedBounds(p_rect, MCGAffineTransformMakeScale(p_scale, p_scale));
}

static inline MCRectangle MCRectangleGetScaledInterior(const MCRectangle &p_rect, MCGFloat p_scale)
{
	return MCRectangleGetTransformedInterior(p_rect, MCGAffineTransformMakeScale(p_scale, p_scale));
}

static inline MCRectangle MCRectangleGetScaledFloorRect(const MCRectangle &p_rect, MCGFloat p_scale)
{
	return MCRectangleGetTransformedFloorRect(p_rect, MCGAffineTransformMakeScale(p_scale, p_scale));
}

static inline MCRectangle MCRectangleGetScaledCeilingRect(const MCRectangle &p_rect, MCGFloat p_scale)
{
	return MCRectangleGetTransformedCeilingRect(p_rect, MCGAffineTransformMakeScale(p_scale, p_scale));
}

inline MCPoint MCPointTransform(const MCPoint &p_point, const MCGAffineTransform &p_transform)
{
	return MCGPointToMCPoint(MCGPointApplyAffineTransform(MCPointToMCGPoint(p_point), p_transform));
}

////////////////////////////////////////////////////////////////////////////////

inline MCGFloat MCGAffineTransformGetEffectiveScale(const MCGAffineTransform &p_transform)
{
	return MCMax(MCAbs(p_transform.a) + MCAbs(p_transform.c), MCAbs(p_transform.d) + MCAbs(p_transform.b));
}

////////////////////////////////////////////////////////////////////////////////

inline MCGPoint MCGRectangleGetCenter(const MCGRectangle &p_rect)
{
	return MCGPointMake(p_rect.origin.x + p_rect.size.width / 2.0f, p_rect.origin.y + p_rect.size.height / 2.0f);
}

inline MCGRectangle MCGRectangleCenterOnPoint(const MCGRectangle &p_rect, const MCGPoint &p_point)
{
	return MCGRectangleMake(p_point.x - p_rect.size.width / 2.0f, p_point.y - p_rect.size.height / 2.0f, p_rect.size.width, p_rect.size.height);
}

inline MCGRectangle MCGRectangleCenterOnRect(const MCGRectangle &p_rect_a, const MCGRectangle &p_rect_b)
{
	return MCGRectangleCenterOnPoint(p_rect_a, MCGRectangleGetCenter(p_rect_b));
}

////////////////////////////////////////////////////////////////////////////////
// MM-2014-06-02: [[ CoreText ]] We now no longer need the style attribute of the MCGFont Struct.
//   Was only used by the ATSUI routines.

#if defined(TARGET_SUBPLATFORM_ANDROID)

#include "mblandroidtypeface.h"

static inline MCGFont MCFontStructToMCGFont(MCFontStruct *p_font)
{
	MCAndroidFont *t_android_font;
	t_android_font = (MCAndroidFont*)p_font-> fid;
	
	MCGFont t_font;
	t_font . size = t_android_font -> size;
	t_font . m_ascent = p_font -> m_ascent;
	t_font . m_descent = p_font -> m_descent;
    t_font . m_leading = p_font -> m_leading;
	t_font . fid = t_android_font -> typeface;
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
	t_font . m_ascent = p_font -> m_ascent;
	t_font . m_descent = p_font -> m_descent;
    t_font . m_leading = p_font -> m_leading;
	t_font . fid = static_cast<MCNewFontStruct *>(p_font) -> description;
	t_font . ideal = false;
	return t_font;
}

#elif defined(_WINDOWS_DESKTOP) || defined(_WINDOWS_SERVER)

static inline MCGFont MCFontStructToMCGFont(MCFontStruct *p_font)
{
	MCGFont t_font;
	if (p_font == nil)
	{
		MCMemoryClear(&t_font, sizeof(t_font));
		return t_font;
	}

	t_font . size = p_font -> size;
	t_font . m_ascent = p_font -> m_ascent;
	t_font . m_descent = p_font -> m_descent;
    t_font . m_leading = p_font -> m_leading;
	t_font . fid = p_font -> fid;
	t_font . ideal = p_font -> printer == True;
	return t_font;
}

#elif defined(_MAC_DESKTOP) || defined(_MAC_SERVER) || defined(TARGET_SUBPLATFORM_IPHONE)

static inline MCGFont MCFontStructToMCGFont(MCFontStruct *p_font)
{
	MCGFont t_font;
	t_font . size = p_font -> size;
	t_font . m_ascent = p_font -> m_ascent;
	t_font . m_descent = p_font -> m_descent;
    t_font . m_leading = p_font -> m_leading;
	t_font . fid = p_font -> fid;
	t_font . ideal = false;
	return t_font;
}

#elif defined(__EMSCRIPTEN__)

static inline MCGFont
MCFontStructToMCGFont(MCFontStruct *p_font)
{
	MCGFont t_font;
	MCMemoryClear(&t_font, sizeof(t_font));

	t_font . size = p_font -> size;
	t_font . m_ascent = p_font -> m_ascent;
	t_font . m_descent = p_font -> m_descent;
    t_font . m_leading = p_font -> m_leading;
    t_font . fid = p_font -> fid;
	t_font . ideal = false;

	return t_font;
}

#else

#error "Platform doesn't support fonts"

#endif

////////////////////////////////////////////////////////////////////////////////

// IM-2014-10-22: [[ Bug 13746 ]] Raster modifying utility functions
void MCGRasterClearRect(MCGRaster &x_raster, const MCGIntegerRectangle &p_rect);
void MCGRasterApplyAlpha(MCGRaster &x_raster, const MCGRaster &p_alpha, const MCGIntegerPoint &p_offset);

#endif // __GRAPHICS_UTIL_H_
