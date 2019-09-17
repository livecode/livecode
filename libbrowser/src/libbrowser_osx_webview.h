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

#ifndef __LIBBROWSER_OSX_WEBVIEW_H__
#define __LIBBROWSER_OSX_WEBVIEW_H__

#include "libbrowser_internal.h"

@class MCWebView;
@class MCWebViewFrameLoadDelegate;
@class MCWebViewPolicyDelegate;
@class MCWebUIDelegate;
@class MCWebViewProgressDelegate;

class MCWebViewBrowser : public MCBrowserBase
{
public:
	MCWebViewBrowser();
	virtual ~MCWebViewBrowser();
	
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

protected:
	bool GetUrl(char *& r_url);
	
	bool GetHTMLText(char *&r_htmltext);
	bool SetHTMLText(const char *p_htmltext);
	
	//bool GetVerticalScrollbarEnabled(bool& r_value);
	//bool SetVerticalScrollbarEnabled(bool p_value);
	//bool GetHorizontalScrollbarEnabled(bool& r_value);
	//bool SetHorizontalScrollbarEnabled(bool p_value);
	
	bool GetUserAgent(char*& r_user_agent);
	bool SetUserAgent(const char *p_user_agent);
	
	bool GetJavaScriptHandlers(char *&r_handlers);
	bool SetJavaScriptHandlers(const char *p_handlers);

	bool GetIsSecure(bool& r_value);

	bool GetAllowUserInteraction(bool& r_value);
	bool SetAllowUserInteraction(bool p_value);

	// Browser-specific actions
	bool ExecExecute(const char * p_script, char *&r_result);
	
	//UIScrollView *GetScrollView(void);
	
private:
	bool GetView(WebView *&r_view);
	
	bool SyncJavaScriptHandlers(NSArray *p_handlers);

	bool FrameIsSecure(WebFrame *p_frame);
	
	MCWebView *m_view;
	MCWebViewFrameLoadDelegate *m_delegate;
	MCWebViewPolicyDelegate *m_policy_delegate;
	MCWebUIDelegate *m_ui_delegate;
	MCWebViewProgressDelegate *m_progress_delegate;

	char *m_js_handlers;
	NSArray *m_js_handler_list;
};

//////////

bool MCWebViewBrowserFactoryCreate(MCBrowserFactoryRef &r_factory);

#endif // __LIBBROWSER_OSX_WEBVIEW_H__
