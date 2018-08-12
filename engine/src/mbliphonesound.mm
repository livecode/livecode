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
#include "card.h"
#include "image.h"
#include "player.h"
#include "param.h"
#include "eventqueue.h"
#include "osspec.h"

#include "mblsyntax.h"

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>
#import <AVFoundation/AVAudioPlayer.h>
// HC-2011-10-20 [[ Media Picker ]] Import the AVPlayer libraries in order to play iTunes media files.
#import <CoreMedia/CMTime.h>
#import <AVFoundation/AVAsset.h>
#import <AVFoundation/AVAudioMix.h>
#import <AVFoundation/AVFoundation.h>
#import <AVFoundation/AVPlayer.h>
#import <AVFoundation/AVPlayerItem.h>

#include "mbliphone.h"
#include "mbliphoneapp.h"

// HC-2012-02-01: [[ Bug 9983 ]] - Added "playStopped" message to "play" so users can track when their tracks have finished playing
class MCFinishedPlayingSound: public MCCustomEvent
{
private:
	MCStringRef m_media;
    
public:
	MCFinishedPlayingSound (MCStringRef p_media)
	{
        m_media = MCValueRetain(p_media);
	}
    
    // PM-2015-01-29 [[ Bug 14463 ]] Removed ~MCFinishedPlayingSound() code, since it caused over-releasing of m_media and crash
    
	void Destroy(void)
	{
        MCValueRelease(m_media);
		delete this;
	}
	
	void Dispatch(void)
	{
        MCdefaultstackptr -> getcurcard() -> message_with_valueref_args (MCM_play_stopped, m_media);
	}
};

void MCIHandleFinishedPlayingSound(MCStringRef p_payload)
{
	MCEventQueuePostCustom(new MCFinishedPlayingSound (p_payload));
}

// HC-2011-10-20 [[ Media Picker ]] Added functionality to access iPod by accessing the AVFoundation.
@interface com_runrev_livecode_MCSoundPlayerDelegate: NSObject
{
    AVAsset *m_asset;
	AVPlayer *m_player;
	AVPlayerItem *m_player_item;
    bool m_looping;
}

- (void)audioPlayerDidReachEnd:(NSNotification *)notification;
- (void)audioPlayerFailedToReachEnd:(NSNotification *)notification;
- (void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary *)change context:(void *)context;
- (bool)playSound:(NSURL *)media_item looping:(bool)looping;
- (void)setPlayerVolume:(float)volume;
- (id)init;
- (void)dealloc;
@end

static com_runrev_livecode_MCSoundPlayerDelegate *s_sound_player_delegate = nil;
static float s_sound_loudness = 1.0;
static MCStringRef s_sound_file = nil;

bool MCSystemSoundInitialize()
{
	s_sound_file = MCValueRetain(kMCEmptyString);
	return true;
}

bool MCSystemSoundFinalize()
{
	MCValueRelease(s_sound_file);
	return true;
}

@implementation com_runrev_livecode_MCSoundPlayerDelegate

-(id)init
{
    if (self = [super init])
    {
        m_asset = nil;
        m_player = nil;
        m_player_item = nil;
        m_looping = false;
    }
    
    return self;
}

-(void)cleanUp
{
    // MM-2012-03-19: [[ Bug 10100 ]] Remove notification listeners before freeing.
    [[NSNotificationCenter defaultCenter] removeObserver: self];
    if (m_asset != nil)
        [m_asset release];
    if (m_player_item != nil)
    {
        [m_player_item removeObserver: self forKeyPath: @"status"];
        [m_player_item release];
    }
 	if (m_player != nil)
    {
        [m_player removeObserver: self forKeyPath: @"status"];
		[m_player release];
    }
	m_player = nil;
	m_player_item = nil;
    m_asset = nil;
    m_looping = false;
}

-(void)dealloc
{
	if (m_player != nil)
		[m_player pause];
    [self cleanUp];
    [super dealloc];
}

// MM-2012-02-29: [[ BUG 10021 ]] Make sure we clear out the sound file if playback stops for any reason
- (void)audioPlayerFailedToReachEnd:(NSNotification *)notification
{
	MCValueAssign(s_sound_file, kMCEmptyString);
}

