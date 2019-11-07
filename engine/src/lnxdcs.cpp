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

#include "stacklst.h"

#include "sellst.h"

#include "globals.h"

#include "mctheme.h"

#include "lnxdc.h"
#include "lnximagecache.h"

#include "license.h"
#include "revbuild.h"

#include <langinfo.h>
#include <fcntl.h>
#include <sys/shm.h>

#include <libgnomevfs/gnome-vfs.h>
#include <libgnomevfs/gnome-vfs-utils.h>

namespace x11
{
#include <X11/extensions/Xv.h>
#include <X11/extensions/Xvlib.h>
}

#include <gdk/gdkkeysyms.h>

#undef STARTMSGS

#define REQUEST_SIZE 32   // size of request header for X calls
#define MAX_POINTS 8096U   // number of points in polygon
#define ICON_SIZE 48
#define XYCUTOFF 4        // plane based or packed pixel

////////////////////////////////////////////////////////////////////////////////

bool MCImageBitmapCreateWithGdkPixbuf(GdkPixbuf *p_image, MCImageBitmap *&r_bitmap);

////////////////////////////////////////////////////////////////////////////////

GdkDisplay *MCdpy;

////////////////////////////////////////////////////////////////////////////////

extern "C" int initialise_required_weak_link_X11();
extern "C" int initialise_required_weak_link_gobject();
extern "C" int initialise_required_weak_link_gdk();
extern "C" int initialise_required_weak_link_gdk_pixbuf();
extern "C" int initialise_required_weak_link_cairo();

extern "C" int initialise_weak_link_gtk(void);
extern "C" int initialise_weak_link_gtk_color_dialog(void);
extern "C" int initialise_weak_link_gtk_file_dialog(void);
extern "C" int initialise_weak_link_gtk_print_dialog(void);
extern "C" int initialise_weak_link_gnome_vfs ( void ) ;
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

void MCScreenDC::setstatus(MCStringRef status)
{
}

//XDND
void init_xDnD(void);

Drawable MCScreenDC::getdest()
{
	return dest;
}

extern "C"
{
void gtk_main_do_event(GdkEvent*);
}

void gdk_event_fn(GdkEvent *p_event, gpointer dc)
{
    // Let GTK know about the event (needed so it can drive its Save As
    // dialogues and the like)
    gtk_main_do_event(p_event);
    
    //fprintf(stderr, "gdk_event_fn\n");
    ((MCScreenDC*)dc)->EnqueueEvent(gdk_event_copy(p_event));
}

void gdk_event_fn_lost(void*)
{
    ;
}

// Functions for re-directing GTK IM context signals
static void on_commit(GtkIMContext *p_context, gchar *p_str, gpointer p_data)
{
    ((MCScreenDC*)p_data)->IME_OnCommit(p_context, p_str);
}
static gboolean on_delete_surrounding(GtkIMContext *p_context, gint p_offset, gint p_count, gpointer p_data)
{
    return ((MCScreenDC*)p_data)->IME_OnDeleteSurrounding(p_context, p_offset, p_count);
}
static void on_preedit_changed(GtkIMContext *p_context, gpointer p_data)
{
    ((MCScreenDC*)p_data)->IME_OnPreeditChanged(p_context);
}
static void on_preedit_end(GtkIMContext *p_context, gpointer p_data)
{
    ((MCScreenDC*)p_data)->IME_OnPreeditEnd(p_context);
}
static void on_preedit_start(GtkIMContext *p_context, gpointer p_data)
{
    ((MCScreenDC*)p_data)->IME_OnPreeditStart(p_context);
}
static void on_retrieve_surrounding(GtkIMContext *p_context, gpointer p_data)
{
    ((MCScreenDC*)p_data)->IME_OnRetrieveSurrounding(p_context);
}

