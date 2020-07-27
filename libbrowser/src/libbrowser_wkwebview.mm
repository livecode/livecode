/* Copyright (C) 2019 LiveCode Ltd.
 
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
#import <WebKit/WebKit.h>

#include <core.h>

#include "libbrowser_internal.h"
#include "libbrowser_wkwebview.h"
#include "libbrowser_nsvalue.h"

////////////////////////////////////////////////////////////////////////////////

#define LIBBROWSER_DUMMY_URL "http://libbrowser_dummy_url/"

////////////////////////////////////////////////////////////////////////////////

typedef void (^MCWKNavigationDecisionHandler)(WKNavigationActionPolicy);
@class com_livecode_libbrowser_MCWKWebViewNavigationDelegate;

class MCWKWebViewBrowserNavigationRequest : public MCBrowserNavigationRequestBase
{
public:
	MCWKWebViewBrowserNavigationRequest(WKNavigationAction *p_navigation_action, bool p_quiet, MCWKNavigationDecisionHandler p_decision_handler, com_livecode_libbrowser_MCWKWebViewNavigationDelegate *p_delegate);
	virtual ~MCWKWebViewBrowserNavigationRequest();
	void Continue(void);
	void Cancel(void);
	
	bool IsQuiet(void);
	
private:
	bool m_quiet;
	MCWKNavigationDecisionHandler m_decision_handler;
	com_livecode_libbrowser_MCWKWebViewNavigationDelegate *m_delegate;
};

@interface com_livecode_libbrowser_MCWKWebViewLoadRequest : NSObject
{
}

@property (retain) NSURL* url;
@property (retain) NSString* htmlText;
@property (assign) BOOL quiet;

- (id)initWithUrl:(NSURL *)url htmlText:(NSString *)htmlText quiet:(BOOL)quiet;
+ (id)requestWithUrl: (NSURL*)url htmlText:(NSString *)htmlText quiet:(BOOL)quiet;

@end

@compatibility_alias MCWKWebViewLoadRequest com_livecode_libbrowser_MCWKWebViewLoadRequest;

@implementation com_livecode_libbrowser_MCWKWebViewLoadRequest

@synthesize url;
@synthesize htmlText;

+ (id)requestWithUrl:(NSURL *)url htmlText:(NSString *)htmlText quiet:(BOOL)quiet
{
	MCWKWebViewLoadRequest *t_request = [[MCWKWebViewLoadRequest alloc] autorelease];
	if (t_request == nil)
		return nil;
	
	return [t_request initWithUrl:url htmlText:htmlText quiet:quiet];
}

- (id)initWithUrl:(NSURL *)p_url htmlText:(NSString *)p_htmlText quiet:(BOOL)p_quiet
{
	self = [super init];
	if (self == nil)
		return nil;
	
	self.url = p_url;
	self.htmlText = p_htmlText;
	self.quiet = p_quiet;
	
	return self;
}

- (void)dealloc
{
	self.url = nil;
	self.htmlText = nil;
	
	[super dealloc];
}
@end

@interface com_livecode_libbrowser_MCWKWebViewNavigationDelegate : NSObject <WKNavigationDelegate>
{
	MCWKWebViewBrowser *m_instance;
	NSMutableArray<NSValue*> *m_frame_requests;
	MCWKWebViewLoadRequest *m_pending_request;
	MCWKWebViewLoadRequest *m_current_request;
	MCWKWebViewLoadRequest *m_queued_request;
	bool m_can_start_load;
	bool m_delay_requests;
}

- (id)initWithInstance:(MCWKWebViewBrowser*)instance;
- (void)dealloc;

- (void)webView:(WKWebView *)webView didCommitNavigation:(WKNavigation *)navigation;
- (void)webView:(WKWebView *)webView didStartProvisionalNavigation:(WKNavigation *)navigation;
- (void)webView:(WKWebView *)webView didFinishNavigation:(WKNavigation *)navigation;
- (void)webView:(WKWebView *)webView didFailProvisionalNavigation:(WKNavigation *)navigation withError:(NSError *)error;
- (void)webView:(WKWebView *)webView didFailNavigation:(WKNavigation *)navigation withError:(NSError *)error;
- (void)webView:(WKWebView *)webView decidePolicyForNavigationAction:(WKNavigationAction *)navigationAction decisionHandler:(void (^)(WKNavigationActionPolicy))decisionHandler;

- (void)setDelayRequests: (bool)newValue;
- (bool)getDelayRequests;
- (void)continueRequest: (MCWKWebViewBrowserNavigationRequest *)request withDecisionHandler:(MCWKNavigationDecisionHandler)decisionHandler;
- (void)cancelRequest: (MCWKWebViewBrowserNavigationRequest *)request withDecisionHandler:(MCWKNavigationDecisionHandler)decisionHandler;

- (void)loadRequest: (MCWKWebViewLoadRequest*)request inWebView:(WKWebView *)webView;
@end

////////////////////////////////////////////////////////////////////////////////

#if defined(TARGET_SUBPLATFORM_IPHONE)
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

WKDataDetectorTypes MCBrowserDataDetectorTypeToWKDataDetectorTypes(MCBrowserDataDetectorType p_type)
{
	WKDataDetectorTypes t_types;
	t_types = WKDataDetectorTypeNone;
	
	if (p_type & kMCBrowserDataDetectorTypePhoneNumber)
		t_types |= WKDataDetectorTypePhoneNumber;
	if (p_type & kMCBrowserDataDetectorTypeMapAddress)
		t_types |= WKDataDetectorTypeAddress;
	if (p_type & kMCBrowserDataDetectorTypeLink)
		t_types |= WKDataDetectorTypeLink;
	if (p_type & kMCBrowserDataDetectorTypeEmailAddress)
		t_types |= WKDataDetectorTypeLink;
	if (p_type & kMCBrowserDataDetectorTypeCalendarEvent)
		t_types |= WKDataDetectorTypeCalendarEvent;
	if (p_type & kMCBrowserDataDetectorTypeFlightNumber)
		t_types |= WKDataDetectorTypeFlightNumber;
	if (p_type & kMCBrowserDataDetectorTypeLookupSuggestion)
		t_types |= WKDataDetectorTypeLookupSuggestion;
	
	return t_types;
}

MCBrowserDataDetectorType MCBrowserDataDetectorTypeFromWKDataDetectorTypes(WKDataDetectorTypes p_types)
{
	int32_t t_types;
	t_types = kMCBrowserDataDetectorTypeNone;
	
	if (p_types & WKDataDetectorTypePhoneNumber)
		t_types |= kMCBrowserDataDetectorTypePhoneNumber;
	if (p_types & WKDataDetectorTypeAddress)
		t_types |= kMCBrowserDataDetectorTypeMapAddress;
	if (p_types & WKDataDetectorTypeLink)
		t_types |= kMCBrowserDataDetectorTypeLink | kMCBrowserDataDetectorTypeEmailAddress;
	if (p_types & WKDataDetectorTypeCalendarEvent)
		t_types |= kMCBrowserDataDetectorTypeCalendarEvent;
	if (p_types & WKDataDetectorTypeFlightNumber)
		t_types |= kMCBrowserDataDetectorTypeFlightNumber;
	if (p_types & WKDataDetectorTypeLookupSuggestion)
		t_types |= kMCBrowserDataDetectorTypeLookupSuggestion;

	return (MCBrowserDataDetectorType)t_types;
}

////////////////////////////////////////////////////////////////////////////////

MCWKWebViewBrowser::MCWKWebViewBrowser()
{
	m_view = nil;
	m_container_view = nil;
	m_delegate = nil;
	m_message_handler = nil;
	m_content_controller = nil;
	m_configuration = nil;

	m_url = nil;
	m_htmltext = nil;
	m_js_handlers = nil;

	m_autofit = false;
}

MCWKWebViewBrowser::~MCWKWebViewBrowser()
{
	MCBrowserRunBlockOnMainFiber(^{
		if (m_view != nil)
		{
			[m_view setNavigationDelegate: nil];
			[m_view removeFromSuperview];
			[m_view release];
		}
		
		if (m_container_view != nil)
			[m_container_view release];
		
		if (m_delegate != nil)
			[m_delegate release];
		
		if (m_message_handler != nil)
			[m_message_handler release];
		
		if (m_content_controller != nil)
			[m_content_controller release];
		
		if (m_configuration != nil)
			[m_configuration release];
		
		MCBrowserCStringAssign(m_url, nil);
		MCBrowserCStringAssign(m_htmltext, nil);
		MCBrowserCStringAssign(m_js_handlers, nil);
	});
}

////////////////////////////////////////////////////////////////////////////////

bool MCWKWebViewBrowser::GetUrl(char *&r_url)
{
	WKWebView *t_view;
	if (!GetWebView(t_view))
		return false;
	
	__block bool t_success;
	MCBrowserRunBlockOnMainFiber(^{
		t_success = MCCStringClone([[[t_view URL] absoluteString] UTF8String], r_url);
	});
	
	return t_success;
}

////////////////////////////////////////////////////////////////////////////////

bool MCWKWebViewBrowser::SetVerticalScrollbarEnabled(bool p_value)
{
	WKWebView *t_view;
	if (!GetWebView(t_view))
		return false;
	
	MCBrowserRunBlockOnMainFiber(^{
		t_view.scrollView.showsVerticalScrollIndicator = p_value;
	});
	
	return true;
}

bool MCWKWebViewBrowser::GetVerticalScrollbarEnabled(bool& r_value)
{
	WKWebView *t_view;
	if (!GetWebView(t_view))
		return false;
	
	MCBrowserRunBlockOnMainFiber(^{
		r_value = t_view.scrollView.showsVerticalScrollIndicator;
	});
	
	return true;
}

bool MCWKWebViewBrowser::SetHorizontalScrollbarEnabled(bool p_value)
{
	WKWebView *t_view;
	if (!GetWebView(t_view))
		return false;
	
	MCBrowserRunBlockOnMainFiber(^{
		t_view.scrollView.showsHorizontalScrollIndicator = p_value;
	});
	
	return true;
}

bool MCWKWebViewBrowser::GetHorizontalScrollbarEnabled(bool& r_value)
{
	WKWebView *t_view;
	if (!GetWebView(t_view))
		return false;
	
	MCBrowserRunBlockOnMainFiber(^{
		r_value = t_view.scrollView.showsHorizontalScrollIndicator;
	});
	
	return true;
}

bool MCWKWebViewBrowser::SetScrollEnabled(bool p_value)
{
	WKWebView *t_view;
	if (!GetWebView(t_view))
		return false;
	
	MCBrowserRunBlockOnMainFiber(^{
		t_view.scrollView.scrollEnabled = p_value;
	});
	
	return true;
}

bool MCWKWebViewBrowser::GetScrollEnabled(bool& r_value)
{
	WKWebView *t_view;
	if (!GetWebView(t_view))
		return false;
	
	MCBrowserRunBlockOnMainFiber(^{
		r_value = t_view.scrollView.scrollEnabled;
	});
	
	return true;
}

bool MCWKWebViewBrowser::SetScrollCanBounce(bool p_value)
{
	WKWebView *t_view;
	if (!GetWebView(t_view))
		return false;
	
	MCBrowserRunBlockOnMainFiber(^{
		t_view.scrollView.bounces = p_value;
	});
	
	return true;
}

bool MCWKWebViewBrowser::GetScrollCanBounce(bool& r_value)
{
	WKWebView *t_view;
	if (!GetWebView(t_view))
		return false;
	
	MCBrowserRunBlockOnMainFiber(^{
		r_value = t_view.scrollView.bounces;
	});
	
	return true;
}

////////////////////////////////////////////////////////////////////////////////

bool MCWKWebViewBrowser::GetHTMLText(char *&r_htmltext)
{
	return EvaluateJavaScript("document.documentElement.outerHTML", r_htmltext);
}

bool MCWKWebViewBrowser::LoadHTMLText(const char *p_htmltext, const char *p_baseurl)
{
	WKWebView *t_view;
	if (!GetWebView(t_view))
		return false;

	NSURL *t_url = [NSURL URLWithString: [NSString stringWithUTF8String: p_baseurl]];
	NSString *t_htmltext = [NSString stringWithUTF8String: p_htmltext];
	[m_delegate loadRequest:[MCWKWebViewLoadRequest requestWithUrl:t_url htmlText:t_htmltext quiet:YES] inWebView:t_view];
	
	/* UNCHECKED */ MCBrowserCStringAssignCopy(m_htmltext, p_htmltext);
	/* UNCHECKED */ MCBrowserCStringAssignCopy(m_url, p_baseurl);

	return true;
}

