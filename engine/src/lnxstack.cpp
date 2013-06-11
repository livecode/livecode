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

//
// platform-specific MCStack class functions
//
#include "lnxprefix.h"

#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"

#include "execpt.h"
#include "dispatch.h"
#include "stack.h"
#include "card.h"
#include "group.h"
#include "image.h"
#include "field.h"
#include "stacklst.h"
#include "cardlst.h"
#include "sellst.h"
#include "handler.h"
#include "mcerror.h"
#include "param.h"
#include "util.h"
#include "debug.h"
#include "mode.h"
#include "player.h"
#include "globals.h"
#include "region.h"
#include "redraw.h"

#include "lnxdc.h"
#include "graphicscontext.h"

static uint2 calldepth;
static uint2 nwait;

////////////////////////////////////////////////////////////////////////////////

extern void surface_merge_with_alpha(void *p_pixels, uint4 p_pixel_stride, void *p_alpha, uint4 p_alpha_stride, uint4 p_width, uint4 p_height);

////////////////////////////////////////////////////////////////////////////////

// MW-2011-09-14: [[ Redraw ]] If non-nil, this pixmap is used in the next
//   onpaint update.
static Pixmap s_update_pixmap = nil;
static MCRectangle s_update_rect;

////////////////////////////////////////////////////////////////////////////////

