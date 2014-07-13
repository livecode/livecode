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

#include <Cocoa/Cocoa.h>

#include "core.h"
#include "globdefs.h"
#include "region.h"
#include "graphics.h"
#include "graphics_util.h"

#include "platform.h"
#include "platform-internal.h"

#include "mac-internal.h"

#include <objc/objc-runtime.h>

////////////////////////////////////////////////////////////////////////////////


// COCOA-TODO: Clean up external linkage for surface.
extern MCRectangle MCU_intersect_rect(const MCRectangle&, const MCRectangle&);
extern bool MCGRasterToCGImage(const MCGRaster &p_raster, MCGRectangle p_src_rect, CGColorSpaceRef p_colorspace, bool p_copy, bool p_invert, CGImageRef &r_image);
extern bool MCGImageToCGImage(MCGImageRef p_src, MCGRectangle p_src_rect, bool p_copy, bool p_invert, CGImageRef &r_image);
extern MCGFloat MCResGetDeviceScale(void);

////////////////////////////////////////////////////////////////////////////////

static void MCMacRenderBitsToCG(CGContextRef p_target, CGRect p_area, int32_t p_width, int32_t p_height, const void *p_bits, uint32_t p_stride, bool p_has_alpha);
static void MCMacRenderRasterToCG(CGContextRef p_target, CGRect p_area, const MCGRaster &p_raster);
static void MCMacRenderImageToCG(CGContextRef p_target, CGRect p_dst_rect, MCGImageRef &p_src, MCGRectangle p_src_rect, MCGFloat p_alpha, MCGBlendMode p_blend);

static void MCMacClipCGContextToRegion(CGContextRef p_context, MCGRegionRef p_region, uint32_t p_surface_height);

////////////////////////////////////////////////////////////////////////////////

CGRect MCGIntegerRectangleToCGRect(const MCGIntegerRectangle &p_rect)
{
	return CGRectMake(p_rect.origin.x, p_rect.origin.y, p_rect.size.width, p_rect.size.height);
}

CGRect MCMacFlipCGRect(const CGRect &p_rect, uint32_t p_surface_height)
{
	return CGRectMake(p_rect.origin.x, p_surface_height - (p_rect.origin.y + p_rect.size.height), p_rect.size.width, p_rect.size.height);
}

////////////////////////////////////////////////////////////////////////////////

MCMacPlatformSurface::MCMacPlatformSurface(MCMacPlatformWindow *p_window, CGContextRef p_cg_context, MCGRegionRef p_update_rgn)
{
	// Retain the window to ensure it doesn't vanish whilst the surface is alive.
	m_window = p_window;
	m_window -> Retain();
	
	// Borrow the CGContext and MCRegion for now.
	m_cg_context = p_cg_context;
	m_update_rgn = p_update_rgn;
	
	// Initialize state.
	m_locked_context = nil;
	m_locked_bits = nil;
	
    m_defer_unlock = false;
    
	// Setup everything so that its ready for use.
	Lock();
}

MCMacPlatformSurface::~MCMacPlatformSurface(void)
{
	// Teardown any setup.
	Unlock();
	
	m_window -> Release();
}

bool MCMacPlatformSurface::LockGraphics(MCGRegionRef p_region, MCGContextRef& r_context)
{
	MCGRaster t_raster;
	if (LockPixels(MCGRegionGetBounds(p_region), t_raster))
	{
		if (MCGContextCreateWithRaster(t_raster, m_locked_context))
		{
			MCGFloat t_scale;
			t_scale = GetBackingScaleFactor();
			
			// Scale by backing scale
			MCGContextScaleCTM(m_locked_context, t_scale, t_scale);
			
			// Set origin
			MCGContextTranslateCTM(m_locked_context, -m_locked_area . origin . x, -m_locked_area . origin . y);
			
			// Set clipping rect
			MCGContextClipToRegion(m_locked_context, p_region);
			
			r_context = m_locked_context;
			
			return true;
		}
	}
}

void MCMacPlatformSurface::UnlockGraphics(void)
{
	if (m_locked_context == nil)
		return;
	
    if (m_defer_unlock)
        return;
    
	MCGContextRelease(m_locked_context);
	m_locked_context = nil;
	
	UnlockPixels();
}

