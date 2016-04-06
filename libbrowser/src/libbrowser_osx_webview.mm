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
#import <WebKit/WebKit.h>
#import <WebKit/WebFrameLoadDelegate.h>
#import <JavaScriptCore/JavaScriptCore.h>

#include <core.h>

#include "libbrowser_internal.h"
#include "libbrowser_osx_webview.h"

////////////////////////////////////////////////////////////////////////////////

bool MCNSNumberToBrowserValue(NSNumber *p_number, MCBrowserValue &r_value);
bool MCNSDictionaryToBrowserValue(NSDictionary *p_dictionary, MCBrowserValue &r_value);
bool MCNSArrayToBrowserValue(NSArray * p_array, MCBrowserValue &r_value);
bool MCWebScriptObjectToBrowserValue(WebScriptObject *p_obj, MCBrowserValue &r_value);

//////////

bool MCNSObjectToBrowserValue(id p_obj, MCBrowserValue &r_value)
{
	if ([p_obj isKindOfClass: [NSString class]])
		return MCBrowserValueSetUTF8String(r_value, [(NSString*)p_obj cStringUsingEncoding: NSUTF8StringEncoding]);
	else if ([p_obj isKindOfClass: [NSNumber class]])
		return MCNSNumberToBrowserValue((NSNumber*)p_obj, r_value);
	else if ([p_obj isKindOfClass: [NSDictionary class]])
		return MCNSDictionaryToBrowserValue((NSDictionary*)p_obj, r_value);
	else if ([p_obj isKindOfClass: [NSArray class]])
		return MCNSArrayToBrowserValue((NSArray*)p_obj, r_value);
	else if ([p_obj isKindOfClass: [WebScriptObject class]])
		return MCWebScriptObjectToBrowserValue((WebScriptObject*)p_obj, r_value);
	
	return false;
}

bool MCNSDictionaryToBrowserDictionary(NSDictionary *p_dictionary, MCBrowserDictionaryRef &r_dict)
{
	__block bool t_success;
	t_success = true;
	
	__block MCBrowserDictionaryRef t_dict;
	t_dict = nil;
	
	if (t_success)
		t_success = MCBrowserDictionaryCreate(t_dict, [p_dictionary count]);
	
	if (t_success)
		[p_dictionary enumerateKeysAndObjectsUsingBlock: ^(id p_key, id p_obj, BOOL *r_stop) {
			MCBrowserValue t_value;
			MCBrowserMemoryClear(&t_value, sizeof(MCBrowserValue));
			
			t_success = [p_key isKindOfClass: [NSString class]];
			
			if (t_success)
				t_success = MCNSObjectToBrowserValue(p_obj, t_value);
			
			if (t_success)
				t_success = MCBrowserDictionarySetValue(t_dict, [(NSString*)p_key cStringUsingEncoding: NSUTF8StringEncoding], t_value);
			
			if (!t_success)
				*r_stop = YES;
			
			MCBrowserValueClear(t_value);
		}];
	
	if (t_success)
		r_dict = t_dict;
	else
		MCBrowserDictionaryRelease(t_dict);
	
	return t_success;
}

bool MCNSArrayToBrowserList(NSArray *p_array, MCBrowserListRef &r_list)
{
	__block bool t_success;
	t_success = true;
	
	__block MCBrowserListRef t_list;
	t_list = nil;
	
	if (t_success)
		t_success = MCBrowserListCreate(t_list, [p_array count]);
	
	if (t_success)
		[p_array enumerateObjectsUsingBlock: ^(id p_obj, NSUInteger p_index, BOOL *r_stop) {
			MCBrowserValue t_value;
			MCBrowserMemoryClear(&t_value, sizeof(MCBrowserValue));
			
			t_success = MCNSObjectToBrowserValue(p_obj, t_value);
			
			if (t_success)
				t_success = MCBrowserListSetValue(t_list, p_index, t_value);
			
			if (!t_success)
				*r_stop = YES;
			
			MCBrowserValueClear(t_value);
		}];
	
	if (t_success)
		r_list = t_list;
	else
		MCBrowserListRelease(t_list);
	
	return t_success;
}

