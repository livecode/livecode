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
#include "region.h"
#include "graphics.h"

#include "platform.h"
#include "platform-internal.h"

#include "mac-internal.h"

////////////////////////////////////////////////////////////////////////////////

@implementation com_runrev_livecode_MCWindowDelegate

- (id)initWithPlatformWindow: (MCMacPlatformWindow *)window
{
	self = [super init];
	if (self == nil)
		return nil;
	
	m_window = window;
	
	return self;
}

- (void)dealloc
{
	[super dealloc];
}

- (MCMacPlatformWindow *)platformWindow
{
	return m_window;
}

//////////

- (BOOL)windowShouldClose:(id)sender
{
	m_window -> ProcessCloseRequest();
	return NO;
}

- (NSSize)windowWillResize:(NSWindow *)sender toSize:(NSSize)frameSize
{
	return frameSize;
}

- (void)windowDidMove:(NSNotification *)notification
{
	// COCOA-TODO: This only gives a final notification - perhaps use RunLoop
	//   observer to monitor what's going on and check for updates.
	m_window -> ProcessDidMove();
}

- (void)windowDidMiniaturize:(NSNotification *)notification
{
	m_window -> ProcessDidMiniaturize();
}

- (void)windowDidDeminiaturize:(NSNotification *)notification
{
	m_window -> ProcessDidDeminiaturize();
}

- (void)windowDidBecomeKey:(NSNotification *)notification
{
	m_window -> ProcessDidBecomeKey();
}

- (void)windowDidResignKey:(NSNotification *)notification
{
	m_window -> ProcessDidResignKey();
}

@end

////////////////////////////////////////////////////////////////////////////////

@implementation com_runrev_livecode_MCWindowView

//////////

- (id)initWithFrame:(NSRect)frameRect
{
	self = [super initWithFrame: frameRect];
	if (self == nil)
		return nil;
	
	m_tracking_area = nil;
	
	return self;
}

- (void)dealloc
{
	[m_tracking_area release];
	[super dealloc];
}

/////////

- (void)updateTrackingAreas
{
	[super updateTrackingAreas];
	
	// COCOA-TODO: Make sure this is necessary, apparantly things should
	//   automagically resize with InVisibleRect.
	
	if (m_tracking_area != nil)
	{
		[self removeTrackingArea: m_tracking_area];
		[m_tracking_area release];
		m_tracking_area = nil;
	}
	
	m_tracking_area = [[NSTrackingArea alloc] initWithRect: [self bounds]
												   options: (NSTrackingMouseEnteredAndExited | 
															 NSTrackingMouseMoved | 
															 NSTrackingActiveAlways | 
															 NSTrackingInVisibleRect | 
															 NSTrackingEnabledDuringMouseDrag)
													 owner: self
												  userInfo: nil];
	[self addTrackingArea: m_tracking_area];
}

//////////

- (BOOL)isFlipped
{
	return NO;
}

- (BOOL)acceptsFirstMouse:(NSEvent *)theEvent
{
	return YES;
}

- (BOOL)acceptsFirstResponder
{
	return YES;
}

- (BOOL)becomeFirstResponder
{
	return YES;
}

- (BOOL)resignFirstResponder
{
	return YES;
}

//////////

- (void)mouseDown: (NSEvent *)event
{
	[self handleMousePress: event isDown: YES];
}

- (void)mouseUp: (NSEvent *)event
{
	[self handleMousePress: event isDown: NO];
}

- (void)mouseMoved: (NSEvent *)event
{
	[self handleMouseMove: event];
}

- (void)mouseDragged: (NSEvent *)event
{
	[self handleMouseMove: event];
}

- (void)rightMouseDown: (NSEvent *)event
{
	[self handleMousePress: event isDown: YES];
}

- (void)rightMouseUp: (NSEvent *)event
{
	[self handleMousePress: event isDown: NO];
}

- (void)rightMouseMoved: (NSEvent *)event
{
	[self handleMouseMove: event];
}

- (void)rightMouseDragged: (NSEvent *)event
{
	[self handleMouseMove: event];
}

- (void)otherMouseDown: (NSEvent *)event
{
	[self handleMousePress: event isDown: YES];
}

- (void)otherMouseUp: (NSEvent *)event
{
	[self handleMousePress: event isDown: NO];
}

- (void)otherMouseMoved: (NSEvent *)event
{
	[self handleMouseMove: event];
}

- (void)otherMouseDragged: (NSEvent *)event
{
	[self handleMouseMove: event];
}

- (void)mouseEntered: (NSEvent *)event
{
	[self handleMouseMove: event];
}

- (void)mouseExited: (NSEvent *)event
{
	[self handleMouseMove: event];
}

- (void)flagsChanged: (NSEvent *)event
{
}

- (void)keyDown: (NSEvent *)event
{
}

- (void)keyUp: (NSEvent *)event
{
}

//////////

- (void)handleMouseMove: (NSEvent *)event
{
	MCMacPlatformWindow *t_window;
	t_window = [(MCWindowDelegate *)[[self window] delegate] platformWindow];
	
	NSPoint t_location;
	t_location = [event locationInWindow];
	if ([event window] != nil)
		t_location = [[event window] convertBaseToScreen: t_location];
	
	t_window -> ProcessMouseMove(t_location);
}

- (void)handleMousePress: (NSEvent *)event isDown: (BOOL)pressed
{
	[self handleMouseMove: event];
	
	MCMacPlatformWindow *t_window;
	t_window = [(MCWindowDelegate *)[[self window] delegate] platformWindow];
	
	t_window -> ProcessMousePress([event buttonNumber], pressed == YES);
}

//////////

- (void)setFrameSize: (NSSize)size
{
	[super setFrameSize: size];
	
	if ([self window] == nil)
		return;
	
	MCMacPlatformWindow *t_window;
	t_window = [(MCWindowDelegate *)[[self window] delegate] platformWindow];
	
	t_window -> ProcessDidResize();
}
	
