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

#include "prefix.h"

#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"
#include "mcio.h"

#include "globals.h"
#include "stack.h"
#include "image.h"
#include "param.h"

#include "exec.h"

#include "eventqueue.h"

#include "mblsyntax.h"

////////////////////////////////////////////////////////////////////////////////

bool MCParseParameters(MCParameter*& p_parameters, const char *p_format, ...);

bool MCSystemPlaySoundOnChannel(const char *p_channel, const char *p_file, MCSoundChannelPlayType p_type, MCObjectHandle *p_object);
bool MCSystemStopSoundChannel(const char *p_channel);
bool MCSystemPauseSoundChannel(const char *p_channel);
bool MCSystemResumeSoundChannel(const char *p_channel);
bool MCSystemDeleteSoundChannel(const char *p_channel);
bool MCSystemSetSoundChannelVolume(const char *p_channel, int32_t p_volume);
bool MCSystemSoundChannelVolume(const char *p_channel, int32_t& r_volume);
bool MCSystemSoundChannelStatus(const char *p_channel, MCSoundChannelStatus& r_status);
bool MCSystemSoundOnChannel(const char *p_channel, char*& r_sound);
bool MCSystemNextSoundOnChannel(const char *p_channel, char*& r_sound);
bool MCSystemListSoundChannels(char*& r_channels);
bool MCSystemSetAudioCategory(MCSoundAudioCategory p_category);

////////////////////////////////////////////////////////////////////////////////

class MCSoundFinishedOnChannelEvent: public MCCustomEvent
{
public:
	MCSoundFinishedOnChannelEvent(MCObjectHandle *p_object, const char *p_channel, const char *p_sound)
	{
		m_object = nil;
		m_channel = nil;
		m_sound = nil;
		
		m_object = p_object;
		m_object -> Retain();
		MCCStringClone(p_channel, m_channel);
		MCCStringClone(p_sound, m_sound);
	}
	
	void Destroy(void)
	{
		m_object -> Release();
		MCCStringFree(m_channel);
		MCCStringFree(m_sound);
		delete this;
	}
	
	void Dispatch(void)
	{
		if (m_object -> Exists())
			m_object -> Get() -> message_with_args(MCM_sound_finished_on_channel, m_channel, m_sound);
	}
	
private:
	MCObjectHandle *m_object;
	char *m_channel;
	char *m_sound;
};

void MCSoundPostSoundFinishedOnChannelMessage(const char *p_channel, const char *p_sound, MCObjectHandle *p_object)
{
    MCCustomEvent *t_event = nil;
    t_event = new MCSoundFinishedOnChannelEvent(p_object, p_channel, p_sound);
    if (t_event != nil)
        MCEventQueuePostCustom(t_event);
}

////////////////////////////////////////////////////////////////////////////////

void MCSoundExecPlaySoundOnChannel(MCExecContext& ctxt, const char *p_channel, const char *p_file, MCSoundChannelPlayType p_type)
{    
    bool t_success;
	t_success = true;
    
    MCObjectHandle *t_handle;
    t_handle = nil;
    if (t_success)
        t_handle = ctxt.GetObjectHandle();    
    
	if (t_success)
        t_success = MCSystemPlaySoundOnChannel(p_channel, p_file, p_type, t_handle);
	if (!t_success)
    {
		ctxt.SetTheResultToStaticCString("could not play sound");
        if (t_handle != nil)
            t_handle->Release();
    }
}

void MCSoundExecStopSoundOnChannel(MCExecContext& ctxt, const char *p_channel)
{
    bool t_success;
	t_success = true;
    
	if (t_success)
        t_success = MCSystemStopSoundChannel(p_channel);
	if (!t_success)
		ctxt.SetTheResultToStaticCString("could not find channel");
}

void MCSoundExecPauseSoundOnChannel(MCExecContext& ctxt, const char *p_channel)
{
    bool t_success;
	t_success = true;
    
	if (t_success)
        t_success = MCSystemPauseSoundChannel(p_channel);
	if (!t_success)
		ctxt.SetTheResultToStaticCString("could not find channel");
}

void MCSoundExecResumeSoundOnChannel(MCExecContext& ctxt, const char *p_channel)
{
    bool t_success;
	t_success = true;
    
	if (t_success)
        t_success = MCSystemResumeSoundChannel(p_channel);
	if (!t_success)
		ctxt.SetTheResultToStaticCString("could not find channel");
}

