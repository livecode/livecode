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

#include "osxprefix.h"

#include "core.h"
#include "globdefs.h"
#include "parsedef.h"
#include "filedefs.h"
#include "execpt.h"
#include "mcerror.h"
#include "util.h"
#include "objdefs.h"
#include "stack.h"
#include "stacklst.h"
#include "osspec.h"

#include "globals.h"
#include "dispatch.h"
#include "eventqueue.h"
#include "redraw.h"
#include "notify.h"
#include "scrolbar.h"
#include "mctheme.h"
#include "button.h"

#include "mode.h"

#include "osxdc.h"

#import <Cocoa/Cocoa.h>
#import <AppKit/NSEvent.h>

#include "osxdc-stack.h"

////////////////////////////////////////////////////////////////////////////////

CFAbsoluteTime MCScreenDC::s_animation_start_time = 0;
CFAbsoluteTime MCScreenDC::s_animation_current_time = 0;

////////////////////////////////////////////////////////////////////////////////

MCScreenDC::MCScreenDC(void)
{
	m_drag_send_data_upp = NULL;
	m_scrap_promise_keeper_upp = NULL;
	m_current_scrap = NULL;
	m_current_scrap_data = NULL;
	
	m_dst_profile = nil;
}

MCScreenDC::~MCScreenDC(void)
{
}

uint2 MCScreenDC::getdepth(void)
{
	return 32;
}

bool MCScreenDC::hasfeature(MCPlatformFeature p_feature)
{
	switch(p_feature)
	{
		case PLATFORM_FEATURE_WINDOW_TRANSPARENCY:
		case PLATFORM_FEATURE_OS_COLOR_DIALOGS:
		case PLATFORM_FEATURE_OS_FILE_DIALOGS:
		case PLATFORM_FEATURE_OS_PRINT_DIALOGS:
			return true;
			break;
			
		case PLATFORM_FEATURE_TRANSIENT_SELECTION:
			return false;
			break;
			
		default:
			assert(false);
			break;
	}
	
	return false;
}

const char *MCScreenDC::getdisplayname()
{
	return "local Mac";
}

uint2 MCScreenDC::getmaxpoints()
{//max points defined in a polygon on Mac quick Draw
	return 32767;
}

uint2 MCScreenDC::getvclass()
{
	return DirectColor;
}

void MCScreenDC::getvendorstring(MCExecPoint &ep)
{
	ep.setstaticcstring("Mac OS");
}

uint2 MCScreenDC::getpad()
{ //return the boundary each scan line is padded to.
	return 32;
}

Boolean MCScreenDC::open(void)
{
	invisibleWin = nil;
	
	black_pixel.red = black_pixel.green = black_pixel.blue = 0; //black pixel
	white_pixel.red = white_pixel.green = white_pixel.blue = 0xFFFF; //white pixel
	black_pixel.pixel = 0;
	white_pixel.pixel = 0xFFFFFF;
	
	MCzerocolor = MCbrushcolor = white_pixel;
	alloccolor(MCbrushcolor);
	MCselectioncolor = MCpencolor = black_pixel;
	alloccolor(MCselectioncolor);
	alloccolor(MCpencolor);
	gray_pixel.red = gray_pixel.green = gray_pixel.blue = 0x8888;
	alloccolor(gray_pixel);
	background_pixel.red = background_pixel.green = background_pixel.blue = 0xffff;
	alloccolor(background_pixel);
	
	// COCOA-TODO: Remove LMGetHiliteRGB
	RGBColor hiliteRGB;
	LMGetHiliteRGB(&hiliteRGB);
	MChilitecolor.red = hiliteRGB.red;
	MChilitecolor.green = hiliteRGB.green;
	MChilitecolor.blue = hiliteRGB.blue;
	alloccolor(MChilitecolor);	
	
	MCColor *syscolors = getaccentcolors();
	if (syscolors != NULL)
		MCaccentcolor = syscolors[4];
	else
	{
		MCaccentcolor.red = MCaccentcolor.green = 0x0000;
		MCaccentcolor.blue = 0x8080;
	}
	alloccolor(MCaccentcolor);
	
	// COCOA-TODO: Remove GetDblTime
	MCdoubletime = GetDblTime() * 1000 / 60;
	
	// Initialize templates
	MCtemplatescrollbar->alloccolors();
	if (IsMacEmulatedLF()) // no AM
		MCtemplatebutton->allocicons();
	
	MCcursors[PI_NONE] = nil;
	
	// COCOA-TODO: Remove GetCaretTime
	MCblinkrate = GetCaretTime() * 1000 / 60;
	
	MCDisplay const *t_displays;
	getdisplays(t_displays, false);
	MCwbr = t_displays[0] . workarea;
	
	s_animation_current_time = s_animation_start_time = CFAbsoluteTimeGetCurrent();
	
	// Initialize menus
	open_menus();
	
	// Initialize textinput
	open_textinput();
	
	// Initial drag-drop
	open_dragdrop();
	
	// Initialize iconmenu
	open_iconmenu();
	
	// Initialize color mapping
	open_colormapping();
}

