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

bool MCParseParameters(MCParameter*& p_parameters, const char *p_format, ...);

////////////////////////////////////////////////////////////////////////////////

class MCAndroidBrowserControl: public MCAndroidControl
{
public:
    MCAndroidBrowserControl(void);
    
	virtual MCNativeControlType GetType(void);

    virtual Exec_stat Set(MCNativeControlProperty property, MCExecPoint &ep);
    virtual Exec_stat Get(MCNativeControlProperty property, MCExecPoint &ep);
    virtual Exec_stat Do(MCNativeControlAction action, MCParameter *parameters);

    void HandleStartEvent(const char *p_url);
	void HandleFinishEvent(const char *p_url);
    void HandleLoadFailed(const char *p_url, const char *p_error);

    static char *s_js_tag;
    static char *s_js_result;

protected:
    virtual ~MCAndroidBrowserControl(void);
    virtual jobject CreateView(void);
    virtual void DeleteView(jobject view);
    
    char *ExecuteJavaScript(const char *p_javascript);
};


////////////////////////////////////////////////////////////////////////////////

char *MCAndroidBrowserControl::s_js_tag = nil;
char *MCAndroidBrowserControl::s_js_result = nil;

MCAndroidBrowserControl::MCAndroidBrowserControl(void)
{
}

MCAndroidBrowserControl::~MCAndroidBrowserControl(void)
{
    
}

MCNativeControlType MCAndroidBrowserControl::GetType(void)
{
    return kMCNativeControlTypeBrowser;
}

Exec_stat MCAndroidBrowserControl::Set(MCNativeControlProperty p_property, MCExecPoint &ep)
{
    jobject t_view;
    t_view = GetView();
    
    switch (p_property)
    {
        case kMCNativeControlPropertyUrl:
            if (t_view != nil)
                MCAndroidObjectRemoteCall(t_view, "setUrl", "vS", nil, &(ep.getsvalue()));
            return ES_NORMAL;
            
			// MW-2012-09-20: [[ Bug 10304 ]] Give access to bounce and scroll enablement of
			//   the WebView.
		case kMCNativeControlPropertyCanBounce:
			if (t_view != nil)
				MCAndroidObjectRemoteCall(t_view, "setCanBounce", "vb", nil, ep . getsvalue() == MCtruemcstring);
			return ES_NORMAL;
		case kMCNativeControlPropertyScrollingEnabled:
			if (t_view != nil)
				MCAndroidObjectRemoteCall(t_view, "setScrollingEnabled", "vb", nil, ep . getsvalue() == MCtruemcstring);
			return ES_NORMAL;
			
        default:
            break;
    }
    
    return MCAndroidControl::Set(p_property, ep);
}

Exec_stat MCAndroidBrowserControl::Get(MCNativeControlProperty p_property, MCExecPoint &ep)
{
    jobject t_view;
    t_view = GetView();
    
    switch (p_property)
    {
        case kMCNativeControlPropertyUrl:
        {
            char *t_url = nil;
            if (t_view != nil)
            {
                MCAndroidObjectRemoteCall(t_view, "getUrl", "s", &t_url);
                ep.setcstring(t_url);
                ep.grabsvalue();
            }
            return ES_NORMAL;
        }
            
        case kMCNativeControlPropertyCanRetreat:
        {
            bool t_can_retreat = false;
            if (t_view != nil)
                MCAndroidObjectRemoteCall(t_view, "canGoBack", "b", &t_can_retreat);
            ep.setsvalue(MCU_btos(t_can_retreat));
            return ES_NORMAL;
        }
            
        case kMCNativeControlPropertyCanAdvance:
        {
            bool t_can_retreat = false;
            if (t_view != nil)
                MCAndroidObjectRemoteCall(t_view, "canGoForward", "b", &t_can_retreat);
            ep.setsvalue(MCU_btos(t_can_retreat));
            return ES_NORMAL;
        }
            
        case kMCNativeControlPropertyCanBounce:
        {
            bool t_can_bounce = false;
            if (t_view != nil)
                MCAndroidObjectRemoteCall(t_view, "getCanBounce", "b", &t_can_bounce);
            ep.setsvalue(MCU_btos(t_can_bounce));
            return ES_NORMAL;
        }
			
        case kMCNativeControlPropertyScrollingEnabled:
        {
            bool t_scroll_enabled = false;
            if (t_view != nil)
                MCAndroidObjectRemoteCall(t_view, "getScrollingEnabled", "b", &t_scroll_enabled);
            ep.setsvalue(MCU_btos(t_scroll_enabled));
            return ES_NORMAL;
        }
			
        default:
            break;
    }
    
    return MCAndroidControl::Get(p_property, ep);
}