bool MCWKWebViewBrowser::SetHTMLText(const char *p_htmltext)
{
	return LoadHTMLText(p_htmltext, LIBBROWSER_DUMMY_URL);
}

////////////////////////////////////////////////////////////////////////////////

bool MCWKWebViewBrowser::GetJavaScriptHandlers(char *&r_handlers)
{
	return MCCStringClone(m_js_handlers ? m_js_handlers : "", r_handlers);
}

bool MCWKWebViewBrowser::SetJavaScriptHandlers(const char *p_handlers)
{
	char *t_handlers;
	t_handlers = nil;
	
	if (!MCCStringIsEmpty(p_handlers) && !MCCStringClone(p_handlers, t_handlers))
		return false;
	
	MCBrowserCStringAssign(m_js_handlers, t_handlers);
	
	SyncJavaScriptHandlers();
	return true;
}

@interface MCWKWebViewScriptMessageHandler : NSObject <WKScriptMessageHandler>
{
	MCWKWebViewBrowser *m_instance;
}

@property (retain) NSArray* jsHandlers;

- (id)initWithInstance:(MCWKWebViewBrowser *)instance;
- (void)dealloc;

- (void)userContentController:(WKUserContentController *)userContentController didReceiveScriptMessage:(WKScriptMessage *)message;