void MCSoundExecDeleteChannel(MCExecContext& ctxt, const char *p_channel)
{
    bool t_success;
	t_success = true;
    
	if (t_success)
        t_success = MCSystemDeleteSoundChannel(p_channel);
	if (!t_success)
		ctxt.SetTheResultToStaticCString("could not find channel");
}

void MCSoundSetVolumeOfChannel(MCExecContext& ctxt, const char *p_channel, int32_t p_volume)
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
	if (!t_success)
		ctxt.SetTheResultToStaticCString("could not find channel");
}

bool MCSoundGetVolumeOfChannel(MCExecContext& ctxt, const char *p_channel, int32_t& r_volume)
{
    bool t_success;
	t_success = true;
    
	if (t_success)
        t_success = MCSystemSoundChannelVolume(p_channel, r_volume);
	if (!t_success)
		ctxt.SetTheResultToStaticCString("could not find channel");
    return t_success;
}

bool MCSoundGetStatusOfChannel(MCExecContext& ctxt, const char *p_channel, MCSoundChannelStatus& r_status)
{
    bool t_success;
	t_success = true;
    
	if (t_success)
        t_success = MCSystemSoundChannelStatus(p_channel, r_status);
	if (!t_success)
		ctxt.SetTheResultToStaticCString("could not find channel");
    return t_success;
}

bool MCSoundGetSoundOfChannel(MCExecContext& ctxt, const char *p_channel, char*& r_sound)
{
    bool t_success;
	t_success = true;
    
	if (t_success)
        t_success = MCSystemSoundOnChannel(p_channel, r_sound);
	if (!t_success)
		ctxt.SetTheResultToStaticCString("could not find channel");
    return t_success;
}

bool MCSoundGetNextSoundOfChannel(MCExecContext& ctxt, const char *p_channel, char*& r_sound)
{
    bool t_success;
	t_success = true;
    
	if (t_success)
        t_success = MCSystemNextSoundOnChannel(p_channel, r_sound);
	if (!t_success)
		ctxt.SetTheResultToStaticCString("could not find channel");
    return t_success;
}

bool MCSoundGetSoundChannels(MCExecContext& ctxt, char*& r_channels)
{
    bool t_success;
	t_success = true;
    
	if (t_success)
        t_success = MCSystemListSoundChannels(r_channels);
	if (!t_success)
		ctxt.SetTheResultToStaticCString("could not find channel");
    return t_success;
}

// MM-2012-09-07: Added support for setting the category of the current audio session (how mute button is handled etc.
bool MCSoundSetAudioCategory(MCExecContext &ctxt, MCSoundAudioCategory p_category)
{
    if(!MCSystemSetAudioCategory(p_category))
    {
        ctxt.SetTheResultToStaticCString("unable to set audio category");
        return false;
    }
    return true;
}

////////////////////////////////////////////////////////////////////////////////

Exec_stat MCHandlePlaySoundOnChannel(void *context, MCParameter *p_parameters)
{
    MCExecPoint ep(nil, nil, nil);
    MCExecContext t_ctxt(ep);
	t_ctxt . SetTheResultToEmpty();  
    
	bool t_success;
	t_success = true;
	
	char *t_sound, *t_channel, *t_type;
	t_sound = nil;
	t_channel = nil;
	t_type = nil;
	if (t_success)
		t_success = MCParseParameters(p_parameters, "sss", &t_sound, &t_channel, &t_type);
	
	if (t_success)
	{
		MCSoundChannelPlayType t_play_type;
		t_play_type = kMCSoundChannelPlayNow;
		if (MCCStringEqualCaseless(t_type, "next"))
			t_play_type = kMCSoundChannelPlayNext;
		else if (MCCStringEqualCaseless(t_type, "looping"))
			t_play_type = kMCSoundChannelPlayLooping;
		
		MCSoundExecPlaySoundOnChannel(t_ctxt, t_channel, t_sound, t_play_type);
	}
		
	delete t_sound;
	delete t_channel;
	delete t_type;
	
    return t_ctxt.GetExecStat();
}

Exec_stat MCHandlePausePlayingOnChannel(void *context, MCParameter *p_parameters)
{
    MCExecPoint ep(nil, nil, nil);
    MCExecContext t_ctxt(ep);
	t_ctxt . SetTheResultToEmpty();  
    
	bool t_success;
	t_success = true;
	
	char *t_channel;
	t_channel = nil;
	if (t_success)
		t_success = MCParseParameters(p_parameters, "s", &t_channel);
	
	if (t_success)
		MCSoundExecPauseSoundOnChannel(t_ctxt, t_channel);
	
	delete t_channel;
	
    return t_ctxt.GetExecStat();
}

