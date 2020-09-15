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

#ifndef __MBL_IPHONE_APP__
#define __MBL_IPHONE_APP__

////////////////////////////////////////////////////////////////////////////////

#import <UIKit/UIKit.h>

////////////////////////////////////////////////////////////////////////////////

@class com_runrev_livecode_MCIPhoneApplication;
#define MCIPhoneApplication com_runrev_livecode_MCIPhoneApplication

@class com_runrev_livecode_MCIPhoneWindow;
#define MCIPhoneWindow com_runrev_livecode_MCIPhoneWindow

@class com_runrev_livecode_MCIPhoneStartupViewController;
#define MCIPhoneStartupViewController com_runrev_livecode_MCIPhoneStartupViewController

@class com_runrev_livecode_MCIPhoneMainViewController;
#define MCIPhoneMainViewController com_runrev_livecode_MCIPhoneMainViewController

@class com_runrev_livecode_MCIPhoneRootView;
#define MCIPhoneRootView com_runrev_livecode_MCIPhoneRootView

@class com_runrev_livecode_MCIPhoneMainView;
#define MCIPhoneMainView com_runrev_livecode_MCIPhoneMainView

@class com_runrev_livecode_MCIPhoneDisplayView;
#define MCIPhoneDisplayView com_runrev_livecode_MCIPhoneDisplayView

@class com_runrev_livecode_MCIPhoneUIKitDisplayView;
#define MCIPhoneUIKitDisplayView com_runrev_livecode_MCIPhoneUIKitDisplayView

@class com_runrev_livecode_MCIPhoneOpenGLDisplayView;
#define MCIPhoneOpenGLDisplayView com_runrev_livecode_MCIPhoneOpenGLDisplayView

////////////////////////////////////////////////////////////////////////////////

// The state the application is currently in.
enum MCIPhoneApplicationStatus
{
	// The application is starting up and currently displaying the launch image.
	kMCIPhoneApplicationStatusStartup,
	// The application is preparing to display the main app screen, no more
	// initial autorotation should occur.
	kMCIPhoneApplicationStatusPrepare,
	// The application is now executing the main app, notifications are enabled
	// and autorotation will occur after notification.
	kMCIPhoneApplicationStatusExecute,
	// The application is now in the process of shutting down.
	kMCIPhoneApplicationStatusShutdown,
};

enum MCIPhoneKeyboardDisplayMode
{
    kMCIPhoneKeyboardDisplayModeOver,
    kMCIPhoneKeyboardDisplayModePan,
};

@interface MCIPhoneApplication : NSObject <UIApplicationDelegate>
{
	// The actual application object (remember this object is only a delegate!).
	UIApplication *m_application;
  	// The main window of our application.
	MCIPhoneWindow *m_window;
	// The display link to use.
	CADisplayLink *m_display_link;

	// The state the application is currently in.
	MCIPhoneApplicationStatus m_status;
	
	// The status bar style and visibility to use for frame computations - i.e. that
	// the app wants.
	UIStatusBarStyle m_status_bar_style;
	BOOL m_status_bar_hidden;
    BOOL m_status_bar_solid;
	
	// The startup controller is that which is displayed on startup. This is
	// presented as a modal over the main controller until the app starts
	// executing.
	MCIPhoneStartupViewController *m_startup_controller;
	
	// This is the current main view controller of our application.
	MCIPhoneMainViewController *m_main_controller;
		
	// The pending orientation notification.
	NSNotification *m_pending_orientation_notification;
	// The orientation of the most recent notifications
	UIInterfaceOrientation m_pending_orientation;
	// The set of allowed orientations.
	uint32_t m_allowed_orientations;
	// The orientation lock count.
	uint32_t m_orientation_lock;
	
	// The status bar is currently animating.
	bool m_in_autorotation : 1;
	// A switch of the controller is currently pending.
	bool m_prepare_status_pending : 1;
	// We are waiting for an orientation event to complete.
	bool m_in_orientation_changed : 1;
	// A request to activate the keyboard was made before the main controller
	// was displaed.
	bool m_keyboard_activation_pending : 1;
	// An orientation changed request was received, but not yet acted on.
	bool m_orientation_changed_pending : 1;
    
    bool m_keyboard_is_visible : 1;

    bool m_is_remote_notification :1;

