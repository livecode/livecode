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

#include <Cocoa/Cocoa.h>
#include <AVFoundation/AVFoundation.h>

#include "globdefs.h"
#include "imagebitmap.h"
#include "region.h"
#include "notify.h"

#include "platform.h"
#include "platform-internal.h"

#include "mac-internal.h"

#include "mac-player.h"
#include "graphics_util.h"

////////////////////////////////////////////////////////////////////////////////

class MCAVFoundationPlayer;

@interface com_runrev_livecode_MCAVFoundationPlayerObserver: NSObject
{
    MCAVFoundationPlayer *m_av_player;
}

- (id)initWithPlayer: (MCAVFoundationPlayer *)player;

- (void)movieFinished: (id)object;

// PM-2014-08-05: [[ Bug 13105 ]] Make sure a currenttimechanged message is sent when we click step forward/backward buttons
- (void)timeJumped: (id)object;

- (void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary *)change context:(void *)context;

- (void)updateCurrentFrame;

@end

@interface com_runrev_livecode_MCAVFoundationPlayerView : NSView

- (AVPlayer*)player;
- (void)setPlayer:(AVPlayer *)player;

@end

class MCAVFoundationPlayer: public MCPlatformPlayer
{
public:
	MCAVFoundationPlayer(void);
	virtual ~MCAVFoundationPlayer(void);
    
	virtual bool IsPlaying(void);
	virtual void Start(double rate);
	virtual void Stop(void);
	virtual void Step(int amount);
    
	virtual void LockBitmap(MCImageBitmap*& r_bitmap);
	virtual void UnlockBitmap(MCImageBitmap *bitmap);
    
	virtual void SetProperty(MCPlatformPlayerProperty property, MCPlatformPropertyType type, void *value);
	virtual void GetProperty(MCPlatformPlayerProperty property, MCPlatformPropertyType type, void *value);
    
	virtual void CountTracks(uindex_t& r_count);
	virtual bool FindTrackWithId(uint32_t id, uindex_t& r_index);
	virtual void SetTrackProperty(uindex_t index, MCPlatformPlayerTrackProperty property, MCPlatformPropertyType type, void *value);
	virtual void GetTrackProperty(uindex_t index, MCPlatformPlayerTrackProperty property, MCPlatformPropertyType type, void *value);
    
    void MovieFinished(void);
    void TimeJumped(void);
    void MovieIsLoading(CMTimeRange p_timerange);
    
    AVPlayer *getPlayer(void);
    
    static void DoUpdateCurrentFrame(void *ctxt);
    
protected:
	virtual void Realize(void);
	virtual void Unrealize(void);
    
private:
	void Load(MCStringRef filename, bool is_url);
	void Synchronize(void);
	void Switch(bool new_offscreen);
    
    CMTime CMTimeFromLCTime(uint32_t lc_time);
    uint32_t CMTimeToLCTime(CMTime cm_time);
    
    void SeekToTimeAndWait(uint32_t p_lc_time);
    
    void HandleCurrentTimeChanged(void);
    
    void CacheCurrentFrame(void);
    static CVReturn MyDisplayLinkCallback (CVDisplayLinkRef displayLink,
                                                          const CVTimeStamp *inNow,
                                                          const CVTimeStamp *inOutputTime,
                                                          CVOptionFlags flagsIn,
                                                          CVOptionFlags *flagsOut,
                                    void *displayLinkContext);
    
	static void DoSwitch(void *context);
    static void DoUpdateCurrentTime(void *ctxt);
    
    NSLock *m_lock;
    
	AVPlayer *m_player;
    AVPlayerItemVideoOutput *m_player_item_video_output;
    CVDisplayLinkRef m_display_link;
    com_runrev_livecode_MCAVFoundationPlayerView *m_view;
    uint32_t m_selection_start, m_selection_finish;
    uint32_t m_selection_duration;
    uint32_t m_buffered_time;
    CMTimeScale m_time_scale;
    
    bool m_play_selection_only : 1;
    bool m_looping : 1;
    
	CVImageBufferRef m_current_frame;
    //CGImageRef m_current_frame;
    CMTime m_next_frame_time;
    CMTime m_observed_time;
    
    com_runrev_livecode_MCAVFoundationPlayerObserver *m_observer;
    
    id m_time_observer_token;
    
    // MW-2014-10-22: [[ Bug 13267 ]] Use an end-time observer rather than the built in 'action at end' (10.8 bug).
    id m_endtime_observer_token;
    
    uint32_t *m_markers;
    uindex_t m_marker_count;
    uint32_t m_last_marker;
    
	MCRectangle m_rect;
	bool m_visible : 1;
	bool m_offscreen : 1;
	bool m_pending_offscreen : 1;
	bool m_switch_scheduled : 1;
    bool m_playing : 1;
    bool m_synchronizing : 1;
    bool m_frame_changed_pending : 1;
    bool m_finished : 1;
    bool m_loaded : 1;
    bool m_stepped : 1;
    bool m_has_invalid_filename : 1;
};

////////////////////////////////////////////////////////////////////////////////

@implementation com_runrev_livecode_MCAVFoundationPlayerObserver

- (id)initWithPlayer: (MCAVFoundationPlayer *)player
{
    self = [super init];
    if (self == nil)
        return nil;
    
    m_av_player = player;
    
    return self;
}

// PM-2014-08-05: [[ Bug 13105 ]] Make sure a currenttimechanged message is sent when we click step forward/backward buttons
- (void)timeJumped: (id)object
{
    m_av_player -> TimeJumped();
}

- (void)movieFinished: (id)object
{
    m_av_player -> MovieFinished();
}

