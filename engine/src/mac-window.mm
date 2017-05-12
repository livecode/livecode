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
#include "unicode.h"

#include "platform.h"
#include "platform-internal.h"

#include "mac-clipboard.h"
#include "mac-internal.h"

#include <objc/objc-runtime.h>

#include "graphics_util.h"

#include "osspec.h"

///////////////////////////////////////////////////////////////////////////

static bool s_inside_focus_event = false;

//static MCMacPlatformWindow *s_focused_window = nil;

////////////////////////////////////////////////////////////////////////////////

static MCRectangle MCRectangleFromNSRect(const NSRect &p_rect)
{
	return MCRectangleMake(p_rect.origin.x, p_rect.origin.y, p_rect.size.width, p_rect.size.height);
}

////////////////////////////////////////////////////////////////////////////////

static bool s_lock_responder_change = false;

@implementation com_runrev_livecode_MCWindow

- (id)initWithContentRect:(NSRect)contentRect styleMask:(NSUInteger)windowStyle backing:(NSBackingStoreType)bufferingType defer:(BOOL)deferCreation
{
    self = [super initWithContentRect: contentRect styleMask: windowStyle backing: bufferingType defer: deferCreation];
    if (self == nil)
        return nil;
    
    m_can_become_key = false;
    return self;
}

- (NSRect)movingFrame
{
    if (MCMacPlatformApplicationWindowIsMoving([(MCWindowDelegate *)[self delegate] platformWindow]))
        return m_moving_frame;
    else
        return [self frame];
}

- (void)setMovingFrame:(NSRect)p_moving_frame
{
    m_moving_frame = p_moving_frame;
}

- (void)setCanBecomeKeyWindow: (BOOL)p_value
{
	m_can_become_key = p_value;
}

// The default implementation doesn't allow borderless windows to become key.
- (BOOL)canBecomeKeyWindow
{
	if (MCMacPlatformApplicationPseudoModalFor() != nil)
        return NO;
    
    return m_can_become_key;
}

- (BOOL)makeFirstResponder: (NSResponder *)p_responder
{
	if (MCMacPlatformApplicationPseudoModalFor() != nil)
        return NO;
    
    NSResponder *t_previous;
	t_previous = [self firstResponder];
    
	if (![super makeFirstResponder: p_responder])
		return NO;
	
	if (s_lock_responder_change)
		return YES;
	
	if ([p_responder isKindOfClass: [NSView class]])
	{
		NSView *t_view;
		t_view = (NSView *)p_responder;
		while(t_view != nil)
		{
			if ([t_view respondsToSelector:@selector(com_runrev_livecode_nativeViewId)])
			{
				[(MCWindowDelegate *)[self delegate] viewFocusSwitched: (uintptr_t)[t_view com_runrev_livecode_nativeViewId]];
				return YES;
			}
			
			t_view = [t_view superview];
		}
	}
	
	[(MCWindowDelegate *)[self delegate] viewFocusSwitched: 0];
	
	return YES;
}

// MW-2014-04-23: [[ Bug 12270 ]] If user reshaping then apply standard
//   constrain, otherwise don't constrain.
- (NSRect)constrainFrameRect: (NSRect)frameRect toScreen: (NSScreen *)screen
{
    MCWindowDelegate *t_delegate;
    t_delegate = (MCWindowDelegate *)[self delegate];
    if ([t_delegate inUserReshape])
        return [super constrainFrameRect: frameRect toScreen: screen];
    
    return frameRect;
}

@end

@implementation com_runrev_livecode_MCPanel

// SN-2014-10-01: [[ Bug 13522 ]] Popup menus changed to inherit NSPanel instead of NSWindow,
//   allowing us to set the workWhenModal to true.
//   All the popup-specific code has been moved from com_runrev_livecode_MCWindow to com_runrev_livecode_MCPanel

// MW-2014-06-11: [[ Bug 12451 ]] When we 'popup' a window we install a monitor to catch
//   other mouse events outside the host window. This allows us to close them and still
//   continue to process the event as normal.

// MW-2014-07-16: [[ Bug 12708 ]] Mouse events targeted as popup windows don't cancel popups
//   so that cascading menus work.
- (bool)isPopupWindow
{
    return m_is_popup;
}

- (id)initWithContentRect:(NSRect)contentRect styleMask:(NSUInteger)windowStyle backing:(NSBackingStoreType)bufferingType defer:(BOOL)deferCreation
{
    self = [super initWithContentRect: contentRect styleMask: windowStyle backing: bufferingType defer: deferCreation];
    if (self == nil)
        return nil;
    
    m_can_become_key = false;
    m_is_popup = false;
    m_monitor = nil;
    
    return self;
}

- (NSRect)movingFrame
{
    if (MCMacPlatformApplicationWindowIsMoving([(MCWindowDelegate *)[self delegate] platformWindow]))
        return m_moving_frame;
    else
        return [self frame];
}

- (void)setMovingFrame:(NSRect)p_moving_frame
{
    m_moving_frame = p_moving_frame;
}

- (void)dealloc
{
    if (m_monitor != nil)
        [NSEvent removeMonitor: m_monitor];
    [super dealloc];
}

- (void)setCanBecomeKeyWindow: (BOOL)p_value
{
	m_can_become_key = p_value;
}

// The default implementation doesn't allow borderless windows to become key.
- (BOOL)canBecomeKeyWindow
{
	if (MCMacPlatformApplicationPseudoModalFor() != nil)
        return NO;
    
    return m_can_become_key;
}

- (BOOL)makeFirstResponder: (NSResponder *)p_responder
{
	if (MCMacPlatformApplicationPseudoModalFor() != nil)
        return NO;
    
    NSResponder *t_previous;
	t_previous = [self firstResponder];

	if (![super makeFirstResponder: p_responder])
		return NO;
	
	if (s_lock_responder_change)
		return YES;
	
	if ([p_responder isKindOfClass: [NSView class]])
	{
		NSView *t_view;
		t_view = (NSView *)p_responder;
		while(t_view != nil)
		{
			if ([t_view respondsToSelector:@selector(com_runrev_livecode_nativeViewId)])
			{
				[(MCWindowDelegate *)[self delegate] viewFocusSwitched: (uintptr_t)[t_view com_runrev_livecode_nativeViewId]];
				return YES;
			}
			
			t_view = [t_view superview];
		}
	}
	
	[(MCWindowDelegate *)[self delegate] viewFocusSwitched: 0];
	
	return YES;
}

// MW-2014-04-23: [[ Bug 12270 ]] If user reshaping then apply standard
//   constrain, otherwise don't constrain.
- (NSRect)constrainFrameRect: (NSRect)frameRect toScreen: (NSScreen *)screen
{
    MCWindowDelegate *t_delegate;
    t_delegate = (MCWindowDelegate *)[self delegate];
    if ([t_delegate inUserReshape])
        return [super constrainFrameRect: frameRect toScreen: screen];
    
    return frameRect;
}

- (void)popupWindowClosed: (NSNotification *)notification
{
    if (m_monitor != nil)
    {
        [NSEvent removeMonitor: m_monitor];
        m_monitor = nil;
    }
    
    [[NSNotificationCenter defaultCenter] removeObserver:self name:NSWindowWillCloseNotification object:self];
    [[NSNotificationCenter defaultCenter] removeObserver:self name:NSApplicationDidResignActiveNotification object:nil];
    
    m_is_popup = false;
}

- (void)popupWindowShouldClose: (NSNotification *)notification
{
    [self close];
    MCPlatformCallbackSendWindowCancel([(MCWindowDelegate *)[self delegate] platformWindow]);
}

- (void)popupAndMonitor
{
    NSWindow *t_window;
    t_window = self;
    
    m_is_popup = true;
    
    // MW-2014-07-24: [[ Bug 12720 ]] We must not change focus if inside a focus/unfocus event
    //   as it seems to confuse Cocoa.
    if (s_inside_focus_event)
        [self orderFront: nil];
    else
        [self makeKeyAndOrderFront: nil];
    
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(popupWindowClosed:) name:NSWindowWillCloseNotification object:self];
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(popupWindowShouldClose:) name:NSApplicationDidResignActiveNotification object:nil];
    
    m_monitor = [NSEvent addLocalMonitorForEventsMatchingMask: NSLeftMouseDownMask | NSRightMouseDownMask | NSOtherMouseDownMask
                                                      handler:^(NSEvent *incomingEvent)
                 {
                     NSEvent *result = incomingEvent;
                     NSWindow *targetWindowForEvent = [incomingEvent window];
                     
                     if (![targetWindowForEvent respondsToSelector: @selector(isPopupWindow)] ||
                         ![targetWindowForEvent isPopupWindow])
                     {
                         if (targetWindowForEvent != t_window)
						 {
                             [self popupWindowShouldClose: nil];
							 // IM-2015-04-21: [[ Bug 15129 ]] Block mouse down event after cancelling popup
							 result = nil;
						 }
                     }
                     
                     return result;
                 }];
}

@end

////////////////////////////////////////////////////////////////////////////////

void MCMacPlatformWindowWindowMoved(NSWindow *self, MCPlatformWindowRef p_window)
{
    // The NSWindow's frame isn't updated whilst a window is being moved. However,
    // the window server's bounds *are*. Thus we ask for the updated bounds from
    // the window server each time we get a mouseDragged event (sent on the main
    // thread to the NSWindow from the MCWindowTrackingThread event tap).
    
    uintptr_t t_window_ids[1];
    t_window_ids[0] = (uintptr_t)[self windowNumber];
    
    CFArrayRef t_window_id_array;
    t_window_id_array = CFArrayCreate(kCFAllocatorDefault, (const void **)t_window_ids, 1, NULL);
    
	NSArray *t_info_array;
	t_info_array = (NSArray *)CGWindowListCreateDescriptionFromArray(t_window_id_array);
	
    if ([t_info_array count] > 0)
    {
        NSDictionary *t_rect_dict;
        t_rect_dict = [[t_info_array objectAtIndex: 0] objectForKey: (NSString *)kCGWindowBounds];
        
        CGRect t_rect;
        CGRectMakeWithDictionaryRepresentation((CFDictionaryRef)t_rect_dict, &t_rect);
        
        NSScreen * t_screen = [[NSScreen screens] objectAtIndex:0];
        t_rect . origin . y = [t_screen frame] . size . height - t_rect . origin . y - t_rect . size . height;
        
        [(NSWindow <com_runrev_livecode_MCMovingFrame> *)self setMovingFrame:NSRectFromCGRect(t_rect)];
        
        ((MCMacPlatformWindow *)p_window) -> ProcessDidMove();
    }
    
    [t_info_array release];
    CFRelease(t_window_id_array);
}

