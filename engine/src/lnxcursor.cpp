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

#include "lnxprefix.h"

#include "globdefs.h"
#include "objdefs.h"
#include "parsedef.h"
#include "filedefs.h"

#include "globals.h"
#include "dispatch.h"
#include "image.h"

#include "lnxdc.h"

#include <gtk/gtk.h>

const guint kMCDefaultCursorSize = 64;

////////////////////////////////////////////////////////////////////////////////

typedef void (*g_object_getPTR) (void *widget, const gchar *first_property_name, ...);
extern g_object_getPTR g_object_get_ptr;

////////////////////////////////////////////////////////////////////////////////

// The <id> field holds the PI_* index of the required cursor. The handle is
// the XCursor that we want to use. If <handle> is nil, then it means this
// cursor has yet to be resolved (will be nil initially for all 'standard'
// cursors).
struct MCCursor
{
	Pixmap_ids id;
	GdkCursor *handle;
};

static MCCursorRef create_cursor(Pixmap_ids p_id, GdkCursor *p_cursor)
{
	MCCursorRef t_cursor;
	t_cursor = new (nothrow) MCCursor;
	t_cursor -> id = p_id;
	t_cursor -> handle = p_cursor;
	return t_cursor;
}

////////////////////////////////////////////////////////////////////////////////

static GdkCursorType cursorlist[PI_NCURSORS] =
    {
        GDK_BLANK_CURSOR, GDK_LEFT_PTR, GDK_X_CURSOR, GDK_SPRAYCAN, GDK_X_CURSOR, GDK_COFFEE_MUG,
        GDK_CLOCK, GDK_CROSS, GDK_HAND1, GDK_XTERM, GDK_HAND2, GDK_PENCIL, GDK_GUMBY,
        GDK_CROSSHAIR, GDK_WATCH, GDK_QUESTION_ARROW
    };

////////////////////////////////////////////////////////////////////////////////

static GdkCursor* fetch_standard_cursor(GdkDisplay *dpy, GdkCursorType index)
{
	return gdk_cursor_new_for_display(dpy, index);
}

void MCScreenDC::resetcursors()
{
	// MW-2010-09-10: Make sure no stacks reference one of the standard cursors
	MCdispatcher -> clearcursors();

    // Query cursor properties
    static bool s_checked_cursors = false;
    if (!s_checked_cursors)
    {
        s_checked_cursors = true;
        
        // X11 natively supports bicolor (not necessary monochrome!) cursors...
		// Although this is breaking at the moment so we will force b&w...
		MCcursorbwonly = True;

        // Colour and alpha support can be queried from GDK
        MCcursorcanbecolor = gdk_display_supports_cursor_color(dpy);
        MCcursorcanbealpha = gdk_display_supports_cursor_alpha(dpy);
        
        // Get the maximum cursor size
        guint width, height;
        gdk_display_get_maximal_cursor_size(dpy, &width, &height);
		if (0 == width || 0 == height)
			MCcursormaxsize = kMCDefaultCursorSize;
		else
			MCcursormaxsize = MCU_max(width, height);
    }
    
    // TODO: do we need to do this?
	/*if (hasfeature(PLATFORM_FEATURE_NATIVE_THEMES))
	{
		GtkSettings *t_settings;
		t_settings = gtk_settings_get_default();
		
		gchar *theme;
		gint size;
		theme = nil;
		size = 0;
  
		if (t_settings != nil)
			g_object_get_ptr(t_settings, 
				"gtk-cursor-theme-name", &theme,
				"gtk-cursor-theme-size", &size,
				NULL);

		if (theme != nil)
			XcursorSetTheme(dpy, theme);
		if (size > 0)
			XcursorSetDefaultSize(dpy, size);
		
		g_free(theme);
	}*/

	uint2 i;
	MCImage *im;
	for (i = 0 ; i < PI_NCURSORS ; i++)
	{
		freecursor(MCcursors[i]);
		MCcursors[i] = nil;

        if ((im = (MCImage *)MCdispatcher->getobjid(CT_IMAGE, i)) != NULL)
            MCcursors[i] = im->createcursor();
        else if (i < PI_BUSY1)
            MCcursors[i] = create_cursor((Pixmap_ids)i, fetch_standard_cursor(dpy, cursorlist[i]));
        else if (i >= PI_BUSY1 && i <= PI_BUSY8)
            MCcursors[i] = create_cursor(PI_BUSY, fetch_standard_cursor(dpy, cursorlist[PI_BUSY]));
        else
            MCcursors[i] = nil;
	}
}

void MCScreenDC::setcursor(Window w, MCCursorRef c)
{
	if (c == nil)
		c = MCcursors[0];

	if (c != nil)
	{
		if (c -> handle != nil)
            gdk_window_set_cursor(w, c->handle);
        
		gdk_display_flush(dpy);
	}
}

MCCursorRef MCScreenDC::createcursor(MCImageBitmap *p_image, int2 p_xhot, int2 p_yhot)
{
	if (p_image == nil)
        return nil;
		
	// Create a pixbuf from the image data
    GdkPixbuf *t_cursor_pixbuf;
    t_cursor_pixbuf = gdk_pixbuf_new_from_data((const guchar*)p_image->data, GDK_COLORSPACE_RGB, true, 8, p_image->width, p_image->height, p_image->stride, NULL, NULL);
    
    // Turn the pixbuf into a cursor. If GDK cannot create an RGBA cursor, it will
    // instead use a monochrome approximation of it.
    GdkCursor *t_cursor;
    t_cursor = gdk_cursor_new_from_pixbuf(dpy, t_cursor_pixbuf, p_xhot, p_yhot);
    
    // We don't need the pixmap any more
    g_object_unref(t_cursor_pixbuf);
    
	return create_cursor(PI_NONE, t_cursor);
}

void MCScreenDC::freecursor(MCCursorRef c)
{
	if (c == nil)
		return;

	if (c -> handle != nil)
		gdk_cursor_unref(c->handle);

	delete c;
}
