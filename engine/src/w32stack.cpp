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

#include "core.h"
#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"

#include "execpt.h"
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

////////////////////////////////////////////////////////////////////////////////

// MW-2011-09-14: [[ Redraw ]] If non-nil, this pixmap is used in the next
//   onpaint update.
// IM-2013-06-19: [[ RefactorGraphics]] replace update pixmap with callback
static MCStackUpdateCallback s_update_callback = nil;
static void *s_update_context = nil;
static MCRectangle s_update_rect;

////////////////////////////////////////////////////////////////////////////////

extern bool create_temporary_dib(HDC p_dc, uint4 p_width, uint4 p_height, HBITMAP& r_bitmap, void*& r_bits);
extern void surface_merge_with_alpha(void *p_pixels, uint4 p_pixel_stride, void *p_alpha, uint4 p_alpha_stride, uint4 p_width, uint4 p_height);

////////////////////////////////////////////////////////////////////////////////

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

void MCStack::openwindow(Boolean p_override)
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

	if (m_window_shape != NULL && !m_window_shape -> is_sharp)
	{
		wstyle &= ~(WS_BORDER | WS_CAPTION | WS_DLGFRAME | WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_SIZEBOX | WS_SYSMENU | WS_THICKFRAME);
		exstyle &= ~(WS_EX_CLIENTEDGE | WS_EX_DLGMODALFRAME | WS_EX_STATICEDGE | WS_EX_WINDOWEDGE);
		wstyle |= WS_POPUP;
		exstyle |= WS_EX_LAYERED;
	}
	else if (blendlevel < 100)
		exstyle |= WS_EX_LAYERED;
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

	if (m_window_shape != NULL && !m_window_shape -> is_sharp)
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

			if (!t_is_xp_menu && mode < WM_PULLDOWN)
				window -> handle . window = (MCSysWindowHandle)CreateWindowExW(t_ex_style, MC_WIN_CLASS_NAME_W, WideCString(getname_cstring()), t_style | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, t_rect . left, t_rect . top, t_rect . right - t_rect . left, t_rect . bottom - t_rect . top, NULL, NULL, MChInst, NULL);
			else
				window -> handle . window = (MCSysWindowHandle)CreateWindowExA(t_ex_style, t_is_xp_menu ? MC_MENU_WIN_CLASS_NAME : mode >= WM_PULLDOWN ? MC_POPUP_WIN_CLASS_NAME : MC_WIN_CLASS_NAME, getname_cstring(), t_style | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, t_rect . left, t_rect . top, t_rect . right - t_rect . left, t_rect . bottom - t_rect . top, NULL, NULL, MChInst, NULL);
			
			// MW-2010-10-22: [[ Bug 8151 ]] Make sure we update the title string.
			if (titlestring != nil)
				MCscreen -> setname(window, titlestring);

			SetWindowLongA((HWND)window->handle.window, GWL_USERDATA, mode);
			
			if (flags & F_DECORATIONS && !(decorations & WD_SHAPE) && !(decorations & WD_CLOSE))
				EnableMenuItem(GetSystemMenu((HWND)window->handle.window, False), SC_CLOSE, MF_BYCOMMAND | MF_GRAYED);
		
			if (m_window_shape != nil && m_window_shape -> is_sharp)
			{
				MCRegionRef t_region;
				t_region = (MCRegionRef)m_window_shape -> handle;
				MCRegionOffset(t_region, rect . x - t_rect . left, rect . y - t_rect . top);
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

			MCPlayer *t_player;
			for(t_player = MCplayers; t_player != NULL; t_player = t_player -> getnextplayer())
				if (t_player -> getstack() == this)
					t_player -> changewindow((MCSysWindowHandle)t_old_window);
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
		MCRegionDestroy((MCRegionRef)m_window_shape -> handle);
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

		if ( getextendedstate(ECS_FULLSCREEN) )
		{
			const MCDisplay *t_display;
			t_display = MCscreen -> getnearestdisplay(rect);
			MCRectangle t_workarea, t_viewport;
			t_workarea = t_display -> workarea;
			t_viewport = t_display -> viewport ;
			setrect(t_viewport);
		}

		wrect = getwrect(rect, wstyle, exstyle);
		LONG x = wrect.left;
		LONG y = wrect.top;
		LONG width = wrect.right - wrect.left;
		LONG height = wrect.bottom - wrect.top;
		if (flags & F_WM_PLACE && !(state & CS_BEEN_MOVED))
			x = CW_USEDEFAULT;
		window = new _Drawable;
		window->type = DC_WINDOW;
		window->handle.window = 0; // protect against creation callbacks

		// MW-2011-02-15: [[ Bug 9396 ]] Tooltips should have a shadow on Windows.
		Boolean isxpmenu = (mode == WM_PULLDOWN || mode == WM_POPUP || mode == WM_CASCADE || mode == WM_TOOLTIP) \
		                   && (MCcurtheme && MCcurtheme->getthemeid() == LF_NATIVEWIN);

		// MW-2012-09-07: [[ Bug 10368 ]] If the 'no-shadow' bit is set, then we don't
		//   want to use the menu class.
		if (getflag(F_DECORATIONS) && (decorations & WD_NOSHADOW) != 0)
			isxpmenu = False;

		HWND t_parenthwnd = NULL;

		// MW-2007-07-06: [[ Bug 3226 ]] If the platform is NT always create a Unicode window
		if ((MCruntimebehaviour & RTB_NO_UNICODE_WINDOWS) == 0 && !isxpmenu && mode < WM_PULLDOWN)
			window -> handle . window = (MCSysWindowHandle)CreateWindowExW(exstyle, MC_WIN_CLASS_NAME_W, WideCString(getname_cstring()),wstyle | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, x, y, width, height,
		                 t_parenthwnd, NULL, MChInst, NULL);
		else
			window->handle.window = (MCSysWindowHandle)CreateWindowExA(exstyle, isxpmenu? MC_MENU_WIN_CLASS_NAME: mode >= WM_PULLDOWN ? MC_POPUP_WIN_CLASS_NAME
		                 : MC_WIN_CLASS_NAME, getname_cstring(), wstyle | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, x, y, width, height,
		                 t_parenthwnd, NULL, MChInst, NULL);

		SetWindowLongA((HWND)window->handle.window, GWL_USERDATA, mode);
		
		if (flags & F_DECORATIONS && !(decorations & WD_SHAPE) && !(decorations & WD_CLOSE))
			EnableMenuItem(GetSystemMenu((HWND)window->handle.window, False), SC_CLOSE, MF_BYCOMMAND | MF_GRAYED);

		if (m_window_shape != nil && m_window_shape -> is_sharp)
		{
			MCRegionRef t_region;
			t_region = (MCRegionRef)m_window_shape -> handle;
			MCRegionOffset(t_region, rect . x - wrect . left, rect . y - wrect . top);
			MCRegionSetAsWindowShape(t_region, window->handle.window);

			// The window now owns the region.
			m_window_shape -> handle = nil;
		}

		if ((m_window_shape == NULL || m_window_shape -> is_sharp) && blendlevel != 100)
			setopacity(blendlevel * 255 / 100);
	}
	
	start_externals();
}