Boolean MCScreenDC::open()
{
	// We require GDK in order to do any windowing at all
    initialise_required_weak_link_X11();
    initialise_required_weak_link_gobject();
    initialise_required_weak_link_gdk();
    initialise_required_weak_link_gdk_pixbuf();
    initialise_required_weak_link_cairo();
    
    gdk_init(0, NULL);
    //gdk_threads_init();
    
    // Check to see if we are in a UTF8 locale
	// TS : Changed 2008-01-08 as a more relaible way of testing for UTF-8
	MCutf8 = (strcmp(nl_langinfo(CODESET), "UTF-8") == 0)	;
	
	MCimagecache = new (nothrow) MCXImageCache ;
	
    if (MCdisplayname == NULL)
        MCdisplayname = gdk_get_display();
    
	if ((dpy = gdk_display_open(MCdisplayname)) == NULL)
	{
        MCAutoStringRefAsSysString t_cmd;
        t_cmd.Lock(MCcmd);
        fprintf(stderr, "%s: Can't open display %s\n",
                *t_cmd, MCdisplayname);
		return False;
	}

    // Build the class name
    MCAutoStringRef t_class_name;
    MCAutoStringRefAsUTF8String t_class_name_utf8;
    bool t_community;
    t_community = MClicenseparameters.license_class == kMCLicenseClassCommunity;
    
    /* UNCHECKED */ MCStringCreateMutable(0, &t_class_name);
    /* UNCHECKED */ MCStringAppendFormat(*t_class_name, "%s%s_%s", MCapplicationstring, t_community ? "community" : "", MC_BUILD_ENGINE_SHORT_VERSION);
    /* UNCHECKED */ MCStringFindAndReplaceChar(*t_class_name, '.', '_', kMCStringOptionCompareExact);
    /* UNCHECKED */ MCStringFindAndReplaceChar(*t_class_name, '-', '_', kMCStringOptionCompareExact);
    /* UNCHECKED */ t_class_name_utf8.Lock(*t_class_name);
    
    // Used to load the icon and other desktop properties
    gdk_set_program_class(*t_class_name_utf8);
    
    // The GLib event loop calls this function to respond to GDK events.
    // Unfortunately, when GTK gets initied, it will try to steal this from us.
    gdk_event_handler_set(&gdk_event_fn, gpointer(this), &gdk_event_fn_lost);
    
    x11::Display* XDisplay;
    XDisplay = x11::gdk_x11_display_get_xdisplay(dpy);
    
    GdkScreen *t_screen;
    t_screen = gdk_display_get_default_screen(dpy);
    
    {
        MCAutoStringRef t_displayname;
        MCNewAutoNameRef t_displayname_asname;
        if (MCStringCreateWithSysString(gdk_display_get_name(dpy),
                                        &t_displayname) &&
            MCNameCreate(*t_displayname, &t_displayname_asname))
        {
            displayname.Reset(t_displayname_asname.Take());
        }
    }
	{
        MCAutoStringRef t_vendor_string, t_vendor;
        MCNewAutoNameRef t_vendorname;
        if (MCStringCreateWithSysString(x11::XServerVendor(XDisplay), &t_vendor) &&
            MCStringFormat(&t_vendor_string, "%@ %d", *t_vendor,
                           x11::XVendorRelease(XDisplay)) &&
            MCNameCreate(*t_vendor_string, &t_vendorname))
        {
            vendorname.Reset(t_vendorname.Take());
        }
	}
	
#ifdef SYNCMODE
    // TODO: equivalent in GDK?
	XSynchronize(dpy, True);
	XSync ( dpy, False ) ;
#ifdef STARTMSGS
	fprintf(stderr, "Xserver sync on\n");
#endif
#endif
		
	//XDND - Need to set up the xDnD protocol so that we have access to the XdndAware atom.
	init_xDnD();
    
    if (MCvisualid)
    {
        // An explicit visual ID was specified on the command line. Get it.
        vis = x11::gdk_x11_screen_lookup_visual(t_screen, MCvisualid);
        
        if (vis == NULL)
        {
            MCAutoStringRefAsSysString t_cmd;
            t_cmd.Lock(MCcmd);
            fprintf(stderr, "%s: Bad visual id %x\n", *t_cmd, MCvisualid);
            MCvisualid = 0;
        }
        else
        {
            // What type of visual are we dealing with?
            switch (gdk_visual_get_visual_type(vis))
            {
                case GDK_VISUAL_STATIC_GRAY:
                case GDK_VISUAL_STATIC_COLOR:
                {
                    cmap = gdk_colormap_new(vis, FALSE);
                    setupcolors();
                    break;
                }
                    
                case GDK_VISUAL_TRUE_COLOR:
                {
                    cmap = gdk_colormap_new(vis, FALSE);
                    
                    gint t_redshift, t_greenshift, t_blueshift;
                    gint t_redbits, t_greenbits, t_bluebits;
                    gdk_visual_get_red_pixel_details(vis, NULL, &t_redshift, &t_redbits);
                    gdk_visual_get_green_pixel_details(vis, NULL, &t_greenshift, &t_greenbits);
                    gdk_visual_get_blue_pixel_details(vis, NULL, &t_blueshift, &t_bluebits);
                    redshift = t_redshift;
                    greenshift = t_greenshift;
                    blueshift = t_blueshift;
                    redbits = t_redbits;
                    greenbits = t_greenbits;
                    bluebits = t_bluebits;
                    
                    break;
                }
                    
                case GDK_VISUAL_DIRECT_COLOR:
                {
                    GdkColor defs[256];
                    cmap = gdk_colormap_new(vis, TRUE);
                    guint32 t_redmask, t_greenmask, t_bluemask;
                    gint t_redshift, t_greenshift, t_blueshift;
                    gint t_redbits, t_greenbits, t_bluebits;
                    gdk_visual_get_red_pixel_details(vis, &t_redmask, &t_redshift, &t_redbits);
                    gdk_visual_get_green_pixel_details(vis, &t_greenmask, &t_greenshift, &t_greenbits);
                    gdk_visual_get_blue_pixel_details(vis, &t_bluemask, &t_blueshift, &t_bluebits);
                    redshift = t_redshift;
                    greenshift = t_greenshift;
                    blueshift = t_blueshift;
                    redbits = t_redbits;
                    greenbits = t_greenbits;
                    bluebits = t_bluebits;
                    
                    uint4 r_scale = cmap_scale[redbits];
                    uint4 g_scale = cmap_scale[greenbits];
					uint4 b_scale = cmap_scale[bluebits];
					uint4 rmsk = (1 << redbits) - 1;
					uint4 gmsk = (1 << greenbits) - 1;
					uint4 bmsk = (1 << bluebits) - 1;
					for(int i = 0 ; i < gdk_visual_get_colormap_size(vis) ; i++)
					{
						defs[i].pixel = (i << redshift & t_redmask)
							| (i << greenshift & t_greenmask)
							| (i << blueshift & t_bluemask);
						defs[i].red   = (i & rmsk) * r_scale;
						defs[i].green = (i & gmsk) * g_scale;
						defs[i].blue  = (i & bmsk) * b_scale;
					}
                    gdk_colors_store(cmap, defs, gdk_visual_get_colormap_size(vis));
                }
                    
                case GDK_VISUAL_GRAYSCALE:
                {
                    // Create a grayscale colormap with a linear ramp
                    cmap = gdk_colormap_new(vis, TRUE);
                    setupcolors();
                    GdkColor *t_colors;
                    if (!MCMemoryNewArray(ncolors, t_colors))
                    {
                      return False;
                    }
                    
                    for (int i = 0 ; i < ncolors ; i++)
                    {
                        colors[i].red = colors[i].green = colors[i].blue = i * MAXUINT2 / ncolors;
                        t_colors[i].red = t_colors[i].green = t_colors[i].blue = i * MAXUINT2 / ncolors;
                        t_colors[i].pixel = i;
                    }
                    gdk_colors_store(cmap, t_colors, ncolors);
                    MCMemoryDelete(t_colors);
                    break;
                }
                    
                case GDK_VISUAL_PSEUDO_COLOR:
                {
                    // Each stack uses its own colormap
                    cmap = gdk_colormap_new(vis, FALSE);
                    setupcolors();
                    MCuseprivatecmap = true;
                    break;
                }
            }
        }
    }

	if (MCvisualid == 0)
	{
        // If the screen is composited, we can expect alpha values to work
        if (gdk_screen_is_composited(t_screen))
        {
            // Get an RGBA visual to use
#ifdef STARTMSGS
			fprintf(stderr, "Composite window manager detected. Trying to use RGBA visual.\n");
#endif
            vis = gdk_screen_get_rgba_visual(t_screen);
            cmap = gdk_screen_get_rgba_colormap(t_screen);
        }
        
        // If getting the RGBA visual fails, or we are not composited, then
        // use the default visual.
        if (vis == nullptr)
        {
            // Get the default visual and colormap
            vis = gdk_screen_get_system_visual(t_screen);
            cmap = gdk_screen_get_system_colormap(t_screen);
        }
        
        // If getting the system visual fails then use the rgb visual.
        if (vis == nullptr)
        {
            vis = gdk_screen_get_rgb_visual(t_screen);
            cmap = gdk_screen_get_rgb_colormap(t_screen);
        }

		if (gdk_visual_get_visual_type(vis) == GDK_VISUAL_TRUE_COLOR)
		{
            gint t_redshift, t_greenshift, t_blueshift;
            gint t_redbits, t_greenbits, t_bluebits;
            gdk_visual_get_red_pixel_details(vis, NULL, &t_redshift, &t_redbits);
            gdk_visual_get_green_pixel_details(vis, NULL, &t_greenshift, &t_greenbits);
            gdk_visual_get_blue_pixel_details(vis, NULL, &t_blueshift, &t_bluebits);
            redshift = t_redshift;
            greenshift = t_greenshift;
            blueshift = t_blueshift;
            redbits = t_redbits;
            greenbits = t_greenbits;
            bluebits = t_bluebits;
		}
		else
		{
			setupcolors();
		}
	}
	
    // Temporarily create a window so we can get a GC for vis
    GdkWindowAttr gdkwa;
    gdkwa.event_mask = GDK_ALL_EVENTS_MASK & ~GDK_POINTER_MOTION_HINT_MASK;
    gdkwa.x = gdkwa.y = gdkwa.width = gdkwa.height = 8;
    gdkwa.visual = vis;
    gdkwa.colormap = cmap;
    gdkwa.wclass = GDK_INPUT_OUTPUT;
    gdkwa.window_type = GDK_WINDOW_TOPLEVEL;
    
    GdkWindow *w = gdk_window_new(gdk_screen_get_root_window(t_screen),
                                  &gdkwa,
                                  GDK_WA_X|GDK_WA_Y|GDK_WA_VISUAL);
    gc = gdk_gc_new(w);
    gdk_window_destroy(w);
    
    // Create the NULL window
    NULLWindow = gdk_window_new(gdk_screen_get_root_window(t_screen),
                                &gdkwa,
                                GDK_WA_X|GDK_WA_Y|GDK_WA_VISUAL);

    GdkColor t_color;
    t_color.pixel = 0;
    gdk_gc_set_exposures(gc, FALSE);
    gdk_gc_set_background(gc, &t_color);

	black_pixel.red = black_pixel.green = black_pixel.blue = 0;
	white_pixel.red = white_pixel.green = white_pixel.blue = MAXUINT2;

	MCdpy = dpy;

    /*GdkPixmap *cdata = gdk_pixmap_new(getroot(), 16, 16, 1);
    GdkPixmap *cmask = gdk_pixmap_new(getroot(), 16, 16, 1);
    gdk_gc_set_foreground(gc1, &t_color);
    gdk_draw_rectangle(cdata, gc1, TRUE, 0, 0, 16, 16);
    gdk_draw_rectangle(cmask, gc1, TRUE, 0, 0, 16, 16);
    t_color.pixel = 1;
    gdk_gc_set_foreground(gc1, &t_color);*/
    
    MCColor c;
    c.red = c.green = c.blue = 0x0;
    
    for (uint32_t i = 0; i < PI_NCURSORS; i++)
        MCcursors[i] = nil;
    
    //g_object_unref(cdata);
    //g_object_unref(cmask);
    
	MConecolor.red = MConecolor.green = MConecolor.blue = 0xFFFF;
	MCselectioncolor = MCpencolor = black_pixel;
	MCbrushcolor = white_pixel;
	MCaccentcolor = MChilitecolor;
	gray_pixel.red = gray_pixel.green = gray_pixel.blue = 0x8080;

	background_pixel.red = background_pixel.green = background_pixel.blue = 0xdcdc;
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
	
	m_has_native_theme = m_has_gtk = initialise_weak_link_gtk() != 0;
	
	if (m_has_native_theme)
	{
		m_has_native_color_dialogs = initialise_weak_link_gtk_color_dialog() != 0;
		m_has_native_file_dialogs = initialise_weak_link_gtk_file_dialog() != 0;
		m_has_native_print_dialogs = initialise_weak_link_gtk_print_dialog() != 0;
	}

    // If we have GTK, we can make use of the GTK IME support
    if (m_has_gtk)
    {
        m_im_context = gtk_im_multicontext_new();
        
        // Set up the signals for the IM context
        g_signal_connect(m_im_context, "commit", G_CALLBACK(&on_commit), this);
        g_signal_connect(m_im_context, "delete-surrounding", G_CALLBACK(&on_delete_surrounding), this);
        g_signal_connect(m_im_context, "preedit-changed", G_CALLBACK(&on_preedit_changed), this);
        g_signal_connect(m_im_context, "preedit-end", G_CALLBACK(&on_preedit_end), this);
        g_signal_connect(m_im_context, "preedit-start", G_CALLBACK(&on_preedit_start), this);
        g_signal_connect(m_im_context, "retrieve-surrounding", G_CALLBACK(&on_retrieve_surrounding), this);
    }
		
	if ( initialise_weak_link_gnome_vfs() != 0 )
	{
		MCuselibgnome = initialise_weak_link_libgnome();
		gnome_vfs_init();
	}
    
    // There are also some atoms that we need to set up
    MCworkareaatom = gdk_atom_intern_static_string("_NET_WORKAREA");
    MCclientlistatom = gdk_atom_intern_static_string("_NET_CLIENT_LIST");
    MCstrutpartialatom = gdk_atom_intern_static_string("_NET_WM_STRUT_PARTIAL");
    MCdndselectionatom = gdk_atom_intern_static_string("XdndSelection");

	return True; 
}

