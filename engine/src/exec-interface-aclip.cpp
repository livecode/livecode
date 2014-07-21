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
#include "sysdefs.h"

#include "globals.h"
#include "object.h"
#include "stack.h"
#include "cdata.h"
#include "objptr.h"
#include "field.h"
#include "object.h"
#include "button.h"
#include "card.h"
#include "exec.h"
#include "aclip.h"
#include "player.h"

#include "exec-interface.h"

#ifdef FEATURE_PLATFORM_AUDIO
#include "platform.h"
#elif defined(_WINDOWS_DESKTOP)
// SN-2014-06-26 [[ PlatformPlayer ]]
// These 2 definitions must be accessible from exec-interface-aclip
#include "w32prefix.h"
extern HWAVEOUT hwaveout;  //handle to audio device opened
extern WAVEHDR wh;         //wave header structure
#endif

//////////

static MCExecEnumTypeElementInfo _kMCInterfacePlayDestinationElementInfo[] =
{	
	{ "external", 0, false },
	{ "internal", 1, false },
};

static MCExecEnumTypeInfo _kMCInterfacePlayDestinationTypeInfo =
{
	"Interface.PlayDestination",
	sizeof(_kMCInterfacePlayDestinationElementInfo) / sizeof(MCExecEnumTypeElementInfo),
	_kMCInterfacePlayDestinationElementInfo
};

////////////////////////////////////////////////////////////////////////////////

MCExecEnumTypeInfo *kMCInterfacePlayDestinationTypeInfo = &_kMCInterfacePlayDestinationTypeInfo;

////////////////////////////////////////////////////////////////////////////////

void MCAudioClip::GetPlayProp(MCExecContext& ctxt, integer_t& r_loudness)
{
    // SN-2014-06-25 [[ PlatformAudio ]]
    if (this == MCtemplateaudio)
    {
        extern bool MCSystemGetPlayLoudness(uint2& r_loudness);
#ifdef _MOBILE
        if (MCSystemGetPlayLoudness(loudness))
#else
			if (false)
#endif
				;
			else if (!supported)
				loudness = 0;
			else
			{
#if defined FEATURE_PLATFORM_AUDIO
                double t_volume;
                MCPlatformGetSystemProperty(kMCPlatformSystemPropertyVolume, kMCPlatformPropertyTypeDouble, &t_volume);
                loudness = t_volume * 100.0;
#elif defined _WINDOWS
				if (hwaveout == NULL)
				{
					WAVEFORMATEX pwfx;
					pwfx.wFormatTag = WAVE_FORMAT_PCM;
					pwfx.nChannels = 1;
					pwfx.nSamplesPerSec = 22050;
					pwfx.nAvgBytesPerSec = 22050;
					pwfx.nBlockAlign = 1;
					pwfx.wBitsPerSample = 8;
					pwfx.cbSize = 0;
					if (waveOutOpen(&hwaveout, WAVE_MAPPER, &pwfx, 0, 0, CALLBACK_NULL
					                | WAVE_ALLOWSYNC) == MMSYSERR_NOERROR)
					{
						DWORD v;
						waveOutGetVolume(hwaveout, &v);
						loudness = MCU_min((uint2)((v & 0xFFFF) * 100 / 0xFFFF) + 1, 100);
						waveOutClose(hwaveout);
						hwaveout = NULL;
					}
				}
#elif defined _MACOSX
				long volume;
				GetDefaultOutputVolume(&volume);
				loudness = (HiWord(volume) + LoWord(volume)) * 50 / 255;
#elif defined TARGET_PLATFORM_LINUX
				if ( x11audio != NULL)
					loudness = x11audio -> getloudness();
#endif
			}
    }
}

void MCAudioClip::SetPlayProp(MCExecContext& ctxt, uint2 p_loudness)
{
    // MERG_2014-06-25 [[ PlatformAudio ]]
    if (this == MCtemplateaudio)
    {
        extern bool MCSystemSetPlayLoudness(uint2 loudness);
#ifdef _MOBILE
        if (MCSystemSetPlayLoudness(loudness))
            return;
#endif
        if (MCplayers != NULL)
        {
            MCPlayer *tptr = MCplayers;
            while (tptr != NULL)
            {
                tptr->setvolume(loudness);
                tptr = tptr->getnextplayer();
            }
        }
#if defined FEATURE_PLATFORM_AUDIO
        double t_volume;
        t_volume = loudness / 100.0;
        MCPlatformSetSystemProperty(kMCPlatformSystemPropertyVolume, kMCPlatformPropertyTypeDouble, &t_volume);
#elif defined _WINDOWS
        WORD v = loudness * MAXUINT2 / 100;
        if (hwaveout != NULL)
            waveOutSetVolume(hwaveout, v | (v << 16));
        else
        {
            WAVEFORMATEX pwfx;
            pwfx.wFormatTag = WAVE_FORMAT_PCM;
            pwfx.nChannels = 1;
            pwfx.nSamplesPerSec = 22050;
            pwfx.nAvgBytesPerSec = 22050;
            pwfx.nBlockAlign = 1;
            pwfx.wBitsPerSample = 8;
            pwfx.cbSize = 0;
            if (waveOutOpen(&hwaveout, WAVE_MAPPER, &pwfx, 0, 0,
                            CALLBACK_NULL | WAVE_ALLOWSYNC) == MMSYSERR_NOERROR)
            {
                waveOutSetVolume(hwaveout, v | (v << 16));
                waveOutClose(hwaveout);
                hwaveout = NULL;
            }
        }
        
#elif defined _MACOSX
        long volume = loudness * 255 / 100;
        SetDefaultOutputVolume(volume | volume << 16);
#elif defined TARGET_PLATFORM_LINUX
        if ( x11audio != NULL)
            x11audio -> setloudness ( loudness ) ;
#endif
    }
}

void MCAudioClip::GetPlayDestination(MCExecContext& ctxt, intenum_t& r_dest)
{
	integer_t t_loudness;
	GetPlayProp(ctxt, t_loudness);
	r_dest = (flags & F_EXTERNAL) ? 0 : 1;
}

void MCAudioClip::SetPlayDestination(MCExecContext& ctxt, intenum_t p_dest)
{
	if (p_dest == 0)
		flags |= F_EXTERNAL;
	else
		flags &= ~F_EXTERNAL;

	SetPlayProp(ctxt, loudness);
}

void MCAudioClip::GetPlayLoudness(MCExecContext& ctxt, integer_t& r_value)
{
	GetPlayProp(ctxt, r_value);
}

void MCAudioClip::SetPlayLoudness(MCExecContext& ctxt, integer_t p_value)
{
	loudness = MCU_max(MCU_min(p_value, 100), 0);
	if (loudness == 100)
		flags &= ~F_LOUDNESS;
	else
		flags |= F_LOUDNESS;

	SetPlayProp(ctxt, loudness);
}

void MCAudioClip::GetSize(MCExecContext& ctxt, uinteger_t& r_size)
{
	r_size = size;
}
