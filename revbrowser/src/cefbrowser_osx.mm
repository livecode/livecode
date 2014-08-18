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

#include "external.h"
#include "cefbrowser.h"

#include "core.h"

#include <AppKit/AppKit.h>

#include <include/cef_app.h>

#include <objc/objc-runtime.h>

#include "WebAuthenticationPanel.h"
#include <include/cef_url.h>

////////////////////////////////////////////////////////////////////////////////

bool MCCefStringFromNSString(NSString *p_string, CefString &r_string)
{
	bool t_success;
	
	unichar_t *t_chars;
	t_chars = nil;
	
	CefString t_string;
	
	// create array to store unicode chars
	t_success = MCMemoryNewArray([p_string length], t_chars);
	
	if (t_success)
	{
		// fetch chars from NSString object
		[p_string getCharacters:t_chars range:NSMakeRange(0, [p_string length])];
		
		// assign chars to CefString
		t_success = t_string.FromString(t_chars, [p_string length], true);
	}
	
	if (t_success)
		r_string = t_string;
	
	if (t_chars != nil)
		MCMemoryDeleteArray(t_chars);
	
	return t_success;
}

bool MCCefStringToNSString(const CefString &p_string, NSString *&r_string)
{
	bool t_success;
	t_success = true;
	
	NSString *t_string;
	
	t_success = nil != (t_string = [[NSString alloc] initWithCharacters:p_string.c_str() length:p_string.length()]);
	
	if (t_success)
		r_string = t_string;
	
	return t_success;
}

////////////////////////////////////////////////////////////////////////////////

class MCCefBrowserOSX : public MCCefBrowserBase
{
public:
	MCCefBrowserOSX(CefWindowHandle p_parent_window);
	virtual ~MCCefBrowserOSX(void);
	
	bool GetWindowHandle(CefWindowHandle &r_hwnd);
	
	virtual void PlatformConfigureWindow(CefWindowInfo &r_info);
	
	virtual bool PlatformSetVisible(bool p_visible);
	virtual bool PlatformGetVisible(bool &r_visible);
	
	virtual bool PlatformGetRect(int32_t &r_left, int32_t &r_top, int32_t &r_right, int32_t &r_bottom);
	virtual bool PlatformSetRect(int32_t p_left, int32_t p_top, int32_t p_right, int32_t p_bottom);
	virtual bool PlatformGetWindowID(int32_t &r_id);
	
	virtual bool PlatformGetAuthCredentials(bool p_is_proxy, const CefString &p_url, const CefString &p_realm, MCCefAuthScheme p_auth_scheme, CefString &r_user, CefString &r_password);
	
private:
	CefWindowHandle m_parent_window;
	int32_t m_left, m_top, m_right, m_bottom;
};

// IM-2014-08-12: [[ LibCef ]] helper app located in Frameworks folder of main app bundle.
const char *MCCefPlatformGetSubProcessName(void)
{
	static char *s_exe_path = nil;
	
	if (s_exe_path == nil)
	{
		NSBundle *t_bundle;
		t_bundle = [NSBundle mainBundle];
		
		NSString *t_path;
		t_path = [[t_bundle bundlePath] stringByAppendingPathComponent:@"Contents/Frameworks/revbrowser-cefprocess.app/Contents/MacOS/revbrowser-cefprocess"];
		
		/* UNCHECKED */ MCCStringClone([t_path cStringUsingEncoding:NSUTF8StringEncoding], s_exe_path);
	}
	
	return s_exe_path;
}

// IM-2014-08-12: [[ LibCef ]] CEF framework folder located in Frameworks folder of main app bundle.
const char *MCCefPlatformGetCefFrameworkFolder()
{
	static char *s_path = nil;
	
	if (s_path == nil)
	{
		NSBundle *t_bundle;
		t_bundle = [NSBundle mainBundle];
		
		NSString *t_cef_path;
		t_cef_path = [[t_bundle bundlePath] stringByAppendingPathComponent:@"Contents/Frameworks/Chromium Embedded Framework.framework"];
		
		/* UNCHECKED */ MCCStringClone([t_cef_path cStringUsingEncoding:NSUTF8StringEncoding], s_path);
	}
	
	return s_path;
}

bool MCCefPlatformCreateBrowser(int p_window_id, MCCefBrowserBase *&r_browser)
{
	NSWindow *t_app_window;
	t_app_window = [NSApp windowWithWindowNumber:p_window_id];
	
	if (t_app_window == nil)
		return false;
	
	MCCefBrowserOSX *t_browser;
	t_browser = new MCCefBrowserOSX([t_app_window contentView]);
	
	if (t_browser == nil)
		return false;
	
	r_browser = t_browser;
	
	return true;
}

