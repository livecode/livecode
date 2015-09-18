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

#include <Cocoa/Cocoa.h>

#include <AudioToolbox/AudioToolbox.h>

#include "platform.h"
#include "platform-internal.h"

////////////////////////////////////////////////////////////////////////////////

@interface com_runrev_livecode_MCSoundDelegate: NSObject<NSSoundDelegate>

- (void)sound: (NSSound *)sound didFinishPlaying:(BOOL)finishedPlaying;

@end

@implementation com_runrev_livecode_MCSoundDelegate

- (void)sound:(NSSound *)sound didFinishPlaying:(BOOL)finishedPlaying
{
    MCPlatformCallbackSendSoundFinished((MCPlatformSoundRef)sound);
}

@end

static com_runrev_livecode_MCSoundDelegate *s_delegate = nil;

////////////////////////////////////////////////////////////////////////////////

void MCPlatformSoundCreateWithData(const void *p_data, size_t p_data_size, MCPlatformSoundRef& r_sound)
{
    NSData *t_data;
    t_data = [NSData dataWithBytes: p_data length: p_data_size];
    
    NSSound *t_sound;
    t_sound = [[NSSound alloc] initWithData: t_data];

    if (s_delegate == nil)
        s_delegate = [[com_runrev_livecode_MCSoundDelegate alloc] init];
    
    [t_sound setDelegate: s_delegate];
    
    r_sound = (MCPlatformSoundRef)t_sound;
}

void MCPlatformSoundRetain(MCPlatformSoundRef self)
{
    NSSound *t_sound;
    t_sound = (NSSound *)self;
    [t_sound retain];
}

void MCPlatformSoundRelease(MCPlatformSoundRef self)
{
    NSSound *t_sound;
    t_sound = (NSSound *)self;
    [t_sound release];
}

bool MCPlatformSoundIsPlaying(MCPlatformSoundRef self)
{
    NSSound *t_sound;
    t_sound = (NSSound *)self;
    return [t_sound isPlaying];
}

void MCPlatformSoundPlay(MCPlatformSoundRef self)
{
    NSSound *t_sound;
    t_sound = (NSSound *)self;
    [t_sound play];
}

void MCPlatformSoundPause(MCPlatformSoundRef self)
{
    NSSound *t_sound;
    t_sound = (NSSound *)self;
    [t_sound pause];
}

void MCPlatformSoundResume(MCPlatformSoundRef self)
{
    NSSound *t_sound;
    t_sound = (NSSound *)self;
    [t_sound resume];
}

void MCPlatformSoundStop(MCPlatformSoundRef self)
{
    NSSound *t_sound;
    t_sound = (NSSound *)self;
    [t_sound stop];
}

void MCPlatformSoundSetProperty(MCPlatformSoundRef self, MCPlatformSoundProperty property, MCPlatformPropertyType type, void *value)
{
    NSSound *t_sound;
    t_sound = (NSSound *)self;
    
    switch(property)
    {
        case kMCPlatformSoundPropertyVolume:
            [t_sound setVolume: *(double *)value];
            break;
        case kMCPlatformSoundPropertyLooping:
            [t_sound setLoops: *(bool *)value];
            break;
            
        default:
            assert(false);
            break;
    }
}

void MCPlatformSoundGetProperty(MCPlatformSoundRef self, MCPlatformSoundProperty property, MCPlatformPropertyType type, void *value)
{
    NSSound *t_sound;
    t_sound = (NSSound *)self;
}

////////////////////////////////////////////////////////////////////////////////

static AudioDeviceID get_default_audio_device_id(void)
{
    AudioObjectPropertyAddress t_addr;
    t_addr . mElement = kAudioObjectPropertyElementMaster;
    t_addr . mSelector = kAudioHardwarePropertyDefaultOutputDevice;
    t_addr . mScope = kAudioObjectPropertyScopeGlobal;

    AudioDeviceID t_id;
    UInt32 t_prop_size;
    t_id = kAudioObjectUnknown;
    t_prop_size = sizeof(AudioDeviceID);
    if (AudioHardwareServiceHasProperty(kAudioObjectSystemObject, &t_addr))
        AudioHardwareServiceGetPropertyData(kAudioObjectSystemObject, &t_addr, 0, NULL, &t_prop_size, &t_id);
    
    return t_id;
}

