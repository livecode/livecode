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
#include "mcio.h"

#include "globals.h"
#include "debug.h"
#include "handler.h"

#include "exec.h"
#include "mblsyntax.h"

////////////////////////////////////////////////////////////////////////////////

static MCExecEnumTypeElementInfo _kMCSoundAudioCategoryElementInfo[] =
{
    { "ambient", kMCSoundAudioCategoryAmbient, false},
    { "solo ambient", kMCSoundAudioCategorySoloAmbient, false},
    { "playback", kMCSoundAudioCategoryPlayback, false},
    { "record", kMCSoundAudioCategoryRecord, false},
    { "play and record", kMCSoundAudioCategoryPlayAndRecord, false},
    { "audio processing", kMCSoundAudioCategoryAudioProcessing, false}
};

static MCExecEnumTypeInfo _kMCSoundAudioCategoryTypeInfo =
{
    "Sound.AudioCategory",
    sizeof(_kMCSoundAudioCategoryElementInfo) / sizeof(MCExecEnumTypeElementInfo),
    _kMCSoundAudioCategoryElementInfo
};

MCExecEnumTypeInfo* kMCSoundAudioCategoryTypeInfo = &_kMCSoundAudioCategoryTypeInfo;

static MCExecEnumTypeElementInfo _kMCSoundChannelStatusElementInfo[] =
{
    { "stopped", kMCSoundChannelStatusStopped, false},
    { "paused", kMCSoundChannelStatusPaused, false},
    { "playing", kMCSoundChannelStatusPlaying, false}
};

static MCExecEnumTypeInfo _kMCSoundChannelStatusTypeInfo =
{
    "Sound.ChannelStatus",
    sizeof(_kMCSoundChannelStatusElementInfo) / sizeof(MCExecEnumTypeElementInfo),
    _kMCSoundChannelStatusElementInfo
};

MCExecEnumTypeInfo* kMCSoundChannelStatusTypeInfo = &_kMCSoundChannelStatusTypeInfo;

static MCExecEnumTypeElementInfo _kMCSoundChannelPlayTypeElementInfo[] =
{
    { "play now", kMCSoundChannelPlayNow, false},
    { "play next", kMCSoundChannelPlayNext, false},
    { "play looping", kMCSoundChannelPlayLooping, false}
};

static MCExecEnumTypeInfo _kMCSoundChannelPlayTypeInfo =
{
    "Sound.ChannelPlayType",
    sizeof(_kMCSoundChannelPlayTypeElementInfo) / sizeof(MCExecEnumTypeElementInfo),
    _kMCSoundChannelPlayTypeElementInfo
};

MCExecEnumTypeInfo* kMCSoundChannelPlayType = &_kMCSoundChannelPlayTypeInfo;

////////////////////////////////////////////////////////////////////////////////

void MCSoundExecPlaySoundOnChannel(MCExecContext& ctxt, MCStringRef p_channel, MCStringRef p_file, intenum_t p_type)
{
    bool t_success;
	t_success = true;
    
    MCObjectHandle t_handle;
    t_handle = nil;
    if (t_success)
        t_handle = ctxt.GetObjectHandle();
    
	if (t_success)
        t_success = MCSystemPlaySoundOnChannel(p_channel, p_file, (MCSoundChannelPlayType)p_type, t_handle);
	if (!t_success)
    {
		ctxt.SetTheResultToStaticCString("could not play sound");
    }
}

void MCSoundExecStopPlayingOnChannel(MCExecContext& ctxt, MCStringRef p_channel)
{
    bool t_success;
	t_success = true;
    
	if (t_success)
        t_success = MCSystemStopSoundChannel(p_channel);
	if (!t_success)
		ctxt.SetTheResultToStaticCString("could not find channel");
}

