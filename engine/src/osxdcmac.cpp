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

#include "core.h"
#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"

#include "execpt.h"
#include "dispatch.h"
#include "stack.h"
#include "card.h"
#include "stacklst.h"
#include "field.h"
#include "util.h"
#include "player.h"
#include "sellst.h"
#include "button.h"
#include "group.h"
#include "execpt.h"
#include "globals.h"
#include "debug.h"
#include "image.h"
#include "context.h"
#include "notify.h"
#include "osspec.h"
#include "unicode.h"

#include "mctheme.h"
#include "menuparse.h"
#include "mode.h"

#include "osxdc.h"

#include "resolution.h"

extern "C"
{
	OSStatus HMGetHelpMenu(MenuRef *outHelpMenu,
	                       MenuItemIndex *outFirstCustomItemIndex);
}

extern Boolean tripleclick;

extern bool WindowIsInControlGroup(WindowRef p_window);

//was statically defined in MCScreenDC class in macdc.h
//used by the menu routines below, and in button.cpp

uint2 MCScreenDC::submenuIDs[SUB_MENU_LAST_ID];



static uint4 keysyms[] = {
                             0x61, 0x73, 0x64, 0x66, 0x68, 0x67, 0x7A, 0x78, 0x63, 0x76, 0, 0x62,
                             0x71, 0x77, 0x65, 0x72, 0x79, 0x74, 0x31, 0x32, 0x33, 0x34, 0x36,
                             0x35, 0x3D, 0x39, 0x37, 0x2D, 0x38, 0x30, 0x5D, 0x6F, 0x75, 0x5B,
                             0x69, 0x70, 0xFF0D, 0x6C, 0x6A, 0x27, 0x6B, 0x3B, 0x5C, 0x2C, 0x2F,
                             0x6E, 0x6D, 0x2E, 0xFF09, 0x20, 0x60, 0xFF08, 0xFF8D, 0xFF1B, 0, 0,
                             0xFFE1, 0xFFE5, 0, 0xFFE3, 0, 0, 0, 0, 0, 0xFF9F, 0, 0xFFAA, 0,
                             0xFFAB, 0, 0xFF7F, 0, 0, 0, 0xFFAF, 0xFF8D, 0, 0xFFAD, 0, 0, 0xFFD5,
                             0xFF9E, 0xFF9C, 0xFF99, 0xFF9B, 0xFF96, 0xFF9D, 0xFF98, 0xFF95, 0,
                             0xFF97, 0xFF9A, 0, 0, 0, 0xFFC2, 0xFFC3, 0xFFC4, 0xFFC0, 0xFFC5,
                             0xFFC6, 0, 0xFFC8, 0, 0xFFCA, 0xFFCD, 0xFF14, 0, 0xFFC7, 0, 0xFFC9, 0,
                             0xFF13, 0x1004FF6A, 0xFF50, 0xFF55, 0xFFFF, 0xFFC1, 0xFF57, 0xFFBF,
                             0xFF56, 0xFFBE, 0xFF51, 0xFF53, 0xFF54, 0xFF52, 0, 0, 0, 0, 0, 0, 0,
                             0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                             0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                             0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                             0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
                         };

static uint4 shift_keysyms[] = {
                                   0x41, 0x53, 0x44, 0x46, 0x48, 0x47, 0x5A, 0x58, 0x43, 0x56, 0, 0x42,
                                   0x51, 0x57, 0x45, 0x52, 0x59, 0x54, 0x21, 0x40, 0x23, 0x24, 0x5E,
                                   0x25, 0x2B, 0x28, 0x26, 0x5F, 0x2A, 0x29, 0x7D, 0x4F, 0x55, 0x7B,
                                   0x49, 0x50, 0xFF0D, 0x4C, 0x4A, 0x22, 0x4B, 0x3A, 0x7C, 0x3C, 0x3F,
                                   0x4E, 0x4D, 0x3E, 0xFF09, 0x20, 0x7E, 0xFF08, 0xFF8D, 0xFF1B, 0, 0,
                                   0xFFE1, 0xFFE5, 0, 0xFFE3, 0, 0, 0, 0, 0, 0xFFAE, 0, 0xFFAA, 0,
                                   0xFFAB, 0, 0xFF7F, 0, 0, 0, 0xFFAF, 0xFF8D, 0, 0xFFAD, 0, 0, 0xFFD5,
                                   0xFFB0, 0xFFB1, 0xFFB2, 0xFFB3, 0xFFB4, 0xFFB5, 0xFFB6, 0xFFB7, 0,
                                   0xFFB8, 0xFFB9, 0, 0, 0, 0xFFC2, 0xFFC3, 0xFFC4, 0xFFC0, 0xFFC5,
                                   0xFFC6, 0, 0xFFC8, 0, 0xFF62, 0, 0xFF20, 0, 0xFFC7, 0, 0xFFC9, 0,
                                   0xFF6B, 0x1004FF6A, 0xFF50, 0xFF55, 0xFFFF, 0xFFC1, 0xFF57, 0xFFBF,
                                   0xFF56, 0xFFBE, 0xFF51, 0xFF53, 0xFF54, 0xFF52, 0, 0, 0, 0, 0, 0, 0,
                                   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
                               };

typedef struct
{
	uint4 keysym;
	uint2 glyph;
}
KeyGlyph;

static KeyGlyph key_glyphs[] =
	{
		{XK_F1, kMenuF1Glyph},
		{XK_F2, kMenuF2Glyph},
		{XK_F3, kMenuF3Glyph},
		{XK_F4, kMenuF4Glyph},
		{XK_F5, kMenuF5Glyph},
		{XK_F6, kMenuF6Glyph},
		{XK_F7, kMenuF7Glyph},
		{XK_F8, kMenuF8Glyph},
		{XK_F9, kMenuF9Glyph},
		{XK_F10, kMenuF10Glyph},
		{XK_F11, kMenuF11Glyph},
		{XK_F12, kMenuF12Glyph},
		{XK_F13, kMenuF13Glyph},
		{XK_F14, kMenuF14Glyph},
		{XK_F15, kMenuF15Glyph},
        {XK_BackSpace, kMenuDeleteLeftGlyph},
		{XK_Tab, kMenuTabRightGlyph},
		{XK_Return, kMenuReturnGlyph},
        {XK_Escape, kMenuEscapeGlyph},
        {XK_Delete, kMenuDeleteRightGlyph},
		{XK_Home, kMenuNorthwestArrowGlyph},
        {XK_Left, kMenuLeftArrowGlyph},
        {XK_Up, kMenuUpArrowGlyph},
        {XK_Right, kMenuRightArrowGlyph},
        {XK_Down, kMenuDownArrowGlyph},
        {XK_End, kMenuSoutheastArrowGlyph},
        {XK_Prior, kMenuPageUpGlyph},
        {XK_Next, kMenuPageDownGlyph},
        {XK_osfHelp, kMenuHelpGlyph},
        {XK_KP_Enter, kMenuEnterGlyph},
		{XK_space, kMenuSpaceGlyph},
        {0, kMenuNullGlyph},
    };
//        {XK_Linefeed, "linefeed"},
//        {XK_Clear, "clear"},
//        {XK_Pause, "pause"},
//        {XK_Scroll_Lock, "scroll_lock"},
//        {XK_Sys_Req, "sys_req"},
//        {XK_Begin, "begin"},
//        {XK_Select, "select"},
//        {XK_Print, "print"},
//        {XK_Execute, "execute"},
//        {XK_Insert, "insert"},
//        {XK_Undo, "undo"},
//        {XK_Redo, "redo"},
//        {XK_Menu, "menu"},
//        {XK_Find, "find"},
//        {XK_Cancel, "cancel"},
//        {XK_Break, "break"},
//        {XK_Mode_switch, "mode_switch"},
//        {XK_script_switch, "script_switch"},
//        {XK_Num_Lock, "num_lock"},
//        {XK_KP_Space, "kp_space"},
//        {XK_KP_Tab, "kp_tab"},
//        {XK_KP_F1, "kp_f1"},
//        {XK_KP_F2, "kp_f2"},
//        {XK_KP_F3, "kp_f3"},
//        {XK_KP_F4, "kp_f4"},
//        {XK_KP_Begin, "kp_begin"},
//        {XK_KP_Home, "kp_home"},
//        {XK_KP_Left, "kp_left"},
//        {XK_KP_Up, "kp_up"},
//        {XK_KP_Right, "kp_right"},
//        {XK_KP_Down, "kp_down"},
//        {XK_KP_End, "kp_end"},
//        {XK_KP_Insert, "kp_insert"},
//        {XK_KP_Delete, "kp_delete"},
//        {XK_KP_Prior, "kp_page_up"},
//        {XK_KP_Next, "kp_page_down"},
//        {XK_KP_Equal, "kp_equal"},
//        {XK_KP_Multiply, "kp_multiply"},
//        {XK_KP_Add, "kp_add"},
//        {XK_KP_Separator, "kp_separator"},
//        {XK_KP_Subtract, "kp_subtract"},
//        {XK_KP_Decimal, "kp_decimal"},
//        {XK_KP_Divide, "kp_divide"},
//        {XK_KP_0, "kp_0"},
//        {XK_KP_1, "kp_1"},
//        {XK_KP_2, "kp_2"},
//        {XK_KP_3, "kp_3"},
//        {XK_KP_4, "kp_4"},
//        {XK_KP_5, "kp_5"},
//        {XK_KP_6, "kp_6"},
//        {XK_KP_7, "kp_7"},
//        {XK_KP_8, "kp_8"},
//        {XK_KP_9, "kp_9"},

void dohilitewindow(WindowRef p_window, Boolean p_hilite)
{
	// MW-2010-12-05: [[ Bug 9209 ]] Make sure we don't try to hilite windows is the
	//   app isn't active.
	if (!MCappisactive)
		p_hilite = False;
	
	if (MCmajorosversion < 0x1050)
	{
		HiliteWindow(p_window, p_hilite);
		return;
	}
	
	_Drawable t_drawable;
	t_drawable . type = DC_WINDOW;
	t_drawable . handle . window = (MCSysWindowHandle)p_window;
	
	MCStack *t_stack;
	t_stack = MCdispatcher -> findstackd(&t_drawable);
	if (t_stack == NULL || !t_stack -> ismetal())
	{
		HiliteWindow(p_window, p_hilite);
		return;
	}
}

