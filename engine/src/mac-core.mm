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

#include "typedefs.h"
#include "platform.h"
#include "platform-internal.h"

#include "mac-internal.h"

#include "graphics_util.h"

#include "libscript/script.h"

#include <objc/objc-runtime.h>

////////////////////////////////////////////////////////////////////////////////

static bool s_have_desktop_height = false;
static CGFloat s_desktop_height = 0.0f;

static NSLock *s_callback_lock = nil;

// MW-2014-08-14: [[ Bug 13016 ]] This holds the window that is currently being
//   moved by the windowserver.
static MCPlatformWindowRef s_moving_window = nil;

static NSWindow *s_pseudo_modal_for = nil;

////////////////////////////////////////////////////////////////////////////////

enum
{
	kMCMacPlatformBreakEvent = 0,
	kMCMacPlatformMouseSyncEvent = 1,
};

////////////////////////////////////////////////////////////////////////////////

@implementation com_runrev_livecode_MCApplication

- (void)sendEvent:(NSEvent *)p_event
{
	if (!MCMacPlatformApplicationSendEvent(p_event))
	{
        [super sendEvent: p_event];
    }
}

@end

bool MCMacPlatformApplicationSendEvent(NSEvent *p_event)
{
    if ([p_event type] == NSApplicationDefined &&
        [p_event subtype] == kMCMacPlatformMouseSyncEvent)
	{
        MCMacPlatformHandleMouseSync();
		return true;
	}

	// MW-2014-08-14: [[ Bug 13016 ]] Whilst the windowserver moves a window
	//   we intercept mouseDragged events so we can keep script informed.
	NSWindow *t_window;
	t_window = [p_event window];
	if (s_moving_window != nil &&
		t_window == ((MCMacPlatformWindow *)s_moving_window) -> GetHandle())
	{
		if ([p_event type] == NSLeftMouseDragged)
			MCMacPlatformWindowWindowMoved(t_window, s_moving_window);
		else if ([p_event type] == NSLeftMouseUp)
			MCMacPlatformApplicationWindowStoppedMoving(s_moving_window);
	}
	
	return false;
}

bool MCMacPlatformApplicationWindowIsMoving(MCPlatformWindowRef p_window)
{
    return p_window == s_moving_window;
}

void MCMacPlatformApplicationWindowStartedMoving(MCPlatformWindowRef p_window)
{
    if (s_moving_window != nil)
        MCMacPlatformApplicationWindowStoppedMoving(s_moving_window);
    
    MCPlatformRetainWindow(p_window);
    s_moving_window = p_window;
}

void MCMacPlatformApplicationWindowStoppedMoving(MCPlatformWindowRef p_window)
{
    if (s_moving_window == nil)
        return;
    
	// IM-2014-10-29: [[ Bug 13814 ]] Call windowMoveFinished to signal end of dragging,
	//   which is not reported to the delegate when the window doesn't actually move.
	[[((MCMacPlatformWindow*)s_moving_window)->GetHandle() delegate] windowMoveFinished];

    MCPlatformReleaseWindow(s_moving_window);
    s_moving_window = nil;
}

void MCMacPlatformApplicationBecomePseudoModalFor(NSWindow *p_window)
{
    // MERG-2016-03-04: ensure pseudo modals open above any calling modals
    [p_window setLevel: kCGPopUpMenuWindowLevel];
    s_pseudo_modal_for = p_window;
}

NSWindow *MCMacPlatformApplicationPseudoModalFor(void)
{
    // MERG-2016-03-04: ensure pseudo modals remain above any calling modals
    // If we need to check whether we're pseudo-modal, it means we're in a
    // situation where that window needs to be forced to the front
    if (s_pseudo_modal_for != nil)
        [s_pseudo_modal_for orderFrontRegardless];
    
    return s_pseudo_modal_for;
}

////////////////////////////////////////////////////////////////////////////////

@implementation com_runrev_livecode_MCPendingAppleEvent

- (id)initWithEvent: (const AppleEvent *)event andReply: (AppleEvent *)reply
{
    self = [super init];
    if (self == nil)
        return nil;
    
    m_event = *event;
    m_reply = *reply;
    
    return self;
}

- (void)dealloc
{
    AEDisposeDesc(&m_event);
    AEDisposeDesc(&m_reply);
    [super dealloc];
}

- (AppleEvent *)getEvent
{
    return &m_event;
}

- (AppleEvent *)getReply
{
    return &m_reply;
}

@end

////////////////////////////////////////////////////////////////////////////////

@implementation com_runrev_livecode_MCApplicationDelegate

//////////

- (id)initWithArgc:(int)argc argv:(MCStringRef *)argv envp:(MCStringRef*)envp
{
	self = [super init];
	if (self == nil)
		return nil;
	
	m_argc = argc;
	m_argv = argv;
	m_envp = envp;
	
    m_explicit_quit = false;
    
    m_running = false;
    
    m_pending_apple_events = [[NSMutableArray alloc] initWithCapacity: 0];
    
	return self;
}

//////////

- (void)initializeModules
{
	MCPlatformInitializeColorTransform();
	MCPlatformInitializeAbortKey();
    MCPlatformInitializeMenu();
}

- (void)finalizeModules
{
    MCPlatformFinalizeMenu();
	MCPlatformFinalizeAbortKey();
	MCPlatformFinalizeColorTransform();
}

//////////

- (NSError *)application:(NSApplication *)application willPresentError:(NSError *)error
{
    return error;
}

//////////

- (BOOL)applicationShouldHandleReopen:(NSApplication *)sender hasVisibleWindows:(BOOL)flag
{
	return NO;
}

//////////

static OSErr preDispatchAppleEvent(const AppleEvent *p_event, AppleEvent *p_reply, SRefCon p_context)
{
    return [(MCApplicationDelegate*)[NSApp delegate] preDispatchAppleEvent: p_event withReply: p_reply];
}

