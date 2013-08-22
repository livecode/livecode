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
#include "field.h"

#include "dispatch.h"
#include "image.h"
#include "stack.h"
#include "card.h"
#include "button.h"
#include "scrolbar.h"
#include "util.h"
#include "execpt.h"
#include "stacklst.h"
#include "sellst.h"
#include "mode.h"

#include "globals.h"

#include "mctheme.h"
#include "context.h"

#include "osxdc.h"

#include "graphicscontext.h"
#include "resolution.h"

uint2 MCScreenDC::ink_table_c[NUM_INKS] =
    {
        srcXor, notSrcBic, srcCopy,
        srcCopy, srcBic, notSrcXor, srcXor, srcOr, srcCopy, notSrcXor,
        srcXor, srcOr, notSrcCopy, notSrcOr, srcCopy, notSrcXor, srcBic,
        notSrcBic, blend, addPin, addOver, subPin, transparent, addMax,
        subOver, adMin
    };

uint2 MCScreenDC::ink_table_m[NUM_INKS] =
    {
        srcXor, adMin, srcCopy,
        srcCopy, notSrcOr, notSrcXor, subOver, addOver, srcBic, srcXor,
        srcXor, srcOr, notSrcCopy, notSrcOr, notSrcCopy, srcBic, srcBic,
        notSrcBic, blend, addPin, addOver, subPin, transparent, addMax,
        subOver, adMin
    };

extern EventHandlerUPP MCS_weh;

extern Boolean tripleclick;

EventHandlerUPP MCScreenDC::s_icon_menu_event_handler_upp;

TSMDocumentID MCScreenDC::tsmdocument = 0;
AEEventHandlerUPP MCScreenDC::TSMPositionToOffsetUPP;
AEEventHandlerUPP MCScreenDC::TSMOffsetToPositionUPP;
AEEventHandlerUPP MCScreenDC::TSMUpdateHandlerUPP;
AEEventHandlerUPP MCScreenDC::TSMUnicodeNotFromInputUPP;
DragTrackingHandlerUPP MCScreenDC::dragmoveUPP;
DragReceiveHandlerUPP MCScreenDC::dragdropUPP;

CFAbsoluteTime MCScreenDC::s_animation_start_time = 0;
CFAbsoluteTime MCScreenDC::s_animation_current_time = 0;

WindowRef *MCScreenDC::c_window_deletions = NULL;
unsigned int MCScreenDC::c_window_deletion_count = 0;

static MCColor accentcolors[8];

////////

void MCScreenDC::setstatus(const char *status)
{ //No action
}