bool MCNSNumberToBrowserValue(NSNumber *p_number, MCBrowserValue &r_value)
{
	if (p_number == @(YES))
		return MCBrowserValueSetBoolean(r_value, true);
	else if (p_number == @(NO))
		return MCBrowserValueSetBoolean(r_value, false);
	else if (MCCStringEqual([p_number objCType], @encode(int)))
		return MCBrowserValueSetInteger(r_value, [p_number intValue]);
	else
		return MCBrowserValueSetDouble(r_value, [p_number doubleValue]);
}

//////////

bool MCNSDictionaryToBrowserValue(NSDictionary *p_dictionary, MCBrowserValue &r_value)
{
	bool t_success;
	t_success = true;
	
	MCBrowserDictionaryRef t_dict;
	t_dict = nil;

	t_success = MCNSDictionaryToBrowserDictionary(p_dictionary, t_dict);
	
	if (t_success)
		t_success = MCBrowserValueSetDictionary(r_value, t_dict);
	
	MCBrowserDictionaryRelease(t_dict);
	
	return t_success;
}

bool MCNSArrayToBrowserValue(NSArray *p_array, MCBrowserValue &r_value)
{
	bool t_success;
	t_success = true;
	
	MCBrowserListRef t_list;
	t_list = nil;
	
	t_success = MCNSArrayToBrowserList(p_array, t_list);
	
	if (t_success)
		t_success = MCBrowserValueSetList(r_value, t_list);
	
	MCBrowserListRelease(t_list);
	
	return t_success;
}

//////////

bool MCJSObjectToBrowserValue(JSContextRef p_context, JSObjectRef p_value, MCBrowserValue &r_value);
JSContextRef MCGetCurrentJSContext();

bool MCWebScriptObjectToBrowserValue(WebScriptObject *p_obj, MCBrowserValue &r_value)
{
	JSContextRef t_context;
	t_context = MCGetCurrentJSContext();
	
	JSObjectRef t_js_obj;
	t_js_obj = [p_obj JSObject];
	
	return MCJSObjectToBrowserValue(t_context, t_js_obj, r_value);
}

bool MCJSBooleanToBrowserValue(JSContextRef p_context, JSValueRef p_value, MCBrowserValue &r_value)
{
	return MCBrowserValueSetBoolean(r_value, JSValueToBoolean(p_context, p_value));
}

bool MCJSNumberToBrowserValue(JSContextRef p_context, JSValueRef p_value, MCBrowserValue &r_value)
{
	return MCBrowserValueSetDouble(r_value, JSValueToNumber(p_context, p_value, nil));
}

bool MCJSStringToUTF8String(JSStringRef p_string, char *&r_utf8_string)
{
	uint32_t t_bytes;
	t_bytes = JSStringGetMaximumUTF8CStringSize(p_string);
	
	char *t_string;
	t_string = nil;
	
	if (!MCBrowserMemoryAllocate(t_bytes, (void*&)t_string))
		return false;
	
	JSStringGetUTF8CString(p_string, t_string, t_bytes);
	
	r_utf8_string = t_string;
	return true;
}

bool MCJSStringToBrowserValue(JSContextRef p_context, JSValueRef p_value, MCBrowserValue &r_value)
{
	bool t_success;
	t_success = true;
	
	JSStringRef t_string;
	t_string = JSValueToStringCopy(p_context, p_value, nil);
	
	char *t_utf8_string;
	t_utf8_string = nil;
	
	if (t_success)
		t_success = MCJSStringToUTF8String(t_string, t_utf8_string);
	
	if (t_success)
		t_success = MCBrowserValueSetUTF8String(r_value, t_utf8_string);
	
	if (t_string != nil)
		JSStringRelease(t_string);
	
	if (t_utf8_string != nil)
		MCBrowserMemoryDeallocate(t_utf8_string);
	
	return t_success;
}

bool MCJSValueToBrowserValue(JSContextRef p_context, JSValueRef p_value, MCBrowserValue &r_value)
{
	switch (JSValueGetType(p_context, p_value))
	{
		case kJSTypeUndefined:
			break;
			
		case kJSTypeNull:
			MCBrowserValueClear(r_value);
			return true;
			
		case kJSTypeBoolean:
			return MCJSBooleanToBrowserValue(p_context, p_value, r_value);
			
		case kJSTypeNumber:
			return MCJSNumberToBrowserValue(p_context, p_value, r_value);
			
		case kJSTypeString:
			return MCJSStringToBrowserValue(p_context, p_value, r_value);
			
		case kJSTypeObject:
			return MCJSObjectToBrowserValue(p_context, (JSObjectRef)p_value, r_value);
	}
	
	return false;
}

