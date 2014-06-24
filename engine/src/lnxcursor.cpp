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

#include "lnxprefix.h"

#include "core.h"
#include "globdefs.h"
#include "objdefs.h"
#include "parsedef.h"
#include "filedefs.h"

#include "globals.h"
#include "dispatch.h"
#include "image.h"

#include "lnxdc.h"

#include <X11/Xcursor/Xcursor.h>
#include <gtk/gtk.h>

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
	Cursor handle;
};

static MCCursorRef create_cursor(Pixmap_ids p_id, Cursor p_cursor)
{
	MCCursorRef t_cursor;
	t_cursor = new MCCursor;
	t_cursor -> id = p_id;
	t_cursor -> handle = p_cursor;
	return t_cursor;
}

static void ensure_cursor(MCCursorRef cursor)
{
}

////////////////////////////////////////////////////////////////////////////////

static bool s_checked_xcursor = false;
static bool s_has_xcursor = false;

static uint2 cursorlist[PI_NCURSORS] =
    {
        0, XC_left_ptr, XC_X_cursor, XC_X_cursor, XC_X_cursor, XC_coffee_mug,
        XC_clock, XC_cross, XC_hand1, XC_xterm, XC_hand2, XC_pencil, XC_gumby,
        XC_crosshair, XC_watch, XC_question_arrow
    };

extern "C" int initialise_weak_link_libxcursor(void);

////////////////////////////////////////////////////////////////////////////////

static Cursor fetch_standard_cursor(Display *dpy, uint2 index)
{
	if (s_has_xcursor)
		return XcursorShapeLoadCursor(dpy, index);
	return XCreateFontCursor(dpy, index);
}

void MCScreenDC::resetcursors()
{
	// MW-2010-09-10: Make sure no stacks reference one of the standard cursors
	MCdispatcher -> clearcursors();

	// Check for the existence of the Xcursor library.
	if (!s_checked_xcursor)
	{
		s_checked_xcursor = true;
		s_has_xcursor = initialise_weak_link_libxcursor() != 0;

		// X11 natively supports bicolor (not necessary monochrome!) cursors...
		// Although this is breaking at the moment so we will force b&w...
		MCcursorbwonly = True;

		// If we have the Xcursor extension then we can have color and alpha
		// blended cursors... Assuming there is support.
		// MW-2011-12-05: [[ Bug ]] Is s_has_xcursor is false, then this will
		//   crash!
		if (s_has_xcursor)
			MCcursorcanbecolor = MCcursorcanbealpha = XcursorSupportsARGB(MCdpy) != 0;
		else
			MCcursorcanbecolor = MCcursorcanbealpha = False;
		
		uint32_t t_width, t_height;
		XQueryBestCursor(MCdpy, MCscreen -> getroot(), 1024, 1024, &t_width, &t_height);
		MCcursormaxsize = MCMin(t_width, t_height);
	}

	if (s_has_xcursor && hasfeature(PLATFORM_FEATURE_NATIVE_THEMES))
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
	}

	uint2 i;
	MCImage *im;
	for (i = 0 ; i < PI_NCURSORS ; i++)
	{
		freecursor(MCcursors[i]);
		MCcursors[i] = nil;

		if (i == 0)
		{
			MCColor c;
			c.red = c.green = c.blue = 0x0;
			Pixmap cdata = XCreatePixmap(dpy, getroot(), 16, 16, 1);
			Pixmap cmask = XCreatePixmap(dpy, getroot(), 16, 16, 1);
			XSetForeground(dpy, gc1, 0);
			XFillRectangle(dpy, cdata, gc1, 0, 0, 16, 16);
			XFillRectangle(dpy, cmask, gc1, 0, 0, 16, 16);
			XSetForeground(dpy, gc1, 1);
			MCcursors[PI_NONE] = create_cursor(PI_NONE, XCreatePixmapCursor(dpy, cdata, cmask, (XColor *)&c, (XColor *)&c, 0, 0));
			XFreePixmap(dpy, cdata);
			XFreePixmap(dpy, cmask);
		}
		else if ((im = (MCImage *)MCdispatcher->getobjid(CT_IMAGE, i)) != NULL)
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
		if (c -> handle == nil)
			ensure_cursor(c);

		if (c -> handle != nil)
			XDefineCursor(dpy, w, c -> handle);
		
		XFlush(dpy);
	}
}

