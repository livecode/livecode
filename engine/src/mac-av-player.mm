/* Copyright (C) 2016 LiveCode Ltd.
 
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
#include <Accelerate/Accelerate.h>

#include "globdefs.h"
#include "imagebitmap.h"
#include "region.h"
#include "notify.h"

#include "platform.h"
#include "platform-internal.h"

#include "mac-internal.h"

#include "graphics_util.h"

////////////////////////////////////////////////////////////////////////////////

struct MCAVPlayerPanningFilter
{
	float left_balance;
	float right_balance;
	float left_pan;
	float right_pan;
};

bool MCAVPlayerSetupPanningFilter(AVPlayer *p_player, MCAVPlayerPanningFilter *p_panning_filter);
void MCAVPlayerRemovePanningFilter(AVPlayer *p_player);

////////////////////////////////////////////////////////////////////////////////

class MCAVFoundationPlayer;

@interface com_runrev_livecode_MCAVFoundationPlayerObserver: NSObject
{
    MCAVFoundationPlayer *m_av_player;
}

- (id)initWithPlayer: (MCAVFoundationPlayer *)player;

- (void)detachPlayer;

- (void)movieFinished: (id)object;

// PM-2014-08-05: [[ Bug 13105 ]] Make sure a currenttimechanged message is sent when we click step forward/backward buttons
- (void)timeJumped: (id)object;

- (void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary *)change context:(void *)context;

- (void)updateCurrentFrame;

@end

@interface com_runrev_livecode_MCAVFoundationPlayerView : NSView

- (void)dealloc;

- (AVPlayer*)player;
- (void)setPlayer:(AVPlayer *)player;

@end

class MCAVFoundationPlayer: public MCPlatformPlayer
{
public:
	MCAVFoundationPlayer(void);
	virtual ~MCAVFoundationPlayer(void);
    
	virtual bool GetNativeView(void *&r_view);
	virtual bool SetNativeParentView(void *p_view);
	
	virtual bool IsPlaying(void);
	virtual void Start(double rate);
	virtual void Stop(void);
	virtual void Step(int amount);
    
	virtual bool LockBitmap(const MCGIntegerSize &p_size, MCImageBitmap*& r_bitmap);
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
	void Unload();
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
    void Mirror(void);
    void Unmirror(void);
	
    void UpdatePanningFilter(double p_left_balance, double p_right_balance, double p_pan);
    
    NSLock *m_lock;
    
	AVPlayer *m_player;
    AVPlayerItemVideoOutput *m_player_item_video_output;
    CVDisplayLinkRef m_display_link;
    com_runrev_livecode_MCAVFoundationPlayerView *m_view;
    uint32_t m_selection_start, m_selection_finish;
    uint32_t m_selection_duration;
    uint32_t m_buffered_time;
	double m_scale;
    double m_rate;
	
	double m_left_balance;
	double m_right_balance;
	double m_pan;
	
	MCAVPlayerPanningFilter m_panning_filter;
	
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
    
	MCPlatformPlayerDuration *m_markers;
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
    bool m_has_invalid_filename : 1;
    bool m_mirrored : 1;
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

- (void)detachPlayer
{
    m_av_player = nullptr;
}

// PM-2014-08-05: [[ Bug 13105 ]] Make sure a currenttimechanged message is sent when we click step forward/backward buttons
- (void)timeJumped: (id)object
{
    if (m_av_player == nullptr)
    {
        return;
    }
    
    m_av_player -> TimeJumped();
}

- (void)movieFinished: (id)object
{
    if (m_av_player == nullptr)
    {
        return;
    }
    
    m_av_player -> MovieFinished();
}

- (void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary *)change context:(void *)context
{
    if (m_av_player == nullptr)
    {
        return;
    }
    
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
    if (m_av_player == nullptr)
    {
        return;
    }
    
    m_av_player -> DoUpdateCurrentFrame(m_av_player);
}

@end

/////////////////////////////////////////////////////////////////////////////////

@implementation com_runrev_livecode_MCAVFoundationPlayerView

- (void)dealloc
{
    if ([self layer] != nil)
    {
        [(AVPlayerLayer *)[self layer] setPlayer: nil];
    }
    [self setLayer: nil];
    [super dealloc];
}

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
    [self setWantsLayer: YES];
    [self setLayer: t_layer];
    self.layerContentsRedrawPolicy = NSViewLayerContentsRedrawOnSetNeedsDisplay;
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
    
	m_left_balance = 1.0;
	m_right_balance = 1.0;
	m_pan = 0.0;
	UpdatePanningFilter(m_left_balance, m_right_balance, m_pan);
	
	m_rect = MCRectangleMake(0, 0, 0, 0);
	m_visible = true;
	m_offscreen = false;
	m_pending_offscreen = false;
    
	m_switch_scheduled = false;
    
    m_playing = false;
    
    m_synchronizing = false;
    m_frame_changed_pending = false;
    m_finished = false;
    
    m_time_scale = 0;
    
    m_selection_start = 0;
    m_selection_finish = 0;
    m_buffered_time = 0;
	
	m_scale = 1.0;
    m_rate = 1.0;
	
    m_time_observer_token = nil;
    m_endtime_observer_token = nil;

    m_mirrored = false;
}

MCAVFoundationPlayer::~MCAVFoundationPlayer(void)
{
    // Detach the player from the observer
    [m_observer detachPlayer];

	if (m_current_frame != nil)
		CFRelease(m_current_frame);
    
    // First detach the observer from everything we've attached it to.
    if (m_time_observer_token != nil)
        [m_player removeTimeObserver:m_time_observer_token];
    
    // MW-2014-10-22: [[ Bug 13267 ]] Remove the end-time observer.
    if (m_endtime_observer_token != nil)
        [m_player removeTimeObserver:m_endtime_observer_token];

    // Cancel any pending 'perform' requests on the observer.
    [NSObject cancelPreviousPerformRequestsWithTarget: m_observer];
    
    [[NSNotificationCenter defaultCenter] removeObserver: m_observer];
    // Now we can release it.
    [m_observer release];
    
    CVDisplayLinkRelease(m_display_link);
    
    // Now detach the player from everything we've attached it to.
    [m_view setPlayer: nil];
    // No we can release it.
    [m_view release];
    
    // Finally we can release the player.
    Unload();
    [m_player release];
	
	// Release the video output
	[m_player_item_video_output release];
	
    MCMemoryDeleteArray(m_markers);
    
    [m_lock release];
}

bool MCAVFoundationPlayer::GetNativeView(void *& r_view)
{
	if (m_view == nil)
		return false;
	
	r_view = m_view;
	return true;
}

bool MCAVFoundationPlayer::SetNativeParentView(void *p_view)
{
	// Not used
	return true;
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
    
    if (m_offscreen)
        CVDisplayLinkStop(m_display_link);
    
    if (!m_looping)
    {
        m_playing = false;
        MCPlatformCallbackSendPlayerFinished(this);
    }
    else
    {
        // PM-2104-12-119: [[ Bug 14253 ]] For some videos, when looping and playSelection are true, the controller thumb does not update after one or two loops. To prevent this, remove and re-add the periodicTimeObserver in each loop
        if (m_time_observer_token != nil)
        {
            [m_player removeTimeObserver:m_time_observer_token];
            m_time_observer_token = nil;
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

        // PM-2014-07-15: [[ Bug 12812 ]] Make sure we loop within start and finish time when playSelection is true 
        if (m_play_selection_only && m_selection_duration > 0)
            // Note: Calling breakSeekToTimeAndWait(m_selection_start) here causes loop property to break
            [[m_player currentItem] seekToTime:CMTimeFromLCTime(m_selection_start) toleranceBefore:kCMTimeZero toleranceAfter:kCMTimeZero];
        else
            // Note: Calling SeekToTimeAndWait(0) here causes loop property to break
            [[m_player currentItem] seekToTime:kCMTimeZero toleranceBefore:kCMTimeZero toleranceAfter:kCMTimeZero];
        
        if (m_offscreen)
            CVDisplayLinkStart(m_display_link);
        // remember existing playrate when looping
        [m_player setRate:m_rate];
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
    
    // PM-2015-06-12: [[ Bug 15495 ]] If the file has no video component, then just update the currentTime (no need to updateCurrentFrame, since there are no frames)
    bool t_has_video;
    t_has_video = ( [[[[t_self -> m_player currentItem] asset] tracksWithMediaType:AVMediaTypeVideo] count] != 0);
    
    CMTime t_output_item_time = kCMTimeZero;
    
    if (t_has_video)
    {
        t_output_item_time = [t_self -> m_player_item_video_output itemTimeForCVTimeStamp:*inOutputTime];
        
        if (![t_self -> m_player_item_video_output hasNewPixelBufferForItemTime:t_output_item_time])
        {
            return kCVReturnSuccess;
        }
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
    
    bool t_has_video;
    t_has_video = ( [[[[t_player -> m_player currentItem] asset] tracksWithMediaType:AVMediaTypeVideo] count] != 0);
    
    CVImageBufferRef t_image = nullptr;
    
    [t_player -> m_lock lock];
    if (t_has_video)
    {
        t_image = [t_player -> m_player_item_video_output copyPixelBufferForItemTime:t_player -> m_next_frame_time itemTimeForDisplay:nil];
    }
    
    t_player -> m_frame_changed_pending = false;
    [t_player -> m_lock unlock];
    
    if (t_has_video)
    {
        if (t_image != nil)
        {
            if (t_player -> m_current_frame != nil)
                CFRelease(t_player -> m_current_frame);
            t_player -> m_current_frame = t_image;
        }
    }
    
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
	Synchronize();
}

void MCAVFoundationPlayer::Unrealize(void)
{
}

void MCAVFoundationPlayer::Load(MCStringRef p_filename_or_url, bool p_is_url)
{
	Unload();
	
	if (m_player == nil)
	{
		/* SETUP PLAYER */
    	m_player = [[AVPlayer alloc] init];
    	CVDisplayLinkSetOutputCallback(m_display_link, MCAVFoundationPlayer::MyDisplayLinkCallback, this);

		NSDictionary* t_settings = @{ (id)kCVPixelBufferPixelFormatTypeKey : [NSNumber numberWithInt:kCVPixelFormatType_32ARGB] };
		// AVPlayerItemVideoOutput is available in OSX version >= 10.8
		m_player_item_video_output = [[AVPlayerItemVideoOutput alloc] initWithPixelBufferAttributes:t_settings];;

		// Now set the player of the view.
		[m_view setPlayer: m_player];
	}
    // Ensure that removing the video source from the property inspector results immediately in empty player with the controller thumb in the beginning
    if (MCStringIsEmpty(p_filename_or_url))
    {
        MCPlatformPlayerDuration t_zero_time = 0;
        // PM-2014-12-17: [[ Bug 14233 ]] Setting the filename to empty should reset the currentItem
        SetProperty(kMCPlatformPlayerPropertyCurrentTime, kMCPlatformPropertyTypePlayerDuration, &t_zero_time);
        return;
    }

    id t_url;
    if (!p_is_url)
        t_url = [NSURL fileURLWithPath: MCStringConvertToAutoreleasedNSString(p_filename_or_url)];
    else
        t_url = [NSURL URLWithString: MCStringConvertToAutoreleasedNSString(p_filename_or_url)];

    [m_player replaceCurrentItemWithPlayerItem:[AVPlayerItem playerItemWithURL:t_url]];

    // PM-2014-08-19 [[ Bug 13121 ]] Added feature for displaying download progress
    if (p_is_url)
        [m_player addObserver:m_observer forKeyPath:@"currentItem.loadedTimeRanges" options:NSKeyValueObservingOptionNew context:nil];
    
    // Block-wait until the status becomes something.
    [m_player addObserver: m_observer forKeyPath: @"status" options: 0 context: nil];
    
    while([m_player status] == AVPlayerStatusUnknown)
        MCPlatformWaitForEvent(60.0, true);

    [m_player removeObserver: m_observer forKeyPath: @"status" context: nil];
    
    if(p_is_url)
        [m_player removeObserver: m_observer forKeyPath: @"currentItem.loadedTimeRanges" context: nil];
    
    // If we've failed, leave things as they are (dealloc the new player).
    if ([m_player status] == AVPlayerStatusFailed)
    {
		Unload();
        return;
    }

    /*
        PM-2014-07-07: [[Bugs 12758 and 12760]] When the filename is set to a URL or to a local file
        that is not a video, or does not exist, then currentItem is nil. Do this chack to prevent a crash
    */
    if ([m_player currentItem] == nil)
    {
        // PM-2014-12-17: [[ Bug 14232 ]] If we reach here it means the filename is invalid
        m_has_invalid_filename = true;
        Unload();
        return;
    }

	/* UNCHECKED */ MCAVPlayerSetupPanningFilter(m_player, &m_panning_filter);

    m_has_invalid_filename = false;

    [[m_player currentItem] addOutput:m_player_item_video_output];

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
    
    // PM-2014-08-25: [[ Bug 13268 ]] Make sure we release the frame of the old movie before loading a new one
    if (m_current_frame != nil)
    {
        CFRelease(m_current_frame);
        m_current_frame = nil;
    }
    
    // PM-2014-09-02: [[ Bug 13306 ]] Make sure we reset the previous value of loadedtime when loading a new movie 
    m_buffered_time = 0;
    
    Synchronize();

    m_last_marker = UINT32_MAX;

    [[NSNotificationCenter defaultCenter] removeObserver: m_observer];

    [[NSNotificationCenter defaultCenter] addObserver: m_observer selector:@selector(movieFinished:) name: AVPlayerItemDidPlayToEndTimeNotification object: [m_player currentItem]];
    
    // PM-2014-08-05: [[ Bug 13105 ]] Make sure a currenttimechanged message is sent when we click step forward/backward buttons
    [[NSNotificationCenter defaultCenter] addObserver: m_observer selector:@selector(timeJumped:) name: AVPlayerItemTimeJumpedNotification object: [m_player currentItem]];
    
    m_time_scale = [m_player currentItem] . asset . duration . timescale;
    
    // Reset to default values when loading a new video
    m_selection_duration = 0;
    m_selection_finish = 0;
    m_selection_start = 0;
}

