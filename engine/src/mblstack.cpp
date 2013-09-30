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

#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"

#include "execpt.h"
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


MCStack *MCStack::findchildstackd(Window w,uint2 &ccount,uint2 cindex)
{
	Window pwindow = getparentwindow();
	if (pwindow != DNULL && w == pwindow)
		if  (++ccount == cindex)
			return this;
	if (substacks != NULL)
	{
		MCStack *tptr = substacks;
		do
		{
			pwindow = tptr->getparentwindow();
			if (pwindow != DNULL && w == pwindow)
			{
				ccount++;
				if (ccount == cindex)
					return tptr;
			}
			tptr = (MCStack *)tptr->next();
		}
		while (tptr != substacks);
	}
	return NULL;
}

void MCStack::realize(void)
{
	start_externals();

	// For now, we just use the MCStack* as the window handle...
	window = (Window)this;

	// And by default don't draw anything
	setextendedstate(true, ECS_DONTDRAW);
}

void MCStack::sethints(void)
{
}

void MCStack::destroywindowshape(void)
{
}

MCRectangle MCStack::device_setgeom(const MCRectangle &p_rect)
{
	return p_rect;
}

// IM-2013-09-30: [[ FullscreenMode ]] Mobile version of setgeom now calls view_setgeom
void MCStack::setgeom(void)
{
	if (MCnoui || !opened)
		return;
	
	// IM-2013-08-08: [[ ResIndependence ]] Scale stack rect to device space
	MCRectangle t_old_device_rect;
	t_old_device_rect = view_setgeom(rect);
	
	state &= ~CS_NEED_RESIZE;
	
	// IM-2013-08-08: [[ ResIndependence ]] Scale old window rect to user space for comparison
	MCRectangle t_old_user_rect;
	t_old_user_rect = MCGRectangleGetIntegerBounds(MCResDeviceToUserRect(t_old_device_rect));
	
	if (t_old_user_rect.x != rect.x || t_old_user_rect.y != rect.y || t_old_user_rect.width != rect.width || t_old_user_rect.height != rect.height)
	{
		resize(t_old_user_rect.width, t_old_user_rect.height);
	}
	
}

void MCStack::start_externals()
{
	loadexternals();
}

void MCStack::stop_externals()
{
	unloadexternals();
}

void MCStack::openwindow(Boolean override)
{
	MCscreen -> openwindow(window, override);
}

void MCStack::setopacity(unsigned char p_level)
{
}

void MCStack::updatemodifiedmark(void)
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

MCRectangle MCStack::device_getwindowrect() const
{
    return MCGRectangleGetIntegerInterior(MCResUserToDeviceRect(rect));
}
