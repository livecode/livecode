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

#include <pthread.h>
#include <external.h>
#include "DVTiPhoneSimulatorRemoteClient.h"
#include <objc/objc-runtime.h>
#include <CoreFoundation/CFPreferences.h>

////////////////////////////////////////////////////////////////////////////////

static NSTask *s_simulator_task = nil;
static NSDistantObject *s_simulator_proxy = nil;

////////////////////////////////////////////////////////////////////////////////

// These methods simplify the writing of values to Rev external API vars - they
// are basically sprintf for them.

bool VariableFormat(MCVariableRef var, const char *p_format, ...)
{
	va_list t_args;
	int t_count;
	va_start(t_args, p_format);
	t_count = vsnprintf(nil, 0, p_format, t_args);
	va_end(t_args);
	
	char *t_new_string;
	t_new_string = new char[t_count + 1];
	if (t_new_string == nil)
		return false;
	
	va_start(t_args, p_format);
	vsprintf(t_new_string, p_format, t_args);
	va_end(t_args);
	
	MCError t_error;
	t_error = MCVariableStore(var, kMCOptionAsCString, &t_new_string);
	
	delete[] t_new_string;
	
	return t_error == kMCErrorNone;
}

bool VariableAppendFormat(MCVariableRef var, const char *p_format, ...)
{
	va_list t_args;
	int t_count;
	va_start(t_args, p_format);
	t_count = vsnprintf(nil, 0, p_format, t_args);
	va_end(t_args);
	
	char *t_new_string;
	t_new_string = new char[t_count + 1];
	if (t_new_string == nil)
		return false;
	
	va_start(t_args, p_format);
	vsprintf(t_new_string, p_format, t_args);
	va_end(t_args);
	
	MCError t_error;
	t_error = MCVariableAppend(var, kMCOptionAsCString, &t_new_string);
	
	delete[] t_new_string;
	
	return t_error == kMCErrorNone;
}

////////////////////////////////////////////////////////////////////////////////

// These methods provide very simple error handling. When a message is thrown
// it is stored, and then Catch can be used to place it in the result variable.
// CheckError simply verifies that a Rev external API call succeeded or not,
// throwing an error if it didn't.

const char *g_error_message = nil;

bool Throw(const char *p_message)
{
	g_error_message = p_message;
	return false;
}

void Catch(MCVariableRef result)
{
	VariableFormat(result, "%s", g_error_message);
}

bool CheckError(MCError err)
{
	if (err == kMCErrorNone)
		return true;
	return Throw("error");
}

////////////////////////////////////////////////////////////////////////////////

// MM-2014-10-07: [[ Bug 13584 ]] The choosen sim runtime, does a similar job to s_simulator_system_root.
//  Ensures we launch the correct simulator version.
SimRuntime * s_simulator_runtime;

// the currently set simulator root, corresponding to a specific version of the
// iPhone SDK
DTiPhoneSimulatorSystemRoot *s_simulator_system_root = nil;

// reference to the launched iPhone simulator session (if any)
DTiPhoneSimulatorSession *s_simulator_session = nil;

// Syntax:
//    revIPhoneListSimulatorSDKs()
// Returns:
//    A list of available simulator SDK's - one per line. Each of these values
//    can be passed to revIPhoneLaunchInSimulator to choose a specific SDK to
//    use.
//
bool revIPhoneListSimulatorSDKs(MCVariableRef *argv, uint32_t argc, MCVariableRef result)
{
	bool t_success;
	t_success = true;
	
	if (t_success &&
		argc != 0)
		t_success = Throw("syntax error");
	
	if (t_success &&
		s_simulator_proxy == nil)
		t_success = Throw("no toolset");
	
	if (t_success)
	{
		// loop through SDKs available to simulator (see [iPhoneSimulator  showSDKs])
		NSArray *roots;
		roots = [s_simulator_proxy getKnownRoots];
		for (DTiPhoneSimulatorSystemRoot *root in roots)
		{
			t_success = VariableAppendFormat(result, "%s,%s,%s\n",
												[[root sdkDisplayName] cStringUsingEncoding: NSMacOSRomanStringEncoding],
												[[root sdkVersion] cStringUsingEncoding: NSMacOSRomanStringEncoding],
												[[root sdkRootPath] cStringUsingEncoding: NSMacOSRomanStringEncoding]);
			
			if (!t_success)
			{
				break;
			}
		}
	}
	
	if (!t_success)
		Catch(result);
	
	return t_success;
}

