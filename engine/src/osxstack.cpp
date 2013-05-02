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
#include "group.h"
#include "player.h"
#include "field.h"
#include "stacklst.h"
#include "cardlst.h"
#include "sellst.h"
#include "mcerror.h"
#include "util.h"
#include "param.h"
#include "execpt.h"
#include "debug.h"
#include "globals.h"
#include "mode.h"
#include "image.h"
#include "redraw.h"
#include "license.h"
#include "context.h"
#include "region.h"

#include "osxdc.h"

////////////////////////////////////////////////////////////////////////////////

// MW-2011-09-13: [[ Redraw ]] If non-nil, this pixmap is used in the next
//   HIView update.
static Pixmap s_update_pixmap = nil;

////////////////////////////////////////////////////////////////////////////////

OSStatus MCRevolutionStackViewCreate(MCStack *p_stack, ControlRef* r_control);

////////////////////////////////////////////////////////////////////////////////

extern EventHandlerUPP MCS_weh;

RgnHandle getWindowContentRegion(WindowRef window,RgnHandle contentRegion)
{
	Rect portBounds;
	GetWindowBounds(window, kWindowGlobalPortRgn, &portBounds);
	RectRgn(contentRegion, &portBounds);
	return contentRegion;
}

static pascal long BorderlessWindowProc(short varCode, WindowRef window,
                                        short message, long param)
{
#pragma unused( varCode ) 
	switch (message)
	{
	case kWindowMsgGetFeatures:
		*(OptionBits*) param = kWindowCanGetWindowRegion
		                       | kWindowDefSupportsColorGrafPort;
		return 1;
	case kWindowMsgGetRegion:
		{
			GetWindowRegionRec* rgnRec  = (GetWindowRegionRec*) param;
			switch (rgnRec->regionCode)
			{
			case kWindowTitleBarRgn:
			case kWindowTitleTextRgn:
			case kWindowCloseBoxRgn:
			case kWindowZoomBoxRgn:
			case kWindowDragRgn:
			case kWindowGrowRgn:
			case kWindowCollapseBoxRgn:
				SetEmptyRgn(rgnRec->winRgn);
				break;
			case kWindowStructureRgn:
			case kWindowContentRgn:
				getWindowContentRegion(window, rgnRec->winRgn);
				break;
			case kWindowUpdateRgn:
				break;
			default:
				return errWindowRegionCodeInvalid;
			}
			return noErr;
		}
	case kWindowMsgHitTest:
		{
			Point hitPoint;
			static RgnHandle tempRgn = nil;
			if (!tempRgn)
				tempRgn = NewRgn();
			SetPt(&hitPoint, LoWord(param), HiWord(param));//get the point clicked
			if (PtInRgn(hitPoint, getWindowContentRegion(window, tempRgn)))
				return wInContent;
			else
				return wNoHit;
		}
	default:
		break;
	}
	return 0;
}

static pascal long WindowMaskProc(short varCode, WindowRef window, short message, long param)
{
#pragma unused( varCode ) 
	switch (message)
	{
	case kWindowMsgGetFeatures:
		*(OptionBits*) param = kWindowCanGetWindowRegion
		                       | kWindowDefSupportsColorGrafPort;
		return 1;
	case kWindowMsgGetRegion:
		{
			GetWindowRegionRec* rgnRec  = (GetWindowRegionRec*) param;
			switch (rgnRec->regionCode)
			{
			case kWindowTitleBarRgn:
			case kWindowTitleTextRgn:
			case kWindowCloseBoxRgn:
			case kWindowZoomBoxRgn:
			case kWindowDragRgn:
			case kWindowGrowRgn:
			case kWindowCollapseBoxRgn:
				SetEmptyRgn(rgnRec->winRgn);
				break;
			case kWindowStructureRgn:
			case kWindowContentRgn:
				getWindowContentRegion(window, rgnRec->winRgn);
				break;
			case kWindowUpdateRgn:
				break;
			case kWindowOpaqueRgn:
				SetEmptyRgn(rgnRec -> winRgn);
				break;
			default:
				return errWindowRegionCodeInvalid;
			}
			return noErr;
		}
	case kWindowMsgHitTest:
		{
			Point hitPoint;
			static RgnHandle tempRgn = nil;
			if (!tempRgn)
				tempRgn = NewRgn();
			SetPt(&hitPoint, LoWord(param), HiWord(param));//get the point clicked
			if (PtInRgn(hitPoint, getWindowContentRegion(window, tempRgn)))
				return wInContent;
			else
				return wNoHit;
		}
		break;
		
	default:
		break;
	}
	return 0;
}

void UnloadBundle(CFBundleRef theBundle)
{
	/*get bundle resource and convert from xml to propertylist struct so we can look for revdontunload attribute and not unload- necessary for external
	that interface with Cocoa*/
	Boolean dontunload = False;
	CFPropertyListRef tCFPropertyListRef = NULL;
	short resrefnum = CFBundleOpenBundleResourceMap (theBundle );
	CFURLRef resFileCFURLRef = CFBundleCopyResourceURL(theBundle,CFSTR("revinfo"),CFSTR("plist"),NULL);
	CFDataRef resCFDataRef;
	if (resFileCFURLRef && CFURLCreateDataAndPropertiesFromResource(kCFAllocatorDefault,resFileCFURLRef,&resCFDataRef,nil,nil,nil))
	{
		if (resCFDataRef)
		{
			CFStringRef errorString;
			tCFPropertyListRef = CFPropertyListCreateFromXMLData(kCFAllocatorDefault,resCFDataRef,kCFPropertyListImmutable,&errorString);
			CFRelease(resCFDataRef);
		}
	}
	if (tCFPropertyListRef)
	{
		if (CFDictionaryGetValueIfPresent((CFDictionaryRef)tCFPropertyListRef,CFSTR("revdontunload"),NULL))
			dontunload = True;
	}
	if (resFileCFURLRef)
		CFRelease(resFileCFURLRef);
	CFBundleCloseBundleResourceMap (theBundle,resrefnum);
	if (theBundle && !dontunload)
	{
		CFBundleUnloadExecutable(theBundle);
		CFRelease(theBundle);
	}
}

