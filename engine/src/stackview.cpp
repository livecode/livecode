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

#include "prefix.h"

//#include "globdefs.h"
#include "objdefs.h"
#include "parsedef.h"
#include "filedefs.h"

#include "stack.h"
#include "card.h"

#include "util.h"
#include "region.h"

#include "resolution.h"

#include "tilecache.h"
#include "redraw.h"

////////////////////////////////////////////////////////////////////////////////

struct MCStackFullscreenModeNames
{
	const char *name;
	MCStackFullscreenMode mode;
};

static MCStackFullscreenModeNames s_fullscreenmode_names[] = {
	{"", kMCStackFullscreenResize},
	
	// MW-2013-10-31: [[ Bug 11336 ]] Change the modes to be camel-case.
	{"exactFit", kMCStackFullscreenExactFit},
	{"showAll", kMCStackFullscreenShowAll},
	{"noBorder", kMCStackFullscreenNoBorder},
	{"noScale", kMCStackFullscreenNoScale},
	
	// MW-2013-10-31: [[ Bug 1136 ]] REMOVE FOR RELEASE - these are here
	//   for continuity between dp-2 and the next build, will be removed
	//   for release.
	{"exact fit", kMCStackFullscreenExactFit},
	{"show all", kMCStackFullscreenShowAll},
	{"no border", kMCStackFullscreenNoBorder},
	{"no scale", kMCStackFullscreenNoScale},

	{nil, kMCStackFullscreenModeNone},
};

const char *MCStackFullscreenModeToString(MCStackFullscreenMode p_mode)
{
	for (uint32_t i = 0; s_fullscreenmode_names[i].name != nil; i++)
	{
		if (p_mode == s_fullscreenmode_names[i].mode)
			return s_fullscreenmode_names[i].name;
	}

	return nil;
}

bool MCStackFullscreenModeFromString(const char *p_string, MCStackFullscreenMode &r_mode)
{
	for (uint32_t i = 0; s_fullscreenmode_names[i].name != nil; i++)
	{
		if (MCCStringEqualCaseless(p_string, s_fullscreenmode_names[i].name))
		{
			r_mode = s_fullscreenmode_names[i].mode;
			return true;
		}
	}

	return false;
}

////////////////////////////////////////////////////////////////////////////////

struct MCRegionTransformContext
{
	MCRegionRef region;
	MCGAffineTransform transform;
};

bool MCRegionTransformCallback(void *p_context, const MCRectangle &p_rect)
{
	MCRegionTransformContext *t_context;
	t_context = static_cast<MCRegionTransformContext*>(p_context);

	MCRectangle t_transformed_rect;
	t_transformed_rect = MCRectangleGetTransformedBounds(p_rect, t_context->transform);

	return MCRegionIncludeRect(t_context->region, t_transformed_rect);
}

bool MCRegionTransform(MCRegionRef p_region, const MCGAffineTransform &p_transform, MCRegionRef &r_transformed_region)
{
	bool t_success;
	t_success = true;

	MCRegionRef t_new_region;
	t_new_region = nil;

	t_success = MCRegionCreate(t_new_region);

	if (t_success)
	{
		MCRegionTransformContext t_context;
		t_context.region = t_new_region;
		t_context.transform = p_transform;

		t_success = MCRegionForEachRect(p_region, MCRegionTransformCallback, &t_context);
	}

	if (t_success)
		r_transformed_region = t_new_region;
	else
		MCRegionDestroy(t_new_region);

	return t_success;
}

////////////////////////////////////////////////////////////////////////////////

void MCStack::view_init(void)
{
	m_view_fullscreen = false;
	m_view_fullscreenmode = kMCStackFullscreenModeDefault;

	m_view_stack_rect = m_view_rect = MCRectangleMake(0, 0, 0, 0);
	
	// MW-2011-08-26: [[ TileCache ]] Stacks start off with no tilecache.
	m_view_tilecache = nil;
	m_view_bg_layer_id = 0;
	
	// MW-2011-08-19: [[ Redraw ]] Initialize the view's update region
	m_view_update_region = nil;
	
	m_view_transform = MCGAffineTransformMakeIdentity();
}

