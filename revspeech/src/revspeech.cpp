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

#include <cstdlib>
#include <cstring>
#include <stdio.h>

#include <revolution/external.h>
#include <revolution/support.h>

#include "revspeech.h"

///////////////////////////////////////////////////////////////////////////////

#ifdef _WINDOWS
#define strcasecmp _stricmp
#endif

static bool cstring_to_integer(const char *p_string, int& r_value)
{
	char *t_end_ptr;
	r_value = strtol(p_string, &t_end_ptr, 10);
	if (t_end_ptr != p_string + strlen(p_string))
		return false;
	return true;
}

///////////////////////////////////////////////////////////////////////////////

static NarratorProvider s_narrator_provider = kNarratorProviderDefault;
static INarrator *s_narrator = NULL;

///////////////////////////////////////////////////////////////////////////////

static bool NarratorLoad(void)
{
	if (s_narrator != NULL)
		return true;

	s_narrator = InstantiateNarrator(s_narrator_provider);

	if (s_narrator != NULL)
		return s_narrator -> Initialize();

	return false;
}

static bool NarratorUnload(void)
{
	if (s_narrator == NULL)
		return true;

	// OK-2007-04-17: Finalize may return false if the Speech Manager is busy. In this case we don't 
	// delete the narrator, but just report an error to the user to allow them to try again.
	if (s_narrator -> Finalize())
		return false;

	delete s_narrator;
	s_narrator = NULL;

	return true;
}

///////////////////////////////////////////////////////////////////////////////

extern "C" void getXtable();

void revSpeechLoad(char *args[], int nargs, char **retstring,
					Bool *pass, Bool *error)
{
	*pass = False;
	*error = False;
	char *result = NULL;

	if (!NarratorLoad())
		result = strdup("sperr,unable to load speech provider");

	if (result == NULL)
		result = strdup("");

	*retstring = result;
}

void revSpeechUnload(char *args[], int nargs, char **retstring,
					Bool *pass, Bool *error)
{
	*pass = False;
	*error = False;
	
	// OK-2007-04-17: Part of fix for bug 3611. NarratorUnload may fail, in particular if the OS X
	// Speech Manager is still busy speaking, in this case we report an error.
	if (!NarratorUnload())
	{
		*retstring = strdup("sperr,couldn't unload narrator");
	}
	else
	{
		*retstring = strdup("");
	}
}

void revSpeechProvider(char *args[], int nargs, char **retstring, Bool *pass, Bool *error)
{
	char *result = NULL;

	*pass = False;
	*error = False;

	if (nargs == 1)
	{
		if (strcasecmp(args[0], "sapi4") == 0)
			s_narrator_provider = kNarratorProviderSAPI4;
		else if (strcasecmp(args[0], "sapi5") == 0)
			s_narrator_provider = kNarratorProviderSAPI5;
		else if (strcasecmp(args[0], "speechmanager") == 0)
			s_narrator_provider = kNarratorProviderSpeechManager;
		else
			s_narrator_provider = kNarratorProviderDefault;
	}
	else
		*error = True;

	*retstring = result != NULL ? result : strdup("");
}

void revSpeechVoice(char *args[], int nargs, char **retstring, Bool *pass, Bool *error)
{
	char *result = NULL;

	*pass = False;
	*error = False;

	if (nargs == 1)
	{
		NarratorLoad();
		s_narrator -> SetVoice(args[0]);
	}
	else
		*error = True;

	*retstring = result != NULL ? result : strdup("");
}

void revSpeechSpeed(char *args[], int nargs, char **retstring, Bool *pass, Bool *error)
{
	char *result = NULL;

	*pass = False;
	*error = False;

	if (nargs == 1)
	{
		NarratorLoad();
		s_narrator -> SetSpeed(atoi(args[0]));
	}
	else
		*error = True;

	*retstring = result != NULL ? result : strdup("");
}

void revSpeechPitch(char *args[], int nargs, char **retstring, Bool *pass, Bool *error)
{
	char *result = NULL;

	*pass = False;
	*error = False;

	if (nargs == 1)
	{
		NarratorLoad();
		s_narrator -> SetPitch(atoi(args[0]));
	}
	else
		*error = True;

	*retstring = result != NULL ? result : strdup("");
}