void MCSoundExecPausePlayingOnChannel(MCExecContext& ctxt, MCStringRef p_channel)
{
    bool t_success;
	t_success = true;
    
	if (t_success)
        t_success = MCSystemPauseSoundChannel(p_channel);
	if (!t_success)
		ctxt.SetTheResultToStaticCString("could not find channel");
}

void MCSoundExecResumePlayingOnChannel(MCExecContext& ctxt, MCStringRef p_channel)
{
    bool t_success;
	t_success = true;
    
	if (t_success)
        t_success = MCSystemResumeSoundChannel(p_channel);
	if (!t_success)
		ctxt.SetTheResultToStaticCString("could not find channel");
}

void MCSoundExecDeleteSoundChannel(MCExecContext& ctxt, MCStringRef p_channel)
{
    bool t_success;
	t_success = true;
    
	if (t_success)
        t_success = MCSystemDeleteSoundChannel(p_channel);
	if (!t_success)
		ctxt.SetTheResultToStaticCString("could not find channel");
}

void MCSoundSetVolumeOfChannel(MCExecContext& ctxt, MCStringRef p_channel, int32_t p_volume)
{
    bool t_success;
	t_success = true;
    
    if (t_success)
    {
        if (p_volume < 0)
            p_volume = 0;
        else if (p_volume > 100)
            p_volume = 100;
    }
    
	if (t_success)
        t_success = MCSystemSetSoundChannelVolume(p_channel, p_volume);
    
	if (t_success)
        return;
    
	ctxt.SetTheResultToStaticCString("could not find channel");
    ctxt.Throw();
}

void MCSoundGetVolumeOfChannel(MCExecContext& ctxt, MCStringRef p_channel, int32_t& r_volume)
{
    bool t_success;
	t_success = true;
    
	if (t_success)
        t_success = MCSystemSoundChannelVolume(p_channel, r_volume);
    
	if (t_success)
        return;
    
	ctxt.SetTheResultToStaticCString("could not find channel");
    ctxt.Throw();
}

void MCSoundGetStatusOfChannel(MCExecContext& ctxt, MCStringRef p_channel, intenum_t& r_status)
{
    bool t_success;
	t_success = true;
    
	if (t_success)
        t_success = MCSystemSoundChannelStatus(p_channel, r_status);
    
	if (t_success)
        return;
    
	ctxt.SetTheResultToStaticCString("could not find channel");
    ctxt.Throw();
}

void MCSoundGetSoundOfChannel(MCExecContext& ctxt, MCStringRef p_channel, MCStringRef &r_sound)
{
    bool t_success;
	t_success = true;
    
	if (t_success)
        t_success = MCSystemSoundOnChannel(p_channel, r_sound);
    
	if (t_success)
        return;
    
	ctxt.SetTheResultToStaticCString("could not find channel");
    ctxt.Throw();
}

void MCSoundGetNextSoundOfChannel(MCExecContext& ctxt, MCStringRef p_channel, MCStringRef &r_sound)
{
    bool t_success;
	t_success = true;
    
	if (t_success)
        t_success = MCSystemNextSoundOnChannel(p_channel, r_sound);
    
	if (t_success)
        return;
    
	ctxt.SetTheResultToStaticCString("could not find channel");
    ctxt.Throw();
}

void MCSoundGetSoundChannels(MCExecContext& ctxt, MCStringRef &r_channels)
{
    bool t_success;
	t_success = true;
    
	if (t_success)
        t_success = MCSystemListSoundChannels(r_channels);
    
	if (t_success)
        return;
    
    ctxt.SetTheResultToStaticCString("could not find channel");
    ctxt.Throw();
}

// MM-2012-09-07: Added support for setting the category of the current audio session (how mute button is handled etc.
void MCSoundSetAudioCategory(MCExecContext &ctxt, intenum_t p_category)
{
    if(MCSystemSetAudioCategory(p_category))
        return;
    
    ctxt.SetTheResultToStaticCString("unable to set audio category");
    ctxt.Throw();
    
}
