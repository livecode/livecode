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

inline MCRectangle MCRectangleMake(int16_t x, int16_t y, uint16_t width, uint16_t height)
{
	MCRectangle t_rect;
	t_rect.x = x;
	t_rect.y = y;
	t_rect.width = width;
	t_rect.height = height;
	
	return t_rect;
}

inline MCGRectangle MCRectangleToMCGRectangle(MCRectangle p_rect)
{
	return MCGRectangleMake(p_rect.x, p_rect.y, p_rect.width, p_rect.height);
}

inline MCRectangle MCGRectangleGetIntegerBounds(MCGRectangle p_rect)
{
	int32_t t_left, t_right, t_top, t_bottom;
	t_left = floor(p_rect.origin.x);
	t_top = floor(p_rect.origin.y);
	t_right = ceil(p_rect.origin.x + p_rect.size.width);
	t_bottom = ceil(p_rect.origin.y + p_rect.size.height);
	
	MCRectangle t_rect;
	t_rect = MCRectangleMake(t_left, t_top, t_right - t_left, t_bottom - t_top);
	
	return t_rect;
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

inline MCGPoint MCPointToMCGPoint(MCPoint p_point)
{
	MCGPoint t_point;
	t_point . x = (MCGFloat) p_point . x;
	t_point . y = (MCGFloat) p_point . y;
	return t_point;
}

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
	return t_font;
}

#elif defined(TARGET_PLATFORM_MACOS_X)

static inline MCGFont MCFontStructToMCGFont(MCFontStruct *p_font)
{
	MCGFont t_font;
	t_font . size = p_font -> size;
	t_font . ascent = p_font -> ascent;
	t_font . descent = p_font -> descent;
	t_font . style = p_font -> style;
	t_font . fid = p_font -> fid;
	return t_font;
}

#elif defined(TARGET_PLATFORM_LINUX)

#include "lnxflst.h"

static inline MCGFont MCFontStructToMCGFont(MCFontStruct *p_font)
{
	MCGFont t_font;
	t_font . size = p_font -> size;
	t_font . ascent = p_font -> ascent;
	t_font . descent = p_font -> descent;
	t_font . fid = static_cast<MCNewFontStruct *>(p_font) -> description;
	t_font . style = 0;
	return t_font;
}

#elif defined(_SERVER)

static inline MCGFont MCFontStructToMCGFont(MCFontStruct *p_font)
{
	MCGFont t_font;
	t_font . size = p_font -> size;
	t_font . ascent = p_font -> ascent;
	t_font . descent = p_font -> descent;
	t_font . fid = nil;
	t_font . style = 0;
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
	return t_font;
}

#endif

#endif // __GRAPHICS_UTIL_H_