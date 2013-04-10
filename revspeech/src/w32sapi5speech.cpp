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

#include <windows.h>
#include <initguid.h>

#include "w32sapi5speech.h"

#define VOICE_TYPE_STRING 64
#define PITCH_XML_STRING 30


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


INarrator::~INarrator(void)
{
	// Nothing required.
}

// Description
//   Initialises instance variables
WindowsSAPI5Narrator::WindowsSAPI5Narrator(void)
{
	strcpy(m_strLastError,"");
	
	bInited = FALSE;
	isspeaking = FALSE;

	m_bUseDefaultPitch = TRUE;
	m_Npitch = 0;
}

// Description
//   Deinitializes instance variables and frees dynamically allocated memory.
WindowsSAPI5Narrator::~WindowsSAPI5Narrator(void)
{
	Finalize();
}

// Returns
//   true if Initialisation sucessful, false otherwise.
// Description
//   Initialises COM and creates an instance of the voice COM class.
//   Registers interest for all the events associated with the voice COM class.
bool WindowsSAPI5Narrator::Initialize(void)
{
	if(IsInited())
		return true;

	Error(TEXT(""));	
	
	//sapi init
	HRESULT hr = S_OK;		

	if( SUCCEEDED( hr ) )
	{
		hr = CoCreateInstance(CLSID_SpVoice, NULL, CLSCTX_ALL, IID_ISpVoice, (void **)&m_cpVoice);
	}
	else
	{
		return false;
	}

    // We're interested in all TTS events
    if( SUCCEEDED( hr ) )
    {
        hr = m_cpVoice->SetInterest( SPFEI_ALL_TTS_EVENTS, SPFEI_ALL_TTS_EVENTS );
    }	
	else
	{
		return false;
	}
	bInited = true;
	return true;
}

// Returns
//   true if finalizing suceeded, false otherwise
// Description
//   Finalises the Narrator class by releasing the voice COM object and
//   removing the notification mechanism setup in Initialize.
bool WindowsSAPI5Narrator::Finalize(void)
{
	if(!IsInited())
		return false;

	// Release voice, if created
	if(m_cpVoice)
	{
		if (m_cpVoice->SetNotifySink(NULL) != S_OK)
			return false;

		m_cpVoice.Release();
	}	
	m_cpVoice = NULL;
	bInited = false;
	return true;
}

// Returns
//   true if starting suceeded, false otherwise.
// Description
//   Starts speaking the string p_string. If the pitch has been set to a non default
//   value using SetPitch then an xml fragment is generated specifying the speach pitch, and
//   containing the text to speak. The string to speak is converted to WCHAR before being rendered.
//   The voice COM object created in Initialize() is used to speak the text. Sets the instance
//   variable isspeaking to true if succesful.
bool WindowsSAPI5Narrator::Start(const char* p_string)
{
	if (!bInited)
		return false;

	HRESULT hr;
	USES_CONVERSION;
	
	WCHAR* szWTextString;

	if(!m_bUseDefaultPitch)
	{
		char *pstrTotal = new char[ strlen(p_string) + PITCH_XML_STRING + 10 ];
		char strXMLtag[PITCH_XML_STRING];

		sprintf( strXMLtag, "<pitch absmiddle = \'%d\'/> ", m_Npitch);
		strcpy( pstrTotal, strXMLtag);
		strcat( pstrTotal, p_string);
		szWTextString = new WCHAR[ strlen(pstrTotal) + 20 ];	
		wcscpy( szWTextString, A2W(pstrTotal));	
	}
	else
	{
		int t_length;
		t_length = strlen(p_string);
		szWTextString = new WCHAR[ t_length + 1 ];

		wcscpy( szWTextString, A2W(p_string));
	}	

	hr = m_cpVoice->Speak(szWTextString, SPF_ASYNC | SPF_IS_XML, NULL );
	
	delete[] szWTextString;

    if( SUCCEEDED( hr ) )
	{
		isspeaking = true;
		return true;
	}
	else
	{
		return false;
	}

}

