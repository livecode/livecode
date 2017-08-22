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

////////////////////////////////////////////////////////////////////////////////

#include "prefix.h"
#include "foundation.h"
#include "w32compat.h"

// IM-2014-08-08: [[ Bug 12372 ]] Weak-linked QueryActCtxSettingsW function
typedef BOOL (WINAPI *QueryActCtxSettingsWPtr)(DWORD p_flags, HANDLE p_act_ctx,
											   PCWSTR p_settings_namespace,
											   PCWSTR p_setting_name,
											   PWSTR p_buffer, SIZE_T p_buffer_size,
											   SIZE_T *r_out_size);
bool MCWin32QueryActCtxSettingsW(BOOL &r_result, DWORD p_flags, HANDLE p_act_ctx,
								 PCWSTR p_settings_namespace,
								 PCWSTR p_setting_name,
								 PWSTR p_buffer, SIZE_T p_buffer_size,
								 SIZE_T *r_out_size)
{
	static QueryActCtxSettingsWPtr s_QueryActCtxSettingsW = NULL;
	static bool s_init = true;

	if (s_init)
	{
		s_QueryActCtxSettingsW = (QueryActCtxSettingsWPtr)GetProcAddress(GetModuleHandleA("kernel32.dll"), "QueryActCtxSettingsW");
		s_init = false;
	}

	if (s_QueryActCtxSettingsW == NULL)
		return false;

	r_result = s_QueryActCtxSettingsW(p_flags, p_act_ctx, p_settings_namespace,
		p_setting_name, p_buffer, p_buffer_size, r_out_size);

	return true;
}

//////////

// IM-2014-08-08: [[ Bug 12372 ]] Weak-linked SetProcessDPIAware function
typedef BOOL (WINAPI *SetProcessDPIAwarePtr)(VOID);
bool MCWin32SetProcessDPIAware(BOOL &r_result)
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

//////////

// IM-2014-01-28: [[ HiDPI ]] Weak-linked IsProcessDPIAware function
typedef BOOL (WINAPI *IsProcessDPIAwarePtr)(VOID);
bool MCWin32IsProcessDPIAware(BOOL &r_result)
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

//////////

// IM-2014-01-28: [[ HiDPI ]] Weak-linked GetProcessDPIAwareness function
typedef HRESULT (WINAPI *GetProcessDpiAwarenessPTR)(HANDLE hprocess,
													MCWin32ProcessDpiAwareness *value);
bool MCWin32GetProcessDpiAwareness(HRESULT &r_result, HANDLE p_hprocess,
								   MCWin32ProcessDpiAwareness *r_awareness)
{
	static GetProcessDpiAwarenessPTR s_GetProcessDpiAwareness = NULL;
	static bool s_init = true;

	if (s_init)
	{
		s_GetProcessDpiAwareness = (GetProcessDpiAwarenessPTR)GetProcAddress(GetModuleHandleA("shcore.dll"), "GetProcessDpiAwareness");
		s_init = false;
	}

	if (s_GetProcessDpiAwareness == NULL)
		return false;

	r_result = s_GetProcessDpiAwareness(NULL, r_awareness);

	return true;
}

//////////

// IM-2014-01-28: [[ HiDPI ]] Weak-linked GetDPIForMonitor function
typedef HRESULT (WINAPI *GetDpiForMonitorPTR)(HMONITOR hmonitor,
											  MCWin32MonitorDpiType dpiType,
											  UINT *dpiX, UINT *dpiY);
bool MCWin32GetDpiForMonitor(HRESULT &r_result, HMONITOR p_monitor,
							 MCWin32MonitorDpiType p_dpi_type,
							 UINT *r_dpi_x, UINT *r_dpi_y)
{
	static GetDpiForMonitorPTR s_GetDpiForMonitor = NULL;
	static bool s_init = true;

	if (s_init)
	{
		s_GetDpiForMonitor = (GetDpiForMonitorPTR)GetProcAddress(GetModuleHandleA("shcore.dll"), "GetDpiForMonitor");
		s_init = false;
	}

	if (s_GetDpiForMonitor == NULL)
		return false;

	r_result = s_GetDpiForMonitor(p_monitor, p_dpi_type, r_dpi_x, r_dpi_y);

	return true;
}

////////////////////////////////////////////////////////////////////////////////

bool MCWin32QueryActCtxSettings(const unichar_t *p_settings_name, unichar_t *&r_value)
{
	bool t_success;
	t_success = true;

	BOOL t_result;
	SIZE_T t_size;

	unichar_t *t_buffer;
	t_buffer = NULL;

	uint32_t t_buffer_size;
	t_buffer_size = 0;

	t_success = MCWin32QueryActCtxSettingsW(t_result, 0, NULL, NULL, p_settings_name, NULL, 0, &t_size);

	if (t_success)
	{
		DWORD t_error;
		t_error = GetLastError();
		t_success = t_result != FALSE;
		t_success = t_result == FALSE && GetLastError() == ERROR_INSUFFICIENT_BUFFER;
	}

	if (t_success)
	{
		t_buffer_size = t_size;
		t_success = MCMemoryNewArray(t_buffer_size, t_buffer);
	}

	if (t_success)
		t_success = MCWin32QueryActCtxSettingsW(t_result, 0, NULL, NULL, p_settings_name, t_buffer, t_buffer_size, &t_size);

	if (t_success)
		t_success = t_result != FALSE;

	if (t_success)
		r_value = t_buffer;
	else
		MCMemoryDeleteArray(t_buffer);

	return t_success;
}

////////////////////////////////////////////////////////////////////////////////