void MCAVFoundationPlayer::Unload()
{
	if (m_player == nil)
		return;
	MCAVPlayerRemovePanningFilter(m_player);
	[m_player replaceCurrentItemWithPlayerItem:nil];
}

void MCAVFoundationPlayer::Mirror(void)
{
    CGAffineTransform t_transform1 = CGAffineTransformMakeScale(-1, 1);
    
    CGAffineTransform t_transform2 = CGAffineTransformMakeTranslation(m_view.bounds.size.width, 0);
    
    CGAffineTransform t_flip_horizontally = CGAffineTransformConcat(t_transform1, t_transform2);
    
    m_view.layer.sublayerTransform = CATransform3DMakeAffineTransform(t_flip_horizontally);
}

void MCAVFoundationPlayer::Unmirror(void)
{
    m_view.layer.sublayerTransform = CATransform3DMakeAffineTransform(CGAffineTransformMakeScale(1, 1));
}

void MCAVFoundationPlayer::UpdatePanningFilter(double p_left_balance, double p_right_balance, double p_pan)
{
	double t_left_balance, t_right_balance;
	t_left_balance = sqrt(p_left_balance);
	t_right_balance = sqrt(p_right_balance);
	
	double t_left_pan, t_right_pan;
	if (p_pan == 0.0)
	{
		t_left_pan = 0.0;
		t_right_pan = 0.0;
	}
	else if (p_pan < 0.0)
	{
		t_left_pan = 0;
		t_right_pan = sqrt(-p_pan);
	}
	else
	{
		t_left_pan = sqrt(p_pan);
		t_right_pan = 0.0;
	}
	
	m_panning_filter.left_balance = t_left_balance * (1.0 - t_left_pan);
	m_panning_filter.right_balance = t_right_balance * (1.0 - t_right_pan);
	m_panning_filter.left_pan = t_left_pan * t_left_balance;
	m_panning_filter.right_pan = t_right_pan * t_right_balance;
}

