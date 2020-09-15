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
#include "packed.h"
#include "mbldc.h"

#include <pthread.h>

#if defined(_IOS_MOBILE)

#import <OpenGLES/ES3/gl.h>
#import <OpenGLES/ES3/glext.h>

#elif defined(TARGET_SUBPLATFORM_ANDROID)

#define GL_GLEXT_PROTOTYPES
#include <GLES3/gl3.h>
#include <GLES3/gl3ext.h>
#include <EGL/egl.h>

#else
#error tilecachegl3.x.cpp not supported on this platform
#endif

#include "glcontext.h"

////////////////////////////////////////////////////////////////////////////////

#define kSuperTileSize 256
#define kVertexBufferSize 16 * 4

template <class T>
struct vertex_buffer
{
	T vertices[kVertexBufferSize];
	uindex_t count;
};

struct super_tile
{
	GLuint texture;
	vertex_buffer<MCGLTextureVertex> vertex_buffer;
	uint32_t free_count;
	uint8_t free_list[1];
};

struct MCTileCacheOpenGLCompositorContext
{
	// The tilecache
	MCTileCacheRef tilecache;
	
	// The tilesize in use.
	int32_t tile_size;
	
	// The number of tiles per super-tile.
	uint32_t super_tile_arity;
	
	// The array of super-tiles currently allocated.
	super_tile **super_tiles;
	uint32_t super_tile_count;
	
	// The currently bound texture.
	uint32_t current_texture;
	
	// The height of the viewport.
	int32_t viewport_height;
	
	// The current opacity (only needed for rects)
	uint32_t current_opacity;
	
	// If true, then blending is enabled.
	bool is_blending : 1;
	
	// If true, then we are configured for filling rather than texturing.
	bool is_filling : 1;
	
	// The color vertex buffer
	vertex_buffer<MCGLColorVertex> color_vertex_buffer;

	// This is set to true if the super tiles need to be forced flushed
	// (due to a Flush call).
	bool needs_flush;
	
	// If this is true, rendering is flipped in the y direction (relative to
	// 'normal' for LiveCode origin).
	bool flip_y;
	
	// The gl context
	MCGLContextRef gl_context;

	// The original framebuffer (used when doing a snapshot).
	GLuint original_draw_framebuffer;
	GLuint original_read_framebuffer;
	
	// The snapshot framebuffer and renderbuffer (only valid between
	// BeginSnapshot and EndSnapshot).
	GLuint snapshot_framebuffer;
	GLuint snapshot_renderbuffer;
	
	// The origin of the current render (used by snapshot).
	GLint origin_x, origin_y;
};

////////////////////////////////////////////////////////////////////////////////

static inline void MCTileCacheOpenGLCompositorDecodeTile(MCTileCacheOpenGLCompositorContext *self, uintptr_t p_tile, uint32_t& r_super_tile, uint32_t& r_sub_tile)
{
	r_super_tile = (p_tile & 0xffff) - 1;
	r_sub_tile = (p_tile >> 16) - 1;
}

static inline void MCTileCacheOpenGLCompositorEncodeTile(MCTileCacheOpenGLCompositorContext *self, uint32_t p_super_tile, uint32_t p_sub_tile, uint32_t& r_tile)
{
	r_tile = (p_super_tile + 1) | ((p_sub_tile + 1) << 16);
}