// Defined in xans.cpp
extern void gtk_file_tidy_up ( void );



// Returns an XAtom with the given name ;
Atom  MCScreenDC::make_atom ( char * p_atom_name ) 
{
	return gdk_atom_intern(p_atom_name, FALSE);
}

//XDND
extern void MCLinuxDragAndDropFinalize();

Boolean MCScreenDC::close(Boolean force)
{
	// TODO - We may need to do clipboard persistance here
	
	destroybackdrop();
    gdk_window_destroy(NULLWindow);
    
    gdk_display_flush(dpy);
    g_object_unref(gc);

	//XDND
	MCLinuxDragAndDropFinalize();
	
    gdk_display_close(dpy);
	
	delete (char *)selectiontext.getstring();
	opened = False;
	
	gtk_file_tidy_up () ;

	
	delete MCimagecache ;
	
	return True;
}

MCNameRef MCScreenDC::getdisplayname()
{
	return *displayname;
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
    gdk_pointer_grab(w, False,
                     GdkEventMask(GDK_POINTER_MOTION_MASK|GDK_BUTTON_PRESS_MASK|GDK_BUTTON_RELEASE_MASK),
                     NULL, NULL, MCeventtime);
}

void MCScreenDC::ungrabpointer()
{
	gdk_display_pointer_ungrab(dpy, MCeventtime);
}

