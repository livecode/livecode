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
#include "card.h"
#include "group.h"
#include "player.h"
#include "image.h"
#include "field.h"
#include "stacklst.h"
#include "cardlst.h"
#include "sellst.h"
#include "mcerror.h"
#include "param.h"
#include "handler.h"
#include "util.h"
#include "mctheme.h"
#include "license.h"
#include "mode.h"
#include "globals.h"
#include "context.h"
#include "region.h"
#include "redraw.h"
#include "tilecache.h"

#include "graphicscontext.h"

#include "w32dc.h"
#include "w32text.h"
#include "w32dnd.h"

#include "resolution.h"

#include "graphics_util.h"

////////////////////////////////////////////////////////////////////////////////

// MW-2011-09-14: [[ Redraw ]] If non-nil, this pixmap is used in the next
//   onpaint update.
// IM-2013-06-19: [[ RefactorGraphics]] replace update pixmap with callback
static MCStackUpdateCallback s_update_callback = nil;
static void *s_update_context = nil;

////////////////////////////////////////////////////////////////////////////////

extern bool create_temporary_dib(HDC p_dc, uint4 p_width, uint4 p_height, HBITMAP& r_bitmap, void*& r_bits);
extern void surface_merge_with_alpha(void *p_pixels, uint4 p_pixel_stride, void *p_alpha, uint4 p_alpha_stride, uint4 p_width, uint4 p_height);

extern MCGFloat MCWin32GetLogicalToScreenScale(void);

extern void MCWin32RegionDestroy(HRGN p_region);
extern bool MCWin32RegionOffset(HRGN p_region, int32_t p_dx, int32_t p_dy);

extern bool MCWin32MCGRegionToHRGN(MCGRegionRef p_region, HRGN &r_region);
extern bool MCWin32HRGNToMCGRegion(HRGN p_region, MCGRegionRef &r_region);
extern bool MCRegionSetAsWindowShape(HRGN region, MCSysWindowHandle window);

////////////////////////////////////////////////////////////////////////////////

static inline MCRectangle MCRectangleFromWin32RECT(const RECT &p_rect)
{
	return MCRectangleMake(p_rect.left, p_rect.top, p_rect.right - p_rect.left, p_rect.bottom - p_rect.top);
}

static bool MCWin32GetBitmapSize(HBITMAP p_bitmap, uint32_t &r_width, uint32_t &r_height)
{
	if (p_bitmap == NULL)
		return false;

	BITMAP t_bitmap_struct;
	GetObjectA(p_bitmap, sizeof(BITMAP), &t_bitmap_struct);

	r_width = t_bitmap_struct.bmWidth;
	r_height = MCAbs(t_bitmap_struct.bmHeight);

	return true;
}

////////////////////////////////////////////////////////////////////////////////

MCStack *MCStack::findstackd(Window w)
{
    // IM-2014-07-09: [[ Bug 12225 ]] Use window ID to find stack
    return findstackwindowid(MCscreen->dtouint((Drawable)w));
}

void MCStack::platform_openwindow(Boolean p_override)
{
	if (MCModeMakeLocalWindows())
		MCscreen -> openwindow(window, p_override);
}

void MCStack::getstyle(uint32_t &wstyle, uint32_t &exstyle)
{
	wstyle = 0;
	exstyle = 0;

	// MW-2007-09-17: [[ Bug 5392 ]] Having decorations set will cause the mode and
	//   resizable properties to be ignored.
	bool t_process_mode;
	t_process_mode = true;

	// MW-2007-07-05: [[ Bug 2145 ]] Modal dialogs don't work well with window-
	//   shapes.
	if (flags & F_DECORATIONS)
	{
		if (decorations & WD_TITLE)
			wstyle |= WS_CAPTION;
		if (decorations & WD_MENU)
			wstyle |= WS_SYSMENU;
		if (decorations & WD_MINIMIZE)
		{
			wstyle |= WS_MINIMIZEBOX;
			wstyle |= WS_SYSMENU;
		}
		if (decorations & WD_MAXIMIZE)
		{
			wstyle |= WS_MAXIMIZEBOX;
			wstyle |= WS_SYSMENU;
		}
		if (decorations & WD_UTILITY)
			exstyle |= WS_EX_TOPMOST;
		if (decorations & WD_FORCETASKBAR)
			exstyle |= WS_EX_APPWINDOW;
		if (wstyle == 0)
		{
			wstyle = WS_POPUP;
			exstyle |= WS_EX_TOOLWINDOW;
			t_process_mode = false;
		}
	}
	else
		wstyle = WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;

	if (t_process_mode)
	{
		switch (mode)
		{
		case WM_DRAWER:
		case WM_PALETTE:
			if (!(flags & F_DECORATIONS) || decorations & WD_TITLE)
				exstyle |= WS_EX_TOOLWINDOW;
			if (flags & F_RESIZABLE)
				wstyle |= WS_THICKFRAME;
			break;
		case WM_MODAL:
		case WM_SHEET:
			wstyle = DS_CENTER | DS_MODALFRAME;
			exstyle = WS_EX_DLGMODALFRAME;
			if (flags & F_RESIZABLE)
				wstyle |= WS_THICKFRAME;
			if (!(flags & F_DECORATIONS) || decorations & WD_TITLE)
			{
				wstyle |= WS_CAPTION | WS_POPUP;
				exstyle |= WS_EX_TOOLWINDOW;
			}
			// may need to set owner here
			break;
		case WM_MODELESS:
			wstyle |= WS_DLGFRAME | DS_CENTER | WS_POPUP;
			if (flags & F_RESIZABLE)
				wstyle |= WS_THICKFRAME;
			else
				wstyle &= ~(WS_THICKFRAME | WS_MAXIMIZEBOX);
			exstyle = WS_EX_TOOLWINDOW;
			break;
		case WM_TOP_LEVEL:
		case WM_TOP_LEVEL_LOCKED:
			// MW-2007-07-05: [[ Bug 2145 ]] Modal dialogs don't work well with window-
			//   shapes.
			if (!(flags & F_DECORATIONS))
				wstyle |= WS_OVERLAPPEDWINDOW;

			// MW-2006-03-20: Bug 2178 - If we are resizable make sure we have a thick-frame
			if (!(flags & F_RESIZABLE))
				wstyle &= ~(WS_THICKFRAME | WS_MAXIMIZEBOX);
			else
				wstyle |= WS_THICKFRAME;
			break;
		default:
			wstyle = WS_POPUP;
			exstyle = WS_EX_TOPMOST | WS_EX_TOOLWINDOW;
			break;
		}
	}

	if (flags & F_DECORATIONS && decorations & WD_UTILITY)
	{
		exstyle |= WS_EX_TOPMOST;
		if (flags & F_RESIZABLE)
			wstyle |= WS_THICKFRAME;
	}

	if (!isopaque() || (m_window_shape != NULL && !m_window_shape -> is_sharp))
	{
		wstyle &= ~(WS_BORDER | WS_CAPTION | WS_DLGFRAME | WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_SIZEBOX | WS_SYSMENU | WS_THICKFRAME);
		exstyle &= ~(WS_EX_CLIENTEDGE | WS_EX_DLGMODALFRAME | WS_EX_STATICEDGE | WS_EX_WINDOWEDGE);
		wstyle |= WS_POPUP;
		exstyle |= WS_EX_LAYERED;
	}
	else if (blendlevel < 100)
		exstyle |= WS_EX_LAYERED;

	if (getextendedstate(ECS_IGNORE_MOUSE_EVENTS) == True)
		exstyle |= WS_EX_LAYERED | WS_EX_TRANSPARENT;
}

