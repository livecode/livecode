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

#include "w32prefix.h"

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
#include "execpt.h"
#include "vclip.h"
#include "globals.h"
#include "core.h"
#include "notify.h"
#include "osspec.h"

#include "w32dc.h"
#include "eventqueue.h"
#include "mode.h"
#include "redraw.h"

#include "meta.h"

#include "resolution.h"

// Used to be in w32defs.h, but only used by MCScreenDC::boundrect.
#define WM_TITLE_HEIGHT 16

extern "C"
{
#define COMPILE_MULTIMON_STUBS  1
#include <multimon.h>
};

MCDisplay *MCScreenDC::s_monitor_displays = NULL;
uint4 MCScreenDC::s_monitor_count = 0;

static BOOL CALLBACK CountMonitorsCallback(HMONITOR p_monitor, HDC p_monitor_dc, LPRECT p_rect, LPARAM p_data)
{
	*(uint4 *)p_data += 1;
	return TRUE;
}

static BOOL CALLBACK DescribeMonitorsCallback(HMONITOR p_monitor, HDC p_monitor_dc, LPRECT p_rect, LPARAM p_data)
{
	MONITORINFO **t_info = (MONITORINFO **)p_data;
	(*t_info) -> cbSize = sizeof(MONITORINFO);
	if (GetMonitorInfo(p_monitor, *t_info))
		*t_info += 1;
	return TRUE;
}

bool MCScreenDC::device_getdisplays(bool p_effective, MCDisplay *&r_displays, uint32_t &r_count)
{
	if (s_monitor_count == 0)
	{
		bool error = false;

		uint4 t_monitor_count = 0;
		error = !EnumDisplayMonitors(NULL, NULL, CountMonitorsCallback, (LPARAM)&t_monitor_count);
		
		MONITORINFO *t_info = NULL;
		if (!error)
		  error = (t_info = new MONITORINFO[t_monitor_count]) == NULL;

		MONITORINFO *t_info_iterator = t_info;
		if (!error)
			error = !EnumDisplayMonitors(NULL, NULL, DescribeMonitorsCallback, (LPARAM)&t_info_iterator);

		if (!error)
			t_monitor_count = t_info_iterator - t_info;

		MCDisplay *t_monitor_displays = NULL;
		if (!error)
			error = (t_monitor_displays = new MCDisplay[t_monitor_count]) == NULL;

		if (!error)
			for(uint4 t_monitor = 0, t_current_index = 1; t_monitor < t_monitor_count; t_monitor += 1)
			{
				uint4 t_index;

				if (t_info[t_monitor] . dwFlags & MONITORINFOF_PRIMARY)
					t_index = 0;
				else
					t_index = t_current_index++;

				t_monitor_displays[t_index] . index = t_index;

				t_monitor_displays[t_index] . device_viewport . x = int2(t_info[t_monitor] . rcMonitor . left);
				t_monitor_displays[t_index] . device_viewport . y = int2(t_info[t_monitor] . rcMonitor . top);
				t_monitor_displays[t_index] . device_viewport . width = uint2(t_info[t_monitor] . rcMonitor . right - t_info[t_monitor] . rcMonitor . left);
				t_monitor_displays[t_index] . device_viewport . height = uint2(t_info[t_monitor] . rcMonitor . bottom - t_info[t_monitor] . rcMonitor . top);
				
				t_monitor_displays[t_index] . device_workarea . x = int2(t_info[t_monitor] . rcWork . left);
				t_monitor_displays[t_index] . device_workarea . y = int2(t_info[t_monitor] . rcWork . top);
				t_monitor_displays[t_index] . device_workarea . width = uint2(t_info[t_monitor] . rcWork . right - t_info[t_monitor] . rcMonitor . left);
				t_monitor_displays[t_index] . device_workarea . height = uint2(t_info[t_monitor] . rcWork . bottom - t_info[t_monitor] . rcMonitor . top);
			}

		delete[] t_info;

		if (error)
			delete[] t_monitor_displays;
		else
		{
			static Meta::static_ptr_t<MCDisplay> s_freeable_monitors;
			s_monitor_count = t_monitor_count;
			s_freeable_monitors = s_monitor_displays = t_monitor_displays;
		}
	}

	if (s_monitor_count == 0)
	{
		static MCDisplay t_monitor;
		RECT r;
		SystemParametersInfoA(SPI_GETWORKAREA, 0, &r, 0);
		MCU_set_rect(t_monitor . device_viewport, 0, 0, device_getwidth(), device_getheight());
		MCU_set_rect(t_monitor . device_workarea, 0, 0, uint2(r . right - r . left), uint2(r . bottom - r . top));
		r_displays = &t_monitor;
		r_count = 1;
		return true;
	}

	if (taskbarhidden)
		for(uint4 t_index = 0; t_index < s_monitor_count; t_index++)
			s_monitor_displays[t_index] . device_workarea = s_monitor_displays[t_index] . device_viewport;

	r_displays = s_monitor_displays;
	r_count = s_monitor_count;

	return true;
}

void MCScreenDC::device_boundrect(MCRectangle &rect, Boolean title, Window_mode m)
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

	srect = MCGRectangleGetIntegerBounds(MCResUserToDeviceRect(srect));

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
	while (PeekMessageA(&msg, NULL, WM_PAINT, WM_PAINT, PM_REMOVE))
		DispatchMessageA(&msg);
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

void MCScreenDC::device_querymouse(int2 &x, int2 &y)
{
	POINT p;
	if (GetCursorPos(&p))
	{
		x = (int2)p.x;
		y = (int2)p.y;
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

void MCScreenDC::device_setmouse(int2 x, int2 y)
{
	SetCursorPos(x, y);
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
				// IM-2013-08-08: [[ ResIndependence ]] scale device to userspace coords
				MCclicklocx = LOWORD(tptr->lParam) / MCResGetDeviceScale();
				MCclicklocy = HIWORD(tptr->lParam) / MCResGetDeviceScale();
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
		// MW-2009-08-26: Handle any pending notifications
		if (MCNotifyDispatch(dispatch == True) && anyevent)
			break;

		real8 eventtime = exittime;
		Boolean donepending = handlepending(curtime, eventtime, dispatch);
		real8 waittime = donepending ? 0.0 : eventtime - curtime;
		
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
uint1 MCScreenDC::fontnametocharset(const char *oldfontname)
{

	HDC hdc = f_src_dc;
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
	logfont.lfCharSet = DEFAULT_CHARSET;
	HFONT newfont = CreateFontIndirectA(&logfont);
	HFONT oldfont = (HFONT)SelectObject(hdc, newfont);
	uint1 charset = MCU_wincharsettocharset(GetTextCharset(hdc));
	SelectObject(hdc, oldfont);
	DeleteObject(newfont);
	return charset;
}

char *MCScreenDC::charsettofontname(uint1 charset, const char *oldfontname)
{

	HDC hdc = f_src_dc;
	char *fontname = new char[LF_FACESIZE];
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
