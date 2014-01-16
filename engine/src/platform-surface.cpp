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

bool MCPlatformSurfaceLockGraphics(MCPlatformSurfaceRef p_surface, MCRegionRef p_region, MCGContextRef& r_context)
{
	return p_surface -> LockGraphics(p_region, r_context);
}

void MCPlatformSurfaceUnlockGraphics(MCPlatformSurfaceRef p_surface)
{
	p_surface -> UnlockGraphics();
}

bool MCPlatformSurfaceLockPixels(MCPlatformSurfaceRef p_surface, MCRegionRef p_region, MCGRaster& r_raster)
{
	return p_surface -> LockPixels(p_region, r_raster);
}

void MCPlatformSurfaceUnlockPixels(MCPlatformSurfaceRef p_surface)
{
	p_surface -> UnlockPixels();
}

bool MCPlatformSurfaceLockSystemContext(MCPlatformSurfaceRef p_surface, void*& r_context)
{
	p_surface -> LockSystemContext(r_context);
}

void MCPlatformSurfaceUnlockSystemContext(MCPlatformSurfaceRef p_surface)
{
	p_surface -> UnlockSystemContext();
}

bool MCPlatformSurfaceComposite(MCPlatformSurfaceRef p_surface, MCGRectangle p_dst_rect, MCGImageRef p_src_image, MCGRectangle p_src_rect, MCGFloat p_opacity, MCGBlendMode p_blend_mode)
{
	return p_surface -> Composite(p_dst_rect, p_src_image, p_src_rect, p_opacity, p_blend_mode);
}

////////////////////////////////////////////////////////////////////////////////