static bool MCTileCacheOpenGLCompositorCreateTile(MCTileCacheOpenGLCompositorContext *self, uint32_t& r_tile)
{
	// Loop through backwards and find the first super_tile with free space. (We loop
	// backwards under the assumption that more recently allocated tiles are less
	// likely to be freed soon).
	for(uint32_t i = self -> super_tile_count; i > 0; i--)
	{
		super_tile *t_super_tile;
		t_super_tile = self -> super_tiles[i - 1];
		if (t_super_tile == nil)
			continue;
		
		// We found a free tile.
		if (t_super_tile -> free_count > 0)
		{
			// Compute the id.
			MCTileCacheOpenGLCompositorEncodeTile(self, i - 1, t_super_tile -> free_list[--t_super_tile -> free_count], r_tile);
			return true;
		}
	}
	
	// First look for an empty slot.
	uint32_t t_super_tile_index;
	t_super_tile_index = self -> super_tile_count;
	for(uint32_t i = 0; i < self -> super_tile_count; i++)
		if (self -> super_tiles[i] == nil)
		{
			t_super_tile_index = i;
			break;
		}
	
	// Extend the array if needed.
	if (t_super_tile_index == self -> super_tile_count)
	{
		if (!MCMemoryResizeArray(self -> super_tile_count + 1, self -> super_tiles, self -> super_tile_count))
			return false;
	}
	
	// We failed to find an empty super-tile so we must allocate a new one.
	super_tile *t_super_tile;
	if (!MCMemoryAllocate(offsetof(super_tile, free_list) + self -> super_tile_arity, t_super_tile))
		return false;
	
	
	// Generate a texture id, and bind an empty texture.
	glGenTextures(1, &t_super_tile -> texture);
	glBindTexture(GL_TEXTURE_2D, t_super_tile -> texture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	
	// IM_2013-08-21: [[ RefactorGraphics ]] set iOS pixel format to RGBA
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, kSuperTileSize, kSuperTileSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, nil);
	
	// Fill in the free list - subtile 0 is the one we will allocate.
	for(uint32_t i = 1; i < self -> super_tile_arity; i++)
		t_super_tile -> free_list[i - 1] = i;
	t_super_tile -> free_count = self -> super_tile_arity - 1;
	
	t_super_tile->vertex_buffer.count = 0;

	// Set the slot.
	self -> super_tiles[t_super_tile_index] = t_super_tile;
	
	// Make sure we don't make redundent bind calls.
	self -> current_texture = t_super_tile -> texture;

	// And compute the id (note the subtile index is 0!).
	MCTileCacheOpenGLCompositorEncodeTile(self, t_super_tile_index, 0, r_tile);

	return true;
}

static void MCTileCacheOpenGLCompositorDestroyTile(MCTileCacheOpenGLCompositorContext *self, uintptr_t p_tile)
{
	// Fetch the super/sub indices of the tile.
	uint32_t t_super_tile_index, t_sub_tile_index;
	MCTileCacheOpenGLCompositorDecodeTile(self, p_tile, t_super_tile_index, t_sub_tile_index);
	
	// Fetch the super tile structure.
	super_tile *t_super_tile;
	t_super_tile = self -> super_tiles[t_super_tile_index];

	// Push the sub tile index onto the free list. We will free any unused textures
	// at the next 'endtiling' point since we may not be in a OpenGL call safe
	// environment at the moment. (DestroyTile occurs whenever layer mutation occurs
	// not when processing frames).
	t_super_tile -> free_list[t_super_tile -> free_count++] = t_sub_tile_index;
}

static void MCTileCacheOpenGLCompositorFlushSuperTiles(MCTileCacheOpenGLCompositorContext *self, bool p_force)
{
	// Loop through all our supertiles, discarding any which are empty.
	for(uint32_t i = 0; i < self -> super_tile_count; i++)
	{
		if (self -> super_tiles[i] == nil)
			continue;
		
		if (p_force || self -> super_tiles[i] -> free_count == self -> super_tile_arity)
		{
			// deleting the currently bound texture reverts the binding to zero
			if (self->current_texture == self->super_tiles[i]->texture)
				self->current_texture = 0;
			glDeleteTextures(1, &self -> super_tiles[i] -> texture);
			MCMemoryDelete(self -> super_tiles[i]);
			self -> super_tiles[i] = nil;
			
		}
	}
}

bool MCTileCacheOpenGLCompositor_BeginTiling(void *p_context)
{
	MCTileCacheOpenGLCompositorContext *self;
	self = (MCTileCacheOpenGLCompositorContext *)p_context;
	
	// Reset the current texture.
	self -> current_texture = 0;
	
	// Update the tile size.
	self -> tile_size = MCTileCacheGetTileSize(self -> tilecache);
	
	// Compute the super tile arity.
	uint32_t t_super_tile_arity;
	t_super_tile_arity = (kSuperTileSize * kSuperTileSize) / (self -> tile_size * self -> tile_size);
	
	// If the arity has changed or needs flush is set, flush all existing (what must be empty)
	// super_tiles.
	if (t_super_tile_arity != self -> super_tile_arity ||
		self -> needs_flush)
	{
		// MW-2013-06-26: [[ Bug 10991 ]] Don't force flushing of the tilecache, otherwise
		//   we end up discarding tiles that are still needed after a 'compact()' operation.
		MCTileCacheOpenGLCompositorFlushSuperTiles(self, false);
	
		self -> super_tile_arity = t_super_tile_arity;
		self -> needs_flush = false;
	}
	
	// Clear any error flags from OpenGL so we can detect failure.
	/* UNCHECKED */ MCGLCheckError(nil);
	
	return true;
}

