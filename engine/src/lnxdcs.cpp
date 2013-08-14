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
// ScreenDC display specific functions
//

#include "lnxprefix.h"

#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"

#include "dispatch.h"
#include "image.h"
#include "stack.h"
#include "util.h"
#include "execpt.h"
#include "stacklst.h"

#include "sellst.h"

#include "globals.h"

#include "mctheme.h"

#include "lnxdc.h"
#include "lnximagecache.h"

#include <langinfo.h>
#include <fcntl.h>
#include <sys/shm.h>

#include <libgnomevfs/gnome-vfs.h>
#include <libgnomevfs/gnome-vfs-utils.h>

#include <X11/extensions/Xv.h>
#include <X11/extensions/Xvlib.h>

#undef STARTMSGS

#define REQUEST_SIZE 32   // size of request header for X calls
#define MAX_POINTS 8096U   // number of points in polygon
#define ICON_SIZE 48
#define XYCUTOFF 4        // plane based or packed pixel

////////////////////////////////////////////////////////////////////////////////

bool MCImageBitmapCreateWithXImage(XImage *p_image, MCImageBitmap *&r_bitmap);

////////////////////////////////////////////////////////////////////////////////

Display *MCdpy;

////////////////////////////////////////////////////////////////////////////////

extern "C" int initialise_weak_link_gobject(void);
extern "C" int initialise_weak_link_gdk(void);
extern "C" int initialise_weak_link_gtk(void);
extern "C" int initialise_weak_link_gtk_color_dialog(void);
extern "C" int initialise_weak_link_gtk_file_dialog(void);
extern "C" int initialise_weak_link_gtk_print_dialog(void);
extern "C" int initialise_weak_link_gnome_vfs ( void ) ;
extern "C" int initialise_weak_link_glib ( void ) ;
extern "C" int initialise_weak_link_libgnome ( void ) ;
extern "C" int initialise_weak_link_libgnome ( void ) ;
extern "C" int initialise_weak_link_libxv ( void ) ;

////////////////////////////////////////////////////////////////////////////////

static uint1 flip_table[] =
    {
        0x00, 0x80, 0x40, 0xC0, 0x20, 0xA0, 0x60, 0xE0,
        0x10, 0x90, 0x50, 0xD0, 0x30, 0xB0, 0x70, 0xF0,
        0x08, 0x88, 0x48, 0xC8, 0x28, 0xA8, 0x68, 0xE8,
        0x18, 0x98, 0x58, 0xD8, 0x38, 0xB8, 0x78, 0xF8,
        0x04, 0x84, 0x44, 0xC4, 0x24, 0xA4, 0x64, 0xE4,
        0x14, 0x94, 0x54, 0xD4, 0x34, 0xB4, 0x74, 0xF4,
        0x0C, 0x8C, 0x4C, 0xCC, 0x2C, 0xAC, 0x6C, 0xEC,
        0x1C, 0x9C, 0x5C, 0xDC, 0x3C, 0xBC, 0x7C, 0xFC,
        0x02, 0x82, 0x42, 0xC2, 0x22, 0xA2, 0x62, 0xE2,
        0x12, 0x92, 0x52, 0xD2, 0x32, 0xB2, 0x72, 0xF2,
        0x0A, 0x8A, 0x4A, 0xCA, 0x2A, 0xAA, 0x6A, 0xEA,
        0x1A, 0x9A, 0x5A, 0xDA, 0x3A, 0xBA, 0x7A, 0xFA,
        0x06, 0x86, 0x46, 0xC6, 0x26, 0xA6, 0x66, 0xE6,
        0x16, 0x96, 0x56, 0xD6, 0x36, 0xB6, 0x76, 0xF6,
        0x0E, 0x8E, 0x4E, 0xCE, 0x2E, 0xAE, 0x6E, 0xEE,

        0x1E, 0x9E, 0x5E, 0xDE, 0x3E, 0xBE, 0x7E, 0xFE,
        0x01, 0x81, 0x41, 0xC1, 0x21, 0xA1, 0x61, 0xE1,
        0x11, 0x91, 0x51, 0xD1, 0x31, 0xB1, 0x71, 0xF1,
        0x09, 0x89, 0x49, 0xC9, 0x29, 0xA9, 0x69, 0xE9,
        0x19, 0x99, 0x59, 0xD9, 0x39, 0xB9, 0x79, 0xF9,
        0x05, 0x85, 0x45, 0xC5, 0x25, 0xA5, 0x65, 0xE5,
        0x15, 0x95, 0x55, 0xD5, 0x35, 0xB5, 0x75, 0xF5,
        0x0D, 0x8D, 0x4D, 0xCD, 0x2D, 0xAD, 0x6D, 0xED,
        0x1D, 0x9D, 0x5D, 0xDD, 0x3D, 0xBD, 0x7D, 0xFD,
        0x03, 0x83, 0x43, 0xC3, 0x23, 0xA3, 0x63, 0xE3,
        0x13, 0x93, 0x53, 0xD3, 0x33, 0xB3, 0x73, 0xF3,
        0x0B, 0x8B, 0x4B, 0xCB, 0x2B, 0xAB, 0x6B, 0xEB,
        0x1B, 0x9B, 0x5B, 0xDB, 0x3B, 0xBB, 0x7B, 0xFB,
        0x07, 0x87, 0x47, 0xC7, 0x27, 0xA7, 0x67, 0xE7,
        0x17, 0x97, 0x57, 0xD7, 0x37, 0xB7, 0x77, 0xF7,
        0x0F, 0x8F, 0x4F, 0xCF, 0x2F, 0xAF, 0x6F, 0xEF,
        0x1F, 0x9F, 0x5F, 0xDF, 0x3F, 0xBF, 0x7F, 0xFF
    };