Boolean MCScreenDC::close(Boolean force)
{
	close_colormapping();
	close_iconmenu();
	close_dragdrop();
	close_textinput();
	close_menus();
	
	// Sort out menubar (?)
	showmenu();
	
	// Sort out backdrop (?)
	finalisebackdrop();
	
	// Is this still needed?
	uint2 i;
	if (ncolors != 0)
	{
		int2 i;
		for (i = 0 ; i < ncolors ; i++)
		{
			if (colornames[i] != NULL)
				delete colornames[i];
		}
		delete colors;
		delete colornames;
		delete allocs;
	}	
}

////////////////////////////////////////////////////////////////////////////////

// This global will be set by a non-invasive thread if an abort key combination
// is detected.
static volatile bool MCabortdetected = false;

// COCOA-TODO: Setup non-invasive abortkey tracking.

Boolean MCScreenDC::abortkey()
{
	// If no abort was detected, do nothing.
	if (!MCabortdetected)
		return False;
	
	// Reset the detection flag to false.
	MCabortdetected = false;
	
	// If allowing interrupts and current stack does not have cantAbort
	// set, we should break.
	if (MCallowinterrupts && !MCdefaultstackptr -> cantabort())
		return True;
	
	// Set the interrupt flag.
	MCinterrupt = True;
	
	// We don't break at this point, as we've just set the flag.
	return False;
}

////////////////////////////////////////////////////////////////////////////////

static void nspoint_to_xy(NSPoint p_point, int2& r_x, int2& r_y)
{
	r_x = p_point . x;
	r_y = p_point . y;
}

void MCScreenDC::device_querymouse(int2 &x, int2 &y)
{
	nspoint_to_xy([NSEvent mouseLocation], x, y);
}

void MCScreenDC::device_setmouse(int2 x, int2 y)
{ //move mouse/cursor to new (x,y) location
	CGPoint point;
	point.x = x;
	point.y = y;
	CGWarpMouseCursorPosition(point);
}

// This operation is not tied to the event queue - we use the global event
// source state.
Boolean MCScreenDC::getmouse(uint2 button, Boolean& r_abort)
{	
	// COCOA-REVIEW: Is this delay necessary?
	//{
	// We only check for the mouse periodically - so wait until the
	// standard interval has elapsed.
	static real8 lasttime;
	real8 newtime = MCS_time();
	real8 sr = (real8)9.0 / 1000.0;
	if ((newtime - lasttime) < sr)
	{
		r_abort = MCscreen->wait(sr, False, True);
		if (r_abort)
			return False;
	}
	
	r_abort = False;
	lasttime = newtime;
	//}
	
	// If no button specified, take it to be button 1.
	if (button == 0)
		button = 1;
	
	// Map the button to a cg button.
	CGMouseButton t_cg_button;
	if (button == 1)
		t_cg_button = kCGMouseButtonLeft;
	else if (button == 2)
		t_cg_button = kCGMouseButtonCenter;
	else if (button == 3)
		t_cg_button = kCGMouseButtonRight;
	else
		t_cg_button = (CGMouseButton)(button - 1);
	
	return CGEventSourceButtonState(kCGEventSourceStateCombinedSessionState, t_cg_button);
}

