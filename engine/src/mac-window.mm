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
#include "unicode.h"

#include "platform.h"
#include "platform-internal.h"

#include "mac-internal.h"

////////////////////////////////////////////////////////////////////////////////

static NSDragOperation s_drag_operation_result = NSDragOperationNone;

////////////////////////////////////////////////////////////////////////////////

static MCRectangle MCRectangleFromNSRect(const NSRect &p_rect)
{
	return MCRectangleMake(p_rect.origin.x, p_rect.origin.y, p_rect.size.width, p_rect.size.height);
}

////////////////////////////////////////////////////////////////////////////////

static bool s_lock_responder_change = false;

@implementation com_runrev_livecode_MCWindow

- (void)setCanBecomeKeyWindow: (BOOL)p_value
{
	m_can_become_key = p_value;
}

// The default implementation doesn't allow borderless windows to become key.
- (BOOL)canBecomeKeyWindow
{
	return m_can_become_key;
}

- (BOOL)makeFirstResponder: (NSResponder *)p_responder
{
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
				[(MCWindowDelegate *)[self delegate] viewFocusSwitched: (uint32_t)[t_view com_runrev_livecode_nativeViewId]];
				return YES;
			}
			
			t_view = [t_view superview];
		}
	}
	
	[(MCWindowDelegate *)[self delegate] viewFocusSwitched: 0];
	
	return YES;
}

@end

@implementation com_runrev_livecode_MCPanel

- (void)setCanBecomeKeyWindow: (BOOL)p_value
{
	m_can_become_key = p_value;
}

// The default implementation doesn't allow borderless windows to become key.
- (BOOL)canBecomeKeyWindow
{
	return m_can_become_key;
}

- (BOOL)makeFirstResponder: (NSResponder *)p_responder
{
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
				[(MCWindowDelegate *)[self delegate] viewFocusSwitched: (uint32_t)[t_view com_runrev_livecode_nativeViewId]];
				return YES;
			}
			
			t_view = [t_view superview];
		}
	}
	
	[(MCWindowDelegate *)[self delegate] viewFocusSwitched: 0];
	
	return YES;
}

@end

////////////////////////////////////////////////////////////////////////////////

// The window tracking thread manages an event tap on mouseUp and mouseDragged
// events. These are used to update the window frame whilst a window is being
// moved.
@interface com_runrev_livecode_MCWindowTrackingThread: NSThread<NSPortDelegate>
{
	NSPort *m_termination_port;
	BOOL m_is_running;
    
    MCMacPlatformWindow *m_window;
    bool m_signalled : 1;
}

- (id)initWithWindow: (MCMacPlatformWindow *)window;

- (MCMacPlatformWindow *)platformWindow;

- (void)setSignalled: (bool)p_value;

- (void)main;
- (void)terminate;

- (void)handlePortMessage: (NSPortMessage *)message;

@end

@compatibility_alias MCWindowTrackingThread com_runrev_livecode_MCWindowTrackingThread;

// This NSWindow addition handles a message 'windowMoved' which is generated
// by the window tracking thread.
@interface NSWindow (com_runrev_livecode_MCWindowTrackingAdditions)

- (void)com_runrev_livecode_windowMoved: (MCWindowTrackingThread *)tracker;

@end

@implementation NSWindow (com_runrev_livecode_MCWindowTrackingAdditions)

- (void)com_runrev_livecode_windowMoved: (MCWindowTrackingThread *)tracker
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
        
        _frame . origin . x = t_rect . origin . x;
        _frame . origin . y = [[NSScreen mainScreen] frame] . size . height - t_rect . origin . y - t_rect . size . height;
        
        [tracker platformWindow] -> ProcessDidMove();
    }
    
    [t_info_array release];
    CFRelease(t_window_id_array);
    
    [tracker setSignalled: false];
}

@end

@implementation com_runrev_livecode_MCWindowTrackingThread