static uint4 cmap_scale[17] =
    {
        65535, 65535, 21845, 9362,
        4369,  2114,  1040,  516,
        257,   128,    64,   32,
        16,     8,     4,    2,   1
    };

////////////////////////////////////////////////////////////////////////////////

void MCScreenDC::setstatus(const char *status)
{
}

Boolean MCScreenDC::setdest(Drawable d, uint2 depth)
{
	dest = d;
	if (depth == 0)
		destdepth = getdepth();
	else
		destdepth = depth;
	return True;
}


//XDND
void init_xDnD(void);

Drawable MCScreenDC::getdest()
{
	return dest;
}

Boolean MCScreenDC::open()
{
	// Check to see if we are in a UTF8 locale
	// TS : Changed 2008-01-08 as a more relaible way of testing for UTF-8
	MCutf8 = (strcmp(nl_langinfo(CODESET), "UTF-8") == 0)	;
	
	MCimagecache = new MCXImageCache ;
	
	if ((dpy = XOpenDisplay(MCdisplayname)) == NULL)
	{
		fprintf(stderr, "%s: Can't open display %s\n",
		        MCcmd, XDisplayName(MCdisplayname));
		return False;
	}
	fcntl(ConnectionNumber(dpy), F_SETFD, 1);
	displayname = XDisplayName(MCdisplayname);
	
#ifdef SYNCMODE
	XSynchronize(dpy, True);
	XSync ( dpy, False ) ;
#ifdef STARTMSGS
	fprintf(stderr, "Xserver sync on\n");
#endif
#endif
		
	//XDND - Need to set up the xDnD protocol so that we have access to the XdndAware atom.
	init_xDnD();

		
		
	XVisualInfo vis_template;
	int nitems;
	if (MCvisualid != 0)
	{
		vis_template.visualid = MCvisualid;
		vis_template.screen = DefaultScreen(dpy);
		vis = XGetVisualInfo(dpy, VisualIDMask | VisualScreenMask,
		                     &vis_template, &nitems);
		if (nitems != 1)
		{
			fprintf(stderr, "%s: Bad visual id %x\n", MCcmd, MCvisualid);
			MCvisualid = 0;
		}
		else
		{
			int2 i;
			switch (vis->c_class)
			{
			case StaticGray:
			case StaticColor:
				cmap = XCreateColormap(dpy, getroot(), vis->visual, AllocNone);
				setupcolors();
				break;
			case TrueColor:
				cmap = XCreateColormap(dpy, getroot(), vis->visual, AllocNone);

				MCU_getshift(vis->red_mask, redshift, redbits);
				MCU_getshift(vis->green_mask, greenshift, greenbits);
				MCU_getshift(vis->blue_mask, blueshift, bluebits);
				break;
			case DirectColor:
				{
					XColor defs[256];
					cmap = XCreateColormap(dpy, getroot(), vis->visual, AllocAll);
					MCU_getshift(vis->red_mask, redshift, redbits);
					MCU_getshift(vis->green_mask, greenshift, greenbits);
					MCU_getshift(vis->blue_mask, blueshift, bluebits);
					uint4 r_scale = cmap_scale[redbits];

					uint4 g_scale = cmap_scale[greenbits];
					uint4 b_scale = cmap_scale[bluebits];
					uint4 rmsk = (1 << redbits) - 1;
					uint4 gmsk = (1 << greenbits) - 1;
					uint4 bmsk = (1 << bluebits) - 1;
					for( i = 0 ; i < vis->colormap_size ; i++)
					{
						defs[i].pixel = i << redshift & vis->red_mask
						                | i << greenshift & vis->green_mask
						                | i << blueshift & vis->blue_mask;
						defs[i].red   = (i & rmsk) * r_scale;
						defs[i].green = (i & gmsk) * g_scale;
						defs[i].blue  = (i & bmsk) * b_scale;

						defs[i].flags = DoRed | DoGreen | DoBlue;
					}
					XStoreColors(dpy, cmap, (XColor *)defs, vis->colormap_size);
				}
				break;
			case GrayScale:
				cmap = XCreateColormap(dpy, getroot(), vis->visual, AllocAll);
				setupcolors();
				for (i = 0 ; i < ncolors ; i++)
					colors[i].red = colors[i].green = colors[i].blue
					                                  = i * MAXUINT2 / ncolors;
				XStoreColors(dpy, cmap, (XColor *)colors, ncolors);
				break;
			case PseudoColor:
				cmap = XCreateColormap(dpy, getroot(), vis->visual, AllocNone);
				setupcolors();
				MCuseprivatecmap = True;
				break;
			}
		}
	}
	if (MCvisualid == 0)
	{
		vis_template.screen = DefaultScreen(dpy);
		
		has_composite_wm = is_composite_wm( 0 ) ;

		
		
		if ( has_composite_wm ) 
		{
#ifdef STARTMSGS			
			fprintf(stderr, "Composite window manager detected. Trying to use 32 bit windowing.\n");
#endif
			vis_template.depth = 32;
			vis = XGetVisualInfo(dpy, VisualScreenMask | VisualDepthMask,
			                     &vis_template, &nitems);
			
			
			cmap = XCreateColormap (dpy, getroot(), vis->visual, AllocNone);
		}
		else
		{
			vis_template.visualid = DefaultVisual(dpy, DefaultScreen(dpy))->visualid;
			vis = XGetVisualInfo(dpy, VisualScreenMask | VisualIDMask,
			                     &vis_template, &nitems);
			cmap = DefaultColormap(dpy, getscreen());
		}
		
	
		// Create our 32bit visual and colormap for internal use
		vis_template.depth = getdepth();
		
		vis_template.visualid = 0 ; 
		vis32 = XGetVisualInfo(dpy, VisualDepthMask, &vis_template, &nitems);
		cmap32 = XCreateColormap (dpy, getroot(), vis->visual, AllocNone);


		
		if (vis->c_class == TrueColor)
		{
			MCU_getshift(vis->red_mask, redshift, redbits);
			MCU_getshift(vis->green_mask, greenshift, greenbits);
			MCU_getshift(vis->blue_mask, blueshift, bluebits);
		}
		else
		{
			setupcolors();
		}
	}
	


	XSetWindowAttributes xswa;
	unsigned long xswamask = CWBorderPixel | CWColormap;
	xswa.border_pixel = 0;
	xswa.colormap = cmap; 
	
	

	// Creation of the "Native" depth GC
	Window w = XCreateWindow(dpy, RootWindow(dpy, vis->screen), 8, 8, 8, 8,
	                         0, MCscreen->getrealdepth(), InputOutput, vis->visual,
	                         xswamask, &xswa);
	
	gc = XCreateGC(dpy, w, 0, NULL);
	XDestroyWindow(dpy, w);

	NULLWindow = XCreateWindow(dpy, RootWindow(dpy, vis->screen), 8, 8, 8, 8,
	                         0, MCscreen->getrealdepth(), InputOutput, vis->visual,
	                         xswamask, &xswa);
	XSelectInput(dpy, NULLWindow,  ButtonPressMask | ButtonReleaseMask
	             | EnterWindowMask | LeaveWindowMask | PointerMotionMask
	             | KeyPressMask | KeyReleaseMask | ExposureMask
	             | FocusChangeMask | StructureNotifyMask | PropertyChangeMask);
	
	
	
	
	
	
	XSetGraphicsExposures(dpy, gc, False);
	XSetBackground(dpy, gc, 0);

	initatoms();

	black_pixel.red = black_pixel.green = black_pixel.blue = 0;
	black_pixel.pixel = 0xff000000;
	white_pixel.red = white_pixel.green = white_pixel.blue = MAXUINT2;
	white_pixel.pixel = 0xffffffff;

	MCdpy = dpy;

	Pixmap cdata = XCreatePixmap(dpy, getroot(), 16, 16, 1);
	Pixmap cmask = XCreatePixmap(dpy, getroot(), 16, 16, 1);
	XSetForeground(dpy, gc1, 0);
	XFillRectangle(dpy, cdata, gc1, 0, 0, 16, 16);

	XFillRectangle(dpy, cmask, gc1, 0, 0, 16, 16);
	XSetForeground(dpy, gc1, 1);
	MCColor c;
	c.red = c.green = c.blue = 0x0;
	for(uint32_t i = 0; i < PI_NCURSORS; i++)
		MCcursors[i] = nil;
	XFreePixmap(dpy, cdata);
	XFreePixmap(dpy, cmask);

	MConecolor.red = MConecolor.green = MConecolor.blue = 0xFFFF;
	MConecolor.pixel = 1;
	MCselectioncolor = MCpencolor = black_pixel;
	alloccolor(MCselectioncolor);
	alloccolor(MCpencolor);
	MCbrushcolor = white_pixel;
	alloccolor(MCbrushcolor);
	alloccolor(MChilitecolor);
	MCaccentcolor = MChilitecolor;
	alloccolor(MCaccentcolor);
	gray_pixel.red = gray_pixel.green = gray_pixel.blue = 0x8080;
	alloccolor(gray_pixel);

	background_pixel.red = background_pixel.green = background_pixel.blue = 0xdcdc;
	alloccolor(background_pixel);
	if (MCcurtheme && MCcurtheme->getthemeid() == LF_NATIVEGTK)
		MCcurtheme->load();
	opened = True;
	selectiontext = NULL;
	int2 x, y;
	querymouse(x, y);

	if (MCmodifierstate & 0x02 << MCextendkey)
		MCextendkey = 5; // disable bogus numlock key binding on some SPARCs
	MCwbr.y = MCwbr.x = 0;
	MCwbr.width = getwidth();
	MCwbr.height = getheight();
	
	m_has_native_theme =
		initialise_weak_link_gobject() != 0 &&
		initialise_weak_link_gdk() != 0 &&
		initialise_weak_link_gtk() != 0;
	
	if (m_has_native_theme)
	{
		m_has_native_color_dialogs = initialise_weak_link_gtk_color_dialog() != 0;
		m_has_native_file_dialogs = initialise_weak_link_gtk_file_dialog() != 0;
		m_has_native_print_dialogs = initialise_weak_link_gtk_print_dialog() != 0;
	}

	MCXVideo = false ;
	if ( initialise_weak_link_libxv () != 0 ) 
	{
		XvAdaptorInfo	*ai;
		unsigned int	p_num_adaptors;
		int ret ;
		ret = XvQueryAdaptors(dpy, DefaultRootWindow(dpy), &p_num_adaptors, &ai);
		
		MCXVideo = (( ret == Success ) && ( p_num_adaptors >  0 )) ;
		
	}
		
	if ( initialise_weak_link_gnome_vfs() != 0 )
	{
		initialise_weak_link_glib();
		MCuselibgnome = initialise_weak_link_libgnome();
		gnome_vfs_init();
	}
	
	// Create the various Transfer data stores....
	m_DND_store = new MCXTransferStore ( dpy ) ;
	m_Clipboard_store = new MCXTransferStore ( dpy ) ;
	m_Selection_store = new MCXTransferStore ( dpy ) ;

	return True; 
}