void MCCefBrowserOSX::PlatformConfigureWindow(CefWindowInfo &r_info)
{
	NSRect t_win_rect;
	t_win_rect = [m_parent_window frame];
	
	// IM-2014-06-13: [[ Bug 12631 ]] Set initial window rect to the topleft of the window
	r_info.SetAsChild(m_parent_window, 0,t_win_rect.size.height,0,0);
    
	NSView *t_handle;
	if (!GetWindowHandle(t_handle))
		return false;
    
    t_handle = t_handle;
}

void MCCefPlatformCloseBrowserWindow(CefRefPtr<CefBrowser> p_browser)
{
	NSView *t_handle;
	t_handle = p_browser->GetHost()->GetWindowHandle();
	if (t_handle == nil)
		return;
	
	[t_handle removeFromSuperview];
}

MCCefBrowserOSX::MCCefBrowserOSX(CefWindowHandle p_parent_window) : MCCefBrowserBase()
{
	m_parent_window = p_parent_window;
	
	m_left = m_top = m_right = m_bottom = 0;
}

MCCefBrowserOSX::~MCCefBrowserOSX(void)
{
}

static unsigned int cef_com_runrev_livecode_nativeViewId(id self, SEL _cmd)
{
    return 0xffffffffU;
}

bool MCCefBrowserOSX::GetWindowHandle(CefWindowHandle &r_hwnd)
{
	CefRefPtr<CefBrowser> t_browser;
	t_browser = GetCefBrowser();
	
	if (t_browser == nil)
		return false;
	
	CefWindowHandle t_handle;
	t_handle = t_browser->GetHost()->GetWindowHandle();
	
	if (t_handle == nil)
		return false;
    
    // MW-2014-04-10: [[ Bug 12047 ]] First time we get the window handle
    //   inject the method that the engine needs to process focus.
    if (![t_handle respondsToSelector: @selector(com_runrev_livecode_nativeViewId)])
    {
        Class t_class;
        t_class = object_getClass(t_handle);
        class_addMethod(t_class, @selector(com_runrev_livecode_nativeViewId), (IMP)cef_com_runrev_livecode_nativeViewId, "I@:");
    }
	
	r_hwnd = t_handle;
	
	return true;
}

bool MCCefBrowserOSX::PlatformGetRect(int32_t &r_left, int32_t &r_top, int32_t &r_right, int32_t &r_bottom)
{
	// IM-2014-06-13: [[ Bug 12631 ]] Return the last rect set
	r_left = m_left;
	r_top = m_top;
	r_right = m_right;
	r_bottom = m_bottom;
	
	return true;
}

bool MCCefBrowserOSX::PlatformSetRect(int32_t p_left, int32_t p_top, int32_t p_right, int32_t p_bottom)
{
	NSView *t_handle;
	if (!GetWindowHandle(t_handle))
		return false;
	
	NSRect t_win_rect;
	t_win_rect = [[[t_handle window] contentView] frame];
	
	NSRect t_rect;
	t_rect = NSMakeRect(p_left, t_win_rect.size.height - p_bottom, p_right - p_left, p_bottom - p_top);
	
	[t_handle setFrame:t_rect];
	
	// IM-2014-06-13: [[ Bug 12631 ]] Store the rect values
	m_left = p_left;
	m_top = p_top;
	m_right = p_right;
	m_bottom = p_bottom;
	
	return true;
}

bool MCCefBrowserOSX::PlatformGetVisible(bool &r_visible)
{
	NSView *t_handle;
	if (!GetWindowHandle(t_handle))
		return false;
	
	r_visible = ![t_handle isHidden];
	
	return true;
}

bool MCCefBrowserOSX::PlatformSetVisible(bool p_visible)
{
	NSView *t_handle;
	if (!GetWindowHandle(t_handle))
		return false;
	
	[t_handle setHidden:!p_visible];
	
	return true;
}

bool MCCefBrowserOSX::PlatformGetWindowID(int32_t &r_id)
{
	r_id = (int32_t) [[m_parent_window window] windowNumber];
	
	return true;
}

////////////////////////////////////////////////////////////////////////////////

bool MCCefAuthSchemeToNSURLAuthenticationMethod(MCCefAuthScheme p_scheme, NSString *&r_method)
{
	switch (p_scheme) {
		case kMCCefAuthBasic:
			r_method = NSURLAuthenticationMethodHTTPBasic;
			break;
			
		case kMCCefAuthDigest:
			r_method = NSURLAuthenticationMethodHTTPDigest;
			break;
			
		case kMCCefAuthNegotiate:
			r_method = NSURLAuthenticationMethodNegotiate;
			break;
			
		case kMCCefAuthNTLM:
			r_method = NSURLAuthenticationMethodNTLM;
			break;
			
		default:
			// Unrecognised auth scheme - use default method
			r_method = NSURLAuthenticationMethodDefault;
			break;
	}
	
	return true;
}

