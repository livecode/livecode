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

#if DOES_NOT_WORK

////////////////////////////////////////////////////////////////////////////////

// We run a separate thread which has an eventtap monitoring for key events.
// When a Cmd-. occurs, this global (volatile - to make sure its not optimized
// away!) is set to true. This can then be picked up in the GetAbortKeyPressed()
// method.
//
// This approach should mean that detecting for the abort-key has zero overhead.

static volatile bool s_abort_key_pressed = false;

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

@implementation com_runrev_livecode_MCAbortKeyThread

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
	t_event_tap_port = CGEventTapCreateForPSN(&t_psn, kCGHeadInsertEventTap, kCGEventTapOptionDefault, CGEventMaskBit(kCGEventKeyDown), abort_key_callback, nil);
	
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

////////////////////////////////////////////////////////////////////////////////

bool MCPlatformGetAbortKeyPressed(void)
{
	if (!s_abort_key_pressed)
		return false;
	
	s_abort_key_pressed = false;
	
	return true;
}

////////////////////////////////////////////////////////////////////////////////

static MCAbortKeyThread *s_abort_key_thread = nil;

bool MCPlatformInitializeAbortKey(void)
{
	// COCOA-TODO: If AX isn't enabled, then we can't use event taps. It does
	//   seem one can make a process trusted (after a restart at least), we could
	//   easily integrate this into the IDE.
	if (!AXAPIEnabled())
		return true;
	
	s_abort_key_thread = [[MCAbortKeyThread alloc] init];
	[s_abort_key_thread start];
	return true;
}

void MCPlatformFinalizeAbortKey(void)
{
	[s_abort_key_thread terminate];
	[s_abort_key_thread release];
	s_abort_key_thread = nil;
}

////////////////////////////////////////////////////////////////////////////////

#elif DOES_NOT_WORK

static CFAbsoluteTime s_last_time_abort_checked = 0.0;

bool MCPlatformGetAbortKeyPressed(void)
{
	CFAbsoluteTime t_current_time;
	t_current_time = CFAbsoluteTimeGetCurrent();
	if (t_current_time - s_last_time_abort_checked < 0.5)
		return false;
	
	s_last_time_abort_checked = t_current_time;
	
#ifdef 1
	
#else
	NSAutoreleasePool *t_pool;
	t_pool = [[NSAutoreleasePool alloc] init];
	
	NSEvent *t_event;
	t_event = [NSApp nextEventMatchingMask: NSKeyDownMask
								 untilDate: [NSDate distantPast]
									inMode: NSDefaultRunLoopMode
								   dequeue: NO];
	
	bool t_abort;
	t_abort = false;
	if (t_event != nil)
		if (([t_event modifierFlags] & NSCommandKeyMask) != 0)
			if ([[t_event characters] length] == 1)
				if ([[t_event characters] characterAtIndex: 0] == '.')
				{
					NSEvent *t_event;
					t_event = [NSApp nextEventMatchingMask: NSKeyDownMask
												 untilDate: [NSDate distantPast]
													inMode: NSDefaultRunLoopMode
												   dequeue: YES];
					t_abort = true;
				}
	
	[t_pool release];
#endif
	
	return t_abort;
}

bool MCPlatformInitializeAbortKey(void)
{
	s_last_time_abort_checked = CFAbsoluteTimeGetCurrent();
}

void MCPlatformFinalizeAbortKey(void)
{
}

#else

bool MCPlatformGetAbortKeyPressed(void)
{
	return false;
}

bool MCPlatformInitializeAbortKey(void)
{
	return true;
}

void MCPlatformFinalizeAbortKey(void)
{
}

#endif

