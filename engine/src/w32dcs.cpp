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

#include "execpt.h"
#include "dispatch.h"
#include "stack.h"
#include "image.h"
#include "util.h"
#include "date.h"
#include "player.h"

#include "sellst.h"
#include "stacklst.h"
#include "pxmaplst.h"
#include "card.h"
#include "globals.h"
#include "osspec.h"

#include "w32dc.h"
#include "w32context.h"
#include "resource.h"
#include "meta.h"
#include "mode.h"

#include "w32text.h"

#ifndef CS_DROPSHADOW
#define CS_DROPSHADOW   0x00020000
#endif

uint4 MCScreenDC::image_inks[] =
    {
        BLACKNESS, SRCAND, SRCERASE,
        SRCCOPY, 0x00220326, 0x00AA0029, SRCINVERT, SRCPAINT, NOTSRCERASE,
        SRCINVERT, DSTINVERT, 0x00DD0228, NOTSRCCOPY, MERGEPAINT,
        0x007700E6, WHITENESS
    };

static MCColor vgapalette[16] =
    {
        {0, 0x0000, 0x0000, 0x0000, 0, 0}, {1, 0x8080, 0x0000, 0x0000, 0, 0},
        {2, 0x0000, 0x8080, 0x0000, 0, 0}, {3, 0x0000, 0x0000, 0x8080, 0, 0},
        {4, 0x8080, 0x8080, 0x0000, 0, 0}, {5, 0x8080, 0x0000, 0x8080, 0, 0},
        {6, 0x0000, 0x8080, 0x8080, 0, 0}, {7, 0x8080, 0x8080, 0x8080, 0, 0},
        {8, 0xC0C0, 0xC0C0, 0xC0C0, 0, 0}, {9, 0xFFFF, 0x0000, 0x0000, 0, 0},
        {10, 0x0000, 0xFFFF, 0x0000, 0, 0}, {11, 0x0000, 0x0000, 0xFFFF, 0, 0},
        {12, 0xFFFF, 0xFFFF, 0x0000, 0, 0}, {13, 0xFFFF, 0x0000, 0xFFFF, 0, 0},
        {14, 0x0000, 0xFFFF, 0xFFFF, 0, 0}, {15, 0xFFFF, 0xFFFF, 0xFFFF, 0, 0},
    };

void MCScreenDC::setstatus(MCStringRef status)
{ //No action
}

Boolean MCScreenDC::open()
{
	WNDCLASSA  wc;  //window class

	// Fill in window class structure with parameters that describe
	// the MAIN window.
	//
	// MW-2006-01-04: This used to have CS_CLASSDC - but this causes problems
	//   with certain kinds of window. Also, given we always double-buffer rendering
	//   now, the only thing we have to (potentially) setup a DC with is a palette.
	wc.style         = CS_DBLCLKS;
	wc.lpfnWndProc   = (WNDPROC)MCWindowProc;
	wc.cbClsExtra    = 0;
	wc.cbWndExtra    = 4;
	wc.hInstance     = MChInst;
	wc.hIcon         = LoadIconA(MChInst, MAKEINTRESOURCEA(IDI_ICON1));
	wc.hCursor       = NULL;
	wc.hbrBackground = NULL;
	wc.lpszMenuName  = MC_APP_NAME;
	wc.lpszClassName = MC_WIN_CLASS_NAME;

	WNDCLASSW wwc;
	wwc.style         = CS_DBLCLKS;
	wwc.lpfnWndProc   = (WNDPROC)MCWindowProc;
	wwc.cbClsExtra    = 0;
	wwc.cbWndExtra    = 4;
	wwc.hInstance     = MChInst;
	wwc.hIcon         = LoadIconA(MChInst, MAKEINTRESOURCEA(IDI_ICON1));
	wwc.hCursor       = NULL;
	wwc.hbrBackground = NULL;
	wwc.lpszMenuName  = MC_APP_NAME_W;
	wwc.lpszClassName = MC_WIN_CLASS_NAME_W;

	// Register the window class and return success/failure code.
	if (RegisterClassW(&wwc) == 0)
		return FALSE;

	if (RegisterClassA(&wc) == 0)
		return FALSE;
	wc.style |= CS_SAVEBITS;
	wc.lpszClassName = MC_POPUP_WIN_CLASS_NAME;
	if (RegisterClassA(&wc) == 0)
		return FALSE;
	wc.lpszClassName = MC_MENU_WIN_CLASS_NAME;

	HINSTANCE tdll = LoadLibraryA("UxTheme.dll");
	if (tdll)
	{
		wc.style |= CS_DROPSHADOW;
		FreeLibrary(tdll);
	}
	if (RegisterClassA(&wc) == 0)
		return FALSE;
	// Define the VIDEO CLIP window. Has its own DC
	wc.style         = /*CS_OWNDC | */CS_VREDRAW | CS_HREDRAW;
	wc.lpfnWndProc   = (WNDPROC)MCPlayerWindowProc;
	wc.lpszClassName = MC_VIDEO_WIN_CLASS_NAME;  //video class
	if (RegisterClassA(&wc) == 0)
		return FALSE;

	// Define the QT VIDEO CLIP window
	wc.style         = 0/*CS_OWNDC | CS_VREDRAW | CS_HREDRAW*/;
	wc.lpfnWndProc   = (WNDPROC)MCQTPlayerWindowProc;
	wc.lpszClassName = MC_QTVIDEO_WIN_CLASS_NAME;  //video class
	if (RegisterClassA(&wc) == 0)
		return FALSE;

	// Define Snapshot window. Has its own window proc
	wc.style         = CS_DBLCLKS | CS_CLASSDC;
	wc.lpfnWndProc   = (WNDPROC)MCSnapshotWindowProc;
	wc.lpszClassName = MC_SNAPSHOT_WIN_CLASS_NAME;  //snapshot class
	if (RegisterClassA(&wc) == 0)
		return FALSE;

	// Define backdrop window class. Has its own window proc
	wc.style         = CS_CLASSDC;
	wc.lpfnWndProc   = (WNDPROC)MCBackdropWindowProc;
	wc.lpszClassName = MC_BACKDROP_WIN_CLASS_NAME;  //backdrop class
	if (RegisterClassA(&wc) == 0)
		return FALSE;

	f_src_dc = CreateCompatibleDC(NULL);
	f_dst_dc = CreateCompatibleDC(NULL);

	vis = new MCVisualInfo;

	ncolors = 0;
			redbits = greenbits = bluebits = 8;
			redshift = 16;
			greenshift = 8;
			blueshift = 0;
			vis->red_mask = 0x00FF0000;
			vis->green_mask = 0x0000FF00;
			vis->blue_mask = 0x000000FF;

	black_pixel.red = black_pixel.green = black_pixel.blue = 0;
	white_pixel.red = white_pixel.green = white_pixel.blue = 0xFFFF;
		black_pixel.pixel = 0;
		white_pixel.pixel = 0xFFFFFF;

	MCselectioncolor = MCpencolor = black_pixel;
	alloccolor(MCselectioncolor);
	alloccolor(MCpencolor);
	
	MConecolor = MCbrushcolor = white_pixel;
	alloccolor(MCbrushcolor);
	
	gray_pixel.red = gray_pixel.green = gray_pixel.blue = 0x8080;
	alloccolor(gray_pixel);
	
	MChilitecolor.red = MChilitecolor.green = 0x0000;
	MChilitecolor.blue = 0x8080;

	MCExecPoint ep;
	ep.setstaticcstring("HKEY_CURRENT_USER\\Control Panel\\Colors\\Hilight");
	MCS_query_registry(ep);
	if (ep.getsvalue().getlength())
	{
		char *cstring = ep.getsvalue().clone();
		char *sptr = cstring;
		do
		{
			if (*sptr == ' ')
				*sptr = ',';
		}
		while (*++sptr);
		parsecolor(cstring, &MChilitecolor, nil);
	}
	alloccolor(MChilitecolor);
	
	MCaccentcolor = MChilitecolor;
	alloccolor(MCaccentcolor);
	
	background_pixel.red = background_pixel.green = background_pixel.blue = 0xC0C0;

	if (MCmajorosversion > 400)
		ep.setstaticcstring("HKEY_CURRENT_USER\\Control Panel\\Colors\\MenuBar");
	else
		ep.setstaticcstring("HKEY_CURRENT_USER\\Control Panel\\Colors\\Menu");

	MCS_query_registry(ep);
	if (ep.getsvalue().getlength())
	{
		char *cstring = ep.getsvalue().clone();
		char *sptr = cstring;
		do
		{
			if (*sptr == ' ')
				*sptr = ',';
		}
		while (*++sptr);
		parsecolor(cstring, &background_pixel, nil);
	}
	alloccolor(background_pixel);

	SetBkMode(f_dst_dc, OPAQUE);
	SetBkColor(f_dst_dc, black_pixel . pixel);
	SetTextColor(f_dst_dc, white_pixel . pixel);
	SetBkMode(f_src_dc, OPAQUE);
	SetBkColor(f_src_dc, black_pixel . pixel);
	SetTextColor(f_src_dc, white_pixel . pixel);

	mousetimer = 0;
	grabbed = False;
	MCcursors[PI_NONE] = nil;

	//create an invisible main window for all the dlgs, secondary windows to use as their
	//parent window, so that they will not have icons show up in the desktop taskbar
	//use video class window which has it's own DC
	invisiblehwnd = CreateWindowA(MC_WIN_CLASS_NAME, "MCdummy",
	                             WS_POPUP, 0, 0, 8, 8,
	                             NULL, NULL, MChInst, NULL);
	mutex = CreateMutexA(NULL, False, NULL);

	MCblinkrate = GetCaretBlinkTime();
	MCdoubletime = GetDoubleClickTime();
	opened++;

	MCDisplay const *t_displays;
	getdisplays(t_displays, false);
	MCwbr = t_displays[0] . workarea;
	owndnd = False;

	// The System and Input codepages are used to translate input characters.
	// A keyboard layout will present characters via WM_CHAR in the
	// input_codepage, while Revolution is running in the system_codepage.
	//
	system_codepage = GetACP();

	char t_info[8];
	t_info[0] = '\0';
	GetLocaleInfoA(MAKELCID(LOWORD(GetKeyboardLayout(NULL)), SORT_DEFAULT), LOCALE_IDEFAULTANSICODEPAGE, t_info, 8);
	input_codepage = atoi(t_info);

	// OK-2007-08-03 : Bug 5265
	input_default_keyboard = GetKeyboardLayout(0);

	// Make sure we initialize the dragDelta property
	MCdragdelta = MCU_max(GetSystemMetrics(SM_CYDRAG), GetSystemMetrics(SM_CXDRAG));

	// MW-2009-12-10: Colorspace support
	PROFILE t_profile_info;
	t_profile_info . dwType = PROFILE_FILENAME;
	t_profile_info . pProfileData = "sRGB Color Space Profile.icm";
	t_profile_info . cbDataSize = strlen((char *)t_profile_info . pProfileData) + 1;
	m_srgb_profile = OpenColorProfileA(&t_profile_info, PROFILE_READ, FILE_SHARE_READ, OPEN_EXISTING);

	return True;
}

