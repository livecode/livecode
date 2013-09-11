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
#include "image.h"
#include "stack.h"
#include "card.h"
#include "field.h"
#include "stacklst.h"
#include "sellst.h"
#include "util.h"
#include "execpt.h"
#include "debug.h"
#include "param.h"
#include "osspec.h"
#include "redraw.h"
#include "region.h"

#include "mctheme.h"

#include "globals.h"

#include "w32dc.h"
#include "player.h"
#include "mode.h"
#include "socket.h"

#include "resolution.h"

#define VK_LAST 0xDE   //last is 222
#define LEAVE_CHECK_INTERVAL 500
#ifndef WM_MOUSEWHEEL
#define WM_MOUSEWHEEL (WM_MOUSELAST+1)
#endif

#ifndef WM_MOUSEHWHEEL
#define WM_MOUSEHWHEEL                  0x020E
#endif

#define MSH_MOUSEWHEEL "MSWHEEL_ROLLMSG"
static UINT mousewheel;

#ifndef WM_THEMECHANGED
#define WM_THEMECHANGED 0x031A /* winxp only */
#endif

static uint2 keysyms[] =
    {
        /* 0x00 */ 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
		/* 0x08 */ 0xFF08, 0xFF09, 0x0000, 0x0000, 0xFFBD, 0xFF0D, 0x0000, 0x0000,
		/* 0x10 */ 0xFFE1, 0xFFE3, 0xFFE9, 0xFF13, 0xFFE5, 0x0000, 0x0000, 0x0000,
		/* 0x18 */ 0x0000, 0x0000, 0x0000, 0xFF1B, 0x0000, 0x0000, 0x0000, 0x0000,
		/* 0x20 */ 0x0020, 0xFF55, 0xFF56, 0xFF57, 0xFF50, 0xFF51, 0xFF52, 0xFF53,
		/* 0x28 */ 0xFF54, 0xFF60, 0xFF61, 0xFF62, 0xFF61, 0xFF63, 0xFFFF, 0xFF6A,
		/* 0x30 */ 0x0030, 0x0031, 0x0032, 0x0033, 0x0034, 0x0035, 0x0036, 0x0037,
		/* 0x38 */ 0x0038, 0x0039, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
		/* 0x40 */ 0x0000, 0x0061, 0x0062, 0x0063, 0x0064, 0x0065, 0x0066, 0x0067,
		/* 0x48 */ 0x0068, 0x0069, 0x006A, 0x006B, 0x006C, 0x006D, 0x006E, 0x006F,
		/* 0x50 */ 0x0070, 0x0071, 0x0072, 0x0073, 0x0074, 0x0075, 0x0076, 0x0077,
		/* 0x58 */ 0x0078, 0x0079, 0x007A, 0xFF6C, 0xFF6D, 0xFF67, 0x0000, 0x0000,
		/* 0x60 */ 0xFFB0, 0xFFB1, 0xFFB2, 0xFFB3, 0xFFB4, 0xFFB5, 0xFFB6, 0xFFB7,
		/* 0x68 */ 0xFFB8, 0xFFB9, 0xFFAA, 0xFFAB, 0xFFAC, 0xFFAD, 0xFFAE, 0xFFAF,
		/* 0x70 */ 0xFFBE, 0xFFBF, 0xFFC0, 0xFFC1, 0xFFC2, 0xFFC3, 0xFFC4, 0xFFC5,
		/* 0x78 */ 0xFFC6, 0xFFC7, 0xFFC8, 0xFFC9, 0xFFCA, 0xFFCB, 0xFFCC, 0xFFCD,
		/* 0x80 */ 0xFFCE, 0xFFCF, 0xFFD0, 0xFFD1, 0xFFD2, 0xFFD3, 0xFFD4, 0xFFD5,
		/* 0x88 */ 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
		/* 0x90 */ 0xFF7F, 0xFF14, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
		/* 0x98 */ 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
		/* 0xA0 */ 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
		/* 0xA8 */ 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
		/* 0xB0 */ 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
		/* 0xB8 */ 0x0000, 0x0000, 0x003B, 0x003D, 0x002C, 0x002D, 0x002E, 0x002F,
		/* 0xC0 */ 0x0060, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
		/* 0xC8 */ 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 
		/* 0xD0 */ 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
		/* 0xD8 */ 0x0000, 0x0000, 0x0000, 0x005B, 0x005C, 0x005D, 0x0027
    };

static uint2 shift_keysyms[] =
    {
        /* 0x00 */ 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
		/* 0x08 */ 0xFF08, 0xFF09, 0x0000, 0x0000, 0xFFB5, 0xFF0D, 0x0000, 0x0000,
		/* 0x10 */ 0xFFE1, 0xFFE3, 0xFFE9, 0xFF6b, 0xFFE5, 0x0000, 0x0000, 0x0000,
		/* 0x18 */ 0x0000, 0x0000, 0x0000, 0xFF1B, 0x0000, 0x0000, 0x0000, 0x0000,
		/* 0x20 */ 0x0020, 0xFF55, 0xFF56, 0xFF57, 0xFF50, 0xFF51, 0xFF52, 0xFF53,
		/* 0x28 */ 0xFF54, 0xFF60, 0xFF61, 0xFF62, 0xFF61, 0xFF63, 0xFFFF, 0xFF6A,
		/* 0x30 */ 0x0029, 0x0021, 0x0040, 0x0023, 0x0024, 0x0025, 0x005E, 0x0026,
		/* 0x38 */ 0x002A, 0x0028, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
		/* 0x40 */ 0x0000, 0x0041, 0x0042, 0x0043, 0x0044, 0x0045, 0x0046, 0x0047,
		/* 0x48 */ 0x0048, 0x0049, 0x004A, 0x004B, 0x004C, 0x004D, 0x004E, 0x004F,
		/* 0x50 */ 0x0050, 0x0051, 0x0052, 0x0053, 0x0054, 0x0055, 0x0056, 0x0057,
		/* 0x58 */ 0x0058, 0x0059, 0x005A, 0xFF6C, 0xFF6D, 0xFF67, 0x0000, 0x0000,
		/* 0x60 */ 0xFF9E, 0xFF9C, 0xFF99, 0xFF9B, 0xFF96, 0xFF9D, 0xFF98, 0xFF95,
		/* 0x68 */ 0xFF97, 0xFF9A, 0xFFAA, 0xFFAB, 0xFFAC, 0xFFAD, 0xFFAE, 0xFFAF,
		/* 0x70 */ 0xFFBE, 0xFFBF, 0xFFC0, 0xFFC1, 0xFFC2, 0xFFC3, 0xFFC4, 0xFFC5,
		/* 0x78 */ 0xFFC6, 0xFFC7, 0xFFC8, 0xFFC9, 0xFFCA, 0xFFCB, 0xFFCC, 0xFFCD,
		/* 0x80 */ 0xFFCE, 0xFFCF, 0xFFD0, 0xFFD1, 0xFFD2, 0xFFD3, 0xFFD4, 0xFFD5,
		/* 0x88 */ 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
		/* 0x90 */ 0xFF7F, 0xFF20, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
		/* 0x98 */ 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
		/* 0xA0 */ 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
		/* 0xA8 */ 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
		/* 0xB0 */ 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
		/* 0xB8 */ 0x0000, 0x0000, 0x003A, 0x002B, 0x003C, 0x005F, 0x003E, 0x003F,
		/* 0xC0 */ 0x007E, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
		/* 0xC8 */ 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
		/* 0xD0 */ 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
		/* 0xD8 */ 0x0000, 0x0000, 0x0000, 0x007B, 0x007C, 0x007D, 0x0022
    };