    // We store the payload from a pending local notification here until the stack has become active and is ready to receive the message with the data.
    NSString *m_pending_local_notification;
    // We store the payload from a pending push notification here until the stack has become active and is ready to receive the message with the data.
    NSString *m_pending_push_notification;
    // We store the device token for push notification here
    MCStringRef m_device_token;
    // We store the wakeup token for custom URL schemes here
    MCStringRef m_launch_url;
    // HSC-2012-03-13 [[ Bug 10076 ]] Prevent Push Notification crashing when applicationDidBecomeActive is called multiple times
    // We need to know if there is a pending launch url that was received.
    // We do not want to send a URL message again, should the app become active again.
    bool m_pending_launch_url;
    // We need to know if the application is active before we can send a message
    bool m_did_become_active;

    MCIPhoneKeyboardDisplayMode m_keyboard_display;
    UIKeyboardType m_keyboard_type;
    UIReturnKeyType m_return_key_type;
}

//////////

+ (MCIPhoneApplication *)sharedApplication;

//////////

- (id)init;
- (void)dealloc;

//////////

// MM-2014-09-30: [[ iOS 8 Support ]] Method called after successully registering (push) notification settings.
#ifdef __IPHONE_8_0
- (void)application: (UIApplication *)application didRegisterUserNotificationSettings: (UIUserNotificationSettings *)notificationSettings;
#endif

//- (void)applicationDidFinishLaunching:(UIApplication *)application;
- (BOOL)application:(UIApplication *)p_application didFinishLaunchingWithOptions:(NSDictionary *)p_launchOptions;
- (void)application:(UIApplication *)p_application didReceiveLocalNotification:(UILocalNotification *)p_notification;
- (void)applicationDidBecomeActive:(UIApplication *)application;
- (void)applicationWillResignActive:(UIApplication *)application;
- (void)applicationWillTerminate:(UIApplication *)application;
- (void)applicationDidReceiveMemoryWarning:(UIApplication *)application;

- (NSUInteger)application:(UIApplication *)application supportedInterfaceOrientationsForWindow:(UIWindow *)window;

//////////

- (void)orientationChanged:(NSNotification *)notification;
- (void)keyboardWillActivate:(NSNotification *)notification;
- (void)keyboardWillDeactivate:(NSNotification *)notification;
- (void)keyboardDidActivate:(NSNotification *)notification;
- (void)keyboardDidDeactivate:(NSNotification *)notification;

//////////

- (void)applicationProtectedDataWillBecomeUnavailable:(UIApplication *)application;
- (void)applicationProtectedDataDidBecomeAvailable:(UIApplication *)application;

// Returns the current application status.
- (MCIPhoneApplicationStatus)status;

// Start preparing for application execution.
- (void)startPreparing;
// Start application execution.
- (void)startExecuting;

// Switch the style of the status bar to 'newStyle'.
- (void)switchToStatusBarStyle: (UIStatusBarStyle)newStyle;
// Switch the visibility of the status bar to 'newVisible'.
- (void)switchToStatusBarVisibility: (BOOL)newVisibile;
// If statusbarstyle=solid, we move stack down to 20pixels
- (void)setStatusBarSolid: (BOOL)p_is_solid;

// Returns the current screen bounds in logical units - taking into account the
// current orientation.
- (CGRect)fetchScreenBounds;
// Returns the current view bounds in logical units - taking into account the
// current orientation and status bar state.
- (CGRect)fetchViewBounds;
// Returns the current orientation of the screen.
- (UIInterfaceOrientation)fetchOrientation;

// Returns the root view.
- (MCIPhoneRootView *)fetchRootView;
// Returns the main view.
- (MCIPhoneMainView *)fetchMainView;
// Returns the view displaying content.
- (MCIPhoneDisplayView *)fetchDisplayView;
// Returns the main view controller.
- (UIViewController *)fetchMainViewController;
// MM-2014-10-15: [[ Bug 13665 ]] Returns the currently active view controller.
- (UIViewController *)fetchCurrentViewController;
// Returns the device token that is used for push notificaiton.
- (MCStringRef)fetchDeviceToken;
// Returns the URL from which the device was launched.
- (MCStringRef)fetchLaunchUrl;

// Start an autorotation operation
- (void)beginAutorotation;
// Finish an autorotation operation
- (void)endAutorotation;

