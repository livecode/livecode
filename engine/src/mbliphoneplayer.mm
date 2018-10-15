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

@interface com_runrev_livecode_MCiOSPlayerDelegate : NSObject
{
	MCiOSPlayerControl *m_instance;
    UIControl *m_overlay;
}

- (id)initWithInstance:(MCiOSPlayerControl*)instance;
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

class MCiOSPlayerControl: public MCiOSControl
{
	static MCPropertyInfo kProperties[];
	static MCObjectPropertyTable kPropertyTable;
    static MCNativeControlActionInfo kActions[];
	static MCNativeControlActionTable kActionTable;

public:
	MCiOSPlayerControl(void);
	
	virtual MCNativeControlType GetType(void);
    virtual const MCObjectPropertyTable *getpropertytable(void) const { return &kPropertyTable; }
	virtual const MCNativeControlActionTable *getactiontable(void) const { return &kActionTable; }
    
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
    void GetLoadState(MCExecContext& ctxt, MCNativeControlLoadState& r_state);
    void GetPlaybackState(MCExecContext& ctxt, MCNativeControlPlaybackState& r_state);
    void GetNaturalSize(MCExecContext& ctxt, integer_t r_size[2]);
    void GetIsReadyForDisplay(MCExecContext& ctxt, bool& r_value);
    
    
	// Player-specific actions
	void ExecPlay(MCExecContext& ctxt);
	void ExecPause(MCExecContext& ctxt);
    void ExecStop(MCExecContext& ctxt);
	void ExecPrepareToPlay(MCExecContext& ctxt);
	void ExecBeginSeekingBackward(MCExecContext& ctxt);
	void ExecBeginSeekingForward(MCExecContext& ctxt);
	void ExecEndSeeking(MCExecContext& ctxt);
	void ExecSnapshot(MCExecContext& ctxt, integer_t t_time, integer_t* t_max_width, integer_t* t_max_height);
	void ExecSnapshotExactly(MCExecContext& ctxt, integer_t t_time, integer_t* t_max_width, integer_t* t_max_height);
    
	void HandlePropertyAvailableEvent(const char *p_property);
	void HandleNotifyEvent(MCNameRef p_event);
	
    void Play();
    
	MPMoviePlayerController *GetController(void);

	
protected:
	virtual ~MCiOSPlayerControl(void);
	virtual UIView *CreateView(void);
	virtual void DeleteView(UIView *view);
	
private:
	MPMoviePlayerController *m_controller;
	com_runrev_livecode_MCiOSPlayerDelegate *m_delegate;
};

////////////////////////////////////////////////////////////////////////////////

