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

#include "prefix.h"

#include "parsedef.h"
#include "filedefs.h"
#include "globals.h"

#include <objc/runtime.h>
#include <objc/message.h>
#include <unistd.h>

#include "mbliphoneview.h"
#include "graphics_util.h"

#include "mblnotification.h"
#import <sys/utsname.h>

#include "libscript/script.h"

////////////////////////////////////////////////////////////////////////////////

#ifdef _DEBUG
#undef MCLog
#define MCLog(...) NSLog(@__VA_ARGS__)
#endif

////////////////////////////////////////////////////////////////////////////////

// MW-2014-09-22: [[ Bug 13446 ]] iOS8 sends notification related messages before
//   'didBecomeActive' so we queue these, and then post to the event queue after
//   the LiveCode side of the app has been initialized in didBecomeActive.

enum MCPendingNotificationEventType
{
    kMCPendingNotificationEventTypeDidReceiveLocalNotification,
    kMCPendingNotificationEventTypeDidReceiveRemoteNotification,
    kMCPendingNotificationEventTypeDidRegisterForRemoteNotification,
    kMCPendingNotificationEventTypeDidFailToRegisterForRemoteNotification,
};

struct MCPendingNotificationEvent
{
    MCPendingNotificationEvent *next;
    MCPendingNotificationEventType type;
    NSString *text;
};

static MCPendingNotificationEvent *s_notification_events = nil;

static void queue_notification_event(MCPendingNotificationEventType p_event_type, NSString *p_string)
{
    MCPendingNotificationEvent *t_event;
    t_event = new MCPendingNotificationEvent;
    t_event -> next = nil;
    t_event -> type = p_event_type;
    t_event -> text = [p_string retain];
    if (s_notification_events == nil)
        s_notification_events = t_event;
    else
        for(MCPendingNotificationEvent *t_last = s_notification_events; 1; t_last = t_last -> next)
            if (t_last -> next == nil)
            {
                t_last -> next = t_event;
                break;
            }
}

static void dispatch_notification_events(void)
{
    while(s_notification_events != nil)
    {
        MCPendingNotificationEvent *t_event;
        t_event = s_notification_events;
        s_notification_events = s_notification_events -> next;

        MCAutoStringRef t_text;
		// PM-2015-10-27: [[ Bug 16279 ]] Prevent crash when the payload is empty
		if (t_event -> text != nil)
			/* UNCHECKED */ MCStringCreateWithCFStringRef((CFStringRef)t_event -> text, &t_text);
		else
			t_text = MCValueRetain(kMCEmptyString);

        switch(t_event -> type)
        {
            case kMCPendingNotificationEventTypeDidReceiveLocalNotification:
                MCNotificationPostLocalNotificationEvent(*t_text);
                break;
            case kMCPendingNotificationEventTypeDidReceiveRemoteNotification:
                MCNotificationPostPushNotificationEvent(*t_text);
                break;
            case kMCPendingNotificationEventTypeDidRegisterForRemoteNotification:
                MCNotificationPostPushRegistered(*t_text);
                break;
            case kMCPendingNotificationEventTypeDidFailToRegisterForRemoteNotification:MCNotificationPostPushRegistrationError(*t_text);
                break;
        }
        
        [t_event -> text release];
        delete t_event;
    }
}

////////////////////////////////////////////////////////////////////////////////

static MCIPhoneApplication *s_application = nil;

////////////////////////////////////////////////////////////////////////////////

static UIDeviceOrientation s_pending_device_orientation = UIDeviceOrientationUnknown;

static UIDeviceOrientation patch_device_orientation(id self, SEL _cmd)
{
	return s_pending_device_orientation;
}

////////////////////////////////////////////////////////////////////////////////

@interface UIView (com_runrev_livecode_MCIPhoneUIViewUtilities)

- (UIView *)com_runrev_livecode_findFirstResponder;

- (BOOL)com_runrev_livecode_passMotionEvents;

@end

@implementation UIView (com_runrev_livecode_MCIPhoneUIViewUtilities)

- (UIView *)com_runrev_livecode_findFirstResponder
{
    if ([self isFirstResponder])        
        return self;     
	
    for (UIView *t_subview in [self subviews])
	{
        UIView *t_first_responder;
		t_first_responder = [t_subview com_runrev_livecode_findFirstResponder];
		
        if (t_first_responder != nil)
			return t_first_responder;
    }
	
    return nil;
}

- (BOOL)com_runrev_livecode_passMotionEvents
{
	if ([self superview] == nil)
		return NO;
	
	return [[self superview] com_runrev_livecode_passMotionEvents];
}

@end

////////////////////////////////////////////////////////////////////////////////

@implementation MCIPhoneApplication

//////////

+ (MCIPhoneApplication *)sharedApplication
{
	return s_application;
}

//////////

- (id)init
{
	MCLog("Application: init\n");

	// Initialize the superclass.
	self = [super init];
	if (self == nil)
		return nil;
	
	// Initialize the singleton ref.
	s_application = self;
	
	// Initialize everything to initial values.
	m_application = nil;
	m_window = nil;
	m_display_link = nil;
	m_startup_controller = nil;
	m_main_controller = nil;
	
	m_status_bar_style = UIStatusBarStyleDefault;
	m_status_bar_hidden = NO;
    m_status_bar_solid = NO;
	
	m_in_autorotation = false;
	m_prepare_status_pending = false;
	
	m_pending_orientation_notification = nil;
	m_pending_orientation = UIInterfaceOrientationPortrait;
	m_orientation_lock = 0;
	m_in_orientation_changed = false;
	
	m_keyboard_activation_pending = false;
    m_keyboard_is_visible = false;
    m_is_remote_notification = false;
	
    m_pending_push_notification = nil;
    m_pending_local_notification = nil;
    m_device_token = MCValueRetain(kMCEmptyString);
    m_launch_url = MCValueRetain(kMCEmptyString);
    m_pending_launch_url = false;
    m_did_become_active = false;
   
	// Get the user interface idiom.
	UIUserInterfaceIdiom t_idiom;
	t_idiom = [[UIDevice currentDevice] userInterfaceIdiom];
	
	// Get the info dictionary.
	NSDictionary *t_info_dict;
	t_info_dict = [[NSBundle mainBundle] infoDictionary];
	
	// Read the allowed orientations from the plist.
	NSArray *t_allowed_orientations;
	t_allowed_orientations = [t_info_dict objectForKey: @"UISupportedInterfaceOrientations"];
	if (t_allowed_orientations != nil)
	{
		m_allowed_orientations = 0;
		for(NSString *t_orientation_string in t_allowed_orientations)
		{
			UIInterfaceOrientation t_orientation;
			if ([t_orientation_string isEqualToString: @"UIInterfaceOrientationPortrait"])
				t_orientation = UIInterfaceOrientationPortrait;
			else if ([t_orientation_string isEqualToString: @"UIInterfaceOrientationPortraitUpsideDown"])
				t_orientation = UIInterfaceOrientationPortraitUpsideDown;
			else if ([t_orientation_string isEqualToString: @"UIInterfaceOrientationLandscapeLeft"])
				t_orientation = UIInterfaceOrientationLandscapeLeft;
			else if ([t_orientation_string isEqualToString: @"UIInterfaceOrientationLandscapeRight"])
				t_orientation = UIInterfaceOrientationLandscapeRight;
			else
				t_orientation = (UIInterfaceOrientation)0;
			m_allowed_orientations |= 1 << t_orientation;
		}
	}
	else
		m_allowed_orientations =
				(1 << UIInterfaceOrientationPortrait) | (1 << UIInterfaceOrientationLandscapeLeft) | 
				(1 << UIInterfaceOrientationLandscapeRight) | (1 << UIInterfaceOrientationPortraitUpsideDown);

	// We begin in 'startup' mode.
	m_status = kMCIPhoneApplicationStatusStartup;
    
    // MW-2014-10-02: [[ iOS 8 Support ]] We need this global initialized as early as
    //   possible.
    // Setup the value of the major OS version global.
	// PM-2016-09-08: [[ Bug 18327 ]] Take into account if x.y.z version of iOS has more than one digits in x,y,z
    NSArray *t_sys_version_array = [[UIDevice currentDevice].systemVersion componentsSeparatedByString:@"."];
    
    MCmajorosversion = [[t_sys_version_array objectAtIndex:0] intValue] * 100;
    MCmajorosversion += [[t_sys_version_array objectAtIndex:1] intValue] * 10;
    
    if ([t_sys_version_array count] == 3)
    {
        MCmajorosversion += [[t_sys_version_array objectAtIndex:2] intValue];
    }
    
    m_keyboard_display = kMCIPhoneKeyboardDisplayModeOver;
    m_keyboard_type = UIKeyboardTypeDefault;
    m_return_key_type = UIReturnKeyDefault;

	// We are done (successfully) so return ourselves.
	return self;
}