bool MCTileCacheOpenGLCompositor_EndTiling(void *p_context)
{
	MCTileCacheOpenGLCompositorContext *self;
	self = (MCTileCacheOpenGLCompositorContext *)p_context;
	
	// Flush any empty textures.
	MCTileCacheOpenGLCompositorFlushSuperTiles(self, false);
	
	// If an OpenGL error occurred, then return false.
	return MCGLCheckError("EndTiling - ");
}

bool MCTileCacheOpenGLCompositor_AllocateTile(void *p_context, int32_t p_size, const void *p_bits, uint32_t p_stride, void*& r_tile)
{
	MCTileCacheOpenGLCompositorContext *self;
	self = (MCTileCacheOpenGLCompositorContext *)p_context;

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
	
	void *t_tile;
	uint32_t t_tile_id;
	t_tile = nil;
	if (t_data != nil && MCTileCacheOpenGLCompositorCreateTile(self, t_tile_id))
	{
		// Fetch the super/sub indices of the tile.
		uint32_t t_super_tile_index, t_sub_tile_index;
		MCTileCacheOpenGLCompositorDecodeTile(self, t_tile_id, t_super_tile_index, t_sub_tile_index);
		
		// Now fetch the texture and bind it if necessary.
		GLuint t_texture;
		t_texture = self -> super_tiles[t_super_tile_index] -> texture;
		if (t_texture != self -> current_texture)
		{
			glBindTexture(GL_TEXTURE_2D, t_texture);
			self -> current_texture = t_texture;
		}
		
		// Calculate its location.
		GLint t_x, t_y;
		t_x = t_sub_tile_index % (kSuperTileSize / self -> tile_size);
		t_y = t_sub_tile_index / (kSuperTileSize / self -> tile_size);
		
		// Fill the texture.
		// IM_2013-08-21: [[ RefactorGraphics ]] set iOS pixel format to RGBA
		glTexSubImage2D(GL_TEXTURE_2D, 0, t_x * self -> tile_size, t_y * self -> tile_size, self -> tile_size, self -> tile_size, GL_RGBA, GL_UNSIGNED_BYTE, t_data);
		// SN-2015-04-13: [[ Bug 14879 ]] This function seems to fail sometimes,
		//  and we want to get the error here, not in
		//  MCTileCacheOpenGLCompositorFlushSuperTiles as it happens in the
		//  stack attached to the bug report.
		GLenum t_error = glGetError();
		if (t_error != GL_NO_ERROR)
			MCLog("glTextSubImage2D(x,x,%d,%d,%d,%d,...) returned error 0x%X", t_x * self -> tile_size, t_y * self -> tile_size, self -> tile_size, self -> tile_size, t_error);

		// Set the tile id.
		t_tile = (void *)t_tile_id;
	}
	
	if (t_data != p_bits)
		MCMemoryDeallocate(t_data);
	
	if (t_tile == nil)
		return false;
	
	r_tile = t_tile;
	
	return true;
}

void MCTileCacheOpenGLCompositor_DeallocateTile(void *p_context, void *p_tile)
{
	MCTileCacheOpenGLCompositorContext *self;
	self = (MCTileCacheOpenGLCompositorContext *)p_context;
	
	MCTileCacheOpenGLCompositorDestroyTile(self, (uintptr_t)p_tile);
}

