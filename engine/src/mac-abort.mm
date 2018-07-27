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

////////////////////////////////////////////////////////////////////////////////

// USE_EVENTTAP:
//
// We run a separate thread which has an eventtap monitoring for key events.
// When a Cmd-. occurs, this global (volatile - to make sure its not optimized
// away!) is set to true. This can then be picked up in the GetAbortKeyPressed()
// method.
//
// This approach should mean that detecting for the abort-key has zero overhead.
//
// Unfortunately this method is not ideal - it requires Accessibility to be turned
// on, and needs a poke at the event queue during normal script execution which
// can cause things like redraws to occur at inappropriate points (it seems at
// least!).
//
// NO USE_EVENTTAP:
//
// Again, a separate thread is used but instead it polls at a fixed interval to
// see if the '.' key and Cmd key are down. This should be no worse than the
// eventtap model in terms of responsiveness, it just means you have to hold down
// the Cmd-'.' key for a short while (just like on Windows).
//
// Of course, the only issue is we need to know what key maps to '.' (how annoying
// that people like different keyboard layouts ;)) but this can be done using
// UCKeyTranslate magic.
//
// Now if only Mac allowed you to attach a monitor to low-level event input
// stream on a secondary thread (i.e. where events are posted to the internal
// event queue)...

// WATCHDOG
//
// In previous engines, 'CheckEventQueueForUserCancel()' would be used periodically
// to check for abort. This prevented the SPOD from ever appearing when script was
// in tight loops as the event queue was being serviced. With Cocoa, we don't have
// this call anymore so the only thing we can do is try and tickle the event queue
// at specific points and hopefully this will not have any side-effects.
//
// To achieve this we use the same thread as the AbortKey and when that checks for
// keys being down it also sets the 's_abort_key_checked' flag to false. This can
// then be picked up on the main thread to cause a tickle.

static volatile bool s_abort_key_pressed = false;
static volatile bool s_abort_key_checked = false;

static unsigned int s_abort_key_disabled = 0;

////////////////////////////////////////////////////////////////////////////////

@interface com_runrev_livecode_MCAbortKeyThread: NSThread<NSPortDelegate>
{
	NSPort *m_termination_port;
	BOOL m_is_running;
}

- (void)main;
- (void)terminate;

- (void)handlePortMessage: (NSPortMessage *)message;

@end;

@compatibility_alias MCAbortKeyThread com_runrev_livecode_MCAbortKeyThread;

////////////////////////////////////////////////////////////////////////////////

#ifndef USE_EVENTTAP
static TISInputSourceRef s_current_input_source = nil;
static CGKeyCode s_current_period_keycode = 0xffff;
static bool s_current_period_needs_shift = false;

static unichar map_keycode_to_char(TISInputSourceRef p_input_source, CGKeyCode p_key_code, bool p_with_shift)
{
	CFDataRef t_layout_data;
	t_layout_data = (CFDataRef)TISGetInputSourceProperty(p_input_source, kTISPropertyUnicodeKeyLayoutData);
	
    // MW-2014-07-18: [[ Bug 12841 ]] If we don't have any layout data, then we can't map :(
    if (t_layout_data == nil)
        return 0;
    
	const UCKeyboardLayout *t_layout;
	t_layout = (const UCKeyboardLayout *)CFDataGetBytePtr(t_layout_data);
	
	UInt32 t_keys_down;
	UniChar t_chars[4];
	UniCharCount t_real_length;
	t_keys_down = 0;
	
	UCKeyTranslate(t_layout,
				   p_key_code,
				   kUCKeyActionDisplay,
				   p_with_shift ? (1 << 9) >> 8 /* shiftKey >> 8 */ : 0,
				   LMGetKbdType(),
				   kUCKeyTranslateNoDeadKeysBit,
				   &t_keys_down,
				   sizeof(t_chars) / sizeof(t_chars[0]),
				   &t_real_length,
				   t_chars);
	
	if (t_real_length == 0 || t_real_length > 1)
		return 0xffff;

	return t_chars[0];
}
#endif

