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

#include <windows.h>
#include <initguid.h>
#include <tchar.h>

#include "w32sapi4speech.h"

static char *path_from_native(const char *p_path)
{
	char *t_path;
	t_path = strdup(p_path);

	char *dptr;
	dptr = t_path;

	if (!*dptr)
		return dptr;

	do {
		if (*dptr == '\\')
			*dptr = '/';
	} while (*++dptr);

	return t_path;
}

static char *path_to_native(const char *p_path)
{
	char *t_path;
	t_path = strdup(p_path);

	char *dptr;
	dptr = t_path;

	if (!*dptr)
		return dptr;

	do {
		if (*dptr == '/')
			*dptr = '\\';
	} while (*++dptr);

	return t_path;
}

enum AudioInterfaceType
{
	kAudioInterfaceDevice,
	kAudioInterfaceFile
};

CNotify::CNotify(WindowsSAPI4Narrator *p_narrator)
{
	m_narrator = p_narrator;
}

CNotify::~CNotify(void)
{
	return;
}

STDMETHODIMP CNotify::QueryInterface(REFIID riid, LPVOID *ppv)
{
	*ppv = NULL;

	/* always return our IUnknown for IID_IUnknown */
	if (IsEqualIID (riid, IID_IUnknown) || IsEqualIID(riid,IID_ITTSNotifySinkA))
	{
		*ppv = (LPVOID) this;
		return S_OK;
	}
	// otherwise, cant find
	return ResultFromScode (E_NOINTERFACE);
}

STDMETHODIMP_ (ULONG)CNotify::AddRef (void)
{
	return 1;
}

STDMETHODIMP_(ULONG)CNotify::Release (void)
{
	return 1;
}


STDMETHODIMP CNotify::AttribChanged(DWORD dwAttribID)
{
   return NOERROR;
}

STDMETHODIMP CNotify::AudioStart(QWORD qTimeStamp)
{
  m_narrator -> isspeaking = true;
  return NOERROR;
}

STDMETHODIMP CNotify::AudioStop(QWORD qTimeStamp)
{
   m_narrator -> isspeaking = false;
   return NOERROR;
}


STDMETHODIMP CNotify::Visual(QWORD qTimeStamp, CHAR cIPAPhoneme,
				CHAR cEnginePhoneme, DWORD dwHints, PTTSMOUTH pTTSMouth)
{
	return NOERROR;
}

STDMETHODIMP CNotify::Error(LPUNKNOWN p_error)
{
	HRESULT t_result;
	PISPCHERRORA t_error_interface;
	t_result = p_error ->QueryInterface(IID_ISpchErrorA, (void **)&t_error_interface);
	if (t_result != NOERROR)
	{
		// failed to get error interface, nothing more to do
		return NOERROR;
	}

	CHAR *t_message;
	t_message = NULL;

	DWORD t_message_size;
	t_message_size = 0;

	DWORD t_needed;
	t_needed = 0;

	t_error_interface -> ErrorMessageGet(t_message, t_message_size, &t_needed);
	return NOERROR;
}

STDMETHODIMP CNotify::VisualFuture(DWORD p_milliseconds, QWORD p_timestamp, CHAR p_IPA_phoneme, CHAR p_engine_phoneme, DWORD p_hints, PTTSMOUTH p_ttsmouth)
{
	return NOERROR;

}

STDMETHODIMP CNotify::Warning(LPUNKNOWN p_warning)
{
	return NOERROR;
}

WindowsSAPI4Narrator::WindowsSAPI4Narrator(void)
{
	isspeaking = false;
	m_device_engine = NULL;
	m_device_attributes = NULL;
	strcpy(ttsvoice,"");
	m_device_notifier_id = 0;
	m_device_notifier = NULL;
	inited = false;
}

WindowsSAPI4Narrator::~WindowsSAPI4Narrator(void)
{
	Finalize();
}