static RECT getwrect(MCRectangle rect, DWORD wstyle, DWORD exstyle)
{
	RECT wrect;
	wrect.left = rect.x;
	wrect.top = rect.y;
	wrect.right = rect.x + rect.width;
	wrect.bottom = rect.y + rect.height;
	AdjustWindowRectEx(&wrect, wstyle, False, exstyle);
	return wrect;
}

struct MCNativeLayerReattachVisitor : public MCObjectVisitor
{
public:
	virtual bool OnObject(MCObject *p_control)
	{
		MCNativeLayer *t_layer;
		t_layer = p_control->getNativeLayer();
		if (t_layer != nil && t_layer->isAttached())
		{
			// reattach native layer view to new window
			t_layer->OnDetach();
			t_layer->OnAttach();
		}

		return true;
	}
};

// MW-2006-03-20: Bug 3316 - There seems absolutely no way of preventing flicker
//   when transitioning from no alpha to alpha. Therefore, we need to create a new
//   window opened *behind* the current window; show it; redraw it; then delete
//   the existing window.
void MCStack::setopacity(uint1 p_level)
{
	// If the stack is not ours to open, then we do nothing ('runtime' mode/remoteable
	// window).
	if (!MCModeMakeLocalWindows())
		return;

	// Do nothing if not NT
	if (MCmajorosversion < 0x0500)
		return;

	if (!isopaque() || m_window_shape != NULL && !m_window_shape -> is_sharp)
		composite();
	else if (window != NULL)
	{
		HWND t_old_window;
		t_old_window = NULL;

		DWORD t_old_long, t_new_long;
		t_old_long = GetWindowLong((HWND)window -> handle . window, GWL_EXSTYLE);

		if (p_level == 255)
			t_new_long = t_old_long & ~WS_EX_LAYERED;
		else
			t_new_long = t_old_long | WS_EX_LAYERED;

		if (t_new_long != t_old_long)
		{
			if (IsWindowVisible((HWND)window -> handle . window) && (t_new_long & WS_EX_LAYERED) != 0)
				t_old_window = (HWND)window -> handle . window;
			else
				SetWindowLong((HWND)window -> handle . window, GWL_EXSTYLE, t_new_long);
		}

		if (t_old_window != NULL)
		{
			uint32_t t_style, t_ex_style;
			getstyle(t_style, t_ex_style);

			// MW-2006-07-27: [[ Bug 3690 ]] - Make sure layered attribute is set if we need it
			t_ex_style = (t_ex_style & ~WS_EX_LAYERED) | (t_new_long & WS_EX_LAYERED);

			RECT t_rect;
			t_rect = getwrect(rect, t_style, t_ex_style);

			Bool t_is_xp_menu;
			t_is_xp_menu = (mode == WM_PULLDOWN || mode == WM_POPUP || mode == WM_CASCADE) && (MCcurtheme && MCcurtheme->getthemeid() == LF_NATIVEWIN);

            MCAutoStringRefAsWString t_window_name;
            /* UNCHECKED */ t_window_name.Lock(MCNameGetString(getname()));
            window -> handle . window = (MCSysWindowHandle)CreateWindowExW(t_ex_style, MC_WIN_CLASS_NAME_W, *t_window_name, t_style | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, t_rect . left, t_rect . top, t_rect . right - t_rect . left, t_rect . bottom - t_rect . top, NULL, NULL, MChInst, NULL);
			
			// MW-2010-10-22: [[ Bug 8151 ]] Make sure we update the title string.
			MCscreen -> setname(window, titlestring);

			SetWindowLongPtrA((HWND)window->handle.window, GWLP_USERDATA, mode);
			
			if (flags & F_DECORATIONS && !(decorations & WD_SHAPE) && !(decorations & WD_CLOSE))
				EnableMenuItem(GetSystemMenu((HWND)window->handle.window, False), SC_CLOSE, MF_BYCOMMAND | MF_GRAYED);
		
			if (m_window_shape != nil && m_window_shape -> is_sharp)
			{
				HRGN t_region;
				t_region = (HRGN)m_window_shape -> handle;
				MCWin32RegionOffset(t_region, rect . x - t_rect . left, rect . y - t_rect . top);
				MCRegionSetAsWindowShape(t_region, window->handle.window);

				// The window now owns the region.
				m_window_shape -> handle = nil;
			}

			RevokeDragDrop(t_old_window);
			CoLockObjectExternal(droptarget, FALSE, TRUE);
			droptarget -> setstack(NULL);

			droptarget -> setstack(this);
			CoLockObjectExternal(droptarget, TRUE, TRUE);
			RegisterDragDrop((HWND)window -> handle . window, droptarget);

			SetWindowPos((HWND)window -> handle . window, t_old_window, 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOREDRAW | SWP_NOSENDCHANGING | SWP_NOSIZE | SWP_SHOWWINDOW);

			// Reattach any native layers on this stack
			MCNativeLayerReattachVisitor t_visitor;
			visit_children(kMCObjectVisitorRecursive | kMCObjectVisitorHeirarchical, 0, &t_visitor);
		}

		if (p_level < 255)
			SetLayeredWindowAttributes((HWND)window -> handle . window, 0, p_level, LWA_ALPHA);

		if (t_old_window != NULL)
		{
			RedrawWindow((HWND)window -> handle . window, NULL, NULL, RDW_INVALIDATE | RDW_FRAME | RDW_UPDATENOW);
			DestroyWindow(t_old_window);
		}
	}
}

