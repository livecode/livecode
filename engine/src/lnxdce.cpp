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

//
// ScreenDC event and signal handling functions
//
#include "lnxprefix.h"

#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"
#include "mcio.h"

#include "dispatch.h"
#include "stack.h"
#include "card.h"
#include "image.h"
#include "util.h"
#include "date.h"
#include "param.h"
#include "execpt.h"
#include "notify.h"
#include "eventqueue.h"
#include "region.h"
#include "osspec.h"
#include "mode.h"
#include "globals.h"
#include "notify.h"
#include "redraw.h"

#include "lnxdc.h"

#include "resolution.h"

#define WM_TITLE_HEIGHT 16

// IM-2014-01-29: [[ HiDPI ]] Placeholder method for Linux HiDPI support
void MCScreenDC::platform_boundrect(MCRectangle &rect, Boolean title, Window_mode m)
{
	device_boundrect(rect, title, m);
}

void MCScreenDC::device_boundrect(MCRectangle &rect, Boolean title, Window_mode m)
{
	MCRectangle srect;
	if (m >= WM_MODAL)
		MCU_set_rect(srect, 0, 0, device_getwidth(), device_getheight());
	else
	{
		// IM-2014-01-29: [[ HiDPI ]] Convert logical to screen coords
		srect = logicaltoscreenrect(MCwbr);
	}
	
	
	if (rect.x < srect.x)
		rect.x = srect.x;
	else
		if (rect.x + rect.width > srect.x + srect.width)
			rect.x = srect.x + srect.width - rect.width;
	int2 top = title ? WM_TITLE_HEIGHT : 0;
	srect.y += top;
	srect.height -= top;
	if (rect.y < srect.y)
		rect.y = srect.y;
	else
		if (rect.y + rect.height > srect.y + srect.height)
			rect.y = srect.y + srect.height - rect.height;
}

void MCScreenDC::expose()
{
	XEvent event;

	while (True)
	{
		MCRegionRef t_dirty;
		t_dirty = nil;
		
		Window t_window;
		if (XCheckTypedEvent(dpy, Expose, &event))
		{
			XExposeEvent *eevent = (XExposeEvent *)&event;
			t_window = eevent -> window;

			MCRegionCreate(t_dirty);
			MCRegionIncludeRect(t_dirty, MCU_make_rect(eevent->x, eevent->y, eevent->width, eevent->height));
			while (XCheckTypedWindowEvent(dpy, t_window, Expose, &event))
				MCRegionIncludeRect(t_dirty,  MCU_make_rect(eevent->x, eevent->y, eevent->width, eevent->height));
		}
		else if (XCheckTypedEvent(dpy, GraphicsExpose, &event))
		{
			XGraphicsExposeEvent *geevent = (XGraphicsExposeEvent *)&event;
			t_window = geevent -> drawable;

			MCRegionCreate(t_dirty);
			MCRegionIncludeRect(t_dirty, MCU_make_rect(geevent->x, geevent->y, geevent->width, geevent->height));
			while (XCheckTypedWindowEvent(dpy, t_window, GraphicsExpose, &event))
				MCRegionIncludeRect(t_dirty,  MCU_make_rect(geevent->x, geevent->y, geevent->width, geevent->height));
		}
		else
			break;

		MCStack *t_stack;
		t_stack = MCdispatcher -> findstackd(t_window);
		if (t_stack != nil)
			t_stack -> onexpose(t_dirty);

		MCRegionDestroy(t_dirty);
	}
}

Boolean MCScreenDC::abortkey()
{
	if (MCabortscript)
		return True;
	if (!MCalarm)
		return False;
	MCU_play();
	MCalarm = False;
	XFlush(dpy);
	Boolean abort, reset;
	handle(False, False, abort, reset);
	if (pendingevents != NULL)
	{
		MCEventnode *tptr = pendingevents;
		KeyCode abortcode1 = XKeysymToKeycode(dpy, XK_Break);
		KeyCode abortcode2 = XKeysymToKeycode(dpy, '.');
		do
		{
			if (tptr->event.type == KeyPress)
			{
				XKeyEvent *kpevent = (XKeyEvent *)&tptr->event;
				if (kpevent->state & ControlMask
				        && kpevent->keycode==abortcode1 || kpevent->keycode==abortcode2)
				{
					abort = True;
					tptr->remove
					(pendingevents);
					delete tptr;
					if (pendingevents == NULL)
						break;
					tptr = pendingevents;
					continue;
				}
			}
			tptr = (MCEventnode *)tptr->next();
		}
		while (tptr != pendingevents);
	}
	if (abort)
		if (MCallowinterrupts && !MCdefaultstackptr->cantabort())
			return True;
		else
			MCinterrupt = True;
	return False;
}

