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

#include "platform.h"
#include "platform-internal.h"

#include "mac-internal.h"

#include "mac-player.h"

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

@end

class MCAVFoundationPlayer: public MCPlatformPlayer
{
public:
	MCAVFoundationPlayer(void);
	virtual ~MCAVFoundationPlayer(void);
    
	virtual bool IsPlaying(void);
	virtual void Start(void);
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
    void SelectionChanged(void);
    void CurrentTimeChanged(void);
    void RateChanged(void);
    
protected:
	virtual void Realize(void);
	virtual void Unrealize(void);
    
private:
	void Load(const char *filename, bool is_url);
	void Synchronize(void);
	void Switch(bool new_offscreen);
    
    void CacheCurrentFrame(void);
    
	static void DoSwitch(void *context);
	//static OSErr MovieDrawingComplete(Movie movie, long ref);
    //static Boolean MovieActionFilter(MovieController mc, short action, void *params, long refcon);
    
	AVPlayer *m_player;
    NSView *m_view;
    AVPlayerLayer *m_player_layer;
    CMTime m_selection_start, m_selection_finish;
    bool m_play_selection_only : 1;
    bool m_looping : 1;
    
	CVImageBufferRef m_current_frame;
    
    com_runrev_livecode_MCAVFoundationPlayerObserver *m_observer;
    
    id m_timeObserverToken;
    
    uint32_t *m_markers;
    uindex_t m_marker_count;
    uint32_t m_last_marker;
    
