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


#include "globals.h"
#include "stack.h"
#include "system.h"
#include "player.h"
#include "eventqueue.h"
#include "osspec.h"

#include "mblandroid.h"
#include "mblandroidutil.h"
#include "mblandroidjava.h"

#include "mblsyntax.h"

#include <jni.h>

bool path_to_apk_path(MCStringRef p_path, MCStringRef &r_apk_path);

////////////////////////////////////////////////////////////////////////////////

bool MCSystemSetPlayLoudness(uint2 p_loudness)
{
	bool t_success;
	MCAndroidEngineCall("setPlayLoudness", "bi", &t_success, p_loudness);
	return t_success;
}

bool MCSystemGetPlayLoudness(uint2& r_loudness)
{
	int32_t t_loudness;
	MCAndroidEngineCall("getPlayLoudness", "i", &t_loudness);
	r_loudness = t_loudness;
	return true;
}

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

bool MCSystemPlaySound(MCStringRef p_file, bool p_looping)
{
	//MCLog("MCSystemPlaySound(%s, %s)", p_file, p_looping?"true":"false");
	bool t_success;
	
	MCValueRelease(s_sound_file);
	
	/* UNCHECKED */ MCS_resolvepath(p_file, s_sound_file);
    
	MCAutoStringRef t_apk_file;
	if (path_to_apk_path(s_sound_file, &t_apk_file))
		MCAndroidEngineCall("playSound", "bxbb", &t_success, *t_apk_file, true, p_looping);
	else
		MCAndroidEngineCall("playSound", "bxbb", &t_success, s_sound_file, false, p_looping);
	if (!t_success)
	{
		MCValueAssign(s_sound_file, kMCEmptyString);
	}
    
	return t_success;
}

void MCSystemGetPlayingSound(MCStringRef &r_sound)
{
	r_sound = MCValueRetain(s_sound_file);
}

////////////////////////////////////////////////////////////////////////////////

bool MCSystemPlaySoundOnChannel(MCStringRef p_channel, MCStringRef p_file, MCSoundChannelPlayType p_type, MCObjectHandle p_object)
{
    bool t_success;
    t_success = true;

    // IM-2013-11-13: [[ Bug 11428 ]] Resolve path to make sure asset paths are valid
    MCAutoStringRef t_resolved;
    if (t_success)
        t_success = MCS_resolvepath(p_file, &t_resolved);
    
    // Retain a reference to the object on behalf of the Java code
    MCObjectProxy<>* t_proxy = p_object.ExternalRetain();
    
    MCAutoStringRef t_apk_file;
    if (t_success)
        if (path_to_apk_path(*t_resolved, &t_apk_file))
            MCAndroidEngineRemoteCall("playSoundOnChannel", "bxxxibj", &t_success, p_channel, *t_apk_file, p_file, (int32_t) p_type, true, long(t_proxy));
        else
            MCAndroidEngineRemoteCall("playSoundOnChannel", "bxxxibj", &t_success, p_channel, *t_resolved, p_file, (int32_t) p_type, false, long(t_proxy));
    
    return t_success;
}

bool MCSystemStopSoundChannel(MCStringRef p_channel)
{
    bool t_success;
    t_success = true;    
    if (t_success)
        MCAndroidEngineRemoteCall("stopSoundOnChannel", "bx", &t_success, p_channel);
    return t_success;
}

bool MCSystemPauseSoundChannel(MCStringRef p_channel)
{
    bool t_success;
    t_success = true;    
    if (t_success)
        MCAndroidEngineRemoteCall("pauseSoundOnChannel", "bx", &t_success, p_channel);    
    return t_success;
}

bool MCSystemResumeSoundChannel(MCStringRef p_channel)
{
    bool t_success;
    t_success = true;    
    if (t_success)
        MCAndroidEngineRemoteCall("resumeSoundOnChannel", "bx", &t_success, p_channel);    
    return t_success;
}

bool MCSystemDeleteSoundChannel(MCStringRef p_channel)
{
    bool t_success;
    t_success = true;    
    if (t_success)
        MCAndroidEngineRemoteCall("deleteSoundChannel", "bx", &t_success, p_channel);    
    return t_success;
}

