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

#include <include/cef_app.h>

#include <Windows.h>

class MCCefWin32Browser : public MCCefBrowserBase
{
public:
	MCCefWin32Browser(HWND p_parent_window);
	virtual ~MCCefWin32Browser(void);

	bool GetHWND(HWND &r_hwnd);

	virtual void PlatformConfigureWindow(CefWindowInfo &r_info);

	virtual bool PlatformSetVisible(bool p_visible);
	virtual bool PlatformGetVisible(bool &r_visible);

	virtual bool PlatformGetRect(int32_t &r_left, int32_t &r_top, int32_t &r_right, int32_t &r_bottom);
	virtual bool PlatformSetRect(int32_t p_left, int32_t p_top, int32_t p_right, int32_t p_bottom);
	virtual bool PlatformGetWindowID(int32_t &r_id);

private:
	HWND m_parent_window;
};

const char *MCCefPlatformGetSubProcessName(void)
{
	return "revbrowser-cefprocess.exe";
}

bool MCCefPlatformCreateBrowser(int p_window_id, MCCefBrowserBase *&r_browser)
{
	MCCefWin32Browser *t_browser;
	t_browser = new MCCefWin32Browser((HWND)p_window_id);

	if (t_browser == nil)
		return false;

	r_browser = t_browser;

	return true;
}

void MCCefWin32Browser::PlatformConfigureWindow(CefWindowInfo &r_info)
{
	RECT t_rect;
	//int t_left, t_top, t_right, t_bottom;
	//GetRect(t_left, t_top, t_right, t_bottom);

	//::SetRect(&t_rect, t_left, t_top, t_right, t_bottom);
	::SetRect(&t_rect, 0, 0, 1, 1);

	r_info.SetAsChild(m_parent_window, t_rect);
}

void MCCefPlatformCloseBrowserWindow(CefRefPtr<CefBrowser> p_browser)
{
	HWND t_win;
	t_win = p_browser->GetHost()->GetWindowHandle();

	//SetParent(t_win, HWND_MESSAGE);
	//CloseWindow(t_win);
	DestroyWindow(t_win);
}

MCCefWin32Browser::MCCefWin32Browser(HWND p_parent_window) : MCCefBrowserBase()
{
	m_parent_window = p_parent_window;
}

MCCefWin32Browser::~MCCefWin32Browser(void)
{
}

bool MCCefWin32Browser::GetHWND(HWND &r_hwnd)
{
	CefRefPtr<CefBrowser> t_browser;
	t_browser = GetCefBrowser();

	if (t_browser == nil)
		return false;

	HWND t_handle;
	t_handle = t_browser->GetHost()->GetWindowHandle();

	if (t_handle == nil)
		return false;

	r_hwnd = t_handle;

	return true;
}

bool MCCefWin32Browser::PlatformGetRect(int32_t &r_left, int32_t &r_top, int32_t &r_right, int32_t &r_bottom)
{
	HWND t_handle;
	if (!GetHWND(t_handle))
		return false;

	RECT t_rect;
	if (!GetWindowRect(t_handle, &t_rect))
		return false;

	POINT t_origin;
	t_origin.x = t_rect.left;
	t_origin.y = t_rect.top;

	if (!ScreenToClient(m_parent_window, &t_origin))
		return false;

	OffsetRect(&t_rect, t_origin.x - t_rect.left, t_origin.y - t_rect.top);

	r_left = t_rect.left;
	r_top = t_rect.top;
	r_right = t_rect.right;
	r_bottom = t_rect.bottom;

	return true;
}

bool MCCefWin32Browser::PlatformSetRect(int32_t p_left, int32_t p_top, int32_t p_right, int32_t p_bottom)
{
	HWND t_handle;
	if (!GetHWND(t_handle))
		return false;

	return TRUE == MoveWindow(t_handle, p_left, p_top, p_right - p_left, p_bottom - p_top, FALSE);
}

bool MCCefWin32Browser::PlatformGetVisible(bool &r_visible)
{
	HWND t_handle;
	if (!GetHWND(t_handle))
		return false;

	r_visible = IsWindowVisible(t_handle) == TRUE;

	return true;
}

bool MCCefWin32Browser::PlatformSetVisible(bool p_visible)
{
	HWND t_handle;
	if (!GetHWND(t_handle))
		return false;

	ShowWindow(t_handle, p_visible ? SW_SHOW : SW_HIDE);

	return true;
}

bool MCCefWin32Browser::PlatformGetWindowID(int32_t &r_id)
{
	r_id = (int32_t) m_parent_window;

	return true;
}


static HINSTANCE s_instance = nil;

// DLL Entry Point
extern "C" BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
	switch (dwReason)
	{
	case DLL_PROCESS_ATTACH:
		s_instance = hInstance;
		break;

	case DLL_PROCESS_DETACH:
		break;

	}

	return TRUE;
}
