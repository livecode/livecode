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
#include "param.h"
#include "eventqueue.h"
#include "osspec.h"
#include "exec.h"


#include <jni.h>
#include "mblandroidjava.h"
#include "mblandroidcontrol.h"
#include "mblandroidutil.h"

////////////////////////////////////////////////////////////////////////////////

bool MCParseParameters(MCParameter*& p_parameters, const char *p_format, ...);

////////////////////////////////////////////////////////////////////////////////

class MCAndroidBrowserControl: public MCAndroidControl
{
protected:
	static MCPropertyInfo kProperties[];
	static MCObjectPropertyTable kPropertyTable;
    static MCNativeControlActionInfo kActions[];
	static MCNativeControlActionTable kActionTable;

public:
    MCAndroidBrowserControl(void);
    
	virtual MCNativeControlType GetType(void);
    virtual const MCObjectPropertyTable *getpropertytable(void) const { return &kPropertyTable; }
    virtual const MCNativeControlActionTable *getactiontable(void) const { return &kActionTable; }
    
    void SetUrl(MCExecContext& ctxt, MCStringRef p_url);
    void SetCanBounce(MCExecContext& ctxt, bool p_value);
    void SetScrollingEnabled(MCExecContext& ctxt, bool p_value);
    
    void GetUrl(MCExecContext& ctxt, MCStringRef& r_url);
    void GetCanBounce(MCExecContext& ctxt, bool& r_value);
    void GetScrollingEnabled(MCExecContext& ctxt, bool& r_value);
    void GetCanAdvance(MCExecContext& ctxt, bool& r_value);
    void GetCanRetreat(MCExecContext& ctxt, bool& r_value);
    
	// Browser-specific actions
	void ExecAdvance(MCExecContext& ctxt, integer_t *p_steps);
	void ExecRetreat(MCExecContext& ctxt, integer_t *p_steps);
	void ExecReload(MCExecContext& ctxt);
    void ExecStop(MCExecContext& ctxt);
	void ExecExecute(MCExecContext& ctxt, MCStringRef p_script);
	void ExecLoad(MCExecContext& ctxt, MCStringRef p_url, MCStringRef p_html);
    
    void HandleStartEvent(const char *p_url);
	void HandleFinishEvent(const char *p_url);
    void HandleLoadFailed(const char *p_url, const char *p_error);

    static MCStringRef s_js_tag;
    static MCStringRef s_js_result;

protected:
    virtual ~MCAndroidBrowserControl(void);
    virtual jobject CreateView(void);
    virtual void DeleteView(jobject view);
    
    bool ExecuteJavaScript(MCStringRef p_javascript, MCStringRef& r_result);
};

////////////////////////////////////////////////////////////////////////////////

MCPropertyInfo MCAndroidBrowserControl::kProperties[] =
{
    DEFINE_RW_CTRL_PROPERTY(P_URL, String, MCAndroidBrowserControl, Url)
    DEFINE_RW_CTRL_PROPERTY(P_CAN_BOUNCE, Bool, MCAndroidBrowserControl, CanBounce)
    DEFINE_RW_CTRL_PROPERTY(P_SCROLLING_ENABLED, Bool, MCAndroidBrowserControl, ScrollingEnabled)
    DEFINE_RO_CTRL_PROPERTY(P_CAN_ADVANCE, Bool, MCAndroidBrowserControl, CanAdvance)
    DEFINE_RO_CTRL_PROPERTY(P_CAN_RETREAT, Bool, MCAndroidBrowserControl, CanRetreat)
};

MCObjectPropertyTable MCAndroidBrowserControl::kPropertyTable =
{
	&MCAndroidControl::kPropertyTable,
	sizeof(kProperties) / sizeof(kProperties[0]),
	&kProperties[0],
};

////////////////////////////////////////////////////////////////////////////////

