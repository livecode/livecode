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

#ifndef __REVSPEECH__
#define __REVSPEECH__

enum NarratorProvider
{
	kNarratorProviderDefault,
	kNarratorProviderSAPI4,
	kNarratorProviderSAPI5,
	kNarratorProviderSpeechManager
};

enum NarratorGender
{
	kNarratorGenderAll,
	kNarratorGenderMale,
	kNarratorGenderFemale,
	kNarratorGenderNeuter
};

typedef bool (*NarratorListVoicesCallback)(void* p_context, NarratorGender p_gender, const char* p_voice);

class INarrator
{
public:
	virtual ~INarrator(void) = 0;

	virtual bool Initialize(void) = 0;
	virtual bool Finalize(void) = 0;

	virtual bool Start(const char* p_string, bool p_is_utf8) = 0;

#ifdef FEATURE_REVSPEAKTOFILE	
	virtual bool SpeakToFile(const char* p_string, const char* p_file) = 0;
#endif

	virtual bool Stop(void) = 0;
	virtual bool Busy(void) = 0;

	virtual bool ListVoices(NarratorGender p_gender, NarratorListVoicesCallback p_callback, void* p_context) = 0;
	virtual bool SetVoice(const char* p_voice) = 0;

	virtual bool SetSpeed(int p_speed) = 0;
	virtual bool GetSpeed(int& r_speed) = 0;

	virtual bool SetPitch(int p_pitch) = 0;
	virtual bool GetPitch(int& r_pitch) = 0;

	virtual bool SetVolume(int p_volume) = 0;
	virtual bool GetVolume(int& p_volume) = 0;
};

extern INarrator *InstantiateNarrator(NarratorProvider p_provider);

#endif
