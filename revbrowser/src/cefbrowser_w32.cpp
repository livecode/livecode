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

#include "core.h"

#include <revolution/external.h>

#include "cefbrowser.h"

#include <include/cef_app.h>
#include <include/cef_parser.h>

#include <Windows.h>

#include "revbrowser.rc.h"

extern HINSTANCE MCWin32BrowserGetHINSTANCE();

bool MCCefStringFromWSTR(const WCHAR *p_string, CefString &r_string)
{
	return cef_string_from_wide(p_string, wcslen(p_string), r_string.GetWritableStruct());
}

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
	virtual bool PlatformGetWindowID(uintptr_t &r_id);

	virtual bool PlatformGetAuthCredentials(bool p_is_proxy, const CefString &p_url, const CefString &p_realm, MCCefAuthScheme p_auth_scheme, CefString &r_user, CefString &r_password);

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

bool MCCefPlatformCreateBrowser(int p_window_id, MCCefBrowserBase *&r_browser)
{
	MCCefWin32Browser *t_browser;
	t_browser = new (nothrow) MCCefWin32Browser((HWND)p_window_id);

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

bool MCCefWin32Browser::PlatformGetWindowID(uintptr_t &r_id)
{
	r_id = (int32_t) m_parent_window;

	return true;
}

////////////////////////////////////////////////////////////////////////////////

struct MCBrowserAuthDialogStrings
{
	char *title;
	char *reason;
	char *realm;
	char *secure;
	char *username_label;
	char *password_label;
	char *login_label;
	char *cancel_label;
};

////////////////////////////////////////////////////////////////////////////////

bool MCBrowserAuthDialogMakeStrings(const char *p_host, uint32_t p_port, const char *p_realm, bool p_obfuscated, MCBrowserAuthDialogStrings& r_strings)
{
	bool t_success;
	t_success = true;

	if (t_success)
		t_success = MCCStringClone("Authentication Required", r_strings . title);
	if (t_success)
	{
		if (p_port != 0)
			t_success = MCCStringFormat(r_strings . reason, "To view this page, you must log into this area on %s:%u", p_host, p_port);
		else
			t_success = MCCStringFormat(r_strings . reason, "To view this page, you must log into this area on %s", p_host, p_port);
	}
	if (t_success)
		t_success = MCCStringClone(p_realm == nil || *p_realm == '\0' ? "(unknown)" : p_realm, r_strings . realm);
	if (t_success)
		t_success = MCCStringClone(p_obfuscated ? "Your password will be sent securely." : "Your password will be sent unencrypted.", r_strings . secure);
	if (t_success)
		t_success = MCCStringClone("Username:", r_strings . username_label);
	if (t_success)
		t_success = MCCStringClone("Password:", r_strings . password_label);
	if (t_success)
		t_success = MCCStringClone("Cancel", r_strings . cancel_label);
	if (t_success)
		t_success = MCCStringClone("Log In", r_strings . login_label);

	return t_success;
}

bool MCCefAuthIsObfuscated(const CefString &p_scheme, MCCefAuthScheme p_auth_scheme)
{
	if (p_scheme == "https")
		return true;

	if (p_auth_scheme == kMCCefAuthDigest || p_auth_scheme == kMCCefAuthNegotiate || p_auth_scheme == kMCCefAuthNTLM)
		return true;

	return false;
}

bool MCBrowserAuthDialogMakeStrings(const CefString &p_url, const CefString &p_realm, MCCefAuthScheme p_auth_scheme, MCBrowserAuthDialogStrings& r_strings)
{
	bool t_success;
	t_success = true;

	CefURLParts t_url_parts;
	t_success = CefParseURL(p_url, t_url_parts);

	char *t_host;
	t_host = nil;

	char *t_realm;
	t_realm = nil;

	uint32_t t_port;
	t_port = 0;

	if (t_success)
		t_success = MCCefStringToUtf8String(CefString(&t_url_parts.host), t_host);
	if (t_success)
		t_success = MCCefStringToUtf8String(p_realm, t_realm);
	if (t_success)
	{
		CefString t_port_string(&t_url_parts.port);
		if (!t_port_string.empty())
			t_success = MCCefStringToUInt(t_port_string, t_port);
	}

	if (t_success)
	{
		bool t_obfuscated;
		t_obfuscated = MCCefAuthIsObfuscated(CefString(&t_url_parts.scheme), p_auth_scheme);

		t_success = MCBrowserAuthDialogMakeStrings(t_host, t_port, t_realm, t_obfuscated, r_strings);
	}

	if (t_host != nil)
		MCCStringFree(t_host);
	if (t_realm != nil)
		MCCStringFree(t_realm);

	return t_success;
}


void MCBrowserAuthDialogFreeStrings(MCBrowserAuthDialogStrings& p_strings)
{
	MCCStringFree(p_strings . title);
	MCCStringFree(p_strings . reason);
	MCCStringFree(p_strings . realm);
	MCCStringFree(p_strings . secure);
	MCCStringFree(p_strings . username_label);
	MCCStringFree(p_strings . password_label);
	MCCStringFree(p_strings . cancel_label);
	MCCStringFree(p_strings . login_label);
}

////////////////////////////////////////////////////////////////////////////////

struct AuthDialogState
{
	// Owner
	HWND owner;

	// The strings
	MCBrowserAuthDialogStrings strings;

	// Return parameters
	CefString username;
	CefString password;
	bool login;
};

static void SetDlgItemTextWithCString(HWND p_dialog, int p_item, const char *p_string)
{
	wchar_t *t_w_string;
	if (!MCCStringToUnicode(p_string, t_w_string))
		return;

	SetDlgItemTextW(p_dialog, p_item, t_w_string);

	MCMemoryDeleteArray(t_w_string);
}

static bool GetDlgItemTextAsCefString(HWND p_dialog, int p_item, CefString &r_string)
{
	WCHAR t_w_string[256];
	if (GetDlgItemTextW(p_dialog, p_item, t_w_string, 256) == 0)
		t_w_string[0] = '\0';

	return MCCefStringFromWSTR(t_w_string, r_string);
}

static INT_PTR CALLBACK AuthDialogProc(HWND p_dialog, UINT p_message, WPARAM p_wparam, LPARAM p_lparam)
{
	AuthDialogState *t_state;
	t_state = (AuthDialogState *)GetWindowLongPtrW(p_dialog, DWLP_USER);

	switch(p_message)
	{
	case WM_INITDIALOG:
		{
			SetWindowLongPtrW(p_dialog, DWLP_USER, (LONG)p_lparam);
			t_state = (AuthDialogState *)p_lparam;

			SetDlgItemTextWithCString(p_dialog, IDC_AUTH_USERNAME_LABEL, t_state -> strings . username_label);
			SetDlgItemTextWithCString(p_dialog, IDC_AUTH_PASSWORD_LABEL, t_state -> strings . password_label);
			SetDlgItemTextWithCString(p_dialog, IDOK, t_state -> strings . login_label);
			SetDlgItemTextWithCString(p_dialog, IDCANCEL, t_state -> strings . cancel_label);

			SetDlgItemTextWithCString(p_dialog, IDC_AUTH_REASON, t_state -> strings . reason);
			SetDlgItemTextWithCString(p_dialog, IDC_AUTH_REALM, t_state -> strings . realm);
			SetDlgItemTextWithCString(p_dialog, IDC_AUTH_SECURE, t_state -> strings . secure);

			RECT t_owner_rect, t_rect;
			GetWindowRect(t_state -> owner, &t_owner_rect);
			GetWindowRect(p_dialog, &t_rect);

			MoveWindow(p_dialog,
				(t_owner_rect . right + t_owner_rect . left) / 2 - (t_rect . right - t_rect . left) / 2,
				(t_owner_rect . bottom + t_owner_rect . top) / 2 - (t_rect . bottom - t_rect . top) / 2,
				t_rect . right - t_rect . left, t_rect . bottom - t_rect . top, TRUE);
		}
		return TRUE;
 
	case WM_COMMAND:
		{
			if (LOWORD(p_wparam) == IDOK)
			{
				if (GetDlgItemTextAsCefString(p_dialog, IDC_AUTH_USERNAME, t_state -> username) &&
					GetDlgItemTextAsCefString(p_dialog, IDC_AUTH_PASSWORD, t_state -> password))
				{
					t_state -> login = true;
					EndDialog(p_dialog, 1);
				}
				else
					EndDialog(p_dialog, 0);
			}
			else if (LOWORD(p_wparam) == IDCANCEL)
			{
				t_state -> username.clear();
				t_state -> password.clear();
				t_state -> login = false;
				EndDialog(p_dialog, 1);
			}
		}
		return TRUE;
	}

	return FALSE;
}

bool MCCefWin32Browser::PlatformGetAuthCredentials(bool p_is_proxy, const CefString &p_url, const CefString &p_realm, MCCefAuthScheme p_auth_scheme, CefString &r_user, CefString &r_password)
{
	bool t_success;
	t_success = true;

	AuthDialogState t_state;
	MCMemoryClear(&t_state.strings, sizeof(t_state.strings));

	if (t_success)
		t_success = MCBrowserAuthDialogMakeStrings(p_url, p_realm, p_auth_scheme, t_state.strings);

	if (t_success)
	{
		t_state.owner = m_parent_window;
		t_state.login = false;

		DialogBoxParamW(MCWin32BrowserGetHINSTANCE(), MAKEINTRESOURCEW(IDD_AUTH_DIALOG), (HWND)m_parent_window, AuthDialogProc, (LPARAM)&t_state);
	}

	MCBrowserAuthDialogFreeStrings(t_state.strings);

	return t_success;
}

////////////////////////////////////////////////////////////////////////////////

