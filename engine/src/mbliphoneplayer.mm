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
#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"

#include "mcerror.h"
#include "execpt.h"
#include "printer.h"
#include "globals.h"
#include "dispatch.h"
#include "stack.h"
#include "image.h"
#include "player.h"
#include "param.h"
#include "eventqueue.h"
#include "osspec.h"

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

#include "mbldc.h"
#include "mbliphonecontrol.h"

#import <MediaPlayer/MPMoviePlayerController.h>
#import <MediaPlayer/MPMoviePlayerViewController.h>

////////////////////////////////////////////////////////////////////////////////

bool MCParseParameters(MCParameter*& p_parameters, const char *p_format, ...);
UIView *MCIPhoneGetView(void);

////////////////////////////////////////////////////////////////////////////////

extern bool g_movie_player_in_use;
extern MPMoviePlayerViewController *g_movie_player;

////////////////////////////////////////////////////////////////////////////////

class MCNativePlayerControl;

@interface MCNativePlayerDelegate : NSObject
{
	MCNativePlayerControl *m_instance;
    UIControl *m_overlay;
}

- (id)initWithInstance:(MCNativePlayerControl*)instance;
- (void)beginWithOverlay: (bool)p_overlay;
- (void)end;
- (void)dealloc;

- (void)movieDurationAvailable: (NSNotification *)notification;
- (void)movieMediaTypesAvailable: (NSNotification *)notification;
- (void)movieNaturalSizeAvailable: (NSNotification *)notification;
- (void)movieSourceTypeAvailable: (NSNotification *)notification;
- (void)movieLoadStateDidChange: (NSNotification *)notification;

- (void)playerWillEnterFullscreen: (NSNotification *)notification;
- (void)playerDidExitFullscreen: (NSNotification *)notification;
- (void)playerMovieChanged: (NSNotification *)notification;
- (void)playerPlaybackDidFinish: (NSNotification *)notification;
- (void)playerPlaybackStateDidChange: (NSNotification *)notification;
- (void)playerScalingModeDidChange: (NSNotification *)notification;
- (void)playerWindowTouched: (UIControl*) p_sender;

@end

class MCNativePlayerControl: public MCiOSControl
{
public:
	MCNativePlayerControl(void);
	
	virtual MCNativeControlType GetType(void);
	
	virtual Exec_stat Set(MCNativeControlProperty property, MCExecPoint& ep);
	virtual Exec_stat Get(MCNativeControlProperty property, MCExecPoint& ep);	
	virtual Exec_stat Do(MCNativeControlAction action, MCParameter *parameters);
	
	void HandlePropertyAvailableEvent(const char *p_property);
	void HandleNotifyEvent(MCNameRef p_event);
	
	MPMoviePlayerController *GetController(void);
	
	static bool FormatTimeInterval(MCExecPoint& ep, NSTimeInterval interval);
	
protected:
	virtual ~MCNativePlayerControl(void);
	virtual UIView *CreateView(void);
	virtual void DeleteView(UIView *view);
	
private:
	MPMoviePlayerController *m_controller;
	MCNativePlayerDelegate *m_delegate;
};

////////////////////////////////////////////////////////////////////////////////

static void MCIPhoneImportUIImage(UIImage *p_image, int32_t p_max_width, int32_t p_max_height)
{
	uint32_t t_width, t_height;
	t_width = [p_image size] . width * [p_image scale];
	t_height = [p_image size] . height * [p_image scale];

	double t_scale_x, t_scale_y;
	t_scale_x = p_max_width != 0  && t_width > p_max_width ? p_max_width / (double)t_width : 1.0;
	t_scale_y = p_max_height != 0 && t_height > p_max_height ? p_max_height / (double)t_height : 1.0;
	
	double t_scale;
	t_scale = fmin(t_scale_x, t_scale_y);
	
	if (t_scale < 1.0)
	{
		CGSize t_size;
		t_size = CGSizeMake(t_width * t_scale, t_height * t_scale);
		UIGraphicsBeginImageContext(t_size);
		[p_image drawInRect: CGRectMake(0.0f, 0.0f, t_size . width, t_size . height)];
		p_image = UIGraphicsGetImageFromCurrentImageContext();
		UIGraphicsEndImageContext();
	}
	
	NSData *t_data;
	t_data = nil;
	if (MCpaintcompression == EX_JPEG)
		t_data = UIImageJPEGRepresentation(p_image, MCjpegquality / 100.0f);
	else
		t_data = UIImagePNGRepresentation(p_image);
	
	MCtemplateimage->setparent((MCObject *)MCdefaultstackptr -> getcurcard());
	MCImage *iptr = (MCImage *)MCtemplateimage->clone(False, OP_NONE, false);
	MCtemplateimage->setparent(NULL);
	iptr -> attach(OP_CENTER, false);

	MCExecPoint ep(nil, nil, nil);
	ep . setsvalue(MCString((const char *)[t_data bytes], [t_data length]));
	iptr -> setprop(0, P_TEXT, ep, false);
}

