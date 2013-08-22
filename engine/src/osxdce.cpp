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

#include "osxprefix.h"

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
#include "execpt.h"
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

Boolean tripleclick = False;

MCDisplay *MCScreenDC::s_monitor_displays = NULL;
uint4 MCScreenDC::s_monitor_count = 0;

bool MCScreenDC::device_getdisplays(bool p_effective, MCDisplay *& p_displays, uint32_t &r_count)
{
	bool t_success;
	t_success = true;
	
	MCDisplay *t_displays;
	t_displays = nil;
	
	uint32_t t_display_count;
	t_display_count = 0;
	
	if (s_monitor_count != 0)
	{
		t_displays = s_monitor_displays;
		t_display_count = s_monitor_count;
	}
	else
	{
		for(GDHandle t_device = GetDeviceList(); t_device != NULL; t_device = GetNextDevice(t_device))
			if (TestDeviceAttribute(t_device, screenDevice) && TestDeviceAttribute(t_device, screenActive))
				t_display_count += 1;
			
		t_success = t_display_count != 0;
		
		if (t_success)
			t_success = MCMemoryNewArray(t_display_count, t_displays);
			
		if (t_success)
		{
			uint4 t_current_index = 1;
			for(GDHandle t_device = GetDeviceList(); t_device != NULL; t_device = GetNextDevice(t_device))
				if (TestDeviceAttribute(t_device, screenDevice) && TestDeviceAttribute(t_device, screenActive))
				{
					uint4 t_index;
					
					HLock((Handle)t_device);
					
					if (TestDeviceAttribute(t_device, mainScreen))
						t_index = 0;
					else
						t_index = t_current_index++;
						
					t_displays[t_index] . index = t_index;
						
					t_displays[t_index] . device_viewport . x = (*t_device) -> gdRect . left;
					t_displays[t_index] . device_viewport . y = (*t_device) -> gdRect . top;
					t_displays[t_index] . device_viewport . width = (*t_device) -> gdRect . right - (*t_device) -> gdRect . left;
					t_displays[t_index] . device_viewport . height = (*t_device) -> gdRect . bottom - (*t_device) -> gdRect . top;
				
					Rect t_workarea;
					GetAvailableWindowPositioningBounds(t_device, &t_workarea);
					t_displays[t_index] . device_workarea . x = t_workarea . left;
					t_displays[t_index] . device_workarea . y = t_workarea . top;
					t_displays[t_index] . device_workarea . width = t_workarea . right - t_workarea . left;
					t_displays[t_index] . device_workarea . height = t_workarea . bottom - t_workarea . top;
					
					HUnlock((Handle)t_device);
				}
		}
		
		if (t_success)
		{
			s_monitor_count = t_display_count;
			s_monitor_displays = t_displays;
		}
		else
			MCMemoryDeleteArray(t_displays);
	}
	
	if (!t_success)
	{
		static MCDisplay t_display;
		Rect t_workarea;
		
		MCU_set_rect(t_display . device_viewport, 0, 0, device_getwidth(), device_getheight());
		GetAvailableWindowPositioningBounds(GetMainDevice(), &t_workarea);
		t_display . index = 0;
		t_display . device_workarea . x = t_workarea . left;
		t_display . device_workarea . y = t_workarea . top;
		t_display . device_workarea . width = t_workarea . right - t_workarea . left;
		t_display . device_workarea . height = t_workarea . bottom - t_workarea . top;
		
		t_displays = &t_display;
		t_display_count = 1;
	}
	
	p_displays = t_displays;
	r_count = t_display_count;
	
	return true;
}

void MCScreenDC::device_boundrect(MCRectangle &rect, Boolean title, Window_mode mode)
{	
	MCRectangle srect;

	if (mode >= WM_MODAL)
	{
		const MCDisplay *t_display;
		t_display = getnearestdisplay(rect);
		srect = t_display -> device_workarea;
	}
	else
		srect = MCGRectangleGetIntegerInterior(MCResUserToDeviceRect(MCwbr));

	uint2 sr, sw, sb, sh;
	Rect screenRect;
	
	SetRect(&screenRect, srect . x, srect . y, srect . x + srect . width, srect . y + srect . height);

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

void MCScreenDC::device_querymouse(int2 &x, int2 &y)
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

void MCScreenDC::device_setmouse(int2 x, int2 y)
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
				MCmousex = p.h;
				MCmousey = p.v;
				if (MCmousestackptr != nil)
					MCmousey += MCmousestackptr -> getscroll();
				
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

uint1 MCScreenDC::fontnametocharset(const char *oldfontname)
{
	// MW-2006-06-09: [[ Bug 3670 ]] Fixed length buffer can cause a crash
	char fname[256];
	strncpy(fname, oldfontname, 255);
	fname[255] = '\0';
	
	char *sptr = fname;
	if ((sptr = strchr(fname, ',')) != NULL)
		*sptr = '\0';
	short ffamilyid;		    //font family ID
	StringPtr reqnamePascal = c2pstr(fname);
	GetFNum(reqnamePascal, &ffamilyid);
	return MCS_langidtocharset(FontToScript(ffamilyid));
}

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

void MCScreenDC::openIME()
{
	if (tsmdocument)
		return;
	InterfaceTypeList supportedTypes;
	if (MCS_imeisunicode())
		supportedTypes[0] = kUnicodeDocument;
	else
		supportedTypes[0] = kTextService;
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
