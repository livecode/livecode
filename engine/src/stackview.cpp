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

#include "objdefs.h"
#include "parsedef.h"
#include "filedefs.h"
#include "globals.h"
#include "stack.h"
#include "card.h"

#include "util.h"
#include "region.h"

#include "resolution.h"

#include "tilecache.h"
#include "redraw.h"
#include "player.h"

////////////////////////////////////////////////////////////////////////////////

struct MCStackFullscreenModeNames
{
	const char *name;
	MCStackFullscreenMode mode;
};

static MCStackFullscreenModeNames s_fullscreenmode_names[] = {
	{"", kMCStackFullscreenResize},
	
	// MW-2013-10-31: [[ Bug 11336 ]] Change the modes to be camel-case.
	// IM-2013-11-20: [[ FullscreenMode ]] Rename showAll to letterbox and remove two-word mode synonyms
	{"exactFit", kMCStackFullscreenExactFit},
	{"letterbox", kMCStackFullscreenLetterbox},
	{"noBorder", kMCStackFullscreenNoBorder},
	{"noScale", kMCStackFullscreenNoScale},
	{"showAll", kMCStackFullscreenShowAll},

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

void MCStack::view_init(void)
{
	m_view_fullscreen = false;
	m_view_fullscreenmode = kMCStackFullscreenModeDefault;

	m_view_adjusted_stack_rect = m_view_requested_stack_rect = m_view_rect = MCRectangleMake(0, 0, 0, 0);
	m_view_stack_visible_rect = MCRectangleMake(0, 0, 0, 0);
	
	// MW-2011-08-26: [[ TileCache ]] Stacks start off with no tilecache.
	m_view_tilecache = nil;
	m_view_bg_layer_id = 0;
	
	// MW-2011-08-19: [[ Redraw ]] Initialize the view's update region
	m_view_update_region = nil;
	
	m_view_transform = MCGAffineTransformMakeIdentity();
	
	m_view_content_scale = 1.0;

	// IM-2014-01: [[ HiDPI ]] Initialize the view backing surface scale
	m_view_backing_scale = 1.0;
}

void MCStack::view_copy(const MCStack &p_view)
{
	m_view_fullscreen = p_view.m_view_fullscreen;
	m_view_fullscreenmode = p_view.m_view_fullscreenmode;

	m_view_requested_stack_rect = p_view.m_view_requested_stack_rect;
	m_view_adjusted_stack_rect = p_view.m_view_adjusted_stack_rect;
	
	m_view_stack_visible_rect = p_view.m_view_stack_visible_rect;
	
    // FG-2014-01-30 [[ Valgrind ]] Unitinialized value
    m_view_rect = p_view.m_view_rect;

	// MW-2011-08-26: [[ TileCache ]] Stacks start off with no tilecache.
	m_view_tilecache = nil;
	m_view_bg_layer_id = 0;

	// MW-2011-08-19: [[ Redraw ]] Initialize the view's update region
	m_view_update_region = nil;
	
	m_view_transform = p_view.m_view_transform;
	
	m_view_content_scale = p_view.m_view_content_scale;

	// IM-2014-01: [[ HiDPI ]] Initialize the view backing surface scale
	m_view_backing_scale = p_view.m_view_backing_scale;

    // SN-2014-14-10: [[ ViewRect ]] Ensure that the view rect is copied (uninitialised m_view_rect
    //  might lead to a crash)
    m_view_rect = p_view.m_view_rect;
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
	bool t_fullscreen = view_getfullscreen();
	
	// IM-2014-02-12: [[ Bug 11783 ]] We may also need to reset the fonts on Windows when
	//   fullscreen is changed
	bool t_ideal_layout;
	t_ideal_layout = getuseideallayout();

	m_view_fullscreen = p_fullscreen;
	
	// IM-2014-01-16: [[ StackScale ]] Reopen the window after changing fullscreen
	if (t_fullscreen != view_getfullscreen())
	{
		// IM-2014-05-27: [[ Bug 12321 ]] Work out here whether or not we need to purge fonts
		if ((t_ideal_layout != getuseideallayout()) && opened)
			m_purge_fonts = true;

		reopenwindow();
		
		// IM-2014-05-27: [[ Bug 12321 ]] Move view transform update to reopenwindow() to allow stack rect to be reset correctly before reopening
	}
}

bool MCStack::view_getfullscreen(void) const
{
	// IM-2013-09-30: [[ FullscreenMode ]] return true if windows are always fullscreen on this display
	return m_view_fullscreen || (MCscreen != nil && MCscreen->fullscreenwindows());
}

void MCStack::view_setfullscreenmode(MCStackFullscreenMode p_mode)
{
	m_view_fullscreenmode = p_mode;
	
	// IM-2014-01-16: [[ StackScale ]] Update view transform after changing view property
	view_update_transform();
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

//////////

void MCStack::view_set_content_scale(MCGFloat p_scale)
{
	if (m_view_content_scale == p_scale)
		return;
	
	m_view_content_scale = p_scale;
	
	// IM-2014-01-16: [[ StackScale ]] Update view transform after changing view property
	view_update_transform(true);
	// IM-2014-10-22: [[ Bug 13746 ]] Update window mask when stack scale changes
	loadwindowshape();
}

MCGFloat MCStack::view_get_content_scale(void) const
{
	return m_view_content_scale;
}

//////////

// IM-2014-02-13: [[ StackScale ]] Update to work with MCGRectangles
MCGRectangle MCStack::view_constrainstackviewport(const MCGRectangle &p_rect)
{
	// IM-2014-01-16: [[ StackScale ]] stack rect now constrained to min/max size elsewhere
	MCGRectangle t_stackrect;
	t_stackrect = p_rect;
	
	// MW-2012-10-04: [[ Bug 10436 ]] Make sure we constrain stack size to screen size
	//   if in fullscreen mode.
	MCGRectangle t_new_rect;
	if (view_getfullscreen())
	{
		const MCDisplay *t_display;
		t_display = MCscreen -> getnearestdisplay(MCGRectangleGetIntegerInterior(t_stackrect));
		
		switch (m_view_fullscreenmode)
		{
			case kMCStackFullscreenResize:
				// resize stack to fullscreen rect
				t_new_rect = MCRectangleToMCGRectangle(MCscreen->fullscreenrect(t_display));
				break;
				
			case kMCStackFullscreenNoScale:
				// center rect on screen
				t_new_rect = MCGRectangleCenterOnRect(t_stackrect, MCRectangleToMCGRectangle(MCscreen->fullscreenrect(t_display)));
				break;
				
			case kMCStackFullscreenExactFit:
			case kMCStackFullscreenLetterbox:
			case kMCStackFullscreenNoBorder:
			case kMCStackFullscreenShowAll:
				// scaling modes should return the requested stack rect
				t_new_rect = t_stackrect;
				break;
				
			default:
				MCAssert(false);
		}
	}
	else
	{
		// IM-2014-02-28: [[ Bug 11844 ]] Don't constrain stack rect here unless fullscreen
		t_new_rect = t_stackrect;
	}
	
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

	case kMCStackFullscreenLetterbox:
	case kMCStackFullscreenShowAll:
		t_scale = MCMin((MCGFloat)p_screen_rect.width / (MCGFloat)p_stack_rect.width, (MCGFloat)p_screen_rect.height / (MCGFloat)p_stack_rect.height);
		t_transform = MCGAffineTransformMakeTranslation(-(MCGFloat)p_stack_rect.width / 2.0, -(MCGFloat)p_stack_rect.height / 2.0);
		t_transform = MCGAffineTransformPreScale(t_transform, t_scale, t_scale);
		t_transform = MCGAffineTransformPreTranslate(t_transform, (MCGFloat)p_screen_rect.width / 2.0, (MCGFloat)p_screen_rect.height / 2.0);

		return t_transform;

	case kMCStackFullscreenNoBorder:
		t_scale = MCMax((MCGFloat)p_screen_rect.width / (MCGFloat)p_stack_rect.width, (MCGFloat)p_screen_rect.height / (MCGFloat)p_stack_rect.height);
		t_transform = MCGAffineTransformMakeTranslation(-(MCGFloat)p_stack_rect.width / 2.0, -(MCGFloat)p_stack_rect.height / 2.0);
		t_transform = MCGAffineTransformPreScale(t_transform, t_scale, t_scale);
		t_transform = MCGAffineTransformPreTranslate(t_transform, (MCGFloat)p_screen_rect.width / 2.0, (MCGFloat)p_screen_rect.height / 2.0);

		return t_transform;

	case kMCStackFullscreenNoScale:
		// offset so stack is centered in screen
		MCRectangle t_rect;
		t_rect = MCU_center_rect(p_screen_rect, p_stack_rect);
		// IM-2013-12-19: [[ Bug 11590 ]] Adjust for screen rect origins other than 0,0
		return MCGAffineTransformMakeTranslation(t_rect.x - p_screen_rect.x, t_rect.y - p_screen_rect.y);
    default:
        MCUnreachableReturn(MCGAffineTransformMakeIdentity());
	}
}

void MCStack::view_on_rect_changed(void)
{
	// IM-2013-10-03: [[ FullscreenMode ]] if the view rect has changed, update the tilecache geometry
	view_updatetilecacheviewport();
	
	if (view_getfullscreen() || view_platform_dirtyviewonresize())
		dirtyall();
}

void MCStack::view_setrect(const MCRectangle &p_rect)
{
	if (MCU_equal_rect(p_rect, m_view_rect))
		return;
	
	MCRectangle t_old_rect;
	t_old_rect = m_view_rect;
	
	// IM-2013-10-08: [[ FullscreenMode ]] Update view rect before calling setsizehints()
	m_view_rect = p_rect;
	
	// IM-2014-01-16: [[ StackScale ]] Update window geometry if we have a window
	// IM-2014-02-27: [[ Bug 11858 ]] Allow window geometry update when stack is closed
	if (window != nil)
	{
		// IM-2013-10-03: [[ FullscreenMode ]] if the view rect has changed, update the window geometry

		// IM-2014-01-24: [[ HiDPI ]] Change to use logical coordinates - device coordinate conversion no longer needed
		/* CODE REMOVED */

		// IM-2013-10-08: [[ FullscreenMode ]] Update window size hints when setting the view geometry.
		setsizehints();
		view_setgeom(m_view_rect);
	}
	
	view_on_rect_changed();
}

void MCStack::view_sync_window_geometry(void)
{
	if (view_getfullscreen())
	{
		// if the view is fullscreen then reconfigure
		view_configure(True);
	}
	else
	{
		// otherwise reset the window rect which may have changed as a result of scaling

		// IM-2014-01-24: [[ HiDPI ]] Change to use logical coordinates - device coordinate conversion no longer needed
		/* CODE REMOVED */
		
		// IM-2013-10-08: [[ FullscreenMode ]] Update window size hints when setting the view geometry.
		setsizehints();
		view_setgeom(m_view_rect);
		
		view_on_rect_changed();
	}
}

// IM-2014-01-16: [[ StackScale ]] Utility method to calculate new rects and view transform
void MCStack::view_calculate_viewports(const MCRectangle &p_stack_rect, MCRectangle &r_adjusted_stack_rect, MCRectangle &r_view_rect, MCGAffineTransform &r_transform)
{
	MCRectangle t_view_rect;

	MCGAffineTransform t_transform;
	t_transform = MCGAffineTransformMakeScale(m_view_content_scale, m_view_content_scale);
	
	MCRectangle t_stack_rect;
	
	// IM-2014-01-16: [[ StackScale ]] Constrain stack size within the min/max size
	t_stack_rect = constrainstackrect(p_stack_rect);
	
	// IM-2014-01-16: [[ StackScale ]] transform stack rect using scale factor
	// IM-2014-02-13: [[ StackScale ]] Use MCGRectangle to avoid resizing due to rounding errors
	MCGRectangle t_scaled_rect;
	t_scaled_rect = MCGRectangleApplyAffineTransform(MCRectangleToMCGRectangle(t_stack_rect), t_transform);

	t_scaled_rect = view_constrainstackviewport(t_scaled_rect);
	
	MCStackFullscreenMode t_mode;
	
	if (view_getfullscreen())
	{
		t_mode = m_view_fullscreenmode;
		
		const MCDisplay *t_display;
		t_display = MCscreen -> getnearestdisplay(MCGRectangleGetIntegerInterior(t_scaled_rect));

		t_view_rect = MCscreen->fullscreenrect(t_display);
		// IM-2013-12-19: [[ Bug 11590 ]] Removed adjustment of screen rect to 0,0 origin
	}
	else
	{
		t_mode = kMCStackFullscreenModeNone;
		t_view_rect = MCGRectangleGetIntegerInterior(t_scaled_rect);
	}
	
	// IM-2014-01-16: [[ StackScale ]] store adjusted rect in stack coords
	MCGRectangle t_adjusted;
	t_adjusted = MCGRectangleApplyAffineTransform(t_scaled_rect, MCGAffineTransformInvert(t_transform));
	
	r_adjusted_stack_rect = MCGRectangleGetIntegerRect(t_adjusted);
	
	r_view_rect = t_view_rect;
	
	// IM-2014-01-16: [[ StackScale ]] append scale transform to fullscreenmode transform
	r_transform = MCGAffineTransformConcat(view_get_stack_transform(t_mode, MCGRectangleGetIntegerBounds(t_scaled_rect), t_view_rect), t_transform);
}

#if defined(_MOBILE)
#include "mblsyntax.h"
#endif
	
void MCStack::view_update_transform(bool p_ensure_onscreen)
{
	MCRectangle t_view_rect;
	MCGAffineTransform t_transform;
    
#if defined(_MOBILE)
    MCOrientation t_orientation;
    MCSystemGetOrientation(t_orientation);
    MCOrientationGetRectForOrientation(t_orientation ,m_view_requested_stack_rect);
#endif
	
	// IM-2014-01-16: [[ StackScale ]] Use utility method to calculate new values
	view_calculate_viewports(m_view_requested_stack_rect, m_view_adjusted_stack_rect, t_view_rect, t_transform);
	
	// IM-2013-12-20: [[ ShowAll ]] Calculate new stack visible rect
	MCRectangle t_stack_visible_rect;
	t_stack_visible_rect = MCRectangleGetTransformedBounds(MCRectangleMake(0, 0, t_view_rect.width, t_view_rect.height), MCGAffineTransformInvert(t_transform));
	if (m_view_fullscreenmode == kMCStackFullscreenLetterbox || m_view_fullscreenmode == kMCStackFullscreenNoScale)
		t_stack_visible_rect = MCU_intersect_rect(t_stack_visible_rect, MCRectangleMake(0, 0, m_view_adjusted_stack_rect.width, m_view_adjusted_stack_rect.height));
	
	// IM-2013-10-03: [[ FullscreenMode ]] if the transform has changed, redraw everything
	// IM-2013-12-20: [[ ShowAll ]] if the stack viewport has changed, redraw everything
	bool t_rect_changed, t_transform_changed;
	t_rect_changed = !MCU_equal_rect(t_stack_visible_rect, m_view_stack_visible_rect);
	t_transform_changed = !MCGAffineTransformIsEqual(t_transform, m_view_transform);
	if (t_rect_changed || t_transform_changed)
	{
		m_view_transform = t_transform;
		m_view_stack_visible_rect = t_stack_visible_rect;
		
		dirtyall();
		if (t_transform_changed)
			this->OnViewTransformChanged();
	}
	
	// PM-2015-07-17: [[ Bug 13754 ]] Make sure stack does not disappear off screen when changing the scalefactor
    MCRectangle t_bounded_rect;
    if (p_ensure_onscreen)
    {
        // AL-2015-10-01: [[ Bug 16017 ]] Remember location of stacks on a second monitor
        const MCDisplay* t_nearest_display;
        t_nearest_display = MCscreen -> getnearestdisplay(t_view_rect);
        
        if (t_nearest_display != nil)
        {
			MCRectangle t_screen_rect;
            t_screen_rect = t_nearest_display -> viewport;
            t_bounded_rect = MCU_bound_rect(t_view_rect, t_screen_rect . x, t_screen_rect . y, t_screen_rect . width, t_screen_rect . height);
        }
        else
        {
            // In noUI mode, we don't have a nearest display.
            t_bounded_rect = MCU_bound_rect(t_view_rect, 0, 0, MCscreen -> getwidth(), MCscreen -> getheight());
        }
    }
    else
    {
        t_bounded_rect = t_view_rect;
    }
    
    // IM-2014-01-16: [[ StackScale ]] Update view rect if needed
    view_setrect(t_bounded_rect);
}

MCRectangle MCStack::view_setstackviewport(const MCRectangle &p_rect)
{
	m_view_requested_stack_rect = p_rect;

	// IM-2014-01-16: [[ StackScale ]] Update view transform after changing view property
	view_update_transform();
	
	return m_view_adjusted_stack_rect;
}

void MCStack::view_configure(bool p_user)
{
	MCRectangle t_view_rect;
	mode_getrealrect(t_view_rect);

	// IM-2014-10-29: [[ Bug 13812 ]] Remove need resize check and unset flag
	m_view_need_resize = false;

	if (!MCU_equal_rect(t_view_rect, m_view_rect))
	{
		// IM-2014-02-13: [[ StackScale ]] Test if the view size has changed
		bool t_resize;
		t_resize = t_view_rect.width != m_view_rect.width || t_view_rect.height != m_view_rect.height;
		
		m_view_rect = t_view_rect;
		
		view_on_rect_changed();
		
		if (view_getfullscreen())
		{
			// IM-2014-01-16: [[ StackScale ]] recalculate fullscreenmode transform after view rect change
			view_update_transform();
		}
		else
		{
			uint32_t t_current_width, t_current_height;
			t_current_width = m_view_adjusted_stack_rect.width;
			t_current_height = m_view_adjusted_stack_rect.height;
			
			// IM-2014-01-16: [[ StackScale ]] set the stack rects to the scaled down view rect
			m_view_requested_stack_rect = m_view_adjusted_stack_rect = MCRectangleGetTransformedBounds(m_view_rect, MCGAffineTransformInvert(m_view_transform));
			
			// IM-2014-02-13: [[ StackScale ]] If the view size has not changed then make sure
			//   the stack size also remains the same
			if (!t_resize)
			{
				//restore current logical width & height
				m_view_requested_stack_rect.width = m_view_adjusted_stack_rect.width = t_current_width;
				m_view_requested_stack_rect.height = m_view_adjusted_stack_rect.height = t_current_height;
			}
			
			// IM-2014-02-06: [[ ShowAll ]] Update the visible stack rect
			m_view_stack_visible_rect = MCRectangleMake(0, 0, m_view_adjusted_stack_rect.width, m_view_adjusted_stack_rect.height);
		}
	}
	configure(p_user);
}

MCRectangle MCStack::view_getstackviewport()
{
	return m_view_adjusted_stack_rect;
}

MCRectangle MCStack::view_getstackvisiblerect(void)
{
	return m_view_stack_visible_rect;
}

void MCStack::view_render(MCGContextRef p_target, MCRectangle p_rect)
{
    if (getextendedstate(ECS_DONTDRAW))
        return;
    
    // redraw borders if visible

	// scale & position stack redraw rect
	// update stack region
	// IM-2014-01-16: [[ StackScale ]] Transform redraw rect to stack coords in all cases
		MCRectangle t_update_rect;
		t_update_rect = MCRectangleGetTransformedBounds(p_rect, MCGAffineTransformInvert(m_view_transform));
		
		MCGContextSave(p_target);
		MCGContextConcatCTM(p_target, m_view_transform);
		
	if (view_getfullscreen())
	{
		
		// IM-2013-12-19: [[ ShowAll ]] Check if the view background needs to be drawn
		// IM-2013-12-20: [[ ShowAll ]] Draw the stack into its viewport
		if (!MCU_rect_in_rect(t_update_rect, m_view_stack_visible_rect))
		{
			// IM-2013-10-08: [[ FullscreenMode ]] draw the view backdrop if the render area
			// falls outside the stack rect
			/* OVERHAUL - REVISIT: currently just draws black behind the stack area */
			MCGContextAddRectangle(p_target, MCRectangleToMCGRectangle(t_update_rect));
			MCGContextSetFillRGBAColor(p_target, 0.0, 0.0, 0.0, 1.0);
			MCGContextFill(p_target);
		}

		t_update_rect = MCU_intersect_rect(t_update_rect, m_view_stack_visible_rect);
		
		MCGContextClipToRect(p_target, MCRectangleToMCGRectangle(t_update_rect));
	}

		render(p_target, t_update_rect);
		MCGContextRestore(p_target);
	}

// IM-2013-10-14: [[ FullscreenMode ]] Move update region tracking into view abstraction
void MCStack::view_updatewindow(void)
{
	if (m_view_update_region == nil)
		return;
	
	// IM-2014-01-24: [[ HiDPI ]] Change to use logical coordinates - device coordinate conversion no longer needed
	/* CODE REMOVED */
	
	view_platform_updatewindow(m_view_update_region);
}

void MCStack::view_updatestack(MCRegionRef p_region)
{
	// transform stack region to view region
	MCRegionRef t_view_region;
	t_view_region = nil;

	// IM-2014-01-24: [[ HiDPI ]] Change to use logical coordinates - convert to view coordinates rather than device coords
	
	/* UNCHECKED */ MCRegionTransform(p_region, getviewtransform(), t_view_region);
	
	view_platform_updatewindow(t_view_region);

	MCRegionDestroy(t_view_region);
}

MCPoint MCStack::view_viewtostackloc(const MCPoint &p_loc) const
{
	return MCPointTransform(p_loc, MCGAffineTransformInvert(m_view_transform));
}

MCPoint MCStack::view_stacktoviewloc(const MCPoint &p_loc) const
{
	return MCPointTransform(p_loc, m_view_transform);
}

////////////////////////////////////////////////////////////////////////////////

bool MCStack::view_getacceleratedrendering(void)
{
	return m_view_tilecache != nil;
}

void MCStack::view_setacceleratedrendering(bool p_value)
{
#ifdef _SERVER
    // We don't have accelerated rendering on Server
    return;
#else
    
	// If we are turning accelerated rendering off, then destroy the tilecache.
	if (!p_value)
	{
		MCTileCacheDestroy(m_view_tilecache);
		m_view_tilecache = nil;
		
		// MW-2012-03-15: [[ Bug ]] Make sure we dirty the stack to ensure all the
		//   layer mode attrs are rest.
		dirtyall();
		
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
#elif defined(_WINDOWS_DESKTOP) || defined(_LINUX_DESKTOP) || defined(__EMSCRIPTEN__)
	t_compositor_type = kMCTileCacheCompositorSoftware;
	t_tile_size = 32;
	t_cache_limit = 32 * 1024 * 1024;
#elif defined(_IOS_MOBILE) || defined(_ANDROID_MOBILE)
	t_compositor_type = kMCTileCacheCompositorStaticOpenGL;
	
	const MCDisplay *t_display;
	MCscreen -> getdisplays(t_display, false);
	
	MCRectangle t_viewport;
	t_viewport = t_display -> viewport;
	
	// IM-2014-01-30: [[ HiDPI ]] Use backing-surface size to determine small, medium, or large
	t_viewport = MCRectangleGetScaledBounds(t_viewport, view_getbackingscale());
	
	bool t_small_screen, t_medium_screen;
	t_small_screen = MCMin(t_viewport . width, t_viewport . height) <= 480 && MCMax(t_viewport . width, t_viewport . height) <= 640;
	t_medium_screen = MCMin(t_viewport . width, t_viewport . height) <= 768 && MCMax(t_viewport . width, t_viewport . height) <= 1024;
	
	if (t_small_screen)
		t_tile_size = 32, t_cache_limit = 16 * 1024 * 1024;
	else if (t_medium_screen)
		t_tile_size = 64, t_cache_limit = 32 * 1024 * 1024;
	else
		t_tile_size = 64, t_cache_limit = 64 * 1024 * 1024;
#else
#   error "No tile cache implementation defined for this platform"
#endif
	
	MCTileCacheCreate(t_tile_size, t_cache_limit, m_view_tilecache);
	view_updatetilecacheviewport();
	MCTileCacheSetCompositor(m_view_tilecache, t_compositor_type);
	
	dirtyall();
#endif /* !_SERVER */
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
			
			view_updatetilecacheviewport();
		}
		
		MCTileCacheSetCompositor(m_view_tilecache, p_type);
	}
	
	dirtyall();
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
		dirtyall();
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
		dirtyall();
	}
}