bool WindowsSAPI4Narrator::Initialize(void)
{
	Finalize();

	TTSMODEINFOA t_device_info;

	memset (&t_device_info, 0, sizeof(t_device_info)); 
	if (ttsvoice[0] != '\0') 
		strcpy(t_device_info.szModeName, ttsvoice);

	m_device_engine = FindAndSelect(&t_device_info);
	if (m_device_engine == NULL) 
		return false;

	m_device_engine->QueryInterface(IID_ITTSAttributesA, (LPVOID *)&m_device_attributes);

	m_device_notifier = new CNotify(this);
	if (m_device_notifier)
	{
		HRESULT hRes = m_device_engine->Register(m_device_notifier, IID_ITTSNotifySink2A, &m_device_notifier_id);
		if(FAILED(hRes))
			m_device_notifier_id = 0;
	}

	inited = true;
	return true;
}

bool WindowsSAPI4Narrator::Finalize(void)
{
	if (!inited) 
		return false;

	isspeaking = false;
	Stop();
	if (m_device_notifier_id)
		m_device_engine->UnRegister(m_device_notifier_id);
	if (m_device_notifier){
		delete m_device_notifier;
		m_device_notifier = NULL;
	}
	m_device_engine -> Release();
	m_device_attributes -> Release();
	m_device_engine = NULL;
	m_device_attributes = NULL;

	inited = false;
	return true;
}


bool WindowsSAPI4Narrator::Start(const char *p_string)
{
	if (!inited)
		Initialize();

	if (!inited)
		return false;

	SDATA SDATvar1;
	SDATvar1.pData = (void *)p_string;
	SDATvar1.dwSize = strlen(p_string)+1;
	m_device_engine->TextData(CHARSET_TEXT,0, SDATvar1, NULL, IID_ITTSBufNotifySink);
	return true;
}

bool WindowsSAPI4Narrator::Stop(void)
{
	if (!inited)
		return false;

	if (m_device_engine->AudioReset() != NOERROR)
		return false;

	isspeaking = false;
	return true;
}

bool WindowsSAPI4Narrator::Busy(void)
{
	return isspeaking;
}


bool WindowsSAPI4Narrator::ListVoices(NarratorGender p_gender, NarratorListVoicesCallback p_callback, void *p_context)
{
	PITTSENUMA   TTSENvar1;
	HRESULT HRSLresult = CoCreateInstance (CLSID_TTSEnumerator, NULL, CLSCTX_ALL, IID_ITTSEnumA, (void**)&TTSENvar1);
	if (HRSLresult) 
		return false;

	TTSMODEINFOA     TTSINFvar1;
	int Ncount = 0;
	while (!TTSENvar1->Next(1, &TTSINFvar1, NULL))
	{
		if ((p_gender == kNarratorGenderMale) && TTSINFvar1.wGender != GENDER_MALE ) 
			continue;
		else if ((p_gender == kNarratorGenderFemale) && TTSINFvar1.wGender != GENDER_FEMALE ) 
			continue;
		else if ((p_gender == kNarratorGenderNeuter) && TTSINFvar1.wGender != GENDER_NEUTRAL )
			continue;
		Ncount++;
	}
	if (Ncount < 1) 
		return true;

	TTSENvar1->Reset();


	int Ncount2 = 1;
	while (!TTSENvar1->Next(1, &TTSINFvar1, NULL))
	{
		if ((p_gender == kNarratorGenderMale) && TTSINFvar1.wGender != GENDER_MALE ) 
			continue;
		else if ((p_gender == kNarratorGenderFemale) && TTSINFvar1.wGender != GENDER_FEMALE ) 
			continue;
		
		p_callback(p_context, p_gender, TTSINFvar1.szModeName);

		Ncount2++;
	}

	TTSENvar1->Release();

	return true;
}

bool WindowsSAPI4Narrator::SetVoice(const char *p_voice)
{
	strcpy(ttsvoice, p_voice);
	if (inited) Initialize();
	return true;
}

bool WindowsSAPI4Narrator::SetSpeed(int p_speed)
{
	if (!inited)
		return false;

	if (m_device_attributes -> SpeedSet((WORD)p_speed) != NOERROR)
		return false;

	return true;
}