MCStack *MCStack::findstackd(Window w)
{
	if (w == DNULL)
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
	if (pwindow != DNULL && w == pwindow)
		if  (++ccount == cindex)
			return this;
	if (substacks != NULL)
	{
		MCStack *tptr = substacks;
		do
		{
			pwindow = tptr->getparentwindow();
			if (pwindow != DNULL && w == pwindow)
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

//XDND
void xdnd_make_window_aware ( Window w ) ;

void MCStack::realize()
{
	if (MCnoui)
	{
		start_externals();
		return;
	}

	if (MCModeMakeLocalWindows())
	{
		MCScreenDC *screen = (MCScreenDC *)MCscreen;
		XSetWindowAttributes xswa;
		unsigned long xswamask = CWBorderPixel | CWColormap;
		xswa.border_pixel = 0;
		xswa.colormap = screen->getcmap();

		if (rect.width == 0)
			rect.width = MCminsize << 4;
		if (rect.height == 0)
			rect.height = MCminsize << 3;
		
		if ( getextendedstate(ECS_FULLSCREEN) )
		{
			const MCDisplay *t_display;
			t_display = MCscreen -> getnearestdisplay(rect);
			MCRectangle t_workarea, t_viewport;
			t_workarea = t_display -> workarea;
			setrect(t_viewport);
		}

		window = XCreateWindow(MCdpy, screen->getroot(), rect.x, rect.y,
							   rect.width, rect.height,
							   0, screen->getrealdepth(), InputOutput,
							   screen->getvisual(), xswamask, &xswa);
		
		//XDND
		xdnd_make_window_aware ( window ) ;
		if ( screen -> get_backdrop() != DNULL)
			XSetTransientForHint(MCdpy, window, screen-> get_backdrop());

		
		XSelectInput(MCdpy, window,  ButtonPressMask | ButtonReleaseMask
					 | EnterWindowMask | LeaveWindowMask | PointerMotionMask
					 | KeyPressMask | KeyReleaseMask | ExposureMask
					 | FocusChangeMask | StructureNotifyMask | PropertyChangeMask);
		loadwindowshape();
		if (m_window_shape != nil && m_window_shape -> is_sharp)
			XShapeCombineMask(MCdpy,window, ShapeBounding, 0, 0, (Pixmap)m_window_shape -> handle, ShapeSet);

		XSync(MCdpy, False);
	}

	start_externals();
}

static void wmspec_change_state(Window p_window, Atom p_first_atom, Atom p_second_atom, bool p_add)
{
	XClientMessageEvent xclient;
	
#define _NET_WM_STATE_REMOVE        0    /* remove/unset property */
#define _NET_WM_STATE_ADD           1    /* add/set property */
#define _NET_WM_STATE_TOGGLE        2    /* toggle property  */  
	
	memset (&xclient, 0, sizeof (xclient));
	xclient.type = ClientMessage;
	xclient.window = p_window;
	xclient.message_type = XInternAtom(MCdpy, "_NET_WM_STATE", True);
	xclient.format = 32;
	xclient.data.l[0] = p_add ? _NET_WM_STATE_ADD : _NET_WM_STATE_REMOVE;
	xclient.data.l[1] = p_first_atom;
	xclient.data.l[2] = p_second_atom;
	xclient.data.l[3] = 0;
	xclient.data.l[4] = 0;
	
	XSendEvent (MCdpy, MCscreen -> getroot(), False,
				SubstructureRedirectMask | SubstructureNotifyMask,
				(XEvent *)&xclient);
}

void MCStack::setmodalhints()
{
	if (mode == WM_MODAL || mode == WM_SHEET)
	{
		wmspec_change_state(window, XInternAtom(MCdpy, "_NET_WM_STATE_MODAL", True), None, true);
		XSetTransientForHint(MCdpy, window, mode == WM_SHEET ? parentwindow : None);
	}
}

void MCStack::sethints()
{
	if (!opened || MCnoui || window == DNULL)
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
	if (opened)
	{
		XSizeHints hints;
		if (flags & F_RESIZABLE )
		{
			hints.min_width = minwidth;
			hints.min_height = minheight;
			hints.max_width = MCU_min(maxwidth, MCscreen->getwidth());
			hints.max_height = MCU_min(maxheight, MCscreen->getheight());
		}
		else
		{
			hints.min_width = hints.max_width = rect.width;
			hints.min_height = hints.max_height = rect.height;
		}
		hints.width_inc = hints.height_inc = 1;
		hints.win_gravity = StaticGravity;
		hints.flags = PMaxSize | PMinSize | PResizeInc | PWinGravity;
		XSetWMNormalHints(MCdpy, window, &hints);
	}

	XSetWindowAttributes xswa;
	unsigned long xswamask = CWOverrideRedirect | CWSaveUnder;
	xswa.override_redirect = xswa.save_under = mode >= WM_PULLDOWN && mode <= WM_LICENSE;
	XChangeWindowAttributes(MCdpy, window, xswamask, &xswa);

	XWMHints whints;
	memset(&whints, 0, sizeof(XWMHints));
	whints.flags = InputHint | StateHint;
	whints.input = MCpointerfocus;
	whints.initial_state = flags & F_START_UP_ICONIC ? IconicState:NormalState;
	whints.flags |= WindowGroupHint;
	whints.window_group = ((MCScreenDC *)MCscreen) -> GetNullWindow();
	
	XSetWMHints(MCdpy, window, &whints);

	XClassHint chints;
	chints.res_name = (char *)getname_cstring();

	chints.res_class = (char *)MCapplicationstring;
	XSetClassHint(MCdpy, window, &chints);

	Atom protocols[3];
	protocols[0] = MCdeletewindowatom;
	if (MCpointerfocus)
		XChangeProperty(MCdpy, window, MCprotocolatom, XA_ATOM, 32,
		                PropModeReplace, (unsigned char *)protocols, 1);
	else
	{
		protocols[1] = MCmwmmessageatom;
		protocols[2] = MCtakefocusatom;
		XChangeProperty(MCdpy, window, MCprotocolatom, XA_ATOM, 32,
		                PropModeReplace, (unsigned char *)protocols, 3);
	}
	if (mode >= WM_PALETTE)
	{
		uint4 data = 5;
		XChangeProperty(MCdpy, window, MClayeratom, XA_CARDINAL, 32,
		                PropModeReplace, (unsigned char *)&data, 1);
	}
	
	
	MwmHints mwmhints;
	Atom OLAhints[3];
	Atom OLDhints[3];
	uint2 nOLAhints = 0;
	uint2 nOLDhints = 0;
	mwmhints.flags = MWM_HINTS_FUNCTIONS | MWM_HINTS_DECORATIONS | MWM_HINTS_INPUT_MODE; 
	if (flags & F_RESIZABLE ) // && mode != WM_PALETTE)
	{
		mwmhints.decorations
		= MWM_DECOR_TITLE | MWM_DECOR_RESIZEH | MWM_DECOR_MAXIMIZE;
		mwmhints.functions = MWM_FUNC_RESIZE | MWM_FUNC_MOVE
		                     | MWM_FUNC_MAXIMIZE | MWM_FUNC_CLOSE;
		OLAhints[nOLAhints++] = MColresizeatom;
	}

	else
	{
		mwmhints.decorations =  MWM_DECOR_TITLE | MWM_DECOR_BORDER;
		mwmhints.functions = MWM_FUNC_MOVE | MWM_FUNC_MINIMIZE | MWM_FUNC_CLOSE;
		OLDhints[nOLDhints++] = MColresizeatom;
	}

	switch (mode)
	{
	case WM_TOP_LEVEL:
	case WM_TOP_LEVEL_LOCKED:
	case WM_MODELESS:
	case WM_PALETTE:
	case WM_DRAWER:
		mwmhints.decorations |= MWM_DECOR_MENU;
		if (mode != WM_PALETTE && rect.width > DECORATION_MINIMIZE_WIDTH)
		//if (rect.width > DECORATION_MINIMIZE_WIDTH)
		{
			mwmhints.decorations |= MWM_DECOR_MINIMIZE;
			mwmhints.functions |= MWM_FUNC_MINIMIZE;
		}
		mwmhints.input_mode = MWM_INPUT_MODELESS;
		OLAhints[nOLAhints++] = MColheaderatom;
		OLAhints[nOLAhints++] = MColcloseatom;
		break;
	case WM_LICENSE:
		mwmhints.flags = MWM_HINTS_DECORATIONS | MWM_HINTS_INPUT_MODE;
		mwmhints.input_mode = MWM_INPUT_SYSTEM_MODAL;
		OLDhints[nOLDhints++] = MColheaderatom;
		OLDhints[nOLDhints++] = MColcloseatom;
		break;
	default:
		OLAhints[nOLAhints++] = MColheaderatom;
		OLDhints[nOLDhints++] = MColcloseatom;
		mwmhints.input_mode = MWM_INPUT_FULL_APPLICATION_MODAL;
		break;
	}
	if (flags & F_DECORATIONS)
	{
		
		nOLAhints = 0;
		nOLDhints = 0;
		mwmhints.decorations = 0;
		mwmhints.functions = 0 ;
		
		if ( ( decorations & ( WD_TITLE | WD_MENU | WD_MINIMIZE | WD_MAXIMIZE | WD_CLOSE ) )  && flags & F_RESIZABLE ) //&& mode != WM_PALETTE)
		{
			mwmhints.decorations |= MWM_DECOR_RESIZEH;
			//TS-2007-08-20 changed to |= from = so that we ALWAYS have the MOVE functionality
			mwmhints.functions |= MWM_FUNC_RESIZE;
			OLAhints[nOLAhints++] = MColresizeatom;
		}
		else
			OLDhints[nOLDhints++] = MColresizeatom;
		if (decorations & WD_TITLE)
		{
			mwmhints.decorations |= MWM_DECOR_TITLE | MWM_DECOR_BORDER;
			OLAhints[nOLAhints++] = MColheaderatom;
		}
		else
			OLDhints[nOLDhints++] = MColheaderatom;
		if (decorations & WD_MENU)
		{
			mwmhints.decorations |= MWM_DECOR_MENU;
			OLAhints[nOLAhints++] = MColcloseatom;
		}
		else
			OLDhints[nOLDhints++] = MColcloseatom;
		if (decorations & WD_MINIMIZE)
		{
			mwmhints.decorations |= MWM_DECOR_MINIMIZE;
			mwmhints.functions |= MWM_FUNC_MINIMIZE;
		}
		if (decorations & WD_MAXIMIZE)
		{
			mwmhints.decorations |= MWM_DECOR_MAXIMIZE;
			mwmhints.functions |= MWM_FUNC_MAXIMIZE;
		}
		
		//TS-2007-08-20 Added handler for WD_CLOSE
		if (decorations & WD_CLOSE)
		{
			mwmhints.functions |= MWM_FUNC_CLOSE;
		}
		
		
		if ( decorations != 0 ) 
			mwmhints.functions |= MWM_FUNC_MOVE ;

		
		//TS 
		if ( decorations & WD_SHAPE )
		{
			mwmhints.decorations = 0 ;
		}
		
	}
	// Gnome gets confused with these set
	if (flags & F_DECORATIONS)
		XChangeProperty(MCdpy, window, MCmwmhintsatom, MCmwmhintsatom, 32,
		                PropModeReplace, (unsigned char *)&mwmhints,
		                sizeof(mwmhints) / 4);
	
	Atom t_type, t_control ;
	t_control = XInternAtom(MCdpy, "_NET_WM_STATE", false);
	
	// Use the window manager to set to full screen.
	if ( getextendedstate(ECS_FULLSCREEN) )
	{
		t_type = XInternAtom ( MCdpy, "_NET_WM_STATE_FULLSCREEN", false );
		XChangeProperty(MCdpy, window, t_control, XA_ATOM, 32, PropModeReplace, (unsigned char*)&t_type, 1);
		XUnmapWindow(MCdpy, window );
		XMapWindow(MCdpy, window );
	}

	//TS 2007-11-08 : Adding in additional hint _NET_WM_STATE == _NET_WM_STATE_ABOVE if we have set WD_UTILITY (i.e. systemwindow == true)
	if ( decorations & WD_UTILITY)
	{
		t_type = 	XInternAtom(MCdpy, "_NET_WM_STATE_ABOVE", false);	
		XChangeProperty(MCdpy, window, t_control, XA_ATOM, 32, PropModeReplace, (unsigned char*)&t_type, 1);
	}
	
	if (mode == WM_SHEET || mode == WM_MODAL)
	{
		Atom t_type;
		t_type = XInternAtom(MCdpy, "_NET_WM_WINDOW_TYPE_DIALOG", True);
		XChangeProperty(MCdpy, window, XInternAtom(MCdpy, "_NET_WM_WINDOW_TYPE", True), XA_ATOM, 32, PropModeReplace, (unsigned char*)&t_type, 1);
	}

	
#ifdef OLWM

	if (mode != WM_MODAL && mode != WM_SHEET)
		XChangeProperty(MCdpy, window, MColwinatom, XA_ATOM,
		                32, PropModeReplace, (unsigned char*)&MColwtotheratom, 1);
	else
		XChangeProperty(MCdpy, window, MColwinatom, XA_ATOM,
		                32, PropModeReplace, (unsigned char*)&MColwtpropatom, 1);
	XChangeProperty(MCdpy, window, MColddecoratom, XA_ATOM,
	                32, PropModeReplace, (unsigned char*)OLDhints, nOLDhints);
	XChangeProperty(MCdpy, window, MColadecoratom, XA_ATOM,
	                32, PropModeReplace, (unsigned char*)OLAhints, nOLAhints);
#endif
}

void MCStack::destroywindowshape()
{
	if (m_window_shape == nil)
		return;

	// Delete the data ptr (might be null).
	delete[] m_window_shape -> data;

	// If the mask is sharp, then 'handle' is a Pixmap used to set the window
	// shape. Otherwise it is nil.
	if (m_window_shape -> is_sharp)
	{
		Pixmap t_pixmap;
		t_pixmap = (Pixmap)m_window_shape -> handle;
		if (t_pixmap != nil)
			((MCScreenDC*)MCscreen) -> freepixmap(t_pixmap);
	}

	delete m_window_shape;
	m_window_shape = nil;
}

MCRectangle MCStack::getwindowrect(void) const
{
	if (window == DNULL)
		return rect;

	Window t_root, t_child, t_parent;
	Window *t_children;
	int32_t t_win_x, t_win_y, t_x_offset, t_y_offset;
	uint32_t t_width, t_height, t_border_width, t_depth, t_child_count;

	Window t_window = window;

	XQueryTree(MCdpy, t_window, &t_root, &t_parent, &t_children, &t_child_count);
	XFree(t_children);
	while (t_parent != t_root)
	{
		t_window = t_parent;
		XQueryTree(MCdpy, t_window, &t_root, &t_parent, &t_children, &t_child_count);
		XFree(t_children);
	}

	XGetGeometry(MCdpy, t_window, &t_root, &t_win_x, &t_win_y, &t_width, &t_height, &t_border_width, &t_depth);
	XTranslateCoordinates(MCdpy, t_window, t_root, 0, 0, &t_win_x, &t_win_y, &t_child);

	MCRectangle t_rect;
	t_rect.x = t_win_x - t_border_width;
	t_rect.y = t_win_y - t_border_width;
	t_rect.width = t_width + t_border_width * 2;
	t_rect.height = t_height + t_border_width * 2;

	return t_rect;
}

void MCStack::setgeom()
{
	if (MCnoui || !opened)
		return;
	
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

	Window t_root, t_child;
	int t_win_x, t_win_y;
	unsigned int t_width, t_height, t_border_width, t_depth;
	
	XGetGeometry(MCdpy, window, &t_root, &t_win_x, &t_win_y, &t_width, &t_height, &t_border_width, &t_depth);

	XTranslateCoordinates(MCdpy, window, t_root, 0, 0, &t_win_x, &t_win_y, &t_child);
	
	if (!(flags & F_WM_PLACE) || state & CS_BEEN_MOVED)
	{
		XSizeHints hints;
		hints.x = rect.x;
		hints.y = rect.y;
		hints.width = rect.width;
		hints.height = rect.height;
		if (flags & F_RESIZABLE )
		{
			hints.min_width = minwidth;
			hints.min_height = minheight;
			hints.max_width = MCU_min(maxwidth, MCscreen->getwidth());
			hints.max_height = MCU_min(maxheight, MCscreen->getheight());
		}
		else
		{
			hints.min_width = hints.max_width = rect.width;
			hints.min_height = hints.max_height = rect.height;
		}
		hints.width_inc = hints.height_inc = 1;
		hints.win_gravity = StaticGravity;
		hints.flags = USSize | PMaxSize | PMinSize | PResizeInc | PWinGravity;
		if (!(flags & F_WM_PLACE) || state & CS_BEEN_MOVED)
			hints.flags |= USPosition;
		XSetWMNormalHints(MCdpy, window, &hints);
	}
	
	
	state &= ~CS_NEED_RESIZE;
	if ((!(flags & F_WM_PLACE) || state & CS_BEEN_MOVED)
	        && (t_win_x != rect.x || t_win_y != rect.y))
	{
		if (t_width != rect.width || t_height != rect.height)
		{
			XMoveResizeWindow(MCdpy, window, rect.x, rect.y, rect.width, rect.height);
			resize(t_width, t_height);
		}
		else
			XMoveWindow(MCdpy, window, rect.x, rect.y);
	}
	else
		if (t_width != rect.width || t_height != rect.height)
		{
			XResizeWindow(MCdpy, window, rect.width, rect.height);
			resize(t_width, t_height);
		}
		
	state &= ~CS_ISOPENING;
}

 void MCStack::start_externals()
 {
		loadexternals();
 }

 void MCStack::stop_externals()
 {
	 destroywindowshape();
	 unloadexternals();
}
 
void MCStack::openwindow(Boolean override)
{
	if (MCModeMakeLocalWindows())
	{
		// MW-2010-11-29: Make sure we reset the geometry on the window before
		//   it gets mapped - otherwise we will get upward drift due to StaticGravity
		//   being used.
		setgeom();
		MCscreen -> openwindow(window, override);
		setmodalhints();
	}
}

void MCStack::setopacity(unsigned char p_level)
{
	// If the stack is not ours to open, then we do nothing ('runtime' mode/remoteable
	// window).
	if (!MCModeMakeLocalWindows())
		return;

	double op ;
	op = (double)p_level / 255 ;
	uint4 cardinal;
	cardinal = op * 0xffffffff ;

	Atom t_atom = XInternAtom ( MCdpy, "_NET_WM_WINDOW_OPACITY", False );
	XChangeProperty (MCdpy, window, t_atom, XA_CARDINAL, 32, PropModeReplace, ( unsigned char*) &cardinal, 1);
}

void MCStack::updatemodifiedmark(void)
{
}

void MCStack::redrawicon(void)
{
}

void MCStack::enablewindow(bool p_enable)
{
	long t_event_mask;
	if (p_enable)
		t_event_mask = ButtonPressMask | ButtonReleaseMask
		| EnterWindowMask | LeaveWindowMask | PointerMotionMask
		| KeyPressMask | KeyReleaseMask | ExposureMask
		| FocusChangeMask | StructureNotifyMask | PropertyChangeMask;
	else
		t_event_mask = ExposureMask | StructureNotifyMask | PropertyChangeMask;
	
	XSelectInput(MCdpy, window, t_event_mask);
}

void MCStack::applyscroll(void)
{
}

void MCStack::clearscroll(void)
{
}

////////////////////////////////////////////////////////////////////////////////

void MCBitmapClearRegion(MCBitmap *p_image, int32_t p_x, int32_t p_y, uint32_t p_width, uint32_t p_height)
{
	uint8_t *t_dst_row = (uint8_t*)p_image->data + p_y * p_image->bytes_per_line + p_x * sizeof(uint32_t);
	for (uint32_t y = 0; y < p_height; y++)
	{
		MCMemoryClear(t_dst_row, p_width * sizeof(uint32_t));
		t_dst_row += p_image->bytes_per_line;
	}
}

////////////////////////////////////////////////////////////////////////////////

class MCLinuxStackSurface: public MCStackSurface
{
	MCStack *m_stack;
	MCRegionRef m_region;

	MCRectangle m_locked_area;
	MCContext *m_locked_context;
	MCBitmap *m_locked_bitmap;
	void *m_locked_bits;
	uint32_t m_locked_stride;

public:
	MCLinuxStackSurface(MCStack *p_stack, MCRegionRef p_region)
	{
		m_stack = p_stack;
		m_region = p_region;

		m_locked_context = nil;
		m_locked_bitmap = nil;
	}

	bool LockGraphics(MCRegionRef p_area, MCContext*& r_context)
	{
		MCGRaster t_raster;
		if (LockPixels(p_area, t_raster))
		{
			m_locked_context = new MCGraphicsContext(t_raster.width, t_raster.height, t_raster.stride, t_raster.pixels, true);
			if (m_locked_context != nil)
			{
				m_locked_context->setorigin(m_locked_area.x, m_locked_area.y);
				m_locked_context->setclip(m_locked_area);
				
				r_context = m_locked_context;
				
				return true;
			}
			
			UnlockPixels(false);
		}
		
		return false;
	}

	void UnlockGraphics(void)
	{
		if (m_locked_context == nil)
			return;
		
		delete m_locked_context;
		m_locked_context = nil;
		
		UnlockPixels(true);
	}

	bool LockPixels(MCRegionRef p_area, MCGRaster &r_raster)
	{
		MCRectangle t_actual_area;
		t_actual_area = MCU_intersect_rect(MCRegionGetBoundingBox(p_area), MCRegionGetBoundingBox(m_region));
		if (MCU_empty_rect(t_actual_area))
			return false;
		
		m_locked_bitmap = ((MCScreenDC*)MCscreen)->createimage(32, t_actual_area.width, t_actual_area.height, False, 0x0, False, False);
		if (m_locked_bitmap == nil)
			return false;
		
		m_locked_area = t_actual_area;
		m_locked_bits = m_locked_bitmap->data;
		m_locked_stride = m_locked_bitmap->bytes_per_line;
		
		r_raster . format = kMCGRasterFormat_ARGB;
		r_raster . width = t_actual_area . width;
		r_raster . height = t_actual_area . height;
		r_raster . stride = m_locked_stride;
		r_raster . pixels = m_locked_bits;
		
		return true;
	}

	void UnlockPixels(void)
	{
		UnlockPixels(true);
	}
	
	void UnlockPixels(bool p_update)
	{
		if (m_locked_bitmap == nil)
			return;
		
		if (p_update)
		{
			MCWindowShape *t_mask;
			t_mask = m_stack -> getwindowshape();
			if (t_mask != nil && !t_mask -> is_sharp)
			{
				if (m_locked_area.x + m_locked_area.width > t_mask->width)
					MCBitmapClearRegion(m_locked_bitmap, t_mask->width, 0, m_locked_area.x + m_locked_area.width - t_mask->width, m_locked_area.height);
				if (m_locked_area.y + m_locked_area.height > t_mask->height)
					MCBitmapClearRegion(m_locked_bitmap, 0, t_mask->height, m_locked_area.width, m_locked_area.y + m_locked_area.height - t_mask->height);
					
				uint32_t t_width = 0;
				uint32_t t_height = 0;
				if (t_mask->width > m_locked_area.x)
					t_width = MCMin(t_mask->width - m_locked_area.x, m_locked_area.width);
				if (t_mask->height > m_locked_area.y)
					t_height = MCMin(t_mask->height - m_locked_area.y, m_locked_area.height);
					
				void *t_src_ptr;
				t_src_ptr = t_mask -> data + m_locked_area . y * t_mask -> stride + m_locked_area . x;
				surface_merge_with_alpha(m_locked_bits, m_locked_stride, t_src_ptr, t_mask -> stride, t_width, t_height);
			}
			
			((MCScreenDC*)MCscreen)->putimage(m_stack->getwindow(), m_locked_bitmap, 0, 0, m_locked_area.x, m_locked_area.y, m_locked_area.width, m_locked_area.height);
		}
		
		((MCScreenDC*)MCscreen)->destroyimage(m_locked_bitmap);
		m_locked_bitmap = nil;
		m_locked_bits = nil;
	}

	bool LockTarget(MCStackSurfaceTargetType p_type, void*& r_context)
	{
		return false;
	}
	
	void UnlockTarget(void)
	{
	}
};

void MCStack::updatewindow(MCRegionRef p_region)
{
	MCRegionRef t_update_region;
	t_update_region = nil;

	XEvent t_event;
	while(XCheckTypedWindowEvent(MCdpy, window, Expose, &t_event))
	{
		XExposeEvent *t_eevent;
		t_eevent = (XExposeEvent *)&t_event;
		
		if (t_update_region == nil)
			MCRegionCreate(t_update_region);

		MCRegionIncludeRect(t_update_region,  MCU_make_rect(t_eevent->x, t_eevent->y, t_eevent->width, t_eevent->height));
	}

	if (t_update_region != nil)
		MCRegionUnion(t_update_region, t_update_region, p_region);
	else
		t_update_region = p_region;

	onexpose(t_update_region);

	if (t_update_region != p_region)
		MCRegionDestroy(t_update_region);
}

void MCStack::updatewindowwithbuffer(Pixmap p_buffer, MCRegionRef p_region)
{
	s_update_pixmap = p_buffer;
	s_update_rect = MCRegionGetBoundingBox(p_region);
	updatewindow(p_region);
	s_update_pixmap = nil;
}

void MCStack::onexpose(MCRegionRef p_region)
{
	if (s_update_pixmap == nil)
	{
		MCLinuxStackSurface t_surface(this, p_region);
		redrawwindow(&t_surface, p_region);
	}
	else
		MCscreen -> copyarea(s_update_pixmap, window, 0, 0, 0, s_update_rect . width, s_update_rect . height, s_update_rect . x, s_update_rect . y, GXcopy);
}

////////////////////////////////////////////////////////////////////////////////