bool MCJSObjectToBrowserValue(JSContextRef p_context, JSObjectRef p_object, MCBrowserValue &r_value)
{
	bool t_success;
	t_success = true;
	
	JSPropertyNameArrayRef t_prop_names;
	t_prop_names = nil;
	
	if (t_success)
	{
		t_prop_names = JSObjectCopyPropertyNames(p_context, p_object);
		t_success = t_prop_names != nil;
	}
	
	uint32_t t_count;
	t_count = 0;
	
	if (t_success)
		t_count = JSPropertyNameArrayGetCount(t_prop_names);
	
	MCBrowserDictionaryRef t_dict;
	t_dict = nil;
	
	if (t_success)
		t_success = MCBrowserDictionaryCreate(t_dict);
	
	for (uint32_t i = 0; t_success && i < t_count; i++)
	{
		JSStringRef t_name;
		t_name = JSPropertyNameArrayGetNameAtIndex(t_prop_names, i);
		
		JSValueRef t_js_value;
		t_js_value = JSObjectGetProperty(p_context, p_object, t_name, nil);
		
		char *t_utf8_name;
		t_utf8_name = nil;
		
		if (t_success)
			t_success = MCJSStringToUTF8String(t_name, t_utf8_name);
		
		MCBrowserValue t_value;
		MCBrowserMemoryClear(&t_value, sizeof(MCBrowserValue));
		
		if (t_success)
			t_success = MCJSValueToBrowserValue(p_context, t_js_value, t_value);
		
		if (t_success)
			t_success = MCBrowserDictionarySetValue(t_dict, t_utf8_name, t_value);
		
		MCBrowserValueClear(t_value);
	}
	
	if (t_success)
		MCBrowserValueSetDictionary(r_value, t_dict);
	
	MCBrowserDictionaryRelease(t_dict);
	
	if (t_prop_names != nil)
		JSPropertyNameArrayRelease(t_prop_names);
	
	return t_success;
}

////////////////////////////////////////////////////////////////////////////////

#define LIBBROWSER_DUMMY_URL "http://libbrowser_dummy_url/"

////////////////////////////////////////////////////////////////////////////////

@interface MCWebViewFrameLoadDelegate : NSObject
{
	MCWebViewBrowser *m_instance;
	bool m_pending_request;
	bool m_frame_request;
	char *m_request_url;
	char *m_frame_request_url;
}

- (id)initWithInstance:(MCWebViewBrowser*)instance;
- (void)dealloc;

- (void)webView:(WebView *)sender didStartProvisionalLoadForFrame:(WebFrame *)frame;
- (void)webView:(WebView *)sender didCommitLoadForFrame:(WebFrame *)frame;
- (void)webView:(WebView *)sender didFinishLoadForFrame:(WebFrame *)frame;
- (void)webView:(WebView *)sender didFailProvisionalLoadWithError:(NSError *)error forFrame:(WebFrame *)frame;
- (void)webView:(WebView *)sender didFailLoadWithError:(NSError *)error forFrame:(WebFrame *)frame;

- (void)setPendingRequest: (bool)newValue;

@end

@interface MCWebViewPolicyDelegate : NSObject
{
	MCWebViewBrowser *m_instance;
}

- (id)initWithInstance:(MCWebViewBrowser *)instance;
- (void)dealloc;

- (void)webView:(WebView *)webView decidePolicyForMIMEType:(NSString *)type request:(NSURLRequest *)request frame:(WebFrame *)frame decisionListener:(id<WebPolicyDecisionListener>)listener;
- (void)webView:(WebView *)webView decidePolicyForNavigationAction:(NSDictionary *)actionInformation request:(NSURLRequest *)request frame:(WebFrame *)frame decisionListener:(id<WebPolicyDecisionListener>)listener;

@end

@interface MCWebUIDelegate : NSObject
{
}

