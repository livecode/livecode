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

#include <core.h>

#include <libbrowser.h>
#include "libbrowser_cef.h"

#include <include/cef_app.h>
#include <include/cef_parser.h>

#include <Windows.h>

#include "libbrowser_win.rc.h"

bool MCCefStringFromWSTR(const WCHAR *p_string, CefString &r_string)
{
	return cef_string_from_wide(p_string, wcslen(p_string), r_string.GetWritableStruct());
}

class MCCefWin32Browser : public MCCefBrowserBase
{
public:
	MCCefWin32Browser(HWND p_parent_window);
	virtual ~MCCefWin32Browser(void);

	virtual void OnNavigationBegin(bool p_in_frame, const char *p_url);
	virtual void OnNavigationComplete(bool p_in_frame, const char *p_url);
	virtual void OnNavigationFailed(bool p_in_frame, const char *p_url, const char *p_error);
	virtual void OnDocumentLoadBegin(bool p_in_frame, const char *p_url);
	virtual void OnDocumentLoadComplete(bool p_in_frame, const char *p_url);
	virtual void OnDocumentLoadFailed(bool p_in_frame, const char *p_url, const char *p_error);
	
	virtual void OnNavigationRequestUnhandled(bool p_in_frame, const char *p_url);
	
	virtual void OnJavaScriptCall(const char *p_handler, MCBrowserListRef p_params);
	
	bool GetHWND(HWND &r_hwnd);

	virtual void PlatformConfigureWindow(CefWindowInfo &r_info);

	virtual bool PlatformGetNativeLayer(void *&r_layer);	

	virtual bool PlatformSetVisible(bool p_visible);
	virtual bool PlatformGetVisible(bool &r_visible);

	virtual bool PlatformGetRect(MCBrowserRect &r_rect);
	virtual bool PlatformSetRect(const MCBrowserRect &p_rect);
	virtual bool PlatformGetWindowID(int32_t &r_id);

	virtual bool PlatformGetAuthCredentials(bool p_is_proxy, const CefString &p_url, const CefString &p_realm, MCCefAuthScheme p_auth_scheme, CefString &r_user, CefString &r_password);

	virtual bool PlatformGetAllowUserInteraction(bool &r_allow_interaction);
	virtual bool PlatformSetAllowUserInteraction(bool p_allow_interaction);

private:
	bool CreateMessageWindow();
	bool DestroyMessageWindow();

	bool PostBrowserNavigationEvent(MCBrowserNavigationEventType p_type, MCBrowserNavigationState p_state, bool p_frame, const char *p_url, const char *p_error);
	bool PostJavaScriptCall(const char *p_handler, MCBrowserListRef p_params);

	HWND m_parent_window;
	HWND m_message_window;
};