- (void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary *)change context:(void *)context
{
    if ([keyPath isEqualToString: @"status"])
        MCPlatformBreakWait();
    else if([keyPath isEqualToString: @"currentItem.loadedTimeRanges"])
    {
        // PM-2014-10-14: [[ Bug 13650 ]] Do this check to prevent a crash
        if([change objectForKey:NSKeyValueChangeNewKey] != [NSNull null])
        {
            NSArray *t_time_ranges = (NSArray *)[change objectForKey:NSKeyValueChangeNewKey];
            
            if (t_time_ranges && [t_time_ranges count])
            {
                CMTimeRange timerange = [[t_time_ranges objectAtIndex:0] CMTimeRangeValue];
                m_av_player -> MovieIsLoading(timerange);
            }
        }
    }
}

- (void)updateCurrentFrame
{
    m_av_player -> DoUpdateCurrentFrame(m_av_player);
}

@end

/////////////////////////////////////////////////////////////////////////////////

@implementation com_runrev_livecode_MCAVFoundationPlayerView

- (AVPlayer*)player
{
    if ([self layer] != nil)
        return [(AVPlayerLayer *)[self layer] player];
    return nil;
}

- (void)setPlayer:(AVPlayer *)player
{
    if ([self layer] != nil)
    {
        [(AVPlayerLayer *)[self layer] setPlayer:player];
        return;
    }
    
    AVPlayerLayer *t_layer;
    t_layer = [AVPlayerLayer playerLayerWithPlayer: player];
    [t_layer setVideoGravity: AVLayerVideoGravityResize];
    [self setLayer: t_layer];
    [self setWantsLayer: YES];
}

@end
////////////////////////////////////////////////////////


MCAVFoundationPlayer::MCAVFoundationPlayer(void)
{
    // COMMENT: We can follow the same pattern as QTKitPlayer here - however we won't have
    //   an AVPlayer until we load, so that starts off as nil and we create a PlayerView
    //   with zero frame.
    m_player = nil;
    m_view = [[com_runrev_livecode_MCAVFoundationPlayerView alloc] initWithFrame: NSZeroRect];
	m_observer = [[com_runrev_livecode_MCAVFoundationPlayerObserver alloc] initWithPlayer: this];
    
    m_lock = [[NSLock alloc] init];
    
    m_player_item_video_output = nil;
    CVDisplayLinkCreateWithActiveCGDisplays(&m_display_link);
    
	m_current_frame = nil;
    
    m_markers = nil;
    m_marker_count = 0;
    
    m_play_selection_only = false;
    m_looping = false;
    
	m_rect = MCRectangleMake(0, 0, 0, 0);
	m_visible = true;
	m_offscreen = false;
	m_pending_offscreen = false;
    
	m_switch_scheduled = false;
    
    m_playing = false;
    
    m_synchronizing = false;
    m_frame_changed_pending = false;
    m_finished = false;
    m_loaded = false;
    
    m_time_scale = 0;
    
    m_selection_start = 0;
    m_selection_finish = 0;
    m_stepped = false;
    m_buffered_time = 0;
    
    m_time_observer_token = nil;
    m_endtime_observer_token = nil;
}

MCAVFoundationPlayer::~MCAVFoundationPlayer(void)
{
	if (m_current_frame != nil)
		CFRelease(m_current_frame);
    
    // First detach the observer from everything we've attached it to.
    if (m_time_observer_token != nil)
        [m_player removeTimeObserver:m_time_observer_token];
    
    // MW-2014-10-22: [[ Bug 13267 ]] Remove the end-time observer.
    if (m_endtime_observer_token != nil)
        [m_player removeTimeObserver:m_endtime_observer_token];
    
    @try
    {
        [m_player removeObserver: m_observer forKeyPath: @"currentItem.loadedTimeRanges"];
    }
    @catch (id anException) {
        //do nothing, obviously it wasn't attached because an exception was thrown
    }
    
    [[NSNotificationCenter defaultCenter] removeObserver: m_observer];
    // Now we can release it.
    [m_observer release];
    
    CVDisplayLinkRelease(m_display_link);
    
    // Now detach the player from everything we've attached it to.
    [m_view setPlayer: nil];
    // No we can release it.
    [m_view release];
    
    // Finally we can release the player.
    [m_player release];
    
    MCMemoryDeleteArray(m_markers);
    
    [m_lock release];
}

void MCAVFoundationPlayer::TimeJumped(void)
{
    //MCLog("Time Jumped!", nil);
}

void MCAVFoundationPlayer::MovieIsLoading(CMTimeRange p_timerange)
{
    uint32_t t_buffered_time;
    t_buffered_time = CMTimeToLCTime(p_timerange.duration);
    m_buffered_time = t_buffered_time;
    MCPlatformCallbackSendPlayerBufferUpdated(this);
    /*
    float t_movie_duration, t_loaded_part;
    t_movie_duration = (float)CMTimeToLCTime(m_player.currentItem.duration);
    t_loaded_part = (float)CMTimeToLCTime(p_timerange.duration);
    MCLog(" Start is %d", CMTimeToLCTime(p_timerange.start));
    MCLog(" Duration is %d", CMTimeToLCTime(p_timerange.duration));
    MCLog("=============Loaded %.2f / 1.00", t_loaded_part/t_movie_duration);
    */
}

void MCAVFoundationPlayer::MovieFinished(void)
{
    m_finished = true;
    m_stepped = false;
    
    if (m_offscreen)
        CVDisplayLinkStop(m_display_link);
    
    if (!m_looping)
    {
        m_playing = false;
        MCPlatformCallbackSendPlayerFinished(this);
    }
    else
    {
        // PM-2014-07-15: [[ Bug 12812 ]] Make sure we loop within start and finish time when playSelection is true 
        if (m_play_selection_only && m_selection_duration > 0)
            // Note: Calling breakSeekToTimeAndWait(m_selection_start) here causes loop property to break
            [[m_player currentItem] seekToTime:CMTimeFromLCTime(m_selection_start) toleranceBefore:kCMTimeZero toleranceAfter:kCMTimeZero];
        else
            // Note: Calling SeekToTimeAndWait(0) here causes loop property to break
            [[m_player currentItem] seekToTime:kCMTimeZero toleranceBefore:kCMTimeZero toleranceAfter:kCMTimeZero];
        
        if (m_offscreen)
            CVDisplayLinkStart(m_display_link);

        [m_player play];
        m_playing = true;
        m_finished = false;
    }
}

