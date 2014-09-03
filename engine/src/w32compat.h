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

/*

LiveCode is currently built agains Windows SDK 6.1.

To support features introduced in subsequent SDK versions, this header defines
structures and weak-linked functions required to use those newer features.

*/

////////////////////////////////////////////////////////////////////////////////

typedef enum __MCWin32ProcessDPIAwareness
{
	kMCWin32ProcessDPIUnaware,
	kMCWin32ProcessSystemDPIAware,
	kMCWin32ProcessPerMonitorDPIAware,
} MCWin32ProcessDPIAwareness;

typedef enum __MCWin32MonitorDPIType
{
	kMCWin32MDTEffectiveDPI,
	kMCWin32MDTAngularDPI,
	kMCWin32MDTRawDPI,
	kMCWin32MDTDefault = kMCWin32MDTEffectiveDPI,
} MCWin32MonitorDPIType;

////////////////////////////////////////////////////////////////////////////////

// Windows Vista, kernel32.dll
bool MCWin32QueryActCtxSettingsW(BOOL &r_result, DWORD p_flags, HANDLE p_act_ctx,
								 PCWSTR p_settings_namespace,
								 PCWSTR p_setting_name,
								 PWSTR p_buffer, SIZE_T p_buffer_size,
								 SIZE_T *r_out_size);

// Windows Vista, user32.dll
bool MCWin32SetProcessDPIAware(BOOL &r_result);

// Windows Vista, user32.dll
bool MCWin32IsProcessDPIAware(BOOL &r_result);

// Windows 8.1, shcore.dll
bool MCWin32GetProcessDPIAwareness(HRESULT &r_result, HANDLE p_hprocess,
								   MCWin32ProcessDPIAwareness *r_awareness);

// Windows 8.1, shcore.dll
bool MCWin32GetDPIForMonitor(HRESULT &r_result, HMONITOR p_monitor,
							 MCWin32MonitorDPIType p_dpi_type,
							 UINT *r_dpi_x, UINT *r_dpi_y);

////////////////////////////////////////////////////////////////////////////////

// Convienience functions

bool MCWin32QueryActCtxSettings(const unichar_t *p_settings_name, unichar_t *&r_value);