bool MCMacPlatformSurface::LockPixels(MCGIntegerRectangle p_region, MCGRaster& r_raster)
{
	MCGIntegerRectangle t_actual_area;
	t_actual_area = MCGIntegerRectangleIntersection(p_region, MCGRegionGetBounds(m_update_rgn));
	
	if (MCGIntegerRectangleIsEmpty(t_actual_area))
		return false;
	
	MCGFloat t_scale;
	t_scale = GetBackingScaleFactor();
	
	m_locked_raster.width = t_actual_area . size . width * t_scale;
	m_locked_raster.height = t_actual_area . size . height * t_scale;
	m_locked_raster.stride = m_locked_raster.width * sizeof(uint32_t);
	m_locked_raster.format = kMCGRasterFormat_xRGB;

	m_locked_bits = malloc(m_locked_raster.height * m_locked_raster.stride);
	if (m_locked_bits != nil)
	{
		m_locked_area = t_actual_area;
		m_locked_raster.pixels = m_locked_bits;
		r_raster = m_locked_raster;
		return true;
	}
	
	return false;
}

void MCMacPlatformSurface::UnlockPixels(void)
{
	if (m_locked_bits == nil)
		return;
	
	// COCOA-TODO: Getting the height to flip round is dependent on a friend.
	int t_surface_height;
	t_surface_height = m_window -> m_content . height;
	
	MCGFloat t_scale;
	t_scale = GetBackingScaleFactor();
	
	CGRect t_dst_rect;
	t_dst_rect = MCMacFlipCGRect(MCGIntegerRectangleToCGRect(m_locked_area), t_surface_height);
	
	MCMacClipCGContextToRegion(m_cg_context, m_update_rgn, t_surface_height);
	MCMacRenderRasterToCG(m_cg_context, t_dst_rect, m_locked_raster);
	
	free(m_locked_bits);
	m_locked_bits = nil;
}

bool MCMacPlatformSurface::LockSystemContext(void*& r_context)
{
	// IM-2014-06-12: [[ Bug 12354 ]] Lock the surface context without scaling
	CGFloat t_scale;
	t_scale = 1.0 / GetBackingScaleFactor();
	
	CGContextSaveGState(m_cg_context);
	CGContextScaleCTM(m_cg_context, t_scale, t_scale);
	
	r_context = m_cg_context;
	return true;
}

void MCMacPlatformSurface::UnlockSystemContext(void)
{
	CGContextRestoreGState(m_cg_context);
}

bool MCMacPlatformSurface::Composite(MCGRectangle p_dst_rect, MCGImageRef p_src_image, MCGRectangle p_src_rect, MCGFloat p_opacity, MCGBlendMode p_blend_mode)
{
	// IM-2013-08-21: [[ RefactorGraphics]] Rework to fix positioning of composited src image
	// compute transform from src rect to dst rect
	MCGFloat t_sx, t_sy, t_dx, t_dy;
	t_sx = p_dst_rect.size.width / p_src_rect.size.width;
	t_sy = p_dst_rect.size.height / p_src_rect.size.height;
	
	t_dx = p_dst_rect.origin.x - (p_src_rect.origin.x * t_sx);
	t_dy = p_dst_rect.origin.y - (p_src_rect.origin.y * t_sy);
	
	// COCOA-TODO: Getting the height to flip round is dependent on a friend.
	int t_surface_height;
	// IM-2014-06-12: [[ Bug 12354 ]] Apply the backing scale to get the surface pixel height
	t_surface_height = m_window -> m_content . height * GetBackingScaleFactor();
	
	// apply transformation to rect (0, 0, image width, image height)
	MCGRectangle t_dst_rect, t_src_rect;
	t_src_rect = MCGRectangleMake(0, 0, MCGImageGetWidth(p_src_image), MCGImageGetHeight(p_src_image));
	t_dst_rect = MCGRectangleMake(t_dx, t_dy, t_src_rect.size.width * t_sx, t_src_rect.size.height * t_sy);
	
	CGContext *t_context;
	t_context = nil;
	
	// IM-2014-06-12: [[ Bug 12354 ]] Use lock method to get the context for compositing
	/* UNCHECKED */ LockSystemContext((void*&)t_context);
	
	// clip to dst rect
	MCRectangle t_bounds;
	t_bounds = MCGRectangleGetIntegerBounds(p_dst_rect);
	CGRect t_dst_clip;
	t_dst_clip = CGRectMake(t_bounds . x, t_surface_height - (t_bounds . y + t_bounds . height), t_bounds . width, t_bounds . height);
	CGContextClipToRect(t_context, t_dst_clip);
	
	// render image to transformed rect
	CGRect t_dst_cgrect;
	t_dst_cgrect = CGRectMake(t_dst_rect . origin . x, t_surface_height - (t_dst_rect . origin . y + t_dst_rect . size . height), t_dst_rect . size . width, t_dst_rect . size . height);
	MCMacRenderImageToCG(t_context, t_dst_cgrect, p_src_image, t_src_rect, p_opacity, p_blend_mode);
	
	UnlockSystemContext();
	
	return true;
}