// This operation is tied to the event queue.
Boolean MCScreenDC::getmouseclick(uint2 button, Boolean& r_abort)
{
	// Collect all pending events in the event queue.
	r_abort = wait(0.0, False, False);
	if (r_abort)
		return False;
	
	// Now get the eventqueue to filter for a mouse click.
	return MCEventQueueGetMouseClick(button);
}

////////////////////////////////////////////////////////////////////////////////

// This operation is not tied to the event queue - we use the global event
// source state.
void MCScreenDC::getkeysdown(MCExecPoint& ep)
{
	// COCOA-TODO: We should be able to use CGEventSourceKeyState here - although
	//   it might be somewhat inefficient.
	ep.clear();
}

// MW-2008-06-12: Updated to use more modern GetCurrentKeyModifiers function
//   to fetch the modifier state.
uint2 MCScreenDC::querymods()
{
	if (lockmods)
		return MCmodifierstate;
	
	uint2 state;
	state = 0;
	
#ifdef OLD_MAC
	UInt32 t_modifiers;
	t_modifiers = GetCurrentKeyModifiers();
	
	if ((t_modifiers & (1 << shiftKeyBit)) != 0)
		state |= MS_SHIFT;
	
	if ((t_modifiers & (1 << cmdKeyBit)) != 0)
		state |= MS_CONTROL;
	
	if ((t_modifiers & (1 << optionKeyBit)) != 0)
		state |= MS_MOD1;
	
	if ((t_modifiers & (1 << controlKeyBit)) != 0)
		state |= MS_MOD2;
	
	if ((t_modifiers & (1 << alphaLockBit)) != 0)
		state |= MS_CAPS_LOCK;
#endif
	
	// COCOA-TODO: Implement modifier fetching.
	
	return state;
}

////////////////////////////////////////////////////////////////////////////////

// This global is set by the eventqueue when a triple-click is detected.
Boolean tripleclick = False;

Boolean MCScreenDC::istripleclick()
{
	return tripleclick;
}

void MCMacBreakWait(void)
{	
	[NSApp postEvent:[NSEvent otherEventWithType:NSApplicationDefined
										location:NSMakePoint(0, 0)
								   modifierFlags:0
									   timestamp:0
									windowNumber:0
										 context:NULL
										 subtype:0
										   data1:0
										   data2:0]
			atStart:NO];
}

static EventRef *s_event_queue = nil;
static uindex_t s_event_queue_length = 0;