- (void)audioPlayerDidReachEnd:(NSNotification *)notification
{
	if (!MCStringIsEmpty(s_sound_file))
    {
        // MM-2012-02-29: [[ BUG 10039 ]] Audio looping does not work on iOS
        if (m_looping)
            [m_player_item seekToTime:kCMTimeZero];
        else
        {
            // HC-2012-02-01: [[ Bug 9983 ]] - Added "playStopped" message to "play" so users can track when their tracks have finished playing
            // Send a message to indicate that we have finished playing a track.
            MCIHandleFinishedPlayingSound (s_sound_file);
            MCValueAssign(s_sound_file, kMCEmptyString);
        }
    }
}

// MM-2012-02-29: [[ BUG 10021 ]] Make sure we clear out the sound file if playback stops for any reason
- (void)observeValueForKeyPath:(NSString *)keyPath ofObject:(id)object change:(NSDictionary *)change context:(void *)context
{
    if (m_player != nil)
    {
        if ([m_player status] == AVPlayerStatusFailed || [m_player_item status] == AVPlayerItemStatusFailed)
        {
            MCValueAssign(s_sound_file, kMCEmptyString);
        }
    }
}

-(bool)playSound:(NSURL *)media_item looping:(bool)looping
{
    if (media_item == nil)
        return false;
    
    bool t_success;
    t_success = true;

    if (t_success)
        [self cleanUp];
    
    if (t_success)
    {
        m_asset = [[AVURLAsset alloc] initWithURL: media_item options:[NSDictionary dictionaryWithObject:[NSNumber numberWithBool:NO] forKey:AVURLAssetPreferPreciseDurationAndTimingKey]];
        t_success = m_asset != nil;
    }
    
    if (t_success)
    {
        m_player_item = [[AVPlayerItem alloc] initWithAsset:m_asset];
        t_success = m_player_item != nil;
    }
    
    if (t_success)
    {
        m_player = [[AVPlayer alloc] initWithPlayerItem:m_player_item];
        t_success = m_player != nil; 
    }
    
    if (t_success)
    {
        // HC-2011-10-28 [[ BUG 9835 ]] Reinstated notification and added ":" 
        [m_player setActionAtItemEnd: AVPlayerActionAtItemEndNone];
        m_looping = looping;
        [[NSNotificationCenter defaultCenter] addObserver:self
                                                 selector:@selector(audioPlayerDidReachEnd:)
                                                     name:AVPlayerItemDidPlayToEndTimeNotification
                                                   object:[m_player currentItem]];
        if (MCmajorosversion >= 500)
        {
#ifdef __IPHONE_5_0
            // MM-2012-03-23: [[ Bug ]] AVPlayerItemFailedToPlayToEndTimeNotification only added in 4.3 - use string represnetation instead
            [[NSNotificationCenter defaultCenter] addObserver:self
                                                     selector:@selector(audioPlayerFaliedToReachEnd:)
                                                         name:@"AVPlayerItemFailedToPlayToEndTimeNotification"
                                                       object:[m_player currentItem]];
#endif
        }
        [m_player_item addObserver:self forKeyPath:@"status" options:0 context:nil];
        [m_player addObserver:self forKeyPath:@"status" options:0 context:nil];
        [self setPlayerVolume: s_sound_loudness];
        [m_player play];
    }
    
    if (!t_success)
        [self cleanUp];
    
    return t_success;    
}
	
-(void)setPlayerVolume:(float)p_volume
{
    NSMutableArray *t_all_audio_params = [NSMutableArray array];
    AVAssetTrack *t_track;
    AVMutableAudioMix *t_audio_mix;
    AVMutableAudioMixInputParameters *t_audio_input_params;
    for (t_track in [[m_player_item asset] tracks])
    {
        t_audio_input_params = [AVMutableAudioMixInputParameters audioMixInputParameters];
        [t_audio_input_params setVolume:p_volume atTime:kCMTimeZero];
        [t_audio_input_params setTrackID:[t_track trackID]];
        [t_all_audio_params addObject:t_audio_input_params];
    }
    if ([[[m_player_item asset] tracks] count] > 0)
    {
        t_audio_mix = [AVMutableAudioMix audioMix];
        [t_audio_mix setInputParameters:t_all_audio_params];
        [m_player_item setAudioMix:t_audio_mix];
    }
}
@end