void MCStack::destroywindowshape()
{
	if (m_window_shape == nil)
		return;

	// Delete the data ptr (might be nil).
	delete[] m_window_shape -> data;

	// If the mask is sharp, delete the region; otherwise
	// handle might be a pixmap.
	if (m_window_shape -> is_sharp)
		MCWin32RegionDestroy((HRGN)m_window_shape -> handle);
	else
	{
		HBITMAP t_bitmap = (HBITMAP)m_window_shape->handle;
		if (t_bitmap != nil)
			DeleteObject(t_bitmap);
	}

	delete m_window_shape;
	m_window_shape = nil;
}

void MCStack::realize()
{
	if (MCnoui)
	{
		start_externals();
		return;
	}

	if (MCModeMakeLocalWindows())
	{
		uint32_t wstyle, exstyle;
		loadwindowshape();
		getstyle(wstyle, exstyle);
		RECT wrect ;

		// IM-2013-09-23: [[ FullscreenMode ]] Don't change stack rect if fullscreen
		/* CODE DELETED */

		MCRectangle t_rect;
		// IM-2014-01-16: [[ StackScale ]] Use scaled view rect as window size
		t_rect = view_getrect();

		// IM-2014-01-28: [[ HiDPI ]] Convert logical to screen coords
		t_rect = MCscreen->logicaltoscreenrect(t_rect);

		wrect = getwrect(t_rect, wstyle, exstyle);
		LONG x = wrect.left;
		LONG y = wrect.top;
		LONG width = wrect.right - wrect.left;
		LONG height = wrect.bottom - wrect.top;
		if (flags & F_WM_PLACE && !(state & CS_BEEN_MOVED))
			x = CW_USEDEFAULT;
		window = new (nothrow) _Drawable;
		window->type = DC_WINDOW;
		window->handle.window = 0; // protect against creation callbacks

		// MW-2011-02-15: [[ Bug 9396 ]] Tooltips should have a shadow on Windows.
		// IM-2015-03-12: [[ WidgetPopup ]] Disable dropshadow for non-opaque stacks.
		Boolean isxpmenu = (mode == WM_PULLDOWN || mode == WM_POPUP || mode == WM_CASCADE || mode == WM_TOOLTIP) \
		                   && (MCcurtheme && MCcurtheme->getthemeid() == LF_NATIVEWIN) && isopaque();

		// MW-2012-09-07: [[ Bug 10368 ]] If the 'no-shadow' bit is set, then we don't
		//   want to use the menu class.
		if (getflag(F_DECORATIONS) && (decorations & WD_NOSHADOW) != 0)
			isxpmenu = False;

		HWND t_parenthwnd = NULL;

        MCAutoStringRefAsWString t_window_name;
        /* UNCHECKED */ t_window_name.Lock(MCNameGetString(getname()));
        window -> handle . window = (MCSysWindowHandle)CreateWindowExW(exstyle, MC_WIN_CLASS_NAME_W, *t_window_name, wstyle | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, x, y, width, height,
                                                                       t_parenthwnd, NULL, MChInst, NULL);

		SetWindowLongPtrA((HWND)window->handle.window, GWLP_USERDATA, mode);
		
		if (flags & F_DECORATIONS && !(decorations & WD_SHAPE) && !(decorations & WD_CLOSE))
			EnableMenuItem(GetSystemMenu((HWND)window->handle.window, False), SC_CLOSE, MF_BYCOMMAND | MF_GRAYED);

		if (m_window_shape != nil && m_window_shape -> is_sharp)
		{
			HRGN t_region;
			t_region = (HRGN)m_window_shape -> handle;
			MCWin32RegionOffset(t_region, t_rect . x - wrect . left, t_rect . y - wrect . top);
			MCRegionSetAsWindowShape(t_region, window->handle.window);

			// The window now owns the region.
			m_window_shape -> handle = nil;
		}

		if ((m_window_shape == NULL || m_window_shape -> is_sharp) && blendlevel != 100)
			setopacity(blendlevel * 255 / 100);
	}
	
	// IM-2014-01-28: [[ HiDPI ]] Initialize window backing scale to the system DPI
	// TODO - Windows 8 implementation will require getting the appropriate per-window value
	view_setbackingscale(MCWin32GetLogicalToScreenScale());

	start_externals();
}

void MCStack::setsizehints()
{
}

void MCStack::sethints()
{
	if (!opened || MCnoui || window == DNULL)
		return;
	MCStack *sptr = MCdefaultstackptr == this ? MCtopstackptr : MCdefaultstackptr;
	if (sptr != NULL && sptr != this && sptr->getw() != DNULL
	        && GetWindowLongA((HWND)window->handle.window, 0) == 0)
		SetWindowLongA((HWND)window->handle.window, 0, (LONG)sptr->getw()->handle.window);
}

