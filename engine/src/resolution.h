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

#ifndef __RESOLUTION_INDEPENDENCE_H__
#define __RESOLUTION_INDEPENDENCE_H__

#include "graphics_util.h"

////////////////////////////////////////////////////////////////////////////////

// Return the scaling factor from stack to device coordinates.
MCGFloat MCResGetDeviceScale(void);

////////////////////////////////////////////////////////////////////////////////

// Utility functions to convert rectangle to/from device coordinates

inline MCGRectangle MCGRectangleToUserSpace(const MCGRectangle p_device_rect)
{
	return MCGRectangleScale(p_device_rect, 1 / MCResGetDeviceScale());
}

inline MCGRectangle MCGRectangleToDeviceSpace(const MCGRectangle p_user_rect)
{
	return MCGRectangleScale(p_user_rect, MCResGetDeviceScale());
}

inline MCGRectangle MCResUserToDeviceRect(MCRectangle p_rect)
{
	return MCGRectangleToDeviceSpace(MCRectangleToMCGRectangle(p_rect));
}
inline MCGRectangle MCResDeviceToUserRect(MCRectangle p_rect)
{
	return MCGRectangleToUserSpace(MCRectangleToMCGRectangle(p_rect));
}

////////////////////////////////////////////////////////////////////////////////

#endif // __RESOLUTION_INDEPENDENCE_H__