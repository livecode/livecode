/* Copyright (C) 2015 LiveCode Ltd.
 
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

/* JSCore not supported on iOS version < 7.0 */
#ifdef __IPHONE_7_0
#import <JavaScriptCore/JavaScriptCore.h>
#endif

#include <core.h>

#include "libbrowser_internal.h"
#include "libbrowser_uiwebview.h"
#include "libbrowser_nsvalue.h"

////////////////////////////////////////////////////////////////////////////////

#define LIBBROWSER_DUMMY_URL "http://libbrowser_dummy_url/"

////////////////////////////////////////////////////////////////////////////////

@interface com_runrev_livecode_MCUIWebViewBrowserDelegate : NSObject <UIWebViewDelegate>
{
	MCUIWebViewBrowser *m_instance;
	bool m_pending_request;
	char *m_request_url;
	char *m_frame_request_url;
}

- (id)initWithInstance:(MCUIWebViewBrowser*)instance;
- (void)dealloc;

- (void)webView:(UIWebView *)webView didFailLoadWithError:(NSError *)error;
- (BOOL)webView:(UIWebView *)webView shouldStartLoadWithRequest:(NSURLRequest *)request navigationType:(UIWebViewNavigationType)navigationType;
- (void)webViewDidFinishLoad:(UIWebView *)webView;
- (void)webViewDidStartLoad:(UIWebView *)webView;

- (void)setPendingRequest: (bool)newValue;

@end

////////////////////////////////////////////////////////////////////////////////

#ifdef TARGET_SUBPLATFORM_IPHONE
extern void MCIPhoneRunBlockOnMainFiber(void (^block)(void));
inline void MCBrowserRunBlockOnMainFiber(void (^p_block)(void))
{
	MCIPhoneRunBlockOnMainFiber(p_block);
}
#else
inline void MCBrowserRunBlockOnMainFiber(void (^p_block)(void))
{
	p_block();
}
#endif

////////////////////////////////////////////////////////////////////////////////

MCUIWebViewBrowser::MCUIWebViewBrowser(void)
{
	m_view = nil;
	m_delegate = nil;
	
	m_js_handlers = nil;
	m_js_handler_list =nil;
}

MCUIWebViewBrowser::~MCUIWebViewBrowser(void)
{
	MCBrowserRunBlockOnMainFiber(^{
		if (m_view != nil)
		{
			[m_view setDelegate: nil];
			[m_view release];
		}
		
		if (m_delegate != nil)
			[m_delegate release];
        
        if (m_js_handlers != nil)
            MCCStringFree(m_js_handlers);
        
		if (m_js_handler_list != nil)
			[m_js_handler_list release];
	});
}

bool MCUIWebViewBrowser::GoToURL(const char *p_url)
{
	UIWebView *t_view;
	if (!GetView(t_view))
		return false;
	
	//[m_delegate setPendingRequest: true];
	MCBrowserRunBlockOnMainFiber(^{
		[t_view loadRequest: [NSURLRequest requestWithURL: [NSURL URLWithString: [NSString stringWithCString: p_url encoding:NSUTF8StringEncoding]]]];
	});
	
	return true;
}

bool MCUIWebViewBrowser::GetUrl(char *&r_url)
{
	UIWebView *t_view;
	if (!GetView(t_view))
		return false;
	
	__block bool t_success;
	MCBrowserRunBlockOnMainFiber(^{
		t_success = MCCStringClone([[[[t_view request] URL] absoluteString] cStringUsingEncoding: NSUTF8StringEncoding], r_url);
	});
	
	return t_success;
}

bool MCUIWebViewBrowser::SetVerticalScrollbarEnabled(bool p_value)
{
	UIWebView *t_view;
	if (!GetView(t_view))
		return false;
	
	MCBrowserRunBlockOnMainFiber(^{
		GetScrollView().showsVerticalScrollIndicator = p_value ? YES : NO;
	});
	
	return true;
}

bool MCUIWebViewBrowser::GetVerticalScrollbarEnabled(bool& r_value)
{
	UIWebView *t_view;
	if (!GetView(t_view))
		return false;
	
	MCBrowserRunBlockOnMainFiber(^{
		r_value = GetScrollView().showsVerticalScrollIndicator;
	});
	
	return true;
}

bool MCUIWebViewBrowser::SetHorizontalScrollbarEnabled(bool p_value)
{
	UIWebView *t_view;
	if (!GetView(t_view))
		return false;
	
	MCBrowserRunBlockOnMainFiber(^{
		GetScrollView().showsHorizontalScrollIndicator = p_value ? YES : NO;
	});
	
	return true;
}