MCPropertyInfo MCiOSPlayerControl::kProperties[] =
{
    DEFINE_RW_CTRL_CUSTOM_PROPERTY(P_BACKGROUND_COLOR, NativeControlColor, MCiOSPlayerControl, BackgroundColor)
    DEFINE_RW_CTRL_PROPERTY(P_CONTENT, String, MCiOSPlayerControl, Content)
    DEFINE_RW_CTRL_PROPERTY(P_FULLSCREEN, Bool, MCiOSPlayerControl, Fullscreen)
    DEFINE_RW_CTRL_PROPERTY(P_PRESERVE_ASPECT, Bool, MCiOSPlayerControl, PreserveAspect)
    // PM-2014-10-24: [[ Bug 13790 ]] Setting the showController to true resulted in going fullscreen in ios player
    DEFINE_RW_CTRL_PROPERTY(P_SHOW_CONTROLLER, Bool, MCiOSPlayerControl, ShowController)
    DEFINE_RW_CTRL_PROPERTY(P_USE_APPLICATION_AUDIO_SESSION, Bool, MCiOSPlayerControl, UseApplicationAudioSession)
    DEFINE_RW_CTRL_PROPERTY(P_START_TIME, Int32, MCiOSPlayerControl, StartTime)
    DEFINE_RW_CTRL_PROPERTY(P_END_TIME, Int32, MCiOSPlayerControl, EndTime)
    DEFINE_RW_CTRL_PROPERTY(P_CURRENT_TIME, Int32, MCiOSPlayerControl, CurrentTime)
    DEFINE_RW_CTRL_PROPERTY(P_SHOULD_AUTOPLAY, Bool, MCiOSPlayerControl, ShouldAutoplay)
    DEFINE_RW_CTRL_PROPERTY(P_LOOPING, Bool, MCiOSPlayerControl, Looping)
    DEFINE_RW_CTRL_PROPERTY(P_ALLOWS_AIR_PLAY, Bool, MCiOSPlayerControl, AllowsAirPlay)
    DEFINE_RW_CTRL_PROPERTY(P_PLAY_RATE, Double, MCiOSPlayerControl, PlayRate)
    DEFINE_RO_CTRL_PROPERTY(P_DURATION, Int32, MCiOSPlayerControl, Duration)
    DEFINE_RO_CTRL_PROPERTY(P_PLAYABLE_DURATION, Int32, MCiOSPlayerControl, PlayableDuration)
    DEFINE_RO_CTRL_PROPERTY(P_IS_PREPARED_TO_PLAY, Bool, MCiOSPlayerControl, IsPreparedToPlay)
    DEFINE_RO_CTRL_SET_PROPERTY(P_LOAD_STATE, NativeControlLoadState, MCiOSPlayerControl, LoadState)
    DEFINE_RO_CTRL_ENUM_PROPERTY(P_PLAYBACK_STATE, NativeControlPlaybackState, MCiOSPlayerControl, PlaybackState)
    DEFINE_RO_CTRL_PROPERTY(P_NATURAL_SIZE, Int32X2, MCiOSPlayerControl, NaturalSize)
    DEFINE_RO_CTRL_PROPERTY(P_READY_FOR_DISPLAY, Bool, MCiOSPlayerControl, IsReadyForDisplay)
};

MCObjectPropertyTable MCiOSPlayerControl::kPropertyTable =
{
	&MCiOSControl::kPropertyTable,
	sizeof(kProperties) / sizeof(kProperties[0]),
	&kProperties[0],
};

////////////////////////////////////////////////////////////////////////////////

MCNativeControlActionInfo MCiOSPlayerControl::kActions[] =
{
    DEFINE_CTRL_EXEC_METHOD(Play, Void, MCiOSPlayerControl, Play)
    DEFINE_CTRL_EXEC_METHOD(Pause, Void, MCiOSPlayerControl, Pause)
    DEFINE_CTRL_EXEC_METHOD(Stop, Void, MCiOSPlayerControl, Stop)
    DEFINE_CTRL_EXEC_METHOD(PrepareToPlay, Void, MCiOSPlayerControl, PrepareToPlay)
    DEFINE_CTRL_EXEC_METHOD(BeginSeekingForward, Void, MCiOSPlayerControl, BeginSeekingForward)
    DEFINE_CTRL_EXEC_METHOD(BeginSeekingBackward, Void, MCiOSPlayerControl, BeginSeekingBackward)
    DEFINE_CTRL_EXEC_METHOD(EndSeeking, Void, MCiOSPlayerControl, EndSeeking)
    DEFINE_CTRL_EXEC_TERNARY_METHOD(Snapshot, Integer_OptInteger_OptInteger, MCiOSPlayerControl, Int32, OptionalInt32, OptionalInt32, Snapshot)
    DEFINE_CTRL_EXEC_TERNARY_METHOD(SnapshotExactly, Integer_OptInteger_OptInteger, MCiOSPlayerControl, Int32, OptionalInt32, OptionalInt32, SnapshotExactly)
};

