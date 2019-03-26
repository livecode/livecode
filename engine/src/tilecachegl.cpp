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
#include <CoreGraphics/CoreGraphics.h>
#import <OpenGLES/ES1/gl.h>
#import <OpenGLES/ES1/glext.h>
extern void MCIPhoneSwitchToUIKit(void);
extern void MCIPhoneSwitchToOpenGL(void);
#elif defined(TARGET_SUBPLATFORM_ANDROID)
#define GL_GLEXT_PROTOTYPES
#include <GLES/gl.h>
#include <GLES/glext.h>
#include <EGL/egl.h>
extern void MCAndroidEnableOpenGLMode(void);
extern void MCAndroidDisableOpenGLMode(void);
#else
#error tilecachegl.cpp not supported on this platform
#endif

////////////////////////////////////////////////////////////////////////////////


#define kSuperTileSize 256

struct super_tile
{
	GLuint texture;
	uint32_t free_count;
	uint8_t free_list[1];
};

enum MCTileCacheOpenGLVersion
{
	kMCTileCacheOpenGLVersionUnknown = 0,
	kMCTileCacheOpenGLVersion1_0 = 1,
	kMCTileCacheOpenGLVersion1_1 = 2,
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
	
	// The last color applied, or 0 if it was a texture
	uint32_t current_color;
	
	// The current opacity (only needed for rects)
	uint32_t current_opacity;
	
    // If true, then blending is enabled.
    bool is_blending : 1;
    
    // If true, then we are configured for filling rather than texturing.
    bool is_filling : 1;
	
    // The vertex data that is used.
    GLshort vertex_data[16];

	// The version of OpenGL in use.
	MCTileCacheOpenGLVersion opengl_version;
	
	// This is set to true if the super tiles need to be forced flushed
	// (due to a Flush call).
	bool needs_flush;
	
	// The original framebuffer (used when doing a snapshot).
	GLuint original_framebuffer;
	
	// The snapshot framebuffer and renderbuffer (only valid between
	// BeginSnapshot and EndSnapshot).
	GLuint snapshot_framebuffer;
	GLuint snapshot_renderbuffer;
	
	// If this is true, rendering is flipped in the y direction (relative to
	// 'normal' for LiveCode origin).
	bool flip_y;
	
	// The origin of the current render (used by snapshot).
	GLint origin_x, origin_y;
};

////////////////////////////////////////////////////////////////////////////////

// The 'glv' version of Color4ub provides auto-fallback to Color4f on 1.0
// versions.
static inline void glvColor4ub(MCTileCacheOpenGLVersion p_version, uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
	if (p_version > kMCTileCacheOpenGLVersion1_0)
		glColor4ub(r, g, b, a);
	else
		glColor4f(r / 255.0, g / 255.0, b / 255.0, a / 255.0);
}

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
	if (!MCMemoryAllocate(8 + self -> super_tile_arity, t_super_tile))
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
	while(glGetError() != GL_NO_ERROR)
		;
	
	return true;
}