void MCStack::view_copy(const MCStack &p_view)
{
	m_view_fullscreen = p_view.m_view_fullscreen;
	m_view_fullscreenmode = p_view.m_view_fullscreenmode;

	m_view_stack_rect = p_view.m_view_stack_rect;
	
	// MW-2011-08-26: [[ TileCache ]] Stacks start off with no tilecache.
	m_view_tilecache = nil;
	m_view_bg_layer_id = 0;

	// MW-2011-08-19: [[ Redraw ]] Initialize the view's update region
	m_view_update_region = nil;
}

void MCStack::view_destroy(void)
{
	// MW-2011-08-26: [[ TileCache ]] Destroy the stack's tilecache - if any.
	MCTileCacheDestroy(m_view_tilecache);
	m_view_tilecache = nil;

	// MW-2011-08-19: [[ Redraw ]] Destroy the view's update region.
	MCRegionDestroy(m_view_update_region);
}

void MCStack::view_setfullscreen(bool p_fullscreen)
{
	m_view_fullscreen = p_fullscreen;
}

bool MCStack::view_getfullscreen(void) const
{
	// IM-2013-09-30: [[ FullscreenMode ]] return true if windows are always fullscreen on this display
	return m_view_fullscreen || (MCscreen != nil && MCscreen->fullscreenwindows());
}

void MCStack::view_setfullscreenmode(MCStackFullscreenMode p_mode)
{
	m_view_fullscreenmode = p_mode;
}

MCStackFullscreenMode MCStack::view_getfullscreenmode(void) const
{
	return m_view_fullscreenmode;
}

MCGAffineTransform MCStack::view_getviewtransform(void) const
{
	return m_view_transform;
}

MCGAffineTransform MCStack::view_getroottransform(void) const
{
	return MCGAffineTransformMakeTranslation(m_view_rect.x, m_view_rect.y);
}

MCRectangle MCStack::view_getrect(void) const
{
	return m_view_rect;
}

MCRectangle MCStack::view_constrainstackviewport(const MCRectangle &p_rect)
{
	// IM-2013-10-08: [[ FullscreenMode ]] Constrain resizable stacks here rather than
	// in MCStack::sethints()

	MCRectangle t_stackrect;
	t_stackrect = constrainstackrect(p_rect);
	
	// MW-2012-10-04: [[ Bug 10436 ]] Make sure we constrain stack size to screen size
	//   if in fullscreen mode.
	MCRectangle t_new_rect;
	if (view_getfullscreen())
	{
		const MCDisplay *t_display;
		t_display = MCscreen -> getnearestdisplay(t_stackrect);

		switch (m_view_fullscreenmode)
		{
		case kMCStackFullscreenResize:
			// resize stack to fullscreen rect
			t_new_rect = MCscreen->fullscreenrect(t_display);
			break;

		case kMCStackFullscreenNoScale:
			// center rect on screen
			t_new_rect = MCU_center_rect(MCscreen->fullscreenrect(t_display), t_stackrect);
			break;

		case kMCStackFullscreenExactFit:
		case kMCStackFullscreenShowAll:
		case kMCStackFullscreenNoBorder:
			// scaling modes should return the requested stack rect
			t_new_rect = t_stackrect;
			break;

		default:
			MCAssert(false);
		}
	}
	else
		t_new_rect = constrainstackrecttoscreen(t_stackrect);

	return t_new_rect;
}

