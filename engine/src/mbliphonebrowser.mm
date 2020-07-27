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
#include "exec.h"

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

#include "mbliphoneapp.h"
#include "mbliphonecontrol.h"
#include "mblcontrol.h"

#include <libbrowser.h>

////////////////////////////////////////////////////////////////////////////////

extern "C" bool MCEngineBrowserLibrarySetupRunloop();
extern "C" void MCEngineBrowserLibraryTeardownRunloop();

bool MCParseParameters(MCParameter*& p_parameters, const char *p_format, ...);

////////////////////////////////////////////////////////////////////////////////

class MCiOSBrowserControl: public MCiOSControl
{
protected:
	static MCPropertyInfo kProperties[];
	static MCObjectPropertyTable kPropertyTable;
    static MCNativeControlActionInfo kActions[];
	static MCNativeControlActionTable kActionTable;
    
public:
	MCiOSBrowserControl(void);
	
	virtual MCNativeControlType GetType(void);
    
    virtual const MCObjectPropertyTable *getpropertytable(void) const { return &kPropertyTable; }
    virtual const MCNativeControlActionTable *getactiontable(void) const { return &kActionTable; }
    
    void SetUrl(MCExecContext& ctxt, MCStringRef p_url);
    void SetAutoFit(MCExecContext& ctxt, bool p_value);
    void SetDelayRequests(MCExecContext& ctxt, bool p_value);
    void SetDataDetectorTypes(MCExecContext& ctxt, MCNativeControlInputDataDetectorType p_type);
    void SetAllowsInlineMediaPlayback(MCExecContext& ctxt, bool p_value);
    void SetMediaPlaybackRequiresUserAction(MCExecContext& ctxt, bool p_value);
    void SetCanBounce(MCExecContext& ctxt, bool p_value);
    void SetScrollingEnabled(MCExecContext& ctxt, bool p_value);
    
    void GetUrl(MCExecContext& ctxt, MCStringRef& r_url);
    void GetAutoFit(MCExecContext& ctxt, bool& r_value);
    void GetDelayRequests(MCExecContext& ctxt, bool& r_value);
    void GetDataDetectorTypes(MCExecContext& ctxt, MCNativeControlInputDataDetectorType& r_type);
    void GetAllowsInlineMediaPlayback(MCExecContext& ctxt, bool& r_value);
    void GetMediaPlaybackRequiresUserAction(MCExecContext& ctxt, bool& r_value);
    void GetCanBounce(MCExecContext& ctxt, bool& r_value);
    void GetScrollingEnabled(MCExecContext& ctxt, bool& r_value);
    
    void GetCanAdvance(MCExecContext& ctxt, bool& r_value);
    void GetCanRetreat(MCExecContext& ctxt, bool& r_value);
    
	// Browser-specific actions
	void ExecAdvance(MCExecContext& ctxt);
	void ExecRetreat(MCExecContext& ctxt);
	void ExecReload(MCExecContext& ctxt);
    void ExecStop(MCExecContext& ctxt);
	void ExecExecute(MCExecContext& ctxt, MCStringRef p_script);
	void ExecLoad(MCExecContext& ctxt, MCStringRef p_url, MCStringRef p_html);
    
	void HandleStartEvent(void);
	void HandleFinishEvent(void);
	bool HandleLoadRequest(MCBrowserNavigationRequest *p_request, bool notify);
	void HandleLoadFailed(MCStringRef p_error);
	
	bool GetDelayRequests(void);
	
protected:
	virtual ~MCiOSBrowserControl(void);
	virtual UIView *CreateView(void);
	virtual void DeleteView(UIView *view);
	
private:
	bool m_delay_requests;
	MCBrowser *m_browser;
};

////////////////////////////////////////////////////////////////////////////////