////////////////////////////////////////////////////////////////////////////////

bool view_device_render_background(void *p_context, MCGContextRef p_target, const MCRectangle32& p_rectangle)
{
	/* OVERHAUL - REVISIT: currently just draws black behind the stack area */
	MCGContextAddRectangle(p_target, MCRectangle32ToMCGRectangle(p_rectangle));
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
	t_bg_layer . region = MCRectangle32Make(0, 0, 8192, 8192);
	t_bg_layer . clip = MCRectangle32Make(0, 0, 8192, 8192);
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

void MCStack::view_updatetilecacheviewport(void)
{
	if (m_view_tilecache == nil)
		return;
	
	// IM-2013-10-02: [[ FullscreenMode ]] Use view rect when setting the size of the tilecache
	// IM-2013-10-10: [[ FullscreenMode ]] Align tilecache viewport to origin
	// IM-2014-01-24: [[ HiDPI ]] Set tilecache viewport in backing surface coords
	MCRectangle t_view_rect;
	t_view_rect = MCRectangleMake(0, 0, m_view_rect.width, m_view_rect.height);
	
	MCRectangle t_surface_rect;
	t_surface_rect = MCRectangleGetScaledBounds(t_view_rect, view_getbackingscale());
	
	MCTileCacheSetViewport(m_view_tilecache, t_surface_rect);
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
	
	// IM-2014-01-24: [[ HiDPI ]] use backing surface coords for tilecache operations
	MCRectangle t_device_rect;
	t_device_rect = MCRectangleGetScaledBounds(t_view_rect, view_getbackingscale());
	return MCTileCacheSnapshot(m_view_tilecache, t_device_rect, r_image);
}

////////////////////////////////////////////////////////////////////////////////

// IM-2013-10-14: [[ FullscreenMode ]] Move update region tracking into view abstraction
void MCStack::view_apply_updates()
{
	// IM-2014-09-23: [[ Bug 13349 ]] Sync window geometry before any redraw updates.
	if (m_view_need_resize)
	{
		view_update_geometry();
		m_view_need_redraw = true;
	}
	
	// Ensure the content is up to date.
	if (m_view_need_redraw)
	{
		// IM-2014-09-30: [[ Bug 13501 ]] Unset need_redraw flag here to prevent further updates while drawing

		// We no longer need to redraw.
		m_view_need_redraw = false;
		
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
	}
}

// IM-2013-10-14: [[ FullscreenMode ]] Move update region tracking into view abstraction
void MCStack::view_reset_updates()
{
	MCRegionDestroy(m_view_update_region);
	m_view_update_region = nil;
	m_view_need_redraw = false;
	
	// IM-2014-09-23: [[ Bug 13349 ]] reset geometry update flag
	m_view_need_resize = false;
}

// IM-2013-10-14: [[ FullscreenMode ]] Move update region tracking into view abstraction
void MCStack::view_dirty_rect(const MCRectangle &p_rect)
{
    MCRectangle t_visible_rect = MCRectangleGetTransformedBounds(view_getstackvisiblerect(), view_getviewtransform());
    MCRectangle t_dirty_rect = MCU_intersect_rect(p_rect, t_visible_rect);

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

////////////////////////////////////////////////////////////////////////////////

MCRectangle MCStack::view_getwindowrect(void) const
{
	return view_platform_getwindowrect();
}

MCRectangle MCStack::view_setgeom(const MCRectangle &p_rect)
{
	// IM-2014-09-23: [[ Bug 13349 ]] Defer window resizing if the screen is locked.
	if ((MCRedrawIsScreenLocked() || !MCRedrawIsScreenUpdateEnabled()) && (opened && getflag(F_VISIBLE)))
	{
		m_view_need_resize = true;
		MCRedrawScheduleUpdateForStack(this);
		
		return p_rect;
	}
	
	dirtyall();
	return view_platform_setgeom(p_rect);
}

void MCStack::view_update_geometry()
{
	if (m_view_need_resize &&
        window != NULL)
		view_platform_setgeom(m_view_rect);
	
	m_view_need_resize = false;
}

MCGFloat MCStack::view_getbackingscale(void) const
{
	return m_view_backing_scale;
}

void MCStack::view_setbackingscale(MCGFloat p_scale)
{
	if (p_scale == m_view_backing_scale)
		return;
	
	m_view_backing_scale = p_scale;

	// IM-2014-06-30: [[ Bug 12715 ]] backing scale changes occur when redrawing the whole stack,
	// so we need to update the tilecache here before continuing to draw.
	
	view_flushtilecache();
	view_updatetilecacheviewport();
	view_updatetilecache();
	// IM-2014-10-22: [[ Bug 13746 ]] Update window mask when backing scale changes
	loadwindowshape();
}

////////////////////////////////////////////////////////////////////////////////
