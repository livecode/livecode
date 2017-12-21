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


#include "dispatch.h"
#include "stack.h"
#include "card.h"
#include "group.h"
#include "image.h"
#include "field.h"
#include "stacklst.h"
#include "cardlst.h"
#include "sellst.h"
#include "handler.h"
#include "mcerror.h"
#include "param.h"
#include "util.h"
#include "debug.h"
#include "player.h"
#include "visual.h"
#include "tilecache.h"

#include "mbldc.h"
#include "region.h"

#include "globals.h"

#include "resolution.h"

MCStack *MCStack::findstackd(Window w)
{
	if (w == window)
		return this;
	if (substacks != NULL)
	{
		MCStack *tptr = substacks;
		do
		{
			if (w == tptr->window)
				return tptr;
			tptr = (MCStack *)tptr->next();
		}
		while (tptr != substacks);
	}
	return NULL;
}


void MCStack::realize(void)
{
	// IM-2014-07-21: [[ Bug 12860 ]] Initialize window backing scale to the pixel scale
	view_setbackingscale(MCResGetPixelScale());
	
	start_externals();

	// For now, we just use the MCStack* as the window handle...
	window = (Window)this;

	// And by default don't draw anything
	setextendedstate(true, ECS_DONTDRAW);
}

void MCStack::setsizehints(void)
{
}

void MCStack::sethints(void)
{
}

void MCStack::destroywindowshape(void)
{
}

// IM-2013-09-30: [[ FullscreenMode ]] Mobile version of setgeom now calls view methods
void MCStack::setgeom(void)
{
	if (!opened)
		return;
	
	// IM-2013-10-03: [[ FullscreenMode ]] Use view methods to get / set the stack viewport
	// IM-2013-10-14: [[ FullscreenMode ]] Compare the new rect with the stack's existing rect
	MCRectangle t_old_rect;
	t_old_rect = rect;
	
	rect = view_setstackviewport(rect);
	
	state &= ~CS_NEED_RESIZE;
	
	// IM-2013-10-03: [[ FullscreenMode ]] Return values from view methods are
	// in stack coords so don't need to transform
	if (t_old_rect.x != rect.x || t_old_rect.y != rect.y || t_old_rect.width != rect.width || t_old_rect.height != rect.height)
		resize(t_old_rect.width, t_old_rect.height);
	
}

void MCStack::start_externals()
{
	loadexternals();
}

void MCStack::stop_externals()
{
	unloadexternals();
}

void MCStack::platform_openwindow(Boolean override)
{
	MCscreen -> openwindow(window, override);
}

void MCStack::setopacity(unsigned char p_level)
{
}

void MCStack::updatemodifiedmark(void)
{
}

// MERG-2014-06-02: [[ IgnoreMouseEvents ]] Stub for ignoreMouseEvents.
void MCStack::updateignoremouseevents(void)
{
}

void MCStack::redrawicon(void)
{
}

void MCStack::enablewindow(bool enable)
{
}

void MCStack::applyscroll(void)
{
}

void MCStack::clearscroll(void)
{
}

// MERG-2015-10-12: [[ DocumentFilename ]] Stub for documentFilename.
void MCStack::updatedocumentfilename(void)
{
}

////////////////////////////////////////////////////////////////////////////////

bool MCStack::view_platform_dirtyviewonresize() const
{
	return false;
}

MCRectangle MCStack::view_platform_getwindowrect() const
{
	return view_getrect();
}

MCRectangle MCStack::view_platform_setgeom(const MCRectangle &p_rect)
{
	return p_rect;
}

// IM-2014-01-30: [[ HiDPI ]] Platform wrapper converts logical to screen coords
void MCStack::view_platform_updatewindow(MCRegionRef p_dirty_region)
{
	MCRegionRef t_scaled_region;
	t_scaled_region = nil;
	
	MCRegionRef t_screen_region;
	t_screen_region = nil;
	
	MCGFloat t_scale;
	t_scale = MCScreenDC::logicaltoscreenscale();
	
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

////////////////////////////////////////////////////////////////////////////////

bool MCStack::configure_window_buffer()
{
	return true;
}

void MCStack::release_window_buffer()
{
}

////////////////////////////////////////////////////////////////////////////////