CFBundleRef LoadBundle(const char *tpath)
{
	OSErr theErr;
	CFBundleRef theBundle = NULL;
	FSSpec fspec;
	if ((theErr = MCS_path2FSSpec(tpath, &fspec)) != noErr)
		return NULL;
	FSRef theRef;
	CFURLRef theBundleURL;
	theErr = FSpMakeFSRef(&fspec, &theRef);
	theBundleURL = CFURLCreateFromFSRef(kCFAllocatorSystemDefault, &theRef);
	if (theBundleURL != NULL)
	{
		/* Turn the CFURL into a bundle reference */
		theBundle = CFBundleCreate(kCFAllocatorSystemDefault, theBundleURL);
		CFRelease(theBundleURL);
	}
	if (theErr != noErr || theBundle == NULL)
		return NULL;
	Boolean isLoaded = CFBundleLoadExecutable(theBundle);
	if (!isLoaded)
	{
		CFRelease(theBundle);
		return NULL;
	}
	return theBundle;
}

MCStack *MCStack::findstackd(Window w)
{
	if (w == NULL)
		return NULL;
	if ((window != DNULL) && (w->handle.window == window->handle.window))
		return this;
	if (substacks != NULL)
	{
		MCStack *tptr = substacks;
		do
		{
			if ((tptr->window != DNULL) &&
			        (w->handle.window == tptr->window->handle.window))
				return tptr;
			tptr = (MCStack *)tptr->next();
		}
		while (tptr != substacks);
	}
	return NULL;
}

MCStack *MCStack::findchildstackd(Window w,uint2 &ccount,uint2 cindex)
{
	Window pwindow = getparentwindow();
	if (pwindow != DNULL && w->handle.window == pwindow->handle.window)
		if  (++ccount == cindex)
			return this;
	if (substacks != NULL)
	{
		MCStack *tptr = substacks;
		do
		{
			pwindow = tptr->getparentwindow();
			if (pwindow != DNULL && w->handle.window == pwindow->handle.window)
			{
				ccount++;
				if (ccount == cindex)
					return tptr;
			}
			tptr = (MCStack *)tptr->next();
		}
		while (tptr != substacks);
	}
	return NULL;
}

void MCStack::realize()
{ //create window
	if (!MCnoui && MCModeMakeLocalWindows())
	{
		if ( getextendedstate(ECS_FULLSCREEN) )
		{
		//TS-2008-08-01 : [[Bug 5703 - fullscreen stack prop interacts badly with HideMenuBar]]
			if (!((MCScreenDC*)MCscreen)->getmenubarhidden())
				SetSystemUIMode(kUIModeAllHidden, kUIOptionAutoShowMenuBar);

			const MCDisplay *t_display;
			t_display = MCscreen -> getnearestdisplay(rect);
			MCRectangle t_workarea, t_viewport;
			t_workarea = t_display -> workarea;
			t_viewport = t_display -> viewport ;
			setrect(t_viewport);
		}
		else
		{
			if (!((MCScreenDC*)MCscreen)->getmenubarhidden())
				SetSystemUIMode(kUIModeNormal, NULL);
		}

		Rect wrect;
		MCScreenDC *psdc = (MCScreenDC *)MCscreen;
		psdc->MCRect2MacRect(rect, wrect);
		window = new _Drawable;
		window->type = DC_WINDOW;
		window->handle.window = 0;
		char *tmpname = NULL;
		const unsigned char *namePascal;
		if (!isunnamed())
		{ //set window title to name of stack temporarily.
			tmpname = strclone(getname_cstring()); //it will be changed by setname() later.
			namePascal = c2pstr(tmpname);
		}
		else
			namePascal = (unsigned char*)"\p";

		loadwindowshape();


		window->handle.window = NULL;
		uint32_t wclass;
		uint32_t wattributes;
		getWinstyle(wattributes,wclass);
		
		wattributes |= kWindowCompositingAttribute;

		long testdecorations = WD_TITLE | WD_MENU | WD_CLOSE | WD_MINIMIZE | WD_MAXIMIZE;

		if (m_window_shape != NULL)
		{
			static WindowDefUPP s_window_mask_proc = NULL;
			if (s_window_mask_proc == NULL)
				s_window_mask_proc = NewWindowDefUPP(WindowMaskProc);
			
			WindowDefSpec t_spec;
			t_spec . defType = kWindowDefProcPtr;
			t_spec . u . defProc = s_window_mask_proc;
			
			CreateCustomWindow(&t_spec, wclass, wattributes, &wrect, (WindowPtr *)&window -> handle . window);
			HIWindowChangeFeatures((WindowPtr)window -> handle . window, 0, kWindowIsOpaque);
		}
		else if (((flags & F_DECORATIONS && !(decorations & testdecorations))
		          || wclass == kPlainWindowClass))
		{
			static WindowDefUPP s_borderless_proc = NULL;
			if (s_borderless_proc == NULL)
				s_borderless_proc = NewWindowDefUPP(BorderlessWindowProc);
				
			WindowDefSpec t_spec;
			t_spec . defType = kWindowDefProcPtr;
			t_spec . u . defProc = s_borderless_proc;

			if (wclass == kPlainWindowClass)
				wclass = kUtilityWindowClass;

			CreateCustomWindow(&t_spec, wclass, wattributes, &wrect, (WindowPtr *)&window->handle.window);
		}
		else
			CreateNewWindow(wclass, wattributes,&wrect, (WindowPtr *)&window->handle.window);

		if (wclass == kFloatingWindowClass)
			ChangeWindowAttributes((WindowPtr)window -> handle . window, kWindowNoAttributes, kWindowHideOnSuspendAttribute);
		
		// MW-2009-10-31: Make sure we can collapse any rev window
		HIWindowChangeFeatures((WindowPtr)window -> handle . window, kWindowCanCollapse, 0);
		
		if (window->handle.window == NULL)
			SetWTitle((WindowPtr)window->handle.window, namePascal);
		SetWTitle((WindowPtr)window->handle.window, namePascal);
		setopacity(blendlevel * 255 / 100);
		SetWRefCon((WindowPtr)window->handle.window, mode);
		
		ControlRef t_control;
		MCRevolutionStackViewCreate(this, &t_control);
		ControlRef t_root_control;
		GetRootControl((WindowPtr)window -> handle . window, &t_root_control);
		HIViewAddSubview(t_root_control, t_control);
		ShowControl(t_control);
		
		if (wclass == kDrawerWindowClass)
		{
			Window pwindow = NULL;
			if (parentwindow != DNULL)
				pwindow = parentwindow;
			else
				if (MCdefaultstackptr && MCdefaultstackptr->getw() != DNULL )
					pwindow = MCdefaultstackptr->getw();
			if (pwindow && GetWRefCon((WindowPtr)pwindow->handle.window) != WM_DRAWER)
			{
				SetDrawerParent((WindowPtr)window->handle.window, (WindowPtr)pwindow->handle.window);
				WindowAttributes watt;
				GetWindowAttributes((WindowPtr)pwindow->handle.window,&watt);
				if (wattributes & kWindowResizableAttribute)
					ChangeWindowAttributes((WindowPtr)pwindow->handle.window, kWindowLiveResizeAttribute, 0);
				OptionBits draweredge;
				switch (wposition)
				{
				case WP_PARENTTOP:
					draweredge = kWindowEdgeTop;
					break;
				case WP_PARENTRIGHT:
					draweredge = kWindowEdgeRight;
					break;
				case WP_PARENTBOTTOM:
					draweredge = kWindowEdgeBottom;
					break;
				case WP_PARENTLEFT:
					draweredge = kWindowEdgeLeft;
					break;
				default:
					draweredge =  kWindowEdgeDefault;
					break;
				}
				SetDrawerPreferredEdge((WindowPtr)window->handle.window, draweredge);
				if (walignment)
				{
					MCRectangle parentwindowrect;
					MCscreen->getwindowgeometry(pwindow, parentwindowrect);
					int2 wspace = 0;
					RgnHandle r = NewRgn();
					GetWindowRegion((WindowPtr)window->handle.window, kWindowStructureRgn, r);
					Rect tRect;
					GetRegionBounds(r, &tRect);
					DisposeRgn(r);
					MCRectangle drawerwindowrect;
					psdc->MacRect2MCRect(tRect, drawerwindowrect);
					if (wposition == WP_PARENTTOP || wposition == WP_PARENTBOTTOM)
					{
						wspace = parentwindowrect.width - drawerwindowrect.width;
						if (watt & kWindowMetalAttribute)
							if (wspace)
								wspace += 10; //for metal
					}
					else
					{
						wspace = parentwindowrect.height - drawerwindowrect.height;
						if (watt & kWindowMetalAttribute)
							if (wspace)
								wspace += 5; //for metal
					}
					if (wspace > 0)
						switch (walignment)
						{
						case OP_CENTER:
							SetDrawerOffsets ((WindowPtr)window->handle.window,ceil(wspace/2) -1,floor(wspace/2) + 1);
							break;
						case OP_RIGHT:
						case OP_BOTTOM:
							SetDrawerOffsets ((WindowPtr)window->handle.window,wspace,0);
							break;
						case OP_TOP:
						case OP_LEFT:
							SetDrawerOffsets ((WindowPtr)window->handle.window,0,wspace);
							break;
						}
				}
			}
		}
		delete tmpname;


		// MW-2005-11-06: We also need to catch window constraining events so we can flush
		//   the screen geometry cache.
		EventTypeSpec list[] =
		{
		
			{kEventClassWindow, kEventWindowCollapsed},
			{kEventClassWindow, kEventWindowExpanded},
			{kEventClassMouse, kEventMouseWheelMoved},
			{kEventClassWindow, kEventWindowBoundsChanging},
			{kEventClassWindow, kEventWindowBoundsChanged},
			{kEventClassWindow, kEventWindowConstrain},
			{kEventClassWindow, kEventWindowFocusAcquired},
			{kEventClassWindow, kEventWindowFocusRelinquish},
			{kEventClassWindow, kEventWindowActivated},
			{kEventClassWindow, kEventWindowDeactivated},
			{kEventClassWindow, kEventWindowClose},
		};

		EventHandlerRef ref;
		
		// MW-2005-09-07: Pass the window handle as user data, otherwise 'takewindow' causes problems
		InstallWindowEventHandler((WindowPtr)window->handle.window, MCS_weh, sizeof(list) / sizeof(EventTypeSpec), list, (WindowPtr)window -> handle . window, &ref);
		
		ChangeWindowAttributes((WindowPtr)window->handle.window, 0, kWindowHideOnFullScreenAttribute);
		
		updatemodifiedmark();
	}
	start_externals();
}

