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


#include "util.h"
#include "date.h"
#include "sellst.h"
#include "stack.h"
#include "aclip.h"
#include "player.h"
#include "card.h"
#include "param.h"
#include "mcerror.h"
#include "osspec.h"

#include "globals.h"
#include "exec.h"

#if defined FEATURE_PLATFORM_AUDIO
#include "platform.h"
static MCPlatformSoundRef s_current_sound = nil;
#elif defined _WINDOWS_DESKTOP
// SN-2014-06-26 [[ PlatformPlayer ]]
// These 2 definitions must be accessible from exec-interface-aclip
HWAVEOUT hwaveout;  //handle to audio device opened
WAVEHDR wh;         //wave header structure
#elif defined _MAC_DESKTOP
#include "osxprefix.h"
static SndChannelPtr soundChannel; //used for playing a sound on MAC
static Handle sound;   //is the handle to the sound resource to be played
static Boolean playing;  //flag inicates if a sound is playing
#endif


MCStack *MCAudioClip::mstack;
Boolean MCAudioClip::supported = False;
uint4 MCAudioClip::curindex;
Boolean MCAudioClip::looping;
real8 MCAudioClip::endtime;

static const int2 ulaw_table[256] = {
                                  -32124, -31100, -30076, -29052, -28028, -27004, -25980, -24956,
                                  -23932, -22908, -21884, -20860, -19836, -18812, -17788, -16764,
                                  -15996, -15484, -14972, -14460, -13948, -13436, -12924, -12412,
                                  -11900, -11388, -10876, -10364,  -9852,  -9340,  -8828,  -8316,
                                  -7932,  -7676,  -7420,  -7164,  -6908,  -6652,  -6396,  -6140,
                                  -5884,  -5628,  -5372,  -5116,  -4860,  -4604,  -4348,  -4092,
                                  -3900,  -3772,  -3644,  -3516,  -3388,  -3260,  -3132,  -3004,
                                  -2876,  -2748,  -2620,  -2492,  -2364,  -2236,  -2108,  -1980,
                                  -1884,  -1820,  -1756,  -1692,  -1628,  -1564,  -1500,  -1436,
                                  -1372,  -1308,  -1244,  -1180,  -1116,  -1052,   -988,   -924,
                                  -876,   -844,   -812,   -780,   -748,   -716,   -684,   -652,
                                  -620,   -588,   -556,   -524,   -492,   -460,   -428,   -396,
                                  -372,   -356,   -340,   -324,   -308,   -292,   -276,   -260,
                                  -244,   -228,   -212,   -196,   -180,   -164,   -148,   -132,
                                  -120,   -112,   -104,    -96,    -88,    -80,    -72,    -64,
                                  -56,    -48,    -40,    -32,    -24,    -16,     -8,      0,
                                  32124,  31100,  30076,  29052,  28028,  27004,  25980,  24956,
                                  23932,  22908,  21884,  20860,  19836,  18812,  17788,  16764,
                                  15996,  15484,  14972,  14460,  13948,  13436,  12924,  12412,
                                  11900,  11388,  10876,  10364,   9852,   9340,   8828,   8316,
                                  7932,   7676,   7420,   7164,   6908,   6652,   6396,   6140,
                                  5884,   5628,   5372,   5116,   4860,   4604,   4348,   4092,
                                  3900,   3772,   3644,   3516,   3388,   3260,   3132,   3004,
                                  2876,   2748,   2620,   2492,   2364,   2236,   2108,   1980,
                                  1884,   1820,   1756,   1692,   1628,   1564,   1500,   1436,
                                  1372,   1308,   1244,   1180,   1116,   1052,    988,    924,
                                  876,    844,    812,    780,    748,    716,    684,    652,
                                  620,    588,    556,    524,    492,    460,    428,    396,
                                  372,    356,    340,    324,    308,    292,    276,    260,
                                  244,    228,    212,    196,    180,    164,    148,    132,
                                  120,    112,    104,     96,     88,     80,     72,     64,
                                  56,     48,     40,     32,     24,     16,      8,      0 };

