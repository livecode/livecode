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
#include "typedefs.h"
#include "platform.h"
#include "platform-internal.h"

#include "mac-internal.h"

////////////////////////////////////////////////////////////////////////////////

static bool s_have_primary_screen_height = false;
static CGFloat s_primary_screen_height = 0.0f;

////////////////////////////////////////////////////////////////////////////////

@implementation com_runrev_livecode_MCApplicationDelegate

//////////

- (id)initWithArgc:(int)argc argv:(char**)argv envp:(char **)envp
{
	self = [super init];
	if (self == nil)
		return nil;
	
	m_argc = argc;
	m_argv = argv;
	m_envp = envp;
	
	return self;
}

//////////

- (NSError *)application:(NSApplication *)application willPresentError:(NSError *)error
{
}

//////////

- (BOOL)applicationShouldHandleReopen:(NSApplication *)sender hasVisibleWindows:(BOOL)flag
{
	return NO;
}

//////////

- (void)applicationWillFinishLaunching: (NSNotification *)notification
{
}

- (void)applicationDidFinishLaunching: (NSNotification *)notification
{
	// Dispatch the startup callback.
	int t_error_code;
	char *t_error_message;
	MCPlatformCallbackSendApplicationStartup(m_argc, m_argv, m_envp, t_error_code, t_error_message);
	
	// If the error code is non-zero, startup failed so quit.
	if (t_error_code != 0)
	{
		// If the error message is non-nil, report it in a suitable way.
		if (t_error_message!= nil)
		{
			fprintf(stderr, "Startup error - %s\n", t_error_message);
			free(t_error_message);
		}
			
		// Now exit the application with the appropriate code.
		exit(t_error_code);
	}

	// We started up successfully, so queue the root runloop invocation
	// message.
	[self performSelector: @selector(runMainLoop) withObject: nil afterDelay: 0];
}

- (void)runMainLoop
{
	MCPlatformCallbackSendApplicationRun();
	[NSApp terminate: self];
}

//////////

// This is sent when the last window closes - as LiveCode apps are expected
// to control termination (via quit), we always say 'NO' don't close.
- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)sender
{
	return NO;
}

- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication *)sender
{
	// There is an NSApplicationTerminateReplyLater result code which will place
	// the runloop in a modal loop for exit dialogs. We'll try the simpler
	// option for now of just sending the callback and seeing what AppKit does
	// with the (eventual) event loop that will result...
	bool t_terminate;
	MCPlatformCallbackSendApplicationShutdownRequest(t_terminate);
	
	if (t_terminate)
		return NSTerminateNow;
	
	return NSTerminateCancel;
}

- (void)applicationWillTerminate:(NSNotification *)notification
{
	// Dispatch the shutdown callback.
	int t_exit_code;
	MCPlatformCallbackSendApplicationShutdown(t_exit_code);
	
	// Now exit the application with the appropriate code.
	exit(t_exit_code);
}

// Dock menu handling.
- (NSMenu *)applicationDockMenu:(NSApplication *)sender
{
	return nil;
}

//////////

- (void)applicationWillHide:(NSNotification *)notification;
{
}

- (void)applicationDidHide:(NSNotification *)notification;
{
}

- (void)applicationWillUnhide:(NSNotification *)notification;
{
}

- (void)applicationDidUnhide:(NSNotification *)notification
{
}

//////////

- (void)applicationWillBecomeActive:(NSNotification *)notification
{
}

- (void)applicationDidBecomeActive:(NSNotification *)notification
{
}

- (void)applicationWillResignActive:(NSNotification *)notification
{
}

- (void)applicationDidResignActive:(NSNotification *)notification
{
}

//////////

- (void)applicationWillUpdate:(NSNotification *)notification
{
}

- (void)applicationDidUpdate:(NSNotification *)notification
{
}

//////////

- (void)applicationDidChangeScreenParameters:(NSNotification *)notification
{
	// Make sure we refetch the primary screen height.
	s_have_primary_screen_height = false;
	
	// Dispatch the notification.
	MCPlatformCallbackSendScreenParametersChanged();
}

//////////