void MCStack::sethints()
{
	if (!opened || MCnoui || window == DNULL) //not opened or no user interface
		return;
	if (flags & F_RESIZABLE)
	{
		rect.width = MCU_max(minwidth, rect.width);
		rect.width = MCU_min(maxwidth, rect.width);
		rect.width = MCU_min(MCscreen->getwidth(), rect.width);
		rect.height = MCU_max(minheight, rect.height);
		rect.height = MCU_min(maxheight, rect.height);
		rect.height = MCU_min(MCscreen->getheight(), rect.height);
	}
}

MCRectangle MCStack::getwindowrect() const
{
    if (window == DNULL)
        return rect;
    
    MCRectangle t_rect;
    Rect t_winrect;
    RgnHandle t_rgn;
    t_rgn = NewRgn();
    GetWindowRegion((WindowRef)window->handle.window, kWindowStructureRgn, t_rgn);
    GetRegionBounds(t_rgn, &t_winrect);
    DisposeRgn(t_rgn);
    
    t_rect.x = t_winrect.left;
    t_rect.y = t_winrect.top;
    t_rect.width = t_winrect.right - t_winrect.left;
    t_rect.height = t_winrect.bottom - t_winrect.top;
    
    return t_rect;
}

void MCStack::setgeom()
{
	//set stack(window) size or position from script
	if (MCnoui || !opened)
		return;
	
	// MW-2009-09-25: Ensure things are the right size when doing
	//   remote dialog/menu windows.
	if (window == DNULL)
	{
		// MW-2011-08-18: [[ Redraw ]] Update to use redraw.
		MCRedrawLockScreen();
		state &= ~CS_NEED_RESIZE;
		resize(rect . width, rect . height);
		MCRedrawUnlockScreen();
		mode_setgeom();
		return;
	}
	
	// MW-2011-09-12: [[ MacScroll ]] Make sure we apply the current scroll setting.
	applyscroll();
	
	Rect windRect;
	GetPortBounds(GetWindowPort((WindowPtr)window->handle.window), &windRect);
	SetGWorld(GetWindowPort((WindowPtr)window->handle.window), GetMainDevice());
	Point p;
	p.h = windRect.left;
	p.v = windRect.top;
	LocalToGlobal(&p);
	int2 curWidth = windRect.right - windRect.left;
	int2 curHeight = windRect.bottom - windRect.top;
	if (IsWindowVisible((WindowPtr)window->handle.window))
	{
		if (mode != WM_SHEET && mode != WM_DRAWER
		        && (rect.x != p.h || rect.y != p.v))
		{
			MoveWindow((WindowPtr)window->handle.window, rect.x, rect.y, False);
			state |= CS_BEEN_MOVED;
		}
		if (rect.width != curWidth || rect.height != curHeight)
		{
			SizeWindow((WindowPtr)window->handle.window, rect.width, rect.height, True);
			resize(curWidth, curHeight + getscroll());
		}
		state &= ~CS_NEED_RESIZE;
	}
	else
	{
		if (mode != WM_SHEET && mode != WM_DRAWER)
			MoveWindow((WindowPtr)window->handle.window, rect.x, rect.y, False);
		if (rect.width != curWidth || rect.height != curHeight)
		{
			SizeWindow((WindowPtr)window->handle.window, rect.width, rect.height, True);
			resize(curWidth, curHeight + getscroll());

		}
		state &= ~(CS_BEEN_MOVED | CS_NEED_RESIZE);
	}
}