@implementation com_runrev_livecode_MCWindowDelegate

- (id)initWithPlatformWindow: (MCMacPlatformWindow *)window
{
	self = [super init];
	if (self == nil)
		return nil;
	
	m_window = window;
    
    m_user_reshape = false;
	
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

- (bool)inUserReshape
{
    return m_user_reshape;
}

//////////

- (BOOL)windowShouldClose:(id)sender
{
	m_window -> ProcessCloseRequest();
	return NO;
}

// MW-2014-06-25: [[ Bug 12632 ]] Make sure we map frame to content rects and back
//   again (engine WindowConstrain expects content sizes, whereas Cocoa expects
//   frame sizes).
- (NSSize)windowWillResize:(NSWindow *)sender toSize:(NSSize)frameSize
{
    MCRectangle t_frame;
    t_frame = MCRectangleMake(0, 0, frameSize . width, frameSize . height);
    
    MCRectangle t_content;
    m_window -> DoMapFrameRectToContentRect(t_frame, t_content);
    
	MCPoint t_size;
	t_size . x = t_content . width;
    t_size . y = t_content . height;
	
	MCPoint t_new_size;
	MCPlatformCallbackSendWindowConstrain(m_window, t_size, t_new_size);
    
    t_content . width = t_new_size . x;
    t_content . height = t_new_size . y;
    
    m_window -> DoMapContentRectToFrameRect(t_content, t_frame);
	
	return NSMakeSize(t_frame . width, t_frame . height);
}

- (void)windowWillMove:(NSNotification *)notification
{
    
    NSWindow * t_window = m_window -> GetHandle();
    if ([t_window respondsToSelector: @selector(setMovingFrame:)])
        [((NSWindow <com_runrev_livecode_MCMovingFrame> *)t_window) setMovingFrame:[t_window frame]];
    
    if (!m_window -> IsSynchronizing())
    {
        // MW-2014-04-23: [[ Bug 12270 ]] The user has started moving the window
        //   so set us as reshape by user.
        m_user_reshape = true;
        
        // MW-2014-08-14: [[ Bug 13016 ]] Ask our NSApp to start sending us windowMoved
        //   messages.
		MCMacPlatformApplicationWindowStartedMoving(m_window);
    }
}

- (void)windowMoveFinished
{
    // IM-2014-10-29: [[ Bug 13814 ]] Make sure we unset the user reshape flag once dragging is finished.
	m_user_reshape = false;
}

- (void)windowDidMove:(NSNotification *)notification
{
    m_window -> ProcessDidMove();
}

- (void)windowWillStartLiveResize:(NSNotification *)notification
{
    // MW-2014-04-23: [[ Bug 12270 ]] The user has started sizing the window
    //   so set us as reshape by user.
    m_user_reshape = true;
    
    // MW-2014-06-27: [[ Bug 13284 ]] Make sure the mouse temporarily leaves the window.
    MCMacPlatformHandleMouseForResizeStart();
}

- (void)windowDidEndLiveResize:(NSNotification *)notification
{
    // MW-2014-06-27: [[ Bug 13284 ]] Make sure the mouse returns to the window.
    MCMacPlatformHandleMouseForResizeEnd();
    
    // MW-2014-04-23: [[ Bug 12270 ]] The user has stopped sizing the window
    //   so unset us as reshape by user.
    m_user_reshape = false;
}

- (void)windowWillMiniaturize:(NSNotification *)notification
{
	m_window -> ProcessWillMiniaturize();
}

- (void)windowDidMiniaturize:(NSNotification *)notification
{
	m_window -> ProcessDidMiniaturize();
}

- (void)windowDidDeminiaturize:(NSNotification *)notification
{
	m_window -> ProcessDidDeminiaturize();
}

// MW-2014-07-22: [[ Bug 12720 ]] Mark the period we are inside a focus event handler.
- (void)windowDidBecomeKey:(NSNotification *)notification
{
    if (s_inside_focus_event)
        m_window -> ProcessDidBecomeKey();
    else
    {
        s_inside_focus_event = true;
        m_window -> ProcessDidBecomeKey();
        s_inside_focus_event = false;
    }
}

// MW-2014-07-22: [[ Bug 12720 ]] Mark the period we are inside a focus event handler.
- (void)windowDidResignKey:(NSNotification *)notification
{
    if (s_inside_focus_event)
        m_window -> ProcessDidResignKey();
    else
    {
        s_inside_focus_event = true;
        m_window -> ProcessDidResignKey();
        s_inside_focus_event = false;
    }
}

- (void)windowDidChangeBackingProperties:(NSNotification *)notification
{
    //MCLog("didChangeBacking %lf", objc_msgSend_fpret(m_window -> GetHandle(), @selector(backingScaleFactor)));
}

- (void)didEndSheet: (NSWindow *)sheet returnCode: (NSInteger)returnCode contextInfo: (void *)info
{
}

//////////

- (void)viewFocusSwitched:(uint32_t)p_id
{
	MCPlatformCallbackSendViewFocusSwitched(m_window, p_id);
}

@end

////////////////////////////////////////////////////////////////////////////////

@implementation com_runrev_livecode_MCWindowView

//////////

- (id)initWithPlatformWindow:(MCMacPlatformWindow *)window
{
	self = [super initWithFrame: NSZeroRect];
	if (self == nil)
		return nil;
	
    m_window = window;
	m_tracking_area = nil;
	m_use_input_method = false;
	m_input_method_event = nil;
	
	// Register for all dragging types (well ones that convert to 'data' anyway).
	// MW-2014-06-10: [[ Bug 12388 ]] Make sure we respond to our private datatype.
    [self registerForDraggedTypes: [NSArray arrayWithObjects: (NSString *)kUTTypeData, @"com.runrev.livecode.private", nil]];
	
	return self;
}

- (void)dealloc
{
	[m_tracking_area release];
	[super dealloc];
}

/////////

- (MCMacPlatformWindow *)platformWindow
{
    return m_window;
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

- (BOOL)canBecomeKeyView
{
	if (MCMacPlatformApplicationPseudoModalFor() != nil)
        return NO;
    
    return YES;
}

- (BOOL)acceptsFirstMouse:(NSEvent *)theEvent
{
    if (MCMacPlatformApplicationPseudoModalFor() != nil)
        return NO;
    
    // MW-2014-04-23: [[ CocoaBackdrop ]] This method is called after the window has
    //   been re-ordered but before anything else - an ideal time to sync the backdrop.
    MCMacPlatformSyncBackdrop();
	return YES;
}

- (BOOL)acceptsFirstResponder
{
	if (MCMacPlatformApplicationPseudoModalFor() != nil)
        return NO;
    
    return YES;
}

- (BOOL)becomeFirstResponder
{
	if (MCMacPlatformApplicationPseudoModalFor() != nil)
        return NO;
    
    //MCPlatformCallbackSendViewFocus([(MCWindowDelegate *)[[self window] delegate] platformWindow]);
	return YES;
}

- (BOOL)resignFirstResponder
{
	//MCPlatformCallbackSendViewUnfocus([(MCWindowDelegate *)[[self window] delegate] platformWindow]);
	return YES;
}

- (BOOL)com_runrev_livecode_subviewBecomeFirstResponder: (uint32_t)p_id
{
	//MCPlatformCallbackSendNativeViewFocus([(MCWindowDelegate *)[[self window] delegate] platformWindow], p_id);
	return YES;
}

- (BOOL)com_runrev_livecode_subviewResignFirstResponder: (uint32_t)p_id
{
	//MCPlatformCallbackSendNativeViewUnfocus([(MCWindowDelegate *)[[self window] delegate] platformWindow], p_id);
	return YES;
}

//////////

- (void)mouseDown: (NSEvent *)event
{
	if (MCMacPlatformApplicationPseudoModalFor() != nil)
        return;
    
    if ([self useTextInput])
		if ([[self inputContext] handleEvent: event])
			return;
	
	[self handleMousePress: event isDown: YES];
}

- (void)mouseUp: (NSEvent *)event
{
    if (MCMacPlatformApplicationPseudoModalFor() != nil)
        return;
    
    if ([self useTextInput])
		if ([[self inputContext] handleEvent: event])
			return;
	
	[self handleMousePress: event isDown: NO];
}

- (void)mouseMoved: (NSEvent *)event
{
    if (MCMacPlatformApplicationPseudoModalFor() != nil)
        return;
    
    [self handleMouseMove: event];
}

- (void)mouseDragged: (NSEvent *)event
{
    if (MCMacPlatformApplicationPseudoModalFor() != nil)
        return;
    
    if ([self useTextInput])
		if ([[self inputContext] handleEvent: event])
			return;
	
	[self handleMouseMove: event];
}

- (void)rightMouseDown: (NSEvent *)event
{
    if (MCMacPlatformApplicationPseudoModalFor() != nil)
        return;
    
    // [[ Bug ]] When a sheet is shown, for some reason we get rightMouseDown events.
    if ([[self window] attachedSheet] != nil)
        return;
    
	[self handleMousePress: event isDown: YES];
}

- (void)rightMouseUp: (NSEvent *)event
{
    if (MCMacPlatformApplicationPseudoModalFor() != nil)
    return;
    
    // [[ Bug ]] When a sheet is shown, for some reason we get rightMouseDown events.
    if ([[self window] attachedSheet] != nil)
        return;
    
	[self handleMousePress: event isDown: NO];
}

- (void)rightMouseMoved: (NSEvent *)event
{
    if (MCMacPlatformApplicationPseudoModalFor() != nil)
        return;
    
    [self handleMouseMove: event];
}

- (void)rightMouseDragged: (NSEvent *)event
{
    if (MCMacPlatformApplicationPseudoModalFor() != nil)
        return;
    
    [self handleMouseMove: event];
}

- (void)otherMouseDown: (NSEvent *)event
{
    if (MCMacPlatformApplicationPseudoModalFor() != nil)
        return;
    
    // [[ Bug ]] When a sheet is shown, for some reason we get rightMouseDown events.
    if ([[self window] attachedSheet] != nil)
        return;
    
	[self handleMousePress: event isDown: YES];
}

- (void)otherMouseUp: (NSEvent *)event
{
    if (MCMacPlatformApplicationPseudoModalFor() != nil)
        return;
    
    // [[ Bug ]] When a sheet is shown, for some reason we get rightMouseDown events.
    if ([[self window] attachedSheet] != nil)
        return;
    
	[self handleMousePress: event isDown: NO];
}

- (void)otherMouseMoved: (NSEvent *)event
{
    if (MCMacPlatformApplicationPseudoModalFor() != nil)
        return;
    
    [self handleMouseMove: event];
}

- (void)otherMouseDragged: (NSEvent *)event
{
    if (MCMacPlatformApplicationPseudoModalFor() != nil)
        return;
    
    [self handleMouseMove: event];
}

- (void)mouseEntered: (NSEvent *)event
{
    // MW-2014-04-22: [[ Bug 12255 ]] mouseEntered can get sent before dragEntered and when it is
    //   it isn't possible to tell if a dragging operation is going to start. Thus we don't handle
    //   this message, and rely on the first mouseMove instead.
	//[self handleMouseMove: event];
}

- (void)mouseExited: (NSEvent *)event
{
    if (MCMacPlatformApplicationPseudoModalFor() != nil)
        return;
    
    [self handleMouseMove: event];
}

- (void)flagsChanged: (NSEvent *)event
{
	MCMacPlatformHandleModifiersChanged(MCMacPlatformMapNSModifiersToModifiers([event modifierFlags]));
}

// MW-2014-06-25: [[ Bug 12370 ]] Factor out key mapping code so it can be used by both
//   IME and direct key events.
static void map_key_event(NSEvent *event, MCPlatformKeyCode& r_key_code, codepoint_t& r_mapped, codepoint_t& r_unmapped)
{
	MCMacPlatformMapKeyCode([event keyCode], [event modifierFlags], r_key_code);
    
    // SN-2014-12-08: [[ Bug 14067 ]] If the key pressed was a numpad key, then we
    // do not produce a mapped char. That will allow a different treatment, as we
    // do not want to use the numeric value of the native char to get the rawKeyDown
    // parameter.
    if (([event modifierFlags] & NSNumericPadKeyMask) != 0)
    {
        MCMacMapNSStringToCodepoint([event characters], r_unmapped);
        r_mapped = r_unmapped;
        return;
    }
    
    if (r_key_code > 0xff00 && r_key_code <= 0xffff)
    {
        r_mapped = r_unmapped = 0xffffffffU;
        return;
    }
    
    // MW-2014-07-17: [[ Bug 12747 ]] For reasons currently unknown to me, if Cmd is in the modifier
    //   flags, the characters / charactersIgnoringModifiers fields seem to be inverted.
    if (([event modifierFlags] & NSCommandKeyMask) == 0)
    {
        if (!MCMacMapNSStringToCodepoint([event characters], r_mapped))
            r_mapped = 0xffffffffU;
        
        if (!MCMacMapNSStringToCodepoint([event charactersIgnoringModifiers], r_unmapped))
            r_unmapped = 0xffffffffU;
        
        // MW-2014-08-26: [[ Bug 13279 ]] If Ctrl is used, then Space onwards gets mapped to 0 onwards,
        //   so undo this.
        if (([event modifierFlags] & NSControlKeyMask) != 0 &&
            r_mapped < 32)
        {
            r_mapped = r_unmapped;
        }
    }
    else
    {
        if (!MCMacMapNSStringToCodepoint([event charactersIgnoringModifiers], r_mapped))
            r_mapped = 0xffffffffU;
        
        if (!MCMacMapNSStringToCodepoint([event characters], r_unmapped))
            r_unmapped = 0xffffffffU;
    }
    
	// The unicode range 0xF700 - 0xF8FF is reserved by the system to indicate
	// keys which have no printable value, but represent an action (such as F11,
	// PageUp etc.). We don't want this mapping as we do it ourselves from the
	// keycode, so if the mapped codepoint is in this range we reset it.
	if (r_mapped >= 0xf700 && r_mapped < 0xf8ff)
		r_mapped = r_unmapped = 0xffffffffU;
	
	// Now, if we have an unmapped codepoint, but no mapped codepoint then we
	// propagate the unmapped codepoint as the mapped codepoint. This is so that
	// dead-keys (when input methods are turned off) propagate an appropriate
	// char (e.g. alt-e generates no mapped codepoint, but we want an e).
	if (r_mapped == 0xffffffffU)
		r_mapped = r_unmapped;
}

- (void)keyDown: (NSEvent *)event
{
	MCMacPlatformHandleModifiersChanged(MCMacPlatformMapNSModifiersToModifiers([event modifierFlags]));
	
    // Make the event key code to a platform keycode and codepoints.
    MCPlatformKeyCode t_key_code;
    codepoint_t t_mapped_codepoint, t_unmapped_codepoint;
    map_key_event(event, t_key_code, t_mapped_codepoint, t_unmapped_codepoint);
    
    // Notify the host that a keyDown event has been received - this is to work around the
    // issue with IME not playing nice with rawKey messages. Eventually this should work
    // by completely separating rawKey messages from text input messages.
    MCPlatformCallbackSendRawKeyDown([self platformWindow], t_key_code, t_mapped_codepoint, t_unmapped_codepoint);
    
	if ([self useTextInput])
	{
		// Store the event being processed, if it results in a doCommandBySelector,
		// then dispatch as a key event.
		m_input_method_event = event;
		if ([[self inputContext] handleEvent: event])
		{
			m_input_method_event = nil;
			return;
		}
		m_input_method_event = nil;
	}
	[self handleKeyPress: event isDown: YES];
}

- (void)keyUp: (NSEvent *)event
{
	MCMacPlatformHandleModifiersChanged(MCMacPlatformMapNSModifiersToModifiers([event modifierFlags]));
	
    // SN-2014-10-31: [[ Bug 13832 ]] We want the rawKeyUp and keyUp messages to be sent in their due course when
    //  useTextInput is true, since the key messages have been enqueued appropriately
	[self handleKeyPress: event isDown: NO];
}

//////////

- (BOOL)useTextInput
{
	MCMacPlatformWindow *t_window;
	t_window = [self platformWindow];
	if (t_window == nil)
		return NO;
	
	return t_window -> IsTextInputActive();
}

- (void)insertText:(id)aString replacementRange:(NSRange)replacementRange
{
	MCMacPlatformWindow *t_window;
	t_window = [self platformWindow];
	if (t_window == nil)
		return;
	
	// It seems that if replacementRange is NSNotFound then we should
	// take the marked range if any, otherwise take the selected range.
	if (replacementRange . location == NSNotFound)
	{
		MCRange t_marked_range, t_selected_range;
		MCPlatformCallbackSendTextInputQueryTextRanges([self platformWindow], t_marked_range, t_selected_range);
		if (t_marked_range . offset != UINDEX_MAX)
			replacementRange = NSMakeRange(t_marked_range . offset, t_marked_range . length);
		else
			replacementRange = NSMakeRange(t_selected_range . offset, t_selected_range . length);
	}
	
	NSString *t_string;
	if ([aString isKindOfClass: [NSAttributedString class]])
		t_string = [aString string];
	else
		t_string = aString;
	
	NSUInteger t_length;
	t_length = [t_string length];
	
	unichar_t *t_chars;
	t_chars = new unichar_t[t_length];
	
	[t_string getCharacters: t_chars range: NSMakeRange(0, t_length)];
	
	// Insert the text replacing the given range and setting the selection after
	// it... (It isn't clear what should - if anything - be selected after insert!).
	MCPlatformCallbackSendTextInputInsertText(t_window,
											  t_chars, t_length,
											  MCRangeMake(replacementRange . location, replacementRange . length),
											  MCRangeMake(replacementRange . location + t_length, 0),
											  false);
	
	delete t_chars;
	
	// It appears that 'insert' is a commit operation - we've already told the input field to
	// not mark text, so now we just need to tell the input context.
	[[self inputContext] discardMarkedText];
	[[self inputContext] invalidateCharacterCoordinates];
}

- (void)doCommandBySelector:(SEL)aSelector
{
    //MCLog("doCommandBySelector:", 0);
	
	MCMacPlatformWindow *t_window;
	t_window = [self platformWindow];
	if (t_window == nil)
		return;
	
	// If the event came about as a result of a keyDown input method event
	// then process the key directly.
	if (m_input_method_event != nil)
	{
		[self handleKeyPress: m_input_method_event isDown: YES];
        // SN-2014-11-03: [[ Bug 13832 ]] keyUp will be called, and the message will have been queued.
//		[self handleKeyPress: m_input_method_event isDown: NO];
        
        // PM-2014-09-15: [[ Bug 13442 ]] Set m_input_method_event to nil to prevent rawKeyDown from firing twice when altKey is down
        m_input_method_event = nil;
	}
	else
	{
		MCPlatformTextInputAction t_action;
		if (MCMacMapSelectorToTextInputAction(aSelector, t_action))
		{
			MCPlatformCallbackSendTextInputAction([self platformWindow], t_action);
			return;
		}
		
		[super doCommandBySelector: aSelector];
	}
}

- (void)setMarkedText:(id)aString selectedRange:(NSRange)newSelection replacementRange:(NSRange)replacementRange
{
	MCMacPlatformWindow *t_window;
	t_window = [self platformWindow];
	if (t_window == nil)
		return;
	
	// It seems that if replacementRange is NSNotFound then we should
	// take the marked range if any, otherwise take the selected range.
	if (replacementRange . location == NSNotFound)
	{
		MCRange t_marked_range, t_selected_range;
		MCPlatformCallbackSendTextInputQueryTextRanges(t_window, t_marked_range, t_selected_range);
		if (t_marked_range . offset != UINDEX_MAX)
			replacementRange = NSMakeRange(t_marked_range . offset, t_marked_range . length);
		else
			replacementRange = NSMakeRange(t_selected_range . offset, t_selected_range . length);
	}
	
	NSString *t_string;
	if ([aString isKindOfClass: [NSAttributedString class]])
		t_string = [aString string];
	else
		t_string = aString;
	
	NSUInteger t_length;
	t_length = [t_string length];
	
	unichar_t *t_chars;
	t_chars = new unichar_t[t_length];
	
	[t_string getCharacters: t_chars range: NSMakeRange(0, t_length)];

	MCPlatformCallbackSendTextInputInsertText(t_window,
											  t_chars, t_length,
											  MCRangeMake(replacementRange . location, replacementRange . length),
											  MCRangeMake(replacementRange . location + newSelection . location, newSelection . length),
											  true);

	delete t_chars;
	
	// If the operation had a zero length string, it means remove markedText
	// (it seems!).
	if (t_length == 0)
		[[self inputContext] discardMarkedText];
	
	// We've inserted something, so tell the context to invalidate its caches.
	[[self inputContext] invalidateCharacterCoordinates];
}

- (void)unmarkText
{
    //MCLog("unmarkText", 0);
	
	MCMacPlatformWindow *t_window;
	t_window = [self platformWindow];
	if (t_window == nil)
		return;
	
	// This is the 'commit' operation, any marked text currently in the buffer should be accepted.
	// We do this by inserting an empty string, leaving the current selection untouched.
	
	MCRange t_marked_range, t_selected_range;
	MCPlatformCallbackSendTextInputQueryTextRanges(t_window, t_marked_range, t_selected_range);
	
	MCPlatformCallbackSendTextInputInsertText(t_window, nil, 0, MCRangeMake(0, 0), t_selected_range, false);
	
	[[self inputContext] discardMarkedText];
}

- (NSRange)selectedRange
{
	MCMacPlatformWindow *t_window;
	t_window = [self platformWindow];
	if (t_window == nil)
		return NSMakeRange(NSNotFound, 0);
	
	MCRange t_marked_range, t_selected_range;
	MCPlatformCallbackSendTextInputQueryTextRanges(t_window, t_marked_range, t_selected_range);
    //MCLog("selectedRange() = (%d, %d)", t_selected_range . offset, t_selected_range . length);
	return NSMakeRange(t_selected_range . offset, t_selected_range . length);
}

- (NSRange)markedRange
{
	MCMacPlatformWindow *t_window;
	t_window = [self platformWindow];
	if (t_window == nil)
		return NSMakeRange(NSNotFound, 0);
	
	MCRange t_marked_range, t_selected_range;
	MCPlatformCallbackSendTextInputQueryTextRanges(t_window, t_marked_range, t_selected_range);
    //MCLog("markedRange() = (%d, %d)", t_marked_range . offset, t_marked_range . length);
	return NSMakeRange(t_marked_range . offset, t_marked_range . length);
}

- (BOOL)hasMarkedText
{
	MCMacPlatformWindow *t_window;
	t_window = [self platformWindow];
	if (t_window == nil)
		return NO;
	
	MCRange t_marked_range, t_selected_range;
	MCPlatformCallbackSendTextInputQueryTextRanges(t_window, t_marked_range, t_selected_range);
    //MCLog("hasMarkedText() = %d", t_marked_range . offset != UINDEX_MAX);
	return t_marked_range . offset != UINDEX_MAX;
}

- (NSAttributedString *)attributedSubstringForProposedRange:(NSRange)aRange actualRange:(NSRangePointer)actualRange
{
	MCMacPlatformWindow *t_window;
	t_window = [self platformWindow];
	if (t_window == nil)
		return nil;
	
	unichar_t *t_chars;
	uindex_t t_char_count;
	MCRange t_actual_range;
	MCPlatformCallbackSendTextInputQueryText(t_window,
											 MCRangeMake(aRange . location, aRange . length),
											 t_chars, t_char_count,
											 t_actual_range);
	
	NSString *t_string;
	NSAttributedString *t_attr_string;
	t_string = [[NSString alloc] initWithCharacters: t_chars length: t_char_count];
	t_attr_string = [[[NSAttributedString alloc] initWithString: t_string] autorelease];
	[t_string release];
	
	if (actualRange != nil)
		*actualRange = NSMakeRange(t_actual_range . offset, t_actual_range . length);
	
	return t_attr_string;
}

- (NSArray*)validAttributesForMarkedText
{
    //MCLog("validAttributesForMarkedText() = []", nil);
	return [NSArray array];
}

- (NSRect)firstRectForCharacterRange:(NSRange)aRange actualRange:(NSRangePointer)actualRange
{
	MCMacPlatformWindow *t_window;
	t_window = [self platformWindow];
	if (t_window == nil)
		return NSZeroRect;
	
	MCRange t_actual_range;
	MCRectangle t_rect;
	MCPlatformCallbackSendTextInputQueryTextRect(t_window,
												 MCRangeMake(aRange . location, aRange . length),
												 t_rect,
												 t_actual_range);
	
	NSRect t_ns_rect;
	t_ns_rect = [self mapMCRectangleToNSRect: t_rect];
	
	if (actualRange != nil)
		*actualRange = NSMakeRange(t_actual_range . offset, t_actual_range . length);
	
    //MCLog("firstRectForCharacterRange(%d, %d -> %d, %d) = [%d, %d, %d, %d]", aRange . location, aRange . length, t_actual_range . offset, t_actual_range . length, t_rect . x, t_rect . y, t_rect . width, t_rect . height);
	
	t_ns_rect . origin = [[self window] convertBaseToScreen: t_ns_rect . origin];
	
	return t_ns_rect;
}

- (NSUInteger)characterIndexForPoint:(NSPoint)aPoint
{
	aPoint = [[self window] convertScreenToBase: aPoint];
	
	MCPoint t_location;
	t_location = [self mapNSPointToMCPoint: aPoint];
	
	MCMacPlatformWindow *t_window;
	t_window = [self platformWindow];
	if (t_window == nil)
		return 0;
	
	uindex_t t_index;
	MCPlatformCallbackSendTextInputQueryTextIndex(t_window, t_location, t_index);
	
    //MCLog("characterIndexForPoint(%d, %d) = %d", t_location . x, t_location . y, t_index);
	
	return t_index;
}

//////////

// MW-2014-08-06: [[ Bug 13114 ]] It seems Cmd+Ctrl+Arrow by-passes the input system and various
//   other mechanisms for no apparant reason. Therefore if we get these selectors we dispatch
//   directly.
- (void)synthesizeKeyPress: (uint32_t)p_key_code
{
	MCMacPlatformWindow *t_window;
	t_window = [self platformWindow];
	if (t_window == nil)
		return;
    
    t_window -> ProcessKeyDown(p_key_code, 0xffffffffU, 0xffffffffU);
    t_window -> ProcessKeyUp(p_key_code, 0xffffffffU, 0xffffffffU);
}

- (void)moveLeft:(id)sender
{
    [self synthesizeKeyPress: 0xff51];
}

- (void)moveRight:(id)sender
{
    [self synthesizeKeyPress: 0xff53];
}

- (void)moveUp:(id)sender
{
    [self synthesizeKeyPress: 0xff52];
}

- (void)moveDown:(id)sender
{
    [self synthesizeKeyPress: 0xff54];
}

// MW-2014-05-12: [[ Bug 12383 ]] We need these handlers to ensure things are
//   not disabled in menus when they have a standard tag.

- (void)undo:(id)sender
{
	[self handleAction: @selector(undo:) with: sender];
}

- (void)redo:(id)sender
{
	[self handleAction: @selector(redo:) with: sender];
}

- (void)cut:(id)sender
{
	[self handleAction: @selector(cut:) with: sender];
}

- (void)copy:(id)sender
{
	[self handleAction: @selector(copy:) with: sender];
}

- (void)paste:(id)sender
{
	[self handleAction: @selector(paste:) with: sender];
}

- (void)selectAll:(id)sender
{
	[self handleAction: @selector(selectAll:) with: sender];
}

- (void)delete:(id)sender
{
	[self handleAction: @selector(delete:) with: sender];
}

//////////

#if DEBUG_ACTION_MESSAGES
- (BOOL)respondsToSelector:(SEL)aSelector
{
    MCLog("selector = %s", sel_getName(aSelector));
    return [super respondsToSelector: aSelector];
}
#endif

- (BOOL)validateMenuItem:(NSMenuItem *)menuItem
{
	return [menuItem isEnabled];
}

- (void)handleAction:(SEL)selector with:(id)sender
{
	if (![sender isKindOfClass: [NSMenuItem class]])
	{
		[super tryToPerform: selector with: sender];
		return;
	}
	
	NSMenuItem *t_item;
	t_item = (NSMenuItem *)sender;
	
	if (![[[t_item menu] delegate] isKindOfClass: [MCMenuDelegate class]])
	{
		[super tryToPerform: selector with: sender];
		return;
	}
	
	[(MCMenuDelegate *)[[t_item menu] delegate] menuItemSelected: sender];
}

//////////

- (void)scrollWheel: (NSEvent *)event
{
	MCMacPlatformHandleModifiersChanged(MCMacPlatformMapNSModifiersToModifiers([event modifierFlags]));
	
	MCMacPlatformWindow *t_window;
	t_window = [self platformWindow];
	if (t_window == nil)
		return;
	
	t_window -> ProcessMouseScroll([event deltaX], [event deltaY]);
}

//////////

- (BOOL)shouldDelayWindowOrderingForEvent: (NSEvent *)event
{
	return NO;
}

- (NSDragOperation)draggingSourceOperationMaskForLocal: (BOOL)isLocal
{
	return m_allowed_drag_operations;
}

- (BOOL)ignoreModifierKeysWhileDragging
{
	return YES;
}

- (void)draggedImage:(NSImage *)image beganAt:(NSPoint)point
{
}

- (void)draggedImage:(NSImage *)image movedTo:(NSPoint)point
{
}

- (void)draggedImage:(NSImage *)image endedAt:(NSPoint)point operation:(NSDragOperation)operation
{
}

- (NSDragOperation)dragImage:(NSImage *)image offset:(NSSize)offset allowing:(NSDragOperation)operations pasteboard:(NSPasteboard *)pboard
{
	NSEvent *t_mouse_event;
	t_mouse_event = MCMacPlatformGetLastMouseEvent();
	if (t_mouse_event == nil)
		return NSDragOperationNone;
		
	if ([t_mouse_event window] != [self window])
		return NSDragOperationNone;
	
	NSPoint t_image_loc;
	t_image_loc . x = [t_mouse_event locationInWindow] . x + offset . width;
	t_image_loc . y = [t_mouse_event locationInWindow] . y - ([image size] . height - offset . height);
	
	m_allowed_drag_operations = operations;
	
	[self dragImage: image
				at: t_image_loc 
				offset: NSZeroSize 
				event: t_mouse_event
				pasteboard: pboard
				source: self
				slideBack: YES];
				
	return NSDragOperationNone;
}

//////////

- (void)updateDraggingCursor: (NSDragOperation)op
{
	bool t_is_not_allowed_cursor;
	t_is_not_allowed_cursor = ([NSCursor currentCursor] == [NSCursor operationNotAllowedCursor]);
	if (op == NSDragOperationNone && !t_is_not_allowed_cursor)
		[[NSCursor operationNotAllowedCursor] push];
	else if (op != NSDragOperationNone && t_is_not_allowed_cursor)
		[[NSCursor operationNotAllowedCursor] pop];
}

- (BOOL)wantsPeriodicDraggingUpdates
{
	// We only want updates when the mouse moves, or similar.
	return NO;
}

- (NSDragOperation)draggingEntered: (id<NSDraggingInfo>)sender
{
    // Create a wrapper around the drag board for this operation if the drag
    // source is outside this instance of LiveCode.
    MCAutoRefcounted<MCMacRawClipboard> t_dragboard;
    if ([sender draggingSource] == nil)
        t_dragboard = new MCMacRawClipboard([sender draggingPasteboard]);
    
	NSDragOperation t_ns_operation;
	
	MCMacPlatformWindow *t_window;
	t_window = [self platformWindow];
	if (t_window != nil)
	{
		MCPlatformDragOperation t_operation;
		t_window -> HandleDragEnter(t_dragboard, t_operation);
		t_ns_operation = MCMacPlatformMapDragOperationToNSDragOperation(t_operation);
	}
	else
		t_ns_operation = NSDragOperationNone;
	
	[self updateDraggingCursor: t_ns_operation];
	
	return t_ns_operation;
}

- (void)draggingExited: (id<NSDraggingInfo>)sender
{
	MCMacPlatformWindow *t_window;
	t_window = [self platformWindow];
	t_window -> HandleDragLeave();
}

- (NSDragOperation)draggingUpdated: (id<NSDraggingInfo>)sender
{
	MCPoint t_location;
	MCMacPlatformMapScreenNSPointToMCPoint([[self window] convertBaseToScreen: [sender draggingLocation]], t_location);
	
	MCMacPlatformWindow *t_window;
	t_window = [self platformWindow];
	if (t_window == nil)
		return NSDragOperationNone;
	
	MCPlatformMapPointFromScreenToWindow(t_window, t_location, t_location);
	
	// Update the modifier key state.
	MCMacPlatformHandleModifiersChanged(MCMacPlatformMapNSModifiersToModifiers([[NSApp currentEvent] modifierFlags]));
	
	MCPlatformDragOperation t_operation;
	t_window -> HandleDragMove(t_location, t_operation);
	
	NSDragOperation t_ns_operation;
	t_ns_operation = MCMacPlatformMapDragOperationToNSDragOperation(t_operation);
	
	[self updateDraggingCursor: t_ns_operation];
	
	return t_ns_operation;
}

- (BOOL)performDragOperation: (id<NSDraggingInfo>)sender
{
	MCMacPlatformWindow *t_window;
	t_window = [self platformWindow];
	if (t_window == nil)
		return NO;
	
	bool t_accepted;
	t_window -> HandleDragDrop(t_accepted);
	return t_accepted;
}

- (BOOL)prepareForDragOperation: (id<NSDraggingInfo>)sender
{
	// We alwasys return YES here since the last script handler must have been
	// happy with the dragAction for this to get to this point.
	return YES;
}

- (void)concludeDragOperation:(id<NSDraggingInfo>)sender
{
    // MW-2014-07-15: [[ Bug 12773 ]] Make sure mouseMove is sent after end of drag operation.
    MCMacPlatformSyncMouseAfterTracking();
}

//////////

- (void)handleMouseMove: (NSEvent *)event
{
	MCMacPlatformHandleModifiersChanged(MCMacPlatformMapNSModifiersToModifiers([event modifierFlags]));
	
	MCMacPlatformWindow *t_window;
	t_window = [self platformWindow];
	if (t_window == nil)
		return;
	
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
	t_window = [self platformWindow];
	if (t_window == nil)
		return;
	
	// Swap button numbers 1 and 2.
	NSUInteger t_button;
	if ([event buttonNumber] == 1)
		t_button = 2;
	else if ([event buttonNumber] == 2)
		t_button = 1;
	else
		t_button = [event buttonNumber];
	
	t_window -> ProcessMousePress(t_button, pressed == YES);
}
					 
- (void)handleKeyPress: (NSEvent *)event isDown: (BOOL)pressed
{
	MCPlatformKeyCode t_key_code;
	codepoint_t t_mapped_codepoint;
	codepoint_t t_unmapped_codepoint;
    map_key_event(event, t_key_code, t_mapped_codepoint, t_unmapped_codepoint);
    
	// Finally we process.
	MCMacPlatformWindow *t_window;
	t_window = [self platformWindow];
	if (t_window == nil)
		return;
	
	if (pressed)
		t_window -> ProcessKeyDown(t_key_code, t_mapped_codepoint, t_unmapped_codepoint);
	else
		t_window -> ProcessKeyUp(t_key_code, t_mapped_codepoint, t_unmapped_codepoint);
}

//////////

- (void)setFrameSize: (NSSize)size
{
	[super setFrameSize: size];
    
	if ([self window] == nil)
		return;
	
	MCMacPlatformWindow *t_window;
	t_window = [self platformWindow];
	if (t_window == nil)
		return;
	
	t_window -> ProcessDidResize();
}
	
//////////

- (void)drawRect: (NSRect)dirtyRect
{	
	MCMacPlatformWindow *t_window;
	t_window = [self platformWindow];
	if (t_window == nil)
		return;

	CGContextRef t_graphics;
	t_graphics = (CGContextRef)[[NSGraphicsContext currentContext] graphicsPort];
	
	const NSRect *t_update_rects;
	NSInteger t_update_rect_count;
	[self getRectsBeingDrawn: &t_update_rects count: &t_update_rect_count];
	
	MCGRegionRef t_update_region;
	t_update_region = nil;
	
	/* UNCHECKED */ MCGRegionCreate(t_update_region);
	for(NSInteger i = 0; i < t_update_rect_count; i++)
		/* UNCHECKED */ MCGRegionAddRect(t_update_region, MCRectangleToMCGIntegerRectangle([self mapNSRectToMCRectangle: t_update_rects[i]]));

	//////////
    
    // Save the context state
    CGContextSaveGState(t_graphics);

	{
		// IM-2014-09-30: [[ Bug 13501 ]] Prevent system event checking which can cause re-entrant calls to drawRect
		MCMacPlatformDisableEventChecking();
        MCMacPlatformSurface t_surface(t_window, t_graphics, t_update_region);
        t_window -> HandleRedraw(&t_surface, t_update_region);
		MCMacPlatformEnableEventChecking();
    }
    
    // Restore the context state
    CGContextRestoreGState(t_graphics);

	//////////
	
	MCGRegionDestroy(t_update_region);
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

- (MCPoint)mapNSPointToMCPoint: (NSPoint)p
{
	return MCPointMake(p . x, [self bounds] . size . height - p . y);
}

- (NSPoint)mapMCPointToNSPoint: (MCPoint)p
{
	return NSMakePoint(p . x, [self bounds] . size . height - p . y);
}

@end

@implementation com_runrev_livecode_MCWindowContainerView

- (id)initWithPlatformWindow:(MCMacPlatformWindow *)window
{
    self = [super initWithFrame: NSZeroRect];
    if (self == nil)
        return nil;
    
    m_window = window;
    
    return self;
}

- (void)setFrameSize: (NSSize)size
{
	CGFloat t_height_diff = size.height - [self frame].size.height;
	
    [super setFrameSize: size];
    
	// IM-2015-09-01: [[ BrowserWidget ]] Adjust subviews upward to retain same height from top of view
	NSArray *t_subviews;
	t_subviews = [self subviews];
	for (uint32_t i = 0; i < [t_subviews count]; i++)
	{
		NSView *t_subview;
		t_subview = [t_subviews objectAtIndex:i];
		
        // Adjust any layers added by externals (revbrowser).
        if ([t_subview respondsToSelector:@selector(com_runrev_livecode_nativeViewId)])
        {
			NSPoint t_origin;
			t_origin = [t_subview frame].origin;
			
            t_origin.y += t_height_diff;
			
			[t_subview setFrameOrigin:t_origin];
		}
	}
    
	MCMacPlatformWindow *t_window = m_window;
    if (t_window != nil)
        [t_window -> GetView() setFrameSize: size];
}

@end

////////////////////////////////////////////////////////////////////////////////

MCMacPlatformWindow::MCMacPlatformWindow(void)
{
	m_delegate = nil;
	m_view = nil;
    m_container_view = nil;
	m_handle = nil;
	
	m_shadow_changed = false;
	m_synchronizing = false;
	m_has_sheet = false;
	m_frame_locked = false;
	
	m_parent = nil;
}

MCMacPlatformWindow::~MCMacPlatformWindow(void)
{
	[m_handle setDelegate: nil];
	[m_handle setContentView: nil];
    
	[m_handle close];
	[m_handle release];
    [m_view removeFromSuperview];
    [m_view release];
	[m_container_view release];
	[m_delegate release];
}

////////////////////////////////////////////////////////////////////////////////

MCWindowView *MCMacPlatformWindow::GetView(void)
{
	return m_view;
}

MCWindowContainerView *MCMacPlatformWindow::GetContainerView(void)
{
    return m_container_view;
}

id MCMacPlatformWindow::GetHandle(void)
{
	// IM-2016-06-13: [[ Bug 17815 ]] Ensure the handle has been created before returning.
	if (m_handle == nil)
		RealizeAndNotify();
	return m_handle;
}

void MCMacPlatformWindow::MapMCPointToNSPoint(MCPoint p_location, NSPoint& r_location)
{
	r_location . x = p_location . x;
	r_location . y = m_content . height - p_location . y;
}

void MCMacPlatformWindow::MapMCRectangleToNSRect(MCRectangle p_rect, NSRect& r_ns_rect)
{
	r_ns_rect = [m_view mapMCRectangleToNSRect: p_rect];
}

////////////////////////////////////////////////////////////////////////////////

bool MCMacPlatformWindow::IsSynchronizing(void)
{
    return m_synchronizing;
}

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
	
	// IM-2015-01-30: [[ Bug 14140 ]] Reset any rect changes while the frame is locked.
	if (m_frame_locked)
	{
		m_changes.content_changed = true;
		DoSynchronize();
		return;
	}
	
	// Get the window's new content rect.
	NSRect t_new_cocoa_content;
	t_new_cocoa_content = [m_window_handle contentRectForFrameRect: [(NSWindow <com_runrev_livecode_MCMovingFrame>*)m_window_handle movingFrame]];
	
	// Map from cocoa coords.
	MCRectangle t_content;
	MCMacPlatformMapScreenNSRectToMCRectangle(t_new_cocoa_content, t_content);
	
	// Make sure we don't tickle the event queue whilst resizing, otherwise
	// redraws can be done by the OS during the process resulting in tearing
	// as the window resizes.
	MCMacPlatformDisableEventChecking();
	
	// And get the super class to deal with it.
	HandleReshape(t_content);
	
	MCMacPlatformEnableEventChecking();
}

void MCMacPlatformWindow::ProcessDidResize(void)
{
	ProcessDidMove();
}

void MCMacPlatformWindow::ProcessWillMiniaturize(void)
{
	// Unset the parent window to make sure things don't propagate.
	if ([m_window_handle parentWindow] != nil)
		[[m_window_handle parentWindow] removeChildWindow: m_window_handle];
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

void MCMacPlatformWindow::ProcessMouseScroll(CGFloat dx, CGFloat dy)
{
	MCMacPlatformHandleMouseScroll(dx, dy);
}

// SN-2014-07-11: [[ Bug 12747 ]] Shortcuts: the uncomment script shortcut cmd _ does not work
// Changed parameters order to follow *KeyDown functions consistency
void MCMacPlatformWindow::ProcessKeyDown(MCPlatformKeyCode p_key_code, codepoint_t p_mapped_char, codepoint_t p_unmapped_char)
{
	HandleKeyDown(p_key_code, p_mapped_char, p_unmapped_char);
}

void MCMacPlatformWindow::ProcessKeyUp(MCPlatformKeyCode p_key_code, codepoint_t p_mapped_char, codepoint_t p_unmapped_char)
{
	HandleKeyUp(p_key_code, p_mapped_char, p_unmapped_char);
}

////////////////////////////////////////////////////////////////////////////////

void MCMacPlatformWindow::SetFrameLocked(bool p_locked)
{
	m_frame_locked = p_locked;
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
			t_window_level = kCGNormalWindowLevel;
			break;
		case kMCPlatformWindowStylePalette:
			t_window_level = kCGFloatingWindowLevel;
			break;
		case kMCPlatformWindowStyleDialog:
			t_window_level = kCGModalPanelWindowLevel;
			break;
		case kMCPlatformWindowStyleUtility:
			t_window_level = kCGUtilityWindowLevel;
			break;
		case kMCPlatformWindowStylePopUp:
			t_window_level = kCGPopUpMenuWindowLevel;
			break;
		case kMCPlatformWindowStyleToolTip:
			t_window_level = kCGStatusWindowLevel;
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
    // MW-2014-04-30: [[ Bug 12328 ]] Don't defer window creation, otherwise we don't have a windowId.
	if (t_window_level == kCGFloatingWindowLevel)
		m_panel_handle = [[com_runrev_livecode_MCPanel alloc] initWithContentRect: t_cocoa_content styleMask: t_window_style backing: NSBackingStoreBuffered defer: NO];
    // SN-2014-10-01: [[ Bug 13522 ]] We want all the pulldown menus to work, even when there is a modal window.
    else if (t_window_level == kCGPopUpMenuWindowLevel)
    {
		m_panel_handle = [[com_runrev_livecode_MCPanel alloc] initWithContentRect: t_cocoa_content styleMask: t_window_style backing: NSBackingStoreBuffered defer: NO];
        [m_panel_handle setWorksWhenModal: YES];
    }
	else
		m_window_handle = [[com_runrev_livecode_MCWindow alloc] initWithContentRect: t_cocoa_content styleMask: t_window_style backing: NSBackingStoreBuffered defer: NO];
    
    // AL-2014-07-23: [[ Bug 12131 ]] Explicitly set frame, since initWithContentRect
    //  assumes content is on primary screen.
    NSRect t_cocoa_frame;
    t_cocoa_frame = [m_window_handle frameRectForContentRect: t_cocoa_content];
    
    [m_window_handle setFrame: t_cocoa_frame display: YES];
    
    if (m_delegate == nil)
        m_delegate = [[com_runrev_livecode_MCWindowDelegate alloc] initWithPlatformWindow: this];
	[m_window_handle setDelegate: m_delegate];
	
    m_container_view = [[MCWindowContainerView alloc] initWithPlatformWindow: this];
    [m_container_view setAutoresizesSubviews: NO];
    
	m_view = [[com_runrev_livecode_MCWindowView alloc] initWithPlatformWindow: this];
    [m_container_view addSubview: m_view];
    
	[m_window_handle setContentView: m_container_view];
    
    // MW-2014-04-23: [[ Bug 12080 ]] If the window is a palette and the app is active
    //   then make sure its floating (if app is not active it gets made floating on resume).
    if (t_window_level == kCGFloatingWindowLevelKey)
        [m_panel_handle setFloatingPanel: [NSApp isActive]];
    else
        [m_window_handle setLevel: t_window_level];
	[m_window_handle setOpaque: m_is_opaque && m_mask == nil];
	[m_window_handle setHasShadow: m_has_shadow];
	if (!m_has_zoom_widget)
		[[m_window_handle standardWindowButton: NSWindowZoomButton] setEnabled: NO];
	[m_window_handle setAlphaValue: m_opacity];
	[m_window_handle setDocumentEdited: m_has_modified_mark];
	[m_window_handle setReleasedWhenClosed: NO];
	
	[m_window_handle setCanBecomeKeyWindow: m_style != kMCPlatformWindowStylePopUp && m_style != kMCPlatformWindowStyleToolTip];
    
    // MW-2014-04-08: [[ Bug 12080 ]] Make sure we turn off automatic 'hiding on deactivate'.
    //   The engine handles this itself.
    [m_window_handle setHidesOnDeactivate: m_hides_on_suspend];
    
    // MERG-2015-10-11: [[ DocumentFilename ]] Set documentFilename.
    UpdateDocumentFilename();
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
	
    if (m_changes . has_shadow_changed)
        [m_window_handle setHasShadow: m_has_shadow];
    
	if (m_changes . mask_changed || m_changes . is_opaque_changed)
	{
        // MW-2014-07-29: [ Bug 12997 ]] Make sure we invalidate the whole window when
        //   the mask changes.
        [[m_window_handle contentView] setNeedsDisplay: YES];
		[m_window_handle setOpaque: m_is_opaque && m_mask == nil];
		if (m_has_shadow)
			m_shadow_changed = true;
	}
	
    // MW-2014-08-14: [[ Bug 13016 ]] Ignore any changes to the window frame whilst user
    //   is moving / resizing otherwise bad things happen.
	if (m_changes . content_changed && ![m_delegate inUserReshape])
	{
		NSRect t_cocoa_content;
		MCMacPlatformMapScreenMCRectangleToNSRect(m_content, t_cocoa_content);
		
		NSRect t_cocoa_frame;
		t_cocoa_frame = [m_window_handle frameRectForContentRect: t_cocoa_content];
		
		// COCOA-TODO: At the moment force a re-display here to stop redraw artifacts.
		//   The engine will disable screen updates appropriately to ensure atomicity.
		[m_window_handle setFrame: t_cocoa_frame display: YES];
	}
	
	if (m_changes . title_changed)
		[m_window_handle setTitle: m_title != nil ? MCStringConvertToAutoreleasedNSString(m_title) : @""];
	
	if (m_changes . has_modified_mark_changed)
		[m_window_handle setDocumentEdited: m_has_modified_mark];
	
	if (m_changes . use_live_resizing_changed)
		;
	
    // MW-2014-04-08: [[ Bug 12073 ]] If the cursor has changed, make sure we try to
    //   update it - should this window be the mouse window.
    if (m_changes . cursor_changed)
        MCMacPlatformHandleMouseCursorChange(this);
    
    // MW-2014-04-23: [[ Bug 12080 ]] Sync hidesOnSuspend.
    if (m_changes . hides_on_suspend_changed)
        [m_window_handle setHidesOnDeactivate: m_hides_on_suspend];
    
    // MERG-2014-06-02: [[ IgnoreMouseEvents ]] Sync ignoreMouseEvents.
    if (m_changes . ignore_mouse_events_changed)
        [m_window_handle setIgnoresMouseEvents: m_ignore_mouse_events];
    
    if (m_changes . document_filename_changed)
    {
        UpdateDocumentFilename();
    }
    
    m_synchronizing = false;
}

bool MCMacPlatformWindow::DoSetProperty(MCPlatformWindowProperty p_property, MCPlatformPropertyType p_type, const void *value)
{
	return false;
}

bool MCMacPlatformWindow::DoGetProperty(MCPlatformWindowProperty p_property, MCPlatformPropertyType p_type, void *r_value)
{
	switch(p_property)
	{
		case kMCPlatformWindowPropertySystemId:
			assert(p_type == kMCPlatformPropertyTypeUInt32);
            // MW-2014-04-30: [[ Bug 12328 ]] If we don't have a handle yet make sure we create one.
            if (m_window_handle == nil)
                RealizeAndNotify();
			*(uint32_t *)r_value = m_window_handle != nil ? [m_window_handle windowNumber] : 0;
			return true;
			
		case kMCPlatformWindowPropertySystemHandle:
			assert(p_type == kMCPlatformPropertyTypePointer);
			// MW-2014-04-30: [[ Bug 12328 ]] If we don't have a handle yet make sure we create one.
			if (m_window_handle == nil)
				RealizeAndNotify();
			*(void**)r_value = m_window_handle;
			return true;
		
		default:
			return false;
	}
}

void MCMacPlatformWindow::DoShow(void)
{
	if (m_style == kMCPlatformWindowStyleDialog)
		MCMacPlatformBeginModalSession(this);
	else if (m_style == kMCPlatformWindowStylePopUp)
    {
        [m_window_handle popupAndMonitor];
    }
	else
	{
		[m_view setNeedsDisplay: YES];
        // MW-2014-07-24: [[ Bug 12720 ]] We must not change focus if inside a focus/unfocus event
        //   as it seems to confuse Cocoa.
        if (s_inside_focus_event)
            [m_window_handle orderFront: nil];
        else
            [m_window_handle makeKeyAndOrderFront: nil];
	}
}

void MCMacPlatformWindow::DoShowAsSheet(MCPlatformWindowRef p_parent)
{
	if (p_parent == nil)
		return;
	
	MCMacPlatformWindow *t_parent;
	t_parent = (MCMacPlatformWindow *)p_parent;
	
	if (t_parent -> m_has_sheet)
	{
		m_is_visible = false;
		MCPlatformCallbackSendWindowClose(this);
		return;
	}

	m_parent = t_parent;
	m_parent -> Retain();
	((MCMacPlatformWindow *)m_parent) -> m_has_sheet = true;
	
	[NSApp beginSheet: m_window_handle modalForWindow: t_parent -> m_window_handle modalDelegate: m_delegate didEndSelector: @selector(didEndSheet:returnCode:contextInfo:) contextInfo: nil];
}

void MCMacPlatformWindow::DoHide(void)
{
	if (m_parent != nil)
	{
		[NSApp endSheet: m_window_handle];
		[m_window_handle orderOut: nil];
		((MCMacPlatformWindow *)m_parent) -> m_has_sheet = false;
		m_parent -> Release();
		m_parent = nil;
	}
	else if (m_style == kMCPlatformWindowStyleDialog)
	{
		MCMacPlatformEndModalSession(this);
	}
    else if (m_style == kMCPlatformWindowStylePopUp)
    {
        [m_window_handle popupWindowClosed: nil];
        [m_window_handle orderOut: nil];
    }
	else
	{
		// Unset the parent window to make sure things don't propagate.
		if ([m_window_handle parentWindow] != nil)
			[[m_window_handle parentWindow] removeChildWindow: m_window_handle];
	
		// CW-2015-09-21: [[ Bug 15979 ]] Call close instead of orderOut here to properly close
		// windows that have been iconified.
		[m_window_handle close];
	}
	
	MCMacPlatformHandleMouseAfterWindowHidden();
}

void MCMacPlatformWindow::DoFocus(void)
{
    // MW-2014-07-24: [[ Bug 12720 ]] We must not change focus if inside a focus/unfocus event
    //   as it seems to confuse Cocoa.
    if (!s_inside_focus_event)
        [m_window_handle makeKeyWindow];
}

void MCMacPlatformWindow::DoRaise(void)
{
	[m_window_handle orderFront: nil];
}

static uint32_t s_rect_count = 0;
bool MCMacDoUpdateRegionCallback(void *p_context, const MCRectangle &p_rect)
{
	MCWindowView *t_view = static_cast<MCWindowView*>(p_context);
	[t_view setNeedsDisplayInRect: [t_view mapMCRectangleToNSRect: p_rect]];
	
	return true;
}
void MCMacPlatformWindow::DoUpdate(void)
{	
	// If the shadow has changed (due to the mask changing) we must disable
	// screen updates otherwise we get a flicker.
	// IM-2015-02-23: [[ WidgetPopup ]] Assume shadow changes when redrawing a non-opaque widget
	bool t_shadow_changed;
	t_shadow_changed = (m_shadow_changed || !m_is_opaque) && m_has_shadow;
	
	if (t_shadow_changed)
		NSDisableScreenUpdates();
	
	// Mark the bounding box of the dirty region for needing display.
	// COCOA-TODO: Make display update more specific.
	s_rect_count = 0;
	MCRegionForEachRect(m_dirty_region, MCMacDoUpdateRegionCallback, m_view);
	
	// Force a re-display, this will cause drawRect to be invoked on our view
	// which in term will result in a redraw window callback being sent.
	[m_view displayIfNeeded];
	
	// Re-enable screen updates if needed.
	if (t_shadow_changed)
    {
        // MW-2014-06-11: [[ Bug 12495 ]] Turn the shadow off and on to force recaching.
        [m_window_handle setHasShadow: NO];
        [m_window_handle setHasShadow: YES];
		NSEnableScreenUpdates();
    }
}

void MCMacPlatformWindow::DoIconify(void)
{
	[m_window_handle miniaturize: nil];
}

void MCMacPlatformWindow::DoUniconify(void)
{
	[m_window_handle deminiaturize: nil];
}

void MCMacPlatformWindow::DoConfigureTextInput(void)
{
}

void MCMacPlatformWindow::DoResetTextInput(void)
{
	[[GetView() inputContext] discardMarkedText];
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

// MERG-2015-10-11: [[ DocumentFilename ]] Set documentFilename.
void MCMacPlatformWindow::UpdateDocumentFilename(void)
{
    MCStringRef t_native_filename;
    
    NSString * t_represented_filename;
    t_represented_filename = nil;
    
    if (!MCStringIsEmpty(m_document_filename) && MCS_pathtonative(m_document_filename, t_native_filename))
    {
        t_represented_filename = MCStringConvertToAutoreleasedNSString(t_native_filename);
    }
    else
        t_represented_filename = @"";
    
    // It appears setRepresentedFilename can't be set to nil
    [m_window_handle setRepresentedFilename: t_represented_filename];
}

////////////////////////////////////////////////////////////////////////////////

static bool MCAlphaToCGImageNoCopy(const MCGRaster &p_alpha, CGImageRef &r_image)
{
	bool t_success = true;
	
	CGImageRef t_image = nil;
	CGColorSpaceRef t_colorspace = nil;
	CFDataRef t_data = nil;
	CGDataProviderRef t_dp = nil;
	
	if (t_success)
		t_success = nil != (t_data = CFDataCreateWithBytesNoCopy(kCFAllocatorDefault, (UInt8*)p_alpha.pixels, p_alpha.stride *  p_alpha.height, kCFAllocatorNull));
	
	if (t_success)
		t_success = nil != (t_dp = CGDataProviderCreateWithCFData(t_data));
	
	if (t_success)
		t_success = nil != (t_colorspace = CGColorSpaceCreateDeviceGray());
	
	if (t_success)
		t_success = nil != (t_image = CGImageCreate(p_alpha.width, p_alpha.height, 8, 8, p_alpha.stride, t_colorspace, kCGImageAlphaNone, t_dp, nil, false, kCGRenderingIntentDefault));
	
	CGColorSpaceRelease(t_colorspace);
	CGDataProviderRelease(t_dp);
	CFRelease(t_data);
	
	if (t_success)
		r_image = t_image;
	
	return t_success;
}

void MCPlatformWindowMaskCreateWithAlphaAndRelease(int32_t p_width, int32_t p_height, int32_t p_stride, void *p_bits, MCPlatformWindowMaskRef& r_mask)
{
	MCMacPlatformWindowMask *t_mask;
	t_mask = nil;
	
	bool t_success;
	t_success = MCMemoryNew(t_mask);
	
	if (t_success)
	{
		t_mask->references = 1;
		
		t_mask->mask.pixels = p_bits;
		t_mask->mask.stride = p_stride;
		t_mask->mask.width = p_width;
		t_mask->mask.height = p_height;
		t_mask->mask.format = kMCGRasterFormat_A;
		
		t_success = MCAlphaToCGImageNoCopy(t_mask->mask, t_mask->cg_mask);
	}
	
	if (t_success)
		r_mask = (MCPlatformWindowMaskRef)t_mask;
	else
	{
		MCPlatformWindowMaskRelease((MCPlatformWindowMaskRef)t_mask);
		r_mask = nil;
	}
}

void MCPlatformWindowMaskRetain(MCPlatformWindowMaskRef p_mask)
{
	if (p_mask == nil)
		return;
	
	MCMacPlatformWindowMask *t_mask;
	t_mask = (MCMacPlatformWindowMask*)p_mask;
	
	t_mask->references++;
}

void MCPlatformWindowMaskRelease(MCPlatformWindowMaskRef p_mask)
{
	if (p_mask == nil)
		return;

	MCMacPlatformWindowMask *t_mask;
	t_mask = (MCMacPlatformWindowMask*)p_mask;
	
	if (t_mask->references > 1)
	{
        // PM-2015-02-13: [[ Bug 14588 ]] Decrease ref count
		t_mask->references--;
		return;
	}
	
	CGImageRelease(t_mask->cg_mask);
	MCMemoryDeallocate(t_mask->mask.pixels);
	
	MCMemoryDelete(t_mask);
}

////////////////////////////////////////////////////////////////////////////////

static NSView *MCMacPlatformFindView(MCPlatformWindowRef p_window, uint32_t p_id)
{
	MCMacPlatformWindow *t_window;
	t_window = (MCMacPlatformWindow *)p_window;

	NSView *t_parent_view;
	t_parent_view = t_window -> GetView();

	if (p_id != 0)
	{
		NSArray *t_subviews;
		t_subviews = [t_parent_view subviews];
		for(uindex_t i = 0; i < [t_subviews count]; i++)
		{
			NSView *t_view;
			t_view = (NSView *)[t_subviews objectAtIndex: i];
			if ([t_view respondsToSelector:@selector(com_runrev_livecode_nativeViewId)])
				if ((uintptr_t)[t_view com_runrev_livecode_nativeViewId] == p_id)
					return t_view;
		}
	}
	
	return t_parent_view;
}

void MCPlatformSwitchFocusToView(MCPlatformWindowRef p_window, uint32_t p_id)
{
	NSView *t_view;
	t_view = MCMacPlatformFindView(p_window, p_id);
	
	MCMacPlatformWindow *t_window;
	t_window = (MCMacPlatformWindow *)p_window;
	
	s_lock_responder_change = true;
	if ([t_view nextValidKeyView] != nil &&
		[[t_view nextValidKeyView] previousValidKeyView] != t_view)
		t_view = [t_view nextValidKeyView];
	
	/*if (![t_view canBecomeKeyView])
	{
		NSView *t_next_view;
		t_next_view = [t_view nextValidKeyView];
		if (t_next_view != nil)
			t_view = t_next_view;
	}*/
	[(com_runrev_livecode_MCWindow *)(t_window -> GetHandle()) makeFirstResponder: t_view];
	s_lock_responder_change = false;
}

////////////////////////////////////////////////////////////////////////////////

bool MCMacMapNSStringToCodepoint(NSString *p_string, codepoint_t& r_codepoint)
{
	// If the length of the string is zero, do nothing.
	if ([p_string length] == 0)
		return false;
	
	// If the length of the string > 0 then do nothing.
	if ([p_string length] > 2)
		return false;
	
	// If the length of the string is one, its easy.
	if ([p_string length] == 1)
	{
		r_codepoint = [p_string characterAtIndex: 0];
		return true;
	}
	
	// Otherwise check for surrogate pairs.
	unichar t_high, t_low;
	t_high = [p_string characterAtIndex: 0];
	t_low = [p_string characterAtIndex: 1];
	
	// If we don't have a high then a low, do nothing.
	if (!MCUnicodeCodepointIsHighSurrogate(t_high) ||
		!MCUnicodeCodepointIsLowSurrogate(t_low))
		return false;
	
	// We have a valid surrogate pair so return its value!
	r_codepoint = ((t_high - 0xD800) << 10) | (t_low - 0xDC00);
	
	return true;
}

bool MCMacMapCodepointToNSString(codepoint_t p_codepoint, NSString*& r_string)
{
	if (p_codepoint < 65536)
	{
		unichar t_char;
		t_char = p_codepoint & 0xffff;
		r_string = [[NSString alloc] initWithCharacters: &t_char length: 1];
		return true;
	}
	
	unichar t_chars[2];
	t_chars[0] = (p_codepoint >> 10) + 0xD800;
	t_chars[1] = (p_codepoint & 0x3ff) + 0xDC00;
	r_string = [[NSString alloc] initWithCharacters: t_chars length: 2];
	return true;
}

////////////////////////////////////////////////////////////////////////////////

static struct { SEL selector; MCPlatformTextInputAction action; } s_mac_input_action_map[] =
{
	{ @selector(capitalizeWord:), kMCPlatformTextInputActionCapitalizeWord },
	{ @selector(changeCaseOfLetter:), kMCPlatformTextInputActionChangeCaseOfLetter },
	{ @selector(deleteBackward:), kMCPlatformTextInputActionDeleteBackward },
	{ @selector(deleteBackwardByDecomposingPreviousCharacter:), kMCPlatformTextInputActionDeleteBackwardByDecomposingPreviousCharacter },
	{ @selector(deleteForward:), kMCPlatformTextInputActionDeleteForward },
	{ @selector(deleteToBeginningOfLine:), kMCPlatformTextInputActionDeleteToBeginningOfLine },
	{ @selector(deleteToBeginningOfParagraph:), kMCPlatformTextInputActionDeleteToBeginningOfParagraph },
	{ @selector(deleteToEndOfLine:), kMCPlatformTextInputActionDeleteToEndOfLine },
	{ @selector(deleteToEndOfParagraph:), kMCPlatformTextInputActionDeleteToEndOfParagraph },
	{ @selector(deleteWordBackward:), kMCPlatformTextInputActionDeleteWordBackward },
	{ @selector(deleteWordForward:), kMCPlatformTextInputActionDeleteWordForward },
	{ @selector(insertBacktab:), kMCPlatformTextInputActionInsertBacktab },
	{ @selector(insertContainerBreak:), kMCPlatformTextInputActionInsertContainerBreak },
	{ @selector(insertLineBreak:), kMCPlatformTextInputActionInsertLineBreak },
	{ @selector(insertNewline:), kMCPlatformTextInputActionInsertNewline },
	{ @selector(insertParagraphSeparator:), kMCPlatformTextInputActionInsertParagraphSeparator },
	{ @selector(insertTab:), kMCPlatformTextInputActionInsertTab },
	{ @selector(lowercaseWord:), kMCPlatformTextInputActionLowercaseWord },
	{ @selector(moveBackward:), kMCPlatformTextInputActionMoveBackward },
	{ @selector(moveBackwardAndModifySelection:), kMCPlatformTextInputActionMoveBackwardAndModifySelection },
	{ @selector(moveParagraphForwardAndModifySelection:), kMCPlatformTextInputActionMoveParagraphForwardAndModifySelection },
	{ @selector(moveParagraphBackwardAndModifySelection:), kMCPlatformTextInputActionMoveParagraphBackwardAndModifySelection },
	{ @selector(moveToBeginningOfDocumentAndModfySelection:), kMCPlatformTextInputActionMoveToBeginningOfDocumentAndModfySelection },
	{ @selector(moveToEndOfDocumentAndModfySelection:), kMCPlatformTextInputActionMoveToEndOfDocumentAndModfySelection },
	{ @selector(moveToBeginningOfLineAndModfySelection:), kMCPlatformTextInputActionMoveToBeginningOfLineAndModfySelection },
	{ @selector(moveToEndOfLineAndModfySelection:), kMCPlatformTextInputActionMoveToEndOfLineAndModfySelection },
	{ @selector(moveToBeginningOfParagraphAndModfySelection:), kMCPlatformTextInputActionMoveToBeginningOfParagraphAndModfySelection },
	{ @selector(moveToEndOfParagraphAndModfySelection:), kMCPlatformTextInputActionMoveToEndOfParagraphAndModfySelection },
	{ @selector(moveToLeftEndOfLine:), kMCPlatformTextInputActionMoveToLeftEndOfLine },
	{ @selector(moveToLeftEndOfLineAndModfySelection:), kMCPlatformTextInputActionMoveToLeftEndOfLineAndModfySelection },
	{ @selector(moveToRightEndOfLine:), kMCPlatformTextInputActionMoveToRightEndOfLine },
	{ @selector(moveToRightEndOfLineAndModfySelection:), kMCPlatformTextInputActionMoveToRightEndOfLineAndModfySelection },
	{ @selector(moveDown:), kMCPlatformTextInputActionMoveDown },
	{ @selector(moveDownAndModifySelection:), kMCPlatformTextInputActionMoveDownAndModifySelection },
	{ @selector(moveForward:), kMCPlatformTextInputActionMoveForward },
	{ @selector(moveForwardAndModifySelection:), kMCPlatformTextInputActionMoveForwardAndModifySelection },
	{ @selector(moveLeft:), kMCPlatformTextInputActionMoveLeft },
	{ @selector(moveLeftAndModifySelection:), kMCPlatformTextInputActionMoveLeftAndModifySelection },
	{ @selector(moveRight:), kMCPlatformTextInputActionMoveRight },
	{ @selector(moveRightAndModifySelection:), kMCPlatformTextInputActionMoveRightAndModifySelection },
	{ @selector(moveToBeginningOfDocument:), kMCPlatformTextInputActionMoveToBeginningOfDocument },
	{ @selector(moveToBeginningOfLine:), kMCPlatformTextInputActionMoveToBeginningOfLine },
	{ @selector(moveToBeginningOfParagraph:), kMCPlatformTextInputActionMoveToBeginningOfParagraph },
	{ @selector(moveToEndOfDocument:), kMCPlatformTextInputActionMoveToEndOfDocument },
	{ @selector(moveToEndOfLine:), kMCPlatformTextInputActionMoveToEndOfLine },
	{ @selector(moveToEndOfParagraph:), kMCPlatformTextInputActionMoveToEndOfParagraph },
	{ @selector(moveUp:), kMCPlatformTextInputActionMoveUp },
	{ @selector(moveUpAndModifySelection:), kMCPlatformTextInputActionMoveUpAndModifySelection },
	{ @selector(moveWordBackward:), kMCPlatformTextInputActionMoveWordBackward },
	{ @selector(moveWordBackwardAndModifySelection:), kMCPlatformTextInputActionMoveWordBackwardAndModifySelection },
	{ @selector(moveWordForward:), kMCPlatformTextInputActionMoveWordForward },
	{ @selector(moveWordForwardAndModifySelection:), kMCPlatformTextInputActionMoveWordForwardAndModifySelection },
	{ @selector(moveWordLeft:), kMCPlatformTextInputActionMoveWordLeft },
	{ @selector(moveWordLeftAndModifySelection:), kMCPlatformTextInputActionMoveWordLeftAndModifySelection },
	{ @selector(moveWordRight:), kMCPlatformTextInputActionMoveWordRight },
	{ @selector(moveWordRightAndModifySelection:), kMCPlatformTextInputActionMoveWordRightAndModifySelection },
	{ @selector(pageUp:), kMCPlatformTextInputActionPageUp },
	{ @selector(pageUpAndModifySelection:), kMCPlatformTextInputActionPageUpAndModifySelection },
	{ @selector(pageDown:), kMCPlatformTextInputActionPageDown },
	{ @selector(pageDownAndModifySelection:), kMCPlatformTextInputActionPageDownAndModifySelection },
	{ @selector(scrollToBeginningOfDocument:), kMCPlatformTextInputActionScrollToBeginningOfDocument },
	{ @selector(scrollToEndOfDocument:), kMCPlatformTextInputActionScrollToEndOfDocument },
	{ @selector(scrollLineUp:), kMCPlatformTextInputActionScrollLineUp },
	{ @selector(scrollLineDown:), kMCPlatformTextInputActionScrollLineDown },
	{ @selector(scrollPageUp:), kMCPlatformTextInputActionScrollPageUp },
	{ @selector(scrollPageDown:), kMCPlatformTextInputActionScrollPageDown },
	{ @selector(selectAll:), kMCPlatformTextInputActionSelectAll },
	{ @selector(selectLine:), kMCPlatformTextInputActionSelectLine },
	{ @selector(selectParagraph:), kMCPlatformTextInputActionSelectParagraph },
	{ @selector(selectWord:), kMCPlatformTextInputActionSelectWord },
	{ @selector(transpose:), kMCPlatformTextInputActionTranspose },
	{ @selector(transposeWords:), kMCPlatformTextInputActionTransposeWords },
	{ @selector(uppercaseWord:), kMCPlatformTextInputActionUppercaseWord },
	{ @selector(yank:), kMCPlatformTextInputActionYank },
	{ @selector(cut:), kMCPlatformTextInputActionCut },
	{ @selector(copy:), kMCPlatformTextInputActionCopy },
	{ @selector(paste:), kMCPlatformTextInputActionPaste },
	{ @selector(undo:), kMCPlatformTextInputActionUndo },
	{ @selector(redo:), kMCPlatformTextInputActionRedo },
	{ @selector(delete:), kMCPlatformTextInputActionDelete },
};

bool MCMacMapSelectorToTextInputAction(SEL p_selector, MCPlatformTextInputAction& r_action)
{
	for(uindex_t i = 0; i < sizeof(s_mac_input_action_map) / sizeof(s_mac_input_action_map[0]); i++)
		if (p_selector == s_mac_input_action_map[i] . selector)
		{
			r_action = s_mac_input_action_map[i] . action;
			return true;
		}
	
	return false;
}

////////////////////////////////////////////////////////////////////////////////

void MCMacPlatformCreateWindow(MCPlatformWindowRef& r_window)
{
	r_window = new MCMacPlatformWindow;
}

////////////////////////////////////////////////////////////////////////////////

