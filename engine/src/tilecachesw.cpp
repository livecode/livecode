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

#include "graphics_util.h"

////////////////////////////////////////////////////////////////////////////////

typedef void (*surface_combiner_t)(void *p_dst, int32_t p_dst_stride, const void *p_src, uint4 p_src_stride, uint4 p_width, uint4 p_height, uint1 p_opacity);
extern surface_combiner_t s_surface_combiners_nda[];

struct MCTileCacheSoftwareCompositorContext
{
	// The tilecache
	MCTileCacheRef tilecache;
	
	// The current tilesize
	int32_t tile_size;
	
	// The rectangle the bits cover
	MCRectangle dirty;

	// The raster to render into.
    MCGRaster raster;
    
	// The clip to apply to tiles
	MCRectangle clip;
	// The combiner to use
	surface_combiner_t combiner;
	// The opacity to use
	uint32_t opacity;
	
	// The temporary row used for rect fills.
	uint32_t *tile_row;
	// The last color filled.
	uint32_t tile_row_color;
};

bool MCTileCacheSoftwareCompositor_AllocateTile(void *p_context, int32_t p_size, const void *p_bits, uint32_t p_stride, void*& r_tile)
{
	void *t_data;
	if (!MCMemoryAllocate(p_size * p_size * sizeof(uint32_t), t_data))
		return false;

	// Copy across each scanline of the tile into the buffer.
	for(int32_t y = 0; y < p_size; y++)
		memcpy((uint8_t *)t_data + y * p_size * sizeof(uint32_t), (uint8_t *)p_bits + p_stride * y, p_size * sizeof(uint32_t));

	r_tile = t_data;

	return true;
}

void MCTileCacheSoftwareCompositor_DeallocateTile(void *p_context, void *p_tile)
{
	MCMemoryDeallocate(p_tile);
}

bool MCTileCacheSoftwareCompositor_BeginFrame(void *p_context, MCStackSurface *p_surface, MCGRegionRef p_dirty)
{
	MCTileCacheSoftwareCompositorContext *self;
	self = (MCTileCacheSoftwareCompositorContext *)p_context;
	
	MCGIntegerRectangle t_dirty;
	t_dirty = MCGRegionGetBounds(p_dirty);
	
    // MM-2014-07-31: [[ ThreadedRendering ]] Updated to use the new stack surface API.
	MCGRaster t_raster;
	MCGIntegerRectangle t_locked_area;
	if (!p_surface -> LockPixels(t_dirty, t_raster, t_locked_area))
		return false;
	    
    self -> raster = t_raster;

	MCMemoryDeallocate(self -> tile_row);
	self -> tile_row = nil;
	self -> tile_row_color = 0;
	
	self -> tile_size = MCTileCacheGetTileSize(self -> tilecache);
	self -> dirty = MCRectangleFromMCGIntegerRectangle(t_locked_area);
	self -> clip = self -> dirty;
	self -> combiner = s_surface_combiners_nda[GXcopy];
	self -> opacity = 255;
	
	return true;
}

bool MCTileCacheSoftwareCompositor_EndFrame(void *p_context, MCStackSurface *p_surface)
{
	MCTileCacheSoftwareCompositorContext *self;
	self = (MCTileCacheSoftwareCompositorContext *)p_context;
    
    // MM-2014-07-31: [[ ThreadedRendering ]] Updated to use the new stack surface API.
	p_surface -> UnlockPixels(MCRectangleToMCGIntegerRectangle(self -> dirty), self-> raster);
	
	return true;
}

bool MCTileCacheSoftwareCompositor_BeginLayer(void *p_context, const MCRectangle& p_clip, uint32_t p_opacity, uint32_t p_ink)
{
	MCTileCacheSoftwareCompositorContext *self;
	self = (MCTileCacheSoftwareCompositorContext *)p_context;
	
	self -> clip = MCU_intersect_rect(self -> dirty, p_clip);
	self -> opacity = p_opacity;
	self -> combiner = s_surface_combiners_nda[p_ink];

	return true;
}

bool MCTileCacheSoftwareCompositor_EndLayer(void *p_context)
{
	MCTileCacheSoftwareCompositorContext *self;
	self = (MCTileCacheSoftwareCompositorContext *)p_context;

	self -> clip = self -> dirty;
	self -> opacity = 255;
	self -> combiner = s_surface_combiners_nda[GXcopy];

	return true;
}

