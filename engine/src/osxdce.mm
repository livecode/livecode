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
#include "osxprefix-legacy.h"

#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"

#include "dispatch.h"
#include "stack.h"
#include "card.h"
#include "image.h"
#include "util.h"
#include "date.h"
#include "param.h"
//#include "execpt.h"
#include "player.h"
#include "group.h"
#include "button.h"
#include "globals.h"
#include "mode.h"
#include "eventqueue.h"
#include "osspec.h"
#include "redraw.h"

#include "osxdc.h"

#include "resolution.h"

#include <AppKit/AppKit.h>

////////////////////////////////////////////////////////////////////////////////

Boolean tripleclick = False;

////////////////////////////////////////////////////////////////////////////////

extern bool MCOSXGetDisplayPixelScale(NSScreen *p_display, MCGFloat &r_scale);

////////////////////////////////////////////////////////////////////////////////

static inline MCRectangle MCRectangleFromNSRect(const NSRect &p_rect)
{
	return MCRectangleMake(p_rect.origin.x, p_rect.origin.y, p_rect.size.width, p_rect.size.height);
}

MCRectangle MCRectangleFlipYAxis(const MCRectangle &p_rect, uint32_t p_display_height)
{
	return MCRectangleMake(p_rect.x, p_display_height - (p_rect.y + p_rect.height), p_rect.width, p_rect.height);
}

////////////////////////////////////////////////////////////////////////////////

// IM-2014-01-23: [[ HiDPI ]] Retreive the geometry of the screen as well as the backing scale if available
void MCOSXGetDisplayInfo(NSScreen *p_screen, MCDisplay &r_display)
{
	MCGFloat t_scale;
	if (!MCOSXGetDisplayPixelScale(p_screen, t_scale))
		t_scale = 1.0;
	
	r_display.pixel_scale = t_scale;
	
	r_display.viewport = MCRectangleFromNSRect([p_screen frame]);
	r_display.workarea = MCRectangleFromNSRect([p_screen visibleFrame]);
}

// IM-2014-01-23: [[ HiDPI ]] Retreive the display info of the main screen
void MCOSXGetMainDisplayInfo(MCDisplay &r_display)
{
	MCOSXGetDisplayInfo([NSScreen mainScreen], r_display);
}

// IM-2014-01-23: [[ HiDPI ]] Return an array of display information for all available screens
bool MCOSXGetDisplays(MCDisplay *&r_displays, uint32_t &r_count)
{
	bool t_success;
	t_success = true;
	
	MCDisplay *t_displays;
	t_displays = nil;
	
	uint32_t t_count;
	t_count = 0;
	
	// The main screen will always be at index 0 of the returned array
	NSArray *t_screens;
	t_screens = [NSScreen screens];
	
	t_success = t_screens != nil;
	
	if (t_success)
	{
		t_count = t_screens.count;
		t_success = MCMemoryNewArray(t_count, t_displays);
	}
	
	for (uindex_t i = 0; t_success && i < t_count; i++)
	{
		MCOSXGetDisplayInfo([t_screens objectAtIndex:i], t_displays[i]);
		t_displays[i].index = i;
	}
	
	if (t_success)
	{
		r_displays = t_displays;
		r_count = t_count;
	}
	else
		MCMemoryDeleteArray(t_displays);
	
	return t_success;
}


// IM-2014-01-23: [[ HiRes ]] Reimplemented to use Cocoa, allowing us to get the pixel scale of each display.
bool MCScreenDC::platform_getdisplays(bool p_effective, MCDisplay *& r_displays, uint32_t &r_count)
{
	bool t_success;
	t_success = true;
	
	MCDisplay *t_displays;
	t_displays = nil;
	
	uint32_t t_display_count;
	t_display_count = 0;
	
	// The main screen will always be at index 0 of the returned array
	NSArray *t_screens;
	t_screens = [NSScreen screens];
	
	t_success = t_screens != nil;
	
	if (t_success)
	{
		t_display_count = t_screens.count;
		t_success = MCMemoryNewArray(t_display_count, t_displays);
	}
	
	for (uindex_t i = 0; t_success && i < t_display_count; i++)
	{
		MCOSXGetDisplayInfo([t_screens objectAtIndex:i], t_displays[i]);
		t_displays[i].index = i;
	}
	
	if (!t_success)
	{
		t_success = MCMemoryNew(t_displays);
		
		if (t_success)
		{
			t_display_count = 1;
			MCOSXGetMainDisplayInfo(*t_displays);
		}
	}
	
	if (t_success)
	{
		// flip origin of OSX screenrects
		MCRectangle t_main_viewport;
		t_main_viewport = t_displays[0].viewport;
		
		for (uint32_t i = 0; i < t_display_count; i++)
		{
			t_displays[i].viewport = MCRectangleFlipYAxis(t_displays[i].viewport, t_main_viewport.height);
			t_displays[i].workarea = MCRectangleFlipYAxis(t_displays[i].workarea, t_main_viewport.height);
		}
		
		r_displays = t_displays;
		r_count = t_display_count;
	}
	
	return t_success;
}