static void content_to_url(const char *p_file, NSURL*& r_url)
{
	NSURL *t_url;
	t_url = nil;
	if (strncmp(p_file, "http://", 7) == 0 || strncmp(p_file, "https://", 8) == 0)
		t_url = [NSURL URLWithString: [NSString stringWithCString: p_file encoding: NSMacOSRomanStringEncoding]];
	else if (!MCCStringIsEmpty(p_file))
	{
		char *t_path;
		t_path = MCS_resolvepath(p_file);
		t_url = [NSURL fileURLWithPath: [NSString stringWithCString: t_path encoding: NSMacOSRomanStringEncoding]];
		delete t_path;
	}
	else
		t_url = [NSURL URLWithString: @""];

	r_url = t_url;
}

bool MCNativePlayerControl::FormatTimeInterval(MCExecPoint& ep, NSTimeInterval p_interval)
{
	if (p_interval == -1)
		ep . setnvalue(-1);
	else
		ep . setnvalue((int32_t)(p_interval * 1000));
	return true;
}

////////////////////////////////////////////////////////////////////////////////

static MCNativeControlEnumEntry s_loadstate_set[] =
{
	{ "playable", MPMovieLoadStatePlayable },
	{ "playthrough", MPMovieLoadStatePlaythroughOK },
	{ "stalled", MPMovieLoadStateStalled },
	{ nil, 0 },
};

static MCNativeControlEnumEntry s_playbackstate_enum[] =
{
	{ "stopped", MPMoviePlaybackStateStopped },
	{ "playing", MPMoviePlaybackStatePlaying },
	{ "paused", MPMoviePlaybackStatePaused },
	{ "interrupted", MPMoviePlaybackStateInterrupted },
	{ "seeking forward", MPMoviePlaybackStateSeekingForward },
	{ "seeking backward", MPMoviePlaybackStateSeekingBackward },
	{ nil, 0 },
};

MCNativePlayerControl::MCNativePlayerControl(void)
{
	g_movie_player_in_use = true;
	
	m_delegate = nil;
	m_controller = nil;
}

MCNativePlayerControl::~MCNativePlayerControl(void)
{
	g_movie_player_in_use = false;
}

MCNativeControlType MCNativePlayerControl::GetType(void)
{
	return kMCNativeControlTypePlayer;
}

