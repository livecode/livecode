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
// ScreenDC virtual functions
//

#include "lnxprefix.h"

#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"

//#include "execpt.h"

#include "dispatch.h"
#include "image.h"
#include "stack.h"
#include "util.h"
#include "variable.h"

#include "globals.h"

#include "printer.h"

#include "lnxdc.h"

#include "lnxpsprinter.h"

#include "mctheme.h"

#include "notify.h"

//#include <X11/extensions/Xinerama.h>

#include "graphics_util.h"

////////////////////////////////////////////////////////////////////////////////

static Boolean pserror;
Bool debugtest = False;

extern "C" int initialise_weak_link_Xinerama(void);

////////////////////////////////////////////////////////////////////////////////

MCGFloat MCResGetSystemScale(void)
{
	// IM-2013-08-12: [[ ResIndependence ]] Linux implementation currently returns 1.0
	return 1.0;
}

////////////////////////////////////////////////////////////////////////////////

MCScreenDC::MCScreenDC()
{
	ncolors = 0;
	ownselection = False;
	pendingevents = NULL;
	backdrop = DNULL;
	backdropcolor.pixel = 0;

	m_backdrop_pixmap = nil;
	
	//Xinerama_available = false ;
	//getdisplays_init = false ;

	m_application_has_focus = true ; // The application start's up having focus, one assumes.
	
	backdrop_hard = false;
	backdrop_active = false;
	m_im_context = NULL;
}

MCScreenDC::~MCScreenDC()
{
	if (opened)
		close(True);
	if (ncolors != 0)
	{
		int2 i;
		for (i = 0 ; i < ncolors ; i++)
		{
            if (colornames[i] != NULL)
                MCValueRelease(colornames[i]);
		}
		delete colors;
		delete colornames;
		delete allocs;
	}
	
    MCNameDelete(vendorname);
	
	while (pendingevents != NULL)
	{
		MCEventnode *tptr =(MCEventnode *)pendingevents->remove
		                   (pendingevents);
		
		if ( tptr != NULL ) 
			delete tptr;
	}
}


//TS : X11 Context creation
/*

  These functions are needed to create the context's that graphics are drawn with.
 
*/

bool MCScreenDC::hasfeature(MCPlatformFeature p_feature)
{
	switch(p_feature)
	{
	case PLATFORM_FEATURE_WINDOW_TRANSPARENCY:
		return is_composite_wm(0);
	break;
		
	case PLATFORM_FEATURE_OS_COLOR_DIALOGS:
		return m_has_native_color_dialogs;
	break;

		
	case PLATFORM_FEATURE_OS_FILE_DIALOGS:
		return m_has_native_file_dialogs;
	break;

	case PLATFORM_FEATURE_OS_PRINT_DIALOGS:
		return m_has_native_print_dialogs;
	break;
		
	case PLATFORM_FEATURE_NATIVE_THEMES:
		return m_has_native_theme;
	break;

	case PLATFORM_FEATURE_TRANSIENT_SELECTION:
		return true;
	break;

	default:
		assert(false);
	break;
	}

	return false;
}

///////////////////////////////////////////////////////////////////////////////

bool MCX11GetWindowWorkarea(GdkDisplay *p_display, Window p_window, MCRectangle &r_workarea)
{
    x11::Atom t_ret;
	int t_format, t_status;
	unsigned long t_count, t_after;
	unsigned long *t_workarea = nil;

    x11::Atom XA_CARDINAL = x11::gdk_x11_atom_to_xatom_for_display(p_display, gdk_atom_intern_static_string("CARDINAL"));
    
    t_status = x11::XGetWindowProperty(x11::gdk_x11_display_get_xdisplay(p_display),
                                       x11::gdk_x11_drawable_get_xid(p_window),
                                       x11::gdk_x11_atom_to_xatom_for_display(p_display, MCworkareaatom),
                                       0, 4, False, XA_CARDINAL, &t_ret, &t_format, &t_count, &t_after,
                                       (unsigned char**)&t_workarea);
	
	bool t_success;
	t_success = t_status == Success && t_ret == XA_CARDINAL && t_format == 32 && t_count == 4;
	
	if (t_success)
		r_workarea = MCRectangleMake(t_workarea[0], t_workarea[1], t_workarea[2], t_workarea[3]);
		
	if (t_workarea != nil)
		x11::XFree(t_workarea);
	
	return t_success;
}

// IM-2014-01-29: [[ HiDPI ]] Apply screen workarea to given MCDisplay array
bool MCScreenDC::apply_workarea(MCDisplay *p_displays, uint32_t p_display_count)
{
	bool t_success;
	t_success = true;
	
	MCRectangle t_workarea;
	t_success = MCX11GetWindowWorkarea(dpy, getroot(), t_workarea);
	
	if (t_success)
	{
		for (uint32_t i = 0; i < p_display_count; i++)
			p_displays[i].workarea = MCU_intersect_rect(t_workarea, p_displays[i].viewport);
	}
	
	return t_success;
}