- (void)dealloc
{
	MCLog("Application: dealloc\n");
	
    // MM-2012-09-25: [[ Bug 10391 ]] Release any unprocessed notifications.
    [m_pending_local_notification release];
    [m_pending_push_notification release];
    
	// Release things we have created.
	[m_startup_controller release];
	[m_main_controller release];
	[m_window release];
	
	// Unhook ourselves from the notification center.
	[[NSNotificationCenter defaultCenter] removeObserver: self];
	
	// Tell the device we no longer want notifications.
	[[UIDevice currentDevice] endGeneratingDeviceOrientationNotifications];
	
	// Finalize the superclass.
	[super dealloc];
}

//////////

// MM-2012-09-25: [[ iOS 6.0 ]] Return the orientations the app supports.  Overrides the pList entry.
// MM-2012-10-05: [[ Bug 10434 ]] Return all orientations: Certain native pop over controls (e.g. camera picker) run in portait, causing problems in lansdscape only apps.
- (NSUInteger)application:(UIApplication *)application supportedInterfaceOrientationsForWindow:(UIWindow *)window
{
    return ((1 << UIInterfaceOrientationPortrait) | (1 << UIInterfaceOrientationLandscapeLeft) | (1 << UIInterfaceOrientationLandscapeRight) | (1 << UIInterfaceOrientationPortraitUpsideDown));
}

//- (void)applicationDidFinishLaunching:(UIApplication *)p_application
- (BOOL)application:(UIApplication *)p_application didFinishLaunchingWithOptions:(NSDictionary *)p_launchOptions
{
	MCLog("Application: didFinishLaunching\n");

	// Store the application pointer for easy access.
	m_application = p_application;
    
	// Fetch the status bar state.
	m_status_bar_style = [m_application statusBarStyle];
	m_status_bar_hidden = [m_application isStatusBarHidden];
    
    // PM-2015-02-17: [[ Bug 14482 ]] Check if "solid" status bar style is selected
    NSDictionary *t_dict;
    t_dict = [[NSBundle mainBundle] infoDictionary];
    NSNumber *t_status_bar_solid;
    t_status_bar_solid = [t_dict objectForKey:@"com_livecode_StatusBarSolid"];
    
    m_status_bar_solid = [t_status_bar_solid boolValue];
	
	// Initialize our window with the main screen's bounds.
	m_window = [[MCIPhoneWindow alloc] initWithFrame: [[UIScreen mainScreen] bounds]];
	
	// Create the initial view controller.
	UIStoryboard *sb = [UIStoryboard storyboardWithName:@"StartupScreen" bundle:nil];
	m_startup_controller = [[sb instantiateInitialViewController] retain];
	
	// Create the main view controller.
	m_main_controller = [[MCIPhoneMainViewController alloc] init];

    // Check if we have received a local notification payload.
	Class t_cls = NSClassFromString(@"UILocalNotification");
    if (t_cls)
    {
#ifdef __IPHONE_8_0
        // PM-2015-02-18: [[ Bug 14400 ]] Ask for permission for local notifications only if this option is selected in standalone application settings
        NSNumber *t_uses_local_notifications;
        t_uses_local_notifications = [t_dict objectForKey:@"com_livecode_UsesLocalNotifications"];
        if ([t_uses_local_notifications boolValue])
        {
            // PM-2014-11-14: [[ Bug 13927 ]] From iOS 8 we have to ask for user permission for local notifications
            if ([UIApplication instancesRespondToSelector:@selector(registerUserNotificationSettings:)])
            {
                UIUserNotificationSettings *t_local_settings;
                t_local_settings = [UIUserNotificationSettings settingsForTypes:UIUserNotificationTypeAlert|UIUserNotificationTypeBadge|UIUserNotificationTypeSound categories:nil];
                [[UIApplication sharedApplication] registerUserNotificationSettings:t_local_settings];
            }
        }
#endif
        UILocalNotification *t_notification = [p_launchOptions objectForKey: UIApplicationLaunchOptionsLocalNotificationKey];
        if (t_notification)
        {
            // We have received a local notification with payload. Store the value until we can send the data.
            // MM-2012-09-25: [[ Bug 10391 ]] Retain notification.
            m_pending_local_notification = [[t_notification.userInfo objectForKey:@"payload"] retain];
        }
    }

    // Check if we have received a remote notification payload.
    NSDictionary *t_push_notification = [p_launchOptions objectForKey: UIApplicationLaunchOptionsRemoteNotificationKey];
    // MM-2012-09-25: [[ Bug 10391 ]] Retain notification.
    if (t_push_notification)
	{
		// Prevent crash when launching the app from the notifications screen:
		// If the payload is empty, JSON null will be deserialized to NSNull. Unlike nil,
		// we cannot send (most) messages to NSNull
		id t_pending_push_notification_value = [t_push_notification objectForKey:@"payload"];
		if ([t_pending_push_notification_value isKindOfClass: [NSString class]])
			m_pending_push_notification = [t_pending_push_notification_value retain];
		else
			// t_pending_push_notification_value is NSNull, not NSString
			m_pending_push_notification = nil;
	}
	
    // Check if we have received a custom URL
    // This check is carried out at application launch
    NSURL *t_launch_url = [p_launchOptions objectForKey: UIApplicationLaunchOptionsURLKey];
    if (t_launch_url)
    {    
        MCAutoStringRef t_url_text;
		/* UNCHECKED */ MCStringCreateWithCFStringRef((CFStringRef)[t_launch_url absoluteString], &t_url_text);
        MCValueAssign(m_launch_url, *t_url_text);
		
        // HSC-2012-03-13 [[ Bug 10076 ]] Prevent Push Notification crashing when applicationDidBecomeActive is called multiple times
        m_pending_launch_url = true;
    }    
    
	// Attach the view controller to the window.
	[m_window setRootViewController: m_startup_controller];
	
	// Display the window.
	[m_window makeKeyAndVisible];
    
	// Get the info dictionary.
	NSDictionary *t_info_dict;
	t_info_dict = [[NSBundle mainBundle] infoDictionary];
    
    // Read the allowed notification types from the plist.
    NSArray *t_allowed_push_notifications_array;
    t_allowed_push_notifications_array = [t_info_dict objectForKey: @"CFSupportedRemoteNotificationTypes"];
   
    if (t_allowed_push_notifications_array != nil)
    {
        // MM-2014-09-30: [[ iOS 8 Support ]] Use new iOS 8 registration methods for push notifications.
        if (![[UIApplication sharedApplication] respondsToSelector :@selector(registerUserNotificationSettings:)])
        {
            UIRemoteNotificationType t_allowed_notifications = UIRemoteNotificationTypeNone;
            for (NSString *t_push_notification_string in t_allowed_push_notifications_array)
            {
                if ([t_push_notification_string isEqualToString: @"CFBadge"])
                    t_allowed_notifications = (UIRemoteNotificationType) (t_allowed_notifications | UIRemoteNotificationTypeBadge);
                else if ([t_push_notification_string isEqualToString: @"CFSound"])
                    t_allowed_notifications = (UIRemoteNotificationType) (t_allowed_notifications | UIRemoteNotificationTypeSound);
                else if ([t_push_notification_string isEqualToString: @"CFAlert"])
                    t_allowed_notifications = (UIRemoteNotificationType) (t_allowed_notifications | UIRemoteNotificationTypeAlert);
            }
            
            // IM-2012-02-13 don't try to register if there are no allowed push notification types
            if (t_allowed_notifications != UIRemoteNotificationTypeNone)
            {
                // Inform the device what kind of push notifications we can handle.
                
                // MW-2013-07-29: [[ Bug 10979 ]] Dynamically call the 'registerForRemoteNotificationTypes' to
                //   avoid app-store warnings.
                objc_msgSend([UIApplication sharedApplication], sel_getUid("registerForRemoteNotificationTypes:"), t_allowed_notifications);
            }
        }
#ifdef __IPHONE_8_0
        else
        {
            UIUserNotificationType t_allowed_notifications;
            t_allowed_notifications = UIUserNotificationTypeNone;
            for (NSString *t_push_notification_string in t_allowed_push_notifications_array)
            {
                if ([t_push_notification_string isEqualToString: @"CFBadge"])
                    t_allowed_notifications = (UIRemoteNotificationType) (t_allowed_notifications | UIUserNotificationTypeBadge);
                else if ([t_push_notification_string isEqualToString: @"CFSound"])
                    t_allowed_notifications = (UIRemoteNotificationType) (t_allowed_notifications | UIUserNotificationTypeSound);
                else if ([t_push_notification_string isEqualToString: @"CFAlert"])
                    t_allowed_notifications = (UIRemoteNotificationType) (t_allowed_notifications | UIUserNotificationTypeAlert);
            }
            if (t_allowed_notifications != UIUserNotificationTypeNone)
            {
                m_is_remote_notification = true;
                UIUserNotificationSettings *t_push_settings;
                t_push_settings = [UIUserNotificationSettings settingsForTypes: t_allowed_notifications categories:nil];
                [[UIApplication sharedApplication] registerUserNotificationSettings: t_push_settings];
            }
        }
#endif
    }

    // MM-2014-09-26: [[ iOS 8 Support ]] Move the registration for orientation updates to here from init. Was causing issues with iOS 8.
    // Tell the device we want orientation notifications.
    [[UIDevice currentDevice] beginGeneratingDeviceOrientationNotifications];
    
    // Register for device orientation change notifications.
    [[NSNotificationCenter defaultCenter] addObserver: self
                                             selector: @selector(orientationChanged:)
                                                 name: UIDeviceOrientationDidChangeNotification
                                               object: nil];
    
    return TRUE;
}

