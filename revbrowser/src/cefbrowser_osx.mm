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

#include "core.h"

#include <AppKit/AppKit.h>

#include <include/cef_app.h>

#include <objc/objc-runtime.h>

class MCCefBrowserOSX : public MCCefBrowserBase
{
public:
	MCCefBrowserOSX(CefWindowHandle p_parent_window);
	virtual ~MCCefBrowserOSX(void);
	
	bool GetWindowHandle(CefWindowHandle &r_hwnd);
	
	virtual void PlatformConfigureWindow(CefWindowInfo &r_info);
	
	virtual bool PlatformSetVisible(bool p_visible);
	virtual bool PlatformGetVisible(bool &r_visible);
	
	virtual bool PlatformGetRect(int32_t &r_left, int32_t &r_top, int32_t &r_right, int32_t &r_bottom);
	virtual bool PlatformSetRect(int32_t p_left, int32_t p_top, int32_t p_right, int32_t p_bottom);
	virtual bool PlatformGetWindowID(int32_t &r_id);
	
private:
	CefWindowHandle m_parent_window;
};

const char *MCCefPlatformGetSubProcessName(void)
{
	static char *s_exe_path = nil;
	
	if (s_exe_path == nil)
	{
		NSBundle *t_bundle;
		t_bundle = [NSBundle bundleWithIdentifier:@"com.runrev.revbrowser"];
		
		NSString *t_parent_path;
		t_parent_path = [[t_bundle bundlePath] stringByDeletingLastPathComponent];
		
		char *t_exe_path;
		t_exe_path = nil;
		
		// IM-2014-03-25: [[ revBrowserCEF ]] Look for subprocess executable in CEF subfolder
		bool t_success;
		t_success = MCCStringClone([t_parent_path cStringUsingEncoding:NSUTF8StringEncoding], t_exe_path);
		if (t_success)
			t_success = MCCStringAppend(t_exe_path,
										"/CEF/revbrowser-cefprocess.app/Contents/MacOS/revbrowser-cefprocess");
		
		if (t_success)
			s_exe_path = t_exe_path;
		else if (t_exe_path != nil)
			MCCStringFree(t_exe_path);
	}
	
	return s_exe_path;
}

// IM-2014-03-25: [[ revBrowserCEF ]] Return the path to the libcef.dylib library file
// located in CEF subfolder beside the revbrowser bundle
const char *MCCefPlatformGetCefLibraryPath(void)
{
	static char *s_lib_path = nil;
	
	if (s_lib_path == nil)
	{
		NSBundle *t_bundle;
		t_bundle = [NSBundle bundleWithIdentifier:@"com.runrev.revbrowser"];
		
		NSString *t_parent_path;
		t_parent_path = [[t_bundle bundlePath] stringByDeletingLastPathComponent];
		
		char *t_path;
		t_path = nil;
		
		bool t_success;
		t_success = MCCStringClone([t_parent_path cStringUsingEncoding:NSUTF8StringEncoding], t_path);
		if (t_success)
			t_success = MCCStringAppend(t_path,
										"/CEF/libcef.dylib");
		
		if (t_success)
			s_lib_path = t_path;
		else if (t_path != nil)
			MCCStringFree(t_path);
	}
	
	return s_lib_path;
}

// IM-2014-03-25: [[ revBrowserCEF ]] Can't change the locale path on OSX so return nil
const char *MCCefPlatformGetLocalePath(void)
{
	return nil;
}

bool MCCefPlatformCreateBrowser(int p_window_id, MCCefBrowserBase *&r_browser)
{
	NSWindow *t_app_window;
	t_app_window = [NSApp windowWithWindowNumber:p_window_id];
	
	if (t_app_window == nil)
		return false;
	
	MCCefBrowserOSX *t_browser;
	t_browser = new MCCefBrowserOSX([t_app_window contentView]);
	
	if (t_browser == nil)
		return false;
	
	r_browser = t_browser;
	
	return true;
}