static bool MCTileCacheOpenGLCompositorFlushTextureVertexBuffer(MCTileCacheOpenGLCompositorContext *self, super_tile *p_super_tile)
{
	if (p_super_tile->vertex_buffer.count == 0)
		return true;

	if (p_super_tile->texture != self->current_texture)
	{
		glBindTexture(GL_TEXTURE_2D, p_super_tile->texture);
		self -> current_texture = p_super_tile->texture;
	}

	if (self->is_filling)
	{
		self->is_filling = false;
		MCGLContextSelectProgram(self->gl_context, kMCGLProgramTypeTexture);
	}
	
	glBufferData(GL_ARRAY_BUFFER, sizeof(MCGLTextureVertex) * p_super_tile->vertex_buffer.count, p_super_tile->vertex_buffer.vertices, GL_STREAM_DRAW);

	for (uindex_t i = 0; i < p_super_tile->vertex_buffer.count; i += 4)
	{
		glDrawArrays(GL_TRIANGLE_STRIP, i, 4);
	}

	p_super_tile->vertex_buffer.count = 0;

	return true;
}

static bool MCTileCacheOpenGLCompositorFlushColorVertexBuffer(MCTileCacheOpenGLCompositorContext *self)
{
	if (self->color_vertex_buffer.count == 0)
		return true;

	if (!self->is_filling)
	{
		self->is_filling = true;
		MCGLContextSelectProgram(self->gl_context, kMCGLProgramTypeColor);
	}

	glBufferData(GL_ARRAY_BUFFER, sizeof(MCGLColorVertex) * self->color_vertex_buffer.count, self->color_vertex_buffer.vertices, GL_STREAM_DRAW);

	for (uindex_t i = 0; i < self->color_vertex_buffer.count; i += 4)
	{
		glDrawArrays(GL_TRIANGLE_STRIP, i, 4);
	}

	self->color_vertex_buffer.count = 0;

	return true;
}

static bool MCTileCacheOpenGLCompositorFlushVertexBuffers(MCTileCacheOpenGLCompositorContext *self)
{
	if (!MCTileCacheOpenGLCompositorFlushColorVertexBuffer(self))
		return false;

	for (uindex_t i = 0; i < self->super_tile_count; i++)
	{
		if (self->super_tiles[i] != nil && !MCTileCacheOpenGLCompositorFlushTextureVertexBuffer(self, self->super_tiles[i]))
			return false;
	}

	return true;
}

static void MCTileCacheOpenGLCompositor_PrepareFrame(MCTileCacheOpenGLCompositorContext *self, MCGAffineTransform p_world_transform)
{
#if defined(_ANDROID_MOBILE)
	/* On android the soft keyboard changes the surface height but we do not
	 * update the tilecache height to match. To handle this we get the current
	 * surface height to enusre the correct transform is applied. */
	EGLint t_height;
	eglQuerySurface(eglGetCurrentDisplay(),
					eglGetCurrentSurface(EGL_DRAW),
					EGL_HEIGHT,
					&t_height);

	self -> viewport_height = t_height;
#else
	self -> viewport_height = MCTileCacheGetViewport(self -> tilecache) . height;
#endif
	self -> current_texture = 0;
	self -> current_opacity = 255;
	self -> is_filling = false;
	self -> is_blending = false;
	
	// Set the texture matrix.
	MCGFloat t_texture_scale;
	t_texture_scale = ((float)self->tile_size) / ((float)kSuperTileSize);
	MCGAffineTransform t_texture_transform;
	t_texture_transform = MCGAffineTransformMakeScale(t_texture_scale, t_texture_scale);

	MCGAffineTransform t_world_transform;
	t_world_transform = MCGAffineTransformMakeIdentity();
	if (!self->flip_y)
	{
		t_world_transform = MCGAffineTransformPostTranslate(t_world_transform, 0.0, self->viewport_height);
		t_world_transform = MCGAffineTransformPostScale(t_world_transform, 1.0, -1.0);
	}
	t_world_transform = MCGAffineTransformConcat(t_world_transform, p_world_transform);

	// initialize variables of color & texture shader programs
	MCGLContextSelectProgram(self->gl_context, kMCGLProgramTypeColor);
	MCGLContextSetWorldTransform(self->gl_context, t_world_transform);

	MCGLContextSelectProgram(self->gl_context, kMCGLProgramTypeTexture);
	MCGLContextSetWorldTransform(self->gl_context, t_world_transform);
	MCGLContextSetTextureTransform(self->gl_context, t_texture_transform);
	
	// Initialize the blend function we would use.
	glDisable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
}