// IM-2014-01-29: [[ HiDPI ]] Apply screen struts to given MCDisplay array
bool MCScreenDC::apply_partial_struts(MCDisplay *p_displays, uint32_t p_display_count)
{
	if (MCstrutpartialatom == None || MCclientlistatom == None)
		return false;
	
	bool t_success = true;
	
    x11::Atom t_ret;
	int t_format, t_status;
    x11::Window *t_clients = nil;
	unsigned long t_client_count, t_after;
	
    x11::Atom XA_WINDOW = x11::gdk_x11_atom_to_xatom_for_display(dpy, gdk_atom_intern_static_string("WINDOW"));
    x11::Atom XA_CARDINAL = x11::gdk_x11_atom_to_xatom_for_display(dpy, gdk_atom_intern_static_string("CARDINAL"));
    
    t_status = x11::XGetWindowProperty(x11::gdk_x11_display_get_xdisplay(dpy),
                                       x11::gdk_x11_drawable_get_xid(getroot()),
                                       x11::gdk_x11_atom_to_xatom_for_display(dpy, MCclientlistatom),
                                       0, -1, False,XA_WINDOW, &t_ret, &t_format, &t_client_count, &t_after,
                                       (unsigned char **)&t_clients);
    
	t_success = t_status == Success && t_ret == XA_WINDOW && t_format == 32;
	
	if (t_success)
	{
		int32_t t_screenwidth, t_screenheight;
		t_screenwidth = device_getwidth();
		t_screenheight = device_getheight();
		for (uindex_t i = 0; t_success && i < t_client_count; i++)
		{
			unsigned long t_strut_count;
			unsigned long *t_struts = nil;
			
            t_status = x11::XGetWindowProperty(x11::gdk_x11_display_get_xdisplay(dpy),
                                               t_clients[i],
                                               x11::gdk_x11_atom_to_xatom_for_display(dpy, MCstrutpartialatom),
                                               0, 12, False, XA_CARDINAL, &t_ret, &t_format, &t_strut_count, &t_after,
                                               (unsigned char **)&t_struts);

			if (t_status == Success && t_ret == XA_CARDINAL && t_format == 32 && t_strut_count == 12)
			{
				MCRectangle t_strut_rect = {0,0,0,0};
				MCRectangle t_strut_test = {0,0,0,0};
				
				if (t_struts[0] > 0)
				{
					// LEFT
					t_strut_rect.x = t_struts[0];
					t_strut_rect.y = 0;
					t_strut_rect.width = t_screenwidth - t_strut_rect.x;
					t_strut_rect.height = t_screenheight;
				
					t_strut_test = t_strut_rect;
					t_strut_test.y = t_struts[4];
					t_strut_test.height = t_struts[5] - t_strut_test.y;
				}
				else if (t_struts[1] > 0)
				{
					// RIGHT
					t_strut_rect.x = 0;
					t_strut_rect.y = 0;
					t_strut_rect.width = t_screenwidth - t_struts[1];
					t_strut_rect.height = t_screenheight;
				
					t_strut_test = t_strut_rect;
					t_strut_test.y = t_struts[6];
					t_strut_test.height = t_struts[7] - t_strut_test.y;
				}
				else if (t_struts[2] > 0)
				{
					// TOP
					t_strut_rect.x = 0;
					t_strut_rect.y = t_struts[2];
					t_strut_rect.width = t_screenwidth;
					t_strut_rect.height = t_screenheight - t_strut_rect.y;
				
					t_strut_test = t_strut_rect;
					t_strut_test.x = t_struts[8];
					t_strut_test.width = t_struts[9] - t_strut_test.x;
				}
				else if (t_struts[3] > 0)
				{
					// BOTTOM
					t_strut_rect.x = 0;
					t_strut_rect.y = 0;
					t_strut_rect.width = t_screenwidth;
					t_strut_rect.height = t_screenheight - t_struts[3];
				
					t_strut_test = t_strut_rect;
					t_strut_test.x = t_struts[10];
					t_strut_test.width = t_struts[11] - t_strut_test.x;
				}
				
				for (uindex_t s = 0; s < p_display_count; s++)
				{
					MCRectangle t_workarea = p_displays[s].workarea;

					MCRectangle t_test = MCU_intersect_rect(t_strut_test, t_workarea);
					if (t_test.width != 0 && t_test.height != 0)
						t_workarea = MCU_intersect_rect(t_strut_rect, t_workarea);
						
					p_displays[s].workarea = t_workarea;
				}
			}
			if (t_struts != nil)
				x11::XFree(t_struts);
		}
	}
	
	if (t_clients != nil)
		x11::XFree(t_clients);
		
	return t_success;
}