// MM-2014-09-30: [[ iOS 8 Support ]] Method called after successully registering (local or remote) notification settings.
//  Call registerForRemoteNotifications to finish off push notification registration process. (Will send didRegisterForRemoteNotificationsWithDeviceToken which will be handled as before.)
// PM-2014-11-14: [[ Bug 13927 ]] Call registerForRemoteNotifications only if didRegisterUserNotificationSettings was called after successully registering *remote* notification settings
#ifdef __IPHONE_8_0
- (void)application: (UIApplication *)p_application didRegisterUserNotificationSettings: (UIUserNotificationSettings *)p_notificationSettings
{
    if (m_is_remote_notification)
        [p_application registerForRemoteNotifications];
}
#endif

- (void)application:(UIApplication *)p_application didReceiveLocalNotification:(UILocalNotification *)p_notification
{
    MCLog("application:didReceiveLocalNotification:");
    NSString *t_reminder_text = [p_notification.userInfo objectForKey:@"payload"];
	
    // MW-2014-09-22: [[ Bug 13446 ]] Queue the event.
    queue_notification_event(kMCPendingNotificationEventTypeDidReceiveLocalNotification, t_reminder_text);
    
    // If we are already active, dispatch.
    if (m_did_become_active)
        dispatch_notification_events();
    
}

- (void)application:(UIApplication *)p_application didReceiveRemoteNotification:(NSDictionary *)p_dictionary
{
    MCLog("application:didReceiveRemoteNotification:");
	id t_reminder_text_value = [p_dictionary objectForKey:@"payload"];
	
	// Prevent crash when sending push notifications while the app is running:
	// If the payload is empty, JSON null will be deserialized to NSNull. Unlike nil,
	// we cannot send (most) messages to NSNull
	NSString *t_reminder_text;
	if ([t_reminder_text_value isKindOfClass: [NSString class]])
		t_reminder_text = t_reminder_text_value;
	else
		// t_reminder_text_value is NSNull, not NSString
		t_reminder_text = nil;
	
    // MW-2014-09-22: [[ Bug 13446 ]] Queue the event.
    queue_notification_event(kMCPendingNotificationEventTypeDidReceiveRemoteNotification, t_reminder_text);
    
    // If we are already active, dispatch.
    if (m_did_become_active)
        dispatch_notification_events();
}

- (void)application:(UIApplication*)p_application didRegisterForRemoteNotificationsWithDeviceToken:(NSData*)p_device_token
{
    NSMutableString *t_registration_text = nil;
    NSUInteger t_length = p_device_token.length;
    const unsigned char *t_bytes = (const unsigned char *)p_device_token.bytes;
    if (t_length != 0)
    {
        t_registration_text = [[NSMutableString alloc] init];
        // Desired format: 8 segments of 8 letters/digits each, separated by spaces,
        // bracketed by <>
        [t_registration_text appendString: @"<"];
        for(int i = 0; i < t_length; i++)
        {
            // %4 because each byte produces two chars
            if (i > 0 && i % 4 == 0)
            {
                [t_registration_text appendString: @" "];
            }
            [t_registration_text appendFormat: @"%02x", t_bytes[i]];
        }
        [t_registration_text appendString: @">"];
    }

    if (t_registration_text != nil)
    {
        MCAutoStringRef t_device_token;
		/* UNCHECKED */ MCStringCreateWithCFStringRef((CFStringRef)t_registration_text, &t_device_token);
        MCValueAssign(m_device_token, *t_device_token);
    
        // MW-2014-09-22: [[ Bug 13446 ]] Queue the event.
        queue_notification_event(kMCPendingNotificationEventTypeDidRegisterForRemoteNotification,t_registration_text);
    
        // If we are already active, dispatch.
        if (m_did_become_active)
            dispatch_notification_events();
        
        [t_registration_text release];
    }
}

- (void)application:(UIApplication*)p_application didFailToRegisterForRemoteNotificationsWithError:(NSError*)p_error
{
    NSString *t_error_text = [NSString stringWithFormat:@"%@", p_error];
    
    // MW-2014-09-22: [[ Bug 13446 ]] Queue the event.
    queue_notification_event(kMCPendingNotificationEventTypeDidFailToRegisterForRemoteNotification, t_error_text);
    
    // If we are already active, dispatch.
    if (m_did_become_active)
        dispatch_notification_events();
}

// Check if we have received a custom URL
// This handler is called at runtime, for example if the application tries to launch itself
- (BOOL)application:(UIApplication *)p_application openURL:(NSURL*)p_url sourceApplication:(NSString *)p_source_application annotation:(id)p_annotation
{
    BOOL t_result = NO;
    if (p_url != nil)
    {
        MCAutoStringRef t_url_text;
		/* UNCHECKED */ MCStringCreateWithCFStringRef((CFStringRef)[p_url absoluteString], &t_url_text);
		MCValueAssign(m_launch_url, *t_url_text);
        if (m_did_become_active)
            MCNotificationPostUrlWakeUp(m_launch_url);
        t_result = YES;
    }
    return t_result;
}

- (void)applicationDidBecomeActive:(UIApplication *)p_application
{
	MCLog("Application: didBecomeActive\n");
	
	// Invoke the hook on the engine side.
	// MW-2012-08-06: [[ Fibers ]] Make sure we don't run this code twice.
	if (m_did_become_active)
		return;
	
	MCIPhoneHandleDidBecomeActive();
    	
    // Custom URLs can arrive before we are active. Need to know this so we don't send a message too early.
    m_did_become_active = true;
    
    // Queue the pending notification events (if any)
    dispatch_notification_events();
}

- (void)applicationWillResignActive:(UIApplication *)application
{
	MCLog("Application: willResignActive\n");
}

- (void)applicationWillTerminate:(UIApplication *)application
{
	MCLog("Application: willTerminate\n");
	
	// Invoke the hook on the engine side.
	MCIPhoneHandleWillTerminate();
}

- (void)applicationDidReceiveMemoryWarning:(UIApplication *)application
{
	MCLog("Application: didReceiveMemoryWarning\n");
	
	// Invoke the hook on the engine side.
	MCIPhoneHandleDidReceiveMemoryWarning();
}

void MCiOSFilePostProtectedDataAvailableEvent();
void MCiOSFilePostProtectedDataUnavailableEvent();

- (void)applicationProtectedDataWillBecomeUnavailable:(UIApplication *)application
{
    MCiOSFilePostProtectedDataUnavailableEvent();
}

- (void)applicationProtectedDataDidBecomeAvailable:(UIApplication *)application
{
    MCiOSFilePostProtectedDataAvailableEvent();
}

//////////

- (MCIPhoneApplicationStatus)status
{
	return m_status;
}