MCNativeControlActionTable MCiOSPlayerControl::kActionTable =
{
    &MCiOSControl::kActionTable,
    sizeof(kActions) / sizeof(kActions[0]),
    &kActions[0],
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

	MCExecContext ctxt(nil, nil, nil);
	MCAutoDataRef t_dataref;
	/* UNCHECKED */ MCDataCreateWithBytes((const byte_t *)[t_data bytes], [t_data length], &t_dataref);
	iptr -> setdataprop(ctxt, 0, P_TEXT, false, *t_dataref);	
}

static void content_to_url(MCStringRef p_file, NSURL*& r_url)
{
	NSURL *t_url;
	t_url = nil;
    
	if (MCStringBeginsWith(p_file, MCSTR("http://"), kMCCompareExact) || MCStringBeginsWith(p_file, MCSTR("https://"), kMCCompareExact))
		t_url = [NSURL URLWithString: MCStringConvertToAutoreleasedNSString(p_file )];
	else if (!MCStringIsEmpty(p_file))
	{
		MCAutoStringRef t_path;
		
        MCS_resolvepath(p_file, &t_path);
		t_url = [NSURL fileURLWithPath: MCStringConvertToAutoreleasedNSString(*t_path )];
	}
	else
		t_url = [NSURL URLWithString: @""];

	r_url = t_url;
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
    content_to_url(p_content, t_url);
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
        if (MCStringCreateWithCFStringRef((CFStringRef)t_string, r_content))
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
        t_time = (integer_t)([m_controller initialPlaybackTime] * 1000);
    
    r_time = t_time;
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
        t_time = (integer_t)([m_controller endPlaybackTime] * 1000);
    
    r_time = t_time;
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
        t_time = (integer_t)([m_controller currentPlaybackTime] * 1000);
    
    r_time = t_time;
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
        r_duration = [m_controller duration] * 1000;
    else
        r_duration = 0;
}