- (NSUInteger)webView:(WebView *)webView dragDestinationActionMaskForDraggingInfo:(id<NSDraggingInfo>)draggingInfo;
- (NSUInteger)webView:(WebView *)webView dragSourceActionMaskForPoint:(NSPoint)point;

@end

////////////////////////////////////////////////////////////////////////////////

inline void MCBrowserRunBlockOnMainFiber(void (^p_block)(void))
{
	p_block();
}

////////////////////////////////////////////////////////////////////////////////

MCWebViewBrowser::MCWebViewBrowser(void)
{
	m_view = nil;
	m_delegate = nil;
	m_policy_delegate = nil;
	m_ui_delegate = nil;
	
	m_js_handlers = nil;
	m_js_handler_list =nil;
}

MCWebViewBrowser::~MCWebViewBrowser(void)
{
	MCBrowserRunBlockOnMainFiber(^{
		if (m_view != nil)
		{
			[m_view setFrameLoadDelegate: nil];
			[m_view setPolicyDelegate: nil];
			[m_view setUIDelegate: nil];
			[m_view release];
		}
		
		if (m_delegate != nil)
			[m_delegate release];
		
		if (m_policy_delegate != nil)
			[m_policy_delegate release];
		
		if (m_ui_delegate != nil)
			[m_ui_delegate release];
		
		if (m_js_handler_list != nil)
			[m_js_handler_list release];
	});
}

bool MCWebViewBrowser::GoToURL(const char *p_url)
{
	WebView *t_view;
	if (!GetView(t_view))
		return false;
	
	MCBrowserRunBlockOnMainFiber(^{
		[t_view.mainFrame loadRequest: [NSURLRequest requestWithURL: [NSURL URLWithString: [NSString stringWithCString: p_url encoding:NSUTF8StringEncoding]]]];
	});
	
	return true;
}

bool MCWebViewBrowser::GetUrl(char *&r_url)
{
	WebView *t_view;
	if (!GetView(t_view))
		return false;
	
	__block bool t_success;
	MCBrowserRunBlockOnMainFiber(^{
		t_success = MCCStringClone([[t_view mainFrameURL] cStringUsingEncoding: NSUTF8StringEncoding], r_url);
	});
	
	return t_success;
}

//bool MCWebViewBrowser::SetVerticalScrollbarEnabled(bool p_value)
//{
//	WebView *t_view;
//	if (!GetView(t_view))
//		return false;
//	
//	MCBrowserRunBlockOnMainFiber(^{
//		GetScrollView().showsVerticalScrollIndicator = p_value ? YES : NO;
//	});
//	
//	return true;
//}

//bool MCUIWebViewBrowser::GetVerticalScrollbarEnabled(bool& r_value)
//{
//	UIWebView *t_view;
//	if (!GetView(t_view))
//		return false;
//	
//	MCBrowserRunBlockOnMainFiber(^{
//		r_value = GetScrollView().showsVerticalScrollIndicator;
//	});
//	
//	return true;
//}

//bool MCUIWebViewBrowser::SetHorizontalScrollbarEnabled(bool p_value)
//{
//	UIWebView *t_view;
//	if (!GetView(t_view))
//		return false;
//	
//	MCBrowserRunBlockOnMainFiber(^{
//		GetScrollView().showsHorizontalScrollIndicator = p_value ? YES : NO;
//	});
//	
//	return true;
//}

//bool MCUIWebViewBrowser::GetHorizontalScrollbarEnabled(bool& r_value)
//{
//	UIWebView *t_view;
//	if (!GetView(t_view))
//		return false;
//	
//	MCBrowserRunBlockOnMainFiber(^{
//		r_value = GetScrollView().showsHorizontalScrollIndicator;
//	});
//	
//	return true;
//}

bool MCWebViewBrowser::GetHTMLText(char *&r_htmltext)
{
	/* TODO - obtain directly from mainFrame dataSource data */
	return EvaluateJavaScript("document.documentElement.outerHTML", r_htmltext);
}

bool MCWebViewBrowser::SetHTMLText(const char *p_htmltext)
{
	return ExecLoad(LIBBROWSER_DUMMY_URL, p_htmltext);
}

