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

#include "globals.h"

// MM-2013-03-06: [[ RefactorGraphics ]] Added to allow server engines to compile.
MCGFloat MCResGetDeviceScale(void)
{
	return 1.0;
}

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
}

void MCStack::sethints(void)
{
}

void MCStack::destroywindowshape(void)
{
}

void MCStack::setgeom(void)
{
	state &= ~CS_NEED_RESIZE;
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

void MCStack::device_updatewindow(MCRegionRef region)
{
}

void MCStack::clearscroll(void)
{
}

void MCStack::applyscroll(void)
{
}

void MCStack::effectrect(const MCRectangle& p_rect, Boolean& r_abort)
{
	MCEffectList *t_effects = MCcur_effects;
	MCcur_effects = NULL;
	
	// There are no visual effects in server mode
	while(t_effects != NULL)
	{
		MCEffectList *t_effect;
		t_effect = t_effects;
		t_effects = t_effects -> next;
		delete t_effect;
	}
}

// MM-2013-03-06: [[ RefactorGraphics ]] Added to allow server engines to compile.
MCRectangle MCStack::device_getwindowrect() const
{
	return rect;	
}

void MCStack::redrawicon(void)
{
}

void MCStack::enablewindow(bool enable)
{
}