MCGAffineTransform view_get_stack_transform(MCStackFullscreenMode p_mode, MCRectangle p_stack_rect, MCRectangle p_screen_rect)
{
	MCGFloat t_scale;
	MCGAffineTransform t_transform;

	switch (p_mode)
	{
	case kMCStackFullscreenModeNone:
		return MCGAffineTransformMakeIdentity();
		
	case kMCStackFullscreenResize:
		return MCGAffineTransformMakeIdentity();

	case kMCStackFullscreenExactFit:
		return MCGAffineTransformMakeScale((MCGFloat)p_screen_rect.width / (MCGFloat)p_stack_rect.width, (MCGFloat)p_screen_rect.height / (MCGFloat)p_stack_rect.height);

	case kMCStackFullscreenShowAll:
		t_scale = MCMin((MCGFloat)p_screen_rect.width / (MCGFloat)p_stack_rect.width, (MCGFloat)p_screen_rect.height / (MCGFloat)p_stack_rect.height);
		t_transform = MCGAffineTransformMakeTranslation(-(MCGFloat)p_stack_rect.width / 2.0, -(MCGFloat)p_stack_rect.height / 2.0);
		t_transform = MCGAffineTransformScale(t_transform, t_scale, t_scale);
		t_transform = MCGAffineTransformTranslate(t_transform, (MCGFloat)p_screen_rect.width / 2.0, (MCGFloat)p_screen_rect.height / 2.0);

		return t_transform;

	case kMCStackFullscreenNoBorder:
		t_scale = MCMax((MCGFloat)p_screen_rect.width / (MCGFloat)p_stack_rect.width, (MCGFloat)p_screen_rect.height / (MCGFloat)p_stack_rect.height);
		t_transform = MCGAffineTransformMakeTranslation(-(MCGFloat)p_stack_rect.width / 2.0, -(MCGFloat)p_stack_rect.height / 2.0);
		t_transform = MCGAffineTransformScale(t_transform, t_scale, t_scale);
		t_transform = MCGAffineTransformTranslate(t_transform, (MCGFloat)p_screen_rect.width / 2.0, (MCGFloat)p_screen_rect.height / 2.0);

		return t_transform;

	case kMCStackFullscreenNoScale:
		// offset so stack is centered in screen
		MCRectangle t_rect;
		t_rect = MCU_center_rect(p_screen_rect, p_stack_rect);
		return MCGAffineTransformMakeTranslation(t_rect.x, t_rect.y);
	}
}

void MCStack::view_on_rect_changed(void)
{
	// IM-2013-10-03: [[ FullscreenMode ]] if the view rect has changed, update the tilecache geometry
	if (m_view_tilecache != nil)
	{
		// IM-2013-10-10: [[ FullscreenMode ]] Align tilecache viewport to origin
		MCRectangle t_view_rect;
		t_view_rect = m_view_rect;
		t_view_rect.x = t_view_rect.y = 0;
		
		MCRectangle t_device_rect;
		t_device_rect = MCGRectangleGetIntegerInterior(MCResUserToDeviceRect(t_view_rect));

		MCTileCacheSetViewport(m_view_tilecache, t_device_rect);
	}
	
	if (view_getfullscreen())
		view_dirty_all();
}

void MCStack::view_setrect(const MCRectangle &p_rect)
{
	if (MCU_equal_rect(p_rect, m_view_rect))
		return;
	
	MCLog("MCStack<%p>::view_setrect({%d,%d,%d,%d})", this, p_rect.x, p_rect.y, p_rect.width, p_rect.height);
	
	MCRectangle t_old_rect;
	t_old_rect = m_view_rect;
	
	// IM-2013-10-08: [[ FullscreenMode ]] Update view rect before calling setsizehints()
	m_view_rect = p_rect;
	
	// IM-2013-10-03: [[ FullscreenMode ]] if the view rect has changed, update the window geometry
	MCRectangle t_device_rect;
	t_device_rect = MCGRectangleGetIntegerInterior(MCResUserToDeviceRect(m_view_rect));

	// IM-2013-10-08: [[ FullscreenMode ]] Update window size hints when setting the view geometry.
	setsizehints();
	device_setgeom(t_device_rect);
	
	view_on_rect_changed();
}

MCRectangle MCStack::view_setstackviewport(const MCRectangle &p_rect)
{
	MCLog("MCStack<%p>::view_setstackviewport({%d,%d,%d,%d}", this, p_rect.x, p_rect.y, p_rect.width, p_rect.height);

	MCRectangle t_view_rect;
	MCGAffineTransform t_transform;
	
	MCStackFullscreenMode t_mode;
	
	if (view_getfullscreen())
	{
		t_mode = m_view_fullscreenmode;
		
		const MCDisplay *t_display;
		t_display = MCscreen -> getnearestdisplay(p_rect);

		t_view_rect = MCscreen->fullscreenrect(t_display);
		t_view_rect . x = t_view_rect . y = 0;
		
	}
	else
	{
		t_mode = kMCStackFullscreenModeNone;
		t_view_rect = p_rect;
	}
	
	view_setrect(t_view_rect);
	
	m_view_stack_rect = view_constrainstackviewport(p_rect);
	
	t_transform = view_get_stack_transform(t_mode, m_view_stack_rect, m_view_rect);
	
	// IM-2013-10-03: [[ FullscreenMode ]] if the transform has changed, redraw everything
	if (!MCGAffineTransformIsEqual(t_transform, m_view_transform))
	{
		m_view_transform = t_transform;
		
		view_dirty_all();
	}
	
	return m_view_stack_rect;
}