// Test, speaks the string p_string into the wav file p_file.
bool WindowsSAPI5Narrator::SpeakToFile(const char* p_string, const char* p_file)
{
	if (!bInited)
		return false;

	HRESULT hr;
	USES_CONVERSION;
	
	WCHAR* szWTextString;

	if(!m_bUseDefaultPitch)
	{
		char *pstrTotal = new char[ strlen(p_string) + PITCH_XML_STRING + 10 ];
		char strXMLtag[PITCH_XML_STRING];

		sprintf( strXMLtag, "<pitch absmiddle = \'%d\'/> ", m_Npitch);
		strcpy( pstrTotal, strXMLtag);
		strcat( pstrTotal, p_string);
		szWTextString = new WCHAR[ strlen(pstrTotal) + 20 ];	
		wcscpy( szWTextString, A2W(pstrTotal));	
	}
	else
	{
		int t_length;
		t_length = strlen(p_string);
		szWTextString = new WCHAR[ t_length + 1 ];

		wcscpy( szWTextString, A2W(p_string));
	}	

	bool t_success;
	t_success = true;

	// Set the audio format
	CSpStreamFormat t_audio_format;
	hr = t_audio_format.AssignFormat(SPSF_44kHz16BitStereo);
	if (!SUCCEEDED(hr))
	{
		t_success = false;
	}

	// unicode string containg file path
	WCHAR* t_file;
	t_file = NULL;
	// pointer to the stream object returned
	CComPtr<ISpStream> t_cstream;

	if (t_success)
	{
		// Convert the path to native format and then to unicode
		char *t_native_path;
		t_native_path = path_to_native(p_file);
		if (t_native_path == NULL)
		{
			t_success = false;
		}

		if (t_success)
		{
			t_file = new WCHAR[strlen(t_native_path) + 1];
			wcscpy(t_file, A2W(t_native_path));

			// Bind the output to the stream
			hr = SPBindToFile(t_file, SPFM_CREATE_ALWAYS, &t_cstream, &t_audio_format.FormatId(), t_audio_format.WaveFormatExPtr(), SPFEI_ALL_EVENTS);
			if (!SUCCEEDED(hr))
			{
				t_success = false;
			}
		}
	}

	// Set the voice object's output to the stream
	if (t_success)
	{
		hr = m_cpVoice->SetOutput(t_cstream, TRUE);
		if (!SUCCEEDED(hr))
		{
			t_success = false;
		}
	}

	// Render the text into the file
	if (t_success)
	{
		hr = m_cpVoice->Speak(szWTextString, SPF_IS_XML, NULL );
		if (!SUCCEEDED(hr))
			t_success = false;
	}
	
	// Clean up
	if (t_success)
	{
		delete[] szWTextString;
		delete[] t_file;
		t_cstream->Close();
		m_cpVoice -> SetOutput(NULL, true);
	}

    if(t_success)
	{
		isspeaking = true;
		return true;
	}
	else
	{
		return false;
	}
}

// Returns
//   true if stop speaking suceeded, false otherwise.
// Description
//   Stops any speech that is currently rendering by calling the Speak method 
//   of the voice COM class with NULL as the string to speak. Sets the instance
//   variable isspeaking to false if successful.
bool WindowsSAPI5Narrator::Stop(void)
{
	if (!bInited)
		return false;

	// Stop current rendering with a PURGEBEFORESPEAK...
    HRESULT hr = m_cpVoice->Speak( NULL, SPF_PURGEBEFORESPEAK, 0 );

    if( SUCCEEDED( hr ) )
	{
		isspeaking = false;
		return true;
	}
	else
	{
		return false;
	}
}

// Returns
//   true if speech is currently being rendered, false otherwise.
// Description
//   Uses the GetStatus method of the voice COM object to determine whether
//   speech rendering is in progress. Updates the isspeaking instance variable
//   to reflect the current status of the COM object.
bool WindowsSAPI5Narrator::Busy(void)
{
	if (!bInited)
		return false;

	SPVOICESTATUS spstat;
	HRESULT hr = m_cpVoice->GetStatus(&spstat,NULL);

    if( SUCCEEDED( hr ) )
	{
		isspeaking = (spstat.dwRunningState & SPRS_IS_SPEAKING) != 0;
		
		return isspeaking;
	}
	else
	{
		return false;
	}
}

// Parameters
//   p_volume : The value to set the volume to.
// Returns
//   true if setting the volume suceeded, false otherwise.
// Description
//   Uses the SetVolume method of the voice COM object to attempt to 
//   set the volume to p_volume. 
bool WindowsSAPI5Narrator::SetVolume(int p_volume)
{
	if (!bInited)
		return false;

	HRESULT hr = m_cpVoice->SetVolume(p_volume);
	if ( SUCCEEDED( hr ) )
		return true;
	else
	{
		return false;
	}
}

// Parameters
//   p_volume : The volume is placed into this variable
// Returns
//   true if getting the volume succeeded, false otherwise.
// Description
//   Uses the GetVolume method of the voice COM object to get the volume.
//   Sets the value of p_volume to the volume if successful, otherwise leaves
//   p_volume with the value it originally had.
bool WindowsSAPI5Narrator::GetVolume(int& p_volume)
{
	if (!bInited)
		return false;

	USHORT t_volume;
	HRESULT hr = m_cpVoice->GetVolume(&t_volume);
	if ( SUCCEEDED( hr ) )
	{
		p_volume = (int)t_volume;
		return true;
	}
	else
	{
		return false;
	}
}