bool WindowsSAPI4Narrator::GetSpeed(int& p_speed)
{
	if (!inited)
		return false;

	DWORD t_speed;
	if (m_device_attributes -> SpeedGet(&t_speed) != NOERROR)
		return false;
	
	p_speed = (int)t_speed;
	return true;
}

bool WindowsSAPI4Narrator::SetPitch(int p_pitch)
{
	if (!inited)
		return false;

	if (m_device_attributes -> PitchSet((WORD)p_pitch) != NOERROR)
		return false;

	return true;
}

bool WindowsSAPI4Narrator::GetPitch(int& p_pitch)
{
	if (!inited)
		return false;

	WORD t_pitch;
	if (m_device_attributes -> PitchGet(&t_pitch) != NOERROR)
		return false;

	p_pitch = (int)t_pitch;
	return true;
}

bool WindowsSAPI4Narrator::SetVolume(int p_volume)
{
	if (!inited)
		return false;

	DWORD t_volume;
	t_volume = ((p_volume * 65535) / 100);
	t_volume = t_volume | (t_volume << 16);

	if (m_device_attributes -> VolumeSet(t_volume) != NOERROR)
		return false;

	return true;
}

bool WindowsSAPI4Narrator::GetVolume(int& p_volume)
{
	if (!inited)
		return false;

	DWORD t_volume;
	if (m_device_attributes -> VolumeGet(&t_volume) != NOERROR)
		return false;

	int t_left_volume;
	t_left_volume = t_volume & 0xFFFF;

	int t_right_volume;
	t_right_volume = (t_volume >> 16) & 0xFFFF;

	p_volume = ((((t_left_volume + t_right_volume) / 2 + 654) * 100) / 65535);
	return true;
}


bool WindowsSAPI4Narrator::FindAndSelectFile(TTSMODEINFOA *p_mode_info, PIAUDIOFILE &p_audio_file, PITTSCENTRALA &p_central_interface)
{
	HRESULT t_result;
	TTSMODEINFOA t_found_mode;
	PITTSFINDA t_find_interface;
	PITTSCENTRALA t_central_interface;
	PIAUDIOFILE t_audio_file;

	t_result = CoCreateInstance(CLSID_TTSEnumerator, NULL, CLSCTX_ALL, IID_ITTSFindA, (void**)&t_find_interface);
	if (FAILED(t_result))
	{
		return false;
	}

	t_result = t_find_interface -> Find(p_mode_info, NULL, &t_found_mode);
	if (FAILED(t_result))
	{
		t_find_interface -> Release();
		return false;
	}

	t_result = CoCreateInstance(CLSID_AudioDestFile, NULL, CLSCTX_ALL, IID_IAudioFile, (void **)&t_audio_file);
	if (FAILED(t_result))
	{
		t_find_interface -> Release();
		return false;
	}

	t_result = t_find_interface -> Select(t_found_mode . gModeID, &t_central_interface, t_audio_file);
	if (FAILED(t_result))
	{
		t_find_interface -> Release();
		return false;
	}

	t_find_interface -> Release();
	
	p_audio_file = t_audio_file;
	p_central_interface = t_central_interface;
	return true;
}

class SpeakToFileNotifier2 : public ITTSNotifySinkA
{
public: 
	SpeakToFileNotifier2(void);
	~SpeakToFileNotifier2(void);

	   // IUnknown members
   STDMETHOD (QueryInterface) (THIS_ REFIID riid, LPVOID FAR* ppv);
   STDMETHOD_(ULONG,AddRef)   (THIS);
   STDMETHOD_(ULONG,Release)  (THIS);

   // ITTSNotifySinkA members
   STDMETHOD (AttribChanged)  (DWORD p_attribute);
   STDMETHOD (AudioStart)     (QWORD p_timestamp);
   STDMETHOD (AudioStop)      (QWORD p_timestamp);
   STDMETHOD (Visual)         (QWORD p_timestamp, CHAR p_IPA_phenome, CHAR p_engine_phenome, DWORD p_sentence_type, PTTSMOUTH p_mouth);