void MCAVFoundationPlayer::HandleCurrentTimeChanged(void)
{
    int32_t t_current_time;
    t_current_time = CMTimeToLCTime([m_player currentTime]);
    
    // PM-2014-10-28: [[ Bug 13773 ]] If the thumb is after the first marker and the user drags it before the first marker, then we have to reset m_last marker, so as to be dispatched
    if (t_current_time < m_last_marker)
        m_last_marker = -1;
    
    // PM-2014-08-12: [[ Bug 13156 ]] When clicked'n'hold the back button of the controller, t_current_time was negative after returning to the start of the video, and this was causing the last callback of the queue to be invoked after the first one. So make sure that t_current_time is valid.
    
    if (t_current_time > CMTimeToLCTime(m_player.currentItem.asset.duration) || t_current_time < 0)
        return;
    
    if (m_marker_count > 0)
    {
        // We search for the marker time immediately before the
        // current time and if last marker is not that time,
        // dispatch it.
        uindex_t t_index;
        for(t_index = 0; t_index < m_marker_count; t_index++)
            if (m_markers[t_index] > t_current_time)
                break;
        
        // t_index is now the first marker greater than the current time.
        if (t_index > 0)
        {
            if (m_markers[t_index - 1] != m_last_marker)
            {
                m_last_marker = m_markers[t_index - 1];
                MCPlatformCallbackSendPlayerMarkerChanged(this, m_last_marker);
                m_synchronizing = true;
            }
        }
    }
    
    // PM-2014-10-28: [[ Bug 13773 ]] Make sure we don't send a currenttimechanged messsage if the callback is processed
    if (!m_synchronizing && IsPlaying())
        MCPlatformCallbackSendPlayerCurrentTimeChanged(this);
    
    m_synchronizing = false;
    
}

void MCAVFoundationPlayer::CacheCurrentFrame(void)
{
    // PM_2014-12-17: [[ Bug 14233]] Now currentItem can be nil, if setting an empty or invalid filename
    if ([m_player currentItem] == nil)
        return;
    CVImageBufferRef t_image;
    CMTime t_output_time = [m_player currentItem] . currentTime;
    
    t_image = [m_player_item_video_output copyPixelBufferForItemTime:t_output_time itemTimeForDisplay:nil];
    if (t_image != nil)
    {
        if (m_current_frame != nil)
            CFRelease(m_current_frame);
        m_current_frame = t_image;
    }
}

void MCAVFoundationPlayer::Switch(bool p_new_offscreen)
{
	// If the new setting is the same as the pending setting, do nothing.
	if (p_new_offscreen == m_pending_offscreen)
		return;
    
	// Update the pending offscreen setting and schedule a switch.
	m_pending_offscreen = p_new_offscreen;
    
	if (m_switch_scheduled)
		return;
    
	Retain();
	MCMacPlatformScheduleCallback(DoSwitch, this);
    
	m_switch_scheduled = true;
}

void MCAVFoundationPlayer::SeekToTimeAndWait(uint32_t p_time)
{
    // PM-2014-08-15: [[ Bug 13193 ]] Do this check to prevent LC hanging if filename is invalid the very first time you open a stack with a player object
    if (m_player == nil || m_player.currentItem == nil)
        return;
    
    __block bool t_is_finished = false;
    [[m_player currentItem] seekToTime:CMTimeFromLCTime(p_time) toleranceBefore:kCMTimeZero toleranceAfter:kCMTimeZero completionHandler:^(BOOL finished) {
        t_is_finished = true;
        MCPlatformBreakWait();
    }];
    while(!t_is_finished)
        MCPlatformWaitForEvent(60.0, true);
}

CVReturn MCAVFoundationPlayer::MyDisplayLinkCallback (CVDisplayLinkRef displayLink,
                                const CVTimeStamp *inNow,
                                const CVTimeStamp *inOutputTime,
                                CVOptionFlags flagsIn,
                                CVOptionFlags *flagsOut,
                                void *displayLinkContext)
{
    MCAVFoundationPlayer *t_self = (MCAVFoundationPlayer *)displayLinkContext;
        
    CMTime t_output_item_time = [t_self -> m_player_item_video_output itemTimeForCVTimeStamp:*inOutputTime];
    
    if (![t_self -> m_player_item_video_output hasNewPixelBufferForItemTime:t_output_item_time])
    {
        return kCVReturnSuccess;
    }
    
    bool t_need_update;
    
    [t_self -> m_lock lock];
    t_need_update = !t_self -> m_frame_changed_pending;
    t_self -> m_next_frame_time = t_output_item_time;
    t_self -> m_frame_changed_pending = true;
    [t_self -> m_lock unlock];
    
    // Frame updates don't need to happen at 'safe points' (unlike currentTimeChanged events) so
    // we pass false as the second argument to the notify.
    
    if (t_need_update)
        [t_self -> m_observer performSelectorOnMainThread: @selector(updateCurrentFrame) withObject: nil waitUntilDone: NO];
    
    return kCVReturnSuccess;
    
}

void MCAVFoundationPlayer::DoUpdateCurrentTime(void *ctxt)
{
    MCAVFoundationPlayer *t_player;
	t_player = (MCAVFoundationPlayer *)ctxt;
    
    t_player -> HandleCurrentTimeChanged();
}