// MW-2010-11-28: [[ Bug 3604 ]] Removed the various Classic specific adjustments.
static void dozoomwindow(WindowPtr win, short zoomInOrOut)
{
	GDHandle gdNthDevice, gdZoomOnThisDevice;
	CGrafPtr oldport;
	GDHandle olddevice;
	GetGWorld(&oldport, &olddevice);
	SetGWorld(GetWindowPort(win), GetMainDevice());
	Rect windRect;
	
	GetPortBounds(GetWindowPort(win), &windRect);
	
	if (zoomInOrOut == inZoomOut)
	{  //zooming to standard state

		LocalToGlobal((Point*)&windRect.top); //convert to global coordinates
		LocalToGlobal((Point*)&windRect.bottom);
		RgnHandle r = NewRgn();
		GetWindowRegion(win, kWindowTitleBarRgn, r);
		Rect tbRect;
		GetRegionBounds(r, &tbRect);
		DisposeRgn(r);
		int2 wTitleHeight = tbRect.bottom - tbRect.top;
		/*-------------------------------------------------------------------------*/
		long  sectArea = 0;
		long greatestArea = 0;
		//check window against all gdRects in gDevice list
		for (gdNthDevice = GetDeviceList(); gdNthDevice != NULL;
		        gdNthDevice = GetNextDevice(gdNthDevice))
		{
			HLock((Handle)gdNthDevice);
			//if this device is screen divice and is active
			if (TestDeviceAttribute(gdNthDevice, screenDevice)
			        && TestDeviceAttribute(gdNthDevice, screenActive))
			{
				//do window rectangle and this gDevice rectangle intersect?
				Rect sectRect;
				SectRect(&windRect, &((*gdNthDevice)->gdRect), &sectRect);
				//determine which screen holds greatest window area.
				sectArea = (long)(sectRect.right - sectRect.left)
				           * (sectRect.bottom - sectRect.top);
				if ( sectArea > greatestArea)
				{
					greatestArea = sectArea;            //set greatest area so far
					gdZoomOnThisDevice = gdNthDevice;   //set zoom device
				}
			}
			HUnlock((Handle)gdNthDevice);
		}
		int2 mbarheight = gdZoomOnThisDevice == GetMainDevice()
		                  ? GetMBarHeight() : 0;
		HLock((Handle)gdZoomOnThisDevice);
		Rect srect;
		if (mbarheight)
		{ // On main screen?  Use windowBoundingRect.
			srect.left = MCwbr.x;
			srect.top = MCwbr.y;
			srect.right = MCwbr.x + MCwbr.width;
			srect.bottom = MCwbr.y + MCwbr.height;
		}
		else
		{
			srect = (*gdZoomOnThisDevice)->gdRect;
			srect.top += mbarheight;
		}
		HUnlock((Handle)gdZoomOnThisDevice);
		_Drawable d;
		d.type = DC_WINDOW;
		d.handle.window = (MCSysWindowHandle)win;
		MCStack *sptr = MCdispatcher->findstackd(&d);
		Rect stackMinMaxSize;
		sptr->getminmax(&stackMinMaxSize); //get stack's max size
		/* stackMinMaxSize.right is the width of the max stack size
		    * stackMinMaxSize.bottom is the height of the max stack size
		    * we need to see if either max size's width or height 
		    * runs off the screen, if it does, we need to move the stack
		    * to very top left of the screen. */
		int2 w = MCU_min(stackMinMaxSize.right, srect.right - srect.left);
		int2 h = MCU_min(stackMinMaxSize.bottom, srect.bottom - srect.top
		                 - wTitleHeight);
		windRect.right = windRect.left + w;
		windRect.bottom = windRect.top + h;

		if (windRect.right > srect.right
		        || windRect.bottom > srect.bottom)
		{
			windRect.left = srect.left;
			windRect.top = srect.top + wTitleHeight;
			windRect.right = windRect.left + w;
			windRect.bottom = windRect.top + h;
		}
		else
			windRect.top += wTitleHeight - mbarheight;
		SetWindowStandardState(win, &windRect);
	} //of inZoomOut
	ZoomWindow(win, zoomInOrOut, win == FrontWindow());
	SetGWorld(oldport, olddevice);
}

void MCScreenDC::setmods(int2 modifiers)
{
	/*cmdKey      = 256;      {bit 0 of high byte}
	shiftKey    = 512;      {bit 1 of high byte}
	alphaLock   = 1024;     {bit 2 of high byte}
	optionKey   = 2048;     {bit 3 of high byte}
	controlKey  = 4096; */
	MCmodifierstate = 0;
	if (modifiers & shiftKey)
		MCmodifierstate |= MS_SHIFT;
	if (modifiers & cmdKey)
		MCmodifierstate |= MS_CONTROL;
	if (modifiers & optionKey)
		MCmodifierstate |= MS_MOD1;
	if (modifiers & controlKey)
		MCmodifierstate |= MS_MOD2;
	if (modifiers & alphaLock)
		MCmodifierstate |= MS_CAPS_LOCK;
}

void MCScreenDC::getkeycode(EventRecord &event, char *buffer)
{
	buffer[0] = event.message & charCodeMask;
	buffer[1] = '\0';
}

static Boolean isKeyPressed(unsigned char *km, uint1 keycode)
{
	return (km[keycode >> 3] >> (keycode & 7)) & 1;
}

void MCScreenDC::getkeysdown(MCExecPoint &ep)
{
	ep.clear();
	MCmodifierstate = querymods();
	KeyMap map;
	GetKeys(map);
	unsigned char *km = (unsigned char *)map;
	char kstring[U4L];
	KeySym ksym;
	bool first = true;
	uint2 i;
	for (i = 0; i < 127; i++)
	{
		if (isKeyPressed(km, i))
		{
			ksym = i;
			if (MCmodifierstate & MS_SHIFT || MCmodifierstate & MS_CAPS_LOCK)
				ksym = shift_keysyms[i];
			else
				ksym = keysyms[i];
			if (ksym > 0)
			{
				ep.concatuint(ksym, EC_COMMA, first);
				first = false;
			}
		}
	}
}

void MCScreenDC::mode_globaltolocal(Point& p)
{
	CGrafPtr oldport;
	GDHandle olddevice;
	GetGWorld(&oldport, &olddevice);
	SetGWorld(GetWindowPort((WindowPtr)mousewindow->handle.window), GetMainDevice());
	GlobalToLocal(&p);
	SetGWorld(oldport, olddevice);
	
	// IM-2013-08-01: [[ ResIndependence ]] apply device scale
	p.v = p.v / MCResGetDeviceScale();
	p.h = p.h / MCResGetDeviceScale();
}

void MCScreenDC::mfocus(EventRecord *event, Point p, Boolean dispatch, bool post_or_handle)
{
	if (!mdown && !grabbed)
	{
		WindowPtr curWindow;
		// MW-2009-06-08: Finding the window depends on whether the stack is remoted
		//   or not ('runtime' mode).
		mouseInWinRgn = FindWindow(p, &curWindow);
		if (curWindow != (WindowPtr)mousewindow->handle.window || MCmousestackptr == NULL
		        || mouseInWinRgn != inContent)
		{
			if (dispatch && MCmousestackptr != NULL
			        && (MCtracewindow == DNULL
			            || mousewindow->handle.window != MCtracewindow->handle.window))
				MCdispatcher->wmunfocus(mousewindow);
			if (activewindow->handle.window != NULL
			        && GetWRefCon((WindowPtr)activewindow->handle.window) == WM_MODAL
			        && (WindowPtr)activewindow->handle.window != curWindow)
			{
				mousewindow->handle.window = NULL;
				MCmousestackptr = NULL;
			}
			else
			{
				mousewindow->handle.window = (MCSysWindowHandle)curWindow;
				if (mouseInWinRgn == inContent)
				{
					MCmousestackptr = MCdispatcher->findstackd(mousewindow);
					if (MCmousestackptr != NULL)
						MCmousestackptr->resetcursor(True);
					else
						if (dispatch && (MCtracewindow == DNULL
						                 || mousewindow->handle.window
						                 != MCtracewindow->handle.window))
							MCdispatcher->enter(mousewindow);
				}
				else
				{
					if (dispatch && !MClockcursor)
					{
						Cursor c;
						SetCursor(GetQDGlobalsArrow(&c));
					}
					MCmousestackptr = NULL;
				}
			}
		}
	}
	
	// MW-2012-09-21: [[ Bug 10404 ]] If we don't want to handle this as an event then
	//   return now.
	if (!post_or_handle)
		return;
		
	SetRectRgn(mouseMoveRgn, p.h, p.v, p.h + 1, p.v + 1);
	if (MCmousestackptr != NULL)
	{
		if (MCtracewindow == DNULL
		        || mousewindow->handle.window != MCtracewindow->handle.window)
			if (dispatch)
			{
				// MW-2012-02-22: [[ Bug 9868 ]] Previously this would happen whether dispatching
				//   or not. This seems incorrect as the effect of an event should only be felt
				//   after its dispatched so moving here.
				mode_globaltolocal(p);
				MCmousex = p.h;
				MCmousey = p.v;
				// MW-2011-09-12: [[ MacScroll ]] Adjust the y-coord by scroll position.
				if (MCmousestackptr != nil)
					MCmousey += MCmousestackptr -> getscroll();
				
				MCdispatcher->wmfocus(mousewindow, MCmousex, MCmousey);
				if (MCbuttonstate != 0 && !m_drag_click && (MCU_abs(MCmousex - MCclicklocx) >= MCdragdelta || MCU_abs(MCmousey - MCclicklocy) >= MCdragdelta))
				{
					m_drag_click = true;
					MCdispatcher -> wmdrag(mousewindow);
				}
			}
			else
				if (event != NULL)
				{
					MCEventnode *tptr = new MCEventnode(*event);
					tptr->appendto(pendingevents);
				}
	}
}

static void domenu(short menu, short item)
{
	MenuHandle mhandle = GetMenuHandle(menu);
	if (mhandle == NULL)
		return;

	{
		MCGroup *mb = MCmenubar != NULL ? MCmenubar : MCdefaultmenubar;
		if (mb == NULL)
			return;
		//MetaCard's application menu, starts from id 2. id 1 is Apple menu

		//Aqua app's help menu is just like other menu...
		if (mb != NULL)
		{ //to response to the menu pick msg
			MCButton *bptr = NULL;
			bool isunicode = false;
			bool issubmenu = (menu >= SUB_MENU_START_ID);
			if (issubmenu)
				// Get main menu id of submenu, stored as a property of the menu
				GetMenuItemProperty(mhandle, 0, 'RRev', 'MMID', sizeof(menu), NULL, &menu);
			if (menu == 1)
			{//selection code
				uint2 helpmenu = 0;
				mb->count(CT_MENU, NULL, helpmenu);
				//search for the last mneu, that is the help menu in MC
				// MW-2005-03-08: [[MacOS Classic]] Predecrementing a reference causes problems
				helpmenu -= 1;
				bptr = (MCButton *)mb->findnum(CT_MENU, helpmenu);
			}
			else
			{
				uint2 gmenu = menu - 2;
				bptr = (MCButton *)mb->findnum(CT_MENU, gmenu);
			}
			if (bptr)
			{
				isunicode = bptr->hasunicode();
			}
			bool t_menuhastags;
			GetMenuItemProperty(GetMenuHandle(menu), 0, 'RRev', 'Tags', sizeof(t_menuhastags), NULL, &t_menuhastags);
			
			isunicode &= !t_menuhastags;
			char *menuitemname;
			uint2 menuitemlen;
			MCString t_tag;
			extern bool MCMacGetMenuItemTag(MenuHandle menu, uint2 mitem, MCString &s);
			if (t_menuhastags && MCMacGetMenuItemTag(mhandle, item, t_tag))
			{
				menuitemname = (char*)t_tag.getstring();
				menuitemlen = t_tag.getlength();
			}
			else
			{
				CFStringRef cfmenuitem;
				CopyMenuItemTextAsCFString(mhandle, item, &cfmenuitem);
				uint2 t_menuitemlen = CFStringGetLength(cfmenuitem);
				menuitemlen = t_menuitemlen * MCU_charsize(isunicode);
				if (isunicode)
				{
					menuitemname = new char[menuitemlen];
					CFStringGetCharacters(cfmenuitem, CFRangeMake(0, t_menuitemlen), (UniChar*)menuitemname);
				}
				else
				{
					menuitemname = new char[menuitemlen + 1];
					CFStringGetCString(cfmenuitem, menuitemname, menuitemlen + 1,kCFStringEncodingMacRoman);
				}
				CFRelease(cfmenuitem);
			}

			char *menupick = menuitemname;
			uint2 menupicklen = menuitemlen;
			char *newmenu = NULL;

			if (issubmenu)
			{ //item from submenu is selected
				/* get the menu hierachy to pass
				back to the user. menu hierachy is store in the menu
				handle's data field. It's format is :
				mainmenu|submenu1|submenu2|....    get the
				length of the menu hierachy. PASCAL string byte 0 is the length */

				CFStringRef cftitlestr;
				CopyMenuTitleAsCFString(mhandle,&cftitlestr);
				uint2 titlelen = CFStringGetLength(cftitlestr);
				uint2 t_titlestrlen = titlelen * MCU_charsize(isunicode);
				if (isunicode)
				{
					newmenu = new char[t_titlestrlen + menuitemlen];//bug menuitemlen too small for unicode
					CFStringGetCharacters(cftitlestr,CFRangeMake(0,titlelen),(UniChar *)newmenu);
				}
				else
				{
					newmenu = new char[t_titlestrlen + menuitemlen + 1];
					CFStringGetCString(cftitlestr, newmenu, titlelen + 1, kCFStringEncodingMacRoman);
				}

				memcpy(&newmenu[t_titlestrlen], menuitemname, menuitemlen);
				delete menuitemname;
				
				menupick = newmenu;
				menupicklen = t_titlestrlen + menuitemlen;

				CFRelease(cftitlestr);
			}
			if (bptr != NULL)
			{
				bptr->setmenuhistoryprop(item);
				bptr->message_with_args(MCM_menu_pick,MCString(menupick,menupicklen));
			}
			delete menupick;
		}
	}
}


