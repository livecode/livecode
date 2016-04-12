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

extern "C" int initialise_weak_link_cef(void);

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	// IM-2014-03-18: [[ revBrowserCEF ]] Initialise dynamically loaded cef library
	if (!initialise_weak_link_cef())
		return -1;

	CefMainArgs t_args(hInstance);
	
	CefRefPtr<CefApp> t_app;
	if (!MCCefCreateApp(t_app))
		return -1;
	
	return CefExecuteProcess(t_args, t_app, NULL);
}

////////////////////////////////////////////////////////////////////////////////

// AL-2015-02-17: [[ SB Inclusions ]] Work around problems linking to MCU_ functions from CEF
extern "C"
{

void *MCU_loadmodule(const char *p_source)
{
    return LoadLibraryA(p_source);
}

void MCU_unloadmodule(void *p_module)
{
    
}

void *MCU_resolvemodulesymbol(void *p_module, const char *p_name)
{
    return GetProcAddress((HMODULE)p_module, p_name);
}
    
}

////////////////////////////////////////////////////////////////////////////////