static bool get_device_mute(AudioDeviceID p_device, bool& r_mute)
{
    AudioObjectPropertyAddress t_addr;
    t_addr . mElement = kAudioObjectPropertyElementMaster;
    t_addr . mScope = kAudioDevicePropertyScopeOutput;
    t_addr . mSelector = kAudioDevicePropertyMute;
    if (!AudioHardwareServiceHasProperty(p_device, &t_addr))
        return false;
    
    UInt32 t_prop_size;
    UInt32 t_mute;
    t_prop_size = sizeof(UInt32);
    if (AudioHardwareServiceGetPropertyData(p_device, &t_addr, 0, NULL, &t_prop_size, &t_mute) != noErr)
        return false;
    
    r_mute = (t_mute == 1);
    return true;
}

void MCMacPlatformGetGlobalVolume(double& r_volume)
{
    AudioObjectPropertyAddress t_addr;
    t_addr . mElement = kAudioObjectPropertyElementMaster;
    t_addr . mSelector = kAudioHardwareServiceDeviceProperty_VirtualMasterVolume;
    t_addr . mScope = kAudioDevicePropertyScopeOutput;
    
    AudioDeviceID t_device_id;
    t_device_id = get_default_audio_device_id();
    
    Float32 t_volume;
    UInt32 t_prop_size;
    t_prop_size = sizeof(Float32);
    
    // AL-2014-08-12: [[ Bug 13160 ]] If the system audio is muted, return 0
    bool t_mute;
    if (t_device_id == kAudioObjectUnknown ||
        !AudioHardwareServiceHasProperty(t_device_id, &t_addr) ||
        AudioHardwareServiceGetPropertyData(t_device_id, &t_addr, 0, NULL, &t_prop_size, &t_volume) != noErr ||
        !get_device_mute(t_device_id, t_mute))
    {
        r_volume = 1.0;
        return;
    }
    
    r_volume = t_mute ? 0 : fmin(fmax(t_volume, 0.0), 1.0);
}

static bool set_device_mute(AudioDeviceID p_device, bool p_mute)
{
    AudioObjectPropertyAddress t_addr;
    t_addr . mElement = kAudioObjectPropertyElementMaster;
    t_addr . mScope = kAudioDevicePropertyScopeOutput;
    t_addr . mSelector = kAudioDevicePropertyMute;
    if (!AudioHardwareServiceHasProperty(p_device, &t_addr))
        return false;
    
    Boolean t_can_set;
    if (AudioHardwareServiceIsPropertySettable(p_device, &t_addr, &t_can_set) != noErr ||
        !t_can_set)
        return false;
    
    UInt32 t_prop_size;
    UInt32 t_mute;
    t_prop_size = sizeof(UInt32);
    t_mute = p_mute ? 1 : 0;
    if (AudioHardwareServiceSetPropertyData(p_device, &t_addr, 0, NULL, t_prop_size, &t_mute) != noErr)
        return false;
    
    return true;
}

static bool set_device_volume(AudioDeviceID p_device, double p_volume)
{
    AudioObjectPropertyAddress t_addr;
    t_addr . mElement = kAudioObjectPropertyElementMaster;
    t_addr . mScope = kAudioDevicePropertyScopeOutput;
    t_addr . mSelector = kAudioHardwareServiceDeviceProperty_VirtualMasterVolume;
    if (!AudioHardwareServiceHasProperty(p_device, &t_addr))
        return false;
    
    if (!AudioHardwareServiceHasProperty(p_device, &t_addr))
        return false;
    
    Boolean t_can_set;
    t_can_set = NO;
    if (AudioHardwareServiceIsPropertySettable(p_device, &t_addr, &t_can_set) != noErr ||
        !t_can_set)
        return false;
    
    UInt32 t_prop_size;
    Float32 t_volume;
    t_prop_size = sizeof(Float32);
    t_volume = p_volume;
    if (AudioHardwareServiceSetPropertyData(p_device, &t_addr, 0, NULL, t_prop_size, &t_volume) != noErr)
        return false;
    
    return true;
}

void MCMacPlatformSetGlobalVolume(double p_volume)
{
    p_volume = fmin(fmax(p_volume, 0.0), 1.0);
    
    AudioDeviceID t_device_id;
    t_device_id = get_default_audio_device_id();
    if (t_device_id == kAudioObjectUnknown)
        return;
    
    set_device_volume(t_device_id, p_volume);
    set_device_mute(t_device_id, p_volume < 0.001);
}

////////////////////////////////////////////////////////////////////////////////