Boolean MCScreenDC::dispatchevent(EventRecord &event, Boolean dispatch,
                                  Boolean &abort, Boolean &reset)
{
	KeySym keysym;
	char buffer[XLOOKUPSTRING_SIZE];
	static long lastMouseTick; //is set in mouseUp msg
	static Point lastMousePoint;   //is set in mouseDown msg
	static uint2 mbutton;
	Boolean handled = False;
	MCeventtime = event.when * 1000 / 60;
	WindowPtr oldactive, oldlast;
	
	switch (event.what)
	{
	case kHighLevelEvent:  //high level Apple Events
		AEProcessAppleEvent(&event);
		break;
	case updateEvt:
		doredraw(event);
		break;
	case activateEvt:  //for focus in
		break;
	case keyDown:
	case autoKey:
	
		getkeycode(event, buffer);
		setmods(event.modifiers);
		if (buffer[0] == '.')
		{
			if (MCmodifierstate & MS_CONTROL)
				if (MCallowinterrupts && !MCdefaultstackptr->cantabort())
					abort = True;
				else
					MCinterrupt = True;
		}
		if (dispatch && (MCtracewindow == DNULL
		                 || activewindow->handle.window
		                 != MCtracewindow->handle.window))
		{
			if (MCmodifierstate & MS_CONTROL && MCmodifierstate & MS_SHIFT && keysyms[(event.message & keyCodeMask) >> 8] < 0xFF00)
			{
				// Cmd+Shift+key sends non-shifted keysym in the event, so we call (UC)KeyTranslate to get the shifted keysym
				KeyboardLayoutRef t_kbdlayout;
				if (KLGetCurrentKeyboardLayout(&t_kbdlayout) == noErr)
				{
					KeyboardLayoutKind t_kind;
					if (KLGetKeyboardLayoutProperty(t_kbdlayout, kKLKind, (const void **)&t_kind) == noErr)
					{
						if (t_kind == kKLKCHRKind)
						{
							UInt32 t_keytransstate = 0;
							Ptr t_kchr = (Ptr) GetScriptManagerVariable(smKCHRCache);
							UInt32 t_key = KeyTranslate(t_kchr, ((event.message & keyCodeMask) >> 8) | shiftKey, &t_keytransstate);
							keysym = t_key;
						}
						else
						{
							UCKeyboardLayout *t_uchrdata;
							if (KLGetKeyboardLayoutProperty(t_kbdlayout, kKLuchrData, (const void**) &t_uchrdata) == noErr)
							{
								UInt32 t_deadkeystate = 0, t_key, t_keytransstate = 0;
								UniChar t_unicodestring[16];
								UniCharCount t_maxlength = 16, t_actuallength;

								SInt16 currentKeyScript, lastKeyLayoutID;
								currentKeyScript = GetScriptManagerVariable(smKeyScript);
								lastKeyLayoutID = GetScriptVariable(currentKeyScript, smScriptKeys);

								if (UCKeyTranslate(t_uchrdata, (event.message & keyCodeMask) >> 8, kUCKeyActionDown, (shiftKey >> 8) & 0xFF, LMGetKbdType(), 0, &t_deadkeystate, t_maxlength, &t_actuallength, t_unicodestring) == noErr)
								{
									keysym = t_unicodestring[0];
								}
							}
						} 
					}
				}
			}
			else if (buffer[0] > 0 && buffer[0] != 127 && keysyms[(event.message & keyCodeMask) >> 8] < 0xFF00)
			{
				if (buffer[0] >= 32)
					keysym = buffer[0];
				else
					keysym = buffer[0] + 64;
			}
			else
			{
				if (MCmodifierstate & MS_SHIFT || MCmodifierstate & MS_CAPS_LOCK)
					keysym = shift_keysyms[(event.message & keyCodeMask) >> 8];
				else
				{
					keysym = keysyms[(event.message & keyCodeMask) >> 8];
				}
				
				// MW-2011-03-15: [[ Bug 9451 ]] Reinstate various keyDown messages that were removed
				if ((((unsigned)buffer[0]) < 32 && ((unsigned)buffer[0] != 9)) || ((unsigned)buffer[0]) == 127)
					buffer[0] = '\0';
			}
			if (!MCdispatcher->wkdown(activewindow, buffer, keysym))
				if (event.modifiers & cmdKey)
				{
					long which;

					if (IsMacLFAM())
						which = MenuEvent(&event);
					else
						which = MenuKey((char)(event.message & charCodeMask));
					if (HiWord(which) != 0)
					{
						domenu(HiWord(which), LoWord(which));
						HiliteMenu(0);
					}
				}
			reset = True;
		}
		else
		{
			MCEventnode *tptr = new MCEventnode(event);
			tptr->appendto(pendingevents);
		}
		handled = True;
		break;
	case keyUp:
		getkeycode(event, buffer);
		setmods(event.modifiers);
		if (dispatch && (MCtracewindow == DNULL
		                 || activewindow->handle.window
		                 != MCtracewindow->handle.window))
		{
			if (buffer[0] > 0 && buffer[0] != 127
			        && keysyms[(event.message & keyCodeMask) >> 8] < 0xFF00)
				if (buffer[0] >= 32)
					keysym = buffer[0];
				else
					keysym = buffer[0] + 64;
			else
				if (MCmodifierstate & MS_SHIFT || MCmodifierstate & MS_CAPS_LOCK)
					keysym = shift_keysyms[(event.message & keyCodeMask) >> 8];
				else
					keysym = keysyms[(event.message & keyCodeMask) >> 8];
			MCdispatcher->wkup(activewindow, buffer, keysym);
			reset = True;
		}
		else
		{
			MCEventnode *tptr = new MCEventnode(event);
			tptr->appendto(pendingevents);
		}
		handled = True;


		break;
	case osEvt: // mouseMove event -- unix's MotionNotify event
		/* mouseMovedMessage = 0xFA000000  suspendResumeMessage = 0x01000000    *
		* osEvtMessageMask = 0xFF000000                                        *
		* charCodeMask = 0x000000FF                                            *
		* resumeFlag = 1   convertClipboardFlag = 2                            */
		setmods(event.modifiers);
		if ((event.message & 0xFF000000) == 0xFA000000)
		{ // mouse-move msg
			mfocus(&event, event.where, dispatch);
			handled = True;
		}
		else
			if ((event.message & 0xFF000000) == 0x01000000)
			{// suspend or resume
				if (!(event.message & resumeFlag))
				{ //it's a suspend event
					MCappisactive = False;
					// ACTIVATE ISSUE - fprintf(stderr, "Received suspend - active = %p, last_active = %p, fw = %p\n", activewindow -> handle . window, lastactivewindow -> handle . window, FrontWindow());
					if (mdown)
						clearmdown(event.modifiers & controlKey ? 3 : 1);
					if (grabbed)
					{
						buffer[0] = 0x1B;
						buffer[1] = '\0';
						Boolean oldlock = MClockmessages;
						MClockmessages = True;
						MCdispatcher->wkdown(mousewindow, buffer, XK_Escape);
						MCdispatcher->wkup(mousewindow, buffer, XK_Escape);
						MClockmessages = oldlock;
						handled = True;
						grabbed = False;
						MCmousex = MCmousey = -1; // set to -1 so button msg doesn't reopen menu
						MCdispatcher->wmfocus(mousewindow, MCmousex, MCmousey);
					}
					if (activewindow->handle.window != NULL)
					{
						MCdispatcher->wkunfocus(activewindow);
						dohilitewindow((WindowPtr)activewindow->handle.window, False);
						activewindow->handle.window = NULL;
					}
					if (lastactivewindow->handle.window != NULL)
					{
						dohilitewindow((WindowPtr)lastactivewindow->handle.window, False);
						lastactivewindow->handle.window = NULL;
					}
					if (MChidepalettes)
						MCstacks->hidepalettes(True);
					MCdefaultstackptr->getcard()->message(MCM_suspend);
					mousewindow->handle.window = NULL;
					if (menuBarHidden)
					{ //we are suspended, show menu bar for other process
						showmenu();
						menuBarHidden = True;
					}
					
					if (MChidebackdrop && (backdrop_active || backdrop_hard))
						HideWindow(backdrop_window);
				}
				else
				{ //it's a resume message
					MCappisactive = True;
					WindowPtr fw;
					fw = FrontWindow();
					if (MChidepalettes)
						MCstacks->hidepalettes(False);

					// ACTIVATE ISSUE - fprintf(stderr, "Received resume - fw = %p, last_active = %p\n", fw, lastactivewindow->handle.window);
					
					MCdefaultstackptr->getcard()->message(MCM_resume);

					if (lastactivewindow->handle.window != NULL && fw != NULL
					        && (WindowPtr)lastactivewindow->handle.window != fw)
						dohilitewindow(fw, False);
					else
						lastactivewindow->handle.window = (MCSysWindowHandle)fw;
					if (lastactivewindow->handle.window != NULL)
						dohilitewindow((WindowPtr)lastactivewindow->handle.window, True);
					//we are resumed and the menubar needs to be hidden, hide the menu bar
					if (menuBarHidden)
					{
						menuBarHidden = False;
						hidemenu();
					}
					if (activewindow->handle.window == NULL)
					{
						activewindow->handle.window = lastactivewindow->handle.window;
						MCdispatcher->wkfocus(activewindow);
					}
					
					if (MChidebackdrop && (backdrop_active || backdrop_hard))
						ShowWindow(backdrop_window);
				}
			}
		break;
	case mouseDown:  //handle the floating palette window here
		setmods(event.modifiers);
		if (MCtracewindow == DNULL
		        || mousewindow->handle.window != MCtracewindow->handle.window)
		{
			if (dispatch)
			{
				// MW-2012-02-22: [[ Bug 9868 ]] Previously this would happen whether dispatching
				//   or not. This seems incorrect as the effect of an event should only be felt
				//   after its dispatched so moving here.
				mfocus(&event, event.where, dispatch);
				if (mousewindow->handle.window == NULL && mouseInWinRgn != inDesk
				        && mouseInWinRgn != inMenuBar && mouseInWinRgn != inSysWindow)
				{
					beep();
					break;
				}
				switch (mouseInWinRgn)
				{ //value is set in MouseMove event
				case inDesk:
					break;
				case inMenuBar:
					{
						MCGroup *mb = MCmenubar != NULL ? MCmenubar : MCdefaultmenubar;
						if (mb != NULL)
						{
							mb->message_with_args(MCM_mouse_down, "1");
						}
						long mChoice = MenuSelect(event.where);
						if (mChoice != 0)
						{ //an menu item is selected
							short menu = HiWord(mChoice);
							short item = LoWord(mChoice);
							domenu(menu, item);
							HiliteMenu(0);
						}
					}
					break; //case inMenuBar
				case inZoomIn:
				case inZoomOut:
					if (TrackBox((WindowPtr)mousewindow->handle.window,
					             event.where, mouseInWinRgn))
					{
						dozoomwindow((WindowPtr)mousewindow->handle.window, mouseInWinRgn);
						MCdispatcher->configure(mousewindow);
					}
					break;
				case inDrag:
					if  (GetWRefCon((WindowPtr)mousewindow->handle.window) == WM_DRAWER)
						return False;
					if ((activewindow->handle.window == NULL
					        || activewindow->handle.window != mousewindow->handle.window
					        && (GetWRefCon((WindowPtr)activewindow->handle.window) != WM_MODAL))
					        && !(MCmodifierstate & MS_CONTROL))
						activatewindow(mousewindow);


					if (StillDown())
					{
						Rect crect;
						DragWindow((WindowPtr)mousewindow->handle.window, event.where,
						           GetRegionBounds(GetGrayRgn(), &crect));
					}
					MCdispatcher->configure(mousewindow);
					break;
				case inGrow: //window size changed
					{
						long newsize;
						Rect sizeR;
						MCStack *sptr = MCdispatcher->findstackd(mousewindow);
						sptr->getminmax(&sizeR);
						Rect t_new_size;
						m_in_resize = true;
						ResizeWindow((WindowPtr)mousewindow -> handle . window, event . where, &sizeR, &t_new_size);
						m_in_resize = false;
						MCdispatcher->configure(mousewindow);
					}
					break;
				case inGoAway:
					if (TrackGoAway((WindowPtr)mousewindow->handle.window, event.where))
						MCdispatcher->wclose(mousewindow);
					break;
				case inContent:
				{
					if (MCmousestackptr == NULL)
					{
						if (backdrop_active && (WindowPtr)mousewindow->handle.window == backdrop_window)
						{
							MCdefaultstackptr->getcard()->message_with_args(MCM_mouse_down_in_backdrop,
							                                      mbutton);
							mdown = True;
						}
						break;
					}
					
					oldactive = (WindowPtr)activewindow->handle.window;
					oldlast = (WindowPtr)lastactivewindow->handle.window;
					if (MCpointerfocus && GetWRefCon(FrontWindow()) < WM_PULLDOWN
					        && activewindow->handle.window != mousewindow->handle.window)
						activatewindow(mousewindow);


					WindowClass wclass;
					GetWindowClass((WindowPtr)mousewindow->handle.window, &wclass );
					if (wclass == kUtilityWindowClass)
						SetUserFocusWindow((WindowPtr)mousewindow->handle.window);

					mdown = True;
					mbutton = event.modifiers & controlKey ? 3 : 1;
					MCbuttonstate = 1 << (mbutton - 1);
					if ((event.when - lastMouseTick) * 1000 / 60 < MCdoubletime
							&& MCU_abs(event.where.h - lastMousePoint.h) < MCdoubledelta
							&& MCU_abs(event.where.v - lastMousePoint.v) < MCdoubledelta)
					{
						if (doubleclick)
						{
							doubleclick = False;
							tripleclick = True;
							MCdispatcher->wmdown(mousewindow, mbutton);
							lastMousePoint.v = lastMousePoint.h = 0;
						}
						else
						{
							doubleclick = True;
							MCdispatcher->wdoubledown(mousewindow, mbutton);
						}
					}
					else
					{
						Point p;
						p.h = event.where.h;
						p.v = event.where.v;
						mode_globaltolocal(p);
						MCclicklocx = p.h;
						MCclicklocy = p.v;
						// MW-2011-09-12: [[ MacScroll ]] Adjust the y-coord by scroll position.
						if (MCmousestackptr != nil)
							MCclicklocy += MCmousestackptr -> getscroll();
						MCclickstackptr = MCmousestackptr;
						tripleclick = doubleclick = False;
						m_drag_click = false;
						m_drag_event = event;
						MCdispatcher->wmfocus(mousewindow, MCclicklocx, MCclicklocy);
						MCdispatcher->wmdown(mousewindow, mbutton);
						lastMousePoint = event.where; //for checking the double click msg
					}

					reset = True;
				}
				break;
				default:
					break;
				}//switch (mouseInWinRgn)
			} //if dispatch
			else
				if (mouseInWinRgn == inContent)
				{
					mdown = True;
					MCEventnode *tptr = new MCEventnode(event);
					tptr->appendto(pendingevents);
				}
		}//no MC debug window (MCtracewindow)
		handled = True;
		break;
	case mouseUp:
		setmods(event.modifiers);
		if (mouseInWinRgn != inContent || mousewindow->handle.window == NULL
		        || !mdown)
			break;
		if (dispatch)
		{
			// MW-2012-02-22: [[ Bug 9868 ]] Previously this would happen whether dispatching
			//   or not. This seems incorrect as the effect of an event should only be felt
			//   after its dispatched so moving here.
			MCbuttonstate = 0;
			mdown = False;
			Point p;
			p.h = event.where.h; //get mouse point
			p.v = event.where.v;
			mode_globaltolocal(p);
			
			// MW-2011-09-12: [[ MacScroll ]] Adjust the y-coord by scroll position.
			if (MCmousestackptr != nil)
				p . v += MCmousestackptr -> getscroll();	
			
			if (MCmousex != p.h || MCmousey != p.v)
				MCdispatcher->wmfocus(mousewindow, p.h, p.v);
			if (activewindow != MCtracewindow)
			{
				if (backdrop_active && (WindowPtr)mousewindow->handle.window == backdrop_window)
					MCdefaultstackptr->getcard()->message_with_args(MCM_mouse_up_in_backdrop,
					                                      mbutton);
				else
					if (doubleclick)
						MCdispatcher->wdoubleup(mousewindow, mbutton);
					else
						MCdispatcher->wmup(mousewindow, mbutton);
				reset = True;
			}
		}
		else
		{
			MCEventnode *tptr = new MCEventnode(event);
			tptr->appendto(pendingevents);
		}
		lastMouseTick = event.when; //for determining the double -click msg
		handled = True;
		break;
	case nullEvent:
		handled = True; // get here from socket WakeUpProcess
		break;
	default:
		break;
	}
	return handled;
}