bool MCTileCacheSoftwareCompositor_CompositeTile(void *p_context, int32_t p_x, int32_t p_y, void *p_tile)
{
	MCTileCacheSoftwareCompositorContext *self;
	self = (MCTileCacheSoftwareCompositorContext *)p_context;

	MCRectangle t_dst_rect;
	t_dst_rect . x = p_x;
	t_dst_rect . y = p_y;
	t_dst_rect . width = self -> tile_size;
	t_dst_rect . height = self -> tile_size;
	t_dst_rect = MCU_intersect_rect(t_dst_rect, self -> clip);

	MCRectangle t_src_rect;
	t_src_rect = MCU_offset_rect(t_dst_rect, -p_x, -p_y);

	void *t_dst_ptr, *t_src_ptr;
	t_dst_ptr = (uint8_t *)self -> raster . pixels + self -> raster . stride * (t_dst_rect . y - self -> dirty . y) + (t_dst_rect . x - self -> dirty . x) * sizeof(uint32_t);
	t_src_ptr = (uint32_t *)p_tile + self -> tile_size * t_src_rect . y + t_src_rect . x;

	self -> combiner(t_dst_ptr, self -> raster . stride, t_src_ptr, self -> tile_size * sizeof(uint32_t), t_src_rect . width, t_src_rect . height, self -> opacity);

	return true;
}

bool MCTileCacheSoftwareCompositor_CompositeRect(void *p_context, int32_t p_x, int32_t p_y, uint32_t p_color)
{
	MCTileCacheSoftwareCompositorContext *self;
	self = (MCTileCacheSoftwareCompositorContext *)p_context;
	
	if (self -> tile_row == nil &&
		!MCMemoryAllocate(self -> tile_size * sizeof(uint32_t), self -> tile_row))
		return false;
		
	if (self -> tile_row_color != p_color)
	{
		for(int32_t i = 0; i < self -> tile_size; i++)
			self -> tile_row[i] = p_color;
		self -> tile_row_color = p_color;
	}
			
	MCRectangle t_dst_rect;
	t_dst_rect . x = p_x;
	t_dst_rect . y = p_y;
	t_dst_rect . width = self -> tile_size;
	t_dst_rect . height = self -> tile_size;
	t_dst_rect = MCU_intersect_rect(t_dst_rect, self -> clip);

    // MM-2014-07-31: [[ ThreadedRendering ]] We now wrap the bits in a raster.
	void *t_dst_ptr;
	t_dst_ptr = (uint8_t *)self -> raster . pixels + self -> raster . stride * (t_dst_rect . y - self -> dirty . y) + (t_dst_rect . x - self -> dirty . x) * sizeof(uint32_t);
	
	for(uint32_t y = 0; y < t_dst_rect . height; y++)
		self -> combiner((uint8_t *)t_dst_ptr + y * self -> raster . stride, self -> raster . stride, self -> tile_row, self -> tile_size * sizeof(uint32_t), t_dst_rect . width, 1, self -> opacity);

	return true;
}

void MCTileCacheSoftwareCompositor_Cleanup(void *p_context)
{
	MCTileCacheSoftwareCompositorContext *self;
	self = (MCTileCacheSoftwareCompositorContext *)p_context;
	MCMemoryDeallocate(self -> tile_row);
	MCMemoryDelete(self);
}

////////////////////////////////////////////////////////////////////////////////

bool MCTileCacheSoftwareCompositorConfigure(MCTileCacheRef p_tilecache, MCTileCacheCompositor& r_compositor)
{
	MCTileCacheSoftwareCompositorContext *t_context;
	if (!MCMemoryNew(t_context))
		return false;
	
	t_context -> tilecache = p_tilecache;
	
	r_compositor . context = t_context;
	r_compositor . cleanup = MCTileCacheSoftwareCompositor_Cleanup;
	r_compositor . allocate_tile = MCTileCacheSoftwareCompositor_AllocateTile;
	r_compositor . deallocate_tile = MCTileCacheSoftwareCompositor_DeallocateTile;
	r_compositor . begin_frame = MCTileCacheSoftwareCompositor_BeginFrame;
	r_compositor . end_frame = MCTileCacheSoftwareCompositor_EndFrame;
	r_compositor . begin_layer = MCTileCacheSoftwareCompositor_BeginLayer;
	r_compositor . end_layer = MCTileCacheSoftwareCompositor_EndLayer;
	r_compositor . composite_tile = MCTileCacheSoftwareCompositor_CompositeTile;
	r_compositor . composite_rect = MCTileCacheSoftwareCompositor_CompositeRect;
	
	return true;
}

////////////////////////////////////////////////////////////////////////////////
