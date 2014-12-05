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

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

#include "mbliphoneapp.h"
#include "mbliphonecontrol.h"

////////////////////////////////////////////////////////////////////////////////

bool MCParseParameters(MCParameter*& p_parameters, const char *p_format, ...);

////////////////////////////////////////////////////////////////////////////////

class MCNativeBrowserControl;

@interface MCNativeBrowserDelegate : NSObject <UIWebViewDelegate>
{
	MCNativeBrowserControl *m_instance;
	bool m_pending_request;
}

- (id)initWithInstance:(MCNativeBrowserControl*)instance;

- (void)webView:(UIWebView *)webView didFailLoadWithError:(NSError *)error;
- (BOOL)webView:(UIWebView *)webView shouldStartLoadWithRequest:(NSURLRequest *)request navigationType:(UIWebViewNavigationType)navigationType;
- (void)webViewDidFinishLoad:(UIWebView *)webView;
- (void)webViewDidStartLoad:(UIWebView *)webView;

- (void)setPendingRequest: (bool)newValue;

@end

class MCNativeBrowserControl: public MCiOSControl
{
public:
	MCNativeBrowserControl(void);
	
	virtual MCNativeControlType GetType(void);
	
	virtual Exec_stat Set(MCNativeControlProperty property, MCExecPoint& ep);
	virtual Exec_stat Get(MCNativeControlProperty property, MCExecPoint& ep);	
	virtual Exec_stat Do(MCNativeControlAction action, MCParameter *parameters);
	
	void HandleStartEvent(void);
	void HandleFinishEvent(void);
	bool HandleLoadRequest(NSURLRequest *request, UIWebViewNavigationType type, bool notify);
	void HandleLoadFailed(NSError *error);
	
	const char *GetUrl(void);
	bool GetDelayRequests(void);
	
	UIScrollView *GetScrollView(void);
	
protected:
	virtual ~MCNativeBrowserControl(void);
	virtual UIView *CreateView(void);
	virtual void DeleteView(UIView *view);
	
private:
	MCNativeBrowserDelegate *m_delegate;
	bool m_delay_requests;
};

////////////////////////////////////////////////////////////////////////////////

MCNativeBrowserControl::MCNativeBrowserControl(void)
{
	m_delegate = nil;
	m_delay_requests = false;
}

MCNativeBrowserControl::~MCNativeBrowserControl(void)
{
}

MCNativeControlType MCNativeBrowserControl::GetType(void)
{
	return kMCNativeControlTypeBrowser;
}

