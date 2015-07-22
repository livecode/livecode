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

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>

#include <core.h>

#include "libbrowser_uiwebview.h"

////////////////////////////////////////////////////////////////////////////////

@interface MCUIWebViewBrowserDelegate : NSObject <UIWebViewDelegate>
{
	MCUIWebViewBrowser *m_instance;
	bool m_pending_request;
	bool m_frame_request;
}

- (id)initWithInstance:(MCUIWebViewBrowser*)instance;

- (void)webView:(UIWebView *)webView didFailLoadWithError:(NSError *)error;
- (BOOL)webView:(UIWebView *)webView shouldStartLoadWithRequest:(NSURLRequest *)request navigationType:(UIWebViewNavigationType)navigationType;
- (void)webViewDidFinishLoad:(UIWebView *)webView;
- (void)webViewDidStartLoad:(UIWebView *)webView;

- (void)setPendingRequest: (bool)newValue;

@end

////////////////////////////////////////////////////////////////////////////////

MCUIWebViewBrowser::MCUIWebViewBrowser(void)
{
	m_view = nil;
	m_delegate = nil;
}

MCUIWebViewBrowser::~MCUIWebViewBrowser(void)
{
	[(UIWebView *)m_view setDelegate: nil];
	[m_view release];
	
	[m_delegate release];
}

bool MCUIWebViewBrowser::GoToURL(const char *p_url)
{
	UIWebView *t_view;
	if (!GetView(t_view))
		return false;
	
	//[m_delegate setPendingRequest: true];
	[t_view loadRequest: [NSURLRequest requestWithURL: [NSURL URLWithString: [NSString stringWithCString: p_url encoding:NSUTF8StringEncoding]]]];
	
	return true;
}

bool MCUIWebViewBrowser::SetScrollingEnabled(bool p_value)
{
	UIWebView *t_view;
	if (!GetView(t_view))
		return false;
	
	[GetScrollView() setScrollEnabled: p_value ? YES : NO];
	
	return true;
}

bool MCUIWebViewBrowser::GetUrl(char *&r_url)
{
	UIWebView *t_view;
	if (!GetView(t_view))
		return false;
	
	return MCCStringClone([[[[t_view request] URL] absoluteString] cStringUsingEncoding: NSMacOSRomanStringEncoding], r_url);
}

bool MCUIWebViewBrowser::GetScrollingEnabled(bool& r_value)
{
	UIWebView *t_view;
	if (!GetView(t_view))
		return false;
	
	r_value = [GetScrollView() isScrollEnabled];
	return true;
}

bool MCUIWebViewBrowser::SetBoolProperty(MCBrowserProperty p_property, bool p_value)
{
	switch (p_property)
	{
		case kMCBrowserScrollbars:
			return SetScrollingEnabled(p_value);
			
		default:
			break;
	}
	
	return true;
}

bool MCUIWebViewBrowser::GetBoolProperty(MCBrowserProperty p_property, bool &r_value)
{
	switch (p_property)
	{
		case kMCBrowserScrollbars:
			return GetScrollingEnabled(r_value);
			
		default:
			break;
	}
	
	return true;
}

bool MCUIWebViewBrowser::SetStringProperty(MCBrowserProperty p_property, const char *p_utf8_string)
{
	switch (p_property)
	{
		case kMCBrowserHTMLText:
			return SetHTMLText(p_utf8_string);
			
		default:
			break;
	}
	
	return true;
}

bool MCUIWebViewBrowser::GetStringProperty(MCBrowserProperty p_property, char *&r_utf8_string)
{
	switch (p_property)
	{
		case kMCBrowserURL:
			return GetUrl(r_utf8_string);
			
		case kMCBrowserHTMLText:
			return GetHTMLText(r_utf8_string);
			
		default:
			break;
	}
	
	return true;
}

// Browser-specific actions
bool MCUIWebViewBrowser::ExecStop()
{
	UIWebView *t_view;
	if (!GetView(t_view))
		return false;
	
	[t_view stopLoading];
	return true;
}

bool MCUIWebViewBrowser::ExecAdvance()
{
	UIWebView *t_view;
	if (!GetView(t_view))
		return false;
	
	[t_view goForward];
	return true;
}

bool MCUIWebViewBrowser::ExecRetreat()
{
	UIWebView *t_view;
	if (!GetView(t_view))
		return false;
	
	[t_view goBack];
	return true;
}

bool MCUIWebViewBrowser::ExecReload()
{
	UIWebView *t_view;
	if (!GetView(t_view))
		return false;
	
	[t_view reload];
	return true;
}

bool MCUIWebViewBrowser::ExecExecute(const char *p_script)
{
	UIWebView *t_view;
	if (!GetView(t_view))
		return false;
	
	NSString *t_result;
	t_result = [t_view stringByEvaluatingJavaScriptFromString: [NSString stringWithCString: p_script encoding:NSUTF8StringEncoding]];
	
	if (t_result == nil)
		return false;

//	/* UNCHECKED */ MCStringCreateWithCFString((CFStringRef)t_result, &t_result_string);
	
	return true;
}