MCPropertyInfo MCiOSBrowserControl::kProperties[] =
{
    DEFINE_RW_CTRL_PROPERTY(P_URL, String, MCiOSBrowserControl, Url)
    DEFINE_RW_CTRL_PROPERTY(P_AUTO_FIT, Bool, MCiOSBrowserControl, AutoFit)
    DEFINE_RW_CTRL_PROPERTY(P_DELAY_REQUESTS, Bool, MCiOSBrowserControl, DelayRequests)
    DEFINE_RW_CTRL_SET_PROPERTY(P_DATA_DETECTOR_TYPES, NativeControlInputDataDetectorType, MCiOSBrowserControl, DataDetectorTypes)
    DEFINE_RW_CTRL_PROPERTY(P_ALLOWS_INLINE_MEDIA_PLAYBACK, Bool, MCiOSBrowserControl, AllowsInlineMediaPlayback)
    DEFINE_RW_CTRL_PROPERTY(P_MEDIA_PLAYBACK_REQUIRES_USER_ACTION, Bool, MCiOSBrowserControl, MediaPlaybackRequiresUserAction)
    DEFINE_RW_CTRL_PROPERTY(P_CAN_BOUNCE, Bool, MCiOSBrowserControl, CanBounce)
    DEFINE_RW_CTRL_PROPERTY(P_SCROLLING_ENABLED, Bool, MCiOSBrowserControl, ScrollingEnabled)
    DEFINE_RO_CTRL_PROPERTY(P_CAN_ADVANCE, Bool, MCiOSBrowserControl, CanAdvance)
    DEFINE_RO_CTRL_PROPERTY(P_CAN_RETREAT, Bool, MCiOSBrowserControl, CanRetreat)
};

MCObjectPropertyTable MCiOSBrowserControl::kPropertyTable =
{
	&MCiOSControl::kPropertyTable,
	sizeof(kProperties) / sizeof(kProperties[0]),
	&kProperties[0],
};

////////////////////////////////////////////////////////////////////////////////

MCNativeControlActionInfo MCiOSBrowserControl::kActions[] =
{
    DEFINE_CTRL_EXEC_METHOD(Advance, Void, MCiOSBrowserControl, Advance)
    DEFINE_CTRL_EXEC_METHOD(Retreat, Void, MCiOSBrowserControl, Retreat)
    DEFINE_CTRL_EXEC_METHOD(Reload, Void, MCiOSBrowserControl, Reload)
    DEFINE_CTRL_EXEC_METHOD(Stop, Void, MCiOSBrowserControl, Stop)
    DEFINE_CTRL_EXEC_BINARY_METHOD(Load, String_String, MCiOSBrowserControl, String, String, Load)
    DEFINE_CTRL_WAITABLE_EXEC_UNARY_METHOD(Execute, String, MCiOSBrowserControl, String, Execute)
};

MCNativeControlActionTable MCiOSBrowserControl::kActionTable =
{
     &MCiOSControl::kActionTable,
     sizeof(kActions) / sizeof(kActions[0]),
     &kActions[0],
};

////////////////////////////////////////////////////////////////////////////////

MCiOSBrowserControl::MCiOSBrowserControl(void)
	: m_browser(nil)
{
	/* UNCHECKED */ MCEngineBrowserLibrarySetupRunloop();
	m_delay_requests = false;
}

MCiOSBrowserControl::~MCiOSBrowserControl(void)
{
	if (m_browser != nil)
		m_browser->Release();
	
	MCEngineBrowserLibraryTeardownRunloop();
}

MCNativeControlType MCiOSBrowserControl::GetType(void)
{
	return kMCNativeControlTypeBrowser;
}

void MCiOSBrowserControl::SetUrl(MCExecContext& ctxt, MCStringRef p_url)
{
	MCAutoStringRefAsUTF8String t_utf8_string;
	if (t_utf8_string.Lock(p_url))
		/* UNCHECKED */ m_browser->GoToURL(*t_utf8_string);
}

void MCiOSBrowserControl::SetAutoFit(MCExecContext& ctxt, bool p_value)
{
	m_browser->SetBoolProperty(kMCBrowseriOSAutoFit, p_value);
}