// Defined in xans.cpp
extern void gtk_file_tidy_up ( void );



// Returns an XAtom with the given name ;
Atom  MCScreenDC::make_atom ( char * p_atom_name ) 
{
	return ( XInternAtom ( dpy, p_atom_name, false ) );
}

//XDND
void shutdown_xdnd(void) ;

Boolean MCScreenDC::close(Boolean force)
{
	// TODO - We may need to do clipboard persistance here
	
	destroybackdrop();
	XFlush(dpy);
	
	XFreeGC ( dpy, gc ) ;
	XFreeGC ( dpy, gc1 ) ;
	
	// I.M. 12/01/09
	// We need to free all X server resources before closing
    XFreePixmap(dpy, graystipple);
	//XDND
	shutdown_xdnd();

	
	XCloseDisplay(dpy);
	delete (char *)selectiontext.getstring();
	opened = False;
	
	gtk_file_tidy_up () ;

	
	delete MCimagecache ;
	
	return True;
}

const char *MCScreenDC::getdisplayname()
{
	return displayname;
}


uint2 MCScreenDC::getrealdepth(void)
{
	return vis -> depth ;
}



uint2 MCScreenDC::getdepth(void)
{
	if (vis -> depth < 24)
		return 32;
	
	return ( vis -> depth ) ;
}