   void Wait(void);

private:
	HANDLE m_event;
};

SpeakToFileNotifier2::SpeakToFileNotifier2(void)
{
	m_event = CreateEvent(NULL, FALSE, FALSE, NULL);
}

SpeakToFileNotifier2::~SpeakToFileNotifier2(void)
{
	CloseHandle(m_event);
}

STDMETHODIMP SpeakToFileNotifier2::QueryInterface(REFIID riid, LPVOID *ppv)
{
	*ppv = NULL;

	/* always return our IUnknown for IID_IUnknown */
	if (IsEqualIID (riid, IID_IUnknown) || IsEqualIID(riid,IID_ITTSNotifySinkA))
	{														
		*ppv = (LPVOID) this;
		return S_OK;
	}
	// otherwise, cant find
	return ResultFromScode (E_NOINTERFACE);
}

STDMETHODIMP_ (ULONG)SpeakToFileNotifier2::AddRef(void)
{
	return 1;
}

STDMETHODIMP_(ULONG)SpeakToFileNotifier2::Release(void)
{
	return 1;
}

HRESULT SpeakToFileNotifier2::AttribChanged(DWORD p_attribute)
{
	return NOERROR;
}

HRESULT SpeakToFileNotifier2::AudioStart(QWORD p_timestamp)
{
	return NOERROR;
}

HRESULT SpeakToFileNotifier2::AudioStop(QWORD p_timestamp)
{
	return NOERROR;
}

HRESULT SpeakToFileNotifier2::Visual(QWORD p_timestamp, CHAR p_IPA_phenome, CHAR p_engine_phenome, DWORD p_sentence_type, PTTSMOUTH p_mouth)
{
	return NOERROR;
}

void SpeakToFileNotifier2::Wait(void)
{
	WaitForSingleObject(m_event, 2000);
}


class SpeakToFileNotifier: public ITTSBufNotifySinkA
{
public:
	SpeakToFileNotifier(void);
	~SpeakToFileNotifier(void);

	// IUnknown
	STDMETHODIMP QueryInterface (REFIID, LPVOID FAR *);
	STDMETHODIMP_(ULONG) AddRef(void);
	STDMETHODIMP_(ULONG) Release(void);

	// ITTSBufNotifySink
	STDMETHOD(TextDataDone) (QWORD p_timestamp, DWORD p_flags);
	STDMETHOD(TextDataStarted) (QWORD p_timestamp);
	STDMETHOD(WordPosition) (QWORD p_timestamp, DWORD p_byte_offset);
	STDMETHOD(BookMark) (QWORD p_timestamp, DWORD p_mark_number);

	void Wait(void);

private:
	HANDLE m_event;
};


SpeakToFileNotifier::SpeakToFileNotifier(void)
{
	m_event = CreateEvent(NULL, FALSE, FALSE, NULL);
}

SpeakToFileNotifier::~SpeakToFileNotifier(void)
{
	CloseHandle(m_event);
}

HRESULT SpeakToFileNotifier::TextDataDone(QWORD p_timestamp, DWORD p_flags)
{
	SetEvent(m_event);
	return NOERROR;
}

HRESULT SpeakToFileNotifier::TextDataStarted(QWORD p_timestamp)
{
	return NOERROR;
}

HRESULT SpeakToFileNotifier::WordPosition(QWORD p_timestamp, DWORD p_byte_offset)
{
	return NOERROR;
}

HRESULT SpeakToFileNotifier::BookMark(QWORD p_timestamp, DWORD p_mark_number)
{
	return NOERROR;
}


void SpeakToFileNotifier::Wait(void)
{
	WaitForSingleObject(m_event, INFINITE);
}

STDMETHODIMP SpeakToFileNotifier::QueryInterface(REFIID riid, LPVOID *ppv)
{
	*ppv = NULL;

	/* always return our IUnknown for IID_IUnknown */
	if (IsEqualIID (riid, IID_IUnknown) || IsEqualIID(riid,IID_ITTSBufNotifySinkA))
	{														
		*ppv = (LPVOID) this;
		return S_OK;
	}
	// otherwise, cant find
	return ResultFromScode (E_NOINTERFACE);
}

