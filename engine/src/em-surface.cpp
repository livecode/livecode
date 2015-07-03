/*                                                                     -*-c++-*-

Copyright (C) 2003-2013 Runtime Revolution Ltd.

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

#include "prefix.h"

#include "em-surface.h"
#include "em-util.h"

/* ================================================================
 * Abstract raster surface
 * ================================================================ */

bool
MCAbstractRasterStackSurface::LockGraphics(MCGIntegerRectangle p_area,
                                           MCGContextRef & r_context,
                                           MCGRaster & r_raster)
{
	MCGRaster t_raster;
	MCGIntegerRectangle t_locked_area;

	if (!LockPixels(p_area, t_raster, t_locked_area))
		return false;

	MCGContextRef t_context;

	if (!MCGContextCreateWithRaster(t_raster, t_context))
	{
		UnlockPixels(t_locked_area, t_raster);
		return false;
	}

	/* Set origin */
	MCGContextTranslateCTM(t_context,
	                       -t_locked_area.origin.x,
	                       -t_locked_area.origin.y);

	/* Set clip */
	MCGContextClipToRegion(t_context, GetRegion());
	MCGContextClipToRect(t_context,
	                     MCGIntegerRectangleToMCGRectangle(t_locked_area));

	r_context = t_context;
	r_raster = t_raster;
	return true;
}

void
MCAbstractRasterStackSurface::UnlockGraphics(MCGIntegerRectangle p_area,
                                             MCGContextRef context,
                                             MCGRaster & p_raster)
{
	MCAssert(nil != context);

	MCGContextRelease(context);

	UnlockPixels(p_area, p_raster);
}


bool
MCAbstractRasterStackSurface::LockPixels(MCGIntegerRectangle p_area,
                                         MCGRaster & r_raster,
                                         MCGIntegerRectangle & r_locked_area)
{
	if (!Lock())
	{
		return false;
	}

	MCGIntegerRectangle t_bounds;
	t_bounds = MCGRegionGetBounds(GetRegion());

	MCGIntegerRectangle t_actual_area;
	t_actual_area = MCGIntegerRectangleIntersection(p_area, t_bounds);

	if (MCGIntegerRectangleIsEmpty(t_actual_area))
	{
		return false;
	}

	r_raster.width = t_actual_area.size.width;
	r_raster.height = t_actual_area.size.height;
	r_raster.stride = GetStride();
	r_raster.format = GetFormat();
	r_raster.pixels = GetPixelBuffer(t_actual_area);

	r_locked_area = t_actual_area;

	return true;
}

void
MCAbstractRasterStackSurface::UnlockPixels(MCGIntegerRectangle area,
                                           MCGRaster & raster)
{
	Unlock();
}

bool
MCAbstractRasterStackSurface::LockTarget(MCStackSurfaceTargetType p_type,
                                         void *& r_context)
{
	return false;
}

void
MCAbstractRasterStackSurface::UnlockTarget()
{
}

bool
MCAbstractRasterStackSurface::Lock()
{
	return true;
}

void
MCAbstractRasterStackSurface::Unlock()
{
}

bool
MCAbstractRasterStackSurface::Composite(MCGRectangle p_dest_rect,
                                        MCGImageRef p_source,
                                        MCGRectangle p_src_rect,
                                        MCGFloat p_alpha,
                                        MCGBlendMode p_blend)
{
	bool t_success = true;

	MCGIntegerRectangle t_bounds;
	MCGContextRef t_context = nil;
	MCGRaster t_raster;

	if (t_success)
	{
		t_bounds = MCGRectangleGetBounds(p_dest_rect);
		t_success = LockGraphics(t_bounds, t_context, t_raster);
	}

	if (t_success)
	{
		MCGContextSetBlendMode(t_context, p_blend);
		MCGContextSetOpacity(t_context, p_alpha);

		MCGContextDrawRectOfImage(t_context, p_source,
		                          p_src_rect, p_dest_rect,
		                          kMCGImageFilterNone);
	}

	UnlockGraphics(t_bounds, t_context, t_raster);

	return t_success;
}

/* ================================================================
 * SDL canvas surface
 * ================================================================ */

MCSdlStackSurface::MCSdlStackSurface(uint32_t width, uint32_t height)
    : m_surface(SDL_CreateRGBSurface(SDL_SWSURFACE, width, height, 32, 0x00000FF, 0x0000FF00, 0x00FF0000, 0xFF000000)),
      m_free_surface(true),
	  m_region(nil)
{
	MCAssert(m_surface);
}

MCSdlStackSurface::~MCSdlStackSurface()
{
	/* m_surface is owned by libsdl */
	if (nil != m_surface && m_free_surface)
	{
		SDL_FreeSurface(m_surface);
	}
	if (nil != m_region)
	{
		MCGRegionDestroy(m_region);
	}
}

bool
MCSdlStackSurface::Lock()
{
	MCAssert(nil != m_surface);

	if (SDL_MUSTLOCK(m_surface) &&
	    0 != SDL_LockSurface(m_surface))
	{
		return false;
	}

	return true;
}

void
MCSdlStackSurface::Unlock()
{
	MCAssert(nil != m_surface);

    SDL_Rect t_srcrect = {0, 0, m_surface->w, m_surface->h};
    SDL_Rect t_dstrect = {0, 0, m_surface->w, m_surface->h};

	if (SDL_MUSTLOCK(m_surface))
	{
		SDL_UnlockSurface(m_surface);
	}

    SDL_Surface* t_screen = SDL_GetVideoSurface();
    SDL_BlitSurface(m_surface, &t_srcrect, t_screen, &t_dstrect);
    SDL_Flip(t_screen);
}

MCGRegionRef
MCSdlStackSurface::GetRegion()
{
	if (nil != m_region)
	{
		return m_region;
	}

	MCAssert(nil != m_surface);

	/* Compute the region from the surface */
	MCGRegionCreate(m_region);
	MCGRegionSetRect(m_region,
	                 MCGIntegerRectangleMake(0, 0,
	                                         m_surface->w, m_surface->h));

	return m_region;
}

uint32_t
MCSdlStackSurface::GetStride()
{
	MCAssert(nil != m_surface);

	return m_surface->pitch;
}

MCGRasterFormat
MCSdlStackSurface::GetFormat()
{
	return kMCGRasterFormat_ARGB;
}

void *
MCSdlStackSurface::GetPixelBuffer(MCGIntegerRectangle p_area)
{
	MCAssert(nil != m_surface);

	byte_t *surface_buf = reinterpret_cast<byte_t *>(m_surface->pixels);

	byte_t *pix_buf = (surface_buf +
	                   p_area.origin.y * GetStride() +
	                   p_area.origin.x * sizeof(uint32_t));

	return reinterpret_cast<void *>(pix_buf);
}
