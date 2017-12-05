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

////////////////////////////////////////////////////////////////////////////////

bool MCParseParameters(MCParameter*& p_parameters, const char *p_format, ...);

////////////////////////////////////////////////////////////////////////////////

class MCiOSBrowserControl;

@interface com_runrev_livecode_MCiOSBrowserDelegate : NSObject <UIWebViewDelegate>
{
	MCiOSBrowserControl *m_instance;
	bool m_pending_request;
}

- (id)initWithInstance:(MCiOSBrowserControl*)instance;

- (void)webView:(UIWebView *)webView didFailLoadWithError:(NSError *)error;
- (BOOL)webView:(UIWebView *)webView shouldStartLoadWithRequest:(NSURLRequest *)request navigationType:(UIWebViewNavigationType)navigationType;
- (void)webViewDidFinishLoad:(UIWebView *)webView;
- (void)webViewDidStartLoad:(UIWebView *)webView;

- (void)setPendingRequest: (bool)newValue;

@end

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
	bool HandleLoadRequest(NSURLRequest *request, UIWebViewNavigationType type, bool notify);
	void HandleLoadFailed(NSError *error);
	
	bool GetDelayRequests(void);
	
	UIScrollView *GetScrollView(void);
	
protected:
	virtual ~MCiOSBrowserControl(void);
	virtual UIView *CreateView(void);
	virtual void DeleteView(UIView *view);
	
private:
	com_runrev_livecode_MCiOSBrowserDelegate *m_delegate;
	bool m_delay_requests;
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
    DEFINE_CTRL_EXEC_UNARY_METHOD(Execute, String, MCiOSBrowserControl, String, Execute)
};

MCNativeControlActionTable MCiOSBrowserControl::kActionTable =
{
     &MCiOSControl::kActionTable,
     sizeof(kActions) / sizeof(kActions[0]),
     &kActions[0],
};

////////////////////////////////////////////////////////////////////////////////

MCiOSBrowserControl::MCiOSBrowserControl(void)
{
	m_delegate = nil;
	m_delay_requests = false;
}

MCiOSBrowserControl::~MCiOSBrowserControl(void)
{
}

MCNativeControlType MCiOSBrowserControl::GetType(void)
{
	return kMCNativeControlTypeBrowser;
}

bool datadetectortypes_from_string(const char *p_list, UIDataDetectorTypes &r_types);
bool datadetectortypes_to_string(UIDataDetectorTypes p_types, char *&r_list);

void MCiOSBrowserControl::SetUrl(MCExecContext& ctxt, MCStringRef p_url)
{
    UIWebView *t_view;
	t_view = (UIWebView *)GetView();
    
    if (t_view != nil)
    {
        [m_delegate setPendingRequest: true];
        [t_view loadRequest: [NSURLRequest requestWithURL: [NSURL URLWithString: MCStringConvertToAutoreleasedNSString(p_url)]]];
    }
}

void MCiOSBrowserControl::SetAutoFit(MCExecContext& ctxt, bool p_value)
{
    UIWebView *t_view;
	t_view = (UIWebView *)GetView();
    if (t_view != nil)
        [t_view setScalesPageToFit: p_value ? YES : NO];
}

void MCiOSBrowserControl::SetDelayRequests(MCExecContext& ctxt, bool p_value)
{
    m_delay_requests = p_value;
}

void MCiOSBrowserControl::SetDataDetectorTypes(MCExecContext& ctxt, MCNativeControlInputDataDetectorType p_type)
{
    UIWebView *t_view;
	t_view = (UIWebView *)GetView();
    
	if (t_view)
    {
        UIDataDetectorTypes t_types;
        t_types = UIDataDetectorTypeNone;
        
        if (p_type & kMCNativeControlInputDataDetectorTypeAll)
            t_types |= UIDataDetectorTypeAll;
        if (p_type & kMCNativeControlInputDataDetectorTypeEmailAddress)
            t_types |= UIDataDetectorTypeAddress;
        if (p_type & kMCNativeControlInputDataDetectorTypePhoneNumber)
            t_types |= UIDataDetectorTypePhoneNumber;
        if (p_type & kMCNativeControlInputDataDetectorTypeWebUrl)
            t_types |= UIDataDetectorTypeLink;
        if (p_type & kMCNativeControlInputDataDetectorTypeCalendarEvent)
            t_types |= UIDataDetectorTypeCalendarEvent;
        
        [t_view setDataDetectorTypes: t_types];
    }
}