bool MCTileCacheOpenGLCompositor_EndTiling(void *p_context)
{
	MCTileCacheOpenGLCompositorContext *self;
	self = (MCTileCacheOpenGLCompositorContext *)p_context;
	
	// Flush any empty textures.
	MCTileCacheOpenGLCompositorFlushSuperTiles(self, false);
	
	// If an OpenGL error occurred, then return false.
	GLenum t_error;
	t_error = glGetError();
	if (t_error != GL_NO_ERROR)
	{
		MCLog("EndTiling - glError() == %d", t_error);
		return false;
	}
	
	return true;
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

static void MCTileCacheOpenGLCompositor_PrepareFrame(MCTileCacheOpenGLCompositorContext *self)
{
#if defined(_ANDROID_MOBILE)
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
	self -> current_color = 0xffffffff;
	self -> current_opacity = 255;
	self -> is_filling = false;
	self -> is_blending = false;
	
	// Get the OpenGL version, if unknown.
	if (self -> opengl_version == kMCTileCacheOpenGLVersionUnknown)
	{
		// MW-2012-08-30: [[ Bug ]] If the OpenGLES version string contains 1.1 then
		//   its version 1.1, else assume 1.0.
		if (strstr((const char *)glGetString(GL_VERSION), "1.1") != nil)
			self -> opengl_version = kMCTileCacheOpenGLVersion1_1;
		else
			self -> opengl_version = kMCTileCacheOpenGLVersion1_0;
	}
	
    // Set the texture matrix.
    glMatrixMode(GL_TEXTURE);
    glLoadIdentity();
    glScalef(self -> tile_size / (float)kSuperTileSize, self -> tile_size / (float)kSuperTileSize, 1.0f);
    
    // Set the modelview matrix.
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	if (!self -> flip_y)
	{
		glTranslatef(0.0f, self -> viewport_height, 0.0f);
		glScalef(1.0f, -1.0f, 1.0f);
    }
	
	// We start off with texturing on.
	glEnable(GL_TEXTURE_2D);
	
	// We start off with 255,255,255,255 as the color.
	glvColor4ub(self -> opengl_version, 255, 255, 255, 255);
    
    // Initialize the blend function we would use.
	glDisable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
	
	// Initialize the vertex data pointers.
	glVertexPointer(2, GL_SHORT, 8, self -> vertex_data);
	glTexCoordPointer(2, GL_SHORT, 8, self -> vertex_data + 2);
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
	
	MCTileCacheOpenGLCompositor_PrepareFrame(self);
	
	return true;
}

bool MCTileCacheOpenGLCompositor_EndFrame(void *p_context, MCStackSurface *p_surface)
{
	MCTileCacheOpenGLCompositorContext *self;
	self = (MCTileCacheOpenGLCompositorContext *)p_context;
	
	p_surface -> UnlockTarget();
	
	GLenum t_error;
	t_error = glGetError();
	if (t_error != GL_NO_ERROR)
	{
		MCLog("EndTiling - glError() == %d", t_error);
		return false;
	}
	
	return true;
}

bool MCTileCacheOpenGLCompositor_BeginSnapshot(void *p_context, MCRectangle p_area, MCGRaster &p_target)
{
	MCTileCacheOpenGLCompositorContext *self;
	self = (MCTileCacheOpenGLCompositorContext *)p_context;
	
	glGetIntegerv(GL_FRAMEBUFFER_BINDING_OES, (GLint *)&self -> original_framebuffer);
	
	glGenFramebuffersOES(1, &self -> snapshot_framebuffer);
	glBindFramebufferOES(GL_FRAMEBUFFER_OES, self -> snapshot_framebuffer);
	
	glGenRenderbuffersOES(1, &self -> snapshot_renderbuffer);
	glBindRenderbufferOES(GL_RENDERBUFFER_OES, self -> snapshot_renderbuffer);
	glRenderbufferStorageOES(GL_RENDERBUFFER_OES, GL_RGBA8_OES, p_area . width, p_area . height);
	glFramebufferRenderbufferOES(GL_FRAMEBUFFER_OES, GL_COLOR_ATTACHMENT0_OES, GL_RENDERBUFFER_OES, self -> snapshot_renderbuffer);

	GLenum t_status;
	t_status = glCheckFramebufferStatusOES(GL_FRAMEBUFFER_OES);
	if (t_status != GL_FRAMEBUFFER_COMPLETE_OES)
	{
		glBindFramebufferOES(GL_FRAMEBUFFER_OES, self -> original_framebuffer);
		glDeleteRenderbuffersOES(1, &self -> snapshot_renderbuffer);
		glDeleteFramebuffersOES(1, &self -> snapshot_framebuffer);
		return false;
	}
	
	// MW-2013-03-12: [[ Bug 10705 ]] Set the origin of the snapshot so layer clipping works
	self -> flip_y = true;
	self -> origin_x = -p_area . x;
	self -> origin_y = -p_area . y;
	
	MCTileCacheOpenGLCompositor_PrepareFrame(self);
	
	// MW-2013-06-07: [[ Bug 10705 ]] Make sure we translate the viewport so the correct area
	//   gets rendered (and snapshotted).
	glMatrixMode(GL_MODELVIEW);
	glTranslatef(-p_area . x, -p_area . y, 0.0f);
	
	return true;
}

bool MCTileCacheOpenGLCompositor_EndSnapshot(void *p_context, MCRectangle p_area, MCGRaster &p_target)
{
	MCTileCacheOpenGLCompositorContext *self;
	self = (MCTileCacheOpenGLCompositorContext *)p_context;
	
	/* OVERHAUL - REVISIT: check byte order? */
	// t_bitmap -> is_swapped = true;
	glReadPixels(0, 0, p_area . width, p_area . height, GL_RGBA, GL_UNSIGNED_BYTE, p_target . pixels);
	
	glBindFramebufferOES(GL_FRAMEBUFFER_OES, self -> original_framebuffer);

	glDeleteRenderbuffersOES(1, &self -> snapshot_renderbuffer);
	glDeleteFramebuffersOES(1, &self -> snapshot_framebuffer);
	
	return true;
}

bool MCTileCacheOpenGLCompositor_BeginLayer(void *p_context, const MCRectangle& p_clip, uint32_t p_opacity, uint32_t p_ink)
{
	MCTileCacheOpenGLCompositorContext *self;
	self = (MCTileCacheOpenGLCompositorContext *)p_context;

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
	
	GLuint t_texture;
	t_texture = self -> super_tiles[t_super_tile_index] -> texture;
	if (t_texture != self -> current_texture)
    {
		glBindTexture(GL_TEXTURE_2D, t_texture);
        self -> current_texture = t_texture;
    }
	
    if (self -> is_filling)
    {
        self -> is_filling = false;
		glEnable(GL_TEXTURE_2D);
    }
    
    // MW-2014-03-14: [[ Bug 11880 ]] The color field we store is premultiplied by the current
    //   opacity.
	uint32_t t_new_color;
	t_new_color = packed_scale_bounded(0xffffffff, self -> current_opacity);
    if (self -> current_color != t_new_color)
    {
        self -> current_color = t_new_color;
		
		// MW-2012-08-30: [[ Bug 10341 ]] The color must be premultiplied - in this case this
		//   means iterating the opacity over all components.
		glvColor4ub(self -> opengl_version, self -> current_opacity, self -> current_opacity, self -> current_opacity, self -> current_opacity);
    }
    
    int32_t t_tile_x, t_tile_y;
    t_tile_x = t_sub_tile_index % (kSuperTileSize / self -> tile_size);
    t_tile_y = t_sub_tile_index / (kSuperTileSize / self -> tile_size);
    
    GLshort *v;
    v = self -> vertex_data;
    *v++ = p_x, *v++ = p_y + self -> tile_size;
    *v++ = t_tile_x, *v++ = t_tile_y + 1;
    *v++ = p_x + self -> tile_size, *v++ = p_y + self -> tile_size;
    *v++ = t_tile_x + 1, *v++ = t_tile_y + 1;
    *v++ = p_x, *v++ = p_y;
    *v++ = t_tile_x, *v++ = t_tile_y;
    *v++ = p_x + self -> tile_size, *v++ = p_y;
    *v++ = t_tile_x + 1, *v++ = t_tile_y;
	
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	
	return true;
}

bool MCTileCacheOpenGLCompositor_CompositeRect(void *p_context, int32_t p_x, int32_t p_y, uint32_t p_color)
{
	MCTileCacheOpenGLCompositorContext *self;
	self = (MCTileCacheOpenGLCompositorContext *)p_context;
	
    if (!self -> is_filling)
    {
        self -> is_filling = true;
		glDisable(GL_TEXTURE_2D);
    }
    
    // MW-2014-03-14: [[ Bug 11880 ]] The color field we store is premultiplied by the current
    //   opacity.
    uint32_t t_new_color;
    t_new_color = packed_scale_bounded(p_color, self -> current_opacity);
	if (self -> current_color != t_new_color)
	{
		self -> current_color = t_new_color;
		
		// IM-2013-08-23: [[ RefactorGraphics ]] Use MCGPixelUnpackNative to fix color swap issues
		uint8_t a, r, g, b;
		MCGPixelUnpackNative(t_new_color, r, g, b, a);
		
		glvColor4ub(self -> opengl_version, r, g, b, a);
	}
	
    GLshort *v;
    v = self -> vertex_data;
	*v++ = p_x, *v++ = p_y + self -> tile_size;
	v += 2;
	*v++ = p_x + self -> tile_size, *v++ = p_y + self -> tile_size;
	v += 2;
	*v++ = p_x, *v++ = p_y;
	v += 2;
	*v++ = p_x + self -> tile_size, *v++ = p_y;

	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	
	return true;
}

void MCTileCacheOpenGLCompositor_Cleanup(void *p_context)
{
	MCTileCacheOpenGLCompositorContext *self;
	self = (MCTileCacheOpenGLCompositorContext *)p_context;
	
	MCTileCacheOpenGLCompositorFlushSuperTiles(self, true);
	MCMemoryDeleteArray(self -> super_tiles);
	MCMemoryDelete(self);
	
#ifdef _IOS_MOBILE
	MCIPhoneSwitchToUIKit();
#else
	MCAndroidDisableOpenGLMode();
#endif
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
	
#ifdef _IOS_MOBILE
	MCIPhoneSwitchToOpenGL();
#else
	MCAndroidEnableOpenGLMode();
#endif
	
	return true;
}

////////////////////////////////////////////////////////////////////////////////