bool MCUIWebViewBrowser::GetHorizontalScrollbarEnabled(bool& r_value)
{
	UIWebView *t_view;
	if (!GetView(t_view))
		return false;
	
	MCBrowserRunBlockOnMainFiber(^{
		r_value = GetScrollView().showsHorizontalScrollIndicator;
	});
	
	return true;
}

bool MCUIWebViewBrowser::GetHTMLText(char *&r_htmltext)
{
	return EvaluateJavaScript("document.documentElement.outerHTML", r_htmltext);
}

bool MCUIWebViewBrowser::SetHTMLText(const char *p_htmltext)
{
	return ExecLoad(LIBBROWSER_DUMMY_URL, p_htmltext);
}

bool MCUIWebViewBrowser::GetJavaScriptHandlers(char *&r_handlers)
{
	return MCCStringClone(m_js_handlers ? m_js_handlers : "", r_handlers);
}

bool MCUIWebViewBrowser::SetJavaScriptHandlers(const char *p_handlers)
{
	char *t_handlers;
	t_handlers = nil;
	
	if (!MCCStringIsEmpty(p_handlers) && !MCCStringClone(p_handlers, t_handlers))
		return false;
	
	if (m_js_handlers != nil)
	{
		MCCStringFree(m_js_handlers);
		m_js_handlers = nil;
	}
	
	if (m_js_handler_list != nil)
	{
		[ m_js_handler_list release];
		m_js_handler_list = nil;
	}
	
	m_js_handlers = t_handlers;
	t_handlers = nil;
	
	return AttachJSHandlers();
}

bool MCUIWebViewBrowser::AttachJSHandlers()
{
/* JSCore not supported on iOS version < 7.0 */
#ifdef __IPHONE_7_0
	
	if (NSClassFromString(@"JSContext"))
	{
		if (m_js_handlers != nil)
		{
			if (m_js_handler_list == nil)
				m_js_handler_list = [[[NSString stringWithCString: m_js_handlers encoding:NSUTF8StringEncoding] componentsSeparatedByString: @"\n"] retain];
		}
		
		MCBrowserRunBlockOnMainFiber(^{
			SyncJavaScriptHandlers(m_js_handler_list);
		});
	}
	
#endif

	return true;
}

bool MCUIWebViewBrowser::SyncJavaScriptHandlers(NSArray *p_handlers)
{
	bool t_success;
	t_success = true;
	
/* JSCore not supported on iOS version < 7.0 */
#ifdef __IPHONE_7_0

	// get js context for browser
	JSContext *t_context;
	t_context = nil;
	if (t_success)
	{
		t_context = [m_view valueForKeyPath: @"documentView.webView.mainFrame.javaScriptContext"];
		t_success = t_context != nil;
	}
	
	if (t_success)
	{
		// create empty liveCode object
		t_context[@"liveCode"] = [NSArray array];
		
		t_context[@"liveCode"][@"__invokeHandler"] = ^(NSString *p_handler, NSArray *p_args)
		{
			if ([m_js_handler_list containsObject: p_handler])
			{
				MCBrowserListRef t_args;
				t_args = nil;
				/* UNCHECKED */ MCNSArrayToBrowserList(p_args, t_args);
				
				OnJavaScriptCall([p_handler cStringUsingEncoding: NSUTF8StringEncoding], t_args);

				MCBrowserListRelease(t_args);
			}
		};
	}
	if (t_success)
	{
		for (id t_element in p_handlers)
		{
			NSString *t_js;
			t_js = [NSString stringWithFormat: @"javascript:liveCode.%@ = function() {liveCode.__invokeHandler('%@', Array.prototype.slice.call(arguments)); }", t_element, t_element];
			
			[t_context evaluateScript: t_js];
		}
	}
	
#endif
	
	return t_success;
}

bool MCUIWebViewBrowser::GetIsSecure(bool &r_value)
{
	r_value = false;
	return true;
}

bool MCUIWebViewBrowser::GetAllowUserInteraction(bool &r_value)
{
	UIWebView *t_view;
	if (!GetView(t_view))
		return false;

	MCBrowserRunBlockOnMainFiber(^{
		r_value = [t_view isUserInteractionEnabled] == YES;
	});

	return true;
}

bool MCUIWebViewBrowser::SetAllowUserInteraction(bool p_value)
{
	UIWebView *t_view;
	if (!GetView(t_view))
		return false;

	MCBrowserRunBlockOnMainFiber(^{
		[t_view setUserInteractionEnabled: (p_value) ? YES : NO];
	});

	return true;
}

//////////