STDMETHODIMP_ (ULONG)SpeakToFileNotifier::AddRef (void)
{
	return 1;
}

STDMETHODIMP_(ULONG)SpeakToFileNotifier::Release (void)
{
	return 1;
}

bool WindowsSAPI4Narrator::SpeakToFile(const char *p_string, const char *p_file)
{
	if (!inited)
		return false;

	bool t_success;
	t_success = true;

	TTSMODEINFOA t_mode_info;
	memset(&t_mode_info, 0, sizeof(t_mode_info)); 
	if (ttsvoice[0] != '\0') 
		strcpy(t_mode_info . szModeName, ttsvoice);

	PITTSCENTRALA t_central_interface;
	PIAUDIOFILE t_audio_file;

	t_success = FindAndSelectFile(&t_mode_info, t_audio_file, t_central_interface);

	// Register a notification object to receive events from the text to speech engine.
	DWORD t_notifier_id;
	SpeakToFileNotifier2 *t_notifier;
	if (t_success)
	{
		t_notifier = new SpeakToFileNotifier2();
		if (t_notifier)
		{
			HRESULT t_result = t_central_interface -> Register(t_notifier, IID_ITTSNotifySinkA, &t_notifier_id);
			if(FAILED(t_result))
				t_notifier_id = 0;
		}
	}

	WCHAR* t_file;
	t_file = NULL;
	if (t_success)
	{	
		char *t_native_path;
		t_native_path = path_to_native(p_file);
		if (t_native_path == NULL)
		{
			t_success = false;
		}

		if (t_success)
		{
			t_file = new WCHAR[strlen(t_native_path) + 1];
			if (!MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, t_native_path, -1, t_file, strlen(t_native_path) + 1))
				t_success = false;
		}
	}

	HRESULT t_result;
	if (t_success)
	{
		t_result = t_audio_file -> Set(t_file, 1);
		if (FAILED(t_result))
			t_success = false;
	}
	
	SDATA t_speech_data;
	if (t_success)
	{
		t_speech_data . pData = (void *)p_file;
		t_speech_data . dwSize = strlen(p_file) + 1;
		t_result = t_central_interface -> TextData(CHARSET_TEXT, TTSDATAFLAG_TAGGED, t_speech_data, NULL, IID_ITTSNotifySinkA);
		if (FAILED(t_result))
			t_success = false;

	}
	if (t_success)
	{
			t_notifier -> Wait();
			t_audio_file -> Flush();
			ReleaseInterface(t_audio_file);
			ReleaseInterface(t_central_interface);
	}	
	return true;
}

