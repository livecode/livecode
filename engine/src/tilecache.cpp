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

#include "globals.h"
#include "util.h"
#include "uidc.h"
#include "context.h"
#include "tilecache.h"
#include "region.h"

#include "graphicscontext.h"
#include "graphics_util.h"

#ifdef _HAS_QSORT_R
#define stdc_qsort(a, b, c, d, e) qsort_r(a, b, c, e, d)
#elif defined(_HAS_QSORT_S)
#define stdc_qsort(a, b, c, d, e) qsort_s(a, b, c, d, e)
#else
static void *s_stdc_qsort_thunk;
static int (*s_stdc_qsort_compar)(void *, const void *, const void *);
static int stdc_qsort_thunking_compar(const void *x, const void *y)
{
	return s_stdc_qsort_compar(s_stdc_qsort_thunk, x, y);
}
inline void stdc_qsort(void *base, size_t nel, size_t width, int (*compar)(void *, const void *, const void *), void *thunk)
{
	s_stdc_qsort_thunk = thunk;
	s_stdc_qsort_compar = compar;
	qsort(base, nel, width, stdc_qsort_thunking_compar);
}
#endif

////////////////////////////////////////////////////////////////////////////////

struct MCTileCacheRectangle
{
	int32_t left;
	int32_t top;
	int32_t right;
	int32_t bottom;
};

struct MCTileCacheCell
{
	// The list of scenery tiles cached for this location.
	uint16_t *tiles;
	// The length of the tiles array.
	uint32_t tile_count;
};

struct MCTileCacheRenderer
{
	MCTileCacheRenderCallback callback;
	void *context;
};

struct MCTileCacheSprite
{
	// The render callback for the sprite.
	MCTileCacheRenderer renderer;
	// The offset from the top-left of the sprite contents to (0, 0) in tile co-ords.
	int32_t xorg, yorg;
	// The maximum extent of the tiles array, in tile co-ords.
	uint8_t left, top, right, bottom;
	// The 2d array of tiles currently cached for this sprite. An index of 0
	// means an uncached location.
	uint16_t *tiles;
};

struct MCTileCacheTileList
{
	uint16_t first;
	uint16_t last;
};

struct MCTileCacheTile
{
	// The layers covered by the tile. For sprites, last_layer will be zero.
	uint16_t first_layer, last_layer;
	// The link for the list the tile is in, whether 'used' or 'empty'.
	uint16_t next, previous;
	// The tile co-ords of the tile in the sprite/scenery.
	uint8_t x, y;
	// This byte is an OR of the alpha bytes of the tile. Thus 0 is fully transparent,
	// 255 is fully opaque. It's used to assist with occlusion and to save us storing
	// fully transparent tiles.
	uint8_t alpha;
	// If constant is 1, then 'data' is a pixel value.
	uint8_t constant;
	// The cached data for the tile. If this is nil then it means the tile is
	// completely transparent.
	void *data;
};

struct MCTileCacheRenderList
{
	uint16_t *contents;
	uint32_t length;
	uint32_t capacity;
};

struct MCTileCacheFrontier
{
	uint16_t first_layer;
	uint16_t last_layer;
	uint16_t old_first_layer;
	uint16_t old_last_layer;
};

struct MCTileCache
{
	// If false, an error has occurred while processing an operation on the
	// tilecache. It must be flushed before it can be used again.
	bool valid : 1;
	
	// If true, the tilecache has just been flushed and thus any id's passed in
	// to it should be considered invalid.
	bool clean : 1;
	
	// The size of a single tile in pixels (tiles are square).
	// MDW-2013-04-16: [[ x64 ]] no need for this to be a signed int, messed up comparisons
	uint32_t tile_size;
	
	// The number of bytes currently in use by cached images.
	uint32_t cache_size;
	// The maximum number of bytes to use for caching tile images.
	uint32_t cache_limit;
	
	// The type of compositor.
	MCTileCacheCompositorType compositor_type;
	// The compositor callbacks to use.
	MCTileCacheCompositor compositor;

	// The viewport of the tilecache. The top-left of this rect is the tiling
	// origin.
	MCRectangle viewport;
	
	// The size of the viewport of the the tilecache in tiles.
	int32_t tiles_across, tiles_down;
	
	// A 2d array of cells, listing the tiles for scenery layers at that location
	// which are currently in the cache.
	MCTileCacheCell *cells;
	
	// The array of currently active sprites.
	MCTileCacheSprite *sprites;
	uint32_t sprite_count;
	
	// The array of tiles.
	MCTileCacheTile *tiles;
	// The number of touched tiles - the size of empty + used lists.
	uint32_t tile_count;
	// The number of active tiles in the current frame.
	uint32_t active_tile_count;
	// The maximum number of active tiles to allow.
	uint32_t tile_limit;
	// The total capacity of the tiles array.
	uint32_t tile_capacity;
	// The list of tiles that contain cached images.
	MCTileCacheTileList used_tiles;
	// The list of tiles that are in use, but should be destroyed next frame.
	MCTileCacheTileList dirty_tiles;
	// The list of tiles that are empty and can be recycled.
	MCTileCacheTileList empty_tiles;
	// The first tile in the used list which is not used in the current frame.
	uint32_t inactive_tile_index;
	
	// The display list for the current frame.
	uint16_t *display_list;
	uint32_t display_list_frontier;
	uint32_t display_list_capacity;
	
	// The scenery id mapping from the last frame to the new frame.
	uint16_t *scenery_map;
	// The total capacity of the scenery map array.
	uint32_t scenery_map_capacity;
	
	// The list of renderers mapped by scenery layer id.
	MCTileCacheRenderer *scenery_renderers;
	// The id of the last new scenery layer that was rendered.
	uint32_t scenery_renderers_frontier;
	// The total capacity of the scenery renderer array.
	uint32_t scenery_renderers_capacity;
	
	// The list of scenery tiles to render for the current frame.
	MCTileCacheRenderList scenery_render_list;
	
	// The list of sprite tiles to render for the current frame.
	MCTileCacheRenderList sprite_render_list;
	
	// The 2d array of tile frontiers, used to calculate which tiles to render.
	MCTileCacheFrontier *frontiers;
	
	// The temporary tile (used during tiling).
	void *temporary_tile;
};

////////////////////////////////////////////////////////////////////////////////

// Sets up the compositor hooks appropriately.
static void MCTileCacheConfigureCompositor(MCTileCacheRef self, MCTileCacheCompositorType type);

// Attempt to allocate a new tile to be filled later. If there is no room then
// 'false' is returned and the cache is marked invalid.
static bool MCTileCacheCreateTile(MCTileCacheRef self, uint32_t& r_tile);
// Deallocate the given tile, placing it on the empty list if 'put on empty' is
// true. Note that this call assumes the tile has been dereferenced from its
// corresponding cell/sprite.
static void MCTileCacheDestroyTile(MCTileCacheRef self, uint32_t index, bool put_on_empty);
// Flush the given tile, placing it on the empty tile if 'put on empty' is true.
// Note this call will remove any references to the tile before destroying it.
static void MCTileCacheFlushTile(MCTileCacheRef self, uint32_t index, bool put_on_empty);
// Put the given tile on the dirty list, to be destroyed at the next opportunity.
static void MCTileCacheDirtyTile(MCTileCacheRef self, uint32_t index);
// Mark the tile as in-use, i.e. make sure it isn't flushed during this frame.
static void MCTileCacheTouchTile(MCTileCacheRef self, uint32_t index);
// Fill the given tile with an image from context.
static void MCTileCacheFillTile(MCTileCacheRef self, uint32_t index, MCImageBitmap *bitmap, int32_t x, int32_t y);
// Empty the tile's image (if any).
static void MCTileCacheEmptyTile(MCTileCacheRef self, uint32_t index);

static void MCTileCacheRenderListReset(MCTileCacheRef self, MCTileCacheRenderList& x_list);
static void MCTileCacheRenderListDestroy(MCTileCacheRef self, MCTileCacheRenderList& x_list);
static void MCTileCacheRenderListPush(MCTileCacheRef self, MCTileCacheRenderList& x_list, uint32_t tile);

static void MCTileCacheRenderSpriteTiles(MCTileCacheRef self);
static void MCTileCacheRenderSceneryTiles(MCTileCacheRef self);

////////////////////////////////////////////////////////////////////////////////
//
//  INLINE ACCESSORS
//

// Return the tile ptr for the given tile index.
static inline MCTileCacheTile *MCTileCacheGetTile(MCTileCacheRef self, uint32_t p_index)
{
	return &self -> tiles[p_index];
}

// Return the scenery cell ptr at the given location.
static inline MCTileCacheCell *MCTileCacheGetSceneryCell(MCTileCacheRef self, int32_t x, int32_t y)
{
	return &self -> cells[y * self -> tiles_across + x];
}

// Return the sprite ptr for the given sprite index.
static inline MCTileCacheSprite *MCTileCacheGetSprite(MCTileCacheRef self, uint32_t p_index)
{
	return &self -> sprites[p_index - 1];
}

// Return the sprite cell ptr (tile index ptr) for the given location.
static inline uint16_t *MCTileCacheGetSpriteCell(MCTileCacheRef self, uint32_t p_id, int32_t x, int32_t y)
{
	MCTileCacheSprite *t_sprite;
	t_sprite = MCTileCacheGetSprite(self, p_id);
	return &t_sprite -> tiles[(y - t_sprite -> top) * (t_sprite -> right - t_sprite -> left) + (x - t_sprite -> left)];
}

////////////////////////////////////////////////////////////////////////////////
//
//  INLINE COMPUTATIONS
//

// This computes the index of the nearest tile boundary less than z. We have
// to be careful because of the definition of '/' when the numerator is -ve.
static inline int32_t MCTileCacheTileFloor(MCTileCacheRef self, int32_t z)
{
	// If positive we want the floor.
	if (z >= 0)
		return z / self -> tile_size;

	// If negative, take the positive ceiling and negate.
	return -(((-z) + self -> tile_size - 1) / self -> tile_size);
}

// This computes the index of the nearest tile boundary greater than z. We have
// to be careful because of the definition of '/' when the numerator is -ve.
static inline int32_t MCTileCacheTileCeiling(MCTileCacheRef self, int32_t z)
{
	// If positive we want the ceiling.
	if (z >= 0)
		return (z + self -> tile_size - 1) / self -> tile_size;

	// If negative, we take the positive floor and negate.
	return -((-z) / self -> tile_size);
}

////////////////////////////////////////////////////////////////////////////////
//
//  TILE LIST MANIPULATORS
//

// Remove the given tile from the list.
static inline void MCTileCacheTileListRemove(MCTileCacheRef self, MCTileCacheTileList& x_list, uint32_t p_index)
{
	MCTileCacheTile *t_tile;
	t_tile = &self -> tiles[p_index];

	if (t_tile -> next != 0)
		self -> tiles[t_tile -> next] . previous = t_tile -> previous;
	else
		x_list . last = t_tile -> previous;

	if (t_tile -> previous != 0)
		self -> tiles[t_tile -> previous] . next = t_tile -> next;
	else
		x_list . first = t_tile -> next;

	t_tile -> next = t_tile -> previous = 0;
}

// Remove the last tile from the list.
static inline uint32_t MCTileCacheTileListPop(MCTileCacheRef self, MCTileCacheTileList& x_list)
{
	// If the list is empty, return.
	if (x_list . last == 0)
		return 0;

	// Otherwise remove the last entry from the list.
	uint32_t t_tile_id;
	t_tile_id = x_list . last;
	MCTileCacheTileListRemove(self, x_list, t_tile_id);

	return t_tile_id;
}

// Pust the given tile onto the front of the list.
static inline void MCTileCacheTileListPush(MCTileCacheRef self, MCTileCacheTileList& x_list, uint32_t p_index)
{
	if (x_list . first != 0)
	{
		self -> tiles[p_index] . next = x_list . first;
		self -> tiles[x_list . first] . previous = p_index;
		x_list . first = p_index;
	}
	else
		x_list . first = x_list . last = p_index;
}

////////////////////////////////////////////////////////////////////////////////

