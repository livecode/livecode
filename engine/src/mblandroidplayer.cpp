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

#include "prefix.h"

#include "core.h"
#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"

#include "mcerror.h"
#include "execpt.h"
#include "printer.h"
#include "globals.h"
#include "dispatch.h"
#include "stack.h"
#include "image.h"
#include "player.h"
#include "param.h"
#include "eventqueue.h"
#include "osspec.h"

#include <jni.h>
#include "mblandroidjava.h"
#include "mblandroidcontrol.h"
#include "mblandroidutil.h"

////////////////////////////////////////////////////////////////////////////////

bool path_to_apk_path(const char *p_path, const char *&r_apk_path);

////////////////////////////////////////////////////////////////////////////////

class MCAndroidPlayerControl: public MCAndroidControl
{
public:
    MCAndroidPlayerControl(void);
    
    virtual MCNativeControlType GetType(void);
    
    virtual Exec_stat Set(MCNativeControlProperty property, MCExecPoint &ep);
    virtual Exec_stat Get(MCNativeControlProperty property, MCExecPoint &ep);
    virtual Exec_stat Do(MCNativeControlAction action, MCParameter *parameters);
    
    void HandlePropertyAvailableEvent(const char *property);
    
protected:
    virtual ~MCAndroidPlayerControl(void);
    virtual jobject CreateView(void);
    virtual void DeleteView(jobject view);
    
private:
    char *m_path;
};

////////////////////////////////////////////////////////////////////////////////

MCAndroidPlayerControl::MCAndroidPlayerControl(void)
{
    m_path = nil;
}

MCAndroidPlayerControl::~MCAndroidPlayerControl(void)
{
    if (m_path != nil)
        MCCStringFree(m_path);
}

////////////////////////////////////////////////////////////////////////////////

MCNativeControlType MCAndroidPlayerControl::GetType(void)
{
    return kMCNativeControlTypePlayer;
}

Exec_stat MCAndroidPlayerControl::Set(MCNativeControlProperty p_property, MCExecPoint &ep)
{
    bool t_bool = false;
    int32_t t_integer;
    
    jobject t_view;
    t_view = GetView();
    
    switch (p_property)
    {
        case kMCNativeControlPropertyContent:
        {
            bool t_success = true;
            MCCStringFree(m_path);
            t_success = MCCStringClone(ep.getcstring(), m_path);
            if (MCCStringBeginsWith(m_path, "http://") || MCCStringBeginsWith(m_path, "https://"))
            {
                MCAndroidObjectRemoteCall(t_view, "setUrl", "bs", &t_success, m_path);
            }
            else
            {
                char *t_resolved_path = nil;
                bool t_is_asset = false;
                const char *t_asset_path = nil;
                
                t_resolved_path = MCS_resolvepath(m_path);
                t_is_asset = path_to_apk_path(t_resolved_path, t_asset_path);
                
                MCAndroidObjectRemoteCall(t_view, "setFile", "bsb", &t_success, t_is_asset ? t_asset_path : t_resolved_path, t_is_asset);
                
                MCCStringFree(t_resolved_path);
            }
            return ES_NORMAL;
        }
            
        case kMCNativeControlPropertyShowController:
        {
            if (!ParseBoolean(ep, t_bool))
                return ES_ERROR;
            MCAndroidObjectRemoteCall(t_view, "setShowController", "vb", nil, t_bool);
            return ES_NORMAL;
        }
            
        case kMCNativeControlPropertyCurrentTime:
        {
            if (!ParseInteger(ep, t_integer))
                return ES_ERROR;
            MCAndroidObjectRemoteCall(t_view, "setCurrentTime", "vi", nil, t_integer);
            return ES_NORMAL;
        }
            
        case kMCNativeControlPropertyLooping:
        {
            if (!ParseBoolean(ep, t_bool))
                return ES_ERROR;
            MCAndroidObjectRemoteCall(t_view, "setLooping", "vb", nil, t_bool);
            return ES_NORMAL;
        }
            
        default:
            break;
    }
    
    return MCAndroidControl::Set(p_property, ep);
}

Exec_stat MCAndroidPlayerControl::Get(MCNativeControlProperty p_property, MCExecPoint &ep)
{
    bool t_bool = false;
    int32_t t_integer;
    
    jobject t_view;
    t_view = GetView();
    
    switch (p_property)
    {
        case kMCNativeControlPropertyContent:
        {
            ep.setsvalue(m_path);
            return ES_NORMAL;
        }
            
        case kMCNativeControlPropertyShowController:
        {
            MCAndroidObjectRemoteCall(t_view, "getShowController", "b", &t_bool);
            FormatBoolean(ep, t_bool);
            return ES_NORMAL;
        }
        
        case kMCNativeControlPropertyLooping:
        {
            MCAndroidObjectRemoteCall(t_view, "getLooping", "b", &t_bool);
            FormatBoolean(ep, t_bool);
            return ES_NORMAL;
        }
            
        case kMCNativeControlPropertyDuration:
        {
            MCAndroidObjectRemoteCall(t_view, "getDuration", "i", &t_integer);
            FormatInteger(ep, t_integer);
            return ES_NORMAL;
        }
            
        case kMCNativeControlPropertyCurrentTime:
        {
            MCAndroidObjectRemoteCall(t_view, "getCurrentTime", "i", &t_integer);
            FormatInteger(ep, t_integer);
            return ES_NORMAL;
        }
            
        case kMCNativeControlPropertyNaturalSize:
        {
            int32_t t_width = 0, t_height = 0;
            MCAndroidObjectRemoteCall(t_view, "getVideoWidth", "i", &t_width);
            MCAndroidObjectRemoteCall(t_view, "getVideoHeight", "i", &t_height);
            sprintf(ep.getbuffer(I2L * 2 + 3), "%d,%d", t_width, t_height);
            ep.setstrlen();
            return ES_NORMAL;
        }
            
        default:
            break;
    }
    
    return MCAndroidControl::Get(p_property, ep);
}