bool MCWebViewBrowser::GetUserAgent(char*& r_user_agent)
{
	WebView *t_view;
	if (!GetView(t_view))
		return false;
	
	__block char *t_result;
	t_result = nil;
	MCBrowserRunBlockOnMainFiber(^{
		NSString *t_user_agent;
		t_user_agent = [t_view customUserAgent];
		
		t_result = strdup(t_user_agent != nil ? [t_user_agent cStringUsingEncoding: NSUTF8StringEncoding] : "");
	});
	
	if (t_result == nil)
		return false;
	
	r_user_agent = t_result;
	
	return true;
}

bool MCWebViewBrowser::SetUserAgent(const char *p_user_agent)
{
	WebView *t_view;
	if (!GetView(t_view))
		return false;
	
	MCBrowserRunBlockOnMainFiber(^{
		NSString *t_ns_user_agent;
		if (strcmp(p_user_agent, "") == 0)
			t_ns_user_agent = nil;
		else
			t_ns_user_agent = [NSString stringWithCString: p_user_agent encoding: NSUTF8StringEncoding];
		
		[t_view setCustomUserAgent: t_ns_user_agent];
	});
	
	return true;
}

bool MCWebViewBrowser::GetJavaScriptHandlers(char *&r_handlers)
{
	return MCCStringClone(m_js_handlers ? m_js_handlers : "", r_handlers);
}

bool MCWebViewBrowser::SetJavaScriptHandlers(const char *p_handlers)
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
	
	m_js_handlers = t_handlers;
	t_handlers = nil;
	
	if (m_js_handlers != nil)
	{
		if (m_js_handler_list != nil)
			[m_js_handler_list release];
		m_js_handler_list = nil;
		
		m_js_handler_list = [[[NSString stringWithCString: m_js_handlers encoding:NSUTF8StringEncoding] componentsSeparatedByString: @"\n"] retain];
	}
	
	MCBrowserRunBlockOnMainFiber(^{
		SyncJavaScriptHandlers(m_js_handler_list);
	});
	
	return true;
}

static JSContextRef s_js_context;
JSContextRef MCGetCurrentJSContext()
{
	return s_js_context;
}

@interface MCWebViewJSInterface : NSObject
{
	MCWebViewBrowser *m_instance;
	NSArray *m_js_handlers;
}

- (id)initWithBrowser: (MCWebViewBrowser *)browser;
- (void)dealloc;

- (void)setJSHandlers: (NSArray*)handlers;
- (id)invokeUndefinedMethodFromWebScript: (NSString*)name withArguments:(NSArray *)arguments;

+ (BOOL)isSelectorExcludedFromWebScript:(SEL)selector;

@end

@implementation MCWebViewJSInterface

- (id)initWithBrowser:(MCWebViewBrowser *)browser
{
	self = [super init];
	if (self == nil)
		return nil;
	
	m_instance = browser;
	m_js_handlers = nil;
	
	return self;
}

- (void)dealloc
{
	if (m_js_handlers != nil)
		[m_js_handlers release];
	
	[super dealloc];
}

- (void)setJSHandlers:(NSArray *)handlers
{
	if (m_js_handlers != nil)
		[m_js_handlers release];
	
	m_js_handlers = [handlers retain];
}

- (id)invokeUndefinedMethodFromWebScript: (NSString*)name withArguments:(NSArray *)arguments
{
	if ([m_js_handlers containsObject: name])
	{
		MCBrowserListRef t_args;
		t_args = nil;
		/* UNCHECKED */ MCNSArrayToBrowserList(arguments, t_args);
		
		m_instance->OnJavaScriptCall([name cStringUsingEncoding: NSUTF8StringEncoding], t_args);
		
		MCBrowserListRelease(t_args);
	}
	
	return nil;
}

+ (BOOL)isSelectorExcludedFromWebScript:(SEL)selector
{
	return YES;
}

@end

bool MCWebViewBrowser::SyncJavaScriptHandlers(NSArray *p_handlers)
{
	bool t_success;
	t_success = true;
	
	WebView *t_view;
	t_success = GetView(t_view);
	
	WebScriptObject *t_script_object;
	t_script_object = nil;
	
	MCWebViewJSInterface *t_js_interface;
	t_js_interface = nil;
	
	s_js_context = t_view.mainFrame.globalContext;
	
	if (t_success)
	{
		t_js_interface = [[[MCWebViewJSInterface alloc] initWithBrowser:this] autorelease];
		t_success = t_js_interface != nil;
	}
	
	if (t_success)
	{
		[t_js_interface setJSHandlers:p_handlers];
		[t_view.windowScriptObject setValue:t_js_interface forKey:@"liveCode"];
	}
	
	return t_success;
}