// IM-2014-01-29: [[ HiDPI ]] We receive notification of desktop changes on OSX, so can safely cache display info
bool MCScreenDC::platform_displayinfocacheable(void)
{
	return true;
}

// IM-2014-01-24: [[ HiDPI ]] Change to use logical coordinates - device coordinate conversion no longer needed
void MCScreenDC::platform_boundrect(MCRectangle &rect, Boolean title, Window_mode mode)
{	
	MCRectangle srect;

	if (mode >= WM_MODAL)
	{
		const MCDisplay *t_display;
		t_display = getnearestdisplay(rect);
		srect = t_display -> workarea;
	}
	else
		srect = MCwbr;

	uint2 sr, sw, sb, sh;
	Rect screenRect;
	screenRect = MCRectToMacRect(srect);

	if (title && mode <= WM_SHEET && mode != WM_DRAWER)
	{
		if (mode == WM_PALETTE)
			screenRect.top += 13;
		else
		{
			long osversion;
			Gestalt(gestaltSystemVersion, &osversion);
			if (osversion >= 0x00000800)
				screenRect.top += 22;
			else
				screenRect.top += 19;
		}
		sr = sb = 10;
		sw = 20;
		sh = 12;
	}
	else
		sr = sw = sb = sh = 0;

	if (rect.x < screenRect.left)
		rect.x = screenRect.left;
	if (rect.x + rect.width > screenRect.right - sr)
	{
		if (rect.width > screenRect.right - screenRect.left - sw)
			rect.width = screenRect.right - screenRect.left - sw;
		rect.x = screenRect.right - rect.width - sr;
	}

	if (rect.y < screenRect.top)
		rect.y = screenRect.top;
	if (rect.y + rect.height > screenRect.bottom - sb)
	{
		if (rect.height > screenRect.bottom - screenRect.top - sh)
			rect.height = screenRect.bottom - screenRect.top - sh;
		rect.y = screenRect.bottom - rect.height - sb;
	}
}

void MCScreenDC::expose()
{
	SetGWorld(GetWindowPort(getinvisiblewin()), GetMainDevice());

	EventRecord event;
	while (GetNextEvent(updateMask, &event))
		doredraw(event);
}

Boolean MCScreenDC::abortkey()
{ /* check for Command-. system abort key */
	if (MCabortscript)
		return True;
	static uint4 alarmcount;
	uint4 newcount = TickCount();
	if (newcount < alarmcount)
		return False;
	alarmcount = newcount + 30;
			
	// MW-2008-07-31: [[ Bug 6850 ]] Variable watcher breaks out when using scroll wheel.
	//   This bug was caused by the previous version of checking for an abortkey causing
	//   Carbon events to be handled. This can result in things such WheelUp/WheelDown
	//   events being dispatched *inside* previous invocations.
	if (CheckEventQueueForUserCancel())
	{
		if (MCallowinterrupts && !MCdefaultstackptr -> cantabort())
			return True;
		else
			MCinterrupt = True;
			
		// OK-2010-04-29: [[Bug]] - cantAbort / allowInterrupts not working on OS X
		return False;
	}
			
	if (MCnsockets != 0)
		MCS_handle_sockets();
	return False;
}

// IM-2014-01-24: [[ HiDPI ]] Change to use logical coordinates - device coordinate conversion no longer needed
void MCScreenDC::platform_querymouse(int2 &x, int2 &y)
{
	CGrafPtr oldport;
	GDHandle olddevice;
	GetGWorld(&oldport, &olddevice);
	MoveWindow(invisibleWin, 0, 0, False); // OS X moves this.
	SetGWorld(GetWindowPort(invisibleWin), GetMainDevice());
	Point mloc;
	GetMouse(&mloc);      //get local mouse position
	SetGWorld(oldport, olddevice);
	
	x = mloc.h;
	y = mloc.v;
}

static Boolean isKeyPressed(unsigned char *km, uint1 keycode)
{
	return (km[keycode >> 3] >> (keycode & 7)) & 1;
}