//////////

// COCOA-TODO: Improve dirty rect calc
- (void)drawRect: (NSRect)dirtyRect
{
	MCMacPlatformWindow *t_window;
	t_window = [(MCWindowDelegate *)[[self window] delegate] platformWindow];
	
	CGContextRef t_graphics;
	t_graphics = (CGContextRef)[[NSGraphicsContext currentContext] graphicsPort];
	
	const NSRect *t_update_rects;
	NSInteger t_update_rect_count;
	[self getRectsBeingDrawn: &t_update_rects count: &t_update_rect_count];
	
	MCRegionRef t_update_rgn;
	/* UNCHECKED */ MCRegionCreate(t_update_rgn);
	for(NSInteger i = 0; i < t_update_rect_count; i++)
		/* UNCHECKED */ MCRegionIncludeRect(t_update_rgn, [self mapNSRectToMCRectangle: t_update_rects[i]]);
	
	//////////
	
	// Save the context state
	CGContextSaveGState(t_graphics);
	
	{
		MCMacPlatformSurface t_surface(t_window, t_graphics, t_update_rgn);
		t_window -> HandleRedraw(&t_surface, t_update_rgn);
	}
	
	// Restore the context state
	CGContextRestoreGState(t_graphics);
	
	//////////
	
	MCRegionDestroy(t_update_rgn);
}

//////////

- (MCRectangle)mapNSRectToMCRectangle: (NSRect)r
{
	return MCRectangleMake(r . origin . x, [self bounds] . size . height - (r . origin . y + r . size . height), r . size . width, r . size . height);
}

- (NSRect)mapMCRectangleToNSRect: (MCRectangle)r
{
	return NSMakeRect(r . x, [self bounds] . size . height - (r . y + r . height), r . width, r . height);
}

@end

////////////////////////////////////////////////////////////////////////////////

MCMacPlatformWindow::MCMacPlatformWindow(void)
{
	m_delegate = nil;
	m_view = nil;
	m_handle = nil;
	
	m_shadow_changed = false;
	m_synchronizing = false;
}

MCMacPlatformWindow::~MCMacPlatformWindow(void)
{
	[m_view release];
	[m_handle release];
	[m_delegate release];
}

////////////////////////////////////////////////////////////////////////////////

void MCMacPlatformWindow::ProcessCloseRequest(void)
{
	// Just pass the handling onto the universal imp.
	HandleCloseRequest();
}

void MCMacPlatformWindow::ProcessDidMove(void)
{
	// Don't handle move/resize events when synchronizing properties.
	if (m_synchronizing)
		return;
	
	// Get the window's new content rect.
	NSRect t_new_cocoa_content;
	t_new_cocoa_content = [m_window_handle contentRectForFrameRect: [m_window_handle frame]];
	
	// Map from cocoa coords.
	MCRectangle t_content;
	MCMacPlatformMapScreenNSRectToMCRectangle(t_new_cocoa_content, t_content);
	
	// And get the super class to deal with it.
	HandleReshape(t_content);
}

void MCMacPlatformWindow::ProcessDidResize(void)
{
	ProcessDidMove();
}

void MCMacPlatformWindow::ProcessDidMiniaturize(void)
{
	HandleIconify();
}

void MCMacPlatformWindow::ProcessDidDeminiaturize(void)
{
	HandleUniconify();
}

void MCMacPlatformWindow::ProcessDidBecomeKey(void)
{
	HandleFocus();
}

void MCMacPlatformWindow::ProcessDidResignKey(void)
{
	HandleUnfocus();
}

void MCMacPlatformWindow::ProcessMouseMove(NSPoint p_location_cocoa)
{
	MCPoint t_location;
	MCMacPlatformMapScreenNSPointToMCPoint(p_location_cocoa, t_location);
	MCMacPlatformHandleMouseMove(t_location);
}

void MCMacPlatformWindow::ProcessMousePress(NSInteger p_button, bool p_is_down)
{
	MCMacPlatformHandleMousePress(p_button, p_is_down);
}

////////////////////////////////////////////////////////////////////////////////

void MCMacPlatformWindow::DoRealize(void)
{
	// If the window already exists, we have nothing to do.
	if (m_handle != nil)
		return;
	
	// Compute the window level based on the window style.
	int t_window_level;
	switch(m_style)
	{
		case kMCPlatformWindowStyleNone:
		case kMCPlatformWindowStyleDocument:
			t_window_level = kCGNormalWindowLevelKey;
			break;
		case kMCPlatformWindowStylePalette:
			t_window_level = kCGFloatingWindowLevelKey;
			break;
		case kMCPlatformWindowStyleDialog:
			t_window_level = kCGModalPanelWindowLevelKey;
			break;
		case kMCPlatformWindowStyleUtility:
			t_window_level = kCGUtilityWindowLevelKey;
			break;
		case kMCPlatformWindowStylePopUp:
			t_window_level = kCGPopUpMenuWindowLevelKey;
			break;
		case kMCPlatformWindowStyleToolTip:
			t_window_level = kCGStatusWindowLevelKey;
			break;
		default:
			assert(false);
			break;
	}
	
	// Compute the cocoa window style from our window style and widget options.
	NSUInteger t_window_style;
	ComputeCocoaStyle(t_window_style);
	
	// Compute the cocoa-oriented content rect.
	NSRect t_cocoa_content;
	MCMacPlatformMapScreenMCRectangleToNSRect(m_content, t_cocoa_content);
	
	// For floating window levels, we use a panel, otherwise a normal window will do.
	// (Note that NSPanel is a subclass of NSWindow)
	if (t_window_level != kCGFloatingWindowLevelKey)
		m_window_handle = [[NSWindow alloc] initWithContentRect: t_cocoa_content styleMask: t_window_style backing: NSBackingStoreBuffered defer: YES];
	else
		m_panel_handle = [[NSPanel alloc] initWithContentRect: t_cocoa_content styleMask: t_window_style backing: NSBackingStoreBuffered defer: YES];
	
	m_delegate = [[com_runrev_livecode_MCWindowDelegate alloc] initWithPlatformWindow: this];
	[m_window_handle setDelegate: m_delegate];
	
	m_view = [[com_runrev_livecode_MCWindowView alloc] initWithFrame: NSZeroRect];
	[m_window_handle setContentView: m_view];
	
	[m_window_handle setLevel: t_window_level];
	[m_window_handle setOpaque: m_mask != nil];
	[m_window_handle setHasShadow: m_has_shadow];
	if (!m_has_zoom_widget)
		[[m_window_handle standardWindowButton: NSWindowZoomButton] setEnabled: NO];
	[m_window_handle setAlphaValue: m_opacity];
	[m_window_handle setDocumentEdited: m_has_modified_mark];
	
	// COCOA-TODO: live resizing
}