bool MCTileCacheCreate(int32_t p_tile_size, uint32_t p_cache_limit, MCTileCacheRef& r_tilecache)
{
	MCTileCacheRef self;
	self = nil;
	if (!MCMemoryNew(self))
		return false;

	// The cache starts life as valid.
	self -> valid = true;
	// Configure the size of the tiles.
	self -> tile_size = p_tile_size;
	// The maximum number of tiles we support is 65536 at the moment.
	self -> tile_limit = 65536;
	// Set the initial limit on memory to use for images.
	self -> cache_limit = p_cache_limit;

	// Return the array (everything else will be 0).
	r_tilecache = self;

	return true;
}

void MCTileCacheDestroy(MCTileCacheRef self)
{
	if (self == nil)
		return;

	// Flush everything. This deletes all cached image data as well as freeing
	// any ancilliary arrays used by sprites.
	MCTileCacheFlush(self);
	
	// Cleanup the compositor (if any)
	if (self -> compositor . cleanup != nil)
		self -> compositor . cleanup(self -> compositor . context);
	
	// Now delete the arrays themselves.
	MCMemoryDeleteArray(self -> cells);
	MCMemoryDeleteArray(self -> sprites);
	MCMemoryDeleteArray(self -> tiles);
	MCMemoryDeleteArray(self -> scenery_map);
	MCMemoryDeleteArray(self -> scenery_renderers);
	MCMemoryDeleteArray(self -> display_list);
	MCTileCacheRenderListDestroy(self, self -> scenery_render_list);
	MCTileCacheRenderListDestroy(self, self -> sprite_render_list);
	
	// Finally the state structure.
	MCMemoryDelete(self);
}

void MCTileCacheActivate(MCTileCacheRef self)
{	
	MCTileCacheConfigureCompositor(self, self -> compositor_type);
	
	self -> valid = true;
}

void MCTileCacheDeactivate(MCTileCacheRef self)
{
	MCTileCacheFlush(self);
	
	MCTileCacheConfigureCompositor(self, kMCTileCacheCompositorNone);
	
	self -> valid = false;
}

bool MCTileCacheIsValid(MCTileCacheRef self)
{
	return self -> valid;
}

bool MCTileCacheIsClean(MCTileCacheRef self)
{
	return self -> clean;
}

void MCTileCacheInvalidate(MCTileCacheRef self)
{
	self -> valid = false;
}

void MCTileCacheSetCacheLimit(MCTileCacheRef self, uint32_t p_new_cache_limit)
{
	// If the new cache limit is less than the current usage, we need to flush.
	if (self -> cache_size > p_new_cache_limit)
		MCTileCacheFlush(self);
	
	// Set the new cache limit.
	self -> cache_limit = p_new_cache_limit;
}

uint32_t MCTileCacheGetCacheLimit(MCTileCacheRef self)
{
	return self -> cache_limit;
}

void MCTileCacheSetTileSize(MCTileCacheRef self, uint32_t p_new_tile_size)
{
	// If the new tile size is different from the old, we need to flush.
	if (self -> tile_size != p_new_tile_size)
		MCTileCacheFlush(self);
		
	// Set the new tile size.
	self -> tile_size = p_new_tile_size;
	
	// Update the viewport - need to do this to recompute across/down (this
	// is a little messy...)
	MCRectangle t_old_viewport;
	t_old_viewport = self -> viewport;
	MCTileCacheSetViewport(self, MCU_make_rect(0, 0, 0, 0));
	MCTileCacheSetViewport(self, t_old_viewport);
}

int32_t MCTileCacheGetTileSize(MCTileCacheRef self)
{
	return self -> tile_size;
}

void MCTileCacheSetCompositor(MCTileCacheRef self, MCTileCacheCompositorType p_type)
{
	// Changing compositor always requires a flush.
	MCTileCacheFlush(self);
	
	// Store the type of compositor we are using.
	self -> compositor_type = p_type;
	
	// Configure it.
	MCTileCacheConfigureCompositor(self, self -> compositor_type);
}

MCTileCacheCompositorType MCTileCacheGetCompositor(MCTileCacheRef self)
{
	return self -> compositor_type;
}

bool MCTileCacheSupportsCompositor(MCTileCacheCompositorType p_type)
{
	if (p_type == kMCTileCacheCompositorNone ||
		p_type == kMCTileCacheCompositorSoftware)
		return true;
	
#if defined(_IOS_MOBILE) || defined(_MAC_DESKTOP)
	if (p_type == kMCTileCacheCompositorCoreGraphics)
		return true;
#endif
	
#if defined(_IOS_MOBILE) || defined(TARGET_SUBPLATFORM_ANDROID)
	if (p_type == kMCTileCacheCompositorStaticOpenGL)
		return true;
#endif
	
	return false;
}

static void MCTileCacheConfigureCompositor(MCTileCacheRef self, MCTileCacheCompositorType p_type)
{
	// Cleanup the old compositor, if any.
	if (self -> compositor . cleanup != nil)
		self -> compositor . cleanup(self -> compositor . context);
	
	// Empty the hooks.
	MCMemoryClear(&self -> compositor, sizeof(MCTileCacheCompositor));
	
	// Now call the compositor config function.
	switch(p_type)
	{
		case kMCTileCacheCompositorSoftware:
			MCTileCacheSoftwareCompositorConfigure(self, self -> compositor);
			break;
#if defined(_IOS_MOBILE) || defined(_MAC_DESKTOP)
		case kMCTileCacheCompositorCoreGraphics:
			MCTileCacheCoreGraphicsCompositorConfigure(self, self -> compositor);
			break;
#endif
#if defined(_IOS_MOBILE) || defined(TARGET_SUBPLATFORM_ANDROID)
		case kMCTileCacheCompositorStaticOpenGL:
			MCTileCacheOpenGLCompositorConfigure(self, self -> compositor);
			break;
#endif
		default:
			break;
	}
}

////////////////////////////////////////////////////////////////////////////////

void MCTileCacheSetViewport(MCTileCacheRef self, const MCRectangle& p_new_viewport)
{
	// Setting the viewport flushes if not valid.
	if (!self -> valid)
		MCTileCacheFlush(self);

	// If the viewport is the same, then do nothing.
	if (MCU_equal_rect(self -> viewport, p_new_viewport))
		return;

	// Compute the new size of the canvas in tiles.
	int32_t t_new_tiles_across, t_new_tiles_down;
	t_new_tiles_across = MCTileCacheTileCeiling(self, p_new_viewport . width);
	t_new_tiles_down = MCTileCacheTileCeiling(self, p_new_viewport . height);

	// Now try to allocate a new cells array.
	MCTileCacheCell *t_new_cells;
	if (!MCMemoryNewArray(t_new_tiles_across * t_new_tiles_down, t_new_cells))
	{
		MCTileCacheInvalidate(self);
		return;
	}
		
	// If the top-left of the bounds is the same, we can move over any old cells
	// (this could be improved, if the top-left has moved by a mulitple of the
	// tile size).
	if (self -> viewport . x == p_new_viewport . x && self -> viewport . y == p_new_viewport . y)
	{
		// As we must only copy across full tiles, we need to calculate the old
		// number of complete tiles across and down.
		int32_t t_old_tiles_across, t_old_tiles_down;
		t_old_tiles_across = MCTileCacheTileFloor(self, self -> viewport . width);
		t_old_tiles_down = MCTileCacheTileFloor(self, self -> viewport . height);
		
		// Now copy across the intesecting cells, clearing out the old ones.
		for(int32_t y = 0; y < MCMin(t_old_tiles_down, t_new_tiles_down); y++)
			for(int32_t x = 0; x < MCMin(t_old_tiles_across, t_new_tiles_across); x++)
			{
				// Get a ptr to the current scenery cell.
				MCTileCacheCell *t_cell;
				t_cell = MCTileCacheGetSceneryCell(self, x, y);
				
				// Copy across the old cell.
				t_new_cells[y * t_new_tiles_across + x] = *t_cell;
				
				// Clear out the old cell.
				t_cell -> tile_count = 0;
				t_cell -> tiles = nil;
			}
	}
	
	// We've cleared out the cells we've utilized, so now flush all the remaining
	// cells in the old array.
	for(int32_t y = 0; y < self -> tiles_down; y++)
		for(int32_t x = 0; x < self -> tiles_across; x++)
		{
			MCTileCacheCell *t_cell;
			t_cell = MCTileCacheGetSceneryCell(self, x, y);
			
			// Dirty any tiles that are in the cells list - these get destroyed
			// at next start of frame.
			for(uint32_t i = 0; i < t_cell -> tile_count; i++)
				MCTileCacheDirtyTile(self, t_cell -> tiles[i]);
			
			// Delete the tiles array.
			MCMemoryDeleteArray(t_cell -> tiles);
		}
		
	// Delete the old cells array.
	MCMemoryDeleteArray(self -> cells);
	
	// Update the viewport and new cells array.
	self -> viewport = p_new_viewport;
	self -> cells = t_new_cells;
	self -> tiles_across = t_new_tiles_across;
	self -> tiles_down = t_new_tiles_down;
}

MCRectangle MCTileCacheGetViewport(MCTileCacheRef self)
{
	return self -> viewport;
}

////////////////////////////////////////////////////////////////////////////////

void MCTileCacheFlush(MCTileCacheRef self)
{
	// First free all the tile lists in each cell.
	for(int32_t y = 0; y < self -> tiles_down; y++)
		for(int32_t x = 0; x < self -> tiles_across; x++)
			MCMemoryDeleteArray(MCTileCacheGetSceneryCell(self, x, y) -> tiles);
			
	// Now free all the sprite tile arrays and reset them.
	for(uint32_t i = 0; i < self -> sprite_count; i++)
		MCMemoryDeleteArray(self -> sprites[i] . tiles);
		
	// Now destroy all the tiles (if a tile is unused, DestroyTile is just a
	// no-op).
	for(uint32_t i = 0; i < self -> tile_count; i++)
		MCTileCacheDestroyTile(self, i, false);

	// If the compositor has a flush method, use it.
	if (self -> compositor . flush != nil)
		self -> compositor . flush(self -> compositor . context);
	
	// Clear the cells and sprites.
	memset(self -> cells, 0, self -> tiles_across * self -> tiles_down * sizeof(MCTileCacheCell));
	memset(self -> sprites, 0, self -> sprite_count * sizeof(MCTileCacheSprite));
	memset(self -> tiles, 0, self -> tile_count * sizeof(MCTileCacheTile));
	
	// Make sure the tile lists are empty.
	self -> empty_tiles . first = 0;
	self -> empty_tiles . last = 0;
	self -> dirty_tiles . first = 0;
	self -> dirty_tiles . last = 0;
	self -> used_tiles . first = 0;
	self -> used_tiles . last = 0;
	self -> inactive_tile_index = 0;
	
	// Reset the display list frontier.
	self -> display_list_frontier = 0;
	
	// The tile count always starts off at 1 since 0 is the unused tile index.
	if (self -> tile_capacity > 0)
		self -> tile_count = 1;
		
	// After flushing, the tilecache becomes valid again.
	self -> valid = true;
	
	// Mark the tilecache as being clean, this causes ids to be reallocated.
	self -> clean = true;
}

void MCTileCacheCompact(MCTileCacheRef self)
{
	// Flush all inactive tiles.
	while(self -> inactive_tile_index != 0)
		MCTileCacheFlushTile(self, self -> used_tiles . last, true);
	
	// If the compositor has a flush method, use it.
	if (self -> compositor . flush != nil)
		self -> compositor . flush(self -> compositor . context);
}

////////////////////////////////////////////////////////////////////////////////

static bool MCTileCacheCreateTile(MCTileCacheRef self, uint32_t& r_tile)
{
	// First see if we can pop a tile from the empty list.
	uint32_t t_tile_id;
	t_tile_id = MCTileCacheTileListPop(self, self -> empty_tiles);

	// If that fails, see if there is room to allocate one already.
	if (t_tile_id == 0 &&
		self -> tile_count < self -> tile_capacity)
		t_tile_id = self -> tile_count++;

	// If that fails, see if we can resize the tile array.
	if (t_tile_id == 0)
	{
		if (self -> tile_count == self -> tile_limit ||
			!MCMemoryResizeArray((self -> tile_capacity != 0 ? self -> tile_capacity * 2 : 4), self -> tiles, self -> tile_capacity))
		{
			MCTileCacheInvalidate(self);
			return false;
		}

		if (self -> tile_count == 0)
			self -> tile_count = 1;

		t_tile_id = self -> tile_count++;
	}

	// If that fails, see if we can pop one from the inactive list.
	if (t_tile_id == 0 &&
		self -> inactive_tile_index != 0)
	{
		t_tile_id = self -> used_tiles . last;
		MCTileCacheFlushTile(self, t_tile_id, false);
	}

	// We have a tile (yay!) so place it on the active tiles list.
	MCTileCacheTileListPush(self, self -> used_tiles, t_tile_id);
	// Increment the active tile count.
	self -> active_tile_count += 1;

	r_tile = t_tile_id;

	return true;
}

