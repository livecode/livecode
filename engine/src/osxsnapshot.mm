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

#include "core.h"
#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"

#include "util.h"
#include "uidc.h"
#include "globals.h"
#include "image.h"
#include "osxdc.h"

#include <Cocoa/Cocoa.h>

#ifndef _IOS_MOBILE
#define CGFloat float
#endif

static CGPoint s_snapshot_start_point, s_snapshot_end_point;
static bool s_snapshot_done = false;

static float menu_screen_height(void)
{
	NSArray *t_screens;
	t_screens = [NSScreen screens];
	
	NSScreen *t_screen;
	for(uint32_t i = 0; i < [t_screens count]; i++)
	{
		t_screen = (NSScreen *)[t_screens objectAtIndex: i];
		if (NSPointInRect(NSZeroPoint, [t_screen frame]))
			break;
	}
	
	return NSMaxY([t_screen frame]);
}

static NSPoint cocoa_point_from_carbon(CGFloat x, CGFloat y)
{
	return NSMakePoint(x, menu_screen_height() - y);
}

static NSRect cocoa_rect_from_carbon(CGRect p_rect)
{
	NSPoint t_top_left, t_bottom_right;
	t_top_left = cocoa_point_from_carbon(p_rect . origin . x, p_rect . origin . y + p_rect . size . height);
	return NSMakeRect(t_top_left . x, t_top_left . y, p_rect . size . width, p_rect . size . height);
}

static CGPoint carbon_point_from_cocoa(NSPoint p_point)
{
	return CGPointMake(p_point . x, menu_screen_height() - p_point . y);
}

static CGRect cgrect_from_points(CGPoint x, CGPoint y)
{
	CGFloat l, t, r, b;
	l = MCMin(x . x, y . x);
	r = MCMax(x . x, y . x);
	t = MCMin(x . y, y . y);
	b = MCMax(x . y, y . y);
	return CGRectMake(l, t, r - l, b - t);
}

static MCRectangle mcrect_from_points(CGPoint x, CGPoint y)
{
	CGFloat l, t, r, b;
	l = MCMin(x . x, y . x);
	r = MCMax(x . x, y . x);
	t = MCMin(x . y, y . y);
	b = MCMax(x . y, y . y);
	return MCU_make_rect(l, t, r - l, b - t);
}

static Rect rect_from_points(CGPoint x, CGPoint y)
{
	CGFloat l, t, r, b;
	l = MCMin(x . x, y . x);
	r = MCMax(x . x, y . x);
	t = MCMin(x . y, y . y);
	b = MCMax(x . y, y . y);
	
	Rect t_rect;
	SetRect(&t_rect, l, t, r, b);
	
	return t_rect;
}

@interface MCSnapshotWindow : NSWindow
{
	NSBox *m_region;
}

- (id)init;
- (void)dealloc;

- (void)mouseDown: (NSEvent *)event;
- (void)mouseUp: (NSEvent *)event;
- (void)mouseDragged: (NSEvent *)event;
@end

@implementation MCSnapshotWindow

- (id)init
{
	// Compute the desktop bounds.
	NSRect t_bounds;
	t_bounds = NSZeroRect;
	NSArray *t_screens;
	t_screens = [NSScreen screens];
	for(uint32_t i = 0; i < [t_screens count]; i++)
	{
		NSScreen *t_screen;
		t_screen = (NSScreen *)[t_screens objectAtIndex: i];
		t_bounds = NSUnionRect(t_bounds, [t_screen frame]);
	}
	
	// Initialize the window.
	self = [super initWithContentRect: t_bounds styleMask: NSBorderlessWindowMask backing: NSBackingStoreBuffered defer: NO];
	
	// Make sure the window is not opaque to events, and renders everything
	// at 50% transparency.
	[self setOpaque: NO];
	[self setAlphaValue: 0.5];
	[self setBackgroundColor: [NSColor clearColor]];
	[self setLevel: NSScreenSaverWindowLevel];
	[self setIgnoresMouseEvents: NO];
	
	// Setup the region used to display the selected area.
	m_region = [[NSBox alloc] initWithFrame: NSZeroRect];
	[m_region setTitlePosition: NSNoTitle];
	[m_region setBorderType: NSLineBorder];
	[m_region setBoxType: MCmajorosversion >= 0x1050 ? (NSBoxType) 4 : NSBoxPrimary];
	[m_region setBorderWidth: 1];
	[m_region setBorderColor: [NSColor whiteColor]];
	[m_region setFillColor: [NSColor grayColor]];
	[[self contentView] addSubview: m_region];
	
	// Remember to return the window.
	return self;
}