void MCCefBrowserOSX::PlatformConfigureWindow(CefWindowInfo &r_info)
{
	r_info.SetAsChild(m_parent_window, 0,0,1,1);
    
    
	NSView *t_handle;
	if (!GetWindowHandle(t_handle))
		return false;
    
    t_handle = t_handle;
}

void MCCefPlatformCloseBrowserWindow(CefRefPtr<CefBrowser> p_browser)
{
	NSView *t_handle;
	t_handle = p_browser->GetHost()->GetWindowHandle();
	if (t_handle == nil)
		return;
	
	[t_handle removeFromSuperview];
}

MCCefBrowserOSX::MCCefBrowserOSX(CefWindowHandle p_parent_window) : MCCefBrowserBase()
{
	m_parent_window = p_parent_window;
}

MCCefBrowserOSX::~MCCefBrowserOSX(void)
{
}

static unsigned int cef_com_runrev_livecode_nativeViewId(id self, SEL _cmd)
{
    return 0xffffffffU;
}

bool MCCefBrowserOSX::GetWindowHandle(CefWindowHandle &r_hwnd)
{
	CefRefPtr<CefBrowser> t_browser;
	t_browser = GetCefBrowser();
	
	if (t_browser == nil)
		return false;
	
	CefWindowHandle t_handle;
	t_handle = t_browser->GetHost()->GetWindowHandle();
	
	if (t_handle == nil)
		return false;
    
    // MW-2014-04-10: [[ Bug 12047 ]] First time we get the window handle
    //   inject the method that the engine needs to process focus.
    if (![t_handle respondsToSelector: @selector(com_runrev_livecode_nativeViewId)])
    {
        Class t_class;
        t_class = object_getClass(t_handle);
        class_addMethod(t_class, @selector(com_runrev_livecode_nativeViewId), (IMP)cef_com_runrev_livecode_nativeViewId, "I@:");
    }
	
	r_hwnd = t_handle;
	
	return true;
}

bool MCCefBrowserOSX::PlatformGetRect(int32_t &r_left, int32_t &r_top, int32_t &r_right, int32_t &r_bottom)
{
	NSView *t_handle;
	if (!GetWindowHandle(t_handle))
		return false;
	
	NSRect t_rect;
	t_rect = [t_handle frame];
	
	NSRect t_win_rect;
	t_win_rect = [[[t_handle window] contentView] frame];
	
	r_left = t_rect.origin.x;
	r_top = t_win_rect.size.height - t_rect.origin.y;
	r_right = r_left + t_rect.size.width;
	r_bottom = r_top + t_rect.size.height;
	
	return true;
}

bool MCCefBrowserOSX::PlatformSetRect(int32_t p_left, int32_t p_top, int32_t p_right, int32_t p_bottom)
{
	NSView *t_handle;
	if (!GetWindowHandle(t_handle))
		return false;
	
	NSRect t_win_rect;
	t_win_rect = [[[t_handle window] contentView] frame];
	
	NSRect t_rect;
	t_rect = NSMakeRect(p_left, t_win_rect.size.height - p_bottom, p_right - p_left, p_bottom - p_top);
	
	[t_handle setFrame:t_rect];
	
	return true;
}

bool MCCefBrowserOSX::PlatformGetVisible(bool &r_visible)
{
	NSView *t_handle;
	if (!GetWindowHandle(t_handle))
		return false;
	
	r_visible = ![t_handle isHidden];
	
	return true;
}

bool MCCefBrowserOSX::PlatformSetVisible(bool p_visible)
{
	NSView *t_handle;
	if (!GetWindowHandle(t_handle))
		return false;
	
	[t_handle setHidden:!p_visible];
	
	return true;
}

bool MCCefBrowserOSX::PlatformGetWindowID(int32_t &r_id)
{
	r_id = (int32_t) [[m_parent_window window] windowNumber];
	
	return true;
}
