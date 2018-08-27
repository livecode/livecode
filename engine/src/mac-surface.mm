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

#include <Cocoa/Cocoa.h>

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
extern bool MCGRasterToCGImage(const MCGRaster &p_raster, const MCGIntegerRectangle &p_src_rect, CGColorSpaceRef p_colorspace, bool p_copy, bool p_invert, CGImageRef &r_image);
extern bool MCGImageToCGImage(MCGImageRef p_src, const MCGIntegerRectangle &p_src_rect, bool p_invert, CGImageRef &r_image);
extern MCGFloat MCResGetDeviceScale(void);

////////////////////////////////////////////////////////////////////////////////

// IM-2014-10-03: [[ Bug 13432 ]] Add src_rect, alpha & blend mode parameters to MCMacRenderRasterToCG().
static void MCMacRenderRasterToCG(CGContextRef p_target, CGRect p_dst_rect, const MCGRaster &p_src, MCGRectangle p_src_rect, MCGFloat p_alpha, MCGBlendMode p_blend);
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
	
	// IM-2015-02-23: [[ WidgetPopup ]] Find out if this surface is for an opaque window
	MCPlatformGetWindowProperty(p_window, kMCPlatformWindowPropertyIsOpaque, kMCPlatformPropertyTypeBool, &m_opaque);
	
	// Borrow the CGContext and MCRegion for now.
	m_cg_context = p_cg_context;
	m_cg_context_first_lock = true;
	
	m_update_rgn = p_update_rgn;
    
    m_raster . pixels = nil;
    
	// Setup everything so that its ready for use.
	Lock();
}

MCMacPlatformSurface::~MCMacPlatformSurface(void)
{
	// Teardown any setup.
	Unlock();
	
	m_window -> Release();
}

// MM-2014-07-31: [[ ThreadedRendering ]] Updated to use the new platform surface API.
bool MCMacPlatformSurface::LockGraphics(MCGIntegerRectangle p_region, MCGContextRef& r_context, MCGRaster &r_raster)
{
	MCGRaster t_raster;
	MCGIntegerRectangle t_locked_area;
	if (LockPixels(p_region, t_raster, t_locked_area))
	{
        MCGContextRef t_gcontext;
		if (MCGContextCreateWithRaster(t_raster, t_gcontext))
		{
            MCGFloat t_scale;
            t_scale = GetBackingScaleFactor();
            
			// Scale by backing scale
			MCGContextScaleCTM(t_gcontext, t_scale, t_scale);
			
			// Set origin
			MCGContextTranslateCTM(t_gcontext, -t_locked_area . origin . x, -t_locked_area . origin . y);
			
			// Set clipping rect
            MCGContextClipToRegion(t_gcontext, m_update_rgn);
            MCGContextClipToRect(t_gcontext, MCGIntegerRectangleToMCGRectangle(p_region));
			
			r_context = t_gcontext;
            r_raster = t_raster;
			
			return true;
		}
		
		UnlockPixels(t_locked_area, t_raster);
	}
    return false;
}

// MM-2014-07-31: [[ ThreadedRendering ]] Updated to use the new platform surface API.
void MCMacPlatformSurface::UnlockGraphics(MCGIntegerRectangle p_region, MCGContextRef p_context, MCGRaster &p_raster)
{
	if (p_context == nil)
		return;
    
	MCGContextRelease(p_context);
	UnlockPixels(p_region, p_raster);
}

// MM-2014-07-31: [[ ThreadedRendering ]] Updated to use the new platform surface API.
//  We create a single backing buffer for the entire surface (created the first time lock pixels is called)
//  and return a raster that points to the desired region of the backing buffer.
bool MCMacPlatformSurface::LockPixels(MCGIntegerRectangle p_region, MCGRaster& r_raster, MCGIntegerRectangle &r_locked_area)
{
    MCGIntegerRectangle t_bounds;
    t_bounds = MCGRegionGetBounds(m_update_rgn);
    
    MCGFloat t_scale;
    t_scale = GetBackingScaleFactor();
    
    if (m_raster . pixels == nil)
    {
        void *t_bits;
        t_bits = malloc(t_bounds . size . height * t_scale * t_bounds . size . width * t_scale * sizeof(uint32_t));
        if (t_bits == nil)
            return false;
        
        m_raster . width = t_bounds . size . width * t_scale;
        m_raster . height = t_bounds . size . height * t_scale;
        m_raster . stride = m_raster . width * sizeof(uint32_t);
		// IM-2015-02-23: [[ WidgetPopup ]] Specify ARGB format for non-opaque surfaces
		m_raster . format = m_opaque ? kMCGRasterFormat_xRGB : kMCGRasterFormat_ARGB;
        m_raster . pixels = t_bits;
    }
    
	MCGIntegerRectangle t_actual_area;
	t_actual_area = MCGIntegerRectangleIntersection(p_region, t_bounds);
	
	if (MCGIntegerRectangleIsEmpty(t_actual_area))
		return false;
	
    r_raster . width = t_actual_area . size . width * t_scale;
    r_raster . height = t_actual_area . size . height * t_scale;
    r_raster . stride = m_raster . stride;
    r_raster . format = m_raster . format;
    r_raster . pixels = (uint8_t*)m_raster . pixels + (int32_t)((t_actual_area . origin . y - t_bounds . origin.y) * t_scale * m_raster . stride + (t_actual_area . origin . x - t_bounds . origin . x) * t_scale * sizeof(uint32_t));
    
	r_locked_area = t_actual_area;
	
    return true;
}