static void MCTileCacheTouchTile(MCTileCacheRef self, uint32_t p_index)
{
	// If the tile is the head of the inactive tiles, then adjust the inactive
	// head index.
	if (self -> inactive_tile_index == p_index)
		self -> inactive_tile_index = self -> tiles[p_index] . next;

	// Remove the tile from used.
	MCTileCacheTileListRemove(self, self -> used_tiles, p_index);
	// Push the tile onto the front of used.
	MCTileCacheTileListPush(self, self -> used_tiles, p_index);
	// Increment the active tile count.
	self -> active_tile_count += 1;
}

static void MCTileCacheDestroyTile(MCTileCacheRef self, uint32_t p_index, bool p_relink)
{
	if (p_index == 0)
		return;

	MCTileCacheTile *t_tile;
	t_tile = &self -> tiles[p_index];

	// Empty the tile (should it be full).
	MCTileCacheEmptyTile(self, p_index);

	if (p_relink)
	{
		if (p_index == self -> inactive_tile_index)
			self -> inactive_tile_index = t_tile -> next;
		MCTileCacheTileListRemove(self, self -> used_tiles, p_index);
		MCTileCacheTileListPush(self, self -> empty_tiles, p_index);
	}
}

static void MCTileCacheDirtyTile(MCTileCacheRef self, uint32_t p_index)
{
	if (p_index == 0)
		return;

	MCTileCacheTile *t_tile;
	t_tile = &self -> tiles[p_index];
	
	if (p_index == self -> inactive_tile_index)
		self -> inactive_tile_index = t_tile -> next;
	MCTileCacheTileListRemove(self, self -> used_tiles, p_index);
	MCTileCacheTileListPush(self, self -> dirty_tiles, p_index);
}

static void MCTileCacheFlushTile(MCTileCacheRef self, uint32_t p_index, bool p_put_on_empty)
{
	if (p_index == 0)
		return;

	// Get the tile pointer.
	MCTileCacheTile *t_tile;
	t_tile = MCTileCacheGetTile(self, p_index);
	
	// We must remove the reference to the tile from the cache - how we do this
	// depends on the type of layer.
	if (t_tile -> last_layer == 0)
	{
		// This is a sprite layer, so just set the relevant cell in the sprite
		// to 0.
		*MCTileCacheGetSpriteCell(self, t_tile -> first_layer, t_tile -> x, t_tile -> y) = 0;
	}
	else
	{
		// This is a scenery layer, so we need to search the cells for the tile.
		MCTileCacheCell *t_cell;
		t_cell = MCTileCacheGetSceneryCell(self, t_tile -> x, t_tile -> y);

		uint32_t t_index;
		t_index = 0;
		for(t_index = 0; t_index < t_cell -> tile_count; t_index++)
			if (t_cell -> tiles[t_index] == p_index)
				break;

		if (t_index < t_cell -> tile_count)
			memmove(&t_cell -> tiles[t_index], &t_cell -> tiles[t_index + 1], (t_cell -> tile_count - t_index - 1) * sizeof(uint16_t));

		t_cell -> tile_count -= 1;

		if (t_cell -> tile_count == 0)
		{
			MCMemoryDeleteArray(t_cell -> tiles);
			t_cell -> tiles = nil;
		}
	}

	// Empty the tile (should it be full).
	MCTileCacheEmptyTile(self, p_index);

	// Remove the tile from the used list.
	if (p_index == self -> inactive_tile_index)
		self -> inactive_tile_index = t_tile -> next;
	MCTileCacheTileListRemove(self, self -> used_tiles, p_index);

	// Place the tile on the empty list, if requested.
	if (p_put_on_empty)
		MCTileCacheTileListPush(self, self -> empty_tiles, p_index);
}

static void MCTileCacheCopyTileBits(int32_t p_size, uint32_t *p_dst_ptr, const uint32_t *p_src_ptr, uint32_t p_src_advance, uint32_t& r_or_mask, uint32_t& r_and_mask)
{
	uint32_t t_or_mask, t_and_mask;
	t_or_mask = 0;
	t_and_mask = 0xffffffff;
	for(uint32_t y = p_size; y > 0; y--)
	{
		for(uint32_t x = p_size; x > 0; x--)
		{
			uint32_t t_pixel;
			t_pixel = *p_src_ptr++;
			*p_dst_ptr++ = t_pixel;
			t_or_mask |= t_pixel;
			t_and_mask &= t_pixel;
		}
		
		p_src_ptr += p_src_advance;
	}
	
	r_or_mask = t_or_mask;
	r_and_mask = t_and_mask;
}

static bool MCTileCacheEnsureTile(MCTileCacheRef self)
{
	// Make sure we have room in the cache.
	uint32_t t_tile_bytes;
	t_tile_bytes = self -> tile_size * self -> tile_size * sizeof(uint32_t);
	while(self -> cache_size + t_tile_bytes > self -> cache_limit)
	{
		// If there are no more inactive tiles, we have no room so become
		// invalid
		if (self -> inactive_tile_index == 0)
		{
			MCTileCacheInvalidate(self);
			return false;
		}
		
		// Flush the tile, hopefully making some room for ourselves.
		MCTileCacheFlushTile(self, self -> used_tiles . last, true);
	}
	
	// There's room for at least one tile!
	return true;
}

static void MCTileCacheFillTile(MCTileCacheRef self, uint32_t p_index, MCImageBitmap *p_bitmap, int32_t p_x, int32_t p_y)
{	
	// Get the tile ptr.
	MCTileCacheTile *t_tile;
	t_tile = MCTileCacheGetTile(self, p_index);

	// Calculate the source tile offset / stride (in pixels).
	uint32_t *t_src_bits;
	uint32_t t_src_stride;
	t_src_stride = p_bitmap -> stride / sizeof(uint32_t);
	t_src_bits = (uint32_t *)p_bitmap -> data + p_y * self -> tile_size * t_src_stride + p_x * self -> tile_size;

	// First thing to do is to do the opacity and constancy check as if the
	// tile is transparent, or constant we don't need to do anything. To avoid
	// copying unless absolutely necessary, we check the stride of the context.
	void *t_tile_ptr;
	uint32_t t_tile_stride;
	uint32_t t_and_bits, t_or_bits;
	if (t_src_stride != self -> tile_size)
	{
		// Allocate the temporary tile, if it isn't already there.
		if (self -> temporary_tile == nil &&
			!MCMemoryAllocate(self -> tile_size * self -> tile_size * sizeof(uint32_t), self -> temporary_tile))
		{
			MCTileCacheInvalidate(self);
			return;
		}
		
		// Copy the tile from the context into the temporary buffer,
		// accumulating a running total of the alpha byte as we go.
		// (might be quicker to a running and/or instead...)
		MCTileCacheCopyTileBits(self -> tile_size, (uint32_t *)self -> temporary_tile, t_src_bits, t_src_stride - self -> tile_size, t_or_bits, t_and_bits); 
			
		// Make sure we use the temporary copy (faster path in compositor as
		// stride is tile size).
		t_tile_ptr = self -> temporary_tile;
		t_tile_stride = self -> tile_size * sizeof(uint32_t);
	}
	else
	{
		// Just compute or/and masks (no need to copy).
		t_and_bits = 0xffffffff;
		t_or_bits = 0;
		for(uint32_t i = 0; i < t_src_stride * self -> tile_size; i++)
			t_and_bits &= t_src_bits[i], t_or_bits |= t_src_bits[i];
	
		// Use direct access to context back-buffer.
		t_tile_ptr = (void *)t_src_bits;
		t_tile_stride = p_bitmap -> stride;
	}
	
	// The tile is constant if the or bits are the same as the and bits.
	// The tile is opaque if the top byte of the and bits is 255.
	// The tile is transparent if the top byte of the or bits is 0.
	// Note that there's no need to check for transparency as since we
	// use premultiplied alpha, type byte == 0 ==> pixel == 0.
	if (t_or_bits == t_and_bits)
	{
		t_tile -> constant = 1;
		t_tile -> alpha = t_or_bits >> 24;

		// IM-2013-08-23: [[ RefactorGraphics ]] Use MCGPixelUnpackNative to fix color swap issues
		t_tile -> data = (void *)t_or_bits;
	}
	else if (MCTileCacheEnsureTile(self))
	{
		t_tile -> constant = 0;
		t_tile -> alpha = (t_and_bits >> 24) == 255 ? 255 : 127;
		
		// Ask the compositor to allocate the tile.
		if (self -> compositor . allocate_tile != nil &&
			self -> compositor . allocate_tile(self -> compositor . context, self -> tile_size, t_tile_ptr, t_tile_stride, t_tile -> data))
		{
			// We allocated a tile, so increase the cache usage.
			self -> cache_size += self -> tile_size * self -> tile_size * sizeof(uint32_t);
		}
		else
		{
			// Allocation failed so invalidate.
			MCTileCacheInvalidate(self);
		}
	}
}

static void MCTileCacheEmptyTile(MCTileCacheRef self, uint32_t p_index)
{
	MCTileCacheTile *t_tile;
	t_tile = MCTileCacheGetTile(self, p_index);

	if (t_tile -> data == nil || t_tile -> constant != 0)
		return;
		
	// Reduce the image bytes used.
	self -> cache_size -= self -> tile_size * self -> tile_size * sizeof(uint32_t);

	// Make sure we free the tile's data.
	if (self -> compositor . deallocate_tile != nil)
		self -> compositor . deallocate_tile(self -> compositor . context, t_tile -> data);

	// Reset the data ptr.
	t_tile -> data = nil;
}

////////////////////////////////////////////////////////////////////////////////

// Computes the bounding box (in tiles) of all tiles touched by the given rect.
static MCTileCacheRectangle MCTileCacheComputeTouchedTiles(MCTileCacheRef self, const MCRectangle32& p_rect)
{
	MCTileCacheRectangle t_tile_rect;
	t_tile_rect . left = MCMax(MCTileCacheTileFloor(self, p_rect . x - self -> viewport . x), 0);
	t_tile_rect . top = MCMax(MCTileCacheTileFloor(self, p_rect . y - self -> viewport . y), 0);
	t_tile_rect . right = MCMin(MCTileCacheTileCeiling(self, p_rect . x + p_rect . width - self -> viewport . x), self -> tiles_across);
	t_tile_rect . bottom = MCMin(MCTileCacheTileCeiling(self, p_rect . y + p_rect . height - self -> viewport . y), self -> tiles_down);
	return t_tile_rect;
}

// Computes the bounding box (in tiles) of all tiles occluded by the given rect.
static MCTileCacheRectangle MCTileCacheComputeOccludedTiles(MCTileCacheRef self, const MCRectangle32& p_rect)
{
	MCTileCacheRectangle t_tile_rect;
	t_tile_rect . left = MCMax(MCTileCacheTileCeiling(self, p_rect . x - self -> viewport . x), 0);
	t_tile_rect . top = MCMax(MCTileCacheTileCeiling(self, p_rect . y - self -> viewport . y), 0);
	t_tile_rect . right = MCMin(MCTileCacheTileFloor(self, p_rect . x + p_rect . width - self -> viewport . x), self -> tiles_across);
	t_tile_rect . bottom = MCMin(MCTileCacheTileFloor(self, p_rect . y + p_rect . height - self -> viewport . y), self -> tiles_down);
	return t_tile_rect;
}