bool WindowsSAPI4Narrator::SpeakToFileOld(const char *p_string, const char *p_file)
{
	if (!inited)
		return false;

	bool t_success;
	t_success = true;

	HRESULT t_result;

	// Find a text to speech mode that supports our current voice.
	PITTSFINDA t_find_interface;
	if (t_success)
	{
		t_result = CoCreateInstance(CLSID_TTSEnumerator, NULL, CLSCTX_ALL, IID_ITTSFindA, (void**)&t_find_interface);
		if (FAILED(t_result))
			t_success = false;
	}

	PTTSMODEINFOA t_mode_info_ptr;
	t_mode_info_ptr = NULL;
	TTSMODEINFOA t_mode_info;
	TTSMODEINFOA t_found_mode_info;
	if (t_success)
	{
		memset(&t_mode_info, 0, sizeof(t_mode_info)); 
		if (ttsvoice[0] != '\0') 
			strcpy(t_mode_info.szModeName, ttsvoice);

		t_mode_info_ptr = &t_mode_info;

		t_result = t_find_interface -> Find(t_mode_info_ptr, NULL, &t_found_mode_info);
		if (t_result != NOERROR)
		{
			ReleaseInterface(t_find_interface);
			t_success = false;
		}
	}

	// Create an audio file destination object.
	PIAUDIOFILE t_audio_file;
	if (t_success)
	{
		t_result = CoCreateInstance(CLSID_AudioDestFile, NULL, CLSCTX_ALL, IID_IAudioFile, (void**)&t_audio_file);
		if (t_result != NOERROR)
		{
			ReleaseInterface(t_find_interface);
			t_success = false;
		}
	}
	
	// Convert the path to native format, then convert it from ascii to unicode. Set the audio file object to the output file path.
	if (t_success)
	{
		WCHAR* t_file;
		t_file = NULL;

		char *t_native_path;
		t_native_path = path_to_native(p_file);
		if (t_native_path == NULL)
		{
			t_success = false;
		}

		if (t_success)
		{
			t_file = new WCHAR[strlen(t_native_path) + 1];
			if (!MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, t_native_path, -1, t_file, strlen(t_native_path) + 1))
				t_success = false;
		}

		if (t_success)
		{
			if (t_audio_file -> Set(t_file, NULL) != NOERROR)
			{
				ReleaseInterface(t_find_interface);
				t_success = false;
			}
		}
	}

	// Select the audio file destination interface. This creates a text to speech engine, the interface
	// to which is placed into the t_central_interface variable.
	PITTSCENTRALA  t_central_interface;
	t_central_interface = NULL;
	if (t_success)
	{
		t_result = t_find_interface -> Select(t_found_mode_info.gModeID, &t_central_interface, t_audio_file);
		if (t_result != NOERROR) 
		{
			ReleaseInterface(t_find_interface);
			t_success = false;
		}
	}
	
	// Query the text to speech engine to obtain an interface to the engine attributes.
	PITTSATTRIBUTESA t_attributes;
	t_attributes = NULL;
	if (t_success)
	{
		ReleaseInterface(t_find_interface);

		t_result = t_central_interface->QueryInterface(IID_ITTSAttributesA, (LPVOID *)&t_attributes);
		if (t_result != NOERROR)
		{
			t_success = false;
		}
	}

	// Register a notification object to receive events from the text to speech engine.
	DWORD t_notifier_id;
	CNotify *t_notifier;
	if (t_success)
	{
		t_notifier = new CNotify(this);
		if (t_notifier)
		{
			HRESULT t_result = t_central_interface -> Register(t_notifier, IID_ITTSNotifySinkA, &t_notifier_id);
			if(FAILED(t_result))
				t_notifier_id = 0;
		}
	}

	// Copy the prosody attributes (pitch, speed, volume) from the main revSpeech attributes interface
	// to the newly created text to speech engine.
	if (t_success)
	{
		t_success = CopyProsody(m_device_attributes, t_attributes);
	}

	// Set the wave format of the output file by obtaining an IAudio interface from the audio file destination.
	SDATA t_wave_format_buffer;
	WAVEFORMATEX t_wave_format;
	if (t_success)
	{
		t_wave_format_buffer . pData = &t_wave_format;
		t_wave_format_buffer . dwSize = sizeof(t_wave_format);
		t_wave_format = GetWaveFormat();
	}

	PIAUDIO t_audio_interface;
	if (t_success)
	{
	   t_audio_interface = NULL;
	   if (t_audio_file -> QueryInterface(IID_IAudio, (LPVOID *)&t_audio_interface) != NOERROR)
	   {
		   t_success = false;
	   }
	}

	// Render the text into the audio file.
	if (t_success)
	{
		SDATA t_string;
		t_string.pData = (void *)p_string;
		t_string.dwSize = strlen(p_string);
		t_result = t_central_interface->TextData(CHARSET_TEXT, 0, t_string, t_notifier, IID_ITTSNotifySink2A);
		if (t_result != NOERROR)
			t_success = false;
	}

	ReleaseInterface(t_attributes);

	t_central_interface -> UnRegister(t_notifier_id);
	ReleaseInterface(t_central_interface);

	ReleaseInterface(t_audio_interface);

	t_audio_file -> Set(NULL, 1);
	ReleaseInterface(t_audio_file);
	
	return t_success;
}