void MCAVFoundationPlayer::DoUpdateCurrentFrame(void *ctxt)
{
    MCAVFoundationPlayer *t_player;
	t_player = (MCAVFoundationPlayer *)ctxt;
    
    if (!t_player -> m_frame_changed_pending)
        return;
    
    CVImageBufferRef t_image;
    [t_player -> m_lock lock];
    t_image = [t_player -> m_player_item_video_output copyPixelBufferForItemTime:t_player -> m_next_frame_time itemTimeForDisplay:nil];
    t_player -> m_frame_changed_pending = false;
    [t_player -> m_lock unlock];
    
    if (t_image != nil)
    {
        if (t_player -> m_current_frame != nil)
            CFRelease(t_player -> m_current_frame);
        t_player -> m_current_frame = t_image;
    }

    // Now video is loaded
    t_player -> m_loaded = true;
	MCPlatformCallbackSendPlayerFrameChanged(t_player);
    
    if (t_player -> IsPlaying())
        t_player -> HandleCurrentTimeChanged();
}


void MCAVFoundationPlayer::DoSwitch(void *ctxt)
{
	MCAVFoundationPlayer *t_player;
	t_player = (MCAVFoundationPlayer *)ctxt;
	t_player -> m_switch_scheduled = false;
    
	if (t_player -> m_pending_offscreen == t_player -> m_offscreen)
	{
		// Do nothing if there is no state change.
	}
	else if (t_player -> m_pending_offscreen)
    {
        // PM-2014-07-08: [[ Bug 12722 ]] Player should stop playing when switching from run to edit mode
        if (t_player -> IsPlaying())
            t_player -> Stop();

        t_player -> CacheCurrentFrame();
        
		if (t_player -> m_view != nil)
			t_player -> Unrealize();
        
        // PM_2014-12-17: [[ Bug 14233]] Now currentItem can be nil, if setting an empty or invalid filename
        if (!CVDisplayLinkIsRunning(t_player -> m_display_link) && t_player -> m_player.currentItem != nil)
        {
            CVDisplayLinkStart(t_player -> m_display_link);
        }
        

		t_player -> m_offscreen = t_player -> m_pending_offscreen;
	}
	else
	{
		if (t_player -> m_current_frame != nil)
		{
			CFRelease(t_player -> m_current_frame);
			t_player -> m_current_frame = nil;
		}
		
        if (CVDisplayLinkIsRunning(t_player -> m_display_link))
        {
            CVDisplayLinkStop(t_player -> m_display_link);
        }

        
		// Switching to non-offscreen
		t_player -> m_offscreen = t_player -> m_pending_offscreen;
		t_player -> Realize();
    
	}
    t_player -> Release();
}

void MCAVFoundationPlayer::Realize(void)
{
	if (m_window == nil)
		return;
    
	MCMacPlatformWindow *t_window;
	t_window = (MCMacPlatformWindow *)m_window;
    
	if (!m_offscreen)
	{
		MCWindowView *t_parent_view;
		t_parent_view = t_window -> GetView();
		[t_parent_view addSubview: m_view];
	}
    
	Synchronize();
}

void MCAVFoundationPlayer::Unrealize(void)
{
	if (m_offscreen || m_window == nil)
		return;
    
	if (!m_offscreen)
	{
		MCMacPlatformWindow *t_window;
		t_window = (MCMacPlatformWindow *)m_window;
        
		MCWindowView *t_parent_view;
		t_parent_view = t_window -> GetView();
        
		[m_view removeFromSuperview];
	}
}