// MW-2011-09-12: [[ MacScroll ]] This is called to apply the Mac menu scroll. It
//   causes the stack to be shrunk and the content to be shifted up.
void MCStack::applyscroll(void)
{
	int32_t t_new_scroll;
	t_new_scroll = getnextscroll();
	
	// If the scroll isn't changing, do nothing.
	if (t_new_scroll == m_scroll)
		return;
	
	// Otherwise, set the scroll back to the unmolested version.
	rect . height += m_scroll;
	
	// Update the scroll value.
	m_scroll = t_new_scroll;
	
	// Update the rect...
	rect . height -= t_new_scroll;
	
	// Make sure window contents reflects the new scroll.
	syncscroll();
}

// MW-2011-09-12: [[ MacScroll ]] This is called to clear any currently applied
//   Mac menu scroll.
void MCStack::clearscroll(void)
{
	if (m_scroll == 0)
		return;
	
	// Set the rect back.
	rect . height += m_scroll;

	// Reset the scroll.
	m_scroll = 0;
	
	// Make sure window contents reflects the new scroll.
	syncscroll();
}

void MCStack::syncscroll(void)
{
	// If we have no window, no need to adjust the HIView.
	if (window == nil)
		return;
	
	// And tweak the HIView's location...
	ControlRef t_root_control;
	GetRootControl((WindowPtr)window -> handle . window, &t_root_control);
	ControlRef t_subcontrol;
	if (GetIndexedSubControl(t_root_control, 1, &t_subcontrol) == noErr)
	{
		Rect t_bounds;
		t_bounds . left = 0;
		t_bounds . top = -m_scroll;
		t_bounds . right = rect . width;
		t_bounds . bottom = rect . height;
		SetControlBounds(t_subcontrol, &t_bounds);
	}
	
	// MW-2011-11-30: [[ Bug 9887 ]] Make sure all the player objects on this
	//   stack are adjusted as appropriate.
	for(MCPlayer *t_player = MCplayers; t_player != nil; t_player = t_player -> getnextplayer())
		if (!t_player -> isbuffering() && t_player -> getstack() == this)
			t_player -> setrect(t_player -> getrect());
			
	// MW-2012-10-08: [[ Bug 10442 ]] Set the scroll window property so revBrowser
	//   works properly.
	SetWindowProperty((WindowPtr)window -> handle . window, 'revo', 'scrl', 4, &m_scroll);
	
	// Now send a sync event so revBrowser picks it up.
	EventRef t_event;
	CreateEvent(NULL, 'revo', 'sync', GetCurrentEventTime(), 0, &t_event);
	SendEventToEventTarget(t_event, GetWindowEventTarget((WindowPtr)window -> handle . window));
	ReleaseEvent(t_event);
}

// MW-2011-09-13: [[ Masks ]] The windowshape is now stored in a 'WindowMask'
//   struct. We don't distinguish between 1-bit and 8-bit masks on Mac.
void MCStack::destroywindowshape()
{
	if (m_window_shape != nil)
	{
		delete[] m_window_shape -> data;
		if (m_window_shape -> handle != nil)
			CGImageRelease((CGImageRef)m_window_shape -> handle);
		delete m_window_shape;
		m_window_shape = nil;
	}
}

void MCStack::getminmax(Rect *r)
{ //get the min & max size of a stack
	r->left = minwidth;
	r->top =  minheight;
	r->right = maxwidth > MAXINT2 ? MAXINT2 : maxwidth;
	r->bottom = maxheight > MAXINT2 ? MAXINT2 : maxheight;
}


