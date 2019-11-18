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
#import <AVFoundation/AVFoundation.h>
#import <AVKit/AVKit.h>

////////////////////////////////////////////////////////////////////////////////

#if __IPHONE_OS_VERSION_MAX_ALLOWED < 100000
	typedef NSString *NSKeyValueChangeKey;
#endif

////////////////////////////////////////////////////////////////////////////////

UIViewController *MCIPhoneGetViewController(void);
UIView *MCIPhoneGetView(void);

////////////////////////////////////////////////////////////////////////////////

static AVPlayerViewController *s_movie_player = nil;

////////////////////////////////////////////////////////////////////////////////

@interface com_runrev_livecode_MCFullscreenMovieDelegate : NSObject
{
	bool m_running;
	bool m_looping;
	CMTime m_start_time;
	AVPlayer *m_player;
	UIControl *m_overlay;
}

- (id)initWithPlayer:(AVPlayer*)player startTime:(CMTime)startTime looping:(BOOL)looping;
- (void)dealloc;

- (void)begin: (bool)p_add_overlay;
- (void)end;
- (void)stop;
- (bool)isRunning;

- (void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary<NSKeyValueChangeKey,id> *)change context:(void *)context;
- (void)playerItemDidPlayToEndTime:(NSNotification*)notification;
- (void)playerItemFailedToPlayToEndTime:(NSNotification*)notification;
- (void)movieWindowTouched: (UIControl*) p_sender;

@end

@compatibility_alias MCFullscreenMovieDelegate com_runrev_livecode_MCFullscreenMovieDelegate;

////////////////////////////////////////////////////////////////////////////////

static MCFullscreenMovieDelegate *s_movie_player_delegate = nil;

////////////////////////////////////////////////////////////////////////////////

@implementation com_runrev_livecode_MCFullscreenMovieDelegate

// AL-2014-09-09: [[ Bug 13354 ]] Replace deprecated MPMoviePlayerContentPreloadDidFinishNotification
- (id)initWithPlayer:(AVPlayer *)p_player startTime:(CMTime)startTime looping:(BOOL)looping
{
	self = [super init];
	if (self == nil)
		return nil;
	
	m_player = [p_player retain];
	
	[m_player.currentItem
		addObserver:self
		forKeyPath:@"status"
		options:NSKeyValueObservingOptionNew | NSKeyValueObservingOptionOld
		context:nil];
	[NSNotificationCenter.defaultCenter
		addObserver:self
		selector:@selector(playerItemDidPlayToEndTime:)
		name:AVPlayerItemDidPlayToEndTimeNotification
		object:m_player.currentItem];
	[NSNotificationCenter.defaultCenter
		addObserver:self
		selector:@selector(playerItemFailedToPlayToEndTime:)
		name:AVPlayerItemFailedToPlayToEndTimeNotification
		object:m_player.currentItem];

	m_running = true;
	m_overlay = nil;
	
	m_start_time = startTime;
	m_looping = looping;
	
	s_movie_player_delegate = self;
	
	return self;
}