// MM-2014-10-07: [[ Bug 13584 ]] Also return the SimRuntime where applicable.
static bool fetch_named_simulator_root(const char *p_display_name, DTiPhoneSimulatorSystemRoot *&r_root, SimRuntime *&r_runtime)
{
	DTiPhoneSimulatorSystemRoot *t_root;
	t_root = nil;
	SimRuntime *t_runtime;
	t_runtime = nil;
	
	NSString *t_sdk_string;
	t_sdk_string = [NSString stringWithCString:p_display_name encoding: NSMacOSRomanStringEncoding];
	
	NSArray *t_runtimes;
	t_runtimes = [s_simulator_proxy getSimRuntimes];
	
	if (t_runtimes != nil)
	{
		for (SimRuntime *t_candidate in t_runtimes)
		{
			if ([[t_candidate name] caseInsensitiveCompare: t_sdk_string] == NSOrderedSame ||
				[[t_candidate identifier] caseInsensitiveCompare: t_sdk_string] == NSOrderedSame ||
				[[t_candidate root] caseInsensitiveCompare: t_sdk_string] == NSOrderedSame ||
				[[t_candidate versionString] caseInsensitiveCompare: t_sdk_string] == NSOrderedSame ||
                [[t_candidate versionString] hasPrefix: t_sdk_string])
			{
				t_runtime = t_candidate;
				t_root = [s_simulator_proxy getRootWithSimRuntime: t_runtime];
				break;
			}			
		}
	}
	else
	{	
		NSArray *t_knownroots;
		t_knownroots = [s_simulator_proxy getKnownRoots];
		for (DTiPhoneSimulatorSystemRoot *t_candidate in t_knownroots)
		{
			if ([[t_candidate sdkDisplayName] caseInsensitiveCompare: t_sdk_string] == NSOrderedSame ||
				[[t_candidate sdkVersion] caseInsensitiveCompare: t_sdk_string] == NSOrderedSame ||
				[[t_candidate sdkRootPath] caseInsensitiveCompare: t_sdk_string] == NSOrderedSame)
			{
				t_root = t_candidate;
				break;
			}
		}
	}
	
    // As of Xcode 8, DTiPhoneSimulatorSystemRoot is no longer used, so don't worry if we can't find one.
    // As long as we have a SimRuntime we can still launch apps.
	if (t_root != nil || t_runtime != nil)
	{
		r_root = t_root;
		r_runtime = t_runtime;
		return true;
	}	
	return false;
}

// Syntax:
//   revIPhoneSetSimulatorSDK <sdk>
// Action:
//   Choose which SDK to use for the next app launch. If <sdk> is empty, or a
//   specific SDK has not been requested, the 'default SDK' is used. (see
//   [DTiPhoneSimulatorRoot defaultRoot].
bool revIPhoneSetSimulatorSDK(MCVariableRef *argv, uint32_t argc, MCVariableRef result)
{
	bool t_success;
	t_success = true;
	
	if (t_success && argc > 1)
		t_success = Throw("syntax error");
	
	if (t_success &&
		s_simulator_proxy == nil)
		t_success = Throw("no toolset");
	
	DTiPhoneSimulatorSystemRoot * t_root;
	t_root = nil;
	SimRuntime *t_runtime;
	t_runtime = nil;
	if (t_success)
	{
		if (argc == 0)
			t_root = [s_simulator_proxy getDefaultRoot];
		else
		{
			char *t_sdk_cstring = nil;
			
			t_success = CheckError(MCVariableFetch(argv[0], kMCOptionAsCString, &t_sdk_cstring));
			if (t_success)
			{
				t_success = fetch_named_simulator_root(t_sdk_cstring, t_root, t_runtime);
				if (!t_success)
				{
					t_success = Throw("iPhone Simulator version not found");
				}
			}
		}

		// Check whether SDK is a valid sdk, and if so fetch the appropriate object
		// and store a reference in a global var.
	}
	
	if (t_success)
	{
		if (s_simulator_system_root != nil)
			[s_simulator_system_root release];
		if (t_root != nil)
			[t_root retain];
		s_simulator_system_root = t_root;
		if (t_runtime != nil)
			s_simulator_runtime = t_runtime;
	}
	if (!t_success)
		Catch(result);
	
	return t_success;
}

