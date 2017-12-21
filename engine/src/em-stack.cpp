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

bool MCStack::view_platform_dirtyviewonresize() const
{
	return true;
}

void
MCStack::view_platform_updatewindow(MCRegionRef p_dirty_region)
{
	/* FIXME implement HiDPI support */

	/* dirtyrect() calls that occur prior to configure() being called
	 * for the first time will result in an update region being too
	 * big. Restrict to a valid region. */

	uint32_t t_window = reinterpret_cast<uint32_t>(window);
	
	MCGRegionRef t_region = MCGRegionRef(p_dirty_region);
	MCRectangle t_valid = MCEmscriptenGetWindowRect(t_window);
	t_valid.x = t_valid.y = 0;

	MCGRegionIntersectRect(t_region, MCRectangleToMCGIntegerRectangle(t_valid));

    MCGIntegerRectangle t_rect = MCGRegionGetBounds(t_region);

	MCEmscriptenSyncCanvasSize(t_window, t_valid.width, t_valid.height);
	
    MCHtmlCanvasStackSurface t_surface(t_window, t_rect);
	view_surface_redrawwindow(&t_surface, t_region);
}

MCRectangle
MCStack::view_platform_setgeom(const MCRectangle &p_rect)
{
	uint32_t t_window = reinterpret_cast<uint32_t>(window);

	MCRectangle t_old = MCEmscriptenGetWindowRect(t_window);
	MCEmscriptenSetWindowRect(t_window, p_rect);
	return t_old;
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