- (BOOL)application:(NSApplication *)sender openFile:(NSString *)filename
{
	return NO;
}

- (void)application:(NSApplication *)sender openFiles:(NSArray *)filenames
{
	[NSApp replyToOpenOrPrint: NSApplicationDelegateReplyCancel];
}

- (BOOL)application:(NSApplication *)sender openTempFile:(NSString *)filename
{
	return NO;
}

- (BOOL)applicationShouldOpenUntitledFile:(NSApplication *)sender
{
	return NO;
}

- (BOOL)applicationOpenUntitledFile:(NSApplication *)sender
{
	return NO;
}

- (BOOL)application:(id)sender openFileWithoutUI:(NSString *)filename
{
	return NO;
}

//////////

- (NSApplicationPrintReply)application:(NSApplication *)application printFiles:(NSArray *)fileNames withSettings:(NSDictionary *)printSettings showPrintPanels:(BOOL)showPrintPanels
{
	return NSPrintingCancelled;
}

//////////

@end

////////////////////////////////////////////////////////////////////////////////

void MCPlatformGetSystemProperty(MCPlatformSystemProperty p_property, MCPlatformPropertyType p_type, void *r_value)
{
	switch(p_property)
	{
		case kMCPlatformSystemPropertyDoubleClickInterval:
			*(double *)r_value = GetDblTime() * 1000.0 / 60.0;
			break;
			
		case kMCPlatformSystemPropertyCaretBlinkInterval:
			*(double *)r_value = GetCaretTime() * 1000.0 / 60.0;
			break;
			
		case kMCPlatformSystemPropertyHiliteColor:
		{
			RGBColor hiliteRGB;
			LMGetHiliteRGB(&hiliteRGB);
			((MCColor *)r_value) -> red = hiliteRGB.red;
			((MCColor *)r_value) -> green = hiliteRGB.green;
			((MCColor *)r_value) -> blue = hiliteRGB.blue;
		}
		break;
			
		case kMCPlatformSystemPropertyAccentColor:
			// COCOA-TODO: GetAccentColor
			break;
			
		case kMCPlatformSystemPropertyMaximumCursorSize:
			*(int32_t *)r_value = 256;
			break;
		
		case kMCPlatformSystemPropertyCursorImageSupport:
			*(MCPlatformCursorImageSupport *)r_value = kMCPlatformCursorImageSupportAlpha; 
			break;
			
		default:
			assert(false);
			break;
	}
}

////////////////////////////////////////////////////////////////////////////////

static NSEvent *s_last_mouse_event = nil;

static CFRunLoopObserverRef s_observer = nil;
static bool s_in_blocking_wait = false;
static bool s_wait_broken = false;

enum
{
	kMCMacPlatformBreakEvent = 0,
	kMCMacPlatformMouseSyncEvent = 1,
};

void MCPlatformBreakWait(void)
{
	//NSLog(@"Application -> BreakWait()");
	
	if (s_wait_broken)
		return;
	
	s_wait_broken = true;
	
	NSAutoreleasePool *t_pool;
	t_pool = [[NSAutoreleasePool alloc] init];
	
	NSEvent *t_event;
	t_event = [NSEvent otherEventWithType:NSApplicationDefined
								 location:NSMakePoint(0, 0)
							modifierFlags:0
								timestamp:0
							 windowNumber:0
								  context:NULL
								  subtype:kMCMacPlatformBreakEvent
									data1:0
									data2:0];
	
	// COCOA-TODO: Make this not leak!
	[NSApp postEvent: t_event
			 atStart: YES];
	
	[t_pool release];
}

static void runloop_observer(CFRunLoopObserverRef observer, CFRunLoopActivity activity, void *info)
{
	if (s_in_blocking_wait)
		MCPlatformBreakWait();
}