void MCiOSBrowserControl::SetDelayRequests(MCExecContext& ctxt, bool p_value)
{
    m_delay_requests = p_value;
}

void MCiOSBrowserControl::SetDataDetectorTypes(MCExecContext& ctxt, MCNativeControlInputDataDetectorType p_type)
{
	int32_t t_types;
	t_types = kMCBrowserDataDetectorTypeNone;
	
	if (p_type & kMCNativeControlInputDataDetectorTypeAll)
		t_types |= kMCBrowserDataDetectorTypeAll;
	
	if (p_type & kMCNativeControlInputDataDetectorTypeEmailAddress)
		t_types |= kMCBrowserDataDetectorTypeEmailAddress;
	if (p_type & kMCNativeControlInputDataDetectorTypeMapAddress)
		t_types |= kMCBrowserDataDetectorTypeMapAddress;
	if (p_type & kMCNativeControlInputDataDetectorTypePhoneNumber)
		t_types |= kMCBrowserDataDetectorTypePhoneNumber;
	if (p_type & kMCNativeControlInputDataDetectorTypeWebUrl)
		t_types |= kMCBrowserDataDetectorTypeLink;
	if (p_type & kMCNativeControlInputDataDetectorTypeCalendarEvent)
		t_types |= kMCBrowserDataDetectorTypeCalendarEvent;
	
	m_browser->SetIntegerProperty(kMCBrowserDataDetectorTypes, t_types);
}

void MCiOSBrowserControl::SetAllowsInlineMediaPlayback(MCExecContext& ctxt, bool p_value)
{
	m_browser->SetBoolProperty(kMCBrowseriOSAllowsInlineMediaPlayback, p_value);
}

void MCiOSBrowserControl::SetMediaPlaybackRequiresUserAction(MCExecContext& ctxt, bool p_value)
{
	m_browser->SetBoolProperty(kMCBrowseriOSMediaPlaybackRequiresUserAction, p_value);
}
void MCiOSBrowserControl::SetCanBounce(MCExecContext& ctxt, bool p_value)
{
	m_browser->SetBoolProperty(kMCBrowserScrollCanBounce, p_value);
}

void MCiOSBrowserControl::SetScrollingEnabled(MCExecContext& ctxt, bool p_value)
{
	m_browser->SetBoolProperty(kMCBrowserScrollEnabled, p_value);
}

void MCiOSBrowserControl::GetUrl(MCExecContext& ctxt, MCStringRef& r_url)
{
	MCAutoUTF8CharArray t_utf8_string;
	char *t_url;
	if (m_browser->GetStringProperty(kMCBrowserURL, t_url))
	{
		t_utf8_string.Give((char_t*)t_url, MCCStringLength(t_url));
		/* UNCHECKED */ t_utf8_string.CreateStringAndRelease(r_url);
	}
}
               
void MCiOSBrowserControl::GetCanAdvance(MCExecContext& ctxt, bool& r_value)
{
	m_browser->GetBoolProperty(kMCBrowserCanGoForward, r_value);
}

void MCiOSBrowserControl::GetCanRetreat(MCExecContext& ctxt, bool& r_value)
{
	m_browser->GetBoolProperty(kMCBrowserCanGoBack, r_value);
}

void MCiOSBrowserControl::GetAutoFit(MCExecContext& ctxt, bool& r_value)
{
	m_browser->GetBoolProperty(kMCBrowseriOSAutoFit, r_value);
}

void MCiOSBrowserControl::GetDelayRequests(MCExecContext& ctxt, bool& r_value)
{
    r_value = m_delay_requests;
}

