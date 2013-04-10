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

#ifndef __W32SAPI5SPEECH__
#define __W32SAPI5SPEECH__

#ifndef __REVSPEECH__
#include "revspeech.h"
#endif

#include <atlbase.h>
#include <sapi.h>
#include <sphelper.h>
#include <mlang.h>
#include <stdio.h>

class WindowsSAPI5Narrator: public INarrator
{
public:
	bool isspeaking;

	WindowsSAPI5Narrator(void);
	~WindowsSAPI5Narrator(void);

	bool Initialize(void);
	bool Finalize(void);
	bool IsInited() {return bInited;}

	bool Start(const char* p_string);
	bool SpeakToFile(const char* p_string, const char* p_file);
	bool Stop(void);
	bool Busy(void);

	bool SetVolume(int p_volume);
	bool GetVolume(int& p_volume);

	bool ListVoices(NarratorGender p_gender, NarratorListVoicesCallback p_callback, void *p_context);
	bool SetVoice(const char* p_voice);

	bool SetSpeed(int p_speed);
	bool GetSpeed(int& p_speed);

	bool SetPitch(int p_pitch);
	bool GetPitch(int& p_pitch);

private:
	bool bInited;
	// Last Error Message
	char m_strLastError[256];

	// Internal Error Message
	void Error( TCHAR* pText ){wcscpy((WCHAR *)m_strLastError, pText); };
	void Error( TCHAR* pText, HRESULT hr){ sprintf(m_strLastError,"%s Error Code:0x%x", pText, hr); };

	// Pointer to our tts voice	
	CComPtr<ISpVoice> m_cpVoice;		
	void ManageCleanup( WCHAR** ppszTokenIds, CSpDynamicString*  ppcDesciptionString, ULONG ulNumTokens);

	bool m_bUseDefaultPitch;
	short m_Npitch;
};

#endif
