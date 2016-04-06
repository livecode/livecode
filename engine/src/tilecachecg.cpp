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

#include "prefix.h"

#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"

#include "util.h"
#include "stack.h"
#include "region.h"
#include "tilecache.h"

#ifdef _IOS_MOBILE
#include <CoreGraphics/CoreGraphics.h>
#else
#include <ApplicationServices/ApplicationServices.h>
#endif

////////////////////////////////////////////////////////////////////////////////

#ifndef _IOS_MOBILE
#define CGFloat float
#endif

////////////////////////////////////////////////////////////////////////////////

extern CGBitmapInfo MCGPixelFormatToCGBitmapInfo(uint32_t p_pixel_format, bool p_alpha);
extern bool MCImageGetCGColorSpace(CGColorSpaceRef &r_colorspace);

typedef void (*surface_combiner_t)(void *p_dst, int32_t p_dst_stride, const void *p_src, uint4 p_src_stride, uint4 p_width, uint4 p_height, uint1 p_opacity);
extern surface_combiner_t s_surface_combiners_nda[];

struct MCTileCacheCoreGraphicsCompositorContext
{
	// The tilecache
	MCTileCacheRef tilecache;
	
	// The size of the tiles
	int32_t tile_size;
	
	// The height of the viewport.
	int32_t viewport_height;
	
	// The colorspace to use for tiles.
	CGColorSpaceRef colorspace;
	
	// The CoreGraphics context to use.
	CGContextRef cgcontext;
};

bool MCTileCacheCoreGraphicsCompositor_AllocateTile(void *p_context, int32_t p_size, const void *p_bits, uint32_t p_stride, void*& r_tile)
{
	MCTileCacheCoreGraphicsCompositorContext *self;
	self = (MCTileCacheCoreGraphicsCompositorContext *)p_context;
	
	// If the stride is exactly one tile wide, we don't need a copy.
	void *t_data;
	t_data = nil;
	if (p_stride == p_size * sizeof(uint32_t))
		t_data = (void *)p_bits;
	else if (MCMemoryAllocate(p_size * p_size * sizeof(uint32_t), t_data))
	{
		// Copy across each scanline of the tile into the buffer.
		for(int32_t y = 0; y < p_size; y++)
			memcpy((uint8_t *)t_data + y * p_size * sizeof(uint32_t), (uint8_t *)p_bits + p_stride * y, p_size * sizeof(uint32_t));
	}
	
	CGImageRef t_tile;
	t_tile = nil;
	if (t_data != nil)
	{
		// IM-2013-08-21: [[ RefactorGraphics ]] Refactor CGImage creation code to be pixel-format independent
		CGBitmapInfo t_bm_info;
		t_bm_info = MCGPixelFormatToCGBitmapInfo(kMCGPixelFormatNative, true);
		
		CGContextRef t_cgcontext;
		t_cgcontext = CGBitmapContextCreate((void *)t_data, p_size, p_size, 8, p_size * sizeof(uint32_t), self -> colorspace, t_bm_info);
		if (t_cgcontext != nil)
		{
			t_tile = CGBitmapContextCreateImage(t_cgcontext);
			CGContextRelease(t_cgcontext);
		}
	}
	
	if (t_data != p_bits)
		MCMemoryDeallocate(t_data);
		
	if (t_tile == nil)
		return false;

	r_tile = t_tile;
	
	return true;
}

void MCTileCacheCoreGraphicsCompositor_DeallocateTile(void *p_context, void *p_tile)
{
	MCTileCacheCoreGraphicsCompositorContext *self;
	self = (MCTileCacheCoreGraphicsCompositorContext *)p_context;
	
	CGImageRelease((CGImageRef)p_tile);
}

bool MCTileCacheCoreGraphicsCompositor_BeginFrame(void *p_context, MCStackSurface *p_surface, MCGRegionRef p_dirty)
{
	MCTileCacheCoreGraphicsCompositorContext *self;
	self = (MCTileCacheCoreGraphicsCompositorContext *)p_context;
	
	void *t_target;
	if (!p_surface -> LockTarget(kMCStackSurfaceTargetCoreGraphics, t_target))
		return false;
	
	self -> tile_size = MCTileCacheGetTileSize(self -> tilecache);
	self -> viewport_height = MCTileCacheGetViewport(self -> tilecache) . height;
	self -> cgcontext = (CGContextRef)t_target;
	
	return true;
}

bool MCTileCacheCoreGraphicsCompositor_EndFrame(void *p_context, MCStackSurface *p_surface)
{
	MCTileCacheCoreGraphicsCompositorContext *self;
	self = (MCTileCacheCoreGraphicsCompositorContext *)p_context;
	
	p_surface -> UnlockTarget();
	
	return true;
}