void MCStack::view_configure(bool p_user)
{
	MCRectangle t_view_rect;
	mode_getrealrect(t_view_rect);
	
	if (!MCU_equal_rect(t_view_rect, m_view_rect))
	{
		m_view_rect = t_view_rect;
		view_on_rect_changed();
	}
	configure(p_user);
}

MCRectangle MCStack::view_getstackviewport()
{
	return m_view_stack_rect;
}

void MCStack::view_render(MCGContextRef p_target, MCRectangle p_rect)
{
	// redraw borders if visible

	// scale & position stack redraw rect
	// update stack region
	if (view_getfullscreen())
	{
		MCRectangle t_update_rect;
		t_update_rect = MCRectangleGetTransformedBounds(p_rect, MCGAffineTransformInvert(m_view_transform));
		
		MCGContextSave(p_target);
		MCGContextConcatCTM(p_target, m_view_transform);
		
		// IM-2013-10-14: [[ FullscreenMode ]] Logical stack rect has 0,0 origin
		MCRectangle t_stack_rect;
		t_stack_rect = m_view_stack_rect;
		t_stack_rect.x = t_stack_rect.y = 0;
		
		if (!MCU_rect_in_rect(t_update_rect, t_stack_rect))
		{
			// IM-2013-10-08: [[ FullscreenMode ]] draw the view backdrop if the render area
			// falls outside the stack rect
			/* OVERHAUL - REVISIT: currently just draws black behind the stack area */
			MCGContextAddRectangle(p_target, MCRectangleToMCGRectangle(t_update_rect));
			MCGContextSetFillRGBAColor(p_target, 0.0, 0.0, 0.0, 1.0);
			MCGContextFill(p_target);
		}

		t_update_rect = MCU_intersect_rect(t_update_rect, MCRectangleMake(0, 0, m_view_stack_rect.width, m_view_stack_rect.height));
		MCGContextClipToRect(p_target, MCRectangleToMCGRectangle(t_update_rect));
		render(p_target, t_update_rect);
		MCGContextRestore(p_target);
	}
	else
	{
		render(p_target, p_rect);
	}

}

// IM-2013-10-14: [[ FullscreenMode ]] Move update region tracking into view abstraction
void MCStack::view_updatewindow(void)
{
	if (m_view_update_region == nil)
		return;
	
	MCGAffineTransform t_transform;
	t_transform = MCResGetDeviceTransform();
	
	// transform view region to device region
	MCRegionRef t_device_region;
	t_device_region = nil;
	
	/* UNCHECKED */ MCRegionTransform(m_view_update_region, t_transform, t_device_region);
	
	device_updatewindow(t_device_region);
	
	MCRegionDestroy(t_device_region);
}

void MCStack::view_updatestack(MCRegionRef p_region)
{
	MCGAffineTransform t_transform;
	t_transform = getdevicetransform();

	// transform stack region to device region
	MCRegionRef t_view_region;
	t_view_region = nil;

	/* UNCHECKED */ MCRegionTransform(p_region, t_transform, t_view_region);
	
	device_updatewindow(t_view_region);

	MCRegionDestroy(t_view_region);
}

MCPoint MCStack::view_viewtostackloc(const MCPoint &p_loc) const
{
	if (!view_getfullscreen() || m_view_fullscreenmode == kMCStackFullscreenResize)
		return p_loc;

	MCGAffineTransform t_transform = MCGAffineTransformInvert(m_view_transform);
	return MCGPointToMCPoint(MCGPointApplyAffineTransform(MCPointToMCGPoint(p_loc), t_transform));
}