void MCAVFoundationPlayer::Load(MCStringRef p_filename_or_url, bool p_is_url)
{
    // Ensure that removing the video source from the property inspector results immediately in empty player with the controller thumb in the beginning
    if (MCStringIsEmpty(p_filename_or_url))
    {
        m_player_item_video_output = nil;
        [m_view setPlayer: nil];
        uint4 t_zero_time = 0;
        // PM-2014-12-17: [[ Bug 14233 ]] Setting the filename to empty should reset the currentItem
        [m_player replaceCurrentItemWithPlayerItem:nil];
        SetProperty(kMCPlatformPlayerPropertyCurrentTime, kMCPlatformPropertyTypeUInt32, &t_zero_time);
        return;
    }

    id t_url;
    if (!p_is_url)
        t_url = [NSURL fileURLWithPath: [NSString stringWithMCStringRef: p_filename_or_url]];
    else
        t_url = [NSURL URLWithString: [NSString stringWithMCStringRef: p_filename_or_url]];

    AVPlayer *t_player;
    t_player = [[AVPlayer alloc] initWithURL: t_url];

    // PM-2014-08-19 [[ Bug 13121 ]] Added feature for displaying download progress
    if (p_is_url)
        [t_player addObserver:m_observer forKeyPath:@"currentItem.loadedTimeRanges" options:NSKeyValueObservingOptionNew context:nil];
    
    // Block-wait until the status becomes something.
    [t_player addObserver: m_observer forKeyPath: @"status" options: 0 context: nil];
    while([t_player status] == AVPlayerStatusUnknown)
        MCPlatformWaitForEvent(60.0, true);

    [t_player removeObserver: m_observer forKeyPath: @"status"];

    // If we've failed, leave things as they are (dealloc the new player).
    if ([t_player status] == AVPlayerStatusFailed)
    {
        // error obtainable via [t_player error]
        if (p_is_url)
            [t_player removeObserver: m_observer forKeyPath: @"currentItem.loadedTimeRanges"];
        [t_player release];
        return;
    }

    /*
        PM-2014-07-07: [[Bugs 12758 and 12760]] When the filename is set to a URL or to a local file
        that is not a video, or does not exist, then currentItem is nil. Do this chack to prevent a crash
    */
    if ([t_player currentItem] == nil)
    {
        if(p_is_url)
            [t_player removeObserver: m_observer forKeyPath: @"currentItem.loadedTimeRanges"];
        // PM-2014-12-17: [[ Bug 14232 ]] If we reach here it means the filename is invalid
        m_has_invalid_filename = true;
        [t_player release];
        // PM-2014-12-17: [[ Bug 14233 ]] Setting an invalid filename should reset a previously opened movie
        [m_player replaceCurrentItemWithPlayerItem:nil];
        return;
    }

    m_has_invalid_filename = false;

    // Reset this to false when loading a new movie, so as the first frame of the new movie to be displayed
    m_loaded = false;

    CVDisplayLinkSetOutputCallback(m_display_link, MCAVFoundationPlayer::MyDisplayLinkCallback, this);
    //CVDisplayLinkStop(m_display_link);

    NSDictionary* t_settings = @{ (id)kCVPixelBufferPixelFormatTypeKey : [NSNumber numberWithInt:kCVPixelFormatType_32ARGB] };
    // AVPlayerItemVideoOutput is available in OSX version >= 10.8
    AVPlayerItemVideoOutput* t_output = [[[AVPlayerItemVideoOutput alloc] initWithPixelBufferAttributes:t_settings] autorelease];
    m_player_item_video_output = t_output;
    AVPlayerItem* t_player_item = [t_player currentItem];
    [t_player_item addOutput:m_player_item_video_output];
    [t_player replaceCurrentItemWithPlayerItem:t_player_item];

    // Release the old player (if any).
    [m_view setPlayer: nil];
    if (m_time_observer_token != nil)
    {
        [m_player removeTimeObserver:m_time_observer_token];
        m_time_observer_token = nil;
    }
    
    // MW-2014-10-22: [[ Bug 13267 ]] Remove the endtime observer.
    if (m_endtime_observer_token != nil)
    {
        [m_player removeTimeObserver:m_endtime_observer_token];
        m_endtime_observer_token = nil;
    }
    
    @try
    {
        [m_player removeObserver: m_observer forKeyPath: @"currentItem.loadedTimeRanges"];
    }
    @catch (id anException) {
        //do nothing, obviously it wasn't attached because an exception was thrown
    }
    [m_player release];
    
    // PM-2014-08-25: [[ Bug 13268 ]] Make sure we release the frame of the old movie before loading a new one
    if (m_current_frame != nil)
    {
        CFRelease(m_current_frame);
        m_current_frame = nil;
    }
    
    // PM-2014-09-02: [[ Bug 13306 ]] Make sure we reset the previous value of loadedtime when loading a new movie 
    m_buffered_time = 0;
    
    // We want this player.
    m_player = t_player;

    // Now set the player of the view.
    [m_view setPlayer: m_player];

    m_last_marker = UINT32_MAX;

    [[NSNotificationCenter defaultCenter] removeObserver: m_observer];

    [[NSNotificationCenter defaultCenter] addObserver: m_observer selector:@selector(movieFinished:) name: AVPlayerItemDidPlayToEndTimeNotification object: [m_player currentItem]];
    
    // PM-2014-08-05: [[ Bug 13105 ]] Make sure a currenttimechanged message is sent when we click step forward/backward buttons
    [[NSNotificationCenter defaultCenter] addObserver: m_observer selector:@selector(timeJumped:) name: AVPlayerItemTimeJumpedNotification object: [m_player currentItem]];
    
    m_time_scale = [m_player currentItem] . asset . duration . timescale;
}


void MCAVFoundationPlayer::Synchronize(void)
{
	if (m_window == nil)
		return;
    
	MCMacPlatformWindow *t_window;
	t_window = (MCMacPlatformWindow *)m_window;
    
	NSRect t_frame;
	t_window -> MapMCRectangleToNSRect(m_rect, t_frame);
    
    m_synchronizing = true;
    
	[m_view setFrame: t_frame];
    
	[m_view setHidden: !m_visible];
    
    m_synchronizing = false;
}

bool MCAVFoundationPlayer::IsPlaying(void)
{
	return [m_player rate] != 0;
}