// This is just an example of how to use the new externals API to dispatch a message
// to an object.
static bool dispatch_message(MCObjectRef p_target, const char *p_message, const char *p_param)
{
	bool t_success;
	t_success = true;
	
	// Create a temporary var for the parameter
	MCVariableRef t_param;
	t_param = nil;
	if (t_success)
		t_success = CheckError(MCVariableCreate(&t_param));
	
	// Set the parameter
	if (t_success)
		t_success = CheckError(MCVariableStore(t_param, kMCOptionAsCString, &p_param));
	
	// Dispatch the message - note the last parameter is a return for the status (handled,
	// not handled, pass) which is ignored here.
	if (t_success)
		t_success = MCObjectDispatch(p_target, kMCDispatchTypeCommand, p_message, &t_param, 1, nil);

	// Free the var (it does not harm to release nil pointers)
	MCVariableRelease(t_param);
	
	return t_success;
}

static MCError MCRunOnMainThread_Fix(MCThreadCallback callback, void *state, MCRunOnMainThreadOptions options);

@interface revIPhoneLaunchDelegate : NSObject <DTiPhoneSimulatorSessionDelegate>
{
	MCObjectRef m_target;
	char *m_message;
	char *m_params;
	bool m_did_end;
	bool m_quiet;
}

- (MCObjectRef) getTarget;
- (const char *) getMessage;
- (const char *) getParams;
- (bool) getQuiet;

- (void) setTarget: (MCObjectRef) target;
- (void) setMessage: (const char*) message;
- (void) setParams: (const char*) params;
- (void) setQuiet: (bool) quiet;

- (void) setErrorMsg: (NSError *) error;

- (bool) didEnd;

@end

@implementation revIPhoneLaunchDelegate
- (id) init
{
	if (self = [super init])
	{
		m_message = nil;
		m_params = nil;
		m_did_end = false;
		m_quiet = false;
	}
	return self;
}

- (void) dealloc
{
	free(m_message);
	free(m_params);
	[super dealloc];
}

- (const char *) getMessage
{
	return m_message;
}

- (const char *) getParams
{
	return m_params;
}

- (bool) getQuiet
{
	return m_quiet;
}

- (void) setMessage: (const char *)p_message
{
	if (m_message != nil)
		free(m_message);
	if (p_message == nil)
		m_message = nil;
	else
		m_message = strdup(p_message);
}

- (void) setParams: (const char *)p_params
{
	if (m_params != nil)
		free(m_params);
	if (p_params == nil)
		m_params = nil;
	else
		m_params = strdup(p_params);
}

- (void) setErrorMsg: (NSError*) p_error
{
	NSString *t_errorstring = [[NSString stringWithCString: "error: " encoding: NSMacOSRomanStringEncoding] stringByAppendingString: [p_error localizedDescription]];
	[self setParams: [t_errorstring cStringUsingEncoding: NSMacOSRomanStringEncoding]];
}

- (void)setQuiet: (bool) p_value
{
	m_quiet = p_value;
}

- (MCObjectRef) getTarget
{
	return m_target;
}

- (void) setTarget: (MCObjectRef) p_target
{
	m_target = p_target;
}

- (bool) didEnd
{
	return m_did_end;
}

- (void) session: (DTiPhoneSimulatorSession *) p_session didEndWithError: (NSError *) p_error
{
	m_did_end = true;
	if (p_error != nil)
		[self setErrorMsg:p_error];
	else
		[self setParams: "stopped"];
	MCRunOnMainThread_Fix(nil, self, kMCRunOnMainThreadLater);
	
	// MM-2014-03-18: [[ iOS 7.1 Support ]] Interrupt anything waiting for the session to end.
	MCWaitBreak();
}

- (void) session: (DTiPhoneSimulatorSession *) p_session didStart: (BOOL) p_started withError: (NSError *) p_error
{
	if (p_error != nil)
	{
		m_did_end = true;
		[self setErrorMsg:p_error];
	}
	else
		[self setParams: "started"];
	MCRunOnMainThread_Fix(nil, self, kMCRunOnMainThreadLater);
}

@end

