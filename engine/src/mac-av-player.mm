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

#include "core.h"
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
- (void)currentTimeChanged: (id)object;
- (void)rateChanged: (id)object;
- (void)selectionChanged: (id)object;

- (void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary *)change context:(void *)context;

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
    virtual double getDuration(void);
    virtual double getCurrentTime(void);
    
    void MovieFinished(void);
    void SelectionFinished(void);
    void SelectionChanged(void);
    void CurrentTimeChanged(void);
    void RateChanged(void);
    AVPlayer *getPlayer(void);
    
protected:
	virtual void Realize(void);
	virtual void Unrealize(void);
    
private:
	void Load(const char *filename, bool is_url);
	void Synchronize(void);
	void Switch(bool new_offscreen);
    
    void CacheCurrentFrame(void);
    static CVReturn MyDisplayLinkCallback (CVDisplayLinkRef displayLink,
                                                          const CVTimeStamp *inNow,
                                                          const CVTimeStamp *inOutputTime,
                                                          CVOptionFlags flagsIn,
                                                          CVOptionFlags *flagsOut,
                                    void *displayLinkContext);
    
	static void DoSwitch(void *context);
    static void DoUpdateCurrentFrame(void *ctxt);
    static void DoUpdateCurrentTime(void *ctxt);
    
    NSLock *m_lock;
    
	AVPlayer *m_player;
    AVPlayerItemVideoOutput *m_player_item_video_output;
    CVDisplayLinkRef m_display_link;
    com_runrev_livecode_MCAVFoundationPlayerView *m_view;
    uint32_t m_selection_start, m_selection_finish;
    bool m_play_selection_only : 1;
    bool m_looping : 1;
    
	CVImageBufferRef m_current_frame;
    //CGImageRef m_current_frame;
    CMTime m_next_frame_time;
    CMTime m_observed_time;
    
    com_runrev_livecode_MCAVFoundationPlayerObserver *m_observer;
    
    id m_time_observer_token;
    
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

- (void)movieFinished: (id)object
{
    m_av_player -> MovieFinished();
}

- (void)currentTimeChanged: (id)object
{
    m_av_player -> CurrentTimeChanged();
}

- (void)rateChanged: (id)object
{
    m_av_player -> RateChanged();
}

- (void)selectionChanged: (id)object
{
    m_av_player -> SelectionChanged();
}

- (void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary *)change context:(void *)context
{
    if ([keyPath isEqualToString: @"status"])
        MCPlatformBreakWait();
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
}

