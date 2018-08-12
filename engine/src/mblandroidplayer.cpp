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

#include "globdefs.h"
#include "filedefs.h"
#include "objdefs.h"
#include "parsedef.h"

#include "mcerror.h"

#include "printer.h"
#include "globals.h"
#include "dispatch.h"
#include "stack.h"
#include "image.h"
#include "player.h"
//#include "mblevent.h"
#include "param.h"
#include "eventqueue.h"
#include "osspec.h"
#include "exec.h"

#include <jni.h>
#include "mblandroidjava.h"
#include "mblandroidcontrol.h"
#include "mblandroidutil.h"

////////////////////////////////////////////////////////////////////////////////

bool path_to_apk_path(MCStringRef p_path, MCStringRef &r_apk_path);

////////////////////////////////////////////////////////////////////////////////

class MCAndroidPlayerControl: public MCAndroidControl
{
protected:
	static MCPropertyInfo kProperties[];
	static MCObjectPropertyTable kPropertyTable;
    static MCNativeControlActionInfo kActions[];
	static MCNativeControlActionTable kActionTable;
    
public:
    MCAndroidPlayerControl(void);
    
    virtual MCNativeControlType GetType(void);
    
    virtual const MCNativeControlActionTable *getactiontable(void) const { return &kActionTable; }
    virtual const MCObjectPropertyTable *getpropertytable(void) const { return &kPropertyTable; }

    void SetContent(MCExecContext& ctxt, MCStringRef p_content);
    void GetContent(MCExecContext& ctxt, MCStringRef& r_content);
    void SetShowController(MCExecContext& ctxt, bool p_value);
    void GetShowController(MCExecContext& ctxt, bool& r_value);
    void SetCurrentTime(MCExecContext& ctxt, integer_t p_time);
    void GetCurrentTime(MCExecContext& ctxt, integer_t& r_time);
    void SetLooping(MCExecContext& ctxt, bool p_value);
    void GetLooping(MCExecContext& ctxt, bool& r_value);
    
    void GetDuration(MCExecContext& ctxt, integer_t& r_duration);
    void GetNaturalSize(MCExecContext& ctxt, integer_t r_size[2]);
    // // PM-2015-09-15: [[ Bug 15925 ]] Add "playableDuration" prop to Android native player
    void GetPlayableDuration(MCExecContext& ctxt, integer_t& r_duration);
    
	// Player-specific actions
	void ExecPlay(MCExecContext& ctxt);
	void ExecPause(MCExecContext& ctxt);
    void ExecStop(MCExecContext& ctxt);
    
    void HandlePropertyAvailableEvent(const char *property);
    
protected:
    virtual ~MCAndroidPlayerControl(void);
    virtual jobject CreateView(void);
    virtual void DeleteView(jobject view);
    
private:
    char *m_path;
};

////////////////////////////////////////////////////////////////////////////////

MCPropertyInfo MCAndroidPlayerControl::kProperties[] =
{
    DEFINE_RW_CTRL_PROPERTY(P_CONTENT, String, MCAndroidPlayerControl, Content)
    DEFINE_RW_CTRL_PROPERTY(P_SHOW_CONTROLLER, Bool, MCAndroidPlayerControl, ShowController)
    DEFINE_RW_CTRL_PROPERTY(P_CURRENT_TIME, Int32, MCAndroidPlayerControl, CurrentTime)
    DEFINE_RW_CTRL_PROPERTY(P_LOOPING, Bool, MCAndroidPlayerControl, Looping)
    DEFINE_RO_CTRL_PROPERTY(P_DURATION, Int32, MCAndroidPlayerControl, Duration)
    DEFINE_RO_CTRL_PROPERTY(P_PLAYABLE_DURATION, Int32, MCAndroidPlayerControl, PlayableDuration)
    DEFINE_RO_CTRL_PROPERTY(P_NATURAL_SIZE, Int32X2, MCAndroidPlayerControl, NaturalSize)
};

MCObjectPropertyTable MCAndroidPlayerControl::kPropertyTable =
{
	&MCAndroidControl::kPropertyTable,
	sizeof(kProperties) / sizeof(kProperties[0]),
	&kProperties[0],
};

////////////////////////////////////////////////////////////////////////////////

MCNativeControlActionInfo MCAndroidPlayerControl::kActions[] =
{
    DEFINE_CTRL_EXEC_METHOD(Play, Void, MCAndroidPlayerControl, Play)
    DEFINE_CTRL_EXEC_METHOD(Pause, Void, MCAndroidPlayerControl, Pause)
    DEFINE_CTRL_EXEC_METHOD(Stop, Void, MCAndroidPlayerControl, Stop)
};