Exec_stat MCAndroidBrowserControl::Do(MCNativeControlAction p_action, MCParameter *p_parameters)
{
    jobject t_view;
    t_view = GetView();
    
    switch (p_action)
    {
        case kMCNativeControlActionAdvance:
        {
            bool t_success = true;
            
            int32_t t_steps = 1;
            if (p_parameters != nil)
                t_success = MCParseParameters(p_parameters, "i", &t_steps);
            
            if (t_success && t_view != nil)
                MCAndroidObjectRemoteCall(t_view, "goForward", "vi", nil, t_steps);
            
            return ES_NORMAL;
        }
            
        case kMCNativeControlActionRetreat:
        {
            bool t_success = true;
            
            int32_t t_steps = 1;
            if (p_parameters != nil)
                t_success = MCParseParameters(p_parameters, "i", &t_steps);
            
            if (t_success && t_view != nil)
                MCAndroidObjectRemoteCall(t_view, "goBack", "vi", nil, t_steps);
            
            return ES_NORMAL;
        }
            
        case kMCNativeControlActionReload:
            if (t_view != nil)
                MCAndroidObjectRemoteCall(t_view, "reload", "v", nil);
            return ES_NORMAL;
            
        case kMCNativeControlActionStop:
            if (t_view != nil)
                MCAndroidObjectRemoteCall(t_view, "stop", "v", nil);
            return ES_NORMAL;
            
        case kMCNativeControlActionLoad:
        {
            bool t_success = true;
            
            char *t_url, *t_html;
            t_url = nil;
            t_html = nil;
            if (t_success)
                t_success = MCParseParameters(p_parameters, "ss", &t_url, &t_html);
            
            if (t_success)
                MCAndroidObjectRemoteCall(t_view, "loadData", "vss", nil, t_url, t_html);
            
            delete t_url;
            delete t_html;
            return ES_NORMAL;
        }
            
        case kMCNativeControlActionExecute:
        {
            bool t_success = true;
            
            char *t_script = nil;
            char *t_result = nil;
            
            if (t_success)
                t_success = MCParseParameters(p_parameters, "s", &t_script);
            
            if (t_success)
            {
                t_result = ExecuteJavaScript(t_script);
                
                if (t_result != nil)
                    MCresult->grab(t_result, MCCStringLength(t_result));
                else
                    MCresult->sets("error");
            }
            
            delete t_script;
            return ES_NORMAL;
        }
            
        default:
            break;
    }
    
    return MCAndroidControl::Do(p_action, p_parameters);
}

////////////////////////////////////////////////////////////////////////////////

char *MCAndroidBrowserControl::ExecuteJavaScript(const char *p_script)
{
    char *t_result = nil;
    if (s_js_tag != nil)
        return nil;
    
    MCAndroidObjectRemoteCall(GetView(), "executeJavaScript", "ss", &s_js_tag, p_script);
    
    // wait for result, timeout after 30 seconds    
    real8 t_current_time = MCS_time();
    real8 t_timeout = t_current_time + 30.0;
    
    while (s_js_tag != nil && t_current_time < t_timeout)
    {
        MCscreen->wait(t_timeout - t_current_time, False, True);
        t_current_time = MCS_time();
    }
    
    if (s_js_tag != nil)
    {
        // timeout
        MCCStringFree(s_js_tag);
        s_js_tag = nil;
        return nil;
    }
    
    t_result = s_js_result;
    s_js_result = nil;
    
    return t_result;
}

extern "C" JNIEXPORT void JNICALL Java_com_runrev_android_nativecontrol_BrowserControl_doJSExecutionResult(JNIEnv *env, jobject object, jstring tag, jstring result) __attribute__((visibility("default")));

JNIEXPORT void JNICALL Java_com_runrev_android_nativecontrol_BrowserControl_doJSExecutionResult(JNIEnv *env, jobject object, jstring tag, jstring result)
{
    bool t_match = true;
    char *t_tag = nil;
    MCJavaStringToNative(MCJavaGetThreadEnv(), tag, t_tag);
    if (t_tag == nil || MCAndroidBrowserControl::s_js_tag == nil || !MCCStringEqual(t_tag, MCAndroidBrowserControl::s_js_tag))
        t_match = false;
    
    if (t_match)
    {
        MCJavaStringToNative(MCJavaGetThreadEnv(), result, MCAndroidBrowserControl::s_js_result);
        MCCStringFree(MCAndroidBrowserControl::s_js_tag);
        MCAndroidBrowserControl::s_js_tag = nil;
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
        MCNativeControl *t_old_target;
        t_old_target = ChangeTarget(this);
        t_target -> message_with_args(MCM_browser_started_loading, p_url);
        ChangeTarget(t_old_target);
    }
}

void MCAndroidBrowserControl::HandleFinishEvent(const char *p_url)
{
    MCObject *t_target;
    t_target = GetOwner();
    if (t_target != nil)
    {
        MCNativeControl *t_old_target;
        t_old_target = ChangeTarget(this);
        t_target -> message_with_args(MCM_browser_finished_loading, p_url);
        ChangeTarget(t_old_target);
    }
}

void MCAndroidBrowserControl::HandleLoadFailed(const char *p_url, const char *p_error)
{
	MCObject *t_target;
	t_target = GetOwner();
	if (t_target != nil)
	{
		MCNativeControl *t_old_target;
		t_old_target = ChangeTarget(this);
		t_target -> message_with_args(MCM_browser_load_failed, p_url, p_error);
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
        t_event = new MCAndroidBrowserStartFinishEvent((MCAndroidBrowserControl*)t_control, t_url, false);
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
        t_event = new MCAndroidBrowserStartFinishEvent((MCAndroidBrowserControl*)t_control, t_url, true);
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
        t_event = new MCAndroidBrowserLoadFailedEvent((MCAndroidBrowserControl*)t_control, t_url, t_error);
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
    r_control = new MCAndroidBrowserControl();
    return true;
}

////////////////////////////////////////////////////////////////////////////////