bool MCStack::view_platform_dirtyviewonresize() const
{
	return false;
}

MCRectangle MCStack::view_platform_getwindowrect() const
{
	if (window == DNULL)
		return rect;

	RECT wrect;
	GetWindowRect((HWND)window->handle.window, &wrect);

	MCRectangle t_frame_rect;
	t_frame_rect = MCRectangleFromWin32RECT(wrect);
    
    // IM-2014-01-28: [[ HiDPI ]] Convert screen to logical coords
    t_frame_rect = MCscreen->screentologicalrect(t_frame_rect);
    
    if (MClockscreen != 0)
    {
        MCRectangle t_content_rect, t_diff_rect;
        MCscreen->platform_getwindowgeometry(window, t_content_rect);
        // the content rect of a window should always be contained (or equal) to the frame rect
        // so compute these 4 margins and then apply them to the rect of the stack
        t_diff_rect.x = rect.x - (t_content_rect.x - t_frame_rect.x);
        t_diff_rect.y = rect.y - (t_content_rect.y - t_frame_rect.y);
        t_diff_rect.width = rect.width + (t_frame_rect.width - t_content_rect.width);
        t_diff_rect.height = rect.height + (t_frame_rect.height - t_content_rect.height);
        return t_diff_rect;
    }
    
	return t_frame_rect;
}

// IM-2013-09-23: [[ FullscreenMode ]] Factor out device-specific window sizing
MCRectangle MCStack::view_platform_setgeom(const MCRectangle &p_rect)
{
	MCRectangle t_new_rect;
	t_new_rect = p_rect;

	// IM-2014-01-28: [[ HiDPI ]] Convert logical to screen coords
	t_new_rect = MCscreen->logicaltoscreenrect(t_new_rect);

	uint32_t wstyle, exstyle;
	getstyle(wstyle, exstyle);
	RECT newrect = getwrect(t_new_rect, wstyle, exstyle);
	RECT wrect;
	GetWindowRect((HWND)window->handle.window, &wrect);

	MCRectangle t_old_rect;
	t_old_rect = MCRectangleFromWin32RECT(wrect);

	// IM-2014-01-28: [[ HiDPI ]] Convert screen to logical coords
	t_old_rect = MCscreen->screentologicalrect(t_old_rect);

	LONG t_width = newrect.right - newrect.left;
	LONG t_height = newrect.bottom - newrect.top;

	if (t_old_rect.x != p_rect.x || t_old_rect.y != p_rect.y
	        || t_old_rect.width != p_rect.width || t_old_rect.height != p_rect.height)
	{
		state |= CS_NO_CONFIG;
		MoveWindow((HWND)window->handle.window, newrect.left, newrect.top, t_width, t_height, True);
		state &= ~CS_NO_CONFIG;
	}

	return t_old_rect;
}

void MCStack::setgeom()
{
	if (!opened)
		return;

	// MW-2009-09-25: Ensure things are the right size when doing
	//   remote dialog/menu windows.
	if (window == DNULL)
	{
		state &= ~CS_NEED_RESIZE;
		// MW-2011-08-18: [[ Redraw ]] Update to use redraw.
		MCRedrawLockScreen();
		resize(rect . width, rect . height);
		MCRedrawUnlockScreen();
		mode_setgeom();
		return;
	}

	// IM-2013-10-04: [[ FullscreenMode ]] Use view methods to get / set the stack viewport
	MCRectangle t_old_rect;
	t_old_rect = view_getstackviewport();
	
	rect = view_setstackviewport(rect);
	
	state &= ~CS_NEED_RESIZE;
	
	// IM-2013-10-04: [[ FullscreenMode ]] Return values from view methods are
	// in stack coords so don't need to transform
	if (t_old_rect.x != rect.x || t_old_rect.y != rect.y || t_old_rect.width != rect.width || t_old_rect.height != rect.height)
		resize(t_old_rect.width, t_old_rect.height);
}

void MCStack::constrain(intptr_t lp)
{
	uint32_t wstyle, exstyle;
	getstyle(wstyle, exstyle);

	// IM-2014-05-27: [[ Bug 12462 ]] Convert stack rect to screen coords
	MCRectangle t_rect;
	t_rect = MCscreen->logicaltoscreenrect(rect);

	RECT wrect = getwrect(t_rect, wstyle, exstyle);
	int4 dx = wrect.right - wrect.left - t_rect.width;
	int4 dy = wrect.bottom - wrect.top - t_rect.height;
	LPMINMAXINFO mmptr = (LPMINMAXINFO)lp;
	const MCDisplay *t_display;
	t_display = MCscreen -> getnearestdisplay(rect);

	if (mode < WM_MODAL)
	{
		MCRectangle t_workarea, t_viewport;
		if (MCU_point_in_rect(t_display -> workarea, MCwbr . x, MCwbr . y))
			t_workarea = MCU_intersect_rect(MCwbr, t_display -> workarea);
		else
			t_workarea = t_display -> workarea;
		t_viewport = t_display -> viewport;

		if (memcmp(&t_workarea, &t_display -> workarea, sizeof(MCRectangle)))
			MCU_reduce_rect(t_workarea, -dx / 2);

		// IM-2014-05-27: [[ Bug 12462 ]] Convert screen rects to screen coords
		t_workarea = MCscreen->logicaltoscreenrect(t_workarea);
		t_viewport = MCscreen->logicaltoscreenrect(t_viewport);

		mmptr -> ptMaxSize . x = MCU_min(maxwidth + dx, t_workarea . width);
		mmptr -> ptMaxSize . y = MCU_min(maxheight + dy, t_workarea . height);
		mmptr -> ptMaxPosition . x = t_workarea . x - t_viewport . x;
		mmptr -> ptMaxPosition . y = t_workarea . y - t_viewport . y;
	}
		
	MCGFloat t_screenPixelScale = t_display->pixel_scale;

	// AB-2018-10-23: We no longer support Windows 98 -> remove clamping
	mmptr -> ptMinTrackSize . x = minwidth * t_screenPixelScale + dx;
	mmptr -> ptMinTrackSize . y = minheight * t_screenPixelScale + dy;
	mmptr -> ptMaxTrackSize . x = maxwidth * t_screenPixelScale + dx;
	mmptr -> ptMaxTrackSize . y = maxheight * t_screenPixelScale + dy;

}