Exec_stat MCNativePlayerControl::Set(MCNativeControlProperty p_property, MCExecPoint& ep)
{
	bool t_bool;
	int32_t t_enum;
	int32_t t_integer;
	double t_real;
	
	switch(p_property)
	{
		case kMCNativeControlPropertyBackgroundColor:
		{
			UIColor *t_color;
			if (ParseColor(ep, t_color) != ES_NORMAL)
				return ES_ERROR;
			
			if (m_controller != nil)
				[[m_controller backgroundView] setBackgroundColor: t_color];
		}
		break;
		case kMCNativeControlPropertyContent:
		{
			NSURL *t_url;
			MPMovieSourceType t_type;
			content_to_url(ep . getcstring(), t_url);
			if (m_controller != nil)
			{
				[m_controller setContentURL: t_url];
				[m_controller prepareToPlay];
			}
		}
		return ES_NORMAL;
			
		case kMCNativeControlPropertyFullscreen:
			if (!ParseBoolean(ep, t_bool))
				return ES_ERROR;
			if (m_controller != nil)
				[m_controller setFullscreen: t_bool];
			return ES_NORMAL;
		
		case kMCNativeControlPropertyPreserveAspect:
			if (!ParseBoolean(ep, t_bool))
				return ES_ERROR;
			if (m_controller != nil)
				[m_controller setScalingMode: t_bool ? MPMovieScalingModeAspectFit : MPMovieScalingModeFill];
			return ES_NORMAL;
			
		case kMCNativeControlPropertyShowController:
			if (!ParseBoolean(ep, t_bool))
				return ES_ERROR;
			if (m_controller != nil)
				[m_controller setControlStyle: !t_bool ? MPMovieControlStyleNone : ([m_controller isFullscreen] ? MPMovieControlStyleFullscreen : MPMovieControlStyleEmbedded)];
			return ES_NORMAL;
			
		case kMCNativeControlPropertyUseApplicationAudioSession:
			if (!ParseBoolean(ep, t_bool))
				return ES_ERROR;
			if (m_controller != nil)
				[m_controller setUseApplicationAudioSession: t_bool];
			return ES_NORMAL;
			
		case kMCNativeControlPropertyStartTime:
			if (!ParseInteger(ep, t_integer))
				return ES_ERROR;
			if (m_controller != nil)
				[m_controller setInitialPlaybackTime: t_integer / 1000.0];
			return ES_NORMAL;
		
		case kMCNativeControlPropertyEndTime:
			if (!ParseInteger(ep, t_integer))
				return ES_ERROR;
			if (m_controller != nil)
				[m_controller setEndPlaybackTime: (t_integer / 1000.0)];
			return ES_NORMAL;
			
		case kMCNativeControlPropertyCurrentTime:
			if (!ParseInteger(ep, t_integer))
				return ES_ERROR;
			if (m_controller != nil)
			{
				if ([m_controller playbackState] == MPMoviePlaybackStateStopped)
					[m_controller prepareToPlay];
				[m_controller setCurrentPlaybackTime: t_integer / 1000.0];
			}
			return ES_NORMAL;
			
		case kMCNativeControlPropertyShouldAutoplay:
			if (!ParseBoolean(ep, t_bool))
				return ES_ERROR;
			if (m_controller != nil)
				[m_controller setShouldAutoplay: t_bool];
			return ES_NORMAL;
			
		case kMCNativeControlPropertyLooping:
			if (!ParseBoolean(ep, t_bool))
				return ES_ERROR;
			if (m_controller != nil)
				[m_controller setRepeatMode: t_bool ? MPMovieRepeatModeOne : MPMovieRepeatModeNone];
			return ES_NORMAL;
			
		case kMCNativeControlPropertyAllowsAirPlay:
			if (!ParseBoolean(ep, t_bool))
				return ES_ERROR;
			if (m_controller != nil && MCmajorosversion >= 430)
				[m_controller setAllowsAirPlay: t_bool];
			return ES_NORMAL;
			
		case kMCNativeControlPropertyPlayRate:
			if (!ParseReal(ep, t_real))
				return ES_ERROR;
			if (m_controller != nil)
				[m_controller setCurrentPlaybackRate: (float)t_real];
			return ES_NORMAL;
		
		default:
			break;
	}
	
	return MCiOSControl::Set(p_property, ep);
}