static CGEventRef mouse_event_callback(CGEventTapProxy p_proxy, CGEventType p_type, CGEventRef p_event, void *p_refcon)
{
    MCWindowTrackingThread *t_self;
    t_self = (MCWindowTrackingThread *)p_refcon;

    // If the event is a MouseUp then terminate the thread. Otherwise if we haven't already
    // signalled for a windowMoved message, send one.
    if (p_type == kCGEventLeftMouseUp)
        [t_self terminate];
    else if (!t_self -> m_signalled)
    {
        t_self -> m_signalled = true;
        [(NSWindow *)(t_self -> m_window -> GetHandle()) performSelectorOnMainThread: @selector(com_runrev_livecode_windowMoved:) withObject: t_self waitUntilDone: NO];
    }
    
    return p_event;
}

- (id)initWithWindow: (MCMacPlatformWindow *)window
{
    self = [super init];
    if (self == nil)
        return nil;
    
    m_window = window;
    m_signalled = false;
    
    return self;
}

- (MCMacPlatformWindow *)platformWindow
{
    return m_window;
}

- (void)setSignalled:(bool)p_value
{
    m_signalled = p_value;
}

- (void)main
{
	NSRunLoop *t_loop;
	t_loop = [NSRunLoop currentRunLoop];
	
	NSData *t_future;
	t_future = [NSDate distantFuture];
	
	m_termination_port = [[NSPort port] retain];
	[m_termination_port setDelegate: self];
	[t_loop addPort: m_termination_port forMode: NSDefaultRunLoopMode];
    
	ProcessSerialNumber t_psn;
	GetCurrentProcess(&t_psn);
	
	CFMachPortRef t_event_tap_port;
	t_event_tap_port = CGEventTapCreateForPSN(&t_psn, kCGHeadInsertEventTap, kCGEventTapOptionListenOnly, CGEventMaskBit(kCGEventLeftMouseDragged) | CGEventMaskBit(kCGEventLeftMouseUp), mouse_event_callback, self);
	
	CFRunLoopSourceRef t_event_tap_source;
	t_event_tap_source = CFMachPortCreateRunLoopSource(NULL, t_event_tap_port, 0);
	CFRunLoopAddSource([t_loop getCFRunLoop], t_event_tap_source, kCFRunLoopDefaultMode);
	
	m_is_running = true;
	while(m_is_running &&
		  [t_loop runMode: NSDefaultRunLoopMode beforeDate: [NSDate distantFuture]])
		;
    
	CFRunLoopRemoveSource([t_loop getCFRunLoop], t_event_tap_source, kCFRunLoopDefaultMode);
	CFRelease(t_event_tap_source);
	CFRelease(t_event_tap_port);
    
	[m_termination_port release];
	m_termination_port = nil;
}

- (void)terminate
{
	NSPortMessage *t_message;
	t_message = [[NSPortMessage alloc] initWithSendPort: m_termination_port
											receivePort: m_termination_port
											 components: nil];
	[t_message sendBeforeDate: [NSDate distantFuture]];
	[t_message release];
}

- (void)handlePortMessage: (NSPortMessage *)message
{
	m_is_running = false;
}

@end

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
	MCPoint t_size;
	t_size . x = frameSize . width;
	t_size . y = frameSize . height;
	
	MCPoint t_new_size;
	MCPlatformCallbackSendWindowConstrain(m_window, t_size, t_new_size);
	
	return NSMakeSize(t_new_size . x, t_new_size . y);
}

- (void)windowWillMove:(NSNotification *)notification
{
    // The window server will not send continuous updates when moving a window which is
    // a bad thing. Even worse, it won't even update the frame of the window whilst it
    // is being dragged. This means that the only way to track window movement during a
    // move is to track mouse movements on a separate thread, which we do using an
    // event tap.

    // If the window is 'synchronizing'. i.e. Updating itself from property changes
    // then *don't* spawn a tracking thread as this movement will be because of script
    // setting the window frame, rather than a user interaction.
    if (!m_window -> IsSynchronizing())
    {
        // MW-2014-04-08: [[ Bug 12087 ]] Launch a tracking thread so that we get
        //   continuous moveStack messages.
        MCWindowTrackingThread *t_thread;
        t_thread = [[MCWindowTrackingThread alloc] initWithWindow: m_window];
        [t_thread autorelease];
        [t_thread start];
    }
}