- (void)startPreparing
{	
	MCLog("Application: startPreparing\n");

	if (m_in_autorotation)
	{
		m_prepare_status_pending = true;
		return;
	}
	
	// MW-2012-08-31: [[ Bug 10340 ]] Post any pending events to the queue for
	//   notifications.
    if (m_pending_local_notification != nil)
    {
        MCAutoStringRef t_mc_reminder_text;
		/* UNCHECKED */ MCStringCreateWithCFStringRef((CFStringRef)m_pending_local_notification, &t_mc_reminder_text);
		MCNotificationPostLocalNotificationEvent(*t_mc_reminder_text);
		
		// HSC-2012-03-13 [[ Bug 10076 ]] Prevent Push Notification crashing when applicationDidBecomeActive is called multiple times
        [m_pending_local_notification release];
        m_pending_local_notification = nil;
    }
    if (m_pending_push_notification != nil)
    {
        MCAutoStringRef t_mc_reminder_text;
		/* UNCHECKED */ MCStringCreateWithCFStringRef((CFStringRef)m_pending_push_notification, &t_mc_reminder_text);
        MCNotificationPostPushNotificationEvent(*t_mc_reminder_text);
       
		// HSC-2012-03-13 [[ Bug 10076 ]] Prevent Push Notification crashing when applicationDidBecomeActive is called multiple times
        [m_pending_push_notification release];
        m_pending_push_notification = nil;
    }
    if (m_pending_launch_url == true && !MCStringIsEmpty(m_launch_url))
    {
        MCNotificationPostUrlWakeUp(m_launch_url);
        // HSC-2012-03-13 [[ Bug 10076 ]] Prevent Push Notification crashing when applicationDidBecomeActive is called multiple times
        m_pending_launch_url = false;
    }
	
	// Change state to prepare.
	m_status = kMCIPhoneApplicationStatusPrepare;
	
	// Set the orientation of the main controller to that of the startup controller.
	[m_main_controller setPendingOrientation: [m_startup_controller interfaceOrientation]];
	
	// Invoke the hook on the engine side.
	MCIPhoneHandleDidStartPreparing();
}

- (void)startExecuting
{	
	MCLog("Application: startExecuting\n");
	
	// We are no longer in the startup phase.
	m_status = kMCIPhoneApplicationStatusExecute;
	 
	// Register for keyboard activation/deactivation notifications.
	[[NSNotificationCenter defaultCenter] addObserver:self
											 selector:@selector(keyboardWillActivate:)
												 name: UIKeyboardWillShowNotification object:nil];
	[[NSNotificationCenter defaultCenter] addObserver:self
											 selector:@selector(keyboardWillDeactivate:)
												 name: UIKeyboardWillHideNotification object:nil];
    
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(keyboardDidActivate:)
                                                 name: UIKeyboardDidShowNotification object:nil];
    
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(keyboardDidDeactivate:)
                                                 name: UIKeyboardDidHideNotification object:nil];
	
	// Swap over the controllers.
	[m_window setRootViewController: m_main_controller];
	
	// Honour any previous keyboard request.
	if (m_keyboard_activation_pending)
		[self activateKeyboard];
	
	// Ensure the orientation is synced.
	[self syncOrientation];

	// Invoke the hook on the engine side.
	MCIPhoneHandleDidStartExecuting();
}

//////////

- (void)beginAutorotation
{
	MCLog("Application: beginAutorotation\n");
	
	m_in_autorotation = true;
}

- (void)endAutorotation
{
	MCLog("Application: endAutorotation\n");
	
	m_in_autorotation = false;

	// If a request was made to switch to prepare state during auto-rotation, then
	// do it.
	if (m_prepare_status_pending)
	{
		m_prepare_status_pending = false;
		[self startPreparing];
		return;
	}
	
	// Sync the orientation - if needed.
	[self syncOrientation];
}

//////////

- (void)orientationChanged:(NSNotification *)notification
{
	MCLog("Application: orientationChanged (%d)\n", [[UIDevice currentDevice] orientation]);
	
	// Fetch the orientation that is being requested.
	UIDeviceOrientation t_device_orientation;
	t_device_orientation = [[UIDevice currentDevice] orientation];
	
	if (UIDeviceOrientationIsValidInterfaceOrientation(t_device_orientation))
	{
		UIInterfaceOrientation t_new_orientation;
		switch(t_device_orientation)
		{
			case UIDeviceOrientationPortrait:
				t_new_orientation = UIInterfaceOrientationPortrait;
				break;
			case UIDeviceOrientationPortraitUpsideDown:
				t_new_orientation = UIInterfaceOrientationPortraitUpsideDown;
				break;
			case UIDeviceOrientationLandscapeLeft:
				t_new_orientation = UIInterfaceOrientationLandscapeRight;
				break;
			case UIDeviceOrientationLandscapeRight:
				t_new_orientation = UIInterfaceOrientationLandscapeLeft;
				break;
            default:
                MCUnreachableReturn();
		}
		
		// Store the pending notification.
		if (m_pending_orientation_notification != notification)
		{
			[m_pending_orientation_notification release];
			m_pending_orientation_notification = notification;
			[notification retain];
		}
		
		// Store the actual device orientation for patching.
		s_pending_device_orientation = t_device_orientation;
		
		// Store the orientation as the pending orientation.
		m_pending_orientation = t_new_orientation;
	}
	
	// Force an orientation sync event.
	[self forceSyncOrientation];
}

- (void)keyboardWillActivate:(NSNotification *)notification
{
    NSDictionary *t_info;
	t_info = [notification userInfo];
	
    CGSize t_size;
	t_size = [[t_info objectForKey:UIKeyboardFrameEndUserInfoKey] CGRectValue].size;
    
    // MM-2012-02-26: [[ Bug 10677 ]] Keyboard dimensions do not take into account orientation.
    //  We want height here, so assume the keyboard is always wider than it is taller and take the min of the two.
    CGFloat t_height = MCMin(t_size . height, t_size . width);
    MCIPhoneHandleKeyboardWillActivate(t_height);
    
    if (m_keyboard_display == kMCIPhoneKeyboardDisplayModePan)
    {
        MCObject *t_object = nullptr;
        if (MCactivefield.IsValid())
        {
            t_object = MCactivefield;
        }
        
        if (t_object == nullptr)
        {
            t_object = MCdefaultstackptr->getcard()->getkfocused();
        }
        
        CGRect t_focused_rect;
        if (t_object != nullptr)
        {
            MCRectangle t_object_rect = t_object -> getrect();
            MCGAffineTransform t_transform = t_object->getstack()->getroottransform();
            
            MCRectangle t_transformed_object_rect =
                    MCRectangleGetTransformedBounds(t_object_rect, t_transform);
            t_focused_rect = MCRectangleToCGRect(t_transformed_object_rect);
        }
        else
        {
            UIView *t_responder = [[m_main_controller rootView] com_runrev_livecode_findFirstResponder];
            if (t_responder == nil)
            {
                return;
            }
            
            t_focused_rect = t_responder.frame;
        }
        
        UIView* t_view = [m_main_controller rootView];
        CGRect t_frame = t_view.frame;
        
        CGFloat t_pan = t_frame.size.height - t_height - (t_focused_rect.origin.y + t_focused_rect.size.height);
        
        // if the focused rect is high we don't want to pan the top of it out of view
        if (t_pan < -t_focused_rect.origin.y)
        {
            t_pan = -t_focused_rect.origin.y;
        }
        
        if (t_pan > 0)
        {
            // no need to pan
            return;
        }
        
        [UIView beginAnimations: @"panrootviewup" context: nil];
        [UIView setAnimationBeginsFromCurrentState: YES];
        [UIView setAnimationDuration: 0.2f];
        t_frame.origin.y = t_pan;
        t_view.frame = t_frame;
        [UIView commitAnimations];
    }
}

- (void)keyboardWillDeactivate:(NSNotification *)notification
{
	MCIPhoneHandleKeyboardWillDeactivate();
    
    UIView* t_view = [m_main_controller rootView];
    if (t_view.frame.origin.y != 0)
    {
        [UIView beginAnimations: @"panrootviewdown" context: nil];
        [UIView setAnimationBeginsFromCurrentState: YES];
        [UIView setAnimationDuration: 0.2f];
        t_view.frame = CGRectOffset(t_view.frame, 0, -t_view.frame.origin.y);
        [UIView commitAnimations];
    }
}

- (void)keyboardDidActivate:(NSNotification *)notification
{
    m_keyboard_is_visible = true;
}

- (void)keyboardDidDeactivate:(NSNotification *)notification
{
    m_keyboard_is_visible = false;
    
    [[[m_main_controller rootView] textView] setKeyboardType: m_keyboard_type];
    [[[m_main_controller rootView] textView] setReturnKeyType: m_return_key_type];
    
}

- (BOOL)isKeyboardVisible
{
    return m_keyboard_is_visible;
}
//////////

- (void)switchToStatusBarStyle: (UIStatusBarStyle)p_new_style
{
	m_status_bar_style = p_new_style;
	
	[m_application setStatusBarStyle: p_new_style animated: NO];
	[[m_main_controller rootView] reshape];
}

- (void)switchToStatusBarVisibility: (BOOL)p_new_visible
{
	m_status_bar_hidden = !p_new_visible;
	
	[m_application setStatusBarHidden: !p_new_visible withAnimation: UIStatusBarAnimationNone];
	[[m_main_controller rootView] reshape];
}

