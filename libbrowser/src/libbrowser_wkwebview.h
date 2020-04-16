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

#ifndef __LIBBROWSER_WKWEBVIEW_H__
#define __LIBBROWSER_WKWEBVIEW_H__

#include "libbrowser_internal.h"

////////////////////////////////////////////////////////////////////////////////

@class com_livecode_libbrowser_MCWKWebViewNavigationDelegate;
@class MCWKWebViewScriptMessageHandler;

class MCWKWebViewBrowser : public MCBrowserBase
{
public:
	MCWKWebViewBrowser();
	virtual ~MCWKWebViewBrowser();

	bool Init(void);
	
	virtual void *GetNativeLayer();
	
	virtual bool GetRect(MCBrowserRect &r_rect);
	virtual bool SetRect(const MCBrowserRect &p_rect);
	
	virtual bool GetBoolProperty(MCBrowserProperty p_property, bool &r_value);
	virtual bool SetBoolProperty(MCBrowserProperty p_property, bool p_value);
	virtual bool GetStringProperty(MCBrowserProperty p_property, char *&r_utf8_string);
	virtual bool SetStringProperty(MCBrowserProperty p_property, const char *p_utf8_string);
	virtual bool GetIntegerProperty(MCBrowserProperty p_property, int32_t &r_value);
	virtual bool SetIntegerProperty(MCBrowserProperty p_property, int32_t p_value);

	virtual bool GoBack();
	virtual bool GoForward();
	virtual bool GoToURL(const char *p_url);
	virtual bool LoadHTMLText(const char *p_htmltext, const char *p_base_url);
	virtual bool StopLoading();
	virtual bool Reload();
	virtual bool EvaluateJavaScript(const char *p_script, char *&r_result);

	void SyncJavaScriptHandlers();

public:
	// return the webview used by the browser class
	bool GetWebView(WKWebView *&r_view);

protected:
	bool GetUrl(char *& r_url);
	
	bool GetHTMLText(char *&r_htmltext);
	bool SetHTMLText(const char *p_htmltext);
	
	bool GetVerticalScrollbarEnabled(bool& r_value);
	bool SetVerticalScrollbarEnabled(bool p_value);
	bool GetHorizontalScrollbarEnabled(bool& r_value);
	bool SetHorizontalScrollbarEnabled(bool p_value);
	bool GetScrollEnabled(bool& r_value);
	bool SetScrollEnabled(bool p_value);
	bool GetScrollCanBounce(bool& r_value);
	bool SetScrollCanBounce(bool p_value);

	bool GetJavaScriptHandlers(char *&r_handlers);
	bool SetJavaScriptHandlers(const char *p_handlers);
	
	bool GetUserAgent(char *&r_useragent);
	bool SetUserAgent(const char *p_useragent);
	
	bool GetIsSecure(bool& r_value);
	
	bool GetCanGoForward(bool& r_value);
	bool GetCanGoBack(bool& r_value);

	bool GetAllowUserInteraction(bool& r_value);
	bool SetAllowUserInteraction(bool p_value);
	
	bool GetDelayRequests(bool& r_value);
	bool SetDelayRequests(bool p_value);
	
	bool GetAutoFit(bool& r_value);
	bool SetAutoFit(bool p_value);
	
	bool GetDataDetectorTypes(int32_t &r_types);
	bool SetDataDetectorTypes(int32_t p_types);

	bool GetAllowsInlineMediaPlayback(bool& r_value);
	bool SetAllowsInlineMediaPlayback(bool p_value);

	bool GetMediaPlaybackRequiresUserAction(bool& r_value);
	bool SetMediaPlaybackRequiresUserAction(bool p_value);

private:
	bool GetContainerView(UIView *&r_view);
	
	bool Reconfigure(void);
	
	bool SyncJavaScriptHandlers(NSArray *p_handlers);
	
	WKWebView *m_view;
	UIView *m_container_view;
	com_livecode_libbrowser_MCWKWebViewNavigationDelegate *m_delegate;
	MCWKWebViewScriptMessageHandler *m_message_handler;
	WKUserContentController *m_content_controller;
	WKWebViewConfiguration *m_configuration;

	char *m_js_handlers;
	char *m_htmltext;
	char *m_url;
	
	bool m_autofit;
};

////////////////////////////////////////////////////////////////////////////////

bool MCWKWebViewBrowserFactoryCreate(MCBrowserFactoryRef &r_factory);

////////////////////////////////////////////////////////////////////////////////

#endif // __LIBBROWSER_WKWEBVIEW_H__