static void MCTileCacheFlushCellsContainingLayers(MCTileCacheRef self, uint32_t p_first_layer, uint32_t p_last_layer, const MCRectangle32& p_region)
{
	// Do nothing if the tilecache is invalid.
	if (!self -> valid)
		return;

	// Compute the cells touched.
	MCTileCacheRectangle t_affected_rect;
	t_affected_rect = MCTileCacheComputeTouchedTiles(self, p_region);

	// Now loop over all touched cells, searching for any tiles within them that
	// impinge on the range of layers.
	for(int32_t y = t_affected_rect . top; y < t_affected_rect . bottom; y++)
		for(int32_t x = t_affected_rect . left; x < t_affected_rect . right; x++)
		{
			// Fetch the cell pointer.
			MCTileCacheCell *t_cell;
			t_cell = MCTileCacheGetSceneryCell(self, x, y);
			
			// Loop through each tile, destroying any that have become invalid.
			// MDW-2013-04-16: [[ x64 ]] was comparing signed and unsigned values
			uint32_t t_new_tile_count;
			t_new_tile_count = 0;
			for(uint32_t i = 0; i < t_cell -> tile_count; i++)
			{
				// Get the index of the tile we are considering.
				uint32_t t_tile_index;
				t_tile_index = t_cell -> tiles[i];
				
				// Fetch the tile pointer.
				MCTileCacheTile *t_tile;
				t_tile = MCTileCacheGetTile(self, t_tile_index);
				
				// If the tile is in the affected range, dirty the tile and
				// remove from the list. It will get destroyed at next frame.
				if (p_first_layer >= t_tile -> first_layer && p_last_layer <= t_tile -> last_layer)
					MCTileCacheDirtyTile(self, t_tile_index);
				else
					t_cell -> tiles[t_new_tile_count++] = t_tile_index;
			}
			
			// Update the tile count, and resize.
			if (t_new_tile_count != t_cell -> tile_count)
			{
				if (t_new_tile_count != 0)
					MCMemoryResizeArray(t_new_tile_count, t_cell -> tiles, t_cell -> tile_count);
				else
				{
					MCMemoryDeleteArray(t_cell -> tiles);
					t_cell -> tiles = nil;
				}
				t_cell -> tile_count = t_new_tile_count;
			}
		}
}

void MCTileCacheInsertScenery(MCTileCacheRef self, uint32_t p_before_layer, const MCRectangle32& p_region)
{
	// If we are inserting at the start, there is nothing to do.
	if (p_before_layer == 1)
		return;

	// When we insert a scenery layer, we need to flush any tiles that contain layers before and after.
	MCTileCacheFlushCellsContainingLayers(self, p_before_layer - 1, p_before_layer, p_region);
}

void MCTileCacheRemoveScenery(MCTileCacheRef self, uint32_t p_layer, const MCRectangle32& p_region)
{
	// When removing a scenery layer, we need only flush any tiles containing it.
	MCTileCacheFlushCellsContainingLayers(self, p_layer, p_layer, p_region);
}

void MCTileCacheReshapeScenery(MCTileCacheRef self, uint32_t p_layer, const MCRectangle32& p_old_region, const MCRectangle32& p_new_region)
{
	// Remove the layer for the old region.
	MCTileCacheRemoveScenery(self, p_layer, p_old_region);
	// Add the layer for the new region.
	MCTileCacheInsertScenery(self, p_layer + 1, p_new_region);
}

void MCTileCacheUpdateScenery(MCTileCacheRef self, uint32_t p_layer, const MCRectangle32& p_region)
{
	// When updating a scenery layer, we need only flush any tiles touching it and the update region.
	MCTileCacheFlushCellsContainingLayers(self, p_layer, p_layer, p_region);
}

////////////////////////////////////////////////////////////////////////////////

// This method ensures that the the 'window' of tiles currently cached for the
// sprite contains req_rect. It updates the sprite origin and req_rect as
// required.
static bool MCTileCacheExpandSprite(MCTileCacheRef self, uint32_t p_sprite_id, MCTileCacheRectangle& x_req_rect)
{
	MCTileCacheSprite *t_sprite;
	t_sprite = MCTileCacheGetSprite(self, p_sprite_id);
	
	// The new rect we require starts off the same as the req rect.
	MCTileCacheRectangle t_new_rect;
	t_new_rect = x_req_rect;

	// Expand the requested rect to take into account any currently cached
	// tiles.
	for(int32_t y = t_sprite -> top; y < t_sprite -> bottom; y++)
		for(int32_t x = t_sprite -> left; x < t_sprite -> right; x++)
			if (t_sprite -> tiles[(y - t_sprite -> top) * (t_sprite -> right - t_sprite -> left) + (x - t_sprite -> left)] != 0)
			{
				if (x < t_new_rect . left)
					t_new_rect . left = x;
				else if (x + 1 > t_new_rect . right)
					t_new_rect . right = x + 1;

				if (y < t_new_rect . top)
					t_new_rect . top = y;
				else if (y + 1 > t_new_rect . bottom)
					t_new_rect . bottom = y + 1;
			}

	// If the rect hasn't changed, then return.
	if (t_new_rect . left == t_sprite -> left &&
		t_new_rect . top == t_sprite -> top &&
		t_new_rect . right == t_sprite -> right &&
		t_new_rect . bottom == t_sprite -> bottom)
		return true;

	// If any of the rects sides fall outside the region allowed (0,0)-(256,256)
	// we must scroll the rect / sprite appropriately. Any overflow will have
	// been caused by req_rect. Note that req_rect will always be at most the size
	// of the viewport (in tiles) so we can never have the situation where we
	// can't satisfy req_rect.

	// Compute the offset of tiles in the new array relative to old.
	// (new pos = old pos + dx).
	int32_t t_dx;
	if (t_new_rect . left < 0)
	{
		t_dx = -t_new_rect . left;
		t_new_rect . left = 0;
		t_new_rect . right = MCMin(t_new_rect . right + t_dx, 255);
	}
	else if (t_new_rect . right > 255)
	{
		t_dx = 255 - t_new_rect . right;
		t_new_rect . left = MCMax(0, t_new_rect . left + t_dx);
		t_new_rect . right = 255;
	}
	else
		t_dx = 0;

	int32_t t_dy;
	if (t_new_rect . top < 0)
	{
		t_dy = -t_new_rect . top;
		t_new_rect . top = 0;
		t_new_rect . bottom = MCMin(t_new_rect . bottom + t_dy, 255);
	}
	else if (t_new_rect . bottom > 255)
	{
		t_dy = 255 - t_new_rect . bottom;
		t_new_rect . top = MCMax(0, t_new_rect . top + t_dy);
		t_new_rect . bottom = 255;
	}
	else
		t_dy = 0;

	// Construct a new tile array, big enough for the new window.
	uint16_t *t_new_tiles;
	if (!MCMemoryNewArray((t_new_rect . right - t_new_rect . left) * (t_new_rect . bottom - t_new_rect . top), t_new_tiles))
	{
		MCTileCacheInvalidate(self);
		return false;
	}

	// Go through and map tiles across.
	for(int32_t y = t_sprite -> top; y < t_sprite -> bottom; y++)
		for(int32_t x = t_sprite -> left; x < t_sprite -> right; x++)
		{
			// Fetch the tile index at (x, y) in the old array.
			uint16_t t_tile_index;
			t_tile_index = t_sprite -> tiles[(y - t_sprite -> top) * (t_sprite -> right - t_sprite -> left) + (x - t_sprite -> left)];

			// If the tile index is 0, there is nothing to do.
			if (t_tile_index == 0)
				continue;

			// If we have no delta values, we can just copy across.
			if (t_dx == 0 && t_dy == 0)
			{
				t_new_tiles[(y - t_new_rect . top) * (t_new_rect . right - t_new_rect . left) + (x - t_new_rect . left)] = t_tile_index;
				continue;
			}

			// Otherwise if the tile is within the new bounds we can copy across but
			// must update the tile's pos.
			int32_t t_new_x, t_new_y;
			t_new_x = x + t_dx;
			t_new_y = y + t_dy;
			if (t_new_x >= 0 && t_new_x < 255 && t_new_y >= 0 && t_new_y < 255)
			{
				MCTileCacheTile *t_tile;
				t_tile = MCTileCacheGetTile(self, t_tile_index);
				t_tile -> x = t_new_x;
				t_tile -> y = t_new_y;
				t_new_tiles[(t_new_y - t_new_rect . top) * (t_new_rect . right - t_new_rect . left) + (t_new_x - t_new_rect . left)] = t_tile_index;
				continue;
			}

			// Finally, if we get here, it means the tile isn't within the window
			// any more so must be dirtied so that it is destroyed at next frame.
			MCTileCacheDirtyTile(self, t_tile_index);
		}

	// Delete the old sprite array.
	MCMemoryDeleteArray(t_sprite -> tiles);

	// Update the window and tiles.
	t_sprite -> tiles = t_new_tiles;
	t_sprite -> left = t_new_rect . left;
	t_sprite -> top = t_new_rect . top;
	t_sprite -> right = t_new_rect . right;
	t_sprite -> bottom = t_new_rect . bottom;

	// Finally, we must tweak the origin of the contents.
	t_sprite -> xorg += t_dx * self -> tile_size;
	t_sprite -> yorg += t_dy * self -> tile_size;

	// And the requested req.
	x_req_rect . left += t_dx;
	x_req_rect . top += t_dy;
	x_req_rect . right += t_dx;
	x_req_rect . bottom += t_dy;

	return true;
}

static bool MCTileCacheInsertSprite(MCTileCacheRef self, MCTileCacheRenderCallback p_callback, void *p_context, uint32_t& r_id)
{
	// Look for the first unused sprite.
	uint32_t t_id;
	t_id = 0;
	for(t_id = 0; t_id < self -> sprite_count; t_id++)
	{
		if (self -> sprites[t_id] . renderer . callback == nil)
			break;
	}
	
	// If there are no unused sprites, then extend the array.
	if (t_id == self -> sprite_count)
	{
		// If we can allocate no more sprites, or if extending the array fails
		// then invalidate.
		if (self -> sprite_count == 65536 ||
			!MCMemoryResizeArray(self -> sprite_count != 0 ? self -> sprite_count * 2 : 4, self -> sprites, self -> sprite_count))
		
		{
			MCTileCacheInvalidate(self);
			return false;
		}
		
		t_id = self -> sprite_count / 2;
	}
	
	// Sprite id's are one more than the index (index 0 means no sprite).
	t_id += 1;
	
	// Compute the sprite ref.
	MCTileCacheSprite *t_sprite;
	t_sprite = MCTileCacheGetSprite(self, t_id);
	
	// Now initialize the renderer.
	t_sprite -> renderer . callback = p_callback;
	t_sprite -> renderer . context = p_context;

	// Start with (128, 128) in tile coords being (0, 0) for sprite contents.
	if (self -> tile_size < 256)
	{
		t_sprite -> xorg = 128 * self -> tile_size;
		t_sprite -> yorg = 128 * self -> tile_size;
	}
	else
	{
		t_sprite -> xorg = 64 * self -> tile_size;
		t_sprite -> yorg = 64 * self -> tile_size;
	}

	// Initialize the tile bounds to the empty rect.
	t_sprite -> left = 0;
	t_sprite -> right = 0;
	t_sprite -> top = 0;
	t_sprite -> bottom = 0;
	
	// There are no cached tiles to begin with.
	t_sprite -> tiles = nil;
	
	// Return the id and success.
	r_id = t_id;
	
	return true;
}

void MCTileCacheRemoveSprite(MCTileCacheRef self, uint32_t p_id)
{
	// Do nothing if the tilecache is invalid.
	if (!self -> valid)
		return;
	
	// If the tilecache is clean, there is nothing to do.
	if (self -> clean)
		return;
		
	// If the id is zero, then there is no sprite yet.
	if (p_id == 0)
		return;

	// Get the sprite ptr
	MCTileCacheSprite *t_sprite;
	t_sprite = MCTileCacheGetSprite(self, p_id);

	// Dirty the tiles the sprite is currently using - these will be destroyed
	// at next frame.
	for(int32_t y = t_sprite -> top; y < t_sprite -> bottom; y++)
		for(int32_t x = t_sprite -> left; x < t_sprite -> right; x++)
			MCTileCacheDirtyTile(self, *MCTileCacheGetSpriteCell(self, p_id, x, y));
}