void MCAVFoundationPlayer::Start(double rate)
{
    if (m_offscreen && !CVDisplayLinkIsRunning(m_display_link))
        CVDisplayLinkStart(m_display_link);

    // PM-2014-07-15 Various tweaks to handle all cases of playback
    if (!m_play_selection_only)
    {
        if (m_finished && CMTimeCompare(m_player . currentTime, m_player . currentItem . duration) >= 0)
        {
            //[m_player seekToTime:kCMTimeZero toleranceBefore:kCMTimeZero toleranceAfter:kCMTimeZero];
            SeekToTimeAndWait(0);
        }
    }
    else
    {
        if (m_selection_duration > 0 && (CMTimeCompare(m_player . currentTime, CMTimeFromLCTime(m_selection_finish)) >= 0 || CMTimeCompare(m_player . currentTime, CMTimeFromLCTime(m_selection_start)) <= 0))
        {
            //[m_player seekToTime:CMTimeFromLCTime(m_selection_start) toleranceBefore:kCMTimeZero toleranceAfter:kCMTimeZero];
            SeekToTimeAndWait(m_selection_start);
        }
        
        // PM-2014-07-15 [[ Bug 12818 ]] If the duration of the selection is 0 then the player ignores the selection
        if (m_selection_duration == 0 && CMTimeCompare(m_player . currentTime, m_player . currentItem . duration) >= 0)
        {
            //[m_player seekToTime:kCMTimeZero toleranceBefore:kCMTimeZero toleranceAfter:kCMTimeZero];
            SeekToTimeAndWait(0);
        }

    }
    
    // PM-2014-08-14: This fixes the issue of pause not being instant.
    if (m_time_observer_token != nil)
    {
        [m_player removeTimeObserver:m_time_observer_token];
        m_time_observer_token = nil;
    }
    
    // MW-2014-10-22: [[ Bug 13267 ]] Remove the endtime observer.
    if (m_endtime_observer_token != nil)
    {
        [m_player removeTimeObserver: m_endtime_observer_token];
        m_endtime_observer_token = nil;
    }
    
    
    // MW-2014-10-22: [[ Bug 13267 ]] If we are only playing a selection, then setup a boundary time observer which will
    //   pause at the endtime.
    if (m_play_selection_only)
    {
        // MW-2014-10-22: [[ Bug 13267 ]] Boundary time observers often go 'past' the requested time by a small amount
        //   so we set the boundary to be one timescale unit before the actual end time, and then adjust when we hit
        //   that time.
        CMTime t_end_time, t_original_end_time;
        if (m_selection_duration != 0)
            t_end_time = CMTimeFromLCTime(m_selection_finish);
        else
            t_end_time = [m_player currentItem] . asset . duration;
        t_original_end_time = t_end_time;
        t_end_time . value -= 1;
        m_endtime_observer_token = [m_player addBoundaryTimeObserverForTimes: [NSArray arrayWithObject: [NSValue valueWithCMTime: t_end_time]]
                                                                       queue: nil usingBlock: ^(void) {
                                                                           [m_player pause];
                                                                           [m_player seekToTime: t_original_end_time toleranceBefore:kCMTimeZero toleranceAfter:kCMTimeZero];
                                                                           // PM-2014-11-10: [[ Bug 13968 ]] Make sure we loop within start and finish time when playSelection is true 
                                                                           MovieFinished();
                                                                       }];
    }
    
    m_time_observer_token = [m_player addPeriodicTimeObserverForInterval:CMTimeMake(30, 1000) queue:nil usingBlock:^(CMTime time) {

        /*
        When alwaysBuffer is true and movie finishes, the CVDisplayLink stops. After that, in case currenttime changes
        (for example when user clicks on the controller well), we have to re-start the CVDisplayLink so as to update
        the current movie frame
        */
        
        if (m_offscreen && !CVDisplayLinkIsRunning(m_display_link))
            CVDisplayLinkStart(m_display_link);

        if (CMTimeCompare(time, m_observed_time) == 0)
            return;

        m_observed_time = time;

        // MW-2014-10-22: [[ Bug 13267 ]] If offscreen and this is called when the rate is 0,
        //   we must have just been paused by an endtime observer, so force a refresh by updating
        //   the frame.
        if (!m_offscreen)
            HandleCurrentTimeChanged();
        else if ([m_player rate] == 0.0)
            DoUpdateCurrentFrame(this);
        
    }];
    
    m_playing = true;
    m_finished = false;
    m_stepped = false;
    [m_player setRate:rate];
}

void MCAVFoundationPlayer::Stop(void)
{
    // Calling CVDisplayLinkStop here will cause problems, since Stop() is called when switching from run to edit mode and the player IsPlaying()
    
    [m_player pause];
}

void MCAVFoundationPlayer::Step(int amount)
{
    [[m_player currentItem] stepByCount:amount];
    m_stepped = true;
}

void MCAVFoundationPlayer::LockBitmap(MCImageBitmap*& r_bitmap)
{
	// First get the image from the view - this will have black where the movie
	// should be.
	MCImageBitmap *t_bitmap;
	t_bitmap = new MCImageBitmap;
	t_bitmap -> width = m_rect . width;
	t_bitmap -> height = m_rect . height;
	t_bitmap -> stride = m_rect . width * sizeof(uint32_t);
	t_bitmap -> data = (uint32_t *)malloc(t_bitmap -> stride * t_bitmap -> height);
    memset(t_bitmap -> data, 0,t_bitmap -> stride * t_bitmap -> height);
	t_bitmap -> has_alpha = t_bitmap -> has_transparency = true;
    
    // If we remove the video source from the property inspector
    if (m_player_item_video_output == nil)
    {
        r_bitmap = t_bitmap;
        return;
    }
    
	// Now if we have a current frame, then composite at the appropriate size into
	// the movie portion of the buffer.
	if (m_current_frame != nil)
	{
		extern CGBitmapInfo MCGPixelFormatToCGBitmapInfo(uint32_t p_pixel_format, bool p_alpha);
        
		CGColorSpaceRef t_colorspace;
		/* UNCHECKED */ MCMacPlatformGetImageColorSpace(t_colorspace);
        
		CGContextRef t_cg_context;
		t_cg_context = CGBitmapContextCreate(t_bitmap -> data, t_bitmap -> width, t_bitmap -> height, 8, t_bitmap -> stride, t_colorspace, MCGPixelFormatToCGBitmapInfo(kMCGPixelFormatNative, true));
        
		CIImage *t_ci_image;
		t_ci_image = [[CIImage alloc] initWithCVImageBuffer: m_current_frame];
        
        NSAutoreleasePool *t_pool;
        t_pool = [[NSAutoreleasePool alloc] init];
        
		CIContext *t_ci_context;
		t_ci_context = [CIContext contextWithCGContext: t_cg_context options: nil];
        
		[t_ci_context drawImage: t_ci_image inRect: CGRectMake(0, 0, m_rect . width, m_rect . height) fromRect: [t_ci_image extent]];
        
        [t_pool release];
        
		[t_ci_image release];
        
		CGContextRelease(t_cg_context);
		CGColorSpaceRelease(t_colorspace);
	}
    
	r_bitmap = t_bitmap;
}

void MCAVFoundationPlayer::UnlockBitmap(MCImageBitmap *bitmap)
{
    free(bitmap -> data);
	delete bitmap;
}

