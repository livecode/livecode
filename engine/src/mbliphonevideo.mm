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

#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"

#include "mcerror.h"

#include "exec.h"
#include "printer.h"
#include "globals.h"
#include "dispatch.h"
#include "stack.h"
#include "image.h"
#include "player.h"
#include "param.h"
#include "eventqueue.h"
#include "osspec.h"

#include "mblevent.h"
#include "mbliphoneapp.h"

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>
#import <MediaPlayer/MPMoviePlayerController.h>
#import <MediaPlayer/MPMoviePlayerViewController.h>

////////////////////////////////////////////////////////////////////////////////

UIViewController *MCIPhoneGetViewController(void);
UIView *MCIPhoneGetView(void);

////////////////////////////////////////////////////////////////////////////////

bool g_movie_player_in_use = false;
MPMoviePlayerViewController *g_movie_player = nil;

////////////////////////////////////////////////////////////////////////////////

static NSObject *s_movie_player_delegate = nil;

////////////////////////////////////////////////////////////////////////////////

@interface com_runrev_livecode_MCFullscreenMovieDelegate : NSObject
{
	bool m_running;
	UIControl *m_overlay;
}

- (id)initWithPlayer;
- (void)dealloc;

- (void)begin: (bool)p_add_overlay;
- (void)end;
- (void)stop;
- (bool)isRunning;

- (void)movieFinished: (NSNotification *) p_notification;
- (void)moviePreloadFinished: (NSNotification *) p_notification;
- (void)movieWindowTouched: (UIControl*) p_sender;

@end

@implementation com_runrev_livecode_MCFullscreenMovieDelegate

// AL-2014-09-09: [[ Bug 13354 ]] Replace deprecated MPMoviePlayerContentPreloadDidFinishNotification
- (id)initWithPlayer: (MPMoviePlayerController *)p_player
{
	self = [super init];
	if (self == nil)
		return nil;
	
	[[NSNotificationCenter defaultCenter] addObserver:self 
											 selector:@selector(movieFinished:)
												 name:MPMoviePlayerPlaybackDidFinishNotification 
											   object:nil];

	[[NSNotificationCenter defaultCenter] addObserver:self 
											 selector:@selector(moviePreloadFinished:)
												 name:MPMoviePlayerLoadStateDidChangeNotification 
											   object:p_player];
	
	m_running = true;
	m_overlay = nil;
	
	s_movie_player_delegate = self;
	
	return self;
}

- (void)dealloc
{
	s_movie_player_delegate = nil;
	[[NSNotificationCenter defaultCenter] removeObserver: self];
	[super dealloc];
}

//////////

- (void)begin: (bool)p_with_overlay
{
	if (p_with_overlay)
	{
		UIWindow *t_window = [[UIApplication sharedApplication] keyWindow];
		m_overlay = [[UIControl alloc] initWithFrame: [t_window frame]];
		[m_overlay addTarget: self action: @selector(movieWindowTouched:) forControlEvents: UIControlEventTouchDown];
		[t_window addSubview: m_overlay];
	}
}

- (void)end
{
	if (m_overlay != nil)
	{
		[m_overlay removeTarget: self action: @selector(movieWindowTouched:) forControlEvents: UIControlEventTouchDown];
		[m_overlay removeFromSuperview];
		[m_overlay release];
	}
}

- (void)stop
{
	m_running = false;
	
	// MW-2011-08-16: [[ Wait ]] Tell the wait to exit (our wait has anyevent == True).
	MCscreen -> pingwait();
}

- (bool)isRunning
{
	return m_running;
}

//////////

- (void)movieFinished: (NSNotification *) p_notification
{
	m_running = false;
	
	// MW-2011-08-16: [[ Wait ]] Tell the wait to exit (our wait has anyevent == True).
	MCscreen -> pingwait();
}

- (void)moviePreloadFinished: (NSNotification *) p_notification
{
	// AL-2014-09-09: [[ Bug 13354 ]] Replace deprecated MPMoviePlayerContentPreloadDidFinishNotification
	if ([[p_notification object] loadState] & MPMovieLoadStateUnknown)
	{
		m_running = false;
		
		// MW-2011-08-16: [[ Wait ]] Tell the wait to exit (our wait has anyevent == True).
		MCscreen -> pingwait();
	}
}

- (void)movieWindowTouched: (UIControl*) p_sender
{
	extern MCExecContext *MCECptr;
	MCEventQueuePostCustom(new MCMovieTouchedEvent(MCECptr -> GetObject()));
}

@end

static void configure_playback_range(MPMoviePlayerController *p_player)
{
	if (!MCtemplateplayer -> getflag(F_PLAY_SELECTION))
		return;
	
	uint32_t t_start_time, t_end_time;
	t_start_time = MCtemplateplayer -> getstarttime();
	t_end_time = MCtemplateplayer -> getendtime();
	[p_player setInitialPlaybackTime: t_start_time / 1000.0];
	[p_player setEndPlaybackTime: t_end_time / 1000.0];
}

struct play_fullscreen_t
{
	NSURL *url;
	bool looping;
	bool with_controller;
	MPMoviePlayerController *movie_player;
	com_runrev_livecode_MCFullscreenMovieDelegate *delegate;
};

