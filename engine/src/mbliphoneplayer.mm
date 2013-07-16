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
#include "exec.h"

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

#include "mbldc.h"
#include "mbliphone.h"
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

class MCiOSPlayerControl;

@interface MCiOSPlayerDelegate : NSObject
{
	MCiOSPlayerControl *m_instance;
}

- (id)initWithInstance:(MCiOSPlayerControl*)instance;
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

@end

class MCiOSPlayerControl: public MCiOSControl
{
public:
	MCiOSPlayerControl(void);
	
	virtual MCNativeControlType GetType(void);
	
	virtual Exec_stat Set(MCNativeControlProperty property, MCExecPoint& ep);
	virtual Exec_stat Get(MCNativeControlProperty property, MCExecPoint& ep);	
	virtual Exec_stat Do(MCNativeControlAction action, MCParameter *parameters);
	
    virtual void SetBackgroundColor(MCExecContext& ctxt, const MCNativeControlColor& p_color);
    virtual void GetBackgroundColor(MCExecContext& ctxt, MCNativeControlColor& r_color);

    void SetContent(MCExecContext& ctxt, MCStringRef p_content);
    void GetContent(MCExecContext& ctxt, MCStringRef& r_content);
    void SetFullscreen(MCExecContext& ctxt, bool p_value);
    void GetFullscreen(MCExecContext& ctxt, bool& r_value);
    void SetPreserveAspect(MCExecContext& ctxt, bool p_value);
    void GetPreserveAspect(MCExecContext& ctxt, bool& r_value);
    void SetShowController(MCExecContext& ctxt, bool p_value);
    void GetShowController(MCExecContext& ctxt, bool& r_value);
    void SetUseApplicationAudioSession(MCExecContext& ctxt, bool p_value);
    void GetUseApplicationAudioSession(MCExecContext& ctxt, bool& r_value);
    void SetStartTime(MCExecContext& ctxt, integer_t p_time);
    void GetStartTime(MCExecContext& ctxt, integer_t& r_time);
    void SetEndTime(MCExecContext& ctxt, integer_t p_time);
    void GetEndTime(MCExecContext& ctxt, integer_t& r_time);
    void SetCurrentTime(MCExecContext& ctxt, integer_t p_time);
    void GetCurrentTime(MCExecContext& ctxt, integer_t& r_time);
    void SetShouldAutoplay(MCExecContext& ctxt, bool p_value);
    void GetShouldAutoplay(MCExecContext& ctxt, bool& r_value);
    void SetLooping(MCExecContext& ctxt, bool p_value);
    void GetLooping(MCExecContext& ctxt, bool& r_value);
    void SetAllowsAirPlay(MCExecContext& ctxt, bool p_value);
    void GetAllowsAirPlay(MCExecContext& ctxt, bool& r_value);
    void SetPlayRate(MCExecContext& ctxt, double p_rate);
    void GetPlayRate(MCExecContext& ctxt, double& r_rate);
    
    void GetDuration(MCExecContext& ctxt, integer_t& r_duration);
    void GetPlayableDuration(MCExecContext& ctxt, integer_t& r_duration);
    void GetIsPreparedToPlay(MCExecContext& ctxt, bool& r_value);
    void GetLoadState(MCExecContext& ctxt, intset_t& r_state);
    void GetPlaybackState(MCExecContext& ctxt, intenum_t& r_state);
    void GetNaturalSize(MCExecContext& ctxt, MCPoint32& r_size);
    
	void HandlePropertyAvailableEvent(const char *p_property);
	void HandleNotifyEvent(MCNameRef p_event);
	
	MPMoviePlayerController *GetController(void);
	