// PM-2015-02-17: [[ Bug 14482 ]]
// This method should be called before switchToStatusBarStyle
- (void)setStatusBarSolid: (BOOL)p_is_solid
{
    m_status_bar_solid = p_is_solid;
}

//////////

- (CGRect)fetchScreenBounds
{
	CGRect t_viewport;
	t_viewport = [[UIScreen mainScreen] bounds];
	
    // MW-2014-10-02: [[ iOS 8 Support ]] iOS 8 already takes into account orientation when returning the bounds.
	if (MCmajorosversion < 800 && UIInterfaceOrientationIsLandscape([self fetchOrientation]))
		return CGRectMake(0.0f, 0.0f, t_viewport . size . height, t_viewport . size . width);
	
	return t_viewport;
}

- (CGRect)fetchViewBounds
{
	CGRect t_viewport;
	t_viewport = [[UIScreen mainScreen] bounds];

	// MW-2011-10-24: [[ Bug ]] The status bar only clips the display if actually
	//   hidden, or on iPhone with black translucent style.
    // MM-2013-09-25: [[ iOS7Support ]] The status bar is always overlaid ontop of the view, irrespective of its style.
    // PM-2015-02-17: [[ Bug 14482 ]] If the style is "solid", do not put status bar on top of the view
	CGFloat t_status_bar_size;
	if (m_status_bar_hidden || (MCmajorosversion >= 700 && !m_status_bar_solid)|| ([[UIDevice currentDevice] userInterfaceIdiom] == UIUserInterfaceIdiomPhone && m_status_bar_style == UIStatusBarStyleBlackTranslucent))
		t_status_bar_size = 0.0f;
	else
		t_status_bar_size = 20.0f;
	
    // MM-2014-09-26: [[ iOS 8 Support ]] iOS 8 already takes into account orientation when returning the bounds.
	if (MCmajorosversion < 800 && UIInterfaceOrientationIsLandscape([self fetchOrientation]))
		return CGRectMake(0.0f, t_status_bar_size, t_viewport . size . height, t_viewport . size . width - t_status_bar_size);
	
	return CGRectMake(0.0f, t_status_bar_size, t_viewport . size . width, t_viewport . size . height - t_status_bar_size);
}

- (UIInterfaceOrientation)fetchOrientation
{
	// During startup, the orientation is that of the startup controller.
	if (m_status == kMCIPhoneApplicationStatusStartup)
		return [m_startup_controller interfaceOrientation];
	
	// Otherwise, it is that of the main controller.
	return [m_main_controller pendingOrientation];
}

- (MCIPhoneRootView *)fetchRootView
{
	return [m_main_controller rootView];
}

- (MCIPhoneMainView *)fetchMainView
{
	return [[m_main_controller rootView] mainView];
}

- (MCIPhoneDisplayView *)fetchDisplayView
{
	return [[m_main_controller rootView] displayView];
}

- (UIViewController *)fetchMainViewController
{
	return m_main_controller;
}

// MM-2014-10-15: [[ Bug 13665 ]] Returns the currently active view controller.
- (UIViewController *)fetchCurrentViewController
{
    if (m_status == kMCIPhoneApplicationStatusStartup || m_status == kMCIPhoneApplicationStatusPrepare)
        return m_startup_controller;
    else
        return m_main_controller;
}

- (MCStringRef)fetchDeviceToken
{
    return m_device_token;
}

- (MCStringRef)fetchLaunchUrl
{
	return m_launch_url;
}

extern UIKeyboardType MCInterfaceGetUIKeyboardTypeFromExecEnum(MCInterfaceKeyboardType p_type);
extern UIReturnKeyType MCInterfaceGetUIReturnKeyTypeFromExecEnum(MCInterfaceReturnKeyType p_type);

- (void)activateKeyboard
{
	// If we are still starting up, then record the request.
	if (m_status == kMCIPhoneApplicationStatusStartup)
	{
		m_keyboard_activation_pending = true;
		return;
	}
    
    if (MCactivefield.IsValid())
    {
        MCInterfaceKeyboardType t_ktype = MCactivefield->getkeyboardtype();
        if (t_ktype != kMCInterfaceKeyboardTypeNone)
        {
            UIKeyboardType t_keyboard_type = MCInterfaceGetUIKeyboardTypeFromExecEnum(t_ktype);
            [[[m_main_controller rootView] textView] setKeyboardType: t_keyboard_type];
        }
        else
        {
            [[[m_main_controller rootView] textView] setKeyboardType: m_keyboard_type];
        }
        
        MCInterfaceReturnKeyType t_rtype = MCactivefield->getreturnkeytype();
        if (t_rtype != kMCInterfaceReturnKeyTypeNone)
        {
            UIReturnKeyType t_return_key_type = MCInterfaceGetUIReturnKeyTypeFromExecEnum(t_rtype);
            [[[m_main_controller rootView] textView] setReturnKeyType:(UIReturnKeyType) t_return_key_type];
        }
        else
        {
            [[[m_main_controller rootView] textView] setReturnKeyType: m_return_key_type];
        }
    }
	
	// Tell the root view to activate it's text view.
	[[m_main_controller rootView] activateKeyboard];
}

- (void)deactivateKeyboard
{
	// If we are in startup, unrecord any keyboard request.
	if (m_status == kMCIPhoneApplicationStatusStartup)
	{
		m_keyboard_activation_pending = false;
		return;
	}
	
	// If the root view switches from text to main activation, we are
	// done.
	if ([[m_main_controller rootView] deactivateKeyboard])
		return;
	
	// Otherwise, find the current first responder and force it to resign.
	UIView *t_first_responder;
	t_first_responder = [m_window com_runrev_livecode_findFirstResponder];
	if (t_first_responder != nil)
		[t_first_responder resignFirstResponder];
	
	// Make sure our main view becomes first responder.
	[[[m_main_controller rootView] mainView] becomeFirstResponder];
}

- (UIKeyboardType)fetchKeyboardType
{
	return [[[m_main_controller rootView] textView] keyboardType];
}

- (void)configureKeyboardType: (UIKeyboardType)p_new_type
{
    m_keyboard_type = p_new_type;
    
	[[[m_main_controller rootView] textView] setKeyboardType: p_new_type];
}

- (void)configureReturnKeyType: (UIReturnKeyType)p_new_type
{
    m_return_key_type = p_new_type;
	[[[m_main_controller rootView] textView] setReturnKeyType: p_new_type];
}

- (void)configureKeyboardDisplay: (MCIPhoneKeyboardDisplayMode)p_mode
{
    m_keyboard_display = p_mode;
}
//////////

- (uint32_t)allowedOrientations
{
	return m_allowed_orientations;
}

- (void)setAllowedOrientations: (uint32_t)allowedOrientations
{	
	// If we aren't changing anything, do nothing.
	if (m_allowed_orientations == allowedOrientations)
		return;
	
	m_allowed_orientations = allowedOrientations;
	
	[self forceSyncOrientation];
}

// Lock orientation changes.
- (void)lockOrientation
{
	m_orientation_lock += 1;
}

// Unlock orientation changes.
- (void)unlockOrientation
{
	if (m_orientation_lock == 0)
		return;
	
	m_orientation_lock -= 1;
	
	if (m_orientation_lock == 0)
		[self forceSyncOrientation];
}

// Returns if orientation is currently locked.
- (BOOL)orientationLocked
{
	return m_orientation_lock != 0;
}

- (void)forceSyncOrientation
{
	// If we are already in an orientation changed request, do nothing.
	if (m_in_orientation_changed)
		return;
	
	// Mark there as being a orientation change pending.
	m_orientation_changed_pending = true;
	
	// Now just sync.
	[self syncOrientation];
}

- (void)syncOrientation
{
	// If we are already in an orientation changed request, do nothing.
	if (m_in_orientation_changed)
		return;
	
	// If there is no pending orientation changed, do nothing.
	if (!m_orientation_changed_pending)
		return;

	// If we are not executing, or are in an autorotation, do nothing.
	if (m_in_autorotation || m_status != kMCIPhoneApplicationStatusExecute)
		return;
	
	MCLog("Application: handleOrientationChanged\n");
	
	// We are in a suitable state for orientationChanged dispatch.
	m_orientation_changed_pending = false;
	m_in_orientation_changed = true;
	MCIPhoneHandleOrientationChanged();
}