Exec_stat MCHandleResumePlayingOnChannel(void *context, MCParameter *p_parameters)
{
    MCExecPoint ep(nil, nil, nil);
    MCExecContext t_ctxt(ep);
	t_ctxt . SetTheResultToEmpty(); 
    
	bool t_success;
	t_success = true;
		
	char *t_channel;
	t_channel = nil;
	if (t_success)
		t_success = MCParseParameters(p_parameters, "s", &t_channel);
	
	if (t_success)
		MCSoundExecResumeSoundOnChannel(t_ctxt, t_channel);
	
	delete t_channel;
	
    return t_ctxt.GetExecStat();
}

Exec_stat MCHandleStopPlayingOnChannel(void *context, MCParameter *p_parameters)
{
    MCExecPoint ep(nil, nil, nil);
    MCExecContext t_ctxt(ep);
	t_ctxt . SetTheResultToEmpty();
    
	bool t_success;
	t_success = true;
	
	char *t_channel;
	t_channel = nil;
	if (t_success)
		t_success = MCParseParameters(p_parameters, "s", &t_channel);
	
	if (t_success)
		MCSoundExecStopSoundOnChannel(t_ctxt, t_channel);
		
	delete t_channel;
	
    return t_ctxt.GetExecStat();
}

Exec_stat MCHandleDeleteSoundChannel(void *context, MCParameter *p_parameters)
{
    MCExecPoint ep(nil, nil, nil);
    MCExecContext t_ctxt(ep);
	t_ctxt . SetTheResultToEmpty();  
    
	bool t_success;
	t_success = true;
	
	char *t_channel;
	t_channel = nil;
	if (t_success)
		t_success = MCParseParameters(p_parameters, "s", &t_channel);
	
	if (t_success)
		MCSoundExecDeleteChannel(t_ctxt, t_channel);
		
	delete t_channel;
	
    return t_ctxt.GetExecStat();
}

Exec_stat MCHandleSetSoundChannelVolume(void *context, MCParameter *p_parameters)
{
    MCExecPoint ep(nil, nil, nil);
    MCExecContext t_ctxt(ep);
	t_ctxt . SetTheResultToEmpty();  

	bool t_success;
	t_success = true;
	
	int32_t t_volume;
	char *t_channel;
	t_channel = nil;
	if (t_success)
		t_success = MCParseParameters(p_parameters, "su", &t_channel, &t_volume);
	
	if (t_success)
		MCSoundSetVolumeOfChannel(t_ctxt, t_channel, t_volume);
	
	delete t_channel;
	
    return t_ctxt.GetExecStat();
}

Exec_stat MCHandleSoundChannelVolume(void *context, MCParameter *p_parameters)
{
    MCExecPoint ep(nil, nil, nil);
    MCExecContext t_ctxt(ep);
	t_ctxt . SetTheResultToEmpty();  

	bool t_success;
	t_success = true;
	
	char *t_channel;
	t_channel = nil;
	if (t_success)
		t_success = MCParseParameters(p_parameters, "s", &t_channel);
	
	int32_t t_volume;
	if (t_success)
		t_success = MCSoundGetVolumeOfChannel(t_ctxt, t_channel, t_volume);
	
	if (t_success)
		MCresult -> setnvalue(t_volume);
	
	delete t_channel;
	
    return t_ctxt.GetExecStat();
}

Exec_stat MCHandleSoundChannelStatus(void *context, MCParameter *p_parameters)
{
    MCExecPoint ep(nil, nil, nil);
    MCExecContext t_ctxt(ep);
	t_ctxt . SetTheResultToEmpty();
    
	bool t_success;
	t_success = true;
	
	char *t_channel;
	t_channel = nil;
	if (t_success)
		t_success = MCParseParameters(p_parameters, "s", &t_channel);
	
	MCSoundChannelStatus t_status;
	if (t_success)
		t_success = MCSoundGetStatusOfChannel(t_ctxt, t_channel, t_status);
	
	if (t_success && t_status >= 0)
	{
		static const char *s_status_strings[] =
		{
			"stopped",
			"paused",
			"playing"
		};
		MCresult -> sets(s_status_strings[t_status]);
	}
	
	delete t_channel;
	
    return t_ctxt.GetExecStat();	
}