bool datadetectortypes_from_string(const char *p_list, UIDataDetectorTypes &r_types);
bool datadetectortypes_to_string(UIDataDetectorTypes p_types, char *&r_list);
Exec_stat MCNativeBrowserControl::Set(MCNativeControlProperty p_property, MCExecPoint& ep)
{
	UIWebView *t_view;
	t_view = (UIWebView *)GetView();
	switch(p_property)
	{
		case kMCNativeControlPropertyUrl:
		{
			if (t_view != nil)
			{
                [m_delegate setPendingRequest: true];
				[t_view loadRequest: [NSURLRequest requestWithURL: [NSURL URLWithString: [NSString stringWithCString: ep . getcstring() encoding: NSMacOSRomanStringEncoding]]]];
			}
			
		}
		return ES_NORMAL;
		
		case kMCNativeControlPropertyAutoFit:
			if (t_view != nil)
				[t_view setScalesPageToFit: ep . getsvalue() == MCtruemcstring ? YES : NO];
			return ES_NORMAL;
			
		case kMCNativeControlPropertyDelayRequests:
			m_delay_requests = ep . getsvalue() == MCtruemcstring;
			return ES_NORMAL;
			
		case kMCNativeControlPropertyDataDetectorTypes:
			UIDataDetectorTypes t_data_types;
			t_data_types = UIDataDetectorTypeNone;
			if (ep.isempty() || ep.getsvalue() == "none")
				t_data_types = UIDataDetectorTypeNone;
			else if (ep.getsvalue() == "all")
				t_data_types = UIDataDetectorTypeAll;
			else
			{
				if (!datadetectortypes_from_string(ep.getcstring(), t_data_types))
				{
					MCeerror->add(EE_UNDEFINED, 0, 0, ep.getsvalue());
					return ES_ERROR;
				}
			}
			
			[t_view setDataDetectorTypes: t_data_types];

			return ES_NORMAL;
			
		case kMCNativeControlPropertyAllowsInlineMediaPlayback:
			if (t_view != nil)
				[t_view setAllowsInlineMediaPlayback: ep.getsvalue() == MCtruemcstring ? YES : NO];
			return ES_NORMAL;
			
		case kMCNativeControlPropertyMediaPlaybackRequiresUserAction:
			if (t_view != nil)
				[t_view setMediaPlaybackRequiresUserAction: ep.getsvalue() == MCtruemcstring ? YES : NO];
			return ES_NORMAL;
            
		// MW-2012-09-20: [[ Bug 10304 ]] Give access to bounce and scroll enablement of
		//   the UIWebView.
		case kMCNativeControlPropertyCanBounce:
			if (t_view != nil)
				[GetScrollView() setBounces: ep . getsvalue() == MCtruemcstring ? YES : NO];
			return ES_NORMAL;
		case kMCNativeControlPropertyScrollingEnabled:
			if (t_view != nil)
				[GetScrollView() setScrollEnabled: ep . getsvalue() == MCtruemcstring ? YES : NO];
			return ES_NORMAL;
            
        default:
            break;
	}
	
	return MCiOSControl::Set(p_property, ep);
}

Exec_stat MCNativeBrowserControl::Get(MCNativeControlProperty p_property, MCExecPoint& ep)
{
	UIWebView *t_view;
	t_view = (UIWebView *)GetView();
	switch(p_property)
	{
		case kMCNativeControlPropertyUrl:
			ep . copysvalue(GetUrl());
			return ES_NORMAL;
		case kMCNativeControlPropertyCanAdvance:
			ep . setsvalue(MCU_btos(t_view != nil ? [t_view canGoForward] == YES : NO));
			return ES_NORMAL;
		case kMCNativeControlPropertyCanRetreat:
			ep . setsvalue(MCU_btos(t_view != nil ? [t_view canGoBack] == YES : NO));
			return ES_NORMAL;
		case kMCNativeControlPropertyAutoFit:
			ep . setsvalue(MCU_btos(t_view != nil ? [t_view  scalesPageToFit] == YES : NO));
			return ES_NORMAL;
		case kMCNativeControlPropertyDelayRequests:
			ep . setsvalue(MCU_btos(m_delay_requests ? True : False));
			return ES_NORMAL;
		case kMCNativeControlPropertyDataDetectorTypes:
		{
			char *t_type_list = nil;
			if (!datadetectortypes_to_string([t_view dataDetectorTypes], t_type_list))
			{
				MCeerror->add(EE_UNDEFINED, 0, 0);
				return ES_ERROR;
			}
			ep.grabbuffer(t_type_list, MCCStringLength(t_type_list));
			return ES_NORMAL;
		}
		case kMCNativeControlPropertyAllowsInlineMediaPlayback:
			ep.setsvalue(MCU_btos(t_view != nil ? [t_view allowsInlineMediaPlayback] == YES : NO));
			return ES_NORMAL;
		case kMCNativeControlPropertyMediaPlaybackRequiresUserAction:
			ep.setsvalue(MCU_btos(t_view != nil ? [t_view mediaPlaybackRequiresUserAction] == YES : NO));
			return ES_NORMAL;
		
		// MW-2012-09-20: [[ Bug 10304 ]] Give access to bounce and scroll enablement of
		//   the UIWebView.
		case kMCNativeControlPropertyCanBounce:
			ep.setsvalue(MCU_btos(t_view != nil ? [GetScrollView() bounces] == YES : NO));
			return ES_NORMAL;
		case kMCNativeControlPropertyScrollingEnabled:
			ep.setsvalue(MCU_btos(t_view != nil ? [GetScrollView() isScrollEnabled] == YES : NO));
			return ES_NORMAL;
                    
        default:
            break;
	}
	
	return MCiOSControl::Get(p_property, ep);
}

