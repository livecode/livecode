/* Copyright (C) 2003-2015 Runtime Revolution Ltd.
 
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

#ifndef __LIBBROWSER_CEF_H__
#define __LIBBROWSER_CEF_H__

#include "libbrowser.h"

#include <include/cef_browser.h>
#include <include/cef_client.h>

class MCCefBrowserClient;
class MCCefMessageResult;

enum MCCefAuthScheme
{
	kMCCefAuthBasic = 0,
	kMCCefAuthDigest,
	kMCCefAuthNTLM,
	kMCCefAuthNegotiate,
	kMCCefAuthSPDYProxy,
};

class MCCefBrowserBase : public MCBrowser
{
public:
	virtual void *GetNativeLayer();
	
	virtual bool GetRect(MCBrowserRect &r_rect);
	virtual bool SetRect(const MCBrowserRect &p_rect);
	
	virtual bool GetBoolProperty(MCBrowserProperty p_property, bool &r_value);
	virtual bool SetBoolProperty(MCBrowserProperty p_property, bool p_value);
	
	virtual bool GetStringProperty(MCBrowserProperty p_property, char *&r_utf8_string);
	virtual bool SetStringProperty(MCBrowserProperty p_property, const char *p_utf8_string);
	
	virtual bool GoBack();
	virtual bool GoForward();
	virtual bool GoToURL(const char *p_url);
	virtual bool EvaluateJavaScript(const char *p_script, char *&r_result);
	
private:
	CefRefPtr<CefBrowser> m_browser;
	CefRefPtr<MCCefBrowserClient> m_client;
	
	CefString m_user_agent;
	char *m_javascript_handlers;
	
	bool m_show_context_menu;
	bool m_allow_new_window;
	bool m_send_advanced_messages;
	int m_instance_id;
	
	bool SetJavaScriptHandlers(CefRefPtr<CefListValue> p_handlers);
	
	void WaitOnResult(void);
	bool GetMessageResult(CefProcessId p_target, CefRefPtr<CefProcessMessage> p_message, const MCCefMessageResult *&r_result);
	bool EvalJavaScript(const CefString &p_script, const MCCefMessageResult *&r_result);
	
	bool EvalJavaScript(const CefString &p_script, CefString &r_value);
	bool EvalJavaScript(const CefString &p_script, int &r_value);
	
	bool WaitOnResultString(CefString &r_result);
	bool GetMessageResultString(CefProcessId p_target, CefRefPtr<CefProcessMessage> p_message, CefString &r_result);
	
	bool GetOverflowHidden();
	void SetOverflowHidden(bool p_hidden);
	
public:
	bool GetUserAgent(CefString &r_user_agent);
	
	// CWebBrowserBase Interface
public:
	MCCefBrowserBase();
	virtual ~MCCefBrowserBase(void);
	
	bool Initialize();
	
	// Browser Properties
	
	virtual char *GetSource(void);
	virtual void SetSource(const char *p_source);
	
	virtual bool GetScrollbars(void);
	virtual void SetScrollbars(bool p_scrollbars);
	
	virtual char *GetUserAgent(void);
	virtual void SetUserAgent(const char *p_user_agent);
	
	virtual bool GetAllowNewWindow(void);
	virtual void SetAllowNewWindow(bool p_allow);
	
	virtual bool GetEnableContextMenu(void);
	virtual void SetEnableContextMenu(bool p_enable);
	
	virtual char *GetURL(void);
	virtual bool GetImage(void*& r_data, int& r_length);
	
	// Browser Actions
	
	virtual bool SetJavaScriptHandlers(const char *p_handlers);
	virtual bool GetJavaScriptHandlers(char *&r_handlers);
	
	// Platform-specific methods
	
	virtual void PlatformConfigureWindow(CefWindowInfo &r_info) = 0;
	
	virtual bool PlatformGetRect(MCBrowserRect &r_rect) = 0;
	virtual bool PlatformSetRect(const MCBrowserRect &p_rect) = 0;
	
	virtual bool PlatformGetNativeLayer(void *&r_layer) = 0;
	
	virtual bool PlatformGetAuthCredentials(bool p_is_proxy, const CefString &p_url, const CefString &p_realm, MCCefAuthScheme p_auth_scheme, CefString &r_user, CefString &r_password) = 0;
	
	// Access methods
	
	CefRefPtr<CefBrowser> GetCefBrowser(void);
	
	// Browser event handlers
	virtual void OnCefBrowserCreated(CefRefPtr<CefBrowser> p_browser);
	virtual void OnCefBrowserClosed(CefRefPtr<CefBrowser> p_browser);
	
	friend class MCCefBrowserClient;
};

class MCCefBrowserFactory : public MCBrowserFactory
{
public:
	MCCefBrowserFactory();
	virtual ~MCCefBrowserFactory();
	
	bool CreateBrowser(MCBrowser *&r_browser) override;
	
	//////////
	
	bool Initialize();
};

bool MCCefBrowserFactoryCreate(MCBrowserFactoryRef &r_factory);

bool MCCefPlatformCreateBrowser(void *p_parent_window, MCCefBrowserBase *&r_browser);
void MCCefPlatformCloseBrowserWindow(CefRefPtr<CefBrowser> p_browser);

const char *MCCefPlatformGetSubProcessName(void);

bool MCCefStringToUtf8String(const CefString &p_cef_string, char *&r_u8_string);
bool MCCefStringFromUtf8String(const char *p_u8_string, CefString &r_cef_string);
bool MCCefStringToUInt(const CefString &p_string, uint32_t &r_int);

bool MCCefAuthSchemeFromCefString(const CefString &p_string, MCCefAuthScheme &r_scheme);

//////////

#define MC_CEFMSG_EXECUTE_SCRIPT "cefbrowser_request_execute_script"
#define MC_CEFMSG_CALL_SCRIPT "cefbrowser_request_call_script"
#define MC_CEFMSG_GET_SELECTED_TEXT "cefbrowser_request_get_seleted_text"
#define MC_CEFMSG_GET_TITLE "cefbrowser_request_get_title"
#define MC_CEFMSG_SET_JS_HANDLER_LIST "cebrowser_request_set_js_handler_list"

#define MC_CEFMSG_JS_HANDLER "cefbrowser_js_handler"

#define MC_CEFMSG_RESULT "cefbrowser_result"

const char *MCCefPlatformGetCefLibraryPath(void);
const char *MCCefPlatformGetResourcesDirPath(void);
const char *MCCefPlatformGetLocalePath(void);

// AL-2015-02-17: [[ SB Inclusions ]] Work around problems linking to MCU_ functions from CEF

extern "C" void *MCU_loadmodule(const char *p_source);
extern "C" void MCU_unloadmodule(void *p_module);
extern "C" void *MCU_resolvemodulesymbol(void *p_module, const char *p_symbol);

// Engine wait loop hooks

typedef struct __MCRunloopAction *MCRunloopActionRef;
typedef void (MCRunloopActionCallback)(void *p_context);
extern "C" bool MCEngineAddRunloopAction(MCRunloopActionCallback p_callback, void *p_context, MCRunloopActionRef &r_action);
extern "C" void MCEngineRemoveRunloopAction(MCRunloopActionRef p_action);
extern "C" bool MCEngineRunloopWait();

extern "C" void MCWidgetEvalStackNativeView(void *&r_native_view);

#endif /* __LIBBROWSER_CEF_H__ */
