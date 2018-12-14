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

//
// platform-specific MCStack class functions
//
#include "lnxprefix.h"

#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"


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

#include "resolution.h"
#include "stacktile.h"
#include "graphics.h"

#include "license.h"
#include "revbuild.h"

static uint2 calldepth;
static uint2 nwait;

////////////////////////////////////////////////////////////////////////////////

extern void surface_merge_with_alpha(void *p_pixels, uint4 p_pixel_stride, void *p_alpha, uint4 p_alpha_stride, uint4 p_width, uint4 p_height);

////////////////////////////////////////////////////////////////////////////////

static MCStackUpdateCallback s_update_callback = nil;
static void *s_update_context = nil;

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

		// IM-2013-10-08: [[ FullscreenMode ]] Don't change stack rect if fullscreen
		/* CODE DELETED */

		MCRectangle t_rect;
		// IM-2014-01-29: [[ HiDPI ]] Convert logical to screen coords
		t_rect = MCscreen->logicaltoscreenrect(view_getrect());

		if (t_rect.width == 0)
			t_rect.width = MCminsize << 4;
		if (t_rect.height == 0)
			t_rect.height = MCminsize << 3;
		
        GdkWindowAttr gdkwa;
        guint gdk_valid_wa;
        gdk_valid_wa = GDK_WA_X|GDK_WA_Y|GDK_WA_VISUAL;
        gdkwa.x = t_rect.x;
        gdkwa.y = t_rect.y;
        gdkwa.width = t_rect.width;
        gdkwa.height = t_rect.height;
        gdkwa.wclass = GDK_INPUT_OUTPUT;
        gdkwa.window_type = GDK_WINDOW_TOPLEVEL;
        gdkwa.visual = screen->getvisual();
        gdkwa.colormap = screen->getcmap();
        gdkwa.event_mask = GDK_ALL_EVENTS_MASK & ~GDK_POINTER_MOTION_HINT_MASK;
        
        window = gdk_window_new(screen->getroot(), &gdkwa, gdk_valid_wa);
        
        // FG-2014-07-30: [[ Bugfix 12905 ]]
        // This is necessary otherwise the window manager might ignore the
        // position that we have specified for the new window
        view_platform_setgeom(t_rect);
        
		// This is necessary to be able to receive drag-and-drop events
        gdk_window_register_dnd(window);
        
		if (screen -> get_backdrop() != DNULL)
            gdk_window_set_transient_for(window, screen->get_backdrop());
        
		loadwindowshape();
		if (m_window_shape != nil && m_window_shape -> is_sharp)
            gdk_window_shape_combine_mask(window, (GdkPixmap*)m_window_shape->handle, 0, 0);
        
        // At least one window has been created so startup is complete
        gdk_notify_startup_complete();
        
        // DEBUGGING
        //gdk_window_set_debug_updates(TRUE);
        //gdk_window_invalidate_rect(window, NULL, TRUE);
        //gdk_window_process_all_updates();

		gdk_display_sync(MCdpy);
	}

	start_externals();
}

void MCStack::setmodalhints()
{
	if (mode == WM_MODAL || mode == WM_SHEET)
	{
		Window t_window = nullptr;
		if (mode == WM_SHEET)
		{
			t_window = getparentwindow();
		}
		
		if (nullptr == t_window && MCtopstackptr.IsValid()) 
		{
			t_window = MCtopstackptr->getwindow();
		}
		gdk_window_set_transient_for(window, t_window);
		gdk_window_set_modal_hint(window, TRUE);
	}
}