@end

@implementation MCWKWebViewScriptMessageHandler

@synthesize jsHandlers;

- (id)initWithInstance:(MCWKWebViewBrowser *)instance
{
	self = [super init];
	if (self == nil)
		return nil;
	
	m_instance = instance;
	
	return self;
}

- (void)dealloc
{
	self.jsHandlers = nil;
	
	[super dealloc];
}

- (void)userContentController:(WKUserContentController *)userContentController didReceiveScriptMessage:(WKScriptMessage *)message
{
	NSString *name;
	name = [message name];

	NSArray *arguments;
	arguments = [message body];
	
	if (![arguments isKindOfClass:[NSArray class]] || [arguments count] != 2)
		return;
	
	NSArray *t_message;
	t_message = arguments;
	
	if ([t_message count] != 2)
		return;
	
	NSString *t_name;
	t_name = [t_message objectAtIndex:0];
	
	NSArray *t_arguments;
	t_arguments = [t_message objectAtIndex:1];
	
	if (![t_name isKindOfClass:[NSString class]] || ![t_arguments isKindOfClass:[NSArray class]])
		return;
	
	if ([self.jsHandlers containsObject: t_name])
	{
		MCBrowserListRef t_args;
		t_args = nil;
		/* UNCHECKED */ MCNSArrayToBrowserList(t_arguments, t_args);
		
		m_instance->OnJavaScriptCall([t_name UTF8String], t_args);
		
		MCBrowserListRelease(t_args);
	}
}

@end

void MCWKWebViewBrowser::SyncJavaScriptHandlers()
{
	MCBrowserRunBlockOnMainFiber(^{
		NSArray *t_js_handlers;
		t_js_handlers = nil;
		if (m_js_handlers != nil)
			t_js_handlers = [[NSString stringWithUTF8String:m_js_handlers] componentsSeparatedByString: @"\n"];
		SyncJavaScriptHandlers(t_js_handlers);
	});
}

bool MCWKWebViewBrowser::SyncJavaScriptHandlers(NSArray *p_handlers)
{
	bool t_success;
	t_success = true;
	
	m_message_handler.jsHandlers = p_handlers;
	
	if (t_success)
	{
		char *t_result;
		t_result = nil;
		
		[m_view evaluateJavaScript:@"window.liveCode={};" completionHandler:nil];
		
		for (id t_element in p_handlers)
		{
			NSString *t_js;
			t_js = [NSString stringWithFormat: @"window.liveCode.%@ = function() {window.webkit.messageHandlers.liveCode.postMessage(['%@', Array.prototype.slice.call(arguments)]); }", t_element, t_element];
			
			[m_view evaluateJavaScript:t_js completionHandler:nil];
		}
	}
	
	return t_success;
}

////////////////////////////////////////////////////////////////////////////////

bool MCWKWebViewBrowser::GetUserAgent(char *&r_useragent)
{
	WKWebView *t_view;
	if (!GetWebView(t_view))
		return false;
	
	__block bool t_success;
	MCBrowserRunBlockOnMainFiber(^{
		NSString *t_custom_useragent;
		t_custom_useragent = [t_view customUserAgent];
		t_success = MCCStringClone(t_custom_useragent != nil ? [t_custom_useragent UTF8String] : "", r_useragent);
	});
	
	return t_success;
}

bool MCWKWebViewBrowser::SetUserAgent(const char *p_useragent)
{
	WKWebView *t_view;
	if (!GetWebView(t_view))
		return false;
	
	NSString *t_custom_useragent;
	t_custom_useragent = MCCStringIsEmpty(p_useragent) ? nil : [NSString stringWithUTF8String:p_useragent];

	MCBrowserRunBlockOnMainFiber(^{
		[t_view setCustomUserAgent:t_custom_useragent];
	});
	
	return true;
}

////////////////////////////////////////////////////////////////////////////////

bool MCWKWebViewBrowser::GetIsSecure(bool &r_value)
{
	WKWebView *t_view;
	if (!GetWebView(t_view))
		return false;
	
	MCBrowserRunBlockOnMainFiber(^{
		r_value = t_view.hasOnlySecureContent;
	});

	return true;
}

////////////////////////////////////////////////////////////////////////////////

bool MCWKWebViewBrowser::GetCanGoForward(bool &r_value)
{
	WKWebView *t_view;
	if (!GetWebView(t_view))
		return false;
	
	MCBrowserRunBlockOnMainFiber(^{
		r_value = t_view.canGoForward;
	});

	return true;
}