void MCTileCacheScrollSprite(MCTileCacheRef self, uint32_t p_id, int32_t p_dx, int32_t p_dy)
{
	// Do nothing if the tilecache is invalid.
	if (!self -> valid)
		return;
		
	// If the tilecache is clean, there is nothing to do.
	if (self -> clean)
		return;
		
	// If the id is zero, then there is no sprite yet.
	if (p_id == 0)
		return;

	// Get the sprite pointer.
	MCTileCacheSprite *t_sprite;
	t_sprite = MCTileCacheGetSprite(self, p_id);

	// Scroll it.
	t_sprite -> xorg -= p_dx;
	t_sprite -> yorg -= p_dy;
}

void MCTileCacheUpdateSprite(MCTileCacheRef self, uint32_t p_id, const MCRectangle32& p_region)
{
	// Do nothing if the tilecache is invalid.
	if (!self -> valid)
		return;
		
	// If the tilecache is clean, there is nothing to do.
	if (self -> clean)
		return;
		
	// If the id is zero, then there is no sprite yet.
	if (p_id == 0)
		return;
	
	// Get the sprite pointer.
	MCTileCacheSprite *t_sprite;
	t_sprite = MCTileCacheGetSprite(self, p_id);
	
	// The region passed in is in sprite layer co-ords, so first compute it
	// in tile-coords.
	MCRectangle32 t_tile_rect;
	t_tile_rect = MCRectangle32Offset(p_region, t_sprite -> xorg, t_sprite -> yorg);

	// Compute the bounds of the touched cells.
	int32_t t_left, t_top, t_right, t_bottom;
    // AL-2014-10-28 : [[ Bug 13833 ]] Ensure we use the correct version of MCMax.
    //  Using the uint8_t version causes incorrect values for bounds.
	t_left = MCMax((int32_t)t_sprite -> left, MCTileCacheTileFloor(self, t_tile_rect . x));
	t_top = MCMax((int32_t)t_sprite -> top, MCTileCacheTileFloor(self, t_tile_rect . y));
	t_right = MCMin((int32_t)t_sprite -> right, MCTileCacheTileCeiling(self, t_tile_rect . x + t_tile_rect . width));
	t_bottom = MCMin((int32_t)t_sprite -> bottom, MCTileCacheTileCeiling(self, t_tile_rect . y + t_tile_rect . height));
	
	// Now iterate over the tiles, dirtying those that are in the region.
	for(int32_t y = t_top; y < t_bottom; y++)
		for(int32_t x = t_left; x < t_right; x++)
		{
			uint16_t *t_cell;
			t_cell = MCTileCacheGetSpriteCell(self, p_id, x, y);
			if (*t_cell != 0)
			{
				MCTileCacheDirtyTile(self, *t_cell);
				*t_cell = 0;
			}
		}
}

////////////////////////////////////////////////////////////////////////////////

void MCTileCacheBeginFrame(MCTileCacheRef self)
{
	// If the tilecache is invalid, do nothing.
	if (!self -> valid)
		return;
	
	// If the tilecache isn't clean, then allocate the scenery map array.
	if (!self -> clean &&
		!MCMemoryResizeArray(self -> scenery_renderers_frontier + 1, self -> scenery_map, self -> scenery_map_capacity))
	{
		MCTileCacheInvalidate(self);
		return;
	}
	
	// Allocate the frontiers array.
	if (!MCMemoryNewArray(self -> tiles_across * self -> tiles_down, self -> frontiers))
	{
		MCTileCacheInvalidate(self);
		return;
	}
	
	// Flush all the dirty tiles.
	while(self -> dirty_tiles . first != 0)
	{
		uint32_t t_tile;
		t_tile = MCTileCacheTileListPop(self, self -> dirty_tiles);
		MCTileCacheTileListPush(self, self -> empty_tiles, t_tile);
		MCTileCacheDestroyTile(self, t_tile, false);
	}

	// Reset the display list.
	self -> display_list_frontier = 0;
	
	// Reset the render lists.
	MCTileCacheRenderListReset(self, self -> scenery_render_list);
	MCTileCacheRenderListReset(self, self -> sprite_render_list);
	
	// Reset the scenery control list.
	self -> scenery_renderers_frontier = 0;
	
	// Move the start of the inactive tile list to the start of the used tiles.
	self -> inactive_tile_index = self -> used_tiles . first;
	
	// Start off with no active tile count.
	self -> active_tile_count = 0;
}

void MCTileCacheEndFrame(MCTileCacheRef self)
{
	// Destroy the frontiers array.
	MCMemoryDeleteArray(self -> frontiers);
	self -> frontiers = nil;
	
	// Tell the compositor we are about to start generating tiles.
	if (self -> valid && self -> compositor . begin_tiling != nil)
		if (!self -> compositor . begin_tiling(self -> compositor . context))
			MCTileCacheInvalidate(self);
	
	// Render the sprites;
	if (self -> valid)
		MCTileCacheRenderSpriteTiles(self);
	
	// Render the scenery
	if (self -> valid)
		MCTileCacheRenderSceneryTiles(self);
	
	// Tell the compositor we are about to end generating tiles.
	if (self -> valid && self -> compositor . end_tiling != nil)
		if (!self -> compositor . end_tiling(self -> compositor . context))
			MCTileCacheInvalidate(self);
			
	// Free the temporary tile (if there).
	MCMemoryDeallocate(self -> temporary_tile);
	self -> temporary_tile = nil;

	// The tilecache is no longer clean.
	self -> clean = false;

	// Some statistics
#ifdef _DEBUG
    MCLog("Frame - %d sprite tiles, %d scenery tiles, %d active tiles, %d instructions, %d bytes",
    			self -> sprite_render_list . length, self -> scenery_render_list . length, self -> active_tile_count, self -> display_list_frontier, self -> cache_size);
    MCLog("      - %d tile count, %d sprite count, %d scenery count", self->tile_count, self->sprite_count, self->scenery_renderers_frontier);
#endif
}

static void MCTileCacheDrawSprite(MCTileCacheRef self, uint32_t p_sprite_id, MCGContextRef p_context, const MCRectangle32& p_rect)
{
	MCTileCacheSprite *t_sprite;
	t_sprite = MCTileCacheGetSprite(self, p_sprite_id);
	
	if (!t_sprite -> renderer . callback(t_sprite -> renderer . context, p_context, p_rect))
		MCTileCacheInvalidate(self);
}

static void MCTileCacheRenderSpriteTiles(MCTileCacheRef self)
{
	// At the moment sprites are not re-used so the required tiles for each
	// sprite will be in a contiguous block... At some point, however, sprites
	// might get re-used so we will need to sort at that point.
	// <sort sprite list by sprite id>

	// Loop through the render list, processing batches of tile requests with
	// the same id.
	uint32_t t_index;
	t_index = 0;
	while(self -> valid && t_index < self -> sprite_render_list . length)
	{
		// IM-2014-07-03: [[ GraphicsPerformance ]] MCGRegion to collect dirty tile rects.
		MCGRegionRef t_tile_region;
		t_tile_region = nil;
		
		if (!MCGRegionCreate(t_tile_region))
			MCTileCacheInvalidate(self);
		
		// Record the first index
		uint32_t t_sprite_index;
		t_sprite_index = t_index;

		// The id of the sprite we are processing.
		uint32_t t_sprite_id;
		t_sprite_id = self -> tiles[self -> sprite_render_list . contents[t_index]] . first_layer;

		// At some point we'll support fully accurate regions for rendering, but for
		// now we don't so just compute the tile bounds we require.
		MCTileCacheRectangle t_required_tiles;
		t_required_tiles . left = t_required_tiles . top = INT32_MAX;
		t_required_tiles . right = t_required_tiles . bottom = INT32_MIN;
		while(self -> valid && t_index < self -> sprite_render_list . length)
		{
			// Fetch the current tile.
			MCTileCacheTile *t_tile;
			t_tile = &self -> tiles[self -> sprite_render_list . contents[t_index]];

			// Check to see if it is part of the same sprite.
			if (t_tile -> first_layer != t_sprite_id)
				break;

			if (!MCGRegionAddRect(t_tile_region, MCGIntegerRectangleMake(t_tile->x * self->tile_size, t_tile->y * self->tile_size, self->tile_size, self->tile_size)))
				MCTileCacheInvalidate(self);
			
			// Extend the required tiles rect.
			if (t_tile -> x < t_required_tiles . left)
				t_required_tiles . left = t_tile -> x;
			if (t_tile -> x + 1 > t_required_tiles . right)
				t_required_tiles . right = t_tile -> x + 1;
			if (t_tile -> y < t_required_tiles . top)
				t_required_tiles . top = t_tile -> y;
			if (t_tile -> y + 1> t_required_tiles . bottom)
				t_required_tiles . bottom = t_tile -> y + 1;

			// Move to next item.
			t_index++;
		}

		// Get the sprite pointer.
		MCTileCacheSprite *t_sprite;
		t_sprite = MCTileCacheGetSprite(self, t_sprite_id);

		// IM-2014-07-03: [[ GraphicsPerformance ]] Offset region to sprite origin
		MCGRegionTranslate(t_tile_region, -t_sprite->xorg, -t_sprite->yorg);
		
		// Compute the rect of the tiles
		MCRectangle32 t_required_rect;
		t_required_rect = MCRectangle32FromMCGIntegerRectangle(MCGRegionGetBounds(t_tile_region));

		// Create a memory context of the appropriate size.
		MCGContextRef t_context = nil;
		MCImageBitmap *t_bitmap = nil;

		if (self -> valid)
		{
			bool t_success = true;
			t_success = MCImageBitmapCreate(t_required_rect.width, t_required_rect.height, t_bitmap);
			if (t_success)
			{
				// IM-2013-08-22: [[ RefactorGraphics ]] clear sprite bitmap before rendering to it
				MCImageBitmapClear(t_bitmap);
				t_success = MCGContextCreateWithPixels(t_bitmap->width, t_bitmap->height, t_bitmap->stride, t_bitmap->data, true, t_context);
			}

			if (!t_success)
				MCTileCacheInvalidate(self);
		}

		// Invoke the sprite renderer to draw it.
		if (self -> valid)
		{
			// IM-2014-07-03: [[ GraphicsPerformance ]] Set the origin of the context to the topleft of the sprite
			MCGContextTranslateCTM(t_context, -t_required_rect.x, -t_required_rect.y);
			// IM-2014-07-03: [[ GraphicsPerformance ]] Clip the context to only the damaged tiles
			MCGContextClipToRegion(t_context, t_tile_region);
			MCTileCacheDrawSprite(self, t_sprite_id, t_context, t_required_rect);
		}

		// Get rid of the temporary context.
		MCGContextRelease(t_context);

		// Free the tile region
		MCGRegionDestroy(t_tile_region);
		
		// Now extract each of the required tiles.
		if (self -> valid)
			for(uint32_t i = t_sprite_index; i < t_index; i++)
			{
				// Fetch the required tile.
				MCTileCacheTile *t_tile;
				t_tile = MCTileCacheGetTile(self, self -> sprite_render_list . contents[i]);

				// Update the sprites cache array.
				uint16_t *t_cell;
				t_cell = MCTileCacheGetSpriteCell(self, t_sprite_id, t_tile -> x, t_tile -> y);
				*t_cell = self -> sprite_render_list . contents[i];

				// Fetch the tile's image.
				MCTileCacheFillTile(self, self -> sprite_render_list . contents[i], t_bitmap, t_tile -> x - t_required_tiles . left, t_tile -> y - t_required_tiles . top);
			}

		// free the temporary bitmap
		MCImageFreeBitmap(t_bitmap);
	}
}

static int MCTileCacheSortRenderListByIncreasingTo(void *p_context, const void *p_left, const void *p_right)
{
	MCTileCacheRef self;
	self = (MCTileCacheRef)p_context;

	uint16_t t_left, t_right;
	t_left = *(const uint16_t *)p_left;
	t_right = *(const uint16_t *)p_right;

	return self -> tiles[t_left] . last_layer - self -> tiles[t_right] . last_layer;
}

static int MCTileCacheSortRenderListByDecreasingFrom(void *p_context, const void *p_left, const void *p_right)
{
	MCTileCacheRef self;
	self = (MCTileCacheRef)p_context;

	uint16_t t_left, t_right;
	t_left = *(const uint16_t *)p_left;
	t_right = *(const uint16_t *)p_right;

	return self -> tiles[t_right] . first_layer - self -> tiles[t_left] . first_layer;
}