static void reviphone_callback(void *p_state)
{
	revIPhoneLaunchDelegate *t_delegate = (revIPhoneLaunchDelegate *)p_state;
	if (![t_delegate getQuiet])
		dispatch_message([t_delegate getTarget], [t_delegate getMessage], [t_delegate getParams]);
	if ([t_delegate didEnd])
	{
		//NSLog(@"delegate = %p", t_delegate);
		[t_delegate release];
		if (s_simulator_session != nil /*&& [s_simulator_session delegate] == t_delegate*/)
		{
			//NSLog(@"session = %p", s_simulator_session);
			[s_simulator_session release];
			s_simulator_session = nil;
		}
	}
}

static MCError MCRunOnMainThread_Fix(MCThreadCallback callback, void *state, MCRunOnMainThreadOptions options)
{
	return MCRunOnMainThread(reviphone_callback, state, options);
}

// Syntax:
//   revIPhoneLaunchInSimulator <path_to_app_bundle>, <family>, <callback>
// Action:
//   Launch the given app bundle in the simulator. The callback message should be
//   invoked to update the caller on status. It should be passed a single
//   parameter - either "started" or "stopped".
//   This method should throw an error is an app that has previously been launched
//   in the simulator has yet to terminate.
//
bool revIPhoneLaunchAppInSimulator(MCVariableRef *argv, uint32_t argc, MCVariableRef result)
{
	bool t_success;
	t_success = true;
	
	if (t_success && argc != 3)
		t_success = Throw("syntax error");
	
	if (t_success &&
		s_simulator_proxy == nil)
		t_success = Throw("no toolset");
	
	// check if there is a running simulator session
	if (s_simulator_session != nil)
		t_success = Throw("simulator already running");
	
	// Fetch the app path
	MCString t_app;
	if (t_success)
        // SN-2014-10-28: [[ Bug 13827 ]] Fetch the string as a UTF-8 encoded MCString
		t_success = CheckError(MCVariableFetch(argv[0], kMCOptionAsUTF8String, &t_app));
	
	// Fetch the family string
	const char *t_family;
	t_family = nil;
	if (t_success)
		t_success = CheckError(MCVariableFetch(argv[1], kMCOptionAsCString, &t_family));
	
	// Fetch the callback string
	const char *t_callback;
	t_callback = nil;
	if (t_success)
		t_success = CheckError(MCVariableFetch(argv[2], kMCOptionAsCString, &t_callback));
	
	// This fetches the object handle that should be used to send the callback
	// to.
	MCObjectRef t_target;
	t_target = nil;
	if (t_success)
		t_success = CheckError(MCContextMe(&t_target));
	
	if (t_success)
	{
		
		DTiPhoneSimulatorApplicationSpecifier *t_app_spec;
		DTiPhoneSimulatorSessionConfig *t_session_config;
		
		revIPhoneLaunchDelegate *t_delegate = [[revIPhoneLaunchDelegate alloc] init];
		[t_delegate setMessage: t_callback];
		[t_delegate setTarget: t_target];

        // SN-2014-10-27: [[ Bug 13827 ]] We need to pass the parameter as a UTF-8 string
		t_app_spec = [s_simulator_proxy specifierWithApplicationPath: [NSString stringWithUTF8String:t_app.buffer]];
		
		t_session_config = [s_simulator_proxy newSessionConfig];
        
		
		[t_session_config setApplicationToSimulateOnStart:t_app_spec];
		
        // As of Xcode 8, DTiPhoneSimulatorSystemRoot (s_simulator_system_root) is no longer used.
        if ([t_session_config respondsToSelector: @selector(setSimulatedSystemRoot:)])
        {
            if (s_simulator_system_root == nil)
            {
                s_simulator_system_root = [s_simulator_proxy getDefaultRoot];
                [s_simulator_system_root retain];
            }
            [t_session_config setSimulatedSystemRoot:s_simulator_system_root];
        }
		
		// MM-2014-10-07: [[ Bug 13584 ]] As well as setting the sys root, also set the sim runtime where applicable.
		//   Ensures we launch the correct version of the simulator.
		if (s_simulator_runtime != nil && [t_session_config respondsToSelector: @selector(setRuntime:)])
        {
			[t_session_config setRuntime: s_simulator_runtime];
        }
		[t_session_config setSimulatedApplicationShouldWaitForDebugger: NO];
		[t_session_config setSimulatedApplicationLaunchArgs: [NSArray array]];
		[t_session_config setSimulatedApplicationLaunchEnvironment: [NSDictionary dictionary]];
        
        // As of Xcode 8.3 the property NSString *localizedClientName is no longer available in DTiPhoneSimulatorSessionConfig class
        if ([t_session_config respondsToSelector: @selector(setLocalizedClientName:)])
            [t_session_config setLocalizedClientName: @"LiveCode"];
        
		bool t_is_ipad;
		t_is_ipad = strcasecmp(t_family, "ipad") == 0;
		
		if ([t_session_config respondsToSelector: @selector(setSimulatedDeviceFamily:)])
		{
			int32_t t_family_int;
			if (t_is_ipad)
				t_family_int = 2;
			else
				t_family_int = 1;
			[t_session_config setSimulatedDeviceFamily: [NSNumber numberWithInt: t_family_int]];
		}

		// MM-2014-09-30: [[ iOS 8 Support ]] For iOS 8, we must choose a device from the set the simulator offers
		//  in order to launch successfully.
		// MM-2014-10-07: [[ Bug 13584 ]] Make sure we choose the device which matches the sim runtime. This ensures
		//   we launch the correct device when the sim SDK has multiple sim versions installed.
		NSArray *t_devices;
		t_devices = [s_simulator_proxy getSimDeviceSet];
		if (t_devices != nil)
		{
			SimDevice *t_device;
			t_device = nil;
			
			bool t_found_device;
			t_found_device = false;
			
			// Choose the last run device if it is of the desired type.
			// Each device has a plist with a state entry. A state of 3 appears to suggest it's the last run.
			for (t_device in t_devices)
			{
				if (!((t_is_ipad && [[t_device name] hasPrefix: @"iPad"]) || (!t_is_ipad && [[t_device name] hasPrefix: @"iPhone"])))
					continue;

				NSString *t_dev_plist_path;
				t_dev_plist_path = [[t_device devicePath] stringByAppendingPathComponent: @"device.plist"];
				if ([[NSFileManager defaultManager] fileExistsAtPath: t_dev_plist_path])
				{					
					NSDictionary *t_dev_plist;
					t_dev_plist = [NSDictionary dictionaryWithContentsOfFile: t_dev_plist_path];
                    if (t_dev_plist != nil)
                    {
                        NSNumber *t_state;
                        t_state = [t_dev_plist objectForKey: @"state"];
                        // In Xcode 9+ you can run multiple simulator instances. This seems to have broken the check [t_device runtime] == s_simulator_runtime.
                        // So check for matching identifiers to detect if a simulator device is already up and running
                        bool t_maching_runtimes = [t_device runtime] == s_simulator_runtime;
                        bool t_maching_runtime_identifiers = [t_device.runtime.identifier compare: s_simulator_runtime.identifier] == NSOrderedSame;
                        if (t_state != nil && [t_state intValue] == 3 && (t_maching_runtimes || t_maching_runtime_identifiers))
                        {
                            t_found_device = true;
                            break;
                        }
                    }
				}				
			}
			
			// If the last run device is not suitable or not found, then just choose the first device in the list of the desired type.
			if (!t_found_device)
				for (t_device in t_devices)
					if (((t_is_ipad && [[t_device name] hasPrefix: @"iPad"]) || (!t_is_ipad && [[t_device name] hasPrefix: @"iPhone"]))
						&& [t_device runtime] == s_simulator_runtime)
					{
						t_found_device = true;
						break;
					}
			
			if (t_found_device)
				[t_session_config setDevice: t_device];
		}
		else if ([t_session_config respondsToSelector: @selector(setSimulatedDeviceInfoName:)])
		{
			// MM-2014-03-18: [[ iOS 7.1 Support ]] Device name is required by the DVT framework.
			
			// MW-2014-03-20: [[ Bug 11946 ]] Fetch the current device preference from the simulator
			//   and if it matches the requested family, then use that string for the device. This
			//   ensures the last set device in the simulator gets used rather than falling back to
			//   just iPad or iPhone.
			NSString *t_current;
			t_current = (NSString *)CFPreferencesCopyAppValue((CFStringRef)@"SimulateDevice", (CFStringRef)@"com.apple.iphonesimulator");			
            
			if (t_is_ipad)
            {
                if ([t_current hasPrefix: @"iPad"])
                    [t_session_config setSimulatedDeviceInfoName: t_current];
                else
                    [t_session_config setSimulatedDeviceInfoName: @"iPad"];
            }
			else
            {
                if ([t_current hasPrefix: @"iPhone"])
                    [t_session_config setSimulatedDeviceInfoName: t_current];
                else
                    [t_session_config setSimulatedDeviceInfoName: @"iPhone"];
            }
            [t_current release];
		}		
		
		s_simulator_session = [s_simulator_proxy newSession];
		[s_simulator_session setDelegate: t_delegate];
	
		// MM-2014-03-18: [[ iOS 7.1 Support ]] For the DVT framework, the application PID is an int rather than a NSNumber.
		//  Check to see which type the session accepts.
		if ([s_simulator_session respondsToSelector: @selector(setSimulatedApplicationPID:)])
		{
			const char *t_arg;
			t_arg = [[s_simulator_session methodSignatureForSelector: @selector(setSimulatedApplicationPID:)] getArgumentTypeAtIndex: 2];
			if (strcmp(t_arg, "i") == 0)
				[s_simulator_session setSimulatedApplicationPID: 35];
			else
				[s_simulator_session setSimulatedApplicationPID: [NSNumber numberWithInt: 35]];
		}
		
		// It seems that a session object that has been 'started' will auto-release itself on termination
		// thus, on success we retain the object.
		NSError *t_error;
		t_error = nil;
		if (![s_simulator_session requestStartWithConfig: t_session_config timeout: 30 error: &t_error])
		{
			t_success = Throw("could not start simulator");
			[t_delegate release];
			[s_simulator_session release];
			s_simulator_session = nil;
		}
		else
			[s_simulator_session retain];
		
		[t_session_config release];
		[t_app_spec release];
	}
	
	if (!t_success)
		Catch(result);
	
	return t_success;
}