// IM-2014-01-29: [[ HiDPI ]] Placeholder method for Linux HiDPI support
uint16_t MCScreenDC::platform_getwidth(void)
{
	return device_getwidth();
}

uint16_t MCScreenDC::device_getwidth(void)
{
	return gdk_screen_get_width(getscreen());
}

// IM-2014-01-29: [[ HiDPI ]] Placeholder method for Linux HiDPI support
uint16_t MCScreenDC::platform_getheight(void)
{
	return device_getheight();
}

uint16_t MCScreenDC::device_getheight(void)
{
	return gdk_screen_get_height(getscreen());
}

uint2 MCScreenDC::getwidthmm()
{
	return gdk_screen_get_width_mm(getscreen());
}

uint2 MCScreenDC::getheightmm()
{
	return gdk_screen_get_height_mm(getscreen());
}

// MW-2005-09-24: We shouldn't be accessing the display structure like this
//   so use the XMaxRequestSize() call instead
uint2 MCScreenDC::getmaxpoints()
{
	return MCU_min(x11::XMaxRequestSize(x11::gdk_x11_display_get_xdisplay(dpy)) - REQUEST_SIZE, MAX_POINTS);
}

uint2 MCScreenDC::getvclass()
{
	switch (gdk_visual_get_visual_type(vis))
    {
        case GDK_VISUAL_STATIC_GRAY:
            return StaticGray;
            
        case GDK_VISUAL_GRAYSCALE:
            return GrayScale;
            
        case GDK_VISUAL_STATIC_COLOR:
            return StaticColor;
            
        case GDK_VISUAL_PSEUDO_COLOR:
            return PseudoColor;
            
        case GDK_VISUAL_TRUE_COLOR:
            return TrueColor;
            
        case GDK_VISUAL_DIRECT_COLOR:
            return DirectColor;
            
        default:
            MCAssert(false);
            return 0;
    }
}

void MCScreenDC::openwindow(Window window, Boolean override)
{
	MCStack *target = MCdispatcher->findstackd(window);
    gdk_window_show(window);
	MCstacks -> enableformodal(window, False);
}

void MCScreenDC::closewindow(Window window)
{
	MCStack *target = MCdispatcher->findstackd(window);
	MCstacks -> enableformodal(window, True);
    gdk_window_hide(window);
}

void MCScreenDC::destroywindow(Window &window)
{
	gdk_window_destroy(window);
	window = DNULL;
}

void MCScreenDC::raisewindow(Window window)
{
	gdk_window_raise(window);
}

void MCScreenDC::iconifywindow(Window window)
{
	gdk_window_iconify(window);
}

void MCScreenDC::uniconifywindow(Window window)
{
	gdk_window_deiconify(window);
}

void MCScreenDC::setname(Window window, MCStringRef newname)
{
	MCAutoStringRefAsUTF8String t_newname_utf8;
	/* UNCHECKED */ t_newname_utf8 . Lock(newname);
	
	gdk_window_set_title(window, *t_newname_utf8);
}

void MCScreenDC::setcmap(MCStack *sptr)
{
	//gdk_drawable_set_colormap(sptr->getw(), getcmap());
}

void MCScreenDC::sync(Window w)
{
	gdk_display_sync(dpy);
}

void MCScreenDC::flush(Window w)
{
	gdk_display_flush(dpy);
}

void MCScreenDC::beep()
{
	gdk_beep();
}

void MCScreenDC::setinputfocus(Window window)
{
	gdk_window_focus(window, MCeventtime);
}

void MCScreenDC::freepixmap(Pixmap &pixmap)
{
	if (pixmap != DNULL)
	{
		g_object_unref(pixmap);
		pixmap = DNULL;
	}
}

Pixmap MCScreenDC::createpixmap(uint2 width, uint2 height,
                                uint2 depth, Boolean purge)
{
	if (depth == 0 || depth == 32)
	{
		if (gdk_visual_get_depth(vis) == 24)
			depth = 24;
		else
			depth = 32;
	}
	
    GdkPixmap *pm = gdk_pixmap_new(getroot(), width, height, depth);
	assert ( pm != DNULL ) ;
	
	return pm;
}

// IM-2014-01-29: [[ HiDPI ]] Placeholder method for Linux HiDPI support
bool MCScreenDC::platform_getwindowgeometry(Window w, MCRectangle &r_rect)
{
	return device_getwindowgeometry(w, r_rect);
}