void MCStack::sethints()
{
	if (!opened || MCnoui || window == DNULL)
		return;
	if (flags & F_RESIZABLE)
	{
		rect.width = MCU_max(minwidth, rect.width);
		rect.width = MCU_min(maxwidth, rect.width);
		rect.height = MCU_max(minheight, rect.height);
		rect.height = MCU_min(maxheight, rect.height);
	}
	MCStack *sptr = MCdefaultstackptr == this ? MCtopstackptr : MCdefaultstackptr;
	if (sptr != NULL && sptr != this && sptr->getw() != DNULL
	        && GetWindowLongA((HWND)window->handle.window, 0) == 0)
		SetWindowLongA((HWND)window->handle.window, 0, (LONG)sptr->getw()->handle.window);
}

MCRectangle MCStack::device_getwindowrect() const
{
	if (window == DNULL)
		return rect;

	RECT wrect;
	GetWindowRect((HWND)window->handle.window, &wrect);
	MCRectangle t_rect;
	t_rect.x = wrect.left;
	t_rect.y = wrect.top;
	t_rect.width = wrect.right - wrect.left;
	t_rect.height = wrect.bottom - wrect.top;

	return t_rect;
}

static inline MCRectangle MCWinRectToMCRect(RECT p_rect)
{
	return MCRectangleMake(p_rect.left, p_rect.top, p_rect.right - p_rect.left, p_rect.bottom - p_rect.top);
}