Boolean MCScreenDC::open()
{
	owndnd = False;
	mouseMoveRgn = NewRgn();
	SetRectRgn(mouseMoveRgn, 0, 0, 1, 1);

	//create a invisible window, and set port to this window
	//so that later on at the very first time MC select and set font
	//will only affect in this invisible window, not other apps on the desk top,
	// when MC is first started.  The size of the window is random.
	//  Rect invisibleWinRect;
	Rect invisibleWinRect;
	SetRect(&invisibleWinRect, 0, 0, 20, 20);


	invisibleWin = NewCWindow(nil, &invisibleWinRect, "\p", False,
	                          kUtilityWindowClass, (WindowRef)(-1L), False, 0);



	long response;
	if (Gestalt(gestaltSystemVersion, &response) == noErr)
	{
		if (response >= 0x1030 && response < 0x1040)
			MCantialiasedtextworkaround = True;
		else
			MCantialiasedtextworkaround = False;
	}

	SetGWorld(GetWindowPort(invisibleWin), GetMainDevice());

	vis = new MCVisualInfo;
	
	devdepth = 32;

	black_pixel.red = black_pixel.green = black_pixel.blue = 0; //black pixel
	white_pixel.red = white_pixel.green = white_pixel.blue = 0xFFFF; //white pixel
		black_pixel.pixel = 0;
		white_pixel.pixel = 0xFFFFFF;

			redbits = greenbits = bluebits = 8;
			redshift = 16;
			greenshift = 8;
			blueshift = 0;
			vis->red_mask = 0x00FF0000;
			vis->green_mask = 0x0000FF00;
			vis->blue_mask = 0x000000FF;

	MCzerocolor = MCbrushcolor = white_pixel;
	alloccolor(MCbrushcolor);
	MCselectioncolor = MCpencolor = black_pixel;
	alloccolor(MCselectioncolor);
	alloccolor(MCpencolor);
	gray_pixel.red = gray_pixel.green = gray_pixel.blue = 0x8888;
	alloccolor(gray_pixel);
	background_pixel.red = background_pixel.green = background_pixel.blue = 0xffff;
	alloccolor(background_pixel);

	//query the system for the hilited text color, and set ours
	RGBColor hiliteRGB;
	LMGetHiliteRGB(&hiliteRGB);
	MChilitecolor.red = hiliteRGB.red;
	MChilitecolor.green = hiliteRGB.green;
	MChilitecolor.blue = hiliteRGB.blue;
	alloccolor(MChilitecolor);

	MCColor *syscolors = getaccentcolors();
	if (syscolors != NULL)
		MCaccentcolor = syscolors[4];
	else
	{
		MCaccentcolor.red = MCaccentcolor.green = 0x0000;
		MCaccentcolor.blue = 0x8080;
	}
	alloccolor(MCaccentcolor);

	grabbed = False;
	tripleclick = doubleclick = False;
	MCdoubletime = GetDblTime() * 1000 / 60;
	opened = True;
	mousewindow = new _Drawable;
	activewindow = new _Drawable;
	lastactivewindow = new _Drawable;
	mousewindow->type = activewindow->type = lastactivewindow->type = DC_WINDOW;
	mousewindow->handle.window = activewindow->handle.window
	                             = lastactivewindow->handle.window = 0;

	//get handle of application menu bar
	menuBar = GetMenuBar();
	SetMenuBar(menuBar);  //set menu bar as current menulist
	
	//create Apple menu
	appleMenu = NewMenu(mApple, "\p\024"); //menu title is an apple icon
	InsertMenuItem(appleMenu, "\pAbout...", 0);
	InsertMenu(appleMenu, 0);
	
	DrawMenuBar(); //draw the menu bar with the Apple menu
	usetemp = False;
	Handle tmem = Get1IndResource('TMEM', 1);
	if (tmem != NULL)
	{
		char *ptr = *tmem;
		if (*(ptr + 1))
			usetemp = True;
	}
	MCtemplatescrollbar->alloccolors();
	if (IsMacEmulatedLF()) // no AM
		MCtemplatebutton->allocicons();

	// preallocate these because GetItemMark can't distinguish them
	submenuIDs[0] = 1;
	submenuIDs[checkMark] = 1;
	submenuIDs[diamondMark] = 1;
	
	MCcursors[PI_NONE] = nil;
	MCblinkrate = GetCaretTime() * 1000 / 60;

	MCDisplay const *t_displays;
	getdisplays(t_displays, false);
	MCwbr = t_displays[0] . workarea;

	//TSM - INIT TSM APPLICATION AND INSTALL REQUIRED APPLEVENT HANDLERS
	TSMPositionToOffsetUPP = NewAEEventHandlerUPP(TSMPositionToOffset);
	TSMOffsetToPositionUPP = NewAEEventHandlerUPP(TSMOffsetToPosition);
	TSMUpdateHandlerUPP = NewAEEventHandlerUPP(TSMUpdateHandler);
	TSMUnicodeNotFromInputUPP
	= NewAEEventHandlerUPP(TSMUnicodeNotFromInputHandler);
	AEInstallEventHandler(kTextServiceClass, kPos2Offset,
	                      TSMPositionToOffsetUPP, 0L , False);
	AEInstallEventHandler(kTextServiceClass, kOffset2Pos,
	                      TSMOffsetToPositionUPP, 0L , False);
	AEInstallEventHandler(kTextServiceClass, kUpdateActiveInputArea,
	                      TSMUpdateHandlerUPP, 0L , False);
	AEInstallEventHandler(kTextServiceClass, kUnicodeNotFromInputMethod,
	                      TSMUnicodeNotFromInputUPP, 0L , False);
	openIME();

	dragdropUPP = NewDragReceiveHandlerUPP(DragReceiveHandler);
	InstallReceiveHandler(dragdropUPP, NULL, NULL);
	dragmoveUPP = NewDragTrackingHandlerUPP(DragTrackingHandler);
	InstallTrackingHandler(dragmoveUPP, NULL, NULL);
	
	s_animation_current_time = s_animation_start_time = CFAbsoluteTimeGetCurrent();
	
	//// Dock Menu Initialization
	
	EventTypeSpec t_menu_event_specs[1];
	t_menu_event_specs[0] . eventClass = kEventClassMenu;
	t_menu_event_specs[0] . eventKind = kEventMenuPopulate;
	
	CreateNewMenu(0, 0, &f_icon_menu);
	s_icon_menu_event_handler_upp = NewEventHandlerUPP((EventHandlerProcPtr)handleiconmenuevent);
	InstallEventHandler(GetMenuEventTarget(f_icon_menu), s_icon_menu_event_handler_upp, 1, t_menu_event_specs, NULL, NULL);
	
	t_menu_event_specs[0] . eventClass = kEventClassCommand;
	t_menu_event_specs[0] . eventKind = kEventCommandProcess;
	InstallEventHandler(GetApplicationEventTarget(), s_icon_menu_event_handler_upp, 1, t_menu_event_specs, NULL, NULL);
	
	SetApplicationDockTileMenu(f_icon_menu);
	
	//// Color Profile Initialization
	
	CMGetDefaultProfileBySpace(cmRGBData, &m_dst_profile);

	CMProfileLocation t_location;
	t_location . locType = cmPathBasedProfile;
	strcpy(t_location . u . pathLoc . path, "/System/Library/ColorSync/Profiles/sRGB Profile.icc");
	CMOpenProfile(&m_srgb_profile, &t_location);
	
	////
	
	return True;
}