// If this global is true, it means that we are in a WNE and can dispatch
// events.
bool g_osx_dispatch_event = false;

Boolean MCScreenDC::handle(real8 sleep, Boolean dispatch, Boolean anyevent,
                           Boolean &abort, Boolean &reset)
{
	if (!MCModeMakeLocalWindows())
	{
		Boolean handled;
		handled = False;
		
		abort = False;
		reset = False;
		
		bool t_old_dispatch;
		t_old_dispatch = g_osx_dispatch_event;
		g_osx_dispatch_event = dispatch == True;
		
		if (MCnsockets != 0)
		{
			if (MCS_poll(0.0, 0))
			{
				handled = True;
				sleep = 0.0;
			}
		}
		
		MCNotifyDispatch(dispatch == True);
		
		if (MCrecording && sleep > 1.0/20.0)
			sleep = 1.0/20.0;
		
		CFRunLoopRunInMode(kCFRunLoopDefaultMode, sleep, true);
		
		s_animation_current_time = CFAbsoluteTimeGetCurrent();
		g_osx_dispatch_event = t_old_dispatch;
		
		if (MCrecording)
			MCtemplateplayer->handlerecord();
		
		return handled;
	}
	
	abort = reset = False;
	Boolean handled = False;

	if (!dispatch && abortkey())
	{
		abort = True;
		return True;
	}
	EventRecord event;
	Boolean synth = False;
	
	// MW-2010-09-09: If variant of MCS_poll returns resolving we
	//   only WNE for 1 tick, rather than long time.
	bool t_resolving;
	t_resolving = false;
	if (MCnsockets != 0)
	{
		if (MCS_poll(0.0, 0) == True)
		{
			handled = True;
			sleep = 0.0;
		}
	}

	// Dispatch notifications...
	MCNotifyDispatch(dispatch == True);

	bool t_old_dispatch;
	t_old_dispatch = g_osx_dispatch_event;
	g_osx_dispatch_event = dispatch == True;

	if (dispatch && pendingevents != NULL)
	{
		event = pendingevents->event;
		MCEventnode *tptr = (MCEventnode *)pendingevents->remove
		                    (pendingevents);
		delete tptr;
		synth = True;
	}
	else
	{
		extern uint32_t g_name_resolution_count;
		if (g_name_resolution_count > 0)
			sleep = 1.0 / 60.0;
		else if (sleep < 1.0/60.0)
			sleep = 0.0;

		// MW-2008-07-22: [[ Bug 1276 ]] Window resize continues after mouse has
		//   been released. Make sure we mask out mouseUp events if we are in a
		//   resize, otherwise the OS doesn't get it and it sticks in ResizeWindow.
		EventMask t_event_mask;
		t_event_mask = everyEvent;
		if (m_in_resize)
			t_event_mask &= ~mUpMask;
		
		if (sleep == 0.0)
		{
			// TODO - investigate potential use of GetOSEvent instead
			if (!GetNextEvent(t_event_mask, &event))
			{
				Point mloc;
				GetMouse(&mloc);


				LocalToGlobal(&mloc);
				if (!PtInRgn(mloc, mouseMoveRgn))
				{
					// mouse moved, so make up a synthetic event
					event.what = osEvt;
					event.when = TickCount();
					event.where = mloc;
					event.message = 0xFA000000;
					MCmodifierstate = querymods();
					event.modifiers = 0;
					if (MCmodifierstate & MS_SHIFT)
						event.modifiers |= shiftKey;
					if (MCmodifierstate & MS_CONTROL)
						event.modifiers |= cmdKey;
					if (MCmodifierstate & MS_MOD1)
						event.modifiers |= optionKey;
					if (MCmodifierstate & MS_MOD2)
						event.modifiers |= controlKey;
					if (MCmodifierstate & MS_CAPS_LOCK)
						event.modifiers |= alphaLock;
					synth = True;
				}
			}
		}
		else
		{
			WaitNextEvent(t_event_mask, &event, (unsigned long)(sleep * 60.0), mouseMoveRgn);
			s_animation_current_time = CFAbsoluteTimeGetCurrent();
		}
	}
		
	g_osx_dispatch_event = t_old_dispatch;

	// Delete any windows that are pending for deletion
	if (c_window_deletion_count > 0)
	{
		for(unsigned int i = 0; i < c_window_deletion_count; ++i)
		{
            // MM-2012-04-02: Use new MC*Window wrapper function - fixes bugs where a cocoa NSWindow has been
            //  created from the carbon WindowRef.
			extern void MCDisposeWindow(void *);
			MCDisposeWindow(c_window_deletions[i]);
		}
		free(c_window_deletions);
		c_window_deletions = NULL;
		c_window_deletion_count = 0;
	}

	/**********handle the QT movie controller msg here **************/
	Boolean dodispatch = True;

	/**********handle recording **************/
	if (MCrecording)
		MCtemplateplayer->handlerecord();
	if (dodispatch || event.what == mouseDown || event.what == mouseUp
	        || event.what == osEvt && (event.message & 0xFF000000) == 0xFA000000)
		return dispatchevent(event, dispatch, abort, reset) || handled;
	else
		return False;
}