MCNativeControlActionInfo MCAndroidBrowserControl::kActions[] =
{
    DEFINE_CTRL_EXEC_UNARY_METHOD(Advance, OptInteger, MCAndroidBrowserControl, OptionalInt32, Advance)
    DEFINE_CTRL_EXEC_UNARY_METHOD(Retreat, OptInteger, MCAndroidBrowserControl, OptionalInt32, Retreat)
    DEFINE_CTRL_EXEC_METHOD(Reload, Void, MCAndroidBrowserControl, Reload)
    DEFINE_CTRL_EXEC_METHOD(Stop, Void, MCAndroidBrowserControl, Stop)
    DEFINE_CTRL_EXEC_BINARY_METHOD(Load, String_String, MCAndroidBrowserControl, String, String, Load)
    DEFINE_CTRL_EXEC_UNARY_METHOD(Execute, String, MCAndroidBrowserControl, String, Execute)
};

MCNativeControlActionTable MCAndroidBrowserControl::kActionTable =
{
    &MCAndroidControl::kActionTable,
    sizeof(kActions) / sizeof(kActions[0]),
    &kActions[0],
};

////////////////////////////////////////////////////////////////////////////////

MCStringRef MCAndroidBrowserControl::s_js_tag = nil;
MCStringRef MCAndroidBrowserControl::s_js_result = nil;

MCAndroidBrowserControl::MCAndroidBrowserControl(void)
{
    s_js_tag = MCValueRetain(kMCEmptyString);
    s_js_result = MCValueRetain(kMCEmptyString);
}

MCAndroidBrowserControl::~MCAndroidBrowserControl(void)
{
    MCValueRelease(s_js_tag);
    MCValueRelease(s_js_result);
}

MCNativeControlType MCAndroidBrowserControl::GetType(void)
{
    return kMCNativeControlTypeBrowser;
}

void MCAndroidBrowserControl::SetUrl(MCExecContext& ctxt, MCStringRef p_url)
{
    jobject t_view;
    t_view = GetView();
    
    if (t_view != nil)
        MCAndroidObjectRemoteCall(t_view, "setUrl", "vx", nil, p_url);
}

void MCAndroidBrowserControl::SetCanBounce(MCExecContext& ctxt, bool p_value)
{
    // MW-2012-09-20: [[ Bug 10304 ]] Give access to bounce and scroll enablement of
    //   the WebView.
    jobject t_view;
    t_view = GetView();
    
    if (t_view != nil)
        MCAndroidObjectRemoteCall(t_view, "setCanBounce", "vb", nil, p_value);
}

void MCAndroidBrowserControl::SetScrollingEnabled(MCExecContext& ctxt, bool p_value)
{
    jobject t_view;
    t_view = GetView();
    
    if (t_view != nil)
        MCAndroidObjectRemoteCall(t_view, "setScrollingEnabled", "vb", nil, p_value);
}

void MCAndroidBrowserControl::GetUrl(MCExecContext& ctxt, MCStringRef& r_url)
{
    jobject t_view;
    t_view = GetView();
    
    MCAutoStringRef t_url;
    if (t_view != nil)
    {
        // PM-2015-06-12: [[ Bug 15494 ]] return type is stringref
        MCAndroidObjectRemoteCall(t_view, "getUrl", "x", &(&t_url));
        r_url = MCValueRetain(*t_url);
    }
}

void MCAndroidBrowserControl::GetCanAdvance(MCExecContext& ctxt, bool& r_value)
{
    jobject t_view;
    t_view = GetView();
    
    bool t_can_retreat = false;
    if (t_view != nil)
        MCAndroidObjectRemoteCall(t_view, "canGoForward", "b", &t_can_retreat);
    
    r_value = t_can_retreat;
}

void MCAndroidBrowserControl::GetCanRetreat(MCExecContext& ctxt, bool& r_value)
{
    jobject t_view;
    t_view = GetView();
    
    bool t_can_retreat = false;
    if (t_view != nil)
        MCAndroidObjectRemoteCall(t_view, "canGoBack", "b", &t_can_retreat);
    
    r_value = t_can_retreat;
}