#ifdef USE_EVENTTAP
static CGEventRef abort_key_callback(CGEventTapProxy p_proxy, CGEventType p_type, CGEventRef p_event, void *p_refcon)
{
	if ((CGEventGetFlags(p_event) & kCGEventFlagMaskCommand) == 0)
		return p_event;
	
	UniChar t_string[1];
	UniCharCount t_length;
	CGEventKeyboardGetUnicodeString(p_event, 1, &t_length, t_string);
	if (t_length != 1)
		return p_event;
	
	if (t_string[0] != '.')
		return p_event;
	
	s_abort_key_pressed = true;
	
	return p_event;
}
#endif

static void update_keyboard_input_source()
{
    // Update our period key mapping if the input source has changed / hasn't
    // been initialized.
    TISInputSourceRef t_input_source;
    t_input_source = TISCopyCurrentKeyboardInputSource();
    if (t_input_source != s_current_input_source || s_current_input_source == nil)
    {
        if (s_current_input_source != nil)
            CFRelease(s_current_input_source);
        
        s_current_input_source = t_input_source;
        s_current_period_keycode = 0xffff;
        s_current_period_needs_shift = false;
        
        // If we have a valid keyboard input source then resolve '.'.
        if (s_current_input_source != nil)
        {
            // Loop through all possible keycodes and map with no-shift and shift
            // to see if we can find our '.' key.
            for(uindex_t i = 0; i < 127; i++)
            {
                unichar t_char;
                t_char = map_keycode_to_char(t_input_source, i, false);
                if (t_char == '.')
                {
                    s_current_period_keycode = i;
                    s_current_period_needs_shift = false;
                    break;
                }
                
                t_char = map_keycode_to_char(t_input_source, i, true);
                if (t_char == '.')
                {
                    s_current_period_keycode = i;
                    s_current_period_needs_shift = true;
                    break;
                }
            }
        }
    }
}

void update_keyboard_input_source_callback(CFNotificationCenterRef p_center,
                                          void *p_observer,
                                          CFStringRef p_name,
                                          const void *p_object,
                                          CFDictionaryRef p_userInfo)
{
    update_keyboard_input_source();
}

static void abort_key_timer_callback(CFRunLoopTimerRef p_timer, void *p_info)
{
    if (s_abort_key_disabled > 0)
    {
        return;
    }

	s_abort_key_checked = false;
	
	// If we successfully found period - we must now check the state of the keys.
	if (s_current_period_keycode != 0xffff)
	{
		if (!CGEventSourceKeyState(kCGEventSourceStateCombinedSessionState, s_current_period_keycode))
			return;
		
		if (!CGEventSourceKeyState(kCGEventSourceStateCombinedSessionState, 0x37 /* LeftCommand */) &&
			!CGEventSourceKeyState(kCGEventSourceStateCombinedSessionState, 0x36 /* RightCommand */))
			return;

		bool t_has_shift;
		t_has_shift =
				CGEventSourceKeyState(kCGEventSourceStateCombinedSessionState, 0x38 /* LeftShift */) ||
				CGEventSourceKeyState(kCGEventSourceStateCombinedSessionState, 0x3C /* RightShift */);
		if (s_current_period_needs_shift &&
			!t_has_shift)
			return;
		if (!s_current_period_needs_shift &&
			t_has_shift)
			return;
		
		// If we get here then:
		//   the period keycode is down
		//   left command or right command is down
		//   if the period keycode needs shift, then left shift or right shift is down
		// i.e. We have detected Cmd-'.'
		//
		s_abort_key_pressed = true;
        
		MCPlatformBreakWait();
	}
}

@implementation com_runrev_livecode_MCAbortKeyThread