void MCiOSBrowserControl::SetAllowsInlineMediaPlayback(MCExecContext& ctxt, bool p_value)
{
    UIWebView *t_view;
	t_view = (UIWebView *)GetView();
    if (t_view != nil)
        [t_view setAllowsInlineMediaPlayback: p_value ? YES : NO];
}

void MCiOSBrowserControl::SetMediaPlaybackRequiresUserAction(MCExecContext& ctxt, bool p_value)
{
    UIWebView *t_view;
	t_view = (UIWebView *)GetView();
    if (t_view != nil)
        [t_view setMediaPlaybackRequiresUserAction: p_value ? YES : NO];
}
void MCiOSBrowserControl::SetCanBounce(MCExecContext& ctxt, bool p_value)
{
    // MW-2012-09-20: [[ Bug 10304 ]] Give access to bounce and scroll enablement of
    //   the UIWebView.
    UIWebView *t_view;
	t_view = (UIWebView *)GetView();
    if (t_view != nil)
        [GetScrollView() setBounces: p_value ? YES : NO];
}

void MCiOSBrowserControl::SetScrollingEnabled(MCExecContext& ctxt, bool p_value)
{
    UIWebView *t_view;
	t_view = (UIWebView *)GetView();
    if (t_view != nil)
        [GetScrollView() setScrollEnabled: p_value ? YES : NO];
}

void MCiOSBrowserControl::GetUrl(MCExecContext& ctxt, MCStringRef& r_url)
{
	UIWebView *t_view;
	t_view = (UIWebView *)GetView();
	if (t_view != nil && [[t_view request] URL] != nil)
    {
        /* UNCHECKED */ MCStringCreateWithCFStringRef((CFStringRef)[[[t_view request] URL] absoluteString], r_url);
        return;
    }
	
    r_url = MCValueRetain(kMCEmptyString);
}
               
void MCiOSBrowserControl::GetCanAdvance(MCExecContext& ctxt, bool& r_value)
{
    UIWebView *t_view;
	t_view = (UIWebView *)GetView();
    
    r_value = (t_view != nil ? [t_view canGoForward] == YES : NO);
}

void MCiOSBrowserControl::GetCanRetreat(MCExecContext& ctxt, bool& r_value)
{
    UIWebView *t_view;
	t_view = (UIWebView *)GetView();
    
    r_value = (t_view != nil ? [t_view canGoBack] == YES : NO);
}

void MCiOSBrowserControl::GetAutoFit(MCExecContext& ctxt, bool& r_value)
{
    UIWebView *t_view;
	t_view = (UIWebView *)GetView();
    
    r_value = (t_view != nil ? [t_view  scalesPageToFit] == YES : NO);
}

void MCiOSBrowserControl::GetDelayRequests(MCExecContext& ctxt, bool& r_value)
{
    r_value = m_delay_requests;
}

