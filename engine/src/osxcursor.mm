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

#include "osxprefix.h"

#include "globdefs.h"
#include "filedefs.h"
#include "parsedef.h"
#include "objdefs.h"

#include "uidc.h"
#include "globals.h"
#include "dispatch.h"
#include "image.h"

#include "osxdc.h"

#include <Cocoa/Cocoa.h>

////////////////////////////////////////////////////////////////////////////////

extern bool MCImageBitmapToCGImage(MCImageBitmap *p_bitmap, bool p_copy, bool p_invert, CGImageRef &r_image);

////////////////////////////////////////////////////////////////////////////////

enum MCCursorKind
{
	// The hidden cursor
	kMCCursorNone,

	// A standard 'theme' cursor
	kMCCursorStandard,

	// A custom cursor made from an image
	kMCCursorCustom
};

struct MCCursor
{
	MCCursorKind kind;
	union
	{
		ThemeCursor standard;
		NSCursor *custom;
	};
};

static MCCursorRef create_hidden_cursor(void)
{
	uint32_t t_img_data;
	t_img_data = 0;
	
	MCImageBitmap t_image;
	t_image . width = 1;
	t_image . height = 1;
	t_image . stride = 4;
	t_image . data = &t_img_data;
	return MCscreen -> createcursor(&t_image, 0, 0);
}

static MCCursorRef create_standard_cursor(ThemeCursor p_theme)
{
	MCCursorRef t_cursor;
	t_cursor = new MCCursor;
	t_cursor -> kind = kMCCursorStandard;
	t_cursor -> standard = p_theme;
	return t_cursor;
}

static MCCursorRef create_custom_cursor(NSCursor *p_cursor)
{
	MCCursorRef t_cursor;
	t_cursor = new MCCursor;
	t_cursor -> kind = kMCCursorCustom;
	t_cursor -> custom = p_cursor;
	return t_cursor;
}

////////////////////////////////////////////////////////////////////////////////

static ThemeCursor theme_cursorlist[PI_NCURSORS] =
{
	kThemeArrowCursor, kThemeArrowCursor,
	kThemeArrowCursor, kThemeArrowCursor, kThemeArrowCursor, kThemeWatchCursor, kThemeWatchCursor,
	kThemeCrossCursor, kThemeArrowCursor, kThemeIBeamCursor, kThemeArrowCursor, kThemeArrowCursor,
	kThemeArrowCursor, kThemeCrossCursor, kThemeWatchCursor, kThemeArrowCursor   
};

// This function expects to be called after a pool has been allocated.
static NSImage *CreateNSImageFromCGImage(CGImageRef p_image)
{
    NSRect t_image_rect = NSMakeRect(0.0, 0.0, 0.0, 0.0);
    CGContextRef t_image_context = nil;
    NSImage* t_new_image = nil;
	
    // Get the image dimensions.
    t_image_rect . size . height = CGImageGetHeight(p_image);
    t_image_rect . size . width = CGImageGetWidth(p_image);
	
    // Create a new image to receive the Quartz image data.
    t_new_image = [[NSImage alloc] initWithSize:t_image_rect.size];
    [t_new_image lockFocus];
	
    // Get the Quartz context and draw.
    t_image_context = (CGContextRef)[[NSGraphicsContext currentContext] graphicsPort];
    CGContextDrawImage(t_image_context, *(CGRect*)&t_image_rect, p_image);
    [t_new_image unlockFocus];
	
    return t_new_image;
}

void MCScreenDC::resetcursors(void)
{
	// MW-2010-09-10: Make sure no stacks reference one of the standard cursors
	MCdispatcher -> clearcursors();

	MCcursorbwonly = False;
	MCcursorcanbecolor = True;
	MCcursorcanbealpha = True;
	MCcursormaxsize = 256;
	
	for(uint32_t i = 0 ; i < PI_NCURSORS ; i++)
	{
		freecursor(MCcursors[i]);
		MCcursors[i] = nil;
		
		MCImage *im;
		if (i == PI_NONE)
			MCcursors[i] = create_hidden_cursor();
		else if ((im = (MCImage *)MCdispatcher->getobjid(CT_IMAGE, i)) != NULL)
			MCcursors[i] = im -> createcursor();
		else if (i < PI_BUSY1)
			MCcursors[i] = create_standard_cursor(theme_cursorlist[i]);
		else
			MCcursors[i] = create_standard_cursor(kThemeWatchCursor);
	}
}

MCCursorRef MCScreenDC::createcursor(MCImageBitmap *p_image, int2 p_hotspot_x, int2 p_hotspot_y)
{
	CGImageRef t_cg_image;
	/* UNCHECKED */ MCImageBitmapToCGImage(p_image, false, false, t_cg_image);
	
	// Convert the CGImage into an NSIMage
	NSImage *t_cursor_image;
	t_cursor_image = CreateNSImageFromCGImage(t_cg_image);
	
	// Get rid of the CG Image
	CGImageRelease(t_cg_image);
	
	// MW-2010-10-12: [[ Bug 8994 ]] If the hotspot is outwith the cursor rect, we get a
	//   duff cursor.
	p_hotspot_x = MCU_max(0, MCU_min((int32_t)p_image -> width - 1, p_hotspot_x));
	p_hotspot_y = MCU_max(0, MCU_min((int32_t)p_image -> height - 1, p_hotspot_y));
	
	NSCursor *t_nscursor = nil;
	
	t_nscursor = [[NSCursor alloc] initWithImage:t_cursor_image hotSpot: NSMakePoint(p_hotspot_x, p_hotspot_y)];
	
	[t_cursor_image release];

	return create_custom_cursor(t_nscursor);
}

void MCScreenDC::freecursor(MCCursorRef p_cursor)
{
	if (p_cursor == nil)
		return;
	
	if (p_cursor -> kind == kMCCursorCustom)
		[ p_cursor -> custom release ];
	delete p_cursor;
}

void MCScreenDC::setcursor(Window w, MCCursorRef c)
{
	// Disable cursor setting when we are a drag-target
	if (MCdispatcher -> isdragtarget())
		return;
	
	if (c == nil || c -> kind == kMCCursorNone)
	{
	}
	else if (c -> kind == kMCCursorStandard)
	{
		SetThemeCursor(c -> standard);
	}
	else if (c -> custom != nil)
	{
		[ c -> custom set ];
	}
}

////////////////////////////////////////////////////////////////////////////////