// Commit to the current pending orientation if suitable.
- (void)commitOrientation
{
	MCLog("Application: commitOrientation\n");
	
	// There is no longer a change request pending.
	m_orientation_changed_pending = false;
	
	// If the orientation lock is in effect, do notion.
	if (m_orientation_lock == 0 && m_allowed_orientations != 0)
	{
		// Work out what orientation to use.
		uint32_t t_new_orientation;
		t_new_orientation = 0;
		
		// If there is just a single orientation in the set, use that.
		for(uint32_t i = 1; i <= 4; i++)
			if ((m_allowed_orientations & (1 << i)) == m_allowed_orientations)
				t_new_orientation = i;
		
		// If the pending orientation is in the allowed set, use that. Otherwise
		// we choose the 'closest' orientation we can.
		if (t_new_orientation == 0)
		{
			if ((m_allowed_orientations & (1 << m_pending_orientation)) != 0)
				t_new_orientation = m_pending_orientation;
			else if (UIInterfaceOrientationIsLandscape(m_pending_orientation))
			{
				if ((m_allowed_orientations & (1 << UIInterfaceOrientationPortrait)) != 0)
					t_new_orientation = UIInterfaceOrientationPortrait;
				else
					t_new_orientation = UIInterfaceOrientationPortraitUpsideDown;
			}
			else
			{
				if ((m_allowed_orientations & (1 << UIInterfaceOrientationLandscapeLeft)) != 0)
					t_new_orientation = UIInterfaceOrientationLandscapeLeft;
				else
					t_new_orientation = UIInterfaceOrientationLandscapeRight;
			}
		}
		
		// The orientation is not the chosen one, then switch.
		if ([m_main_controller pendingOrientation] != t_new_orientation)
		{
			MCLog("Application: switchToOrientation (%d)\n", t_new_orientation);
		
			[m_main_controller setPendingOrientation: (UIInterfaceOrientation)t_new_orientation];
		
			if (m_pending_orientation_notification != nil)
			{
				// MW-2012-02-24: [[ Bug 10020 ]] Make sure we set the pending orientation correctly before
				//   forcing the change.
				s_pending_device_orientation = (UIDeviceOrientation)t_new_orientation;
				
				IMP t_original;
				t_original = class_replaceMethod([UIDevice class], @selector(orientation), (IMP)patch_device_orientation, @encode(UIDeviceOrientation));
				[[NSNotificationCenter defaultCenter] postNotification: m_pending_orientation_notification];
				class_replaceMethod([UIDevice class], @selector(orientation), t_original, @encode(UIDeviceOrientation));
			}
		}
	}
	
	// We are finished with the orientation changed.
	m_in_orientation_changed = false;
};

//////////

- (void)setRedrawInterval: (uint32_t)p_interval
{
	// If the redraw interval is 0, then do nothing.
	if (p_interval <= 0)
	{
		if (m_display_link != nil)
		{
			[m_display_link invalidate];
			[m_display_link release];
			m_display_link = nil;
		}
		
		return;
	}
	
	// We have a redraw interval so make sure we have an display link.
	if (m_display_link == nil)
	{
		m_display_link = [CADisplayLink displayLinkWithTarget: self selector: @selector(performRedraw:)];
		[m_display_link addToRunLoop: [NSRunLoop mainRunLoop] forMode: NSRunLoopCommonModes];
		
		// MW-2011-10-17: [[ Bug 9814 ]] Make sure we retain the object else we crash
		//   later!
		[m_display_link retain];
	}
	
	// Set the redraw interval.
	[m_display_link setFrameInterval: p_interval];
}

- (void)disableRedrawInterval
{
	if (m_display_link != nil)
		[m_display_link setPaused: YES];
}

- (void)enableRedrawInterval
{
	if (m_display_link != nil)
		[m_display_link setPaused: NO];
}

- (void)performRedraw: (CADisplayLink *)sender
{
	MCIPhoneHandlePerformRedraw();
}

//////////

- (void)setAnimateAutorotation: (bool)p_animate
{
	[[m_main_controller rootView] setAnimateAutorotation: p_animate];
}

//////////

- (void)switchToUIKitDisplay
{
    MCLog("Application: switchToUIKitDisplay");
	[[m_main_controller rootView] switchToDisplayClass: [MCIPhoneUIKitDisplayView class]];
}

- (void)switchToOpenGLDisplay
{
    MCLog("Application: switchToOpenGLDisplay");
	[[m_main_controller rootView] switchToDisplayClass: [MCIPhoneOpenGLDisplayView class]];
}

- (void)configureContentScale: (float)p_new_scale
{
	UIView *t_display_view;
	t_display_view = [[m_main_controller rootView] displayView];
	if ([t_display_view isMemberOfClass: [MCIPhoneOpenGLDisplayView class]])
	{
		[t_display_view setContentScaleFactor: p_new_scale];
		
		MCIPhoneOpenGLDisplayView *t_gl_view = static_cast<MCIPhoneOpenGLDisplayView*>(t_display_view);
		// IM-2014-02-18: [[ Bug 11814 ]] Update the GL layer size after changing scale
		[t_gl_view resizeFromLayer:(CAEAGLLayer *)[t_gl_view layer]];
	}
}

@end

////////////////////////////////////////////////////////////////////////////////

@implementation MCIPhoneWindow

- (BOOL)passMotion
{
	UIView *t_responder;
	t_responder = [self com_runrev_livecode_findFirstResponder];
	if (t_responder != nil)
		return [t_responder com_runrev_livecode_passMotionEvents];
	return NO;
}

- (void)motionBegan: (UIEventSubtype)motion withEvent: (UIEvent *)event
{
	if ([self passMotion])
	{
		[super motionBegan: motion withEvent: event];
		return;
	}
	
	MCIPhoneHandleMotionBegan(motion, [event timestamp]);
}

- (void)motionCancelled: (UIEventSubtype)motion withEvent: (UIEvent *)event
{
	if ([self passMotion])
	{
		[super motionCancelled: motion withEvent: event];
		return;
	}
	
	MCIPhoneHandleMotionCancelled(motion, [event timestamp]);
}

- (void)motionEnded:(UIEventSubtype)motion withEvent: (UIEvent *)event
{
	if ([self passMotion])
	{
		[super motionEnded: motion withEvent: event];
		return;
	}
	
	MCIPhoneHandleMotionEnded(motion, [event timestamp]);
}

// MW-2013-05-30: [[ RemoteControl ]] Handle the remote control event and pass
//   on if applicable.
- (void)remoteControlReceivedWithEvent: (UIEvent *)event
{
	if ([event type] == UIEventTypeRemoteControl)
		MCIPhoneHandleRemoteControlEvent([event subtype], [event timestamp]);
}

@end

////////////////////////////////////////////////////////////////////////////////

@implementation MCIPhoneStartupViewController

- (id)init
{
	// Initialize the superclass.
	self = [super init];
	if (self == nil)
		return nil;
	
	m_image_orientation = UIInterfaceOrientationPortrait;
	
	// We always use full screen layout.
	[self setWantsFullScreenLayout: YES];
	
	return self;
}

- (void)dealloc
{
	// Finalize the superclass.
	[super dealloc];
}

// MM-2012-09-25: [[ iOS 6.0 ]] Implement the new iOS 6.0 auto rotation methods.
- (BOOL)shouldAutorotate
{
    MCLog("StartupViewController: shouldAutorotate\n");
    return [[MCIPhoneApplication sharedApplication] status] == kMCIPhoneApplicationStatusStartup;
}

// MM-2012-09-25: [[ iOS 6.0 ]] Implement the new iOS 6.0 auto rotation methods.
- (NSUInteger)supportedInterfaceOrientations
{
    MCLog("StartupViewController: supportedInterfaceOrientations\n");
    return [[MCIPhoneApplication sharedApplication] allowedOrientations];
}

- (BOOL)shouldAutorotateToInterfaceOrientation: (UIInterfaceOrientation)p_new_orientation
{
	MCLog("StartupViewController: shouldAutorotateToInterfaceOrientation %d\n", p_new_orientation);
	
	// If the orientation is now locked (happens when the app enters 'prepare'
	// state) then we don't allow anymore autorotation.
	if ([[MCIPhoneApplication sharedApplication] status] != kMCIPhoneApplicationStatusStartup)
		return m_image_orientation == p_new_orientation;
	
	// Otherwise, we allow any orientation in the current allowed orientations.
	return ([[MCIPhoneApplication sharedApplication] allowedOrientations] & (1 << p_new_orientation)) != 0;
}

- (void)viewDidLoad;
{
	MCLog("StartupViewController: viewDidLoad\n");
}

- (void)viewDidUnload;
{
	MCLog("StartupViewController: viewDidUnload\n");
}

- (void)viewWillAppear:(BOOL)animated
{
	MCLog("StartupViewController: viewWillAppear\n");
}