void revGetSpeechPitch(char *args[], int nargs, char **retstring, Bool *pass, Bool *error)
{
	*pass = False;
	*error = False;

	bool t_success = true;

	int t_pitch;

	if (nargs == 0)
	{
		NarratorLoad();
		t_success = s_narrator -> GetPitch(t_pitch);
	}

	if (t_success)
	{
		char t_buffer[16];
		sprintf(t_buffer, "%d", t_pitch);
		*retstring = strdup(t_buffer);
	}
}

void revSpeechSpeak(char *args[], int nargs, char **retstring,
	       Bool *pass, Bool *error)
{
	char *result = NULL;

	*pass = False;
	*error = False;

	if (nargs == 1)
	{
		NarratorLoad();

		s_narrator -> Start(args[0], false);
	}
	else
		*error = True;

	*retstring = result != NULL ? result : strdup("");
}

void revSpeechSpeakUTF8(char *args[], int nargs, char **retstring,
                    Bool *pass, Bool *error)
{
	char *result = NULL;
    
	*pass = False;
	*error = False;
    
	if (nargs == 1)
	{
		NarratorLoad();
        
		s_narrator -> Start(args[0], true);
	}
	else
		*error = True;
    
	*retstring = result != NULL ? result : strdup("");
}

void revSpeechStop(char *args[], int nargs, char **retstring,
	       Bool *pass, Bool *error)
{ 	char *result = NULL;

	*pass = False;
	*error = False;

	NarratorLoad();
	s_narrator -> Stop();

	*retstring = result != NULL ? result : strdup("");
}

void revSpeechBusy(char *args[], int nargs, char **retstring,
	       Bool *pass, Bool *error)
{

	char *result = NULL;

	*pass = False;
	*error = False;

	NarratorLoad();

	if (s_narrator -> Busy())
		result = strdup("TRUE");
	else
		result = strdup("FALSE");

	*retstring = result != NULL ? result: (char *)calloc(1,0);
}

static bool revSpeechVoicesCallback(void *p_callback, NarratorGender p_gender, const char *p_voice)
{
	char** t_result_var;
	t_result_var = (char **)p_callback;
	
	*t_result_var = (char *)realloc(*t_result_var, strlen(*t_result_var) + strlen(p_voice) + 2);
	if (**t_result_var != '\0')
		strcat(*t_result_var, "\n");
	strcat(*t_result_var, p_voice);
	
	return true;
}

void revSpeechVoices(char *args[], int nargs, char **retstring,
	       Bool *pass, Bool *error)
{
	char *result = NULL;

	*pass = False;
	*error = False;

	if (nargs < 2)
	{
		NarratorLoad();

		NarratorGender t_gender;
		t_gender = kNarratorGenderAll;

		if (nargs == 1)
		{
			if (strcasecmp(args[0], "male") == 0)
				t_gender = kNarratorGenderMale;
			else if (strcasecmp(args[0], "female") == 0)
				t_gender = kNarratorGenderFemale;
			else if (strcasecmp(args[0], "neuter") == 0)
				t_gender = kNarratorGenderNeuter;
		}

		result = (char *)malloc(1);
		result[0] = '\0';

		s_narrator -> ListVoices(t_gender, revSpeechVoicesCallback, &result);
	}
	else
		*error = True;

	*retstring = result != NULL ? result : (char *)calloc(1,0);
}

void revSpeechSetVolume(char *args[], int nargs, char **retstring,
	       Bool *pass, Bool *error)
{

	bool t_success;
	t_success = true;

	// Check argument count
	if (nargs != 1)
	{
		*retstring = strdup("wrong number of arguments");
		t_success = false;
	}
	
	int t_volume;
	
	// Convert argument to integer, check that it is valid, and if applicable coerce the value into the correct range.
	if (t_success)
	{
		NarratorLoad();
		t_success = cstring_to_integer(args[0], t_volume);
		
		if (!t_success)
		{
			*retstring = strdup("volume must be an integer");
		}
	}
	
	if (t_success)
	{
		if (t_volume < 0)
			t_volume = 0;
			
		if (t_volume > 100)
			t_volume = 100;
	}
	
	// Use the narrator class to set the volume to the specified value
	if (t_success)
	{
		t_success = s_narrator -> SetVolume(t_volume);
		if (!t_success)
		{
			*retstring = strdup("failed to set volume");
		}
	}
	
	if (t_success)
	{
		*retstring =  strdup("");
	}
	
	*error = False;
	*pass = False;
}