MCPoint MCStack::view_stacktoviewloc(const MCPoint &p_loc) const
{
	if (!view_getfullscreen() || m_view_fullscreenmode == kMCStackFullscreenResize)
		return p_loc;

	return MCGPointToMCPoint(MCGPointApplyAffineTransform(MCPointToMCGPoint(p_loc), m_view_transform));
}

////////////////////////////////////////////////////////////////////////////////

bool MCStack::view_getacceleratedrendering(void)
{
	return m_view_tilecache != nil;
}

void MCStack::view_setacceleratedrendering(bool p_value)
{
	// If we are turning accelerated rendering off, then destroy the tilecache.
	if (!p_value)
	{
		MCTileCacheDestroy(m_view_tilecache);
		m_view_tilecache = nil;
		
		// MW-2012-03-15: [[ Bug ]] Make sure we dirty the stack to ensure all the
		//   layer mode attrs are rest.
		view_dirty_all();
		
		return;
	}
	
	// If we are turning accelerated rendering on, and we already have a tile-
	// cache, then do nothing.
	if (m_view_tilecache != nil)
		return;
	
	// Otherwise, we configure based on platform settings.
	int32_t t_tile_size;
	int32_t t_cache_limit;
	MCTileCacheCompositorType t_compositor_type;
#ifdef _MAC_DESKTOP
	t_compositor_type = kMCTileCacheCompositorCoreGraphics;
	t_tile_size = 32;
	t_cache_limit = 32 * 1024 * 1024;
#elif defined(_WINDOWS_DESKTOP) || defined(_LINUX_DESKTOP)
	t_compositor_type = kMCTileCacheCompositorSoftware;
	t_tile_size = 32;
	t_cache_limit = 32 * 1024 * 1024;
#elif defined(_IOS_MOBILE) || defined(_ANDROID_MOBILE)
	t_compositor_type = kMCTileCacheCompositorStaticOpenGL;
	
	const MCDisplay *t_display;
	MCscreen -> getdisplays(t_display, false);
	
	MCRectangle t_viewport;
	t_viewport = t_display -> device_viewport;
	
	bool t_small_screen, t_medium_screen;
	t_small_screen = MCMin(t_viewport . width, t_viewport . height) <= 480 && MCMax(t_viewport . width, t_viewport . height) <= 640;
	t_medium_screen = MCMin(t_viewport . width, t_viewport . height) <= 768 && MCMax(t_viewport . width, t_viewport . height) <= 1024;
	
	if (t_small_screen)
		t_tile_size = 32, t_cache_limit = 16 * 1024 * 1024;
	else if (t_medium_screen)
		t_tile_size = 64, t_cache_limit = 32 * 1024 * 1024;
	else
		t_tile_size = 64, t_cache_limit = 64 * 1024 * 1024;
#endif
	
	// IM-2013-08-21: [[ ResIndependence ]] Use device coords for tilecache operation
	// IM-2013-09-30: [[ FullscreenMode ]] Use view rect when setting the size of the tilecache
	// IM-2013-10-10: [[ FullscreenMode ]] Align tilecache viewport to origin
	MCRectangle t_view_rect;
	t_view_rect = view_getrect();
	t_view_rect.x = t_view_rect.y = 0;
	
	MCRectangle t_device_rect;
	t_device_rect = MCGRectangleGetIntegerBounds(MCResUserToDeviceRect(t_view_rect));
	MCTileCacheCreate(t_tile_size, t_cache_limit, m_view_tilecache);
	MCTileCacheSetViewport(m_view_tilecache, t_device_rect);
	MCTileCacheSetCompositor(m_view_tilecache, t_compositor_type);
	
	view_dirty_all();
}

//////////

MCTileCacheCompositorType MCStack::view_getcompositortype(void)
{
	if (m_view_tilecache == nil)
		return kMCTileCacheCompositorNone;
	else
		return MCTileCacheGetCompositor(m_view_tilecache);
}