void MCMacPlatformWindow::DoSynchronize(void)
{
	// If we don't have a window handle at the moment, we have nothing to
	// synchronize.
	if (m_handle == nil)
		return;
	
	m_synchronizing = true;
	
	// COCOA-TODO: Sort out changes that affect window type (not needed at the moment
	//   since the engine recreates such).
	if (m_changes . style_changed)
	{
	}
	
	if (m_changes . opacity_changed)
		[m_window_handle setAlphaValue: m_opacity];
	
	if (m_changes . mask_changed)
	{
		[m_window_handle setOpaque: m_mask != nil];
		if (m_has_shadow)
			m_shadow_changed = true;
	}
	
	if (m_changes . content_changed)
	{
		NSRect t_cocoa_content;
		MCMacPlatformMapScreenMCRectangleToNSRect(m_content, t_cocoa_content);
		
		NSRect t_cocoa_frame;
		t_cocoa_frame = [m_window_handle frameRectForContentRect: t_cocoa_content];
		
		[m_window_handle setFrame: t_cocoa_frame display: NO];
	}
	
	if (m_changes . title_changed)
		[m_window_handle setTitle: m_title != nil ? [NSString stringWithCString: m_title encoding: NSUTF8StringEncoding] : @""];
	
	if (m_changes . has_modified_mark_changed)
		[m_window_handle setDocumentEdited: m_has_modified_mark];
	
	if (m_changes . use_live_resizing_changed)
		;
	
	m_synchronizing = false;
}

bool MCMacPlatformWindow::DoSetProperty(MCPlatformWindowProperty property, MCPlatformPropertyType type, const void *value)
{
	return false;
}

bool MCMacPlatformWindow::DoGetProperty(MCPlatformWindowProperty property, MCPlatformPropertyType type, void *r_value)
{
	return false;
}

void MCMacPlatformWindow::DoShow(void)
{
	[m_window_handle makeKeyAndOrderFront: nil];
}

void MCMacPlatformWindow::DoHide(void)
{
	[m_window_handle orderOut: nil];
}

void MCMacPlatformWindow::DoFocus(void)
{
	// COCOA-TODO: Implement focus.
}

void MCMacPlatformWindow::DoRaise(void)
{
	[m_window_handle orderFront: nil];
}

void MCMacPlatformWindow::DoUpdate(void)
{	
	// If the shadow has changed (due to the mask changing) we must disable
	// screen updates otherwise we get a flicker.
	if (m_shadow_changed)
		NSDisableScreenUpdates();
	
	// Mark the bounding box of the dirty region for needing display.
	// COCOA-TODO: Make display update more specific.
	[m_view setNeedsDisplayInRect: [m_view mapMCRectangleToNSRect: MCRegionGetBoundingBox(m_dirty_region)]];
	
	// Force a re-display, this will cause drawRect to be invoked on our view
	// which in term will result in a redraw window callback being sent.
	[m_view display];
	
	// Re-enable screen updates if needed.
	if (m_shadow_changed)
		NSEnableScreenUpdates();
}

void MCMacPlatformWindow::DoIconify(void)
{
}

void MCMacPlatformWindow::DoUniconify(void)
{
}

void MCMacPlatformWindow::DoMapContentRectToFrameRect(MCRectangle p_content, MCRectangle& r_frame)
{
	// This method must work for the potentially unapplied property changes
	// and when the window has not been created - thus we use the NSWindow
	// class method.
	
	// Compute the window style.
	NSUInteger t_window_style;
	ComputeCocoaStyle(t_window_style);
	
	// Map the content rect to cocoa coord system.
	NSRect t_cocoa_content;
	MCMacPlatformMapScreenMCRectangleToNSRect(p_content, t_cocoa_content);
	
	// Consult the window class to transform the rect.
	NSRect t_cocoa_frame;
	t_cocoa_frame = [NSWindow frameRectForContentRect: t_cocoa_content styleMask: t_window_style];
	
	// Map the rect from the cocoa coord system.
	MCMacPlatformMapScreenNSRectToMCRectangle(t_cocoa_frame, r_frame);
}

void MCMacPlatformWindow::DoMapFrameRectToContentRect(MCRectangle p_frame, MCRectangle& r_content)
{
	// This method must work for the potentially unapplied property changes
	// and when the window has not been created - thus we use the NSWindow
	// class method.
	
	NSUInteger t_window_style;
	ComputeCocoaStyle(t_window_style);
	
	NSRect t_cocoa_frame;
	MCMacPlatformMapScreenMCRectangleToNSRect(p_frame, t_cocoa_frame);
	
	NSRect t_cocoa_content;
	t_cocoa_content = [NSWindow contentRectForFrameRect: t_cocoa_frame styleMask: t_window_style];
	
	MCMacPlatformMapScreenNSRectToMCRectangle(t_cocoa_content, r_content);
}

////////////////////////////////////////////////////////////////////////////////