Boolean MCScreenDC::close(Boolean force)
{
	if (m_srgb_profile != nil)
	{
		CloseColorProfile(m_srgb_profile);
		m_srgb_profile = nil;
	}

	if (f_has_icon)
		configurestatusicon(0, nil, nil);

	disablebackdrop(true);
	disablebackdrop(false);
	finalisebackdrop();

	DeleteDC(f_src_dc);
	DeleteDC(f_dst_dc);

	if (m_printer_dc != NULL)
		DeleteDC(m_printer_dc);

	delete vis;
	opened = False;

	if (mousetimer != 0)
		if (MClowrestimers)
			KillTimer(NULL, mousetimer);
		else
			timeKillEvent(mousetimer);
	timeEndPeriod(1);
	opened = 0;
	if (dnddata != NULL)
	{
		dnddata->Release();
		dnddata = NULL;
	}
	return True;
}

MCNameRef MCScreenDC::getdisplayname()
{
	return MCN_local_win32;
}

void MCScreenDC::grabpointer(Window w)
{
	if (MCModeMakeLocalWindows())
	{
		if (w != nil)
			SetCapture((HWND)w->handle.window);
		grabbed = True;
	}
}

void MCScreenDC::ungrabpointer()
{
	if (MCModeMakeLocalWindows())
	{
		grabbed = False;
		ReleaseCapture();
	}
}

uint2 MCScreenDC::getwidth()
{
	HDC winhdc = GetDC(NULL);
	uint2 width = (uint2)GetDeviceCaps(winhdc, HORZRES);
	ReleaseDC(NULL, winhdc);
	return width;
}

uint2 MCScreenDC::getheight()
{
	HDC winhdc = GetDC(NULL);
	uint2 height = (uint2)GetDeviceCaps(winhdc, VERTRES);
	ReleaseDC(NULL, winhdc);
	return height;
}

uint2 MCScreenDC::getwidthmm()
{
	HDC winhdc = GetDC(NULL);
	uint2 widthmm = (uint2)GetDeviceCaps(winhdc, HORZSIZE);
	ReleaseDC(NULL, winhdc);
	return widthmm;
}

uint2 MCScreenDC::getheightmm()
{
	HDC winhdc = GetDC(NULL);
	uint2 heightmm = (uint2)GetDeviceCaps(winhdc, VERTSIZE);
	ReleaseDC(NULL, winhdc);
	return heightmm;
}

uint2 MCScreenDC::getdepth()
{
	return 32;
}

uint2 MCScreenDC::getmaxpoints()
{ //max points defined in a polygon
	return MAX_POINT_IN_POLYGON;
}

uint2 MCScreenDC::getvclass()
{
	return DirectColor;
}

// MW-2007-07-05: [[ Bug 471 ]] Modal/Sheets do not enable/disable windows correctly.
void MCScreenDC::openwindow(Window w, Boolean override)
{
	// MW-2009-11-01: Do nothing if there is no window
	if (w == NULL)
		return;

	if (override)
		ShowWindow((HWND)w->handle.window, SW_SHOWNA);
	else
		ShowWindow((HWND)w->handle.window, SW_SHOW);

	MCStack *t_stack;
	t_stack = MCdispatcher -> findstackd(w);
	if (t_stack != NULL)
	{
		if (t_stack -> getmode() == WM_SHEET || t_stack -> getmode() == WM_MODAL)
			MCstacks -> enableformodal(w, False);
	}
}