void MCMacPlatformSurface::Lock(void)
{
	CGImageRef t_mask;
	t_mask = nil;
	if (m_window -> m_mask != nil)
		t_mask = (CGImageRef)m_window -> m_mask;
	
	if (t_mask != nil)
	{
		// COCOA-TODO: Getting the height to flip round is dependent on a friend.
		int t_surface_height;
		t_surface_height = m_window -> m_content . height;
		
		MCGIntegerRectangle t_rect;
		t_rect = MCGRegionGetBounds(m_update_rgn);
		CGContextClearRect(m_cg_context, MCMacFlipCGRect(MCGIntegerRectangleToCGRect(t_rect), t_surface_height));
		
		MCGFloat t_mask_height, t_mask_width;
		t_mask_width = CGImageGetWidth(t_mask);
		t_mask_height = CGImageGetHeight(t_mask);
		
		CGRect t_dst_rect;
		t_dst_rect . origin . x = 0;
		t_dst_rect . origin . y = t_surface_height - t_mask_height;
		t_dst_rect . size . width = t_mask_width;
		t_dst_rect . size . height = t_mask_height;
		CGContextClipToMask(m_cg_context, t_dst_rect, t_mask);
	}
	
	CGContextSaveGState(m_cg_context);
}

void MCMacPlatformSurface::Unlock(void)
{
	CGContextRestoreGState(m_cg_context);
}

MCGFloat MCMacPlatformSurface::GetBackingScaleFactor(void)
{
	if ([m_window -> GetHandle() respondsToSelector: @selector(backingScaleFactor)])
		return objc_msgSend_fpret(m_window -> GetHandle(), @selector(backingScaleFactor));
	return 1.0f;
}

void MCMacPlatformSurface::setDeferUnlock(bool p_value)
{ 
    m_defer_unlock = p_value; 
}

////////////////////////////////////////////////////////////////////////////////

static inline CGBlendMode MCGBlendModeToCGBlendMode(MCGBlendMode p_blend)
{
	// MM-2013-08-28: [[ RefactorGraphics ]] Tweak for 10.4 SDK support.
	switch (p_blend)
	{
		case kMCGBlendModeClear:
			return kCGBlendModeClear;
		case kMCGBlendModeCopy:
			return kCGBlendModeCopy;
		case kMCGBlendModeSourceIn:
			return kCGBlendModeSourceIn;
		case kMCGBlendModeSourceOut:
			return kCGBlendModeSourceOut;
		case kMCGBlendModeSourceAtop:
			return kCGBlendModeSourceAtop;
		case kMCGBlendModeDestinationOver:
			return kCGBlendModeDestinationOver;
		case kMCGBlendModeDestinationIn:
			return kCGBlendModeDestinationIn;
		case kMCGBlendModeDestinationOut:
			return kCGBlendModeDestinationOut;
		case kMCGBlendModeDestinationAtop:
			return kCGBlendModeDestinationAtop;
		case kMCGBlendModeXor:
			return kCGBlendModeXOR;
		case kMCGBlendModePlusDarker:
			return kCGBlendModePlusDarker;
		case kMCGBlendModePlusLighter:
			return kCGBlendModePlusLighter;
		case kMCGBlendModeSourceOver:
			return kCGBlendModeNormal;
		case kMCGBlendModeMultiply:
			return kCGBlendModeMultiply;
		case kMCGBlendModeScreen:
			return kCGBlendModeScreen;
		case kMCGBlendModeOverlay:
			return kCGBlendModeOverlay;
		case kMCGBlendModeDarken:
			return kCGBlendModeDarken;
		case kMCGBlendModeLighten:
			return kCGBlendModeLighten;
		case kMCGBlendModeColorDodge:
			return kCGBlendModeColorDodge;
		case kMCGBlendModeColorBurn:
			return kCGBlendModeColorBurn;
		case kMCGBlendModeSoftLight:
			return kCGBlendModeSoftLight;
		case kMCGBlendModeHardLight:
			return kCGBlendModeHardLight;
		case kMCGBlendModeDifference:
			return kCGBlendModeDifference;
		case kMCGBlendModeExclusion:
			return kCGBlendModeExclusion;
		case kMCGBlendModeHue:
			return kCGBlendModeHue;
		case kMCGBlendModeSaturation:
			return kCGBlendModeSaturation;
		case kMCGBlendModeColor:
			return kCGBlendModeColor;
		case kMCGBlendModeLuminosity:
			return kCGBlendModeLuminosity;
	}
	
	MCAssert(false); // unknown blend mode
}