void MCMacPlatformWindow::ComputeCocoaStyle(NSUInteger& r_cocoa_style)
{
	NSUInteger t_window_style;
	t_window_style = NSBorderlessWindowMask;
	if (m_has_title_widget)
		t_window_style |= NSTitledWindowMask;
	if (m_has_close_widget)
		t_window_style |= NSClosableWindowMask;
	if (m_has_collapse_widget)
		t_window_style |= NSMiniaturizableWindowMask;
	if (m_has_size_widget)
		t_window_style |= NSResizableWindowMask;
	if (m_style == kMCPlatformWindowStylePalette)
		t_window_style |= NSUtilityWindowMask;
	r_cocoa_style = t_window_style;
}

////////////////////////////////////////////////////////////////////////////////

void MCPlatformWindowMaskCreate(int32_t p_width, int32_t p_height, int32_t p_stride, void *p_bits, MCPlatformWindowMaskRef& r_mask)
{
	r_mask = nil;
}

void MCPlatformWindowMaskRetain(MCPlatformWindowMaskRef p_mask)
{
}

void MCPlatformWindowMaskRelease(MCPlatformWindowMaskRef p_mask)
{
}

////////////////////////////////////////////////////////////////////////////////

void MCMacPlatformCreateWindow(MCPlatformWindowRef& r_window)
{
	r_window = new MCMacPlatformWindow;
}

////////////////////////////////////////////////////////////////////////////////

#if 0
#include "mac-internal.h"
////////////////////////////////////////////////////////////////////////////////

static void MCCocoaPlatformRealizeWindow(MCPlatformWindowRef window);
static void MCCocoaPlatformShowWindow(MCPlatformWindowRef window);
static void MCCocoaPlatformHideWindow(MCPlatformWindowRef window);
static void MCCocoaPlatformFocusWindow(MCPlatformWindowRef window);
static void MCCocoaPlatformRaiseWindow(MCPlatformWindowRef window);
static void MCCocoaPlatformSetWindowTitle(MCPlatformWindowRef window, NSString*& x_title, const char *p_value);
static void MCCocoaPlatformGetWindowTitle(MCPlatformWindowRef window, NSString*& x_title, char*& r_value);
static void MCCocoaPlatformSetWindowMask(MCPlatformWindowRef window, CGImageRef& x_mask, const MCPlatformWindowMask& mask);
static void MCCocoaPlatformGetWindowMask(MCPlatformWindowRef window, CGImageRef& x_mask, MCPlatformWindowMask& r_mask);
static void MCCocoaPlatformMapWindowContentRectToFrameRect(MCPlatformWindowRef window, MCRectangle content, MCRectangle& r_frame);
static void MCCocoaPlatformMapWindowFrameRectToContentRect(MCPlatformWindowRef window, MCRectangle frame, MCRectangle& r_content);
static void MCCocoaPlatformComputeWindowStyle(MCPlatformWindowRef window, NSUInteger& r_cocoa_style);
static void MCCocoaPlatformSyncWindow(MCPlatformWindowRef p_window);
static void MCCocoaPlatformUpdateWindow(MCPlatformWindowRef p_window, MCRegionRef p_region);


////////////////////////////////////////////////////////////////////////////////

@implementation com_runrev_livecode_MCWindowView

//////////

- (id)initWithFrame:(NSRect)frameRect
{
	self = [super initWithFrame: frameRect];
	if (self == nil)
		return nil;
	
	m_tracking_area = nil;
	
	return self;
}

- (void)dealloc
{
	[m_tracking_area release];
	[super dealloc];
}

/////////

- (void)updateTrackingAreas
{
	[super updateTrackingAreas];
	
	// COCOA-TODO: Make sure this is necessary, apparantly things should
	//   automagically resize with InVisibleRect.
	
	if (m_tracking_area != nil)
	{
		[self removeTrackingArea: m_tracking_area];
		[m_tracking_area release];
		m_tracking_area = nil;
	}
	
	m_tracking_area = [[NSTrackingArea alloc] initWithRect: [self bounds]
												   options: (NSTrackingMouseEnteredAndExited | 
															 NSTrackingMouseMoved | 
															 NSTrackingActiveAlways | 
															 NSTrackingInVisibleRect | 
															 NSTrackingEnabledDuringMouseDrag)
													 owner: self
												  userInfo: nil];
	[self addTrackingArea: m_tracking_area];
}

//////////

- (BOOL)isFlipped
{
	return NO;
}

- (BOOL)acceptsFirstMouse:(NSEvent *)theEvent
{
	return YES;
}

- (BOOL)acceptsFirstResponder
{
	return YES;
}

- (BOOL)becomeFirstResponder
{
	return YES;
}

- (BOOL)resignFirstResponder
{
	return YES;
}

//////////

- (void)mouseDown: (NSEvent *)event
{
}

- (void)mouseUp: (NSEvent *)event
{
}

- (void)mouseMoved: (NSEvent *)event
{
}

- (void)mouseDragged: (NSEvent *)event
{
}

- (void)rightMouseDown: (NSEvent *)event
{
}

- (void)rightMouseUp: (NSEvent *)event
{
}

- (void)rightMouseMoved: (NSEvent *)event
{
}

- (void)rightMouseDragged: (NSEvent *)event
{
}

- (void)otherMouseDown: (NSEvent *)event
{
}

- (void)otherMouseUp: (NSEvent *)event
{
}

- (void)otherMouseMoved: (NSEvent *)event
{
}

- (void)otherMouseDragged: (NSEvent *)event
{
}

- (void)mouseEntered: (NSEvent *)event
{
}

- (void)mouseExited: (NSEvent *)event
{
}

- (void)flagsChanged: (NSEvent *)event
{
}

- (void)keyDown: (NSEvent *)event
{
}

- (void)keyUp: (NSEvent *)event
{
}

//////////

