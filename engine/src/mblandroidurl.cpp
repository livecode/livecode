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

#include "system.h"
#include "mblandroid.h"
#include "mblandroidutil.h"
#include "mblandroidjava.h"

////////////////////////////////////////////////////////////////////////////////

typedef struct _mcurlinfo
{
	uint32_t id;
	char *url;
    uint32_t upload_byte_count;
    
	MCSystemUrlCallback callback;
	void *context;

	struct _mcurlinfo *next;
} MCUrlInfo;

static MCUrlInfo* s_urlinfo_list = nil;

bool getURLInfo(uint32_t p_id, MCUrlInfo *&r_info)
{
	MCUrlInfo *t_info = s_urlinfo_list;
	while (t_info != nil && t_info->id < p_id)
		t_info = t_info->next;

	if (t_info != nil && t_info->id == p_id)
	{
		r_info = t_info;
		return true;
	}

	return false;
}

bool storeURLInfo(MCStringRef p_url, MCSystemUrlCallback p_callback, void *p_context, MCUrlInfo *&r_info)
{
	bool t_success = true;

	MCUrlInfo *t_info = nil;

	t_success = MCMemoryNew(t_info);
	if (t_success)
		t_success = MCStringConvertToCString(p_url, t_info->url);
	if (t_success)
	{
		t_info->callback = p_callback;
		t_info->context = p_context;

		if (s_urlinfo_list == nil)
		{
			s_urlinfo_list = t_info;
			t_info->id = 1;
		}
		else
		{
			uint32_t t_id = s_urlinfo_list->id + 1;
			MCUrlInfo *t_list = s_urlinfo_list;
			while (t_list->next != NULL && t_list->next->id == t_id)
			{
				t_list = t_list->next;
				t_id++;
			}
			t_info->next = t_list->next;
			t_list->next = t_info;
			t_info->id = t_id;
		}
	}

	if (t_success)
		r_info = t_info;
	else
	{
		if (t_info != nil)
			MCCStringFree(t_info->url);
		MCMemoryDelete(t_info);
	}

	return t_success;
}

void deleteUrlInfo(MCUrlInfo *p_info)
{
	if (p_info != nil)
	{
		MCCStringFree(p_info->url);
		MCMemoryDelete(p_info);
	}
}

bool removeURLInfo(MCUrlInfo *p_info)
{
	if (p_info == nil)
		return false;

	if (s_urlinfo_list == p_info)
		s_urlinfo_list = p_info->next;
	else
	{
		MCUrlInfo *t_prev = s_urlinfo_list;
		while (t_prev != nil)
		{
			if (t_prev->next == p_info)
			{
				t_prev->next = p_info->next;
				deleteUrlInfo(p_info);
				return true;
			}
			t_prev = t_prev->next;
		}
	}
	return false;
}

////////////////////////////////////////////////////////////////////////////////

extern MCStringRef MChttpheaders;
extern real8 MCsockettimeout;

bool MCSystemLoadUrl(MCStringRef p_url, MCSystemUrlCallback p_callback, void *p_context)
{
	bool t_success = true;
	
	MCUrlInfo *t_info = nil;
	t_success = storeURLInfo(p_url, p_callback, p_context, t_info);

	if (t_success)
	{
		MCAndroidEngineCall("setURLTimeout", "vi", nil, (int32_t)MCsockettimeout);
		MCAndroidEngineCall("loadURL", "bixx", &t_success, t_info->id, p_url, MChttpheaders);
	}

	return t_success;
}

bool MCSystemPostUrl(MCStringRef p_url, MCDataRef p_data, uint32_t p_length, MCSystemUrlCallback p_callback, void *p_context)
{
	bool t_success = true;

	MCUrlInfo *t_info = nil;
	t_success = storeURLInfo(p_url, p_callback, p_context, t_info);

	if (t_success)
	{
		MCAndroidEngineCall("setURLTimeout", "vi", nil, (int32_t)MCsockettimeout);
		MCAndroidEngineCall("postURL", "bixxd", &t_success, t_info->id, p_url, MChttpheaders, p_data);
	}

	return t_success;
}

bool MCSystemPutUrl(MCStringRef p_url, MCDataRef p_data, uint32_t p_length, MCSystemUrlCallback p_callback, void *p_context)
{
    bool t_success = true;
    
    MCUrlInfo *t_info = nil;
    t_success = storeURLInfo(p_url, p_callback, p_context, t_info);
    
    if (t_success)
    {
        t_info->upload_byte_count = p_length;
        
        MCAndroidEngineCall("setURLTimeout", "vi", nil, (int32_t)MCsockettimeout);
        MCAndroidEngineCall("putURL", "bixxd", &t_success, t_info->id, p_url, MChttpheaders, p_data);
    }
    
    return t_success;
}

// MW-2013-10-02: [[ MobileSSLVerify ]] Enable or disable SSL verification.
void MCSystemSetUrlSSLVerification(bool enabled)
{
	MCAndroidEngineCall("setURLSSLVerification", "vb", nil, enabled);
}

