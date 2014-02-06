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
	MCMacPlatformWindowFocusing(m_window);
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
	
	// Register for all dragging types (well ones that convert to 'data' anyway).
	// COCOA-TODO: Restrict this to the types we actually handle. When we support
	//   'custom' types, these will have to be declared by the app somehow so
	//   there will always be a finite list.
	[self registerForDraggedTypes: [NSArray arrayWithObject: (NSString *)kUTTypeData]];
	
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

// COCOA-TODO: This is probably not the best way to handle modifiers - these
//   should probably be passed through with the event to ensure synchronization.
//   Also that would solve the issue with things like alt-a producing a optionKeyDown
//   message when it should be keyDown. We can use flagsChanged to detect the modifier
//   key presses though (should we wish to propagate such!).
- (void)flagsChanged: (NSEvent *)event
{
	// The type of modifier key is determined by the cocoa defined flags:
	//   NSAlphaShiftKeyMask (CapsLock)
	//   NSShiftKeyMask
	//   NSControlKeyMask
	//   NSAlternateKeyMask
	//   NSCommandKeyMask
	//   NSNumericPadKeyMask
	//   NSHelpKeyMask
	//   NSFunctionKeyMask
	
	// Whether it is the left or right variant is determined by other bits (which
	// don't seem to have defines anywhere...):
	//   left-control = 1 << 0
	//   left-alt = 1 << 5
	//   left-command = 1 << 2
	//   right-command = 1 << 4
	//   right-alt = 1 << 6
	//   right-control = 1 << 13
	
	/*NSUInteger t_flags;
	t_flags = [event modifierFlags];
	
	MCPlatformKeyCode t_key_code;
	if ((t_flags & NSAlphaShiftKeyMask) != 0)
		t_key_code = kMCPlatformKeyCodeCapsLock;
	else if ((*/
	//NSLog(@"Flags = %08x, KeyCode = %04x", [event modifierFlags], [event keyCode]);
	
	// The keyCode of the event contains the key that causes the change,
	// where as the modifierFlags will indicate whether it is a down or up.
#if 0
	NSUInteger t_modifier_flag;
	MCPlatformKeyCode t_key_code;
	switch([event keyCode])
	{
		case kVK_Shift:
			t_key_code = kMCPlatformKeyCodeLeftShift;
			t_modifier_flag = NSShiftKeyMask;
			break;
		case kVK_RightShift:
			t_key_code = kMCPlatformKeyCodeRightShift;
			t_modifier_flag = NSShiftKeyMask;
			break;
		case kVK_Option:
			t_key_code = kMCPlatformKeyCodeLeftAlt;
			t_modifier_flag = NSAlternateKeyMask;
			break;
		case kVK_RightOption:
			t_key_code = kMCPlatformKeyCodeRightAlt;
			t_modifier_flag = NSAlternateKeyMask;
			break;
		case kVK_Control:
			t_key_code = kMCPlatformKeyCodeLeftControl;
			t_modifier_flag = NSControlKeyMask;
			break;
		case kVK_RightControl:
			t_key_code = kMCPlatformKeyCodeRightControl;
			t_modifier_flag = NSControlKeyMask;
			break;
		case kVK_Command:
			t_key_code = kMCPlatformKeyCodeLeftMeta;
			t_modifier_flag = NSCommandKeyMask;
			break;
		case 0x0036: /* kVK_RightCommand */
			t_key_code = kMCPlatformKeyCodeRightMeta;
			t_modifier_flag = NSCommandKeyMask;
			break;
		case kVK_Function: // COCOA-TODO
		case kVK_Help: // COCOA-TODO
		// ?? NSNumericPadMask ?? // COCOA-TODO
		default:
			// We don't recognise this modifier, so ignore it.
			return;
	}
#endif
	
	MCMacPlatformHandleModifiersChanged(MCMacPlatformMapNSModifiersToModifiers([event modifierFlags]));
	
#if 0
	// Send a key up or key down event depending on whether the flag is now
	// set.
	MCMacPlatformWindow *t_window;
	t_window = [(MCWindowDelegate *)[[self window] delegate] platformWindow];
	if (([event modifierFlags] & t_modifier_flag) != 0)
		t_window -> ProcessKeyDown(t_key_code, 0xffffffffU, 0xffffffffU);
	else
		t_window -> ProcessKeyUp(t_key_code, 0xffffffffU, 0xffffffffU);
#endif
}

- (void)keyDown: (NSEvent *)event
{
	[self handleKeyPress: event isDown: YES];
}

- (void)keyUp: (NSEvent *)event
{
	[self handleKeyPress: event isDown: NO];
}

- (void)scrollWheel: (NSEvent *)event
{
	MCMacPlatformWindow *t_window;
	t_window = [(MCWindowDelegate *)[[self window] delegate] platformWindow];
	t_window -> ProcessMouseScroll([event deltaX], [event deltaY]);
}

//////////

- shouldDelayWindowOrderingForEvent: (NSEvent *)event
{
	return NO;
}

- (NSDragOperation)draggingSourceOperationMaskForLocal: (BOOL)isLocal
{
	return NSDragOperationNone;
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
	NSLog(@"draggedImage:endedAt");
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
	t_image_loc . y = [t_mouse_event locationInWindow] . y + offset . height;
	
	NSLog(@"Drag image start");
	
	[self dragImage: image
				at: t_image_loc 
				offset: NSZeroSize 
				event: t_mouse_event
				pasteboard: [NSPasteboard pasteboardWithName: NSDragPboard]
				source: self
				slideBack: YES];
				
	NSLog(@"Drag image end");
				
	return NSDragOperationNone;
}