static int MCTileCacheSortRenderListByIncreasingYThenX(void *p_context, const void *p_left, const void *p_right)
{
	MCTileCacheRef self;
	self = (MCTileCacheRef)p_context;

	MCTileCacheTile *t_left, *t_right;
	t_left = &self -> tiles[*(const uint16_t *)p_left];
	t_right = &self -> tiles[*(const uint16_t *)p_right];

	int d;
	d = t_left -> y - t_right -> y;
	if (d == 0)
		d = t_left -> x - t_right -> x;

	return d;
}

static void MCTileCacheDrawScenery(MCTileCacheRef self, uint32_t p_layer_id, MCGContextRef p_context, const MCRectangle32& p_rect)
{
	if (!self -> scenery_renderers[p_layer_id] . callback(self -> scenery_renderers[p_layer_id] . context, p_context, p_rect))
		MCTileCacheInvalidate(self);
}

static void MCTileCacheRenderSceneryTiles(MCTileCacheRef self)
{
	// The scenery render list is a sequence of tiles representing from/to (inc.)
	// ranges of layers to composite together. In each case the 'from' layer is
	// the top-most, and the 'to' layer is the bottom-most.

	// To produce the scenery tiles, we render the layers from bottom-most to
	// top-most, emitting a tile when it's top-most layer is reached. For
	// efficiency, we only want to render the areas of the canvas which touch
	// tiles we still want. To do this, the clipping region is set to the union
	// of all tiles remaining that include the layer we are currently rendering.

	// Get the render list.
	uint16_t *t_render_list;
	uint32_t t_render_list_length;
	t_render_list = self ->  scenery_render_list . contents;
	t_render_list_length = self -> scenery_render_list . length;

	// Take a copy of the render list which we use to first determine when
	// layers leave.
	uint16_t *t_sorted_render_list;
	t_sorted_render_list = nil;
	if (self -> valid)
		if (!MCMemoryNewArray(t_render_list_length, t_sorted_render_list))
			MCTileCacheInvalidate(self);

	// Copy the original render list and sort by decreasing from layer. Then
	// sort the original render list by increasing to layer.
	if (self -> valid)
	{
		memcpy(t_sorted_render_list, t_render_list, sizeof(uint16_t) * t_render_list_length);
		stdc_qsort(t_sorted_render_list, t_render_list_length, sizeof(uint16_t), MCTileCacheSortRenderListByDecreasingFrom, self);
		stdc_qsort(t_render_list, t_render_list_length, sizeof(uint16_t), MCTileCacheSortRenderListByIncreasingTo, self);
	}

	// IM-2014-07-02: [[ GraphicsPerformance ]] MCGRegion used to collect required tile rects.
	MCGRegionRef t_tile_region;
	t_tile_region = nil;
	
	if (self->valid)
		if (!MCGRegionCreate(t_tile_region))
			MCTileCacheInvalidate(self);
	
	// Work out the bounds of the update.
	MCTileCacheRectangle t_required_tiles;
	t_required_tiles . left = t_required_tiles . top = INT32_MAX;
	t_required_tiles . right = t_required_tiles . bottom = INT32_MIN;
	for(uint32_t i = 0; i < t_render_list_length; i++)
	{
		// Fetch the current tile.
		MCTileCacheTile *t_tile;
		t_tile = &self -> tiles[t_render_list[i]];

		// IM-2014-07-02: [[ GraphicsPerformance ]] Add tile rect to redraw region
		MCGRegionAddRect(t_tile_region, MCGIntegerRectangleMake(t_tile -> x * self -> tile_size, t_tile -> y * self -> tile_size, self -> tile_size, self -> tile_size));
		
		// Extend the required tiles rect.
		if (t_tile -> x < t_required_tiles . left)
			t_required_tiles . left = t_tile -> x;
		if (t_tile -> x + 1 > t_required_tiles . right)
			t_required_tiles . right = t_tile -> x + 1;
		if (t_tile -> y < t_required_tiles . top)
			t_required_tiles . top = t_tile -> y;
		if (t_tile -> y + 1> t_required_tiles . bottom)
			t_required_tiles . bottom = t_tile -> y + 1;
	}

	// Convenience vars.
	int32_t t_required_width, t_required_height;
	t_required_width = t_required_tiles . right - t_required_tiles . left;
	t_required_height = t_required_tiles . bottom - t_required_tiles . top;

	// Compute the rect of the tiles
	MCRectangle32 t_required_rect;
	// IM-2014-07-02: [[ GraphicsPerformance ]] Required rect is the bounds of all required tile rects
	t_required_rect = MCRectangle32FromMCGIntegerRectangle(MCGRegionGetBounds(t_tile_region));

	// While rendering, we need to keep track of the 'active' tiles so we know
	// when to erase.
	uint8_t *t_active_tiles;
	t_active_tiles = nil;
	if (self -> valid)
		if (!MCMemoryNewArray(t_required_width * t_required_height, t_active_tiles))
			MCTileCacheInvalidate(self);

	// Create a memory context of the appropriate size.
	MCImageBitmap *t_bitmap = nil;
	MCGContextRef t_context = nil;
	if (self -> valid)
	{
		bool t_success = true;
		t_success = MCImageBitmapCreate(t_required_rect.width, t_required_rect.height, t_bitmap);
		if (t_success)
			t_success = MCGContextCreateWithPixels(t_bitmap->width, t_bitmap->height, t_bitmap->stride, t_bitmap->data, true, t_context);

		if (!t_success)
			MCTileCacheInvalidate(self);
	}

	// Configure the context.
	if (self -> valid)
	{
		MCGContextTranslateCTM(t_context, -t_required_rect.x, -t_required_rect.y);
		// IM-2014-07-02: [[ GraphicsPerformance ]] Clip context to only the tiles we need.
		MCGContextClipToRegion(t_context, t_tile_region);
	}
	
	// Now we use the original render list in reverse to determine what layers
	// to render, siphoning off tiles as we reach them in the sorted render
	// list.
	uint32_t t_output_index, t_input_index, t_layer;
	t_output_index = 0;
	t_input_index = t_render_list_length;
	if (t_render_list_length > 0)
		t_layer = self -> tiles[t_render_list[t_input_index - 1]] . last_layer;
	while(t_input_index > 0 && self -> valid)
	{
		// Scan forward to find the range of layers to render with the current
		// activation.
		uint32_t t_next_layer;
		while(t_input_index > 0)
		{
			MCTileCacheTile *t_tile;
			t_tile = &self -> tiles[t_render_list[t_input_index - 1]];

			t_next_layer = t_tile -> last_layer;
			if (t_next_layer != t_layer)
				break;

			// Get the activity byte ptr.
			uint8_t *t_activity;
			t_activity = &t_active_tiles[(t_tile -> y - t_required_tiles . top) * t_required_width + (t_tile -> x - t_required_tiles . left)];

			// If the tile is inactive but previously used, we must erase it. (As
			// we are not clipping accurately yet, we must always erase).
			if (*t_activity < 2)
			{
				for(uint32_t y = 0; y < self -> tile_size; y++)
					memset((uint8_t*)t_bitmap -> data + t_bitmap -> stride * (y + (t_tile -> y - t_required_tiles . top) * self -> tile_size) + (t_tile -> x - t_required_tiles . left) * self -> tile_size * sizeof(uint32_t), 0, self -> tile_size * sizeof(uint32_t));
			}

			// An active tile is marked with a 2.
			*t_activity = 2;

			t_input_index -= 1;
		}

		if (t_input_index == 0)
			t_next_layer = 0;

		// Iterate forwards, rendering layers as we go, until we get to the
		// next layer that changes clip (t_next_layer).
		while(t_layer > t_next_layer)
		{
			// Render the current layer - but only if there are tiles from it we need.
			if (t_output_index < t_render_list_length)
				MCTileCacheDrawScenery(self, t_layer, t_context, t_required_rect);

			// Extract any tiles that are now ready.
			while(t_output_index < t_render_list_length)
			{
				// Fetch the required tile.
				MCTileCacheTile *t_tile;
				t_tile = MCTileCacheGetTile(self, t_sorted_render_list[t_output_index]);

				// If it isn't for this layer, then do nowt.
				if (t_tile -> first_layer != t_layer)
					break;

				// Fetch the tile's image.
				MCTileCacheFillTile(self, t_sorted_render_list[t_output_index], t_bitmap, t_tile -> x - t_required_tiles . left, t_tile -> y - t_required_tiles . top);

				// Mark the tile as inactive but used.
				uint8_t *t_activity;
				t_activity = &t_active_tiles[(t_tile -> y - t_required_tiles . top) * t_required_width + (t_tile -> x - t_required_tiles . left)];
				*t_activity = 1;

				// Move to next tile.
				t_output_index += 1;
			}

			// Move to the next layer.
			t_layer -= 1;
		}
	}

	// Get rid of the context.
	MCGContextRelease(t_context);
	MCImageFreeBitmap(t_bitmap);

	// Get rid of the active tiles array
	MCMemoryDeleteArray(t_active_tiles);

	// Get rid of the tile region
	MCGRegionDestroy(t_tile_region);

	// Finally, update the tile cache list.
	if (self -> valid)
	{
		// First sort the render list by y then x.
		stdc_qsort(t_sorted_render_list, t_render_list_length, sizeof(uint16_t), MCTileCacheSortRenderListByIncreasingYThenX, self);

		// The current index in the render list.
		uint32_t t_index;
		t_index = 0;

		// Next iterate over all cells updating layer ids and appending new
		// tiles as necessary.
		for(int32_t y = 0; y < self -> tiles_down; y++)
			for(int32_t x = 0; x < self -> tiles_across; x++)
			{
				// Get the cell ptr.
				MCTileCacheCell *t_cell;
				t_cell = MCTileCacheGetSceneryCell(self, x, y);
				
				// Update the layer ids via indirection through the scenery map.
				for(uint32_t i = 0; i < t_cell -> tile_count; i++)
				{
					MCTileCacheTile *t_tile;
					t_tile = MCTileCacheGetTile(self, t_cell -> tiles[i]);
					t_tile -> first_layer = self -> scenery_map[t_tile -> first_layer];
					t_tile -> last_layer = self -> scenery_map[t_tile -> last_layer];
				}

				// Now see if we have any new tiles to add to it.
				uint32_t t_first_index;
				t_first_index = t_index;
				while(t_index < t_render_list_length)
				{
					MCTileCacheTile *t_tile;
					t_tile = MCTileCacheGetTile(self, t_sorted_render_list[t_index]);
					
					// If the x or y has changed, we've moved to another cell.
					if (t_tile -> x != x || t_tile -> y != y)
						break;

					t_index += 1;
				}

				// Extend the tiles array (if needed)
				if (t_first_index != t_index)
				{
					uint32_t t_old_tile_count;
					t_old_tile_count = t_cell -> tile_count;
					if (MCMemoryResizeArray(t_cell -> tile_count + (t_index - t_first_index), t_cell -> tiles, t_cell -> tile_count))
					{
						for(uint32_t i = t_first_index; i < t_index; i++)
							t_cell -> tiles[t_old_tile_count + i - t_first_index] = t_sorted_render_list[i];
					}
					else
						MCTileCacheInvalidate(self);
				}
			}
	}

	// Get rid of the sorted render list.
	MCMemoryDeleteArray(t_sorted_render_list);
}

////////////////////////////////////////////////////////////////////////////////
//
//  RENDER LIST MANAGEMENT
//

static void MCTileCacheRenderListDestroy(MCTileCacheRef self, MCTileCacheRenderList& x_list)
{
	MCMemoryDeleteArray(x_list . contents);
	x_list . contents = nil;
	x_list . capacity = 0;
	x_list . length = 0;
}

static void MCTileCacheRenderListReset(MCTileCacheRef self, MCTileCacheRenderList& x_list)
{
	x_list . length = 0;
}

static void MCTileCacheRenderListPush(MCTileCacheRef self, MCTileCacheRenderList& x_list, uint32_t p_tile)
{
	if (x_list . length == x_list . capacity &&
		!MCMemoryResizeArray(x_list . capacity != 0 ? x_list . capacity * 2 : 2, x_list . contents, x_list . capacity))
	{
		MCTileCacheInvalidate(self);
		return;
	}

	x_list . contents[x_list . length++] = p_tile;
}