bool MCTileCacheOpenGLCompositor_BeginFrame(void *p_context, MCStackSurface *p_surface, MCGRegionRef p_dirty)
{
	MCTileCacheOpenGLCompositorContext *self;
	self = (MCTileCacheOpenGLCompositorContext *)p_context;
	
	void *t_target;
	if (!p_surface -> LockTarget(kMCStackSurfaceTargetEAGLContext, t_target))
		return false;
	
	// MW-2013-03-12: [[ Bug 10705 ]] Reset the origin to 0.
	self -> flip_y = false;
	self -> origin_x = 0;
	self -> origin_y = 0;
	
	MCTileCacheOpenGLCompositor_PrepareFrame(self, MCGAffineTransformMakeIdentity());
	
	return true;
}

bool MCTileCacheOpenGLCompositor_EndFrame(void *p_context, MCStackSurface *p_surface)
{
	MCTileCacheOpenGLCompositorContext *self;
	self = (MCTileCacheOpenGLCompositorContext *)p_context;
	
	MCTileCacheOpenGLCompositorFlushVertexBuffers(self);

	p_surface -> UnlockTarget();
	
	return MCGLCheckError("EndFrame - ");
}

bool MCTileCacheOpenGLCompositor_BeginSnapshot(void *p_context, MCRectangle p_area, MCGRaster &p_target)
{
	MCTileCacheOpenGLCompositorContext *self;
	self = (MCTileCacheOpenGLCompositorContext *)p_context;
	
	glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, (GLint *)&self -> original_draw_framebuffer);
	glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, (GLint *)&self -> original_read_framebuffer);
	
	glGenFramebuffers(1, &self -> snapshot_framebuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, self -> snapshot_framebuffer);
	
	glGenRenderbuffers(1, &self -> snapshot_renderbuffer);
	glBindRenderbuffer(GL_RENDERBUFFER, self -> snapshot_renderbuffer);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA8, p_area . width, p_area . height);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, self -> snapshot_renderbuffer);

	GLenum t_status;
	t_status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (t_status != GL_FRAMEBUFFER_COMPLETE)
	{
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, self -> original_draw_framebuffer);
		glBindFramebuffer(GL_READ_FRAMEBUFFER, self -> original_read_framebuffer);
		glDeleteRenderbuffers(1, &self -> snapshot_renderbuffer);
		glDeleteFramebuffers(1, &self -> snapshot_framebuffer);
		return false;
	}
	
	// MW-2013-03-12: [[ Bug 10705 ]] Set the origin of the snapshot so layer clipping works
	self -> flip_y = true;
	self -> origin_x = -p_area . x;
	self -> origin_y = -p_area . y;
	
	// MW-2013-06-07: [[ Bug 10705 ]] Make sure we translate the viewport so the correct area
	//   gets rendered (and snapshotted).
	MCGAffineTransform t_world_transform;
	t_world_transform = MCGAffineTransformMakeTranslation(-p_area.x, -p_area.y);

	MCTileCacheOpenGLCompositor_PrepareFrame(self, t_world_transform);
	
	return true;
}

bool MCTileCacheOpenGLCompositor_EndSnapshot(void *p_context, MCRectangle p_area, MCGRaster &p_target)
{
	MCTileCacheOpenGLCompositorContext *self;
	self = (MCTileCacheOpenGLCompositorContext *)p_context;
	
	MCTileCacheOpenGLCompositorFlushVertexBuffers(self);
	
	/* OVERHAUL - REVISIT: check byte order? */
	// t_bitmap -> is_swapped = true;
	glReadPixels(0, 0, p_area . width, p_area . height, GL_RGBA, GL_UNSIGNED_BYTE, p_target . pixels);
	
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, self -> original_draw_framebuffer);
	glBindFramebuffer(GL_READ_FRAMEBUFFER, self -> original_read_framebuffer);

	glDeleteRenderbuffers(1, &self -> snapshot_renderbuffer);
	glDeleteFramebuffers(1, &self -> snapshot_framebuffer);
	
	return true;
}

