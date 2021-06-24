/*                                                                     -*-c++-*-

Copyright (C) 2003-2015 LiveCode Ltd.

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

#include "em-dc.h"
#include "em-view.h"
#include "em-surface.h"
#include "em-util.h"

#include "globdefs.h"
#include "objdefs.h"
#include "parsedef.h"
#include "sysdefs.h"
#include "stack.h"
#include "resolution.h"
#include "graphics_util.h"
#include "globals.h"

#include "region.h"

/* ================================================================
 * Stack initialisation
 * ================================================================ */

void
MCStack::realize()
{
	view_setbackingscale(MCResGetPixelScale());

	start_externals();

	uint32_t t_window = MCEmscriptenCreateWindow();
	window = reinterpret_cast<Window>(t_window);

	/* By default, don't draw anything */
	setextendedstate(true, ECS_DONTDRAW);
}

void
MCStack::start_externals()
{
	loadexternals();
}

void
MCStack::stop_externals()
{
	unloadexternals();
}

/* ================================================================
 * Window management
 * ================================================================ */

void
MCStack::platform_openwindow(Boolean override)
{
	MCAssert(window != nil);

	/* Make sure to reset the geometry on the window before mapping it. */
	setgeom();

	MCscreen->openwindow(window, override);
}

void
MCStack::setgeom()
{
	if (!opened)
		return;

	MCRectangle t_old_rect;
	t_old_rect = rect;

	rect = view_setstackviewport(rect);

	/* Resize the stack, if necessary */
	state &= ~CS_NEED_RESIZE;

	if (t_old_rect.x      != rect.x ||
	    t_old_rect.y      != rect.y ||
	    t_old_rect.width  != rect.width ||
	    t_old_rect.height != rect.height)
	{
		resize(t_old_rect.width, t_old_rect.height);
	}
}

void
MCStack::release_window_buffer()
{
}

/* ================================================================
 * View management
 * ================================================================ */

static MCStackUpdateCallback s_updatewindow_callback = nullptr;
static void *s_updatewindow_context = nullptr;

bool MCStack::view_platform_dirtyviewonresize() const
{
	return true;
}

void MCStack::view_platform_updatewindowwithcallback(MCRegionRef p_region, MCStackUpdateCallback p_callback, void *p_context)
{
	s_updatewindow_callback = p_callback;
	s_updatewindow_context = p_context;

	view_platform_updatewindow(p_region);

	s_updatewindow_callback = nil;
	s_updatewindow_context = nil;
}

void MCStack::view_platform_updatewindow(MCRegionRef p_dirty_region)
{
	MCRegionRef t_scaled_region;
	t_scaled_region = nil;
	
	MCRegionRef t_screen_region;
	t_screen_region = nil;
	
	MCGFloat t_scale;
	t_scale = MCResGetPixelScale();
	
	if (t_scale != 1.0)
	{
		/* UNCHECKED */ MCRegionTransform(p_dirty_region, MCGAffineTransformMakeScale(t_scale, t_scale), t_scaled_region);
		t_screen_region = t_scaled_region;
	}
	else
		t_screen_region = p_dirty_region;
	
	view_device_updatewindow(t_screen_region);
	
	if (t_scaled_region != nil)
		MCRegionDestroy(t_scaled_region);
}

void MCStack::view_device_updatewindow(MCRegionRef p_region)
{
	/* dirtyrect() calls that occur prior to configure() being called
	 * for the first time will result in an update region being too
	 * big. Restrict to a valid region. */

	uint32_t t_window = reinterpret_cast<uint32_t>(window);
	
	MCRectangle t_window_rect = MCEmscriptenGetWindowRect(t_window);
	MCRectangle t_canvas_rect = MCRectangleGetScaledCeilingRect(t_window_rect, MCResGetPixelScale());
	t_canvas_rect.x = t_canvas_rect.y = 0;

	MCGRegionRef t_region = MCGRegionRef(p_region);

	MCGRegionIntersectRect(t_region, MCRectangleToMCGIntegerRectangle(t_canvas_rect));

	MCGIntegerRectangle t_rect = MCGRegionGetBounds(t_region);
	MCEmscriptenSyncCanvasSize(t_window, t_canvas_rect.width, t_canvas_rect.height);

	// IM-2014-01-30: [[ HiDPI ]] Ensure stack backing scale is set
	view_setbackingscale(MCResGetPixelScale());

	MCHtmlCanvasStackSurface t_surface(t_window, MCGRegionGetBounds(t_region));
	if (t_surface.Lock())
	{
		// IM-2014-01-31: [[ HiDPI ]] If a callback is given then use it to render to the surface
		if (s_updatewindow_callback != nil)
			s_updatewindow_callback(&t_surface, (MCRegionRef)t_region, s_updatewindow_context);
		else
			view_surface_redrawwindow(&t_surface, t_region);

		t_surface.Unlock();
	}
}

MCRectangle
MCStack::view_platform_setgeom(const MCRectangle &p_rect)
{
	uint32_t t_window = reinterpret_cast<uint32_t>(window);

	MCRectangle t_old = MCEmscriptenGetWindowRect(t_window);
	MCEmscriptenSetWindowRect(t_window, p_rect);
	return t_old;
}

MCRectangle MCStack::view_platform_getwindowrect() const
{
	uint32_t t_window = reinterpret_cast<uint32_t>(window);

	return MCEmscriptenGetWindowRect(t_window);
}

/* ================================================================
 * Stub functions
 * ================================================================ */

void
MCStack::sethints()
{
}

void
MCStack::applyscroll()
{
}

void
MCStack::clearscroll()
{
}

void
MCStack::setsizehints()
{
}

void
MCStack::updateignoremouseevents()
{
}