static void MCMacRenderImageToCG(CGContextRef p_target, CGRect p_dst_rect, MCGImageRef &p_src, MCGRectangle p_src_rect, MCGFloat p_alpha, MCGBlendMode p_blend)
{
	bool t_success = true;
	
	CGImageRef t_image = nil;
	
	t_success = MCGImageToCGImage(p_src, p_src_rect, false, false, t_image);
	if (t_success)
	{
		CGContextSaveGState(p_target);
		
		CGContextClipToRect(p_target, p_dst_rect);
		CGContextSetAlpha(p_target, p_alpha);
		CGContextSetBlendMode(p_target, MCGBlendModeToCGBlendMode(p_blend));
		CGContextDrawImage(p_target, p_dst_rect, t_image);
		
		CGContextRestoreGState(p_target);
		CGImageRelease(t_image);
	}
}

static void MCMacRenderRasterToCG(CGContextRef p_target, CGRect p_area, const MCGRaster &p_raster)
{
	CGColorSpaceRef t_colorspace;
	t_colorspace = CGColorSpaceCreateDeviceRGB();
	if (t_colorspace != nil)
	{
		CGImageRef t_image;
		t_image = nil;
		
		if (MCGRasterToCGImage(p_raster, MCGRectangleMake(0, 0, p_raster.width, p_raster.height), t_colorspace, false, false, t_image))
		{
			CGContextClipToRect((CGContextRef)p_target, p_area);
			CGContextDrawImage((CGContextRef)p_target, p_area, t_image);
			CGImageRelease(t_image);
		}
		
		CGColorSpaceRelease(t_colorspace);
	}
}

static void MCMacRenderBitsToCG(CGContextRef p_target, CGRect p_area, int32_t p_width, int32_t p_height, const void *p_bits, uint32_t p_stride, bool p_has_alpha)
{
	MCGRaster t_raster;
	t_raster.width = p_width;
	t_raster.height = p_height;
	t_raster.pixels = const_cast<void*>(p_bits);
	t_raster.stride = p_stride;
	t_raster.format = p_has_alpha ? kMCGRasterFormat_ARGB : kMCGRasterFormat_xRGB;
	
	MCMacRenderRasterToCG(p_target, p_area, t_raster);
}

////////////////////////////////////////////////////////////////////////////////

struct MCGRegionConvertToCGRectsState
{
	CGRect *rects;
	uint32_t count;
};

static bool MCGRegionConvertToCGRectsCallback(void *p_state, const MCGIntegerRectangle &p_rect)
{
	MCGRegionConvertToCGRectsState *state;
	state = static_cast<MCGRegionConvertToCGRectsState*>(p_state);
	
	if (!MCMemoryResizeArray(state -> count + 1, state -> rects, state -> count))
		return false;
	
	state -> rects[state -> count - 1] = MCGIntegerRectangleToCGRect(p_rect);
	
	return true;
}

bool MCGRegionConvertToCGRects(MCGRegionRef self, CGRect *&r_cgrects, uint32_t& r_cgrect_count)
{
	MCGRegionConvertToCGRectsState t_state;
	t_state . rects = nil;
	t_state . count = 0;
	
	if (!MCGRegionIterate(self, MCGRegionConvertToCGRectsCallback, &t_state))
	{
		MCMemoryDeleteArray(t_state . rects);
		return false;
	}
	
	r_cgrects = t_state . rects;
	r_cgrect_count = t_state . count;
	
	return true;
}

static void MCMacClipCGContextToRegion(CGContextRef p_context, MCGRegionRef p_region, uint32_t p_surface_height)
{
	CGRect *t_rects;
	t_rects = nil;
	
	uint32_t t_count;
	
	if (!MCGRegionConvertToCGRects(p_region, t_rects, t_count))
		return;
	
	for (uint32_t i = 0; i < t_count; i++)
		t_rects[i] = MCMacFlipCGRect(t_rects[i], p_surface_height);
	
	CGContextClipToRects(p_context, t_rects, t_count);
	MCMemoryDeleteArray(t_rects);
}

////////////////////////////////////////////////////////////////////////////////