	static bool FormatTimeInterval(MCExecPoint& ep, NSTimeInterval interval);
	
protected:
	virtual ~MCiOSPlayerControl(void);
	virtual UIView *CreateView(void);
	virtual void DeleteView(UIView *view);
	
private:
	MPMoviePlayerController *m_controller;
	MCiOSPlayerDelegate *m_delegate;
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

bool MCiOSPlayerControl::FormatTimeInterval(MCExecPoint& ep, NSTimeInterval p_interval)
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

MCiOSPlayerControl::MCiOSPlayerControl(void)
{
	g_movie_player_in_use = true;
	
	m_delegate = nil;
	m_controller = nil;
}

MCiOSPlayerControl::~MCiOSPlayerControl(void)
{
	g_movie_player_in_use = false;
}

MCNativeControlType MCiOSPlayerControl::GetType(void)
{
	return kMCNativeControlTypePlayer;
}

void MCiOSPlayerControl::SetBackgroundColor(MCExecContext& ctxt, const MCNativeControlColor& p_color)
{
    UIColor *t_color;
    if (ParseColor(p_color, t_color))
        if (m_controller != nil)
            [[m_controller backgroundView] setBackgroundColor: t_color];
}

void MCiOSPlayerControl::GetBackgroundColor(MCExecContext& ctxt, MCNativeControlColor& r_color)
{
    if (m_controller != nil)
        FormatColor([[m_controller backgroundView] backgroundColor], r_color);
}

void MCiOSPlayerControl::SetContent(MCExecContext& ctxt, MCStringRef p_content)
{
    NSURL *t_url;
    MPMovieSourceType t_type;
    content_to_url(MCStringGetCString(p_content), t_url);
    if (m_controller != nil)
    {
        [m_controller setContentURL: t_url];
        [m_controller prepareToPlay];
    }
}

void MCiOSPlayerControl::GetContent(MCExecContext& ctxt, MCStringRef& r_content)
{
    NSString *t_string;
    t_string = nil;
    
    if (m_controller != nil)
    {
        NSURL *t_url;
        
        t_url = [m_controller contentURL];
        if ([t_url isFileURL])
            t_string = [t_url path];
        else
            t_string = [t_url relativeString];
    }
    if (t_string != nil)
    {
        if (MCStringCreateWithCString([t_string nativeCString], r_content))
            return;
    }
    else
    {
        r_content = MCValueRetain(kMCEmptyString);
        return;
    }
    
    ctxt . Throw();
}
void MCiOSPlayerControl::SetFullscreen(MCExecContext& ctxt, bool p_value)
{
    if (m_controller != nil)
        [m_controller setFullscreen: p_value];
}

void MCiOSPlayerControl::GetFullscreen(MCExecContext& ctxt, bool& r_value)
{
    if (m_controller != nil)
        r_value = [m_controller isFullscreen];
    else
        r_value = false;
}

void MCiOSPlayerControl::SetPreserveAspect(MCExecContext& ctxt, bool p_value)
{
    if (m_controller != nil)
        [m_controller setScalingMode: p_value ? MPMovieScalingModeAspectFit : MPMovieScalingModeFill];
}

void MCiOSPlayerControl::GetPreserveAspect(MCExecContext& ctxt, bool& r_value)
{
    if (m_controller != nil)
        r_value = [m_controller scalingMode] != MPMovieScalingModeAspectFill;
    else
        r_value = false;
}

void MCiOSPlayerControl::SetShowController(MCExecContext& ctxt, bool p_value)
{
    if (m_controller != nil)
        [m_controller setControlStyle: !p_value ? MPMovieControlStyleNone : ([m_controller isFullscreen] ? MPMovieControlStyleFullscreen : MPMovieControlStyleEmbedded)];
}

void MCiOSPlayerControl::GetShowController(MCExecContext& ctxt, bool& r_value)
{
    if (m_controller != nil)
        r_value = [m_controller controlStyle] != MPMovieControlStyleNone;
    else
        r_value = false;
}

void MCiOSPlayerControl::SetUseApplicationAudioSession(MCExecContext& ctxt, bool p_value)
{
    if (m_controller != nil)
        [m_controller setUseApplicationAudioSession: p_value];
}

void MCiOSPlayerControl::GetUseApplicationAudioSession(MCExecContext& ctxt, bool& r_value)
{
    if (m_controller != nil)
        r_value = [m_controller useApplicationAudioSession];
    else
        r_value = false;
}

void MCiOSPlayerControl::SetStartTime(MCExecContext& ctxt, integer_t p_time)
{
    if (m_controller != nil)
        [m_controller setInitialPlaybackTime: p_time / 1000.0];
}

void MCiOSPlayerControl::GetStartTime(MCExecContext& ctxt, integer_t& r_time)
{
    integer_t t_time;
    t_time = -1;
    
    if (m_controller != nil)
        t_time = (integer_t)[m_controller initialPlaybackTime];
    
    if (t_time == -1)
        r_time = t_time;
    else
        r_time = t_time * 1000;
}

void MCiOSPlayerControl::SetEndTime(MCExecContext& ctxt, integer_t p_time)
{
    if (m_controller != nil)
        [m_controller setEndPlaybackTime: p_time / 1000.0];
}

void MCiOSPlayerControl::GetEndTime(MCExecContext& ctxt, integer_t& r_time)
{
    integer_t t_time;
    t_time = -1;
    
    if (m_controller != nil)
        t_time = (integer_t)[m_controller endPlaybackTime];
    
    if (t_time == -1)
        r_time = t_time;
    else
        r_time = t_time * 1000;
}
void MCiOSPlayerControl::SetCurrentTime(MCExecContext& ctxt, integer_t p_time)
{
    if (m_controller != nil)
    {
        if ([m_controller playbackState] == MPMoviePlaybackStateStopped)
            [m_controller prepareToPlay];
        [m_controller setCurrentPlaybackTime: p_time / 1000.0];
    }
}

void MCiOSPlayerControl::GetCurrentTime(MCExecContext& ctxt, integer_t& r_time)
{
    integer_t t_time;
    t_time = -1;
    
    if (m_controller != nil)
        t_time = (integer_t)[m_controller currentPlaybackTime];
    
    if (t_time == -1)
        r_time = t_time;
    else
        r_time = t_time * 1000;
}

void MCiOSPlayerControl::SetShouldAutoplay(MCExecContext& ctxt, bool p_value)
{
    if (m_controller != nil)
        [m_controller setShouldAutoplay: p_value];
}

void MCiOSPlayerControl::GetShouldAutoplay(MCExecContext& ctxt, bool& r_value)
{
    if (m_controller != nil)
        r_value = [m_controller shouldAutoplay];
    else
        r_value = false;
}

void MCiOSPlayerControl::SetLooping(MCExecContext& ctxt, bool p_value)
{
    if (m_controller != nil)
        [m_controller setRepeatMode: p_value ? MPMovieRepeatModeOne : MPMovieRepeatModeNone];
}

void MCiOSPlayerControl::GetLooping(MCExecContext& ctxt, bool& r_value)
{
    if (m_controller != nil)
        r_value = [m_controller repeatMode] != MPMovieRepeatModeNone;
    else
        r_value = false;
}

void MCiOSPlayerControl::SetAllowsAirPlay(MCExecContext& ctxt, bool p_value)
{
    if (m_controller != nil && MCmajorosversion >= 430)
        [m_controller setAllowsAirPlay: p_value];
}

void MCiOSPlayerControl::GetAllowsAirPlay(MCExecContext& ctxt, bool& r_value)
{
    if (m_controller != nil && MCmajorosversion >= 430)
        r_value = [m_controller allowsAirPlay];
    else
        r_value = false;
}

void MCiOSPlayerControl::SetPlayRate(MCExecContext& ctxt, double p_rate)
{
    if (m_controller != nil)
        [m_controller setCurrentPlaybackRate: (float)p_rate];
}

void MCiOSPlayerControl::GetPlayRate(MCExecContext& ctxt, double& r_rate)
{
    if (m_controller != nil)
        r_rate = [m_controller currentPlaybackRate];
    else
        r_rate = 0;
}
void MCiOSPlayerControl::GetDuration(MCExecContext& ctxt, integer_t& r_duration)
{
    if (m_controller != nil)
        r_duration = [m_controller duration];
    else
        r_duration = 0;
}

void MCiOSPlayerControl::GetPlayableDuration(MCExecContext& ctxt, integer_t& r_duration)
{
    if (m_controller != nil)
        r_duration = [m_controller playableDuration];
    else
        r_duration = 0;
}

void MCiOSPlayerControl::GetIsPreparedToPlay(MCExecContext& ctxt, bool& r_value)
{
    if (m_controller != nil)
        r_value = [m_controller isPreparedToPlay];
    else
        r_value = false;
}

void MCiOSPlayerControl::GetLoadState(MCExecContext& ctxt, intset_t& r_state)
{
    r_state = kMCNativeControlLoadStateNone;
    
    if (m_controller != nil)
    {
        MPMovieLoadState t_state;
        t_state = [m_controller loadState];
        
        if (t_state & MPMovieLoadStatePlayable)
            r_state |= kMCNativeControlLoadStatePlayable;
        if (t_state & MPMovieLoadStatePlaythroughOK)
            r_state |= kMCNativeControlLoadStatePlaythroughOK;
        if (t_state & MPMovieLoadStateStalled)
            r_state |= kMCNativeControlLoadStateStalled;
    }
}

void MCiOSPlayerControl::GetPlaybackState(MCExecContext& ctxt, intenum_t& r_state)
{
    if (m_controller != nil)
    {
        switch ([m_controller playbackState])
        {
            case MPMoviePlaybackStateInterrupted:
                r_state = kMCNativeControlPlaybackStateInterrupted;
                return;
            case MPMoviePlaybackStatePaused:
                r_state = kMCNativeControlPlaybackStatePaused;
                return;
            case MPMoviePlaybackStatePlaying:
                r_state = kMCNativeControlPlaybackStatePlaying;
                return;
            case MPMoviePlaybackStateSeekingBackward:
                r_state = kMCNativeControlPlaybackStateSeekingBackward;
                return;
            case MPMoviePlaybackStateSeekingForward:
                r_state = kMCNativeControlPlaybackStateSeekingForward;
                return;
            case MPMoviePlaybackStateStopped:
                r_state = kMCNativeControlPlaybackStateStopped;
                return;
        }
    }
    else
        r_state = kMCNativeControlPlaybackStateNone;
}

void MCiOSPlayerControl::GetNaturalSize(MCExecContext& ctxt, MCPoint32& r_size)
{
    if (m_controller != nil)
    {
        CGSize t_size;
        t_size = [m_controller naturalSize];
        r_size . x = (int32_t)t_size . width;
        r_size . y = (int32_t)t_size . height;
    }
    else
    {
        r_size . x = 0;
        r_size . y = 0;
    }
}

Exec_stat MCiOSPlayerControl::Set(MCNativeControlProperty p_property, MCExecPoint& ep)
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

Exec_stat MCiOSPlayerControl::Get(MCNativeControlProperty p_property, MCExecPoint& ep)
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
				ep.setstringf("%d,%d", (int32_t)t_size . width, (int32_t)t_size . height);
			}
			return ES_NORMAL;
			
		default:
			break;
	}
	
	return MCiOSControl::Get(p_property, ep);
}

