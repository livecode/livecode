/* Copyright (C) 2003-2017 LiveCode Ltd.
 
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

#include "platform.h"
#include "platform-internal.h"
#include "mac-extern.h"

////////////////////////////////////////////////////////////////////////////////

MCPlatformSound::Sound(void)
{
}

MCPlatformSound::~Sound(void)
{
}

////////////////////////////////////////////////////////////////////////////////

void MCPlatformSoundCreateWithData(const void *p_data, size_t p_data_size, MCPlatformSoundRef& r_sound)
{
    MCPlatform::SoundRef t_sound = MCMacPlatformCreateSound();
    if (t_sound &&
        !t_sound->CreateWithData(p_data, p_data_size))
    {
        t_sound.reset();
    }
    r_sound = t_sound.unsafeTake();
}

void MCPlatformSoundRetain(MCPlatformSoundRef p_sound)
{
    p_sound->Retain();
}

void MCPlatformSoundRelease(MCPlatformSoundRef p_sound)
{
    p_sound->Release();
}

bool MCPlatformSoundIsPlaying(MCPlatformSoundRef p_sound)
{
    return p_sound->IsPlaying();
}

void MCPlatformSoundPlay(MCPlatformSoundRef p_sound)
{
    p_sound->Play();
}

void MCPlatformSoundPause(MCPlatformSoundRef p_sound)
{
    p_sound->Pause();
}

void MCPlatformSoundResume(MCPlatformSoundRef p_sound)
{
    p_sound->Resume();
}

void MCPlatformSoundStop(MCPlatformSoundRef p_sound)
{
    p_sound->Stop();
}

void MCPlatformSoundSetProperty(MCPlatformSoundRef p_sound, MCPlatformSoundProperty property, MCPlatformPropertyType type, void *value)
{
    p_sound->SetProperty(property, type, value);
}

void MCPlatformSoundGetProperty(MCPlatformSoundRef p_sound, MCPlatformSoundProperty property, MCPlatformPropertyType type, void *value)
{
    p_sound->GetProperty(property, type, value);
}

////////////////////////////////////////////////////////////////////////////////