// This method waits for an event for at most p_max_wait seconds. If an event
// is collected in that time, it returns true.
bool MCScreenDC::collectevent(real8 p_max_wait, bool p_no_dispatch)
{
	/*if (p_no_dispatch)
	{
		EventTypeSpec t_event_types[] = { { 'FOOB', 'FOOB' } };
		OSStatus t_err;
		EventRef t_event;
		//t_err = ReceiveNextEvent(1, t_event_types, p_max_wait, False, &t_event);
		
		double t_end_time;
		t_end_time = MCS_time() + p_max_wait;
		for(;;)
		{
			double t_time_now;
			t_time_now = MCS_time();
			if (t_time_now >= t_end_time)
				break;
			t_err = ReceiveNextEvent(0, nil, t_end_time - t_time_now, TRUE, &t_event);
		
			DebugPrintEvent(t_event);
			ReleaseEvent(t_event);
		}
		
		return false;
	}*/
	
	NSAutoreleasePool *t_pool;
	t_pool = [[NSAutoreleasePool alloc] init];
	
	NSEvent *t_event;
	t_event = [NSApp nextEventMatchingMask: p_no_dispatch ? NSApplicationDefinedMask : NSAnyEventMask
								 untilDate: [NSDate dateWithTimeIntervalSinceNow: p_max_wait]
									inMode: p_no_dispatch ? NSEventTrackingRunLoopMode /*@"NoDispatchRunLoopMode"*/ : NSDefaultRunLoopMode
								   dequeue: YES];
	
	if (t_event != nil)
		[NSApp sendEvent: t_event];
	
	NSLog(@"Processed event");
	
	[t_pool release];

	return t_event != nil;

#if 0
	NSRunLoop *t_runloop;
	t_runloop = [NSRunLoop currentRunLoop];
	
	BOOL t_something_happened;
	t_something_happened = [t_runloop runMode: p_no_dispatch ? NSEventTrackingRunLoopMode : NSDefaultRunLoopMode beforeDate: [NSDate dateWithTimeIntervalSinceNow: p_max_wait]];
	
	NSLog(@"Processed event - %d", t_something_happened);
	
	return t_something_happened == YES;
#endif
	
/*	bool t_got_event;
	t_got_event = false;
	
	if (p_no_dispatch || s_event_queue_length == 0)
	{
		OSStatus t_err;
		EventRef t_event;
		t_event = nil;
		t_err = ReceiveNextEvent(0, nil, p_max_wait, TRUE, &t_event);
		if (t_event != nil)
		{
			MCMemoryResizeArray(s_event_queue_length + 1, s_event_queue, s_event_queue_length);
			s_event_queue[s_event_queue_length - 1] = t_event;
			t_got_event = true;
		}
	}
	
	if (!p_no_dispatch && s_event_queue_length > 0)
	{
		EventRef t_event;
		t_event = s_event_queue[0];
		MCMemoryMove(s_event_queue, s_event_queue + 1, sizeof(s_event_queue[0]) * (s_event_queue_length - 1));
		s_event_queue_length -= 1;
		
		NSEvent *t_ns_event;
		t_ns_event = [NSEvent eventWithEventRef: t_event];
		if (t_ns_event != nil)
			[NSApp sendEvent: t_ns_event];
		else
			SendEventToEventTarget(t_event, GetEventDispatcherTarget());
		
		t_got_event = true;
	}
	
	return t_got_event;
	
	/*NSAutoreleasePool *t_pool;
	t_pool = [[NSAutoreleasePool alloc] init];
	
	BOOL t_handled;
	t_handled = [[NSRunLoop currentRunLoop] runMode: p_no_dispatch ? @"NoDispatchRunLoopMode" : NSDefaultRunLoopMode beforeDate: [NSDate dateWithTimeIntervalSinceNow: p_max_wait]];
	
	[t_pool release];

	return t_handled == YES;*/
}

// Wait for 'duration' seconds, collecting events in that time. If 'dispatch'
// is True then messages will be dispatched as necessary. If 'anyevent' is true
// the wait will continue until the first system event is collected.
Boolean MCScreenDC::wait(real8 duration, Boolean dispatch, Boolean anyevent)
{
	real8 curtime = MCS_time();
	
	if (duration < 0.0)
		duration = 0.0;
	
	real8 exittime = curtime + duration;
	
	Boolean abort = False;
	Boolean reset = False;
	Boolean done = False;
	
	MCwaitdepth++;
	
	do
	{
		// Dispatch any notify events.
		if (MCNotifyDispatch(dispatch == True) && anyevent)
			break;
		
		// Handle pending events
		real8 eventtime = exittime;
		if (handlepending(curtime, eventtime, dispatch))
		{
			if (anyevent)
				done = True;
			
			if (MCquit)
			{
				abort = True;
				break;
			}
		}
		
		// If dispatching then dispatch the first event.
		if (dispatch && MCEventQueueDispatch())
		{
			if (anyevent)
				done = True;
			
			if (MCquit)
			{
				abort = True;
				break;
			}
		}
		
		// MW-2012-09-19: [[ Bug 10218 ]] Make sure we update the screen in case
		//   any engine event handling methods need us to.
		MCRedrawUpdateScreen();
		
		// Get the time now
		curtime = MCS_time();
		
		// And work out how long to sleep for.
		real8 t_sleep;
		t_sleep = 0.0;
		if (curtime >= exittime)
			done = True;
		else if (!done && eventtime > curtime)
			t_sleep = MCMin(eventtime - curtime, exittime - curtime);
		
		// Wait for t_sleep seconds and collect at most one event. If an event
		// is collected and anyevent is True, then we are done.
		if (collectevent(t_sleep, dispatch == False) && anyevent)
			done = True;
		
		// If 'quit' has been set then we must have got a finalization request
		if (MCquit)
		{
			abort = True;
			break;
		}
	}
	while(!done);
	
	MCwaitdepth--;
	
	// MW-2012-09-19: [[ Bug 10218 ]] Make sure we update the screen in case
	//   any engine event handling methods need us to.
	MCRedrawUpdateScreen();
	
	return abort;
}

