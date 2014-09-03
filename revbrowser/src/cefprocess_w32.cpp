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

#include <include/cef_app.h>

#include <Windows.h>

////////////////////////////////////////////////////////////////////////////////

extern bool MCCefCreateApp(CefRefPtr<CefApp> &r_app);

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