// MW-2008-06-12: Updated to use more modern GetCurrentKeyModifiers function
//   to fetch the modifier state.
uint2 MCScreenDC::querymods()
{
	if (lockmods)
		return MCmodifierstate;
		
	UInt32 t_modifiers;
	t_modifiers = GetCurrentKeyModifiers();
	
	uint2 state;
	state = 0;
	
	if ((t_modifiers & (1 << shiftKeyBit)) != 0)
		state |= MS_SHIFT;
	
	if ((t_modifiers & (1 << cmdKeyBit)) != 0)
		state |= MS_CONTROL;
		
	if ((t_modifiers & (1 << optionKeyBit)) != 0)
		state |= MS_MOD1;
		
	if ((t_modifiers & (1 << controlKeyBit)) != 0)
		state |= MS_MOD2;
		
	if ((t_modifiers & (1 << alphaLockBit)) != 0)
		state |= MS_CAPS_LOCK;
		
	return state;
}

void MCScreenDC::platform_setmouse(int2 x, int2 y)
{ //move mouse/cursor to new (x,y) location

	CGPoint point;
	point.x = x;
	point.y = y;
	CGWarpMouseCursorPosition(point);
}

// MW-2008-06-12: [[ Bug 6396 ]] Make sure r_abort is set to False if
//   the first two checks against 'button' fail. Also update the code
//   to use GetCurrentButtonState
Boolean MCScreenDC::getmouse(uint2 button, Boolean& r_abort)
{
	// Make the check interval what it always has been - 9!
	static real8 lasttime;
	real8 newtime = MCS_time();
	real8 sr = (real8)9.0 / 1000.0;
	if ((newtime - lasttime) < sr)
	{
		r_abort = MCscreen->wait(sr, False, False);
		if (r_abort)
			return False;
	}
	
	r_abort = False;
	lasttime = newtime;
	
	UInt32 t_state;
	t_state = GetCurrentButtonState();
	
	if (button == 0)
		return t_state != 0;
	
	if (button == 1)
		return (t_state & (1 << 0)) != 0;
		
	if (button == 2)
		return (t_state & (1 << 2)) != 0;
		
	if (button == 3)
		return (t_state & (1 << 1)) != 0 || ((t_state & (1 << 0)) != 0 && (querymods() & MS_MOD2) != 0);
		
	return False;
}

