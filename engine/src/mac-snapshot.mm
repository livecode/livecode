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

#include "core.h"
#include "globdefs.h"

#include "imagebitmap.h"

#include "platform.h"
#include "platform-internal.h"

#include "mac-internal.h"

////////////////////////////////////////////////////////////////////////////////

extern MCRectangle MCU_make_rect(int2 x, int2 y, uint2 w, uint2 h);

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
	[m_region setBoxType: NSBoxCustom];
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

////////////////////////////////////////////////////////////////////////////////

static void MCMacPlatformCGImageToMCImageBitmap(CGImageRef p_image, MCImageBitmap*& r_bitmap)
{
	if (p_image != nil)
	{
		MCImageBitmap *t_bitmap;
		/* UNCHECKED */ MCImageBitmapCreate(CGImageGetWidth(p_image), CGImageGetHeight(p_image), t_bitmap);
		
		CFDataRef t_data;
		t_data = CGDataProviderCopyData(CGImageGetDataProvider(p_image));
		
		uint8_t *t_bytes;
		t_bytes = (uint8_t *)CFDataGetBytePtr(t_data);
		
		for(int32_t y = 0; y < CGImageGetHeight(p_image); y++)
			memcpy((uint8_t*)t_bitmap -> data + y * t_bitmap -> stride, t_bytes + y * CGImageGetBytesPerRow(p_image), CGImageGetWidth(p_image) * 4);
		
		CFRelease(t_data);
		
		r_bitmap = t_bitmap;
	}
	else
		r_bitmap = nil;
}

void MCPlatformScreenSnapshotOfUserArea(MCImageBitmap*& r_bitmap)
{
	// Compute the rectangle to grab in screen co-ords.
	MCRectangle t_screen_rect;

	// Create a window that covers the whole screen.
	MCSnapshotWindow *t_window;
	t_window = [[MCSnapshotWindow alloc] init];
	[t_window orderFront: nil];
	
	// Set the cursor to cross.
	[[NSCursor crosshairCursor] push];
	
	// Wait until the mouse has been released.
	s_snapshot_done = false;
	while(!s_snapshot_done)
		MCPlatformWaitForEvent(60.0, false);

	// Remove the window from display.
	[t_window orderOut: nil];
	[t_window release];
	
	// Return the cursor to arrow.
	[NSCursor pop];
	
	// Compute the selected rectangle.
	t_screen_rect = mcrect_from_points(s_snapshot_start_point, s_snapshot_end_point);

	MCPlatformScreenSnapshot(t_screen_rect, r_bitmap);
}

void MCPlatformScreenSnapshotOfWindow(uint32_t p_window_id, MCImageBitmap*& r_bitmap)
{
	CGImageRef t_image;
	t_image = CGWindowListCreateImage(CGRectNull, kCGWindowListOptionIncludingWindow, p_window_id, kCGWindowImageBoundsIgnoreFraming);
	MCMacPlatformCGImageToMCImageBitmap(t_image, r_bitmap);
	CGImageRelease(t_image);
}

void MCPlatformScreenSnapshot(MCRectangle p_screen_rect, MCImageBitmap*& r_bitmap)
{
	CGRect t_area;
	t_area = CGRectMake(p_screen_rect . x, p_screen_rect . y, p_screen_rect . width, p_screen_rect . height);
	
	CGImageRef t_image;
	t_image = CGWindowListCreateImage(t_area, kCGWindowListOptionOnScreenOnly, kCGNullWindowID, kCGWindowImageDefault);
	MCMacPlatformCGImageToMCImageBitmap(t_image, r_bitmap);
	CGImageRelease(t_image);
}

////////////////////////////////////////////////////////////////////////////////
