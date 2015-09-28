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

#include "core.h"
#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"

#include "execpt.h"
#include "globals.h"
#include "stack.h"
#include "system.h"
#include "player.h"
#include "eventqueue.h"
#include "osspec.h"

#include "mblevent.h"

#include "mblandroid.h"
#include "mblandroidutil.h"

#include <jni.h>

////////////////////////////////////////////////////////////////////////////////

bool path_to_apk_path(const char * p_path, const char *&r_apk_path);

static bool s_video_playing = false;

bool MCSystemPlayVideo(const char *p_file)
{
	if (s_video_playing)
		return false;

	//MCLog("MCSystemplayVideo(\"%s\")", p_file);
	bool t_success;
	const char *t_video_path = nil;

	char *t_resolved_path = nil;

	bool t_is_url = false;
	bool t_is_asset = false;


	if (MCCStringBeginsWith(p_file, "http://") || MCCStringBeginsWith(p_file, "https://"))
	{
		t_is_url = true;
		t_video_path = p_file;
	}
	else
	{
		t_resolved_path = MCS_resolvepath(p_file);
		t_video_path = t_resolved_path;
		t_is_asset = path_to_apk_path(t_resolved_path, t_video_path);
	}

	bool t_looping, t_show_controller;
	// set from showController, looping properties of the templatePlayer
	t_looping = MCtemplateplayer -> getflag(F_LOOPING) == True;
	t_show_controller = MCtemplateplayer -> getflag(F_SHOW_CONTROLLER) == True;

	s_video_playing = true;
	MCAndroidEngineRemoteCall("playVideo", "bsbbbb", &t_success, t_video_path, t_is_asset, t_is_url, t_looping, t_show_controller);

	if (!t_success)
		s_video_playing = false;

	while (s_video_playing)
		if (MCscreen->wait(60.0, True, True))
			break;

	s_video_playing = false;

	MCCStringFree(t_resolved_path);
	return t_success;
}

void MCSystemStopVideo(void)
{
	MCLog("MCSystemStopVideo()", nil);
	MCAndroidEngineRemoteCall("stopVideo", "v", nil);
}

extern "C" JNIEXPORT void JNICALL Java_com_runrev_android_Engine_doMovieStopped(JNIEnv *env, jobject object) __attribute__((visibility("default")));
JNIEXPORT void JNICALL Java_com_runrev_android_Engine_doMovieStopped(JNIEnv *env, jobject object)
{
	MCLog("doMovieStopped", nil);
	s_video_playing=false;
}

extern "C" JNIEXPORT void JNICALL Java_com_runrev_android_Engine_doMovieTouched(JNIEnv *env, jobject object) __attribute__((visibility("default")));
JNIEXPORT void JNICALL Java_com_runrev_android_Engine_doMovieTouched(JNIEnv *env, jobject object)
{
	MCLog("doMovieTouched", nil);
	extern MCExecPoint *MCEPptr;
	MCEventQueuePostCustom(new MCMovieTouchedEvent(MCEPptr -> getobj()));
}

////////////////////////////////////////////////////////////////////////////////