void MCStack::view_setcompositortype(MCTileCacheCompositorType p_type)
{
	if (p_type == kMCTileCacheCompositorNone)
	{
		MCTileCacheDestroy(m_view_tilecache);
		m_view_tilecache = nil;
	}
	else
	{
		if (m_view_tilecache == nil)
		{
			MCTileCacheCreate(32, 4096 * 1024, m_view_tilecache);
			// IM-2013-08-21: [[ ResIndependence ]] Use device coords for tilecache operation
			// IM-2013-10-02: [[ FullscreenMode ]] Use view rect when setting the size of the tilecache
			// IM-2013-10-10: [[ FullscreenMode ]] Align tilecache viewport to origin
			MCRectangle t_view_rect;
			t_view_rect = view_getrect();
			t_view_rect.x = t_view_rect.y = 0;
			
			MCRectangle t_device_rect;
			t_device_rect = MCGRectangleGetIntegerBounds(MCResUserToDeviceRect(t_view_rect));
			MCTileCacheSetViewport(m_view_tilecache, t_device_rect);
		}
		
		MCTileCacheSetCompositor(m_view_tilecache, p_type);
	}
	
	view_dirty_all();
}

//////////

uint32_t MCStack::view_getcompositorcachelimit()
{
	if (m_view_tilecache == nil)
		return 0;
	else
		return MCTileCacheGetCacheLimit(m_view_tilecache);
}

void MCStack::view_setcompositorcachelimit(uint32_t p_limit)
{
	if (m_view_tilecache != nil)
	{
		MCTileCacheSetCacheLimit(m_view_tilecache, p_limit);
		view_dirty_all();
	}
}

//////////

bool MCStack::view_isvalidcompositortilesize(uint32_t p_size)
{
	return MCIsPowerOfTwo(p_size) && p_size >= 16 && p_size <= 256;
}

uint32_t MCStack::view_getcompositortilesize()
{
	if (m_view_tilecache == nil)
		return 0;
	else
		return MCTileCacheGetTileSize(m_view_tilecache);
}

void MCStack::view_setcompositortilesize(uint32_t p_size)
{
	if (m_view_tilecache != nil)
	{
		MCTileCacheSetTileSize(m_view_tilecache, p_size);
		view_dirty_all();
	}
}

////////////////////////////////////////////////////////////////////////////////

bool view_device_render_background(void *p_context, MCGContextRef p_target, const MCRectangle& p_rectangle)
{
	/* OVERHAUL - REVISIT: currently just draws black behind the stack area */
	MCGContextAddRectangle(p_target, MCRectangleToMCGRectangle(p_rectangle));
	MCGContextSetFillRGBAColor(p_target, 0.0, 0.0, 0.0, 1.0);
	MCGContextFill(p_target);
	
	return true;
}

void MCStack::view_updatetilecache(void)
{
	if (m_view_tilecache == nil)
		return;
	
	// If the tilecache is not valid, flush it.
	if (!MCTileCacheIsValid(m_view_tilecache))
		MCTileCacheFlush(m_view_tilecache);
	
	MCTileCacheBeginFrame(m_view_tilecache);
	curcard -> render();

	// IM-2013-10-14: [[ FullscreenMode ]] Add tilecache scenery layer to render the view background
	
	// Final step is to render the background. Note that the background layer
	// really only needs to be the rect rounded outward to the nearest tile
	// boundaries, but 8192, 8192 is bigger than it can ever be at present so
	// is an easier alternative.
	MCTileCacheLayer t_bg_layer;
	t_bg_layer . id = m_view_bg_layer_id;
	t_bg_layer . region = MCU_make_rect(0, 0, 8192, 8192);
	t_bg_layer . clip = MCU_make_rect(0, 0, 8192, 8192);
	t_bg_layer . is_opaque = true;
	t_bg_layer . opacity = 255;
	t_bg_layer . ink = GXblendSrcOver;
	t_bg_layer . callback = view_device_render_background;
	t_bg_layer . context = this;
	MCTileCacheRenderScenery(m_view_tilecache, t_bg_layer);
	m_view_bg_layer_id = t_bg_layer . id;
	
	MCTileCacheEndFrame(m_view_tilecache);
}

void MCStack::view_deactivatetilecache(void)
{
    if (m_view_tilecache == nil)
        return;
    
    MCTileCacheDeactivate(m_view_tilecache);
}

void MCStack::view_flushtilecache(void)
{
	if (m_view_tilecache == nil)
		return;
	
	MCTileCacheFlush(m_view_tilecache);
}