bool MCSystemSetPlayLoudness(uint2 p_loudness)
{
    // HC-2011-11-21: [[ Bug 9873 ]] Make sure the upper volume limit is 100.
    uint2 t_loudness;
    p_loudness > 100 ? t_loudness = 100 : t_loudness = p_loudness;
    s_sound_loudness = t_loudness / 100.0;
    if (s_sound_player_delegate != nil)
		MCIPhoneRunBlockOnMainFiber(^(void) {
			[s_sound_player_delegate setPlayerVolume: s_sound_loudness];
		});
	return true;
}

bool MCSystemGetPlayLoudness(uint2& r_loudness)
{
	r_loudness = s_sound_loudness * 100;
	return true;
}

bool MCSystemPlaySound(MCStringRef p_sound, bool p_looping)
{
	if (s_sound_player_delegate != nil)
	{
		[s_sound_player_delegate dealloc];
		s_sound_player_delegate = nil;
	}
	
    bool t_success;
    t_success = true;
    
    NSURL *t_url;
    if (t_success)
    {
        // Check if we are playing an ipod file or a resource file.
		if (MCStringBeginsWith(p_sound, MCSTR("ipod-library://"), kMCCompareExact) || \
			MCStringBeginsWith(p_sound, MCSTR("http://"), kMCCompareExact) || \
			MCStringBeginsWith(p_sound, MCSTR("https://"), kMCCompareExact))
        {
            t_url = [NSURL URLWithString: MCStringConvertToAutoreleasedNSString(p_sound)];
        }
		
        else
        {
            MCAutoStringRef t_sound_file;
			/* UNCHECKED */ MCS_resolvepath(p_sound, &t_sound_file);
            t_url = [NSURL fileURLWithPath: MCStringConvertToAutoreleasedNSString(*t_sound_file)];
        }
        t_success = t_url != nil;
    }
    
	bool *t_success_ptr;
	t_success_ptr = &t_success;
    if (t_success)
    { 
		MCIPhoneRunBlockOnMainFiber(^(void) {
			s_sound_player_delegate = [com_runrev_livecode_MCSoundPlayerDelegate alloc];
			[s_sound_player_delegate init];
			*t_success_ptr = [s_sound_player_delegate playSound:t_url looping: p_looping];
		});
    }
	
    // MM-2012-02-29: [[ BUG 10021 ]] Only store the sound if playback is successful
    if (t_success)
    {
        MCValueAssign(s_sound_file, p_sound);
        MCresult->clear();
    }
    else
    {
        MCValueAssign(s_sound_file, kMCEmptyString);
        MCresult->sets("could not play sound");
    }
    
	return true;		
}

void MCSystemGetPlayingSound(MCStringRef &r_sound)
{
	r_sound = MCValueRetain(s_sound_file);
}

////////////////////////////////////////////////////////////////////////////////

struct MCSystemSoundChannel;

@interface com_runrev_livecode_MCSystemSoundChannelDelegate : NSObject <AVAudioPlayerDelegate>
{
	MCSystemSoundChannel *m_channel;
}

- (id)initWithSoundChannel: (MCSystemSoundChannel *)channel;

- (void)audioPlayerDidFinishPlaying: (AVAudioPlayer *)player successfully: (BOOL)flag;
@end

struct MCSystemSoundPlayer
{
	MCStringRef sound;
	AVAudioPlayer *player;
	MCObjectHandle object;
};

struct MCSystemSoundChannel
{
	MCSystemSoundChannel *next;
	MCStringRef name;
	com_runrev_livecode_MCSystemSoundChannelDelegate *delegate;
	MCSystemSoundPlayer current_player;
	MCSystemSoundPlayer next_player;
	bool paused;
	uint32_t volume;
};

static MCSystemSoundChannel *s_sound_channels = nil;