- (void)dealloc
{
	[m_region release];
	[super dealloc];
}

- (void)mouseDown: (NSEvent *)event
{
	// Start the snapshot region.
	s_snapshot_start_point = carbon_point_from_cocoa([event locationInWindow]); //CGEventGetLocation([event CGEvent]);
	s_snapshot_end_point = s_snapshot_start_point;
}

- (void)mouseUp: (NSEvent *)event
{
	// Finish the snapshot region.
	s_snapshot_end_point = carbon_point_from_cocoa([event locationInWindow]); //CGEventGetLocation([event CGEvent]);
	s_snapshot_done = true;
	
	// Remove the region from display and force an update to ensure in QD mode, we don't get
	// partial grayness over the selected area.
	[m_region setHidden: YES];
	[[self contentView] setNeedsDisplayInRect: [m_region frame]];
	[self displayIfNeeded];
}

- (void)mouseDragged: (NSEvent *)event
{
	// Update the snapshot end point
	s_snapshot_end_point = carbon_point_from_cocoa([event locationInWindow]); //CGEventGetLocation([event CGEvent]);
	
	// Get the old and new frames.
	NSRect t_old_frame, t_new_frame;
	t_old_frame = [m_region frame];
	t_new_frame = cocoa_rect_from_carbon(cgrect_from_points(s_snapshot_start_point, s_snapshot_end_point));
	
	// Update the frames
	[m_region setFrame: t_new_frame];
	[m_region setNeedsDisplay: YES];
	
	// Make sure the contentview is redrawn in the appropriate places.
	[[self contentView] setNeedsDisplayInRect: t_old_frame];
	[[self contentView] setNeedsDisplayInRect: t_new_frame];
}

@end

MCImageBitmap *getimage(CGrafPtr src)
{
	uint2 depth;
	uint4 bpl;                //bytes per line
	char *sptr;               //point to the begining of src image data
	PixMapHandle srcpm;
	
	Rect r;
	int32_t x = 0, y = 0;
	uint32_t w, h;
	
	srcpm = GetGWorldPixMap(src);
	LockPixels(srcpm);
	
	bpl = GetPixRowBytes(srcpm);
	y -= GetPortBounds(src, &r)->top;
	depth = (*srcpm)->pixelSize;
	sptr = GetPixBaseAddr(srcpm) + y * bpl + x * (depth >> 3);
	
	w = r.right - r.left;
	h = r.bottom - r.top;
	
	MCImageBitmap *image = nil;
	
	{
		assert(depth == 32);
		/* UNCHECKED */ MCImageBitmapCreate(w, h, image);

		{
			uint8_t *dptr = (uint8_t*)image->data; //point to destination image data buffer
			uint4 bytes = (w * depth + 7) >> 3;
			while (h--)
			{
				memcpy(dptr, sptr, bytes);
				sptr += bpl;
				dptr += image->stride;
			}
		}
	}
	
	UnlockPixels(srcpm);
	
	return image;
}