void MCAndroidBrowserControl::GetCanBounce(MCExecContext& ctxt, bool& r_value)
{
    jobject t_view;
    t_view = GetView();
    
    bool t_can_bounce = false;
    if (t_view != nil)
        MCAndroidObjectRemoteCall(t_view, "getCanBounce", "b", &t_can_bounce);
    
    r_value = t_can_bounce;
}

void MCAndroidBrowserControl::GetScrollingEnabled(MCExecContext& ctxt, bool& r_value)
{
    jobject t_view;
    t_view = GetView();
    
    bool t_scroll_enabled = false;
    if (t_view != nil)
        MCAndroidObjectRemoteCall(t_view, "getScrollingEnabled", "b", &t_scroll_enabled);
    
    r_value = t_scroll_enabled;
}

// Browser-specific actions
void MCAndroidBrowserControl::ExecAdvance(MCExecContext& ctxt, integer_t *p_steps)
{
    jobject t_view;
    t_view = GetView();
    
    if (t_view == nil)
        return;

    MCAndroidObjectRemoteCall(t_view, "goForward", "vi", nil, p_steps != nil ? *p_steps : 1);
}

void MCAndroidBrowserControl::ExecRetreat(MCExecContext& ctxt, integer_t *p_steps)
{
    jobject t_view;
    t_view = GetView();
    
    if (t_view == nil)
        return;
    
    MCAndroidObjectRemoteCall(t_view, "goBack", "vi", nil, p_steps != nil ? *p_steps : 1);
}

void MCAndroidBrowserControl::ExecReload(MCExecContext& ctxt)
{
    jobject t_view;
    t_view = GetView();
    
    if (t_view == nil)
        return;
    
    MCAndroidObjectRemoteCall(t_view, "reload", "v", nil);
}

void MCAndroidBrowserControl::ExecLoad(MCExecContext& ctxt, MCStringRef p_url, MCStringRef p_html)
{    
    jobject t_view;
    t_view = GetView();
    
    if (t_view == nil)
        return;
    
    MCAndroidObjectRemoteCall(t_view, "loadData", "vxx", nil, p_url, p_html);
}

void MCAndroidBrowserControl::ExecStop(MCExecContext& ctxt)
{
    jobject t_view;
    t_view = GetView();
    
    if (t_view != nil)
        MCAndroidObjectRemoteCall(t_view, "stop", "v", nil);
}

void MCAndroidBrowserControl::ExecExecute(MCExecContext& ctxt, MCStringRef p_script)
{
    jobject t_view;
    t_view = GetView();
    
    if (t_view == nil)
        return;
    
    MCAutoStringRef t_result;
    
    if (!ExecuteJavaScript(p_script, &t_result))
    {
        ctxt.SetTheResultToCString("error");
        return;
    }
    
    ctxt.SetTheResultToValue(*t_result);
}

////////////////////////////////////////////////////////////////////////////////

bool MCAndroidBrowserControl::ExecuteJavaScript(MCStringRef p_script, MCStringRef& r_result)
{
    char *t_result = nil;
    if (!MCStringIsEmpty(s_js_tag))
        return false;
    
    MCAndroidObjectRemoteCall(GetView(), "executeJavaScript", "xx", &s_js_tag, p_script);
    
    // wait for result, timeout after 30 seconds    
    real8 t_current_time = MCS_time();
    real8 t_timeout = t_current_time + 30.0;
    
    while (!MCStringIsEmpty(s_js_tag) && t_current_time < t_timeout)
    {
        MCscreen->wait(t_timeout - t_current_time, False, True);
        t_current_time = MCS_time();
    }
    
    if (!MCStringIsEmpty(s_js_tag))
    {
        // timeout
        MCValueRelease(s_js_tag);
        s_js_tag = MCValueRetain(kMCEmptyString);
        return false;
    }
    
    r_result = MCValueRetain(s_js_result);
    MCValueRelease(s_js_result);
	s_js_result = nil;
    
    return true;
}