// Returns a pointer to an SDATA structure which contains a pointer to
// a WAVEFORMATEX structure describing the wave format to use.
WAVEFORMATEX WindowsSAPI4Narrator::GetWaveFormat(void)
{
	WAVEFORMATEX t_wave_format;
	memset(&t_wave_format, 0, sizeof(t_wave_format));

	t_wave_format . wFormatTag = WAVE_FORMAT_PCM;
	t_wave_format . nChannels = 1;
	t_wave_format . nSamplesPerSec = 22050;
	t_wave_format . nAvgBytesPerSec = 44100;
	t_wave_format . nBlockAlign = 2;
	t_wave_format . wBitsPerSample = 16;
	
	return t_wave_format;
}

// Copies the speed, pitch and volume information from p_source to p_destination
bool WindowsSAPI4Narrator::CopyProsody(PITTSATTRIBUTESA p_source, PITTSATTRIBUTESA p_destination)
{
	bool t_success = true;

	{
		DWORD t_speed;
		if (t_success)
		{	
			if (p_source -> SpeedGet(&t_speed) != NOERROR)
				t_success = false;
		}

		if (t_success)
		{
			if (p_destination -> SpeedSet(t_speed) != NOERROR)
				t_success = false;
		}
	}

	{
		WORD t_pitch;
		if (t_success)
		{
			if (p_source -> PitchGet(&t_pitch) != NOERROR)
				t_success = false;
		}

		if (t_success)
		{
			if (p_destination -> PitchSet(t_pitch) != NOERROR)
				return false;
		}
	}

	{
		DWORD t_volume;
		if (t_success)
		{
			if (p_source -> VolumeGet(&t_volume) != NOERROR)
				t_success = false;
		}

		if (t_success)
		{
			if (p_destination -> VolumeSet(t_volume) != NOERROR)
				t_success = false;
		}
	}
	return t_success;
}

void WindowsSAPI4Narrator::ReleaseInterface(IUnknown* p_interface)
{

	if (p_interface == NULL)
		return;

	p_interface -> Release();
	return;
}


// If p_path is null, the default audio device is used. Otherwise, p_path is assumed to be
// the file to write the speech to.
PITTSCENTRALA WindowsSAPI4Narrator::FindAndSelect(PTTSMODEINFOA TTSINFinfo)
{
	HRESULT        HRSLresult;
	TTSMODEINFOA        TTSINFresult;        // final result
	PITTSFINDA      TTSFNDfind;             // find interface
	PIAUDIOMULTIMEDIADEVICE    t_audio_device;      // multimedia device interface for audio-dest
	PIAUDIOFILE	t_audio_file;						// audio file where the speech will be written to if required.

	PITTSCENTRALA  TTSCTRvar1;           // central interface

	HRSLresult = CoCreateInstance(CLSID_TTSEnumerator, NULL, CLSCTX_ALL, IID_ITTSFindA, (void**)&TTSFNDfind);
	if (FAILED(HRSLresult))
		return NULL;
	
	HRSLresult = TTSFNDfind->Find(TTSINFinfo, NULL, &TTSINFresult);
	if (HRSLresult)
	{
		TTSFNDfind->Release();
		return NULL;     // error
	}

	
	HRSLresult = CoCreateInstance(CLSID_MMAudioDest, NULL, CLSCTX_ALL, IID_IAudioMultiMediaDevice, (void**)&t_audio_device);
	
	HRSLresult = CoCreateInstance(CLSID_AudioDestFile, NULL, CLSCTX_ALL, IID_IAudioFile, (void**)&t_audio_file);

	if (HRSLresult)
	{
		TTSFNDfind->Release();
		return NULL;     // error
	}

	t_audio_device -> DeviceNumSet (WAVE_MAPPER);
	HRSLresult = TTSFNDfind -> Select(TTSINFresult.gModeID, &TTSCTRvar1, (LPUNKNOWN) t_audio_device);
	//}
	
	if (HRSLresult) {
		TTSFNDfind -> Release();
		return NULL;
	};
	TTSFNDfind -> Release();
	
	m_device_interface = t_audio_device;

	return TTSCTRvar1;
}