Exec_stat MCNativePlayerControl::Get(MCNativeControlProperty p_property, MCExecPoint& ep)
{
	switch(p_property)
	{
		case kMCNativeControlPropertyBackgroundColor:
			if (m_controller != nil)
				FormatColor(ep, [[m_controller backgroundView] backgroundColor]);
			return ES_NORMAL;
			
		case kMCNativeControlPropertyContent:
			if (m_controller != nil)
			{
				NSURL *t_url;
				t_url = [m_controller contentURL];
				if ([t_url isFileURL])
					FormatString(ep, [t_url path]);
				else
					FormatString(ep, [t_url relativeString]);
			}
			return ES_NORMAL;
			
		case kMCNativeControlPropertyFullscreen:
			if (m_controller != nil)
				FormatBoolean(ep, [m_controller isFullscreen]);
			return ES_NORMAL;
			
		case kMCNativeControlPropertyPreserveAspect:
			if (m_controller != nil)
				FormatBoolean(ep, [m_controller scalingMode] != MPMovieScalingModeFill);
			return ES_NORMAL;
			
		case kMCNativeControlPropertyShowController:
			if (m_controller != nil)
				FormatBoolean(ep, [m_controller controlStyle] != MPMovieControlStyleNone);
			return ES_NORMAL;
			
		case kMCNativeControlPropertyUseApplicationAudioSession:
			if (m_controller != nil)
				FormatBoolean(ep, [m_controller useApplicationAudioSession]);
			return ES_NORMAL;
		
		case kMCNativeControlPropertyShouldAutoplay:
			if (m_controller != nil)
				FormatBoolean(ep, [m_controller shouldAutoplay]);
			return ES_NORMAL;
			
		case kMCNativeControlPropertyLooping:
			if (m_controller != nil)
				FormatBoolean(ep, [m_controller repeatMode] != MPMovieRepeatModeNone);
			return ES_NORMAL;
			
		case kMCNativeControlPropertyAllowsAirPlay:
			if (m_controller != nil)
				FormatBoolean(ep, MCmajorosversion >= 430 ? [m_controller allowsAirPlay] : NO);
			return ES_NORMAL;
			
		case kMCNativeControlPropertyDuration:
			if (m_controller != nil)
				FormatTimeInterval(ep, [m_controller duration]);
			return ES_NORMAL;
			
		case kMCNativeControlPropertyPlayableDuration:
			if (m_controller != nil)
				FormatTimeInterval(ep, [m_controller playableDuration]);
			return ES_NORMAL;
			
		case kMCNativeControlPropertyCurrentTime:
			if (m_controller != nil)
				FormatTimeInterval(ep, [m_controller currentPlaybackTime]);
			return ES_NORMAL;
			
		case kMCNativeControlPropertyStartTime:
			if (m_controller != nil)
				FormatTimeInterval(ep, [m_controller initialPlaybackTime]);
			return ES_NORMAL;
			
		case kMCNativeControlPropertyEndTime:
			if (m_controller != nil)
				FormatTimeInterval(ep, [m_controller endPlaybackTime]);
			return ES_NORMAL;
			
		case kMCNativeControlPropertyPlayRate:
			if (m_controller != nil)
				FormatReal(ep, [m_controller currentPlaybackRate]);
			return ES_NORMAL;
			
		case kMCNativeControlPropertyIsPreparedToPlay:
			if (m_controller != nil)
				FormatBoolean(ep, [m_controller isPreparedToPlay]);
			return ES_NORMAL;
			
		case kMCNativeControlPropertyLoadState:
			if (m_controller != nil)
				FormatSet(ep, s_loadstate_set, (int32_t)[m_controller loadState]);
			return ES_NORMAL;
			
		case kMCNativeControlPropertyPlaybackState:
			if (m_controller != nil)
				FormatEnum(ep, s_playbackstate_enum, (int32_t)[m_controller playbackState]);
			return ES_NORMAL;
			
		case kMCNativeControlPropertyNaturalSize:
			if (m_controller != nil)
			{
				CGSize t_size;
				t_size = [m_controller naturalSize];
				sprintf(ep.getbuffer(I2L * 2 + 3), "%d,%d", (int32_t)t_size . width, (int32_t)t_size . height);
				ep.setstrlen();
			}
			return ES_NORMAL;
			
		default:
			break;
	}
	
	return MCiOSControl::Get(p_property, ep);
}