static void delete_player_on_channel(MCSystemSoundChannel *p_channel, MCSystemSoundPlayer& x_player)
{
	[x_player . player setDelegate: nil];
	[x_player . player stop];
	[x_player . player release];
	x_player . player = nil;
	MCValueRelease(x_player.sound);
	x_player . object = nil;
}

static void delete_sound_channel(MCSystemSoundChannel *p_channel)
{
	MCListRemove(s_sound_channels, p_channel);
	
	if (p_channel -> next_player . player != nil)
		delete_player_on_channel(p_channel, p_channel -> next_player);
	
	if (p_channel -> current_player . player != nil)
		delete_player_on_channel(p_channel, p_channel -> current_player);
	
	[p_channel -> delegate release];
	MCValueRelease(p_channel -> name);
	MCMemoryDestroy(p_channel);
}

static bool new_sound_channel(MCStringRef p_channel, MCSystemSoundChannel*& r_channel)
{
	bool t_success;
	t_success = true;
	
	MCSystemSoundChannel *t_channel;
	t_channel = nil;
	if (t_success)
		t_success = MCMemoryCreate(t_channel);
	
	if (t_success)
		t_channel->name = MCValueRetain(p_channel);
	
	if (t_success)
	{
		t_channel -> delegate = [[com_runrev_livecode_MCSystemSoundChannelDelegate alloc] initWithSoundChannel: t_channel];
		if (t_channel -> delegate == nil)
			t_success = false;
	}
	
	if (t_success)
	{
		t_channel -> volume = 100;
		t_channel -> paused = false;
		
		MCListPushFront(s_sound_channels, t_channel);
	
		r_channel = t_channel;
	}
	else
		delete_sound_channel(t_channel);
	
	return t_success;
}

static bool find_sound_channel(MCStringRef p_channel, bool p_create, MCSystemSoundChannel*& r_channel)
{
	for(MCSystemSoundChannel *t_channel = s_sound_channels; t_channel != nil; t_channel = t_channel -> next)
		if (MCStringIsEqualTo(p_channel, t_channel -> name, kMCCompareCaseless))
		{
			r_channel = t_channel;
			return true;
		}
	
	if (p_create)
		return new_sound_channel(p_channel, r_channel);
		
	return false;
}

static bool new_player_for_channel(MCSystemSoundChannel *p_channel, MCStringRef p_file, bool p_looping, MCObjectHandle p_object, MCSystemSoundPlayer& x_player)
{
	MCStringRef t_resolved_file = nil;
	/* UNCHECKED */ MCS_resolvepath(p_file, t_resolved_file);
	
	CFStringRef cfresolvedfile = nil;
	/* UNCHECKED */ MCStringConvertToCFStringRef(t_resolved_file, cfresolvedfile);
	NSURL *t_url;
	t_url = [NSURL fileURLWithPath: (NSString *)cfresolvedfile];
	CFRelease(cfresolvedfile);
	
	AVAudioPlayer *t_player;
	t_player = [[AVAudioPlayer alloc] initWithContentsOfURL: t_url error: nil];
	if (t_player == nil)
	{
		MCValueRelease(t_resolved_file);
		return false;
	}
	
	if (p_looping)
		[t_player setNumberOfLoops: -1];
	
	[t_player setVolume: p_channel -> volume / 100.0f];
	
	[t_player setDelegate: p_channel -> delegate];
	
	x_player . player = t_player;
	x_player . sound = t_resolved_file;
	x_player . object = p_object;
	
	return true;
}