bool MCScreenDC::device_getwindowgeometry(Window w, MCRectangle &drect)
{
	Window root, child;
	gint x, y;
	gint width, height;
    
    gdk_window_get_geometry(w, NULL, NULL, &width, &height, NULL);
    gdk_window_get_origin(w, &x, &y);
    
	MCU_set_rect(drect, x, y, width, height);
	return true;
}


Boolean MCScreenDC::getpixmapgeometry(Pixmap p, uint2 &w, uint2 &h, uint2 &d)
{
    gint width, height;
    gdk_pixmap_get_size(p, &width, &height);
    
    w = width;
    h = height;
    d = gdk_drawable_get_depth(p);
    
	return True;
}

void MCScreenDC::setgraphicsexposures(Boolean on, MCStack *sptr)
{
	gdk_gc_set_exposures(gc, TRUE);
}

GdkFunction MCScreenDC::XOpToGdkOp(int op)
{
    switch (op)
    {
        case GXcopy:
            return GDK_COPY;
            
        case GXinvert:
            return GDK_INVERT;
            
        case GXxor:
            return GDK_XOR;
            
        case GXclear:
            return GDK_CLEAR;
            
        case GXand:
            return GDK_AND;
            
        case GXandReverse:
            return GDK_AND_REVERSE;
            
        case GXandInverted:
            return GDK_AND_INVERT;
            
        case GXnoop:
            return GDK_NOOP;
            
        case GXor:
            return GDK_OR;
            
        case GXequiv:
            return GDK_EQUIV;
            
        case GXorReverse:
            return GDK_OR_REVERSE;
            
        case GXcopyInverted:
            return GDK_COPY_INVERT;
            
        case GXorInverted:
            return GDK_OR_INVERT;
            
        case GXnand:
            return GDK_NAND;
            
        case GXnor:
            return GDK_NOR;
            
        case GXset:
            return GDK_SET;
    }
    
    MCAssert(false);
    return GDK_COPY;
}

void MCScreenDC::copyarea(Drawable s, Drawable d, int2 depth,
                          int2 sx, int2 sy,  uint2 sw, uint2 sh, int2 dx,
                          int2 dy, uint4 rop)
{
	if (s == nil || d == nil)
		return;
	
    GdkGC *t_gc;
    t_gc = gdk_gc_new(d);
    
    assert(rop <= GXset);
    
    if (rop != GXcopy)
        gdk_gc_set_function(t_gc, XOpToGdkOp(rop));
    
    gdk_draw_drawable(d, t_gc, s, sx, sy, dx, dy, sw, sh);
    
    if (rop != GXcopy)
        gdk_gc_set_function(t_gc, GDK_COPY);

    g_object_unref(t_gc);
}

MCBitmap *MCScreenDC::createimage(uint16_t depth, uint16_t width, uint16_t height, bool set, uint8_t value)
{
    // NOTE:
    //  The specified depth is completely ignored and a 32-bit RGBA image is
    //  always created. GdkPixbuf doesn't work well with 1-bit images.
    
    // Create a new bitmap
    GdkPixbuf *t_image;
    t_image = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, width, height);
    
    if (set && t_image != nil)
        memset(gdk_pixbuf_get_pixels(t_image), value, gdk_pixbuf_get_byte_length(t_image));
    
    return (MCBitmap*)t_image;
}

void MCScreenDC::destroyimage(MCBitmap *image)
{
    g_object_unref(image);
}

void MCScreenDC::putimage(Drawable d, MCBitmap *source, int2 sx, int2 sy,
                          int2 dx, int2 dy, uint2 w, uint2 h)
{
	if (d == nil)
		return;

    // If we use gdk_draw_pixbuf, the pixbuf gets blended with the existing
    // contents of the window - something that we definitely do not want. We
    // need to use Cairo directly to do the drawing to the window surface.
    cairo_t *t_cr = gdk_cairo_create(d);
    cairo_set_operator(t_cr, CAIRO_OPERATOR_SOURCE);
    cairo_rectangle(t_cr, dx, dy, w, h);
    cairo_clip(t_cr);
    gdk_cairo_set_source_pixbuf(t_cr, source, dx-sx, dy-sy);
    cairo_paint(t_cr);
    cairo_destroy(t_cr);
}

MCBitmap *MCScreenDC::getimage(Drawable d, int2 x, int2 y, uint2 w, uint2 h)
{
	GdkPixbuf *t_image;
    t_image = gdk_pixbuf_get_from_drawable(NULL, d, NULL, x, y, 0, 0, w, h);
    return (MCBitmap*)t_image;
}

void MCScreenDC::flipimage(MCBitmap *image, int2 byte_order, int2 bit_order)
{
    // Not needed
}

void MCScreenDC::setfunction(uint4 rop)
{
	if (rop < 0x10)
        gdk_gc_set_function(getgc(), XOpToGdkOp(rop));
}

uintptr_t MCScreenDC::dtouint(Drawable d)
{
	// Return the XID
    return d != DNULL ? x11::gdk_x11_drawable_get_xid(d) : 0;
}

Boolean MCScreenDC::uinttowindow(uintptr_t id, Window &w)
{
    // Look up the XID in GDK's window table
    w = x11::gdk_x11_window_lookup_for_display(dpy, id);
	return True;
}

void MCScreenDC::getbeep(uint4 which, int4& r_value)
{
    // There doesn't seem to be a way to do this with GDK...
    x11::XKeyboardState values;
    x11::XGetKeyboardControl(x11::gdk_x11_display_get_xdisplay(dpy), &values);
	switch (which)
	{
	case P_BEEP_LOUDNESS:
		r_value = values.bell_percent;
		break;
	case P_BEEP_PITCH:
		r_value = values.bell_pitch;
		break;
	case P_BEEP_DURATION:
		r_value = values.bell_duration;
		break;
	}
}


