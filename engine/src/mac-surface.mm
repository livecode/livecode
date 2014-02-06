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

////////////////////////////////////////////////////////////////////////////////

// COCOA-TODO: Clean up external linkage for surface.
extern MCRectangle MCU_intersect_rect(const MCRectangle&, const MCRectangle&);
extern bool MCU_empty_rect(const MCRectangle&);
extern bool MCGRasterToCGImage(const MCGRaster &p_raster, MCGRectangle p_src_rect, CGColorSpaceRef p_colorspace, bool p_copy, bool p_invert, CGImageRef &r_image);
extern bool MCGImageToCGImage(MCGImageRef p_src, MCGRectangle p_src_rect, bool p_copy, bool p_invert, CGImageRef &r_image);
extern MCGFloat MCResGetDeviceScale(void);

////////////////////////////////////////////////////////////////////////////////

static void MCMacRenderBitsToCG(CGContextRef p_target, CGRect p_area, const void *p_bits, uint32_t p_stride, bool p_has_alpha);
static void MCMacRenderImageToCG(CGContextRef p_target, CGRect p_dst_rect, MCGImageRef &p_src, MCGRectangle p_src_rect, MCGFloat p_alpha, MCGBlendMode p_blend);

////////////////////////////////////////////////////////////////////////////////

MCMacPlatformSurface::MCMacPlatformSurface(MCMacPlatformWindow *p_window, CGContextRef p_cg_context, MCRegionRef p_update_rgn)
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
	m_locked_stride = 0;
	
	// Setup everything so that its ready for use.
	Lock();
}

MCMacPlatformSurface::~MCMacPlatformSurface(void)
{
	// Teardown any setup.
	Unlock();
	
	m_window -> Release();
}

bool MCMacPlatformSurface::LockGraphics(MCRegionRef p_region, MCGContextRef& r_context)
{
	MCGRaster t_raster;
	if (LockPixels(p_region, t_raster))
	{
		if (MCGContextCreateWithRaster(t_raster, m_locked_context))
		{
			// Set origin
			MCGContextTranslateCTM(m_locked_context, -m_locked_area . x, -m_locked_area . y);
			
			// Set clipping rect
			MCGContextClipToRect(m_locked_context, MCRectangleToMCGRectangle(m_locked_area));
			
			r_context = m_locked_context;
			
			return true;
		}
	}
}

void MCMacPlatformSurface::UnlockGraphics(void)
{
	if (m_locked_context == nil)
		return;
	
	MCGContextRelease(m_locked_context);
	m_locked_context = nil;
	
	UnlockPixels();
}

bool MCMacPlatformSurface::LockPixels(MCRegionRef p_region, MCGRaster& r_raster)
{
	MCRectangle t_actual_area;
	t_actual_area = MCU_intersect_rect(MCRegionGetBoundingBox(p_region), MCRegionGetBoundingBox(m_update_rgn));
	
	if (MCU_empty_rect(t_actual_area))
		return false;
	
	m_locked_stride = t_actual_area . width * sizeof(uint32_t);
	m_locked_bits = malloc(t_actual_area . height * m_locked_stride);
	if (m_locked_bits != nil)
	{
		m_locked_area = t_actual_area;
		
		r_raster . width = t_actual_area . width;
		r_raster . height = t_actual_area . height;
		r_raster . stride = m_locked_stride;
		r_raster . pixels = m_locked_bits;
		r_raster . format = kMCGRasterFormat_ARGB;
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
	
	CGRect t_dst_rect;
	t_dst_rect = CGRectMake(m_locked_area . x, t_surface_height - (m_locked_area . y + m_locked_area . height), m_locked_area . width, m_locked_area . height);
	
	MCMacRenderBitsToCG(m_cg_context, t_dst_rect, m_locked_bits, m_locked_stride, false);
	
	free(m_locked_bits);
	m_locked_bits = nil;
}

bool MCMacPlatformSurface::LockSystemContext(void*& r_context)
{
	r_context = m_cg_context;
	return true;
}

void MCMacPlatformSurface::UnlockSystemContext(void)
{
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
	t_surface_height = m_window -> m_content . height;
	
	// apply transformation to rect (0, 0, image width, image height)
	MCGRectangle t_dst_rect, t_src_rect;
	t_src_rect = MCGRectangleMake(0, 0, MCGImageGetWidth(p_src_image), MCGImageGetHeight(p_src_image));
	t_dst_rect = MCGRectangleMake(t_dx, t_dy, t_src_rect.size.width * t_sx, t_src_rect.size.height * t_sy);
	
	CGContextSaveGState(m_cg_context);
	
	// clip to dst rect
	MCRectangle t_bounds;
	t_bounds = MCGRectangleGetIntegerBounds(p_dst_rect);
	CGRect t_dst_clip;
	t_dst_clip = CGRectMake(t_bounds . x, t_surface_height - (t_bounds . y + t_bounds . height), t_bounds . width, t_bounds . height);
	CGContextClipToRect(m_cg_context, t_dst_clip);
	
	// render image to transformed rect
	CGRect t_dst_cgrect;
	t_dst_cgrect = CGRectMake(t_dst_rect . origin . x, t_surface_height - (t_dst_rect . origin . y + t_dst_rect . size . height), t_dst_rect . size . width, t_dst_rect . size . height);
	MCMacRenderImageToCG(m_cg_context, t_dst_cgrect, p_src_image, t_src_rect, p_opacity, p_blend_mode);
	
	CGContextRestoreGState(m_cg_context);
	
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
		
		MCRectangle t_rect;
		t_rect = MCRegionGetBoundingBox(m_update_rgn);
		CGContextClearRect(m_cg_context, CGRectMake(t_rect . x, t_surface_height - (t_rect . y + t_rect . height), t_rect . width, t_rect . height));
		
		// IM-2013-08-29: [[ ResIndependence ]] scale mask to device coords
		MCGFloat t_scale;
		t_scale = MCResGetDeviceScale();
		
		MCGFloat t_mask_height, t_mask_width;
		t_mask_width = CGImageGetWidth(t_mask) * t_scale;
		t_mask_height = CGImageGetHeight(t_mask) * t_scale;
		
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

static void MCMacRenderBitsToCG(CGContextRef p_target, CGRect p_area, const void *p_bits, uint32_t p_stride, bool p_has_alpha)
{
	CGColorSpaceRef t_colorspace;
	t_colorspace = CGColorSpaceCreateDeviceRGB();
	if (t_colorspace != nil)
	{
		MCGRaster t_raster;
		t_raster.width = p_area.size.width;
		t_raster.height = p_area.size.height;
		t_raster.pixels = const_cast<void*>(p_bits);
		t_raster.stride = p_stride;
		t_raster.format = p_has_alpha ? kMCGRasterFormat_ARGB : kMCGRasterFormat_xRGB;
		
		CGImageRef t_image;
		t_image = nil;
		
		if (MCGRasterToCGImage(t_raster, MCGRectangleMake(0, 0, t_raster.width, t_raster.height), t_colorspace, false, false, t_image))
		{
			CGContextClipToRect((CGContextRef)p_target, p_area);
			CGContextDrawImage((CGContextRef)p_target, p_area, t_image);
			CGImageRelease(t_image);
		}
		
		CGColorSpaceRelease(t_colorspace);
	}
}

////////////////////////////////////////////////////////////////////////////////