Boolean MCScreenDC::close(Boolean force)
{
	if (m_dst_profile != nil)
	{
		CMCloseProfile(m_dst_profile);
		m_dst_profile = nil;
	}
	
	if (m_srgb_profile != nil)
	{
		CMCloseProfile(m_srgb_profile);
		m_srgb_profile = nil;
	}
	
	SetApplicationDockTileMenu(NULL);
	ReleaseMenu(f_icon_menu);
	DisposeEventHandlerUPP(s_icon_menu_event_handler_upp);
	f_icon_menu = NULL;
	s_icon_menu_event_handler_upp = NULL;
	showmenu(); //if the menu is hidden, show it.
	finalisebackdrop();
	DisposeRgn(mouseMoveRgn); //dispose the region crated in open()
	uint2 i;
	if (ncolors != 0)
	{
		int2 i;
		for (i = 0 ; i < ncolors ; i++)
		{
			if (colornames[i] != NULL)
				delete colornames[i];
		}
		delete colors;
		delete colornames;
		delete allocs;
	}
	DisposeWindow((WindowPtr)invisibleWin);

	delete vis;
	delete mousewindow;
	delete activewindow;
	delete lastactivewindow;

	//TSM - closes down TSM for this app and removes appleevents
	AERemoveEventHandler(kTextServiceClass, kPos2Offset,
	                     TSMPositionToOffsetUPP, False);
	AERemoveEventHandler(kTextServiceClass, kOffset2Pos,
	                     TSMOffsetToPositionUPP, False);
	AERemoveEventHandler(kTextServiceClass, kUpdateActiveInputArea,
	                     TSMUpdateHandlerUPP, False);
	AERemoveEventHandler(kTextServiceClass, kUnicodeNotFromInputMethod,
	                     TSMUnicodeNotFromInputUPP, False);
	DisposeAEEventHandlerUPP(TSMPositionToOffsetUPP);
	DisposeAEEventHandlerUPP(TSMOffsetToPositionUPP);
	DisposeAEEventHandlerUPP(TSMUpdateHandlerUPP);
	DisposeAEEventHandlerUPP(TSMUnicodeNotFromInputUPP);
	closeIME();





	RemoveReceiveHandler(dragdropUPP, NULL);
	RemoveTrackingHandler(dragmoveUPP, NULL);
	DisposeDragTrackingHandlerUPP(dragmoveUPP);
	DisposeDragReceiveHandlerUPP(dragdropUPP);
	
	opened = False;
	return True;
}

const char *MCScreenDC::getdisplayname()
{
	return "local Mac";
}

void MCScreenDC::grabpointer(Window w)
{
	grabbed = True;
}

void MCScreenDC::ungrabpointer()
{
	grabbed = False;
}

uint16_t MCScreenDC::device_getwidth(void)
{
	GDHandle mainScreen = GetMainDevice();
	HLock((Handle)mainScreen);
	uint2 swidth = ((GDPtr)*mainScreen)->gdRect.right
	               - ((GDPtr)*mainScreen)->gdRect.left;
	HUnlock((Handle)mainScreen);
	return swidth;
}

uint16_t MCScreenDC::device_getheight(void)
{
	GDHandle mainScreen = GetMainDevice();
	HLock((Handle)mainScreen);
	uint2 sheight = ((GDPtr)*mainScreen)->gdRect.bottom
	                - ((GDPtr)*mainScreen)->gdRect.top;
	HUnlock((Handle)mainScreen);
	return sheight;
}

uint2 MCScreenDC::getmaxpoints()
{//max points defined in a polygon on Mac quick Draw
	return 32767;
}

uint2 MCScreenDC::getvclass()
{
	return DirectColor;
}

MCColor* MCScreenDC::getaccentcolors()
{
	
	SInt32 response;
	if (Gestalt(gestaltAppearanceVersion, &response) == noErr)
	{ // 1.0.1 or later
		// theme drawing only supported in version 1.1, OS 8.5 and later
		RegisterAppearanceClient(); // no harm in calling this multiple times
		if (response < 0x0110)
			MClook = LF_MAC;
		else
		{
			ThemeScrollBarThumbStyle tstyle;
			GetThemeScrollBarThumbStyle(&tstyle);
			MCproportionalthumbs = tstyle == kThemeScrollBarThumbProportional;
		}
		//get the 8 accent colors, for drawing scroll bar
		CTabHandle themeCT; //handle to the Theme color table
		if (GetThemeAccentColors(&themeCT) == noErr)
		{
			uint2 i = MCU_min(8, (*themeCT)->ctSize + 1);
			while (i--)
			{
				accentcolors[i].red = (*themeCT)->ctTable[i].rgb.red;
				accentcolors[i].green = (*themeCT)->ctTable[i].rgb.green;
				accentcolors[i].blue = (*themeCT)->ctTable[i].rgb.blue;
			}
			return accentcolors;
		}
		return NULL;
	}
	
	MClook = LF_MAC;
	return NULL;
}

static void getMouseLoc(Point *mouseLoc)
{
	GetMouse(mouseLoc);
	LocalToGlobal(mouseLoc);
}

void MCScreenDC::enactraisewindows(void)
{
	if (MCraisewindows)
		initialisebackdrop();
	
	MCstacks -> refresh();
}

