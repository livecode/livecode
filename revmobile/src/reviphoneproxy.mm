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

#include <unistd.h>

////////////////////////////////////////////////////////////////////////////////

static pid_t s_parent_pid;
static BOOL s_keep_running;
static NSBundle *s_simulator_bundle;

static id s_DTiPhoneSimulatorSystemRoot_class = nil;
static id s_DTiPhoneSimulatorApplicationSpecifier_class = nil;
static id s_DTiPhoneSimulatorSessionConfig_class = nil;
static id s_DTiPhoneSimulatorSession_class = nil;

@interface RevIPhoneProxy: NSObject

- (id)init;

- (void)quit;

- (id)getKnownRoots;

@end

@implementation RevIPhoneProxy

- (id)init
{
	return [super init];
}

- (BOOL)isValid
{
	return
		s_simulator_bundle != nil &&
		s_DTiPhoneSimulatorSystemRoot_class != nil &&
		s_DTiPhoneSimulatorApplicationSpecifier_class != nil &&
		s_DTiPhoneSimulatorSessionConfig_class != nil &&
		s_DTiPhoneSimulatorSession_class != nil;
}

- (void)quit
{
	//NSLog(@"received quit");
	s_keep_running = NO;
}

- (id)getKnownRoots
{
	//NSLog(@"received getKnownRoots");
	return [s_DTiPhoneSimulatorSystemRoot_class knownRoots];
}

- (id)getDefaultRoot
{
	//NSLog(@"received getDefaultRoot");
	return [s_DTiPhoneSimulatorSystemRoot_class defaultRoot];
}

- (id)specifierWithApplicationPath: (NSString*)path
{
	//NSLog(@"received specified with application path");
	return [[s_DTiPhoneSimulatorApplicationSpecifier_class specifierWithApplicationPath: path] retain];
}

- (id)newSessionConfig
{
	//NSLog(@"received newSessionConfig");
	return [[s_DTiPhoneSimulatorSessionConfig_class alloc] init];
}

- (id)newSession
{
	//NSLog(@"received newSession");
	return [[s_DTiPhoneSimulatorSession_class alloc] init];
}

- (void)checkForParent
{
	if (getppid() != s_parent_pid)
		s_keep_running = NO;
	else
		[self performSelector: @selector(checkForParent) withObject: nil afterDelay: 0.1];
}

@end

////////////////////////////////////////////////////////////////////////////////