void MCStack::view_activatetilecache(void)
{
	if (m_view_tilecache == nil)
		return;
	
	MCTileCacheActivate(m_view_tilecache);
}

void MCStack::view_compacttilecache(void)
{
	if (m_view_tilecache == nil)
		return;
	
	MCTileCacheCompact(m_view_tilecache);
}

////////////////////////////////////////////////////////////////////////////////

bool MCStack::view_snapshottilecache(const MCRectangle &p_stack_rect, MCGImageRef &r_image)
{
	if (m_view_tilecache == nil)
		return false;
	
	// MW-2013-10-29: [[ Bug 11330 ]] Transform stack to (local) view co-ords.
	MCRectangle t_view_rect;
	t_view_rect = MCRectangleGetTransformedBounds(p_stack_rect, getviewtransform());
	t_view_rect = MCU_intersect_rect(t_view_rect, MCU_make_rect(0, 0, view_getrect() . width, view_getrect() . height));
	
	// IM-2013-08-21: [[ ResIndependence ]] Use device coords for tilecache operation
	// IM-2013-09-30: [[ FullscreenMode ]] Use stack transform to get device coords
	MCRectangle t_device_rect;
	t_device_rect = MCRectangleGetTransformedBounds(t_view_rect, MCResGetDeviceTransform());
	return MCTileCacheSnapshot(m_view_tilecache, t_device_rect, r_image);
}

////////////////////////////////////////////////////////////////////////////////

// IM-2013-10-14: [[ FullscreenMode ]] Move update region tracking into view abstraction
void MCStack::view_apply_updates()
{
	// Ensure the content is up to date.
	if (m_view_need_redraw)
	{
		// MW-2012-04-20: [[ Bug 10185 ]] Only update if there is a window to update.
		//   (we can get here if a stack has its window taken over due to go in window).
		if (window != nil)
		{
#ifdef _IOS_MOBILE
			// MW-2013-03-20: [[ Bug 10748 ]] We defer switching the display class on iOS as
			//   it causes flashes when switching between stacks.
			extern void MCIPhoneSyncDisplayClass(void);
			MCIPhoneSyncDisplayClass();
#endif
			
			// MW-2011-09-08: [[ TileCache ]] If we have a tilecache, then attempt to update
			//   it.
			updatetilecache();
			
			// MW-2011-09-08: [[ TileCache ]] Perform a redraw of the window within
			//   the update region.
			view_updatewindow();
			
			// Clear the update region.
			MCRegionSetEmpty(m_view_update_region);
		}
		
		// We no longer need to redraw.
		m_view_need_redraw = false;
	}
}

// IM-2013-10-14: [[ FullscreenMode ]] Move update region tracking into view abstraction
void MCStack::view_reset_updates()
{
	MCRegionDestroy(m_view_update_region);
	m_view_update_region = nil;
	m_view_need_redraw = false;
}

// IM-2013-10-14: [[ FullscreenMode ]] Move update region tracking into view abstraction
void MCStack::view_dirty_rect(const MCRectangle &p_rect)
{
	MCRectangle t_view_rect;
	t_view_rect = MCRectangleMake(0, 0, m_view_rect.width, m_view_rect.height);
	
	MCRectangle t_dirty_rect;
	t_dirty_rect = MCU_intersect_rect(p_rect, t_view_rect);
	
	if (t_dirty_rect.width == 0 || t_dirty_rect.height == 0)
		return;
	
	// If there is no region yet, make one.
	if (m_view_update_region == nil)
		/* UNCHECKED */ MCRegionCreate(m_view_update_region);
	
	MCRegionIncludeRect(m_view_update_region, t_dirty_rect);
	
	// Mark the stack as needing a redraw and schedule an update.
	m_view_need_redraw = true;

	MCRedrawScheduleUpdateForStack(this);
}

// IM-2013-10-14: [[ FullscreenMode ]] Move update region tracking into view abstraction
void MCStack::view_dirty_all(void)
{
	view_dirty_rect(MCRectangleMake(0, 0, m_view_rect.width, m_view_rect.height));
	
	dirtyall();
}

////////////////////////////////////////////////////////////////////////////////