- (OSErr)preDispatchAppleEvent: (const AppleEvent *)p_event withReply: (AppleEvent *)p_reply
{
    extern OSErr MCAppleEventHandlerDoAEAnswer(const AppleEvent *event, AppleEvent *reply, long refcon);
    extern OSErr MCAppleEventHandlerDoSpecial(const AppleEvent *event, AppleEvent *reply, long refcon);
    extern OSErr MCAppleEventHandlerDoOpenDoc(const AppleEvent *event, AppleEvent *reply, long refcon);
    
    if (!m_running)
    {
        MCPendingAppleEvent *t_event;
        t_event = [[MCPendingAppleEvent alloc] initWithEvent: p_event andReply: p_reply];
        [m_pending_apple_events addObject: t_event];
        AESuspendTheCurrentEvent(p_event);
        return noErr;
    }
    
	DescType rType;
	Size rSize;
	AEEventClass aeclass;
	AEGetAttributePtr(p_event, keyEventClassAttr, typeType, &rType, &aeclass, sizeof(AEEventClass), &rSize);
    
	AEEventID aeid;
	AEGetAttributePtr(p_event, keyEventIDAttr, typeType, &rType, &aeid, sizeof(AEEventID), &rSize);
    
    // MW-2014-08-12: [[ Bug 13140 ]] Handle the appleEvent to cause a termination otherwise
    //   we don't quit if the app is in the background (I think this is because we roll our
    //   own event handling loop and don't use [NSApp run]).
    if (aeclass == kCoreEventClass && aeid == kAEQuitApplication)
    {
        [NSApp terminate: self];
        return noErr;
    }
    
    if (aeclass == kCoreEventClass && aeid == kAEAnswer)
        return MCAppleEventHandlerDoAEAnswer(p_event, p_reply, 0);
    
    // SN-2014-10-13: [[ Bug 13644 ]] Break the wait loop after we handled the Apple Event
    OSErr t_err;
    t_err = MCAppleEventHandlerDoSpecial(p_event, p_reply, 0);
    if (t_err == errAEEventNotHandled)
    {
        if (aeclass == kCoreEventClass && aeid == kAEOpenDocuments)
            t_err = MCAppleEventHandlerDoOpenDoc(p_event, p_reply, 0);
    }
    
    if (t_err != errAEEventNotHandled)
        MCPlatformBreakWait();
    
    return t_err;
}

- (void)applicationWillFinishLaunching: (NSNotification *)notification
{
    AEInstallSpecialHandler(keyPreDispatch, preDispatchAppleEvent, False);
}

- (void)applicationDidFinishLaunching: (NSNotification *)notification
{	
	// Initialize everything.
	[self initializeModules];
	
	NSAutoreleasePool *t_pool;
	t_pool = [[NSAutoreleasePool alloc] init];
	
    // MW-2014-04-23: [[ Bug 12080 ]] Always create a dummy window which should
    //   always sit at the bottom of our window list so that palettes have something
    //   to float above.
    NSWindow *t_dummy_window;
    t_dummy_window = [[NSWindow alloc] initWithContentRect: NSZeroRect styleMask: NSBorderlessWindowMask backing:NSBackingStoreBuffered defer:YES];
    [t_dummy_window orderFront: nil];
    
	// Dispatch the startup callback.
	int t_error_code;
	MCAutoStringRef t_error_message;
	MCPlatformCallbackSendApplicationStartup(m_argc, m_argv, m_envp, t_error_code, &t_error_message);
	
	[t_pool release];
	
	// If the error code is non-zero, startup failed so quit.
	if (t_error_code != 0)
	{
		// If the error message is non-nil, report it in a suitable way.
		if (*t_error_message != nil)
        {
            MCAutoStringRefAsUTF8String t_utf8_message;
            t_utf8_message . Lock(*t_error_message);
			fprintf(stderr, "Startup error - %s\n", *t_utf8_message);
        }
		
		// Finalize everything
		[self finalizeModules];
		
		// Now exit the application with the appropriate code.
		exit(t_error_code);
	}
    
    m_running = true;

    // Dispatch pending apple events
    while([m_pending_apple_events count] > 0)
    {
        MCPendingAppleEvent *t_event;
        t_event = [m_pending_apple_events objectAtIndex: 0];
        [m_pending_apple_events removeObjectAtIndex: 0];
        
        [self preDispatchAppleEvent: [t_event getEvent] withReply: [t_event getReply]];
        AEResumeTheCurrentEvent([t_event getEvent], [t_event getReply], (AEEventHandlerUPP)kAENoDispatch, 0);
        
        //[t_event process];
        
        [t_event release];
    }
	
	[[NSDistributedNotificationCenter defaultCenter] addObserver:self
									 selector:@selector(interfaceThemeChangedNotification:)
								     name:@"AppleInterfaceThemeChangedNotification" object:nil];
    
	// We started up successfully, so queue the root runloop invocation
	// message.
	[self performSelector: @selector(runMainLoop) withObject: nil afterDelay: 0];
}

- (void)interfaceThemeChangedNotification:(NSNotification *)notification
{
	MCPlatformCallbackSendSystemAppearanceChanged();
}

- (void)runMainLoop
{
    for(;;)
    {
        bool t_continue;
    
        NSAutoreleasePool *t_pool;
        t_pool = [[NSAutoreleasePool alloc] init];
    
        MCPlatformCallbackSendApplicationRun(t_continue);
        
        [t_pool release];
        
        if (!t_continue)
            break;
    }
    
    // If we get here then it was due to an exit from the main runloop caused
    // by an explicit quit. In this case, then we set a flag so that termination
    // happens without sending messages.
    m_explicit_quit = true;
	
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
    // If the quit was explicit (runloop exited) then just terminate now.
    if (m_explicit_quit)
        return NSTerminateNow;
    
    if (MCMacPlatformApplicationPseudoModalFor() != nil)
        return NSTerminateCancel;
    
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
    // MW-2014-04-23: [[ Bug 12080 ]] Loop through all our windows and any MCPanels
    //   get set to not be floating. This is so that they sit behind the windows
    //   of other applications (like we did before).
    for(NSNumber *t_window_id in [[NSWindow windowNumbersWithOptions: 0] reverseObjectEnumerator])
    {
        NSWindow *t_window;
        t_window = [NSApp windowWithWindowNumber: [t_window_id longValue]];
        if (![t_window isKindOfClass: [com_runrev_livecode_MCPanel class]])
        {
            continue;
        }
        
        [t_window setFloatingPanel: YES];
    }
}

- (void)applicationDidBecomeActive:(NSNotification *)notification
{
	MCPlatformCallbackSendApplicationResume();
}