// Syntax:
//    revIPhoneTerminateAppInSimulator
// Action:
//    Termiante the currently running app in the simulator that the external launched.
//
bool revIPhoneTerminateAppInSimulator(MCVariableRef *argv, uint32_t argc, MCVariableRef result)
{
	bool t_success;
	t_success = true;

	if (t_success && argc != 0)
		t_success = Throw("syntax error");
	
	if (t_success &&
		s_simulator_proxy == nil)
		t_success = Throw("no toolset");
	
	if (t_success)
	{
		// Terminate our current app instance
		if (s_simulator_session != nil)
		{
			[[s_simulator_session delegate] setQuiet: true];
			[s_simulator_session requestEndWithTimeout: 30];
			//[s_simulator_session release];
			//s_simulator_session = nil;
		}
	}
	
	if (!t_success)
		Catch(result);
	
	return t_success;
}

// Syntax:
//   revIPhoneAppIsRunning
// Action:
//   Returns 'true' if there is an active session, or false otherwise.
//
bool revIPhoneAppIsRunning(MCVariableRef *argv, uint32_t argc, MCVariableRef result)
{
	bool t_value;
	t_value = s_simulator_session != nil;
	MCVariableStore(result, kMCOptionAsBoolean, (void *)&t_value);
	return true;
		
}