bool MCSystemSetSoundChannelVolume(MCStringRef p_channel, int32_t p_volume)
{
    bool t_success;
    t_success = true;    
    if (t_success)
        MCAndroidEngineRemoteCall("setSoundChannelVoulme", "bxi", &t_success, p_channel, p_volume);    
    return t_success;
}

bool MCSystemSoundChannelVolume(MCStringRef p_channel, int32_t& r_volume)
{
    bool t_success;
    t_success = true;    
    if (t_success)
    {
        int32_t t_volume;
        MCAndroidEngineRemoteCall("getSoundChannelVolume", "ix", &t_volume, p_channel);
        if (t_volume >= 0)
            r_volume = t_volume;
        else
            t_success = false;
    }
    return t_success;
}

bool MCSystemSoundChannelStatus(MCStringRef p_channel, intenum_t& r_status)
{
    bool t_success;
    t_success = true;    
    if (t_success)
    {
        int32_t t_status;
        MCAndroidEngineRemoteCall("getSoundChannelStatus", "ix", &t_status, p_channel);
        if (t_status >= 0)
            r_status = t_status;
        else
            t_success = false;
    }
    return t_success;
}

bool MCSystemSoundOnChannel(MCStringRef p_channel, MCStringRef &r_sound)
{
    MCAndroidEngineRemoteCall("getSoundOnChannel", "xx", &r_sound, p_channel);
    return true;
}

bool MCSystemNextSoundOnChannel(MCStringRef p_channel, MCStringRef &r_sound)
{
    MCAndroidEngineRemoteCall("getNextSoundOnChannel", "xx", &r_sound, p_channel);
    return true;
}

bool MCSystemListSoundChannels(MCStringRef &r_channels)
{
    MCAndroidEngineRemoteCall("getSoundChannels", "x", &r_channels);
    return true;
}

bool MCSystemSetAudioCategory(intenum_t p_category)
{
    return true;
}

////////////////////////////////////////////////////////////////////////////////

extern void MCSoundPostSoundFinishedOnChannelMessage(MCStringRef p_channel, MCStringRef p_sound, MCObjectHandle p_object);

extern "C" JNIEXPORT void JNICALL Java_com_runrev_android_SoundModule_doSoundFinishedOnChannel(JNIEnv *env, jobject object, jstring channel, jstring sound, jlong object_handle) __attribute__((visibility("default")));

JNIEXPORT void JNICALL Java_com_runrev_android_SoundModule_doSoundFinishedOnChannel(JNIEnv *env, jobject object, jstring channel, jstring sound, jlong object_proxy)
{
	MCAutoStringRef t_channel;
	MCAutoStringRef t_sound;
	/* UNCHECKED */ MCJavaStringToStringRef(env, channel, &t_channel);
	/* UNCHECKED */ MCJavaStringToStringRef(env, sound, &t_sound);
	
	// Notify the callback object that the sound has finished playing.
	// Don't release the handle here, as it is released via
	// doSoundReleaseCallbackHandle when the player is subsequently
	// reset.
    MCObjectHandle t_object_handle = reinterpret_cast<MCObjectProxy<>*> (object_proxy);
    MCSoundPostSoundFinishedOnChannelMessage(*t_channel, *t_sound, t_object_handle);
}

extern "C" JNIEXPORT void JNICALL Java_com_runrev_android_SoundModule_doSoundReleaseCallbackHandle(JNIEnv *env, jobject object, jlong object_handle) __attribute__((visibility("default")));

JNIEXPORT void JNICALL Java_com_runrev_android_SoundModule_doSoundReleaseCallbackHandle(JNIEnv *env, jobject object, jlong object_proxy)
{    
	// Convert to an object handle and release the reference that was retained
	// on behalf of the Java world.
	MCObjectHandle t_object = reinterpret_cast<MCObjectProxy<>*> (object_proxy);
    t_object.ExternalRelease();
}

////////////////////////////////////////////////////////////////////////////////

extern "C" JNIEXPORT void JNICALL Java_com_runrev_android_SoundModule_doSoundStopped(JNIEnv *env, jobject object) __attribute__((visibility("default")));

JNIEXPORT void JNICALL Java_com_runrev_android_SoundModule_doSoundStopped(JNIEnv *env, jobject object)
{
	//MCLog("doSoundStopped", nil);
	MCValueAssign(s_sound_file, kMCEmptyString);
}

////////////////////////////////////////////////////////////////////////////////