bool CreateNSURLAuthenticationChallenge(bool p_is_proxy, const CefString &p_url, const CefString &p_realm, MCCefAuthScheme p_auth_scheme, NSURLAuthenticationChallenge *&r_challenge)
{
	bool t_success;
	t_success = true;
	
	NSString *t_scheme;
	t_scheme = nil;

	NSString *t_host;
	t_host = nil;
	
	NSString *t_method;
	t_method = nil;
	
	NSString *t_realm;
	t_realm = nil;
	
	uint32_t t_port;
	t_port = 0;
	
	CefURLParts t_parts;
	
	if (t_success)
		t_success = CefParseURL(p_url, t_parts);
	
	if (t_success)
		t_success = MCCefStringToNSString(CefString(&t_parts.scheme), t_scheme);
	if (t_success)
		t_success = MCCefStringToNSString(CefString(&t_parts.host), t_host);
	if (t_success)
	{
		CefString t_port_string(&t_parts.port);
		if (!t_port_string.empty())
			t_success = MCCefStringToUInt(t_port_string, t_port);
	}
	
	if (t_success)
		t_success = MCCefAuthSchemeToNSURLAuthenticationMethod(p_auth_scheme, t_method);
	
	if (t_success)
		t_success = MCCefStringToNSString(p_realm, t_realm);
	
	NSURLProtectionSpace *t_protection_space;
	t_protection_space = nil;
	
	if (t_success)
	{
		if (p_is_proxy)
		{
			t_protection_space = [[NSURLProtectionSpace alloc] initWithProxyHost:t_host port:t_port type:t_scheme realm:t_realm authenticationMethod:t_method];
			t_success = nil != t_protection_space;
		}
		else
		{
			t_protection_space = [[NSURLProtectionSpace alloc] initWithHost:t_host port:t_port protocol:t_scheme realm:t_realm authenticationMethod:t_method];
			t_success = nil != t_protection_space;
		}
	}
	
	if (t_host != nil)
		[t_host release];
	if (t_scheme != nil)
		[t_scheme release];
	if (t_realm != nil)
		[t_realm release];
	
	NSURLAuthenticationChallenge *t_challenge;
	t_challenge = nil;
	
	if (t_success)
	{
		t_challenge = [[NSURLAuthenticationChallenge alloc] initWithProtectionSpace:t_protection_space proposedCredential:nil previousFailureCount:0 failureResponse:nil error:nil sender:nil];
		t_success = nil != t_challenge;
	}

	if (t_protection_space != nil)
		[t_protection_space release];
	
	if (t_success)
		r_challenge = t_challenge;
	
	return t_success;
}

@interface MCCefBrowserAuthDialogCallback : NSObject
{
	CefString user;
	CefString password;
	
	bool cancelled;
}

@property (readonly) CefString &user;
@property (readonly) CefString &password;

- (void)authenticationDoneWithChallenge: (NSURLAuthenticationChallenge *)challenge result:(NSURLCredential *)credential;
- (id)init;

@end

@implementation MCCefBrowserAuthDialogCallback

@synthesize user;
@synthesize password;

- (void)authenticationDoneWithChallenge: (NSURLAuthenticationChallenge *)challenge result:(NSURLCredential *)credential
{
	cancelled = (credential == nil);
	
	if (!cancelled)
	{
		/* UNCHECKED */ MCCefStringFromNSString([credential user], user);
		if ([credential hasPassword])
			/* UNCHECKED */ MCCefStringFromNSString([credential password], password);
	}
}

- (id)init
{
	self = [super init];
	if (self == nil)
		return nil;
	
	return self;
}

@end

bool MCCefBrowserOSX::PlatformGetAuthCredentials(bool p_is_proxy, const CefString &p_url, const CefString &p_realm, MCCefAuthScheme p_auth_scheme, CefString &r_user, CefString &r_password)
{
	bool t_success;
	t_success = true;
	
	NSURLAuthenticationChallenge *t_challenge;
	t_challenge = nil;
	
	if (t_success)
		t_success = CreateNSURLAuthenticationChallenge(p_is_proxy, p_url, p_realm, p_auth_scheme, t_challenge);
	
	MCCefBrowserAuthDialogCallback* t_callback;
	t_callback = nil;
	
	if (t_success)
		t_success = nil != (t_callback = [[ MCCefBrowserAuthDialogCallback alloc] init]);
	
	if (t_success)
	{
		com_runrev_livecode_WebAuthenticationPanel *t_panel;
		t_panel = [[com_runrev_livecode_WebAuthenticationPanel alloc] initWithCallback: t_callback selector: @selector(authenticationDoneWithChallenge:result:)];
		[t_panel /*runAsSheetOnWindow: [self window]*/ runAsModalDialogWithChallenge: t_challenge];
		[t_panel release];
	}
	
	if (t_success)
	{
		r_user = t_callback.user;
		r_password = t_callback.password;
	}
	
	if (t_callback != nil)
		[t_callback release];
	
	return t_success;
}

