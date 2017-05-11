/* Copyright (C) 2003-2016 LiveCode Ltd.

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
static id s_SimDeviceSet_class = nil;
static id s_SimRuntime_class = nil;

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

// MM-2014-09-30: [[ iOS 8 Support ]] Return the array of devices the simulator offers.
//   A device type must be choosen when launching the iOS 8 simulator.
- (id)getSimDeviceSet
{
    //NSLog(@"received getSimDeviceSet");
    if (s_SimDeviceSet_class == nil || s_SimRuntime_class == nil)
        return nil;

    // As of Xcode 8, the default device set is not necessarily the device set used by the current simulator.
    // This can happen if multiple versions of Xcode is installed.
    // So for Xcode 8 and later, make sure we fetch the device set used by the current service context.
    // As of Xcode 8.1, the defaultSet property of the DeviceSet class has been removed,
    // so use the new defaultDeviceSetWithError method of the SimRuntime class instead.
    if ([s_SimRuntime_class respondsToSelector: @selector(defaultDeviceSetWithError:)])
    {
        NSError* t_error;
        t_error = nil;
        id t_dev_set;
        t_dev_set = [s_SimRuntime_class defaultDeviceSetWithError: &t_error];
        if (t_error == nil && t_dev_set != nil)
            return [t_dev_set availableDevices];
    }
    else
    {
        id t_default_set;
        t_default_set = [s_SimDeviceSet_class defaultSet];
        id t_current_dev_set;
        t_current_dev_set = nil;
        if (t_default_set != nil && [s_SimDeviceSet_class respondsToSelector: @selector(setForSetPath: serviceContext:)])
            t_current_dev_set = [s_SimDeviceSet_class setForSetPath: [t_default_set setPath] serviceContext: s_SimRuntime_class];

        if (t_current_dev_set == nil)
            t_current_dev_set = t_default_set;
        if (t_current_dev_set != nil)
            return [t_current_dev_set availableDevices];
    }
    
    return nil;
}

// MM-2014-10-07: [[ Bug 13584 ]] Return the set of runtimes that the current sim supports.
- (id)getSimRuntimes
{
	//NSLog(@"received getSimRuntimes");
	if (s_SimRuntime_class != nil)
		return [s_SimRuntime_class supportedRuntimes];
	return nil;
}

// MM-2014-10-07: [[ Bug 13584 ]] Return the DTiPhoneSimulatorSystemRoot for the current SimRuntime.
- (id)getRootWithSimRuntime: (id)runtime
{
	//NSLog(@"received getRootWithSimRuntime");
	return [s_DTiPhoneSimulatorSystemRoot_class rootWithSimRuntime: runtime];
}

@end

////////////////////////////////////////////////////////////////////////////////

int getXcodeVersion(char *path_to_developer_folder)
{
    NSBundle *t_xcode_bundle = nil;
    NSString *t_xcode_path = [NSString stringWithFormat: @"%s/../../", path_to_developer_folder];
    
    t_xcode_bundle = [NSBundle bundleWithPath: t_xcode_path];
    
    NSString *t_xcode_version_string = [[t_xcode_bundle infoDictionary] objectForKey:@"CFBundleShortVersionString"];
    NSArray *t_xcode_version_array = [t_xcode_version_string componentsSeparatedByString:@"."];
    
    int t_xcode_version;
    t_xcode_version = [[t_xcode_version_array objectAtIndex:0] intValue] * 100;
    t_xcode_version += [[t_xcode_version_array objectAtIndex:1] intValue] * 10;
    
    if ([t_xcode_version_array count] == 3)
    {
        t_xcode_version += [[t_xcode_version_array objectAtIndex:2] intValue];
    }
    
    return t_xcode_version;
}

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
            
            int t_xcode_version = getXcodeVersion(argv[1]);
            
            // In Xcode 8.3 the Xcode.app/Contents/OtherFrameworks/DevToolsFoundation.framework is no longer present
            // In fact it is not needed anyway
            if (t_xcode_version < 830)
            {
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
            }
			
			if (t_success)
			{
				Class t_dvt_platform_class;
				t_dvt_platform_class = NSClassFromString(@"DVTPlatform");
				NSError* t_error;
				t_success = [t_dvt_platform_class loadAllPlatformsReturningError: &t_error];
				
				//NSLog(@"DVTPlatform %d", t_success);
			}
			
			// MM-2014-09-30: [[ iOS 8 Support ]] Load the new CoreSimulator bundle if found. Allows us to query the available devices.
			NSBundle *t_core_sim_bundle;
			if (t_success)
			{
				NSString *t_core_sim_path;
				t_core_sim_path = [NSString stringWithFormat: @"%s/Library/PrivateFrameworks/CoreSimulator.framework",  argv[1]];

				if ([[NSFileManager defaultManager] fileExistsAtPath: t_core_sim_path])
				{
					t_core_sim_bundle = [NSBundle bundleWithPath: t_core_sim_path];
					t_success = [t_core_sim_bundle load];
					
					//NSLog(@"CoreSimBundle %d", t_success);
				}
			}
			
			if (t_success)
			{
				// MM-2014-09-30: [[ iOS 8 Support ]] DVTiPhoneSimulatorRemoteClient has moved in Xcode 6.
				t_sim_bundle_path = [NSString stringWithFormat: @"%s/Platforms/iPhoneSimulator.platform/Developer/Library/PrivateFrameworks/DVTiPhoneSimulatorRemoteClient.framework",  argv[1]];
				if (![[NSFileManager defaultManager] fileExistsAtPath: t_sim_bundle_path])
					t_sim_bundle_path = [NSString stringWithFormat: @"%s/../SharedFrameworks/DVTiPhoneSimulatorRemoteClient.framework",  argv[1]];

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
		
		// MM-2014-09-30: [[ iOS 8 Support ]] Fetch the device set allowing us to query the available devices.
		s_SimDeviceSet_class = NSClassFromString(@"SimDeviceSet");
		
		// MM-2014-10-07: [[ Bug 13584 ]] Fetch the SimRuntime class, required to return the set of runtime environments the current sim supports.
		//   e.g. iOS7 sim, iOS 8 sim etc.
        // As of Xcode 8, the SimRuntime class no longer returns a valid set of supported runtimes.
        // Instead, we need to use the SimServiceContext class, making sure we fetch the correct context for the current version of Xcode.
        id t_service_context;
        t_service_context = NSClassFromString(@"SimServiceContext");
		// PM-2016-09-19: [[ Bug 18422] We want to use SimServiceContext class *only* in xcode 8
        if (t_service_context != nil && [t_service_context respondsToSelector:@selector(sharedServiceContextForDeveloperDir: error:)])
        {
            NSString *t_dev_dir;
            t_dev_dir = [NSString stringWithFormat: @"%s", argv[1]];
            NSError *t_error;
            t_error = nil;
            s_SimRuntime_class = [t_service_context sharedServiceContextForDeveloperDir: t_dev_dir error: t_error];
            
            if (t_error != nil)
                s_SimRuntime_class = nil;
        }
        else
            s_SimRuntime_class = NSClassFromString(@"SimRuntime");

		s_keep_running = YES;
        while((s_keep_running && 
               [[NSRunLoop currentRunLoop] runMode: NSDefaultRunLoopMode beforeDate: [NSDate distantFuture]]) ||
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