////////////////////////////////////////////////////////////////////////////////
//
//  DISPLAY LIST MANAGEMENT
//

static void MCTileCacheDestroyDisplayList(MCTileCacheRef self)
{
	MCMemoryDeallocate(self -> display_list);
	self -> display_list = nil;
	self -> display_list_frontier = 0;
	self -> display_list_capacity = 0;
}

static void MCTileCacheResetDisplayList(MCTileCacheRef self)
{
	self -> display_list_frontier = 0;
}

static bool MCTileCacheEnsureDisplayList(MCTileCacheRef self, uint32_t p_amount)
{
	if (self -> display_list_frontier + p_amount > self -> display_list_capacity)
	{
		if (!MCMemoryReallocate(self -> display_list, (self -> display_list_capacity != 0 ? self -> display_list_capacity * 2 : 16) * sizeof(self -> display_list[0]), self -> display_list))
		{
			MCTileCacheInvalidate(self);
			return false;
		}

		self -> display_list_capacity = self -> display_list_capacity != 0 ? self -> display_list_capacity * 2 : 16;
	}

	return true;
}

static void MCTileCachePushCompositeOntoDisplayList(MCTileCacheRef self, uint32_t p_tile)
{
	if (!MCTileCacheEnsureDisplayList(self, 1))
		return;

	self -> display_list[self -> display_list_frontier++] = p_tile;
}

static inline void _push_uint32(uint16_t *p_display_list, uint32_t &x_index, uint32_t p_value)
{
	p_display_list[x_index++] = p_value >> 16;
	p_display_list[x_index++] = p_value & 0xFFFF;
}

static void MCTileCachePushBeginLayerOntoDisplayList(MCTileCacheRef self, int32_t p_ox, int32_t p_oy, const MCRectangle32& p_clip, uint32_t p_opacity, uint32_t p_ink)
{
	if (!MCTileCacheEnsureDisplayList(self, 14))
		return;

	// Note that the order is reversed from what you expect. This is because the
	// display list is played backwards.
	self -> display_list[self -> display_list_frontier++] = (p_opacity << 8) | p_ink;

	// IM-2014-02-28: [[ Bug 11617 ]] Push 32bit values onto list as 2 16bit values
	_push_uint32(self->display_list, self->display_list_frontier, p_clip.height);
	_push_uint32(self->display_list, self->display_list_frontier, p_clip.width);
	_push_uint32(self->display_list, self->display_list_frontier, p_clip.y);
	_push_uint32(self->display_list, self->display_list_frontier, p_clip.x);

	_push_uint32(self->display_list, self->display_list_frontier, p_oy);
	_push_uint32(self->display_list, self->display_list_frontier, p_ox);

	self -> display_list[self -> display_list_frontier++] = 0;
}

static inline uint32_t _pop_uint32(uint16_t *p_display_list, uint32_t &x_index)
{
	uint32_t t_tmp;
	t_tmp = p_display_list[--x_index];
	t_tmp |= p_display_list[--x_index] << 16;
	return t_tmp;
}

// IM-2014-02-28: [[ Bug 11617 ]] Retrieve layer values from display list
static void MCTileCachePopLayerFromDisplayList(uint16_t *p_display_list, uint32_t &x_index, int32_t &r_ox, int32_t &r_oy, MCRectangle32 &r_clip, uint32_t &r_opacity, uint32_t &r_ink)
{
	r_ox = _pop_uint32(p_display_list, x_index);
	r_oy = _pop_uint32(p_display_list, x_index);
	
	r_clip.x = _pop_uint32(p_display_list, x_index);
	r_clip.y = _pop_uint32(p_display_list, x_index);
	r_clip.width = _pop_uint32(p_display_list, x_index);
	r_clip.height = _pop_uint32(p_display_list, x_index);

	r_ink = p_display_list[--x_index] & 0xff;
	r_opacity = p_display_list[x_index] >> 8;
}

static void MCTileCachePushEndLayerOntoDisplayList(MCTileCacheRef self)
{
	if (!MCTileCacheEnsureDisplayList(self, 1))
		return;

	self -> display_list[self -> display_list_frontier++] = 0;
}

////////////////////////////////////////////////////////////////////////////////
//
//  SCENERY RENDERING
//

static inline bool MCTileCacheFrontierIsOccluded(MCTileCacheFrontier *self)
{
	return self -> first_layer == 0 && self -> last_layer == 65535;
}

static inline bool MCTileCacheFrontierIsEmpty(MCTileCacheFrontier *self)
{
	return self -> first_layer == 0 && self -> last_layer == 0;
}

static inline void MCTileCacheFrontierSetOccluded(MCTileCacheFrontier *self)
{
	self -> first_layer = 0;
	self -> last_layer = 65535;
}

static inline void MCTileCacheFrontierSetEmpty(MCTileCacheFrontier *self)
{
	self -> first_layer = 0;
	self -> last_layer = 0;
}

static void MCTileCacheRenderSceneryTile(MCTileCacheRef self, int32_t p_x, int32_t p_y, MCTileCacheFrontier *p_frontier, bool p_is_opaque)
{
	// First look to see if there is an appropriate tile in the cache. (This is
	// only possible if 'old_layer' ids are zero).
	uint32_t t_tile_index;
	t_tile_index = 0;
	if (p_frontier -> old_first_layer != 0 && p_frontier -> old_last_layer != 0)
	{
		MCTileCacheCell *t_cell;
		t_cell = MCTileCacheGetSceneryCell(self, p_x, p_y);
		for(uint32_t i = 0; i < t_cell -> tile_count; i++)
		{
			MCTileCacheTile *t_tile;
			t_tile = MCTileCacheGetTile(self, t_cell -> tiles[i]);
			if (t_tile -> first_layer == p_frontier -> old_first_layer &&
				t_tile -> last_layer == p_frontier -> old_last_layer)
			{
				t_tile_index = t_cell -> tiles[i];

				// Make sure we move the tile to the head of the used list.
				MCTileCacheTouchTile(self, t_tile_index);

				break;
			}
		}
	}
	
	// If we don't have a tile, we allocate a new one and push it onto the
	// render list.
	MCTileCacheTile *t_tile;
	if (t_tile_index == 0)
	{
		// First search for an unused tile.
		if (!MCTileCacheCreateTile(self, t_tile_index))
			return;

		// Get the tile's ptr
		t_tile = MCTileCacheGetTile(self, t_tile_index);
		
		// Update the tile entry.
		t_tile -> first_layer = p_frontier -> first_layer;
		t_tile -> last_layer = p_frontier -> last_layer;
		t_tile -> x = p_x;
		t_tile -> y = p_y;
		
		// Set the alpha appropriately. This allows us to use the information
		// sooner. More accurate determination will be done after rendering.
		t_tile -> alpha = p_is_opaque ? 255 : 127;

		// Push the tile onto the render list.
		MCTileCacheRenderListPush(self, self -> scenery_render_list, t_tile_index);
	}
	else	
		t_tile = MCTileCacheGetTile(self, t_tile_index);

	// Finally, push the tile onto the display list, but only if it is not
	// fully transparent.
	if (t_tile -> alpha != 0)
		MCTileCachePushCompositeOntoDisplayList(self, t_tile_index);
		
	// Finally, update the frontier. If the tile we have just output is opaque we
	// mark the frontier as occluded, else empty.
	if (t_tile -> alpha == 255)
		MCTileCacheFrontierSetOccluded(p_frontier);
	else
		MCTileCacheFrontierSetEmpty(p_frontier);
}

void MCTileCacheRenderScenery(MCTileCacheRef self, MCTileCacheLayer& x_layer)
{
	// If the tilecache isn't valid, do nothing.
	if (!self -> valid)
		return;
		
	// If the tilecache is clean, reset the id.
	if (self -> clean)
		x_layer . id = 0;
		
	// Allocate a new layer id.
	uint32_t t_layer_id;
	t_layer_id = ++self -> scenery_renderers_frontier;
	
	// Ensure there is room in the renderer list.
	if (self -> scenery_renderers_frontier >= self -> scenery_renderers_capacity &&
		!MCMemoryResizeArray(self -> scenery_renderers_capacity != 0 ? self -> scenery_renderers_capacity * 2 : 4, self -> scenery_renderers, self -> scenery_renderers_capacity))
	{
		MCTileCacheInvalidate(self);
		return;
	}
	
	// Fill in the renderer list.
	self -> scenery_renderers[self -> scenery_renderers_frontier] . callback = x_layer . callback;
	self -> scenery_renderers[self -> scenery_renderers_frontier] . context = x_layer . context;
	
	// Update the scenery mapping.
	if (x_layer . id != 0)
		self -> scenery_map[x_layer . id] = t_layer_id;
	
	// Compute the affected and occluded tiles.
	MCTileCacheRectangle t_affected_cells, t_inside_cells;
	t_affected_cells = MCTileCacheComputeTouchedTiles(self, x_layer . region);
	t_inside_cells = MCTileCacheComputeOccludedTiles(self, x_layer . region);

	// Iterate over the affected frontiers from top to bottom.
	for(int32_t y = t_affected_cells . top; y < t_affected_cells . bottom; y++)
	{
		// Iterate over the affected frontiers from left to right.
		for(int32_t x = t_affected_cells . left; x < t_affected_cells . right; x++)
		{
			// Get the frontier ptr.
			MCTileCacheFrontier *t_frontier;
			t_frontier = &self -> frontiers[y * self -> tiles_across + x];
			
			// Skip the cell if it is completely occluded.
			if (MCTileCacheFrontierIsOccluded(t_frontier))
				continue;
				
			// Check to see if the tile occludes or not.
			bool t_occludes;
			if (x_layer . is_opaque && x_layer . opacity == 255)
				t_occludes = y >= t_inside_cells . top && y < t_inside_cells . bottom &&
								x >= t_inside_cells . left && x < t_inside_cells . right;
			else
				t_occludes = false;
				
			// If the frontier is empty, then update the first layer.
			if (MCTileCacheFrontierIsEmpty(t_frontier))
			{
				t_frontier -> first_layer = t_layer_id;
				t_frontier -> old_first_layer = x_layer . id;
			}
			
			// Always update the last layer.
			t_frontier -> last_layer = t_layer_id;
			t_frontier -> old_last_layer = x_layer . id;
			
			// If the cell needs flushing, then flush it (which also marks the
			// frontier as occluded).
			if (t_occludes)
				MCTileCacheRenderSceneryTile(self, x, y, t_frontier, true);
		}
	}
	
	// Update the layer id to return.
	x_layer . id = t_layer_id;
}

////////////////////////////////////////////////////////////////////////////////
//
//  SPRITE RENDERING
//

static void MCTileCacheRenderSpriteTile(MCTileCacheRef self, int32_t p_x, int32_t p_y, uint32_t p_id, bool p_is_opaque)
{
	// Fetch the sprite ptr.
	MCTileCacheSprite *t_sprite;
	t_sprite = MCTileCacheGetSprite(self, p_id);
	
	// See if the tile is already cached.
	uint32_t t_tile_index;
	t_tile_index = 0;
	if (p_x >= t_sprite -> left && p_x < t_sprite -> right && p_y >= t_sprite -> top && p_y < t_sprite -> bottom)
		t_tile_index = *MCTileCacheGetSpriteCell(self, p_id, p_x, p_y);
		
	// If the tile is already cached, then touch it, otherwise we must allocate
	// a new one and schedule rendering.
	MCTileCacheTile *t_tile;
	if (t_tile_index != 0)
	{
		MCTileCacheTouchTile(self, t_tile_index);
		t_tile = MCTileCacheGetTile(self, t_tile_index);
	}
	else
	{
		// Search for an unused tile.
		if (!MCTileCacheCreateTile(self, t_tile_index))
			return;
			
		// Get the tile's ptr
		t_tile = MCTileCacheGetTile(self, t_tile_index);
		
		// Update the tile's info to correspond to the sprite.
		t_tile -> first_layer = p_id;
		t_tile -> last_layer = 0;
		t_tile -> x = p_x;
		t_tile -> y = p_y;
		
		// Set the alpha appropriately. This allows us to use the information
		// sooner. More accurate determination will be done after rendering.
		t_tile -> alpha = p_is_opaque ? 255 : 127;
		
		// Push the tile onto the render list.
		MCTileCacheRenderListPush(self, self -> sprite_render_list, t_tile_index);
	}
	
	// Finally, push the tile onto the display list for compositing - but only if
	// it is not (fully) transparent.
	if (t_tile -> alpha != 0)
		MCTileCachePushCompositeOntoDisplayList(self, t_tile_index);
}