bool MCPlatformWaitForEvent(double p_duration, bool p_blocking)
{
	//NSLog(@"Application -> WaitForEvent(%lf, %d)", p_duration, p_blocking);
	
	// Make sure we have our observer and install it. This is used when we are
	// blocking and should break the event loop whenever a new event is added
	// to the queue.
	if (s_observer == nil)
	{
		s_observer = CFRunLoopObserverCreate(kCFAllocatorDefault, kCFRunLoopAfterWaiting, true, 0, runloop_observer, NULL);
		CFRunLoopAddObserver([[NSRunLoop currentRunLoop] getCFRunLoop], s_observer, (CFStringRef)NSEventTrackingRunLoopMode);
	}
	
	s_in_blocking_wait = true;
	s_wait_broken = false;
	
	NSAutoreleasePool *t_pool;
	t_pool = [[NSAutoreleasePool alloc] init];
	
	NSEvent *t_event;
	t_event = [NSApp nextEventMatchingMask: p_blocking ? NSApplicationDefinedMask : NSAnyEventMask
								 untilDate: [NSDate dateWithTimeIntervalSinceNow: p_duration]
									inMode: p_blocking ? NSEventTrackingRunLoopMode : NSDefaultRunLoopMode
								   dequeue: YES];

	s_in_blocking_wait = false;

	if (t_event != nil)
	{
		[s_last_mouse_event release];
		s_last_mouse_event = nil;
		
		if ([t_event type] == NSApplicationDefined)
		{
			if ([t_event subtype] == kMCMacPlatformMouseSyncEvent)
				MCMacPlatformHandleMouseSync();
		}
		else if ([t_event type] == NSLeftMouseDown || [t_event type] == NSLeftMouseDragged)
		{
			s_last_mouse_event = t_event;
			[t_event retain];
			[NSApp sendEvent: t_event];
		}
		else
			[NSApp sendEvent: t_event];
	}
	
	[t_pool release];
	
	return t_event != nil;
}

// COCOA-TODO: abort key
bool MCPlatformGetAbortKeyPressed(void)
{
	return false;
}

bool MCPlatformGetMouseButtonState(uindex_t p_button)
{
	NSUInteger t_buttons;
	t_buttons = [NSEvent pressedMouseButtons];
	if (p_button == 0)
		return t_buttons != 0;
	if (p_button == 1)
		return (t_buttons & (1 << 0)) != 0;
	if (p_button == 2)
		return (t_buttons & (1 << 2)) != 0;
	if (p_button == 3)
		return (t_buttons & (1 << 1)) != 0;
	return (t_buttons & (1 << (p_button - 1))) != 0;
}

bool MCPlatformGetMouseClick(uindex_t p_button, MCPoint& r_location)
{
	// We want to try and remove a whole click from the queue. Which button
	// is determined by p_button and if zero means any button. So, first
	// we search for a mouseDown.
	
	NSAutoreleasePool *t_pool;
	t_pool = [[NSAutoreleasePool alloc] init];
	
	NSUInteger t_down_mask;
	t_down_mask = 0;
	if (p_button == 0 || p_button == 1) 
		t_down_mask |= NSLeftMouseDownMask;
	if (p_button == 0 || p_button == 3) 
		t_down_mask |= NSRightMouseDownMask;
	if (p_button == 0 || p_button == 2) 
		t_down_mask |= NSOtherMouseDownMask;
	
	NSEvent *t_down_event;
	t_down_event = [NSApp nextEventMatchingMask: t_down_mask untilDate: nil inMode: NSEventTrackingRunLoopMode dequeue: NO];
	
	// If there is no mouse down event then there is no click.
	if (t_down_event == nil)
	{
		[t_pool release];
		return false;
	}
	
	// Now search for a matching mouse up event.
	NSUInteger t_up_mask;
	if ([t_down_event buttonNumber] == 0)
	{
		t_down_mask = NSLeftMouseDownMask;
		t_up_mask = NSLeftMouseUpMask;
	}
	else if ([t_down_event buttonNumber] == 1)
	{
		t_down_mask = NSRightMouseDownMask;
		t_up_mask = NSRightMouseUpMask;
	}
	else
	{
		t_down_mask = NSOtherMouseDownMask;
		t_up_mask = NSOtherMouseUpMask;
	}
	
	NSEvent *t_up_event;
	t_up_event = [NSApp nextEventMatchingMask: t_up_mask untilDate: nil inMode: NSEventTrackingRunLoopMode dequeue: NO];
	
	// If there is no mouse up event then there is no click.
	if (t_up_event == nil)
	{
		[t_pool release];
		return false;
	}
	
	// If the up event preceeds the down event, there is no click.
	if ([t_down_event timestamp] > [t_up_event timestamp])
	{
		[t_pool release];
		return false;
	}
	
	// Otherwise, clear out all dragged / move etc. messages up to the mouse up event.
	[NSApp discardEventsMatchingMask: NSLeftMouseDraggedMask |
										NSRightMouseDraggedMask |
											NSOtherMouseDraggedMask |
												NSMouseMovedMask |
													NSMouseEnteredMask |
														NSMouseExitedMask
						 beforeEvent: t_up_event];
	
	// And finally deque the up event and down event.
	t_down_event = [NSApp nextEventMatchingMask: t_down_mask untilDate: nil inMode: NSEventTrackingRunLoopMode dequeue: YES];
	t_up_event = [NSApp nextEventMatchingMask: t_up_mask untilDate: nil inMode: NSEventTrackingRunLoopMode dequeue: YES];
	
	// Fetch its location.
	NSPoint t_screen_loc;
	if ([t_up_event window] != nil)
		t_screen_loc = [[t_up_event window] convertBaseToScreen: [t_up_event locationInWindow]];
	else
		t_screen_loc = [t_up_event locationInWindow];
	
	MCMacPlatformMapScreenNSPointToMCPoint(t_screen_loc, r_location);
	
	[t_pool release];
	
	return true;
}