bool MCUIWebViewBrowser::ExecLoad(const char *p_url, const char *p_html)
{
	UIWebView *t_view;
	if (!GetView(t_view))
		return false;
	
	// MW-2012-10-01: [[ Bug 10422 ]] Make sure we mark a pending request so the
	//   HTML loading doesn't divert through a loadRequested message.
	[m_delegate setPendingRequest: true];
	[t_view loadHTMLString: [NSString stringWithCString: p_html encoding:NSUTF8StringEncoding] baseURL: [NSURL URLWithString: [NSString stringWithCString: p_url encoding:NSUTF8StringEncoding]]];
	return true;
}

////////////////////////////////////////////////////////////////////////////////

static bool datadetectortypes_from_string(const char *p_list, UIDataDetectorTypes &r_types)
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

static bool datadetectortypes_to_string(UIDataDetectorTypes p_types, char *&r_list)
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

//
// MW-2012-09-20: [[ Bug 10304 ]] Returns the UIScrollView which is embedded in
//   the UIWebView.
UIScrollView *MCUIWebViewBrowser::GetScrollView(void)
{
	UIWebView *t_view;
	if (!GetView(t_view))
		return nil;
	
	return [t_view scrollView];
}

////////////////////////////////////////////////////////////////////////////////

bool MCUIWebViewBrowser::Init(void)
{
	UIWebView *t_view;
	t_view = [[UIWebView alloc] initWithFrame: CGRectMake(0, 0, 0, 0)];
	if (t_view == nil)
		return false;
	
	[t_view setHidden: YES];
	
	m_delegate = [[MCUIWebViewBrowserDelegate alloc] initWithInstance: this];
	[t_view setDelegate: m_delegate];
	
	m_view = t_view;
	return true;
}

////////////////////////////////////////////////////////////////////////////////

@implementation MCUIWebViewBrowserDelegate

- (id)initWithInstance:(MCUIWebViewBrowser*)instance
{
	self = [super init];
	if (self == nil)
		return nil;
	
	m_instance = instance;
	m_pending_request = false;
	
	return self;
}

- (BOOL)webView:(UIWebView *)webView shouldStartLoadWithRequest:(NSURLRequest *)request navigationType:(UIWebViewNavigationType)navigationType
{
	if (m_pending_request)
	{
		m_pending_request = false;
		return YES;
	}
	
	m_frame_request = [request URL] != [request mainDocumentURL];
	
	if (!m_frame_request)
		m_instance->OnNavigationBegin(false, [[[request URL] absoluteString] cStringUsingEncoding:NSUTF8StringEncoding]);

	return YES;
}

- (void)webViewDidStartLoad:(UIWebView *)webView
{
	const char *t_url;
	t_url = [[[[webView request] URL] absoluteString] cStringUsingEncoding:NSUTF8StringEncoding];
	
	m_instance->OnDocumentLoadBegin(m_frame_request, t_url);
}

- (void)webViewDidFinishLoad:(UIWebView *)webView
{
	const char *t_url;
	t_url = [[[[webView request] URL] absoluteString] cStringUsingEncoding:NSUTF8StringEncoding];
	
	m_instance->OnDocumentLoadComplete(m_frame_request, t_url);
	if (!m_frame_request)
		m_instance->OnNavigationComplete(m_frame_request, t_url);
}

- (void)webView:(UIWebView *)webView didFailLoadWithError:(NSError *)error
{
	const char *t_url;
	t_url = [[[[webView request] URL] absoluteString] cStringUsingEncoding:NSUTF8StringEncoding];
	
	const char *t_error;
	t_error = [[error localizedDescription] cStringUsingEncoding:NSUTF8StringEncoding];
	m_instance->OnDocumentLoadFailed(m_frame_request, t_url, t_error);
	if (!m_frame_request)
		m_instance->OnNavigationFailed(m_frame_request, t_url, t_error);
}

- (void)setPendingRequest:(bool)p_new_value
{
	m_pending_request = p_new_value;
}

@end

////////////////////////////////////////////////////////////////////////////////

class MCUIWebViewBrowserFactory : public MCBrowserFactory
{
	virtual bool CreateBrowser(void *p_display, void *p_parent_view, MCBrowser *&r_browser)
	{
		MCUIWebViewBrowser *t_browser;
		t_browser = new MCUIWebViewBrowser();
		if (t_browser == nil)
			return false;
		
		if (!t_browser->Init())
		{
			delete t_browser;
			return false;
		}
		
		r_browser = t_browser;
		return true;
	}
};

bool MCUIWebViewBrowserFactoryCreate(MCBrowserFactoryRef &r_factory)
{
	MCBrowserFactory *t_factory;
	t_factory = new MCUIWebViewBrowserFactory();
	
	if (t_factory == nil)
		return false;
	
	r_factory = (MCBrowserFactoryRef)t_factory;
	return true;
}

////////////////////////////////////////////////////////////////////////////////
