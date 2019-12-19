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
#include "mbliphoneapp.h"
#include "mbliphonecontrol.h"

#import <AVFoundation/AVFoundation.h>
#import <AVKit/AVKit.h>

////////////////////////////////////////////////////////////////////////////////

#if __IPHONE_OS_VERSION_MAX_ALLOWED < 100000
	typedef NSString *NSKeyValueChangeKey;
#endif

////////////////////////////////////////////////////////////////////////////////

bool MCParseParameters(MCParameter*& p_parameters, const char *p_format, ...);
UIView *MCIPhoneGetView(void);
UIViewController *MCIPhoneGetViewController(void);

////////////////////////////////////////////////////////////////////////////////

enum MCiOSPlayerStatus
{
	kMCiOSPlayerStatusNone,
	kMCiOSPlayerStatusError,

	kMCiOSPlayerStatusPaused,
	kMCiOSPlayerStatusWaiting,
	kMCiOSPlayerStatusPlaying,
	kMCiOSPlayerStatusStopped,
};

class MCiOSPlayerControl;

@interface com_runrev_livecode_MCiOSPlayerDelegate : NSObject
{
	MCiOSPlayerControl *m_instance;
	AVPlayer *m_player;
	AVPlayerItem *m_player_item;

 	UIControl *m_overlay;
}

- (id)initWithInstance:(MCiOSPlayerControl*)instance player:(AVPlayer*)player;
- (void)setPlayer:(AVPlayer*)player;
- (void)setCurrentItem:(AVPlayerItem*)item;

- (void)beginFullscreenWithOverlay:(BOOL)overlay;
- (void)removeOverlay;
- (void)endFullscreen;
- (void)dealloc;

- (void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary<NSKeyValueChangeKey,id> *)change context:(void *)context;

- (void)playerItemDidPlayToEndTime:(NSNotification*)notification;

/* TODO - update */
//- (void)playerWillEnterFullscreen: (NSNotification *)notification;
//- (void)playerDidExitFullscreen: (NSNotification *)notification;
//- (void)playerMovieChanged: (NSNotification *)notification;
//- (void)playerPlaybackDidFinish: (NSNotification *)notification;

- (void)playerWindowTouched: (UIControl*) p_sender;

@end

@compatibility_alias MCiOSPlayerDelegate com_runrev_livecode_MCiOSPlayerDelegate;

@interface com_livecode_engine_MCiOSAVPlayerViewController : AVPlayerViewController
{
	MCiOSPlayerControl *m_instance;
}

- (id)initWithInstance:(MCiOSPlayerControl*)instance;
- (void)viewDidDisappear:(BOOL)animated;
@end

@compatibility_alias MCiOSAVPlayerViewController com_livecode_engine_MCiOSAVPlayerViewController;

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
	
    virtual void SetVisible(MCExecContext& ctxt, bool p_visible);

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
	
	// Event handling
	void OnPlayerTimeControlStatusChanged();
	void OnPlayerDidPlayToEndTime();
	void OnFullscreenPlayerDismissed(bool p_user);
	
	void HandlePropertyAvailableEvent(MCStringRef p_property);
	void HandleNotifyEvent(MCNameRef p_event);
	
	void ConfigureControllers();
	void ConfigureController(AVPlayerViewController *p_controller);
	
	void UpdateFullscreen();
	
    void Play();
    void Pause();
    void Stop();
    void SeekTo(CMTime p_time);
	void SeekToStart(void);
    void Autoplay();
	
    void Snapshot(integer_t p_time, const integer_t* p_max_width, const integer_t* p_max_height, bool p_exact);
	
protected:
	virtual ~MCiOSPlayerControl(void);
	virtual UIView *CreateView(void);
	virtual void DeleteView(UIView *view);
	
private:
	AVPlayer *m_player;
	AVPlayerViewController *m_controller;
	MCiOSPlayerDelegate *m_delegate;
	UIView *m_container;
	AVPlayerViewController *m_fullscreen_controller;

	CMTime m_start_time;
	CMTime m_end_time;

	MCiOSPlayerStatus m_status;
	MCiOSPlayerStatus m_pending_status;

	bool m_looping;
	bool m_should_autoplay;
	bool m_fullscreen;
	bool m_preserve_aspect;
	bool m_show_controller;

	bool m_did_autoplay;
	bool m_is_fullscreen;
};

////////////////////////////////////////////////////////////////////////////////