void revSpeechGetVolume(char *args[], int nargs, char **retstring,
	       Bool *pass, Bool *error)
{

	bool t_success;
	t_success = true;

	if (nargs != 0)
	{
		*retstring = strdup("wrong number of arguments");
		t_success = false;
	}
	
	int t_volume;
	t_volume = -1;
	
	if (t_success)
	{
		NarratorLoad();
		t_success = s_narrator -> GetVolume(t_volume);
		if (!t_success)
		{
			*retstring = strdup("failed to get volume");
		}
	}
		
	if (t_volume < 0 || t_volume > 100)
	{
		*retstring = strdup("error getting volume");
		t_success = false;
	}
	
	if (t_success)
	{
		char t_buffer[16];
		sprintf(t_buffer, "%d", t_volume);
		*retstring = strdup(t_buffer);
	}
	
	*error = False;
	*pass = False;
}


#ifdef FEATURE_REVSPEAKTOFILE
void revSpeakToFile(char *args[], int nargs, char **retstring,
	       Bool *pass, Bool *error)
{
	*pass = False;
	*error = False;

	if (nargs != 2)
	{
		*error = True;
		return;
	}

	// TODO: Add validity checking for given path.

	bool t_success = true;

	NarratorLoad();
	t_success = s_narrator ->SpeakToFile(args[0], args[1]);
	if (t_success)
	{
		*retstring = strdup("");
	}
	else
	{
		*retstring = strdup("failed");
	}
}
#endif

#define REVSPEECH_VERSIONSTRING "2.9.0"

void REVSpeech_Version(char *args[], int nargs, char **retstring,
		   Bool *pass, Bool *error)
{
	char *result = NULL;
	*error = False;
	*pass = False;
	 result = strdup(REVSPEECH_VERSIONSTRING);
	*retstring = (result != NULL ? result : (char *)calloc(1,1));
}

EXTERNAL_BEGIN_DECLARATIONS("revSpeech")
	EXTERNAL_DECLARE_FUNCTION("revspeech_version", REVSpeech_Version)
	EXTERNAL_DECLARE_COMMAND("revLoadSpeech", revSpeechLoad)
	EXTERNAL_DECLARE_COMMAND("revUnloadSpeech", revSpeechUnload)
	EXTERNAL_DECLARE_COMMAND("revSetSpeechVoice", revSpeechVoice)
	EXTERNAL_DECLARE_COMMAND("revSetSpeechSpeed", revSpeechSpeed)
	EXTERNAL_DECLARE_COMMAND("revSetSpeechPitch", revSpeechPitch)
	EXTERNAL_DECLARE_COMMAND("revSetSpeechVolume", revSpeechSetVolume)
	EXTERNAL_DECLARE_FUNCTION("revGetSpeechVolume", revSpeechGetVolume)
    EXTERNAL_DECLARE_COMMAND("revSpeak", revSpeechSpeak)
    EXTERNAL_DECLARE_COMMAND_UTF8("revSpeak", revSpeechSpeakUTF8)
	EXTERNAL_DECLARE_COMMAND("revStopSpeech", revSpeechStop)
	EXTERNAL_DECLARE_FUNCTION("revIsSpeaking", revSpeechBusy)
	EXTERNAL_DECLARE_FUNCTION("revSpeechVoices", revSpeechVoices)
	EXTERNAL_DECLARE_COMMAND("revSetSpeechProvider", revSpeechProvider)
#ifdef FEATURE_REVSPEAKTOFILE
	EXTERNAL_DECLARE_COMMAND("revSpeakToFile", revSpeakToFile)
#endif
EXTERNAL_END_DECLARATIONS