////////////////////////////////////////////////////////////////////////////////

bool MCSystemLaunchUrl(MCStringRef p_url)
{
	MCAndroidEngineCall("launchUrl", "vx", nil, p_url);
	return true;
}

////////////////////////////////////////////////////////////////////////////////

#include <jni.h>

extern "C" JNIEXPORT void JNICALL Java_com_runrev_android_Engine_doUrlDidStart(JNIEnv *env, jobject object, int id) __attribute__((visibility("default")));
extern "C" JNIEXPORT void JNICALL Java_com_runrev_android_Engine_doUrlDidConnect(JNIEnv *env, jobject object, int id, int content_length) __attribute__((visibility("default")));
extern "C" JNIEXPORT void JNICALL Java_com_runrev_android_Engine_doUrlDidSendData(JNIEnv *env, jobject object, int id, int length) __attribute__((visibility("default")));
extern "C" JNIEXPORT void JNICALL Java_com_runrev_android_Engine_doUrlDidReceiveData(JNIEnv *env, jobject object, int id, jbyteArray bytes, int length) __attribute__((visibility("default")));
extern "C" JNIEXPORT void JNICALL Java_com_runrev_android_Engine_doUrlDidFinish(JNIEnv *env, jobject object, int id) __attribute__((visibility("default")));
extern "C" JNIEXPORT void JNICALL Java_com_runrev_android_Engine_doUrlError(JNIEnv *env, jobject object, int id, jstring error_string) __attribute__((visibility("default")));

//public static native void doUrlDidStart(int id);
JNIEXPORT void JNICALL Java_com_runrev_android_Engine_doUrlDidStart(JNIEnv *env, jobject object, int id)
{
	MCUrlInfo *t_info = nil;

	if (getURLInfo(id, t_info))
	{
		t_info->callback(t_info->context, kMCSystemUrlStatusStarted, nil);
	}
}

//public static native void doUrlDidSendData(int id, int bytes_sent);
JNIEXPORT void JNICALL Java_com_runrev_android_Engine_doUrlDidSendData(JNIEnv *env, jobject object, int id, int bytes_sent)
{
	MCUrlInfo *t_info = nil;
    
	if (getURLInfo(id, t_info))
	{
        if (t_info->upload_byte_count == bytes_sent)
            t_info->callback(t_info->context, kMCSystemUrlStatusUploaded, &bytes_sent);
        else
            t_info->callback(t_info->context, kMCSystemUrlStatusUploading, &bytes_sent);
	}
}

//public static native void doUrlDidConnect(int id, int content_length);
JNIEXPORT void JNICALL Java_com_runrev_android_Engine_doUrlDidConnect(JNIEnv *env, jobject object, int id, int content_length)
{
	MCUrlInfo *t_info = nil;

	if (getURLInfo(id, t_info))
	{
		t_info->callback(t_info->context, kMCSystemUrlStatusNegotiated, &content_length);
	}
}

//public static native void doUrlDidReceiveData(int id, byte data[], int length);
JNIEXPORT void JNICALL Java_com_runrev_android_Engine_doUrlDidReceiveData(JNIEnv *env, jobject object, int id, jbyteArray bytes, int length)
{
	MCUrlInfo *t_info = nil;

	if (getURLInfo(id, t_info))
	{
		// get bytes from byte[] object
		jbyte *t_bytes = env->GetByteArrayElements(bytes, nil);

        // AL_2014-07-15: [[ Bug 12478 ]] Pass a DataRef to url callbacks
		MCAutoDataRef t_data;
		MCJavaByteArrayToDataRef(env, bytes, &t_data);
		t_info->callback(t_info->context, kMCSystemUrlStatusLoading, *t_data);
        // PM-2015-02-11: [[ Bug 14515 ]] Unpin the bytes array in the JNI to prevent a crash
        env->ReleaseByteArrayElements(bytes, t_bytes, 0);
	}
}

//public static native void doUrlDidFinish(int id);
JNIEXPORT void JNICALL Java_com_runrev_android_Engine_doUrlDidFinish(JNIEnv *env, jobject object, int id)
{
	MCUrlInfo *t_info = nil;

	if (getURLInfo(id, t_info))
	{
		t_info->callback(t_info->context, kMCSystemUrlStatusFinished, nil);

		removeURLInfo(t_info);
	}
}

//public static native void doUrlError(int id, String error_str);
JNIEXPORT void JNICALL Java_com_runrev_android_Engine_doUrlError(JNIEnv *env, jobject object, int id, jstring error_string)
{
	MCLog("doUrlError(id=%d)", id);
	MCUrlInfo *t_info = nil;

	if (getURLInfo(id, t_info))
	{
        // AL_2014-07-15: [[ Bug 12478 ]] Error is passed to callback as StringRef
		MCAutoStringRef t_error;
        MCJavaStringToStringRef(env, error_string, &t_error);
		t_info->callback(t_info->context, kMCSystemUrlStatusError, *t_error);

		removeURLInfo(t_info);
	}
}

////////////////////////////////////////////////////////////////////////////////