void MCTileCacheRenderSprite(MCTileCacheRef self, MCTileCacheLayer& x_layer)
{
	// If the tilecache isn't valid, do nothing.
	if (!self -> valid)
		return;
	
	// If the tilecache is clean, then reset the id.
	if (self -> clean)
		x_layer . id = 0;
	
	// If the layer id is 0, then we must create the sprite.
	if (x_layer . id == 0 &&
		!MCTileCacheInsertSprite(self, x_layer . callback, x_layer . context, x_layer . id))
		return;
	
	// Compute the visible region of the sprite in global co-ords.
	MCRectangle32 t_visible;
	t_visible = MCRectangle32Intersect(MCRectangle32FromMCRectangle(self -> viewport), MCRectangle32Intersect(x_layer . region, x_layer . clip));

	// Compute the region of the canvas touched by the sprite (in tiles).
	MCTileCacheRectangle t_affected_cells;
	t_affected_cells = MCTileCacheComputeTouchedTiles(self, t_visible);
	
	// Iterate over the affected frontiers from top to bottom.
	for(int32_t y = t_affected_cells . top; y < t_affected_cells . bottom; y++)
	{
		// Iterate over the affected frontiers from left to right.
		for(int32_t x = t_affected_cells . left; x < t_affected_cells . right; x++)
		{
			// Fetch a pointer to the frontier.
			MCTileCacheFrontier *t_frontier;
			t_frontier = &self -> frontiers[y * self -> tiles_across + x];
			
			// Skip the cell if it is occluded.
			if (MCTileCacheFrontierIsOccluded(t_frontier))
				continue;
				
			// Skip the cell if it is empty.
			if (MCTileCacheFrontierIsEmpty(t_frontier))
				continue;
				
			// Render the cell, which will also mark the frontier as either
			// empty or occluded, depending on any previously cached tile's
			// status.
			MCTileCacheRenderSceneryTile(self, x, y, t_frontier, false);
		}
	}
	
	// Get the sprite pointer.
	MCTileCacheSprite *t_sprite;
	t_sprite = MCTileCacheGetSprite(self, x_layer . id);

	// Compute the visible rect in tile co-ords.
	MCRectangle32 t_visible_tile_rect;
	t_visible_tile_rect = MCRectangle32Offset(t_visible, t_sprite->xorg - x_layer.region.x, t_sprite->yorg - x_layer.region.y);

	// Compute the visible tiles of the sprite.
	MCTileCacheRectangle t_visible_tiles;
	t_visible_tiles . left = MCTileCacheTileFloor(self, t_visible_tile_rect . x);
	t_visible_tiles . top = MCTileCacheTileFloor(self, t_visible_tile_rect . y);
	t_visible_tiles . right = MCTileCacheTileCeiling(self, t_visible_tile_rect . x + t_visible_tile_rect . width);
	t_visible_tiles . bottom = MCTileCacheTileCeiling(self, t_visible_tile_rect . y + t_visible_tile_rect . height);
	
	// Make sure the entire visible tiles rect fits in the sprites 'window'.
	// This might alter one or other of the visible tiles array and sprite
	// origin.
	if (!MCTileCacheExpandSprite(self, x_layer . id, t_visible_tiles))
		return;

	// This is true if we've already output the layer marker into the display
	// list.
	bool t_layer_begun;
	t_layer_begun = false;
	
	// Loop over the visible regin of the sprite, rendering any tiles that are
	// not occluded by scenery.
	for(int32_t y = t_visible_tiles . top; y < t_visible_tiles . bottom; y++)
	{
		// Compute the top and bottom canvas cell the current row of sprite tiles
		// covers - these can be between 0 and self -> tiles_down.
		int32_t t_canvas_top, t_canvas_bottom;
		t_canvas_top = MCMax(0, MCTileCacheTileFloor(self, x_layer . region . y + y * self -> tile_size - t_sprite -> yorg));
		t_canvas_bottom = MCMin(self -> tiles_down - 1, MCTileCacheTileCeiling(self, x_layer . region . y + y * self -> tile_size - t_sprite -> yorg));
		for(int32_t x = t_visible_tiles . left; x < t_visible_tiles . right; x++)
		{
			// Compute the left and right canvas cell the current row of sprite
			// tiles covers - these can be between 0 and self -> tiles_across.
			int32_t t_canvas_left, t_canvas_right;
			t_canvas_left = MCMax(0, MCTileCacheTileFloor(self, x_layer . region . x + x * self -> tile_size - t_sprite -> xorg));
			t_canvas_right = MCMin(self -> tiles_across - 1, MCTileCacheTileCeiling(self, x_layer . region . x + x * self -> tile_size - t_sprite -> xorg));

			// Check to see if the sprite's tile is occluded
			if (MCTileCacheFrontierIsOccluded(&self -> frontiers[t_canvas_top * self -> tiles_across + t_canvas_left]) &&
				MCTileCacheFrontierIsOccluded(&self -> frontiers[t_canvas_bottom * self -> tiles_across + t_canvas_left]) &&
				MCTileCacheFrontierIsOccluded(&self -> frontiers[t_canvas_top * self -> tiles_across + t_canvas_right]) &&
				MCTileCacheFrontierIsOccluded(&self -> frontiers[t_canvas_bottom * self -> tiles_across + t_canvas_right]))
				continue;

			if (!t_layer_begun)
			{
				// We push the end-layer marker first, since the display list
				// is run backwards (process front-to-back, render back-to-
				// front).
				MCTileCachePushEndLayerOntoDisplayList(self);
				t_layer_begun = true;
			}

			MCTileCacheRenderSpriteTile(self, x, y, x_layer . id, x_layer . is_opaque);
		}
	}
	
	// Push the layer 'begin' marker but only if we actually rendered any sprite
	// tiles.
	if (t_layer_begun)
		MCTileCachePushBeginLayerOntoDisplayList(self,
					-t_sprite -> xorg + x_layer . region . x - self -> viewport . x, -t_sprite -> yorg + x_layer . region . y - self -> viewport . y,
					x_layer . clip,
					x_layer . opacity,
					x_layer . ink);
					
	// Finally, if the sprite is opaque, then occlude any frontiers that the
	// sprite covers. (Note only do this if we are rendering with full opacity).
	if (x_layer . is_opaque && x_layer . opacity == 255)
	{
		// Compute the region of the canvas touched by the sprite.
		MCTileCacheRectangle t_inside_cells;
		t_inside_cells = MCTileCacheComputeOccludedTiles(self, t_visible);

		// Iterate over the affected frontiers from top to bottom.
		for(int32_t y = t_inside_cells . top; y < t_inside_cells . bottom; y++)
		{
			// Iterate over the affected frontiers from left to right.
			for(int32_t x = t_inside_cells . left; x < t_inside_cells . right; x++)
			{
				// Fetch a pointer to the frontier.
				MCTileCacheFrontier *t_frontier;
				t_frontier = &self -> frontiers[y * self -> tiles_across + x];

				// Mark the cell as occluded.
				MCTileCacheFrontierSetOccluded(t_frontier);
			}
		}
	}
} 

////////////////////////////////////////////////////////////////////////////////

static bool MCTileCacheDoComposite(MCTileCacheRef self)
{
	if (self -> display_list_frontier == 0)
		return true;
	
	bool t_success;
	t_success = true;
	
	// Keep track of whether we are in a layer or not.
	bool t_in_layer;
	t_in_layer = false;
	
	// Keep track of the current layer origin.
	int32_t t_ox, t_oy;
	t_ox = self -> viewport . x;
	t_oy = self -> viewport . y;
	
	// Keep track of the instruction we are on - this loop terminates when
	// t_index reaches zero, or the compositor returns false.
	uint32_t t_index;
	t_index = self -> display_list_frontier;
	do
	{
		// Move to the next index.
		t_index -= 1;
		
		// Check to see what we should do.
		if (self -> display_list[t_index] != 0)
		{
			// Non-zero means composite the tile, so first get the tile ptr.
			MCTileCacheTile *t_tile;
			t_tile = MCTileCacheGetTile(self, self -> display_list[t_index]);
			
			// Now compute its location.
			int32_t t_x, t_y;
			t_x = t_ox + t_tile -> x * self -> tile_size;
			t_y = t_oy + t_tile -> y * self -> tile_size;
			
			// Finally tell the compositor about it.
			if (t_tile -> constant == 0)
			{
				if (self -> compositor . composite_tile != nil)
					t_success = self -> compositor . composite_tile(self -> compositor . context, t_x, t_y, t_tile -> data);
			}
			else if (t_tile -> alpha != 0)
			{
				if (self -> compositor . composite_rect != nil)
					t_success = self -> compositor . composite_rect(self -> compositor . context, t_x, t_y, (uintptr_t)(t_tile -> data));
			}
		}
		else if (!t_in_layer)
		{
			// A zero means start a layer if not in one.
			MCRectangle32 t_clip;
			uint32_t t_ink, t_opacity;
			
			// IM-2014-02-28: [[ Bug 11617 ]] Call function to get layer values from display list
			MCTileCachePopLayerFromDisplayList(self->display_list, t_index, t_ox, t_oy, t_clip, t_opacity, t_ink);
			
			// Now we notify the compositor about starting a new layer.
			if (self -> compositor . begin_layer != nil)
				t_success = self -> compositor . begin_layer(self -> compositor . context, MCRectangle32ToMCRectangle(t_clip), t_opacity, t_ink);
			
			// Mark ourselves as being within a layer.
			t_in_layer = true;
		}
		else
		{
			// A zero means end layer if in one. So reset the origin.
			t_ox = 0;
			t_oy = 0;
			
			// Notify the compositor.
			if (self -> compositor . end_layer != nil)
				t_success = self -> compositor . end_layer(self -> compositor . context);
			
			// Mark ourselves as being outside a layer.
			t_in_layer = false;
		}
	}
	while(t_index != 0 && t_success);
	
	return t_success;
}

bool MCTileCacheComposite(MCTileCacheRef self, MCStackSurface *p_surface, MCGRegionRef p_dirty_rgn)
{
	// Keep track of whether the compositor calls succeeded.
	bool t_success;
	t_success = true;

	// First notify of a beginning of frame.
	if (t_success && self -> compositor . begin_frame)
		t_success = self -> compositor . begin_frame(self -> compositor . context, p_surface, p_dirty_rgn);

	// Next run through the display list (backwards).
	if (t_success)
		t_success = MCTileCacheDoComposite(self);
	
	// Final step, tell the compositor we are done.
	if (t_success && self -> compositor . end_frame != nil)
		t_success = self -> compositor . end_frame(self -> compositor . context, p_surface);

	return t_success;
}

bool MCTileCacheSnapshot(MCTileCacheRef self, MCRectangle p_area, MCGImageRef& r_image)
{
	if (self -> compositor . begin_snapshot == nil ||
		self -> compositor . end_snapshot == nil)
		return false;
	
	bool t_success;
	t_success = true;
    
	MCGRaster t_raster;
	t_raster.width = p_area.width;
	t_raster.height = p_area.height;
	t_raster.pixels = nil;
	t_raster.stride = p_area.width * sizeof(uint32_t);
	// IM-2014-05-20: [[ GraphicsPerformance ]] Use opaque raster format for snapshot
	t_raster.format = kMCGRasterFormat_xRGB;
	
	if (t_success)
		t_success = MCMemoryAllocate(t_raster.stride * t_raster.height, t_raster.pixels);
	
	if (t_success)
		t_success = self -> compositor . begin_snapshot(self -> compositor . context, p_area, t_raster);
	
	if (t_success)
		t_success = MCTileCacheDoComposite(self);
	
	if (t_success)
		t_success = self -> compositor . end_snapshot(self -> compositor . context, p_area, t_raster);
	
	if (t_success)
	{
		t_success = MCGImageCreateWithRasterAndRelease(t_raster, r_image);
		if (t_success)
			t_raster.pixels = nil;
	}

	MCMemoryDeallocate(t_raster.pixels);
	
	return t_success;
}