void MCScreenDC::grabpointer(Window w)
{

	XGrabPointer(dpy, w, False,
	             PointerMotionMask | ButtonPressMask | ButtonReleaseMask,
	             GrabModeAsync, GrabModeAsync, None, None, MCeventtime);
}

void MCScreenDC::ungrabpointer()
{
	XUngrabPointer(dpy, MCeventtime);
}

uint16_t MCScreenDC::device_getwidth(void)
{
	return DisplayWidth(dpy, getscreen());
}

uint16_t MCScreenDC::device_getheight(void)
{
	return DisplayHeight(dpy, getscreen());
}

uint2 MCScreenDC::getwidthmm()
{
	return WidthMMOfScreen(ScreenOfDisplay(dpy, getscreen()));
}

uint2 MCScreenDC::getheightmm()
{
	return HeightMMOfScreen(ScreenOfDisplay(dpy, getscreen()));
}

// MW-2005-09-24: We shouldn't be accessing the display structure like this
//   so use the XMaxRequestSize() call instead
uint2 MCScreenDC::getmaxpoints()
{
	return MCU_min(XMaxRequestSize(dpy) - REQUEST_SIZE, MAX_POINTS);
}

uint2 MCScreenDC::getvclass()
{
	return vis->c_class;
}

void MCScreenDC::openwindow(Window window, Boolean override)
{
	MCStack *target = MCdispatcher->findstackd(window);
	XMapRaised(dpy, window);
	MCstacks -> enableformodal(window, False);
}

void MCScreenDC::closewindow(Window window)
{
	MCStack *target = MCdispatcher->findstackd(window);
	MCstacks -> enableformodal(window, True);
	XWithdrawWindow(dpy, window, getscreen());
}

void MCScreenDC::destroywindow(Window &window)
{
	XDestroyWindow(dpy, window);
	window = DNULL;
}

void MCScreenDC::raisewindow(Window window)
{
	XRaiseWindow(dpy, window);
}

void MCScreenDC::iconifywindow(Window window)

{
	XIconifyWindow(dpy, window, getscreen());
}

void MCScreenDC::uniconifywindow(Window window)
{
	XMapRaised(dpy, window);
}

void MCScreenDC::setname(Window window, const char *newname)
{
	MCExecPoint ep(NULL, NULL, NULL) ;
	ep.setsvalue ( newname );
	ep.utf8tonative() ;
	XStoreName(dpy, window, ep.getcstring());
	XSetIconName(dpy, window, ep.getcstring());

	XChangeProperty(dpy, window, XInternAtom(dpy, "_NET_WM_NAME", false), XInternAtom(dpy, "UTF8_STRING", false), 8, PropModeReplace, (unsigned char *)newname, strlen(newname));

}

void MCScreenDC::setcmap(MCStack *sptr)
{
	XSetWindowColormap(dpy, sptr->getw(), getcmap());
}

void MCScreenDC::sync(Window w)
{
	XSync(dpy, False);
}

void MCScreenDC::flush(Window w)
{
	XFlush(dpy);
}

void MCScreenDC::beep()
{
	bool t_use_internal = false ; 
	if ( m_sound_internal != NULL)
		if ( strcmp(m_sound_internal, "internal") == 0 )
			t_use_internal = true ;
	XBell(dpy, 0);
}

void MCScreenDC::setinputfocus(Window window)
{
	XSetInputFocus(dpy, window, RevertToParent, MCeventtime);
}

void MCScreenDC::freepixmap(Pixmap &pixmap)
{
	if (pixmap != DNULL)
	{
		XFreePixmap(dpy, pixmap);

		pixmap = DNULL;
	}
}