bool MCWKWebViewBrowser::GetCanGoBack(bool &r_value)
{
	WKWebView *t_view;
	if (!GetWebView(t_view))
		return false;
	
	MCBrowserRunBlockOnMainFiber(^{
		r_value = t_view.canGoBack;
	});

	return true;
}

////////////////////////////////////////////////////////////////////////////////

bool MCWKWebViewBrowser::GetAllowUserInteraction(bool &r_value)
{
	WKWebView *t_view;
	if (!GetWebView(t_view))
		return false;

	MCBrowserRunBlockOnMainFiber(^{
		r_value = [t_view isUserInteractionEnabled] == YES;
	});

	return true;
}

bool MCWKWebViewBrowser::SetAllowUserInteraction(bool p_value)
{
	WKWebView *t_view;
	if (!GetWebView(t_view))
		return false;

	MCBrowserRunBlockOnMainFiber(^{
		t_view.userInteractionEnabled = p_value;
	});

	return true;
}

////////////////////////////////////////////////////////////////////////////////

bool MCWKWebViewBrowser::GetDelayRequests(bool &r_value)
{
	if (m_delegate == nil)
		return false;
	
	r_value = [m_delegate getDelayRequests];
	return true;
}

bool MCWKWebViewBrowser::SetDelayRequests(bool p_value)
{
	if (m_delegate == nil)
		return false;
	
	[m_delegate setDelayRequests:p_value];
	
	return true;
}

////////////////////////////////////////////////////////////////////////////////

bool MCWKWebViewBrowser::GetDataDetectorTypes(int32_t &r_types)
{
	r_types = MCBrowserDataDetectorTypeFromWKDataDetectorTypes(m_configuration.dataDetectorTypes);
	return true;
}

bool MCWKWebViewBrowser::SetDataDetectorTypes(int32_t p_types)
{
	WKDataDetectorTypes t_types;
	t_types = MCBrowserDataDetectorTypeToWKDataDetectorTypes((MCBrowserDataDetectorType)p_types);
	
	if (t_types == m_configuration.dataDetectorTypes)
		return true;
	
	m_configuration.dataDetectorTypes = t_types;
	return Reconfigure();
}

////////////////////////////////////////////////////////////////////////////////

bool MCWKWebViewBrowser::GetAllowsInlineMediaPlayback(bool &r_value)
{
	r_value = m_configuration.allowsInlineMediaPlayback;
	return true;
}

bool MCWKWebViewBrowser::SetAllowsInlineMediaPlayback(bool p_value)
{
	if (p_value == m_configuration.allowsInlineMediaPlayback)
		return true;
	
	m_configuration.allowsInlineMediaPlayback = p_value;
	return Reconfigure();
}

////////////////////////////////////////////////////////////////////////////////

bool MCWKWebViewBrowser::GetMediaPlaybackRequiresUserAction(bool &r_value)
{
	r_value = m_configuration.mediaPlaybackRequiresUserAction;
	return true;
}

bool MCWKWebViewBrowser::SetMediaPlaybackRequiresUserAction(bool p_value)
{
	if (p_value == m_configuration.mediaPlaybackRequiresUserAction)
		return true;
	
	m_configuration.mediaPlaybackRequiresUserAction = p_value;
	return Reconfigure();
}

////////////////////////////////////////////////////////////////////////////////

bool MCWKWebViewBrowser::GetAutoFit(bool &r_value)
{
	r_value = m_autofit;
	return true;
}

bool MCWKWebViewBrowser::SetAutoFit(bool p_value)
{
	if (p_value == m_autofit)
		return true;
	
	if (p_value)
	{
		NSString *t_script_string;
		t_script_string = @"var meta = document.createElement('meta'); meta.setAttribute('name', 'viewport'); meta.setAttribute('content', 'width=device-width'); document.getElementsByTagName('head')[0].appendChild(meta);";

		WKUserScript *t_script;
		t_script = [[WKUserScript alloc] initWithSource:t_script_string injectionTime:WKUserScriptInjectionTimeAtDocumentEnd forMainFrameOnly:YES];
		
		[m_content_controller addUserScript:t_script];
		[t_script release];
	}
	else
		[m_content_controller removeAllUserScripts];
	
	return true;
}

////////////////////////////////////////////////////////////////////////////////

bool MCWKWebViewBrowser::SetBoolProperty(MCBrowserProperty p_property, bool p_value)
{
	switch (p_property)
	{
		case kMCBrowserVerticalScrollbarEnabled:
			return SetVerticalScrollbarEnabled(p_value);
			
		case kMCBrowserHorizontalScrollbarEnabled:
			return SetHorizontalScrollbarEnabled(p_value);

		case kMCBrowserScrollEnabled:
			return SetScrollEnabled(p_value);
			
		case kMCBrowserScrollCanBounce:
			return SetScrollCanBounce(p_value);
			
		case kMCBrowserAllowUserInteraction:
			return SetAllowUserInteraction(p_value);

		case kMCBrowseriOSDelayRequests:
			return SetDelayRequests(p_value);
			
		case kMCBrowseriOSAllowsInlineMediaPlayback:
			return SetAllowsInlineMediaPlayback(p_value);
		
		case kMCBrowseriOSMediaPlaybackRequiresUserAction:
			return SetMediaPlaybackRequiresUserAction(p_value);
			
		default:
			break;
	}
	
	return true;
}

bool MCWKWebViewBrowser::GetBoolProperty(MCBrowserProperty p_property, bool &r_value)
{
	switch (p_property)
	{
		case kMCBrowserVerticalScrollbarEnabled:
			return GetVerticalScrollbarEnabled(r_value);
			
		case kMCBrowserHorizontalScrollbarEnabled:
			return GetHorizontalScrollbarEnabled(r_value);
			
		case kMCBrowserScrollEnabled:
			return GetScrollEnabled(r_value);
			
		case kMCBrowserScrollCanBounce:
			return GetScrollCanBounce(r_value);
			
		case kMCBrowserIsSecure:
			return GetIsSecure(r_value);

		case kMCBrowserCanGoForward:
			return GetCanGoForward(r_value);
		
		case kMCBrowserCanGoBack:
			return GetCanGoBack(r_value);
		
		case kMCBrowserAllowUserInteraction:
			return GetAllowUserInteraction(r_value);

		case kMCBrowseriOSDelayRequests:
			return GetDelayRequests(r_value);
		
		case kMCBrowseriOSAllowsInlineMediaPlayback:
			return GetAllowsInlineMediaPlayback(r_value);
		
		case kMCBrowseriOSMediaPlaybackRequiresUserAction:
			return GetMediaPlaybackRequiresUserAction(r_value);
			
		default:
			break;
	}
	
	return true;
}