Exec_stat MCNativePlayerControl::Do(MCNativeControlAction p_action, MCParameter *p_parameters)
{
	if (m_controller == nil)
		return MCiOSControl::Do(p_action, p_parameters);
	
	switch(p_action)
	{
		case kMCNativeControlActionPlay:
        // PM-2014-09-18: [[ Bug 13048 ]] Make sure movieTouched message is sent
        {
            [m_delegate beginWithOverlay:[m_controller isFullscreen]];
            [m_controller play];
			return ES_NORMAL;
        }
		case kMCNativeControlActionPause:
			[m_controller pause];
			return ES_NORMAL;
		case kMCNativeControlActionPrepareToPlay:
			[m_controller prepareToPlay];
			return ES_NORMAL;
		case kMCNativeControlActionStop:
			[m_controller stop];
			return ES_NORMAL;
		case kMCNativeControlActionBeginSeekingForward:
			[m_controller beginSeekingForward];
			return ES_NORMAL;
		case kMCNativeControlActionBeginSeekingBackward:
			[m_controller beginSeekingBackward];
			return ES_NORMAL;
		case kMCNativeControlActionEndSeeking:
			[m_controller endSeeking];
			return ES_NORMAL;
		case kMCNativeControlActionSnapshot:
		case kMCNativeControlActionSnapshotExactly:
		{
			int32_t t_time, t_max_width, t_max_height;
			t_time = 0;
			if (MCParseParameters(p_parameters, "i", &t_time))
			{
				if (MCParseParameters(p_parameters, "ii", &t_max_width, &t_max_height))
				{
					if (p_parameters != nil)
						return ES_ERROR;
				}
				else
					t_max_width = t_max_height = 0;
			}
			else
				return ES_ERROR;
				
			UIImage *t_image;
			t_image = [m_controller thumbnailImageAtTime: t_time / 1000.0
											  timeOption: (p_action == kMCNativeControlActionSnapshot ? MPMovieTimeOptionNearestKeyFrame : MPMovieTimeOptionExact)];
			if (t_image != nil)
				MCIPhoneImportUIImage(t_image, t_max_width, t_max_height);
		}
		return ES_NORMAL;
		default:
			break;
	}
	
	return MCiOSControl::Do(p_action, p_parameters);
}

////////////////////////////////////////////////////////////////////////////////

void MCNativePlayerControl::HandlePropertyAvailableEvent(const char *p_property)
{
	MCObject *t_target;
	t_target = GetOwner();
	if (t_target != nil)
	{
		MCNativeControl *t_old_target;
		t_old_target = ChangeTarget(this);
		t_target -> message_with_args(MCM_player_property_available, p_property);
		ChangeTarget(t_old_target);
	}
}

void MCNativePlayerControl::HandleNotifyEvent(MCNameRef p_message)
{
	MCObject *t_target;
	t_target = GetOwner();
	if (t_target != nil)
	{
		MCNativeControl *t_old_target;
		t_old_target = ChangeTarget(this);
		t_target -> message(p_message);
		ChangeTarget(t_old_target);
	}
}

////////////////////////////////////////////////////////////////////////////////

MPMoviePlayerController *MCNativePlayerControl::GetController(void)
{
	return m_controller;
}

////////////////////////////////////////////////////////////////////////////////

UIView *MCNativePlayerControl::CreateView(void)
{
    // MM-2012-02-26: [[ Bug 10568 ]] Initialising a MPMoviePlayerController with an empty URL appears to cause a crash in the 6.0 (and later) simulator.
    //  Init with nil instead.
    if (MCmajorosversion >= 420)
        m_controller = [[MPMoviePlayerController alloc] initWithContentURL: nil];
    else
    {
        if (g_movie_player == nil)
            g_movie_player = [[MPMoviePlayerViewController alloc] initWithContentURL: [NSURL URLWithString: @""]];
        m_controller = [g_movie_player moviePlayer];
        [m_controller setFullscreen: NO];
        [m_controller setControlStyle: MPMovieControlStyleEmbedded];
        g_movie_player_in_use = true;
    }

    [m_controller setShouldAutoplay: NO];
    [m_controller stop];
    
    UIView *t_view;
    t_view = [m_controller view];
    if (t_view == nil)
        return nil;

    [t_view setHidden: YES];
    [t_view setFrame: CGRectMake(0, 0, 0, 0)];

    m_delegate = [[MCNativePlayerDelegate alloc] initWithInstance: this];
    return t_view;
}

void MCNativePlayerControl::DeleteView(UIView *p_view)
{
	[m_controller stop];
	if (MCmajorosversion >= 420)
		[m_controller release];
	m_controller = nil;
	
	[m_delegate release];
	m_delegate = nil;
	
	if (MCmajorosversion < 420)
	{
		[[g_movie_player moviePlayer] setContentURL: [NSURL URLWithString: @""]];
		g_movie_player_in_use = false;
	}
}