// Activate the keyboard.
- (void)activateKeyboard;
// Deactivate the keyboard.
- (void)deactivateKeyboard;
// Fetch the current type of keyboard that will be displayed.
- (UIKeyboardType)fetchKeyboardType;
// Configure the current type of keyboard.
- (void)configureKeyboardType: (UIKeyboardType)newKeyboardType;
// Configure the type of return key.
- (void)configureReturnKeyType: (UIReturnKeyType)newReturnKeyType;
// Configure the keyboard display.
- (void)configureKeyboardDisplay: (MCIPhoneKeyboardDisplayMode)p_mode;

// Get the set of allowed orientations.
- (uint32_t)allowedOrientations;
// Set the set of allowed orientations.
- (void)setAllowedOrientations: (uint32_t)allowedOrientations;
// Lock orientation changes.
- (void)lockOrientation;
// Unlock orientation changes.
- (void)unlockOrientation;
// Returns if orientation is currently locked.
- (BOOL)orientationLocked;
// Force a sync orientation if necessary and appropriate.
- (void)forceSyncOrientation;
// Make sure that a suitable orientation request is dispatched if
// necessary and appropriate.
- (void)syncOrientation;
// Called when an orientation changed event has been processed, causing the
// current pending orientation to be committed, if possible.
- (void)commitOrientation;

// Turns on or off the 'display link' feature.
- (void)setRedrawInterval: (uint32_t)interval;
// Temporarily disables the 'display link' feature.
- (void)disableRedrawInterval;
// Temporarily enables the 'display link' feature.
- (void)enableRedrawInterval;
// Invoked by the display link to update.
- (void)performRedraw: (CADisplayLink *)sender;

// Turns on or off autorotation animation.
- (void)setAnimateAutorotation: (bool)animate;

// Change the main view to use the UIKit presentation method.
- (void)switchToUIKitDisplay;
// Change the main view to use the OpenGL presentation method.
- (void)switchToOpenGLDisplay;

// Configure the content scale factor (ignored unless OpenGL mode)
- (void)configureContentScale: (float)scale;

@end

////////////////////////////////////////////////////////////////////////////////

@interface MCIPhoneWindow : UIWindow
- (void)motionBegan: (UIEventSubtype)motion withEvent: (UIEvent *)event;
- (void)motionCancelled: (UIEventSubtype)motion withEvent: (UIEvent *)event;
- (void)motionEnded:(UIEventSubtype)motion withEvent: (UIEvent *)event;
- (void)remoteControlReceivedWithEvent:(UIEvent *)event;
@end

////////////////////////////////////////////////////////////////////////////////

@interface MCIPhoneStartupViewController : UIViewController
{
	// The orientation of the current image view.
	UIInterfaceOrientation m_image_orientation;
}

- (id)init;
- (void)dealloc;

- (BOOL)shouldAutorotate;
- (NSUInteger)supportedInterfaceOrientations;
- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)toInterfaceOrientation;

- (void)viewDidLoad;
- (void)viewDidUnload;

- (void)viewWillAppear:(BOOL)animated;
- (void)viewDidAppear:(BOOL)animated;
- (void)viewWillDisappear:(BOOL)animated;
- (void)viewDidDisappear:(BOOL)animated;

- (void)willRotateToInterfaceOrientation:(UIInterfaceOrientation)toInterfaceOrientation duration:(NSTimeInterval)duration;
- (void)willAnimateRotationToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation duration:(NSTimeInterval)duration;
- (void)didRotateFromInterfaceOrientation:(UIInterfaceOrientation)fromInterfaceOrientation;

@end

////////////////////////////////////////////////////////////////////////////////

@interface MCIPhoneMainViewController : UIViewController
{
	// The current orientation of the view controller.
	UIInterfaceOrientation m_current_orientation;
    UIInterfaceOrientation m_pending_orientation;
	
	// The main view.
	MCIPhoneRootView *m_root_view;
}

- (id)init;

- (MCIPhoneRootView *)rootView;

- (UIInterfaceOrientation)pendingOrientation;
- (void)setPendingOrientation: (UIInterfaceOrientation)newOrientation;

- (BOOL)shouldAutorotate;
- (NSUInteger)supportedInterfaceOrientations;
- (BOOL)shouldAutorotateToInterfaceOrientation:(UIInterfaceOrientation)toInterfaceOrientation;

- (void)loadView;