void MCStack::setgeom()
{
	if (MCnoui || !opened)
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

	// IM-2013-08-08: [[ ResIndependence ]] Scale stack rect to device space
	MCRectangle t_device_rect;
	t_device_rect = MCGRectangleGetIntegerInterior(MCResUserToDeviceRect(rect));

	uint32_t wstyle, exstyle;
	getstyle(wstyle, exstyle);
	RECT newrect = getwrect(t_device_rect, wstyle, exstyle);
	RECT wrect;
	GetWindowRect((HWND)window->handle.window, &wrect);
	LONG cx = newrect.right - newrect.left;
	LONG cy = newrect.bottom - newrect.top;
	state &= ~CS_NEED_RESIZE;

	// IM-2013-08-08: [[ ResIndependence ]] Scale old window rect to user space for comparison
	MCRectangle t_old_user_rect;
	t_old_user_rect = MCGRectangleGetIntegerBounds(MCResDeviceToUserRect(MCWinRectToMCRect(wrect)));

	if (t_old_user_rect.x != rect.x || t_old_user_rect.y != rect.y
	        || t_old_user_rect.width != rect.width || t_old_user_rect.height != rect.height)
	{
		state |= CS_NO_CONFIG;
		MoveWindow((HWND)window->handle.window, newrect.left, newrect.top, cx, cy, True);
		if (t_old_user_rect.width != rect.width || t_old_user_rect.height != rect.height)
			resize(t_old_user_rect.width, t_old_user_rect.height);
		state &= ~CS_NO_CONFIG;
	}
}

void MCStack::constrain(intptr_t lp)
{
	uint32_t wstyle, exstyle;
	getstyle(wstyle, exstyle);
	RECT wrect = getwrect(rect, wstyle, exstyle);
	int4 dx = wrect.right - wrect.left - rect.width;
	int4 dy = wrect.bottom - wrect.top - rect.height;
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

		mmptr -> ptMaxSize . x = MCU_min(maxwidth + dx, t_workarea . width);
		mmptr -> ptMaxSize . y = MCU_min(maxheight + dy, t_workarea . height);
		mmptr -> ptMaxPosition . x = t_workarea . x - t_viewport . x;
		mmptr -> ptMaxPosition . y = t_workarea . y - t_viewport . y;
	}

	// MW-2007-07-27: In Windows 98 we need to clamp to 32767...
	
	mmptr -> ptMinTrackSize . x = minwidth + dx;
	mmptr -> ptMinTrackSize . y = minheight + dy;
	mmptr -> ptMaxTrackSize . x = MCU_min(32767, maxwidth + dx);
	mmptr -> ptMaxTrackSize . y = MCU_min(32767, maxheight + dy);

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
		droptarget = new CDropTarget;
		droptarget->setstack(this);
		CoLockObjectExternal(droptarget, TRUE, TRUE);
		RegisterDragDrop((HWND)window->handle.window, droptarget);
	}
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

	if (!MCnoui && window != DNULL)
	{
		RevokeDragDrop((HWND)window->handle.window);
		CoLockObjectExternal(droptarget, FALSE, TRUE);
		droptarget->setstack(NULL);
		delete droptarget;
		droptarget = nil;
	}

	destroywindowshape();
	MClockmessages = oldlock;

	unloadexternals();
}

void MCStack::updatemodifiedmark(void)
{
}

void MCStack::enablewindow(bool p_enable)
{
	EnableWindow((HWND)getw() -> handle . window, p_enable);
}

void MCStack::redrawicon(void)
{
}

////////////////////////////////////////////////////////////////////////////////