void MCiOSBrowserControl::GetDataDetectorTypes(MCExecContext& ctxt, MCNativeControlInputDataDetectorType& r_type)
{
	uint32_t t_types;
	t_types = 0;
	
	int32_t t_browser_types;
	t_browser_types = kMCBrowserDataDetectorTypeNone;
	/* UNCHECKED */ m_browser->GetIntegerProperty(kMCBrowserDataDetectorTypes, t_browser_types);
	
	if (t_browser_types == kMCBrowserDataDetectorTypeAll)
		t_types = kMCNativeControlInputDataDetectorTypeAll;
	else
	{
		if (t_browser_types & kMCBrowserDataDetectorTypeCalendarEvent)
			t_types |= kMCNativeControlInputDataDetectorTypeCalendarEvent;
		if (t_browser_types & kMCBrowserDataDetectorTypeEmailAddress)
			t_types |= kMCNativeControlInputDataDetectorTypeEmailAddress;
		if (t_browser_types & kMCBrowserDataDetectorTypeMapAddress)
			t_types |= kMCNativeControlInputDataDetectorTypeMapAddress;
		if (t_browser_types & kMCBrowserDataDetectorTypePhoneNumber)
			t_types |= kMCNativeControlInputDataDetectorTypePhoneNumber;
		if (t_browser_types & kMCBrowserDataDetectorTypeLink)
			t_types |= kMCNativeControlInputDataDetectorTypeWebUrl;
	}
	
	if (t_types == 0)
		t_types = kMCNativeControlInputDataDetectorTypeNone;
	
    r_type = (MCNativeControlInputDataDetectorType)t_types;
}

void MCiOSBrowserControl::GetAllowsInlineMediaPlayback(MCExecContext& ctxt, bool& r_value)
{
	m_browser->GetBoolProperty(kMCBrowseriOSAllowsInlineMediaPlayback, r_value);
}

void MCiOSBrowserControl::GetMediaPlaybackRequiresUserAction(MCExecContext& ctxt, bool& r_value)
{
	m_browser->GetBoolProperty(kMCBrowseriOSMediaPlaybackRequiresUserAction, r_value);
}

void MCiOSBrowserControl::GetCanBounce(MCExecContext& ctxt, bool& r_value)
{
	MCBrowserGetBoolProperty(m_browser, kMCBrowserScrollCanBounce, r_value);
}

void MCiOSBrowserControl::GetScrollingEnabled(MCExecContext& ctxt, bool& r_value)
{
	m_browser->GetBoolProperty(kMCBrowserScrollEnabled, r_value);
}

// Browser-specific actions

void MCiOSBrowserControl::ExecStop(MCExecContext& ctxt)
{
	m_browser->StopLoading();
}

void MCiOSBrowserControl::ExecAdvance(MCExecContext& ctxt)
{
	m_browser->GoForward();
}

void MCiOSBrowserControl::ExecRetreat(MCExecContext& ctxt)
{
	m_browser->GoBack();
}

void MCiOSBrowserControl::ExecReload(MCExecContext& ctxt)
{
	m_browser->Reload();
}

void MCiOSBrowserControl::ExecExecute(MCExecContext& ctxt, MCStringRef p_script)
{
	MCAutoStringRefAsUTF8String t_script;
	if (!t_script.Lock(p_script))
		return;
	
	char *t_result;
	t_result = nil;
	
	if (!m_browser->EvaluateJavaScript(*t_script, t_result))
	{
        ctxt.SetTheResultToCString("error");
		return;
	}
	
	MCAutoUTF8CharArray t_utf8_string;
	t_utf8_string.Give((char_t*)t_result, MCCStringLength(t_result));
	
	MCAutoStringRef t_string_ref;
	/* UNCHECKED */ t_utf8_string.CreateStringAndRelease(&t_string_ref);
	
    ctxt.SetTheResultToValue(*t_string_ref);
}

void MCiOSBrowserControl::ExecLoad(MCExecContext& ctxt, MCStringRef p_url, MCStringRef p_html)
{
	MCAutoStringRefAsUTF8String t_html;
	if (!t_html.Lock(p_html))
		return;
	
	MCAutoStringRefAsUTF8String t_url;
	if (!t_url.Lock(p_url))
		return;
	
	m_browser->LoadHTMLText(*t_html, *t_url);
}