	MCRectangle m_rect;
	bool m_visible : 1;
	bool m_offscreen : 1;
	//bool m_show_controller : 1;
	//bool m_show_selection : 1;
	bool m_pending_offscreen : 1;
	bool m_switch_scheduled : 1;
    bool m_playing : 1;
    bool m_synchronizing : 1;
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

@end

////////////////////////////////////////////////////////

MCAVFoundationPlayer::MCAVFoundationPlayer(void)
{
	m_player = [[NSClassFromString(@"AVPlayer") alloc] init];
    m_player_layer = [AVPlayerLayer playerLayerWithPlayer : m_player];
    
	m_view = [[NSClassFromString(@"NSView") alloc] initWithFrame: NSZeroRect];
    [m_view setLayer : m_player_layer];
    
	m_observer = [[com_runrev_livecode_MCAVFoundationPlayerObserver alloc] initWithPlayer: this];
    
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
}

MCAVFoundationPlayer::~MCAVFoundationPlayer(void)
{
	if (m_current_frame != nil)
		CFRelease(m_current_frame);
    
    [[NSNotificationCenter defaultCenter] removeObserver: m_observer];
    [m_player removeTimeObserver:m_timeObserverToken];
    [m_observer release];
	[m_view release];
	[m_player release];
    
    MCMemoryDeleteArray(m_markers);
}

void MCAVFoundationPlayer::MovieFinished(void)
{
    if (!m_looping)
    {
        m_playing = false;
        MCPlatformCallbackSendPlayerStopped(this);
    }
    else
    {
        [[m_player currentItem] seekToTime:kCMTimeZero];
        [m_player play];
        m_playing = true;
        MCPlatformCallbackSendPlayerStarted(this);
    }
}

double MCAVFoundationPlayer::getDuration(void)
{
    AVPlayerItem *t_playerItem = [m_player currentItem];
    
    if ([t_playerItem status] == AVPlayerItemStatusReadyToPlay)
        return CMTimeGetSeconds([[t_playerItem asset] duration]);
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
    AVPlayerItem *t_playerItem = [m_player currentItem];
    // AVPlayerItemOutput is available in OSX 10.8 and later
    AVPlayerItemOutput *t_playerItemOutput;
    
    [t_playerItem addOutput: t_playerItemOutput];
    AVPlayerItemVideoOutput *t_playerItemVideoOutput;
    CVImageBufferRef t_image;
    t_image = [t_playerItemVideoOutput copyPixelBufferForItemTime:t_playerItem . currentTime itemTimeForDisplay:nil];
    
    if (t_image != nil)
	{
		if (m_current_frame != nil)
			CFRelease(m_current_frame);
		m_current_frame = t_image;
	}
}

//TODO
OSErr MCAVFoundationPlayer::MovieDrawingComplete(Movie p_movie, long p_ref)
{
	MCAVFoundationPlayer *t_self;
	t_self = (MCAVFoundationPlayer *)p_ref;
    
    t_self -> CacheCurrentFrame();
    
	MCPlatformCallbackSendPlayerFrameChanged(t_self);
    
	return noErr;
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

//TODO
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
        t_player -> CacheCurrentFrame();
        
		if (t_player -> m_view != nil)
			t_player -> Unrealize();
        
		SetMovieDrawingCompleteProc([t_player -> m_movie quickTimeMovie], movieDrawingCallWhenChanged, MCQTKitPlayer::MovieDrawingComplete, (long int)t_player);
        
		t_player -> m_offscreen = t_player -> m_pending_offscreen;
	}
	else
	{
		if (t_player -> m_current_frame != nil)
		{
			CFRelease(t_player -> m_current_frame);
			t_player -> m_current_frame = nil;
		}
		SetMovieDrawingCompleteProc([t_player -> m_movie quickTimeMovie], movieDrawingCallWhenChanged, nil, nil);
        
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

Boolean MCAVFoundationPlayer::MovieActionFilter(MovieController mc, short action, void *params, long refcon)
{
    switch(action)
    {
        case mcActionIdle:
        {
            MCAVFoundationPlayer *self;
            self = (MCAVFoundationPlayer *)refcon;
            
            if (self -> m_marker_count > 0)
            {
                CMTime t_current_time;
                t_current_time = [self -> m_player currentTime];
                
                // We search for the marker time immediately before the
                // current time and if last marker is not that time,
                // dispatch it.
                uindex_t t_index;
                for(t_index = 0; t_index < self -> m_marker_count; t_index++)
                    if (self -> m_markers[t_index] > t_current_time . value)
                        break;
                
                // t_index is now the first marker greater than the current time.
                if (t_index > 0)
                {
                    if (self -> m_markers[t_index - 1] != self -> m_last_marker)
                    {
                        self -> m_last_marker = self -> m_markers[t_index - 1];
                        MCPlatformCallbackSendPlayerMarkerChanged(self, self -> m_last_marker);
                    }
                }
            }
        }
            break;
            
        default:
            break;
    }
    
    return False;
}


void MCAVFoundationPlayer::Load(const char *p_filename, bool p_is_url)
{
    NSError *t_error;
	t_error = nil;
    
    id t_filename_or_url;
    
    t_filename_or_url = [NSURL URLWithString: [NSString stringWithCString: p_filename encoding: NSMacOSRomanStringEncoding]];
    
	// Create an asset with our URL, asychronously load its tracks, its duration, and whether it's playable or protected.
    // When that loading is complete, configure a player to play the asset.
    AVURLAsset *t_urlAsset = [AVAsset assetWithURL:t_filename_or_url];
    NSArray *assetKeysToLoadAndTest = [NSArray arrayWithObjects:@"playable", @"hasProtectedContent", @"tracks", @"duration", nil];
    [t_urlAsset loadValuesAsynchronouslyForKeys:assetKeysToLoadAndTest completionHandler:^(void) {
        
        // The asset invokes its completion handler on an arbitrary queue when loading is complete.
        // Because we want to access our AVPlayer in our ensuing set-up, we must dispatch our handler to the main queue.
        dispatch_async(dispatch_get_main_queue(), ^(void) {
            
            // First test whether the values of each of the keys we need have been successfully loaded.
            for (NSString *key in assetKeysToLoadAndTest)
            {
                NSError *error = nil;
                
                if ([t_urlAsset statusOfValueForKey:key error:&error] == AVKeyValueStatusFailed)
                {
                    return;
                }
            }
            
            if (![t_urlAsset isPlayable] || [t_urlAsset hasProtectedContent])
            {
                // We can't play this asset. Show the "Unplayable Asset" label.
                return;
            }
            
            // We can play this asset.
            // Set up an AVPlayerLayer according to whether the asset contains video.
            if ([[t_urlAsset tracksWithMediaType:AVMediaTypeVideo] count] != 0)
            {
                // Create an AVPlayerLayer and add it to the player view if there is video, but hide it until it's ready for display
                AVPlayerLayer *t_newPlayerLayer = [AVPlayerLayer playerLayerWithPlayer:m_player];
                [t_newPlayerLayer setFrame:[[m_view layer] bounds]];
                [t_newPlayerLayer setAutoresizingMask:kCALayerWidthSizable | kCALayerHeightSizable];
                [t_newPlayerLayer setHidden:YES];
                [[m_view layer] addSublayer:t_newPlayerLayer];
                m_player_layer = t_newPlayerLayer;
                
            }
            else
            {
                // This asset has no video tracks. Show the "No Video" label.
            }
            
            // Create a new AVPlayerItem and make it our player's current item.
            AVPlayerItem *t_playerItem = [AVPlayerItem playerItemWithAsset:t_urlAsset];
            [m_player replaceCurrentItemWithPlayerItem:t_playerItem];
            
            
            [[NSNotificationCenter defaultCenter] removeObserver: m_observer];
            
            [[NSNotificationCenter defaultCenter] addObserver: m_observer selector:@selector(movieFinished:) name: AVPlayerItemDidPlayToEndTimeNotification object: [m_player currentItem]];
            
            [[NSNotificationCenter defaultCenter] addObserver: m_observer selector:@selector(currentTimeChanged:) name: AVPlayerItemTimeJumpedNotification object: [m_player currentItem]];
            
            m_timeObserverToken = [m_player addPeriodicTimeObserverForInterval:CMTimeMake(1, 10) queue:dispatch_get_main_queue() usingBlock:^(CMTime time) {
                CurrentTimeChanged();
            }];
            
            
            extern NSString **QTMovieRateDidChangeNotification_ptr;
            [[NSNotificationCenter defaultCenter] addObserver: m_observer selector:@selector(rateChanged:) name: *QTMovieRateDidChangeNotification_ptr object: [m_player currentItem]];
            
            extern NSString **QTMovieSelectionDidChangeNotification_ptr;
            [[NSNotificationCenter defaultCenter] addObserver: m_observer selector:@selector(selectionChanged:) name: *QTMovieSelectionDidChangeNotification_ptr object: [m_player currentItem]];
            
        });
        
    }];
    
    // Set the last marker to very large so that any marker will trigger.
    m_last_marker = UINT32_MAX;
    
    MCSetActionFilterWithRefCon([m_movie quickTimeMovieController], MovieActionFilter, (long)this);
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
    
	MCMovieChanged([m_movie quickTimeMovieController], [m_movie quickTimeMovie]);
    
    m_synchronizing = false;
}

bool MCAVFoundationPlayer::IsPlaying(void)
{
	return [m_player rate] != 0;
}

void MCAVFoundationPlayer::Start(void)
{
	[m_player play];
}

void MCAVFoundationPlayer::Stop(void)
{
	[m_player pause];
}

void MCAVFoundationPlayer::Step(int amount)
{
    [[m_player currentItem] stepByCount:amount];
}

void MCAVFoundationPlayer::LockBitmap(MCImageBitmap*& r_bitmap)
{
	// First get the image from the view - this will have black where the movie
	// should be.
    
	NSRect t_rect;
	if (m_offscreen)
		t_rect = NSMakeRect(0, 0, m_rect . width, m_rect . height);
	else
		t_rect = [m_view frame];
    
	NSRect t_movie_rect;
	t_movie_rect = [m_view movieBounds];
    
	NSBitmapImageRep *t_rep;
	t_rep = [m_view bitmapImageRepForCachingDisplayInRect: t_rect];
	[m_view cacheDisplayInRect: t_rect toBitmapImageRep: t_rep];
    
	MCImageBitmap *t_bitmap;
	t_bitmap = new MCImageBitmap;
	t_bitmap -> width = [t_rep pixelsWide];
	t_bitmap -> height = [t_rep pixelsHigh];
	t_bitmap -> stride = [t_rep bytesPerRow];
	t_bitmap -> data = (uint32_t *)[t_rep bitmapData];
	t_bitmap -> has_alpha = t_bitmap -> has_transparency = true;
    
	// Now if we have a current frame, then composite at the appropriate size into
	// the movie portion of the buffer.
	if (m_current_frame != nil)
	{
		extern CGBitmapInfo MCGPixelFormatToCGBitmapInfo(uint32_t p_pixel_format, bool p_alpha);
        
		CGColorSpaceRef t_colorspace;
		t_colorspace = CGColorSpaceCreateDeviceRGB();
        
		CGContextRef t_cg_context;
		t_cg_context = CGBitmapContextCreate(t_bitmap -> data, t_bitmap -> width, t_bitmap -> height, 8, t_bitmap -> stride, t_colorspace, MCGPixelFormatToCGBitmapInfo(kMCGPixelFormatNative, true));
        
		CIContext *t_ci_context;
		t_ci_context = [CIContext contextWithCGContext: t_cg_context options: nil];
        
		CIImage *t_ci_image;
		t_ci_image = [[CIImage alloc] initWithCVImageBuffer: m_current_frame];
        
		[t_ci_context drawImage: t_ci_image inRect: CGRectMake(0, t_rect . size . height - t_movie_rect . size . height, t_movie_rect . size . width, t_movie_rect . size . height) fromRect: [t_ci_image extent]];
        
		[t_ci_image release];
        
		CGContextRelease(t_cg_context);
		CGColorSpaceRelease(t_colorspace);
	}
    
	r_bitmap = t_bitmap;
}

void MCAVFoundationPlayer::UnlockBitmap(MCImageBitmap *bitmap)
{
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
            [m_player seekToTime:CMTimeMake(*(uint32_t *)p_value, 1)];
			break;
		case kMCPlatformPlayerPropertyStartTime:
		{
            CMTime t_selection_finish;
            
            t_selection_finish = m_selection_finish;
            
			uint32_t t_start_time, t_end_time;
			t_start_time = *(uint32_t *)p_value;
			t_end_time = t_selection_finish . value;
            
            m_selection_start = CMTimeMake(t_start_time, 1);
            
			if (t_start_time > t_end_time)
            {
                m_selection_finish = m_selection_start;
				t_end_time = t_start_time;
            }
            
            SelectionChanged();
            if (m_play_selection_only)
                [m_player seekToTime:CMTimeMake(t_start_time, 1) toleranceBefore:kCMTimeZero toleranceAfter: CMTimeMake(t_end_time - t_start_time,1)];
            
		}
            break;
		case kMCPlatformPlayerPropertyFinishTime:
		{
            CMTime t_selection_start;
            
            t_selection_start = m_selection_start;
            
			uint32_t t_start_time, t_end_time;
			t_start_time = t_selection_start . value;
			t_end_time = *(uint32_t *)p_value;
            
            m_selection_finish = CMTimeMake(t_end_time, 1);
            
			if (t_start_time > t_end_time)
            {
                m_selection_finish = m_selection_start;
				t_end_time = t_start_time;
            }
            
            SelectionChanged();
            if (m_play_selection_only)
                [m_player seekToTime:CMTimeMake(t_start_time, 1) toleranceBefore:kCMTimeZero toleranceAfter: CMTimeMake(t_end_time - t_start_time,1)];
        }
            break;
		case kMCPlatformPlayerPropertyPlayRate:
            [m_player setRate: *(double *)p_value];
            RateChanged();
			break;
		case kMCPlatformPlayerPropertyVolume:
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

/*
 static Boolean IsQTVRMovie(Movie theMovie)
 {
 Boolean IsQTVR = False;
 OSType evaltype,targettype =  kQTVRUnknownType;
 UserData myUserData;
 if (theMovie == NULL)
 return False;
 myUserData = GetMovieUserData(theMovie);
 if (myUserData != NULL)
 {
 GetUserDataItem(myUserData, &targettype, sizeof(targettype),
 kUserDataMovieControllerType, 0);
 evaltype = EndianU32_BtoN(targettype);
 if (evaltype == kQTVRQTVRType || evaltype == kQTVROldPanoType
 || evaltype == kQTVROldObjectType)
 IsQTVR = true;
 }
 return(IsQTVR);
 }
 
 static Boolean QTMovieHasType(Movie tmovie, OSType movtype)
 {
 switch (movtype)
 {
 case VisualMediaCharacteristic:
 case AudioMediaCharacteristic:
 return (GetMovieIndTrackType(tmovie, 1, movtype,
 movieTrackCharacteristic) != NULL);
 case kQTVRQTVRType:
 return IsQTVRMovie(tmovie);
 default:
 return (GetMovieIndTrackType(tmovie, 1, movtype,
 movieTrackMediaType) != NULL);
 }
 }
 */

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
			AVAssetTrack *t_assetTrack;
            NSArray *t_tracks = [[[m_player currentItem] asset] tracks];
            t_assetTrack = [t_tracks objectAtIndex:0];
            CGSize t_size = [t_assetTrack naturalSize];
            
			*(MCRectangle *)r_value = MCRectangleMake(0, 0, t_size . width, t_size . height);
		}
            break;
            /*
             case kMCPlatformPlayerPropertyMovieRect:
             {
             NSValue *t_value;
             extern NSString **QTMovieNaturalSizeAttribute_ptr;
             //TODO
             t_value = [m_movie attributeForKey: *QTMovieNaturalSizeAttribute_ptr];
             *(MCRectangle *)r_value = MCRectangleMake(0, 0, [t_value sizeValue] . width, [t_value sizeValue] . height);
             }
             break;
             */
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
            /*
             case kMCPlatformPlayerPropertyMediaTypes:
             {
             MCPlatformPlayerMediaTypes t_types;
             t_types = 0;
             if (QTMovieHasType([m_movie quickTimeMovie], VisualMediaCharacteristic))
             t_types |= kMCPlatformPlayerMediaTypeVideo;
             if (QTMovieHasType([m_movie quickTimeMovie], AudioMediaCharacteristic))
             t_types |= kMCPlatformPlayerMediaTypeAudio;
             if (QTMovieHasType([m_movie quickTimeMovie], TextMediaType))
             t_types |= kMCPlatformPlayerMediaTypeText;
             if (QTMovieHasType([m_movie quickTimeMovie], kQTVRQTVRType))
             t_types |= kMCPlatformPlayerMediaTypeQTVR;
             if (QTMovieHasType([m_movie quickTimeMovie], SpriteMediaType))
             t_types |= kMCPlatformPlayerMediaTypeSprite;
             if (QTMovieHasType([m_movie quickTimeMovie], FlashMediaType))
             t_types |= kMCPlatformPlayerMediaTypeFlash;
             *(MCPlatformPlayerMediaTypes *)r_value = t_types;
             }
             break;
             */
		case kMCPlatformPlayerPropertyDuration:
            *(uint32_t *)r_value = [m_player currentItem] . duration . value;
			break;
		case kMCPlatformPlayerPropertyTimescale:
			*(uint32_t *)r_value = [m_player currentTime] . timescale;
			break;
		case kMCPlatformPlayerPropertyCurrentTime:
			*(uint32_t *)r_value = [m_player currentTime] . value;
			break;
		case kMCPlatformPlayerPropertyStartTime:
			*(uint32_t *)r_value = m_selection_start . value;
			break;
		case kMCPlatformPlayerPropertyFinishTime:
			*(uint32_t *)r_value = m_selection_finish . value;
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
    
    AVPlayerItemTrack *t_playerItemTrack;
    t_playerItemTrack = [t_tracks objectAtIndex:p_index];
    [t_playerItemTrack setEnabled:*(bool *)p_value];
    
}

void MCAVFoundationPlayer::GetTrackProperty(uindex_t p_index, MCPlatformPlayerTrackProperty p_property, MCPlatformPropertyType p_type, void *r_value)
{
    NSArray *t_tracks;
    t_tracks = [[[m_player currentItem] asset] tracks];
    
    AVPlayerItemTrack *t_playerItemTrack;
    t_playerItemTrack = [t_tracks objectAtIndex:p_index];
    
    AVAssetTrack *t_assetTrack = [t_playerItemTrack assetTrack];
    
	switch(p_property)
	{
		case kMCPlatformPlayerTrackPropertyId:
			*(uint32_t *)r_value = [t_assetTrack trackID];
			break;
		case kMCPlatformPlayerTrackPropertyMediaTypeName:
		{
            NSString *t_mediaType;
            t_mediaType = [t_assetTrack mediaType];
            *(char **)r_value = (char *)[t_mediaType cStringUsingEncoding: NSMacOSRomanStringEncoding];
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
			*(bool *)r_value = [t_playerItemTrack isEnabled];
			break;
	}
}

////////////////////////////////////////////////////////

MCAVFoundationPlayer *MCAVFoundationPlayerCreate(void)
{
    return new MCAVFoundationPlayer;
}

////////////////////////////////////////////////////////