void MCScreenDC::activatewindow(Window window)
{
	if (activewindow->handle.window != NULL
	        && GetWRefCon((WindowPtr)activewindow->handle.window) == WM_MODAL
	        && GetWRefCon((WindowPtr)window->handle.window) != WM_MODAL)
		return;
	
	if (activewindow->handle.window != NULL)
	{
		MCdispatcher->wkunfocus(activewindow);
		if (!MCactivatepalettes && GetWRefCon((WindowPtr)window->handle.window) != WM_SHEET)
			dohilitewindow((WindowPtr)activewindow->handle.window, False);
	}
	activewindow->handle.window = window->handle.window;
	
	Window bw = DNULL;

	if (bw == DNULL || GetWRefCon((WindowPtr)window->handle.window) == WM_MODAL
	        || GetWRefCon((WindowPtr)window->handle.window) == WM_PALETTE)
	{
        // MM-2012-04-02: Use new MC*Window wrapper function - fixes bugs where a cocoa NSWindow has been
        //  created from the carbon WindowRef.
		void MCBringWindowToFront(void *);
		// ACTIVATE ISSUE - fprintf(stderr, "Bringing window to front - %p\n", activewindow->handle.window);
		MCBringWindowToFront(activewindow->handle.window);
	}
	else
		if (bw != DNULL)
			SendBehind((WindowPtr)activewindow->handle.window, (WindowPtr)bw->handle.window);
	if (MCactivatepalettes
	        && GetWRefCon((WindowPtr)activewindow->handle.window) != WM_PALETTE)
	{
		if (activewindow->handle.window != lastactivewindow->handle.window )
		{
			if (GetWRefCon((WindowPtr)window->handle.window) != WM_DRAWER)
				dohilitewindow((WindowPtr)activewindow->handle.window, True);
			if (lastactivewindow->handle.window != NULL
			        && GetWRefCon((WindowPtr)window->handle.window) != WM_SHEET && GetWRefCon((WindowPtr)window->handle.window) != WM_DRAWER)
				dohilitewindow((WindowPtr)lastactivewindow->handle.window, False);
			lastactivewindow->handle.window = activewindow->handle.window;
		}
	}
	else
		dohilitewindow((WindowPtr)activewindow->handle.window, True);


	if (GetWRefCon((WindowPtr)window->handle.window) == WM_DRAWER)
	{
		dohilitewindow(GetDrawerParent((WindowPtr)window->handle.window), True);
		SetUserFocusWindow((WindowPtr)window->handle.window);
	}
	else
	{
		WindowClass wclass;
		GetWindowClass((WindowPtr)window->handle.window,&wclass);
		if (wclass == kUtilityWindowClass)
		{
			SetUserFocusWindow((WindowPtr)window->handle.window);
			SelectWindow((WindowPtr)window->handle.window);
		}
	}
	
	// MW-2012-08-31: [[ Bug 10085 ]] Make sure we tell the OS to activate
	//   the window, otherwise it returns the wrong value for 'FrontWindow'
	//   causing activation issues.
	ActivateWindow((WindowPtr)window -> handle . window, True);
	
	MCdispatcher->wkfocus(activewindow);
}

void MCScreenDC::doredraw(EventRecord &event, bool p_update_called)
{
	if ((WindowPtr)event.message != backdrop_window)
		return;

	CGrafPtr oldport;
	GDHandle olddevice;
	GetGWorld(&oldport, &olddevice);
	SetGWorld(GetWindowPort((WindowPtr)event.message), GetMainDevice());
	if (!p_update_called)
		BeginUpdate((WindowPtr)event.message);
	_Drawable d;
	d.type = DC_WINDOW;
	d.handle.window = (MCSysWindowHandle)event.message; //get the window ptr
	RgnHandle r = NewRgn();
	GetPortVisibleRegion(GetWindowPort((WindowPtr)d.handle.window), r);
	if ((WindowPtr)d.handle.window == backdrop_window)
	{
		Rect vrect;
		GetRegionBounds(r, &vrect);
		MCRectangle dirtyRect;
		dirtyRect = MCMacRectToMCRect(vrect);
		updatebackdrop(dirtyRect);
	}
	else
		MCdispatcher->redraw(&d, (MCRegionRef)r);
	DisposeRgn(r);
	SetGWorld(GetWindowPort((WindowPtr)event.message), GetMainDevice());
	if (!p_update_called)
		EndUpdate((WindowPtr)event.message);
	SetGWorld(oldport, olddevice);
}

void MCScreenDC::copybits(Drawable s, Drawable d, int2 sx, int2 sy,
                          uint2 sw, uint2 sh, int2 dx, int2 dy, uint4 rop)
{
	const BitMap *sbm, *dbm;
	PixMapHandle spm, dpm;
	if (s->type == DC_WINDOW)
		sbm = GetPortBitMapForCopyBits(GetWindowPort((WindowPtr)s->handle.window));
	else
		{
			spm = GetGWorldPixMap((CGrafPtr)s->handle.pixmap);
			LockPixels(spm);
			sbm = (BitMap *)*spm;
		}
	if (d->type == DC_WINDOW)
		dbm = GetPortBitMapForCopyBits(GetWindowPort((WindowPtr)d->handle.window));
	else
		{
			dpm = GetGWorldPixMap((CGrafPtr)d->handle.pixmap);
			LockPixels(dpm);
			dbm = (BitMap *)*dpm;
		}
	Rect srcR;
	Rect destR;
	SetRect(&srcR, sx, sy, sx + sw , sy + sh);
	SetRect(&destR, dx, dy, dx + sw, dy + sh);
	CopyBits(sbm, dbm, &srcR, &destR, rop, NULL);
	if (s->type == DC_BITMAP)
		UnlockPixels(spm);
	if (d->type == DC_BITMAP)
		UnlockPixels(dpm);
}

/**************************************************************************
    * Following functions create and handle menus.  These menus are MC       *
    * stack-based menus i.e. menus attached to a stack.  In Buttom.cpp file  *
    * it contains codes for MAC platform to display objects/buttons based    *
    * menus such as popup, options, and pulldown menus etc.		  *
    *			                                                  *
    **************************************************************************/
/**************************************************************************

        * The Application's Main Menu id starts from 2 to 32.  Apple reserves
       * id 0 and 1.  The id for the submenu of the Application's Main
       * menu, the button's pulldow menu, it's submenu, button's popup
       * menu, it's submenu and button's option menus starts from 33 to
       * 235.  No menu will be created when the maximum id is reached.  
       * 
       * An array of 256 of short int is maintained to keep track of the
       * allocation non-application main menu id, and it's de-allocation.
       * The reason behine this is that user main continue to create
       * button's popup/option/pulldown menus and their submenus, delete
       * them and re-create them again in one instance of a MC session.
       * Therefore it is necessary to keep track the used and unused id
       * Only member 33 to 235 would be used in this array
       **************************************************************************/

short MCScreenDC::allocateSubMenuID(Boolean issubmenu)
{
	/*
	routine go through each element of the array submenuID and find
	 the first available element and return the element's number as the
	 available submenu ID */
	short i = issubmenu? SUB_MENU_START_ID: 33;
	short submaxid = issubmenu? SUB_MENU_LAST_ID: 255;
	while (i < submaxid)
	{
		if (submenuIDs[i])
			i++;
		else
		{
			submenuIDs[i] = 1;
			return i;
		}
	}
	return 0;  //can't find available id for a new submenu use
}

void MCScreenDC::freeMenuAndID(short menuID, MCButton *p_menubutton)
{
	//routine to de-allocate menu ID which is allocated from submenuID
	//array and to free and delete the menu and it's submenus, if there is one
	MenuHandle menuHndl;
	submenuIDs[menuID] = 0; //zero out this id, put it back to available pool.
	// fprintf(stderr,"freemenuid getmenuhandle %d\n",menuID);
	
	menuHndl = GetMenuHandle(menuID); //get menu handl from id. if no menu
	if (menuHndl == NULL)             //for this id, we are done. The id is freed
		return;
	
	//remove accelerators assocated with this menu
	if (p_menubutton != NULL)
		MCstacks->deleteaccelerator(p_menubutton,NULL);

	int2 count = CountMenuItems(menuHndl);
	int2 i;
	for (i = 1; i <= count; i++)
	{
		short id;
		if (GetMenuItemHierarchicalID(menuHndl, i, &id) == noErr && id != 0)
			freeMenuAndID(id);
	}
	DeleteMenu(menuID);
	DisposeMenu(menuHndl); //release the memory
}

struct OSXMenuItem : public MCMenuItem
{
	MenuHandle mh;
	char mark;
	uint2 glyph;
	uint2 after; //insert new menu item after this number
};

static void SetMacMenuItemText(MenuHandle mh, uint2 mitem, char *mitemtext, uint2 mitemlength, bool isunicode)
{

	CFStringRef cfmenuitemstring;

	if (isunicode && mitemlength > 0)
		cfmenuitemstring = CFStringCreateWithCharacters(NULL, (UniChar *)mitemtext, mitemlength >> 1);
	else
		cfmenuitemstring = CFStringCreateWithCString(NULL, mitemtext, kCFStringEncodingMacRoman);

	SetMenuItemTextWithCFString(mh, mitem, cfmenuitemstring);
	CFRelease(cfmenuitemstring);
}

uint2 MCMacKeysymToGlyph(uint4 key)
{
	for (int i = 0; key_glyphs[i].glyph != 0; i++)
	{
		if (key == key_glyphs[i].keysym)
		{
			return key_glyphs[i].glyph;
		}
	}
	return kMenuNullGlyph;
}

uint4 MCMacGlyphToKeysym(uint2 glyph)
{
	for (int i = 0; key_glyphs[i].glyph != 0; i++)
	{
		if (glyph == key_glyphs[i].glyph)
		{
			return key_glyphs[i].keysym;
		}
	}
	return 0;
}

class OSXMenuCallback : public IParseMenuCallback
{
	bool firstTime;
	bool isunicode;
	OSXMenuItem curMenuStruct[MAX_SUBMENU_DEPTH + 1];
	uint2 firstitem;
	int2 curMenuLevel;
	uint2 mainMenuID;
	MCScreenDC *screen;
	MCFontStruct *font;
	MenuHandle appleMenu;
	MCButton *menubutton;

public:
	OSXMenuCallback(MCScreenDC *p_screen, MCButton *p_menubutton, MenuHandle p_mainmenu, MenuHandle p_applemenu, uint2 p_mainmenu_id)
	{
		screen = p_screen;
		menubutton = p_menubutton;
		appleMenu = p_applemenu;
		isunicode = p_menubutton -> hasunicode();
		firstTime = true;
		mainMenuID = p_mainmenu_id;
		memset(curMenuStruct, 0, sizeof(curMenuStruct));
		curMenuLevel = 0;
		firstitem = 0;
		curMenuStruct[0].after = firstitem;
		//fill in element 0 to indicates this is the top level menu
		curMenuStruct[0].mh = p_mainmenu; //put the top-most-level menu handle in the
		//element 0 of "mh" field
	}
	
	void SetMacMenuItemProperties(MenuHandle p_menu, uint2 p_item, OSXMenuItem *p_menuitem)
	{
		if (p_menuitem->accelerator)
		{
			if (p_menuitem->glyph)
				SetMenuItemKeyGlyph(p_menu, p_item, p_menuitem->glyph);
			else
				SetMenuItemCommandKey(p_menu, p_item, false, p_menuitem->accelerator);
			uint1 t_mods = p_menuitem->modifiers;
			uint1 t_macmods = 0;
			if (t_mods & MS_SHIFT)
				t_macmods |= kMenuShiftModifier;
			if (t_mods & MS_ALT)
				t_macmods |= kMenuOptionModifier;
			if (t_mods & MS_MAC_CONTROL)
				t_macmods |= kMenuControlModifier;
			if (!(t_mods & MS_CONTROL))
				t_macmods |= kMenuNoCommandModifier;
			SetMenuItemModifiers(p_menu, p_item, t_macmods);
			
			uint2 t_accel = p_menuitem->accelerator;
			if (t_accel < 256)
				t_accel = MCS_tolower(t_accel);
			MCstacks->addaccelerator(menubutton, menubutton->getstack(), t_accel, t_mods);
		}
		if (p_menuitem->is_disabled)
		{
			DisableMenuItem(p_menu, p_item);
		}
		if (p_menuitem->tag != NULL)
		{
			MCString t_tag = p_menuitem->tag;
			SetMenuItemProperty(p_menu, p_item,'RRev','MTag',t_tag.getlength(),(const void *)t_tag.getstring());
			
			static const struct { const char *tag; MenuCommand id; } s_tag_to_id[] =
			{
				{"undo", kHICommandUndo},
				{"redo", kHICommandRedo},
				{"cut", kHICommandCut},
				{"copy", kHICommandCopy},
				{"paste", kHICommandPaste},
				{"clear", kHICommandClear},
				{"select all", kHICommandSelectAll},
				{"preferences", kHICommandPreferences},
				{"about", kHICommandAbout},
			};

			for(uint4 i = 0; i < sizeof(s_tag_to_id) / sizeof(s_tag_to_id[0]); ++i)
				if (t_tag == s_tag_to_id[i] . tag)
				{
					SetMenuItemCommandID(p_menu, p_item, s_tag_to_id[i] . id);
					break;
				}
		}
	}