////////////////////////////////////////////////////////////////////////////////

class MCNativePlayerPropertyAvailableEvent: public MCCustomEvent
{
public:
	// Note that we require p_property to be a C-string constant as we don't
	// copy it.
	MCNativePlayerPropertyAvailableEvent(MCNativePlayerControl *p_target, const char *p_property)
	{
		m_target = p_target;
		m_target -> Retain();
		m_property = p_property;
	}
	
	void Destroy(void)
	{
		m_target -> Release();
		delete this;
	}
	
	void Dispatch(void)
	{
		m_target -> HandlePropertyAvailableEvent(m_property);
	}
	
private:
	MCNativePlayerControl *m_target;
	const char *m_property;
};

class MCNativePlayerNotifyEvent: public MCCustomEvent
{
public:
	// Note that we require p_notification to be a C-string constant as we don't
	// copy it.
	MCNativePlayerNotifyEvent(MCNativePlayerControl *p_target, MCNameRef p_notification)
	{
		m_target = p_target;
		m_target -> Retain();
		m_notification = p_notification;
	}
	
	void Destroy(void)
	{
		m_target -> Release();
		delete this;
	}
	
	void Dispatch(void)
	{
		m_target -> HandleNotifyEvent(m_notification);
	}
	
private:
	MCNativePlayerControl *m_target;
	MCNameRef m_notification;
};

// MM-2013-09-23: [[ iOS7 Support ]] Tweaked type to appease llvm 5.0.
static struct { NSString* const* name; SEL selector; } s_player_notifications[] =
{
	{ &MPMovieDurationAvailableNotification, @selector(movieDurationAvailable:) },
	{ &MPMovieMediaTypesAvailableNotification, @selector(movieMediaTypesAvailable:) },
	{ &MPMovieNaturalSizeAvailableNotification, @selector(movieNaturalSizeAvailable:) },
	{ &MPMovieSourceTypeAvailableNotification, @selector(movieSourceTypeAvailable:) },
	{ &MPMoviePlayerWillEnterFullscreenNotification, @selector(playerWillEnterFullscreen:) },
	{ &MPMoviePlayerDidExitFullscreenNotification, @selector(playerDidExitFullscreen:) },
	{ &MPMoviePlayerLoadStateDidChangeNotification, @selector(movieLoadStateDidChange:) },
	{ &MPMoviePlayerNowPlayingMovieDidChangeNotification, @selector(playerMovieChanged:) },
	{ &MPMoviePlayerPlaybackDidFinishNotification, @selector(playerPlaybackDidFinish:) },
	{ &MPMoviePlayerPlaybackStateDidChangeNotification, @selector(playerPlaybackStateDidChange:) },
	{ &MPMoviePlayerScalingModeDidChangeNotification, @selector(playerScalingModeDidChange:) },
	{ nil, nil }
};

////////////////////////////////////////////////////////////////////////////////

@implementation MCNativePlayerDelegate

- (id)initWithInstance:(MCNativePlayerControl*)instance
{
	self = [super init];
	if (self == nil)
		return nil;
	
	m_instance = instance;
    m_overlay = nil;
	
	for(uint32_t i = 0; s_player_notifications[i] . name != nil; i++)
		if (*s_player_notifications[i] . name != nil)
			[[NSNotificationCenter defaultCenter] addObserver: self
													 selector: s_player_notifications[i] . selector 
														 name: *s_player_notifications[i] . name 
													   object: m_instance -> GetController()];
	
	return self;
}

// PM-2015-02-12: [[ Bug 14525 ]] Moved the overlay code entirely into the delegate
- (void)beginWithOverlay:(bool)p_overlay
{
    if (p_overlay)
    {
        // The movie's window is the one that is active
        UIWindow *t_window = [[UIApplication sharedApplication] keyWindow];

        // Now we create an invisible control with the same size as the window
        m_overlay = [[UIControl alloc] initWithFrame: [t_window frame]];
        
        // We want to get notified whenever the overlay control is touched
        [m_overlay addTarget: self action: @selector(playerWindowTouched:) forControlEvents: UIControlEventTouchDown];
        [t_window addSubview: m_overlay];
    }
}