void MCStack::applyscroll(void)
{
}

void MCStack::clearscroll(void)
{
}

static void deleter(char *data)
{
	delete data;
}

void MCStack::start_externals()
{
	loadexternals();

	if (!MCnoui && window != DNULL)
	{
		droptarget = new (nothrow) CDropTarget;
		droptarget->setstack(this);
		CoLockObjectExternal(droptarget, TRUE, TRUE);
		RegisterDragDrop((HWND)window->handle.window, droptarget);
	}
}

void MCStack::stop_externals()
{
	Boolean oldlock = MClockmessages;
	MClockmessages = True;
    
    MCPlayer::StopPlayers(this);

	if (!MCnoui && window != DNULL)
	{
		RevokeDragDrop((HWND)window->handle.window);
		CoLockObjectExternal(droptarget, FALSE, TRUE);
		droptarget->setstack(NULL);
		delete droptarget;
		droptarget = nil;
	}

	destroywindowshape();
	release_window_buffer();

	MClockmessages = oldlock;

	unloadexternals();
}

void MCStack::updatemodifiedmark(void)
{
}

// MERG-2014-06-02: [[ IgnoreMouseEvents ]] Stub for ignoreMouseEvents.
void MCStack::updateignoremouseevents(void)
{
	uint32_t t_window_style_ex;
	uint32_t t_window_style;
	
	getstyle(t_window_style, t_window_style_ex);
	
	SetWindowLongA((HWND)getw() -> handle . window, GWL_EXSTYLE, t_window_style_ex);
}

void MCStack::enablewindow(bool p_enable)
{
	EnableWindow((HWND)getw() -> handle . window, p_enable);
}

void MCStack::redrawicon(void)
{
}

// MERG-2015-10-12: [[ DocumentFilename ]] Stub for documentFilename.
void MCStack::updatedocumentfilename(void)
{
}

////////////////////////////////////////////////////////////////////////////////

bool __MCApplyMaskCallback(void *p_context, const MCGIntegerRectangle &p_rect)
{
	MCGContextRef t_context = static_cast<MCGContextRef>(p_context);

	MCGContextAddRectangle(t_context, MCGIntegerRectangleToMCGRectangle(p_rect));
	MCGContextFill(t_context);

	return true;
}

////////////////////////////////////////////////////////////////////////////////

// MM-2014-07-31: [[ ThreadedRendering ]] Updated to implement the new stack surface API.
class MCWindowsStackSurface: public MCStackSurface
{
	MCStack *m_stack;
	MCGRegionRef m_region;
	HDC m_dc;

	MCGRaster *m_mask;

	MCGRegionRef m_redraw_region;
	MCGIntegerRectangle m_area;

	HBITMAP m_bitmap;
	MCGRaster m_raster;

	bool m_is_opaque;

public:
	MCWindowsStackSurface(MCGRaster *p_mask, MCGRegionRef p_region, HDC p_dc, bool p_is_opaque)
	{
		m_region = p_region;
		m_dc = p_dc;

		m_mask = p_mask;

		m_bitmap = nil;

		m_is_opaque = p_is_opaque;
	}

	bool Lock(void)
	{
		if (m_bitmap != nil)
			return false;

		MCGIntegerRectangle t_actual_area;
		t_actual_area = MCGRegionGetBounds(m_region);
		if (MCGIntegerRectangleIsEmpty(t_actual_area))
			return false;

		bool t_success = true;

		void *t_bits = nil;

		if (t_success)
			t_success = MCGRegionCreate(m_redraw_region);

		if (t_success)
			t_success = create_temporary_dib(m_dc, t_actual_area . size . width, t_actual_area . size . height, m_bitmap, t_bits);

		if (t_success)
		{
			m_raster . format = kMCGRasterFormat_ARGB;
			m_raster . width = t_actual_area . size . width;
			m_raster . height = t_actual_area . size . height;
			m_raster . stride = t_actual_area . size . width * sizeof(uint32_t);
			m_raster . pixels = t_bits;

			m_area = t_actual_area;

			return true;
		}

		MCGRegionDestroy(m_redraw_region);
		m_redraw_region = nil;

		if (m_bitmap != nil)
			DeleteObject(m_bitmap);
		m_bitmap = nil;

		return false;
	}

	void Unlock(void)
	{
		Unlock(true);
	}

	void Unlock(bool p_update)
	{
		if (m_bitmap == nil)
			return;

		if (p_update)
		{
			if (m_mask != nil)
				MCGRasterApplyAlpha(m_raster, *m_mask, m_area.origin);

			HDC t_src_dc = ((MCScreenDC *)MCscreen) -> getsrchdc();

			HGDIOBJ t_old_object;
			HRGN t_old_clip, t_clip;

			/* UNCHECKED */ MCWin32MCGRegionToHRGN(m_redraw_region, t_clip);

			t_old_clip = CreateRectRgn(0, 0, 0, 0);
			GetClipRgn(m_dc, t_old_clip);
			t_old_object = SelectObject(t_src_dc, m_bitmap);
			SelectClipRgn(m_dc, t_clip);

			BitBlt(m_dc, m_area.origin.x, m_area.origin.y, m_area.size.width, m_area.size.height, t_src_dc, 0, 0, SRCCOPY);

			SelectClipRgn(m_dc, t_old_clip);
			SelectObject(t_src_dc, t_old_object);

			DeleteObject(t_old_clip);
			DeleteObject(t_clip);
		}

		MCGRegionDestroy(m_redraw_region);
		DeleteObject(m_bitmap);
		m_bitmap = nil;
	}