bool MCTileCacheCoreGraphicsCompositor_BeginLayer(void *p_context, const MCRectangle& p_clip, uint32_t p_opacity, uint32_t p_ink)
{
	MCTileCacheCoreGraphicsCompositorContext *self;
	self = (MCTileCacheCoreGraphicsCompositorContext *)p_context;
	
	CGContextSaveGState(self -> cgcontext);
	
	// MW-2012-09-18: [[ Bug 10202 ]] If the ink is no-op then ensure nothing happens.
	if (p_ink == GXnoop)
		CGContextClipToRect(self -> cgcontext, CGRectZero);
	else
	{
		CGContextClipToRect(self -> cgcontext, CGRectMake(p_clip . x, self -> viewport_height - (p_clip . y + p_clip . height), p_clip . width, p_clip . height));
		CGContextSetAlpha(self -> cgcontext, p_opacity / 255.0);
		
		CGBlendMode t_blend_mode;
		switch(p_ink)
		{
			case GXblendMultiply:
				t_blend_mode = kCGBlendModeMultiply;
				break;
			case GXblendScreen: 
				t_blend_mode = kCGBlendModeScreen;
				break;
			case GXblendOverlay: 
				t_blend_mode = kCGBlendModeOverlay;
				break;
			case GXblendDarken: 
				t_blend_mode = kCGBlendModeDarken;
				break;
			case GXblendLighten: 
				t_blend_mode = kCGBlendModeLighten;
				break;
			case GXblendDodge: 
				t_blend_mode = kCGBlendModeColorDodge;
				break;
			case GXblendBurn: 
				t_blend_mode = kCGBlendModeColorBurn;
				break;
			case GXblendSoftLight: 
				t_blend_mode = kCGBlendModeSoftLight;
				break;
			case GXblendHardLight: 
				t_blend_mode = kCGBlendModeHardLight;
				break;
			case GXblendDifference: 
				t_blend_mode = kCGBlendModeDifference;
				break;
			case GXblendExclusion: 
				t_blend_mode = kCGBlendModeExclusion;
				break;
			default:
				t_blend_mode = kCGBlendModeNormal;
				break;
		}
		CGContextSetBlendMode(self -> cgcontext, t_blend_mode);
	}
	
	return true;
}

bool MCTileCacheCoreGraphicsCompositor_EndLayer(void *p_context)
{
	MCTileCacheCoreGraphicsCompositorContext *self;
	self = (MCTileCacheCoreGraphicsCompositorContext *)p_context;
	
	CGContextRestoreGState(self -> cgcontext);
	
	return true;
}

bool MCTileCacheCoreGraphicsCompositor_CompositeTile(void *p_context, int32_t p_x, int32_t p_y, void *p_tile)
{
	MCTileCacheCoreGraphicsCompositorContext *self;
	self = (MCTileCacheCoreGraphicsCompositorContext *)p_context;
	
	CGContextDrawImage(self -> cgcontext, CGRectMake(p_x, self -> viewport_height - (p_y + self -> tile_size), self -> tile_size, self -> tile_size), (CGImageRef)p_tile);
	
	return true;
}

bool MCTileCacheCoreGraphicsCompositor_CompositeRect(void *p_context, int32_t p_x, int32_t p_y, uint32_t p_color)
{
	MCTileCacheCoreGraphicsCompositorContext *self;
	self = (MCTileCacheCoreGraphicsCompositorContext *)p_context;
	
	// IM-2013-08-23: [[ RefactorGraphics ]] Use MCGPixelUnpackNative to fix color swap issues
	uint8_t r, g, b, a;
	MCGPixelUnpackNative(p_color, r, g, b, a);
	
	CGFloat t_red, t_green, t_blue, t_alpha;
	t_red = r / 255.0;
	t_green = g / 255.0;
	t_blue = b / 255.0;
	t_alpha = a / 255.0;
	
	if (t_alpha != 1.0)
	{
		t_red /= t_alpha;
		t_green /= t_alpha;
		t_blue /= t_alpha;
	}
	
	CGContextSetRGBFillColor(self -> cgcontext, t_red, t_green, t_blue, t_alpha);
	CGContextFillRect(self -> cgcontext, CGRectMake(p_x, self -> viewport_height - (p_y + self -> tile_size), self -> tile_size, self -> tile_size));
	
	return true;
}

void MCTileCacheCoreGraphicsCompositor_Cleanup(void *p_context)
{
	MCTileCacheCoreGraphicsCompositorContext *self;
	self = (MCTileCacheCoreGraphicsCompositorContext *)p_context;
	
	CGColorSpaceRelease(self -> colorspace);
	
	MCMemoryDelete(self);
}

////////////////////////////////////////////////////////////////////////////////

bool MCTileCacheCoreGraphicsCompositorConfigure(MCTileCacheRef p_tilecache, MCTileCacheCompositor& r_compositor)
{
	MCTileCacheCoreGraphicsCompositorContext *t_context;
	if (!MCMemoryNew(t_context))
		return false;
	
	t_context -> tilecache = p_tilecache;
	/* UNCHECKED */ MCImageGetCGColorSpace(t_context -> colorspace);
	
	r_compositor . context = t_context;
	r_compositor . cleanup = MCTileCacheCoreGraphicsCompositor_Cleanup;
	r_compositor . allocate_tile = MCTileCacheCoreGraphicsCompositor_AllocateTile;
	r_compositor . deallocate_tile = MCTileCacheCoreGraphicsCompositor_DeallocateTile;
	r_compositor . begin_frame = MCTileCacheCoreGraphicsCompositor_BeginFrame;
	r_compositor . end_frame = MCTileCacheCoreGraphicsCompositor_EndFrame;
	r_compositor . begin_layer = MCTileCacheCoreGraphicsCompositor_BeginLayer;
	r_compositor . end_layer = MCTileCacheCoreGraphicsCompositor_EndLayer;
	r_compositor . composite_tile = MCTileCacheCoreGraphicsCompositor_CompositeTile;
	r_compositor . composite_rect = MCTileCacheCoreGraphicsCompositor_CompositeRect;
	
	return true;
}

////////////////////////////////////////////////////////////////////////////////