- (void)end
{
    if (m_overlay != nil)
    {
        [m_overlay removeTarget: self action: @selector(playerWindowTouched:) forControlEvents: UIControlEventTouchDown];
        [m_overlay removeFromSuperview];
        [m_overlay release];
        m_overlay = nil;
    }
}

- (void)dealloc
{
	[[NSNotificationCenter defaultCenter] removeObserver: self];
	[super dealloc];
}

- (void)movieDurationAvailable: (NSNotification *)notification
{
	MCEventQueuePostCustom(new MCNativePlayerPropertyAvailableEvent(m_instance, "duration"));
}

- (void)movieMediaTypesAvailable: (NSNotification *)notification
{
}

- (void)movieNaturalSizeAvailable: (NSNotification *)notification
{
	MCEventQueuePostCustom(new MCNativePlayerPropertyAvailableEvent(m_instance, "naturalSize"));
}

- (void)movieSourceTypeAvailable: (NSNotification *)notification
{
}

- (void)movieLoadStateDidChange: (NSNotification *)notification
{
	MCEventQueuePostCustom(new MCNativePlayerNotifyEvent(m_instance, MCM_player_progress_changed));
}

- (void)playerWillEnterFullscreen: (NSNotification *)notification
{
	// MW-2011-04-07: [[ Bug 9468 ]] When the player enters fullscreen it doesn't cause touch
	//   cancellation to occur and so we get lingering touch events
	static_cast<MCScreenDC *>(MCscreen) -> cancel_touches();
	
	MCEventQueuePostCustom(new MCNativePlayerNotifyEvent(m_instance, MCM_player_enter_fullscreen));
}

- (void)playerDidExitFullscreen: (NSNotification *)notification
{
	MCEventQueuePostCustom(new MCNativePlayerNotifyEvent(m_instance, MCM_player_leave_fullscreen));
}

- (void)playerMovieChanged: (NSNotification *)notification
{
	MCEventQueuePostCustom(new MCNativePlayerNotifyEvent(m_instance, MCM_player_movie_changed));
}

- (void)playerPlaybackDidFinish: (NSNotification *)notification
{
    // PM-2014-09-18: [[ Bug 13048 ]] Clear m_overlay if playback finishes for any reason
    [self end];
    
	NSObject *t_value;
	
	int32_t t_reason;
	t_reason = [[[notification userInfo] objectForKey: MPMoviePlayerPlaybackDidFinishReasonUserInfoKey] intValue];
	
	MCNameRef t_message;
	t_message = nil;
	if (t_reason == MPMovieFinishReasonPlaybackEnded)
		t_message = MCM_player_finished;
	else if (t_reason == MPMovieFinishReasonPlaybackError)
	{
		NSString *t_string;
		t_string = [[m_instance -> GetController() contentURL] absoluteString];
		if (![t_string isEqualToString: @""])
			t_message = MCM_player_error;
	}
	else if (t_reason == MPMovieFinishReasonUserExited)
		t_message = MCM_player_stopped;
	
	if (t_message != nil)
		MCEventQueuePostCustom(new MCNativePlayerNotifyEvent(m_instance, t_message));
}

- (void)playerPlaybackStateDidChange: (NSNotification *)notification
{
	MCEventQueuePostCustom(new MCNativePlayerNotifyEvent(m_instance, MCM_player_state_changed));
}

- (void)playerScalingModeDidChange: (NSNotification *)notification
{
}

- (void)playerWindowTouched: (UIControl*) p_sender
{
    MCEventQueuePostCustom(new MCNativePlayerNotifyEvent(m_instance, MCM_movie_touched));
}
@end

////////////////////////////////////////////////////////////////////////////////

bool MCNativePlayerControlCreate(MCNativeControl*& r_control)
{
	if (MCmajorosversion < 420 && g_movie_player_in_use)
		return false;
	
	r_control = new MCNativePlayerControl;
	return true;
}

////////////////////////////////////////////////////////////////////////////////

