/*                                                                     -*-c++-*-

Copyright (C) 2003-2015 LiveCode Ltd.

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
 * Functions implemented in em-surface.js
 * ================================================================ */


extern "C" void MCEmscriptenBlitToWindowCanvas(uint32_t p_window_id,
                                               const uint8_t* p_rgba_buffer,
                                               uint32_t p_dest_x,
                                               uint32_t p_dest_y,
                                               uint32_t p_width,
                                               uint32_t p_height);


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
 * HTML canvas surface
 * ================================================================ */

MCHtmlCanvasStackSurface::MCHtmlCanvasStackSurface(uint32_t p_window_id, const MCGIntegerRectangle& p_rect)
    : m_window_id(p_window_id),
      m_surface(nil),
      m_region(nil),
      m_rect(p_rect)
{
    ;
}

MCHtmlCanvasStackSurface::~MCHtmlCanvasStackSurface()
{
    delete[] m_surface;

	if (nil != m_region)
	{
		MCGRegionDestroy(m_region);
	}
}

bool
MCHtmlCanvasStackSurface::Lock()
{
    // Allocate a buffer for us to use, if not already done
    if (m_surface == nil)
    {
        m_surface = new (nothrow) uint8_t[m_rect.size.width * m_rect.size.height * sizeof(uint32_t)];
    }

    return m_surface != nil;
}

void
MCHtmlCanvasStackSurface::Unlock()
{
    // This is implemented in JavaScript
    MCEmscriptenBlitToWindowCanvas(m_window_id, m_surface, m_rect.origin.x, m_rect.origin.y, m_rect.size.width, m_rect.size.height);
}

MCGRegionRef
MCHtmlCanvasStackSurface::GetRegion()
{
	if (nil != m_region)
	{
		return m_region;
	}

	MCAssert(nil != m_surface);

	/* Compute the region from the surface */
	MCGRegionCreate(m_region);
    MCGRegionSetRect(m_region, m_rect);

	return m_region;
}

uint32_t
MCHtmlCanvasStackSurface::GetStride()
{
	MCAssert(nil != m_surface);

    return m_rect.size.width * sizeof(uint32_t);
}

MCGRasterFormat
MCHtmlCanvasStackSurface::GetFormat()
{
	return kMCGRasterFormat_ARGB;
}

void *
MCHtmlCanvasStackSurface::GetPixelBuffer(MCGIntegerRectangle p_area)
{
	MCAssert(nil != m_surface);

    uint8_t *pix_buf = (m_surface +
                       (p_area.origin.y-m_rect.origin.y) * GetStride() +
                       (p_area.origin.x-m_rect.origin.x) * sizeof(uint32_t));

    return static_cast<void *>(pix_buf);
}