void  MCStack::getWinstyle(uint32_t &wstyle, uint32_t &wclass)
{
	wclass = kDocumentWindowClass;
	switch (mode)
	{
	case WM_TOP_LEVEL:
	case WM_TOP_LEVEL_LOCKED:
	case WM_MODELESS:
	case WM_CLOSED:
		wclass = kDocumentWindowClass;
		break;
	case WM_PALETTE:
		wclass = kFloatingWindowClass;
		break;
	case WM_MODAL:
		if (!(flags & F_DECORATIONS) || decorations & WD_TITLE)
			wclass = kMovableModalWindowClass;
		else
			wclass = kModalWindowClass;
		break;
	case WM_DRAWER:
		wclass =  kDrawerWindowClass;
		break;
	case WM_SHEET:
		wclass = kSheetWindowClass;
		break;
	default:
		wclass =  kPlainWindowClass;
		break;
	}
	wstyle = kWindowNoAttributes;
	if (decorations & WD_UTILITY)
		wclass = kUtilityWindowClass;
	if (flags & F_DECORATIONS && !(decorations & WD_SHAPE))
	{
		if (decorations & WD_CLOSE &&
		        (wclass == kDocumentWindowClass || wclass ==  kFloatingWindowClass
		         || wclass == kUtilityWindowClass) )
			wstyle |= kWindowCloseBoxAttribute;
		if (decorations & WD_METAL  &&
		        (wclass == kDocumentWindowClass || wclass ==  kFloatingWindowClass))
		{
			wstyle |= kWindowMetalAttribute;
			if (MCmajorosversion >= 0x1040)
				wstyle |= kWindowMetalNoContentSeparatorAttribute;
		}
		if (decorations & WD_NOSHADOW)
			wstyle |= kWindowNoShadowAttribute;
		if (decorations & WD_MINIMIZE && (wclass == kDocumentWindowClass || wclass ==  kFloatingWindowClass || wclass == kUtilityWindowClass))
			if (wclass == kDocumentWindowClass)
				wstyle |= kWindowCollapseBoxAttribute;
		if (decorations & WD_MAXIMIZE &&
		        (wclass == kDocumentWindowClass || wclass ==  kFloatingWindowClass
		         || wclass == kUtilityWindowClass))
			wstyle |= kWindowFullZoomAttribute;
		if (decorations & WD_LIVERESIZING)
			wstyle |= kWindowLiveResizeAttribute;
	}
	else
	{
		if ( wclass ==  kFloatingWindowClass || wclass == kUtilityWindowClass )
			wstyle = kWindowStandardFloatingAttributes;
		else if ( wclass == kDocumentWindowClass)
			wstyle = kWindowStandardDocumentAttributes;
	}
	wstyle &= ~kWindowResizableAttribute;
	if (flags & F_RESIZABLE &&
	        (wclass == kDocumentWindowClass || wclass ==  kFloatingWindowClass
	         || wclass == kUtilityWindowClass || wclass == kSheetWindowClass ||
	         wclass == kMovableModalWindowClass || wclass == kDrawerWindowClass))
		wstyle |= kWindowResizableAttribute;
		
// MW-2005-04-20: Fix shadow alteration on window-shaped windows
  if (decorations & WD_NOSHADOW)
		wstyle |= kWindowNoShadowAttribute;
}

void MCStack::drawgrowicon(const MCRectangle &dirty)
{
}

void MCRevolutionStackViewRelink(WindowPtr p_window, MCStack *p_stack);

void MCStack::start_externals()
{
	if (window != NULL)
		MCRevolutionStackViewRelink((WindowPtr)window -> handle . window, this);
	loadexternals();
}

void MCStack::stop_externals()
{
	Boolean oldlock = MClockmessages;
	MClockmessages = True;

	MCPlayer *tptr = MCplayers;

	while (tptr != NULL)
	{
		if (tptr->getstack() == this)
		{
			if (tptr->playstop())
				tptr = MCplayers; // was removed, start search over
		}
		else
			tptr = tptr->getnextplayer();
	}
	destroywindowshape();

	MClockmessages = oldlock;
	
	unloadexternals();
	
	if (window != NULL)
		MCRevolutionStackViewRelink((WindowPtr)window -> handle . window, NULL);
}

void MCStack::openwindow(Boolean p_override)
{
	if (MCModeMakeLocalWindows() && window != DNULL)
		MCscreen -> openwindow(window, p_override);
}

void MCStack::setopacity(uint1 p_level)
{
	if (!MCModeMakeLocalWindows())
		return;
	
	if (window != NULL)
		SetWindowAlpha((WindowPtr)window -> handle . window, p_level / 255.0);
}

void MCStack::updatemodifiedmark(void)
{
	if (!MCModeMakeLocalWindows())
		return;
	
	if (window != NULL)
		SetWindowModified((WindowPtr)window -> handle . window, getextendedstate(ECS_MODIFIED_MARK));
}

void MCStack::redrawicon()
{
	// MW-2005-07-18: It is possible for this to be called if window == NULL in which
	//   case bad things can happen - so don't let this occur.
	if (iconid != 0 && window != NULL)
	{
		MCImage *iptr = (MCImage *)getobjid(CT_IMAGE, iconid);
		if (iptr != NULL)
		{
			CGImageRef tdockimage;
			CGrafPtr tport;
			CGrafPtr curport;
			CGContextRef context;
			OSStatus theErr;
			CGRect cgrect = CGRectMake(0,0,128,128);
			GetPort( &curport);
			OSErr err = CreateQDContextForCollapsedWindowDockTile((WindowPtr)window->handle.window, &tport);
			if (err == noErr)
			{
				SetPort(tport);
				CreateCGContextForPort(tport, &context);
				tdockimage = iptr -> makeicon(128, 128);
				CGContextDrawImage(context,cgrect,tdockimage);
				if ( tdockimage )
					CGImageRelease( tdockimage );
				CGContextFlush(context);
				CGContextRelease(context);
				SetPort(curport);
				ReleaseQDContextForCollapsedWindowDockTile((WindowPtr)window->handle.window, tport);
			}
		}
	}
}

void MCStack::enablewindow(bool p_enable)
{
}

// MW-2011-09-11: [[ Redraw ]] Force an immediate update of the window within the given
//   region. The actual rendering is done by deferring to the 'redrawwindow' method.
void MCStack::updatewindow(MCRegionRef p_region)
{
	HIViewRef t_root;
	GetRootControl((WindowPtr)window -> handle . window, &t_root);
	
	HIViewRef t_view;
	GetIndexedSubControl(t_root, 1, &t_view);
	
	// MW-2011-10-07: [[ Bug 9792 ]] If the mask hasn't changed, use the update region,
	//   else redraw the whole view.
	if (!getextendedstate(ECS_MASK_CHANGED))
		HIViewSetNeedsDisplayInRegion(t_view, (RgnHandle)p_region, TRUE);
	else
	{
		HIViewSetNeedsDisplay(t_view, TRUE);
		DisableScreenUpdates();
	}
	
	HIViewRender(t_view);
	
	// MW-2012-09-04: [[ Bug 10333 ]] Flush the window immediately to make sure
	//   we see every update.
	// MW-2012-09-10: [[ Revert Bug 10333 ]] Delayed until IDE issues can be resolved.
	// HIWindowFlush((WindowPtr)window -> handle . window);
	
	// Update the shadow, if required.
	if (getextendedstate(ECS_MASK_CHANGED))
	{
		// MW-2012-09-10: [[ Revert Bug 10333 ]] Delayed until IDE issues can be resolved.
		HIWindowFlush((WindowPtr)window -> handle . window);
	
		HIWindowInvalidateShadow((WindowPtr)window -> handle . window);
		
		EnableScreenUpdates();
		
		setextendedstate(False, ECS_MASK_CHANGED);
	}
}