void MCPlatformGetMousePosition(MCPoint& r_location)
{
	MCMacPlatformMapScreenNSPointToMCPoint([NSEvent mouseLocation], r_location);
}

void MCPlatformSetMousePosition(MCPoint p_location)
{
	CGPoint t_point;
	t_point . x = p_location . x;
	t_point . y = p_location . y;
	CGWarpMouseCursorPosition(t_point);
}

void MCPlatformGetWindowAtPoint(MCPoint p_loc, MCPlatformWindowRef& r_window)
{
	NSPoint t_loc_cocoa;
	MCMacPlatformMapScreenMCPointToNSPoint(p_loc, t_loc_cocoa);
	
	NSInteger t_number;
	t_number = [NSWindow windowNumberAtPoint: t_loc_cocoa belowWindowWithWindowNumber: 0];
	
	NSWindow *t_window;
	t_window = [NSApp windowWithWindowNumber: t_number];
	
	if (t_window != nil &&
		[[t_window delegate] isKindOfClass: [MCWindowDelegate class]] &&
		NSPointInRect(t_loc_cocoa, [t_window contentRectForFrameRect: [t_window frame]]))
		r_window = [(MCWindowDelegate *)[t_window delegate] platformWindow];
	else
		r_window = nil;
}

uint32_t MCPlatformGetEventTime(void)
{
	return [[NSApp currentEvent] timestamp] * 1000.0;
}

NSEvent *MCMacPlatformGetLastMouseEvent(void)
{
	return s_last_mouse_event;
}

////////////////////////////////////////////////////////////////////////////////

void MCPlatformGetScreenCount(uindex_t& r_count)
{
	r_count = [[NSScreen screens] count];
}

void MCPlatformGetScreenViewport(uindex_t p_index, MCRectangle& r_viewport)
{
	NSRect t_viewport;
	t_viewport = [[[NSScreen screens] objectAtIndex: p_index] frame];
	if (p_index == 0)
	{
		s_have_primary_screen_height = true;
		s_primary_screen_height = t_viewport . size . height;
		
		r_viewport . x = t_viewport . origin . x;
		r_viewport . y = t_viewport . origin . y;
		r_viewport . width = t_viewport . size . width;
		r_viewport . height = t_viewport . size . height;
		return;
	}
	
	MCMacPlatformMapScreenNSRectToMCRectangle(t_viewport, r_viewport);
}

void MCPlatformGetScreenWorkarea(uindex_t p_index, MCRectangle& r_workarea)
{
	MCMacPlatformMapScreenNSRectToMCRectangle([[[NSScreen screens] objectAtIndex: p_index] visibleFrame], r_workarea);
}