void MCScreenDC::waitconfigure(Window w)
{
}

void MCScreenDC::waitreparent(Window w)
{
	waitmessage(w, ReparentNotify);
}

void MCScreenDC::waitfocus()
{
	XSync(dpy, False);
	XEvent event;
	XFocusChangeEvent *foevent = (XFocusChangeEvent *)&event;
	if (XCheckTypedEvent(dpy, FocusOut, &event))
		MCdispatcher->wkunfocus(foevent->window);
}

// IM-2014-01-29: [[ HiDPI ]] Placeholder method for Linux HiDPI support
void MCScreenDC::platform_querymouse(int16_t &x, int16_t &y)
{
	device_querymouse(x, y);
}

void MCScreenDC::device_querymouse(int2 &x, int2 &y)
{
	Window root, child;
	int rx, ry, wx, wy;
	unsigned int state;
	XQueryPointer(dpy, getroot(), &root, &child, &rx, &ry, &wx, &wy, &state);
	x = rx;
	y = ry;
}

uint2 MCScreenDC::querymods()
{
	if (lockmods)
		return MCmodifierstate;
	Window root, child;
	int rx, ry, wx, wy;
	unsigned int state;
	XQueryPointer(dpy, getroot(), &root, &child, &rx, &ry, &wx, &wy, &state);
	
	// MW-2010-10-13: [[ Bug 9059 ]] Map the X masks to our masks correctly - previously
	//   this ditched the capsLock mask.
	unsigned int t_rstate;
	t_rstate = 0;
	if ((state & ShiftMask) != 0)
		t_rstate |= MS_SHIFT;
	if ((state & LockMask) != 0)
		t_rstate |= MS_CAPS_LOCK;
	if ((state & ControlMask) != 0)
		t_rstate |= MS_CONTROL;
	if ((state & Mod1Mask) != 0)
		t_rstate |= MS_MOD1;
	if ((state & Mod2Mask) != 0)
		t_rstate |= MS_MOD2;

	return t_rstate;
}

// IM-2014-01-29: [[ HiDPI ]] Placeholder method for Linux HiDPI support
void MCScreenDC::platform_setmouse(int16_t x, int16_t y)
{
	device_setmouse(x, y);
}

void MCScreenDC::device_setmouse(int2 x, int2 y)
{
	XWarpPointer(dpy, None, getroot(), 0, 0, 0, 0, x, y);
}

Boolean MCScreenDC::getmouse(uint2 button, Boolean& r_abort)
{
	static real8 lasttime;
	real8 newtime = MCS_time();
	real8 sr = (real8)9.0 / 1000.0;
	if ((newtime - lasttime) < sr)
	{
		r_abort = MCscreen->wait(sr, False, False);
		if (r_abort)
			return False;
	}
	else
		r_abort = False;
	lasttime = newtime;
	Window root, child;
	int rx, ry, wx, wy;
	unsigned int state;
	XQueryPointer(dpy, getroot(), &root, &child, &rx, &ry, &wx, &wy, &state);
	state = state >> 8 & 0x1F;
	
	//TS-2008-03-25 : Bug [[6199]] - Linux mouse buttons for mouse() not matching up with other platforms.
	return button == 0 && state || state & (0x1L << button - 1);
//	return button == 0 && state || state & ~(0x1L << button - 1);
}

