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

#import <UIKit/UIKit.h>

extern "C" UIView *LiveCodeGetView(void);

////////////////////////////////////////////////////////////////////////////////

@interface LiveCodeDelegate : NSObject

- mouseUp: (NSArray *)args;

@end

@implementation LiveCodeDelegate

- (void)mouseUpWithButton: (NSNumber*)button
{
	NSLog(@"Got mouse up - %@", button);
}

- (void)mouseMovedTo: (int)x comma: (int)y;
{
	NSLog(@"Got mouse move - %d, %d", x, y);
}

@end

////////////////////////////////////////////////////////////////////////////////

@interface LiveCodeEmbeddedApplication : NSObject <UIApplicationDelegate>
{
	UIApplication *m_application;
	UIWindow *m_window;
	UIViewController *m_controller;
	UIView *m_view;
	UIView *m_livecode_view;
}
@end

@implementation LiveCodeEmbeddedApplication

- (id)init
{
	self = [super init];
	if (self == nil)
		return nil;
	
	return self;
}

- (void)dealloc
{
	[super dealloc];
}

- (BOOL)application:(UIApplication *)p_application didFinishLaunchingWithOptions:(NSDictionary *)p_options
{
	m_application = p_application;
	
	m_window = [[UIWindow alloc] initWithFrame: [[UIScreen mainScreen] bounds]];
	[m_window setBackgroundColor: [UIColor redColor]];
	
	m_controller = [[UIViewController alloc] init];
	
	m_view = [[UIView alloc] initWithFrame: CGRectMake(0.0, 0.0, 768.0, 1004.0)];
	
	m_livecode_view = LiveCodeGetView();
	[m_livecode_view setViewBounds: CGRectMake(40.0, 40.0, 400.0, 400.0)];
	[m_livecode_view setDelegate: [[LiveCodeDelegate alloc] init]];
	[m_livecode_view mapMessage: "mouseUp" withSignature: "v#N" toSelector: @selector(mouseUpWithButton:)];
	[m_livecode_view mapMessage: "mouseMove" withSignature: "vii" toSelector: @selector(mouseMovedTo:comma:)];
	[m_view addSubview: m_livecode_view];
	
	UIButton *t_button;
	t_button = [UIButton buttonWithType: UIButtonTypeRoundedRect];
	[t_button setFrame: CGRectMake(8, 8, 80, 40)];
	[t_button addTarget: self action: @selector(helloWorld) forControlEvents: UIControlEventTouchUpInside];
	[m_view addSubview: t_button];
	
	[m_controller setView: m_view];

	[m_window setRootViewController: m_controller];

	return YES;
}

- (void)applicationDidBecomeActive:(UIApplication *)p_application
{
	[m_livecode_view startup];
	[m_window makeKeyAndVisible];
}

- (void)applicationWillTerminate:(UIApplication *)application
{
	[m_livecode_view shutdown];
}

- (void)applicationDidReceiveMemoryWarning:(UIApplication *)application
{
}

- (void)helloWorld
{
	[m_livecode_view setHidden: ![m_livecode_view isHidden]];
	[m_livecode_view post: @"helloWord"];
}

@end

////////////////////////////////////////////////////////////////////////////////

int platform_main(int argc, char *argv[], char *envp[])
{
	int t_exit_code;
	
	NSAutoreleasePool *t_pool;
	t_pool = [[NSAutoreleasePool alloc] init];
	
	t_exit_code = UIApplicationMain(argc, argv, nil, @"LiveCodeEmbeddedApplication");
	
	[t_pool release];
	
	return t_exit_code;
}

////////////////////////////////////////////////////////////////////////////////
