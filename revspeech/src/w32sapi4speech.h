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

#ifndef __W32SAPI4SPEECH__
#define __W32SAPI4SPEECH__

#ifndef __REVSPEECH__
#include "revspeech.h"
#endif

#include <Speech.h>

class WindowsSAPI4Narrator;

class CNotify : public ITTSNotifySink2A
{
public:
	CNotify(WindowsSAPI4Narrator *p_narrator);
	~CNotify(void);

	STDMETHODIMP QueryInterface (REFIID, LPVOID FAR *);
	STDMETHODIMP_(ULONG) AddRef(void);
	STDMETHODIMP_(ULONG) Release(void);
	STDMETHOD (AttribChanged)  (DWORD);
	STDMETHOD (AudioStart)     (QWORD);
	STDMETHOD (AudioStop)      (QWORD);
	STDMETHOD (Visual)         (QWORD, CHAR, CHAR, DWORD, PTTSMOUTH);
	STDMETHOD (Error)		   (LPUNKNOWN p_error);
	STDMETHOD (VisualFuture)   (DWORD p_milliseconds, QWORD p_timestamp, CHAR p_IPA_phoneme, CHAR p_engine_phoneme, DWORD p_hints, PTTSMOUTH p_ttsmouth);
	STDMETHOD (Warning)		   (LPUNKNOWN p_warning);

protected:
	WindowsSAPI4Narrator *m_narrator;
};


class WindowsSAPI4Narrator: public INarrator
{
public:
	WindowsSAPI4Narrator(void);
	~WindowsSAPI4Narrator(void);

	bool Initialize(void);
	bool Finalize(void);

	bool Start(const char *p_string);
	bool SpeakToFile(const char* p_string, const char* p_file);
	bool SpeakToFileOld(const char* p_string, const char* p_file);
	bool Stop(void);
	bool Busy(void);

	bool ListVoices(NarratorGender p_gender, NarratorListVoicesCallback p_callback, void *p_context);
	bool SetVoice(const char* p_voice);

	bool SetSpeed(int p_speed);
	bool GetSpeed(int& p_speed);

	bool SetPitch(int p_pitch);
	bool GetPitch(int& p_pitch);

	bool SetVolume(int p_volume);
	bool GetVolume(int& p_volume);

	bool isspeaking;

protected:
	PITTSCENTRALA m_device_engine;
	PIAUDIOMULTIMEDIADEVICE m_device_interface;
	PITTSATTRIBUTESA m_device_attributes;

	PITTSCENTRALA m_file_engine;
	PIAUDIOFILE m_file_interface;
	PITTSATTRIBUTESA m_file_attributes; 
	
	CNotify *m_device_notifier;
	CNotify *m_file_notifier;
	DWORD m_device_notifier_id;
	DWORD m_file_notifier_id;
	bool inited;
	char ttsvoice[255];
	PITTSCENTRALA FindAndSelect(PTTSMODEINFOA TTSINFinfo);
	bool FindAndSelectFile(PTTSMODEINFOA TTSINFinfo, PIAUDIOFILE &p_audio_file, PITTSCENTRALA &p_central_interface);
	WAVEFORMATEX GetWaveFormat(void);
	bool CopyProsody(PITTSATTRIBUTESA p_source, PITTSATTRIBUTESA p_destination);
	void ReleaseInterface(IUnknown* p_interface);
};

#endif