- (void)viewDidAppear:(BOOL)animated
{
	MCLog("StartupViewController: viewDidAppear\n");
}

- (void)viewWillDisappear:(BOOL)animated
{
	MCLog("StartupViewController: viewWillDisappear\n");
}

- (void)viewDidDisappear:(BOOL)animated
{
	MCLog("StartupViewController: viewDidDisappear\n");
}

- (void)willRotateToInterfaceOrientation:(UIInterfaceOrientation)toInterfaceOrientation duration:(NSTimeInterval)duration
{
	MCLog("StartupViewController: willRotateToInterfaceOrientation (%d)\n", toInterfaceOrientation);
	[[MCIPhoneApplication sharedApplication] beginAutorotation];
}

- (void)willAnimateRotationToInterfaceOrientation:(UIInterfaceOrientation)newInterfaceOrientation duration:(NSTimeInterval)duration
{
	MCLog("StartupViewController: willAnimateToInterfaceOrientation (%d)\n", newInterfaceOrientation);
}

- (void)didRotateFromInterfaceOrientation:(UIInterfaceOrientation)fromInterfaceOrientation
{
	MCLog("StartupViewController: didRotateFromInterfaceOrientation (%d)\n", fromInterfaceOrientation);
	[[MCIPhoneApplication sharedApplication] endAutorotation];
}

@end

////////////////////////////////////////////////////////////////////////////////

@implementation MCIPhoneMainViewController

- (id)init
{
	MCLog("MainViewController: init\n");
	
	// Initialize the superclass.
	self = [super init];
	if (self == nil)
		return nil;
	
	// The root view of the view controller - this contains the app's views.
	m_root_view = [[MCIPhoneRootView alloc] init];
	
    // MM-2012-09-25: [[ iOS 6.0 ]] Added m_pending orientaiton to differentiate the current orientation of the controller and the orientation we want it to be in.
    //   Used by the new iOS 6.0 orientation methods.
	// We assume we are in portrait orientation to begin with.
    m_pending_orientation = UIInterfaceOrientationPortrait;
	m_current_orientation = UIInterfaceOrientationPortrait;
	
	// We always use full screen layout - the main/display subviews of the root
	// view are adjusted to take into account the statusbar.
	[self setWantsFullScreenLayout: YES];
	
	return self;
}

- (void)dealloc
{
    // PM-2014-10-13: [[ Bug 13659 ]] Remove the observer
    [[NSNotificationCenter defaultCenter] removeObserver:self];
	[m_root_view release];
	
	[super dealloc];
}

- (MCIPhoneRootView *)rootView
{
	return m_root_view;
}

- (UIInterfaceOrientation)pendingOrientation
{
	return m_pending_orientation;
}

- (void)setPendingOrientation: (UIInterfaceOrientation)p_new_orientation
{
	MCLog("MainViewController: setPendingOrientation (%d)\n", p_new_orientation);
	
	m_pending_orientation = p_new_orientation;
}

//////////

- (void)loadView
{
	MCLog("MainViewController: loadView\n");

	// Load the view.
	[self setView: m_root_view];
}

- (void)viewDidLoad;
{
	MCLog("MainViewController: viewDidLoad\n");
    
    // PM-2014-10-13: [[ Bug 13659 ]] Make sure we can interact with the LC app when Voice Over is enabled/disabled while our view is already onscreen
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(voiceOverStatusChanged)
                                                 name:UIAccessibilityVoiceOverStatusChanged
                                               object:nil];
}

- (void)viewDidUnload;
{
	MCLog("MainViewController: viewDidUnload\n");
}

- (void)viewWillAppear:(BOOL)animated
{
	MCLog("MainViewController: viewWillAppear\n");
}

- (void)viewDidAppear:(BOOL)animated
{
	MCLog("MainViewController: viewDidAppear\n");
	
    // MM-2012-09-25: [[ iOS 6.0 ]] When the startup view controller is visible, it appears that didRotateFromInterfaceOrientation is not called.
    //   Set current orientation here instead.
    if (MCmajorosversion >= 600)
        m_current_orientation = m_pending_orientation;
}

- (void)viewWillDisappear:(BOOL)animated
{
	MCLog("MainViewController: viewWillDisappear\n");
}

- (void)viewDidDisappear:(BOOL)animated
{
	MCLog("MainViewController: viewDidDisappear\n");
}

//////////

// MM-2012-09-25: [[ iOS 6.0 ]] Implement the new iOS 6.0 rotation methods.
//   We should only rotate if the current orientation of the controller is not that of the desired orientaiton.
- (BOOL)shouldAutorotate
{
    MCLog("MainViewController: shouldAutorotate\n");
    return m_pending_orientation != m_current_orientation;
}

// MM-2012-09-25: [[ iOS 6.0 ]] Implement the new iOS 6.0 rotation methods.
//   Called after shouldAutorotate.  Just return the list of allowed orientations for the app.
- (NSUInteger)supportedInterfaceOrientations
{
    MCLog("MainViewController: supportedInterfaceOrientations\n");
    return [[MCIPhoneApplication sharedApplication] allowedOrientations];
}

- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)p_requested_orientation
{
	MCLog("MainViewController: shouldAutorotateToInterfaceOrientation %d\n", p_requested_orientation);
	
	// If the view has already been loaded, we stick with whatever orientation
	// we currently have set.
	return m_pending_orientation == p_requested_orientation;
}

- (void)willRotateToInterfaceOrientation:(UIInterfaceOrientation)toInterfaceOrientation duration:(NSTimeInterval)duration
{
	MCLog("MainViewController: willRotateToInterfaceOrientation (%d)\n", toInterfaceOrientation);
	
	// Tell the application autorotation is about to start. This prevents
	// orientation updates occuring during rotation.
	[[MCIPhoneApplication sharedApplication] beginAutorotation];
	
	[m_root_view beginAutorotation];
}

- (void)willAnimateRotationToInterfaceOrientation:(UIInterfaceOrientation)newInterfaceOrientation duration:(NSTimeInterval)duration
{
	MCLog("MainViewController: willAnimateToInterfaceOrientation (%d)\n", newInterfaceOrientation);
	
	[m_root_view animateAutorotation];
}

- (void)didRotateFromInterfaceOrientation:(UIInterfaceOrientation)fromInterfaceOrientation
{
	MCLog("MainViewController: didRotateFromInterfaceOrientation (%d)\n", fromInterfaceOrientation);
	
	[m_root_view endAutorotation];
	
	// Tell the application autorotation has ended. This will sync the
	// orientation again if needed.
	[[MCIPhoneApplication sharedApplication] endAutorotation];
    
    // MM-2012-09-25: [[ iOS 6.0 ]] We've finished rotation, so the current orientation is now the required orientation.
    m_current_orientation = m_pending_orientation;
}

// PM-2014-10-13: [[ Bug 13659 ]] Make sure we can interact with the LC app when Voice Over is enabled/disabled while our view is already onscreen
- (void)voiceOverStatusChanged
{
    if (MCignorevoiceoversensitivity == True)
        return;
    
    UIView *t_main_view;
    t_main_view = [[MCIPhoneApplication sharedApplication] fetchMainView];

    if (UIAccessibilityIsVoiceOverRunning())
    {
        t_main_view.isAccessibilityElement = YES;
#ifdef __IPHONE_5_0
        [t_main_view setAccessibilityTraits:UIAccessibilityTraitAllowsDirectInteraction];
#endif
    }
    else
    {
        [t_main_view setAccessibilityTraits:UIAccessibilityTraitNone];
        t_main_view.isAccessibilityElement = NO;
    }
}

- (void)traitCollectionDidChange:(UITraitCollection *)previousTraitCollection
{
	MCNotificationPostSystemAppearanceChanged();
}

@end

////////////////////////////////////////////////////////////////////////////////