Boolean MCScreenDC::getmouseclick(uint2 button, Boolean& r_abort)
{
	Boolean abort, reset;
	handle(False, False, abort, reset);
	if (abort)
	{
		r_abort = True;
		return False;
	}
	
	r_abort = False;
	
	MCEventnode *tptr = pendingevents;
	MCEventnode *pressptr = NULL;
	MCEventnode *releaseptr = NULL;
	if (pendingevents != NULL)
		do
		{
			if (tptr->event.type == ButtonPress)
			{
				XButtonEvent *bpevent = (XButtonEvent *)&tptr->event;
				if (button == 0
				        || bpevent->state >> 8 & 0x1F & ~(0x1L << button - 1))
				{
					setmods(bpevent->state, 0, bpevent->button, False);
					
					MCPoint t_clickloc;
					t_clickloc = MCPointMake(bpevent->x, bpevent->y);
					
					// IM-2014-01-29: [[ HiDPI ]] Convert screen to logical coords
					t_clickloc = screentologicalpoint(t_clickloc);
					
					// IM-2013-10-09: [[ FullscreenMode ]] Update clickloc with MCscreen getters & setters
					MCscreen->setclickloc(MCmousestackptr, t_clickloc);
					
					pressptr = tptr;
					tptr = (MCEventnode *)tptr->next();
					break;
				}
			}
			tptr = (MCEventnode *)tptr->next();
		}
		while (tptr != pendingevents);
	Boolean value = False;
	if (tptr != pendingevents)
		do
		{
			if (tptr->event.type == ButtonRelease)
			{
				XButtonEvent *brevent = (XButtonEvent *)&tptr->event;
				if (button == 0
				        || brevent->state >> 8 & 0x1F & ~(0x1L << button - 1))
				{
					setmods(brevent->state, 0, brevent->button, False);
					releaseptr = tptr;
					break;
				}
			}
			tptr = (MCEventnode *)tptr->next();
		}
		while (tptr != pendingevents);
	if (pressptr != NULL && releaseptr != NULL)
		value = True;
	else
		pressptr = releaseptr = NULL;
	if (pressptr != NULL)
	{
		tptr = (MCEventnode *)pressptr->remove
		       (pendingevents);
		delete tptr;
	}
	if (releaseptr != NULL)
	{
		tptr = (MCEventnode *)releaseptr->remove
		       (pendingevents);
		delete tptr;
	}
	return value;
}

Boolean MCScreenDC::wait(real8 duration, Boolean dispatch, Boolean anyevent)
{
    MCDeletedObjectsEnterWait(dispatch);
    
	MCwaitdepth++;
	real8 curtime = MCS_time();
	if (duration < 0.0)
		duration = 0.0;
	real8 exittime = curtime + duration;
	Boolean abort = False;
	Boolean reset = False;
	modalclosed = False;
	Boolean done = False;
	Boolean donepending = False;
	do
	{
		// IM-2014-03-06: [[ revBrowserCEF ]] Call additional runloop callbacks
		DoRunloopActions();

		real8 eventtime = exittime;
		donepending = handlepending(curtime, eventtime, dispatch);
		siguser();
		
		// MM-2012-09-04: Make sure MCModeQueueEvents is called.  This is used by the property listener feature.
		MCModeQueueEvents();
		
		if (MCplayers != NULL)
			IO_cleanprocesses();

		if (modalclosed ||
			(MCNotifyDispatch(dispatch == True) ||
			 dispatch && MCEventQueueDispatch() ||
			 handle(dispatch, anyevent, abort, reset) ||
			 donepending) && anyevent ||
			 abort)
			break;

		if (MCquit)
			break;

		// MW-2012-09-19: [[ Bug 10218 ]] Make sure we update the screen in case
		//   any engine event handling methods need us to.
		MCRedrawUpdateScreen();
        MCDeletedObjectsDrain();

		if (curtime < eventtime)
			done = MCS_poll(donepending ? 0 : eventtime - curtime, ConnectionNumber(dpy));
		curtime = MCS_time();
        
        MCDeletedObjectsDrain();
	}
	while (curtime < exittime && !(anyevent && (done || donepending)));
	if (MCquit)
		abort = True;
	MCwaitdepth--;
	
	// MW-2012-09-19: [[ Bug 10218 ]] Make sure we update the screen in case
	//   any engine event handling methods need us to.
	MCRedrawUpdateScreen();

    MCDeletedObjectsLeaveWait(dispatch);
    
	return abort;
}

void MCScreenDC::flushevents(uint2 e)
{
	static int event_types[FE_LAST] = { 0, ButtonPress, ButtonRelease,
	                                    KeyPress, KeyRelease, 1, 1,
	                                    FocusIn, 1, 1 };
	Boolean abort, reset;
	handle(False, False, abort, reset);
	if (pendingevents != NULL)
	{
		MCEventnode *tptr = pendingevents;
		Boolean done;
		do
		{
			done = True;
			if (e == FE_ALL || event_types[e] == tptr->event.type)
			{
				tptr->remove
				(pendingevents);
				delete tptr;
				if (pendingevents == NULL)
					break;
				tptr = pendingevents;
				done = False;
			}
			else
				tptr = (MCEventnode *)tptr->next();
		}
		while (!done || tptr != pendingevents);
	}
}

Boolean MCScreenDC::istripleclick()
{
	return tripleclick;
}