bool MCSystemPlaySoundOnChannel(MCStringRef p_channel, MCStringRef p_file, MCSoundChannelPlayType p_type, MCObjectHandle p_object)
{
	MCSystemSoundChannel *t_channel;
	if (!find_sound_channel(p_channel, true, t_channel))
		return false;
	
	// Always delete the next player
	if (t_channel -> next_player . player != nil)
		delete_player_on_channel(t_channel, t_channel -> next_player);
	
	if (p_type == kMCSoundChannelPlayNext &&
		t_channel -> current_player . player != nil)
	{	
		// If no file is given, cancel the next sound.
		if (p_file == nil || MCStringIsEmpty(p_file))
			return true;
		
		if (new_player_for_channel(t_channel, p_file, false, p_object, t_channel -> next_player))
		{
            // MM-2012-02-11: [[Bug 9916]] iPhonePlaySoundOnChannel "next" option has too much latency
            if (![t_channel -> next_player . player playAtTime: [t_channel -> current_player . player deviceCurrentTime] + ([t_channel -> current_player . player duration] - [t_channel -> current_player . player currentTime])])
			{
				delete_player_on_channel(t_channel, t_channel -> next_player);
				return false;
			}
			
			return true;
		}
		
		return false;
	}
	
	// Delete any current player
	if (t_channel -> current_player . player != nil)
		delete_player_on_channel(t_channel, t_channel -> current_player);
	
	// If no file is given, cancel the current sound.
	if (p_file == nil || MCStringIsEmpty(p_file))
		return true;
	
	if (new_player_for_channel(t_channel, p_file, p_type == kMCSoundChannelPlayLooping, p_object, t_channel -> current_player))
	{
		// If we played next on an empty channel, then prepare and pause.
		if (p_type == kMCSoundChannelPlayNext)
		{
			if (![t_channel -> current_player . player prepareToPlay])
			{
				delete_player_on_channel(t_channel, t_channel -> current_player);
				return false;
			}
			
			t_channel -> paused = true;
		}
		else
		{
			if (![t_channel -> current_player . player play])
			{
				delete_player_on_channel(t_channel, t_channel -> current_player);
				return false;
			}
			
			t_channel -> paused = false;
		}
		
		return true;
	}
	
	return false;
}

bool MCSystemStopSoundChannel(MCStringRef p_channel)
{
	MCSystemSoundChannel *t_channel;
	if (!find_sound_channel(p_channel, false, t_channel))
		return false;
	
	if (t_channel -> next_player . player != nil)
		delete_player_on_channel(t_channel, t_channel -> next_player);
	
	if (t_channel -> current_player . player != nil)
		delete_player_on_channel(t_channel, t_channel -> current_player);
	
	t_channel -> paused = false;
		
	return true;
}

// MM-2012-02-11: Refactored PauseResume to two funciton calls
bool MCSystemPauseSoundChannel(MCStringRef p_channel)
{
	MCSystemSoundChannel *t_channel;
	if (!find_sound_channel(p_channel, false, t_channel))
		return false;
	
	// If there is no sound on the current channel, we can't pause/resume.
	if (t_channel -> current_player . player == nil)
		return true;
	
	if (!t_channel -> paused)
	{
		t_channel -> paused = true;
		[t_channel -> current_player . player pause];
	}
	
	return true;
}

bool MCSystemResumeSoundChannel(MCStringRef p_channel)
{
	MCSystemSoundChannel *t_channel;
	if (!find_sound_channel(p_channel, false, t_channel))
		return false;
	
	// If there is no sound on the current channel, we can't pause/resume.
	if (t_channel -> current_player . player == nil)
		return true;
	
	else if (t_channel -> paused)
	{
		t_channel -> paused = false;
		[t_channel -> current_player . player play];
	}
	
	return true;
}

bool MCSystemDeleteSoundChannel(MCStringRef p_channel)
{
	MCSystemSoundChannel *t_channel;
	if (!find_sound_channel(p_channel, false, t_channel))
		return false;
	
	MCListRemove(s_sound_channels, t_channel);
	delete_sound_channel(t_channel);
	
	return true;
}

bool MCSystemSetSoundChannelVolume(MCStringRef p_channel, int32_t p_loudness)
{
	MCSystemSoundChannel *t_channel;
	if (!find_sound_channel(p_channel, true, t_channel))
		return false;
	
	t_channel -> volume = p_loudness;
	if (t_channel -> current_player . player != nil)
		[t_channel -> current_player . player setVolume: p_loudness / 100.0f];
	if (t_channel -> next_player . player != nil)
		[t_channel -> next_player . player setVolume: p_loudness / 100.0f];
	
	return true;
}

bool MCSystemSoundChannelVolume(MCStringRef p_channel, int32_t& r_volume)
{
	MCSystemSoundChannel *t_channel;
	if (!find_sound_channel(p_channel, false, t_channel))
		return false;
	
	r_volume = t_channel -> volume;
	
	return true;
}

