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
// ScreenDC virtual functions
//

#include "lnxprefix.h"

#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"

#include "execpt.h"

#include "dispatch.h"
#include "image.h"
#include "stack.h"
#include "util.h"

#include "globals.h"

#include "printer.h"

#include "lnxdc.h"
#include "lnxcontext.h"

#include "lnxpsprinter.h"

#include "mctheme.h"

#include "notify.h"

#include <X11/extensions/Xinerama.h>

static Boolean pserror;
Bool debugtest = False;

MCDisplay *MCScreenDC::s_monitor_displays = NULL;
uint4 MCScreenDC::s_monitor_count = 0;

extern "C" int initialise_weak_link_Xinerama(void);

MCScreenDC::MCScreenDC()
{
	ncolors = 0;
	ownselection = False;
	pendingevents = NULL;
	backdrop = DNULL;
	backdropcolor.pixel = 0;
	Xinerama_available = false ;
	getdisplays_init = false ;
	m_application_has_focus = true ; // The application start's up having focus, one assumes.
	
	backdrop_hard = false;
	backdrop_active = false;

	MCNotifyInitialize();
}

MCScreenDC::~MCScreenDC()
{
	MCNotifyFinalize();

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



MCContext *MCScreenDC::createcontext ( Drawable p_drawable, MCBitmap *p_mask)
{
	MCContext *t_context ;
	t_context = createcontext ( p_drawable, true, false ) ;
	if ( p_mask != NULL ) 
		((MCX11Context*)t_context) -> map_alpha_data ( p_mask );
	
	return t_context ;
	
}



MCContext *MCScreenDC::createcontext(Drawable p_drawable, bool p_alpha, bool p_transient)
{
	MCContext *t_context;
	uint2 w, h, d ;
	
	getpixmapgeometry(p_drawable, w, h, d ) ;
		
	assert( w > 0 ) ;
	assert( h > 0 ) ;
	
	t_context = MCX11Context::create_context(p_drawable, w, h, dpy, p_alpha);

	return t_context;
}


MCContext *MCScreenDC::creatememorycontext(uint2 p_width, uint2 p_height, bool p_alpha, bool p_transient)
{	
	return MCX11Context::create_memory_context(p_width, p_height, dpy, p_alpha);
}

void MCScreenDC::freecontext(MCContext *p_context)
{
	delete p_context;
}

GC MCScreenDC::getgc(void)
{
	return ( destdepth == 1 ? gc1 : gc );
}


int4 MCScreenDC::textwidth(MCFontStruct *f, const char *s, uint2 l, bool p_unicode_override)
{
	return MCFontlistGetCurrent() -> ctxt_textwidth(f, s, l, p_unicode_override);
}

bool MCScreenDC::apply_workarea()
{
	Atom t_ret;
	int t_format, t_status;
	unsigned long t_count, t_after;
	unsigned long *t_workarea = NULL;

	t_status = XGetWindowProperty(dpy, getroot(), MCworkareaatom, 0, 4, False, XA_CARDINAL,
		&t_ret, &t_format, &t_count, &t_after, (unsigned char**)&t_workarea);
	
	bool t_success;
	t_success = t_status == Success && t_ret == XA_CARDINAL && t_format == 32 && t_count == 4;
	
	if (t_success)
	{
		MCRectangle t_work_rect;
		t_work_rect.x = t_workarea[0];
		t_work_rect.y = t_workarea[1];
		t_work_rect.width = t_workarea[2];
		t_work_rect.height = t_workarea[3];
		
		for (uindex_t i = 0; i < s_monitor_count; i++)
		{
			s_monitor_displays[i].workarea  = MCU_intersect_rect(t_work_rect, s_monitor_displays[i].viewport);
		}
	}
	
	if (t_workarea != NULL)
		XFree(t_workarea);
	
	return t_success;
}

bool MCScreenDC::apply_partial_struts()
{
	if (MCstrutpartialatom == None || MCclientlistatom == None)
		return false;
	
	bool t_success = true;
	
	Atom t_ret;
	int t_format, t_status;
	Window *t_clients = nil;
	unsigned long t_client_count, t_after;
	
	t_status = XGetWindowProperty(dpy, getroot(), MCclientlistatom, 0, -1, False, XA_WINDOW,
		&t_ret, &t_format, &t_client_count, &t_after, (unsigned char**)&t_clients);
	
	t_success = t_status == Success && t_ret == XA_WINDOW && t_format == 32;
	
	if (t_success)
	{
		int32_t t_screenwidth, t_screenheight;
		t_screenwidth = getwidth();
		t_screenheight = getheight();
		for (uindex_t i = 0; t_success && i < t_client_count; i++)
		{
			unsigned long t_strut_count;
			unsigned long *t_struts = nil;
			
			t_status = XGetWindowProperty(dpy, t_clients[i], MCstrutpartialatom, 0, 12, False, XA_CARDINAL,
				&t_ret, &t_format, &t_strut_count, &t_after, (unsigned char**)&t_struts);
			
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
				
				for (uindex_t s = 0; s < s_monitor_count; s++)
				{
					MCRectangle t_workarea = s_monitor_displays[s].workarea;

					MCRectangle t_test = MCU_intersect_rect(t_strut_test, t_workarea);
					if (t_test.width != 0 && t_test.height != 0)
						t_workarea = MCU_intersect_rect(t_strut_rect, t_workarea);
						
					s_monitor_displays[s].workarea = t_workarea;
				}
			}
			if (t_struts != nil)
				XFree(t_struts);
		}
	}
	
	if (t_clients != nil)
		XFree(t_clients);
		
	return t_success;
}