    bool LockGraphics(MCGIntegerRectangle p_area, MCGContextRef &r_context, MCGRaster &r_raster)
	{
		MCGRaster t_raster;
		MCGIntegerRectangle t_locked_area;
		if (LockPixels(p_area, t_raster, t_locked_area))
		{
            MCGContextRef t_context;
            if (MCGContextCreateWithRaster(t_raster, t_context))
			{
				// Set origin
                MCGContextTranslateCTM(t_context, -t_locked_area . origin . x, -t_locked_area . origin . y);
                
				// Set clipping rect
				MCGContextClipToRegion(t_context, m_region);
				MCGContextClipToRect(t_context, MCGIntegerRectangleToMCGRectangle(t_locked_area));
				
				r_context = t_context;
                r_raster = t_raster;
				
				return true;
			}
			
			UnlockPixels(t_locked_area, t_raster);
		}
		
		return false;
	}

	void UnlockGraphics(MCGIntegerRectangle p_area, MCGContextRef p_context, MCGRaster &p_raster)
	{
		if (p_context == nil)
			return;
		
		MCGContextRelease(p_context);
		UnlockPixels(p_area, p_raster);
	}

    bool LockPixels(MCGIntegerRectangle p_area, MCGRaster& r_raster, MCGIntegerRectangle &r_locked_area)
    {
		if (m_bitmap == nil)
			return false;

		MCGIntegerRectangle t_bounds;
		t_bounds = MCGRegionGetBounds(m_region);

        MCGIntegerRectangle t_actual_area;
        t_actual_area = MCGIntegerRectangleIntersection(p_area, t_bounds);
        
        if (MCGIntegerRectangleIsEmpty(t_actual_area))
            return false;

		/* UNCHECKED */ MCGRegionAddRect(m_redraw_region, t_actual_area);
        
        r_raster . width = t_actual_area . size . width ;
        r_raster . height = t_actual_area . size . height;
        r_raster . stride = m_raster . stride;
		r_raster . format = m_is_opaque ? kMCGRasterFormat_xRGB : kMCGRasterFormat_ARGB;
		r_raster . pixels = (uint8_t*)m_raster . pixels + (t_actual_area . origin . y - t_bounds . origin.y) * m_raster . stride + (t_actual_area . origin . x - t_bounds . origin . x) * sizeof(uint32_t);

		r_locked_area = t_actual_area;

        return true;
	}

	void UnlockPixels(MCGIntegerRectangle p_area, MCGRaster& p_raster)
	{
	}

	bool LockTarget(MCStackSurfaceTargetType p_type, void*& r_context)
	{
		return false;
	}
	
	void UnlockTarget(void)
	{
	}

	bool Composite(MCGRectangle p_dst_rect, MCGImageRef p_src, MCGRectangle p_src_rect, MCGFloat p_alpha, MCGBlendMode p_blend)
	{
		bool t_success = true;
				
        MCGIntegerRectangle t_bounds;
        MCGContextRef t_context = nil;
        MCGRaster t_raster;
		if (t_success)
        {
            t_bounds = MCGRectangleGetBounds(p_dst_rect);
            t_success = LockGraphics(t_bounds, t_context, t_raster);
        }
		
		if (t_success)
		{
			// MW-2013-11-08: [[ Bug ]] Make sure we set the blend/alpha on the context.
			MCGContextSetBlendMode(t_context, p_blend);
			MCGContextSetOpacity(t_context, p_alpha);

            // MM-2014-01-27: [[ UpdateImageFilters ]] Updated to use new libgraphics image filter types (was nearest).
			MCGContextDrawRectOfImage(t_context, p_src, p_src_rect, p_dst_rect, kMCGImageFilterNone);
		}
		
		UnlockGraphics(t_bounds, t_context, t_raster);
		
		return t_success;
	}
};

// MM-2014-07-31: [[ ThreadedRendering ]] Updated to implement the new stack surface API.
class MCWindowsLayeredStackSurface: public MCStackSurface
{
	MCGIntegerPoint m_raster_offset;
	MCGRaster m_raster;
	MCGRaster *m_mask;
	MCGRegionRef m_redraw_region;

	bool m_locked;
	bool m_is_opaque;

public:
	MCWindowsLayeredStackSurface(MCGRaster p_raster, const MCGIntegerPoint &p_raster_offset, MCGRaster *p_mask)
	{
		m_raster = p_raster;
		m_raster_offset = p_raster_offset;
		m_mask = p_mask;
		m_redraw_region = nil;
		m_locked = false;
	}

	bool Lock()
	{
		if (m_locked)
			return false;

		if (!MCGRegionCreate(m_redraw_region))
			return false;

		m_locked = true;

		return true;
	}

	void Unlock(void)
	{
		if (!m_locked)
			return;

		Unlock(true);

		MCGRegionDestroy(m_redraw_region);
		m_redraw_region = nil;
		m_locked = false;
	}

	void Unlock(bool p_update)
	{
		if (p_update)
		{
			void *t_src_ptr, *t_dst_ptr;
			if (m_mask != nil)
				MCGRasterApplyAlpha(m_raster, *m_mask, MCGIntegerPointMake(-m_raster_offset.x, -m_raster_offset.y));
		}
	}

	
    bool LockGraphics(MCGIntegerRectangle p_area, MCGContextRef &r_context, MCGRaster &r_raster)
	{
		MCGRaster t_raster;
		MCGIntegerRectangle t_locked_area;
		if (LockPixels(p_area, t_raster, t_locked_area))
		{
            MCGContextRef t_context;
            if (MCGContextCreateWithRaster(t_raster, t_context))
			{
				// Set origin
                MCGContextTranslateCTM(t_context, -t_locked_area . origin . x, -t_locked_area . origin . y);
                
				// Set clipping rect
				MCGContextClipToRect(t_context, MCGIntegerRectangleToMCGRectangle(t_locked_area));
				
				r_context = t_context;
                r_raster = t_raster;
				
				return true;
			}
			
			UnlockPixels(t_locked_area, t_raster);
		}
		
		return false;
	}
	