bool __MCApplyMaskCallback(void *p_context, const MCRectangle &p_rect)
{
	MCGContextRef t_context = static_cast<MCGContextRef>(p_context);

	MCGContextAddRectangle(t_context, MCRectangleToMCGRectangle(p_rect));
	MCGContextFill(t_context);

	return true;
}

bool MCWin32ApplyMaskToRasterRegion(MCGRaster &p_raster, uint32_t p_x_origin, uint32_t p_y_origin, const MCGRaster &p_mask, MCRegionRef p_region)
{
	// IM-2013-09-11: [[ ResIndependence ]] reduce mask params to single MCGRaster
	bool t_success;
	t_success = true;

	MCGContextRef t_gcontext;
	t_gcontext = nil;

	if (t_success)
		t_success = MCGContextCreateWithRaster(p_raster, t_gcontext);

	MCGImageRef t_mask_image;
	t_mask_image = nil;

	if (t_success)
		t_success = MCGImageCreateWithRasterNoCopy(p_mask, t_mask_image);

	if (t_success)
	{
		// IM-2013-09-11: [[ ResIndependence ]] Apply scaled mask to target raster.
		// Drawing the mask directly will not work as the effective shape of the drawing operation
		// will be defined by the opaque parts of the mask - areas outside will be unaffected. Instead we
		// draw a solid rectangle over the intended areas using the mask as a pattern.
		MCGFloat t_scale;
		t_scale = MCResGetDeviceScale();

		MCGContextSetFillPattern(t_gcontext, t_mask_image, MCGAffineTransformMakeScale(t_scale, t_scale), kMCGImageFilterNearest);
		MCGContextTranslateCTM(t_gcontext, -(MCGFloat)p_x_origin, -(MCGFloat)p_y_origin);
		MCGContextSetBlendMode(t_gcontext, kMCGBlendModeDestinationIn);

		t_success = MCRegionForEachRect(p_region, __MCApplyMaskCallback, t_gcontext);
	}

	MCGImageRelease(t_mask_image);

	MCGContextRelease(t_gcontext);

	return t_success;
}

////////////////////////////////////////////////////////////////////////////////

class MCWindowsStackSurface: public MCStackSurface
{
	MCStack *m_stack;
	MCRegionRef m_region;
	HDC m_dc;

	MCGRaster *m_mask;

	bool m_locked;
	MCGContextRef m_locked_context;

	MCRegionRef m_redraw_region;
	MCRectangle m_area;
	MCRectangle m_locked_area;
	HBITMAP m_bitmap;
	MCGRaster m_raster;

public:
	MCWindowsStackSurface(MCGRaster *p_mask, HRGN p_region, HDC p_dc)
	{
		m_region = (MCRegionRef)p_region;
		m_dc = p_dc;

		m_mask = p_mask;

		m_locked = false;
		m_locked_context = nil;
		m_bitmap = nil;
	}

	bool Lock(void)
	{
		if (m_bitmap != nil)
			return false;

		MCRectangle t_actual_area;
		t_actual_area = MCRegionGetBoundingBox(m_region);
		if (MCU_empty_rect(t_actual_area))
			return false;

		bool t_success = true;

		void *t_bits = nil;

		if (t_success)
			t_success = MCRegionCreate(m_redraw_region);

		if (t_success)
			t_success = create_temporary_dib(m_dc, t_actual_area . width, t_actual_area . height, m_bitmap, t_bits);

		if (t_success)
		{
			m_raster . format = kMCGRasterFormat_ARGB;
			m_raster . width = t_actual_area . width;
			m_raster . height = t_actual_area . height;
			m_raster . stride = t_actual_area . width * sizeof(uint32_t);
			m_raster . pixels = t_bits;

			m_area = t_actual_area;

			return true;
		}

		MCRegionDestroy(m_redraw_region);
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
				MCWin32ApplyMaskToRasterRegion(m_raster, m_area.x, m_area.y, *m_mask, m_redraw_region);

			HDC t_src_dc = ((MCScreenDC *)MCscreen) -> getsrchdc();

			HGDIOBJ t_old_object;
			HRGN t_old_clip;

			t_old_clip = CreateRectRgn(0, 0, 0, 0);
			GetClipRgn(m_dc, t_old_clip);
			t_old_object = SelectObject(t_src_dc, m_bitmap);
			SelectClipRgn(m_dc, (HRGN)m_redraw_region);

			BitBlt(m_dc, m_area . x, m_area . y, m_area . width, m_area . height, t_src_dc, 0, 0, SRCCOPY);

			SelectClipRgn(m_dc, t_old_clip);
			SelectObject(t_src_dc, t_old_object);

			DeleteObject(t_old_clip);
		}