void MCScreenDC::setbeep(uint4 which, int4 beep)
{
	// There doesn't seem to be a way to do this with GDK...
    x11::XKeyboardControl control;
	switch (which)
	{
	case P_BEEP_LOUDNESS:
		beep = MCU_min(beep, 100);
		control.bell_percent = beep;
        x11::XChangeKeyboardControl(x11::gdk_x11_display_get_xdisplay(dpy), KBBellPercent, &control);
		break;
	case P_BEEP_PITCH:
		control.bell_pitch = beep;
		x11::XChangeKeyboardControl(x11::gdk_x11_display_get_xdisplay(dpy), KBBellPitch, &control);
		break;
	case P_BEEP_DURATION:
		control.bell_duration = beep;
		x11::XChangeKeyboardControl(x11::gdk_x11_display_get_xdisplay(dpy), KBBellDuration, &control);

		break;
	}
}

MCNameRef MCScreenDC::getvendorname(void)
{
	return *vendorname;
}

uint2 MCScreenDC::getpad()
{
	// No longer seems to be necessary
    return 32;
}

Window MCScreenDC::getroot()
{
	return gdk_screen_get_root_window(gdk_display_get_default_screen(dpy));
}

MCImageBitmap *MCScreenDC::snapshot(MCRectangle &r, uint4 window, MCStringRef displayname, MCPoint *size)
{
	GdkDisplay* t_display;
    GdkScreen* t_screen;
    GdkWindow* t_root;
    
    // Are we connecting to a different display?
    if (displayname != nil)
    {
        // Create a new connection and get the corresponding screen and root
        MCAutoStringRefAsSysString t_displayname;
        t_displayname.Lock(displayname);
        
        if ((t_display = gdk_display_open(*t_displayname)) == NULL)
        {
            // Could not open the requested display
            return NULL;
        }
        
        t_screen = gdk_display_get_default_screen(t_display);
        t_root = gdk_screen_get_root_window(t_screen);
    }
    else
    {
        // Use the current connection
        t_display = dpy;
        t_screen = gdk_display_get_default_screen(t_display);
        t_root = gdk_screen_get_root_window(t_screen);
    }
    
    // Ensure that we are up-to-date with the window server
    gdk_display_sync(t_display);
    
    // Are we going to display a selection rectangle?
    if (window == 0 && r.x == -32768)
    {
        // Switch to a box drawing cursor and take control of the pointer
        GdkCursor *t_cursor = gdk_cursor_new(GDK_PLUS);
        if (gdk_pointer_grab(t_root, False,
                             GdkEventMask(GDK_POINTER_MOTION_MASK|GDK_BUTTON_PRESS_MASK|GDK_BUTTON_RELEASE_MASK),
                             NULL, t_cursor, GDK_CURRENT_TIME) != GDK_GRAB_SUCCESS)
        {
            // Could not grab the pointer
            return NULL;
        }
        
        // Prepare for drawing the selection rectangle
        MCRectangle t_rect(kMCEmptyRectangle);
        GdkColor t_color;
        t_color.pixel = ~0;
        GdkGC *t_gc = gdk_gc_new(t_root);
        gdk_gc_set_function(t_gc, GDK_XOR);
        gdk_gc_set_foreground(t_gc, &t_color);
        gdk_gc_set_subwindow(t_gc, GDK_INCLUDE_INFERIORS);
        int16_t t_start_x = 0;
        int16_t t_start_y = 0;
        bool t_drawing = false;
        bool t_done = false;
        
        // Minature event loop for handling mouse and key events while selecting
        while (!t_done)
        {
            gdk_display_sync(t_display);
            
            // Place all events onto the pending event queue
            EnqueueGdkEvents();
            
            bool t_queue = false;
            GdkEvent *t_event = NULL;
            if (pendingevents != NULL)
            {
                // Get the next event from the queue
                t_event = gdk_event_copy(pendingevents->event);
                MCEventnode *tptr = (MCEventnode *)pendingevents->remove(pendingevents);
                delete tptr;
            }
            
            // If there are no events, actively wait for one
            if (t_event == NULL)
            {
                g_main_context_iteration(NULL, TRUE);
                continue;
            }
            
            // Various type casts of the event structure
            GdkEventKey *t_event_key = (GdkEventKey*)t_event;
            GdkEventMotion *t_event_motion = (GdkEventMotion*)t_event;
            GdkEventButton *t_event_button = (GdkEventButton*)t_event;
            
            // What is the event?
            switch (t_event->type)
            {
                case GDK_EXPOSE:
                    // Put the event back on the queue and do an expose
                    gdk_event_put(t_event);
                    MCscreen->expose();
                    break;
                    
                case GDK_KEY_PRESS:
                    MCeventtime = gdk_event_get_time(t_event);
                    if (t_event_key->keyval == GDK_KEY_Escape)
                    {
                        if (t_drawing)
                        {
                            // Draw the selection rectangle and release the server
                            gdk_draw_rectangle(t_root, t_gc, FALSE, t_rect.x, t_rect.y, t_rect.width - 1, t_rect.height - 1);
                            x11::gdk_x11_ungrab_server();
                        }
                        
                        // End the selection
                        t_done = true;
                    }
                    break;
                    
                case GDK_MOTION_NOTIFY:
                    MCeventtime = gdk_event_get_time(t_event);
                    if (t_drawing)
                    {
                        // Update the selection rectangle
                        gdk_draw_rectangle(t_root, t_gc, FALSE, t_rect.x, t_rect.y, t_rect.width - 1, t_rect.height - 1);
                        t_rect = MCU_compute_rect(t_start_x, t_start_y, t_event_motion->x, t_event_motion->y);
                        gdk_draw_rectangle(t_root, t_gc, FALSE, t_rect.x, t_rect.y, t_rect.width - 1, t_rect.height - 1);
                    }
                    break;
                    
                case GDK_BUTTON_PRESS:
                    x11::gdk_x11_grab_server();
                    MCeventtime = gdk_event_get_time(t_event);
                    t_start_x = t_event_button->x;
                    t_start_y = t_event_button->y;
                    t_rect = MCU_compute_rect(t_start_x, t_start_y, t_start_x, t_start_y);
                    gdk_draw_rectangle(t_root, t_gc, FALSE, t_rect.x, t_rect.y, t_rect.width - 1, t_rect.height - 1);
                    t_drawing = true;
                    break;
                    
                case GDK_BUTTON_RELEASE:
                    MCeventtime = gdk_event_get_time(t_event);
                    setmods(t_event_button->state, 0, t_event_button->button, True);
                    gdk_draw_rectangle(t_root, t_gc, FALSE, t_rect.x, t_rect.y, t_rect.width - 1, t_rect.height - 1);
                    r = MCU_compute_rect(t_start_x, t_start_y, t_event_button->x, t_event_button->y);
                    if (r.width < 4 && r.height < 4)
                        r.width = r.height = 0;
                    x11::gdk_x11_ungrab_server();
                    t_done = true;
                    break;
                    
                case GDK_GRAB_BROKEN:
                    t_done = true;
                    break;

				default:
					/* Ignore this event */
					break;
            }
            
            // The event needs to be released
            gdk_event_free(t_event);
        }
        
        // Release the grabs and other resources that were acquired
        gdk_display_pointer_ungrab(dpy, GDK_CURRENT_TIME);
        gdk_cursor_unref(t_cursor);
        g_object_unref(t_gc);
        gdk_display_flush(t_display);
    }
    if (r.x == -32768)
    {
        r.x = r.y = 0;
    }
    if (window != 0 || r.width == 0 || r.height == 0)
    {
        int rx, ry, wx, wy;
        unsigned int kb;
        GdkWindow *t_child;
        if (window == 0)
        {
            // This is done using Xlib calls because there are no suitable
            // equivalents in GDK (it can only return windows beloning to this
            // application when mapping coordinates to a window).
            x11::Display* dpy;
            dpy = x11::gdk_x11_display_get_xdisplay(t_display);
            x11::Window root;
            root = x11::gdk_x11_drawable_get_xid(t_root);
            
            // Grab the server to prevent pointer movement during this operation
            x11::XGrabServer(dpy);
            
            // Get the window under the pointer
            x11::Window troot, child;
            x11::XQueryPointer(dpy, root, &troot, &child, &rx, &ry, &wx, &wy, &kb);
            
            // Warp the pointer to the coordinates specified by the user and get
            // the child window at that point
			int2 oldx = rx;
			int2 oldy = ry;
            x11::XWarpPointer(dpy, None, x11::gdk_x11_drawable_get_xid(getroot()), 0, 0, 0, 0, r.x, r.y);
            x11::XQueryPointer(dpy, root, &troot, &child, &rx, &ry, &wx, &wy, &kb);
			
            // If no child window, use the screen's root window instead
            if (child == None)
				child = troot;
            
            // If the control key is not held down, do the query again
            //
            // ??? Why is this here? I can see it being necessary in the case
            // that root and troot are not on the same screen but why does it
            // depend on whether the control key is held down...?
			if (!(MCmodifierstate & MS_CONTROL))
			{
                x11::Window oldchild = child;
                x11::XQueryPointer(dpy, child, &troot, &child, &rx, &ry, &wx, &wy, &kb);
				if (child == None)
					child = oldchild;
			}
            
            // Convert the X11 window to a GDK window
            t_child = x11::gdk_x11_window_foreign_new_for_display(t_display, child);
            
            // Restore the pointer location and ungrab the server
            x11::XWarpPointer(dpy, None, x11::gdk_x11_drawable_get_xid(getroot()), 0, 0, 0, 0, oldx, oldy);
            x11::XUngrabServer(dpy);
        }
        else
        {
            // Convert the X11 window to a GDK window
            t_child = x11::gdk_x11_window_foreign_new_for_display(t_display, window);
        }
        
        // Get the position and dimensions of the window
        gint x, y;
        unsigned int w, h;
        w = gdk_window_get_width(t_child);
        h = gdk_window_get_height(t_child);
        gdk_window_get_origin(t_child, &x, &y);
        
        // Are we wanting the whole window or just a specified rectangle?
        if (r.width == 0 || r.height == 0)
        {
            // Whole window
            MCU_set_rect(r, x, y, w, h);
            r = MCU_clip_rect(r, 0, 0, gdk_screen_get_width(t_screen), gdk_screen_get_height(t_screen));
        }
        else
        {
            // Rectangle within window
            r.x += x;
            r.y += y;
        }
        
        // Release the reference to the child window that we created
        g_object_unref(t_child);
    }
    
    // Clip to the device boundaries
    r = MCU_clip_rect(r, 0, 0, device_getwidth(), device_getheight());
    
    // Nothing more to do if the rectangle is degenerate
    if (r.width == 0 || r.height == 0)
        return NULL;
    
    // Get the snapshot from the root window.
    // FG-2014-08-05: [[ Bugfix 13032 ]]
    // We need to pre-create the pixbuf to ensure we get one with alpha.
    GdkPixbuf *t_image;
    t_image = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, r.width, r.height);
    t_image = gdk_pixbuf_get_from_drawable(t_image, t_root, NULL, r.x, r.y, 0, 0, r.width, r.height);
    
    // Abort if the pixbuf could not be captured
    if (t_image == NULL)
        return NULL;
    
    // Close the connection to the display
    if (displayname != nil)
    {
        gdk_display_close(t_display);
    }
    
    // Turn the GDK pixbuf into an engine bitmap
    MCImageBitmap *t_bitmap = nil;
    /* UNCHECKED */ MCImageBitmapCreateWithGdkPixbuf(t_image, t_bitmap);
    g_object_unref(t_image);
    
    // Do any scaling that is required to satisfy the specified size
	if (size != nil && 
	    ((uint32_t) size -> x != t_bitmap -> width ||
	     (uint32_t) size -> y != t_bitmap -> height))
	{
		MCImageBitmap *t_new_bitmap;
		MCImageScaleBitmap(t_bitmap, size -> x, size -> y, INTERPOLATION_BILINEAR, t_new_bitmap);
		MCImageFreeBitmap(t_bitmap);
		t_bitmap = t_new_bitmap;
	}

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
		disablebackdrop(false);
	}
	else
	{
		enablebackdrop(false);
	}
}