MCPropertyInfo MCiOSPlayerControl::kProperties[] =
{
    DEFINE_RW_CTRL_CUSTOM_PROPERTY(P_BACKGROUND_COLOR, NativeControlColor, MCiOSPlayerControl, BackgroundColor)
    DEFINE_RW_CTRL_PROPERTY(P_CONTENT, String, MCiOSPlayerControl, Content)
    DEFINE_RW_CTRL_PROPERTY(P_FULLSCREEN, Bool, MCiOSPlayerControl, Fullscreen)
    DEFINE_RW_CTRL_PROPERTY(P_PRESERVE_ASPECT, Bool, MCiOSPlayerControl, PreserveAspect)
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
	CGFloat t_width, t_height;
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

MCiOSPlayerControl::MCiOSPlayerControl(void)
{
	m_player = nil;
	m_controller = nil;
	m_delegate = nil;
	m_container = nil;
	m_fullscreen_controller = nil;
	
	m_status = kMCiOSPlayerStatusNone;
	m_pending_status = kMCiOSPlayerStatusNone;
	
	m_looping = false;
	m_should_autoplay = false;
	m_fullscreen = false;
	m_preserve_aspect = false;
	m_show_controller = false;

	m_did_autoplay = false;
	m_is_fullscreen = false;

	m_start_time = kCMTimeInvalid;
	m_end_time = kCMTimeInvalid;
}

MCiOSPlayerControl::~MCiOSPlayerControl(void)
{
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
        	m_controller.view.backgroundColor = t_color;
}

void MCiOSPlayerControl::GetBackgroundColor(MCExecContext& ctxt, MCNativeControlColor& r_color)
{
    if (m_controller != nil)
    	FormatColor(m_controller.view.backgroundColor, r_color);
}

void MCiOSPlayerControl::SetContent(MCExecContext& ctxt, MCStringRef p_content)
{
	Stop();
	
    NSURL *t_url;
    content_to_url(p_content, t_url);
    if (m_player != nil)
    {
		AVPlayerItem *t_item;
		t_item = [AVPlayerItem playerItemWithURL:t_url];
		
    	[m_delegate setCurrentItem:t_item];
    	[m_player replaceCurrentItemWithPlayerItem:t_item];

		m_did_autoplay = false;
    }
}

void MCiOSPlayerControl::SetVisible(MCExecContext &ctxt, bool p_visible)
{
	MCiOSControl::SetVisible(ctxt, p_visible);
	UpdateFullscreen();
}

void MCiOSPlayerControl::GetContent(MCExecContext& ctxt, MCStringRef& r_content)
{
    NSString *t_string;
    t_string = nil;
    
    if (m_player != nil)
    {
        NSURL *t_url;
		
        t_url = ((AVURLAsset*)m_player.currentItem.asset).URL;
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
	if (m_fullscreen == p_value)
		return;
	
	m_fullscreen = p_value;
	UpdateFullscreen();
}

void MCiOSPlayerControl::GetFullscreen(MCExecContext& ctxt, bool& r_value)
{
	r_value = m_fullscreen;
}

void MCiOSPlayerControl::SetPreserveAspect(MCExecContext& ctxt, bool p_value)
{
	m_preserve_aspect = p_value;
	ConfigureControllers();
}

void MCiOSPlayerControl::GetPreserveAspect(MCExecContext& ctxt, bool& r_value)
{
	r_value = m_preserve_aspect;
}

void MCiOSPlayerControl::SetShowController(MCExecContext& ctxt, bool p_value)
{
	m_show_controller = p_value;
	ConfigureControllers();
}

void MCiOSPlayerControl::GetShowController(MCExecContext& ctxt, bool& r_value)
{
	r_value = m_show_controller;
}

void MCiOSPlayerControl::SetUseApplicationAudioSession(MCExecContext& ctxt, bool p_value)
{
	/* UNIMPLEMENTED */
}

void MCiOSPlayerControl::GetUseApplicationAudioSession(MCExecContext& ctxt, bool& r_value)
{
	/* UNIMPLEMENTED */
	r_value = false;
}

void MCiOSPlayerControl::SetStartTime(MCExecContext& ctxt, integer_t p_time)
{
	if (p_time == -1)
		m_start_time = kCMTimeInvalid;
	else
		m_start_time = CMTimeMake(p_time, 1000);

    if (m_player != nil)
		m_player.currentItem.reversePlaybackEndTime = m_start_time;
}

void MCiOSPlayerControl::GetStartTime(MCExecContext& ctxt, integer_t& r_time)
{
    integer_t t_time;
    t_time = -1;
    
   	if (CMTIME_IS_INVALID(m_start_time))
   		t_time = -1;
	else
		t_time = m_start_time.value;
	
    r_time = t_time;
}

void MCiOSPlayerControl::SetEndTime(MCExecContext& ctxt, integer_t p_time)
{
	if (p_time == -1)
		m_end_time = kCMTimeInvalid;
	else
		m_end_time = CMTimeMake(p_time, 1000);

    if (m_player != nil)
    	m_player.currentItem.forwardPlaybackEndTime = m_end_time;
}

void MCiOSPlayerControl::GetEndTime(MCExecContext& ctxt, integer_t& r_time)
{
    integer_t t_time;
    t_time = -1;
	
    if (CMTIME_IS_INVALID(m_end_time))
    	t_time = -1;
	else
		t_time = m_end_time.value;
	
    r_time = t_time;
}
void MCiOSPlayerControl::SetCurrentTime(MCExecContext& ctxt, integer_t p_time_in_millis)
{
	SeekTo(CMTimeMake(p_time_in_millis, 1000));
}

void MCiOSPlayerControl::GetCurrentTime(MCExecContext& ctxt, integer_t& r_time_in_millis)
{
    integer_t t_time;
    t_time = -1;
    
    if (m_player != nil)
		t_time = CMTimeGetSeconds(m_player.currentTime) * 1000;
	
    r_time_in_millis = t_time;
}

void MCiOSPlayerControl::SetShouldAutoplay(MCExecContext& ctxt, bool p_value)
{
	m_should_autoplay = p_value;
}

void MCiOSPlayerControl::GetShouldAutoplay(MCExecContext& ctxt, bool& r_value)
{
	r_value = m_should_autoplay;
}

void MCiOSPlayerControl::SetLooping(MCExecContext& ctxt, bool p_value)
{
	m_looping = p_value;
}

void MCiOSPlayerControl::GetLooping(MCExecContext& ctxt, bool& r_value)
{
	r_value = m_looping;
}

void MCiOSPlayerControl::SetAllowsAirPlay(MCExecContext& ctxt, bool p_value)
{
	if (m_player != nil)
		m_player.allowsExternalPlayback = p_value;
}

void MCiOSPlayerControl::GetAllowsAirPlay(MCExecContext& ctxt, bool& r_value)
{
	if (m_player != nil)
		r_value = m_player.allowsExternalPlayback;
	else
		r_value = false;
}

void MCiOSPlayerControl::SetPlayRate(MCExecContext& ctxt, double p_rate)
{
    if (m_player != nil)
    	m_player.rate = p_rate;
}

void MCiOSPlayerControl::GetPlayRate(MCExecContext& ctxt, double& r_rate)
{
    if (m_player != nil)
    	r_rate = m_player.rate;
    else
        r_rate = 0;
}
void MCiOSPlayerControl::GetDuration(MCExecContext& ctxt, integer_t& r_duration)
{
    if (m_player != nil)
    	r_duration = CMTimeGetSeconds(m_player.currentItem.duration);
    else
        r_duration = 0;
}

void MCiOSPlayerControl::GetPlayableDuration(MCExecContext& ctxt, integer_t& r_duration)
{
    if (m_player != nil)
    {
    	// Calculate sum of duration of playable ranges
    	CGFloat t_duration;
    	t_duration = 0;
    	for (NSValue *i in m_player.currentItem.loadedTimeRanges)
    	{
			CMTimeRange t_time_range;
			t_time_range = [i CMTimeRangeValue];
			t_duration += CMTimeGetSeconds(t_time_range.duration);
		}
        r_duration = t_duration * 1000;
	}
    else
        r_duration = 0;
}

void MCiOSPlayerControl::GetIsPreparedToPlay(MCExecContext& ctxt, bool& r_value)
{
    if (m_player != nil)
    	r_value = m_player.status == AVPlayerStatusReadyToPlay;
    else
        r_value = false;
}

// SN-2015-09-04: [[ Bug 9744 ]] Add getter for readyForDisplay property
void MCiOSPlayerControl::GetIsReadyForDisplay(MCExecContext& ctxt, bool& r_value)
{
    r_value = false;
    
    if (m_controller != nil)
    	r_value = m_controller.readyForDisplay;
}

void MCiOSPlayerControl::GetLoadState(MCExecContext& ctxt, MCNativeControlLoadState& r_state)
{
    uint32_t t_load_state;
    t_load_state = 0;
    
    if (m_player != nil)
    {
		AVPlayerItemStatus t_status;
		t_status = m_player.currentItem.status;
		
		if (t_status == AVPlayerItemStatusReadyToPlay)
			t_load_state |= 1 << kMCNativeControlLoadStatePlayable;
		if (m_player.currentItem.playbackLikelyToKeepUp)
			t_load_state |= 1 << kMCNativeControlLoadStatePlaythroughOK;
		if (t_status == AVPlayerItemStatusFailed) /* TODO - CHECK THIS */
			t_load_state |= 1 << kMCNativeControlLoadStateStalled;
    }
    r_state = (MCNativeControlLoadState)t_load_state;
}

void MCiOSPlayerControl::GetPlaybackState(MCExecContext& ctxt, MCNativeControlPlaybackState& r_state)
{
	switch (m_status)
	{
		case kMCiOSPlayerStatusNone:
			r_state = kMCNativeControlPlaybackStateNone;
			return;
		case kMCiOSPlayerStatusPlaying:
			r_state = kMCNativeControlPlaybackStatePlaying;
			return;
		case kMCiOSPlayerStatusWaiting:
			r_state = kMCNativeControlPlaybackStateInterrupted;
			return;
		case kMCiOSPlayerStatusPaused:
			r_state = kMCNativeControlPlaybackStatePaused;
			return;
		case kMCiOSPlayerStatusStopped:
			r_state = kMCNativeControlPlaybackStateStopped;
			return;
			
		default:
			r_state = kMCNativeControlPlaybackStateNone;
			return;
	}
}

void MCiOSPlayerControl::GetNaturalSize(MCExecContext& ctxt, integer_t r_size[2])
{
    if (m_player != nil)
    {
        CGSize t_size;
        t_size = m_player.currentItem.asset.naturalSize;
        r_size[0] = (int32_t)t_size . width;
        r_size[1] = (int32_t)t_size . height;
    }
    else
    {
        r_size[0] = 0;
        r_size[1] = 0;
    }
}

////////////////////////////////////////////////////////////////////////////////

void MCiOSPlayerControl::SeekTo(CMTime p_time)
{
	if (m_player == nil)
		return;
	
	if (CMTIME_IS_INVALID(p_time))
		return;
		
	CMTime t_start_time;
	t_start_time = m_player.currentItem.seekableTimeRanges.firstObject.CMTimeRangeValue.start;

	CMTime t_end_time;
	t_end_time = CMTimeRangeGetEnd(m_player.currentItem.seekableTimeRanges.lastObject.CMTimeRangeValue);

	CMTimeRange t_seekable_range;
	t_seekable_range = CMTimeRangeFromTimeToTime(t_start_time, t_end_time);
	
	[m_player seekToTime:CMTimeClampToRange(p_time, t_seekable_range)];
	
	// if stopped, move to paused status
	if (m_status == kMCiOSPlayerStatusStopped)
		m_status = kMCiOSPlayerStatusPaused;
}

void MCiOSPlayerControl::SeekToStart()
{
	if (m_player == nil)
		return;

	CGFloat t_rate;
	t_rate = m_player.rate;
	
	if (t_rate < 0)
	{
		// playing in reverse - seek to end
		if (CMTIME_IS_VALID(m_end_time))
			SeekTo(m_end_time);
		else
		{
			CMTime t_end_time;
			t_end_time = CMTimeRangeGetEnd(m_player.currentItem.seekableTimeRanges.lastObject.CMTimeRangeValue);
			SeekTo(t_end_time);
		}
	}
	else
	{
		if (CMTIME_IS_VALID(m_start_time))
			SeekTo(m_start_time);
		else
		{
			CMTime t_start_time;
			t_start_time = m_player.currentItem.seekableTimeRanges.firstObject.CMTimeRangeValue.start;
			SeekTo(t_start_time);
		}
	}
}

////////////////////////////////////////////////////////////////////////////////

void MCiOSPlayerControl::Play()
{
	if (m_player == nil)
		return;

	if (m_status == kMCiOSPlayerStatusStopped)
		SeekToStart();

	m_pending_status = kMCiOSPlayerStatusPlaying;
	[m_player play];
}

void MCiOSPlayerControl::Pause()
{
	if (m_player == nil)
		return;

	m_pending_status = kMCiOSPlayerStatusPaused;
	[m_player pause];
}

void MCiOSPlayerControl::Stop()
{
	if (m_player == nil)
		return;

	m_pending_status = kMCiOSPlayerStatusStopped;
	[m_player pause];
}

void MCiOSPlayerControl::Snapshot(integer_t p_time, const integer_t *p_max_width, const integer_t *p_max_height, bool p_exact)
{
	bool t_success;
	t_success = true;
	
	AVAssetImageGenerator *t_gen;
	t_gen = nil;
	
	if (t_success)
	{
		t_gen = [AVAssetImageGenerator assetImageGeneratorWithAsset:m_player.currentItem.asset];
		t_success = t_gen != nil;
	}
	
	CGImageRef t_cg_image;
	t_cg_image = nil;
	if (t_success)
	{
		if (p_exact)
		{
			t_gen.requestedTimeToleranceBefore = kCMTimeZero;
			t_gen.requestedTimeToleranceAfter = kCMTimeZero;
		}
		
		t_cg_image = [t_gen copyCGImageAtTime:CMTimeMake(p_time, 1000) actualTime:nil error:nil];
		t_success = t_cg_image != nil;
	}
	
	UIImage *t_image;
	t_image = nil;
	if (t_success)
	{
		t_image = [UIImage imageWithCGImage:t_cg_image];
		t_success = t_image != nil;
	}

	if (!t_success)
		return;
	
	p_max_width != nil && p_max_height != nil ? MCIPhoneImportUIImage(t_image, *p_max_width, *p_max_height) : MCIPhoneImportUIImage(t_image, 0, 0);
	
	if (t_cg_image != nil)
		CGImageRelease(t_cg_image);
}

void MCiOSPlayerControl::Autoplay()
{
	if (m_should_autoplay && !m_did_autoplay)
	{
		m_did_autoplay = true;
		Play();
	}
}

////////////////////////////////////////////////////////////////////////////////

void MCiOSPlayerControl::ExecPlay(MCExecContext& ctxt)
{
    Play();
}
void MCiOSPlayerControl::ExecPause(MCExecContext& ctxt)
{
	Pause();
}
void MCiOSPlayerControl::ExecPrepareToPlay(MCExecContext& ctxt)
{
	Play();
	Pause();
}
void MCiOSPlayerControl::ExecStop(MCExecContext& ctxt)
{
	Stop();
}
// PM-2016-02-23: [[ Bug 16984 ]] Make sure the seek direction is respected
void MCiOSPlayerControl::ExecBeginSeekingBackward(MCExecContext& ctxt)
{
	/* UNIMPLEMENTED */
}

void MCiOSPlayerControl::ExecBeginSeekingForward(MCExecContext& ctxt)
{
	/* UNIMPLEMENTED */
}
void MCiOSPlayerControl::ExecEndSeeking(MCExecContext& ctxt)
{
	/* UNIMPLEMENTED */
}

void MCiOSPlayerControl::ExecSnapshot(MCExecContext& ctxt, integer_t p_time, integer_t *p_max_width, integer_t *p_max_height)
{
	Snapshot(p_time, p_max_width, p_max_height, false);
}

void MCiOSPlayerControl::ExecSnapshotExactly(MCExecContext& ctxt, integer_t p_time, integer_t* p_max_width, integer_t* p_max_height)
{
	Snapshot(p_time, p_max_width, p_max_height, true);
}

////////////////////////////////////////////////////////////////////////////////

void MCiOSPlayerControl::HandlePropertyAvailableEvent(MCStringRef p_property)
{
	MCObject *t_target;
	t_target = GetOwner();
	if (t_target != nil)
	{
        MCNativeControl *t_old_target;
		t_old_target = ChangeTarget(this);
		t_target -> message_with_valueref_args(MCM_player_property_available, p_property);
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

void MCiOSPlayerControl::ConfigureControllers()
{
	if (m_controller)
		ConfigureController(m_controller);
	if (m_fullscreen_controller)
		ConfigureController(m_fullscreen_controller);
}

void MCiOSPlayerControl::ConfigureController(AVPlayerViewController *p_controller)
{
	p_controller.videoGravity = m_preserve_aspect ? AVLayerVideoGravityResizeAspect : AVLayerVideoGravityResize;
	p_controller.showsPlaybackControls = m_show_controller;
}

////////////////////////////////////////////////////////////////////////////////

void MCiOSPlayerControl::UpdateFullscreen()
{
	bool t_fullscreen;
	t_fullscreen = m_fullscreen && !m_container.hidden;
	
	if (t_fullscreen == m_is_fullscreen)
		return;
	
	m_is_fullscreen = t_fullscreen;
	if (m_controller != nil)
	{
		if (t_fullscreen)
		{
			// remove controller view from container
			[m_controller.view removeFromSuperview];
			m_controller.player = nil;
			
			m_fullscreen_controller = [[MCiOSAVPlayerViewController alloc] initWithInstance:this];
			m_fullscreen_controller.player = m_player;
			ConfigureController(m_fullscreen_controller);
			
			// present player modally
			[MCIPhoneGetViewController()
				presentViewController:m_fullscreen_controller
				animated:NO
				completion:^{
					[m_delegate beginFullscreenWithOverlay:!m_show_controller];
				}];
		}
		else
		{
			if (m_fullscreen_controller != nil)
			{
				// dismiss modal player view
				[MCIPhoneGetViewController()
					dismissViewControllerAnimated:YES
					completion:^{
						OnFullscreenPlayerDismissed(false);
					}];
			}
		}
	}
}

////////////////////////////////////////////////////////////////////////////////

UIView *MCiOSPlayerControl::CreateView(void)
{
	bool t_success;
	t_success = true;
	
	AVPlayer *t_player;
	t_player = nil;
	if (t_success)
	{
		t_player = [AVPlayer playerWithURL:[NSURL URLWithString:@""]];
		t_success = t_player != nil;
	}

	AVPlayerViewController *t_controller;
	t_controller = nil;
	if (t_success)
	{
		t_controller = [[AVPlayerViewController new] autorelease];
		t_success = t_controller != nil;
	}
	
	UIView *t_view;
	t_view = nil;
	if (t_success)
	{
		t_player.actionAtItemEnd = AVPlayerActionAtItemEndNone;
		t_controller.player = t_player;
		t_view = t_controller.view;
		t_success = t_view != nil;
	}
	
	UIView *t_container_view;
	t_container_view = nil;
	if (t_success)
	{
		t_container_view = [[[UIView alloc] initWithFrame:CGRectMake(0, 0, 0, 0)] autorelease];
		t_success = t_container_view != nil;
	}
	
	MCiOSPlayerDelegate *t_delegate;
	t_delegate = nil;
	if (t_success)
	{
		t_delegate = [[[MCiOSPlayerDelegate alloc] initWithInstance:this player:t_player] autorelease];
		t_success = t_delegate != nil;
	}
	
	if (t_success)
	{
		t_container_view.hidden = YES;
		t_container_view.autoresizesSubviews = YES;
		
		t_view.hidden = NO;
		t_view.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
		
		[t_container_view addSubview:t_view];
		t_view.frame = t_container_view.bounds;

		[MCIPhoneGetViewController() addChildViewController:t_controller];
		
		m_player = [t_player retain];
		m_controller = [t_controller retain];
		m_delegate = [t_delegate retain];
		m_container = [t_container_view retain];
		
		return m_container;
	}
	else
		return nil;
}

void MCiOSPlayerControl::DeleteView(UIView *p_view)
{
	if (m_player != nil)
	{
		[m_delegate release];
		m_delegate = nil;
		
		m_controller.player = nil;
		[m_player release];
		m_player = nil;
		
		[m_controller removeFromParentViewController];
		[m_controller release];
		m_controller = nil;

		[m_container release];
		m_container = nil;
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
		m_property = nil;
		/* UNCHECKED */ MCStringCreateWithCString(p_property, m_property);
	}
	
	void Destroy(void)
	{
		m_target -> Release();
		if (m_property != nil)
			MCValueRelease(m_property);
		delete this;
	}
	
	void Dispatch(void)
	{
		m_target -> HandlePropertyAvailableEvent(m_property);
	}
	
private:
	MCiOSPlayerControl *m_target;
	MCStringRef m_property;
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

////////////////////////////////////////////////////////////////////////////////

static void MCiOSPlayerPostPropertyAvailableEvent(MCiOSPlayerControl *p_target, const char *p_property)
{
	MCIPhoneRunBlockOnMainFiber(^{
		MCEventQueuePostCustom(new MCiOSPlayerPropertyAvailableEvent(p_target, p_property));
	});
}

static void MCiOSPlayerPostNotifyEvent(MCiOSPlayerControl *p_target, MCNameRef p_message)
{
	MCIPhoneRunBlockOnMainFiber(^{
		MCEventQueuePostCustom(new MCiOSPlayerNotifyEvent(p_target, p_message));
	});
}

////////////////////////////////////////////////////////////////////////////////

void MCiOSPlayerControl::OnPlayerTimeControlStatusChanged()
{
#if __IPHONE_OS_VERSION_MAX_ALLOWED >= 100000
	if (m_player == nil)
		return;
	
	if (@available(iOS 10.0, *))
	{
		MCiOSPlayerStatus t_old_status;
		t_old_status = m_status;

		switch (m_player.timeControlStatus)
		{
			case AVPlayerTimeControlStatusWaitingToPlayAtSpecifiedRate:
				m_status = kMCiOSPlayerStatusWaiting;
				break;
				
			case AVPlayerTimeControlStatusPaused:
				if (m_pending_status == kMCiOSPlayerStatusStopped)
					m_status = kMCiOSPlayerStatusStopped;
				else
					m_status = kMCiOSPlayerStatusPaused;
				break;
				
			case AVPlayerTimeControlStatusPlaying:
				m_status = kMCiOSPlayerStatusPlaying;
				break;
		}
		
		// notify on status change
		if (m_status != t_old_status)
			MCiOSPlayerPostNotifyEvent(this, MCM_player_state_changed);
	}
#endif
}

void MCiOSPlayerControl::OnPlayerDidPlayToEndTime()
{
	MCiOSPlayerPostNotifyEvent(this, MCM_player_finished);
	if (m_looping)
	{
		// reset back to start (or end if playing backwards)
		SeekToStart();
	}
	else
		Stop();
}

void MCiOSPlayerControl::OnFullscreenPlayerDismissed(bool p_user)
{
	[m_delegate endFullscreen];

	m_fullscreen_controller.player = nil;
	[m_fullscreen_controller release];
	m_fullscreen_controller = nil;

	// reattach controller view to container
	m_controller.player = m_player;
	[m_container addSubview:m_controller.view];
	m_controller.view.frame = m_container.bounds;
	
	if (p_user)
	{
		m_is_fullscreen = false;
		m_fullscreen = false;
		MCiOSPlayerPostNotifyEvent(this, MCM_player_stopped);
	}
}

////////////////////////////////////////////////////////////////////////////////

@implementation com_livecode_engine_MCiOSAVPlayerViewController
{
}

- (id)initWithInstance:(MCiOSPlayerControl*)instance
{
	self = [super init];
	if (self == nil)
		return nil;

	m_instance = instance;
	
	return self;
}

- (void)viewDidDisappear:(BOOL)animated
{
	[super viewDidDisappear:animated];
	m_instance->OnFullscreenPlayerDismissed(true);
}

@end

////////////////////////////////////////////////////////////////////////////////

static struct { NSString *const *name; SEL selector; } s_player_item_notifications[] =
{
	{&AVPlayerItemDidPlayToEndTimeNotification, @selector(playerItemDidPlayToEndTime:)},
	{&AVPlayerItemFailedToPlayToEndTimeNotification, @selector(playerItemFailedToPlayToEndTime:)},
};
#define s_player_item_notification_count (sizeof(s_player_item_notifications) / sizeof(s_player_item_notifications[0]))

static NSString* s_player_item_keys[] =
{
	@"status",
	@"playbackLikelyToKeepUp",
};
#define s_player_item_key_count (sizeof(s_player_item_keys) / sizeof(s_player_item_keys[0]))

static NSString* s_player_asset_async_keys[] =
{
	@"duration",
	@"naturalSize",
};
#define s_player_asset_async_key_count (sizeof(s_player_asset_async_keys) / sizeof(s_player_asset_async_keys[0]))

#if __IPHONE_OS_VERSION_MAX_ALLOWED >= 100000
static NSString* s_player_keys[] =
{
	@"timeControlStatus",
};
#define s_player_key_count (sizeof(s_player_keys) / sizeof(s_player_keys[0]))
#else
static NSString* s_player_keys[] =
{
};
#define s_player_key_count (0)
#endif

////////////////////////////////////////////////////////////////////////////////

@implementation com_runrev_livecode_MCiOSPlayerDelegate

- (id)initWithInstance:(MCiOSPlayerControl*)instance player:(AVPlayer*)player
{
	self = [super init];
	if (self == nil)
		return nil;
	
	m_instance = instance;
	m_player = nil;
	m_player_item = nil;
	m_overlay = nil;

	[self setPlayer:player];
	
	return self;
}

- (void)setPlayer:(AVPlayer *)player
{
	if (player == m_player)
		return;
	
	if (m_player != nil)
	{
		for (uint32_t i = 0; i < s_player_key_count; i++)
			[m_player removeObserver:self forKeyPath:s_player_keys[i]];
		
		[m_player release];
	}

	m_player = [player retain];
	
	if (m_player != nil)
	{
		for (uint32_t i = 0; i < s_player_key_count; i++)
		{
			[m_player addObserver:self
				forKeyPath:s_player_keys[i]
				options:NSKeyValueObservingOptionNew | NSKeyValueObservingOptionOld
				context:nil];
		}
	}
}

- (void)setCurrentItem:(AVPlayerItem *)item
{
	if (item == m_player_item)
		return;
	
	if (m_player_item != nil)
	{
		for (uint32_t i = 0; i < s_player_item_key_count; i++)
			[m_player_item removeObserver:self forKeyPath:s_player_item_keys[i]];
		
		for (uint32_t i = 0; i < s_player_item_notification_count; i++)
			[[NSNotificationCenter defaultCenter] removeObserver:self name:*s_player_item_notifications[i].name object:m_player_item];
		
		[m_player_item.asset cancelLoading];
		[m_player_item release];
		m_player_item = nil;
	}
	
	m_player_item = item;
	
	if (m_player_item != nil)
	{
		[m_player_item retain];
		
		for (uint32_t i = 0; i < s_player_item_key_count; i++)
		{
			[m_player_item addObserver:self
				forKeyPath:s_player_item_keys[i]
				options:NSKeyValueObservingOptionNew | NSKeyValueObservingOptionOld
				context:nil];
		}

		for(uint32_t i = 0; i < s_player_item_notification_count; i++)
			if (*s_player_item_notifications[i] . name != nil)
				[[NSNotificationCenter defaultCenter] addObserver: self
														 selector: s_player_item_notifications[i] . selector
															 name: *s_player_item_notifications[i] . name
														   object: m_player_item];
		
		[m_player_item.asset
			loadValuesAsynchronouslyForKeys:[NSArray arrayWithObjects:s_player_asset_async_keys count:s_player_asset_async_key_count]
			completionHandler:^{
				for (uindex_t i = 0; i < s_player_asset_async_key_count; i++)
				{
					NSError *t_error;
					t_error = nil;
					if ([m_player_item.asset statusOfValueForKey:s_player_asset_async_keys[i] error:nil] == AVKeyValueStatusLoaded)
					{
						MCiOSPlayerPostPropertyAvailableEvent(m_instance, s_player_asset_async_keys[i].UTF8String);
					}
				}
			}];
	}
}

- (void)beginFullscreenWithOverlay:(BOOL)overlay
{
	if (overlay)
	{
		// The movie's window is the one that is active
		UIWindow *t_window = [[UIApplication sharedApplication] keyWindow];

		// Now we create an invisible control with the same size as the window
		m_overlay = [[UIControl alloc] initWithFrame: [t_window frame]];
		
		// We want to get notified whenever the overlay control is touched
		[m_overlay addTarget: self action: @selector(playerWindowTouched:) forControlEvents: UIControlEventTouchDown];
		[t_window addSubview: m_overlay];
	}

	MCiOSPlayerPostNotifyEvent(m_instance, MCM_player_enter_fullscreen);
}

- (void)removeOverlay
{
    if (m_overlay != nil)
    {
        [m_overlay removeTarget: self action: @selector(playerWindowTouched:) forControlEvents: UIControlEventTouchDown];
        [m_overlay removeFromSuperview];
        [m_overlay release];
        m_overlay = nil;
    }
}

- (void)endFullscreen
{
	[self removeOverlay];
	MCiOSPlayerPostNotifyEvent(m_instance, MCM_player_leave_fullscreen);
}

- (void)dealloc
{
	[self removeOverlay];
	[self setCurrentItem:nil];
	[self setPlayer:nil];
	[super dealloc];
}

- (void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary<NSKeyValueChangeKey,id> *)change context:(void *)context
{
	if ([keyPath isEqualToString:@"status"])
	{
		MCiOSPlayerPostNotifyEvent(m_instance, MCM_player_progress_changed);
	}
	else if ([keyPath isEqualToString:@"playbackLikelyToKeepUp"])
	{
		MCiOSPlayerPostNotifyEvent(m_instance, MCM_player_progress_changed);
		if (m_player_item.playbackLikelyToKeepUp)
			m_instance->Autoplay();
	}
	else if ([keyPath isEqualToString:@"timeControlStatus"])
	{
		m_instance->OnPlayerTimeControlStatusChanged();
	}
}

- (void)playerItemDidPlayToEndTime:(NSNotification *)notification
{
	m_instance->OnPlayerDidPlayToEndTime();
}

- (void)playeritemFailedToPlayToEndTime:(NSNotification*)notification
{
	MCiOSPlayerPostNotifyEvent(m_instance, MCM_player_error);
}

- (void)playerWindowTouched: (UIControl*) p_sender
{
	MCiOSPlayerPostNotifyEvent(m_instance, MCM_movie_touched);
}

@end

////////////////////////////////////////////////////////////////////////////////

bool MCNativePlayerControlCreate(MCNativeControl*& r_control)
{
	r_control = new MCiOSPlayerControl;
	return true;
}

////////////////////////////////////////////////////////////////////////////////