static bool build_pick_string(MCExecPoint& p_ep, HMENU p_menu, UINT32 p_command);

void MCScreenDC::appendevent(MCEventnode *tptr)
{
	tptr->appendto(pendingevents);
}

void CALLBACK mouseproc(UINT id, UINT msg, DWORD user, DWORD dw1, DWORD dw2)
{
	MCScreenDC *pms = (MCScreenDC *)MCscreen;
	pms->setmousetimer(0);
	PostMessageA(pms->getinvisiblewindow(), WM_APP, 0, 0);
}

typedef struct
{
	Boolean dispatch, abort, reset, handled, live;
	KeySym keysym;
}
stateinfo;
static stateinfo *curinfo;

void MCScreenDC::setmods()
{
	if (!curinfo->live)
		return;
	MCmodifierstate = querymods();
}

enum ks {
    KS_UNDEFINED,
    KS_LCTRL,
    KS_RALT,
    KS_ALTGR
};

KeySym MCScreenDC::getkeysym(WPARAM wParam, LPARAM lParam)
{
	static int curks;
	if (curks == KS_UNDEFINED && lParam == 0x001d0001)
		curks = KS_LCTRL;
	else
		if (curks == KS_LCTRL && lParam == 0x21380001)
			curks = KS_RALT;
		else
			if (curks == KS_RALT || curks == KS_ALTGR)
				curks = KS_ALTGR;
			else
				curks = KS_UNDEFINED;
	uint2 keysym;
	setmods();
	if (curks == KS_ALTGR)
		if (MCmodifierstate & MS_CONTROL && MCmodifierstate & MS_MOD1)
			MCmodifierstate &= ~(MS_CONTROL| MS_MOD1);
		else
			curks = KS_UNDEFINED;
	if (wParam <= VK_LAST)
		if (MCmodifierstate & MS_SHIFT || GetKeyState(VK_CAPITAL) & 0x1)
			keysym = shift_keysyms[wParam];
		else
			keysym = keysyms[wParam];
	else
		keysym = wParam;
	if (keysym == 0xFF0D && lParam & 0x01000000)
		keysym = 0xFF8D;
	return keysym;
}

void MCScreenDC::getkeysdown(MCExecPoint &ep)
{
	BYTE keystates[256];
	ep.clear();
	if (GetKeyboardState((PBYTE)&keystates) == 0) // should use an async function?
		return;
	MCmodifierstate = querymods();
	bool first = true;
	KeySym ksym;
	uint2 i;
	for (i = 0; i < 256; i++)
	{
		if (keystates[i] & 0x80)
		{
			ksym = i;
			if (i <= VK_LAST)
			{
				if (MCmodifierstate & MS_SHIFT || GetKeyState(VK_CAPITAL) & 0x1)
					ksym = shift_keysyms[i];
				else
					ksym = keysyms[i];
			}
			if (ksym > 0)
			{
				ep.concatuint(ksym, EC_COMMA, first);
				first = false;
			}
		}
	}
}

extern HANDLE g_notify_wakeup;
Boolean MCScreenDC::handle(real8 sleep, Boolean dispatch, Boolean anyevent,
                           Boolean &abort, Boolean &reset)
{
	MSG msg, tmsg;
	stateinfo oldinfo = *curinfo;
	curinfo->abort = curinfo->reset = False;
	curinfo->dispatch = dispatch;
	curinfo->handled = False;
	curinfo->keysym = 0;
	curinfo->live = True;
	if (mousewheel == 0)
		mousewheel = RegisterWindowMessageA(MSH_MOUSEWHEEL);
	if (dispatch && pendingevents != NULL
	        || PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE)
	        || sleep != 0
	        && MsgWaitForMultipleObjects(1, &g_notify_wakeup, False,
	                                     (DWORD)(sleep * 1000.0), QS_ALLINPUT)
	        != WAIT_TIMEOUT && PeekMessageA(&msg, NULL, 0, 0, PM_REMOVE))
	{
		if (dispatch && pendingevents != NULL)
		{
			curinfo->live = False;
			MCEventnode *tptr = (MCEventnode *)pendingevents->remove
			                    (pendingevents);
			MCmodifierstate = tptr->modifier;
			MCeventtime = tptr->time;
			curinfo->keysym = tptr->keysym;
			MCWindowProc(tptr->hwnd, tptr->msg, tptr->wParam, tptr->lParam);
			delete tptr;
		}
		else
		{
			Boolean dodispatch = True;
			if (dodispatch)
			{
				curinfo->live = True;
				MCeventtime = msg.time;
				if (msg.message == WM_KEYDOWN || msg.message == WM_SYSKEYDOWN
				        || msg.message == WM_KEYUP || msg.message == WM_SYSKEYUP)
					curinfo->keysym = getkeysym(msg.wParam, msg.lParam);
				TranslateMessage(&msg);

				bool t_char_found;
				if ((MCruntimebehaviour & RTB_ACCURATE_UNICODE_INPUT) != 0)
					t_char_found = PeekMessageW(&tmsg, NULL, WM_CHAR, WM_CHAR, PM_REMOVE) ||
				                 PeekMessageW(&tmsg, NULL, WM_SYSCHAR, WM_SYSCHAR, PM_REMOVE);
				else
					t_char_found = PeekMessageA(&tmsg, NULL, WM_CHAR, WM_CHAR, PM_REMOVE) ||
				                 PeekMessageA(&tmsg, NULL, WM_SYSCHAR, WM_SYSCHAR, PM_REMOVE);
				if (t_char_found)
				{
					_Drawable _dw;
					Drawable dw = &_dw;
					dw->type = DC_WINDOW;
					dw->handle.window = (MCSysWindowHandle)msg.hwnd;
					if (MCdispatcher->findstackd(dw) == NULL)
					{
						if ((MCruntimebehaviour & RTB_ACCURATE_UNICODE_INPUT) != 0)
							DispatchMessageW(&msg);
						else
							DispatchMessageA(&msg);
					}
					memcpy(&msg, &tmsg, sizeof(MSG));
				}
				if ((MCruntimebehaviour & RTB_ACCURATE_UNICODE_INPUT) != 0)
					DispatchMessageW(&msg);
				else
					DispatchMessageA(&msg);
			}
		}
	}
	if (MCrecording)
		MCtemplateplayer->handlerecord();

	abort = curinfo->abort;
	reset = curinfo->reset;
	Boolean handled = curinfo->handled;
	*curinfo = oldinfo;
	return handled;
}