uint4 MCScreenDC::getdisplays(MCDisplay const *& p_displays, bool p_effective)
{
	MCDisplay *t_monitor_displays = NULL;

	// MW-2010-12-14: [[ Bug 9242 ]] The extension name was spelt wrongly! Making this
	//   'XINERAMA' causes screenRects and all things that use it work right.
	if ( !getdisplays_init )
	{
		Xinerama_available = initialise_weak_link_Xinerama();
		if ( Xinerama_available )
		{
			int4 foo ;
			Xinerama_available = XQueryExtension(dpy, "XINERAMA", &foo,&foo,&foo);
		}
		
		getdisplays_init = true ;
	}
	
	if (s_monitor_displays != nil)
	{
		delete s_monitor_displays;
		s_monitor_displays = nil;
		s_monitor_count = 0;
	}
	
		if (Xinerama_available && XineramaIsActive ( dpy ) )
		{

			bool error = false;

			int4 t_monitor_count = 0;
			XineramaScreenInfo *monitors = XineramaQueryScreens (dpy , &t_monitor_count );
			
			t_monitor_displays = new MCDisplay[t_monitor_count];

			for (uint4 a = 0 ; a < t_monitor_count; a++)
			{
				
				t_monitor_displays[a] . index = a;

				t_monitor_displays[a] . viewport . x = monitors[a] . x_org ;
				t_monitor_displays[a] . viewport . y = monitors[a] . y_org ;
				t_monitor_displays[a] . viewport . width = monitors[a] . width ;
				t_monitor_displays[a] . viewport . height = monitors[a] . height ;
				
				t_monitor_displays[a] . workarea . x = monitors[a] . x_org ;
				t_monitor_displays[a] . workarea . y = monitors[a] . y_org ;
				t_monitor_displays[a] . workarea . width = monitors[a] . width ;
				t_monitor_displays[a] . workarea . height = monitors[a] . height ;
			}
			
			XFree(monitors);
			s_monitor_displays = t_monitor_displays ;
			s_monitor_count = t_monitor_count;
		}
		else
		{
			t_monitor_displays = new MCDisplay[1];
			MCU_set_rect(t_monitor_displays[0] . viewport, 0, 0, getwidth(), getheight());
			MCU_set_rect(t_monitor_displays[0] . workarea, 0, 0, getwidth(), getheight());
			t_monitor_displays[0] . index = 0 ;
			s_monitor_count = 1 ;
			s_monitor_displays = t_monitor_displays ;

		}
	
	if (s_monitor_count == 1)
	{
		apply_workarea() || apply_partial_struts();
	}
	else if (s_monitor_count > 1)
	{
		apply_partial_struts();
	}
	
	p_displays = s_monitor_displays;
	
	return (s_monitor_count);
		

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
	MCresult->eval(false, (MCValueRef&)r_printers);
	return true;
}



MCPrinter *MCScreenDC::createprinter(void)
{
	return ( new MCPSPrinter );
	
}


MCStack *MCScreenDC::getstackatpoint(int32_t x, int32_t y)
{
	return nil;
}
