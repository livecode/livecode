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
#include "image.h"
#include "util.h"
#include "date.h"
#include "param.h"

#include "vclip.h"
#include "globals.h"
#include "notify.h"
#include "osspec.h"

#include "w32dc.h"
#include "eventqueue.h"
#include "mode.h"
#include "redraw.h"

#include "meta.h"

#include <strsafe.h>
#include "resolution.h"

// Used to be in w32defs.h, but only used by MCScreenDC::boundrect.
#define WM_TITLE_HEIGHT 16

extern "C"
{
#define COMPILE_MULTIMON_STUBS  1
#include <multimon.h>
};

////////////////////////////////////////////////////////////////////////////////

extern bool MCWin32GetMonitorPixelScale(HMONITOR p_monitor, MCGFloat &r_pixel_scale);

static inline MCRectangle MCRectangleFromWin32RECT(const RECT &p_rect)
{
	return MCRectangleMake(p_rect.left, p_rect.top, p_rect.right - p_rect.left, p_rect.bottom - p_rect.top);
}

static inline MCPoint MCPointFromWin32POINT(const POINT &p_point)
{
	return MCPointMake(p_point.x, p_point.y);
}

////////////////////////////////////////////////////////////////////////////////

typedef struct
{
	MCDisplay *displays;
	uint32_t display_count;
	uint32_t index;
} MCWin32GetDisplaysContext;

static BOOL CALLBACK CountMonitorsCallback(HMONITOR p_monitor, HDC p_monitor_dc, LPRECT p_rect, LPARAM p_data)
{
	*(uint32_t *)p_data += 1;
	return TRUE;
}

// IM-2014-01-28: [[ HiDPI ]] Set MCDisplay fields directly in monitor enumeration callback
static BOOL CALLBACK DescribeMonitorsCallback(HMONITOR p_monitor, HDC p_monitor_dc, LPRECT p_rect, LPARAM p_data)
{
	MCWin32GetDisplaysContext *t_context;
	t_context = (MCWin32GetDisplaysContext*)(p_data);

	MONITORINFO t_info;
	t_info.cbSize = sizeof(MONITORINFO);

	if (GetMonitorInfo(p_monitor, &t_info))
	{
		uint32_t t_index;
		if (t_info.dwFlags & MONITORINFOF_PRIMARY)
			t_index = 0;
		else
			t_index = t_context->index++;

		if (t_index <= t_context->display_count)
		{
			t_context->displays[t_index].index = t_index;

			// IM-2014-01-28: [[ HiDPI ]] Get the pixel scale for each display
			if (!MCWin32GetMonitorPixelScale(p_monitor, t_context->displays[t_index].pixel_scale))
				t_context->displays[t_index].pixel_scale = 1.0;

			MCRectangle t_viewport, t_workarea;
			t_viewport = MCRectangleFromWin32RECT(t_info.rcMonitor);
			t_workarea = MCRectangleFromWin32RECT(t_info.rcWork);

			// IM-2014-01-28: [[ HiDPI ]] Convert screen to logical coords
			t_context->displays[t_index].viewport = MCscreen->screentologicalrect(t_viewport);
			t_context->displays[t_index].workarea = MCscreen->screentologicalrect(t_workarea);
		}
	}

	return TRUE;
}

// IM-2014-01-28: [[ HiDPI ]] Refactored to handle display info caching in MCUIDC superclass
bool MCScreenDC::platform_getdisplays(bool p_effective, MCDisplay *&r_displays, uint32_t &r_count)
{
	bool t_success;
	t_success = true;

	MCDisplay *t_displays;
	t_displays = nil;

	uint32_t t_display_count;
	t_display_count = 0;

	t_success = EnumDisplayMonitors(NULL, NULL, CountMonitorsCallback, (LPARAM)&t_display_count);
	
	if (t_success)
		t_success = MCMemoryNewArray(t_display_count, t_displays);

	if (t_success)
	{
		// IM-2014-01-28: [[ HiDPI ]] Removed intermediary MONITORINFO array. Now using context info to fill MCDisplay array directly.
		MCWin32GetDisplaysContext t_context;
		t_context.displays = t_displays;
		t_context.display_count = t_display_count;
		t_context.index = 1;

		t_success = EnumDisplayMonitors(NULL, NULL, DescribeMonitorsCallback, (LPARAM)&t_context);
	}

	if (!t_success)
	{
		MCMemoryDeleteArray(t_displays);
		t_displays = nil;

		t_success = MCMemoryNew(t_displays);
		t_display_count = 1;

		if (t_success)
		{
			RECT r;
			SystemParametersInfoA(SPI_GETWORKAREA, 0, &r, 0);
			t_displays->index = 0;
			t_displays->pixel_scale = 1.0;
			t_displays->viewport = MCRectangleMake(0, 0, platform_getwidth(), platform_getheight());

			MCRectangle t_workarea;
			t_workarea = MCRectangleFromWin32RECT(r);

			// IM-2014-01-28: [[ HiDPI ]] Convert screen to logical coords
			t_displays->workarea = screentologicalrect(t_workarea);
		}
	}

	if (t_success)
	{
		if (taskbarhidden)
			for(uint32_t i = 0; i < t_display_count; i++)
				t_displays[i].workarea = t_displays[i].viewport;

		r_displays = t_displays;
		r_count = t_display_count;
	}
	else
		MCMemoryDeleteArray(t_displays);

	return t_success;
}

