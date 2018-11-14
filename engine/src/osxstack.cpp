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

#include "osxprefix.h"

#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"

#include "dispatch.h"
#include "stack.h"
#include "card.h"
#include "group.h"
#include "player.h"
#include "field.h"
#include "stacklst.h"
#include "cardlst.h"
#include "sellst.h"
#include "mcerror.h"
#include "util.h"
#include "param.h"

#include "debug.h"
#include "globals.h"
#include "mode.h"
#include "image.h"
#include "redraw.h"
#include "license.h"
#include "context.h"
#include "region.h"

#include "graphicscontext.h"
#include "resolution.h"

////////////////////////////////////////////////////////////////////////////////

MCStack *MCStack::findstackd(Window w)
{
	if (w == NULL)
		return NULL;
	
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

void MCStack::setsizehints()
{
}

void MCStack::sethints(void)
{
}

extern void MCMacDisableScreenUpdates();
extern void MCMacEnableScreenUpdates();

void MCStack::setgeom()
{
	//set stack(window) size or position from script
	if (!opened)
		return;
	
	// MW-2009-09-25: Ensure things are the right size when doing
	//   remote dialog/menu windows.
	if (window == NULL)
	{
		// MW-2011-08-18: [[ Redraw ]] Update to use redraw.
		MCRedrawLockScreen();
		state &= ~CS_NEED_RESIZE;
		resize(rect . width, rect . height);
		MCRedrawUnlockScreen();
		mode_setgeom();
		return;
	}
	
	// MW-2011-09-12: [[ MacScroll ]] Make sure we apply the current scroll setting.
	applyscroll();
	
	// IM-2013-10-03: [[ FullscreenMode ]] Use view methods to get / set the stack viewport
	MCRectangle t_old_rect;
	t_old_rect = view_getstackviewport();
	
	// MW-2014-02-26: [[ Cocoa ]] It seems that to stop unwanted redraw artifacts we need to
	//   set the frame of the NSWindow and update the content at the same time. To achieve this
	//   neatly we will need to restructure how we go about resizes. However, for now, hopefully
	//   just disabling screen updates and enabling them between setting the frame and resizing
	//   should be enough...
	
	MCMacDisableScreenUpdates();
	
	rect = view_setstackviewport(rect);
	
	state &= ~CS_NEED_RESIZE;
	
	// IM-2013-10-03: [[ FullscreenMode ]] Return values from view methods are
	// in stack coords so don't need to transform
	if (t_old_rect.x != rect.x || t_old_rect.y != rect.y || t_old_rect.width != rect.width || t_old_rect.height != rect.height)
		resize(t_old_rect.width, t_old_rect.height);
	
	MCMacEnableScreenUpdates();
}

// MW-2011-09-12: [[ MacScroll ]] This is called to apply the Mac menu scroll. It
//   causes the stack to be shrunk and the content to be shifted up.
void MCStack::applyscroll(void)
{
	int32_t t_new_scroll;
	t_new_scroll = getnextscroll(false);
	
	// If the scroll isn't changing, do nothing.
	if (t_new_scroll == m_scroll)
		return;
	
    // dirty the current view
	dirtyall();
    
    // Otherwise, set the scroll back to the unmolested version.
	rect . height += m_scroll;
	
	// Update the scroll value.
	m_scroll = t_new_scroll;
	
	// Update the rect...
	rect . height -= t_new_scroll;
	
	// Make sure window contents reflects the new scroll.
	syncscroll();
	
	// IM-2013-10-11: [[ FullscreenMode ]] Trigger redraw of stack after scroll changes
	dirtyall();
}

// MW-2011-09-12: [[ MacScroll ]] This is called to clear any currently applied
//   Mac menu scroll.
void MCStack::clearscroll(void)
{
	if (m_scroll == 0)
		return;
	
	// Set the rect back.
	rect . height += m_scroll;

	// Reset the scroll.
	m_scroll = 0;
	
	// Make sure window contents reflects the new scroll.
	syncscroll();
}

void MCStack::platform_openwindow(Boolean p_override)
{
	if (MCModeMakeLocalWindows() && window != NULL)
		MCscreen -> openwindow(window, p_override);
}

void MCStack::redrawicon()
{
}

void MCStack::enablewindow(bool p_enable)
{
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