bool MCTileCacheOpenGLCompositor_BeginLayer(void *p_context, const MCRectangle& p_clip, uint32_t p_opacity, uint32_t p_ink)
{
	MCTileCacheOpenGLCompositorContext *self;
	self = (MCTileCacheOpenGLCompositorContext *)p_context;

	MCTileCacheOpenGLCompositorFlushVertexBuffers(self);
	glEnable(GL_SCISSOR_TEST);
	
	// MW-2012-09-18: [[ Bug 10202 ]] If the ink is no-op then ensure nothing happens.
	if (p_ink == GXnoop)
		glScissor(0, 0, 0, 0);
	else
	{
		// MW-2013-03-12: [[ Bug 10705 ]] Adjust the scissor rect by the origin of the snapshot (scissor is in device, not user space).
		if (!self -> flip_y)
			glScissor(p_clip . x + self -> origin_x, self -> viewport_height - (p_clip . y + p_clip . height) + self -> origin_y, p_clip . width, p_clip . height);
		else
			glScissor(p_clip . x + self -> origin_x, p_clip . y + self -> origin_y, p_clip . width, p_clip . height);
	}
	
	if (!self -> is_blending)
	{
		self -> is_blending = true;
		glEnable(GL_BLEND);
	}
		
	self -> current_opacity = p_opacity;
	
	return true;
}

bool MCTileCacheOpenGLCompositor_EndLayer(void *p_context)
{
	MCTileCacheOpenGLCompositorContext *self;
	self = (MCTileCacheOpenGLCompositorContext *)p_context;
	
	MCTileCacheOpenGLCompositorFlushVertexBuffers(self);

	self -> current_opacity = 255;
	
	glDisable(GL_SCISSOR_TEST);
	
	return true;
}

bool MCTileCacheOpenGLCompositor_CompositeTile(void *p_context, int32_t p_x, int32_t p_y, void *p_tile)
{
	if (p_tile == nil)
		return false;
	
	MCTileCacheOpenGLCompositorContext *self;
	self = (MCTileCacheOpenGLCompositorContext *)p_context;

	uint32_t t_super_tile_index, t_sub_tile_index;
	MCTileCacheOpenGLCompositorDecodeTile(self, (uintptr_t)p_tile, t_super_tile_index, t_sub_tile_index);
	
	super_tile *t_super_tile;
	t_super_tile = self->super_tiles[t_super_tile_index];

	if (t_super_tile->vertex_buffer.count + 4 > kVertexBufferSize)
	{
		if (!MCTileCacheOpenGLCompositorFlushTextureVertexBuffer(self, t_super_tile))
			return false;
	}
	
	int32_t t_tile_x, t_tile_y;
	t_tile_x = t_sub_tile_index % (kSuperTileSize / self -> tile_size);
	t_tile_y = t_sub_tile_index / (kSuperTileSize / self -> tile_size);
	
	MCGLTextureVertex *t_vertices;
	t_vertices = &t_super_tile->vertex_buffer.vertices[t_super_tile->vertex_buffer.count];
	t_vertices[0].position[0] = p_x;                   t_vertices[0].position[1] = p_y + self->tile_size;
	t_vertices[0].texture_position[0] = t_tile_x;      t_vertices[0].texture_position[1] = t_tile_y + 1;
	t_vertices[1].position[0] = p_x + self->tile_size; t_vertices[1].position[1] = p_y + self->tile_size;
	t_vertices[1].texture_position[0] = t_tile_x + 1;  t_vertices[1].texture_position[1] = t_tile_y + 1;
	t_vertices[2].position[0] = p_x;                   t_vertices[2].position[1] = p_y;
	t_vertices[2].texture_position[0] = t_tile_x;      t_vertices[2].texture_position[1] = t_tile_y;
	t_vertices[3].position[0] = p_x + self->tile_size; t_vertices[3].position[1] = p_y;
	t_vertices[3].texture_position[0] = t_tile_x + 1;  t_vertices[3].texture_position[1] = t_tile_y;

	t_super_tile->vertex_buffer.count += 4;

	return true;
}