// MW-2011-09-11: [[ Redraw ]] Force an immediate update of the window within the given
//   region but using the pixmap given.
void MCStack::updatewindowwithbuffer(Pixmap p_pixmap, MCRegionRef p_region)
{
	HIViewRef t_root;
	GetRootControl((WindowPtr)window -> handle . window, &t_root);
	
	HIViewRef t_view;
	GetIndexedSubControl(t_root, 1, &t_view);
	HIViewSetNeedsDisplayInRegion(t_view, (RgnHandle)p_region, TRUE);
		
	// Set the file-local static to the pixmap to use (stacksurface picks this up!)
	s_update_pixmap = p_pixmap;
	HIViewRender(t_view);
	// Unset the file-local static.
	s_update_pixmap = nil;
	
	// MW-2011-10-18: [[ Bug 9798 ]] Make sure we force a screen update after every
	//   update.
	// MW-2012-09-10: [[ Revert Bug 10333 ]] Delayed until IDE issues can be resolved.
	// HIWindowFlush((WindowPtr)window -> handle . window);
	
	// Update the shadow, if required.
	if (getextendedstate(ECS_MASK_CHANGED))
	{
		// MW-2012-09-10: [[ Revert Bug 10333 ]] Delayed until IDE issues can be resolved.
		HIWindowFlush((WindowPtr)window -> handle . window);
		
		HIWindowInvalidateShadow((WindowPtr)window -> handle . window);
		
		EnableScreenUpdates();
		
		setextendedstate(False, ECS_MASK_CHANGED);
	}
}

////////////////////////////////////////////////////////////////////////////////

static void MCMacRenderBitsToCG(CGContextRef p_target, CGRect p_area, const void *p_bits, uint32_t p_stride, bool p_has_alpha)
{
	CGColorSpaceRef t_colorspace;
	t_colorspace = CGColorSpaceCreateDeviceRGB();
	if (t_colorspace != nil)
	{
		CGBitmapInfo t_bitmap_info;
		t_bitmap_info = kCGBitmapByteOrder32Host;
		t_bitmap_info |= p_has_alpha ? kCGImageAlphaPremultipliedFirst : kCGImageAlphaNoneSkipFirst;
		
		CGContextRef t_cgcontext;
		t_cgcontext = CGBitmapContextCreate((void *)p_bits, p_area . size . width, p_area . size . height, 8, p_stride, t_colorspace, t_bitmap_info);
		if (t_cgcontext != nil)
		{
			CGImageRef t_image;
			t_image = CGBitmapContextCreateImage(t_cgcontext);
			CGContextRelease(t_cgcontext);
			
			if (t_image != nil)
			{
				CGContextClipToRect((CGContextRef)p_target, p_area);
				CGContextDrawImage((CGContextRef)p_target, p_area, t_image);
				CGImageRelease(t_image);
			}
		}
		
		CGColorSpaceRelease(t_colorspace);
	}
}

class MCMacStackSurface: public MCStackSurface
{
	MCStack *m_stack;
	MCRegionRef m_region;
	CGContextRef m_context;
	
	MCRectangle m_locked_area;
	MCContext *m_locked_context;
	Pixmap m_locked_pixmap;
	void *m_locked_bits;
	
public:
	MCMacStackSurface(MCStack *p_stack, MCRegionRef p_region, CGContextRef p_context)
	{
		m_stack = p_stack;
		m_region = p_region;
		m_context = p_context;
		
		m_locked_context = nil;
		m_locked_pixmap = nil;
		m_locked_bits = nil;
	}
	
	bool LockGraphics(MCRegionRef p_area, MCContext*& r_context)
	{
		MCRectangle t_actual_area;
		t_actual_area = MCU_intersect_rect(MCRegionGetBoundingBox(p_area), MCRegionGetBoundingBox(m_region));
		
		if (MCU_empty_rect(t_actual_area))
			return false;
		
		m_locked_pixmap = MCscreen -> createpixmap(t_actual_area . width, t_actual_area . height, 0, False);
		if (m_locked_pixmap != nil)
		{
			m_locked_context = MCscreen -> createcontext(m_locked_pixmap, False, False);
			if (m_locked_context != nil)
			{
				m_locked_context -> setorigin(t_actual_area . x, t_actual_area . y);
				m_locked_context -> setclip(t_actual_area);
				
				m_locked_area = t_actual_area;
				r_context = m_locked_context;
				
				return true;
			}
			
			MCscreen -> freepixmap(m_locked_pixmap);
		}
		
		return false;
	}
	
	void UnlockGraphics(void)
	{
		if (m_locked_context == nil)
			return;
		
		MCscreen -> freecontext(m_locked_context);
		m_locked_context = nil;
		
		void *t_bits;
		uint32_t t_stride;
		MCscreen -> lockpixmap(m_locked_pixmap, t_bits, t_stride);
		FlushBits(t_bits, t_stride);
		MCscreen -> unlockpixmap(m_locked_pixmap, t_bits, t_stride);
		
		MCscreen -> freepixmap(m_locked_pixmap);
	}
	
	bool LockPixels(MCRegionRef p_area, void*& r_bits, uint32_t& r_stride)
	{
		MCRectangle t_actual_area;
		t_actual_area = MCU_intersect_rect(MCRegionGetBoundingBox(p_area), MCRegionGetBoundingBox(m_region));
		
		if (MCU_empty_rect(t_actual_area))
			return false;
		
		m_locked_bits = malloc(t_actual_area . width * t_actual_area . height * sizeof(uint32_t));
		if (m_locked_bits != nil)
		{
			m_locked_area = t_actual_area;
			
			r_bits = m_locked_bits;
			r_stride = t_actual_area . width * sizeof(uint32_t);
			return true;
		}
		
		return false;
	}
	
	void UnlockPixels(void)
	{
		if (m_locked_bits == nil)
			return;
		
		FlushBits(m_locked_bits, m_locked_area . width * sizeof(uint32_t));
		
		free(m_locked_bits);
		m_locked_bits = nil;
	}
	
	void FlushBits(void *p_bits, uint32_t p_stride)
	{
		void *t_target;
		if (!LockTarget(kMCStackSurfaceTargetCoreGraphics, t_target))
			return;
		
		int32_t t_height;
		t_height = m_stack -> getcurcard() -> getrect() . height;
		
		CGRect t_dst_rect;
		t_dst_rect = CGRectMake(m_locked_area . x, t_height - (m_locked_area . y + m_locked_area . height), m_locked_area . width, m_locked_area . height);
		
		MCMacRenderBitsToCG(m_context, t_dst_rect, p_bits, p_stride, false);
		
		UnlockTarget();
	}
	