bool MCSystemSoundChannelStatus(MCStringRef p_channel, intenum_t& r_status)
{
	MCSystemSoundChannel *t_channel;
	if (!find_sound_channel(p_channel, false, t_channel))
		return false;
	
	if (t_channel -> current_player . player == nil &&
		t_channel -> next_player . player == nil)
		r_status = kMCSoundChannelStatusStopped;
	else if (t_channel -> paused)
		r_status = kMCSoundChannelStatusPaused;
	else
		r_status = kMCSoundChannelStatusPlaying;
	
	return true;
}

bool MCSystemSoundOnChannel(MCStringRef p_channel, MCStringRef& r_sound)
{
	MCSystemSoundChannel *t_channel;
	if (!find_sound_channel(p_channel, false, t_channel))
		return false;
	
	MCValueAssign(r_sound, t_channel -> current_player . sound);
	return true;
}

bool MCSystemNextSoundOnChannel(MCStringRef p_channel, MCStringRef& r_sound)
{
	MCSystemSoundChannel *t_channel;
	if (!find_sound_channel(p_channel, false, t_channel))
		return false;
	
	MCValueAssign(r_sound, t_channel -> next_player . sound);
	return true;
}

// MM-2012-02-11: Refactored to return a formatted sting of channels
bool MCSystemListSoundChannels(MCStringRef& r_channels)
{
	MCAutoListRef t_list;
	MCListCreateMutable('\n', &t_list);
    bool t_elementsfound = false;
    
	for(MCSystemSoundChannel *t_channel = s_sound_channels; t_channel != nil; t_channel = t_channel -> next)
    {
		t_elementsfound = true;
		MCListAppend(*t_list, t_channel->name);
    }
	
    MCListCopyAsString(*t_list, r_channels);
	return t_elementsfound;
}

extern void MCSoundPostSoundFinishedOnChannelMessage(MCStringRef p_channel, MCStringRef p_sound, MCObjectHandle p_object);

@implementation com_runrev_livecode_MCSystemSoundChannelDelegate

- (id)initWithSoundChannel: (MCSystemSoundChannel *)channel
{
	self = [super init];
	if (self == nil)
		return nil;
	
	self -> m_channel = channel;
	
	return self;
}

- (void)audioPlayerDidFinishPlaying: (AVAudioPlayer *)player successfully: (BOOL)flag
{
    MCSoundPostSoundFinishedOnChannelMessage(m_channel -> name, m_channel -> current_player . sound, m_channel -> current_player . object);						   
	delete_player_on_channel(m_channel, m_channel -> current_player);
	if (m_channel -> next_player . player != nil)
	{
		m_channel -> current_player = m_channel -> next_player;
        
        // Re-initialise the sound player struct
        MCMemoryReinit(m_channel->next_player);
	}
}

@end

////////////////////////////////////////////////////////////////////////////////

// MM-2012-09-07: Added support for setting the category of the current audio session (how mute button is handled etc.
bool MCSystemSetAudioCategory(intenum_t p_category)
{
    NSString *t_category;
    t_category = nil;
    switch (p_category)
    {
        case kMCSoundAudioCategoryAmbient:
            t_category = AVAudioSessionCategoryAmbient;
            break;
        case kMCSoundAudioCategorySoloAmbient:
            t_category = AVAudioSessionCategorySoloAmbient;
            break;
        case kMCSoundAudioCategoryPlayback:
            t_category = AVAudioSessionCategoryPlayback;
            break;
        case kMCSoundAudioCategoryRecord:
            t_category = AVAudioSessionCategoryRecord;
            break;
        case kMCSoundAudioCategoryPlayAndRecord:
            t_category = AVAudioSessionCategoryPlayAndRecord;
            break;
        case kMCSoundAudioCategoryAudioProcessing:
            t_category = AVAudioSessionCategoryAudioProcessing;
            break;
        default:
            return false;
    }
    if (t_category != nil)
        return [[AVAudioSession sharedInstance] setCategory: t_category error: NULL] == YES;
    return false;
}

////////////////////////////////////////////////////////////////////////////////