void MCScreenDC::openwindow(Window w, Boolean override)
{
	if (IsWindowVisible((WindowPtr)w->handle.window))
		return;
		
	if (override)
	{
        // MM-2012-04-02: Use new MC*Window wrapper function - fixes bugs where a cocoa NSWindow has been
        //  created from the carbon WindowRef.
		void MCBringWindowToFront(void *);
		MCBringWindowToFront(w->handle.window);
	}
	else
		activatewindow(w);

	Window parentwindow;
	MCStack *tstack = MCdispatcher->findstackd(w);
	bool t_ismodal = false;
	if (tstack != NULL)
	{
		parentwindow = tstack->getparentwindow();
		if (tstack -> getmode() == WM_MODAL)
			t_ismodal = true;
	}
	if (parentwindow == DNULL)
		parentwindow = MCdefaultstackptr->getw();
	if (GetWRefCon((WindowPtr)w->handle.window) == WM_SHEET)
	{
		if (GetWRefCon((WindowPtr)parentwindow->handle.window) == WM_SHEET
		        || GetWRefCon((WindowPtr)parentwindow->handle.window) == WM_MODAL || !IsWindowVisible((WindowPtr)parentwindow -> handle . window))
		{
			SetWRefCon((WindowPtr)w->handle.window, WM_MODAL);
            // MM-2012-04-02: Use new MC*Window wrapper function - fixes bugs where a cocoa NSWindow has been
            //  created from the carbon WindowRef.
			extern void MCShowHideWindow(void *, bool);
			MCShowHideWindow(w->handle.window, true);
		}
		else
		{
			ShowSheetWindow((WindowPtr)w->handle.window, (WindowPtr)parentwindow->handle.window);
			WindowAttributes watt;
			GetWindowAttributes((WindowPtr)parentwindow->handle.window,&watt);
			MCStack *s = NULL;
		}
	}
	else
		if (GetWRefCon((WindowPtr)w->handle.window) == WM_DRAWER)
		{
			if (GetWRefCon((WindowPtr)parentwindow->handle.window) == WM_SHEET
			        || GetWRefCon((WindowPtr)parentwindow->handle.window) == WM_MODAL ||
			        GetWRefCon((WindowPtr)parentwindow->handle.window) == WM_DRAWER)
			{
                // MM-2012-04-02: Use new MC*Window wrapper function - fixes bugs where a cocoa NSWindow has been
                //  created from the carbon WindowRef.
				extern void MCShowHideWindow(void *, bool);
				MCShowHideWindow(w->handle.window, true);
			}
			else
			{
				OpenDrawer((WindowPtr)w->handle.window,kWindowEdgeDefault,False);
				MCdispatcher->configure(w); //opendrawer moves and resizes draw
			}
		}
		else
		{
			// MW-2007-05-03: [[ Bug 4831 ]] It's possible to get here with tstack == NULL.
			//   This is very strange since this routine is called from MCStack...
			//
			if (tstack != NULL)
				assignbackdrop(tstack -> getmode(), w);
            // MM-2012-04-02: Use new MC*Window wrapper function - fixes bugs where a cocoa NSWindow has been
            //  created from the carbon WindowRef.
			extern void MCShowHideWindow(void *, bool);
			MCShowHideWindow(w->handle.window, true);
			if (activewindow -> handle . window == w -> handle . window)
			{
                // MM-2012-04-02: Use new MC*Window wrapper function - fixes bugs where a cocoa NSWindow has been
                //  created from the carbon WindowRef.
				void MCBringWindowToFront(void *);
				MCBringWindowToFront(w->handle.window);
				
				// MW-2008-03-17: [[ Bug 5877 ]] Make sure we actually focus the target window
				//   if override is not true.
				if (!override && !t_ismodal)
					SetUserFocusWindow((WindowPtr)w -> handle . window);
			}
		}
	if (!override && !mdown && !grabbed)
	{
		Point mouseLoc;
		getMouseLoc(&mouseLoc);
		MCdispatcher->wmunfocus(mousewindow);
		mousewindow->handle.window = NULL;
		//do the window focusing/unfocusing and change the cursor stuff
		mfocus(NULL, mouseLoc, True);
	}
	else if (t_ismodal && mdown)
	{
		// MW-2008-08-30: [[ Bug 2479 ]] If a modal dialog is open we don't receive matching mouseUp/mouseDown
		//   events, so we have to simulate one here.
		Point mouseLoc;
		getMouseLoc(&mouseLoc);
		MCbuttonstate = 0;
		mdown = False;
		setmods(0);
		MCdispatcher -> wmup(mousewindow, 1);
		MCdispatcher -> wmunfocus(mousewindow);
		mousewindow -> handle . window = NULL;
		mfocus(NULL, mouseLoc, True);
	}
}