void MCAVFoundationPlayer::SetProperty(MCPlatformPlayerProperty p_property, MCPlatformPropertyType p_type, void *p_value)
{
    m_synchronizing = true;
    
	switch(p_property)
	{
		case kMCPlatformPlayerPropertyURL:
			Load(*(MCStringRef*)p_value, true);
			Synchronize();
			break;
		case kMCPlatformPlayerPropertyFilename:
			Load(*(MCStringRef*)p_value, false);
			Synchronize();
			break;
		case kMCPlatformPlayerPropertyOffscreen:
			Switch(*(bool *)p_value);
			break;
		case kMCPlatformPlayerPropertyRect:
			m_rect = *(MCRectangle *)p_value;
			Synchronize();
			break;
		case kMCPlatformPlayerPropertyVisible:
			m_visible = *(bool *)p_value;
			Synchronize();
			break;
		case kMCPlatformPlayerPropertyCurrentTime:
        {
            // MW-2014-07-29: [[ Bug 12989 ]] Make sure we use the duration timescale.
            // MW-2014-08-01: [[ Bug 13046 ]] Use a completion handler to wait until the currentTime is
            //   where we want it to be.
            /*__block bool t_is_finished = false;
            [[m_player currentItem] seekToTime:CMTimeFromLCTime(*(uint32_t *)p_value) toleranceBefore:kCMTimeZero toleranceAfter:kCMTimeZero completionHandler:^(BOOL finished) {
                t_is_finished = true;
                MCPlatformBreakWait();
            }];
            while(!t_is_finished)
                MCPlatformWaitForEvent(60.0, true);*/
            SeekToTimeAndWait(*(uint32_t *)p_value);
        }
        break;
		case kMCPlatformPlayerPropertyStartTime:
		{
            m_selection_start = *(uint32_t *)p_value;
            
			if (m_selection_start > m_selection_finish)
            {
                // PM-2014-07-09: [[ Bug 12761 ]] Make sure dragging the selection_start marker does not move the selection_finish marker
                m_selection_start = m_selection_finish;
            }
            // PM-2014-07-15 [[ Bug 12818 ]] If the duration of the selection is 0 then the player ignores the selection
            m_selection_duration = m_selection_finish - m_selection_start;
        }
            break;
		case kMCPlatformPlayerPropertyFinishTime:
		{
            m_selection_finish = *(uint32_t *)p_value;
            
			if (m_selection_start > m_selection_finish)
            {
                m_selection_finish = m_selection_start;
            }
            // PM-2014-07-15 [[ Bug 12818 ]] If the duration of the selection is 0 then the player ignores the selection
            m_selection_duration = m_selection_finish - m_selection_start;
        }
            break;
		case kMCPlatformPlayerPropertyPlayRate:
            if (m_player != nil)
                [m_player setRate: *(double *)p_value];
			break;
		case kMCPlatformPlayerPropertyVolume:
            if (m_player != nil)
                [m_player setVolume: *(uint16_t *)p_value / 100.0f];
			break;
        case kMCPlatformPlayerPropertyOnlyPlaySelection:
			m_play_selection_only = *(bool *)p_value;
			break;
		case kMCPlatformPlayerPropertyLoop:
			m_looping = *(bool *)p_value;
			break;
        case kMCPlatformPlayerPropertyMarkers:
        {
            array_t<uint32_t> *t_markers;
            t_markers = (array_t<uint32_t> *)p_value;
            
            m_last_marker = UINT32_MAX;
            MCMemoryDeleteArray(m_markers);
            m_markers = nil;
            
            /* UNCHECKED */ MCMemoryResizeArray(t_markers -> count, m_markers, m_marker_count);
            MCMemoryCopy(m_markers, t_markers -> ptr, m_marker_count * sizeof(uint32_t));
        }
        break;
	}
    
    m_synchronizing = false;
}

uint32_t MCAVFoundationPlayer::CMTimeToLCTime(CMTime p_cm_time)
{
    return CMTimeConvertScale(p_cm_time, m_time_scale, kCMTimeRoundingMethod_QuickTime) . value;
}

CMTime MCAVFoundationPlayer::CMTimeFromLCTime(uint32_t p_lc_time)
{
    return CMTimeMake(p_lc_time, m_time_scale);
}

static Boolean AVAssetHasType(AVAsset *p_asset, NSString *p_type)
{
    NSArray *t_tracks = [p_asset tracksWithMediaType:p_type];
    return [t_tracks count] != 0;
}