	bool LockTarget(MCStackSurfaceTargetType p_type, void*& r_context)
	{
		if (p_type != kMCStackSurfaceTargetCoreGraphics)
			return false;

		CGImageRef t_mask;
		t_mask = nil;
		if (m_stack -> getwindowshape() != nil)
			t_mask = (CGImageRef)m_stack -> getwindowshape() -> handle;
		
		if (t_mask != nil)
		{
			MCRectangle t_card_rect;
			t_card_rect = m_stack -> getcurcard() -> getrect();
			
			MCRectangle t_rect;
			t_rect = MCRegionGetBoundingBox(m_region);
			CGContextClearRect(m_context, CGRectMake(t_rect . x, t_card_rect . height - (t_rect . y + t_rect . height), t_rect . width, t_rect . height));
			
			// MW-2012-07-25: [[ Bug ]] Make sure we use signed arithmetic to
			//   compute the y-origin otherwise it wraps to 2^32!
			int32_t t_mask_height, t_mask_width;
			t_mask_width = (int32_t)CGImageGetWidth(t_mask);
			t_mask_height = (int32_t)CGImageGetHeight(t_mask);
			
			CGRect t_dst_rect;
			t_dst_rect . origin . x = 0;
			t_dst_rect . origin . y = ((int32_t)t_card_rect . height) - t_mask_height - m_stack -> getscroll();
			t_dst_rect . size . width = t_mask_width;
			t_dst_rect . size . height = t_mask_height;
			CGContextClipToMask(m_context, t_dst_rect, t_mask);
		}
	
		CGContextSaveGState(m_context);

		r_context = m_context;
		
		return true;
	}
	
	void UnlockTarget(void)
	{
		CGContextRestoreGState(m_context);
	}
};

////////////////////////////////////////////////////////////////////////////////

#define kHIRevolutionStackViewClassID CFSTR("com.runrev.revolution.stackview")

OSStatus HIRevolutionStackViewHandler(EventHandlerCallRef p_call_ref, EventRef p_event, void *p_data);

struct HIRevolutionStackViewData
{
	HIObjectRef view;
	MCStack *stack;
};

void HIRevolutionStackViewRegister(void)
{
	static HIObjectClassRef s_class = NULL;
	
	OSStatus t_status;
	t_status = noErr;
	
	if (s_class == NULL)
	{
		EventTypeSpec t_events[] =
		{
			{ kEventClassHIObject, kEventHIObjectConstruct },
			{ kEventClassHIObject, kEventHIObjectInitialize },
			{ kEventClassHIObject, kEventHIObjectDestruct },
			{ kEventClassControl, kEventControlInitialize },
			{ kEventClassControl, kEventControlDraw },
			{ kEventClassControl, kEventControlHitTest },
			{ kEventClassControl, kEventControlGetPartRegion },
			{ kEventClassControl, kEventControlHiliteChanged },
			{ kEventClassControl, kEventControlActivate },
			{ kEventClassControl, kEventControlDeactivate },
			{ 'revo', 'rlnk' }
		};
		
		t_status = HIObjectRegisterSubclass(kHIRevolutionStackViewClassID, kHIViewClassID, NULL,
			HIRevolutionStackViewHandler, sizeof(t_events) / sizeof(EventTypeSpec), t_events, NULL, &s_class);
	}
}