////////////////////////////////////////////////////////////////////////////////

// Our mouse handling code relies on getting a stream of mouse move messages
// with screen co-ordinates, and mouse press messages indicating button state
// changes. As we need to handle things like mouse grabbing, and windows popping
// up and moving under the mouse we don't rely on enter/leave from the OS,
// instead we do it ourselves to ensure we never get into unpleasant situations.

// For this to work, we just need the OS to provide us with:
//   - mouse press messages within our own windows
//   - mouse move messages when the mouse moves over our windows *and* when
//     the mouse is down and the mouse is outside our windows.

// If this is true, then the mouse is currently grabbed so we should defer
// switching active window until ungrabbed.
static bool s_mouse_grabbed = false;

// This is the currently active window (the one receiving mouse events).
static MCPlatformWindowRef s_mouse_window = nil;

// This is the current mask of buttons that are pressed.
static uint32_t s_mouse_buttons = 0;

// This is the button that is being dragged (if not 0xffffffff).
static uint32_t s_mouse_drag_button = 0xffffffff;

// This is the number of successive clicks detected on the primary button.
static uint32_t s_mouse_click_count = 0;

// This is the button of the last click (mouseDown then mouseUp) that was
// detected.
static uint32_t s_mouse_last_click_button = 0;

// This is the time of the last mouseUp, used to detect multiple clicks.
static uint32_t s_mouse_last_click_time = 0;

// This is the screen position of the last click, used to detect multiple
// clicks.
static MCPoint s_mouse_last_click_screen_position = { 0, 0 };

// This is the window location in the mouse window that we last posted
// a position event for.
static MCPoint s_mouse_position = { INT16_MIN, INT16_MAX };

// This is the last screen location we received a mouse move message for.
static MCPoint s_mouse_screen_position;

// COCOA-TODO: Clean up this external dependency.
extern uint2 MCdoubledelta;
extern uint2 MCdoubletime;
extern uint2 MCdragdelta;

void MCPlatformGrabPointer(MCPlatformWindowRef p_window)
{
	// If we are grabbing for the given window already, do nothing.
	if (s_mouse_grabbed && p_window == s_mouse_window)
		return;
	
	// If the mouse window is already w, then just grab.
	if (p_window == s_mouse_window)
	{
		s_mouse_grabbed = true;
		return;
	}
	
	// Otherwise do nothing - the mouse window must be w for us to grab.
	// (If we don't have this rule, then strange things could happen with
	//  mouse presses in different windows!).
}

void MCPlatformUngrabPointer(void)
{
	// If buttons are down, then ungrab will happen when they are released.
	if (s_mouse_buttons != 0)
		return;
	
	// Otherwise just turn off the grabbed flag.
	s_mouse_grabbed = false;
}

