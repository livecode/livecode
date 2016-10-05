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

#include <Carbon/Carbon.h>

#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <math.h>

#include "revspeech.h"

#include "osxspeech.h"

INarrator *InstantiateNarrator(NarratorProvider p_provider)
{
	return new OSXSpeechNarrator();
}

INarrator::~INarrator(void)
{
	// Nothing required.
}

OSXSpeechNarrator::OSXSpeechNarrator(void)
{
	spchannel = NULL;
	strcpy(speechvoice, "empty");
	speechtext = NULL;
	speechspeed = 0;
	speechpitch = 0;
}

OSXSpeechNarrator::~OSXSpeechNarrator(void)
{
	// Nothing required.
}

bool OSXSpeechNarrator::Initialize(void)
{
	// Nothing required.
	return true;
}

bool OSXSpeechNarrator::Finalize(void)
{
	if (SpeechStop(true))
	{
		return true;
	}
	return false;
}

bool OSXSpeechNarrator::Start(const char* p_string, bool p_is_utf8)
{
	if (SpeechStart(true) == false)
	{
		return false;
	}
    
    if (speechtext != nil)
        CFRelease(speechtext);
    speechtext = CFStringCreateWithCString(kCFAllocatorDefault, p_string, p_is_utf8 ? kCFStringEncodingUTF8 : kCFStringEncodingMacRoman);
	SpeakCFString(spchannel, speechtext, nil);
	return true;
}

bool OSXSpeechNarrator::Stop(void)
{
    if (spchannel != nil)
		StopSpeech(spchannel);
		
	return true;
}


bool OSXSpeechNarrator::Busy(void)
{
	if (spchannel != nil)
	 {
        if (SpeechBusy())
		{
			return true;
		}
		else
		{
			return false;
		}
    }
	return false;
}

bool OSXSpeechNarrator::ListVoices(NarratorGender p_gender, NarratorListVoicesCallback p_callback, void* p_context)
{
    short NumVoices;
	if (CountVoices(&NumVoices) != noErr)
		return false;
	
    VoiceSpec tVoice;
   
	short t_gender = -1;
	if (p_gender == kNarratorGenderNeuter)
	{
		t_gender = 0;
	}
	if (p_gender == kNarratorGenderMale)
	{
		t_gender = 1;
	}
	
	if (p_gender == kNarratorGenderFemale)
	{
		t_gender = 2;
	}
	
	VoiceDescription myInfo;
    for (short count = 1; count<=NumVoices; count++)
	{
        if ((GetIndVoice(count, &tVoice) == noErr)
            &&(GetVoiceDescription(&tVoice, &myInfo, sizeof(myInfo)) == noErr))
        {
            if ((t_gender != -1) && (myInfo.gender != t_gender)) continue;
            char cvoice[256];
            strncpy(cvoice, (const char*)&myInfo.name[1], myInfo.name[0]);
			// PM-2016-02-15: [[ Bug 16929 ]] Make sure the copied strings are null-terminated
			cvoice[myInfo.name[0]]='\0';
			p_callback(p_context, p_gender, cvoice);
		}
	}
    return true;
}

bool OSXSpeechNarrator::SetVoice(const char* p_string)
{
   	
	strcpy(speechvoice, p_string);
	if (spchannel != nil)
	{
		SpeechStop(false);
		SpeechStart(false);
	}
	return true;
}

bool OSXSpeechNarrator::SetSpeed(int p_speed)
{
	speechspeed = Long2Fix(p_speed);
    if (spchannel != nil)
        SetSpeechRate (spchannel, speechspeed);
		
	return true;
}

bool OSXSpeechNarrator::GetSpeed(int& p_speed)
{
	p_speed = Fix2Long(speechspeed);
	return true;
}

bool OSXSpeechNarrator::SetPitch(int p_pitch)
{
    speechpitch = Long2Fix(p_pitch);
    if (spchannel != nil)
        SetSpeechPitch(spchannel, speechpitch);

	return true;
}