static stateinfo dummycurinfo;
static HWND capturehwnd;
static uint2 lastdown;
static Boolean doubledown;
static char lastchar;
static WPARAM lastwParam;
static KeySym lastkeysym;
static Boolean doubleclick;
Boolean tripleclick;
static uint4 clicktime;
static Boolean dragclick;

Boolean MCScreenDC::istripleclick()
{
	return tripleclick;
}

void MCScreenDC::restackwindows(HWND p_window, UINT p_message, WPARAM p_wparam, LPARAM p_lparam)
{
	WINDOWPOS *t_info;
	MCStack *t_stack;
	_Drawable t_drawable;
	t_drawable . handle . window = (MCSysWindowHandle)p_window;
	t_drawable . type = DC_WINDOW;

	t_info = (WINDOWPOS *)p_lparam;
	t_stack = MCdispatcher -> findstackd(&t_drawable);

	if (t_stack != NULL && t_stack -> getflag(F_DECORATIONS) && (t_stack -> getdecorations() & WD_UTILITY) != 0)
		return;

	if ((t_info -> flags & SWP_NOZORDER) != 0 && (t_info -> flags & SWP_NOACTIVATE) != 0)
		return;

	// MW-2007-02-11: [[ Bug 3794 ]] Background windows comes to front unexpectedly
	if ((t_info -> flags & SWP_HIDEWINDOW) != 0)
		return;

	HWND t_after;
	if ((t_info -> flags & SWP_NOZORDER) != 0)
		t_after = HWND_TOP;
	else
		t_after = t_info -> hwndInsertAfter;

	if ((t_info -> flags & SWP_NOACTIVATE) == 0)
	{
		if (t_stack != NULL)
			MCstacks -> top(t_stack);
		t_after = HWND_TOP;
	}

	MCStacknode *t_node;
	t_node = MCstacks -> topnode();
	if (t_node != NULL)
	{
		do
		{
			if (t_node -> getstack() == t_stack)
				break;

			Window t_current;
			if (!t_node -> getstack() -> isiconic())
				t_current = t_node -> getstack() -> getwindow();
			else
				t_current = NULL;

			if (t_current != NULL && IsWindowVisible((HWND)t_current -> handle . window) && (GetWindowLongA((HWND)t_current -> handle . window, GWL_EXSTYLE) & WS_EX_TOPMOST) == 0)
			{
				if ((t_info -> flags & SWP_NOACTIVATE) == 0)
					SetWindowPos((HWND)t_current -> handle . window, t_after, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE | SWP_NOSENDCHANGING);
	
				t_after = (HWND)t_current -> handle . window;
			}

			t_node = t_node -> next();
		}
		while(t_node != MCstacks -> topnode());
	}

	if (p_window != backdrop_window && (backdrop_hard || backdrop_active || MCraisewindows))
	{
		if (t_node != NULL && (t_node != MCstacks -> topnode() || t_stack == t_node -> getstack()))
			do
			{
				Window t_current;

				if (!t_node -> getstack() -> isiconic())
					t_current = t_node -> getstack() -> getwindow();
				else
					t_current = NULL;

				if (t_current != NULL && IsWindowVisible((HWND)t_current -> handle . window) && (GetWindowLongA((HWND)t_current -> handle . window, GWL_EXSTYLE) & WS_EX_TOPMOST) == 0)
				{
					UINT t_flags;
					if (t_node -> getstack() == t_stack)
						t_flags = t_info -> flags & SWP_NOACTIVATE;
					else
						t_flags = SWP_NOACTIVATE;

					SetWindowPos((HWND)t_current -> handle . window, t_after, 0, 0, 0, 0, t_flags | SWP_NOMOVE | SWP_NOSIZE | SWP_NOSENDCHANGING);
					t_after = (HWND)t_current -> handle . window;
				}

				t_node = t_node -> next();
			}
			while(t_node != MCstacks -> topnode());

		if (IsWindowVisible(backdrop_window))
			SetWindowPos(backdrop_window, t_after, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE | SWP_NOSENDCHANGING | SWP_NOOWNERZORDER);

		t_info -> flags |= SWP_NOACTIVATE | SWP_NOZORDER;
	}
	else
	{
		SetWindowPos(t_info -> hwnd, t_after, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOSENDCHANGING | SWP_NOOWNERZORDER);

		t_info -> flags |= SWP_NOZORDER;
	}

	return;
}

