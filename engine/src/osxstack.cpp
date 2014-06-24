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
//#include "execpt.h"
#include "debug.h"
#include "globals.h"
#include "mode.h"
#include "image.h"
#include "redraw.h"
#include "license.h"
#include "context.h"
#include "region.h"

#include "graphicscontext.h"
#include "resolution.h"

////////////////////////////////////////////////////////////////////////////////

MCStack *MCStack::findstackd(Window w)
{
	if (w == NULL)
		return NULL;
	
	if (w == window)
		return this;
	if (substacks != NULL)
	{
		MCStack *tptr = substacks;
		do
		{
			if (w == tptr->window)
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
	if (pwindow != NULL && w == pwindow)
		if  (++ccount == cindex)
			return this;
	if (substacks != NULL)
	{
		MCStack *tptr = substacks;
		do
		{
			pwindow = tptr->getparentwindow();
			if (pwindow != NULL && w == pwindow)
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

<<<<<<< HEAD
void MCStack::realize()
{ //create window
	if (!MCnoui && MCModeMakeLocalWindows())
	{
		if (view_getfullscreen())
		{
			//TS-2008-08-01 : [[Bug 5703 - fullscreen stack prop interacts badly with HideMenuBar]]
			if (!((MCScreenDC*)MCscreen)->getmenubarhidden())
				SetSystemUIMode(kUIModeAllHidden, kUIOptionAutoShowMenuBar);

			// IM-2013-10-04: [[ FullscreenMode ]] Don't change stack rect if fullscreen
			/* CODE DELETED */
		}
		else
		{
			if (!((MCScreenDC*)MCscreen)->getmenubarhidden())
				SetSystemUIMode(kUIModeNormal, NULL);
		}

		// IM-2014-01-24: [[ HiDPI ]] Change to use logical coordinates - device coordinate conversion no longer needed
		/* CODE REMOVED */
		
		Rect wrect;
		// IM-2014-01-16: [[ StackScale ]] Use view rect as window size
		wrect = MCRectToMacRect(view_getrect());
		window = new _Drawable;
		window->type = DC_WINDOW;
		window->handle.window = 0;
		char *tmpname = NULL;
		const unsigned char *namePascal;
		if (!isunnamed())
		{ //set window title to name of stack temporarily.
			MCStringConvertToCString(MCNameGetString(getname()), tmpname); //it will be changed by setname() later.
			namePascal = c2pstr(tmpname);
		}
		else
			namePascal = (unsigned char*)"\p";

		loadwindowshape();


		window->handle.window = NULL;
		uint32_t wclass;
		uint32_t wattributes;
		getWinstyle(wattributes,wclass);
		
		// IM-2014-01-17: [[ HiDPI ]] Add frameworkscaled flag when creating window to
		// enable Hi-DPI rendering
		// IM-2014-01-27: [[ HiDPI ]] Only add frameworkscaled flag when pixel scaling is enabled
		wattributes |= kWindowCompositingAttribute;
		if (MCResGetUsePixelScaling())
			wattributes |= kWindowFrameworkScaledAttribute;
		
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
					drawerwindowrect = MCMacRectToMCRect(tRect);
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

=======
>>>>>>> develop
void MCStack::setsizehints()
{
}

void MCStack::sethints(void)
{
}

void MCStack::setgeom()
{
	//set stack(window) size or position from script
	if (MCnoui || !opened)
		return;
	
	// MW-2009-09-25: Ensure things are the right size when doing
	//   remote dialog/menu windows.
	if (window == NULL)
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
	
	// IM-2013-10-03: [[ FullscreenMode ]] Use view methods to get / set the stack viewport
	MCRectangle t_old_rect;
	t_old_rect = view_getstackviewport();
	
	// MW-2014-02-26: [[ Cocoa ]] It seems that to stop unwanted redraw artifacts we need to
	//   set the frame of the NSWindow and update the content at the same time. To achieve this
	//   neatly we will need to restructure how we go about resizes. However, for now, hopefully
	//   just disabling screen updates and enabling them between setting the frame and resizing
	//   should be enough...
	
	DisableScreenUpdates();
	
	rect = view_setstackviewport(rect);
	
	state &= ~CS_NEED_RESIZE;
	
	// IM-2013-10-03: [[ FullscreenMode ]] Return values from view methods are
	// in stack coords so don't need to transform
	if (t_old_rect.x != rect.x || t_old_rect.y != rect.y || t_old_rect.width != rect.width || t_old_rect.height != rect.height)
		resize(t_old_rect.width, t_old_rect.height);
	
	EnableScreenUpdates();
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
	
	// IM-2013-10-11: [[ FullscreenMode ]] Trigger redraw of stack after scroll changes
	dirtyall();
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

void MCStack::openwindow(Boolean p_override)
{
	if (MCModeMakeLocalWindows() && window != NULL)
		MCscreen -> openwindow(window, p_override);
}

void MCStack::redrawicon()
{
}

void MCStack::enablewindow(bool p_enable)
{
}

////////////////////////////////////////////////////////////////////////////////