	void UnlockGraphics(MCGIntegerRectangle p_area, MCGContextRef p_context, MCGRaster &p_raster)
	{
		if (p_context == nil)
			return;
		
		MCGContextRelease(p_context);
		UnlockPixels(p_area, p_raster);
	}
	
    bool LockPixels(MCGIntegerRectangle p_area, MCGRaster& r_raster, MCGIntegerRectangle &r_locked_area)
    {
		if (m_raster . pixels == nil)
			return false;

        MCGIntegerRectangle t_actual_area;
        t_actual_area = MCGIntegerRectangleIntersection(p_area, MCGIntegerRectangleMake(m_raster_offset.x, m_raster_offset.y, m_raster . width, m_raster . height));
        
        if (MCGIntegerRectangleIsEmpty(t_actual_area))
            return false;

		/* UNCHECKED */ MCGRegionAddRect(m_redraw_region, t_actual_area);
        
        r_raster . width = t_actual_area . size . width ;
        r_raster . height = t_actual_area . size . height;
        r_raster . stride = m_raster . stride;
        r_raster . format = m_raster . format;
		r_raster . pixels = (uint8_t*) m_raster . pixels + (t_actual_area . origin . y - m_raster_offset.y) * m_raster . stride + (t_actual_area . origin . x - m_raster_offset.x) * sizeof(uint32_t);

		r_locked_area = t_actual_area;

        return true;
	}
	
	void UnlockPixels(MCGIntegerRectangle p_area, MCGRaster& p_raster)
	{
	}

	bool LockTarget(MCStackSurfaceTargetType p_type, void*& r_context)
	{
		return false;
	}
	
	void UnlockTarget(void)
	{
	}

	bool Composite(MCGRectangle p_dst_rect, MCGImageRef p_src, MCGRectangle p_src_rect, MCGFloat p_alpha, MCGBlendMode p_blend)
	{
		bool t_success = true;
				
        MCGIntegerRectangle t_bounds;
        MCGContextRef t_context = nil;
        MCGRaster t_raster;
		if (t_success)
        {
            t_bounds = MCGRectangleGetBounds(p_dst_rect);
            t_success = LockGraphics(t_bounds, t_context, t_raster);
        }
		
		if (t_success)
		{
			// MW-2013-11-08: [[ Bug ]] Make sure we set the blend/alpha on the context.
			MCGContextSetBlendMode(t_context, p_blend);
			MCGContextSetOpacity(t_context, p_alpha);

            // MM-2014-01-27: [[ UpdateImageFilters ]] Updated to use new libgraphics image filter types (was nearest).
			MCGContextDrawRectOfImage(t_context, p_src, p_src_rect, p_dst_rect, kMCGImageFilterNone);
		}
		
		UnlockGraphics(t_bounds, t_context, t_raster);
		
		return t_success;
	}
};

bool MCWin32GetWindowShapeAlphaMask(MCWindowShape *p_shape, MCGRaster &r_mask)
{
	if (p_shape == nil || p_shape->data == nil || p_shape->is_sharp)
		return false;

	r_mask.width = p_shape->width;
	r_mask.height = p_shape->height;
	r_mask.pixels = p_shape->data;
	r_mask.stride = p_shape->stride;
	r_mask.format = kMCGRasterFormat_A;

	return true;
}

void MCStack::view_platform_updatewindow(MCRegionRef p_region)
{
	// IM-2014-01-28: [[ HiDPI ]] Create surface at the view's backing scale
	MCGFloat t_surface_scale;
	t_surface_scale = view_getbackingscale();

	MCGAffineTransform t_surface_transform;
	t_surface_transform = MCGAffineTransformMakeScale(t_surface_scale, t_surface_scale);

	MCGRegionRef t_surface_region, t_scaled_region;
	t_surface_region = t_scaled_region = nil;

	if (MCGAffineTransformIsIdentity(t_surface_transform))
		t_surface_region = (MCGRegionRef)p_region;
	else
	{
		/* UNCHECKED */ MCGRegionCopyWithTransform((MCGRegionRef)p_region, t_surface_transform, t_scaled_region);
		t_surface_region = t_scaled_region;
	}

	if (isopaque() && (m_window_shape == nil || m_window_shape -> is_sharp))
	{
		HRGN t_hrgn;
		/* UNCHECKED */ MCWin32MCGRegionToHRGN(t_surface_region, t_hrgn);

		// TODO - Windows 8.1 per-monitor DPI-awareness may require the update region be in logical coords
		InvalidateRgn((HWND)window -> handle . window, t_hrgn, FALSE);
		UpdateWindow((HWND)window -> handle . window);

		MCWin32RegionDestroy(t_hrgn);
	}
	else
	{
		MCGIntegerRectangle t_region_bounds = MCGRegionGetBounds(t_surface_region);

		/* UNCHECKED */ configure_window_buffer();

		HBITMAP t_bitmap;
		t_bitmap = (HBITMAP)m_window_buffer;

		BITMAP t_bitmap_struct;
		GetObjectA(t_bitmap, sizeof(BITMAP), &t_bitmap_struct);

		t_region_bounds = MCGIntegerRectangleIntersection(t_region_bounds, MCGIntegerRectangleMake(0, 0, t_bitmap_struct.bmWidth, t_bitmap_struct.bmHeight));

		MCGRaster t_raster;
		t_raster.width = t_region_bounds.size.width;
		t_raster.height = t_region_bounds.size.height;
		t_raster.stride = t_bitmap_struct.bmWidthBytes;
		t_raster.pixels = ((uint8_t*)t_bitmap_struct.bmBits) + t_region_bounds.origin.y * t_raster.stride + t_region_bounds.origin.x * sizeof(uint32_t);
		t_raster.format = isopaque() ? kMCGRasterFormat_xRGB : kMCGRasterFormat_ARGB;

		MCGRaster *t_mask_ptr;
		t_mask_ptr = nil;

		MCGRaster t_mask;
		if (m_window_shape != nil && MCWin32GetWindowShapeAlphaMask(m_window_shape, t_mask))
			t_mask_ptr = &t_mask;

		MCWindowsLayeredStackSurface t_surface(t_raster, t_region_bounds.origin, t_mask_ptr);

		if (t_surface.Lock())
		{
			if (s_update_callback == nil)
				view_surface_redrawwindow(&t_surface, t_surface_region);
			else
				s_update_callback(&t_surface, (MCRegionRef)t_surface_region, s_update_context);

			t_surface.Unlock();

			composite();
		}
	}

	// IM-2014-03-27: [[ Bug 12010 ]] Free the created surface region once the update is done.
	MCGRegionDestroy(t_scaled_region);
}

