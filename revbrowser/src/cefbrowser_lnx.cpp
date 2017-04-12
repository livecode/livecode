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

#include "revolution/external.h"

#include "cefbrowser.h"

#include <X11/Xlib.h>
#include <dlfcn.h>

////////////////////////////////////////////////////////////////////////////////

class MCCefLinuxBrowser : public MCCefBrowserBase
{
public:
	MCCefLinuxBrowser(Window p_parent_window);
	virtual ~MCCefLinuxBrowser(void);

	static bool GetXDisplay(Display *&r_display);
	bool GetXWindow(Window &r_window);

	virtual void PlatformConfigureWindow(CefWindowInfo &r_info);

	virtual bool PlatformSetVisible(bool p_visible);
	virtual bool PlatformGetVisible(bool &r_visible);

	virtual bool PlatformGetRect(int32_t &r_left, int32_t &r_top, int32_t &r_right, int32_t &r_bottom);
	virtual bool PlatformSetRect(int32_t p_left, int32_t p_top, int32_t p_right, int32_t p_bottom);
	virtual bool PlatformGetWindowID(uintptr_t &r_id);

	virtual bool PlatformGetAuthCredentials(bool p_is_proxy, const CefString &p_url, const CefString &p_realm, MCCefAuthScheme p_auth_scheme, CefString &r_user, CefString &r_password);

private:
	Window m_parent_window;
};

//////////

MCCefLinuxBrowser::MCCefLinuxBrowser(Window p_parent_window)
{
	m_parent_window = p_parent_window;
}

MCCefLinuxBrowser::~MCCefLinuxBrowser()
{
}

//////////

bool MCCefLinuxBrowser::GetXDisplay(Display *&r_display)
{
	void *t_display;
	int t_result;
	
	GetXDisplayHandle(&t_display, &t_result);
	
	if (t_result == EXTERNAL_FAILURE)
		return false;
		
	r_display = (Display*)t_display;
	
	return true;
}

bool MCCefLinuxBrowser::GetXWindow(Window &r_window)
{
	CefRefPtr<CefBrowser> t_browser;
	t_browser = GetCefBrowser();

	if (t_browser == nil)
		return false;

	Window t_window;
	t_window = t_browser->GetHost()->GetWindowHandle();

	if (t_window == None)
		return false;

	r_window = t_window;

	return true;
}

//////////

void MCCefLinuxBrowser::PlatformConfigureWindow(CefWindowInfo &r_info)
{
	r_info.SetAsChild(m_parent_window, CefRect(0,0,1,1));
}

bool MCCefLinuxBrowser::PlatformGetRect(int32_t &r_left, int32_t &r_top, int32_t &r_right, int32_t &r_bottom)
{
	Display *t_display;
	if (!GetXDisplay(t_display))
		return false;

	Window t_window;
	if (!GetXWindow(t_window))
		return false;

	Window t_root;
	int32_t t_win_x, t_win_y;
	uint32_t t_width, t_height, t_border_width, t_depth;

	XGetGeometry(t_display, t_window, &t_root, &t_win_x, &t_win_y, &t_width, &t_height, &t_border_width, &t_depth);

	r_left = t_win_x;
	r_top = t_win_y;
	r_right = t_win_x + t_width;
	r_bottom = t_win_y + t_height;

	return true;
}

bool MCCefLinuxBrowser::PlatformSetRect(int32_t p_left, int32_t p_top, int32_t p_right, int32_t p_bottom)
{
	Display *t_display;
	if (!GetXDisplay(t_display))
		return false;

	Window t_window;
	if (!GetXWindow(t_window))
		return false;

	XMoveResizeWindow(t_display, t_window, p_left, p_top, p_right - p_left, p_bottom - p_top);

	return true;
}

bool MCCefLinuxBrowser::PlatformGetVisible(bool &r_visible)
{
	Display *t_display;
	if (!GetXDisplay(t_display))
		return false;

	Window t_window;
	if (!GetXWindow(t_window))
		return false;

	XWindowAttributes t_attributes;
	
	XGetWindowAttributes(t_display, t_window, &t_attributes);
	
	r_visible = t_attributes.map_state != IsUnmapped;
	
	return true;
}

bool MCCefLinuxBrowser::PlatformSetVisible(bool p_visible)
{
	Display *t_display;
	if (!GetXDisplay(t_display))
		return false;

	Window t_window;
	if (!GetXWindow(t_window))
		return false;

	if (p_visible)
		XMapWindow(t_display, t_window);
	else
		XUnmapWindow(t_display, t_window);
		
	return true;
}

bool MCCefLinuxBrowser::PlatformGetWindowID(uintptr_t &r_id)
{
	r_id = uintptr_t(m_parent_window);
	return true;
}

bool MCCefLinuxBrowser::PlatformGetAuthCredentials(bool p_is_proxy, const CefString &p_url, const CefString &p_realm, MCCefAuthScheme p_auth_scheme, CefString &r_user, CefString &r_password)
{
	/* TODO - implement */
	return false;
}

//////////

void MCCefPlatformCloseBrowserWindow(CefRefPtr<CefBrowser> p_browser)
{
	Window t_window;
	t_window = p_browser->GetHost()->GetWindowHandle();
	
	Display *t_display;
	if (!MCCefLinuxBrowser::GetXDisplay(t_display))
		return;
	
	XDestroyWindow(t_display, t_window);
}

////////////////////////////////////////////////////////////////////////////////

bool MCCefPlatformCreateBrowser(int p_window_id, MCCefBrowserBase *&r_browser)
{
	MCCefLinuxBrowser *t_browser;
	t_browser = new (nothrow) MCCefLinuxBrowser((Window)p_window_id);
	
	if (t_browser == nil)
		return false;
	
	r_browser = t_browser;
	
	return true;
}

////////////////////////////////////////////////////////////////////////////////