// Returns
//   true if listing voices suceeeded, false otherwise.
// Description
//    Iterates through the appropriate available voices and calls p_callback to return each voice.
//    If p_gender is kNarratorGenderMale then only male voices are returned, if p_gender is kNarratorGenderFemale
//    then only female voices are returned, otherwise all voices are returned. Can return false either if unable
//    to get a token enumerator for the voices or if out of memory when listing voices.
bool WindowsSAPI5Narrator::ListVoices(NarratorGender p_gender, NarratorListVoicesCallback p_callback, void* p_context)
{
	if(!bInited)	
		return false;

	ISpObjectToken *pToken = NULL;		// Token interface pointer
	CComPtr<IEnumSpObjectTokens> cpEnum;// Pointer to token enumerator

	ULONG ulIndex = 0;

	CSpDynamicString*  ppcDesciptionString;
	ppcDesciptionString = NULL;

	WCHAR**  ppszTokenIds;
	ppszTokenIds = NULL;

	ULONG ulNumTokens;
	WCHAR *szRequiredAttributes = NULL;

   // Set the required attributes field for the enum if we have special needs
    // based on our LPARAM in
    if (p_gender == kNarratorGenderMale)
    {
        szRequiredAttributes = L"Gender=Male";
    }
    else if (p_gender == kNarratorGenderFemale)
    {
        szRequiredAttributes = L"Gender=Female";
    }
	else if (p_gender == kNarratorGenderNeuter)
	{
		szRequiredAttributes = L"Gender=Neutral";
	}

    // Get a token enumerator for tts voices available
    HRESULT hr = SpEnumTokens(SPCAT_VOICES, szRequiredAttributes, NULL, &cpEnum);

	if (SUCCEEDED(hr))
	{
		hr = cpEnum->GetCount( &ulNumTokens );

		if ( SUCCEEDED( hr ) && 0 != ulNumTokens )
        {
			ppcDesciptionString = new CSpDynamicString [ulNumTokens];
            if ( NULL == ppcDesciptionString )
            {
                hr = E_OUTOFMEMORY;
                return false;
            }

			ppszTokenIds = new WCHAR* [ulNumTokens];
            if ( NULL == ppszTokenIds )
            {
                hr = E_OUTOFMEMORY;
                return false;
            }
            ZeroMemory( ppszTokenIds, ulNumTokens*sizeof( WCHAR* ) );                    
                
			// Get the next token in the enumeration
            // State is maintained in the enumerator
            while (cpEnum->Next(1, &pToken, NULL) == S_OK)
            {
                // Get a string which describes the token, in our case, the voice name
                hr = SpGetDescription( pToken, &ppcDesciptionString[ulIndex] );
                _ASSERTE( SUCCEEDED( hr ) );
                
                // Get the token id, for a low overhead way to retrieve the token later
                // without holding on to the object itself
                hr = pToken->GetId( &ppszTokenIds[ulIndex] );
                _ASSERTE( SUCCEEDED( hr ) );
                
                ulIndex++;
                
                // Release the token itself
                pToken->Release();
                pToken = NULL;
			}
		}
		// if we've failed to properly initialize, then we should completely shut-down
        if ( S_OK != hr )
        {
            if ( pToken )
            {
                pToken->Release();
            }

            ppszTokenIds = NULL;
            ppcDesciptionString = NULL;
            ulNumTokens = 0;
        }

		for ( ulIndex = 0; ulIndex < ulNumTokens; ulIndex++ )
		{
			char* string = ppcDesciptionString[ulIndex].CopyToChar();
			p_callback(p_context, p_gender, (const char *)string);
		}

		ManageCleanup( ppszTokenIds, ppcDesciptionString, ulNumTokens );
		return true;
	}
	else	
	{
		return false;
	}
}