// COCOA-TODO: Improve dirty rect calc
- (void)drawRect: (NSRect)dirtyRect
{
	MCPlatformWindowRef t_window;
	t_window = [(MCWindowDelegate *)[[self window] delegate] platformWindowRef];
	
	CGContextRef t_graphics;
	t_graphics = (CGContextRef)[[NSGraphicsContext currentContext] graphicsPort];
	
	const NSRect *t_dirty_rects;
	NSInteger t_dirty_rect_count;
	[self getRectsBeingDrawn: &t_dirty_rects count: &t_dirty_rect_count];
	
	MCRegionRef t_dirty_rgn;
	/* UNCHECKED */ MCRegionCreate(t_dirty_rgn);
	for(NSInteger i = 0; i < t_dirty_rect_count; i++)
		/* UNCHECKED */ MCRegionIncludeRect(t_dirty_rgn, [self mapNSRectToMCRectangle: t_dirty_rects[i]]);
	
	//////////
	
	// Save the context state
	CGContextSaveGState(t_graphics);
	
	MCPlatformSurfaceRef t_surface;
	MCPlatformSurfaceCreate(t_window, t_graphics, t_dirty_rgn, t_surface);
	MCPlatformCallbackSendWindowRedraw(t_window, t_surface, t_dirty_rgn);
	MCPlatformSurfaceDestroy(t_surface);
	
	// Restore the context state
	CGContextRestoreGState(t_graphics);
	
	//////////
	
	MCRegionDestroy(t_dirty_rgn);
	
#ifdef PRE_PLATFORM
	CGContextRef t_graphics;
	t_graphics = (CGContextRef)[[NSGraphicsContext currentContext] graphicsPort];
	
	MCRegionRef t_dirty_rgn;
	MCRegionCreate(t_dirty_rgn);
	MCRegionSetRect(t_dirty_rgn, NSRectToMCRectangleLocal(self, dirtyRect));
	
	// IM-2013-08-23: [[ ResIndependence ]] provide surface height in device scale
	MCGFloat t_scale;
	t_scale = MCResGetDeviceScale();
	
	// IM-2013-10-10: [[ FullscreenMode ]] Use height of view instead of card
	int32_t t_surface_height;
	t_surface_height = floor(m_stack->view_getrect().height * t_scale);
	
	// Save the context state
	CGContextSaveGState(t_graphics);
	
	MCMacStackSurface t_surface(m_stack, t_surface_height, (MCRegionRef)t_dirty_rgn, t_graphics);
	
	if (t_surface.Lock())
	{
		// If we don't have an update pixmap, then use redrawwindow.
		if (s_update_callback == nil)
			m_stack -> device_redrawwindow(&t_surface, (MCRegionRef)t_dirty_rgn);
		else
			s_update_callback(&t_surface, (MCRegionRef)t_dirty_rgn, s_update_context);
		t_surface.Unlock();
	}
	
	// Restore the context state
	CGContextRestoreGState(t_graphics);
#endif
}

//////////

- (MCRectangle)mapNSRectToMCRectangle: (NSRect)r
{
	return MCRectangleMake(r . origin . x, [self bounds] . size . height - r . origin . y, r . size . width, r . size . height);
}

- (NSRect)mapMCRectangleToNSRect: (MCRectangle)r
{
	return NSMakeRect(r . x, [self bounds] . size . height - r . y, r . width, r . height);
}

@end

////////////////////////////////////////////////////////////////////////////////

void MCPlatformCreateWindow(MCPlatformWindowRef& r_window)
{
	MCPlatformWindowRef t_window;
	/* UNCHECKED */ MCMemoryNew(t_window);
	
	// We start off with 1 reference.
	t_window -> references = 1;
	
	// Clear the field changes.
	MCMemoryClear(&t_window -> changes, sizeof(t_window -> changes));
	
	// Set all properties to default values.
	t_window -> style = kMCPlatformWindowStyleNone;
	t_window -> opacity = 1.0f;
	t_window -> mask = nil;
	t_window -> content = MCRectangleMake(0, 0, 0, 0);
	t_window -> title = @"";
	t_window -> has_title_widget = false;
	t_window -> has_close_widget = false;
	t_window -> has_collapse_widget = false;
	t_window -> has_zoom_widget = false;
	t_window -> has_size_widget = false;
	t_window -> has_shadow = false;
	t_window -> has_modified_mark = false;
	t_window -> use_live_resizing = false;
	
	// The handle starts off as nil, meaning not yet created.
	t_window -> handle = nil;
	
	// Return the window.
	r_window = t_window;
}

void MCPlatformRetainWindow(MCPlatformWindowRef p_window)
{
	assert(p_window != nil);
	
	p_window -> references += 1;
}

void MCPlatformReleaseWindow(MCPlatformWindowRef p_window)
{
	assert(p_window != nil);
	
	p_window -> references -= 1;
	if (p_window -> references > 0)
		return;
	
	delete p_window;
}

void MCPlatformUpdateWindow(MCPlatformWindowRef p_window, MCRegionRef p_region)
{
	assert(p_window != nil);
	
	if (!p_window -> is_visible)
		return;
	
	MCCocoaPlatformUpdateWindow(p_window, p_region);
}

void MCPlatformShowWindow(MCPlatformWindowRef p_window)
{
	assert(p_window != nil);
	
	// Do nothing if the window has been shown.
	if (p_window -> is_visible)
		return;
	
	// Make sure the window has been created.
	MCCocoaPlatformRealizeWindow(p_window);
	
	// Now present the window.
	MCCocoaPlatformShowWindow(p_window);
	
	// Mark the window as visible.
	p_window -> is_visible = true;
}

void MCPlatformHideWindow(MCPlatformWindowRef p_window)
{
	assert(p_window != nil);
	
	// Do nothing if the window isn't visible.
	if (!p_window -> is_visible)
		return;
	
	// Dismiss the window.
	MCCocoaPlatformHideWindow(p_window);
	
	// Mark the window as invisible.
	p_window -> is_visible = false;
}