// IM-2013-10-08: [[ FullscreenMode ]] Separate out window sizing hints
void MCStack::setsizehints(void)
{
	if (!opened || MCnoui || window == DNULL)
		return;

	if (opened)
	{
		// IM-2013-08-12: [[ ResIndependence ]] Use device coordinates when setting WM hints
		GdkGeometry t_geo;
        gint t_flags = 0;
		if (flags & F_RESIZABLE)
		{
			// IM-2013-10-18: [[ FullscreenMode ]] Assume min/max sizes in view coords
			// for resizable stacks - transform to device coords
			MCRectangle t_minrect, t_maxrect;
			t_minrect = MCRectangleMake(0, 0, minwidth, minheight);
			t_maxrect = MCRectangleMake(0, 0, maxwidth, maxheight);
			
			// IM-2014-01-29: [[ HiDPI ]] Convert logical to screen coords
			t_minrect = MCscreen->logicaltoscreenrect(t_minrect);
			t_maxrect = MCscreen->logicaltoscreenrect(t_maxrect);
			
            t_geo.min_width = t_minrect.width;
            t_geo.max_width = t_maxrect.width;
            t_geo.min_height = t_minrect.height;
            t_geo.max_height = t_maxrect.height;
            t_flags |= GDK_HINT_MIN_SIZE|GDK_HINT_MAX_SIZE;
		}
		else
		{
			// IM-2014-01-29: [[ HiDPI ]] Convert logical to screen coords
			MCRectangle t_device_rect;
			t_device_rect = MCscreen->logicaltoscreenrect(view_getrect());
			
			t_geo.min_width = t_geo.max_width = t_device_rect.width;
            t_geo.min_height = t_geo.max_height = t_device_rect.height;
            t_flags |= GDK_HINT_MIN_SIZE|GDK_HINT_MAX_SIZE;
		}
		
        t_geo.win_gravity = GDK_GRAVITY_STATIC;
        t_flags |= GDK_HINT_WIN_GRAVITY;
        
        gdk_window_set_geometry_hints(window, &t_geo, GdkWindowHints(t_flags));
	}
}

