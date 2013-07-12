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

#endif // __GRAPHICS_UTIL_H_