void MCScreenDC::closewindow(Window w)
{
	if (!IsWindowVisible((WindowPtr)w->handle.window) && MCstacks->getactive())
		return;
	if (w->handle.window == activewindow->handle.window)
	{
		MCdispatcher->wkunfocus(activewindow);
		activewindow->handle.window = NULL;
	}
	if (w->handle.window == lastactivewindow->handle.window)
		lastactivewindow->handle.window = NULL;
	Point mouseLoc;
	if (!mdown && !grabbed)
	{
		mousewindow->handle.window = NULL;
		MCmousestackptr = NULL;
		getMouseLoc(&mouseLoc);
	}

	if (GetWRefCon((WindowPtr)w->handle.window) == WM_SHEET)
	{
		WindowRef pw;
		GetSheetWindowParent((WindowPtr)w->handle.window,&pw);
		HideSheetWindow((WindowPtr)w->handle.window);
		WindowAttributes watt;
		GetWindowAttributes(pw,&watt);
		_Drawable d;
		d.type = DC_WINDOW;
		d.handle.window = (MCSysWindowHandle)pw;
	}
	else if (GetWRefCon((WindowPtr)w->handle.window) == WM_DRAWER)
	{
		//turn off live resizing for parent
		WindowRef parentwindow = GetDrawerParent((WindowPtr)w->handle.window);
		if (parentwindow != NULL)
		{
			_Drawable d;
			d.type = DC_WINDOW;
			d.handle.window = (MCSysWindowHandle)parentwindow;
			MCStack *pstack = MCdispatcher->findstackd(&d);
			if (pstack)
			{
				pstack->setparentwindow(DNULL);
				if (!pstack->getflag(F_DECORATIONS) || !(pstack->getdecorations() & WD_LIVERESIZING))
					ChangeWindowAttributes(parentwindow, 0, kWindowLiveResizeAttribute);
			}
		}
		CloseDrawer((WindowPtr)w->handle.window,False);
	}
	else
	{
        // MM-2012-04-02: Use new MC*Window wrapper function - fixes bugs where a cocoa NSWindow has been
        //  created from the carbon WindowRef.
		extern void MCShowHideWindow(void *, bool);
		MCShowHideWindow(w->handle.window, false);
	}

	if (GetWRefCon((WindowPtr)w->handle.window) < WM_PULLDOWN)
	{
		WindowPtr theWindow = FrontWindow();
		_Drawable twindow;
		twindow.type = DC_WINDOW;
		while (theWindow)
		{
			twindow.handle.window = (MCSysWindowHandle)theWindow;
			MCStack *sptr;
			if (twindow.handle.window != w->handle.window
			        && (sptr = MCdispatcher->findstackd(&twindow)) != NULL
			        && sptr->getopened() && (GetWRefCon(theWindow) < WM_PALETTE
			                                 || GetWRefCon(theWindow) == WM_MODAL))
			{
				activatewindow(&twindow);
				break;
			}
			theWindow = GetNextWindow(theWindow);
		}
	}
	if (!mdown && !grabbed)
		mfocus(NULL, mouseLoc, True);
}

void MCScreenDC::destroywindow(Window &w)
{
	WindowRef *t_new_window_deletions;
	t_new_window_deletions = (WindowRef *)realloc(c_window_deletions, sizeof(WindowRef) * (c_window_deletion_count + 1));
	if (t_new_window_deletions != NULL)
	{
		t_new_window_deletions[c_window_deletion_count++] = (WindowPtr)w -> handle . window;
		c_window_deletions = t_new_window_deletions;
	}
	
    // MM-2012-04-02: Use new MC*Window wrapper function - fixes bugs where a cocoa NSWindow has been
    //  created from the carbon WindowRef.
	extern void MCHideWindow(void *);
	MCHideWindow(w->handle.window);

	delete w;
	w = DNULL;
}

void MCScreenDC::raisewindow(Window window)
{
	Window bw = MCstacks->restack(NULL);
	
	// MW-2005-10-31: It is possible for window == NULL in some cases so exit
	//   if this is the case
	if (window == NULL)
		return;
	
    // MM-2012-04-02: Use new MC*Window wrapper function - fixes bugs where a cocoa NSWindow has been
    //  created from the carbon WindowRef.
	void MCBringWindowToFront(void *);
	MCBringWindowToFront(window->handle.window);
}

void MCScreenDC::iconifywindow(Window window)
{
	CollapseWindow((WindowPtr)window->handle.window, True);
}

void MCScreenDC::uniconifywindow(Window window)
{
	CollapseWindow((WindowPtr)window->handle.window, False);
}

// MW-2007-07-09: [[ Bug 3226 ]] Update the call to take 'newname' as UTF-8
void MCScreenDC::setname(Window w, const char *newname)
{
	CFStringRef t_title;
	t_title = convertutf8tocf(newname);
	// MW-2013-07-01: [[ Bug 11006 ]] If we failed to create a CFString from the
	//   UTF-8 it must be malformed. In theory this shouldn't happen, so as a
	//   fallback, just create a CFString as if the string were MacRoman.
	if (t_title == nil)
		t_title = CFStringCreateWithCString(kCFAllocatorDefault, newname, kCFStringEncodingMacRoman);
	if (t_title != nil)
	{
		SetWindowTitleWithCFString((WindowPtr)w -> handle . window, t_title);
		CFRelease(t_title);
	}
}

void MCScreenDC::setcmap(MCStack *sptr)
{// no action
}

void MCScreenDC::sync(Window w)
{
	flush(w);
}

void MCScreenDC::flush(Window w)
{
	if (w != DNULL)
	{
		CGrafPtr t_port;
		t_port = GetWindowPort((WindowPtr)w -> handle . window);
		if (t_port != NULL)
			QDFlushPortBuffer(GetWindowPort((WindowPtr)w->handle.window), NULL);
	}
}

void MCScreenDC::beep()
{
	SndSetSysBeepState(sysBeepEnable | sysBeepSynchronous);
	SysBeep(beepduration / 16);
}

void MCScreenDC::setinputfocus(Window window)
{
	if (window->handle.window != activewindow->handle.window)
		activatewindow(window);
}

void MCScreenDC::freepixmap(Pixmap &p)
{
	if (p != DNULL)
	{
		DisposeGWorld((CGrafPtr)p->handle.pixmap);
		delete p;
		p = DNULL;
	}
}

