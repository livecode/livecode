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
#ifdef LEGACY_EXEC
    virtual Exec_stat Set(MCNativeControlProperty property, MCExecPoint &ep);
    virtual Exec_stat Get(MCNativeControlProperty property, MCExecPoint &ep);
#endif
    virtual Exec_stat Do(MCNativeControlAction action, MCParameter *parameters);

    void SetContent(MCExecContext& ctxt, MCStringRef p_content);
    void GetContent(MCExecContext& ctxt, MCStringRef& r_content);
    void SetFullscreen(MCExecContext& ctxt, bool p_value);
    void GetFullscreen(MCExecContext& ctxt, bool& r_value);
    void SetPreserveAspect(MCExecContext& ctxt, bool p_value);
    void GetPreserveAspect(MCExecContext& ctxt, bool& r_value);
    void SetShowController(MCExecContext& ctxt, bool p_value);
    void GetShowController(MCExecContext& ctxt, bool& r_value);
    void SetUseApplicationAudioSession(MCExecContext& ctxt, bool p_value);
    void GetUseApplicationAudioSession(MCExecContext& ctxt, bool& r_value);
    void SetStartTime(MCExecContext& ctxt, integer_t p_time);
    void GetStartTime(MCExecContext& ctxt, integer_t& r_time);
    void SetEndTime(MCExecContext& ctxt, integer_t p_time);
    void GetEndTime(MCExecContext& ctxt, integer_t& r_time);
    void SetCurrentTime(MCExecContext& ctxt, integer_t p_time);
    void GetCurrentTime(MCExecContext& ctxt, integer_t& r_time);
    void SetShouldAutoplay(MCExecContext& ctxt, bool p_value);
    void GetShouldAutoplay(MCExecContext& ctxt, bool& r_value);
    void SetLooping(MCExecContext& ctxt, bool p_value);
    void GetLooping(MCExecContext& ctxt, bool& r_value);
    void SetAllowsAirPlay(MCExecContext& ctxt, bool p_value);
    void GetAllowsAirPlay(MCExecContext& ctxt, bool& r_value);
    void SetPlayRate(MCExecContext& ctxt, double p_rate);
    void GetPlayRate(MCExecContext& ctxt, double& r_rate);
    
    void GetDuration(MCExecContext& ctxt, integer_t& r_duration);
    void GetPlayableDuration(MCExecContext& ctxt, integer_t& r_duration);
    void GetIsPreparedToPlay(MCExecContext& ctxt, bool& r_value);
    void GetLoadState(MCExecContext& ctxt, intset_t& r_state);
    void GetPlaybackState(MCExecContext& ctxt, intenum_t& r_state);
    void GetNaturalSize(MCExecContext& ctxt, MCPoint32& r_size);
    
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


void MCAndroidPlayerControl::SetContent(MCExecContext& ctxt, MCStringRef p_content)
{
    bool t_success = true;
    MCCStringFree(m_path);
    t_success = MCCStringClone(MCStringGetCString(p_content), m_path);
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
}

void MCAndroidPlayerControl::GetContent(MCExecContext& ctxt, MCStringRef& r_content)
{
    if (MCStringCreateWithCString(m_path, r_content))
        return;
    
    ctxt . Throw();
}
void MCAndroidPlayerControl::SetFullscreen(MCExecContext& ctxt, bool p_value)
{
    // NO-OP
}

void MCAndroidPlayerControl::GetFullscreen(MCExecContext& ctxt, bool& r_value)
{
    r_value = false;
}

void MCAndroidPlayerControl::SetPreserveAspect(MCExecContext& ctxt, bool p_value)
{
    // NO-OP
}

void MCAndroidPlayerControl::GetPreserveAspect(MCExecContext& ctxt, bool& r_value)
{
    r_value = false;
}

void MCAndroidPlayerControl::SetShowController(MCExecContext& ctxt, bool p_value)
{
    jobject t_view;
    t_view = GetView();
    
    MCAndroidObjectRemoteCall(t_view, "setShowController", "vb", nil, p_value);
}