void MCStack::view_platform_updatewindowwithcallback(MCRegionRef p_region, MCStackUpdateCallback p_callback, void *p_context)
{
	s_update_callback = p_callback;
	s_update_context = p_context;
	view_platform_updatewindow(p_region);
	s_update_callback = nil;
	s_update_context = nil;
}

void MCStack::onpaint(void)
{
	HRGN t_hrgn;
	t_hrgn = CreateRectRgn(0, 0, 0, 0);
	GetUpdateRgn((HWND)window -> handle . window, t_hrgn, FALSE);

	MCGRegionRef t_update_region;
	t_update_region = nil;

	/* UNCHECKED */ MCWin32HRGNToMCGRegion(t_hrgn, t_update_region);
	DeleteObject(t_hrgn);

	PAINTSTRUCT t_paint;
	BeginPaint((HWND)window -> handle . window, &t_paint);

	MCGRaster t_mask;
	MCGRaster *t_mask_ptr;

	if (MCWin32GetWindowShapeAlphaMask(getwindowshape(), t_mask))
		t_mask_ptr = &t_mask;
	else
		t_mask_ptr = nil;

	MCWindowsStackSurface t_surface(t_mask_ptr, t_update_region, t_paint . hdc, isopaque());

	if (t_surface.Lock())
	{
		if (s_update_callback == nil)
			view_surface_redrawwindow(&t_surface, t_update_region);
		else
			s_update_callback(&t_surface, (MCRegionRef)t_update_region, s_update_context);

		t_surface.Unlock();
	}

	EndPaint((HWND)window -> handle . window, &t_paint);

	MCGRegionDestroy(t_update_region);
}

void MCStack::composite(void)
{
	if (m_window_buffer == nil)
		return;

	POINT t_offset;
	POINT t_location;
	SIZE t_size;

	HDC t_dst_dc;
	t_dst_dc = ((MCScreenDC *)MCscreen) -> getdsthdc();

	HGDIOBJ t_old_dst;
	HBITMAP t_bitmap;
	t_bitmap = (HBITMAP)m_window_buffer;
	t_old_dst = SelectObject(t_dst_dc, t_bitmap);

	MCRectangle t_device_stack_rect;
	// IM-2014-01-28: [[ HiDPI ]] Convert logical to screen coords
	t_device_stack_rect = MCscreen->logicaltoscreenrect(rect);

	uint32_t t_width, t_height;
	/* UNCHECKED */ MCWin32GetBitmapSize(t_bitmap, t_width, t_height);

	MCRectangle t_device_shape_rect;
	t_device_shape_rect = MCRectangleMake(0, 0, t_width, t_height);

	t_offset . x = 0;
	t_offset . y = 0;
	t_location . x = t_device_stack_rect . x;
	t_location . y = t_device_stack_rect . y;
	t_size . cx = t_width;;
	t_size . cy = t_height;

	BLENDFUNCTION t_blend;
	t_blend . BlendOp = AC_SRC_OVER;
	t_blend . BlendFlags = 0;
	t_blend . SourceConstantAlpha = blendlevel * 255 / 100;
	t_blend . AlphaFormat = AC_SRC_ALPHA;

	UpdateLayeredWindow((HWND)window -> handle . window, t_dst_dc, &t_location, &t_size, t_dst_dc, &t_offset, 0, &t_blend, ULW_ALPHA);

	SelectObject(t_dst_dc, t_old_dst);
}

////////////////////////////////////////////////////////////////////////////////

bool MCStack::configure_window_buffer()
{
	bool t_need_buffer;
	t_need_buffer = false;

	uint32_t t_width, t_height;

	if (!isopaque())
	{
		t_need_buffer = true;
		t_width = rect.width;
		t_height = rect.height;
	}
	else if (m_window_shape != nil && !m_window_shape->is_sharp)
	{
		t_need_buffer = true;
		t_width = m_window_shape->width;
		t_height = m_window_shape->height;
	}
	else
		t_need_buffer = false;

	if (!t_need_buffer)
	{
		release_window_buffer();
		return true;
	}

	if (m_window_buffer != nil)
	{
		uint32_t t_buffer_width, t_buffer_height;
		if (MCWin32GetBitmapSize((HBITMAP)m_window_buffer, t_buffer_width, t_buffer_height) &&
			t_buffer_width == t_width && t_buffer_height == t_height)
			return true;

		release_window_buffer();
	}

	HBITMAP t_bitmap;
	t_bitmap = nil;
	void *t_bits = nil;

	if (!create_temporary_dib(((MCScreenDC*)MCscreen)->getdsthdc(), t_width, t_height, t_bitmap, t_bits))
		return false;

	m_window_buffer = (MCSysBitmapHandle)t_bitmap;
	return true;
}

void MCStack::release_window_buffer()
{
	if (m_window_buffer == NULL)
		return;

	DeleteObject((HBITMAP)m_window_buffer);
	m_window_buffer = nil;
}

////////////////////////////////////////////////////////////////////////////////