bool MCWKWebViewBrowser::SetStringProperty(MCBrowserProperty p_property, const char *p_utf8_string)
{
	switch (p_property)
	{
		case kMCBrowserHTMLText:
			return SetHTMLText(p_utf8_string);
			
		case kMCBrowserJavaScriptHandlers:
			return SetJavaScriptHandlers(p_utf8_string);
			
		case kMCBrowserUserAgent:
			return SetUserAgent(p_utf8_string);
			
		default:
			break;
	}
	
	return true;
}

bool MCWKWebViewBrowser::GetStringProperty(MCBrowserProperty p_property, char *&r_utf8_string)
{
	switch (p_property)
	{
		case kMCBrowserURL:
			return GetUrl(r_utf8_string);
			
		case kMCBrowserHTMLText:
			return GetHTMLText(r_utf8_string);
			
		case kMCBrowserJavaScriptHandlers:
			return GetJavaScriptHandlers(r_utf8_string);
			
		case kMCBrowserUserAgent:
			return GetUserAgent(r_utf8_string);
			
		default:
			break;
	}
	
	return true;
}

bool MCWKWebViewBrowser::SetIntegerProperty(MCBrowserProperty p_property, int32_t p_value)
{
	switch (p_property)
	{
		case kMCBrowserDataDetectorTypes:
			return SetDataDetectorTypes(p_value);
			
		default:
			break;
	}
	
	return true;
}

bool MCWKWebViewBrowser::GetIntegerProperty(MCBrowserProperty p_property, int32_t &r_value)
{
	switch (p_property)
	{
		case kMCBrowserDataDetectorTypes:
			return GetDataDetectorTypes(r_value);
			
		default:
			break;
	}
	
	return true;
}

////////////////////////////////////////////////////////////////////////////////