void MCiOSPlayerControl::GetPlayableDuration(MCExecContext& ctxt, integer_t& r_duration)
{
    if (m_controller != nil)
        r_duration = [m_controller playableDuration] * 1000;
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

// SN-2015-09-04: [[ Bug 9744 ]] Add getter for readyForDisplay property
void MCiOSPlayerControl::GetIsReadyForDisplay(MCExecContext& ctxt, bool& r_value)
{
    r_value = false;
    
    if (m_controller != nil)
    {
#ifdef __IPHONE_6_0
        if (MCmajorosversion >= 600)
            r_value = [m_controller readyForDisplay];
#endif
    }
}

void MCiOSPlayerControl::GetLoadState(MCExecContext& ctxt, MCNativeControlLoadState& r_state)
{
    uint32_t t_load_state;
    t_load_state = 0;
    
    if (m_controller != nil)
    {
        MPMovieLoadState t_state;
        t_state = [m_controller loadState];
        
        // PM-2015-02-13: [[ Bug 14604 ]] Used (1 << kMCNativeControlLoadState*) to align with the definition of MCExecFormatSet()
        if (t_state & MPMovieLoadStatePlayable)
            t_load_state |= 1 << kMCNativeControlLoadStatePlayable;
        if (t_state & MPMovieLoadStatePlaythroughOK)
            t_load_state |= 1 << kMCNativeControlLoadStatePlaythroughOK;
        if (t_state & MPMovieLoadStateStalled)
            t_load_state |= 1 << kMCNativeControlLoadStateStalled;
    }
    r_state = (MCNativeControlLoadState)t_load_state;
}

void MCiOSPlayerControl::GetPlaybackState(MCExecContext& ctxt, MCNativeControlPlaybackState& r_state)
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

void MCiOSPlayerControl::GetNaturalSize(MCExecContext& ctxt, integer_t r_size[2])
{
    if (m_controller != nil)
    {
        CGSize t_size;
        t_size = [m_controller naturalSize];
        r_size[0] = (int32_t)t_size . width;
        r_size[1] = (int32_t)t_size . height;
    }
    else
    {
        r_size[0] = 0;
        r_size[1] = 0;
    }
}

void MCiOSPlayerControl::Play()
{
    // PM-2014-09-18: [[ Bug 13048 ]] Make sure movieTouched message is sent
    // PM-2015-03-06: [[ Bug 14816 ]] movieTouched msg to be sent only when in fullscreen and showController=false
    [m_delegate beginWithOverlay:([m_controller isFullscreen] && [m_controller controlStyle] == MPMovieControlStyleNone)];
    [m_controller play];
}

void MCiOSPlayerControl::ExecPlay(MCExecContext& ctxt)
{
    Play();
}
void MCiOSPlayerControl::ExecPause(MCExecContext& ctxt)
{
    [m_controller pause];
}
void MCiOSPlayerControl::ExecPrepareToPlay(MCExecContext& ctxt)
{
    [m_controller prepareToPlay];
}
void MCiOSPlayerControl::ExecStop(MCExecContext& ctxt)
{
    [m_controller stop];
}
// PM-2016-02-23: [[ Bug 16984 ]] Make sure the seek direction is respected
void MCiOSPlayerControl::ExecBeginSeekingBackward(MCExecContext& ctxt)
{
    [m_controller beginSeekingBackward];
}

void MCiOSPlayerControl::ExecBeginSeekingForward(MCExecContext& ctxt)
{
    [m_controller beginSeekingForward];
}
void MCiOSPlayerControl::ExecEndSeeking(MCExecContext& ctxt)
{
    [m_controller endSeeking];
}
void MCiOSPlayerControl::ExecSnapshot(MCExecContext& ctxt, integer_t p_time, integer_t *p_max_width, integer_t *p_max_height)
{
    UIImage *t_image;
    t_image = [m_controller thumbnailImageAtTime: p_time / 1000.0
                                      timeOption: MPMovieTimeOptionNearestKeyFrame];
    
    if (t_image == nil)
        return;
    
    p_max_width != nil ? MCIPhoneImportUIImage(t_image, *p_max_width, *p_max_height) : MCIPhoneImportUIImage(t_image, 0, 0);
}

void MCiOSPlayerControl::ExecSnapshotExactly(MCExecContext& ctxt, integer_t p_time, integer_t* p_max_width, integer_t* p_max_height)
{
    UIImage *t_image;
    t_image = [m_controller thumbnailImageAtTime: p_time / 1000.0
                                      timeOption: MPMovieTimeOptionExact];
    
    if (t_image == nil)
        return;
    
    p_max_width != nil ? MCIPhoneImportUIImage(t_image, *p_max_width, *p_max_height) : MCIPhoneImportUIImage(t_image, 0, 0);
}

////////////////////////////////////////////////////////////////////////////////

void MCiOSPlayerControl::HandlePropertyAvailableEvent(const char *p_property)
{
	MCObject *t_target;
	t_target = GetOwner();
	if (t_target != nil)
	{
        MCAutoStringRef t_string;
        /* UNCHECKED */ MCStringCreateWithCString(p_property, &t_string);
        MCNativeControl *t_old_target;
		t_old_target = ChangeTarget(this);
		t_target -> message_with_valueref_args(MCM_player_property_available, *t_string);
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

    m_delegate = [[com_runrev_livecode_MCiOSPlayerDelegate alloc] initWithInstance: this];
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

@implementation com_runrev_livecode_MCiOSPlayerDelegate

- (id)initWithInstance:(MCiOSPlayerControl*)instance
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
    [self end];
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
		MCEventQueuePostCustom(new MCiOSPlayerNotifyEvent(m_instance, t_message));
}

- (void)playerPlaybackStateDidChange: (NSNotification *)notification
{
	MCEventQueuePostCustom(new MCiOSPlayerNotifyEvent(m_instance, MCM_player_state_changed));
}

- (void)playerScalingModeDidChange: (NSNotification *)notification
{
}

- (void)playerWindowTouched: (UIControl*) p_sender
{
    MCEventQueuePostCustom(new MCiOSPlayerNotifyEvent(m_instance, MCM_movie_touched));
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