static void play_fullscreen_movie_prewait(void *p_context)
{
	play_fullscreen_t *ctxt;
	ctxt = (play_fullscreen_t *)p_context;
	
    // MM-2011-12-09: [[ Bug 9892 ]] Destroy previous movie player.  Fixes bug with iOS 5 where 
	//		showController is ignored on second running.  Since g_movie_player is only used for native players
    //      in iOS 4.2 or earlier, we can use g_movie_player_in_use to make sure we aren't releasing
    //      a controller that is already in use.
    if (g_movie_player != nil)
	{
        [g_movie_player release];
        g_movie_player = nil;
    }
    
	// Make sure we have a movie player view controller with correct url
	if (g_movie_player == nil)	
		g_movie_player = [[MPMoviePlayerViewController alloc] initWithContentURL: ctxt -> url];
	else
		[[g_movie_player moviePlayer] setContentURL: ctxt -> url];
	
	g_movie_player_in_use = true;
	
	ctxt -> movie_player = [g_movie_player moviePlayer];
	
	[[ctxt -> movie_player view] setHidden: NO];
	[[ctxt -> movie_player view] setOpaque: YES];
	[[ctxt -> movie_player view] setAlpha: 1.0f];
	[[ctxt -> movie_player backgroundView] setBackgroundColor: [UIColor blackColor]];
	[ctxt -> movie_player setFullscreen: YES];
	[ctxt -> movie_player setScalingMode: MPMovieScalingModeAspectFit];
	[ctxt -> movie_player setControlStyle: (ctxt -> with_controller ? MPMovieControlStyleFullscreen : MPMovieControlStyleNone)];
	[ctxt -> movie_player setUseApplicationAudioSession: YES];
	[ctxt -> movie_player setInitialPlaybackTime: -1];
	[ctxt -> movie_player setEndPlaybackTime: -1];
	[ctxt -> movie_player setShouldAutoplay: YES];
	if (ctxt -> looping)
		[ctxt -> movie_player setRepeatMode: MPMovieRepeatModeOne];
	if (MCmajorosversion >= 430)
		[ctxt -> movie_player setAllowsAirPlay: NO];
	
	configure_playback_range(ctxt -> movie_player);
	
	ctxt -> delegate = [[com_runrev_livecode_MCFullscreenMovieDelegate alloc] initWithPlayer:ctxt -> movie_player];
	
	// Present the view controller and get the delegate to setup its overlay
	// if needed.
	[MCIPhoneGetViewController() presentModalViewController: g_movie_player animated: NO];
	[ctxt -> delegate begin: !ctxt -> with_controller];	
}

static void play_fullscreen_movie_postwait(void *p_context)
{
	play_fullscreen_t *ctxt;
	ctxt = (play_fullscreen_t *)p_context;
	
	// Clear up and overlay and dismiss the controller.
	[ctxt -> delegate end];
	[MCIPhoneGetViewController() dismissModalViewControllerAnimated: NO];
	
	// Cleanup the delegate
	[ctxt -> delegate release];
	
	// Make sure we reset the movie player to nothing.
	[ctxt -> movie_player stop];
	[ctxt -> movie_player setContentURL: [NSURL URLWithString: @""]];
	
	g_movie_player_in_use = false;
}

static bool play_fullscreen_movie_new(NSURL *p_movie, bool p_looping, bool p_with_controller)
{
	// If on iOS < 4.2 don't allow this if there is a native player control
	if (MCmajorosversion < 420 && g_movie_player_in_use)
		return false;
	
	// Don't allow nested play calls
	if (s_movie_player_delegate != nil)
		return false;
	
	play_fullscreen_t ctxt;
	ctxt . url = p_movie;
	ctxt . looping = p_looping;
	ctxt . with_controller = p_with_controller;
	
	MCIPhoneRunOnMainFiber(play_fullscreen_movie_prewait, &ctxt);
	
	while([ctxt . delegate isRunning])
		MCscreen -> wait(60.0, True, True);

	MCIPhoneRunOnMainFiber(play_fullscreen_movie_postwait, &ctxt);
	
	return true;
}

////////////////////////////////////////////////////////////////////////////////

bool MCSystemPlayVideo(MCStringRef p_video)
{
	// MW-2010-06-22: Add support for streaming video by detecting a URL rather
	//   than a file;
	NSURL *t_url;
	t_url = nil;
	if (MCStringBeginsWithCString(p_video, (const char_t *)"http://", kMCStringOptionCompareExact)
		|| MCStringBeginsWithCString(p_video, (const char_t *)"https://", kMCStringOptionCompareExact))
	{
		CFStringRef cfstrurl = nil;
		/* UNCHECKED */ MCStringConvertToCFStringRef(p_video, cfstrurl);
		t_url = [NSURL URLWithString: (NSString *)cfstrurl];
		CFRelease(cfstrurl);
	}
	else
	{
		MCAutoStringRef t_path;
		CFStringRef cfstrpath = nil;
		/* UNCHECKED */ MCS_resolvepath(p_video, &t_path);
		/* UNCHECKED */ MCStringConvertToCFStringRef(*t_path, cfstrpath);
		t_url = [NSURL fileURLWithPath: (NSString *)cfstrpath];
		CFRelease(cfstrpath);
	}
	
	if (t_url == nil)
		return false;
	
	bool t_result;
	t_result = play_fullscreen_movie_new(t_url, MCtemplateplayer -> getflag(F_LOOPING) == True, MCtemplateplayer -> getflag(F_SHOW_CONTROLLER) == True);
	
	return t_result;
}

bool MCSystemStopVideo(void)
{
	if (s_movie_player_delegate != nil)
		[s_movie_player_delegate stop];
	
	return true;
}

////////////////////////////////////////////////////////////////////////////////