////////////////////////////////////////////////////////////////////////////////

bool MCiOSBrowserControl::GetDelayRequests(void)
{
	return m_delay_requests;
}

////////////////////////////////////////////////////////////////////////////////

void MCiOSBrowserControl::HandleStartEvent(void)
{
	MCObject *t_target;
	t_target = GetOwner();
	if (t_target != nil)
	{
		MCAutoStringRef t_string;
        MCExecContext ctxt(nil, nil, nil);
        GetUrl(ctxt, &t_string);
        MCNativeControl *t_old_target;
		t_old_target = ChangeTarget(this);
		t_target -> message_with_valueref_args(MCM_browser_started_loading, *t_string);
		ChangeTarget(t_old_target);
	}
}

void MCiOSBrowserControl::HandleFinishEvent(void)
{
	MCObject *t_target;
	t_target = GetOwner();
	if (t_target != nil)
	{
		MCAutoStringRef t_string;
        MCExecContext ctxt(nil, nil, nil);
        GetUrl(ctxt, &t_string);
        MCNativeControl *t_old_target;
		t_old_target = ChangeTarget(this);
		t_target -> message_with_valueref_args(MCM_browser_finished_loading, *t_string);
		ChangeTarget(t_old_target);
	}
}

bool MCBrowserNavigationTypeToString(MCBrowserNavigationType p_type, MCStringRef &r_string)
{
	switch (p_type)
	{
		case kMCBrowserNavigationTypeFollowLink:
			return MCStringCreateWithCString("click", r_string);
		case kMCBrowserNavigationTypeSubmitForm:
			return MCStringCreateWithCString("submit", r_string);
		case kMCBrowserNavigationTypeBackForward:
			return MCStringCreateWithCString("navigate", r_string);
		case kMCBrowserNavigationTypeReload:
			return MCStringCreateWithCString("reload", r_string);
		case kMCBrowserNavigationTypeResubmitForm:
			return MCStringCreateWithCString("resubmit", r_string);
		case kMCBrowserNavigationTypeOther:
			return MCStringCreateWithCString("other", r_string);
	}
}

// MW-2011-11-03: [[ LoadRequested ]] Returns 'true' if a request was loaded. If
//   'notify' is true, it dispatches a loadRequested message.
bool MCiOSBrowserControl::HandleLoadRequest(MCBrowserNavigationRequest *p_request, bool p_notify)
{
	MCObject *t_target;
	t_target = GetOwner();
	if (t_target == nil)
		return false;

	MCAutoStringRef t_type;
	/* UNCHECKED */ MCBrowserNavigationTypeToString(p_request->GetNavigationType(), &t_type);

	MCAutoStringRef t_url;
	/* UNCHECKED */ MCStringCreateWithBytes((char_t*)p_request->GetURL(), MCCStringLength(p_request->GetURL()), kMCStringEncodingUTF8, false, &t_url);
	
	MCNativeControl *t_old_target;
	t_old_target = ChangeTarget(this);
	
	// We return 'true' if we performed a 'loadRequest'. This only happens when
	// delayRequests is true, and this isn't the initial (non-notify) event.
	bool t_did_load;
	t_did_load = false;
	
	// Whether we send loadRequest or loadRequested depends on 'notify'.
	Exec_stat t_stat;
	t_stat = t_target -> message_with_valueref_args(p_notify ? MCM_browser_load_requested : MCM_browser_load_request, *t_url, *t_type);
	
	// Only in the initial (non-notify) mode do we need to potentially re-request
	// the load.
	if (!p_notify)
	{
		if (t_stat == ES_PASS || t_stat == ES_NOT_HANDLED)
		{
			p_request->Continue();
			t_did_load = true;
		}
	}
	
	ChangeTarget(t_old_target);

	return t_did_load;
}