MCNativeControlActionTable MCAndroidPlayerControl::kActionTable =
{
    &MCAndroidControl::kActionTable,
    sizeof(kActions) / sizeof(kActions[0]),
    &kActions[0],
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
    jobject t_view;
    t_view = GetView();
    
    bool t_success = true;
    MCCStringFree(m_path);
    t_success = MCStringConvertToCString(p_content, m_path);
    if (MCCStringBeginsWith(m_path, "http://") || MCCStringBeginsWith(m_path, "https://"))
    {
        MCAndroidObjectRemoteCall(t_view, "setUrl", "bs", &t_success, m_path);
    }
    else
    {
        MCAutoStringRef t_resolved_path;
        MCAutoStringRef t_path;
        bool t_is_asset = false;
        MCAutoStringRef t_asset_path;
        
        /* UNCHECKED */ MCStringCreateWithCString(m_path, &t_path);
        
        /* UNCHECKED */ MCS_resolvepath(*t_path, &t_resolved_path);
        t_is_asset = path_to_apk_path(*t_resolved_path, &t_asset_path);
        
        MCAndroidObjectRemoteCall(t_view, "setFile", "bxb", &t_success, t_is_asset ? *t_asset_path : *t_resolved_path, t_is_asset);
    }
}

void MCAndroidPlayerControl::GetContent(MCExecContext& ctxt, MCStringRef& r_content)
{
    if (MCStringCreateWithCString(m_path, r_content))
        return;
    
    ctxt . Throw();
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

void MCAndroidPlayerControl::GetDuration(MCExecContext& ctxt, integer_t& r_duration)
{
    jobject t_view;
    t_view = GetView();
    
    MCAndroidObjectRemoteCall(t_view, "getDuration", "i", &r_duration);
}

// PM-2015-09-15: [[ Bug 15925 ]] Allow mobileControlGet(myPlayer, "playableDuration" on Android
void MCAndroidPlayerControl::GetPlayableDuration(MCExecContext& ctxt, integer_t& r_duration)
{
    jobject t_view;
    t_view = GetView();

    MCAndroidObjectRemoteCall(t_view, "getPlayableDuration", "i", &r_duration);
}

void MCAndroidPlayerControl::GetNaturalSize(MCExecContext& ctxt, integer_t r_size[2])
{
    jobject t_view;
    t_view = GetView();
    
    int32_t t_width = 0, t_height = 0;
    MCAndroidObjectRemoteCall(t_view, "getVideoWidth", "i", &t_width);
    MCAndroidObjectRemoteCall(t_view, "getVideoHeight", "i", &t_height);
    r_size[0] = t_width;
    r_size[1] = t_height;
}

// Player-specific actions
void MCAndroidPlayerControl::ExecPlay(MCExecContext& ctxt)
{
    jobject t_view;
    t_view = GetView();
    
    if (t_view == nil)
        return;
    
    MCAndroidObjectRemoteCall(t_view, "start", "v", nil);
}

void MCAndroidPlayerControl::ExecPause(MCExecContext& ctxt)
{
    jobject t_view;
    t_view = GetView();
    
    if (t_view == nil)
        return;
    
    MCAndroidObjectRemoteCall(t_view, "pause", "v", nil);
}

void MCAndroidPlayerControl::ExecStop(MCExecContext& ctxt)
{
    jobject t_view;
    t_view = GetView();
    
    if (t_view == nil)
        return;
    
    MCAndroidObjectRemoteCall(t_view, "stop", "v", nil);
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
    r_control = new (nothrow) MCAndroidPlayerControl();
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
		MCAutoStringRef t_property;
        /* UNCHECKED */ MCStringCreateWithCString(p_property, &t_property);
        MCNativeControl *t_old_target;
		t_old_target = ChangeTarget(this);
		t_target -> message_with_valueref_args(MCM_player_property_available, *t_property);
		ChangeTarget(t_old_target);
	}
}

extern "C" JNIEXPORT void JNICALL Java_com_runrev_android_nativecontrol_VideoControl_doPlayerFinished(JNIEnv *env, jobject object) __attribute__((visibility("default")));
JNIEXPORT void JNICALL Java_com_runrev_android_nativecontrol_VideoControl_doPlayerFinished(JNIEnv *env, jobject object)
{
    MCLog("doPlayerFinished");
    MCAndroidControl *t_control = nil;
    
    if (MCAndroidControl::FindByView(object, t_control))
        t_control->PostNotifyEvent(MCM_player_finished);
}

extern "C" JNIEXPORT void JNICALL Java_com_runrev_android_nativecontrol_VideoControl_doPlayerError(JNIEnv *env, jobject object) __attribute__((visibility("default")));
JNIEXPORT void JNICALL Java_com_runrev_android_nativecontrol_VideoControl_doPlayerError(JNIEnv *env, jobject object)
{
    MCLog("doPlayerError");
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
    MCLog("doPropertyAvailable");
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
        t_event = new (nothrow) MCNativePlayerPropertyAvailableEvent(t_player, t_prop_name);
        MCEventQueuePostCustom(t_event);
    }
}

// PM-2014-09-11: [[ Bug 13048 ]] Send movieTouched msg when touching the video screen
extern "C" JNIEXPORT void JNICALL Java_com_runrev_android_nativecontrol_VideoControl_doMovieTouched(JNIEnv *env, jobject object) __attribute__((visibility("default")));
JNIEXPORT void JNICALL Java_com_runrev_android_nativecontrol_VideoControl_doMovieTouched(JNIEnv *env, jobject object)
{
    MCLog("doMovieTouched");
    MCAndroidControl *t_control = nil;
    
    if (MCAndroidControl::FindByView(object, t_control))
        t_control->PostNotifyEvent(MCM_movie_touched);
    
}