bool MCUIWebViewBrowser::SetBoolProperty(MCBrowserProperty p_property, bool p_value)
{
	switch (p_property)
	{
		case kMCBrowserVerticalScrollbarEnabled:
			return SetVerticalScrollbarEnabled(p_value);
			
		case kMCBrowserHorizontalScrollbarEnabled:
			return SetHorizontalScrollbarEnabled(p_value);

		case kMCBrowserAllowUserInteraction:
			return SetAllowUserInteraction(p_value);

		default:
			break;
	}
	
	return true;
}

bool MCUIWebViewBrowser::GetBoolProperty(MCBrowserProperty p_property, bool &r_value)
{
	switch (p_property)
	{
		case kMCBrowserVerticalScrollbarEnabled:
			return GetVerticalScrollbarEnabled(r_value);
			
		case kMCBrowserHorizontalScrollbarEnabled:
			return GetHorizontalScrollbarEnabled(r_value);
			
		case kMCBrowserIsSecure:
			return GetIsSecure(r_value);

		case kMCBrowserAllowUserInteraction:
			return GetAllowUserInteraction(r_value);

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
			
		case kMCBrowserJavaScriptHandlers:
			return SetJavaScriptHandlers(p_utf8_string);
			
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
			
		case kMCBrowserJavaScriptHandlers:
			return GetJavaScriptHandlers(r_utf8_string);
			
		default:
			break;
	}
	
	return true;
}

bool MCUIWebViewBrowser::GetRect(MCBrowserRect &r_rect)
{
	UIWebView *t_view;
	if (!GetView(t_view))
		return false;
	
	MCBrowserRunBlockOnMainFiber(^{
		CGRect t_frame = [t_view frame];
		r_rect.left = t_frame.origin.x;
		r_rect.top = t_frame.origin.y;
		r_rect.right = t_frame.origin.x + t_frame.size.width;
		r_rect.bottom = t_frame.origin.y + t_frame.size.height;
	});
	
	return true;
}

bool MCUIWebViewBrowser::SetRect(const MCBrowserRect &p_rect)
{
	UIWebView *t_view;
	if (!GetView(t_view))
		return false;
	
	CGRect t_rect;
	t_rect = CGRectMake(p_rect.left, p_rect.top, p_rect.right - p_rect.left, p_rect.bottom - p_rect.top);
	
	MCBrowserRunBlockOnMainFiber(^{
		[t_view setFrame:t_rect];
	});

	return true;
}

// Browser-specific actions
bool MCUIWebViewBrowser::ExecStop()
{
	UIWebView *t_view;
	if (!GetView(t_view))
		return false;
	
	MCBrowserRunBlockOnMainFiber(^{
		[t_view stopLoading];
	});
	
	return true;
}

bool MCUIWebViewBrowser::GoForward()
{
	UIWebView *t_view;
	if (!GetView(t_view))
		return false;
	
	MCBrowserRunBlockOnMainFiber(^{
		[t_view goForward];
	});
	
	return true;
}

bool MCUIWebViewBrowser::GoBack()
{
	UIWebView *t_view;
	if (!GetView(t_view))
		return false;
	
	MCBrowserRunBlockOnMainFiber(^{
		[t_view goBack];
	});
	
	return true;
}

bool MCUIWebViewBrowser::ExecReload()
{
	UIWebView *t_view;
	if (!GetView(t_view))
		return false;
	
	MCBrowserRunBlockOnMainFiber(^{
		[t_view reload];
	});
	
	return true;
}

bool MCUIWebViewBrowser::EvaluateJavaScript(const char *p_script, char *&r_result)
{
	UIWebView *t_view;
	if (!GetView(t_view))
		return false;
	
	__block NSString *t_result;
	MCBrowserRunBlockOnMainFiber(^{
		t_result = [t_view stringByEvaluatingJavaScriptFromString: [NSString stringWithCString: p_script encoding:NSUTF8StringEncoding]];
	});
	
	if (t_result == nil)
		return false;

	return MCCStringClone([t_result cStringUsingEncoding:NSUTF8StringEncoding], r_result);
}

bool MCUIWebViewBrowser::ExecLoad(const char *p_url, const char *p_html)
{
	UIWebView *t_view;
	if (!GetView(t_view))
		return false;
	
	// MW-2012-10-01: [[ Bug 10422 ]] Make sure we mark a pending request so the
	//   HTML loading doesn't divert through a loadRequested message.

	MCBrowserRunBlockOnMainFiber(^{
		[m_delegate setPendingRequest: true];
		[t_view loadHTMLString: [NSString stringWithCString: p_html encoding:NSUTF8StringEncoding] baseURL: [NSURL URLWithString: [NSString stringWithCString: p_url encoding:NSUTF8StringEncoding]]];
	});
	
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
	
	__block UIScrollView *t_scroll;

	MCBrowserRunBlockOnMainFiber(^{
		t_scroll = [t_view scrollView];
	});
	
	return t_scroll;
}