Exec_stat MCNativeBrowserControl::Do(MCNativeControlAction p_action, MCParameter *p_parameters)
{
	UIWebView *t_view;
	t_view = (UIWebView *)GetView();
	
	switch(p_action)
	{
		case kMCNativeControlActionAdvance:
			if (t_view != nil)
				[t_view goForward];
			return ES_NORMAL;
		case kMCNativeControlActionRetreat:
			if (t_view != nil)
				[t_view goBack];
			return ES_NORMAL;
		case kMCNativeControlActionReload:
			if (t_view != nil)
				[t_view reload];
			return ES_NORMAL;
		case kMCNativeControlActionStop:
			if (t_view != nil)
				[t_view stopLoading];
			return ES_NORMAL;
		case kMCNativeControlActionLoad:
		{
			bool t_success;
			t_success = true;
			
			char *t_url, *t_html;
			t_url = nil;
			t_html = nil;
			if (t_success)
				t_success = MCParseParameters(p_parameters, "ss", &t_url, &t_html);
			
			// MW-2010-12-08: [[ Bug 9221 ]] Passed the wrong object type to baseURL!
			if (t_success)
            {
                // MW-2012-10-01: [[ Bug 10422 ]] Make sure we mark a pending request so the
                //   HTML loading doesn't divert through a loadRequested message.
                [m_delegate setPendingRequest: true];
				[t_view loadHTMLString: [NSString stringWithCString: t_html encoding: NSMacOSRomanStringEncoding] baseURL: [NSURL URLWithString: [NSString stringWithCString: t_url encoding: NSMacOSRomanStringEncoding]]];
            }
                
			delete t_url;
			delete t_html;
		}
		return ES_NORMAL;
			
			
		case kMCNativeControlActionExecute:
		{
			bool t_success;
			t_success = true;
			
			char *t_script;
			t_script = nil;
			if (t_success)
				t_success = MCParseParameters(p_parameters, "s", &t_script);
			
			if (t_success)
			{
				NSString *t_result;
				t_result = [t_view stringByEvaluatingJavaScriptFromString: [NSString stringWithCString: t_script encoding: NSMacOSRomanStringEncoding]];
				
				if (t_result != nil)
					MCresult -> copysvalue(MCString([t_result cStringUsingEncoding: NSMacOSRomanStringEncoding]));
				else
					MCresult -> sets("error");
			}
			
			delete t_script;
		}
		return ES_NORMAL;
            
        default:
            break;
	}
	
	return MCiOSControl::Do(p_action, p_parameters);
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

const char *MCNativeBrowserControl::GetUrl(void)
{
	UIWebView *t_view;
	t_view = (UIWebView *)GetView();
	if (t_view == nil)
		return "";
	
	return [[[[t_view request] URL] absoluteString] cStringUsingEncoding: NSMacOSRomanStringEncoding];
}

bool MCNativeBrowserControl::GetDelayRequests(void)
{
	return m_delay_requests;
}

// 
// MW-2012-09-20: [[ Bug 10304 ]] Returns the UIScrollView which is embedded in
//   the UIWebView.
UIScrollView *MCNativeBrowserControl::GetScrollView(void)
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

void MCNativeBrowserControl::HandleStartEvent(void)
{
	MCObject *t_target;
	t_target = GetOwner();
	if (t_target != nil)
	{
		MCNativeControl *t_old_target;
		t_old_target = ChangeTarget(this);
		t_target -> message_with_args(MCM_browser_started_loading, GetUrl());
		ChangeTarget(t_old_target);
	}
}

void MCNativeBrowserControl::HandleFinishEvent(void)
{
	MCObject *t_target;
	t_target = GetOwner();
	if (t_target != nil)
	{
		MCNativeControl *t_old_target;
		t_old_target = ChangeTarget(this);
		t_target -> message_with_args(MCM_browser_finished_loading, GetUrl());
		ChangeTarget(t_old_target);
	}
}

// MW-2011-11-03: [[ LoadRequested ]] Returns 'true' if a request was loaded. If
//   'notify' is true, it dispatches a loadRequested message.
bool MCNativeBrowserControl::HandleLoadRequest(NSURLRequest *p_request, UIWebViewNavigationType p_type, bool p_notify)
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
	t_stat = t_target -> message_with_args(p_notify ? MCM_browser_load_requested : MCM_browser_load_request, [[[p_request URL] absoluteString] cStringUsingEncoding: NSMacOSRomanStringEncoding], s_types[p_type]);
	
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

void MCNativeBrowserControl::HandleLoadFailed(NSError *p_error)
{
	MCObject *t_target;
	t_target = GetOwner();
	if (t_target != nil)
	{
		MCNativeControl *t_old_target;
		t_old_target = ChangeTarget(this);
		t_target -> message_with_args(MCM_browser_load_failed, GetUrl(), [[p_error localizedDescription] cStringUsingEncoding: NSMacOSRomanStringEncoding]);
		ChangeTarget(t_old_target);
	}
}

////////////////////////////////////////////////////////////////////////////////

UIView *MCNativeBrowserControl::CreateView(void)
{
	UIWebView *t_view;
	t_view = [[UIWebView alloc] initWithFrame: CGRectMake(0, 0, 0, 0)];
	if (t_view == nil)
		return nil;
	
	[t_view setHidden: YES];
	
	m_delegate = [[MCNativeBrowserDelegate alloc] initWithInstance: this];
	[t_view setDelegate: m_delegate];
	
	return t_view;
}

void MCNativeBrowserControl::DeleteView(UIView *p_view)
{
	[(UIWebView *)p_view setDelegate: nil];
	[p_view release];
	
	[m_delegate release];
}

////////////////////////////////////////////////////////////////////////////////

class MCNativeBrowserStartFinishEvent: public MCCustomEvent
{
public:
	MCNativeBrowserStartFinishEvent(MCNativeBrowserControl *p_target, bool p_finish)
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
	MCNativeBrowserControl *m_target;
	bool m_finish;
};

class MCNativeBrowserLoadRequestEvent: public MCCustomEvent
{
public:
	MCNativeBrowserLoadRequestEvent(MCNativeBrowserControl *p_target, NSURLRequest *p_request, UIWebViewNavigationType p_type, bool p_notify)
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
		// MW-2011-11-03: [[ LoadRequested ]] If a load occured, then post a load requested
		//   event.
		if (m_target -> HandleLoadRequest(m_request, m_type, m_notify))
			MCEventQueuePostCustom(new MCNativeBrowserLoadRequestEvent(m_target, m_request, m_type, true));
	}
	
private:
	MCNativeBrowserControl *m_target;
	NSURLRequest *m_request;
	UIWebViewNavigationType m_type;
	bool m_notify;
};

class MCNativeBrowserLoadFailedEvent: public MCCustomEvent
{
public:
	MCNativeBrowserLoadFailedEvent(MCNativeBrowserControl *p_target, NSError *p_error)
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
	MCNativeBrowserControl *m_target;
	NSURLRequest *m_request;
	NSError *m_error;
};

////////////////////////////////////////////////////////////////////////////////

@implementation MCNativeBrowserDelegate

- (id)initWithInstance:(MCNativeBrowserControl*)instance
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
	r_control = new MCNativeBrowserControl;
	return true;
}

////////////////////////////////////////////////////////////////////////////////