Pixmap MCScreenDC::createpixmap(uint2 width, uint2 height,
                                uint2 depth, Boolean purge)
{
	if (depth == 0 || depth == 32)
	{
		if (vis -> depth == 24)
			depth = 24;
		else
			depth = 32;
	}
	
	Pixmap pm = XCreatePixmap(dpy, getroot(), width, height, depth);
	Window root;
	int x, y;
	unsigned int w, h, b, d;
	if (!XGetGeometry(dpy, pm, &root, &x, &y, &w, &h, &b, &d))
		pm = DNULL;

	assert ( pm != DNULL ) ;
	
	return pm;
}

bool MCScreenDC::device_getwindowgeometry(Window w, MCRectangle &drect)
{
	Window root, child;
	int x, y;
	unsigned int width, bwidth, height, depth;
	if (!XGetGeometry(dpy, w, &root, &x, &y, &width, &height, &bwidth, &depth))
		return false;
	XTranslateCoordinates(dpy, w, root, 0, 0, &x, &y, &child);
	MCU_set_rect(drect, x, y, width, height);
	return true;
}


Boolean MCScreenDC::getpixmapgeometry(Pixmap p, uint2 &w, uint2 &h, uint2 &d)
{
	Window root;
	int x, y;
	unsigned int width, bwidth, height, depth;
	
	Status t_status;
	t_status = XGetGeometry(dpy, p, &root, &x, &y, &width, &height, &bwidth, &depth);
	
	assert(t_status != 0);
	
	w = width;
	h = height;
	d = depth;
	
	return True;
}

void MCScreenDC::setgraphicsexposures(Boolean on, MCStack *sptr)
{
	XSetGraphicsExposures(dpy, gc, on);
}

void MCScreenDC::copyarea(Drawable s, Drawable d, int2 depth,
                          int2 sx, int2 sy,  uint2 sw, uint2 sh, int2 dx,
                          int2 dy, uint4 rop)
{
	if (s == nil || d == nil)
		return;
	
	GC t_gc ; 
	t_gc = XCreateGC( dpy, d, 0, NULL ) ;
	
	assert ( rop <= GXset) ;
	if (s == DNULL || d == DNULL)
		return;
	if (rop != GXcopy)
		XSetFunction(dpy, t_gc, rop);
	XCopyArea(dpy, s, d, t_gc, sx, sy, sw, sh, dx, dy);
	if (rop != GXcopy)
		XSetFunction(dpy, t_gc, GXcopy);
	
	XFreeGC ( dpy, t_gc ) ;
}

MCBitmap *MCScreenDC::createimage(uint2 depth, uint2 width, uint2 height,
                                  Boolean set
	                                  , uint1 value,
	                                  Boolean shm, Boolean forceZ)
{
	uint4 bytes = 0;
	MCBitmap *image = NULL;
	
	
	if (depth == 0 || depth == 32)
	{
		if (vis -> depth == 24)
			depth = 24;
		else
			depth = 32;
	}

	if (image == NULL)
	{
		if (forceZ || depth > XYCUTOFF)
			image = (MCBitmap *)XCreateImage(dpy, getvisual(), depth, ZPixmap, 0,
			                     NULL, width, height, getpad(), 0);
		else
			image = (MCBitmap *)XCreateImage(dpy, getvisual(), depth, XYPixmap, 0,
			                     NULL, width, height, getpad(), 0);
		
		assert(image != NULL);

		bytes = image->bytes_per_line * image->height;
		if (image->bits_per_pixel == 1)
			bytes *= image->depth;
		image->data = (char *)new uint1[bytes];
		image->obdata = NULL;
	}
	if (set && image != NULL )
		memset(image->data, value, bytes);

	assert(image != NULL);
	
	image->byte_order = MCswapbytes && depth > 1 ? LSBFirst : MSBFirst;
	image->bitmap_bit_order = MSBFirst;
	image->red_mask = image->green_mask = image->blue_mask
	    	                                  = depth == 1 || depth == getdepth() ? 0x00 : 0xFF;

	return image;
}

void MCScreenDC::destroyimage(MCBitmap *image)
{
	if (image->data != NULL)
	{
		delete image->data;
		image->data = NULL;
	}
	XDestroyImage((XImage *)image);
	image = NULL ;
}

void MCScreenDC::putimage(Drawable d, MCBitmap *source, int2 sx, int2 sy,
                          int2 dx, int2 dy, uint2 w, uint2 h)
{
	if (d == nil)
		return;

	GC t_gc;

	t_gc = XCreateGC(dpy, d, 0, NULL ) ;
	XPutImage(dpy, d, t_gc , (XImage *)source, sx, sy, dx, dy, w, h);
	XFreeGC(dpy, t_gc);
}

XImage *MCScreenDC::getimage(Drawable d, int2 x, int2 y,
                               uint2 w, uint2 h, Boolean shm)
{
	if (d == DNULL)
		d = getroot();
	XImage *b = XGetImage(dpy, d, x, y, w, h, AllPlanes, ZPixmap);

	assert ( b != NULL ) ;
	
	flipimage(b, MSBFirst, MSBFirst);
	
	return b;

}