////////////////////////////////////////////////////////////////////////////////

MCPropertyInfo MCAudioClip::kProperties[] =
{
	DEFINE_RO_OBJ_PROPERTY(P_SIZE, UInt16, MCAudioClip, Size)
	DEFINE_RW_OBJ_ENUM_PROPERTY(P_PLAY_DESTINATION, InterfacePlayDestination, MCAudioClip, PlayDestination)
	DEFINE_RW_OBJ_PROPERTY(P_PLAY_LOUDNESS, Int16, MCAudioClip, PlayLoudness)
};

MCObjectPropertyTable MCAudioClip::kPropertyTable =
{
	&MCObject::kPropertyTable,
	sizeof(kProperties) / sizeof(kProperties[0]),
	&kProperties[0],
};

////////////////////////////////////////////////////////////////////////////////

MCAudioClip::MCAudioClip()
{
	size = 0;
	samples = osamples = NULL;
	format = AF_SLINEAR;
	nchannels = 1;
	swidth = 1;
	rate = 8000;
	disposable = False;
	loudness = 100;
#ifdef TARGET_PLATFORM_LINUX
	x11audio = new (nothrow) X11Audio ;
#endif
}

MCAudioClip::MCAudioClip(const MCAudioClip &aref) : MCObject(aref)
{
	size = aref.size;
	samples = osamples = NULL;
	if (size != 0)
	{
		samples = new (nothrow) int1[size];
		memcpy(samples, aref.samples, size);
	}
	format = aref.format;
	nchannels = aref.nchannels;
	swidth = aref.swidth;
	rate = aref.rate;
	disposable = False;
	loudness = aref.loudness;
#ifdef TARGET_PLATFORM_LINUX
	x11audio = new (nothrow) X11Audio ;
#endif
}

MCAudioClip::~MCAudioClip()
{
	if (MCacptr == this)
	{
		stop(True);
		MCacptr = nil;
	}
	delete[] samples; /* Allocated with new[] */
	delete osamples;
#ifdef TARGET_PLATFORM_LINUX
	if ( x11audio != NULL )
		delete x11audio ;
#endif
}

Chunk_term MCAudioClip::gettype() const
{
	return CT_AUDIO_CLIP;
}

const char *MCAudioClip::gettypestring()
{
	return MCaudiostring;
}

bool MCAudioClip::visit_self(MCObjectVisitor* p_visitor)
{
    return p_visitor -> OnAudioClip(this);
}