void MCPlatformFocusWindow(MCPlatformWindowRef p_window)
{
	assert(p_window != nil);
	
	// Do nothing if the window isn't visible.
	if (!p_window -> is_visible)
		return;
	
	// Do nothing if the window is currently focused.
	if (p_window -> is_focused)
		return;
	
	// Focus the window.
	MCCocoaPlatformFocusWindow(p_window);
	
	// Mark the window as focused.
	p_window -> is_focused = true;
}

void MCPlatformRaiseWindow(MCPlatformWindowRef p_window)
{
	assert(p_window != nil);
	
	// Do nothing if the window isn't visible.
	if (!p_window -> is_visible)
		return;
	
	// Raise the window.
	MCCocoaPlatformRaiseWindow(p_window);
}

void MCPlatformIconifyWindow(MCPlatformWindowRef p_window)
{
}

void MCPlatformUniconifyWindow(MCPlatformWindowRef p_window)
{
}

//////////

void MCPlatformSetWindowProperty(MCPlatformWindowRef p_window, MCPlatformWindowProperty p_property, MCPlatformPropertyType p_type, const void *p_value)
{
	switch(p_property)
	{
		case kMCPlatformWindowPropertyTitle:
			assert(p_type == kMCPlatformPropertyTypeUTF8CString);
			MCCocoaPlatformSetWindowTitle(p_window, p_window -> title, *(const char **)p_value);
			p_window -> changes . title_changed = true;
			break;
		case kMCPlatformWindowPropertyStyle:
			assert(p_type == kMCPlatformPropertyTypeWindowStyle);
			p_window -> style = *(MCPlatformWindowStyle *)p_value;
			p_window -> changes . style_changed = true;
			break;
		case kMCPlatformWindowPropertyOpacity:
			assert(p_type == kMCPlatformPropertyTypeFloat);
			p_window -> opacity = *(float *)p_value;
			p_window -> changes . opacity_changed = true;
			break;
		case kMCPlatformWindowPropertyMask:
			assert(p_type == kMCPlatformPropertyTypeWindowMask);
			MCCocoaPlatformSetWindowMask(p_window, p_window -> mask, *(MCPlatformWindowMask *)p_value);
			p_window -> changes . mask_changed = true;
			break;
		case kMCPlatformWindowPropertyContentRect:
			assert(p_type == kMCPlatformPropertyTypeRectangle);
			p_window -> content = *(MCRectangle *)p_value;
			p_window -> changes . content_changed = true;
			break;
		case kMCPlatformWindowPropertyFrameRect:
			assert(p_type == kMCPlatformPropertyTypeRectangle);
			MCCocoaPlatformMapWindowFrameRectToContentRect(p_window, *(MCRectangle *)p_value, p_window -> content);
			p_window -> changes . content_changed = true;
			break;
		case kMCPlatformWindowPropertyHasTitleWidget:
			assert(p_type == kMCPlatformPropertyTypeBool);
			p_window -> has_title_widget = *(bool *)p_value;
			p_window -> changes . has_title_widget_changed = true;
			break;
		case kMCPlatformWindowPropertyHasCloseWidget:
			assert(p_type == kMCPlatformPropertyTypeBool);
			p_window -> has_close_widget = *(bool *)p_value;
			p_window -> changes . has_close_widget_changed = true;
			break;
		case kMCPlatformWindowPropertyHasCollapseWidget:
			assert(p_type == kMCPlatformPropertyTypeBool);
			p_window -> has_collapse_widget = *(bool *)p_value;
			p_window -> changes . has_collapse_widget_changed = true;
			break;
		case kMCPlatformWindowPropertyHasZoomWidget:
			assert(p_type == kMCPlatformPropertyTypeBool);
			p_window -> has_zoom_widget = *(bool *)p_value;
			p_window -> changes . has_zoom_widget_changed = true;
			break;
		case kMCPlatformWindowPropertyHasSizeWidget:
			assert(p_type == kMCPlatformPropertyTypeBool);
			p_window -> has_size_widget = *(bool *)p_value;
			p_window -> changes . has_size_widget_changed = true;
			break;
		case kMCPlatformWindowPropertyHasShadow:
			assert(p_type == kMCPlatformPropertyTypeBool);
			p_window -> has_shadow = *(bool *)p_value;
			p_window -> changes . has_shadow_changed = true;
			break;
		case kMCPlatformWindowPropertyHasModifiedMark:
			assert(p_type == kMCPlatformPropertyTypeBool);
			p_window -> has_modified_mark = *(bool *)p_value;
			p_window -> changes . has_modified_mark_changed = true;
			break;
		case kMCPlatformWindowPropertyUseLiveResizing:
			assert(p_type == kMCPlatformPropertyTypeBool);
			p_window -> use_live_resizing = *(bool *)p_value;
			p_window -> changes . use_live_resizing_changed = true;
			break;
		default:
			assert(false);
			break;
	}
	
	MCCocoaPlatformSyncWindow(p_window);
}