void MCStack::sethints()
{
	if (!opened || MCnoui || window == DNULL)
		return;
		
    // Choose the appropriate type fint for the window
    GdkWindowTypeHint t_type_hint = GDK_WINDOW_TYPE_HINT_NORMAL;
    switch (mode)
    {
        case WM_CLOSED:
        case WM_TOP_LEVEL:
        case WM_TOP_LEVEL_LOCKED:
        case WM_MODELESS:
            break;
            
        case WM_PALETTE:
            t_type_hint = GDK_WINDOW_TYPE_HINT_UTILITY;
            break;
            
        case WM_MODAL:
        case WM_SHEET:
            t_type_hint = GDK_WINDOW_TYPE_HINT_DIALOG;
            break;
            
        case WM_PULLDOWN:
            t_type_hint = GDK_WINDOW_TYPE_HINT_DROPDOWN_MENU;
            break;
            
        case WM_POPUP:
        case WM_OPTION:
        case WM_CASCADE:
            t_type_hint = GDK_WINDOW_TYPE_HINT_POPUP_MENU;
            break;
            
        case WM_COMBO:
            t_type_hint = GDK_WINDOW_TYPE_HINT_COMBO;
            break;
            
        case WM_ICONIC:
            break;
            
        case WM_DRAWER:
            t_type_hint = GDK_WINDOW_TYPE_HINT_DIALOG;
            break;
            
        case WM_TOOLTIP:
            t_type_hint = GDK_WINDOW_TYPE_HINT_TOOLTIP;
            break;
    }
    
    gdk_window_set_type_hint(window, t_type_hint);
    
    if ((mode >= WM_PULLDOWN && mode <= WM_LICENSE))
    {
        gdk_window_set_override_redirect(window, TRUE);
    }
    else
    {
        gdk_window_set_override_redirect(window, FALSE);
    }
    
    // TODO: initial input focus and initial window state
	//whints.input = MCpointerfocus;
	//whints.initial_state = flags & F_START_UP_ICONIC ? IconicState:NormalState;

    gdk_window_set_group(window, ((MCScreenDC*)MCscreen)->GetNullWindow());
    
    // GDK does not provide an easy way to change the WM class properties after
    // window creation time. As a result, we have to do it manually.
    x11::XClassHint chints;

	// Use the name of the home stack as the application name for standalones
	// and the LiveCode name and version for the IDE
    MCAutoStringRef t_app_name;
    MCAutoStringRefAsCString t_app_name_cstr;
	int t_env = MCModeGetEnvironmentType();
	if (t_env == kMCModeEnvironmentTypeEditor
		|| t_env == kMCModeEnvironmentTypeInstaller
		|| t_env == kMCModeEnvironmentTypeServer)
	{
        
        MCAutoStringRef t_edition_name;
        if (MCStringCreateMutable(0, &t_app_name) &&
            MCEditionStringFromLicenseClass(MClicenseparameters.license_class, &t_edition_name))
		{
			bool t_success = true;
			if (t_env == kMCModeEnvironmentTypeEditor)
				t_success = MCStringAppendFormat(*t_app_name, "%s%@_%s", MCapplicationstring, *t_edition_name, MC_BUILD_ENGINE_SHORT_VERSION);
			else
				t_success = MCStringAppendFormat(*t_app_name, "%s%@_%@_%s", MCapplicationstring, *t_edition_name, MCModeGetEnvironment(), MC_BUILD_ENGINE_SHORT_VERSION);
				
			if (t_success)
			{
				t_success = MCStringFindAndReplaceChar(*t_app_name, '.', '_', kMCStringOptionCompareExact) 
					&& MCStringFindAndReplaceChar(*t_app_name, '-', '_', kMCStringOptionCompareExact);
			}
			
			if (!t_success)
				t_app_name.Reset();
		}
	}
	else
	{
		t_app_name = MCNameGetString(MCdispatcher->gethome()->getname());
	}

    if (t_app_name_cstr.Lock(*t_app_name))
	{
		chints.res_name = (char*)*t_app_name_cstr;
		chints.res_class = (char*)*t_app_name_cstr;
    	x11::XSetClassHint(x11::gdk_x11_display_get_xdisplay(MCdpy), x11::gdk_x11_drawable_get_xid(window), &chints);
	}

    // TODO: is this just another way of ensuring on-top-ness?
	//if (mode >= WM_PALETTE)
	//{
	//	uint4 data = 5;
	//	XChangeProperty(MCdpy, window, MClayeratom, XA_CARDINAL, 32,
	//	                PropModeReplace, (unsigned char *)&data, 1);
	//}
	
    // What decorations and modifications should the window have?
    gint t_decorations = 0;
    gint t_functions = 0;
	if (flags & F_RESIZABLE) // && mode != WM_PALETTE)
	{
        t_decorations = GDK_DECOR_RESIZEH | GDK_DECOR_MAXIMIZE | GDK_DECOR_TITLE;
        t_functions = GDK_FUNC_RESIZE | GDK_FUNC_MOVE | GDK_FUNC_MAXIMIZE | GDK_FUNC_CLOSE;
	}
	else
	{
		t_decorations = GDK_DECOR_TITLE | GDK_DECOR_BORDER;
        t_functions = GDK_FUNC_MOVE | GDK_FUNC_MINIMIZE | GDK_FUNC_CLOSE;
	}

    // TODO: input modality hints
    // (According to the GDK documentation, most WMs ignore the Motif hints anyway)
	switch (mode)
	{
        case WM_TOP_LEVEL:
        case WM_TOP_LEVEL_LOCKED:
        case WM_MODELESS:
        case WM_PALETTE:
        case WM_DRAWER:
            t_decorations |= GDK_DECOR_MENU;
            if (mode != WM_PALETTE && view_getrect().width > DECORATION_MINIMIZE_WIDTH)
            {
                t_decorations |= GDK_DECOR_MINIMIZE;
                t_functions |= GDK_FUNC_MINIMIZE;
            }

            //mwmhints.input_mode = MWM_INPUT_MODELESS;
            break;
        case WM_LICENSE:
            t_functions = 0;
            //mwmhints.input_mode = MWM_INPUT_SYSTEM_MODAL;
            break;
        default:
            //mwmhints.input_mode = MWM_INPUT_FULL_APPLICATION_MODAL;
            break;
	}
    
	if (flags & F_DECORATIONS)
	{
        // Set all of the decorations manually
        t_decorations = 0;
        t_functions = 0;
		
		if ( ( decorations & ( WD_TITLE | WD_MENU | WD_MINIMIZE | WD_MAXIMIZE | WD_CLOSE ) )  && flags & F_RESIZABLE ) //&& mode != WM_PALETTE)
		{
			t_decorations |= GDK_DECOR_RESIZEH;
            t_functions |= GDK_FUNC_RESIZE;
		}

		if (decorations & WD_TITLE)
		{
			t_decorations |= GDK_DECOR_TITLE | GDK_DECOR_BORDER;
		}

		if (decorations & WD_MENU)
		{
			t_decorations |= GDK_DECOR_MENU;
		}

		if (decorations & WD_MINIMIZE)
		{
			t_decorations |= GDK_DECOR_MINIMIZE;
            t_functions |= GDK_FUNC_MINIMIZE;
		}
        
		if (decorations & WD_MAXIMIZE)
		{
			t_decorations |= GDK_DECOR_MAXIMIZE;
            t_functions |= GDK_FUNC_MAXIMIZE;
		}
		
		//TS-2007-08-20 Added handler for WD_CLOSE
		if (decorations & WD_CLOSE)
		{
			t_functions |= GDK_FUNC_CLOSE;
		}
		
		if ( decorations != 0 ) 
			t_functions |= GDK_FUNC_MOVE;

		//TS 
		if ( decorations & WD_SHAPE )
		{
			t_decorations = 0;
		}
		
	}
    
    // TODO: test if this comment is still true
	// Gnome gets confused with these set
	//if (flags & F_DECORATIONS)
    gdk_window_set_decorations(window, GdkWMDecoration(t_decorations));
    gdk_window_set_functions(window, GdkWMFunction(t_functions));
	
	//TS 2007-11-08 : Adding in additional hint _NET_WM_STATE == _NET_WM_STATE_ABOVE if we have set WD_UTILITY (i.e. systemwindow == true)
	if (decorations & WD_UTILITY)
	{
		gdk_window_set_keep_above(window, TRUE);
	}
	
	MCstacks->restack(this);
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
		GdkPixmap *t_pixmap;
		t_pixmap = (GdkPixmap*)m_window_shape -> handle;
		if (t_pixmap != nil)
			((MCScreenDC*)MCscreen) -> freepixmap(t_pixmap);
	}

	delete m_window_shape;
	m_window_shape = nil;
}

