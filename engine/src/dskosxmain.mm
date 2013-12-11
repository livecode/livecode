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

#include "osxprefix.h"

#include "core.h"
#include "globdefs.h"
#include "filedefs.h"
#include "osspec.h"
#include "typedefs.h"
#include "parsedef.h"
#include "mode.h"

#include "execpt.h"
#include "scriptpt.h"
#include "mcerror.h"
#include "globals.h"
#include "util.h"
#include "eventqueue.h"

#include <unistd.h>
#include <Cocoa/Cocoa.h>

////////////////////////////////////////////////////////////////////////////////

bool X_init(int argc, char *argv[], char *envp[]);
void X_main_loop_iteration();
int X_close();

////////////////////////////////////////////////////////////////////////////////

void X_main_loop(void)
{
	while(!MCquit)
	{
		NSAutoreleasePool *t_pool;
		t_pool = [[NSAutoreleasePool alloc] init];
		X_main_loop_iteration();
		[t_pool release];
	}
}

////////////////////////////////////////////////////////////////////////////////

@interface com_runrev_livecode_MCAppDelegate: NSObject<NSApplicationDelegate>
{
	int m_argc;
	char **m_argv;
	char **m_envp;
}

- (void)applicationDidFinishLaunching: (NSNotification *)notification;

- (void)applicationWillTerminate: (NSNotification *)notification;

@end

@implementation com_runrev_livecode_MCAppDelegate

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

- (void)applicationDidFinishLaunching: (NSNotification *)notification
{
	if (!X_init(m_argc, m_argv, m_envp))
	{
		if (MCresult != NULL && MCresult -> getvalue() . is_string())
		{
			char *t_message;
			t_message = MCresult -> getvalue() . get_string() . clone();
			fprintf(stderr, "Startup error - %s\n", t_message);
		}
		exit(-1);
	}
	
	[self performSelector: @selector(runMainLoop) withObject: nil afterDelay: 0];
}

- (void)applicationWillTerminate: (NSNotification *)notification
{
	int t_exit_code = X_close();
	exit(t_exit_code);
}

- (NSApplicationTerminateReply)applicationShouldTerminate: (NSApplication *)sender
{
	// If we haven't already asked to quit, then post a quit request.
	if (!MCquit)
	{
		MCEventQueuePostQuitApp();
		return NSTerminateCancel;
	}
	
	return NSTerminateNow;
}

- (void)applicationWillBecomeActive:(NSNotification *)notification
{
	MCEventQueuePostResumeApp();
}

- (void)applicationDidResignActive:(NSNotification *)notification
{
	MCEventQueuePostSuspendApp();
}

- (void)runMainLoop
{
	X_main_loop();
	[[NSApplication sharedApplication] terminate: self];
}

@end

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
	
	// Setup our delegate
	com_runrev_livecode_MCAppDelegate *t_delegate;
	t_delegate = [[[com_runrev_livecode_MCAppDelegate alloc] initWithArgc: argc argv: argv envp: envp] autorelease];
	
	// Assign our delegate
	[t_application setDelegate: t_delegate];
	
	// Run the application - this never returns!
	[t_application run];
	
	// Drain the autorelease pool.
	[t_pool release];
	
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