Boolean MCScreenDC::getmouseclick(uint2 button, Boolean& r_abort)
{/* check which mouse is down & set the modifier key states.            *
	  * check the pendingevents queue first, if there's events in the queue *
	  * process the events, and remove them.                                */
	if (!MCModeMakeLocalWindows())
	{
		r_abort = wait(0.0, False, True);
		if (r_abort)
			return False;
		return MCEventQueueGetMouseClick(button);
	}
	
	r_abort = this->wait(0.0, False, True);
	if (r_abort)
		return False;

	MCEventnode *tptr = pendingevents;
	MCEventnode *pressptr = NULL;
	MCEventnode *releaseptr = NULL;
	if (pendingevents != NULL)
		do
		{ //Mac has only 1 mouse button, there is no need to check which
			if (tptr->event.what == mouseDown)
			{ //button is down
				setmods(tptr->event.modifiers);
				
				// MW-2012-02-08: [[ Bug 10063 ]] Make sure we sync the mouseloc state
				//   to coincide with the click so clickLoc works correctly.
				Point p;
				p = tptr -> event . where;
				// MW-2012-09-21: [[ Bug 10404 ]] Make sure we only use mfocus to sync
				//   the mouseWindow, otherwise we get phantom mdowns!
				mfocus(&tptr -> event, tptr -> event.where, False, false);
				mode_globaltolocal(p);
				
				// IM-2013-10-08: [[ FullscreenMode ]] Update mouseloc with MCscreen getters & setters
				MCPoint t_mouseloc;
				t_mouseloc = MCPointMake(p.h, p.v);
				
				MCscreen->setmouseloc(MCmousestackptr, t_mouseloc);
				
				uint2 mbutton;
				MCclickstackptr = MCmousestackptr;
				MCclicklocx = MCmousex;
				MCclicklocy = MCmousey;
				mbutton = tptr->event.modifiers & controlKey ? 3 : 1;
				MCbuttonstate = 1 << (mbutton - 1);
				pressptr = tptr;
				tptr = (MCEventnode *)tptr->next();
				break;
			}
			tptr = (MCEventnode *)tptr->next();
		}
		while (tptr != pendingevents);
	Boolean value = False;
	if (tptr != pendingevents)
		do
		{
			if (tptr->event.what == mouseUp)
			{
				setmods(tptr->event.modifiers);
				MCbuttonstate = 0;
				releaseptr = tptr;
				break;
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
		real8 eventtime = exittime;
		donepending = handlepending(curtime, eventtime, dispatch);
		siguser();
		real8 waittime = donepending ? 0.0 : eventtime - curtime;
		
		MCModeQueueEvents();
		if (MCquit)
		{
			abort = True;
			break;
		}

		if (MCnsockets > 0)
			waittime = waittime < 0.025 ? waittime: 0.025;

		//calls handle() in macdcmac.cpp
		if (modalclosed ||
			(dispatch && MCEventQueueDispatch() ||
			handle(waittime, dispatch, anyevent, abort, reset) ||
			donepending) && anyevent ||
			abort)
			break;
		if (MCquit)
		{
			abort = True;
			break;
		}

		// MW-2012-09-19: [[ Bug 10218 ]] Make sure we update the screen in case
		//   any engine event handling methods need us to.
		MCRedrawUpdateScreen();

		curtime = MCS_time();
	}
	while (curtime < exittime);
	MCwaitdepth--;
	
	// MW-2012-09-19: [[ Bug 10218 ]] Make sure we update the screen in case
	//   any engine event handling methods need us to.
	MCRedrawUpdateScreen();

	return abort;
}


void MCScreenDC::flushevents(uint2 e)
{
	static int event_types[FE_LAST] = { 0, mouseDown, mouseUp,
	                                    keyDown, keyUp, autoKey, diskEvt,
	                                    activateEvt, kHighLevelEvent, osEvt };
	e = event_types[e];
	Boolean abort, reset;
	handle(0.0, False, False, abort, reset);
	Boolean done;
	if (pendingevents != NULL)
	{
		MCEventnode *tptr = pendingevents;
		do
		{
			done = True;
			handle(0.0, False, False, abort, reset);
			if (e == 0 || e == tptr->event.what)
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

uint1 MCScreenDC::fontnametocharset(MCStringRef p_fontname)
{
	// MW-2006-06-09: [[ Bug 3670 ]] Fixed length buffer can cause a crash
	char fname[256];
    char *t_fontname;
    /* UNCHECKED */ MCStringConvertToCString(p_fontname, t_fontname);
	strncpy(fname, (const char *)t_fontname, 255);
	fname[255] = '\0';
    delete t_fontname;
	
	char *sptr = fname;
	short ffamilyid;		    //font family ID
	StringPtr reqnamePascal = c2pstr(fname);
	GetFNum(reqnamePascal, &ffamilyid);
	return MCS_langidtocharset(FontToScript(ffamilyid));
}

// Dead code?
/*
char *MCScreenDC::charsettofontname(uint1 charset, const char *oldfontname)
{
	char *fname = new char[255];
	strcpy(fname, oldfontname);
	char *sptr = fname;
	if ((sptr = strchr(fname, ',')) != NULL)
		*sptr = '\0';
	char *tmpname = strclone(fname);//make a copy of the font name
	short ffamilyid;		    //font family ID
	StringPtr reqnamePascal = c2pstr(tmpname);
	GetFNum(reqnamePascal, &ffamilyid);
	delete tmpname;
	if (FontToScript(ffamilyid) != MCS_charsettolangid(charset))
	{
		GetFontName(GetScriptVariable(MCS_charsettolangid(charset),
		                              smScriptAppFond), (unsigned char *)fname);
		p2cstr((unsigned char *)fname);
	}
	return fname;
}
*/

void MCScreenDC::openIME()
{
	if (tsmdocument)
		return;
	InterfaceTypeList supportedTypes;
    supportedTypes[0] = kUnicodeDocument;
    
	NewTSMDocument(1, supportedTypes, &tsmdocument, NULL);
}

void MCScreenDC::activateIME(Boolean activate)
{
	if (tsmdocument)
	{
		if (activate)
		{
			ActivateTSMDocument(tsmdocument);
			UseInputWindow(tsmdocument, !MCinlineinput);
		}
		else
			DeactivateTSMDocument(tsmdocument);
	}
}

void MCScreenDC::clearIME(Window w)
{
	if (tsmdocument)
		FixTSMDocument(tsmdocument);
}

void MCScreenDC::closeIME()
{
	if (!tsmdocument)
		return;
	DeleteTSMDocument(tsmdocument);
	tsmdocument = 0;
}