void MCiOSBrowserControl::GetDataDetectorTypes(MCExecContext& ctxt, MCNativeControlInputDataDetectorType& r_type)
{
    UIWebView *t_view;
	t_view = (UIWebView *)GetView();
    
    uint32_t t_native_types;
    uint32_t t_types;
    
    t_types = 0;
    
	if (t_view)
    {
        t_native_types = [t_view dataDetectorTypes];
        
        if (t_native_types & UIDataDetectorTypeAll)
        {
            t_types |= kMCNativeControlInputDataDetectorTypeCalendarEvent;
            t_types |= kMCNativeControlInputDataDetectorTypeEmailAddress;
            t_types |= kMCNativeControlInputDataDetectorTypePhoneNumber;
            t_types |= kMCNativeControlInputDataDetectorTypeWebUrl;
        }
        
        if (t_native_types & UIDataDetectorTypeCalendarEvent)
            t_types |= kMCNativeControlInputDataDetectorTypeCalendarEvent;
        if (t_native_types & UIDataDetectorTypeAddress)
            t_types |= kMCNativeControlInputDataDetectorTypeEmailAddress;
        if (t_native_types & UIDataDetectorTypePhoneNumber)
            t_types |= kMCNativeControlInputDataDetectorTypePhoneNumber;
        if (t_native_types & UIDataDetectorTypeLink)
            t_types |= kMCNativeControlInputDataDetectorTypeWebUrl;
    }
    
    r_type = (MCNativeControlInputDataDetectorType)t_types;
}

void MCiOSBrowserControl::GetAllowsInlineMediaPlayback(MCExecContext& ctxt, bool& r_value)
{
    UIWebView *t_view;
	t_view = (UIWebView *)GetView();
    
    r_value = (t_view != nil ? [t_view allowsInlineMediaPlayback] == YES : NO);
}

void MCiOSBrowserControl::GetMediaPlaybackRequiresUserAction(MCExecContext& ctxt, bool& r_value)
{
    UIWebView *t_view;
	t_view = (UIWebView *)GetView();
    
    r_value = (t_view != nil ? [t_view mediaPlaybackRequiresUserAction] == YES : NO);
}

void MCiOSBrowserControl::GetCanBounce(MCExecContext& ctxt, bool& r_value)
{
    // MW-2012-09-20: [[ Bug 10304 ]] Give access to bounce and scroll enablement of
    //   the UIWebView.
    UIWebView *t_view;
	t_view = (UIWebView *)GetView();
    
    r_value = (t_view != nil ? [GetScrollView() bounces] == YES : NO);
}

void MCiOSBrowserControl::GetScrollingEnabled(MCExecContext& ctxt, bool& r_value)
{
    UIWebView *t_view;
	t_view = (UIWebView *)GetView();
    
    r_value = (t_view != nil ? [GetScrollView() isScrollEnabled] == YES : NO);
}

// Browser-specific actions

void MCiOSBrowserControl::ExecStop(MCExecContext& ctxt)
{  
    UIWebView *t_view;
	t_view = (UIWebView *)GetView();
    
    if (t_view != nil)
        [t_view stopLoading];
}

void MCiOSBrowserControl::ExecAdvance(MCExecContext& ctxt)
{
	UIWebView *t_view;
	t_view = (UIWebView *)GetView();
    
    if (t_view == nil)
        return;
    
    [t_view goForward];
}

void MCiOSBrowserControl::ExecRetreat(MCExecContext& ctxt)
{
	UIWebView *t_view;
	t_view = (UIWebView *)GetView();
    
    if (t_view == nil)
        return;
    
    [t_view goBack];
}

void MCiOSBrowserControl::ExecReload(MCExecContext& ctxt)
{
	UIWebView *t_view;
	t_view = (UIWebView *)GetView();
    
    if (t_view == nil)
        return;
    
    [t_view reload];
}

void MCiOSBrowserControl::ExecExecute(MCExecContext& ctxt, MCStringRef p_script)
{
	UIWebView *t_view;
	t_view = (UIWebView *)GetView();
    
    NSString *t_result;
    t_result = [t_view stringByEvaluatingJavaScriptFromString: MCStringConvertToAutoreleasedNSString(p_script)];
    
    if (t_result == nil)
    {
        ctxt.SetTheResultToCString("error");
        return;
    }
    MCAutoStringRef t_result_string;
    /* UNCHECKED */ MCStringCreateWithCFStringRef((CFStringRef)t_result, &t_result_string);
    ctxt.SetTheResultToValue(*t_result_string);
}

