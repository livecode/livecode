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

#include "platform.h"
#include "platform-internal.h"

////////////////////////////////////////////////////////////////////////////////
//
//  Platform Surface Class Implementation
//

MCPlatformSurface::MCPlatformSurface(void)
{
}

MCPlatformSurface::~MCPlatformSurface(void)
{
}

////////////////////////////////////////////////////////////////////////////////
//
//  Platform Surface Procedural Wrappers
//

// MM-2014-07-31: [[ ThreadedRendering ]] Updated to use new platform surface API.
bool MCPlatformSurfaceLockGraphics(MCPlatformSurfaceRef p_surface, MCGIntegerRectangle p_region, MCGContextRef& r_context, MCGRaster& r_raster)
{
	return p_surface -> LockGraphics(p_region, r_context, r_raster);
}

// MM-2014-07-31: [[ ThreadedRendering ]] Updated to use new platform surface API.
void MCPlatformSurfaceUnlockGraphics(MCPlatformSurfaceRef p_surface, MCGIntegerRectangle p_region, MCGContextRef p_context, MCGRaster& p_raster)
{
	p_surface -> UnlockGraphics(p_region, p_context, p_raster);
}

// MM-2014-07-31: [[ ThreadedRendering ]] Updated to use new platform surface API.
bool MCPlatformSurfaceLockPixels(MCPlatformSurfaceRef p_surface, MCGIntegerRectangle p_region, MCGRaster& r_raster, MCGIntegerRectangle &r_locked_area)
{
	return p_surface -> LockPixels(p_region, r_raster, r_locked_area);
}

// MM-2014-07-31: [[ ThreadedRendering ]] Updated to use new platform surface API.
void MCPlatformSurfaceUnlockPixels(MCPlatformSurfaceRef p_surface, MCGIntegerRectangle p_region, MCGRaster& p_raster)
{
	p_surface -> UnlockPixels(p_region, p_raster);
}

bool MCPlatformSurfaceLockSystemContext(MCPlatformSurfaceRef p_surface, void*& r_context)
{
    // MW-2014-04-18: [[ Bug 12230 ]] Make sure we return the result.
	return p_surface -> LockSystemContext(r_context);
}

void MCPlatformSurfaceUnlockSystemContext(MCPlatformSurfaceRef p_surface)
{
	p_surface -> UnlockSystemContext();
}

bool MCPlatformSurfaceComposite(MCPlatformSurfaceRef p_surface, MCGRectangle p_dst_rect, MCGImageRef p_src_image, MCGRectangle p_src_rect, MCGFloat p_opacity, MCGBlendMode p_blend_mode)
{
	return p_surface -> Composite(p_dst_rect, p_src_image, p_src_rect, p_opacity, p_blend_mode);
}

MCGFloat MCPlatformSurfaceGetBackingScaleFactor(MCPlatformSurfaceRef p_surface)
{
	return p_surface -> GetBackingScaleFactor();
}

////////////////////////////////////////////////////////////////////////////////