bool MCWKWebViewBrowser::GetRect(MCBrowserRect &r_rect)
{
	UIView *t_view;
	if (!GetContainerView(t_view))
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

bool MCWKWebViewBrowser::SetRect(const MCBrowserRect &p_rect)
{
	UIView *t_view;
	if (!GetContainerView(t_view))
		return false;
	
	CGRect t_rect;
	t_rect = CGRectMake(p_rect.left, p_rect.top, p_rect.right - p_rect.left, p_rect.bottom - p_rect.top);
	
	MCBrowserRunBlockOnMainFiber(^{
		[t_view setFrame:t_rect];
	});

	return true;
}

////////////////////////////////////////////////////////////////////////////////

bool MCWKWebViewBrowser::GoToURL(const char *p_url)
{
	MCLog("MCWKWebViewBrowser::GoToURL(%s)", p_url);
	WKWebView *t_view;
	if (!GetWebView(t_view))
		return false;
	
	NSURL *t_url;
	if (MCCStringIsEmpty(p_url))
	{
		// Sending an empty url to the webview will bypass the delegate's
		// decidePolicyForNavigationAction method. We can prevent that by
		// loading a blank page instead.
		[m_delegate loadRequest:[MCWKWebViewLoadRequest requestWithUrl:nil htmlText:@"<html><body></body></html>" quiet:YES] inWebView:t_view];
	}
	else
	{
		t_url = [NSURL URLWithString: [NSString stringWithUTF8String: p_url]];

		// reject file urls with empty path components
		if (t_url.fileURL && t_url.path.length == 0)
			return false;

		// check that we have permission to access file urls before proceeding
		__block bool t_reachable = true;
		if (t_url.fileURL)
		{
			MCBrowserRunBlockOnMainFiber(^{
				t_reachable = [t_url checkResourceIsReachableAndReturnError:nil];
			});
		}
		if (!t_reachable)
			return false;
		
		[m_delegate loadRequest:[MCWKWebViewLoadRequest requestWithUrl:t_url htmlText:nil quiet:NO] inWebView:t_view];
	}
	
	/* UNCHECKED */ MCBrowserCStringAssignCopy(m_url, p_url);
	MCBrowserCStringAssign(m_htmltext, nil);
	
	return true;
}

bool MCWKWebViewBrowser::StopLoading()
{
	WKWebView *t_view;
	if (!GetWebView(t_view))
		return false;
	
	MCBrowserRunBlockOnMainFiber(^{
		[t_view stopLoading];
	});
	
	return true;
}

bool MCWKWebViewBrowser::Reload()
{
	WKWebView *t_view;
	if (!GetWebView(t_view))
		return false;
	
	MCBrowserRunBlockOnMainFiber(^{
		[t_view reload];
	});
	
	return true;
}

bool MCWKWebViewBrowser::GoForward()
{
	WKWebView *t_view;
	if (!GetWebView(t_view))
		return false;
	
	MCBrowserRunBlockOnMainFiber(^{
		[t_view goForward];
	});
	
	return true;
}

bool MCWKWebViewBrowser::GoBack()
{
	WKWebView *t_view;
	if (!GetWebView(t_view))
		return false;
	
	MCBrowserRunBlockOnMainFiber(^{
		[t_view goBack];
	});
	
	return true;
}

////////////////////////////////////////////////////////////////////////////////

void _SyncEvaluateJavaScript(WKWebView *p_view, NSString *p_javascript, id &r_result, NSError *&r_error)
{
	__block bool t_evaluating;
	t_evaluating = false;
	__block id t_result;
	t_result = nil;
	__block NSError *t_error;
	t_error = nil;
	
	t_evaluating = true;
	MCBrowserRunBlockOnMainFiber(^{
		[ p_view evaluateJavaScript:p_javascript completionHandler:^(id result, NSError *error) {
			if (error != nil)
			{
				t_result = nil;
				t_error = [error retain];
			}
			else
			{
				t_result = [result retain];
				t_error = nil;
			}
			
			t_evaluating = false;
		}];
	});
	
	while (t_evaluating)
		MCBrowserRunloopWait();
	
	r_result = t_result;
	r_error = t_error;
}

bool MCWKWebViewBrowser::EvaluateJavaScript(const char *p_script, char *&r_result)
{
	WKWebView *t_view;
	if (!GetWebView(t_view))
		return false;
	
	id t_result = nil;
	NSError *t_error = nil;
	
	_SyncEvaluateJavaScript(t_view, [NSString stringWithUTF8String:p_script], t_result, t_error);
	
	if (t_error != nil)
	{
		[t_error release];
		return false;
	}
	
	if (t_result == nil)
	{
		// return the empty string if there is no result value
		return MCCStringClone("", r_result);
	}
	
	NSString *t_result_string = [NSString stringWithFormat:@"%@", t_result];
	[t_result release];
	
	return MCCStringClone([t_result_string UTF8String], r_result);
}

////////////////////////////////////////////////////////////////////////////////

bool MCWKWebViewBrowser::GetWebView(WKWebView *&r_view)
{
	if (m_view == nil)
		return false;
	
	r_view = m_view;
	return true;
}

bool MCWKWebViewBrowser::GetContainerView(UIView *&r_view)
{
	if (m_container_view == nil)
		return false;
	
	r_view = m_container_view;
	return true;
}

void *MCWKWebViewBrowser::GetNativeLayer()
{
	UIView *t_view;
	if (!GetContainerView(t_view))
		return nil;
	
	return t_view;
}

////////////////////////////////////////////////////////////////////////////////

bool MCWKWebViewBrowser::Init(void)
{
	__block bool t_success;
	t_success = true;
	
	MCBrowserRunBlockOnMainFiber(^{
		UIView *t_container_view;
		t_container_view = nil;
		
		if (t_success)
		{
			t_container_view = [[UIView alloc] initWithFrame:CGRectMake(0, 0, 0, 0)];
			t_success = t_container_view != nil;
		}
	
		MCWKWebViewScriptMessageHandler *t_message_handler;
		t_message_handler = nil;
		if (t_success)
		{
			t_message_handler = [[MCWKWebViewScriptMessageHandler alloc] initWithInstance:this];
			t_success = t_message_handler != nil;
		}
		
		com_livecode_libbrowser_MCWKWebViewNavigationDelegate *t_delegate;
		t_delegate = nil;
		if (t_success)
		{
			t_delegate = [[com_livecode_libbrowser_MCWKWebViewNavigationDelegate alloc] initWithInstance:this];
			t_success = t_delegate != nil;
		}
		
		WKUserContentController *t_content_controller;
		t_content_controller = nil;
		if (t_success)
		{
			t_content_controller = [[WKUserContentController alloc] init];
			t_success = t_content_controller != nil;
		}

		WKWebViewConfiguration *t_config;
		t_config = nil;
		if (t_success)
		{
			t_config = [[WKWebViewConfiguration alloc] init];
			t_success = t_config != nil;
		}

		if (t_success)
		{
			[t_content_controller addScriptMessageHandler:t_message_handler name:@"liveCode"];
			[t_config setUserContentController:t_content_controller];
		}

		WKWebView *t_view;
		t_view = nil;
		if (t_success)
		{
			t_view = [[WKWebView alloc] initWithFrame:CGRectMake(0, 0, 0, 0) configuration:t_config];
			t_success = t_view != nil;
		}
		
		if (t_success)
		{
			[t_view setNavigationDelegate: t_delegate];
			[t_view setHidden: NO];
			[t_view setAutoresizingMask:UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight];

			[t_container_view setAutoresizesSubviews:YES];
			[t_container_view addSubview:t_view];
			[t_container_view setHidden: YES];

			m_view = t_view;
			m_container_view = t_container_view;
			m_delegate = t_delegate;
			m_message_handler = t_message_handler;
			m_content_controller = t_content_controller;
			m_configuration = t_config;
		}
		else
		{
		 	if (t_view != nil)
				[t_view release];
			if (t_container_view != nil)
				[t_container_view release];
			if (t_delegate != nil)
				[t_delegate release];
			if (t_message_handler != nil)
				[t_message_handler release];
			if (t_content_controller != nil)
				[t_content_controller release];
			if (t_config != nil)
				[t_config release];
		}
	});
	
	return t_success;
}

bool MCWKWebViewBrowser::Reconfigure()
{
	__block bool t_success;
	t_success = true;
	
	if (m_view == nil || m_container_view == nil)
		return false;
	
	MCBrowserRunBlockOnMainFiber(^{
		/* Save webview properties */
		bool t_vertical_scrollbar_enabled;
		bool t_horizontal_scrollbar_enabled;
		bool t_scroll_enabled;
		bool t_scroll_can_bounce;
		bool t_allow_user_interaction;
		t_vertical_scrollbar_enabled = m_view.scrollView.showsVerticalScrollIndicator;
		t_horizontal_scrollbar_enabled = m_view.scrollView.showsHorizontalScrollIndicator;
		t_scroll_enabled = m_view.scrollView.scrollEnabled;
		t_scroll_can_bounce = m_view.scrollView.bounces;
		t_allow_user_interaction = m_view.userInteractionEnabled;
		
		NSString *t_useragent;
		t_useragent = m_view.customUserAgent;
		
		WKWebView *t_view;
		t_view = nil;
		if (t_success)
		{
			t_view = [[WKWebView alloc] initWithFrame:m_container_view.bounds configuration:m_configuration];
			t_success = t_view != nil;
		}
		
		if (t_success)
		{
			// release current webview
			[m_view removeFromSuperview];
			[m_view release];
			m_view = nil;
			
			[t_view setNavigationDelegate: m_delegate];
			[t_view setHidden: NO];
			[t_view setAutoresizingMask:UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight];

			[m_container_view addSubview:t_view];
			
			m_view = t_view;
			
			m_view.customUserAgent = t_useragent;
			
			m_view.scrollView.showsVerticalScrollIndicator = t_vertical_scrollbar_enabled;
			m_view.scrollView.showsHorizontalScrollIndicator = t_horizontal_scrollbar_enabled;
			m_view.scrollView.scrollEnabled = t_scroll_enabled;
			m_view.scrollView.bounces = t_scroll_can_bounce;
			m_view.userInteractionEnabled = t_allow_user_interaction;
			
			if (!MCCStringIsEmpty(m_htmltext))
				/* UNCHECKED */ LoadHTMLText(m_htmltext, m_url);
			else if (!MCCStringIsEmpty(m_url))
				GoToURL(m_url);
		}
		else
		{
		 	if (t_view != nil)
				[t_view release];
		}
	});
	
	return t_success;
}

////////////////////////////////////////////////////////////////////////////////

MCBrowserNavigationType MCBrowserNavigationTypeFromWKNavigationType(WKNavigationType p_type)
{
	switch (p_type)
	{
		case WKNavigationTypeLinkActivated:
			return kMCBrowserNavigationTypeFollowLink;
		
		case WKNavigationTypeFormSubmitted:
			return kMCBrowserNavigationTypeSubmitForm;
		
		case WKNavigationTypeFormResubmitted:
			return kMCBrowserNavigationTypeResubmitForm;
		
		case WKNavigationTypeReload:
			return kMCBrowserNavigationTypeReload;
		
		case WKNavigationTypeBackForward:
			return kMCBrowserNavigationTypeBackForward;
		
		case WKNavigationTypeOther:
			return kMCBrowserNavigationTypeOther;
	}
}

////////////////////////////////////////////////////////////////////////////////

MCWKWebViewBrowserNavigationRequest::MCWKWebViewBrowserNavigationRequest(WKNavigationAction *p_action, bool p_quiet, MCWKNavigationDecisionHandler p_handler, com_livecode_libbrowser_MCWKWebViewNavigationDelegate* p_delegate)
	: MCBrowserNavigationRequestBase(p_action.request.URL.absoluteString.UTF8String, !p_action.targetFrame.isMainFrame, MCBrowserNavigationTypeFromWKNavigationType(p_action.navigationType))
{
	m_quiet = p_quiet;
	m_decision_handler = p_handler;

	m_delegate = [p_delegate retain];
}

MCWKWebViewBrowserNavigationRequest::~MCWKWebViewBrowserNavigationRequest()
{
	if (m_delegate != nil)
		[m_delegate release];
}

bool MCWKWebViewBrowserNavigationRequest::IsQuiet()
{
	return m_quiet;
}

void MCWKWebViewBrowserNavigationRequest::Continue()
{
	MCBrowserRunBlockOnMainFiber(^{
		[m_delegate continueRequest:this withDecisionHandler:m_decision_handler];
	});
}

void MCWKWebViewBrowserNavigationRequest::Cancel()
{
	MCBrowserRunBlockOnMainFiber(^{
		[m_delegate cancelRequest:this withDecisionHandler:m_decision_handler];
	});
}

////////////////////////////////////////////////////////////////////////////////

@implementation com_livecode_libbrowser_MCWKWebViewNavigationDelegate

- (id)initWithInstance:(MCWKWebViewBrowser*)instance
{
	self = [super init];
	if (self == nil)
		return nil;
	
	m_instance = instance;
	m_delay_requests = false;
	m_can_start_load = true;
	m_frame_requests = [[NSMutableArray array] retain];
	m_pending_request = nil;
	m_current_request = nil;
	m_queued_request = nil;

	return self;
}

- (void)dealloc
{
	if (m_pending_request != nil)
		[m_pending_request release];
	
	if (m_current_request != nil)
		[m_current_request release];
	
	if (m_queued_request != nil)
		[m_queued_request release];
	
	if (m_frame_requests != nil)
	{
		for (NSValue *t_value in m_frame_requests)
		{
			MCWKWebViewBrowserNavigationRequest *t_request = (MCWKWebViewBrowserNavigationRequest*)[t_value pointerValue];
			t_request->Release();
		}
		[m_frame_requests release];
	}

	[super dealloc];
}

- (void)finishNavigationWithError:(const char *)errorMsg
{
	for (NSValue *t_value in m_frame_requests)
	{
		MCWKWebViewBrowserNavigationRequest *t_frame_request;
		t_frame_request = (MCWKWebViewBrowserNavigationRequest*)t_value.pointerValue;
		if (!m_current_request.quiet)
		{
			if (errorMsg == nil)
				m_instance->OnDocumentLoadComplete(true, t_frame_request->GetURL());
			else
				m_instance->OnDocumentLoadFailed(true, t_frame_request->GetURL(), errorMsg);
		}
		t_frame_request->Release();
	}
	[m_frame_requests removeAllObjects];
	
	if (!m_current_request.quiet)
	{
		const char *t_url_string;
		t_url_string = m_current_request.url.absoluteString.UTF8String;
		if (errorMsg == nil)
		{
			m_instance->OnDocumentLoadComplete(false, t_url_string);
			m_instance->OnNavigationComplete(false, t_url_string);
		}
		else
		{
			m_instance->OnDocumentLoadFailed(false, t_url_string, errorMsg);
			m_instance->OnNavigationFailed(false, t_url_string, errorMsg);
		}
	}

	// the current request has ended so release it here
	[m_current_request release];
	m_current_request = nil;
	
	// Once the navigation is complete it will be safe to start a new load request
	m_can_start_load = true;

	[self checkQueuedRequest];
}

- (void)finishNavigation
{
	[self finishNavigationWithError:nil];
}

- (void)webView:(WKWebView *)webView decidePolicyForNavigationAction:(WKNavigationAction *)navigationAction decisionHandler:(void (^)(WKNavigationActionPolicy))decisionHandler
{
	NSString *t_url_string;
	t_url_string = [[[navigationAction request] URL] absoluteString];
	
	// Check if this request came from a libbrowser API call, and find whether
	// or not messages should be sent while the request is serviced
	bool t_quiet = false;
	if (navigationAction.navigationType == WKNavigationTypeOther)
	{
		MCAssert(m_pending_request != nil);
		t_quiet = m_pending_request.quiet;
	}
	
	MCWKWebViewBrowserNavigationRequest *t_request;
	t_request = new MCWKWebViewBrowserNavigationRequest(navigationAction, t_quiet, [decisionHandler copy], self);
	
	if (t_quiet || [NSURLConnection canHandleRequest: [navigationAction request]])
	{
		if (!t_quiet && m_delay_requests)
		{
			if (m_instance->OnNavigationRequest(t_request))
			{
				t_request->Release();
				return;
			}
		}
		
		t_request->Continue();
		t_request->Release();
	}
	else
	{
		m_instance->OnNavigationRequestUnhandled(t_request->IsFrame(), [t_url_string UTF8String]);
		
		t_request->Release();
		decisionHandler(WKNavigationActionPolicyCancel);
	}
	
}

- (void)webView:(WKWebView *)webView didCommitNavigation:(WKNavigation *)navigation
{
	if (m_current_request == nil)
		return;
	
	if (!m_current_request.quiet)
		m_instance->OnDocumentLoadBegin(false, m_current_request.url.absoluteString.UTF8String);

	// Once the navigation is committed, new requests can be made safely
	m_can_start_load = true;
	[self checkQueuedRequest];
}

- (void)webView:(WKWebView *)webView didStartProvisionalNavigation:(WKNavigation *)navigation
{
	if (m_current_request == nil)
		return;
	
	if (!m_current_request.quiet)
		m_instance->OnNavigationBegin(false, m_current_request.url.absoluteString.UTF8String);
}

- (void)webView:(WKWebView *)webView didFinishNavigation:(WKNavigation *)navigation
{
	if (m_current_request == nil)
		return;
	
	m_instance->SyncJavaScriptHandlers();
	
	[self finishNavigation];
}

- (void)webView:(WKWebView *)webView didFailProvisionalNavigation:(null_unspecified WKNavigation *)navigation withError:(NSError *)error
{
	if (m_current_request == nil)
		return;
	
	const char *t_error;
	t_error = [[error localizedDescription] UTF8String];
	
	[self finishNavigationWithError:t_error];
}

- (void)webView:(WKWebView *)webView didFailNavigation:(WKNavigation *)navigation withError:(NSError *)error
{
	if (m_current_request == nil)
		return;
	
	const char *t_error;
	t_error = [[error localizedDescription] UTF8String];

	[self finishNavigationWithError:t_error];
}

- (void)continueRequest:(MCWKWebViewBrowserNavigationRequest *)request withDecisionHandler:(MCWKNavigationDecisionHandler)decisionHandler
{
	if (request->IsFrame())
	{
		[m_frame_requests addObject:[NSValue valueWithPointer:request]];
		request->Retain();
		decisionHandler(WKNavigationActionPolicyAllow);
	}
	else
	{
		// continuing this request will cause any in-progress requests to be
		// silently cancelled, so end that request here and send an appropriate
		// error message.
		if (m_current_request != nil)
			[self finishNavigationWithError: "navigation request cancelled"];

		// The pending load has been accepted so make it current
		if (m_current_request != nil)
			[m_current_request release];
		m_current_request = m_pending_request;
		m_pending_request = nil;
		
		decisionHandler(WKNavigationActionPolicyAllow);
	}
}

- (void)cancelRequest: (MCWKWebViewBrowserNavigationRequest *)request withDecisionHandler:(MCWKNavigationDecisionHandler)decisionHandler
{
	decisionHandler(WKNavigationActionPolicyCancel);
	
	if (!request->IsQuiet())
		m_instance->OnNavigationFailed(request->IsFrame(), request->GetURL(), "navigation cancelled");
	
	if (!request->IsFrame())
	{
		// clear the rejected pending load request
		if (m_pending_request != nil)
		{
			[m_pending_request release];
			m_pending_request = nil;
		}

		m_can_start_load = true;
		
		[self checkQueuedRequest];
	}
}

- (void) checkQueuedRequest
{
	// if there is a queued request from API calls then load it
	if (m_queued_request != nil)
	{
		MCWKWebViewLoadRequest *t_request = m_queued_request;
		m_queued_request = nil;
		
		WKWebView *t_webview = nil;
		/* UNCHECKED */ m_instance->GetWebView(t_webview);

		[self loadRequest:t_request inWebView:t_webview];
		
		[t_request release];
	}
}

- (void) loadRequest:(MCWKWebViewLoadRequest *)request inWebView:(WKWebView *)webView
{
	if (request == nil)
		return;
	
	if (!m_can_start_load)
	{
		[request retain];
		if (m_queued_request != nil)
			[m_queued_request release];
		m_queued_request = request;
		return;
	}
	
	// Prevent other loads until this request has begun.
	// This avoids a permissions error when making multiple requests to file urls within
	// an app's sandboxed folder.
	m_can_start_load = false;
	m_pending_request = [request retain];

	MCBrowserRunBlockOnMainFiber(^{
		if (request.htmlText == nil)
		{
			[webView loadRequest:[NSURLRequest requestWithURL:request.url]];
		}
		else
		{
			[webView loadHTMLString:request.htmlText baseURL:request.url];
		}
	});
}

- (void)setDelayRequests:(bool)p_new_value
{
	m_delay_requests = p_new_value;
}

- (bool)getDelayRequests
{
	return m_delay_requests;
}

@end

////////////////////////////////////////////////////////////////////////////////

class MCWKWebViewBrowserFactory : public MCBrowserFactory
{
	virtual bool CreateBrowser(void *p_display, void *p_parent_view, MCBrowser *&r_browser)
	{
		MCWKWebViewBrowser *t_browser;
		t_browser = new MCWKWebViewBrowser();
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

bool MCWKWebViewBrowserFactoryCreate(MCBrowserFactoryRef &r_factory)
{
	MCBrowserFactory *t_factory;
	t_factory = new MCWKWebViewBrowserFactory();
	
	if (t_factory == nil)
		return false;
	
	r_factory = (MCBrowserFactoryRef)t_factory;
	return true;
}

////////////////////////////////////////////////////////////////////////////////