// Syntax:
//   revIPhoneSetToolset <path>
// Action:
//   Set the external to use the given XCode toolset. The path specified must be
//   a valid toolset root - e.g. /Developer. If the appropriate framework cannot
//   be loaded "invalid toolset" is thrown.
bool revIPhoneSetToolset(MCVariableRef *argv, uint32_t argc, MCVariableRef result)
{
	bool t_success;
	t_success = true;
	
	// First stop/unload current things.
	if (s_simulator_session != nil)
	{
		[[s_simulator_session delegate] setQuiet: true];
		[s_simulator_session requestEndWithTimeout: 30];
		
		// MM-2014-03-18: [[ iOS 7.1 Support ]] Wait until the simulator session has ended to pervent any overlap.
		while(![[s_simulator_session delegate] didEnd])
			MCWaitRun();
		
		[s_simulator_session release];
		s_simulator_session = nil;
	}
	
	if (s_simulator_system_root != nil)
	{
		[s_simulator_system_root release];
		s_simulator_system_root = nil;
	}
	
	if (s_simulator_task != nil)
	{
		if (s_simulator_proxy != nil)
		{
			[s_simulator_proxy quit];
			[[s_simulator_proxy connectionForProxy] invalidate];
		}
		[s_simulator_task waitUntilExit];
		[s_simulator_task release];
		s_simulator_task = nil;
	}
	
	if (s_simulator_proxy != nil)
	{
		[s_simulator_proxy release];
		s_simulator_proxy = nil;
	}
	
	const char *t_toolset_cstring;
	t_toolset_cstring = nil;
	if (t_success)
		t_success = CheckError(MCVariableFetch(argv[0], kMCOptionAsCString, &t_toolset_cstring));
	
	NSString *t_path;
	t_path = nil;
	if (t_success)
	{
		// MM-2014-03-18: [[ iOS 7.1 Support ]] The proxy now chooses the appropriate simulator framework to use. Just pass the path to the dev tools
		t_path = [NSString stringWithCString: t_toolset_cstring encoding: NSASCIIStringEncoding];
		if (t_path == nil)
			t_success = false;
	}
		
	NSTask *t_task;
	NSString *t_id;
	t_id = nil;
	t_task = nil;
	if (t_success)
	{
		NSBundle *t_bundle;
		t_bundle = [NSBundle bundleForClass: objc_getClass("revIPhoneLaunchDelegate")];
		t_id = [NSString stringWithFormat: @"com.runrev.reviphoneproxy.%08x.%08lx", getpid(), clock()];
		t_task = [NSTask launchedTaskWithLaunchPath: [t_bundle pathForAuxiliaryExecutable: @"reviphoneproxy"] //[(NSURL *)t_url path]] //@"/Volumes/Macintosh HD/Users/mark/Workspace/revolution/trunk-mobile/_build/Debug/reviphoneproxy"
										  arguments: [NSArray arrayWithObjects: t_path, t_id, (NSString*)nil]];
		if (t_task == nil)
			t_success = Throw("unable to start proxy");
	}
	
	id t_proxy;
	t_proxy = nil;
	if (t_success)
	{
		while([t_task isRunning] && t_proxy == nil)
		{
			t_proxy = [NSConnection rootProxyForConnectionWithRegisteredName: t_id host: nil];
			if (t_proxy != nil)
				break;
			usleep(1000 * 100);
		}
		
		if (t_proxy == nil)
			t_success = Throw("unable to resolve proxy");
	}
	
	if (t_success)
	{
		[t_task retain];
		s_simulator_task = t_task;
		[t_proxy retain];
		s_simulator_proxy = t_proxy;
	}
	else
	{
		if (t_proxy != nil)
			[t_proxy quit];
		
		if (t_task != nil)
			[t_task terminate];
	}
		
	return t_success;
}