// PM-2014-10-22: [[ Bug 13750 ]] Utility method to determine the exact device model. If on simulator, it returns "i386" or "x86_64"
NSString* MCIPhoneGetDeviceModelName(void)
{
    struct utsname t_system_info;
    uname(&t_system_info);
    
    NSString *t_machine_name = [NSString stringWithCString:t_system_info.machine encoding:NSUTF8StringEncoding];
    
    // MARK: We can just return t_machine_name. Following is for convenience
    // Full list at http://theiphonewiki.com/wiki/Models
    
	NSDictionary *commonNamesDictionary = [NSDictionary dictionaryWithObjectsAndKeys:
										   @"iPhone",       @"iPhone1,1",
										   @"iPhone 3G",    @"iPhone1,2",
										   @"iPhone 3GS",   @"iPhone2,1",
                                           @"iPhone 4",     @"iPhone3,1",
										   
										   @"iPhone 4(Rev A)",      @"iPhone3,2",
										   @"iPhone 4(CDMA)",       @"iPhone3,3",
										   @"iPhone 4S",            @"iPhone4,1",
										   @"iPhone 5(GSM)",        @"iPhone5,1",
										   @"iPhone 5(GSM+CDMA)",   @"iPhone5,2",
										   @"iPhone 5c(GSM)",       @"iPhone5,3",
										   @"iPhone 5c(GSM+CDMA)",  @"iPhone5,4",
										   @"iPhone 5s(GSM)",       @"iPhone6,1",
                                           @"iPhone 5s(GSM+CDMA)",  @"iPhone6,2",
										   
										   @"iPhone 6+ (GSM+CDMA)", @"iPhone7,1",
                                           @"iPhone 6 (GSM+CDMA)",  @"iPhone7,2",
										   
                                           @"iPad",                     @"iPad1,1",
                                           @"iPad 2(WiFi)",             @"iPad2,1",
                                           @"iPad 2(GSM)",              @"iPad2,2",
										   @"iPad 2(CDMA)",             @"iPad2,3",
										   @"iPad 2(WiFi Rev A)",       @"iPad2,4",
										   @"iPad Mini 1G (WiFi)",      @"iPad2,5",
										   @"iPad Mini 1G (GSM)",       @"iPad2,6",
										   @"iPad Mini 1G (GSM+CDMA)",  @"iPad2,7",
                                           @"iPad 3(WiFi)",             @"iPad3,1",
										   @"iPad 3(GSM+CDMA)",         @"iPad3,2",
										   @"iPad 3(GSM)",              @"iPad3,3",
										   @"iPad 4(WiFi)",             @"iPad3,4",
                                           @"iPad 4(GSM)",              @"iPad3,5",
										   @"iPad 4(GSM+CDMA)",         @"iPad3,6",
                                        
                                           @"iPad Air(WiFi)",        @"iPad4,1",
                                           @"iPad Air(GSM)",         @"iPad4,2",
                                           @"iPad Air(GSM+CDMA)",    @"iPad4,3",
										   
                                           @"iPad Mini 2G (WiFi)",       @"iPad4,4",
                                           @"iPad Mini 2G (GSM)",        @"iPad4,5",
                                           @"iPad Mini 2G (GSM+CDMA)",   @"iPad4,6",
										   
                                           @"iPod 1st Gen",         @"iPod1,1",
                                           @"iPod 2nd Gen",         @"iPod2,1",
                                           @"iPod 3rd Gen",         @"iPod3,1",
										   @"iPod 4th Gen",         @"iPod4,1",
                                           @"iPod 5th Gen",         @"iPod5,1",
                                           // PM-2015-03-03: [[ Bug 14689 ]] Cast to NSString* to prevent EXC_BAD_ACCESS when in release mode and run in 64bit device/sim
                                           (NSString *)nil];
										   
	
	NSString *t_device_name = [commonNamesDictionary objectForKey: t_machine_name];
    
    if (t_device_name == nil)
        t_device_name = t_machine_name;
    
    return t_device_name;
}

MCIPhoneApplication *MCIPhoneGetApplication(void)
{
	return [MCIPhoneApplication sharedApplication];
}

UIView *MCIPhoneGetView(void)
{
    // PM-2014-10-13: [[ Bug 13659 ]] Make sure we can interact with the LC app when Voice Over is turned on
    // This only takes care of situations where VoiceOver is in use when our view loads.
    // In case Voice Over is activated or disabled when our view is already onscreen,
    // we need to register an observer for the notification in the viewDidLoad method
    UIView *t_main_view;
    t_main_view = [[MCIPhoneApplication sharedApplication] fetchMainView];
    
    if (UIAccessibilityIsVoiceOverRunning())
    {
        t_main_view.isAccessibilityElement = YES;
#ifdef __IPHONE_5_0
        [t_main_view setAccessibilityTraits:UIAccessibilityTraitAllowsDirectInteraction];
#endif
    }
    
    return t_main_view;
}

UIView *MCIPhoneGetRootView(void)
{
	return [[MCIPhoneApplication sharedApplication] fetchRootView];
}

UIView *MCIPhoneGetDisplayView(void)
{
	return [MCIPhoneGetRootView() displayView];
}

// MM-2014-10-15: [[ Bug 13665 ]] Return the topmost view controller. This is not necessarily always the main view controller,
//   could be the startup view controller. Was causing issues when presenting dialogs on startup.
UIViewController *MCIPhoneGetViewController(void)
{
    return [[MCIPhoneApplication sharedApplication] fetchCurrentViewController];
}

void MCIPhoneSetKeyboardType(UIKeyboardType p_type)
{
	[[MCIPhoneApplication sharedApplication] configureKeyboardType: p_type];
}

UIKeyboardType MCIPhoneGetKeyboardType(void)
{
	return [[MCIPhoneApplication sharedApplication] fetchKeyboardType];
}

void MCIPhoneSetReturnKeyType(UIReturnKeyType p_type)
{
	[[MCIPhoneApplication sharedApplication] configureReturnKeyType: p_type];
}

void MCIPhoneSetKeyboardDisplay(MCIPhoneKeyboardDisplayMode p_mode)
{
    [[MCIPhoneApplication sharedApplication] configureKeyboardDisplay: p_mode];
}

void MCIPhoneActivateKeyboard(void)
{
	[[MCIPhoneApplication sharedApplication] activateKeyboard];
}

void MCIPhoneDeactivateKeyboard(void)
{
	[[MCIPhoneApplication sharedApplication] deactivateKeyboard];
}

// PM-2014-10-15: [[ Bug 13677 ]] Utility for checking if keyboard is currently on screen
bool MCIPhoneIsKeyboardVisible(void)
{
    return [[MCIPhoneApplication sharedApplication] isKeyboardVisible];
}

UIInterfaceOrientation MCIPhoneGetOrientation(void)
{
	return [[MCIPhoneApplication sharedApplication] fetchOrientation];
}

CGRect MCIPhoneGetViewBounds(void)
{
	return [[MCIPhoneApplication sharedApplication] fetchViewBounds];
}

CGRect MCIPhoneGetScreenBounds(void)
{
	return [[MCIPhoneApplication sharedApplication] fetchScreenBounds];
}

void MCIPhoneSwitchViewToOpenGL(void)
{
	[[MCIPhoneApplication sharedApplication] switchToOpenGLDisplay];
}

void MCIPhoneSwitchViewToUIKit(void)
{
	[[MCIPhoneApplication sharedApplication] switchToUIKitDisplay];
}

void MCIPhoneConfigureContentScale(int32_t p_scale)
{
	[[MCIPhoneApplication sharedApplication] configureContentScale: p_scale];
}

bool MCIPhoneIsEmbedded(void)
{
	return false;
}

Exec_stat MCIPhoneHandleMessage(MCNameRef p_message, MCParameter *p_parameters)
{
	return ES_NOT_HANDLED;
}

////////////////////////////////////////////////////////////////////////////////

// MW-2012-08-06: [[ Fibers ]] 'main' moved to here.

#define VALGRIND "/usr/local/bin/valgrind"

extern void setup_simulator_hooks(void);

static char *my_strndup(const char * p, int n)
{
	char *s;
	s = (char *)malloc(n + 1);
	strncpy(s, p, n);
	s[n] = '\0';
	return s;
}

MC_DLLEXPORT_DEF int platform_main(int argc, char *argv[], char *envp[])
{
#if defined(_DEBUG) && defined(_VALGRIND)
	if (argc < 2 ||  (argc >= 2 && strcmp(argv[1], "-valgrind") != 0))
	{
        execl(VALGRIND, VALGRIND, "--dsymutil=yes", "--leak-check=full", argv[0], "-valgrind", NULL);
		exit(-1);
	}
#endif
	
    if (!MCInitialize() ||
        !MCSInitialize() ||
        !MCScriptInitialize())
        return -1;
    
	int t_exit_code;
    
    // MW-2012-09-26: [[ Bug ]] Make sure we set a valid current folder on
    //   startup, otherwise the iOS6 simulator complains later.
    char *t_path;
    t_path = my_strndup(argv[0], strrchr(argv[0], '/') - argv[0]);
    chdir(t_path);
    free(t_path);
    
	NSAutoreleasePool *t_pool;
	t_pool = [[NSAutoreleasePool alloc] init];

	setup_simulator_hooks();
	
	t_exit_code = UIApplicationMain(argc, argv, nil, @"com_runrev_livecode_MCIPhoneApplication");
	
	[t_pool release];
	
	return t_exit_code;
}

////////////////////////////////////////////////////////////////////////////////
