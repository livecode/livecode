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

#include <include/cef_app.h>

#include <Windows.h>

////////////////////////////////////////////////////////////////////////////////

extern bool MCCefCreateApp(CefRefPtr<CefApp> &r_app);

////////////////////////////////////////////////////////////////////////////////

// IM-2014-08-08: [[ Bug 12372 ]] Weak-linked SetProcessDPIAware function
typedef BOOL (WINAPI *SetProcessDPIAwarePtr)(VOID);
bool MCCefWin32SetProcessDPIAware(BOOL &r_result)
{
	static SetProcessDPIAwarePtr s_SetProcessDPIAware = NULL;
	static bool s_init = true;

	if (s_init)
	{
		s_SetProcessDPIAware = (SetProcessDPIAwarePtr)GetProcAddress(GetModuleHandleA("user32.dll"), "SetProcessDPIAware");
		s_init = false;
	}

	if (s_SetProcessDPIAware == NULL)
		return false;

	r_result = s_SetProcessDPIAware();

	return true;
}

bool MCCefPlatformEnableHiDPI()
{
	BOOL t_result;
	if (!MCCefWin32SetProcessDPIAware(t_result))
		return false;

	return t_result;
}

////////////////////////////////////////////////////////////////////////////////

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	CefMainArgs t_args(hInstance);
	
	CefRefPtr<CefApp> t_app;
	if (!MCCefCreateApp(t_app))
		return -1;
	
	return CefExecuteProcess(t_args, t_app, NULL);
}

////////////////////////////////////////////////////////////////////////////////