- (void)viewDidLoad;
- (void)viewDidUnload;

- (void)viewWillAppear:(BOOL)animated;
- (void)viewDidAppear:(BOOL)animated;
- (void)viewWillDisappear:(BOOL)animated;
- (void)viewDidDisappear:(BOOL)animated;

- (void)willRotateToInterfaceOrientation:(UIInterfaceOrientation)toInterfaceOrientation duration:(NSTimeInterval)duration;
- (void)willAnimateRotationToInterfaceOrientation:(UIInterfaceOrientation)interfaceOrientation duration:(NSTimeInterval)duration;
- (void)didRotateFromInterfaceOrientation:(UIInterfaceOrientation)fromInterfaceOrientation;

@end

////////////////////////////////////////////////////////////////////////////////

void MCIPhoneHandleDidBecomeActive(void);
void MCIPhoneHandleWillTerminate(void);
void MCIPhoneHandleDidReceiveMemoryWarning(void);
void MCIPhoneHandleDidStartPreparing(void);
void MCIPhoneHandleDidStartExecuting(void);
void MCIPhoneHandleOrientationChanged(void);
void MCIPhoneHandleViewBoundsChanged(void);
void MCIPhoneHandleTouches(UIView *p_view, NSSet *touches, UITouchPhase phase);
void MCIPhoneHandleBeginTextInput(void);
void MCIPhoneHandleEndTextInput(void);
void MCIPhoneHandleProcessTextInput(uint32_t t_char_code, uint32_t t_key_code);
void MCIPhoneHandleMotionBegan(UIEventSubtype motion, NSTimeInterval timestamp);
void MCIPhoneHandleMotionCancelled(UIEventSubtype motion, NSTimeInterval timestamp);
void MCIPhoneHandleMotionEnded(UIEventSubtype motion, NSTimeInterval timestamp);
void MCIPhoneHandleRemoteControlEvent(UIEventSubtype event, NSTimeInterval timestamp);
void MCIPhoneHandleKeyboardWillActivate(float height);
void MCIPhoneHandleKeyboardWillDeactivate(void);
void MCIPhoneHandlePerformRedraw(void);

////////////////////////////////////////////////////////////////////////////////

MCIPhoneApplication *MCIPhoneGetApplication(void);
NSString* MCIPhoneGetDeviceModelName(void);

UIViewController *MCIPhoneGetViewController(void);
UIView *MCIPhoneGetView(void);
UIView *MCIPhoneGetRootView(void);
UIView *MCIPhoneGetDisplayView(void);
CGRect MCIPhoneGetViewBounds(void);
CGRect MCIPhoneGetScreenBounds(void);
void MCIPhoneActivateKeyboard(void);
void MCIPhoneDeactivateKeyboard(void);
bool MCIPhoneIsKeyboardVisible(void);
void MCIPhoneConfigureContentScale(int32_t scale);
void MCIPhoneSwitchViewToUIKit(void);
void MCIPhoneSwitchViewToOpenGL(void);
UIKeyboardType MCIPhoneGetKeyboardType(void);
void MCIPhoneSetKeyboardType(UIKeyboardType type);
void MCIPhoneSetReturnKeyType(UIReturnKeyType type);
void MCIPhoneSetKeyboardDisplay(MCIPhoneKeyboardDisplayMode p_mode);
UIInterfaceOrientation MCIPhoneGetOrientation(void);
bool MCIPhoneIsEmbedded(void);
void MCIPhoneBreakWait(void);

MCRectangle MCDeviceRectFromLogicalCGRect(const CGRect cgrect);
CGRect MCUserRectToLogicalCGRect(const MCRectangle rect);

////////////////////////////////////////////////////////////////////////////////

// MW-2012-08-06: [[ Fibers ]] New methods for executing code on the main fiber.
void MCIPhoneCallSelectorOnMainFiber(id object, SEL selector);
void MCIPhoneCallSelectorOnMainFiberWithObject(id object, SEL selector, id arg);
void MCIPhoneRunOnScriptFiber(void (*)(void *), void *);
void MCIPhoneRunOnMainFiber(void (*)(void *), void *);
void MCIPhoneRunBlockOnScriptFiber(void (^)(void));
void MCIPhoneRunBlockOnMainFiber(void (^)(void));

////////////////////////////////////////////////////////////////////////////////

#endif