		DeleteObject(m_redraw_region);
		DeleteObject(m_bitmap);
		m_bitmap = nil;
	}

	bool LockGraphics(MCRegionRef p_area, MCGContextRef& r_context)
	{
		MCGRaster t_raster;
		if (LockPixels(p_area, t_raster))
		{
			if (MCGContextCreateWithRaster(t_raster, m_locked_context))
			{
				// Set origin
				MCGContextTranslateCTM(m_locked_context, -m_locked_area.x, -m_locked_area.y);
				// Set clipping rect
				MCGContextClipToRect(m_locked_context, MCRectangleToMCGRectangle(m_locked_area));
				
				r_context = m_locked_context;
				
				return true;
			}

			UnlockPixels();
		}
		
		return false;
	}
	
	void UnlockGraphics(void)
	{
		if (m_locked_context == nil)
			return;

		MCGContextRelease(m_locked_context);
		m_locked_context = nil;
		
		UnlockPixels();
	}
	
	bool LockPixels(MCRegionRef p_area, MCGRaster& r_raster)
	{
		if (m_bitmap == nil || m_locked)
			return false;

		MCRectangle t_bounds = MCRegionGetBoundingBox(m_region);
		MCRectangle t_actual_area;
		t_actual_area = MCU_intersect_rect(MCRegionGetBoundingBox(p_area), t_bounds);
		if (MCU_empty_rect(t_actual_area))
			return false;

		/* UNCHECKED */ MCRegionIncludeRect(m_redraw_region, t_actual_area);

		uint8_t *t_bits = (uint8_t*)m_raster.pixels + (t_actual_area.y - t_bounds.y) * m_raster.stride + (t_actual_area.x - t_bounds.x) * sizeof(uint32_t);

		m_locked_area = t_actual_area;

		r_raster . format = kMCGRasterFormat_ARGB;
		r_raster . width = t_actual_area . width;
		r_raster . height = t_actual_area . height;
		r_raster . stride = m_raster.stride;
		r_raster . pixels = t_bits;

		m_locked = true;

		return true;
	}
	
	void UnlockPixels(void)
	{
		m_locked = false;
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

		MCGContextRef t_context = nil;
		MCRegionRef t_region = nil;

		t_success = MCRegionCreate(t_region);

		if (t_success)
			t_success = MCRegionSetRect(t_region, MCGRectangleGetIntegerBounds(p_dst_rect));

		if (t_success)
			t_success = LockGraphics(t_region, t_context);

		if (t_success)
		{
			MCGContextDrawRectOfImage(t_context, p_src, p_src_rect, p_dst_rect, kMCGImageFilterNearest);
		}

		UnlockGraphics();

		MCRegionDestroy(t_region);

		return t_success;
	}
};

class MCWindowsLayeredStackSurface: public MCStackSurface
{
	MCGRaster m_raster;
	MCGRaster *m_mask;
	MCRegionRef m_redraw_region;

	bool m_locked;

	MCRectangle m_locked_area;
	MCGContextRef m_locked_context;
	void *m_locked_bits;
	uint32_t m_locked_stride;

public:
	MCWindowsLayeredStackSurface(MCGRaster p_raster, MCGRaster *p_mask)
	{
		m_raster = p_raster;
		m_mask = p_mask;
		m_redraw_region = nil;

		m_locked = false;

		m_locked_context = nil;
		m_locked_bits = nil;
		m_locked_stride = 0;
	}

	bool Lock()
	{
		if (m_locked)
			return false;

		if (!MCRegionCreate(m_redraw_region))
			return false;

		m_locked = true;

		return true;
	}