struct _audioprocessingcontext
{
	MCAVPlayerPanningFilter *panning_filter;
	void *buffer;
	uinteger_t buffer_length;
};

void _audioprocessingtap_init(MTAudioProcessingTapRef tap, void *clientInfo, void **tapStorageOut)
{
	_audioprocessingcontext *t_context = nil;
	MCMemoryNew(t_context);
	t_context->panning_filter = (MCAVPlayerPanningFilter*)clientInfo;
	t_context->buffer = nil;
	t_context->buffer_length = 0;
	*tapStorageOut = t_context;
}

void _audioprocessingtap_finalize(MTAudioProcessingTapRef tap)
{
	_audioprocessingcontext *t_context;
	t_context = (_audioprocessingcontext*) MTAudioProcessingTapGetStorage(tap);
	MCMemoryDelete(t_context);
}

void _audioprocessingtap_prepare(MTAudioProcessingTapRef tap, CMItemCount maxFrames, const AudioStreamBasicDescription *processingFormat)
{
	_audioprocessingcontext *t_context;
	t_context = (_audioprocessingcontext*) MTAudioProcessingTapGetStorage(tap);
	if (processingFormat->mChannelsPerFrame == 2)
	{
		t_context->buffer_length = maxFrames * processingFormat->mBitsPerChannel / 8;
		MCMemoryAllocate(4 * t_context->buffer_length, t_context->buffer);
	}
}