// IM-2014-01-29: [[ HiDPI ]] We receive notification of desktop changes on Windows, so can safely cache display info
bool MCScreenDC::platform_displayinfocacheable(void)
{
	return true;
}

////////////////////////////////////////////////////////////////////////////////

void MCScreenDC::platform_boundrect(MCRectangle &rect, Boolean title, Window_mode m, Boolean resizable)
{
	MCRectangle srect;

	if (m >= WM_MODAL)
	{
		const MCDisplay *t_display;
		t_display = getnearestdisplay(rect);
		srect = t_display -> workarea;
	}
	else
		srect = MCwbr;

	// IM-2014-01-28: [[ HiDPI ]] Don't scale to screen coords here
	/* CODE REMOVED */

	// TODO - This section needs to be revised as using a hardcoded titlebar size will give the wrong results

	if (rect.x < srect.x)
		rect.x = srect.x;
	else if (rect.x + rect.width > srect.x + srect.width)
		rect.x = MCU_max(srect.x, srect.x + srect.width - rect.width);
	int2 top = title ? WM_TITLE_HEIGHT : 0;
	srect.y += top;
	srect.height -= top;
	if (rect.y < srect.y)
		rect.y = srect.y;
	else if (rect.y + rect.height > srect.y + srect.height)
		rect.y = MCU_max(srect.y, srect.y + srect.height - rect.height);
}

void MCScreenDC::expose()
{
	MSG msg;
	while (PeekMessageW(&msg, NULL, WM_PAINT, WM_PAINT, PM_REMOVE))
		DispatchMessageW(&msg);
}

Boolean MCScreenDC::abortkey()
{
	if (MCabortscript)
		return True;
	if (!MCalarm)
		return False;
	MCalarm = False;
	MSG msg;
	Boolean abort = False;
	Boolean reset;
	while (PeekMessageA(&msg, NULL, WM_KEYDOWN, WM_KEYDOWN, PM_NOREMOVE))
		handle(0, False, True, abort, reset);
	if (pendingevents != NULL)
	{
		MCEventnode *tptr = pendingevents;
		do
		{
			// MW-2005-10-29: We should use tptr -> modifier here as opposed to direct
			//   GetAsyncKeyState, but unfortunately the stored modifierstate is also
			//   acquired directly, meaning it is (potentially) out of step with key
			//   presses
			if (tptr->msg == WM_KEYDOWN && (tptr->wParam == VK_CANCEL
			                                || (tptr->wParam == 190
			                                    && GetAsyncKeyState(VK_CONTROL)
			                                    & 0x8000)))
			{
				//_RPT0(_CRT_WARN, "Abort found in pending messages queue\n");
				abort = True;
				tptr->remove
				(pendingevents);
				delete tptr;
				if (pendingevents == NULL)
					break;
				tptr = pendingevents;
				continue;
			}
			tptr = (MCEventnode *)tptr->next();
		}
		while (tptr != pendingevents);
	}
	if (abort)
		if (MCallowinterrupts && !MCdefaultstackptr->cantabort())
		{
			//_RPT0(_CRT_WARN, "Abort caught as break\n");
			return True;
		}
		else
		{
			//_RPT0(_CRT_WARN, "Abort caught as interrupt\n");
			MCinterrupt = True;
		}
	return False;
}