bool OSXSpeechNarrator::GetPitch(int& p_pitch)
{
	p_pitch = Fix2Long(speechpitch);
	return true;
}

// Parameters
//   p_volume : the value to set the volume to. Note, this is assumed to be between 0 and 100
//              and is not checked. A value outside this range may cause unexpected volume or failure to set volume.
// Returns
//   true if setting the volume suceeded, false otherwise.
// Description
//   Sets the speech volume. If a speech channel is not already open, one is created and the volume of it set. Setting the volume
//   will not take affect until after the current text being spoken is finished.
bool OSXSpeechNarrator::SetVolume(int p_volume)
{
	
	// Convert t_volume to the format expected by the speech manager.
	Fixed t_volume;
	t_volume = (p_volume * 65536) / 100;
	
	// As the volume can only be set with an open channel, we check if a channel is open
	// and if not, create a speech channel here using FindAndSelect().
	if (spchannel == NULL)
		FindAndSelect();

	if (spchannel == NULL)
		return false;
		
	if (SetSpeechInfo(spchannel, soVolume, &t_volume) != noErr)
	{
		return false;
	}
	return true;
}

bool OSXSpeechNarrator::GetVolume(int& p_volume)
{
	// If no speech channel has been created then we create one via FindAndSelect() first.
	Fixed t_volume;
	t_volume = -1;
	if (spchannel == NULL)
	{
		FindAndSelect();
	}
	
	// Get the volume of the current channel and check that the operation suceeded.
	if (GetSpeechInfo(spchannel, soVolume, &t_volume) != noErr)
		return false;
	
	// Convert the volume to a standardised value and return.
	p_volume = (((t_volume + 654) * 100) / 65536);
	return true;
}

bool OSXSpeechNarrator::SpeechStart(bool StartInit)
{
    if (spchannel == nil)
	{
        FindAndSelect();
        if (spchannel == nil) return false;
        if (speechspeed != 0)
            SetSpeechRate (spchannel, speechspeed);
        if (speechpitch != 0)
            SetSpeechPitch(spchannel, speechpitch);
    }
    return true;
}
bool OSXSpeechNarrator::SpeechStop(bool ReleaseInit)
{

	if (spchannel == nil)
		return true;

	// OK-2007-04-16: Fix for bug 3611 and 3591.
	StopSpeech(spchannel);
	do
	{
        // Run an inner main loop until the speech has finished processing
        EventRef t_event;
        OSStatus t_status = ReceiveNextEvent(0, NULL, 1, true, &t_event);
        if (t_status == noErr)
        {
            SendEventToEventTarget(t_event, GetEventDispatcherTarget());
            ReleaseEvent(t_event);
        }
	}
	while(SpeechBusy() > 0);
	
    DisposeSpeechChannel(spchannel);
    spchannel = nil;
    if (speechtext != nil)
    {
        CFRelease(speechtext);
        speechtext = nil;
    }
    
   	return true;
}

void OSXSpeechNarrator::FindAndSelect()
{
    VoiceSpec *FoundVoice = nil;
    VoiceSpec tVoice;
    char cvoice[256];
    short NumVoices;
    if (CountVoices(&NumVoices) == noErr) {
        for (short count = 1;count<=NumVoices;count++)
        {
            if (GetIndVoice(count, &tVoice) == noErr)
            {
                VoiceDescription myInfo;
                if (GetVoiceDescription(&tVoice, &myInfo, sizeof(myInfo)) == noErr) {
                    strncpy(cvoice, (const char*)&myInfo.name[1], myInfo.name[0]);
					// PM-2016-02-15: [[ Bug 16929 ]] Make sure the copied strings are null-terminated
					cvoice[myInfo.name[0]]='\0';
                    if (strcmp(cvoice,speechvoice) == 0)
                    {
                        FoundVoice = &tVoice;
                        break;
                    }
                }
            }
        }
    }
    if (FoundVoice != nil)
        NewSpeechChannel(FoundVoice, &spchannel);
    else
        NewSpeechChannel(nil, &spchannel);
}
