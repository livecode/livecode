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

#ifndef __REVBROWSER__
#define __REVBROWSER__

#include <core.h>

class CWebBrowserBase
{
public:
	virtual ~CWebBrowserBase(void) {};

	virtual void SetVisible(bool p_visible) = 0;
	virtual void SetBrowser(const char *p_type) = 0;
	virtual void SetMessages(bool p_value) = 0;
	virtual void SetSelectedText(const char *p_text) = 0;
	virtual void SetOffline(bool p_value) = 0;
	virtual void SetContextMenu(bool p_menu) = 0;
	virtual void SetNewWindow(bool p_new_window) = 0;
	virtual void SetSource(const char *p_source) = 0;
	virtual void SetScale(bool p_scale) = 0;
	virtual void SetBorder(bool p_border) = 0;
	virtual void SetScrollbars(bool p_scrollbars) = 0;
	virtual void SetRect(int p_left, int p_top, int p_right, int p_bottom) = 0;
	virtual void SetInst(int p_id) = 0;
	virtual void SetVScroll(int p_vscroll_pixels) = 0;
	virtual void SetHScroll(int p_hscroll_pixels) = 0;
	virtual void SetWindowId(uintptr_t p_new_id) = 0;
	virtual void SetUserAgent(const char *p_user_agent) = 0;

	virtual bool GetBusy(void) = 0;
	virtual bool GetMessages(void) = 0;
	virtual char *GetURL(void) = 0;
	virtual char *GetSource(void) = 0;
	virtual bool GetOffline(void) = 0;
	virtual bool GetVisible(void) = 0;
	virtual bool GetScrollbars(void) = 0;
	virtual bool GetBorder(void) = 0;
	virtual bool GetScale(void) = 0;
	virtual bool GetContextMenu(void) = 0;
	virtual char *GetTitle(void) = 0;
	virtual bool GetNewWindow(void) = 0;
	virtual char *GetSelectedText(void) = 0;
	virtual char *GetBrowser(void) = 0;
	virtual bool GetImage(void*& r_data, int& r_length) = 0;
	virtual void GetRect(int& r_left, int& r_top, int& r_right, int& r_bottom) = 0;
	virtual int GetInst(void) = 0;
	virtual int GetVScroll(void) = 0;
	virtual int GetHScroll(void) = 0;
	virtual int GetFormattedHeight(void) = 0;
	virtual int GetFormattedWidth(void) = 0;
	virtual void GetFormattedRect(int& r_left, int& r_top, int& r_right, int& r_bottom) = 0;
	virtual uintptr_t GetWindowId(void) = 0;
	virtual char *GetUserAgent(void) = 0;

	virtual char *ExecuteScript(const char *p_javascript_string) = 0;
	virtual char *CallScript(const char *p_function_name, char **p_arguments, unsigned int p_argument_count) = 0;
	virtual bool FindString(const char *p_string, bool p_search_up) = 0;
	virtual void GoURL(const char *p_url, const char *target_frame = nil) = 0;
	virtual void GoBack(void) = 0;
	virtual void GoForward(void) = 0;
	virtual void Focus(void) = 0;
	virtual void Unfocus(void) = 0;
	virtual void Refresh(void) = 0;
	virtual void Stop(void) = 0;
	virtual void Print(void) = 0;
	virtual void Redraw(void) = 0;
	virtual void MakeTextBigger(void) = 0;
	virtual void MakeTextSmaller(void) = 0;

	// IM-2014-03-06: [[ revBrowserCEF ]] Make LiveCode handler available to JavaScript
	virtual void AddJavaScriptHandler(const char *p_handler) = 0;
	// IM-2014-03-06: [[ revBrowserCEF ]] Unregister LiveCode handler from JavaScript
	virtual void RemoveJavaScriptHandler(const char *p_handler) = 0;
};

CWebBrowserBase *InstantiateBrowser(int p_window_id);
// IM-2014-03-18: [[ revBrowserCEF ]] Create new cef-based browser instance
CWebBrowserBase *MCCefBrowserInstantiate(int p_window_id);

// IM-2014-03-06: [[ revBrowserCEF ]] Send the handler message with the given args
void CB_Custom(int p_instance_id, const char *p_message, char **p_args, uint32_t p_arg_count, bool *r_cancel);

void CB_ElementEnter(int p_instance_id, const char *p_element);
void CB_ElementLeave(int p_instance_id, const char *p_element);
void CB_ElementClick(int p_instance_id, const char *p_element);

void CB_NavigateRequest(int p_instance_id, const char *p_url, bool *r_cancel);
void CB_NavigateFrameRequest(int p_instance_id, const char *p_url, bool *r_cancel);
void CB_DownloadRequest(int p_instance_id, const char *p_url, bool *r_cancel);

void CB_NavigateComplete(int p_instance_id, const char *p_url);
void CB_NavigateFrameComplete(int p_instance_id, const char *p_url);

void CB_DocumentComplete(int p_instance_id, const char *p_url);
void CB_DocumentFrameComplete(int p_instance_id, const char *p_url);

void CB_DocumentFailed(int p_instance_id, const char *p_url, const char *p_error);
void CB_DocumentFrameFailed(int p_instance_id, const char *p_url, const char *p_error);

void CB_CreateInstance(int p_instance_id);
void CB_DestroyInstance(int p_instance_id);

void CB_NewWindow(int p_instance_id, const char *p_url);

#endif
