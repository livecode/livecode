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

#ifndef __MCCEF_BROWSER_H__
#define __MCCEF_BROWSER_H__

#include "revbrowser.h"

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

class MCCefBrowserBase : public CWebBrowserBase
{
private:
	CefRefPtr<CefBrowser> m_browser;

	CefRefPtr<MCCefBrowserClient> m_client;

	CefString m_user_agent;

	bool m_show_context_menu;
	bool m_allow_new_window;
	bool m_send_advanced_messages;
	int m_instance_id;

	bool SetJavaScriptHandlerEnabled(const CefString &p_handler, bool p_enabled);

	void WaitOnResult(void);
	bool GetMessageResult(CefProcessId p_target, CefRefPtr<CefProcessMessage> p_message, const MCCefMessageResult *&r_result);
	bool EvalJavaScript(const CefString &p_script, const MCCefMessageResult *&r_result);

	bool EvalJavaScript(const CefString &p_script, CefString &r_value);
	bool EvalJavaScript(const CefString &p_script, int &r_value);

	bool WaitOnResultString(CefString &r_result);
	bool GetMessageResultString(CefProcessId p_target, CefRefPtr<CefProcessMessage> p_message, CefString &r_result);



public:
	bool GetUserAgent(CefString &r_user_agent);

// CWebBrowserBase Interface
public:
	MCCefBrowserBase();
	virtual ~MCCefBrowserBase(void);

	bool Initialize();

	// Browser Properties

	bool GetVisible(void);
	void SetVisible(bool p_visible);

	char *GetBrowser(void);
	void SetBrowser(const char *p_type);

	bool GetMessages(void);
	void SetMessages(bool p_value);

	char *GetSelectedText(void);
	void SetSelectedText(const char *p_text);

	virtual bool GetOffline(void);
	virtual void SetOffline(bool p_value);

	virtual bool GetContextMenu(void);
	virtual void SetContextMenu(bool p_menu);

	virtual bool GetNewWindow(void);
	virtual void SetNewWindow(bool p_new_window);

	virtual char *GetSource(void);
	virtual void SetSource(const char *p_source);

	virtual bool GetScale(void);
	virtual void SetScale(bool p_scale);

	virtual bool GetBorder(void);
	virtual void SetBorder(bool p_border);

	bool GetOverflowHidden(void);
	void SetOverflowHidden(bool p_hidden);
	
	virtual bool GetScrollbars(void);
	virtual void SetScrollbars(bool p_scrollbars);

	virtual void GetRect(int& r_left, int& r_top, int& r_right, int& r_bottom);
	virtual void SetRect(int p_left, int p_top, int p_right, int p_bottom);

	virtual int GetInst(void);
	virtual void SetInst(int p_id);

	virtual int GetVScroll(void);
	virtual void SetVScroll(int p_vscroll_pixels);

	virtual int GetHScroll(void);
	virtual void SetHScroll(int p_hscroll_pixels);

	virtual uintptr_t GetWindowId(void);
	virtual void SetWindowId(uintptr_t p_new_id);

	virtual char *GetUserAgent(void);
	virtual void SetUserAgent(const char *p_user_agent);

	virtual bool GetBusy(void);
	virtual char *GetURL(void);
	virtual char *GetTitle(void);
	virtual bool GetImage(void*& r_data, int& r_length);
	virtual int GetFormattedHeight(void);
	virtual int GetFormattedWidth(void);
	virtual void GetFormattedRect(int& r_left, int& r_top, int& r_right, int& r_bottom);

	// Browser Actions

	virtual char *ExecuteScript(const char *p_javascript_string);
	virtual char *CallScript(const char *p_function_name, char **p_arguments, unsigned int p_argument_count);
	virtual bool FindString(const char *p_string, bool p_search_up);
	virtual void GoURL(const char *p_url, const char *target_frame = NULL);
	virtual void GoBack(void);
	virtual void GoForward(void);
	virtual void Focus(void);
	virtual void Unfocus(void);
	virtual void Refresh(void);
	virtual void Stop(void);
	virtual void Print(void);
	virtual void Redraw(void);
	virtual void MakeTextBigger(void);
	virtual void MakeTextSmaller(void);

	virtual void AddJavaScriptHandler(const char *p_handler);
	virtual void RemoveJavaScriptHandler(const char *p_handler);

	// Platform-specific methods

	virtual void PlatformConfigureWindow(CefWindowInfo &r_info) = 0;

	virtual bool PlatformGetVisible(bool &r_visible) = 0;
	virtual bool PlatformSetVisible(bool p_visible) = 0;

	virtual bool PlatformGetRect(int32_t &t_left, int32_t &t_top, int32_t &t_right, int32_t &t_bottom) = 0;
	virtual bool PlatformSetRect(int32_t t_left, int32_t t_top, int32_t t_right, int32_t t_bottom) = 0;

	virtual bool PlatformGetWindowID(uintptr_t &r_id) = 0;
	
	virtual bool PlatformGetAuthCredentials(bool p_is_proxy, const CefString &p_url, const CefString &p_realm, MCCefAuthScheme p_auth_scheme, CefString &r_user, CefString &r_password) = 0;

	// Access methods

	CefRefPtr<CefBrowser> GetCefBrowser(void);

	// Browser event handlers
	virtual void OnCefBrowserCreated(CefRefPtr<CefBrowser> p_browser);
	virtual void OnCefBrowserClosed(CefRefPtr<CefBrowser> p_browser);
};

bool MCCefPlatformCreateBrowser(int p_window_id, MCCefBrowserBase *&r_browser);
void MCCefPlatformCloseBrowserWindow(CefRefPtr<CefBrowser> p_browser);

bool MCCefStringToUtf8String(const CefString &p_cef_string, char *&r_u8_string);
bool MCCefStringFromUtf8String(const char *p_u8_string, CefString &r_cef_string);
bool MCCefStringToUInt(const CefString &p_string, uint32_t &r_int);

bool MCCefAuthSchemeFromCefString(const CefString &p_string, MCCefAuthScheme &r_scheme);

#endif /* __CEF_BROWSER_H__ */
