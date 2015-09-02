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

#ifndef __RESOLUTION_INDEPENDENCE_H__
#define __RESOLUTION_INDEPENDENCE_H__

#include "graphics_util.h"

////////////////////////////////////////////////////////////////////////////////

// IM-2014-01-27: [[ HiDPI ]] Set pixel scaling to default platform-supported values
void MCResInitPixelScaling(void);

////////////////////////////////////////////////////////////////////////////////

// IM-2013-12-04: [[ PixelScale ]] Set the screen scaling factor
void MCResSetPixelScale(MCGFloat p_scale);
// Return the scaling factor from stack to device coordinates.
MCGFloat MCResGetPixelScale(void);
// IM-2014-03-14: [[ HiDPI ]] Return the scaling factor from stack to platform UI coordinates
MCGFloat MCResGetUIScale(void);

// IM-2014-01-27: [[ HiDPI ]] enable or disable pixel scaling
void MCResSetUsePixelScaling(bool p_use_scaling);
// IM-2014-01-27: [[ HiDPI ]] return the current pixel scale setting
bool MCResGetUsePixelScaling(void);

////////////////////////////////////////////////////////////////////////////////

// IM-2014-01-27: [[ HiDPI ]] Return the pixel scales of the main screen (or all screens)
// as an array
void MCResListScreenPixelScales(bool p_plural, uindex_t& r_count, double *&r_list);

////////////////////////////////////////////////////////////////////////////////

// IM-2014-08-14: [[ Bug 12372 ]] Perform any platform-specific pixel scaling setup.
void MCResPlatformInitPixelScaling(void);
// IM-2014-01-30: [[ HiDPI ]] Return the default pixel scale for the platform
MCGFloat MCResPlatformGetDefaultPixelScale(void);
// IM-2014-03-14: [[ HiDPI ]] Return the scale factor from platform UI coords to device pixels
MCGFloat MCResPlatformGetUIDeviceScale(void);

// IM-2014-01-27: [[ HiDPI ]] Return whether or not pixel scaling is supported by the platform
bool MCResPlatformSupportsPixelScaling(void);
// IM-2014-01-27: [[ HiDPI ]] Return whether or not pixel scaling can be enabled / disabled at runtime
bool MCResPlatformCanChangePixelScaling(void);
// IM-2014-01-30: [[ HiDPI ]] Return whether or not the picel scale value can be modified
bool MCResPlatformCanSetPixelScale(void);

// IM-2014-01-29: [[ HiDPI ]] update system settings after change to pixel scaling
void MCResPlatformHandleScaleChange(void);

////////////////////////////////////////////////////////////////////////////////

#endif // __RESOLUTION_INDEPENDENCE_H__