	void Unlock(void)
	{
		if (!m_locked)
			return;

		Unlock(true);

		MCRegionDestroy(m_redraw_region);
		m_redraw_region = nil;
		m_locked = false;
	}

	void Unlock(bool p_update)
	{
		if (p_update)
		{
			void *t_src_ptr, *t_dst_ptr;
			MCWin32ApplyMaskToRasterRegion(m_raster, 0, 0, *m_mask, m_redraw_region);
		}
	}

	bool LockGraphics(MCRegionRef p_area, MCGContextRef& r_context)
	{
		MCGRaster t_raster;
		if (LockPixels(p_area, t_raster))
		{
			if (MCGContextCreateWithRaster(t_raster, m_locked_context))
			{
				// Set origin
				MCGContextTranslateCTM(m_locked_context, -m_locked_area.x, -m_locked_area.y);
				// Set clipping rect
				MCGContextClipToRect(m_locked_context, MCRectangleToMCGRectangle(m_locked_area));
				
				r_context = m_locked_context;
				
				return true;
			}

			UnlockPixels();
		}

		return false;
	}
	
	void UnlockGraphics(void)
	{
		if (m_locked_context == nil)
			return;
		
		MCGContextRelease(m_locked_context);
		m_locked_context = nil;

		UnlockPixels();
	}
	
	bool LockPixels(MCRegionRef p_area, MCGRaster &r_raster)
	{
		m_locked_bits = m_raster.pixels;
		m_locked_stride = m_raster.stride;

		m_locked_area = MCRegionGetBoundingBox(p_area);
		// restrict locked area to intersection with raster
		m_locked_area = MCU_intersect_rect(m_locked_area, MCU_make_rect(0, 0, m_raster.width, m_raster.height));

		/* UNCHECKED */ MCRegionIncludeRect(m_redraw_region, m_locked_area);

		r_raster.width = m_locked_area.width;
		r_raster.height = m_locked_area.height;
		r_raster.pixels = (uint8_t *)m_locked_bits + m_locked_area . y * m_locked_stride + m_locked_area . x * sizeof(uint32_t);
		r_raster.stride = m_locked_stride;
		r_raster.format = m_raster.format;
		return true;
	}
	