// MM-2014-07-31: [[ ThreadedRendering ]] Updated to use the new platform surface API.
void MCMacPlatformSurface::UnlockPixels(MCGIntegerRectangle p_region, MCGRaster& p_raster)
{
}

bool MCMacPlatformSurface::LockSystemContext(void*& r_context)
{
	// IM-2014-10-03: [[ Bug 13432 ]] Make sure the window mask has been applied to the context.
	if (m_cg_context_first_lock)
	{
		m_cg_context_first_lock = false;
		ApplyMaskToCGContext();
	}
	
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

void MCMacPlatformSurface::ApplyMaskToCGContext()
{
    CGImageRef t_mask;
	t_mask = nil;
	if (m_window -> m_mask != nil)
		t_mask = ((MCMacPlatformWindowMask*)m_window -> m_mask) -> cg_mask;
    
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
}

void MCMacPlatformSurface::Lock(void)
{
	CGContextSaveGState(m_cg_context);
}

void MCMacPlatformSurface::Unlock(void)
{
    // MM-2014-07-31: [[ ThreadedRendering ]] Moved the drawing to the system context to unlock from unlock pixels.
    //  This way, we only blit to screen once, after all individual tiles have been rendered.
    if (m_raster . pixels != nil)
    {
        // COCOA-TODO: Getting the height to flip round is dependent on a friend.
        int t_surface_height;
        t_surface_height = m_window -> m_content . height;
        
        MCGIntegerRectangle t_bounds;
        t_bounds = MCGRegionGetBounds(m_update_rgn);
        
        CGRect t_dst_rect;
        t_dst_rect = MCMacFlipCGRect(MCGIntegerRectangleToCGRect(t_bounds), t_surface_height);
        
		if (m_window->m_mask != nil)
		{
			// IM-2014-10-22: [[ Bug 13746 ]] Apply the backing scale to the mask offset
			MCGFloat t_scale;
			t_scale = GetBackingScaleFactor();
			
			// IM-2014-10-03: [[ Bug 13432 ]] Set the buffer alpha directly from the mask raster.
			MCMacPlatformWindowMask *t_mask;
			t_mask = (MCMacPlatformWindowMask*)m_window->m_mask;
			MCGRasterApplyAlpha(m_raster, t_mask->mask, MCGIntegerPointMake(-t_bounds.origin.x * t_scale, -t_bounds.origin.y * t_scale));
		}
			
        MCMacClipCGContextToRegion(m_cg_context, m_update_rgn, t_surface_height);
        // IM-2014-10-03: [[ Bug 13432 ]] Render with copy blend mode to replace destination alpha with the source alpha.
        MCMacRenderRasterToCG(m_cg_context, t_dst_rect, m_raster, MCGRectangleMake(0, 0, m_raster.width, m_raster.height), 1.0, kMCGBlendModeCopy);
        
        m_raster . pixels = nil;
    }

	CGContextRestoreGState(m_cg_context);
}

MCGFloat MCMacPlatformSurface::GetBackingScaleFactor(void)
{
	if ([m_window -> GetHandle() respondsToSelector: @selector(backingScaleFactor)])
    {
        return static_cast<MCGFloat>([m_window -> GetHandle() backingScaleFactor]);
    }
	return 1.0f;
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
		default:
			MCUnreachableReturn(kCGBlendModeNormal); // unknown blend mode
	}
	
}

static void MCMacRenderCGImage(CGContextRef p_target, CGRect p_dst_rect, CGImageRef p_src, MCGFloat p_alpha, MCGBlendMode p_blend)
{
	CGContextSaveGState(p_target);
	
	CGContextClipToRect(p_target, p_dst_rect);
	CGContextSetAlpha(p_target, p_alpha);
	CGContextSetBlendMode(p_target, MCGBlendModeToCGBlendMode(p_blend));
	CGContextDrawImage(p_target, p_dst_rect, p_src);
	
	CGContextRestoreGState(p_target);
}

static void MCMacRenderImageToCG(CGContextRef p_target, CGRect p_dst_rect, MCGImageRef &p_src, MCGRectangle p_src_rect, MCGFloat p_alpha, MCGBlendMode p_blend)
{
	bool t_success = true;
	
	CGImageRef t_image = nil;
	
	t_success = MCGImageToCGImage(p_src, MCGRectangleGetBounds(p_src_rect), false, t_image);
	if (t_success)
	{
		MCMacRenderCGImage(p_target, p_dst_rect, t_image, p_alpha, p_blend);
		CGImageRelease(t_image);
	}
}

static void MCMacRenderRasterToCG(CGContextRef p_target, CGRect p_dst_rect, const MCGRaster &p_src, MCGRectangle p_src_rect, MCGFloat p_alpha, MCGBlendMode p_blend)
{
	CGColorSpaceRef t_colorspace;
	if (MCMacPlatformGetImageColorSpace(t_colorspace))
	{
		CGImageRef t_image;
		t_image = nil;
		
		if (MCGRasterToCGImage(p_src, MCGRectangleGetBounds(p_src_rect), t_colorspace, false, false, t_image))
		{
			MCMacRenderCGImage(p_target, p_dst_rect, t_image, p_alpha, p_blend);
			CGImageRelease(t_image);
		}
		
		CGColorSpaceRelease(t_colorspace);
	}
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

extern bool MCImageGetCGColorSpace(CGColorSpaceRef &r_colorspace);
bool MCMacPlatformGetImageColorSpace(CGColorSpaceRef &r_colorspace)
{
	return MCImageGetCGColorSpace(r_colorspace);
}

////////////////////////////////////////////////////////////////////////////////