- (void)windowDidMove:(NSNotification *)notification
{
	m_window -> ProcessDidMove();
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

- (void)windowDidBecomeKey:(NSNotification *)notification
{
	m_window -> ProcessDidBecomeKey();
}

- (void)windowDidResignKey:(NSNotification *)notification
{
	m_window -> ProcessDidResignKey();
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

- (id)initWithFrame:(NSRect)frameRect
{
	self = [super initWithFrame: frameRect];
	if (self == nil)
		return nil;
	
	m_tracking_area = nil;
	m_use_input_method = false;
	m_input_method_event = nil;
	
	// Register for all dragging types (well ones that convert to 'data' anyway).
	[self registerForDraggedTypes: [NSArray arrayWithObject: (NSString *)kUTTypeData]];
	
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
	MCWindowDelegate *t_delegate;
	t_delegate = (MCWindowDelegate *)[[self window] delegate];
	return [t_delegate platformWindow];
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
	return YES;
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
	if ([self useTextInput])
		if ([[self inputContext] handleEvent: event])
			return;
	
	[self handleMousePress: event isDown: YES];
}

- (void)mouseUp: (NSEvent *)event
{
	if ([self useTextInput])
		if ([[self inputContext] handleEvent: event])
			return;
	
	[self handleMousePress: event isDown: NO];
}

- (void)mouseMoved: (NSEvent *)event
{
	[self handleMouseMove: event];
}

- (void)mouseDragged: (NSEvent *)event
{
	if ([self useTextInput])
		if ([[self inputContext] handleEvent: event])
			return;
	
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
	MCMacPlatformHandleModifiersChanged(MCMacPlatformMapNSModifiersToModifiers([event modifierFlags]));
}

- (void)keyDown: (NSEvent *)event
{
	MCMacPlatformHandleModifiersChanged(MCMacPlatformMapNSModifiersToModifiers([event modifierFlags]));
	
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
	
	if ([self useTextInput])
	{
		//if ([[self inputContext] handleEvent: event])
		return;
	}
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
	// NSLog(@"insertText('%@', (%d, %d))", aString, replacementRange . location, replacementRange . length);
	
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
	
	/*if (replacementRange . location == NSNotFound)
	{
		NSRange t_marked_range;
		t_marked_range = [self markedRange];
		if (t_marked_range . location != NSNotFound)
			replacementRange = t_marked_range;
		else
			replacementRange = [self selectedRange];
	}
	
	int32_t si, ei;
	si = 0;
	ei = INT32_MAX;
	MCactivefield -> resolvechars(0, si, ei, replacementRange . location, replacementRange . length);
	
	NSString *t_string;
	if ([aString isKindOfClass: [NSAttributedString class]])
		t_string = [aString string];
	else
		t_string = aString;
	
	NSUInteger t_length;
	t_length = [t_string length];
	
	unichar *t_chars;
	t_chars = new unichar[t_length];
	
	[t_string getCharacters: t_chars range: NSMakeRange(0, t_length)];
	
	MCactivefield -> settextindex(0,
								  si,
								  ei,
								  MCString((char *)t_chars, t_length * 2),
								  True,
								  true);
    
	delete t_chars;
	
	[self unmarkText];
	[[self inputContext] invalidateCharacterCoordinates];*/
}

- (void)doCommandBySelector:(SEL)aSelector
{
	MCLog("doCommandBySelector:", 0);
	
	MCMacPlatformWindow *t_window;
	t_window = [self platformWindow];
	if (t_window == nil)
		return;
	
	// If the event came about as a result of a keyDown input method event
	// then process the key directly.
	if (m_input_method_event != nil)
	{
		[self handleKeyPress: m_input_method_event isDown: YES];
		[self handleKeyPress: m_input_method_event isDown: NO];
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
	// NSLog(@"setMarkedText('%@', (%d, %d), (%d, %d)", aString, newSelection . location, newSelection . length, replacementRange . location, replacementRange . length);
	
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
	
	/*if (replacementRange . location == NSNotFound)
	 {
	 NSRange t_marked_range;
	 t_marked_range = [self markedRange];
	 if (t_marked_range . location != NSNotFound)
	 replacementRange = t_marked_range;
	 else
	 replacementRange = [self selectedRange];
	 }
	 
	 int32_t si, ei;
	 si = ei = 0;
	 MCactivefield -> resolvechars(0, si, ei, replacementRange . location, replacementRange . length);
	 
	 NSString *t_string;
	 if ([aString isKindOfClass: [NSAttributedString class]])
	 t_string = [aString string];
	 else
	 t_string = aString;
	 
	 NSUInteger t_length;
	 t_length = [t_string length];
	 
	 unichar *t_chars;
	 t_chars = new unichar[t_length];
	 
	 [t_string getCharacters: t_chars range: NSMakeRange(0, t_length)];
	 
	 MCactivefield -> stopcomposition(True, False);
	 
	 if ([t_string length] == 0)
	 [self unmarkText];
	 else
	 {
	 MCactivefield -> startcomposition();
	 MCactivefield -> finsertnew(FT_IMEINSERT, MCString((char *)t_chars, t_length * 2), 0, true);
	 }
	 
	 MCactivefield -> seltext(replacementRange . location + newSelection . location,
	 replacementRange . location + newSelection . location + newSelection . length,
	 False);
	 
	 [[self inputContext] invalidateCharacterCoordinates];*/
}

- (void)unmarkText
{
	MCLog("unmarkText", 0);
	
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
	
/*	MCactivefield -> stopcomposition(False, False);
	[[self inputContext] discardMarkedText];*/
}

- (NSRange)selectedRange
{
	MCMacPlatformWindow *t_window;
	t_window = [self platformWindow];
	if (t_window == nil)
		return NSMakeRange(NSNotFound, 0);
	
	MCRange t_marked_range, t_selected_range;
	MCPlatformCallbackSendTextInputQueryTextRanges(t_window, t_marked_range, t_selected_range);
	MCLog("selectedRange() = (%d, %d)", t_selected_range . offset, t_selected_range . length);
	return NSMakeRange(t_selected_range . offset, t_selected_range . length);
	
/*	if (MCactivefield == nil)
		return NSMakeRange(NSNotFound, 0);
	
	int4 si, ei;
	MCactivefield -> selectedmark(False, si, ei, False, False);
	MCLog("selectedRange() = (%d, %d)", si, ei - si);
 //	return NSMakeRange(si, ei - si);*/
}

- (NSRange)markedRange
{
	MCMacPlatformWindow *t_window;
	t_window = [self platformWindow];
	if (t_window == nil)
		return NSMakeRange(NSNotFound, 0);
	
	MCRange t_marked_range, t_selected_range;
	MCPlatformCallbackSendTextInputQueryTextRanges(t_window, t_marked_range, t_selected_range);
	MCLog("markedRange() = (%d, %d)", t_marked_range . offset, t_marked_range . length);
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
	MCLog("hasMarkedText() = %d", t_marked_range . offset != UINDEX_MAX);
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
	
	// NSLog(@"attributedSubstringForProposedRange(%d, %d -> %d, %d) = '%@'", aRange . location, aRange . length, t_actual_range . offset, t_actual_range . length, t_attr_string);
	return t_attr_string;
}

- (NSArray*)validAttributesForMarkedText
{
	// MCLog("validAttributesForMarkedText() = []", nil);
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
	
	MCLog("firstRectForCharacterRange(%d, %d -> %d, %d) = [%d, %d, %d, %d]", aRange . location, aRange . length, t_actual_range . offset, t_actual_range . length, t_rect . x, t_rect . y, t_rect . width, t_rect . height);
	
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
	
	MCLog("characterIndexForPoint(%d, %d) = %d", t_location . x, t_location . y, t_index);
	
	return t_index;
}

//////////

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

#if 0
- (void)capitalizeWord:(id)sender;
- (void)changeCaseOfLetter:(id)sender;
- (void)deleteBackward:(id)sender;
- (void)deleteBackwardByDecomposingPreviousCharacter:(id)sender;
- (void)deleteForward:(id)sender;
- (void)deleteToBeginningOfLine:(id)sender;
- (void)deleteToBeginningOfParagraph:(id)sender;
- (void)deleteToEndOfLine:(id)sender;
- (void)delete
#endif

//////////

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

- shouldDelayWindowOrderingForEvent: (NSEvent *)event
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

- (NSDragOperation)dragImage:(NSImage *)image offset:(NSSize)offset allowing:(NSDragOperation)operations
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
				pasteboard: [NSPasteboard pasteboardWithName: NSDragPboard]
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
	MCPlatformPasteboardRef t_pasteboard;
	MCMacPlatformPasteboardCreate([sender draggingPasteboard], t_pasteboard);
	
	NSDragOperation t_ns_operation;
	
	MCMacPlatformWindow *t_window;
	t_window = [self platformWindow];
	if (t_window != nil)
	{
		MCPlatformDragOperation t_operation;
		t_window -> HandleDragEnter(t_pasteboard, t_operation);
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
	t_window = [(MCWindowDelegate *)[[self window] delegate] platformWindow];
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
	MCMacMapKeyCode([event keyCode], t_key_code);
	
	codepoint_t t_mapped_codepoint;
	if (!MCMacMapNSStringToCodepoint([event characters], t_mapped_codepoint))
		t_mapped_codepoint = 0xffffffffU;
	
	codepoint_t t_unmapped_codepoint;
	if (!MCMacMapNSStringToCodepoint([event charactersIgnoringModifiers], t_unmapped_codepoint))
		t_unmapped_codepoint = 0xffffffffU;

	// The unicode range 0xF700 - 0xF8FF is reserved by the system to indicate
	// keys which have no printable value, but represent an action (such as F11,
	// PageUp etc.). We don't want this mapping as we do it ourselves from the
	// keycode, so if the mapped codepoint is in this range we reset it.
	if (t_mapped_codepoint >= 0xf700 && t_mapped_codepoint < 0xf8ff)
		t_mapped_codepoint = t_unmapped_codepoint = 0xffffffffU;
	
	// Now, if we have an unmapped codepoint, but no mapped codepoint then we
	// propagate the unmapped codepoint as the mapped codepoint. This is so that
	// dead-keys (when input methods are turned off) propagate an appropriate
	// char (e.g. alt-e generates no mapped codepoint, but we want an e).
	if (t_mapped_codepoint == 0xffffffffU)
		t_mapped_codepoint = t_unmapped_codepoint;
	
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
	CGFloat t_height_diff = size.height - [self frame].size.height;
	
	[super setFrameSize: size];
	
	// IM-2014-03-28: [[ Bug 12046 ]] Adjust subviews upward to retain same height from top of view
	NSArray *t_subviews;
	t_subviews = [self subviews];
	for (uint32_t i = 0; i < [t_subviews count]; i++)
	{
		NSView *t_subview;
		t_subview = [t_subviews objectAtIndex:i];
		
		NSPoint t_origin;
		t_origin = [t_subview frame].origin;
		t_origin.y += t_height_diff;
		
		[t_subview setFrameOrigin:t_origin];
	}
	
	if ([self window] == nil)
		return;
	
	MCMacPlatformWindow *t_window;
	t_window = [self platformWindow];
	if (t_window == nil)
		return;
	
	t_window -> ProcessDidResize();
}

- (BOOL)autoresizesSubviews
{
	// IM-2014-03-28: [[ Bug 12046 ]] Prevent embedded content from being automatically resized with this view
	return NO;
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

- (MCPoint)mapNSPointToMCPoint: (NSPoint)p
{
	return MCPointMake(p . x, [self bounds] . size . height - p . y);
}

- (NSPoint)mapMCPointToNSPoint: (MCPoint)p
{
	return NSMakePoint(p . x, [self bounds] . size . height - p . y);
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
	m_has_sheet = false;
	
	m_parent = nil;
}

MCMacPlatformWindow::~MCMacPlatformWindow(void)
{
	if (m_is_visible)
		MCMacPlatformWindowHiding(this);

	[m_handle setDelegate: nil];
	[m_handle setContentView: nil];
	[m_handle close];
	[m_handle release];
	[m_view release];
	[m_delegate release];
}

////////////////////////////////////////////////////////////////////////////////

MCWindowView *MCMacPlatformWindow::GetView(void)
{
	return m_view;
}

id MCMacPlatformWindow::GetHandle(void)
{
	return m_handle;
}

void MCMacPlatformWindow::SetBackdropWindow(MCPlatformWindowRef p_window)
{
	if (m_window_handle == nil)
		return;
	
	if (p_window == this)
		return;
	
	// Any windows that float above everything don't need to be parented by the
	// backdrop window.
	if (m_style == kMCPlatformWindowStyleDialog ||
		m_style == kMCPlatformWindowStyleUtility ||
		m_style == kMCPlatformWindowStylePopUp ||
		m_style == kMCPlatformWindowStyleToolTip)
		return;
	
	
	MCMacPlatformWindow *t_backdrop;
	t_backdrop = (MCMacPlatformWindow *)p_window;
	if ([m_window_handle parentWindow] != nil)
		[[m_window_handle parentWindow] removeChildWindow: m_window_handle];
	
	if (t_backdrop != nil &&
		t_backdrop -> m_window_handle != nil)
	{
		NSInteger t_level;
		t_level = [m_window_handle level];
		[t_backdrop -> m_window_handle addChildWindow: m_window_handle ordered: NSWindowAbove];
		[m_window_handle setLevel: t_level];
	}
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

void MCMacPlatformWindow::ProcessWillMiniaturize(void)
{
	// Unset the parent window to make sure things don't propagate.
	if ([m_window_handle parentWindow] != nil)
		[[m_window_handle parentWindow] removeChildWindow: m_window_handle];
	
	MCMacPlatformWindowHiding(this);
}

void MCMacPlatformWindow::ProcessDidMiniaturize(void)
{
	HandleIconify();
}

void MCMacPlatformWindow::ProcessDidDeminiaturize(void)
{
	MCMacPlatformWindowShowing(this);
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

void MCMacPlatformWindow::ProcessKeyDown(MCPlatformKeyCode p_key_code, codepoint_t p_unmapped_char, codepoint_t p_mapped_char)
{
	HandleKeyDown(p_key_code, p_unmapped_char, p_mapped_char);
}

void MCMacPlatformWindow::ProcessKeyUp(MCPlatformKeyCode p_key_code, codepoint_t p_unmapped_char, codepoint_t p_mapped_char)
{
	HandleKeyUp(p_key_code, p_unmapped_char, p_mapped_char);
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
	if (t_window_level != kCGFloatingWindowLevel)
		m_window_handle = [[com_runrev_livecode_MCWindow alloc] initWithContentRect: t_cocoa_content styleMask: t_window_style backing: NSBackingStoreBuffered defer: YES];
	else
		m_panel_handle = [[com_runrev_livecode_MCPanel alloc] initWithContentRect: t_cocoa_content styleMask: t_window_style backing: NSBackingStoreBuffered defer: YES];
	
	m_delegate = [[com_runrev_livecode_MCWindowDelegate alloc] initWithPlatformWindow: this];
	[m_window_handle setDelegate: m_delegate];
	
	m_view = [[com_runrev_livecode_MCWindowView alloc] initWithFrame: NSZeroRect];
	[m_window_handle setContentView: m_view];
	
	[m_window_handle setLevel: t_window_level];
	[m_window_handle setOpaque: m_mask == nil];
	[m_window_handle setHasShadow: m_has_shadow];
	if (!m_has_zoom_widget)
		[[m_window_handle standardWindowButton: NSWindowZoomButton] setEnabled: NO];
	[m_window_handle setAlphaValue: m_opacity];
	[m_window_handle setDocumentEdited: m_has_modified_mark];
	[m_window_handle setReleasedWhenClosed: NO];
	
	[m_window_handle setCanBecomeKeyWindow: m_style != kMCPlatformWindowStylePopUp && m_style != kMCPlatformWindowStyleToolTip];
    
    // MW-2014-04-08: [[ Bug 12080 ]] Make sure we turn off automatic 'hiding on deactivate'.
    //   The engine handles this itself.
    [m_window_handle setHidesOnDeactivate: NO];
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
		[m_window_handle setOpaque: m_mask == nil];
		if (m_has_shadow)
			m_shadow_changed = true;
	}
	
	if (m_changes . content_changed)
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
		[m_window_handle setTitle: m_title != nil ? [NSString stringWithCString: m_title encoding: NSUTF8StringEncoding] : @""];
	
	if (m_changes . has_modified_mark_changed)
		[m_window_handle setDocumentEdited: m_has_modified_mark];
	
	if (m_changes . use_live_resizing_changed)
		;
	
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
			*(uint32_t *)r_value = m_window_handle != nil ? [m_window_handle windowNumber] : 0;
			return true;
			
		// IM-2014-03-26: [[ Bug 12021 ]] Return NSWindow frame rect
		case kMCPlatformWindowPropertyFrameRect:
			assert(p_type == kMCPlatformPropertyTypeRectangle);
			*(MCRectangle *)r_value = m_window_handle != nil ? MCRectangleFromNSRect([m_window_handle frame]) : MCRectangleMake(0, 0, 0, 0);
			return true;
	}
	return false;
}

void MCMacPlatformWindow::DoShow(void)
{
	if (m_style == kMCPlatformWindowStyleDialog)
		MCMacPlatformBeginModalSession(this);
	else
	{
		[m_view setNeedsDisplay: YES];
		[m_window_handle makeKeyAndOrderFront: nil];
		MCMacPlatformWindowShowing(this);
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
	else
	{
		// Unset the parent window to make sure things don't propagate.
		if ([m_window_handle parentWindow] != nil)
			[[m_window_handle parentWindow] removeChildWindow: m_window_handle];
	
		MCMacPlatformWindowHiding(this);
		[m_window_handle orderOut: nil];
	}
}

void MCMacPlatformWindow::DoFocus(void)
{
	[m_window_handle makeKeyWindow];
}

void MCMacPlatformWindow::DoRaise(void)
{
	MCMacPlatformWindowShowing(this);
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
	[m_view displayIfNeeded];
	
	// Re-enable screen updates if needed.
	if (m_shadow_changed)
		NSEnableScreenUpdates();
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

////////////////////////////////////////////////////////////////////////////////

static bool MCAlphaToCGImage(uindex_t p_width, uindex_t p_height, uint8_t* p_data, uindex_t p_stride, CGImageRef &r_image)
{
	bool t_success = true;
	
	CGImageRef t_image = nil;
	CGColorSpaceRef t_colorspace = nil;
	CFDataRef t_data = nil;
	CGDataProviderRef t_dp = nil;
	
	if (t_success)
		t_success = nil != (t_data = CFDataCreate(kCFAllocatorDefault, (uint8_t*)p_data, p_stride * p_height));
	
	if (t_success)
		t_success = nil != (t_dp = CGDataProviderCreateWithCFData(t_data));
	
	if (t_success)
		t_success = nil != (t_colorspace = CGColorSpaceCreateDeviceGray());
	
	if (t_success)
		t_success = nil != (t_image = CGImageCreate(p_width, p_height, 8, 8, p_stride, t_colorspace, kCGImageAlphaNone, t_dp, nil, false, kCGRenderingIntentDefault));
	
	CGColorSpaceRelease(t_colorspace);
	CGDataProviderRelease(t_dp);
	CFRelease(t_data);
	
	if (t_success)
		r_image = t_image;
	
	return t_success;
}

void MCPlatformWindowMaskCreate(int32_t p_width, int32_t p_height, int32_t p_stride, void *p_bits, MCPlatformWindowMaskRef& r_mask)
{
	CGImageRef t_mask;
	t_mask = nil;
	MCAlphaToCGImage(p_width, p_height, (uint8_t *)p_bits, p_stride, t_mask);
	r_mask = (MCPlatformWindowMaskRef)t_mask;
}

void MCPlatformWindowMaskRetain(MCPlatformWindowMaskRef p_mask)
{
	CGImageRef t_mask;
	t_mask = (CGImageRef)p_mask;
	CGImageRetain(t_mask);
}

void MCPlatformWindowMaskRelease(MCPlatformWindowMaskRef p_mask)
{
	CGImageRef t_mask;
	t_mask = (CGImageRef)p_mask;
	CGImageRelease(t_mask);
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
				if ((uint32_t)[t_view com_runrev_livecode_nativeViewId] == p_id)
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

MCPlatformKeyCode s_mac_keycode_map[] =
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

bool MCMacMapKeyCode(uint32_t p_mac_keycode, MCPlatformKeyCode& r_keycode)
{
	if (p_mac_keycode > 0x7f)
		return false;
	
	if (s_mac_keycode_map[p_mac_keycode] == kMCPlatformKeyCodeUndefined)
		return false;
	
	r_keycode = s_mac_keycode_map[p_mac_keycode];

	return true;
}

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