- (void)dealloc
{
	s_movie_player_delegate = nil;
	[NSNotificationCenter.defaultCenter removeObserver: self];
	[m_player release];
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

- (void)playerItemDidPlayToEndTime:(NSNotification *)notification
{
	if (m_looping)
	{
		[m_player seekToTime:m_start_time];
		return;
	}
	
	m_running = false;
	
	// MW-2011-08-16: [[ Wait ]] Tell the wait to exit (our wait has anyevent == True).
	MCscreen -> pingwait();
}

- (void)playerItemFailedToPlayToEndTime:(NSNotification *)notification
{
	m_running = false;
	MCscreen->pingwait();
}

- (void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary<NSKeyValueChangeKey,id> *)change context:(void *)context
{
	if ([keyPath isEqualToString:@"status"])
	{
		if (((AVPlayerItem*)object).status == AVPlayerItemStatusFailed)
		{
			m_running = false;
			
			// MW-2011-08-16: [[ Wait ]] Tell the wait to exit (our wait has anyevent == True).
			MCscreen -> pingwait();
		}
	}
}

- (void)movieWindowTouched: (UIControl*) p_sender
{
	extern MCExecContext *MCECptr;
	MCEventQueuePostCustom(new MCMovieTouchedEvent(MCECptr -> GetObject()));
}

@end

static void configure_playback_range(AVPlayerViewController *p_player, CMTime &r_start_time)
{
	if (!MCtemplateplayer -> getflag(F_PLAY_SELECTION))
	{
		r_start_time = kCMTimeZero;
		return;
	}
	
	uint32_t t_start_time, t_end_time;
	t_start_time = MCtemplateplayer -> getstarttime();
	t_end_time = MCtemplateplayer -> getendtime();
	p_player.player.currentItem.forwardPlaybackEndTime = CMTimeMake(t_end_time, 1000);
	r_start_time = CMTimeMake(t_start_time, 1000);
}

struct play_fullscreen_t
{
	NSURL *url;
	bool looping;
	bool with_controller;
	AVPlayerViewController *movie_player;
	MCFullscreenMovieDelegate *delegate;
	CMTime start_time;
};

static void play_fullscreen_movie_prewait(void *p_context)
{
	play_fullscreen_t *ctxt;
	ctxt = (play_fullscreen_t *)p_context;
	
    // MM-2011-12-09: [[ Bug 9892 ]] Destroy previous movie player.  Fixes bug with iOS 5 where 
	//		showController is ignored on second running.
    if (s_movie_player != nil)
	{
        [s_movie_player release];
        s_movie_player = nil;
    }
    
	// Make sure we have a movie player view controller with correct url
	s_movie_player = [[AVPlayerViewController alloc] init];
	s_movie_player.player = [AVPlayer playerWithURL:ctxt->url];
	
	ctxt->movie_player = s_movie_player;
	
	ctxt->movie_player.view.hidden = NO;
	ctxt->movie_player.view.opaque = YES;
	ctxt->movie_player.view.alpha = 1.0f;
	ctxt->movie_player.view.backgroundColor = [UIColor blackColor];
	
	ctxt->movie_player.videoGravity = AVLayerVideoGravityResizeAspect;
	ctxt->movie_player.showsPlaybackControls = ctxt->with_controller;
	if (ctxt -> looping)
		ctxt->movie_player.player.actionAtItemEnd = AVPlayerActionAtItemEndNone;
	
	ctxt->movie_player.allowsPictureInPicturePlayback = NO;
	ctxt->movie_player.player.allowsExternalPlayback = NO;
	
	CMTime t_start_time;
	configure_playback_range(ctxt -> movie_player, t_start_time);
	
	ctxt -> delegate = [[MCFullscreenMovieDelegate alloc]
		initWithPlayer:ctxt -> movie_player.player
		startTime:t_start_time
		looping:ctxt->looping];
	
	// Present the view controller and get the delegate to setup its overlay
	// if needed.
	[MCIPhoneGetViewController()
	 	presentViewController:s_movie_player
	 	animated:NO
	 	completion:^{
			[s_movie_player.player seekToTime:t_start_time];
			[s_movie_player.player play];
		}];
	[ctxt -> delegate begin: !ctxt -> with_controller];
}

static void play_fullscreen_movie_postwait(void *p_context)
{
	play_fullscreen_t *ctxt;
	ctxt = (play_fullscreen_t *)p_context;
	
	// Clear up and overlay and dismiss the controller.
	[ctxt -> delegate end];
	[MCIPhoneGetViewController() dismissViewControllerAnimated:NO completion:^{}];
	
	// Cleanup the delegate
	[ctxt -> delegate release];
	
	// Make sure we reset the movie player to nothing.
	[ctxt->movie_player.player pause];
	[ctxt->movie_player.player replaceCurrentItemWithPlayerItem:[AVPlayerItem playerItemWithURL:[NSURL URLWithString:@""]]];
}

static bool play_fullscreen_movie_new(NSURL *p_movie, bool p_looping, bool p_with_controller)
{
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
		t_url = [NSURL URLWithString:MCStringConvertToAutoreleasedNSString(p_video)];
	}
	else
	{
		MCAutoStringRef t_path;
		/* UNCHECKED */ MCS_resolvepath(p_video, &t_path);
		t_url = [NSURL fileURLWithPath:MCStringConvertToAutoreleasedNSString(*t_path)];
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
