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

#include "core.h"
#include "globdefs.h"
#include "parsedef.h"
#include "filedefs.h"
#include "execpt.h"
#include "mcerror.h"
#include "util.h"
#include "objdefs.h"
#include "stack.h"
#include "stacklst.h"
#include "osspec.h"

#include "globals.h"
#include "dispatch.h"
#include "eventqueue.h"
#include "redraw.h"
#include "notify.h"
#include "scrolbar.h"
#include "mctheme.h"
#include "button.h"

#include "mode.h"

#include "osxdc.h"

#import <Cocoa/Cocoa.h>

extern MCRectangle NSRectToMCRectangleGlobal(NSRect r);

////////////////////////////////////////////////////////////////////////////////

void MCScreenDC::enactraisewindows(void)
{
#ifdef OLD_MAC
	if (MCraisewindows)
		initialisebackdrop();
	
	MCstacks -> refresh();
#endif
}

void MCScreenDC::openwindow(Window w, Boolean override)
{
	NSWindow *t_window;
	t_window = (NSWindow *)w;
	
	if ([t_window isVisible])
		return;
	
	if (override)
		[t_window orderFront: nil];
	else
		[t_window makeKeyAndOrderFront: nil];
	
	//[[t_window contentView] syncMouseInsideState];
	
	// Process sheets / drawers
	
#ifdef OLD_MAC
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
#endif
}

void MCScreenDC::closewindow(Window w)
{
	NSWindow *t_window;
	t_window = (NSWindow *)w;

	if (![t_window isVisible])
		return;
	
	[t_window orderOut: nil];
	
#ifdef OLD_MAC
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
#endif
}

void MCScreenDC::destroywindow(Window &w)
{
	NSObject *t_delegate;
	t_delegate = [(NSWindow *)w delegate];
	[(NSWindow *)w setDelegate: nil];
	[t_delegate release];
	[(NSWindow *)w release];
	w = nil;
	
#ifdef OLD_MAC
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
#endif
}

void MCScreenDC::raisewindow(Window window)
{
	[(NSWindow *)window orderFront: nil];
	
#ifdef OLD_MAC
	Window bw = MCstacks->restack(NULL);
	
	// MW-2005-10-31: It is possible for window == NULL in some cases so exit
	//   if this is the case
	if (window == NULL)
		return;
	
    // MM-2012-04-02: Use new MC*Window wrapper function - fixes bugs where a cocoa NSWindow has been
    //  created from the carbon WindowRef.
	void MCBringWindowToFront(void *);
	MCBringWindowToFront(window->handle.window);
#endif
}

void MCScreenDC::iconifywindow(Window window)
{
	[(NSWindow *)window miniaturize: nil];
#ifdef OLD_MAC
	CollapseWindow((WindowPtr)window->handle.window, True);
#endif
}

void MCScreenDC::uniconifywindow(Window window)
{
	[(NSWindow *)window deminiaturize: nil];
#ifdef OLD_MAC
	CollapseWindow((WindowPtr)window->handle.window, False);
#endif
}

// MW-2007-07-09: [[ Bug 3226 ]] Update the call to take 'newname' as UTF-8
void MCScreenDC::setname(Window w, const char *newname)
{
	[(NSWindow *)w setTitle: [NSString stringWithCString: newname encoding: NSUTF8StringEncoding]];
	 
#ifdef OLD_MAC
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
#endif
}

void MCScreenDC::setinputfocus(Window window)
{
	[(NSWindow *)window makeKeyAndOrderFront: nil];
	
#ifdef OLD_MAC
	if (window->handle.window != activewindow->handle.window)
		activatewindow(window);
#endif
}

bool MCScreenDC::device_getwindowgeometry(Window w, MCRectangle &drect)
{
	if (w == nil)
		return false;
	
	drect = NSRectToMCRectangleGlobal([(NSWindow *)w contentRectForFrameRect: [(NSWindow *)w frame]]);
	return true;
#ifdef OLD_MAC
	//get the client window's geometry in screen coord
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
#endif
}

uint4 MCScreenDC::dtouint4(Drawable d)
{
#ifdef OLD_MAC
	if (d == DNULL)
		return 0;
	else
		if (d->type == DC_WINDOW)
			return (uint4)(d->handle.window);
		else
			return (uint4)(d->handle.pixmap);
#endif
}

Boolean MCScreenDC::uint4towindow(uint4 id, Window &w)
{
#ifdef OLD_MAC
	w = new _Drawable;
	w->type = DC_WINDOW;
	w->handle.window = (MCSysWindowHandle)id;
	return True;
#endif
}

////////////////////////////////////////////////////////////////////////////////

void MCScreenDC::enablebackdrop(bool p_hard)
{
#ifdef OLD_MAC
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
#endif
}

void MCScreenDC::disablebackdrop(bool p_hard)
{
#ifdef OLD_MAC
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
#endif
}

bool MCScreenDC::initialisebackdrop(void)
{
#ifdef OLD_MAC
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
#endif
}

void MCScreenDC::finalisebackdrop(void)
{
#ifdef OLD_MAC
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
#endif
}

void MCScreenDC::configurebackdrop(const MCColor& p_colour, MCPatternRef p_pattern, MCImage *p_badge)
{
#ifdef OLD_MAC
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
#endif
}

void MCScreenDC::assignbackdrop(Window_mode p_mode, Window p_window)
{
#ifdef OLD_MAC
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
#endif
}

void MCScreenDC::updatebackdrop(const MCRectangle& p_dirty)
{
#ifdef OLD_MAC
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
#endif
}

void MCScreenDC::redrawbackdrop(MCContext *p_context, const MCRectangle& p_dirty)
{
#ifdef OLD_MAC
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
#endif
}

////////////////////////////////////////////////////////////////////////////////