void MCScreenDC::platform_querymouse(int2 &x, int2 &y)
{
	POINT p;
	if (GetCursorPos(&p))
	{
		MCPoint t_loc;
		t_loc = MCPointFromWin32POINT(p);

		// IM-2014-01-28: [[ HiDPI ]] Convert screen to logical coords
		t_loc = screentologicalpoint(t_loc);

		x = t_loc.x;
		y = t_loc.y;
	}
}

uint2 MCScreenDC::querymods()
{
	if (lockmods)
		return MCmodifierstate;
	uint2 state = 0;
	if (GetAsyncKeyState(VK_CONTROL) & 0x8000)
		state |= MS_CONTROL;
	if (GetAsyncKeyState(VK_MENU) & 0x8000)
		state |= MS_MOD1;
	if (GetAsyncKeyState(VK_SHIFT) & 0x8000)
		state |= MS_SHIFT;

	// MW-2006-03-20: Bug 3398 - the state of the caps-lock key should be the toggled state, not the *actual* state
	if (GetKeyState(VK_CAPITAL) & 0x0001)
		state |= MS_CAPS_LOCK;
	return state;
}

void MCScreenDC::platform_setmouse(int2 x, int2 y)
{
	MCPoint t_loc;
	t_loc = MCPointMake(x, y);

	// IM-2014-01-28: [[ HiDPI ]] Convert logical to screen coords
	t_loc = logicaltoscreenpoint(t_loc);

	SetCursorPos(t_loc.x, t_loc.y);
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
	switch (button)
	{
	case 1:
		if (GetSystemMetrics(SM_SWAPBUTTON))
			return (GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0;
		else
			return (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
	case 2:
		return (GetAsyncKeyState(VK_MBUTTON) & 0x8000) != 0;
	case 3:
		if (GetSystemMetrics(SM_SWAPBUTTON))
			return (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
		else
			return (GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0;
	default:
		return ((GetAsyncKeyState(VK_LBUTTON) | GetAsyncKeyState(VK_MBUTTON)
		         | GetAsyncKeyState(VK_RBUTTON)) & 0x8000) != 0;
	}
}

Boolean MCScreenDC::getmouseclick(uint2 button, Boolean& r_abort)
{
	if (!MCModeMakeLocalWindows())
	{
		r_abort = wait(0.0, False, True);
		if (r_abort)
			return False;
		return MCEventQueueGetMouseClick(button);
	}
	
	Boolean abort, reset;
	MSG msg;
	
	// MW-2008-03-19: [[ Bug 6124 ]] Make sure we reset the r_abort variable to
	//   false by default.
	r_abort = False;
	
	while (PeekMessageA(&msg, NULL, WM_MOUSEFIRST, WM_MOUSELAST, PM_NOREMOVE))
	{
		handle(0, False, True, abort, reset);
		if (abort)
		{
			r_abort = True;
			return False;
		}
	}
	MCEventnode *tptr = pendingevents;
	MCEventnode *pressptr = NULL;
	MCEventnode *releaseptr = NULL;
	uint2 b;
	if (pendingevents != NULL)
		do
		{
			if (button == 0
			        && (tptr->msg == WM_LBUTTONDOWN || tptr->msg == WM_MBUTTONDOWN
			            || tptr->msg == WM_RBUTTONDOWN || tptr->msg == WM_LBUTTONDBLCLK
			            || tptr->msg == WM_MBUTTONDBLCLK
			            || tptr->msg == WM_RBUTTONDBLCLK)
			        || (button == 1
			            && (tptr->msg == WM_LBUTTONDOWN||tptr->msg == WM_LBUTTONDBLCLK))
			        || (button == 2 && (tptr->msg == WM_MBUTTONDOWN
			                            || tptr->msg == WM_MBUTTONDBLCLK))
			        || (button == 3 && (tptr->msg == WM_RBUTTONDOWN
			                            || tptr->msg == WM_RBUTTONDBLCLK)))
			{
				switch (tptr->msg)
				{
				case WM_MBUTTONDOWN:
				case WM_MBUTTONDBLCLK:
					b = 2;
					break;
				case WM_RBUTTONDOWN:
				case WM_RBUTTONDBLCLK:
					b = 3;
					break;
				default:
					b = 1;
					break;
				}
				pressptr = tptr;
				MCbuttonstate |= 1L << (b - 1);
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
			if (button == 0
			        && (tptr->msg == WM_LBUTTONUP || tptr->msg == WM_MBUTTONUP
			            || tptr->msg == WM_RBUTTONUP)
			        || button == 1 && tptr->msg == WM_LBUTTONUP
			        || button == 2 && tptr->msg == WM_MBUTTONUP
			        || button == 3 && tptr->msg == WM_RBUTTONUP)
			{
				switch (tptr->msg)
				{
				case WM_MBUTTONUP:
					b = 2;
					break;
				case WM_RBUTTONUP:
					b = 3;
					break;
				default:
					b = 1;
					break;
				}
				MCbuttonstate &= ~(1L << (b - 1));

				MCPoint t_clickloc;
				t_clickloc = MCPointMake(LOWORD(tptr->lParam), HIWORD(tptr->lParam));

				// IM-2014-01-28: [[ HiDPI ]] Convert screen to logical coords
				t_clickloc = MCscreen->screentologicalpoint(t_clickloc);

				MCscreen->setclickloc(MCmousestackptr, t_clickloc);
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

void MCScreenDC::addmessage(MCObject *optr, MCNameRef name, real8 time, MCParameter *params)
{
	WaitForSingleObject(mutex, 1000);
	MCUIDC::addmessage(optr, name, time, params);
	ReleaseMutex(mutex);
	PostMessageA(invisiblehwnd, WM_USER, 0, 0);
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
	Boolean done = False;
	do
	{
		// IM-2014-03-06: [[ revBrowserCEF ]] Call additional runloop callbacks
		DoRunloopActions();

		// Always handle notifications first.

		// MW-2009-08-26: Handle any pending notifications
		if (MCNotifyDispatch(dispatch == True))
        {
            if (anyevent)
                break;
        }

		// MW-2014-04-16: [[ Bug 11690 ]] Work out the next pending message time.
		real8 t_pending_eventtime;
		if (m_messages.GetCount() == 0)
			t_pending_eventtime = exittime;
		else
			t_pending_eventtime = m_messages[0].m_time;

		// MW-2014-04-16: [[ Bug 11690 ]] Work out the next system event time.
		real8 t_system_eventtime;
		MSG t_msg;
		if (PeekMessageA(&t_msg, NULL, 0, 0, FALSE))
		{
			// MW-2014-04-16: [[ Bug 11690 ]] Event times are in ticks since system startup,
			//   but pending messages use real time. Thus we need to adjust for the difference,
			//   and take into account the potential wrapping of the system ticks.
			static real8 s_event_start = 0.0f;
			static DWORD s_event_start_ticks = 0xffffffffU;

			// If the start ticks > message time then tick counter has wrapped so re-sync.
			if (s_event_start_ticks > t_msg . time)
			{
				s_event_start = MCS_time();
				s_event_start_ticks = GetTickCount();
			}

			// Compute the system event time in real time.
			t_system_eventtime = s_event_start + (t_msg . time - s_event_start_ticks) / 1000.0;
		}
		else
			t_system_eventtime = DBL_MAX;

		// MW-2014-04-16: [[ Bug 11690 ]] If the pending event time is less than system eventime
		//   then dispatch a pending message. Otherwise don't do that, and move directly to
		//   dispatching a system message.
		Boolean donepending;
		real8 waittime;
		if (t_pending_eventtime < t_system_eventtime)
		{
			real8 eventtime = exittime;
			donepending = handlepending(curtime, eventtime, dispatch);
			waittime = donepending ? 0.0 : eventtime - curtime;
		}
		else
		{
			donepending = False;
			waittime = t_pending_eventtime - curtime;
		}
		
		// MW-2014-08-20: [[ Bug 12361 ]] If the waittime is negative then it gets coerced
		//   to a large positive value which causes waits to stall until an event occurs.
		if (waittime < 0.0)
			waittime = 0.0;

		MCModeQueueEvents();
		if (MCquit)
		{
			abort = True;
			break;
		}

		if ((dispatch && MCEventQueueDispatch() ||
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

    MCDeletedObjectsLeaveWait(dispatch);
    
	return abort;
}

void MCScreenDC::flushevents(uint2 e)
{
	static UINT event_types[FE_LAST] = { 0, WM_LBUTTONDOWN, WM_LBUTTONUP,
	                                     WM_KEYDOWN, WM_KEYUP, 0, 0,
	                                     WM_SETFOCUS, 0, 0 };
	Boolean abort, reset;
	MSG msg;
	while (PeekMessageA(&msg, NULL, 0, 0, PM_NOREMOVE))
		handle(0, False, True, abort, reset);
	if (pendingevents != NULL)
	{
		MCEventnode *tptr = pendingevents;
		Boolean done;
		do
		{
			done = True;
			if (e == FE_ALL || event_types[e] == tptr->msg
			        || e == FE_KEYDOWN && tptr->msg == WM_CHAR
			        || e == FE_MOUSEDOWN && (tptr->msg == WM_MBUTTONDOWN
			                                 || tptr->msg == WM_RBUTTONDOWN
			                                 || tptr->msg == WM_LBUTTONDBLCLK
			                                 || tptr->msg == WM_MBUTTONDBLCLK
			                                 || tptr->msg == WM_RBUTTONDBLCLK)
			        || e == FE_MOUSEUP && (tptr->msg == WM_MBUTTONUP
			                               || tptr->msg == WM_RBUTTONUP))
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

// MW-2006-03-21: Bug 3408 - fix memory leak due to unused memory allocation
// MW-2006-03-24: Bug 3408 - fix resource leak due to non-deletion of font
uint1 MCScreenDC::fontnametocharset(MCStringRef p_font)
{
	HDC hdc = f_src_dc;
	LOGFONTW logfont;
	memset(&logfont, 0, sizeof(LOGFONTW));

	MCAutoStringRefAsWString t_font_wstr;
	/* UNCHECKED */ t_font_wstr.Lock(p_font);

	/* UNCHECKED */ StringCchCopyW(logfont.lfFaceName, LF_FACESIZE, *t_font_wstr);

	//parse font and encoding
	logfont.lfCharSet = DEFAULT_CHARSET;
	HFONT newfont = CreateFontIndirectW(&logfont);
	HFONT oldfont = (HFONT)SelectObject(hdc, newfont);
	uint1 charset = MCU_wincharsettocharset(GetTextCharset(hdc));
	SelectObject(hdc, oldfont);
	DeleteObject(newfont);
	return charset;
}

/*
char *MCScreenDC::charsettofontname(uint1 charset, const char *oldfontname)
{

	HDC hdc = f_src_dc;
	char *fontname = new (nothrow) char[LF_FACESIZE];
	LOGFONTA logfont;
	memset(&logfont, 0, sizeof(LOGFONTA));
	uint4 maxlength = MCU_min(LF_FACESIZE - 1U, strlen(oldfontname));
	strncpy(logfont.lfFaceName, oldfontname, maxlength);
	logfont.lfFaceName[maxlength] = '\0';
	//parse font and encoding
	char *sptr = logfont.lfFaceName;
	if (sptr = strchr(logfont.lfFaceName, ','))
		*sptr = '\0';
	//parse font and encoding
	CHARSETINFO cs;
	if (charset)
	{
		char szLocaleData[6] ;
		GetLocaleInfoA(MAKELCID(MCS_charsettolangid(charset), SORT_DEFAULT),
		               LOCALE_IDEFAULTANSICODEPAGE, szLocaleData, 6);
		uint2 codepage = (uint2)strtoul(szLocaleData, NULL, 10);
		TranslateCharsetInfo((unsigned long *)codepage, &cs, TCI_SRCCODEPAGE);
		logfont.lfCharSet = cs.ciCharset;
	}
	else
		logfont.lfCharSet = DEFAULT_CHARSET;
	HFONT newfont = CreateFontIndirectA(&logfont);
	HFONT oldfont = (HFONT)SelectObject(hdc, newfont);
	memset(fontname, 0, LF_FACESIZE);
	GetTextFaceA(hdc, LF_FACESIZE, fontname);
	SelectObject(hdc, oldfont);
	return fontname;
}
*/

void MCScreenDC::openIME()
{}

void MCScreenDC::activateIME(Boolean activate)
{}

void MCScreenDC::clearIME(Window w)
{
	HIMC hIMC = ImmGetContext((HWND)w->handle.window);
	ImmNotifyIME(hIMC, NI_COMPOSITIONSTR, CPS_CANCEL, 0);
	ImmReleaseContext((HWND)w->handle.window,hIMC);
}

void MCScreenDC::closeIME()
{}