void _audioprocessingtap_unprepare(MTAudioProcessingTapRef tap)
{
	_audioprocessingcontext *t_context;
	t_context = (_audioprocessingcontext*) MTAudioProcessingTapGetStorage(tap);
	if (t_context->buffer != nil)
		MCMemoryDeallocate(t_context->buffer);
	t_context->buffer = nil;
}

void _audioprocessingtap_process(MTAudioProcessingTapRef tap, CMItemCount numberFrames,
	MTAudioProcessingTapFlags flags, AudioBufferList *bufferListInOut,
	CMItemCount *numberFramesOut, MTAudioProcessingTapFlags *flagsOut)
{
	MTAudioProcessingTapGetSourceAudio(tap, numberFrames, bufferListInOut, flagsOut, nil, numberFramesOut);
	
	if (bufferListInOut->mNumberBuffers == 2)
	{
		_audioprocessingcontext *t_context;
		t_context = (_audioprocessingcontext*)MTAudioProcessingTapGetStorage(tap);
		
		void *t_ll = t_context->buffer;
		void *t_lr = ((uint8_t*)t_context->buffer) + t_context->buffer_length;
		void *t_rl = ((uint8_t*)t_context->buffer) + t_context->buffer_length * 2;
		void *t_rr = ((uint8_t*)t_context->buffer) + t_context->buffer_length * 3;

		vDSP_vsmul((float*)bufferListInOut->mBuffers[0].mData, 1, &t_context->panning_filter->left_balance, (float*)t_ll, 1, *numberFramesOut);
		vDSP_vsmul((float*)bufferListInOut->mBuffers[1].mData, 1, &t_context->panning_filter->right_balance, (float*)t_rr, 1, *numberFramesOut);

		vDSP_vsmul((float*)bufferListInOut->mBuffers[0].mData, 1, &t_context->panning_filter->left_pan, (float*)t_rl, 1, *numberFramesOut);
		vDSP_vsmul((float*)bufferListInOut->mBuffers[1].mData, 1, &t_context->panning_filter->right_pan, (float*)t_lr, 1, *numberFramesOut);

		vDSP_vadd((float*)t_ll, 1, (float*)t_lr, 1, (float*)bufferListInOut->mBuffers[0].mData, 1, *numberFramesOut);
		vDSP_vadd((float*)t_rl, 1, (float*)t_rr, 1, (float*)bufferListInOut->mBuffers[1].mData, 1, *numberFramesOut);
	}
}