void MCScreenDC::flipimage(XImage *image, int2 byte_order, int2 bit_order)
{
	if (image == NULL)
		return;
	if (image->depth == 1 && image->bitmap_bit_order != bit_order)
	{
		uint4 bytes = image->bytes_per_line * image->height;
		if (image->bits_per_pixel != image->depth)
			bytes *= image->depth / image->bits_per_pixel;
		uint1 *dptr = (uint1 *)image->data;
		while (bytes--)
		{
			*dptr = flip_table[*dptr];
			dptr++;
		}
		image->bitmap_bit_order = bit_order;
	}
	else
		if (image->byte_order == MCswapbytes)
		{
			Boolean oldswap = MCswapbytes;
			MCswapbytes = True;
			uint4 pixels = image->width * image->height;
			if (image->depth >= 24)
			{
				uint4 *sptr = (uint4 *)image->data;
				while (pixels--)
					swap_uint4(sptr++);
			}
			else
				if (image->depth > 8)
				{
					uint2 *sptr = (uint2 *)image->data;
					while (pixels--)
						swap_uint2(sptr++);
				}
			MCswapbytes = oldswap;
		}
}

#ifdef OLD_GRAPHICS
MCBitmap *MCScreenDC::regiontomask(MCRegionRef r, int32_t w, int32_t h)
{
	Pixmap t_image;
	t_image = createpixmap(w, h, 1, False);

	XSetForeground(dpy, gc1, 0);
	XFillRectangle(dpy, t_image, gc1, 0, 0, w, h);

	XSetRegion(dpy, gc1, (Region)r);

	XSetForeground(dpy, gc1, 1);
	XFillRectangle(dpy, t_image, gc1, 0, 0, w, h);

	XSetClipMask(dpy, gc1, None);

	MCBitmap *t_bitmap;
	t_bitmap = getimage(t_image, 0, 0, w, h, False);

	freepixmap(t_image);

	return t_bitmap;
}
#endif

void MCScreenDC::setfunction(uint4 rop)
{
	if (rop < 0x10)
		XSetFunction(dpy, getgc(), rop);
}

uint4 MCScreenDC::dtouint4(Drawable d)
{
	return d;
}

Boolean MCScreenDC::uint4towindow(uint4 id, Window &w)
{
	w = id;
	return True;
}

void MCScreenDC::getbeep(uint4 which, MCExecPoint &ep)
{
	XKeyboardState values;
	XGetKeyboardControl(dpy, &values);
	switch (which)
	{
	case P_BEEP_LOUDNESS:
		ep.setint(values.bell_percent);
		break;
	case P_BEEP_PITCH:
		ep.setint(values.bell_pitch);
		break;
	case P_BEEP_DURATION:
		ep.setint(values.bell_duration);
		break;
	}
}


void MCScreenDC::setbeep(uint4 which, int4 beep)
{
	XKeyboardControl control;
	switch (which)
	{
	case P_BEEP_LOUDNESS:
		beep = MCU_min(beep, 100);
		control.bell_percent = beep;
		XChangeKeyboardControl(dpy, KBBellPercent, &control);
		break;
	case P_BEEP_PITCH:
		control.bell_pitch = beep;
		XChangeKeyboardControl(dpy, KBBellPitch, &control);
		break;
	case P_BEEP_DURATION:
		control.bell_duration = beep;
		XChangeKeyboardControl(dpy, KBBellDuration, &control);

		break;
	}
}

void MCScreenDC::getvendorstring(MCExecPoint &ep)
{
	ep.setstringf("%s %d", ServerVendor(dpy), VendorRelease(dpy));
}

uint2 MCScreenDC::getpad()
{
	return BitmapPad(dpy);
}

Window MCScreenDC::getroot()
{
	return RootWindow(dpy, getscreen());
}