void MCAudioClip::timer(MCNameRef mptr, MCParameter *params)
{
    // PM-2014-11-11: [[ Bug 13950 ]] Make sure looping audioClip can be stopped
#ifndef FEATURE_PLATFORM_AUDIO
    if (play())
    {
        MCscreen->addtimer(this, MCM_internal, looping ? LOOP_RATE: PLAY_RATE);
#else
    if (MCPlatformSoundIsPlaying(s_current_sound))
    {
        // Do nothing
#endif
    }
	else
	{
		MCacptr = nil;
		if (mstack != NULL)
		{
			if (mstack->getopened())
			{
				MCCard *card = mstack->getcurcard();
				if (card)
					card->message_with_valueref_args(MCM_play_stopped, getname());
			}
			mstack = NULL;
		}
		if (isdisposable())
			delete this;
	}
}

Boolean MCAudioClip::del(bool p_check_flag)
{
    if (!isdeletable(p_check_flag))
        return False;
    
    getstack()->removeaclip(this);
    
    // MCObject now does things on del(), so we must make sure we finish by
    // calling its implementation.
	return MCObject::del(p_check_flag);
}

void MCAudioClip::paste(void)
{
	MCdefaultstackptr -> appendaclip(this);
}

void MCAudioClip::init()
{
#if defined FEATURE_PLATFORM_AUDIO
    supported = True;
#elif defined _WINDOWS
	supported = True;
#elif defined _MACOSX
	supported = True;
#elif defined(_LINUX)
	supported = true ;
#endif
	if (supported)
	{
        // AL-2014-08-12: [[ Bug 13161 ]] Set the initial loudness of templateAudioClip to the system loudness
        loudness = MCS_getplayloudness();
	}
}

void MCAudioClip::convert_mulawtolin16()
{
	int2 *newsamples = new (nothrow) int2[size];
	uint1 *sptr = (uint1 *)samples;
	int2 *dptr = newsamples;
	uint4 count = size;
	while(count--)
		*dptr++ = ulaw_table[*sptr++];
	oformat = format;
	osize = size;
	osamples = samples;
	onchannels = nchannels;
	oswidth = swidth;
	orate = rate;
	samples = (int1 *)newsamples;
	swidth = 2;
	size <<= 1;
	format = AF_SLINEAR;
}

void MCAudioClip::convert_mulawtoulin8()
{
	int1 *newsamples = new (nothrow) int1[size];
	uint1 *sptr = (uint1 *)samples;
	int1 *dptr = newsamples;
	uint4 count = size;
	while(count--)
		*dptr++ = (ulaw_table[*sptr++] >> 8) + 128;
	oformat = format;
	osize = size;
	osamples = samples;
	onchannels = nchannels;
	oswidth = swidth;
	orate = rate;
	samples = newsamples;
	format = AF_ULINEAR;
}

void MCAudioClip::convert_slin8toslin16()
{
	oformat = format;
	osize = size;
	osamples = samples;
	onchannels = nchannels;
	oswidth = swidth;
	orate = rate;
	uint4 count = size;
	size <<= 1;
	swidth = 2;
	int1 *newsamples = new (nothrow) int1[size];
	int1 *sptr = samples;
	int2 *dptr = (int2 *)newsamples;
	while(count--)
		*dptr++ = *sptr++ << 8;
	samples = newsamples;
	format = AF_SLINEAR;
}

void MCAudioClip::convert_slintoulin()
{
	if (swidth == 1)
	{
		uint1 *dptr = (uint1 *)samples;
		uint4 i = size;
		while (i--)
			*dptr++ += 128;
	}
	else
	{
		uint1 *dptr = (uint1 *)samples;
		uint4 i = size >> 1;
		while (i--)
		{
			uint1 tmp = *dptr;
			*dptr = *(dptr + 1);
			dptr++;
			*dptr++ = tmp;
		}
	}
	format = AF_ULINEAR;
}

void MCAudioClip::convert_ulintoslin()
{
	if (swidth == 1)
	{
		uint1 *dptr = (uint1 *)samples;
		uint4 i = size;
		while (i--)
			*dptr++ -= 128;
	}
	else
		convert_slintoulin();
	format = AF_SLINEAR;
}

Boolean MCAudioClip::isdisposable()
{
	return disposable;
}

Boolean MCAudioClip::issupported()
{
	return supported;
}

void MCAudioClip::setdisposable()
{
	disposable = True;
}

void MCAudioClip::setlooping(Boolean loop)
{
	looping = loop;
}

Boolean MCAudioClip::import(MCStringRef fname, IO_handle stream)
{
	size = (uint4)MCS_fsize(stream);
	if (size == 0)
		return False;
	samples = new (nothrow) int1[size];
	if (IO_read(samples, size, stream) != IO_NORMAL)
		return False;
	if (strnequal((char*)samples, ".snd", 4))
	{
		uint4 *header = (uint4 *)samples;
		uint4 start = swap_uint4(&header[1]);
		size -= start;
		rate = swap_uint4(&header[4]);
		nchannels = swap_uint4(&header[5]);
		switch (swap_uint4(&header[3]))
		{
		case 0:
		case 1:
			swidth = 1;
			format = AF_MULAW;
			break;
		case 2:
			swidth = 1;
			break;
		case 3:
			swidth = 2;
			break;
		case 4:
		case 5:
			swidth = 3;
			break;
		}
		memmove(samples, &samples[start], size);
	}
	else
	{
		if (strnequal((char*)samples, "FORM", 4)
		        || strnequal((char*)&samples[8], "AIFF", 4))
		{
			int1 *sptr = &samples[12];
			while (True)
			{
				uint4 length;
				memcpy((char *)&length, sptr + 4, 4);
				swap_uint4(&length);
				if (strnequal((char*)sptr, "COMM", 4))
				{
					memcpy((char *)&nchannels, sptr + 8, 2);
					memcpy((char *)&swidth, sptr + 14, 2);
					swap_uint2(&nchannels);
					swap_uint2(&swidth);
					swidth >>= 3;
					rate = (int4)MCU_stoIEEE((char *)(sptr + 16));
				}
				else
					if (strnequal((char*)sptr, "SSND", 4))
					{
						size = length - 8;
						memmove(samples, sptr + 12, size);
						break;
					}
				sptr += length + (length & 1) + 8;
			}
		}
		else
			if (strnequal((char*)samples, "RIFF", 4)
			        && strnequal((char*)&samples[8], "WAVE", 4))
			{
				uint4 fsize = size;
				uint4 skip = 0;
				MCswapbytes = !MCswapbytes;
				while (!strnequal((char*)&samples[12 + skip], "fmt ", 4))
				{
					memcpy((char *)&size, &samples[16 + skip], 4);
					swap_uint4(&size);
					skip += size + 8;
					if (skip + 12 > fsize)
					{
						MCswapbytes = !MCswapbytes;
						return False;
					}
				}
				memcpy((char *)&nchannels, &samples[22 + skip], 2);
				swap_uint2(&nchannels);
				memcpy((char *)&swidth, &samples[34 + skip], 2);
				swap_uint2(&swidth);
				swidth >>= 3;
				memcpy((char *)&rate, &samples[24 + skip], 2);
				swap_uint2(&rate);
				memcpy((char *)&size, &samples[16 + skip], 4);
				swap_uint4(&size);
				skip += size + 8;
				while (!strnequal((char*)&samples[12 + skip], "data", 4))
				{
					memcpy((char *)&size, &samples[16 + skip], 4);
					swap_uint4(&size);
					skip += size + 8;
					if (skip + 12 > fsize)
					{
						MCswapbytes = !MCswapbytes;
						return False;
					}
				}
				memcpy((char *)&size, &samples[16 + skip], 4);
				swap_uint4(&size);
				format = AF_ULINEAR;
				memmove(samples, &samples[20 + skip], size);
				MCswapbytes = !MCswapbytes;
			}
			else
			{
				rate = 11000;
			}
	}
    uindex_t t_sep;
    MCStringRef t_fname;
    if (MCStringLastIndexOfChar(fname, PATH_SEPARATOR, UINDEX_MAX, kMCCompareExact, t_sep))
        /* UNCHECKED */ MCStringCopySubstring(fname, MCRangeMakeMinMax(t_sep + 1, MCStringGetLength(fname)), t_fname);
    else
        t_fname = MCValueRetain(fname);
    
    MCNewAutoNameRef t_name;
    if (!MCNameCreateAndRelease(t_fname, &t_name))
        return False;
    setname(*t_name);
    return True;
}

#if defined FEATURE_PLATFORM_AUDIO

struct au_file_header_t
{
    uint32_t magic;
    uint32_t offset;
    uint32_t size;
    uint32_t encoding;
    uint32_t sample_rate;
    uint32_t channels;
};

void MCAudioClip::convert_tocontainer(void*& r_data, size_t& r_data_size)
{
    au_file_header_t t_header;
    t_header . magic = 0x2e736e64;
    t_header . offset = sizeof(au_file_header_t);
    t_header . size = size;
    if (format == AF_MULAW)
        t_header . encoding = 1;
    else if (format == AF_SLINEAR)
        t_header . encoding = (swidth == 1 ? 2 : (swidth == 2 ? 3 : 4));
    else if (format == AF_ULINEAR)
    {
        convert_ulintoslin();
        t_header . encoding = (swidth == 1 ? 2 : (swidth == 2 ? 3 : 4));
    }
    t_header . sample_rate = rate;
    t_header . channels = nchannels;
    
    swap_uint4(&t_header . magic);
    swap_uint4(&t_header . offset);
    swap_uint4(&t_header . size);
    swap_uint4(&t_header . encoding);
    swap_uint4(&t_header . sample_rate);
    swap_uint4(&t_header . channels);
    
    r_data = malloc(sizeof(au_file_header_t) + size);
    r_data_size = sizeof(au_file_header_t) + size;
    
    memcpy(r_data, &t_header, sizeof(au_file_header_t));
    memcpy(((au_file_header_t *)r_data) + 1, samples, size);
}

Boolean MCAudioClip::open_audio(void)
{
    if (s_current_sound != nil)
        return True;
    
    void *t_data;
    size_t t_data_size;
    convert_tocontainer(t_data, t_data_size);
    
    MCPlatformSoundCreateWithData(t_data, t_data_size, s_current_sound);
    
    free(t_data);
    
    if (s_current_sound == nil)
        return False;
    
    double t_volume;
    t_volume = loudness / 100.0;
    MCPlatformSoundSetProperty(s_current_sound, kMCPlatformSoundPropertyVolume, kMCPlatformPropertyTypeDouble, &t_volume);
    
    bool t_looping;
    t_looping = looping == True;
    MCPlatformSoundSetProperty(s_current_sound, kMCPlatformSoundPropertyLooping, kMCPlatformPropertyTypeBool, &t_looping);
    
    MCPlatformSoundPlay(s_current_sound);
    
    return True;
}
#elif defined _WINDOWS
Boolean MCAudioClip::open_audio()
{
	if (hwaveout == NULL)
	{
		WAVEFORMATEX pwfx;
		if (format == AF_MULAW)
			convert_mulawtoulin8();
		else
			if (format == AF_SLINEAR)
				convert_slintoulin();
		pwfx.wFormatTag = WAVE_FORMAT_PCM;
		pwfx.nChannels = nchannels;
		pwfx.nSamplesPerSec = rate;
		pwfx.nAvgBytesPerSec = rate * nchannels * swidth;
		pwfx.nBlockAlign = swidth * nchannels;
		pwfx.wBitsPerSample = swidth << 3;
		pwfx.cbSize = 0;
		if (waveOutOpen(&hwaveout, WAVE_MAPPER, &pwfx,
		                0, 0, CALLBACK_NULL | WAVE_ALLOWSYNC) != MMSYSERR_NOERROR)
			return False;

		WORD v = MCtemplateaudio->loudness * loudness * 0xFFFF / 10000;
		waveOutSetVolume(hwaveout, v | (v << 16));

		wh.lpData = (char *) samples;          // address of the waveform buffer
		wh.dwBufferLength = size;    // length, in bytes, of the buffer
		wh.dwBytesRecorded = 0;          // see below
		wh.dwUser = 0;
		wh.dwFlags = 0;
		wh.dwLoops = looping ? MAXUINT2 : 1;
		wh.lpNext = 0;                   // reserved; must be 0
		wh.reserved = 0;                 // reserved; must be zero
		if (waveOutPrepareHeader(hwaveout, &wh,
		                         sizeof(WAVEHDR)) != MMSYSERR_NOERROR)
			return False;
		if (looping)
			wh.dwFlags |= WHDR_BEGINLOOP | WHDR_ENDLOOP;
		if (waveOutWrite(hwaveout, &wh, sizeof(WAVEHDR)) != MMSYSERR_NOERROR)
			return False;
		opened++;
	}
	return True;
}
#elif defined _MAC_DESKTOP
Boolean MCAudioClip::open_audio() //plays a sound immediately
{
	if (sound != NULL)  //if sound is already open and constructed
		return True;

	soundChannel = NULL;
	if (SndNewChannel(&soundChannel, 0, 0, NULL) != noErr || soundChannel == NULL)
		return False;
	if (loudness != 100)
	{
		SndCommand vCmd;
		vCmd.cmd = volumeCmd;
		vCmd.param1 = 0;
		vCmd.param2 = loudness * 255 / 100;
		vCmd.param2 |= vCmd.param2 << 16;
		SndDoImmediate(soundChannel, &vCmd);
	}

	//check & convert sound format
	if (format == AF_MULAW)
		convert_mulawtoulin8();
	else
		if (format == AF_SLINEAR && swidth == 1)
			convert_slintoulin();
		else
			if (format == AF_ULINEAR && swidth == 2)
				convert_ulintoslin();
	sound = NewHandle(100L + size);
	if (sound == NULL)
	{
		SndDisposeChannel(soundChannel, True);
		soundChannel = NULL;
		return False;
	}
	short headerLen;
	
	// MW-2006-05-03: [[ Bug 3504 ]] - Ensure the sound data is the right way round	
	SetupSndHeader((SndListResource **)sound, nchannels, FixRatio(rate, 1),
	               swidth << 3, swidth == 2 ? k16BitBigEndianFormat : 'NONE', kMiddleC, size, &headerLen);
	HLock(sound);
	memcpy(*sound + headerLen, samples, size);
	if (SndPlay(soundChannel, (SndListResource **)sound, True) != noErr)
	{
		HUnlock(sound);
		DisposeHandle(sound);
		sound = NULL;
		SndDisposeChannel(soundChannel, True);
		soundChannel = NULL;
		return False;
	}
	else
		return True;
}
#elif defined TARGET_PLATFORM_LINUX
// TS-2007-11-20 : Stopping LINUX from playing any sound - for 2.9.0-DP-2
// TS-2007-12-04 : Adding in support for ESD
Boolean MCAudioClip::open_audio()
{
	if (format == AF_MULAW)
		convert_mulawtoulin8();
	else
		if (format == AF_SLINEAR && swidth == 1)
			convert_slintoulin();
		else
			if (format == AF_ULINEAR && swidth == 2)
				convert_ulintoslin();

	
	if ( x11audio != NULL )	
		return x11audio -> init(NULL, nchannels, swidth);
	return false ;
}
#elif defined _SERVER || defined(_MOBILE) || defined(__EMSCRIPTEN__)
Boolean MCAudioClip::open_audio()
{
	return False;
}
#else
#error "MCAudioClip::open_audio() not defined for this platform"
#endif

Boolean MCAudioClip::play()
{
#ifndef _MOBILE
	if (!open_audio())
	{
		real8 delay =  MCS_time() + (real8)size / (real8)(rate*nchannels*swidth);
		MCParameter *newparam = new (nothrow) MCParameter;
		newparam->setvalueref_argument(getname());
		MCscreen->addmessage(MCdefaultstackptr->getcurcard(), MCM_play_stopped, delay, newparam);
		return False;
	}
	else
		if (mstack == NULL)
			mstack = MCdefaultstackptr;
#endif
    
#if defined FEATURE_PLATFORM_AUDIO
    // MW-2014-07-25: [[ Bug 12946 ]] If the sound is no longer playing the 'stop'
    //   (i.e. send a playStopped message).
    if (!MCPlatformSoundIsPlaying(s_current_sound))
    {
        stop(False);
        return False;
    }
    
    return True;
#elif defined _WINDOWS
	if (wh.dwFlags & WHDR_DONE)
	{
		stop(False);//check to see if it is done, call stop();
		return False;
	}
	return True;
#elif defined _MAC_DESKTOP
	SCStatus cs;       //channel status record
	OSErr err = noErr;
	// the sizeof(SCStatus) gives 27 bytes, but the actual record size is 24
	// if you use sizeof() as 2nd parameter, you get a -50(a param is incorrect) error
	if ((err = SndChannelStatus(soundChannel, 24/*sizeof(SCStatus)*/, &cs)) != noErr)
	{
		HUnlock(sound);
		DisposeHandle(sound);
		sound = NULL;
		return False;
	}
	if (!cs.scChannelBusy)
		if (looping)
		{
			if (SndPlay(soundChannel, (SndListResource**)sound, True) != noErr)
			{
				HUnlock(sound);
				DisposeHandle(sound);
				sound = NULL;
				SndDisposeChannel(soundChannel, True);
				soundChannel = NULL;
				return False;
			}
			return True;
		}
		else
		{
			stop(False);
			return False;
		}
	return True;
#elif defined(TARGET_PLATFORM_LINUX)
	if (looping || curindex < size)
	{
		while (True)
		{
			int4 count = 0;

			int4 nsamples = size - curindex;
			
			if ( MCuseESD)
			{
				if ( x11audio != NULL)
					count = x11audio -> play(&samples[curindex], nsamples, rate);
				if ( count == nsamples )
				{
					stop(false);
					return false ;
				}
			}
			
			if (count <= 0)
			{
				return True;
			}

			if (curindex == 0 && !looping)
				endtime = MCS_time() + (real8)size
				          / (real8)(rate * nchannels * swidth);
			curindex += count;
			if (curindex >= size)
				if (looping)
					curindex = 0;
				else
					break;
			else
				break;
		}
	}
	else
	{
		if (MCS_time() > endtime)
		{
			stop(False);
			return False;
		}
	}

	return True;
#elif defined(_SERVER) || defined(_MOBILE) || defined(__EMSCRIPTEN__)
	return True;
#else
#error "MCAudioClip::play() not supported for this platform"
	return True;
#endif
}

void MCAudioClip::stop(Boolean abort)
{
	MCscreen->cancelmessageobject(this, NULL);
   
#if defined FEATURE_PLATFORM_AUDIO
    if (s_current_sound != nil)
    {
        // MW-2014-07-25: [[ Bug 12946 ]] Make sure we actually stop the sound.
        MCPlatformSoundStop(s_current_sound);
        MCPlatformSoundRelease(s_current_sound);
        s_current_sound = nil;
    }
#elif defined _WINDOWS
	if (hwaveout != NULL)
	{
		waveOutReset(hwaveout);
		waveOutUnprepareHeader(hwaveout, &wh, sizeof(WAVEHDR));
		waveOutClose(hwaveout);
		hwaveout = NULL;
	}
#elif defined _MAC_DESKTOP  //minshe
	//MAC stuff here.... Send a quiet command to stop a sound that is currenty playing
	if (sound != NULL)
	{
		SndCommand scmd;
		scmd.cmd = quietCmd;
		scmd.param1 = scmd.param2 = 0;
		SndDoImmediate(soundChannel, &scmd);
		HUnlock(sound);
		DisposeHandle(sound);
		sound = NULL;
		SndDisposeChannel(soundChannel, True);
		soundChannel = NULL;
	}
#elif defined(_LINUX)
#endif
}

bool MCAudioClip::isPlaying()
{
#if defined FEATURE_PLATFORM_AUDIO
    return MCPlatformSoundIsPlaying(s_current_sound);
#endif
    return true;
}

///////////////////////////////////////////////////////////////////////////////
//
//  SAVING AND LOADING
//

IO_stat MCAudioClip::extendedsave(MCObjectOutputStream& p_stream, uint4 p_part, uint32_t p_version)
{
	return defaultextendedsave(p_stream, p_part, p_version);
}

IO_stat MCAudioClip::extendedload(MCObjectInputStream& p_stream, uint32_t p_version, uint4 p_length)
{
	return defaultextendedload(p_stream, p_version, p_length);
}

IO_stat MCAudioClip::save(IO_handle stream, uint4 p_part, bool p_force_ext, uint32_t p_version)
{
	IO_stat stat;

	if ((stat = IO_write_uint1(OT_AUDIO_CLIP, stream)) != IO_NORMAL)
		return stat;
	if ((stat = MCObject::save(stream, p_part, false, p_version)) != IO_NORMAL)
		return stat;
	if (osamples != NULL)
	{
		size = osize;
		delete samples;
		samples = osamples;
		osamples = NULL;
		format = oformat;
		nchannels = onchannels;
		swidth = oswidth;
		rate = orate;
	}
	if ((stat = IO_write_uint4(size, stream)) != IO_NORMAL)
		return stat;
	if ((stat = IO_write(samples, sizeof(int1), size, stream)) != IO_NORMAL)
		return stat;
	if ((stat = IO_write_uint2(format, stream)) != IO_NORMAL)
		return stat;
	if ((stat = IO_write_uint2(nchannels, stream)) != IO_NORMAL)
		return stat;
	if ((stat = IO_write_uint2(swidth, stream)) != IO_NORMAL)
		return stat;
	if ((stat = IO_write_uint2(rate, stream)) != IO_NORMAL)
		return stat;
	if (flags & F_LOUDNESS)
		if ((stat = IO_write_uint2(loudness, stream)) != IO_NORMAL)
			return stat;
	return savepropsets(stream, p_version);
}

IO_stat MCAudioClip::load(IO_handle stream, uint32_t version)
{
	IO_stat stat;

	if ((stat = MCObject::load(stream, version)) != IO_NORMAL)
		return checkloadstat(stat);
	if ((stat = IO_read_uint4(&size, stream)) != IO_NORMAL)
		return checkloadstat(stat);
	if (size != 0)
	{
		samples = new (nothrow) int1[size];
		if ((stat = IO_read(samples, size, stream)) != IO_NORMAL)
			return checkloadstat(stat);
	}
	if ((stat = IO_read_uint2(&format, stream)) != IO_NORMAL)
		return checkloadstat(stat);
	if ((stat = IO_read_uint2(&nchannels, stream)) != IO_NORMAL)
		return checkloadstat(stat);
	if ((stat = IO_read_uint2(&swidth, stream)) != IO_NORMAL)
		return checkloadstat(stat);
	if ((stat = IO_read_uint2(&rate, stream)) != IO_NORMAL)
		return checkloadstat(stat);
	if (flags & F_LOUDNESS)
		if ((stat = IO_read_uint2(&loudness, stream)) != IO_NORMAL)
            return checkloadstat(stat);
	return loadpropsets(stream, version);
}

///////////////////////////////////////////////////////////////////////////////

uint2 MCS_getplayloudness()
{
    uint2 t_loudness = 0;
#if defined FEATURE_PLATFORM_AUDIO
    double t_volume;
    MCPlatformGetSystemProperty(kMCPlatformSystemPropertyVolume, kMCPlatformPropertyTypeDouble, &t_volume);
    // AL-2014-09-10: [[ Bug 13393 ]] Add 0.5 so that loudness is rounded to nearest integer
    t_loudness = 100.0 * t_volume + 0.5;
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
            t_loudness = MCU_min((uint2)((v & 0xFFFF) * 100 / 0xFFFF) + 1, 100);
            waveOutClose(hwaveout);
            hwaveout = NULL;
        }
    }