bool MCAVPlayerCreateAudioProcessingTap(MCAVPlayerPanningFilter *p_panning_filter, MTAudioProcessingTapRef &r_tap)
{
	MTAudioProcessingTapCallbacks t_callbacks;
	t_callbacks.version = kMTAudioProcessingTapCallbacksVersion_0;
	t_callbacks.clientInfo = p_panning_filter;
	t_callbacks.init = _audioprocessingtap_init;
	t_callbacks.prepare = _audioprocessingtap_prepare;
	t_callbacks.process = _audioprocessingtap_process;
	t_callbacks.unprepare = _audioprocessingtap_unprepare;
	t_callbacks.finalize = _audioprocessingtap_finalize;

	MTAudioProcessingTapRef t_tap = nil;
	OSStatus t_error;
	t_error = MTAudioProcessingTapCreate(kCFAllocatorDefault, &t_callbacks, kMTAudioProcessingTapCreationFlag_PostEffects, &t_tap);

	if (t_error || t_tap == nil)
		return false;
	
	r_tap = t_tap;
	
	return true;
}

bool MCAVPlayerSetupPanningFilter(AVPlayer *p_player, MCAVPlayerPanningFilter *p_panning_filter)
{
	AVPlayerItem *t_current_item = [p_player currentItem];
	if (t_current_item == nil)
		return false;
	
	AVAsset *t_asset = [t_current_item asset];
	if (t_asset == nil)
		return false;

	NSMutableArray *t_inputparam_array = [NSMutableArray array];
	if (t_inputparam_array == nil)
		return false;
	
	AVMutableAudioMix *t_mix = [AVMutableAudioMix audioMix];
	if (t_mix == nil)
		return false;

	bool t_success = true;
	for (AVAssetTrack *t_assettrack in [t_asset tracksWithMediaType:AVMediaTypeAudio])
	{
		AVMutableAudioMixInputParameters *t_inputparams;
		t_inputparams = [AVMutableAudioMixInputParameters audioMixInputParametersWithTrack:t_assettrack];
		t_success = t_inputparams != nil;
		if (!t_success)
			break;
		
		MTAudioProcessingTapRef t_tap = nil;
		t_success = MCAVPlayerCreateAudioProcessingTap(p_panning_filter, t_tap);
		if (!t_success)
			break;
		
		t_inputparams.audioTapProcessor = t_tap;
		CFRelease(t_tap);
		
		[t_inputparam_array addObject:t_inputparams];
	}
	
	if (!t_success)
	{
		// clear taps from array of input params
		for (AVMutableAudioMixInputParameters *t_params in t_inputparam_array)
			t_params.audioTapProcessor = nil;
		
		return false;
	}
	
	t_mix.inputParameters = t_inputparam_array;
	t_current_item.audioMix = t_mix;
	
	return true;
}

