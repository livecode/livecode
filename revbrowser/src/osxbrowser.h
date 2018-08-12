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

#ifndef __OSX_BROWSER__
#define __OSX_BROWSER__

#include <Carbon/Carbon.h>
#include <WebKit/WebKit.h>
#include <WebKit/HIWebView.h>
#include <WebKit/CarbonUtils.h>
#include <WebKit/WebUIDelegate.h>

#ifndef __REVBROWSER__
#include "revbrowser.h"
#endif

@class WebBrowserAdapter;

class TAltBrowser: public CWebBrowserBase 
{
public:
	TAltBrowser(void);
	virtual ~TAltBrowser(void);
	
	void init(uintptr_t p_window_id);
 
 	virtual void SetVisible(bool p_visible);
	virtual void SetBrowser(const char *p_type);
	virtual void SetMessages(bool p_value);
	virtual void SetSelectedText(const char *p_text);
	virtual void SetOffline(bool p_value);
	virtual void SetContextMenu(bool p_menu);
	virtual void SetNewWindow(bool p_new_window);
	virtual void SetSource(const char *p_source);
	virtual void SetScale(bool p_scale);
	virtual void SetBorder(bool p_border);
	virtual void SetScrollbars(bool p_scrollbars);
	virtual void SetRect(int p_left, int p_top, int p_right, int p_bottom);
	virtual void SetInst(int p_id);
	virtual void SetVScroll(int p_vscroll_pixels);
	virtual void SetHScroll(int p_hscroll_pixels);
	virtual void SetWindowId(uintptr_t p_new_id);
	virtual void SetUserAgent(const char *p_user_agent);

	virtual bool GetBusy(void);
	virtual bool GetMessages(void);
	virtual char *GetURL(void);
	virtual char *GetSource(void);
	virtual bool GetOffline(void);
	virtual bool GetVisible(void);
	virtual bool GetScrollbars(void);
	virtual bool GetBorder(void);
	virtual bool GetScale(void);
	virtual bool GetContextMenu(void);
	virtual char *GetTitle(void);
	virtual bool GetNewWindow(void);
	virtual char *GetSelectedText(void);
	virtual char *GetBrowser(void);
	virtual bool GetImage(void*& r_data, int& r_length);
	virtual void GetRect(int& r_left, int& r_top, int& r_right, int& r_bottom);
	virtual int GetInst(void);
	virtual int GetVScroll(void);
	virtual int GetHScroll(void);
	virtual int GetFormattedHeight(void);
	virtual int GetFormattedWidth(void);
	virtual void GetFormattedRect(int& r_left, int& r_top, int& r_right, int& r_bottom);
	virtual uintptr_t GetWindowId(void);
	virtual char *GetUserAgent(void);

	virtual char *ExecuteScript(const char *p_javascript_string);
	virtual char *CallScript(const char *p_function_name, char **p_arguments, unsigned int p_argument_count);
	virtual bool FindString(const char *p_string, bool p_search_up);
	virtual void GoURL(const char *p_url, const char *p_target_frame);
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
	
	bool m_lock_update;
 
 protected:
	bool scaleenabled;
	bool isvisible;
	bool allownewwindow;
	bool scrollbarsenabled;
	bool borderenabled;
	bool contextmenus;
	bool messages;
	
	int instance_id;
	
	uint32_t m_parent;
	
	Rect m_bounds;
	
	WebView *m_web_browser;
	
	WebBrowserAdapter *m_web_adapter;
	
    static OSStatus ParentEventHandler(EventHandlerCallRef p_call_chain, EventRef p_event, void *p_context);
    static OSStatus ContainerEventHandler(EventHandlerCallRef p_call_chain, EventRef p_event, void *p_context);
    static OSStatus WebViewEventHandler(EventHandlerCallRef p_call_chain, EventRef p_event, void *p_context);
};

#endif