//////////

- (BOOL)wantsPeriodicDraggingUpdates
{
	// We only want updates when the mouse moves, or similar.
	return NO;
}

- (NSDragOperation)draggingEntered: (id<NSDraggingInfo>)sender
{
	NSLog(@"draggingEntered");
	
	MCPlatformPasteboardRef t_pasteboard;
	MCMacPlatformPasteboardCreate([sender draggingPasteboard], t_pasteboard);
	
	MCMacPlatformWindow *t_window;
	t_window = [(MCWindowDelegate *)[[self window] delegate] platformWindow];
	
	MCPlatformDragOperation t_operation;
	t_window -> HandleDragEnter(t_pasteboard, t_operation);
	
	return MCMacPlatformMapDragOperationToNSDragOperation(t_operation);
}

- (void)draggingExited: (id<NSDraggingInfo>)sender
{
	NSLog(@"draggingExited");
	
	MCMacPlatformWindow *t_window;
	t_window = [(MCWindowDelegate *)[[self window] delegate] platformWindow];
	t_window -> HandleDragLeave();
}

- (NSDragOperation)draggingUpdated: (id<NSDraggingInfo>)sender
{
	NSLog(@"draggingUpdated");
	
	MCPoint t_location;
	MCMacPlatformMapScreenNSPointToMCPoint([[self window] convertBaseToScreen: [sender draggingLocation]], t_location);
	
	MCMacPlatformWindow *t_window;
	t_window = [(MCWindowDelegate *)[[self window] delegate] platformWindow];
	
	MCPlatformMapPointFromScreenToWindow(t_window, t_location, t_location);
	
	// Update the modifier key state.
	MCMacPlatformHandleModifiersChanged(MCMacPlatformMapNSModifiersToModifiers([[NSApp currentEvent] modifierFlags]));
	
	MCPlatformDragOperation t_operation;
	t_window -> HandleDragMove(t_location, t_operation);
	
	return MCMacPlatformMapDragOperationToNSDragOperation(t_operation);
}

- (BOOL)performDragOperation: (id<NSDraggingInfo>)sender
{
	MCMacPlatformWindow *t_window;
	t_window = [(MCWindowDelegate *)[[self window] delegate] platformWindow];
	
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
	t_window = [(MCWindowDelegate *)[[self window] delegate] platformWindow];
	if (pressed)
		t_window -> ProcessKeyDown(t_key_code, t_mapped_codepoint, t_unmapped_codepoint);
	else
		t_window -> ProcessKeyUp(t_key_code, t_mapped_codepoint, t_unmapped_codepoint);
	
#if 0
	NSLog(@"Keycode = %04x", [event keyCode]);
	if ([[event characters] length] > 0)
		NSLog(@"Chars[0] = %04x (length = %d)", [[event characters] characterAtIndex: 0], [[event characters] length]);
	if ([[event charactersIgnoringModifiers] length] > 0)
		NSLog(@"CharsIgnoringMods[0] = %04x (length = %d)", [[event charactersIgnoringModifiers] characterAtIndex: 0], [[event charactersIgnoringModifiers] length]);
#endif
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

MCWindowView *MCMacPlatformWindow::GetView(void)
{
	return m_view;
}

void MCMacPlatformWindow::SetBackdropWindow(MCPlatformWindowRef p_window)
{
	if (m_window_handle == nil)
		return;
	
	if (p_window == this)
		return;
	
	// Any windows that float above everything don't need to be parented by the
	// backdrop window.
	if (m_style == kMCPlatformWindowStyleUtility ||
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
		m_window_handle = [[NSWindow alloc] initWithContentRect: t_cocoa_content styleMask: t_window_style backing: NSBackingStoreBuffered defer: YES];
	else
		m_panel_handle = [[NSPanel alloc] initWithContentRect: t_cocoa_content styleMask: t_window_style backing: NSBackingStoreBuffered defer: YES];
	
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
	
	// If this is a panel (floating window) then hide unrequired tool buttons.
	// COCOA-TODO: This doesn't work - indeed Apple HIG says windows must always have
	//   the three buttons, just disabled if not applicable.
#if 0
	if (t_window_level == kCGFloatingWindowLevel)
	{
		if (!m_has_close_widget)
			[[m_window_handle standardWindowButton: NSWindowCloseButton] setFrame: NSZeroRect];
		if (!m_has_collapse_widget)
			[[m_window_handle standardWindowButton: NSWindowMiniaturizeButton] setFrame: NSZeroRect];
		if (!m_has_zoom_widget)
			[[m_window_handle standardWindowButton: NSWindowZoomButton] setFrame: NSZeroRect];
	}
#endif
	
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
	}
	return false;
}

void MCMacPlatformWindow::DoShow(void)
{
	[m_window_handle makeKeyAndOrderFront: nil];
	MCMacPlatformWindowShowing(this);
}

void MCMacPlatformWindow::DoHide(void)
{
	// Unset the parent window to make sure things don't propagate.
	if ([m_window_handle parentWindow] != nil)
		[[m_window_handle parentWindow] removeChildWindow: m_window_handle];
	
	MCMacPlatformWindowHiding(this);
	[m_window_handle orderOut: nil];
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
	[m_view display];
	
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

void MCMacPlatformCreateWindow(MCPlatformWindowRef& r_window)
{
	r_window = new MCMacPlatformWindow;
}

////////////////////////////////////////////////////////////////////////////////

