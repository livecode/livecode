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

#include "prefix.h"

#include "core.h"
#include "parsedef.h"
#include "filedefs.h"
#include "globals.h"

#include <objc/runtime.h>
#include <objc/message.h>
#include <unistd.h>

#include "mbliphoneview.h"

#include "mblnotification.h"

////////////////////////////////////////////////////////////////////////////////

#ifdef _DEBUG
#undef MCLog
#define MCLog(...) NSLog(@__VA_ARGS__)
#endif

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
	
	m_in_autorotation = false;
	m_prepare_status_pending = false;
	
	m_pending_orientation_notification = nil;
	m_pending_orientation = UIInterfaceOrientationPortrait;
	m_orientation_lock = 0;
	m_in_orientation_changed = false;
	
	m_keyboard_activation_pending = false;
	
    m_pending_push_notification = nil;
    m_pending_local_notification = nil;
    m_device_token.set("", 1);
    m_launch_url.set("", 1);
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
	
	// Tell the device we want orientation notifications.
	[[UIDevice currentDevice] beginGeneratingDeviceOrientationNotifications];
	
	// Register for device orientation change notifications.
	[[NSNotificationCenter defaultCenter] addObserver: self
											 selector: @selector(orientationChanged:)
												 name: UIDeviceOrientationDidChangeNotification
											   object: nil];

	// We begin in 'startup' mode.
	m_status = kMCIPhoneApplicationStatusStartup;
	
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
	
	// Initialize our window with the main screen's bounds.
	m_window = [[MCIPhoneWindow alloc] initWithFrame: [[UIScreen mainScreen] bounds]];
	
	// Create the initial view controller.
	m_startup_controller = [[MCIPhoneStartupViewController alloc] init];
	
	// Create the main view controller.
	m_main_controller = [[MCIPhoneMainViewController alloc] init];

    // Check if we have received a local notification payload.
	Class t_cls = NSClassFromString(@"UILocalNotification");
    if (t_cls)
    {
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
        m_pending_push_notification = [[t_push_notification objectForKey:@"payload"] retain];

    // Check if we have received a custom URL
    // This check is carried out at application launch
    NSURL *t_launch_url = [p_launchOptions objectForKey: UIApplicationLaunchOptionsURLKey];
    if (t_launch_url)
    {    
        MCString t_url_text;
        t_url_text.set ([[t_launch_url absoluteString] cStringUsingEncoding:NSMacOSRomanStringEncoding], [[t_launch_url absoluteString] length]);
        m_launch_url = t_url_text.clone();
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
    UIRemoteNotificationType t_allowed_notifications = UIRemoteNotificationTypeNone;
	if (t_allowed_push_notifications_array != nil)
	{
		for (NSString *t_push_notification_string in t_allowed_push_notifications_array)
		{
			if ([t_push_notification_string isEqualToString: @"CFBadge"])
                t_allowed_notifications = (UIRemoteNotificationType) (t_allowed_notifications | UIRemoteNotificationTypeBadge);
            else if ([t_push_notification_string isEqualToString: @"CFSound"])
                t_allowed_notifications = (UIRemoteNotificationType) (t_allowed_notifications | UIRemoteNotificationTypeSound);
			else if ([t_push_notification_string isEqualToString: @"CFAlert"])
                t_allowed_notifications = (UIRemoteNotificationType) (t_allowed_notifications | UIRemoteNotificationTypeAlert);
		}
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

- (void)application:(UIApplication *)p_application didReceiveLocalNotification:(UILocalNotification *)p_notification
{
    MCLog("application:didReceiveLocalNotification:");
    UIApplicationState t_state = [p_application applicationState];
    MCString t_mc_reminder_text;
    NSString *t_reminder_text = [p_notification.userInfo objectForKey:@"payload"];
    t_mc_reminder_text.set ([t_reminder_text cStringUsingEncoding:NSMacOSRomanStringEncoding], [t_reminder_text length]);
    if (m_did_become_active)
    {
        if (t_state == UIApplicationStateInactive)
        {
            // The application is running the in the background, so launch the reminder text.
            MCNotificationPostLocalNotificationEvent (t_mc_reminder_text);
        }
        else
        {
            // Send a message to indicate that we have received a Local Notification. Include the reminder text.
            MCNotificationPostLocalNotificationEvent (t_mc_reminder_text);
        }
    }
}

- (void)application:(UIApplication *)p_application didReceiveRemoteNotification:(NSDictionary *)p_dictionary
{
    MCLog("application:didReceiveRemoteNotification:");
    UIApplicationState t_state = [p_application applicationState];
    MCString t_mc_push_notification_text;
    NSString *t_reminder_text = [p_dictionary objectForKey:@"payload"];
    if (t_reminder_text != nil)
        t_mc_push_notification_text.set ([t_reminder_text cStringUsingEncoding:NSMacOSRomanStringEncoding], [t_reminder_text length]);
    if (m_did_become_active)
    {
        if (t_state == UIApplicationStateInactive)
        {
            // The application is running the in the background, so launch the reminder text.
            MCNotificationPostPushNotificationEvent(t_mc_push_notification_text);
        }
        else
        {
            // Send a message to indicate that we have received a Local Notification. Include the reminder text.
            MCNotificationPostPushNotificationEvent (t_mc_push_notification_text);
        }
    }
}

- (void)application:(UIApplication*)p_application didRegisterForRemoteNotificationsWithDeviceToken:(NSData*)p_device_token
{
    NSString *t_to_log = [NSString stringWithFormat:@"%s%@%s", "Application: push notification device token (", p_device_token, ")"];
    NSString *t_registration_text = [NSString stringWithFormat:@"%@", p_device_token];
    if (t_registration_text != nil)
    {
        MCString t_device_token;
        t_device_token.set ([t_registration_text cStringUsingEncoding:NSMacOSRomanStringEncoding], [t_registration_text length]);
        m_device_token = t_device_token.clone();
        MCLog("%s\n", [t_to_log cStringUsingEncoding: NSMacOSRomanStringEncoding]);
        MCNotificationPostPushRegistered(m_device_token);
    }
}

- (void)application:(UIApplication*)p_application didFailToRegisterForRemoteNotificationsWithError:(NSError*)p_error
{
    NSString *t_to_log = [NSString stringWithFormat:@"%s%@%s", "Application: push notification device token error (", p_error, ")"];
    NSString *t_error_text = [NSString stringWithFormat:@"%@", p_error];
    if (t_error_text != nil)
    {
        MCString t_mc_error_text;
        t_mc_error_text.set ([t_error_text cStringUsingEncoding:NSMacOSRomanStringEncoding], [t_error_text length]);
        MCLog("%s\n", [t_to_log cStringUsingEncoding: NSMacOSRomanStringEncoding]);
        MCNotificationPostPushRegistrationError(t_mc_error_text);
    }
}

// Check if we have received a custom URL
// This handler is called at runtime, for example if the application tries to launch itself
- (BOOL)application:(UIApplication *)p_application openURL:(NSURL*)p_url sourceApplication:(NSString *)p_source_application annotation:(id)p_annotation
{
    BOOL t_result = NO;
    if (p_url != nil)
    {
        MCString t_url_text;
        t_url_text.set ([[p_url absoluteString] cStringUsingEncoding:NSMacOSRomanStringEncoding], [[p_url absoluteString] length]);
        m_launch_url = t_url_text.clone();
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
        MCString t_mc_reminder_text = nil;
        t_mc_reminder_text.set ([m_pending_local_notification cStringUsingEncoding:NSMacOSRomanStringEncoding], [m_pending_local_notification length]);
        MCNotificationPostLocalNotificationEvent(t_mc_reminder_text);
        // HSC-2012-03-13 [[ Bug 10076 ]] Prevent Push Notification crashing when applicationDidBecomeActive is called multiple times
        [m_pending_local_notification release];
        m_pending_local_notification = nil;
    }
    if (m_pending_push_notification != nil)
    {
        MCString t_mc_reminder_text = nil;
        t_mc_reminder_text.set ([m_pending_push_notification cStringUsingEncoding:NSMacOSRomanStringEncoding], [m_pending_push_notification length]);
        MCNotificationPostPushNotificationEvent(t_mc_reminder_text);
        // HSC-2012-03-13 [[ Bug 10076 ]] Prevent Push Notification crashing when applicationDidBecomeActive is called multiple times
        [m_pending_push_notification release];
        m_pending_push_notification = nil;
    }
    if (m_pending_launch_url == true && m_launch_url.getlength() > 1)
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
                assert(false);
                break;
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
	t_size = [[t_info objectForKey:UIKeyboardFrameBeginUserInfoKey] CGRectValue].size;

    // MM-2012-02-26: [[ Bug 10677 ]] Keyboard dimensions do not take into account orientation.
    //  We want height here, so assume the keyboard is always wider than it is taller and take the min of the two.
	MCIPhoneHandleKeyboardWillActivate(MCMin(t_size . height, t_size . width));
}

- (void)keyboardWillDeactivate:(NSNotification *)notification
{
	MCIPhoneHandleKeyboardWillDeactivate();
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

//////////

- (CGRect)fetchScreenBounds
{
	CGRect t_viewport;
	t_viewport = [[UIScreen mainScreen] bounds];
	
	if (UIInterfaceOrientationIsLandscape([self fetchOrientation]))
		return CGRectMake(0.0f, 0.0f, t_viewport . size . height, t_viewport . size . width);
	
	return t_viewport;
}

- (CGRect)fetchViewBounds
{
	CGRect t_viewport;
	t_viewport = [[UIScreen mainScreen] bounds];
	
	// MW-2011-10-24: [[ Bug ]] The status bar only clips the display if actually
	//   hidden, or on iPhone with black translucent style.
	CGFloat t_status_bar_size;
	if (m_status_bar_hidden || ([[UIDevice currentDevice] userInterfaceIdiom] == UIUserInterfaceIdiomPhone && m_status_bar_style == UIStatusBarStyleBlackTranslucent))
		t_status_bar_size = 0.0f;
	else
		t_status_bar_size = 20.0f;
	
	if (UIInterfaceOrientationIsLandscape([self fetchOrientation]))
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

- (const char *)fetchDeviceToken
{
    return m_device_token.getstring ();
}

- (const char *)fetchLaunchUrl
{
	return m_launch_url.getstring ();
}

- (void)activateKeyboard
{
	// If we are still starting up, then record the request.
	if (m_status == kMCIPhoneApplicationStatusStartup)
	{
		m_keyboard_activation_pending = true;
		return;
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
	[[[m_main_controller rootView] textView] setKeyboardType: p_new_type];
}

- (void)configureReturnKeyType: (UIReturnKeyType)p_new_type
{
	[[[m_main_controller rootView] textView] setReturnKeyType: p_new_type];
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
		[t_display_view setContentScaleFactor: p_new_scale];
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
	
	// We start off with no image.
	m_image_view = nil;
	m_image_orientation = UIInterfaceOrientationPortrait;
	
	// We always use full screen layout.
	[self setWantsFullScreenLayout: YES];
	
	return self;
}

- (void)dealloc
{
	// Release the image.
	[m_image_view release];
	
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

- (void)loadView
{
	MCLog("StartupViewController: loadView (%d)\n", [self interfaceOrientation]);
	
	// Create an empty view covering the application frame.
	UIView *t_view;
	t_view = [[UIView alloc] initWithFrame: [[UIScreen mainScreen] applicationFrame]];
	[t_view setContentMode: UIViewContentModeCenter];
	
	// We turn off user interaction as we don't need any events from the startup
	// image.
	[t_view setUserInteractionEnabled: NO];
	
	// Set the view, and release our reference to it - we can manipulate the empty
	// view using [self view];
	[self setView: t_view];
	[t_view release];
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
	
	// If we don't have an image view yet, make sure we get one. This can happen
	// when the device starts up in portrait - 'WillAnimate' will never be called.
	if (m_image_view == nil)
		[self switchImageToOrientation: [self interfaceOrientation]];
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
	
	// When the view disappears, get rid of the image view to save space.
	[m_image_view removeFromSuperview];
	[m_image_view release];
	m_image_view = nil;
}

- (void)willRotateToInterfaceOrientation:(UIInterfaceOrientation)toInterfaceOrientation duration:(NSTimeInterval)duration
{
	MCLog("StartupViewController: willRotateToInterfaceOrientation (%d)\n", toInterfaceOrientation);
	[[MCIPhoneApplication sharedApplication] beginAutorotation];
}

- (void)willAnimateRotationToInterfaceOrientation:(UIInterfaceOrientation)newInterfaceOrientation duration:(NSTimeInterval)duration
{
	MCLog("StartupViewController: willAnimateToInterfaceOrientation (%d)\n", newInterfaceOrientation);
	
	// We switch the image here, which should mean we get a nice smooth rotation
	// animation (this method is invoked within an animation block).
	[self switchImageToOrientation: newInterfaceOrientation];
}

- (void)didRotateFromInterfaceOrientation:(UIInterfaceOrientation)fromInterfaceOrientation
{
	MCLog("StartupViewController: didRotateFromInterfaceOrientation (%d)\n", fromInterfaceOrientation);
	[[MCIPhoneApplication sharedApplication] endAutorotation];
}

- (void)switchImageToOrientation: (UIInterfaceOrientation)p_new_orientation
{
	MCLog("StartupViewController: switchImageToOrientation (%d)\n", p_new_orientation);
	
	if (m_image_view != nil && m_image_orientation == p_new_orientation)
		return;
	
	// Remove any existing image view from the main view.
	if (m_image_view != nil)
	{
		[m_image_view removeFromSuperview];
		[m_image_view release];
		m_image_view = nil;
	}
	
	// Compute the list of image names (and rotations) to try in order.
	NSString *t_image_names[5];
	CGFloat t_image_angles[5];
	if ([[UIDevice currentDevice] userInterfaceIdiom] == UIUserInterfaceIdiomPad)
	{
		// MW-2011-10-18: [[ Bug 9823 ]] The S/B only ever has portrait / landscape
		//   images, so simplify the switch to handle that case.
		// On iPad we use and rotate the appropriate image.
		switch(p_new_orientation)
		{
			default:
			case UIInterfaceOrientationPortrait:
				t_image_names[0] = @"Default-Portrait.png";
				t_image_angles[0] = 0.0f;
				t_image_names[1] = @"Default.png";
				t_image_angles[1] = 0.0f;
				t_image_names[2] = nil;
				break;
			case UIInterfaceOrientationPortraitUpsideDown:
				t_image_names[0] = @"Default-Portrait.png";
				t_image_angles[0] = 0.0f;
				t_image_names[1] = @"Default.png";
				t_image_angles[1] = 0.0f;
				t_image_names[2] = nil;
				break;
			case UIInterfaceOrientationLandscapeLeft:
				t_image_names[0] = @"Default-Landscape.png";
				t_image_angles[0] = 0.0f;
				t_image_names[1] = @"Default.png";
				t_image_angles[1] = -90.0f;
				t_image_names[2] = nil;
				break;
			case UIInterfaceOrientationLandscapeRight:
				t_image_names[0] = @"Default-Landscape.png";
				t_image_angles[0] = 0.0f;
				t_image_names[1] = @"Default.png";
				t_image_angles[1] = -90.0f;
				t_image_names[2] = nil;
				break;
		}
	}
	else
	{
		// On iPhone there is only ever a 'Default' image, which we must
		// rotate appropriately since the screen could be in any orientation.
        // MM-2012-10-08: [[ Bug 10448 ]] Make sure the startup view uses the 568px tall splash screen for 4 inch devices.
		if ([[UIScreen mainScreen] bounds] . size . height == 568)
        {
            t_image_names[0] = @"Default-568h@2x.png";
            t_image_names[1] = nil;
        }
        else
        {
            t_image_names[0] = @"Default.png";
            t_image_names[1] = nil;
        }
        
		switch(p_new_orientation)
		{
			default:
			case UIInterfaceOrientationPortrait:
				t_image_angles[0] = 0.0f;
				break;
			case UIInterfaceOrientationPortraitUpsideDown:
				t_image_angles[0] = 180.0f;
				break;
			case UIInterfaceOrientationLandscapeLeft:
				t_image_angles[0] = 90.0f;
				break;
			case UIInterfaceOrientationLandscapeRight:
				t_image_angles[0] = 270.0f;
				break;
		}
	}
	
	
	// Loop through the image names until we succeed.
	UIImage *t_image;
	CGFloat t_angle;
	t_image = nil;
	t_angle = 0.0f;
	for(int i = 0; t_image_names[i] != nil; i++)
	{
		t_image = [UIImage imageWithContentsOfFile: [NSString stringWithFormat: @"%@/%@", [[NSBundle mainBundle] bundlePath], t_image_names[i]]];
		if (t_image != nil)
		{
			MCLog("StartupViewController:   using image '%@' with angle %f\n", t_image_names[i], t_image_angles[i]);
			t_angle = t_image_angles[i];
			break;
		}
	}
	
	// We have an image, and an angle so create and set our image view.
	m_image_view = [[UIImageView alloc] initWithImage: t_image];
	
	// Rotate the image view appropriately.
	[m_image_view setTransform: CGAffineTransformMakeRotation(t_angle * M_PI / 180.0f)];
	
	// Get the bounds of the screen, for placement.
	CGRect t_screen_bounds;
	t_screen_bounds = [[MCIPhoneApplication sharedApplication] fetchScreenBounds];
	
	// Center the image in the screen.
	[m_image_view setCenter: CGPointMake(t_screen_bounds . size . width / 2.0f, t_screen_bounds . size . height / 2.0f)];
	
	// Insert the image view into our view.
	[[self view] addSubview: m_image_view];
	
	// Store the orientation of the image to stop unnecessary switches.
	m_image_orientation = p_new_orientation;
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

@end

////////////////////////////////////////////////////////////////////////////////

MCIPhoneApplication *MCIPhoneGetApplication(void)
{
	return [MCIPhoneApplication sharedApplication];
}

UIView *MCIPhoneGetView(void)
{
	return [[MCIPhoneApplication sharedApplication] fetchMainView];
}

UIView *MCIPhoneGetRootView(void)
{
	return [[MCIPhoneApplication sharedApplication] fetchRootView];
}

UIView *MCIPhoneGetDisplayView(void)
{
	return [MCIPhoneGetRootView() displayView];
}

UIViewController *MCIPhoneGetViewController(void)
{
	return [[MCIPhoneApplication sharedApplication] fetchMainViewController];
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

void MCIPhoneActivateKeyboard(void)
{
	[[MCIPhoneApplication sharedApplication] activateKeyboard];
}

void MCIPhoneDeactivateKeyboard(void)
{
	[[MCIPhoneApplication sharedApplication] deactivateKeyboard];
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

int main(int argc, char *argv[], char *envp[])
{
#if defined(_DEBUG) && defined(_VALGRIND)
	if (argc < 2 ||  (argc >= 2 && strcmp(argv[1], "-valgrind") != 0))
	{
        execl(VALGRIND, VALGRIND, "--dsymutil=yes", "--leak-check=full", argv[0], "-valgrind", NULL);
		exit(-1);
	}
#endif
	
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