// IM-2014-01-29: [[ HiDPI ]] Placeholder method for Linux HiDPI support
bool MCScreenDC::platform_getdisplays(bool p_effective, MCDisplay *&r_displays, uint32_t &r_display_count)
{
	return device_getdisplays(p_effective, r_displays, r_display_count);
}

// IM-2014-01-29: [[ HiDPI ]] Refactored to handle display info caching in MCUIDC superclass
bool MCScreenDC::device_getdisplays(bool p_effective, MCDisplay * &r_displays, uint32_t &r_display_count)
{
	// NOTE: this code assumes that there is only one GdkScreen!
    GdkScreen *t_screen;
    t_screen = gdk_display_get_default_screen(dpy);
    
    // Get the number of monitors attached to this screen
    gint t_monitor_count;
    t_monitor_count = gdk_screen_get_n_monitors(t_screen);
    
    // Allocate the list of monitors
    MCDisplay *t_displays;
    MCMemoryNewArray(t_monitor_count, t_displays);
    
    // Get the geometry of each monitor
    for (gint i = 0; i < t_monitor_count; i++)
    {
        GdkRectangle t_rect;
        gdk_screen_get_monitor_geometry(t_screen, i, &t_rect);
        
        MCRectangle t_mc_rect;
        t_mc_rect = MCRectangleMake(t_rect.x, t_rect.y, t_rect.width, t_rect.height);
        
        t_displays[i].index = i;
        t_displays[i].pixel_scale = 1.0;
        t_displays[i].viewport = t_displays[i].workarea = t_mc_rect;
    }
    
    if (t_monitor_count == 1)
    {
        apply_workarea(t_displays, t_monitor_count) || apply_partial_struts(t_displays, t_monitor_count);
    }
    else
    {
        apply_partial_struts(t_displays, t_monitor_count);
    }
    
    // All done
    r_displays = t_displays;
    r_display_count = t_monitor_count;
    return true;
}



#define LIST_PRINTER_SCRIPT "put \"\" into tPrinters;" \
 "repeat for each line tLine in shell(\"lpstat -a\");" \
 "put word 1 of tLine & return after tPrinters ;" \
 "end repeat;" \
 "delete the last char of tPrinters;" \
 "get tPrinters; return it" \



bool MCScreenDC::listprinters(MCStringRef& r_printers)
{
	MCdefaultstackptr->domess(MCSTR(LIST_PRINTER_SCRIPT));
    MCresult->copyasvalueref((MCValueRef&)r_printers);
	return true;
}



MCPrinter *MCScreenDC::createprinter(void)
{
	return ( new MCPSPrinter );
	
}

////////////////////////////////////////////////////////////////////////////////

// IM-2014-01-29: [[ HiDPI ]] Placeholder method for Linux HiDPI support
MCPoint MCScreenDC::logicaltoscreenpoint(const MCPoint &p_point)
{
	return p_point;
}

// IM-2014-01-29: [[ HiDPI ]] Placeholder method for Linux HiDPI support
MCPoint MCScreenDC::screentologicalpoint(const MCPoint &p_point)
{
	return p_point;
}

// IM-2014-01-29: [[ HiDPI ]] Placeholder method for Linux HiDPI support
MCRectangle MCScreenDC::logicaltoscreenrect(const MCRectangle &p_rect)
{
	return p_rect;
}

// IM-2014-01-29: [[ HiDPI ]] Placeholder method for Linux HiDPI support
MCRectangle MCScreenDC::screentologicalrect(const MCRectangle &p_rect)
{
	return p_rect;
}

////////////////////////////////////////////////////////////////////////////////

void MCResPlatformInitPixelScaling(void)
{
}

// IM-2014-01-29: [[ HiDPI ]] Pixel scaling not supported on Linux
bool MCResPlatformSupportsPixelScaling(void)
{
	return false;
}

// IM-2014-01-29: [[ HiDPI ]] Pixel scaling not supported on Linux
bool MCResPlatformCanChangePixelScaling(void)
{
	return false;
}

// IM-2014-01-30: [[ HiDPI ]] Pixel scaling not supported on Linux
bool MCResPlatformCanSetPixelScale(void)
{
	return false;
}

// IM-2014-01-30: [[ HiDPI ]] Pixel scale is 1.0 on Linux
MCGFloat MCResPlatformGetDefaultPixelScale(void)
{
	return 1.0;
}

// IM-2014-03-14: [[ HiDPI ]] UI scale is 1.0 on Linux
MCGFloat MCResPlatformGetUIDeviceScale(void)
{
	return 1.0;
}

// IM-2014-01-30: [[ HiDPI ]] Pixel scaling not supported on Linux
void MCResPlatformHandleScaleChange(void)
{
}

////////////////////////////////////////////////////////////////////////////////