void MCiOSBrowserControl::ExecLoad(MCExecContext& ctxt, MCStringRef p_url, MCStringRef p_html)
{
	UIWebView *t_view;
	t_view = (UIWebView *)GetView();
    
    if (t_view == nil)
        return;
    
    // MW-2012-10-01: [[ Bug 10422 ]] Make sure we mark a pending request so the
    //   HTML loading doesn't divert through a loadRequested message.
    [m_delegate setPendingRequest: true];
    [t_view loadHTMLString: MCStringConvertToAutoreleasedNSString(p_html) baseURL: [NSURL URLWithString: MCStringConvertToAutoreleasedNSString(p_url)]];
}

////////////////////////////////////////////////////////////////////////////////

bool datadetectortypes_from_string(const char *p_list, UIDataDetectorTypes &r_types)
{
	bool t_success = true;
	UIDataDetectorTypes t_data_types = UIDataDetectorTypeNone;
	
	const char *t_string_start = nil;
	const char *t_string_next = nil;
	uint32_t t_string_len = 0;
	
	t_string_start = p_list;
	
	while (t_success && t_string_start != nil)
	{
		t_string_next = nil;
		
		if (MCCStringFirstIndexOf(t_string_start, ',', t_string_len))
			t_string_next = t_string_start + t_string_len + 1;
		else
			t_string_len = MCCStringLength(t_string_start);
		
		while (t_string_len > 0 && t_string_start[0] == ' ')
		{
			t_string_len--;
			t_string_start++;
		}
		while (t_string_len > 0 && t_string_start[t_string_len - 1] == ' ')
			t_string_len--;
		
		if (t_string_len > 0)
		{
			if (MCCStringEqualSubstringCaseless(t_string_start, "phone number", t_string_len))
				t_data_types |= UIDataDetectorTypePhoneNumber;
			else if (MCCStringEqualSubstringCaseless(t_string_start, "link", t_string_len))
				t_data_types |= UIDataDetectorTypeLink;
			else if (MCCStringEqualSubstringCaseless(t_string_start, "address", t_string_len))
				t_data_types |= UIDataDetectorTypeAddress;
			else if (MCCStringEqualSubstringCaseless(t_string_start, "calendar event", t_string_len))
				t_data_types |= UIDataDetectorTypeCalendarEvent;
			else
				t_success = false;
		}
		
		t_string_start = t_string_next;
	}
	
	if (t_success)
		r_types = t_data_types;
	
	return t_success;
}

bool datadetectortypes_to_string(UIDataDetectorTypes p_types, char *&r_list)
{
	bool t_success = true;
	char *t_list = nil;
	
	if (p_types == UIDataDetectorTypeNone)
		t_success = MCCStringClone("", t_list);
	else if (p_types == UIDataDetectorTypeAll)
		t_success = MCCStringClone("all", t_list);
	else
	{
		if (t_success && (p_types & UIDataDetectorTypePhoneNumber))
			t_success = MCCStringAppendFormat(t_list, "%s%s", t_list == nil ? "" : ",", "phone number");
		if (t_success && (p_types & UIDataDetectorTypeLink))
			t_success = MCCStringAppendFormat(t_list, "%s%s", t_list == nil ? "" : ",", "link");
		if (t_success && (p_types & UIDataDetectorTypeAddress))
			t_success = MCCStringAppendFormat(t_list, "%s%s", t_list == nil ? "" : ",", "address");
		if (t_success && (p_types & UIDataDetectorTypeCalendarEvent))
			t_success = MCCStringAppendFormat(t_list, "%s%s", t_list == nil ? "" : ",", "calendar event");
	}
	
	if (t_success)
		r_list = t_list;
	else
		MCCStringFree(t_list);
	
	return t_success;
}

////////////////////////////////////////////////////////////////////////////////

bool MCiOSBrowserControl::GetDelayRequests(void)
{
	return m_delay_requests;
}