void MCAVFoundationPlayer::GetProperty(MCPlatformPlayerProperty p_property, MCPlatformPropertyType p_type, void *r_value)
{
	switch(p_property)
	{
		case kMCPlatformPlayerPropertyOffscreen:
			*(bool *)r_value = m_offscreen;
			break;
		case kMCPlatformPlayerPropertyRect:
			*(MCRectangle *)r_value = m_rect;
			break;
        case kMCPlatformPlayerPropertyMovieRect:
		{
            // TODO: the 'naturalSize' method of AVAsset is deprecated, but we use for the moment.
            CGSize t_size;
            if (m_player != nil)
                t_size = [[[m_player currentItem] asset] naturalSize];
            else
            {
                // This is in case a player is created by script, where the filename is nil (thus m_player is nil as well)
                t_size . width = 306;
                t_size . height = 244;
            }
            // PM-2014-12-17: [[ Bug 14232 ]] Check if the file is corrupted
            if (t_size . width == 0 && t_size . height == 0)
                m_has_invalid_filename = true;
			*(MCRectangle *)r_value = MCRectangleMake(0, 0, t_size . width, t_size . height);
		}
        break;
		case kMCPlatformPlayerPropertyVisible:
			*(bool *)r_value = m_visible;
			break;
        case kMCPlatformPlayerPropertyMediaTypes:
		{
			MCPlatformPlayerMediaTypes t_types;
			t_types = 0;
            AVAsset *t_asset = [[m_player currentItem] asset];
            
			if (AVAssetHasType(t_asset, AVMediaTypeVideo))
				t_types |= kMCPlatformPlayerMediaTypeVideo;
			if (AVAssetHasType(t_asset, AVMediaTypeAudio))
				t_types |= kMCPlatformPlayerMediaTypeAudio;
			if (AVAssetHasType(t_asset, AVMediaTypeText))
				t_types |= kMCPlatformPlayerMediaTypeText;
            // TODO: Add more types??
            *(MCPlatformPlayerMediaTypes *)r_value = t_types;
		}
        break;
    
            // PM-2014-08-20: [[ Bug 13121 ]] Added property for displaying download progress
        case kMCPlatformPlayerPropertyLoadedTime:
			*(uint32_t *)r_value = m_buffered_time;
			break;
		case kMCPlatformPlayerPropertyDuration:
            *(uint32_t *)r_value = CMTimeToLCTime([m_player currentItem] . asset . duration);
			break;
		case kMCPlatformPlayerPropertyTimescale:
            // MW-2014-07-29: [[ Bug 12989 ]] Use the duration timescale.
			*(uint32_t *)r_value = [m_player currentItem] . asset . duration . timescale;
			break;
		case kMCPlatformPlayerPropertyCurrentTime:
			*(uint32_t *)r_value = CMTimeToLCTime([m_player currentItem] . currentTime);
			break;
		case kMCPlatformPlayerPropertyStartTime:
			*(uint32_t *)r_value = m_selection_start;
			break;
		case kMCPlatformPlayerPropertyFinishTime:
			*(uint32_t *)r_value = m_selection_finish;
			break;
		case kMCPlatformPlayerPropertyPlayRate:
			*(double *)r_value = [m_player rate];
			break;
		case kMCPlatformPlayerPropertyVolume:
			*(uint16_t *)r_value = [m_player volume] * 100.0f;
			break;
		case kMCPlatformPlayerPropertyOnlyPlaySelection:
			*(bool *)r_value = m_play_selection_only;
			break;
            //TODO
		case kMCPlatformPlayerPropertyLoop:
			*(bool *)r_value = m_looping;
			break;
        // PM-2014-12-17: [[ Bug 14232 ]] Read-only property that indicates if a filename is invalid or if the file is corrupted
        case kMCPlatformPlayerPropertyInvalidFilename:
			*(bool *)r_value = m_has_invalid_filename;
			break;
	}
}

void MCAVFoundationPlayer::CountTracks(uindex_t& r_count)
{
    NSArray *t_tracks;
    t_tracks = [[[m_player currentItem] asset] tracks];
    r_count = [t_tracks count];
}

bool MCAVFoundationPlayer::FindTrackWithId(uint32_t p_id, uindex_t& r_index)
{
    NSArray *t_tracks;
    t_tracks = [[[m_player currentItem] asset] tracks];
    
    AVAssetTrack *t_assetTrack;
    t_assetTrack = [[[m_player currentItem] asset] trackWithTrackID: p_id];
    if (t_assetTrack == nil)
        return false;
    r_index = [t_tracks indexOfObject:t_assetTrack];
    return true;
    
}

void MCAVFoundationPlayer::SetTrackProperty(uindex_t p_index, MCPlatformPlayerTrackProperty p_property, MCPlatformPropertyType p_type, void *p_value)
{
	if (p_property != kMCPlatformPlayerTrackPropertyEnabled)
		return;
    
    NSArray *t_tracks;
    t_tracks = [[[m_player currentItem] asset] tracks];
    
    // TODO: Fix error LiveCode-Community[20563:303] -[AVAssetTrack setEnabled:]: unrecognized selector sent to instance 0xb281f50
    /*AVPlayerItemTrack *t_playerItemTrack;
    t_playerItemTrack = [t_tracks objectAtIndex:p_index];
    [t_playerItemTrack setEnabled:*(bool *)p_value];*/
}

void MCAVFoundationPlayer::GetTrackProperty(uindex_t p_index, MCPlatformPlayerTrackProperty p_property, MCPlatformPropertyType p_type, void *r_value)
{
    NSArray *t_tracks;
    t_tracks = [[[m_player currentItem] asset] tracks];
    
    // PM-2014-07-10: [[ Bug 12757 ]] Get the AVAssetTrack from t_tracks 
    AVAssetTrack *t_asset_track = (AVAssetTrack *)[t_tracks objectAtIndex:p_index];
    
	switch(p_property)
	{
		case kMCPlatformPlayerTrackPropertyId:
			*(uint32_t *)r_value = [t_asset_track trackID];
			break;
		case kMCPlatformPlayerTrackPropertyMediaTypeName:
		{
            NSString *t_mediaType;
            t_mediaType = [t_asset_track mediaType];
            MCStringCreateWithCFString((CFStringRef)t_mediaType, *(MCStringRef*)r_value);
		}
            break;
		case kMCPlatformPlayerTrackPropertyOffset:
        {
			CMTimeRange t_timeRange = [t_asset_track timeRange];
            *(uint32_t *)r_value = CMTimeToLCTime(t_timeRange . start);
        }
			break;
		case kMCPlatformPlayerTrackPropertyDuration:
        {
            CMTimeRange t_timeRange = [t_asset_track timeRange];
            *(uint32_t *)r_value = CMTimeToLCTime(t_timeRange . duration);
        }
			break;
		case kMCPlatformPlayerTrackPropertyEnabled:
			*(bool *)r_value = [t_asset_track isEnabled];
			break;
	}
}

////////////////////////////////////////////////////////

MCAVFoundationPlayer *MCAVFoundationPlayerCreate(void)
{
    return new MCAVFoundationPlayer;
}

////////////////////////////////////////////////////////