extern "C" JNIEXPORT void JNICALL Java_com_runrev_android_nativecontrol_BrowserControl_doJSExecutionResult(JNIEnv *env, jobject object, jstring tag, jstring result) __attribute__((visibility("default")));

JNIEXPORT void JNICALL Java_com_runrev_android_nativecontrol_BrowserControl_doJSExecutionResult(JNIEnv *env, jobject object, jstring tag, jstring result)
{
    bool t_match = true;
    char *t_tag = nil;
    MCJavaStringToNative(MCJavaGetThreadEnv(), tag, t_tag);
	
    if (t_tag == nil || MCStringIsEmpty(MCAndroidBrowserControl::s_js_tag) || !MCStringIsEqualToCString(MCAndroidBrowserControl::s_js_tag, t_tag, kMCCompareExact))
        t_match = false;
    
    if (t_match)
    {
        MCJavaStringToStringRef(MCJavaGetThreadEnv(), result, MCAndroidBrowserControl::s_js_result);
        MCValueRelease(MCAndroidBrowserControl::s_js_tag);
        MCAndroidBrowserControl::s_js_tag = MCValueRetain(kMCEmptyString);
    }
    
    MCCStringFree(t_tag);
}

////////////////////////////////////////////////////////////////////////////////

class MCAndroidBrowserStartFinishEvent: public MCCustomEvent
{
public:
	MCAndroidBrowserStartFinishEvent(MCAndroidBrowserControl *p_target, const char *p_url, bool p_finish)
	{
        MCCStringClone(p_url, m_url);
		m_target = p_target;
		m_target -> Retain();
		m_finish = p_finish;
	}
	
	void Destroy(void)
	{
		m_target -> Release();
        MCCStringFree(m_url);
		delete this;
	}
	
	void Dispatch(void)
	{
		if (!m_finish)
			m_target -> HandleStartEvent(m_url);
		else
			m_target -> HandleFinishEvent(m_url);
	}
	
private:
	MCAndroidBrowserControl *m_target;
    char *m_url;
	bool m_finish;
};

class MCAndroidBrowserLoadFailedEvent: public MCCustomEvent
{
public:
	MCAndroidBrowserLoadFailedEvent(MCAndroidBrowserControl *p_target, const char *p_url, const char *p_error)
	{
        MCCStringClone(p_url, m_url);
        MCCStringClone(p_error, m_error);
		m_target = p_target;
		m_target -> Retain();
	}
	
	void Destroy(void)
	{
		m_target -> Release();
        MCCStringFree(m_url);
        MCCStringFree(m_error);
		delete this;
	}
	
	void Dispatch(void)
	{
		m_target -> HandleLoadFailed(m_url, m_error);
	}
	
private:
	MCAndroidBrowserControl *m_target;
    char *m_url;
    char *m_error;
};

void MCAndroidBrowserControl::HandleStartEvent(const char *p_url)
{
    MCObject *t_target;
    t_target = GetOwner();
    if (t_target != nil)
    {
        MCAutoStringRef t_url;
        /* UNCHECKED */ MCStringCreateWithCString(p_url, &t_url);
        MCNativeControl *t_old_target;
        t_old_target = ChangeTarget(this);
        t_target -> message_with_valueref_args(MCM_browser_started_loading, *t_url);
        ChangeTarget(t_old_target);
    }
}

void MCAndroidBrowserControl::HandleFinishEvent(const char *p_url)
{
    MCObject *t_target;
    t_target = GetOwner();
    if (t_target != nil)
    {
        MCAutoStringRef t_url;
        /* UNCHECKED */ MCStringCreateWithCString(p_url, &t_url);
        MCNativeControl *t_old_target;
        t_old_target = ChangeTarget(this);
        t_target -> message_with_valueref_args(MCM_browser_finished_loading, *t_url);
        ChangeTarget(t_old_target);
    }
}