	virtual bool ProcessItem(MCMenuItem *p_menuitem)
	{
		uint2 t_keyglyph = 0;
		uint4 t_key = p_menuitem->accelerator;
		const char *t_keyname = p_menuitem->accelerator_name;
		if (t_keyname != NULL)
		{
			t_keyglyph = MCMacKeysymToGlyph(t_key);
		}

		char markchar = 0;
		
		if (p_menuitem->is_hilited)
		{
			if (p_menuitem->is_radio)
				markchar = diamondMark;
			else
				markchar = checkMark;
		}
		
		if (firstTime)
		{
			curMenuStruct[curMenuLevel].assignFrom(p_menuitem);
			curMenuStruct[curMenuLevel].label.set( p_menuitem->label.clone() , p_menuitem->label.getlength() );
			if (p_menuitem->tag != NULL)
				curMenuStruct[curMenuLevel].tag.set( p_menuitem->tag.clone() , p_menuitem->tag.getlength() );
			curMenuStruct[curMenuLevel].mark = markchar;
			curMenuStruct[curMenuLevel].glyph = t_keyglyph;
			firstTime = false;
			return false;
		}
		if (p_menuitem->depth > curMenuStruct[curMenuLevel].depth)
		{ //cur item goes down one level deep
			//new submenu name reflects the hierachy of it's menu level :

			uint4 t_combined_length = 0;
			for (int i = 0; i <= curMenuLevel; i++)
			{
				if (curMenuStruct[i].tag != NULL)
					t_combined_length += curMenuStruct[i].tag.getlength() * MCU_charsize(isunicode);
				else
					t_combined_length += curMenuStruct[i].label.getlength();
					
				t_combined_length += MCU_charsize(isunicode);
			}

			char *newname = new char[t_combined_length + 1];

			uint2 submenutitlelen = 0;

			MCExecPoint ep;
			for (int i = 0; i <= curMenuLevel; i++)
			{
				if (curMenuStruct[i].tag != NULL)
				{
					MCString t_tag = curMenuStruct[i].tag;
					if (isunicode)
					{
						ep.setsvalue(t_tag);
						ep.nativetoutf16();
						t_tag = ep.getsvalue();
					}
					memcpy(&newname[submenutitlelen], t_tag.getstring(), t_tag.getlength());
					submenutitlelen += t_tag.getlength();
				}
				else
				{
					memcpy(&newname[submenutitlelen], curMenuStruct[i].label.getstring(), curMenuStruct[i].label.getlength());
					submenutitlelen += curMenuStruct[i].label.getlength();
				}

				MCU_copychar('|',&newname[submenutitlelen],isunicode);
				submenutitlelen += MCU_charsize(isunicode);
			}

			newname[submenutitlelen] = 0;

			short menuID = screen->allocateSubMenuID(True);// get an id, if none is found, bail out
			if (menuID == 0)
			{
				return true;
			}

			CreateNewMenu(menuID, 0, &curMenuStruct[curMenuLevel + 1].mh);
			SetMenuItemProperty(curMenuStruct[curMenuLevel + 1].mh, 0, 'RRev','MMID',sizeof(mainMenuID),&mainMenuID);
			InsertMenu(curMenuStruct[curMenuLevel + 1].mh, -1); //-1 is submenu

			CFStringRef cfmenustring;
			if (isunicode)
				cfmenustring = CFStringCreateWithCharacters(NULL, (UniChar *)newname,submenutitlelen >> 1);
			else
				cfmenustring = CFStringCreateWithCString(NULL, newname, kCFStringEncodingMacRoman);

			SetMenuTitleWithCFString(curMenuStruct[curMenuLevel + 1].mh,cfmenustring);
			CFRelease(cfmenustring);
			delete newname;

			curMenuStruct[curMenuLevel + 1].after = 0;

			const char *t_label = (const char *)curMenuStruct[curMenuLevel].label.getstring();
			
			if (isunicode)
				cfmenustring = CFStringCreateWithCharacters(NULL, (UniChar *)t_label, curMenuStruct[curMenuLevel].label.getlength() >> 1);
			else
				cfmenustring = CFStringCreateWithCString(NULL, t_label, kCFStringEncodingMacRoman);
			// HS-2010-06-07: [[ Bug 8659 ]] Make sure OS X meta-characters are ignored except for strings like '-'
			UInt32 t_attributes;
			CFStringGetLength(cfmenustring) > 1 ? t_attributes = kMenuItemAttrIgnoreMeta : t_attributes = 0;
			//Insert the item and increment the item insertion point by 1
			InsertMenuItemTextWithCFString(curMenuStruct[curMenuLevel].mh, cfmenustring, curMenuStruct[curMenuLevel].after++, t_attributes, 0);
			CFRelease(cfmenustring);

			SetMacMenuItemProperties(curMenuStruct[curMenuLevel].mh, curMenuStruct[curMenuLevel].after, &curMenuStruct[curMenuLevel]);
			SetMenuItemHierarchicalID(curMenuStruct[curMenuLevel].mh,  curMenuStruct[curMenuLevel].after, menuID);

			//DO NOT SET MARK CHAR Here, this would reset the submenu attribute.
			//    delete newt;
			curMenuLevel++;
		}
		else
		{
			CFStringRef cfmenustring;
			if (isunicode)
				cfmenustring = CFStringCreateWithCharacters(NULL, (UniChar *)curMenuStruct[curMenuLevel].label.getstring(),curMenuStruct[curMenuLevel].label.getlength() >> 1);
			else
				cfmenustring = CFStringCreateWithCString(NULL, (char*)curMenuStruct[curMenuLevel].label.getstring(), kCFStringEncodingMacRoman);
			// HS-2010-06-07: [[ Bug 8659 ]] Make sure OS X meta-characters are ignored except for strings like '-'
			UInt32 t_attributes;
			CFStringGetLength(cfmenustring) > 1 ? t_attributes = kMenuItemAttrIgnoreMeta : t_attributes = 0;
			InsertMenuItemTextWithCFString(curMenuStruct[curMenuLevel].mh, cfmenustring, curMenuStruct[curMenuLevel].after++, t_attributes, 0);
			CFRelease(cfmenustring);

			SetMacMenuItemProperties(curMenuStruct[curMenuLevel].mh, curMenuStruct[curMenuLevel].after, &curMenuStruct[curMenuLevel]);
			SetItemMark(curMenuStruct[curMenuLevel].mh, curMenuStruct[curMenuLevel].after, curMenuStruct[curMenuLevel].mark);

			//go back up in menu levels. set curMenuLevel to reflect the changes

			while (curMenuLevel && curMenuStruct[curMenuLevel].depth > p_menuitem->depth)
			{
				delete curMenuStruct[curMenuLevel].label.getstring();
				curMenuStruct[curMenuLevel].label.set(NULL, 0);
				delete curMenuStruct[curMenuLevel].tag.getstring();
				curMenuStruct[curMenuLevel].tag.set(NULL, 0);
				curMenuLevel--;
			}
			if (curMenuStruct[curMenuLevel].depth < p_menuitem->depth)
				curMenuLevel++;
		}
		delete curMenuStruct[curMenuLevel].label.getstring();
		curMenuStruct[curMenuLevel].label.set(NULL, 0);
		delete curMenuStruct[curMenuLevel].tag.getstring();
		curMenuStruct[curMenuLevel].tag.set(NULL, 0);

		//keep record of the current item
		curMenuStruct[curMenuLevel].assignFrom(p_menuitem);
		curMenuStruct[curMenuLevel].label.set( p_menuitem->label.clone() , p_menuitem->label.getlength() );
		if (p_menuitem->tag != NULL)
			curMenuStruct[curMenuLevel].tag.set( p_menuitem->tag.clone(), p_menuitem->tag.getlength() );
		curMenuStruct[curMenuLevel].mark = markchar;
		curMenuStruct[curMenuLevel].glyph = t_keyglyph;


		return false;
	}
	
	virtual bool End(bool p_hastags)
	{
		if (curMenuStruct[curMenuLevel].label.getlength())
		{
			//handle single menu item and the last menu item situation
			//add the menu item to the menu
			CFStringRef cfmenustring;
			if (isunicode)
				cfmenustring = CFStringCreateWithCharacters(NULL, (UniChar *)curMenuStruct[curMenuLevel].label.getstring(),curMenuStruct[curMenuLevel].label.getlength() >> 1);
			else
				cfmenustring = CFStringCreateWithCString(NULL, (char*)curMenuStruct[curMenuLevel].label.getstring(), kCFStringEncodingMacRoman);
			// HS-2010-06-07: [[ Bug 8659 ]] Make sure OS X meta-characters are ignored except for strings like '-'
			UInt32 t_attributes;
			CFStringGetLength(cfmenustring) > 1 ? t_attributes = kMenuItemAttrIgnoreMeta : t_attributes = 0;

			// HS-2010-06-07: [[ Bug 8659 ]] Make sure OS X meta-characters are ignored except for strings like '-'
			InsertMenuItemTextWithCFString(curMenuStruct[curMenuLevel].mh, cfmenustring, curMenuStruct[curMenuLevel].after++, t_attributes, 0);
			SetItemMark(curMenuStruct[curMenuLevel].mh,
						curMenuStruct[curMenuLevel].after, curMenuStruct[curMenuLevel].mark);
			SetMacMenuItemProperties(curMenuStruct[curMenuLevel].mh, curMenuStruct[curMenuLevel].after, &curMenuStruct[curMenuLevel]);

			CFRelease(cfmenustring);
		}
		while (curMenuLevel >= 0)
		{
			delete curMenuStruct[curMenuLevel].label.getstring();
			curMenuStruct[curMenuLevel].label.set(NULL, 0);
			delete curMenuStruct[curMenuLevel].tag.getstring();
			curMenuStruct[curMenuLevel].tag.set(NULL, 0);
			curMenuLevel--;
		}
		MenuHandle t_mainmenu = curMenuStruct[0].mh;
		
		SetMenuItemProperty(t_mainmenu, 0, 'RRev', 'Tags', sizeof(p_hastags), &p_hastags);
		return false;
	}
};

Boolean MCScreenDC::addMenuItemsAndSubMenu(uint2 mainMenuID, MenuHandle mainMenu, MCButton *p_menubutton, uint1 menumode)
{
	OSXMenuCallback t_callback(this, p_menubutton, mainMenu, appleMenu, mainMenuID);
	bool isunicode = p_menubutton -> hasunicode();
	MCString t_string;
	p_menubutton->getmenustring(t_string);
	MCParseMenuString(t_string, &t_callback, isunicode, menumode);

	return True;
}

