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

#ifndef __MC_TILE_CACHE__
#define __MC_TILE_CACHE__

// The tilecache opaque handle.
typedef struct MCTileCache *MCTileCacheRef;

// The callback type required to render layers.
typedef bool (*MCTileCacheRenderCallback)(void *context, MCGContextRef target, const MCRectangle32& region);

// The compositor cleanup callback
typedef void (*MCTileCacheCleanupCallback)(void *context);
// The compositor flush callback.
typedef void (*MCTileCacheFlushCallback)(void *context);
// Construction of tiles is about to begin.
typedef bool (*MCTileCacheBeginTilingCallback)(void *context);
// Construction of tiles is about to end.
typedef bool (*MCTileCacheEndTilingCallback)(void *context);
// The tile allocation compositor callback.
typedef bool (*MCTileCacheAllocateTileCallback)(void *context, int32_t size, const void *bits, uint32_t stride, void*& r_tile);
// The tile deallocation compositor callback.
typedef void (*MCTileCacheDeallocateTileCallback)(void *context, void *tile);
// Compositing of the current frame has begun.
typedef bool (*MCTileCacheBeginFrameCallback)(void *context, MCStackSurface *surface, MCGRegionRef dirty);
// Compositing of the current frame has ended.
typedef bool (*MCTileCacheEndFrameCallback)(void *context, MCStackSurface *surface);
// Compositing of a layer has begun (layers are never nested).
typedef bool (*MCTileCacheBeginLayerCallback)(void *context, const MCRectangle& clip, uint32_t opacity, uint32_t ink);
// Compositing of a layer has ended.
typedef bool (*MCTileCacheEndLayerCallback)(void *context);
// Composite the tile at the given location.
typedef bool (*MCTileCacheCompositeTileCallback)(void *context, int32_t x, int32_t y, void *tile);
// Composite a tile-sized rectangle at the given location.
typedef bool (*MCTileCacheCompositeRectCallback)(void *context, int32_t x, int32_t y, uint32_t color);
// Snapshot of the current frame has begun.
typedef bool (*MCTileCacheBeginSnapshotCallback)(void *context, MCRectangle area, MCGRaster &target);
// Snapshot of the current frame has ended.
typedef bool (*MCTileCacheEndSnapshotCallback)(void *context, MCRectangle area, MCGRaster &target);

// The compositor description structure.
struct MCTileCacheCompositor
{
	void *context;
	MCTileCacheCleanupCallback cleanup;
	MCTileCacheFlushCallback flush;
	MCTileCacheBeginTilingCallback begin_tiling;
	MCTileCacheEndTilingCallback end_tiling;
	MCTileCacheAllocateTileCallback allocate_tile;
	MCTileCacheDeallocateTileCallback deallocate_tile;
	MCTileCacheBeginFrameCallback begin_frame;
	MCTileCacheEndFrameCallback end_frame;
	MCTileCacheBeginLayerCallback begin_layer;
	MCTileCacheEndLayerCallback end_layer;
	MCTileCacheCompositeTileCallback composite_tile;
	MCTileCacheCompositeRectCallback composite_rect;
	MCTileCacheBeginSnapshotCallback begin_snapshot;
	MCTileCacheEndSnapshotCallback end_snapshot;
};

// The types of compositor that are supported.
enum MCTileCacheCompositorType
{
	kMCTileCacheCompositorNone,
	kMCTileCacheCompositorSoftware,
	kMCTileCacheCompositorCoreGraphics,
	kMCTileCacheCompositorOpenGL,
	kMCTileCacheCompositorStaticOpenGL,
	kMCTileCacheCompositorDynamicOpenGL
};

// The layer description structure.
// IM-2014-02-28: [[ Bug 11617 ]] Update tilecache layer to use 32-bit rectangles
struct MCTileCacheLayer
{
	// The previous id of the layer (or zero if new).
	uint32_t id;
	// The region the layer touches.
	MCRectangle32 region;
	// This should be true if the layer is completely opaque in region.
	bool is_opaque;
	// The clipping rectangle to apply to the layer.
	MCRectangle32 clip;
	// The opacity of the layer (0 transparent, 255 solid).
	uint32_t opacity;
	// The ink to use when compositing the layer.
	uint32_t ink;
	// The rendering callback to use to render the layer.
	MCTileCacheRenderCallback callback;
	// The render callback context ptr.
	void *context;
};

// Create a tilecache using the given tile size and cache limit.
bool MCTileCacheCreate(int32_t tile_size, uint32_t cache_limit, MCTileCacheRef& r_tilecache);
// Destroy the given tilecache.
void MCTileCacheDestroy(MCTileCacheRef self);

// Activates the tilecache that was previously deactivated.
void MCTileCacheActivate(MCTileCacheRef self);
// Deactivates a tilecache.
void MCTileCacheDeactivate(MCTileCacheRef self);

