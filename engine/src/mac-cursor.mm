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

#include <Cocoa/Cocoa.h>
#include <Carbon/Carbon.h>

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

static bool s_cursor_is_hidden = false;
static NSCursor *s_watch_cursor = nil;

static unsigned char s_watch_cursor_bits[] =
{
    0x07, 0xE0, 0x07, 0xE0, 0x07, 0xE0, 0x07, 0xE0, 0x08, 0x10, 0x10, 0x88, 0x10, 0x88, 0x10, 0x88,
    0x13, 0x88, 0x10, 0x08, 0x10, 0x08, 0x08, 0x10, 0x07, 0xE0, 0x07, 0xE0, 0x07, 0xE0, 0x07, 0xE0
};

static unsigned char s_watch_cursor_mask[] =
{
    0x07, 0xE0, 0x07, 0xE0, 0x07, 0xE0, 0x07, 0xE0, 0x0F, 0xF0, 0x1F, 0xF8, 0x1F, 0xF8, 0x1F, 0xF8,
    0x1F, 0xF8, 0x1F, 0xF8, 0x1F, 0xF8, 0x0F, 0xF0, 0x07, 0xE0, 0x07, 0xE0, 0x07, 0xE0, 0x07, 0xE0,
};

static int s_watch_cursor_hotspot[2] = { 8, 8 };

static NSImage *CreateNSImageFromBits(int p_width, int p_height, unsigned char *p_bits, unsigned char *p_mask)
{
    NSImage *t_image;
    t_image = [[NSImage alloc] initWithSize: NSMakeSize(p_width, p_height)];
    
    int t_stride;
    t_stride = (p_width + 7) >> 3;
    
    NSBitmapImageRep *t_rep;
    t_rep = [[NSBitmapImageRep alloc] initWithBitmapDataPlanes: NULL
                                                    pixelsWide:p_width
                                                    pixelsHigh:p_height
                                                 bitsPerSample:1
                                               samplesPerPixel:2
                                                      hasAlpha:YES
                                                      isPlanar:YES
                                                colorSpaceName:NSCalibratedWhiteColorSpace
                                                   bytesPerRow: t_stride
                                                  bitsPerPixel: 1];
    
    unsigned char *t_planes[5];
    [t_rep getBitmapDataPlanes: t_planes];
    
    for(int y = 0; y < p_height; y++)
    {
        for(int x = 0; x < t_stride; x++)
        {
            t_planes[0][y * t_stride + x] = (~p_bits[y * t_stride + x] & p_mask[y * t_stride + x]);
            t_planes[1][y * t_stride + x] = p_mask[y * t_stride + x];
        }
    }
    
    [t_image addRepresentation: t_rep];
    [t_rep release];
    
    return t_image;
}

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
	/* UNCHECKED */ MCImageBitmapToCGImage(p_image, true, false, t_cg_image);
	
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

void MCPlatformSetCursor(MCPlatformCursorRef p_cursor)
{
	if (p_cursor -> is_standard)
    {
        // By default, we want the cursor to be visible.
        if (s_cursor_is_hidden)
        {
            [NSCursor unhide];
            s_cursor_is_hidden = false;
        }
		switch(p_cursor -> standard)
		{
        // SN-2015-06-16: [[ Bug 14056 ]] Hidden cursor is part of the cursors
        case kMCPlatformStandardCursorNone:
            [NSCursor hide];
            s_cursor_is_hidden = true;
            break;
		case kMCPlatformStandardCursorArrow:
            [[NSCursor arrowCursor] set];
			break;
		case kMCPlatformStandardCursorWatch:
            if (s_watch_cursor == nil)
            {
                NSImage *t_image;
                t_image = CreateNSImageFromBits(16, 16, s_watch_cursor_bits, s_watch_cursor_mask);
                s_watch_cursor = [[NSCursor alloc] initWithImage: t_image hotSpot: NSMakePoint(s_watch_cursor_hotspot[0], s_watch_cursor_hotspot[1])];
                [t_image release];
            }
            [s_watch_cursor set];
			break;
		case kMCPlatformStandardCursorCross:
            [[NSCursor crosshairCursor] set];
			break;
		case kMCPlatformStandardCursorIBeam:
            [[NSCursor IBeamCursor] set];
			break;
		default:
			assert(false);
			break;
		}
	}
	else
		[p_cursor -> custom set];
}

void MCPlatformHideCursorUntilMouseMoves(void)
{
    [NSCursor setHiddenUntilMouseMoves: YES];
}

void MCMacPlatformResetCursor(void)
{
    [[NSCursor arrowCursor] set];
}