void MCAndroidPlayerControl::GetShowController(MCExecContext& ctxt, bool& r_value)
{
    jobject t_view;
    t_view = GetView();
    
    MCAndroidObjectRemoteCall(t_view, "getShowController", "b", &r_value);
}

void MCAndroidPlayerControl::SetUseApplicationAudioSession(MCExecContext& ctxt, bool p_value)
{
    // NO-OP
}

void MCAndroidPlayerControl::GetUseApplicationAudioSession(MCExecContext& ctxt, bool& r_value)
{
    
    r_value = false;
}

void MCAndroidPlayerControl::SetStartTime(MCExecContext& ctxt, integer_t p_time)
{
    // NO-OP
}

void MCAndroidPlayerControl::GetStartTime(MCExecContext& ctxt, integer_t& r_time)
{
    r_time = -1;
}

void MCAndroidPlayerControl::SetEndTime(MCExecContext& ctxt, integer_t p_time)
{
    // NO-OP
}

void MCAndroidPlayerControl::GetEndTime(MCExecContext& ctxt, integer_t& r_time)
{
    r_time = -1;
}
void MCAndroidPlayerControl::SetCurrentTime(MCExecContext& ctxt, integer_t p_time)
{
    jobject t_view;
    t_view = GetView();
    
    MCAndroidObjectRemoteCall(t_view, "setCurrentTime", "vi", nil, p_time);
}

void MCAndroidPlayerControl::GetCurrentTime(MCExecContext& ctxt, integer_t& r_time)
{
    jobject t_view;
    t_view = GetView();
    
    MCAndroidObjectRemoteCall(t_view, "getCurrentTime", "i", &r_time);
}

void MCAndroidPlayerControl::SetShouldAutoplay(MCExecContext& ctxt, bool p_value)
{
    // NO-OP
}

void MCAndroidPlayerControl::GetShouldAutoplay(MCExecContext& ctxt, bool& r_value)
{
    r_value = false;
}

void MCAndroidPlayerControl::SetLooping(MCExecContext& ctxt, bool p_value)
{
    jobject t_view;
    t_view = GetView();
    
    MCAndroidObjectRemoteCall(t_view, "setLooping", "vb", nil, p_value);
}

void MCAndroidPlayerControl::GetLooping(MCExecContext& ctxt, bool& r_value)
{
    jobject t_view;
    t_view = GetView();
    
    MCAndroidObjectRemoteCall(t_view, "getLooping", "b", &r_value);
}

void MCAndroidPlayerControl::SetAllowsAirPlay(MCExecContext& ctxt, bool p_value)
{
    // NO-OP
}

void MCAndroidPlayerControl::GetAllowsAirPlay(MCExecContext& ctxt, bool& r_value)
{
    r_value = false;
}

void MCAndroidPlayerControl::SetPlayRate(MCExecContext& ctxt, double p_rate)
{
    // NO-OP
}

void MCAndroidPlayerControl::GetPlayRate(MCExecContext& ctxt, double& r_rate)
{
    r_rate = 0;
}
void MCAndroidPlayerControl::GetDuration(MCExecContext& ctxt, integer_t& r_duration)
{
    jobject t_view;
    t_view = GetView();
    
    MCAndroidObjectRemoteCall(t_view, "getDuration", "i", &r_duration);
}

void MCAndroidPlayerControl::GetPlayableDuration(MCExecContext& ctxt, integer_t& r_duration)
{
    r_duration = 0;
}

void MCAndroidPlayerControl::GetIsPreparedToPlay(MCExecContext& ctxt, bool& r_value)
{
    r_value = false;
}

void MCAndroidPlayerControl::GetLoadState(MCExecContext& ctxt, intset_t& r_state)
{
    r_state = kMCNativeControlLoadStateNone;
}

void MCAndroidPlayerControl::GetPlaybackState(MCExecContext& ctxt, intenum_t& r_state)
{
    r_state = kMCNativeControlPlaybackStateNone;
}