void MCMacPlatformHandleMousePress(uint32_t p_button, bool p_new_state)
{
	bool t_state;
	t_state = (s_mouse_buttons & (1 << p_button)) != 0;
	
	// If the state is not different from the new state, do nothing.
	if (p_new_state == t_state)
		return;
	
	// Update the state.
	if (p_new_state)
		s_mouse_buttons |= (1 << p_button);
	else
		s_mouse_buttons &= ~(1 << p_button);
	
	// If we are grabbed, and mouse buttons are zero, then ungrab.
	// If mouse buttons are zero, then reset the drag button.
	if (s_mouse_buttons == 0)
	{
		s_mouse_grabbed = false;
		s_mouse_drag_button = 0xffffffff;
	}
		
	// If mouse buttons are non-zero, then grab.
	if (s_mouse_buttons != 0)
		s_mouse_grabbed = true;
	
	// Determine the press state - this can be down, up or release. If
	// the new state is 'up', then we must dispatch a release message
	// if the mouse location is not within the window.
	if (p_new_state)
	{
		// Get the time of the mouse press event.
		uint32_t t_event_time;
		t_event_time = MCPlatformGetEventTime();
		
		// If the click occured within the double click time and double click
		// radius *and* if the button is the same as the last clicked button
		// then increment the click count.
		if (t_event_time - s_mouse_last_click_time < MCdoubletime &&
			MCU_abs(s_mouse_last_click_screen_position . x - s_mouse_screen_position . x) < MCdoubledelta &&
			MCU_abs(s_mouse_last_click_screen_position . y - s_mouse_screen_position . y) < MCdoubledelta &&
			s_mouse_last_click_button == p_button)
			s_mouse_click_count += 1;
		else
			s_mouse_click_count = 0;
		
		// Update the last click position / button.
		s_mouse_last_click_button = p_button;
		s_mouse_last_click_screen_position = s_mouse_screen_position;
		
		MCPlatformCallbackSendMouseDown(s_mouse_window, p_button, s_mouse_click_count);
	}
	else
	{
		MCPoint t_global_pos;
		MCPlatformMapPointFromWindowToScreen(s_mouse_window, s_mouse_position, t_global_pos);
		
		Window t_new_mouse_window;
		MCPlatformGetWindowAtPoint(t_global_pos, t_new_mouse_window);
		
		if (t_new_mouse_window == s_mouse_window)
		{
			// If this is the same button as the last mouseDown, then
			// update the click time.
			if (p_button == s_mouse_last_click_button)
				s_mouse_last_click_time = MCPlatformGetEventTime();
			
			MCPlatformCallbackSendMouseUp(s_mouse_window, p_button, s_mouse_click_count);
		}
		else
		{
			// Any release causes us to cancel multi-click tracking.
			s_mouse_click_count = 0;
			s_mouse_last_click_time = 0;
			
			MCPlatformCallbackSendMouseRelease(s_mouse_window, p_button);
		}
	}
}

void MCMacPlatformHandleMouseMove(MCPoint p_screen_loc)
{
	// First compute the window that should be active now.
	MCPlatformWindowRef t_new_mouse_window;
	if (s_mouse_grabbed)
	{
		// If the mouse is grabbed, the mouse window does not change.
		t_new_mouse_window = s_mouse_window;
	}
	else
	{
		// If the mouse is not grabbed, then we must determine which of our
		// window views we are now over.
		MCPlatformGetWindowAtPoint(p_screen_loc, t_new_mouse_window);
	}
	
	// If the mouse window has changed, then we must exit/enter.
	if (t_new_mouse_window != s_mouse_window)
	{
		if (s_mouse_window != nil)
			MCPlatformCallbackSendMouseLeave(s_mouse_window);
		
		if (t_new_mouse_window != nil)
			MCPlatformCallbackSendMouseEnter(t_new_mouse_window);
		
		if (s_mouse_window != nil)
			MCPlatformReleaseWindow(s_mouse_window);
		
		s_mouse_window = t_new_mouse_window;
		
		if (s_mouse_window != nil)
			MCPlatformRetainWindow(s_mouse_window);
	}
	
	// Regardless of whether we post a mouse move, update the screen mouse position.
	s_mouse_screen_position = p_screen_loc;
	
	// If we have a new mouse window, then translate screen loc and update.
	if (s_mouse_window != nil)
	{
		MCPoint t_window_loc;
		MCPlatformMapPointFromScreenToWindow(s_mouse_window, p_screen_loc, t_window_loc);
		
		if (t_window_loc . x != s_mouse_position . x ||
			t_window_loc . y != s_mouse_position . y)
		{
			s_mouse_position = t_window_loc;
			
			// Send the mouse move.
			MCPlatformCallbackSendMouseMove(s_mouse_window, t_window_loc);
			
			// If this is the start of a drag, then send a mouse drag.
			if (s_mouse_buttons != 0 && s_mouse_drag_button == 0xffffffff &&
				(MCU_abs(p_screen_loc . x - s_mouse_last_click_screen_position . x) >= MCdragdelta ||
				 MCU_abs(p_screen_loc . y - s_mouse_last_click_screen_position . y) >= MCdragdelta))
			{
				s_mouse_drag_button = s_mouse_last_click_button;
				MCPlatformCallbackSendMouseDrag(s_mouse_window, s_mouse_drag_button);
			}
		}
	}	
}

