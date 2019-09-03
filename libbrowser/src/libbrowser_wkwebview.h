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
	
	virtual bool GoBack();
	virtual bool GoForward();
	virtual bool GoToURL(const char *p_url);
	virtual bool EvaluateJavaScript(const char *p_script, char *&r_result);

	void SyncJavaScriptHandlers();

protected:
	bool GetUrl(char *& r_url);
	
	bool GetHTMLText(char *&r_htmltext);
	bool SetHTMLText(const char *p_htmltext);
	
	bool GetVerticalScrollbarEnabled(bool& r_value);
	bool SetVerticalScrollbarEnabled(bool p_value);
	bool GetHorizontalScrollbarEnabled(bool& r_value);
	bool SetHorizontalScrollbarEnabled(bool p_value);

	bool GetJavaScriptHandlers(char *&r_handlers);
	bool SetJavaScriptHandlers(const char *p_handlers);
	
	bool GetUserAgent(char *&r_useragent);
	bool SetUserAgent(const char *p_useragent);
	
	bool GetIsSecure(bool& r_value);

	bool GetAllowUserInteraction(bool& r_value);
	bool SetAllowUserInteraction(bool p_value);

private:
	bool GetView(WKWebView *&r_view);
	
	bool SyncJavaScriptHandlers(NSArray *p_handlers);
	
	WKWebView *m_view;
	com_livecode_libbrowser_MCWKWebViewNavigationDelegate *m_delegate;
	MCWKWebViewScriptMessageHandler *m_message_handler;

	char *m_js_handlers;
};

//////////

bool MCWKWebViewBrowserFactoryCreate(MCBrowserFactoryRef &r_factory);

#endif // __LIBBROWSER_WKWEBVIEW_H__