////////////////////////////////////////////////////////////////////////////////

bool revIPhoneStartup(void)
{
	return true;
}

void revIPhoneShutdown(void)
{
	if (s_simulator_system_root != nil)
	{
		[s_simulator_system_root release];
		s_simulator_system_root = nil;
	}
	
	if (s_simulator_proxy != nil)
	{
		@try
		{
			[s_simulator_proxy quit];
		}
		@catch(NSException *exception)
		{
		}
		
		[s_simulator_proxy release];
		s_simulator_proxy = nil;
	}
	
	if (s_simulator_task != nil)
	{
		[s_simulator_task waitUntilExit];
		[s_simulator_task release];
		s_simulator_task = nil;
	}
}

////////////////////////////////////////////////////////////////////////////////

MC_EXTERNAL_NAME("reviphone")
MC_EXTERNAL_STARTUP(revIPhoneStartup)
MC_EXTERNAL_SHUTDOWN(revIPhoneShutdown)
MC_EXTERNAL_HANDLERS_BEGIN
MC_EXTERNAL_COMMAND("revIPhoneSetToolset", revIPhoneSetToolset)
MC_EXTERNAL_FUNCTION("revIPhoneListSimulatorSDKs", revIPhoneListSimulatorSDKs)
MC_EXTERNAL_COMMAND("revIPhoneSetSimulatorSDK", revIPhoneSetSimulatorSDK)
MC_EXTERNAL_COMMAND("revIPhoneLaunchAppInSimulator", revIPhoneLaunchAppInSimulator)
MC_EXTERNAL_COMMAND("revIPhoneTerminateAppInSimulator", revIPhoneTerminateAppInSimulator)
MC_EXTERNAL_FUNCTION("revIPhoneAppIsRunning", revIPhoneAppIsRunning)
MC_EXTERNAL_HANDLERS_END