void MCAVPlayerRemovePanningFilter(AVPlayer *p_player)
{
	AVPlayerItem *t_current_item = [p_player currentItem];
	if (t_current_item == nil)
		return;
	
	AVAudioMix *t_mix = [t_current_item audioMix];
	if (t_mix == nil)
		return;
	
	for (AVAudioMixInputParameters *t_params in [t_mix inputParameters])
	{
		if (t_params.audioTapProcessor != nil)
			((AVMutableAudioMixInputParameters*)t_params).audioTapProcessor = nil;
	}
	
	t_current_item.audioMix = nil;
}

void MCAVFoundationPlayer::Synchronize(void)
{
    m_synchronizing = true;
    
    if (m_mirrored)
        Mirror();
    else
        Unmirror();
    
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
        if (m_finished && CMTimeCompare(m_player . currentTime, m_player . currentItem . asset . duration) >= 0 && rate > 0)
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
            if (rate > 0)
                SeekToTimeAndWait(m_selection_start);
            else
                SeekToTimeAndWait(m_selection_finish);
        }
        
        // PM-2014-07-15 [[ Bug 12818 ]] If the duration of the selection is 0 then the player ignores the selection
        if (m_selection_duration == 0 && CMTimeCompare(m_player . currentTime, m_player . currentItem . asset . duration) >= 0 && rate > 0)
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
                                                                           MCPlatformBreakWait();
                                                                
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
        
        // PM-2017-07-19: [[ Bug 19893 ]]
        /* The docs for "addBoundaryTimeObserverForTimes" say "Requests invocation of a block when specified times are traversed during normal playback."
         
         This probably explains why the block of "addBoundaryTimeObserverForTimes" is not invoked for *reverse* playback.
         
         So use "addPeriodicTimeObserverForInterval" to tackle this case:
         */
        
        if (m_play_selection_only && m_selection_duration != 0 && rate < 0 && CMTimeCompare(time, CMTimeFromLCTime(m_selection_start)) <= 0)
        {
            [m_player pause];
            [m_player seekToTime: CMTimeFromLCTime(m_selection_start) toleranceBefore:kCMTimeZero toleranceAfter:kCMTimeZero];
            
            MovieFinished();
            MCPlatformBreakWait();
        }
    }];
    
    m_playing = true;
    m_finished = false;
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
}

bool MCMacPlayerSnapshotCVImageBuffer(CVImageBufferRef p_imagebuffer, uint32_t p_width, uint32_t p_height, bool p_mirror, MCImageBitmap *&r_bitmap)
{
	bool t_success = true;
	
	// If we don't have an image buffer then exit
	t_success = p_imagebuffer != nil;
	
	// Create the bitmap to draw the composited frame onto.
	MCImageBitmap *t_bitmap;
	t_bitmap = nil;
	if (t_success)
		t_success = MCImageBitmapCreate(p_width, p_height, t_bitmap);
	
	if (t_success)
		MCImageBitmapClear(t_bitmap);
	
	extern CGBitmapInfo MCGPixelFormatToCGBitmapInfo(uint32_t p_pixel_format, bool p_alpha);
	
	CGColorSpaceRef t_colorspace;
	t_colorspace = nil;
	
	if (t_success)
		t_success = MCMacPlatformGetImageColorSpace(t_colorspace);
	
	CGContextRef t_cg_context;
	t_cg_context = nil;
	if (t_success)
	{
		t_cg_context = CGBitmapContextCreate(t_bitmap -> data, t_bitmap -> width, t_bitmap -> height, 8, t_bitmap -> stride, t_colorspace, MCGPixelFormatToCGBitmapInfo(kMCGPixelFormatNative, true));
		t_success = t_cg_context != nil;;
	}
	
	if (t_success && p_mirror)
	{
		// flip context transform
		CGContextTranslateCTM(t_cg_context, p_width, p_height);
		CGContextScaleCTM(t_cg_context, -1, 1);
	}
	
	CIImage *t_ci_image;
	t_ci_image = nil;
	if (t_success)
	{
		t_ci_image = [[CIImage alloc] initWithCVImageBuffer: p_imagebuffer];
		t_success = t_ci_image != nil;
	}
	
	NSAutoreleasePool *t_pool;
	t_pool = nil;
	if (t_success)
	{
		t_pool = [[NSAutoreleasePool alloc] init];
		t_success = t_pool != nil;
	}
	
	CIContext *t_ci_context;
	t_ci_context = nil;
	if (t_success)
	{
		t_ci_context = [CIContext contextWithCGContext: t_cg_context options: nil];
		t_success = t_ci_context != nil;
	}
	
	// Composite frame at the appropriate size into the buffer.
	if (t_success)
		[t_ci_context drawImage: t_ci_image inRect: CGRectMake(0, 0, p_width, p_height) fromRect: [t_ci_image extent]];
	
	if (t_pool != nil)
		[t_pool release];
	
	if (t_ci_image != nil)
		[t_ci_image release];
	
	if (t_cg_context != nil)
		CGContextRelease(t_cg_context);
	
	if (t_colorspace != nil)
		CGColorSpaceRelease(t_colorspace);
	
	if (t_success)
		r_bitmap = t_bitmap;
	else
		MCImageFreeBitmap(t_bitmap);
	
	return t_success;
}