// 
// MW-2012-09-20: [[ Bug 10304 ]] Returns the UIScrollView which is embedded in
//   the UIWebView.
UIScrollView *MCiOSBrowserControl::GetScrollView(void)
{
	// For iOS5+ theres a proper accessor for it.
	if (MCmajorosversion >= 500)
		return [GetView() scrollView];
	
	// Otherwise, loop through subviews until we find (a subclass of a) scrollview.
	for(id t_subview in GetView().subviews)
		if ([[t_subview class] isSubclassOfClass: [UIScrollView class]])
			return (UIScrollView *)t_subview;
	
	return nil;
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

// MW-2011-11-03: [[ LoadRequested ]] Returns 'true' if a request was loaded. If
//   'notify' is true, it dispatches a loadRequested message.
bool MCiOSBrowserControl::HandleLoadRequest(NSURLRequest *p_request, UIWebViewNavigationType p_type, bool p_notify)
{
	MCObject *t_target;
	t_target = GetOwner();
	if (t_target == nil)
		return false;

	static const char *s_types[] =
	{
		"click",
		"submit",
		"navigate",
		"reload",
		"resumbit",
		"other"
	};
	
	MCNativeControl *t_old_target;
	t_old_target = ChangeTarget(this);
	
	// We return 'true' if we performed a 'loadRequest'. This only happens when
	// delayRequests is true, and this isn't the initial (non-notify) event.
	bool t_did_load;
	t_did_load = false;
	
	// Whether we send loadRequest or loadRequested depends on 'notify'.
	Exec_stat t_stat;
    MCAutoStringRef t_url, t_type;
    /* UNCHECKED */ MCStringCreateWithCFStringRef((CFStringRef)[[p_request URL] absoluteString], &t_url);
    /* UNCHECKED */ MCStringCreateWithCString(s_types[p_type], &t_type);
	t_stat = t_target -> message_with_valueref_args(p_notify ? MCM_browser_load_requested : MCM_browser_load_request, *t_url, *t_type);
	
	// Only in the initial (non-notify) mode do we need to potentially re-request
	// the load.
	if (!p_notify)
	{
		if (t_stat == ES_PASS || t_stat == ES_NOT_HANDLED)
		{
			[m_delegate setPendingRequest: true];
			// MW-2012-08-06: [[ Fibers ]] Invoke the system call on the main fiber.
			/* REMOTE */ MCIPhoneCallSelectorOnMainFiberWithObject(GetView(), @selector(loadRequest:), p_request);
			t_did_load = true;
		}
	}
	
	ChangeTarget(t_old_target);

	return t_did_load;
}

void MCiOSBrowserControl::HandleLoadFailed(NSError *p_error)
{
	MCObject *t_target;
	t_target = GetOwner();
	if (t_target != nil)
	{
        MCAutoStringRef t_url, t_description;
        MCExecContext ctxt(nil, nil, nil);
        GetUrl(ctxt, &t_url);
        /* UNCHECKED */ MCStringCreateWithCFStringRef((CFStringRef)[p_error localizedDescription], &t_description);
        MCNativeControl *t_old_target;
		t_old_target = ChangeTarget(this);
		t_target -> message_with_valueref_args(MCM_browser_load_failed, *t_url, *t_description);
		ChangeTarget(t_old_target);
	}
}

////////////////////////////////////////////////////////////////////////////////

UIView *MCiOSBrowserControl::CreateView(void)
{
	UIWebView *t_view;
	t_view = [[UIWebView alloc] initWithFrame: CGRectMake(0, 0, 0, 0)];
	if (t_view == nil)
		return nil;
	
	[t_view setHidden: YES];
	
	m_delegate = [[com_runrev_livecode_MCiOSBrowserDelegate alloc] initWithInstance: this];
	[t_view setDelegate: m_delegate];
	
	return t_view;
}

void MCiOSBrowserControl::DeleteView(UIView *p_view)
{
	[(UIWebView *)p_view setDelegate: nil];
	[p_view release];
	
	[m_delegate release];
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
	MCNativeBrowserLoadRequestEvent(MCiOSBrowserControl *p_target, NSURLRequest *p_request, UIWebViewNavigationType p_type, bool p_notify)
	{
		m_target = p_target;
		m_target -> Retain();
		m_request = p_request;
		[m_request retain];
		m_type = p_type;
		m_notify = p_notify;
	}
	
	void Destroy(void)
	{
		m_target -> Release();
		[m_request release];
		delete this;
	}
	
	void Dispatch(void)
	{
		// MW-2011-11-03: [[ LoadRequested ]] If a load occurred, then post a load requested
		//   event.
		if (m_target -> HandleLoadRequest(m_request, m_type, m_notify))
			MCEventQueuePostCustom(new MCNativeBrowserLoadRequestEvent(m_target, m_request, m_type, true));
	}
	
private:
	MCiOSBrowserControl *m_target;
	NSURLRequest *m_request;
	UIWebViewNavigationType m_type;
	bool m_notify;
};

class MCNativeBrowserLoadFailedEvent: public MCCustomEvent
{
public:
	MCNativeBrowserLoadFailedEvent(MCiOSBrowserControl *p_target, NSError *p_error)
	{
		m_target = p_target;
		m_target -> Retain();
		m_error = p_error;
		[m_error retain];
	}
	
	void Destroy(void)
	{
		m_target -> Release();
		[m_error release];
		delete this;
	}
	
	void Dispatch(void)
	{
		m_target -> HandleLoadFailed(m_error);
	}
	
private:
	MCiOSBrowserControl *m_target;
	NSURLRequest *m_request;
	NSError *m_error;
};

////////////////////////////////////////////////////////////////////////////////

@implementation com_runrev_livecode_MCiOSBrowserDelegate

- (id)initWithInstance:(MCiOSBrowserControl*)instance
{
	self = [super init];
	if (self == nil)
		return nil;
	
	m_instance = instance;
	m_pending_request = false;
	
	return self;
}

- (void)webView:(UIWebView *)webView didFailLoadWithError:(NSError *)error
{
	MCCustomEvent *t_event;
	t_event = new MCNativeBrowserLoadFailedEvent(m_instance, error);
	MCEventQueuePostCustom(t_event);
}

- (BOOL)webView:(UIWebView *)webView shouldStartLoadWithRequest:(NSURLRequest *)request navigationType:(UIWebViewNavigationType)navigationType
{
	if (m_pending_request)
	{
		m_pending_request = false;
		return YES;
	}
	
	if (!m_instance -> GetDelayRequests())
	{
		// MW-2011-11-03: [[ LoadRequested ]] In non-delayrequests mode, we always
		//   post a loadRequested event (notice that we pass 'true' to notify).
		MCEventQueuePostCustom(new MCNativeBrowserLoadRequestEvent(m_instance, request, navigationType, true));
		return YES;
	}

	MCCustomEvent *t_event;
	t_event = new MCNativeBrowserLoadRequestEvent(m_instance, request, navigationType, false);
	MCEventQueuePostCustom(t_event);
	return NO;
}

- (void)webViewDidFinishLoad:(UIWebView *)webView
{
	MCCustomEvent *t_event;
	t_event = new MCNativeBrowserStartFinishEvent(m_instance, true);
	MCEventQueuePostCustom(t_event);
}

- (void)webViewDidStartLoad:(UIWebView *)webView
{
	MCCustomEvent *t_event;
	t_event = new MCNativeBrowserStartFinishEvent(m_instance, false);
	MCEventQueuePostCustom(t_event);
}

- (void)setPendingRequest:(bool)p_new_value
{
    m_pending_request = p_new_value;
}

@end

////////////////////////////////////////////////////////////////////////////////

bool MCNativeBrowserControlCreate(MCNativeControl*& r_control)
{
	r_control = new MCiOSBrowserControl;
	return true;
}

////////////////////////////////////////////////////////////////////////////////