bool MCStack::view_platform_dirtyviewonresize() const
{
	return false;
}

// IM-2014-01-29: [[ HiDPI ]] Placeholder method for Linux HiDPI support
MCRectangle MCStack::view_platform_getwindowrect(void) const
{
	return view_device_getwindowrect();
}

MCRectangle MCStack::view_device_getwindowrect(void) const
{
    GdkRectangle t_frame;
    gdk_window_get_frame_extents(window, &t_frame);
    MCRectangle t_frame_rect;
    t_frame_rect = MCRectangleMake(t_frame . x, t_frame . y, t_frame . width, t_frame . height);
    
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

// IM-2014-01-29: [[ HiDPI ]] Placeholder method for Linux HiDPI support
MCRectangle MCStack::view_platform_setgeom(const MCRectangle &p_rect)
{
	return view_device_setgeom(p_rect, minwidth, minheight, maxwidth, maxheight);
}

// IM-2013-08-12: [[ ResIndependence ]] factor out device-specific window-sizing code
// set window rect to p_rect, returns old window rect
// IM-2014-01-29: [[ HiDPI ]] Parameterize min/max width/height
MCRectangle MCStack::view_device_setgeom(const MCRectangle &p_rect,
	uint32_t p_minwidth, uint32_t p_minheight,
	uint32_t p_maxwidth, uint32_t p_maxheight)
{
	// Get the position of the window in root coordinates
    gint t_root_x, t_root_y;
    gdk_window_get_origin(window, &t_root_x, &t_root_y);
    
    // Get the dimensions of the window
    gint t_width, t_height;
    t_width = gdk_window_get_width(window);
    t_height = gdk_window_get_height(window);
    
    MCRectangle t_old_rect;
    t_old_rect = MCU_make_rect(t_root_x, t_root_y, t_width, t_height);
    
    if (!(flags & F_WM_PLACE) || state & CS_BEEN_MOVED)
    {
        GdkGeometry t_geom;
        t_geom.win_gravity = GDK_GRAVITY_STATIC;
        
        gint t_hints = GDK_HINT_MIN_SIZE|GDK_HINT_MAX_SIZE|GDK_HINT_WIN_GRAVITY;
        
        if (flags & F_RESIZABLE)
        {
            t_geom.min_width = p_minwidth;
            t_geom.max_width = p_maxwidth;
            t_geom.min_height = p_minheight;
            t_geom.max_height = p_maxheight;
        }
        else
        {
            t_geom.min_width = t_geom.max_width = p_rect.width;
            t_geom.min_height = t_geom.max_height = p_rect.height;
        }
        
        // By setting these "user" flags, we tell the window manager that we
        // know best and it should not attempt to remove or resize the window
        // according to its preferences.
        if (!(flags & F_WM_PLACE) || (state & CS_BEEN_MOVED))
            t_hints |= GDK_HINT_USER_POS;
        t_hints |= GDK_HINT_USER_SIZE;
        
        gdk_window_set_geometry_hints(window, &t_geom, GdkWindowHints(t_hints));
        //gdk_window_move_resize(window, p_rect.x, p_rect.y, p_rect.width, p_rect.height);
    }
    
    if ((!(flags & F_WM_PLACE) || state & CS_BEEN_MOVED) && (t_root_x != p_rect.x || t_root_y != p_rect.y))
    {
        if (t_width != p_rect.width || t_height != p_rect.height)
            gdk_window_move_resize(window, p_rect.x, p_rect.y, p_rect.width, p_rect.height);
        else
            gdk_window_move(window, p_rect.x, p_rect.y);
    }
    else
    {
        if (t_width != p_rect.width || t_height != p_rect.height)
            gdk_window_resize(window, p_rect.width, p_rect.height);
    }
    
    // Set the fullscreen-ness of the window appropriately
    // Use the window manager to set to full screen.
    if (getextendedstate(ECS_FULLSCREEN))
        gdk_window_fullscreen(window);
    else
        gdk_window_unfullscreen(window);
    
	return t_old_rect;
}

void MCStack::setgeom()
{
	if (!opened)
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

	// IM-2013-10-04: [[ FullscreenMode ]] Use view methods to get / set the stack viewport
	MCRectangle t_old_rect;
	t_old_rect = view_getstackviewport();
	
	rect = view_setstackviewport(rect);
	
	state &= ~CS_NEED_RESIZE;
	
	// IM-2013-10-04: [[ FullscreenMode ]] Return values from view methods are
	// in stack coords so don't need to transform
	if (t_old_rect.x != rect.x || t_old_rect.y != rect.y || t_old_rect.width != rect.width || t_old_rect.height != rect.height)
		resize(t_old_rect.width, t_old_rect.height);
		
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
 
void MCStack::platform_openwindow(Boolean override)
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

	gdouble t_opacity;
	t_opacity = gdouble(p_level) / 255.0;

    if (p_level == 255)
        gdk_window_set_opacity(window, 1.0);
    else
        gdk_window_set_opacity(window, t_opacity);
}

void MCStack::updatemodifiedmark(void)
{
}

// MERG-2014-06-02: [[ IgnoreMouseEvents ]] Stub for ignoreMouseEvents.
void MCStack::updateignoremouseevents(void)
{
}

void MCStack::redrawicon(void)
{
}

void MCStack::enablewindow(bool p_enable)
{
	gint t_event_mask;
    
    if (p_enable)
    {
        t_event_mask = GDK_ALL_EVENTS_MASK & ~GDK_POINTER_MOTION_HINT_MASK;
    }
    else
    {
        t_event_mask = GDK_EXPOSURE_MASK | GDK_STRUCTURE_MASK | GDK_PROPERTY_CHANGE_MASK;
    }
    
	gdk_window_set_events(window, GdkEventMask(t_event_mask));
}

void MCStack::applyscroll(void)
{
}

void MCStack::clearscroll(void)
{
}

// MERG-2015-10-12: [[ DocumentFilename ]] Stub for documentFilename.
void MCStack::updatedocumentfilename(void)
{
}

////////////////////////////////////////////////////////////////////////////////

void MCBitmapClearRegion(MCBitmap *p_image, int32_t p_x, int32_t p_y, uint32_t p_width, uint32_t p_height)
{
	uint8_t *t_dst_row = (uint8_t*)gdk_pixbuf_get_pixels(p_image) + p_y * gdk_pixbuf_get_rowstride(p_image) + p_x * sizeof(uint32_t);
    for (uint32_t y = 0; y < p_height; y++)
    {
        MCMemoryClear(t_dst_row, p_width * sizeof(uint32_t));
        t_dst_row += gdk_pixbuf_get_rowstride(p_image);
    }
}

////////////////////////////////////////////////////////////////////////////////

static inline MCRectangle MCGRectangleToMCRectangle(const MCGRectangle &p_rect)
{
	return MCU_make_rect(p_rect.origin.x, p_rect.origin.y, p_rect.size.width, p_rect.size.height);
}

////////////////////////////////////////////////////////////////////////////////

void MCX11PutImage(GdkDisplay *p_dpy, GdkDrawable* d, GdkRegion* p_clip_region, GdkPixbuf *source, int2 sx, int2 sy,
                          int2 dx, int2 dy, uint2 w, uint2 h)
{
	if (d == nil)
		return;

    // If we use gdk_draw_pixbuf, the pixbuf gets blended with the existing
    // contents of the window - something that we definitely do not want. We
    // need to use Cairo directly to do the drawing to the window surface.
    cairo_t *t_cr = gdk_cairo_create(d);
    cairo_set_operator(t_cr, CAIRO_OPERATOR_SOURCE);
    gdk_cairo_region(t_cr, p_clip_region);
    cairo_clip(t_cr);
    gdk_cairo_set_source_pixbuf(t_cr, source, dx-sx, dy-sy);
    cairo_paint(t_cr);
    cairo_destroy(t_cr);
}

bool MCLinuxMCGRegionToRegion(MCGRegionRef p_region, GdkRegion* &r_region);
void MCLinuxRegionDestroy(GdkRegion* p_region);

//////////

// MM-2014-07-31: [[ ThreadedRendering ]] Updated to use the new stack surface API.
class MCLinuxStackSurface: public MCStackSurface
{
	MCStack *m_stack;
	MCGRegionRef m_region;
	
	MCBitmap *m_bitmap;
	MCGIntegerRectangle m_area;
    MCGRaster m_raster;
    
public:
	MCLinuxStackSurface(MCStack *p_stack, MCGRegionRef p_region)
	{
		m_stack = p_stack;
		m_region = p_region;
		m_bitmap = nil;
    }

	bool Lock(void)
	{
		if (m_bitmap != nil)
			return false;
			
		MCGIntegerRectangle t_actual_area;
		t_actual_area = MCGRegionGetBounds(m_region);
		if (MCGIntegerRectangleIsEmpty(t_actual_area))
			return false;

        //fprintf(stderr, "MCLinuxStackSurface::lock(): %d,%d,%d,%d\n", t_actual_area.x, t_actual_area.y, t_actual_area.width, t_actual_area.height);
        
		bool t_success = true;

		if (t_success)
			t_success = nil != (m_bitmap = ((MCScreenDC*)MCscreen)->createimage(32, t_actual_area.size.width, t_actual_area.size.height, false, 0x00));

		if (t_success)
		{
			m_raster . format = kMCGRasterFormat_ARGB;
			m_raster . width = t_actual_area.size.width;
			m_raster . height = t_actual_area.size.height;
			m_raster . stride = gdk_pixbuf_get_rowstride(m_bitmap);
			m_raster . pixels = gdk_pixbuf_get_pixels(m_bitmap);
			m_area = t_actual_area;

			return true;
		}

		if (m_bitmap != nil)
			((MCScreenDC*)MCscreen)->destroyimage(m_bitmap);
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
			MCWindowShape *t_mask;
			t_mask = m_stack -> getwindowshape();
			if (t_mask != nil && !t_mask -> is_sharp)
			{
				if (m_area.origin.x + (int32_t) m_area.size.width > (int32_t) t_mask->width)
					MCBitmapClearRegion(m_bitmap, t_mask->width, 0, m_area.origin.x + m_area.size.width - t_mask->width, m_area.size.height);
				if (m_area.origin.y + (int32_t) m_area.size.height > (int32_t) t_mask->height)
					MCBitmapClearRegion(m_bitmap, 0, t_mask->height, m_area.size.width, m_area.origin.y + m_area.size.height - t_mask->height);
					
				uint32_t t_width = 0;
				uint32_t t_height = 0;
				if (t_mask->width > m_area.origin.x)
					t_width = MCMin((uint32_t)(t_mask->width - m_area.origin.x), m_area.size.width);
				if (t_mask->height > m_area.origin.y)
					t_height = MCMin((uint32_t)(t_mask->height - m_area.origin.y), m_area.size.height);
					
				void *t_src_ptr;
				t_src_ptr = t_mask -> data + m_area.origin.y * t_mask -> stride + m_area.origin.x;
				surface_merge_with_alpha(m_raster.pixels, m_raster.stride, t_src_ptr, t_mask -> stride, t_width, t_height);
			}

			GdkRegion* t_region;
			t_region = nil;
			
			/* UNCHECKED */ MCLinuxMCGRegionToRegion(m_region, t_region);
			
			MCX11PutImage(((MCScreenDC*)MCscreen)->getDisplay(), m_stack->getwindow(), t_region, m_bitmap, 0, 0, m_area.origin.x, m_area.origin.y, m_area.size.width, m_area.size.height);
			
			MCLinuxRegionDestroy(t_region);
		}

		((MCScreenDC*)MCscreen)->destroyimage(m_bitmap);
		m_bitmap = nil;
	}
	
	bool LockGraphics(MCGIntegerRectangle p_area, MCGContextRef& r_context, MCGRaster &r_raster)
	{
		MCGRaster t_raster;
		MCGIntegerRectangle t_locked_area;
		if (LockPixels(p_area, t_raster, t_locked_area))
		{
            MCGContextRef t_context;
			if (MCGContextCreateWithRaster(t_raster, t_context))
			{
				// Set origin
				MCGContextTranslateCTM(t_context, -t_locked_area . origin.x, -t_locked_area . origin.y);

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

	bool LockPixels(MCGIntegerRectangle p_area, MCGRaster &r_raster, MCGIntegerRectangle &r_locked_area)
	{
		if (m_bitmap == nil)
			return false;

        MCGIntegerRectangle t_bounds;
        t_bounds = MCGRegionGetBounds(m_region);
        
        MCGIntegerRectangle t_actual_area;
        t_actual_area = MCGIntegerRectangleIntersection(p_area, MCGRegionGetBounds(m_region));
        
        if (MCGIntegerRectangleIsEmpty(t_actual_area))
            return false;
        
        uint8_t *t_bits;
        t_bits = (uint8_t*) m_raster . pixels + (t_actual_area . origin . y - t_bounds . origin . y) * m_raster.stride + (t_actual_area . origin . x - t_bounds . origin . x) * sizeof(uint32_t);
        r_raster . width = t_actual_area . size . width ;
        r_raster . height = t_actual_area . size . height;
        r_raster . stride = m_raster . stride;
        r_raster . format = kMCGRasterFormat_xRGB;
        r_raster . pixels = t_bits;

		r_locked_area = t_actual_area;

        return true;
	}

	void UnlockPixels(MCGIntegerRectangle p_area, MCGRaster &p_raster)
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

// IM-2014-01-29: [[ HiDPI ]] Placeholder method for Linux HiDPI support
void MCStack::view_platform_updatewindow(MCRegionRef p_region)
{
	view_device_updatewindow(p_region);
}

static bool filter_expose(GdkEvent *p_event, void *p_window)
{
    return p_event->any.window == p_window && (p_event->type == GDK_EXPOSE || p_event->type == GDK_DAMAGE);
}

void MCStack::view_device_updatewindow(MCRegionRef p_region)
{
	MCRegionRef t_update_region;
	t_update_region = nil;

    GdkEvent *t_event;
    while (((MCScreenDC*)MCscreen)->GetFilteredEvent(filter_expose, t_event, window))
    {
        if (t_update_region == nil)
            MCRegionCreate(t_update_region);
        
        MCRegionIncludeRect(t_update_region, MCU_make_rect(t_event->expose.area.x, t_event->expose.area.y, t_event->expose.area.width, t_event->expose.area.height));
    }
    
	if (t_update_region != nil)
		MCRegionAddRegion(t_update_region, p_region);
	else
		t_update_region = p_region;

	onexpose(t_update_region);

	if (t_update_region != p_region)
		MCRegionDestroy(t_update_region);
}

void MCStack::view_platform_updatewindowwithcallback(MCRegionRef p_region, MCStackUpdateCallback p_callback, void *p_context)
{
	s_update_callback = p_callback;
	s_update_context = p_context;
	view_platform_updatewindow(p_region);
	s_update_callback = nil;
	s_update_context = nil;
}

void MCStack::onexpose(MCRegionRef p_region)
{
    MCLinuxStackSurface t_surface(this, (MCGRegionRef)p_region);
    if (t_surface.Lock())
    {
        if (s_update_callback == nil)
            view_surface_redrawwindow(&t_surface, (MCGRegionRef)p_region);
        else
            s_update_callback(&t_surface, p_region, s_update_context);
        
        t_surface.Unlock();
    }
}

////////////////////////////////////////////////////////////////////////////////

bool MCStack::configure_window_buffer()
{
	return true;
}

void MCStack::release_window_buffer()
{
}

////////////////////////////////////////////////////////////////////////////////