- (void)applicationWillResignActive:(NSNotification *)notification
{
    // MW-2014-04-23: [[ Bug 12080 ]] Loop through all our windows and move any
    //   MCPanels to be above the top-most non-panel.
    NSInteger t_above_window_id;
    for(NSNumber *t_window_id in [[NSWindow windowNumbersWithOptions: 0] reverseObjectEnumerator])
    {
        NSWindow *t_window;
        t_window = [NSApp windowWithWindowNumber: [t_window_id longValue]];
        if (![t_window isKindOfClass: [com_runrev_livecode_MCPanel class]])
        {
            t_above_window_id = [t_window_id longValue];
            continue;
        }
        
        [t_window setFloatingPanel: NO];
        [t_window orderWindow: NSWindowAbove relativeTo: t_above_window_id];
        t_above_window_id = [t_window_id longValue];
    }
}

- (void)applicationDidResignActive:(NSNotification *)notification
{
	MCPlatformCallbackSendApplicationSuspend();
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
	s_have_desktop_height = false;
	
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
            // Get the double-click interval, in milliseconds
            *(uint16_t *)r_value = uint16_t([NSEvent doubleClickInterval] * 1000.0);
			break;
			
		case kMCPlatformSystemPropertyCaretBlinkInterval:
        {
            // Query the user's settings for the cursor blink rate
            NSInteger t_rate_ms = [[NSUserDefaults standardUserDefaults] integerForKey:@"NSTextInsertionPointBlinkPeriod"];
            
            // If the query failed, use the standard value (this seems to be
            // 567ms on OSX, not that this is documented anywhere).
            if (t_rate_ms == 0)
                t_rate_ms = 567;
            
            *(uint16_t *)r_value = uint16_t(t_rate_ms);
			break;
        }
			
		case kMCPlatformSystemPropertyHiliteColor:
		{
            NSColor *t_color;
            t_color = [[NSColor selectedTextBackgroundColor] colorUsingColorSpaceName: NSCalibratedRGBColorSpace];
			((MCColor *)r_value) -> red = [t_color redComponent] * 65535;
			((MCColor *)r_value) -> green = [t_color greenComponent] * 65535;
			((MCColor *)r_value) -> blue = [t_color blueComponent] * 65535;
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
			
        case kMCPlatformSystemPropertyVolume:
            MCMacPlatformGetGlobalVolume(*(double *)r_value);
            break;
		
		case kMCPlatformSystemPropertySystemAppearance:
		{
			NSUserDefaults *t_defaults = [NSUserDefaults standardUserDefaults];
			NSString *t_appearance = [t_defaults stringForKey:@"AppleInterfaceStyle"];
			if (t_appearance == nil || ![t_appearance isEqualToString:@"Dark"])
			{
				*(int16_t *)r_value = kMCPlatformSystemAppearanceLight;
			}
			else
			{
				*(int16_t *)r_value = kMCPlatformSystemAppearanceDark;
			}
		}
			break;
		default:
			assert(false);
			break;
	}
}