bool MCTileCacheOpenGLCompositor_CompositeRect(void *p_context, int32_t p_x, int32_t p_y, uint32_t p_color)
{
	MCTileCacheOpenGLCompositorContext *self;
	self = (MCTileCacheOpenGLCompositorContext *)p_context;
	
	if (self->color_vertex_buffer.count + 4 > kVertexBufferSize)
	{
		if (!MCTileCacheOpenGLCompositorFlushColorVertexBuffer(self))
			return false;
	}

	// MW-2014-03-14: [[ Bug 11880 ]] The color field we store is premultiplied by the current
	//   opacity.
	uint32_t t_new_color;
	t_new_color = packed_scale_bounded(p_color, self->current_opacity);
	uint32_t t_rgba_color;
	{
		uint8_t r, g, b, a;
		MCGPixelUnpackNative(t_new_color, r, g, b, a);
		t_rgba_color = MCGPixelPack(kMCGPixelFormatRGBA, r, g, b, a);
	}

	MCGLColorVertex *t_vectors = &self->color_vertex_buffer.vertices[self->color_vertex_buffer.count];
	t_vectors[0].position[0] = p_x; t_vectors[0].position[1] = p_y + self->tile_size;
	t_vectors[0].color = t_rgba_color;
	t_vectors[1].position[0] = p_x + self->tile_size; t_vectors[1].position[1] = p_y + self->tile_size;
	t_vectors[1].color = t_rgba_color;
	t_vectors[2].position[0] = p_x; t_vectors[2].position[1] = p_y;
	t_vectors[2].color = t_rgba_color;
	t_vectors[3].position[0] = p_x + self->tile_size; t_vectors[3].position[1] = p_y;
	t_vectors[3].color = t_rgba_color;

	self->color_vertex_buffer.count += 4;

	return true;
}

void MCTileCacheOpenGLCompositor_Cleanup(void *p_context)
{
	MCTileCacheOpenGLCompositorContext *self;
	self = (MCTileCacheOpenGLCompositorContext *)p_context;
	
	MCTileCacheOpenGLCompositorFlushSuperTiles(self, true);
	MCMemoryDeleteArray(self -> super_tiles);
	MCMemoryDelete(self);
	
	MCPlatformDisableOpenGLMode();
}

void MCTileCacheOpenGLCompositor_Flush(void *p_context)
{
	MCTileCacheOpenGLCompositorContext *self;
	self = (MCTileCacheOpenGLCompositorContext *)p_context;

	self -> needs_flush = true;
}

////////////////////////////////////////////////////////////////////////////////

bool MCTileCacheOpenGLCompositorConfigure(MCTileCacheRef p_tilecache, MCTileCacheCompositor& r_compositor)
{
	MCTileCacheOpenGLCompositorContext *t_context;
	if (!MCMemoryNew(t_context))
		return false;
	
	t_context -> tilecache = p_tilecache;
	
	r_compositor . context = t_context;
	r_compositor . cleanup = MCTileCacheOpenGLCompositor_Cleanup;
	r_compositor . flush = MCTileCacheOpenGLCompositor_Flush;
	r_compositor . begin_tiling = MCTileCacheOpenGLCompositor_BeginTiling;
	r_compositor . end_tiling = MCTileCacheOpenGLCompositor_EndTiling;
	r_compositor . allocate_tile = MCTileCacheOpenGLCompositor_AllocateTile;
	r_compositor . deallocate_tile = MCTileCacheOpenGLCompositor_DeallocateTile;
	r_compositor . begin_frame = MCTileCacheOpenGLCompositor_BeginFrame;
	r_compositor . end_frame = MCTileCacheOpenGLCompositor_EndFrame;
	r_compositor . begin_layer = MCTileCacheOpenGLCompositor_BeginLayer;
	r_compositor . end_layer = MCTileCacheOpenGLCompositor_EndLayer;
	r_compositor . composite_tile = MCTileCacheOpenGLCompositor_CompositeTile;
	r_compositor . composite_rect = MCTileCacheOpenGLCompositor_CompositeRect;
	r_compositor . begin_snapshot = MCTileCacheOpenGLCompositor_BeginSnapshot;
	r_compositor . end_snapshot = MCTileCacheOpenGLCompositor_EndSnapshot;
	
	MCPlatformEnableOpenGLMode();
	t_context->gl_context = MCPlatformGetOpenGLContext();

	MCAssert(t_context->gl_context != nil);
	
	return true;
}

////////////////////////////////////////////////////////////////////////////////
