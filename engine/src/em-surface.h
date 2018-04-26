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

#ifndef __MC_EMSCRIPTEN_STACK_SURFACE_H__
#define __MC_EMSCRIPTEN_STACK_SURFACE_H__

//#include "globdefs.h"
#include "objdefs.h"
#include "parsedef.h"
//#include "sysdefs.h"
#include "stack.h"
#include "graphics.h"


/* ================================================================
 * Abstract raster surface
 * ================================================================ */

class MCAbstractRasterStackSurface : public MCStackSurface
{
public:
	/* ---------- Default implementations */

	/* Lock/unlock the surface for access with an MCGContextRef */
	virtual bool LockGraphics(MCGIntegerRectangle p_area,
	                          MCGContextRef & r_context,
	                          MCGRaster & r_raster);
	virtual void UnlockGraphics(MCGIntegerRectangle p_area,
	                            MCGContextRef context,
	                            MCGRaster & p_raster);

	/* Lock/unlock pixels within a given region */
	virtual bool LockPixels(MCGIntegerRectangle p_area,
	                        MCGRaster & r_raster,
	                        MCGIntegerRectangle & r_locked_area);
	virtual void UnlockPixels(MCGIntegerRectangle area, MCGRaster & raster);

	/* Lock/unlock the surface for direct access via the underlying system
	 * resource. */
	virtual bool LockTarget(MCStackSurfaceTargetType p_type, void *& r_context);
	virtual void UnlockTarget();

	/* Compose an image onto the surface */
	virtual bool Composite(MCGRectangle p_dst_rect,
	                       MCGImageRef p_source,
	                       MCGRectangle p_src_rect,
	                       MCGFloat p_alpha,
	                       MCGBlendMode p_blend);

	/* Lock and unlock the surface. Override in subclasses if
	 * necessary. */
	virtual bool Lock();
	virtual void Unlock();

protected:
	/* ---------- Implement in subclasses */

	virtual MCGRegionRef GetRegion() = 0;
	virtual uint32_t GetStride() = 0;
	virtual MCGRasterFormat GetFormat() = 0;
	virtual void *GetPixelBuffer(MCGIntegerRectangle p_area) = 0;
};

/* ================================================================
 * SDL canvas surface
 * ================================================================ */

class MCHtmlCanvasStackSurface : public MCAbstractRasterStackSurface
{
public:
    MCHtmlCanvasStackSurface(uint32_t p_window_id, const MCGIntegerRectangle& p_rect);
    virtual ~MCHtmlCanvasStackSurface();

	/* Lock and unlock the surface. */
	virtual bool Lock();
	virtual void Unlock();

protected:
	virtual MCGRegionRef GetRegion();
	virtual uint32_t GetStride();
	virtual MCGRasterFormat GetFormat();
	virtual void *GetPixelBuffer(MCGIntegerRectangle p_area);

	uint32_t m_window_id;
    uint8_t *m_surface;
	MCGRegionRef m_region;
    MCGIntegerRectangle m_rect;
};

#endif /* !__MC_EMSCRIPTEN_STACK_SURFACE_H__ */