MCAVFoundationPlayer::~MCAVFoundationPlayer(void)
{
	if (m_current_frame != nil)
		CFRelease(m_current_frame);
    
    // First detach the observer from everything we've attached it to.
    [m_player removeObserver: m_observer forKeyPath: @"status"];
    [m_player removeTimeObserver:m_time_observer_token];
    
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

void MCAVFoundationPlayer::SelectionFinished(void)
{
/*
    if (m_play_selection_only && CMTimeCompare(m_observed_time, CMTimeMake(m_selection_finish, 1000)) >= 0)
    {
        if (m_player.rate != 0.0)
        {
            [m_player setRate:0.0];
            [m_player seekToTime:CMTimeMake(m_selection_finish, 1000) toleranceBefore:kCMTimeZero toleranceAfter:kCMTimeZero];
        }
    }
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
        MCPlatformCallbackSendPlayerStopped(this);
    }
    else
    {
        [[m_player currentItem] seekToTime:kCMTimeZero toleranceBefore:kCMTimeZero toleranceAfter:kCMTimeZero];
        
        if (m_offscreen)
            CVDisplayLinkStart(m_display_link);
        
        [m_player play];
        m_playing = true;
        m_finished = false;
        MCPlatformCallbackSendPlayerStarted(this);
    }
}

AVPlayer* MCAVFoundationPlayer::getPlayer(void)
{
    return m_player;
}

double MCAVFoundationPlayer::getDuration(void)
{
    AVPlayerItem *t_player_item = [m_player currentItem];
    
    if ([t_player_item status] == AVPlayerItemStatusReadyToPlay)
        return CMTimeGetSeconds([[t_player_item asset] duration]);
    else
        return 0.f;
}

double MCAVFoundationPlayer::getCurrentTime(void)
{
    return CMTimeGetSeconds([m_player currentTime]);
}

void MCAVFoundationPlayer::RateChanged(void)
{
    if (m_playing && [m_player rate] == 0.0 && getCurrentTime() != getDuration())
    {
        m_playing = false;
        MCPlatformCallbackSendPlayerPaused(this);
    }
    else if (!m_playing && [m_player rate] != 0.0)
    {
        m_playing = true;
        MCPlatformCallbackSendPlayerStarted(this);
    }
}

void MCAVFoundationPlayer::SelectionChanged(void)
{
    if (m_play_selection_only)
        [[m_player currentItem] setForwardPlaybackEndTime:CMTimeMake(m_selection_finish, 1000)];
    else
        [[m_player currentItem] setForwardPlaybackEndTime:kCMTimeInvalid];
    
    NSLog(@"Selection changed");
    if (!m_synchronizing)
        MCPlatformCallbackSendPlayerSelectionChanged(this);
}

void MCAVFoundationPlayer::CurrentTimeChanged(void)
{
    if (!m_synchronizing)
        MCPlatformCallbackSendPlayerCurrentTimeChanged(this);
}


void MCAVFoundationPlayer::CacheCurrentFrame(void)
{
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
        //NSLog(@"We do NOT have a new frame!");
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
        MCNotifyPush(DoUpdateCurrentFrame, t_self, false, false);
    
    if (t_self -> IsPlaying())
        MCNotifyPush(DoUpdateCurrentTime, t_self, true, false);
    
    return kCVReturnSuccess;
    
}

void MCAVFoundationPlayer::DoUpdateCurrentTime(void *ctxt)
{
    MCAVFoundationPlayer *t_player;
	t_player = (MCAVFoundationPlayer *)ctxt;
    
    t_player -> CurrentTimeChanged();
}

void MCAVFoundationPlayer::DoUpdateCurrentFrame(void *ctxt)
{
    MCAVFoundationPlayer *t_player;
	t_player = (MCAVFoundationPlayer *)ctxt;
    
    // PM-2014-07-07: [[Bug 12746]] Removed code to make player display the first frame when a new movie is loaded
    //if (t_player -> m_loaded && !t_player -> IsPlaying())
        //return;
    
    NSLog(@"We have a new frame!");
    
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
        
        if (!CVDisplayLinkIsRunning(t_player -> m_display_link))
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

void MCAVFoundationPlayer::Load(const char *p_filename_or_url, bool p_is_url)
{
    // Ensure that removing the video source from the property inspector results immediately in empty player with the controller thumb in the beginning  
    if (p_filename_or_url == nil)
    {
        m_player_item_video_output = nil;
        uint4 t_zero_time = 0;
        SetProperty(kMCPlatformPlayerPropertyCurrentTime, kMCPlatformPropertyTypeUInt32, &t_zero_time);
        return;
    }
    
    id t_url;
    if (!p_is_url)
        t_url = [NSURL fileURLWithPath: [NSString stringWithCString: p_filename_or_url encoding: NSMacOSRomanStringEncoding]];
    else
        t_url = [NSURL URLWithString: [NSString stringWithCString: p_filename_or_url encoding: NSMacOSRomanStringEncoding]];
    
    AVPlayer *t_player;
    t_player = [[AVPlayer alloc] initWithURL: t_url];
    
    // Observe the status property.
    [t_player addObserver: m_observer forKeyPath: @"status" options: 0 context: nil];
    
    // Block-wait until the status becomes something.
    while([t_player status] == AVPlayerStatusUnknown)
        MCPlatformWaitForEvent(60.0, true);
    
    // If we've failed, leave things as they are (dealloc the new player).
    if ([t_player status] == AVPlayerStatusFailed)
    {
        // error obtainable via [t_player error]
        [t_player removeObserver: m_observer forKeyPath: @"status"];
        [t_player release];
        return;
    }
    
    /*
        PM-2014-07-07: [[Bugs 12758 and 12760]] When the filename is set to a URL or to a local file 
        that is not a video, or does not exist, then currentItem is nil. Do this chack to prevent a crash
    */
    if ([t_player currentItem] == nil)
    {
        NSLog(@"Invalid filename or URL!");
        [t_player removeObserver: m_observer forKeyPath: @"status"];
        [t_player release];
        return;
    }
    
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
    [m_player removeObserver: m_observer forKeyPath: @"status"];
    [m_player removeTimeObserver:m_time_observer_token];

    [m_player release];
    
    // We want this player.
    m_player = t_player;
    
    // Now set the player of the view.
    [m_view setPlayer: m_player];
    
    [[NSNotificationCenter defaultCenter] removeObserver: m_observer];
    
    [[NSNotificationCenter defaultCenter] addObserver: m_observer selector:@selector(movieFinished:) name: AVPlayerItemDidPlayToEndTimeNotification object: [m_player currentItem]];
    
    //[[NSNotificationCenter defaultCenter] addObserver: m_observer selector:@selector(currentTimeChanged:) name: AVPlayerItemTimeJumpedNotification object: [m_player currentItem]];
    
    m_time_observer_token = [m_player addPeriodicTimeObserverForInterval:CMTimeMake(1, 1000) queue:nil usingBlock:^(CMTime time) {
    
        m_observed_time = time;

        // This fixes the issue of pause not being instant when alwaysBuffer = false
        if (IsPlaying() && !m_offscreen)
        {
            //SelectionFinished();
            CurrentTimeChanged();
        }
        
        }];
    
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
    
    if (m_finished && !m_play_selection_only)
    {
        [m_player seekToTime:kCMTimeZero toleranceBefore:kCMTimeZero toleranceAfter:kCMTimeZero];
        m_playing = true;
        m_finished = false;
    }

    // put the thumb in the beginning of the selected area, if it is outside
    if (m_play_selection_only && (CMTimeCompare(m_player . currentTime, CMTimeMake(m_selection_finish, 1000)) >= 0 || CMTimeCompare(m_player . currentTime, CMTimeMake(m_selection_start, 1000)) <= 0))
    {
        [m_player seekToTime:CMTimeMake(m_selection_start, 1000) toleranceBefore:kCMTimeZero toleranceAfter:kCMTimeZero];
        
    }
    
    [m_player setRate:rate];
}

void MCAVFoundationPlayer::Stop(void)
{
    // Calling CVDisplayLinkStop here will cause problems, since Stop() is called when switching from run to edit mode and the player IsPlaying()
    
    [m_player pause];
    MCPlatformCallbackSendPlayerPaused(this);    
}

void MCAVFoundationPlayer::Step(int amount)
{
    [[m_player currentItem] stepByCount:amount];
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
		t_colorspace = CGColorSpaceCreateDeviceRGB();
        
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
			Load(*(const char **)p_value, true);
			Synchronize();
			break;
		case kMCPlatformPlayerPropertyFilename:
			Load(*(const char **)p_value, false);
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
            // Add toleranceBefore/toleranceAfter for accurate scrubbing
            [[m_player currentItem] seekToTime:CMTimeMake(*(uint32_t *)p_value, 1000) toleranceBefore:kCMTimeZero toleranceAfter:kCMTimeZero];
			break;
		case kMCPlatformPlayerPropertyStartTime:
		{
            m_selection_start = *(uint32_t *)p_value;
            
			if (m_selection_start > m_selection_finish)
            {
                // PM-2014-09-07: [[ Bug 12761 ]] Make sure dragging the selection_start marker does not move the selection_finish marker
                m_selection_start = m_selection_finish;
            }
            
        }
            break;
		case kMCPlatformPlayerPropertyFinishTime:
		{
            m_selection_finish = *(uint32_t *)p_value;
            
			if (m_selection_start > m_selection_finish)
            {
                m_selection_finish = m_selection_start;
            }
            
        }
            break;
		case kMCPlatformPlayerPropertyPlayRate:
            [m_player setRate: *(double *)p_value];
            //RateChanged();
			break;
		case kMCPlatformPlayerPropertyVolume:
            [m_player setVolume: *(uint16_t *)p_value / 100.0f];
			break;
        case kMCPlatformPlayerPropertyOnlyPlaySelection:
			m_play_selection_only = *(bool *)p_value;
            SelectionChanged();
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
            t_size = [[[m_player currentItem] asset] naturalSize];
            
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
		case kMCPlatformPlayerPropertyDuration:
            *(uint32_t *)r_value = 1000 * CMTimeGetSeconds([m_player currentItem] . asset . duration);
			break;
		case kMCPlatformPlayerPropertyTimescale:
			*(uint32_t *)r_value = [m_player currentItem] . currentTime . timescale;
			break;
		case kMCPlatformPlayerPropertyCurrentTime:
			*(uint32_t *)r_value = 1000 * CMTimeGetSeconds([m_player currentTime]);
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
    AVPlayerItemTrack *t_playerItemTrack;
    t_playerItemTrack = [t_tracks objectAtIndex:p_index];
    [t_playerItemTrack setEnabled:*(bool *)p_value];
   
    
}

void MCAVFoundationPlayer::GetTrackProperty(uindex_t p_index, MCPlatformPlayerTrackProperty p_property, MCPlatformPropertyType p_type, void *r_value)
{
    NSArray *t_tracks;
    t_tracks = [[[m_player currentItem] asset] tracks];
    
    /*
    AVPlayerItemTrack *t_playerItemTrack;
    t_playerItemTrack = [t_tracks objectAtIndex:p_index];
    
    // TODO: Fix error "LiveCode-Community[18526:303] -[AVAssetTrack assetTrack]: unrecognized selector sent to instance 0xa9d5aa0"
    AVAssetTrack *t_assetTrack = t_playerItemTrack.assetTrack;
    */
    AVAssetTrack *t_assetTrack = [t_tracks objectAtIndex:p_index];
    
	switch(p_property)
	{
		case kMCPlatformPlayerTrackPropertyId:
			*(uint32_t *)r_value = [t_assetTrack trackID];
			break;
		case kMCPlatformPlayerTrackPropertyMediaTypeName:
		{
            NSString *t_mediaType;
            t_mediaType = [t_assetTrack mediaType];
            *(char **)r_value = strdup([t_mediaType cStringUsingEncoding: NSMacOSRomanStringEncoding]);
		}
            break;
		case kMCPlatformPlayerTrackPropertyOffset:
        {
			CMTimeRange t_timeRange = [t_assetTrack timeRange];
            *(uint32_t *)r_value = t_timeRange . start . value;
        }
			break;
		case kMCPlatformPlayerTrackPropertyDuration:
        {
            CMTimeRange t_timeRange = [t_assetTrack timeRange];
            *(uint32_t *)r_value = t_timeRange . duration . value;
        }
			break;
		case kMCPlatformPlayerTrackPropertyEnabled:
			*(bool *)r_value = [t_assetTrack isEnabled];
			break;
	}
}

////////////////////////////////////////////////////////

MCAVFoundationPlayer *MCAVFoundationPlayerCreate(void)
{
    return new MCAVFoundationPlayer;
}

////////////////////////////////////////////////////////