void MCiOSBrowserControl::HandleLoadFailed(MCStringRef p_error)
{
	MCObject *t_target;
	t_target = GetOwner();
	if (t_target != nil)
	{
        MCAutoStringRef t_url;
        MCExecContext ctxt(nil, nil, nil);
        GetUrl(ctxt, &t_url);
        MCNativeControl *t_old_target;
		t_old_target = ChangeTarget(this);
		t_target -> message_with_valueref_args(MCM_browser_load_failed, *t_url, p_error);
		ChangeTarget(t_old_target);
	}
}

////////////////////////////////////////////////////////////////////////////////

class MCNativeBrowserStartFinishEvent: public MCCustomEvent
{
public:
	MCNativeBrowserStartFinishEvent(MCiOSBrowserControl *p_target, bool p_finish)
	{
		m_target = p_target;
		m_target -> Retain();
		m_finish = p_finish;
	}
	
	void Destroy(void)
	{
		m_target -> Release();
		delete this;
	}
	
	void Dispatch(void)
	{
		if (!m_finish)
			m_target -> HandleStartEvent();
		else
			m_target -> HandleFinishEvent();
	}
	
private:
	MCiOSBrowserControl *m_target;
	bool m_finish;
};

class MCNativeBrowserLoadRequestEvent: public MCCustomEvent
{
public:
	MCNativeBrowserLoadRequestEvent(MCiOSBrowserControl *p_target, MCBrowserNavigationRequest *p_request, bool p_notify)
	{
		m_target = p_target;
		m_target -> Retain();
		m_request = p_request;
		if (m_request != nil)
			m_request->Retain();
		m_notify = p_notify;
	}
	
	void Destroy(void)
	{
		m_target -> Release();
		if (m_request != nil)
			m_request->Release();
		delete this;
	}
	
	void Dispatch(void)
	{
		// MW-2011-11-03: [[ LoadRequested ]] If a load occurred, then post a load requested
		//   event.
		if (m_target -> HandleLoadRequest(m_request, m_notify))
			MCEventQueuePostCustom(new MCNativeBrowserLoadRequestEvent(m_target, m_request, true));
	}
	
private:
	MCiOSBrowserControl *m_target;
	MCBrowserNavigationRequest *m_request;
	bool m_notify;
};

class MCNativeBrowserLoadFailedEvent: public MCCustomEvent
{
public:
	MCNativeBrowserLoadFailedEvent(MCiOSBrowserControl *p_target, MCStringRef p_error)
	{
		m_target = p_target;
		m_target -> Retain();
		m_error = p_error;
		if (m_error != nil)
			MCValueRetain(m_error);
	}
	
	void Destroy(void)
	{
		m_target -> Release();
		if (m_error != nil)
			MCValueRelease(m_error);
		delete this;
	}
	
	void Dispatch(void)
	{
		m_target -> HandleLoadFailed(m_error);
	}
	
private:
	MCiOSBrowserControl *m_target;
	MCStringRef m_error;
};

////////////////////////////////////////////////////////////////////////////////

class MCiOSBrowserEventHandler : public MCBrowserEventHandler
{
public:
	MCiOSBrowserEventHandler(MCiOSBrowserControl *p_browser_control) : m_browser_control(p_browser_control)
	{
	}
	
	virtual void OnNavigationBegin(MCBrowser *p_browser, bool p_in_frame, const char *p_url)
	{
	}
	
	virtual void OnNavigationComplete(MCBrowser *p_browser, bool p_in_frame, const char *p_url)
	{
	}
	
	virtual void OnNavigationFailed(MCBrowser *p_browser, bool p_in_frame, const char *p_url, const char *p_error)
	{
	}

	virtual void OnDocumentLoadBegin(MCBrowser *p_browser, bool p_in_frame, const char *p_url)
	{
		if (!p_in_frame)
			loadStartFinish(false);
	}
	