// Parameters
//   p_voice : The string to set the voice to
// Returns
//   true if setting the voice succeeded, false otherwise.
// Description
//   Lists the available voice tokens and iterates through them comparing each to p_voice.
//   If a token is found that matches p_voice then the voice is set to that token. If p_voice
//   doesn't match any of the voices available then returns false, and if setting the voice to
//   the matching token fails then also returns false.
bool WindowsSAPI5Narrator::SetVoice(const char* p_voice)
{
	if(!bInited)
		return false;
	
	CComPtr<IEnumSpObjectTokens> cpEnum;// Pointer to token enumerator	
	ULONG ulIndex = 0, ulCurTokenIndex = 0, ulNumTokens = 0;
	
    // Find out which token corresponds to our voice which is currently in use
    ISpObjectToken *pToken = NULL;		// Token interface pointer

	HRESULT hr = SpEnumTokens(SPCAT_VOICES, NULL, NULL, &cpEnum);

	if (SUCCEEDED(hr) )
	{
		hr = cpEnum->GetCount( &ulNumTokens );
	}
	else
	{
		return false;
	}

	if ( SUCCEEDED(hr) && 0 != ulNumTokens)
	{
		while (cpEnum->Next(1, &pToken, NULL) == S_OK)
        {
			CSpDynamicString  ppcDesciptionString;  
			// Get a string which describes the token, in our case, the voice name
            hr = SpGetDescription( pToken, &ppcDesciptionString);
            _ASSERTE( SUCCEEDED( hr ) );
            
            // Release the token itself
            pToken->Release();
            pToken = NULL;

			char* string = ppcDesciptionString.CopyToChar();
			if(strcmp(string, p_voice) == 0)
			{
				ulCurTokenIndex = ulIndex;
				break;
			}
			ulIndex++;
		}
	}
	else 
		return false;

	if ( ulIndex < ulNumTokens )
	{
		cpEnum->Item( ulCurTokenIndex, &pToken );
		hr = m_cpVoice->SetVoice( pToken );
		if (SUCCEEDED(hr))
		{
			pToken->Release();					
			return true;
		}
		else
		{
			return false;
		}
	}
	else
	{
		return false;		
	}
}

// Parameters
//   p_speed : The value to set the speed to
// Returns
//   true if setting the speed succeeded, false otherwise.
// Description
//   Attempts to set the speed to p_speed using the voice COM object SetRate method.
bool WindowsSAPI5Narrator::SetSpeed(int p_speed)
{
	if (!bInited)
		return false;

	int newint   = (int)(-10 + ((double)((p_speed - 30)/360.0) * 20));

	HRESULT hr = hr = m_cpVoice->SetRate(newint);
	if ( SUCCEEDED( hr ) )
		return true;
	else
	{
		return false;
	}
}

// Parameters
//   p_speed : The speed is placed into this variable
// Returns
//   true if getting the speed suceeded, false otherwise.
// Description
//   Gets the speed using the GetRate method of the voice COM object.
//   If getting the speed suceeeded then the resulting value will be placed into 
//   p_speed, otherwise the value of p_speed will be unchanged.
bool WindowsSAPI5Narrator::GetSpeed(int& p_speed)
{
	if (!bInited)
		return false;

	long lRate;
	HRESULT hr = m_cpVoice->GetRate( &lRate );
    // initialize sliders and edit boxes with default rate
	lRate = 30 + (18 * (lRate - -10));

	if ( SUCCEEDED( hr ) )
	{
		p_speed = (int)lRate;
		return true;
	}
	else
	{
		return false;
	}
}

// Parameters
//   p_pitch : the value to set the pitch to.
// Returns
//   true if setting the pitch suceeded, false otherwise
// Description
//   Translates p_pitch to a value in the correct range, then puts
//   the resulting number into the instance variable m_Npitch. If the pitch
//   is non-default, sets the instance variable m_bUseDefaultPitch to true.
//   The new pitch will not take effect take the next call to Start.
bool WindowsSAPI5Narrator::SetPitch(int p_pitch)
{
	m_Npitch = p_pitch;
		m_Npitch = (int)(-10 + ((double)((p_pitch - 30)/360.0) * 20));
	if(m_Npitch == 0)
		m_bUseDefaultPitch = true;
	else
		m_bUseDefaultPitch = false;

	return true;
}

// Parameters
//  p_pitch : variable to place value of the pitch into
// Returns
//  always returns true
// Description
//  Returns the current pitch by placing it into p_pitch.
bool WindowsSAPI5Narrator::GetPitch(int& p_pitch)
{
	p_pitch = (int)((((double)(m_Npitch + 10) / 20) * 360.0) + 30);

	return true;
}
	
// Helper function, frees tokens allocated by ListVoices.
void WindowsSAPI5Narrator::ManageCleanup( WCHAR** ppszTokenIds, CSpDynamicString*  ppcDesciptionString, ULONG ulNumTokens)
{
    ULONG ulIndex;

    // Free all allocated token ids
    if ( ppszTokenIds )
    {
        for ( ulIndex = 0; ulIndex < ulNumTokens; ulIndex++ )
        {
            if ( NULL != ppszTokenIds[ulIndex] )
            {
                CoTaskMemFree( ppszTokenIds[ulIndex] );
            }
        }
        
        delete [] ppszTokenIds;
    }
    
    if ( ppcDesciptionString )
    {
        delete [] ppcDesciptionString;
    }
}