void MCMacPlatformHandleMouseSync(void)
{
	if (s_mouse_window != nil)
	{
		for(uindex_t i = 0; i < 3; i++)
			if ((s_mouse_buttons & (1 << i)) != 0)
			{
				s_mouse_buttons &= ~(1 << i);
				MCPlatformCallbackSendMouseRelease(s_mouse_window, i);
			}
	}
	
	s_mouse_grabbed = false;
	s_mouse_drag_button = 0xffffffff;
	s_mouse_click_count = 0;
	s_mouse_last_click_time = 0;

	MCPoint t_location;
	MCMacPlatformMapScreenNSPointToMCPoint([NSEvent mouseLocation], t_location);
	
	MCMacPlatformHandleMouseMove(t_location);
}

void MCMacPlatformSyncMouseAfterTracking(void)
{
	NSEvent *t_event;
	t_event = [NSEvent otherEventWithType:NSApplicationDefined
								 location:NSMakePoint(0, 0)
							modifierFlags:0
								timestamp:0
							 windowNumber:0
								  context:NULL
								  subtype:kMCMacPlatformMouseSyncEvent
									data1:0
									data2:0];
	[NSApp postEvent: t_event atStart: YES];
}

////////////////////////////////////////////////////////////////////////////////

void MCMacPlatformMapScreenMCPointToNSPoint(MCPoint p, NSPoint& r_point)
{
	if (!s_have_primary_screen_height)
	{
		MCRectangle t_viewport;
		MCPlatformGetScreenViewport(0, t_viewport);
	}
	
	r_point = NSMakePoint(p . x, s_primary_screen_height - p . y);
}

void MCMacPlatformMapScreenNSPointToMCPoint(NSPoint p, MCPoint& r_point)
{
	if (!s_have_primary_screen_height)
	{
		MCRectangle t_viewport;
		MCPlatformGetScreenViewport(0, t_viewport);
	}
	
	r_point . x = p . x;
	r_point . y = s_primary_screen_height - p . y;
}

void MCMacPlatformMapScreenMCRectangleToNSRect(MCRectangle r, NSRect& r_rect)
{
	if (!s_have_primary_screen_height)
	{
		MCRectangle t_viewport;
		MCPlatformGetScreenViewport(0, t_viewport);
	}
	
	r_rect = NSMakeRect(r . x, s_primary_screen_height - (r . y + r . height), r . width, r . height);
}

void MCMacPlatformMapScreenNSRectToMCRectangle(NSRect r, MCRectangle& r_rect)
{
	if (!s_have_primary_screen_height)
	{
		MCRectangle t_viewport;
		MCPlatformGetScreenViewport(0, t_viewport);
	}
	
	r_rect = MCRectangleMake(r . origin . x, s_primary_screen_height - (r . origin . y + r . size . height), r . size . width, r . size . height);
}

////////////////////////////////////////////////////////////////////////////////

static void display_reconfiguration_callback(CGDirectDisplayID display, CGDisplayChangeSummaryFlags flags, void *userInfo)
{
	// COCOA-TODO: Make this is a little more discerning (only need to reset if
	//   primary geometry changes).
	s_have_primary_screen_height = false;
}

////////////////////////////////////////////////////////////////////////////////

int main(int argc, char *argv[], char *envp[])
{
	// COCOA-TODO: Elevated slave.
	
	NSAutoreleasePool *t_pool;
	t_pool = [[NSAutoreleasePool alloc] init];
	
	// Create the normal NSApplication object.
	NSApplication *t_application;
	t_application = [NSApplication sharedApplication];
	
	// Register for reconfigurations.
	CGDisplayRegisterReconfigurationCallback(display_reconfiguration_callback, nil);
	
	// Setup our delegate
	com_runrev_livecode_MCApplicationDelegate *t_delegate;
	t_delegate = [[com_runrev_livecode_MCApplicationDelegate alloc] initWithArgc: argc argv: argv envp: envp];
	
	// Assign our delegate
	[t_application setDelegate: t_delegate];
	
	// Run the application - this never returns!
	[t_application run];
	
	// Drain the autorelease pool.
	[t_pool release];
	
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