Pixmap MCScreenDC::createpixmap(uint2 width, uint2 height,
                                uint2 depth, Boolean purge)
{
	GWorldPtr hpixmap = NULL;
	Rect r;
	r.top = r.left = 0;
	r.bottom = height;
	r.right = width;
	GWorldFlags f = 0;
	if (depth != 1)
		depth = 32;
	if (usetemp && width * height * depth > MAXUINT2 || FreeMem() < (MAXUINT2 * 8))
		f = useTempMem;
	QDErr err = NewGWorld(&hpixmap, depth, &r, NULL, NULL, MCmajorosversion >= 0x1040 ? kNativeEndianPixMap : 0);
	if (hpixmap == NULL || err != noErr)
		return DNULL;
	Pixmap pm = new _Drawable;
	pm->type = DC_BITMAP;
	pm->handle.pixmap = (MCSysBitmapHandle)hpixmap;
	return pm;
}

bool MCScreenDC::lockpixmap(Pixmap p_pixmap, void*& r_bits, uint4& r_stride)
{
	PixMapHandle t_src_pixmap;
	
	t_src_pixmap = GetGWorldPixMap((CGrafPtr)p_pixmap -> handle . pixmap);
	LockPixels(t_src_pixmap);
	HLock((Handle)t_src_pixmap);
	
	r_bits = GetPixBaseAddr(t_src_pixmap);
	r_stride = GetPixRowBytes(t_src_pixmap);

	return true;
}

void MCScreenDC::unlockpixmap(Pixmap p_pixmap, void *p_bits, uint4 p_stride)
{
	PixMapHandle t_src_pixmap;
	
	t_src_pixmap = GetGWorldPixMap((CGrafPtr)p_pixmap -> handle . pixmap);
	HUnlock((Handle)t_src_pixmap);
	UnlockPixels(t_src_pixmap);
}

bool MCScreenDC::device_getwindowgeometry(Window w, MCRectangle &drect)
{//get the client window's geometry in screen coord
	if (w == DNULL || w->handle.window == 0)
		return false;


	RgnHandle r = NewRgn();
	GetWindowRegion((WindowPtr)w->handle.window, kWindowContentRgn, r);
	Rect windowRect;
	GetRegionBounds(r, &windowRect);
	DisposeRgn(r);
	drect = MCMacRectToMCRect(windowRect);
	if (drect.height == 0)
		drect.x = drect.y = -1; // windowshaded, so don't move it

	return true;
}

Boolean MCScreenDC::getpixmapgeometry(Pixmap p, uint2 &w, uint2 &h, uint2 &d)
{
	if (p != DNULL)
	{
		PixMapHandle hpm = GetGWorldPixMap((CGrafPtr)p->handle.pixmap);
		Rect r;
		GetPixBounds(hpm, &r);
		w = r.right - r.left;
		h = r.bottom - r.top;
		d = GetPixDepth(hpm);
		return True;
	}
	return False;
}

void MCScreenDC::setgraphicsexposures(Boolean on, MCStack *sptr)
{
	ge = on;
}