void MCScreenDC::flushevents(uint2 e)
{
	// COCOA-TODO: Do something sensible.
}

////////////////////////////////////////////////////////////////////////////////

MCStack *MCScreenDC::device_getstackatpoint(int32_t x, int32_t y)
{
	// COCOA-TODO: Use NSApplication windows property to get window list and
	//   iterate through to find one under the cursor.
	return nil;
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
static Window s_mouse_window = nil;

// This is the current mask of buttons that are pressed.
static uint32_t s_mouse_buttons = 0;

// This is the window location in the mouse window that we last posted
// a position event for.
static MCPoint s_mouse_position = { INT16_MIN, INT16_MAX };

// This is the last screen location we received a mouse move message for.
static MCPoint s_mouse_screen_position;

void MCScreenDC::grabpointer(Window w)
{
	// If we are grabbing for the given window already, do nothing.
	if (s_mouse_grabbed && w == s_mouse_window)
		return;
	
	// If the mouse window is already w, then just grab.
	if (w == s_mouse_window)
	{
		s_mouse_grabbed = true;
		return;
	}
	
	// Otherwise do nothing - the mouse window must be w for us to grab.
	// (If we don't have this rule, then strange things could happen with
	//  mouse presses in different windows!).
}

void MCScreenDC::ungrabpointer(void)
{
	// If buttons are down, then ungrab will happen when they are released.
	if (s_mouse_buttons != 0)
		return;
	
	// Otherwise just turn off the grabbed flag.
	s_mouse_grabbed = false;
}

void MCScreenDC::event_modifierschanged(uint32_t modifiers)
{
}

void MCScreenDC::event_mousepress(uint32_t p_button, bool p_new_state)
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
	if (s_mouse_buttons == 0)
		s_mouse_grabbed = false;
	
	// Fetch the stack attached to the window.
	MCStack *t_mouse_stack;
	t_mouse_stack = event_windowtostack(s_mouse_window);
	
	// If we have no stack, do nothing more.
	if (t_mouse_stack == nil)
		return;
	
	// If mouse buttons are non-zero, then grab.
	if (s_mouse_buttons != 0)
		s_mouse_grabbed = true;
	
	// Determine the press state - this can be down, up or release. If
	// the new state is 'up', then we must dispatch a release message
	// if the mouse location is not within the window.
	MCMousePressState t_press_state;
	if (p_new_state)
		t_press_state = kMCMousePressStateDown;
	else
	{
		MCPoint t_global_pos;
		t_global_pos = event_localtoglobal(t_mouse_stack, s_mouse_position);
		
		Window t_new_mouse_window;
		t_new_mouse_window = event_getwindowatpoint(t_global_pos);
		
		if (t_new_mouse_window == s_mouse_window)
			t_press_state = kMCMousePressStateUp;
		else
			t_press_state = kMCMousePressStateRelease;
	}
	
	// Finally dispatch a mouse press event.
	/* UNCHECKED */ MCEventQueuePostMousePress(t_mouse_stack, 0, 0, t_press_state, p_button);
}