static void pixel_to_color(uint32_t p, MCColor& r_color)
{
	r_color . red = (p >> 16) & 0xff;
	r_color . red |= r_color . red << 8;
	r_color . green = (p >> 8) & 0xff;
	r_color . green |= r_color . green << 8;
	r_color . blue = (p >> 0) & 0xff;
	r_color . blue |= r_color . blue << 8;
	r_color . pixel = p;
	r_color . pad = 0;
	r_color . flags = DoRed | DoGreen | DoBlue;
}

MCCursorRef MCScreenDC::createcursor(MCImageBitmap *p_image, int2 p_xhot, int2 p_yhot)
{
	Cursor t_xcursor;
	t_xcursor = nil;
	if (MCcursorcanbealpha)
	{
		// An image buffer is in ARGB pre-multiplied form already.
		XcursorImage t_cimage;
		t_cimage . version = XCURSOR_IMAGE_VERSION;
		t_cimage . size = MCMax(p_image -> width, p_image -> height);
		t_cimage . width = p_image -> width;
		t_cimage . height = p_image -> height;
		t_cimage . xhot = p_xhot;
		t_cimage . yhot = p_yhot;
		t_cimage . delay = 0;
		t_cimage . pixels = (XcursorPixel *)p_image -> data;
		t_xcursor = XcursorImageLoadCursor(MCdpy, &t_cimage);
	}
	else
	{
		// In this case, we will have an ARGB buffer with at most two colors,
		// and alpha values of either 0xff or 0x00 which we can turn into masks.

		// We need some MCBitmaps into which we can put our masks...
		MCBitmap *t_color_mask, *t_trans_mask;
		t_color_mask = ((MCScreenDC*)MCscreen) -> createimage(1, p_image -> width, p_image -> height, True, 0, False, True);
		t_color_mask -> byte_order = LSBFirst;
		t_color_mask -> bitmap_bit_order = LSBFirst;
		t_trans_mask = ((MCScreenDC*)MCscreen) -> createimage(1, p_image -> width, p_image -> height, True, 0, False, True);
		t_trans_mask -> byte_order = LSBFirst;
		t_trans_mask -> bitmap_bit_order = LSBFirst;

		// Now loop over the pixels. We only map two colors (the first two
		// encountered) and fill in the color and trans mask appropriately.
		uint32_t t_first_color, t_second_color;
		t_first_color = 0;
		t_second_color = 0;
		for(int32_t y = 0; y < p_image -> height; y++)
			for(int32_t x = 0; x < p_image -> width; x++)
			{
				uint32_t t_pixel;
				t_pixel = *(((uint32_t *)p_image -> data) + y * p_image -> stride / 4 + x);
				if ((t_pixel >> 24) == 0)
					continue;

				if (t_first_color == 0)
					t_first_color = t_pixel;
				else if (t_second_color == 0 && t_pixel != t_first_color)
					t_second_color = t_pixel;

				if (t_pixel == t_second_color)
					t_color_mask -> data[y * t_color_mask -> bytes_per_line + (x >> 3)] |= 1 << (x & 7);

				t_trans_mask -> data[y * t_trans_mask -> bytes_per_line + (x >> 3)] |= 1 << (x & 7);
			}

		// Turn our bitmaps into pixmaps
		Pixmap t_color_pm, t_trans_pm;
		t_color_pm = ((MCScreenDC*)MCscreen) -> createpixmap(p_image -> width, p_image -> height, 1, False);
		t_trans_pm = ((MCScreenDC*)MCscreen) -> createpixmap(p_image -> width, p_image -> height, 1, False);

		((MCScreenDC*)MCscreen) -> putimage(t_color_pm, t_color_mask, 0, 0, 0, 0, p_image -> width, p_image -> height);
		((MCScreenDC*)MCscreen) -> putimage(t_trans_pm, t_trans_mask, 0, 0, 0, 0, p_image -> width, p_image -> height);
	
		MCColor fc, bc;
		pixel_to_color(t_second_color, fc);
		pixel_to_color(t_first_color, bc);
		t_xcursor = XCreatePixmapCursor(dpy, t_color_pm, t_trans_pm, (XColor *)&fc, (XColor *)&bc, p_xhot - 1, p_yhot - 1);

		((MCScreenDC*)MCscreen) -> freepixmap(t_trans_pm);
		((MCScreenDC*)MCscreen) -> freepixmap(t_color_pm);

		((MCScreenDC*)MCscreen) -> destroyimage(t_color_mask);
		((MCScreenDC*)MCscreen) -> destroyimage(t_trans_mask);
	}

	return create_cursor(PI_NONE, t_xcursor);
}

void MCScreenDC::freecursor(MCCursorRef c)
{
	if (c == nil)
		return;

	if (c -> handle != nil)
		XFreeCursor(dpy, c -> handle);

	delete c;
}