Exec_stat MCiOSPlayerControl::Do(MCNativeControlAction p_action, MCParameter *p_parameters)
{
	if (m_controller == nil)
		return MCiOSControl::Do(p_action, p_parameters);
	
	switch(p_action)
	{
		case kMCNativeControlActionPlay:
			[m_controller play];
			return ES_NORMAL;
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

void MCiOSPlayerControl::HandlePropertyAvailableEvent(const char *p_property)
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

void MCiOSPlayerControl::HandleNotifyEvent(MCNameRef p_message)
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

MPMoviePlayerController *MCiOSPlayerControl::GetController(void)
{
	return m_controller;
}

////////////////////////////////////////////////////////////////////////////////

UIView *MCiOSPlayerControl::CreateView(void)
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

    m_delegate = [[MCiOSPlayerDelegate alloc] initWithInstance: this];
    return t_view;
}

void MCiOSPlayerControl::DeleteView(UIView *p_view)
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

class MCiOSPlayerPropertyAvailableEvent: public MCCustomEvent
{
public:
	// Note that we require p_property to be a C-string constant as we don't
	// copy it.
	MCiOSPlayerPropertyAvailableEvent(MCiOSPlayerControl *p_target, const char *p_property)
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
	MCiOSPlayerControl *m_target;
	const char *m_property;
};

class MCiOSPlayerNotifyEvent: public MCCustomEvent
{
public:
	// Note that we require p_notification to be a C-string constant as we don't
	// copy it.
	MCiOSPlayerNotifyEvent(MCiOSPlayerControl *p_target, MCNameRef p_notification)
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
	MCiOSPlayerControl *m_target;
	MCNameRef m_notification;
};

static struct { NSString * const *name; SEL selector; } s_player_notifications[] =
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

@implementation MCiOSPlayerDelegate

- (id)initWithInstance:(MCiOSPlayerControl*)instance
{
	self = [super init];
	if (self == nil)
		return nil;
	
	m_instance = instance;
	
	for(uint32_t i = 0; s_player_notifications[i] . name != nil; i++)
		if (*s_player_notifications[i] . name != nil)
			[[NSNotificationCenter defaultCenter] addObserver: self
													 selector: s_player_notifications[i] . selector 
														 name: *s_player_notifications[i] . name 
													   object: m_instance -> GetController()];
	
	return self;
}

- (void)dealloc
{
	[[NSNotificationCenter defaultCenter] removeObserver: self];
	[super dealloc];
}

- (void)movieDurationAvailable: (NSNotification *)notification
{
	MCEventQueuePostCustom(new MCiOSPlayerPropertyAvailableEvent(m_instance, "duration"));
}

- (void)movieMediaTypesAvailable: (NSNotification *)notification
{
}

- (void)movieNaturalSizeAvailable: (NSNotification *)notification
{
	MCEventQueuePostCustom(new MCiOSPlayerPropertyAvailableEvent(m_instance, "naturalSize"));
}

- (void)movieSourceTypeAvailable: (NSNotification *)notification
{
}

- (void)movieLoadStateDidChange: (NSNotification *)notification
{
	MCEventQueuePostCustom(new MCiOSPlayerNotifyEvent(m_instance, MCM_player_progress_changed));
}

- (void)playerWillEnterFullscreen: (NSNotification *)notification
{
	// MW-2011-04-07: [[ Bug 9468 ]] When the player enters fullscreen it doesn't cause touch
	//   cancellation to occur and so we get lingering touch events
	static_cast<MCScreenDC *>(MCscreen) -> cancel_touches();
	
	MCEventQueuePostCustom(new MCiOSPlayerNotifyEvent(m_instance, MCM_player_enter_fullscreen));
}

- (void)playerDidExitFullscreen: (NSNotification *)notification
{
	MCEventQueuePostCustom(new MCiOSPlayerNotifyEvent(m_instance, MCM_player_leave_fullscreen));
}

- (void)playerMovieChanged: (NSNotification *)notification
{
	MCEventQueuePostCustom(new MCiOSPlayerNotifyEvent(m_instance, MCM_player_movie_changed));
}

- (void)playerPlaybackDidFinish: (NSNotification *)notification
{
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
		MCEventQueuePostCustom(new MCiOSPlayerNotifyEvent(m_instance, t_message));
}

- (void)playerPlaybackStateDidChange: (NSNotification *)notification
{
	MCEventQueuePostCustom(new MCiOSPlayerNotifyEvent(m_instance, MCM_player_state_changed));
}

- (void)playerScalingModeDidChange: (NSNotification *)notification
{
}

@end

////////////////////////////////////////////////////////////////////////////////

bool MCNativePlayerControlCreate(MCNativeControl*& r_control)
{
	if (MCmajorosversion < 420 && g_movie_player_in_use)
		return false;
	
	r_control = new MCiOSPlayerControl;
	return true;
}

////////////////////////////////////////////////////////////////////////////////

