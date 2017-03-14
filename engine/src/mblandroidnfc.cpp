/* Copyright (C) 2017 LiveCode Ltd.
 
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

#include "globals.h"

#include "eventqueue.h"

#include "mblandroidutil.h"
#include "mblandroidjava.h"

////////////////////////////////////////////////////////////////////////////////

bool MCSystemNFCIsAvailable(void)
{
	bool t_available;
	MCAndroidEngineRemoteCall("isNFCAvailable", "b", &t_available);
	return t_available;
}

bool MCSystemNFCIsEnabled(void)
{
	bool t_enabled;
	MCAndroidEngineRemoteCall("isNFCEnabled", "b", &t_enabled);
	return t_enabled;
}

void MCSystemEnableNFCDispatch(void)
{
	MCAndroidEngineRemoteCall("enableNFCDispatch", "v", nil);
}

void MCSystemDisableNFCDispatch(void)
{
	MCAndroidEngineRemoteCall("disableNFCDispatch", "v", nil);
}

////////////////////////////////////////////////////////////////////////////////

class MCNFCTagReceivedEvent: public MCCustomEvent
{
private:
	MCArrayRef m_tag;
	
public:
	MCNFCTagReceivedEvent (MCArrayRef p_tag)
	{
		m_tag = MCValueRetain(p_tag);
	}
	
	~MCNFCTagReceivedEvent()
	{
		MCValueRelease(m_tag);
	}
	
	void Destroy(void)
	{
		delete this;
	}
	
	void Dispatch(void)
	{
		MCdefaultstackptr -> getcurcard() -> message_with_valueref_args(MCM_nfc_tag_received, m_tag);
	}
};

bool MCNFCPostTagReceived(MCArrayRef p_tag)
{
	bool t_success;
	t_success = true;
	
	MCNFCTagReceivedEvent *t_event;
	t_event = nil;
	
	if (t_success)
	{
		t_event = new (nothrow) MCNFCTagReceivedEvent(p_tag);
		t_success = t_event != nil;
	}
	
	if (t_success)
	t_success = MCEventQueuePostCustom(t_event);
	
	if (!t_success)
	{
		if (t_event != nil)
			delete t_event;
	}
	
	return t_success;
}

extern "C" JNIEXPORT void JNICALL Java_com_runrev_android_NFCModule_doTagReceived(JNIEnv *env, jobject object, jobject tagMap) __attribute__((visibility("default")));
JNIEXPORT void JNICALL Java_com_runrev_android_NFCModule_doTagReceived(JNIEnv *env, jobject object, jobject tagMap)
{
	bool t_success;
	MCAutoArrayRef t_tag;
	t_success = MCJavaMapToArrayRef(env, tagMap, &t_tag);
	if (t_success)
		MCNFCPostTagReceived(*t_tag);
}

////////////////////////////////////////////////////////////////////////////////