// Returns true if the tilecache mechanism on this platform supports the given
// compositor type.
bool MCTileCacheSupportsCompositor(MCTileCacheCompositorType type);

// Check to see if the tile cache is valid. It can become invalid if a memory
// allocation error occurs while performing any method. An invalid cache must
// be flushed before it can be used again.
bool MCTileCacheIsValid(MCTileCacheRef self);
// Check to see if the tilecache is clean - i.e. has just been flushed.
bool MCTileCacheIsClean(MCTileCacheRef self);

// Configure the cache limit of the tilecache.
void MCTileCacheSetCacheLimit(MCTileCacheRef self, uint32_t new_cachelimit);
// Fetch the cache limit of the tilecache.
uint32_t MCTileCacheGetCacheLimit(MCTileCacheRef self);

// Set the tile size of the tilecache.
void MCTileCacheSetTileSize(MCTileCacheRef self, uint32_t new_tilesize);
// Fetch the tile size of the tilecache
int32_t MCTileCacheGetTileSize(MCTileCacheRef self);

// Change the viewport of the tilecache. The tiling origin of the scenery tiles
// is taken to be the top-left of the viewport.
void MCTileCacheSetViewport(MCTileCacheRef self, const MCRectangle& new_viewport);
// Fetch the viewport of the tilecache.
MCRectangle MCTileCacheGetViewport(MCTileCacheRef self);

// Set the compositor of the tilecache.
void MCTileCacheSetCompositor(MCTileCacheRef self, MCTileCacheCompositorType type);
// Fetch the type of compositor of the tilecache.
MCTileCacheCompositorType MCTileCacheGetCompositor(MCTileCacheRef self);

// Flush all cached tiles and sprites from the tilecache.
void MCTileCacheFlush(MCTileCacheRef self);
// Minimize the amount of memory used by the tilecache.
void MCTileCacheCompact(MCTileCacheRef self);

// A scenery layer has been inserted before the given layer touching the given region.
void MCTileCacheInsertScenery(MCTileCacheRef self, uint32_t before_layer, const MCRectangle32& region);
// The given scenery layer touching the given region has been removed.
void MCTileCacheRemoveScenery(MCTileCacheRef self, uint32_t layer, const MCRectangle32& region);
// The given scenery layer has been reshaped from the old region to the new region.
void MCTileCacheReshapeScenery(MCTileCacheRef self, uint32_t layer, const MCRectangle32& old_region, const MCRectangle32& new_region);
// The given scenery layer has changed within the given region.
void MCTileCacheUpdateScenery(MCTileCacheRef self, uint32_t layer, const MCRectangle32& region);

// The given sprite has been removed.
void MCTileCacheRemoveSprite(MCTileCacheRef self, uint32_t id);
// The given sprite has changed within the given region.
void MCTileCacheUpdateSprite(MCTileCacheRef self, uint32_t id, const MCRectangle32& region);
// The given sprite's cached tiles are scrolled by the given amount.
void MCTileCacheScrollSprite(MCTileCacheRef self, uint32_t id, int32_t dx, int32_t dy);

// Start processing a new frame.
void MCTileCacheBeginFrame(MCTileCacheRef self);
// Finish processing the current frame, generating any required tiles.
void MCTileCacheEndFrame(MCTileCacheRef self);

// Render a scenery layer with the given parameters into the current frame.
void MCTileCacheRenderScenery(MCTileCacheRef self, MCTileCacheLayer& layer);
// Render a sprite layer with the given parameters into the current frame.
void MCTileCacheRenderSprite(MCTileCacheRef self, MCTileCacheLayer& layer);
// Render a direct sprite layer with the given parameters into the current frame.
void MCTileCacheRenderDirectSprite(MCTileCacheRef self, MCTileCacheLayer& layer, const void *color_bits, const void *alpha_bits);

// Composite the current frame onto the given surface.
bool MCTileCacheComposite(MCTileCacheRef self, MCStackSurface *surface, MCGRegionRef region);

// Render the current frame into an offscreen buffer.
bool MCTileCacheSnapshot(MCTileCacheRef self, MCRectangle area, MCGImageRef& r_pixmap);

// Configure the tilecache to use the software compositor.
bool MCTileCacheSoftwareCompositorConfigure(MCTileCacheRef p_tilecache, MCTileCacheCompositor& compositor);
// Configure the tilecache to use the software compositor.
bool MCTileCacheCoreGraphicsCompositorConfigure(MCTileCacheRef p_tilecache, MCTileCacheCompositor& compositor);
// Configure the tilecache to use the opengl compositor.
bool MCTileCacheOpenGLCompositorConfigure(MCTileCacheRef p_tilecache, MCTileCacheCompositor& compositor);

#endif