int main(int argc, char *argv[])
{
	bool t_success;
	t_success = true;
	
	s_parent_pid = getppid();
	
	NSAutoreleasePool *t_pool;
	t_pool = [[NSAutoreleasePool alloc] init];

	RevIPhoneProxy *t_proxy;
	t_proxy = [[RevIPhoneProxy alloc] init];
	[t_proxy checkForParent];
	
	//NSLog(@"REVIPHONEPROXY start: %s %s", argv[1], argv[2]);
	
	[[NSConnection defaultConnection] setRootObject: t_proxy];
	[[NSConnection defaultConnection] registerName: [NSString stringWithUTF8String: argv[2]]];
	
	// MM-2014-03-18: [[ iOS 7.1 Support ]] iPhoneSimulatorRemoteClient.framework is no longer present in Xcode 5.1.
	//  If the framework cannot be found, use DVTiPhoneSimulatorRemoteClient, which appears to have largley the same API.
	if (t_success)
	{	
		NSString *t_sim_bundle_path;
		t_sim_bundle_path = [NSString stringWithFormat: @"%s/Platforms/iPhoneSimulator.platform/Developer/Library/PrivateFrameworks/iPhoneSimulatorRemoteClient.framework", argv[1]];
		
		if ([[NSFileManager defaultManager] fileExistsAtPath: t_sim_bundle_path])
		{	
			t_sim_bundle_path = [NSString stringWithFormat: @"%s/Platforms/iPhoneSimulator.platform/Developer/Library/PrivateFrameworks/iPhoneSimulatorRemoteClient.framework", argv[1]];
			s_simulator_bundle = [NSBundle bundleWithPath: t_sim_bundle_path];
			t_success = [s_simulator_bundle load];
			
			//NSLog(@"Old Style SimBundle %d", t_success);
		}
		else
		{			
			
			// MM-2014-03-18: [[ iOS 7.1 Support ]] DVTiPhoneSimulatorRemoteClient.framework requires DVTFoundation and DevToolsFoundation. Load these first.
			NSBundle *t_dvt_foundation_bundle;
			t_dvt_foundation_bundle = nil;
			if (t_success)
			{	
				NSString *t_dvt_foundation_path;
				t_dvt_foundation_path = [NSString stringWithFormat: @"%s/../SharedFrameworks/DVTFoundation.framework",  argv[1]];
				
				if (![[NSFileManager defaultManager] fileExistsAtPath: t_dvt_foundation_path])
					t_dvt_foundation_path = [NSString stringWithFormat: @"%s/Library/PrivateFrameworks/DVTFoundation.framework",  argv[1]];
				
				t_dvt_foundation_bundle = [NSBundle bundleWithPath: t_dvt_foundation_path];
				t_success = [t_dvt_foundation_bundle load];		
				
				//NSLog(@"DVTFoundation %d", t_success);
			}
			
			NSBundle *t_dev_tools_bundle;
			t_dev_tools_bundle = nil;
			if (t_success)
			{
				NSString *t_dev_tools_path;
				t_dev_tools_path = [NSString stringWithFormat: @"%s/../OtherFrameworks/DevToolsFoundation.framework",  argv[1]];
				
				if (![[NSFileManager defaultManager] fileExistsAtPath: t_dev_tools_path])
					t_dev_tools_path = [NSString stringWithFormat: @"%s/Library/PrivateFrameworks/DevToolsFoundation.framework",  argv[1]];
				
				t_dev_tools_bundle = [NSBundle bundleWithPath: t_dev_tools_path];
				t_success = [t_dev_tools_bundle load];
								
				//NSLog(@"DevTools %d", t_success);
			}
			
			if (t_success)
			{
				Class t_dvt_platform_class;
				t_dvt_platform_class = NSClassFromString(@"DVTPlatform");
				NSError* t_error;
				t_success = [t_dvt_platform_class loadAllPlatformsReturningError: &t_error];
				
				//NSLog(@"DVTPlatform %d", t_success);
			}
			
			if (t_success)
			{
				t_sim_bundle_path = [NSString stringWithFormat: @"%s/Platforms/iPhoneSimulator.platform/Developer/Library/PrivateFrameworks/DVTiPhoneSimulatorRemoteClient.framework",  argv[1]];
				s_simulator_bundle = [NSBundle bundleWithPath: t_sim_bundle_path];
				t_success = [s_simulator_bundle load];
				
				//NSLog(@"SimBundle %d", t_success);
			}			
		}		 
	}
			
	if (t_success)
	{
		s_DTiPhoneSimulatorSystemRoot_class = [s_simulator_bundle classNamed: @"DTiPhoneSimulatorSystemRoot"];
		s_DTiPhoneSimulatorApplicationSpecifier_class = [s_simulator_bundle classNamed: @"DTiPhoneSimulatorApplicationSpecifier"];
		s_DTiPhoneSimulatorSessionConfig_class = [s_simulator_bundle classNamed: @"DTiPhoneSimulatorSessionConfig"];
		s_DTiPhoneSimulatorSession_class = [s_simulator_bundle classNamed: @"DTiPhoneSimulatorSession"];
		
		s_keep_running = YES;
		while(s_keep_running && 
			  [[NSRunLoop currentRunLoop] runMode: NSDefaultRunLoopMode beforeDate: [NSDate distantFuture]] ||
			  [NSConnection currentConversation] != nil)
			;
		
		// Just linger around a little longer to make sure our connection has closed
		// down cleanly.
		[[NSRunLoop currentRunLoop] runUntilDate: [NSDate dateWithTimeIntervalSinceNow: 0.25]];
		[NSObject cancelPreviousPerformRequestsWithTarget: t_proxy];
	}
		
	[t_proxy release];	
	[t_pool release];
	
	//NSLog(@"REVIPHONEPROXY exit %s", argv[1]);
	
	return 0;
}

////////////////////////////////////////////////////////////////////////////////