bool MCUIWebViewBrowser::GetView(UIWebView *&r_view)
{
	if (m_view == nil)
		return false;
	
	r_view = m_view;
	return true;
}

void *MCUIWebViewBrowser::GetNativeLayer()
{
	UIWebView *t_view;
	if (!GetView(t_view))
		return nil;
	
	return t_view;
}

////////////////////////////////////////////////////////////////////////////////

bool MCUIWebViewBrowser::Init(void)
{
	__block bool t_success;
	t_success = true;
	
	MCBrowserRunBlockOnMainFiber(^{
		UIWebView *t_view;
		t_view = [[UIWebView alloc] initWithFrame: CGRectMake(0, 0, 0, 0)];
		t_success = t_view != nil;
		
		if (t_success)
		{
			[t_view setHidden: YES];
			
			m_delegate = [[com_runrev_livecode_MCUIWebViewBrowserDelegate alloc] initWithInstance: this];
			t_success = m_delegate != nil;
		}
		
		if (t_success)
		{
			[t_view setDelegate: m_delegate];
			m_view = t_view;
		}
		else if (t_view != nil)
			[t_view release];
	});
	
	return t_success;
}

////////////////////////////////////////////////////////////////////////////////

@implementation com_runrev_livecode_MCUIWebViewBrowserDelegate

- (id)initWithInstance:(MCUIWebViewBrowser*)instance
{
	self = [super init];
	if (self == nil)
		return nil;
	
	m_instance = instance;
	m_pending_request = false;
	m_request_url = nil;
	m_frame_request_url = nil;
	
	return self;
}

- (void)dealloc
{
	if (m_request_url != nil)
		MCCStringFree(m_request_url);
	if (m_frame_request_url != nil)
		MCCStringFree(m_frame_request_url);
	
	[super dealloc];
}

- (BOOL)webView:(UIWebView *)webView shouldStartLoadWithRequest:(NSURLRequest *)request navigationType:(UIWebViewNavigationType)navigationType
{
	if (m_pending_request)
	{
		m_pending_request = false;
		return YES;
	}
	
	bool t_frame_request = ![[request URL] isEqual: [request mainDocumentURL]];
	
	char *&t_url = t_frame_request ? m_frame_request_url : m_request_url;
	if (t_url != nil)
		MCCStringFree(t_url);
	t_url = nil;
	
	/* UNCHECKED */ MCCStringClone([request.URL.absoluteString cStringUsingEncoding: NSUTF8StringEncoding], t_url);
	
	if ([NSURLConnection canHandleRequest: request])
	{
		if (!t_frame_request)
			m_instance->OnNavigationBegin(false, t_url);

		return YES;
	}
	else
	{
		m_instance->OnNavigationRequestUnhandled(t_frame_request, t_url);
		
		return NO;
	}
}

- (void)webViewDidStartLoad:(UIWebView *)webView
{
    bool t_frame_request = ![[[webView request] URL] isEqual: [[webView request] mainDocumentURL]];
	char *t_url = t_frame_request ? m_frame_request_url : m_request_url;
	if (t_url == nil)
		return;
	
	m_instance->OnDocumentLoadBegin(t_frame_request, t_url);
}

- (void)webViewDidFinishLoad:(UIWebView *)webView
{
    bool t_frame_request = ![[[webView request] URL] isEqual: [[webView request] mainDocumentURL]];
	if (!t_frame_request)
		m_instance->AttachJSHandlers();
	
	char *&t_url = t_frame_request ? m_frame_request_url : m_request_url;
	if (t_url == nil)
		return;

	m_instance->OnDocumentLoadComplete(t_frame_request, t_url);
	if (!t_frame_request)
		m_instance->OnNavigationComplete(false, t_url);

	MCCStringFree(t_url);
	t_url = nil;
}
- (void)webView:(UIWebView *)webView didFailLoadWithError:(NSError *)error
{
    bool t_frame_request = ![[[webView request] URL] isEqual: [[webView request] mainDocumentURL]];
	char *&t_url = t_frame_request ? m_frame_request_url : m_request_url;
	if (t_url == nil)
		return;

	const char *t_error;
	t_error = [[error localizedDescription] cStringUsingEncoding:NSUTF8StringEncoding];

	m_instance->OnDocumentLoadFailed(t_frame_request, t_frame_request ? m_frame_request_url : m_request_url, t_error);
	if (!t_frame_request)
		m_instance->OnNavigationFailed(false, m_request_url, t_error);

	MCCStringFree(t_url);
	t_url = nil;
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