// MW-2007-07-05: [[ Bug 471 ]] Modal/Sheets do not enable/disable windows correctly.
void MCScreenDC::closewindow(Window w)
{
	// MW-2009-11-01: Do nothing if there is no window
	if (w == NULL)
		return;

	MCStack *t_stack;
	t_stack = MCdispatcher -> findstackd(w);

	// If we are a sheet or a modal dialog we need to ensure we enable the right windows
	// and activate the next obvious window.
	if (t_stack != NULL && (t_stack -> getmode() == WM_SHEET || t_stack -> getmode() == WM_MODAL))
	{
		// By default we will activate the (sheet's) parent...
		MCStack *t_parent;
		t_parent = MCdispatcher -> findstackd(t_stack -> getparentwindow());

		// However modals do not a have a parent so we search...
		MCStack *t_visual_parent;
		if (t_parent == NULL || !t_parent -> isvisible() || t_parent -> getwindow() == NULL)
		{
			MCStacknode *t_node;
			t_node = MCstacks -> topnode();

			t_visual_parent = NULL;

			// Iterate through the open stacks from top to bottom...
			if (t_node != NULL)
			{
				do
				{
					// Ignore the stack we are closing...
					if (t_node -> getstack() != t_stack)
					{
						// The stack should be activated if either:
						//   . it is a MODAL dialog, or
						//   . it is a SHEET dialog that has a MODAL in its ancestry
						//   . it has the topstack in its ancestry
						MCStack *t_parent;
						t_parent = t_node -> getstack();
						while(t_parent != NULL)
						{
							if (t_parent -> isvisible() && t_parent -> getwindow() != NULL)
							{
								if (t_parent -> getmode() == WM_MODAL || t_parent == MCtopstackptr)
								{
									t_visual_parent = t_node -> getstack();
									break;
								}
							}

							t_parent = MCdispatcher -> findstackd(t_parent -> getparentwindow());
						}

						if (t_visual_parent != NULL)
							break;
					}

					t_node = t_node -> next();
				}
				while(t_node != MCstacks -> topnode());
			}
		}
		else
			t_visual_parent = t_parent;
		
		// Ask the stacklist to iterate through and enable/disable windows as appropriate
		MCstacks -> enableformodal(w, True);

		// If the dialog is the active window then we have to ensure we activate
		// the next one in the chain - but atomically to stop window flicker!
		//
		// Here if t_parent_stack is NULL it means that there is no sensible
		// stack for us to fall back to. Therefore, we just fall through to
		// the default behaviour of just hiding the window.
		//
		if (GetActiveWindow() == (HWND)w -> handle . window && t_visual_parent != NULL)
		{
			HDWP t_dwp;
			t_dwp = BeginDeferWindowPos(2);
			DeferWindowPos(t_dwp, (HWND)w -> handle . window, NULL, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_HIDEWINDOW);
			DeferWindowPos(t_dwp, (HWND)t_visual_parent -> getwindow() -> handle . window, GetWindow((HWND)w -> handle . window, GW_HWNDPREV), 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
			EndDeferWindowPos(t_dwp);
			return;
		}
	}
	
	ShowWindow((HWND)w->handle.window, SW_HIDE);
}

void MCScreenDC::destroywindow(Window &w)
{
	// MW-2009-11-01: Do nothing if there is no window
	if (w == NULL)
		return;

	DestroyWindow((HWND)w->handle.window);
	delete w;
	w = DNULL;
}

void MCScreenDC::raisewindow(Window w)
{
	// MW-2009-11-01: Do nothing if there is no window
	if (w == NULL)
		return;

	if (w != nil)
		SetWindowPos((HWND)w->handle.window, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

}

void MCScreenDC::iconifywindow(Window w)
{
	// MW-2009-11-01: Do nothing if there is no window
	if (w == NULL)
		return;

	ShowWindow((HWND)w->handle.window, SW_MINIMIZE);
}

void MCScreenDC::uniconifywindow(Window w)
{
	// MW-2009-11-01: Do nothing if there is no window
	if (w == NULL)
		return;

	ShowWindow((HWND)w->handle.window, SW_RESTORE);
}

// MW-2007-07-09: [[ Bug 3226 ]] Updated to convert UTF-8 title to appropriate
//   encoding.
// MW-2008-07-22: [[ Bug 6235 ]] Make sure we deassociate any players temporarily
//   around the SetWindowText call. Failing to do this causes the call to only
//   set the first character as its treated as partially non-Unicode.
// MW-2008-09-01: [[ Bug 6235 ]] Unfortunately my nifty association/deassociation
//   logic doesn't work :o(. Therefore I'm removing this for now and have instead
//   added a bit to 'revRuntimeBehaviour' which allows turning off creation of
//   unicode windows.
void MCScreenDC::setname(Window w, MCStringRef newname)
{
	// MW-2009-11-01: Do nothing if there is no window
	if (w == NULL)
		return;

	if (IsWindowUnicode((HWND)w -> handle . window))
	{
		MCAutoStringRefAsWString t_newname_w;
		/* UNCHECKED */ t_newname_w . Lock(newname);
		SetWindowTextW((HWND)w -> handle . window, *t_newname_w);
	}
	else
	{
		MCAutoStringRefAsCString t_newname_a;
		/* UNCHECKED */ t_newname_a . Lock(newname);
		SetWindowTextA((HWND)w->handle.window, *t_newname_a);
	}
}

void MCScreenDC::setcmap(MCStack *sptr)
{}

void MCScreenDC::sync(Window w)
{
	GdiFlush();
}

static BOOL IsRemoteSession(void)
{
   return GetSystemMetrics(SM_REMOTESESSION);
}

void MCScreenDC::beep()
{
	bool t_use_internal = false ;
	if ( m_sound_internal != NULL ) 
		t_use_internal = (MCString(m_sound_internal) == "internal") == True;
	
	if ( t_use_internal || IsRemoteSession())
		Beep(beeppitch, beepduration);
	else 
		MessageBeep(-1);
}

void MCScreenDC::setinputfocus(Window w)
{
	SetFocus((HWND)w->handle.window);
}

void MCScreenDC::freepixmap(Pixmap &p)
{
	if (p != DNULL)
	{
		DeleteObject(p->handle.pixmap);
		delete p;
		p = DNULL;
	}
}

extern bool create_temporary_dib(HDC p_dc, uint4 p_width, uint4 p_height, HBITMAP& r_bitmap, void*& r_bits);
Pixmap MCScreenDC::createpixmap(uint2 width, uint2 height,
                                uint2 depth, Boolean purge)
{
	HBITMAP hbm;
	if (depth == 1)
		hbm = CreateBitmap(width, height, 1, 1, NULL);
	else
	{
		HDC windc = GetDC(NULL);
		void *bits;

		if (!create_temporary_dib(windc, width, height, hbm, bits))
			hbm = NULL;

		ReleaseDC(NULL, windc);
	}
	if (hbm == NULL)
		return DNULL;
	Pixmap pm = new _Drawable;
	pm->type = DC_BITMAP;
	pm->handle.pixmap = (MCSysBitmapHandle)hbm;
	return pm;
}

bool MCScreenDC::lockpixmap(Pixmap p_pixmap, void*& r_data, uint4& r_stride)
{
	BITMAP t_bitmap;

	GetObjectA(p_pixmap -> handle . pixmap, sizeof(BITMAP), &t_bitmap);
	if (t_bitmap . bmBits == NULL)
		return false;

	r_data = t_bitmap . bmBits;
	r_stride = t_bitmap . bmWidthBytes;

	return true;
}

void MCScreenDC::unlockpixmap(Pixmap p_pixmap, void *p_data, uint4 p_stride)
{
}

Pixmap MCScreenDC::createstipple(uint2 width, uint2 height, uint4 *bits)
{
	HBITMAP hbm = CreateBitmap(width, height, 1, 1, bits);
	if (hbm == NULL)
		return DNULL;
	Pixmap pm = new _Drawable;
	pm->type = DC_BITMAP;
	pm->handle.pixmap = (MCSysBitmapHandle)hbm;
	return pm;
}

Boolean MCScreenDC::getwindowgeometry(Window w, MCRectangle &drect)
{//get the client window's geometry in screen coord
	if (w == DNULL || w->handle.window == 0)
		return False;
	RECT wrect;
	GetClientRect((HWND)w->handle.window, &wrect);
	POINT p;
	p.x = wrect.left;
	p.y = wrect.top;
	ClientToScreen((HWND)w->handle.window, &p);
	drect.x = (int2)p.x;
	drect.y = (int2)p.y;
	drect.width = (uint2)(wrect.right - wrect.left);
	drect.height = (uint2)(wrect.bottom - wrect.top);
	return True;
}

Boolean MCScreenDC::getpixmapgeometry(Pixmap p, uint2 &w, uint2 &h, uint2 &d)
{
	if (p != DNULL)
	{
		BITMAP bm;
		if (!GetObjectA(p->handle.pixmap, sizeof(BITMAP), &bm))
			return False;
		w = (uint2)bm.bmWidth;
		h = (uint2)bm.bmHeight;
		d = bm.bmBitsPixel;
		return True;
	}
	return False;
}

void MCScreenDC::setgraphicsexposures(Boolean on, MCStack *sptr)
{
	exposures = on;
}

void MCScreenDC::copyarea(Drawable s, Drawable d, int2 depth, int2 sx, int2 sy,
                          uint2 sw, uint2 sh, int2 dx, int2 dy, uint4 rop)
{
	if (s == DNULL || d == DNULL)
		return;

	if (s == d && s->type == DC_WINDOW)
	{
		RECT wrect;
		wrect.left = sx;
		wrect.top = sy;
		wrect.right = sx + sw;
		wrect.bottom = sy + sh;
		if (ScrollWindowEx((HWND)s->handle.window, dx - sx, dy - sy, &wrect, NULL, NULL,
		                   NULL, exposures ? SW_INVALIDATE : 0) == COMPLEXREGION)
			expose();
	}
	else
	{
		HDC t_src_dc, t_dst_dc;
		HGDIOBJ t_old_src, t_old_dst;

		if (s -> type == DC_WINDOW)
			t_src_dc = GetDC((HWND)s -> handle . window); // Released
		else
			t_old_src = SelectObject(f_src_dc, s -> handle . pixmap), t_src_dc = f_src_dc;

		if (d -> type == DC_WINDOW)
			t_dst_dc = GetDC((HWND)d -> handle . window); // Released
		else
			t_old_dst = SelectObject(f_dst_dc, d -> handle . pixmap), t_dst_dc = f_dst_dc;

		BitBlt(t_dst_dc, dx, dy, sw, sh, t_src_dc, sx, sy, image_inks[rop]);

		if (d -> type == DC_WINDOW)
			ReleaseDC((HWND)d -> handle . window, t_dst_dc);
		else
			SelectObject(f_dst_dc, t_old_dst);

		if (s -> type == DC_WINDOW)
			ReleaseDC((HWND)s -> handle . window, t_src_dc);
		else
			SelectObject(f_src_dc, t_old_src);
	}
}

void MCScreenDC::copyplane(Drawable s, Drawable d, int2 sx, int2 sy,
                           uint2 sw, uint2 sh, int2 dx, int2 dy,
                           uint4 rop, uint4 pixel)
{
	HDC t_src_dc, t_dst_dc;
	HGDIOBJ t_old_src, t_old_dst;

	if (s == nil || d == nil)
		return;
	
	if (s -> type == DC_WINDOW)
		t_src_dc = GetDC((HWND)s -> handle . window); // Released
	else
		t_old_src = SelectObject(f_src_dc, s -> handle . pixmap), t_src_dc = f_src_dc;

	if (d -> type == DC_WINDOW)
		t_dst_dc = GetDC((HWND)s -> handle . window); // Released
	else
		t_old_dst = SelectObject(f_dst_dc, d -> handle . pixmap), t_dst_dc = f_dst_dc;

	int t_old_mode;
	t_old_mode = SetBkMode(f_dst_dc, OPAQUE);
	if (rop == GXand)
		MaskBlt(t_dst_dc, dx, dy, sw, sh, t_dst_dc, dx, dy, (HBITMAP)s->handle.pixmap, sx, sy, MAKEROP4(BLACKNESS, SRCCOPY));
	else
	{
		// MW-2011-07-15: [[ COLOR ]] The 'pixel' field of MCColor is now 0xXXRRGGBB
		//   universally. As Win32 uses 0xXXBBGGRR, we must flip.
		SetTextColor(t_dst_dc, (pixel & 0x00ff00) | ((pixel & 0x0000ff) << 16) | ((pixel & 0xff0000) >> 16));
		BitBlt(t_dst_dc, dx, dy, sw, sh, t_src_dc, sx, sy, image_inks[rop]);
		SetTextColor(t_dst_dc, white_pixel . pixel);
	}
	SetBkMode(f_dst_dc, t_old_mode);

	if (d -> type == DC_WINDOW)
		ReleaseDC((HWND)d -> handle . window, t_dst_dc);
	else
		SelectObject(f_dst_dc, t_old_dst);

	if (s -> type == DC_WINDOW)
		ReleaseDC((HWND)s -> handle . window, t_src_dc);
	else
		SelectObject(f_src_dc, t_old_src);
}

MCBitmap *MCScreenDC::createimage(uint2 depth, uint2 width, uint2 height, Boolean set, uint1 value, Boolean shm, Boolean forceZ)
{
	if (depth == 0)
		depth = getdepth();
	if (depth == 24)
		depth = 32;

	MCBitmap *image = new MCBitmap;
	image->width = width;
	image->height = height;
	image->format = ZPixmap;
	image->bitmap_unit = 32;
	image->byte_order = MSBFirst;
	image->bitmap_pad = 32;
	image->bitmap_bit_order = MSBFirst;
	image->depth = (uint1)depth;
	image->bytes_per_line = ((width * depth + 31) >> 3) & 0xFFFFFFFC;
	image->bits_per_pixel = (uint1)depth;
	image->red_mask = image->green_mask = image->blue_mask
	                                      = depth == 1 || depth == getdepth() ? 0x00 : 0xFF;
	image->data = NULL;
	image->bm = NULL;

	BITMAPINFO *bmi = NULL;
	if (depth < 16)
	{
		if (depth == 1)
		{
			bmi = (BITMAPINFO *)new char[sizeof(BITMAPINFOHEADER)
			                             + 2 * sizeof(RGBQUAD)];
			memset(bmi, 0, sizeof(BITMAPINFOHEADER));
			bmi->bmiColors[0].rgbRed = bmi->bmiColors[0].rgbGreen
			                           = bmi->bmiColors[0].rgbBlue = bmi->bmiColors[0].rgbReserved
			                                                         = bmi->bmiColors[1].rgbReserved = 0;
			bmi->bmiColors[1].rgbRed = bmi->bmiColors[1].rgbGreen
			                           = bmi->bmiColors[1].rgbBlue = 0xFF;
		}
		else
		{
			uint2 cells = MCU_min(1L << depth, ncolors);
			bmi = (BITMAPINFO *)new char[sizeof(BITMAPINFOHEADER)
			                             + cells * sizeof(uint2)];
			memset(bmi, 0, sizeof(BITMAPINFOHEADER));
			uint2 *dptr = (uint2 *)(&bmi->bmiColors[0]);
			if (depth == 1)
			{
				*dptr++ = 0;
				*dptr = 0xFF;
			}
			else
			{
				uint2 i;
				for (i = 0 ; i < cells ; i++)
					*dptr++ = i;
			}
		}
		bmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
		bmi->bmiHeader.biCompression = BI_RGB;
	}
	else
	{
		bmi = (BITMAPINFO *)new char[sizeof(BITMAPV4HEADER)];
		memset(bmi, 0, sizeof(BITMAPV4HEADER));
		bmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
		bmi->bmiHeader.biCompression = BI_BITFIELDS;
		BITMAPV4HEADER *b4 = (BITMAPV4HEADER *)bmi;
		if (depth == getdepth())
		{
			b4->bV4RedMask = vis->red_mask;
			b4->bV4GreenMask = vis->green_mask;
			b4->bV4BlueMask = vis->blue_mask;
		}
		else
		{
			b4->bV4RedMask = 0xFF0000;
			b4->bV4GreenMask = 0xFF00;
			b4->bV4BlueMask = 0xFF;
		}
	}
	bmi->bmiHeader.biWidth = width;
	bmi->bmiHeader.biHeight = -height;
	bmi->bmiHeader.biPlanes = 1;
	bmi->bmiHeader.biBitCount = depth;
	bmi->bmiHeader.biSizeImage = image->bytes_per_line * height;

	UINT iusage = DIB_RGB_COLORS;

	image->bm = (MCSysBitmapHandle)CreateDIBSection(f_dst_dc, bmi, iusage,
	                             (void **)&image->data, NULL, 0);
	delete bmi;

	if (image->data == NULL || image->bm == NULL)
		image->data = new char[image->bytes_per_line * height];
	if (set)
	{
		uint4 bytes = image->bytes_per_line * height;
		memset(image->data, value, bytes);
	}
	return image;
}

void MCScreenDC::destroyimage(MCBitmap *image)
{
	if (image->bm == NULL)
		delete image->data;
	else
		DeleteObject(image->bm);
	delete image;
}

MCBitmap *MCScreenDC::copyimage(MCBitmap *source, Boolean invert)
{
	MCBitmap *image = createimage(source->depth, source->width,
	                              source->height, 0, 0, False, False);
	uint4 bytes = image->bytes_per_line * image->height;
	if (invert)
	{
		uint1 *sptr = (uint1 *)source->data;
		uint1 *dptr = (uint1 *)image->data;
		while (bytes--)
			*dptr++ = ~*sptr++;
	}
	else
		memcpy(image->data, source->data, bytes);
	return image;
}

void MCScreenDC::putimage(Drawable dest, MCBitmap *source, int2 sx, int2 sy,
                          int2 dx, int2 dy, uint2 w, uint2 h)
{
	// Win32s doesn't support BitBlt for DIBs, and it's broken
	// in 8-bit mode in Windows 98
	HDC t_dst_dc;
	HGDIOBJ t_old_dst;
	
	if (dest == nil)
		return;

	if (dest -> type == DC_WINDOW)
		t_dst_dc = GetDC((HWND)dest -> handle . window);
	else
	{
		t_dst_dc = f_dst_dc;
		t_old_dst = SelectObject(f_dst_dc, dest -> handle . pixmap);
	}

	if (source->bm == NULL)
	{
		BITMAPINFO *bmi = NULL;
		if (source->depth < 16)
		{
			if (source->depth == 1)
			{
				bmi = (BITMAPINFO *)new char[sizeof(BITMAPINFOHEADER)
				                             + 2 * sizeof(RGBQUAD)];
				memset(bmi, 0, sizeof(BITMAPINFOHEADER));
				bmi->bmiColors[0].rgbRed = bmi->bmiColors[0].rgbGreen
				                           = bmi->bmiColors[0].rgbBlue = bmi->bmiColors[0].rgbReserved
				                                                         = bmi->bmiColors[1].rgbReserved = 0;
				bmi->bmiColors[1].rgbRed = bmi->bmiColors[1].rgbGreen
				                           = bmi->bmiColors[1].rgbBlue = 0xFF;
			}
			else
			{
				uint2 cells = MCU_min(1L << source->depth, ncolors);
				bmi = (BITMAPINFO *)new char[sizeof(BITMAPINFOHEADER)
				                             + cells * sizeof(uint2)];
				memset(bmi, 0, sizeof(BITMAPINFOHEADER));
				uint2 *dptr = (uint2 *)(&bmi->bmiColors[0]);
				if (source->depth == 1)
				{
					*dptr++ = 0;
					*dptr = 0xFF;
				}
				else
				{
					uint2 i;
					for (i = 0 ; i < cells ; i++)
						*dptr++ = i;
				}
			}
		}
		else
		{
			bmi = (BITMAPINFO *)new char[sizeof(BITMAPINFOHEADER)];
			memset(bmi, 0, sizeof(BITMAPINFOHEADER));
		}
		bmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
		bmi->bmiHeader.biWidth = source->width;
		bmi->bmiHeader.biHeight = source->height;
		bmi->bmiHeader.biPlanes = 1;
		bmi->bmiHeader.biBitCount = source->depth;
		bmi->bmiHeader.biCompression = BI_RGB;
		bmi->bmiHeader.biSizeImage = source->bytes_per_line * source->height;
		UINT iusage = DIB_RGB_COLORS;
		bmi->bmiHeader.biHeight = -source->height;

		SetDIBitsToDevice(t_dst_dc, dx, dy, w, h, sx, sy, sx, source->height, source->data, bmi, iusage);
		delete bmi;
		if (source->depth == 1)
			BitBlt(t_dst_dc, dx, dy, w, h, f_dst_dc, dx, dy, DSTINVERT);
	}
	else
	{
		HBITMAP obm = (HBITMAP)SelectObject(f_src_dc, source->bm);
		BitBlt(t_dst_dc, dx, dy, w, h, f_src_dc, sx, sy, SRCCOPY);
		SelectObject(f_src_dc, obm);
	}

	if (dest -> type == DC_WINDOW)
		ReleaseDC((HWND)dest -> handle . window, t_dst_dc);
	else
		SelectObject(f_dst_dc, t_old_dst);
}

MCBitmap *MCScreenDC::getimage(Drawable s, int2 x, int2 y, uint2 w, uint2 h, Boolean shm)
{
	MCBitmap *image = NULL;
	if (s == DNULL)
		s = getroot();
	if (s->type == DC_WINDOW)
	{
		image = createimage(getdepth(), w, h, False, 0, True, True);
		HBITMAP odbm = (HBITMAP)SelectObject(f_dst_dc, image->bm);
		HDC t_dc;
		t_dc = GetDC((HWND)s -> handle . window); // Released
		BitBlt(f_dst_dc, 0, 0, w, h, t_dc, x, y, SRCCOPY);
		ReleaseDC((HWND)s -> handle . window, t_dc);
		SelectObject(f_dst_dc, odbm);
	}
	else
	{
		uint2 bw, bh, d;
		
		getpixmapgeometry(s, bw, bh, d);

		image = createimage(d, w, h, False, 0, True, True);

		HBITMAP odbm = (HBITMAP)SelectObject(f_dst_dc, image->bm);
		HBITMAP osbm = (HBITMAP)SelectObject(f_src_dc, s->handle.pixmap);

		BitBlt(f_dst_dc, 0, 0, w, h, f_src_dc, x, y, SRCCOPY);

		SelectObject(f_src_dc, osbm);
		SelectObject(f_dst_dc, odbm);
	}
	return image;
}

static void fixdata(uint4 *bits, uint2 width, uint2 height)
{
	uint4 mask = width == 16 ? 0xFFFF0000 : 0xFF000000;
	while (height--)
		*bits++ |= mask;
}

uint4 MCScreenDC::dtouint4(Drawable d)
{
	if (d == DNULL)
		return 0;
	else
		if (d->type == DC_WINDOW)
			return (uint4)(d->handle.window);
		else
			return (uint4)(d->handle.pixmap);
}

Boolean MCScreenDC::uint4topixmap(uint4 id, Pixmap &p)
{
	p = new _Drawable;
	p->type = DC_BITMAP;
	p->handle.pixmap = (MCSysBitmapHandle)id;
	return True;
}

Boolean MCScreenDC::uint4towindow(uint4 id, Window &w)
{
	w = new _Drawable;
	w->type = DC_WINDOW;
	w->handle.window = (MCSysWindowHandle)id;
	return True;
}

void MCScreenDC::getbeep(uint4 property, int4& r_value)
{
	switch (property)
	{
	case P_BEEP_LOUDNESS:
		r_value = 100;
		break;
	case P_BEEP_PITCH:
		r_value = beeppitch;
		break;
	case P_BEEP_DURATION:
		r_value = beepduration;
		break;
	}
}

void MCScreenDC::setbeep(uint4 which, int4 beep)
{
	switch (which)
	{
	case P_BEEP_LOUDNESS:
		break;
	case P_BEEP_PITCH:
		if (beep == -1)
			beeppitch = 1440;
		else
			beeppitch = beep;
		break;
	case P_BEEP_DURATION:
		if (beep == -1)
			beepduration = 1000;
		else
			beepduration = beep;
		break;
	}
}

MCNameRef MCScreenDC::getvendorname(void)
{
	return MCN_win32;
}

uint2 MCScreenDC::getpad()
{
	return 32;
}

Window MCScreenDC::getroot()
{
	static Meta::static_ptr_t<_Drawable> mydrawable;
	if (mydrawable == DNULL)
		mydrawable = new _Drawable;
	mydrawable->type = DC_WINDOW;
	mydrawable->handle.window = (MCSysWindowHandle)GetDesktopWindow();
	return mydrawable;
}

MCBitmap *snapimage;
static Boolean snapdone;
static Boolean snapcancelled;
static Boolean rubbering;
static HDC snaphdc;
static HDC snapdesthdc;
static MCRectangle snaprect;
static int32_t snapoffsetx, snapoffsety;

typedef HRESULT (CALLBACK *DwmIsCompositionEnabledPtr)(BOOL *p_enabled);
static HMODULE s_dwmapi_library = NULL;
static DwmIsCompositionEnabledPtr s_dwm_is_composition_enabled = NULL;

static bool WindowsIsCompositionEnabled(void)
{
	if (MCmajorosversion < 0x0600)
		return false;

	if (s_dwmapi_library == NULL)
	{
		s_dwmapi_library = LoadLibraryA("dwmapi.dll");
		if (s_dwmapi_library == NULL)
			return false;

		s_dwm_is_composition_enabled = (DwmIsCompositionEnabledPtr)GetProcAddress(s_dwmapi_library, "DwmIsCompositionEnabled");

		if (s_dwm_is_composition_enabled == NULL)
		{
			FreeLibrary(s_dwmapi_library);
			s_dwmapi_library = NULL;
			return false;
		}
	}

	BOOL t_enabled;
	if (s_dwm_is_composition_enabled(&t_enabled) != S_OK)
		return false;

	return t_enabled != FALSE;
}

MCBitmap *MCScreenDC::snapshot(MCRectangle &r, uint4 window,
                               MCStringRef displayname)
{
	bool t_is_composited;
	t_is_composited = WindowsIsCompositionEnabled();

	expose();
	//make the parent window to be the invisible window, so that the snapshot window icon
	//does not show up in the desktop taskbar

	MCDisplay const *t_displays;
	uint4 t_display_count;
	MCRectangle t_virtual_viewport;
	t_display_count = getdisplays(t_displays, false);
	t_virtual_viewport = t_displays[0] . viewport;
	for(uint4 t_index = 1; t_index < t_display_count; ++t_index)
		t_virtual_viewport = MCU_union_rect(t_virtual_viewport, t_displays[t_index] . viewport);

	HWND hwndsnap = CreateWindowExA(WS_EX_TRANSPARENT | WS_EX_TOPMOST,
	                               MC_SNAPSHOT_WIN_CLASS_NAME,"", WS_POPUP, t_virtual_viewport . x, t_virtual_viewport . y,
	                               t_virtual_viewport . width, t_virtual_viewport . height, invisiblehwnd,
	                               NULL, MChInst, NULL);
	SetWindowPos(hwndsnap, HWND_TOPMOST,
	             t_virtual_viewport . x, t_virtual_viewport . y, t_virtual_viewport . width, t_virtual_viewport . height, SWP_NOACTIVATE | SWP_SHOWWINDOW
	             | SWP_DEFERERASE | SWP_NOREDRAW);

	if (t_is_composited)
	{
		snaphdc = GetDC(NULL);
		snapoffsetx = t_virtual_viewport . x;
		snapoffsety = t_virtual_viewport . y;
	}
	else
	{
		snaphdc = GetDC(hwndsnap); // Released
		snapoffsetx = 0;
		snapoffsety = 0;
	}

	snapdone = False;
	snapcancelled = False;
	snapdesthdc = f_dst_dc;
	if (window == 0 && r.x == -32768)
	{
		snapdone = False;
		rubbering = False;
		SetFocus(hwndsnap);
		setcursor(nil, MCcursors[PI_CROSS]);
		SetROP2(snaphdc, R2_NOT);
		SelectObject(snaphdc, GetStockObject(HOLLOW_BRUSH));
		MSG msg;
		while (!snapdone && GetMessageA(&msg, NULL, 0, 0))
		{
			TranslateMessage(&msg);
			DispatchMessageA(&msg);
		}
		r = snaprect;
	}
	MCBitmap *newimage = NULL;
	if (!snapcancelled)
	{
		if (r.x == -32768)
			r.x = r.y = 0;
		if (window != 0 || r.width == 0 || r.height == 0)
		{
			HWND w;
			if (window == 0)
			{
				POINT p;
				p.x = r.x;
				p.y = r.y;
				ShowWindow(hwndsnap, SW_HIDE);
				if ((w = WindowFromPoint(p)) == NULL)
				{
					ReleaseDC(hwndsnap, snaphdc);
					DestroyWindow(hwndsnap);
					return NULL;
				}
				ShowWindow(hwndsnap, SW_SHOWNA);
			}
			else
				w = (HWND)window;
			RECT cr;
			GetClientRect(w, &cr);
			POINT p;
			p.x = p.y = 0;
			ClientToScreen(w, &p);
			if (r.width == 0 || r.height == 0)
				MCU_set_rect(r, (int2)p.x, (int2)p.y,
							 (uint2)(cr.right), (uint2)(cr.bottom));
			else
			{
				r.x += (int2)p.x;
				r.y += (int2)p.y;
			}
		}
		r = MCU_clip_rect(r, t_virtual_viewport . x, t_virtual_viewport . y, t_virtual_viewport . width, t_virtual_viewport . height);
		if (r.width != 0 && r.height != 0)
		{
			snapimage = createimage(32, r.width, r.height, False, 0, False, False);
			if (snapimage != NULL)
			{
				HBITMAP obm = (HBITMAP)SelectObject(snapdesthdc, snapimage->bm);
				
				// MW-2012-09-19: [[ Bug 4173 ]] Add the 'CAPTUREBLT' flag to make sure
				//   layered windows are included.
				if (t_is_composited)
					BitBlt(snapdesthdc, 0, 0, r . width, r . height, snaphdc, r . x, r . y, CAPTUREBLT | SRCCOPY);
				else
				{
					BitBlt(snapdesthdc, 0, 0, r.width, r.height,
						   snaphdc, r.x - t_virtual_viewport . x, r.y - t_virtual_viewport . y, CAPTUREBLT | SRCCOPY);
				}
				SelectObject(snapdesthdc, obm);
				snapimage->red_mask = 0xFF;
				newimage = snapimage;
				snapimage = NULL;
			}
		}
	}
	SetWindowPos(hwndsnap, HWND_TOPMOST,
				 0, 0, getwidth(), getheight(), SWP_HIDEWINDOW
				 | SWP_DEFERERASE | SWP_NOREDRAW);
	if (t_is_composited)
		ReleaseDC(NULL, snaphdc);
	else
		ReleaseDC(hwndsnap, snaphdc);
	DestroyWindow(hwndsnap);
	return newimage;
}

LRESULT CALLBACK MCSnapshotWindowProc(HWND hwnd, UINT msg, WPARAM wParam,
                                      LPARAM lParam)
{
	static int2 sx, sy;

	switch (msg)
	{
	case WM_KEYDOWN:
		if (wParam == VK_ESCAPE)
		{
			if (rubbering)
				Rectangle(snaphdc, snaprect.x, snaprect.y,
				          snaprect.x + snaprect.width, snaprect.y + snaprect.height);
			snapdone = True;
			snapcancelled = True;
		}
		break;
	case WM_MOUSEMOVE:
		if (rubbering)
		{
			Rectangle(snaphdc, snaprect.x, snaprect.y,
			          snaprect.x + snaprect.width, snaprect.y + snaprect.height);
			snaprect = MCU_compute_rect(sx, sy, LOWORD(lParam) + snapoffsetx, HIWORD(lParam) + snapoffsety);
			Rectangle(snaphdc, snaprect.x, snaprect.y,
			          snaprect.x + snaprect.width, snaprect.y + snaprect.height);
		}
		break;
	case WM_LBUTTONDOWN:
		sx = LOWORD(lParam) + snapoffsetx;
		sy = HIWORD(lParam) + snapoffsety;
		snaprect = MCU_compute_rect(sx, sy, sx, sy);
		Rectangle(snaphdc, snaprect.x, snaprect.y,
		          snaprect.x + snaprect.width, snaprect.y + snaprect.height);
		rubbering = True;
		break;
	case WM_LBUTTONUP:
		if (rubbering)
		{
			Rectangle(snaphdc, snaprect.x, snaprect.y,
			          snaprect.x + snaprect.width, snaprect.y + snaprect.height);
			snaprect = MCU_compute_rect(sx, sy, LOWORD(lParam) + snapoffsetx, HIWORD(lParam) + snapoffsety);
			if (snaprect.width < 4 && snaprect.height < 4)
				snaprect.width = snaprect.height = 0;
			snapdone = True;
		}
		break;
	case WM_SETCURSOR:
		MCscreen -> setcursor(nil, MCcursors[PI_CROSS]);
		return 0;
	default:
		return DefWindowProcA(hwnd, msg, wParam, lParam);
	}
	return 0;
}

void MCScreenDC::settaskbarstate(bool p_visible)
{
	HWND taskbarwnd = FindWindowA("Shell_traywnd", "");
	SetWindowPos(taskbarwnd, 0, 0, 0, 0, 0, SWP_NOACTIVATE | (p_visible ? SWP_SHOWWINDOW : SWP_HIDEWINDOW));

	if (MCmajorosversion >= 0x0600)
	{
		HWND t_start_window;
		t_start_window = FindWindowExA(NULL, NULL, "Button", "Start");
		SetWindowPos(t_start_window, 0, 0, 0, 0, 0, SWP_NOACTIVATE | (p_visible ? SWP_SHOWWINDOW : SWP_HIDEWINDOW));
		if (p_visible)
		{
			InvalidateRect(t_start_window, NULL, TRUE);
			UpdateWindow(t_start_window);
		}
	}

	processdesktopchanged(false);
}

void MCScreenDC::processdesktopchanged(bool p_notify)
{
	const MCDisplay *t_monitors = NULL;
	MCDisplay *t_old_monitors = NULL;
	uint4 t_monitor_count, t_old_monitor_count;
	bool t_changed;

	t_old_monitor_count = s_monitor_count;
	t_old_monitors = s_monitor_displays;

	s_monitor_count = 0;
	s_monitor_displays = NULL;

	if (t_old_monitors != NULL)
	{
		t_monitor_count = ((MCScreenDC *)MCscreen) -> getdisplays(t_monitors, false);
		t_changed = t_monitor_count != t_old_monitor_count || memcmp(t_old_monitors, t_monitors, sizeof(MCDisplay) * t_monitor_count) != 0;
		delete[] t_old_monitors;
	}
	else
		t_changed = true;

	if (t_changed && (backdrop_active || backdrop_hard))
	{
		uint4 t_display_count;
		const MCDisplay *t_displays;

		t_display_count = getdisplays(t_displays, false);
		SetWindowPos(backdrop_window, NULL, t_displays[0] . workarea . x, t_displays[0] . workarea . y, t_displays[0] . workarea . width, t_displays[0] . workarea . height, 0);
	}

	if (p_notify && t_changed)
		MCscreen -> delaymessage(MCdefaultstackptr -> getcurcard(), MCM_desktop_changed);
}

void MCScreenDC::showtaskbar()
{
	if (!taskbarhidden && !(backdrop_active || backdrop_hard)) //if the menu bar is already showing, bail out
		return;

	taskbarhidden = False;
	settaskbarstate(true);
}

void MCScreenDC::hidetaskbar()
{
	if (taskbarhidden && !(backdrop_active || backdrop_hard))
		return;

	taskbarhidden = True;
	settaskbarstate(false);
}

// MW-2006-05-26: [[ Bug 3642 ]] Changed global allocation routines to use malloc
HRGN MCScreenDC::PixmapToRegion(Pixmap p)
{
	HRGN hRgn = NULL;
	COLORREF cTransparentColor = 0xFFFFFF;
	HBITMAP hBmp = (HBITMAP)p->handle.pixmap;
	COLORREF cTolerance = 0x101010;
	if (hBmp)
	{
		// Create a memory DC inside which we will scan the bitmap content
		HDC hMemDC = CreateCompatibleDC(NULL);
		if (hMemDC)
		{
			// Get bitmap size
			BITMAP bm;
			GetObjectA(hBmp, sizeof(bm), &bm);

			// Create a 32 bits depth bitmap and select it into the memory DC
			BITMAPINFOHEADER bmi = {
			                           sizeof(BITMAPINFOHEADER), bm.bmWidth, bm.bmHeight, 1, 32, BI_RGB,
			                           0, 0, 0, 0, 0 };
			VOID * pbits32;
			HBITMAP hbm32 = CreateDIBSection(hMemDC, (BITMAPINFO *)&bmi,
			                                 DIB_RGB_COLORS, &pbits32, NULL, 0);
			if (hbm32)
			{
				HBITMAP holdBmp = (HBITMAP)SelectObject(hMemDC, hbm32);
				HDC hDC = CreateCompatibleDC(hMemDC);
				if (hDC)
				{
					// Get how many bytes per row we have for the bitmap bits (rounded up
					// to 32 bits)
					BITMAP bm32;
					GetObjectA(hbm32, sizeof(bm32), &bm32);
					while (bm32.bmWidthBytes % 4)
						bm32.bmWidthBytes++;

					// Copy the bitmap into the memory DC
					HBITMAP holdBmp = (HBITMAP)SelectObject(hDC, hBmp);
					BitBlt(hMemDC, 0, 0, bm.bmWidth, bm.bmHeight, hDC, 0, 0, SRCCOPY);
					// For better performances, we will use the ExtCreateRegion() function
					// to create the region. This function take a RGNDATA structure on
					// entry. We will add rectangles by amount of ALLOC_UNIT number in
					// this structure.
#define ALLOC_UNIT 100

					DWORD maxRects = ALLOC_UNIT;
					RGNDATA *pData;
					pData = (RGNDATA *)malloc(sizeof(RGNDATAHEADER) + (sizeof(RECT) * maxRects));
					if (pData == NULL)
						goto no_mem_error;

					pData->rdh.dwSize = sizeof(RGNDATAHEADER);
					pData->rdh.iType = RDH_RECTANGLES;
					pData->rdh.nCount = pData->rdh.nRgnSize = 0;
					SetRect(&pData->rdh.rcBound, MAXLONG, MAXLONG, 0, 0);

					// Keep on hand highest and lowest values for the "transparent" pixels
					BYTE lr = GetRValue(cTransparentColor);
					BYTE lg = GetGValue(cTransparentColor);
					BYTE lb = GetBValue(cTransparentColor);
					BYTE hr = MCU_min(0xff, lr + GetRValue(cTolerance));
					BYTE hg = MCU_min(0xff, lg + GetGValue(cTolerance));
					BYTE hb = MCU_min(0xff, lb + GetBValue(cTolerance));

					// Scan each bitmap row from bottom to top (the bitmap is
					// inverted vertically)
					BYTE *p32 = (BYTE *)bm32.bmBits + (bm32.bmHeight - 1)
					            * bm32.bmWidthBytes;
					for (int y = 0; y < bm.bmHeight; y++)
					{
						// Scan each bitmap pixel from left to right
						for (int x = 0; x < bm.bmWidth; x++)
						{
							// Search for a continuous range of "non transparent pixels"
							int x0 = x;
							LONG *p = (LONG *)p32 + x;
							while (x < bm.bmWidth)
							{
								BYTE b = GetRValue(*p);
								if (b >= lr && b <= hr)
								{
									b = GetGValue(*p);
									if (b >= lg && b <= hg)
									{
										b = GetBValue(*p);
										if (b >= lb && b <= hb)
											// This pixel is "transparent"
											break;
									}
								}
								p++;
								x++;
							}

							if (x > x0)
							{
								// Add the pixels (x0, y) to (x, y+1) as a new
								// rectangle in the region
								if (pData->rdh.nCount >= maxRects)
								{
									maxRects += ALLOC_UNIT;
									pData = (RGNDATA *)realloc(pData, sizeof(RGNDATAHEADER) + (sizeof(RECT) * maxRects));
									if (pData == NULL)
										goto no_mem_error;
								}
								RECT *pr = (RECT *)&pData->Buffer;
								SetRect(&pr[pData->rdh.nCount], x0, y, x, y+1);
								if (x0 < pData->rdh.rcBound.left)
									pData->rdh.rcBound.left = x0;
								if (y < pData->rdh.rcBound.top)
									pData->rdh.rcBound.top = y;
								if (x > pData->rdh.rcBound.right)
									pData->rdh.rcBound.right = x;
								if (y+1 > pData->rdh.rcBound.bottom)
									pData->rdh.rcBound.bottom = y+1;
								pData->rdh.nCount++;

								// On Windows98, ExtCreateRegion() may fail if the
								// number of rectangles is too large (ie: >
								// 4000). Therefore, we have to create the region by
								// multiple steps.
								if (pData->rdh.nCount == 2000)
								{
									HRGN h = ExtCreateRegion(NULL, sizeof(RGNDATAHEADER)
									                         + (sizeof(RECT) * maxRects), pData);
									if (hRgn)
									{
										CombineRgn(hRgn, hRgn, h, RGN_OR);
										DeleteObject(h);
									}
									else
										hRgn = h;
									pData->rdh.nCount = 0;
									SetRect(&pData->rdh.rcBound, MAXLONG, MAXLONG, 0, 0);
								}
							}
						}

						// Go to next row (remember, the bitmap is inverted vertically)
						p32 -= bm32.bmWidthBytes;
					}

					// Create or extend the region with the remaining rectangles
					HRGN h = ExtCreateRegion(NULL, sizeof(RGNDATAHEADER)
					                         + (sizeof(RECT) * maxRects), pData);
					if (hRgn)
					{
						CombineRgn(hRgn, hRgn, h, RGN_OR);
						DeleteObject(h);
					}
					else
						hRgn = h;

					// Clean up
no_mem_error:
					if (pData != NULL)
						free(pData);
					else if (hRgn != NULL)
					{
						DeleteObject(hRgn);
						hRgn = NULL;
					}

					SelectObject(hDC, holdBmp);
					DeleteDC(hDC);
				}

				DeleteObject(SelectObject(hMemDC, holdBmp));
			}

			DeleteDC(hMemDC);
		}
	}
	return hRgn;
}

void MCScreenDC::enablebackdrop(bool p_hard)
{
	if (!MCModeMakeLocalWindows())
		return;
	
	bool t_error;
	t_error = false;
	
	if (p_hard && backdrop_hard)
		return;

	if (!p_hard && backdrop_active)
		return;
	
	if (p_hard)
		backdrop_hard = true;
	else
		backdrop_active = True;
		
	if (backdrop_window != NULL)
	{
		finalisebackdrop();
		backdrop_window = NULL;
	}

	if (backdrop_window == NULL)
		t_error = !initialisebackdrop();
	
	if (!t_error)
	{
		InvalidateRect(backdrop_window, NULL, TRUE);
		ShowWindow(backdrop_window, SW_SHOW);
	}
	else
	{
		backdrop_active = false;
		finalisebackdrop();
	}
}

void MCScreenDC::disablebackdrop(bool p_hard)
{
	if (!MCModeMakeLocalWindows())
		return;
	
	if (!backdrop_hard && p_hard)
		return;

	if (!backdrop_active && !p_hard)
		return;

	if (p_hard)
		backdrop_hard = false;
	else
		backdrop_active = False;

	if (!backdrop_active && !backdrop_hard)
	{
		ShowWindow(backdrop_window, SW_HIDE);
	}
	else
		InvalidateRect(backdrop_window, NULL, TRUE);
}

void MCScreenDC::configurebackdrop(const MCColor& p_colour, Pixmap p_pattern, MCImage *p_badge)
{
	if (backdrop_badge != p_badge || backdrop_pattern != p_pattern || backdrop_colour . red != p_colour . red || backdrop_colour . green != p_colour . green || backdrop_colour . blue != p_colour . blue)
	{
		backdrop_badge = p_badge;
		backdrop_pattern = p_pattern;
		backdrop_colour = p_colour;
	
		alloccolor(backdrop_colour);
	
		if (backdrop_active || backdrop_hard)
			InvalidateRect(backdrop_window, NULL, TRUE);
	}
}

void MCScreenDC::assignbackdrop(Window_mode p_mode, Window p_window)
{
}

bool MCScreenDC::initialisebackdrop(void)
{
	uint4 t_display_count;
	const MCDisplay *t_displays;

	t_display_count = getdisplays(t_displays, false);

	// MW-2012-10-08: [[ Bug 10286 ]] If the taskbar is hidden then show it and then
	//   present the backdrop as fullscreen.
	if (taskbarhidden)
	{
		settaskbarstate(true);
		taskbarhidden = false;
		backdrop_window = CreateWindowExA(0, MC_BACKDROP_WIN_CLASS_NAME, "", WS_POPUP, t_displays[0] . viewport . x, t_displays[0] . viewport . y, t_displays[0] . viewport . width, t_displays[0] . viewport . height, invisiblehwnd, NULL, MChInst, NULL);
	}
	else
	{
		int32_t t_height;
		if (t_displays[0] . workarea . height == t_displays[0] . viewport . height)
			t_height = t_displays[0] . viewport . height - 2;
		else
			t_height = t_displays[0] . workarea . height;
		backdrop_window = CreateWindowExA(0, MC_BACKDROP_WIN_CLASS_NAME, "", WS_POPUP, t_displays[0] . workarea . x, t_displays[0] . workarea . y, t_displays[0] . workarea . width, t_height, invisiblehwnd, NULL, MChInst, NULL);
	}

	return true;
}

void MCScreenDC::finalisebackdrop(void)
{
	if (backdrop_window != NULL)
	{
		DestroyWindow(backdrop_window);
		backdrop_window = NULL;
	}
}

void MCScreenDC::redrawbackdrop(void)
{
	_Drawable t_drawable;		
	MCContext *t_context;
	t_drawable . type = DC_WINDOW;
	t_drawable . handle . window = (MCSysWindowHandle)backdrop_window;
	t_context = MCscreen -> createcontext(&t_drawable);

	t_context -> setforeground(backdrop_colour);

	if (backdrop_pattern != NULL)
		t_context -> setfillstyle(FillTiled, backdrop_pattern, 0, 0);
	else
		t_context -> setfillstyle(FillSolid, NULL, 0, 0);

	t_context -> fillrect(t_context -> getclip());

	if (backdrop_badge != NULL && backdrop_hard)
	{
		MCRectangle t_rect;
		t_rect = backdrop_badge -> getrect();
		backdrop_badge -> drawme(t_context, 0, 0, t_rect . width, t_rect . height, 32, getheight() - 32 - t_rect . height);
	}

	MCscreen -> freecontext(t_context);
}

void MCScreenDC::enactraisewindows(void)
{
}

LRESULT CALLBACK MCBackdropWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	uint4 mbutton;

	switch(msg)
	{
		case WM_ERASEBKGND:
		{
			((MCScreenDC *)MCscreen) -> redrawbackdrop();
		}
		return 1;

		case WM_WINDOWPOSCHANGING:
		{
			((MCScreenDC *)MCscreen) -> restackwindows(hwnd, msg, wParam, lParam);
			return DefWindowProcA(hwnd, msg, wParam, lParam);
		}

		case WM_LBUTTONDOWN:
		case WM_MBUTTONDOWN:
		case WM_RBUTTONDOWN:
			if (MCtopstackptr != NULL && MCtopstackptr -> getwindow() != NULL && IsWindowVisible((HWND)MCtopstackptr -> getwindow() -> handle . window))
				SetActiveWindow((HWND)MCtopstackptr -> getwindow() -> handle . window);
			else
			{
				MCStacknode *t_node;
				t_node = MCstacks -> topnode();
				do
				{
					if (t_node -> getstack() -> getwindow() != NULL && IsWindowVisible((HWND)t_node -> getstack() -> getwindow() -> handle . window))
					{
						SetActiveWindow((HWND)t_node -> getstack() -> getwindow() -> handle . window);
						break;
					}

					t_node = t_node -> next();
				}
				while(t_node != MCstacks -> topnode());
			}

			if (msg == WM_LBUTTONDOWN)
				mbutton = 1;
			else if (msg = WM_MBUTTONDOWN)
				mbutton = 2;
			else
				mbutton = 3;
			MCdefaultstackptr->getcard()->message_with_args(MCM_mouse_down_in_backdrop, mbutton);
		return 0;
	
		case WM_LBUTTONUP:
		case WM_MBUTTONUP:
		case WM_RBUTTONUP:
			if (msg == WM_LBUTTONUP)
				mbutton = 1;
			else if (msg = WM_MBUTTONUP)
				mbutton = 2;
			else
				mbutton = 3;
			MCdefaultstackptr->getcard()->message_with_args(MCM_mouse_up_in_backdrop, mbutton);
		return 0;

		case WM_SETCURSOR:
		{
			// MW-2008-01-11: [[ Bug 4336 ]] Resize cursor sticks over backdrop on Windows
			MCCursorRef t_cursor;
			if (MCwatchcursor)
				t_cursor = MCcursors[PI_WATCH];
			else if (MCcursor != None)
				t_cursor = MCcursor;
			else if (MCdefaultcursorid)
				t_cursor = MCdefaultcursor;
			else
				t_cursor = MCcursors[PI_HAND];
			MCscreen -> setcursor(NULL, t_cursor);
		}
		return 0;
	}

	return DefWindowProcA(hwnd, msg, wParam, lParam);
}