void MCScreenDC::hidebackdrop(Boolean p_hide)
{
	if (!MChidebackdrop)
		return;

	if (!backdrop_active && !backdrop_hard)
		return;

	if (p_hide && IsWindowVisible(backdrop_window))
		ShowWindow(backdrop_window, SW_HIDE);
	else if (!p_hide && !IsWindowVisible(backdrop_window))
		SetWindowPos(backdrop_window, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
}

LRESULT CALLBACK MCPlayerWindowProc(HWND hwnd, UINT msg, WPARAM wParam,
                                    LPARAM lParam)
{
	return DefWindowProcA(hwnd, msg, wParam, lParam);
}

#if defined(MCSSL) && defined(_WINDOWS)
extern void SeedSSL(UINT iMesg, WPARAM wParam, LPARAM lParam);
#endif

#include <crtdbg.h>

LRESULT CALLBACK MCWindowProc(HWND hwnd, UINT msg, WPARAM wParam,
                              LPARAM lParam)
{
	static Boolean isactive = True;
	KeySym keysym;
	MCStack *target;
	_Drawable _dw;
	Drawable dw = &_dw;
	dw->type = DC_WINDOW;
	dw->handle.window = (MCSysWindowHandle)hwnd;
	MCScreenDC *pms = (MCScreenDC *)MCscreen;
	MCStack *omousestack = MCmousestackptr;
	uint2 button;
	Boolean down;
	char buffer[XLOOKUPSTRING_SIZE];

	// IM-2013-08-08: [[ ResIndependence ]] scale mouse position from device to user space
	int32_t t_mx, t_my;
	t_mx = LOWORD(lParam) / MCResGetDeviceScale();
	t_my = HIWORD(lParam) / MCResGetDeviceScale();

	// MW-2005-02-20: Seed the SSL random number generator
#ifdef MCSSL
	SeedSSL(msg, wParam, lParam);
#endif

	if (curinfo == NULL)
		curinfo = &dummycurinfo;

	switch (msg)
	{
#ifdef FEATURE_RELAUNCH_SUPPORT
		case WM_COPYDATA:
		{
			LRESULT t_result;
			COPYDATASTRUCT *t_data;
			t_data = (COPYDATASTRUCT *)lParam;

			t_result = 0;
			if (t_data -> dwData == CWM_RELAUNCH)
			{
				MCresult -> clear();

				MCParameter *t_first_parameter = NULL;
				MCParameter *t_current_parameter = NULL;

				extern char *strndup(const char *, unsigned int);
				char *t_command_line = strndup((char *)t_data -> lpData, t_data -> cbData);

				char *sptr = t_command_line;
				while(*sptr)
				{
					if (*sptr == '\\')
						*sptr = '/';
					sptr++;
				}
				sptr = t_command_line;
				while(*sptr)
				{
					char *t_argument;
					int t_argument_length;

					while (isspace(*sptr))
						sptr++;

					t_argument_length = 0;
					if (*sptr == '"')
					{
						t_argument = ++sptr;
						while (*sptr && *sptr != '"')
						{
							sptr++;
							t_argument_length += 1;
						}
					}
					else
					{
						t_argument = sptr;
						while(*sptr && !isspace(*sptr))
						{
							sptr++;
							t_argument_length += 1;
						}
					}

					if (t_argument_length != 0)
					{
						MCParameter *t_parameter;
						MCString t_param;
						t_param . set(t_argument, t_argument_length);
						t_parameter = new MCParameter(t_param);
						if (t_first_parameter == NULL)
							t_first_parameter = t_parameter;
						else
							t_current_parameter -> setnext(t_parameter);
						t_current_parameter = t_parameter;
					}

					if (*sptr)
						sptr++;
				}

				if (MCdispatcher -> gethome() -> message(MCM_relaunch, t_first_parameter, False, True) != ES_NORMAL)
					t_result = 0;
				else
				{
					MCExecPoint t_ep;
					MCresult -> fetch(t_ep);
					
					if (t_ep . getsvalue() == "background")
						t_result = (LRESULT)HWND_BOTTOM;
					else
						t_result = MCdefaultstackptr -> getwindow() == NULL ? (LRESULT)HWND_BOTTOM : (LRESULT)(MCdefaultstackptr -> getwindow() -> handle . window);
				}

				while(t_first_parameter != NULL)
				{
					MCParameter *t_next;
					t_next = t_first_parameter -> getnext();
					delete t_first_parameter;
					t_first_parameter = t_next;
				}

				free(t_command_line);

				MCresult -> clear();
			}

			return t_result;
		}
#endif
	case CWM_TASKBAR_NOTIFICATION:
		((MCScreenDC *)MCscreen) -> processtaskbarnotify(hwnd, wParam, lParam);
		break;

	case WM_DISPLAYCHANGE:
	case WM_SETTINGCHANGE:
	{
		if (hwnd != ((MCScreenDC *)MCscreen) -> getinvisiblewindow())
			break;

		((MCScreenDC *)MCscreen) -> processdesktopchanged(true);
	}
	break;
	case WM_PALETTECHANGED:
		dw->handle.window = (MCSysWindowHandle)wParam;
		if (MCdispatcher->findstackd(dw) == NULL)
		{
			dw->handle.window = (MCSysWindowHandle)hwnd;
			MCStack *sptr = MCdispatcher->findstackd(dw);
			if (sptr != NULL)
				sptr->dirtyall();
			}
		break;
	case WM_PAINT:
		{
			MCStack *t_stack;
			t_stack = MCdispatcher -> findstackd(dw);
			if (t_stack != nil)
				t_stack -> onpaint();
		}
		break;
	case WM_SETFOCUS: //FocusIn
		if (curinfo->dispatch)
		{
			if (MCtracewindow == DNULL || hwnd != (HWND)MCtracewindow->handle.window)
			{
				dw->handle.window = (MCSysWindowHandle)GetFocus();
				MCdispatcher->wkfocus(dw);
			}
			curinfo->handled = True;
		}
		else
		{
			MCEventnode *tptr = new MCEventnode(hwnd, msg, wParam, lParam, 0,
			                                    MCmodifierstate, MCeventtime);
			pms->appendevent(tptr);
		}
		break;
	case WM_KILLFOCUS: //FocusOut:
		if (curinfo->dispatch)
		{
			if (MCtracewindow == DNULL || hwnd != (HWND)MCtracewindow->handle.window)
				MCdispatcher->wkunfocus(dw);
			curinfo->handled = True;
		}
		else
		{
			MCEventnode *tptr = new MCEventnode(hwnd, msg, wParam, lParam, 0,
			                                    MCmodifierstate, MCeventtime);
			pms->appendevent(tptr);
		}
		break;

	case WM_SYSKEYDOWN:
	case WM_SYSCHAR:
	case WM_CHAR:
	case WM_KEYDOWN:
	{
		if (wParam == VK_CONTROL)
			break;

		char t_input_char;
		t_input_char = (char)wParam;

		if (IsWindowUnicode(hwnd))
		{
			if (wParam >= 128)
			{
				bool t_is_unicode;
				WCHAR t_wide[1];
			
				// MW-2012-07-25: [[ Bug 9200 ]] Make sure we roundtrip the input character
				//   through 1252 *not* the active code page (which could be anything).
				t_wide[0] = (WCHAR)wParam;
				t_is_unicode = (WideCharToMultiByte(1252, 0, t_wide, 1, &t_input_char, 1, NULL, NULL) == 0);
				if (!t_is_unicode)
				{
					WCHAR t_reverse_wide[1];
					t_is_unicode = MultiByteToWideChar(1252, 0, &t_input_char, 1, t_reverse_wide, 1) == 0;
					if (!t_is_unicode)
						t_is_unicode = t_reverse_wide[0] != t_wide[0];
				}

				if (t_is_unicode && (msg == WM_CHAR || msg == WM_SYSCHAR))
				{
					if (MCactivefield)
					{
						MCString t_unicode_string;
						t_unicode_string . set((char *)t_wide, 2);

						// MW-2012-02-03: [[ Unicode Block ]] Use the new finsert method to insert
						//   text in unicode mode.
						MCactivefield -> finsertnew(FT_IMEINSERT, t_unicode_string, LCH_UNICODE, true);
					}
					break;
				}
			}
		}
		else if (wParam >= 128 && (((MCScreenDC *)MCscreen) -> system_codepage) != (((MCScreenDC *)MCscreen) -> input_codepage))
		{
			WCHAR t_unicode_char;
			MultiByteToWideChar((((MCScreenDC *)MCscreen) -> input_codepage), 0, &t_input_char, 1, &t_unicode_char, 1);

			bool t_is_unicode;
			t_is_unicode = (WideCharToMultiByte((((MCScreenDC *)MCscreen) -> system_codepage), 0, &t_unicode_char, 1, &t_input_char, 1, NULL, NULL) == 0);
			if (!t_is_unicode)
			{
				WCHAR t_reverse_unicode_char;
				t_is_unicode = MultiByteToWideChar((((MCScreenDC *)MCscreen) -> system_codepage), 0, &t_input_char, 1, &t_reverse_unicode_char, 1) == 0;
				if (!t_is_unicode)
					t_is_unicode = t_reverse_unicode_char != t_unicode_char;
			}

			if (t_is_unicode)
			{
				if (MCactivefield)
				{
					MCString t_unicode_string;
					t_unicode_string . set((char *)&t_unicode_char, 2);

					// MW-2012-02-03: [[ Unicode Block ]] Use the new finsert method to insert
					//   text in unicode mode.
					MCactivefield -> finsertnew(FT_IMEINSERT, t_unicode_string, LCH_UNICODE, true);
				}
				break;
			}
		}

		if (msg == WM_CHAR || msg == WM_SYSCHAR)
			wParam = t_input_char;

		buffer[0] = buffer[1] = 0;

		if (msg == WM_CHAR || msg == WM_SYSCHAR)
			buffer[0] = lastchar = wParam;

		// MW-2010-11-17: [[ Bug 3892 ]] Ctrl+Alt can be the same as AltGr.
		//   If we are a CHAR message *and* have a non-control character *and* have Ctrl+Alt set, we discard the modifiers
		if ((msg == WM_CHAR || msg == WM_SYSCHAR) && wParam >= 32 && (MCmodifierstate & (MS_CONTROL | MS_ALT)) == (MS_CONTROL | MS_ALT))
			MCmodifierstate = 0;

		if (curinfo->keysym == 0) // event came from some other dispatch
			keysym = pms->getkeysym(wParam, lParam);
		else
			keysym = curinfo->keysym;
		lastkeysym = keysym;
		if (MCmodifierstate & MS_CONTROL)
			if (wParam == VK_CANCEL || keysym == '.')
			{
				if (MCallowinterrupts && !MCdefaultstackptr->cantabort())
					curinfo->abort = True;
				else
					MCinterrupt = True;
			}
			else
				if (msg == WM_KEYDOWN)
					buffer[0] = lastchar = wParam;
		if (curinfo->dispatch)
		{
			if (MCtracewindow == DNULL || hwnd != (HWND)MCtracewindow->handle.window)
			{
				uint2 count = LOWORD(lParam);
				while (count--)
				{
					if (!MCdispatcher->wkdown(dw, buffer, keysym)
					        && (msg == WM_SYSKEYDOWN || msg == WM_SYSCHAR))
						return IsWindowUnicode(hwnd) ? DefWindowProcW(hwnd, msg, wParam, lParam) : DefWindowProcA(hwnd, msg, wParam, lParam);
					if (count || lParam & 0x40000000)
						MCdispatcher->wkup(dw, buffer, keysym);
				}
				curinfo->handled = curinfo->reset = True;
			}
		}
		else
		{
			MCEventnode *tptr = new MCEventnode(hwnd, msg, wParam, lParam, keysym,
			                                    MCmodifierstate, MCeventtime);
			pms->appendevent(tptr);
		}
	}
	break;
	case WM_KEYUP:
	case WM_SYSKEYUP:
	{	
		if (curinfo->keysym == 0) // event came from some other dispatch
			keysym = pms->getkeysym(wParam, lParam);
		else
			keysym = curinfo->keysym;
		if (keysym == lastkeysym)
			buffer[0] = lastchar;
		else
			buffer[0] = 0;
		buffer[1] = 0;
		if (curinfo->dispatch)
		{
			if (MCtracewindow == DNULL || hwnd != (HWND)MCtracewindow->handle.window)
			{
				MCeventtime = GetMessageTime(); //krevent->time;
				MCdispatcher->wkup(dw, buffer, keysym);
				curinfo->handled = curinfo->reset = True;
			}
		}
		else
		{
			MCEventnode *tptr = new MCEventnode(hwnd, msg, wParam, lParam, 0,
			                                    MCmodifierstate, MCeventtime);
			pms->appendevent(tptr);
		}
	}	
	break;
	case WM_IME_STARTCOMPOSITION:
		if (!MCinlineinput)
			return IsWindowUnicode(hwnd) ? DefWindowProcW(hwnd, msg, wParam, lParam) : DefWindowProcA(hwnd, msg, wParam, lParam);
		break;
	case WM_IME_ENDCOMPOSITION:
		if (!MCinlineinput)
			return IsWindowUnicode(hwnd) ? DefWindowProcW(hwnd, msg, wParam, lParam) : DefWindowProcA(hwnd, msg, wParam, lParam);
		break;
	case WM_IME_CHAR:
		{
			if (!MCactivefield)
				break;
			uint2 unicodekey = MAXUINT2;
			uint4 destlen;
			if (IsWindowUnicode(hwnd))
			{
				unicodekey = wParam;
				destlen = 2;
			}
			else
			{
				char multibytechar[3];
				multibytechar[0] =  HIBYTE((WORD)wParam) ;
				multibytechar[1] =  LOBYTE((WORD)wParam) ;
				multibytechar[2] = '\0';
				MCU_multibytetounicode(multibytechar, 2, (char *)&unicodekey, 2,
					                     destlen,  MCS_langidtocharset(LOWORD(GetKeyboardLayout(0))));
			}
			MCString unicodestr;
			unicodestr.set((char *)&unicodekey, destlen);

			// MW-2012-02-03: [[ Unicode Block ]] Use the new finsert method to insert
			//   text in unicode mode.
			MCactivefield->finsertnew(FT_IMEINSERT, unicodestr, 0, true);
		}
		break;
	case WM_IME_COMPOSITION:
		{
			if (!MCinlineinput)
				return IsWindowUnicode(hwnd) ? DefWindowProcW(hwnd, msg, wParam, lParam) : DefWindowProcA(hwnd, msg, wParam, lParam);
			if (!MCactivefield)
				break;
			DWORD dwindex = 0;
			if (lParam & GCS_RESULTSTR)
			{
				MCactivefield->stopcomposition(True,False);
				dwindex = GCS_RESULTSTR;
			}
			else if (lParam & GCS_COMPSTR)
			{
				MCactivefield->startcomposition();
				dwindex = GCS_COMPSTR;
			}
			HIMC hIMC = ImmGetContext(hwnd);
			if (!hIMC || !dwindex)
				break;
			int2 cursorpos = LOWORD(ImmGetCompositionStringA(hIMC, GCS_CURSORPOS,
			                        NULL, 0));
			MCactivefield->setcompositioncursoroffset(cursorpos << 1);
			uint2 compstrsize = 0;
			char *compstring = NULL;
			compstrsize = (uint2)ImmGetCompositionStringW(hIMC, dwindex, NULL, 0);
			compstring = new char[compstrsize+sizeof(WCHAR)];
			ImmGetCompositionStringW(hIMC, dwindex, compstring, compstrsize);
			MCString unicodestr(compstring, compstrsize);

			// MW-2012-02-03: [[ Unicode Block ]] Use the new finsert method to insert
			//   text in unicode mode.
			MCactivefield->finsertnew(FT_IMEINSERT, unicodestr, 0, true);
			delete compstring;
			ImmReleaseContext(hwnd, hIMC);
		}
		break;
	case WM_IME_NOTIFY: //sent when IME opens windows
		switch (wParam)
		{
		case  IMN_OPENCANDIDATE:
			{
				HIMC hIMC = ImmGetContext(hwnd);
				DWORD imeprop = ImmGetProperty(GetKeyboardLayout(0), IGP_PROPERTY);
				if (imeprop & IME_PROP_AT_CARET)
				{
					if (MCactivefield)
					{
						uint1 i;
						for (i = 0; i < 4; i++)
						{
							MCRectangle r;
							CANDIDATEFORM cdf;
							cdf.dwIndex = i;
							cdf.dwStyle = CFS_CANDIDATEPOS;
							MCactivefield->getcompositionrect(r, -1);
							cdf.ptCurrentPos.x = r.x;
							cdf.ptCurrentPos.y = r.y + r.height + 32;
							cdf.rcArea.right = 1;
							cdf.rcArea.left = r.x;
							cdf.rcArea.top = r.y + r.height + 32;
							cdf.rcArea.bottom = 1;
							ImmSetCandidateWindow(hIMC, &cdf);
						}
					}
				}
			}
			break;
		case IMN_SETOPENSTATUS:
			{
				COMPOSITIONFORM cpf;
				HIMC hIMC = ImmGetContext(hwnd);
				cpf.dwStyle = CFS_DEFAULT;
				cpf.ptCurrentPos.x = 0;
				cpf.ptCurrentPos.y = 0;
				ImmSetCompositionWindow(hIMC, &cpf);
				ImmReleaseContext(hwnd, hIMC);
			}
			break;
		}
		return IsWindowUnicode(hwnd) ? DefWindowProcW(hwnd, msg, wParam, lParam) : DefWindowProcA(hwnd, msg, wParam, lParam);
		break;
	case WM_SETCURSOR:
		if (curinfo->live && !pms->isgrabbed() && LOWORD(lParam) != HTCLIENT)
			return IsWindowUnicode(hwnd) ? DefWindowProcW(hwnd, msg, wParam, lParam) : DefWindowProcA(hwnd, msg, wParam, lParam);
		MCmousestackptr = MCdispatcher->findstackd(dw);
		if (MCmousestackptr != NULL)
		{
			MCmousestackptr->resetcursor(True);
			if (pms->getmousetimer() == 0)
				pms->setmousetimer(timeSetEvent(LEAVE_CHECK_INTERVAL, 100,
				                                mouseproc, 0, TIME_ONESHOT));
		}
		if (omousestack != MCmousestackptr)
		{
			if (omousestack != NULL && omousestack != MCtracestackptr)
				omousestack->munfocus();
			if (MCmousestackptr != NULL && MCmousestackptr != MCtracestackptr)
				MCmousestackptr->enter();
		}
		break;
	case WM_CAPTURECHANGED:
		if (curinfo->live)
		{
			if (pms->isgrabbed())
			{
				MCStack *sptr = MCdispatcher->findstackd(dw);
				if (sptr != NULL)
				{
					if (lastdown != 0)
						sptr->mup(lastdown);
					buffer[0] = 0x1B; // escape
					buffer[1] = '\0';
					Boolean oldlock = MClockmessages;
					MClockmessages = True;
					sptr->kdown(buffer, XK_Escape);
					sptr->kup(buffer, XK_Escape);
					MClockmessages = oldlock;
					sptr->munfocus();
					pms->setgrabbed(False);
					curinfo->handled = True;
				}
				capturehwnd = NULL;
			}
		}
		break;
	case WM_MOUSEMOVE:  //MotionNotify:
	case WM_NCMOUSEMOVE:
		if (MCmousex != t_mx || MCmousey != t_my)
		{
			MCmousex = t_mx;
			MCmousey = t_my;
			if (curinfo->dispatch)
			{
				MCStack *oms = MCmousestackptr;
				if (msg != WM_NCMOUSEMOVE)
					MCmousestackptr = MCdispatcher->findstackd(dw);
				if (MCtracewindow == DNULL || hwnd != (HWND)MCtracewindow->handle.window)
				{
					if (oms != NULL && MCmousestackptr != oms)
						oms->munfocus();
					if (msg == WM_MOUSEMOVE)
					{
						MCdispatcher->wmfocus(dw, MCmousex, MCmousey);
						if (capturehwnd != NULL && MCbuttonstate != 0 && !dragclick && (MCU_abs(MCmousex - MCclicklocx) >= MCdragdelta || MCU_abs(MCmousey - MCclicklocy) >= MCdragdelta))
						{
							dragclick = True;
							MCdispatcher -> wmdrag(dw);
						}
					}
					else
						if (MCmousestackptr != NULL)
							MCmousestackptr->munfocus();
					curinfo->handled = True;
				}
			}
			else
			{
				MCEventnode *tptr = new MCEventnode(hwnd, msg, wParam, lParam, 0,
				                                    MCmodifierstate, MCeventtime);
				pms->appendevent(tptr);
			}
		}
		if (msg == WM_NCMOUSEMOVE)
			return IsWindowUnicode(hwnd) ? DefWindowProcW(hwnd, msg, wParam, lParam) : DefWindowProcA(hwnd, msg, wParam, lParam);
		break;
	case WM_APP:
		if (MCmousestackptr != NULL && MCdispatcher->getmenu() == NULL)
		{
			POINT p;
			if (!GetCursorPos(&p)
			        || !MCU_point_in_rect(MCmousestackptr->getrect(),
			                              (int2)p.x, (int2)p.y))
			{
				if (MCmousestackptr != MCtracestackptr)
					MCmousestackptr->munfocus();
			}
			else
				pms->setmousetimer(timeSetEvent(LEAVE_CHECK_INTERVAL, 100,
				                                mouseproc, 0, TIME_ONESHOT));
		}
		curinfo->handled = True;
		break;
	case WM_LBUTTONDOWN:
	case WM_MBUTTONDOWN:
	case WM_RBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_MBUTTONUP:
	case WM_RBUTTONUP:
	case WM_LBUTTONDBLCLK:
	case WM_MBUTTONDBLCLK:
	case WM_RBUTTONDBLCLK:
		if (msg == WM_LBUTTONDOWN || msg == WM_LBUTTONUP
		        || msg == WM_LBUTTONDBLCLK)
			button = 1;
		else
			if (msg == WM_MBUTTONDOWN || msg == WM_MBUTTONUP
			        || msg == WM_MBUTTONDBLCLK)
				button = 2;
			else
				button = 3;
		if (msg == WM_LBUTTONUP || msg == WM_MBUTTONUP || msg == WM_RBUTTONUP)
		{
			if (curinfo->live && !pms->isgrabbed())
			{
				ReleaseCapture();
				capturehwnd = NULL;
			}
			MCbuttonstate &= ~(1L << (button - 1));
			down = False;
			lastdown = 0;
		}
		else
		{
			if (curinfo->live && !pms->isgrabbed())
			{
				SetCapture(hwnd);
				capturehwnd = hwnd;
				lastdown = button;
			}
			MCbuttonstate |= 1L << (button - 1);
			down = True;
			if (msg == WM_LBUTTONDBLCLK || msg == WM_MBUTTONDBLCLK
			        || msg == WM_RBUTTONDBLCLK)
				doubledown = True;
		}
		if (curinfo->dispatch)
		{
			if (MCtracewindow == DNULL || hwnd != (HWND)MCtracewindow->handle.window)
				if (down)
					if (doubledown)
						MCdispatcher->wdoubledown(dw, button);
					else
					{
						if (doubleclick && MCeventtime - clicktime < MCdoubletime
						        && MCU_abs(MCclicklocx - t_mx) < MCdoubledelta
						        && MCU_abs(MCclicklocy - t_my) < MCdoubledelta)
							tripleclick = True;
						else
							tripleclick = False;
						doubleclick = False;
						MCclicklocx = t_mx;
						MCclicklocy = t_my;
						MCclickstackptr = MCmousestackptr;
						dragclick = False;
						MCdispatcher->wmfocus(dw, t_mx, t_my);
						MCdispatcher->wmdown(dw, button);
					}
				else
				{
					if (doubledown)
					{
						doubledown = False;
						doubleclick = True;
						clicktime = MCeventtime;
						MCdispatcher->wdoubleup(dw, button);
					}
					else
						MCdispatcher->wmup(dw, button);
				}
			curinfo->handled = curinfo->reset = True;
		}
		else
		{
			MCEventnode *tptr = new MCEventnode(hwnd, msg, wParam, lParam, 0,
			                                    MCmodifierstate, MCeventtime);
			pms->appendevent(tptr);
		}
		break;
	case WM_SIZE:
		{
			MCStack *target = MCdispatcher->findstackd(dw);
			if (target != NULL)
			{
				if (wParam == SIZE_MINIMIZED)
					target->iconify();
				else
					if (target->isiconic())
					{
						MCstacks->restack(target);
						target->configure(True);
						target->uniconify();
						SetWindowPos((HWND)target -> getwindow() -> handle . window, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER);
					}
					else
						target->configure(True);
				curinfo->handled = True;
			}
		}
		break;
	case WM_MOVE:
		MCdispatcher->configure(dw);
		curinfo->handled = True;
		break;
	case WM_CLOSE:
		MCdispatcher->wclose(dw);
		curinfo->handled = True;
		break;
	case WM_GETMINMAXINFO:
		target = MCdispatcher->findstackd(dw);
		if (target != NULL)
			target->constrain(lParam);
		break;
	case WM_ERASEBKGND:
		break;
	case WM_TIMER:
		curinfo->handled = True;
		if (MCmousestackptr != NULL && MCdispatcher->getmenu() == NULL)
		{
			int2 x, y;
			pms->querymouse(x, y);
			MCRectangle rect;
			if (pms->getwindowgeometry(MCmousestackptr->getw(), rect)
			        && capturehwnd == NULL && !pms->getgrabbed()
			        && !MCU_point_in_rect(rect, x, y))
			{
				MCmousestackptr->munfocus();
				MCmousestackptr = NULL;
			}
		}
		break;
	case WM_CANCELMODE:
		if (pms->isgrabbed())
		{
			buffer[0] = 0x1B;
			buffer[1] = '\0';
			Boolean oldlock = MClockmessages;
			MClockmessages = True;
			MCdispatcher->wkdown(dw, buffer, XK_Escape);
			MCdispatcher->wkup(dw, buffer, XK_Escape);
			MClockmessages = oldlock;
			curinfo->handled = True;
			pms->setgrabbed(False);
			MCmousex = MCmousey = -1; // prevent button msg from reopening menu
			MCdispatcher->wmfocus(dw, MCmousex, MCmousey);
		}
		break;
	case MM_MCINOTIFY:
		if (wParam == MCI_NOTIFY_SUCCESSFUL)
		{
			MCPlayer *tptr = MCplayers;
			while (tptr != NULL)
			{
				if (lParam == (LPARAM)tptr->getDeviceID())
				{
					if (tptr->isdisposable())
						tptr->playstop();
					else
						tptr->message_with_args(MCM_play_stopped, tptr->getname());
					break;
				}
				tptr = tptr->getnextplayer();
			}
			curinfo->handled = True;
		}
		break;
	case WM_USER:
		{
			uint2 i;
			for (i = 0 ; i < MCnsockets ; i++)
			{
				if (MCsockets[i]->fd == 0)
					MCsockets[i]->readsome();
				if (wParam == MCsockets[i]->fd && !MCsockets[i]->shared)
					break;
			}
			if (i < MCnsockets)
			{
				if (WSAGETSELECTERROR(lParam))
				{
					MCsockets[i]->error = new char[16 + I4L];
					sprintf(MCsockets[i]->error, "Error %d on socket",
					        WSAGETSELECTERROR(lParam));
					MCsockets[i]->doclose();
				}
				else
				{
					/* I.M
					 * TODO - look in to this further:
					 * we sometimes get FD_READ, but there's no data ready to read
					 * so trying to read when using SSL results in us getting stuck
					 * in SSTATE_RETRYREAD, which won't be cleared until data is
					 * available to read.  As a quick fix, we can check the socket with select()
					 */
					int t_events = 0;
					TIMEVAL t_time = {0,0};
					fd_set rmaskfd, wmaskfd, emaskfd;
					FD_ZERO(&rmaskfd);
					FD_ZERO(&wmaskfd);
					FD_ZERO(&emaskfd);
					FD_SET(wParam, &rmaskfd);
					FD_SET(wParam, &emaskfd);
					select(0, &rmaskfd, &wmaskfd, &emaskfd, &t_time);
					if (FD_ISSET(wParam, &emaskfd))
						t_events = t_events;
					if (FD_ISSET(wParam, &rmaskfd))
						t_events |= FD_READ;
					if (FD_ISSET(wParam, &wmaskfd))
						t_events |= FD_WRITE;
					switch (WSAGETSELECTEVENT(lParam))
					{
					case FD_OOB: // bogus, from MCS_read_socket
					case FD_READ:
						if (t_events & FD_READ)
							MCsockets[i]->readsome();
						break;
					case FD_WRITE:
						MCsockets[i]->writesome();
						MCsockets[i]->setselect();
						break;
					case FD_CONNECT:
						MCsockets[i]->writesome();
						MCsockets[i]->readsome();
						MCsockets[i]->setselect();
						break;
					case FD_ACCEPT:
						MCsockets[i]->acceptone();
						break;
					case FD_CLOSE:
						MCsockets[i]->readsome();
#ifdef MCSSL

						if (MCsockets[i]->fd != 0 && !MCsockets[i]->secure)
#else

						if (MCsockets[i]->fd != 0)
#endif

							MCsockets[i]->doclose();
						break;
					}
				}
			}
			curinfo->handled = True;
			break;
		}
	case WM_WINDOWPOSCHANGING:
	{
		((MCScreenDC *)MCscreen) -> restackwindows(hwnd, msg, wParam, lParam);
//			return IsWindowUnicode(hwnd) ? DefWindowProcW(hwnd, msg, wParam, lParam) : DefWindowProcA(hwnd, msg, wParam, lParam);
	}
	break;
	case WM_POWERBROADCAST:
		MCS_reset_time();
		return TRUE;
	case WM_THEMECHANGED:
	case WM_SYSCOLORCHANGE:
		if (hwnd == pms->getinvisiblewindow() && MCcurtheme && MCcurtheme->getthemeid() == LF_NATIVEWIN)
		{
			MCcurtheme->unload();
			MCcurtheme->load();

			// MW-2011-08-17: [[ Redraw ]] The theme has changed so redraw everything.
			MCRedrawDirtyScreen();
		}
		break;
	case WM_ACTIVATEAPP:
		if (wParam != isactive)
		{
			MCstacks->hidepalettes(!wParam);
			((MCScreenDC *)MCscreen) -> hidebackdrop(!wParam);
			if (MCdefaultstackptr != NULL)
				MCdefaultstackptr->getcard()->message(wParam ? MCM_resume : MCM_suspend);
			isactive = wParam;
			if (!wParam)
			{
				if (pms->taskbarhidden)
				{ //we are suspended, show menu bar for other process
					pms->showtaskbar();
					pms->taskbarhidden = True;
				}
			}
			else
			{
				if (pms->taskbarhidden)
				{
					pms->taskbarhidden = False;
					pms->hidetaskbar();
				}
			}
		}
		break;
	case WM_INPUTLANGCHANGE:
		{
			LCID t_locale_id;
			t_locale_id = MAKELCID(lParam, SORT_DEFAULT);
			
			char t_info[8];
			GetLocaleInfoA(t_locale_id, LOCALE_IDEFAULTANSICODEPAGE, t_info, 8);
			((MCScreenDC *)MCscreen) -> input_codepage = atoi(t_info);
			((MCScreenDC *)MCscreen) -> system_codepage = GetACP();
		}
		break;

	case WM_NCACTIVATE:
		if (MCactivatepalettes && wParam == FALSE && MCstacks->getactive())
		{
			MCStack *sptr = MCdispatcher->findstackd(dw);
			if (sptr != NULL && sptr->getmode() == WM_PALETTE)
				wParam = TRUE;
		}
		return IsWindowUnicode(hwnd) ? DefWindowProcW(hwnd, msg, wParam, lParam) : DefWindowProcA(hwnd, msg, wParam, lParam);
	case WM_MOUSEWHEEL:
	case WM_MOUSEHWHEEL:
		if (MCmousestackptr != NULL)
		{
			MCObject *mfocused = MCmousestackptr->getcard()->getmfocused();
			if (mfocused == NULL)
				mfocused = MCmousestackptr -> getcard();

			if (mfocused != NULL)
			{
				int4 val = (short)HIWORD(wParam);
				if (msg == WM_MOUSEWHEEL)
				{
					if (val < 0)
						mfocused->kdown("", XK_WheelUp);
					else
						mfocused->kdown("", XK_WheelDown);
				}
				else if (msg == WM_MOUSEHWHEEL)
				{
					if (val < 0)
						mfocused->kdown("", XK_WheelLeft);
					else
						mfocused->kdown("", XK_WheelRight);
				}
			}
		}
	break;
	default:
		return IsWindowUnicode(hwnd) ? DefWindowProcW(hwnd, msg, wParam, lParam) : DefWindowProcA(hwnd, msg, wParam, lParam);
	}

	return 0;
}