void MCAndroidBrowserControl::HandleLoadFailed(const char *p_url, const char *p_error)
{
	MCObject *t_target;
	t_target = GetOwner();
	if (t_target != nil)
	{
		MCAutoStringRef t_url, t_error;
        /* UNCHECKED */ MCStringCreateWithCString(p_url, &t_url);
        /* UNCHECKED */ MCStringCreateWithCString(p_error, &t_error);
        MCNativeControl *t_old_target;
		t_old_target = ChangeTarget(this);
		t_target -> message_with_valueref_args(MCM_browser_load_failed, *t_url, *t_error);
		ChangeTarget(t_old_target);
	}
}

extern "C" JNIEXPORT void JNICALL Java_com_runrev_android_nativecontrol_BrowserControl_doStartedLoading(JNIEnv *env, jobject object, jstring url) __attribute__((visibility("default")));
JNIEXPORT void JNICALL Java_com_runrev_android_nativecontrol_BrowserControl_doStartedLoading(JNIEnv *env, jobject object, jstring url)
{
    MCCustomEvent *t_event;
    MCAndroidControl *t_control = nil;
    char *t_url = nil;
    
    if (MCAndroidControl::FindByView(object, t_control) && MCJavaStringToNative(env, url, t_url))
    {
        t_event = new (nothrow) MCAndroidBrowserStartFinishEvent((MCAndroidBrowserControl*)t_control, t_url, false);
        MCEventQueuePostCustom(t_event);
    }
    
    MCCStringFree(t_url);
}

extern "C" JNIEXPORT void JNICALL Java_com_runrev_android_nativecontrol_BrowserControl_doFinishedLoading(JNIEnv *env, jobject object, jstring url) __attribute__((visibility("default")));
JNIEXPORT void JNICALL Java_com_runrev_android_nativecontrol_BrowserControl_doFinishedLoading(JNIEnv *env, jobject object, jstring url)
{
    MCCustomEvent *t_event;
    MCAndroidControl *t_control = nil;
    char *t_url = nil;
    
    if (MCAndroidControl::FindByView(object, t_control) && MCJavaStringToNative(env, url, t_url))
    {
        t_event = new (nothrow) MCAndroidBrowserStartFinishEvent((MCAndroidBrowserControl*)t_control, t_url, true);
        MCEventQueuePostCustom(t_event);
    }
    
    MCCStringFree(t_url);
}

extern "C" JNIEXPORT void JNICALL Java_com_runrev_android_nativecontrol_BrowserControl_doLoadingError(JNIEnv *env, jobject object, jstring url, jstring error) __attribute__((visibility("default")));
JNIEXPORT void JNICALL Java_com_runrev_android_nativecontrol_BrowserControl_doLoadingError(JNIEnv *env, jobject object, jstring url, jstring error)
{
    MCCustomEvent *t_event;
    MCAndroidControl *t_control = nil;
    char *t_url = nil;
    char *t_error = nil;
    
    if (MCAndroidControl::FindByView(object, t_control) &&
        MCJavaStringToNative(env, url, t_url) &&
        MCJavaStringToNative(env, error, t_error))
    {
        t_event = new (nothrow) MCAndroidBrowserLoadFailedEvent((MCAndroidBrowserControl*)t_control, t_url, t_error);
        MCEventQueuePostCustom(t_event);
    }
    
    MCCStringFree(t_url);
    MCCStringFree(t_error);
}

////////////////////////////////////////////////////////////////////////////////

jobject MCAndroidBrowserControl::CreateView(void)
{
    jobject t_view;
    MCAndroidEngineRemoteCall("createBrowserControl", "o", &t_view);
    return t_view;
}

void MCAndroidBrowserControl::DeleteView(jobject p_view)
{
    JNIEnv *env;
    env = MCJavaGetThreadEnv();
    
    env->DeleteGlobalRef(p_view);
}

////////////////////////////////////////////////////////////////////////////////

bool MCNativeBrowserControlCreate(MCNativeControl *&r_control)
{
    r_control = new (nothrow) MCAndroidBrowserControl();
    return true;
}

////////////////////////////////////////////////////////////////////////////////