OSStatus HIRevolutionStackViewHandler(EventHandlerCallRef p_call_ref, EventRef p_event, void *p_data)
{
	OSStatus t_status;
	t_status = eventNotHandledErr;
	
	UInt32 t_event_class;
	t_event_class = GetEventClass(p_event);
	
	UInt32 t_event_kind;
	t_event_kind = GetEventKind(p_event);
	
	HIRevolutionStackViewData *t_context;
	t_context = (HIRevolutionStackViewData *)p_data;
	
	switch(t_event_class)
	{
	case 'revo':
		switch(t_event_kind)
		{
			case 'rlnk':
				GetEventParameter(p_event, 'Stak', typeVoidPtr, NULL, sizeof(void *), NULL, &t_context -> stack);
			break;
		}
	break;
	
	case kEventClassHIObject:
		switch(t_event_kind)
		{
		case kEventHIObjectConstruct:
		{
			HIRevolutionStackViewData *t_data;
			t_data = new HIRevolutionStackViewData;
			t_data -> stack = NULL;
			GetEventParameter(p_event, kEventParamHIObjectInstance, typeHIObjectRef, NULL, sizeof(HIObjectRef), NULL, (HIObjectRef *)&t_data -> view);
			SetEventParameter(p_event, kEventParamHIObjectInstance, typeVoidPtr, sizeof(HIRevolutionStackViewData *), &t_data);
			t_status = noErr;
		}
		break;
		
		case kEventHIObjectInitialize:
		{
			GetEventParameter(p_event, 'Stak', typeVoidPtr, NULL, sizeof(void *), NULL, &t_context -> stack);
			
			Rect t_bounds;
			t_bounds . left = 0;
			// MW-2011-09-12: [[ MacScroll ]] Make sure the top of the HIView takes into
			//   account the scroll.
			t_bounds . top = -t_context -> stack -> getscroll();
			t_bounds . right = t_context -> stack -> getrect() . width;
			t_bounds . bottom = t_context -> stack -> getrect() . height;
			SetControlBounds((ControlRef)t_context -> view, &t_bounds);
			
			t_status = noErr;
		}
		break;
		
		case kEventHIObjectDestruct:
		{
			delete t_context;
			t_status = noErr;
		}
		break;
		}
	break;
	
	case kEventClassControl:
		switch(t_event_kind)
		{
		case kEventControlInitialize:
		{
			t_status = noErr;
		}
		break;
		
		case kEventControlDraw:
		{
			CGContextRef t_graphics;
			GetEventParameter(p_event, kEventParamCGContextRef, typeCGContextRef, NULL, sizeof(CGContextRef), NULL, &t_graphics);
			
			RgnHandle t_dirty_rgn;
			GetEventParameter(p_event, kEventParamRgnHandle, typeQDRgnHandle, NULL, sizeof(RgnHandle), NULL, &t_dirty_rgn);

			if (t_context -> stack != NULL)
			{
				// Compute the clip region for players.
				RgnHandle t_clip_rgn, t_rect_rgn;
				t_clip_rgn = NULL;
				t_rect_rgn = NULL;
				for(MCPlayer *t_player = MCplayers; t_player != NULL; t_player = t_player -> getnextplayer())
					if (t_player -> isvisible() && t_player -> getcard() == t_context -> stack -> getcurcard() && !t_player -> isbuffering())
					{
						MCRectangle t_rect;
						Rect t_mac_rect;
						
						t_rect = t_player -> getactiverect();
						
						if (t_clip_rgn == NULL)
						{
							t_clip_rgn = NewRgn();
							CopyRgn((RgnHandle)t_dirty_rgn, t_clip_rgn);
							t_rect_rgn = NewRgn();
						}
						
						SetRect(&t_mac_rect, t_rect . x, t_rect . y, t_rect . x + t_rect . width, t_rect . y + t_rect . height);
						RectRgn(t_rect_rgn, &t_mac_rect);
						DiffRgn(t_clip_rgn, t_rect_rgn, t_clip_rgn);
						
					}
				
				// We don't need the rect rgn anymore.
				if (t_rect_rgn != NULL)
					DisposeRgn(t_rect_rgn);

				// If the clip region is non-nil, then apply it.
				if (t_clip_rgn != NULL)
				{
					// As we can't clip to empty path, if the region is empty, we set the context
					// to nil.
					if (!EmptyRgn(t_clip_rgn))
					{
						HIShapeRef t_shape;
						t_shape = HIShapeCreateWithQDRgn(t_clip_rgn);
						HIShapeReplacePathInCGContext(t_shape, t_graphics);
						CGContextClip(t_graphics);
						CFRelease(t_shape);
					}
					else
						t_graphics = nil;
					DisposeRgn(t_clip_rgn);
				}

				// If the graphics context is non-nil (i.e. we aren't completely occluded) then
				// draw something.
				if (t_graphics != nil)
				{
					// HIView gives us a context in top-left origin mode which isn't so good
					// for our CG rendering so, revert back to bottom-left.
					CGContextScaleCTM(t_graphics, 1.0, -1.0);
					CGContextTranslateCTM(t_graphics, 0.0, -t_context -> stack -> getcurcard() -> getrect() . height);
					
					// Save the context state
					CGContextSaveGState(t_graphics);
					
					// If we don't have an update pixmap, then use redrawwindow.
					if (s_update_pixmap == nil)
					{
						MCMacStackSurface t_surface(t_context -> stack, (MCRegionRef)t_dirty_rgn, t_graphics);
						t_context -> stack -> redrawwindow(&t_surface, (MCRegionRef)t_dirty_rgn);
					}
					else
					{
						int32_t t_height;
						t_height = t_context -> stack -> getcurcard() -> getrect() . height;
						
						MCRectangle t_rect;
						t_rect = MCRegionGetBoundingBox((MCRegionRef)t_dirty_rgn);
						
						CGRect t_area;
						t_area = CGRectMake(t_rect . x, t_height - (t_rect . y + t_rect . height), t_rect . width, t_rect . height);
						
						CGContextClearRect(t_graphics, t_area);

						void *t_bits;
						uint32_t t_stride;
						MCscreen -> lockpixmap(s_update_pixmap, t_bits, t_stride);
						MCMacRenderBitsToCG(t_graphics, t_area, t_bits, t_stride, t_context -> stack -> getwindowshape() != nil ? true : false);
						MCscreen -> unlockpixmap(s_update_pixmap, t_bits, t_stride);
					}

					// Restore the context state
					CGContextRestoreGState(t_graphics);
				}
				
				// MW-2011-11-23: [[ Bug ]] Force a redraw of the players on the stack
				//   after we've drawn the rest of the content. This ensures players
				//   which are just appearing don't disappear behind said content.
				for(MCPlayer *t_player = MCplayers; t_player != NULL; t_player = t_player -> getnextplayer())
					if (t_player -> isvisible() && t_player -> getcard() == t_context -> stack -> getcurcard() && !t_player -> isbuffering())
						MCDoAction((MovieController)t_player -> getMovieController(), mcActionDraw, t_context -> stack -> getqtwindow());
			}
			
			t_status = noErr;
		}
		break;
		
		case kEventControlHitTest:
		break;
		
		case kEventControlGetPartRegion:
		{
			ControlPartCode t_part;
			GetEventParameter(p_event, kEventParamControlPart, typeControlPartCode, NULL, sizeof(ControlPartCode), NULL, &t_part);
			
			RgnHandle t_region;
			GetEventParameter(p_event, kEventParamControlRegion, typeQDRgnHandle, NULL, sizeof(RgnHandle), NULL, &t_region);
		}
		break;
		
		case kEventControlHiliteChanged:
		break;
		
		case kEventControlActivate:
		break;
		
		case kEventControlDeactivate:
		break;
		}
	break;
	}

	return t_status;
}

OSStatus MCRevolutionStackViewCreate(MCStack *p_stack, ControlRef* r_control)
{
	HIRevolutionStackViewRegister();
	
	EventRef t_event;
	CreateEvent(NULL, kEventClassHIObject, kEventHIObjectInitialize, GetCurrentEventTime(), 0, &t_event);
	SetEventParameter(t_event, 'Stak', typeVoidPtr, sizeof(void *), &p_stack);
	
	ControlRef t_control;
	HIObjectCreate(kHIRevolutionStackViewClassID, t_event, (HIObjectRef *)&t_control);
	
	ReleaseEvent(t_event);
	
	*r_control = t_control;
	
	return noErr;
}

void MCRevolutionStackViewRelink(WindowRef p_window, MCStack *p_new_stack)
{
	EventRef t_event;
	CreateEvent(NULL, 'revo', 'rlnk', GetCurrentEventTime(), 0, &t_event);
	SetEventParameter(t_event, 'Stak', typeVoidPtr, sizeof(void *), &p_new_stack);
	
	HIViewRef t_root;
	GetRootControl(p_window, &t_root);
	
	HIViewRef t_view;
	GetIndexedSubControl(t_root, 1, &t_view);
	SendEventToEventTarget(t_event, GetControlEventTarget(t_view));
	ReleaseEvent(t_event);
}

////////////////////////////////////////////////////////////////////////////////