void MCScreenDC::createbackdrop_window(void)
{
	GdkWindowAttr gdkwa;
    gdkwa.event_mask = GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | GDK_FOCUS_CHANGE_MASK
        | GDK_POINTER_MOTION_MASK | GDK_ENTER_NOTIFY_MASK | GDK_LEAVE_NOTIFY_MASK | GDK_EXPOSURE_MASK;
    gdkwa.x = gdkwa.y = 0;
    gdkwa.width = gdkwa.height = 100;
    gdkwa.wclass = GDK_INPUT_OUTPUT;
    gdkwa.visual = getvisual();
    gdkwa.colormap = getcmap();
    gdkwa.window_type = GDK_WINDOW_TOPLEVEL;
    
    backdrop = gdk_window_new(getroot(), &gdkwa, GDK_WA_VISUAL);
    gdk_display_sync(MCdpy);
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
	
	t_error = (backdrop == DNULL) ;
	
	if (!t_error)	
	{
        gdk_window_set_functions(backdrop, GdkWMFunction(0));
        gdk_window_set_decorations(backdrop, GdkWMDecoration(0));
        gdk_window_set_skip_taskbar_hint(backdrop, TRUE);
        gdk_window_set_skip_pager_hint(backdrop, TRUE);
        gdk_window_move_resize(backdrop, 0, 0, device_getwidth(), device_getheight());
        gdk_window_lower(backdrop);
        gdk_window_show_unraised(backdrop);
	}
	else
	{
		backdrop_active = False;
		//finalisebackdrop();
	}
    // MDW [17323] - refresh *after* the gdk calls
    MCstacks -> refresh();
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
		if (backdrop != DNULL)
			gdk_window_hide(backdrop);
		MCstacks -> refresh();
	}
	
}

