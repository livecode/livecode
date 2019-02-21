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
#include "w32dsk-legacy.h"

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

#define MSH_MOUSEWHEEL L"MSWHEEL_ROLLMSG"
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

void MCScreenDC::appendevent(MCEventnode *tptr)
{
	tptr->appendto(pendingevents);
}

void CALLBACK mouseproc(UINT id, UINT msg, DWORD_PTR user, DWORD_PTR dw1, DWORD_PTR dw2)
{
	MCScreenDC *pms = (MCScreenDC *)MCscreen;
	pms->setmousetimer(0);
	PostMessageA(pms->getinvisiblewindow(), WM_APP, 0, 0);
}

// SN-2014-09-10: [[ Bug 13348 ]] We need to know what was intended (key down or key up)
// when WM_[SYS|IME_]CHAR is triggered (it seems like WM_KEYUP events are only translated 
// to WM_CHAR when the key is kept pressed).
enum KeyMove
{
	KM_KEY_DOWN,
	KM_KEY_UP,
	KM_NO_KEY_MOVE
};

typedef struct
{
	Boolean dispatch, abort, reset, handled, live;
	// SN-2014-09-10: [[ Bug 13348 ]] KeyMove added to the stateinfo of the event
	KeyMove keymove;
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
	// Behaviour:
	//	If no control keys accumulated and this is a left Ctrl, then add LCTRL
	//	Else, if accumulated an LCTRL and receive a right Alt, switch to RALT
	//	Else, if accumulated a RALT or ALTGR, switch to ALTGR
	//  Else, clear accumulated keys.
	//
	//  If accumulated an ALTGR and modifier state holds Ctrl+Alt, clear Ctrl-Alt
	//	Else, clear an accumulated ALTGR
	//
	// So, what this is doing is:
	//	All control keys are passed unmodified, except:
	//		Right Alt has special behaviour after Left Ctrl: act as AltGr on subsequent keys

	static int curks;
	if (curks == KS_UNDEFINED && wParam == VK_CONTROL && !(lParam & 0x01000000))
		curks = KS_LCTRL;
	else
	{
		if (curks == KS_LCTRL && wParam == VK_MENU && (lParam == 0x01000000))
			curks = KS_RALT;
		else
		{
			if (curks == KS_RALT || curks == KS_ALTGR)
				curks = KS_ALTGR;
			else
				curks = KS_UNDEFINED;
		}
	}
	KeySym keysym;

	// We need to update the modifier state here. If the current event being
	// processed is not live then we must rely on the state of MCmodifierstate
	// as it is. Otherwise, we must fetch the synchronous key state.
	if (curinfo -> live)
	{
		uint2 t_state = 0;
		if (GetKeyState(VK_CONTROL) & 0x8000)
			t_state |= MS_CONTROL;
		if (GetKeyState(VK_MENU) & 0x8000)
			t_state |= MS_MOD1;
		if (GetKeyState(VK_SHIFT) & 0x8000)
			t_state |= MS_SHIFT;
		if (GetKeyState(VK_CAPITAL) & 0x0001)
			t_state |= MS_CAPS_LOCK;
		MCmodifierstate = t_state;
	}

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

bool MCScreenDC::getkeysdown(MCListRef& r_list)
{
	MCAutoListRef t_list;
	if (!MCListCreateMutable(',', &t_list))
		return false;

	BYTE keystates[256];
	if (GetKeyboardState((PBYTE)&keystates) == 0) // should use an async function?
	{
		r_list = MCValueRetain(kMCEmptyList);
		return true;
	}

	MCmodifierstate = querymods();
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
				if (!MCListAppendInteger(*t_list, ksym))
					return false;
		}
	}
	return MCListCopy(*t_list, r_list);
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
	// SN-2014-09-10: [[ Bug 13348 ]] No key move by default
	curinfo->keymove = KM_NO_KEY_MOVE;
	if (mousewheel == 0)
		mousewheel = RegisterWindowMessageW(MSH_MOUSEWHEEL);
	if (dispatch && pendingevents != NULL
	        || PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE)
	        || sleep != 0
	        && MsgWaitForMultipleObjects(1, &g_notify_wakeup, False,
	                                     (DWORD)(sleep * 1000.0), QS_ALLINPUT)
	        != WAIT_TIMEOUT && PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE))
	{
		if (dispatch && pendingevents != NULL)
		{
			MCEventnode *tptr = pendingevents->remove(pendingevents);;
			curinfo->live = False;
			MCmodifierstate = tptr->modifier;
			msg.hwnd = tptr->hwnd;
			msg.message = tptr->msg;
			msg.wParam = tptr->wParam;
			msg.lParam = tptr->lParam;
			msg.pt.x = msg.pt.y = 0;
            msg.time = tptr->time;
			delete tptr;
		}

		bool t_keymessage, t_syskeymessage;
		t_keymessage = msg.message == WM_KEYDOWN || msg.message == WM_KEYUP;
		t_syskeymessage = msg.message == WM_SYSKEYDOWN || msg.message == WM_SYSKEYUP;

		MCeventtime = msg.time;
		if (t_keymessage || t_syskeymessage)
			curinfo->keysym = getkeysym(msg.wParam, msg.lParam);
				
		// SN-2014-09-10: [[ Bug 13348 ]] Set the key move appropriately
		if (msg.message == WM_KEYUP || msg.message == WM_SYSKEYUP)
			curinfo->keymove = KM_KEY_UP;
		else if (msg.message == WM_KEYDOWN || msg.message == WM_SYSKEYDOWN)
			curinfo->keymove = KM_KEY_DOWN;

		// IM-2016-01-11: [[ Bug 16415 ]] Always use DispatchMessage for non-stack windows		
		bool t_foreign_window;
		t_foreign_window = MCdispatcher->findstackwindowid((uintptr_t)msg.hwnd) == nil;
		bool t_os_dispatch;
		t_os_dispatch = !dispatch || t_foreign_window;

		if (t_keymessage || t_syskeymessage)
		{
			// IM-2016-01-11: [[ Bug 16415 ]] Call TranslateMessage for any hwnd.
			//    Only remove WM_(SYS)KEYDOWN for stack windows.
			TranslateMessage(&msg);

			// If the window receiving the key message is a stack window then we
			// only want to see WM_CHAR and not WM_KEYDOWN. However, if it is a
			// non-stack window (like CEF Browser's) we don't want to fiddle with
			// the message flow.
			if (!t_foreign_window)
			{
				// SN-2014-09-05: [[ Bug 13348 ]] Remove the WM_KEYDOWN, WM_SYSKEYDOWN messages
				// in case TranslateMessage succeeded, and queued a WM_[SYS]CHAR message
				if (t_keymessage)
					PeekMessageW(&msg, NULL, WM_CHAR, WM_DEADCHAR, PM_REMOVE);				
				else if (t_syskeymessage)
					PeekMessageW(&msg, NULL, WM_SYSCHAR, WM_SYSDEADCHAR, PM_REMOVE);
			}
		}

		if (t_os_dispatch)
			DispatchMessageW(&msg);
		else
			MCWindowProc(msg.hwnd, msg.message, msg.wParam, msg.lParam);
	}

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
// SN-2014-09-10: [[ Bug 13348 ]] We want to keep the last codepoint now, 
// not the last char
static uint32_t lastcodepoint;
static KeySym lastkeysym;
// SN-2014-09-12: [[ Bug 13423 ]] Keeps whether a the next char follows a dead char
static Boolean deadcharfollower = False;
// SN-2015-05-18: [[ Bug 15040 ]] Keep an indication if we are in an Alt+<number>
//  sequence. We don't want to send any keyDown/Up message (same as dead chars).
static Boolean isInAltPlusSequence = False;
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

	t_info = (WINDOWPOS *)p_lparam;
	t_stack = MCdispatcher -> findstackwindowid((uint32_t)p_window);

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

	MCPoint t_mouseloc;
	t_mouseloc = MCPointMake(LOWORD(lParam), HIWORD(lParam));

	// IM-2014-01-28: [[ HiDPI ]] Convert screen to logical coords
	t_mouseloc = MCscreen->screentologicalpoint(t_mouseloc);

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

				MCStringRef t_tempstr;
				MCAutoStringRef t_cmdline;
				/* UNCHECKED */ MCStringCreateWithBytes((const byte_t*)t_data -> lpData, t_data -> cbData, kMCStringEncodingUTF16, false, t_tempstr);
				/* UNCHECKED */ MCStringMutableCopyAndRelease(t_tempstr, &t_cmdline);
				/* UNCHECKED */ MCStringFindAndReplaceChar(*t_cmdline, '\\', '/', kMCStringOptionCompareExact);

				uindex_t sptr = 0;
				unichar_t t_char;
				while((t_char = MCStringGetCharAtIndex(*t_cmdline, sptr)) != '\0')
				{
					uindex_t t_argument;
					int t_argument_length;

					while (iswspace(t_char))
						t_char = MCStringGetCharAtIndex(*t_cmdline, ++sptr);

					t_argument_length = 0;
					if (t_char == '"')
					{
						t_argument = ++sptr;
						while ((t_char = MCStringGetCharAtIndex(*t_cmdline, sptr)) != '\0'
							&& t_char != '"')
						{
							sptr++;
							t_argument_length += 1;
						}
					}
					else
					{
						t_argument = sptr;
						// SN-2014-11-19: [[ Bug 14058 ]] We consider the characters which are NOT a space
						while (t_char != '\0' && !iswspace(t_char))
						{
							t_char = MCStringGetCharAtIndex(*t_cmdline, ++sptr);
							t_argument_length += 1;
						}
					}

					if (t_argument_length != 0)
					{
						MCParameter *t_parameter;
						MCAutoStringRef t_param;
						/* UNCHECKED */ MCStringCopySubstring(*t_cmdline, MCRangeMake(t_argument, t_argument_length), &t_param);
						t_parameter = new (nothrow) MCParameter;
						t_parameter -> setvalueref_argument(*t_param);
						if (t_first_parameter == NULL)
							t_first_parameter = t_parameter;
						else
							t_current_parameter -> setnext(t_parameter);
						t_current_parameter = t_parameter;
					}

					if (t_char != '\0')
						t_char = MCStringGetCharAtIndex(*t_cmdline, ++sptr);
				}

				if (MCdispatcher -> gethome() -> message(MCM_relaunch, t_first_parameter, False, True) != ES_NORMAL)
					t_result = 0;
				else
				{
                    MCExecContext ctxt(nil, nil, nil);
                    MCAutoValueRef t_value;
					/* UNCHECKED */ MCresult -> eval(ctxt, &t_value);
                    MCAutoStringRef t_string;
                    /* UNCHECKED */ ctxt . ConvertToString(*t_value, &t_string);
					
                    if (MCStringIsEqualToCString(*t_string, "background", kMCCompareCaseless))
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
			MCEventnode *tptr = new (nothrow) MCEventnode(hwnd, msg, wParam, lParam, 0,
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
			MCEventnode *tptr = new (nothrow) MCEventnode(hwnd, msg, wParam, lParam, 0,
			                                    MCmodifierstate, MCeventtime);
			pms->appendevent(tptr);
		}
		break;

	// SN-2014-09-12: [[ Bug 13423 ]] The next character typed will follow a dead char. Sets the flag.
	case WM_DEADCHAR:
	case WM_SYSDEADCHAR:
		deadcharfollower = true;
		break;

	case WM_CHAR:
	case WM_SYSCHAR:
	case WM_IME_CHAR:
	{
		// Don't bother processing if we're not dispatching
		if (!curinfo->dispatch)
		{
			MCEventnode *tptr = new (nothrow) MCEventnode(hwnd, msg, wParam, lParam, 0,
			                                    MCmodifierstate, MCeventtime);
			pms->appendevent(tptr);
			break;
		}
		
		// If the repeat count > 1 then we only process one event right now
		// and push a pending event onto the queue for the rest. This means that
		// 'flushEvents' can purge any repeated key messages.
		if (LOWORD(lParam) > 1)
		{
			MCEventnode *tptr = new (nothrow) MCEventnode(hwnd, msg, wParam, MAKELPARAM(LOWORD(lParam) - 1, HIWORD(lParam)), 0, MCmodifierstate, MCeventtime);
			pms->appendevent(tptr);
			lParam = MAKELPARAM(1, HIWORD(lParam));
		}

		// SN-2014-09-10: [[ Bug 13348 ]] The keysym is got as for the WM_KEYDOWN case
		if (curinfo->keysym == 0) // event came from some other dispatch
			// SN-2014-09-12: [[ Bug 13423 ]] If we are following a dead char, no conversion needed:
			// the message is fired without passing by MCScreenDC::handle, that's why
			// curinfo->keysym hasn't been set, and the typed char is in wParam
			if (deadcharfollower)
				keysym = wParam;
			else
				keysym = pms->getkeysym(wParam, lParam);
		else
			keysym = curinfo->keysym;

		lastkeysym = keysym;
		
		// UTF-16 or ANSI character has been received
		WCHAR t_char;
		t_char = LOWORD(wParam);

		MCAutoStringRef t_input;

		// MW-2010-11-17: [[ Bug 3892 ]] Ctrl+Alt can be the same as AltGr.
		//   If we have Ctrl+Alt set, we discard the modifiers
		if ((MCmodifierstate & (MS_CONTROL | MS_ALT)) == (MS_CONTROL | MS_ALT))
			MCmodifierstate = 0;

		if (IsWindowUnicode(hwnd))
		{
			// Window is Unicode; received characters are UTF-16
			/* UNCHECKED */ MCStringCreateWithChars((const unichar_t*)&t_char, 1, &t_input);
		}
		else if (msg == WM_IME_CHAR)
		{
			// The window isn't Unicode and the character came from an IME
			// Translate the characters to UTF-16
			unichar_t t_utf16[2] = {0, 0};
			uindex_t t_len;
			char multibytechar[3];
			multibytechar[0] =  HIBYTE((WORD)wParam) ;
			multibytechar[1] =  LOBYTE((WORD)wParam) ;
			multibytechar[2] = '\0';
			MCU_multibytetounicode(multibytechar, 2, (char *)&t_utf16, 4,
				                     t_len,  MCS_langidtocharset(LOWORD(GetKeyboardLayout(0))));
			/* UNCHECKED */ MCStringCreateWithChars(t_utf16, 2, &t_input);
		}
		else
		{
			// Window isn't Unicode and the character came from something that wasn't an IME
			// Translate the characters to UTF-16
			WCHAR t_utf16;
			MultiByteToWideChar(pms->input_codepage, 0, (const char *)&t_char, 1, &t_utf16, 1);
			/* UNCHECKED */ MCStringCreateWithChars((const unichar_t*)&t_utf16, 1, &t_input);
		}

		// Translate the input character into its corresponding keysym
		// TODO: surrogate pairs?
		codepoint_t t_codepoint = MCStringGetCodepointAtIndex(*t_input, 0);
		// SN-2014-09-10: [[ Bug 13348 ]] Only add the codepoint if it is not a control char
		if (iswcntrl(t_codepoint))
			lastcodepoint = 0;
		else	
			lastcodepoint = t_codepoint;

		KeySym t_keysym;

		// No need to send control characters as text
		if (iswcntrl(t_char))
			t_keysym = keysym;
		// SN-2014-09-18: [[ MERGE-6_7_RC_2 ]] We accept the whole extended ASCII table
		else if (t_codepoint > 0xFF)
		{
			// This is a non-ASCII codepoint
			t_keysym = t_codepoint | XK_Class_codepoint;
		}
		else
		{
			// The keysym is ASCII; send as a compat keysym
			t_keysym = t_codepoint;
		}

		if (MCtracewindow == DNULL || hwnd != (HWND)MCtracewindow->handle.window)
		{
			// SN-2014-09-05: [[ Bug 13348 ]] Call the appropriate message
			//	 [[ MERGE-6_7_RC_2 ]] and add the KeyDown/Up in case we follow
			//   a dead-key started sequence
			// SN-2015-05-18: [[ Bug 15040 ]] We must send the char resulting
			//  from an Alt+<number> sequence.
            // If the current event isn't live, then we cannot trust the keymove
            // field, so we assume that we must process it.
			if (!curinfo->live || curinfo->keymove == KM_KEY_DOWN || deadcharfollower || isInAltPlusSequence)
			{
				// Pressing Alt starts a number-typing sequence.
				//  Otherwise, we are not in such a sequence - be it because a normal
				//  char has been typed, or because the sequence is terminated.
				isInAltPlusSequence = (MCmodifierstate == MS_ALT);

				// We don't want to send any Key message for the "+" pressed
				//  to start the Alt+<number> sequence
				if (isInAltPlusSequence
						|| (!MCdispatcher->wkdown(dw, *t_input, t_keysym)
							&& msg == WM_SYSCHAR))
					return IsWindowUnicode(hwnd) ? DefWindowProcW(hwnd, msg, wParam, lParam) : DefWindowProcA(hwnd, msg, wParam, lParam);
			}
				
			if (curinfo->keymove == KM_KEY_UP || deadcharfollower)
  				MCdispatcher->wkup(dw, *t_input, t_keysym);

			curinfo->handled = curinfo->reset = true;
		}

		// SN-2015-05-18: [[ Bug 15040 ]] Make sure that it can't let be set to True
		isInAltPlusSequence = False;

		break;
	}

	case WM_KEYDOWN:
	case WM_SYSKEYDOWN:
	{
		if (wParam == VK_CONTROL)
			break;

		// Did the event come via MCScreenDC::handle?
		if (curinfo->keysym == 0) // event came from some other dispatch
			keysym = pms->getkeysym(wParam, lParam);
		else
			keysym = curinfo->keysym;

		// SN-2014-09-18: [[ MERGE-6_7_RC_2 ]] Reaching here means that any
		// dead-key sequence is aborted. Reset the flag and the codepoint
		deadcharfollower = False;
		lastkeysym = keysym;
		lastcodepoint = 0;

		if (MCmodifierstate & MS_CONTROL)
			if (wParam == VK_CANCEL || keysym == '.')
			{
				if (MCallowinterrupts && !MCdefaultstackptr->cantabort())
					curinfo->abort = True;
				else
					MCinterrupt = True;
			}

		if (curinfo->dispatch)
		{
			// SN-2015-05-18: [[ Bug 15040 ]] We do not want to send any key
			//  message if we are in an Alt+<number> sequence.
			if ((MCtracewindow == DNULL || hwnd != (HWND)MCtracewindow->handle.window) 
					&& !isInAltPlusSequence)
			{
				// The low word contains the repeat count
				uint2 count = LOWORD(lParam);
				while (count--)
				{
					// This is (effectively) equivalent to TranslateAccerator. If the key gets
					// handled at this stage, nothing further happens with it. Otherwise, it
					// gets re-dispatched as a character input message.
					//
					// Parts of the engine that care about keystrokes examine the keysym member.
					// Parts that want text only care about the string. Dispatch as stroke-only here.
					bool t_handled;
					t_handled = MCdispatcher->wkdown(dw, kMCEmptyString, keysym);

					// Some system key strokes have special behaviour provided by DefWindowProc
					if (!t_handled && msg == WM_SYSKEYDOWN)
						return IsWindowUnicode(hwnd) ? DefWindowProcW(hwnd, msg, wParam, lParam) : DefWindowProcA(hwnd, msg, wParam, lParam);
					
					// If the repeat count was >1, simulate the corresponding key up messages
					// SN-2014-09-15: [[ Bug 13423 ]] We want to send a KEYUP for the dead char, if it failed to combine
					if (count || lParam & 0x40000000 || deadcharfollower)
						MCdispatcher->wkup(dw, kMCEmptyString, keysym);
				}
				curinfo->handled = curinfo->reset = True;
			}
		}
		else
		{
			// Dispatch isn't currently enabled; accumulate to the event queue.
			MCEventnode *tptr = new (nothrow) MCEventnode(hwnd, msg, wParam, lParam, keysym,
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
		// SN-2014-09-15: [[ Bug 13423 ]] We don't regard the event if we are following a dead char WITH a
		// keysym: that's the dead char itself.
		// SN-2014-09-18: [[ MERGE-6_7_RC_2 ]] SYSKEYUP shall pass
		else if (!deadcharfollower || msg == WM_SYSKEYUP)
			keysym = curinfo->keysym;
		else
			break;

		// SN-2014-09-10: [[ Bug 13348 ]] We want to get the last codepoint translated, if any,
		// since the KEYUP events are not translated (but when keeping pressed a key)
		MCAutoStringRef t_string;

		// SN-2014-09-18: [[ MERGE-6_7_RC_2 ]] Only build a string if there is a codepoint
		if (lastkeysym == keysym && lastcodepoint)
		{
			unichar_t t_pair[2];
			uindex_t t_char_count;
			
			t_char_count = MCStringCodepointToSurrogates(lastcodepoint, t_pair);
			/* UNCHECKED */ MCStringCreateWithChars(t_pair, t_char_count, &t_string);
		}
		else
		{
			lastkeysym = keysym;
			lastcodepoint = 0;
			t_string = kMCEmptyString;
		}

		if (curinfo->dispatch)
		{
			if (MCtracewindow == DNULL || hwnd != (HWND)MCtracewindow->handle.window)
			{
				// Character messages handle the text keyup themselves
				MCeventtime = GetMessageTime(); //krevent->time;
				// SN-2014-09-10: [[ Bug 13348 ]] Send the string we could build from the last
				// codepoint.
				// SN-2015-05-18: [[ Bug 15040 ]] We don't send any key message
				//  if we are in an Alt+<number> sequence.
                isInAltPlusSequence = (MCmodifierstate == MS_ALT);
				if (!isInAltPlusSequence)
					MCdispatcher->wkup(dw, *t_string, keysym);
				curinfo->handled = curinfo->reset = True;
			}
		}
		else
		{
			// Add to the event queue
			MCEventnode *tptr = new (nothrow) MCEventnode(hwnd, msg, wParam, lParam, 0,
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
	case WM_IME_COMPOSITION:
		{
			// Because the Win32-provided controls are not in use, proper display of IME
			// text is the responsibility of the engine instead of the operating system.
			if (!MCinlineinput)
				return IsWindowUnicode(hwnd) ? DefWindowProcW(hwnd, msg, wParam, lParam) : DefWindowProcA(hwnd, msg, wParam, lParam);
			
			// No point doing any IME work if there isn't an active field (IME is not
			// allowed for shortcuts, accelerators, etc)
			if (!MCactivefield)
				break;

			// On Windows, IME input can be divided into three parts:
			//	Composition string
			//	Result string
			//	Cursor
			//
			// The composition string is the text that the IME system has provisionally
			// guessed to be the desired text. It is normally displayed with an underline.
			//
			// The results string is the text that has been accepted as correct.
			//
			// The cursor specifies where in the text the insertion cursor should appear.
			//
			// Note that the Windows IME model allows both the result string and the
			// composition string to be presented simultaneously.
			HIMC hIMC;
			uint16_t t_cursorpos;
			hIMC = ImmGetContext(hwnd);

			// Update the compose cursor position
			// TODO: pay attention to the CS_INSERTCHAR and CS_NOMOVECARET flags
			if (!hIMC)
				break;
			t_cursorpos = LOWORD(ImmGetCompositionStringW(hIMC, GCS_CURSORPOS, NULL, 0));
			MCactivefield->setcompositioncursoroffset(t_cursorpos << 1);

			if (lParam & GCS_RESULTSTR)
			{
				// A result string has been provided. Stop composition and display it.
				MCactivefield->stopcomposition(True, False);
				
				LONG t_reslen;
				unichar_t *t_resstr;
				MCAutoStringRef t_string;
				t_reslen = ImmGetCompositionStringW(hIMC, GCS_RESULTSTR, NULL, 0);
				t_resstr = new (nothrow) unichar_t[t_reslen/sizeof(unichar_t) + 1];
				/* UNCHECKED */ ImmGetCompositionStringW(hIMC, GCS_RESULTSTR, t_resstr, t_reslen);
				/* UNCHECKED */ MCStringCreateWithCharsAndRelease(t_resstr, t_reslen/2, &t_string);
				MCactivefield->finsertnew(FT_IMEINSERT, *t_string, 0);
			}

			if (lParam & GCS_COMPSTR)
			{
				// A composition string has been provided. Start composition
				MCactivefield->startcomposition();
				
				LONG t_complen;
				unichar_t *t_compstr;
				MCAutoStringRef t_string;
				t_complen = ImmGetCompositionStringW(hIMC, GCS_COMPSTR, NULL, 0);
				t_compstr = new (nothrow) unichar_t[t_complen/sizeof(unichar_t) + 1];
				/* UNCHECKED */ ImmGetCompositionStringW(hIMC, GCS_COMPSTR, t_compstr, t_complen);
				/* UNCHECKED */ MCStringCreateWithCharsAndRelease(t_compstr, t_complen/2, &t_string);
				MCactivefield->finsertnew(FT_IMEINSERT, *t_string, 0);
			}

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
		if (MCmousestackptr)
		{
			MCmousestackptr->resetcursor(True);
			if (pms->getmousetimer() == 0)
				pms->setmousetimer(timeSetEvent(LEAVE_CHECK_INTERVAL, 100,
				                                mouseproc, 0, TIME_ONESHOT));
		}
		if (!MCmousestackptr.IsBoundTo(omousestack))
		{
			if (omousestack != NULL && omousestack != MCtracestackptr)
				omousestack->munfocus();
			if (MCmousestackptr && MCmousestackptr != MCtracestackptr)
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
						sptr->mup(lastdown, false);
					buffer[0] = 0x1B; // escape
					buffer[1] = '\0';
					Boolean oldlock = MClockmessages;
					MClockmessages = True;
					sptr->kdown(MCSTR("\x1B"), XK_Escape);
					sptr->kup(MCSTR("\x1B"), XK_Escape);
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
		// IM-2013-09-23: [[ FullscreenMode ]] Update mouseloc with MCscreen getters & setters
		MCStack *t_old_mousestack;
		MCPoint t_old_mouseloc;
		MCscreen->getmouseloc(t_old_mousestack, t_old_mouseloc);
		if (t_old_mouseloc.x != t_mouseloc.x || t_old_mouseloc.y != t_mouseloc.y)
		{
			MCscreen->setmouseloc(t_old_mousestack, t_mouseloc);
			if (curinfo->dispatch)
			{
				if (msg != WM_NCMOUSEMOVE)
					MCscreen->setmouseloc(MCdispatcher->findstackd(dw), t_mouseloc);
				if (MCtracewindow == DNULL || hwnd != (HWND)MCtracewindow->handle.window)
				{
					if (t_old_mousestack != NULL && !MCmousestackptr.IsBoundTo(t_old_mousestack))
						t_old_mousestack->munfocus();
					if (msg == WM_MOUSEMOVE)
					{
						MCPoint t_clickloc;
						MCStack *t_stackptr;
						MCscreen->getclickloc(t_stackptr, t_clickloc);

						MCdispatcher->wmfocus(dw, t_mouseloc.x, t_mouseloc.y);
						if (capturehwnd != NULL && MCbuttonstate != 0 && !dragclick && (MCU_abs(t_mouseloc.x - t_clickloc.x) >= MCdragdelta || MCU_abs(t_mouseloc.y - t_clickloc.y) >= MCdragdelta))
						{
							dragclick = True;
							MCdispatcher -> wmdrag(dw);
						}
					}
					else
						if (MCmousestackptr)
							MCmousestackptr->munfocus();
					curinfo->handled = True;
				}
			}
			else
			{
				MCEventnode *tptr = new (nothrow) MCEventnode(hwnd, msg, wParam, lParam, 0,
				                                    MCmodifierstate, MCeventtime);
				pms->appendevent(tptr);
			}
		}
		if (msg == WM_NCMOUSEMOVE)
			return IsWindowUnicode(hwnd) ? DefWindowProcW(hwnd, msg, wParam, lParam) : DefWindowProcA(hwnd, msg, wParam, lParam);
		break;
	case WM_APP:
		if (MCmousestackptr && MCdispatcher->getmenu() == NULL)
		{
			// IM-2014-04-17: [[ Bug 12227 ]] Convert logical stack rect to screen coords when testing for mouse intersection
			// IM-2014-08-01: [[ Bug 13058 ]] Use stack view rect to get logical window rect
			MCRectangle t_rect;
			t_rect = pms->logicaltoscreenrect(MCmousestackptr->view_getrect());

			POINT p;
			if (!GetCursorPos(&p)
			        || !MCU_point_in_rect(t_rect,
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
						        && MCU_abs(MCclicklocx - t_mouseloc.x) < MCdoubledelta
						        && MCU_abs(MCclicklocy - t_mouseloc.y) < MCdoubledelta)
							tripleclick = True;
						else
							tripleclick = False;
						doubleclick = False;
						// IM-2013-09-23: [[ FullscreenMode ]] Update clickloc with MCscreen getters & setters
						MCscreen->setclickloc(MCmousestackptr, t_mouseloc);
						dragclick = False;
						MCdispatcher->wmfocus(dw, t_mouseloc.x, t_mouseloc.y);
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
			MCEventnode *tptr = new (nothrow) MCEventnode(hwnd, msg, wParam, lParam, 0,
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
						MCdispatcher->wreshape(dw);
						target->uniconify();
						SetWindowPos((HWND)target -> getwindow() -> handle . window, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER);
					}
					else
						MCdispatcher->wreshape(dw);
				curinfo->handled = True;
			}
		}
		break;
	case WM_MOVE:
		// IM-2016-04-14: [[ Bug 16749 ]] WM_MOVE can arrive before WM_SIZE when minimized.
		//   - check if window is minimized before reconfiguring the stack
		if (IsIconic(hwnd))
		{
			MCStack *target = MCdispatcher->findstackd(dw);
			if (target != NULL)
				target->iconify();
		}
		else
		{
			MCdispatcher->wreshape(dw);
			curinfo->handled = True;
		}
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
		if (MCmousestackptr && MCdispatcher->getmenu() == NULL)
		{
			int2 x, y;
			pms->querymouse(x, y);
			MCRectangle rect;
			if (pms->getwindowgeometry(MCmousestackptr->getw(), rect)
			        && capturehwnd == NULL && !pms->getgrabbed()
			        && !MCU_point_in_rect(rect, x, y))
			{
				MCmousestackptr->munfocus();
				MCmousestackptr = nil;
			}
		}
		break;
	case WM_CANCELMODE:
		if (pms->isgrabbed())
		{
			Boolean oldlock = MClockmessages;
			MClockmessages = True;
			MCdispatcher->wkdown(dw, MCSTR("\x1B"), XK_Escape);
			MCdispatcher->wkup(dw, MCSTR("\x1B"), XK_Escape);
			MClockmessages = oldlock;
			curinfo->handled = True;
			pms->setgrabbed(False);
			MCmousex = MCmousey = -1; // prevent button msg from reopening menu
			MCdispatcher->wmfocus(dw, MCmousex, MCmousey);
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
					MCsockets[i]->error = new (nothrow) char[16 + I4L];
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
		if (MCmousestackptr)
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
						mfocused->kdown(kMCEmptyString, XK_WheelUp);
					else
						mfocused->kdown(kMCEmptyString, XK_WheelDown);
				}
				else if (msg == WM_MOUSEHWHEEL)
				{
					if (val < 0)
						mfocused->kdown(kMCEmptyString, XK_WheelLeft);
					else
						mfocused->kdown(kMCEmptyString, XK_WheelRight);
				}
			}
		}
	break;
	default:
		return IsWindowUnicode(hwnd) ? DefWindowProcW(hwnd, msg, wParam, lParam) : DefWindowProcA(hwnd, msg, wParam, lParam);
	}

	return 0;
}
