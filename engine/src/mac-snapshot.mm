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
#include <CoreVideo/CVDisplayLink.h>

#include "globdefs.h"
#include "parsedef.h"
#include "globals.h"
#include "desktop-dc.h"

#include "imagebitmap.h"

#include "platform.h"
#include "platform-internal.h"

#include "mac-internal.h"

#include "graphics_util.h"

////////////////////////////////////////////////////////////////////////////////

extern bool MCOSXCreateCGContextForBitmap(MCImageBitmap *p_bitmap, CGContextRef &r_context);

////////////////////////////////////////////////////////////////////////////////

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
	NSPoint t_top_left;
	t_top_left = cocoa_point_from_carbon(p_rect . origin . x, p_rect . origin . y + p_rect . size . height);
	return NSMakeRect(t_top_left . x, t_top_left . y, p_rect . size . width, p_rect . size . height);
}

static CGRect carbon_rect_from_cocoa(NSRect p_rect)
{
	return CGRectMake(p_rect.origin.x, menu_screen_height() - (p_rect.origin.y + p_rect.size.height),
					  p_rect.size.width, p_rect.size.height);
}

static CGRect carbon_rect_from_mcrect(MCRectangle p_rect)
{
	return CGRectMake(p_rect.x, p_rect.y, p_rect.width, p_rect.height);
}