void MCPlatformSetSystemProperty(MCPlatformSystemProperty p_property, MCPlatformPropertyType p_type, void *p_value)
{
    switch(p_property)
    {
        case kMCPlatformSystemPropertyVolume:
            MCMacPlatformSetGlobalVolume(*(double *)p_value);
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

static MCPlatformPreWaitForEventCallback s_pre_waitforevent_callback = nullptr;
static MCPlatformPostWaitForEventCallback s_post_waitforevent_callback = nullptr;

void MCPlatformSetWaitForEventCallbacks(MCPlatformPreWaitForEventCallback p_pre, MCPlatformPostWaitForEventCallback p_post)
{
    s_pre_waitforevent_callback = p_pre;
    s_post_waitforevent_callback = p_post;
}

void MCPlatformBreakWait(void)
{
    [s_callback_lock lock];
	if (s_wait_broken)
    {
        [s_callback_lock unlock];
        return;
    }
    s_wait_broken = true;
	[s_callback_lock unlock];
    
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

void MCMacPlatformWaitEventObserverCallback(CFRunLoopObserverRef observer, CFRunLoopActivity activity, void *info)
{
 	if (s_in_blocking_wait)
		MCPlatformBreakWait();
}

static uindex_t s_event_checking_disabled = 0;

void MCMacPlatformEnableEventChecking(void)
{
	s_event_checking_disabled -= 1;
}

void MCMacPlatformDisableEventChecking(void)
{
	s_event_checking_disabled += 1;
}

bool MCMacPlatformIsEventCheckingEnabled(void)
{
	return s_event_checking_disabled == 0;
}

bool MCPlatformWaitForEvent(double p_duration, bool p_blocking)
{
	if (!MCMacPlatformIsEventCheckingEnabled())
		return false;
	
    if (s_pre_waitforevent_callback != nullptr)
    {
        if (!s_pre_waitforevent_callback(p_duration, p_blocking))
        {
            return false;
        }
    }
    else
    {
        // Make sure we have our observer and install it. This is used when we are
        // blocking and should break the event loop whenever a new event is added
        // to the queue.
        if (s_observer == nil)
        {
            s_observer = CFRunLoopObserverCreate(kCFAllocatorDefault, kCFRunLoopAfterWaiting, true, 0, MCMacPlatformWaitEventObserverCallback, NULL);
            CFRunLoopAddObserver([[NSRunLoop currentRunLoop] getCFRunLoop], s_observer, (CFStringRef)NSEventTrackingRunLoopMode);
        }
    }
    
	// Handle all the pending callbacks.
    MCCallback *t_callbacks;
    uindex_t t_callback_count;
    [s_callback_lock lock];
	s_wait_broken = false;
    t_callbacks = s_callbacks;
    t_callback_count = s_callback_count;
    s_callbacks = nil;
    s_callback_count = 0;
    [s_callback_lock unlock];
    
	for(uindex_t i = 0; i < t_callback_count; i++)
		t_callbacks[i] . method(t_callbacks[i] . context);
	MCMemoryDeleteArray(t_callbacks);
	t_callbacks = nil;
	t_callback_count = 0;
	
	s_in_blocking_wait = true;
	
	bool t_modal;
	t_modal = s_modal_session_count > 0;
	
	NSAutoreleasePool *t_pool;
	t_pool = [[NSAutoreleasePool alloc] init];
	
    // MW-2014-07-24: [[ Bug 12939 ]] If we are running a modal session, then don't then wait
    //   for events - event handling happens inside the modal session.
    NSEvent *t_event;
    
    // MW-2014-04-09: [[ Bug 10767 ]] Don't run in the modal panel runloop mode as this stops
    //   WebViews from working.    
    // SN-2014-10-02: [[ Bug 13555 ]] We want the event to be sent in case it passes through
    //   the modal session.
    t_event = [NSApp nextEventMatchingMask: p_blocking ? NSApplicationDefinedMask : NSAnyEventMask
                                 untilDate: [NSDate dateWithTimeIntervalSinceNow: p_duration]
                                    inMode: p_blocking ? NSEventTrackingRunLoopMode : NSDefaultRunLoopMode
                                   dequeue: YES];
    
    // Run the modal session, if it has been created yet (it might not if this
    // wait was triggered by reacting to an event caused as part of creating
    // the modal session, e.g. when losing window focus).
	if (t_modal && s_modal_sessions[s_modal_session_count - 1].session != nil)
		[NSApp runModalSession: s_modal_sessions[s_modal_session_count - 1] . session];
    
	s_in_blocking_wait = false;

	if (t_event != nil)
	{
		if ([t_event type] == NSLeftMouseDown || [t_event type] == NSLeftMouseDragged)
		{
            if (s_last_mouse_event != nullptr)
                [s_last_mouse_event release];
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
	
    if (s_post_waitforevent_callback != nullptr)
    {
        return s_post_waitforevent_callback(t_event != nullptr);
    }
    
	return t_event != nil;
}

void MCMacPlatformClearLastMouseEvent(void)
{
    if (s_last_mouse_event == nil)
    {
        return;
    }
    
    [s_last_mouse_event release];
    s_last_mouse_event = nil;
}

void MCMacPlatformBeginModalSession(MCMacPlatformWindow *p_window)
{
    // MW-2014-07-24: [[ Bug 12898 ]] The context of the click is changing, so make sure we sync
    //   mouse state - i.e. send a mouse release if in mouseDown and send a mouseLeave for the
    //   current mouse window.
	MCMacPlatformSyncMouseBeforeDragging();
    
	/* UNCHECKED */ MCMemoryResizeArray(s_modal_session_count + 1, s_modal_sessions, s_modal_session_count);
	
	s_modal_sessions[s_modal_session_count - 1] . is_done = false;
	s_modal_sessions[s_modal_session_count - 1] . window = p_window;
	p_window -> Retain();
	// IM-2015-01-30: [[ Bug 14140 ]] lock the window frame to prevent it from being centered on the screen.
	p_window->SetFrameLocked(true);
	s_modal_sessions[s_modal_session_count - 1] . session = [NSApp beginModalSessionForWindow: (NSWindow *)(p_window -> GetHandle())];
	p_window->SetFrameLocked(false);
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
		[s_modal_sessions[t_final_index - 1] . window -> GetHandle() performSelector:@selector(orderOut:) withObject:nil afterDelay:0];
		s_modal_sessions[t_final_index - 1] . window -> Release();
		s_modal_session_count -= 1;
	}
}

void MCMacPlatformScheduleCallback(void (*p_callback)(void *), void *p_context)
{
    [s_callback_lock lock];
	/* UNCHECKED */ MCMemoryResizeArray(s_callback_count + 1, s_callbacks, s_callback_count);
	s_callbacks[s_callback_count - 1] . method = p_callback;
	s_callbacks[s_callback_count - 1] . context = p_context;
    [s_callback_lock unlock];
    
    MCPlatformBreakWait();
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
	
	bool t_flags;
	t_flags = [NSEvent modifierFlags];
    
	uindex_t t_code_count;
	t_code_count = 0;
	for(uindex_t i = 0; i < 127; i++)
	{
		if (!CGEventSourceKeyState(kCGEventSourceStateCombinedSessionState, i))
			continue;
		
		MCPlatformKeyCode t_code;
        if (MCMacPlatformMapKeyCode(i, t_flags, t_code))
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
	NSRect t_content_rect;
    if (t_window != nil && [t_window conformsToProtocol:NSProtocolFromString(@"com_runrev_livecode_MCMovingFrame")])
        t_content_rect = [(NSWindow <com_runrev_livecode_MCMovingFrame>*)t_window movingFrame];
    else
        t_content_rect = [t_window frame];
    
    // MW-2014-05-28: [[ Bug 12437 ]] Seems the window at point uses inclusive co-ords
    //   in the in-rect calculation - so adjust the rect appropriately.
    t_content_rect = [t_window contentRectForFrameRect: t_content_rect];
    
    bool t_is_in_frame;
    t_content_rect . size . width += 1, t_content_rect . size . height += 1;
    t_is_in_frame = NSPointInRect(t_loc_cocoa, t_content_rect);
    
	if (t_window != nil &&
		[[t_window delegate] isKindOfClass: [MCWindowDelegate class]] &&
		t_is_in_frame)
		r_window = [(MCWindowDelegate *)[t_window delegate] platformWindow];
	else
		r_window = nil;
}

// MW-2014-07-15: [[ Bug 12800 ]] Map a window number to a platform window - if there is one.
bool MCPlatformGetWindowWithId(uint32_t p_id, MCPlatformWindowRef& r_window)
{
    NSWindow *t_ns_window;
    t_ns_window = [NSApp windowWithWindowNumber: p_id];
    if (t_ns_window == nil)
        return false;
    
    id t_delegate;
    t_delegate = [t_ns_window delegate];
    if (t_delegate == nil)
        return false;
    
    if (![t_delegate isKindOfClass: [MCWindowDelegate class]])
        return false;
    
    r_window = [(MCWindowDelegate *)t_delegate platformWindow];
    
    return true;
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

void MCPlatformBeep(void)
{
    NSBeep();
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
		r_scale = static_cast<MCGFloat>([t_screen backingScaleFactor]);
	else
		r_scale = 1.0f;
}

////////////////////////////////////////////////////////////////////////////////

static MCPlatformWindowRef s_backdrop_window = nil;

void MCMacPlatformSyncBackdrop(void)
{
    if (s_backdrop_window == nil)
        return;
    
    NSWindow *t_backdrop;
    t_backdrop = ((MCMacPlatformWindow *)s_backdrop_window) -> GetHandle();
    
    NSDisableScreenUpdates();
    [t_backdrop orderOut: nil];
    
    // Loop from front to back on our windows, making sure the backdrop window is
    // at the back.
    NSInteger t_window_above_id;
    t_window_above_id = -1;
    for(NSNumber *t_window_id in [NSWindow windowNumbersWithOptions: 0])
    {
        NSWindow *t_window;
        t_window = [NSApp windowWithWindowNumber: [t_window_id longValue]];
        
        if (t_window == t_backdrop)
            continue;
        
        if (t_window_above_id != -1)
            [t_window orderWindow: NSWindowBelow relativeTo: t_window_above_id];
        
        t_window_above_id = [t_window_id longValue];
    }
    
    [t_backdrop orderWindow: NSWindowBelow relativeTo: t_window_above_id];
    
    NSEnableScreenUpdates();
}

void MCPlatformConfigureBackdrop(MCPlatformWindowRef p_backdrop_window)
{
	if (s_backdrop_window != nil)
	{
		MCPlatformReleaseWindow(s_backdrop_window);
		s_backdrop_window = nil;
	}
	
	s_backdrop_window = p_backdrop_window;
	
	if (s_backdrop_window != nil)
		MCPlatformRetainWindow(s_backdrop_window);
	
	MCMacPlatformSyncBackdrop();
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
	0xFFAB, 0, 0xFF7F, 0, 0, 0, 0xFFAF, 0xFF8D, 0, 0xFFAD, 0, 0, 0xFFBD,
	0xFF9E, 0xFF9C, 0xFF99, 0xFF9B, 0xFF96, 0xFF9D, 0xFF98, 0xFF95, 0,
	0xFF97, 0xFF9A, 0, 0, 0, 0xFFC2, 0xFFC3, 0xFFC4, 0xFFC0, 0xFFC5,
	0xFFC6, 0, 0xFFC8, 0, 0xFFCA, 0xFFCD, 0xFF14, 0, 0xFFC7, 0, 0xFFC9, 0,
	0xFF13, 0xFF6A, 0xFF50, 0xFF55, 0xFFFF, 0xFFC1, 0xFF57, 0xFFBF,
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
	0xFFAB, 0, 0xFF7F, 0, 0, 0, 0xFFAF, 0xFF8D, 0, 0xFFAD, 0, 0, 0xFFBD,
	0xFFB0, 0xFFB1, 0xFFB2, 0xFFB3, 0xFFB4, 0xFFB5, 0xFFB6, 0xFFB7, 0,
	0xFFB8, 0xFFB9, 0, 0, 0, 0xFFC2, 0xFFC3, 0xFFC4, 0xFFC0, 0xFFC5,
	0xFFC6, 0, 0xFFC8, 0, 0xFF62, 0, 0xFF20, 0, 0xFFC7, 0, 0xFFC9, 0,
	0xFF6B, 0xFF6A, 0xFF50, 0xFF55, 0xFFFF, 0xFFC1, 0xFF57, 0xFFBF,
	0xFF56, 0xFFBE, 0xFF51, 0xFF53, 0xFF54, 0xFF52, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static MCPlatformKeyCode s_mac_keycode_map[] =
{
	/* 0x00 */ kMCPlatformKeyCodeA,
	/* 0x01 */ kMCPlatformKeyCodeS,
	/* 0x02 */ kMCPlatformKeyCodeD,
	/* 0x03 */ kMCPlatformKeyCodeF,
	/* 0x04 */ kMCPlatformKeyCodeH,
	/* 0x05 */ kMCPlatformKeyCodeG,
	/* 0x06 */ kMCPlatformKeyCodeZ,
	/* 0x07 */ kMCPlatformKeyCodeX,
	/* 0x08 */ kMCPlatformKeyCodeC,
	/* 0x09 */ kMCPlatformKeyCodeV,
	/* 0x0A */ kMCPlatformKeyCodeISOSection,
	/* 0x0B */ kMCPlatformKeyCodeB,
	/* 0x0C */ kMCPlatformKeyCodeQ,
	/* 0x0D */ kMCPlatformKeyCodeW,
	/* 0x0E */ kMCPlatformKeyCodeE,
	/* 0x0F */ kMCPlatformKeyCodeR,
	/* 0x10 */ kMCPlatformKeyCodeY,
	/* 0x11 */ kMCPlatformKeyCodeT,
	/* 0x12 */ kMCPlatformKeyCode1,
	/* 0x13 */ kMCPlatformKeyCode2,
	/* 0x14 */ kMCPlatformKeyCode3,
	/* 0x15 */ kMCPlatformKeyCode4,
	/* 0x16 */ kMCPlatformKeyCode6,
	/* 0x17 */ kMCPlatformKeyCode5,
	/* 0x18 */ kMCPlatformKeyCodeEqual,
	/* 0x19 */ kMCPlatformKeyCode9,
	/* 0x1A */ kMCPlatformKeyCode7,
	/* 0x1B */ kMCPlatformKeyCodeMinus,
	/* 0x1C */ kMCPlatformKeyCode8,
	/* 0x1D */ kMCPlatformKeyCode0,
	/* 0x1E */ kMCPlatformKeyCodeRightBracket,
	/* 0x1F */ kMCPlatformKeyCodeO,
	/* 0x20 */ kMCPlatformKeyCodeU,
	/* 0x21 */ kMCPlatformKeyCodeLeftBracket,
	/* 0x22 */ kMCPlatformKeyCodeI,
	/* 0x23 */ kMCPlatformKeyCodeP,
	/* 0x24 */ kMCPlatformKeyCodeReturn,
	/* 0x25 */ kMCPlatformKeyCodeL,
	/* 0x26 */ kMCPlatformKeyCodeJ,
	/* 0x27 */ kMCPlatformKeyCodeQuote,
	/* 0x28 */ kMCPlatformKeyCodeK,
	/* 0x29 */ kMCPlatformKeyCodeSemicolon,
	/* 0x2A */ kMCPlatformKeyCodeBackslash,
	/* 0x2B */ kMCPlatformKeyCodeComma,
	/* 0x2C */ kMCPlatformKeyCodeSlash,
	/* 0x2D */ kMCPlatformKeyCodeN,
	/* 0x2E */ kMCPlatformKeyCodeM,
	/* 0x2F */ kMCPlatformKeyCodePeriod,
	/* 0x30 */ kMCPlatformKeyCodeTab,
	/* 0x31 */ kMCPlatformKeyCodeSpace,
	/* 0x32 */ kMCPlatformKeyCodeGrave,
	/* 0x33 */ kMCPlatformKeyCodeBackspace,
	/* 0x34 */ kMCPlatformKeyCodeUndefined,
	/* 0x35 */ kMCPlatformKeyCodeEscape,
	/* 0x36 */ kMCPlatformKeyCodeRightCommand,
	/* 0x37 */ kMCPlatformKeyCodeLeftCommand,
	/* 0x38 */ kMCPlatformKeyCodeLeftShift,
	/* 0x39 */ kMCPlatformKeyCodeCapsLock,
	/* 0x3A */ kMCPlatformKeyCodeLeftOption,
	/* 0x3B */ kMCPlatformKeyCodeLeftControl,
	/* 0x3C */ kMCPlatformKeyCodeRightShift,
	/* 0x3D */ kMCPlatformKeyCodeRightOption,
	/* 0x3E */ kMCPlatformKeyCodeRightControl,
	/* 0x3F */ kMCPlatformKeyCodeFunction,
	/* 0x40 */ kMCPlatformKeyCodeF17,
	/* 0x41 */ kMCPlatformKeyCodeKeypadDecimal,
	/* 0x42 */ kMCPlatformKeyCodeUndefined,
	/* 0x43 */ kMCPlatformKeyCodeKeypadMultiply,
	/* 0x44 */ kMCPlatformKeyCodeUndefined,
	/* 0x45 */ kMCPlatformKeyCodeKeypadAdd,
	/* 0x46 */ kMCPlatformKeyCodeUndefined,
	/* 0x47 */ kMCPlatformKeyCodeNumLock, // COCO-TODO: This should be keypad-clear - double-check!
	/* 0x48 */ kMCPlatformKeyCodeVolumeUp,
	/* 0x49 */ kMCPlatformKeyCodeVolumeDown,
	/* 0x4A */ kMCPlatformKeyCodeMute,
	/* 0x4B */ kMCPlatformKeyCodeKeypadDivide,
	/* 0x4C */ kMCPlatformKeyCodeKeypadEnter,
	/* 0x4D */ kMCPlatformKeyCodeUndefined,
	/* 0x4E */ kMCPlatformKeyCodeKeypadSubtract,
	/* 0x4F */ kMCPlatformKeyCodeF18,
	/* 0x50 */ kMCPlatformKeyCodeF19,
	/* 0x51 */ kMCPlatformKeyCodeKeypadEqual,
	/* 0x52 */ kMCPlatformKeyCodeKeypad0,
	/* 0x53 */ kMCPlatformKeyCodeKeypad1,
	/* 0x54 */ kMCPlatformKeyCodeKeypad2,
	/* 0x55 */ kMCPlatformKeyCodeKeypad3,
	/* 0x56 */ kMCPlatformKeyCodeKeypad4,
	/* 0x57 */ kMCPlatformKeyCodeKeypad5,
	/* 0x58 */ kMCPlatformKeyCodeKeypad6,
	/* 0x59 */ kMCPlatformKeyCodeKeypad7,
	/* 0x5A */ kMCPlatformKeyCodeF20,
	/* 0x5B */ kMCPlatformKeyCodeKeypad8,
	/* 0x5C */ kMCPlatformKeyCodeKeypad9,
	/* 0x5D */ kMCPlatformKeyCodeJISYen,
	/* 0x5E */ kMCPlatformKeyCodeJISUnderscore,
	/* 0x5F */ kMCPlatformKeyCodeJISKeypadComma,
	/* 0x60 */ kMCPlatformKeyCodeF5,
	/* 0x61 */ kMCPlatformKeyCodeF6,
	/* 0x62 */ kMCPlatformKeyCodeF7,
	/* 0x63 */ kMCPlatformKeyCodeF3,
	/* 0x64 */ kMCPlatformKeyCodeF8,
	/* 0x65 */ kMCPlatformKeyCodeF9,
	/* 0x66 */ kMCPlatformKeyCodeJISEisu,
	/* 0x67 */ kMCPlatformKeyCodeF11,
	/* 0x68 */ kMCPlatformKeyCodeJISKana,
	/* 0x69 */ kMCPlatformKeyCodeF13,
	/* 0x6A */ kMCPlatformKeyCodeF16,
	/* 0x6B */ kMCPlatformKeyCodeF14,
	/* 0x6C */ kMCPlatformKeyCodeUndefined,
	/* 0x6D */ kMCPlatformKeyCodeF10,
	/* 0x6E */ kMCPlatformKeyCodeUndefined,
	/* 0x6F */ kMCPlatformKeyCodeF12,
	/* 0x70 */ kMCPlatformKeyCodeUndefined,
	/* 0x71 */ kMCPlatformKeyCodeF15,
	/* 0x72 */ kMCPlatformKeyCodeHelp,
	/* 0x73 */ kMCPlatformKeyCodeBegin,
	/* 0x74 */ kMCPlatformKeyCodePrevious,
	/* 0x75 */ kMCPlatformKeyCodeDelete,
	/* 0x76 */ kMCPlatformKeyCodeF4,
	/* 0x77 */ kMCPlatformKeyCodeEnd,
	/* 0x78 */ kMCPlatformKeyCodeF2,
	/* 0x79 */ kMCPlatformKeyCodeNext,
	/* 0x7A */ kMCPlatformKeyCodeF1,
	/* 0x7B */ kMCPlatformKeyCodeLeft,
	/* 0x7C */ kMCPlatformKeyCodeRight,
	/* 0x7D */ kMCPlatformKeyCodeDown,
	/* 0x7E */ kMCPlatformKeyCodeUp,
	/* 0x7F */ kMCPlatformKeyCodeUndefined,
};

bool MCMacPlatformMapKeyCode(uint32_t p_mac_keycode, uint32_t p_modifier_flags, MCPlatformKeyCode& r_keycode)
{
	if (p_mac_keycode > 0x7f)
		return false;
    
	if (s_mac_keycode_map[p_mac_keycode] == kMCPlatformKeyCodeUndefined)
		return false;
    
    // PLATFORM-TODO: Shifted keysym handling should be in the engine rather than
    //   here.
    bool t_is_shift;
    t_is_shift = (p_modifier_flags & (NSShiftKeyMask | NSAlphaShiftKeyMask)) != 0;
    if (t_is_shift)
        r_keycode = shift_keysyms[p_mac_keycode];
    else
        r_keycode = keysyms[p_mac_keycode];
    
    /* The keysyms and shift_keysyms arrays don't have entries for everything. If
     * we get 0 here we fall back to the underlying list's entries. This means
     * that modifier keys are correctly handled */
    if (r_keycode == 0)
        r_keycode = s_mac_keycode_map[p_mac_keycode];
    
	return true;
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

// MW-2014-06-11: [[ Bug 12436 ]] This is used to temporarily turn off cursor setting
//   when doing an user-import snapshot.
static bool s_mouse_cursor_locked = false;

void MCMacPlatformLockCursor(void)
{
    s_mouse_cursor_locked = true;
}

void MCMacPlatformUnlockCursor(void)
{
    s_mouse_cursor_locked = false;
    
    if (s_mouse_window == nil)
        MCMacPlatformResetCursor();
    else
        MCMacPlatformHandleMouseCursorChange(s_mouse_window);
}

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
		
		// If the click occurred within the double click time and double click
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
			
            // MW-2014-06-11: [[ Bug 12339 ]] Only send a mouseRelease message if this wasn't the result of a popup menu.
			MCPlatformCallbackSendMouseRelease(s_mouse_window, p_button, false);
		}
	}
}

void MCMacPlatformHandleMouseCursorChange(MCPlatformWindowRef p_window)
{
    // If the mouse is not currently over the window whose cursor has
    // changed - do nothing.
    if (s_mouse_window != p_window)
        return;
    
    // MW-2014-06-11: [[ Bug 12436 ]] If the cursor is locked, do nothing.
    if (s_mouse_cursor_locked)
        return;
    
    MCMacPlatformWindow *t_window;
    t_window = (MCMacPlatformWindow *)p_window;
    
    // If we are on Lion+ then check to see if the mouse location is outside
    // of any of the system tracking rects (used for resizing etc.)
    extern uint4 MCmajorosversion;
    if (MCmajorosversion >= 0x1070)
    {
        // MW-2014-06-11: [[ Bug 12437 ]] Make sure we only check tracking rectangles if we have
        //   a resizable frame.
        bool t_is_resizable;
        MCPlatformGetWindowProperty(p_window, kMCPlatformWindowPropertyHasSizeWidget, kMCPlatformPropertyTypeBool, &t_is_resizable);
        
        if (t_is_resizable)
        {
            NSArray *t_tracking_areas;
            t_tracking_areas = [[t_window -> GetContainerView() superview] trackingAreas];
            
            NSPoint t_mouse_loc;
            t_mouse_loc = [t_window -> GetView() mapMCPointToNSPoint: s_mouse_position];
            for(uindex_t i = 0; i < [t_tracking_areas count]; i++)
            {
                if (NSPointInRect(t_mouse_loc, [(NSTrackingArea *)[t_tracking_areas objectAtIndex: i] rect]))
                    return;
            }
        }
    }
    
    // MW-2014-06-25: [[ Bug 12634 ]] Make sure we only change the cursor if we are not
    //   within a native view.
    if ([t_window -> GetContainerView() hitTest: [t_window -> GetView() mapMCPointToNSPoint: s_mouse_position]] == t_window -> GetView())
    {
        // Show the cursor attached to the window.
        MCPlatformCursorRef t_cursor;
        MCPlatformGetWindowProperty(p_window, kMCPlatformWindowPropertyCursor, kMCPlatformPropertyTypeCursorRef, &t_cursor);
        
        // PM-2014-04-02: [[ Bug 12082 ]] IDE no longer crashes when changing an applied pattern
        if (t_cursor != nil)
            MCPlatformSetCursor(t_cursor);
        // SN-2014-10-01: [[ Bug 13516 ]] Hiding a cursor here is not what we want to happen if a cursor hasn't been found
        else
            MCMacPlatformResetCursor();
    }
}

void MCMacPlatformHandleMouseAfterWindowHidden(void)
{
	MCMacPlatformHandleMouseMove(s_mouse_screen_position);
}

// MW-2014-06-27: [[ Bug 13284 ]] When live resizing starts, leave the window, and enter it again when it finishes.
void MCMacPlatformHandleMouseForResizeStart(void)
{
    if (s_mouse_window != nil)
        MCPlatformCallbackSendMouseLeave(s_mouse_window);
}

void MCMacPlatformHandleMouseForResizeEnd(void)
{
    if (s_mouse_window != nil)
        MCPlatformCallbackSendMouseEnter(s_mouse_window);
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
	bool t_window_changed;
	t_window_changed = false;
	if (t_new_mouse_window != s_mouse_window)
	{
		if (s_mouse_window != nil)
			MCPlatformCallbackSendMouseLeave(s_mouse_window);
		
		if (t_new_mouse_window != nil)
			MCPlatformCallbackSendMouseEnter(t_new_mouse_window);
		
		// If there is no mouse window, reset the cursor to default.
		if (t_new_mouse_window == nil)
        {
            // MW-2014-06-11: [[ Bug 12436 ]] If the cursor is locked, do nothing.
            if (!s_mouse_cursor_locked)
                MCMacPlatformResetCursor();
        }
			
		if (s_mouse_window != nil)
			MCPlatformReleaseWindow(s_mouse_window);
		
		s_mouse_window = t_new_mouse_window;
		
		if (s_mouse_window != nil)
			MCPlatformRetainWindow(s_mouse_window);
			
		t_window_changed = true;
	}
	
	// Regardless of whether we post a mouse move, update the screen mouse position.
	s_mouse_screen_position = p_screen_loc;
	
	// If we have a new mouse window, then translate screen loc and update.
	if (s_mouse_window != nil)
	{
		MCPoint t_window_loc;
		MCPlatformMapPointFromScreenToWindow(s_mouse_window, p_screen_loc, t_window_loc);
		
		if (t_window_changed ||
			t_window_loc . x != s_mouse_position . x ||
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
        
        // MW-2014-04-22: [[ Bug 12253 ]] Ending a drag-drop can cause the mouse window to go.
        // Update the mouse cursor for the mouse window.
        if (s_mouse_window != nil)
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
				
                // MW-2014-06-11: [[ Bug 12339 ]] Don't send a mouseRelease message in this case.
				if (s_mouse_was_control_click &&
					i == 0)
					MCPlatformCallbackSendMouseRelease(s_mouse_window, 2, true);
				else
					MCPlatformCallbackSendMouseRelease(s_mouse_window, i, true);
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
        // MW-2014-06-11: [[ Bug 12339 ]] Ensure mouseRelease is sent if drag is starting.
		if (t_button_to_release != 0xffffffff)
			MCPlatformCallbackSendMouseRelease(s_mouse_window, t_button_to_release, false);
		MCPlatformCallbackSendMouseLeave(s_mouse_window);
		
        // SN-2015-01-13: [[ Bug 14350 ]] The user can close the stack in
        //  a mouseLeave handler
        if (s_mouse_window != nil)
        {
            MCPlatformReleaseWindow(s_mouse_window);
            s_mouse_window = nil;
        }
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

CGFloat get_desktop_height()
{
	if (!s_have_desktop_height)
	{
		s_desktop_height = 0.0f;
		
		for (NSScreen * t_screen in [NSScreen screens])
		{
			NSRect t_rect = [t_screen frame];
			if (t_rect.origin.y + t_rect.size.height > s_desktop_height)
				s_desktop_height = t_rect.origin.y + t_rect.size.height;
		}
		
		s_have_desktop_height = true;
	}
	
	return s_desktop_height;
}

void MCMacPlatformMapScreenMCPointToNSPoint(MCPoint p, NSPoint& r_point)
{
	r_point = NSMakePoint(p . x, get_desktop_height() - p . y);
}

void MCMacPlatformMapScreenNSPointToMCPoint(NSPoint p, MCPoint& r_point)
{
	r_point . x = int16_t(p . x);
	r_point . y = int16_t(get_desktop_height() - p . y);
}

void MCMacPlatformMapScreenMCRectangleToNSRect(MCRectangle r, NSRect& r_rect)
{
	r_rect = NSMakeRect(CGFloat(r . x), get_desktop_height() - CGFloat(r . y + r . height), CGFloat(r . width), CGFloat(r . height));
}

void MCMacPlatformMapScreenNSRectToMCRectangle(NSRect r, MCRectangle& r_rect)
{
	r_rect = MCRectangleMake(int16_t(r . origin . x), int16_t(get_desktop_height() - (r . origin . y + r . size . height)), int16_t(r . size . width), int16_t(r . size . height));
}

////////////////////////////////////////////////////////////////////////////////

void MCMacPlatformShowMessageDialog(MCStringRef p_title,
                                    MCStringRef p_message)
{
    NSAlert *t_alert = [[NSAlert alloc] init];
    [t_alert addButtonWithTitle:@"OK"];
    [t_alert setMessageText: MCStringConvertToAutoreleasedNSString(p_title)];
    [t_alert setInformativeText: MCStringConvertToAutoreleasedNSString(p_message)];
    [t_alert setAlertStyle:NSInformationalAlertStyle];
    [t_alert runModal];
}

////////////////////////////////////////////////////////////////////////////////

static void display_reconfiguration_callback(CGDirectDisplayID display, CGDisplayChangeSummaryFlags flags, void *userInfo)
{
	// COCOA-TODO: Make this is a little more discerning (only need to reset if
	//   primary geometry changes).
	s_have_desktop_height = false;
}

////////////////////////////////////////////////////////////////////////////////

int platform_main(int argc, char *argv[], char *envp[])
{
	extern bool MCS_mac_elevation_bootstrap_main(int argc, char* argv[]);
	if (argc == 2 && strcmp(argv[1], "-elevated-slave") == 0)
		if (!MCS_mac_elevation_bootstrap_main(argc, argv))
			return -1;
	
	NSAutoreleasePool *t_pool;
	t_pool = [[NSAutoreleasePool alloc] init];
	
    s_callback_lock = [[NSLock alloc] init];
    
	// Create the normal NSApplication object.
	NSApplication *t_application;
	t_application = [com_runrev_livecode_MCApplication sharedApplication];
	
	// Register for reconfigurations.
	CGDisplayRegisterReconfigurationCallback(display_reconfiguration_callback, nil);
    
	if (!MCInitialize() ||
        !MCSInitialize() ||
	    !MCScriptInitialize())
		exit(-1);
    
	// On OSX, argv and envp are encoded as UTF8
	MCStringRef *t_new_argv;
	/* UNCHECKED */ MCMemoryNewArray(argc, t_new_argv);
	
	for (int i = 0; i < argc; i++)
	{
		/* UNCHECKED */ MCStringCreateWithBytes((const byte_t *)argv[i], strlen(argv[i]), kMCStringEncodingUTF8, false, t_new_argv[i]);
	}
	
	MCStringRef *t_new_envp;
	/* UNCHECKED */ MCMemoryNewArray(1, t_new_envp);
	
	int i = 0;
	uindex_t t_envp_count = 0;
	
	while (envp[i] != NULL)
	{
		t_envp_count++;
		uindex_t t_count = i;
		/* UNCHECKED */ MCMemoryResizeArray(i + 1, t_new_envp, t_count);
		/* UNCHECKED */ MCStringCreateWithBytes((const byte_t *)envp[i], strlen(envp[i]), kMCStringEncodingUTF8, false, t_new_envp[i]);
		i++;
	}
	
	/* UNCHECKED */ MCMemoryResizeArray(i + 1, t_new_envp, t_envp_count);
	t_new_envp[i] = nil;
	
	// Setup our delegate
	com_runrev_livecode_MCApplicationDelegate *t_delegate;
	t_delegate = [[com_runrev_livecode_MCApplicationDelegate alloc] initWithArgc: argc argv: t_new_argv envp: t_new_envp];
	
	// Assign our delegate
	[t_application setDelegate: t_delegate];
	
	// Run the application - this never returns!
	[t_application run];    
	
	for (i = 0; i < argc; i++)
		MCValueRelease(t_new_argv[i]);
	for (i = 0; i < t_envp_count; i++)
		MCValueRelease(t_new_envp[i]);    
	
	MCMemoryDeleteArray(t_new_argv);
	MCMemoryDeleteArray(t_new_envp);
	
	// Drain the autorelease pool.
	[t_pool release];
	
    MCScriptFinalize();
	MCFinalize();
	
	return 0;
}

////////////////////////////////////////////////////////////////////////////////

// MM-2014-07-31: [[ ThreadedRendering ]] Helper functions used to create an auto-release pool for each new thread.
void *MCMacPlatfromCreateAutoReleasePool()
{
    NSAutoreleasePool *t_pool;
    t_pool = [[NSAutoreleasePool alloc] init];
    return (void *) t_pool;
}

void MCMacPlatformReleaseAutoReleasePool(void *p_pool)
{
    NSAutoreleasePool *t_pool;
    t_pool = (NSAutoreleasePool *) p_pool;
    [t_pool release];
}
////////////////////////////////////////////////////////////////////////////////