bool MCPatternToX11Pixmap(MCPatternRef p_pattern, Pixmap &r_pixmap);
void MCScreenDC::configurebackdrop(const MCColor& p_colour, MCPatternRef p_pattern, MCImage *p_badge)
{
	// IM-2014-04-15: [[ Bug 11603 ]] Convert pattern to Pixmap for use with XSetWindowAttributes structure
	freepixmap(m_backdrop_pixmap);
    m_backdrop_pixmap = NULL;
	
	if (p_pattern != nil)
		/* UNCHECKED */ MCPatternToX11Pixmap(p_pattern, m_backdrop_pixmap);
		
	if ( backdrop == DNULL ) 
		createbackdrop_window();
	
    if (m_backdrop_pixmap == nil)
    {
        backdropcolor = p_colour;
    }
	else
		MCColorSetPixel(backdropcolor, 0);
	
    if (backdrop == DNULL)
        return;
    
	if (m_backdrop_pixmap != DNULL)
	{
		gdk_window_set_back_pixmap(backdrop, m_backdrop_pixmap, FALSE);
	}
	else
	{
		GdkColor t_colour;
        t_colour.red = p_colour.red;
        t_colour.green = p_colour.green;
        t_colour.blue = p_colour.blue;
        gdk_rgb_find_color(gdk_drawable_get_colormap(backdrop), &t_colour);
        gdk_window_set_background(backdrop, &t_colour);
	}
    
    gdk_window_clear(backdrop);

	MCstacks -> refresh();
}


void MCScreenDC::assignbackdrop(Window_mode p_mode, Window p_window)
{
	if (p_mode <= WM_PALETTE && backdrop != DNULL)
    {
        if (backdrop_active||backdrop_hard)
            gdk_window_set_transient_for(p_window, backdrop);
        else
            gdk_property_delete(p_window, gdk_atom_intern_static_string("WM_TRANSIENT_FOR"));
    }
}


/*void MCScreenDC::createbackdrop(MCStringRef color)
void MCScreenDC::createbackdrop(MCStringRef color)
{
	if (m_backdrop_pixmap == DNULL &&
            parsecolor(color, backdropcolor))
        alloccolor(backdropcolor);
	else
		backdropcolor.pixel = 0;
	
    GdkWindowAttr t_wa;
    t_wa.x = t_wa.y = 0;
    t_wa.width = device_getwidth();
    t_wa.height = device_getheight();
    t_wa.wclass = GDK_INPUT_OUTPUT;
    t_wa.visual = vis;
    t_wa.window_type = GDK_WINDOW_TOPLEVEL;
    t_wa.override_redirect = TRUE;
    t_wa.event_mask = GDK_BUTTON_PRESS_MASK|GDK_BUTTON_RELEASE_MASK|GDK_ENTER_NOTIFY_MASK
        |GDK_LEAVE_NOTIFY_MASK|GDK_POINTER_MOTION_MASK|GDK_EXPOSURE_MASK;
    
    backdrop = gdk_window_new(NULL, &t_wa, GDK_WA_X|GDK_WA_Y|GDK_WA_VISUAL);
    
	if (m_backdrop_pixmap != DNULL)
	{
		gdk_window_set_back_pixmap(backdrop, m_backdrop_pixmap, FALSE);
	}
	else
	{
		GdkColor t_colour;
        t_colour.red = backdropcolor.red;
        t_colour.green = backdropcolor.green;
        t_colour.blue = backdropcolor.blue;
        gdk_rgb_find_color(gdk_drawable_get_colormap(backdrop), &t_colour);
        gdk_window_set_background(backdrop, &t_colour);
	}
    
    gdk_window_show_unraised(backdrop);
}*/


void MCScreenDC::destroybackdrop()
{
	if (backdrop != DNULL)
	{
		gdk_window_hide(backdrop);
        gdk_window_destroy(backdrop);
		backdrop = DNULL;
	}
	
	freepixmap(m_backdrop_pixmap);
}


Bool MCScreenDC::is_composite_wm ( int screen_id ) 
{
    return gdk_display_supports_composite(dpy);
}