MCImageBitmap *MCScreenDC::snapshot(MCRectangle &r, uint4 window,
                               const char *displayname)
{
	Display *olddpy = dpy;
	Colormap oldcmap = cmap;
	Window root = getroot();
	uint2 screen = getscreen();
	if (displayname != NULL)
	{
		if ((dpy = XOpenDisplay(displayname)) == NULL)
		{
			dpy = olddpy;
			return NULL;
		}
		root = DefaultRootWindow(dpy);
		screen = DefaultScreen(dpy);
		cmap = DefaultColormap(dpy, screen);
	}
	XSync(dpy, False);
	if (window == 0 && r.x == -32768)
	{
		Cursor c = XCreateFontCursor(dpy, XC_plus);
		if (XGrabPointer(dpy, root, False,
		                 PointerMotionMask | ButtonPressMask | ButtonReleaseMask,
		                 GrabModeAsync, GrabModeAsync, None,
		                 c, CurrentTime) != GrabSuccess)
			return NULL;
		Boolean done = False;
		Boolean drawing = False;

		MCRectangle trect;
		GC gc = XCreateGC(dpy, root, 0, NULL);
		XSetFunction(dpy, gc, GXxor);
		XSetForeground(dpy, gc, AllPlanes);
		XSetSubwindowMode(dpy, gc, IncludeInferiors);
		int2 startx = 0;
		int2 starty = 0;
		while (!done)
		{
			XEvent event;
			XNextEvent(dpy, &event);

			XKeyEvent *kpevent = (XKeyEvent *)&event;
			XMotionEvent *mevent = (XMotionEvent *)&event;
			XButtonEvent *bpevent = (XButtonEvent *)&event;
			XButtonEvent *brevent = (XButtonEvent *)&event;
			switch (event.type)
			{
			case Expose:
			case GraphicsExpose:
				XPutBackEvent(dpy, &event);
				MCscreen->expose();
				break;
			case KeyPress:
				MCeventtime = kpevent->time;
				if (XLookupKeysym(kpevent, ShiftMapIndex) == XK_Escape)
				{
					if (drawing)
					{
						XDrawRectangle(dpy, root, gc, trect.x, trect.y,
						               trect.width - 1, trect.height - 1);
						XUngrabServer(dpy);
					}
					done = True;
				}
				break;
			case MotionNotify:
				while (XCheckTypedWindowEvent(dpy, mevent->window,
				                              MotionNotify, &event))

					;
				MCeventtime = mevent->time;
				if (drawing)
				{
					XDrawRectangle(dpy, root, gc, trect.x, trect.y,
					               trect.width - 1, trect.height - 1);
					trect = MCU_compute_rect(startx, starty, mevent->x, mevent->y);
					XDrawRectangle(dpy, root, gc, trect.x, trect.y,

					               trect.width - 1, trect.height - 1);
				}
				break;
			case ButtonPress:
				XGrabServer(dpy);
				MCeventtime = bpevent->time;
				startx = bpevent->x;
				starty = bpevent->y;
				trect = MCU_compute_rect(startx, starty, startx, starty);
				XDrawRectangle(dpy, root, gc, trect.x, trect.y,
				               trect.width - 1, trect.height - 1);
				drawing = True;
				break;
			case ButtonRelease:
				MCeventtime = brevent->time;
				setmods(brevent->state, 0, brevent->button, True);
				XDrawRectangle(dpy, root, gc, trect.x, trect.y,
				               trect.width - 1, trect.height - 1);
				r = MCU_compute_rect(startx, starty, brevent->x, brevent->y);
				if (r.width < 4 && r.height < 4)
					r.width = r.height = 0;
				XUngrabServer(dpy);
				done = True;

				break;
			default:
				break;
			}
		}
		XUngrabPointer(dpy, CurrentTime);
		XFreeCursor(dpy, c);
		XFreeGC(dpy, gc);
		XFlush(dpy);
	}
	if (r.x == -32768)
		r.x = r.y = 0;
	if (window != 0 || r.width == 0 || r.height == 0)
	{
		int rx, ry, wx, wy;
		unsigned int kb;
		Window troot;
		Window child = root;
		if (window == 0)
		{
			XGrabServer(dpy);
			XQueryPointer(dpy, root, &troot, &child, &rx, &ry, &wx, &wy, &kb);
			int2 oldx = rx;
			int2 oldy = ry;
			XWarpPointer(dpy, None, getroot(), 0, 0, 0, 0, r.x, r.y);
			XQueryPointer(dpy, root, &troot, &child, &rx, &ry, &wx, &wy, &kb);
			if (child == None)
				child = troot;
			if (!(MCmodifierstate & MS_CONTROL))
			{
				Window oldchild = child;
				XQueryPointer(dpy, child, &troot, &child, &rx, &ry, &wx, &wy, &kb);
				if (child == None)
					child = oldchild;
			}
			XWarpPointer(dpy, None, getroot(), 0, 0, 0, 0, oldx, oldy);
			XUngrabServer(dpy);
		}
		else
			child = window;
		int x, y;
		unsigned int w, h, bw, depth;
		if (!XGetGeometry(dpy, child, &troot, &x, &y, &w, &h, &bw, &depth))
			return NULL;
		XWindowAttributes atts;
		if (XGetWindowAttributes(dpy, child, &atts) && atts.map_installed)
			cmap = atts.colormap;
		XTranslateCoordinates(dpy, child, root, 0, 0, &x, &y, &child);
		if (r.width == 0 || r.height == 0)
		{
			MCU_set_rect(r, x, y, w, h);
			r = MCU_clip_rect(r, 0, 0, DisplayWidth(dpy, screen),
			                  DisplayHeight(dpy, screen));
		}
		else
		{
			r.x += x;
			r.y += y;
		}
	}
	r = MCU_clip_rect(r, 0, 0, device_getwidth(), device_getheight());
	if (r.width == 0 || r.height == 0)
		return NULL;
	XImage *t_image = XGetImage(dpy, root, r.x, r.y, r.width, r.height, AllPlanes, ZPixmap);
	if (t_image == NULL)
		return NULL;
	t_image->red_mask = t_image->green_mask = t_image->blue_mask = 0;
	flipimage(t_image, MSBFirst, MSBFirst);
	if (dpy != olddpy)
	{
		XCloseDisplay(dpy);
		dpy = olddpy;
	}
	cmap = oldcmap;

	MCImageBitmap *t_bitmap = nil;
	/* UNCHECKED */ MCImageBitmapCreateWithXImage(t_image, t_bitmap);
	XDestroyImage(t_image);

	return t_bitmap;
}


void MCScreenDC::hidebackdrop(bool p_hide)
{
	if (!MChidebackdrop)
		return;

	if (!backdrop_active && !backdrop_hard)
		return;
	
	if ( backdrop == DNULL)
		return ;

	if ( p_hide )
	{
		XUnmapWindow(MCdpy, backdrop);
		MCstacks -> refresh() ;
	}
	else
	{
		
		Atom t_type, t_control ;
		t_type = 	XInternAtom(dpy, "_NET_WM_STATE_FULLSCREEN", false);
		t_control = XInternAtom(dpy, "_NET_WM_STATE", false);
				
		XChangeProperty(dpy, backdrop, t_control, XA_ATOM, 32, PropModeReplace, (unsigned char*)&t_type, 1);
		
		
		XMapWindow(dpy, backdrop);
		MCstacks -> refresh();

	}
}