bool MCAVFoundationPlayer::LockBitmap(const MCGIntegerSize &p_size, MCImageBitmap*& r_bitmap)
{
	// If we don't have a video source or a captured frame then exit
	if (m_player_item_video_output == nil || m_current_frame == nil)
		return false;
	
	return MCMacPlayerSnapshotCVImageBuffer(m_current_frame, p_size.width, p_size.height, m_mirrored, r_bitmap);
}

void MCAVFoundationPlayer::UnlockBitmap(MCImageBitmap *bitmap)
{
	MCImageFreeBitmap(bitmap);
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
		case kMCPlatformPlayerPropertyScalefactor:
			m_scale = *(double *)p_value;
			Synchronize();
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
			MCAssert(p_type == kMCPlatformPropertyTypePlayerDuration);
            // MW-2014-07-29: [[ Bug 12989 ]] Make sure we use the duration timescale.
            // MW-2014-08-01: [[ Bug 13046 ]] Use a completion handler to wait until the currentTime is
            //   where we want it to be.
            /*__block bool t_is_finished = false;
            [[m_player currentItem] seekToTime:CMTimeFromLCTime(*(MCPlatformPlayerDuration*)p_value) toleranceBefore:kCMTimeZero toleranceAfter:kCMTimeZero completionHandler:^(BOOL finished) {
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
			MCAssert(p_type == kMCPlatformPropertyTypePlayerDuration);
            m_selection_start = *(MCPlatformPlayerDuration*)p_value;
            
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
			MCAssert(p_type == kMCPlatformPropertyTypePlayerDuration);
            m_selection_finish = *(MCPlatformPlayerDuration*)p_value;
            
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
            {
                [m_player setRate: *(double *)p_value];
                m_rate = *(double *)p_value;
            }
			break;
		case kMCPlatformPlayerPropertyVolume:
            if (m_player != nil)
                [m_player setVolume: *(uint16_t *)p_value / 100.0f];
			break;
		case kMCPlatformPlayerPropertyLeftBalance:
			m_left_balance = *(double *)p_value / 100.0;
			UpdatePanningFilter(m_left_balance, m_right_balance, m_pan);
			break;
		case kMCPlatformPlayerPropertyRightBalance:
			m_right_balance = *(double *)p_value / 100.0;
			UpdatePanningFilter(m_left_balance, m_right_balance, m_pan);
			break;
		case kMCPlatformPlayerPropertyPan:
			m_pan = *(double *)p_value / 100.0;
			UpdatePanningFilter(m_left_balance, m_right_balance, m_pan);
			break;
        case kMCPlatformPlayerPropertyOnlyPlaySelection:
			m_play_selection_only = *(bool *)p_value;
			break;
		case kMCPlatformPlayerPropertyLoop:
			m_looping = *(bool *)p_value;
			break;
        case kMCPlatformPlayerPropertyMirrored:
            m_mirrored = *(bool *)p_value;
            if (m_mirrored)
                Mirror();
            else
                Unmirror();
			break;
        case kMCPlatformPlayerPropertyMarkers:
        {
			MCAssert(p_type == kMCPlatformPropertyTypePlayerDurationArray);
			
            MCPlatformPlayerDurationArray *t_markers;
            t_markers = (MCPlatformPlayerDurationArray*)p_value;
            
            m_last_marker = UINT32_MAX;
            MCMemoryDeleteArray(m_markers);
            m_markers = nil;
            
            /* UNCHECKED */ MCMemoryResizeArray(t_markers -> count, m_markers, m_marker_count);
            MCMemoryCopy(m_markers, t_markers -> ptr, m_marker_count * sizeof(MCPlatformPlayerDuration));
        }
        break;
		
		default:
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
            // PM-2015-01-08: [[ Bug 14345 ]] Make sure we mark a filename as invalid if it has a movie rect ze of 0 and is not audio file. Previously we checked only if the movie rect size was 0, and this caused audio files to be marked as invalid.
            bool t_has_audio;
            t_has_audio = ( [[[[m_player currentItem] asset] tracksWithMediaType:AVMediaTypeAudio] count] != 0);
            if (t_size . width == 0 && t_size . height == 0 && !t_has_audio)
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
			MCAssert(p_type == kMCPlatformPropertyTypePlayerDuration);
			*(MCPlatformPlayerDuration*)r_value = m_buffered_time;
			break;
		case kMCPlatformPlayerPropertyDuration:
			MCAssert(p_type == kMCPlatformPropertyTypePlayerDuration);
            *(MCPlatformPlayerDuration*)r_value = CMTimeToLCTime([m_player currentItem] . asset . duration);
			break;
		case kMCPlatformPlayerPropertyTimescale:
			MCAssert(p_type == kMCPlatformPropertyTypePlayerDuration);
            // MW-2014-07-29: [[ Bug 12989 ]] Use the duration timescale.
			*(MCPlatformPlayerDuration*)r_value = [m_player currentItem] . asset . duration . timescale;
			break;
		case kMCPlatformPlayerPropertyCurrentTime:
			MCAssert(p_type == kMCPlatformPropertyTypePlayerDuration);
			*(MCPlatformPlayerDuration*)r_value = CMTimeToLCTime([m_player currentItem] . currentTime);
			break;
		case kMCPlatformPlayerPropertyStartTime:
			MCAssert(p_type == kMCPlatformPropertyTypePlayerDuration);
			*(MCPlatformPlayerDuration*)r_value = m_selection_start;
			break;
		case kMCPlatformPlayerPropertyFinishTime:
			MCAssert(p_type == kMCPlatformPropertyTypePlayerDuration);
			*(MCPlatformPlayerDuration*)r_value = m_selection_finish;
			break;
		case kMCPlatformPlayerPropertyPlayRate:
            *(double *)r_value = m_rate;
			break;
		case kMCPlatformPlayerPropertyVolume:
			*(uint16_t *)r_value = [m_player volume] * 100.0f;
			break;
		case kMCPlatformPlayerPropertyLeftBalance:
			*(double *)r_value = m_left_balance * 100.0;
			break;
		case kMCPlatformPlayerPropertyRightBalance:
			*(double *)r_value = m_right_balance * 100.0;
			break;
		case kMCPlatformPlayerPropertyPan:
			*(double *)r_value = m_pan * 100.0;
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

        case kMCPlatformPlayerPropertyMirrored:
            *(bool *)r_value = m_mirrored;
			break;
			
		case kMCPlatformPlayerPropertyScalefactor:
            *(double *)r_value = m_scale;
			break;
		default:
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
    t_tracks = [[m_player currentItem] tracks];
    
    // PM-2015-03-23: [[ Bug 15052 ]] Make sure we actually set the enabledTracks
    AVPlayerItemTrack *t_playerItemTrack;
    t_playerItemTrack = (AVPlayerItemTrack *)[t_tracks objectAtIndex:p_index];
    [t_playerItemTrack setEnabled:*(bool *)p_value];
}

