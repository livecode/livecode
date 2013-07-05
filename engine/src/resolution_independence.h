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

//MCGAffineTransform MCResUserToDeviceTransform(void);
//MCGAffineTransform MCResDeviceToUserTransform(void);

inline MCGFloat MCResGetDeviceScale(void) { return 1.25; };

inline MCGRectangle MCGRectangleToUserSpace(const MCGRectangle p_device_rect)
{
	return MCGRectangleScale(p_device_rect, 1 / MCResGetDeviceScale());
}

inline MCGRectangle MCGRectangleToDeviceSpace(const MCGRectangle p_user_rect)
{
	return MCGRectangleScale(p_user_rect, MCResGetDeviceScale());
}

#endif // __RESOLUTION_INDEPENDENCE_H__