void MCPlatformGetWindowProperty(MCPlatformWindowRef p_window, MCPlatformWindowProperty p_property, MCPlatformPropertyType p_type, void *r_value)
{
	switch(p_property)
	{
		case kMCPlatformWindowPropertyTitle:
			assert(p_type == kMCPlatformPropertyTypeUTF8CString);
			
			break;
		case kMCPlatformWindowPropertyStyle:
			assert(p_type == kMCPlatformPropertyTypeWindowStyle);
			break;
		case kMCPlatformWindowPropertyOpacity:
			assert(p_type == kMCPlatformPropertyTypeFloat);
			break;
		case kMCPlatformWindowPropertyMask:
			assert(p_type == kMCPlatformPropertyTypeWindowMask);
			break;
		case kMCPlatformWindowPropertyFrameRect:
			assert(p_type == kMCPlatformPropertyTypeRectangle);
			break;
		case kMCPlatformWindowPropertyContentRect:
			assert(p_type == kMCPlatformPropertyTypeRectangle);
			break;
		case kMCPlatformWindowPropertyHasTitleWidget:
			assert(p_type == kMCPlatformPropertyTypeBool);
			break;
		case kMCPlatformWindowPropertyHasCloseWidget:
			assert(p_type == kMCPlatformPropertyTypeBool);
			break;
		case kMCPlatformWindowPropertyHasCollapseWidget:
			assert(p_type == kMCPlatformPropertyTypeBool);
			break;
		case kMCPlatformWindowPropertyHasZoomWidget:
			assert(p_type == kMCPlatformPropertyTypeBool);
			break;
		case kMCPlatformWindowPropertyHasSizeWidget:
			assert(p_type == kMCPlatformPropertyTypeBool);
			break;
		case kMCPlatformWindowPropertyHasShadow:
			assert(p_type == kMCPlatformPropertyTypeBool);
			break;
		case kMCPlatformWindowPropertyHasModifiedMark:
			assert(p_type == kMCPlatformPropertyTypeBool);
			break;
		case kMCPlatformWindowPropertyUseLiveResizing:
			assert(p_type == kMCPlatformPropertyTypeBool);
			break;
		default:
			assert(false);
			break;
	}
}

//////////

void MCPlatformSetWindowContentRect(MCPlatformWindowRef p_window, MCRectangle p_content_rect)
{
	MCPlatformSetWindowProperty(p_window, kMCPlatformWindowPropertyContentRect, kMCPlatformPropertyTypeRectangle, &p_content_rect);
}

void MCPlatformGetWindowContentRect(MCPlatformWindowRef p_window, MCRectangle& r_content_rect)
{
	MCPlatformGetWindowProperty(p_window, kMCPlatformWindowPropertyContentRect, kMCPlatformPropertyTypeRectangle, &r_content_rect);
}

void MCPlatformSetWindowFrameRect(MCPlatformWindowRef p_window, MCRectangle p_frame_rect)
{
	MCPlatformSetWindowProperty(p_window, kMCPlatformWindowPropertyFrameRect, kMCPlatformPropertyTypeRectangle, &p_frame_rect);
}

void MCPlatformGetWindowFrameRect(MCPlatformWindowRef p_window, MCRectangle& r_frame_rect)
{
	MCPlatformSetWindowProperty(p_window, kMCPlatformWindowPropertyFrameRect, kMCPlatformPropertyTypeRectangle, &r_frame_rect);
}

//////////

void MCPlatformSetWindowBoolProperty(MCPlatformWindowRef p_window, MCPlatformWindowProperty p_property, bool p_value)
{
	MCPlatformSetWindowProperty(p_window, p_property, kMCPlatformPropertyTypeBool, &p_value);
}

void MCPlatformSetWindowFloatProperty(MCPlatformWindowRef p_window, MCPlatformWindowProperty p_property, float p_value)
{
	MCPlatformSetWindowProperty(p_window, p_property, kMCPlatformPropertyTypeFloat, &p_value);
}

///////////////////////////////////////////////////////////////////////////////

static void MCCocoaPlatformRealizeWindow(MCPlatformWindowRef p_window)
{
	if (p_window -> handle != nil)
		return;
	
	int t_window_level;
	switch(p_window -> style)
	{
		case kMCPlatformWindowStyleNone:
		case kMCPlatformWindowStyleDocument:
			t_window_level = kCGNormalWindowLevelKey;
			break;
		case kMCPlatformWindowStylePalette:
			t_window_level = kCGFloatingWindowLevelKey;
			break;
		case kMCPlatformWindowStyleDialog:
			t_window_level = kCGModalPanelWindowLevelKey;
			break;
		case kMCPlatformWindowStyleUtility:
			t_window_level = kCGUtilityWindowLevelKey;
			break;
		case kMCPlatformWindowStylePopUp:
			t_window_level = kCGPopUpMenuWindowLevelKey;
			break;
		case kMCPlatformWindowStyleToolTip:
			t_window_level = kCGStatusWindowLevelKey;
			break;
		default:
			assert(false);
			break;
	}
	
	NSUInteger t_window_style;
	MCCocoaPlatformComputeWindowStyle(p_window, t_window_style);
	
	NSRect t_cocoa_content;
	MCCocoaPlatformMapScreenMCRectangleToNSRect(p_window -> content, t_cocoa_content);
	
	if (t_window_level != kCGFloatingWindowLevelKey)
		p_window -> handle_as_window = [[NSWindow alloc] initWithContentRect: t_cocoa_content styleMask: t_window_style backing: NSBackingStoreBuffered defer: YES];
	else
		p_window -> handle_as_panel = [[NSPanel alloc] initWithContentRect: t_cocoa_content styleMask: t_window_style backing: NSBackingStoreBuffered defer: YES];
	
	[p_window -> handle_as_window setLevel: t_window_level];
	[p_window -> handle_as_window setOpaque: p_window -> mask != nil];
	[p_window -> handle_as_window setHasShadow: p_window -> has_shadow];
	if (!p_window -> has_zoom_widget)
		[[p_window -> handle_as_window standardWindowButton: NSWindowZoomButton] setEnabled: NO];
	[p_window -> handle_as_window setAlphaValue: p_window -> opacity];
	[p_window -> handle_as_window setDocumentEdited: p_window -> has_modified_mark];
	
	// COCOA-TODO: live resizing
	
}

static void MCCocoaPlatformUnrealizeWindow(MCPlatformWindowRef p_window)
{
	if (p_window -> handle == nil)
		return;
	
	[p_window -> handle_as_window release];
	
	p_window -> handle = nil;
}