bool MCCefPlatformCreateBrowser(void *p_display, void *p_parent_view, MCCefBrowserBase *&r_browser)
{
	MCCefWin32Browser *t_browser;
	t_browser = new (nothrow) MCCefWin32Browser((HWND)p_parent_view);

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

bool MCCefWin32Browser::PlatformGetNativeLayer(void *&r_layer)
{
	return GetHWND((HWND&)r_layer);
}

MCCefWin32Browser::MCCefWin32Browser(HWND p_parent_window) : MCCefBrowserBase()
{
	m_parent_window = p_parent_window;

	CreateMessageWindow();
}

MCCefWin32Browser::~MCCefWin32Browser(void)
{
	DestroyMessageWindow();
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

bool MCCefWin32Browser::PlatformGetRect(MCBrowserRect &r_rect)
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

	r_rect.left = t_rect.left;
	r_rect.top = t_rect.top;
	r_rect.right = t_rect.right;
	r_rect.bottom = t_rect.bottom;

	return true;
}

bool MCCefWin32Browser::PlatformSetRect(const MCBrowserRect &p_rect)
{
	HWND t_handle;
	if (!GetHWND(t_handle))
		return false;

	return TRUE == MoveWindow(t_handle, p_rect.left, p_rect.top, p_rect.right - p_rect.left, p_rect.bottom - p_rect.top, FALSE);
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

bool MCCefWin32Browser::PlatformGetAllowUserInteraction(bool &r_value)
{
	HWND t_handle;
	if (!GetHWND(t_handle))
		return false;

	r_value = IsWindowEnabled(t_handle) == TRUE;
	return true;
}

bool MCCefWin32Browser::PlatformSetAllowUserInteraction(bool p_value)
{
	HWND t_handle;
	if (!GetHWND(t_handle))
		return false;

	return EnableWindow(t_handle, (p_value) ? TRUE : FALSE) == TRUE;
}

////////////////////////////////////////////////////////////////////////////////

void MCCefWin32Browser::OnNavigationBegin(bool p_in_frame, const char *p_url)
{
	PostBrowserNavigationEvent(kMCBrowserNavigationEventTypeNavigate, kMCBrowserNavigationStateBegin, p_in_frame, p_url, nil);
}

void MCCefWin32Browser::OnNavigationComplete(bool p_in_frame, const char *p_url)
{
	PostBrowserNavigationEvent(kMCBrowserNavigationEventTypeNavigate, kMCBrowserNavigationStateComplete, p_in_frame, p_url, nil);
}

void MCCefWin32Browser::OnNavigationFailed(bool p_in_frame, const char *p_url, const char *p_error)
{
	PostBrowserNavigationEvent(kMCBrowserNavigationEventTypeNavigate, kMCBrowserNavigationStateFailed, p_in_frame, p_url, p_error);
}

void MCCefWin32Browser::OnDocumentLoadBegin(bool p_in_frame, const char *p_url)
{
	PostBrowserNavigationEvent(kMCBrowserNavigationEventTypeDocumentLoad, kMCBrowserNavigationStateBegin, p_in_frame, p_url, nil);
}

void MCCefWin32Browser::OnDocumentLoadComplete(bool p_in_frame, const char *p_url)
{
	PostBrowserNavigationEvent(kMCBrowserNavigationEventTypeDocumentLoad, kMCBrowserNavigationStateComplete, p_in_frame, p_url, nil);
}

void MCCefWin32Browser::OnDocumentLoadFailed(bool p_in_frame, const char *p_url, const char *p_error)
{
	PostBrowserNavigationEvent(kMCBrowserNavigationEventTypeDocumentLoad, kMCBrowserNavigationStateFailed, p_in_frame, p_url, p_error);
}

void MCCefWin32Browser::OnNavigationRequestUnhandled(bool p_in_frame, const char *p_url)
{
	PostBrowserNavigationEvent(kMCBrowserNavigationEventTypeNavigate, kMCBrowserNavigationStateUnhandled, p_in_frame, p_url, nil);
}

void MCCefWin32Browser::OnJavaScriptCall(const char *p_handler, MCBrowserListRef p_params)
{
	PostJavaScriptCall(p_handler, p_params);
}
	
////////////////////////////////////////////////////////////////////////////////

HINSTANCE MCWin32BrowserGetHINSTANCE();

#define MCCEFWIN32_MSGWNDCLASS "MCCEFWIN32MSGWINDOW"
#define MCCEFWIN32_MESSAGE_BROWSER_NAVIGATION_EVENT (WM_USER + 0)
#define MCCEFWIN32_MESSAGE_JAVASCRIPT_CALL (WM_USER + 1)

LRESULT CALLBACK MCCefWin32MessageWndProc(HWND p_hwnd, UINT p_message, WPARAM p_wparam, LPARAM p_lparam);
bool MCCefWin32Browser::CreateMessageWindow()
{
	HINSTANCE t_hinstance;
	t_hinstance = MCWin32BrowserGetHINSTANCE();

	WNDCLASSEX t_wnd_class;
	MCBrowserMemoryClear(t_wnd_class);
	t_wnd_class.cbSize = sizeof(WNDCLASSEX);
	t_wnd_class.lpfnWndProc = MCCefWin32MessageWndProc;
	t_wnd_class.hInstance = t_hinstance;
	t_wnd_class.lpszClassName = MCCEFWIN32_MSGWNDCLASS;
	RegisterClassEx(&t_wnd_class);

	m_message_window = CreateWindow(MCCEFWIN32_MSGWNDCLASS, 0, 0, 0, 0, 0, 0, HWND_MESSAGE, 0, t_hinstance, 0);

	if (m_message_window == nil)
		return false;

	SetWindowLongPtr(m_message_window, GWLP_USERDATA, (LONG_PTR)this);

	return m_message_window != nil;
}

bool MCCefWin32Browser::DestroyMessageWindow()
{
	if (m_message_window == nil)
		return true;

	if (!DestroyWindow(m_message_window))
		return false;

	m_message_window = nil;
	return true;
}

struct MCCefBrowserNavigationEvent
{
	MCBrowserNavigationEventType type;
	MCBrowserNavigationState state;
	bool frame;
	char *url;
	char *error;
};

struct MCCefBrowserJavaScriptCallEvent
{
	char *handler;
	MCBrowserListRef params;
};

void MCCefBrowserNavigationEventDestroy(MCCefBrowserNavigationEvent *p_event)
{
	if (p_event != nil)
	{
		if (p_event->url != nil)
			MCCStringFree(p_event->url);
		if (p_event->error != nil)
			MCCStringFree(p_event->error);

		MCBrowserMemoryDelete(p_event);
	}
}

bool MCCefBrowserNavigationEventCreate(MCBrowserNavigationEventType p_type, MCBrowserNavigationState p_state, bool p_frame, const char *p_url, const char *p_error, MCCefBrowserNavigationEvent *&r_event)
{
	bool t_success;
	t_success = true;

	MCCefBrowserNavigationEvent *t_event;
	t_event = nil;

	t_success = MCBrowserMemoryNew(t_event);

	if (t_success && p_url != nil)
		t_success = MCCStringClone(p_url, t_event->url);

	if (t_success && p_error != nil)
		t_success = MCCStringClone(p_error, t_event->error);

	if (t_success)
	{
		t_event->type = p_type;
		t_event->state = p_state;
		t_event->frame = p_frame;

		r_event = t_event;
	}
	else
		MCCefBrowserNavigationEventDestroy(t_event);

	return t_success;
}

void MCCefBrowserJavaScriptCallEventDestroy(MCCefBrowserJavaScriptCallEvent *p_event)
{
	if (p_event != nil)
	{
		if (p_event->handler != nil)
			MCCStringFree(p_event->handler);
		if (p_event->params != nil)
			MCBrowserListRelease(p_event->params);

		MCBrowserMemoryDelete(p_event);
	}
}

bool MCCefBrowserJavaScriptCallEventCreate(const char *p_handler, MCBrowserListRef p_params, MCCefBrowserJavaScriptCallEvent *&r_event)
{
	bool t_success;
	t_success = true;

	MCCefBrowserJavaScriptCallEvent *t_event;
	t_event = nil;

	t_success = MCBrowserMemoryNew(t_event);

	if (t_success)
		t_success = MCCStringClone(p_handler, t_event->handler);

	if (t_success)
	{
		t_event->params = MCBrowserListRetain(p_params);
		r_event = t_event;
	}
	else
		MCCefBrowserJavaScriptCallEventDestroy(t_event);

	return t_success;
}

bool MCCefWin32Browser::PostBrowserNavigationEvent(MCBrowserNavigationEventType p_type, MCBrowserNavigationState p_state, bool p_frame, const char *p_url, const char *p_error)
{
	MCCefBrowserNavigationEvent *t_event;
	t_event = nil;
	if (!MCCefBrowserNavigationEventCreate(p_type, p_state, p_frame, p_url, p_error, t_event))
		return false;

	if (!PostMessage(m_message_window, MCCEFWIN32_MESSAGE_BROWSER_NAVIGATION_EVENT, (WPARAM)t_event, 0))
	{
		MCCefBrowserNavigationEventDestroy(t_event);
		return false;
	}

	return true;
}

bool MCCefWin32Browser::PostJavaScriptCall(const char *p_handler, MCBrowserListRef p_params)
{
	MCCefBrowserJavaScriptCallEvent *t_event;
	t_event = nil;
	if (!MCCefBrowserJavaScriptCallEventCreate(p_handler, p_params, t_event))
		return false;

	if (!PostMessage(m_message_window, MCCEFWIN32_MESSAGE_JAVASCRIPT_CALL, (WPARAM)t_event, 0))
	{
		MCCefBrowserJavaScriptCallEventDestroy(t_event);
		return false;
	}

	return true;
}

LRESULT CALLBACK MCCefWin32MessageWndProc(HWND p_hwnd, UINT p_message, WPARAM p_wparam, LPARAM p_lparam)
{
	MCCefWin32Browser *t_browser;
	t_browser = reinterpret_cast<MCCefWin32Browser*>(GetWindowLongPtr(p_hwnd, GWLP_USERDATA));

	switch (p_message)
	{
	case MCCEFWIN32_MESSAGE_BROWSER_NAVIGATION_EVENT:
		{
			MCCefBrowserNavigationEvent *t_event;
			t_event = reinterpret_cast<MCCefBrowserNavigationEvent*>(p_wparam);

			MCBrowserEventHandler *t_event_handler;
			t_event_handler = t_browser->GetEventHandler();

			if (t_event_handler != nil)
			{
				switch (t_event->type)
				{
				case kMCBrowserNavigationEventTypeNavigate:
					switch (t_event->state)
					{
					case kMCBrowserNavigationStateBegin:
						t_event_handler->OnNavigationBegin(t_browser, t_event->frame, t_event->url);
						break;

					case kMCBrowserNavigationStateComplete:
						t_event_handler->OnNavigationComplete(t_browser, t_event->frame, t_event->url);
						break;

					case kMCBrowserNavigationStateFailed:
						t_event_handler->OnNavigationFailed(t_browser, t_event->frame, t_event->url, t_event->error);
						break;

					case kMCBrowserNavigationStateUnhandled:
						t_event_handler->OnNavigationRequestUnhandled(t_browser, t_event->frame, t_event->url);
						break;
					}
					break;

				case kMCBrowserNavigationEventTypeDocumentLoad:
					switch (t_event->state)
					{
					case kMCBrowserNavigationStateBegin:
						t_event_handler->OnDocumentLoadBegin(t_browser, t_event->frame, t_event->url);
						break;

					case kMCBrowserNavigationStateComplete:
						t_event_handler->OnDocumentLoadComplete(t_browser, t_event->frame, t_event->url);
						break;

					case kMCBrowserNavigationStateFailed:
						t_event_handler->OnDocumentLoadFailed(t_browser, t_event->frame, t_event->url, t_event->error);
						break;
					}
					break;
				}
			}

			MCCefBrowserNavigationEventDestroy(t_event);
		}
		break;

	case MCCEFWIN32_MESSAGE_JAVASCRIPT_CALL:
		{
			MCCefBrowserJavaScriptCallEvent *t_event;
			t_event = reinterpret_cast<MCCefBrowserJavaScriptCallEvent*>(p_wparam);

			MCBrowserJavaScriptHandler *t_handler;
			t_handler = t_browser->GetJavaScriptHandler();

			if (t_handler != nil)
			{
				t_handler->OnJavaScriptCall(t_browser, t_event->handler, t_event->params);
			}

			MCCefBrowserJavaScriptCallEventDestroy(t_event);
		}
		break;

	default:
		break;
	}

	return DefWindowProc(p_hwnd, p_message, p_wparam, p_lparam);
}

////////////////////////////////////////////////////////////////////////////////

// IM-2014-01-28: [[ HiDPI ]] Weak-linked IsProcessDPIAware function
typedef BOOL (WINAPI *IsProcessDPIAwarePtr)(VOID);
bool MCCefWin32IsProcessDPIAware(BOOL &r_result)
{
	static IsProcessDPIAwarePtr s_IsProcessDPIAware = NULL;
	static bool s_init = true;

	if (s_init)
	{
		s_IsProcessDPIAware = (IsProcessDPIAwarePtr)GetProcAddress(GetModuleHandleA("user32.dll"), "IsProcessDPIAware");
		s_init = false;
	}

	if (s_IsProcessDPIAware == NULL)
		return false;

	r_result = s_IsProcessDPIAware();

	return true;
}

bool MCCefPlatformGetHiDPIEnabled()
{
	BOOL t_result;
	if (!MCCefWin32IsProcessDPIAware(t_result))
		return false;

	return t_result;
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

HINSTANCE MCWin32BrowserGetHINSTANCE()
{
	HINSTANCE t_instance;
	t_instance = nil;
	GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCSTR)MCWin32BrowserGetHINSTANCE, &t_instance);

	return t_instance;
}

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
	MCBrowserMemoryClear(&t_state.strings, sizeof(t_state.strings));

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