- (void)main
{
	NSRunLoop *t_loop;
	t_loop = [NSRunLoop currentRunLoop];
	
	NSDate *t_future;
	t_future = [NSDate distantFuture];
	
	m_termination_port = [[NSPort port] retain];
	[m_termination_port setDelegate: self];
	[t_loop addPort: m_termination_port forMode: NSDefaultRunLoopMode];
	
#ifdef USE_EVENTTAP
	ProcessSerialNumber t_psn;
	GetCurrentProcess(&t_psn);
	
	CFMachPortRef t_event_tap_port;
	t_event_tap_port = CGEventTapCreateForPSN(&t_psn, kCGHeadInsertEventTap, kCGEventTapOptionDefault, CGEventMaskBit(kCGEventKeyDown), abort_key_callback, nil);
	
	CFRunLoopSourceRef t_event_tap_source;
	t_event_tap_source = CFMachPortCreateRunLoopSource(NULL, t_event_tap_port, 0);
	CFRunLoopAddSource([t_loop getCFRunLoop], t_event_tap_source, kCFRunLoopDefaultMode);
#endif
	
	CFRunLoopTimerRef t_runloop_timer;
	t_runloop_timer = CFRunLoopTimerCreate(NULL, CFAbsoluteTimeGetCurrent(), 0.25, 0, 0, abort_key_timer_callback, nil);
	CFRunLoopAddTimer([t_loop getCFRunLoop], t_runloop_timer, kCFRunLoopDefaultMode);
	
	m_is_running = true;
	while(m_is_running &&
		  [t_loop runMode: NSDefaultRunLoopMode beforeDate: [NSDate distantFuture]])
		;

	CFRunLoopRemoveTimer([t_loop getCFRunLoop], t_runloop_timer, kCFRunLoopDefaultMode);
	CFRelease(t_runloop_timer);
	
#ifdef USE_EVENTTAP
	CFRunLoopRemoveSource([t_loop getCFRunLoop], t_event_tap_source, kCFRunLoopDefaultMode);
	CFRelease(t_event_tap_source);
	CFRelease(t_event_tap_port);
#endif
	
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

////////////////////////////////////////////////////////////////////////////////

void MCPlatformDisableAbortKey(void)
{
    s_abort_key_disabled += 1;
}

void MCPlatformEnableAbortKey(void)
{
    s_abort_key_disabled -= 1;
}

bool MCPlatformGetAbortKeyPressed(void)
{
    // MW-2014-04-23: [[ Bug 12163 ]] If the abortKey hasn't been checked
    //   recently, then tickle the event queue so that we suppress the SPOD.
	if (!s_abort_key_checked && MCMacPlatformIsEventCheckingEnabled())
	{
		NSDisableScreenUpdates();
		[NSApp nextEventMatchingMask: 0
						   untilDate: nil
							  inMode: NSEventTrackingRunLoopMode
							 dequeue: NO];
		NSEnableScreenUpdates();
		s_abort_key_checked = true;
	}
	
	if (!s_abort_key_pressed)
		return false;
	
	s_abort_key_pressed = false;
	
	return true;
}

////////////////////////////////////////////////////////////////////////////////

static MCAbortKeyThread *s_abort_key_thread = nil;

bool MCPlatformInitializeAbortKey(void)
{
#ifdef USE_EVENTTAP
	if (!AXAPIEnabled())
		return true;
#endif
    
    update_keyboard_input_source();
    
    CFNotificationCenterAddObserver(CFNotificationCenterGetDistributedCenter(),
                                nullptr,
                                update_keyboard_input_source_callback,
                                kTISNotifySelectedKeyboardInputSourceChanged,
                                nullptr,
                                CFNotificationSuspensionBehaviorDeliverImmediately);
	
	s_abort_key_thread = [[MCAbortKeyThread alloc] init];
	[s_abort_key_thread start];
	return true;
}

void MCPlatformFinalizeAbortKey(void)
{
    CFNotificationCenterRemoveObserver(CFNotificationCenterGetDistributedCenter(),
                                  nullptr,
                                  kTISNotifySelectedKeyboardInputSourceChanged,
                                  nullptr);
    
	[s_abort_key_thread terminate];
	[s_abort_key_thread release];
	s_abort_key_thread = nil;
}

////////////////////////////////////////////////////////////////////////////////