void MCWebViewBrowser::SyncJavaScriptHandlers()
{
	if (m_js_handler_list != nil)
		/* UNCHECKED */ SyncJavaScriptHandlers(m_js_handler_list);
}

//////////

bool MCWebViewBrowser::SetBoolProperty(MCBrowserProperty p_property, bool p_value)
{
	switch (p_property)
	{
		//case kMCBrowserVerticalScrollbarEnabled:
		//	return SetVerticalScrollbarEnabled(p_value);
			
		//case kMCBrowserHorizontalScrollbarEnabled:
		//	return SetHorizontalScrollbarEnabled(p_value);
			
		default:
			break;
	}
	
	return true;
}

bool MCWebViewBrowser::GetBoolProperty(MCBrowserProperty p_property, bool &r_value)
{
	switch (p_property)
	{
		//case kMCBrowserVerticalScrollbarEnabled:
		//	return GetVerticalScrollbarEnabled(r_value);
			
		//case kMCBrowserHorizontalScrollbarEnabled:
		//	return GetHorizontalScrollbarEnabled(r_value);
			
		default:
			break;
	}
	
	return true;
}

bool MCWebViewBrowser::SetStringProperty(MCBrowserProperty p_property, const char *p_utf8_string)
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

bool MCWebViewBrowser::GetStringProperty(MCBrowserProperty p_property, char *&r_utf8_string)
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

bool MCWebViewBrowser::GetRect(MCBrowserRect &r_rect)
{
	WebView *t_view;
	if (!GetView(t_view))
		return false;
	
	MCBrowserRunBlockOnMainFiber(^{
		NSRect t_frame = [t_view frame];
		r_rect.left = t_frame.origin.x;
		r_rect.top = t_frame.origin.y;
		r_rect.right = t_frame.origin.x + t_frame.size.width;
		r_rect.bottom = t_frame.origin.y + t_frame.size.height;
	});
	
	return true;
}

bool MCWebViewBrowser::SetRect(const MCBrowserRect &p_rect)
{
	WebView *t_view;
	if (!GetView(t_view))
		return false;
	
	NSRect t_rect;
	t_rect = NSMakeRect(p_rect.left, p_rect.top, p_rect.right - p_rect.left, p_rect.bottom - p_rect.top);
	
	MCBrowserRunBlockOnMainFiber(^{
		[t_view setFrame:t_rect];
	});

	return true;
}

// Browser-specific actions
bool MCWebViewBrowser::ExecStop()
{
	WebView *t_view;
	if (!GetView(t_view))
		return false;
	
	MCBrowserRunBlockOnMainFiber(^{
		[t_view.mainFrame stopLoading];
	});
	
	return true;
}

bool MCWebViewBrowser::GoForward()
{
	WebView *t_view;
	if (!GetView(t_view))
		return false;
	
	MCBrowserRunBlockOnMainFiber(^{
		[t_view goForward];
	});
	
	return true;
}

bool MCWebViewBrowser::GoBack()
{
	WebView *t_view;
	if (!GetView(t_view))
		return false;
	
	MCBrowserRunBlockOnMainFiber(^{
		[t_view goBack];
	});
	
	return true;
}

bool MCWebViewBrowser::ExecReload()
{
	WebView *t_view;
	if (!GetView(t_view))
		return false;
	
	MCBrowserRunBlockOnMainFiber(^{
		[t_view.mainFrame reload];
	});
	
	return true;
}