void MCScreenDC::event_mousemove(MCPoint p_screen_loc)
{
	// First compute the window that should be active now.
	Window t_new_mouse_window;
	if (s_mouse_grabbed)
	{
		// If the mouse is grabbed, the mouse window does not change.
		t_new_mouse_window = s_mouse_window;
	}
	else
	{
		// If the mouse is not grabbed, then we must determine which of our
		// window views we are now over.
		t_new_mouse_window = event_getwindowatpoint(p_screen_loc);
	}
	
	// Fetch the stack of the current mouse window.
	MCStack *t_mouse_stack;
	t_mouse_stack = event_windowtostack(s_mouse_window);
	
	// If the mouse window has changed, then we must exit/enter.
	if (t_new_mouse_window != s_mouse_window)
	{
		MCStack *t_new_mouse_stack;
		t_new_mouse_stack = event_windowtostack(t_new_mouse_window);
		
		// Leave the previous mouse stack.
		if (t_mouse_stack != nil)
			/* UNCHECKED */ MCEventQueuePostMouseFocus(t_mouse_stack, 0, false);
		
		// Enter the new mouse stack.
		if (t_new_mouse_stack != nil)
			/* UNCHECKED */ MCEventQueuePostMouseFocus(t_new_mouse_stack, 0, true);
	
		// Update the mouse window.
		s_mouse_window = t_new_mouse_window;
		t_mouse_stack = t_new_mouse_stack;
	}
	
	// If we have a new mouse window, then translate screen loc and update.
	if (t_mouse_stack != nil)
	{
		MCPoint t_window_loc;
		t_window_loc = event_globaltolocal(t_mouse_stack, p_screen_loc);
		
		if (t_window_loc . x != s_mouse_position . x ||
			t_window_loc . y != s_mouse_position . y)
		{
			/* UNCHECKED */ MCEventQueuePostMousePosition(t_mouse_stack, 0, 0, t_window_loc . x, t_window_loc . y);
			s_mouse_position = t_window_loc;
		}
	}
	
	// Regardless of whether we posted a mouse move, update the screen mouse position.
	s_mouse_screen_position = p_screen_loc;
}

MCStack *MCScreenDC::event_windowtostack(Window window)
{
	if (window == nil)
		return nil;
	
	NSView *t_view;
	t_view = [(NSWindow *)window contentView];
	
	return (MCStack *)[t_view stack];
}

Window MCScreenDC::event_stacktowindow(MCStack *stack)
{
	if (stack == nil)
		return nil;
	
	return stack -> getwindow();
}

Window MCScreenDC::event_getwindowatpoint(MCPoint location)
{
	NSPoint t_location;
	t_location = NSPointFromMCPointGlobal(location);
	
	NSInteger t_number;
	t_number = [NSWindow windowNumberAtPoint: NSPointFromMCPointGlobal(location) belowWindowWithWindowNumber: 0];
	
	NSWindow *t_window;
	t_window = [NSApp windowWithWindowNumber: t_number];
	
	if (t_window == nil)
		return nil;
	
	NSView *t_view;
	t_view = [t_window contentView];
	if ([t_view hitTest: [t_view convertPoint: [t_window convertScreenToBase: t_location] fromView: nil]] == t_view)
		return (Window)t_window;
	
	return nil;
}

MCPoint MCScreenDC::event_globaltolocal(MCStack *stack, MCPoint location)
{
	Window t_window;
	t_window = stack -> getwindow();
	if (t_window == nil)
		return location;
	
	NSView *t_view;
	t_view = [(NSWindow *)t_window contentView];
	
	return (MCPoint)[t_view globalToLocal: location];
}

