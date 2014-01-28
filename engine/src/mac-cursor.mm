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

#include <Cocoa/Cocoa.h>
#include <Carbon/Carbon.h>

#include "core.h"
#include "globdefs.h"
#include "region.h"
#include "graphics.h"
#include "imagebitmap.h"

#include "platform.h"
#include "platform-internal.h"

#include "mac-internal.h"

extern bool MCImageBitmapToCGImage(MCImageBitmap *p_bitmap, bool p_copy, bool p_invert, CGImageRef &r_image);

class MCPlatformCursor
{
public:
	uint32_t references;
	bool is_standard : 1;
	union
	{
		MCPlatformStandardCursor standard;
		NSCursor *custom;
	};
};

static MCPlatformCursor *s_hidden_cursor = nil;

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

void MCPlatformCreateStandardCursor(MCPlatformStandardCursor p_standard_cursor, MCPlatformCursorRef& r_cursor)
{
	MCPlatformCursorRef t_cursor;
	/* UNCHECKED */ MCMemoryNew(t_cursor);
	t_cursor -> references = 1;
	t_cursor -> is_standard = true;
	t_cursor -> standard = p_standard_cursor;
	r_cursor = t_cursor;
}

void MCPlatformCreateCustomCursor(MCImageBitmap *p_image, MCPoint p_hotspot, MCPlatformCursorRef& r_cursor)
{
	MCPlatformCursorRef t_cursor;
	/* UNCHECKED */ MCMemoryNew(t_cursor);
	t_cursor -> references = 1;
	t_cursor -> is_standard = false;
	
	CGImageRef t_cg_image;
	/* UNCHECKED */ MCImageBitmapToCGImage(p_image, false, false, t_cg_image);
	
	// Convert the CGImage into an NSIMage
	NSImage *t_cursor_image;
	t_cursor_image = CreateNSImageFromCGImage(t_cg_image);
	
	// Get rid of the CG Image
	CGImageRelease(t_cg_image);

	// MW-2010-10-12: [[ Bug 8994 ]] If the hotspot is outwith the cursor rect, we get a
	//   duff cursor.
	NSPoint t_ns_hotspot;
	t_ns_hotspot . x = MCU_max(0, MCU_min((int32_t)p_image -> width - 1, p_hotspot . x));
	t_ns_hotspot . y = MCU_max(0, MCU_min((int32_t)p_image -> height - 1, p_hotspot . y));
	
	NSCursor *t_nscursor = nil;
	
	t_nscursor = [[NSCursor alloc] initWithImage:t_cursor_image hotSpot: t_ns_hotspot];
	
	[t_cursor_image release];
	
	t_cursor -> custom = t_nscursor;
	
	r_cursor = t_cursor;
}

void MCPlatformRetainCursor(MCPlatformCursorRef p_cursor)
{
	p_cursor -> references += 1;
}

void MCPlatformReleaseCursor(MCPlatformCursorRef p_cursor)
{
	p_cursor -> references -= 1;
	if (p_cursor -> references > 0)
		return;
		
	if (!p_cursor -> is_standard)
		[p_cursor -> custom release ];
		
	MCMemoryDelete(p_cursor);
}

void MCPlatformShowCursor(MCPlatformCursorRef p_cursor)
{
	[NSCursor unhide];
	if (p_cursor -> is_standard)
	{
		switch(p_cursor -> standard)
		{
		case kMCPlatformStandardCursorArrow:
			SetThemeCursor(kThemeArrowCursor);
			break;
		case kMCPlatformStandardCursorWatch:
			SetThemeCursor(kThemeWatchCursor);
			break;
		case kMCPlatformStandardCursorCross:
			SetThemeCursor(kThemeCrossCursor);
			break;
		case kMCPlatformStandardCursorIBeam:
			SetThemeCursor(kThemeIBeamCursor);
			break;
		default:
			assert(false);
			break;
		}
	}
	else
		[p_cursor -> custom set];
}

void MCPlatformHideCursor(void)
{
	if (s_hidden_cursor == nil)
	{
		uint32_t t_img_data;
		t_img_data = 0;
		
		MCImageBitmap t_image;
		t_image . width = 1;
		t_image . height = 1;
		t_image . stride = 4;
		t_image . data = &t_img_data;
		
		MCPoint t_hot_spot;
		t_hot_spot . x = 0;
		t_hot_spot . y = 0;
		MCPlatformCreateCustomCursor(&t_image, t_hot_spot, s_hidden_cursor);
	
	}

	MCPlatformShowCursor(s_hidden_cursor);
}