#elif defined _MAC_DESKTOP
    long volume;
    GetDefaultOutputVolume(&volume);
    t_loudness = (HiWord(volume) + LoWord(volume)) * 50 / 255;
#elif defined TARGET_PLATFORM_LINUX
    X11Audio *t_x11audio = nil;
    t_x11audio = new (nothrow) X11Audio ;
    if ( t_x11audio != nil)
    {
        t_loudness = t_x11audio -> getloudness() ;
        delete t_x11audio;
    }
#endif
    return t_loudness;
}

void MCS_setplayloudness(uint2 p_loudness)
{
#if defined FEATURE_PLATFORM_AUDIO
    double t_volume;
    t_volume = p_loudness / 100.0;
    MCPlatformSetSystemProperty(kMCPlatformSystemPropertyVolume, kMCPlatformPropertyTypeDouble, &t_volume);
#elif defined _WINDOWS
    WORD v = p_loudness * MAXUINT2 / 100;
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
#elif defined _MAC_DESKTOP
    long volume = p_loudness * 255 / 100;
    SetDefaultOutputVolume(volume | volume << 16);
#elif defined TARGET_PLATFORM_LINUX
    X11Audio *t_x11audio = nil;
    t_x11audio = new (nothrow) X11Audio ;
    if (t_x11audio != nil)
    {
        t_x11audio -> setloudness(p_loudness);
        delete t_x11audio;
    }
#endif
}