MCPoint MCScreenDC::event_localtoglobal(MCStack *stack, MCPoint location)
{
	Window t_window;
	t_window = stack -> getwindow();
	if (t_window == nil)
		return location;
	
	NSView *t_view;
	t_view = [(NSWindow *)t_window contentView];
	
	return (MCPoint)[t_view localToGlobal: location];
}

/*void MCScreenDC::event_mousefocus(bool inside)
{
	// Update our state.
	s_mouse_inside = inside;
	
	// If we are not grabbed, we might need to generate a enter/leave
	// event. We do this by just re-processing a mouseMove.
	if (!s_mouse_grabbed)
		event_mousemove(s_mouse_position);
}*/

#if 0
// We track two aspects of mouse movements.
//
// We track the window the mouse is actually over and whether it is within the
// view.
//
// We track the window the mouse is currently grabbed by and whether it is within
// the view.

// The window that the mouse is physically over.
static Window s_mouse_window = nil;
// Whether the mouse is within the view inside the window that the mouse is
// physically over.
static bool s_mouse_window_within = false;
// The mouse pointer has been grabbed by the grab window.
static bool s_mouse_grabbed = false;
// The window that is currently receiving mouseMove messages.
static Window s_mouse_grabbed_window = nil;
// Whether the mouse is actually still within the window that is receiving
// mouseMove messages.
static bool s_mouse_grabbed_window_within = false;

// The window the mouse is physically over.
static Window s_mouse_window = nil;
// If true, the mouse is within the LiveCode view of the window.
static bool s_mouse_within = false;
// The location of the mouse in the co-ordinate system of the LiveCode view
// within s_mouse_window.
static MCGPoint s_mouse_location;

void MCScreenDC::event_modifierschanged(Window target, uint32_t timestamp, uint32_t modifiers)
{
}

void MCScreenDC::event_mousepress(Window target, uint32_t timestamp, uint32_t button, bool pressed)
{
}

// This method is invoked when a mouse moves within the target window. Whether the mouse is
// within the view in the window is determined by 'within'.
void MCScreenDC::event_mousemove(Window target, uint32_t timestamp, MCGPoint location, bool within)
{
	// If the mouse is grabbed, then we must update the state of the grabbed window
	if (s_mouse_grabbed)
	{
	}

	
	if (within)
	{
		// The mouse has moved to be within the visible region of the target window, and also
		// within the active area of the target window's view.
		
		// If the current window is not the target, then check to see if we were within it and
		// 'leave' (so we aren't in any window).
		if (s_mouse_window != target)
		{
			if (s_mouse_window != nil && s_mouse_within)
				post_mousefocus_to_window(s_mouse_window, timestamp, false);
			s_mouse_window = nil;
			s_mouse_within = false;
			s_mouse_location = MCGPointMake(FLT_MIN, FLT_MAX);
		}
		
		// If the current window is not the target, or if we were previously not within target
		// then 'enter' (so we are within the target).
		if (s_mouse_window != target ||
			!s_mouse_within)
		{
			post_mousefocus_to_window(s_mouse_window, timestamp, true);
			s_mouse_window = target;
			s_mouse_within = true;
			s_mouse_location = MCGPointMake(FLT_MIN, FLT_MAX);
		}
		
		// At this point the window the mouse is over is target, and we are marked as being
		// within it. So, if the mouse position has changed, post a mouse move.
		if (s_mouse_location . x != location . x ||
			s_mouse_location . y != location . y)
		{
			post_mousemove_to_window(s_mouse_window, timestamp, location);
			s_mouse_location = location;
		}
	}
	else
	{
		// The mouse has moved to be within the visible region of the target window, but has
		// not entered the active area of the target window's view.
		
		// If the current window is not the target, then just
		if (s_mouse_window != target)
	}

}
#endif

////////////////////////////////////////////////////////////////////////////////

// IM-2013-08-01: [[ ResIndependence ]] OSX implementation currently returns 1.0
MCGFloat MCResGetDeviceScale(void)
{
	return 1.0;
}

////////////////////////////////////////////////////////////////////////////////