void MCScreenDC::updatemenubar(Boolean force)
{
	//get a pointer to each button (MCButton *) in the menu bar group.
	//if the stack does not have it's own menu bar group, use he default
	//menu bar group is called by reference and is changed by the
	//findnum() routine while not end of the button list

	MenuHandle menu; //handle for a menu
	MCButton *bptr; //point to a button
	MCGroup *newMenuGroup; //pointer to the menu group
	static MCGroup *curMenuGroup; //current menu bar handle
	static uint2 curMenuCount;    //current menu bar's menu count

	if (MCdefaultmenubar == NULL)   // the menu of first stack opened becomes
		MCdefaultmenubar = MCmenubar; // the default menu bar automatically
	//get current menu group
	if (MCmenubar != NULL)
		newMenuGroup = MCmenubar;
	else
		newMenuGroup = MCdefaultmenubar;
	
	//if doesn't need update and not force update then exit
	if (newMenuGroup == NULL || newMenuGroup == curMenuGroup && !force && !curMenuGroup->getstate(CS_NEED_UPDATE))
		return;

	uint2 i;
	//delete the existing Main menus and it's submenus except Apple
	//menu. Deleting Main menu starts from menu id 2.  Aslo Delete menu
	//items in the Application Help menu, if in None-Aqua env.
	//start deleting menus
	for (i = 0; i < curMenuCount; i++)
		freeMenuAndID(i + 2);

	/* Delete Items in the system provided help menu. Assume Help menu
	    * does not have submenus. In Aqua, There isn't a system provided
	    * help menu that you can add your help items to, therefore no
	    * action here, the application created help menu is deleted by the
	    * freeMenuAndID() in the above. MCaqua is a global var, inited in
	    * MCS_init() in macspec.cpp */
	//get menu handle and index
	
	
	uint2 which;    //menu item index
	which = i = 0;  //init to 0
	uint2 menuCount = 0;

	/**********************************************************************
	    * used to keep track of the CURRENT hierachical menu level
	    * structure.  * array[0] ==the top level menu, array[1] ==first
	    * level sub menu, array[2]==2nd level sub menu, the array contains
	    * the menu handle of each level of submenu.  The Application Main
	    * menu is limited from id 2 to id 32(MAIN_MENU_LAST_ID)
	    **********************************************************************/

	newMenuGroup->count(CT_MENU, NULL, menuCount); //get the # of MC menus
	// the last menu is the Help menu, it's menu items will be inserted into the
	// Application Help menu if the OS is non-Aqua.  Create a menu even if
	// it's contents is empty
	
	// ****** menuCount--; //so that the last menu (help menu) will not be created
	
	bool t_pref_item_found;
	t_pref_item_found = false;
	
	bool t_about_item_found;
	t_about_item_found = false;
	while (((bptr = (MCButton *)newMenuGroup->findnum(CT_MENU, i)) != NULL) &&
		   which < menuCount &&
		   (which + MAIN_MENU_FIRST_ID) <= MAIN_MENU_LAST_ID)
	{
		// IM-2008-09-05: [[ bug 7089 ]] - existing message path puts OSX system menu shortcut handling
		// after control-specific key handling.  Now changed to use stacklist accelerator handling,
		// consistent with windows/linux menu shortcuts
		
		// remove any menu shortcuts for the current button from accelerator list
		MCstacks->deleteaccelerator(bptr, bptr->getstack());
		
		//found a menu group & is not the very last menu
		MCString t_menu_label;
		bool t_is_unicode;
		bptr->getlabeltext(t_menu_label, t_is_unicode); // clone menu title/name
		
		if (t_menu_label . getlength() != 0 && bptr->getflag(F_VISIBLE))
		{
			//unsigned char *tmp = c2pstr(mname);
			
			//Apple menu's id == 1, new application's menus starts with id 2 and up
			//menu = NewMenu(which + MAIN_MENU_FIRST_ID, tmp);
			
			menu = NULL;
			CreateNewMenu(which + MAIN_MENU_FIRST_ID, 0, &menu);
			
			//menu title.  Has to be Pascal string let's add menu items to
			//the newly created menu get menu item list, each item is
			//separated by ','. the cloned menustring is deleted in
			//addMenuItemsAndSubMenu().
			CFStringRef cfmenustring;
			if (t_is_unicode)
				cfmenustring = CFStringCreateWithCharacters(NULL, (UniChar *)t_menu_label.getstring(), t_menu_label.getlength() >> 1);
			else
				cfmenustring = CFStringCreateWithBytes(NULL, (UInt8*)t_menu_label . getstring(), t_menu_label . getlength(), kCFStringEncodingMacRoman, False);
			SetMenuTitleWithCFString(menu,cfmenustring);
			CFRelease(cfmenustring);

			addMenuItemsAndSubMenu(which + MAIN_MENU_FIRST_ID, menu, bptr, True);
			if (which == 0)
			{
				int2 count = CountMenuItems(menu);
				DeleteMenuItem(menu, count--);
				DeleteMenuItem(menu, count);
			}
			else
			{
				// MW-2011-02-28: [[ Bug 3071 ]] We only enable the preferences item if:
				//   1) There is an enabled edit menu
				//   2) The edit menu has as last item one with tag "Preferences", or with name that begins with "Preferences"
				//   3) The relevant item, itself, is enabled.
				//
				const char *tname = bptr->getname_cstring();
				if (!t_pref_item_found && MCU_strncasecmp("Edit", tname, strlen(tname)) == 0)
				{
					uint32_t count = CountMenuItems(menu);
					MenuCommand t_id;
					if (GetMenuItemCommandID(menu, count, &t_id) != noErr)
						t_id = 0;
					if (t_id != kHICommandPreferences)
					{
						CFStringRef t_text;
						if (CopyMenuItemTextAsCFString(menu, count, &t_text) == noErr)
						{
							if (CFStringCompareWithOptions(t_text, CFSTR("Preferences"), CFRangeMake(0, 11), kCFCompareCaseInsensitive) == 0)
								t_pref_item_found = true;
							CFRelease(t_text);
						}
					}
					else
						t_pref_item_found = true;
					
					if (t_pref_item_found)
					{
						t_pref_item_found = !bptr -> isdisabled() && IsMenuItemEnabled(menu, count);
						
						DeleteMenuItem(menu, count--);
						DeleteMenuItem(menu, count);
					}
					
					MenuRef prefmenu = NULL;
					MenuItemIndex item = 0;
					GetIndMenuItemWithCommandID(NULL, kHICommandPreferences, 1, &prefmenu, &item);
					if (prefmenu != NULL && item != 0)
						if (t_pref_item_found)
							EnableMenuItem(prefmenu, item);
						else
							DisableMenuItem(prefmenu, item);
					
					t_pref_item_found = true;
				}
				
				// MW-2011-02-28: [[ Bug 818 ]] Be more specific about the 'About' item, and
				//   remove/disable if its not present / not enabled.
				if (!t_about_item_found && MCU_strncasecmp("Help", tname, strlen(tname)) == 0)
				{
					uint32_t count = CountMenuItems(menu);
					MenuCommand t_id;
					if (GetMenuItemCommandID(menu, count, &t_id) != noErr)
						t_id = 0;
					if (t_id != kHICommandAbout)
					{
						CFStringRef t_text;
						if (CopyMenuItemTextAsCFString(menu, count, &t_text) == noErr)
						{
							if (CFStringCompareWithOptions(t_text, CFSTR("About"), CFRangeMake(0, 5), kCFCompareCaseInsensitive) == 0)
								t_about_item_found = true;
							CFRelease(t_text);
						}
					}
					else
						t_about_item_found = true;
					
					if (t_about_item_found)
					{
						CFStringRef t_about_text;
						t_about_text = NULL;
						CopyMenuItemTextAsCFString(menu, count, &t_about_text);
						
						bool t_about_enabled;
						t_about_enabled = !bptr -> isdisabled() && IsMenuItemEnabled(menu, count);
							
						DeleteMenuItem(menu, count--);
						DeleteMenuItem(menu, count);
						
						SetMenuItemTextWithCFString(appleMenu, 1, t_about_text);
						ChangeMenuItemAttributes(appleMenu, 1, t_about_enabled ? 0 : kMenuItemAttrDisabled, (t_about_enabled ? kMenuItemAttrDisabled : 0) | kMenuItemAttrHidden);
						
						if (t_about_text != NULL)
							CFRelease(t_about_text);
					}
					else
						ChangeMenuItemAttributes(appleMenu, 1, kMenuItemAttrHidden | kMenuItemAttrDisabled, 0);
					
					t_about_item_found = true;
				}
			}
			
			// MW-2005-09-05: Only add a menu if it has items...
			if (CountMenuItems(menu) > 0)
				InsertMenu(menu, which + MAIN_MENU_FIRST_ID);
			
			//if menu disabled flag is set, disabled the entire menu
			if (bptr->isdisabled())
				DisableMenuItem(menu, 0);  // 8.5, disables past 31 items
		}
		which++;
		i = which;
	}
	curMenuCount = which; //set the menu count, exclude the Apple and Help menus

	curMenuGroup = newMenuGroup;
	curMenuGroup->setstate(False, CS_NEED_UPDATE);
	DrawMenuBar();
	HiliteMenu(0);
}




void MCScreenDC::clearmdown(uint2 which)
{
	// MW-2009-10-27: Fix to make sure remote popup menus work more like we expect.
	if (MCModeMakeLocalWindows() && mdown ||
		!MCModeMakeLocalWindows() && MCbuttonstate != 0)
	{
		MCbuttonstate = 0;
		mdown = False;
		MCdispatcher->wmup(mousewindow, which);
	}
}

void MCScreenDC::setdnddata(DragReference theDrag)
{
	if (!owndnd || theDrag != dnddata)
	{
		dnddata = theDrag;
		owndnd = False;
		MCdragsource = NULL;
	}
}

pascal  OSErr TSMOffsetToPosition(const AppleEvent *theAppleEvent,
                                  AppleEvent *reply, long handlerRefcon)
{
	OSErr err;
	DescType returnedType;
	Point thePoint = {0,0};
	uint2 offset;
	long actualsize;
	OSErr rterr;
	err = AEGetParamPtr(theAppleEvent, keyAEOffset, typeLongInteger,
	                    &returnedType, &offset, sizeof(offset), &actualsize);
	if (err != noErr)
		return err;
	rterr = noErr;
	if (MCactivefield)
	{
		MCRectangle r;
		Boolean incomposition = MCactivefield->getcompositionrect(r,offset);
		thePoint.h = r.x;
		thePoint.v = r.y+(r.height >> 1);
		if (!incomposition)
			rterr = errOffsetInvalid;
		else
		{
			Window w = MCactivefield->getstack()->getw();
			CGrafPtr oldport;
			GDHandle olddevice;

			GetGWorld(&oldport, &olddevice);
			SetGWorld(GetWindowPort((WindowPtr)w->handle.window), GetMainDevice());
			LocalToGlobal(&thePoint);
			SetGWorld(oldport,olddevice);
		}
	}
	else
		rterr = errOffsetIsOutsideOfView;
	err = AEPutParamPtr(reply, keyAEPoint, typeQDPoint, &thePoint, sizeof(Point));
	err = AEPutParamPtr(reply, keyErrorNumber, typeShortInteger, &rterr, sizeof(rterr));
	return err;
}

pascal OSErr TSMPositionToOffset(const AppleEvent *theAppleEvent,
                                 AppleEvent *reply, long handlerRefcon)
{
	OSErr err;
	DescType returnedType;
	Point thePoint;
	uint4 offset = 0;
	long actualsize;
	short where = kTSMOutsideOfBody;
	//extract position
	err = AEGetParamPtr(theAppleEvent, keyAECurrentPoint, typeQDPoint,
	                    &returnedType, &thePoint, sizeof(thePoint), &actualsize);
	if (err != noErr)
		return err;
	
	err = AEPutParamPtr(reply, keyAEOffset, typeLongInteger,
	                    &offset, sizeof(offset));
	if (err != noErr)
		return err;
	err = AEPutParamPtr(reply, keyAERegionClass, typeShortInteger,
	                    &where, sizeof(where));
	if (err != noErr)
		return err;
	return noErr;
}