bool MCWebViewBrowser::EvaluateJavaScript(const char *p_script, char *&r_result)
{
	WebView *t_view;
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

bool MCWebViewBrowser::ExecLoad(const char *p_url, const char *p_html)
{
	WebView *t_view;
	if (!GetView(t_view))
		return false;
	
	// MW-2012-10-01: [[ Bug 10422 ]] Make sure we mark a pending request so the
	//   HTML loading doesn't divert through a loadRequested message.

	MCBrowserRunBlockOnMainFiber(^{
		[m_delegate setPendingRequest: true];
		[t_view.mainFrame loadHTMLString: [NSString stringWithCString: p_html encoding:NSUTF8StringEncoding] baseURL: [NSURL URLWithString: [NSString stringWithCString: p_url encoding:NSUTF8StringEncoding]]];
	});
	
	return true;
}

////////////////////////////////////////////////////////////////////////////////

//
// MW-2012-09-20: [[ Bug 10304 ]] Returns the UIScrollView which is embedded in
//   the UIWebView.
//UIScrollView *MCUIWebViewBrowser::GetScrollView(void)
//{
//	UIWebView *t_view;
//	if (!GetView(t_view))
//		return nil;
//	
//	__block UIScrollView *t_scroll;
//
//	MCBrowserRunBlockOnMainFiber(^{
//		t_scroll = [t_view scrollView];
//	});
//	
//	return t_scroll;
//}

bool MCWebViewBrowser::GetView(WebView *&r_view)
{
	if (m_view == nil)
		return false;
	
	r_view = m_view;
	return true;
}

void *MCWebViewBrowser::GetNativeLayer()
{
	WebView *t_view;
	if (!GetView(t_view))
		return nil;
	
	return t_view;
}

////////////////////////////////////////////////////////////////////////////////

bool MCWebViewBrowser::Init(void)
{
	__block bool t_success;
	t_success = true;
	
	MCBrowserRunBlockOnMainFiber(^{
		WebView *t_view;
		t_view = [[WebView alloc] initWithFrame: NSMakeRect(0, 0, 0, 0)];
		t_success = t_view != nil;
		
		if (t_success)
		{
			m_delegate = [[MCWebViewFrameLoadDelegate alloc] initWithInstance: this];
			t_success = m_delegate != nil;
		}
		
		if (t_success)
		{
			m_policy_delegate = [[MCWebViewPolicyDelegate alloc] initWithInstance: this];
			t_success = m_policy_delegate != nil;
		}
		
		if (t_success)
		{
			m_ui_delegate = [[MCWebUIDelegate alloc] init];
			t_success = m_ui_delegate != nil;
		}
		
		if (t_success)
		{
			[t_view setHidden: YES];
			[t_view setFrameLoadDelegate: m_delegate];
			[t_view setPolicyDelegate: m_policy_delegate];
			[t_view setUIDelegate: m_ui_delegate];
			m_view = t_view;
		}
		else if (t_view != nil)
			[t_view release];
	});
	
	return t_success;
}

////////////////////////////////////////////////////////////////////////////////

@implementation MCWebViewFrameLoadDelegate

- (id)initWithInstance:(MCWebViewBrowser*)instance
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

- (void)webView:(WebView *)sender didStartProvisionalLoadForFrame:(WebFrame *)frame
{
	m_frame_request = ![sender.mainFrame isEqual: frame];
	
	if (m_pending_request)
	{
		m_pending_request = false;
		return;
	}
	
	NSURLRequest *t_request;
	t_request = frame.provisionalDataSource.initialRequest;

	char *&t_url = m_frame_request ? m_frame_request_url : m_request_url;
	if (t_url != nil)
		MCCStringFree(t_url);
	t_url = nil;
	
	/* UNCHECKED */ MCCStringClone([t_request.URL.absoluteString cStringUsingEncoding: NSUTF8StringEncoding], t_url);
	
	if (!m_frame_request)
		m_instance->OnNavigationBegin(false, t_url);
	
	return;
}

- (void)webView:(WebView *)sender didCommitLoadForFrame:(WebFrame *)frame
{
	if (!m_frame_request)
		m_instance->SyncJavaScriptHandlers();
	
	char *t_url = m_frame_request ? m_frame_request_url : m_request_url;
	if (t_url == nil)
		return;
	
	m_instance->OnDocumentLoadBegin(m_frame_request, t_url);
}

- (void)webView:(WebView *)sender didFinishLoadForFrame:(WebFrame *)frame
{
	char *&t_url = m_frame_request ? m_frame_request_url : m_request_url;
	if (t_url == nil)
		return;
	
	m_instance->OnDocumentLoadComplete(m_frame_request, t_url);
	if (!m_frame_request)
		m_instance->OnNavigationComplete(false, t_url);
	
	MCCStringFree(t_url);
	t_url = nil;
}

- (void)webView:(WebView *)sender didFailProvisionalLoadWithError:(NSError *)error forFrame:(WebFrame *)frame
{
	char *&t_url = m_frame_request ? m_frame_request_url : m_request_url;
	if (t_url == nil)
		return;
	
	const char *t_error;
	t_error = [[error localizedDescription] cStringUsingEncoding:NSUTF8StringEncoding];
	
	m_instance->OnDocumentLoadFailed(m_frame_request, m_frame_request ? m_frame_request_url : m_request_url, t_error);
	if (!m_frame_request)
		m_instance->OnNavigationFailed(false, m_request_url, t_error);
	
	MCCStringFree(t_url);
	t_url = nil;
}

- (void)webView:(WebView *)sender didFailLoadWithError:(NSError *)error forFrame:(WebFrame *)frame
{
	char *&t_url = m_frame_request ? m_frame_request_url : m_request_url;
	if (t_url == nil)
		return;
	
	const char *t_error;
	t_error = [[error localizedDescription] cStringUsingEncoding:NSUTF8StringEncoding];
	
	m_instance->OnDocumentLoadFailed(m_frame_request, m_frame_request ? m_frame_request_url : m_request_url, t_error);
	if (!m_frame_request)
		m_instance->OnNavigationFailed(false, m_request_url, t_error);
	
	MCCStringFree(t_url);
	t_url = nil;
}

- (void)setPendingRequest:(bool)p_new_value
{
	m_pending_request = p_new_value;
}

@end

@implementation MCWebViewPolicyDelegate

- (id)initWithInstance:(MCWebViewBrowser*)instance
{
	self = [super init];
	if (self == nil)
		return nil;
	
	m_instance = instance;
	
	return self;
}

- (void)dealloc
{
	[super dealloc];
}

- (void)webView:(WebView *)webView decidePolicyForMIMEType:(NSString *)type request:(NSURLRequest *)request frame:(WebFrame *)frame decisionListener:(id<WebPolicyDecisionListener>)listener
{
	if ([WebView canShowMIMEType:type])
		[listener use];
	else
	{
		[listener ignore];
		m_instance->OnNavigationRequestUnhandled(![webView.mainFrame isEqual: frame], [request.URL.absoluteString cStringUsingEncoding: NSUTF8StringEncoding]);
	}
}

- (void)webView:(WebView *)webView decidePolicyForNavigationAction:(NSDictionary *)actionInformation request:(NSURLRequest *)request frame:(WebFrame *)frame decisionListener:(id<WebPolicyDecisionListener>)listener
{
	if ([NSURLConnection canHandleRequest:request])
		[listener use];
	else
	{
		[listener ignore];
		m_instance->OnNavigationRequestUnhandled(![webView.mainFrame isEqual: frame], [request.URL.absoluteString cStringUsingEncoding: NSUTF8StringEncoding]);
	}
}

@end

@implementation MCWebUIDelegate

- (NSUInteger)webView:(WebView *)webView dragDestinationActionMaskForDraggingInfo:(id<NSDraggingInfo>)draggingInfo
{
	return WebDragDestinationActionNone;
}

- (NSUInteger)webView:(WebView *)webView dragSourceActionMaskForPoint:(NSPoint)point
{
	return WebDragSourceActionNone;
}

@end

////////////////////////////////////////////////////////////////////////////////

class MCWebViewBrowserFactory : public MCBrowserFactory
{
	virtual bool CreateBrowser(void *p_display, void *p_parent_view, MCBrowser *&r_browser)
	{
		MCWebViewBrowser *t_browser;
		t_browser = new MCWebViewBrowser();
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

bool MCWebViewBrowserFactoryCreate(MCBrowserFactoryRef &r_factory)
{
	MCBrowserFactory *t_factory;
	t_factory = new MCWebViewBrowserFactory();
	
	if (t_factory == nil)
		return false;
	
	r_factory = (MCBrowserFactoryRef)t_factory;
	return true;
}

////////////////////////////////////////////////////////////////////////////////