/* OVERHAUL - REVISIT: p_scale_factor parameter currently ignored */
MCImageBitmap *MCScreenDC::snapshot(MCRectangle& p_rect, MCGFloat p_scale_factor, uint32_t p_window, const char *p_display_name)
{
	// Compute the rectangle to grab in screen co-ords.
	MCRectangle t_screen_rect;
	if (p_window == 0 && (p_rect . width == 0 || p_rect . height == 0))
	{
		// If the rect is of zero size and no window has been specified then allow
		// a rect to be dragged out.
		
		// Create a window that covers the whole screen.
		MCSnapshotWindow *t_window;
		t_window = [[MCSnapshotWindow alloc] init];
		[t_window orderFront: nil];
		
		// Set the cursor to cross.
		setcursor(nil, MCcursors[PI_CROSS]);
		
		// Wait until the mouse has been released.
		s_snapshot_done = false;
		while(!s_snapshot_done)
			MCscreen -> wait(60.0, False, True);
	
		// Remove the window from display.
		[t_window orderOut: nil];
		[t_window release];

		// Return the cursor to arrow.
		setcursor(nil, MCcursors[PI_ARROW]);
		
		// Compute the selected rectangle.
		t_screen_rect = mcrect_from_points(s_snapshot_start_point, s_snapshot_end_point);
	}
	else if (p_window == 0)
	{
		// We have no window, but do have a rect so just copy it.
		t_screen_rect = p_rect;
	}
	else
	{
		// Get the window bounds.
		Rect t_window_bounds;
		GetWindowBounds((WindowPtr)p_window, kWindowGlobalPortRgn, &t_window_bounds);
		
		// We have a rect relative to a window so map the co-ords.
		t_screen_rect = MCU_offset_rect(p_rect, t_window_bounds . left, t_window_bounds . top);
	}

	// Now we have a screen rect, we can grab the contents. On 10.5 and above we
	// use the CGWindow APIs; otherwise we fall back to QD for now.
	MCImageBitmap *t_snapshot;
	t_snapshot = nil;
	if (MCmajorosversion >= 0x1050)
	{
		typedef CGImageRef (*CGWindowListCreateImageProc)(CGRect, uint32_t listOption, uint32_t windowID, uint32_t imageOption);
		typedef CFDataRef (*CGDataProviderCopyDataProc)(CGDataProviderRef);
		static CGWindowListCreateImageProc s_cg_window_list_create_image = nil;
		static CGDataProviderCopyDataProc s_cg_data_provider_copy_data = nil;
		if (s_cg_window_list_create_image == nil)
		{
			s_cg_window_list_create_image = (CGWindowListCreateImageProc)CFBundleGetFunctionPointerForName(CFBundleGetBundleWithIdentifier(CFSTR("com.apple.ApplicationServices")), CFSTR("CGWindowListCreateImage"));
			s_cg_data_provider_copy_data = (CGDataProviderCopyDataProc)CFBundleGetFunctionPointerForName(CFBundleGetBundleWithIdentifier(CFSTR("com.apple.ApplicationServices")), CFSTR("CGDataProviderCopyData"));
		}
		
		CGRect t_area;
		t_area = CGRectMake(t_screen_rect . x, t_screen_rect . y, t_screen_rect . width, t_screen_rect . height);
		
		CGImageRef t_image;
		t_image = s_cg_window_list_create_image(t_area, /*kCGWindowListOptionOnScreenOnly*/ (1 << 0), /*kCGNullWindowID*/ 0, /*kCGWindowImageDefault*/ 0);
		if (t_image != nil)
		{
			MCImageBitmap *t_bitmap;
			/* UNCHECKED */ MCImageBitmapCreate(CGImageGetWidth(t_image), CGImageGetHeight(t_image), t_bitmap);
			
			CFDataRef t_data;
			t_data = s_cg_data_provider_copy_data(CGImageGetDataProvider(t_image));
			
			uint8_t *t_bytes;
			t_bytes = (uint8_t *)CFDataGetBytePtr(t_data);
			
			for(int32_t y = 0; y < CGImageGetHeight(t_image); y++)
				memcpy((uint8_t*)t_bitmap -> data + y * t_bitmap -> stride, t_bytes + y * CGImageGetBytesPerRow(t_image), CGImageGetWidth(t_image) * 4);
			
			CFRelease(t_data);
			
			CGImageRelease(t_image);
			
			t_snapshot = t_bitmap;
		}
	}
	else
	{
		Rect t_src_rect, t_dst_rect;
		SetRect(&t_src_rect, t_screen_rect . x, t_screen_rect . y, t_screen_rect . x + t_screen_rect . width, t_screen_rect . y + t_screen_rect . height);
		SetRect(&t_dst_rect, 0, 0, t_screen_rect . width, t_screen_rect . height);

		Pixmap t_snap_pixmap;
		t_snap_pixmap = createpixmap(t_screen_rect . width, t_screen_rect . height, 32, False);

		CGrafPtr port;
		port = CreateNewPort();
		
		PixMapHandle spm, dpm;
		spm = GetGWorldPixMap(port);
		dpm = GetGWorldPixMap((CGrafPtr)t_snap_pixmap -> handle . pixmap);
		LockPixels(spm);
		LockPixels(dpm);
		
		const BitMap *sbm, *dbm;
		sbm = (BitMap *)*spm;
		dbm = (BitMap *)*dpm;
		
		CopyBits(sbm, dbm, &t_src_rect, &t_dst_rect, srcCopy, NULL);
		
		UnlockPixels(dpm);
		UnlockPixels(spm);
		
		DisposePort(port);
		
		MCImageBitmap *snapimage;
		snapimage = getimage((CGrafPtr)t_snap_pixmap -> handle . pixmap);
		freepixmap(t_snap_pixmap);
		
		t_snapshot = snapimage;
	}

	return t_snapshot;
}