pascal OSErr TSMUpdateHandler(const AppleEvent *theAppleEvent,
                              AppleEvent *reply, long handlerRefcon)
{
	OSErr err;
	DescType returnedType;
	AEDesc text,hiliterange;
	long actualsize;
	TextRangeArray *hiliterangeptr = NULL;
	long fixLength;
	AEDesc slr;
	if (MCactivefield == NULL)
		return paramErr;
	ScriptLanguageRecord scriptLangRec;
	err = AEGetParamDesc(theAppleEvent, keyAETSMScriptTag,
	                     typeIntlWritingCode, &slr);
	err = AEGetDescData(&slr, (void *)&scriptLangRec,
	                    sizeof(ScriptLanguageRecord));
	if (err != noErr)
		return err;
	ScriptCode scriptcode = scriptLangRec.fScript;
	AEDisposeDesc(&slr);

	//  if (!charset)
	//  return paramErr; //pass event if script is english
	if (!MCactivefield)
		return noErr;
	//get updated IME text
	if (MCS_imeisunicode())
		err = AEGetParamDesc(theAppleEvent, keyAETheData, typeUnicodeText, &text);
	else
		err = AEGetParamDesc(theAppleEvent, keyAETheData, typeChar, &text);
	if (err != noErr)
		return err;
	err = AEGetParamPtr(theAppleEvent, keyAEFixLength, typeLongInteger,
	                    &returnedType, &fixLength,
	                    sizeof(fixLength), &actualsize);
	//if fixedLength == -1 then text is final
	//if fixedLength == 0 no comfirmed text
	//> 0 number of characters confirmed
	if (err != noErr)
		return err;
	int4 imetextsize;
	imetextsize = AEGetDescDataSize(&text);
	char *imetext = new char[imetextsize + 1];
	AEGetDescData(&text, (void *) imetext, imetextsize);
	imetext[imetextsize] = '\0';
	uint2 commitedLen = (fixLength == -1) ?imetextsize: fixLength;
	//get hilite range
	err = AEGetParamDesc(theAppleEvent, keyAEHiliteRange,
	                     typeTextRangeArray, &hiliterange);
	if (err == noErr)
	{
		uint4 hiliterangesize = AEGetDescDataSize(&hiliterange);
		hiliterangeptr = (TextRangeArray *)new char[hiliterangesize];
		AEGetDescData(&hiliterange, (void *) hiliterangeptr, hiliterangesize);
	}
	if (!MCS_imeisunicode())
	{
		uint4 unicodelen;
		char *unicodeimetext = new char[imetextsize << 1];
		uint2 charset = MCS_langidtocharset(GetScriptManagerVariable(smKeyScript));;
		if (commitedLen != 0)
		{
			if (!charset)
			{
				MCactivefield->stopcomposition(False, True);
				//user switched keyboard to english so we end composition and clear IME
				fixLength = -1;
			}
			MCactivefield->stopcomposition(True,False);
			MCU_multibytetounicode(imetext, commitedLen, unicodeimetext,
			                       imetextsize << 1, unicodelen, charset);
			MCString unicodestr(unicodeimetext, unicodelen);
			MCactivefield->finsertnew( FT_IMEINSERT, unicodestr, 0, true);
		}
		if (fixLength != -1)
			if (imetextsize != fixLength)
			{
				MCactivefield->startcomposition();
				MCU_multibytetounicode(&imetext[commitedLen], imetextsize-commitedLen,
				                       unicodeimetext, imetextsize << 1,
				                       unicodelen, charset);
				MCString unicodestr(unicodeimetext, unicodelen);
				MCactivefield->finsertnew(FT_IMEINSERT, unicodestr, 0, true);
			}
			else if (imetextsize == 0 && fixLength == 0)
				MCactivefield->stopcomposition(True,False);
		if (hiliterangeptr)
		{
			uint2 i;
			for (i = 0; i < hiliterangeptr->fNumOfRanges; i++)
				if (hiliterangeptr->fRange[i].fHiliteStyle == kTSMHiliteCaretPosition)
				{
					MCU_multibytetounicode(&imetext[hiliterangeptr->fRange[i].fStart],
					                       imetextsize-hiliterangeptr->fRange[i].fStart,
					                       unicodeimetext, imetextsize << 1,
					                       unicodelen, charset);
					MCactivefield->setcompositioncursoroffset(unicodelen);
				}
			MCactivefield->setcompositionconvertingrange(0,0);
		}
		delete unicodeimetext;
	}
	else
	{
		// printf("fixlength %d, imetextsize %d, commitedlen %d\n",fixLength,imetextsize,commitedLen);
		if (hiliterangeptr)
		{
			uint2 i;
			for (i = 0; i < hiliterangeptr->fNumOfRanges; i++)
				if (hiliterangeptr->fRange[i].fHiliteStyle == kTSMHiliteCaretPosition)
					MCactivefield->setcompositioncursoroffset(hiliterangeptr
					        ->fRange[i].fStart);
			for (i = 0; i < hiliterangeptr->fNumOfRanges; i++)
				if (hiliterangeptr->fRange[i].fHiliteStyle == kTSMHiliteSelectedConvertedText)
					MCactivefield->setcompositionconvertingrange(hiliterangeptr->fRange[i].fStart,hiliterangeptr->fRange[i].fEnd);
		}
		
		// MW-2012-10-01: [[ Bug 10425 ]] Make sure we only use down/up messages if the input
		//   text is 1 Unicode char long, and maps to MacRoman.
		uint1 t_char;
		if (commitedLen == 2 && MCUnicodeMapToNative((const uint2 *)imetext, 1, t_char))
		{
			// MW-2012-10-30: [[ Bug 10501 ]] Make sure we stop composing
			MCactivefield -> stopcomposition(True, False);
		
			// MW-2008-08-21: [[ Bug 6700 ]] Make sure we generate synthentic keyUp/keyDown events
			//   for MacRoman characters entered using the default IME.
			char tbuf[2];
			tbuf[0] = t_char;
			tbuf[1] = 0;
			MCdispatcher->wkdown(MCactivefield -> getstack() -> getwindow(), tbuf, ((unsigned char *)tbuf)[0]);
			MCdispatcher->wkup(MCactivefield -> getstack() -> getwindow(), tbuf, ((unsigned char *)tbuf)[0]);
		}
		else
		{
			if (commitedLen != 0)
			{
				MCactivefield->stopcomposition(True,False);
				MCString unicodestr(imetext, commitedLen);
				MCactivefield->finsertnew( FT_IMEINSERT, unicodestr, 0, true);
			}
		}
		
		if (fixLength != -1)
			if (imetextsize != fixLength)
			{
				MCactivefield->startcomposition();
				MCString unicodestr(&imetext[commitedLen], imetextsize-commitedLen);
				MCactivefield->finsertnew( FT_IMEINSERT, unicodestr, 0, true);
			}
			else if (imetextsize == 0 && fixLength == 0)
				MCactivefield->stopcomposition(True,False);
	}
	if (hiliterangeptr)
		delete hiliterangeptr;
	delete imetext;
	AEDisposeDesc(&text);
	AEDisposeDesc(&hiliterange);
	return noErr;
}

#ifdef FIX_BUG_5500
pascal OSErr TSMUnicodeNotFromInputHandler(const AppleEvent *theAppleEvent,
        AppleEvent *reply,
        long handlerRefcon)
{
	OSErr err;
	AEDesc text,slr;
	if (!MCactivefield)
		return paramErr;
	ScriptLanguageRecord scriptLangRec;
	err = AEGetParamDesc(theAppleEvent, keyAETSMScriptTag,
	                     typeIntlWritingCode, &slr);
	err = AEGetDescData( &slr, (void *)&scriptLangRec,
	                     sizeof(ScriptLanguageRecord));
	if (err != noErr)
		return err;
	ScriptCode scriptcode = scriptLangRec.fScript;
	uint1 charset = MCS_langidtocharset(scriptcode);
	AEDisposeDesc(&slr);
	err = AEGetParamDesc(theAppleEvent, keyAETheData, typeUnicodeText, &text);
	if (err != noErr)
		return err;
		
	// MW-2008-01-11: [[ Bug 5500 ]] Can't insert non-Unicode characters from character palette
	//   This is caused by no low-level event being passed through to WaitNextEvent when the
	//   AppleEvent is ignored. We fix this by detecting a synthetic event (no low-level key code
	//   in the event record) and using IME insert regardless of character.
	AEDesc event;
	bool t_is_synthetic;
	if (AEGetParamDesc(theAppleEvent, keyAETSMEventRecord, typeLowLevelEventRecord, &event) == noErr)
	{
		EventRecord t_record;
		AEGetDescData(&event, (void *)&t_record, sizeof(EventRecord));
		t_is_synthetic = (t_record . message & 0xFF00) == 0;
		AEDisposeDesc(&event);
	}
	else
		t_is_synthetic = true;
		
	int4 imetextsize;
	imetextsize = AEGetDescDataSize(&text);
	if (!imetextsize)
		return paramErr;
	char *imetext = new char[imetextsize + 1];
	AEGetDescData(&text, (void *) imetext, imetextsize);
	imetext[imetextsize] = '\0';
	if (imetextsize != 0)
	{
		if (imetextsize == 2)
		{
			uint2 *ukey = (uint2 *)imetext;
			//allow ansi characters to be passed as low level keyevent
			if (*ukey <= MAXINT1 && !t_is_synthetic)
			{
				delete imetext;
				AEDisposeDesc(&text);
				return paramErr;
			}
			else if (!charset)
			{
				uint1 tcharset = MCU_unicodetocharset(*ukey);
				if ((tcharset == LCH_ENGLISH || tcharset == LCH_GREEK) && !t_is_synthetic)
				{
					delete imetext;
					AEDisposeDesc(&text);
					return paramErr;
				}
				else
				{
					uint2 tmods = MCscreen->querymods();
					tmods &= ~MS_CAPS_LOCK;
					if (tmods != 0)
					{
						delete imetext;
						AEDisposeDesc(&text);
						return paramErr;
					}
					else
						charset = tcharset;
				}
			}
		}
		MCactivefield->stopcomposition(True,False);
		MCString unicodestr(imetext, imetextsize);
		
		// If the scriptcode is roman, first check to see if we can convert it to the native
		// encoding.
		if (scriptcode == smRoman)
		{
			char t_char;
			uint4 t_len;
			t_len = 1;
			MCS_unicodetomultibyte(unicodestr . getstring(), unicodestr . getlength(), &t_char, 1, t_len, 0);
			if (t_len != 0)
			{
				unicodestr . set(&t_char, 1);
				MCactivefield -> finsert(FT_IMEINSERT, unicodestr, 0);
			}
			else
				MCactivefield -> finsert(FT_IMEINSERT, unicodestr, LCH_ROMAN);
		}
		else
			MCactivefield->finsert(FT_IMEINSERT, unicodestr, charset);
	}
	delete imetext;
	AEDisposeDesc(&text);
	return  noErr;
}
#else
pascal OSErr TSMUnicodeNotFromInputHandler(const AppleEvent *theAppleEvent,
        AppleEvent *reply,
        long handlerRefcon)
{
	OSErr err;
	AEDesc text,slr;
	if (!MCactivefield)
		return paramErr;
	ScriptLanguageRecord scriptLangRec;
	err = AEGetParamDesc(theAppleEvent, keyAETSMScriptTag,
	                     typeIntlWritingCode, &slr);
	err = AEGetDescData( &slr, (void *)&scriptLangRec,
	                     sizeof(ScriptLanguageRecord));
	if (err != noErr)
		return err;
	ScriptCode scriptcode = scriptLangRec.fScript;
	uint1 charset = MCS_langidtocharset(scriptcode);
	AEDisposeDesc(&slr);
	err = AEGetParamDesc(theAppleEvent, keyAETheData, typeUnicodeText, &text);
	if (err != noErr)
		return err;
	int4 imetextsize;
	imetextsize = AEGetDescDataSize(&text);
	if (!imetextsize)
		return paramErr;
	char *imetext = new char[imetextsize + 1];
	AEGetDescData(&text, (void *) imetext, imetextsize);
	imetext[imetextsize] = '\0';
	if (imetextsize != 0)
	{
		if (imetextsize == 2)
		{
			uint2 *ukey = (uint2 *)imetext;
			//allow ansi characters to be passed as low level keyevent
			if (*ukey <= MAXINT1)
			{
				delete imetext;
				AEDisposeDesc(&text);
				return paramErr;
			}
			else if (!charset)
			{
				uint1 tcharset = MCU_unicodetocharset(*ukey);
				if (tcharset == LCH_ENGLISH || tcharset == LCH_GREEK)
				{
					delete imetext;
					AEDisposeDesc(&text);
					return paramErr;
				}
				else
				{
					uint2 tmods = MCscreen->querymods();
					tmods &= ~MS_CAPS_LOCK;
					if (tmods != 0)
					{
						delete imetext;
						AEDisposeDesc(&text);
						return paramErr;
					}
					else
						charset = tcharset;
				}
			}
		}
		MCactivefield->stopcomposition(True,False);
		MCString unicodestr(imetext, imetextsize);
		// we pass charset as keysym to avoid changing keyboards
		MCactivefield->finsertnew(FT_IMEINSERT, unicodestr, charset, true);
	}
	delete imetext;
	AEDisposeDesc(&text);
	return  noErr;
}
#endif
