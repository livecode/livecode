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

#include "core.h"

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

// IM-2014-03-25: [[ revBrowserCEF ]] Return path containing the revbrowser dll
const char *MCCefWin32GetExternalPath(void)
{
	static char *s_external_path = nil;

	if (s_external_path == nil)
	{
		bool t_success;
		t_success = true;

		HMODULE t_hm;
		t_hm = nil;

		t_success = 0 != GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCSTR) &MCCefWin32GetExternalPath, &t_hm);

		int32_t t_buffer_size, t_name_size;
		t_buffer_size = 32;

		char *t_buffer;
		t_buffer = nil;

		do
		{
			t_buffer_size *= 2;
			t_success = MCMemoryReallocate(t_buffer, t_buffer_size, t_buffer);

			if (t_success)
				t_success = 0 != (t_name_size = GetModuleFileNameA(t_hm, t_buffer, t_buffer_size));
		} while (t_success && t_name_size == t_buffer_size);

		if (t_success)
		{
			// remove dll component from path
			uint32_t t_index;
			if (MCCStringLastIndexOf(t_buffer, '\\', t_index))
				t_buffer[t_index] = '\0';
		}

		if (t_success)
			s_external_path = t_buffer;
		else if (t_buffer != nil)
			MCMemoryDeallocate(t_buffer);
	}

	return s_external_path;
}

bool MCCefWin32AppendPath(const char *p_base, const char *p_path, char *&r_path)
{
	if (p_base == nil)
		return MCCStringClone(p_path, r_path);
	else if (MCCStringEndsWith(p_base, "\\"))
		return MCCStringFormat(r_path, "%s%s", p_base, p_path);
	else
		return MCCStringFormat(r_path, "%s\\%s", p_base, p_path);
}

// IM-2014-03-25: [[ revBrowserCEF ]] locales located in CEF subfolder relative to revbrowser dll
const char *MCCefPlatformGetLocalePath(void)
{
	static char *s_locale_path = nil;

	if (s_locale_path == nil)
		/* UNCHECKED */ MCCefWin32AppendPath(MCCefWin32GetExternalPath(), "CEF\\locales", s_locale_path);

	return s_locale_path;
}

// IM-2014-03-25: [[ revBrowserCEF ]] subprocess executable located in CEF subfolder relative to revbrowser dll
const char *MCCefPlatformGetSubProcessName(void)
{
	static char *s_exe_path = nil;

	if (s_exe_path == nil)
		/* UNCHECKED */ MCCefWin32AppendPath(MCCefWin32GetExternalPath(), "CEF\\revbrowser-cefprocess.exe", s_exe_path);

	return s_exe_path;
}

// IM-2014-03-25: [[ revBrowserCEF ]] libcef dll located in CEF subfolder relative to revbrowser dll
const char *MCCefPlatformGetCefLibraryPath(void)
{
	static char *s_lib_path = nil;

	if (s_lib_path == nil)
		/* UNCHECKED */ MCCefWin32AppendPath(MCCefWin32GetExternalPath(), "CEF\\libcef.dll", s_lib_path);

	return s_lib_path;
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