void MCScreenDC::copyarea(Drawable s, Drawable d, int2 depth,
                          int2 sx, int2 sy, uint2 sw, uint2 sh, int2 dx,
                          int2 dy, uint4 rop)
{
	if (s == DNULL || d == DNULL)
		return;
	CGrafPtr oldport;
	GDHandle olddevice;
	GetGWorld(&oldport, &olddevice);
	if (ge && d->type == DC_WINDOW && s == d && rop == GXcopy)
	{
		SetGWorld(GetWindowPort((WindowPtr)d->handle.window), GetMainDevice());
		Rect r;
		r.top = MCU_min(sy, dy);
		r.left = MCU_min(sx, dx);
		r.right = MCU_max(sx + sw, dx + sw);
		r.bottom = MCU_max(sy + sh, dy + sh);
		RgnHandle dirty = NewRgn();
		Rect windRect;
		RectRgn(dirty, GetPortBounds(GetWindowPort((WindowPtr)d->handle.window), &windRect));
		SetClip(dirty);
		ScrollRect(&r, dx - sx, dy - sy, dirty);
		r.left = sx <= dx ? sx : dx + sw;
		r.top = sy <= dy ? sy : dy + sh;
		r.right = sx == dx ? sx + sw : r.left + MCU_abs(sx - dx);
		r.bottom = sy == dy ? sy + sh : r.top + MCU_abs(sy - dy);
		RgnHandle clean = NewRgn();
		RectRgn(clean, &r);
		DiffRgn(dirty, clean, dirty);
		if (!EmptyRgn(dirty))
		{
			InvalWindowRgn((WindowPtr)d->handle.window, dirty);
			expose();
		}
		DisposeRgn(dirty);
		DisposeRgn(clean);
	}
	else
	{
		if (d -> type == DC_WINDOW)
			SetGWorld(GetWindowPort((WindowPtr)d -> handle . window), GetMainDevice());
		else
			SetGWorld((CGrafPtr)d->handle.pixmap, NULL);
		ForeColor(blackColor);
		BackColor(whiteColor);
		copybits(s, d, sx, sy, sw, sh, dx, dy, ink_table_c[rop]);
	}
	SetGWorld(oldport, olddevice);
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

Boolean MCScreenDC::uint4towindow(uint4 id, Window &w)
{
	w = new _Drawable;
	w->type = DC_WINDOW;
	w->handle.window = (MCSysWindowHandle)id;
	return True;
}

void MCScreenDC::getbeep(uint4 which, MCExecPoint &ep)
{
	long v;
	switch (which)
	{
	case P_BEEP_LOUDNESS:
		GetSysBeepVolume(&v);
		ep.setint(v);
		break;
	case P_BEEP_PITCH:
		ep.setint(beeppitch);
		break;
	case P_BEEP_DURATION:
		ep.setint(beepduration);
		break;
	}
}

void MCScreenDC::setbeep(uint4 which, int4 beep)
{
	switch (which)
	{
	case P_BEEP_LOUDNESS:
		SetSysBeepVolume(beep);
		break;
	case P_BEEP_PITCH:
		if (beep == -1)
			beeppitch = 1440;
		else
			beeppitch = beep;
		break;
	case P_BEEP_DURATION:
		if (beep == -1)
			beepduration = 500;
		else
			beepduration = beep;
		break;
	}
}

void MCScreenDC::getvendorstring(MCExecPoint &ep)
{
	ep.setstaticcstring("Mac OS");
}

uint2 MCScreenDC::getpad()
{ //return the boundary each scan line is padded to.
	return 32;
}

Window MCScreenDC::getroot()
{
	static Drawable mydrawable;
	GrafPtr wMgrPort = NULL;  //window manager port == desktop port
	if (mydrawable == DNULL)
		mydrawable = new _Drawable;
	mydrawable->type = DC_WINDOW;
	mydrawable->handle.window = (MCSysWindowHandle)wMgrPort; //broken
	return mydrawable;
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
		
	if (backdrop_window == NULL)
		t_error = !initialisebackdrop();
	
	if (!t_error)
	{
		MCRectangle t_rect;
		MCU_set_rect(t_rect, 0, 0, device_getwidth(), device_getheight());
		updatebackdrop(t_rect);
		
		MCstacks -> refresh();
		ShowWindow(backdrop_window);
	}
	else
	{
		backdrop_active = False;
		MCstacks -> refresh();
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
		HideWindow(backdrop_window);
		if ( !menubarhidden )
			SetSystemUIMode(kUIModeNormal, NULL);
		MCstacks -> refresh();
	}
	else
	{
		MCRectangle t_rect;
		MCU_set_rect(t_rect, 0, 0, device_getwidth(), device_getheight());
		updatebackdrop(t_rect);
	}
}

bool MCScreenDC::initialisebackdrop(void)
{
	OSStatus t_status;

	if (backdrop_window != NULL)
		return true;
		
	bool t_error;
	t_error = false;

	Rect t_bounds;
	SetRect(&t_bounds, 0, 0, device_getwidth(), device_getheight());
	t_error = CreateNewWindow(kPlainWindowClass, kWindowNoAttributes, &t_bounds, &backdrop_window) != noErr;
	if (!t_error)
		t_error = CreateWindowGroup(kWindowGroupAttrLayerTogether | kWindowGroupAttrSelectAsLayer, &backdrop_group);
	if (!t_error)
		t_error = CreateWindowGroup(0, &backdrop_background_group);
	if (!t_error)
		t_error = CreateWindowGroup(0, &backdrop_document_group);
	if (!t_error)
		t_error = CreateWindowGroup(0, &backdrop_palette_group);
	if (!t_error)
	{
		t_status = SetWindowGroupParent(backdrop_background_group, backdrop_group);
		t_status = SetWindowGroupParent(backdrop_document_group, backdrop_group);
		t_status = SetWindowGroupParent(backdrop_palette_group, backdrop_group);
		t_status = SetWindowGroupParent(backdrop_group, GetWindowGroupOfClass(kFloatingWindowClass));
		
		t_status = SendWindowGroupBehind(backdrop_document_group, backdrop_palette_group);
		t_status = SendWindowGroupBehind(backdrop_background_group, backdrop_document_group);
	
		t_status = SetWindowGroup(backdrop_window, backdrop_background_group);
	}
	if (!t_error)
	{
		EventTypeSpec list[] =
		{
			{kEventClassWindow, kEventWindowUpdate},
			{kEventClassWindow, kEventWindowDrawContent}
		};

		EventHandlerRef ref;
		
		InstallWindowEventHandler(backdrop_window, MCS_weh, 2, list, backdrop_window, &ref);
	}

	return !t_error;
}

void MCScreenDC::finalisebackdrop(void)
{
	if (backdrop_palette_group != NULL)
	{
		ReleaseWindowGroup(backdrop_palette_group);
		backdrop_palette_group = NULL;
	}
	
	if (backdrop_document_group != NULL)
	{
		ReleaseWindowGroup(backdrop_document_group);
		backdrop_document_group = NULL;
	}
	
	if (backdrop_background_group != NULL)
	{
		ReleaseWindowGroup(backdrop_background_group);
		backdrop_background_group = NULL;
	}
	
	if (backdrop_group != NULL)
	{
		ReleaseWindowGroup(backdrop_group);
		backdrop_group = NULL;
	}
	
	if (backdrop_window != NULL)
	{
		ReleaseWindow(backdrop_window);
		backdrop_window = NULL;
	}
}

void MCScreenDC::configurebackdrop(const MCColor& p_colour, MCPatternRef p_pattern, MCImage *p_badge)
{
	if (backdrop_badge != p_badge || backdrop_pattern != p_pattern || backdrop_colour . red != p_colour . red || backdrop_colour . green != p_colour . green || backdrop_colour . blue != p_colour . blue)
	{
		backdrop_badge = p_badge;
		backdrop_pattern = p_pattern;
		backdrop_colour = p_colour;
	
		alloccolor(backdrop_colour);
	
		if (backdrop_active || backdrop_hard)
		{
			MCRectangle t_rect;
			MCU_set_rect(t_rect, 0, 0, device_getwidth(), device_getheight());
	
			updatebackdrop(t_rect);
		}
	}
}

void MCScreenDC::assignbackdrop(Window_mode p_mode, Window p_window)
{
	WindowGroupRef t_group;
	WindowClass t_class;
	
	// MW-2006-05-18: [[ Bug 3592 ]] We need to pass a different mode if we are a 'system window'
	GetWindowClass((WindowPtr)p_window -> handle . window, &t_class);
	if (t_class == kUtilityWindowClass)
	  p_mode = WM_LAST;
	
	switch(p_mode)
	{
		case WM_LAST:
			t_group = GetWindowGroupOfClass(kUtilityWindowClass);
		break;
	
		case WM_TOP_LEVEL:
		case WM_TOP_LEVEL_LOCKED:
		case WM_MODELESS:
		case WM_CLOSED:
			if (backdrop_active || backdrop_hard || MCraisewindows)
				t_group = backdrop_document_group;
			else
				t_group = GetWindowGroupOfClass(kDocumentWindowClass);
		break;
		
		case WM_PALETTE:
			if (backdrop_active || backdrop_hard || MCraisewindows)
				t_group = backdrop_palette_group;
			else
				t_group = GetWindowGroupOfClass(kFloatingWindowClass);
		break;
		
		default:
			return;
	}

	OSStatus t_status;
	t_status = SetWindowGroup((WindowPtr)p_window -> handle . window, t_group);
}

void MCScreenDC::updatebackdrop(const MCRectangle& p_dirty)
{
	MCGContextRef t_context = nil;
	MCGraphicsContext *t_gfxcontext = nil;
	
	CGrafPtr t_gptr = nil;
	PixMapHandle t_pix = nil;
	Rect t_bounds;
	Rect t_pix_bounds;
	
	void *t_bits = nil;
	uint32_t t_stride;
	
	/* UNCHECKED */ t_gptr = GetWindowPort(backdrop_window);
	LockPortBits(t_gptr);
	/* UNCHECKED */ t_pix = GetPortPixMap(t_gptr);
	LockPixels(t_pix);
	HLock((Handle)t_pix);
	
	/* UNCHECKED */ t_bits = GetPixBaseAddr(t_pix);
	t_stride = GetPixRowBytes(t_pix);
	GetPortBounds(t_gptr, &t_bounds);
	GetPixBounds(t_pix, &t_pix_bounds);
	
	t_bits = (uint8_t*)t_bits + t_stride * -t_pix_bounds.top + sizeof(uint32_t) * -t_pix_bounds.left;
	
	/* UNCHECKED */ MCGContextCreateWithPixels(t_bounds.right - t_bounds.left, t_bounds.bottom - t_bounds.top, t_stride, t_bits, true, t_context);
	/* UNCHECKED */ t_gfxcontext = new MCGraphicsContext(t_context);
	
	t_gfxcontext->setclip(p_dirty);
	redrawbackdrop(t_gfxcontext, p_dirty);

	delete t_gfxcontext;
	MCGContextRelease(t_context);
	
	HUnlock((Handle)t_pix);
	UnlockPixels(t_pix);
	UnlockPortBits(t_gptr);
}

void MCScreenDC::redrawbackdrop(MCContext *p_context, const MCRectangle& p_dirty)
{
	p_context -> setforeground(backdrop_colour);

	if (backdrop_pattern != NULL)
		p_context -> setfillstyle(FillTiled, backdrop_pattern, 0, 0);
	else
		p_context -> setfillstyle(FillSolid, NULL, 0, 0);
	
	p_context -> fillrect(p_dirty);
	
	if (backdrop_badge != NULL && backdrop_hard)
	{
		MCRectangle t_rect;
		t_rect = backdrop_badge -> getrect();
		backdrop_badge -> drawme(p_context, 0, 0, t_rect . width, t_rect . height, 32, getheight() - 32 - t_rect . height);
	}
}

// MW-2007-8-29: [[ Bug 4691 ]] Make sure the working screenrect is adjusted appropriately when we show/hide the menubar
void MCScreenDC::hidemenu()
{
	HideMenuBar();
	menubarhidden = true ;
	if (s_monitor_count != 0)
	{
		delete[] s_monitor_displays;
		s_monitor_displays = NULL;
		s_monitor_count = 0;
	}
}

void MCScreenDC::showmenu()
{
	ShowMenuBar();
	menubarhidden = false ;
	if (s_monitor_count != 0)
	{
		delete[] s_monitor_displays;
		s_monitor_displays = NULL;
		s_monitor_count = 0;
	}
}