static void MCCocoaPlatformSyncWindow(MCPlatformWindowRef p_window)
{
	if (p_window -> handle == nil)
		return;
	
	// COCOA-TODO: Sort out changes that affect window type (not needed at the moment
	//   since the engine recreates such).
	if (p_window -> changes . style_changed)
	{
	}
	
	if (p_window -> changes . opacity_changed)
		[p_window -> handle_as_window setAlphaValue: p_window -> opacity];
	
	if (p_window -> changes . mask_changed)
	{
		[p_window -> handle_as_window setOpaque: p_window -> mask != nil];
		if (p_window -> has_shadow)
			p_window -> shadow_changed = true;
	}
	
	if (p_window -> changes . content_changed)
	{
		NSRect t_cocoa_content;
		MCCocoaPlatformMapScreenMCRectangleToNSRect(p_window -> content, t_cocoa_content);
		
		NSRect t_cocoa_frame;
		t_cocoa_frame = [p_window -> handle_as_window frameRectForContentRect: t_cocoa_content];
		
		[p_window -> handle_as_window setFrame: t_cocoa_frame display: NO];
	}
	
	if (p_window -> changes . title_changed)
		[p_window -> handle_as_window setTitle:	p_window -> title];
	
	if (p_window -> changes . has_modified_mark_changed)
		[p_window -> handle_as_window setDocumentEdited: p_window -> has_modified_mark];
	
	if (p_window -> changes . use_live_resizing_changed)
		;
	
	MCMemoryClear(&p_window -> changes, sizeof(p_window -> changes));
}

static void MCCocoaPlatformShowWindow(MCPlatformWindowRef p_window)
{
	assert(p_window -> handle != nil);
	
	[p_window -> handle_as_window makeKeyAndOrderFront: nil];
}

static void MCCocoaPlatformHideWindow(MCPlatformWindowRef p_window)
{
	assert(p_window -> handle != nil);
	
	[p_window -> handle_as_window orderOut: nil];
}

static void MCCocoaPlatformFocusWindow(MCPlatformWindowRef p_window)
{
}

static void MCCocoaPlatformRaiseWindow(MCPlatformWindowRef p_window)
{
	assert(p_window -> handle != nil);
	
	[p_window -> handle_as_window orderFront: nil];
}

static void MCCocoaPlatformUpdateWindow(MCPlatformWindowRef p_window, MCRegionRef p_region)
{
}

static void MCCocoaPlatformSetWindowTitle(MCPlatformWindowRef p_window, NSString*& x_title, const char *p_value)
{
	if (x_title != nil)
		[x_title release];
	x_title = [[NSString alloc] initWithCString: p_value encoding: NSUTF8StringEncoding];
}

static void MCCocoaPlatformGetWindowTitle(MCPlatformWindowRef p_window, NSString*& x_title, char*& r_value)
{
	r_value = strdup([x_title cStringUsingEncoding: NSUTF8StringEncoding]);
}

static void MCCocoaPlatformSetWindowMask(MCPlatformWindowRef p_window, CGImageRef& x_mask, const MCPlatformWindowMask& p_mask)
{
	if (p_window -> mask != nil)
		CGImageRelease(p_window -> mask);
	
	if (p_mask . bits == nil)
	{
		p_window -> mask = nil;
		return;
	}
	
	// COCOA-TODO: Build CGImageRef from mask.
}

static void MCCocoaPlatformGetWindowMask(MCPlatformWindowRef p_window, CGImageRef& x_mask, MCPlatformWindowMask& r_mask)
{
	// COCOA-TODO: Fetch image mask
	//if (p_window -> mask == nil)
	{
		MCMemoryClear(&r_mask, sizeof(r_mask));
		return;
	}
}

static void MCCocoaPlatformMapWindowContentRectToFrameRect(MCPlatformWindowRef p_window, MCRectangle p_content, MCRectangle& r_frame)
{
	NSUInteger t_window_style;
	MCCocoaPlatformComputeWindowStyle(p_window, t_window_style);
	
	NSRect t_cocoa_content;
	MCCocoaPlatformMapScreenMCRectangleToNSRect(p_content, t_cocoa_content);
	
	NSRect t_cocoa_frame;
	t_cocoa_frame = [NSWindow frameRectForContentRect: t_cocoa_content styleMask: t_window_style];
	
	MCCocoaPlatformMapScreenNSRectToMCRectangle(t_cocoa_frame, r_frame);
}

static void MCCocoaPlatformMapWindowFrameRectToContentRect(MCPlatformWindowRef p_window, MCRectangle p_frame, MCRectangle& r_content)
{
	NSUInteger t_window_style;
	MCCocoaPlatformComputeWindowStyle(p_window, t_window_style);
	
	NSRect t_cocoa_frame;
	MCCocoaPlatformMapScreenMCRectangleToNSRect(p_frame, t_cocoa_frame);
	
	NSRect t_cocoa_content;
	t_cocoa_content = [NSWindow contentRectForFrameRect: t_cocoa_frame styleMask: t_window_style];
	
	MCCocoaPlatformMapScreenNSRectToMCRectangle(t_cocoa_content, r_content);
}

void MCCocoaPlatformComputeWindowStyle(MCPlatformWindowRef p_window, NSUInteger& r_cocoa_style)
{
	NSUInteger t_window_style;
	t_window_style = NSBorderlessWindowMask;
	if (p_window -> has_title_widget)
		t_window_style |= NSTitledWindowMask;
	if (p_window -> has_close_widget)
		t_window_style |= NSClosableWindowMask;
	if (p_window -> has_collapse_widget)
		t_window_style |= NSMiniaturizableWindowMask;
	if (p_window -> has_size_widget)
		t_window_style |= NSResizableWindowMask;
	if (p_window -> style == kMCPlatformWindowStylePalette)
		t_window_style |= NSUtilityWindowMask;
	r_cocoa_style = t_window_style;
}

///////////////////////////////////////////////////////////////////////////////
#endif