	virtual void OnDocumentLoadComplete(MCBrowser *p_browser, bool p_in_frame, const char *p_url)
	{
		if (!p_in_frame)
			loadStartFinish(true);
	}
	
	virtual void OnDocumentLoadFailed(MCBrowser *p_browser, bool p_in_frame, const char *p_url, const char *p_error)
	{
		if (!p_in_frame)
			loadFail(p_error);
	}

	virtual void OnNavigationRequestUnhandled(MCBrowser *p_browser, bool p_in_frame, const char *p_url)
	{
	}
	
private:
	void loadStartFinish(bool p_finish)
	{
		MCCustomEvent *t_event;
		t_event = new MCNativeBrowserStartFinishEvent(m_browser_control, p_finish);
		MCEventQueuePostCustom(t_event);
	}
	
	void loadFail(const char *p_error)
	{
		MCAutoStringRef t_error;
		/* UNCHECKED */ MCStringCreateWithBytes((char_t*)p_error, MCCStringLength(p_error), kMCStringEncodingUTF8, false, &t_error);
		MCCustomEvent *t_event;
		t_event = new MCNativeBrowserLoadFailedEvent(m_browser_control, *t_error);
		MCEventQueuePostCustom(t_event);
	}
	
	MCiOSBrowserControl *m_browser_control;
};

class MCiOSBrowserNavigationRequestHandler : public MCBrowserNavigationRequestHandler
{
public:
	MCiOSBrowserNavigationRequestHandler(MCiOSBrowserControl *p_browser_control)
		: m_browser_control(p_browser_control)
	{
	}
	
	virtual bool OnNavigationRequest(MCBrowser *p_browser, MCBrowserNavigationRequest *p_request)
	{
		if (!m_browser_control -> GetDelayRequests())
		{
			// MW-2011-11-03: [[ LoadRequested ]] In non-delayrequests mode, we always
			//   post a loadRequested event (notice that we pass 'true' to notify).
			MCEventQueuePostCustom(new MCNativeBrowserLoadRequestEvent(m_browser_control, p_request, true));
			return false;
		}

		MCCustomEvent *t_event;
		t_event = new MCNativeBrowserLoadRequestEvent(m_browser_control, p_request, false);
		MCEventQueuePostCustom(t_event);
		return true;
	}

private:
	MCiOSBrowserControl *m_browser_control;
};

////////////////////////////////////////////////////////////////////////////////

UIView *MCiOSBrowserControl::CreateView(void)
{
	MCBrowser *t_browser;
	t_browser = nil;
	
	MCBrowserFactory *t_factory;
	t_factory = nil;
	
	if (!MCBrowserFactoryGet("default", t_factory))
		return nil;
	
	if (!t_factory->CreateBrowser(nil, nil, t_browser))
		return nil;
	
	/* UNCHECKED */ t_browser->SetBoolProperty(kMCBrowseriOSDelayRequests, true);
	
	MCBrowserEventHandler *t_event_handler;
	t_event_handler = new MCiOSBrowserEventHandler(this);
	
	MCBrowserNavigationRequestHandler *t_request_handler;
	t_request_handler = new MCiOSBrowserNavigationRequestHandler(this);
	
	t_browser->SetEventHandler(t_event_handler);
	t_browser->SetNavigationRequestHandler(t_request_handler);
	
	t_event_handler->Release();
	t_request_handler->Release();
	
	m_browser = t_browser;
	
	return (UIView*)t_browser->GetNativeLayer();
}

void MCiOSBrowserControl::DeleteView(UIView *p_view)
{
	if (m_browser)
	{
		m_browser->SetEventHandler(nil);
		m_browser->SetNavigationRequestHandler(nil);
		
		m_browser->Release();
		
		m_browser = nil;
	}
}

////////////////////////////////////////////////////////////////////////////////

bool MCNativeBrowserControlCreate(MCNativeControl*& r_control)
{
	r_control = new MCiOSBrowserControl;
	return true;
}

////////////////////////////////////////////////////////////////////////////////