void MCAndroidPlayerControl::GetNaturalSize(MCExecContext& ctxt, MCPoint32& r_size)
{
    jobject t_view;
    t_view = GetView();
    
    int32_t t_width = 0, t_height = 0;
    MCAndroidObjectRemoteCall(t_view, "getVideoWidth", "i", &t_width);
    MCAndroidObjectRemoteCall(t_view, "getVideoHeight", "i", &t_height);
    r_size . x = t_width;
    r_size . y = t_height;
}

#ifdef /* MCAndroidPlayerControl::Set */
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
#endif /* MCAndroidPlayerControl::Set */

void MCAndroidPlayerControl::Set(MCExecContext& ctxt, MCNativeControlProperty p_property)
{
    MCExecPoint& ep = ctxt . GetEP();
    
    switch (p_property)
    {
        case kMCNativeControlPropertyContent:
        {
            MCAutoStringRef t_string;
            /* UNCHECKED */ ep . copyasstringref(&t_string);
            SetContent(ctxt, *t_string);
            return;
        }
        case kMCNativeControlPropertyShowController:
        {
            bool t_value;
            if (!ep . copyasbool(t_value))
                ctxt . LegacyThrow(EE_OBJECT_NAB);
            else
                SetShowController(ctxt, t_value);
            return;
        }
        case kMCNativeControlPropertyCurrentTime:
        {
            int32_t t_time;
            if (!ep . copyasint(t_time))
                ctxt . LegacyThrow(EE_OBJECT_NAN);
            else
                SetCurrentTime(ctxt, t_time);
            return;
        }
        case kMCNativeControlPropertyLooping:
        {
            bool t_value;
            if (!ep . copyasbool(t_value))
                ctxt . LegacyThrow(EE_OBJECT_NAB);
            else
                SetLooping(ctxt, t_value);
            return;
        }
        default:
            break;
    }
    
    MCAndroidControl::Set(ctxt, p_property);
}

#ifdef /* MCAndroidPlayerControl::Get */ LEGACY_EXEC
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
			char t_buffer[I2L * 2 + 3];
            sprintf(t_buffer, "%d,%d", t_width, t_height);
            ep.setuint(strlen(t_buffer));
            return ES_NORMAL;
        }
            
        default:
            break;
    }
    
    return MCAndroidControl::Get(p_property, ep);
}
#endif /* MCAndroidPlayerControl::Get */

void MCAndroidPlayerControl::Get(MCExecContext& ctxt, MCNativeControlProperty p_property)
{
    MCExecPoint& ep = ctxt . GetEP();
    switch (p_property)
    {
        case kMCNativeControlPropertyContent:
        {
            MCAutoStringRef t_string;
            GetContent(ctxt, &t_string);
            if (*t_string != nil)
                ep . setvalueref(*t_string);
            return;
        }
        case kMCNativeControlPropertyShowController:
        {
            bool t_value;
            GetShowController(ctxt, t_value);
            ep . setbool(t_value);
            return;
        }
        case kMCNativeControlPropertyLooping:
        {
            bool t_value;
            GetLooping(ctxt, t_value);
            ep . setbool(t_value);
            return;
        }
        case kMCNativeControlPropertyDuration:
        {
            int32_t t_duration;
            GetDuration(ctxt, t_duration);
            ep . setnvalue(t_duration);
            return;
        }
        case kMCNativeControlPropertyCurrentTime:
        {
            int32_t t_time;
            GetCurrentTime(ctxt, t_time);
            ep . setnvalue(t_time);
            return;
        }
        case kMCNativeControlPropertyNaturalSize:
        {
            MCPoint32 t_size;
            GetNaturalSize(ctxt, t_size);
            ep.setstringf("%d,%d", (int32_t)t_size . x, (int32_t)t_size . y);
        }
        default:
            break;
    }
    MCAndroidControl::Get(ctxt, p_property);
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