static MCRectangle mcrect_from_cocoa(NSRect p_rect)
{
	return MCRectangleMake(p_rect.origin.x, menu_screen_height() - (p_rect.origin.y + p_rect.size.height),
						   p_rect.size.width, p_rect.size.height);
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
	return MCRectangleMake(l, t, r - l, b - t);
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
    
	// MW-2014-03-11: [[ Bug 11654 ]] Make sure we force the wait to finish.
	MCPlatformBreakWait();
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

static void MCMacPlatformCGImageToMCImageBitmap(CGImageRef p_image, MCPoint p_size, MCImageBitmap*& r_bitmap)
{
	if (p_image != nil)
	{
		uint32_t t_width, t_height;
		uint32_t t_image_width, t_image_height;
		t_image_width = CGImageGetWidth(p_image);
		t_image_height = CGImageGetHeight(p_image);
		t_width = p_size . x;
		t_height = p_size . y;
		
		MCGFloat t_hscale, t_vscale;
		t_hscale = (MCGFloat)t_width / (MCGFloat)t_image_width;
		t_vscale = (MCGFloat)t_height / (MCGFloat)t_image_height;
		
		MCImageBitmap *t_bitmap;
		/* UNCHECKED */ MCImageBitmapCreate(t_width, t_height, t_bitmap);
		
		MCImageBitmapClear(t_bitmap);
		
		CGContextRef t_context;
		/* UNCHECKED */ MCOSXCreateCGContextForBitmap(t_bitmap, t_context);
		
		// Draw the image scaled down onto the bitmap
		CGContextScaleCTM(t_context, t_hscale, t_vscale);
		CGContextDrawImage(t_context, CGRectMake(0, 0, t_image_width, t_image_height), p_image);
		
		CGContextRelease(t_context);

		r_bitmap = t_bitmap;
	}
	else
		r_bitmap = nil;
}

//////////

// MW-2014-06-11: [[ Bug 12436 ]] This code uses a displaylink to wait for screen update.

static bool s_display_link_fired;

static CVReturn MyDisplayLinkCallback(CVDisplayLinkRef displayLink, const CVTimeStamp* now, const CVTimeStamp* outputTime, CVOptionFlags flagsIn, CVOptionFlags* flagsOut, void* displayLinkContext)
{
    s_display_link_fired = true;
    MCPlatformBreakWait();
    return kCVReturnSuccess;
}

static void wait_for_refresh(void)
{
    CVDisplayLinkRef t_link;
    // Create a display link capable of being used with all active displays
    CVDisplayLinkCreateWithActiveCGDisplays(&t_link);
    // Set the renderer output callback function
    CVDisplayLinkSetOutputCallback(t_link, &MyDisplayLinkCallback, nil);
    
    CVDisplayLinkStart(t_link);
    
    // We wait for two display link firings as the first will likely be before the
    // screen is updated, and the second will be after. i.e. The screen buffer is
    // up to date at some point between the two firings.
    
    s_display_link_fired = false;
    
    while(!s_display_link_fired)
		MCPlatformWaitForEvent(60.0, true);
    
    s_display_link_fired = false;
    
    while(!s_display_link_fired)
		MCPlatformWaitForEvent(60.0, true);
    
    CVDisplayLinkStop(t_link);
    
    CVDisplayLinkRelease(t_link);
}

//////////

void MCPlatformScreenSnapshotOfUserArea(MCPoint *p_size, MCImageBitmap*& r_bitmap)
{
	// Compute the rectangle to grab in screen co-ords.
	MCRectangle t_screen_rect;

	// Create a window that covers the whole screen.
	MCSnapshotWindow *t_window;
	t_window = [[MCSnapshotWindow alloc] init];
	[t_window orderFront: nil];
	
    MCMacPlatformLockCursor();
    
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
    
    MCMacPlatformUnlockCursor();
    
	// Compute the selected rectangle.
	t_screen_rect = mcrect_from_points(s_snapshot_start_point, s_snapshot_end_point);
	
	// AL-2014-05-23: [[ Bug 12443 ]] Import snapshot crashes when no rect is selected.
	//  6.1 behaviour was to default to snapshot of the whole screen.
	if (t_screen_rect . width == 0 || t_screen_rect . height == 0)
	{
		const MCDisplay *t_displays;
		MCscreen -> getdisplays(t_displays, false);
		
		t_screen_rect = t_displays[0] . viewport;
	}
    
	MCPlatformScreenSnapshot(t_screen_rect, p_size, r_bitmap);
}

void MCMacPlatformScreenSnapshotOfWindowWithinBounds(uint32_t p_window_id, MCRectangle p_bounds, MCPoint *p_size, MCImageBitmap *&r_bitmap)
{
    // MW-2014-06-11: [[ Bug 12436 ]] Wait for the screen to be up to date.
    wait_for_refresh();
    
	CGImageRef t_image;
	t_image = CGWindowListCreateImage(carbon_rect_from_mcrect(p_bounds), kCGWindowListOptionIncludingWindow, p_window_id, kCGWindowImageBoundsIgnoreFraming);
	
	MCPoint t_size;
	if (p_size == 0)
		t_size = MCPointMake(p_bounds . width, p_bounds . height);
	else
		t_size = *p_size;
	
	MCMacPlatformCGImageToMCImageBitmap(t_image, t_size, r_bitmap);
	CGImageRelease(t_image);
}

void MCPlatformScreenSnapshotOfWindow(uint32_t p_window_id, MCPoint *p_size, MCImageBitmap*& r_bitmap)
{
	// IM-2014-04-03: [[ Bug 12085 ]] Update to use Cocoa API to get window bounds
	NSWindow *t_window;
	t_window = [NSApp windowWithWindowNumber: p_window_id];
	
	if (t_window == nil)
		return;
	
	NSRect t_frame_rect = [t_window frame];
	NSRect t_content_rect = [[t_window contentView] frame];
	
	NSRect t_rect = NSOffsetRect(t_content_rect, t_frame_rect.origin.x, t_frame_rect.origin.y);
	
	MCMacPlatformScreenSnapshotOfWindowWithinBounds(p_window_id, mcrect_from_cocoa(t_rect), p_size, r_bitmap);
}

// IM-2014-04-03: [[ Bug 12115 ]] Implement snapshot of rect of window
void MCPlatformScreenSnapshotOfWindowArea(uint32_t p_window_id, MCRectangle p_area, MCPoint *p_size, MCImageBitmap*& r_bitmap)
{
	NSWindow *t_window;
	t_window = [NSApp windowWithWindowNumber: p_window_id];
	
	if (t_window == nil)
		return;
	
	NSRect t_frame_rect = [t_window frame];
	NSRect t_content_rect = [[t_window contentView] frame];
	
	CGFloat t_content_top, t_content_left;
	t_content_top = menu_screen_height() - (t_frame_rect.origin.y + t_content_rect.origin.y + t_content_rect.size.height);
	t_content_left = t_frame_rect.origin.x + t_content_rect.origin.x;
	
	MCRectangle t_rect;
	t_rect = MCRectangleMake(p_area.x + t_content_left, p_area.y + t_content_top, p_area.width, p_area.height);
	
	MCMacPlatformScreenSnapshotOfWindowWithinBounds(p_window_id, t_rect, p_size, r_bitmap);
}

void MCPlatformScreenSnapshot(MCRectangle p_screen_rect, MCPoint *p_size, MCImageBitmap*& r_bitmap)
{
    // MW-2014-06-11: [[ Bug 12436 ]] Wait for the screen to be up to date.
    wait_for_refresh();
    
	// IM-2017-04-21: [[ Bug 19529 ]] CGWindowListCreateImage assumes coords with origin at top-left of primary screen,
	//    so our capture area needs to be offset onto that coordinate system
	MCRectangle t_origin;
	MCPlatformGetScreenViewport(0, t_origin);
	
	CGRect t_area;
	t_area = CGRectMake(p_screen_rect.x - t_origin.x, p_screen_rect.y - t_origin.y, p_screen_rect.width, p_screen_rect.height);
	
	CGImageRef t_image;
	t_image = CGWindowListCreateImage(t_area, kCGWindowListOptionOnScreenOnly, kCGNullWindowID, kCGWindowImageDefault);
	
	MCPoint t_size;
	if (p_size == nil)
		t_size = MCPointMake(p_screen_rect . width, p_screen_rect . height);
	else
		t_size = *p_size;
	
	MCMacPlatformCGImageToMCImageBitmap(t_image, t_size, r_bitmap);
	CGImageRelease(t_image);
}

////////////////////////////////////////////////////////////////////////////////