	void UnlockPixels(void)
	{
		m_locked_bits = nil;
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

		MCGContextRef t_context = nil;
		MCRegionRef t_region = nil;

		t_success = MCRegionCreate(t_region);

		if (t_success)
			t_success = MCRegionSetRect(t_region, MCGRectangleGetIntegerBounds(p_dst_rect));

		if (t_success)
			t_success = LockGraphics(t_region, t_context);

		if (t_success)
		{
			MCGContextDrawRectOfImage(t_context, p_src, p_src_rect, p_dst_rect, kMCGImageFilterNearest);
		}

		UnlockGraphics();

		MCRegionDestroy(t_region);

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

void MCStack::device_updatewindow(MCRegionRef p_region)
{
	if (m_window_shape == nil || m_window_shape -> is_sharp)
	{
		InvalidateRgn((HWND)window -> handle . window, (HRGN)p_region, FALSE);
		UpdateWindow((HWND)window -> handle . window);
	}
	else
	{
		MCRectangle t_device_rect;
		t_device_rect = MCGRectangleGetIntegerBounds(MCResUserToDeviceRect(MCRectangleMake(0, 0, m_window_shape->width, m_window_shape->height)));

		HBITMAP t_bitmap = nil;
		void *t_bits = nil;

		if (m_window_shape -> handle == nil)
		{
			if (!create_temporary_dib(((MCScreenDC*)MCscreen)->getdsthdc(), t_device_rect.width, t_device_rect.height, t_bitmap, t_bits))
				return;

			m_window_shape -> handle = t_bitmap;
		}
		else
		{
			t_bitmap = (HBITMAP)m_window_shape -> handle;

			BITMAP t_bitmap_struct;
			GetObjectA(t_bitmap, sizeof(BITMAP), &t_bitmap_struct);
			t_bits = t_bitmap_struct.bmBits;
		}

		MCGRaster t_raster;
		t_raster.width = t_device_rect.width;
		t_raster.height = t_device_rect.height;
		t_raster.pixels = t_bits;
		t_raster.stride = t_raster.width * sizeof(uint32_t);
		t_raster.format = kMCGRasterFormat_ARGB;

		MCGRaster t_mask;
		/* UNCHECKED */ MCWin32GetWindowShapeAlphaMask(m_window_shape, t_mask);

		MCWindowsLayeredStackSurface t_surface(t_raster, &t_mask);

		if (t_surface.Lock())
		{
			if (s_update_callback == nil)
				device_redrawwindow(&t_surface, (MCRegionRef)p_region);
			else
				s_update_callback(&t_surface, (MCRegionRef)p_region, s_update_context);

			t_surface.Unlock();

			composite();
		}
	}
}

void MCStack::device_updatewindowwithcallback(MCRegionRef p_region, MCStackUpdateCallback p_callback, void *p_context)
{
	s_update_callback = p_callback;
	s_update_context = p_context;
	s_update_rect = MCRegionGetBoundingBox(p_region);
	device_updatewindow(p_region);
	s_update_callback = nil;
	s_update_context = nil;
}

void MCStack::onpaint(void)
{
	HRGN t_update_region;
	t_update_region = CreateRectRgn(0, 0, 0, 0);
	GetUpdateRgn((HWND)window -> handle . window, t_update_region, FALSE);

	PAINTSTRUCT t_paint;
	BeginPaint((HWND)window -> handle . window, &t_paint);

	MCGRaster t_mask;
	MCGRaster *t_mask_ptr;

	if (MCWin32GetWindowShapeAlphaMask(getwindowshape(), t_mask))
		t_mask_ptr = &t_mask;
	else
		t_mask_ptr = nil;

	MCWindowsStackSurface t_surface(t_mask_ptr, t_update_region, t_paint . hdc);

	if (t_surface.Lock())
	{
		if (s_update_callback == nil)
			device_redrawwindow(&t_surface, (MCRegionRef)t_update_region);
		else
			s_update_callback(&t_surface, (MCRegionRef)t_update_region, s_update_context);

		t_surface.Unlock();
	}

	EndPaint((HWND)window -> handle . window, &t_paint);

	DeleteObject(t_update_region);
}

void MCStack::composite(void)
{
	if (m_window_shape == nil || m_window_shape -> is_sharp || m_window_shape -> handle == nil)
		return;

	POINT t_offset;
	POINT t_location;
	SIZE t_size;

	HDC t_dst_dc;
	t_dst_dc = ((MCScreenDC *)MCscreen) -> getdsthdc();

	HGDIOBJ t_old_dst;
	HBITMAP t_bitmap;
	t_bitmap = (HBITMAP)m_window_shape -> handle;
	t_old_dst = SelectObject(t_dst_dc, t_bitmap);

	MCRectangle t_device_stack_rect;
	t_device_stack_rect = MCGRectangleGetIntegerInterior(MCResUserToDeviceRect(rect));

	MCRectangle t_device_shape_rect;
	t_device_shape_rect = MCGRectangleGetIntegerBounds(MCResUserToDeviceRect(MCRectangleMake(0, 0, m_window_shape->width, m_window_shape->height)));

	t_offset . x = 0;
	t_offset . y = 0;
	t_location . x = t_device_stack_rect . x;
	t_location . y = t_device_stack_rect . y;
	t_size . cx = t_device_shape_rect . width;;
	t_size . cy = t_device_shape_rect . height;

	BLENDFUNCTION t_blend;
	t_blend . BlendOp = AC_SRC_OVER;
	t_blend . BlendFlags = 0;
	t_blend . SourceConstantAlpha = blendlevel * 255 / 100;
	t_blend . AlphaFormat = AC_SRC_ALPHA;

	UpdateLayeredWindow((HWND)window -> handle . window, t_dst_dc, &t_location, &t_size, t_dst_dc, &t_offset, 0, &t_blend, ULW_ALPHA);

	SelectObject(t_dst_dc, t_old_dst);
}

////////////////////////////////////////////////////////////////////////////////
