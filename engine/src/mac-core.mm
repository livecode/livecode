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

#include <objc/objc-runtime.h>

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

- (void)initializeModules
{
	MCPlatformInitializeColorTransform();
	MCPlatformInitializeAbortKey();
}

- (void)finalizeModules
{
	MCPlatformFinalizeAbortKey();
	MCPlatformFinalizeColorTransform();
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
	// Initialize everything.
	[self initializeModules];
	
	NSAutoreleasePool *t_pool;
	t_pool = [[NSAutoreleasePool alloc] init];
	
	// Dispatch the startup callback.
	int t_error_code;
	char *t_error_message;
	MCPlatformCallbackSendApplicationStartup(m_argc, m_argv, m_envp, t_error_code, t_error_message);
	
	[t_pool release];
	
	// If the error code is non-zero, startup failed so quit.
	if (t_error_code != 0)
	{
		// If the error message is non-nil, report it in a suitable way.
		if (t_error_message!= nil)
		{
			fprintf(stderr, "Startup error - %s\n", t_error_message);
			free(t_error_message);
		}
		
		// Finalize everything
		[self finalizeModules];
		
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
	
	// Finalize everything
	[self finalizeModules];
	
	// Now exit the application with the appropriate code.
	exit(t_exit_code);
}

// Dock menu handling.
- (NSMenu *)applicationDockMenu:(NSApplication *)sender
{
	return MCMacPlatformGetIconMenu();
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
	MCPlatformCallbackSendApplicationResume();
}

- (void)applicationWillResignActive:(NSNotification *)notification
{
	MCPlatformCallbackSendApplicationSuspend();
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
			*(uint16_t *)r_value = GetDblTime() * 1000.0 / 60.0;
			break;
			
		case kMCPlatformSystemPropertyCaretBlinkInterval:
			*(uint16_t *)r_value = GetCaretTime() * 1000.0 / 60.0;
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
			((MCColor *)r_value) -> red = 0x0000;
			((MCColor *)r_value) -> green = 0x0000;
			((MCColor *)r_value) -> blue = 0x8080;
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

struct MCModalSession
{
	NSModalSession session;
	MCMacPlatformWindow *window;
	bool is_done;
};

static MCModalSession *s_modal_sessions = nil;
static uindex_t s_modal_session_count = 0;

struct MCCallback
{
	void (*method)(void *);
	void *context;
};

static MCCallback *s_callbacks = nil;
static uindex_t s_callback_count;

enum
{
	kMCMacPlatformBreakEvent = 0,
	kMCMacPlatformMouseSyncEvent = 1,
};

void MCPlatformBreakWait(void)
{
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
	// Handle all the pending callbacks.
	for(uindex_t i = 0; i < s_callback_count; i++)
		s_callbacks[i] . method(s_callbacks[i] . context);
	MCMemoryDeleteArray(s_callbacks);
	s_callbacks = nil;
	s_callback_count = 0;
	
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
	
	bool t_modal;
	t_modal = s_modal_session_count > 0;
	
	NSAutoreleasePool *t_pool;
	t_pool = [[NSAutoreleasePool alloc] init];
	
	if (t_modal)
		[NSApp runModalSession: s_modal_sessions[s_modal_session_count - 1] . session];
	
	NSEvent *t_event;
	t_event = [NSApp nextEventMatchingMask: p_blocking ? NSApplicationDefinedMask : NSAnyEventMask
								 untilDate: [NSDate dateWithTimeIntervalSinceNow: p_duration]
									inMode: p_blocking ? NSEventTrackingRunLoopMode : (t_modal ? NSModalPanelRunLoopMode : NSDefaultRunLoopMode)
								   dequeue: YES];
	
	s_in_blocking_wait = false;

	if (t_event != nil)
	{
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
		{
			if ([t_event type] == NSLeftMouseUp)
			{
				[s_last_mouse_event release];
				s_last_mouse_event = nil;
			}
			
			[NSApp sendEvent: t_event];
		}
	}
	
	[t_pool release];
	
	return t_event != nil;
}


void MCMacPlatformBeginModalSession(MCMacPlatformWindow *p_window)
{
	/* UNCHECKED */ MCMemoryResizeArray(s_modal_session_count + 1, s_modal_sessions, s_modal_session_count);
	
	s_modal_sessions[s_modal_session_count - 1] . is_done = false;
	s_modal_sessions[s_modal_session_count - 1] . window = p_window;
	p_window -> Retain();
	s_modal_sessions[s_modal_session_count - 1] . session = [NSApp beginModalSessionForWindow: (NSWindow *)(p_window -> GetHandle())];
}

void MCMacPlatformEndModalSession(MCMacPlatformWindow *p_window)
{
	uindex_t t_index;
	for(t_index = 0; t_index < s_modal_session_count; t_index++)
		if (s_modal_sessions[t_index] . window == p_window)
			break;
	
	if (t_index == s_modal_session_count)
		return;
	
	s_modal_sessions[t_index] . is_done = true;
	
	for(uindex_t t_final_index = s_modal_session_count; t_final_index > 0; t_final_index--)
	{
		if (!s_modal_sessions[t_final_index - 1] . is_done)
			return;
		
		[NSApp endModalSession: s_modal_sessions[t_final_index - 1] . session];
		[s_modal_sessions[t_final_index - 1] . window -> GetHandle() orderOut: nil];
		s_modal_sessions[t_final_index - 1] . window -> Release();
		s_modal_session_count -= 1;
	}
}

void MCMacPlatformScheduleCallback(void (*p_callback)(void *), void *p_context)
{
	/* UNCHECKED */ MCMemoryResizeArray(s_callback_count + 1, s_callbacks, s_callback_count);
	s_callbacks[s_callback_count - 1] . method = p_callback;
	s_callbacks[s_callback_count - 1] . context = p_context;
}

////////////////////////////////////////////////////////////////////////////////

typedef void (*MCPlatformDeathGripFreeCallback)(void *);

@interface com_runrev_livecode_MCPlatformDeathGrip: NSObject
{
	void *m_pointer;
	MCPlatformDeathGripFreeCallback m_free;
}

- (id)initWithPointer: (void *)pointer freeWith: (MCPlatformDeathGripFreeCallback)free;
- (void)dealloc;

@end

@implementation com_runrev_livecode_MCPlatformDeathGrip

- (id)initWithPointer: (void *)pointer freeWith: (MCPlatformDeathGripFreeCallback)free
{
	self = [super init];
	if (self == nil)
		return nil;
	
	m_pointer = pointer;
	m_free = free;

	return self;
}

- (void)dealloc
{
	m_free(m_pointer);
	[super dealloc];
}

@end

// When an event is dispatched to high-level it is possible for the main object
// to which it refers to be deleted. This can cause problems in the cocoa event
// handling chain. A deathgrip lasts for the scope of the current autorelease
// pool - so means such objects won't get completely destroyed until after event
// handling has completed.
void MCPlatformWindowDeathGrip(MCPlatformWindowRef p_window)
{
	// Retain the window.
	MCPlatformRetainWindow(p_window);
	
	// Now push an autorelease object onto the stack that will release the object
	// after event dispatch.
	[[[com_runrev_livecode_MCPlatformDeathGrip alloc] initWithPointer: p_window freeWith: (MCPlatformDeathGripFreeCallback)MCPlatformReleaseWindow] autorelease];
}

////////////////////////////////////////////////////////////////////////////////

// These tables are taken from the Carbon implementation - as keysDown takes into
// account shift states. I'm not sure this is entirely correct, but we must keep
// backwards compat.

static uint4 keysyms[] = {
	0x61, 0x73, 0x64, 0x66, 0x68, 0x67, 0x7A, 0x78, 0x63, 0x76, 0, 0x62,
	0x71, 0x77, 0x65, 0x72, 0x79, 0x74, 0x31, 0x32, 0x33, 0x34, 0x36,
	0x35, 0x3D, 0x39, 0x37, 0x2D, 0x38, 0x30, 0x5D, 0x6F, 0x75, 0x5B,
	0x69, 0x70, 0xFF0D, 0x6C, 0x6A, 0x27, 0x6B, 0x3B, 0x5C, 0x2C, 0x2F,
	0x6E, 0x6D, 0x2E, 0xFF09, 0x20, 0x60, 0xFF08, 0xFF8D, 0xFF1B, 0, 0,
	0xFFE1, 0xFFE5, 0, 0xFFE3, 0, 0, 0, 0, 0, 0xFF9F, 0, 0xFFAA, 0,
	0xFFAB, 0, 0xFF7F, 0, 0, 0, 0xFFAF, 0xFF8D, 0, 0xFFAD, 0, 0, 0xFFD5,
	0xFF9E, 0xFF9C, 0xFF99, 0xFF9B, 0xFF96, 0xFF9D, 0xFF98, 0xFF95, 0,
	0xFF97, 0xFF9A, 0, 0, 0, 0xFFC2, 0xFFC3, 0xFFC4, 0xFFC0, 0xFFC5,
	0xFFC6, 0, 0xFFC8, 0, 0xFFCA, 0xFFCD, 0xFF14, 0, 0xFFC7, 0, 0xFFC9, 0,
	0xFF13, 0x1004FF6A, 0xFF50, 0xFF55, 0xFFFF, 0xFFC1, 0xFF57, 0xFFBF,
	0xFF56, 0xFFBE, 0xFF51, 0xFF53, 0xFF54, 0xFF52, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static uint4 shift_keysyms[] = {
	0x41, 0x53, 0x44, 0x46, 0x48, 0x47, 0x5A, 0x58, 0x43, 0x56, 0, 0x42,
	0x51, 0x57, 0x45, 0x52, 0x59, 0x54, 0x21, 0x40, 0x23, 0x24, 0x5E,
	0x25, 0x2B, 0x28, 0x26, 0x5F, 0x2A, 0x29, 0x7D, 0x4F, 0x55, 0x7B,
	0x49, 0x50, 0xFF0D, 0x4C, 0x4A, 0x22, 0x4B, 0x3A, 0x7C, 0x3C, 0x3F,
	0x4E, 0x4D, 0x3E, 0xFF09, 0x20, 0x7E, 0xFF08, 0xFF8D, 0xFF1B, 0, 0,
	0xFFE1, 0xFFE5, 0, 0xFFE3, 0, 0, 0, 0, 0, 0xFFAE, 0, 0xFFAA, 0,
	0xFFAB, 0, 0xFF7F, 0, 0, 0, 0xFFAF, 0xFF8D, 0, 0xFFAD, 0, 0, 0xFFD5,
	0xFFB0, 0xFFB1, 0xFFB2, 0xFFB3, 0xFFB4, 0xFFB5, 0xFFB6, 0xFFB7, 0,
	0xFFB8, 0xFFB9, 0, 0, 0, 0xFFC2, 0xFFC3, 0xFFC4, 0xFFC0, 0xFFC5,
	0xFFC6, 0, 0xFFC8, 0, 0xFF62, 0, 0xFF20, 0, 0xFFC7, 0, 0xFFC9, 0,
	0xFF6B, 0x1004FF6A, 0xFF50, 0xFF55, 0xFFFF, 0xFFC1, 0xFF57, 0xFFBF,
	0xFF56, 0xFFBE, 0xFF51, 0xFF53, 0xFF54, 0xFF52, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

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

MCPlatformModifiers MCPlatformGetModifiersState(void)
{
	return MCMacPlatformMapNSModifiersToModifiers([NSEvent modifierFlags]);
}

bool MCPlatformGetKeyState(MCPlatformKeyCode*& r_codes, uindex_t& r_code_count)
{
	MCPlatformKeyCode *t_codes;
	if (!MCMemoryNewArray(128, t_codes))
		return false;
	
	bool t_is_shift;
	t_is_shift = ([NSEvent modifierFlags] & (NSShiftKeyMask | NSAlphaShiftKeyMask)) != 0;
	
	uindex_t t_code_count;
	t_code_count = 0;
	for(uindex_t i = 0; i < 127; i++)
	{
		if (!CGEventSourceKeyState(kCGEventSourceStateCombinedSessionState, i))
			continue;
		
		MCPlatformKeyCode t_code;
		if (t_is_shift)
			t_code = shift_keysyms[i];
		else
			t_code = keysyms[i];
		
		if (t_code != 0)
			t_codes[t_code_count++] = t_code;
	}
	
	r_codes = t_codes;
	r_code_count = t_code_count;
	
	return true;
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

void MCPlatformFlushEvents(MCPlatformEventMask p_mask)
{
	NSUInteger t_ns_mask;
	t_ns_mask = 0;
	if ((p_mask & kMCPlatformEventMouseDown) != 0)
		t_ns_mask |= NSLeftMouseDownMask | NSRightMouseDownMask | NSOtherMouseDownMask;
	if ((p_mask & kMCPlatformEventMouseUp) != 0)
		t_ns_mask |= NSLeftMouseUpMask | NSRightMouseUpMask | NSOtherMouseUpMask;
	if ((p_mask & kMCPlatformEventKeyDown) != 0)
		t_ns_mask |= NSKeyDownMask;
	if ((p_mask & kMCPlatformEventKeyUp) != 0)
		t_ns_mask |= NSKeyUpMask;
	
	NSDate *t_distant_past = [NSDate distantPast];
	for(;;)
	{
		NSEvent *t_event;
		t_event = [NSApp nextEventMatchingMask: t_ns_mask
									untilDate: t_distant_past
									inMode: NSDefaultRunLoopMode
									dequeue: YES];
		if (t_event == nil)
			break;
	}
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

void MCPlatformGetScreenPixelScale(uindex_t p_index, MCGFloat& r_scale)
{
	NSScreen *t_screen;
	t_screen = [[NSScreen screens] objectAtIndex: p_index];
	if ([t_screen respondsToSelector: @selector(backingScaleFactor)])
		r_scale = objc_msgSend_fpret(t_screen, @selector(backingScaleFactor));
	else
		r_scale = 1.0f;
}

////////////////////////////////////////////////////////////////////////////////

static MCPlatformWindowRef s_backdrop_window = nil;
static MCMacPlatformWindow **s_visible_windows = nil;
static uindex_t s_visible_window_count = 0;

static void MCMacPlatformAddVisibleWindow(MCMacPlatformWindow *p_window)
{
	/* UNCHECKED */ MCMemoryResizeArray(s_visible_window_count + 1, s_visible_windows, s_visible_window_count);
	s_visible_windows[s_visible_window_count - 1] = p_window;
}

static void MCMacPlatformRemoveVisibleWindow(MCMacPlatformWindow *p_window)
{
	for(uindex_t i = 0; i < s_visible_window_count; i++)
		if (s_visible_windows[i] == p_window)
		{
			MCMemoryMove(s_visible_windows + i, s_visible_windows + i + 1, sizeof(MCMacPlatformWindow *) * (s_visible_window_count - i - 1));
			s_visible_window_count -= 1;
			break;
		}
}

static void MCMacPlatformSyncBackdropForStyle(MCPlatformWindowStyle p_style)
{
	bool t_need_backdrop;
	if (s_backdrop_window != nil)
		t_need_backdrop = MCPlatformIsWindowVisible(s_backdrop_window);
	else
		t_need_backdrop = false;
	
	for(uindex_t i = 0; i < s_visible_window_count; i++)
	{
		MCPlatformWindowStyle t_style;
		s_visible_windows[i] -> GetProperty(kMCPlatformWindowPropertyStyle, kMCPlatformPropertyTypeWindowStyle, &t_style);
		if (t_style == p_style)
			s_visible_windows[i] -> SetBackdropWindow(t_need_backdrop ? s_backdrop_window : nil);
	}
}

static void MCMacPlatformSyncBackdrop(void)
{
	NSDisableScreenUpdates();
	MCMacPlatformSyncBackdropForStyle(kMCPlatformWindowStyleDocument);
	MCMacPlatformSyncBackdropForStyle(kMCPlatformWindowStylePalette);
	MCMacPlatformSyncBackdropForStyle(kMCPlatformWindowStyleDialog);
	NSEnableScreenUpdates();
	/*bool t_need_backdrop;
	if (s_backdrop_window != nil)
		t_need_backdrop = MCPlatformIsWindowVisible(s_backdrop_window);
	else
		t_need_backdrop = false;
	
	for(uindex_t i = 0; i < s_visible_window_count; i++)
		s_visible_windows[i] -> SetBackdropWindow(nil);
	if (s_backdrop_window != nil)
		for(index_t i = 0; i < s_visible_window_count; i++)
			s_visible_windows[i] -> SetBackdropWindow(s_backdrop_window);*/
}

void MCPlatformConfigureBackdrop(MCPlatformWindowRef p_backdrop_window)
{
#if TO_FIX
	if (s_backdrop_window != nil)
	{
		MCPlatformReleaseWindow(s_backdrop_window);
		s_backdrop_window = nil;
	}
	
	s_backdrop_window = p_backdrop_window;
	
	if (s_backdrop_window != nil)
		MCPlatformRetainWindow(s_backdrop_window);
	
	MCMacPlatformSyncBackdrop();
#endif
}

void MCMacPlatformWindowFocusing(MCMacPlatformWindow *p_window)
{
#if TO_FIX
	MCMacPlatformRemoveVisibleWindow(p_window);
	MCMacPlatformAddVisibleWindow(p_window);
	MCMacPlatformSyncBackdrop();
#endif
}

void MCMacPlatformWindowShowing(MCMacPlatformWindow *p_window)
{
#if TO_FIX
	MCMacPlatformAddVisibleWindow(p_window);
	MCMacPlatformSyncBackdrop();
#endif
}

void MCMacPlatformWindowHiding(MCMacPlatformWindow *p_window)
{	
#if TO_FIX
	MCMacPlatformRemoveVisibleWindow(p_window);
	MCMacPlatformSyncBackdrop();
#endif
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

// If this is true there was an explicit request for grabbing.
static bool s_mouse_grabbed_explicit = false;

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

// This is the current modifier state, and whether the control key was down
// for a button 0 press.
static MCPlatformModifiers s_mouse_modifiers = 0;
static bool s_mouse_was_control_click = false;

// COCOA-TODO: Clean up this external dependency.
extern uint2 MCdoubledelta;
extern uint2 MCdoubletime;
extern uint2 MCdragdelta;

void MCPlatformGrabPointer(MCPlatformWindowRef p_window)
{
	// If we are grabbing for the given window already, do nothing.
	if (s_mouse_grabbed && p_window == s_mouse_window)
	{
		s_mouse_grabbed_explicit = true;
		return;
	}
	
	// If the mouse window is already w, then just grab.
	if (p_window == s_mouse_window)
	{
		s_mouse_grabbed = true;
		s_mouse_grabbed_explicit = true;
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
	s_mouse_grabbed_explicit = false;
}

void MCMacPlatformHandleMousePress(uint32_t p_button, bool p_new_state)
{
	bool t_state;
	t_state = (s_mouse_buttons & (1 << p_button)) != 0;
	
	// If the state is not different from the new state, do nothing.
	if (p_new_state == t_state)
		return;
	
	// If we are mouse downing with no window, then do nothing.
	if (p_new_state && s_mouse_window == nil)
		return;
	
	// Update the state.
	if (p_new_state)
		s_mouse_buttons |= (1 << p_button);
	else
		s_mouse_buttons &= ~(1 << p_button);
	
	// Record whether it was an explicit grab.
	bool t_grabbed_explicit;
	t_grabbed_explicit = s_mouse_grabbed_explicit;
	
	// If we are grabbed, and mouse buttons are zero, then ungrab.
	// If mouse buttons are zero, then reset the drag button.
	if (s_mouse_buttons == 0)
	{
		s_mouse_grabbed = false;
		s_mouse_grabbed_explicit = false;
		s_mouse_drag_button = 0xffffffff;
	}
		
	// If mouse buttons are non-zero, then grab.
	if (s_mouse_buttons != 0)
		s_mouse_grabbed = true;
	
	// If the control key is down (which we will see as the command key) and if
	// the button is 0, then we actually want to dispatch a button 2.
	if (p_button == 0 &&
		(s_mouse_modifiers & kMCPlatformModifierCommand) != 0 &&
		p_new_state)
	{
		p_button = 2;
		s_mouse_was_control_click = true;
	}
	
	if (!p_new_state &&
		s_mouse_was_control_click && p_button == 0)
		p_button = 2;
		
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
		
		s_mouse_was_control_click = false;
		
		// If the mouse was grabbed explicitly, we send mouseUp not mouseRelease.
		if (t_new_mouse_window == s_mouse_window || t_grabbed_explicit)
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

void MCMacPlatformHandleMouseCursorChange(MCPlatformWindowRef p_window)
{
    // If the mouse is not currently over the window whose cursor has
    // changed - do nothing.
    if (s_mouse_window != p_window)
        return;

    // Show the cursor attached to the window.
    MCPlatformCursorRef t_cursor;
    MCPlatformGetWindowProperty(p_window, kMCPlatformWindowPropertyCursor, kMCPlatformPropertyTypeCursorRef, &t_cursor);
    
    // PM-2014-04-02: [[ Bug 12082 ]] IDE no longer crashes when changing an applied pattern
    if (t_cursor != nil)
        MCPlatformShowCursor(t_cursor);
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
		
		// If there is no mouse window, reset the cursor to default.
		if (t_new_mouse_window == nil)
			MCMacPlatformResetCursor();
			
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
        
        // Update the mouse cursor for the mouse window.
        MCMacPlatformHandleMouseCursorChange(s_mouse_window);
	}
}

void MCMacPlatformHandleMouseScroll(CGFloat dx, CGFloat dy)
{
	if (s_mouse_window == nil)
		return;
	
	if (dx != 0.0 || dy != 0.0)
		MCPlatformCallbackSendMouseScroll(s_mouse_window, dx < 0.0 ? -1 : (dx > 0.0 ? 1 : 0), dy < 0.0 ? -1 : (dy > 0.0 ? 1 : 0));
}

void MCMacPlatformHandleMouseSync(void)
{
	if (s_mouse_window != nil)
	{
		for(uindex_t i = 0; i < 3; i++)
			if ((s_mouse_buttons & (1 << i)) != 0)
			{
				s_mouse_buttons &= ~(1 << i);
				
				if (s_mouse_was_control_click &&
					i == 0)
					MCPlatformCallbackSendMouseRelease(s_mouse_window, 2);
				else
					MCPlatformCallbackSendMouseRelease(s_mouse_window, i);
			}
	}
	
	s_mouse_grabbed = false;
	s_mouse_drag_button = 0xffffffff;
	s_mouse_click_count = 0;
	s_mouse_last_click_time = 0;
	s_mouse_was_control_click = false;

	MCPoint t_location;
	MCMacPlatformMapScreenNSPointToMCPoint([NSEvent mouseLocation], t_location);
	
	MCMacPlatformHandleMouseMove(t_location);
}

void MCMacPlatformSyncMouseBeforeDragging(void)
{
	// Release the mouse.
	uindex_t t_button_to_release;
	if (s_mouse_buttons != 0)
	{
		t_button_to_release = s_mouse_drag_button;
		if (t_button_to_release == 0xffffffffU)
			t_button_to_release = s_mouse_last_click_button;
		
		s_mouse_buttons = 0;
		s_mouse_grabbed = false;
		s_mouse_click_count = 0;
		s_mouse_last_click_time = 0;
		s_mouse_drag_button = 0xffffffff;
		s_mouse_was_control_click = false;
	}
	else
		t_button_to_release = 0xffffffff;
	
	if (s_mouse_window != nil)
	{
		if (t_button_to_release != 0xffffffff)
			MCPlatformCallbackSendMouseRelease(s_mouse_window, t_button_to_release);
		MCPlatformCallbackSendMouseLeave(s_mouse_window);
		
		MCPlatformReleaseWindow(s_mouse_window);
		s_mouse_window = nil;
	}
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

void MCMacPlatformHandleModifiersChanged(MCPlatformModifiers p_modifiers)
{
	if (s_mouse_modifiers != p_modifiers)
	{
		s_mouse_modifiers = p_modifiers;
		MCPlatformCallbackSendModifiersChanged(p_modifiers);
	}
}
	
////////////////////////////////////////////////////////////////////////////////

MCPlatformModifiers MCMacPlatformMapNSModifiersToModifiers(NSUInteger p_modifiers)
{
	MCPlatformModifiers t_modifiers;
	t_modifiers = 0;
	
	if ((p_modifiers & NSShiftKeyMask) != 0)
		t_modifiers |= kMCPlatformModifierShift;
	if ((p_modifiers & NSAlternateKeyMask) != 0)
		t_modifiers |= kMCPlatformModifierOption;
	
	// COCOA-TODO: Abstract Command/Control switching.
	if ((p_modifiers & NSControlKeyMask) != 0)
		t_modifiers |= kMCPlatformModifierCommand;
	if ((p_modifiers & NSCommandKeyMask) != 0)
		t_modifiers |= kMCPlatformModifierControl;
	
	if ((p_modifiers & NSAlphaShiftKeyMask) != 0)
		t_modifiers |= kMCPlatformModifierCapsLock;

	return t_modifiers;
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
	extern bool MCS_mac_elevation_bootstrap_main(int argc, char* argv[]);
	if (argc == 2 && strcmp(argv[1], "-elevated-slave") == 0)
		if (!MCS_mac_elevation_bootstrap_main(argc, argv))
			return -1;
	
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