void MCAVFoundationPlayer::GetTrackProperty(uindex_t p_index, MCPlatformPlayerTrackProperty p_property, MCPlatformPropertyType p_type, void *r_value)
{
    // PM-2015-03-23: [[ Bug 15052 ]] Get the value of the enabledTracks property from the AVPlayerItemTrack
    NSArray *t_player_item_tracks;
    t_player_item_tracks = [[m_player currentItem] tracks];
    AVPlayerItemTrack *t_player_item_track = (AVPlayerItemTrack *)[t_player_item_tracks objectAtIndex:p_index];
    // PM-2014-07-10: [[ Bug 12757 ]] Get the AVAssetTrack from t_player_item_track
    AVAssetTrack *t_asset_track = t_player_item_track . assetTrack;
    
	switch(p_property)
	{
		case kMCPlatformPlayerTrackPropertyId:
			*(uint32_t *)r_value = [t_asset_track trackID];
			break;
		case kMCPlatformPlayerTrackPropertyMediaTypeName:
		{
            NSString *t_mediaType;
            t_mediaType = [t_asset_track mediaType];
            MCStringCreateWithCFStringRef((CFStringRef)t_mediaType, *(MCStringRef*)r_value);
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
        // PM-2015-03-23: [[ Bug 15052 ]] Get the value of the enabledTracks property from the AVPlayerItemTrack
			*(bool *)r_value = [t_player_item_track isEnabled];
			break;
	}
}

////////////////////////////////////////////////////////

MCAVFoundationPlayer *MCAVFoundationPlayerCreate(void)
{
    return new MCAVFoundationPlayer;
}

////////////////////////////////////////////////////////