Exec_stat MCAndroidPlayerControl::Do(MCNativeControlAction p_action, MCParameter *p_parameters)
{
    jobject t_view;
    t_view = GetView();
    
    switch (p_action)
    {
        case kMCNativeControlActionPlay:
            MCAndroidObjectRemoteCall(t_view, "start", "v", nil);
            return ES_NORMAL;
            
        case kMCNativeControlActionPause:
            MCAndroidObjectRemoteCall(t_view, "pause", "v", nil);
            return ES_NORMAL;
            
        case kMCNativeControlActionStop:
            MCAndroidObjectRemoteCall(t_view, "stop", "v", nil);
            return ES_NORMAL;
            
        default:
            break;
    }
    
    return MCAndroidControl::Do(p_action, p_parameters);
}

////////////////////////////////////////////////////////////////////////////////

jobject MCAndroidPlayerControl::CreateView(void)
{
    jobject t_view;
    MCAndroidEngineRemoteCall("createPlayerControl", "o", &t_view);
    return t_view;
}

void MCAndroidPlayerControl::DeleteView(jobject p_view)
{
    JNIEnv *env;
    env = MCJavaGetThreadEnv();
    
    env->DeleteGlobalRef(p_view);
}

////////////////////////////////////////////////////////////////////////////////

bool MCNativePlayerControlCreate(MCNativeControl *&r_control)
{
    r_control = new MCAndroidPlayerControl();
    return true;
}

////////////////////////////////////////////////////////////////////////////////

class MCNativePlayerPropertyAvailableEvent: public MCCustomEvent
{
public:
	// Note that we require p_property to be a C-string constant as we don't
	// copy it.
	MCNativePlayerPropertyAvailableEvent(MCAndroidPlayerControl *p_target, const char *p_property)
	{
		m_target = p_target;
		m_target -> Retain();
		m_property = p_property;
	}
	
	void Destroy(void)
	{
		m_target -> Release();
		delete this;
	}
	
	void Dispatch(void)
	{
		m_target -> HandlePropertyAvailableEvent(m_property);
	}
	
private:
	MCAndroidPlayerControl *m_target;
	const char *m_property;
};

void MCAndroidPlayerControl::HandlePropertyAvailableEvent(const char *p_property)
{
	MCObject *t_target;
	t_target = GetOwner();
	if (t_target != nil)
	{
	MCNativeControl *t_old_target;
		t_old_target = ChangeTarget(this);
		t_target -> message_with_args(MCM_player_property_available, p_property);
		ChangeTarget(t_old_target);
	}
}

extern "C" JNIEXPORT void JNICALL Java_com_runrev_android_nativecontrol_VideoControl_doPlayerFinished(JNIEnv *env, jobject object) __attribute__((visibility("default")));
JNIEXPORT void JNICALL Java_com_runrev_android_nativecontrol_VideoControl_doPlayerFinished(JNIEnv *env, jobject object)
{
    MCLog("doPlayerFinished", nil);
    MCAndroidControl *t_control = nil;
    
    if (MCAndroidControl::FindByView(object, t_control))
        t_control->PostNotifyEvent(MCM_player_finished);
}

extern "C" JNIEXPORT void JNICALL Java_com_runrev_android_nativecontrol_VideoControl_doPlayerError(JNIEnv *env, jobject object) __attribute__((visibility("default")));
JNIEXPORT void JNICALL Java_com_runrev_android_nativecontrol_VideoControl_doPlayerError(JNIEnv *env, jobject object)
{
    MCLog("doPlayerError", nil);
    MCAndroidControl *t_control = nil;
    
    if (MCAndroidControl::FindByView(object, t_control))
        t_control->PostNotifyEvent(MCM_player_error);
}

typedef enum
{
    kMCPlayerAvailablePropertyDuration = 1,
    kMCPlayerAvailablePropertyNaturalSize = 2,
} player_available_property;

extern "C" JNIEXPORT void JNICALL Java_com_runrev_android_nativecontrol_VideoControl_doPropertyAvailable(JNIEnv *env, jobject object, jint availableProperty) __attribute__((visibility("default")));
JNIEXPORT void JNICALL Java_com_runrev_android_nativecontrol_VideoControl_doPropertyAvailable(JNIEnv *env, jobject object, jint availableProperty)
{
    MCLog("doPropertyAvailable", nil);
    MCAndroidControl *t_control = nil;
    
    if (MCAndroidControl::FindByView(object, t_control))
    {
        const char *t_prop_name = nil;
        switch (availableProperty)
        {
            case kMCPlayerAvailablePropertyDuration:
                t_prop_name = "duration";
                break;
            case kMCPlayerAvailablePropertyNaturalSize:
                t_prop_name = "naturalSize";
                break;
        }
        MCAndroidPlayerControl *t_player = (MCAndroidPlayerControl*)t_control;
        MCCustomEvent *t_event;
        t_event = new MCNativePlayerPropertyAvailableEvent(t_player, t_prop_name);
        MCEventQueuePostCustom(t_event);
    }
}