void MCScreenDC::createbackdrop_window(void)
{
	XSetWindowAttributes xswa;
	unsigned long xswamask = CWBorderPixel | CWColormap;
	xswa.border_pixel = 0;
	xswa.colormap = cmap ;

	backdrop = XCreateWindow(dpy, getroot(), 0, 0,
	                         100,100,0,getrealdepth(), //getwidth(), getheight(), 0, getrealdepth(), // 
	                         InputOutput,getvisual(), xswamask, &xswa);
	
	XSelectInput(dpy, backdrop,  ButtonPressMask | ButtonReleaseMask | FocusChangeMask
	             | EnterWindowMask | LeaveWindowMask | PointerMotionMask);
	
}

	



void MCScreenDC::enablebackdrop(bool p_hard)
{

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
	
	t_error = ( backdrop == DNULL) ;
	
	if (!t_error)	
	{
		Atom t_type, t_control ;
		t_type = 	XInternAtom(dpy, "_NET_WM_STATE_FULLSCREEN", false);
		t_control = XInternAtom(dpy, "_NET_WM_STATE", false);
				
		XChangeProperty(dpy, backdrop, t_control, XA_ATOM, 32, PropModeReplace, (unsigned char*)&t_type, 1);
		
		XMapWindow(dpy, backdrop);
			
		MCstacks -> refresh();
	}
	else
	{
		backdrop_active = False;
		MCstacks -> refresh();
		//finalisebackdrop();
	}
}

void MCScreenDC::disablebackdrop(bool p_hard)
{	
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
		if ( backdrop != DNULL)
			XUnmapWindow(dpy, backdrop);
		MCstacks -> refresh();
	}
	
}



void MCScreenDC::configurebackdrop(const MCColor& p_colour, MCGImageRef p_pattern, MCImage *p_badge)
{
#ifdef LIBGRAPHICS_BROKEN
	if ( backdrop == DNULL ) 
		createbackdrop_window();
	
	char *cname = NULL;
	if (MCbackdroppm == DNULL &&
	        parsecolor(MCbackdropcolor, &backdropcolor, &cname))
	{
		delete cname;
		alloccolor(backdropcolor);
	}
	else
		backdropcolor.pixel = 0;
	

	XSetWindowAttributes xswa;
	unsigned long xswamask = CWBorderPixel | CWColormap;
	xswa.border_pixel = 0;
	xswa.colormap = cmap ;

	if (p_pattern != DNULL)
	{
		xswamask |= CWBackPixmap;
		xswa.background_pixmap = p_pattern;
	}
	else
	{
		xswamask |= CWBackPixel;
		xswa.background_pixel = backdropcolor.pixel;
	}

	if (backdrop != DNULL)
	{
		XChangeWindowAttributes(dpy, backdrop, xswamask, &xswa);
		XClearWindow(dpy, backdrop);
	}
	
	MCstacks -> refresh();
#endif
}


void MCScreenDC::assignbackdrop(Window_mode p_mode, Window p_window)
{
	if ( p_mode <= WM_PALETTE && backdrop != DNULL )
		XSetTransientForHint(dpy, p_window, backdrop);
}





void MCScreenDC::createbackdrop(const char *color)
{
#ifdef LIBGRAPHICS_BROKEN
	char *cname = NULL;
	if (MCbackdroppm == DNULL &&
	        parsecolor(color, &backdropcolor, &cname))
	{
		delete cname;
		alloccolor(backdropcolor);
	}
	else
		backdropcolor.pixel = 0;
	
	XSetWindowAttributes xswa;
	unsigned long xswamask = 0;
	if (MCbackdroppm != DNULL)
	{
		xswamask |= CWBackPixmap;
		xswa.background_pixmap = MCbackdroppm;
	}
	else
	{
		xswamask |= CWBackPixel;
		xswa.background_pixel = backdropcolor.pixel;
	}
	
	
	if (backdrop != DNULL)
	{
		XChangeWindowAttributes(dpy, backdrop, xswamask, &xswa);
		XClearWindow(dpy, backdrop);
		return;
	}
	
	
	xswamask |= CWBorderPixel | CWColormap | CWOverrideRedirect ;
	xswa.border_pixel = 0;
	xswa.colormap = cmap;
	xswa.override_redirect = True;
	//DH
	backdrop = XCreateWindow(dpy, RootWindow(dpy, vis->screen), 0, 0,
	                         device_getwidth(), device_getheight(), 0, vis->depth,
	                         InputOutput, vis->visual, xswamask, &xswa);
	
	XSelectInput(dpy, backdrop,  ButtonPressMask | ButtonReleaseMask
	             | EnterWindowMask | LeaveWindowMask | PointerMotionMask);
	
	
	XMapWindow(dpy, backdrop);
#endif
}


void MCScreenDC::destroybackdrop()
{
	if (backdrop != DNULL)
	{
		XUnmapWindow(dpy, backdrop);
		XDestroyWindow(dpy, backdrop);
		backdrop = DNULL;
		backdropcolor.pixel = 0;
	}
}






Bool MCScreenDC::is_composite_wm ( int screen_id ) 
{
	Atom t_atom ;
	char buf[100] ;
	memset(buf, 0, (sizeof(char))*100);	
	sprintf(buf,"_NET_WM_CM_S%d", screen_id ) ;
	t_atom = XInternAtom ( dpy, buf, False ) ;
	return ( XGetSelectionOwner ( dpy, t_atom ) != None ) ;
}