Exec_stat MCHandleSoundOnChannel(void *context, MCParameter *p_parameters)
{
    MCExecPoint ep(nil, nil, nil);
    MCExecContext t_ctxt(ep);
	t_ctxt . SetTheResultToEmpty();

	bool t_success;
	t_success = true;
	
	char *t_channel;
	t_channel = nil;
	if (t_success)
		t_success = MCParseParameters(p_parameters, "s", &t_channel);
	
#ifdef MOBILE_BROKEN
    MCAutoRawCString t_sound;
	if (t_success)
		t_success = MCSoundGetSoundOfChannel(t_ctxt, t_channel, t_sound);
	
    if (t_success)
        if (t_sound.Borrow() != nil)
            ep.copysvalue(t_sound.Borrow());
    
    if (t_success)
        MCresult->store(ep, False);
#endif
    
    return t_ctxt.GetExecStat();
}

Exec_stat MCHandleNextSoundOnChannel(void *context, MCParameter *p_parameters)
{
    MCExecPoint ep(nil, nil, nil);
    MCExecContext t_ctxt(ep);
	t_ctxt . SetTheResultToEmpty();
    
	bool t_success;
	t_success = true;
	
	char *t_channel;
	t_channel = nil;
	if (t_success)
		t_success = MCParseParameters(p_parameters, "s", &t_channel);
	
#ifdef MOBILE_BROKEN
    MCAutoRawCString t_sound;
	if (t_success)
		t_success = MCSoundGetNextSoundOfChannel(t_ctxt, t_channel, t_sound);
	
    if (t_success)
        if (t_sound.Borrow() != nil)
            ep.copysvalue(t_sound.Borrow());
    
    if (t_success)
        MCresult->store(ep, False);
#endif

    return t_ctxt.GetExecStat();
}

Exec_stat MCHandleSoundChannels(void *context, MCParameter *p_parameters)
{
    MCExecPoint ep(nil, nil, nil);
    MCExecContext t_ctxt(ep);
	t_ctxt . SetTheResultToEmpty();	
    
    bool t_success;
	t_success = true;
    
#ifdef MOBILE_BROKEN
    MCAutoRawCString t_channels;
	if (t_success)
		t_success = MCSoundGetSoundChannels(t_ctxt, t_channels);
	
    if (t_success)
        if (t_channels.Borrow() != nil)
            ep.copysvalue(t_channels.Borrow());
    
    if (t_success)
        MCresult->store(ep, False);
#endif

    return t_ctxt.GetExecStat();
}

// MM-2012-09-07: Added support for setting the category of the current audio session (how mute button is handled etc.
Exec_stat MCHandleSetAudioCategory(void *context, MCParameter *p_parameters)
{
    MCExecPoint ep(nil, nil, nil);
    MCExecContext t_ctxt(ep);
	t_ctxt . SetTheResultToEmpty();    
    
	bool t_success;
	t_success = true;
	
	char *t_category_string;
	t_category_string = nil;
	if (t_success)
		t_success = MCParseParameters(p_parameters, "s", &t_category_string);
    
    MCSoundAudioCategory t_category;
    t_category = kMCMCSoundAudioCategoryUnknown;
    if (t_success)
    {
        if (MCCStringEqualCaseless(t_category_string, "ambient"))
            t_category = kMCMCSoundAudioCategoryAmbient;
        else if (MCCStringEqualCaseless(t_category_string, "solo ambient"))
            t_category = kMCMCSoundAudioCategorySoloAmbient;
        else if (MCCStringEqualCaseless(t_category_string, "playback"))
            t_category = kMCMCSoundAudioCategoryPlayback;
        else if (MCCStringEqualCaseless(t_category_string, "record"))
            t_category = kMCMCSoundAudioCategoryRecord;
        else if (MCCStringEqualCaseless(t_category_string, "play and record"))
            t_category = kMCMCSoundAudioCategoryPlayAndRecord;
        else if (MCCStringEqualCaseless(t_category_string, "audio processing"))
            t_category = kMCMCSoundAudioCategoryAudioProcessing;
    }

    if (t_success)
        t_success = MCSoundSetAudioCategory(t_ctxt, t_category);

#ifdef MOBILE_BROKEN
    if (t_success)
        MCresult->store(ep, False);
#endif
    
    MCCStringFree(t_category_string);
    
    return t_ctxt.GetExecStat();
}

